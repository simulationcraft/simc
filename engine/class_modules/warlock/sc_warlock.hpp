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
  VERSION_10_1_5,
  VERSION_10_1_0,
  VERSION_10_0_7,
  VERSION_10_0_5,
  VERSION_10_0_0,
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
  propagate_const<buff_t*> debuffs_dread_touch;
  propagate_const<buff_t*> debuffs_cruel_epiphany; // Dummy debuff applied to primary target of Seed of Corruption for bug purposes
  propagate_const<buff_t*> debuffs_infirmity; // T30 4pc

  // Destro
  propagate_const<dot_t*> dots_immolate;

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

  double soc_threshold; // Aff - Seed of Corruption counts damage from cross-spec spells such as Drain Life

  warlock_t& warlock;
  warlock_td_t( player_t* target, warlock_t& p );

  void reset()
  {
    soc_threshold = 0;
  }

  void target_demise();

  int count_affliction_dots();
};

struct warlock_t : public player_t
{
public:
  player_t* havoc_target;
  player_t* ua_target; // Used for handling Unstable Affliction target swaps
  player_t* ss_source; // Needed to track where Soul Swap copies from
  struct ss_full_state_t
  {
    struct ss_action_state_t
    {
      action_t* action;
      bool action_copied;
      timespan_t duration;
      int stacks;
    };

    ss_action_state_t corruption;
    ss_action_state_t agony;
    ss_action_state_t unstable_affliction;
    ss_action_state_t siphon_life;
    ss_action_state_t haunt;
    ss_action_state_t soul_rot;
    ss_action_state_t phantom_singularity;
    ss_action_state_t vile_taint;
    // Seed of Corruption is also copied, NYI
  } soul_swap_state;
  std::vector<action_t*> havoc_spells; // Used for smarter target cache invalidation.
  double agony_accumulator;
  double corruption_accumulator;
  double cdf_accumulator; // For T30 Destruction tier set
  int incinerate_last_target_count; // For use with T30 Destruction tier set
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
    const spell_data_t* agony_2; // Rank 2 still a separate spell (learned automatically). Grants increased max stacks
    const spell_data_t* xavian_teachings; // Separate spell (learned automatically). Instant cast data in this spell, points to base Corruption spell (172) for the direct damage
    const spell_data_t* potent_afflictions; // Affliction Mastery - Increased DoT and Malefic Rapture damage
    const spell_data_t* affliction_warlock; // Spec aura

    // Demonology
    const spell_data_t* hand_of_guldan;
    const spell_data_t* hog_impact; // Secondary spell responsible for impact damage
    const spell_data_t* wild_imp; // Data for pet summoning
    const spell_data_t* fel_firebolt_2; // Still a separate spell (learned automatically). Reduces pet's energy cost
    const spell_data_t* demonic_core; // The passive responsible for the proc chance
    const spell_data_t* demonic_core_buff; // Buff spell data
    const spell_data_t* master_demonologist; // Demonology Mastery - Increased demon damage
    const spell_data_t* demonology_warlock; // Spec aura

    // Destruction
    const spell_data_t* immolate; // Replaces Corruption
    const spell_data_t* immolate_dot; // Primary spell data only contains information on direct damage
    const spell_data_t* incinerate; // Replaces Shadow Bolt
    const spell_data_t* incinerate_energize; // Soul Shard data is in a separate spell
    const spell_data_t* chaotic_energies; // Destruction Mastery - Increased spell damage with random range
    const spell_data_t* destruction_warlock; // Spec aura
  } warlock_base;

  // Main pet held in active/last, guardians should be handled by pet spawners. TODO: Use spawner for Infernal/Darkglare?
  struct pets_t
  {
    warlock_pet_t* active;

    spawner::pet_spawner_t<pets::destruction::infernal_t, warlock_t> infernals;
    spawner::pet_spawner_t<pets::destruction::blasphemy_t, warlock_t> blasphemy;

    spawner::pet_spawner_t<pets::affliction::darkglare_t, warlock_t> darkglares;

    spawner::pet_spawner_t<pets::demonology::dreadstalker_t, warlock_t> dreadstalkers; // TODO: Damage increased by 15% in 10.1.5
    spawner::pet_spawner_t<pets::demonology::vilefiend_t, warlock_t> vilefiends;
    spawner::pet_spawner_t<pets::demonology::demonic_tyrant_t, warlock_t> demonic_tyrants;
    spawner::pet_spawner_t<pets::demonology::grimoire_felguard_pet_t, warlock_t> grimoire_felguards;
    spawner::pet_spawner_t<pets::demonology::wild_imp_pet_t, warlock_t> wild_imps;

    // Nether Portal demons (TOCHECK: Are spawn rates still uniform?)
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
    player_talent_t grim_feast; // Faster Drain Life
    player_talent_t summon_soulkeeper; // Active ground AoE which spends hidden stacking buff. NOT A PET
    const spell_data_t* summon_soulkeeper_aoe; // The actual active spell which triggers the AoE
    const spell_data_t* tormented_soul_buff; // Stacks periodically, duration of Summon Soulkeeper is based on stack count
    const spell_data_t* soul_combustion; // AoE tick damage for Summon Soulkeeper
    player_talent_t inquisitors_gaze;
    const spell_data_t* inquisitors_gaze_buff; // Aura which triggers the damage procs
    const spell_data_t* fel_barrage; // Inquisitor's Eye damage spell
    player_talent_t soulburn;
    const spell_data_t* soulburn_buff; // This buff is applied after using Soulburn and prevents another usage unless cleared

    // Specializations

    // Shared
    player_talent_t grimoire_of_sacrifice; // Aff/Destro only
    const spell_data_t* grimoire_of_sacrifice_buff; // 1 hour duration, enables proc functionality, canceled if pet summoned
    const spell_data_t* grimoire_of_sacrifice_proc; // Damage data is here, but RPPM of proc trigger is in buff data
    player_talent_t grand_warlocks_design; // One spell data for all 3 specs

    // Affliction
    player_talent_t malefic_rapture;
    const spell_data_t* malefic_rapture_dmg; // Damage events use this ID, but primary talent contains the spcoeff

    player_talent_t unstable_affliction;
    const spell_data_t* unstable_affliction_2; // Soul Shard on demise, still seems to be separate spell (learned automatically)
    const spell_data_t* unstable_affliction_3; // +5 seconds to duration, still seems to be separate spell (learned automatically)
    player_talent_t seed_of_corruption;
    const spell_data_t* seed_of_corruption_aoe; // Explosion damage when Seed ticks

    player_talent_t nightfall;
    const spell_data_t* nightfall_buff;
    player_talent_t writhe_in_agony;
    player_talent_t sow_the_seeds;

    player_talent_t shadow_embrace;
    const spell_data_t* shadow_embrace_debuff; // Default values set from talent data, but contains debuff info
    player_talent_t dark_virtuosity;
    player_talent_t kindled_malice;
    player_talent_t agonizing_corruption; // Only applies to targets which already have Agony

    player_talent_t drain_soul; // This represents the talent node but not much else
    const spell_data_t* drain_soul_dot; // This is the previous talent spell, contains all channel data
    player_talent_t absolute_corruption;
    player_talent_t siphon_life;
    player_talent_t phantom_singularity;
    const spell_data_t* phantom_singularity_tick; // Actual AoE spell information in here
    player_talent_t vile_taint; // Base talent, AoE cast data
    const spell_data_t* vile_taint_dot; // DoT data

    player_talent_t pandemic_invocation; // Late DoT refresh deals damage and has Soul Shard chance
    const spell_data_t* pandemic_invocation_proc; // Damage data
    player_talent_t inevitable_demise; // The talent version of the ability
    const spell_data_t* inevitable_demise_buff; // The buff version referenced by the talent tooltip
    player_talent_t soul_swap; // Spend Soul Shard to apply core dots (Corruption, Agony, UA)
    const spell_data_t* soul_swap_ua; // Separate copy of Unstable Affliction data, since UA is applied even without the talent
    const spell_data_t* soul_swap_buff; // Buff indicating Soul Swap is holding a copy of data
    const spell_data_t* soul_swap_exhale; // Second action that replaces Soul Swap while holding a copy, applies the copies to target
    player_talent_t soul_flame; // AoE damage on kills
    const spell_data_t* soul_flame_proc; // The actual spell damage data
    // Grimoire of Sacrifice (shared with Destruction)
    
    player_talent_t focused_malignancy; // Increaed Malefic Rapture damage to target with Unstable Affliction
    player_talent_t withering_bolt; // Increased damage on Shadow Bolt/Drain Soul based on active DoT count on target
    player_talent_t sacrolashs_dark_strike; // Increased Corruption ticking damage, and ticks extend Curses (not implemented)

    player_talent_t creeping_death;
    player_talent_t haunt;
    player_talent_t summon_darkglare; 
    player_talent_t soul_rot;

    player_talent_t malefic_affliction; // TODO: REMOVED in 10.1.5
    const spell_data_t* malefic_affliction_buff; // TODO: REMOVED in 10.1.5
    player_talent_t xavius_gambit; // Unstable Affliction Damage Multiplier, replaces Malefic Affliction in 10.1.5
    player_talent_t tormented_crescendo; // Free, instant Malefic Rapture procs from Shadow Bolt/Drain Soul
    const spell_data_t* tormented_crescendo_buff;
    player_talent_t seized_vitality; // Additional Haunt damage
    player_talent_t malevolent_visionary; // Longer Darkglare and more damage scaling
    player_talent_t wrath_of_consumption; // DoT damage buff on target deaths
    const spell_data_t* wrath_of_consumption_buff;
    player_talent_t souleaters_gluttony; // Soul Rot CDR from Unstable Affliction

    player_talent_t doom_blossom; // Reworked in 10.1.5: Seed of Corruption damage on Unstable Affliction target procs AoE damage
    const spell_data_t* doom_blossom_proc; // Spell data updated in 10.1.5 to new spell ID
    player_talent_t dread_touch; // Reworked in 10.1.5: Malefic Rapture on Unstable Affliction target applies debuff increasing DoT damage
    const spell_data_t* dread_touch_debuff; // Applied to target when Dread Touch procs
    player_talent_t haunted_soul; // Haunt increase ALL DoT damage while active
    const spell_data_t* haunted_soul_buff; // Applied to player while Haunt is active
    // Grand Warlock's Design (formerly Wilfred's). Shared across all 3 specs
    player_talent_t grim_reach; // Darkglare hits all targets affected by DoTs
    player_talent_t dark_harvest; // Buffs from hitting targets with Soul Rot
    const spell_data_t* dark_harvest_buff;

    // Demonology
    player_talent_t call_dreadstalkers;
    const spell_data_t* call_dreadstalkers_2; // Contains duration data

    player_talent_t demonbolt; // Note: Demonic Core is a guaranteed passive, even though this talent is optional
    player_talent_t dreadlash;
    player_talent_t annihilan_training; // Permanent aura on Felguard that gives 10% damage buff
    const spell_data_t* annihilan_training_buff; // Applied to pet, not player

    player_talent_t demonic_knowledge; // Demonic Core chance on Hand of Gul'dan cast
    player_talent_t summon_vilefiend;
    player_talent_t soul_strike;
    player_talent_t bilescourge_bombers; // Reworked in 10.1.5: Soul Shard cost reduced to 0
    const spell_data_t* bilescourge_bombers_aoe; // Ground AoE data
    player_talent_t demonic_strength;
    player_talent_t the_houndmasters_stratagem; // Whitelisted warlock spells do more damage to target afflicted with debuff
    const spell_data_t* the_houndmasters_stratagem_debuff; // Debuff applied by Dreadstalker's Dreadbite

    player_talent_t implosion;
    const spell_data_t* implosion_aoe; // Note: in combat logs this is attributed to the player, not the imploding pet
    player_talent_t shadows_bite; // Demonbolt damage increase after Dreadstalkers despawn
    const spell_data_t* shadows_bite_buff;
    player_talent_t carnivorous_stalkers; // Chance for Dreadstalkers to perform additional Dreadbites
    player_talent_t fel_and_steel; // Reworked in 10.1.5: Increase's primary Felguard's Legion Strike and Felstorm damage
    player_talent_t fel_might; // TODO: REMOVED in 10.1.5, replaced by Heavy Handed
    player_talent_t heavy_handed; // Primary Felguard crit chance increase (additive)

    player_talent_t power_siphon; // NOTE: Power Siphon WILL consume Imp Gang Boss as if it were a regular imp (last checked 2022-10-04)
    const spell_data_t* power_siphon_buff; // Semi-hidden aura that controls the bonus Demonbolt damage
    player_talent_t malefic_impact; // Increased damage and critical strike chance for Hand of Gul'dan (NOTE: Temporarily named 'Dirty Hands' on PTR)
    player_talent_t imperator; // Increased critical strike chance for Wild Imps' Fel Firebolt (additive)
    player_talent_t grimoire_felguard;

    player_talent_t bloodbound_imps; // Increased Demonic Core proc chance from Wild Imps
    player_talent_t inner_demons; // Reworked in 10.1.5: Now only 1 rank and tree location changed
    player_talent_t doom;
    player_talent_t demonic_calling; // Reworked in 10.1.5: Now only 1 rank and tree location changed
    const spell_data_t* demonic_calling_buff;
    player_talent_t demonic_meteor; // TODO: REMOVED in 10.1.5
    player_talent_t fel_sunder; // Increase damage taken debuff when hit by main pet Felstorm
    const spell_data_t* fel_sunder_debuff;

    player_talent_t fel_covenant; // TODO: REMOVED in 10.1.5
    const spell_data_t* fel_covenant_buff;
    player_talent_t umbral_blaze; // Reworked in 10.1.5: Location changed in tree and increased proc chance.
    const spell_data_t* umbral_blaze_dot; // Reworked in 10.1.5: "retains remaining damage when reapplied"
    player_talent_t imp_gang_boss;
    player_talent_t kazaaks_final_curse; // Doom deals increased damage based on active demon count
    player_talent_t ripped_through_the_portal; // TODO: REMOVED in 10.1.5
    player_talent_t dread_calling; // Stacking buff to next Dreadstalkers damage. Reworked in 10.1.5: Now 2 ranks and tree location has changed
    const spell_data_t* dread_calling_buff; // This buffs stacks on the warlock, a different one applies to the pet
    player_talent_t cavitation; // Increased critical strike damage for primary Felguard. TOCHECK: As of 2023-06-21 PTR, this is actually granting double the stated value

    player_talent_t nether_portal; // TOCHECK: 2022-10-07 Portal summon damage is possibly slightly above current in-game values (~1% max), full audit needed closer to release
    const spell_data_t* nether_portal_buff; // Aura on player while the portal is active
    player_talent_t summon_demonic_tyrant; // TOCHECK: 2022-10-07 Pit Lord is not currently extendable by Tyrant
    const spell_data_t* demonic_power_buff; // Buff on player when Tyrant is summoned, should grant 15% damage to pets
    player_talent_t antoran_armaments; // Increased Felguard damage and Soul Strike cleave (TOCHECK: 2022-10-08 - this is applying to Grimoire: Felguard erratically)

    player_talent_t nerzhuls_volition; // Chance to summon additional demon from Nether Portal summons (TOCHECK: It is currently assumed this cannot proc itself)
    player_talent_t stolen_power; // Stacking buff from Wild Imps, at max get increased Shadow Bolt or Demonbolt damage
    const spell_data_t* stolen_power_stacking_buff; // Triggers final buff when reaching max stacks
    const spell_data_t* stolen_power_final_buff;
    player_talent_t sacrificed_souls;
    player_talent_t soulbound_tyrant; // Soul Shards on Tyrant summons
    player_talent_t pact_of_the_imp_mother; // Chance for Hand of Gul'dan to proc a second time on execute
    player_talent_t the_expendables; // Per-pet stacking buff to damage when a Wild Imp expires
    player_talent_t infernal_command; // Increased Wild Imp and Dreadstalker damage while Felguard active

    player_talent_t guldans_ambition; // Summons a Pit Lord at end of Nether Portal
    const spell_data_t* soul_glutton; // Buff on Pit Lord based on demons summoned
    player_talent_t reign_of_tyranny; // Each summoned active pet gives stacks of Demonic Servitude. Tyrant snapshots this buff on summon for more damage
    const spell_data_t* demonic_servitude; // TOCHECK: 2022-10-09 - In addition to aura stack bugs, Nether Portal demons are not currently giving stacks in beta (not implemented)
    // Grand Warlock's Design (formerly Wilfred's). Shared across all 3 specs
    player_talent_t immutable_hatred;
    player_talent_t guillotine;

    // Destruction
    player_talent_t chaos_bolt;

    player_talent_t conflagrate; // Base 2 charges
    const spell_data_t* conflagrate_2; // Contains Soul Shard information
    player_talent_t reverse_entropy;
    const spell_data_t* reverse_entropy_buff;
    player_talent_t internal_combustion;
    player_talent_t rain_of_fire;
    const spell_data_t* rain_of_fire_tick;

    player_talent_t backdraft;
    const spell_data_t* backdraft_buff;
    player_talent_t mayhem; // It appears that the only spells that can proc Mayhem are ones that can be Havoc'd
    player_talent_t havoc; // Talent data for Havoc is both the debuff and the action
    const spell_data_t* havoc_debuff; // This is a second copy of the talent data for use in places that are shared by Havoc and Mayhem
    player_talent_t pyrogenics; // Enemies affected by Rain of Fire receive debuff for increased Fire damage
    const spell_data_t* pyrogenics_debuff;

    player_talent_t roaring_blaze;
    const spell_data_t* conflagrate_debuff; // Debuff associated with Roaring Blaze
    player_talent_t improved_conflagrate; // +1 charge for Conflagrate
    player_talent_t explosive_potential; // Reduces base Conflagrate cooldown by 2 seconds
    player_talent_t channel_demonfire;
    const spell_data_t* channel_demonfire_tick;
    const spell_data_t* channel_demonfire_travel; // Only holds travel speed
    player_talent_t pandemonium; // Additional trigger chance for Mayhem or debuff duration for Havoc (talent)
    player_talent_t cry_havoc; // Chaos Bolts on Havoc'd target proc AoE
    const spell_data_t* cry_havoc_proc; // AoE damage (includes target hit)
    player_talent_t improved_immolate; // Duration increase
    player_talent_t inferno; // TOCHECK: Do SL target caps remain in effect?
    player_talent_t cataclysm;

    player_talent_t soul_fire;
    const spell_data_t* soul_fire_2; // Contains Soul Shard energize data
    player_talent_t shadowburn;
    const spell_data_t* shadowburn_2; // Contains Soul Shard energize data
    player_talent_t raging_demonfire; // Additional Demonfire bolts and bolts extend Immolate
    player_talent_t rolling_havoc; // Increased damage buff when spells are duplicated by Mayhem/Havoc
    const spell_data_t* rolling_havoc_buff;
    player_talent_t backlash; // Crit chance increase. NOT IMPLEMENTED: Damage proc when physically attacked
    player_talent_t fire_and_brimstone;

    player_talent_t decimation; // Incinerate and Conflagrate casts reduce Soul Fire cooldown
    player_talent_t conflagration_of_chaos; // Conflagrate/Shadowburn has chance to make next cast of it a guaranteed crit
    const spell_data_t* conflagration_of_chaos_cf; // Player buff which affects next Conflagrate
    const spell_data_t* conflagration_of_chaos_sb; // Player buff which affects next Shadowburn
    player_talent_t flashpoint; // Stacking haste buff from Immolate ticks on high-health targets
    const spell_data_t* flashpoint_buff;
    player_talent_t scalding_flames; // Increased Immolate damage

    player_talent_t ruin; // Damage increase to several spells
    player_talent_t eradication;
    const spell_data_t* eradication_debuff;
    player_talent_t ashen_remains; // Increased Chaos Bolt and Incinerate damage to targets afflicted by Immolate
    // Grimoire of Sacrifice (shared with Affliction)

    player_talent_t summon_infernal;
    const spell_data_t* summon_infernal_main; // Data for main infernal summoning
    const spell_data_t* infernal_awakening; // AoE on impact is attributed to the Warlock
    player_talent_t diabolic_embers; // Incinerate generates more Soul Shards
    player_talent_t ritual_of_ruin;
    const spell_data_t* impending_ruin_buff; // Stacking buff, triggers Ritual of Ruin buff at max
    const spell_data_t* ritual_of_ruin_buff;
    
    player_talent_t crashing_chaos; // Reworked in 10.1.5: Summon Infernal increases the damage of next 8 Chaos Bolt or Rain of Fire casts
    const spell_data_t* crashing_chaos_buff; // Spell data updated in 10.1.5 to new spell ID
    player_talent_t infernal_brand; // Infernal melees increase Infernal AoE damage
    player_talent_t power_overwhelming; // Stacking mastery buff for spending Soul Shards
    const spell_data_t* power_overwhelming_buff;
    player_talent_t madness_of_the_azjaqir; // Buffs that reward repeating certain spells
    const spell_data_t* madness_cb;
    const spell_data_t* madness_rof;
    const spell_data_t* madness_sb;
    player_talent_t master_ritualist; // Reduces proc cost of Ritual of Ruin
    player_talent_t burn_to_ashes; // Chaos Bolt and Rain of Fire increase damage of next 2 Incinerates
    const spell_data_t* burn_to_ashes_buff;

    player_talent_t rain_of_chaos; // TOCHECK: Ensure behavior is unchanged from SL
    const spell_data_t* rain_of_chaos_buff;
    const spell_data_t* summon_infernal_roc; // Contains Rain of Chaos infernal duration
    // Grand Warlock's Design (formerly Wilfred's). Shared across all 3 specs
    player_talent_t chaos_incarnate; // Maximum mastery value for some spells
    player_talent_t dimensional_rift;
    const spell_data_t* shadowy_tear_summon; // This only creates the "pet"
    const spell_data_t* shadow_barrage; // Casts Rift version of Shadow Bolt on ticks
    const spell_data_t* rift_shadow_bolt; // Separate ID from Warlock's Shadow Bolt
    const spell_data_t* unstable_tear_summon; // This only creates the "pet"
    const spell_data_t* chaos_barrage; // Triggers ticks of Chaos Barrage bolts
    const spell_data_t* chaos_barrage_tick;
    const spell_data_t* chaos_tear_summon; // This only creates the "pet"
    const spell_data_t* rift_chaos_bolt; // Separate ID from Warlock's Chaos Bolt
    player_talent_t avatar_of_destruction; // Summons Blasphemy when consuming Ritual of Ruin
    const spell_data_t* summon_blasphemy;
  } talents;

  struct proc_actions_t
  {
    action_t* soul_flame_proc;
    action_t* pandemic_invocation_proc;
    action_t* bilescourge_bombers_aoe_tick;
    action_t* summon_random_demon; // Nether Portal and Inner Demons
    action_t* rain_of_fire_tick;
    action_t* avatar_of_destruction; // Triggered when Ritual of Ruin is consumed
    action_t* soul_combustion; // Summon Soulkeeper AoE tick
    action_t* fel_barrage; // Inquisitor's Eye spell (new as of 10.0.5)
    action_t* channel_demonfire; // Destruction T30 proc
  } proc_actions;

  struct tier_sets_t
  {
    // Affliction
    const spell_data_t* cruel_inspiration; // T29 2pc procs haste buff
    const spell_data_t* cruel_epiphany; // T29 4pc also procs stacks of this buff when 2pc procs, increases Malefic Rapture/Seed of Corruption damage
    const spell_data_t* infirmity; // T30 4pc applies this debuff when using Vile Taint/Phantom Singularity

    // Demonology
    const spell_data_t* blazing_meteor; // T29 4pc procs buff which makes next Hand of Gul'dan instant + increased damage
    const spell_data_t* rite_of_ruvaraad; // T30 4pc buff which increases pet damage while Grimoire: Felguard is active

    // Destruction 
    const spell_data_t* chaos_maelstrom; // T29 2pc procs crit chance buff
    const spell_data_t* channel_demonfire; // T30 2pc damage proc is separate from talent version
    const spell_data_t* umbrafire_embers; // T30 4pc enables stacking buff on 2pc procs
  } tier;

  // Cooldowns - Used for accessing cooldowns outside of their respective actions, such as reductions/resets
  struct cooldowns_t
  {
    propagate_const<cooldown_t*> haunt;
    propagate_const<cooldown_t*> darkglare;
    propagate_const<cooldown_t*> demonic_tyrant;
    propagate_const<cooldown_t*> infernal;
    propagate_const<cooldown_t*> shadowburn;
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
    propagate_const<buff_t*> tormented_soul; // Hidden stacking buff
    propagate_const<buff_t*> tormented_soul_generator; // Dummy buff with periodic tick to add a stack every 20 seconds
    propagate_const<buff_t*> inquisitors_gaze; // Aura that indicates Inquisitor's Eye is summoned
    propagate_const<buff_t*> soulburn;
    propagate_const<buff_t*> pet_movement; // One unified buff for some form of pet movement stat tracking

    // Affliction Buffs
    propagate_const<buff_t*> drain_life; // Dummy buff used internally for handling Inevitable Demise cases
    propagate_const<buff_t*> nightfall;
    propagate_const<buff_t*> inevitable_demise; // TOCHECK: (noticed 2023-03-16) Having one point in this talent may be getting half the intended value!
    propagate_const<buff_t*> soul_swap; // Buff for when Soul Swap currently is holding copies
    propagate_const<buff_t*> soul_rot; // Buff for determining if Drain Life is zero cost and aoe.
    propagate_const<buff_t*> wrath_of_consumption;
    propagate_const<buff_t*> tormented_crescendo;
    propagate_const<buff_t*> malefic_affliction;
    propagate_const<buff_t*> haunted_soul;
    propagate_const<buff_t*> active_haunts; // Dummy buff used for tracking Haunts in multi-target to properly handle Haunted Soul
    propagate_const<buff_t*> dark_harvest_haste; // One buff in game...
    propagate_const<buff_t*> dark_harvest_crit; // ...but split into two in simc for better handling
    propagate_const<buff_t*> cruel_inspiration; // T29 2pc
    propagate_const<buff_t*> cruel_epiphany; // T29 4pc

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
    gain_t* demonic_meteor;
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
    proc_t* doom_blossom;
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
    proc_t* demonic_meteor;
    proc_t* imp_gang_boss;
    proc_t* umbral_blaze;
    proc_t* nerzhuls_volition;
    proc_t* pact_of_the_imp_mother;
    proc_t* blazing_meteor; // T29 4pc

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
  } procs;

  int initial_soul_shards;
  std::string default_pet;
  bool disable_auto_felstorm; // For Demonology main pet
  bool use_pet_stat_update_delay;
  shuffled_rng_t* rain_of_chaos_rng;
  const spell_data_t* version_10_1_5_data;

  warlock_t( sim_t* sim, util::string_view name, race_e r );

  // Character Definition
  void init_spells() override;
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
  void darkglare_extension_helper( warlock_t* p, timespan_t darkglare_extension );
  int active_demon_count() const;
  void expendables_trigger_helper( warlock_pet_t* source );
  bool min_version_check( version_check_e version ) const;
  action_t* pass_corruption_action( warlock_t* p ); // Horrible, horrible hack for getting Corruption in Aff module until things are re-merged
  action_t* pass_soul_rot_action( warlock_t* p ); // ...they made me do it for Soul Rot too
  bool crescendo_check( warlock_t* p ); 
  void create_actions() override;
  void create_soul_swap_actions();
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

  // sc_warlock
  action_t* create_action_warlock( util::string_view, util::string_view );

  // sc_warlock_affliction
  action_t* create_action_affliction( util::string_view, util::string_view );
  void create_buffs_affliction();
  void init_spells_affliction();
  void init_gains_affliction();
  void init_rng_affliction();
  void init_procs_affliction();

  // sc_warlock_demonology
  action_t* create_action_demonology( util::string_view, util::string_view );
  void create_buffs_demonology();
  void init_spells_demonology();
  void init_gains_demonology();
  void init_rng_demonology();
  void init_procs_demonology();

  // sc_warlock_destruction
  action_t* create_action_destruction( util::string_view, util::string_view );
  void create_buffs_destruction();
  void init_spells_destruction();
  void init_gains_destruction();
  void init_rng_destruction();
  void init_procs_destruction();

  // sc_warlock_pets
  pet_t* create_main_pet( util::string_view pet_name, util::string_view pet_type );
  pet_t* create_demo_pet( util::string_view pet_name, util::string_view pet_type );
  std::unique_ptr<expr_t> create_pet_expression( util::string_view name_str );
};

namespace actions
{

// Event for triggering delayed refunds from Soul Conduit
// Delay prevents instant reaction time issues for rng refunds
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

struct warlock_heal_t : public heal_t
{
  warlock_heal_t( util::string_view n, warlock_t* p, const uint32_t id ) : heal_t( n, p, p->find_spell( id ) )
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
  bool can_havoc;

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

  void consume_resource() override
  {
    spell_t::consume_resource();

    if ( resource_current == RESOURCE_SOUL_SHARD && p()->in_combat )
    {
      // This event prevents instantaneous reactions by the sim to refunded shards
      if ( p()->talents.soul_conduit->ok() )
      {
        make_event<sc_event_t>( *p()->sim, p(), as<int>( last_resource_cost ) );
      }

      if ( p()->talents.grand_warlocks_design.ok() )
      {
        switch ( p()->specialization() )
        {
          case WARLOCK_AFFLICTION:
            p()->cooldowns.darkglare->adjust( -last_resource_cost * p()->talents.grand_warlocks_design->effectN( 1 ).time_value(), false );
            break;
          case WARLOCK_DEMONOLOGY:
            p()->cooldowns.demonic_tyrant->adjust( -last_resource_cost * p()->talents.grand_warlocks_design->effectN( 2 ).time_value(), false );
            break;
          case WARLOCK_DESTRUCTION:
            p()->cooldowns.infernal->adjust( -last_resource_cost * p()->talents.grand_warlocks_design->effectN( 3 ).time_value(), false );
            break;
          default:
            break;
        }
      }
    }
  }

  void execute() override
  {
    spell_t::execute();

    if ( p()->specialization() == WARLOCK_DESTRUCTION && p()->talents.rolling_havoc.ok() && use_havoc() )
    {
      p()->buffs.rolling_havoc->trigger();
    }
  }

  void impact( action_state_t* s ) override
  {
    spell_t::impact( s );

    if ( p()->specialization() == WARLOCK_DESTRUCTION && p()->talents.reverse_entropy.ok() )
    {
      bool success = p()->buffs.reverse_entropy->trigger();
      if ( success )
      {
        p()->procs.reverse_entropy->occur();
      }
    }

    if ( can_havoc && p()->specialization() == WARLOCK_DESTRUCTION && p()->talents.mayhem.ok() )
    {
      // Attempt a Havoc trigger here. The debuff has an ICD so we do not need to worry about checking if it is already up
      auto tl = target_list();
      auto n = available_targets( tl );
      
      if ( n > 1u )
      {
        player_t* trigger_target = tl.at( 1u + rng().range( n - 1u ) );
        if ( td( trigger_target )->debuffs_havoc->trigger() )
          p()->procs.mayhem->occur();
      }
    }
  }

  void tick( dot_t* d ) override
  {
    spell_t::tick( d );

    if ( p()->specialization() == WARLOCK_DESTRUCTION && p()->talents.reverse_entropy.ok() )
    {
      bool success = p()->buffs.reverse_entropy->trigger();
      if ( success )
      {
        p()->procs.reverse_entropy->occur();
      }
    }
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = spell_t::composite_target_multiplier( t );

    if ( p()->talents.dread_touch.ok() && td( t )->debuffs_dread_touch->check() && data().affected_by( p()->talents.dread_touch_debuff->effectN( 1 ) ) )
      m *= 1.0 + td( t )->debuffs_dread_touch->check_stack_value(); // TOCHECK: Is using stack value an outdated holdover?

    return m;
  }

  double action_multiplier() const override
  {
    double m = spell_t::action_multiplier();

    m *= 1.0 + p()->buffs.demonic_synergy->check_stack_value();

    return m;
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double m = spell_t::composite_ta_multiplier( s );

    if ( p()->talents.wrath_of_consumption.ok() && p()->buffs.wrath_of_consumption->check() && data().affected_by( p()->talents.wrath_of_consumption_buff->effectN( 1 ) ) )
      m *= 1.0 + p()->buffs.wrath_of_consumption->check_stack_value();

    if ( p()->talents.haunted_soul.ok() && p()->buffs.haunted_soul->check() && data().affected_by( p()->talents.haunted_soul_buff->effectN( 1 ) ) )
      m *= 1.0 + p()->buffs.haunted_soul->check_stack_value();

    return m;
  }

  double composite_crit_damage_bonus_multiplier() const override
  {
    double m = spell_t::composite_crit_damage_bonus_multiplier();

    if ( p()->specialization() == WARLOCK_DESTRUCTION && p()->sets->has_set_bonus( WARLOCK_DESTRUCTION, T29, B4 ) )
      m += p()->sets->set( WARLOCK_DESTRUCTION, T29, B4 )->effectN( 1 ).percent();
    
    return m;
  }

  void extend_dot( dot_t* dot, timespan_t extend_duration )
  {
    if ( dot->is_ticking() )
    {
      dot->adjust_duration( extend_duration, dot->current_action->dot_duration * 1.5 );
    }
  }

  //Destruction specific things for Havoc/Mayhem

  // We need to ensure that the target cache is invalidated, which sometimes does not take place in select_target()
  // due to other methods we have overridden involving Havoc
  bool select_target() override
  {
    auto saved_target = target;

    bool passed = spell_t::select_target();

    if ( passed && target != saved_target && use_havoc() )
      target_cache.is_valid = false;

    return passed;
  }

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
        base_aoe_multiplier *= p()->talents.havoc_debuff->effectN( 1 ).percent();
        p()->havoc_spells.push_back(this);
    }
  }

  //End Destruction specific things

  std::unique_ptr<expr_t> create_expression( util::string_view name_str ) override
  {
    return spell_t::create_expression( name_str );
  }
};

struct grimoire_of_sacrifice_damage_t : public warlock_spell_t
{
  grimoire_of_sacrifice_damage_t( warlock_t* p )
    : warlock_spell_t( "grimoire_of_sacrifice_damage_proc", p, p->talents.grimoire_of_sacrifice_proc )
  {
    background = true;
    proc = true;

    base_dd_multiplier *= 1.0 + p->talents.demonic_inspiration->effectN( 2 ).percent();
    base_dd_multiplier *= 1.0 + p->talents.wrathful_minion->effectN( 2 ).percent();
  }
};

struct demonic_synergy_callback_t : public dbc_proc_callback_t
{
  warlock_t* owner;

  demonic_synergy_callback_t( warlock_t* p, special_effect_t& e )
    : dbc_proc_callback_t( p, e ), owner( p )
  {
  }

  void execute( action_t* /* a */, action_state_t* ) override
  {
    if ( owner->buffs.grimoire_of_sacrifice->check() )
    {
      owner->buffs.demonic_synergy->trigger();
    }
    else if ( owner->warlock_pet_list.active )
    {
      owner->warlock_pet_list.active->buffs.demonic_synergy->trigger();
    }
  }
};

struct inquisitors_gaze_callback_t : public dbc_proc_callback_t
{
  warlock_t* w;

  inquisitors_gaze_callback_t( warlock_t* p, special_effect_t& e )
    : dbc_proc_callback_t( p, e ), w( p )
  { }

  void execute( action_t*, action_state_t* ) override
  {
    w->buffs.inquisitors_gaze->trigger();
    w->procs.inquisitors_gaze->occur();
  }
};

using residual_action_t = residual_action::residual_periodic_action_t<warlock_spell_t>;

struct summon_pet_t : public warlock_spell_t
{
  timespan_t summoning_duration;
  std::string pet_name;
  warlock_pet_t* pet;

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

    target = player;
  }

public:
  summon_pet_t( util::string_view n, warlock_t* p, util::string_view sname = {} )
    : warlock_spell_t( p, sname.empty() ? fmt::format( "Summon {}", n ) : sname ),
      summoning_duration( timespan_t::zero() ),
      pet_name( sname.empty() ? n : sname ),
      pet( nullptr )
  {
    _init_summon_pet_t();
  }

  summon_pet_t( util::string_view n, warlock_t* p, int id )
    : warlock_spell_t( n, p, p->find_spell( id ) ),
      summoning_duration( timespan_t::zero() ),
      pet_name( n ),
      pet( nullptr )
  {
    _init_summon_pet_t();
  }

  summon_pet_t( util::string_view n, warlock_t* p, const spell_data_t* sd )
    : warlock_spell_t( n, p, sd ), summoning_duration( timespan_t::zero() ), pet_name( n ), pet( nullptr )
  {
    _init_summon_pet_t();
  }

  void init_finished() override
  {
    pet = debug_cast<warlock_pet_t*>( player->find_pet( pet_name ) );

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

  summon_main_pet_t( util::string_view n, warlock_t* p, int id )
    : summon_pet_t( n, p, id ), instant_cooldown( p->get_cooldown( "instant_summon_pet" ) )
  {
    instant_cooldown->duration = 60_s;
    ignore_false_positive      = true;
  }

  summon_main_pet_t( util::string_view n, warlock_t* p )
    : summon_pet_t( n, p ), instant_cooldown( p->get_cooldown( "instant_summon_pet" ) )
  {
    instant_cooldown->duration = 60_s;
    ignore_false_positive      = true;
  }

  void schedule_execute( action_state_t* state = nullptr ) override
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

    p()->warlock_pet_list.active = pet;

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
