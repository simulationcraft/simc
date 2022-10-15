#pragma once
#include "simulationcraft.hpp"

#include "player/pet_spawner.hpp"
#include "sc_warlock_pets.hpp"
#include "class_modules/apl/warlock.hpp"

namespace warlock
{
struct warlock_t;

//Used for version checking in code (e.g. PTR vs Live)
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
  propagate_const<dot_t*> dots_drain_life_aoe; // Affliction - Soul Rot effect
  propagate_const<dot_t*> dots_soul_rot; // DF - Affliction only
  propagate_const<dot_t*> dots_corruption; // DF - Removed from Destruction

  // Aff
  propagate_const<dot_t*> dots_agony;
  propagate_const<dot_t*> dots_seed_of_corruption;
  propagate_const<dot_t*> dots_drain_soul;
  propagate_const<dot_t*> dots_siphon_life;
  propagate_const<dot_t*> dots_phantom_singularity;
  propagate_const<dot_t*> dots_unstable_affliction;
  propagate_const<dot_t*> dots_vile_taint;

  propagate_const<buff_t*> debuffs_haunt;
  propagate_const<buff_t*> debuffs_shadow_embrace;
  propagate_const<buff_t*> debuffs_malefic_affliction;
  propagate_const<buff_t*> debuffs_dread_touch;

  // Destro
  propagate_const<dot_t*> dots_immolate;

  propagate_const<buff_t*> debuffs_shadowburn;
  propagate_const<buff_t*> debuffs_eradication;
  propagate_const<buff_t*> debuffs_roaring_blaze;
  propagate_const<buff_t*> debuffs_havoc;
  propagate_const<buff_t*> debuffs_pyrogenics;
  propagate_const<buff_t*> debuffs_conflagrate; // Artist formerly known as debuffs_roaring_blaze

  // Demo
  propagate_const<dot_t*> dots_doom;

  propagate_const<buff_t*> debuffs_from_the_shadows;
  propagate_const<buff_t*> debuffs_fel_sunder; // Done in owner target data for easier handling
  propagate_const<buff_t*> debuffs_kazaaks_final_curse; // Not an actual debuff in-game, but useful as a utility feature for Doom

  double soc_threshold; //Aff - Seed of Corruption counts damage from cross-spec spells such as Drain Life

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
  std::vector<action_t*> havoc_spells;  // Used for smarter target cache invalidation.
  double agony_accumulator;
  double corruption_accumulator;
  std::vector<event_t*> wild_imp_spawns;      // Used for tracking incoming imps from HoG

  unsigned active_pets;

  // This should hold any spell data that is guaranteed in the base class or spec, without talents or other external systems required
  struct base_t
  {
    // Shared
    const spell_data_t* drain_life;
    const spell_data_t* corruption;
    const spell_data_t* shadow_bolt;
    const spell_data_t* nethermancy; // Int bonus for all cloth slots. TOCHECK: As of 2022-09-21 this is possibly bugged on beta and not working

    // Affliction
    const spell_data_t* agony;
    const spell_data_t* agony_2; // Rank 2 still learned on level up, grants increased max stacks
    const spell_data_t* potent_afflictions; // Affliction Mastery - Increased DoT and Malefic Rapture damage
    const spell_data_t* affliction_warlock; // Spec aura

    // Demonology
    const spell_data_t* hand_of_guldan;
    const spell_data_t* hog_impact; // Secondary spell responsible for impact damage
    const spell_data_t* wild_imp; // Data for pet summoning
    const spell_data_t* fel_firebolt_2; // 2022-10-03 - This is still a separate spell learned automatically when switching spec?
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
    warlock_pet_t* last;
    static const int INFERNAL_LIMIT  = 1;

    //TODO: Refactor infernal code including new talent Rain of Chaos
    std::array<pets::destruction::infernal_t*, INFERNAL_LIMIT> infernals;
    spawner::pet_spawner_t<pets::destruction::infernal_t, warlock_t>
        roc_infernals;  // Infernal(s) summoned by Rain of Chaos
    spawner::pet_spawner_t<pets::destruction::blasphemy_t, warlock_t>
        blasphemy;  // DF - Now a Destruction Talent

    spawner::pet_spawner_t<pets::affliction::darkglare_t, warlock_t> darkglare;

    spawner::pet_spawner_t<pets::demonology::dreadstalker_t, warlock_t> dreadstalkers;
    spawner::pet_spawner_t<pets::demonology::vilefiend_t, warlock_t> vilefiends;
    spawner::pet_spawner_t<pets::demonology::demonic_tyrant_t, warlock_t> demonic_tyrants;
    spawner::pet_spawner_t<pets::demonology::grimoire_felguard_pet_t, warlock_t> grimoire_felguards;

    spawner::pet_spawner_t<pets::demonology::wild_imp_pet_t, warlock_t> wild_imps;
    // DF - New Wild Imp variant - Imp Gang Boss


    // DF - Soulkeeper and Inquisitor Eye are not guardians (Bilescourge Bombers/Arcane Familiar are more appropriate matches, respectively)

    // DF - Nether Portal demons - check spawn rates
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

  //TODO: DF - Rename this section and leverage for some common purpose like 
  struct active_t
  {
    spell_t* rain_of_fire; //TODO: DF - This is the definition for the ground aoe event, how is it used?
    spell_t* bilescourge_bombers; //TODO: DF - This is the definition for the ground aoe event, how is it used?
    spell_t* summon_random_demon; //TODO: DF - This is the definition for a helper action for Nether Portal, does it belong here?
  } active;

  // DF - Does everything go in this struct? Probably yes, though spell_data_t could be replaced with player_talent_t
  // Talents
  struct talents_t
  {
    // Shared
    player_talent_t grimoire_of_sacrifice; // Aff/Destro only
    const spell_data_t* grimoire_of_sacrifice_buff; // 1 hour duration, enables proc functionality, canceled if pet summoned
    const spell_data_t* grimoire_of_sacrifice_proc; // Damage data is here, but RPPM of proc trigger is in buff data
    player_talent_t grand_warlocks_design; // One spell data for all 3 specs

    // Class Tree
    const spell_data_t* soul_conduit; // DF - Verify unchanged other than in class tree now
    // DF - Demonic Embrace is a stamina talent, may be irrelevant now
    // DF - Demonic Inspiration (Pet haste on soul shard fill)
    // DF - Wrathful Minion (Pet damage on soul shard fill)
    // DF - Demonic Fortitude is a health talent, may be irrelevant now
    // DF - Grimoire of Synergy (moved from SL Legendary power)
    // DF - Claw of Endereth (moved from SL Legendary power)
    // DF - Summon Soulkeeper (Active ground aoe which spends hidden stacking buff)
    // DF - Inquisitor's Gaze (Non-guardian pet summon which behaves like Arcane Familiar)

    // AFF
    player_talent_t malefic_rapture;
    const spell_data_t* malefic_rapture_dmg; // Damage events use this ID, but primary talent contains the spcoeff

    player_talent_t unstable_affliction;
    const spell_data_t* unstable_affliction_2; // Soul Shard on demise, still seems to be separate spell (auto-learned on spec switch?)
    const spell_data_t* unstable_affliction_3; // +5 seconds to duration, still seems to be separate spell (auto-learned on spec switch?)
    player_talent_t seed_of_corruption;
    const spell_data_t* seed_of_corruption_aoe; // Explosion damage when Seed ticks

    player_talent_t nightfall; //TODO: RNG information is missing from spell data. Confirmed 2022-09-22: Buff stacks to 2, can refresh at max stacks, duration refreshes when proc occurs
    const spell_data_t* nightfall_buff;
    player_talent_t xavian_teachings; // Instant cast data in this spell, talent points to base Corruption spell (172) for the direct damage
    player_talent_t sow_the_seeds;

    player_talent_t shadow_embrace;
    const spell_data_t* shadow_embrace_debuff; // Default values set from talent data, but contains debuff info
    player_talent_t harvester_of_souls;
    const spell_data_t* harvester_of_souls_dmg; // Talent only controls proc, damage is in separate spell
    player_talent_t writhe_in_agony; // 2022-09-23 DF Beta - Writhe In Agony's second point is not registering in-game
    player_talent_t agonizing_corruption; // Only applies to targets which already have Agony

    player_talent_t drain_soul; // This represents the talent node but not much else. 2022-09-25 Some weird behavior with Nightfall has been documented.
    const spell_data_t* drain_soul_dot; // This is the previous talent spell, contains all channel data
    player_talent_t absolute_corruption; // DF - Now a choice against Siphon Life
    player_talent_t siphon_life; // DF - Now a choice against Absolute Corruption
    player_talent_t phantom_singularity; // DF - Now a choice against Vile Taint
    const spell_data_t* phantom_singularity_tick; // Actual AoE spell information in here
    player_talent_t vile_taint; // Base talent, AoE cast data
    const spell_data_t* vile_taint_dot; // DoT data

    player_talent_t soul_tap; // Sacrifice Soul Leech for Soul Shard. TODO: Add controls to limit usage
    player_talent_t inevitable_demise; // The talent version of the ability
    const spell_data_t* inevitable_demise_buff; // The buff version referenced by the talent tooltip
    player_talent_t soul_swap; // Spend Soul Shard to apply core dots (Corruption, Agony, UA)
    const spell_data_t* soul_swap_ua; // Separate copy of Unstable Affliction data, since UA is applied even without the talent
    player_talent_t soul_flame; // AoE damage on kills
    const spell_data_t* soul_flame_proc; // The actual spell damage data
    // Grimoire of Sacrifice (shared with Destruction)
    
    player_talent_t pandemic_invocation; // Late DoT refresh deals damage and has Soul Shard chance
    const spell_data_t* pandemic_invocation_proc; // Damage data
    player_talent_t withering_bolt; // Increased damage on Shadow Bolt/Drain Soul based on active DoT count on target
    player_talent_t sacrolashs_dark_strike; // Increased Corruption ticking damage, and ticks extend Curses (not implemented)

    player_talent_t creeping_death; // DF - No long reduces duration
    player_talent_t haunt;
    player_talent_t summon_darkglare; 
    player_talent_t soul_rot; // DF - now Affliction only

    player_talent_t malefic_affliction; // Stacking damage increase to Unstable Affliction until UA is cancelled/swapped/ends
    const spell_data_t* malefic_affliction_debuff; // Target debuff applied on Malefic Rapture casts
    player_talent_t tormented_crescendo; // Free, instant Malefic Rapture procs from Shadow Bolt/Drain Soul
    const spell_data_t* tormented_crescendo_buff;
    player_talent_t seized_vitality; // Additional Haunt damage
    player_talent_t malevolent_visionary; // Longer Darkglare and more damage scaling
    player_talent_t wrath_of_consumption; // DoT damage buff on target deaths
    const spell_data_t* wrath_of_consumption_buff;
    player_talent_t souleaters_gluttony; // Soul Rot CDR from Unstable Affliction

    player_talent_t doom_blossom; // Damage proc on Corruption ticks based on Malefic Affliction stacks
    const spell_data_t* doom_blossom_proc;
    player_talent_t dread_touch; // Increased DoT damage based on Malefic Affliction procs
    const spell_data_t* dread_touch_debuff; // Applied to target when Dread Touch procs
    player_talent_t haunted_soul; // Haunt increase ALL DoT damage while active
    const spell_data_t* haunted_soul_buff; // Applied to player while Haunt is active
    // Grand Warlock's Design (formerly Wilfred's). Shared across all 3 specs
    player_talent_t grim_reach; // Darkglare hits all targets affected by DoTs
    player_talent_t dark_harvest; // Buffs from hitting targets with Soul Rot (Formerly Decaying Soul Satchel)
    const spell_data_t* dark_harvest_buff; // TOCHECK: As of 2022-10-01, buff data has a different value from talent tooltip, and neither is correct?

    // DEMO
    player_talent_t call_dreadstalkers;
    const spell_data_t* call_dreadstalkers_2; // Contains duration data

    player_talent_t demonbolt; // Note: Demonic Core is a guaranteed passive, even though this talent is optional
    player_talent_t dreadlash;
    player_talent_t annihilan_training; // Permanent aura on Felguard that gives 10% damage buff
    const spell_data_t* annihilan_training_buff; // Applied to pet, not player

    player_talent_t demonic_knowledge; // Demonic Core chance on Hand of Gul'dan cast
    player_talent_t summon_vilefiend;
    player_talent_t soul_strike;
    player_talent_t bilescourge_bombers;
    const spell_data_t* bilescourge_bombers_aoe; // Ground AoE data
    player_talent_t demonic_strength;
    player_talent_t from_the_shadows;
    const spell_data_t* from_the_shadows_debuff; // Tooltip says "Shadowflame" but this contains an explicit whitelist (for the *warlock*, pets are unknown and we'll fall back to schools)

    player_talent_t implosion;
    const spell_data_t* implosion_aoe; // Note: in combat logs this is attributed to the player, not the imploding pet
    player_talent_t shadows_bite; // Demonbolt damage increase after Dreadstalkers despawn
    const spell_data_t* shadows_bite_buff;
    player_talent_t carnivorous_stalkers; // Chance for Dreadstalkers to perform additional Dreadbites
    player_talent_t fel_and_steel; // Felstorm and Dreadbite damage increase
    player_talent_t fel_might; // Shorter Felstorm CD - main pet only!

    player_talent_t power_siphon; // NOTE: Power Siphon WILL consume Imp Gang Boss as if it were a regular imp (last checked 2022-10-04)
    const spell_data_t* power_siphon_buff; // Semi-hidden aura that controls the bonus Demonbolt damage
    player_talent_t inner_demons; // DF - Now a 2 point talent
    player_talent_t demonic_calling; // DF - Now a 2 point talent
    const spell_data_t* demonic_calling_buff;
    player_talent_t grimoire_felguard;

    player_talent_t bloodbound_imps; // Increased Demonic Core proc chance from Wild Imps
    player_talent_t dread_calling; // Stacking buff to next Dreadstalkers damage (Formerly SL Legendary)
    const spell_data_t* dread_calling_buff; // This buffs stacks on the warlock, a different one applies to the pet
    player_talent_t doom;
    player_talent_t demonic_meteor; // Increased Hand of Gul'dan damage and chance to refund soul shard
    player_talent_t fel_sunder; // Increase damage taken debuff when hit by main pet Felstorm
    const spell_data_t* fel_sunder_debuff;

    player_talent_t fel_covenant; // Stacking Demonbolt buff when casting Shadow Bolt, not consumed after cast
    const spell_data_t* fel_covenant_buff;
    player_talent_t imp_gang_boss; // DF - Imp Gang Boss (2 point talent, Wild Imp has chance to be this pet instead)
    player_talent_t kazaaks_final_curse; // Doom deals increased damage based on active demon count
    player_talent_t ripped_through_the_portal; // Increased Dreadstalker count chance (Formerly SL Tier bonus)
    player_talent_t hounds_of_war; // Shadow Bolt and Demonbolt have a chance to reset Call Dreadstalkers
    
    player_talent_t nether_portal; // TOCHECK: 2022-10-07 Portal summon damage is possibly slightly above current in-game values (~1% max), full audit needed closer to release
    const spell_data_t* nether_portal_buff; // Aura on player while the portal is active
    player_talent_t summon_demonic_tyrant; // TOCHECK: 2022-10-07 Pit Lord is not currently extendable by Tyrant
    const spell_data_t* demonic_power_buff; // Buff on player when Tyrant is summoned, should grant 15% damage to pets (TOCHECK: 2022-10-07 - buff is missing guardian aura, only main pet working)
    player_talent_t antoran_armaments; // Increased Felguard damage and Soul Strike cleave (TOCHECK: 2022-10-08 - this is applying to Grimoire: Felguard erratically)

    player_talent_t nerzhuls_volition; // Chance to summon additional demon from Nether Portal summons (TOCHECK: It is currently assumed this cannot proc itself)
    player_talent_t stolen_power; // Stacking buff from Wild Imps, at max get increased Shadow Bolt or Demonbolt damage
    const spell_data_t* stolen_power_stacking_buff; // Triggers final buff when reaching max stacks
    const spell_data_t* stolen_power_final_buff;
    player_talent_t sacrificed_souls;
    player_talent_t soulbound_tyrant; // Soul Shards on Tyrant summons
    player_talent_t pact_of_the_imp_mother; // Chance for Hand of Gul'dan to proc a second time on execute (Formerly SL Legendary)
    player_talent_t the_expendables; // Per-pet stacking buff to damage when a Wild Imp expires
    player_talent_t infernal_command; // Increased Wild Imp and Dreadstalker damage while Felguard active (TOCHECK: 2022-10-08 - this is behaving very buggily on beta)

    player_talent_t guldans_ambition; // Summons a Pit Lord at end of Nether Portal
    const spell_data_t* soul_glutton; // Buff on Pit Lord based on demons summoned
    player_talent_t reign_of_tyranny; // Each summoned active gives stacks of Demonic Servitude (Wild Imps give 1, others 2). Tyrant snapshots this buff on summon for more damage
    const spell_data_t* demonic_servitude; // TOCHECK: 2022-10-09 - In addition to aura stack bugs, Nether Portal demons are not currently giving stacks in beta (not implemented)
    // Grand Warlock's Design (formerly Wilfred's). Shared across all 3 specs
    player_talent_t guillotine;

    // DESTRO
    player_talent_t chaos_bolt;

    player_talent_t conflagrate; // Base 2 charges
    const spell_data_t* conflagrate_2; // Contains Soul Shard information
    player_talent_t reverse_entropy;
    const spell_data_t* reverse_entropy_buff;
    player_talent_t internal_combustion;
    player_talent_t rain_of_fire;
    const spell_data_t* rain_of_fire_tick;

    player_talent_t backdraft;
    const spell_data_t* backdraft_buff; // DF - Now affects Soul Fire
    player_talent_t mayhem; // It appears that the only spells that can proc Mayhem are ones that can be Havoc'd
    player_talent_t havoc; // Talent data for Havoc is both the debuff and the action
    const spell_data_t* havoc_debuff; // This is a second copy of the talent data for use in places that are shared by Havoc and Mayhem
    player_talent_t pyrogenics; // Enemies affected by Rain of Fire receive debuff for increased Fire damage
    const spell_data_t* pyrogenics_debuff;

    player_talent_t roaring_blaze;
    const spell_data_t* conflagrate_debuff; // Formerly called Roaring Blaze
    player_talent_t improved_conflagrate; // +1 charge for Conflagrate
    player_talent_t explosive_potential; // Reduces base Conflagrate cooldown by 2 seconds
    player_talent_t channel_demonfire;
    const spell_data_t* channel_demonfire_tick;
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
    // DF - Flashpoint (2 point talent, stacking haste buff from Immolate ticks on high-health targets)
    // DF - Scalding Flames (2 point talent, increased Immolate damage)

    // DF - Ruin (2 point talent, damage increase to several spells)
    const spell_data_t* eradication; // DF - Now a 2 point talent
    // DF - Ashen Remains (2 point talent, formerly SL Conduit)
    // Grimoire of Sacrifice (shared with Affliction)

    // DF - Summon Infernal
    // DF - Embers of the Diabolic (formerly SL Legendary)
    // DF - Ritual of Ruin (formerly SL Tier Bonus, functionality slightly modified)
    
    // DF - Crashing Chaos (2 point talent, Summon Infernal reduces cost of next X casts)
    // DF - Infernal Brand (2 point talent, formerly SL Conduit)
    // DF - Power Overwhelming (2 point talent, stacking mastery buff for spending Soul Shards)
    // DF - Madness of the Azj'aqir (2 point talent, formerly SL Legendary, now applies to more spells)
    // DF - Master Ritualist (2 point talent, reduces proc cost of Ritual of Ruin)
    // DF - Burn to Ashes (2 point talent, Chaos Bolt and Rain of Fire increase damage of next 2 Incinerates)

    const spell_data_t* rain_of_chaos; // DF - Now a choice against Wilfred's, check deck of cards RNG
    // DF - Wilfred's Sigil of Superior Summoning (Choice against Rain of Chaos, formerly SL Legendary, NOTE: SHARES NAME WITH OTHER SPEC TALENTS)
    // DF - Chaos Incarnate (Choice against Dimensional Rift, maximum mastery value for some spells)
    // DF - Dimensional Rift (Choice against Chaos Incarnate, charge cooldown instant spell which deals damage and grants fragments)
    // DF - Avatar of Destruction (Formerly SL Tier Bonus, summons Blasphemy when consuming Ritual of Ruin)
  } talents;

  struct proc_actions_t
  {
    action_t* soul_flame_proc;
    action_t* pandemic_invocation_proc;
    action_t* bilescourge_bombers_aoe_tick;
    action_t* summon_random_demon; // Nether Portal and Inner Demons
    action_t* rain_of_fire_tick;
  } proc_actions;

  // DF - This struct will be retired, need to determine if needed for pre-patch
  struct legendary_t
  {
    // Legendaries
    // Cross-spec
    item_runeforge_t claw_of_endereth; // DF - Now class talent
    item_runeforge_t relic_of_demonic_synergy; // DF - Now class talent
    item_runeforge_t wilfreds_sigil_of_superior_summoning; // DF - Now a talent in all 3 spec trees
    // Affliction
    item_runeforge_t sacrolashs_dark_strike; // DF - Now an Affliction talent
    item_runeforge_t wrath_of_consumption; // DF - Now an Affliction talent
    // Demonology
    item_runeforge_t balespiders_burning_core; // DF - Now a Demonology talent
    item_runeforge_t forces_of_the_horned_nightmare; // DF - Now a Demonology talent
    item_runeforge_t grim_inquisitors_dread_calling; // DF - Now a Demonology talent
    // Destruction
    item_runeforge_t cinders_of_the_azjaqir; // DF - Reworked into Improved Conflagrate
    item_runeforge_t embers_of_the_diabolic_raiment; // DF - Now a Destruction talent
    item_runeforge_t madness_of_the_azjaqir; // DF - Now a Destruction talent
    // Covenant
    item_runeforge_t decaying_soul_satchel; // DF - Now an Affliction talent
  } legendary;

  // DF - This struct will be retired, need to determine if needed for pre-patch
  struct conduit_t
  {
    // Conduits
    // Affliction
    conduit_data_t withering_bolt; // DF - Now an Affliction talent
    // Demonology
    conduit_data_t borne_of_blood; // DF - Now a Demonology talent
    conduit_data_t carnivorous_stalkers; // DF - Now a Demonology talent
    conduit_data_t fel_commando; // DF - Now a Demonology talent
    // Destruction
    conduit_data_t ashen_remains; // DF - Now a Destruction talent
    conduit_data_t infernal_brand; // DF - Now a Destruction talent
  } conduit;

  // DF - This struct will be retired, need to determine if needed for pre-patch
  struct covenant_t
  {
    // Covenant Abilities
    const spell_data_t* soul_rot;               // DF - Now an Affliction talent
  } covenant;

  // DF - To review while implementing talents for new additions
  // Cooldowns - Used for accessing cooldowns outside of their respective actions, such as reductions/resets
  struct cooldowns_t
  {
    propagate_const<cooldown_t*> haunt;
    propagate_const<cooldown_t*> phantom_singularity;
    propagate_const<cooldown_t*> darkglare;
    propagate_const<cooldown_t*> demonic_tyrant;
    propagate_const<cooldown_t*> infernal;
    propagate_const<cooldown_t*> shadowburn;
    propagate_const<cooldown_t*> soul_rot;
    propagate_const<cooldown_t*> call_dreadstalkers;
    propagate_const<cooldown_t*> soul_fire;
  } cooldowns;

  // DF - Retire this section, combine remnants with the mastery_spells struct above in a "core" or "base" spells section
  struct specs_t
  {
    // Affliction only
    const spell_data_t* corruption_2; // DF - Baked into Xavian Teachings talent
    const spell_data_t* corruption_3; // DF - Baked into Xavian Teachings talent
    const spell_data_t* summon_darkglare; // DF - Now an Affliction talent
    const spell_data_t* summon_darkglare_2; // DF - Baked into Affliction talent (2 minute cooldown)

    // Demonology only
    const spell_data_t* call_dreadstalkers_2; // DF - Partially baked in to Demonology talent (Cast time reduction REMOVED, leap ability retained)
    const spell_data_t* fel_firebolt_2; // DF - Baked into base Wild Imp behavior (Fel Firebolt energy cost reduction of 20%)
    const spell_data_t* summon_demonic_tyrant_2; // DF - Baked into Soulbound Tyrant talent

    // Destruction only
    const spell_data_t* conflagrate; // DF - Now a Destruction talent (base 2 charges)
    const spell_data_t* conflagrate_2; // DF - Baked into Conflagrate talent (used to be 1->2 charges)
    const spell_data_t* havoc; // DF - Now a Destruction talent
    const spell_data_t* havoc_2; // DF - Baked into Havoc talent (12 second total duration)
    const spell_data_t* rain_of_fire_2; // DF - Should be irrelevant now (used to be increased Rain of Fire damage)
    const spell_data_t* summon_infernal_2; // DF - Should be irrelevant now (used to be increased impact damage)
  } spec;

  // DF - Many new effects to be added here as talents are implemented
  // Buffs
  struct buffs_t
  {
    propagate_const<buff_t*> demonic_power; //Buff from Summon Demonic Tyrant (increased demon damage + duration)
    propagate_const<buff_t*> grimoire_of_sacrifice; // Buff which grants damage proc
    // DF - Summon Soulkeeper has a hidden stacking buff
    // DF - Determine if dummy buff should be added for Inquisitor's Eye
    propagate_const<buff_t*> demonic_synergy; // DF - Now comes from Class talent

    // Affliction Buffs
    propagate_const<buff_t*> drain_life; //Dummy buff used internally for handling Inevitable Demise cases
    propagate_const<buff_t*> nightfall;
    propagate_const<buff_t*> inevitable_demise;
    propagate_const<buff_t*> calamitous_crescendo;
    propagate_const<buff_t*> soul_rot; // DF - Now Affliction only. Buff for determining if Drain Life is zero cost and aoe.
    propagate_const<buff_t*> wrath_of_consumption; // DF - Now comes from Affliction talent.
    propagate_const<buff_t*> decaying_soul_satchel_haste; // DF - Now comes from Affliction talent
    propagate_const<buff_t*> decaying_soul_satchel_crit; // These are one unified buff in-game but splitting them in simc to make it easier to apply stat pcts
    propagate_const<buff_t*> tormented_crescendo;
    propagate_const<buff_t*> haunted_soul;
    propagate_const<buff_t*> dark_harvest_haste; // One buff in game...
    propagate_const<buff_t*> dark_harvest_crit; // ...but split into two in simc for better handling

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
    propagate_const<buff_t*> grimoire_felguard; //Buff for tracking if GFG pet is currently out
    propagate_const<buff_t*> prince_malchezaar; //Buff for tracking Malchezaar (who is currently disabled in sims)
    propagate_const<buff_t*> eyes_of_guldan; //Buff for tracking if rare random summon is currently out
    propagate_const<buff_t*> dread_calling; // DF - Now comes from Demonology talent
    propagate_const<buff_t*> balespiders_burning_core; // DF - Now comes from Demonology talent
    propagate_const<buff_t*> shadows_bite;
    propagate_const<buff_t*> fel_covenant;
    propagate_const<buff_t*> stolen_power_building; // Stacking buff, triggers final buff as a separate buff at max stacks
    propagate_const<buff_t*> stolen_power_final;
    propagate_const<buff_t*> nether_portal_total; // Dummy buff. Used for Gul'dan's Ambition as the counter to trigger Soul Gluttony
    propagate_const<buff_t*> demonic_servitude; // From Reign of Tyranny talent

    // Destruction Buffs
    propagate_const<buff_t*> backdraft; // DF - Max 2 stacks
    propagate_const<buff_t*> reverse_entropy;
    propagate_const<buff_t*> rain_of_chaos;
    propagate_const<buff_t*> impending_ruin; // DF - Impending Ruin and Ritual of Ruin now come from Destruction talent
    propagate_const<buff_t*> ritual_of_ruin;
    propagate_const<buff_t*> madness_of_the_azjaqir; // DF - Now comes from Destruction talent
    propagate_const<buff_t*> rolling_havoc;
    propagate_const<buff_t*> conflagration_of_chaos_cf;
    propagate_const<buff_t*> conflagration_of_chaos_sb;
    // DF - Flashpoint (stacking haste from Immolate ticks)
    // DF - Crashing Chaos (cost reduction after Infernal summon)
    // DF - Power Overwhelming (stacking mastery when spending Soul Shards)
    // DF - Burn to Ashes (increased Incinerate damage after Chaos Bolt/Rain of Fire)
    // DF - Chaos Incarnate? (passive max mastery on certain spells)
  } buffs;

  //TODO: Determine if any gains are not currently being tracked
  // Gains - Many of these are automatically handled for resource gains if get_gain( name ) is given the same name as the action source
  struct gains_t
  {
    gain_t* soul_conduit;

    gain_t* agony;
    gain_t* drain_soul;
    gain_t* unstable_affliction_refund;
    gain_t* soul_tap;
    gain_t* pandemic_invocation;

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

    gain_t* miss_refund;

    gain_t* shadow_bolt;
    gain_t* doom;
    gain_t* soulbound_tyrant;
    gain_t* demonic_meteor;
  } gains;

  // Procs
  struct procs_t
  {
    proc_t* soul_conduit;

    // aff
    proc_t* nightfall;
    proc_t* calamitous_crescendo;
    std::array<proc_t*, 8> malefic_rapture; // This length should be at least equal to the maximum number of Affliction DoTs that can be active on a target.
    proc_t* harvester_of_souls;
    proc_t* pandemic_invocation_shard;
    proc_t* tormented_crescendo;
    proc_t* doom_blossom;

    // demo
    proc_t* demonic_knowledge;
    proc_t* demonic_calling;
    proc_t* one_shard_hog;
    proc_t* two_shard_hog;
    proc_t* three_shard_hog;
    proc_t* summon_random_demon;
    proc_t* portal_summon;
    proc_t* carnivorous_stalkers; // DF - Now a Demonology talent
    proc_t* horned_nightmare; // DF - Now a Demonology talent
    proc_t* demonic_meteor;
    proc_t* imp_gang_boss;
    proc_t* hounds_of_war;
    proc_t* nerzhuls_volition;
    proc_t* pact_of_the_imp_mother;

    // destro
    proc_t* reverse_entropy;
    proc_t* rain_of_chaos;
    proc_t* ritual_of_ruin;
    proc_t* avatar_of_destruction;
    proc_t* mayhem;
    proc_t* conflagration_of_chaos_cf;
    proc_t* conflagration_of_chaos_sb;
  } procs;

  int initial_soul_shards;
  std::string default_pet;
  shuffled_rng_t* rain_of_chaos_rng;
  // DF - Possibly add pre-patch spell for automatic switchover for DF

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
  void init_special_effects() override;
  void reset() override;
  void create_options() override;
  int get_spawning_imp_count();
  timespan_t time_to_imps( int count );
  int imps_spawned_during( timespan_t period );
  void darkglare_extension_helper( warlock_t* p, timespan_t darkglare_extension );
  int active_demon_count() const;
  void expendables_trigger_helper( warlock_pet_t* source );
  bool min_version_check( version_check_e version ) const;
  action_t* pass_corruption_action( warlock_t* p ); // Horrible, horrible hack for getting Corruption in Aff module until things are re-merged
  bool crescendo_check( warlock_t* p ); 
  void create_actions() override;
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
  double composite_rating_multiplier( rating_e rating ) const override;
  void invalidate_cache( cache_e ) override;
  double composite_mastery() const override;
  double composite_spell_crit_chance() const override;
  double composite_melee_crit_chance() const override;
  double resource_regen_per_second( resource_e ) const override;
  double composite_attribute_multiplier( attribute_e attr ) const override;
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
  void create_all_pets();
  std::unique_ptr<expr_t> create_pet_expression( util::string_view name_str );
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
  bool can_havoc; // DF - Also need to utilize this for Mayhem

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
      // lets try making all lock specs not react instantly to shard gen
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
      m *= 1.0 + td( t )->debuffs_dread_touch->check_stack_value();

    return m;
  }

  double action_multiplier() const override
  {
    double pm = spell_t::action_multiplier();

    pm *= 1.0 + p()->buffs.demonic_synergy->check_stack_value();

    return pm;
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

  void extend_dot( dot_t* dot, timespan_t extend_duration )
  {
    if ( dot->is_ticking() )
    {
      dot->adjust_duration( extend_duration, dot->current_action->dot_duration * 1.5 );
    }
  }

  //Destruction specific things for Havoc/Mayhem

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

    // 2022-09-30 - It seems like the damage is either double dipping on the spec aura effect, or is benefiting from pet damage multiplier as well
    // Using the latter for now as that seems *slightly* less silly
    if ( p->specialization() == WARLOCK_AFFLICTION )
      base_dd_multiplier *= 1.0 + p->warlock_base.affliction_warlock->effectN( 3 ).percent();
    if ( p->specialization() == WARLOCK_DESTRUCTION )
      base_dd_multiplier *= 1.0 + p->warlock_base.destruction_warlock->effectN( 3 ).percent();
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
    if ( owner->warlock_pet_list.active )
    {
      auto pet = owner->warlock_pet_list.active;
      //Always set the pet's buff value using the owner's to ensure specialization value is correct
      pet->buffs.demonic_synergy->trigger( 1, owner->buffs.demonic_synergy->default_value );
    }
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
