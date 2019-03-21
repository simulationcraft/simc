// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE
// ==========================================================================
// Druid
// ==========================================================================

/*
  BfA TODO:

  Feral =====================================================================
  Audit abilites
  Feral Frenzy
  Finish Azerite Traits

  Balance ===================================================================
  Still need AP Coeff for Treants

  Guardian ==================================================================
  Catweaving APL

  Resto =====================================================================

  Needs Documenting =========================================================
  predator_rppm option
  initial_astral_power option
  initial_moon_stage option
  moon stage expressions
*/

// Forward declarations
struct druid_t;

// Active actions
struct stalwart_guardian_t;
namespace spells {
struct moonfire_t;
struct starshards_t;
struct shooting_stars_t;
struct solar_empowerment_t;
}
namespace heals {
struct cenarion_ward_hot_t;
}
namespace cat_attacks {
struct gushing_wound_t;
}
namespace bear_attacks {
struct bear_attack_t;
}

enum form_e {
  CAT_FORM       = 0x1,
  NO_FORM        = 0x2,
  TRAVEL_FORM    = 0x4,
  AQUATIC_FORM   = 0x8, // Legacy
  BEAR_FORM      = 0x10,
  DIRE_BEAR_FORM = 0x40, // Legacy
  MOONKIN_FORM   = 0x40000000,
  MOONKIN_FORM_AFFINITY = 0x40000001, //Under Development Jan 08 2017
};

enum moon_stage_e {
  NEW_MOON,
  HALF_MOON,
  FULL_MOON,
  FREE_FULL_MOON,
};

//Azerite Trait 
enum streaking_stars_e {
  SS_NONE,
  // Spells
  SS_LUNAR_STRIKE,
  SS_SOLAR_WRATH,
  SS_MOONFIRE,
  SS_SUNFIRE,
  SS_STARSURGE,
  SS_STELLAR_FLARE,
  SS_NEW_MOON,
  SS_HALF_MOON,
  SS_FULL_MOON,
  //These target the last hit enemy in game
  SS_STARFALL,
  SS_FORCE_OF_NATURE,
  SS_FURY_OF_ELUNE,
  SS_CELESTIAL_ALIGNMENT,
};

struct druid_td_t : public actor_target_data_t
{
  struct dots_t
  {
    dot_t* fury_of_elune;
    dot_t* gushing_wound;
    dot_t* lifebloom;
    dot_t* moonfire;
    dot_t* rake;
    dot_t* regrowth;
    dot_t* rejuvenation;
    dot_t* rip;
    dot_t* stellar_flare;
    dot_t* sunfire;
    dot_t* starfall;
    dot_t* thrash_cat;
    dot_t* thrash_bear;
    dot_t* wild_growth;
  } dots;

  struct buffs_t
  {
    buff_t* lifebloom;
  } buff;

  struct debuffs_t
  {
    buff_t* bloodletting; 
  } debuff;

  druid_td_t( player_t& target, druid_t& source );

  bool hot_ticking()
  {
    return dots.regrowth      -> is_ticking() ||
           dots.rejuvenation  -> is_ticking() ||
           dots.lifebloom     -> is_ticking() ||
           dots.wild_growth   -> is_ticking();
  }

  unsigned feral_tier19_4pc_bleeds()
  {
     return dots.rip->is_ticking()
           + dots.rake -> is_ticking()
           + dots.thrash_cat -> is_ticking();
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
    for ( auto& elem : b )
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
private:
  form_e form; // Active druid form
public:
  moon_stage_e moon_stage;

  // counters for snapshot tracking
  std::vector<snapshot_counter_t*> counters;

  double starshards;
  double expected_max_health; // For Bristling Fur calculations.

  //Azerite
  streaking_stars_e previous_streaking_stars;
  streaking_stars_e previous_streaking_stars_cast;

  // RPPM objects
  struct rppms_t
  {
    // Feral
    real_ppm_t* predator;  // Optional RPPM approximation
    real_ppm_t* blood_mist;  // Azerite trait
    
    // Balance
    real_ppm_t* power_of_the_moon;
  } rppm;

  // Options
  double predator_rppm_rate;
  double initial_astral_power;
  int    initial_moon_stage;
  int    lively_spirit_stacks; //to set how many spells a healer will cast during Innervate
  bool ahhhhh_the_great_outdoors;
  bool catweave_bear;
  bool t21_2pc;
  bool t21_4pc;

  struct active_actions_t
  {
    stalwart_guardian_t*            stalwart_guardian;
    action_t* gushing_wound;
    heals::cenarion_ward_hot_t*     cenarion_ward_hot;
    action_t* brambles;
    action_t* brambles_pulse;
    spell_t*  galactic_guardian;
    action_t* natures_guardian;
    spell_t*  shooting_stars;
    spell_t*  starfall;
    spell_t* solar_empowerment;
    spell_t* fury_of_elune;
    spell_t*  starshards;
    action_t* yseras_gift;
    spell_t* lunar_shrapnel;
    spell_t* streaking_stars;
  } active;

  // Pets
  std::array<pet_t*, 3> force_of_nature;
  // Auto-attacks
  weapon_t caster_form_weapon;
  weapon_t cat_weapon;
  weapon_t bear_weapon;
  melee_attack_t* caster_melee_attack;
  melee_attack_t* cat_melee_attack;
  melee_attack_t* bear_melee_attack;
  double sylvan_walker;

  double equipped_weapon_dps;

  //other
  spells::solar_empowerment_t* solar_empowerment;

  // Druid Events
  std::vector<event_t*> persistent_buff_delay;

  // Azerite
  struct azerite_t
  {  // Not yet implemented
     // Balance
    azerite_power_t long_night; //seems to be removed

    // Guardian
    azerite_power_t craggy_bark; // Low Priority
    azerite_power_t gory_regeneration; // Low Priority

    // Implemented
    // Balance
    azerite_power_t lively_spirit;
    azerite_power_t dawning_sun;
    azerite_power_t lunar_shrapnel;
    azerite_power_t power_of_the_moon;
    azerite_power_t high_noon;
    azerite_power_t streaking_stars;
    azerite_power_t arcanic_pulsar;
    // Feral
    azerite_power_t blood_mist; //check spelldata
    azerite_power_t gushing_lacerations; //check spelldata
    azerite_power_t iron_jaws; //-||-
    azerite_power_t primordial_rage; //-||-
    azerite_power_t raking_ferocity; //-||-
    azerite_power_t shredding_fury; //-||-
    azerite_power_t wild_fleshrending; //-||-
    azerite_power_t jungle_fury;
    azerite_power_t untamed_ferocity;

    // Guardian
    azerite_power_t guardians_wrath;
    azerite_power_t layered_mane; // TODO: check if second Ironfur benefits from Guardian of Elune
    azerite_power_t masterful_instincts;
    azerite_power_t twisted_claws;
    azerite_power_t burst_of_savagery;

  } azerite;

  // Buffs
  struct buffs_t
  {
    // General
    buff_t* bear_form;
    buff_t* cat_form;
    buff_t* dash;
    buff_t* tiger_dash;
    buff_t* cenarion_ward;
    buff_t* clearcasting;
    buff_t* prowl;
    buff_t* stampeding_roar;
    buff_t* wild_charge_movement;
    buff_t* sephuzs_secret;
    buff_t* innervate;

    // Balance
    buff_t* natures_balance;
    buff_t* celestial_alignment;
    buff_t* fury_of_elune; // Astral Power Gain
    buff_t* incarnation_moonkin;
    buff_t* lunar_empowerment;
    buff_t* moonkin_form;
    buff_t* oneths_intuition; // Legion Legendary
    buff_t* oneths_overconfidence; // Legion Legendary
    buff_t* power_of_elune; // Legion Legendary
    buff_t* solar_empowerment;
    buff_t* the_emerald_dreamcatcher; // Legion Legendary
    buff_t* warrior_of_elune;
    buff_t* owlkin_frenzy;
    buff_t* solar_solstice; //T21 4P Balance
    buff_t* starfall;
    buff_t* astral_acceleration; //T20 4P Balance
    buff_t* starlord; //talent 
    buff_t* stellar_drift_2; // stellar drift mobility buff ID 202461

    // Feral
    buff_t* apex_predator;
    buff_t* berserk;
    buff_t* bloodtalons;
    //buff_t* fiery_red_maimers; // Legion Legendary
    buff_t* incarnation_cat;
    buff_t* predatory_swiftness;
    buff_t* savage_roar;
    buff_t* scent_of_blood;
    buff_t* tigers_fury;
    buff_t* feral_tier17_4pc;
    buff_t* fury_of_ashamane;
    buff_t* jungle_stalker;

    // Guardian
    buff_t* barkskin;
    buff_t* bristling_fur;
    buff_t* earthwarden;
    buff_t* earthwarden_driver;
    buff_t* galactic_guardian;
    buff_t* gore;
    buff_t* guardian_of_elune;
    buff_t* incarnation_bear;
    buff_t* ironfur;
    buff_t* oakhearts_puny_quods; // Legion Legendary
    buff_t* pulverize;
    buff_t* survival_instincts;
    buff_t* guardian_tier17_4pc;
    buff_t* guardian_tier19_4pc;
    buff_t* guardians_wrath;
    buff_t* masterful_instincts;
    buff_t* twisted_claws;
    buff_t* burst_of_savagery;

    // Restoration
    buff_t* incarnation_tree;
    buff_t* soul_of_the_forest; // needs checking
    buff_t* yseras_gift;
    buff_t* harmony; // NYI
    buff_t* moonkin_form_affinity;

    // Azerite
    buff_t* dawning_sun;
    buff_t* lively_spirit;
    buff_t* shredding_fury;
    buff_t* iron_jaws;
    buff_t* raking_ferocity;
    buff_t* arcanic_pulsar;
    buff_t* jungle_fury;

  } buff;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* berserk;
    cooldown_t* celestial_alignment;
    cooldown_t* innervate;
    cooldown_t* growl;
    cooldown_t* incarnation;
    cooldown_t* mangle;
    cooldown_t* thrash_bear;
    cooldown_t* maul;
    cooldown_t* moon_cd; // New / Half / Full Moon
    cooldown_t* wod_pvp_4pc_melee;
    cooldown_t* swiftmend;
    cooldown_t* tigers_fury;
    cooldown_t* warrior_of_elune;
    cooldown_t* barkskin;
    cooldown_t* rage_from_melees;
  } cooldown;

  // Gains
  struct gains_t
  {
    // Multiple Specs / Forms
    gain_t* clearcasting;       // Feral & Restoration
    gain_t* soul_of_the_forest; // Feral & Guardian

    // Balance
    gain_t* natures_balance; //talent
    gain_t* force_of_nature;
    gain_t* warrior_of_elune;
    gain_t* fury_of_elune;
    gain_t* stellar_flare;
    gain_t* lunar_strike;
    gain_t* moonfire;
    gain_t* shooting_stars;
    gain_t* solar_wrath;
    gain_t* sunfire;
    gain_t* celestial_alignment;
    gain_t* incarnation;
    gain_t* arcanic_pulsar;

    // Feral (Cat)
    gain_t* brutal_slash;
    gain_t* energy_refund;
    gain_t* feral_frenzy;
    gain_t* primal_fury;
    gain_t* rake;
    gain_t* shred;
    gain_t* swipe_cat;
    gain_t* tigers_fury;
    gain_t* feral_tier17_2pc;
    gain_t* feral_tier18_4pc;
    gain_t* feral_tier19_2pc;
    gain_t* gushing_lacerations;

    // Guardian (Bear)
    gain_t* bear_form;
    gain_t* blood_frenzy;
    gain_t* brambles;
    gain_t* bristling_fur;
    gain_t* galactic_guardian;
    gain_t* gore;
    gain_t* stalwart_guardian;
    gain_t* rage_refund;
    gain_t* guardian_tier17_2pc;
    gain_t* guardian_tier18_2pc;
    gain_t* oakhearts_puny_quods;
    gain_t* rage_from_melees;
  } gain;

  // Masteries
  struct masteries_t
  {
    // Done
    const spell_data_t* natures_guardian;
    const spell_data_t* natures_guardian_AP;
    const spell_data_t* razor_claws;
    const spell_data_t* starlight;

    // NYI / TODO!
    const spell_data_t* harmony;
  } mastery;

  // Procs
  struct procs_t
  {
    // Feral & Resto
    proc_t* clearcasting;
    proc_t* clearcasting_wasted;

    // Feral
    proc_t* the_wildshapers_clutch;
    proc_t* predator;
    proc_t* predator_wasted;
    proc_t* primal_fury;
    proc_t* tier17_2pc_melee;
    proc_t* blood_mist;
    proc_t* gushing_lacerations;

    // Balance
    proc_t* starshards;
    proc_t* power_of_the_moon;
    proc_t* streaking_stars_gain; // extra proc from in-flight spells
    proc_t* streaking_stars_loss; // munched proc from spell travel timing

    // Guardian
    proc_t* gore;
  } proc;

  // Class Specializations
  struct specializations_t
  {
    // Generic
    const spell_data_t* druid;
    const spell_data_t* critical_strikes;       // Feral & Guardian
    const spell_data_t* leather_specialization; // All Specializations
    const spell_data_t* omen_of_clarity;        // Feral & Restoration
    const spell_data_t* innervate;              // Balance & Restoration

    // Feral
    const spell_data_t* feral;
    const spell_data_t* feral_overrides;
    const spell_data_t* feral_overrides2;
    const spell_data_t* cat_form; // Cat form hidden effects
    const spell_data_t* cat_form_speed;
    const spell_data_t* feline_swiftness; // Feral Affinity
    const spell_data_t* gushing_wound; // tier17_4pc
    const spell_data_t* bloody_gash; //tier21_2pc
    const spell_data_t* predatory_swiftness;
    const spell_data_t* primal_fury;
    const spell_data_t* rip;
    const spell_data_t* sharpened_claws;
    const spell_data_t* swipe_cat;
    const spell_data_t* rake_2;
    const spell_data_t* tigers_fury;
    const spell_data_t* tigers_fury_2;
    const spell_data_t* ferocious_bite_2;
    const spell_data_t* shred;
    const spell_data_t* shred_2;
    const spell_data_t* shred_3;
    const spell_data_t* swipe_2;
    const spell_data_t* savage_roar; // Hidden buff

    // Balance
    const spell_data_t* balance;
    const spell_data_t* eclipse;
    const spell_data_t* astral_influence; // Balance Affinity
    const spell_data_t* astral_power;
    const spell_data_t* celestial_alignment;
    const spell_data_t* moonkin_form;
    const spell_data_t* starfall;
    const spell_data_t* balance_tier19_2pc;
    const spell_data_t* starsurge_2;
    const spell_data_t* moonkin_2;
    const spell_data_t* sunfire_2;
    const spell_data_t* stellar_drift_2; // stellar drift mobility buff ID 202461
    const spell_data_t* owlkin_frenzy;
    const spell_data_t* lunar_empowerment;
    const spell_data_t* solar_empowerment;
    const spell_data_t* solar_wrath_empowerment;
    const spell_data_t* sunfire_dmg;
    const spell_data_t* moonfire_dmg;
    const spell_data_t* shooting_stars_dmg;
    const spell_data_t* half_moon;
    const spell_data_t* full_moon;

    // Guardian
    const spell_data_t* guardian;
    const spell_data_t* guardian_overrides;
    const spell_data_t* bear_form;
    const spell_data_t* gore;
    const spell_data_t* ironfur;
    const spell_data_t* thick_hide; // Guardian Affinity
    const spell_data_t* thrash_bear_dot; // For Rend and Tear modifier
    const spell_data_t* lightning_reflexes;
    const spell_data_t* mangle_2; // Rank 2
    const spell_data_t* ironfur_2; // Rank 2
    const spell_data_t* frenzied_regeneration_2; // Rank 2

    // Resto
    const spell_data_t* restoration;
    const spell_data_t* yseras_gift; // Restoration Affinity
    const spell_data_t* moonkin_form_affinity;
  } spec;

  // Talents
  struct talents_t
  {
    // Multiple Specs
    const spell_data_t* renewal;
    const spell_data_t* tiger_dash;
    const spell_data_t* wild_charge;

    const spell_data_t* balance_affinity;
    const spell_data_t* feral_affinity;
    const spell_data_t* guardian_affinity;
    const spell_data_t* restoration_affinity;

    const spell_data_t* mighty_bash;
    const spell_data_t* mass_entanglement;
    const spell_data_t* typhoon;

    const spell_data_t* soul_of_the_forest;
    const spell_data_t* moment_of_clarity;

    // Feral
    const spell_data_t* predator;
    const spell_data_t* blood_scent;
    const spell_data_t* lunar_inspiration;

    const spell_data_t* incarnation_cat;
    const spell_data_t* jagged_wounds; 

    const spell_data_t* sabertooth;
    const spell_data_t* brutal_slash;
    const spell_data_t* savage_roar;
    
    const spell_data_t* bloodtalons;
    const spell_data_t* feral_frenzy;
    const spell_data_t* scent_of_blood;
    const spell_data_t* primal_wrath;

    // Balance
    const spell_data_t* natures_balance;
    const spell_data_t* warrior_of_elune;
    const spell_data_t* force_of_nature;

    const spell_data_t* starlord;
    const spell_data_t* incarnation_moonkin;

    const spell_data_t* twin_moons;
    const spell_data_t* stellar_drift;
    const spell_data_t* stellar_flare;

    const spell_data_t* shooting_stars;
    const spell_data_t* fury_of_elune;
    const spell_data_t* new_moon;

    // Guardian
    const spell_data_t* brambles;
    const spell_data_t* pulverize;
    const spell_data_t* blood_frenzy;

    const spell_data_t* gutteral_roars;

    const spell_data_t* incarnation_bear;
    const spell_data_t* galactic_guardian;

    const spell_data_t* earthwarden;
    const spell_data_t* guardian_of_elune;
    const spell_data_t* survival_of_the_fittest;

    const spell_data_t* rend_and_tear;
    const spell_data_t* lunar_beam;
    const spell_data_t* bristling_fur;

    // Restoration
    const spell_data_t* verdant_growth;
    const spell_data_t* cenarion_ward;
    const spell_data_t* germination;

    const spell_data_t* incarnation_tree;
    const spell_data_t* cultivation;

    const spell_data_t* prosperity;
    const spell_data_t* inner_peace;
    const spell_data_t* profusion;

    const spell_data_t* stonebark;
    const spell_data_t* flourish;
  } talent;

  struct legendary_t
  {
    // General
    const spell_data_t* sephuzs_secret = spell_data_t::not_found();
    double LegendaryDamageMod = 0.0;

    // Balance
    timespan_t impeccable_fel_essence;

    // Feral
    double the_wildshapers_clutch;
    double behemoth_headdress = 0;
    timespan_t ailuro_pouncers;

    // Guardian
    double fury_of_nature = 0;
  } legendary;

  druid_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, DRUID, name, r ),
    form( NO_FORM ),
    starshards( 0.0 ),
    previous_streaking_stars(SS_NONE),
    previous_streaking_stars_cast(SS_NONE),
    predator_rppm_rate( 0.0 ),
    initial_astral_power( 8 ),
    initial_moon_stage( NEW_MOON ),
    lively_spirit_stacks(9),  //set a usually fitting default value
    ahhhhh_the_great_outdoors( false ),
    catweave_bear( false ),
    t21_2pc(false),
    t21_4pc(false),
    active( active_actions_t() ),
    force_of_nature(),
    caster_form_weapon(),
    caster_melee_attack( nullptr ),
    cat_melee_attack( nullptr ),
    bear_melee_attack( nullptr ),
    buff( buffs_t() ),
    cooldown( cooldowns_t() ),
    gain( gains_t() ),
    mastery( masteries_t() ),
    proc( procs_t() ),
    spec( specializations_t() ),
    talent( talents_t() ),
    legendary( legendary_t() )
  {
    cooldown.berserk             = get_cooldown( "berserk"             );
    cooldown.celestial_alignment = get_cooldown( "celestial_alignment" );
    cooldown.growl               = get_cooldown( "growl"               );
    cooldown.incarnation         = get_cooldown( "incarnation"         );
    cooldown.mangle              = get_cooldown( "mangle"              );
    cooldown.thrash_bear         = get_cooldown( "thrash_bear"         );
    cooldown.maul                = get_cooldown( "maul"                );
    cooldown.moon_cd             = get_cooldown( "moon_cd"             );
    cooldown.wod_pvp_4pc_melee   = get_cooldown( "wod_pvp_4pc_melee"   );
    cooldown.swiftmend           = get_cooldown( "swiftmend"           );
    cooldown.tigers_fury         = get_cooldown( "tigers_fury"         );
    cooldown.warrior_of_elune    = get_cooldown( "warrior_of_elune"    );
    cooldown.barkskin            = get_cooldown( "barkskin"            );
    cooldown.innervate           = get_cooldown( "innervate"           );
    cooldown.rage_from_melees    = get_cooldown( "rage_from_melees"    );

    cooldown.wod_pvp_4pc_melee -> duration = timespan_t::from_seconds( 30.0 );
    cooldown.rage_from_melees -> duration  = timespan_t::from_seconds( 1.0 );

    legendary.the_wildshapers_clutch = 0.0;
    sylvan_walker = 0;
    equipped_weapon_dps = 0;

    regen_type = REGEN_DYNAMIC;
    regen_caches[ CACHE_HASTE ] = true;
    regen_caches[ CACHE_ATTACK_HASTE ] = true;

    talent_points.register_validity_fn( [ this ]( const spell_data_t* spell ) {
       return strcmp( spell->name_cstr() , "Soul of the Forest" ) == 0 && find_item_by_id( 151636 ) != nullptr;
    } );
  }

  virtual           ~druid_t();

  // Character Definition
  virtual void      activate() override;
  virtual void      init() override;
  virtual void      init_absorb_priority() override;
  virtual void      init_action_list() override;
  virtual void      init_base_stats() override;
  virtual void      init_gains() override;
  virtual void      init_procs() override;
  virtual void      init_resources( bool ) override;
  virtual void      init_rng() override;
  virtual void      init_spells() override;
  virtual void      init_scaling() override;
  virtual void      create_buffs() override;
  std::string       default_flask() const override;
  std::string       default_potion() const override;
  std::string       default_food() const override;
  std::string       default_rune() const override;
  virtual void      invalidate_cache( cache_e ) override;
  virtual void      arise() override;
  virtual void      reset() override;
  virtual void      merge( player_t& other ) override;
  virtual timespan_t available() const override;
  virtual double    composite_armor() const override;
  virtual double    composite_armor_multiplier() const override;
  virtual double    composite_melee_attack_power() const override;
  virtual double    composite_attack_power_multiplier() const override;
  virtual double    composite_attribute( attribute_e attr ) const override;
  virtual double    composite_attribute_multiplier( attribute_e attr ) const override;
  virtual double    composite_block() const override { return 0; }
  virtual double    composite_crit_avoidance() const override;
  virtual double    composite_dodge() const override;
  virtual double    composite_dodge_rating() const override;
  virtual double    composite_damage_versatility_rating() const override;
  virtual double    composite_heal_versatility_rating() const override;
  virtual double    composite_mitigation_versatility_rating() const override;
  virtual double    composite_leech() const override;
  virtual double    composite_melee_crit_chance() const override;
  virtual double    composite_melee_haste() const override;
  virtual double    composite_melee_expertise( const weapon_t* ) const override;
  virtual double    composite_parry() const override { return 0; }
  virtual double    composite_player_multiplier( school_e school ) const override;
  virtual double    composite_rating_multiplier( rating_e ) const override;
  virtual double    composite_spell_crit_chance() const override;
  virtual double    composite_spell_haste() const override;
  virtual double    composite_spell_power( school_e school ) const override;
  virtual double    composite_spell_power_multiplier() const override;
  virtual double    temporary_movement_modifier() const override;
  virtual double    passive_movement_modifier() const override;
  virtual double    matching_gear_multiplier( attribute_e attr ) const override;
  virtual expr_t*   create_expression( const std::string& name ) override;
  virtual action_t* create_action( const std::string& name, const std::string& options ) override;
  virtual pet_t*    create_pet   ( const std::string& name, const std::string& type = std::string() ) override;
  virtual void      create_pets() override;
  virtual resource_e primary_resource() const override;
  virtual role_e    primary_role() const override;
  virtual stat_e    convert_hybrid_stat( stat_e s ) const override;
  virtual double    resource_regen_per_second( resource_e ) const override;
  virtual void      target_mitigation( school_e, dmg_e, action_state_t* ) override;
  virtual void      assess_damage( school_e, dmg_e, action_state_t* ) override;
  virtual void      assess_damage_imminent_pre_absorb( school_e, dmg_e, action_state_t* ) override;
  virtual void      assess_heal( school_e, dmg_e, action_state_t* ) override;
  virtual void      recalculate_resource_max( resource_e ) override;
  virtual void      create_options() override;
  virtual std::string      create_profile( save_e type ) override;
  virtual druid_td_t* get_target_data( player_t* target ) const override;
  virtual void      copy_from( player_t* ) override;
  virtual void output_json_report(js::JsonOutput& /* root */) const override;

  form_e get_form() const { return form; }
  void shapeshift( form_e );
  void init_beast_weapon( weapon_t&, double );
  const spell_data_t* find_affinity_spell( const std::string& ) const;
  specialization_e get_affinity_spec() const;
  void trigger_natures_guardian( const action_state_t* );
  void trigger_solar_empowerment (const action_state_t*);
  double calculate_expected_max_health() const;
  const spelleffect_data_t* query_aura_effect( const spell_data_t* aura_spell, effect_type_t type = E_APPLY_AURA,
                                               effect_subtype_t subtype = A_ADD_PCT_MODIFIER, double misc_value = 0,
                                               const spell_data_t* affected_spell = spell_data_t::nil() );

private:
  void              apl_precombat();
  void              apl_default();
  void              apl_feral();
  void              apl_balance();
  void              apl_guardian();
  void              apl_restoration();

  target_specific_t<druid_td_t> target_data;
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

// Stalwart Guardian ( 6.2 T18 Guardian Trinket) ============================

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

  stalwart_guardian_t( const special_effect_t& effect ) :
    absorb_t( "stalwart_guardian_bg", effect.player, effect.player -> find_spell( 185321 ) ),
    incoming_damage( 0 ), absorb_size( 0 )
  {
    background = quiet = true;
    may_crit = false;
    target = player;
    harmful = false;
    attack_power_mod.direct = effect.driver() -> effectN( 1 ).average( effect.item ) / 100.0;
    absorb_limit = effect.driver() -> effectN( 2 ).percent();

    reflect = new stalwart_guardian_reflect_t( p() );
  }

  druid_t* p() const
  { return static_cast<druid_t*>( player ); }

  void init() override
  {
    absorb_t::init();

    snapshot_flags &= ~STATE_VERSATILITY; // Is not affected by versatility.
  }

  void execute() override
  {
    absorb_t::execute();

    reflect -> base_dd_min = absorb_size;
    reflect -> target = triggering_enemy;
    reflect -> execute();
  }

  void impact( action_state_t* s ) override
  {
    s -> result_amount = std::min( s -> result_total, absorb_limit * incoming_damage );
    absorb_size = s -> result_amount;
  }
};

namespace pets {

// ==========================================================================
// Pets and Guardians
// ==========================================================================

// Force of Nature ==================================================

struct force_of_nature_t : public pet_t
{
  struct fon_melee_t : public melee_attack_t
  {
    bool first_attack;
    fon_melee_t( pet_t* p, const char* name = "melee" ) :
      melee_attack_t( name, p, spell_data_t::nil() ),
      first_attack( true )
    {
      school = SCHOOL_PHYSICAL;
      weapon = &( p -> main_hand_weapon );
      weapon_multiplier = 1.0;
      base_execute_time = weapon -> swing_time;
      may_crit = background = repeating = true;
    }

    timespan_t execute_time() const override
    {
      if ( first_attack )
      {
        return timespan_t::zero();
      }
      else
      {
        return melee_attack_t::execute_time();
      }
    }

    void cancel() override
    {
      melee_attack_t::cancel();
      first_attack = true;
    }

    void schedule_execute( action_state_t* s ) override
    {
      melee_attack_t::schedule_execute( s );
      first_attack = false;
    }
  };

  struct auto_attack_t : public melee_attack_t
  {
    auto_attack_t( pet_t* player ) : melee_attack_t( "auto_attack", player )
    {
      assert( player -> main_hand_weapon.type != WEAPON_NONE );
      player -> main_hand_attack = new fon_melee_t( player );
      trigger_gcd = timespan_t::zero();
    }

    void execute() override
    { player -> main_hand_attack -> schedule_execute(); }

    bool ready() override
    {
      return ( player -> main_hand_attack -> execute_event == nullptr );
    }
  };

  druid_t* o() { return static_cast< druid_t* >( owner ); }

  force_of_nature_t( sim_t* sim, druid_t* owner ) :
    pet_t( sim, owner, "treant", true /*GUARDIAN*/, true )
  {
    owner_coeff.ap_from_sp = 0.6; //not confirmed
    regen_type = REGEN_DISABLED;
    main_hand_weapon.type = WEAPON_BEAST;
  }

  void init_action_list() override
  {
    action_priority_list_t* def = get_action_priority_list( "default" );
    def -> add_action( "auto_attack" );

    pet_t::init_action_list();
  }

  double composite_player_multiplier( school_e school ) const override
  {
    return owner -> cache.player_multiplier( school );
  }

  double composite_melee_crit_chance() const override
  {
    return owner -> cache.spell_crit_chance();
  }

  double composite_spell_haste() const override
  {
    return owner -> cache.spell_haste();
  }

  double composite_damage_versatility() const override
  {
    return owner -> cache.damage_versatility();
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
  }

  resource_e primary_resource() const override { return RESOURCE_NONE; }

  action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "auto_attack" ) return new auto_attack_t( this );

    return pet_t::create_action( name, options_str );
  }
};
} // end namespace pets

namespace buffs {

template <typename BuffBase>
struct druid_buff_t : public BuffBase
{
protected:
  typedef druid_buff_t base_t;

  // Used when shapeshifting to switch to a new attack & schedule it to occur
  // when the current swing timer would have ended.
  void swap_melee( attack_t* new_attack, weapon_t& new_weapon )
  {
    if ( p().main_hand_attack && p().main_hand_attack -> execute_event )
    {
      new_attack -> base_execute_time = new_weapon.swing_time;
      new_attack -> execute_event = new_attack -> start_action_execute_event(
                                      p().main_hand_attack -> execute_event -> remains() );
      p().main_hand_attack -> cancel();
    }
    new_attack -> weapon = &new_weapon;
    p().main_hand_attack = new_attack;
    p().main_hand_weapon = new_weapon;
  }

public:
  druid_buff_t( druid_t& p, const std::string& name, const spell_data_t* s = spell_data_t::nil(), const item_t* item = nullptr ) :
    BuffBase( &p, name, s, item )
  { }
  druid_buff_t( druid_td_t& td, const std::string& name, const spell_data_t* s = spell_data_t::nil(), const item_t* item = nullptr ) :
    BuffBase( td, name, s, item )
  { }

  druid_t& p()
  {
    return *debug_cast<druid_t*>( BuffBase::source );
  }
  const druid_t& p() const
  {
    return *debug_cast<druid_t*>( BuffBase::source );
  }

};

// Bear Form ================================================================

struct bear_form_t : public druid_buff_t< buff_t >
{
public:
  bear_form_t( druid_t& p ) :
    base_t( p, "bear_form", p.find_class_spell( "Bear Form" ) ),
    rage_spell( p.find_spell( 17057 ) )
  {
    add_invalidate( CACHE_ATTACK_POWER );
    add_invalidate( CACHE_STAMINA );
    add_invalidate( CACHE_ARMOR );
    add_invalidate( CACHE_EXP );
    add_invalidate( CACHE_CRIT_AVOIDANCE );
  }

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    base_t::expire_override( expiration_stacks, remaining_duration );

    swap_melee( p().caster_melee_attack, p().caster_form_weapon );

    p().recalculate_resource_max( RESOURCE_HEALTH );
  }

  virtual void start( int stacks, double value, timespan_t duration ) override
  {
    p().buff.moonkin_form -> expire();
    p().buff.moonkin_form_affinity -> expire(); //Balance affinity moonkin form.
    p().buff.cat_form -> expire();

    p().buff.tigers_fury -> expire(); // Mar 03 2016: Tiger's Fury ends when you enter bear form.

    swap_melee( p().bear_melee_attack, p().bear_weapon );

    // Set rage to 0 and then gain rage to 10
    p().resource_loss( RESOURCE_RAGE, p().resources.current[ RESOURCE_RAGE ] );
    p().resource_gain( RESOURCE_RAGE, rage_spell -> effectN( 1 ).base_value() / 10.0, p().gain.bear_form );
    // TODO: Clear rage on bear form exit instead of entry.

    base_t::start( stacks, value, duration );

    p().recalculate_resource_max( RESOURCE_HEALTH );
  }
private:
  const spell_data_t* rage_spell;
};

// Berserk Buff Template ====================================================

struct berserk_buff_base_t : public druid_buff_t<buff_t>
{
  double increased_max_energy;

  berserk_buff_base_t( druid_t& p, const std::string& name, const spell_data_t* spell ) :
    base_t( p, name, spell ),
    increased_max_energy(0)
  // Cooldown handled by ability
  {
    set_cooldown( timespan_t::zero() );
  }

  virtual bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    bool refresh = check() != 0;
    bool success = druid_buff_t<buff_t>::trigger( stacks, value, chance, duration );

    if ( ! refresh && success )
      player -> resources.max[ RESOURCE_ENERGY ] += increased_max_energy;

    return success;
  }

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    player -> resources.max[ RESOURCE_ENERGY ] -= increased_max_energy;
    // Force energy down to cap if it's higher.
    player -> resources.current[ RESOURCE_ENERGY ] = std::min( player -> resources.current[ RESOURCE_ENERGY ],
        player -> resources.max[ RESOURCE_ENERGY ] );

    druid_buff_t<buff_t>::expire_override( expiration_stacks, remaining_duration );
  }
};

// Berserk Buff =============================================================

struct berserk_buff_t : public berserk_buff_base_t
{
  berserk_buff_t( druid_t& p ) :
    berserk_buff_base_t( p, "berserk", p.find_spell( 106951 ) )
  {
    default_value        = data().effectN( 1 ).percent(); // cost modifier
    increased_max_energy = data().effectN( 3 ).resource( RESOURCE_ENERGY );
  }
};

// Incarnation: King of the Jungle ==========================================

struct incarnation_cat_buff_t : public berserk_buff_base_t
{
  incarnation_cat_buff_t( druid_t& p ) :
    berserk_buff_base_t( p, "incarnation_king_of_the_jungle", p.talent.incarnation_cat )
  {
    default_value        = data().effectN( 1 ).percent(); // cost modifier
    increased_max_energy = data().effectN( 2 ).resource(RESOURCE_ENERGY);
  }
};

// Cat Form =================================================================

struct cat_form_t : public druid_buff_t< buff_t >
{
  cat_form_t( druid_t& p ) :
    base_t( p, "cat_form", p.find_class_spell( "Cat Form" ) )
  {
    add_invalidate( CACHE_ATTACK_POWER );
  }

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    base_t::expire_override( expiration_stacks, remaining_duration );

    swap_melee( p().caster_melee_attack, p().caster_form_weapon );
  }

  virtual void start( int stacks, double value, timespan_t duration ) override
  {
    p().buff.bear_form -> expire();
    p().buff.moonkin_form -> expire();
    p().buff.moonkin_form_affinity -> expire(); //Balance affinity moonkin form.

    swap_melee( p().cat_melee_attack, p().cat_weapon );

    base_t::start( stacks, value, duration );
  }
};

//Innervate Buff ============================================================
struct innervate_buff_t : public druid_buff_t<buff_t>
{
  innervate_buff_t(druid_t& p) :
    base_t(p, "innervate", p.spec.innervate)
  {}

  void expire_override(int expiration_stacks, timespan_t remaining_duration) override
  {
    buff_t::expire_override(expiration_stacks, remaining_duration);

    druid_t* p = debug_cast<druid_t*>(player);

    if (p->azerite.lively_spirit.enabled())
    {
      p->buff.lively_spirit->trigger(p->lively_spirit_stacks);
    }
  }
};

struct tigers_fury_buff_t : public druid_buff_t<buff_t>
{
  tigers_fury_buff_t(druid_t& p) : base_t(p, "tigers_fury", p.spec.tigers_fury)
  {
    set_default_value(p.spec.tigers_fury->effectN(1).percent());
    set_cooldown(timespan_t::zero());
  }

  virtual void start(int stacks, double value, timespan_t duration) override
  {
    if (p().azerite.jungle_fury.enabled())
      p().buff.jungle_fury->trigger(1, DEFAULT_VALUE(), 1.0, duration);

    base_t::start(stacks, value, duration);
  }

  virtual void expire_override(int expiration_stacks, timespan_t remaining_duration) override
  {
    p().buff.jungle_fury->expire();

    base_t::expire_override(expiration_stacks, remaining_duration);
  }

};

//Tiger Dash Buff ===========================================================
struct tiger_dash_buff_t : public druid_buff_t<buff_t>
{
  tiger_dash_buff_t(druid_t& p) : base_t(p, "tiger_dash", p.talent.tiger_dash)
  {
    set_default_value(p.talent.tiger_dash->effectN(1).percent());
    set_cooldown(timespan_t::zero());
    set_tick_callback([this] (buff_t* b, int, const timespan_t&)
    {
      b->current_value -= b->data().effectN(2).percent();
    } );
  }
  
  bool freeze_stacks() override
  {
    return true;
  }
};

// Moonkin Form =============================================================

struct moonkin_form_t : public druid_buff_t< buff_t >
{
  moonkin_form_t( druid_t& p ) :
    base_t( p, "moonkin_form", p.find_affinity_spell( "Moonkin Form" ) )
  {
    add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    add_invalidate( CACHE_ARMOR );
    set_chance( 1.0 );
  }

  virtual void start( int stacks, double value, timespan_t duration ) override
  {
    p().buff.bear_form -> expire();
    p().buff.cat_form  -> expire();

    base_t::start( stacks, value, duration );
  }
};

// Moonkin Form Affinity

struct moonkin_form_affinity_t : public druid_buff_t< buff_t >
{
  moonkin_form_affinity_t(druid_t& p) :
    base_t(p, "moonkin_form_affinity", p.spec.moonkin_form_affinity)
  {
    add_invalidate(CACHE_PLAYER_DAMAGE_MULTIPLIER);
    add_invalidate(CACHE_ARMOR);
    set_chance(1.0);
  }

  virtual void start(int stacks, double value, timespan_t duration) override
  {
    p().buff.bear_form->expire();
    p().buff.cat_form->expire();

    base_t::start(stacks, value, duration);
  }
};

// Warrior of Elune Buff ====================================================

struct warrior_of_elune_buff_t : public druid_buff_t<buff_t>
{
  warrior_of_elune_buff_t( druid_t& p ) :
    base_t( p, "warrior_of_elune", p.talent.warrior_of_elune )
  {
    set_default_value(p.talent.warrior_of_elune->effectN(1).percent());
  }

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    druid_buff_t<buff_t>::expire_override(expiration_stacks, remaining_duration);
    p().cooldown.warrior_of_elune -> start();
  }
};

// Survival Instincts =======================================================

struct survival_instincts_buff_t : public druid_buff_t<buff_t>
{
survival_instincts_buff_t( druid_t& p ) :
  base_t( p, "survival_instincts", p.find_specialization_spell( "Survival Instincts" ) )
  {
    set_cooldown( timespan_t::zero() );
    set_default_value( data().effectN( 1 ).percent() );
  }

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    druid_buff_t<buff_t>::expire_override( expiration_stacks, remaining_duration );
    p().buff.masterful_instincts -> trigger();
  }
};

} // end namespace buffs

// Template for common druid action code. See priest_action_t.
template <class Base>
struct druid_action_t : public Base
{
  unsigned form_mask; // Restricts use of a spell based on form.
  bool may_autounshift; // Allows a spell that may be cast in NO_FORM but not in current form to be cast by exiting form.
  unsigned autoshift; // Allows a spell that may not be cast in the current form to be cast by automatically changing to the specified form.
private:
  typedef Base ab; // action base, eg. spell_t
public:
  typedef druid_action_t base_t;

  bool rend_and_tear;
  bool hasted_gcd;
  double gore_chance;
  bool triggers_galactic_guardian;

  druid_action_t( const std::string& n, druid_t* player,
                  const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s ),
    form_mask( ab::data().stance_mask() ), may_autounshift( true ), autoshift( 0 ),
    rend_and_tear( ab::data().affected_by( player -> spec.thrash_bear_dot -> effectN( 2 ) ) ),
    hasted_gcd( ab::data().affected_by( player -> spec.druid -> effectN( 4 ) ) ),
    gore_chance( player -> spec.gore -> effectN( 1 ).percent() ), triggers_galactic_guardian( true )
  {
    ab::may_crit      = true;
    ab::tick_may_crit = true;
    ab::cooldown -> hasted = ab::data().affected_by( player -> spec.druid -> effectN( 3 ) );

    gore_chance += p() -> sets -> set( DRUID_GUARDIAN, T19, B2 ) -> effectN( 1 ).percent();

    if (player->specialization() == DRUID_BALANCE)
    {
      if (s->affected_by(player->spec.balance->effectN(2))) // Periodic Damage
      {
        ab::base_td_multiplier *= 1.0 + player->spec.balance->effectN(2).percent();
      }
      if (s->affected_by(player->spec.balance->effectN(1))) // Direct Damage
      {
        ab::base_dd_multiplier *= 1.0 + player->spec.balance->effectN(1).percent();
      }
    }
    else if (player->specialization() == DRUID_RESTORATION)
    {
      if (s->affected_by(player->spec.restoration->effectN(9))) // Periodic Damage
      {
        ab::base_td_multiplier *= 1.0 + player->spec.restoration->effectN(8).percent();
      }
      if (s->affected_by(player->spec.restoration->effectN(8))) // Direct Damage
      {
        ab::base_dd_multiplier *= 1.0 + player->spec.restoration->effectN(8).percent();
      }
    }
  }

  druid_t* p()
  { return static_cast<druid_t*>( ab::player ); }
  const druid_t* p() const
  { return static_cast<druid_t*>( ab::player ); }

  druid_td_t* td( player_t* t ) const
  { return p() -> get_target_data( t ); }

  virtual double target_armor( player_t* t ) const override
  {
    double a = ab::target_armor( t );

    return a;
  }

  double cost() const override
  {
    double c = ab::cost();

    if ( p() -> buff.innervate->up() && p() -> specialization()==DRUID_RESTORATION)
      c *= 0;

    return c;
  }

  virtual double composite_target_multiplier( player_t* t ) const override
  {
    double tm = ab::composite_target_multiplier( t );

    if ( rend_and_tear )
      tm *= 1.0 + p() -> talent.rend_and_tear -> effectN( 2 ).percent() * td( t ) -> dots.thrash_bear -> current_stack();

    return tm;
  }

  void impact( action_state_t* s ) override
  {
    ab::impact( s );

    if ( p() -> buff.feral_tier17_4pc -> check() )
      trigger_gushing_wound( s -> target, s -> result_amount );

    trigger_galactic_guardian( s );
  }

  void tick( dot_t* d ) override
  {
    ab::tick( d );

    if ( p() -> buff.feral_tier17_4pc -> check() )
      trigger_gushing_wound( d -> target, d -> state -> result_amount );

    trigger_galactic_guardian( d -> state );
  }

  virtual void schedule_execute( action_state_t* s = nullptr ) override
  {
    if ( ! check_form_restriction() )
    {
      if ( may_autounshift && ( form_mask & NO_FORM ) == NO_FORM )
        p() -> shapeshift( NO_FORM );
      else if ( autoshift )
        p() -> shapeshift( ( form_e ) autoshift );
      else
      {
        assert( false && "Action executed in wrong form with no valid form to shift to!" );
      }
    }

    ab::schedule_execute( s );
  }

  /* Override this function for temporary effects that change the normal
     form restrictions of the spell. eg: Predatory Swiftness */
  virtual bool check_form_restriction()
  {
    return ! form_mask || ( form_mask & p() -> get_form() ) == p() -> get_form();
  }

  virtual bool ready() override
  {
    if ( ! check_form_restriction() && ! ( ( may_autounshift && ( form_mask & NO_FORM ) == NO_FORM ) || autoshift ) )
    {
      if ( ab::sim -> debug )
        ab::sim -> out_debug.printf( "%s ready() failed due to wrong form. form=%#.8x form_mask=%#.8x", ab::name(), p() -> get_form(), form_mask );

      return false;
    }

    return ab::ready();
  }

  void trigger_gushing_wound( player_t* t, double dmg )
  {
    if ( ! ( ab::special && ab::harmful && dmg > 0 ) )
      return;

    residual_action::trigger(
      p() -> active.gushing_wound, // ignite spell
      t, // target
      p() -> spec.gushing_wound -> effectN( 1 ).percent() * dmg );
  }

  bool trigger_gore()
  {
    if ( ab::rng().roll( gore_chance ) )
    {
      p() -> proc.gore -> occur();
      p() -> cooldown.mangle -> reset( true );
      p() -> buff.gore -> trigger();

      return true;
    }
    return false;
  }

  virtual void trigger_galactic_guardian( action_state_t* s )
  {
    if ( ! ( triggers_galactic_guardian && ! ab::proc && ab::harmful ) )
      return;
    if ( ! p() -> talent.galactic_guardian -> ok() )
      return;
    if ( s -> target == p() )
      return;
    if ( ! ab::result_is_hit( s -> result ) )
      return;
    if ( s -> result_total <= 0 )
      return;

    if ( ab::rng().roll( p() -> talent.galactic_guardian -> proc_chance() ) )
    {
      // Trigger Moonfire
      action_state_t* gg_s = p() -> active.galactic_guardian -> get_state();
      gg_s -> target = s -> target;
      p() -> active.galactic_guardian -> snapshot_state( gg_s, DMG_DIRECT );
      p() -> active.galactic_guardian -> schedule_execute( gg_s );

      // Buff is triggered in galactic_guardian_damage_t::execute()
    }
  }

  virtual bool compare_previous_streaking_stars(streaking_stars_e new_ability, bool cast = false)
  {
    return (cast ? p()->previous_streaking_stars_cast : p()->previous_streaking_stars) == new_ability || new_ability == SS_CELESTIAL_ALIGNMENT; // INC & CA themselves do not proc streaking
  }

  // cast = true: only checked against previous_streaking_stars_cast, doesn't schedule damage
  // damage = true: checked against previous_streaking_star, also schedules damage
  virtual bool streaking_stars_trigger(streaking_stars_e new_ability, action_state_t* s, bool cast = true, bool damage = true)
  {
    bool ret = false;

    if (p()->azerite.streaking_stars.ok())
    {
      if (p()->buff.celestial_alignment->check() || p()->buff.incarnation_moonkin->check())
      {
        if (cast && !compare_previous_streaking_stars(new_ability, true))
        {
          ret = true;
        }

        if (damage && !compare_previous_streaking_stars(new_ability))
        {
          action_state_t* ss_s = p()->active.streaking_stars->get_state();
          //Check if the trigger has a target, otherwise use the actors target
          if (s != nullptr)
          {
            ss_s->target = s->target;
          }
          else
          {
            ss_s->target = ab::target;
          }
          p()->active.streaking_stars->snapshot_state(ss_s, DMG_DIRECT);
          p()->active.streaking_stars->schedule_execute(ss_s);
          ret = true;
        }
      }

      if (cast) // only for dummy procs
        p()->previous_streaking_stars_cast = new_ability;
      if (damage) // actual proc
        p()->previous_streaking_stars = new_ability;
    }
    return ret;
  }

  bool verify_actor_spec() const override
  {
    if ( p() -> find_affinity_spell( ab::name() ) -> found() )
    {
      return true;
    }
#ifndef NDEBUG
    else if ( p() -> sim -> debug || p() -> sim -> log )
    {
      p() -> sim -> out_log.printf( "%s failed verification",
                             ab::name() );
    }
#endif

    return ab::verify_actor_spec();
  }

  expr_t* create_expression(const std::string& name_str) override
  {
    std::vector<std::string> splits = util::string_split(name_str, ".");

    if (splits[0] == "dot" && splits[2] == "ticks_gained_on_refresh" | splits[2] == "ticks_gained_on_refresh_pmultiplier")
    {
      // Since we know some action names don't map to the actual dot portion, lets add some exceptions
      // this may have to be made more robust if other specs are interested in using it, but for now lets
      // default any ambiguity to what would make most sense for ferals.
      if (splits[1] == "rake")
        splits[1] = "rake_bleed";
      if (p()->specialization() == DRUID_FERAL && splits[1] == "moonfire")
        splits[1] = "lunar_inspiration";

      bool pmult_adjusted = false;
      if (splits[2] == "ticks_gained_on_refresh_pmultiplier") pmult_adjusted = true;

      if ( splits[ 1 ] == "primal_wrath" )  // special case since pw applies in aoe
      {
        action_t* rip = p()->find_action( "rip" );
        action_t* pw  = p()->find_action( "primal_wrath" );

        return make_fn_expr( name_str, [pw, rip, pmult_adjusted]() -> double {
          int cp = as<int>( pw->player->resources.current[ RESOURCE_COMBO_POINT ] );

          double gained_ticks  = 0;
          timespan_t duration  = rip->dot_duration * 0.5 * ( 1 + cp );
          timespan_t tick_time = rip->base_tick_time * rip->composite_haste();

          for ( player_t* pw_target : pw->targets_in_range_list( pw->target_list() ) )
          {
            timespan_t ttd         = pw_target->time_to_percent( 0 );
            dot_t* dot             = pw_target->get_dot( "rip", pw->player );
            double remaining_ticks = 0;
            double potential_ticks = 0;
            timespan_t durrem      = timespan_t::zero();
            double pmult = 0;

            if ( dot->is_ticking() )
            {
              remaining_ticks = std::min( dot->remains(), ttd ) / tick_time;
              durrem          = rip->calculate_dot_refresh_duration( dot, duration );
              remaining_ticks *= pmult_adjusted ? dot->state->persistent_multiplier : 1.0;
            }

            if (pmult_adjusted)
            {
              action_state_t* state = rip->get_state();

              pw->snapshot_state(state, RESULT_TYPE_NONE);
              state->target = pw_target;

              pmult = pw->composite_persistent_multiplier(state);

              action_state_t::release(state);
            }

            potential_ticks = std::min( std::max( durrem, duration ), ttd ) / tick_time;
            potential_ticks *= pmult_adjusted ? pmult : 1.0;
            gained_ticks += potential_ticks - remaining_ticks;
          }
          return gained_ticks;
        } );
      }
      else if ( splits[ 1 ] == "thrash_cat" )  // special case since pw applies in aoe
      {
        // action_t* rip = p()->find_action("rip");
        action_t* tc = p()->find_action( "thrash_cat" );

        return make_fn_expr( name_str, [tc, pmult_adjusted]() -> double {
          double gained_ticks  = 0;
          timespan_t duration  = tc->dot_duration;
          timespan_t tick_time = tc->base_tick_time * tc->composite_haste();
          

          for ( player_t* tc_target : tc->targets_in_range_list( tc->target_list() ) )
          {
            timespan_t ttd         = tc_target->time_to_percent( 0 );
            dot_t* dot             = tc_target->get_dot( "Thrash", tc->player );
            double remaining_ticks = 0;
            double potential_ticks = 0;
            timespan_t durrem      = timespan_t::zero();
            double pmult = 0;

            if ( dot->is_ticking() )
            {
              remaining_ticks = std::min( dot->remains(), ttd ) / tick_time;
              durrem          = tc->calculate_dot_refresh_duration( dot, duration );
              remaining_ticks *= pmult_adjusted ? dot->state->persistent_multiplier : 1.0;
            }

            if (pmult_adjusted)
            {
              action_state_t* state = tc->get_state();

              tc->snapshot_state(state, RESULT_TYPE_NONE);
              state->target = tc_target;

              pmult = tc->composite_persistent_multiplier(state);

              action_state_t::release(state);
            }

            potential_ticks = std::min( std::max( durrem, duration ), ttd ) / tick_time;
            potential_ticks *= pmult_adjusted ? pmult : 1.0;
            gained_ticks += potential_ticks - remaining_ticks;
          }
          return gained_ticks;
        } );
      }

      action_t* action = p()->find_action(splits[1]);
      if (action)
        return make_fn_expr(name_str, [action, pmult_adjusted]() -> double {
        dot_t* dot = action->get_dot();
        double remaining_ticks = 0;
        double potential_ticks = 0;
        action_state_t* state = action->get_state(dot->state);
        timespan_t duration = action->composite_dot_duration(state);
        timespan_t ttd = action->target->time_to_percent(0);
        double pmult = 0;

        if (dot->is_ticking())
        {
          remaining_ticks = std::min(dot->remains(), ttd) / dot->current_action->tick_time(dot->state);
          duration = action->calculate_dot_refresh_duration(dot, duration);
          remaining_ticks *= pmult_adjusted ? dot->state->persistent_multiplier : 1.0;
        }

        if (pmult_adjusted)
        {
          action->snapshot_state(state, RESULT_TYPE_NONE);
          state->target = action->target;

          pmult = action->composite_persistent_multiplier(state);
        }

        
        potential_ticks = std::min(duration, ttd) / action->tick_time(state);
        potential_ticks *= pmult_adjusted ? pmult : 1.0;
        action_state_t::release(state);
        return potential_ticks - remaining_ticks;
      });
      throw std::invalid_argument("invalid action");
    }
    else if (splits[0] == "ticks_gained_on_refresh")
    {
      return make_fn_expr(name_str, [this]() -> double {
        dot_t* dot = this->get_dot();
        double remaining_ticks = 0;
        double potential_ticks = 0;
        timespan_t duration = this->dot_duration;
        timespan_t ttd = this->target->time_to_percent(0);

        if (dot->is_ticking())
        {
          remaining_ticks = std::min(dot->remains(), ttd) / dot->current_action->tick_time(dot->state);
          duration = this->calculate_dot_refresh_duration(dot, duration);
        }

        potential_ticks = std::min(duration, ttd) / (this->base_tick_time * this->composite_haste());
        return potential_ticks - remaining_ticks;
      });
    }

    return ab::create_expression(name_str);
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

  bool consumes_bloodtalons;
  snapshot_counter_t* bt_counter;
  snapshot_counter_t* tf_counter;
  snapshot_counter_t* sr_counter;
  bool direct_bleed;

  druid_attack_t( const std::string& n, druid_t* player,
                  const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s ), consumes_bloodtalons( false ),
    bt_counter( nullptr ), tf_counter( nullptr ), direct_bleed( false )
  {
    ab::may_glance    = false;
    ab::special       = true;

    parse_special_effect_data();
  }

  void parse_special_effect_data()
  {
    for ( size_t i = 1; i <= this -> data().effect_count(); i++ )
    {
      const spelleffect_data_t& ed = this -> data().effectN( i );
      effect_type_t type = ed.type();

      // Check for bleed flag at effect or spell level.
      if ( ( type == E_SCHOOL_DAMAGE || type == E_WEAPON_PERCENT_DAMAGE )
           && ( ed.mechanic() == MECHANIC_BLEED || this -> data().mechanic() == MECHANIC_BLEED ) )
      {
        direct_bleed = true;
      }
    }
  }

  void init() override
  {
    ab::init();

    consumes_bloodtalons = ab::harmful && ab::special && ab::trigger_gcd > timespan_t::zero();

    if ( consumes_bloodtalons )
    {
      bt_counter = new snapshot_counter_t( ab::p(), ab::p() -> buff.bloodtalons );
      tf_counter = new snapshot_counter_t( ab::p(), ab::p() -> buff.tigers_fury );
      sr_counter = new snapshot_counter_t( ab::p(), ab::p() -> buff.savage_roar );
    }
  }

  virtual timespan_t gcd() const override
  {
    timespan_t g = ab::gcd();

    if ( g == timespan_t::zero() )
      return g;

    if ( ab::hasted_gcd )
      g *= ab::p() -> cache.attack_haste();

    if ( g < ab::min_gcd )
      return ab::min_gcd;
    else
      return g;
  }

  void execute() override
  {
    ab::execute();

    if ( consumes_bloodtalons && ab::hit_any_target )
    {
      bt_counter -> count_execute();
      tf_counter -> count_execute();
      sr_counter -> count_execute();

      ab::p() -> buff.bloodtalons -> decrement();
    }
  }

  void impact( action_state_t* s ) override
  {
    ab::impact( s );

    if ( ab::result_is_hit( s -> result ) && ! ab::special )
      trigger_clearcasting();
  }

  void tick( dot_t* d ) override
  {
    ab::tick( d );

    if ( consumes_bloodtalons )
    {
      bt_counter -> count_tick();
      tf_counter -> count_tick();
      sr_counter -> count_tick();
    }
  }

  virtual double target_armor( player_t* t ) const override
  {
    if ( direct_bleed )
      return 0.0;
    else
      return ab::target_armor( t );
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double tm = ab::composite_target_multiplier( t );

    /* Assume that any action that deals physical and applies a dot deals all bleed damage, so
       that it scales direct "bleed" damage. This is a bad assumption if there is an action
       that applies a dot but does plain physical direct damage, but there are none of those. */
    if ( dbc::is_school( ab::school, SCHOOL_PHYSICAL ) && ab::dot_duration > timespan_t::zero() )
      tm *= 1.0 + ab::td( t ) -> debuff.bloodletting -> value();

    return tm;
  }

  void trigger_clearcasting()
  {
    if ( ab::proc )
      return;
    if ( ! ( ab::p() -> specialization() == DRUID_FERAL && ab::p() -> spec.omen_of_clarity -> ok() ) )
      return;

    // 7.00 PPM via community testing (~368k auto attacks)
    // https://docs.google.com/spreadsheets/d/1vMvlq1k3aAuwC1iHyDjqAneojPZusdwkZGmatuWWZWs/edit#gid=1097586165
    double chance = ab::weapon -> proc_chance_on_swing( 7.00 );

    if ( ab::p() -> sets -> has_set_bonus( DRUID_FERAL, T18, B2 ) )
      chance *= 1.0 + ab::p() -> sets -> set( DRUID_FERAL, T18, B2 ) -> effectN( 1 ).percent();

    if ( ab::p() -> talent.moment_of_clarity -> ok() )
      chance *= 1.0 + ab::p() -> talent.moment_of_clarity -> effectN( 2 ).percent();

    int active = ab::p() -> buff.clearcasting -> check();

    // Internal cooldown is handled by buff.

    if ( ab::p() -> buff.clearcasting -> trigger(
           1,
           buff_t::DEFAULT_VALUE(),
           chance,
           ab::p() -> buff.clearcasting -> buff_duration ) ) {
      ab::p() -> proc.clearcasting -> occur();

      for ( int i = 0; i < active; i++ )
        ab::p() -> proc.clearcasting_wasted -> occur();
    }
  }

};

// Druid "Spell" Base for druid_spell_t, druid_heal_t ( and potentially druid_absorb_t )
template <class Base>
struct druid_spell_base_t : public druid_action_t< Base >
{
private:
  typedef druid_action_t< Base > ab;
public:
  typedef druid_spell_base_t base_t;

  bool cat_form_gcd;
  bool reset_melee_swing; // TRUE(default) to reset swing timer on execute (as most cast time spells do)

  druid_spell_base_t( const std::string& n, druid_t* player,
                      const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s ),
    cat_form_gcd( ab::data().affected_by( player -> spec.cat_form -> effectN( 4 ) ) ),
    reset_melee_swing( true )
  { }

  virtual timespan_t gcd() const override
  {
    timespan_t g = ab::trigger_gcd;

    if ( g == timespan_t::zero() )
      return g;

    if ( cat_form_gcd && ab::p() -> buff.cat_form -> check() )
      g += ab::p() -> spec.cat_form -> effectN( 4 ).time_value();

    g *= ab::composite_haste();

    if ( g < ab::min_gcd )
      return ab::min_gcd;
    else
      return g;
  }

  virtual void execute() override
  {
    if (ab::time_to_execute > timespan_t::zero() && !ab::proc && !ab::background && reset_melee_swing)
    {
      if (ab::p()->main_hand_attack && ab::p()->main_hand_attack->execute_event)
      {
        ab::p()->main_hand_attack->execute_event->reschedule(ab::p()->main_hand_weapon.swing_time);
      }
      // Nothing for OH, as druids don't DW
    }

    ab::execute();
  }
};

//proc_sephuz gives control over how and when sephuz proc to the
//APL creator to allow for more flexible modelling
struct proc_sephuz_t : public action_t
{
   proc_sephuz_t(druid_t* p, const std::string& options_str)
      : action_t(ACTION_USE, "proc_sephuz", p)
   {
      parse_options(options_str);
      trigger_gcd = timespan_t::zero();
      harmful = false;
      cooldown -> duration = player -> find_spell(226262) -> duration();
      ignore_false_positive = true;
      action_skill = 1;
   }

   virtual void execute() override
   {
      druid_t* p = (druid_t*)(player);

      if ( p -> buff.sephuzs_secret -> default_chance == 0 )
         return;

      p -> buff.sephuzs_secret -> trigger();

   }

   bool ready() override
   {
      druid_t* p = (druid_t*)(player);
      if ( p -> legendary.sephuzs_secret == spell_data_t::not_found() )
         return false;


      if ( p -> buff.sephuzs_secret -> cooldown -> up() )
         return action_t::ready();

      return false;
   }
};

namespace spells {

/* druid_spell_t ============================================================
  Early definition of druid_spell_t. Only spells that MUST for use by other
  actions should go here, otherwise they can go in the second spells
  namespace.
========================================================================== */

struct druid_spell_t : public druid_spell_base_t<spell_t>
{
private:
  typedef druid_spell_base_t<spell_t> ab;
public:
  bool warrior_of_elune;
  bool owlkin_frenzy;

  druid_spell_t( const std::string& token, druid_t* p,
                 const spell_data_t* s      = spell_data_t::nil(),
                 const std::string& options = std::string() )
    : base_t( token, p, s ),
      warrior_of_elune( data().affected_by( p -> talent.warrior_of_elune -> effectN( 1 ) ) ),
      owlkin_frenzy( data().affected_by( p -> spec.owlkin_frenzy -> effectN( 1 ) ) )
  {
    parse_options( options );

    // Apply Guardian Druid aura damage modifiers
    if (p -> specialization() == DRUID_GUARDIAN)
    {
      // direct damage
      if ( s -> affected_by( p -> spec.guardian -> effectN( 1 ) ) )
      {
        base_dd_multiplier *= 1.0 + p -> spec.guardian -> effectN( 1 ).percent();
      }

      // ticking damage
      if ( s -> affected_by( p -> spec.guardian -> effectN( 2 ) ) )
      {
        base_td_multiplier *= 1.0 + p -> spec.guardian -> effectN( 2 ).percent();
      }
    }
  }

  virtual double composite_energize_amount( const action_state_t* s ) const override
  {
    double e = ab::composite_energize_amount( s );

    if ( energize_resource_() == RESOURCE_ASTRAL_POWER )
    {
      if (warrior_of_elune && p()->buff.warrior_of_elune->check())
        e *= 1.0 + p()->talent.warrior_of_elune->effectN(2).percent();
    }

    return e;
  }

  virtual void consume_resource() override
  {
    ab::consume_resource();

    trigger_impeccable_fel_essence();
  }

  virtual bool consume_cost_per_tick( const dot_t& dot ) override
  {
    bool ab = ab::consume_cost_per_tick( dot );
    if (ab::consume_per_tick_) {
        trigger_impeccable_fel_essence();
    }
    return ab;
  }

  virtual double composite_ta_multiplier(const action_state_t* s) const override
  {
    double tm = base_t::composite_ta_multiplier(s);

    if (data().affected_by(p()->talent.incarnation_moonkin->effectN(2)) && p()->buff.incarnation_moonkin->up())
    {
      tm *= 1.0 + p()->talent.incarnation_moonkin->effectN(2).percent();
    }
    if (data().affected_by(p()->spec.celestial_alignment->effectN(2)) && p()->buff.celestial_alignment->up())
    {
      tm *= 1.0 + p()->spec.celestial_alignment->effectN(2).percent();
    }
    if (data().affected_by(p()->spec.moonkin_form->effectN(10)) && p()->buff.moonkin_form->up())
    {
      tm *= 1.0 + p()->spec.moonkin_form->effectN(10).percent();
    }

    return tm;
  }

  virtual double composite_da_multiplier(const action_state_t* s) const override
  {
    double dm = base_t::composite_da_multiplier(s);

    if (data().affected_by(p()->talent.incarnation_moonkin->effectN(1)) && p()->buff.incarnation_moonkin->up())
    {
      dm *= 1.0 + p()->talent.incarnation_moonkin->effectN(1).percent();
    }
    if (data().affected_by(p()->spec.celestial_alignment->effectN(1)) && p()->buff.celestial_alignment->up())
    {
      dm *= 1.0 + p()->spec.celestial_alignment->effectN(1).percent();
    }
    if (data().affected_by(p()->spec.moonkin_form->effectN(9)) && p()->buff.moonkin_form->up())
    {
      dm *= 1.0 + p()->spec.moonkin_form->effectN(9).percent();
    }

    return dm;
  }
  
  virtual bool usable_moving() const override
  {
    if (p()->buff.stellar_drift_2->check() && data().affected_by(p()->spec.stellar_drift_2->effectN(1)))
      return true;
    
    return spell_t::usable_moving();
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t et = ab::execute_time();

    if (warrior_of_elune)
      et *= 1 + p()->buff.warrior_of_elune->check_value();
    if (owlkin_frenzy)
      et *= 1 + p()->buff.owlkin_frenzy->check_value();

    return et;
  }

  virtual void execute() override
  {
    // Adjust buffs and cooldowns if we're in precombat.
    if ( ! p() -> in_combat )
    {
      if ( p() -> buff.incarnation_moonkin -> check() )
      {
        timespan_t time = std::max( std::max( min_gcd, trigger_gcd * composite_haste() ), base_execute_time * composite_haste() );
        p() -> buff.incarnation_moonkin -> extend_duration( p(), -time );
        p() -> cooldown.incarnation -> adjust( -time );
      }
    }

    ab::execute();

    if (time_to_execute == timespan_t::zero()) // Check on instant cast to see if instant-cast-buff decrement is needed
    {
      if (warrior_of_elune && p()->buff.warrior_of_elune->up())
        p()->buff.warrior_of_elune->decrement();
      else if (owlkin_frenzy && p()->buff.owlkin_frenzy->up())
        p()->buff.owlkin_frenzy->decrement();
    }
  }

  virtual void trigger_impeccable_fel_essence()
  {
    if ( p() -> legendary.impeccable_fel_essence == timespan_t::zero() )
      return;
    if ( resource_current != RESOURCE_ASTRAL_POWER )
      return;
    if ( last_resource_cost <= 0.0 )
      return;

    timespan_t reduction = last_resource_cost * p() -> legendary.impeccable_fel_essence;
    p() -> cooldown.celestial_alignment -> adjust( reduction );
    p() -> cooldown.incarnation -> adjust( reduction );
  };

  virtual double composite_lunar_empowerment() const
  {
    double le = p() -> buff.lunar_empowerment -> check_value();

    le += p() -> mastery.starlight -> ok() * p() -> cache.mastery_value();
    if (p ()->talent.soul_of_the_forest->ok ())
      le *= (1.0 + p ()->talent.soul_of_the_forest->effectN (1).percent ());

    return le;
  }

  expr_t* create_expression(const std::string& name_str) override
  {
    std::vector<std::string> splits = util::string_split(name_str, ".");

    // check for AP overcap on current action. check for other action handled in druid_t::create_expression
    // syntax: ap_check.<allowed overcap = 0>
    if (p()->specialization() == DRUID_BALANCE && util::str_compare_ci(splits[0], "ap_check"))
    {
      return make_fn_expr(name_str, [this, splits]() {
        action_state_t* state = this->get_state();
        double ap = p()->resources.current[RESOURCE_ASTRAL_POWER];
        ap += composite_energize_amount(state);
        ap += p()->talent.shooting_stars->ok() ? p()->spec.shooting_stars_dmg->effectN(2).base_value() / 10 : 0;
        ap += p()->talent.natures_balance->ok() ? std::ceil(time_to_execute / timespan_t::from_seconds(1.5)) : 0;
        action_state_t::release(state);
        return ap <= p()->resources.base[RESOURCE_ASTRAL_POWER] + (splits.size() >= 2 ? std::stoi(splits[1]) : 0);
      });
    }

    return ab::create_expression(name_str);
  }
}; // end druid_spell_t

// Shooting Stars ===========================================================

struct shooting_stars_t : public druid_spell_t
{
  shooting_stars_t( druid_t* player ) :
    druid_spell_t( "shooting_stars", player, player -> spec.shooting_stars_dmg )
  {
    background = true;
  }
};

// Moonfire Spell ===========================================================

struct moonfire_t : public druid_spell_t
{
  struct moonfire_damage_t : public druid_spell_t
  {
    private:
      double galactic_guardian_dd_multiplier;

    protected:
      bool benefits_from_galactic_guardian;

    public:

    moonfire_damage_t( druid_t* p ) :
      moonfire_damage_t( p, "moonfire_dmg" )
    {}

    moonfire_damage_t( druid_t* p, const std::string& n ) :
      druid_spell_t( n, p, p -> spec.moonfire_dmg )
    {
      if ( p -> spec.astral_power -> ok() )
      {
        energize_resource = RESOURCE_ASTRAL_POWER;
        energize_amount   = p -> spec.astral_power -> effectN( 3 ).resource( RESOURCE_ASTRAL_POWER );
      }
      else
      {
        energize_type = ENERGIZE_NONE;
      }

      if ( p -> talent.shooting_stars -> ok() && ! p -> active.shooting_stars )
      {
        p -> active.shooting_stars = new shooting_stars_t( p );
      }

      if ( p -> talent.galactic_guardian->ok())
      {
        galactic_guardian_dd_multiplier = p->find_spell(213708)->effectN(3).percent();
      }

      may_miss = false;
      triggers_galactic_guardian = false;
      benefits_from_galactic_guardian = true;
      dual = background = true;
      dot_duration       += p -> spec.balance -> effectN( 4 ).time_value();
      base_dd_multiplier *= 1.0 + p -> spec.guardian -> effectN( 8 ).percent();
      aoe = 1;

      if (p->talent.twin_moons->ok())
      {
        aoe += (int) p->talent.twin_moons->effectN(1).base_value();
        /* Twin Moons seems to fire off another MF spell - sometimes concurrently,
           sometimes up to 100ms later. While there are very limited cases where
           this could have an effect, those cases do exist. Possibly worth investigating
           further in the future. */
        radius = p->talent.twin_moons->effectN(1).trigger()->effectN(1).radius_max();
        base_dd_multiplier *= 1.0 + p->talent.twin_moons->effectN(2).percent();
        base_td_multiplier *= 1.0 + p->talent.twin_moons->effectN(3).percent();
      }

      /* June 2016: This hotfix is negated if you shift into Moonkin Form (ever),
        so only apply it if the druid does not have balance affinity. */
      if ( ! p -> talent.balance_affinity -> ok() )
      {
        base_dd_multiplier *= 1.0 + p -> spec.feral_overrides -> effectN( 1 ).percent();
        base_td_multiplier *= 1.0 + p -> spec.feral_overrides -> effectN( 2 ).percent();
      }
    }

    double action_multiplier() const override
    {
        double am = druid_spell_t::action_multiplier();

        am *= 1.0 + p()->buff.solar_solstice->check_value();

        if (p ()->mastery.starlight->ok ())
          am *= 1.0 + p ()->cache.mastery_value ();

        return am;
    }

    virtual double bonus_da(const action_state_t* s) const override
    {
      double da = druid_spell_t::bonus_da(s);
      return da;
    }

    double bonus_ta(const action_state_t* s) const override
    {
      double ta = druid_spell_t::bonus_ta(s);

      ta += p()->azerite.power_of_the_moon.value(2);

      return ta;
    }

    dot_t* get_dot( player_t* t ) override
    {
      if ( ! t ) t = target;
      if ( ! t ) return nullptr;

      return td( t ) -> dots.moonfire;
    }

    virtual double composite_target_da_multiplier( player_t* t ) const override
    {
      double tdm = druid_spell_t::composite_target_da_multiplier( t );

      // IN-GAME BUG (Jan 20 2018): Lady and the Child Galactic Guardian procs benefit from both the DD modifier and the rage gain.
      if ( ( benefits_from_galactic_guardian || ( p() -> bugs && t != target ) )
        && p() -> buff.galactic_guardian -> check() )
      {
        // Galactic Guardian 7.1 Damage Buff
        tdm *= 1.0 + galactic_guardian_dd_multiplier;
      }

      return tdm;
    }

    //virtual double bonus_dd

    void tick( dot_t* d ) override
    {
      druid_spell_t::tick( d );
      double dr_mult = std::sqrt(p()->get_active_dots(internal_id)) / p()->get_active_dots(internal_id);

      if (p()->talent.shooting_stars->ok())
      {
        if (rng().roll(p()->talent.shooting_stars->effectN(1).percent()*dr_mult))
        {
          p()->active.shooting_stars->target = d->target;
          p()->active.shooting_stars->execute();
        }
      }

      if (!p()->azerite.power_of_the_moon.ok())
        return;
      if (!p()->rppm.power_of_the_moon->trigger())
        return;

      p()->proc.power_of_the_moon->occur();
      p()->buff.lunar_empowerment->trigger();

    }

    void impact ( action_state_t* s ) override
    {
      druid_spell_t::impact( s );

      // The buff needs to be handled with the damage handler since 7.1 since it impacts Moonfire DD
      // IN-GAME BUG (Jan 20 2018): Lady and the Child Galactic Guardian procs benefit from both the DD modifier and the rage gain.
      if ( ( benefits_from_galactic_guardian || ( p() -> bugs && s -> chain_target > 0 ) )
        && result_is_hit( s -> result ) && p() -> buff.galactic_guardian -> check() )
      {
        p() -> resource_gain( RESOURCE_RAGE, p() -> buff.galactic_guardian -> value(), p() -> gain.galactic_guardian );

        // buff is not consumed when bug occurs
        if ( benefits_from_galactic_guardian )
          p() -> buff.galactic_guardian -> expire();
      }
    }

    size_t available_targets( std::vector< player_t* >& tl ) const override
    {
      /* When Lady and the Child is active, this is an AoE action meaning it will impact onto the
      first 2 targets in the target list. Instead, we want it to impact on the target of the action
      and 1 additional, so we'll override the target_list to make it so. */
      if ( is_aoe() )
      {
        // Get the full target list.
        druid_spell_t::available_targets( tl );
        std::vector<player_t*> full_list = tl;

        tl.clear();
        // Add the target of the action.
        tl.push_back( target );

        // Loop through the full list and sort into afflicted/unafflicted.
        std::vector<player_t*> afflicted;
        std::vector<player_t*> unafflicted;

        for ( size_t i = 0; i < full_list.size(); i++ )
        {
          if ( full_list[ i ] == target )
          {
            continue;
          }

          if ( td( full_list[ i ] ) -> dots.moonfire -> is_ticking() )
          {
            afflicted.push_back( full_list[ i ] );
          }
          else
          {
            unafflicted.push_back( full_list[ i ] );
          }
        }

        // Fill list with random unafflicted targets.
        while ( tl.size() < as<size_t>(aoe) && unafflicted.size() > 0 )
        {
          // Random target
          size_t i = ( size_t ) p() -> rng().range( 0, ( double ) unafflicted.size() );

          tl.push_back( unafflicted[ i ] );
          unafflicted.erase( unafflicted.begin() + i );
        }

        // Fill list with random afflicted targets.
        while ( tl.size() < as<size_t>(aoe) && afflicted.size() > 0 )
        {
          // Random target
          size_t i = ( size_t ) p() -> rng().range( 0, ( double ) afflicted.size() );

          tl.push_back( afflicted[ i ] );
          afflicted.erase( afflicted.begin() + i );
        }

        return tl.size();
      }

      return druid_spell_t::available_targets( tl );
    }

    void execute() override
    {
      // Force invalidate target cache so that it will impact on the correct targets.
      target_cache.is_valid = false;

      druid_spell_t::execute();
    }
  };

  struct galactic_guardian_damage_t : public moonfire_damage_t
  {
    galactic_guardian_damage_t( druid_t* p ) :
      moonfire_damage_t( p, "galactic_guardian" )
    {
      benefits_from_galactic_guardian = false;
    }

    virtual void execute() override
    {
      moonfire_damage_t::execute();

      p() -> buff.galactic_guardian -> trigger();
    }
  };

  moonfire_damage_t* damage;

  moonfire_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "moonfire", player, player -> find_class_spell( "Moonfire" ), options_str )
  {
    may_miss = false;
    damage = new moonfire_damage_t( player );
    damage -> stats = stats;

    if (player->talent.twin_moons->ok())
    {
      radius = player->talent.twin_moons->effectN(1).trigger()->effectN(1).radius_max();
    }

    add_child(damage);
    if (player->active.galactic_guardian)
    {
      add_child(player->active.galactic_guardian);
    }

    if ( player -> spec.astral_power -> ok() )
    {
      energize_resource = RESOURCE_ASTRAL_POWER;
      energize_amount = player -> spec.astral_power -> effectN( 3 ).resource( RESOURCE_ASTRAL_POWER );
    }
    else
    {
      energize_type = ENERGIZE_NONE;
    }

    // Add damage modifiers in moonfire_damage_t, not here.
  }

  void impact( action_state_t* s ) override
  {
    druid_spell_t::impact(s);
    streaking_stars_trigger(SS_MOONFIRE, s);

    if ( result_is_hit( s -> result ) )
    {
      trigger_gore();
    }
  }

  void execute() override
  {
    druid_spell_t::execute();

    damage -> target = execute_state -> target;
    damage -> schedule_execute();

  }
};

}

namespace heals {

struct druid_heal_t : public druid_spell_base_t<heal_t>
{
  bool target_self;

  druid_heal_t( const std::string& token, druid_t* p,
                const spell_data_t* s = spell_data_t::nil(),
                const std::string& options_str = std::string() ) :
    base_t( token, p, s ),
    target_self( 0 )
  {
    add_option( opt_bool( "target_self", target_self ) );
    parse_options( options_str );

    if ( target_self )
      target = p;

    may_miss          = false;
    weapon_multiplier = 0;
    harmful           = false;
  }

public:
  virtual void execute() override
  {
    base_t::execute();

    if ( p() -> mastery.harmony -> ok() && spell_power_mod.direct > 0 && ! background )
      p() -> buff.harmony -> trigger( 1, p() -> mastery.harmony -> ok() ? p() -> cache.mastery_value() : 0.0 );
  }

  virtual double action_da_multiplier() const override
  {
    double adm = base_t::action_da_multiplier();

    adm *= 1.0 + p() -> buff.incarnation_tree -> check_value();

    if ( p() -> mastery.harmony -> ok() )
      adm *= 1.0 + p() -> cache.mastery_value();

    return adm;
  }

  virtual double action_ta_multiplier() const override
  {
    double atm = base_t::action_da_multiplier();

    atm *= 1.0 + p() -> buff.incarnation_tree -> check_value();

    if ( p() -> mastery.harmony -> ok() )
      atm *= 1.0 + p() -> cache.mastery_value();

    return atm;
  }

  void trigger_clearcasting()
  {
    if ( ! proc && p() -> specialization() == DRUID_RESTORATION && p() -> spec.omen_of_clarity -> ok() )
      p() -> buff.clearcasting -> trigger(); // Proc chance is handled by buff chance
  }
}; // end druid_heal_t

}

namespace caster_attacks {

// Caster Form Melee Attack =================================================

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
    school            = SCHOOL_PHYSICAL;
    may_glance        = background = repeating = true;
    trigger_gcd       = timespan_t::zero();
    special           = false;
    weapon_multiplier = 1.0;
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
protected:
  bool    attack_critical;
public:
  bool    requires_stealth;
  bool    consumes_combo_points;
  bool    consumes_clearcasting;
  bool    trigger_tier17_2pc;
  bool    snapshots_tf;
  bool    trigger_untamed_ferocity;

  // whether the attack gets a damage bonus from Moment of Clarity
  bool    moment_of_clarity;

  // Whether the attack benefits from mastery
  struct {
    bool direct, tick;
  } razor_claws;

  // Whether the attack benefits from TF/SR
  struct {
    bool direct, tick;
  } savage_roar;
  bool    tigers_fury;

  cat_attack_t( const std::string& token, druid_t* p, const spell_data_t* s = spell_data_t::nil(),
                const std::string& options = std::string() )
    : base_t( token, p, s ),
      requires_stealth( false ),
      consumes_combo_points( false ),
      consumes_clearcasting( data().affected_by( p->spec.omen_of_clarity->effectN( 1 ).trigger()->effectN( 1 ) ) ),
      trigger_tier17_2pc( false ),
      snapshots_tf( true ),
      moment_of_clarity( data().affected_by( p->spec.omen_of_clarity->effectN( 1 ).trigger()->effectN( 3 ) ) ),
      tigers_fury( data().affected_by( p->spec.tigers_fury->effectN( 1 ) ) ),
      trigger_untamed_ferocity( data().affected_by( p->azerite.untamed_ferocity.spell()->effectN( 2 ) ) )
  {
    parse_options( options );

    if ( data().cost( POWER_COMBO_POINT ) )
    {
      consumes_combo_points = true;
      form_mask |= CAT_FORM;
    }

    if (p->specialization() == DRUID_BALANCE || p->specialization() == DRUID_RESTORATION)
      ap_type = AP_NO_WEAPON;

    razor_claws.direct = data().affected_by( p -> mastery.razor_claws -> effectN( 1 ) );
    razor_claws.tick = data().affected_by( p -> mastery.razor_claws -> effectN( 2 ) );

    savage_roar.direct = data().affected_by( p -> spec.savage_roar -> effectN( 1 ) );
    savage_roar.tick = data().affected_by( p -> spec.savage_roar -> effectN( 2 ) );

    /* Sanity check Tiger's Fury to assure that benefit is the same for both direct
       and periodic damage. Because we're using composite_persistent_damage_multiplier
       we have to use a single value for the multiplier instead of being completely
       faithful to the spell data. */
    if ( tigers_fury != data().affected_by( p -> spec.tigers_fury -> effectN( 3 ) ) )
      p -> sim -> errorf( "%s (id=%i) spell data has inconsistent Tiger's Fury benefit.", name_str, id );

    if ( p -> spec.tigers_fury -> effectN( 1 ).percent() != p -> spec.tigers_fury -> effectN( 3 ).percent() )
      p -> sim -> errorf( "%s (id=%i) spell data has inconsistent Tiger's Fury modifiers.", name_str, id );

    if (trigger_untamed_ferocity && !p->azerite.untamed_ferocity.ok() )
      trigger_untamed_ferocity = false;

    // Apply Feral Druid Aura damage modifiers
    if ( p -> specialization() == DRUID_FERAL )
    {
       //dots
       if ( s -> affected_by( p -> spec.feral -> effectN(2) ))
       {
          base_td_multiplier *= 1.0 + p -> spec.feral -> effectN(2).percent();
       }
       //dd
       if ( s -> affected_by( p -> spec.feral -> effectN(1) ))
       {
          base_dd_multiplier *= 1.0 + p->spec.feral->effectN(1).percent();
       }
    }

    if ( p-> specialization() == DRUID_FERAL &&  p -> talent.soul_of_the_forest -> ok() &&
      ( data().affected_by( p -> talent.soul_of_the_forest -> effectN(2)) | data().affected_by( p -> talent.soul_of_the_forest -> effectN(3) )))
    {
       base_td_multiplier *= 1.0 + p->talent.soul_of_the_forest->effectN(3).percent();
       base_dd_multiplier *= 1.0 + p->talent.soul_of_the_forest->effectN(2).percent();
    }

    // Apply all Feral Affinity damage modifiers.
    if ( p -> talent.feral_affinity -> ok() )
    {
      for ( size_t i = 1; i <= p -> talent.feral_affinity -> effect_count(); i++ )
      {
        const spelleffect_data_t& effect = p -> talent.feral_affinity -> effectN( i );

        if ( ! ( effect.type() == E_APPLY_AURA && effect.subtype() == A_ADD_PCT_MODIFIER ) )
          continue;

        if ( data().affected_by( effect ) )
        {
          switch(static_cast<property_type_t>(effect.misc_value1()))
          {
            case P_GENERIC:
              base_dd_multiplier *= 1.0 + effect.percent();
              break;
            case P_TICK_DAMAGE:
              base_td_multiplier *= 1.0 + effect.percent();
              break;
            default:
              base_multiplier *= 1.0 + effect.percent();
              break;
          }
        }
      }
    }
  }

  // For effects that specifically trigger only when "prowling."
  virtual bool prowling() const
  {
    // Make sure we call all three methods for accurate benefit tracking.
    bool prowl = p() -> buff.prowl -> up(),
         inc   = p() -> buff.incarnation_cat -> up();

    return prowl || inc;
  }

  virtual bool stealthed() const // For effects that require any form of stealth.
  {
    // Make sure we call all three methods for accurate benefit tracking.
    bool shadowmeld = p() -> buffs.shadowmeld -> up(),
         prowl = prowling();

    return prowl || shadowmeld;
  }

  virtual double cost() const override
  {
    double c = base_t::cost();

    if ( c == 0 )
      return 0;

    if ( consumes_clearcasting && current_resource() == RESOURCE_ENERGY &&
      p() -> buff.clearcasting -> check() )
      return 0;

    c *= 1.0 + p() -> buff.berserk -> check_value();
    c *= 1.0 + p() -> buff.incarnation_cat -> check_value();

    return c;
  }

  virtual void consume_resource() override
  {
    // Treat Omen of Clarity energy savings like an energy gain for tracking purposes.
    if ( consumes_clearcasting && current_resource() == RESOURCE_ENERGY &&
      base_t::cost() > 0 && p() -> buff.clearcasting -> up() )
    {
      // Base cost doesn't factor in Berserk, but Omen of Clarity does net us less energy during it, so account for that here.
      double eff_cost = base_t::cost();
      eff_cost *= 1.0 + p() -> buff.berserk -> value();
      eff_cost *= 1.0 + p() -> buff.incarnation_cat -> check_value();

      p() -> gain.clearcasting -> add( RESOURCE_ENERGY, eff_cost );

      // Feral tier18 4pc occurs before the base cost is consumed.
      if ( p() -> sets -> has_set_bonus( DRUID_FERAL, T18, B4 ) )
      {
        p() -> resource_gain( RESOURCE_ENERGY,
          base_t::cost() * p() -> sets -> set( DRUID_FERAL, T18, B4 ) -> effectN( 1 ).percent(),
          p() -> gain.feral_tier18_4pc );
      }
    }

    base_t::consume_resource();

    if ( consumes_clearcasting && current_resource() == RESOURCE_ENERGY && base_t::cost() > 0 )
      p() -> buff.clearcasting -> decrement();

    if ( consumes_combo_points && hit_any_target )
    {
      int consumed = ( int ) p() -> resources.current[ RESOURCE_COMBO_POINT ];

      if ( p() -> buff.tigers_fury -> check() )
      {
         p() -> buff.tigers_fury -> extend_duration( p() , timespan_t::from_seconds(p() -> legendary.behemoth_headdress * consumed) );
      }

      p() -> resource_loss( RESOURCE_COMBO_POINT, consumed, nullptr, this );

      if ( sim -> log )
      {
        sim -> out_log.printf( "%s consumes %d %s for %s (0)",
          player -> name(),
          consumed,
          util::resource_type_string( RESOURCE_COMBO_POINT ),
          name() );
      }

      stats -> consume_resource( RESOURCE_COMBO_POINT, consumed );

      if ( p() -> spec.predatory_swiftness -> ok() )
        p() -> buff.predatory_swiftness -> trigger( 1, 1, consumed * 0.20 );

      if ( p() -> talent.soul_of_the_forest -> ok() && p() -> specialization() == DRUID_FERAL )
      {
        p() -> resource_gain( RESOURCE_ENERGY,
          consumed * p() -> talent.soul_of_the_forest -> effectN( 1 ).resource( RESOURCE_ENERGY ),
          p() -> gain.soul_of_the_forest );
      }
    }
  }

  virtual double composite_target_crit_chance( player_t* t ) const override
  {
    double tc = base_t::composite_target_crit_chance( t );

    if ( special && t -> debuffs.bleeding && t -> debuffs.bleeding -> check() )
      tc += p() -> talent.blood_scent -> effectN( 1 ).percent();

    return tc;
  }

  double action_multiplier() const override
  {
    double am = base_t::action_multiplier();

    if ( tigers_fury && ! snapshots_tf )
    {
      if ( special )
        am *= 1.0 + p() -> buff.tigers_fury -> check_value();
      else
        am *= 1.0 + p() -> buff.tigers_fury -> data().effectN( 4 ).percent();
    }

    return am;
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    double pm = base_t::composite_persistent_multiplier( s );

    if ( consumes_bloodtalons )
      pm *= 1.0 + p() -> buff.bloodtalons -> check_value();

    if ( moment_of_clarity )
      pm *= 1.0 + p() -> buff.clearcasting -> check_value();

    if ( tigers_fury && snapshots_tf )
      pm *= 1.0 + p() -> buff.tigers_fury -> check_value();

    return pm;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double dm = base_t::composite_da_multiplier( state );

    /* Modifiers for direct bleed damage. */
    if ( razor_claws.direct )
      dm *= 1.0 + p() -> cache.mastery_value();

    if ( savage_roar.direct && p() -> buff.savage_roar -> check() )
    {
      if ( special )
        dm *= 1.0 + p() -> spec.savage_roar -> effectN( 1 ).percent();
      else
        dm *= 1.0 + p() -> spec.savage_roar -> effectN( 3 ).percent();
    }

    return dm;
  }

  virtual double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double tm = base_t::composite_ta_multiplier( s );

    if ( razor_claws.tick ) // Don't need to check school, it modifies all periodic damage.
      tm *= 1.0 + p() -> cache.mastery_value();

    if ( savage_roar.tick && p() -> buff.savage_roar -> check() )
      tm *= 1.0 + p() -> spec.savage_roar -> effectN( 2 ).percent();

    return tm;
  }

  virtual double bonus_da(const action_state_t* s) const override
  {
    double da = base_t::bonus_da(s);

    if (trigger_untamed_ferocity)
      da += p()->azerite.untamed_ferocity.value(2);

    return da;
  }

  virtual void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( result_is_hit( s -> result ) && s -> result == RESULT_CRIT )
      attack_critical = true;
  }

  void tick( dot_t* d ) override
  {
    base_t::tick( d );

    trigger_predator();
    trigger_wildshapers_clutch( d -> state );

    if ( trigger_tier17_2pc )
    {
      p() -> resource_gain( RESOURCE_ENERGY,
        p() -> sets -> set( DRUID_FERAL, T17, B2 ) -> effectN( 1 ).base_value(),
        p() -> gain.feral_tier17_2pc );
    }
  }

  virtual void execute() override
  {
    attack_critical = false;

    base_t::execute();

    if ( energize_resource == RESOURCE_COMBO_POINT && energize_amount > 0 && hit_any_target )
    {
      if ( attack_critical && p() -> specialization() == DRUID_FERAL )
      {
        trigger_primal_fury();
      }
    }

    if ( hit_any_target && trigger_untamed_ferocity )
    {
      p()->cooldown.berserk->adjust( -p()->azerite.untamed_ferocity.time_value( 3 ), false );
      p()->cooldown.incarnation->adjust( -p()->azerite.untamed_ferocity.time_value( 4 ), false );
    }

    if ( ! hit_any_target )
      trigger_energy_refund();

    if ( harmful )
    {
      p() -> buff.prowl -> expire();
      p() -> buffs.shadowmeld -> expire();

      // Track benefit of damage buffs
      p() -> buff.tigers_fury -> up();
      p() -> buff.savage_roar -> up();
      if ( special && base_costs[ RESOURCE_ENERGY ] > 0 )
        p() -> buff.berserk -> up();
    }
  }

  virtual bool ready() override
  {
    if ( requires_stealth && ! stealthed() )
      return false;

    if ( consumes_combo_points && p() -> resources.current[ RESOURCE_COMBO_POINT ] < 1 )
      return false;

    return base_t::ready();
  }

  void trigger_energy_refund()
  {
    player -> resource_gain( RESOURCE_ENERGY, last_resource_cost * 0.80,
      p() -> gain.energy_refund );
  }

  virtual void trigger_primal_fury()
  {
    if ( ! p() -> spec.primal_fury -> ok() )
      return;

    p() -> proc.primal_fury -> occur();
    p() -> resource_gain( RESOURCE_COMBO_POINT, p() -> spec.primal_fury ->
      effectN( 1 ).base_value(), p() -> gain.primal_fury );
  }

  void trigger_predator()
  {
    if ( ! ( p() -> talent.predator -> ok() && p() -> predator_rppm_rate > 0 ) )
      return;
    if ( ! p() -> rppm.predator -> trigger() )
      return;

    if ( ! p() -> cooldown.tigers_fury -> down() )
      p() -> proc.predator_wasted -> occur();

    p() -> cooldown.tigers_fury -> reset( true );
    p() -> proc.predator -> occur();
  }

  void trigger_wildshapers_clutch( action_state_t* s )
  {
    if ( p() -> legendary.the_wildshapers_clutch == 0.0 )
      return;
    if ( ! dbc::is_school( school, SCHOOL_PHYSICAL ) && s->action->id != 210705) // bleeds only TODO(feral): Refactor this.
      return;
    if ( s -> result != RESULT_CRIT )
      return;
    if ( s -> result_amount <= 0 )
      return;

    if ( p() -> rng().roll( p() -> legendary.the_wildshapers_clutch ) )
    {
      p() -> proc.the_wildshapers_clutch -> occur();
      trigger_primal_fury();
    }
  }
}; // end druid_cat_attack_t

// Cat Melee Attack =========================================================

struct cat_melee_t : public cat_attack_t
{
  cat_melee_t( druid_t* player ) :
    cat_attack_t( "cat_melee", player, spell_data_t::nil(), "" )
  {
    form_mask = CAT_FORM;

    may_glance = background = repeating = true;
    school            = SCHOOL_PHYSICAL;
    trigger_gcd       = timespan_t::zero();
    special           = false;
    weapon_multiplier = 1.0;

    // Manually enable benefit from TF & SR on autos because it isn't handled
    // by the affected_by() magic in cat_attack_t.
    savage_roar.direct = true;
    tigers_fury = true;
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

// Rip State ================================================================

struct rip_state_t : public action_state_t
{
  int combo_points;
  druid_t* druid;

  rip_state_t( druid_t* p, action_t* a, player_t* target ) : action_state_t( a, target ), combo_points( 0 ), druid( p )
  {
  }

  void initialize() override
  {
    action_state_t::initialize();

    combo_points = ( int ) druid -> resources.current[ RESOURCE_COMBO_POINT ];
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

// Berserk ==================================================================

struct berserk_t : public cat_attack_t
{
  berserk_t( druid_t* player, const std::string& options_str ) :
    cat_attack_t( "berserk", player, player -> find_specialization_spell( "Berserk" ), options_str )
  {
    harmful = may_miss = may_parry = may_dodge = may_crit = false;
  }

  void execute() override
  {
    cat_attack_t::execute();

    p() -> buff.berserk -> trigger();

    if ( p() -> sets -> has_set_bonus( DRUID_FERAL, T17, B4 ) )
      p() -> buff.feral_tier17_4pc -> trigger();
  }

  bool ready() override
  {
    if ( p() -> talent.incarnation_cat -> ok() )
      return false;

    return cat_attack_t::ready();
  }
};

// Brutal Slash =============================================================

struct brutal_slash_t : public cat_attack_t
{
  brutal_slash_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "brutal_slash", p, p -> talent.brutal_slash, options_str )
  {
    aoe = -1;
    energize_amount = data().effectN( 2 ).base_value();
    energize_resource = RESOURCE_COMBO_POINT;
    energize_type = ENERGIZE_ON_HIT;
    cooldown -> hasted = true;
 
  }

  double cost() const override
  {
    double c = cat_attack_t::cost();
 
    c -= p()->buff.scent_of_blood->check_stack_value();

    return c;
  }

  virtual double bonus_da(const action_state_t* s) const override
  {
    double b = cat_attack_t::bonus_da(s);

    if (td(s->target)->dots.thrash_cat->is_ticking() ||
        td(s->target)->dots.thrash_bear->is_ticking())
    {
      b += p()->azerite.wild_fleshrending.value(2);
    }

    return b;
  }

  virtual double composite_target_multiplier(player_t* t) const override
  {
     double tm = cat_attack_t::composite_target_multiplier(t);

     if (!t->bugs && p()->sets -> has_set_bonus(DRUID_FERAL, T19, B4)) //currently bugged in-game. (2017-03-07)
     {
        tm *= 1.0 + td(t)->feral_tier19_4pc_bleeds() *
           p()->sets -> set(DRUID_FERAL, T19, B4)->effectN(1).percent();
     }

     return tm;
  }
};

//==== Feral Frenzy ==============================================

//Currently incorrect TODO(Xanzara)
struct feral_frenzy_driver_t : public cat_attack_t
{
  struct feral_frenzy_dot_t : public cat_attack_t
  {
    bool is_direct_damage;

    feral_frenzy_dot_t(druid_t* p) : cat_attack_t("feral_frenzy_tick", p, p->find_spell(274838))
    {
      background = dual = true;
      is_direct_damage = false;
      direct_bleed = false;
      dot_max_stack = 5;
      //dot_behavior = DOT_CLIP;
    }
    
    //Refreshes, but doesn't pandemic
    timespan_t calculate_dot_refresh_duration(const dot_t* dot, timespan_t triggered_duration) const override
    {
      return dot->time_to_next_tick() + triggered_duration;
    }

    //Small hack to properly distinguish instant ticks from the driver, from actual periodic ticks from the bleed
    dmg_e report_amount_type(const action_state_t* state) const override
    {
      if (is_direct_damage) return DMG_DIRECT;

      return state->result_type;
    }

    void execute() override
    {
      is_direct_damage = true;
      cat_attack_t::execute();
      is_direct_damage = false;
    }

    void impact(action_state_t* s) override
    {
      cat_attack_t::impact(s);
    }

    void trigger_primal_fury() override
    {}

  };

  double tick_ap_ratio;
  feral_frenzy_driver_t(druid_t* p, const std::string& /*options_str*/) :
    cat_attack_t("feral_frenzy", p, p->find_spell(274837) )
  {
    //ffdot = new feral_frenzy_dot_t(p);
    tick_action = new feral_frenzy_dot_t(p);
    tick_action->stats = stats;
    //hasted_ticks = true;
    dynamic_tick_action = true;
    tick_ap_ratio = p->find_spell(274838)->effectN(3).ap_coeff();
  }

  virtual bool ready() override
  {
    if (!p()->talent.feral_frenzy->ok())
      return false;

    return cat_attack_t::ready();
  }
};


// Ferocious Bite ===========================================================

struct ferocious_bite_t : public cat_attack_t
{
  double excess_energy;
  double max_excess_energy;
  double combo_points;
  bool max_energy;

  ferocious_bite_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "ferocious_bite", p, p -> find_affinity_spell( "Ferocious Bite" ) ),
    excess_energy( 0 ), max_excess_energy( 0 ), max_energy( false )
  {
    add_option( opt_bool( "max_energy" , max_energy ) );
    parse_options( options_str );

    max_excess_energy = 1 * data().effectN( 2 ).base_value();

    if ( p -> talent.sabertooth -> ok() )
    {
       base_multiplier *= 1.0 + p -> talent.sabertooth -> effectN(1).percent();
    }
  }

  double maximum_energy() const
  {
    double req = base_costs[ RESOURCE_ENERGY ] + max_excess_energy;

    req *= 1.0 + p() -> buff.berserk -> check_value();
    req *= 1.0 + p() -> buff.incarnation_cat -> check_value();

    if ( p() -> buff.apex_predator -> check() )
    {
       req *= ( 1 - p() -> buff.apex_predator -> data().effectN(1).percent() );
    }

    return req;
  }

  bool ready() override
  {
    if ( max_energy && p() -> resources.current[ RESOURCE_ENERGY ] < maximum_energy() )
      return false;

    return cat_attack_t::ready();
  }

  double cost() const override
  {
    double c = cat_attack_t::cost();

    if ( p() -> buff.apex_predator -> check() )
    {
      c *= (1 - p() -> buff.apex_predator -> data().effectN(1).percent() );
    }

    return c;
  }

  void execute() override
  {
    // Berserk does affect the additional energy consumption.
    max_excess_energy *= 1.0 + p() -> buff.berserk -> value();
    max_excess_energy *= 1.0 + p() -> buff.incarnation_cat -> check_value();

    excess_energy = std::min( max_excess_energy,
      ( p() -> resources.current[ RESOURCE_ENERGY ] - cat_attack_t::cost() ) );

    combo_points = p() -> resources.current[RESOURCE_COMBO_POINT];

    cat_attack_t::execute();

    max_excess_energy = 1 * data().effectN( 2 ).base_value();

    p()->buff.iron_jaws->trigger( 1,
                                  p()->azerite.iron_jaws.value( 1 ) * ( 0.5 + 0.5 / p()->azerite.iron_jaws.n_items()) ,
                                  p()->azerite.iron_jaws.spell()->effectN(2).percent() * combo_points );

    p()->buff.raking_ferocity->expire();
  }

  virtual double bonus_da( const action_state_t* s ) const override
  {
    double da = cat_attack_t::bonus_da(s);

    da += p()->buff.raking_ferocity->value();

    return da;
  }

  void impact( action_state_t* s ) override
  {
    cat_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if (p()->talent.sabertooth->ok())
      {
        td(s->target)
          ->dots.rip->extend_duration(
            timespan_t::from_seconds(p()->talent.sabertooth->effectN(2).base_value() * combo_points),
            timespan_t::from_seconds(32), 0);
      }
    }
  }

  void ApexPredatorResource()
  {
     if ( p() -> buff.tigers_fury -> check() )
     {
        p() -> buff.tigers_fury -> extend_duration( p(), timespan_t::from_seconds( p() -> legendary.behemoth_headdress * 5 ));
     }

     if ( p() -> spec.predatory_swiftness -> ok() )
        p() -> buff.predatory_swiftness -> trigger( 1, 1, 5 * 0.20 );

     if ( p() -> talent.soul_of_the_forest -> ok() && p() -> specialization() == DRUID_FERAL )
     {
        p() -> resource_gain( RESOURCE_ENERGY,
           5 * p() -> talent.soul_of_the_forest -> effectN(1).resource( RESOURCE_ENERGY ),
           p() -> gain.soul_of_the_forest);
     }

     p() -> buff.apex_predator -> expire();

  }

  void consume_resource() override
  {
     if ( p() -> buff.apex_predator -> check() )
     {
        ApexPredatorResource();
        return;
     }

    /* Extra energy consumption happens first. In-game it happens before the
       skill even casts but let's not do that because its dumb. */
    if ( hit_any_target )
    {
      player -> resource_loss( current_resource(), excess_energy );
      stats -> consume_resource( current_resource(), excess_energy );
    }

    cat_attack_t::consume_resource();
  }

  double action_multiplier() const override
  {
    double am = cat_attack_t::action_multiplier();

    if ( p() -> buff.apex_predator -> up() )
    {
      am *= 2.0;
      return am;
    }

    am *= p() -> resources.current[ RESOURCE_COMBO_POINT ]
      / p() -> resources.max[ RESOURCE_COMBO_POINT ];

    am *= 1.0 + excess_energy / max_excess_energy;

    return am;
  }
};

// Gushing Wound (tier17_feral_4pc) =========================================

struct gushing_wound_t : public residual_action::residual_periodic_action_t<cat_attack_t>
{
  gushing_wound_t( druid_t* p ) :
    residual_action::residual_periodic_action_t<cat_attack_t>( "gushing_wound", p, p -> find_spell( 166638 ) )
  {
    background = dual = proc = true;
    may_miss = may_dodge = may_parry = false;
    trigger_tier17_2pc = p -> sets -> has_set_bonus( DRUID_FERAL, T17, B2 );
  }
};

// Lunar Inspiration ========================================================

struct lunar_inspiration_t : public cat_attack_t
{
  lunar_inspiration_t( druid_t* player, const std::string& options_str ) :
    cat_attack_t( "lunar_inspiration", player, player -> find_spell( 155625 ), options_str )
  {
    may_dodge = may_parry = may_block = may_glance = false;

    snapshots_tf = true;
    hasted_ticks = true;
    energize_type = ENERGIZE_ON_HIT;

    hasted_gcd = true;
  }

  void init() override
  {
    cat_attack_t::init();

    consumes_bloodtalons = false;
  }

  size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    /* When Lady and the Child is active, this is an AoE action meaning it will impact onto the
    first 2 targets in the target list. Instead, we want it to impact on the target of the action
    and 1 additional, so we'll override the target_list to make it so. */
    if ( is_aoe() )
    {
      // Get the full target list.
      cat_attack_t::available_targets( tl );
      std::vector<player_t*> full_list = tl;

      tl.clear();
      // Add the target of the action.
      tl.push_back( target );

      // Loop through the full list and sort into afflicted/unafflicted.
      std::vector<player_t*> afflicted;
      std::vector<player_t*> unafflicted;

      for ( size_t i = 0; i < full_list.size(); i++ )
      {
        if ( full_list[ i ] == target )
        {
          continue;
        }

        if ( td( full_list[ i ] ) -> dots.moonfire -> is_ticking() )
        {
          afflicted.push_back( full_list[ i ] );
        }
        else
        {
          unafflicted.push_back( full_list[ i ] );
        }
      }

      // Fill list with random unafflicted targets.
      while ( tl.size() < as<size_t>(aoe) && unafflicted.size() > 0 )
      {
        // Random target
        size_t i = ( size_t ) p() -> rng().range( 0, ( double ) unafflicted.size() );

        tl.push_back( unafflicted[ i ] );
        unafflicted.erase( unafflicted.begin() + i );
      }

      // Fill list with random afflicted targets.
      while ( tl.size() < as<size_t>(aoe) && afflicted.size() > 0 )
      {
        // Random target
        size_t i = ( size_t ) p() -> rng().range( 0, ( double ) afflicted.size() );

        tl.push_back( afflicted[ i ] );
        afflicted.erase( afflicted.begin() + i );
      }

      return tl.size();
    }

    return cat_attack_t::available_targets( tl );
  }

  virtual double bonus_ta( const action_state_t* s ) const override
  {
    double ta = cat_attack_t::bonus_ta( s );

    ta += p()->azerite.power_of_the_moon.value( 2 );

    return ta;
  }

  void execute() override
  {
    // Force invalidate target cache so that it will impact on the correct targets.
    target_cache.is_valid = false;

    cat_attack_t::execute();
  }

  virtual bool ready() override
  {
    if ( ! p() -> talent.lunar_inspiration -> ok() )
      return false;

    return cat_attack_t::ready();
  }
};

// Maim =====================================================================

struct maim_t : public cat_attack_t
{
  maim_t( druid_t* player, const std::string& options_str ) :
    cat_attack_t( "maim", player, player -> find_specialization_spell( "Maim" ), options_str )
  {
  }

  int n_targets() const override
  {
    int n = cat_attack_t::n_targets();

    //n += p() -> buff.fiery_red_maimers -> data().effectN( 1 ).base_value();

    return n;
  }

  double action_multiplier() const override
  {
    double am = cat_attack_t::action_multiplier();

    am *= p() -> resources.current[ RESOURCE_COMBO_POINT ];

    //am *= 1.0 + p() -> buff.fiery_red_maimers -> check_value();

    return am;
  }

  virtual double bonus_da( const action_state_t* s ) const override
  {
    double da = cat_attack_t::bonus_da( s );

    if (p()->buff.iron_jaws->up())
      da += p()->buff.iron_jaws->check_value();

    return da;
  }

  void execute() override
  {
    cat_attack_t::execute();

    if ( p()->buff.iron_jaws->up() )
      p()->buff.iron_jaws->expire();

    /*if ( p()->buff.fiery_red_maimers->up() )
    {
      p()->buff.fiery_red_maimers->expire();
    }*/
  }
};

// Rake =====================================================================

struct rake_t : public cat_attack_t
{
  struct rake_bleed_t : public cat_attack_t
  {
    rake_bleed_t( druid_t* p ) :
      cat_attack_t( "rake_bleed", p, p -> find_spell( 155722 ) )
    {
      background = dual = true;
      may_miss = may_parry = may_dodge = may_crit = false;
      hasted_ticks = true;

      base_tick_time *= 1.0 + p -> talent.jagged_wounds -> effectN( 1 ).percent();
      dot_duration   *= 1.0 + p -> talent.jagged_wounds -> effectN( 2 ).percent();

      // Direct damage modifiers go in rake_t!
      trigger_tier17_2pc = p -> sets -> has_set_bonus( DRUID_FERAL, T17, B2 );
    }

    dot_t* get_dot( player_t* t ) override
    {
      if ( ! t ) t = target;
      if ( ! t ) return nullptr;

      return td( t ) -> dots.rake;
    }

    virtual double bonus_ta( const action_state_t* s ) const override
    {
      double ta = cat_attack_t::bonus_ta( s );

      ta += p()->azerite.blood_mist.value( 2 );

      return ta;
    }

    void tick( dot_t* d ) override
    {
      cat_attack_t::tick( d );

      if ( !p()->azerite.blood_mist.ok() )
        return;
      if ( p()->buff.berserk->check() || p()->buff.incarnation_cat->check() )
        return;
      if ( !p()->rppm.blood_mist->trigger() )
        return;

      p()->proc.blood_mist->occur();
      p()->buff.berserk->trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, timespan_t::from_seconds( 6 ) );
    }
  };

  action_t* bleed;

  rake_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "rake", p, p -> find_affinity_spell( "Rake" ) )
  {
    parse_options( options_str );

    bleed = p -> find_action( "rake_bleed" );

    if ( ! bleed )
    {
      bleed = new rake_bleed_t( p );
      bleed -> stats = stats;
    }

    // Periodic damage modifiers go in rake_bleed_t!
    trigger_tier17_2pc = p -> sets -> has_set_bonus( DRUID_FERAL, T17, B2 );
 }

  dot_t* get_dot(player_t* t) override
  {
    if (!t) t = target;
    if (!t) return nullptr;

    return td(t)->dots.rake;
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    double pm = cat_attack_t::composite_persistent_multiplier( s );

    if ( stealthed() )
      pm *= 1.0 + data().effectN( 4 ).percent();

    return pm;
  }

  void impact( action_state_t* s ) override
  {
    cat_attack_t::impact( s );

    action_state_t* b_state = bleed -> get_state();
    b_state -> target = s -> target;
    bleed -> snapshot_state( b_state, DMG_OVER_TIME );
    // Copy persistent multipliers from the direct attack.
    b_state -> persistent_multiplier = s -> persistent_multiplier;
    bleed -> schedule_execute( b_state );
  }

  void execute() override
  {
    cat_attack_t::execute();

    if ( p()->azerite.raking_ferocity.ok() )
    {
      p()->buff.raking_ferocity->trigger( 1, p()->azerite.raking_ferocity.value() );
    }
  }
};

// Rip ======================================================================

struct rip_t : public cat_attack_t
{
  // Bloody Gash ============================================================

  struct bloody_gash_t : public cat_attack_t
  {
    bloody_gash_t( druid_t* p ) :
      cat_attack_t( "bloody_gash", p, p -> spec.bloody_gash )
    {
      background = dual = proc = true;
      may_miss = may_dodge = may_parry = may_crit = false;
      may_block = true;
    }

    virtual void init() override
    {
      cat_attack_t::init();

      snapshot_flags &= STATE_NO_MULTIPLIER;
      snapshot_flags |= STATE_TGT_MUL_DA;
    }

    void execute() override
    {
      cat_attack_t::execute();

      p() -> buff.apex_predator -> trigger();
    }

    void impact( action_state_t* s ) override
    {
      cat_attack_t::impact(s);

      trigger_wildshapers_clutch( s );
    }
  };

  double combo_point_on_tick_proc_rate;
  action_t* bloody_gash;

  rip_t( druid_t* p, const std::string& options_str )
    : cat_attack_t( "rip", p, p->find_affinity_spell( "Rip" ), options_str )
  {
    special      = true;
    may_crit     = false;

    hasted_ticks = true;

    trigger_tier17_2pc = p -> sets -> has_set_bonus( DRUID_FERAL, T17, B2 );

    if ( p-> sets -> has_set_bonus( DRUID_FERAL, T20, B4) )
    {
       base_multiplier *= (1.0 + p-> find_spell(242235) -> effectN(1).percent());
       dot_duration += timespan_t::from_millis( p->find_spell(242235) -> effectN(2).base_value() );
    }

    base_tick_time *= 1.0 + p -> talent.jagged_wounds -> effectN( 1 ).percent();
    dot_duration   *= 1.0 + p -> talent.jagged_wounds -> effectN( 2 ).percent();

    if ( p -> sets -> has_set_bonus( DRUID_FERAL, T20, B2) )
    {
       energize_amount = p->find_spell(245591)->effectN(1).base_value();
       energize_resource = RESOURCE_ENERGY;
       energize_type = ENERGIZE_PER_TICK;
    }

    combo_point_on_tick_proc_rate = 0.0;
    if ( p -> azerite.gushing_lacerations.ok() )
    {
      combo_point_on_tick_proc_rate = p->find_spell( 279468 )->proc_chance();
    }

    if ( p -> sets -> has_set_bonus( DRUID_FERAL, T21, B2 ) )
    {
      bloody_gash = p -> find_action( "bloody_gash" );

      if ( ! bloody_gash )
      {
        bloody_gash = new bloody_gash_t( p );
        add_child( bloody_gash );
      }
    }
  }

  action_state_t* new_state() override
  { return new rip_state_t( p(), this, target ); }

  timespan_t composite_dot_duration( const action_state_t* s ) const
  {
    timespan_t t = cat_attack_t::composite_dot_duration( s );

    return t *= ( p()->resources.current[ RESOURCE_COMBO_POINT ] + 1 );

    return t;
  }

  double attack_tick_power_coefficient( const action_state_t* s ) const override
  {
    return cat_attack_t::attack_tick_power_coefficient( s );
  }

  virtual double bonus_ta( const action_state_t* s ) const override
  {
    double ta = cat_attack_t::bonus_ta( s );

    ta += p()->azerite.gushing_lacerations.value( 2 );

    if ( p()->buff.berserk->up() || p()->buff.incarnation_cat->up() )
    {
      ta += p()->azerite.primordial_rage.value();
    }

    return ta;
  }

  void impact( action_state_t* s ) override
  {
    cat_attack_t::impact( s );
  }

  void tick( dot_t* d ) override
  {
     base_t::tick( d );

     trigger_wildshapers_clutch( d -> state );
     trigger_bloody_gash( d );

     p() -> buff.apex_predator -> trigger();

    if ( p()->rng().roll( combo_point_on_tick_proc_rate ) )
    {
      p()->resource_gain( RESOURCE_COMBO_POINT, 1, p()->gain.gushing_lacerations );
      p()->proc.gushing_lacerations->occur();
    }
  }

  void trigger_bloody_gash( dot_t* d )
  {
    if ( ! p() -> sets -> has_set_bonus( DRUID_FERAL, T21, B2 ) )
      return;

    if ( p() -> rng().roll( p() -> find_spell( 251789 ) -> proc_chance() ) )
    {
      bloody_gash -> target = d -> target;
      bloody_gash -> base_dd_min = bloody_gash -> base_dd_max = d -> state -> result_total;
      bloody_gash -> execute();
    }
  }
};

// Primal Wrath =============================================================
struct primal_wrath_t : public cat_attack_t
{
  int combo_points;
  rip_t* rip;

  primal_wrath_t( druid_t* p, const std::string& options_str )
    : cat_attack_t( "primal_wrath", p, p->talent.primal_wrath, options_str ), combo_points( 0 )
  {
    parse_options( options_str );

    snapshots_tf          = true;
    consumes_bloodtalons  = true;
    consumes_clearcasting = false;
    special               = true;
    aoe = -1;

    // rip = (rip_t*)p->find_action("rip");

    /*if (!rip)
    {*/
    rip = new rip_t( p, "" );
    //}
    rip->stats = stats;

    // stats->add_child(rip->stats);

    // add_child(rip);
  }

  dot_t* get_dot( player_t* t ) override
  {
    if ( !t )
      t = target;
    if ( !t )
      return nullptr;

    return td( t )->dots.rip;
  }

  double attack_direct_power_coefficient(const action_state_t* s) const override
  {
    double adpc = cat_attack_t::attack_direct_power_coefficient(s);

    adpc *= 1 + combo_points;

    return adpc;
  }

  void impact( action_state_t* s ) override
  {
    cat_attack_t::impact( s );

    auto b_state = rip->get_state();
    b_state->target      = s->target;
    rip->snapshot_state( b_state, DMG_OVER_TIME );
    // Copy persistent multipliers from the direct attack.
    b_state->persistent_multiplier = s->persistent_multiplier;

    if ( !td( s->target )->dots.rip->state )
    {
      td( s->target )->dots.rip->state          = rip->get_state();
      td( s->target )->dots.rip->current_action = rip;
    }

    td( s->target )->dots.rip->state->copy_state( b_state );
    td( s->target )->dots.rip->trigger( rip->dot_duration * 0.5 * ( combo_points + 1 ) );  // this seems to be hardcoded

    action_state_t::release( b_state );
  }

  void execute() override
  {
    combo_points = as<int>(p()->resources.current[RESOURCE_COMBO_POINT]);

    cat_attack_t::execute();
  }

};

// Savage Roar ==============================================================

struct savage_roar_t : public cat_attack_t
{
  savage_roar_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "savage_roar", p, p -> talent.savage_roar, options_str )
  {
    consumes_combo_points = true; // Missing from spell data.
    may_crit = may_miss = harmful = false;
    dot_duration = base_tick_time = timespan_t::zero();
  }

  /* We need a custom implementation of Pandemic refresh mechanics since our ready()
     method relies on having the correct final value. */
  timespan_t duration( int combo_points = -1 )
  {
    if ( combo_points == -1 )
      combo_points = ( int ) p() -> resources.current[ RESOURCE_COMBO_POINT ];

    timespan_t d = data().duration() + data().duration() * combo_points;

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
    // Savage Roar may not be cast if the new duration is less than that of the current
    if ( duration() < p() -> buff.savage_roar -> remains() )
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
    // Base spell generates 0 CP, Feral passive or Feral Affinity increase it to 1 CP.
    /*energize_amount += p -> spec.feral -> effectN( 3 ).base_value()
      + p -> talent.feral_affinity -> effectN( 8 ).base_value();*/

    energize_amount +=
        p->query_aura_effect( p->spec.feral, E_APPLY_AURA, A_ADD_FLAT_MODIFIER, 12, p->find_class_spell( "Shred" ) )
            ->base_value() +
        p -> talent.feral_affinity -> effectN( 8 ).base_value();
  }

  virtual void impact( action_state_t* s ) override
  {
    cat_attack_t::impact( s );

    if ( s -> result == RESULT_CRIT && p() -> sets -> has_set_bonus( DRUID_FERAL, PVP, B4 ) )
      td( s -> target ) -> debuff.bloodletting -> trigger(); // Druid module debuff
  }

  virtual void execute() override
  {
    cat_attack_t::execute();
    
    if ( p()->buff.shredding_fury->up() )
      p()->buff.shredding_fury->decrement();
    
  }

  virtual double bonus_da( const action_state_t* s ) const override
  {
    double b = cat_attack_t::bonus_da( s );

    if ( td( s->target )->dots.thrash_cat->is_ticking() ||
         td( s->target )->dots.thrash_bear->is_ticking() )
    {
      b += p()->azerite.wild_fleshrending.value( 1 );
    }

    if ( p()->buff.shredding_fury->check() )
    {
      b += p()->buff.shredding_fury->value();
    }

    return b;
  }

  virtual double composite_target_multiplier( player_t* t ) const override
  {
    double tm = cat_attack_t::composite_target_multiplier( t );

    if ( t -> debuffs.bleeding && t -> debuffs.bleeding -> up() )
      tm *= 1.0 + p() -> spec.shred -> effectN( 4 ).percent();

    if ( p() -> sets -> has_set_bonus( DRUID_FERAL, T19, B4 ) )
    {
      tm *= 1.0 + td( t ) -> feral_tier19_4pc_bleeds() *
        p() -> sets -> set( DRUID_FERAL, T19, B4 ) -> effectN( 1 ).percent();
    }

    return tm;
  }

  double composite_crit_chance_multiplier() const override
  {
    double cm = cat_attack_t::composite_crit_chance_multiplier();

    if ( stealthed() )
      cm *= 2.0;

    return cm;
  }

  double action_multiplier() const override
  {
    double m = cat_attack_t::action_multiplier();

    if ( stealthed() )
      m *= 1.0 + data().effectN( 4 ).percent();

    return m;
  }
};

// Swipe (Cat) ====================================================================

struct swipe_cat_t : public cat_attack_t
{
public:
  swipe_cat_t( druid_t* player, const std::string& options_str ) :
    cat_attack_t( "swipe_cat", player, player -> find_affinity_spell( "Swipe" ) -> ok() ?
      player -> spec.swipe_cat : spell_data_t::not_found(), options_str )
  {
    aoe = -1;
    energize_amount = data().effectN(1).base_value();
    energize_resource = RESOURCE_COMBO_POINT;
    energize_type = ENERGIZE_ON_HIT;

  }

  double cost() const override
  {
    double c = cat_attack_t::cost();

    c -= p()->buff.scent_of_blood->check_stack_value();

    return c;
  }

  virtual double bonus_da( const action_state_t* s ) const override
  {
    double b = cat_attack_t::bonus_da( s );

    if ( td( s->target )->dots.thrash_cat->is_ticking() ||
         td( s->target )->dots.thrash_bear->is_ticking() )
    {
      b += p()->azerite.wild_fleshrending.value( 2 );
    }

    return b;
  }

  virtual void execute() override
  {
    cat_attack_t::execute();
  }

  virtual double composite_target_multiplier( player_t* t ) const override
  {
    double tm = cat_attack_t::composite_target_multiplier( t );

    if ( t -> debuffs.bleeding && t -> debuffs.bleeding -> up() )
      tm *= 1.0 + data().effectN( 2 ).percent();

    if ( p() -> sets -> has_set_bonus( DRUID_FERAL, T19, B4 ) )
    {
      tm *= 1.0 + td( t ) -> feral_tier19_4pc_bleeds() *
        p() -> sets -> set( DRUID_FERAL, T19, B4 ) -> effectN( 1 ).percent();
    }

    return tm;
  }

  virtual bool ready() override
  {
    if ( p() -> talent.brutal_slash -> ok() )
      return false;

    return cat_attack_t::ready();
  }
};

// Tiger's Fury =============================================================

struct tigers_fury_t : public cat_attack_t
{
  timespan_t duration;

  tigers_fury_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "tigers_fury", p, p -> spec.tigers_fury, options_str ),
    duration( p -> buff.tigers_fury -> buff_duration )
  {
    harmful = may_miss = may_parry = may_dodge = may_crit = false;
    autoshift = form_mask = CAT_FORM;
    energize_type = ENERGIZE_ON_CAST;
    energize_amount += p -> spec.tigers_fury_2 -> effectN( 1 ).resource( RESOURCE_ENERGY );

    if ( p->talent.predator->ok() )
      duration += p->talent.predator->effectN( 1 ).time_value();

    if ( p->azerite.jungle_fury.enabled() )
    {
      /*if ( p->talent.predator->ok() )
        duration = p->azerite.jungle_fury.time_value( 1, azerite_power_t::S );
      else
        duration = p->azerite.jungle_fury.time_value( 2, azerite_power_t::S );*/
      duration += timespan_t::from_millis( 2000 );
    }
  }

  void execute() override
  {
    cat_attack_t::execute();

    p()->buff.tigers_fury->trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, duration );

    //p()->buff.jungle_fury->trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, duration );
  }
};

// Thrash (Cat) =============================================================

struct thrash_cat_t : public cat_attack_t
{
  thrash_cat_t( druid_t* p, const std::string& options_str ):
    cat_attack_t( "thrash_cat", p, p -> find_spell( 106830 ), options_str )
  {
    aoe = -1;
    spell_power_mod.direct = 0;
    //amount_delta = 0.25;
    hasted_ticks = true;

    trigger_tier17_2pc = p -> sets -> has_set_bonus( DRUID_FERAL, T17, B2 );

    // For some reason this is in a different spell
    energize_amount = p -> find_spell( 211141 ) -> effectN(1).base_value();
    energize_resource = RESOURCE_COMBO_POINT;
    energize_type = ENERGIZE_ON_HIT;

    if ( p -> sets -> has_set_bonus( DRUID_FERAL, T19, B2 ) )
    {
      base_multiplier *= 1.0 + p -> find_spell( 211140 ) -> effectN(1).percent();
    }

    base_tick_time *= 1.0 + p -> talent.jagged_wounds -> effectN( 1 ).percent();
    dot_duration *= 1.0 + p -> talent.jagged_wounds -> effectN( 2 ).percent();
  }

  void impact( action_state_t* s ) override
  {
    cat_attack_t::impact( s );

    if ( p() -> azerite.twisted_claws.ok() )
      p() -> buff.twisted_claws -> trigger();

    p() -> buff.scent_of_blood -> trigger();

  }

  void execute() override
  {
    // Remove potential existing scent of blood buff here
    p()->buff.scent_of_blood->expire();

    cat_attack_t::execute();
  }
};

} // end namespace cat_attacks

namespace bear_attacks {

// ==========================================================================
// Druid Bear Attack
// ==========================================================================

struct bear_attack_t : public druid_attack_t<melee_attack_t>
{
  bool gore;

  bear_attack_t( const std::string& n, druid_t* p,
                 const spell_data_t* s = spell_data_t::nil(),
                 const std::string& options_str = std::string() ) :
    base_t( n, p, s ), gore( false )
  {
    parse_options( options_str );

    if (p->specialization() == DRUID_BALANCE || p->specialization() == DRUID_RESTORATION)
      ap_type = AP_NO_WEAPON;

    // Apply Guardian Druid aura damage modifiers
    if (p -> specialization() == DRUID_GUARDIAN)
    {
      // direct damage
      if ( s -> affected_by( p -> spec.guardian -> effectN( 1 ) ) )
      {
        base_dd_multiplier *= 1.0 + p -> spec.guardian -> effectN( 1 ).percent();
      }

      // ticking damage
      if ( s -> affected_by( p -> spec.guardian -> effectN( 2 ) ) )
      {
        base_td_multiplier *= 1.0 + p -> spec.guardian -> effectN( 2 ).percent();
      }
    }
  }

  virtual void execute() override
  {
    base_t::execute();

    if ( hit_any_target && gore )
      trigger_gore();
  }
}; // end bear_attack_t

// Bear Melee Attack ========================================================

struct bear_melee_t : public bear_attack_t
{
  bear_melee_t( druid_t* player ) :
    bear_attack_t( "bear_melee", player )
  {
    form_mask = BEAR_FORM;

    school            = SCHOOL_PHYSICAL;
    may_glance        = background = repeating = true;
    trigger_gcd       = timespan_t::zero();
    special           = false;
    weapon_multiplier = 1.0;

    energize_type     = ENERGIZE_ON_HIT;
    energize_resource = RESOURCE_RAGE;
    energize_amount   = 4;
  }

  virtual timespan_t execute_time() const override
  {
    if ( ! player -> in_combat )
      return timespan_t::from_seconds( 0.01 );

    return bear_attack_t::execute_time();
  }
};

// Growl ====================================================================

struct growl_t: public bear_attack_t
{
  growl_t( druid_t* player, const std::string& options_str ):
    bear_attack_t( "growl", player, player -> find_class_spell( "Growl" ), options_str )
  {
    ignore_false_positive = true;
    may_miss = may_parry = may_dodge = may_block = may_crit = false;
    use_off_gcd = true;
  }

  void update_ready( timespan_t ) override
  {
    timespan_t cd = cooldown -> duration;

    if ( p() -> buff.incarnation_bear -> up() )
      cd = timespan_t::zero();

    bear_attack_t::update_ready( cd );
  }

  void impact( action_state_t* s ) override
  {
    if ( s -> target -> is_enemy() )
      target -> taunt( player );

    bear_attack_t::impact( s );
  }
};

// Mangle ===================================================================

struct mangle_t : public bear_attack_t
{
  double bleeding_multiplier;

  mangle_t( druid_t* player, const std::string& options_str ) :
    bear_attack_t( "mangle", player, player -> find_class_spell( "Mangle" ), options_str )
  {
    bleeding_multiplier = data().effectN( 3 ).percent();

    if ( p() -> specialization() == DRUID_GUARDIAN )
    {
      energize_amount += player -> talent.soul_of_the_forest -> effectN( 1 ).resource( RESOURCE_RAGE );
      base_multiplier *= 1.0 + player -> talent.soul_of_the_forest -> effectN( 2 ).percent();
    }
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double em = bear_attack_t::composite_energize_amount( s );

    em += p() -> buff.gore -> value();

    return em;
  }

  void update_ready( timespan_t ) override
  {
    timespan_t cd = cooldown -> duration;

    if ( p() -> buff.incarnation_bear -> up() )
      cd = timespan_t::zero();

    bear_attack_t::update_ready( cd );
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double tm = bear_attack_t::composite_target_multiplier( t );

    if ( t -> debuffs.bleeding && t -> debuffs.bleeding -> check() )
      tm *= 1.0 + bleeding_multiplier;

    return tm;
  }

  int n_targets() const override
  {
    if ( p() -> buff.incarnation_bear -> up() )
      return (int) p() -> buff.incarnation_bear -> data().effectN( 4 ).base_value();

    return bear_attack_t::n_targets();
  }

  void impact( action_state_t* s ) override
  {
    bear_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
      p() -> buff.guardian_of_elune -> trigger();
  }

  void execute() override
  {
    bear_attack_t::execute();

    if ( p() -> buff.gore -> up() ) {
      if ( p() -> sets -> has_set_bonus( DRUID_GUARDIAN, T21, B2 ) ) {
        auto reduction = p() -> sets -> set( DRUID_GUARDIAN, T21, B2 ) -> effectN( 1 ).time_value();
        p() -> cooldown.barkskin -> adjust( - reduction );
      }

      p() -> buff.gore -> expire();

      if ( p() -> azerite.burst_of_savagery.ok() ) {
        p() -> buff.burst_of_savagery -> trigger();
        trigger_gore();
      }
    }

    p() -> buff.guardian_tier19_4pc -> trigger();
  }
};

// Maul =====================================================================

struct maul_t : public bear_attack_t
{
  maul_t( druid_t* player, const std::string& options_str ) :
    bear_attack_t( "maul", player, player -> find_specialization_spell( "Maul" ), options_str )
  {
    gore = true;
  }

  virtual void update_ready( timespan_t ) override
  {
    timespan_t cd = cooldown -> duration;

    if ( p() -> buff.incarnation_bear -> up() )
      cd = timespan_t::zero();

    bear_attack_t::update_ready( cd );
  }

  virtual void impact( action_state_t* s ) override
  {
    bear_attack_t::impact( s );

    if ( result_is_hit( s -> result ) && p() -> azerite.guardians_wrath.ok() )
    {
      p() -> buff.guardians_wrath -> up(); // benefit tracking
      p() -> buff.guardians_wrath -> trigger();
    }
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double da = bear_attack_t::bonus_da( s );

    if ( p() -> azerite.guardians_wrath.ok() )
    {
      da += p() -> azerite.guardians_wrath.value( 2 );
    }

    return da;
  }
};

// Pulverize ================================================================

struct pulverize_t : public bear_attack_t
{
  pulverize_t( druid_t* player, const std::string& options_str ) :
    bear_attack_t( "pulverize", player, player -> talent.pulverize, options_str )
  {}

  virtual void impact( action_state_t* s ) override
  {
    bear_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      // consumes x stacks of Thrash on the target
      td( s -> target ) -> dots.thrash_bear -> decrement( (int) data().effectN( 3 ).base_value() );

      // and reduce damage taken by x% for y sec.
      p() -> buff.pulverize -> trigger();
    }
  }

  bool target_ready( player_t* candidate_target ) override
  {
    // Call bear_attack_t::ready() first for proper targeting support.
    // Hardcode stack max since it may not be set if this code runs before Thrash is cast.
    return bear_attack_t::target_ready( candidate_target ) &&
      td( candidate_target ) -> dots.thrash_bear -> current_stack() >= data().effectN( 3 ).base_value();
  }
};

// Swipe (Bear) =============================================================

struct swipe_bear_t : public bear_attack_t
{
  swipe_bear_t( druid_t* p, const std::string& options_str ) :
    bear_attack_t( "swipe_bear", p, p -> find_spell( 213771 ), options_str )
  {
    aoe = -1;
    gore = true;
  }

  void update_ready( timespan_t ) override
  {
    timespan_t cd = cooldown -> duration;

    if ( p() -> buff.incarnation_bear -> up() )
      cd = timespan_t::zero();

    bear_attack_t::update_ready( cd );
  }

  virtual double bonus_da( const action_state_t* s ) const override
  {
    double b = bear_attack_t::bonus_da( s );

    if ( td( s->target )->dots.thrash_cat->is_ticking() ||
         td( s->target )->dots.thrash_bear->is_ticking() )
    {
      b += p()->azerite.wild_fleshrending.value( 2 );
    }

    return b;
  }
};

// Thrash (Bear) ============================================================

struct thrash_bear_t : public bear_attack_t
{
  double blood_frenzy_amount;

  thrash_bear_t( druid_t* p, const std::string& options_str ) :
    bear_attack_t( "thrash_bear", p, p -> find_spell( 77758 ), options_str ),
    blood_frenzy_amount( 0 )
  {
    aoe = -1;
    spell_power_mod.direct = 0;
    gore = true;
    dot_duration = p -> spec.thrash_bear_dot -> duration();
    base_tick_time = p -> spec.thrash_bear_dot -> effectN( 1 ).period();
    hasted_ticks = true;
    attack_power_mod.tick = p -> spec.thrash_bear_dot -> effectN( 1 ).ap_coeff();
    dot_max_stack = 3;
    // Apply hidden passive damage multiplier
    base_dd_multiplier *= 1.0 + p -> spec.guardian_overrides -> effectN( 6 ).percent();
    // Bear Form cost modifier
    base_costs[ RESOURCE_RAGE ] *= 1.0 + p -> buff.bear_form -> data().effectN( 7 ).percent();

    if ( p -> talent.blood_frenzy -> ok() )
      blood_frenzy_amount = p -> find_spell( 203961 ) -> effectN( 1 ).resource( RESOURCE_RAGE );
  }

  void update_ready( timespan_t ) override
  {
    timespan_t cd = cooldown -> duration;

    if ( p() -> buff.incarnation_bear -> up() )
      cd = timespan_t::zero();

    bear_attack_t::update_ready( cd );
  }

  virtual void tick( dot_t* d ) override
  {
    bear_attack_t::tick( d );

    p() -> resource_gain( RESOURCE_RAGE, blood_frenzy_amount, p() -> gain.blood_frenzy );
  }

  virtual void impact( action_state_t* state ) override
  {
    bear_attack_t::impact( state );

    if ( p() -> azerite.twisted_claws.ok() )
      p() -> buff.twisted_claws -> trigger();
  }
};

} // end namespace bear_attacks

namespace heals {

// ==========================================================================
// Druid Heal
// ==========================================================================

// Cenarion Ward ============================================================

struct cenarion_ward_hot_t : public druid_heal_t
{
  cenarion_ward_hot_t( druid_t* p ) :
    druid_heal_t( "cenarion_ward_hot", p, p -> find_spell( 102352 ) )
  {
    target = p;
    background = proc = true;
    harmful = hasted_ticks = false;
  }

  virtual void execute() override
  {
    heal_t::execute();

    p() -> buff.cenarion_ward -> expire();
  }
};

struct cenarion_ward_t : public druid_heal_t
{
  cenarion_ward_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "cenarion_ward", p, p -> talent.cenarion_ward,  options_str )
  {}

  virtual void execute() override
  {
    druid_heal_t::execute();

    p() -> buff.cenarion_ward -> trigger();
  }

  virtual bool target_ready( player_t* candidate_target ) override
  {
    if ( candidate_target != p() )
    {
      assert( false && "Cenarion Ward will not trigger on other players!" );
      return false;
    }

    return druid_heal_t::target_ready( candidate_target );
  }
};

// Frenzied Regeneration ====================================================

struct frenzied_regeneration_t : public heals::druid_heal_t
{
  heal_t* skysecs_hold;

  frenzied_regeneration_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "frenzied_regeneration", p,
      p -> find_affinity_spell( "Frenzied Regeneration" ), options_str ),
      skysecs_hold( nullptr )
  {
    /* use_off_gcd = quiet = true; */
    may_crit = tick_may_crit = false;
    target = p;
    cooldown -> hasted = true;
    hasted_ticks = false;
    tick_zero = true;

    if ( p -> specialization() == DRUID_GUARDIAN )
      cooldown -> charges += (int) p -> spec.frenzied_regeneration_2 -> effectN( 1 ).base_value();
  }

  void init() override
  {
    druid_heal_t::init();

    snapshot_flags = STATE_MUL_TA | STATE_VERSATILITY | STATE_MUL_PERSISTENT | STATE_TGT_MUL_TA;
  }

  void execute() override
  {
    druid_heal_t::execute();

    if ( skysecs_hold )
      skysecs_hold -> schedule_execute();

    if ( p() -> buff.guardian_of_elune -> up() )
      p() -> buff.guardian_of_elune -> expire();

    if ( p() -> buff.guardian_tier19_4pc -> up() )
      p() -> buff.guardian_tier19_4pc -> expire();
  }

  double action_multiplier() const override
  {
    double am = druid_heal_t::action_multiplier();

    am *= 1.0 + p() -> buff.guardian_of_elune -> check()
      * p() -> buff.guardian_of_elune -> data().effectN( 2 ).percent();

    return am;
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

    ctm *= td( target ) -> buff.lifebloom -> check();

    return ctm;
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
      td( t ) -> buff.lifebloom -> expire();
    }

    druid_heal_t::impact( state );

    if ( result_is_hit( state -> result ) )
      td( state -> target ) -> buff.lifebloom -> trigger();
  }

  virtual void last_tick( dot_t* d ) override
  {
    if ( ! d -> state -> target -> is_sleeping() ) // Prevent crash at end of simulation
      bloom -> execute();
    td( d -> state -> target ) -> buff.lifebloom -> expire();

    druid_heal_t::last_tick( d );
  }

  virtual void tick( dot_t* d ) override
  {
    druid_heal_t::tick( d );

    trigger_clearcasting();
  }
};

// Nature's Guardian ========================================================

struct natures_guardian_t : public druid_heal_t
{
  natures_guardian_t( druid_t* p ) :
    druid_heal_t( "natures_guardian", p, p -> find_spell( 227034 ) )
  {
    background = true;
    may_crit = false;
    target = p;
  }

  void init() override
  {
    druid_heal_t::init();

    // Not affected by multipliers of any sort.
    snapshot_flags &= ~STATE_NO_MULTIPLIER;
  }
};

// Regrowth =================================================================

struct regrowth_t: public druid_heal_t
{
  regrowth_t( druid_t* p, const std::string& options_str ):
    druid_heal_t( "regrowth", p, p -> find_class_spell( "Regrowth" ), options_str )
  {
    form_mask = NO_FORM | MOONKIN_FORM;
    may_autounshift = true;
    ignore_false_positive = true; // Prevents cat/bear from failing a skill check and going into caster form.
    
    // Spec passive modifiers. Note: Resto also has a modifier, but as a part
    // of a "spec-wide" modifier so better to account for that somewhere else.
    //base_dd_multiplier *= 1.0 + p->spec.feral->effectN( 5 ).percent() + p->spec.balance->effectN( 6 ).percent() +
    //                      p->spec.guardian->effectN( 4 ).percent();

    //base_td_multiplier *= 1.0 + p->spec.feral->effectN( 6 ).percent() + p->spec.balance->effectN( 7 ).percent() +
    //                      p->spec.guardian->effectN( 5 ).percent();

    //base_costs[ RESOURCE_MANA ] *= 1.0 + p->spec.feral->effectN( 7 ).percent() +
    //                               p->spec.balance->effectN( 8 ).percent() + p->spec.guardian->effectN( 6 ).percent();

    const spell_data_t* spelldata = p->find_class_spell("Regrowth");

    base_dd_multiplier *=
        1.0 + p->query_aura_effect( p->spec.feral, E_APPLY_AURA, A_ADD_PCT_MODIFIER, 0, spelldata )->percent() +
        p->query_aura_effect( p->spec.balance, E_APPLY_AURA, A_ADD_PCT_MODIFIER, 0, spelldata )->percent() +
        p->query_aura_effect( p->spec.guardian, E_APPLY_AURA, A_ADD_PCT_MODIFIER, 0, spelldata )->percent();

    base_td_multiplier *=
        1.0 + p->query_aura_effect( p->spec.feral, E_APPLY_AURA, A_ADD_PCT_MODIFIER, 22, spelldata )->percent() +
        p->query_aura_effect( p->spec.balance, E_APPLY_AURA, A_ADD_PCT_MODIFIER, 22, spelldata )->percent() +
        p->query_aura_effect( p->spec.guardian, E_APPLY_AURA, A_ADD_PCT_MODIFIER, 22, spelldata )->percent();

    base_costs[ RESOURCE_MANA ] *=
        1.0 + p->query_aura_effect( p->spec.feral, E_APPLY_AURA, A_ADD_PCT_MODIFIER, 14, spelldata )->percent() +
        p->query_aura_effect( p->spec.balance, E_APPLY_AURA, A_ADD_PCT_MODIFIER, 14, spelldata )->percent() +
        p->query_aura_effect( p->spec.guardian, E_APPLY_AURA, A_ADD_PCT_MODIFIER, 14, spelldata )->percent();

    // Hack for feral to be able to use target-related expressions on this action
    // Disables healing entirely
    if ( p -> specialization() == DRUID_FERAL ) {
      target = sim -> target;
      base_multiplier = 0;
    }
  }

  double cost() const override
  {
    if ( p() -> buff.predatory_swiftness -> check() )
      return 0;

    return druid_heal_t::cost();
  }

  void consume_resource() override
  {
    // Prevent from consuming Omen of Clarity unnecessarily
    if ( p() -> buff.predatory_swiftness -> check() )
      return;

    druid_heal_t::consume_resource();
  }

  timespan_t execute_time() const override
  {
    if ( p() -> buff.predatory_swiftness -> check() )
      return timespan_t::zero();

    timespan_t et = druid_heal_t::execute_time();

    et *= 1.0 + p() -> buff.power_of_elune -> current_stack
      * p() -> buff.power_of_elune -> data().effectN( 2 ).percent();

    return et;
  }

  double action_multiplier() const override
  {
    double am = druid_heal_t::action_multiplier();

    am *= 1.0 + p() -> buff.power_of_elune -> current_stack
      * p() -> buff.power_of_elune -> data().effectN( 1 ).percent();

    return am;
  }

  bool check_form_restriction() override
  {
    if ( p() -> buff.predatory_swiftness -> check() )
      return true;

    return druid_heal_t::check_form_restriction();
  }

  void execute() override
  {
    druid_heal_t::execute();

    if ( p() -> talent.bloodtalons -> ok() )
      p() -> buff.bloodtalons -> trigger( 2 );

    p() -> buff.predatory_swiftness -> decrement( 1 );

    if ( p() -> buff.power_of_elune -> up() )
      p() -> buff.power_of_elune -> expire();
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
};

// Renewal ==================================================================

struct renewal_t : public druid_heal_t
{
  renewal_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "renewal", p, p -> find_talent_spell( "Renewal" ), options_str )
  {
    may_crit = false;
  }

  virtual void execute() override
  {
    base_dd_min = p() -> resources.max[ RESOURCE_HEALTH ] * data().effectN( 1 ).percent();

    druid_heal_t::execute();
  }
};

// Swiftmend ================================================================

// TODO: in game, you can swiftmend other druids' hots, which is not supported here
struct swiftmend_t : public druid_heal_t
{
  swiftmend_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "swiftmend", p, p -> find_class_spell( "Swiftmend" ), options_str )
  {}

  virtual void impact( action_state_t* state ) override
  {
    druid_heal_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      if ( p() -> talent.soul_of_the_forest -> ok() )
        p() -> buff.soul_of_the_forest -> trigger();
    }
  }
};

// Tranquility ==============================================================

struct tranquility_t : public druid_heal_t
{
  tranquility_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "tranquility", p, p -> find_specialization_spell( "Tranquility" ), options_str )
  {
    aoe               = -1;
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
    ignore_false_positive = true;
  }

  int n_targets() const override
  {
    int n = druid_heal_t::n_targets();

    if ( p() -> buff.incarnation_tree -> check() )
      n += 2;

    return n;
  }
};

// Ysera's Gift =============================================================

struct yseras_gift_t : public druid_heal_t
{
  yseras_gift_t( druid_t* p ) :
    druid_heal_t( "yseras_gift", p, p -> find_spell( 145110 ) )
  {
    may_crit = false;
    background = dual = true;
    base_pct_heal = p -> spec.yseras_gift -> effectN( 1 ).percent();
  }

  void init() override
  {
    druid_heal_t::init();

    snapshot_flags &= ~STATE_VERSATILITY; // Is not affected by versatility.
  }

  virtual double action_multiplier() const override
  {
    double am = druid_heal_t::action_multiplier();

    if ( p() -> buff.bear_form -> check() )
      am *= 1.0 + p() -> buff.bear_form -> data().effectN( 8 ).percent();

    return am;
  }

  virtual void execute() override
  {
    if ( p() -> health_percentage() < 100 )
      target = p();
    else
      target = smart_target();

    druid_heal_t::execute();
  }
};

} // end namespace heals

namespace spells {

// ==========================================================================
// Druid Spells
// ==========================================================================

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

    return ( player -> main_hand_attack -> execute_event == nullptr ); // not swinging
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

    cooldown -> duration *= 1.0 + player -> spec.guardian -> effectN( 3 ).percent();
    cooldown -> duration *= 1.0 + player -> talent.survival_of_the_fittest -> effectN( 1 ).percent();
    dot_duration = timespan_t::zero();

    if ( player -> talent.brambles -> ok() )
      add_child( player -> active.brambles_pulse );
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.barkskin -> trigger();
    p() -> buff.oakhearts_puny_quods -> trigger();
  }
};

// Bear Form Spell ==========================================================

struct bear_form_t : public druid_spell_t
{
  bear_form_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "bear_form", player, player -> find_class_spell( "Bear Form" ), options_str )
  {
    form_mask = NO_FORM | CAT_FORM | MOONKIN_FORM;
    may_autounshift = false;

    harmful = false;
    min_gcd = timespan_t::from_seconds( 1.5 );
    ignore_false_positive = true;

    if ( ! player -> bear_melee_attack )
    {
      player -> init_beast_weapon( player -> bear_weapon, 2.5 );
      player -> bear_melee_attack = new bear_attacks::bear_melee_t( player );
    }
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> shapeshift( BEAR_FORM );
  }
};
// Brambles =================================================================

struct brambles_t : public druid_spell_t
{
  brambles_t( druid_t* p ) :
    druid_spell_t( "brambles_reflect", p, p -> find_spell( 203958 ) )
  {
    // Ensure reflect scales with damage multipliers
    snapshot_flags |= STATE_VERSATILITY | STATE_TGT_MUL_DA | STATE_MUL_DA;
    background = may_crit = proc = may_miss = true;
  }
};

struct brambles_pulse_t : public druid_spell_t
{
  brambles_pulse_t( druid_t* p ) :
    druid_spell_t( "brambles_pulse", p, p -> find_spell( 213709 ) )
  {
    background = dual = true;
    aoe = -1;
  }
};

// Bristling Fur Spell ======================================================

struct bristling_fur_t : public druid_spell_t
{
  bristling_fur_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "bristling_fur", player, player -> talent.bristling_fur, options_str  )
  {
    harmful = false;
    use_off_gcd = true;
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
    druid_spell_t( "cat_form", player, player -> find_class_spell( "Cat Form" ), options_str )
  {
    form_mask = NO_FORM | BEAR_FORM | MOONKIN_FORM;
    may_autounshift = false;

    harmful = false;
    min_gcd = timespan_t::from_seconds( 1.5 );
    ignore_false_positive = true;

    if ( ! player -> cat_melee_attack )
    {
      player -> init_beast_weapon( player -> cat_weapon, 1.0 );
      player -> cat_melee_attack = new cat_attacks::cat_melee_t( player );
    }
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> shapeshift( CAT_FORM );
  }
};


// Celestial Alignment ======================================================

struct celestial_alignment_t : public druid_spell_t
{
  bool precombat;

  celestial_alignment_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "celestial_alignment", player, player -> spec.celestial_alignment , options_str ),
    precombat()
  {
    harmful = false;
  }

  void init_finished() override
  {
    druid_spell_t::init_finished();

    if ( action_list -> name_str == "precombat" )
      precombat = true;
  }

  void schedule_execute( action_state_t* s ) override
  {
    // buff applied first to reduce GCD
    if ( !precombat )
      p() -> buff.celestial_alignment -> trigger();

    druid_spell_t::schedule_execute( s );
  }

  void execute() override
  {
    druid_spell_t::execute();

    // Precombat actions skip schedule_execute, so the buff needs to be
    // triggered here for precombat actions.
    if ( precombat )
      p() -> buff.celestial_alignment -> trigger();
    
    //Trigger after triggering the buff so the cast procs the spell
    streaking_stars_trigger(SS_CELESTIAL_ALIGNMENT, nullptr);
  }

  virtual bool ready() override
  {
    if ( p() -> talent.incarnation_moonkin -> ok() )
      return false;

    return druid_spell_t::ready();
  }
};

// Streaking Stars =======================================================

struct streaking_stars_t : public druid_spell_t
{
  streaking_stars_t(druid_t* p) :
    //druid_spell_t("streaking_stars", p, p->azerite.streaking_stars.spell()->effectN(1).trigger()->effectN(1).trigger())
    druid_spell_t("streaking_stars", p, p->find_spell(272873)) // blizz decided to remove all references to this spell, so hardcoded by ID now
  {
    background = 1;
    base_dd_min = base_dd_max = p->azerite.streaking_stars.value(1);
    if (p->talent.incarnation_moonkin->ok())
    {
      //This spell deals less damage when incarnation is talented which is not found in the spelldata 8/10/18
      base_dd_min *= 0.6667;
      base_dd_max *= 0.6667;
    }
  }
};

// Fury of Elune =========================================================

struct fury_of_elune_t : public druid_spell_t
{
  struct fury_of_elune_tick_t : public druid_spell_t
  {
    fury_of_elune_tick_t(const std::string& n, druid_t* p, const spell_data_t* s) :
        druid_spell_t(n, p, s)
    {
      background = dual = ground_aoe = true;
      aoe = -1;
    }
  };

  fury_of_elune_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "fury_of_elune", p, p -> talent.fury_of_elune, options_str )
  {
      if (!p->active.fury_of_elune)
      {
          p->active.fury_of_elune = new fury_of_elune_tick_t("fury_of_elune_tick", p, p->find_spell(211545));
          p->active.fury_of_elune->stats = stats;
      }
  }

  void execute() override
  {
      druid_spell_t::execute();

      streaking_stars_trigger(SS_FURY_OF_ELUNE, nullptr);

      timespan_t pulse_time = data().effectN(3).period();
      make_event<ground_aoe_event_t>(*sim, p(), ground_aoe_params_t()
          .target(execute_state->target)
          .pulse_time(pulse_time * p()->composite_spell_haste()) //tick time is saved in an effect
          .duration(data().duration())
          .action(p()->active.fury_of_elune));
      p() -> buff.fury_of_elune -> trigger();
  }
};

// Dash =====================================================================

struct dash_t : public druid_spell_t
{
  dash_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t("dash", player,
      player->talent.tiger_dash->ok() ? player->talent.tiger_dash : player->find_class_spell("Dash"),
      options_str )
  {
    autoshift = form_mask = CAT_FORM;

    harmful = may_crit = may_miss = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    if (p()->talent.tiger_dash->ok())
      p()->buff.tiger_dash->trigger();
    else
      p()->buff.dash->trigger();
  }
};

// Tiger Dash =====================================================================

struct tiger_dash_t : public druid_spell_t
{
  tiger_dash_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "tiger_dash", player, player -> talent.tiger_dash, options_str )
  {
    autoshift = form_mask = CAT_FORM;

    harmful = may_crit = may_miss = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.tiger_dash -> trigger();
  }
};

// Full Moon Spell ==========================================================

struct full_moon_t : public druid_spell_t
{
  bool radiant_moonlight;
  full_moon_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "full_moon", player, player -> spec.full_moon, options_str ), radiant_moonlight(false)
  {
    aoe = -1;
    split_aoe_damage = true;
    cooldown = player -> cooldown.moon_cd;
  }

  double calculate_direct_amount( action_state_t* state ) const override
  {
    double da = druid_spell_t::calculate_direct_amount( state );

    // Sep 3 2016: Main target takes full damage, this reverses the divisor in action_t::calculate_direct_amount (hotfixed).
    if ( state -> chain_target == 0 )
    {
      da *= state -> n_targets;
    }

    return da;
  }

  void impact(action_state_t* s) override
  {
    druid_spell_t::impact(s);
    streaking_stars_trigger(SS_FULL_MOON, s); // proc munching shenanigans, munch tracking NYI
  }

  void execute() override
  {
    druid_spell_t::execute();
    
    if (p()->moon_stage == FULL_MOON && radiant_moonlight) {
      p()->moon_stage = FREE_FULL_MOON;
      p()->cooldown.moon_cd->reset(true);
    }
    else {
      p()->moon_stage = NEW_MOON; //Requires hit
    }
  }

  timespan_t travel_time() const override
  {
      return timespan_t::from_millis(900); //this has a set travel time since it spawns on the target
  }

  bool ready() override
  {
    if ( ! p() -> talent.new_moon )
      return false;
    if ( p() -> moon_stage != FULL_MOON && p()->moon_stage != FREE_FULL_MOON)
      return false;
    if ( ! p() -> cooldown.moon_cd -> up() )
      return false;

    return druid_spell_t::ready();
  }
};

// Half Moon Spell ==========================================================

struct half_moon_t : public druid_spell_t
{
  half_moon_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "half_moon", player, player -> spec.half_moon, options_str )
  {
    cooldown = player -> cooldown.moon_cd;
  }

  void impact(action_state_t* s) override
  {
    druid_spell_t::impact(s);
    streaking_stars_trigger(SS_HALF_MOON, s); // proc munching shenanigans, munch tracking NYI
  }

  void execute() override
  {
    druid_spell_t::execute();
    p() -> moon_stage++;
  }

  bool ready() override
  {
    if ( ! p() -> talent.new_moon )
      return false;
    if ( p() -> moon_stage != HALF_MOON )
      return false;
    if ( ! p() -> cooldown.moon_cd -> up() )
      return false;

    return druid_spell_t::ready();
  }
};

// Thrash ===================================================================

struct thrash_proxy_t : public druid_spell_t
{
  action_t* thrash_cat;
  action_t* thrash_bear;

  thrash_proxy_t( druid_t* p, const std::string& options_str )
    : druid_spell_t( "thrash", p, p->find_spell( 106832 ), options_str )
  {
    thrash_cat  = new cat_attacks::thrash_cat_t( p, options_str );
    thrash_bear = new bear_attacks::thrash_bear_t( p, options_str );

    // At the moment, both versions of these spells are the same
    //(and only the cat version has a talent that changes this)
    // so we use this spell data here. If this changes we will
    // have to think of something more robust. These two are
    // important for the "ticks_gained_on_refresh" expression to work
    dot_duration   = thrash_cat->dot_duration;
    base_tick_time = thrash_cat->base_tick_time;
  }

  timespan_t gcd() const override
  {
    if ( p()->buff.cat_form->check() )
      return thrash_cat->gcd();
    else
      return thrash_bear->gcd();
  }

  void execute() override
  {
    if ( p()->buff.cat_form->check() )
      thrash_cat->execute();

    else if ( p()->buff.bear_form->check() )
      thrash_bear->execute();
  }

  bool action_ready() override
  {
    if ( p()->buff.cat_form->check() )
      return thrash_cat->action_ready();

    else if ( p()->buff.bear_form->check() )
      return thrash_bear->action_ready();

    return false;
  }

  dot_t* get_dot( player_t* t ) override
  {
    if ( p()->buff.bear_form->check() )
      return thrash_bear->get_dot( t );
    return thrash_cat->get_dot( t );
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( p()->buff.cat_form->check() )
      return thrash_cat->target_ready( candidate_target );

    else if ( p()->buff.bear_form->check() )
      return thrash_bear->target_ready( candidate_target );

    return false;
  }

  bool ready() override
  {
    if ( p()->buff.cat_form->check() )
      return thrash_cat->ready();

    else if ( p()->buff.bear_form->check() )
      return thrash_bear->ready();

    return false;
  }

  double cost() const override
  {
    if ( p()->buff.cat_form->check() )
      return thrash_cat->cost();

    else if ( p()->buff.bear_form->check() )
      return thrash_bear->cost();

    return 0;
  }
};

struct swipe_proxy_t : public druid_spell_t
{
  action_t* swipe_cat;
  action_t* swipe_bear;

  swipe_proxy_t(druid_t* p, const std::string& options_str) : druid_spell_t("swipe", p, spell_data_t::nil(), options_str)
  {
    swipe_cat = new cat_attacks::swipe_cat_t(p, options_str);
    swipe_bear = new bear_attacks::swipe_bear_t(p, options_str);
  }

  void execute() override
  {
    if (p()->buff.cat_form->check())
      swipe_cat->execute();

    else if (p()->buff.bear_form->check())
      swipe_bear->execute();

  }

  bool action_ready() override
  {
    if (p()->buff.cat_form->check())
      return swipe_cat->action_ready();

    else if (p()->buff.bear_form->check())
      return swipe_bear->action_ready();

    return false;
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if (p()->buff.cat_form->check())
      return swipe_cat->target_ready( candidate_target );

    else if (p()->buff.bear_form->check())
      return swipe_bear->target_ready( candidate_target );

    return false;
  }

  bool ready() override
  {
    if (p()->buff.cat_form->check())
      return swipe_cat->ready();

    else if (p()->buff.bear_form->check())
      return swipe_bear->ready();

    return false;
  }

  double cost() const override
  {
    if (p()->buff.cat_form->check())
      return swipe_cat->cost();

    else if (p()->buff.bear_form->check())
      return swipe_bear->cost();

    return 0;
  }


};

// Incarnation ==============================================================

struct incarnation_t : public druid_spell_t
{
  buff_t* spec_buff;
  bool precombat;

  incarnation_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "incarnation", p,
                   p -> specialization() == DRUID_BALANCE     ? p -> talent.incarnation_moonkin :
                   p -> specialization() == DRUID_FERAL       ? p -> talent.incarnation_cat     :
                   p -> specialization() == DRUID_GUARDIAN    ? p -> talent.incarnation_bear    :
                   p -> specialization() == DRUID_RESTORATION ? p -> talent.incarnation_tree    :
                   spell_data_t::nil(), options_str ),
    precombat()
  {
    switch ( p -> specialization() )
    {
    case DRUID_BALANCE:
      spec_buff = p -> buff.incarnation_moonkin;
      break;
    case DRUID_FERAL:
      spec_buff = p -> buff.incarnation_cat;
      break;
    case DRUID_GUARDIAN:
      spec_buff = p -> buff.incarnation_bear;
      break;
    case DRUID_RESTORATION:
      spec_buff = p -> buff.incarnation_tree;
      break;
    default:
      {
        assert( false && "Actor attempted to create incarnation action with no specialization." );
      }
    }

    harmful = false;
  }

  void init_finished() override
  {
    druid_spell_t::init_finished();

    if ( action_list -> name_str == "precombat" )
      precombat = true;
  }
  
  void schedule_execute( action_state_t* s ) override
  {
    // buff applied first to reduce GCD for moonkin
    if ( !precombat )
      spec_buff -> trigger();

    druid_spell_t::schedule_execute( s );
  }

  void execute() override
  {
    druid_spell_t::execute();

    // Precombat actions skip schedule_execute, so the buff needs to be
    // triggered here for precombat actions.
    if ( precombat )
      spec_buff -> trigger();

    if ( p() -> buff.incarnation_cat -> check() )
    {
      p() -> buff.jungle_stalker -> trigger();
    }

    if (p()->buff.incarnation_moonkin->check())
    {
      streaking_stars_trigger(SS_CELESTIAL_ALIGNMENT, nullptr);
    }

    if ( ! p() -> in_combat )
    {
      timespan_t time = std::max( min_gcd, trigger_gcd * composite_haste() );

      spec_buff -> extend_duration( p(), -time );
      cooldown -> adjust( -time );

      // King of the Jungle raises energy cap, so manually trigger some regen so that the actor starts with the correct amount of energy.
      if ( p() -> buff.incarnation_cat -> check() )
        p() -> regen( time );
    }

    if ( p() -> buff.incarnation_bear -> check() )
    {
      p() -> cooldown.mangle      -> reset( false );
      p() -> cooldown.thrash_bear -> reset( false );
      p() -> cooldown.growl       -> reset( false );
      p() -> cooldown.maul        -> reset( false );

    }
  }
};

// Innervate ================================================================
struct innervate_t : public druid_spell_t
{
  innervate_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "innervate", p, p -> find_specialization_spell( "Innervate" ), options_str )
  {
    harmful = false;
  }
  virtual void execute() override
  {
    druid_spell_t::execute();

    p()->buff.innervate->trigger();
  }
};

// Ironfur ==================================================================

struct ironfur_t : public druid_spell_t
{
  ironfur_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "ironfur", p, p -> spec.ironfur, options_str )
  {
    use_off_gcd = true;
    harmful = may_miss = may_parry = may_dodge = may_crit = false;
  }

  timespan_t composite_buff_duration()
  {
    timespan_t bd = p() -> buff.ironfur -> buff_duration;

    bd += timespan_t::from_seconds( p() -> buff.guardian_of_elune -> value() );
    bd += timespan_t::from_seconds( p() -> buff.guardian_tier19_4pc -> check_stack_value() );

    return bd;
  }

  double cost() const override
  {
    double c = druid_spell_t::cost();

    c += p() -> buff.guardians_wrath -> check_stack_value();

    return c;
  }

  virtual void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.ironfur -> trigger( 1, buff_t::DEFAULT_VALUE(), -1, composite_buff_duration() );

    if ( p() -> buff.guardian_of_elune -> up() )
      p() -> buff.guardian_of_elune -> expire();

    if ( p() -> buff.guardian_tier19_4pc -> up() )
      p() -> buff.guardian_tier19_4pc -> expire();

    if ( p() -> buff.guardians_wrath -> up() )
      p() -> buff.guardians_wrath -> expire();

    if ( p() -> azerite.layered_mane.ok() && rng().roll( p() -> azerite.layered_mane.spell() -> effectN( 2 ).percent() ) ) {
      p() -> buff.ironfur -> trigger( 1, buff_t::DEFAULT_VALUE(), -1, composite_buff_duration() );
    }
  }
};

// Lunar Beam ===============================================================

struct lunar_beam_t : public druid_spell_t
{
  struct lunar_beam_heal_t : public heals::druid_heal_t
  {
    lunar_beam_heal_t( druid_t* p ) :
      heals::druid_heal_t( "lunar_beam_heal", p, p -> find_spell( 204069 ) )
    {
      target = p;
      attack_power_mod.direct = data().effectN( 1 ).ap_coeff();
      background = true;
    }
  };

  struct lunar_beam_damage_t : public druid_spell_t
  {
    action_t* heal;

    lunar_beam_damage_t( druid_t* p ) :
      druid_spell_t( "lunar_beam_damage", p, p -> find_spell( 204069 ) )
    {
      aoe = -1;
      dual = background = ground_aoe = true;
      attack_power_mod.direct = data().effectN( 2 ).ap_coeff();

      heal = new lunar_beam_heal_t( p );
    }

    void execute() override
    {
      druid_spell_t::execute();

      heal -> schedule_execute();
    }
  };

  action_t* damage;

  lunar_beam_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "lunar_beam", p, p -> talent.lunar_beam, options_str )
  {
    may_crit = tick_may_crit = may_miss = hasted_ticks = false;
    dot_duration = timespan_t::zero();
    base_tick_time = timespan_t::from_seconds( 1.0 ); // TODO: Find in spell data... somewhere!

    damage = p -> find_action( "lunar_beam_damage" );

    if ( ! damage )
    {
      damage = new lunar_beam_damage_t( p );
      damage -> stats = stats;
    }
  }

  void execute() override
  {
    druid_spell_t::execute();

    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
        .target( execute_state -> target )
        .x( execute_state -> target -> x_position )
        .y( execute_state -> target -> y_position )
        .pulse_time( base_tick_time )
        .duration( data().duration() )
        .start_time( sim -> current_time() )
        .action( damage ), false );
  }
};

// Lunar Strike =============================================================

struct lunar_strike_t : public druid_spell_t
{
  lunar_strike_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "lunar_strike", player,
      player -> talent.balance_affinity -> ok() ? player->find_spell(197628) : player -> find_affinity_spell("Lunar Strike"), options_str )
  {
    aoe = -1;
    base_aoe_multiplier = player->talent.balance_affinity->ok() ? data().effectN(2).percent() : data().effectN(3).percent();
  }

  double composite_crit_chance() const override
  {
    double cc = druid_spell_t::composite_crit_chance();

    if ( p() -> buff.lunar_empowerment -> check() )
      cc += p() -> spec.balance_tier19_2pc -> effectN( 1 ).percent();

    return cc;
  }

  double action_multiplier() const override
  {
    double am = druid_spell_t::action_multiplier();

    if ( p() -> buff.lunar_empowerment -> check() )
      am *= 1.0 + composite_lunar_empowerment();

    return am;
  }

  timespan_t gcd() const override
  {
    timespan_t g = druid_spell_t::gcd();

    if (p() -> buff.lunar_empowerment -> check())
      g *= 1 + p()->buff.lunar_empowerment->data().effectN(3).percent();

    g = std::max( min_gcd, g );

    return g;
  }

  timespan_t execute_time() const override
  {
    timespan_t et = druid_spell_t::execute_time();

    if (p() -> buff.lunar_empowerment -> check() )
      et *= 1 + p()->buff.lunar_empowerment->data().effectN(2).percent();

    return et;
  }

  void impact(action_state_t* s)
  {
    druid_spell_t::impact(s);
    streaking_stars_trigger(SS_LUNAR_STRIKE, s);
  }

  void execute() override
  {
    p() -> buff.lunar_empowerment -> up();

    druid_spell_t::execute();

    p() -> buff.lunar_empowerment -> decrement();

    if (player->specialization() == DRUID_BALANCE)
    {
        if (rng().roll(p()->spec.eclipse->effectN(1).percent()))
        {
            p()->buff.solar_empowerment->trigger();
        }
    }

    p() -> buff.power_of_elune -> trigger();

    if (p()->azerite.dawning_sun.ok())
    {
      p()->buff.dawning_sun->trigger(1, p()->azerite.dawning_sun.value());
    }

  }
};

// New Moon Spell ===========================================================

struct new_moon_t : public druid_spell_t
{
  new_moon_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "new_moon", player, player -> talent.new_moon, options_str )
  {
    cooldown = player -> cooldown.moon_cd;
    cooldown -> duration = data().charge_cooldown();
    cooldown -> charges  = data().charges();
  }

  void impact (action_state_t* s) override
  {
    druid_spell_t::impact(s);
    streaking_stars_trigger(SS_NEW_MOON, s); // proc munching shenanigans, munch tracking NYI
  }

  void execute() override
  {
    druid_spell_t::execute();
    p() -> moon_stage++;
  }

  bool ready() override
  {
    if ( ! p() ->talent.new_moon)
      return false;
    if ( p() -> moon_stage != NEW_MOON )
      return false;
    if ( ! p() -> cooldown.moon_cd -> up() )
      return false;

    return druid_spell_t::ready();
  }
};

// Sunfire Spell ============================================================

struct sunfire_t : public druid_spell_t
{

  struct sunfire_damage_t : public druid_spell_t
  {
    int sunfire_action_id;
    sunfire_damage_t( druid_t* p ) :
      druid_spell_t( "sunfire_dmg", p, p -> spec.sunfire_dmg ),
      sunfire_action_id(0)
    {
      if ( p -> talent.shooting_stars -> ok() && ! p -> active.shooting_stars )
      {
        p -> active.shooting_stars = new shooting_stars_t( p );
      }

      dual = background = true;
      aoe = -1;
      base_aoe_multiplier = 0;
      dot_duration += p -> spec.balance -> effectN( 4 ).time_value();

      if (p->azerite.high_noon.ok())
      {
        radius += p->azerite.high_noon.value();
      }

    }

    double action_multiplier() const override
    {
        double am = druid_spell_t::action_multiplier();

        am *= 1.0 + p()->buff.solar_solstice->check_value();

        if (p ()->mastery.starlight->ok ())
          am *= 1.0 + p ()->cache.mastery_value ();

        return am;
    }

    dot_t* get_dot( player_t* t ) override
    {
      if ( ! t ) t = target;
      if ( ! t ) return nullptr;

      return td( t ) -> dots.sunfire;
    }

    void tick( dot_t* d ) override
    {
      druid_spell_t::tick( d );
      double dr_mult = std::sqrt(p()->get_active_dots(internal_id)) / p()->get_active_dots(internal_id);

      if (p()->talent.shooting_stars->ok())
      {
        if (rng().roll(p()->talent.shooting_stars->effectN(1).percent()*dr_mult))
        {
          p()->active.shooting_stars->target = d->target;
          p()->active.shooting_stars->execute();
        }
      }
    }

    double bonus_da(const action_state_t* s) const override
    {
      double da = druid_spell_t::bonus_da(s);
      return da;
    }
    
    double bonus_ta(const action_state_t* s) const override
    {
      double ta = druid_spell_t::bonus_ta(s);

      ta += p()->azerite.high_noon.value(2);

      return ta;
    }

  };

  sunfire_damage_t* damage;

  sunfire_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "sunfire", player, player -> find_affinity_spell( "Sunfire" ), options_str )
  {
    may_miss = false;
    damage = new sunfire_damage_t( player );
    damage -> stats = stats;

    add_child(damage);

    if ( player -> spec.astral_power -> ok() )
    {
      energize_resource = RESOURCE_ASTRAL_POWER;
      energize_amount = player -> spec.astral_power -> effectN( 3 ).resource( RESOURCE_ASTRAL_POWER );
    }
    else
    {
      energize_type = ENERGIZE_NONE;
    }

    // Add damage modifiers in sunfire_damage_t, not here.
  }

  void impact(action_state_t* s) override
  {
    druid_spell_t::impact(s);
    streaking_stars_trigger(SS_SUNFIRE, s);
  }

  void execute() override
  {
    druid_spell_t::execute();
 
    damage -> target = execute_state -> target;
    damage -> schedule_execute();
  }
};

// Moonkin Form Spell =======================================================

struct moonkin_form_t : public druid_spell_t
{
  moonkin_form_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "moonkin_form", player, player -> find_affinity_spell( "Moonkin Form" ), options_str )
  {
    form_mask = NO_FORM | CAT_FORM | BEAR_FORM;
    may_autounshift = false;

    harmful = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> shapeshift( MOONKIN_FORM );
  }
};

// Moonkin Form Affinity Spell ===(Under development Jan 08 2017)============

struct moonkin_form_affinity_t : public druid_spell_t
{
  moonkin_form_affinity_t(druid_t* player, const std::string& options_str) :
    druid_spell_t("moonkin_form_affinity", player, player -> spec.moonkin_form_affinity, options_str)
  {
    form_mask = NO_FORM | CAT_FORM | BEAR_FORM;
    may_autounshift = false;

    harmful = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->shapeshift(MOONKIN_FORM_AFFINITY);
  }
};

// Prowl ====================================================================

struct prowl_t : public druid_spell_t
{
  prowl_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "prowl", player, player -> find_class_spell( "Prowl" ), options_str )
  {
    autoshift = form_mask = CAT_FORM;

    trigger_gcd = timespan_t::zero();
    use_off_gcd = true;
    harmful = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    if ( sim -> log )
      sim -> out_log.printf( "%s performs %s", player -> name(), name() );

    p() -> buff.jungle_stalker -> expire();
    p() -> buff.prowl -> trigger();
  }

  bool ready() override
  {
    if ( p() -> buff.prowl -> check() )
      return false;

    if ( p() -> in_combat && ! ( p() -> specialization() == DRUID_FERAL &&
      p() -> buff.jungle_stalker -> check() ) )
    {
      return false;
    }

    return druid_spell_t::ready();
  }
};

// Skull Bash ===============================================================

struct skull_bash_t : public druid_spell_t
{
  skull_bash_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "skull_bash", player, player -> find_specialization_spell( "Skull Bash" ), options_str )
  {
    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    if ( p() -> sets -> has_set_bonus( DRUID_FERAL, PVP, B2 ) )
      p() -> cooldown.tigers_fury -> reset( false );
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( ! candidate_target -> debuffs.casting || ! candidate_target -> debuffs.casting -> check() )
      return false;

    return druid_spell_t::target_ready( candidate_target );
  }
};

// Solar Wrath ==============================================================
struct solar_wrath_state_t :public action_state_t
{
  bool empowered;
  bool proc_streaking;
  solar_wrath_state_t (action_t* action, player_t* target) :
    action_state_t (action, target), empowered (false), proc_streaking(false)
  {}

  void initialize () override
  {
    action_state_t::initialize();
    empowered = false;
    proc_streaking = false;
  }

  std::ostringstream& debug_str (std::ostringstream& s) override
  {
    action_state_t::debug_str (s) << " empowered=" << empowered;
    return s;
  }

  void copy_state (const action_state_t* o) override
  {
    action_state_t::copy_state (o);
    const solar_wrath_state_t* st = debug_cast<const solar_wrath_state_t*>(o);
    empowered = st->empowered;
    proc_streaking = st->proc_streaking;
  }
};

struct solar_empowerment_t : public druid_spell_t
{
  solar_empowerment_t (druid_t* p) :
    druid_spell_t("solar_empowerment", p, p->spec.solar_wrath_empowerment)
  {
    background = true;
    may_crit = false;
    aoe = -1;
  }

  void init () override
  {
    druid_spell_t::init ();

    snapshot_flags = update_flags = 0;
  }
};

struct solar_wrath_t : public druid_spell_t
{
  solar_wrath_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "solar_wrath", player, player -> find_affinity_spell( "Solar Wrath" ), options_str )
  {
    form_mask = MOONKIN_FORM;

    if (player->specialization() == DRUID_RESTORATION)
      form_mask = NO_FORM | MOONKIN_FORM;

    add_child (player->active.solar_empowerment);

    energize_amount = player -> spec.astral_power -> effectN( 2 ).resource( RESOURCE_ASTRAL_POWER );
  }

  double composite_crit_chance() const override
  {
    double cc = druid_spell_t::composite_crit_chance();

    if ( p() -> buff.solar_empowerment -> check() )
      cc += p() -> spec.balance_tier19_2pc -> effectN( 1 ).percent();

    return cc;
  }

  action_state_t* new_state () override
  {
    return new solar_wrath_state_t (this, target);
  }

  void snapshot_state (action_state_t* state, dmg_e type) override
  {
    druid_spell_t::snapshot_state (state, type);

    if (p()->buff.solar_empowerment->check())
    {
      debug_cast<solar_wrath_state_t*>(state)->empowered = true;
    }
    if (streaking_stars_trigger(SS_SOLAR_WRATH, state, true, false))
    {
      debug_cast<solar_wrath_state_t*>(state)->proc_streaking = true;
    }
  }

  void impact (action_state_t* s) override
  {
    druid_spell_t::impact (s);

    solar_wrath_state_t* st = debug_cast<solar_wrath_state_t*>(s);
    if (st->empowered)
    {
      p ()->trigger_solar_empowerment (s);
    }
  }

  timespan_t gcd() const override
  {
    timespan_t g = druid_spell_t::gcd();

    if ( p() -> buff.solar_empowerment -> check() )
      g *= 1 + p()->buff.solar_empowerment->data().effectN(3).percent();

    g = std::max( min_gcd, g );

    return g;
  }

  timespan_t execute_time() const override
  {
    timespan_t et = druid_spell_t::execute_time();

    if (p() -> buff.solar_empowerment -> check() )
      et *= 1 + p()->buff.solar_empowerment->data().effectN(2).percent();

    return et;
  }

  void execute() override
  {
    p() -> buff.solar_empowerment -> up();

    druid_spell_t::execute();

    streaking_stars_trigger( SS_SOLAR_WRATH, execute_state );

    p() -> buff.solar_empowerment -> decrement();

    if (player->specialization() == DRUID_BALANCE)
    {
        if (rng().roll(p()->spec.eclipse->effectN(1).percent()))
        {
            p()->buff.lunar_empowerment->trigger();
            expansion::bfa::trigger_leyshocks_grand_compilation( STAT_VERSATILITY_RATING, player );
        }
    }

    p() -> buff.power_of_elune -> trigger();
  }

  virtual double bonus_da(const action_state_t* s) const override
  {
    double da = druid_spell_t::bonus_da(s);

    da += p()->buff.dawning_sun->value();

    return da;
  }
};

// Stampeding Roar ==========================================================

struct stampeding_roar_t : public druid_spell_t
{
  stampeding_roar_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "stampeding_roar", p, p -> find_class_spell( "Stampeding Roar" ), options_str )
  {
    form_mask = BEAR_FORM | CAT_FORM;
    autoshift = BEAR_FORM;

    harmful = false;
    radius *= 1.0 + p -> talent.gutteral_roars -> effectN( 1 ).percent();
    cooldown -> duration *= 1.0 + p -> talent.gutteral_roars -> effectN( 2 ).percent();
  }

  void execute() override
  {
    druid_spell_t::execute();

    for ( size_t i = 0; i < sim -> player_non_sleeping_list.size(); ++i )
    {
      player_t* p = sim -> player_non_sleeping_list[ i ];
      if ( p -> type == PLAYER_GUARDIAN )
        continue;

      p -> buffs.stampeding_roar -> trigger();
    }
  }
};

// Starfall Spell ===========================================================

struct lunar_shrapnel_t : public druid_spell_t
{
  lunar_shrapnel_t(druid_t* p) :
    druid_spell_t("lunar_shrapnel", p, p->find_spell(279641))
  {
    background = true;
    aoe = -1;
    base_dd_min = base_dd_max = p->azerite.lunar_shrapnel.value(1);
  }
};

struct starfall_t : public druid_spell_t
{
  struct starfall_tick_t : public druid_spell_t
  {
    starfall_tick_t(const std::string& n, druid_t* p, const spell_data_t* s) :
      druid_spell_t(n, p, s)
    {
      aoe = -1;
      background = dual = direct_tick = true;
      callbacks = false;
      radius = p->spec.starfall->effectN(1).radius();
      radius *= 1.0 + p->talent.stellar_drift->effectN(1).percent();

      base_multiplier *= 1.0 + p->talent.stellar_drift->effectN(2).percent();
    }

    timespan_t travel_time() const override
    {
      //Has a set travel time
      return timespan_t::from_millis(800);
    }

    double action_multiplier() const override
    {
      double am = druid_spell_t::action_multiplier();

      if (p()->mastery.starlight->ok())
        am *= 1.0 + p()->cache.mastery_value();

      if (p()->sets->has_set_bonus(DRUID_BALANCE, T21, B2))
        am *= 1.0 + p()->sets->set(DRUID_BALANCE, T21, B2)->effectN(1).percent();

      return am;
    }

    virtual void impact(action_state_t* s) override
    {
      druid_spell_t::impact(s);
      if (p()->azerite.lunar_shrapnel.ok() && td(target)->dots.moonfire->is_ticking())
      {
        p()->active.lunar_shrapnel->set_target(s->target);
        p()->active.lunar_shrapnel->execute();
      }
    }
  };

  starfall_t(druid_t* p, const std::string& options_str) :
    druid_spell_t("starfall", p, p -> find_specialization_spell("Starfall"), options_str)
  {
    may_miss = may_crit = false;

    if (!p->active.starfall)
    {
      p->active.starfall = new starfall_tick_t("starfall_tick", p, p->find_spell(191037));
      p->active.starfall->stats = stats;
    }

    if (p->azerite.lunar_shrapnel.ok())
    {
      add_child(p->active.lunar_shrapnel);
    }

    base_costs[RESOURCE_ASTRAL_POWER] +=
      p->talent.soul_of_the_forest->effectN(2).resource(RESOURCE_ASTRAL_POWER);
  }

  double cost() const override
  {
    if (p()->buff.oneths_overconfidence->check())
      return 0;

    return druid_spell_t::cost();
  }

  virtual void execute() override
  {
    if (p()->sets->has_set_bonus(DRUID_BALANCE, T20, B4))
    {
      p()->buff.astral_acceleration->trigger();
    }
    if (p()->talent.starlord->ok())
    {
      p()->buff.starlord->trigger();
    }
    if (p()->sets->has_set_bonus(DRUID_BALANCE, T21, B4))
    {
      p()->buff.solar_solstice->trigger();
    }
    druid_spell_t::execute();

    streaking_stars_trigger(SS_STARFALL, nullptr);

    make_event<ground_aoe_event_t>(*sim, p(), ground_aoe_params_t()
      .target(execute_state->target)
      .pulse_time(data().duration() / 9) //ticks 9 times
      .duration(data().duration())
      .action(p()->active.starfall));

    if (p()->buff.oneths_overconfidence->up()) // benefit tracking
      p()->buff.oneths_overconfidence->decrement();
    p()->buff.oneths_intuition->trigger();
    p()->buff.starfall->trigger();
    /* Stellar Drift free movement is granted by uncategorized spell ID 202461
       which is separate from the talent. Currently implemented to grant you this
       buff along with the normal 'starfall' buff. Assumption is that you will
       always choose to place your starfall such that it will cover any movement
       you make. Inaccurate for more complex movement modeling, but such modeling
       is highly unlikely to be examined in the near future.
       
       NOTE that buff is named stellar_drift_2 to hopefully avoid potential confusion
       with the talent spell */
    if (p()->talent.stellar_drift->ok())
      p()->buff.stellar_drift_2->trigger();
  }
};
struct starshards_t : public starfall_t
{
  starshards_t( druid_t* player ) :
    starfall_t( player, "" )
  {
    background = true;
    target = sim -> target;
    radius = 40;
    // Does not cost astral power.
    base_costs.fill( 0 );
  }

  // Allow the action to execute even when Starfall is already active.
  bool ready() override
  { return druid_spell_t::ready(); }
};

// Starsurge Spell ==========================================================

struct starsurge_t : public druid_spell_t
{

  starsurge_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "starsurge", p, p->specialization() == DRUID_BALANCE ? p -> find_affinity_spell( "Starsurge" ) : p->find_spell(197626) , options_str )
  {
  }

  double cost() const override
  {
    if ( p() -> buff.oneths_intuition -> check() )
      return 0;

    double c = druid_spell_t::cost();

    c += p() -> buff.the_emerald_dreamcatcher -> check_stack_value();

    return c;
  }

  double action_multiplier() const override
  {
    double am = druid_spell_t::action_multiplier();

    if ( p() -> mastery.starlight -> ok() )
      am *= 1.0 + p() -> cache.mastery_value();

    if (p()->sets->has_set_bonus(DRUID_BALANCE, T21, B2))
      am *= 1.0 + p()->sets->set(DRUID_BALANCE, T21, B2)->effectN(2).percent();

    return am;
  }

  double composite_crit_damage_bonus_multiplier() const override
  {
     double cdbm = druid_spell_t::composite_crit_damage_bonus_multiplier();

     if ( p() -> sets -> has_set_bonus( DRUID_BALANCE, T20, B2 ))
     {
        cdbm += p() -> sets -> set( DRUID_BALANCE, T20, B2 ) -> effectN(2).percent();

     }


     return cdbm;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double tm = druid_spell_t::composite_target_multiplier( target );
    if ( p()->sets -> has_set_bonus( DRUID_BALANCE, T19, B4 ) )
    {
      bool apply = td( target )->dots.moonfire->is_ticking() & td(target)->dots.sunfire->is_ticking();
      if ( apply )
        tm *= 1.0 + p()->sets -> set( DRUID_BALANCE, T19, B4 )->effectN( 1 ).percent();
    }
    return tm;
  }

  void execute() override
  {
    druid_spell_t::execute();

    if ( p() -> sets -> has_set_bonus( DRUID_BALANCE, T20, B4 ))
       p() -> buff.astral_acceleration -> trigger();

    if (p()->talent.starlord->ok())
        p()->buff.starlord->trigger();

    if (p()->sets->has_set_bonus(DRUID_BALANCE, T21, B4))
        p()->buff.solar_solstice->trigger();

    if ( hit_any_target )
    {
      // Dec 3 2015: Starsurge must hit to grant empowerments, but grants them on cast not impact.
      p() -> buff.solar_empowerment -> trigger();
      p() -> buff.lunar_empowerment -> trigger();
    }

    if ( rng().roll( p() -> starshards ) )
    {
      p() -> proc.starshards -> occur();
      p() -> active.starshards -> execute();
    }

    p() -> buff.the_emerald_dreamcatcher -> up(); // benefit tracking
    p() -> buff.the_emerald_dreamcatcher -> trigger();

    if ( p() -> buff.oneths_intuition -> up() ) // benefit tracking
      p() -> buff.oneths_intuition -> decrement();

    p() -> buff.oneths_overconfidence -> trigger();
  }

  virtual double bonus_da(const action_state_t* s) const override
  {
    double da = druid_spell_t::bonus_da(s);

    if (p()->azerite.arcanic_pulsar.ok())
      da += p()->azerite.arcanic_pulsar.value(2);

    return da;
  }


  void impact(action_state_t* s) override
  {
    druid_spell_t::impact(s);

    if (p()->azerite.arcanic_pulsar.ok())
    {
      p()->buff.arcanic_pulsar->trigger();

      if (p()->buff.arcanic_pulsar->check() == p()->buff.arcanic_pulsar->max_stack())
      {
        timespan_t pulsar_dur = timespan_t::from_seconds(p()->azerite.arcanic_pulsar.spell()->effectN(3).base_value());
        buff_t* proc_buff = p()->talent.incarnation_moonkin->ok() ? p()->buff.incarnation_moonkin : p()->buff.celestial_alignment;

        if (proc_buff->check())
          proc_buff->extend_duration(p(), pulsar_dur);
        else
        {
          proc_buff->trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, pulsar_dur );
          //this number is nowhere to be found in the spell data
          p()->resource_gain( RESOURCE_ASTRAL_POWER, 12, p()->gain.arcanic_pulsar );
        }

        p()->buff.arcanic_pulsar->expire();
        streaking_stars_trigger(SS_CELESTIAL_ALIGNMENT, nullptr);
      }
    }

    streaking_stars_trigger(SS_STARSURGE, s);
  }
};

// Stellar Flare ============================================================

struct stellar_flare_t : public druid_spell_t
{
  stellar_flare_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "stellar_flare", player, player -> talent.stellar_flare, options_str )
  {
    energize_amount = player->talent.stellar_flare->effectN (3).resource (RESOURCE_ASTRAL_POWER);
  }
  double action_multiplier() const override
  {
      double am = druid_spell_t::action_multiplier();

      if (p()->mastery.starlight->ok())
          am *= 1.0 + p()->cache.mastery_value();

      return am;
  }

  void impact(action_state_t* s) override
  {
    druid_spell_t::impact(s);
    streaking_stars_trigger(SS_STELLAR_FLARE, s);
  }
};

// Survival Instincts =======================================================

struct survival_instincts_t : public druid_spell_t
{
  survival_instincts_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "survival_instincts", player, player -> find_specialization_spell( "Survival Instincts" ),
      options_str )
  {
    harmful = false;
    use_off_gcd = true;

    // Spec-based cooldown modifiers
    cooldown -> duration += player -> spec.feral_overrides2 -> effectN( 6 ).time_value();
    cooldown -> duration *= 1.0 + player -> spec.guardian -> effectN( 3 ).percent();

    cooldown -> duration *= 1.0 + player -> talent.survival_of_the_fittest -> effectN( 1 ).percent();
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.survival_instincts -> trigger();
  }
};

// Typhoon ==================================================================

struct typhoon_t : public druid_spell_t
{
  typhoon_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "typhoon", player, player -> talent.typhoon, options_str )
  {
    ignore_false_positive = true;
  }
};

// Warrior of Elune =========================================================

struct warrior_of_elune_t : public druid_spell_t
{
  warrior_of_elune_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "warrior_of_elune", player, player -> talent.warrior_of_elune, options_str )
  {
    harmful = false;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->cooldown.warrior_of_elune->reset(false);

    p() -> buff.warrior_of_elune -> trigger( p() -> talent.warrior_of_elune -> max_stacks() );
  }

  virtual bool ready() override
  {
    if ( p() -> buff.warrior_of_elune -> check() )
      return false;

    return druid_spell_t::ready();
  }
};

// Wild Charge ==============================================================

struct wild_charge_t : public druid_spell_t
{
  double movement_speed_increase;

  wild_charge_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "wild_charge", p, p -> talent.wild_charge, options_str ),
    movement_speed_increase( 5.0 )
  {
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
      p() -> shapeshift( NO_FORM );
  }

  void execute() override
  {
    p() -> buff.wild_charge_movement -> trigger( 1, movement_speed_increase, 1,
        timespan_t::from_seconds( p() -> current.distance_to_move / ( p() -> base_movement_speed * ( 1 + p() -> passive_movement_modifier() +
                                  movement_speed_increase ) ) ) );

    druid_spell_t::execute();
  }

  bool ready() override
  {
    if ( p() -> current.distance_to_move < data().min_range() ) // Cannot charge if the target is too close.
      return false;

    return druid_spell_t::ready();
  }
};

struct force_of_nature_t : public druid_spell_t
{
  timespan_t summon_duration;
  force_of_nature_t( druid_t* p, const std::string options ) :
    druid_spell_t( "force_of_nature", p, p -> talent.force_of_nature ),
    summon_duration( timespan_t::zero() )
  {
    parse_options( options );
    harmful = may_crit = false;
    summon_duration = p->find_spell(248280)->duration() + timespan_t::from_millis(1);
    energize_amount = p->talent.force_of_nature->effectN(5).resource(RESOURCE_ASTRAL_POWER);
  }

  virtual void execute() override
  {
    druid_spell_t::execute();

    streaking_stars_trigger(SS_FORCE_OF_NATURE, nullptr);

    for ( size_t i = 0; i < p() -> force_of_nature.size(); i++ )
    {
      if ( p() -> force_of_nature[i] -> is_sleeping() )
      {
        p() -> force_of_nature[i] -> summon( summon_duration );
        expansion::bfa::trigger_leyshocks_grand_compilation( STAT_CRIT_RATING, player );
      }
    }
  }
};
} // end namespace spells

// ==========================================================================
// Druid Helper Functions & Structures
// ==========================================================================

// Brambles Absorb/Reflect Handler ==========================================

double brambles_handler( const action_state_t* s )
{
  druid_t* p = static_cast<druid_t*>( s -> target );
  assert( p -> talent.brambles -> ok() );
  assert( s );

  /* Calculate the maximum amount absorbed. This is not affected by
     versatility (and likely other player modifiers). */
  double weapon_ap = 0.0;

  if ( p -> buff.cat_form -> check() ) {
    weapon_ap = p -> cat_weapon.dps * WEAPON_POWER_COEFFICIENT;
  } else if ( p -> buff.bear_form -> check() ) {
    weapon_ap = p -> bear_weapon.dps * WEAPON_POWER_COEFFICIENT;
  } else {
    weapon_ap = p -> main_hand_weapon.dps * WEAPON_POWER_COEFFICIENT;
  }

  double attack_power = ( p -> cache.attack_power() + weapon_ap ) * p -> composite_attack_power_multiplier();

  // Brambles coefficient is not in spelldata :(
  double absorb_cap = attack_power * 0.06;

  // Calculate actual amount absorbed.
  double amount_absorbed = std::min( s -> result_mitigated, absorb_cap );

  // Prevent self-harm
  if ( s -> action -> player != p )
  {
    // Schedule reflected damage.
    p -> active.brambles -> base_dd_min = p -> active.brambles -> base_dd_max =
      amount_absorbed;
    action_state_t* ref_s = p -> active.brambles -> get_state();
    ref_s -> target = s -> action -> player;
    p -> active.brambles -> snapshot_state( ref_s, DMG_DIRECT );
    p -> active.brambles -> schedule_execute( ref_s );
  }

  return amount_absorbed;
}

// Earthwarden Absorb Handler ===============================================

double earthwarden_handler( const action_state_t* s )
{
  if ( s -> action -> special )
    return 0;

  druid_t* p = static_cast<druid_t*>( s -> target );
  assert( p -> talent.earthwarden -> ok() );

  if ( ! p -> buff.earthwarden -> up() )
    return 0;

  double absorb = s -> result_amount * p -> buff.earthwarden -> check_value();
  p -> buff.earthwarden -> decrement();

  return absorb;
}

// Persistent Buff Delay Event ==============================================

struct persistent_buff_delay_event_t : public event_t
{
  buff_t* buff;

  persistent_buff_delay_event_t( druid_t* p, buff_t* b ) :
    event_t( *p ), buff( b )
  {
    /* Delay triggering the buff a random amount between 0 and buff_period.
       This prevents fixed-period driver buffs from ticking at the exact same
       times on every iteration.

       Buffs that use the event to activate should implement tick_zero-like
       behavior. */
    schedule( rng().real() * b -> buff_period );
  }

  const char* name() const override
  { return "persistent_buff_delay"; }

  void execute() override
  { buff -> trigger(); }
};

// Ailuro Pouncers Event ====================================================

struct ailuro_pouncers_event_t : public event_t
{
  druid_t* druid;

  ailuro_pouncers_event_t( druid_t* p,
                           timespan_t set_delay = timespan_t::zero() )
    : event_t( *p, set_delay > timespan_t::zero()
                       ? set_delay
                       : p->legendary.ailuro_pouncers ),
      druid( p )
  {
  }

  const char* name() const override
  { return "ailuro_pouncers"; }

  void execute() override
  {
    druid -> buff.predatory_swiftness -> trigger();

    make_event<ailuro_pouncers_event_t>( sim(), druid );
  }
};

// ==========================================================================
// Druid Character Definition
// ==========================================================================

// druid_t::activate ========================================================
void druid_t::activate()
{
   if ( talent.predator -> ok() )
   {
      range::for_each( sim -> actor_list, [this] (player_t* target) -> void {
         if ( !target -> is_enemy() )
            return;

         target->callbacks_on_demise.push_back([this](player_t* target) -> void {
            auto p = this;
            if (p -> specialization() != DRUID_FERAL)
               return;

            if (get_target_data(target) ->       dots.thrash_cat -> is_ticking() ||
                get_target_data(target) ->              dots.rip -> is_ticking() ||
                get_target_data(target) ->             dots.rake -> is_ticking() )
            {


               if (!p->cooldown.tigers_fury->down())
                  p->proc.predator_wasted->occur();

               p->cooldown.tigers_fury->reset(true);
               p->proc.predator->occur();
            }
         });
      });


      callbacks_on_demise.push_back( [this]( player_t* target ) -> void {
         if (sim->active_player->specialization() != DRUID_FERAL)
            return;

         if ( get_target_data( target ) -> dots.thrash_cat       -> is_ticking() ||
              get_target_data( target ) -> dots.rip              -> is_ticking() ||
              get_target_data( target ) -> dots.rake             -> is_ticking() )
         {
            auto p = ( (druid_t*) sim -> active_player );

            if ( !p -> cooldown.tigers_fury -> down() )
                  p -> proc.predator_wasted -> occur( );

            p -> cooldown.tigers_fury -> reset( true );
            p -> proc.predator -> occur();
         }
      });
   }

   player_t::activate();
}

// druid_t::create_action  ==================================================

action_t* druid_t::create_action( const std::string& name,
                                  const std::string& options_str )
{
  using namespace cat_attacks;
  using namespace bear_attacks;
  using namespace heals;
  using namespace spells;

  if ( name == "auto_attack"            ) return new            auto_attack_t( this, options_str );
  if ( name == "barkskin"               ) return new               barkskin_t( this, options_str );
  if ( name == "berserk"                ) return new                berserk_t( this, options_str );
  if ( name == "bear_form"              ) return new      spells::bear_form_t( this, options_str );
  if ( name == "brutal_slash"           ) return new           brutal_slash_t( this, options_str );
  if ( name == "bristling_fur"          ) return new          bristling_fur_t( this, options_str );
  if ( name == "cat_form"               ) return new       spells::cat_form_t( this, options_str );
  if ( name == "celestial_alignment"    ) return new    celestial_alignment_t( this, options_str );
  if ( name == "cenarion_ward"          ) return new          cenarion_ward_t( this, options_str );
  if ( name == "dash"                   ) return new                   dash_t( this, options_str );
  if ( name == "tiger_dash"             ) return new             tiger_dash_t( this, options_str );
  if ( name == "feral_frenzy"           ) return new    feral_frenzy_driver_t( this, options_str );
  if ( name == "ferocious_bite"         ) return new         ferocious_bite_t( this, options_str );
  if ( name == "force_of_nature"        ) return new        force_of_nature_t( this, options_str );
  if ( name == "frenzied_regeneration"  ) return new  frenzied_regeneration_t( this, options_str );
  if ( name == "full_moon"              ) return new              full_moon_t( this, options_str );
  if ( name == "fury_of_elune"          ) return new          fury_of_elune_t( this, options_str );
  if ( name == "growl"                  ) return new                  growl_t( this, options_str );
  if ( name == "half_moon"              ) return new              half_moon_t( this, options_str );
  if ( name == "innervate"              ) return new              innervate_t( this, options_str );
  if ( name == "ironfur"                ) return new                ironfur_t( this, options_str );
  if ( name == "lifebloom"              ) return new              lifebloom_t( this, options_str );
  if ( name == "lunar_beam"             ) return new             lunar_beam_t( this, options_str );
  if ( name == "lunar_strike"           ) return new           lunar_strike_t( this, options_str );
  if ( name == "maim"                   ) return new                   maim_t( this, options_str );
  if ( name == "mangle"                 ) return new                 mangle_t( this, options_str );
  if ( name == "maul"                   ) return new                   maul_t( this, options_str );
  if ( name == "moonfire"               ) return new               moonfire_t( this, options_str );
  if ( name == "moonfire_cat" ||
       name == "lunar_inspiration" )      return new      lunar_inspiration_t( this, options_str );
  if ( name == "new_moon"               ) return new               new_moon_t( this, options_str );
  if ( name == "proc_sephuz"            ) return new            proc_sephuz_t( this, options_str );
  if ( name == "sunfire"                ) return new                sunfire_t( this, options_str );
  if ( name == "moonkin_form"           ) return new   spells::moonkin_form_t( this, options_str );
  if ( name == "moonkin_form_affinity"  ) return new   spells::moonkin_form_affinity_t(this, options_str);
  if ( name == "primal_wrath"           ) return new           primal_wrath_t( this, options_str );
  if ( name == "pulverize"              ) return new              pulverize_t( this, options_str );
  if ( name == "rake"                   ) return new                   rake_t( this, options_str );
  if ( name == "renewal"                ) return new                renewal_t( this, options_str );
  if ( name == "regrowth"               ) return new               regrowth_t( this, options_str );
  if ( name == "rejuvenation"           ) return new           rejuvenation_t( this, options_str );
  if ( name == "rip"                    ) return new                    rip_t( this, options_str );
  if ( name == "savage_roar"            ) return new            savage_roar_t( this, options_str );
  if ( name == "shred"                  ) return new                  shred_t( this, options_str );
  if ( name == "skull_bash"             ) return new             skull_bash_t( this, options_str );
  if ( name == "solar_wrath"            ) return new            solar_wrath_t( this, options_str );
  if ( name == "stampeding_roar"        ) return new        stampeding_roar_t( this, options_str );
  if ( name == "starfall"               ) return new               starfall_t( this, options_str );
  if ( name == "starsurge"              ) return new              starsurge_t( this, options_str );
  if ( name == "stellar_flare"          ) return new          stellar_flare_t( this, options_str );
  if ( name == "prowl"                  ) return new                  prowl_t( this, options_str );
  if ( name == "survival_instincts"     ) return new     survival_instincts_t( this, options_str );
  if ( name == "swipe_cat"              ) return new              swipe_cat_t( this, options_str );
  if ( name == "swipe_bear"             ) return new             swipe_bear_t( this, options_str );
  if ( name == "swipe"                  ) return new            swipe_proxy_t( this, options_str );
  if ( name == "swiftmend"              ) return new              swiftmend_t( this, options_str );
  if ( name == "tigers_fury"            ) return new            tigers_fury_t( this, options_str );
  if ( name == "thrash_bear"            ) return new            thrash_bear_t( this, options_str );
  if ( name == "thrash_cat"             ) return new             thrash_cat_t( this, options_str );
  if ( name == "thrash"                 ) return new           thrash_proxy_t( this, options_str );
  if ( name == "tranquility"            ) return new            tranquility_t( this, options_str );
  if ( name == "typhoon"                ) return new                typhoon_t( this, options_str );
  if ( name == "warrior_of_elune"       ) return new       warrior_of_elune_t( this, options_str );
  if ( name == "wild_charge"            ) return new            wild_charge_t( this, options_str );
  if ( name == "wild_growth"            ) return new            wild_growth_t( this, options_str );
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

  if ( talent.force_of_nature -> ok() )
  {
    for ( pet_t*& pet : force_of_nature )
      pet = new pets::force_of_nature_t( sim, this );
  }
}

// druid_t::init_spells =====================================================

void druid_t::init_spells()
{
  player_t::init_spells();

  // Specializations ========================================================

  // Generic / Multiple specs
  spec.critical_strikes           = find_specialization_spell( "Critical Strikes" );
  spec.druid                      = find_spell( 137009 );
  spec.leather_specialization     = find_specialization_spell( "Leather Specialization" );
  spec.omen_of_clarity            = find_specialization_spell( "Omen of Clarity" );

  // Balance
  spec.astral_power               = find_specialization_spell( "Astral Power" );
  spec.balance                    = find_specialization_spell( "Balance Druid" );
  spec.lunar_empowerment          = find_spell( 164547 );
  spec.solar_empowerment          = find_spell( 164545 );
  spec.solar_wrath_empowerment    = find_spell( 279729 );
  spec.celestial_alignment        = find_specialization_spell( "Celestial Alignment" );
  spec.innervate                  = find_specialization_spell( "Innervate" );
  spec.moonkin_form               = find_specialization_spell( "Moonkin Form" );
  spec.eclipse                    = find_specialization_spell( "Eclipse");
  spec.starfall                   = find_specialization_spell( "Starfall" );
  spec.balance_tier19_2pc         = sets -> has_set_bonus( DRUID_BALANCE, T19, B2 ) ? find_spell( 211089 ) : spell_data_t::not_found();
  spec.starsurge_2                = find_specialization_spell( 231021 ); // Increased empowerment max stacks
  spec.moonkin_2                  = find_specialization_spell( 231042 ); // Owlkin Frenzy proc rate RAWR
  spec.sunfire_2                  = find_specialization_spell( 231050 ); // Sunfire spread. currently contains no value data.
  spec.stellar_drift_2            = find_spell( 202461 ); // stellar drift mobility buff
  spec.owlkin_frenzy              = find_spell( 157228 ); // Owlkin Frenzy RAWR
  spec.sunfire_dmg                = find_spell( 164815 ); // dot debuff for sunfire
  spec.moonfire_dmg               = find_spell( 164812 ); // dot debuff for moonfire
  spec.shooting_stars_dmg         = find_spell( 202497 ); // not referenced in shooting stars talent
  spec.half_moon                  = find_spell( 274282 ); // not referenced in new moon talent
  spec.full_moon                  = find_spell( 274283 ); // not referenced in new moon talent

  // Feral
  spec.cat_form                   = find_class_spell( "Cat Form" ) -> ok() ? find_spell( 3025   ) : spell_data_t::not_found();
  spec.cat_form_speed             = find_class_spell( "Cat Form" ) -> ok() ? find_spell( 113636 ) : spell_data_t::not_found();
  spec.feral                      = find_specialization_spell( "Feral Druid" );
  spec.feral_overrides            = specialization() == DRUID_FERAL ? find_spell( 197692 ) : spell_data_t::not_found();
  spec.feral_overrides2           = specialization() == DRUID_FERAL ? find_spell( 106733 ) : spell_data_t::not_found();
  spec.gushing_wound              = sets -> has_set_bonus( DRUID_FERAL, T17, B4 ) ? find_spell( 165432 ) : spell_data_t::not_found();
  spec.bloody_gash                = sets -> has_set_bonus( DRUID_FERAL, T21, B2 ) ? find_spell( 252750 ) : spell_data_t::not_found();
  spec.predatory_swiftness        = find_specialization_spell( "Predatory Swiftness" );
  spec.primal_fury                = find_spell( 16953 );
  spec.rip                        = find_specialization_spell( "Rip" );
  spec.sharpened_claws            = find_specialization_spell( "Sharpened Claws" );
  spec.swipe_cat                  = find_spell( 106785 );
  spec.rake_2                     = find_spell( 231052 );
  spec.tigers_fury                = find_specialization_spell( "Tiger's Fury" );
  spec.tigers_fury_2              = find_spell( 231055 );
  spec.ferocious_bite_2           = find_spell( 231056 );
  spec.shred                      = find_spell( 5221 );
  spec.shred_2                    = find_spell( 231063 );
  spec.shred_3                    = find_spell( 231057 );
  spec.swipe_2                    = find_spell( 231283 );
  spec.savage_roar                = find_spell( 62071 );


  // Guardian
  spec.bear_form                  = find_class_spell( "Bear Form" ) -> ok() ? find_spell( 1178 ) : spell_data_t::not_found();
  spec.gore                       = find_specialization_spell( "Gore" );
  spec.guardian                   = find_specialization_spell( "Guardian Druid" );
  spec.guardian_overrides         = find_specialization_spell( "Guardian Overrides Passive" );
  spec.ironfur                    = find_specialization_spell( "Ironfur" );
  spec.thrash_bear_dot            = find_specialization_spell( "Thrash" ) -> ok() ? find_spell( 192090 ) : spell_data_t::not_found();
  spec.lightning_reflexes         = find_specialization_spell( 231064 );
  spec.mangle_2                   = find_specialization_spell( 231064 );
  spec.ironfur_2                  = find_specialization_spell( 231070 );
  spec.frenzied_regeneration_2    = find_specialization_spell( 273048 );

  // Restoration
  spec.moonkin_form_affinity      = find_spell(197625);
  spec.restoration                = find_specialization_spell("Restoration Druid");

  // Talents ================================================================

  // Multiple Specs
  talent.renewal                        = find_talent_spell( "Renewal" );
  talent.tiger_dash                     = find_talent_spell( "Tiger Dash" );
  talent.wild_charge                    = find_talent_spell( "Wild Charge" );

  talent.balance_affinity               = find_talent_spell( "Balance Affinity" );
  talent.feral_affinity                 = find_talent_spell( "Feral Affinity" );
  talent.guardian_affinity              = find_talent_spell( "Guardian Affinity" );
  talent.restoration_affinity           = find_talent_spell( "Restoration Affinity" );

  talent.mighty_bash                    = find_talent_spell( "Mighty Bash" );
  talent.mass_entanglement              = find_talent_spell( "Mass Entanglement" );
  talent.typhoon                        = find_talent_spell( "Typhoon" );

  talent.soul_of_the_forest             = find_talent_spell( "Soul of the Forest" );
  talent.moment_of_clarity              = find_talent_spell( "Moment of Clarity" );

  // Feral
  talent.predator                       = find_talent_spell( "Predator" );
  talent.blood_scent                    = find_talent_spell( "Blood Scent" );
  talent.lunar_inspiration              = find_talent_spell( "Lunar Inspiration" );

  talent.incarnation_cat                = find_talent_spell( "Incarnation: King of the Jungle" );
  talent.brutal_slash                   = find_talent_spell( "Brutal Slash" );

  talent.sabertooth                     = find_talent_spell( "Sabertooth" );
  talent.jagged_wounds                  = find_talent_spell( "Jagged Wounds" );

  talent.scent_of_blood = find_talent_spell("Scent of Blood");
  talent.primal_wrath = find_talent_spell("Primal Wrath");

  talent.savage_roar                    = find_talent_spell( "Savage Roar" );
  talent.bloodtalons                    = find_talent_spell( "Bloodtalons" );
  talent.feral_frenzy = find_talent_spell("Feral Frenzy");

  // Balance
  talent.natures_balance                = find_talent_spell( "Nature's Balance" );
  talent.warrior_of_elune               = find_talent_spell( "Warrior of Elune" );
  talent.force_of_nature                = find_talent_spell( "Force of Nature");

  talent.twin_moons                     = find_talent_spell( "Twin Moons");
  talent.incarnation_moonkin            = find_talent_spell( "Incarnation: Chosen of Elune" );

  talent.starlord                       = find_talent_spell( "Starlord");
  talent.stellar_drift                  = find_talent_spell( "Stellar Drift");
  talent.stellar_flare                  = find_talent_spell( "Stellar Flare");

  talent.shooting_stars                 = find_talent_spell( "Shooting Stars");
  talent.fury_of_elune                  = find_talent_spell( "Fury of Elune" );
  talent.new_moon                       = find_talent_spell( "New Moon");

  // Guardian
  talent.brambles                       = find_talent_spell( "Brambles" );
  talent.pulverize                      = find_talent_spell( "Pulverize" );
  talent.blood_frenzy                   = find_talent_spell( "Blood Frenzy" );

  talent.gutteral_roars                 = find_talent_spell( "Gutteral Roars" );

  talent.incarnation_bear               = find_talent_spell( "Incarnation: Guardian of Ursoc" );
  talent.galactic_guardian              = find_talent_spell( "Galactic Guardian" );

  talent.earthwarden                    = find_talent_spell( "Earthwarden" );
  talent.guardian_of_elune              = find_talent_spell( "Guardian of Elune" );
  talent.survival_of_the_fittest        = find_talent_spell( "Survival of the Fittest" );

  talent.rend_and_tear                  = find_talent_spell( "Rend and Tear" );
  talent.lunar_beam                     = find_talent_spell( "Lunar Beam" );
  talent.bristling_fur                  = find_talent_spell( "Bristling Fur" );

  // Restoration
  talent.verdant_growth                 = find_talent_spell( "Verdant Growth" );
  talent.cenarion_ward                  = find_talent_spell( "Cenarion Ward" );
  talent.germination                    = find_talent_spell( "Germination" );

  talent.incarnation_tree               = find_talent_spell( "Incarnation: Tree of Life" );
  talent.cultivation                    = find_talent_spell( "Cultivation" );

  talent.prosperity                     = find_talent_spell( "Prosperity" );
  talent.inner_peace                    = find_talent_spell( "Inner Peace" );
  talent.profusion                      = find_talent_spell( "Profusion" );

  talent.stonebark                      = find_talent_spell( "Stonebark" );
  talent.flourish                       = find_talent_spell( "Flourish" );

  if ( talent.earthwarden -> ok() )
  {
    instant_absorb_list.insert( std::make_pair<unsigned, instant_absorb_t>(
        talent.earthwarden->id(),
        instant_absorb_t( this, find_spell( 203975 ), "earthwarden", &earthwarden_handler ) ) );
  }

  // Azerite ================================================================
  // Balance
  azerite.dawning_sun = find_azerite_spell("Dawning Sun");
  azerite.high_noon = find_azerite_spell("High Noon");
  azerite.lively_spirit = find_azerite_spell("Lively Spirit");
  azerite.long_night = find_azerite_spell("Long Night");
  azerite.lunar_shrapnel = find_azerite_spell("Lunar Shrapnel");
  azerite.power_of_the_moon = find_azerite_spell("Power of the Moon");
  azerite.streaking_stars = find_azerite_spell("Streaking Stars");
  azerite.arcanic_pulsar = find_azerite_spell("Arcanic Pulsar");

  // Feral
  azerite.blood_mist = find_azerite_spell("Blood Mist");
  azerite.gushing_lacerations = find_azerite_spell("Gushing Lacerations");
  azerite.iron_jaws = find_azerite_spell("Iron Jaws");
  azerite.primordial_rage = find_azerite_spell("Primordial Rage");
  azerite.raking_ferocity = find_azerite_spell("Raking Ferocity");
  azerite.shredding_fury = find_azerite_spell("Shredding Fury");
  azerite.wild_fleshrending = find_azerite_spell("Wild Fleshrending");
  azerite.jungle_fury = find_azerite_spell("Jungle Fury");
  azerite.untamed_ferocity = find_azerite_spell("Untamed Ferocity");

  // Guardian
  azerite.craggy_bark = find_azerite_spell("Craggy Bark");
  azerite.gory_regeneration = find_azerite_spell("Gory Regeneration");
  azerite.guardians_wrath = find_azerite_spell("Guardian's Wrath");
  azerite.layered_mane = find_azerite_spell("Layered Mane");
  azerite.masterful_instincts = find_azerite_spell("Masterful Instincts");
  azerite.twisted_claws = find_azerite_spell("Twisted Claws");
  azerite.burst_of_savagery = find_azerite_spell("Burst of Savagery");

  // Affinities =============================================================

  spec.feline_swiftness = specialization() == DRUID_FERAL || talent.feral_affinity -> ok() ?
    find_spell( 131768 ) : spell_data_t::not_found();

  spec.astral_influence = specialization() == DRUID_BALANCE || talent.balance_affinity -> ok() ?
    find_spell( 197524 ) : spell_data_t::not_found();

  spec.thick_hide = specialization() == DRUID_GUARDIAN || talent.guardian_affinity -> ok() ?
    find_spell( 16931 ) : spell_data_t::not_found();

  spec.yseras_gift = specialization() == DRUID_RESTORATION || talent.restoration_affinity -> ok() ?
    find_spell( 145108 ) : spell_data_t::not_found();

  // Masteries ==============================================================

  mastery.razor_claws         = find_mastery_spell( DRUID_FERAL );
  mastery.harmony             = find_mastery_spell( DRUID_RESTORATION );
  mastery.natures_guardian    = find_mastery_spell( DRUID_GUARDIAN );
  mastery.natures_guardian_AP = mastery.natures_guardian -> ok() ?
    find_spell( 159195 ) : spell_data_t::not_found();
  mastery.starlight           = find_mastery_spell( DRUID_BALANCE );

  // Active Actions =========================================================

  caster_melee_attack = new caster_attacks::druid_melee_t( this );
  if ( !this -> cat_melee_attack )
  {
     this -> init_beast_weapon( this -> cat_weapon, 1.0 );
     this -> cat_melee_attack = new cat_attacks::cat_melee_t( this );
  }

  if ( !this -> bear_melee_attack )
  {
     this -> init_beast_weapon( this->bear_weapon, 2.5 );
     this -> bear_melee_attack = new bear_attacks::bear_melee_t( this );
  }

  if ( talent.cenarion_ward -> ok() )
    active.cenarion_ward_hot  = new heals::cenarion_ward_hot_t( this );
  if ( spec.yseras_gift )
    active.yseras_gift        = new heals::yseras_gift_t( this );
  if ( sets -> has_set_bonus( DRUID_FERAL, T17, B4 ) )
    active.gushing_wound      = new cat_attacks::gushing_wound_t( this );
  if ( talent.brambles -> ok() )
  {
    active.brambles           = new spells::brambles_t( this );
    active.brambles_pulse     = new spells::brambles_pulse_t( this );

    instant_absorb_list.insert( std::make_pair<unsigned, instant_absorb_t>(
        talent.brambles->id(), instant_absorb_t( this, talent.brambles, "brambles", &brambles_handler ) ) );
  }
  if ( talent.galactic_guardian -> ok() )
  {
    active.galactic_guardian  = new spells::moonfire_t::galactic_guardian_damage_t( this );
    active.galactic_guardian -> stats = get_stats( "moonfire" );
  }
 
  if ( mastery.natures_guardian -> ok() )
    active.natures_guardian = new heals::natures_guardian_t( this );

  active.solar_empowerment = new spells::solar_empowerment_t (this);

  if (azerite.lunar_shrapnel.ok())
  {
    active.lunar_shrapnel = new spells::lunar_shrapnel_t(this);
  }
  if (azerite.streaking_stars.ok())
  {
    active.streaking_stars = new spells::streaking_stars_t(this);
  }
}

// druid_t::init_base =======================================================

void druid_t::init_base_stats()
{
  // Set base distance based on spec
  if ( base.distance < 1 )
    base.distance = ( specialization() == DRUID_FERAL || specialization() == DRUID_GUARDIAN ) ? 5 : 30;

  player_t::init_base_stats();

  base.attack_power_per_agility  = specialization() == DRUID_FERAL || specialization() == DRUID_GUARDIAN ? 1.0 : 0.0;
  base.spell_power_per_intellect = specialization() == DRUID_BALANCE || specialization() == DRUID_RESTORATION ? 1.0 : 0.0;

  // Resources
  resources.base[ RESOURCE_RAGE         ] = 100;
  resources.base[ RESOURCE_COMBO_POINT  ] = 5;
  resources.base[ RESOURCE_ASTRAL_POWER ] = 100
      + sets -> set( DRUID_BALANCE, T20, B2 )->effectN(1).resource(RESOURCE_ASTRAL_POWER);
  resources.base[ RESOURCE_ENERGY       ] = 100
      + sets->set(DRUID_FERAL, T18, B2)->effectN(2).resource(RESOURCE_ENERGY)
      + talent.moment_of_clarity->effectN(3).resource(RESOURCE_ENERGY);

  resources.active_resource[ RESOURCE_ASTRAL_POWER ] = specialization() == DRUID_BALANCE;
  resources.active_resource[ RESOURCE_HEALTH       ] = primary_role() == ROLE_TANK || talent.guardian_affinity -> ok();
  resources.active_resource[ RESOURCE_MANA         ] = primary_role() == ROLE_HEAL || talent.restoration_affinity -> ok()
      || talent.balance_affinity -> ok() || specialization() == DRUID_GUARDIAN;
  resources.active_resource[ RESOURCE_ENERGY       ] = primary_role() == ROLE_ATTACK || talent.feral_affinity -> ok()
      || specialization() == DRUID_RESTORATION;
  resources.active_resource[ RESOURCE_COMBO_POINT  ] = primary_role() == ROLE_ATTACK || talent.feral_affinity -> ok();
  resources.active_resource[ RESOURCE_RAGE         ] = primary_role() == ROLE_TANK || talent.guardian_affinity -> ok();

  resources.base_regen_per_second[ RESOURCE_ENERGY ] = 10;
  if ( specialization() == DRUID_FERAL )
  {
    resources.base_regen_per_second[ RESOURCE_ENERGY ] *=
        1.0 + query_aura_effect( spec.feral, E_APPLY_AURA, A_MOD_POWER_REGEN_PERCENT, 3 )->percent();
  }
  resources.base_regen_per_second[ RESOURCE_ENERGY ] *= 1.0 + talent.feral_affinity -> effectN( 2 ).percent();

  base_gcd = timespan_t::from_seconds( 1.5 );
}

// druid_t::init_buffs ======================================================

void druid_t::create_buffs()
{
  player_t::create_buffs();

  using namespace buffs;

  // Generic / Multi-spec druid buffs

  buff.bear_form             = new buffs::bear_form_t( *this );

  buff.cat_form              = new buffs::cat_form_t( *this );

  buff.clearcasting          = buff_creator_t( this, "clearcasting", spec.omen_of_clarity -> effectN( 1 ).trigger() )
                               .cd( spec.omen_of_clarity -> internal_cooldown() )
                               .chance( spec.omen_of_clarity -> proc_chance() )
                               .max_stack( (unsigned) ( 1 + talent.moment_of_clarity->effectN(1).base_value() ) )
                               .default_value( specialization() != DRUID_RESTORATION
                                               ? talent.moment_of_clarity -> effectN( 4 ).percent()
                                               : 0.0 );

  buff.dash = buff_creator_t( this, "dash", find_class_spell( "Dash" ) )
                  .cd( timespan_t::zero() )
                  .default_value( find_class_spell( "Dash" )->effectN( 1 ).percent() );

  buff.prowl = buff_creator_t( this, "prowl", find_class_spell( "Prowl" ) );

  buff.innervate = new innervate_buff_t(*this);

  // Azerite
  buff.shredding_fury =
      buff_creator_t( this, "shredding_fury", find_spell( 274426 ) ).default_value( azerite.shredding_fury.value() );

  buff.jungle_fury = make_buff<stat_buff_t>( this, "jungle_fury", find_spell( 274425 ) )
                         ->add_stat( STAT_CRIT_RATING, azerite.jungle_fury.value( 1 ) )
                         ->set_chance( azerite.jungle_fury.enabled() ? 1.0 : 0.0 );

  buff.iron_jaws = buff_creator_t( this, "iron_jaws", find_spell( 276026 ) );

  buff.raking_ferocity = buff_creator_t( this, "raking_ferocity", find_spell( 273340 ) );

  buff.dawning_sun = make_buff(this, "dawning_sun", azerite.dawning_sun.spell()->effectN(1).trigger()->effectN(1).trigger());

  buff.lively_spirit = make_buff<stat_buff_t>(this, "lively_spirit", find_spell(279648))
    ->add_stat(STAT_INTELLECT, azerite.lively_spirit.value());

  buff.arcanic_pulsar = make_buff(this, "arcanic_pulsar", azerite.arcanic_pulsar.spell()->effectN(1).trigger()->effectN(1).trigger());

  // Talent buffs
  buff.tiger_dash = new tiger_dash_buff_t(*this);
  
  buff.wild_charge_movement  = buff_creator_t( this, "wild_charge_movement" );

  buff.cenarion_ward         = buff_creator_t( this, "cenarion_ward", find_talent_spell( "Cenarion Ward" ) );

  buff.incarnation_cat       = new incarnation_cat_buff_t( *this );

  buff.jungle_stalker        = buff_creator_t( this, "jungle_stalker", find_spell( 252071 ) );

  buff.incarnation_bear      = buff_creator_t( this, "incarnation_guardian_of_ursoc", talent.incarnation_bear )
                               .add_invalidate( CACHE_ARMOR )
                               .cd( timespan_t::zero() );

  buff.incarnation_tree      = buff_creator_t( this, "incarnation_tree_of_life", talent.incarnation_tree )
                               .duration( timespan_t::from_seconds( 30 ) )
                               .default_value( find_spell( 5420 ) -> effectN( 1 ).percent() )
                               .add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER )
                               .cd( timespan_t::zero() );

  buff.bloodtalons           = buff_creator_t( this, "bloodtalons", talent.bloodtalons -> ok() ? find_spell( 145152 ) : spell_data_t::not_found() )
                               .default_value( find_spell( 145152 ) -> effectN( 1 ).percent() )
                               .max_stack( 2 ); 

  buff.galactic_guardian     = buff_creator_t( this, "galactic_guardian", find_spell( 213708 ) )
                               .chance( talent.galactic_guardian -> ok() )
                               .default_value( find_spell( 213708 ) -> effectN( 1 ).resource( RESOURCE_RAGE ) );

  buff.gore                  = buff_creator_t( this, "mangle", find_spell( 93622 ) )
                               .default_value( find_spell( 93622 ) -> effectN( 1 ).resource( RESOURCE_RAGE ) );

  buff.guardian_of_elune     = buff_creator_t( this, "guardian_of_elune", talent.guardian_of_elune -> effectN( 1 ).trigger() )
                               .chance( talent.guardian_of_elune -> ok() )
                               .default_value( talent.guardian_of_elune -> effectN( 1 ).trigger() -> effectN( 1 ).time_value().total_seconds() );

  // Balance

  buff.natures_balance = make_buff(this, "natures_balance", talent.natures_balance)
    ->set_tick_zero(true)
    ->set_tick_callback([this] (buff_t*, int, const timespan_t&) {
        resource_gain(RESOURCE_ASTRAL_POWER, talent.natures_balance->effectN(1).resource(RESOURCE_ASTRAL_POWER), gain.natures_balance);
      } );
  
  buff.celestial_alignment = make_buff(this, "celestial_alignment", spec.celestial_alignment)
    ->set_cooldown(timespan_t::zero())
    ->set_default_value(spec.celestial_alignment->effectN(1).percent())
    ->add_invalidate(CACHE_PLAYER_DAMAGE_MULTIPLIER)
    ->add_invalidate(CACHE_HASTE)
    ->add_invalidate(CACHE_SPELL_HASTE);

  buff.incarnation_moonkin = make_buff(this, "incarnation_chosen_of_elune", talent.incarnation_moonkin)
    ->set_cooldown(timespan_t::zero())
    ->set_default_value(talent.incarnation_moonkin->effectN(1).percent())
    ->add_invalidate(CACHE_PLAYER_DAMAGE_MULTIPLIER)
    ->add_invalidate(CACHE_HASTE)
    ->add_invalidate(CACHE_SPELL_HASTE);
  
  buff.fury_of_elune = make_buff(this, "fury_of_elune", talent.fury_of_elune)
    ->set_tick_callback([this](buff_t*, int, const timespan_t&) {
        resource_gain(RESOURCE_ASTRAL_POWER, talent.fury_of_elune->effectN(3).resource(RESOURCE_ASTRAL_POWER), gain.fury_of_elune);
      } );

  buff.lunar_empowerment = make_buff(this, "lunar_empowerment", spec.lunar_empowerment)
    ->set_default_value(spec.lunar_empowerment->effectN(1).percent())
    ->set_max_stack(spec.lunar_empowerment->max_stacks() + (unsigned) spec.starsurge_2->effectN(1).base_value());

  buff.solar_empowerment = make_buff(this, "solar_empowerment", spec.solar_empowerment)
    ->set_default_value(spec.solar_empowerment->effectN(1).percent())
    ->set_max_stack(spec.solar_empowerment->max_stacks() + (unsigned) spec.starsurge_2->effectN(1).base_value());

  buff.moonkin_form = new buffs::moonkin_form_t(*this);

  buff.moonkin_form_affinity = new buffs::moonkin_form_affinity_t(*this);

  buff.warrior_of_elune = new warrior_of_elune_buff_t(*this);

  buff.owlkin_frenzy = make_buff(this, "owlkin_frenzy", spec.owlkin_frenzy)
    ->set_default_value(spec.owlkin_frenzy->effectN(1).percent())
    ->set_chance(spec.moonkin_2->effectN(1).percent());

  buff.astral_acceleration = make_buff(this, "astral_acceleration", find_spell(242232))
    ->set_cooldown(timespan_t::zero())
    ->set_default_value(find_spell(242232)->effectN(1).percent())
    ->set_refresh_behavior(buff_refresh_behavior::DISABLED)
    ->add_invalidate(CACHE_SPELL_HASTE);

  buff.starlord = make_buff(this, "starlord", find_spell(279709))
    ->set_cooldown(timespan_t::zero())
    ->set_default_value(find_spell(279709)->effectN(1).percent())
    ->set_refresh_behavior(buff_refresh_behavior::DISABLED)
    ->add_invalidate(CACHE_SPELL_HASTE);

  buff.solar_solstice = make_buff(this, "solar_solstice", find_spell(252767))
    ->set_default_value(find_spell(252767)->effectN(1).percent());

  buff.starfall = make_buff(this, "starfall", spec.starfall);

  buff.stellar_drift_2 = make_buff(this, "stellar_drift", find_spell(202461))
    ->set_duration(spec.starfall->duration()); // peg stellar drift duration to starfall's

  // Feral

  buff.apex_predator        = buff_creator_t(this, "apex_predator", find_spell(252752))
                               .chance(  sets -> has_set_bonus( DRUID_FERAL, T21, B4 ) ? find_spell( 251790 ) -> proc_chance() : 0 /*: 0 )*/ );

  buff.berserk               = new berserk_buff_t( *this );

  buff.predatory_swiftness   = buff_creator_t( this, "predatory_swiftness", find_spell( 69369 ) )
                               .chance( spec.predatory_swiftness -> ok() );

  buff.savage_roar           = buff_creator_t( this, "savage_roar", talent.savage_roar )
                               .default_value( spec.savage_roar -> effectN( 1 ).percent() )
                               .refresh_behavior( buff_refresh_behavior::DURATION ) // Pandemic refresh is done by the action
                               .tick_behavior( buff_tick_behavior::NONE );

  buff.tigers_fury           = new tigers_fury_buff_t( *this );
    /*= buff_creator_t( this, "tigers_fury", spec.tigers_fury )
                               .default_value( spec.tigers_fury -> effectN( 1 ).percent() )
                               .cd( timespan_t::zero() );*/

  buff.scent_of_blood = make_buff( this, "scent_of_blood", find_spell( 285646 ) )
                            ->set_max_stack( 10 )
                            ->set_default_value( talent.scent_of_blood->effectN( 1 ).base_value() )
                            ->set_chance( talent.scent_of_blood->ok() ? 1.0 : 0.0 );

  // Guardian
  buff.barkskin              = buff_creator_t( this, "barkskin", find_specialization_spell( "Barkskin" ) )
                               .cd( timespan_t::zero() )
                               .default_value( find_specialization_spell( "Barkskin" ) -> effectN( 2 ).percent() )
                               .duration( find_specialization_spell( "Barkskin" ) -> duration() )
                               .tick_behavior( talent.brambles -> ok() ? buff_tick_behavior::REFRESH : buff_tick_behavior::NONE )
                               .tick_callback( [ this ] ( buff_t*, int, const timespan_t& ) {
                                 if ( talent.brambles -> ok() )
                                   active.brambles_pulse -> execute();
                               } );

  buff.bristling_fur         = buff_creator_t( this, "bristling_fur", talent.bristling_fur )
                               .cd( timespan_t::zero() );

  buff.earthwarden           = buff_creator_t( this, "earthwarden", find_spell( 203975 ) )
                               .default_value( talent.earthwarden -> effectN( 1 ).percent() );

  buff.earthwarden_driver    = buff_creator_t( this, "earthwarden_driver", talent.earthwarden )
                               .quiet( true )
                               .tick_callback( [ this ] ( buff_t*, int, const timespan_t& ) { buff.earthwarden -> trigger(); } )
                               .tick_zero( true );

  buff.guardians_wrath       = buff_creator_t( this, "guardians_wrath", find_spell( 279541 ) )
                               .default_value( find_spell( 279541 ) -> effectN( 1 ).resource( RESOURCE_RAGE ) );

  buff.ironfur               = buff_creator_t( this, "ironfur", spec.ironfur )
                               .duration( spec.ironfur -> duration() )
                               .default_value( spec.ironfur -> effectN( 1 ).percent() )
                               .add_invalidate( CACHE_ARMOR )
                               .add_invalidate( CACHE_AGILITY )
                               .max_stack( (unsigned) ( specialization() == DRUID_GUARDIAN ? spec.ironfur_2 -> effectN( 1 ).base_value() :
                                           1 ) )
                               .stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
                               .cd( timespan_t::zero() );

  buff.masterful_instincts   = make_buff<stat_buff_t>( this, "masterful_instincts", find_spell( 273349 ) )
                               -> add_stat( STAT_MASTERY_RATING, azerite.masterful_instincts.value( 1 ) )
                               -> add_stat( STAT_ARMOR, azerite.masterful_instincts.value( 2 ) );

  buff.pulverize             = buff_creator_t( this, "pulverize", find_spell( 158792 ) )
                               .default_value( find_spell( 158792 ) -> effectN( 1 ).percent() )
                               .refresh_behavior( buff_refresh_behavior::PANDEMIC );

  buff.survival_instincts    = new survival_instincts_buff_t( *this );

  buff.twisted_claws         = make_buff<stat_buff_t>( this, "twisted_claws", find_spell( 275909 ) )
                               -> add_stat( STAT_AGILITY, azerite.twisted_claws.value( 1 ) )
                               -> set_chance( find_spell( 275908 ) -> proc_chance() );

  buff.burst_of_savagery     = make_buff<stat_buff_t>( this, "burst_of_savagery", find_spell( 289315 ) )
                               -> add_stat( STAT_MASTERY_RATING, azerite.burst_of_savagery.value( 1 ) );

  // Restoration
  buff.harmony               = buff_creator_t( this, "harmony", mastery.harmony -> ok() ? find_spell( 100977 ) : spell_data_t::not_found() );

  buff.soul_of_the_forest    = buff_creator_t( this, "soul_of_the_forest",
                               talent.soul_of_the_forest -> ok() ? find_spell( 114108 ) : spell_data_t::not_found() )
                               .default_value( find_spell( 114108 ) -> effectN( 1 ).percent() );

  if ( specialization() == DRUID_RESTORATION || talent.restoration_affinity -> ok() )
  {
    buff.yseras_gift         = buff_creator_t( this, "yseras_gift_driver", spec.yseras_gift )
                               .quiet( true )
                               .tick_callback( [ this ]( buff_t*, int, const timespan_t& ) {
                                 active.yseras_gift -> schedule_execute(); } )
                               .tick_zero( true );
  }

  // Set Bonuses

  buff.feral_tier17_4pc      = buff_creator_t( this, "feral_tier17_4pc", find_spell( 166639 ) )
                               .quiet( true );

  buff.guardian_tier17_4pc   = buff_creator_t( this, "guardian_tier17_4pc", find_spell( 177969 ) )
                               .default_value( find_spell( 177969 ) -> effectN( 1 ).percent() );

  buff.guardian_tier19_4pc   = buff_creator_t( this, "natural_defenses",
                                 sets -> set( DRUID_GUARDIAN, T19, B4 ) -> effectN( 1 ).trigger() )
                               .trigger_spell( sets -> set( DRUID_GUARDIAN, T19, B4 ) )
                               .default_value( sets -> set( DRUID_GUARDIAN, T19, B4 ) -> effectN( 1 ).trigger() -> effectN( 1 ).base_value() / 1000.0 );
}

// ALL Spec Pre-Combat Action Priority List =================================
std::string druid_t::default_flask() const
{
   std::string balance_flask =   (true_level > 110) ? "endless_fathoms" :
                                 (true_level > 100) ? "whispered_pact" :
                                 (true_level >= 90) ? "greater_draenic_intellect_flask" :
                                 (true_level >= 85) ? "warm_sun" :
                                 (true_level >= 80) ? "draconic_mind" :
                                 "disabled";

   std::string feral_flask =     (true_level >  110) ? "flask_of_the_currents" :
                                 (true_level >  100) ? "seventh_demon" :
                                 (true_level >= 90) ? "greater_draenic_agility_flask" :
                                 (true_level >= 85) ? "spring_blossoms" :
                                 (true_level >= 80) ? "winds" :
                                 "disabled";

   std::string guardian_flask =  (true_level >  110) ? "flask_of_the_currents" :
                                 (true_level >  100) ? "seventh_demon" :
                                 (true_level >= 90) ? "greater_draenic_agility_flask" :
                                 (true_level >= 85) ? "spring_blossoms" :
                                 (true_level >= 80) ? "winds" :
                                 "disabled";

   switch ( specialization() )
   {
   case DRUID_FERAL:
      return feral_flask;
   case DRUID_BALANCE:
      return balance_flask;
   case DRUID_GUARDIAN:
      return guardian_flask;
   default:
      return "disabled";
   }

}
std::string druid_t::default_potion() const
{
   std::string balance_pot =  (true_level > 110) ? "potion_of_rising_death" :
                              (true_level > 100) ? "deadly_grace" :
                              (true_level >= 90) ? "draenic_intellect" :
                              (true_level >= 85) ? "jade_serpent" :
                              (true_level >= 80) ? "volcanic" :
                              "disabled";

      std::string feral_pot = (true_level > 110) ? "battle_potion_of_agility" :
                              (true_level > 100) ? "potion_of_prolonged_power" :
                              (true_level >= 90) ? "draenic_agility" :
                              (true_level >= 85) ? "virmens_bite" :
                              (true_level >= 80) ? "tolvir" :
                              "disabled";

   std::string guardian_pot = (true_level > 110) ? "potion_of_bursting_blood" :
                              (true_level > 100) ? "old_war" :
                              (true_level >= 90) ? "draenic_agility" :
                              (true_level >= 85) ? "virmens_bite" :
                              (true_level >= 80) ? "tolvir" :
                              "disabled";


   switch (specialization())
   {
   case DRUID_FERAL:
      return feral_pot;
   case DRUID_BALANCE:
      return balance_pot;
   case DRUID_GUARDIAN:
      return guardian_pot;
   default:
      return "disabled";
   }
}

std::string druid_t::default_food() const
{
   return (true_level > 110) ? "bountiful_captains_feast" :
          (true_level > 100 && specialization() == DRUID_FERAL) ? "lemon_herb_filet" :
          (true_level >  100) ? "lavish_suramar_feast" :
          (true_level >  90) ?  "pickled_eel" :
          (true_level >= 90) ?  "sea_mist_rice_noodles" :
          (true_level >= 80) ?  "seafood_magnifique_feast" :
          "disabled";
}

std::string druid_t::default_rune() const
{
   return (true_level >= 120) ? "battle_scarred" :
          (true_level >= 110) ? "defiled" :
          (true_level >= 100) ? "hyper" :
          "disabled";
}


void druid_t::apl_precombat()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

   // Flask
   precombat->add_action("flask");

   // Food
   precombat->add_action("food");

   // Rune
   precombat->add_action("augmentation");

  // Feral: Bloodtalons
  if ( specialization() == DRUID_FERAL && true_level >= 100 )
    precombat -> add_action( this, "Regrowth", "if=talent.bloodtalons.enabled" );

  // Feral: Rotational control variables
  if ( specialization() == DRUID_FERAL )
  {
    precombat->add_action( "variable,name=use_thrash,value=0", "It is worth it for almost everyone to maintain thrash" );
    precombat->add_action( "variable,name=use_thrash,value=2,if=azerite.wild_fleshrending.enabled");
    //precombat->add_action( "variable,name=opener_done,value=0" );
    /*precombat->add_action( "variable,name=delayed_tf_opener,value=0",
                           "Opener TF is delayed if we need to hardcast regrowth later on in the rotation" );
    precombat->add_action(
        "variable,name=delayed_tf_opener,value=1,if=talent.sabertooth.enabled&talent.bloodtalons.enabled&!talent.lunar_"
        "inspiration.enabled",
        "This happens when Sabertooth, Bloodtalons but not LI is talented" );*/
  }

  // Balance
  if ( specialization() == DRUID_BALANCE )
  {
    // Azerite variables
    precombat->add_action("variable,name=az_ss,value=azerite.streaking_stars.rank", "Azerite variables");
    precombat->add_action("variable,name=az_ap,value=azerite.arcanic_pulsar.rank");
    // Starfall v Starsurge target cutoff
    precombat->add_action("variable,name=sf_targets,value=4", "Starfall v Starsurge target cutoff");
    precombat->add_action("variable,name=sf_targets,op=add,value=1,if=azerite.arcanic_pulsar.enabled");
    precombat->add_action("variable,name=sf_targets,op=add,value=1,if=talent.starlord.enabled");
    precombat->add_action("variable,name=sf_targets,op=add,value=1,if=azerite.streaking_stars.rank>2&azerite.arcanic_pulsar.enabled");
    precombat->add_action("variable,name=sf_targets,op=sub,value=1,if=!talent.twin_moons.enabled");
  }
  
  // Forms
  if ( ( specialization() == DRUID_FERAL && primary_role() == ROLE_ATTACK ) ||
       ( specialization() == DRUID_GUARDIAN && catweave_bear && talent.feral_affinity -> ok() ) ||
       primary_role() == ROLE_ATTACK )
  {
    precombat -> add_action( this, "Cat Form" );
    precombat -> add_action( this, "Prowl" );
  }
  else if ( specialization() == DRUID_RESTORATION && role == ROLE_ATTACK )
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
  //else if (specialization() == DRUID_RESTORATION && (primary_role() == ROLE_DPS || primary_role() == ROLE_SPELL))
  //{
  //  precombat->add_action(this, "moonkin_form_affinity");
  //}

  // Snapshot stats
  precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  // Pre-Potion
  precombat -> add_action("potion");

  // Spec Specific Optimizations
  if ( specialization() == DRUID_BALANCE )
  {
    precombat -> add_action( this, "Solar Wrath" );
  }
  else if ( specialization() == DRUID_RESTORATION )
    precombat -> add_talent( this, "Cenarion Ward" );
  else if ( specialization() == DRUID_FERAL )
    precombat -> add_action("berserk");
    
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

  if ( primary_role() == ROLE_ATTACK )
  {
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
    def -> add_action( this, "Regrowth", "if=mana.pct>=30" );
  }
}

// Feral Combat Action Priority List ========================================

void druid_t::apl_feral()
{
   action_priority_list_t* def = get_action_priority_list("default");
   action_priority_list_t* opener = get_action_priority_list("opener");
   action_priority_list_t* cooldowns = get_action_priority_list("cooldowns");
   //action_priority_list_t* st = get_action_priority_list("single_target");
   action_priority_list_t* finisher = get_action_priority_list("finishers");
   action_priority_list_t* generator = get_action_priority_list("generators");

   def->add_action("auto_attack,if=!buff.prowl.up&!buff.shadowmeld.up");
   def->add_action("run_action_list,name=opener,if=variable.opener_done=0");
   def->add_action("cat_form,if=!buff.cat_form.up");
   def->add_action("rake,if=buff.prowl.up|buff.shadowmeld.up");
   def->add_action("call_action_list,name=cooldowns");
   def->add_action("ferocious_bite,target_if=dot.rip.ticking&dot.rip.remains<3&target.time_to_die>10&(talent.sabertooth.enabled)");
   def->add_action("regrowth,if=combo_points=5&buff.predatory_swiftness.up&talent.bloodtalons.enabled&buff.bloodtalons.down");
   def->add_action("run_action_list,name=finishers,if=combo_points>4");
   def->add_action("run_action_list,name=generators");

   opener->add_action(
       "tigers_fury",
       "The opener generally follow the logic of the rest of the apl, but is separated out here for logical clarity\n"
       "# We will open with TF, you can safely cast this from stealth without breaking it." );

   opener->add_action( "rake,if=!ticking|buff.prowl.up",
                       "Always open with rake, consuming stealth and one BT charge (if talented)" );
   opener->add_action( "variable,name=opener_done,value=dot.rip.ticking",
                       "Lets make sure we end the opener \"sequence\" when our first rip is ticking" );
   opener->add_action( "wait,sec=0.001,if=dot.rip.ticking", "Break out of the action list" );
   opener->add_action( "moonfire_cat,if=!ticking",
                       "If we have LI, and haven't applied it yet use moonfire." );
   opener->add_action( "rip,if=!ticking",
                       "no need to wait for 5 CPs anymore, just rip and we are up and running" );

   //cooldowns->add_action( "dash,if=!buff.cat_form.up" );
   //cooldowns->add_action("prowl,if=buff.incarnation.remains<0.5&buff.jungle_stalker.up");
   cooldowns->add_action("berserk,if=energy>=30&(cooldown.tigers_fury.remains>5|buff.tigers_fury.up)");
   cooldowns->add_action("tigers_fury,if=energy.deficit>=60");
   cooldowns->add_action("berserking");
   cooldowns->add_action("feral_frenzy,if=combo_points=0");
   cooldowns->add_action("incarnation,if=energy>=30&(cooldown.tigers_fury.remains>15|buff.tigers_fury.up)");
   cooldowns->add_action("potion,name=battle_potion_of_agility,if=target.time_to_die<65|(time_to_die<180&(buff.berserk.up|buff.incarnation.up))");
   cooldowns->add_action("shadowmeld,if=combo_points<5&energy>=action.rake.cost&dot.rake.pmultiplier<2.1&buff.tigers_fury.up&(buff.bloodtalons.up|!talent.bloodtalons.enabled)&(!talent.incarnation.enabled|cooldown.incarnation.remains>18)&!buff.incarnation.up");
   cooldowns->add_action("use_items");

   finisher->add_action("pool_resource,for_next=1");
   finisher->add_action("savage_roar,if=buff.savage_roar.down");
   finisher->add_action("pool_resource,for_next=1");
   finisher->add_action("primal_wrath,target_if=spell_targets.primal_wrath>1&dot.rip.remains<4");
   finisher->add_action("pool_resource,for_next=1");
   finisher->add_action("primal_wrath,target_if=spell_targets.primal_wrath>=2");
   finisher->add_action("pool_resource,for_next=1");
   finisher->add_action("rip,target_if=!ticking|(remains<=duration*0.3)&(!talent.sabertooth.enabled)|(remains<=duration*0.8&persistent_multiplier>dot.rip.pmultiplier)&target.time_to_die>8");
   finisher->add_action("pool_resource,for_next=1");
   finisher->add_action("savage_roar,if=buff.savage_roar.remains<12");
   finisher->add_action("pool_resource,for_next=1");
   finisher->add_action("maim,if=buff.iron_jaws.up");
   finisher->add_action("ferocious_bite,max_energy=1");

   generator->add_action("regrowth,if=talent.bloodtalons.enabled&buff.predatory_swiftness.up&buff.bloodtalons.down&combo_points=4&dot.rake.remains<4");
   generator->add_action("regrowth,if=talent.bloodtalons.enabled&buff.bloodtalons.down&buff.predatory_swiftness.up&talent.lunar_inspiration.enabled&dot.rake.remains<1");
   generator->add_action("brutal_slash,if=spell_targets.brutal_slash>desired_targets");
   generator->add_action("pool_resource,for_next=1");
   generator->add_action("thrash_cat,if=(refreshable)&(spell_targets.thrash_cat>2)");
   generator->add_action("pool_resource,for_next=1");
   generator->add_action("thrash_cat,if=(talent.scent_of_blood.enabled&buff.scent_of_blood.down)&spell_targets.thrash_cat>3");
   generator->add_action("pool_resource,for_next=1");
   generator->add_action("swipe_cat,if=buff.scent_of_blood.up");
   generator->add_action("pool_resource,for_next=1");
   generator->add_action("rake,target_if=!ticking|(!talent.bloodtalons.enabled&remains<duration*0.3)&target.time_to_die>4");
   generator->add_action("pool_resource,for_next=1");
   generator->add_action("rake,target_if=talent.bloodtalons.enabled&buff.bloodtalons.up&((remains<=7)&persistent_multiplier>dot.rake.pmultiplier*0.85)&target.time_to_die>4");
   generator->add_action("moonfire_cat,if=buff.bloodtalons.up&buff.predatory_swiftness.down&combo_points<5", "With LI & BT, we can use moonfire to save BT charges, allowing us to better refresh rake");
   generator->add_action("brutal_slash,if=(buff.tigers_fury.up&(raid_event.adds.in>(1+max_charges-charges_fractional)*recharge_time))");
   generator->add_action("moonfire_cat,target_if=refreshable");
   generator->add_action("pool_resource,for_next=1");
   generator->add_action("thrash_cat,if=refreshable&((variable.use_thrash=2&(!buff.incarnation.up|azerite.wild_fleshrending.enabled))|spell_targets.thrash_cat>1)");
   generator->add_action("thrash_cat,if=refreshable&variable.use_thrash=1&buff.clearcasting.react&(!buff.incarnation.up|azerite.wild_fleshrending.enabled)");
   generator->add_action("pool_resource,for_next=1");
   generator->add_action("swipe_cat,if=spell_targets.swipe_cat>1");
   generator->add_action("shred,if=dot.rake.remains>(action.shred.cost+action.rake.cost-energy)%energy.regen|buff.clearcasting.react");

 //  action_priority_list_t* def = get_action_priority_list("default");
 //  action_priority_list_t* opener = get_action_priority_list("opener");
 //  action_priority_list_t* finish = get_action_priority_list("finisher");
 //  action_priority_list_t* generate = get_action_priority_list("generator");
 //  action_priority_list_t* sbt = get_action_priority_list("sbt_opener");

 //  std::string              potion_action = "potion,name=";
 //  if (true_level > 100)
 //     potion_action += "old_war";
 //  else if (true_level > 90)
 //     potion_action += "draenic_agility";
 //  else
 //     potion_action += "tolvir";

 //  // Opener =================================================================
 //  opener->add_action(this, "Rake", "if=buff.prowl.up");
 //  opener->add_talent(this, "Savage Roar", "if=buff.savage_roar.down");
 //  opener->add_action(this, "Berserk", "if=buff.savage_roar.up&!equipped.draught_of_souls");
 //  opener->add_action(this, "Tiger's Fury", "if=buff.berserk.up&!equipped.draught_of_souls");
 //  opener->add_action(this, artifact.ashamanes_frenzy, "frenzy", "if=buff.bloodtalons.up&!equipped.draught_of_souls");
 //  opener->add_action(this, "Regrowth", "if=combo_points=5&buff.bloodtalons.down&buff.predatory_swiftness.up&!equipped.draught_of_souls");
 //  opener->add_action(this, "Rip", "if=combo_points=5&buff.bloodtalons.up&!equipped.draught_of_souls");
 //  if (sets->has_set_bonus(DRUID_FERAL, T19, B4))
 //     opener->add_action("thrash_cat,if=combo_points<5&!ticking&!equipped.draught_of_souls");
 //  opener->add_action(this, "Shred", "if=combo_points<5&buff.savage_roar.up&!equipped.draught_of_souls");

 //  // Main List ==============================================================

 //  def->add_action(this, "Dash", "if=!buff.cat_form.up");
 //  def->add_action(this, "Cat Form");
 //  def->add_action("call_action_list,name=opener,if=!dot.rip.ticking&time<15&talent.savage_roar.enabled&talent.jagged_wounds.enabled&talent.bloodtalons.enabled&desired_targets<=1");
 //  def->add_talent(this, "Wild Charge");
 //  def->add_talent(this, "Displacer Beast", "if=movement.distance>10");
 //  def->add_action(this, "Dash", "if=movement.distance&buff.displacer_beast.down&buff.wild_charge_movement.down");
 //  if (race == RACE_NIGHT_ELF)
 //     def->add_action(this, "Rake", "if=buff.prowl.up|buff.shadowmeld.up");
 //  else
 //     def->add_action(this, "Rake", "if=buff.prowl.up");
 //  def->add_action("auto_attack");
 //  def->add_action(this, "Skull Bash");
 //  def->add_action(this, "Berserk", "if=buff.tigers_fury.up");
 //  def->add_action("incarnation,if=cooldown.tigers_fury.remains<gcd");

 //  // On-Use Items
 //  //for ( size_t i = 0; i < items.size(); i++ )
 //  //{
 //  //  if ( items[i].has_use_special_effect() )
 //  //  {
 //  //    std::string line = std::string( "use_item,slot=" ) + items[i].slot_name();
 //  //    if (items[i].name_str == "mirror_of_the_blademaster")
 //  //       line += ",if=raid_event.adds.in>60|!raid_event.adds.exists|spell_targets.swipe_cat>desired_targets";
 //  //    else if (items[i].name_str == "ring_of_collapsing_futures")
 //  //       line += ",if=(buff.tigers_fury.up|target.time_to_die<45)";
 //  //    else if (items[i].name_str == "draught_of_souls")
 //  //       line += ",if=buff.tigers_fury.up&energy.time_to_max>3&(!talent.savage_roar.enabled|buff.savage_roar.up)";
 //  //    else if (items[i].slot == SLOT_WAIST) continue;
 //  //    else if (items[i].name_str != "maalus_the_blood_drinker") //NOTE: must be on the bottom of the if chain.
 //  //       line += ",if=(buff.tigers_fury.up&(target.time_to_die>trinket.stat.any.cooldown|target.time_to_die<45))|buff.incarnation.remains>20";
 //  //    def -> add_action( line );
 //  //  }
 //  //}
 //  if ( items[SLOT_FINGER_1].name_str == "ring_of_collapsing_futures" || items[SLOT_FINGER_2].name_str == "ring_of_collapsing_futures")
 //    def -> add_action( "use_item,name=ring_of_collapsing_futures,if=(buff.tigers_fury.up|target.time_to_die<45)" );
 //
 // def -> add_action( "use_items" );

 // if ( sim -> allow_potions && true_level >= 80 )
 //   def -> add_action( potion_action +
 //                      ",if=((buff.berserk.remains>10|buff.incarnation.remains>20)&(target.time_to_die<180|(trinket.proc.all.react&target.health.pct<25)))|target.time_to_die<=40" );

 // // Racials
 // if ( race == RACE_TROLL )
 //   def -> add_action( "berserking,if=buff.tigers_fury.up" );

 // def -> add_action( this, "Tiger's Fury",
 //                    "if=(!buff.clearcasting.react&energy.deficit>=60)|energy.deficit>=80|(t18_class_trinket&buff.berserk.up&buff.tigers_fury.down)" );
 // def -> add_action( "incarnation,if=energy.time_to_max>1&energy>=35" );
 // def -> add_action( this, "Ferocious Bite", "cycle_targets=1,if=dot.rip.ticking&dot.rip.remains<3&target.time_to_die>3&(target.health.pct<25|talent.sabertooth.enabled)",
 //                    "Keep Rip from falling off during execute range." );
 // def -> add_action( this, "Regrowth",
 //                    "if=talent.bloodtalons.enabled&buff.predatory_swiftness.up&buff.bloodtalons.down&(combo_points>=5|buff.predatory_swiftness.remains<1.5"
 //                    "|(talent.bloodtalons.enabled&combo_points=2&cooldown.ashamanes_frenzy.remains<gcd)|"
 //                    "(talent.elunes_guidance.enabled&((cooldown.elunes_guidance.remains<gcd&combo_points=0)|(buff.elunes_guidance.up&combo_points>=4))))",
 //                    "Use Healing Touch at 5 Combo Points, if Predatory Swiftness is about to fall off, at 2 Combo Points before Ashamane's Frenzy, "
 //                    "before Elune's Guidance is cast or before the Elune's Guidance buff gives you a 5th Combo Point." );
 // def->add_action(this, "Regrowth",
 //                   "if=talent.bloodtalons.enabled&buff.predatory_swiftness.up&buff.bloodtalons.down&combo_points=4&dot.rake.remains<4");
 // def -> add_action( "call_action_list,name=sbt_opener,if=talent.sabertooth.enabled&time<20" );
 // def -> add_action( this, "Regrowth", "if=equipped.ailuro_pouncers&talent.bloodtalons.enabled&(buff.predatory_swiftness.stack>2|(buff.predatory_swiftness.stack>1&dot.rake.remains<3))&buff.bloodtalons.down",
 //   "Special logic for Ailuro Pouncers legendary." );
 // def -> add_action( "call_action_list,name=finisher" );
 // def -> add_action( "call_action_list,name=generator" );
 // if (items[SLOT_TRINKET_1].name_str == "draught_of_souls" || items[SLOT_TRINKET_2].name_str == "draught_of_souls")
 //    def->add_action("use_item,name=draught_of_souls");
 // def -> add_action( "wait,sec=1,if=energy.time_to_max>3", "The Following line massively increases performance of the simulation but can be safely removed." );


 // // Finishers
 // finish -> add_action( "pool_resource,for_next=1",
 //   "Use Savage Roar if it's expired and you're at 5 combo points or are about to use Brutal Slash" );
 // finish -> add_talent( this, "Savage Roar",
 //   "if=!buff.savage_roar.up&(combo_points=5|(talent.brutal_slash.enabled&spell_targets.brutal_slash>desired_targets&action.brutal_slash.charges>0))" );
 // finish -> add_action( "pool_resource,for_next=1",
 //   "Thrash has higher priority than finishers at 5 targets" );
 // finish -> add_action( "thrash_cat,cycle_targets=1,if=remains<=duration*0.3&spell_targets.thrash_cat>=5" );
 // finish -> add_action( "pool_resource,for_next=1",
 //   "Replace Rip with Swipe at 8 targets" );
 // finish -> add_action( "swipe_cat,if=spell_targets.swipe_cat>=8" );
 // finish -> add_action( this, "Rip", "cycle_targets=1,if=(!ticking|(remains<8&target.health.pct>25&!talent.sabertooth.enabled)|"
 //   "persistent_multiplier>dot.rip.pmultiplier)&target.time_to_die-remains>tick_time*4&combo_points=5&"
 //   "(energy.time_to_max<variable.pooling|buff.berserk.up|buff.incarnation.up|buff.elunes_guidance.up|cooldown.tigers_fury.remains<3|"
 //   "set_bonus.tier18_4pc|(buff.clearcasting.react&energy>65)|talent.soul_of_the_forest.enabled|!dot.rip.ticking|(dot.rake.remains<1.5&spell_targets.swipe_cat<6))",
 //   "Refresh Rip at 8 seconds or for a stronger Rip" );
 // finish -> add_talent( this, "Savage Roar", "if=((buff.savage_roar.remains<=10.5&talent.jagged_wounds.enabled)|(buff.savage_roar.remains<=7.2))&"
 //   "combo_points=5&(energy.time_to_max<variable.pooling|buff.berserk.up|buff.incarnation.up|buff.elunes_guidance.up|cooldown.tigers_fury.remains<3|"
 //   "set_bonus.tier18_4pc|(buff.clearcasting.react&energy>65)|talent.soul_of_the_forest.enabled|!dot.rip.ticking|(dot.rake.remains<1.5&spell_targets.swipe_cat<6))",
 //   "Refresh Savage Roar early with Jagged Wounds" );
 // finish -> add_action( "swipe_cat,if=combo_points=5&(spell_targets.swipe_cat>=6|(spell_targets.swipe_cat>=3&!talent.bloodtalons.enabled))&"
 //   "combo_points=5&(energy.time_to_max<variable.pooling|buff.berserk.up|buff.incarnation.up|buff.elunes_guidance.up|cooldown.tigers_fury.remains<3|"
 //   "set_bonus.tier18_4pc|(talent.moment_of_clarity.enabled&buff.clearcasting.react))",
 //   "Replace FB with Swipe at 6 targets for Bloodtalons or 3 targets otherwise." );
 // finish->add_action(this, "Maim", ",if=combo_points=5&buff.fiery_red_maimers.up&"
 //       "(energy.time_to_max<variable.pooling|buff.berserk.up|buff.incarnation.up|buff.elunes_guidance.up|cooldown.tigers_fury.remains<3)");
 // finish -> add_action( this, "Ferocious Bite", "max_energy=1,cycle_targets=1,if=combo_points=5&"
 //   "(energy.time_to_max<variable.pooling|buff.berserk.up|buff.incarnation.up|buff.elunes_guidance.up|cooldown.tigers_fury.remains<3)" );

 // // Generators
 // generate -> add_talent( this, "Brutal Slash", "if=spell_targets.brutal_slash>desired_targets&combo_points<5",
 //   "Brutal Slash if there's adds up" );
 // generate -> add_action( this, artifact.ashamanes_frenzy, "ashamanes_frenzy",
 //   "if=combo_points<=2&buff.elunes_guidance.down&(buff.bloodtalons.up|!talent.bloodtalons.enabled)&(buff.savage_roar.up|!talent.savage_roar.enabled)" );
 // generate -> add_action( "pool_resource,if=talent.elunes_guidance.enabled&combo_points=0&energy<action.ferocious_bite.cost+25-energy.regen*cooldown.elunes_guidance.remains",
 //   "Pool energy for Elune's Guidance when it's coming off cooldown." );
 // generate -> add_talent( this, "Elune's Guidance", "if=talent.elunes_guidance.enabled&combo_points=0&energy>=action.ferocious_bite.cost+25" );
 // generate -> add_action( "pool_resource,for_next=1",
 //   "Spam Thrash over Rake or Moonfire at 9 targets with Brutal Slash talent." );
 // generate -> add_action( "thrash_cat,if=talent.brutal_slash.enabled&spell_targets.thrash_cat>=9" );
 // generate -> add_action( "pool_resource,for_next=1",
 //   "Use Swipe over Rake or Moonfire at 6 targets." );
 // generate -> add_action( "swipe_cat,if=spell_targets.swipe_cat>=6" );
 // if ( race == RACE_NIGHT_ELF )
 //   generate -> add_action( "shadowmeld,if=combo_points<5&energy>=action.rake.cost&dot.rake.pmultiplier<2.1&buff.tigers_fury.up&"
 //   "(buff.bloodtalons.up|!talent.bloodtalons.enabled)&(!talent.incarnation.enabled|cooldown.incarnation.remains>18)&!buff.incarnation.up",
 //   "Shadowmeld to buff Rake" );
 // generate -> add_action( "pool_resource,for_next=1", "Refresh Rake early with Bloodtalons" );
 // generate -> add_action( this, "Rake", "cycle_targets=1,if=combo_points<5&(!ticking|(!talent.bloodtalons.enabled&remains<duration*0.3)|"
 //   "(talent.bloodtalons.enabled&buff.bloodtalons.up&(remains<=variable.rake_refresh)&"
 //   "persistent_multiplier>dot.rake.pmultiplier*0.80))&target.time_to_die-remains>tick_time" );
 // generate -> add_action( "moonfire_cat,cycle_targets=1,if=combo_points<5&remains<=4.2&target.time_to_die-remains>tick_time*2" );
 // generate -> add_action( "pool_resource,for_next=1" );
 ///* if ( sets -> has_set_bonus( DRUID_FERAL, T19, B4 ) )
 //    generate->add_action( "thrash_cat,cycle_targets=1,if=remains<=duration*0.3&(spell_targets.swipe_cat>=2|(buff.clearcasting.up&buff.bloodtalons.down))", "Use Thrash on Clearcasting, or during Berserk/Incarnation if you have T19 4set" );
 // else
 //    generate -> add_action( "thrash_cat,cycle_targets=1,if=remains<=duration*0.3&spell_targets.swipe_cat>=2" );*/
 // generate -> add_action( "thrash_cat,cycle_targets=1,if=set_bonus.tier19_4pc&remains<=duration*0.3&combo_points<5&(spell_targets.swipe_cat>=2|((equipped.luffa_wrappings|buff.clearcasting.up)&(equipped.ailuro_pouncers|buff.bloodtalons.down)))", "Use Thrash on single-target if you have T19 4set" );
 // generate -> add_action( "thrash_cat,cycle_targets=1,if=!set_bonus.tier19_4pc&remains<=duration*0.3&(spell_targets.swipe_cat>=2|(buff.clearcasting.up&equipped.luffa_wrappings&(equipped.ailuro_pouncers|buff.bloodtalons.down)))" );
 // generate -> add_talent( this, "Brutal Slash",
 //   "if=combo_points<5&((raid_event.adds.exists&raid_event.adds.in>(1+max_charges-charges_fractional)*15)|(!raid_event.adds.exists&(charges_fractional>2.66&time>10)))",
 //   "Brutal Slash if you would cap out charges before the next adds spawn" );
 // generate -> add_action( "swipe_cat,if=combo_points<5&spell_targets.swipe_cat>=3" );
 // generate -> add_action( this, "Shred", "if=combo_points<5&(spell_targets.swipe_cat<3|talent.brutal_slash.enabled)" );

 // // Sabertooth Opener
 // sbt -> add_action( this, "Regrowth", "if=talent.bloodtalons.enabled&combo_points=5&!buff.bloodtalons.up&!dot.rip.ticking",
 //   "Hard-cast a Healing Touch for Bloodtalons buff. Use Dash to re-enter Cat Form." );
 // sbt -> add_action( this, "Tiger's Fury", "if=!dot.rip.ticking&combo_points=5",
 //   "Force use of Tiger's Fury before applying Rip." );
}

// Balance Combat Action Priority List ======================================

void druid_t::apl_balance()
{
  std::vector<std::string> racial_actions = get_racial_actions();
  std::vector<std::string> item_actions = get_item_actions();

  action_priority_list_t* default_list = get_action_priority_list("default");

  if (sim->allow_potions && true_level >= 80)
  {
    default_list->add_action("potion,if=buff.ca_inc.remains>6&active_enemies=1");
    default_list->add_action("potion,name=battle_potion_of_intellect,if=buff.ca_inc.remains>6");
  }

  for (size_t i = 0; i < racial_actions.size(); i++)
    default_list->add_action(racial_actions[i] + ",if=buff.ca_inc.up");

  // CDs
  default_list->add_action("use_item,name=balefire_branch,if=equipped.159630&cooldown.ca_inc.remains>30", "CDs");
  default_list->add_action("use_item,name=dread_gladiators_badge,if=equipped.161902&cooldown.ca_inc.remains>30");
  default_list->add_action("use_item,name=azurethos_singed_plumage,if=equipped.161377&cooldown.ca_inc.remains>30");
  default_list->add_action("use_item,name=tidestorm_codex,if=equipped.165576");
  default_list->add_action("use_items,if=cooldown.ca_inc.remains>30");
  default_list->add_talent(this, "Warrior of Elune", "");
  default_list->add_action(this, "Innervate", "if=azerite.lively_spirit.enabled&(cooldown.incarnation.remains<2|cooldown.celestial_alignment.remains<12)");
  default_list->add_action("incarnation,if=dot.sunfire.remains>8&dot.moonfire.remains>12&(dot.stellar_flare.remains>6|!talent.stellar_flare.enabled)&ap_check&!buff.ca_inc.up");
  default_list->add_action(this, "Celestial Alignment",
                            "if=astral_power>=40&!buff.ca_inc.up&ap_check&(!azerite.lively_spirit.enabled|buff.lively_spirit.up)&(dot."
                            "sunfire.remains>2&dot.moonfire.ticking&(dot.stellar_flare.ticking|!talent.stellar_flare."
                            "enabled))" );
  default_list->add_talent(this, "Fury of Elune", "if=(buff.ca_inc.up|cooldown.ca_inc.remains>30)&solar_wrath.ap_check");
  default_list->add_talent(this, "Force of Nature", "if=(buff.ca_inc.up|cooldown.ca_inc.remains>30)&ap_check");
  // Spenders
  default_list->add_action("cancel_buff,name=starlord,if=buff.starlord.remains<8&!solar_wrath.ap_check", "Spenders");
  default_list->add_action(this, "Starfall", "if="
                                    "(buff.starlord.stack<3|buff.starlord.remains>=8)"
                                    "&spell_targets>=variable.sf_targets"
                                    "&(target.time_to_die+1)*spell_targets>cost%2.5");
  default_list->add_action(this, "Starsurge",
                            "if=(talent.starlord.enabled&(buff.starlord.stack<3|buff.starlord.remains>=8&buff.arcanic_"
                            "pulsar.stack<8)|!talent.starlord.enabled&(buff.arcanic_pulsar.stack<8|buff.ca_inc.up))&"
                            "spell_targets.starfall<variable.sf_targets&buff.lunar_empowerment.stack+buff.solar_"
                            "empowerment.stack<4&buff.solar_empowerment.stack<3&buff.lunar_empowerment.stack<3&(!"
                            "variable.az_ss|!buff.ca_inc.up|!prev.starsurge)|target.time_to_die<=execute_time*astral_"
                            "power%40|!solar_wrath.ap_check" );
  // DoTs - for ttd calculations see https://docs.google.com/spreadsheets/d/16NyCGvWcXXwERuiSNlVhdD347jA5iWh-ELs33GtW1XQ/
  default_list->add_action(this, "Sunfire", "if=buff.ca_inc.up&buff.ca_inc.remains<gcd.max&variable.az_ss&dot.moonfire.remains>remains" );
  default_list->add_action(this, "Moonfire", "if=buff.ca_inc.up&buff.ca_inc.remains<gcd.max&variable.az_ss" );
  default_list->add_action(this, "Sunfire", "target_if=refreshable,if=ap_check"
                                    "&floor(target.time_to_die%(2*spell_haste))*spell_targets>=ceil(floor(2%spell_targets)*1.5)+2*spell_targets"
                                    "&(spell_targets>1+talent.twin_moons.enabled|dot.moonfire.ticking)"
                                    "&(!variable.az_ss|!buff.ca_inc.up|!prev.sunfire)&(buff.ca_inc.remains>remains|!buff.ca_inc.up)", "DoTs");
  default_list->add_action(this, "Moonfire", "target_if=refreshable,if=ap_check"
                                    "&floor(target.time_to_die%(2*spell_haste))*spell_targets>=6"
                                    "&(!variable.az_ss|!buff.ca_inc.up|!prev.moonfire)&(buff.ca_inc.remains>remains|!buff.ca_inc.up)");
  default_list->add_talent(this, "Stellar Flare", "target_if=refreshable,if=ap_check"
                                    "&floor(target.time_to_die%(2*spell_haste))>=5"
                                    "&(!variable.az_ss|!buff.ca_inc.up|!prev.stellar_flare)");
  // Generators
  default_list->add_action(this, "New Moon", "if=ap_check", "Generators");
  default_list->add_action(this, "Half Moon", "if=ap_check");
  default_list->add_action(this, "Full Moon", "if=ap_check");
  default_list->add_action(this, "Lunar Strike",
                            "if=buff.solar_empowerment.stack<3&(ap_check|buff.lunar_empowerment.stack=3)&((buff."
                            "warrior_of_elune.up|buff.lunar_empowerment.up|spell_targets>=2&!buff.solar_empowerment.up)"
                            "&(!variable.az_ss|!buff.ca_inc.up)|variable.az_ss&buff.ca_inc.up&prev.solar_wrath)" );
  default_list->add_action(this, "Solar Wrath", "if="
                                    "variable.az_ss<3|!buff.ca_inc.up|!prev.solar_wrath");
  // Fallthru for movement
  default_list->add_action(this, "Sunfire", "", "Fallthru for movement");
}

// Guardian Combat Action Priority List =====================================

void druid_t::apl_guardian()
{
  action_priority_list_t* default_list    = get_action_priority_list( "default" );
  action_priority_list_t* cooldowns       = get_action_priority_list( "cooldowns" );

  std::vector<std::string> item_actions   = get_item_actions();
  std::vector<std::string> racial_actions = get_racial_actions();

  if ( sim -> allow_potions )
    cooldowns -> add_action( "potion" );
  // Racials don't currently work

  for (size_t i = 0; i < racial_actions.size(); i++)
    cooldowns -> add_action( racial_actions[i] );
  cooldowns -> add_action( this, "Barkskin", "if=buff.bear_form.up" );
  cooldowns -> add_talent( this, "Lunar Beam", "if=buff.bear_form.up" );
  cooldowns -> add_talent( this, "Bristling Fur", "if=buff.bear_form.up" );
  cooldowns -> add_action( "use_items" );

  if ( catweave_bear && talent.feral_affinity -> ok() )
  {
    action_priority_list_t* bear = get_action_priority_list( "bear" );
    action_priority_list_t* cat  = get_action_priority_list( "cat" );

    default_list -> add_action( this, "Rake", "if=buff.prowl.up&buff.cat_form.up" );
    default_list -> add_action( "auto_attack" );
    default_list -> add_action( "call_action_list,name=cooldowns" );
    default_list -> add_action( "call_action_list,name=cat,if=talent.feral_affinity.enabled&((cooldown.thrash_bear.remains>0&cooldown.mangle.remains>0&rage<45&buff.incarnation.down&buff.galactic_guardian.down)|(buff.cat_form.up&energy>20)|(dot.rip.ticking&dot.rip.remains<3&target.health.pct<25))" );
    default_list -> add_action( "call_action_list,name=bear" );

    bear -> add_action( this, "Bear Form" );
    bear -> add_action( this, "Maul", "if=rage.deficit<10&active_enemies<4" );
    bear -> add_action( this, "Ironfur", "if=cost=0|(rage>cost&azerite.layered_mane.enabled&active_enemies>2)" );
    bear -> add_talent( this, "Pulverize", "target_if=dot.thrash_bear.stack=dot.thrash_bear.max_stacks" );
    bear -> add_action( this, "Moonfire", "target_if=dot.moonfire.refreshable&active_enemies<2" );
    bear -> add_action( "incarnation" );
    bear -> add_action( "thrash,if=(buff.incarnation.down&active_enemies>1)|(buff.incarnation.up&active_enemies>4)" );
    bear -> add_action( "swipe,if=buff.incarnation.down&active_enemies>4" );
    bear -> add_action( this, "Mangle", "if=dot.thrash_bear.ticking" );
    bear -> add_action( this, "Moonfire", "target_if=buff.galactic_guardian.up&active_enemies<2" );
    bear -> add_action( "thrash" );
    bear -> add_action( this, "Maul" );
    bear -> add_action( this, "Moonfire", "if=azerite.power_of_the_moon.rank>1&active_enemies=1", "Fill with Moonfire with PotMx2" );
    bear -> add_action( "swipe" );

    cat -> add_action( this, "Cat Form" );
    cat -> add_action( "rip,if=dot.rip.refreshable&combo_points=5" );
    cat -> add_action( "ferocious_bite,if=combo_points=5" );
    cat -> add_action( "rake,if=dot.rake.refreshable&combo_points<5" );
    cat -> add_action( this, "Shred" );
  } else {
    default_list -> add_action( "auto_attack" );
    default_list -> add_action( "call_action_list,name=cooldowns" );
    default_list -> add_action( this, "Maul", "if=rage.deficit<10&active_enemies<4" );
    default_list -> add_action( this, "Ironfur", "if=cost=0|(rage>cost&azerite.layered_mane.enabled&active_enemies>2)" );
    default_list -> add_talent( this, "Pulverize", "target_if=dot.thrash_bear.stack=dot.thrash_bear.max_stacks" );
    default_list -> add_action( this, "Moonfire", "target_if=dot.moonfire.refreshable&active_enemies<2" );
    default_list -> add_action( "incarnation" );
    default_list -> add_action( "thrash,if=(buff.incarnation.down&active_enemies>1)|(buff.incarnation.up&active_enemies>4)" );
    default_list -> add_action( "swipe,if=buff.incarnation.down&active_enemies>4" );
    default_list -> add_action( this, "Mangle", "if=dot.thrash_bear.ticking" );
    default_list -> add_action( this, "Moonfire", "target_if=buff.galactic_guardian.up&active_enemies<2" );
    default_list -> add_action( "thrash" );
    default_list -> add_action( this, "Maul" );
    default_list -> add_action( this, "Moonfire", "if=azerite.power_of_the_moon.rank>1&active_enemies=1", "Fill with Moonfire with PotMx2" );
    default_list -> add_action( "swipe" );
  }
}

// Restoration Combat Action Priority List ==================================

void druid_t::apl_restoration()
{
  action_priority_list_t* default_list    = get_action_priority_list( "default" );
  //action_priority_list_t* heal = get_action_priority_list("heal");
  //action_priority_list_t* dps = get_action_priority_list("dps"); //Base DPS APL - Guardian affinity
  // action_priority_list_t* BAFF = get_action_priority_list("baff"); //Balance affinity
  // action_priority_list_t* FAFF = get_action_priority_list("faff"); //Feral affinity

  std::vector<std::string> item_actions       = get_item_actions();
  std::vector<std::string> racial_actions     = get_racial_actions();

  for ( size_t i = 0; i < racial_actions.size(); i++ )
    default_list -> add_action( racial_actions[i] );
  for ( size_t i = 0; i < item_actions.size(); i++ )
    default_list -> add_action( item_actions[i] );

  default_list->add_action("rake,if=buff.shadowmeld.up|buff.prowl.up");
  default_list->add_action("auto_attack");
  default_list->add_action("moonfire,if=refreshable|(prev_gcd.1.sunfire&remains<duration*0.5)");
  default_list->add_action("sunfire,if=refreshable|(prev_gcd.1.moonfire&remains<duration*0.5)");
  default_list->add_action("cat_form,if=!buff.cat_form.up&energy>50");
  default_list->add_action("ferocious_bite,if=(combo_points>3&target.time_to_die<3)|(combo_points=5&energy>=50&dot.rip.remains>14)");
  default_list->add_action("swipe_cat,if=spell_targets.swipe_cat>=6");
  default_list->add_action("rip,if=refreshable&combo_points=5");
  default_list->add_action("ferocious_bite,max_energy=1,if=combo_points=5&energy.time_to_max<2");
  default_list->add_action("rake,if=refreshable");
  default_list->add_action("swipe_cat,if=spell_targets.swipe_cat>=2");
  default_list->add_action("shred");
  default_list->add_action("solar_wrath");

  //default_list -> add_action("swap_action_list,name=dps,if=role.attack");
  //default_list->add_action("call_action_list,name=dps");
  //default_list -> add_action("call_action_list,name=heal,if=role.heal");

  //heal -> add_action( this, "Healing Touch", "if=buff.clearcasting.up" );
  //heal -> add_action( this, "Rejuvenation", "if=remains<=duration*0.3" );
  //heal -> add_action( this, "Lifebloom", "if=debuff.lifebloom.down" );
  //heal -> add_action( this, "Swiftmend" );
  //heal -> add_action( this, "Healing Touch" );

  //dps -> add_action(this, "Moonfire", "if=refreshable");
  //dps -> add_action(this, "Sunfire", "if=refreshable");
  //dps -> add_action(this, "Cat Form");
  //dps -> add_action(this, "Rip", "if=refreshable");
  //dps -> add_action(this, "Rake", "if=refreshable");
  //dps -> add_action(this, "Shred");
  //dps -> add_action(this, "Bear Form");
  //dps -> add_action ("swipe_bear");

}

// druid_t::init_scaling ====================================================

void druid_t::init_scaling()
{
  player_t::init_scaling();

  equipped_weapon_dps = main_hand_weapon.damage / main_hand_weapon.swing_time.total_seconds();

  scaling -> disable( STAT_STRENGTH );

  // Save a copy of the weapon
  caster_form_weapon = main_hand_weapon;

  // Bear/Cat form weapons need to be scaled up if we are calculating scale factors for the weapon
  // dps. The actual cached cat/bear form weapons are created before init_scaling is called, so the
  // adjusted values for the "main hand weapon" have not yet been added.
  if ( sim -> scaling -> scale_stat == STAT_WEAPON_DPS )
  {
    if ( cat_weapon.damage > 0 )
    {
      auto coeff = sim -> scaling -> scale_value * cat_weapon.swing_time.total_seconds();
      cat_weapon.damage  += coeff;
      cat_weapon.min_dmg += coeff;
      cat_weapon.max_dmg += coeff;
      cat_weapon.dps     += sim -> scaling -> scale_value;
    }

    if ( bear_weapon.damage > 0 )
    {
      auto coeff = sim -> scaling -> scale_value * bear_weapon.swing_time.total_seconds();
      bear_weapon.damage  += coeff;
      bear_weapon.min_dmg += coeff;
      bear_weapon.max_dmg += coeff;
      bear_weapon.dps     += sim -> scaling -> scale_value;
    }
  }
}

// druid_t::init ============================================================

void druid_t::init()
{
  player_t::init();

 // if ( specialization() == DRUID_RESTORATION )
  // sim -> errorf( "%s is using an unsupported spec.", name() );
}

// druid_t::init_gains ======================================================

void druid_t::init_gains()
{
  player_t::init_gains();

  // Multiple Specs / Forms
  gain.clearcasting          = get_gain( "clearcasting" );       // Feral & Restoration
  gain.soul_of_the_forest    = get_gain( "soul_of_the_forest" ); // Feral & Guardian

  // Balance
  gain.natures_balance       = get_gain("natures_balance");
  gain.force_of_nature       = get_gain("force_of_nature");
  gain.fury_of_elune         = get_gain("fury_of_elune");
  gain.stellar_flare         = get_gain("stellar_flare");
  gain.lunar_strike          = get_gain( "lunar_strike"          );
  gain.moonfire              = get_gain( "moonfire"              );
  gain.shooting_stars        = get_gain( "shooting_stars"        );
  gain.solar_wrath           = get_gain( "solar_wrath"           );
  gain.sunfire               = get_gain( "sunfire" );
  gain.celestial_alignment   = get_gain( "celestial_alignment" );
  gain.incarnation           = get_gain( "incarnation" );


  // Feral 
  gain.brutal_slash          = get_gain( "brutal_slash"          );
  gain.energy_refund         = get_gain( "energy_refund"         );
  gain.feral_frenzy          = get_gain( "feral_frenzy"          );
  gain.primal_fury           = get_gain( "primal_fury"           );
  gain.rake                  = get_gain( "rake"                  );
  gain.shred                 = get_gain( "shred"                 );
  gain.swipe_cat             = get_gain( "swipe_cat"             );
  gain.tigers_fury           = get_gain( "tigers_fury"           );

  // Guardian
  gain.bear_form             = get_gain( "bear_form"             );
  gain.blood_frenzy          = get_gain( "blood_frenzy"          );
  gain.brambles              = get_gain( "brambles"              );
  gain.bristling_fur         = get_gain( "bristling_fur"         );
  gain.galactic_guardian     = get_gain( "galactic_guardian"     );
  gain.gore                  = get_gain( "gore"                  );
  gain.rage_refund           = get_gain( "rage_refund"           );
  gain.stalwart_guardian     = get_gain( "stalwart_guardian"     );
  gain.rage_from_melees      = get_gain( "rage_from_melees"      );

  // Set Bonuses
  gain.feral_tier17_2pc      = get_gain( "feral_tier17_2pc"      );
  gain.feral_tier18_4pc      = get_gain( "feral_tier18_4pc"      );
  gain.feral_tier19_2pc      = get_gain( "feral_tier19_2pc"      );
  gain.guardian_tier17_2pc   = get_gain( "guardian_tier17_2pc"   );
  gain.guardian_tier18_2pc   = get_gain( "guardian_tier18_2pc"   );

  // Legendaries
  gain.oakhearts_puny_quods  = get_gain( "oakhearts_puny_quods"  );
}

// druid_t::init_procs ======================================================

void druid_t::init_procs()
{
  player_t::init_procs();

  proc.clearcasting             = get_proc( "clearcasting"           );
  proc.clearcasting_wasted      = get_proc( "clearcasting_wasted"    );
  proc.gore                     = get_proc( "gore"                   );
  proc.gushing_lacerations      = get_proc( "gushing_lacerations"    );
  proc.blood_mist = get_proc("blood_mist");
  proc.the_wildshapers_clutch   = get_proc( "the_wildshapers_clutch" );
  proc.predator                 = get_proc( "predator"               );
  proc.predator_wasted          = get_proc( "predator_wasted"        );
  proc.primal_fury              = get_proc( "primal_fury"            );
  proc.starshards               = get_proc( "Starshards"             );
  proc.tier17_2pc_melee         = get_proc( "tier17_2pc_melee"       );
  proc.power_of_the_moon = get_proc("power_of_the_moon");
  proc.streaking_stars_gain = get_proc("streaking_stars_gain");
  proc.streaking_stars_loss = get_proc("streaking_stars_loss");
}

// druid_t::init_resources ==================================================

void druid_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[ RESOURCE_RAGE ] = 0;
  resources.current[ RESOURCE_COMBO_POINT ] = 0;
  resources.current[ RESOURCE_ASTRAL_POWER ] = std::max(talent.natures_balance->ok() ? 58.0 : 0.0, initial_astral_power);
  expected_max_health = calculate_expected_max_health();
}

// druid_t::init_rng ========================================================

void druid_t::init_rng()
{
  // RPPM objects
  rppm.predator           = get_rppm( "predator", predator_rppm_rate );  // Predator: optional RPPM approximation.
  rppm.blood_mist         = get_rppm( "blood_mist", find_spell(azerite.blood_mist.spell()->effectN(1).trigger_spell_id())->real_ppm() ) ;
  rppm.power_of_the_moon = get_rppm("power_of_the_moon", azerite.power_of_the_moon.spell()->effectN(1).trigger()->real_ppm());

  player_t::init_rng();
}


// druid_t::init_actions ====================================================

void druid_t::init_action_list()
{
#ifdef NDEBUG // Only restrict on release builds.
  // Restoration isn't fully supported atm
  if ( specialization() == DRUID_RESTORATION && role != ROLE_ATTACK)
  {
    if ( ! quiet )
      sim -> errorf( "Druid restoration healing for player %s is not currently supported.", name() );

    quiet = true;
    return;
  }
#endif
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

  // Reset druid_t variables to their original state.
  form = NO_FORM;
  moon_stage = ( moon_stage_e ) initial_moon_stage;
  previous_streaking_stars = SS_NONE;
  previous_streaking_stars_cast = SS_NONE;

  base_gcd = timespan_t::from_seconds( 1.5 );

  // Restore main hand attack / weapon to normal state
  main_hand_attack = caster_melee_attack;
  main_hand_weapon = caster_form_weapon;

  // Reset any custom events to be safe.
  persistent_buff_delay.clear();

  if ( mastery.natures_guardian -> ok() )
    recalculate_resource_max( RESOURCE_HEALTH );
}

// druid_t::merge ===========================================================

void druid_t::merge( player_t& other )
{
  player_t::merge( other );

  druid_t& od = static_cast<druid_t&>( other );

  for ( size_t i = 0, end = counters.size(); i < end; i++ )
    counters[ i ] -> merge( *od.counters[ i ] );
}

// druid_t::mana_regen_per_second ===========================================

double druid_t::resource_regen_per_second( resource_e r ) const
{
  double reg = player_t::resource_regen_per_second( r );

  if ( r == RESOURCE_MANA )
  {
    if ( buff.moonkin_form -> check() )
      reg *= 1.0 + buff.moonkin_form -> data().effectN( 5 ).percent() + ( 1 / cache.spell_haste() );
  }

  if (r == RESOURCE_ENERGY)
  {
    if (buff.savage_roar->check() && buff.savage_roar->data().effectN(3).subtype() == A_MOD_POWER_REGEN_PERCENT)
      reg *= 1.0 + buff.savage_roar->data().effectN(3).percent();
  }

  return reg;
}

// druid_t::available =======================================================

timespan_t druid_t::available() const
{
  if ( primary_resource() != RESOURCE_ENERGY )
    return timespan_t::from_seconds( 0.1 );

  double energy = resources.current[ RESOURCE_ENERGY ];

  if ( energy > 25 ) return timespan_t::from_seconds( 0.1 );

  return std::max(
           timespan_t::from_seconds( ( 25 - energy ) / resource_regen_per_second( RESOURCE_ENERGY ) ),
           timespan_t::from_seconds( 0.1 )
         );
}

// druid_t::arise ===========================================================

void druid_t::arise()
{
  player_t::arise();

  if ( talent.earthwarden -> ok() )
    buff.earthwarden -> trigger( buff.earthwarden -> max_stack() );

  if (talent.natures_balance->ok())
    buff.natures_balance->trigger();

  // Trigger persistent buffs
  if ( buff.yseras_gift )
    persistent_buff_delay.push_back( make_event<persistent_buff_delay_event_t>( *sim, this, buff.yseras_gift ) );

  if ( talent.earthwarden -> ok() )
    persistent_buff_delay.push_back( make_event<persistent_buff_delay_event_t>( *sim, this, buff.earthwarden_driver ) );

  if ( legendary.ailuro_pouncers > timespan_t::zero() )
  {
    timespan_t preproc = legendary.ailuro_pouncers * rng().real();
    if ( preproc < buff.predatory_swiftness -> buff_duration )
    {
      buff.predatory_swiftness -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0,
        buff.predatory_swiftness -> buff_duration - preproc );
    }

    make_event<ailuro_pouncers_event_t>( *sim, this, timespan_t::from_seconds( 15 ) - preproc );
  }
}

// druid_t::recalculate_resource_max ========================================

void druid_t::recalculate_resource_max( resource_e rt )
{
  double pct_health = 0, current_health = 0;
  bool adjust_natures_guardian_health = mastery.natures_guardian -> ok() && rt == RESOURCE_HEALTH;
  if ( adjust_natures_guardian_health )
  {
    current_health = resources.current[ rt ];
    pct_health = resources.pct( rt );
  }

  player_t::recalculate_resource_max( rt );

  if ( adjust_natures_guardian_health )
  {
    resources.max[ rt ] *= 1.0 + cache.mastery_value();
    // Maintain current health pct.
    resources.current[ rt ] = resources.max[ rt ] * pct_health;

    if ( sim -> log )
      sim -> out_log.printf( "%s recalculates maximum health. old_current=%.0f new_current=%.0f net_health=%.0f",
                             name(), current_health, resources.current[ rt ], resources.current[ rt ] - current_health );
  }
}

// druid_t::invalidate_cache ================================================

void druid_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
  case CACHE_ATTACK_POWER:
    if ( specialization() == DRUID_GUARDIAN || specialization() == DRUID_FERAL )
      invalidate_cache( CACHE_SPELL_POWER );
    break;
  case CACHE_SPELL_POWER:
    if ( specialization() == DRUID_BALANCE || specialization() == DRUID_RESTORATION )
      invalidate_cache( CACHE_ATTACK_POWER );
    break;
  case CACHE_MASTERY:
    if ( mastery.natures_guardian -> ok() )
    {
      invalidate_cache( CACHE_ATTACK_POWER );
      recalculate_resource_max( RESOURCE_HEALTH );
    }
    break;
  case CACHE_CRIT_CHANCE:
    if ( specialization() == DRUID_GUARDIAN )
      invalidate_cache( CACHE_DODGE );
    break;
  case CACHE_AGILITY:
    if ( buff.ironfur -> check() )
      invalidate_cache( CACHE_ARMOR );
    break;
  default:
    break;
  }
}

// druid_t::composite_melee_attack_power ===============================

double druid_t::composite_melee_attack_power() const
{
  // In 8.0 Killer Instinct is gone, replaced with modifiers in balance/resto auras.
  if ( specialization() == DRUID_BALANCE )
  {
    return spec.balance -> effectN( 9 ).percent() * cache.spell_power( SCHOOL_MAX ) *
      composite_spell_power_multiplier();
  }

  if ( specialization() == DRUID_RESTORATION )
  {
    return spec.restoration -> effectN( 10 ).percent() * cache.spell_power( SCHOOL_MAX ) *
      composite_spell_power_multiplier();
  }

  return player_t::composite_melee_attack_power();
}

// druid_t::composite_attack_power_multiplier ===============================

double druid_t::composite_attack_power_multiplier() const
{

  if ( specialization() == DRUID_BALANCE || specialization() == DRUID_RESTORATION )
  {
    return 1.0;
  }

  double ap = player_t::composite_attack_power_multiplier();

  // All modifiers MUST invalidate CACHE_ATTACK_POWER or Nurturing Instinct will break.

  if ( mastery.natures_guardian -> ok() )
    ap *= 1.0 + cache.mastery() * mastery.natures_guardian_AP -> effectN( 1 ).mastery_value();

  return ap;
}

// druid_t::composite_armor =================================================

double druid_t::composite_armor() const
{
  double a = player_t::composite_armor();

  if ( buff.ironfur -> up() )
  {
    a += ( buff.ironfur -> check_stack_value() * cache.agility() );
  }

  return a;
}

// druid_t::composite_armor_multiplier ======================================

double druid_t::composite_armor_multiplier() const
{
  double a = player_t::composite_armor_multiplier();

  if ( buff.bear_form -> check() )
  {
    a *= 1.0 + buff.bear_form -> data().effectN( 3 ).percent();
    a *= 1.0 + buff.incarnation_bear -> data().effectN( 5 ).percent();
  }

  return a;
}

// druid_t::composite_player_multiplier =====================================

double druid_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  m *= 1.0 + legendary.LegendaryDamageMod;

  // Fury of Nature increases Arcane and Nature damage
  if ( buff.bear_form -> check() && ( dbc::is_school( school, SCHOOL_ARCANE ) || dbc::is_school( school, SCHOOL_NATURE ) ) )
    m *= 1.0 + legendary.fury_of_nature;

  return m;
}

// druid_t::composite_rating_multiplier ====================================

double druid_t::composite_rating_multiplier( rating_e rating ) const
{
  double rm = player_t::composite_rating_multiplier( rating );

  return rm;
}

// druid_t::composite_melee_expertise( weapon_t* ) ==========================

double druid_t::composite_melee_expertise( const weapon_t* ) const
{
  double exp = player_t::composite_melee_expertise();

  if ( buff.bear_form -> check() )
    exp += buff.bear_form -> data().effectN( 6 ).base_value();

  return exp;
}

// druid_t::composite_melee_crit_chance ============================================

double druid_t::composite_melee_crit_chance() const
{
  double crit = player_t::composite_melee_crit_chance();

  crit += spec.critical_strikes -> effectN( 1 ).percent();

  return crit;
}

// druid_t::temporary_movement_modifier =====================================

double druid_t::temporary_movement_modifier() const
{
  double active = player_t::temporary_movement_modifier();

  if ( buff.dash -> up() && buff.cat_form -> check() )
    active = std::max( active, buff.dash -> check_value() );
  
  if ( buff.wild_charge_movement -> check() )
    active = std::max( active, buff.wild_charge_movement -> check_value() );

  if ( buff.tiger_dash -> up() && buff.cat_form -> check() )
    active = std::max( active, buff.tiger_dash -> check_value() );
    
  return active;
}

// druid_t::passive_movement_modifier =======================================

double druid_t::passive_movement_modifier() const
{
  double ms = player_t::passive_movement_modifier();

  if ( buff.cat_form -> up() )
    ms += spec.cat_form_speed -> effectN( 1 ).percent();

  ms += spec.feline_swiftness -> effectN( 1 ).percent();

  return ms;
}

// druid_t::composite_spell_crit_chance ============================================

double druid_t::composite_spell_crit_chance() const
{
  double crit = player_t::composite_spell_crit_chance();

  crit += spec.critical_strikes -> effectN( 1 ).percent();

  return crit;
}

// druid_t::composite_spell_haste ===========================================

double druid_t::composite_spell_haste() const
{
  double sh = player_t::composite_spell_haste();

  sh *= 1.0 / (1.0 + buff.astral_acceleration->stack_value());

  sh *= 1.0 / (1.0 + buff.starlord->stack_value());

  if (buff.celestial_alignment->check())
      sh /= (1.0 + spec.celestial_alignment->effectN(3).percent());

  if (buff.incarnation_moonkin->check())
      sh /= (1.0 + talent.incarnation_moonkin->effectN(3).percent());

  if ( buff.sephuzs_secret -> check() )
     sh *= 1.0 / ( 1.0 + buff.sephuzs_secret -> stack_value() );

  if ( legendary.sephuzs_secret -> ok() )
     sh *= 1.0 / ( 1.0 + 0.02 ); //Todo(feral): fix spelldata hook.

  return sh;
}

// druid_t::composite_meele_haste ===========================================

double druid_t::composite_melee_haste() const
{
   double mh = player_t::composite_melee_haste();

   if ( buff.sephuzs_secret -> check() )
      mh *= 1.0 / ( 1.0 + buff.sephuzs_secret -> stack_value() );

   if ( legendary.sephuzs_secret -> ok() )
      mh *= 1.0 / ( 1.0 + 0.02 ); //Todo(feral): Fix spelldata hook.

   return mh;
}

// druid_t::composite_spell_power ===========================================

double druid_t::composite_spell_power( school_e school ) const
{
  double sp = 0.0;

  // In 8.0 Nurturing Instinct is gone, replaced with modifiers in feral/guardian auras.
  double ap_coeff = 0.0;

  if ( specialization() == DRUID_GUARDIAN ) { ap_coeff = spec.guardian -> effectN( 11 ).percent(); }
  if ( specialization() == DRUID_FERAL    ) { ap_coeff = spec.feral    -> effectN( 11 ).percent(); }

  if ( ap_coeff > 0 )
  {
    double weapon_sp = 0.0;

    if ( buff.cat_form -> check() ) {
      weapon_sp = cat_weapon.dps * WEAPON_POWER_COEFFICIENT;
    } else if ( buff.bear_form -> check() ) {
      weapon_sp = bear_weapon.dps * WEAPON_POWER_COEFFICIENT;
    } else {
      weapon_sp = main_hand_weapon.dps * WEAPON_POWER_COEFFICIENT;
    }

    sp += composite_attack_power_multiplier() * ( cache.attack_power() + weapon_sp ) * ap_coeff;
  }

  sp += player_t::composite_spell_power( school );

  return sp;
}

// druid_t::composite_spell_power_multiplier ================================

double druid_t::composite_spell_power_multiplier() const
{
  if ( specialization() == DRUID_GUARDIAN || specialization() == DRUID_FERAL)
  {
    return 1.0;
  }

  return player_t::composite_spell_power_multiplier();
}


// druid_t::composite_attribute =============================================

double druid_t::composite_attribute( attribute_e attr ) const
{
  double a = player_t::composite_attribute( attr );

  switch ( attr )
  {
    case ATTR_AGILITY:
      if ( buff.ironfur -> up() && azerite.layered_mane.ok() )
      a += azerite.layered_mane.value( 1 ) * buff.ironfur -> stack();
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
    if ( buff.bear_form -> check() )
      m *= 1.0 + spec.bear_form -> effectN( 2 ).percent();
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

// druid_t::composite_crit_avoidance ========================================

double druid_t::composite_crit_avoidance() const
{
  double c = player_t::composite_crit_avoidance();

  if ( buff.bear_form -> check() )
    c += buff.bear_form -> data().effectN( 2 ).percent();

  return c;
}

// druid_t::composite_dodge =================================================

double druid_t::composite_dodge() const
{
  double d = player_t::composite_dodge();

  return d;
}

// druid_t::composite_dodge_rating ==========================================

double druid_t::composite_dodge_rating() const
{
  double dr = player_t::composite_dodge_rating();

  // FIXME: Spell dataify me.
  if ( specialization() == DRUID_GUARDIAN )
  {
    dr += composite_rating( RATING_MELEE_CRIT );
  }

  return dr;
}

// druid_t::composite_damage_versatility_rating ==========================================

double druid_t::composite_damage_versatility_rating() const
{
  double cdvr = player_t::composite_damage_versatility_rating();

  if ( ahhhhh_the_great_outdoors )
    cdvr += sylvan_walker;

  return cdvr;
}

// druid_t::composite_heal_versatility_rating ==========================================

double druid_t::composite_heal_versatility_rating() const
{
  double chvr = player_t::composite_heal_versatility_rating();

  if ( ahhhhh_the_great_outdoors )
    chvr += sylvan_walker;

  return chvr;
}

// druid_t::composite_mitigation_versatility_rating ==========================================

double druid_t::composite_mitigation_versatility_rating() const
{
  double cmvr = player_t::composite_mitigation_versatility_rating();

  if ( ahhhhh_the_great_outdoors )
    cmvr += sylvan_walker;

  return cmvr;
}

// druid_t::composite_leech =================================================

double druid_t::composite_leech() const
{
  double l = player_t::composite_leech();

  return l;
}

// druid_t::create_expression ===============================================

expr_t* druid_t::create_expression( const std::string& name_str )
{
  std::vector<std::string> splits = util::string_split( name_str, "." );

  if ( splits[ 0 ] == "druid" && (splits[ 2 ] == "ticks_gained_on_refresh" | splits[2] == "ticks_gained_on_refresh_pmultiplier" ))
  {
    // Since we know some action names don't map to the actual dot portion, lets add some exceptions
    // this may have to be made more robust if other specs are interested in using it, but for now lets
    // default any ambiguity to what would make most sense for ferals.
    if ( splits[ 1 ] == "rake" )
      splits[ 1 ] = "rake_bleed";
    if ( specialization() == DRUID_FERAL && splits[ 1 ] == "moonfire" )
      splits[ 1 ] = "lunar_inspiration";

    bool pmult_adjusted = false;

    if (splits[2] == "ticks_gained_on_refresh_pmultiplier") pmult_adjusted = true;

    action_t* action = find_action( splits[ 1 ] );
    if ( action )
      return make_fn_expr( name_str, [action, pmult_adjusted]() -> double {
        dot_t* dot             = action->get_dot();
        double remaining_ticks = 0;
        double potential_ticks = 0;
        action_state_t* state  = action->get_state( dot->state );
        timespan_t duration    = action->composite_dot_duration( state );
        state->target = action->target = action->player->target;
        timespan_t ttd         = action->target->time_to_percent( 0 );
        double pmult = 0;
        //action->snapshot_state(state, DMG_OVER_TIME);

        if ( dot->is_ticking() )
        {
          remaining_ticks = std::min( dot->remains(), ttd ) / dot->current_action->tick_time( dot->state );
          duration        = action->calculate_dot_refresh_duration( dot, duration );
          remaining_ticks *= pmult_adjusted ? dot->state->persistent_multiplier : 1.0;
        }

        if (pmult_adjusted)
        {
          action->snapshot_state(state, RESULT_TYPE_NONE);
          state->target = action->target;

          pmult = action->composite_persistent_multiplier(state);
        }

        potential_ticks = std::min( duration, ttd ) / ( action->tick_time(state) );
        potential_ticks *= pmult_adjusted ? pmult : 1.0;
        return potential_ticks - remaining_ticks;
      } );
    throw std::invalid_argument( "invalid action" );
  }

  if ( splits[ 0 ] == "action" && splits[ 1 ] == "ferocious_bite_max" && splits[ 2 ] == "damage" )
  {
    action_t* action = find_action( "ferocious_bite" );

    return make_fn_expr( name_str, [action, this]() -> double {
      action_state_t* state = action->get_state();
      state->n_targets      = 1;
      state->chain_target   = 0;
      state->result         = RESULT_HIT;

      action->snapshot_state( state, DMG_DIRECT );
      state->target = action->target;
    //  (p()->resources.current[RESOURCE_ENERGY] - cat_attack_t::cost()));

    //combo_points = p()->resources.current[RESOURCE_COMBO_POINT];
      double current_energy = this->resources.current[RESOURCE_ENERGY];
      double current_cp = this->resources.current[RESOURCE_COMBO_POINT];
      this->resources.current[RESOURCE_ENERGY] = 50;
      this->resources.current[RESOURCE_COMBO_POINT] = 5;

      double amount;
      state->result_amount = action->calculate_direct_amount(state);
      state->target->target_mitigation( action->get_school(), DMG_DIRECT, state );
      amount = state->result_amount;
      amount *= 1.0 + clamp( state->crit_chance + state->target_crit_chance, 0.0, 1.0 ) *
                          action->composite_player_critical_multiplier( state );

      this->resources.current[RESOURCE_ENERGY] = current_energy;
      this->resources.current[RESOURCE_COMBO_POINT] = current_cp;

      action_state_t::release(state);
      return amount;
    } );
  }

  if ( util::str_compare_ci( name_str, "combo_points" ) )
  {
    return make_ref_expr( "combo_points", resources.current[ RESOURCE_COMBO_POINT ] );
  }

  if (specialization() == DRUID_BALANCE)
  {
    if (util::str_compare_ci( name_str, "astral_power"))
    {
      return make_ref_expr(name_str, resources.current[RESOURCE_ASTRAL_POWER]);
    }
    
    // New Moon stage related expressions
    else if (util::str_compare_ci(name_str, "new_moon"))
    {
      return make_fn_expr(name_str, [this]() { return moon_stage == NEW_MOON; });
    }
    else if (util::str_compare_ci(name_str, "half_moon"))
    {
      return make_fn_expr(name_str, [this]() { return moon_stage == HALF_MOON; });
    }
    else if (util::str_compare_ci(name_str, "full_moon"))
    {
      return make_fn_expr(name_str, [this]() { return moon_stage == FULL_MOON || moon_stage == FREE_FULL_MOON; });
    }
    else if (util::str_compare_ci(name_str, "free_full_moon"))
    {
      return make_fn_expr(name_str, [this]() { return moon_stage == FREE_FULL_MOON; });
    }

    // automatic resolution of Celestial Alignment vs talented Incarnation
    else if (splits.size() == 3 && util::str_compare_ci(splits[1], "ca_inc"))
    {
      if (util::str_compare_ci(splits[0], "cooldown"))
        splits[1] = talent.incarnation_moonkin->ok() ? ".incarnation." : ".celestial_alignment.";
      else
        splits[1] = talent.incarnation_moonkin->ok() ? ".incarnation_chosen_of_elune." : ".celestial_alignment.";
      return player_t::create_expression(splits[0] + splits[1] + splits[2]);
    }

    // check for AP overcap on action other than current action. check for current action handled in druid_spell_t::create_expression
    // syntax: <action>.ap_check.<allowed overcap = 0>
    else if (splits.size() >= 2 && util::str_compare_ci(splits[1], "ap_check"))
    {
      action_t* action = find_action(splits[0]);
      if (action)
      {
        return make_fn_expr(name_str, [action, this, splits]() {
          action_state_t* state = action->get_state();
          double ap = this->resources.current[RESOURCE_ASTRAL_POWER];
          ap += action->composite_energize_amount(state);
          ap += this->talent.shooting_stars->ok() ? this->spec.shooting_stars_dmg->effectN(2).base_value() / 10 : 0;
          ap += this->talent.natures_balance->ok() ? std::ceil(action->time_to_execute / timespan_t::from_seconds(1.5)) : 0;
          action_state_t::release(state);
          return ap <= this->resources.base[RESOURCE_ASTRAL_POWER] + (splits.size() >= 3 ? std::stoi(splits[2]) : 0);
        });
      }
      throw std::invalid_argument("invalid action");
    }
  }

  // Convert talent.incarnation.* & buff.incarnation.* to spec-based incarnations. cooldown.incarnation.* doesn't need name conversion.
  if (splits.size() == 3 && util::str_compare_ci(splits[1], "incarnation") && (util::str_compare_ci(splits[0], "buff") || util::str_compare_ci(splits[0], "talent")))
  {
    switch(specialization())
    {
      case DRUID_BALANCE: {
        splits[1] = ".incarnation_chosen_of_elune.";
      } break;
      case DRUID_FERAL: {
        splits[1] = ".incarnation_king_of_the_jungle.";
      } break;
      case DRUID_GUARDIAN: {
        splits[1] = ".incarnation_guardian_of_ursoc.";
      } break;
      case DRUID_RESTORATION: {
        splits[1] = ".incarnation_tree_of_life.";
      } break;
      default: break;
    }

    return player_t::create_expression(splits[0] + splits[1] + splits[2]);
  }

  return player_t::create_expression( name_str );
}

// druid_t::create_options ==================================================

void druid_t::create_options()
{
  player_t::create_options();

  add_option( opt_float ( "predator_rppm", predator_rppm_rate ) );
  add_option( opt_bool  ( "feral_t21_2pc", t21_2pc) );
  add_option( opt_bool  ( "feral_t21_4pc", t21_4pc) );
  add_option( opt_float ( "initial_astral_power", initial_astral_power ) );
  add_option( opt_int   ( "initial_moon_stage", initial_moon_stage ) );
  add_option( opt_int   ( "lively_spirit_stacks", lively_spirit_stacks));
  add_option( opt_bool  ( "outside", ahhhhh_the_great_outdoors ) );
  add_option( opt_bool  ( "catweave_bear", catweave_bear ) );
}

// druid_t::create_profile ==================================================

std::string druid_t::create_profile( save_e type )
{
  return player_t::create_profile( type );
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

// druid_t::convert_hybrid_stat =============================================

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
  default:
    return s;
  }
}

// druid_t::primary_resource ================================================

resource_e druid_t::primary_resource() const
{
  if ( specialization() == DRUID_BALANCE && primary_role() == ROLE_SPELL )
    return RESOURCE_ASTRAL_POWER;

  if ( primary_role() == ROLE_HEAL || primary_role() == ROLE_SPELL )
    return RESOURCE_MANA;

  if ( primary_role() == ROLE_TANK )
    return RESOURCE_RAGE;

  return RESOURCE_ENERGY;
}

// druid_t::init_absorb_priority ============================================

void druid_t::init_absorb_priority()
{
  player_t::init_absorb_priority();

  absorb_priority.push_back( 184878 ); // Stalwart Guardian
  absorb_priority.push_back( talent.brambles -> id() ); // Brambles
  absorb_priority.push_back( talent.earthwarden -> id() ); // Earthwarden - Legion TOCHECK
}

// druid_t::target_mitigation ===============================================

void druid_t::target_mitigation( school_e school, dmg_e type, action_state_t* s )
{
  s -> result_amount *= 1.0 + buff.barkskin -> value();

  s -> result_amount *= 1.0 + buff.survival_instincts -> value();

  s -> result_amount *= 1.0 + buff.pulverize -> value();

  if ( spec.thick_hide )
    s -> result_amount *= 1.0 + spec.thick_hide -> effectN( 1 ).percent();

  if ( talent.rend_and_tear -> ok() )
    s -> result_amount *= 1.0 - talent.rend_and_tear -> effectN( 2 ).percent()
      * get_target_data( s -> action -> player ) -> dots.thrash_bear -> current_stack();

  player_t::target_mitigation( school, type, s );
}

// druid_t::assess_damage ===================================================

void druid_t::assess_damage( school_e school,
                             dmg_e    dtype,
                             action_state_t* s )
{
  if ( dbc::is_school( school, SCHOOL_PHYSICAL ) && dtype == DMG_DIRECT &&
    s -> result == RESULT_HIT )
  {
    buff.ironfur -> up();
  }

  player_t::assess_damage( school, dtype, s );
}

// druid_t::assess_damage_imminent_pre_absorb ===============================

// Trigger effects based on being hit or taking damage.

void druid_t::assess_damage_imminent_pre_absorb( school_e school, dmg_e dmg, action_state_t* s )
{
  player_t::assess_damage_imminent_pre_absorb( school, dmg, s);

  if ( action_t::result_is_hit( s -> result ) && s -> result_amount > 0 )
  {

    // Guardian rage from melees
    if ( specialization() == DRUID_GUARDIAN && !s -> action -> special && cooldown.rage_from_melees -> up())
    {
      resource_gain( RESOURCE_RAGE,
                     spec.bear_form -> effectN( 3 ).base_value(),
                     gain.rage_from_melees );
      cooldown.rage_from_melees -> start( cooldown.rage_from_melees -> duration );
    }

    if (specialization() == DRUID_BALANCE && buff.moonkin_form->check())
      buff.owlkin_frenzy->trigger();

    if ( buff.cenarion_ward -> up() )
      active.cenarion_ward_hot -> execute();

    if ( buff.bristling_fur -> up() )
    {
      // 1 rage per 1% of maximum health taken
      resource_gain( RESOURCE_RAGE,
                     s -> result_amount / expected_max_health * 100,
                     gain.bristling_fur );
    }
  }
}

// druid_t::assess_heal =====================================================

void druid_t::assess_heal( school_e school,
                           dmg_e    dmg_type,
                           action_state_t* s )
{
  if ( sets -> has_set_bonus( DRUID_GUARDIAN, T18, B2 ) && buff.ironfur -> check() )
  {
    double pct = sets -> set( DRUID_GUARDIAN, T18, B2 ) -> effectN( 1 ).percent();

    // Trigger a gain so we can track how much the set bonus helped.
    // The gain is 100% overflow so it doesn't distort charts.
    gain.guardian_tier18_2pc -> add( RESOURCE_HEALTH, 0, s -> result_total * pct );

    s -> result_total *= 1.0 + pct;
  }

  player_t::assess_heal( school, dmg_type, s );

  trigger_natures_guardian( s );
}

// druid_t::shapeshift ======================================================

void druid_t::shapeshift( form_e f )
{
  if ( get_form() == f )
    return;

  buff.cat_form     -> expire();
  buff.bear_form    -> expire();
  buff.moonkin_form -> expire();
  buff.moonkin_form_affinity->expire();

  switch ( f )
  {
  case CAT_FORM:
    buff.cat_form -> trigger();
    break;
  case BEAR_FORM:
    buff.bear_form -> trigger();
    break;
  case MOONKIN_FORM:
    buff.moonkin_form -> trigger();
    break;
  case MOONKIN_FORM_AFFINITY:
    buff.moonkin_form_affinity -> trigger();
    break;
  case NO_FORM:
     break;
  default:
    assert( 0 );
    break;
  }

  form = f;
}

// druid_t::get_target_data =================================================

druid_td_t* druid_t::get_target_data( player_t* target ) const
{
  assert( target );
  druid_td_t*& td = target_data[ target ];
  if ( ! td )
  {
    td = new druid_td_t( *target, const_cast<druid_t&>( *this ) );
  }
  return td;
}

// druid_t::init_beast_weapon ===============================================

void druid_t::init_beast_weapon( weapon_t& w, double swing_time )
{
  // use main hand weapon as base
  w = main_hand_weapon;

  if ( w.type == WEAPON_NONE ) {
    // if main hand weapon is empty, use unarmed damage
    // unarmed base beast weapon damage range is 1-1
    // Jul 25 2018
    w.min_dmg = w.max_dmg = w.damage  = 1;
  } else {
    // Otherwise normalize the main hand weapon's damage to the beast weapon's speed.
    double normalizing_factor = swing_time /  w.swing_time.total_seconds();
    w.min_dmg *= normalizing_factor;
    w.max_dmg *= normalizing_factor;
    w.damage *= normalizing_factor;
  }

  w.type = WEAPON_BEAST;
  w.school = SCHOOL_PHYSICAL;
  w.swing_time = timespan_t::from_seconds( swing_time );
}

// druid_t::get_affinity_spec ===============================================
specialization_e druid_t::get_affinity_spec() const
{
  if ( talent.balance_affinity -> ok() )
  {
    return DRUID_BALANCE;
  }

  if ( talent.restoration_affinity -> ok() )
  {
    return DRUID_RESTORATION;
  }

  if ( talent.feral_affinity -> ok() )
  {
    return DRUID_FERAL;
  }

  if ( talent.guardian_affinity -> ok() )
  {
    return DRUID_GUARDIAN;
  }

  return SPEC_NONE;
}

// druid_t::find_affinity_spell =============================================

const spell_data_t* druid_t::find_affinity_spell( const std::string& name ) const
{
  const spell_data_t* spec_spell = find_specialization_spell( name );

  if ( spec_spell->found() )
  {
    return spec_spell;
  }

  spec_spell = find_spell( dbc.specialization_ability_id( get_affinity_spec(), util::inverse_tokenize( name ).c_str() ) );

  if ( spec_spell->found() )
  {
    return spec_spell;
  }

  return spell_data_t::not_found();
}

// druid_t::trigger_natures_guardian ========================================

void druid_t::trigger_natures_guardian( const action_state_t* trigger_state )
{
  if ( ! mastery.natures_guardian -> ok() )
    return;
  if ( trigger_state -> result_total <= 0 )
    return;
  if ( trigger_state -> action == active.natures_guardian ||
       trigger_state -> action == active.yseras_gift || 
       trigger_state -> action -> id == 22842 ) // Frenzied Regeneration
    return;

  active.natures_guardian -> base_dd_min = active.natures_guardian -> base_dd_max =
    trigger_state -> result_total * cache.mastery_value();
  action_state_t* s = active.natures_guardian -> get_state();
  s -> target = this;
  active.natures_guardian -> snapshot_state( s, HEAL_DIRECT );
  active.natures_guardian -> schedule_execute( s );
}

//druid_t::trigger_solar_empowerment

void druid_t::trigger_solar_empowerment (const action_state_t* state)
{
  double dm = buff.solar_empowerment->data().effectN(1).percent();

  dm += mastery.starlight->ok() * cache.mastery()*mastery.starlight->effectN(4).mastery_value();

  if(talent.soul_of_the_forest->ok())
    dm *= (1.0 + talent.soul_of_the_forest->effectN (1).percent ());
  dm = floor(dm*100)/100; //Currently Solar Wrath Empowerment is rounded down to the next %

  double amount = state->result_amount * dm;
  active.solar_empowerment->base_dd_min = amount;
  active.solar_empowerment->base_dd_max = amount;
  active.solar_empowerment->set_target (state->target);
  active.solar_empowerment->execute ();
}

// druid_t::calculate_expected_max_health ===================================

double druid_t::calculate_expected_max_health() const
{
  double slot_weights = 0;
  double prop_values = 0;

  for ( size_t i = 0; i < items.size(); i++ )
  {
    const item_t* item = &items[ i ];
    if ( ! item || item -> slot == SLOT_SHIRT || item -> slot == SLOT_RANGED ||
      item -> slot == SLOT_TABARD || item -> item_level() <= 0 )
    {
      continue;
    }

    const random_prop_data_t item_data = dbc.random_property( item -> item_level() );
    int index = item_database::random_suffix_type( &item -> parsed.data );
    if ( item_data.p_epic[ 0 ] == 0 )
    {
      continue;
    }

    slot_weights += item_data.p_epic[ index ] / item_data.p_epic[ 0 ];

    if ( ! item -> active() )
    {
      continue;
    }

    prop_values += item_data.p_epic[ index ];
  }

  double expected_health = ( prop_values / slot_weights ) * 8.318556;
  expected_health += base.stats.attribute[ STAT_STAMINA ];
  expected_health *= 1.0 + matching_gear_multiplier( ATTR_STAMINA );
  expected_health *= 1.0 + spec.bear_form -> effectN( 2 ).percent();
  expected_health *= current.health_per_stamina;

  return expected_health;
}

const spelleffect_data_t* druid_t::query_aura_effect( const spell_data_t* aura_spell, effect_type_t type,
                                                      effect_subtype_t subtype, double misc_value,
                                                      const spell_data_t* affected_spell )
{ 
  for ( size_t i = 1; i <= aura_spell->effect_count(); i++ )
  {
    const spelleffect_data_t& effect = aura_spell->effectN( i );

    if ( affected_spell != spell_data_t::nil() && !affected_spell->affected_by( effect ) )
      return spelleffect_data_t::nil();

    if ( effect.type() == type && effect.subtype() == subtype )
    {
      if ( misc_value != 0 )
      {
        if ( effect.misc_value1() == misc_value )
          return &effect;
      }
      else
      {
        return &effect;
      }
    }
  }

  return spelleffect_data_t::nil();
}

druid_td_t::druid_td_t( player_t& target, druid_t& source )
  : actor_target_data_t( &target, &source ),
    dots( dots_t() ),
    buff( buffs_t() ),
    debuff( debuffs_t() )
{
  dots.fury_of_elune    = target.get_dot( "fury_of_elune",    &source );
  dots.gushing_wound    = target.get_dot( "gushing_wound",    &source );
  dots.lifebloom        = target.get_dot( "lifebloom",        &source );
  dots.moonfire         = target.get_dot( "moonfire",         &source );
  dots.stellar_flare    = target.get_dot( "stellar_flare",    &source );
  dots.rake             = target.get_dot( "rake",             &source );
  dots.regrowth         = target.get_dot( "regrowth",         &source );
  dots.rejuvenation     = target.get_dot( "rejuvenation",     &source );
  dots.rip              = target.get_dot( "rip",              &source );
  dots.sunfire          = target.get_dot( "sunfire",          &source );
  dots.starfall         = target.get_dot( "starfall",         &source );
  dots.thrash_bear      = target.get_dot( "thrash_bear",      &source );
  dots.thrash_cat       = target.get_dot( "thrash_cat",       &source );
  dots.wild_growth      = target.get_dot( "wild_growth",      &source );

  buff.lifebloom = buff_creator_t( *this, "lifebloom", source.find_class_spell( "Lifebloom" ) );

  debuff.bloodletting        = buff_creator_t( *this, "bloodletting", source.find_spell( 165699 ) )
                               .default_value( source.find_spell( 165699 ) -> effectN( 1 ).percent() );
}

// Copypasta for reporting
bool has_amount_results( const std::array<stats_t::stats_results_t, FULLTYPE_MAX>& res )
{
  return (
           res[ FULLTYPE_HIT ].actual_amount.mean() > 0 ||
           res[ FULLTYPE_CRIT ].actual_amount.mean() > 0
         );
}

// druid_t::copy_from =====================================================

void druid_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  druid_t* p = debug_cast<druid_t*>( source );

  predator_rppm_rate = p -> predator_rppm_rate;
  initial_astral_power = p -> initial_astral_power;
  initial_moon_stage = p -> initial_moon_stage;
  lively_spirit_stacks = p->lively_spirit_stacks;
  t21_2pc = p -> t21_2pc;
  t21_4pc = p -> t21_4pc;
  ahhhhh_the_great_outdoors = p -> ahhhhh_the_great_outdoors;
}
void druid_t::output_json_report(js::JsonOutput& /*root*/) const
{
   return; //NYI.
   if ( specialization() != DRUID_FERAL ) return;

   /* snapshot_stats:
   *    savage_roar:
   *        [ name: Shred Exec: 15% Benefit: 98% ]
   *        [ name: Rip Exec: 88% Benefit: 35% ]
   *    bloodtalons:
   *        [ name: Rip Exec: 99% Benefit: 99% ]
   *    tigers_fury:
   */
   for (size_t i = 0, end = stats_list.size(); i < end; i++)
   {
      stats_t* stats = stats_list[i];
      double tf_exe_up = 0, tf_exe_total = 0;
      double tf_benefit_up = 0, tf_benefit_total = 0;
      double sr_exe_up = 0, sr_exe_total = 0;
      double sr_benefit_up = 0, sr_benefit_total = 0;
      double bt_exe_up = 0, bt_exe_total = 0;
      double bt_benefit_up = 0, bt_benefit_total = 0;
      //int n = 0;

      for (size_t j = 0, end2 = stats->action_list.size(); j < end2; j++)
      {
         cat_attacks::cat_attack_t* a = dynamic_cast<cat_attacks::cat_attack_t*>(stats->action_list[j]);
         if (!a)
            continue;

         if (!a->consumes_bloodtalons)
            continue;

         tf_exe_up += a->tf_counter->mean_exe_up();
         tf_exe_total += a->tf_counter->mean_exe_total();
         tf_benefit_up += a->tf_counter->mean_tick_up();
         tf_benefit_total += a->tf_counter->mean_tick_total();
         if (has_amount_results(stats->direct_results))
         {
            tf_benefit_up += a->tf_counter->mean_exe_up();
            tf_benefit_total += a->tf_counter->mean_exe_total();
         }

         if (talent.savage_roar->ok())
         {
            sr_exe_up += a->sr_counter->mean_exe_up();
            sr_exe_total += a->sr_counter->mean_exe_total();
            sr_benefit_up += a->sr_counter->mean_tick_up();
            sr_benefit_total += a->sr_counter->mean_tick_total();
            if (has_amount_results(stats->direct_results))
            {
               sr_benefit_up += a->sr_counter->mean_exe_up();
               sr_benefit_total += a->sr_counter->mean_exe_total();
            }
         }
         if (talent.bloodtalons->ok())
         {
            bt_exe_up += a->bt_counter->mean_exe_up();
            bt_exe_total += a->bt_counter->mean_exe_total();
            bt_benefit_up += a->bt_counter->mean_tick_up();
            bt_benefit_total += a->bt_counter->mean_tick_total();
            if (has_amount_results(stats->direct_results))
            {
               bt_benefit_up += a->bt_counter->mean_exe_up();
               bt_benefit_total += a->bt_counter->mean_exe_total();
            }
         }

         if (tf_exe_total > 0 || bt_exe_total > 0 || sr_exe_total > 0)
         {
            //auto snapshot = root["snapshot_stats"].add();
            if (talent.savage_roar->ok())
            {
               //auto sr = snapshot["savage_roar"].make_array();





            }
            if (talent.bloodtalons->ok())
            {

            }



         }

      }

   }



}

//void druid_t::output_json_report(js::JsonOutput& root) const
//{
//   if (specialization() == DRUID_FERAL)
//   {
//      auto snapshot = root["snapshot_stats"].add();
//
//      if ( talent.savage_roar -> ok() )
//      {
//         snapshot["savage_roar"].make_array();
//         *(void*)0 == 0;
//      }
//
//   }
//}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class druid_report_t : public player_report_extension_t
{
public:
  druid_report_t( druid_t& player ) :
    p( player )
  { }

  virtual void html_customsection(report::sc_html_stream& os) override
  {
     if (p.specialization() == DRUID_FERAL)
     {
        feral_snapshot_table(os);
     }
  }

  void feral_snapshot_table( report::sc_html_stream& os )
  {
    // Write header
     os << "<table class=\"sc\">\n"
        << "<tr>\n"
        << "<th >Ability</th>\n"
        << "<th colspan=2>Tiger's Fury</th>\n";
    if( p.talent.savage_roar -> ok() )
    {
      os << "<th colspan=2>Savage Roar</th>\n";
    }
    if ( p.talent.bloodtalons -> ok() )
    {
      os << "<th colspan=2>Bloodtalons</th>\n";
    }
    os << "</tr>\n";

    os << "<tr>\n"
       << "<th>Name</th>\n"
       << "<th>Execute %</th>\n"
       << "<th>Benefit %</th>\n";
    if ( p.talent.savage_roar -> ok() )
    {
       os << "<th>Execute %</th>\n"
          << "<th>Benefit %</th>\n";
    }

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
      double sr_exe_up = 0, sr_exe_total = 0;
      double sr_benefit_up = 0, sr_benefit_total = 0;
      double bt_exe_up = 0, bt_exe_total = 0;
      double bt_benefit_up = 0, bt_benefit_total = 0;
      int n = 0;

      for (size_t j = 0, end2 = stats->action_list.size(); j < end2; j++)
      {
         cat_attacks::cat_attack_t* a = dynamic_cast<cat_attacks::cat_attack_t*>(stats->action_list[j]);
         if (!a)
            continue;

         if (!a->consumes_bloodtalons)
            continue;

         tf_exe_up += a->tf_counter->mean_exe_up();
         tf_exe_total += a->tf_counter->mean_exe_total();
         tf_benefit_up += a->tf_counter->mean_tick_up();
         tf_benefit_total += a->tf_counter->mean_tick_total();
         if (has_amount_results(stats->direct_results))
         {
            tf_benefit_up += a->tf_counter->mean_exe_up();
            tf_benefit_total += a->tf_counter->mean_exe_total();
         }

         if (p.talent.savage_roar->ok())
         {
         sr_exe_up += a->sr_counter->mean_exe_up();
         sr_exe_total += a->sr_counter->mean_exe_total();
         sr_benefit_up += a->sr_counter->mean_tick_up();
         sr_benefit_total += a->sr_counter->mean_tick_total();
         if (has_amount_results(stats->direct_results))
         {
            sr_benefit_up += a->sr_counter->mean_exe_up();
            sr_benefit_total += a->sr_counter->mean_exe_total();
         }
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

      if ( tf_exe_total > 0 || bt_exe_total > 0 || sr_exe_total > 0 )
      {
        std::string name_str = report::action_decorator_t( stats -> action_list[ 0 ] ).decorate();
        std::string row_class_str = "";
        if ( ++n & 1 )
          row_class_str = " class=\"odd\"";

        // Table Row : Name, TF up, TF total, TF up/total, TF up/sum(TF up)
        os.printf( "<tr%s><td class=\"left\">%s</td><td class=\"right\">%.2f %%</td><td class=\"right\">%.2f %%</td>\n",
                   row_class_str.c_str(),
                   name_str.c_str(),
                   util::round( tf_exe_up / tf_exe_total * 100, 2 ),
                   util::round( tf_benefit_up / tf_benefit_total * 100, 2 ) );
        if ( p.talent.savage_roar -> ok() )
        {
           os.printf("<td class=\"right\">%.2f %%</td><td class=\"right\">%.2f %%</td>\n",
              util::round(sr_exe_up / sr_exe_total * 100, 2),
              util::round(sr_benefit_up / sr_benefit_total * 100, 2));
        }
        if ( p.talent.bloodtalons -> ok() )
        {
          // Table Row : Name, TF up, TF total, TF up/total, TF up/sum(TF up)
          os.printf( "<td class=\"right\">%.2f %%</td><td class=\"right\">%.2f %%</td>\n",
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

private:
  druid_t& p;
};

// ==========================================================================
// Druid Special Effects
// ==========================================================================

using namespace unique_gear;
using namespace spells;
using namespace cat_attacks;
using namespace bear_attacks;
using namespace heals;
using namespace buffs;

// Warlords of Draenor ======================================================

struct wildcat_celerity_t : public scoped_buff_callback_t<buff_t>
{
  wildcat_celerity_t( const std::string& n ) :
    super( DRUID_FERAL, n )
  {}

  void manipulate( buff_t* b, const special_effect_t& e ) override
  {
    if ( b -> player -> true_level < 110 )
    {
      b -> buff_duration *= 1.0 + e.driver() -> effectN( 1 ).average( e.item ) / 100.0;
    }
  }
};

struct starshards_callback_t : public scoped_actor_callback_t<druid_t>
{
  starshards_callback_t() : super( DRUID_BALANCE )
  {}

  void manipulate( druid_t* p, const special_effect_t& e ) override
  {
    p -> starshards = e.driver() -> effectN( 1 ).average( e.item ) / 100.0;
    p -> active.starshards = new starshards_t( p );
  }
};


struct stalwart_guardian_callback_t : public scoped_actor_callback_t<druid_t>
{
  stalwart_guardian_callback_t() : super( DRUID_GUARDIAN )
  {}

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

  void manipulate( druid_t* p, const special_effect_t& /* e */ ) override
  {
    p -> active.stalwart_guardian = new stalwart_guardian_t( p );

    p->instant_absorb_list.insert( std::make_pair<unsigned, instant_absorb_t>(
        184878, instant_absorb_t( p, p->find_spell( 184878 ), "stalwart_guardian", &stalwart_guardian_handler ) ) );
  }
};

// Legion Artifacts =========================================================

struct fangs_of_ashamane_t : public scoped_actor_callback_t<druid_t>
{
  fangs_of_ashamane_t() : super( DRUID )
  {}

  void manipulate( druid_t* p, const special_effect_t& ) override
  {
    // Fangs of Ashamane act as a 2 handed weapon when in Cat Form.
    unsigned ilevel = p -> items[ SLOT_MAIN_HAND ].item_level();
    double mod = p -> sim -> dbc.item_damage_2h( ilevel ).values[ ITEM_QUALITY_ARTIFACT ]
                / p -> sim -> dbc.item_damage_1h( ilevel ).values[ ITEM_QUALITY_ARTIFACT ];
    p -> cat_weapon.min_dmg *= mod;
    p -> cat_weapon.max_dmg *= mod;
    p -> cat_weapon.damage  *= mod;
  }
};

// Legion Legendaries =======================================================

// General

template<typename T>
struct luffa_wrappings_t : public scoped_action_callback_t<T>
{
  luffa_wrappings_t( const std::string& name ) :
    scoped_action_callback_t<T>( DRUID, name )
  {}

  void manipulate( T* a, const special_effect_t& e ) override
  {
    // Feral Druid passive modifies the strength of the effect.
    a->radius *= 1.0 + e.driver()->effectN(1).percent();
      //+ p -> spec.feral -> effectN( 8 ).percent();
    a->base_multiplier *= 1.0 + e.driver()->effectN(2).percent();
      //+ p -> spec.feral -> effectN( 8 ).percent();
  }
};

struct dual_determination_t : public scoped_action_callback_t<survival_instincts_t>
{
  dual_determination_t() : super( DRUID, "survival_instincts" )
  {}

  void manipulate( survival_instincts_t* a, const special_effect_t& e ) override
  {
    a -> cooldown -> charges += (int) e.driver() -> effectN( 1 ).base_value();
    a -> base_recharge_multiplier *= 1.0 + e.driver() -> effectN( 2 ).percent();
  }
};

struct sephuzs_t : scoped_actor_callback_t<druid_t>
{
   sephuzs_t() : scoped_actor_callback_t(DRUID)
   { }

   void manipulate(druid_t* p, const special_effect_t& e) override
   {
      p->legendary.sephuzs_secret = e.driver();
   };
};

struct sephuzs_secret_t : public class_buff_cb_t<druid_t>
{
   sephuzs_secret_t() : super(DRUID, "sephuzs_secret")
   { }

   buff_t*& buff_ptr(const special_effect_t& e) override
   {
      return ((druid_t*)(e.player))->buff.sephuzs_secret;
   }

   buff_t* creator(const special_effect_t& e) const override
   {
      auto buff = make_buff( e.player, buff_name, e.trigger());
      buff->set_cooldown(e.player->find_spell(226262)->duration())
         ->set_default_value(e.trigger()->effectN(2).percent())
         ->add_invalidate(CACHE_RUN_SPEED)
         ->add_invalidate(CACHE_HASTE);
      return buff;
   }
};



// Feral

struct ailuro_pouncers_t : public scoped_buff_callback_t<buff_t>
{
  ailuro_pouncers_t() : super( DRUID_FERAL, "predatory_swiftness" )
  {}

  void manipulate( buff_t* b, const special_effect_t& e ) override
  {
    b -> set_max_stack( as<unsigned>( b -> max_stack() + e.driver() -> effectN( 1 ).base_value() ) );

    druid_t* p = debug_cast<druid_t*>( b -> player );
    p -> legendary.ailuro_pouncers = e.driver() -> effectN( 2 ).period();
  }
};

struct behemoth_headdress_t : public scoped_actor_callback_t<druid_t>
{
   behemoth_headdress_t() : super( DRUID )
   {}

   void manipulate( druid_t* d, const special_effect_t& e ) override
   {
      d->legendary.behemoth_headdress = e.driver()->effectN(1).base_value() / 10.0;
   }
};

struct chatoyant_signet_t : public scoped_actor_callback_t<druid_t>
{
  chatoyant_signet_t() : super( DRUID )
  {}

  void manipulate( druid_t* p, const special_effect_t& e ) override
  {
    p -> resources.base[ RESOURCE_ENERGY ] += e.driver() -> effectN( 1 ).resource( RESOURCE_ENERGY );
    p -> resources.base_regen_per_second[ RESOURCE_ENERGY ] *= ( 1.0 + e.driver() -> effectN( 2 ).percent( ) );
  }
};

struct the_wildshapers_clutch_t : public scoped_actor_callback_t<druid_t>
{
  the_wildshapers_clutch_t() : super( DRUID )
  {}

  void manipulate( druid_t* p, const special_effect_t& e ) override
  {
    p -> legendary.the_wildshapers_clutch = e.driver() -> proc_chance();
  }
};

struct fiery_red_maimers_t : public scoped_actor_callback_t<druid_t>
{
  fiery_red_maimers_t() : super( DRUID )
  {}

  void manipulate(druid_t* /*p*/, const special_effect_t& /*e*/) override
  {
    //only relevant for a few weeks, unlikely to change until then - hardcoding it for now.
    //p->legendary.LegendaryDamageMod = 0.03;
  }
};

// Balance

struct impeccable_fel_essence_t : public scoped_actor_callback_t<druid_t>
{
  impeccable_fel_essence_t() : super( DRUID )
  {}

  void manipulate( druid_t* p, const special_effect_t& e ) override
  {
    p -> legendary.impeccable_fel_essence =
      - timespan_t::from_seconds( e.driver() -> effectN( 1 ).base_value() ) / e.driver() -> effectN( 2 ).base_value();
  }
};

struct radiant_moonlight_t : public scoped_action_callback_t<full_moon_t>
{
    radiant_moonlight_t() : super(DRUID, "full_moon")
    {}

    void manipulate(full_moon_t* p, const special_effect_t&/* e */) override {
        p->radiant_moonlight = true;
    }
};

struct promise_of_elune_t : public class_buff_cb_t<druid_t>
{
  promise_of_elune_t() : super( DRUID, "power_of_elune_the_moon_goddess" )
  {}

  buff_t*& buff_ptr( const special_effect_t& e ) override
  { return actor( e ) -> buff.power_of_elune; }

  buff_t* creator( const special_effect_t& e ) const override
  {
    return make_buff( e.player, buff_name, e.driver() -> effectN( 1 ).trigger() );
  }
};

struct promise_of_elune_lunar_t : public scoped_action_callback_t<lunar_strike_t>
{
  promise_of_elune_lunar_t() : super(DRUID, "lunar_strike")
  {}

  void manipulate(lunar_strike_t* action, const special_effect_t& e) override
  {
    action -> base_multiplier *= 1.0 + e.driver() -> effectN( 2 ).percent();
  }
};

struct promise_of_elune_solar_t : public scoped_action_callback_t<solar_wrath_t>
{
  promise_of_elune_solar_t() : super(DRUID, "solar_wrath")
  {}

  void manipulate(solar_wrath_t* action, const special_effect_t& e) override
  {
    action -> base_multiplier *= 1.0 + e.driver() -> effectN( 2 ).percent();
  }
};

struct oneths_intuition_t : public class_buff_cb_t<druid_t>
{
  oneths_intuition_t() : super( DRUID, "oneths_intuition" )
  {}

  buff_t*& buff_ptr( const special_effect_t& e ) override
  { return actor( e ) -> buff.oneths_intuition; }

  buff_t* creator( const special_effect_t& e ) const override
  {
    return make_buff( e.player, buff_name, e.player -> find_spell( 209406 ) )
      ->set_chance( e.driver() -> proc_chance() );
  }
};

struct oneths_overconfidence_t : public class_buff_cb_t<druid_t>
{
  oneths_overconfidence_t() : super( DRUID, "oneths_overconfidence" )
  {}

  buff_t*& buff_ptr( const special_effect_t& e ) override
  { return actor( e ) -> buff.oneths_overconfidence; }

  buff_t* creator( const special_effect_t& e ) const override
  {
    return make_buff( e.player, buff_name, e.player -> find_spell( 209407 ) )
      ->set_chance( e.driver() -> proc_chance() );
  }
};

struct the_emerald_dreamcatcher_t : public class_buff_cb_t<druid_t>
{
  the_emerald_dreamcatcher_t() : super( DRUID, "the_emerald_dreamcatcher" )
  {}

  buff_t*& buff_ptr( const special_effect_t& e ) override
  { return actor( e ) -> buff.the_emerald_dreamcatcher; }

  buff_t* creator( const special_effect_t& e ) const override
  {
    return make_buff( e.player, buff_name, e.driver() -> effectN( 1 ).trigger() )
      ->set_default_value( e.driver() -> effectN( 1 ).trigger()
        -> effectN( 1 ).resource( RESOURCE_ASTRAL_POWER ) );
  }
};

template<typename T>
struct lady_and_the_child_t : public scoped_action_callback_t<T>
{
  lady_and_the_child_t( const std::string& name ) :
    scoped_action_callback_t<T>( DRUID, name )
  {}

  void manipulate( T* a, const special_effect_t& e ) override
  {
    a -> aoe += (int) e.driver() -> effectN( 1 ).base_value();
    a -> base_dd_multiplier *= 1.0 + e.driver() -> effectN( 2 ).percent();
    a -> base_td_multiplier *= 1.0 + e.driver() -> effectN( 3 ).percent();
  }
};

// Guardian

struct fury_of_nature_t : public scoped_actor_callback_t<druid_t>
{

   fury_of_nature_t() : super( DRUID )
   {}

   void manipulate( druid_t* d, const special_effect_t& e ) override
   {
      d -> legendary.fury_of_nature += e.driver() -> effectN( 1 ).percent();
   }
};

struct elizes_everlasting_encasement_t : public scoped_action_callback_t<thrash_bear_t>
{
  elizes_everlasting_encasement_t() : super( DRUID, "thrash_bear" )
  {}

  void manipulate( thrash_bear_t* a, const special_effect_t& e ) override
  {
    a -> dot_max_stack += (int) e.driver() -> effectN( 1 ).base_value();
  }
};

struct skysecs_hold_t : public scoped_action_callback_t<frenzied_regeneration_t>
{
  struct skysecs_hold_heal_t : public druid_heal_t
  {
    skysecs_hold_heal_t( const special_effect_t& effect ) :
      druid_heal_t( "skysecs_hold", debug_cast<druid_t*>( effect.player ),
      effect.driver() -> effectN( 1 ).trigger() )
    {
      background = true;
      hasted_ticks = may_crit = tick_may_crit = false;
      target = player;
      tick_pct_heal = data().effectN( 1 ).percent();
    }

    void init() override
    {
      druid_heal_t::init();

      snapshot_flags = update_flags = 0;
    }
  };

  skysecs_hold_t() : super( DRUID, "frenzied_regeneration_driver" )
  {}

  void manipulate( frenzied_regeneration_t* a, const special_effect_t& e ) override
  {
    a -> skysecs_hold = new skysecs_hold_heal_t( e );
  }
};

struct oakhearts_puny_quods_t : public scoped_action_callback_t<barkskin_t>
{
  oakhearts_puny_quods_t() : super( DRUID, "barkskin" )
  {}

  void manipulate( barkskin_t* a, const special_effect_t& e ) override
  {
    assert( a -> energize_resource == RESOURCE_NONE );

    a -> energize_resource = RESOURCE_RAGE;
    a -> energize_amount = e.driver() -> effectN( 1 ).trigger()
      -> effectN( 1 ).resource( RESOURCE_RAGE );
    a -> energize_type = ENERGIZE_ON_CAST;
  }
};

struct sylvan_walker_t: public unique_gear::scoped_actor_callback_t<druid_t>
{
  sylvan_walker_t(): super( DRUID )
  {}

  void manipulate( druid_t* druid, const special_effect_t& e ) override
  {
    druid -> sylvan_walker = util::round( e.driver() -> effectN( 1 ).average( e.item ) );
  }
};

struct oakhearts_puny_quods_buff_t : public class_buff_cb_t<druid_t>
{
  oakhearts_puny_quods_buff_t() : super( DRUID, "oakhearts_puny_quods" )
  {}

  buff_t*& buff_ptr( const special_effect_t& e ) override
  {
    return actor( e ) -> buff.oakhearts_puny_quods;
  }

  buff_t* creator(const special_effect_t& e) const override
  {
    return make_buff( e.player, buff_name, e.driver() -> effectN( 1 ).trigger() )
      ->set_default_value( e.driver() -> effectN( 1 ).trigger()
        -> effectN( 2 ).resource( RESOURCE_RAGE ) )
      ->set_tick_callback( []( buff_t* b, int, const timespan_t& ) {
      assert(b -> player);
      b -> player -> resource_gain( RESOURCE_RAGE,
        b -> default_value,
        debug_cast<druid_t*>( b -> player ) -> gain.oakhearts_puny_quods ); } );
  }
};

// Druid Special Effects ====================================================

#if 0
static void init_special_effect( druid_t*                 player,
                                 specialization_e         spec,
                                 const special_effect_t*& ptr,
                                 const special_effect_t&  effect )
{
  /* Ensure we have the spell data. This will prevent the trinket effect from
     working on live Simulationcraft. Also ensure correct specialization. */
  if ( ! player -> find_spell( effect.spell_id ) -> ok() ||
       ( player -> specialization() != spec && spec != SPEC_NONE ) )
  {
    return;
  }

  // Set pointer, module considers non-null pointer to mean the effect is "enabled"
  ptr = &( effect );
}
#endif

// DRUID MODULE INTERFACE ===================================================

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
    // Warlords of Draenor
    register_special_effect( 184876, starshards_callback_t() );
    register_special_effect( 184877, wildcat_celerity_t( "incarnation_king_of_the_jungle" ) );
    register_special_effect( 184877, wildcat_celerity_t( "berserk" ) );
    register_special_effect( 184877, wildcat_celerity_t( "tigers_fury" ) );
    register_special_effect( 184878, stalwart_guardian_callback_t() );
    // register_special_effect( 184879, flourish_t() );

    // Legion Artifacts
    register_special_effect( 214843, fangs_of_ashamane_t() );

    // Legion Legendaries
    register_special_effect( 248083, fury_of_nature_t() );
    register_special_effect( 208228, dual_determination_t() );
    register_special_effect( 208342, elizes_everlasting_encasement_t() );
    register_special_effect( 208199, impeccable_fel_essence_t() );
    register_special_effect( 208319, the_wildshapers_clutch_t() );
    register_special_effect( 209405, oneths_intuition_t(), true );
    register_special_effect( 209405, oneths_overconfidence_t(), true );
    register_special_effect( 207523, chatoyant_signet_t() );
    register_special_effect( 208209, ailuro_pouncers_t() );
    register_special_effect( 208283, promise_of_elune_t(), true );
    register_special_effect( 208283, promise_of_elune_lunar_t() );
    register_special_effect( 208283, promise_of_elune_solar_t() );
    register_special_effect( 208219, skysecs_hold_t() );
    register_special_effect( 208190, the_emerald_dreamcatcher_t(), true );
    register_special_effect( 208681, luffa_wrappings_t<thrash_cat_t>( "thrash_cat" ) );
    register_special_effect( 208681, luffa_wrappings_t<thrash_bear_t>( "thrash_bear" ) );
    register_special_effect( 236478, oakhearts_puny_quods_t() );
    register_special_effect( 236478, oakhearts_puny_quods_buff_t(), true );
    register_special_effect( 200818, lady_and_the_child_t<moonfire_t::moonfire_damage_t>( "moonfire_dmg" ) );
    register_special_effect( 200818, lady_and_the_child_t<moonfire_t::galactic_guardian_damage_t>( "galactic_guardian" ) );
    register_special_effect( 200818, lady_and_the_child_t<lunar_inspiration_t>( "lunar_inspiration" ) );
    register_special_effect( 212875, fiery_red_maimers_t(), true );
    register_special_effect( 222270, sylvan_walker_t() );
    register_special_effect( 208051, sephuzs_t() );
    register_special_effect( 208051, sephuzs_secret_t(), true);
    register_special_effect( 248081, behemoth_headdress_t() );
    register_special_effect( 248163, radiant_moonlight_t());
    // register_special_effect( 208220, amanthuls_wisdom );
    // register_special_effect( 207943, edraith_bonds_of_aglaya );
    // register_special_effect( 210667, ekowraith_creator_of_worlds );
    // register_special_effect( 207932, tearstone_of_elune );
    // register_special_effect( 207271, the_dark_titans_advice );
    // register_special_effect( 208191, essence_of_infusion_t() );

    // Druid Leyshock's Grand Compendium basic hooks
    // Balance
    // Assumption is buff procs before streaking stars damage is calculated
    // TODO:Moonfire (crit on dot ticks?)
    //      Sunfire (vers on dot ticks?)
    //      Force of Nature (crit on treant melee?)
    //      Fury of Elune (haste on tick)
    //      Starfall (mastery per tic, haste per impact?)
    //      Solar Beam (vers & haste)
    //      Nature's Balance (haste when out of combat)
    //      Flap? (mastery)
    //      Generic druid spells, see https://docs.google.com/spreadsheets/d/1QA5-57JSdpi_DJXxb-1UYnKeEz4-lNVX5OSHaYBc8nM/edit#gid=1802579435
    //
    // Moonfire
    expansion::bfa::register_leyshocks_trigger( 8921, STAT_CRIT_RATING );
    // Sunfire
    expansion::bfa::register_leyshocks_trigger( 93402, STAT_VERSATILITY_RATING );
    // Solar Wrath
    expansion::bfa::register_leyshocks_trigger( 190984, STAT_CRIT_RATING );
    // Lunar Strike
    expansion::bfa::register_leyshocks_trigger( 194153, STAT_HASTE_RATING );
    // Starsurge
    expansion::bfa::register_leyshocks_trigger( 78674, STAT_VERSATILITY_RATING );
    // Warrior of Elune
    expansion::bfa::register_leyshocks_trigger( 202425, STAT_HASTE_RATING );
    // Stellar Flare
    expansion::bfa::register_leyshocks_trigger( 202347, STAT_VERSATILITY_RATING );
    // Fury of Elune
    expansion::bfa::register_leyshocks_trigger( 202770, STAT_MASTERY_RATING );
    // Innervate
    expansion::bfa::register_leyshocks_trigger( 29166, STAT_MASTERY_RATING );
    // Celestial Alignment
    expansion::bfa::register_leyshocks_trigger( 194223, STAT_VERSATILITY_RATING );
    // Incarnation: Chosen of Elune
    expansion::bfa::register_leyshocks_trigger( 102560, STAT_HASTE_RATING );
    // Full Moon
    expansion::bfa::register_leyshocks_trigger( 274283, STAT_VERSATILITY_RATING );
    // Half Moon
    expansion::bfa::register_leyshocks_trigger( 274282, STAT_HASTE_RATING );
    // New Moon
    expansion::bfa::register_leyshocks_trigger( 274281, STAT_VERSATILITY_RATING );
    // Shooting Stars
    expansion::bfa::register_leyshocks_trigger( 202497, STAT_HASTE_RATING );
    // Starfall
    expansion::bfa::register_leyshocks_trigger( 191034, STAT_MASTERY_RATING );
    expansion::bfa::register_leyshocks_trigger( 191037, STAT_MASTERY_RATING );
    //expansion::bfa::register_leyshocks_trigger( 191037, STAT_HASTE_RATING ); In-game procs haste also, but leaving it out in-sim for simplicity purposes
  }

  virtual void register_hotfixes() const override
  {
    /*
    hotfix::register_spell( "Druid", "2016-12-18", "Incorrect spell level for starfall damage component.", 191037 )
      .field( "spell_level" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 40 )
      .verification_value( 76 );
    */
    /*
    hotfix::register_effect( "Druid", "2016-09-23", "Sunfire damage increased by 10%.-dot", 232417 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.10 )
      .verification_value( 0.5 );

    hotfix::register_effect( "Druid", "2016-09-23", "Starfall damage increased by 10%.", 280158 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.10 )
      .verification_value( 0.4 );

    hotfix::register_effect( "Druid", "2016-09-23", "Lunar Strike damage increased by 5%.", 284976 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.05 )
      .verification_value( 2.7 );

    hotfix::register_effect( "Druid", "2016-09-23", "Solar Wrath damage increased by 5%.", 280098 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.05 )
      .verification_value( 1.9 );
      */
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
