// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
//
// TODO
// Legendaries
// Double check all up()/check() usage.
// Expression to estimate imp vs implosion damage.
//
// Affliction -
// Haunt reset
// Soul Flame + Wrath of Consumption on-death effects.
// Peridition needs special crit damage override thing NYI.
// Compounding Horror
// Fatal Echoes
//
// Destruction/Demonology -
// 20/20 Trait
//
// Better reporting for add buffs.
//
// Wild imps have a 14 sec duration on 104317, expire after 12 UNLESS implosion.
// Check resource generation execute/impact and hit requirement
// Report which spells triggered soul conduit
//
// ==========================================================================
namespace { // unnamed namespace

// ==========================================================================
// Warlock
// ==========================================================================

struct warlock_t;

namespace pets {
  struct wild_imp_pet_t;
  struct t18_illidari_satyr_t;
  struct t18_prince_malchezaar_t;
  struct t18_vicious_hellhound_t;
  struct chaos_tear_t;
  struct dreadstalker_t;
  struct infernal_t;
  struct doomguard_t;
  struct lord_of_flames_infernal_t;
  struct darkglare_t;
  struct thal_kiel_t;
  struct soul_effigy_t;
  namespace shadowy_tear {
    struct shadowy_tear_t;
  }
  namespace chaos_portal {
    struct chaos_portal_t;
  }
}

namespace actions{
  struct effigy_damage_override_t;
}

#define MAX_UAS 5

struct warlock_td_t: public actor_target_data_t
{
  dot_t* dots_agony;
  dot_t* dots_corruption;
  dot_t* dots_doom;
  dot_t* dots_drain_life;
  dot_t* dots_drain_soul;
  dot_t* dots_immolate;
  dot_t* dots_seed_of_corruption;
  dot_t* dots_shadowflame;
  dot_t* dots_unstable_affliction[MAX_UAS];
  dot_t* dots_siphon_life;
  dot_t* dots_phantom_singularity;
  dot_t* dots_channel_demonfire;

  buff_t* debuffs_haunt;
  buff_t* debuffs_shadowflame;
  buff_t* debuffs_agony;
  buff_t* debuffs_flamelicked;
  buff_t* debuffs_eradication;
  buff_t* debuffs_roaring_blaze;
  buff_t* debuffs_havoc;

  int agony_stack;
  double soc_threshold;

  warlock_t& warlock;
  warlock_td_t( player_t* target, warlock_t& p );

  void reset()
  {
    agony_stack = 1;
    soc_threshold = 0;
  }

  void target_demise();
};

struct warlock_t: public player_t
{
public:
  player_t* havoc_target;
  double agony_accumulator;
  double demonwrath_accumulator;

  // Active Pet
  struct pets_t
  {
    pet_t* active;
    pet_t* last;
    static const int WILD_IMP_LIMIT = 40;
    static const int T18_PET_LIMIT = 0;
    static const int DREADSTALKER_LIMIT = 4;
    static const int DIMENSIONAL_RIFT_LIMIT = 6;
    static const int INFERNAL_LIMIT = 1;
    static const int DOOMGUARD_LIMIT = 1;
    static const int LORD_OF_FLAMES_INFERNAL_LIMIT = 3;
    static const int DARKGLARE_LIMIT = 1;
    std::array<pets::wild_imp_pet_t*, WILD_IMP_LIMIT> wild_imps;
    std::array<pets::t18_illidari_satyr_t*, T18_PET_LIMIT> t18_illidari_satyr;
    std::array<pets::t18_prince_malchezaar_t*, T18_PET_LIMIT> t18_prince_malchezaar;
    std::array<pets::t18_vicious_hellhound_t*, T18_PET_LIMIT> t18_vicious_hellhound;
    std::array<pets::shadowy_tear::shadowy_tear_t*, DIMENSIONAL_RIFT_LIMIT> shadowy_tear;
    std::array<pets::chaos_tear_t*, DIMENSIONAL_RIFT_LIMIT> chaos_tear;
    std::array<pets::chaos_portal::chaos_portal_t*, DIMENSIONAL_RIFT_LIMIT> chaos_portal;
    std::array<pets::dreadstalker_t*, DREADSTALKER_LIMIT> dreadstalkers;
    std::array<pets::infernal_t*, INFERNAL_LIMIT> infernal;
    std::array<pets::doomguard_t*, DOOMGUARD_LIMIT> doomguard;
    std::array<pets::lord_of_flames_infernal_t*, LORD_OF_FLAMES_INFERNAL_LIMIT> lord_of_flames_infernal;
    std::array<pets::darkglare_t*, DARKGLARE_LIMIT> darkglare;

    pets::soul_effigy_t* soul_effigy;
  } warlock_pet_list;

  std::vector<std::string> pet_name_list;

  struct active_t
  {
    action_t* demonic_power_proc;
    action_t* thalkiels_discord;
    action_t* harvester_of_souls;
    spell_t* rain_of_fire;
    spell_t* corruption;
    actions::effigy_damage_override_t* effigy_damage_override;

  } active;

  // Talents
  struct talents_t
  {
    // PTR
    const spell_data_t* empowered_life_tap;
    const spell_data_t* malefic_grasp;

    const spell_data_t* haunt;
    const spell_data_t* writhe_in_agony;
    const spell_data_t* drain_soul;

    const spell_data_t* backdraft;
    const spell_data_t* fire_and_brimstone;
    const spell_data_t* shadowburn;

    const spell_data_t* shadowy_inspiration;
    const spell_data_t* shadowflame;
    const spell_data_t* demonic_calling;

    const spell_data_t* contagion;
    const spell_data_t* absolute_corruption;

    const spell_data_t* reverse_entropy;
    const spell_data_t* roaring_blaze;

    const spell_data_t* mana_tap;

    const spell_data_t* impending_doom;
    const spell_data_t* improved_dreadstalkers;
    const spell_data_t* implosion;

    const spell_data_t* demon_skin;
    const spell_data_t* mortal_coil;
    const spell_data_t* howl_of_terror;
    const spell_data_t* shadowfury;

    const spell_data_t* hand_of_doom;
    const spell_data_t* power_trip;

    const spell_data_t* siphon_life;
    const spell_data_t* sow_the_seeds;

    const spell_data_t* eradication;
    const spell_data_t* cataclysm;

    const spell_data_t* soul_harvest;

    const spell_data_t* demonic_circle;
    const spell_data_t* burning_rush;
    const spell_data_t* dark_pact;

    const spell_data_t* grimoire_of_supremacy;
    const spell_data_t* grimoire_of_service;
    const spell_data_t* grimoire_of_sacrifice;
    const spell_data_t* grimoire_of_synergy;

    const spell_data_t* soul_effigy;
    const spell_data_t* phantom_singularity;

    const spell_data_t* wreak_havoc;
    const spell_data_t* channel_demonfire;

    const spell_data_t* summon_darkglare;
    const spell_data_t* demonbolt;

    const spell_data_t* soul_conduit;
  } talents;

  struct artifact_spell_data_t
  {
    // Affliction
    artifact_power_t reap_souls;
    artifact_power_t crystaline_shadows;
    artifact_power_t seeds_of_doom;
    artifact_power_t fatal_echoes;
    artifact_power_t shadows_of_the_flesh;
    artifact_power_t harvester_of_souls;
    artifact_power_t inimitable_agony;
    artifact_power_t drained_to_a_husk;
    artifact_power_t inherently_unstable;
    artifact_power_t sweet_souls;
    artifact_power_t perdition;
    artifact_power_t wrath_of_consumption;
    artifact_power_t hideous_corruption;
    artifact_power_t shadowy_incantations;
    artifact_power_t soul_flames;
    artifact_power_t long_dark_night_of_the_soul;
    artifact_power_t compounding_horror;
    artifact_power_t soulharvester;
    artifact_power_t soulstealer;

    // Demonology
    artifact_power_t thalkiels_consumption;
    artifact_power_t breath_of_thalkiel;
    artifact_power_t the_doom_of_azeroth;
    artifact_power_t sharpened_dreadfangs;
    artifact_power_t fel_skin; //NYI
    artifact_power_t firm_resolve; //NYI
    artifact_power_t thalkiels_discord;
    artifact_power_t legionwrath;
    artifact_power_t dirty_hands;
    artifact_power_t doom_doubled;
    artifact_power_t infernal_furnace;
    artifact_power_t the_expendables;
    artifact_power_t maw_of_shadows;
    artifact_power_t open_link; //NYI
    artifact_power_t stolen_power;
    artifact_power_t imperator;
    artifact_power_t summoners_prowess;
    artifact_power_t thalkiels_lingering_power;//NYI

    // Destruction
    artifact_power_t dimensional_rift;
    artifact_power_t flames_of_the_pit;
    artifact_power_t soulsnatcher;
    artifact_power_t fire_and_the_flames;
    artifact_power_t fire_from_the_sky;
    artifact_power_t impish_incineration;
    artifact_power_t lord_of_flames;
    artifact_power_t eternal_struggle;
    artifact_power_t demonic_durability;
    artifact_power_t chaotic_instability;
    artifact_power_t dimension_ripper;
    artifact_power_t master_of_distaster;
    artifact_power_t burning_hunger;
    artifact_power_t residual_flames;
    artifact_power_t devourer_of_life;
    artifact_power_t planeswalker;
    artifact_power_t conflagration_of_chaos;

  } artifact;

  struct legendary_t
  {
    bool odr_shawl_of_the_ymirjar;
    bool feretory_of_souls;
    bool wilfreds_sigil_of_superior_summoning_flag;
    bool stretens_insanity;
    timespan_t wilfreds_sigil_of_superior_summoning;
    double power_cord_of_lethtendris_chance;
  } legendary;

  // Glyphs
  struct glyphs_t
  {
  } glyphs;

  // Mastery Spells
  struct mastery_spells_t
  {
    const spell_data_t* potent_afflictions;
    const spell_data_t* master_demonologist;
    const spell_data_t* chaotic_energies;
  } mastery_spells;

  //Procs and RNG
  real_ppm_t* misery_rppm; // affliction t17 4pc
  real_ppm_t* demonic_power_rppm; // grimoire of sacrifice
  real_ppm_t* grimoire_of_synergy; //caster ppm, i.e., if it procs, the wl will create a buff for the pet.
  real_ppm_t* grimoire_of_synergy_pet; //pet ppm, i.e., if it procs, the pet will create a buff for the wl.
  real_ppm_t* tormented_souls_rppm; // affliction artifact buff

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* infernal;
    cooldown_t* doomguard;
    cooldown_t* dimensional_rift;
    cooldown_t* haunt;
    cooldown_t* sindorei_spite_icd;
  } cooldowns;

  // Passives
  struct specs_t
  {
    // All Specs
    const spell_data_t* fel_armor;
    const spell_data_t* nethermancy;

    // Affliction only
    const spell_data_t* nightfall;
    const spell_data_t* unstable_affliction;
    const spell_data_t* unstable_affliction_2;
    const spell_data_t* agony;
    const spell_data_t* agony_2;
    const spell_data_t* shadow_bite;
    const spell_data_t* shadow_bite_2;

    // Demonology only
    const spell_data_t* doom;
    const spell_data_t* demonic_empowerment;
    const spell_data_t* wild_imps;

    // Destruction only
    const spell_data_t* immolate;
    const spell_data_t* conflagrate;
    const spell_data_t* conflagrate_2;
    const spell_data_t* unending_resolve;
    const spell_data_t* unending_resolve_2;
    const spell_data_t* firebolt;
    const spell_data_t* firebolt_2;

    // PTr
    const spell_data_t* drain_soul;
  } spec;

  // Buffs
  struct buffs_t
  {
    buff_t* demonic_power;
    buff_t* mana_tap;
    buff_t* empowered_life_tap;
    buff_t* soul_harvest;

    //affliction buffs
    buff_t* shard_instability;
    buff_t* instability;
    buff_t* reap_souls;
    haste_buff_t* misery;
    buff_t* deadwind_harvester;
    buff_t* tormented_souls;
    buff_t* compounding_horror;

    //demonology buffs
    buff_t* tier18_2pc_demonology;
    buff_t* demonic_synergy;
    buff_t* shadowy_inspiration;
    buff_t* stolen_power_stacks;
    buff_t* stolen_power;
    buff_t* demonic_calling;
    buff_t* t18_4pc_driver;

    //destruction_buffs
    buff_t* backdraft;
    buff_t* conflagration_of_chaos;
    buff_t* lord_of_flames;
    buff_t* embrace_chaos;

    // legendary buffs
    buff_t* sindorei_spite;

    buff_t* stretens_insanity;
  } buffs;

  // Gains
  struct gains_t
  {
    gain_t* life_tap;
    gain_t* agony;
    gain_t* conflagrate;
    gain_t* immolate;
    gain_t* shadowburn_shard;
    gain_t* miss_refund;
    gain_t* seed_of_corruption;
    gain_t* drain_soul;
    gain_t* mana_tap;
    gain_t* power_trip;
    gain_t* shadow_bolt;
    gain_t* doom;
    gain_t* demonwrath;
    gain_t* soul_conduit;
    gain_t* reverse_entropy;
    gain_t* soulsnatcher;
    gain_t* t18_4pc_destruction;
    gain_t* t19_2pc_demonology;
    gain_t* recurrent_ritual;
    gain_t* feretory_of_souls;
    gain_t* power_cord_of_lethtendris;
  } gains;

  // Procs
  struct procs_t
  {
    //aff
    proc_t* fatal_echos;
    proc_t* wild_imp;
    proc_t* fragment_wild_imp;
    proc_t* t18_2pc_affliction;
    proc_t* t18_4pc_destruction;
    proc_t* t18_illidari_satyr;
    proc_t* t18_vicious_hellhound;
    proc_t* t18_prince_malchezaar;
    proc_t* shadowy_tear;
    proc_t* chaos_tear;
    proc_t* chaos_portal;
    proc_t* dreadstalker_debug;
    proc_t* dimension_ripper;
    proc_t* soul_conduit;
    proc_t* one_shard_hog;
    proc_t* two_shard_hog;
    proc_t* three_shard_hog;
    proc_t* four_shard_hog;
    //demo
    proc_t* impending_doom;
    proc_t* improved_dreadstalkers;
    proc_t* thalkiels_discord;
    proc_t* demonic_calling;
    proc_t* power_trip;
    proc_t* stolen_power_stack;
    proc_t* stolen_power_used;
    proc_t* t18_demo_4p;
    proc_t* souls_consumed;
    proc_t* the_expendables;
    proc_t* wilfreds_dog;
    proc_t* wilfreds_imp;
    proc_t* wilfreds_darkglare;
  } procs;

  struct spells_t
  {
    spell_t* melee;
    spell_t* seed_of_corruption_aoe;
    spell_t* implosion_aoe;
  } spells;

  int initial_soul_shards;
  double reap_souls_modifier;
  std::string default_pet;

  timespan_t shard_react;

    // Tier 18 (WoD 6.2) trinket effects
  const special_effect_t* affliction_trinket;
  const special_effect_t* demonology_trinket;
  const special_effect_t* destruction_trinket;

  // Artifacts
  const special_effect_t* ulthalesh_the_dreadwind_harvester, *skull_of_the_manari, *scepter_of_sargeras;

  warlock_t( sim_t* sim, const std::string& name, race_e r = RACE_UNDEAD );

  // Character Definition
  virtual void      init_spells() override;
  virtual void      init_base_stats() override;
  virtual void      init_scaling() override;
  virtual void      create_buffs() override;
  virtual void      init_gains() override;
  virtual void      init_procs() override;
  virtual void      init_rng() override;
  virtual void      init_action_list() override;
  virtual void      init_resources( bool force ) override;
  virtual void      reset() override;
  virtual void      create_options() override;
  virtual action_t* create_action( const std::string& name, const std::string& options ) override;
  virtual pet_t*    create_pet( const std::string& name, const std::string& type = std::string() ) override;
  virtual void      create_pets() override;
  virtual std::string      create_profile( save_e = SAVE_ALL ) override;
  virtual void      copy_from( player_t* source ) override;
  virtual resource_e primary_resource() const override { return RESOURCE_MANA; }
  virtual role_e    primary_role() const override     { return ROLE_SPELL; }
  virtual stat_e    convert_hybrid_stat( stat_e s ) const override;
  virtual double    matching_gear_multiplier( attribute_e attr ) const override;
  virtual double    composite_player_multiplier( school_e school ) const override;
  virtual double    composite_rating_multiplier( rating_e rating ) const override;
  virtual void      invalidate_cache( cache_e ) override;
  virtual double    composite_spell_crit_chance() const override;
  virtual double    composite_spell_haste() const override;
  virtual double    composite_melee_haste() const override;
  virtual double    composite_melee_crit_chance() const override;
  virtual double    composite_mastery() const override;
  virtual double    resource_gain( resource_e, double, gain_t* = nullptr, action_t* = nullptr ) override;
  virtual double    mana_regen_per_second() const override;
  virtual double    composite_armor() const override;

  virtual void      halt() override;
  virtual void      combat_begin() override;
  virtual expr_t*   create_expression( action_t* a, const std::string& name_str ) override;

  void trigger_lof_infernal();
  void trigger_effigy(pets::soul_effigy_t *effigy);

  target_specific_t<warlock_td_t> target_data;

  virtual warlock_td_t* get_target_data( player_t* target ) const override
  {
    warlock_td_t*& td = target_data[target];
    if ( ! td )
    {
      td = new warlock_td_t( target, const_cast<warlock_t&>( *this ) );
    }
    return td;
  }



private:
  void apl_precombat();
  void apl_default();
  void apl_affliction();
  void apl_demonology();
  void apl_destruction();
  void apl_global_filler();
};

static void do_trinket_init(  warlock_t*               player,
                              specialization_e         spec,
                              const special_effect_t*& ptr,
                              const special_effect_t&  effect )
{
  // Ensure we have the spell data. This will prevent the trinket effect from working on live
  // Simulationcraft. Also ensure correct specialization.
  if ( !player -> find_spell( effect.spell_id ) -> ok() ||
    player -> specialization() != spec )
  {
    return;
  }
  // Set pointer, module considers non-null pointer to mean the effect is "enabled"
  ptr = &( effect );
}

static void affliction_trinket( special_effect_t& effect )
{
  warlock_t* warlock = debug_cast<warlock_t*>( effect.player );
  do_trinket_init( warlock, WARLOCK_AFFLICTION, warlock -> affliction_trinket, effect );
}

static void demonology_trinket( special_effect_t& effect )
{
  warlock_t* warlock = debug_cast<warlock_t*>( effect.player );
  do_trinket_init( warlock, WARLOCK_DEMONOLOGY, warlock -> demonology_trinket, effect );
}

static void destruction_trinket( special_effect_t& effect )
{
  warlock_t* warlock = debug_cast<warlock_t*>( effect.player );
  do_trinket_init( warlock, WARLOCK_DESTRUCTION, warlock -> destruction_trinket, effect);
}

void parse_spell_coefficient( action_t& a )
{
  for ( size_t i = 1; i <= a.data()._effects -> size(); i++ )
  {
    if ( a.data().effectN( i ).type() == E_SCHOOL_DAMAGE )
      a.spell_power_mod.direct = a.data().effectN( i ).sp_coeff();
    else if ( a.data().effectN( i ).type() == E_APPLY_AURA && a.data().effectN( i ).subtype() == A_PERIODIC_DAMAGE )
      a.spell_power_mod.tick = a.data().effectN( i ).sp_coeff();
  }
}

// Pets
namespace pets {

  struct warlock_pet_t: public pet_t
  {
    action_t* special_action;
    action_t* special_action_two;
    melee_attack_t* melee_attack;
    stats_t* summon_stats;
    const spell_data_t* command;

    warlock_pet_t( sim_t* sim, warlock_t* owner, const std::string& pet_name, pet_e pt, bool guardian = false );
    virtual void init_base_stats() override;
    virtual void init_action_list() override;
    virtual void create_buffs() override;
    virtual void schedule_ready( timespan_t delta_time = timespan_t::zero(),
      bool   waiting = false ) override;
    virtual double composite_player_multiplier( school_e school ) const override;
    virtual double composite_melee_crit_chance() const override;
    virtual double composite_spell_crit_chance() const override;
    virtual double composite_melee_haste() const override;
    virtual double composite_spell_haste() const override;
    virtual double composite_melee_speed() const override;
    virtual double composite_spell_speed() const override;
    virtual resource_e primary_resource() const override { return RESOURCE_ENERGY; }
    warlock_t* o()
    {
      return static_cast<warlock_t*>( owner );
    }
    const warlock_t* o() const
    {
      return static_cast<warlock_t*>( owner );
    }

    struct buffs_t
    {
      buff_t* demonic_synergy;
      haste_buff_t* demonic_empowerment;
      buff_t* the_expendables;
    } buffs;

    bool is_grimoire_of_service = false;
    bool is_demonbolt_enabled = true;
    bool is_lord_of_flames = false;

    struct travel_t: public action_t
    {
      travel_t( player_t* player ): action_t( ACTION_OTHER, "travel", player ) {}
      void execute() override { player -> current.distance = 1; }
      timespan_t execute_time() const override { return timespan_t::from_seconds( player -> current.distance / 33.0 ); }
      bool ready() override { return ( player -> current.distance > 1 ); }
      bool usable_moving() const override { return true; }
    };

    action_t* create_action( const std::string& name,
      const std::string& options_str ) override
    {
      if ( name == "travel" ) return new travel_t( this );

      return pet_t::create_action( name, options_str );
    }
  };

//namespace petactions {

// Template for common warlock pet action code. See priest_action_t.
template <class ACTION_BASE>
struct warlock_pet_action_t: public ACTION_BASE
{
public:
private:
  typedef ACTION_BASE ab; // action base, eg. spell_t
public:
  typedef warlock_pet_action_t base_t;

  warlock_pet_action_t( const std::string& n, warlock_pet_t* p,
                        const spell_data_t* s = spell_data_t::nil()):
                        ab( n, p, s )
  {
    ab::may_crit = true;
  }
  virtual ~warlock_pet_action_t() {}

  warlock_pet_t* p()
  {
    return static_cast<warlock_pet_t*>( ab::player );
  }
  const warlock_pet_t* p() const
  {
    return static_cast<warlock_pet_t*>( ab::player );
  }

  virtual void execute()
  {
    ab::execute();

    // Some aoe pet abilities can actually reduce to 0 targets, so bail out early if we hit that
    // situation
    if ( ab::n_targets() != 0 && ab::target_list().size() == 0 )
    {
      return;
    }

    if ( ab::execute_state && ab::result_is_hit( ab::execute_state -> result ) )
    {
        if( p() -> o() -> talents.grimoire_of_synergy -> ok())
        {
            bool procced = p() -> o() -> grimoire_of_synergy_pet -> trigger(); //check for RPPM
            if ( procced ) p() -> o() -> buffs.demonic_synergy -> trigger(); //trigger the buff
        }
    }
  }

  warlock_td_t* td( player_t* t ) const
  {
    return p() -> o() -> get_target_data( t );
  }


};

struct warlock_pet_melee_t: public melee_attack_t
{
  struct off_hand_swing: public melee_attack_t
  {
    off_hand_swing( warlock_pet_t* p, const char* name = "melee_oh" ):
      melee_attack_t( name, p, spell_data_t::nil() )
    {
      school = SCHOOL_PHYSICAL;
      weapon = &( p -> off_hand_weapon );
      base_execute_time = weapon -> swing_time;
      may_crit = true;
      background = true;
      base_multiplier = 0.5;
    }
  };

  off_hand_swing* oh;

  warlock_pet_melee_t( warlock_pet_t* p, const char* name = "melee" ):
    melee_attack_t( name, p, spell_data_t::nil() ), oh( nullptr )
  {
    school = SCHOOL_PHYSICAL;
    weapon = &( p -> main_hand_weapon );
    base_execute_time = weapon -> swing_time;
    may_crit = background = repeating = true;

    if ( p -> dual_wield() )
      oh = new off_hand_swing( p );
  }

  virtual void execute() override
  {
    if ( ! player -> executing && ! player -> channeling )
    {
      melee_attack_t::execute();
      if ( oh )
      {
        oh -> time_to_execute = time_to_execute;
        oh -> execute();
      }
    }
    else
    {
      schedule_execute();
    }
  }
};

struct warlock_pet_melee_attack_t: public warlock_pet_action_t < melee_attack_t >
{
private:
  void _init_warlock_pet_melee_attack_t()
  {
    weapon = &( player -> main_hand_weapon );
    special = true;
  }

public:
  warlock_pet_melee_attack_t( warlock_pet_t* p, const std::string& n ):
    base_t( n, p, p -> find_pet_spell( n ) )
  {
    _init_warlock_pet_melee_attack_t();
  }

  warlock_pet_melee_attack_t( const std::string& token, warlock_pet_t* p, const spell_data_t* s = spell_data_t::nil() ):
    base_t( token, p, s )
  {
    _init_warlock_pet_melee_attack_t();
  }
};

struct warlock_pet_spell_t: public warlock_pet_action_t < spell_t >
{
private:
  void _init_warlock_pet_spell_t()
  {
    parse_spell_coefficient( *this );
  }

public:
  warlock_pet_spell_t( warlock_pet_t* p, const std::string& n ):
    base_t( n, p, p -> find_pet_spell( n ) )
  {
    _init_warlock_pet_spell_t();
  }

  warlock_pet_spell_t( const std::string& token, warlock_pet_t* p, const spell_data_t* s = spell_data_t::nil() ):
    base_t( token, p, s )
  {
    _init_warlock_pet_spell_t();
  }
};

struct rift_shadow_bolt_t: public warlock_pet_spell_t
{
  struct rift_shadow_bolt_tick_t : public warlock_pet_spell_t
  {
    rift_shadow_bolt_tick_t( warlock_pet_t* p ) :
      warlock_pet_spell_t( "shadow_bolt", p, p -> find_spell( 196657 ) )
    {
      background = dual = true;
      base_execute_time = timespan_t::zero();
    }

    virtual double composite_target_multiplier( player_t* target ) const override
    {
      double m = warlock_pet_spell_t::composite_target_multiplier( target );

      if ( target == p() -> o() -> havoc_target && p() -> o() -> legendary.odr_shawl_of_the_ymirjar )
        m *= 1.0 + p() -> find_spell( 212173 ) -> effectN( 1 ).percent();

      return m;
    }
  };

  rift_shadow_bolt_t( warlock_pet_t* p ) :
    warlock_pet_spell_t( "shadow_bolt", p, p -> find_spell( 196657 ) )
  {
    may_crit = may_miss = false;
    tick_may_crit = hasted_ticks = true;
    spell_power_mod.direct = 0;
    base_dd_min = base_dd_max = 0;
    dot_duration = timespan_t::from_millis( 14000 );
    base_tick_time = timespan_t::from_millis( 2000 );
    base_execute_time = timespan_t::zero();

    tick_action = new rift_shadow_bolt_tick_t( p );
  }

  void execute() override
  {
    warlock_pet_spell_t::execute();

    cooldown -> start( timespan_t::from_seconds( 16 ) );
  }
};

struct chaos_barrage_t : public warlock_pet_spell_t
{
  struct chaos_barrage_tick_t : public warlock_pet_spell_t
  {
    chaos_barrage_tick_t( warlock_pet_t* p ) :
      warlock_pet_spell_t( "chaos_barrage", p, p -> find_spell( 187394 ) )
    {
      background = dual = true;
      base_execute_time = timespan_t::zero();
    }

    virtual double composite_target_multiplier( player_t* target ) const override
    {
      double m = warlock_pet_spell_t::composite_target_multiplier( target );

      if ( target == p() -> o() -> havoc_target && p() -> o() -> legendary.odr_shawl_of_the_ymirjar )
        m *= 1.0 + p() -> find_spell( 212173 ) -> effectN( 1 ).percent();

      return m;
    }
  };

  chaos_barrage_t( warlock_pet_t* p ) :
    warlock_pet_spell_t( "chaos_barrage", p, p -> find_spell( 187394 ) )
  {
    may_crit = may_miss = false;
    tick_may_crit = hasted_ticks = true;
    spell_power_mod.direct = 0;
    base_dd_min = base_dd_max = 0;
    dot_duration = timespan_t::from_millis( 5500 );
    base_tick_time = timespan_t::from_millis( 250 );
    base_execute_time = timespan_t::zero();

    tick_action = new chaos_barrage_tick_t( p );
  }

  void execute() override
  {
    warlock_pet_spell_t::execute();

    cooldown -> start( timespan_t::from_seconds( 6 ) );
  }
};

struct rift_chaos_bolt_t : public warlock_pet_spell_t
{
  rift_chaos_bolt_t( warlock_pet_t* p ) :
    warlock_pet_spell_t( "chaos_bolt", p, p -> find_spell( 215279 ) )
  {
    base_execute_time = timespan_t::from_millis( 3000 );
    cooldown->duration = timespan_t::from_seconds( 5.5 );
  }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double m = warlock_pet_spell_t::composite_target_multiplier( target );

    if ( target == p() -> o() -> havoc_target && p() -> o() -> legendary.odr_shawl_of_the_ymirjar )
      m *= 1.0 + p() -> find_spell( 212173 ) -> effectN( 1 ).percent();

    return m;
  }

  // Force spell to always crit
  double composite_crit_chance() const override
  {
    return 1.0;
  }

  double calculate_direct_amount( action_state_t* state ) const override
  {
    warlock_pet_spell_t::calculate_direct_amount( state );

    // Can't use player-based crit chance from the state object as it's hardcoded to 1.0. Use cached
    // player spell crit instead. The state target crit chance of the state object is correct.
    // Targeted Crit debuffs function as a separate multiplier.
    state -> result_total *= 1.0 + player -> cache.spell_crit_chance() + state -> target_crit_chance;

    return state -> result_total;
  }
};

struct firebolt_t: public warlock_pet_spell_t
{
  firebolt_t( warlock_pet_t* p ):
    warlock_pet_spell_t( "Firebolt", p, p -> find_spell( 3110 ) )
  {
    base_multiplier *= 1.0 + p -> o() -> artifact.impish_incineration.percent();
  }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double m = warlock_pet_spell_t::composite_target_multiplier( target );

    warlock_td_t* td = this -> td( target );

    double immolate = 0;
    double multiplier = p() -> o() -> spec.firebolt_2 -> effectN( 1 ).percent();

    if( td -> dots_immolate -> is_ticking() )
      immolate += multiplier;

    m *= 1.0 + immolate;

    return m;
  }
};

struct dreadbite_t : public warlock_pet_melee_attack_t
{
  timespan_t dreadstalker_duration;
  dreadbite_t( warlock_pet_t* p ) :
    warlock_pet_melee_attack_t( "Dreadbite", p, p -> find_spell( 205196 ) )
  {
    weapon = &( p -> main_hand_weapon );
    dreadstalker_duration = p -> find_spell( 193332 ) -> duration();
    cooldown -> duration = dreadstalker_duration + timespan_t::from_seconds( 1.0 );
  }
};

struct legion_strike_t: public warlock_pet_melee_attack_t
{
  legion_strike_t( warlock_pet_t* p ):
    warlock_pet_melee_attack_t( p, "Legion Strike" )
  {
    aoe = -1;
    weapon = &( p -> main_hand_weapon );
  }

  virtual bool ready() override
  {
    if ( p() -> special_action -> get_dot() -> is_ticking() ) return false;

    return warlock_pet_melee_attack_t::ready();
  }
};

struct felstorm_tick_t: public warlock_pet_melee_attack_t
{
  felstorm_tick_t( warlock_pet_t* p, const spell_data_t& s ):
    warlock_pet_melee_attack_t( "felstorm_tick", p, s.effectN( 1 ).trigger() )
  {
    aoe = -1;
    background = true;
    weapon = &( p -> main_hand_weapon );
  }
};

struct felstorm_t: public warlock_pet_melee_attack_t
{
  felstorm_t( warlock_pet_t* p ) :
    warlock_pet_melee_attack_t( "felstorm", p, p -> find_spell( 89751 ) )
  {
    tick_zero = true;
    hasted_ticks = false;
    may_miss = false;
    may_crit = false;
    weapon_multiplier = 0;

    dynamic_tick_action = true;
    tick_action = new felstorm_tick_t( p, data() );
  }

  virtual void cancel() override
  {
    warlock_pet_melee_attack_t::cancel();

    get_dot() -> cancel();
  }

  virtual void execute() override
  {
    warlock_pet_melee_attack_t::execute();

    p() -> melee_attack -> cancel();
  }

  virtual void last_tick( dot_t* d ) override
  {
    warlock_pet_melee_attack_t::last_tick( d );

    if ( ! p() -> is_sleeping() && ! p() -> melee_attack -> target -> is_sleeping() )
      p() -> melee_attack -> execute();
  }
};

struct shadow_bite_t: public warlock_pet_spell_t
{
  double shadow_bite_mult;
  shadow_bite_t( warlock_pet_t* p ):
    warlock_pet_spell_t( p, "Shadow Bite" ),
    shadow_bite_mult( 0.0 )
  {
    shadow_bite_mult = p -> o() -> spec.shadow_bite_2 -> effectN( 1 ).percent();
  }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double m = warlock_pet_spell_t::composite_target_multiplier( target );

    warlock_td_t* td = this -> td( target );

    double dots = 0;

    for ( int i = 0; i < MAX_UAS; i++ )
      if ( td -> dots_unstable_affliction[i] -> is_ticking() )
        dots += shadow_bite_mult;

    if ( td -> dots_agony -> is_ticking() )
      dots += shadow_bite_mult;

    if ( td -> dots_corruption -> is_ticking() )
      dots += shadow_bite_mult;

    m *= 1.0 + dots;

    return m;
  }
};

struct lash_of_pain_t: public warlock_pet_spell_t
{
  lash_of_pain_t( warlock_pet_t* p ):
    warlock_pet_spell_t( p, "Lash of Pain" )
  {
  }
};

struct whiplash_t: public warlock_pet_spell_t
{
  whiplash_t( warlock_pet_t* p ):
    warlock_pet_spell_t( p, "Whiplash" )
  {
    aoe = -1;
  }
};

struct torment_t: public warlock_pet_spell_t
{
  torment_t( warlock_pet_t* p ):
    warlock_pet_spell_t( p, "Torment" )
  { }
};

struct immolation_tick_t : public warlock_pet_spell_t
{
  immolation_tick_t( warlock_pet_t* p, const spell_data_t& s ) :
    warlock_pet_spell_t( "immolation_tick", p, s.effectN( 1 ).trigger() )
  {
    aoe = -1;
    background = true;
    may_crit = true;
  }
};

struct immolation_t : public warlock_pet_spell_t
{
  immolation_t( warlock_pet_t* p, const std::string& options_str ) :
    warlock_pet_spell_t( "immolation", p, p -> find_spell( 19483 ) )
  {
    parse_options( options_str );

    dynamic_tick_action = hasted_ticks = true;
    tick_action = new immolation_tick_t( p, data() );
  }

  void init() override
  {
    warlock_pet_spell_t::init();

    // Explicitly snapshot haste, as the spell actually has no duration in spell data
    snapshot_flags |= STATE_HASTE;
    update_flags |= STATE_HASTE;
  }

  timespan_t composite_dot_duration( const action_state_t* ) const override
  {
    return player -> sim -> expected_iteration_time * 2;
  }

  virtual void cancel() override
  {
    dot_t* dot = find_dot( target );
    if ( dot && dot -> is_ticking() )
    {
      dot -> cancel();
    }
    action_t::cancel();
  }
};

struct doom_bolt_t: public warlock_pet_spell_t
{
  doom_bolt_t( warlock_pet_t* p ):
    warlock_pet_spell_t( "Doom Bolt", p, p -> find_spell( 85692 ) )
  {
    if ( p -> o() -> talents.grimoire_of_supremacy -> ok() )
      base_multiplier *= 1.0 + p -> o() -> artifact.impish_incineration.data().effectN( 2 ).percent();

    if ( maybe_ptr( p -> o() -> dbc.ptr ) && p -> o() -> talents.grimoire_of_supremacy -> ok() ) //FIXME spelldata?
      base_multiplier *= 0.8;
  }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double m = warlock_pet_spell_t::composite_target_multiplier( target );

    if ( target -> health_percentage() < 20 )
    {
      m *= 1.0 + data().effectN( 2 ).percent();
    }
    return m;
  }
};

struct meteor_strike_t: public warlock_pet_spell_t
{
  meteor_strike_t( warlock_pet_t* p, const std::string& options_str ):
    warlock_pet_spell_t( "Meteor Strike", p, p -> find_spell( 171018 ) )
  {
    parse_options( options_str );
    aoe = -1;
  }

  void execute() override
  {
    warlock_pet_spell_t::execute();

    if ( p() -> o() -> artifact.lord_of_flames.rank() &&
         p() -> o() -> talents.grimoire_of_supremacy -> ok() &&
         ! p() -> o() -> buffs.lord_of_flames -> up() )
    {
      p() -> o() -> trigger_lof_infernal();
      p() -> o() -> buffs.lord_of_flames -> trigger();
    }
  }
};

struct fel_firebolt_t: public warlock_pet_spell_t
{
  fel_firebolt_t( warlock_pet_t* p ):
    warlock_pet_spell_t( "fel_firebolt", p, p -> find_spell( 104318 ) )
  {
      base_multiplier *= 1.0 + p -> o() -> artifact.infernal_furnace.percent();
      this -> base_crit += p -> o() -> artifact.imperator.percent();
  }

  virtual bool ready() override
  {
    return spell_t::ready();
  }


  void execute() override
  {
    warlock_pet_spell_t::execute();

    if ( p() -> o() -> specialization() == WARLOCK_DEMONOLOGY && p() -> o() -> artifact.stolen_power.rank() )
    {
      p() -> o() -> buffs.stolen_power_stacks -> trigger();
    }
  }


  virtual void impact( action_state_t* s ) override
  {
    warlock_pet_spell_t::impact( s );
    if ( result_is_hit( s -> result ) )
    {
      if ( rng().roll( p() -> o() -> sets.set( WARLOCK_DEMONOLOGY, T18, B4 ) -> effectN( 1 ).percent() ) )
      {
        p() -> o() -> buffs.t18_4pc_driver -> trigger();
      }
    }
  }
};

struct eye_laser_t : public warlock_pet_spell_t
{
  eye_laser_t( warlock_pet_t* p ) :
    warlock_pet_spell_t( "eye_laser", p, p -> find_spell( 205231 ) )
  { }

  void schedule_travel( action_state_t* state ) override
  {
    if ( td( state -> target ) -> dots_doom -> is_ticking() )
    {
      warlock_pet_spell_t::schedule_travel( state );
    }
    // Need to release the state since it's not going to be used by a travel event, nor impacting of
    // any kind.
    else
    {
      action_state_t::release( state );
    }
  }
};

struct soul_effigy_spell_t : public warlock_pet_spell_t
{
  soul_effigy_t *effi;
  soul_effigy_spell_t( warlock_pet_t* p , soul_effigy_t *effi) :
    warlock_pet_spell_t( "soul_effigy", p, p -> find_spell( 205260 ) )
  {
    this->effi = effi;
    background = true;
    may_miss = may_crit = false;
  }

  void init() override
  {
    warlock_pet_spell_t::init();
    snapshot_flags = STATE_TGT_MUL_DA;
    update_flags = 0;
  }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double m = warlock_pet_spell_t::composite_target_multiplier( target );

    warlock_td_t* td = this -> td( target );

    if ( p() -> o() -> talents.contagion -> ok() )
    {
      for ( int i = 0; i < MAX_UAS; i++ )
      {
        if ( td -> dots_unstable_affliction[i] -> is_ticking() )
        {
          m *= 1.0 + p() -> o() -> talents.contagion -> effectN( 1 ).percent();
          break;
        }
      }
    }

    return m;
  }

  void execute() override
  {
    this->p()->o()->trigger_effigy(effi);
  }
};

//} // pets::actions

warlock_pet_t::warlock_pet_t( sim_t* sim, warlock_t* owner, const std::string& pet_name, pet_e pt, bool guardian ):
pet_t( sim, owner, pet_name, pt, guardian ), special_action( nullptr ), special_action_two( nullptr ), melee_attack( nullptr ), summon_stats( nullptr )
{
  owner_coeff.ap_from_sp = 1.0;
  owner_coeff.sp_from_sp = 1.0;
  owner_coeff.health = 0.5;
  command = find_spell( 21563 );
}

void warlock_pet_t::init_base_stats()
{
  pet_t::init_base_stats();

  resources.base[RESOURCE_ENERGY] = 200;
  base_energy_regen_per_second = 10;

  base.spell_power_per_intellect = 1;

  intellect_per_owner = 0;
  stamina_per_owner = 0;

  main_hand_weapon.type = WEAPON_BEAST;

  //double dmg = dbc.spell_scaling( owner -> type, owner -> level );

  main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
}

void warlock_pet_t::init_action_list()
{
  if ( special_action )
  {
    if ( type == PLAYER_PET )
      special_action -> background = true;
    else
      special_action -> action_list = get_action_priority_list( "default" );
  }

  if ( special_action_two )
  {
    if ( type == PLAYER_PET )
      special_action_two -> background = true;
    else
      special_action_two -> action_list = get_action_priority_list( "default" );
  }

  pet_t::init_action_list();

  if ( summon_stats )
    for ( size_t i = 0; i < action_list.size(); ++i )
      summon_stats -> add_child( action_list[i] -> stats );
}

void warlock_pet_t::create_buffs()
{
  pet_t::create_buffs();

  buffs.demonic_synergy = buff_creator_t( this, "demonic_synergy", find_spell( 171982 ) )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    .chance( 1 );

  buffs.demonic_empowerment = haste_buff_creator_t( this, "demonic_empowerment", find_spell( 193396 ) )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    .chance( 1 );

  buffs.the_expendables = buff_creator_t( this, "the_expendables", find_spell( 211218 ))
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    .chance( 1 )
    .default_value( find_spell( 211218 ) -> effectN( 1 ).percent() );
}

void warlock_pet_t::schedule_ready( timespan_t delta_time, bool waiting )
{
  dot_t* d;
  if ( melee_attack && ! melee_attack -> execute_event && ! ( special_action && ( d = special_action -> get_dot() ) && d -> is_ticking() ) )
  {
    melee_attack -> schedule_execute();
  }

  pet_t::schedule_ready( delta_time, waiting );
}

double warlock_pet_t::composite_player_multiplier( school_e school ) const
{
  double m = pet_t::composite_player_multiplier( school );

  if ( o() -> race == RACE_ORC )
    m *= 1.0 + command -> effectN( 1 ).percent();

  m *= 1.0 + o() -> buffs.tier18_2pc_demonology -> stack_value();

  if ( buffs.demonic_synergy -> up() )
    m *= 1.0 + buffs.demonic_synergy -> data().effectN( 1 ).percent();

  if( buffs.the_expendables -> up() )
  {
      m*= 1.0 + buffs.the_expendables -> stack_value();
  }

  if ( o() -> buffs.soul_harvest -> check() )
    m *= 1.0 + o() -> talents.soul_harvest -> effectN( 1 ).percent();

  if ( is_grimoire_of_service )
  {
      m *= 1.0 + o() -> find_spell( 216187 ) -> effectN( 1 ).percent();
  }

  if( o() -> mastery_spells.master_demonologist -> ok() && buffs.demonic_empowerment -> check() )
  {
     m *= 1.0 +  o() -> cache.mastery_value();
  }

  if ( o() -> bugs && pet_type != PET_WILD_IMP ) //FIXME sindorei is currently not buffing imps.
    m *= 1.0 + o() -> buffs.sindorei_spite -> check_stack_value();

  return m;
}

double warlock_pet_t::composite_melee_crit_chance() const
{
  double mc = pet_t::composite_melee_crit_chance();
  return mc;
}

double warlock_pet_t::composite_spell_crit_chance() const
{
  double sc = pet_t::composite_spell_crit_chance();

  return sc;
}

double warlock_pet_t::composite_melee_haste() const
{
  double mh = pet_t::composite_melee_haste();

  if ( buffs.demonic_empowerment -> up() )
  {
    mh /= 1.0 + buffs.demonic_empowerment -> data().effectN( 2 ).percent() + o() -> artifact.summoners_prowess.percent();
  }

  return mh;
}

double warlock_pet_t::composite_spell_haste() const
{
  double sh = pet_t::composite_spell_haste();

  if ( buffs.demonic_empowerment -> up() )
    sh /= 1.0 + buffs.demonic_empowerment -> data().effectN( 2 ).percent() + o() -> artifact.summoners_prowess.percent();

  return sh;
}

double warlock_pet_t::composite_melee_speed() const
{
  // Make sure we get our overridden haste values applied to melee_speed
  double cmh =  player_t::composite_melee_speed();

  return cmh;
}

double warlock_pet_t::composite_spell_speed() const
{
  // Make sure we get our overridden haste values applied to spell_speed
  double css = player_t::composite_spell_speed();


  return css;
}

struct imp_pet_t: public warlock_pet_t
{
  imp_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "imp" ):
    warlock_pet_t( sim, owner, name, PET_IMP, name != "imp" )
  {
    action_list_str = "firebolt";
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "firebolt" ) return new firebolt_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct felguard_pet_t: public warlock_pet_t
{
  felguard_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "felguard" ):
    warlock_pet_t( sim, owner, name, PET_FELGUARD, name != "felguard" )
  {
    action_list_str += "/felstorm";
    action_list_str += "/legion_strike,if=cooldown.felstorm.remains";
    owner_coeff.ap_from_sp = 1.1; // HOTFIX
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();

    melee_attack = new warlock_pet_melee_t( this );
    special_action = new felstorm_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "legion_strike" ) return new legion_strike_t( this );
    if ( name == "felstorm" ) return new felstorm_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct t18_illidari_satyr_t: public warlock_pet_t
{
  t18_illidari_satyr_t(sim_t* sim, warlock_t* owner ):
    warlock_pet_t( sim, owner, "illidari_satyr", PET_FELGUARD, true )
  {
    owner_coeff.ap_from_sp = 1;
    is_demonbolt_enabled = false;
    regen_type = REGEN_DISABLED;
    action_list_str = "travel";
  }

  void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();
    base_energy_regen_per_second = 0;
    melee_attack = new warlock_pet_melee_t( this );
    if ( o() -> warlock_pet_list.t18_illidari_satyr[0] )
      melee_attack -> stats = o() -> warlock_pet_list.t18_illidari_satyr[0] -> get_stats( "melee" );
  }
};

struct t18_prince_malchezaar_t: public warlock_pet_t
{
  t18_prince_malchezaar_t(  sim_t* sim, warlock_t* owner ):
    warlock_pet_t( sim, owner, "prince_malchezaar", PET_GHOUL, true )
  {
    owner_coeff.ap_from_sp = 1;
    regen_type = REGEN_DISABLED;
    action_list_str = "travel";
  }

  void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();
    base_energy_regen_per_second = 0;
    melee_attack = new warlock_pet_melee_t( this );
    if ( o() -> warlock_pet_list.t18_prince_malchezaar[0] )
      melee_attack -> stats = o() -> warlock_pet_list.t18_prince_malchezaar[0] -> get_stats( "melee" );
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = warlock_pet_t::composite_player_multiplier( school );
    m *= 9.45; // Prince deals 9.45 times normal damage.. you know.. for reasons.
    return m;
  }
};

struct t18_vicious_hellhound_t: public warlock_pet_t
{
  t18_vicious_hellhound_t( sim_t* sim, warlock_t* owner ):
    warlock_pet_t( sim, owner, "vicious_hellhound", PET_DOG, true )
  {
    owner_coeff.ap_from_sp = 1;
    is_demonbolt_enabled = false;
    regen_type = REGEN_DISABLED;
    action_list_str = "travel";
  }

  void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();
    base_energy_regen_per_second = 0;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.0 );
    melee_attack = new warlock_pet_melee_t( this );
    melee_attack -> base_execute_time = timespan_t::from_seconds( 1.0 );
    if ( o() -> warlock_pet_list.t18_vicious_hellhound[0] )
      melee_attack -> stats = o() -> warlock_pet_list.t18_vicious_hellhound[0] -> get_stats( "melee" );
  }
};

struct chaos_tear_t : public warlock_pet_t
{
  stats_t** chaos_bolt_stats;
  stats_t* regular_stats;

  chaos_tear_t( sim_t* sim, warlock_t* owner ) :
    warlock_pet_t( sim, owner, "chaos_tear", PET_NONE, true ), chaos_bolt_stats( nullptr ), regular_stats(nullptr)
  {
    action_list_str = "chaos_bolt";
    regen_type = REGEN_DISABLED;
  }

  void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();
    base_energy_regen_per_second = 0;
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str ) override
  {
    if ( name == "chaos_bolt" )
    {
      action_t* a = new rift_chaos_bolt_t( this );
      chaos_bolt_stats = &( a -> stats );
      if ( this == o() -> warlock_pet_list.chaos_tear[0] || sim -> report_pets_separately )
      {
        regular_stats = a -> stats;
      }
      else
      {
        regular_stats = o() -> warlock_pet_list.chaos_tear[0] -> get_stats( "chaos_bolt" );
        *chaos_bolt_stats = regular_stats;
      }
      return a;
    }

    return warlock_pet_t::create_action( name, options_str );
  }
};

namespace shadowy_tear {

  struct shadowy_tear_t;

  struct shadowy_tear_td_t : public actor_target_data_t
  {
    dot_t* dots_shadow_bolt;

  public:
    shadowy_tear_td_t( player_t* target, shadowy_tear_t* shadowy );
  };

  struct shadowy_tear_t : public warlock_pet_t
  {
    stats_t** shadow_bolt_stats;
    stats_t* regular_stats;
    target_specific_t<shadowy_tear_td_t> target_data;

    shadowy_tear_t( sim_t* sim, warlock_t* owner ) :
      warlock_pet_t( sim, owner, "shadowy_tear", PET_NONE, true ), shadow_bolt_stats( nullptr ), regular_stats( nullptr )
    {
      action_list_str = "shadow_bolt";
      regen_type = REGEN_DISABLED;
    }

    void init_base_stats() override
    {
      warlock_pet_t::init_base_stats();
      base_energy_regen_per_second = 0;
    }

    shadowy_tear_td_t* td( player_t* t ) const
    {
      return get_target_data( t );
    }

    virtual shadowy_tear_td_t* get_target_data( player_t* target ) const override
    {
      shadowy_tear_td_t*& td = target_data[target];
      if ( !td )
        td = new shadowy_tear_td_t( target, const_cast< shadowy_tear_t* >( this ) );
      return td;
    }

    virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
    {
      if ( name == "shadow_bolt" )
      {
        action_t* a = new rift_shadow_bolt_t( this );
        shadow_bolt_stats = &( a -> stats );
        if ( this == o() -> warlock_pet_list.shadowy_tear[0] || sim -> report_pets_separately )
        {
          regular_stats = a -> stats;
        }
        else
        {
          regular_stats = o() -> warlock_pet_list.shadowy_tear[0] -> get_stats( "shadow_bolt" );
          *shadow_bolt_stats = regular_stats;
        }
        return a;
      }

      return warlock_pet_t::create_action( name, options_str );
    }
  };

  shadowy_tear_td_t::shadowy_tear_td_t( player_t* target, shadowy_tear_t* shadowy ):
    actor_target_data_t( target, shadowy )
  {
    dots_shadow_bolt = target -> get_dot( "shadow_bolt", shadowy );
  }
}

namespace chaos_portal {

  struct chaos_portal_t;

  struct chaos_portal_td_t : public actor_target_data_t
  {
    dot_t* dots_chaos_barrage;

  public:
    chaos_portal_td_t( player_t* target, chaos_portal_t* chaosy );
  };

  struct chaos_portal_t : public warlock_pet_t
  {
    target_specific_t<chaos_portal_td_t> target_data;
    stats_t** chaos_barrage_stats;
    stats_t* regular_stats;

    chaos_portal_t( sim_t* sim, warlock_t* owner ) :
      warlock_pet_t( sim, owner, "chaos_portal", PET_NONE, true ), chaos_barrage_stats( nullptr ), regular_stats( nullptr )
    {
      action_list_str = "chaos_barrage";
      regen_type = REGEN_DISABLED;
    }

    void init_base_stats() override
    {
      warlock_pet_t::init_base_stats();
      base_energy_regen_per_second = 0;
    }

    chaos_portal_td_t* td( player_t* t ) const
    {
      return get_target_data( t );
    }

    virtual chaos_portal_td_t* get_target_data( player_t* target ) const override
    {
      chaos_portal_td_t*& td = target_data[target];
      if ( !td )
        td = new chaos_portal_td_t( target, const_cast< chaos_portal_t* >( this ) );
      return td;
    }

    virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
    {
      if ( name == "chaos_barrage" )
      {
        action_t* a = new chaos_barrage_t( this );
        chaos_barrage_stats = &( a -> stats );
        if ( this == o() -> warlock_pet_list.chaos_portal[0] || sim -> report_pets_separately )
        {
          regular_stats = a -> stats;
        }
        else
        {
          regular_stats = o() -> warlock_pet_list.chaos_portal[0] -> get_stats( "chaos_barrage" );
          *chaos_barrage_stats = regular_stats;
        }
        return a;
      }

      return warlock_pet_t::create_action( name, options_str );
    }
  };

  chaos_portal_td_t::chaos_portal_td_t( player_t* target, chaos_portal_t* chaosy ):
    actor_target_data_t( target, chaosy )
  {
    dots_chaos_barrage = target -> get_dot( "chaos_barrage", chaosy );
  }
}

struct felhunter_pet_t: public warlock_pet_t
{
  felhunter_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "felhunter" ):
    warlock_pet_t( sim, owner, name, PET_FELHUNTER, name != "felhunter" )
  {
    action_list_str = "shadow_bite";
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();

    melee_attack = new warlock_pet_melee_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "shadow_bite" ) return new shadow_bite_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct succubus_pet_t: public warlock_pet_t
{
  succubus_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "succubus" ):
    warlock_pet_t( sim, owner, name, PET_SUCCUBUS, name != "succubus" )
  {
    action_list_str = "lash_of_pain";
    owner_coeff.ap_from_sp = 0.5;
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();

    main_hand_weapon.swing_time = timespan_t::from_seconds( 3.0 );
    melee_attack = new warlock_pet_melee_t( this );
    if ( ! util::str_compare_ci( name_str, "service_succubus" ) )
      special_action = new whiplash_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "lash_of_pain" ) return new lash_of_pain_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct voidwalker_pet_t: public warlock_pet_t
{
  voidwalker_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "voidwalker" ):
    warlock_pet_t( sim, owner, name, PET_VOIDWALKER, name != "voidwalker" )
  {
    action_list_str = "torment";
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();

    melee_attack = new warlock_pet_melee_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "torment" ) return new torment_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct infernal_t: public warlock_pet_t
{
  infernal_t( sim_t* sim, warlock_t* owner ):
    warlock_pet_t( sim, owner, "infernal", PET_INFERNAL )
  {
    owner_coeff.health = 0.4;
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();
    action_list_str = "immolation,if=!ticking";
    if ( o() -> talents.grimoire_of_supremacy -> ok() )
      action_list_str += "/meteor_strike,if=time>1";
    melee_attack = new warlock_pet_melee_t( this );

    resources.base[RESOURCE_ENERGY] = 0;
    base_energy_regen_per_second = 0;
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "immolation" ) return new immolation_t( this, options_str );
    if ( name == "meteor_strike" ) return new meteor_strike_t( this, options_str );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct lord_of_flames_infernal_t : public warlock_pet_t
{
  timespan_t duration;

  lord_of_flames_infernal_t( sim_t* sim, warlock_t* owner ) :
    warlock_pet_t( sim, owner, "lord_of_flames_infernal", PET_INFERNAL )
  {
    duration = o() -> find_spell( 226804 ) -> duration() + timespan_t::from_millis( 1 );
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();
    action_list_str = "immolation,if=!ticking";
    resources.base[RESOURCE_ENERGY] = 0;
    base_energy_regen_per_second = 0;
    melee_attack = new warlock_pet_melee_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "immolation" ) return new immolation_t( this, options_str );

    return warlock_pet_t::create_action( name, options_str );
  }

  void trigger()
  {
    if ( ! o() -> buffs.lord_of_flames -> up() )
      summon( duration );
  }
};

struct doomguard_t: public warlock_pet_t
{
    doomguard_t( sim_t* sim, warlock_t* owner ):
    warlock_pet_t( sim, owner, "doomguard", PET_DOOMGUARD )
  {
    owner_coeff.health = 0.4;
    action_list_str = "doom_bolt";
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();

    resources.base[RESOURCE_ENERGY] = 100;
    base_energy_regen_per_second = 12;
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "doom_bolt" ) return new doom_bolt_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct wild_imp_pet_t: public warlock_pet_t
{
  action_t* firebolt;
  stats_t** fel_firebolt_stats;
  stats_t* regular_stats;
  bool isnotdoge;

  wild_imp_pet_t( sim_t* sim, warlock_t* owner ):
    warlock_pet_t( sim, owner, "wild_imp", PET_WILD_IMP ), fel_firebolt_stats( nullptr ),
    regular_stats(nullptr)
  {
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();

    action_list_str = "fel_firebolt";

    resources.base[RESOURCE_ENERGY] = 1000;
    base_energy_regen_per_second = 0;
  }

  void dismiss( bool expired ) override
  {
      if( expired && o() -> artifact.the_expendables.rank() )
      {
          for( auto& pet : o() -> pet_list )
          {
              pets::warlock_pet_t *lock_pet = static_cast<pets::warlock_pet_t*> ( pet );
              if( lock_pet && !lock_pet -> is_sleeping() && lock_pet != this )
              {
                  lock_pet -> buffs.the_expendables -> bump( 1,
                              buffs.the_expendables -> data().effectN( 1 ).percent() );
                  o() -> procs.the_expendables -> occur();
              }
          }
      }
      pet_t::dismiss( expired );
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str ) override
  {
    if ( name == "fel_firebolt" )
    {
      firebolt = new fel_firebolt_t( this );
      fel_firebolt_stats = &( firebolt -> stats );
      if ( this == o() -> warlock_pet_list.wild_imps[ 0 ] || sim -> report_pets_separately )
      {
        regular_stats = firebolt -> stats;
      }
      else
      {
        regular_stats = o() -> warlock_pet_list.wild_imps[ 0 ] -> get_stats( "fel_firebolt" );
      }
      return firebolt;
    }

    return warlock_pet_t::create_action( name, options_str );
  }

  void arise() override
  {
      warlock_pet_t::arise();

      if( isnotdoge )
      {
         firebolt -> cooldown -> start( timespan_t::from_millis( rng().range( 500 , 1500 )));
      }
  }

  void trigger(bool isdoge = false)
  {
    isnotdoge = !isdoge;
    *fel_firebolt_stats = regular_stats;
    summon( timespan_t::from_millis( 12001 ) );
  }
};

struct dreadstalker_t : public warlock_pet_t
{
    stats_t** dreadbite_stats;
    stats_t* regular_stats;

  dreadstalker_t( sim_t* sim, warlock_t* owner ) :
    warlock_pet_t( sim, owner, "dreadstalker", PET_DREADSTALKER ), dreadbite_stats( nullptr ), regular_stats(nullptr)
  {
    action_list_str = "travel/dreadbite";
    regen_type = REGEN_DISABLED;
    owner_coeff.health = 0.4;
    owner_coeff.ap_from_sp = 1.1; // HOTFIX
  }

  virtual double composite_melee_crit_chance() const override
  {
      double pw = warlock_pet_t::composite_melee_crit_chance();
      pw += o() -> artifact.sharpened_dreadfangs.percent();

      return pw;
  }

  virtual double composite_spell_crit_chance() const override
  {
      double pw = warlock_pet_t::composite_spell_crit_chance();
      pw += o() -> artifact.sharpened_dreadfangs.percent();
      return pw;
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = warlock_pet_t::composite_player_multiplier( school );
    m *= 0.76; // FIXME dreadstalkers do 76% damage for no apparent reason, thanks blizzard.
    return m;
  }

  void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();
    resources.base[RESOURCE_ENERGY] = 0;
    base_energy_regen_per_second = 0;
    melee_attack = new warlock_pet_melee_t( this );
    if ( o() -> warlock_pet_list.dreadstalkers[0] )
      melee_attack -> stats = o() ->warlock_pet_list.dreadstalkers[0] -> get_stats( "melee" );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "dreadbite" )
    {
      action_t* a = new dreadbite_t( this );
      dreadbite_stats = &( a -> stats );
      if ( this == o() ->warlock_pet_list.dreadstalkers[0] || sim -> report_pets_separately )
      {
        regular_stats = a -> stats;
      }
      else
      {
        regular_stats = o() ->warlock_pet_list.dreadstalkers[0] -> get_stats( "dreadbite" );
        *dreadbite_stats = regular_stats;
      }
      return a;
    }

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct darkglare_t : public warlock_pet_t
{
  darkglare_t( sim_t* sim, warlock_t* owner ) :
    warlock_pet_t( sim, owner, "darkglare", PET_OBSERVER )
  {
    action_list_str = "eye_laser";
    regen_type = REGEN_DISABLED;
    owner_coeff.health = 0.4;
  }

  void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();
    resources.base[RESOURCE_ENERGY] = 0;
    base_energy_regen_per_second = 0;
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "eye_laser" ) return new eye_laser_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct soul_effigy_t : public warlock_pet_t
{
  const spell_data_t* soul_effigy_passive;
  soul_effigy_spell_t* damage;

  soul_effigy_t( warlock_t* owner ) :
    warlock_pet_t( owner -> sim, owner, "soul_effigy", PET_WARLOCK ),
    soul_effigy_passive( owner -> find_spell( 205247 ) ), damage( nullptr )
  {
    regen_type = REGEN_DISABLED;
  }

  bool create_actions() override
  {
    damage = new soul_effigy_spell_t( this, this ); //lmfao

    return warlock_pet_t::create_actions();
  }

  void arise() override
  {
    warlock_pet_t::arise();

    // TODO: Does this need to loop pets too?
    range::for_each( owner -> action_list, []( action_t* a ) { a -> target_cache.is_valid = false; } );
  }

  void demise() override
  {
    warlock_pet_t::demise();

    // TODO: Does this need to loop pets too?
    range::for_each( owner -> action_list, []( action_t* a ) { a -> target_cache.is_valid = false; } );
  }

  // Soul Effigy does not run pet_t::assess_damage (that has aoe avoidance)
  void assess_damage( school_e school, dmg_e type, action_state_t* s ) override
  { player_t::assess_damage( school, type, s ); }

  // Damage the bound target (target is bound by the warlock soul_effigy_t action upon summon).
  void do_damage( action_state_t* incoming_state ) override
  {
    warlock_pet_t::do_damage( incoming_state );

    if ( incoming_state -> result_amount > 0 )
    {
      double multiplier = o()->composite_player_target_multiplier(o()->target, incoming_state -> action -> get_school() );
      double amount = soul_effigy_passive -> effectN( 1 ).percent() * incoming_state -> result_amount * multiplier;
      damage -> target = target;
      damage -> base_dd_min = damage -> base_dd_max = amount;
      damage -> execute();
    }
  }
};

} // end namespace pets

// Spells
namespace actions {

struct warlock_heal_t: public heal_t
{
  warlock_heal_t( const std::string& n, warlock_t* p, const uint32_t id ):
    heal_t( n, p, p -> find_spell( id ) )
  {
    target = p;
  }

  warlock_t* p()
  {
    return static_cast<warlock_t*>( player );
  }
};

struct warlock_spell_t: public spell_t
{
private:
  void _init_warlock_spell_t()
  {
    may_crit = true;
    tick_may_crit = true;
    weapon_multiplier = 0.0;
    gain = player -> get_gain( name_str );

    can_havoc = false;
    if ( aoe == 0 )
      can_havoc = true;

    affected_by_contagion = true;
    destro_mastery = true;

    parse_spell_coefficient( *this );
  }

public:
  gain_t* gain;

  mutable std::vector< player_t* > havoc_targets;
  bool can_havoc;

  bool affected_by_contagion;
  bool affected_by_flamelicked;
  bool affected_by_odr_shawl_of_the_ymirjar;
  bool destro_mastery;

  // Warlock module overrides the "target" option handling to properly target their own Soul Effigy
  // if it's enabled
  std::string target_str_override;

  warlock_spell_t( warlock_t* p, const std::string& n ):
    spell_t( n, p, p -> find_class_spell( n ) )
  {
    _init_warlock_spell_t();
    add_option( opt_string( "target", target_str_override ) );
  }

  warlock_spell_t( const std::string& token, warlock_t* p, const spell_data_t* s = spell_data_t::nil() ):
    spell_t( token, p, s )
  {
    _init_warlock_spell_t();
    add_option( opt_string( "target", target_str_override ) );
  }

  warlock_t* p()
  {
    return static_cast<warlock_t*>( player );
  }
  const warlock_t* p() const
  {
    return static_cast<warlock_t*>( player );
  }

  warlock_td_t* td( player_t* t ) const
  {
    return p() -> get_target_data( t );
  }

  bool use_havoc() const
  {
    if ( ! p() -> havoc_target || target == p() -> havoc_target || ! can_havoc )
      return false;

    return true;
  }

  bool init_finished() override
  {
    if ( ! target_str_override.empty() )
    {
      // "target=soul_effigy" overrides default sim target option parsing for actions
      if ( util::str_compare_ci( target_str_override, "soul_effigy" ) )
      {
        // If soul effigy is enabled, skip global sim target parsing, and instead target the
        // warlock's own soul effigy.
        if ( p() -> talents.soul_effigy -> ok() && p() -> warlock_pet_list.soul_effigy )
        {
          default_target = target = p() -> warlock_pet_list.soul_effigy;
        }
        // Soul Effigy not used / not talented, suppress the action using it
        else
        {
          background = true;
        }
      }
      // .. otherwise, parse the target option using the global target option parsing
      else
      {
        target_str = target_str_override;
        parse_target_str();
        // Setting default target here is necessary, as it has been done earlier on in the action
        // init process (in action_t::init()).
        default_target = target;
      }
    }

    return spell_t::init_finished();
  }

  void reset() override
  {
    spell_t::reset();
  }

  void init() override
  {
    action_t::init();

    if ( p() -> destruction_trinket )
    {
      affected_by_flamelicked = data().affected_by( p() -> destruction_trinket -> driver() -> effectN( 1 ).trigger() -> effectN( 1 ) );
    }
    else
    {
      affected_by_flamelicked = false;
    }

    affected_by_odr_shawl_of_the_ymirjar = data().affected_by( p() -> find_spell( 212173 ) -> effectN( 1 ) );
  }

  int n_targets() const override
  {
    if ( aoe == 0 && use_havoc() )
      return 2;

    return spell_t::n_targets();
  }

  void record_data( action_state_t* state ) override
  {
    if ( state -> target == p() -> warlock_pet_list.soul_effigy )
    {
      return;
    }

    spell_t::record_data( state );
  }

  size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    spell_t::available_targets( tl );

    // If Soul Effigy is active, add it to the end of the target list
    // TODO: Should Effigy actually come directly after the "main target"?
    if ( p() -> talents.soul_effigy -> ok() &&
         ! p() -> warlock_pet_list.soul_effigy -> is_sleeping() &&
         n_targets() == 0 )
    {
      if ( range::find( tl, p() -> warlock_pet_list.soul_effigy ) == tl.end() )
      {
        tl.push_back( p() -> warlock_pet_list.soul_effigy );
      }
    }

    return tl.size();
  }

  std::vector< player_t* >& target_list() const override
  {
    if ( use_havoc() )
    {
      if ( ! target_cache.is_valid )
        available_targets( target_cache.list );

      havoc_targets.clear();
      if ( range::find( target_cache.list, target ) != target_cache.list.end() )
        havoc_targets.push_back( target );

      if ( ! p() -> havoc_target -> is_sleeping() &&
           range::find( target_cache.list, p() -> havoc_target ) != target_cache.list.end() )
           havoc_targets.push_back( p() -> havoc_target );
      return havoc_targets;
    }
    else
      return spell_t::target_list();
  }

  double cost() const override
  {
    double c = spell_t::cost();

    return c;
  }

  void execute() override
  {
    spell_t::execute();

    if ( result_is_hit( execute_state -> result ) && p() -> talents.grimoire_of_synergy -> ok() )
    {
      pets::warlock_pet_t* my_pet = static_cast<pets::warlock_pet_t*>( p() -> warlock_pet_list.active ); //get active pet
      if ( my_pet != nullptr )
      {
        bool procced = p() -> grimoire_of_synergy -> trigger();
        if ( procced ) my_pet -> buffs.demonic_synergy -> trigger();
      }
    }
    if ( result_is_hit( execute_state -> result ) && p() -> talents.grimoire_of_sacrifice -> ok() && p() -> buffs.demonic_power -> up() )
    {
      bool procced = p() -> demonic_power_rppm -> trigger();
      if ( procced )
      {
         p() -> active.demonic_power_proc -> target = execute_state -> target;
         p() -> active.demonic_power_proc -> execute();
      }
    }

    if ( !maybe_ptr( p() -> dbc.ptr ) )
      p() -> buffs.mana_tap -> up();
    if ( maybe_ptr( p() -> dbc.ptr ) )
      p() -> buffs.empowered_life_tap -> up();

    p() -> buffs.demonic_synergy -> up();
  }

  void consume_resource() override
  {
    spell_t::consume_resource();

    if ( resource_current == RESOURCE_SOUL_SHARD && p() -> talents.soul_conduit -> ok() )
    {
      double soul_conduit_rng = p() -> talents.soul_conduit -> effectN( 1 ).percent();

      for ( int i = 0; i < resource_consumed; i++ )
      {
        if ( rng().roll( soul_conduit_rng ) )
        {
          p() -> resource_gain( RESOURCE_SOUL_SHARD, 1.0, p() -> gains.soul_conduit );
          p()->procs.soul_conduit->occur();
        }
      }
    }
  }

  void tick( dot_t* d ) override
  {
    spell_t::tick( d );

    if ( d -> state -> result > 0 && result_is_hit( d -> state -> result ) && td( d -> target ) -> dots_seed_of_corruption -> is_ticking()
         && id != p() -> spells.seed_of_corruption_aoe -> id )
    {
      accumulate_seed_of_corruption( td( d -> target ), d -> state -> result_amount );
    }

    if ( !maybe_ptr( p() -> dbc.ptr ) )
      p() -> buffs.mana_tap -> up();
    if ( maybe_ptr( p() -> dbc.ptr ) )
      p() -> buffs.empowered_life_tap -> up();

    p() -> buffs.demonic_synergy -> up();

    if ( result_is_hit( d -> state -> result ) && p() -> artifact.reap_souls.rank() )
    {
      bool procced = p() -> tormented_souls_rppm -> trigger(); //check for RPPM
      if ( procced )
        p() -> buffs.tormented_souls -> trigger(); //trigger the buff
    }
  }

  void impact( action_state_t* s ) override
  {
    spell_t::impact( s );

    if ( s -> result_amount > 0 && result_is_hit( s -> result ) && td( s -> target ) -> dots_seed_of_corruption -> is_ticking()
         && id != p() -> spells.seed_of_corruption_aoe -> id )
    {
      accumulate_seed_of_corruption( td( s -> target ), s -> result_amount );
    }

    if ( result_is_hit( s -> result ) && p() -> artifact.reap_souls.rank() )
    {
      bool procced = p() -> tormented_souls_rppm -> trigger(); //check for RPPM
      if ( procced )
        p() -> buffs.tormented_souls -> trigger(); //trigger the buff
    }
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = 1.0;

    warlock_td_t* td = this -> td( t );

    if ( target == p() -> havoc_target && affected_by_odr_shawl_of_the_ymirjar && p() -> legendary.odr_shawl_of_the_ymirjar )
      m*= 1.0 + p() -> find_spell( 212173 ) -> effectN( 1 ).percent();

    if ( p() -> talents.contagion -> ok() && affected_by_contagion )
    {
      for ( int i = 0; i < MAX_UAS; i++ )
      {
        if ( td -> dots_unstable_affliction[i] -> is_ticking() )
        {
          m *= 1.0 + p() -> talents.contagion -> effectN( 1 ).percent();
          break;
        }
      }
    }

    if ( p() -> talents.eradication -> ok() && td -> debuffs_eradication -> check() )
      m *= 1.0 + p() -> find_spell( 196414 ) -> effectN( 1 ).percent();

    if ( maybe_ptr( p() -> dbc.ptr && td -> debuffs_haunt -> check() ) )
      m *= 1.0 + p() -> find_spell( 48181 ) -> effectN( 2 ).percent();

    return spell_t::composite_target_multiplier( t ) * m;
  }

  double action_multiplier() const override
  {
    double pm = spell_t::action_multiplier();

    if( p() -> mastery_spells.chaotic_energies -> ok() && destro_mastery )
    {
      double chaotic_energies_rng = rng().range( 0, p() -> cache.mastery_value() );
      pm *= 1.0 + chaotic_energies_rng;
    }

    return pm;
  }

  resource_e current_resource() const override
  {
    return spell_t::current_resource();
  }

  double composite_target_crit_chance( player_t* target ) const override
  {
    double c = spell_t::composite_target_crit_chance( target );
    if ( affected_by_flamelicked && p() -> destruction_trinket )
      c += td( target ) -> debuffs_flamelicked -> stack_value();

    return c;
  }

  bool consume_cost_per_tick( const dot_t& dot ) override
  {
    bool consume = spell_t::consume_cost_per_tick( dot );

    // resource_e r = current_resource();

    return consume;
  }

  void extend_dot( dot_t* dot, timespan_t extend_duration )
  {
    if ( dot -> is_ticking() )
    {
      //FIXME: This is roughly how it works, but we need more testing
      dot -> extend_duration( extend_duration, dot -> current_action -> dot_duration * 1.5 );
    }
  }

  static void accumulate_seed_of_corruption( warlock_td_t* td, double amount )
  {
    td -> soc_threshold -= amount;

    if ( td -> soc_threshold <= 0 )
      td -> dots_seed_of_corruption -> cancel();
  }

  static void trigger_wild_imp( warlock_t* p, bool doge = false )
  {
    for ( size_t i = 0; i < p -> warlock_pet_list.wild_imps.size(); i++ )
    {
      if ( p -> warlock_pet_list.wild_imps[i] -> is_sleeping() )
      {

        p -> warlock_pet_list.wild_imps[i] -> trigger(doge);
        p -> procs.wild_imp -> occur();
        if( p -> legendary.wilfreds_sigil_of_superior_summoning_flag && !p -> talents.grimoire_of_supremacy -> ok() )
        {
            p -> cooldowns.doomguard -> adjust( p -> legendary.wilfreds_sigil_of_superior_summoning );
            p -> cooldowns.infernal -> adjust( p -> legendary.wilfreds_sigil_of_superior_summoning );
            p -> procs.wilfreds_imp -> occur();
        }
        return;
      }
    }
    //p -> sim -> errorf( "Playerd %s ran out of wild imps.\n", p -> name() );
    //assert( false ); // Will only get here if there are no available imps
  }

};

// Affliction Spells
struct agony_t: public warlock_spell_t
{
  int agony_action_id;
  double chance;

  agony_t( warlock_t* p ):
    warlock_spell_t( p, "Agony" ), agony_action_id(0)
  {
    may_crit = false;

    chance = p -> find_spell( 199282 ) -> proc_chance();
  }

  void init() override
  {
    warlock_spell_t::init();

    if ( p() -> affliction_trinket )
    {
      const spell_data_t* data = p() -> affliction_trinket -> driver();
      double period_value = data -> effectN( 1 ).average( p() -> affliction_trinket -> item ) / 100.0;
      double duration_value = data -> effectN( 2 ).average( p() -> affliction_trinket -> item ) / 100.0;

      base_tick_time *= 1.0 + period_value;
      dot_duration *= 1.0 + duration_value;
    }
  }

  virtual double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    if ( p() -> mastery_spells.potent_afflictions -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    m *= 1.0 + p() -> artifact.inimitable_agony.percent() * ( p() -> buffs.deadwind_harvester -> check() ? 2.0 : 1.0 );

    return m;
  }

  double composite_crit_damage_bonus_multiplier() const override
  {
    double cd = warlock_spell_t::composite_crit_damage_bonus_multiplier();

    cd *= 1.0 + p() -> artifact.perdition.percent() * ( p() -> buffs.deadwind_harvester -> check() ? 2.0 : 1.0 );

    return cd;
  }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double m = warlock_spell_t::composite_target_multiplier( target );

    warlock_td_t* td = this -> td( target );

    m *= td -> agony_stack;

    if ( maybe_ptr( p() -> dbc.ptr ) && p() -> talents.malefic_grasp -> ok() && td -> dots_drain_soul -> is_ticking() )
      m *= 1.0 + p() -> find_spell( 235155 ) -> effectN( 1 ).percent();

    return m;
  }

  virtual void last_tick( dot_t* d ) override
  {
    td( d -> state -> target ) -> agony_stack = 1;

    if ( p() -> get_active_dots( internal_id ) == 1 )
      p() -> agony_accumulator = rng().range( 0.0, 0.99 );


    warlock_spell_t::last_tick( d );
  }

  virtual void tick( dot_t* d ) override
  {
    if ( p() -> talents.writhe_in_agony -> ok() && td( d -> state -> target ) -> agony_stack < ( 20 ) )
      td( d -> state -> target ) -> agony_stack++;
    else if ( td( d -> state -> target ) -> agony_stack < ( 10 ) )
      td( d -> state -> target ) -> agony_stack++;

    td( d -> target ) -> debuffs_agony -> trigger();


    double active_agonies = p() -> get_active_dots( internal_id );
    double accumulator_increment = rng().range( 0.0, p() -> sets.has_set_bonus( WARLOCK_AFFLICTION, T19, B4 ) ? 0.48 : 0.32 ) / sqrt( active_agonies );

    p() -> agony_accumulator += accumulator_increment;

    if ( p() -> agony_accumulator >= 1 )
    {
      p() -> resource_gain( RESOURCE_SOUL_SHARD, 1.0, p() -> gains.agony );
      p() -> agony_accumulator -= 1.0;

      // If going from 0 to 1 shard was a surprise, the player would have to react to it
      if ( p() -> resources.current[RESOURCE_SOUL_SHARD] == 1 )
        p() ->  shard_react = p() -> sim -> current_time() + p() -> total_reaction_time();
      else if ( p() -> resources.current[RESOURCE_SOUL_SHARD] >= 1 )
        p() -> shard_react = p() -> sim -> current_time();
      else
        p() -> shard_react = timespan_t::max();
    }

    if ( p() -> sets.has_set_bonus( WARLOCK_AFFLICTION, T18, B4 ) )
    {
      bool procced = p() -> misery_rppm -> trigger(); //check for RPPM
      if ( procced )
        p() -> buffs.misery -> trigger(); //trigger the buff
    }

    if ( p() -> artifact.compounding_horror.rank() )
    {
      if ( rng().roll( p() -> buffs.deadwind_harvester -> check() ? chance * 2.0 : chance ) )
      {
        p() -> buffs.compounding_horror -> trigger();
      }
    }

    warlock_spell_t::tick( d );
  }
};

const int ua_spells[5] = {
  233490, 233496, 233497, 233498, 233499
};

struct unstable_affliction_t: public warlock_spell_t
{
  struct real_ua_t : public warlock_spell_t
  {
    int self;
    real_ua_t( warlock_t* p, int num ) :
      warlock_spell_t( "unstable_affliction_" + std::to_string( num + 1 ), p, p -> find_spell( ua_spells[num] ) ),
      self( num )
    {
      background = true;
      dual = true;
      tick_may_crit = hasted_ticks = true;
      affected_by_contagion = false;
    }

    timespan_t composite_dot_duration( const action_state_t* s ) const override
    {
      return s -> action -> tick_time( s ) * 4.0;
    }

    double composite_crit_chance() const override
    {
      double cc = warlock_spell_t::composite_crit_chance();

      cc += p() -> artifact.inherently_unstable.percent() * ( p() -> buffs.deadwind_harvester -> check() ? 2.0 : 1.0 );

      return cc;
    }

    double composite_crit_damage_bonus_multiplier() const override
    {
      double cd = warlock_spell_t::composite_crit_damage_bonus_multiplier();

      cd *= 1.0 + p() -> artifact.perdition.percent() * ( p() -> buffs.deadwind_harvester -> check() ? 2.0 : 1.0 );

      return cd;
    }

    virtual double composite_target_multiplier( player_t* target ) const override
    {
      double m = warlock_spell_t::composite_target_multiplier( target );

      warlock_td_t* td = this -> td( target );

      if ( maybe_ptr( p() -> dbc.ptr ) && p() -> talents.malefic_grasp -> ok() && td -> dots_drain_soul -> is_ticking() )
      m *= 1.0 + p() -> find_spell( 235155 ) -> effectN( 1 ).percent();

      return m;
    }

    void init() override
    {
      warlock_spell_t::init();

      if ( p() -> affliction_trinket )
      {
        const spell_data_t* data = p() -> affliction_trinket -> driver();
        double period_value = data -> effectN( 1 ).average( p() -> affliction_trinket -> item ) / 100.0;

        base_tick_time *= 1.0 + period_value;
      }

      snapshot_flags |= STATE_CRIT | STATE_TGT_CRIT | STATE_HASTE;
    }

    void tick( dot_t* d ) override
    {
      if ( p() -> sets.has_set_bonus( WARLOCK_AFFLICTION, T18, B4 ) )
      {
        p() -> buffs.instability -> trigger();
      }

      warlock_spell_t::tick( d );
    }

    void last_tick( dot_t* d ) override
    {
      bool triggered = trigger_fatal_echos( d -> state );

      if ( p() -> legendary.stretens_insanity )
      {
        bool should_decrement_stretens = !triggered;
        // check if there's another UA if we failed to trigger
        for ( int i = 0; i < MAX_UAS && should_decrement_stretens; i++ )
        {
          if ( i == self )
            continue;
          dot_t* other = td( target ) -> dots_unstable_affliction[i];
          if ( other -> is_ticking() )
          {
            should_decrement_stretens = false;
            break;
          }
        }
        if ( should_decrement_stretens )
          p() -> buffs.stretens_insanity -> decrement( 1 );
      }

      warlock_spell_t::last_tick( d );
    }

    bool trigger_fatal_echos( const action_state_t* source_state )
    {
      if ( !p()->artifact.fatal_echoes.rank() )
      {
        return false;
      }

      if ( rng().roll( p()->artifact.fatal_echoes.data().effectN( 1 ).percent() *
        ( p() -> buffs.deadwind_harvester -> check() ? 2.0 : 1.0 ) ) )
      {
        p()->procs.fatal_echos->occur();

        this -> target = source_state -> target;
        this -> schedule_execute();
        return true;
      }
      return false;
    }

    virtual double action_multiplier() const override
    {
      double m = warlock_spell_t::action_multiplier();

      // Does this snapshot on the base damage or apply to the DoT dynamically?
      if ( p() -> mastery_spells.potent_afflictions -> ok() )
        m *= 1.0 + p() -> cache.mastery_value();

      if ( p() -> talents.contagion -> ok() )
        m *= 1.0 + p() -> talents.contagion -> effectN( 1 ).percent();

      return m;
    }
  };

  struct compounding_horror_t : public warlock_spell_t
  {
    compounding_horror_t( warlock_t* p ) :
      warlock_spell_t( "compounding_horror", p, p -> find_spell( 231489 ) )
    {
      background = true;
      //proc = true; Compounding Horror can proc trinkets and has no resource cost.
      callbacks = true;
    }

    virtual double action_multiplier() const override
    {
      double m = warlock_spell_t::action_multiplier();

      m *= p() -> buffs.compounding_horror -> stack();

      return m;
    }
  };

  real_ua_t* ua_dots[MAX_UAS];
  compounding_horror_t* compounding_horror;


  unstable_affliction_t( warlock_t* p ):
    warlock_spell_t( "unstable_affliction", p, p -> spec.unstable_affliction ),
    compounding_horror( new compounding_horror_t( p ) )
  {
    for ( int i = 0; i < MAX_UAS; i++ )
    {
      ua_dots[i] = new real_ua_t( p, i );
      add_child( ua_dots[i] );
    }
    const spell_data_t* ptr_spell = p -> find_spell( 233490 );
    spell_power_mod.direct = ptr_spell -> effectN( 1 ).sp_coeff();
    dot_duration = timespan_t::zero(); // DoT managed by ignite action.
    affected_by_contagion = false;

    if ( p -> sets.has_set_bonus( WARLOCK_AFFLICTION, T19, B2 ) )
      base_multiplier *= 1.0 + p -> sets.set( WARLOCK_AFFLICTION, T19, B2 ) -> effectN( 1 ).percent();
  }

  double cost() const override
  {
    double c = warlock_spell_t::cost();

    if ( p() -> buffs.shard_instability -> check() )
    {
      return 0;
    }

    return c;
  }

  void init() override
  {
    warlock_spell_t::init();

    snapshot_flags &= ~( STATE_CRIT | STATE_TGT_CRIT );
  }

  virtual void impact( action_state_t* s ) override
  {
    if ( result_is_hit( s -> result ) )
    {
      real_ua_t* real_ua = nullptr;
      timespan_t min_duration = timespan_t::from_seconds( 100 );
      for ( int i = 0; i < MAX_UAS; i++ )
      {
        dot_t* curr_ua = td( s -> target ) -> dots_unstable_affliction[i];
        if ( ! ( curr_ua -> is_ticking() ) )
        {
          real_ua = ua_dots[i];
          break;
        }

        timespan_t rem = curr_ua -> remains();
        if ( rem < min_duration )
        {
          real_ua = ua_dots[i];
          min_duration = rem;
        }
      }
      real_ua -> target = s -> target;
      real_ua -> schedule_execute();
    }
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();
    bool flag = false;
    for ( int i = 0; i < MAX_UAS; i++ )
    {
      if ( td( target ) -> dots_unstable_affliction[i] -> is_ticking() )
      {
        flag = true;
        break;
      }
    }

    p() -> buffs.shard_instability -> expire();
    p() -> procs.t18_2pc_affliction -> occur();


    if ( p()->buffs.compounding_horror->check() )
    {
      compounding_horror -> target = execute_state -> target;
      compounding_horror -> execute();
      p() -> buffs.compounding_horror -> expire();
    }

    if ( !flag && rng().roll( p() -> legendary.power_cord_of_lethtendris_chance ) )
    {
      p() -> resource_gain( RESOURCE_SOUL_SHARD, 1.0, p() -> gains.power_cord_of_lethtendris );
    }
    else if ( !flag )
    {
      // Only increment if the dot wasn't already there.
      if ( p() -> legendary.stretens_insanity )
        p() -> buffs.stretens_insanity -> increment( 1 );
    }
  }
};

struct corruption_t: public warlock_spell_t
{
  double chance;

  corruption_t( warlock_t* p ):
    warlock_spell_t( "Corruption", p, p -> find_spell( 172 ) ) //Use original corruption until DBC acts more friendly.
  {
    may_crit = false;
    dot_duration = data().effectN( 1 ).trigger() -> duration();
    spell_power_mod.tick = data().effectN( 1 ).trigger() -> effectN( 1 ).sp_coeff();
    base_tick_time = data().effectN( 1 ).trigger() -> effectN( 1 ).period();

    if ( p -> talents.absolute_corruption -> ok() )
    {
      dot_duration = sim -> expected_iteration_time > timespan_t::zero() ?
        2 * sim -> expected_iteration_time :
        2 * sim -> max_time * ( 1.0 + sim -> vary_combat_length ); // "infinite" duration
      base_multiplier *= 1.0 + p -> talents.absolute_corruption -> effectN( 2 ).percent();
    }

    chance = p -> find_spell( 199282 ) -> proc_chance();
  }

  void init() override
  {
    warlock_spell_t::init();

    if ( p() -> affliction_trinket )
    {
      const spell_data_t* data = p() -> affliction_trinket -> driver();
      double period_value = data -> effectN( 1 ).average( p() -> affliction_trinket -> item ) / 100.0;
      double duration_value = data -> effectN( 2 ).average( p() -> affliction_trinket -> item ) / 100.0;

      base_tick_time *= 1.0 + period_value;
      dot_duration *= 1.0 + duration_value;
    }
  }

  virtual double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    if ( p() -> mastery_spells.potent_afflictions -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    m *= 1.0 + p() -> artifact.hideous_corruption.percent() * ( p() -> buffs.deadwind_harvester -> check() ? 2.0 : 1.0 );

    return m;
  }

  double composite_crit_damage_bonus_multiplier() const override
  {
    double cd = warlock_spell_t::composite_crit_damage_bonus_multiplier();

    cd *= 1.0 + p() -> artifact.perdition.percent() * ( p() -> buffs.deadwind_harvester -> check() ? 2.0 : 1.0 );

    return cd;
  }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double m = warlock_spell_t::composite_target_multiplier( target );

    warlock_td_t* td = this -> td( target );

    if ( maybe_ptr( p() -> dbc.ptr ) && p() -> talents.malefic_grasp -> ok() && td -> dots_drain_soul -> is_ticking() )
      m *= 1.0 + p() -> find_spell( 235155 ) -> effectN( 1 ).percent();

    return m;
  }

  virtual void tick( dot_t* d ) override
  {

    if ( p() -> artifact.harvester_of_souls.rank() && rng().roll( p() -> artifact.harvester_of_souls.data().proc_chance() * ( p() -> buffs.deadwind_harvester -> check() ? 2.0 : 1.0 ) ) )
    {
      p() -> active.harvester_of_souls -> target = d -> target;
      p() -> active.harvester_of_souls -> execute();
    }

    if ( p() -> artifact.compounding_horror.rank() )
    {
      if ( rng().roll( p() -> buffs.deadwind_harvester -> check() ? chance * 2.0 : chance ) )
      {
        p() -> buffs.compounding_horror -> trigger();
      }
    }

    warlock_spell_t::tick( d );
  }
};

struct drain_life_t: public warlock_spell_t
{
  drain_life_t( warlock_t* p ):
    warlock_spell_t( p, "Drain Life" )
  {
    channeled = true;
    hasted_ticks = false;
    may_crit = false;
  }

  virtual bool ready() override
  {
    if ( p() -> talents.drain_soul -> ok() )
      return false;

    if ( maybe_ptr( p() -> dbc.ptr ) && p() -> specialization() == WARLOCK_AFFLICTION )
      return false;

    return warlock_spell_t::ready();
  }

  virtual double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    m *= 1.0 + p() -> artifact.drained_to_a_husk.percent() * ( p() -> buffs.deadwind_harvester -> check() ? 2.0 : 1.0 );

    if ( p() -> specialization() == WARLOCK_AFFLICTION )
      m *= 1.0 + p() -> find_spell( 205183 ) -> effectN( 1 ).percent();

    return m;
  }

  virtual void tick( dot_t* d ) override
  {

    if ( p() -> sets.has_set_bonus( WARLOCK_AFFLICTION, T18, B2 ) )
    {
      p() -> buffs.shard_instability -> trigger();
    }
    warlock_spell_t::tick( d );
  }
};

struct life_tap_t: public warlock_spell_t
{
  life_tap_t( warlock_t* p ):
    warlock_spell_t( p, "Life Tap" )
  {
    harmful = false;
    ignore_false_positive = true;
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    double health = player -> resources.max[RESOURCE_HEALTH];
    double mana = player -> resources.max[RESOURCE_MANA];

    player -> resource_loss( RESOURCE_HEALTH, health * data().effectN( 2 ).percent() );
    // FIXME run through resource usage
    player -> resource_gain( RESOURCE_MANA, mana * data().effectN( 1 ).percent(), p() -> gains.life_tap );

    if ( maybe_ptr( p() -> dbc.ptr ) && p() -> talents.empowered_life_tap -> ok() )
      p() -> buffs.empowered_life_tap -> trigger();
  }
};


// Demonology Spells

struct shadow_bolt_t: public warlock_spell_t
{
  shadow_bolt_t( warlock_t* p ):
    warlock_spell_t( p, "Shadow Bolt" )
  {
    energize_type = ENERGIZE_ON_CAST;
    energize_resource = RESOURCE_SOUL_SHARD;
    energize_amount = 1;

    if ( p->sets.set( WARLOCK_DEMONOLOGY, T17, B4 ) )
    {
      if ( rng().roll( p->sets.set( WARLOCK_DEMONOLOGY, T17, B4 )->effectN( 1 ).percent() ) )
      {
        energize_amount++;
      }
    }

    base_crit += p -> artifact.maw_of_shadows.percent();
  }

  virtual bool ready() override
  {
    if ( p() -> talents.demonbolt -> ok() )
      return false;

    return warlock_spell_t::ready();
  }

  virtual timespan_t execute_time() const override
  {
    if ( p() -> buffs.shadowy_inspiration -> check() )
    {
      return timespan_t::zero();
    }
    return warlock_spell_t::execute_time();
  }

  virtual double action_multiplier()const override
  {
      double m = warlock_spell_t::action_multiplier();
      if( p() -> buffs.stolen_power -> up() )
      {
          p() -> procs.stolen_power_used -> occur();
          m *= 1.0 + p() -> buffs.stolen_power -> data().effectN( 1 ).percent();
          p() -> buffs.stolen_power->reset();
      }
      return m;
  }

  void execute() override
  {
    warlock_spell_t::execute();

    if ( p() -> talents.demonic_calling -> ok() )
      p() -> buffs.demonic_calling -> trigger();

    if ( p() -> buffs.shadowy_inspiration -> check() )
      p() -> buffs.shadowy_inspiration -> expire();

    if ( p() -> artifact.thalkiels_discord.rank() )
    {
      if ( rng().roll( p() -> artifact.thalkiels_discord.data().proc_chance() ) )
      {
        make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
          .target( execute_state -> target )
          .x( execute_state -> target -> x_position )
          .y( execute_state -> target -> y_position )
          .pulse_time( timespan_t::from_millis( 1500 ) )
          .duration( p() -> find_spell( 211729 ) -> duration() )
          .start_time( sim -> current_time() )
          .action( p() -> active.thalkiels_discord ) );

        p() -> procs.thalkiels_discord -> occur();
      }
    }

    if( p() -> sets.set( WARLOCK_DEMONOLOGY, T18, B2 ) )
    {
        p() -> buffs.tier18_2pc_demonology -> trigger( 1 );
    }
  }
};


struct doom_t: public warlock_spell_t
{
  double kazzaks_final_curse_multiplier;

  doom_t( warlock_t* p ):
    warlock_spell_t( "doom", p, p -> spec.doom ),
    kazzaks_final_curse_multiplier( 0.0 )
  {
    base_tick_time = p -> find_spell( 603 ) -> effectN( 1 ).period();
    dot_duration = p -> find_spell( 603 ) -> duration();
    spell_power_mod.tick = p -> spec.doom -> effectN( 1 ).sp_coeff();

    base_multiplier *= 1.0 + p -> artifact.the_doom_of_azeroth.percent();

    may_crit = true;
    hasted_ticks = true;

    energize_type = ENERGIZE_PER_TICK;
    energize_resource = RESOURCE_SOUL_SHARD;
    energize_amount = 1;

    if ( maybe_ptr( p -> dbc.ptr ) && p -> talents.impending_doom -> ok() )
     base_tick_time += p -> find_spell( 196270 ) -> effectN( 1 ).time_value();
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    timespan_t duration = warlock_spell_t::composite_dot_duration( s );
    return duration * p() -> cache.spell_haste();
  }

  virtual double action_multiplier()const override
  {
    double m = warlock_spell_t::action_multiplier();
    double pet_counter = 0.0;

    if ( p() -> artifact.doom_doubled.rank() )
    {
      if ( rng().roll( 0.25 ) )
      {
        m *= 2.0;
      }
    }

    if ( kazzaks_final_curse_multiplier > 0.0 )
    {
      for ( auto& pet : p() -> pet_list )
      {
        pets::warlock_pet_t *lock_pet = static_cast< pets::warlock_pet_t* > ( pet );

        if ( lock_pet != NULL )
        {
          if ( !lock_pet -> is_sleeping() )
          {
            pet_counter += kazzaks_final_curse_multiplier;
          }
        }
      }
      m *= 1.0 + pet_counter;
    }
    return m;
  }

  virtual void tick( dot_t* d ) override
  {
    warlock_spell_t::tick( d );

    if(  d -> state -> result == RESULT_HIT || result_is_hit( d -> state -> result) )
    {
      if( p() -> talents.impending_doom -> ok() )
      {
        trigger_wild_imp( p() );
        p() -> procs.impending_doom -> occur();
      }

      if ( p()->sets.has_set_bonus( WARLOCK_DEMONOLOGY, T19, B2 ) && rng().roll( p() -> sets.set( WARLOCK_DEMONOLOGY, T19, B2 ) -> effectN( 1 ).percent() ) )
        p() -> resource_gain( RESOURCE_SOUL_SHARD, 1, p() -> gains.t19_2pc_demonology );
    }
  }
};

struct demonic_empowerment_t: public warlock_spell_t
{
  double power_trip_rng;

  demonic_empowerment_t (warlock_t* p) :
    warlock_spell_t( "demonic empowerment", p, p -> spec.demonic_empowerment )
  {
    may_crit = false;
    harmful = false;
    dot_duration = timespan_t::zero();

    power_trip_rng = p -> talents.power_trip -> effectN( 1 ).percent();
  }

  void execute() override
    {
    warlock_spell_t::execute();
    for( auto& pet : p() -> pet_list )
    {
      pets::warlock_pet_t *lock_pet = static_cast<pets::warlock_pet_t*> ( pet );

      if( lock_pet != nullptr )
      {
        if( !lock_pet -> is_sleeping() )
        {
          lock_pet -> buffs.demonic_empowerment -> trigger();
        }
      }
    }

    if ( p() -> talents.power_trip -> ok() && rng().roll( power_trip_rng ) )
      p() -> resource_gain( RESOURCE_SOUL_SHARD, 1, p() -> gains.power_trip );

    if ( p() -> talents.shadowy_inspiration -> ok() )
      p() -> buffs.shadowy_inspiration -> trigger();
  }
};



struct hand_of_guldan_t: public warlock_spell_t
{
  struct trigger_imp_event_t : public player_event_t
  {
    bool initiator;
    int count;
    trigger_imp_event_t( warlock_t* p, int c, bool init = false ) :
      player_event_t( *p, timespan_t::from_millis(1) ), initiator( init ), count( c )//Use original corruption until DBC acts more friendly.
    {
      //add_event( rng().range( timespan_t::from_millis( 500 ),
      //  timespan_t::from_millis( 1500 ) ) );
    }

    virtual const char* name() const override
    {
      return  "trigger_imp";
    }

    virtual void execute() override
    {
      warlock_t* p = static_cast< warlock_t* >( player() );

      if ( p -> demonology_trinket )
      {
        const spell_data_t* data = p -> find_spell( p -> demonology_trinket -> spell_id );
        double demonology_trinket_chance = data -> effectN( 1 ).average( p -> demonology_trinket -> item );
        demonology_trinket_chance /= 100.0;
        if ( rng().roll( demonology_trinket_chance ) )
        {
          count += 3;
          p -> procs.fragment_wild_imp -> occur();
          p -> procs.fragment_wild_imp -> occur();
          p -> procs.fragment_wild_imp -> occur();
        }
      }
      for ( pets::wild_imp_pet_t* wild_imp : p -> warlock_pet_list.wild_imps )
      {
        if ( wild_imp -> is_sleeping() )
        {
          count--;

          trigger_wild_imp( p );
        }
        if ( count == 0 )
          return;
      }
    }
  };

  trigger_imp_event_t* imp_event;
  int shards_used;
  double demonology_trinket_chance;
  doom_t* doom;

  hand_of_guldan_t( warlock_t* p ):
    warlock_spell_t( p, "Hand of Gul'dan" ),
    demonology_trinket_chance( 0.0 ),
    doom( new doom_t( p ) )
  {
    aoe = -1;
    shards_used = 0;

    parse_effect_data( p -> find_spell( 86040 ) -> effectN( 1 ) );

    doom -> background = true;
    doom -> dual = true;
    doom -> base_costs[RESOURCE_MANA] = 0;
    base_multiplier *= 1.0 + p -> artifact.dirty_hands.percent();
  }

  virtual timespan_t travel_time() const override
  {
    return timespan_t::from_millis( 700 );
  }

  virtual bool ready() override
  {
    bool r = warlock_spell_t::ready();

    if ( p() -> resources.current[RESOURCE_SOUL_SHARD] == 0.0 )
      r = false;

    return r;
  }

  virtual double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    m *= resource_consumed;

    return m;
  }

  void consume_resource() override
  {
    warlock_spell_t::consume_resource();

    shards_used = resource_consumed;

    if ( resource_consumed == 1.0 )
      p() -> procs.one_shard_hog -> occur();
    if ( resource_consumed == 2.0 )
      p() -> procs.two_shard_hog -> occur();
    if ( resource_consumed == 3.0 )
      p() -> procs.three_shard_hog -> occur();
    if ( resource_consumed == 4.0 )
      p() -> procs.four_shard_hog -> occur();
  }

  virtual void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if( p() -> talents.hand_of_doom -> ok() )
      {
        doom -> target = s -> target;
        doom -> execute();
      }
      if ( p() -> sets.set( WARLOCK_DEMONOLOGY, T17, B2 ) )
      {
        if ( rng().roll( p() -> sets.set( WARLOCK_DEMONOLOGY, T17, B2 ) -> proc_chance() ) )
        {
          shards_used *= 1.5;
        }
      }
      if ( s -> chain_target == 0 )
        imp_event =  make_event<trigger_imp_event_t>( *sim, p(), floor( shards_used ), true);
    }
  }
};

struct havoc_t: public warlock_spell_t
{
  timespan_t havoc_duration;

  havoc_t( warlock_t* p ):
    warlock_spell_t( p, "Havoc" )
  {
    may_crit = false;

    if ( p -> talents.wreak_havoc -> ok() )
      cooldown -> duration = timespan_t::from_seconds( 0 );
    havoc_duration = p -> find_spell( 80240 ) -> duration();
    if ( p -> talents.wreak_havoc -> ok() )
      havoc_duration += p -> find_spell( 196410 ) -> effectN( 1 ).time_value();
  }

  void execute() override
  {
    warlock_spell_t::execute();
    p() -> havoc_target = execute_state -> target;
  }

  void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    td( s -> target ) -> debuffs_havoc -> trigger( 1, buff_t::DEFAULT_VALUE(),-1, havoc_duration );
  }
};

struct immolate_t: public warlock_spell_t
{
  double roaring_blaze;

  immolate_t( warlock_t* p ):
    warlock_spell_t( "immolate", p, p -> find_spell( 348 ) )
  {
    const spell_data_t* dmg_spell = player -> find_spell( 157736 );

    base_tick_time = dmg_spell -> effectN( 1 ).period();
    dot_duration = dmg_spell -> duration();
    spell_power_mod.tick = dmg_spell -> effectN( 1 ).sp_coeff();
    spell_power_mod.direct = data().effectN( 1 ).sp_coeff();
    hasted_ticks = true;
    tick_may_crit = true;

    base_crit += p -> artifact.burning_hunger.percent();
    base_multiplier *= 1.0 + p -> artifact.residual_flames.percent();

    roaring_blaze = 1.0 + p -> find_spell( 205690 ) -> effectN( 1 ).percent();
  }

  void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      td( s -> target ) -> debuffs_roaring_blaze -> expire();
    }
  }

  virtual double composite_ta_multiplier( const action_state_t* state ) const override
  {
    double m = warlock_spell_t::composite_ta_multiplier( state );

    if ( td( state -> target ) -> dots_immolate -> is_ticking() && p() -> talents.roaring_blaze -> ok() )
      m *= std::pow( roaring_blaze, td( state -> target ) -> debuffs_roaring_blaze -> stack() );

    return m;
  }

  void last_tick( dot_t* d ) override
  {
    warlock_spell_t::last_tick( d );

      td( d -> target ) -> debuffs_roaring_blaze -> expire();
  }

  void execute() override
  {
    warlock_spell_t::execute();

    if ( p() -> legendary.feretory_of_souls && rng().roll( p() -> find_spell( 205702 ) -> proc_chance() ) )
    {
      p() -> resource_gain( RESOURCE_SOUL_SHARD, 1.0, p() -> gains.feretory_of_souls );
    }
  }

  virtual void tick( dot_t* d ) override
  {
    warlock_spell_t::tick( d );

    if ( p() -> sets.has_set_bonus( WARLOCK_DESTRUCTION, T17, B2 ) )
    {
      if ( d -> state -> result == RESULT_CRIT && rng().roll( 0.38 ) )
        p()->resource_gain( RESOURCE_SOUL_SHARD, 1, p() -> gains.immolate );
      else if ( d -> state -> result == RESULT_HIT && rng().roll( 0.19 ) )
        p() -> resource_gain( RESOURCE_SOUL_SHARD, 1, p() -> gains.immolate );
    }
    else
    {
      if ( d -> state -> result == RESULT_CRIT && rng().roll( 0.3 ) )
        p() -> resource_gain( RESOURCE_SOUL_SHARD, 1, p() -> gains.immolate );
      else if ( d -> state -> result == RESULT_HIT && rng().roll( 0.15 ) )
        p() -> resource_gain( RESOURCE_SOUL_SHARD, 1, p() -> gains.immolate );
    }
  }
};

struct conflagrate_t: public warlock_spell_t
{
  timespan_t total_duration;
  timespan_t base_duration;
  conflagrate_t( warlock_t* p ):
    warlock_spell_t( "Conflagrate", p, p -> find_spell( 17962 ) )
  {
    energize_type = ENERGIZE_ON_CAST;
    base_duration = p -> find_spell( 117828 ) -> duration();

    cooldown -> charges += p -> spec.conflagrate_2 -> effectN( 1 ).base_value();

    cooldown -> charges += p -> sets.set( WARLOCK_DESTRUCTION, T19, B4 ) -> effectN( 1 ).base_value();
    cooldown -> duration += p -> sets.set( WARLOCK_DESTRUCTION, T19, B4 ) -> effectN( 2 ).time_value();
  }

  bool ready() override
  {
    if ( maybe_ptr( p() -> dbc.ptr ) && p() -> talents.shadowburn -> ok() )
      return false;

    return warlock_spell_t::ready();
  }

  void init() override
  {
    warlock_spell_t::init();

    cooldown -> hasted = true;
  }

  // Force spell to always crit
  double composite_crit_chance() const override
  {
    double cc = warlock_spell_t::composite_crit_chance();

    if ( p() -> buffs.conflagration_of_chaos -> check() )
      cc = 1.0;

    return cc;
  }

  double calculate_direct_amount( action_state_t* state ) const override
  {
    warlock_spell_t::calculate_direct_amount( state );

    // Can't use player-based crit chance from the state object as it's hardcoded to 1.0. Use cached
    // player spell crit instead. The state target crit chance of the state object is correct.
    // Targeted Crit debuffs function as a separate multiplier.
    if ( p() -> buffs.conflagration_of_chaos -> check() )
      state -> result_total *= 1.0 + player -> cache.spell_crit_chance() + state -> target_crit_chance;

    return state -> result_total;
  }

  void execute() override
  {
    warlock_spell_t::execute();

    if ( p() -> talents.backdraft -> ok() )
    {
      total_duration = base_duration * p() -> cache.spell_haste();
      p() -> buffs.backdraft -> trigger( 2, buff_t::DEFAULT_VALUE(), -1.0, total_duration );
    }

    if ( p() -> buffs.conflagration_of_chaos -> up() )
      p() -> buffs.conflagration_of_chaos -> expire();

    p() -> buffs.conflagration_of_chaos -> trigger();

    if ( p() -> legendary.feretory_of_souls && rng().roll( p() -> find_spell( 205702 ) -> proc_chance() ) )
    {
      p() -> resource_gain( RESOURCE_SOUL_SHARD, 1.0, p() -> gains.feretory_of_souls );
    }
  }

  void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> talents.roaring_blaze -> ok() && td( s -> target ) -> dots_immolate -> is_ticking() )
      {
        td( s -> target ) -> debuffs_roaring_blaze -> trigger( 1 );
      }
    }
  }
};

struct incinerate_t: public warlock_spell_t
{
  double backdraft_gcd;
  double backdraft_cast_time;
  double dimension_ripper;
  incinerate_t( warlock_t* p ):
    warlock_spell_t( p, "Incinerate" )
  {
    if ( p -> talents.fire_and_brimstone -> ok() )
      aoe = -1;

    base_execute_time *= 1.0 + p -> artifact.fire_and_the_flames.percent();
    base_multiplier *= 1.0 + p -> artifact.master_of_distaster.percent();

    dimension_ripper = p -> find_spell( 219415 ) -> proc_chance();

    backdraft_cast_time = 1.0 + p -> buffs.backdraft -> data().effectN( 1 ).percent();
    backdraft_gcd = 1.0 + p -> buffs.backdraft -> data().effectN( 2 ).percent();
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t h = spell_t::execute_time();

    if ( p() -> buffs.backdraft -> up() )
      h *= backdraft_cast_time;

    return h;
  }

  timespan_t gcd() const override
  {
    timespan_t t = action_t::gcd();

    if ( t == timespan_t::zero() )
      return t;

    if ( p() -> buffs.backdraft -> check() )
      t *= backdraft_gcd;
    if ( t < min_gcd )
      t = min_gcd;

    return t;
  }

  void execute() override
  {
    warlock_spell_t::execute();

    if ( p() -> artifact.dimension_ripper.rank() && rng().roll( dimension_ripper ) && p() -> cooldowns.dimensional_rift -> current_charge < p() -> cooldowns.dimensional_rift -> charges )
    {
      p() -> cooldowns.dimensional_rift -> adjust( -p() -> cooldowns.dimensional_rift -> duration ); //decrease remaining time by the duration of one charge, i.e., add one charge
      p() -> procs.dimension_ripper -> occur();
    }

    if ( p() -> legendary.feretory_of_souls && rng().roll( p() -> find_spell( 205702 ) -> proc_chance() ) )
    {
      p() -> resource_gain( RESOURCE_SOUL_SHARD, 1.0, p() -> gains.feretory_of_souls );
    }

    p() -> buffs.backdraft -> decrement();
  }

  virtual double composite_crit_chance() const override
  {
    double cc = warlock_spell_t::composite_crit_chance();

    return cc;
  }

  void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> destruction_trinket )
      {
        td( s -> target ) -> debuffs_flamelicked -> trigger( 1 );
      }
    }
  }
};

struct chaos_bolt_t: public warlock_spell_t
{
  double backdraft_gcd;
  double backdraft_cast_time;
  double refund;
  chaos_bolt_t( warlock_t* p ):
    warlock_spell_t( p, "Chaos Bolt" ), refund(0)
  {
    if ( p -> talents.reverse_entropy -> ok() )
      base_execute_time += p -> talents.reverse_entropy -> effectN( 2 ).time_value();

    crit_bonus_multiplier *= 1.0 + p -> artifact.chaotic_instability.percent();

    base_execute_time += p -> sets.set( WARLOCK_DESTRUCTION, T18, B2 ) -> effectN( 1 ).time_value();
    base_multiplier *= 1.0 + ( p -> sets.set( WARLOCK_DESTRUCTION, T18, B2 ) -> effectN( 2 ).percent() );
    base_multiplier *= 1.0 + ( p -> sets.set( WARLOCK_DESTRUCTION, T17, B4 ) -> effectN( 1 ).percent() );

    backdraft_cast_time = 1.0 + p -> buffs.backdraft -> data().effectN( 1 ).percent();
    backdraft_gcd = 1.0 + p -> buffs.backdraft -> data().effectN( 2 ).percent();
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t h = spell_t::execute_time();

    if ( p() -> buffs.backdraft -> up() )
      h *= backdraft_cast_time;

    if ( p() -> buffs.embrace_chaos -> up() )
      h *= 1.0 + p() -> buffs.embrace_chaos -> data().effectN( 1 ).percent();

    return h;
  }

  timespan_t gcd() const override
  {
    timespan_t t = action_t::gcd();

    if ( t == timespan_t::zero() )
      return t;

    if ( p() -> buffs.backdraft -> check() )
      t *= backdraft_gcd;
    if ( t < min_gcd )
      t = min_gcd;

    return t;
  }

  void impact( action_state_t* s ) override
  {
    if ( result_is_hit( s -> result ) )
      td( s -> target ) -> debuffs_eradication -> trigger();

    warlock_spell_t::impact( s );
  }

  void execute() override
  {
    warlock_spell_t::execute();

    if ( p() -> talents.reverse_entropy -> ok() )
    {
      refund = p() -> resources.max[RESOURCE_MANA] * p() -> talents.reverse_entropy -> effectN( 1 ).percent();
      p() -> resource_gain( RESOURCE_MANA, refund, p() -> gains.reverse_entropy );
    }

    if ( p() -> artifact.soulsnatcher.rank() && rng().roll( p() -> artifact.soulsnatcher.percent() ) )
      p() -> resource_gain( RESOURCE_SOUL_SHARD, 1, p() -> gains.soulsnatcher );

    p() -> buffs.embrace_chaos -> trigger();
    p() -> buffs.backdraft -> decrement();
  }

  // Force spell to always crit
  double composite_crit_chance() const override
  {
    return 1.0;
  }

  double calculate_direct_amount( action_state_t* state ) const override
  {
    warlock_spell_t::calculate_direct_amount( state );

    // Can't use player-based crit chance from the state object as it's hardcoded to 1.0. Use cached
    // player spell crit instead. The state target crit chance of the state object is correct.
    // Targeted Crit debuffs function as a separate multiplier.
    state -> result_total *= 1.0 + player -> cache.spell_crit_chance() + state -> target_crit_chance;

    return state -> result_total;
  }

  double cost() const override
  {
    double c = warlock_spell_t::cost();

    double t18_4pc_rng = p() -> sets.set( WARLOCK_DESTRUCTION, T18, B4 ) -> effectN( 1 ).percent();

    if ( rng().roll( t18_4pc_rng ) )
    {
      p() -> procs.t18_4pc_destruction -> occur();
      return 1;
    }

    return c;
  }
};

// ARTIFACT SPELLS

struct thalkiels_discord_t : public warlock_spell_t
{
  thalkiels_discord_t( warlock_t* p ) :
    warlock_spell_t( "thalkiels_discord", p, p -> find_spell( 211727 ) )
  {
    aoe = -1;
    background = dual = true;
    callbacks = false;
  }

  void init() override
  {
    warlock_spell_t::init();

    // Explicitly snapshot haste, as the spell actually has no duration in spell data
    snapshot_flags |= STATE_HASTE;
  }
};

struct dimensional_rift_t : public warlock_spell_t
{
  timespan_t shadowy_tear_duration;
  timespan_t chaos_tear_duration;
  timespan_t chaos_portal_duration;

  dimensional_rift_t( warlock_t* p ):
    warlock_spell_t( "dimensional_rift", p, p -> artifact.dimensional_rift )
  {
    shadowy_tear_duration = timespan_t::from_millis( 14001 );
    chaos_tear_duration = timespan_t::from_millis( 5001 );
    chaos_portal_duration = timespan_t::from_millis( 5501 );
  }

  void execute() override
  {
    warlock_spell_t::execute();

    double rift = rng().range( 0.0, 1.0 );

    if ( rift <= ( 1.0 / 3.0 ) )
    {
      for ( size_t i = 0; i < p() ->warlock_pet_list.shadowy_tear.size(); i++ )
      {
        if ( p() -> warlock_pet_list.shadowy_tear[i] -> is_sleeping() )
        {
          p() -> warlock_pet_list.shadowy_tear[i] -> summon( shadowy_tear_duration );
          p() -> procs.shadowy_tear -> occur();
          break;
        }
      }
    }
    else if ( rift >= ( 2.0 / 3.0 ) )
    {
      for ( size_t i = 0; i < p() -> warlock_pet_list.chaos_tear.size(); i++ )
      {
        if ( p() -> warlock_pet_list.chaos_tear[i] -> is_sleeping() )
        {
          p() -> warlock_pet_list.chaos_tear[i] -> summon( chaos_tear_duration );
          p() -> procs.chaos_tear -> occur();
          break;
        }
      }
    }
    else
    {
      for ( size_t i = 0; i < p() -> warlock_pet_list.chaos_portal.size(); i++ )
      {
        if ( p() -> warlock_pet_list.chaos_portal[i] -> is_sleeping() )
        {
          p() -> warlock_pet_list.chaos_portal[i] -> summon( chaos_tear_duration );
          p() -> procs.chaos_portal -> occur();
          break;
        }
      }
    }
  }
};

struct thalkiels_consumption_t : public warlock_spell_t
{
  thalkiels_consumption_t( warlock_t* p ) :
    warlock_spell_t( "thalkiels_consumption", p, p -> artifact.thalkiels_consumption )
  {

  }

  void execute() override
  {
    warlock_spell_t::execute();

    double ta_mult = p() -> composite_player_target_multiplier( p() -> target, get_school() );
    double p_mult = p() -> composite_player_multiplier( school );
    double damage = 0;

    for ( auto& pet : p() -> pet_list )
    {
      pets::warlock_pet_t *lock_pet = static_cast< pets::warlock_pet_t* > ( pet );
      if ( lock_pet != NULL )
      {
        if ( !lock_pet -> is_sleeping() )
        {
          damage += ( double ) ( lock_pet->resources.max[RESOURCE_HEALTH] ) * 0.06; //spelldata
        }
      }
    }
    damage *= ta_mult;
    damage *= p_mult;

    this -> base_dd_min = damage;
    this -> base_dd_max = damage;
    //do other stuff
  }
};

// AOE SPELLS

struct seed_of_corruption_t: public warlock_spell_t
{
  struct seed_of_corruption_aoe_t: public warlock_spell_t
  {
    seed_of_corruption_aoe_t( warlock_t* p ):
      warlock_spell_t( "seed_of_corruption_aoe", p, p -> find_spell( 27285 ) )
    {
      aoe = -1;
      dual = true;
      background = true;

      p -> spells.seed_of_corruption_aoe = this;
    }

    double action_multiplier() const override
    {
      double m = warlock_spell_t::action_multiplier();

      m *= 1.0 + p() -> artifact.seeds_of_doom.percent() * ( p() -> buffs.deadwind_harvester -> check() ? 2.0 : 1.0 );

      return m;
    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      if ( p() -> active.corruption )
      {
        p() -> active.corruption -> target = s -> target;
        p() -> active.corruption -> schedule_execute();
      }

      if ( result_is_hit( s -> result ) )
      {
        warlock_td_t* tdata = td( s -> target );

        if ( tdata -> dots_seed_of_corruption -> is_ticking() && tdata -> soc_threshold > 0 )
        {
          tdata -> soc_threshold = 0;
          tdata -> dots_seed_of_corruption -> cancel();
        }
      }
    }
  };

  double threshold_mod;
  double sow_the_seeds_targets;
  double sow_the_seeds_cost;
  seed_of_corruption_aoe_t* explosion;

  seed_of_corruption_t( warlock_t* p ):
    warlock_spell_t( "seed_of_corruption", p, p -> find_spell( 27243 ) ),
    explosion( new seed_of_corruption_aoe_t( p ) )
  {
    may_crit = false;
    threshold_mod = 3.0;
    base_tick_time = dot_duration;
    hasted_ticks = false;

    sow_the_seeds_targets = p -> talents.sow_the_seeds -> effectN( 1 ).base_value();
    sow_the_seeds_cost    = 1.0;

    add_child( explosion );
  }

  void init() override
  {
    warlock_spell_t::init();

    snapshot_flags |= STATE_SP;
  }

  void execute() override
  {
    bool sow_the_seeds = false;

    if ( p() -> talents.sow_the_seeds -> ok() && p() -> resources.current[ RESOURCE_SOUL_SHARD ] >= sow_the_seeds_cost )
    {
      sow_the_seeds = true;
      p() -> resource_loss( RESOURCE_SOUL_SHARD, sow_the_seeds_cost, 0, this );
      stats -> consume_resource( RESOURCE_SOUL_SHARD, sow_the_seeds_cost );
      aoe += sow_the_seeds_targets;
    }

    warlock_spell_t::execute();

    if ( sow_the_seeds )
    {
      aoe -= sow_the_seeds_targets;
    }
  }

  void impact( action_state_t* s ) override
  {
    if ( result_is_hit( s -> result ) )
    {
      td( s -> target ) -> soc_threshold = s -> composite_spell_power() * threshold_mod;
    }

    warlock_spell_t::impact( s );
  }

  void last_tick( dot_t* d ) override
  {
    warlock_spell_t::last_tick( d );

    explosion -> target = d -> target;
    explosion -> execute();
  }
};

struct rain_of_fire_t : public warlock_spell_t
{
  struct rain_of_fire_tick_t : public warlock_spell_t
  {
    rain_of_fire_tick_t( warlock_t* p ) :
      warlock_spell_t( "rain_of_fire_tick", p, p -> find_spell( 42223 ) )
    {
      aoe = -1;
      background = dual = direct_tick = true; // Legion TOCHECK
      callbacks = false;
      radius = p -> find_spell( 5740 ) -> effectN( 1 ).radius();
    }
  };

  rain_of_fire_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "rain_of_fire", p, p -> find_spell( 5740 ) )
  {
    parse_options( options_str );
    dot_duration = timespan_t::zero();
    may_miss = may_crit = false;
    base_tick_time = data().duration() / 8.0; // ticks 8 times (missing from spell data
    base_execute_time = timespan_t::zero(); // HOTFIX


    if ( !p -> active.rain_of_fire )
    {
      p -> active.rain_of_fire = new rain_of_fire_tick_t( p );
      p -> active.rain_of_fire -> stats = stats;
    }
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
      .target( execute_state -> target )
      .x( execute_state -> target -> x_position )
      .y( execute_state -> target -> y_position )
      .pulse_time( base_tick_time * player -> cache.spell_haste() )
      .duration( data().duration() * player -> cache.spell_haste() )
      .start_time( sim -> current_time() )
      .action( p() -> active.rain_of_fire ) );

    if ( p() -> legendary.feretory_of_souls && rng().roll( p() -> find_spell( 205702 ) -> proc_chance() ) )
    {
      p() -> resource_gain( RESOURCE_SOUL_SHARD, 1.0, p() -> gains.feretory_of_souls );
    }
  }
};

struct demonwrath_tick_t: public warlock_spell_t
{

  demonwrath_tick_t( warlock_t* p, const spell_data_t& ):
    warlock_spell_t( "demonwrath_tick", p, p -> find_spell( 193439 ) )
  {
    aoe = -1;
    background = true;
  }

  void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    double accumulator_increment = rng().range( 0.0, 0.3 );

    p() -> demonwrath_accumulator += accumulator_increment;

    if ( p() -> demonwrath_accumulator >= 1 )
    {
      p() -> resource_gain( RESOURCE_SOUL_SHARD, 1.0, p() -> gains.demonwrath );
      p() -> demonwrath_accumulator -= 1.0;

      // If going from 0 to 1 shard was a surprise, the player would have to react to it
      if ( p() -> resources.current[RESOURCE_SOUL_SHARD] == 1 )
        p() -> shard_react = p() -> sim -> current_time() + p() -> total_reaction_time();
      else if ( p() -> resources.current[RESOURCE_SOUL_SHARD] >= 1 )
        p() -> shard_react = p() -> sim -> current_time();
      else
        p() -> shard_react = timespan_t::max();
    }
  }
};

struct demonwrath_t: public warlock_spell_t
{
  demonwrath_t( warlock_t* p ):
    warlock_spell_t( "Demonwrath", p, p -> find_spell( 193440 ) )
  {
    tick_zero = false;
    may_miss = false;
    channeled = true;
    may_crit = false;

    spell_power_mod.tick = base_td = 0;

    base_multiplier *= 1.0 + p -> artifact.legionwrath.percent();

    dynamic_tick_action = true;
    tick_action = new demonwrath_tick_t( p, data() );
  }

  virtual bool usable_moving() const override
  {
    return true;
  }
};

// SUMMONING SPELLS

struct summon_pet_t: public warlock_spell_t
{
  timespan_t summoning_duration;
  std::string pet_name;
  pets::warlock_pet_t* pet;

private:
  void _init_summon_pet_t()
  {
    util::tokenize( pet_name );
    harmful = false;

    if ( data().ok() &&
         std::find( p() -> pet_name_list.begin(), p() -> pet_name_list.end(), pet_name ) ==
         p() -> pet_name_list.end() )
    {
      p() -> pet_name_list.push_back( pet_name );
    }
  }

public:
  summon_pet_t( const std::string& n, warlock_t* p, const std::string& sname = "" ):
    warlock_spell_t( p, sname.empty() ? "Summon " + n : sname ),
    summoning_duration( timespan_t::zero() ),
    pet_name( sname.empty() ? n : sname ), pet( nullptr )
  {
    _init_summon_pet_t();
  }

  summon_pet_t( const std::string& n, warlock_t* p, int id ):
    warlock_spell_t( n, p, p -> find_spell( id ) ),
    summoning_duration( timespan_t::zero() ),
    pet_name( n ), pet( nullptr )
  {
    _init_summon_pet_t();
  }

  summon_pet_t( const std::string& n, warlock_t* p, const spell_data_t* sd ):
    warlock_spell_t( n, p, sd ),
    summoning_duration( timespan_t::zero() ),
    pet_name( n ), pet( nullptr )
  {
    _init_summon_pet_t();
  }

  bool init_finished() override
  {
    pet = debug_cast<pets::warlock_pet_t*>( player -> find_pet( pet_name ) );
    return warlock_spell_t::init_finished();
  }

  virtual void execute() override
  {
    pet -> summon( summoning_duration );

    warlock_spell_t::execute();
  }

  bool ready() override
  {
    if ( ! pet )
    {
      return false;
    }

    return warlock_spell_t::ready();
  }
};

struct summon_main_pet_t: public summon_pet_t
{
  cooldown_t* instant_cooldown;

  summon_main_pet_t( const std::string& n, warlock_t* p ):
    summon_pet_t( n, p ), instant_cooldown( p -> get_cooldown( "instant_summon_pet" ) )
  {
    instant_cooldown -> duration = timespan_t::from_seconds( 60 );
    ignore_false_positive = true;
  }

  virtual void schedule_execute( action_state_t* state = nullptr ) override
  {
    warlock_spell_t::schedule_execute( state );

    if ( p() -> warlock_pet_list.active )
    {
      p() -> warlock_pet_list.active -> dismiss();
      p() -> warlock_pet_list.active = nullptr;
    }
  }

  virtual bool ready() override
  {
    if ( p() -> warlock_pet_list.active == pet )
      return false;

    if ( p() -> talents.grimoire_of_supremacy -> ok() ) //if we have the uberpets, we can't summon our standard pets
      return false;
    return summon_pet_t::ready();
  }

  virtual void execute() override
  {
    summon_pet_t::execute();

    p() -> warlock_pet_list.active = p() -> warlock_pet_list.last = pet;

    if ( p() -> buffs.demonic_power -> check() )
      p() -> buffs.demonic_power -> expire();
  }
};

struct summon_doomguard_t: public warlock_spell_t
{
  timespan_t doomguard_duration;

  summon_doomguard_t( warlock_t* p ):
    warlock_spell_t( "summon_doomguard", p, p -> find_spell( 18540 ) )
  {
    harmful = may_crit = false;

    cooldown = p -> cooldowns.doomguard;
    if ( !p -> talents.grimoire_of_supremacy -> ok() )
      cooldown -> duration = data().cooldown();
    else
      cooldown -> duration = timespan_t::zero();

    if ( p -> talents.grimoire_of_supremacy -> ok() )
      doomguard_duration = timespan_t::from_seconds( -1 );
    else
      doomguard_duration = p -> find_spell( 111685 ) -> duration() + timespan_t::from_millis( 1 );
  }

  virtual void schedule_execute( action_state_t* state = nullptr ) override
  {
    warlock_spell_t::schedule_execute( state );

    if ( p() -> talents.grimoire_of_supremacy -> ok() )
    {
      for ( auto infernal : p() -> warlock_pet_list.infernal )
      {
        if ( !infernal -> is_sleeping() )
        {
          infernal -> dismiss();
        }
      }
    }
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    if ( !p() -> talents.grimoire_of_supremacy -> ok() )
      p() -> cooldowns.infernal -> start();

    for ( size_t i = 0; i < p() -> warlock_pet_list.doomguard.size(); i++ )
    {
      if ( p() -> warlock_pet_list.doomguard[i] -> is_sleeping() )
      {
        p() -> warlock_pet_list.doomguard[i] -> summon( doomguard_duration );
      }
    }
    if ( p() -> cooldowns.sindorei_spite_icd -> up() )
    {
      p() -> buffs.sindorei_spite -> up();
      p() -> buffs.sindorei_spite -> trigger();
      p() -> cooldowns.sindorei_spite_icd -> start( timespan_t::from_seconds( 180.0 ) );
    }
  }
};

struct infernal_awakening_t : public warlock_spell_t
{
  infernal_awakening_t( warlock_t* p, spell_data_t* spell ) :
    warlock_spell_t( "infernal_awakening", p, spell )
  {
    aoe = -1;
    background = true;
    dual = true;
    trigger_gcd = timespan_t::zero();
  }
};

struct summon_infernal_t : public warlock_spell_t
{
  infernal_awakening_t* infernal_awakening;
  timespan_t infernal_duration;

  summon_infernal_t( warlock_t* p ) :
    warlock_spell_t( "Summon_Infernal", p, p -> find_spell( 1122 ) ),
    infernal_awakening( nullptr )
  {
    harmful = may_crit = false;

    cooldown = p -> cooldowns.infernal;
    if ( !p -> talents.grimoire_of_supremacy -> ok() )
      cooldown -> duration = data().cooldown();
    else
      cooldown -> duration = timespan_t::zero();

    if ( p -> talents.grimoire_of_supremacy -> ok() )
      infernal_duration = timespan_t::from_seconds( -1 );
    else
    {
      infernal_duration = p -> find_spell( 111685 ) -> duration() + timespan_t::from_millis( 1 );
      infernal_awakening = new infernal_awakening_t( p, data().effectN( 1 ).trigger() );
      infernal_awakening -> stats = stats;
      radius = infernal_awakening -> radius;
    }
  }

  virtual void schedule_execute( action_state_t* state = nullptr ) override
  {
    warlock_spell_t::schedule_execute( state );

    if ( p() -> talents.grimoire_of_supremacy -> ok() )
    {
      for ( auto doomguard : p() -> warlock_pet_list.doomguard )
      {
        if ( !doomguard -> is_sleeping() )
        {
          doomguard -> dismiss();
        }
      }
    }
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    if ( !p() -> talents.grimoire_of_supremacy -> ok() )
      p() -> cooldowns.doomguard -> start();

    if ( infernal_awakening )
      infernal_awakening -> execute();

    for ( size_t i = 0; i < p() -> warlock_pet_list.infernal.size(); i++ )
    {
      if ( p() -> warlock_pet_list.infernal[i] -> is_sleeping() )
      {
        p() -> warlock_pet_list.infernal[i] -> summon( infernal_duration );
      }
    }

    if ( p() -> artifact.lord_of_flames.rank() && ! p() -> talents.grimoire_of_supremacy -> ok() &&
         ! p() -> buffs.lord_of_flames -> up() )
    {
      p() -> trigger_lof_infernal();
      p() -> buffs.lord_of_flames -> trigger();
    }

    if ( p() -> cooldowns.sindorei_spite_icd -> up() )
    {
      p() -> buffs.sindorei_spite -> up();
      p() -> buffs.sindorei_spite -> trigger();
      p() -> cooldowns.sindorei_spite_icd -> start( timespan_t::from_seconds( 180.0 ) );
    }
  }
};

struct summon_darkglare_t : public warlock_spell_t
{
  timespan_t darkglare_duration;

  summon_darkglare_t( warlock_t* p ) :
    warlock_spell_t( "summon_darkglare", p, p -> talents.summon_darkglare )
  {
    harmful = may_crit = may_miss = false;

    darkglare_duration = data().duration() + timespan_t::from_millis( 1 );
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    for ( size_t i = 0; i < p() -> warlock_pet_list.darkglare.size(); i++ )
    {
      if ( p() -> warlock_pet_list.darkglare[i] -> is_sleeping() )
      {
        p() -> warlock_pet_list.darkglare[i] -> summon( darkglare_duration );
        if(p()->legendary.wilfreds_sigil_of_superior_summoning_flag && !p()->talents.grimoire_of_supremacy->ok())
        {
            p()->cooldowns.doomguard->adjust(p()->legendary.wilfreds_sigil_of_superior_summoning);
            p()->cooldowns.infernal->adjust(p()->legendary.wilfreds_sigil_of_superior_summoning);
            p()->procs.wilfreds_darkglare->occur();
        }
      }
    }
  }
};

struct call_dreadstalkers_t : public warlock_spell_t
{
  timespan_t dreadstalker_duration;
  int dreadstalker_count;
  size_t improved_dreadstalkers;
  double recurrent_ritual;

  call_dreadstalkers_t( warlock_t* p ) :
    warlock_spell_t( "Call_Dreadstalkers", p, p -> find_spell( 104316 ) ),
    recurrent_ritual( 0.0 )
  {
    harmful = may_crit = false;
    dreadstalker_duration = p -> find_spell( 193332 ) -> duration() + ( p -> sets.has_set_bonus( WARLOCK_DEMONOLOGY, T19, B4 ) ? p -> sets.set( WARLOCK_DEMONOLOGY, T19, B4 ) -> effectN( 1 ).time_value() : timespan_t::zero() );
    dreadstalker_count = data().effectN( 1 ).base_value();
    improved_dreadstalkers = p -> talents.improved_dreadstalkers -> effectN( 1 ).base_value();
  }

  double cost() const override
  {
    double c = warlock_spell_t::cost();

    if ( p() -> buffs.demonic_calling -> check() )
    {
      return 0;
    }

    return c;
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    int j = 0;

    for ( size_t i = 0; i < p() -> warlock_pet_list.dreadstalkers.size(); i++ )
    {
      if ( p() -> warlock_pet_list.dreadstalkers[i] -> is_sleeping() )
      {
        p() -> warlock_pet_list.dreadstalkers[i] -> summon( dreadstalker_duration );
        p() -> procs.dreadstalker_debug -> occur();
        if(p()->legendary.wilfreds_sigil_of_superior_summoning_flag && !p()->talents.grimoire_of_supremacy->ok())
        {
            p()->cooldowns.doomguard->adjust(p()->legendary.wilfreds_sigil_of_superior_summoning);
            p()->cooldowns.infernal->adjust(p()->legendary.wilfreds_sigil_of_superior_summoning);
            p()->procs.wilfreds_dog->occur();
        }
        if ( ++j == dreadstalker_count ) break;
      }
    }

    if ( p() -> talents.improved_dreadstalkers -> ok() )
    {
      for ( size_t i = 0; i < improved_dreadstalkers; i++ )
      {
        trigger_wild_imp( p(), true );
        p() -> procs.improved_dreadstalkers -> occur();
      }
    }

    p() -> buffs.demonic_calling -> expire();

    if ( recurrent_ritual > 0 )
    {
      p() -> resource_gain( RESOURCE_SOUL_SHARD, recurrent_ritual, p() -> gains.recurrent_ritual );
    }
  }
};

// TODO: Melee range dropping shenanigans?
struct summon_soul_effigy_t : public warlock_spell_t
{
  summon_soul_effigy_t( warlock_t* p ) :
    warlock_spell_t( "soul_effigy", p, p -> talents.soul_effigy )
  {
    may_crit = may_miss = false;
  }

  void execute() override
  {
    warlock_spell_t::execute();

    // Bind Soul Effigy to the enemy targeted by this spell
    p() -> warlock_pet_list.soul_effigy -> target = execute_state -> target;
    p() -> warlock_pet_list.soul_effigy -> summon( data().duration() );
  }
};

struct effigy_damage_override_t: public warlock_spell_t
{
  //    effigy_damage_override_t( warlock_t * p, pets::soul_effigy_t *t ) :
  //        warlock_spell_t( "effigy_damage", p, NULL ,true )
  //    {
  //        this->initialized = true;
  //        this->target = t->damage->target;
  //        this->base_dd_min = t->damage->base_dd_min;
  //        this->base_dd_max = t->damage->base_dd_min;
  //    }
  effigy_damage_override_t( warlock_t * p ):
    warlock_spell_t( "effigy_damage", p, NULL )
  {
  }

  void setupEffigyStuff( pets::soul_effigy_spell_t *t )
  {
    this->target = t->target;
    this->base_dd_min = t->base_dd_min;
    this->base_dd_max = t->base_dd_min;
  }

  void execute() override
  {
    warlock_spell_t::execute();
  }

};

// TALENT SPELLS

// DEMONOLOGY



struct demonbolt_t : public warlock_spell_t
{
  cooldown_t* icd;

  demonbolt_t( warlock_t* p ) :
    warlock_spell_t( "demonbolt", p, p -> talents.demonbolt )
  {
    energize_type = ENERGIZE_ON_CAST;
    energize_resource = RESOURCE_SOUL_SHARD;
    energize_amount = 1;

    icd = p -> get_cooldown( "discord_icd" );

    if ( p -> sets.set( WARLOCK_DEMONOLOGY, T17, B4 ) )
    {
      if ( rng().roll( p -> sets.set( WARLOCK_DEMONOLOGY, T17, B4 ) -> effectN( 1 ).percent() ) )
      {
        energize_amount++;
      }
    }

    base_crit += p -> artifact.maw_of_shadows.percent();
  }

  //fix this
  virtual double action_multiplier() const override
  {
    double pm = spell_t::action_multiplier();
    double pet_counter = 0.0;

    for ( auto& pet : p()->pet_list )
    {
      pets::warlock_pet_t *lock_pet = static_cast< pets::warlock_pet_t* > ( pet );

      if ( lock_pet != NULL )
      {
        if ( !lock_pet->is_sleeping() )
        {
          pet_counter += data().effectN( 3 ).percent();
        }
      }
    }
    if ( p()->buffs.stolen_power->up() )
    {
      p()->procs.stolen_power_used->occur();
      pm *= 1.0 + p()->buffs.stolen_power->data().effectN( 1 ).percent();
      p()->buffs.stolen_power->reset();
    }

    pm *= 1 + pet_counter;
    return pm;
  }

  virtual timespan_t execute_time() const override
  {
    if ( p()->buffs.shadowy_inspiration->check() )
    {
      return timespan_t::zero();
    }
    return warlock_spell_t::execute_time();
  }

  void execute() override
  {
    warlock_spell_t::execute();

    if ( p() -> talents.demonic_calling -> ok() )
      p() -> buffs.demonic_calling -> trigger();

    if ( p() -> buffs.shadowy_inspiration -> check() )
      p() -> buffs.shadowy_inspiration -> expire();

    if ( p() -> artifact.thalkiels_discord.rank() && icd -> up() )
    {
      if ( rng().roll( p() -> artifact.thalkiels_discord.data().proc_chance() ) )
      {
        make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
          .target( execute_state -> target )
          .x( execute_state -> target -> x_position )
          .y( execute_state -> target -> y_position )
          .pulse_time( timespan_t::from_millis( 1500 ) )
          .duration( p() -> find_spell( 211729 ) -> duration() )
          .start_time( sim -> current_time() )
          .action( p() -> active.thalkiels_discord ) );

        p() -> procs.thalkiels_discord -> occur();
        icd -> start( timespan_t::from_seconds( 6.0 ));
      }
    }

    if ( p() -> sets.set( WARLOCK_DEMONOLOGY, T18, B2 ) )
    {
      p() -> buffs.tier18_2pc_demonology -> trigger( 1 );
    }
  }
};

struct implosion_t : public warlock_spell_t
{
    struct implosion_aoe_t: public warlock_spell_t
    {

      implosion_aoe_t( warlock_t* p ):
        warlock_spell_t( "implosion_aoe", p, p -> find_spell( 196278 ) )
      {
        aoe = -1;
        dual = true;
        background = true;
        callbacks = false;

        p -> spells.implosion_aoe = this;
      }
    };

    implosion_aoe_t* explosion;

    implosion_t(warlock_t* p) :
      warlock_spell_t( "implosion", p, p -> talents.implosion ),
      explosion( new implosion_aoe_t( p ) )
    {
      aoe = -1;
      add_child( explosion );
    }

    virtual bool ready() override
    {
      bool r = warlock_spell_t::ready();

      if(r)
      {
          for ( auto imp : p() -> warlock_pet_list.wild_imps )
          {
            if ( !imp -> is_sleeping() )
              return true;
          }
      }
      return false;
    }

    virtual void execute() override
    {
      warlock_spell_t::execute();
      for( auto imp : p() -> warlock_pet_list.wild_imps )
      {
        if( !imp -> is_sleeping() )
        {
          explosion -> execute();
          imp -> dismiss(true);
        }
      }
    }
};

struct shadowflame_t : public warlock_spell_t
{
  shadowflame_t( warlock_t* p ) :
    warlock_spell_t( "shadowflame", p, p -> talents.shadowflame )
  {
    hasted_ticks = tick_may_crit = true;

    dot_duration = timespan_t::from_seconds( 8.0 );
    spell_power_mod.tick = data().effectN( 2 ).sp_coeff();
    base_tick_time = data().effectN( 2 ).period();
  }

  timespan_t calculate_dot_refresh_duration( const dot_t* dot,
                                             timespan_t triggered_duration ) const override
  { return dot -> time_to_next_tick() + triggered_duration; }

  double composite_ta_multiplier( const action_state_t* state ) const override
  {
    double m = warlock_spell_t::composite_ta_multiplier( state );

    if ( td( state -> target ) -> dots_shadowflame -> is_ticking() )
      m *= td( target ) -> debuffs_shadowflame -> stack();

    return m;
  }

  void last_tick( dot_t* d ) override
  {
    warlock_spell_t::last_tick( d );

    td( d -> state -> target ) -> debuffs_shadowflame -> expire();
  }

  void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      td( s -> target  ) -> debuffs_shadowflame -> trigger();
    }
  }
};

struct drain_soul_t: public warlock_spell_t
{
  drain_soul_t( warlock_t* p ):
    warlock_spell_t( "drain_soul", p, p -> find_spell( 198590 ) )
  {
    channeled = true;
    hasted_ticks = false;
    may_crit = false;
  }

  virtual bool ready() override
  {
    if ( !maybe_ptr( p() -> dbc.ptr ) && !p() -> talents.drain_soul -> ok() )
      return false;

    return warlock_spell_t::ready();
  }

  virtual double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    m *= 1.0 + p() -> artifact.drained_to_a_husk.percent() * ( p() -> buffs.deadwind_harvester -> check() ? 2.0 : 1.0 );

    if ( p() -> specialization() == WARLOCK_AFFLICTION )
      m *= 1.0 + p() -> find_spell( 205183 ) -> effectN( 1 ).percent();

    return m;
  }

  virtual void tick( dot_t* d ) override
  {
    if ( p() -> sets.has_set_bonus( WARLOCK_AFFLICTION, T18, B2 ) )
    {
      p() -> buffs.shard_instability -> trigger();
    }

    warlock_spell_t::tick( d );
  }
};

struct cataclysm_t : public warlock_spell_t
{
  immolate_t* immolate;

  cataclysm_t( warlock_t* p ) :
    warlock_spell_t( "cataclysm", p, p -> talents.cataclysm ),
    immolate( new immolate_t( p ) )
  {
    aoe = -1;

    immolate -> background = true;
    immolate -> dual = true;
    immolate -> base_costs[RESOURCE_MANA] = 0;
  }

  virtual void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      immolate -> target = s -> target;
      immolate -> execute();
    }
  }
};

struct shadowburn_t: public warlock_spell_t
{
  struct resource_event_t: public player_event_t
  {
    shadowburn_t* spell;
    gain_t* shard_gain;
    player_t* target;

    resource_event_t( warlock_t* p, shadowburn_t* s, player_t* t ):
      player_event_t( *p, s -> delay ), spell( s ), shard_gain( p -> gains.shadowburn_shard ), target(t)
    {
    }
    virtual const char* name() const override
    { return "shadowburn_execute_gain"; }
    virtual void execute() override
    {
      if ( target -> is_sleeping() )
      {
        p() -> resource_gain( RESOURCE_SOUL_SHARD, 2, shard_gain );
      }
    }
  };
  resource_event_t* resource_event;
  timespan_t delay;
  timespan_t total_duration;
  timespan_t base_duration;
  shadowburn_t( warlock_t* p ):
    warlock_spell_t( "shadowburn", p, p -> talents.shadowburn ), resource_event( nullptr )
  {
    delay = data().effectN( 1 ).trigger() -> duration();

    if ( maybe_ptr(p -> dbc.ptr ) )
    {
      energize_type = ENERGIZE_ON_CAST;
      base_duration = p -> find_spell( 117828 ) -> duration();

      cooldown -> charges += p -> spec.conflagrate_2 -> effectN( 1 ).base_value();

      cooldown -> charges += p -> sets.set( WARLOCK_DESTRUCTION, T19, B4 ) -> effectN( 1 ).base_value();
      cooldown -> duration += p -> sets.set( WARLOCK_DESTRUCTION, T19, B4 ) -> effectN( 2 ).time_value();
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    resource_event = make_event<resource_event_t>( *sim, p(), this, s -> target );
  }

  void init() override
  {
    warlock_spell_t::init();

    if ( maybe_ptr( p() -> dbc.ptr ) )
    {
      cooldown -> hasted = true;
    }
  }

// Force spell to always crit
  double composite_crit_chance() const override
  {
    double cc = warlock_spell_t::composite_crit_chance();
  
    if ( maybe_ptr( p() -> dbc.ptr ) )
    {
      if ( p() -> buffs.conflagration_of_chaos -> check() )
        cc = 1.0;
    }

    return cc;
  }

  double calculate_direct_amount( action_state_t* state ) const override
  {
    warlock_spell_t::calculate_direct_amount( state );

    // Can't use player-based crit chance from the state object as it's hardcoded to 1.0. Use cached
    // player spell crit instead. The state target crit chance of the state object is correct.
    // Targeted Crit debuffs function as a separate multiplier.
    if ( maybe_ptr( p() -> dbc.ptr ) )
    {
      if ( p() -> buffs.conflagration_of_chaos -> check() )
        state -> result_total *= 1.0 + player -> cache.spell_crit_chance() + state -> target_crit_chance;
    }

    return state -> result_total;
  }

  void execute() override
  {
    warlock_spell_t::execute();

    if ( maybe_ptr( p() -> dbc.ptr ) )
    {
      if ( p() -> buffs.conflagration_of_chaos -> check() )
        p()->buffs.conflagration_of_chaos -> expire();

      p() -> buffs.conflagration_of_chaos -> trigger();

      if ( p() -> legendary.feretory_of_souls && rng().roll( p() -> find_spell( 205702 ) -> proc_chance() ) )
      {
        p() -> resource_gain( RESOURCE_SOUL_SHARD, 1.0, p() -> gains.feretory_of_souls );
      }
    }
  }
};

struct haunt_t: public warlock_spell_t
{
  haunt_t( warlock_t* p ):
    warlock_spell_t( "haunt", p, p -> talents.haunt )
  {
  }

  void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      td( s -> target ) -> debuffs_haunt -> trigger();
    }
  }
};

struct phantom_singularity_tick_t : public warlock_spell_t
{
  phantom_singularity_tick_t( warlock_t* p ):
    warlock_spell_t( "phantom_singularity_tick", p, p -> find_spell( 205246 ) )
  {
    background = true;
    may_miss = false;
    dual = true;
    aoe = -1;
  }
};

struct phantom_singularity_t : public warlock_spell_t
{
  phantom_singularity_tick_t* phantom_singularity;

  phantom_singularity_t( warlock_t* p ):
    warlock_spell_t( "phantom_singularity", p, p -> talents.phantom_singularity )
  {
    callbacks = false;
    hasted_ticks = true;

    phantom_singularity = new phantom_singularity_tick_t( p );
    add_child( phantom_singularity );
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    return s -> action -> tick_time( s ) * 8.0;
  }

  void tick( dot_t* d ) override
  {
    phantom_singularity -> execute();
    warlock_spell_t::tick( d );
  }
};

struct mana_tap_t : public warlock_spell_t
{
  double expenditure;
  mana_tap_t( warlock_t* p ) :
    warlock_spell_t( "mana_tap", p, p -> talents.mana_tap )
  {
    ignore_false_positive = true;
    harmful = false;
    may_crit = false;
    dot_duration = timespan_t::zero();
    resource_current = RESOURCE_MANA;
    base_costs[RESOURCE_MANA] = 1.0;
    expenditure = data().effectN( 2 ).percent();
  }

  virtual bool ready() override
  {
    if ( maybe_ptr( p() -> dbc.ptr ) )
      return false;

    return warlock_spell_t::ready();
  }

  void execute() override
  {
    warlock_spell_t::execute();

      p() -> buffs.mana_tap -> trigger();
  }

  double cost() const override
  {
    return p() -> resources.current[RESOURCE_MANA] * expenditure;
  }
};

struct siphon_life_t : public warlock_spell_t
{
  siphon_life_t( warlock_t* p ) :
    warlock_spell_t( "siphon_life", p, p -> talents.siphon_life )
  {
    may_crit = false;
  }

  virtual double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    if ( p() -> mastery_spells.potent_afflictions -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double m = warlock_spell_t::composite_target_multiplier( target );

    warlock_td_t* td = this -> td( target );

    if ( maybe_ptr( p() -> dbc.ptr ) && p() -> talents.malefic_grasp -> ok() && td -> dots_drain_soul -> is_ticking() )
      m *= 1.0 + p() -> find_spell( 235155 ) -> effectN( 1 ).percent();

    return m;
  }
};

struct soul_harvest_t : public warlock_spell_t
{
  int agony_action_id;
  int doom_action_id;
  int immolate_action_id;
  timespan_t base_duration;
  timespan_t total_duration;

  soul_harvest_t( warlock_t* p ) :
    warlock_spell_t( "soul_harvest", p, p -> talents.soul_harvest )
  {
    harmful = may_crit = may_miss = false;
    base_duration = data().duration();
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    if ( p() -> specialization() == WARLOCK_AFFLICTION )
    {
      total_duration = base_duration + timespan_t::from_seconds( 2.0 ) * p() -> get_active_dots( agony_action_id );
      p() -> buffs.soul_harvest -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, total_duration );
    }

    if ( p() -> specialization() == WARLOCK_DEMONOLOGY )
    {
      total_duration = base_duration + timespan_t::from_seconds( 2.0 ) * p() -> get_active_dots( doom_action_id );
      p() -> buffs.soul_harvest -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, total_duration );
    }

    if ( p() -> specialization() == WARLOCK_DESTRUCTION )
    {
      total_duration = base_duration + timespan_t::from_seconds( 2.0 ) * p() -> get_active_dots( immolate_action_id );
      p() -> buffs.soul_harvest -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, total_duration );
    }
  }

  virtual void init() override
  {
    warlock_spell_t::init();

    agony_action_id = p() -> find_action_id( "agony" );
    doom_action_id = p() -> find_action_id( "doom" );
    immolate_action_id = p() -> find_action_id( "immolate" );
  }
};

struct reap_souls_t: public warlock_spell_t
{
  timespan_t base_duration;
  timespan_t total_duration;
  int souls_consumed;
    reap_souls_t( warlock_t* p ) :
        warlock_spell_t( "reap_souls", p, p -> artifact.reap_souls ), souls_consumed( 0 )
    {
      harmful = may_crit = false;
      ignore_false_positive = true;

      base_duration = p -> buffs.deadwind_harvester -> buff_duration;
    }

    virtual bool ready() override
    {
      if ( !p() -> buffs.tormented_souls -> check() )
        return false;

      return warlock_spell_t::ready();
    }

    virtual void execute() override
    {
      warlock_spell_t::execute();

      if ( p() -> artifact.reap_souls.rank() && p() -> buffs.tormented_souls -> check() )
      {
        souls_consumed = p() -> buffs.tormented_souls -> stack();
        total_duration = base_duration * souls_consumed;
        p() -> buffs.deadwind_harvester -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, total_duration );
        for ( int i = 0; i < souls_consumed; ++i )
        {
          p() -> procs.souls_consumed -> occur();
        }
        p() -> buffs.tormented_souls -> decrement( souls_consumed );
      }
    }
};

struct grimoire_of_sacrifice_t: public warlock_spell_t
{
  grimoire_of_sacrifice_t( warlock_t* p ):
    warlock_spell_t( "grimoire_of_sacrifice", p, p -> talents.grimoire_of_sacrifice )
  {
    harmful = false;
    ignore_false_positive = true;
  }

  virtual bool ready() override
  {
    if ( ! p() -> warlock_pet_list.active ) return false;

    return warlock_spell_t::ready();
  }

  virtual void execute() override
  {
    if ( p() -> warlock_pet_list.active )
    {
      warlock_spell_t::execute();

      p() -> warlock_pet_list.active -> dismiss();
      p() -> warlock_pet_list.active = nullptr;
      p() -> buffs.demonic_power -> trigger();

    }
  }
};

struct demonic_power_damage_t : public warlock_spell_t
{
  demonic_power_damage_t( warlock_t* p ) :
    warlock_spell_t( "demonic_power", p, p -> find_spell( 196100 ) )
  {
    aoe = -1;
    background = true;
    proc = true;
    base_multiplier *= 1.0 + p -> artifact.impish_incineration.data().effectN( 3 ).percent();
    destro_mastery = false;

    // Hotfix on the aff hotfix spell, check regularly.
    if ( p -> specialization() == WARLOCK_AFFLICTION )
    {
      base_multiplier *= 1.0 + p -> find_spell( 137043 ) -> effectN( 1 ).percent();
    }
  }
};

struct harvester_of_souls_t : public warlock_spell_t
{
  harvester_of_souls_t( warlock_t* p ) :
    warlock_spell_t( "harvester_of_souls", p, p -> find_spell( 218615 ) )
  {
    background = true;
    //proc = true; Harvester of Souls can proc trinkets and has no resource cost so no need.
    callbacks = true;
  }
};

struct grimoire_of_service_t: public summon_pet_t
{
  grimoire_of_service_t( warlock_t* p, const std::string& pet_name ):
    summon_pet_t( "service_" + pet_name, p, p -> talents.grimoire_of_service -> ok() ? p -> find_class_spell( "Grimoire: " + pet_name ) : spell_data_t::not_found() )
  {
    cooldown = p -> get_cooldown( "grimoire_of_service" );
    cooldown -> duration = data().cooldown();
    summoning_duration = data().duration() + timespan_t::from_millis( 1 );

  }

  virtual void execute() override
  {
    pet -> is_grimoire_of_service = true;

    summon_pet_t::execute();
  }

  bool init_finished() override
  {
    if ( pet )
    {
      pet -> summon_stats = stats;
    }
    return summon_pet_t::init_finished();
  }
};

struct mortal_coil_heal_t: public warlock_heal_t
{
  mortal_coil_heal_t( warlock_t* p, const spell_data_t& s ):
    warlock_heal_t( "mortal_coil_heal", p, s.effectN( 3 ).trigger_spell_id() )
  {
    background = true;
    may_miss = false;
  }

  virtual void execute() override
  {
    double heal_pct = data().effectN( 1 ).percent();
    base_dd_min = base_dd_max = player -> resources.max[RESOURCE_HEALTH] * heal_pct;

    warlock_heal_t::execute();
  }
};

struct mortal_coil_t: public warlock_spell_t
{
  mortal_coil_heal_t* heal;

  mortal_coil_t( warlock_t* p ):
    warlock_spell_t( "mortal_coil", p, p -> talents.mortal_coil ), heal( nullptr )
  {
    base_dd_min = base_dd_max = 0;
    heal = new mortal_coil_heal_t( p, data() );
  }

  virtual void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
      heal -> execute();
  }
};

struct channel_demonfire_tick_t : public warlock_spell_t
{
  channel_demonfire_tick_t( warlock_t* p ) :
    warlock_spell_t( "channel_demonfire_tick", p, p -> find_spell( 196448 ) )
  {
    background = true;
    may_miss = false;
    dual = true;
    can_havoc = true;

    if ( maybe_ptr( p -> dbc.ptr ) )
    {
      aoe = -1;
      base_aoe_multiplier = data().effectN( 2 ).sp_coeff() / data().effectN( 1 ).sp_coeff();
    }
  }
};

struct channel_demonfire_t: public warlock_spell_t
{
  double backdraft_cast_time;
  double backdraft_tick_time;
  channel_demonfire_tick_t* channel_demonfire;
  int immolate_action_id;

  channel_demonfire_t( warlock_t* p ):
    warlock_spell_t( "channel_demonfire", p, p -> find_spell( 196447 ) ),
    immolate_action_id( 0 )
  {
    channeled = true;
    hasted_ticks = true;
    may_crit = false;
    can_havoc = false;

    channel_demonfire = new channel_demonfire_tick_t( p );
    add_child( channel_demonfire );


    if ( maybe_ptr( p -> dbc.ptr ) )
    {
      backdraft_cast_time = 1.0 + p -> buffs.backdraft -> data().effectN( 4 ).percent();
      backdraft_tick_time = 1.0 + p -> buffs.backdraft -> data().effectN( 3 ).percent();
    }
  }

  void init() override
  {
    warlock_spell_t::init();

    cooldown -> hasted = true;
    immolate_action_id = p() -> find_action_id( "immolate" );
  }

  std::vector< player_t* >& target_list() const override
  {
    target_cache.list = warlock_spell_t::target_list();

    size_t i = target_cache.list.size();
    while ( i > 0 )
    {
      i--;
      player_t* target_ = target_cache.list[i];
      if ( !td( target_ ) -> dots_immolate -> is_ticking() )
        target_cache.list.erase( target_cache.list.begin() + i );
    }
    return target_cache.list;
  }

  void tick( dot_t* d ) override
  {
    std::vector<player_t*> targets = target_list();

    if ( targets.size() > 0 )
    {
      channel_demonfire -> target = targets[ rng().range( 0, targets.size() - 1 ) ];
      channel_demonfire -> execute();
    }

    warlock_spell_t::tick( d );
  }

  timespan_t tick_time( const action_state_t* s ) const override
  {
    timespan_t t = warlock_spell_t::tick_time( s );

    if ( !maybe_ptr( p() -> dbc.ptr ) && p() -> buffs.backdraft -> check() ) //FIXME this might be an oversight on their part.
      t *= backdraft_tick_time;

    return t;
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    return s -> action -> tick_time( s ) * 15.0;
  }

  //void execute() override
  //{
  //  warlock_spell_t::execute();

  //  p() -> buffs.backdraft -> decrement();
  //}

  virtual bool ready() override
  {
    double active_immolates = p() -> get_active_dots( immolate_action_id );

    if ( active_immolates == 0 )
      return false;

    if ( !p() -> talents.channel_demonfire -> ok() )
      return false;

    return warlock_spell_t::ready();
  }
};

} // end actions namespace

namespace buffs
{
template <typename Base>
struct warlock_buff_t: public Base
{
public:
  typedef warlock_buff_t base_t;
  warlock_buff_t( warlock_td_t& p, const buff_creator_basics_t& params ):
    Base( params ), warlock( p.warlock )
  {}

  warlock_buff_t( warlock_t& p, const buff_creator_basics_t& params ):
    Base( params ), warlock( p )
  {}

  warlock_td_t& get_td( player_t* t ) const
  {
    return *( warlock.get_target_data( t ) );
  }

protected:
  warlock_t& warlock;
};

struct debuff_havoc_t: public warlock_buff_t < buff_t >
{
  debuff_havoc_t( warlock_td_t& p ):
    base_t( p, buff_creator_t( static_cast<actor_pair_t>( p ), "havoc", p.source -> find_spell( 80240 ) ) )
  {
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    base_t::expire_override( expiration_stacks, remaining_duration );
    warlock.havoc_target = nullptr;
  }
};

}

warlock_td_t::warlock_td_t( player_t* target, warlock_t& p ):
actor_target_data_t( target, &p ),
agony_stack( 1 ),
soc_threshold( 0 ),
warlock( p )
{
  using namespace buffs;
  dots_corruption = target -> get_dot( "corruption", &p );
  for ( int i = 0; i < MAX_UAS; i++ )
    dots_unstable_affliction[i] = target -> get_dot( "unstable_affliction_" + std::to_string( i + 1 ), &p );
  dots_agony = target -> get_dot( "agony", &p );
  dots_doom = target -> get_dot( "doom", &p );
  dots_drain_life = target -> get_dot( "drain_life", &p );
  dots_drain_soul = target -> get_dot( "drain_soul", &p );
  dots_immolate = target -> get_dot( "immolate", &p );
  dots_shadowflame = target -> get_dot( "shadowflame", &p );
  dots_seed_of_corruption = target -> get_dot( "seed_of_corruption", &p );
  dots_phantom_singularity = target -> get_dot( "phantom_singularity", &p );
  dots_channel_demonfire = target -> get_dot( "channel_demonfire", &p );

  debuffs_haunt = buff_creator_t( *this, "haunt", source -> find_spell( 48181 ) )
    .duration( timespan_t::from_seconds( 15 ) )
    .refresh_behavior( BUFF_REFRESH_PANDEMIC );
  debuffs_shadowflame = buff_creator_t( *this, "shadowflame", source -> find_spell( 205181 ) );
  debuffs_agony = buff_creator_t( *this, "agony", source -> find_spell( 980 ) )
    .refresh_behavior( BUFF_REFRESH_PANDEMIC );
  debuffs_eradication = buff_creator_t( *this, "eradication", source -> find_spell( 196414 ) )
    .refresh_behavior( BUFF_REFRESH_PANDEMIC );
  debuffs_roaring_blaze = buff_creator_t( *this, "roaring_blaze", source -> find_spell( 205690 ) )
    .max_stack( 100 );

  debuffs_havoc = new buffs::debuff_havoc_t( *this );

  if ( warlock.destruction_trinket )
  {
    debuffs_flamelicked = buff_creator_t( *this, "flamelicked", warlock.destruction_trinket -> driver() -> effectN( 1 ).trigger() )
      .default_value( warlock.destruction_trinket -> driver() -> effectN( 1 ).trigger() -> effectN( 1 ).average( warlock.destruction_trinket -> item ) / 100.0 );
  }
  else
  {
    debuffs_flamelicked = buff_creator_t( *this, "flamelicked" )
      .chance( 0 );
  }

  target -> callbacks_on_demise.push_back( std::bind( &warlock_td_t::target_demise, this ) );
}

void warlock_td_t::target_demise()
{
  if ( warlock.specialization() == WARLOCK_AFFLICTION && dots_drain_soul -> is_ticking() )
  {
    if ( warlock.sim -> log )
    {
      warlock.sim -> out_debug.printf( "Player %s demised. Warlock %s gains a shard by channeling drain soul during this.", target -> name(), warlock.name() );
    }
    warlock.resource_gain( RESOURCE_SOUL_SHARD, 1, warlock.gains.drain_soul );
  }
  if ( warlock.specialization() == WARLOCK_AFFLICTION && debuffs_haunt -> check() )
  {
    if ( warlock.sim -> log )
    {
      warlock.sim -> out_debug.printf( "Player %s demised. Warlock %s reset haunt's cooldown.", target -> name(), warlock.name() );
    }
    warlock.cooldowns.haunt -> reset( true );
  }
}

warlock_t::warlock_t( sim_t* sim, const std::string& name, race_e r ):
  player_t( sim, WARLOCK, name, r ),
    havoc_target( nullptr ),
    agony_accumulator( 0 ),
    warlock_pet_list( pets_t() ),
    active( active_t() ),
    talents( talents_t() ),
    legendary( legendary_t() ),
    glyphs( glyphs_t() ),
    mastery_spells( mastery_spells_t() ),
    cooldowns( cooldowns_t() ),
    spec( specs_t() ),
    buffs( buffs_t() ),
    gains( gains_t() ),
    procs( procs_t() ),
    spells( spells_t() ),
    initial_soul_shards( 3 ),
    default_pet( "" ),
    shard_react( timespan_t::zero() ),
    affliction_trinket( nullptr ),
    demonology_trinket( nullptr ),
    destruction_trinket( nullptr )
  {
    base.distance = 40;

    cooldowns.infernal = get_cooldown( "summon_infernal" );
    cooldowns.doomguard = get_cooldown( "summon_doomguard" );
    cooldowns.dimensional_rift = get_cooldown( "dimensional_rift" );
    cooldowns.haunt = get_cooldown( "haunt" );
    cooldowns.sindorei_spite_icd = get_cooldown( "sindorei_spite_icd" );

    regen_type = REGEN_DYNAMIC;
    regen_caches[CACHE_HASTE] = true;
    regen_caches[CACHE_SPELL_HASTE] = true;
    reap_souls_modifier = 2.0;
  }


double warlock_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( buffs.demonic_synergy -> check() )
    m *= 1.0 + buffs.demonic_synergy -> data().effectN( 1 ).percent();

  if ( legendary.stretens_insanity )
    m *= 1.0 + buffs.stretens_insanity -> stack() * buffs.stretens_insanity -> data().effectN( 1 ).percent();

  if ( buffs.empowered_life_tap -> check() )
    m *= 1.0 + talents.empowered_life_tap -> effectN( 1 ).percent();

  if ( buffs.mana_tap -> check() )
    m *= 1.0 + talents.mana_tap -> effectN( 1 ).percent();

  if ( buffs.soul_harvest -> check() )
    m *= 1.0 + talents.soul_harvest -> effectN( 1 ).percent();

  if ( buffs.instability -> check() )
    m *= 1.0 + find_spell( 216472 ) -> effectN( 1 ).percent();

  if ( specialization() == WARLOCK_DESTRUCTION && ( dbc::is_school( SCHOOL_FIRE, school ) || dbc::is_school( SCHOOL_CHROMATIC, school ) ) )
  {
    m *= 1.0 + artifact.flames_of_the_pit.percent();
  }

  if ( specialization() == WARLOCK_DEMONOLOGY && ( dbc::is_school( SCHOOL_FIRE, school ) || dbc::is_school( SCHOOL_SHADOW, school ) || dbc::is_school( SCHOOL_SHADOWFLAME, school ) || dbc::is_school( SCHOOL_CHAOS, school ) ) )
  {
    m *= 1.0 + artifact.breath_of_thalkiel.percent();
  }

  if ( specialization() == WARLOCK_AFFLICTION )
  {
    m *= 1.0 + artifact.soulstealer.percent() * ( buffs.deadwind_harvester -> check() ? 2.0 : 1.0 );
  }

  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
    m *= 1.0 + artifact.thalkiels_lingering_power.percent();
  }

  if ( specialization() == WARLOCK_DESTRUCTION )
  {
    m *= 1.0 + artifact.stolen_power.percent();
  }

  if ( specialization() == WARLOCK_AFFLICTION && ( dbc::is_school( SCHOOL_SHADOW, school ) ) )
  {
    m *= 1.0 + artifact.crystaline_shadows.percent() * ( buffs.deadwind_harvester -> check() ? 2.0 : 1.0 );
    m *= 1.0 + artifact.shadowy_incantations.percent() * ( buffs.deadwind_harvester -> check() ? 2.0 : 1.0 );
  }

  if ( buffs.deadwind_harvester -> check() )
  {
    m *= 1.0 + buffs.deadwind_harvester -> data().effectN( 1 ).percent();
  }

  m *= 1.0 + buffs.sindorei_spite -> check_stack_value();

  return m;
}

void warlock_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
  case CACHE_MASTERY:
    if ( mastery_spells.master_demonologist -> ok() )
      player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    break;
  default: break;
  }
}

double warlock_t::composite_spell_crit_chance() const
{
  double sc = player_t::composite_spell_crit_chance();

  return sc;
}

double warlock_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( buffs.misery -> check() )
  {
    h *= 1.0 / ( 1.0 + buffs.misery -> stack_value() );
  }

  return h;
}

double warlock_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  if ( buffs.misery -> check() )
  {
    h *= 1.0 / ( 1.0 + buffs.misery -> stack_value() );
  }

  return h;
}

double warlock_t::composite_melee_crit_chance() const
{
  double mc = player_t::composite_melee_crit_chance();

  return mc;
}

double warlock_t::composite_mastery() const
{
  double m = player_t::composite_mastery();

  return m;
}

double warlock_t::composite_rating_multiplier( rating_e rating ) const
{
  double m = player_t::composite_rating_multiplier( rating );

  return m;
}

double warlock_t::resource_gain( resource_e resource_type, double amount, gain_t* source, action_t* action )
{

  return player_t::resource_gain( resource_type, amount, source, action );
}

double warlock_t::mana_regen_per_second() const
{
  double mp5 = player_t::mana_regen_per_second();

  mp5 /= cache.spell_haste();

  return mp5;
}

double warlock_t::composite_armor() const
{
  return player_t::composite_armor() + spec.fel_armor -> effectN( 2 ).base_value();
}

void warlock_t::halt()
{
  player_t::halt();

  if ( spells.melee ) spells.melee -> cancel();
}

double warlock_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( attr == ATTR_INTELLECT )
    return spec.nethermancy -> effectN( 1 ).percent();

  return 0.0;
}

action_t* warlock_t::create_action( const std::string& action_name,
                                    const std::string& options_str )
{
  action_t* a;

  if ( ( action_name == "summon_pet" || action_name == "service_pet" ) && default_pet.empty() )
  {
    sim -> errorf( "Player %s used a generic pet summoning action without specifying a default_pet.\n", name() );
    return nullptr;
  }

  using namespace actions;

  if      ( action_name == "conflagrate"           ) a = new                       conflagrate_t( this );
  else if ( action_name == "corruption"            ) a = new                        corruption_t( this );
  else if ( action_name == "agony"                 ) a = new                             agony_t( this );
  else if ( action_name == "demonbolt"             ) a = new                         demonbolt_t( this );
  else if ( action_name == "doom"                  ) a = new                              doom_t( this );
  else if ( action_name == "chaos_bolt"            ) a = new                        chaos_bolt_t( this );
  else if ( action_name == "demonic_empowerment"   ) a = new               demonic_empowerment_t( this );
  else if ( action_name == "drain_life"            ) a = new                        drain_life_t( this );
  else if ( action_name == "drain_soul"            ) a = new                        drain_soul_t( this );
  else if ( action_name == "grimoire_of_sacrifice" ) a = new             grimoire_of_sacrifice_t( this );
  else if ( action_name == "haunt"                 ) a = new                             haunt_t( this );
  else if ( action_name == "phantom_singularity"   ) a = new               phantom_singularity_t( this );
  else if ( action_name == "channel_demonfire"     ) a = new                 channel_demonfire_t( this );
  else if ( action_name == "soul_harvest"          ) a = new                      soul_harvest_t( this );
  else if ( action_name == "siphon_life"           ) a = new                       siphon_life_t( this );
  else if ( action_name == "immolate"              ) a = new                          immolate_t( this );
  else if ( action_name == "incinerate"            ) a = new                        incinerate_t( this );
  else if ( action_name == "life_tap"              ) a = new                          life_tap_t( this );
  else if ( action_name == "mana_tap"              ) a = new                          mana_tap_t( this );
  else if ( action_name == "mortal_coil"           ) a = new                       mortal_coil_t( this );
  else if ( action_name == "shadow_bolt"           ) a = new                       shadow_bolt_t( this );
  else if ( action_name == "shadowburn"            ) a = new                        shadowburn_t( this );
  else if ( action_name == "unstable_affliction"   ) a = new               unstable_affliction_t( this );
  else if ( action_name == "hand_of_guldan"        ) a = new                    hand_of_guldan_t( this );
  else if ( action_name == "thalkiels_consumption" ) a = new             thalkiels_consumption_t( this );
  else if ( action_name == "implosion"             ) a = new                         implosion_t( this );
  else if ( action_name == "havoc"                 ) a = new                             havoc_t( this );
  else if ( action_name == "seed_of_corruption"    ) a = new                seed_of_corruption_t( this );
  else if ( action_name == "cataclysm"             ) a = new                         cataclysm_t( this );
  else if ( action_name == "rain_of_fire"          ) a = new         rain_of_fire_t( this, options_str );
  else if ( action_name == "demonwrath"            ) a = new                        demonwrath_t( this );
  else if ( action_name == "shadowflame"           ) a = new                       shadowflame_t( this );
  else if ( action_name == "reap_souls"            ) a = new                        reap_souls_t( this );
  else if ( action_name == "dimensional_rift"      ) a = new                  dimensional_rift_t( this );
  else if ( action_name == "call_dreadstalkers"    ) a = new                call_dreadstalkers_t( this );
  else if ( action_name == "soul_effigy"           ) a = new                summon_soul_effigy_t( this );
  else if ( action_name == "summon_infernal"       ) a = new                   summon_infernal_t( this );
  else if ( action_name == "summon_doomguard"      ) a = new                  summon_doomguard_t( this );
  else if ( action_name == "summon_darkglare"      ) a = new                  summon_darkglare_t( this );
  else if ( action_name == "summon_felhunter"      ) a = new      summon_main_pet_t( "felhunter", this );
  else if ( action_name == "summon_felguard"       ) a = new       summon_main_pet_t( "felguard", this );
  else if ( action_name == "summon_succubus"       ) a = new       summon_main_pet_t( "succubus", this );
  else if ( action_name == "summon_voidwalker"     ) a = new     summon_main_pet_t( "voidwalker", this );
  else if ( action_name == "summon_imp"            ) a = new            summon_main_pet_t( "imp", this );
  else if ( action_name == "summon_pet"            ) a = new      summon_main_pet_t( default_pet, this );
  else if ( action_name == "service_felguard"      ) a = new               grimoire_of_service_t( this, "felguard" );
  else if ( action_name == "service_felhunter"     ) a = new               grimoire_of_service_t( this, "felhunter" );
  else if ( action_name == "service_imp"           ) a = new               grimoire_of_service_t( this, "imp" );
  else if ( action_name == "service_succubus"      ) a = new               grimoire_of_service_t( this, "succubus" );
  else if ( action_name == "service_voidwalker"    ) a = new               grimoire_of_service_t( this, "voidwalker" );
  else if ( action_name == "service_pet"           ) a = new               grimoire_of_service_t( this,  talents.grimoire_of_supremacy -> ok() ? "doomguard" : default_pet );
  else return player_t::create_action( action_name, options_str );

  a -> parse_options( options_str );

  return a;
}

pet_t* warlock_t::create_pet( const std::string& pet_name,
                              const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  using namespace pets;

  if ( pet_name == "felguard"     ) return new    felguard_pet_t( sim, this );
  if ( pet_name == "felhunter"    ) return new   felhunter_pet_t( sim, this );
  if ( pet_name == "imp"          ) return new         imp_pet_t( sim, this );
  if ( pet_name == "succubus"     ) return new    succubus_pet_t( sim, this );
  if ( pet_name == "voidwalker"   ) return new  voidwalker_pet_t( sim, this );

  if ( pet_name == "service_felguard"     ) return new    felguard_pet_t( sim, this, pet_name );
  if ( pet_name == "service_felhunter"    ) return new   felhunter_pet_t( sim, this, pet_name );
  if ( pet_name == "service_imp"          ) return new         imp_pet_t( sim, this, pet_name );
  if ( pet_name == "service_succubus"     ) return new    succubus_pet_t( sim, this, pet_name );
  if ( pet_name == "service_voidwalker"   ) return new  voidwalker_pet_t( sim, this, pet_name );

  return nullptr;
}

void warlock_t::create_pets()
{
  for ( size_t i = 0; i < pet_name_list.size(); ++i )
  {
    create_pet( pet_name_list[ i ] );
  }

  for ( size_t i = 0; i < warlock_pet_list.infernal.size(); i++ )
  {
    warlock_pet_list.infernal[i] = new pets::infernal_t( sim, this );
  }
  for ( size_t i = 0; i < warlock_pet_list.doomguard.size(); i++ )
  {
    warlock_pet_list.doomguard[i] = new pets::doomguard_t( sim, this );
  }

  if ( artifact.lord_of_flames.rank() )
  {
    for ( size_t i = 0; i < warlock_pet_list.lord_of_flames_infernal.size(); i++ )
    {
      warlock_pet_list.lord_of_flames_infernal[i] = new pets::lord_of_flames_infernal_t( sim, this );
    }
  }

  if ( artifact.dimensional_rift.rank() )
  {
    for ( size_t i = 0; i < warlock_pet_list.shadowy_tear.size(); i++ )
    {
      warlock_pet_list.shadowy_tear[i] = new pets::shadowy_tear::shadowy_tear_t( sim, this );
    }
    for ( size_t i = 0; i < warlock_pet_list.chaos_tear.size(); i++ )
    {
      warlock_pet_list.chaos_tear[i] = new pets::chaos_tear_t( sim, this );
    }
    for ( size_t i = 0; i < warlock_pet_list.chaos_portal.size(); i++ )
    {
      warlock_pet_list.chaos_portal[i] = new pets::chaos_portal::chaos_portal_t( sim, this );
    }
  }

  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
    for ( size_t i = 0; i < warlock_pet_list.wild_imps.size(); i++ )
    {
      warlock_pet_list.wild_imps[ i ] = new pets::wild_imp_pet_t( sim, this );
      if ( i > 0 )
        warlock_pet_list.wild_imps[ i ] -> quiet = 1;
    }
    for ( size_t i = 0; i < warlock_pet_list.dreadstalkers.size(); i++ )
    {
      warlock_pet_list.dreadstalkers[ i ] = new pets::dreadstalker_t( sim, this );
    }
    for ( size_t i = 0; i < warlock_pet_list.darkglare.size(); i++ )
    {
      warlock_pet_list.darkglare[i] = new pets::darkglare_t( sim, this );
    }
    if ( sets.has_set_bonus( WARLOCK_DEMONOLOGY, T18, B4 ) )
    {
      for ( size_t i = 0; i < warlock_pet_list.t18_illidari_satyr.size(); i++ )
      {
        warlock_pet_list.t18_illidari_satyr[i] = new pets::t18_illidari_satyr_t( sim, this );
      }
      for ( size_t i = 0; i < warlock_pet_list.t18_prince_malchezaar.size(); i++ )
      {
        warlock_pet_list.t18_prince_malchezaar[i] = new pets::t18_prince_malchezaar_t( sim, this );
      }
      for ( size_t i = 0; i < warlock_pet_list.t18_vicious_hellhound.size(); i++ )
      {
        warlock_pet_list.t18_vicious_hellhound[i] = new pets::t18_vicious_hellhound_t( sim, this );
      }
    }
  }

  if ( talents.soul_effigy -> ok() && find_action( "soul_effigy" ) )
  {
    warlock_pet_list.soul_effigy = new pets::soul_effigy_t( this );
  }
}

void warlock_t::init_spells()
{
  player_t::init_spells();

  // General
  spec.fel_armor   = find_spell( 104938 );
  spec.nethermancy = find_spell( 86091 );

  // Specialization Spells
  // PTR
  spec.drain_soul             = find_specialization_spell( "Drain Soul" );

  spec.immolate               = find_specialization_spell( "Immolate" );
  spec.nightfall              = find_specialization_spell( "Nightfall" );
  spec.demonic_empowerment    = find_specialization_spell( "Demonic Empowerment" );
  spec.wild_imps              = find_specialization_spell( "Wild Imps" );
  spec.unstable_affliction    = find_specialization_spell( "Unstable Affliction" );
  spec.unstable_affliction_2  = find_specialization_spell( 231791 );
  spec.agony                  = find_specialization_spell( "Agony" );
  spec.agony_2                = find_specialization_spell( 231792 );
  spec.shadow_bite            = find_specialization_spell( "Shadow Bite" );
  spec.shadow_bite_2          = find_specialization_spell( 231799 );
  spec.conflagrate            = find_specialization_spell( "Conflagrate" );
  spec.conflagrate_2          = find_specialization_spell( 231793 );
  spec.unending_resolve       = find_specialization_spell( "Unending Resolve" );
  spec.unending_resolve_2     = find_specialization_spell( 231794 );
  spec.firebolt               = find_specialization_spell( "Firebolt" );
  spec.firebolt_2             = find_specialization_spell( 231795 );

  // Removed terniary for compat.
  spec.doom                   = find_spell( 603 );

  // Mastery
  mastery_spells.chaotic_energies    = find_mastery_spell( WARLOCK_DESTRUCTION );
  mastery_spells.potent_afflictions  = find_mastery_spell( WARLOCK_AFFLICTION );
  mastery_spells.master_demonologist = find_mastery_spell( WARLOCK_DEMONOLOGY );

  // Talents

  // Ptr
  talents.empowered_life_tap     = find_talent_spell( "Empowered Life Tap" );
  talents.malefic_grasp          = find_talent_spell( "Malefic Grasp" );

  talents.haunt                  = find_talent_spell( "Haunt" );
  talents.writhe_in_agony        = find_talent_spell( "Writhe in Agony" );
  talents.drain_soul             = find_talent_spell( "Drain Soul" );

  talents.backdraft              = find_talent_spell( "Backdraft" );
  talents.fire_and_brimstone     = find_talent_spell( "Fire and Brimstone" );
  talents.shadowburn             = find_talent_spell( "Shadowburn" );

  talents.shadowy_inspiration    = find_talent_spell( "Shadowy Inspiration" );
  talents.shadowflame            = find_talent_spell( "Shadowflame" );
  talents.demonic_calling        = find_talent_spell( "Demonic Calling" );

  talents.contagion              = find_talent_spell( "Contagion" );
  talents.absolute_corruption    = find_talent_spell( "Absolute Corruption" );

  talents.reverse_entropy        = find_talent_spell( "Reverse Entropy" );
  talents.roaring_blaze          = find_talent_spell( "Roaring Blaze" );

  talents.mana_tap               = find_talent_spell( "Mana Tap" );

  talents.impending_doom         = find_talent_spell( "Impending Doom" );
  talents.improved_dreadstalkers = find_talent_spell( "Improved Dreadstalkers" );
  talents.implosion              = find_talent_spell( "Implosion" );

  talents.demon_skin             = find_talent_spell( "Soul Leech" );
  talents.mortal_coil            = find_talent_spell( "Mortal Coil" );
  talents.howl_of_terror         = find_talent_spell( "Howl of Terror" );
  talents.shadowfury             = find_talent_spell( "Shadowfury" );

  talents.siphon_life            = find_talent_spell( "Siphon Life" );
  talents.sow_the_seeds          = find_talent_spell( "Sow the Seeds" );

  talents.eradication            = find_talent_spell( "Eradication" );
  talents.cataclysm              = find_talent_spell( "Cataclysm" );

  talents.hand_of_doom           = find_talent_spell( "Hand of Doom" );
  talents.power_trip			       = find_talent_spell( "Power Trip" );

  talents.soul_harvest           = find_talent_spell( "Soul Harvest" );

  talents.demonic_circle         = find_talent_spell( "Demonic Circle" );
  talents.burning_rush           = find_talent_spell( "Burning Rush" );
  talents.dark_pact              = find_talent_spell( "Dark Pact" );

  talents.grimoire_of_supremacy  = find_talent_spell( "Grimoire of Supremacy" );
  talents.grimoire_of_service    = find_talent_spell( "Grimoire of Service" );
  talents.grimoire_of_sacrifice  = find_talent_spell( "Grimoire of Sacrifice" );
  talents.grimoire_of_synergy    = find_talent_spell( "Grimoire of Synergy" );

  talents.soul_effigy            = find_talent_spell( "Soul Effigy" );
  talents.phantom_singularity    = find_talent_spell( "Phantom Singularity" );

  talents.wreak_havoc            = find_talent_spell( "Wreak Havoc" );
  talents.channel_demonfire      = find_talent_spell( "Channel Demonfire" );

  talents.summon_darkglare       = find_talent_spell( "Summon Darkglare" );
  talents.demonbolt              = find_talent_spell( "Demonbolt" );

  talents.soul_conduit           = find_talent_spell( "Soul Conduit" );

  // Artifacts
  artifact.reap_souls = find_artifact_spell( "Reap Souls" );
  artifact.crystaline_shadows = find_artifact_spell( "Crystaline Shadows" );
  artifact.seeds_of_doom = find_artifact_spell( "Seeds of Doom" );
  artifact.fatal_echoes = find_artifact_spell( "Fatal Echoes" );
  artifact.shadows_of_the_flesh = find_artifact_spell( "Shadows of the Flesh" );
  artifact.harvester_of_souls = find_artifact_spell( "Harvester of Souls" );
  artifact.inimitable_agony = find_artifact_spell( "Inimitable Agony" );
  artifact.drained_to_a_husk = find_artifact_spell( "Drained to a Husk" );
  artifact.inherently_unstable = find_artifact_spell( "Inherently Unstable" );
  artifact.sweet_souls = find_artifact_spell( "Sweet Souls" );
  artifact.perdition = find_artifact_spell( "Perdition" );
  artifact.wrath_of_consumption = find_artifact_spell( "Wrath of Consumption" );
  artifact.hideous_corruption = find_artifact_spell( "Hideous Corruption" );
  artifact.shadowy_incantations = find_artifact_spell( "Shadowy Incantations" );
  artifact.soul_flames = find_artifact_spell( "Soul Flames" );
  artifact.long_dark_night_of_the_soul = find_artifact_spell( "Long Dark Night of the Soul" );
  artifact.compounding_horror = find_artifact_spell( "Compounding Horror" );
  artifact.soulharvester = find_artifact_spell( "Soulharvester" );
  artifact.soulstealer = find_artifact_spell( "Soulstealer" );

  artifact.thalkiels_consumption = find_artifact_spell( "Thal'kiel's Consumption" );
  artifact.breath_of_thalkiel = find_artifact_spell( "Breath of Thal'kiel" );
  artifact.the_doom_of_azeroth = find_artifact_spell( "The Doom of Azeroth" );
  artifact.sharpened_dreadfangs = find_artifact_spell( "Sharpened Dreadfangs" );
  artifact.fel_skin = find_artifact_spell( "Fel Skin" );
  artifact.firm_resolve = find_artifact_spell( "Firm Resolve" );
  artifact.thalkiels_discord = find_artifact_spell( "Thal'kiel's Discord" );
  artifact.legionwrath = find_artifact_spell( "Legionwrath" );
  artifact.dirty_hands = find_artifact_spell( "Dirty Hands" );
  artifact.doom_doubled = find_artifact_spell( "Doom, Doubled" );
  artifact.infernal_furnace = find_artifact_spell( "Infernal Furnace" );
  artifact.the_expendables = find_artifact_spell( "The Expendables" );
  artifact.maw_of_shadows = find_artifact_spell( "Maw of Shadows" );
  artifact.open_link = find_artifact_spell( "Open Link" );
  artifact.stolen_power = find_artifact_spell( "Stolen Power" );
  artifact.imperator = find_artifact_spell( "Imp-erator" );
  artifact.summoners_prowess = find_artifact_spell( "Summoner's Prowess" );
  artifact.thalkiels_lingering_power = find_artifact_spell( "Thal'kiel's Lingering Power" );

  artifact.dimensional_rift = find_artifact_spell( "Dimensional Rift" );
  artifact.flames_of_the_pit = find_artifact_spell( "Flames of the Pit" );
  artifact.soulsnatcher = find_artifact_spell( "Soulsnatcher" );
  artifact.fire_and_the_flames = find_artifact_spell( "Fire and the Flames" );
  artifact.fire_from_the_sky = find_artifact_spell( "Fire from the Sky" );
  artifact.impish_incineration = find_artifact_spell( "Impish Incineration" );
  artifact.lord_of_flames = find_artifact_spell( "Lord of Flames" );
  artifact.eternal_struggle = find_artifact_spell( "Eternal Struggle" );
  artifact.demonic_durability = find_artifact_spell( "Demonic Durability" );
  artifact.chaotic_instability = find_artifact_spell( "Chaotic Instability" );
  artifact.dimension_ripper = find_artifact_spell( "Dimension Ripper" );
  artifact.master_of_distaster = find_artifact_spell( "Master of Disaster" );
  artifact.burning_hunger = find_artifact_spell( "Burning Hunger" );
  artifact.residual_flames = find_artifact_spell( "Residual Flames" );
  artifact.devourer_of_life = find_artifact_spell( "Devourer of Life" );
  artifact.planeswalker = find_artifact_spell( "Planeswalker" );
  artifact.conflagration_of_chaos = find_artifact_spell( "Conflagration of Chaos" );

  // Glyphs

  // Active Spells
  active.demonic_power_proc = new actions::demonic_power_damage_t( this );
  active.thalkiels_discord = new actions::thalkiels_discord_t( this );
  active.harvester_of_souls = new actions::harvester_of_souls_t( this );
  active.effigy_damage_override = new actions::effigy_damage_override_t(this);
  if ( specialization() == WARLOCK_AFFLICTION )
  {
    active.corruption = new actions::corruption_t( this );
    active.corruption -> background = true;
  }
}

void warlock_t::init_base_stats()
{
  player_t::init_base_stats();

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility = 0.0;
  base.spell_power_per_intellect = 1.0;

  base.attribute_multiplier[ATTR_STAMINA] *= 1.0 + spec.fel_armor -> effectN( 1 ).percent();

  base.mana_regen_per_second = resources.base[RESOURCE_MANA] * 0.01;

  resources.base[RESOURCE_SOUL_SHARD] = 5;

  if ( default_pet.empty() )
  {
    if ( specialization() == WARLOCK_AFFLICTION )
      default_pet = "felhunter";
    else if ( specialization() == WARLOCK_DEMONOLOGY )
      default_pet = "felguard";
    else if ( specialization() == WARLOCK_DESTRUCTION )
      default_pet = "imp";
  }
}

void warlock_t::init_scaling()
{
  player_t::init_scaling();
}

struct stolen_power_stack_t : public buff_t
{
  stolen_power_stack_t( warlock_t* p ) :
    buff_t( buff_creator_t( p, "stolen_power_stack", p -> find_spell( 211529 ) ).chance( 1.0 ) )
  {
  }

  void execute( int a, double b, timespan_t t ) override
  {
    warlock_t* p = debug_cast<warlock_t*>( player );

    buff_t::execute( a, b, t );

    if ( p -> buffs.stolen_power_stacks -> stack() == 100 )
    {
      p -> buffs.stolen_power_stacks -> reset();
      p -> buffs.stolen_power -> trigger();
    }
  }
};

struct t18_4pc_driver_t : public buff_t        //kept to force imps to proc
{
  timespan_t illidari_satyr_duration;
  timespan_t vicious_hellhound_duration;

  t18_4pc_driver_t( warlock_t* p ) :
    buff_t( buff_creator_t( p, "t18_4pc_driver" ).activated( true ).duration( timespan_t::from_millis( 500 ) ).tick_behavior( BUFF_TICK_NONE ) )
  {
    vicious_hellhound_duration = p -> find_spell( 189298 ) -> duration();
    illidari_satyr_duration = p -> find_spell( 189297 ) -> duration();
  }

  void execute( int a, double b, timespan_t t ) override
  {
    warlock_t* p = debug_cast<warlock_t*>( player );

    buff_t::execute( a, b, t );

    //Which pet will we spawn?
    double pet = rng().range( 0.0, 1.0 );
    if ( pet <= 0.6 ) // 60% chance to spawn hellhound
    {
      for ( size_t i = 0; i < p -> warlock_pet_list.t18_vicious_hellhound.size(); i++ )
      {
        if ( p -> warlock_pet_list.t18_vicious_hellhound[i] -> is_sleeping() )
        {
          p -> warlock_pet_list.t18_vicious_hellhound[i] -> summon( vicious_hellhound_duration );
          p -> procs.t18_vicious_hellhound -> occur();
          break;
        }
      }
    }
    else // 40% chance to spawn illidari
    {
      for ( size_t i = 0; i < p -> warlock_pet_list.t18_illidari_satyr.size(); i++ )
      {
        if ( p -> warlock_pet_list.t18_illidari_satyr[i] -> is_sleeping() )
        {
          p -> warlock_pet_list.t18_illidari_satyr[i] -> summon( illidari_satyr_duration );
          p -> procs.t18_illidari_satyr -> occur();
          break;
        }
      }
    }
  }
};

void warlock_t::create_buffs()
{
  player_t::create_buffs();

  buffs.demonic_power = buff_creator_t( this, "demonic_power", talents.grimoire_of_sacrifice -> effectN( 2 ).trigger() );
  buffs.mana_tap = buff_creator_t( this, "mana_tap", talents.mana_tap )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    .refresh_behavior( BUFF_REFRESH_PANDEMIC )
    .tick_behavior( BUFF_TICK_NONE );
  buffs.empowered_life_tap = buff_creator_t( this, "empowered_life_tap", talents.empowered_life_tap )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    .refresh_behavior( BUFF_REFRESH_PANDEMIC )
    .tick_behavior( BUFF_TICK_NONE );
  buffs.soul_harvest = buff_creator_t( this, "soul_harvest", find_spell( 196098 ) )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buffs.stretens_insanity = buff_creator_t( this, "stretens_insanity", find_spell( 208822 ) )
    .default_value( find_spell( 208822 ) -> effectN( 1 ).percent() )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  //affliction buffs
  buffs.shard_instability = buff_creator_t( this, "shard_instability", find_spell( 216457 ) )
    .chance( sets.set( WARLOCK_AFFLICTION, T18, B2 ) -> proc_chance() );
  buffs.instability = buff_creator_t( this, "instability", sets.set( WARLOCK_AFFLICTION, T18, B4 ) -> effectN( 1 ).trigger() )
    .chance( sets.set( WARLOCK_AFFLICTION, T18, B4 ) -> proc_chance() )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.misery = haste_buff_creator_t( this, "misery", find_spell( 216412 ) )
    .default_value( find_spell( 216412 ) -> effectN( 1 ).percent() );
  buffs.deadwind_harvester = buff_creator_t( this, "deadwind_harvester", find_spell( 216708 ) )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.tormented_souls = buff_creator_t( this, "tormented_souls", find_spell( 216695 ) )
    .tick_behavior( BUFF_TICK_NONE );
  buffs.compounding_horror = buff_creator_t( this, "compounding_horror", find_spell( 199281 ) );

  //demonology buffs
  buffs.demonic_synergy = buff_creator_t( this, "demonic_synergy", find_spell( 171982 ) )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    .chance( 1 );
  buffs.tier18_2pc_demonology = buff_creator_t( this, "demon_rush", sets.set( WARLOCK_DEMONOLOGY, T18, B2 ) -> effectN( 1 ).trigger() )
    .default_value( sets.set( WARLOCK_DEMONOLOGY, T18, B2 ) -> effectN( 1 ).trigger() -> effectN( 1 ).percent() );
  buffs.shadowy_inspiration = buff_creator_t( this, "shadowy_inspiration", find_spell( 196606 ) );
  buffs.demonic_calling = buff_creator_t( this, "demonic_calling", talents.demonic_calling -> effectN( 1 ).trigger() )
    .chance( find_spell( 205145 ) -> proc_chance() );
  buffs.t18_4pc_driver = new t18_4pc_driver_t( this );
  buffs.stolen_power_stacks = new stolen_power_stack_t( this );
  buffs.stolen_power = buff_creator_t( this, "stolen_power", find_spell( 211583 ) )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );


  //destruction buffs
  buffs.backdraft = buff_creator_t( this, "backdraft", find_spell( 117828 ) );
  buffs.lord_of_flames = buff_creator_t( this, "lord_of_flames", find_spell( 226802 ) )
    .tick_behavior( BUFF_TICK_NONE );
  buffs.conflagration_of_chaos = buff_creator_t( this, "conflagration_of_chaos", artifact.conflagration_of_chaos.data().effectN( 1 ).trigger() )
    .chance( artifact.conflagration_of_chaos.rank() ? artifact.conflagration_of_chaos.data().proc_chance() : 0.0 );
  buffs.embrace_chaos = buff_creator_t( this, "embrace_chaos", sets.set( WARLOCK_DESTRUCTION,T19, B2 ) -> effectN( 1 ).trigger() )
    .chance( sets.set( WARLOCK_DESTRUCTION, T19, B2 ) -> proc_chance() );
}

void warlock_t::init_rng()
{
  player_t::init_rng();

  misery_rppm = get_rppm( "misery", sets.set( WARLOCK_AFFLICTION, T17, B4 ) );
  demonic_power_rppm = get_rppm( "demonic_power", find_spell( 196099 ) );
  grimoire_of_synergy = get_rppm( "grimoire_of_synergy", talents.grimoire_of_synergy );
  grimoire_of_synergy_pet = get_rppm( "grimoire_of_synergy_pet", talents.grimoire_of_synergy );
  tormented_souls_rppm = get_rppm( "tormented_souls", 4.5 );
}

void warlock_t::init_gains()
{
  player_t::init_gains();

  gains.life_tap                    = get_gain( "life_tap" );
  gains.agony                       = get_gain( "agony" );
  gains.conflagrate                 = get_gain( "conflagrate" );
  gains.immolate                    = get_gain( "immolate" );
  gains.shadowburn_shard            = get_gain( "shadowburn_shard" );
  gains.miss_refund                 = get_gain( "miss_refund" );
  gains.seed_of_corruption          = get_gain( "seed_of_corruption" );
  gains.drain_soul                  = get_gain( "drain_soul" );
  gains.mana_tap                    = get_gain( "mana_tap" );
  gains.shadow_bolt                 = get_gain( "shadow_bolt" );
  gains.soul_conduit                = get_gain( "soul_conduit" );
  gains.reverse_entropy             = get_gain( "reverse_entropy" );
  gains.soulsnatcher                = get_gain( "soulsnatcher" );
  gains.power_trip                  = get_gain( "power_trip" );
  gains.t18_4pc_destruction         = get_gain( "t18_4pc_destruction" );
  gains.demonwrath                  = get_gain( "demonwrath" );
  gains.t19_2pc_demonology          = get_gain( "t19_2pc_demonology" );
  gains.recurrent_ritual            = get_gain( "recurrent_ritual" );
  gains.feretory_of_souls           = get_gain( "feretory_of_souls" );
  gains.power_cord_of_lethtendris   = get_gain( "power_cord_of_lethtendris" );
}

// warlock_t::init_procs ===============================================

void warlock_t::init_procs()
{
  player_t::init_procs();

  procs.fatal_echos = get_proc( "fatal_echos" );
  procs.wild_imp = get_proc( "wild_imp" );
  procs.fragment_wild_imp = get_proc( "fragment_wild_imp" );
  procs.t18_2pc_affliction = get_proc( "t18_2pc_affliction" );
  procs.t18_4pc_destruction = get_proc( "t18_4pc_destruction" );
  procs.t18_prince_malchezaar = get_proc( "t18_prince_malchezaar" );
  procs.t18_vicious_hellhound = get_proc( "t18_vicious_hellhound" );
  procs.t18_illidari_satyr = get_proc( "t18_illidari_satyr" );
  procs.shadowy_tear = get_proc( "shadowy_tear" );
  procs.chaos_tear = get_proc( "chaos_tear" );
  procs.chaos_portal = get_proc( "chaos_portal" );
  procs.dreadstalker_debug = get_proc( "dreadstalker_debug" );
  procs.dimension_ripper = get_proc( "dimension_ripper" );
  procs.one_shard_hog = get_proc( "one_shard_hog" );
  procs.two_shard_hog = get_proc( "two_shard_hog" );
  procs.three_shard_hog = get_proc( "three_shard_hog" );
  procs.four_shard_hog = get_proc( "four_shard_hog" );
  procs.impending_doom = get_proc( "impending_doom" );
  procs.improved_dreadstalkers = get_proc( "improved_dreadstalkers" );
  procs.thalkiels_discord = get_proc( "thalkiels_discord" );
  procs.demonic_calling = get_proc( "demonic_calling" );
  procs.power_trip = get_proc( "power_trip" );
  procs.stolen_power_stack = get_proc( "stolen_power_proc" );
  procs.stolen_power_used = get_proc( "stolen_power_used" );
  procs.soul_conduit = get_proc( "soul_conduit" );
  procs.t18_demo_4p = get_proc( "t18_demo_4p" );
  procs.souls_consumed = get_proc( "souls_consumed" );
  procs.the_expendables = get_proc( "the_expendables" );
  procs.wilfreds_dog = get_proc( "wilfreds_dog" );
  procs.wilfreds_imp = get_proc( "wilfreds_imp" );
  procs.wilfreds_darkglare = get_proc( "wilfreds_darkglare" );
}

void warlock_t::apl_precombat()
{
  std::string& precombat_list =
    get_action_priority_list( "precombat" )->action_list_str;

  if ( sim -> allow_flasks )
  {
    // Flask
    if ( true_level == 110 )
      precombat_list = "flask,type=whispered_pact";
    else if ( true_level >= 100 )
      precombat_list = "flask,type=greater_draenic_intellect_flask";
  }

  if ( sim -> allow_food )
  {
    // Food
    if ( true_level == 110 && specialization() == WARLOCK_AFFLICTION )
      precombat_list += "/food,type=nightborne_delicacy_platter";
    else if ( true_level == 110 )
      precombat_list += "/food,type=azshari_salad";
    else if ( true_level >= 100 && specialization() == WARLOCK_DESTRUCTION )
      precombat_list += "/food,type=frosty_stew";
    else if ( true_level >= 100 && specialization() == WARLOCK_DEMONOLOGY)
      precombat_list += "/food,type=frosty_stew";
    else if ( true_level >= 100 && specialization() == WARLOCK_AFFLICTION )
      precombat_list += "/food,type=felmouth_frenzy";
  }

  precombat_list += "/summon_pet,if=!talent.grimoire_of_supremacy.enabled&(!talent.grimoire_of_sacrifice.enabled|buff.demonic_power.down)";
  precombat_list += "/summon_infernal,if=talent.grimoire_of_supremacy.enabled&artifact.lord_of_flames.rank>0";
  precombat_list += "/summon_infernal,if=talent.grimoire_of_supremacy.enabled&active_enemies>=3";
  precombat_list += "/summon_doomguard,if=talent.grimoire_of_supremacy.enabled&active_enemies<3&artifact.lord_of_flames.rank=0";

  if ( true_level > 100 )
    precombat_list += "/augmentation,type=defiled";

  precombat_list += "/snapshot_stats";

  if ( specialization() != WARLOCK_DEMONOLOGY )
    precombat_list += "/grimoire_of_sacrifice,if=talent.grimoire_of_sacrifice.enabled";

  if ( sim -> allow_potions )
  {
    // Pre-potion
    if ( true_level == 110 )
      precombat_list += "/potion,name=deadly_grace";
    else if ( true_level >= 100 )
      precombat_list += "/potion,name=draenic_intellect";
  }

  if ( specialization() != WARLOCK_DEMONOLOGY && !maybe_ptr( dbc.ptr ) )
    precombat_list += "/mana_tap,if=talent.mana_tap.enabled&!buff.mana_tap.remains";

  if ( specialization() == WARLOCK_DESTRUCTION )
    precombat_list += "/chaos_bolt";

  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
    precombat_list += "/demonic_empowerment";
    precombat_list += "/demonbolt,if=talent.demonbolt.enabled";
    precombat_list += "/shadow_bolt,if=!talent.demonbolt.enabled";
  }
}

void warlock_t::apl_global_filler()
{
  add_action( "Life Tap" );
}

void warlock_t::apl_default()
{
}

void warlock_t::apl_affliction()
{
  add_action( "Reap Souls", "if=actions=reap_souls,if=!buff.deadwind_harvester.remains&(buff.soul_harvest.remains|buff.tormented_souls.react>=8|target.time_to_die<=buff.tormented_souls.react*5|trinket.proc.any.react" );
  if ( find_item( "horn_of_valor" ) )
    action_list_str += "|buff.valarjars_path.remains";
  if ( find_item( "moonlit_prism" ) )
    action_list_str += "|buff.elunes_light.remains";
  if ( find_item( "obelisk of_the_void" ) )
    action_list_str += "|buff.collapsing_shadow.remains";
  action_list_str += ")";

  action_list_str += "/soul_effigy,if=!pet.soul_effigy.active";
  add_action( "Agony", "cycle_targets=1,if=remains<=tick_time+gcd" );
  action_list_str += "/service_pet,if=dot.corruption.remains&dot.agony.remains";
  add_action( "Summon Doomguard", "if=!talent.grimoire_of_supremacy.enabled&spell_targets.summon_infernal<3&(target.time_to_die>180|target.health.pct<=20|target.time_to_die<30)" );
  add_action( "Summon Infernal", "if=!talent.grimoire_of_supremacy.enabled&spell_targets.summon_infernal>=3" );
  add_action( "Summon Doomguard", "if=talent.grimoire_of_supremacy.enabled&spell_targets.summon_infernal<3&equipped.132379&!cooldown.sindorei_spite_icd.remains" );
  add_action( "Summon Infernal", "if=talent.grimoire_of_supremacy.enabled&spell_targets.summon_infernal>=3&equipped.132379&!cooldown.sindorei_spite_icd.remains" );
  action_list_str += "/berserking";
  action_list_str += "/blood_fury";
  action_list_str += "/arcane_torrent";
  action_list_str += init_use_profession_actions();
  action_list_str += "/soul_harvest";
  for ( int i = as< int >( items.size() ) - 1; i >= 0; i-- )
  {
    if ( items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
    {
      action_list_str += "/use_item,name=";
      action_list_str += items[i].name();
    }
  }

  action_list_str += "/potion,name=deadly_grace,if=buff.soul_harvest.remains|trinket.proc.any.react|target.time_to_die<=45";
  if ( find_item( "horn_of_valor" ) )
    action_list_str += "|buff.valarjars_path.remains";
  if ( find_item( "moonlit_prism" ) )
    action_list_str += "|buff.elunes_light.remains";
  if ( find_item( "obelisk of_the_void" ) )
    action_list_str += "|buff.collapsing_shadow.remains";

  add_action( "Corruption", "cycle_targets=1,if=remains<=tick_time+gcd" );
  add_action( "Siphon Life", "cycle_targets=1,if=remains<=tick_time+gcd" );
  action_list_str += "/mana_tap,if=ptr=0&buff.mana_tap.remains<=buff.mana_tap.duration*0.3&(mana.pct<20|buff.mana_tap.remains<=gcd)&target.time_to_die>buff.mana_tap.duration*0.3";
  action_list_str += "/phantom_singularity";
  add_action( "Unstable Affliction", "if=talent.contagion.enabled|(soul_shard>=4|trinket.proc.intellect.react|trinket.stacking_proc.mastery.react|trinket.proc.mastery.react|trinket.proc.crit.react|trinket.proc.versatility.react|buff.soul_harvest.remains|buff.deadwind_harvester.remains|buff.compounding_horror.react=5|target.time_to_die<=20" );
  if ( find_item( "horn_of_valor" ) )
    action_list_str += "|buff.valarjars_path.remains";
  if ( find_item( "moonlit_prism" ) )
    action_list_str += "|buff.elunes_light.remains";
  if ( find_item( "obelisk of_the_void" ) )
    action_list_str += "|buff.collapsing_shadow.remains";
  action_list_str += ")";

  add_action( "Agony", "cycle_targets=1,if=remains<=duration*0.3&target.time_to_die>=remains" );
  add_action( "Corruption", "cycle_targets=1,if=remains<=duration*0.3&target.time_to_die>=remains" );
  action_list_str += "/haunt";
  add_action( "Siphon Life", "cycle_targets=1,if=remains<=duration*0.3&target.time_to_die>=remains" );
  add_action( "Life Tap", "if=mana.pct<=10" );
  action_list_str += "/drain_soul,chain=1,interrupt=1";
  add_action( "Drain Life", "chain=1,interrupt=1" );

}

void warlock_t::apl_demonology()
{
  action_list_str += "/implosion,if=wild_imp_remaining_duration<=action.shadow_bolt.execute_time&buff.demonic_synergy.remains";
  action_list_str += "/implosion,if=prev_gcd.hand_of_guldan&wild_imp_remaining_duration<=3&buff.demonic_synergy.remains";
  action_list_str += "/implosion,if=wild_imp_count<=4&wild_imp_remaining_duration<=action.shadow_bolt.execute_time&spell_targets.implosion>1";
  action_list_str += "/implosion,if=prev_gcd.hand_of_guldan&wild_imp_remaining_duration<=4&spell_targets.implosion>2";
  action_list_str += "/shadowflame,if=debuff.shadowflame.stack>0&remains<action.shadow_bolt.cast_time+travel_time";
  action_list_str += "/service_pet";
  add_action( "Summon Doomguard", "if=!talent.grimoire_of_supremacy.enabled&spell_targets.infernal_awakening<3&(target.time_to_die>180|target.health.pct<=20|target.time_to_die<30)" );
  add_action( "Summon Infernal", "if=!talent.grimoire_of_supremacy.enabled&spell_targets.infernal_awakening>=3" );
  add_action( "Summon Doomguard", "if=talent.grimoire_of_supremacy.enabled&spell_targets.summon_infernal<3&equipped.132379&!cooldown.sindorei_spite_icd.remains" );
  add_action( "Summon Infernal", "if=talent.grimoire_of_supremacy.enabled&spell_targets.summon_infernal>=3&equipped.132379&!cooldown.sindorei_spite_icd.remains" );
  add_action( "Call Dreadstalkers", "if=!talent.summon_darkglare.enabled&(spell_targets.implosion<3|!talent.implosion.enabled)" );
  add_action( "Hand of Gul'dan", "if=soul_shard>=4&!talent.summon_darkglare.enabled" );
  action_list_str += "/summon_darkglare,if=prev_gcd.hand_of_guldan";
  action_list_str += "/summon_darkglare,if=prev_gcd.call_dreadstalkers";
  action_list_str += "/summon_darkglare,if=cooldown.call_dreadstalkers.remains>5&soul_shard<3";
  action_list_str += "/summon_darkglare,if=cooldown.call_dreadstalkers.remains<=action.summon_darkglare.cast_time&soul_shard>=3";
  action_list_str += "/summon_darkglare,if=cooldown.call_dreadstalkers.remains<=action.summon_darkglare.cast_time&soul_shard>=1&buff.demonic_calling.react";
  add_action( "Call Dreadstalkers", "if=talent.summon_darkglare.enabled&(spell_targets.implosion<3|!talent.implosion.enabled)&cooldown.summon_darkglare.remains>2" );
  add_action( "Call Dreadstalkers", "if=talent.summon_darkglare.enabled&(spell_targets.implosion<3|!talent.implosion.enabled)&prev_gcd.summon_darkglare" );
  add_action( "Call Dreadstalkers", "if=talent.summon_darkglare.enabled&(spell_targets.implosion<3|!talent.implosion.enabled)&cooldown.summon_darkglare.remains<=action.call_dreadstalkers.cast_time&soul_shard>=3" );
  add_action( "Call Dreadstalkers", "if=talent.summon_darkglare.enabled&(spell_targets.implosion<3|!talent.implosion.enabled)&cooldown.summon_darkglare.remains<=action.call_dreadstalkers.cast_time&soul_shard>=1&buff.demonic_calling.react" );
  add_action( "Hand of Gul'dan", "if=soul_shard>=3&prev_gcd.call_dreadstalkers" );
  add_action( "Hand of Gul'dan", "if=soul_shard>=5&cooldown.summon_darkglare.remains<=action.hand_of_guldan.cast_time" );
  add_action( "Hand of Gul'dan", "if=soul_shard>=4&cooldown.summon_darkglare.remains>2" );
  add_action( "Demonic Empowerment", "if=wild_imp_no_de>3|prev_gcd.hand_of_guldan" );
  add_action( "Demonic Empowerment", "if=dreadstalker_no_de>0|darkglare_no_de>0|doomguard_no_de>0|infernal_no_de>0|service_no_de>0" );
  add_action( "Doom", "cycle_targets=1,if=!talent.hand_of_doom.enabled&target.time_to_die>duration&(!ticking|remains<duration*0.3)" );
  for ( int i = as< int >( items.size() ) - 1; i >= 0; i-- )
  {
    if ( items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
    {
      action_list_str += "/use_item,name=";
      action_list_str += items[i].name();
    }
  }
  action_list_str += "/arcane_torrent";
  action_list_str += "/berserking";
  action_list_str += "/blood_fury";
  action_list_str += "/soul_harvest";
  action_list_str += "/potion,name=deadly_grace,if=buff.soul_harvest.remains|target.time_to_die<=45|trinket.proc.any.react";
  action_list_str += "/shadowflame,if=charges=2";
  add_action( "Thal'kiel's Consumption", "if=(dreadstalker_remaining_duration>execute_time|talent.implosion.enabled&spell_targets.implosion>=3)&wild_imp_count>3&wild_imp_remaining_duration>execute_time" );
  add_action( "Life Tap", "if=mana.pct<=30" );
  add_action( "Demonwrath", "chain=1,interrupt=1,if=spell_targets.demonwrath>=3" );
  add_action( "Demonwrath", "moving=1,chain=1,interrupt=1" );
  action_list_str += "/demonbolt";
  add_action( "Shadow Bolt" );
}

void warlock_t::apl_destruction()
{
  // action_priority_list_t* single_target       = get_action_priority_list( "single_target" );
  // action_priority_list_t* aoe                 = get_action_priority_list( "aoe" );

  // artifact check


  for ( int i = as< int >( items.size() ) - 1; i >= 0; i-- )
  {
    if ( items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
    {
      action_list_str += "/use_item,name=";
      action_list_str += items[i].name();
    }
  }

  add_action( "Havoc", "target=2,if=active_enemies>1&active_enemies<6&!debuff.havoc.remains" );
  add_action( "Havoc", "target=2,if=active_enemies>1&!talent.wreak_havoc.enabled&talent.roaring_blaze.enabled&!debuff.roaring_blaze.remains");
  add_action( "Dimensional Rift", "if=charges=3" );
  add_action( "Immolate", "if=remains<=tick_time" );
  add_action( "Immolate", "cycle_targets=1,if=active_enemies>1&remains<=tick_time&(!talent.roaring_blaze.enabled|(!debuff.roaring_blaze.remains&action.conflagrate.charges<2))");
  add_action( "Immolate", "if=talent.roaring_blaze.enabled&remains<=duration&!debuff.roaring_blaze.remains&target.time_to_die>10&(action.conflagrate.charges=2|(action.conflagrate.charges>=1&action.conflagrate.recharge_time<cast_time+gcd)|target.time_to_die<24)" ); 
  action_list_str += "/berserking";
  action_list_str += "/blood_fury";
  action_list_str += "/arcane_torrent";
  action_list_str += "/potion,name=deadly_grace,if=(buff.soul_harvest.remains|trinket.proc.any.react|target.time_to_die<=45)";
  action_list_str += "/shadowburn,if=ptr=1&talent.roaring_blaze.enabled&(charges=2|(charges>=1&recharge_time<gcd)|target.time_to_die<24)";
  action_list_str += "/shadowburn,if=ptr=1&talent.roaring_blaze.enabled&debuff.roaring_blaze.stack>0&dot.immolate.remains>dot.immolate.duration*0.3&(active_enemies=1|soul_shard<3)&soul_shard<5";
  action_list_str += "/shadowburn", "if=ptr=1&!talent.roaring_blaze.enabled&!buff.backdraft.remains&buff.conflagration_of_chaos.remains<=action.chaos_bolt.cast_time";
  action_list_str += "/shadowburn,if=ptr=1&!talent.roaring_blaze.enabled&!buff.backdraft.remains&(charges=1&recharge_time<action.chaos_bolt.cast_time|charges=2)&soul_shard<5";
  add_action( "Conflagrate", "if=talent.roaring_blaze.enabled&(charges=2|(charges>=1&recharge_time<gcd)|target.time_to_die<24)" );
  add_action( "Conflagrate", "if=talent.roaring_blaze.enabled&debuff.roaring_blaze.stack>0&dot.immolate.remains>dot.immolate.duration*0.3&(active_enemies=1|soul_shard<3)&soul_shard<5" );
  add_action( "Conflagrate", "if=!talent.roaring_blaze.enabled&!buff.backdraft.remains&buff.conflagration_of_chaos.remains<=action.chaos_bolt.cast_time" );
  add_action( "Conflagrate", "if=!talent.roaring_blaze.enabled&!buff.backdraft.remains&(charges=1&recharge_time<action.chaos_bolt.cast_time|charges=2)&soul_shard<5" );
  action_list_str += "/service_pet";
  add_action( "Summon Infernal", "if=artifact.lord_of_flames.rank>0&!buff.lord_of_flames.remains" );
  add_action( "Summon Doomguard", "if=!talent.grimoire_of_supremacy.enabled&spell_targets.infernal_awakening<3&(target.time_to_die>180|target.health.pct<=20|target.time_to_die<30)" );
  add_action( "Summon Infernal", "if=!talent.grimoire_of_supremacy.enabled&spell_targets.infernal_awakening>=3" );
  add_action( "Summon Doomguard", "if=talent.grimoire_of_supremacy.enabled&artifact.lord_of_flames.rank>0&buff.lord_of_flames.remains&!pet.doomguard.active" );
  add_action( "Summon Doomguard", "if=talent.grimoire_of_supremacy.enabled&spell_targets.summon_infernal<3&equipped.132379&!cooldown.sindorei_spite_icd.remains" );
  add_action( "Summon Infernal", "if=talent.grimoire_of_supremacy.enabled&spell_targets.summon_infernal>=3&equipped.132379&!cooldown.sindorei_spite_icd.remains" );
  action_list_str += "/soul_harvest";

  if ( !maybe_ptr( dbc.ptr ) )
    action_list_str += "/channel_demonfire,if=dot.immolate.remains>cast_time";
  add_action( "Chaos Bolt", "if=soul_shard>3|buff.backdraft.remains" );
  add_action( "Chaos Bolt", "if=buff.backdraft.remains&prev_gcd.incinerate" );
  add_action( "Incinerate", "if=buff.backdraft.remains" );
  add_action( "Havoc", "if=active_enemies=1&talent.wreak_havoc.enabled&equipped.132375&!debuff.havoc.remains" );
  add_action("Rain of Fire", "if=active_enemies>=4&cooldown.havoc.remains<=12&!talent.wreak_havoc.enabled");
  add_action("Rain of Fire", "if=active_enemies>=6&talent.wreak_havoc.enabled");
  add_action( "Dimensional Rift" );
  action_list_str += "/mana_tap,if=ptr=0&buff.mana_tap.remains<=buff.mana_tap.duration*0.3&(mana.pct<20|buff.mana_tap.remains<=action.chaos_bolt.cast_time)&target.time_to_die>buff.mana_tap.duration*0.3";
  add_action( "Chaos Bolt" );
  action_list_str += "/cataclysm";
  action_list_str += "/shadowburn,if=ptr=1&!buff.backdraft.remains";
  add_action( "Conflagrate", "if=!talent.roaring_blaze.enabled&!buff.backdraft.remains" );
  add_action( "Immolate", "if=!talent.roaring_blaze.enabled&remains<=duration*0.3" );
  add_action( "Life Tap", "if=ptr=0&talent.mana_tap.enabled&mana.pct<=10" );
  add_action( "Incinerate" );
}

void warlock_t::init_action_list()
{
  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    apl_precombat();

    switch ( specialization() )
    {
    case WARLOCK_AFFLICTION:
      apl_affliction();
      break;
    case WARLOCK_DESTRUCTION:
      apl_destruction();
      break;
    case WARLOCK_DEMONOLOGY:
      apl_demonology();
      break;
    default:
      apl_default();
      break;
    }

    apl_global_filler();

    use_default_action_list = true;
  }

  player_t::init_action_list();
}

void warlock_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[RESOURCE_SOUL_SHARD] = initial_soul_shards;

  if ( warlock_pet_list.active )
    warlock_pet_list.active -> init_resources( force );
}

void warlock_t::combat_begin()
{

  player_t::combat_begin();
}

void warlock_t::reset()
{
  player_t::reset();

  for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
  {
    warlock_td_t* td = target_data[sim -> actor_list[i]];
    if ( td ) td -> reset();
  }

  warlock_pet_list.active = nullptr;
  shard_react = timespan_t::zero();
  havoc_target = nullptr;
  agony_accumulator = rng().range( 0.0, 0.99 );
  demonwrath_accumulator = 0.0;
}

void warlock_t::create_options()
{
  player_t::create_options();

  add_option( opt_int( "soul_shards", initial_soul_shards ) );
  add_option( opt_string( "default_pet", default_pet ) );
}

std::string warlock_t::create_profile( save_e stype )
{
  std::string profile_str = player_t::create_profile( stype );

  if ( stype == SAVE_ALL )
  {
    if ( initial_soul_shards != 3 )    profile_str += "soul_shards=" + util::to_string( initial_soul_shards ) + "\n";
    if ( ! default_pet.empty() )       profile_str += "default_pet=" + default_pet + "\n";
  }

  return profile_str;
}

void warlock_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  warlock_t* p = debug_cast<warlock_t*>( source );

  initial_soul_shards = p -> initial_soul_shards;
  default_pet = p -> default_pet;
}

// warlock_t::convert_hybrid_stat ==============================================

stat_e warlock_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
    // This is all a guess at how the hybrid primaries will work, since they
    // don't actually appear on cloth gear yet. TODO: confirm behavior
  case STAT_STR_AGI_INT:
  case STAT_AGI_INT:
  case STAT_STR_INT:
    return STAT_INTELLECT;
  case STAT_STR_AGI:
    return STAT_NONE;
  case STAT_SPIRIT:
    return STAT_NONE;
  case STAT_BONUS_ARMOR:
    return STAT_NONE;
  default: return s;
  }
}

expr_t* warlock_t::create_expression( action_t* a, const std::string& name_str )
{
  if ( name_str == "shard_react" )
  {
    struct shard_react_expr_t: public expr_t
    {
      warlock_t& player;
      shard_react_expr_t( warlock_t& p ):
        expr_t( "shard_react" ), player( p ) { }
      virtual double evaluate() override { return player.resources.current[RESOURCE_SOUL_SHARD] >= 1 && player.sim -> current_time() >= player.shard_react; }
    };
    return new shard_react_expr_t( *this );
  }
  else if ( name_str == "felstorm_is_ticking" )
  {
    struct felstorm_is_ticking_expr_t: public expr_t
    {
      pets::warlock_pet_t* felguard;
      felstorm_is_ticking_expr_t( pets::warlock_pet_t* f ):
        expr_t( "felstorm_is_ticking" ), felguard( f ) { }
      virtual double evaluate() override { return ( felguard ) ? felguard -> special_action -> get_dot() -> is_ticking() : false; }
    };
    return new felstorm_is_ticking_expr_t( debug_cast<pets::warlock_pet_t*>( find_pet( "felguard" ) ) );
  }

  else if( name_str == "wild_imp_count")
  {
    struct wild_imp_count_expr_t: public expr_t
    {
        warlock_t& player;

        wild_imp_count_expr_t( warlock_t& p ):
          expr_t( "wild_imp_count" ), player( p ) { }
        virtual double evaluate() override
        {
            double t = 0;
            for(auto& pet : player.warlock_pet_list.wild_imps)
            {
                if(!pet->is_sleeping())
                    t++;
            }
            return t;
        }

    };
    return new wild_imp_count_expr_t( *this );
  }

  else if ( name_str == "darkglare_count" )
  {
    struct darkglare_count_expr_t : public expr_t
    {
      warlock_t& player;

      darkglare_count_expr_t( warlock_t& p ) :
        expr_t( "darkglare_count" ), player( p ) { }
      virtual double evaluate() override
      {
        double t = 0;
        for ( auto& pet : player.warlock_pet_list.darkglare )
        {
          if ( !pet -> is_sleeping() )
            t++;
        }
        return t;
      }

    };
    return new darkglare_count_expr_t( *this );
  }

  else if( name_str == "dreadstalker_count")
  {
      struct dreadstalker_count_expr_t: public expr_t
      {
          warlock_t& player;

          dreadstalker_count_expr_t( warlock_t& p ):
            expr_t( "dreadstalker_count" ), player( p ) { }
          virtual double evaluate() override
          {
              double t = 0;
              for(auto& pet : player.warlock_pet_list.dreadstalkers)
              {
                  if(!pet->is_sleeping())
                      t++;
              }
              return t;
          }

      };
      return new dreadstalker_count_expr_t( *this );
  }
  else if( name_str == "doomguard_count")
  {
      struct doomguard_count_expr_t: public expr_t
      {
          warlock_t& player;

          doomguard_count_expr_t( warlock_t& p ):
            expr_t( "doomguard_count" ), player( p ) { }
          virtual double evaluate() override
          {
              double t = 0;
              for(auto& pet : player.warlock_pet_list.doomguard)
              {
                  if(!pet->is_sleeping())
                      t++;
              }
              return t;
          }

      };
      return new doomguard_count_expr_t( *this );
  }
  else if( name_str == "infernal_count" )
  {
      struct infernal_count_expr_t: public expr_t
      {
          warlock_t& player;

          infernal_count_expr_t( warlock_t& p ):
            expr_t( "infernal_count" ), player( p ) { }
          virtual double evaluate() override
          {
              double t = 0;
              for(auto& pet : player.warlock_pet_list.infernal)
              {
                  if(!pet->is_sleeping())
                      t++;
              }
              return t;
          }

      };
      return new infernal_count_expr_t( *this );
  }
  else if( name_str == "service_count" )
  {
      struct service_count_expr_t: public expr_t
      {
          warlock_t& player;

          service_count_expr_t( warlock_t& p ):
            expr_t( "service_count" ), player( p ) { }
          virtual double evaluate() override
          {
              double t = 0;
              for(auto& pet : player.pet_list)
              {
                  pets::warlock_pet_t *lock_pet = static_cast<pets::warlock_pet_t*> ( pet );
                  if(lock_pet != NULL)
                  {
                      if(lock_pet->is_grimoire_of_service)
                      {
                          if( !lock_pet->is_sleeping() )
                          {
                              t++;
                          }
                      }
                  }
              }
              return t;
          }

      };
      return new service_count_expr_t( *this );
  }

  else if( name_str == "wild_imp_no_de" )
  {
      struct wild_imp_without_de_expr_t: public expr_t
      {
          warlock_t& player;

          wild_imp_without_de_expr_t( warlock_t& p ):
            expr_t( "wild_imp_no_de" ), player( p ) { }
          virtual double evaluate() override
          {
              double t = 0;
              for(auto& pet : player.warlock_pet_list.wild_imps)
              {
                  if(!pet->is_sleeping() & !pet->buffs.demonic_empowerment->up())
                      t++;
              }
              return t;
          }

      };
      return new wild_imp_without_de_expr_t( *this );
  }

  else if ( name_str == "darkglare_no_de" )
  {
    struct darkglare_without_de_expr_t : public expr_t
    {
      warlock_t& player;

      darkglare_without_de_expr_t( warlock_t& p ) :
        expr_t( "darkglare_no_de" ), player( p ) { }
      virtual double evaluate() override
      {
        double t = 0;
        for ( auto& pet : player.warlock_pet_list.darkglare )
        {
          if ( !pet -> is_sleeping() & !pet -> buffs.demonic_empowerment -> up() )
            t++;
        }
        return t;
      }

    };
    return new darkglare_without_de_expr_t( *this );
  }

  else if( name_str == "dreadstalker_no_de" )
  {
      struct dreadstalker_without_de_expr_t: public expr_t
      {
          warlock_t& player;

          dreadstalker_without_de_expr_t( warlock_t& p ):
            expr_t( "dreadstalker_no_de" ), player( p ) { }
          virtual double evaluate() override
          {
              double t = 0;
              for(auto& pet : player.warlock_pet_list.dreadstalkers)
              {
                  if(!pet->is_sleeping() & !pet->buffs.demonic_empowerment->up())
                      t++;
              }
              return t;
          }

      };
      return new dreadstalker_without_de_expr_t( *this );
  }
  else if( name_str == "doomguard_no_de" )
  {
      struct doomguard_without_de_expr_t: public expr_t
      {
          warlock_t& player;

          doomguard_without_de_expr_t( warlock_t& p ):
            expr_t( "doomguard_no_de" ), player( p ) { }
          virtual double evaluate() override
          {
              double t = 0;
              for(auto& pet : player.warlock_pet_list.doomguard)
              {
                  if(!pet->is_sleeping() & !pet->buffs.demonic_empowerment->up())
                      t++;
              }
              return t;
          }

      };
      return new doomguard_without_de_expr_t( *this );
  }
  else if( name_str == "infernal_no_de" )
  {
      struct infernal_without_de_expr_t: public expr_t
      {
          warlock_t& player;

          infernal_without_de_expr_t( warlock_t& p ):
            expr_t( "infernal_no_de" ), player( p ) { }
          virtual double evaluate() override
          {
              double t = 0;
              for(auto& pet : player.warlock_pet_list.infernal)
              {
                  if(!pet->is_sleeping() & !pet->buffs.demonic_empowerment->up())
                      t++;
              }
              return t;
          }

      };
      return new infernal_without_de_expr_t( *this );
  }
  else if( name_str == "service_no_de" )
  {
      struct service_without_de_expr_t: public expr_t
      {
          warlock_t& player;

          service_without_de_expr_t( warlock_t& p ):
            expr_t( "service_no_de" ), player( p ) { }
          virtual double evaluate() override
          {
              double t = 0;
              for(auto& pet : player.pet_list)
              {
                  pets::warlock_pet_t *lock_pet = static_cast<pets::warlock_pet_t*> ( pet );
                  if(lock_pet != NULL)
                  {
                      if(lock_pet->is_grimoire_of_service)
                      {
                          if(!lock_pet->is_sleeping() & !lock_pet->buffs.demonic_empowerment->up())
                              t++;
                      }
                  }
              }
              return t;
          }

      };
      return new service_without_de_expr_t( *this );
  }

  else if( name_str == "wild_imp_de_duration" )
  {
      struct wild_imp_de_duration_expression_t: public expr_t
      {
          warlock_t& player;

          wild_imp_de_duration_expression_t( warlock_t& p ):
            expr_t( "wild_imp_de_duration" ), player( p ) { }
          virtual double evaluate() override
          {
              double t = 150000;
              for(auto& pet : player.warlock_pet_list.wild_imps)
              {
                  if(!pet->is_sleeping() & !pet->buffs.demonic_empowerment->up())
                  {
                    if(pet->buffs.demonic_empowerment->buff_duration.total_seconds() < t)
                        t = pet->buffs.demonic_empowerment->buff_duration.total_seconds();
                  }
              }
              return t;
          }

      };
      return new wild_imp_de_duration_expression_t( *this );
  }

  else if ( name_str == "darkglare_de_duration" )
  {
    struct darkglare_de_duration_expression_t : public expr_t
    {
      warlock_t& player;

      darkglare_de_duration_expression_t( warlock_t& p ) :
        expr_t( "darkglare_de_duration" ), player( p ) { }
      virtual double evaluate() override
      {
        double t = 150000;
        for ( auto& pet : player.warlock_pet_list.darkglare )
        {
          if ( !pet -> is_sleeping() & !pet -> buffs.demonic_empowerment -> up() )
          {
            if ( pet -> buffs.demonic_empowerment -> buff_duration.total_seconds() < t )
              t = pet -> buffs.demonic_empowerment -> buff_duration.total_seconds();
          }
        }
        return t;
      }

    };
    return new darkglare_de_duration_expression_t( *this );
  }

  else if( name_str == "dreadstalkers_de_duration" )
  {
      struct dreadstalkers_de_duration_expression_t: public expr_t
      {
          warlock_t& player;

          dreadstalkers_de_duration_expression_t( warlock_t& p ):
            expr_t( "dreadstalkers_de_duration" ), player( p ) { }
          virtual double evaluate() override
          {
              double t = 150000;
              for(auto& pet : player.warlock_pet_list.dreadstalkers)
              {
                  if(!pet->is_sleeping() & !pet->buffs.demonic_empowerment->up())
                  {
                    if(pet->buffs.demonic_empowerment->buff_duration.total_seconds() < t)
                        t = pet->buffs.demonic_empowerment->buff_duration.total_seconds();
                  }
              }
              return t;
          }

      };
      return new dreadstalkers_de_duration_expression_t( *this );
  }
  else if( name_str == "infernal_de_duration" )
      {
          struct infernal_de_duration_expression_t: public expr_t
          {
              warlock_t& player;

              infernal_de_duration_expression_t( warlock_t& p ):
                expr_t( "infernal_de_duration" ), player( p ) { }
              virtual double evaluate() override
              {
                  double t = 150000;
                  for(auto& pet : player.warlock_pet_list.infernal)
                  {
                      if(!pet->is_sleeping() & !pet->buffs.demonic_empowerment->up())
                      {
                        if(pet->buffs.demonic_empowerment->buff_duration.total_seconds() < t)
                            t = pet->buffs.demonic_empowerment->buff_duration.total_seconds();
                      }
                  }
                  return t;
              }

          };
          return new infernal_de_duration_expression_t( *this );
      }
  else if( name_str == "doomguard_de_duration" )
      {
          struct doomguard_de_duration_expression_t: public expr_t
          {
              warlock_t& player;

              doomguard_de_duration_expression_t( warlock_t& p ):
                expr_t( "doomguard_de_duration" ), player( p ) { }
              virtual double evaluate() override
              {
                  double t = 150000;
                  for(auto& pet : player.warlock_pet_list.doomguard)
                  {
                      if(!pet->is_sleeping() & !pet->buffs.demonic_empowerment->up())
                      {
                        if(pet->buffs.demonic_empowerment->buff_duration.total_seconds() < t)
                            t = pet->buffs.demonic_empowerment->buff_duration.total_seconds();
                      }
                  }
                  return t;
              }

          };
          return new doomguard_de_duration_expression_t( *this );
      }
  else if( name_str == "service_de_duration" )
  {
      struct service_de_duration: public expr_t
      {
          warlock_t& player;

          service_de_duration( warlock_t& p ):
            expr_t( "service_de_duration" ), player( p ) { }
          virtual double evaluate() override
          {
              double t = 500000;
              for(auto& pet : player.pet_list)
              {
                  pets::warlock_pet_t *lock_pet = static_cast<pets::warlock_pet_t*> ( pet );
                  if(lock_pet != NULL)
                  {
                      if(lock_pet->is_grimoire_of_service)
                      {
                          if(!lock_pet->is_sleeping() & !lock_pet->buffs.demonic_empowerment->up())
                          {
                              if(lock_pet->buffs.demonic_empowerment->buff_duration.total_seconds() )
                                  t = lock_pet->buffs.demonic_empowerment->buff_duration.total_seconds();
                          }
                      }
                  }
              }
              return t;
          }

      };
      return new service_de_duration( *this );
  }

  else if( name_str == "wild_imp_remaining_duration" )
  {
    struct wild_imp_remaining_duration_expression_t: public expr_t
    {
      warlock_t& player;

      wild_imp_remaining_duration_expression_t( warlock_t& p ):
        expr_t( "wild_imp_remaining_duration" ), player( p ) { }
      virtual double evaluate() override
      {
        double t = 5000;
        for( auto& pet : player.warlock_pet_list.wild_imps )
        {
          if( !pet -> is_sleeping() )
          {
            if( t > pet -> expiration->remains().total_seconds() )
            {
              t = pet->expiration->remains().total_seconds();
            }
          }
        }
        if( t == 5000 )
          t = -1;
          return t;
      }
    };

    return new wild_imp_remaining_duration_expression_t( *this );
  }

  else if ( name_str == "darkglare_remaining_duration" )
  {
    struct darkglare_remaining_duration_expression_t : public expr_t
    {
      warlock_t& player;

      darkglare_remaining_duration_expression_t( warlock_t& p ) :
        expr_t( "darkglare_remaining_duration" ), player( p ) { }
      virtual double evaluate() override
      {
        double t = 5000;
        for ( auto& pet : player.warlock_pet_list.darkglare )
        {
          if ( !pet -> is_sleeping() )
          {
            if ( t > pet -> expiration -> remains().total_seconds() )
            {
              t = pet -> expiration -> remains().total_seconds();
            }
          }
        }
        if ( t == 5000 )
          t = -1;
        return t;
      }
    };

    return new darkglare_remaining_duration_expression_t( *this );
  }

  else if( name_str == "dreadstalker_remaining_duration" )
      {
          struct dreadstalker_remaining_duration_expression_t: public expr_t
          {
              warlock_t& player;

              dreadstalker_remaining_duration_expression_t( warlock_t& p ):
                expr_t( "dreadstalker_remaining_duration" ), player( p ) { }
              virtual double evaluate() override
              {
                  double t = 5000;
                  for(auto& pet : player.warlock_pet_list.dreadstalkers)
                  {
                      if( !pet->is_sleeping() )
                      {
                          if( t > pet->expiration->remains().total_seconds() )
                          t = pet->expiration->remains().total_seconds();
                      }
                  }
                  if( t==5000 )
                      t = -1;
                  return t;
              }

          };
          return new dreadstalker_remaining_duration_expression_t( *this );
      }
  else if( name_str == "infernal_remaining_duration" )
      {
          struct infernal_remaining_duration_expression_t: public expr_t
          {
              warlock_t& player;

              infernal_remaining_duration_expression_t( warlock_t& p ):
                expr_t( "infernal_remaining_duration" ), player( p ) { }
              virtual double evaluate() override
              {
                  double t = -1;
                  for(auto& pet : player.warlock_pet_list.infernal)
                  {
                      if(!pet->is_sleeping() )
                      {
                          t = pet->expiration->remains().total_seconds();
                      }
                  }
                  return t;
              }

          };
          return new infernal_remaining_duration_expression_t( *this );
      }
  else if( name_str == "doomguard_remaining_duration" )
      {
          struct doomguard_remaining_duration_expression_t: public expr_t
          {
              warlock_t& player;

              doomguard_remaining_duration_expression_t( warlock_t& p ):
                expr_t( "doomguard_remaining_duration" ), player( p ) { }
              virtual double evaluate() override
              {
                  double t = -1;
                  for(auto& pet : player.warlock_pet_list.doomguard)
                  {
                      if(!pet->is_sleeping() )
                      {
                          t = pet->expiration->remains().total_seconds();
                      }
                  }
                  return t;
              }

          };
          return new doomguard_remaining_duration_expression_t( *this );
      }
  else if( name_str == "service_remaining_duration" )
  {
      struct service_remaining_duration_expr_t: public expr_t
      {
          warlock_t& player;

          service_remaining_duration_expr_t( warlock_t& p ):
            expr_t( "service_remaining_duration" ), player( p ) { }
          virtual double evaluate() override
          {
              double t = -1;
              for(auto& pet : player.pet_list)
              {
                  pets::warlock_pet_t *lock_pet = static_cast<pets::warlock_pet_t*> ( pet );
                  if(lock_pet != NULL)
                  {
                      if(lock_pet->is_grimoire_of_service)
                      {
                          if(!lock_pet->is_sleeping() )
                          {
                              t=lock_pet->expiration->remains().total_seconds();
                          }
                      }
                  }
              }
              return t;
          }

      };
      return new service_remaining_duration_expr_t( *this );
  }

  else
  {
    return player_t::create_expression( a, name_str );
  }
}

void warlock_t::trigger_lof_infernal()
{
  int infernal_count = artifact.lord_of_flames.data().effectN( 1 ).base_value();
  int j = 0;

  for ( size_t i = 0; i < warlock_pet_list.lord_of_flames_infernal.size(); i++ )
  {
    if ( warlock_pet_list.lord_of_flames_infernal[ i ] -> is_sleeping() )
    {
      warlock_pet_list.lord_of_flames_infernal[ i ] -> trigger();
      if ( ++j == infernal_count ) break;
    }
  }
}

void warlock_t::trigger_effigy(pets::soul_effigy_t *effigy)
{
    this->active.effigy_damage_override->setupEffigyStuff(effigy->damage);
    this->active.effigy_damage_override->execute();
//    qDebug() << "Testing that we got here";
//    qDebug() << "Test #2";
}


/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class warlock_report_t: public player_report_extension_t
{
public:
  warlock_report_t( warlock_t& player ):
    p( player )
  {

  }

  virtual void html_customsection( report::sc_html_stream& /* os*/ ) override
  {
    (void)p;
    /*// Custom Class Section
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n"
    << "\t\t\t\t\t<h3 class=\"toggle open\">Custom Section</h3>\n"
    << "\t\t\t\t\t<div class=\"toggle-content\">\n";

    os << p.name();

    os << "\t\t\t\t\t\t</div>\n" << "\t\t\t\t\t</div>\n";*/
  }
private:
  warlock_t& p;
};

// WARLOCK MODULE INTERFACE =================================================

using namespace unique_gear;
using namespace actions;

struct power_cord_of_lethtendris_t : public scoped_actor_callback_t<warlock_t>
{
    power_cord_of_lethtendris_t() : super( WARLOCK_AFFLICTION )
    {}

    void manipulate (warlock_t* p, const special_effect_t& e) override
    {
        p -> legendary.power_cord_of_lethtendris_chance = e.driver() -> effectN( 1 ).percent();
    }
};

struct hood_of_eternal_disdain_t : public scoped_action_callback_t<agony_t>
{
  hood_of_eternal_disdain_t() : super( WARLOCK_AFFLICTION, "agony" )
  {}

  void manipulate( agony_t* a, const special_effect_t& e ) override
  {
    a -> base_tick_time *= 1.0 + e.driver() -> effectN( 2 ).percent();
    a -> dot_duration *= 1.0 + e.driver() -> effectN( 1 ).percent();
  }
};

struct kazzaks_final_curse_t : public scoped_action_callback_t<doom_t>
{
  kazzaks_final_curse_t() : super( WARLOCK_DEMONOLOGY, "doom" )
  {}

  void manipulate( doom_t* a, const special_effect_t& e ) override
  {
    a -> kazzaks_final_curse_multiplier = e.driver() -> effectN( 1 ).percent();
  }
};

struct recurrent_ritual_t : public scoped_action_callback_t<call_dreadstalkers_t>
{
  recurrent_ritual_t() : super( WARLOCK_DEMONOLOGY, "call_dreadstalkers" )
  { }

  void manipulate( call_dreadstalkers_t* a, const special_effect_t& e ) override
  {
    a -> recurrent_ritual = e.driver() -> effectN( 1 ).trigger() -> effectN( 1 ).base_value();
  }
};

struct wilfreds_sigil_of_superior_summoning_t : public scoped_actor_callback_t<warlock_t>
{
  wilfreds_sigil_of_superior_summoning_t() : super( WARLOCK )
  {}

  void manipulate( warlock_t* p, const special_effect_t& e ) override
  {
    p -> legendary.wilfreds_sigil_of_superior_summoning_flag = true;
    p -> legendary.wilfreds_sigil_of_superior_summoning = e.driver() -> effectN( 1 ).time_value();
  }
};

struct sindorei_spite_t : public class_buff_cb_t<warlock_t>
{
  sindorei_spite_t() : super( WARLOCK, "sindorei_spite" )
  {}

  buff_t*& buff_ptr( const special_effect_t& e ) override
  {
    return actor( e ) -> buffs.sindorei_spite;
  }

  buff_creator_t creator( const special_effect_t& e ) const override
  {
    return super::creator( e )
      .spell( e.driver() -> effectN( 1 ).trigger() )
      .default_value( e.driver() -> effectN( 1 ).trigger() -> effectN( 1 ).percent() );
  }
};

struct odr_shawl_of_the_ymirjar_t : public scoped_actor_callback_t<warlock_t>
{
  odr_shawl_of_the_ymirjar_t() : super( WARLOCK_DESTRUCTION )
  { }

  void manipulate( warlock_t* a, const special_effect_t& /* e */ ) override
  {
    a -> legendary.odr_shawl_of_the_ymirjar = true;
  }
};

struct stretens_insanity_t: public scoped_actor_callback_t<warlock_t>
{
  stretens_insanity_t(): super( WARLOCK_AFFLICTION )
  {}

  void manipulate( warlock_t* a, const special_effect_t& /* e */ ) override
  {
    a -> legendary.stretens_insanity = true;
  }
};

struct feretory_of_souls_t : public scoped_actor_callback_t<warlock_t>
{
  feretory_of_souls_t() : super( WARLOCK_DESTRUCTION )
  { }

  void manipulate( warlock_t* a, const special_effect_t& /* e */ ) override
  {
    a -> legendary.feretory_of_souls = true;
  }
};

struct warlock_module_t: public module_t
{
  warlock_module_t(): module_t( WARLOCK ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const override
  {
    auto  p = new warlock_t( sim, name, r );
    p -> report_extension = std::unique_ptr<player_report_extension_t>( new warlock_report_t( *p ) );
    return p;
  }

  virtual void static_init() const override
  {
    // Level 100 Class Trinkets
    unique_gear::register_special_effect( 184922, affliction_trinket);
    unique_gear::register_special_effect( 184923, demonology_trinket);
    unique_gear::register_special_effect( 184924, destruction_trinket);

    // Legendaries
    register_special_effect( 205797, hood_of_eternal_disdain_t() );
    register_special_effect( 214225, kazzaks_final_curse_t() );
    register_special_effect( 205721, recurrent_ritual_t() );
    register_special_effect( 214345, wilfreds_sigil_of_superior_summoning_t() );
    register_special_effect( 208868, sindorei_spite_t(), true );
    register_special_effect( 212172, odr_shawl_of_the_ymirjar_t() );
    register_special_effect( 205702, feretory_of_souls_t() );
    register_special_effect( 208821, stretens_insanity_t() );
    register_special_effect( 205753, power_cord_of_lethtendris_t() );
  }

  virtual void register_hotfixes() const override
  {
    /*
    hotfix::register_effect( "Warlock", "2016-09-23", "Drain Life damage increased by 10%", 271 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.10 )
      .verification_value( 0.35 );

    hotfix::register_effect( "Warlock", "2016-09-23", "Drain Soul damage increased by 10%", 291909 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.10 )
      .verification_value( 0.52 );

    hotfix::register_effect( "Warlock", "2016-09-23", "Corruption damage increased by 10%", 198369 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.1 )
      .verification_value( .3 );

    hotfix::register_effect( "Warlock", "2016-09-23", "Agony damage increased by 5%", 374 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.05 )
      .verification_value( 0.036 );

    hotfix::register_effect( "Warlock", "2016-09-23", "Unstable Affliction damage increased by 15%", 303066 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.15 )
      .verification_value( 0.8 );

    hotfix::register_effect( "Warlock", "2016-09-23", "Seed of Corruption damage increased by 15%", 16922 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.15 )
      .verification_value( 1.2 );

    hotfix::register_effect( "Warlock", "2016-09-23", "Siphon Life damage increased by 10%", 57197 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.1 )
      .verification_value( 0.5 );

    hotfix::register_effect( "Warlock", "2016-09-23", "Haunt damage increased by 15%", 40331 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.15 )
      .verification_value( 7.0 );

    hotfix::register_effect( "Warlock", "2016-09-23", "Phantom Singularity damage increased by 15%", 303063 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.15 )
      .verification_value( 1.44 );

    hotfix::register_effect( "Warlock", "2015-09-23", "Hand of Gul�dan impact damage increased by 20%", 87492 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.2 )
      .verification_value( 0.36 );

    hotfix::register_effect( "Warlock", "2015-09-23", " Demonwrath damage increased by 15%", 283783 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.15 )
      .verification_value( 0.3 );

    hotfix::register_effect( "Warlock", "2015-09-23", "Shadowbolt damage increased by 10%", 267 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.1 )
      .verification_value( 0.8 );

    hotfix::register_effect( "Warlock", "2015-09-23", "Doom damage increased by 10%", 246 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.1 )
      .verification_value( 5.0 );

    hotfix::register_effect( "Warlock", "2015-09-23", "Wild Imps damage increased by 10%", 113740 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.1 )
      .verification_value( 0.14 );

    hotfix::register_effect( "Warlock", "2015-09-23", "Demonbolt (Talent) damage increased by 10%", 219885 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.1 )
      .verification_value( 0.8 );

    hotfix::register_effect( "Warlock", "2015-09-23", "Implosion (Talent) damage increased by 15%", 288085 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.15 )
      .verification_value( 2.0 );

    hotfix::register_effect( "Warlock", "2015-09-23", "Shadowflame (Talent) damage increased by 10%", 302909 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.1 )
      .verification_value( 1.0 );

    hotfix::register_effect( "Warlock", "2015-09-23", "Shadowflame (Talent) damage increased by 10%-2", 302911 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.1 )
      .verification_value( 0.35 );

    hotfix::register_effect( "Warlock", "2015-09-23", "Darkglare (Talent) damage increased by 10%", 302984 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.1 )
      .verification_value( 1.0 );

    hotfix::register_effect( "Warlock", "2016-09-23", "Chaos bolt damage increased by 11%", 132079 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.11 )
      .verification_value( 3.3 );

    hotfix::register_effect( "Warlock", "2016-09-23", "Incinerate damage increased by 11%", 288276 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.11 )
      .verification_value( 2.1 );

    hotfix::register_effect( "Warlock", "2016-09-23", "Immolate damage increased by 11%", 145 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.11 )
      .verification_value( 1.2 );

    hotfix::register_effect( "Warlock", "2016-09-23", "Conflagrate damage increased by 11%", 9553 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.11 )
      .verification_value( 2.041 );

    hotfix::register_effect( "Warlock", "2016-09-23", "Rain of Fire damage increased by 11%, and cast time removed", 33883 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.11 )
      .verification_value( 0.5 );

    hotfix::register_effect( "Warlock", "2016-09-23", "Cataclysm damage increased by 11%", 210584 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.11 )
      .verification_value( 7.0 );

    hotfix::register_effect( "Warlock", "2016-09-23", "Channel Demonfire damage increased by 11%", 288343 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.11 )
      .verification_value( 0.42 );
      */
  }

  virtual bool valid() const override { return true; }
  virtual void init( player_t* ) const override {}
  virtual void combat_begin( sim_t* ) const override {}
  virtual void combat_end( sim_t* ) const override {}
};

} // end unnamed namespace

const module_t* module_t::warlock()
{
  static warlock_module_t m;
  return &m;
}
