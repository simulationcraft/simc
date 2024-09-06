#include "simulationcraft.hpp"

#include "sc_warlock.hpp"

namespace warlock
{
  void warlock_t::init_spells()
  {
    player_t::init_spells();

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
    warlock_base.malefic_rapture = find_specialization_spell( "Malefic Rapture", WARLOCK_AFFLICTION ); // Should be ID 324536
    warlock_base.malefic_rapture_dmg = find_spell( 324540 );
    warlock_base.potent_afflictions = find_mastery_spell( WARLOCK_AFFLICTION ); // Should be ID 77215
    warlock_base.affliction_warlock = find_specialization_spell( "Affliction Warlock", WARLOCK_AFFLICTION ); // Should be ID 137043

    // Demonology
    warlock_base.hand_of_guldan = find_class_spell( "Hand of Gul'dan" ); // Should be ID 105174
    warlock_base.hog_impact = find_spell( 86040 ); // Contains impact damage data
    warlock_base.wild_imp = find_spell( 104317 ); // Contains pet summoning information
    warlock_base.fel_firebolt_2 = find_spell( 334591 ); // 20% cost reduction for Wild Imps
    warlock_base.master_demonologist = find_mastery_spell( WARLOCK_DEMONOLOGY ); // Should be ID 77219
    warlock_base.demonology_warlock = find_specialization_spell( "Demonology Warlock", WARLOCK_DEMONOLOGY ); // Should be ID 137044

    // Destruction
    warlock_base.immolate = find_specialization_spell( "Immolate" ); // Should be ID 193541
    warlock_base.immolate_old = find_spell( 348 ); // This contains the actual direct damage and cast data, but no longer appears in class_spell list
    warlock_base.immolate_dot = find_spell( 157736 ); // DoT data
    warlock_base.incinerate = find_spell( 29722 ); // Should be ID 29722 TODO: 2024-07-05 this spell was missing from the non-PTR class spell list. Fix once this comes back
    warlock_base.incinerate_energize = find_spell( 244670 ); // Used for resource gain information
    warlock_base.chaos_bolt = find_specialization_spell( "Chaos Bolt" ); // Should be ID 116858
    warlock_base.chaotic_energies = find_mastery_spell( WARLOCK_DESTRUCTION ); // Should be ID 77220
    warlock_base.destruction_warlock = find_specialization_spell( "Destruction Warlock", WARLOCK_DESTRUCTION ); // Should be ID 137046

    warlock_t::init_spells_affliction();
    warlock_t::init_spells_demonology();
    warlock_t::init_spells_destruction();

    // Talents
    talents.grimoire_of_sacrifice = find_talent_spell( talent_tree::SPECIALIZATION, "Grimoire of Sacrifice" ); // Aff/Destro only. Should be ID 108503
    talents.grimoire_of_sacrifice_buff = find_spell( 196099 ); // Buff data and RPPM
    talents.grimoire_of_sacrifice_proc = find_spell( 196100 ); // Damage data

    talents.summoners_embrace = find_talent_spell( talent_tree::SPECIALIZATION, "Summoner's Embrace" ); // Should be ID 453105

    talents.demonic_inspiration = find_talent_spell( talent_tree::CLASS, "Demonic Inspiration" ); // Should be ID 386858

    talents.wrathful_minion = find_talent_spell( talent_tree::CLASS, "Wrathful Minion" ); // Should be ID 386864

    talents.socrethars_guile = find_talent_spell( talent_tree::CLASS, "Socrethar's Guile" ); // Should be ID 405936

    talents.sargerei_technique = find_talent_spell( talent_tree::CLASS, "Sargerei Technique" );  // Should be ID 405955

    talents.demonic_tactics = find_talent_spell( talent_tree::CLASS, "Demonic Tactics" ); // Should be ID 452894

    talents.soul_conduit = find_talent_spell( talent_tree::CLASS, "Soul Conduit" ); // Should be ID 215941

    talents.soulburn = find_talent_spell( talent_tree::CLASS, "Soulburn" ); // Should be ID 385899
    talents.soulburn_buff = find_spell( 387626 );

    warlock_t::init_spells_diabolist();
    warlock_t::init_spells_hellcaller();
    warlock_t::init_spells_soul_harvester();
  }

  void warlock_t::init_spells_affliction()
  {
    // Talents
    talents.unstable_affliction = find_talent_spell( talent_tree::SPECIALIZATION, "Unstable Affliction" ); // Should be ID 316099
    talents.unstable_affliction_2 = find_spell( 231791 ); // Soul Shard on demise
    talents.unstable_affliction_3 = find_spell( 334315 ); // +5 seconds duration
    
    talents.writhe_in_agony = find_talent_spell( talent_tree::SPECIALIZATION, "Writhe in Agony" ); // Should be ID 196102

    talents.seed_of_corruption = find_talent_spell( talent_tree::SPECIALIZATION, "Seed of Corruption" ); // Should be ID 27243
    talents.seed_of_corruption_aoe = find_spell( 27285 ); // Explosion damage

    talents.dark_virtuosity = find_talent_spell( talent_tree::SPECIALIZATION, "Dark Virtuosity" ); // Should be ID 405327

    talents.absolute_corruption = find_talent_spell( talent_tree::SPECIALIZATION, "Absolute Corruption" ); // Should be ID 196103

    talents.siphon_life = find_talent_spell( talent_tree::SPECIALIZATION, "Siphon Life" ); // Should be ID 452999

    talents.kindled_malice = find_talent_spell(talent_tree::SPECIALIZATION, "Kindled Malice");  // Should be ID 405330

    talents.nightfall = find_talent_spell( talent_tree::SPECIALIZATION, "Nightfall" ); // Should be ID 108558
    talents.nightfall_buff = find_spell( 264571 );

    talents.volatile_agony = find_talent_spell( talent_tree::SPECIALIZATION, "Volatile Agony" ); // Should be ID 453034
    talents.volatile_agony_aoe = find_spell( 453035 );

    talents.improved_shadow_bolt = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Shadow Bolt" ); // Should be ID 453080

    talents.drain_soul = find_talent_spell( talent_tree::SPECIALIZATION, "Drain Soul" ); // Should be ID 388667
    talents.drain_soul_dot = find_spell( 198590 ); // This contains all the channel data

    talents.vile_taint = find_talent_spell( talent_tree::SPECIALIZATION, "Vile Taint" ); // Should be ID 278350
    talents.vile_taint_dot = find_spell( 386931 ); // DoT info here

    talents.phantom_singularity = find_talent_spell( talent_tree::SPECIALIZATION, "Phantom Singularity" ); // Should be ID 205179
    talents.phantom_singularity_tick = find_spell( 205246 ); // AoE damage info

    talents.haunt = find_talent_spell( talent_tree::SPECIALIZATION, "Haunt" ); // Should be ID 48181

    talents.shadow_embrace = find_talent_spell( talent_tree::SPECIALIZATION, "Shadow Embrace" ); // Should be ID 32388
    talents.shadow_embrace_debuff_ds = find_spell( 32390 );
    talents.shadow_embrace_debuff_sb = find_spell( 453206 );

    talents.sacrolashs_dark_strike = find_talent_spell( talent_tree::SPECIALIZATION, "Sacrolash's Dark Strike" ); // Should be ID 386986

    talents.summon_darkglare = find_talent_spell( talent_tree::SPECIALIZATION, "Summon Darkglare"); // Should be ID 205180
    talents.eye_beam = find_spell( 205231 );

    talents.cunning_cruelty = find_talent_spell( talent_tree::SPECIALIZATION, "Cunning Cruelty" ); // Should be ID
    talents.shadow_bolt_volley = find_spell( 453176 );

    talents.infirmity = find_talent_spell( talent_tree::SPECIALIZATION, "Infirmity" ); // Should be ID 458036
    talents.infirmity_debuff = find_spell( 458219 );

    talents.improved_haunt = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Haunt" ); // Should be ID 458034

    talents.malediction = find_talent_spell( talent_tree::SPECIALIZATION, "Malediction" ); // Should be ID 453087

    talents.malevolent_visionary = find_talent_spell( talent_tree::SPECIALIZATION, "Malevolent Visionary" ); // Should be ID 387273
    talents.malevolent_visionary_blast = find_spell( 453233 );

    talents.contagion = find_talent_spell( talent_tree::SPECIALIZATION, "Contagion" ); // Should be ID 453096

    talents.cull_the_weak = find_talent_spell( talent_tree::SPECIALIZATION, "Cull the Weak" ); // Should be ID 453056

    talents.creeping_death = find_talent_spell( talent_tree::SPECIALIZATION, "Creeping Death" ); // Should be ID 264000

    talents.soul_rot = find_talent_spell( talent_tree::SPECIALIZATION, "Soul Rot" ); // Should be ID 386997

    talents.tormented_crescendo = find_talent_spell( talent_tree::SPECIALIZATION, "Tormented Crescendo" ); // Should be ID 387075
    talents.tormented_crescendo_buff = find_spell( 387079 );

    talents.xavius_gambit = find_talent_spell( talent_tree::SPECIALIZATION, "Xavius' Gambit" ); // Should be ID 416615

    talents.focused_malignancy = find_talent_spell( talent_tree::SPECIALIZATION, "Focused Malignancy" ); // Should be ID 399668

    talents.perpetual_unstability = find_talent_spell( talent_tree::SPECIALIZATION, "Perpetual Unstability" ); // Should be ID 459376
    talents.perpetual_unstability_proc = find_spell( 459461 );

    talents.malign_omen = find_talent_spell( talent_tree::SPECIALIZATION, "Malign Omen" ); // Should be ID 458041
    talents.malign_omen_buff = find_spell( 458043 );

    talents.relinquished = find_talent_spell( talent_tree::SPECIALIZATION, "Relinquished" ); // Should be ID 453083

    talents.withering_bolt = find_talent_spell( talent_tree::SPECIALIZATION, "Withering Bolt" ); // Should be ID 386976

    talents.improved_malefic_rapture = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Malefic Rapture" ); // Should be ID 454378

    talents.oblivion = find_talent_spell( talent_tree::SPECIALIZATION, "Oblivion" ); // Should be ID 417537

    talents.deaths_embrace = find_talent_spell( talent_tree::SPECIALIZATION, "Death's Embrace" ); // Should be ID 453189

    talents.dark_harvest = find_talent_spell( talent_tree::SPECIALIZATION, "Dark Harvest" ); // Should be ID 387016
    talents.dark_harvest_buff = find_spell( 387018 );

    talents.ravenous_afflictions = find_talent_spell( talent_tree::SPECIALIZATION, "Ravenous Afflictions" ); // Should be ID 459440

    talents.malefic_touch = find_talent_spell( talent_tree::SPECIALIZATION, "Malefic Touch" ); // Should be ID 458029
    talents.malefic_touch_proc = find_spell( 458131 );

    // Additional Tier Set spell data

    // Nerub-ar Palace
    tier.hexflame_aff_2pc = sets->set( WARLOCK_AFFLICTION, TWW1, B2 ); // Should be ID 453643
    tier.hexflame_aff_4pc = sets->set( WARLOCK_AFFLICTION, TWW1, B4 ); // Should be ID 453642
    tier.umbral_lattice = find_spell( 455679 );
  }

  void warlock_t::init_spells_demonology()
  {
    // Talents
    talents.demoniac = find_talent_spell( talent_tree::SPECIALIZATION, "Demoniac" ); // Should be ID 426115
    talents.demonbolt_spell = find_spell( 264178 );
    talents.demonic_core_spell = find_spell( 267102 );
    talents.demonic_core_buff = find_spell( 264173 );

    talents.implosion = find_talent_spell( talent_tree::SPECIALIZATION, "Implosion" ); // Should be ID 196277
    talents.implosion_aoe = find_spell( 196278 );

    talents.call_dreadstalkers = find_talent_spell( talent_tree::SPECIALIZATION, "Call Dreadstalkers" ); // Should be ID 104316
    talents.call_dreadstalkers_2 = find_spell( 193332 ); // Duration data

    talents.imp_gang_boss = find_talent_spell( talent_tree::SPECIALIZATION, "Imp Gang Boss" ); // Should be ID 387445
    talents.imp_gang_boss_buff = find_spell( 387458 );

    talents.spiteful_reconstitution = find_talent_spell( talent_tree::SPECIALIZATION, "Spiteful Reconstitution" ); // Should be ID 428394

    talents.dreadlash = find_talent_spell( talent_tree::SPECIALIZATION, "Dreadlash" ); // Should be ID 264078

    talents.carnivorous_stalkers = find_talent_spell( talent_tree::SPECIALIZATION, "Carnivorous Stalkers" ); // Should be ID 386194

    talents.inner_demons = find_talent_spell( talent_tree::SPECIALIZATION, "Inner Demons" ); // Should be ID 267216

    talents.soul_strike = find_talent_spell( talent_tree::SPECIALIZATION, "Soul Strike" ); // Should be ID 428344
    talents.soul_strike_pet = find_spell( 264057 );
    talents.soul_strike_dmg = find_spell( 267964 );

    talents.bilescourge_bombers = find_talent_spell( talent_tree::SPECIALIZATION, "Bilescourge Bombers" ); // Should be ID 267211
    talents.bilescourge_bombers_aoe = find_spell( 267213 );

    talents.demonic_strength = find_talent_spell( talent_tree::SPECIALIZATION, "Demonic Strength" ); // Should be ID 267171

    talents.sacrificed_souls = find_talent_spell( talent_tree::SPECIALIZATION, "Sacrificed Souls" ); // Should be ID 267214

    talents.rune_of_shadows = find_talent_spell( talent_tree::SPECIALIZATION, "Rune of Shadows" ); // Should be ID 453744

    talents.imperator = find_talent_spell( talent_tree::SPECIALIZATION, "Imp-erator" ); // Should be ID 416230

    talents.fel_invocation = find_talent_spell( talent_tree::SPECIALIZATION, "Fel Invocation" ); // Should be ID 428351

    talents.annihilan_training = find_talent_spell( talent_tree::SPECIALIZATION, "Annihilan Training" ); // Should be ID 386174
    talents.annihilan_training_buff = find_spell( 386176 );

    talents.shadow_invocation = find_talent_spell( talent_tree::SPECIALIZATION, "Shadow Invocation" ); // Should be ID 422054

    talents.wicked_maw = find_talent_spell( talent_tree::SPECIALIZATION, "Wicked Maw" ); // Should be ID 267170
    talents.wicked_maw_debuff = find_spell( 270569 );

    talents.power_siphon = find_talent_spell( talent_tree::SPECIALIZATION, "Power Siphon" ); // Should be ID 264130
    talents.power_siphon_buff = find_spell( 334581 );

    talents.summon_demonic_tyrant = find_talent_spell( talent_tree::SPECIALIZATION, "Summon Demonic Tyrant" ); // Should be ID 265187
    talents.demonic_power_buff = find_spell( 265273 );

    talents.grimoire_felguard = find_talent_spell( talent_tree::SPECIALIZATION, "Grimoire: Felguard" ); // Should be ID 111898
    talents.grimoire_of_service = find_spell( 216187 );

    talents.the_expendables = find_talent_spell( talent_tree::SPECIALIZATION, "The Expendables" ); // Should be ID 387600
    talents.the_expendables_buff = find_spell( 387601 );

    talents.blood_invocation = find_talent_spell( talent_tree::SPECIALIZATION, "Blood Invocation" ); // Should be ID 455576

    talents.umbral_blaze = find_talent_spell( talent_tree::SPECIALIZATION, "Umbral Blaze" ); // Should be ID 405798
    talents.umbral_blaze_dot = find_spell( 405802 );

    talents.reign_of_tyranny = find_talent_spell( talent_tree::SPECIALIZATION, "Reign of Tyranny" ); // Should be ID 427684
    talents.reign_of_tyranny_buff = find_spell( 427687 );

    talents.demonic_calling = find_talent_spell( talent_tree::SPECIALIZATION, "Demonic Calling" ); // Should be ID 205145
    talents.demonic_calling_buff = find_spell( 205146 );

    talents.fiendish_oblation = find_talent_spell( talent_tree::SPECIALIZATION, "Fiendish Oblation" ); // Should be ID 455569

    talents.fel_sunder = find_talent_spell( talent_tree::SPECIALIZATION, "Fel Sunder" ); // Should be ID 387399
    talents.fel_sunder_debuff = find_spell( 387402 );

    talents.doom = find_talent_spell( talent_tree::SPECIALIZATION, "Doom" ); // Should be ID 460551
    talents.doom_debuff = find_spell( 460553 );
    talents.doom_dmg = find_spell( 460555 );

    talents.pact_of_the_imp_mother = find_talent_spell( talent_tree::SPECIALIZATION, "Pact of the Imp Mother" ); // Should be ID 387541

    talents.summon_vilefiend = find_talent_spell( talent_tree::SPECIALIZATION, "Summon Vilefiend" ); // Should be ID 264119
    talents.bile_spit = find_spell( 267997 );
    talents.headbutt = find_spell( 267999 );

    talents.dread_calling = find_talent_spell( talent_tree::SPECIALIZATION, "Dread Calling" ); // Should be ID 387391
    talents.dread_calling_buff = find_spell( 387393 );
    talents.dread_calling_pet = find_spell( 387392 );

    talents.antoran_armaments = find_talent_spell( talent_tree::SPECIALIZATION, "Antoran Armaments" ); // Should be ID 387494
    talents.antoran_armaments_buff = find_spell( 387496 );
    talents.soul_cleave = find_spell( 387502 );

    talents.doom_eternal = find_talent_spell( talent_tree::SPECIALIZATION, "Doom Eternal" ); // Should be ID 455585

    talents.impending_doom = find_talent_spell( talent_tree::SPECIALIZATION, "Impending Doom" ); // Should be ID 455587

    talents.foul_mouth = find_talent_spell( talent_tree::SPECIALIZATION, "Foul Mouth" ); // Should be ID 455502

    talents.the_houndmasters_gambit = find_talent_spell( talent_tree::SPECIALIZATION, "The Houndmaster's Gambit" ); // Should be ID 455572
    talents.houndmasters_aura = find_spell( 455611 );

    talents.improved_demonic_tactics = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Demonic Tactics" ); // Should be ID 453800

    talents.demonic_brutality = find_talent_spell( talent_tree::SPECIALIZATION, "Demonic Brutality" ); // Should be ID 453908

    talents.pact_of_the_eredruin = find_talent_spell( talent_tree::SPECIALIZATION, "Pact of the Ered'ruin" ); // Should be ID 453568
    talents.doomguard = find_spell( 453590 );
    talents.doom_bolt = find_spell( 453616 );

    talents.shadowtouched = find_talent_spell( talent_tree::SPECIALIZATION, "Shadowtouched" ); // Should be ID 453619

    talents.mark_of_shatug = find_talent_spell( talent_tree::SPECIALIZATION, "Mark of Shatug" ); // Should be ID 455449
    talents.gloom_slash = find_spell( 455491 );

    talents.mark_of_fharg = find_talent_spell( talent_tree::SPECIALIZATION, "Mark of F'harg" ); // Should be ID 455450
    talents.infernal_presence = find_spell( 428453 );
    talents.infernal_presence_dmg = find_spell( 428455 );

    talents.flametouched = find_talent_spell( talent_tree::SPECIALIZATION, "Flametouched" ); // Should be ID 453699

    talents.immutable_hatred = find_talent_spell( talent_tree::SPECIALIZATION, "Immutable Hatred" ); // Should be ID 405670
    talents.immutable_hatred_proc = find_spell( 405681 );

    talents.guillotine = find_talent_spell( talent_tree::SPECIALIZATION, "Guillotine" ); // Should be ID 386833
    talents.guillotine_pet = find_spell( 386542 );
    talents.fiendish_wrath_buff = find_spell( 386601 );
    talents.fiendish_wrath_dmg = find_spell( 386702 );
    talents.fel_explosion = find_spell( 386609 );

    // Additional Tier Set spell data

    // Nerub-ar Palace
    tier.hexflame_demo_2pc = sets->set( WARLOCK_DEMONOLOGY, TWW1, B2 ); // Should be ID 453644
    tier.hexflame_demo_4pc = sets->set( WARLOCK_DEMONOLOGY, TWW1, B4 ); // Should be ID 453645
    tier.empowered_legion_strike = find_spell( 455647 );

    // Initialize some default values for pet spawners
    warlock_pet_list.wild_imps.set_default_duration( warlock_base.wild_imp->duration() );

    warlock_pet_list.dreadstalkers.set_default_duration( talents.call_dreadstalkers_2->duration() );

    warlock_pet_list.doomguards.set_default_duration( talents.doomguard->duration() );
  }

  void warlock_t::init_spells_destruction()
  {
    // Talents
    talents.conflagrate = find_talent_spell( talent_tree::SPECIALIZATION, "Conflagrate" ); // Should be ID 17962
    talents.conflagrate_2 = find_spell( 245330 );

    talents.backdraft = find_talent_spell( talent_tree::SPECIALIZATION, "Backdraft" ); // Should be ID 196406
    talents.backdraft_buff = find_spell( 117828 );

    talents.rain_of_fire = find_talent_spell( talent_tree::SPECIALIZATION, "Rain of Fire" ); // Should be ID 5740
    talents.rain_of_fire_tick = find_spell( 42223 );

    talents.roaring_blaze = find_talent_spell( talent_tree::SPECIALIZATION, "Roaring Blaze" ); // Should be ID 205184
    talents.conflagrate_debuff = find_spell( 265931 );

    talents.improved_conflagrate = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Conflagrate" ); // Should be ID 231793

    talents.backlash = find_talent_spell( talent_tree::SPECIALIZATION, "Backlash" ); // Should be ID 387384

    talents.mayhem = find_talent_spell( talent_tree::SPECIALIZATION, "Mayhem" ); // Should be ID 387506

    talents.havoc = find_talent_spell( talent_tree::SPECIALIZATION, "Havoc" ); // Should be spell 80240
    talents.havoc_debuff = find_spell( 80240 );

    talents.pyrogenics = find_talent_spell( talent_tree::SPECIALIZATION, "Pyrogenics" ); // Should be ID 387095
    talents.pyrogenics_debuff = find_spell( 387096 );

    talents.inferno = find_talent_spell(talent_tree::SPECIALIZATION, "Inferno"); // Should be ID 270545

    talents.cataclysm = find_talent_spell( talent_tree::SPECIALIZATION, "Cataclysm" ); // Should be ID 152108

    talents.indiscriminate_flames = find_talent_spell( talent_tree::SPECIALIZATION, "Indiscriminate Flames" ); // Should be ID 457114

    talents.rolling_havoc = find_talent_spell( talent_tree::SPECIALIZATION, "Rolling Havoc" ); // Should be ID 387569
    talents.rolling_havoc_buff = find_spell( 387570 );

    talents.scalding_flames = find_talent_spell( talent_tree::SPECIALIZATION, "Scalding Flames" ); // Should be ID 388832

    talents.shadowburn = find_talent_spell( talent_tree::SPECIALIZATION, "Shadowburn" ); // Should be ID 17877
    talents.shadowburn_2 = find_spell( 245731 );

    talents.explosive_potential = find_talent_spell( talent_tree::SPECIALIZATION, "Explosive Potential" ); // Should be ID 388827

    talents.ashen_remains = find_talent_spell( talent_tree::SPECIALIZATION, "Ashen Remains" ); // Should be ID 387252

    talents.channel_demonfire = find_talent_spell( talent_tree::SPECIALIZATION, "Channel Demonfire" ); // Should be ID 196447
    talents.channel_demonfire_tick = find_spell( 196448 ); // Includes both direct and splash damage values
    talents.channel_demonfire_travel = find_spell( 196449 );

    talents.blistering_atrophy = find_talent_spell( talent_tree::SPECIALIZATION, "Blistering Atrophy" ); // Should be ID 456939

    talents.conflagration_of_chaos = find_talent_spell( talent_tree::SPECIALIZATION, "Conflagration of Chaos" ); // Should be ID 387108
    talents.conflagration_of_chaos_cf = find_spell( 387109 );
    talents.conflagration_of_chaos_sb = find_spell( 387110 );

    talents.emberstorm = find_talent_spell( talent_tree::SPECIALIZATION, "Emberstorm" ); // Should be ID 454744

    talents.summon_infernal = find_talent_spell( talent_tree::SPECIALIZATION, "Summon Infernal" ); // Should be ID 1122
    talents.summon_infernal_main = find_spell( 111685 );
    talents.infernal_awakening = find_spell( 22703 );
    talents.immolation_buff = find_spell( 19483 );
    talents.immolation_dmg = find_spell( 20153 );
    talents.embers = find_spell( 264364 );
    talents.burning_ember = find_spell( 264365 );

    talents.fire_and_brimstone = find_talent_spell( talent_tree::SPECIALIZATION, "Fire and Brimstone" ); // Should be ID 196408

    talents.flashpoint = find_talent_spell( talent_tree::SPECIALIZATION, "Flashpoint" ); // Should be 387259
    talents.flashpoint_buff = find_spell( 387263 );

    talents.raging_demonfire = find_talent_spell( talent_tree::SPECIALIZATION, "Raging Demonfire" ); // Should be ID 387166

    talents.fiendish_cruelty = find_talent_spell( talent_tree::SPECIALIZATION, "Fiendish Cruelty" ); // Should be ID 456943

    talents.eradication = find_talent_spell( talent_tree::SPECIALIZATION, "Eradication" ); // Should be ID 196412
    talents.eradication_debuff = find_spell( 196414 );

    talents.crashing_chaos = find_talent_spell( talent_tree::SPECIALIZATION, "Crashing Chaos" ); // Should be ID 417234
    talents.crashing_chaos_buff = find_spell( 417282 );

    talents.rain_of_chaos = find_talent_spell( talent_tree::SPECIALIZATION, "Rain of Chaos" ); // Should be ID 266086
    talents.rain_of_chaos_buff = find_spell( 266087 );
    talents.summon_infernal_roc = find_spell( 335236 );

    talents.reverse_entropy = find_talent_spell( talent_tree::SPECIALIZATION, "Reverse Entropy" ); // Should be ID 205148
    talents.reverse_entropy_buff = find_spell( 266030 );

    talents.internal_combustion = find_talent_spell( talent_tree::SPECIALIZATION, "Internal Combustion" ); // Should be ID 266134

    talents.demonfire_mastery = find_talent_spell( talent_tree::SPECIALIZATION, "Demonfire Mastery" ); // Should be ID 456946

    talents.devastation = find_talent_spell( talent_tree::SPECIALIZATION, "Devastation" ); // Should be ID 454735

    talents.ritual_of_ruin = find_talent_spell( talent_tree::SPECIALIZATION, "Ritual of Ruin" ); // Should be ID 387156
    talents.impending_ruin_buff = find_spell( 387158 );
    talents.ritual_of_ruin_buff = find_spell( 387157 );

    talents.ruin = find_talent_spell( talent_tree::SPECIALIZATION, "Ruin" ); // Should be ID 387103

    talents.soul_fire = find_talent_spell( talent_tree::SPECIALIZATION, "Soul Fire" ); // Should be ID 6353
    talents.soul_fire_2 = find_spell( 281490 );

    talents.improved_chaos_bolt = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Chaos Bolt" ); // Should be ID 456951

    talents.burn_to_ashes = find_talent_spell( talent_tree::SPECIALIZATION, "Burn to Ashes" ); // Should be ID 387153
    talents.burn_to_ashes_buff = find_spell( 387154 );

    talents.master_ritualist = find_talent_spell( talent_tree::SPECIALIZATION, "Master Ritualist" ); // Should be ID 387165

    talents.power_overwhelming = find_talent_spell( talent_tree::SPECIALIZATION, "Power Overwhelming" ); // Should be ID 387279
    talents.power_overwhelming_buff = find_spell( 387283 );

    talents.diabolic_embers = find_talent_spell( talent_tree::SPECIALIZATION, "Diabolic Embers" ); // Should be ID 387173

    talents.dimensional_rift = find_talent_spell( talent_tree::SPECIALIZATION, "Dimensional Rift" ); // Should be ID 387976
    talents.shadowy_tear_summon = find_spell( 394235 );
    talents.shadow_barrage = find_spell( 394237 );
    talents.rift_shadow_bolt = find_spell( 394238 );
    talents.unstable_tear_summon = find_spell( 387979 );
    talents.chaos_barrage = find_spell( 387984 );
    talents.chaos_barrage_tick = find_spell( 387985 );
    talents.chaos_tear_summon = find_spell( 394243 );
    talents.rift_chaos_bolt = find_spell( 394246 );

    talents.decimation = find_talent_spell( talent_tree::SPECIALIZATION, "Decimation" ); // Should be ID 456985
    talents.decimation_buff = find_spell( 457555 );

    talents.chaos_incarnate = find_talent_spell( talent_tree::SPECIALIZATION, "Chaos Incarnate" ); // Should be ID 387275

    talents.avatar_of_destruction = find_talent_spell( talent_tree::SPECIALIZATION, "Avatar of Destruction" ); // Should be ID 456975
    talents.summon_overfiend = find_spell( 434587 );
    talents.overfiend_buff = find_spell( 457578 );
    talents.overfiend_cb = find_spell( 434589 );

    talents.dimension_ripper = find_talent_spell( talent_tree::SPECIALIZATION, "Dimension Ripper" ); // Should be ID 457025

    talents.unstable_rifts = find_talent_spell( talent_tree::SPECIALIZATION, "Unstable Rifts" ); // Should be ID 457064
    talents.dimensional_cinder = find_spell( 460805 );

    // Additional Tier Set spell data

    // Nerub-ar Palace
    tier.hexflame_destro_2pc = sets->set( WARLOCK_DESTRUCTION, TWW1, B2 ); // Should be ID 453647
    tier.hexflame_destro_4pc = sets->set( WARLOCK_DESTRUCTION, TWW1, B4 ); // Should be ID 453646
    tier.echo_of_the_azjaqir = find_spell( 455674 );

    // Initialize some default values for pet spawners
    warlock_pet_list.infernals.set_default_duration( talents.summon_infernal_main->duration() );
    warlock_pet_list.overfiends.set_default_duration( talents.summon_overfiend->duration() );
  }

  void warlock_t::init_spells_diabolist()
  {
    hero.diabolic_ritual = find_talent_spell( talent_tree::HERO, "Diabolic Ritual" ); // Should be ID 428514
    hero.ritual_overlord = find_spell( 431944 );
    hero.ritual_mother = find_spell( 432815 );
    hero.ritual_pit_lord = find_spell( 432816 );
    hero.art_overlord = find_spell( 428524 );
    hero.art_mother = find_spell( 432794 );
    hero.art_pit_lord = find_spell( 432795 );
    hero.summon_overlord = find_spell( 428571 );
    hero.summon_mother = find_spell( 428565 );
    hero.summon_pit_lord = find_spell( 434400 );
    hero.wicked_cleave = find_spell( 432120 );
    hero.chaos_salvo = find_spell( 432569 );
    hero.chaos_salvo_missile = find_spell( 432592 );
    hero.chaos_salvo_dmg = find_spell( 432596 );
    hero.felseeker = find_spell( 438973 );
    hero.felseeker_dmg = find_spell( 434404 );

    hero.cloven_souls = find_talent_spell( talent_tree::HERO, "Cloven Souls" ); // Should be ID 428517
    hero.cloven_soul_debuff = find_spell( 434424 );

    hero.touch_of_rancora = find_talent_spell( talent_tree::HERO, "Touch of Rancora" ); // Should be ID 429893

    hero.secrets_of_the_coven = find_talent_spell( talent_tree::HERO, "Secrets of the Coven" ); // Should be ID 428518
    hero.infernal_bolt = find_spell( 434506 );
    hero.infernal_bolt_buff = find_spell( 433891 );

    hero.cruelty_of_kerxan = find_talent_spell( talent_tree::HERO, "Cruelty of Kerxan" ); // Should be ID 429902

    hero.infernal_machine = find_talent_spell( talent_tree::HERO, "Infernal Machine" ); // Should be ID 429917

    hero.flames_of_xoroth = find_talent_spell( talent_tree::HERO, "Flames of Xoroth" ); // Should be ID 429657

    hero.abyssal_dominion = find_talent_spell( talent_tree::HERO, "Abyssal Dominion" ); // Should be ID 429581
    hero.abyssal_dominion_buff = find_spell( 456323 );
    hero.infernal_fragmentation = find_spell( 456310 );

    hero.gloom_of_nathreza = find_talent_spell( talent_tree::HERO, "Gloom of Nathreza" ); // Should be ID 429899

    hero.ruination = find_talent_spell( talent_tree::HERO, "Ruination" ); // Should be ID 428522
    hero.ruination_buff = find_spell( 433885 );
    hero.ruination_cast = find_spell( 434635 );
    hero.ruination_impact = find_spell( 434636 );
    hero.diabolic_imp = find_spell( 438822 );
    hero.diabolic_bolt = find_spell( 438823 );

    warlock_pet_list.overlords.set_default_duration( hero.summon_overlord->duration() );
    warlock_pet_list.mothers.set_default_duration( hero.summon_mother->duration() );
    warlock_pet_list.pit_lords.set_default_duration( hero.summon_pit_lord->duration() );
    warlock_pet_list.fragments.set_default_duration( hero.infernal_fragmentation->duration() );
  }

  void warlock_t::init_spells_hellcaller()
  {
    hero.wither = find_talent_spell( talent_tree::HERO, "Wither" ); // Should be ID 445465
    hero.wither_direct = find_spell( 445468 );
    hero.wither_dot = find_spell( 445474 );

    hero.xalans_ferocity = find_talent_spell( talent_tree::HERO, "Xalan's Ferocity" ); // Should be ID 440044

    hero.blackened_soul = find_talent_spell( talent_tree::HERO, "Blackened Soul" ); // Should be ID 440043
    hero.blackened_soul_trigger = find_spell( 445731 );
    hero.blackened_soul_dmg = find_spell( 445736 );

    hero.xalans_cruelty = find_talent_spell( talent_tree::HERO, "Xalan's Cruelty" ); // Should be ID 440040

    hero.hatefury_rituals = find_talent_spell( talent_tree::HERO, "Hatefury Rituals" ); // Should be ID 440048

    hero.bleakheart_tactics = find_talent_spell( talent_tree::HERO, "Bleakheart Tactics" ); // Should be ID 440051

    hero.mark_of_xavius = find_talent_spell( talent_tree::HERO, "Mark of Xavius" ); // Should be ID 440046

    hero.seeds_of_their_demise = find_talent_spell( talent_tree::HERO, "Seeds of Their Demise" ); // Should be ID 440055

    hero.mark_of_perotharn = find_talent_spell( talent_tree::HERO, "Mark of Peroth'arn" ); // Should be ID 440045

    hero.malevolence = find_talent_spell( talent_tree::HERO, "Malevolence" ); // Should be ID 430014
    hero.malevolence_buff = find_spell( 442726 );
    hero.malevolence_dmg = find_spell( 446285 );
  }

  void warlock_t::init_spells_soul_harvester()
  {
    hero.demonic_soul = find_talent_spell( talent_tree::HERO, "Demonic Soul" ); // Should be ID 449614
    hero.succulent_soul = find_spell( 449793 );
    hero.demonic_soul_dmg = find_spell( 449801 );

    hero.necrolyte_teachings = find_talent_spell( talent_tree::HERO, "Necrolyte Teachings" ); // Should be ID 449620

    hero.soul_anathema = find_talent_spell( talent_tree::HERO, "Soul Anathema" ); // Should be ID 449624
    hero.soul_anathema_dot = find_spell( 450538 );

    hero.demoniacs_fervor = find_talent_spell( talent_tree::HERO, "Demoniac's Fervor" ); // Should be ID 449629

    hero.shared_fate = find_talent_spell( talent_tree::HERO, "Shared Fate" ); // Should be ID 449704
    hero.shared_fate_debuff = find_spell( 450591 );
    hero.shared_fate_dmg = find_spell( 450593 );

    hero.feast_of_souls = find_talent_spell( talent_tree::HERO, "Feast of Souls" ); // Should be ID 449706

    hero.wicked_reaping = find_talent_spell( talent_tree::HERO, "Wicked Reaping" ); // Should be ID 449631
    hero.wicked_reaping_dmg = find_spell( 449826 );

    hero.quietus = find_talent_spell( talent_tree::HERO, "Quietus" ); // Should be ID 449634

    hero.sataiels_volition = find_talent_spell( talent_tree::HERO, "Sataiel's Volition" ); // Should be ID 449637

    hero.shadow_of_death = find_talent_spell( talent_tree::HERO, "Shadow of Death" ); // Should be ID 449638
    hero.shadow_of_death_energize = find_spell( 449858 );
  }

  void warlock_t::init_base_stats()
  {
    if ( base.distance < 1.0 )
      base.distance = 30.0;

    player_t::init_base_stats();

    base.attack_power_per_strength = 0.0;
    base.attack_power_per_agility  = 0.0;
    base.spell_power_per_intellect = 1.0;

    resources.base[ RESOURCE_SOUL_SHARD ] = 5;

    if ( default_pet.empty() )
    {
      if ( specialization() == WARLOCK_AFFLICTION )
        default_pet = "imp";
      else if ( specialization() == WARLOCK_DEMONOLOGY )
        default_pet = "felguard";
      else if ( specialization() == WARLOCK_DESTRUCTION )
        default_pet = "imp";
    }
  }

  void warlock_t::create_buffs()
  {
    player_t::create_buffs();

    // Shared buffs
    buffs.grimoire_of_sacrifice = make_buff( this, "grimoire_of_sacrifice", talents.grimoire_of_sacrifice_buff )
                                      ->set_chance( 1.0 );

    buffs.soulburn = make_buff( this, "soulburn", talents.soulburn_buff );

    buffs.pet_movement = make_buff( this, "pet_movement" )->set_max_stack( 100 );

    // Affliction buffs
    create_buffs_affliction();

    buffs.soul_rot = make_buff( this, "soul_rot", talents.soul_rot)
                         ->set_cooldown( 0_ms );

    buffs.malign_omen = make_buff( this, "malign_omen", talents.malign_omen_buff )
                            ->set_default_value( talents.malign_omen_buff->effectN( 1 ).percent() );

    buffs.dark_harvest_haste = make_buff( this, "dark_harvest_haste", talents.dark_harvest_buff )
                                   ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
                                   ->set_default_value( talents.dark_harvest_buff->effectN( 1 ).percent() );

    buffs.dark_harvest_crit = make_buff( this, "dark_harvest_crit", talents.dark_harvest_buff )
                                  ->set_pct_buff_type( STAT_PCT_BUFF_CRIT )
                                  ->set_default_value( talents.dark_harvest_buff->effectN( 2 ).percent() );

    // Demonology buffs
    create_buffs_demonology();

    // Destruction buffs
    create_buffs_destruction();

    buffs.rolling_havoc = make_buff( this, "rolling_havoc", talents.rolling_havoc_buff )
                              ->set_default_value( talents.rolling_havoc->effectN( 1 ).percent() )
                              ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

    create_buffs_diabolist();
    create_buffs_hellcaller();
    create_buffs_soul_harvester();
  }

  void warlock_t::create_buffs_affliction()
  {
    buffs.nightfall = make_buff( this, "nightfall", talents.nightfall_buff );

    buffs.tormented_crescendo = make_buff( this, "tormented_crescendo", talents.tormented_crescendo_buff );

    buffs.umbral_lattice = make_buff( this, "umbral_lattice", tier.umbral_lattice )
                               ->set_chance( rng_settings.umbral_lattice.setting_value );
  }

  void warlock_t::create_buffs_demonology()
  {
    buffs.demonic_core = make_buff( this, "demonic_core", talents.demonic_core_buff );

    buffs.power_siphon = make_buff( this, "power_siphon", talents.power_siphon_buff )
                             ->set_default_value( talents.power_siphon_buff->effectN( 1 ).percent() + talents.blood_invocation->effectN( 2 ).percent() + hero.necrolyte_teachings->effectN( 3 ).percent() );

    buffs.demonic_calling = make_buff( this, "demonic_calling", talents.demonic_calling_buff )
                                ->set_chance( talents.demonic_calling->effectN( 3 ).percent() );

    buffs.inner_demons = make_buff( this, "inner_demons", talents.inner_demons )
                             ->set_period( talents.inner_demons->effectN( 1 ).period() )
                             ->set_tick_time_behavior( buff_tick_time_behavior::UNHASTED )
                             ->set_tick_zero( true )
                             ->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
                               warlock_pet_list.wild_imps.spawn();
                             } );

    buffs.dread_calling = make_buff<buff_t>( this, "dread_calling", talents.dread_calling_buff )
                              ->set_default_value( talents.dread_calling->effectN( 1 ).percent() );

    // Pet tracking buffs
    buffs.wild_imps = make_buff( this, "wild_imps" )->set_max_stack( 40 );

    buffs.dreadstalkers = make_buff( this, "dreadstalkers" )->set_max_stack( 8 )
                          ->set_duration( talents.call_dreadstalkers_2->duration() );

    buffs.vilefiend = make_buff( this, "vilefiend" )->set_max_stack( 1 )
                      ->set_duration( talents.summon_vilefiend->duration() );

    buffs.tyrant = make_buff( this, "tyrant" )->set_max_stack( 1 )
                   ->set_duration( find_spell( 265187 )->duration() );

    buffs.grimoire_felguard = make_buff( this, "grimoire_felguard" )->set_max_stack( 1 )
                              ->set_duration( talents.grimoire_felguard->duration() );
  }

  void warlock_t::create_buffs_destruction()
  {
    buffs.backdraft = make_buff( this, "backdraft", talents.backdraft_buff );

    buffs.reverse_entropy = make_buff( this, "reverse_entropy", talents.reverse_entropy_buff )
                                ->set_default_value_from_effect( 1 )
                                ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
                                ->set_trigger_spell( talents.reverse_entropy )
                                ->set_rppm( RPPM_NONE, talents.reverse_entropy->real_ppm() );

    buffs.rain_of_chaos = make_buff( this, "rain_of_chaos", talents.rain_of_chaos_buff );

    buffs.impending_ruin = make_buff ( this, "impending_ruin", talents.impending_ruin_buff )
                               ->set_max_stack( talents.impending_ruin_buff->max_stacks() + as<int>( talents.master_ritualist->effectN( 2 ).base_value() ) )
                               ->set_stack_change_callback( [ this ]( buff_t* b, int, int cur )
                                 {
                                   if ( cur == b->max_stack() )
                                   {
                                     make_event( sim, 0_ms, [ this, b ] { 
                                       buffs.ritual_of_ruin->trigger();
                                       b->expire();
                                       });
                                   };
                                 });

    buffs.ritual_of_ruin = make_buff ( this, "ritual_of_ruin", talents.ritual_of_ruin_buff );

    buffs.conflagration_of_chaos_cf = make_buff( this, "conflagration_of_chaos_cf", talents.conflagration_of_chaos_cf )
                                          ->set_default_value_from_effect( 1 );

    buffs.conflagration_of_chaos_sb = make_buff( this, "conflagration_of_chaos_sb", talents.conflagration_of_chaos_sb )
                                          ->set_default_value_from_effect( 1 );

    buffs.flashpoint = make_buff( this, "flashpoint", talents.flashpoint_buff )
                           ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
                           ->set_default_value_from_effect( 1 );

    buffs.crashing_chaos = make_buff( this, "crashing_chaos", talents.crashing_chaos_buff )
                                 ->set_max_stack( std::max( as<int>( talents.crashing_chaos->effectN( 3 ).base_value() ), 1 ) )
                                 ->set_reverse( true );

    // 2024-07-19 - Power Overwhelming appears to grant double the listed value per stack
    buffs.power_overwhelming = make_buff( this, "power_overwhelming", talents.power_overwhelming_buff )
                                   ->set_pct_buff_type( STAT_PCT_BUFF_MASTERY )
                                   ->set_default_value( talents.power_overwhelming->effectN( 2 ).base_value() / 10.0 )
                                   ->set_refresh_behavior( buff_refresh_behavior::DISABLED );

    buffs.burn_to_ashes = make_buff( this, "burn_to_ashes", talents.burn_to_ashes_buff )
                              ->set_default_value( talents.burn_to_ashes->effectN( 1 ).percent() );

    buffs.decimation = make_buff( this, "decimation", talents.decimation_buff )
                           ->set_default_value_from_effect( 1 );

    buffs.summon_overfiend = make_buff( this, "summon_overfiend", talents.overfiend_buff )
                                 ->set_tick_time_behavior( buff_tick_time_behavior::UNHASTED )
                                 ->set_period( talents.overfiend_buff->effectN( 1 ).period() )
                                 ->set_tick_callback( [ this ]( buff_t*, int, timespan_t )
                                   { resource_gain( RESOURCE_SOUL_SHARD, talents.overfiend_buff->effectN( 1 ).base_value() / 10.0, gains.summon_overfiend ); } );

    buffs.echo_of_the_azjaqir = make_buff( this, "echo_of_the_azjaqir", tier.echo_of_the_azjaqir );
  }

  void warlock_t::create_buffs_diabolist()
  {
    buffs.ritual_overlord = make_buff( this, "diabolic_ritual_overlord", hero.ritual_overlord )
                                ->set_duration( hero.ritual_overlord->duration() + warlock_base.destruction_warlock->effectN( 5 ).time_value() )
                                ->set_stack_change_callback( [ this ]( buff_t*, int, int cur )
                                  {
                                    if ( cur == 0 )
                                    {
                                      make_event( sim, 0_ms, [ this ] { buffs.art_overlord->trigger(); } );
                                      diabolic_ritual = 1;
                                    }
                                  } );

    buffs.ritual_mother = make_buff( this, "diabolic_ritual_mother_of_chaos", hero.ritual_mother )
                              ->set_duration( hero.ritual_mother->duration() + warlock_base.destruction_warlock->effectN( 5 ).time_value() )
                              ->set_stack_change_callback( [ this ]( buff_t*, int, int cur )
                                {
                                  if ( cur == 0 )
                                  {
                                    make_event( sim, 0_ms, [ this ] { buffs.art_mother->trigger(); } );
                                    diabolic_ritual = 2;
                                  }
                                } );

    buffs.ritual_pit_lord = make_buff( this, "diabolic_ritual_pit_lord", hero.ritual_pit_lord )
                                ->set_duration( hero.ritual_pit_lord->duration() + warlock_base.destruction_warlock->effectN( 5 ).time_value() )
                                ->set_stack_change_callback( [ this ]( buff_t*, int, int cur )
                                  {
                                    if ( cur == 0 )
                                    {
                                      make_event( sim, 0_ms, [ this ] { buffs.art_pit_lord->trigger(); } );
                                      diabolic_ritual = 0;
                                    }
                                  } );

    buffs.art_overlord = make_buff( this, "demonic_art_overlord", hero.art_overlord )
                             ->set_stack_change_callback( [ this ]( buff_t*, int, int cur )
                               {
                                 if ( cur == 0 )
                                   warlock_pet_list.overlords.spawn();
                               } );

    buffs.art_mother = make_buff( this, "demonic_art_mother_of_chaos", hero.art_mother )
                           ->set_stack_change_callback( [ this ]( buff_t*, int, int cur )
                             {
                               if ( cur == 0 )
                               {
                                 warlock_pet_list.mothers.spawn();

                                 if ( hero.secrets_of_the_coven.ok() )
                                      buffs.infernal_bolt->trigger();
                               }
                             } );

    buffs.art_pit_lord = make_buff( this, "demonic_art_pit_lord", hero.art_pit_lord )
                             ->set_stack_change_callback( [ this ]( buff_t*, int, int cur )
                               {
                                 if ( cur == 0 )
                                 {
                                   warlock_pet_list.pit_lords.spawn();

                                   if ( hero.ruination.ok() )
                                     buffs.ruination->trigger();
                                 }
                               } );

    buffs.infernal_bolt = make_buff( this, "infernal_bolt", hero.infernal_bolt_buff );

    buffs.abyssal_dominion = make_buff( this, "Abyssal Dominion", hero.abyssal_dominion_buff );

    buffs.ruination = make_buff( this, "ruination", hero.ruination_buff );
  }

  void warlock_t::create_buffs_hellcaller()
  {
    buffs.malevolence = make_buff( this, "malevolence", hero.malevolence_buff )
                            ->set_cooldown( hero.malevolence_buff->cooldown() - 1_s )
                            ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
                            ->set_default_value_from_effect( 1 );
  }

  void warlock_t::create_buffs_soul_harvester()
  {
    buffs.succulent_soul = make_buff( this, "succulent_soul", hero.succulent_soul );
  }

  void warlock_t::create_pets()
  {
    for ( auto& pet : pet_name_list )
    {
      create_pet( pet );
    }
  }

  pet_t* warlock_t::create_pet( util::string_view pet_name, util::string_view pet_type )
  {
    pet_t* p = find_pet( pet_name );
    if ( p )
      return p;

    pet_t* summon_pet = create_main_pet( pet_name, pet_type );
    if ( summon_pet )
      return summon_pet;

    return nullptr;
  }

  void warlock_t::init_gains()
  {
    player_t::init_gains();

    if ( specialization() == WARLOCK_AFFLICTION )
      init_gains_affliction();
    if ( specialization() == WARLOCK_DEMONOLOGY )
      init_gains_demonology();
    if ( specialization() == WARLOCK_DESTRUCTION )
      init_gains_destruction();

    init_gains_diabolist();
    init_gains_hellcaller();
    init_gains_soul_harvester();

    gains.soul_conduit = get_gain( "soul_conduit" );
  }

  void warlock_t::init_gains_affliction()
  {
    gains.agony = get_gain( "agony" );
    gains.unstable_affliction_refund = get_gain( "unstable_affliction_refund" );
    gains.drain_soul = get_gain( "drain_soul" );
  }

  void warlock_t::init_gains_demonology()
  {
    gains.soul_strike = get_gain( "soul_strike" );
  }

  void warlock_t::init_gains_destruction()
  {
    gains.immolate = get_gain( "immolate" );
    gains.immolate_crits = get_gain( "immolate_crits" );
    gains.incinerate_crits = get_gain( "incinerate_crits" );
    gains.infernal = get_gain( "infernal" );
    gains.shadowburn_refund = get_gain( "shadowburn_refund" );
    gains.summon_overfiend = get_gain( "summon_overfiend" );
  }

  void warlock_t::init_gains_diabolist()
  {
  }

  void warlock_t::init_gains_hellcaller()
  {
    gains.wither = get_gain( "wither" );
    gains.wither_crits = get_gain( "wither_crits" );
  }

  void warlock_t::init_gains_soul_harvester()
  {
    gains.feast_of_souls = get_gain( "feast_of_souls" );
    gains.shadow_of_death = get_gain( "shadow_of_death" );
  }

  void warlock_t::init_procs()
  {
    player_t::init_procs();

    if ( specialization() == WARLOCK_AFFLICTION )
      init_procs_affliction();
    if ( specialization() == WARLOCK_DEMONOLOGY )
      init_procs_demonology();
    if ( specialization() == WARLOCK_DESTRUCTION )
      init_procs_destruction();

    init_procs_diabolist();
    init_procs_hellcaller();
    init_procs_soul_harvester();

    procs.demonic_calling = get_proc( "demonic_calling" );
    procs.soul_conduit = get_proc( "soul_conduit" );
    procs.ritual_of_ruin = get_proc( "ritual_of_ruin" );
    procs.avatar_of_destruction = get_proc( "avatar_of_destruction" );
    procs.mayhem = get_proc( "mayhem" );
    procs.conflagration_of_chaos_cf = get_proc( "conflagration_of_chaos_cf" );
    procs.conflagration_of_chaos_sb = get_proc( "conflagration_of_chaos_sb" );
    procs.demonic_inspiration = get_proc( "demonic_inspiration" );
    procs.wrathful_minion = get_proc( "wrathful_minion" );
  }

  void warlock_t::init_procs_affliction()
  {
    procs.nightfall = get_proc( "nightfall" );
    procs.shadow_bolt_volley = get_proc( "shadow_bolt_volley" );
    procs.tormented_crescendo = get_proc( "tormented_crescendo" );
    procs.ravenous_afflictions = get_proc( "ravenous_afflictions" );
    procs.umbral_lattice = get_proc( "umbral_lattice" );

    for ( size_t i = 0; i < procs.malefic_rapture.size(); i++ )
    {
      procs.malefic_rapture[ i ] = get_proc( fmt::format( "Malefic Rapture {}", i + 1 ) );
    }
  }

  void warlock_t::init_procs_demonology()
  {
    procs.demonic_core_dogs = get_proc( "demonic_core_dogs" );
    procs.demonic_core_imps = get_proc( "demonic_core_imps" );
    procs.carnivorous_stalkers = get_proc( "carnivorous_stalkers" );
    procs.shadow_invocation = get_proc( "shadow_invocation" );
    procs.imp_gang_boss = get_proc( "imp_gang_boss" );
    procs.spiteful_reconstitution = get_proc( "spiteful_reconstitution" );
    procs.umbral_blaze = get_proc( "umbral_blaze" );
    procs.pact_of_the_imp_mother = get_proc( "pact_of_the_imp_mother" );
    procs.pact_of_the_eredruin = get_proc( "pact_of_the_eredruin" );
    procs.empowered_legion_strike = get_proc( "empowered_legion_strike" );

    for ( size_t i = 0; i < procs.hand_of_guldan_shards.size(); i++ )
    {
      procs.hand_of_guldan_shards[ i ] = get_proc( fmt::format( "Hand of Gul'dan {}", i + 1 ) );
    }
  }

  void warlock_t::init_procs_destruction()
  {
    procs.reverse_entropy = get_proc( "reverse_entropy" );
    procs.rain_of_chaos = get_proc( "rain_of_chaos" );
    procs.decimation = get_proc( "decimation" );
    procs.dimension_ripper = get_proc( "dimension_ripper" );
    procs.echo_of_the_azjaqir = get_proc( "echo_of_the_azjaqir" );
  }

  void warlock_t::init_procs_diabolist()
  {
  }

  void warlock_t::init_procs_hellcaller()
  {
    procs.blackened_soul = get_proc( "blackened_soul" );
    procs.bleakheart_tactics = get_proc( "bleakheart_tactics" );
    procs.seeds_of_their_demise = get_proc( "seeds_of_their_demise" );
    procs.mark_of_perotharn = get_proc( "mark_of_perotharn" );
  }

  void warlock_t::init_procs_soul_harvester()
  {
    procs.succulent_soul = get_proc( "succulent_soul" );
    procs.feast_of_souls = get_proc( "feast_of_souls" );
  }

  void warlock_t::init_rng()
  {
    if ( specialization() == WARLOCK_AFFLICTION )
      init_rng_affliction();
    if ( specialization() == WARLOCK_DEMONOLOGY )
      init_rng_demonology();
    if ( specialization() == WARLOCK_DESTRUCTION )
      init_rng_destruction();

    init_rng_diabolist();
    init_rng_hellcaller();
    init_rng_soul_harvester();

    player_t::init_rng();
  }

  void warlock_t::init_rng_affliction()
  {
    ravenous_afflictions_rng = get_rppm( "ravenous_afflictions", talents.ravenous_afflictions );
  }

  void warlock_t::init_rng_demonology()
  { }

  void warlock_t::init_rng_destruction()
  {
    // TOCHECK: Presumed to use deck of cards at 3 out of 20. Long sample test needed to reconfirm in TWW
    rain_of_chaos_rng = get_shuffled_rng( "rain_of_chaos", 3, 20 );
  }

  void warlock_t::init_rng_diabolist()
  {
  }

  void warlock_t::init_rng_hellcaller()
  {
  }

  void warlock_t::init_rng_soul_harvester()
  {
  }

  void warlock_t::init_resources( bool force )
  {
    player_t::init_resources( force );

    resources.current[ RESOURCE_SOUL_SHARD ] = initial_soul_shards;
  }

  void warlock_t::init_action_list()
  {
    if ( action_list_str.empty() )
    {
      clear_action_priority_lists();

      switch ( specialization() )
      {
      case WARLOCK_AFFLICTION:
        warlock_apl::affliction( this );
        break;
      case WARLOCK_DEMONOLOGY:
        warlock_apl::demonology( this );
        break;
      case WARLOCK_DESTRUCTION:
        warlock_apl::destruction( this );
        break;
      default:
        break;
      }

      use_default_action_list = true;
    }

    player_t::init_action_list();
  }

  void warlock_t::add_rng_option( warlock_t::rng_settings_t::rng_setting_t& setting )
  {
    add_option( opt_float( "rng_" + setting.option_name, setting.setting_value ) );
  }

  void warlock_t::create_options()
  {
    player_t::create_options();

    add_option( opt_int( "soul_shards", initial_soul_shards ) );
    add_option( opt_string( "default_pet", default_pet ) );
    add_option( opt_bool( "disable_felstorm", disable_auto_felstorm ) );
    add_option( opt_bool( "normalize_destruction_mastery", normalize_destruction_mastery ) );

    add_rng_option( rng_settings.cunning_cruelty_sb );
    add_rng_option( rng_settings.cunning_cruelty_ds );
    add_rng_option( rng_settings.agony );
    add_rng_option( rng_settings.nightfall );
    add_rng_option( rng_settings.pact_of_the_eredruin );
    add_rng_option( rng_settings.shadow_invocation );
    add_rng_option( rng_settings.spiteful_reconstitution );
    add_rng_option( rng_settings.decimation );
    add_rng_option( rng_settings.dimension_ripper );
    add_rng_option( rng_settings.blackened_soul );
    add_rng_option( rng_settings.bleakheart_tactics );
    add_rng_option( rng_settings.seeds_of_their_demise );
    add_rng_option( rng_settings.mark_of_perotharn );
    add_rng_option( rng_settings.succulent_soul_aff );
    add_rng_option( rng_settings.succulent_soul_demo );
    add_rng_option( rng_settings.feast_of_souls );
    add_rng_option( rng_settings.umbral_lattice );
    add_rng_option( rng_settings.empowered_legion_strike );
  }

  void warlock_t::combat_begin()
  {
    player_t::combat_begin();

    if ( specialization() == WARLOCK_DEMONOLOGY && buffs.inner_demons && talents.inner_demons->ok() )
    {
      timespan_t start = timespan_t::from_seconds( rng().range( talents.inner_demons->effectN( 1 ).period().total_seconds() ) );
      make_event( sim, start, [ this ] { buffs.inner_demons->trigger(); } );
    }
  }

  void warlock_t::reset()
  {
    player_t::reset();

    range::for_each( sim->target_list, [ this ]( const player_t* t ) {
      if ( auto td = target_data[ t ] )
        td->reset();

      range::for_each( t->pet_list, [ this ]( const player_t* add ) {
        if ( auto td = target_data[ add ] )
          td->reset();
      } );
    } );

    warlock_pet_list.active = nullptr;
    havoc_target = nullptr;
    ua_target = nullptr;
    agony_accumulator = rng().range( 0.0, 0.99 );
    corruption_accumulator = rng().range( 0.0, 0.99 );
    wild_imp_spawns.clear();
    diabolic_ritual = as<int>( rng().range( 0, 3 ) );
  }
}