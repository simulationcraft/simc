#pragma once
#include "simulationcraft.hpp"

#include "player/pet_spawner.hpp"
#include "sc_warlock_pets.hpp"

namespace warlock
{
struct warlock_t;


template <typename Action, typename Actor, typename... Args>
action_t* get_action( util::string_view name, Actor* actor, Args&&... args )
{
  action_t* a = actor->find_action( name );
  if ( !a )
    a = new Action( name, actor, std::forward<Args>( args )... );
  assert( dynamic_cast<Action*>( a ) && a->name_str == name && a->background );
  return a;
}


struct warlock_td_t : public actor_target_data_t
{
  //TODO: SL Beta - Should Leyshocks triggers be removed from the modules?

  propagate_const<dot_t*> dots_drain_life;
  propagate_const<dot_t*> dots_scouring_tithe;

  // Aff
  propagate_const<dot_t*> dots_agony;
  propagate_const<dot_t*> dots_corruption; //TODO: SL Beta - For bookkeeping purposes this is technically available to all specs
  propagate_const<dot_t*> dots_seed_of_corruption;
  propagate_const<dot_t*> dots_drain_soul;
  propagate_const<dot_t*> dots_siphon_life;
  propagate_const<dot_t*> dots_phantom_singularity;
  propagate_const<dot_t*> dots_unstable_affliction;
  propagate_const<dot_t*> dots_vile_taint;

  propagate_const<buff_t*> debuffs_haunt;
  propagate_const<buff_t*> debuffs_shadow_embrace;

  // Destro
  propagate_const<dot_t*> dots_immolate;
  propagate_const<dot_t*> dots_channel_demonfire; //TODO: SL Beta - is a CDF target data needed?
  propagate_const<dot_t*> dots_roaring_blaze; //TODO: SL Beta - may only need debuffs_roaring_blaze now

  propagate_const<buff_t*> debuffs_shadowburn;
  propagate_const<buff_t*> debuffs_eradication;
  propagate_const<buff_t*> debuffs_roaring_blaze; 
  propagate_const<buff_t*> debuffs_havoc;

  // SL - Legendary
  propagate_const<buff_t*> debuffs_odr;

  // Demo
  propagate_const<dot_t*> dots_doom;
  propagate_const<dot_t*> dots_umbral_blaze;  // BFA - Azerite

  propagate_const<buff_t*> debuffs_from_the_shadows;
  propagate_const<buff_t*> debuffs_jaws_of_shadow;  // BFA - Azerite

  double soc_threshold; //Aff - Seed of Corruption counts damage from cross-spec spells such as Drain Life

  warlock_t& warlock;
  warlock_td_t( player_t* target, warlock_t& p );

  void reset()
  {
    soc_threshold = 0;
  }

  void target_demise();
};

struct warlock_t : public player_t
{
public:
  player_t* havoc_target;
  player_t* ua_target; //Used for handling Unstable Affliction target swaps
  std::vector<action_t*> havoc_spells;  // Used for smarter target cache invalidation.
  bool wracking_brilliance;             // BFA - Azerite
  double agony_accumulator;
  double corruption_accumulator;
  double memory_of_lucid_dreams_accumulator;  // BFA - Essences
  double strive_for_perfection_multiplier;    // BFA - Essences
  double vision_of_perfection_multiplier;     // BFA - Essences
  std::vector<event_t*> wild_imp_spawns;      // Used for tracking incoming imps from HoG
  double flashpoint_threshold;                // BFA - Azerite

  unsigned active_pets;

  //TODO: SL Beta - Are the pet definitions here only for temporary pets? If so, should Grimoire: Felguard be included?
  // Active Pet
  struct pets_t
  {
    pets::warlock_pet_t* active;
    pets::warlock_pet_t* last;
    static const int INFERNAL_LIMIT  = 1;
    static const int DARKGLARE_LIMIT = 1;

    //TODO: SL Beta - Refactor infernal code including new talent Rain of Chaos, potentially reuse VoP Infernals for this?
    std::array<pets::destruction::infernal_t*, INFERNAL_LIMIT> infernals;
    spawner::pet_spawner_t<pets::destruction::infernal_t, warlock_t>
        vop_infernals;  // Infernal(s) summoned by Vision of Perfection

    //TODO: SL Beta - Vision of Perfection spawns should be removed once SL launches
    std::array<pets::affliction::darkglare_t*, DARKGLARE_LIMIT> darkglare;
    spawner::pet_spawner_t<pets::affliction::darkglare_t, warlock_t>
        vop_darkglares;  // Darkglare(s) summoned by Vision of Perfection

    spawner::pet_spawner_t<pets::demonology::dreadstalker_t, warlock_t> dreadstalkers;
    spawner::pet_spawner_t<pets::demonology::vilefiend_t, warlock_t> vilefiends;
    spawner::pet_spawner_t<pets::demonology::demonic_tyrant_t, warlock_t> demonic_tyrants;

    spawner::pet_spawner_t<pets::demonology::wild_imp_pet_t, warlock_t> wild_imps;

    //TODO: SL Beta - confirm Nether Portal demons still same? (minor, only for sanity checking)
    spawner::pet_spawner_t<pets::demonology::random_demons::shivarra_t, warlock_t> shivarra;
    spawner::pet_spawner_t<pets::demonology::random_demons::darkhound_t, warlock_t> darkhounds;
    spawner::pet_spawner_t<pets::demonology::random_demons::bilescourge_t, warlock_t> bilescourges;
    spawner::pet_spawner_t<pets::demonology::random_demons::urzul_t, warlock_t> urzuls;
    spawner::pet_spawner_t<pets::demonology::random_demons::void_terror_t, warlock_t> void_terrors;
    spawner::pet_spawner_t<pets::demonology::random_demons::wrathguard_t, warlock_t> wrathguards;
    spawner::pet_spawner_t<pets::demonology::random_demons::vicious_hellhound_t, warlock_t> vicious_hellhounds;
    spawner::pet_spawner_t<pets::demonology::random_demons::illidari_satyr_t, warlock_t> illidari_satyrs;
    spawner::pet_spawner_t<pets::demonology::random_demons::eyes_of_guldan_t, warlock_t> eyes_of_guldan;
    spawner::pet_spawner_t<pets::demonology::random_demons::prince_malchezaar_t, warlock_t> prince_malchezaar;

    pets_t( warlock_t* w );
  } warlock_pet_list;

  std::vector<std::string> pet_name_list;

  //TODO: SL Beta - what is this struct for/should it be renamed for clarity?
  struct active_t
  {
    action_t* grimoire_of_sacrifice_proc; //TODO: SL Beta - Should Grimoire of Sacrifice be refactored? Regardless, figure out better placement of this
    spell_t* pandemic_invocation;  // BFA - Azerite
    spell_t* corruption; //TODO: SL Beta - This is currently unused, was this meant to be the definition for the primary active ability? Fix this!
    spell_t* roaring_blaze; //TODO: SL Beta - This is currently unused, is there any need to define it or is debuffs_roaring_blaze sufficient?
    spell_t* internal_combustion; //TODO: SL Beta - This is currently unused, is there any need to define it or is talents.internal_combustion sufficient?
    spell_t* rain_of_fire; //TODO: SL Beta - This is the definition for the ground aoe event, should we move this?
    spell_t* bilescourge_bombers; //TODO: SL Beta - This is the definition for the ground aoe event, should we move this?
    spell_t* summon_random_demon; //TODO: SL Beta - This is the definition for a helper action for Nether Portal, should we move this?
    melee_attack_t* soul_strike; //TODO: SL Beta - This is currently unused, Felguard pets are using manual spellID. Fix or remove.
  } active;

  //TODO: SL Beta - Check all labels here since there was a level squish (EXTREMELY minor, just for clarity)
  //TODO: SL Beta - Do we need definitions for non-DPS related rows?
  // Talents
  struct talents_t
  {
    // shared
    // tier 30
    const spell_data_t* demon_skin;
    const spell_data_t* burning_rush;
    const spell_data_t* dark_pact;

    // tier 40
    const spell_data_t* darkfury;
    const spell_data_t* mortal_coil;
    const spell_data_t* howl_of_terror;

    // tier 45 (Aff/Destro)
    const spell_data_t* grimoire_of_sacrifice;

    // tier 50
    const spell_data_t* soul_conduit; //(t45 for demo)

    // AFF
    // tier 15
    const spell_data_t* nightfall; //TODO: SL Beta - RNG information is missing from spell data, and data also says buff can potentially stack to 2. Serious testing needed, especially with multiple corruptions out!
    const spell_data_t* inevitable_demise;
    const spell_data_t* drain_soul;

    // tier 25
    const spell_data_t* writhe_in_agony; //Note: there is an unimplemented bug for SL prepatch
    const spell_data_t* absolute_corruption;
    const spell_data_t* siphon_life;

    // tier 35
    const spell_data_t* sow_the_seeds; //TODO: SL Beta - aoe value seems to be hardcoded and should be pulled from spell data instead
    const spell_data_t* phantom_singularity; //TODO: SL Beta - dot duration seems to be hardcoded and should be pulled from spell data instead
    const spell_data_t* vile_taint;

    // tier 45
    const spell_data_t* dark_caller;
    const spell_data_t* haunt;
    // grimoire of sacrifice

    // tier 50
    // soul conduit
    const spell_data_t* creeping_death;
    const spell_data_t* dark_soul_misery;

    // DEMO
    // tier 15
    const spell_data_t* dreadlash;
    const spell_data_t* bilescourge_bombers;
    const spell_data_t* demonic_strength; //TODO: SL Beta - pet buff for Demonic Strength is currently not using this, should that be fixed?

    // tier 25
    const spell_data_t* demonic_calling;
    const spell_data_t* power_siphon;
    const spell_data_t* doom; //TODO: SL Beta - Doom is now working in the sim and seems to match some tests against beta client, but haste/refresh behavior needs checking still

    // tier 35
    const spell_data_t* from_the_shadows;
    const spell_data_t* soul_strike;  //TODO: SL Beta - double check automagic is handling damage correctly
    const spell_data_t* summon_vilefiend;

    // tier 45
    // soul conduit
    const spell_data_t* inner_demons; //TODO: SL Beta - WHY DOES THIS TALENT NEED AN APL LINE TO WORK FIX THIS
    const spell_data_t* grimoire_felguard; //TODO: SL Beta - Check summoning/buff durations and if permanent Felguard is affected by this ability

    // tier 50
    const spell_data_t* sacrificed_souls; //TODO: SL Beta - check what pets/guardians count for this talent. Also relevant for demonic consumption
    const spell_data_t* demonic_consumption; //TODO: SL Beta - CAREFULLY test and audit damage buff behavior for this talent
    const spell_data_t* nether_portal;

    // DESTRO
    // tier 15
    const spell_data_t* flashover;
    const spell_data_t* eradication;
    const spell_data_t* soul_fire; //TODO: SL Beta - confirm refresh/pandemic behavior matchup for immolates between sim and client

    // tier 25
    const spell_data_t* reverse_entropy; //Note: talent spell (not the buff spell) contains RPPM data
    const spell_data_t* internal_combustion;
    const spell_data_t* shadowburn; //TODO: SL Beta - debuffs_shadowburn may be able to use this definition

    // tier 35
    const spell_data_t* inferno; //TODO: SL Beta - confirm interaction between Inferno and Rank 2 Rain of Fire, as well as if soul shard generation is per-target hit
    const spell_data_t* fire_and_brimstone;
    const spell_data_t* cataclysm;

    // tier 45
    const spell_data_t* roaring_blaze; //TODO: SL Beta - confirm interaction between roaring blaze and eradication
    const spell_data_t* rain_of_chaos; //TODO: THIS HAS NOT BEEN IMPLEMENTED. FIX THIS. (Also need to add the rain of chaos buff 266087)
    // grimoire of sacrifice

    // tier 50
    // soul conduit
    const spell_data_t* channel_demonfire; //TODO: SL Beta - confirm aoe behavior on each tick is correct in sims
    const spell_data_t* dark_soul_instability;
  } talents;

  //TODO: SL Beta - Traits and Essences can be removed once SL launches
  // Azerite traits
  struct azerite_t
  {
    // Shared
    azerite_power_t desperate_power;  // healing
    azerite_power_t lifeblood;        // healing

    // Demo
    azerite_power_t demonic_meteor;
    azerite_power_t shadows_bite;
    azerite_power_t supreme_commander;
    azerite_power_t umbral_blaze;
    azerite_power_t explosive_potential;
    azerite_power_t baleful_invocation;

    // Aff
    azerite_power_t cascading_calamity;
    azerite_power_t dreadful_calling;
    azerite_power_t inevitable_demise;
    azerite_power_t sudden_onset;
    azerite_power_t wracking_brilliance;
    azerite_power_t pandemic_invocation;

    // Destro
    azerite_power_t bursting_flare;
    azerite_power_t chaotic_inferno;
    azerite_power_t crashing_chaos;
    azerite_power_t rolling_havoc;
    azerite_power_t flashpoint;
    azerite_power_t chaos_shards;
  } azerite;

  struct
  {
    azerite_essence_t memory_of_lucid_dreams;  // Memory of Lucid Dreams minor
    azerite_essence_t vision_of_perfection;
  } azerite_essence;

  //TODO: SL Beta - Legendary, Conduit, and Covenant implementation is unchecked/incomplete. Replace this TODO with individual TODOs once audited
  struct legendary_t
  {
    // Legendaries
    // Cross-spec
    item_runeforge_t claw_of_endereth;
    item_runeforge_t mark_of_borrowed_power; //TODO: SL Beta - Confirm with long dummy log that the % chances have no BLP
    item_runeforge_t wilfreds_sigil_of_superior_summoning;
    // Affliction
    item_runeforge_t malefic_wrath;
    item_runeforge_t perpetual_agony_of_azjaqir;
    item_runeforge_t sacrolashs_dark_strike; //TODO: SL Beta - Check if slow effect (unimplemented atm) can proc anything important
    item_runeforge_t wrath_of_consumption;
    // Demonology
    item_runeforge_t balespiders_burning_core;
    item_runeforge_t forces_of_the_horned_nightmare;
    item_runeforge_t grim_inquisitors_dread_calling;
    item_runeforge_t implosive_potential;
    // Destruction
    item_runeforge_t cinders_of_the_azjaqir;
    item_runeforge_t embers_of_the_diabolic_raiment;
    item_runeforge_t madness_of_the_azjaqir;
    item_runeforge_t odr_shawl_of_the_ymirjar;
  } legendary;

  struct conduit_t
  {
    // Conduits
    // Covenant Abilities
    conduit_data_t catastrophic_origin;   // Venthyr
    conduit_data_t exhumed_soul;          // Night Fae
    conduit_data_t prolonged_decimation;  // Necrolord
    conduit_data_t soul_tithe;            // Kyrian
    // Affliction
    conduit_data_t cold_embrace;
    conduit_data_t corrupting_leer;
    conduit_data_t focused_malignancy;
    conduit_data_t rolling_agony;
    // Demonology
    conduit_data_t borne_of_blood;
    conduit_data_t carnivorous_stalkers;
    conduit_data_t fel_commando;
    conduit_data_t tyrants_soul;
    // Destruction
    conduit_data_t ashen_remains;
    conduit_data_t combusting_engine;
    conduit_data_t duplicitous_havoc;
    conduit_data_t infernal_brand;
  } conduit;

  struct covenant_t
  {
    // Covenant Abilities
    const spell_data_t* decimating_bolt;        // Necrolord
    const spell_data_t* impending_catastrophe;  // Venthyr
    const spell_data_t* scouring_tithe;         // Kyrian
    const spell_data_t* soul_rot;               // Night Fae
  } covenant;

  // Mastery Spells
  struct mastery_spells_t
  {
    const spell_data_t* potent_afflictions;
    const spell_data_t* master_demonologist;
    const spell_data_t* chaotic_energies;
  } mastery_spells;

  //TODO: SL Beta - Is this necessary or can this be refactored?
  // Procs and RNG
  propagate_const<real_ppm_t*> grimoire_of_sacrifice_rppm;  // grimoire of sacrifice

  //TODO: SL Beta - Cleanup useless ones and add any as needed
  // Cooldowns - Used for accessing cooldowns outside of their respective actions, such as reductions/resets
  struct cooldowns_t
  {
    propagate_const<cooldown_t*> haunt;
    propagate_const<cooldown_t*> call_dreadstalkers; //probably unneeded
    propagate_const<cooldown_t*> phantom_singularity;
    propagate_const<cooldown_t*> darkglare;
    propagate_const<cooldown_t*> demonic_tyrant;
    propagate_const<cooldown_t*> scouring_tithe;
  } cooldowns;

  //TODO: SL Beta - this struct is supposedly for passives per the comment here, but that is potentially outdated. Consider refactoring and reorganizing ALL of this.
  // Passives
  struct specs_t
  {
    // All Specs
    const spell_data_t* fel_armor; //TODO: SL Beta - removed? Or is this now Demonic Embrace?
    const spell_data_t* nethermancy; //TODO: SL Beta - this name doesn't show in spell book but is still in spell data, is this correct?
    //TODO: SL Beta - Corruption is now class-wide
    //TODO: SL Beta - Ritual of Doom?

    // Affliction only
    const spell_data_t* affliction; //Spec aura
    const spell_data_t* agony; //This is the primary active ability
    const spell_data_t* agony_2; //Rank 2 passive (increased stacks)
    const spell_data_t* corruption_2; //Rank 2 passive (instant cast)
    const spell_data_t* corruption_3; //Rank 3 passive (damage on cast component)
    const spell_data_t* nightfall;  // TODO: SL Beta - There is no specialization data for this spell, remove or fix. (Potential duplicate of talents.nightfall)
    const spell_data_t* shadow_bite; //TODO: SL Beta - Pet spell? Does not appear in specialization data
    const spell_data_t* shadow_bolt; //TODO: SL Beta - This is currently unused. Decide on fix or remove.
    const spell_data_t* summon_darkglare; //This is the active summon ability
    const spell_data_t* unstable_affliction;  //This is the primary active ability
    const spell_data_t* unstable_affliction_2; //Rank 2 passive (soul shard on death)
    const spell_data_t* unstable_affliction_3; //Rank 3 passive (increased duration)

    // Demonology only
    const spell_data_t* demonology; //Spec aura
    const spell_data_t* call_dreadstalkers_2; //Rank 2 passive (reduced cast time, increased pet move speed)
    const spell_data_t* demonic_core; //Spec passive for the ability. See also: buffs.demonic_core
    const spell_data_t* doom; //TODO: SL Beta - Currently not being used - potential duplicate of talents.doom
    const spell_data_t* fel_firebolt_2; //Rank 2 passive (reduced energy)
    const spell_data_t* summon_demonic_tyrant_2; //Rank 2 passive (instant soul shard generation)
    const spell_data_t* wild_imps; //TODO: SL Beta - What is this? There doesn't appear to be any spell data matching this name
    //TODO: SL Beta - Should Implosion be in this list? Currently in spells.implosion_aoe

    // Destruction only
    const spell_data_t* destruction; //Spec aura
    const spell_data_t* conflagrate; //TODO: SL Beta - This is the primary active ability, but is not currently being used. Fix this.
    const spell_data_t* conflagrate_2; //Rank 2 passive (increased charges)
    const spell_data_t* firebolt; //TODO: SL Beta - Pet spell? Does not appear in specialization data
    const spell_data_t* havoc; //This is the primary active ability
    //TODO: SL Beta - debuffs_havoc is currently referencing the spellID directly for rank 2, fix or remove. 
    const spell_data_t* havoc_2; //Rank 2 passive (increased duration)
    const spell_data_t* immolate; //TODO: SL Beta - this is supposed to be the primary active ability but is not being used at the moment - fix this
    const spell_data_t* rain_of_fire_2; //Rank 2 passive (increased damage)
    const spell_data_t* summon_infernal_2; //Rank 2 passive (impact damage)
    const spell_data_t* unending_resolve; //TODO: SL Beta - this isn't even a DPS ability, and doesn't appear in specialization data. Probably remove
  } spec;

  // Buffs
  struct buffs_t
  {
    propagate_const<buff_t*> demonic_power; //Buff from Summon Demonic Tyrant (increased demon damage + duration)
    propagate_const<buff_t*> grimoire_of_sacrifice; //Buff which grants damage proc

    // Affliction Buffs
    propagate_const<buff_t*> active_uas; //TODO: SL Beta - Unstable Affliction behavior has changed in Shadowlands, this is probably outdated and can be removed.
    propagate_const<buff_t*> drain_life; //Dummy buff used internally for handling Inevitable Demise cases
    propagate_const<buff_t*> nightfall;
    propagate_const<buff_t*> dark_soul_misery;

    //TODO: SL Beta - Azerite powers that became talents - do we need to consider edge cases during prepatch involving both?
    // BFA - Affliction Azerite
    propagate_const<buff_t*> cascading_calamity;
    propagate_const<buff_t*> inevitable_demise;
    propagate_const<buff_t*> wracking_brilliance;

    // Demonology Buffs
    propagate_const<buff_t*> demonic_core;
    propagate_const<buff_t*> power_siphon; //Hidden buff from Power Siphon that increases damage of successive Demonbolts
    propagate_const<buff_t*> demonic_calling;
    propagate_const<buff_t*> inner_demons;
    propagate_const<buff_t*> nether_portal;
    propagate_const<buff_t*> wild_imps; //Buff for tracking how many Wild Imps are currently out (does NOT include imps waiting to be spawned)
    propagate_const<buff_t*> dreadstalkers; //Buff for tracking number of Dreadstalkers currently out
    propagate_const<buff_t*> vilefiend; //Buff for tracking if Vilefiend is currently out
    propagate_const<buff_t*> tyrant; //Buff for tracking if Demonic Tyrant is currently out
    propagate_const<buff_t*> portal_summons; //TODO: SL Beta - Is this for tracking purposes or just outdated? Appears unused
    propagate_const<buff_t*> grimoire_felguard; //Buff for tracking if GFG pet is currently out
    propagate_const<buff_t*> prince_malchezaar; //Buff for tracking if rare random summon is currently out
    propagate_const<buff_t*> eyes_of_guldan; //Buff for tracking if rare random summon is currently out

    // BFA - Demonology Azerite
    propagate_const<buff_t*> shadows_bite;
    propagate_const<buff_t*> supreme_commander;
    propagate_const<buff_t*> explosive_potential;

    // Destruction Buffs
    propagate_const<buff_t*> backdraft; //Buff associated with Conflagrate
    propagate_const<buff_t*> reverse_entropy;
    propagate_const<buff_t*> grimoire_of_supremacy_driver; //TODO: SL Beta - GSup is removed in Shadowlands so this should be removed.
    propagate_const<buff_t*> grimoire_of_supremacy; //TODO: SL Beta - GSup is removed in Shadowlands so this should be removed.
    propagate_const<buff_t*> dark_soul_instability;

    // BFA - Destruction Azerite
    propagate_const<buff_t*> bursting_flare;
    propagate_const<buff_t*> chaotic_inferno;
    propagate_const<buff_t*> crashing_chaos;
    propagate_const<buff_t*> crashing_chaos_vop;
    propagate_const<buff_t*> rolling_havoc;
    propagate_const<buff_t*> flashpoint;
    propagate_const<buff_t*> chaos_shards;

    // SL
    propagate_const<buff_t*> decimating_bolt;

    // Legendaries
    propagate_const<buff_t*> madness_of_the_azjaqir;
    propagate_const<buff_t*> balespiders_burning_core;
    propagate_const<buff_t*> malefic_wrath;
    propagate_const<buff_t*> wrath_of_consumption;
    propagate_const<buff_t*> implosive_potential;
  } buffs;

  //TODO: SL Beta - Some of these gains are unused, should they be pruned?
  // Gains
  struct gains_t
  {
    gain_t* soul_conduit;
    gain_t* borrowed_power; // SL - Legendary

    gain_t* agony;
    gain_t* drain_soul;
    gain_t* seed_of_corruption;
    gain_t* unstable_affliction_refund;
    gain_t* pandemic_invocation;  // BFA - Azerite

    gain_t* conflagrate;
    gain_t* incinerate;
    gain_t* incinerate_crits;
    gain_t* incinerate_fnb;
    gain_t* incinerate_fnb_crits;
    gain_t* immolate;
    gain_t* immolate_crits;
    gain_t* soul_fire;
    gain_t* infernal;
    gain_t* shadowburn_refund;
    gain_t* inferno;
    gain_t* chaos_shards;  // BFA - Azerite

    gain_t* miss_refund;

    gain_t* shadow_bolt;
    gain_t* doom;
    gain_t* summon_demonic_tyrant;
    gain_t* demonic_meteor;      // BFA - Azerite
    gain_t* baleful_invocation;  // BFA - Azerite

    gain_t* memory_of_lucid_dreams;  // BFA - Essence

    // SL
    gain_t* scouring_tithe;
  } gains;

  // Procs
  struct procs_t
  {
    proc_t* soul_conduit;
    proc_t* mark_of_borrowed_power;

    // aff
    proc_t* nightfall;
    proc_t* corrupting_leer;
    proc_t* malefic_wrath;

    // demo
    proc_t* demonic_calling;
    proc_t* souls_consumed;
    proc_t* one_shard_hog;
    proc_t* two_shard_hog;
    proc_t* three_shard_hog;
    proc_t* wild_imp;
    proc_t* dreadstalker_debug;
    proc_t* summon_random_demon;
    proc_t* portal_summon;
    proc_t* carnivorous_stalkers; // SL - Conduit
    proc_t* horned_nightmare; // SL - Legendary

    // destro
    proc_t* reverse_entropy;
  } procs;

  //TODO: SL Beta - Why does this catchall struct exist but is barely used?
  struct spells_t
  {
    spell_t* melee; //Is this necessary?
    spell_t* seed_of_corruption_aoe; //Is this unused?
    spell_t* malefic_rapture_aoe; //Is this unused?
    spell_t* corruption_impact_effect;
    spell_t* implosion_aoe; //Is this unused?
    const spell_data_t* memory_of_lucid_dreams_base;  // BFA - Essence
  } spells;

  int initial_soul_shards;
  std::string default_pet;
  timespan_t shard_react; //Was this planned to be used for RNG soul shard reaction timing? Currently unused

  warlock_t( sim_t* sim, util::string_view name, race_e r );

  // Character Definition
  void init_spells() override;
  void init_base_stats() override;
  void init_scaling() override;
  void create_buffs() override;
  void init_gains() override;
  void init_procs() override;
  void init_rng() override;
  void init_action_list() override;
  void init_resources( bool force ) override;
  void reset() override;
  void create_options() override;
  int get_spawning_imp_count();
  timespan_t time_to_imps( int count );
  int imps_spawned_during( timespan_t period );
  void trigger_memory_of_lucid_dreams( double gain );
  void vision_of_perfection_proc() override;
  void vision_of_perfection_proc_destro();
  void vision_of_perfection_proc_aff();
  void vision_of_perfection_proc_demo();
  void darkglare_extension_helper( warlock_t* p, timespan_t darkglare_extension );
  void malignancy_reduction_helper();
  action_t* create_action( util::string_view name, const std::string& options ) override;
  pet_t* create_pet( util::string_view name, util::string_view type = "" ) override;
  void create_pets() override;
  std::string create_profile( save_e ) override;
  void copy_from( player_t* source ) override;
  resource_e primary_resource() const override
  {
    return RESOURCE_MANA;
  }
  role_e primary_role() const override
  {
    return ROLE_SPELL;
  }
  stat_e convert_hybrid_stat( stat_e s ) const override;
  double matching_gear_multiplier( attribute_e attr ) const override;
  double composite_player_multiplier( school_e school ) const override;
  double composite_player_target_multiplier( player_t* target, school_e school ) const override;
  double composite_player_pet_damage_multiplier( const action_state_t* ) const override;
  double composite_rating_multiplier( rating_e rating ) const override;
  void invalidate_cache( cache_e ) override;
  double composite_spell_crit_chance() const override;
  double composite_spell_haste() const override;
  double composite_melee_haste() const override;
  double composite_melee_crit_chance() const override;
  double composite_mastery() const override;
  double resource_gain( resource_e, double, gain_t* = nullptr, action_t* = nullptr ) override;
  double resource_regen_per_second( resource_e ) const override;
  double composite_armor() const override;
  void halt() override;
  void combat_begin() override;
  void init_assessors() override;
  std::unique_ptr<expr_t> create_expression( util::string_view name_str ) override;
  std::string default_potion() const override;
  std::string default_flask() const override;
  std::string default_food() const override;
  std::string default_rune() const override;
  void apply_affecting_auras( action_t& action ) override;

  target_specific_t<warlock_td_t> target_data;

  warlock_td_t* get_target_data( player_t* target ) const override
  {
    warlock_td_t*& td = target_data[ target ];
    if ( !td )
    {
      td = new warlock_td_t( target, const_cast<warlock_t&>( *this ) );
    }
    return td;
  }

  // sc_warlock_affliction
  action_t* create_action_affliction( util::string_view action_name, const std::string& options_str );
  void create_buffs_affliction();
  void init_spells_affliction();
  void init_gains_affliction();
  void init_rng_affliction();
  void init_procs_affliction();
  void create_options_affliction();
  void create_apl_affliction();
  std::unique_ptr<expr_t> create_aff_expression( util::string_view name_str );

  // sc_warlock_demonology
  action_t* create_action_demonology( util::string_view action_name, const std::string& options_str );
  void create_buffs_demonology();
  void init_spells_demonology();
  void init_gains_demonology();
  void init_rng_demonology();
  void init_procs_demonology();
  void create_options_demonology();
  void create_apl_demonology();

  // sc_warlock_destruction
  action_t* create_action_destruction( util::string_view action_name, const std::string& options_str );
  void create_buffs_destruction();
  void init_spells_destruction();
  void init_gains_destruction();
  void init_rng_destruction();
  void init_procs_destruction();
  void create_options_destruction();
  void create_apl_destruction();

  // sc_warlock_pets
  pet_t* create_main_pet( util::string_view pet_name, util::string_view pet_type );
  pet_t* create_demo_pet( util::string_view pet_name, util::string_view pet_type );
  void create_all_pets();
  std::unique_ptr<expr_t> create_pet_expression( util::string_view name_str );

private:
  void apl_precombat();
  void apl_default();
  void apl_global_filler();
};

namespace actions
{
//Event for triggering delayed refunds from Soul Conduit
//Delay prevents instant reaction time issues for rng refunds
struct sc_event_t : public player_event_t
{
  gain_t* shard_gain;
  warlock_t* pl;
  int shards_used;

  sc_event_t( warlock_t* p, int c )
    : player_event_t( *p, 100_ms ),
    shard_gain( p->gains.soul_conduit ),
    pl( p ),
    shards_used( c )
  {
  }

  virtual const char* name() const override
  {
    return "soul_conduit_event";
  }

  virtual void execute() override
  {
    double soul_conduit_rng = pl->talents.soul_conduit->effectN( 1 ).percent();

    for ( int i = 0; i < shards_used; i++ )
    {
      if ( rng().roll( soul_conduit_rng ) )
      {
        pl->sim->print_log( "Soul Conduit proc occurred for Warlock {}, refunding 1.0 soul shards.", pl->name() );
        pl->resource_gain( RESOURCE_SOUL_SHARD, 1.0, shard_gain );
        pl->procs.soul_conduit->occur();
      }
    }
  }
};

//Event for triggering refunds from Mark of Borrowed Power legendary
//TOCHECK: Currently, this refund can occur independently of Soul Conduit refunds, granting more shards than originally spent
struct borrowed_power_event_t : public player_event_t
{
  gain_t* shard_gain;
  warlock_t* pl;
  int shards_used;
  double refund_chance;

  borrowed_power_event_t( warlock_t* p, int c, double chance )
    : player_event_t( *p, 100_ms ),
    shard_gain( p->gains.borrowed_power ),
    pl( p ),
    shards_used( c ),
    refund_chance( chance )
  {
  }

  virtual const char* name() const override
  {
    return "borrowed_power_event";
  }

  virtual void execute() override
  {
      if ( rng().roll( refund_chance ) )
      {
        pl->sim->print_log( "Borrowed power proc occurred for Warlock {}, refunding {} soul shards.", pl->name(), shards_used );
        pl->resource_gain( RESOURCE_SOUL_SHARD, shards_used, shard_gain );
        pl->procs.mark_of_borrowed_power->occur();
      } 
  }
};

struct warlock_heal_t : public heal_t
{
  warlock_heal_t( const std::string& n, warlock_t* p, const uint32_t id ) : heal_t( n, p, p->find_spell( id ) )
  {
    target = p;
  }

  warlock_t* p()
  {
    return static_cast<warlock_t*>( player );
  }
  const warlock_t* p() const
  {
    return static_cast<warlock_t*>( player );
  }
};

struct warlock_spell_t : public spell_t
{
public:
  gain_t* gain;
  bool can_havoc; //Needed in main module for cross-spec spells such as Covenants
  bool affected_by_woc; // SL - Legendary (Wrath of Consumption) checker

  warlock_spell_t( warlock_t* p, util::string_view n ) : warlock_spell_t( n, p, p->find_class_spell( n ) )
  {
  }

  warlock_spell_t( warlock_t* p, util::string_view n, specialization_e s )
    : warlock_spell_t( n, p, p->find_class_spell( n, s ) )
  {
  }

  warlock_spell_t( util::string_view token, warlock_t* p, const spell_data_t* s = spell_data_t::nil() )
    : spell_t( token, p, s )
  {
    may_crit          = true;
    tick_may_crit     = true;
    weapon_multiplier = 0.0;
    gain              = player->get_gain( name_str );
    can_havoc         = false;

    //TOCHECK: Is there a way to link this to the buffs.x spell data so we don't have to remember this is hardcoded?
    affected_by_woc   = data().affected_by( p->find_spell( 337130 )->effectN( 1 ) );
  }

  warlock_t* p()
  {
    return static_cast<warlock_t*>( player );
  }
  const warlock_t* p() const
  {
    return static_cast<warlock_t*>( player );
  }

  warlock_td_t* td( player_t* t )
  {
    return p()->get_target_data( t );
  }

  const warlock_td_t* td( player_t* t ) const
  {
    return p()->get_target_data( t );
  }

  void reset() override
  {
    spell_t::reset();
  }

  double cost() const override
  {
    double c = spell_t::cost();
    return c;
  }

  void execute() override
  {
    spell_t::execute();

    if ( hit_any_target && result_is_hit( execute_state->result ) && p()->talents.grimoire_of_sacrifice->ok() &&
         p()->buffs.grimoire_of_sacrifice->up() )
    {
      bool procced = p()->grimoire_of_sacrifice_rppm->trigger();
      if ( procced )
      {
        p()->active.grimoire_of_sacrifice_proc->set_target( execute_state->target );
        p()->active.grimoire_of_sacrifice_proc->execute();
      }
    }
  }

  void consume_resource() override
  {
    spell_t::consume_resource();

    if ( resource_current == RESOURCE_SOUL_SHARD && p()->in_combat )
    {
      p()->trigger_memory_of_lucid_dreams( last_resource_cost );  // BFA - Essence

      // lets try making all lock specs not react instantly to shard gen
      if ( p()->talents.soul_conduit->ok() )
      {
        make_event<sc_event_t>( *p()->sim, p(), as<int>( last_resource_cost ) );
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    spell_t::impact( s );
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = spell_t::composite_target_multiplier( t );
    return m;
  }

  double action_multiplier() const override
  {
    double pm = spell_t::action_multiplier();

    return pm;
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double m = spell_t::composite_ta_multiplier( s );

    if ( p()->legendary.wrath_of_consumption.ok() && p()->buffs.wrath_of_consumption->check() && affected_by_woc )
      m *= 1.0 + p()->buffs.wrath_of_consumption->check_stack_value();

    return m;
  }

  void extend_dot( dot_t* dot, timespan_t extend_duration )
  {
    if ( dot->is_ticking() )
    {
      dot->adjust_duration( extend_duration, dot->current_action->dot_duration * 1.5 );
    }
  }

  //Destruction specific things for Havoc that unfortunately need to be in main module

  bool use_havoc() const
  {
    // Ensure we do not try to hit the same target twice.
    return can_havoc && p()->havoc_target && p()->havoc_target != target;
  }

  int n_targets() const override
  {
    if ( p()->specialization() == WARLOCK_DESTRUCTION && use_havoc() )
    {
      assert(spell_t::n_targets() == 0);
      return 2;
    }
    else
      return spell_t::n_targets();
  }

  size_t available_targets(std::vector<player_t*>& tl) const override
  {
    spell_t::available_targets(tl);

    // Check target list size to prevent some silly scenarios where Havoc target
    // is the only target in the list.
    if ( p()->specialization() == WARLOCK_DESTRUCTION && tl.size() > 1 && use_havoc())
    {
      // We need to make sure that the Havoc target ends up second in the target list,
      // so that Havoc spells can pick it up correctly.
      auto it = range::find(tl, p()->havoc_target);
      if (it != tl.end())
      {
        tl.erase(it);
        tl.insert(tl.begin() + 1, p()->havoc_target);
      }
    }

    return tl.size();
  }

  void init() override
  {
    spell_t::init();

    if ( p()->specialization() == WARLOCK_DESTRUCTION && can_havoc )
    {
        // SL - Conduit
        base_aoe_multiplier *= p()->spec.havoc->effectN(1).percent() + p()->conduit.duplicitous_havoc.percent();
        p()->havoc_spells.push_back(this);
    }
  }

  //End Destruction specific things

  std::unique_ptr<expr_t> create_expression( util::string_view name_str ) override
  {
    return spell_t::create_expression( name_str );
  }
};

using residual_action_t = residual_action::residual_periodic_action_t<warlock_spell_t>;

struct summon_pet_t : public warlock_spell_t
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
         std::find( p()->pet_name_list.begin(), p()->pet_name_list.end(), pet_name ) == p()->pet_name_list.end() )
    {
      p()->pet_name_list.push_back( pet_name );
    }
  }

public:
  summon_pet_t( const std::string& n, warlock_t* p, const std::string& sname = "" )
    : warlock_spell_t( p, sname.empty() ? "Summon " + n : sname ),
      summoning_duration( timespan_t::zero() ),
      pet_name( sname.empty() ? n : sname ),
      pet( nullptr )
  {
    _init_summon_pet_t();
  }

  summon_pet_t( const std::string& n, warlock_t* p, int id )
    : warlock_spell_t( n, p, p->find_spell( id ) ),
      summoning_duration( timespan_t::zero() ),
      pet_name( n ),
      pet( nullptr )
  {
    _init_summon_pet_t();
  }

  summon_pet_t( const std::string& n, warlock_t* p, const spell_data_t* sd )
    : warlock_spell_t( n, p, sd ), summoning_duration( timespan_t::zero() ), pet_name( n ), pet( nullptr )
  {
    _init_summon_pet_t();
  }

  void init_finished() override
  {
    pet = debug_cast<pets::warlock_pet_t*>( player->find_pet( pet_name ) );

    warlock_spell_t::init_finished();
  }

  virtual void execute() override
  {
    pet->summon( summoning_duration );

    warlock_spell_t::execute();
  }

  bool ready() override
  {
    if ( !pet )
    {
      return false;
    }
    return warlock_spell_t::ready();
  }
};

struct summon_main_pet_t : public summon_pet_t
{
  cooldown_t* instant_cooldown;

  summon_main_pet_t( const std::string& n, warlock_t* p )
    : summon_pet_t( n, p ), instant_cooldown( p->get_cooldown( "instant_summon_pet" ) )
  {
    instant_cooldown->duration = timespan_t::from_seconds( 60 );
    ignore_false_positive      = true;
  }

  virtual void schedule_execute( action_state_t* state = nullptr ) override
  {
    warlock_spell_t::schedule_execute( state );

    if ( p()->warlock_pet_list.active )
    {
      p()->warlock_pet_list.active->dismiss();
      p()->warlock_pet_list.active = nullptr;
    }
  }

  virtual bool ready() override
  {
    if ( p()->warlock_pet_list.active == pet )
      return false;

    return summon_pet_t::ready();
  }

  virtual void execute() override
  {
    summon_pet_t::execute();

    p()->warlock_pet_list.active = p()->warlock_pet_list.last = pet;

    if ( p()->buffs.grimoire_of_sacrifice->check() )
      p()->buffs.grimoire_of_sacrifice->expire();
  }
};

// Event for spawning wild imps for Demonology
// Placed in warlock.cpp for expression purposes
struct imp_delay_event_t : public player_event_t
{
  timespan_t diff;

  imp_delay_event_t( warlock_t* p, double delay, double exp ) : player_event_t( *p, timespan_t::from_millis( delay ) )
  {
    diff = timespan_t::from_millis( exp - delay );
  }

  virtual const char* name() const override
  {
    return "imp_delay";
  }

  virtual void execute() override
  {
    warlock_t* p = static_cast<warlock_t*>( player() );

    p->warlock_pet_list.wild_imps.spawn();
    expansion::bfa::trigger_leyshocks_grand_compilation( STAT_HASTE_RATING, p );

    // Remove this event from the vector
    auto it = std::find( p->wild_imp_spawns.begin(), p->wild_imp_spawns.end(), this );
    if ( it != p->wild_imp_spawns.end() )
      p->wild_imp_spawns.erase( it );
  }

  // Used for APL expressions to estimate when imp is "supposed" to spawn
  timespan_t expected_time()
  {
    return std::max( 0_ms, this->remains() + diff );
  }
};
}  // namespace actions

namespace buffs
{
template <typename Base>
struct warlock_buff_t : public Base
{
public:
  typedef warlock_buff_t base_t;
  warlock_buff_t( warlock_td_t& p, const std::string& name, const spell_data_t* s = spell_data_t::nil(),
                  const item_t* item = nullptr )
    : Base( p, name, s, item )
  {
  }

  warlock_buff_t( warlock_t& p, const std::string& name, const spell_data_t* s = spell_data_t::nil(),
                  const item_t* item = nullptr )
    : Base( &p, name, s, item )
  {
  }

protected:
  warlock_t* p()
  {
    return static_cast<warlock_t*>( Base::source );
  }
  const warlock_t* p() const
  {
    return static_cast<warlock_t*>( Base::source );
  }
};
}  // namespace buffs
}  // namespace warlock
