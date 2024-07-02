#pragma once
#include "simulationcraft.hpp"

#include "player/pet_spawner.hpp"
#include "sc_warlock_pets.hpp"
#include "class_modules/apl/warlock.hpp"

namespace warlock
{
struct warlock_t;

// Used for version checking in code (e.g. PTR vs Live)
enum version_check_e
{
  VERSION_PTR,
  VERSION_ANY
};

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
  // Cross-spec
  propagate_const<dot_t*> dots_drain_life;
  propagate_const<dot_t*> dots_corruption;

  // Aff
  propagate_const<dot_t*> dots_agony;
  propagate_const<dot_t*> dots_seed_of_corruption;
  propagate_const<dot_t*> dots_drain_soul;
  propagate_const<dot_t*> dots_siphon_life;
  propagate_const<dot_t*> dots_phantom_singularity;
  propagate_const<dot_t*> dots_unstable_affliction;
  propagate_const<dot_t*> dots_vile_taint;
  propagate_const<dot_t*> dots_drain_life_aoe; // Soul Rot effect
  propagate_const<dot_t*> dots_soul_rot;

  propagate_const<buff_t*> debuffs_haunt;
  propagate_const<buff_t*> debuffs_shadow_embrace;
  propagate_const<buff_t*> debuffs_cruel_epiphany; // Dummy debuff applied to primary target of Seed of Corruption for bug purposes
  propagate_const<buff_t*> debuffs_infirmity; // T30 4pc
  propagate_const<buff_t*> debuffs_umbrafire_kindling; // T31 4pc dummy debuff to track empowered Seeds of Corruption

  // Destro
  propagate_const<dot_t*> dots_immolate;
  propagate_const<dot_t*> dots_searing_bolt;

  propagate_const<buff_t*> debuffs_shadowburn;
  propagate_const<buff_t*> debuffs_eradication;
  propagate_const<buff_t*> debuffs_havoc;
  propagate_const<buff_t*> debuffs_pyrogenics;
  propagate_const<buff_t*> debuffs_conflagrate;

  // Demo
  propagate_const<dot_t*> dots_doom;

  propagate_const<buff_t*> debuffs_the_houndmasters_stratagem;
  propagate_const<buff_t*> debuffs_fel_sunder; // Done in owner target data for easier handling
  propagate_const<buff_t*> debuffs_kazaaks_final_curse; // Not an actual debuff in-game, but useful as a utility feature for Doom
  propagate_const<buff_t*> debuffs_doom_brand; // T31 2pc

  double soc_threshold; // Aff - Seed of Corruption counts damage from cross-spec spells such as Drain Life

  warlock_t& warlock;
  warlock_td_t( player_t* target, warlock_t& p );

  void reset()
  {
    soc_threshold = 0;
  }

  void target_demise();

  int count_affliction_dots() const;
};

struct warlock_t : public player_t
{
public:
  player_t* havoc_target;
  player_t* ua_target; // Used for handling Unstable Affliction target swaps
  std::vector<action_t*> havoc_spells; // Used for smarter target cache invalidation.
  double agony_accumulator;
  double corruption_accumulator;
  double cdf_accumulator; // For T30 Destruction tier set
  double dimensional_accumulator; // For T31 Destruction tier set
  int incinerate_last_target_count; // For use with T30 Destruction tier set
  double shadow_invocation_proc_chance; // 2023-09-10: Annoyingly, at this time there is no listed proc chance in data for Shadow Invocation
  double doom_brand_accumulator;
  std::vector<event_t*> wild_imp_spawns; // Used for tracking incoming imps from HoG

  unsigned active_pets;

  // This should hold any spell data that is guaranteed in the base class or spec, without talents or other external systems required
  struct base_t
  {
    // Shared
    const spell_data_t* drain_life;
    const spell_data_t* corruption;
    const spell_data_t* shadow_bolt;
    const spell_data_t* nethermancy; // Int bonus for all cloth slots

    // Affliction
    const spell_data_t* agony;
    const spell_data_t* agony_2; // Rank 2 still a separate spell (learned automatically). Grants increased max stacks TODO: Check if needed in TWW
    const spell_data_t* xavian_teachings; // Passive granted only to Affliction. Instant cast data in this spell, points to base Corruption spell (172) for the direct damage
    const spell_data_t* malefic_rapture; // TODO: Move from talent section
    const spell_data_t* potent_afflictions; // Affliction Mastery - Increased DoT and Malefic Rapture damage
    const spell_data_t* affliction_warlock; // Spec aura

    // Demonology
    const spell_data_t* hand_of_guldan;
    const spell_data_t* hog_impact; // Secondary spell responsible for impact damage
    const spell_data_t* wild_imp; // Data for pet summoning
    const spell_data_t* fel_firebolt_2; // Still a separate spell (learned automatically). Reduces pet's energy cost
    const spell_data_t* demonic_core; // The passive responsible for the proc chance. TODO: Move to talents (if not done already)
    const spell_data_t* demonic_core_buff; // Buff spell data TODO: Move to talents (if not done already)
    const spell_data_t* master_demonologist; // Demonology Mastery - Increased demon damage
    const spell_data_t* demonology_warlock; // Spec aura

    // Destruction
    const spell_data_t* immolate; // Replaces Corruption
    const spell_data_t* immolate_dot; // Primary spell data only contains information on direct damage
    const spell_data_t* incinerate; // Replaces Shadow Bolt
    const spell_data_t* incinerate_energize; // Soul Shard data is in a separate spell
    const spell_data_t* chaos_bolt; // TODO: Move from talent section
    const spell_data_t* chaotic_energies; // Destruction Mastery - Increased spell damage with random range
    const spell_data_t* destruction_warlock; // Spec aura
  } warlock_base;

  // Main pet held in active, guardians should be handled by pet spawners.
  struct pets_t
  {
    warlock_pet_t* active;

    spawner::pet_spawner_t<pets::destruction::infernal_t, warlock_t> infernals;
    spawner::pet_spawner_t<pets::destruction::blasphemy_t, warlock_t> blasphemy;

    spawner::pet_spawner_t<pets::affliction::darkglare_t, warlock_t> darkglares;

    spawner::pet_spawner_t<pets::demonology::dreadstalker_t, warlock_t> dreadstalkers;
    spawner::pet_spawner_t<pets::demonology::vilefiend_t, warlock_t> vilefiends;
    spawner::pet_spawner_t<pets::demonology::demonic_tyrant_t, warlock_t> demonic_tyrants;
    spawner::pet_spawner_t<pets::demonology::grimoire_felguard_pet_t, warlock_t> grimoire_felguards;
    spawner::pet_spawner_t<pets::demonology::wild_imp_pet_t, warlock_t> wild_imps;

    // Nether Portal demons (TODO: Remove? Celebrate?)
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

    spawner::pet_spawner_t<pets::demonology::pit_lord_t, warlock_t> pit_lords;
    spawner::pet_spawner_t<pets::demonology::doomfiend_t, warlock_t> doomfiends;

    pets_t( warlock_t* w );
  } warlock_pet_list;

  std::vector<std::string> pet_name_list;

  // Talents
  struct talents_t
  {
    // Class Tree

    player_talent_t demonic_inspiration; // Primary pet attack speed increase
    player_talent_t wrathful_minion; // Primary pet damage increase
    player_talent_t grimoire_of_synergy;
    const spell_data_t* demonic_synergy; // Buff from Grimoire of Synergy
    player_talent_t socrethars_guile;
    player_talent_t sargerei_technique;
    player_talent_t soul_conduit;
    player_talent_t soulburn;
    const spell_data_t* soulburn_buff; // This buff is applied after using Soulburn and prevents another usage unless cleared

    // Specializations

    // Shared
    player_talent_t grimoire_of_sacrifice; // Aff/Destro only
    const spell_data_t* grimoire_of_sacrifice_buff; // 1 hour duration, enables proc functionality, canceled if pet summoned
    const spell_data_t* grimoire_of_sacrifice_proc; // Damage data is here, but RPPM of proc trigger is in buff data

    // Affliction
    player_talent_t malefic_rapture; // TODO: Move to base section
    const spell_data_t* malefic_rapture_dmg; // Damage events use this ID, but primary talent contains the spcoeff
    player_talent_t unstable_affliction; // TODO: Check if higher ranks are still separate spell data
    const spell_data_t* unstable_affliction_2; // Soul Shard on demise, still seems to be separate spell (learned automatically)
    const spell_data_t* unstable_affliction_3; // +5 seconds to duration, still seems to be separate spell (learned automatically)

    player_talent_t writhe_in_agony;
    player_talent_t seed_of_corruption;
    const spell_data_t* seed_of_corruption_aoe; // Explosion damage when Seed ticks

    player_talent_t dark_virtuosity;
    player_talent_t absolute_corruption;
    player_talent_t siphon_life; // TODO: Now a passive, not an ability
    player_talent_t kindled_malice;

    player_talent_t nightfall;
    const spell_data_t* nightfall_buff;
    player_talent_t volatile_agony; // TODO: New

    player_talent_t improved_shadow_bolt; // TODO: New
    player_talent_t drain_soul; // This represents the talent node but not much else
    const spell_data_t* drain_soul_dot; // This is the previous talent spell, contains all channel data
    player_talent_t summoners_embrace; // TODO: New
    // Grimoire of Sacrifice (shared with Destruction)
    player_talent_t phantom_singularity;
    const spell_data_t* phantom_singularity_tick; // Actual AoE spell information in here
    player_talent_t vile_taint; // Base talent, AoE cast data
    const spell_data_t* vile_taint_dot; // DoT data

    player_talent_t haunt; // TODO: Shadow Embrace is applied by other talent
    player_talent_t shadow_embrace; // TODO: Stack count is 2
    const spell_data_t* shadow_embrace_debuff; // Default values set from talent data, but contains debuff info
    player_talent_t sacrolashs_dark_strike; // Increased Corruption ticking damage, and ticks extend Curses (not implemented)
    player_talent_t summon_darkglare;
    player_talent_t cunning_cruelty; // TODO: New
    player_talent_t infirmity; // TODO: Move from tier sets

    player_talent_t improved_haunt; // TODO: New
    player_talent_t malediction; // TODO: New
    player_talent_t malevolent_visionary; // TODO: AoE behavior may have changed
    player_talent_t contagion; // TODO: New
    player_talent_t cull_the_weak; // TODO: New

    player_talent_t creeping_death; 
    player_talent_t soul_rot;
    player_talent_t tormented_crescendo; // Free, instant Malefic Rapture procs from Shadow Bolt/Drain Soul
    const spell_data_t* tormented_crescendo_buff;

    player_talent_t xavius_gambit; // Unstable Affliction Damage Multiplier
    player_talent_t focused_malignancy; // Increaed Malefic Rapture damage to target with Unstable Affliction
    player_talent_t perpetual_unstability; // TODO: New
    player_talent_t malign_omen; // TODO: New
    player_talent_t relinquished; // TODO: New
    player_talent_t withering_bolt; // Increased damage on Shadow Bolt/Drain Soul based on active DoT count on target
    player_talent_t improved_malefic_rapture; // TODO: New

    player_talent_t oblivion; // TODO: New
    player_talent_t deaths_embrace; // TODO: New
    player_talent_t dark_harvest; // Buffs from hitting targets with Soul Rot
    const spell_data_t* dark_harvest_buff;
    player_talent_t ravenous_afflictions; // TODO: New
    player_talent_t malefic_touch; // TODO: New

    // Demonology
    player_talent_t demoniac; // TODO: Move from base section
    const spell_data_t* demonbolt_spell;
    const spell_data_t* demonic_core_spell;
    const spell_data_t* demonic_core_buff;

    player_talent_t implosion;
    const spell_data_t* implosion_aoe; // Note: in combat logs this is attributed to the player, not the imploding pet
    player_talent_t call_dreadstalkers;
    const spell_data_t* call_dreadstalkers_2; // Contains duration data TODO: Check if still relevant
    player_talent_t demonbolt; // TODO: Remove (leftover from earlier in DF)

    player_talent_t imp_gang_boss;
    player_talent_t spiteful_reconstitution; // Increased Implosion damage and consuming Demonic Core may spawn a Wild Imp
    player_talent_t dreadlash;
    player_talent_t carnivorous_stalkers; // Chance for Dreadstalkers to perform additional Dreadbites

    player_talent_t inner_demons;
    player_talent_t soul_strike; // TODO: Make sure older iterations have been cleaned up
    player_talent_t bilescourge_bombers;
    const spell_data_t* bilescourge_bombers_aoe; // Ground AoE data
    player_talent_t demonic_strength;

    player_talent_t rune_of_shadows; // TODO: New
    player_talent_t imperator; // Increased critical strike chance for Wild Imps' Fel Firebolt (additive)
    player_talent_t fel_invocation; // TODO: Update given new design and tree placement for VF
    player_talent_t annihilan_training; // Permanent aura on Felguard that gives 10% damage buff
    const spell_data_t* annihilan_training_buff; // Applied to pet, not player
    player_talent_t shadow_invocation; // Bilescourge Bomber damage and proc. TODO: Review proc chances from AoE hits
    player_talent_t wicked_maw; // TODO: Previously Houndmaster's Strategem. Convert/rename as necessary

    player_talent_t power_siphon; // NOTE: Power Siphon WILL consume Imp Gang Boss as if it were a regular imp (last checked 2022-10-04)
    const spell_data_t* power_siphon_buff; // Semi-hidden aura that controls the bonus Demonbolt damage
    player_talent_t summon_demonic_tyrant; // TODO: Review what is considered baseline given updated tree placement
    const spell_data_t* demonic_power_buff;
    player_talent_t grimoire_felguard;
    player_talent_t demonic_knowledge; // TODO: Remove
    player_talent_t the_houndmasters_stratagem; // TODO: Possibly migrated to Wicked Maw. Remove after migration
    const spell_data_t* the_houndmasters_stratagem_debuff;

    player_talent_t the_expendables; // Per-pet stacking buff to damage when a Wild Imp expires
    player_talent_t blood_invocation; // TODO: New
    player_talent_t umbral_blaze; // TODO: May now be rolling periodic, check DoT behavior
    const spell_data_t* umbral_blaze_dot;
    player_talent_t reign_of_tyranny; // TODO: Review behavior due to updated tree placement
    player_talent_t demonic_calling;
    const spell_data_t* demonic_calling_buff;
    player_talent_t fiendish_oblation; // TODO: New
    player_talent_t fel_sunder; // Increase damage taken debuff when hit by main pet Felstorm TODO: Check GFG does not proc
    const spell_data_t* fel_sunder_debuff;
    player_talent_t bloodbound_imps; // TODO: Remove (leftover from earlier in DF)

    player_talent_t doom; // TODO: Heavily changed, now a passive
    player_talent_t pact_of_the_imp_mother; // Chance for Hand of Gul'dan to proc a second time on execute
    player_talent_t summon_vilefiend;
    player_talent_t dread_calling; // Stacking buff to next Dreadstalkers damage
    const spell_data_t* dread_calling_buff; // This buffs stacks on the warlock, a different one applies to the pet
    player_talent_t antoran_armaments; // Increased Felguard damage and Soul Strike cleave TODO: Check if GFG is benefiting

    player_talent_t doom_eternal; // TODO: New
    player_talent_t impending_doom; // TODO: New
    player_talent_t flametouched; // TODO: New
    player_talent_t foul_mouth; // TODO: New
    player_talent_t shadowtouched; // TODO: New
    player_talent_t improved_demonic_tactics; // TODO: Possibly migrated/combined from other effects (Cavitation/Heavy Handed)
    player_talent_t demonic_brutality; // TODO: New

    player_talent_t pact_of_the_eredruin; // TODO: New
    player_talent_t sacrificed_souls;
    player_talent_t mark_of_shatug; // TODO: New
    player_talent_t mark_of_fharg; // TODO: New
    player_talent_t the_houndmasters_gambit; // TODO: New
    player_talent_t immutable_hatred; // TODO: Review behavior in new core economy
    player_talent_t guillotine;

    player_talent_t kazaaks_final_curse; // TODO: Remove
    player_talent_t cavitation; // TODO: Possibly migrated to Improved Demonic Tactics. Remove after migration
    player_talent_t nether_portal; // TODO: Remove. Celebrate. Then celebrate again.
    const spell_data_t* nether_portal_buff;
    player_talent_t shadows_bite; // TODO: Remove
    const spell_data_t* shadows_bite_buff;
    player_talent_t fel_and_steel; // TODO: Remove
    player_talent_t heavy_handed; // TODO: Remove (maybe merged/overlapped with Cavitation -> Improved Demonic Tactics)
    player_talent_t malefic_impact; // Increased damage and critical strike chance for Hand of Gul'dan (NOTE: Temporarily named 'Dirty Hands' on PTR)
    player_talent_t nerzhuls_volition; // TODO: Remove
    const spell_data_t* nerzhuls_volition_buff;
    player_talent_t stolen_power; // TODO: Remove
    const spell_data_t* stolen_power_stacking_buff;
    const spell_data_t* stolen_power_final_buff;
    player_talent_t soulbound_tyrant; // TODO: Remove
    player_talent_t infernal_command; // TODO: Remove
    player_talent_t guldans_ambition; // TODO: Remove
    const spell_data_t* guldans_ambition_summon;
    const spell_data_t* soul_glutton;
    const spell_data_t* demonic_servitude;

    // Destruction
    player_talent_t chaos_bolt; // TODO: Move to base section
    player_talent_t conflagrate; // Base 2 charges
    const spell_data_t* conflagrate_2; // TODO: Check if separate spells still needed

    player_talent_t backdraft;
    const spell_data_t* backdraft_buff;
    player_talent_t rain_of_fire;
    const spell_data_t* rain_of_fire_tick;

    player_talent_t roaring_blaze;
    const spell_data_t* conflagrate_debuff; // Debuff associated with Roaring Blaze
    player_talent_t improved_conflagrate; // +1 charge for Conflagrate
    player_talent_t backlash; // Crit chance increase. NOT IMPLEMENTED: Damage proc when physically attacked
    player_talent_t mayhem; // It appears that the only spells that can proc Mayhem are ones that can be Havoc'd
    player_talent_t havoc; // Talent data for Havoc is both the debuff and the action
    const spell_data_t* havoc_debuff; // This is a second copy of the talent data for use in places that are shared by Havoc and Mayhem
    player_talent_t pyrogenics; // Enemies affected by Rain of Fire receive debuff for increased Fire damage
    const spell_data_t* pyrogenics_debuff;
    player_talent_t inferno; // TODO: Heavily changed
    player_talent_t cataclysm;

    player_talent_t indiscriminate_flames; // TODO: New
    player_talent_t rolling_havoc; // Increased damage buff when spells are duplicated by Mayhem/Havoc
    const spell_data_t* rolling_havoc_buff;
    player_talent_t scalding_flames; // Increased Immolate damage and duration

    player_talent_t shadowburn;
    const spell_data_t* shadowburn_2; // Contains Soul Shard energize data
    player_talent_t explosive_potential; // Reduces base Conflagrate cooldown by 2 seconds
    // TODO: Summoner's Embrace - may be shared with Affliction?
    // Grimoire of Sacrifice - shared with Affliction
    player_talent_t ashen_remains; // Increased Chaos Bolt and Incinerate damage to targets afflicted by Immolate TODO: Review behavior
    player_talent_t channel_demonfire;
    const spell_data_t* channel_demonfire_tick;
    const spell_data_t* channel_demonfire_travel; // Only holds travel speed

    player_talent_t blistering_atrophy; // TODO: New
    player_talent_t conflagration_of_chaos; // Conflagrate/Shadowburn has chance to make next cast of it a guaranteed crit TODO: Review behavior
    const spell_data_t* conflagration_of_chaos_cf; // Player buff which affects next Conflagrate
    const spell_data_t* conflagration_of_chaos_sb; // Player buff which affects next Shadowburn
    player_talent_t emberstorm; // TODO: New
    player_talent_t summon_infernal;
    const spell_data_t* summon_infernal_main; // Data for main infernal summoning
    const spell_data_t* infernal_awakening; // AoE on impact is attributed to the Warlock
    player_talent_t fire_and_brimstone;
    player_talent_t flashpoint; // Stacking haste buff from Immolate ticks on high-health targets
    const spell_data_t* flashpoint_buff;
    player_talent_t raging_demonfire; // Additional Demonfire bolts and bolts extend Immolate

    player_talent_t fiendish_cruelty; // TODO: New
    player_talent_t eradication;
    const spell_data_t* eradication_debuff;
    player_talent_t crashing_chaos; // Summon Infernal increases the damage of next 8 Chaos Bolt or Rain of Fire casts
    const spell_data_t* crashing_chaos_buff;
    player_talent_t rain_of_chaos; // TOCHECK: Ensure behavior is unchanged from SL
    const spell_data_t* rain_of_chaos_buff;
    const spell_data_t* summon_infernal_roc; // Contains Rain of Chaos infernal duration
    player_talent_t reverse_entropy;
    const spell_data_t* reverse_entropy_buff;
    player_talent_t internal_combustion;
    player_talent_t demonfire_mastery; // TODO: New

    player_talent_t devastation; // TODO: New
    player_talent_t ritual_of_ruin;
    const spell_data_t* impending_ruin_buff; // Stacking buff, triggers Ritual of Ruin buff at max
    const spell_data_t* ritual_of_ruin_buff;
    player_talent_t ruin; // Damage increase to several spells TODO: Review behavior

    player_talent_t soul_fire;
    const spell_data_t* soul_fire_2; // Contains Soul Shard energize data
    player_talent_t improved_chaos_bolt; // TODO: New
    player_talent_t burn_to_ashes; // Chaos Bolt and Rain of Fire increase damage of next 2 Incinerates
    const spell_data_t* burn_to_ashes_buff;
    player_talent_t master_ritualist; // Reduces proc cost of Ritual of Ruin
    player_talent_t power_overwhelming; // Stacking mastery buff for spending Soul Shards
    const spell_data_t* power_overwhelming_buff;
    player_talent_t diabolic_embers; // Incinerate generates more Soul Shards
    player_talent_t dimensional_rift; // TODO: Review behavior with loss of tier set
    const spell_data_t* shadowy_tear_summon; // This only creates the "pet"
    const spell_data_t* shadow_barrage; // Casts Rift version of Shadow Bolt on ticks
    const spell_data_t* rift_shadow_bolt; // Separate ID from Warlock's Shadow Bolt
    const spell_data_t* unstable_tear_summon; // This only creates the "pet"
    const spell_data_t* chaos_barrage; // Triggers ticks of Chaos Barrage bolts
    const spell_data_t* chaos_barrage_tick;
    const spell_data_t* chaos_tear_summon; // This only creates the "pet"
    const spell_data_t* rift_chaos_bolt; // Separate ID from Warlock's Chaos Bolt

    player_talent_t decimation; // Incinerate and Conflagrate casts reduce Soul Fire cooldown TODO: New proc behavior?
    player_talent_t chaos_incarnate; // Maximum mastery value for some spells
    player_talent_t avatar_of_destruction; // TODO: Behavior changed
    const spell_data_t* summon_blasphemy; // TODO: Deprecated in favor of Overfiend. Remove
    player_talent_t dimension_ripper; // TODO: New
    player_talent_t unstable_rifts; // TODO: Newish, possibly DF tier set behavior

    player_talent_t pandemonium; // TODO: Remove
    player_talent_t cry_havoc; // TODO: Remove
    const spell_data_t* cry_havoc_proc;
    player_talent_t improved_immolate; // TODO: Folded into another talent. Remove
    player_talent_t infernal_brand; // TODO: Remove
    player_talent_t madness_of_the_azjaqir; // TODO: Remove (leftover from earlier in DF)
    const spell_data_t* madness_cb;
    const spell_data_t* madness_rof;
    const spell_data_t* madness_sb;
    player_talent_t chaosbringer; // TODO: Remove
  } talents;

  struct hero_talents_t
  {
    // Diabolist
    player_talent_t diabolic_ritual;

    player_talent_t cloven_souls;
    player_talent_t touch_of_rancora;
    player_talent_t secrets_of_the_coven;

    player_talent_t cruelty_of_kerxan;
    player_talent_t infernal_machine;

    player_talent_t flames_of_xoroth;
    player_talent_t abyssal_dominion;
    player_talent_t gloom_of_nathreza;

    player_talent_t ruination;

    // Hellcaller
    player_talent_t wither;

    player_talent_t xalans_ferocity;
    player_talent_t blackened_soul;
    player_talent_t xalans_cruelty;

    player_talent_t hatefury_rituals;
    player_talent_t bleakheart_tactics;

    player_talent_t mark_of_xavius;
    player_talent_t seeds_of_their_demise;
    player_talent_t mark_of_perotharn;

    player_talent_t malevolence;

    // Soul Harvester
    player_talent_t demonic_soul;
    
    player_talent_t necrolyte_teachings;
    player_talent_t soul_anathema;
    player_talent_t demoniacs_fervor;

    player_talent_t shared_fate;
    player_talent_t feast_of_souls;

    player_talent_t wicked_reaping;
    player_talent_t quietus;
    player_talent_t sataiels_volition;

    player_talent_t shadow_of_death;
  } hero;

  struct proc_actions_t
  {
    action_t* bilescourge_bombers_aoe_tick;
    action_t* bilescourge_bombers_proc; // From Shadow Invocation talent
    action_t* summon_random_demon; // Basic version, currently shares overlap with Nether Portal list
    action_t* summon_nether_portal_demon; // Separate version for Nether Portal based summons due to Ner'zhul's Volition
    action_t* doom_brand_explosion; // Demonology T31 2pc
    action_t* rain_of_fire_tick;
    action_t* avatar_of_destruction; // Triggered when Ritual of Ruin is consumed
    action_t* soul_combustion; // Summon Soulkeeper AoE tick
    action_t* channel_demonfire; // Destruction T30 proc
  } proc_actions;

  struct tier_sets_t
  {
    // Affliction
    const spell_data_t* cruel_inspiration; // T29 2pc procs haste buff
    const spell_data_t* cruel_epiphany; // T29 4pc also procs stacks of this buff when 2pc procs, increases Malefic Rapture/Seed of Corruption damage
    const spell_data_t* infirmity; // T30 4pc applies this debuff when using Vile Taint/Phantom Singularity
    const spell_data_t* umbrafire_kindling; // T31 4pc buff after casting Soul Rot. Empowers Malefic Rapture or Seed of Corruption

    // Demonology
    const spell_data_t* blazing_meteor; // T29 4pc procs buff which makes next Hand of Gul'dan instant + increased damage
    const spell_data_t* rite_of_ruvaraad; // T30 4pc buff which increases pet damage while Grimoire: Felguard is active
    const spell_data_t* doom_brand; // T31 2pc debuff which does AoE damage on expiration. Hand of Gul'dan reduces remaining duration on all Brands
    const spell_data_t* doom_brand_debuff; // Doom Brand primary data isn't really useful to SimC, actual debuff is a separate spell. (Misc Value 1 for primary does hold the NPC ID for Doomfiend, though)
    const spell_data_t* doom_brand_aoe;
    const spell_data_t* doom_bolt_volley; // T31 4pc spell used by Doomfiend pet

    // Destruction 
    const spell_data_t* chaos_maelstrom; // T29 2pc procs crit chance buff
    const spell_data_t* channel_demonfire; // T30 2pc damage proc is separate from talent version
    const spell_data_t* umbrafire_embers; // T30 4pc enables stacking buff on 2pc procs
    const spell_data_t* dimensional_cinder; // T31 2pc AoE proc
    const spell_data_t* flame_rift; // T31 4pc - Additional Dimensional Rift type
    const spell_data_t* searing_bolt; // Projectile + DoT from Flame Rift
  } tier;

  // Cooldowns - Used for accessing cooldowns outside of their respective actions, such as reductions/resets
  struct cooldowns_t
  {
    propagate_const<cooldown_t*> haunt;
    propagate_const<cooldown_t*> darkglare;
    propagate_const<cooldown_t*> demonic_tyrant;
    propagate_const<cooldown_t*> infernal;
    propagate_const<cooldown_t*> shadowburn;
    propagate_const<cooldown_t*> dimensional_rift;
    propagate_const<cooldown_t*> soul_rot;
    propagate_const<cooldown_t*> call_dreadstalkers;
    propagate_const<cooldown_t*> soul_fire;
    propagate_const<cooldown_t*> felstorm_icd; // Shared between Felstorm, Demonic Strength, and Guillotine
    propagate_const<cooldown_t*> grimoire_felguard;
  } cooldowns;

  // Buffs
  struct buffs_t
  {
    // Shared Buffs
    propagate_const<buff_t*> grimoire_of_sacrifice; // Buff which grants damage proc
    propagate_const<buff_t*> demonic_synergy;
    propagate_const<buff_t*> soulburn;
    propagate_const<buff_t*> pet_movement; // One unified buff for some form of pet movement stat tracking

    // Affliction Buffs
    propagate_const<buff_t*> drain_life; // Dummy buff used internally for handling Inevitable Demise cases
    propagate_const<buff_t*> nightfall;
    propagate_const<buff_t*> soul_rot; // Buff for determining if Drain Life is zero cost and aoe.
    propagate_const<buff_t*> tormented_crescendo;
    propagate_const<buff_t*> dark_harvest_haste; // One buff in game...
    propagate_const<buff_t*> dark_harvest_crit; // ...but split into two in simc for better handling
    propagate_const<buff_t*> cruel_inspiration; // T29 2pc
    propagate_const<buff_t*> cruel_epiphany; // T29 4pc
    propagate_const<buff_t*> umbrafire_kindling; // T31 4pc buff

    // Demonology Buffs
    propagate_const<buff_t*> demonic_power; // Buff from Summon Demonic Tyrant (increased demon damage + duration)
    propagate_const<buff_t*> demonic_core;
    propagate_const<buff_t*> power_siphon; // Hidden buff from Power Siphon that increases damage of successive Demonbolts
    propagate_const<buff_t*> demonic_calling;
    propagate_const<buff_t*> inner_demons;
    propagate_const<buff_t*> nether_portal;
    propagate_const<buff_t*> wild_imps; // Buff for tracking how many Wild Imps are currently out (does NOT include imps waiting to be spawned)
    propagate_const<buff_t*> dreadstalkers; // Buff for tracking number of Dreadstalkers currently out
    propagate_const<buff_t*> vilefiend; // Buff for tracking if Vilefiend is currently out
    propagate_const<buff_t*> tyrant; // Buff for tracking if Demonic Tyrant is currently out
    propagate_const<buff_t*> grimoire_felguard; // Buff for tracking if GFG pet is currently out
    propagate_const<buff_t*> prince_malchezaar; // Buff for tracking Malchezaar (who is currently disabled in sims)
    propagate_const<buff_t*> eyes_of_guldan; // Buff for tracking if rare random summon is currently out
    propagate_const<buff_t*> dread_calling;
    propagate_const<buff_t*> shadows_bite;
    propagate_const<buff_t*> fel_covenant;
    propagate_const<buff_t*> stolen_power_building; // Stacking buff, triggers final buff as a separate buff at max stacks
    propagate_const<buff_t*> stolen_power_final;
    propagate_const<buff_t*> nether_portal_total; // Dummy buff. Used for Gul'dan's Ambition as the counter to trigger Soul Gluttony
    propagate_const<buff_t*> demonic_servitude; // From Reign of Tyranny talent
    propagate_const<buff_t*> blazing_meteor; // T29 4pc buff
    propagate_const<buff_t*> rite_of_ruvaraad; // T30 4pc buff

    // Destruction Buffs
    propagate_const<buff_t*> backdraft;
    propagate_const<buff_t*> reverse_entropy;
    propagate_const<buff_t*> rain_of_chaos;
    propagate_const<buff_t*> impending_ruin;
    propagate_const<buff_t*> ritual_of_ruin;
    propagate_const<buff_t*> rolling_havoc;
    propagate_const<buff_t*> conflagration_of_chaos_cf;
    propagate_const<buff_t*> conflagration_of_chaos_sb;
    propagate_const<buff_t*> flashpoint;
    propagate_const<buff_t*> crashing_chaos;
    propagate_const<buff_t*> power_overwhelming;
    propagate_const<buff_t*> madness_cb;
    propagate_const<buff_t*> madness_rof;
    propagate_const<buff_t*> madness_sb;
    propagate_const<buff_t*> madness_rof_snapshot; // (Dummy buff) 2022-10-16: For Rain of Fire, Madness of the Azj'Aqir affects ALL active events until the next cast of Rain of Fire
    propagate_const<buff_t*> burn_to_ashes;
    propagate_const<buff_t*> chaos_maelstrom; // T29 2pc buff
    propagate_const<buff_t*> umbrafire_embers; // T30 4pc buff
  } buffs;

  // Gains - Many are automatically handled
  struct gains_t
  {
    // Class Talents
    gain_t* soul_conduit;

    // Affliction
    gain_t* agony;
    gain_t* drain_soul;
    gain_t* unstable_affliction_refund;
    gain_t* pandemic_invocation;

    // Destruction
    gain_t* incinerate_crits;
    gain_t* incinerate_fnb_crits;
    gain_t* immolate;
    gain_t* immolate_crits;
    gain_t* infernal;
    gain_t* shadowburn_refund;
    gain_t* inferno;

    // Demonology
    gain_t* doom;
    gain_t* soulbound_tyrant;
    gain_t* soul_strike; // Only with Fel Invocation talent
  } gains;

  // Procs
  struct procs_t
  {
    // Class Talents
    proc_t* soul_conduit;
    proc_t* demonic_inspiration;
    proc_t* wrathful_minion;
    proc_t* inquisitors_gaze; // Random proc as of 10.0.5

    // Affliction
    proc_t* nightfall;
    std::array<proc_t*, 8> malefic_rapture; // This length should be at least equal to the maximum number of Affliction DoTs that can be active on a target.
    proc_t* pandemic_invocation_shard;
    proc_t* tormented_crescendo;
    proc_t* cruel_inspiration; // T29 2pc

    // Demonology
    proc_t* demonic_knowledge;
    proc_t* demonic_calling;
    proc_t* one_shard_hog;
    proc_t* two_shard_hog;
    proc_t* three_shard_hog;
    proc_t* summon_random_demon;
    proc_t* portal_summon;
    proc_t* carnivorous_stalkers;
    proc_t* shadow_invocation; // Bilescourge Bomber proc on most spells
    proc_t* imp_gang_boss;
    proc_t* spiteful_reconstitution;
    proc_t* umbral_blaze;
    proc_t* nerzhuls_volition;
    proc_t* pact_of_the_imp_mother;
    proc_t* blazing_meteor; // T29 4pc
    proc_t* doomfiend; // T31 4pc

    // Destruction
    proc_t* reverse_entropy;
    proc_t* rain_of_chaos;
    proc_t* ritual_of_ruin;
    proc_t* avatar_of_destruction;
    proc_t* mayhem;
    proc_t* conflagration_of_chaos_cf;
    proc_t* conflagration_of_chaos_sb;
    proc_t* chaos_maelstrom; // T29 2pc
    proc_t* channel_demonfire; // T30 2pc
    proc_t* dimensional_refund; // T31 2pc charge refund on Dimensional Rift
  } procs;

  int initial_soul_shards;
  std::string default_pet;
  bool disable_auto_felstorm; // For Demonology main pet
  shuffled_rng_t* rain_of_chaos_rng;

  warlock_t( sim_t* sim, util::string_view name, race_e r );

  // Character Definition
  void init_spells() override;
  void init_items() override;
  void init_base_stats() override;
  void create_buffs() override;
  void init_gains() override;
  void init_procs() override;
  void init_rng() override;
  void init_action_list() override;
  void init_resources( bool force ) override;
  void init_special_effects() override;
  void reset() override;
  void create_options() override;
  int get_spawning_imp_count();
  timespan_t time_to_imps( int count );
  int active_demon_count() const;
  void expendables_trigger_helper( warlock_pet_t* source );
  bool min_version_check( version_check_e version ) const;
  action_t* pass_corruption_action( warlock_t* p ); // Horrible, horrible hack for getting Corruption in Aff module until things are re-merged
  action_t* pass_soul_rot_action( warlock_t* p ); // ...they made me do it for Soul Rot too
  void create_actions() override;
  void create_affliction_proc_actions();
  void create_demonology_proc_actions();
  void create_destruction_proc_actions();
  action_t* create_action( util::string_view name, util::string_view options ) override;
  pet_t* create_pet( util::string_view name, util::string_view type = {} ) override;
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
  double composite_player_pet_damage_multiplier( const action_state_t*, bool ) const override;
  double composite_player_target_pet_damage_multiplier( player_t* target, bool guardian ) const override;
  void invalidate_cache( cache_e ) override;
  double composite_spell_crit_chance() const override;
  double composite_melee_crit_chance() const override;
  void combat_begin() override;
  void init_assessors() override;
  std::unique_ptr<expr_t> create_expression( util::string_view name_str ) override;
  std::string default_potion() const override { return warlock_apl::potion( this ); }
  std::string default_flask() const override { return warlock_apl::flask( this ); }
  std::string default_food() const override { return warlock_apl::food( this ); }
  std::string default_rune() const override { return warlock_apl::rune( this ); }
  std::string default_temporary_enchant() const override { return warlock_apl::temporary_enchant( this ); }
  void apply_affecting_auras( action_t& action ) override;

  target_specific_t<warlock_td_t> target_data;

  const warlock_td_t* find_target_data( const player_t* target ) const override
  {
    return target_data[ target ];
  }

  warlock_td_t* get_target_data( player_t* target ) const override
  {
    warlock_td_t*& td = target_data[ target ];
    if ( !td )
    {
      td = new warlock_td_t( target, const_cast<warlock_t&>( *this ) );
    }
    return td;
  }

  action_t* create_action_warlock( util::string_view, util::string_view );

  action_t* create_action_affliction( util::string_view, util::string_view );
  void create_buffs_affliction();
  void init_spells_affliction();
  void init_gains_affliction();
  void init_rng_affliction();
  void init_procs_affliction();

  action_t* create_action_demonology( util::string_view, util::string_view );
  void create_buffs_demonology();
  void init_spells_demonology();
  void init_gains_demonology();
  void init_rng_demonology();
  void init_procs_demonology();

  action_t* create_action_destruction( util::string_view, util::string_view );
  void create_buffs_destruction();
  void init_spells_destruction();
  void init_gains_destruction();
  void init_rng_destruction();
  void init_procs_destruction();

  pet_t* create_main_pet( util::string_view pet_name, util::string_view pet_type );
  std::unique_ptr<expr_t> create_pet_expression( util::string_view name_str );
};

namespace helpers
{
  struct imp_delay_event_t : public player_event_t
  {
    imp_delay_event_t( warlock_t*, double, double );
    timespan_t diff;
    virtual const char* name() const override;
    virtual void execute() override;
    timespan_t expected_time();
  };

  struct sc_event_t : public player_event_t
  {
    sc_event_t( warlock_t*, int );
    gain_t* shard_gain;
    warlock_t* pl;
    int shards_used;
    virtual const char* name() const override;
    virtual void execute() override;
  };

  bool crescendo_check( warlock_t* p );
}

namespace buffs
{
template <typename Base>
struct warlock_buff_t : public Base
{
public:
  using base_t = warlock_buff_t;
  warlock_buff_t( warlock_td_t& p, util::string_view name, const spell_data_t* s = spell_data_t::nil(),
                  const item_t* item = nullptr )
    : Base( p, name, s, item )
  {
  }

  warlock_buff_t( warlock_t& p, util::string_view name, const spell_data_t* s = spell_data_t::nil(),
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
