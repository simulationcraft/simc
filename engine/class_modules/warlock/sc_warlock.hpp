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

// Finds an action with the given name. If no action exists, a new one will
// be created.
//
// Use this with secondary background actions to ensure the player only has
// one copy of the action.
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
  propagate_const<dot_t*> dots_phantom_singularity;
  propagate_const<dot_t*> dots_unstable_affliction;
  propagate_const<dot_t*> dots_vile_taint;
  propagate_const<dot_t*> dots_drain_life_aoe; // Soul Rot effect
  propagate_const<dot_t*> dots_soul_rot;

  propagate_const<buff_t*> debuffs_haunt;
  propagate_const<buff_t*> debuffs_shadow_embrace;
  propagate_const<buff_t*> debuffs_infirmity;

  // Demo
  propagate_const<buff_t*> debuffs_wicked_maw;
  propagate_const<buff_t*> debuffs_fel_sunder; // Done in owner target data for easier handling
  propagate_const<buff_t*> debuffs_doom;

  // Destro
  propagate_const<dot_t*> dots_immolate;

  propagate_const<buff_t*> debuffs_shadowburn;
  propagate_const<buff_t*> debuffs_eradication;
  propagate_const<buff_t*> debuffs_havoc;
  propagate_const<buff_t*> debuffs_pyrogenics;
  propagate_const<buff_t*> debuffs_conflagrate;

  // Diabolist
  propagate_const<buff_t*> debuffs_cloven_soul;

  // Hellcaller
  propagate_const<dot_t*> dots_wither;

  propagate_const<buff_t*> debuffs_blackened_soul; // Dummy/Hidden debuff that triggers stack collapse

  // Soul Harvester
  propagate_const<dot_t*> dots_soul_anathema;

  propagate_const<buff_t*> debuffs_shared_fate;

  double soc_threshold; // Aff - Seed of Corruption counts damage from cross-spec spells such as Drain Life

  warlock_t& warlock;
  warlock_td_t( player_t* target, warlock_t& p );

  void reset()
  { soc_threshold = 0; }

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
  std::vector<event_t*> wild_imp_spawns; // Used for tracking incoming imps from HoG TODO: Is this still needed with faster spawns?
  int diabolic_ritual;

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
    const spell_data_t* xavian_teachings; // Passive granted only to Affliction. Instant cast data in this spell, points to base Corruption spell (172) for the direct damage
    const spell_data_t* malefic_rapture; // This contains an old sp_coeff value, but it is most likely no longer in use
    const spell_data_t* malefic_rapture_dmg;
    const spell_data_t* potent_afflictions; // Affliction Mastery - Increased DoT and Malefic Rapture damage
    const spell_data_t* affliction_warlock; // Spec aura

    // Demonology
    const spell_data_t* hand_of_guldan;
    const spell_data_t* hog_impact; // Secondary spell responsible for impact damage
    const spell_data_t* wild_imp; // Data for pet summoning
    const spell_data_t* fel_firebolt_2; // Still a separate spell (learned automatically). Reduces pet's energy cost
    const spell_data_t* master_demonologist; // Demonology Mastery - Increased demon damage
    const spell_data_t* demonology_warlock; // Spec aura

    // Destruction
    const spell_data_t* immolate; // Replaces Corruption
    const spell_data_t* immolate_old; // For some reason, the spellbook spell is now a new spell, but it points to this old one
    const spell_data_t* immolate_dot; // Primary spell data only contains information on direct damage
    const spell_data_t* incinerate; // Replaces Shadow Bolt
    const spell_data_t* incinerate_energize; // Soul Shard data is in a separate spell
    const spell_data_t* chaos_bolt;
    const spell_data_t* chaotic_energies; // Destruction Mastery - Increased spell damage with random range
    const spell_data_t* destruction_warlock; // Spec aura
  } warlock_base;

  // Main pet held in active, guardians should be handled by pet spawners.
  struct pets_t
  {
    warlock_pet_t* active;

    spawner::pet_spawner_t<pets::destruction::infernal_t, warlock_t> infernals;

    spawner::pet_spawner_t<pets::affliction::darkglare_t, warlock_t> darkglares;

    spawner::pet_spawner_t<pets::demonology::dreadstalker_t, warlock_t> dreadstalkers;
    spawner::pet_spawner_t<pets::demonology::vilefiend_t, warlock_t> vilefiends;
    spawner::pet_spawner_t<pets::demonology::demonic_tyrant_t, warlock_t> demonic_tyrants;
    spawner::pet_spawner_t<pets::demonology::grimoire_felguard_pet_t, warlock_t> grimoire_felguards;
    spawner::pet_spawner_t<pets::demonology::wild_imp_pet_t, warlock_t> wild_imps;
    spawner::pet_spawner_t<pets::demonology::doomguard_t, warlock_t> doomguards;

    spawner::pet_spawner_t<pets::destruction::shadowy_tear_t, warlock_t> shadow_rifts;
    spawner::pet_spawner_t<pets::destruction::unstable_tear_t, warlock_t> unstable_rifts;
    spawner::pet_spawner_t<pets::destruction::chaos_tear_t, warlock_t> chaos_rifts;

    spawner::pet_spawner_t<pets::destruction::overfiend_t, warlock_t> overfiends;

    spawner::pet_spawner_t<pets::diabolist::overlord_t, warlock_t> overlords;
    spawner::pet_spawner_t<pets::diabolist::mother_of_chaos_t, warlock_t> mothers;
    spawner::pet_spawner_t<pets::diabolist::pit_lord_t, warlock_t> pit_lords;

    spawner::pet_spawner_t<pets::diabolist::infernal_fragment_t, warlock_t> fragments;

    spawner::pet_spawner_t<pets::diabolist::diabolic_imp_t, warlock_t> diabolic_imps;

    pets_t( warlock_t* w );
  } warlock_pet_list;

  std::vector<std::string> pet_name_list;

  // Talents
  struct talents_t
  {
    // Class Tree

    player_talent_t demonic_inspiration; // Primary pet attack speed increase
    player_talent_t wrathful_minion; // Primary pet damage increase
    player_talent_t socrethars_guile;
    player_talent_t sargerei_technique;
    player_talent_t demonic_tactics;
    player_talent_t soul_conduit;
    player_talent_t soulburn;
    const spell_data_t* soulburn_buff; // This buff is applied after using Soulburn and prevents another usage unless cleared

    // Specializations

    // Shared
    player_talent_t grimoire_of_sacrifice; // Aff/Destro only
    const spell_data_t* grimoire_of_sacrifice_buff; // 1 hour duration, enables proc functionality, canceled if pet summoned
    const spell_data_t* grimoire_of_sacrifice_proc; // Damage data is here, but RPPM of proc trigger is in buff data
    player_talent_t summoners_embrace;

    // Affliction
    player_talent_t unstable_affliction;
    const spell_data_t* unstable_affliction_2; // Soul Shard on demise (learned automatically)
    const spell_data_t* unstable_affliction_3; // +5 seconds to duration (learned automatically)

    player_talent_t writhe_in_agony;
    player_talent_t seed_of_corruption;
    const spell_data_t* seed_of_corruption_aoe; // Explosion damage when Seed ticks

    player_talent_t dark_virtuosity; // Note: Spell data contains multiplier on Drain Soul direct damage as well, which doesn't exist
    player_talent_t absolute_corruption;
    player_talent_t siphon_life;
    player_talent_t kindled_malice;

    player_talent_t nightfall;
    const spell_data_t* nightfall_buff;
    player_talent_t volatile_agony;
    const spell_data_t* volatile_agony_aoe;

    player_talent_t improved_shadow_bolt;
    player_talent_t drain_soul; // This represents the talent node but not much else
    const spell_data_t* drain_soul_dot; // Contains all channel data
    // Summoner's Embrace (shared with Destruction)
    // Grimoire of Sacrifice (shared with Destruction)
    player_talent_t phantom_singularity;
    const spell_data_t* phantom_singularity_tick; // Actual AoE spell information in here
    player_talent_t vile_taint; // Base talent, AoE cast data
    const spell_data_t* vile_taint_dot; // DoT data

    player_talent_t haunt;
    player_talent_t shadow_embrace;
    const spell_data_t* shadow_embrace_debuff_ds; // Drain Soul applies a debuff with 4 stacks, and a 2% base value
    const spell_data_t* shadow_embrace_debuff_sb; // Shadow Bolt applies a debuff with 2 stacks, and a 4% base value
    player_talent_t sacrolashs_dark_strike; // Increased Corruption ticking damage, and ticks extend Curses (not implemented)
    player_talent_t summon_darkglare;
    const spell_data_t* eye_beam; // Darkglare pet ability
    player_talent_t cunning_cruelty; // Note: Damage formula in the tooltip indicates this is affected by Imp. Shadow Bolt and Sargerei Technique
    const spell_data_t* shadow_bolt_volley; // Proc chance is not listed on spell data. Appears to be 50% regardless of talent. Last checked 2024-07-07
    player_talent_t infirmity; // TOCHECK: Update to Beta on 2024-07-30 changed this to an AoE application, with some weird results (currently implemented)
    const spell_data_t* infirmity_debuff;

    player_talent_t improved_haunt;
    player_talent_t malediction;
    player_talent_t malevolent_visionary;
    const spell_data_t* malevolent_visionary_blast; // Damage proc sourced from player when summoning Darkglare
    player_talent_t contagion;
    player_talent_t cull_the_weak;

    player_talent_t creeping_death; 
    player_talent_t soul_rot;
    player_talent_t tormented_crescendo; // Free, instant Malefic Rapture procs from Shadow Bolt/Drain Soul
    const spell_data_t* tormented_crescendo_buff;

    player_talent_t xavius_gambit; // Unstable Affliction Damage Multiplier
    player_talent_t focused_malignancy; // Increaed Malefic Rapture damage to target with Unstable Affliction
    player_talent_t perpetual_unstability;
    const spell_data_t* perpetual_unstability_proc;
    player_talent_t malign_omen;
    const spell_data_t* malign_omen_buff;
    player_talent_t relinquished;
    player_talent_t withering_bolt; // Increased damage on Shadow Bolt/Drain Soul based on active DoT count on target
    player_talent_t improved_malefic_rapture;

    player_talent_t oblivion;
    player_talent_t deaths_embrace; // TOCHECK: Volatile Agony/Perpetual Unstability affected?
    player_talent_t dark_harvest; // Buffs from hitting targets with Soul Rot
    const spell_data_t* dark_harvest_buff;
    player_talent_t ravenous_afflictions;
    player_talent_t malefic_touch;
    const spell_data_t* malefic_touch_proc;

    // Demonology
    player_talent_t demoniac;
    const spell_data_t* demonbolt_spell;
    const spell_data_t* demonic_core_spell;
    const spell_data_t* demonic_core_buff;

    player_talent_t implosion;
    const spell_data_t* implosion_aoe; // Note: in combat logs this is attributed to the player, not the imploding pet
    player_talent_t call_dreadstalkers;
    const spell_data_t* call_dreadstalkers_2; // Contains duration data

    player_talent_t imp_gang_boss;
    const spell_data_t* imp_gang_boss_buff; // Buff on Wild Imps
    player_talent_t spiteful_reconstitution; // Increased Implosion damage and consuming Demonic Core may spawn a Wild Imp
    player_talent_t dreadlash;
    player_talent_t carnivorous_stalkers; // Chance for Dreadstalkers to perform additional Dreadbites

    player_talent_t inner_demons;
    player_talent_t soul_strike;
    const spell_data_t* soul_strike_pet;
    const spell_data_t* soul_strike_dmg;
    player_talent_t bilescourge_bombers;
    const spell_data_t* bilescourge_bombers_aoe; // Ground AoE data
    player_talent_t demonic_strength;

    player_talent_t sacrificed_souls;
    player_talent_t rune_of_shadows;
    player_talent_t imperator; // Increased critical strike chance for Wild Imps' Fel Firebolt (additive)
    player_talent_t fel_invocation;
    player_talent_t annihilan_training; // Permanent aura on Felguard that gives 10% damage buff
    const spell_data_t* annihilan_training_buff; // Applied to pet, not player
    player_talent_t shadow_invocation; // Bilescourge Bomber damage and proc
    player_talent_t wicked_maw;
    const spell_data_t* wicked_maw_debuff;

    player_talent_t power_siphon;
    const spell_data_t* power_siphon_buff; // Semi-hidden aura that controls the bonus Demonbolt damage
    player_talent_t summon_demonic_tyrant;
    const spell_data_t* demonic_power_buff;
    player_talent_t grimoire_felguard;
    const spell_data_t* grimoire_of_service; // Buff on Grimoire: Felguard

    player_talent_t the_expendables; // Per-pet stacking buff to damage when a Wild Imp expires
    const spell_data_t* the_expendables_buff;
    player_talent_t blood_invocation;
    player_talent_t umbral_blaze; // TOCHECK: What is the duration behavior on refresh?
    const spell_data_t* umbral_blaze_dot;
    player_talent_t reign_of_tyranny;
    const spell_data_t* reign_of_tyranny_buff;
    player_talent_t demonic_calling;
    const spell_data_t* demonic_calling_buff;
    player_talent_t fiendish_oblation;
    player_talent_t fel_sunder; // Increase damage taken debuff when hit by main pet Felstorm
    const spell_data_t* fel_sunder_debuff;

    player_talent_t doom;
    const spell_data_t* doom_debuff;
    const spell_data_t* doom_dmg;
    player_talent_t pact_of_the_imp_mother; // Chance for Hand of Gul'dan to proc a second time on execute
    player_talent_t summon_vilefiend;
    const spell_data_t* bile_spit;
    const spell_data_t* headbutt;
    player_talent_t dread_calling; // Stacking buff to next Dreadstalkers damage
    const spell_data_t* dread_calling_buff; // This buffs stacks on the warlock
    const spell_data_t* dread_calling_pet;
    player_talent_t antoran_armaments; // Increased Felguard damage and Soul Strike cleave
    const spell_data_t* antoran_armaments_buff;
    const spell_data_t* soul_cleave;

    player_talent_t doom_eternal;
    player_talent_t impending_doom;
    player_talent_t foul_mouth;
    player_talent_t the_houndmasters_gambit;
    const spell_data_t* houndmasters_aura; // Contains actual referenced % increase
    player_talent_t improved_demonic_tactics;
    player_talent_t demonic_brutality; // TOCHECK: Pets may not be properly benefitting from this in-game

    player_talent_t pact_of_the_eredruin;
    const spell_data_t* doomguard;
    const spell_data_t* doom_bolt;
    player_talent_t shadowtouched;
    player_talent_t mark_of_shatug;
    const spell_data_t* gloom_slash;
    player_talent_t mark_of_fharg;
    const spell_data_t* infernal_presence;
    const spell_data_t* infernal_presence_dmg;
    player_talent_t flametouched;
    player_talent_t immutable_hatred;
    const spell_data_t* immutable_hatred_proc;
    player_talent_t guillotine;
    const spell_data_t* guillotine_pet;
    const spell_data_t* fiendish_wrath_buff;
    const spell_data_t* fiendish_wrath_dmg; // TODO: Multiplier fixes for this
    const spell_data_t* fel_explosion;

    // Destruction
    player_talent_t conflagrate; // Base 2 charges
    const spell_data_t* conflagrate_2; // Energize data

    player_talent_t backdraft;
    const spell_data_t* backdraft_buff;
    player_talent_t rain_of_fire;
    const spell_data_t* rain_of_fire_tick;

    player_talent_t roaring_blaze;
    const spell_data_t* conflagrate_debuff; // Debuff associated with Roaring Blaze
    player_talent_t improved_conflagrate; // +1 charge for Conflagrate
    player_talent_t backlash; // Crit chance increase. NOT IMPLEMENTED: Instant Incinerate proc when physically attacked
    player_talent_t mayhem; // It appears that the only spells that can proc Mayhem are ones that can be Havoc'd
    player_talent_t havoc; // Talent data for Havoc is both the debuff and the action
    const spell_data_t* havoc_debuff; // This is a second copy of the talent data for use in places that are shared by Havoc and Mayhem
    player_talent_t pyrogenics; // Enemies affected by Rain of Fire receive debuff for increased Fire damage
    const spell_data_t* pyrogenics_debuff;
    player_talent_t inferno;
    player_talent_t cataclysm;

    player_talent_t indiscriminate_flames;
    player_talent_t rolling_havoc; // Increased damage buff when spells are duplicated by Mayhem/Havoc
    const spell_data_t* rolling_havoc_buff;
    player_talent_t scalding_flames; // Increased Immolate damage and duration

    player_talent_t shadowburn;
    const spell_data_t* shadowburn_2; // Contains Soul Shard energize data
    player_talent_t explosive_potential; // Reduces base Conflagrate cooldown by 2 seconds
    // Summoner's Embrace (shared with Affliction)
    // Grimoire of Sacrifice (shared with Affliction)
    player_talent_t ashen_remains; // Increased Chaos Bolt and Incinerate damage to targets afflicted by Immolate
    player_talent_t channel_demonfire;
    const spell_data_t* channel_demonfire_tick;
    const spell_data_t* channel_demonfire_travel; // Only holds travel speed

    player_talent_t blistering_atrophy;
    player_talent_t conflagration_of_chaos; // Conflagrate/Shadowburn has chance to make next cast of it a guaranteed crit TODO: Review behavior
    const spell_data_t* conflagration_of_chaos_cf; // Player buff which affects next Conflagrate
    const spell_data_t* conflagration_of_chaos_sb; // Player buff which affects next Shadowburn
    player_talent_t emberstorm;
    player_talent_t summon_infernal;
    const spell_data_t* summon_infernal_main; // Data for main infernal summoning
    const spell_data_t* infernal_awakening; // AoE on impact is attributed to the Warlock
    const spell_data_t* immolation_buff; // Buff on Infernal pet
    const spell_data_t* immolation_dmg; // Ticking AoE damage from buff
    const spell_data_t* embers; // Buff which generates Soul Shards
    const spell_data_t* burning_ember; // Energize data for Soul Shards
    player_talent_t fire_and_brimstone;
    player_talent_t flashpoint; // Stacking haste buff from Immolate ticks on high-health targets
    const spell_data_t* flashpoint_buff;
    player_talent_t raging_demonfire; // Additional Demonfire bolts and bolts extend Immolate

    player_talent_t fiendish_cruelty;
    player_talent_t eradication;
    const spell_data_t* eradication_debuff;
    player_talent_t crashing_chaos; // Summon Infernal increases the damage of next 8 Chaos Bolt or Rain of Fire casts
    const spell_data_t* crashing_chaos_buff;
    player_talent_t rain_of_chaos; // TOCHECK: Confirm RNG behavior (deck of cards) periodically
    const spell_data_t* rain_of_chaos_buff;
    const spell_data_t* summon_infernal_roc; // Contains Rain of Chaos infernal duration
    player_talent_t reverse_entropy;
    const spell_data_t* reverse_entropy_buff;
    player_talent_t internal_combustion;
    player_talent_t demonfire_mastery;

    player_talent_t devastation;
    player_talent_t ritual_of_ruin;
    const spell_data_t* impending_ruin_buff; // Stacking buff, triggers Ritual of Ruin buff at max
    const spell_data_t* ritual_of_ruin_buff;
    player_talent_t ruin; // Damage increase to several spells TODO: Review behavior

    player_talent_t soul_fire;
    const spell_data_t* soul_fire_2; // Contains Soul Shard energize data
    player_talent_t improved_chaos_bolt;
    player_talent_t burn_to_ashes; // Chaos Bolt and Rain of Fire increase damage of next 2 Incinerates
    const spell_data_t* burn_to_ashes_buff;
    player_talent_t master_ritualist; // Reduces proc cost of Ritual of Ruin
    player_talent_t power_overwhelming; // Stacking mastery buff for spending Soul Shards
    const spell_data_t* power_overwhelming_buff;
    player_talent_t diabolic_embers; // Incinerate generates more Soul Shards
    player_talent_t dimensional_rift;
    const spell_data_t* shadowy_tear_summon; // This only creates the "pet"
    const spell_data_t* shadow_barrage; // Casts Rift version of Shadow Bolt on ticks
    const spell_data_t* rift_shadow_bolt; // Separate ID from Warlock's Shadow Bolt
    const spell_data_t* unstable_tear_summon; // This only creates the "pet"
    const spell_data_t* chaos_barrage; // Triggers ticks of Chaos Barrage bolts
    const spell_data_t* chaos_barrage_tick;
    const spell_data_t* chaos_tear_summon; // This only creates the "pet"
    const spell_data_t* rift_chaos_bolt; // Separate ID from Warlock's Chaos Bolt

    player_talent_t decimation; // Crits can proc Soul Fire cooldown reset. Proc chance is not in spell data
    const spell_data_t* decimation_buff;
    player_talent_t chaos_incarnate; // Greater mastery value for some spells
    player_talent_t avatar_of_destruction; // TOCHECK: Is Overfiend benefitting from owner's Mastery?
    const spell_data_t* summon_overfiend;
    const spell_data_t* overfiend_buff; // Buff on Warlock while Overfiend is out, generates Soul Shards
    const spell_data_t* overfiend_cb; // Chaos Bolt cast by Overfiend
    player_talent_t dimension_ripper;
    player_talent_t unstable_rifts;
    const spell_data_t* dimensional_cinder;
  } talents;

  struct hero_talents_t
  {
    // Diabolist
    player_talent_t diabolic_ritual;
    const spell_data_t* ritual_overlord; // Diabolic Ritual: X buffs
    const spell_data_t* ritual_mother;
    const spell_data_t* ritual_pit_lord;
    const spell_data_t* art_overlord; // Demonic Art: X buffs
    const spell_data_t* art_mother;
    const spell_data_t* art_pit_lord;
    const spell_data_t* summon_overlord;
    const spell_data_t* summon_mother;
    const spell_data_t* summon_pit_lord;
    const spell_data_t* wicked_cleave; // Overlord
    const spell_data_t* chaos_salvo; // Mother of Chaos
    const spell_data_t* chaos_salvo_missile;
    const spell_data_t* chaos_salvo_dmg;
    const spell_data_t* felseeker; // Pit Lord
    const spell_data_t* felseeker_dmg;

    player_talent_t cloven_souls;
    const spell_data_t* cloven_soul_debuff;
    player_talent_t touch_of_rancora;
    player_talent_t secrets_of_the_coven; // TODO: Dimension Ripper?
    const spell_data_t* infernal_bolt;
    const spell_data_t* infernal_bolt_buff;

    player_talent_t cruelty_of_kerxan;
    player_talent_t infernal_machine;

    player_talent_t flames_of_xoroth; // TODO: 2024-07-25 Flames of Xoroth spell data has unexpected labels for effects 3 and 4, may be causing unintended values in game
    player_talent_t abyssal_dominion;
    const spell_data_t* abyssal_dominion_buff;
    const spell_data_t* infernal_fragmentation; // TODO: Re-check damage of Infernal Fragments
    player_talent_t gloom_of_nathreza;

    player_talent_t ruination; // TODO: Check damage and buff values closer to release, affected_by lists may be on cast spell not damage in data and could be changed later by Blizzard
    const spell_data_t* ruination_buff;
    const spell_data_t* ruination_cast;
    const spell_data_t* ruination_impact; // TODO: Demonology version appears to include a Hand of Gul'dan when summoning imps. Not currently implemented
    const spell_data_t* diabolic_imp;
    const spell_data_t* diabolic_bolt; // TODO: Socrethar's Guile?

    // Hellcaller
    player_talent_t wither;
    const spell_data_t* wither_direct; // TODO: Damage values are picking up some other weird effects similar to Flames of Xoroth. Check damage again after main implementation work is done
    const spell_data_t* wither_dot; // TODO: In-game, Affliction is picking up the Socrethar's Guile effect, which is almost certainly a bug

    player_talent_t xalans_ferocity; // TODO: This has similar issues to Flames of Xoroth. Is/should this be affecting pets?
    player_talent_t blackened_soul;
    const spell_data_t* blackened_soul_trigger; // Contains interval for stack collapse
    const spell_data_t* blackened_soul_dmg;
    player_talent_t xalans_cruelty; // TODO: Same concerns as Xalan's Ferocity

    player_talent_t hatefury_rituals;
    player_talent_t bleakheart_tactics;

    player_talent_t mark_of_xavius; // TODO: This is almost certainly bugged and applying to Wither for Affliction
    player_talent_t seeds_of_their_demise; // TODO: This still has data buffing Blackened Soul
    player_talent_t mark_of_perotharn;

    player_talent_t malevolence; // TODO: While buff is active this guarantees Blackened Soul, but this could be leftover from earlier versions
    const spell_data_t* malevolence_buff;
    const spell_data_t* malevolence_dmg;

    // Soul Harvester
    player_talent_t demonic_soul;
    const spell_data_t* succulent_soul; // Buff triggered by Demonic Soul proc
    const spell_data_t* demonic_soul_dmg;
    
    player_talent_t necrolyte_teachings;
    player_talent_t soul_anathema;
    const spell_data_t* soul_anathema_dot;
    player_talent_t demoniacs_fervor;

    player_talent_t shared_fate;
    const spell_data_t* shared_fate_debuff;
    const spell_data_t* shared_fate_dmg;
    player_talent_t feast_of_souls;

    player_talent_t wicked_reaping;
    const spell_data_t* wicked_reaping_dmg;
    player_talent_t quietus;
    player_talent_t sataiels_volition;

    player_talent_t shadow_of_death;
    const spell_data_t* shadow_of_death_energize;
  } hero;

  struct proc_actions_t
  {
    action_t* bilescourge_bombers_aoe_tick;
    action_t* bilescourge_bombers_proc; // From Shadow Invocation talent
    action_t* doom_proc;
    action_t* rain_of_fire_tick;
    action_t* blackened_soul;
    action_t* malevolence;
    action_t* demonic_soul;
    action_t* shared_fate;
    action_t* wicked_reaping;
  } proc_actions;

  struct tier_sets_t
  {
    // Affliction
    const spell_data_t* hexflame_aff_2pc;
    const spell_data_t* hexflame_aff_4pc;
    const spell_data_t* umbral_lattice;

    // Demonology
    const spell_data_t* hexflame_demo_2pc;
    const spell_data_t* hexflame_demo_4pc;
    const spell_data_t* empowered_legion_strike;

    // Destruction
    const spell_data_t* hexflame_destro_2pc;
    const spell_data_t* hexflame_destro_4pc;
    const spell_data_t* echo_of_the_azjaqir;
  } tier;

  // Cooldowns - Used for accessing cooldowns outside of their respective actions, such as reductions/resets
  struct cooldowns_t
  {
    propagate_const<cooldown_t*> haunt;
    propagate_const<cooldown_t*> shadowburn;
    propagate_const<cooldown_t*> soul_fire;
    propagate_const<cooldown_t*> dimensional_rift;
    propagate_const<cooldown_t*> felstorm_icd; // Shared between Felstorm, Demonic Strength, and Guillotine TODO: Actually use this!
  } cooldowns;

  // Buffs
  struct buffs_t
  {
    // Shared Buffs
    propagate_const<buff_t*> grimoire_of_sacrifice; // Buff which grants damage proc
    propagate_const<buff_t*> soulburn;
    propagate_const<buff_t*> pet_movement; // One unified buff for some form of pet movement stat tracking

    // Affliction Buffs
    propagate_const<buff_t*> nightfall;
    propagate_const<buff_t*> soul_rot; // Buff for determining if Drain Life is zero cost and aoe.
    propagate_const<buff_t*> tormented_crescendo;
    propagate_const<buff_t*> malign_omen;
    propagate_const<buff_t*> dark_harvest_haste; // One buff in game...
    propagate_const<buff_t*> dark_harvest_crit; // ...but split into two in simc for better handling
    propagate_const<buff_t*> umbral_lattice; // TWW1 4pc

    // Demonology Buffs
    propagate_const<buff_t*> demonic_core;
    propagate_const<buff_t*> power_siphon; // Hidden buff from Power Siphon that increases damage of successive Demonbolts
    propagate_const<buff_t*> demonic_calling;
    propagate_const<buff_t*> inner_demons;
    propagate_const<buff_t*> wild_imps; // Buff for tracking how many Wild Imps are currently out (does NOT include imps waiting to be spawned)
    propagate_const<buff_t*> dreadstalkers; // Buff for tracking number of Dreadstalkers currently out
    propagate_const<buff_t*> vilefiend; // Buff for tracking if Vilefiend is currently out
    propagate_const<buff_t*> tyrant; // Buff for tracking if Demonic Tyrant is currently out
    propagate_const<buff_t*> grimoire_felguard; // Buff for tracking if GFG pet is currently out
    propagate_const<buff_t*> dread_calling;

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
    propagate_const<buff_t*> burn_to_ashes;
    propagate_const<buff_t*> decimation;
    propagate_const<buff_t*> summon_overfiend;
    propagate_const<buff_t*> echo_of_the_azjaqir;

    // Diabolist Buffs
    propagate_const<buff_t*> ritual_overlord;
    propagate_const<buff_t*> ritual_mother;
    propagate_const<buff_t*> ritual_pit_lord;
    propagate_const<buff_t*> art_overlord;
    propagate_const<buff_t*> art_mother;
    propagate_const<buff_t*> art_pit_lord;
    propagate_const<buff_t*> infernal_bolt;
    propagate_const<buff_t*> abyssal_dominion;
    propagate_const<buff_t*> ruination;

    // Hellcaller Buffs
    propagate_const<buff_t*> malevolence;

    // Soul Harvester Buffs
    propagate_const<buff_t*> succulent_soul;
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

    // Demonology
    gain_t* soul_strike; // Only with Fel Invocation talent

    // Destruction
    gain_t* incinerate_crits;
    gain_t* immolate;
    gain_t* immolate_crits;
    gain_t* infernal;
    gain_t* shadowburn_refund;
    gain_t* summon_overfiend;

    // Diabolist

    // Hellcaller
    gain_t* wither;
    gain_t* wither_crits;

    // Soul Harvester
    gain_t* feast_of_souls;
    gain_t* shadow_of_death;
  } gains;

  // Procs
  struct procs_t
  {
    // Class Talents
    proc_t* soul_conduit;
    proc_t* demonic_inspiration;
    proc_t* wrathful_minion;

    // Affliction
    proc_t* nightfall;
    std::array<proc_t*, 8> malefic_rapture; // This length should be at least equal to the maximum number of Affliction DoTs that can be active on a target.
    proc_t* shadow_bolt_volley;
    proc_t* tormented_crescendo;
    proc_t* ravenous_afflictions;
    proc_t* umbral_lattice;

    // Demonology
    proc_t* demonic_calling;
    std::array<proc_t*, 4> hand_of_guldan_shards;
    proc_t* demonic_core_dogs;
    proc_t* demonic_core_imps;
    proc_t* carnivorous_stalkers;
    proc_t* shadow_invocation; // Bilescourge Bomber proc on most spells
    proc_t* imp_gang_boss;
    proc_t* spiteful_reconstitution;
    proc_t* umbral_blaze;
    proc_t* pact_of_the_imp_mother;
    proc_t* pact_of_the_eredruin;
    proc_t* empowered_legion_strike; // TWW1 4pc buff

    // Destruction
    proc_t* reverse_entropy;
    proc_t* rain_of_chaos;
    proc_t* ritual_of_ruin;
    proc_t* avatar_of_destruction;
    proc_t* mayhem;
    proc_t* conflagration_of_chaos_cf;
    proc_t* conflagration_of_chaos_sb;
    proc_t* decimation;
    proc_t* dimension_ripper;
    proc_t* echo_of_the_azjaqir;

    // Diabolist

    // Hellcaller
    proc_t* blackened_soul;
    proc_t* bleakheart_tactics;
    proc_t* seeds_of_their_demise;
    proc_t* mark_of_perotharn;

    // Soul Harvester
    proc_t* succulent_soul;
    proc_t* feast_of_souls;
  } procs;

  struct rng_settings_t
  {
    struct rng_setting_t
    {
      double setting_value;
      double default_value;
      std::string option_name;
    };

    // Affliction
    rng_setting_t cunning_cruelty_sb = { 0.50, 0.50, "cunning_cruelty_sb" };
    rng_setting_t cunning_cruelty_ds = { 0.25, 0.25, "cunning_cruelty_ds" };
    rng_setting_t agony = { 0.368, 0.368, "agony" };
    rng_setting_t nightfall = { 0.13, 0.13, "nightfall" };
    rng_setting_t umbral_lattice = { 0.30, 0.30, "umbral_lattice" };

    // Demonology
    rng_setting_t pact_of_the_eredruin = { 0.40, 0.40, "pact_of_the_eredruin" };
    rng_setting_t shadow_invocation = { 0.20, 0.20, "shadow_invocation" };
    rng_setting_t spiteful_reconstitution = { 0.30, 0.30, "spiteful_reconstitution" };
    rng_setting_t empowered_legion_strike = { 0.05, 0.05, "empowered_legion_strike" };

    // Destruction
    rng_setting_t decimation = { 0.10, 0.10, "decimation" };
    rng_setting_t dimension_ripper = { 0.05, 0.05, "dimension_ripper" };

    // Diabolist

    // Hellcaller
    rng_setting_t blackened_soul = { 0.10, 0.10, "blackened_soul" };
    rng_setting_t bleakheart_tactics = { 0.15, 0.15, "bleakheart_tactics" };
    rng_setting_t seeds_of_their_demise = { 0.15, 0.15, "seeds_of_their_demise" };
    rng_setting_t mark_of_perotharn = { 0.15, 0.15, "mark_of_perotharn" };

    // Soul Harvester
    rng_setting_t succulent_soul_aff = { 0.20, 0.20, "succulent_soul_aff" };
    rng_setting_t succulent_soul_demo = { 0.15, 0.15, "succulent_soul_demo" };
    rng_setting_t feast_of_souls = { 0.125, 0.125, "feast_of_souls" };
  } rng_settings;

  int initial_soul_shards;
  std::string default_pet;
  bool disable_auto_felstorm; // For Demonology main pet
  bool normalize_destruction_mastery;
  shuffled_rng_t* rain_of_chaos_rng;
  real_ppm_t* ravenous_afflictions_rng;

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
  void add_rng_option( warlock_t::rng_settings_t::rng_setting_t& );
  int get_spawning_imp_count(); // TODO: Decide if still needed
  timespan_t time_to_imps( int count ); // TODO: Decide if still needed
  int active_demon_count() const;
  void expendables_trigger_helper( warlock_pet_t* source ); // TODO: Move to helpers?
  bool min_version_check( version_check_e version ) const;
  void create_actions() override;
  void create_affliction_proc_actions();
  void create_demonology_proc_actions();
  void create_destruction_proc_actions();
  void create_diabolist_proc_actions();
  void create_hellcaller_proc_actions();
  void create_soul_harvester_proc_actions();
  action_t* create_action( util::string_view name, util::string_view options ) override;
  pet_t* create_pet( util::string_view name, util::string_view type = {} ) override;
  void create_pets() override;
  std::string create_profile( save_e ) override;
  void copy_from( player_t* source ) override;
  resource_e primary_resource() const override { return RESOURCE_MANA; }
  role_e primary_role() const override { return ROLE_SPELL; }
  stat_e convert_hybrid_stat( stat_e s ) const override;
  double matching_gear_multiplier( attribute_e attr ) const override;
  double composite_player_multiplier( school_e school ) const override;
  double composite_player_target_multiplier( player_t* target, school_e school ) const override;
  double composite_player_pet_damage_multiplier( const action_state_t*, bool ) const override;
  double composite_player_target_pet_damage_multiplier( player_t* target, bool guardian ) const override;
  void invalidate_cache( cache_e ) override;
  double composite_spell_crit_chance() const override;
  double composite_melee_crit_chance() const override;
  double composite_rating_multiplier( rating_e ) const override;
  void combat_begin() override;
  void init_assessors() override;
  std::unique_ptr<expr_t> create_expression( util::string_view name_str ) override;
  std::string default_potion() const override { return warlock_apl::potion( this ); }
  std::string default_flask() const override { return warlock_apl::flask( this ); }
  std::string default_food() const override { return warlock_apl::food( this ); }
  std::string default_rune() const override { return warlock_apl::rune( this ); }
  std::string default_temporary_enchant() const override { return warlock_apl::temporary_enchant( this ); }
  void apply_affecting_auras( action_t& action ) override;
  double resource_gain( resource_e resource_type, double amount, gain_t* source = nullptr, action_t* action = nullptr ) override;
  void feast_of_souls_gain();

  target_specific_t<warlock_td_t> target_data;

  const warlock_td_t* find_target_data( const player_t* target ) const override
  { return target_data[ target ]; }

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

  action_t* create_action_diabolist( util::string_view, util::string_view );
  void create_buffs_diabolist();
  void init_spells_diabolist();
  void init_gains_diabolist();
  void init_rng_diabolist();
  void init_procs_diabolist();

  action_t* create_action_hellcaller( util::string_view, util::string_view );
  void create_buffs_hellcaller();
  void init_spells_hellcaller();
  void init_gains_hellcaller();
  void init_rng_hellcaller();
  void init_procs_hellcaller();

  action_t* create_action_soul_harvester( util::string_view, util::string_view );
  void create_buffs_soul_harvester();
  void init_spells_soul_harvester();
  void init_gains_soul_harvester();
  void init_rng_soul_harvester();
  void init_procs_soul_harvester();

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
  void nightfall_updater( warlock_t* p, dot_t* d );

  void trigger_blackened_soul( warlock_t* p, bool malevolence );

}
}  // namespace warlock
