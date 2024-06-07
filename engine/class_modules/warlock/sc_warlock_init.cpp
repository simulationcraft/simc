#include "simulationcraft.hpp"

#include "sc_warlock.hpp"

namespace warlock
{
  void warlock_t::init_spells()
  {
    player_t::init_spells();

    version_10_2_0_data = find_spell( 422054 ); // For 10.2 version checking, new Shadow Invocation talent

    // Automatic requirement checking and relevant .inc file (/engine/dbc/generated/):
    // find_class_spell - active_spells.inc
    // find_specialization_spell - specialization_spells.inc
    // find_mastery_spell - mastery_spells.inc
    // find_talent_spell - ??
    //
    // If there is no need to check whether a spell is known by the actor, can fall back on find_spell

    // General
    warlock_base.nethermancy = find_spell( 86091 );
    warlock_base.drain_life = find_class_spell( "Drain Life" ); // Should be ID 234153
    warlock_base.corruption = find_class_spell( "Corruption" ); // Should be ID 172, DoT info is in Effect 1's trigger (146739)
    warlock_base.shadow_bolt = find_class_spell( "Shadow Bolt" ); // Should be ID 686, same for both Affliction and Demonology

    // Affliction
    warlock_base.agony = find_class_spell( "Agony" ); // Should be ID 980
    warlock_base.agony_2 = find_spell( 231792 ); // Rank 2, +4 to max stacks
    warlock_base.xavian_teachings = find_specialization_spell( "Xavian Teachings", WARLOCK_AFFLICTION ); // Instant cast corruption and direct damage. Direct damage is in the base corruption spell on effect 3. Should be ID 317031.
    warlock_base.potent_afflictions = find_mastery_spell( WARLOCK_AFFLICTION ); // Should be ID 77215
    warlock_base.affliction_warlock = find_specialization_spell( "Affliction Warlock", WARLOCK_AFFLICTION ); // Should be ID 137043

    // Demonology
    warlock_base.hand_of_guldan = find_class_spell( "Hand of Gul'dan" ); // Should be ID 105174
    warlock_base.hog_impact = find_spell( 86040 ); // Contains impact damage data
    warlock_base.wild_imp = find_spell( 104317 ); // Contains pet summoning information
    warlock_base.fel_firebolt_2 = find_spell( 334591 ); // 20% cost reduction for Wild Imps
    warlock_base.demonic_core = find_specialization_spell( "Demonic Core" ); // Should be ID 267102
    warlock_base.demonic_core_buff = find_spell( 264173 ); // Buff data
    warlock_base.master_demonologist = find_mastery_spell( WARLOCK_DEMONOLOGY ); // Should be ID 77219
    warlock_base.demonology_warlock = find_specialization_spell( "Demonology Warlock", WARLOCK_DEMONOLOGY ); // Should be ID 137044

    // Destruction
    warlock_base.immolate = find_class_spell( "Immolate" ); // Should be ID 348, contains direct damage and cast data
    warlock_base.immolate_dot = find_spell( 157736 ); // DoT data
    warlock_base.incinerate = find_class_spell( "Incinerate" ); // Should be ID 29722
    warlock_base.incinerate_energize = find_spell( 244670 ); // Used for resource gain information
    warlock_base.chaotic_energies = find_mastery_spell( WARLOCK_DESTRUCTION ); // Should be ID 77220
    warlock_base.destruction_warlock = find_specialization_spell( "Destruction Warlock", WARLOCK_DESTRUCTION ); // Should be ID 137046

    warlock_t::init_spells_affliction();
    warlock_t::init_spells_demonology();
    warlock_t::init_spells_destruction();

    // Talents
    talents.seed_of_corruption = find_talent_spell( talent_tree::SPECIALIZATION, "Seed of Corruption" ); // Should be ID 27243
    talents.seed_of_corruption_aoe = find_spell( 27285 ); // Explosion damage

    talents.grimoire_of_sacrifice = find_talent_spell( talent_tree::SPECIALIZATION, "Grimoire of Sacrifice" ); // Aff/Destro only. Should be ID 108503
    talents.grimoire_of_sacrifice_buff = find_spell( 196099 ); // Buff data and RPPM
    talents.grimoire_of_sacrifice_proc = find_spell( 196100 ); // Damage data

    talents.grand_warlocks_design = find_talent_spell( talent_tree::SPECIALIZATION, "Grand Warlock's Design" ); // All 3 specs. Should be ID 387084

    talents.havoc = find_talent_spell( talent_tree::SPECIALIZATION, "Havoc" ); // Should be spell 80240
    talents.havoc_debuff = find_spell( 80240 );

    talents.demonic_inspiration = find_talent_spell( talent_tree::CLASS, "Demonic Inspiration" ); // Should be ID 386858

    talents.wrathful_minion = find_talent_spell( talent_tree::CLASS, "Wrathful Minion" ); // Should be ID 386864

    talents.grimoire_of_synergy = find_talent_spell( talent_tree::CLASS, "Grimoire of Synergy" ); // Should be ID 171975
    talents.demonic_synergy = find_spell( 171982 );

    talents.socrethars_guile   = find_talent_spell( talent_tree::CLASS, "Socrethar's Guile" ); // Should be ID 405936 //405955
    talents.sargerei_technique = find_talent_spell( talent_tree::CLASS, "Sargerei Technique" );  // Should be ID 405955

    talents.soul_conduit = find_talent_spell( talent_tree::CLASS, "Soul Conduit" ); // Should be ID 215941

    talents.grim_feast = find_talent_spell( talent_tree::CLASS, "Grim Feast" ); // Should be ID 386689

    talents.summon_soulkeeper = find_talent_spell( talent_tree::CLASS, "Summon Soulkeeper" ); // Should be ID 386244
    talents.summon_soulkeeper_aoe = find_spell( 386256 );
    talents.tormented_soul_buff = find_spell( 386251 );
    talents.soul_combustion = find_spell( 386265 );

    talents.inquisitors_gaze = find_talent_spell( talent_tree::CLASS, "Inquisitor's Gaze" ); // Should be ID 386344
    talents.inquisitors_gaze_buff = find_spell( 388068 );
    talents.fel_barrage = find_spell( 388070 );

    talents.soulburn = find_talent_spell( talent_tree::CLASS, "Soulburn" ); // Should be ID 385899
    talents.soulburn_buff = find_spell( 387626 );
  }

  void warlock_t::init_spells_affliction()
  {
    // Talents
    talents.malefic_rapture = find_talent_spell( talent_tree::SPECIALIZATION, "Malefic Rapture" ); // Should be ID 324536
    talents.malefic_rapture_dmg = find_spell( 324540 ); // This spell is the ID seen in logs, but the spcoeff is in the primary talent spell

    talents.unstable_affliction = find_talent_spell( talent_tree::SPECIALIZATION, "Unstable Affliction" ); // Should be ID 316099
    talents.unstable_affliction_2 = find_spell( 231791 ); // Soul Shard on demise
    talents.unstable_affliction_3 = find_spell( 334315 ); // +5 seconds duration
  
    talents.nightfall = find_talent_spell( talent_tree::SPECIALIZATION, "Nightfall" ); // Should be ID 108558
    talents.nightfall_buff = find_spell( 264571 );

    talents.writhe_in_agony = find_talent_spell( talent_tree::SPECIALIZATION, "Writhe in Agony" ); // Should be ID 196102
  
    talents.sow_the_seeds = find_talent_spell( talent_tree::SPECIALIZATION, "Sow the Seeds" ); // Should be ID 196226

    talents.shadow_embrace = find_talent_spell( talent_tree::SPECIALIZATION, "Shadow Embrace" ); // Should be ID 32388
    talents.shadow_embrace_debuff = find_spell( 32390 );

    talents.dark_virtuosity = find_talent_spell( talent_tree::SPECIALIZATION, "Dark Virtuosity" ); // Should be ID 405327

    talents.kindled_malice = find_talent_spell( talent_tree::SPECIALIZATION, "Kindled Malice" );  // Should be ID 405330

    talents.agonizing_corruption = find_talent_spell( talent_tree::SPECIALIZATION, "Agonizing Corruption" ); // Should be ID 386922

    talents.drain_soul = find_talent_spell( talent_tree::SPECIALIZATION, "Drain Soul" ); // Should be ID 388667
    talents.drain_soul_dot = find_spell( 198590 ); // This contains all the channel data

    talents.absolute_corruption = find_talent_spell( talent_tree::SPECIALIZATION, "Absolute Corruption" ); // Should be ID 196103

    talents.siphon_life = find_talent_spell( talent_tree::SPECIALIZATION, "Siphon Life" ); // Should be ID 63106

    talents.phantom_singularity = find_talent_spell( talent_tree::SPECIALIZATION, "Phantom Singularity" ); // Should be ID 205179
    talents.phantom_singularity_tick = find_spell( 205246 ); // AoE damage info

    talents.vile_taint = find_talent_spell( talent_tree::SPECIALIZATION, "Vile Taint" ); // Should be ID 278350
    talents.vile_taint_dot = find_spell( 386931 ); // DoT info here

    talents.pandemic_invocation = find_talent_spell( talent_tree::SPECIALIZATION, "Pandemic Invocation" ); // Should be ID 386759
    talents.pandemic_invocation_proc = find_spell( 386760 ); // Proc damage data

    talents.inevitable_demise = find_talent_spell( talent_tree::SPECIALIZATION, "Inevitable Demise" ); // Should be ID 334319
    talents.inevitable_demise_buff = find_spell( 334320 ); // Buff data

    talents.soul_swap = find_talent_spell( talent_tree::SPECIALIZATION, "Soul Swap" ); // Should be ID 386951
    talents.soul_swap_ua = find_spell( 316099 ); // Unnecessary in 10.0.5 due to spell changes
    talents.soul_swap_buff = find_spell( 86211 ); // Buff data
    talents.soul_swap_exhale = find_spell( 86213 ); // Replacement action while buff active

    talents.soul_flame = find_talent_spell( talent_tree::SPECIALIZATION, "Soul Flame" ); // Should be ID 199471
    talents.soul_flame_proc = find_spell( 199581 ); // AoE damage data

    talents.focused_malignancy = find_talent_spell( talent_tree::SPECIALIZATION, "Focused Malignancy" ); // Should be ID 399668

    talents.withering_bolt = find_talent_spell( talent_tree::SPECIALIZATION, "Withering Bolt" ); // Should be ID 386976

    talents.sacrolashs_dark_strike = find_talent_spell( talent_tree::SPECIALIZATION, "Sacrolash's Dark Strike" ); // Should be ID 386986

    talents.creeping_death = find_talent_spell( talent_tree::SPECIALIZATION, "Creeping Death" ); // Should be ID 264000

    talents.haunt = find_talent_spell( talent_tree::SPECIALIZATION, "Haunt" ); // Should be ID 48181

    talents.summon_darkglare = find_talent_spell( talent_tree::SPECIALIZATION, "Summon Darkglare" ); // Should be ID 205180

    talents.soul_rot = find_talent_spell( talent_tree::SPECIALIZATION, "Soul Rot" ); // Should be ID 386997

    talents.xavius_gambit = find_talent_spell( talent_tree::SPECIALIZATION, "Xavius' Gambit" ); // Should be ID 416615

    talents.tormented_crescendo = find_talent_spell( talent_tree::SPECIALIZATION, "Tormented Crescendo" ); // Should be ID 387075
    talents.tormented_crescendo_buff = find_spell( 387079 );

    talents.seized_vitality = find_talent_spell( talent_tree::SPECIALIZATION, "Seized Vitality" ); // Should be ID 387250

    talents.malevolent_visionary = find_talent_spell( talent_tree::SPECIALIZATION, "Malevolent Visionary" ); // Should be ID 387273

    talents.wrath_of_consumption = find_talent_spell( talent_tree::SPECIALIZATION, "Wrath of Consumption" ); // Should be ID 387065
    talents.wrath_of_consumption_buff = find_spell( 387066 );

    talents.souleaters_gluttony = find_talent_spell( talent_tree::SPECIALIZATION, "Soul-Eater's Gluttony" ); // Should be ID 389630

    talents.doom_blossom = find_talent_spell( talent_tree::SPECIALIZATION, "Doom Blossom" ); // Should be ID 389764
    talents.doom_blossom_proc = find_spell( 416699 ); // AoE damage data

    talents.dread_touch = find_talent_spell( talent_tree::SPECIALIZATION, "Dread Touch" ); // Should be ID 389775
    talents.dread_touch_debuff = find_spell( 389868 ); // Applied to target on proc

    talents.haunted_soul = find_talent_spell( talent_tree::SPECIALIZATION, "Haunted Soul" ); // Should be ID 387301
    talents.haunted_soul_buff = find_spell( 387310 );

    talents.grim_reach = find_talent_spell( talent_tree::SPECIALIZATION, "Grim Reach" ); // Should be ID 389992

    talents.dark_harvest = find_talent_spell( talent_tree::SPECIALIZATION, "Dark Harvest" ); // Should be ID 387016
    talents.dark_harvest_buff = find_spell( 387018 );

    // Additional Tier Set spell data

    // T29 (Vault of the Incarnates)
    tier.cruel_inspiration = find_spell( 394215 );
    tier.cruel_epiphany = find_spell( 394253 );

    // T30 (Aberrus, the Shadowed Crucible)
    tier.infirmity = find_spell( 409765 );

    // T31 (Amirdrassil, the Dream's Hope)
    tier.umbrafire_kindling = find_spell( 423765 );
  }

  void warlock_t::init_spells_demonology()
  {
    // Talents
    talents.call_dreadstalkers = find_talent_spell( talent_tree::SPECIALIZATION, "Call Dreadstalkers" ); // Should be ID 104316
    talents.call_dreadstalkers_2 = find_spell( 193332 ); // Duration data

    talents.demonbolt = find_talent_spell( talent_tree::SPECIALIZATION, "Demonbolt" ); // Should be ID 264178

    talents.demoniac = find_talent_spell( talent_tree::SPECIALIZATION, "Demoniac" ); // Should be ID 426115
    talents.demonbolt_spell = find_spell( 264178 );
    talents.demonic_core_spell = find_spell( 267102 );
    talents.demonic_core_buff = find_spell( 264173 );

    talents.dreadlash = find_talent_spell( talent_tree::SPECIALIZATION, "Dreadlash" ); // Should be ID 264078

    talents.annihilan_training = find_talent_spell( talent_tree::SPECIALIZATION, "Annihilan Training" ); // Should be ID 386174
    talents.annihilan_training_buff = find_spell( 386176 );

    talents.demonic_knowledge = find_talent_spell( talent_tree::SPECIALIZATION, "Demonic Knowledge" ); // Should be ID 386185

    talents.summon_vilefiend = find_talent_spell( talent_tree::SPECIALIZATION, "Summon Vilefiend" ); // Should be ID 264119

    talents.soul_strike = find_talent_spell( talent_tree::SPECIALIZATION, "Soul Strike" ); // Should be ID 264057. NOTE: Updated to 428344 in 10.2

    talents.bilescourge_bombers = find_talent_spell( talent_tree::SPECIALIZATION, "Bilescourge Bombers" ); // Should be ID 267211
    talents.bilescourge_bombers_aoe = find_spell( 267213 );

    talents.demonic_strength = find_talent_spell( talent_tree::SPECIALIZATION, "Demonic Strength" ); // Should be ID 267171
  
    talents.the_houndmasters_stratagem = find_talent_spell( talent_tree::SPECIALIZATION, "The Houndmaster's Stratagem" ); // Should be ID 267170
    talents.the_houndmasters_stratagem_debuff = find_spell( 270569 );

    talents.implosion = find_talent_spell( talent_tree::SPECIALIZATION, "Implosion" ); // Should be ID 196277
    talents.implosion_aoe = find_spell( 196278 );

    talents.shadows_bite = find_talent_spell( talent_tree::SPECIALIZATION, "Shadow's Bite" ); // Should be ID 387322
    talents.shadows_bite_buff = find_spell( 272945 );

    talents.fel_invocation = find_talent_spell( talent_tree::SPECIALIZATION, "Fel Invocation" ); // Should be ID 428351

    talents.carnivorous_stalkers = find_talent_spell( talent_tree::SPECIALIZATION, "Carnivorous Stalkers" ); // Should be ID 386194

    talents.shadow_invocation = find_talent_spell( talent_tree::SPECIALIZATION, "Shadow Invocation" ); // Should be ID 422054

    talents.fel_and_steel = find_talent_spell( talent_tree::SPECIALIZATION, "Fel and Steel" ); // Should be ID 386200

    talents.heavy_handed = find_talent_spell( talent_tree::SPECIALIZATION, "Heavy Handed" ); // Should be ID 416183

    talents.power_siphon = find_talent_spell( talent_tree::SPECIALIZATION, "Power Siphon" ); // Should be ID 264130
    talents.power_siphon_buff = find_spell( 334581 );

    talents.malefic_impact = find_talent_spell( talent_tree::SPECIALIZATION, "Malefic Impact" ); // Should be ID 416341

    talents.imperator = find_talent_spell( talent_tree::SPECIALIZATION, "Imp-erator" ); // Should be ID 416230

    talents.grimoire_felguard = find_talent_spell( talent_tree::SPECIALIZATION, "Grimoire: Felguard" ); // Should be ID 111898

    talents.bloodbound_imps = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodbound Imps" ); // Should be ID 387349

    talents.spiteful_reconstitution = find_talent_spell( talent_tree::SPECIALIZATION, "Spiteful Reconstitution" ); // Should be ID 428394
  
    talents.inner_demons = find_talent_spell( talent_tree::SPECIALIZATION, "Inner Demons" ); // Should be ID 267216

    talents.doom = find_talent_spell( talent_tree::SPECIALIZATION, "Doom" ); // Should be ID 603
  
    talents.demonic_calling = find_talent_spell( talent_tree::SPECIALIZATION, "Demonic Calling" ); // Should be ID 205145
    talents.demonic_calling_buff = find_spell( 205146 );

    talents.fel_sunder = find_talent_spell( talent_tree::SPECIALIZATION, "Fel Sunder" ); // Should be ID 387399
    talents.fel_sunder_debuff = find_spell( 387402 );

    talents.umbral_blaze = find_talent_spell( talent_tree::SPECIALIZATION, "Umbral Blaze" ); // Should be ID 405798
    talents.umbral_blaze_dot = find_spell( 405802 );

    talents.imp_gang_boss = find_talent_spell( talent_tree::SPECIALIZATION, "Imp Gang Boss" ); // Should be ID 387445

    talents.kazaaks_final_curse = find_talent_spell( talent_tree::SPECIALIZATION, "Kazaak's Final Curse" ); // Should be ID 387483

    talents.dread_calling = find_talent_spell( talent_tree::SPECIALIZATION, "Dread Calling" ); // Should be ID 387391
    talents.dread_calling_buff = find_spell( 387393 );

    talents.cavitation = find_talent_spell( talent_tree::SPECIALIZATION, "Cavitation" ); // Should be ID 416154

    talents.nether_portal = find_talent_spell( talent_tree::SPECIALIZATION, "Nether Portal" ); // Should be ID 267217
    talents.nether_portal_buff = find_spell( 267218 );

    talents.summon_demonic_tyrant = find_talent_spell( talent_tree::SPECIALIZATION, "Summon Demonic Tyrant" ); // Should be ID 265187
    talents.demonic_power_buff = find_spell( 265273 );

    talents.antoran_armaments = find_talent_spell( talent_tree::SPECIALIZATION, "Antoran Armaments" ); // Should be ID 387494

    talents.nerzhuls_volition = find_talent_spell( talent_tree::SPECIALIZATION, "Ner'zhul's Volition" ); // Should be ID 387526
    talents.nerzhuls_volition_buff = find_spell( 421970 );

    talents.stolen_power = find_talent_spell( talent_tree::SPECIALIZATION, "Stolen Power" ); // Should be ID 387602
    talents.stolen_power_stacking_buff = find_spell( 387603 );
    talents.stolen_power_final_buff = find_spell( 387604 );

    talents.sacrificed_souls = find_talent_spell( talent_tree::SPECIALIZATION, "Sacrificed Souls" ); // Should be ID 267214

    talents.soulbound_tyrant = find_talent_spell( talent_tree::SPECIALIZATION, "Soulbound Tyrant" ); // Should be ID 334585

    talents.pact_of_the_imp_mother = find_talent_spell( talent_tree::SPECIALIZATION, "Pact of the Imp Mother" ); // Should be ID 387541

    talents.the_expendables = find_talent_spell( talent_tree::SPECIALIZATION, "The Expendables" ); // Should be ID 387600

    talents.infernal_command = find_talent_spell( talent_tree::SPECIALIZATION, "Infernal Command" ); // Should be ID 387549

    talents.guldans_ambition = find_talent_spell( talent_tree::SPECIALIZATION, "Gul'dan's Ambition" ); // Should be ID 387578
    talents.guldans_ambition_summon = find_spell( 387590 );
    talents.soul_glutton = find_spell( 387595 );

    talents.reign_of_tyranny = find_talent_spell( talent_tree::SPECIALIZATION, "Reign of Tyranny" ); // Should be ID 390173
    talents.demonic_servitude = find_spell( 390193 );

    talents.immutable_hatred = find_talent_spell( talent_tree::SPECIALIZATION, "Immutable Hatred" ); // Should be ID 405670

    talents.guillotine = find_talent_spell( talent_tree::SPECIALIZATION, "Guillotine" ); // Should be ID 386833

    // Additional Tier Set spell data

    // T29 (Vault of the Incarnates)
    tier.blazing_meteor = find_spell( 394776 );

    // T30 (Aberrus, the Shadowed Crucible)
    tier.rite_of_ruvaraad = find_spell( 409725 );

    // T31 (Amirdrassil, the Dream's Hope)
    tier.doom_brand = find_spell( 423585 );
    tier.doom_brand_debuff = find_spell( 423583 );
    tier.doom_brand_aoe = find_spell( 423584 );
    tier.doom_bolt_volley = find_spell( 423734 );

    // Initialize some default values for pet spawners
    warlock_pet_list.wild_imps.set_default_duration( warlock_base.wild_imp->duration() );

    warlock_pet_list.dreadstalkers.set_default_duration( talents.call_dreadstalkers_2->duration() );
  }

  void warlock_t::init_spells_destruction()
  {
    // Talents
    talents.chaos_bolt = find_talent_spell( talent_tree::SPECIALIZATION, "Chaos Bolt" ); // Should be ID 116858

    talents.conflagrate = find_talent_spell( talent_tree::SPECIALIZATION, "Conflagrate" ); // Should be ID 17962
    talents.conflagrate_2 = find_spell( 245330 );

    talents.reverse_entropy = find_talent_spell( talent_tree::SPECIALIZATION, "Reverse Entropy" ); // Should be ID 205148
    talents.reverse_entropy_buff = find_spell( 266030 );

    talents.internal_combustion = find_talent_spell( talent_tree::SPECIALIZATION, "Internal Combustion" ); // Should be ID 266134

    talents.rain_of_fire = find_talent_spell( talent_tree::SPECIALIZATION, "Rain of Fire" ); // Should be ID 5740
    talents.rain_of_fire_tick = find_spell( 42223 );

    talents.backdraft = find_talent_spell( talent_tree::SPECIALIZATION, "Backdraft" ); // Should be ID 196406
    talents.backdraft_buff = find_spell( 117828 );

    talents.mayhem = find_talent_spell( talent_tree::SPECIALIZATION, "Mayhem" ); // Should be ID 387506

    talents.pyrogenics = find_talent_spell( talent_tree::SPECIALIZATION, "Pyrogenics" ); // Should be ID 387095
    talents.pyrogenics_debuff = find_spell( 387096 );

    talents.roaring_blaze = find_talent_spell( talent_tree::SPECIALIZATION, "Roaring Blaze" ); // Should be ID 205184
    talents.conflagrate_debuff = find_spell( 265931 );

    talents.improved_conflagrate = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Conflagrate" ); // Should be ID 231793

    talents.explosive_potential = find_talent_spell( talent_tree::SPECIALIZATION, "Explosive Potential" ); // Should be ID 388827

    talents.channel_demonfire = find_talent_spell( talent_tree::SPECIALIZATION, "Channel Demonfire" ); // Should be ID 196447
    talents.channel_demonfire_tick = find_spell( 196448 ); // Includes both direct and splash damage values
    talents.channel_demonfire_travel = find_spell( 196449 );

    talents.pandemonium = find_talent_spell( talent_tree::SPECIALIZATION, "Pandemonium" ); // Should be ID 387509

    talents.cry_havoc = find_talent_spell( talent_tree::SPECIALIZATION, "Cry Havoc" ); // Should be ID 387522
    talents.cry_havoc_proc = find_spell( 387547 );

    talents.improved_immolate = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Immolate" ); // Should be ID 387093

    talents.inferno = find_talent_spell( talent_tree::SPECIALIZATION, "Inferno" ); // Should be ID 270545

    talents.cataclysm = find_talent_spell( talent_tree::SPECIALIZATION, "Cataclysm" ); // Should be ID 152108

    talents.soul_fire = find_talent_spell( talent_tree::SPECIALIZATION, "Soul Fire" ); // Should be ID 6353
    talents.soul_fire_2 = find_spell( 281490 );
  
    talents.shadowburn = find_talent_spell( talent_tree::SPECIALIZATION, "Shadowburn" ); // Should be ID 17877
    talents.shadowburn_2 = find_spell( 245731 );

    talents.raging_demonfire = find_talent_spell( talent_tree::SPECIALIZATION, "Raging Demonfire" ); // Should be ID 387166

    talents.rolling_havoc = find_talent_spell( talent_tree::SPECIALIZATION, "Rolling Havoc" ); // Should be ID 387569
    talents.rolling_havoc_buff = find_spell( 387570 );

    talents.backlash = find_talent_spell( talent_tree::SPECIALIZATION, "Backlash" ); // Should be ID 387384

    talents.fire_and_brimstone = find_talent_spell( talent_tree::SPECIALIZATION, "Fire and Brimstone" ); // Should be ID 196408

    talents.decimation = find_talent_spell( talent_tree::SPECIALIZATION, "Decimation" ); // Should be ID 387176

    talents.conflagration_of_chaos = find_talent_spell( talent_tree::SPECIALIZATION, "Conflagration of Chaos" ); // Should be ID 387108
    talents.conflagration_of_chaos_cf = find_spell( 387109 );
    talents.conflagration_of_chaos_sb = find_spell( 387110 );

    talents.flashpoint = find_talent_spell( talent_tree::SPECIALIZATION, "Flashpoint" ); // Should be 387259
    talents.flashpoint_buff = find_spell( 387263 );

    talents.scalding_flames = find_talent_spell( talent_tree::SPECIALIZATION, "Scalding Flames" ); // Should be ID 388832

    talents.ruin = find_talent_spell( talent_tree::SPECIALIZATION, "Ruin" ); // Should be ID 387103

    talents.eradication = find_talent_spell( talent_tree::SPECIALIZATION, "Eradication" ); // Should be ID 196412
    talents.eradication_debuff = find_spell( 196414 );

    talents.ashen_remains = find_talent_spell( talent_tree::SPECIALIZATION, "Ashen Remains" ); // Should be ID 387252

    talents.summon_infernal = find_talent_spell( talent_tree::SPECIALIZATION, "Summon Infernal" ); // Should be ID 1122
    talents.summon_infernal_main = find_spell( 111685 );
    talents.infernal_awakening = find_spell( 22703 );

    talents.diabolic_embers = find_talent_spell( talent_tree::SPECIALIZATION, "Diabolic Embers" ); // Should be ID 387173

    talents.ritual_of_ruin = find_talent_spell( talent_tree::SPECIALIZATION, "Ritual of Ruin" ); // Should be ID 387156
    talents.impending_ruin_buff = find_spell( 387158 );
    talents.ritual_of_ruin_buff = find_spell( 387157 );

    talents.crashing_chaos = find_talent_spell( talent_tree::SPECIALIZATION, "Crashing Chaos" ); // Should be ID 387355
    talents.crashing_chaos_buff = find_spell( 417282 );

    talents.infernal_brand = find_talent_spell( talent_tree::SPECIALIZATION, "Infernal Brand" ); // Should be ID 387475

    talents.power_overwhelming = find_talent_spell( talent_tree::SPECIALIZATION, "Power Overwhelming" ); // Should be ID 387279
    talents.power_overwhelming_buff = find_spell( 387283 );

    talents.madness_of_the_azjaqir = find_talent_spell( talent_tree::SPECIALIZATION, "Madness of the Azj'Aqir" ); // Should be ID 387400
    talents.madness_cb = find_spell( 387409 );
    talents.madness_rof = find_spell( 387413 );
    talents.madness_sb = find_spell( 387414 );

    talents.chaosbringer = find_talent_spell( talent_tree::SPECIALIZATION, "Chaosbringer" ); // Should be ID 422057

    talents.master_ritualist = find_talent_spell( talent_tree::SPECIALIZATION, "Master Ritualist" ); // Should be ID 387165

    talents.burn_to_ashes = find_talent_spell( talent_tree::SPECIALIZATION, "Burn to Ashes" ); // Should be ID 387153
    talents.burn_to_ashes_buff = find_spell( 387154 );

    talents.rain_of_chaos = find_talent_spell( talent_tree::SPECIALIZATION, "Rain of Chaos" ); // Should be ID 266086
    talents.rain_of_chaos_buff = find_spell( 266087 );
    talents.summon_infernal_roc = find_spell( 335236 );

    talents.chaos_incarnate = find_talent_spell( talent_tree::SPECIALIZATION, "Chaos Incarnate" ); // Should be ID 387275

    talents.dimensional_rift = find_talent_spell( talent_tree::SPECIALIZATION, "Dimensional Rift" ); // Should be ID 387976
    talents.shadowy_tear_summon = find_spell( 394235 );
    talents.shadow_barrage = find_spell( 394237 );
    talents.rift_shadow_bolt = find_spell( 394238 );
    talents.unstable_tear_summon = find_spell( 387979 );
    talents.chaos_barrage = find_spell( 387984 );
    talents.chaos_barrage_tick = find_spell( 387985 );
    talents.chaos_tear_summon = find_spell( 394243 );
    talents.rift_chaos_bolt = find_spell( 394246 );

    talents.avatar_of_destruction = find_talent_spell( talent_tree::SPECIALIZATION, "Avatar of Destruction" ); // Should be ID 387159
    talents.summon_blasphemy = find_spell( 387160 );

    // Additional Tier Set spell data

    // T29 (Vault of the Incarnates)
    tier.chaos_maelstrom = find_spell( 394679 );

    // T30 (Aberrus, the Shadowed Crucible)
    tier.channel_demonfire = find_spell( 409890 );
    tier.umbrafire_embers = find_spell( 409652 );

    // T31 (Amirdrassil, the Dream's Hope)
    tier.dimensional_cinder = find_spell( 427285 );
    tier.flame_rift = find_spell( 423874 );
    tier.searing_bolt = find_spell( 423886 );
  }
}