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
    // If there is no need to check whether a spell is known by the actor, can fall back on find_spell, otherwise use conditional_spell_lookup

    // General
    warlock_base.nethermancy = find_spell( 86091 );
    warlock_base.drain_life = find_class_spell( "Drain Life" ); // Should be ID 234153
    warlock_base.corruption = find_class_spell( "Corruption" ); // Should be ID 172, DoT info is in Effect 1's trigger (146739)
    warlock_base.shadow_bolt = find_class_spell( "Shadow Bolt" ); // Should be ID 686, same for both Affliction and Demonology

    // Affliction
    warlock_base.affliction_warlock = find_specialization_spell( "Affliction Warlock", WARLOCK_AFFLICTION ); // Should be ID 137043
    warlock_base.potent_afflictions = find_mastery_spell( WARLOCK_AFFLICTION ); // Should be ID 77215

    // Demonology
    warlock_base.demonology_warlock = find_specialization_spell( "Demonology Warlock", WARLOCK_DEMONOLOGY ); // Should be ID 137044
    warlock_base.master_demonologist = find_mastery_spell( WARLOCK_DEMONOLOGY ); // Should be ID 77219
    warlock_base.wild_imp = conditional_spell_lookup( warlock_base.demonology_warlock->ok(), 104317 ); // Contains pet summoning information (HoG)
    warlock_base.wild_imp_2 = conditional_spell_lookup( warlock_base.demonology_warlock->ok(), 279910 ); // Pet summoning information for Inner Demons, Spiteful Reconstitution and To Hell and Back
    warlock_base.fel_firebolt_2 = conditional_spell_lookup( warlock_base.demonology_warlock->ok(), 334591 ); // 20% cost reduction for Wild Imps
    warlock_base.infernal_command_buff = conditional_spell_lookup( warlock_base.demonology_warlock->ok(), 387552 ); // Buff of an old talent that still applies but with 0 value
    warlock_base.shadow_bolt_energize = conditional_spell_lookup( warlock_base.demonology_warlock->ok(), 194192 ); // Used for resource gain

    // Destruction
    warlock_base.destruction_warlock = find_specialization_spell( "Destruction Warlock", WARLOCK_DESTRUCTION ); // Should be ID 137046
    warlock_base.chaotic_energies = find_mastery_spell( WARLOCK_DESTRUCTION ); // Should be ID 77220
    warlock_base.immolate = find_specialization_spell( "Immolate" ); // Should be ID 193541
    warlock_base.immolate_old = conditional_spell_lookup( warlock_base.destruction_warlock->ok(), 348 ); // This contains the actual direct damage and cast data, but no longer appears in class_spell list
    warlock_base.immolate_dot = conditional_spell_lookup( warlock_base.destruction_warlock->ok(), 157736 ); // DoT data
    warlock_base.incinerate = conditional_spell_lookup( warlock_base.destruction_warlock->ok(), 29722 ); // Should be ID 29722
    warlock_base.incinerate_energize = conditional_spell_lookup( warlock_base.destruction_warlock->ok(), 244670 ); // Used for resource gain information

    warlock_t::init_spells_affliction();
    warlock_t::init_spells_demonology();
    warlock_t::init_spells_destruction();

    // Talents
    talents.demonic_embrace = find_talent_spell( talent_tree::CLASS, "Demonic Embrace" ); // Should be ID 288843

    talents.demonic_fortitude = find_talent_spell( talent_tree::CLASS, "Demonic Fortitude" ); // Should be ID 386617

    talents.pact_of_the_annihilan = find_talent_spell( talent_tree::CLASS, "Pact of the Annihilan" ); // Should be ID 1270693

    talents.pact_of_the_satyr = find_talent_spell( talent_tree::CLASS, "Pact of the Satyr" ); // Should be ID 1270691

    talents.pact_of_the_eredar = find_talent_spell( talent_tree::CLASS, "Pact of the Eredar" ); // Should be ID 1270695

    talents.soulburn = find_talent_spell( talent_tree::CLASS, "Soulburn" ); // Should be ID 385899
    talents.soulburn_buff = conditional_spell_lookup( talents.soulburn.ok(), 387626 );

    talents.summoners_embrace = find_talent_spell( talent_tree::SPECIALIZATION, "Summoner's Embrace" ); // Should be ID 453105

    talents.grimoire_of_sacrifice = find_talent_spell( talent_tree::SPECIALIZATION, "Grimoire of Sacrifice" ); // Aff/Destro only. Should be ID 108503
    talents.grimoire_of_sacrifice_buff = conditional_spell_lookup( talents.grimoire_of_sacrifice.ok(), 196099 ); // Buff data and RPPM
    talents.grimoire_of_sacrifice_proc = conditional_spell_lookup( talents.grimoire_of_sacrifice.ok(), 196100 ); // Damage data

    warlock_t::init_spells_diabolist();
    warlock_t::init_spells_hellcaller();
    warlock_t::init_spells_soul_harvester();

    warlock_t::init_proc_data_entries();

    // Register passives
    // NOTE: 2026-02-17 Currently Gloom of Nathreza talent is bugged for Destruction and does not work
    if ( destruction() && bugs )
      deregister_passive_effect( hero.gloom_of_nathreza->effectN( 2 ) );

    register_passive_effect_mask( hero.mark_of_xavius,
      affliction() ? effect_mask_t( false ).enable( 1, 3 ) : effect_mask_t( true ).disable( 1 ) );

    // Shadowbolt Volley affected by Improved Shadow Bolt but not in the spell data whitelist
    register_passive_affect_list( talents.improved_shadow_bolt, affect_list_t( 2 ).add_spell( 453176 ) );

    // Soul Swipe affected by Wicked Reaping but not in the spell data whitelist
    register_passive_affect_list( hero.wicked_reaping, affect_list_t( 1 ).add_spell( 1269049 ) );

    // NOTE: 2026-02-20 Agony (Malefic Grasp extra tick) is not affected by Shared Agony, Sudden Onset or Mark of Xavius
    // because these do not have an effect that affects direct damage (they only affect periodic damage) (bug?)
    // However, it seems to do almost twice dmg in compensation (bug?) (implemented in agony_mg_t)

    // NOTE: 2026-02-20 Agony (Malefic Grasp extra tick) not included in Mastery: Potent Afflictions (#2) whitelist (bug?)
    if ( affliction() && !bugs )
      register_passive_affect_list( warlock_base.potent_afflictions, affect_list_t( 2 ).add_spell( 1261166 ) );

    // NOTE: 2026-02-20 Agony (Malefic Grasp extra tick) not included in Niskaran Methods (#1) whitelist (bug?)
    if ( affliction() && !bugs )
      register_passive_affect_list( talents.niskaran_methods, affect_list_t( 1 ).add_spell( 1261166 ) );

    // NOTE: 2026-02-20 Agony (MG extra tick) and Wither (MG extra tick) not included in Summoner's Embrace (#1, #3) whitelist (bug?)
    if ( affliction() && !bugs )
      register_passive_affect_list( talents.summoners_embrace, affect_list_t( 1, 3 ).add_spell( 1261166, 1279686 ) );

    // NOTE: 2026-02-20 Wither (Malefic Grasp extra tick) not included in Affliction Warlock (#9, #10) whitelist (bug?)
    if ( affliction() && !bugs )
      register_passive_affect_list( warlock_base.affliction_warlock, affect_list_t( 9, 10 ).add_spell( 1279686 ) );

    // NOTE: 2026-02-20 Wither (Malefic Grasp extra tick) not included in Xalan's Ferocity (#1, #2, #4) whitelist (bug?)
    if ( affliction() && !bugs )
      register_passive_affect_list( hero.xalans_ferocity, affect_list_t( 1, 2, 4 ).add_spell( 1279686 ) );

    // NOTE: 2026-02-20 Wither (Hatefury Rituals extra tick) not included in Xalan's Ferocity (#1) whitelist (bug?)
    // Furthermore, Hatefury Rituals does not have an effect that affects direct damage (it only affects periodic damage) (bug?)
    if ( affliction() && !bugs )
      register_passive_affect_list( hero.hatefury_rituals, affect_list_t( 1 ).add_spell( 1279686 ) );

    parse_all_class_passives();
    parse_all_passive_talents();
    parse_all_passive_sets();
    parse_raid_buffs();

    // NOTE: 2026-02-17 Mark of Perotharn is being applied twice in what appears to be a bug
    if ( bugs )
      parse_passive_effects( hero.mark_of_perotharn, true );
  }

  void warlock_t::init_spells_affliction()
  {
    // Talents
    talents.agony = find_talent_spell( talent_tree::SPECIALIZATION, "Agony" ); // Should be ID 980
    talents.agony_energize = conditional_spell_lookup( talents.agony.ok(), 17941 );

    talents.unstable_affliction = find_talent_spell( talent_tree::SPECIALIZATION, "Unstable Affliction" ); // Should be ID 1259790
    talents.unstable_affliction_2 = conditional_spell_lookup( talents.unstable_affliction.ok(), 231791 ); // Soul Shard on demise

    talents.seed_of_corruption = find_talent_spell( talent_tree::SPECIALIZATION, "Seed of Corruption" ); // Should be ID 27243
    talents.seed_of_corruption_aoe = conditional_spell_lookup( talents.seed_of_corruption.ok(), 27285 ); // Explosion damage
    talents.seed_of_corruption_is_out_dnt = conditional_spell_lookup( talents.seed_of_corruption.ok(), 1279998 );

    talents.nightfall = find_talent_spell( talent_tree::SPECIALIZATION, "Nightfall" ); // Should be ID 108558
    talents.nightfall_buff = conditional_spell_lookup( warlock_base.affliction_warlock->ok(), 264571 );
    talents.nightfall_buff_2 = conditional_spell_lookup( warlock_base.affliction_warlock->ok(), 1260279 );

    talents.haunt = find_talent_spell( talent_tree::SPECIALIZATION, "Haunt" ); // Should be ID 48181

    talents.shared_agony = find_talent_spell( talent_tree::SPECIALIZATION, "Shared Agony" ); // Should be ID 1259825

    talents.improved_shadow_bolt = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Shadow Bolt" ); // Should be ID 453080

    talents.drain_soul = find_talent_spell( talent_tree::SPECIALIZATION, "Drain Soul" ); // Should be ID 388667
    talents.drain_soul_dot = conditional_spell_lookup( talents.drain_soul.ok(), 198590 ); // This contains all the channel data

    talents.improved_haunt = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Haunt" ); // Should be ID 458034

    talents.absolute_corruption = find_talent_spell( talent_tree::SPECIALIZATION, "Absolute Corruption" ); // Should be ID 196103

    talents.siphon_life = find_talent_spell( talent_tree::SPECIALIZATION, "Siphon Life" ); // Should be ID 452999

    talents.cunning_cruelty = find_talent_spell( talent_tree::SPECIALIZATION, "Cunning Cruelty" ); // Should be ID 453172
    talents.shadowbolt_volley = conditional_spell_lookup( talents.cunning_cruelty.ok(), 453176 );

    talents.withering_bolt = find_talent_spell( talent_tree::SPECIALIZATION, "Withering Bolt" ); // Should be ID 386976

    talents.creeping_death = find_talent_spell( talent_tree::SPECIALIZATION, "Creeping Death" ); // Should be ID 264000

    talents.dark_harvest = find_talent_spell( talent_tree::SPECIALIZATION, "Dark Harvest" ); // Should be ID 1257052
    talents.dark_harvest_dmg = conditional_spell_lookup( talents.dark_harvest.ok(), 1257065 );

    talents.practiced_pestilence = find_talent_spell( talent_tree::SPECIALIZATION, "Practiced Pestilence" ); // Should be ID 1259811

    talents.summon_darkglare = find_talent_spell( talent_tree::SPECIALIZATION, "Summon Darkglare" ); // Should be ID 205180
    talents.eye_beam = conditional_spell_lookup( talents.summon_darkglare.ok(), 205231 );
    talents.darkglare_presence_buff = conditional_spell_lookup( talents.summon_darkglare.ok(), 1280663 );

    talents.cull_the_weak = find_talent_spell( talent_tree::SPECIALIZATION, "Cull the Weak" ); // Should be ID 1259886

    talents.malediction = find_talent_spell( talent_tree::SPECIALIZATION, "Malediction" ); // Should be ID 453087

    talents.sudden_onset = find_talent_spell( talent_tree::SPECIALIZATION, "Sudden Onset" ); // Should be ID 1260209

    talents.eye_contract = find_talent_spell( talent_tree::SPECIALIZATION, "Eye Contract" ); // Should be ID 1279521

    talents.malefic_grasp = find_talent_spell( talent_tree::SPECIALIZATION, "Malefic Grasp" ); // Should be ID 1261149
    talents.malefic_grasp_2 = conditional_spell_lookup( talents.malefic_grasp.ok(), 1261153 );
    talents.malefic_grasp_3 = conditional_spell_lookup( talents.malefic_grasp.ok(), 1279659 );
    talents.agony_mg = conditional_spell_lookup( talents.malefic_grasp.ok() && talents.agony.ok(), 1261166 );
    talents.unstable_affliction_mg = conditional_spell_lookup( talents.malefic_grasp.ok() && talents.unstable_affliction.ok(), 1261176 );
    talents.corruption_mg = conditional_spell_lookup( talents.malefic_grasp.ok(), 1261158 );
    talents.wither_mg = conditional_spell_lookup( talents.malefic_grasp.ok(), 1279686 );

    talents.nether_plating = find_talent_spell( talent_tree::SPECIALIZATION, "Nether Plating" ); // Should be ID 1280733

    talents.sacrolashs_dark_strike = find_talent_spell( talent_tree::SPECIALIZATION, "Sacrolash's Dark Strike" ); // Should be ID 386986

    talents.contagion = find_talent_spell( talent_tree::SPECIALIZATION, "Contagion" ); // Should be ID 453096

    talents.shard_instability = find_talent_spell( talent_tree::SPECIALIZATION, "Shard Instability" ); // Should be ID 1260264
    talents.shard_instability_buff = conditional_spell_lookup( warlock_base.affliction_warlock->ok(), 1260269 );

    talents.niskaran_methods = find_talent_spell( talent_tree::SPECIALIZATION, "Niskaran Methods" ); // Should be ID 1279510

    talents.potent_soul_shards = find_talent_spell( talent_tree::SPECIALIZATION, "Potent Soul Shards" ); // Should be ID 1259815

    talents.nocturnal_yield = find_talent_spell( talent_tree::SPECIALIZATION, "Nocturnal Yield" ); // Should be ID 1260271

    talents.xavius_gambit = find_talent_spell( talent_tree::SPECIALIZATION, "Xavius' Gambit" ); // Should be ID 416615

    talents.ravenous_afflictions = find_talent_spell( talent_tree::SPECIALIZATION, "Ravenous Afflictions" ); // Should be ID 459440

    talents.seeds_of_destruction = find_talent_spell( talent_tree::SPECIALIZATION, "Seeds of Destruction" ); // Should be ID 1259838

    talents.fatal_echoes = find_talent_spell( talent_tree::SPECIALIZATION, "Fatal Echoes" ); // Should be ID 1260229

    talents.cascading_calamity = find_talent_spell( talent_tree::SPECIALIZATION, "Cascading Calamity" ); // Should be ID 1261124
    talents.cascading_calamity_buff = conditional_spell_lookup( talents.cascading_calamity.ok(), 1261125 );

    talents.deaths_embrace = find_talent_spell( talent_tree::SPECIALIZATION, "Death's Embrace" ); // Should be ID 234876

    talents.patient_zero = find_talent_spell( talent_tree::SPECIALIZATION, "Patient Zero" ); // Should be ID 1260285

    talents.sow_the_seeds = find_talent_spell( talent_tree::SPECIALIZATION, "Sow the Seeds" ); // Should be ID 196226

    // Affliction Apex
    talents.shadow_of_nathreza_1 = find_talent_spell( talent_tree::SPECIALIZATION, "Shadow of Nathreza", 1 ); // Should be ID 1261984 (I)
    talents.shadow_of_nathreza_2 = find_talent_spell( talent_tree::SPECIALIZATION, "Shadow of Nathreza", 2 ); // Should be ID 1261990 (II)
    talents.shadow_of_nathreza_3 = find_talent_spell( talent_tree::SPECIALIZATION, "Shadow of Nathreza", 3 ); // Should be ID 1261992 (III)
    talents.shadow_of_nathreza_dot = conditional_spell_lookup( talents.shadow_of_nathreza_1.ok(), 1262710 );
    talents.summon_desperate_soul = conditional_spell_lookup( talents.shadow_of_nathreza_3.ok(), 1262094 );
    talents.wrath_of_nathreza = conditional_spell_lookup( talents.shadow_of_nathreza_3.ok(), 1262028 );
    talents.wrath_of_nathreza_impact = conditional_spell_lookup( talents.shadow_of_nathreza_3.ok(), 1278047 );

    // Additional Tier Set spell data
    tier.wl_affliction_12_0_class_set_2pc = sets->set( WARLOCK_AFFLICTION, MID1, B2 ); // Should be ID 1264869
    tier.wl_affliction_12_0_class_set_4pc = sets->set( WARLOCK_AFFLICTION, MID1, B4 ); // Should be ID 1264870

    // Initialize some default values for pet spawners
    warlock_pet_list.darkglares.set_default_duration( talents.summon_darkglare->duration() );
    warlock_pet_list.desperate_souls.set_default_duration( talents.summon_desperate_soul->duration() );
  }

  void warlock_t::init_spells_demonology()
  {
    // Talents
    talents.hand_of_guldan = find_talent_spell( talent_tree::SPECIALIZATION, "Hand of Gul'dan" ); // Should be ID 1250273
    talents.hand_of_guldan_cast = conditional_spell_lookup( talents.hand_of_guldan.ok(), 105174 );
    talents.hog_impact = conditional_spell_lookup( talents.hand_of_guldan.ok(), 86040 ); // Contains impact damage data

    talents.demoniac = find_talent_spell( talent_tree::SPECIALIZATION, "Demoniac" ); // Should be ID 426115
    talents.demonbolt_spell = conditional_spell_lookup( talents.demoniac.ok(), 264178 );
    talents.demonbolt_energize = conditional_spell_lookup( talents.demoniac.ok(), 280127 );
    talents.demonic_core_spell = conditional_spell_lookup( warlock_base.demonology_warlock->ok(), 267102 );
    talents.demonic_core_buff = conditional_spell_lookup( warlock_base.demonology_warlock->ok(), 264173 );

    talents.call_dreadstalkers = find_talent_spell( talent_tree::SPECIALIZATION, "Call Dreadstalkers" ); // Should be ID 104316
    talents.call_dreadstalkers_summon_1 = conditional_spell_lookup( warlock_base.demonology_warlock->ok(), 193331 ); // Summon data
    talents.call_dreadstalkers_summon_2 = conditional_spell_lookup( warlock_base.demonology_warlock->ok(), 193332 ); // Summon data

    talents.dominant_hand = find_talent_spell( talent_tree::SPECIALIZATION, "Dominant Hand" ); // Should be ID 1276433

    talents.fel_intellect = find_talent_spell( talent_tree::SPECIALIZATION, "Fel Intellect" ); // Should be ID 1250372

    talents.practiced_rituals = find_talent_spell( talent_tree::SPECIALIZATION, "Practiced Rituals" ); // Should be ID 1250375

    talents.dreadlash = find_talent_spell( talent_tree::SPECIALIZATION, "Dreadlash" ); // Should be ID 264078

    talents.imperator = find_talent_spell( talent_tree::SPECIALIZATION, "Imp-erator" ); // Should be ID 416230

    talents.implosion = find_talent_spell( talent_tree::SPECIALIZATION, "Implosion" ); // Should be ID 196277
    talents.implosion_aoe = conditional_spell_lookup( talents.implosion.ok(), 196278 );

    talents.power_siphon = find_talent_spell( talent_tree::SPECIALIZATION, "Power Siphon" ); // Should be ID 264130
    talents.power_siphon_buff = conditional_spell_lookup( talents.power_siphon.ok(), 334581 );

    talents.summon_felguard = find_talent_spell( talent_tree::SPECIALIZATION, "Summon Felguard" ); // Should be ID 30146

    talents.infernal_rapidity = find_talent_spell( talent_tree::SPECIALIZATION, "Infernal Rapidity" ); // Should be ID 1263941;

    talents.rune_of_shadows = find_talent_spell( talent_tree::SPECIALIZATION, "Rune of Shadows" ); // Should be ID 453744;

    talents.carnivorous_stalkers = find_talent_spell( talent_tree::SPECIALIZATION, "Carnivorous Stalkers" ); // Should be ID 386194;

    talents.fel_armaments = find_talent_spell( talent_tree::SPECIALIZATION, "Fel Armaments" ); // Should be ID 1263935

    talents.imp_gang_boss = find_talent_spell( talent_tree::SPECIALIZATION, "Imp Gang Boss" ); // Should be ID 1250768

    talents.demonic_brutality = find_talent_spell( talent_tree::SPECIALIZATION, "Demonic Brutality" ); // Should be ID 453908

    talents.inner_demons = find_talent_spell( talent_tree::SPECIALIZATION, "Inner Demons" ); // Should be ID 267216

    talents.summon_demonic_tyrant = find_talent_spell( talent_tree::SPECIALIZATION, "Summon Demonic Tyrant" ); // Should be ID 265187
    talents.demonic_power_buff = conditional_spell_lookup( talents.summon_demonic_tyrant.ok(), 1276788 );

    talents.blighted_maw = find_talent_spell( talent_tree::SPECIALIZATION, "Blighted Maw" ); // Should be ID 1276956
    talents.blighted_maw_dmg = conditional_spell_lookup( talents.blighted_maw.ok(), 1276960 );

    talents.improved_demonic_tactics = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Demonic Tactics" ); // Should be ID 453800

    talents.empowered_felstorm = find_talent_spell( talent_tree::SPECIALIZATION, "Empowered Felstorm" ); // Should be ID 1279575

    talents.spiteful_reconstitution = find_talent_spell( talent_tree::SPECIALIZATION, "Spiteful Reconstitution" ); // Should be ID 428394

    talents.tyrants_oblation = find_talent_spell( talent_tree::SPECIALIZATION, "Tyrant's Oblation" ); // Should be ID 1276746
    talents.tyrants_oblation_buff = conditional_spell_lookup( talents.tyrants_oblation.ok(), 1276767 );

    talents.antoran_armaments = find_talent_spell( talent_tree::SPECIALIZATION, "Antoran Armaments" ); // Should be ID 1250921

    talents.flametouched = find_talent_spell( talent_tree::SPECIALIZATION, "Flametouched" ); // Should be ID 453699
    talents.flametouched_buff = conditional_spell_lookup( talents.flametouched.ok(), 453704 );

    talents.demonic_knowledge = find_talent_spell( talent_tree::SPECIALIZATION, "Demonic Knowledge" ); // Should be ID 386185

    talents.sacrificed_souls = find_talent_spell( talent_tree::SPECIALIZATION, "Sacrificed Souls" ); // Should be ID 267214

    talents.reign_of_tyranny = find_talent_spell( talent_tree::SPECIALIZATION, "Reign of Tyranny" ); // Should be ID 1276748

    talents.master_summoner = find_talent_spell( talent_tree::SPECIALIZATION, "Master Summoner" ); // Should be ID 1240189

    talents.demonic_calling = find_talent_spell( talent_tree::SPECIALIZATION, "Demonic Calling" ); // Should be ID 1276947

    talents.doom = find_talent_spell( talent_tree::SPECIALIZATION, "Doom" ); // Should be ID 460551
    talents.doom_debuff = conditional_spell_lookup( talents.doom.ok(), 460553 );
    talents.doom_dmg = conditional_spell_lookup( talents.doom.ok(), 460555 );

    talents.hellbent_commander = find_talent_spell( talent_tree::SPECIALIZATION, "Hellbent Commander" ); // Should be ID 1250897
    talents.hellbent_commander_buff = conditional_spell_lookup( talents.hellbent_commander.ok(), 1281559 );

    talents.grimoire_imp_lord = find_talent_spell( talent_tree::SPECIALIZATION, "Grimoire: Imp Lord" ); // Should be ID 1276452

    talents.grimoire_fel_ravager = find_talent_spell( talent_tree::SPECIALIZATION, "Grimoire: Fel Ravager" ); // Should be ID 1276467

    talents.grimoire_of_service_buff = conditional_spell_lookup( talents.grimoire_imp_lord.ok() || talents.grimoire_fel_ravager.ok(), 216187 );

    talents.summon_vilefiend = find_talent_spell( talent_tree::SPECIALIZATION, "Summon Vilefiend" ); // Should be ID 1251778
    talents.vilefiend = conditional_spell_lookup( talents.summon_vilefiend.ok(), 1251781 );
    talents.bile_spit = conditional_spell_lookup( talents.summon_vilefiend.ok(), 267997 );
    talents.headbutt = conditional_spell_lookup( talents.summon_vilefiend.ok(), 267999 );

    talents.summon_doomguard = find_talent_spell( talent_tree::SPECIALIZATION, "Summon Doomguard" ); // Should be ID 1276672
    talents.doom_bolt_volley = conditional_spell_lookup( talents.summon_doomguard.ok(), 1251989 );

    talents.to_hell_and_back = find_talent_spell( talent_tree::SPECIALIZATION, "To Hell and Back" ); // Should be ID 1281511
    talents.unstable_soul_buff = conditional_spell_lookup( talents.to_hell_and_back.ok(), 1281512 );
    talents.imp_gang_boss_buff = conditional_spell_lookup( talents.imp_gang_boss.ok() || talents.to_hell_and_back.ok(), 1250772 );

    talents.stabilized_portals = find_talent_spell( talent_tree::SPECIALIZATION, "Stabilized Portals" ); // Should be ID 1276661

    talents.mark_of_shatug = find_talent_spell( talent_tree::SPECIALIZATION, "Mark of Shatug" ); // Should be ID 455449
    talents.gloomhound = conditional_spell_lookup( talents.mark_of_shatug.ok(), 455465 );
    talents.gloom_slash = conditional_spell_lookup( talents.mark_of_shatug.ok(), 455491 );

    talents.mark_of_fharg = find_talent_spell( talent_tree::SPECIALIZATION, "Mark of F'harg" ); // Should be ID 455450
    talents.charhound = conditional_spell_lookup( talents.mark_of_fharg.ok(), 455476 );
    talents.infernal_presence = conditional_spell_lookup( talents.mark_of_fharg.ok(), 428453 );
    talents.infernal_presence_dmg = conditional_spell_lookup( talents.mark_of_fharg.ok(), 428455 );

    talents.dominion_of_argus_1 = find_talent_spell( talent_tree::SPECIALIZATION, "Dominion of Argus", 1 ); // Should be ID 1276163 (I)
    talents.dominion_of_argus_2 = find_talent_spell( talent_tree::SPECIALIZATION, "Dominion of Argus", 2 ); // Should be ID 1276190 (II)
    talents.dominion_of_argus_3 = find_talent_spell( talent_tree::SPECIALIZATION, "Dominion of Argus", 3 ); // Should be ID 1276222 (III)
    talents.dominion_of_argus_1_buff = conditional_spell_lookup( talents.dominion_of_argus_1.ok(), 1276166 );
    talents.dominion_of_argus_3_gain = conditional_spell_lookup( talents.dominion_of_argus_3.ok(), 1276318 );
    talents.doa_lady_sacrolash_summon = conditional_spell_lookup( talents.dominion_of_argus_1.ok(), 1282501 );
    talents.doa_grand_warlock_alythess_summon = conditional_spell_lookup( talents.dominion_of_argus_1.ok(), 1282502 );
    talents.doa_antoran_inquisitor_summon = conditional_spell_lookup( talents.dominion_of_argus_1.ok(), 1276283 );
    talents.doa_antoran_jailer_summon = conditional_spell_lookup( talents.dominion_of_argus_1.ok(), 1276182 );

    // Additional Tier Set spell data
    tier.wl_demonology_12_0_class_set_2pc = sets->set( WARLOCK_DEMONOLOGY, MID1, B2 ); // Should be ID 1264871
    tier.wl_demonology_12_0_class_set_4pc = sets->set( WARLOCK_DEMONOLOGY, MID1, B4 ); // Should be ID 1264872

    // Initialize some default values for pet spawners
    warlock_pet_list.wild_imps.set_default_duration( warlock_base.wild_imp->duration() );
    warlock_pet_list.dreadstalkers.set_default_duration( talents.call_dreadstalkers_summon_2->duration() );
    warlock_pet_list.demonic_tyrants.set_default_duration( talents.summon_demonic_tyrant->duration() );
    warlock_pet_list.grimoire_imp_lords.set_default_duration( talents.grimoire_imp_lord->duration() );
    warlock_pet_list.grimoire_fel_ravagers.set_default_duration( talents.grimoire_fel_ravager->duration() );
    warlock_pet_list.vilefiends.set_default_duration( talents.vilefiend->duration() );
    warlock_pet_list.doomguards.set_default_duration( talents.summon_doomguard->duration() );
  }

  void warlock_t::init_spells_destruction()
  {
    // Talents
    talents.chaos_bolt = find_talent_spell( talent_tree::SPECIALIZATION, "Chaos Bolt" ); // Should be ID 116858

    talents.conflagrate = find_talent_spell( talent_tree::SPECIALIZATION, "Conflagrate" ); // Should be ID 17962
    talents.conflagrate_2 = conditional_spell_lookup( talents.conflagrate.ok(), 245330 );

    talents.rain_of_fire = find_talent_spell( talent_tree::SPECIALIZATION, 5740 ); // Targeting reticle version
    if ( talents.rain_of_fire == spell_data_t::not_found() )
     talents.rain_of_fire = find_talent_spell( talent_tree::SPECIALIZATION, 1214467 ); // If targeting version not found, fall back to checking for on-target version
    talents.rain_of_fire_tick = conditional_spell_lookup( talents.rain_of_fire.ok(), 42223 );

    talents.improved_conflagrate = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Conflagrate" ); // Should be ID 231793

    talents.backdraft = find_talent_spell( talent_tree::SPECIALIZATION, "Backdraft" ); // Should be ID 196406
    talents.backdraft_buff = conditional_spell_lookup( talents.backdraft.ok(), 117828 );

    talents.practiced_chaos = find_talent_spell( talent_tree::SPECIALIZATION, "Practiced Chaos" ); // Should be ID 1244284

    talents.roaring_blaze = find_talent_spell( talent_tree::SPECIALIZATION, "Roaring Blaze" ); // Should be ID 1244310

    talents.explosive_potential = find_talent_spell( talent_tree::SPECIALIZATION, "Explosive Potential" ); // Should be ID 388827

    talents.mayhem = find_talent_spell( talent_tree::SPECIALIZATION, "Mayhem" ); // Should be ID 387506

    talents.havoc = find_talent_spell( talent_tree::SPECIALIZATION, "Havoc" ); // Should be spell 80240
    talents.havoc_debuff = conditional_spell_lookup( talents.mayhem.ok() || talents.havoc.ok(), 80240 );

    talents.scalding_flames = find_talent_spell( talent_tree::SPECIALIZATION, "Scalding Flames" ); // Should be ID 388832

    talents.shadowburn = find_talent_spell( talent_tree::SPECIALIZATION, "Shadowburn" ); // Should be ID 17877
    talents.shadowburn_2 = conditional_spell_lookup( talents.shadowburn.ok(), 245731 );

    talents.backlash = find_talent_spell( talent_tree::SPECIALIZATION, "Backlash" ); // Should be ID 387384

    talents.improved_havoc = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Havoc" ); // Should be ID 1244460

    talents.ashen_remains = find_talent_spell( talent_tree::SPECIALIZATION, "Ashen Remains" ); // Should be ID 387252

    talents.cataclysm = find_talent_spell( talent_tree::SPECIALIZATION, "Cataclysm" ); // Should be ID 152108

    talents.fiendish_cruelty = find_talent_spell( talent_tree::SPECIALIZATION, "Fiendish Cruelty" ); // Should be ID 1245633
    talents.fiendish_cruelty_buff = conditional_spell_lookup( talents.fiendish_cruelty.ok(), 1245664 );

    talents.chaotic_inferno = find_talent_spell( talent_tree::SPECIALIZATION, "Chaotic Inferno" ); // Should be ID 1244788
    talents.chaotic_inferno_buff = conditional_spell_lookup( talents.chaotic_inferno.ok(), 1244860 );

    talents.flashpoint = find_talent_spell( talent_tree::SPECIALIZATION, "Flashpoint" ); // Should be 387259
    talents.flashpoint_buff = conditional_spell_lookup( warlock_base.destruction_warlock->ok(), 387263 );

    talents.summon_infernal = find_talent_spell( talent_tree::SPECIALIZATION, "Summon Infernal" ); // Should be ID 1122
    talents.summon_infernal_main = conditional_spell_lookup( warlock_base.destruction_warlock->ok(), 111685 );
    talents.infernal_awakening = conditional_spell_lookup( warlock_base.destruction_warlock->ok(), 22703 );
    talents.immolation_buff = conditional_spell_lookup( warlock_base.destruction_warlock->ok(), 19483 );
    talents.immolation_dmg = conditional_spell_lookup( warlock_base.destruction_warlock->ok(), 20153 );
    talents.embers = conditional_spell_lookup( warlock_base.destruction_warlock->ok(), 264364 );
    talents.burning_ember = conditional_spell_lookup( warlock_base.destruction_warlock->ok(), 264365 );

    talents.emberstorm = find_talent_spell( talent_tree::SPECIALIZATION, "Emberstorm" ); // Should be ID 454744

    talents.fire_and_brimstone = find_talent_spell( talent_tree::SPECIALIZATION, "Fire and Brimstone" ); // Should be ID 196408

    talents.lake_of_fire = find_talent_spell( talent_tree::SPECIALIZATION, "Lake of Fire" ); // Should be ID 1244877
    talents.lake_of_fire_aoe = conditional_spell_lookup( talents.lake_of_fire.ok(), 1244885 );
    talents.lake_of_fire_tick = conditional_spell_lookup( talents.lake_of_fire.ok(), 1244890 );
    talents.lake_of_fire_debuff = conditional_spell_lookup( talents.lake_of_fire.ok(), 1244918 );

    talents.reverse_entropy = find_talent_spell( talent_tree::SPECIALIZATION, "Reverse Entropy" ); // Should be ID 205148
    talents.reverse_entropy_buff = conditional_spell_lookup( talents.reverse_entropy.ok(), 266030 );

    talents.internal_combustion = find_talent_spell( talent_tree::SPECIALIZATION, "Internal Combustion" ); // Should be ID 266134
    talents.internal_combustion_dmg = conditional_spell_lookup( talents.internal_combustion.ok(), 266136 );

    talents.crashing_chaos = find_talent_spell( talent_tree::SPECIALIZATION, "Crashing Chaos" ); // Should be ID 417234
    talents.crashing_chaos_buff = conditional_spell_lookup( talents.crashing_chaos.ok(), 417282 );

    talents.rain_of_chaos = find_talent_spell( talent_tree::SPECIALIZATION, "Rain of Chaos" ); // Should be ID 266086
    talents.rain_of_chaos_buff = conditional_spell_lookup( talents.rain_of_chaos.ok(), 266087 );
    talents.summon_infernal_roc = conditional_spell_lookup( talents.rain_of_chaos.ok(), 335236 );

    talents.ruin = find_talent_spell( talent_tree::SPECIALIZATION, "Ruin" ); // Should be ID 387103

    talents.improved_chaos_bolt = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Chaos Bolt" ); // Should be ID 456951

    talents.destructive_rapidity = find_talent_spell( talent_tree::SPECIALIZATION, "Destructive Rapidity" ); // Should be ID 1244928

    talents.devastation = find_talent_spell( talent_tree::SPECIALIZATION, "Devastation" ); // Should be ID 454735

    talents.dimensional_rift = find_talent_spell( talent_tree::SPECIALIZATION, "Dimensional Rift" ); // Should be ID 1280868
    talents.shadowy_tear_summon = conditional_spell_lookup( talents.dimensional_rift.ok(), 394235 );
    talents.shadow_barrage = conditional_spell_lookup( talents.dimensional_rift.ok(), 394237 );
    talents.rift_shadow_bolt = conditional_spell_lookup( talents.dimensional_rift.ok(), 394238 );
    talents.unstable_tear_summon = conditional_spell_lookup( talents.dimensional_rift.ok(), 387979 );
    talents.chaos_barrage = conditional_spell_lookup( talents.dimensional_rift.ok(), 387984 );
    talents.chaos_barrage_tick = conditional_spell_lookup( talents.dimensional_rift.ok(), 387985 );
    talents.chaos_tear_summon = conditional_spell_lookup( talents.dimensional_rift.ok(), 394243 );
    talents.rift_chaos_bolt = conditional_spell_lookup( talents.dimensional_rift.ok(), 394246 );

    talents.soul_fire = find_talent_spell( talent_tree::SPECIALIZATION, "Soul Fire" ); // Should be ID 6353
    talents.soul_fire_2 = conditional_spell_lookup( talents.soul_fire.ok(), 281490 );

    talents.chaos_incarnate = find_talent_spell( talent_tree::SPECIALIZATION, "Chaos Incarnate" ); // Should be ID 387275

    talents.conflagration_of_chaos = find_talent_spell( talent_tree::SPECIALIZATION, "Conflagration of Chaos" ); // Should be ID 387108
    talents.conflagration_of_chaos_buff = conditional_spell_lookup( talents.conflagration_of_chaos.ok(), 387109 );

    talents.diabolic_embers = find_talent_spell( talent_tree::SPECIALIZATION, "Diabolic Embers" ); // Should be ID 387173

    talents.demonfire_infusion = find_talent_spell( talent_tree::SPECIALIZATION, "Demonfire Infusion" ); // Should be ID 1214442

    talents.channel_demonfire = find_talent_spell( talent_tree::SPECIALIZATION, "Channel Demonfire" ); // Should be ID 196447
    talents.channel_demonfire_tick = conditional_spell_lookup( talents.demonfire_infusion.ok() || talents.channel_demonfire.ok(), 196448 ); // Includes both direct and splash damage values
    talents.channel_demonfire_travel = conditional_spell_lookup( talents.demonfire_infusion.ok() || talents.channel_demonfire.ok(), 196449 );

    talents.avatar_of_destruction = find_talent_spell( talent_tree::SPECIALIZATION, "Avatar of Destruction" ); // Should be ID 1245089
    talents.summon_overfiend = conditional_spell_lookup( talents.avatar_of_destruction.ok(), 434587 );
    talents.overfiend_buff = conditional_spell_lookup( talents.avatar_of_destruction.ok(), 457578 );
    talents.overfiend_cb = conditional_spell_lookup( talents.avatar_of_destruction.ok(), 434589 );

    talents.inferno = find_talent_spell( talent_tree::SPECIALIZATION, "Inferno" ); // Should be ID 1280483

    talents.alythesss_ire = find_talent_spell( talent_tree::SPECIALIZATION, "Alythess's Ire" ); // Should be ID 1244941
    talents.alythesss_ire_buff = conditional_spell_lookup( talents.alythesss_ire.ok(), 1244947 );

    talents.raging_demonfire = find_talent_spell( talent_tree::SPECIALIZATION, "Raging Demonfire" ); // Should be ID 387166

    talents.embers_of_nihilam_1 = find_talent_spell( talent_tree::SPECIALIZATION, "Embers of Nihilam", 1 ); // Should be ID 1265770 (I)
    talents.embers_of_nihilam_2 = find_talent_spell( talent_tree::SPECIALIZATION, "Embers of Nihilam", 2 ); // Should be ID 1265772 (II)
    talents.embers_of_nihilam_3 = find_talent_spell( talent_tree::SPECIALIZATION, "Embers of Nihilam", 3 ); // Should be ID 1265774 (III)
    talents.echo_of_sargeras = conditional_spell_lookup( talents.embers_of_nihilam_1.ok(), 1265884 );
    talents.vision_of_nihilam = conditional_spell_lookup( talents.embers_of_nihilam_2.ok(), 1265939 );

    cooldowns.echo_of_sargeras->duration = talents.embers_of_nihilam_3->internal_cooldown();

    // Additional Tier Set spell data
    tier.wl_destruction_12_0_class_set_2pc = sets->set( WARLOCK_DESTRUCTION, MID1, B2 ); // Should be ID 1264873
    tier.wl_destruction_12_0_class_set_4pc = sets->set( WARLOCK_DESTRUCTION, MID1, B4 ); // Should be ID 1264874

    // Initialize some default values for pet spawners
    warlock_pet_list.infernals.set_default_duration( talents.summon_infernal_main->duration() );
    warlock_pet_list.rocs.set_default_duration( talents.summon_infernal_roc->duration() );
    warlock_pet_list.shadowy_rifts.set_default_duration( talents.shadowy_tear_summon->duration() );
    warlock_pet_list.unstable_rifts.set_default_duration( talents.unstable_tear_summon->duration() );
    warlock_pet_list.chaos_rifts.set_default_duration( talents.chaos_tear_summon->duration() );
    warlock_pet_list.overfiends.set_default_duration( talents.summon_overfiend->duration() );
  }

  void warlock_t::init_spells_diabolist()
  {
    hero.diabolic_ritual = find_talent_spell( talent_tree::HERO, "Diabolic Ritual" ); // Should be ID 428514
    hero.ritual_overlord = conditional_spell_lookup( hero.diabolic_ritual.ok(), 431944 );
    hero.ritual_mother = conditional_spell_lookup( hero.diabolic_ritual.ok(), 432815 );
    hero.ritual_pit_lord = conditional_spell_lookup( hero.diabolic_ritual.ok(), 432816 );
    hero.art_overlord = conditional_spell_lookup( hero.diabolic_ritual.ok(), 428524 );
    hero.art_mother = conditional_spell_lookup( hero.diabolic_ritual.ok(), 432794 );
    hero.art_pit_lord = conditional_spell_lookup( hero.diabolic_ritual.ok(), 432795 );
    hero.summon_overlord = conditional_spell_lookup( hero.diabolic_ritual.ok(), 428571 );
    hero.summon_mother = conditional_spell_lookup( hero.diabolic_ritual.ok(), 428565 );
    hero.summon_pit_lord = conditional_spell_lookup( hero.diabolic_ritual.ok(), 434400 );
    hero.wicked_cleave = conditional_spell_lookup( hero.diabolic_ritual.ok(), 432120 );
    hero.chaos_salvo = conditional_spell_lookup( hero.diabolic_ritual.ok(), 432569 );
    hero.chaos_salvo_missile = conditional_spell_lookup( hero.diabolic_ritual.ok(), 432592 );
    hero.chaos_salvo_dmg = conditional_spell_lookup( hero.diabolic_ritual.ok(), 432596 );
    hero.felseeker = conditional_spell_lookup( hero.diabolic_ritual.ok(), 438973 );
    hero.felseeker_dmg = conditional_spell_lookup( hero.diabolic_ritual.ok(), 434404 );

    hero.cloven_souls = find_talent_spell( talent_tree::HERO, "Cloven Souls" ); // Should be ID 428517
    hero.cloven_soul_debuff = conditional_spell_lookup( hero.cloven_souls.ok(), 434424 );

    hero.touch_of_rancora = find_talent_spell( talent_tree::HERO, "Touch of Rancora" ); // Should be ID 429893

    hero.secrets_of_the_coven = find_talent_spell( talent_tree::HERO, "Secrets of the Coven" ); // Should be ID 428518
    hero.infernal_bolt = conditional_spell_lookup( hero.secrets_of_the_coven.ok(), 434506 );
    hero.infernal_bolt_buff = conditional_spell_lookup( hero.secrets_of_the_coven.ok(), 433891 );

    hero.cruelty_of_kerxan = find_talent_spell( talent_tree::HERO, "Cruelty of Kerxan" ); // Should be ID 429902

    hero.infernal_machine = find_talent_spell( talent_tree::HERO, "Infernal Machine" ); // Should be ID 429917

    hero.flames_of_xoroth = find_talent_spell( talent_tree::HERO, "Flames of Xoroth" ); // Should be ID 429657

    hero.abyssal_dominion = find_talent_spell( talent_tree::HERO, "Abyssal Dominion" ); // Should be ID 429581
    hero.abyssal_dominion_buff = conditional_spell_lookup( hero.abyssal_dominion.ok() && warlock_base.demonology_warlock->ok(), 456323 );
    hero.infernal_fragmentation = conditional_spell_lookup( hero.abyssal_dominion.ok() && warlock_base.destruction_warlock->ok(), 456310 );

    hero.gloom_of_nathreza = find_talent_spell( talent_tree::HERO, "Gloom of Nathreza" ); // Should be ID 429899

    hero.ruination = find_talent_spell( talent_tree::HERO, "Ruination" ); // Should be ID 428522
    hero.ruination_buff = conditional_spell_lookup( hero.ruination.ok(), 433885 );
    hero.ruination_cast = conditional_spell_lookup( hero.ruination.ok(), 434635 );
    hero.ruination_impact = conditional_spell_lookup( hero.ruination.ok(), 434636 );
    hero.diabolic_imp = conditional_spell_lookup( hero.ruination.ok() && warlock_base.destruction_warlock->ok(), 438822 );
    hero.diabolic_bolt = conditional_spell_lookup( hero.ruination.ok() && warlock_base.destruction_warlock->ok(), 438823 );

    hero.diabolic_oculi = find_talent_spell( talent_tree::HERO, "Diabolic Oculi" ); // Should be ID 1268709
    hero.demonic_oculi_buff = conditional_spell_lookup( hero.diabolic_oculi.ok(), 1269643 );
    hero.eye_explosion = conditional_spell_lookup( hero.diabolic_oculi.ok(), 1269800 );

    hero.looks_that_kill = find_talent_spell( talent_tree::HERO, "Looks That Kill" ); // Should be ID 1268713
    hero.diabolic_gaze_dmg_1 = conditional_spell_lookup( hero.looks_that_kill.ok(), 1269892 );
    hero.diabolic_gaze_dmg_2 = conditional_spell_lookup( hero.looks_that_kill.ok(), 1269886 );
    hero.diabolic_gaze_dmg_3 = conditional_spell_lookup( hero.looks_that_kill.ok(), 1269885 );

    hero.minds_eyes = find_talent_spell( talent_tree::HERO, "Mind's Eyes" ); // Should be ID 1268716
    hero.minds_eyes_buff = conditional_spell_lookup( hero.minds_eyes.ok(), 1269879 );

    // Initialize some default values for pet spawners
    warlock_pet_list.overlords.set_default_duration( hero.summon_overlord->duration() );
    warlock_pet_list.mothers.set_default_duration( hero.summon_mother->duration() );
    warlock_pet_list.pit_lords.set_default_duration( hero.summon_pit_lord->duration() );
    warlock_pet_list.fragments.set_default_duration( hero.infernal_fragmentation->duration() );
  }

  void warlock_t::init_spells_hellcaller()
  {
    hero.wither = find_talent_spell( talent_tree::HERO, "Wither" ); // Should be ID 445465
    hero.wither_direct = conditional_spell_lookup( hero.wither.ok(), 445468 );
    hero.wither_dot = conditional_spell_lookup( hero.wither.ok(), 445474 );

    hero.xalans_ferocity = find_talent_spell( talent_tree::HERO, "Xalan's Ferocity" ); // Should be ID 440044

    hero.blackened_soul = find_talent_spell( talent_tree::HERO, "Blackened Soul" ); // Should be ID 440043
    hero.blackened_soul_trigger = conditional_spell_lookup( hero.wither.ok(), 445731 );
    hero.blackened_soul_dmg = conditional_spell_lookup( hero.wither.ok(), 445736 );

    hero.xalans_cruelty = find_talent_spell( talent_tree::HERO, "Xalan's Cruelty" ); // Should be ID 440040

    hero.hatefury_rituals = find_talent_spell( talent_tree::HERO, "Hatefury Rituals" ); // Should be ID 440048

    hero.bleakheart_tactics = find_talent_spell( talent_tree::HERO, "Bleakheart Tactics" ); // Should be ID 440051

    hero.illhoofs_design = find_talent_spell( talent_tree::HERO, "Illhoof's Design" ); // Should be ID 440070

    hero.mark_of_xavius = find_talent_spell( talent_tree::HERO, "Mark of Xavius" ); // Should be ID 440046

    hero.seeds_of_their_demise = find_talent_spell( talent_tree::HERO, "Seeds of Their Demise" ); // Should be ID 440055

    hero.mark_of_perotharn = find_talent_spell( talent_tree::HERO, "Mark of Peroth'arn" ); // Should be ID 440045

    hero.through_the_felvine = find_talent_spell( talent_tree::HERO, "Through the Felvine" ); // Should be ID 1266799

    hero.devil_fruit = find_talent_spell( talent_tree::HERO, "Devil Fruit" ); // Should be ID 1266805

    hero.alzzins_iniquity = find_talent_spell( talent_tree::HERO, "Alzzin's Iniquity" ); // Should be ID 1266803

    hero.malevolence = find_talent_spell( talent_tree::HERO, "Malevolence" ); // Should be ID 430014
    hero.malevolence_buff = conditional_spell_lookup( hero.malevolence.ok(), 442726 );
    hero.malevolence_dmg = conditional_spell_lookup( hero.malevolence.ok(), 446285 );

    cooldowns.blackened_soul->duration = hero.blackened_soul->internal_cooldown();
  }

  void warlock_t::init_spells_soul_harvester()
  {
    hero.demonic_soul = find_talent_spell( talent_tree::HERO, "Demonic Soul" ); // Should be ID 449614
    hero.succulent_soul = conditional_spell_lookup( hero.demonic_soul.ok(), 449793 );
    hero.demonic_soul_dmg = conditional_spell_lookup( hero.demonic_soul.ok(), 449801 );

    hero.necrolyte_teachings = find_talent_spell( talent_tree::HERO, "Necrolyte Teachings" ); // Should be ID 449620

    hero.soul_anathema = find_talent_spell( talent_tree::HERO, "Soul Anathema" ); // Should be ID 449624
    hero.soul_anathema_dot = conditional_spell_lookup( hero.soul_anathema.ok(), 450538 );

    hero.demoniacs_fervor = find_talent_spell( talent_tree::HERO, "Demoniac's Fervor" ); // Should be ID 449629

    hero.shared_fate = find_talent_spell( talent_tree::HERO, "Shared Fate" ); // Should be ID 449704
    hero.shared_fate_dot = conditional_spell_lookup( hero.shared_fate.ok(), 450591 );

    hero.feast_of_souls = find_talent_spell( talent_tree::HERO, "Feast of Souls" ); // Should be ID 449706
    hero.marked_soul = conditional_spell_lookup( hero.shared_fate.ok() || hero.feast_of_souls.ok(), 450629 );

    hero.wicked_reaping = find_talent_spell( talent_tree::HERO, "Wicked Reaping" ); // Should be ID 449631
    hero.wicked_reaping_dmg = conditional_spell_lookup( hero.wicked_reaping.ok(), 449826 );

    hero.quietus = find_talent_spell( talent_tree::HERO, "Quietus" ); // Should be ID 449634

    hero.sataiels_volition = find_talent_spell( talent_tree::HERO, "Sataiel's Volition" ); // Should be ID 449637

    hero.shadow_of_death = find_talent_spell( talent_tree::HERO, "Shadow of Death" ); // Should be ID 449638
    hero.shadow_of_death_energize = conditional_spell_lookup( hero.shadow_of_death.ok(), 449858 );

    hero.manifested_avarice = find_talent_spell( talent_tree::HERO, "Manifested Avarice" ); // Should be ID 1268884
    hero.manifested_avarice_spell = conditional_spell_lookup( hero.manifested_avarice.ok(), 1269042 );

    hero.shared_vessel = find_talent_spell( talent_tree::HERO, "Shared Vessel" ); // Should be ID 1268889

    hero.eternal_hunger = find_talent_spell( talent_tree::HERO, "Eternal Hunger" ); // Should be ID 1268903

    // Initialize some default values for pet spawners
    warlock_pet_list.demonic_souls.set_default_duration( hero.manifested_avarice_spell->duration() );
  }

  void warlock_t::init_proc_data_entries()
  {
    proc_data_entries.shadow_bolt_energize = warlock_base.shadow_bolt_energize;
    proc_data_entries.agony_energize = talents.agony_energize;
    proc_data_entries.demonbolt_energize = talents.demonbolt_energize;
    proc_data_entries.incinerate_energize = warlock_base.incinerate_energize;
    proc_data_entries.marked_soul = hero.marked_soul;
  }

  void warlock_t::init_base_stats()
  {
    if ( base.distance < 1.0 )
      base.distance = 30.0;

    base.attack_power_per_strength = 0.0;
    base.attack_power_per_agility  = 0.0;
    base.spell_power_per_intellect = 1.0;

    player_t::init_base_stats();

    if ( default_pet.empty() )
    {
      if ( affliction() )
        default_pet = "imp";
      else if ( demonology() )
        if ( talents.summon_felguard.ok() )
          default_pet = "felguard";
        else
          default_pet = "imp";
      else if ( destruction() )
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

    buffs.pet_movement = make_buff( this, "pet_movement" )
                             ->set_max_stack( 100 )
                             ->set_proc_callbacks( false );

    // Affliction buffs
    create_buffs_affliction();

    // Demonology buffs
    create_buffs_demonology();

    // Destruction buffs
    create_buffs_destruction();

    create_buffs_diabolist();
    create_buffs_hellcaller();
    create_buffs_soul_harvester();
  }

  void warlock_t::create_buffs_affliction()
  {
    buffs.nightfall = make_buff( this, "nightfall", talents.nocturnal_yield.ok() ? talents.nightfall_buff_2 : talents.nightfall_buff );

    buffs.darkglare_presence = make_buff( this, "darkglare_presence", talents.darkglare_presence_buff );

    buffs.shard_instability = make_buff( this, "shard_instability", talents.shard_instability_buff );

    buffs.cascading_calamity = make_buff( this, "cascading_calamity", talents.cascading_calamity_buff )
                                   ->set_default_value_from_effect( 1 )
                                   ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );

    buffs.seed_of_corruption_is_out_dnt = make_buff( this, "seed_of_corruption_is_out_dnt", talents.seed_of_corruption_is_out_dnt )
                                              ->set_quiet( true );
  }

  void warlock_t::create_buffs_demonology()
  {
    buffs.demonic_core = make_buff( this, "demonic_core", talents.demonic_core_buff );

    buffs.power_siphon = make_buff( this, "power_siphon", talents.power_siphon_buff );

    buffs.inner_demons = make_buff( this, "inner_demons", talents.inner_demons )
                             ->set_period( talents.inner_demons->effectN( 1 ).period() )
                             ->set_tick_time_behavior( buff_tick_time_behavior::UNHASTED )
                             ->set_tick_zero( true )
                             ->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
                               summons.wild_imp_2->execute();
                             } );

    buffs.tyrants_oblation = make_buff( this, "tyrants_oblation", talents.tyrants_oblation_buff )
                                 ->set_default_value_from_effect( 1 )
                                 ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );

    buffs.hellbent_commander = make_buff( this, "hellbent_commander", talents.hellbent_commander_buff )
                                   ->set_default_value_from_effect( 1 );

    buffs.dominion_of_argus = make_buff( this, "dominion_of_argus", talents.dominion_of_argus_1_buff )
                                  ->set_refresh_behavior( buff_refresh_behavior::DISABLED )
                                  ->set_stack_change_callback( [ this ]( buff_t* b, int, int cur ) {
                                    if ( cur == b->max_stack() )
                                    {
                                      make_event( *sim, 0_ms, [ b ] { b->decrement( b->max_stack() - 1 ); } );
                                      summon_dominion_of_argus_pet( dominion_of_argus_pet_e::DOA_PET_RANDOM );
                                    }
                                  } );

    // Pet tracking buffs
    buffs.wild_imps = make_buff( this, "wild_imps" )->set_max_stack( 40 )
                          ->set_proc_callbacks( false );

    buffs.dreadstalkers = make_buff( this, "dreadstalkers" )->set_max_stack( 8 )
                              ->set_duration( talents.call_dreadstalkers_summon_2->duration() )
                              ->set_proc_callbacks( false );

    buffs.vilefiend = make_buff( this, "vilefiend" )->set_max_stack( 2 )
                          ->set_duration( talents.vilefiend->duration() )
                          ->set_proc_callbacks( false );

    buffs.grimoire_imp_lord = make_buff( this, "grimoire_imp_lord" )->set_max_stack( 1 )
                                  ->set_duration( talents.grimoire_imp_lord->duration() )
                                  ->set_proc_callbacks( false );

    buffs.grimoire_fel_ravager = make_buff( this, "grimoire_fel_ravager" )->set_max_stack( 1 )
                                     ->set_duration( talents.grimoire_fel_ravager->duration() )
                                     ->set_proc_callbacks( false );

    buffs.doomguard = make_buff( this, "doomguard" )->set_max_stack( 4 )
                          ->set_duration( talents.summon_doomguard->duration() )
                          ->set_proc_callbacks( false );

    buffs.tyrant = make_buff( this, "tyrant" )->set_max_stack( 1 )
                       ->set_duration( talents.summon_demonic_tyrant->duration() )
                       ->set_proc_callbacks( false );
  }

  void warlock_t::create_buffs_destruction()
  {
    buffs.backdraft = make_buff( this, "backdraft", talents.backdraft_buff );

    buffs.reverse_entropy = make_buff( this, "reverse_entropy", talents.reverse_entropy_buff )
                                ->set_default_value_from_effect( 1 )
                                ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
                                ->set_trigger_spell( talents.reverse_entropy )
                                ->set_rppm( RPPM_NONE, talents.reverse_entropy->real_ppm() );

    buffs.fiendish_cruelty = make_buff( this, "fiendish_cruelty", talents.fiendish_cruelty_buff )
                                 ->set_default_value_from_effect( 1 );

    buffs.chaotic_inferno = make_buff( this, "chaotic_inferno_buff", talents.chaotic_inferno_buff )
                                ->set_default_value_from_effect( 1 );

    buffs.rain_of_chaos = make_buff( this, "rain_of_chaos", talents.rain_of_chaos_buff );

    buffs.conflagration_of_chaos = make_buff( this, "conflagration_of_chaos", talents.conflagration_of_chaos_buff )
                                       ->set_chance( talents.conflagration_of_chaos->effectN( 1 ).percent() );

    buffs.flashpoint = make_buff( this, "flashpoint", talents.flashpoint_buff )
                           ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
                           ->set_default_value_from_effect( 1 );

    buffs.crashing_chaos = make_buff( this, "crashing_chaos", talents.crashing_chaos_buff )
                                 ->set_max_stack( std::max( as<int>( talents.crashing_chaos->effectN( 3 ).base_value() ), 1 ) )
                                 ->set_reverse( true );

    buffs.alythesss_ire = make_buff( this, "alythesss_ire", talents.alythesss_ire_buff )
                              ->set_default_value_from_effect( 1 );

    buffs.vision_of_nihilam = make_buff( this, "vision_of_nihilam", talents.vision_of_nihilam )
                                  ->set_default_value( talents.embers_of_nihilam_2->effectN( 1 ).percent() )
                                  ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
                                  ->set_pct_buff_type( STAT_PCT_BUFF_CRIT );

    buffs.summon_overfiend = make_buff( this, "summon_overfiend", talents.overfiend_buff )
                                 ->set_tick_time_behavior( buff_tick_time_behavior::UNHASTED )
                                 ->set_period( talents.overfiend_buff->effectN( 1 ).period() )
                                 ->set_tick_callback( [ this ]( buff_t*, int, timespan_t )
                                   { resource_gain( RESOURCE_SOUL_SHARD, talents.overfiend_buff->effectN( 1 ).base_value() / 10.0, gains.summon_overfiend ); } );
  }

  struct diabolic_ritual_buff_t : public buff_t
  {
    const int diabolic_ritual_next_index; // Index of the next Diabolic Ritual buff to cycle
    buff_t *art_buff_trigger; // Demonic Art buff to trigger when the effect of this Diabolic Ritual buff is consumed

    diabolic_ritual_buff_t( warlock_t* p, util::string_view name, const spell_data_t* spell_data, const int _diabolic_ritual_next_index = 0, buff_t* _art_buff_trigger = nullptr )
      : buff_t( p, name, spell_data ),
        diabolic_ritual_next_index( _diabolic_ritual_next_index ),
        art_buff_trigger( _art_buff_trigger )
    {
      set_can_cancel( false );
      set_stack_change_callback( [ this, p ]( buff_t*, int, int cur )
      {
        if ( cur == 0 )
        {
          // The trigger of the Demonic Art buff has a certain delay that can be modeled fairly closely using a normal distribution
          const timespan_t buff_delay = timespan_t::from_millis( rng().gauss( 200, 15 ) );
          make_event( sim, buff_delay, [ this, p ] {
            if ( p->buffs.art_mother->check() || p->buffs.art_pit_lord->check() || p->buffs.art_overlord->check() )
            {
              // Expire other Demonic Art buffs without triggering their effect
              p->demonic_art_buff_replaced = true;
              p->buffs.art_mother->expire();
              p->buffs.art_pit_lord->expire();
              p->buffs.art_overlord->expire();
              p->demonic_art_buff_replaced = false;
            }
            if ( art_buff_trigger )
              art_buff_trigger->trigger();
          } );
          p->diabolic_ritual = diabolic_ritual_next_index;
        }
      } );
    }
  };

  void warlock_t::create_buffs_diabolist()
  {

    buffs.art_overlord = make_buff( this, "demonic_art_overlord", hero.art_overlord )
                             ->set_can_cancel( false )
                             ->set_stack_change_callback( [ this ]( buff_t*, int, int cur )
                               {
                                 if ( cur == 0 && in_combat && !demonic_art_buff_replaced )
                                 {
                                   summons.overlord->execute();
                                 }
                               } );

    buffs.art_mother = make_buff( this, "demonic_art_mother_of_chaos", hero.art_mother )
                           ->set_can_cancel( false )
                           ->set_stack_change_callback( [ this ]( buff_t*, int, int cur )
                             {
                               if ( cur == 0 && in_combat && !demonic_art_buff_replaced )
                               {
                                 summons.mother->execute();
                               }
                             } );

    buffs.art_pit_lord = make_buff( this, "demonic_art_pit_lord", hero.art_pit_lord )
                             ->set_can_cancel( false )
                             ->set_stack_change_callback( [ this ]( buff_t*, int, int cur )
                               {
                                 if ( cur == 0 && in_combat && !demonic_art_buff_replaced )
                                 {
                                   summons.pit_lord->execute();
                                 }
                               } );

    buffs.ritual_overlord = make_buff<diabolic_ritual_buff_t>( this, "diabolic_ritual_overlord", hero.ritual_overlord, 1, buffs.art_overlord );

    buffs.ritual_mother = make_buff<diabolic_ritual_buff_t>( this, "diabolic_ritual_mother_of_chaos", hero.ritual_mother, 2, buffs.art_mother );

    buffs.ritual_pit_lord = make_buff<diabolic_ritual_buff_t>( this, "diabolic_ritual_pit_lord", hero.ritual_pit_lord, 0, buffs.art_pit_lord );

    buffs.infernal_bolt = make_buff( this, "infernal_bolt", hero.infernal_bolt_buff );

    buffs.abyssal_dominion = make_buff( this, "abyssal_dominion", hero.abyssal_dominion_buff )
                                 ->set_duration( hero.abyssal_dominion_buff->duration() + talents.reign_of_tyranny->effectN( 1 ).time_value() );

    buffs.ruination = make_buff( this, "ruination", hero.ruination_buff );

    buffs.demonic_oculi = make_buff( this, "demonic_oculi", hero.demonic_oculi_buff )
                              ->set_period( hero.demonic_oculi_buff->effectN( 2 ).period() )
                              ->set_freeze_stacks( true )
                              ->set_tick_time_behavior( buff_tick_time_behavior::UNHASTED )
                              ->set_tick_callback( [ this ]( buff_t* b, int, timespan_t ) {
                                if ( hero.looks_that_kill.ok() )
                                {
                                  switch ( b->check() )
                                  {
                                    case 1:
                                      proc_actions.diabolic_gaze_1->execute_on_target( target );
                                      break;
                                    case 2:
                                      proc_actions.diabolic_gaze_2->execute_on_target( target );
                                      break;
                                    case 3:
                                      proc_actions.diabolic_gaze_3->execute_on_target( target );
                                  }
                                }
                              } );

    buffs.minds_eyes = make_buff( this, "minds_eyes", hero.minds_eyes_buff )
                                     ->set_pct_buff_type( STAT_PCT_BUFF_INTELLECT )
                                     ->set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE );
  }

  void warlock_t::create_buffs_hellcaller()
  {
    buffs.malevolence = make_buff( this, "malevolence", hero.malevolence_buff )
                            ->set_cooldown( 0_ms )
                            ->set_refresh_behavior( buff_refresh_behavior::EXTEND )
                            ->set_max_stack( 1 )
                            ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
                            ->set_default_value_from_effect( 1 );
  }

  void warlock_t::create_buffs_soul_harvester()
  {
    buffs.succulent_soul = make_buff( this, "succulent_soul", hero.succulent_soul );

    buffs.manifested_demonic_soul = make_buff( this, "manifested_demonic_soul", hero.manifested_avarice_spell )
                                        ->add_invalidate( CACHE_MASTERY );
  }

  void warlock_t::create_pets()
  {
    for ( const auto& pet : pet_name_list )
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

    if ( affliction() )
      init_gains_affliction();
    if ( demonology() )
      init_gains_demonology();
    if ( destruction() )
      init_gains_destruction();

    init_gains_diabolist();
    init_gains_hellcaller();
    init_gains_soul_harvester();
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
    gains.dominion_of_argus = get_gain( "dominion_of_argus" );
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

    if ( affliction() )
      init_procs_affliction();
    if ( demonology() )
      init_procs_demonology();
    if ( destruction() )
      init_procs_destruction();

    init_procs_diabolist();
    init_procs_hellcaller();
    init_procs_soul_harvester();
  }

  void warlock_t::init_procs_affliction()
  {
    procs.nightfall = get_proc( "nightfall" );
    procs.shadowbolt_volley = get_proc( "shadowbolt_volley" );
    procs.ravenous_afflictions = get_proc( "ravenous_afflictions" );
    procs.shard_instability = get_proc( "shard_instability" );
    procs.fatal_echoes = get_proc( "fatal_echoes" );
    procs.wrath_of_nathreza = get_proc( "wrath_of_nathreza" );
  }

  void warlock_t::init_procs_demonology()
  {
    procs.demonic_core_dogs = get_proc( "demonic_core_dogs" );
    procs.demonic_core_imps_fade = get_proc( "demonic_core_imps_fade" );
    procs.demonic_core_imps_implosion = get_proc( "demonic_core_imps_implosion" );
    procs.carnivorous_stalkers = get_proc( "carnivorous_stalkers" );
    procs.infernal_rapidity = get_proc( "infernal_rapidity" );
    procs.spiteful_reconstitution = get_proc( "spiteful_reconstitution" );
    procs.demonic_knowledge = get_proc( "demonic_knowledge" );
  }

  void warlock_t::init_procs_destruction()
  {
    procs.mayhem = get_proc( "mayhem" );
    procs.fiendish_cruelty = get_proc( "fiendish_cruelty" );
    procs.chaotic_inferno = get_proc( "chaotic_inferno" );
    procs.dimensional_rift = get_proc( "dimensional_rift" );
    procs.avatar_of_destruction = get_proc( "avatar_of_destruction" );
    procs.conflagration_of_chaos = get_proc( "conflagration_of_chaos" );
    procs.alythesss_ire = get_proc( "alythesss_ire" );
    procs.reverse_entropy = get_proc( "reverse_entropy" );
    procs.rain_of_chaos = get_proc( "rain_of_chaos" );
    procs.demonfire_infusion_inc = get_proc( "demonfire_infusion_incinerate" );
    procs.demonfire_infusion_dot = get_proc( "demonfire_infusion_dot" );
    procs.echo_of_sargeras = get_proc( "echo_of_sargeras" );
    procs.echo_of_sargeras_cb = get_proc( "echo_of_sargeras_cb" );
    procs.echo_of_sargeras_sb = get_proc( "echo_of_sargeras_sb" );
    procs.echo_of_sargeras_rof = get_proc( "echo_of_sargeras_rof" );
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
    procs.devil_fruit = get_proc( "devil_fruit" );
  }

  void warlock_t::init_procs_soul_harvester()
  {
    procs.succulent_soul = get_proc( "succulent_soul" );
    procs.feast_of_souls = get_proc( "feast_of_souls" );
    procs.manifested_avarice = get_proc( "manifested_avarice" );
  }

  void warlock_t::init_rng()
  {
    if ( affliction() )
      init_rng_affliction();
    if ( demonology() )
      init_rng_demonology();
    if ( destruction() )
      init_rng_destruction();

    init_rng_diabolist();
    init_rng_hellcaller();
    init_rng_soul_harvester();

    player_t::init_rng();
  }

  void warlock_t::init_rng_affliction()
  {
    // Agony energize proc
    {
      // 2018-08-24:
      //   Blizzard has not publicly released the formula for Agony's chance to generate a Soul Shard. This set of code is based on results from
      //   500+ Soul Shard sample sizes, and matches in-game results to within 0.1% of accuracy in all tests conducted on all targets numbers up to 8.
      // 2026-03-06:
      //   New tests were conducted using over 55000+ Soul Shards in both single and multi-target scenarios. The results confirm that Blizzard is still
      //   using the same formula for Agony, though they occasionally make unusual adjustments to talent normalization (so this should be checked regularly).
      //   With the larger sample size, we obtained a more precise initial value for Agony's RNG 'increment_max' of 0.370 (previously 0.368).
      // TOCHECK regularly. If any changes are made to this section of code, please also update the Time_to_Shard expression in sc_warlock.cpp.
      double inc_max = rng_settings.agony_energize.setting_value;

      // NOTE: 2026-03-06 Recent tests noted that Creeping Death is renormalizing shard generation to be neutral with/without the talent
      // However, Creeping Death rank 2 is normalizing using -0.1 value instead of -0.2 (the value of rank 1 or half of rank 2) (bug?)
      if ( talents.creeping_death.ok() )
      {
        if ( !bugs || talents.creeping_death.rank() < 2 )
          inc_max *= 1.0 + talents.creeping_death->effectN( 1 ).percent();
        else
          inc_max *= 1.0 + ( talents.creeping_death->effectN( 1 ).percent() * 0.5 );
      }

      progress_rng.agony_energize = get_threshold_rng( "agony_energize", inc_max,
        [ this ]( double increment_max, action_state_t* s ) {
          assert( s );
          auto tdata = get_target_data( s->target );
          assert( tdata );
          dot_t* agony_dot = tdata->dots.agony;
          assert( agony_dot && agony_dot->is_ticking() );
          unsigned active_agonies = get_active_dots( agony_dot );
          assert( active_agonies > 0 );
          increment_max *= std::pow( active_agonies, -2.0 / 3.0 );
          return rng().range( 0.0, increment_max );
        }, true, true );
    }

    // Nightfall proc
    if ( talents.nightfall.ok() )
    {
      // Blizzard did not publicly release how nightfall was changed.
      // We determined this is the probable functionality copied from Agony by first confirming the
      // DR formula was the same and then confirming that you can get procs on 1st tick.
      // The procs also have a regularity that suggest it does not use a proc chance or rppm.
      // Last checked 2026-03-06.
      double inc_max = rng_settings.nightfall.setting_value;

      // NOTE: 2026-03-06 Creeping Death no longer affects the chance of gaining Nightfall
      // However, Creeping Death rank 2 is normalizing using -0.1 value instead of -0.2 (the value of rank 1 or half of rank 2) (bug?)
      if ( talents.creeping_death.ok() )
      {
        if ( !bugs || talents.creeping_death.rank() < 2 )
          inc_max *= 1.0 + talents.creeping_death->effectN( 1 ).percent();
        else
          inc_max *= 1.0 + ( talents.creeping_death->effectN( 1 ).percent() * 0.5 );
      }

      // Sataiel's Volition no longer affects the chance of gaining Nightfall
      if ( hero.sataiels_volition.ok() )
        inc_max *= 1.0 + hero.sataiels_volition->effectN( 1 ).percent();

      progress_rng.nightfall = get_threshold_rng( "nightfall", inc_max,
        [ this ]( double increment_max, action_state_t* s ) {
          assert( s );
          auto tdata = get_target_data( s->target );
          assert( tdata );
          dot_t* corruption_dot = hero.wither.ok() ? tdata->dots.wither : tdata->dots.corruption;
          assert( corruption_dot && corruption_dot->is_ticking() );
          unsigned active_corruptions = get_active_dots( corruption_dot );
          assert( active_corruptions > 0 );
          increment_max *= std::pow( active_corruptions, -2.0 / 3.0 );
          return rng().range( 0.0, increment_max );
        }, true, true );
    }

    rppm_rng.ravenous_afflictions = get_rppm( "ravenous_afflictions", talents.ravenous_afflictions );
    rppm_rng.wrath_of_nathreza = get_rppm( "wrath_of_nathreza", talents.shadow_of_nathreza_3 );

    // Modeling Cunning Cruelty as a pseudo-random distribution (PRD) with a nominal rate of 50% (SB) / 25% (DS) (MG uses DS rate if
    // selected, SB rate otherwise) which corresponds to PRD constant C = 0.302103025348741965 (SB) / C = 0.084744091852316990 (DS)
    if ( talents.cunning_cruelty.ok() )
    {
      double c_cc = prd::find_constant( talents.drain_soul.ok() ? rng_settings.cunning_cruelty_ds.setting_value : rng_settings.cunning_cruelty_sb.setting_value );
      prd_rng.cunning_cruelty = get_accumulated_rng( "cunning_cruelty", c_cc );
    }

    // Modeling Shard Instability as a pseudo-random distribution (PRD) with a nominal rate of 10% (DS/MG) / 20% (SB),
    // which corresponds to PRD constant C = 0.014745844781072676 (DS/MG) / C = 0.055704042949781852 (SB)
    if ( talents.shard_instability.ok() )
    {
      double c_ds = prd::find_constant( talents.shard_instability->effectN( 1 ).percent() );
      prd_rng.shard_instability_ds = get_accumulated_rng( "shard_instability_ds", c_ds );
      double c_sb = prd::find_constant( talents.shard_instability->effectN( 2 ).percent() );
      prd_rng.shard_instability_sb = get_accumulated_rng( "shard_instability_sb", c_sb );
    }

    // Modeling Fatal Echoes as a pseudo-random distribution (PRD) with a nominal
    // rate of 10%, which corresponds to PRD constant C = 0.014745844781072676.
    if ( talents.fatal_echoes.ok() )
    {
      double c_fe = prd::find_constant( talents.fatal_echoes->effectN( 1 ).percent() );
      prd_rng.fatal_echoes = get_accumulated_rng( "fatal_echoes", c_fe );
    }
  }

  void warlock_t::init_rng_demonology()
  {
    if ( talents.demoniac.ok() )
    {
      // Modeling Demoniac (Wild Imp fade) as a pseudo-random distribution (PRD) with a nominal rate of 10% and a hard cap of 21 attempts.
      // The corresponding PRD constant, calculated with that cap included, is C = 0.014559015812945588.
      unsigned demoniac_imp_fade_hardcap = static_cast<unsigned>( rng_settings.demoniac_imp_fade_hard_cap.setting_value );
      double c_dwif = prd::find_constant( talents.demonic_core_spell->effectN( 1 ).percent(), demoniac_imp_fade_hardcap );
      prd_rng.demoniac_imp_fade = get_accumulated_rng( "demoniac_imp_fade", c_dwif, demoniac_imp_fade_hardcap );

      // NOTE: 2026-04-05 It has been tested that Demoniac (Wild Imp implosion) follows a Flat % chance model for each wild imp imploded
      double demoniac_imp_implosion_chance = talents.demonic_core_spell->effectN( 1 ).percent() + hero.sataiels_volition->effectN( 3 ).percent();
      flat_rng.demoniac_imp_implosion = get_simple_proc_rng( "demoniac_imp_implosion", demoniac_imp_implosion_chance);
    }

    // NOTE: 2026-03-06 It has been tested that Carnivorous Stalkers follows a Flat % chance model for each individual melee hit
    if ( talents.carnivorous_stalkers.ok() )
      flat_rng.carnivorous_stalkers = get_simple_proc_rng( "carnivorous_stalkers", talents.carnivorous_stalkers->effectN( 1 ).percent() );

    // Modeling Infernal Rapidity as a pseudo-random distribution (PRD) with a nominal rate of 10%, which corresponds to PRD constant
    // C = 0.014745844781072676. Each Wild Imp uses its own independent accumulator PRD, reset to 0 on spawn. Since a single Wild Imp
    // can cast at most 6 Fel Firebolts, the PRD never has time to fully ramp up, resulting in an average proc chance of ~4.80%.
    if ( talents.infernal_rapidity.ok() )
    {
      prd_rng.infernal_rapidity_prd_c_value = prd::find_constant( talents.infernal_rapidity->effectN( 1 ).percent() );
    }

    // Modeling Spiteful Reconstitution as a pseudo-random distribution (PRD) with an uncapped nominal rate of 10%.
    // That nominal rate corresponds to PRD constant C = 0.014745844781072676.
    // A separate hard cap of 21 attempts is then applied on top of the PRD, raising the effective average proc chance to ~10.06%.
    if ( talents.spiteful_reconstitution.ok() )
    {
      double c_sr = prd::find_constant( rng_settings.spiteful_reconstitution.setting_value );
      unsigned spiteful_reconstitution_hardcap = static_cast<unsigned>( rng_settings.spiteful_reconstitution_hard_cap.setting_value );
      prd_rng.spiteful_reconstitution = get_accumulated_rng( "spiteful_reconstitution", c_sr, spiteful_reconstitution_hardcap );
    }

    // Demonic Knowledge uses Deck of Cards RNG at 6 out of 80 (rank 1) and 12 out of 80 (rank 2)
    if ( talents.demonic_knowledge.ok() )
    {
      int deck_size = static_cast<int>( rng_settings.demonic_knowledge_deck_size.setting_value );
      int cards = 0;
      assert( talents.demonic_knowledge.rank() == 2 || talents.demonic_knowledge.rank() == 1 );
      if ( talents.demonic_knowledge.rank() == 2 )
        cards = static_cast<int>( rng_settings.demonic_knowledge_rank2_cards.setting_value );
      else if ( talents.demonic_knowledge.rank() == 1 )
        cards = static_cast<int>( rng_settings.demonic_knowledge_rank1_cards.setting_value );

      deck_rng.demonic_knowledge = get_shuffled_rng( "demonic_knowledge", cards, deck_size );
    }
  }

  void warlock_t::init_rng_destruction()
  {
    flat_rng.immolate_crit_energize = get_simple_proc_rng( "immolate_crit_energize", warlock_base.immolate_old->effectN( 2 ).percent() );

    // Modeling Fiendish Cruelty as a pseudo-random distribution (PRD) with a nominal
    // rate of 10%, which corresponds to PRD constant C = 0.014745844781072676.
    if ( talents.fiendish_cruelty.ok() )
    {
      double c_fc = prd::find_constant( talents.fiendish_cruelty->effectN( 1 ).percent() );
      prd_rng.fiendish_cruelty = get_accumulated_rng( "fiendish_cruelty", c_fc );
    }

    // Modeling Chaotic Inferno as a pseudo-random distribution (PRD) with a nominal
    // rate of 25%, which corresponds to PRD constant C = 0.084744091852316990.
    if ( talents.chaotic_inferno.ok() )
    {
      double c_ci = prd::find_constant( talents.chaotic_inferno->effectN( 2 ).percent() );
      prd_rng.chaotic_inferno = get_accumulated_rng( "chaotic_inferno", c_ci );
    }

    // Rain of Chaos uses Deck of Cards RNG at 3 out of 20
    if ( talents.rain_of_chaos.ok() ) {
      int deck_size = static_cast<int>( rng_settings.rain_of_chaos_deck_size.setting_value );
      int cards = static_cast<int>( rng_settings.rain_of_chaos_cards.setting_value );
      deck_rng.rain_of_chaos = get_shuffled_rng( "rain_of_chaos", cards, deck_size );
    }

    // Modeling Dimensional Rift as a pseudo-random distribution (PRD) with a nominal
    // rate of 10%, which corresponds to PRD constant C = 0.014745844781072676.
    if ( talents.dimensional_rift.ok() )
    {
      double c_dr = prd::find_constant( talents.dimensional_rift->effectN( 1 ).percent() );
      prd_rng.dimensional_rift = get_accumulated_rng( "dimensional_rift", c_dr );

      // The pets summoned by Dimensional Rift is choosen following a Deck of Cards model of 2/2/2 out of 6.
      // With the Avatar of Destruction talent, this is modified to a Deck of Cards model of 2/2/2/1 out of 7.
      if ( talents.avatar_of_destruction.ok() )
      {
        std::vector<dimensional_rift_pet_e> deck = {
            DR_PET_SHADOWY_TEAR,  DR_PET_SHADOWY_TEAR,
            DR_PET_UNSTABLE_TEAR, DR_PET_UNSTABLE_TEAR,
            DR_PET_CHAOS_TEAR,    DR_PET_CHAOS_TEAR,
            DR_PET_OVERFIEND };

        deck_rng.dimensional_rift_summon = std::make_unique<shuffled_bag_rng_t<dimensional_rift_pet_e>>( std::move( deck ), this );
      }
      else
      {
        std::vector<dimensional_rift_pet_e> deck = {
            DR_PET_SHADOWY_TEAR,  DR_PET_SHADOWY_TEAR,
            DR_PET_UNSTABLE_TEAR, DR_PET_UNSTABLE_TEAR,
            DR_PET_CHAOS_TEAR,    DR_PET_CHAOS_TEAR };

        deck_rng.dimensional_rift_summon = std::make_unique<shuffled_bag_rng_t<dimensional_rift_pet_e>>( std::move( deck ), this );
      }
    }

    if ( talents.demonfire_infusion.ok() )
    {
      flat_rng.demonfire_infusion_dot = get_simple_proc_rng( "demonfire_infusion_dot", talents.demonfire_infusion->effectN( 1 ).percent() );
      flat_rng.demonfire_infusion_inc = get_simple_proc_rng( "demonfire_infusion_incinerate", talents.demonfire_infusion->effectN( 2 ).percent() );
    }

    if ( talents.alythesss_ire.ok() )
    {
      flat_rng.alythesss_ire_shift = get_simple_proc_rng( "alythesss_ire_shift", rng_settings.alythesss_ire_shift.setting_value );

      const unsigned chance = as<unsigned>( talents.alythesss_ire->effectN( 1 ).base_value() );
      assert( chance != 0u );
      assert( 100u % chance == 0u );
      const unsigned alythesss_ire_trigger = 100u / chance;

      cycle_proc.alythesss_ire = get_rng<fixed_cycle_proc_t>( "alythesss_ire", alythesss_ire_trigger, true, [ this ]( unsigned trigger_count ) {
        // NOTE: 2026-03-06 Alythess's Ire usually procs at a fixed interval of attempts. Rarely, the cycle
        // shifts and advances the next proc; testing suggests this happens randomly in roughly ~1% of procs.
        return flat_rng.alythesss_ire_shift->trigger() ? rng().range( 1u, trigger_count ) : 0u;
      } );
    }

    // Modeling Echo of Sargeras as a pseudo-random distribution (PRD) with a nominal
    // rate of 10%, which corresponds to PRD constant C = 0.014745844781072676.
    if ( talents.embers_of_nihilam_1.ok() )
    {
      double c_es = prd::find_constant( rng_settings.echo_of_sargeras.setting_value );
      prd_rng.echo_of_sargeras = get_accumulated_rng( "echo_of_sargeras", c_es );
    }
  }

  void warlock_t::init_rng_diabolist()
  {
  }

  void warlock_t::init_rng_hellcaller()
  {
    flat_rng.wither_crit_energize = get_simple_proc_rng( "wither_crit_energize", hero.wither_direct->effectN( 2 ).percent() );
    flat_rng.blackened_soul = get_simple_proc_rng( "blackened_soul", rng_settings.blackened_soul.setting_value );

    // Modeling Bleakheart Tactics as a shared pseudo-random distribution (PRD) with a nominal
    // rate of 15%, which corresponds to PRD constant C = 0.032220914373087675.
    if ( hero.bleakheart_tactics.ok() )
    {
      double c_bt = prd::find_constant( rng_settings.bleakheart_tactics.setting_value );
      prd_rng.bleakheart_tactics = get_accumulated_rng( "bleakheart_tactics", c_bt );
    }

    // Seeds of their Demise proc
    if ( hero.seeds_of_their_demise.ok() )
    {
      double base_inc_max = rng_settings.seeds_of_their_demise.setting_value;

      progress_rng.seeds_of_their_demise = get_threshold_rng( "seeds_of_their_demise", base_inc_max,
        [ this ]( double increment_max, action_state_t* s ) {
          assert( hero.wither.ok() );
          assert( s );
          auto tdata = get_target_data( s->target );
          assert( tdata );
          dot_t* wither_dot = tdata->dots.wither;
          assert( wither_dot && wither_dot->is_ticking() );
          const double stacks_before = wither_dot->current_stack() + 1.0;
          unsigned active_withers = get_active_dots( wither_dot );
          assert( active_withers > 0 );
          const double weight = std::pow( stacks_before, -2.0 / 3.0 ) * std::pow( active_withers, -3.0 / 4.0 );
          return rng().range( increment_max * weight );
        }, true, true );
    }

    // Modeling Mark of Perotharn as a shared pseudo-random distribution (PRD) with a nominal
    // rate of 15%, which corresponds to PRD constant C = 0.032220914373087675.
    if ( hero.mark_of_perotharn.ok() )
    {
      double c_mop = prd::find_constant( rng_settings.mark_of_perotharn.setting_value );
      prd_rng.mark_of_perotharn = get_accumulated_rng( "mark_of_perotharn", c_mop );
    }

    rppm_rng.devil_fruit = get_rppm( "devil_fruit", hero.devil_fruit );
  }

  void warlock_t::init_rng_soul_harvester()
  {
    // Modeling Succulent Soul as a pseudo-random distribution (PRD) with a nominal rate of 22.5% (aff) / 15% (demo),
    // which corresponds to PRD constant C = 0.069555224955587218 (aff) / C = 0.032220914373087675 (demo)
    if ( hero.demonic_soul.ok() )
    {
      assert( affliction() || demonology() );
      double c_ss = 0.0;
      if ( affliction() )
        c_ss = prd::find_constant( rng_settings.succulent_soul_aff.setting_value );
      else if ( demonology() )
        c_ss = prd::find_constant( rng_settings.succulent_soul_demo.setting_value );

      prd_rng.succulent_soul = get_accumulated_rng( "succulent_soul", c_ss );
    }

    // Modeling Manifested Avarice as a pseudo-random distribution (PRD) with a nominal
    // rate of 10%, which corresponds to PRD constant C = 0.014745844781072676.
    if ( hero.manifested_avarice.ok() )
    {
      double c_ma = prd::find_constant( rng_settings.manifested_avarice.setting_value );
      prd_rng.manifested_avarice = get_accumulated_rng( "manifested_avarice", c_ma );
    }

    // Modeling Feast of Souls (Kill) as a pseudo-random distribution (PRD) with an uncapped nominal rate of 12% (aff) / 10% (demo), which
    // corresponds to PRD constants C = 0.020983228162532177 (aff) / C = 0.014745844781072676 (demo). Due to a possible bug, Affliction FoS
    // from Quietus shares the same PRD, but with a lower activation chance.
    // Modeling Feast of Souls (Quietus) as a pseudo-random distribution (PRD) with an uncapped nominal rate of 4% (aff) / 10% (demo).
    // Those nominal rates correspond to PRD constants C = 0.002448555471647706 (aff) / C = 0.014745844781072676 (demo). A separate hard
    // cap of 26 attempts is then applied on top of the PRD, raising the effective average proc chance to ~4.94% (aff) / ~10.01% (demo).
    if ( hero.feast_of_souls.ok() )
    {
      assert( affliction() || demonology() );
      if ( affliction() )
      {
        double c_fs = prd::find_constant( rng_settings.feast_of_souls_aff.setting_value );
        double c_fsq = prd::find_constant( rng_settings.feast_of_souls_aff_quietus.setting_value );
        unsigned feast_of_souls_hardcap = static_cast<unsigned>( rng_settings.feast_of_souls_hard_cap_aff.setting_value );
        prd_rng.feast_of_souls = get_accumulated_rng( "feast_of_souls", c_fs, feast_of_souls_hardcap,
          !bugs ? accumulated_rng_fn{} :
          [ c_fsq, cap = feast_of_souls_hardcap ]( double c_fs, unsigned trigger_count, action_state_t* s ) -> double
          {
            return ( cap > 0 && trigger_count >= cap ) ? 1.0 : ( s ? c_fsq : c_fs ) * trigger_count;
          }
        );
      }
      else if ( demonology() )
      {
        double c_fs = prd::find_constant( rng_settings.feast_of_souls_demo.setting_value );
        unsigned feast_of_souls_hardcap = static_cast<unsigned>( rng_settings.feast_of_souls_hard_cap_demo.setting_value );
        prd_rng.feast_of_souls = get_accumulated_rng( "feast_of_souls", c_fs, feast_of_souls_hardcap );
      }
    }
  }

  void warlock_t::init_resources( bool force )
  {
    player_t::init_resources( force );

    if ( initial_soul_shards > 0 )
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

  std::string warlock_t::aura_expr_from_spell_id( unsigned int spell_id, bool on_self ) const
  {
    if ( spell_id == 342938 && !on_self )
      return "dot.unstable_affliction";

    return player_t::aura_expr_from_spell_id( spell_id, on_self );
  }

  parsed_assisted_combat_rule_t warlock_t::parse_assisted_combat_rule( const assisted_combat_rule_data_t& rule,
                                                                       const assisted_combat_step_data_t& step ) const
  {
    if ( rule.condition_type == AC_AURA_ON_PLAYER && rule.condition_value_1 == 335052 )
      return { "1", "Condition discarded as it checks for PvP talent." };

    if ( rule.condition_type == AC_AURA_MISSING_PLAYER && rule.condition_value_1 == 335052 )
      return { "0", "Condition discarded as it checks for PvP talent." };

    if ( rule.condition_type == AC_AURA_ON_PLAYER && rule.condition_value_1 == 387157 )
      return { "", "Ritual of Ruin buff no longer exists" };

    return player_t::parse_assisted_combat_rule( rule, step );
  }

  std::vector<std::string> warlock_t::action_names_from_spell_id( unsigned int spell_id ) const
  {
    if ( spell_id == 172 )  // Wither from corruption
    {
      if ( destruction() )
        return { "wither" };

      return { "wither", "corruption" };
    }

    if ( spell_id == 348 )  // Wither from immolate
      return { "wither", "immolate" };

    if ( spell_id == 686 )  // Shadowbolt
    {
      if ( destruction() )
        return { "infernal_bolt", "incinerate" };

      return { "infernal_bolt", "shadow_bolt" };
    }

    if ( spell_id == 105174 )  // Hand of guldan
      return { "ruination", "hand_of_guldan" };

    if ( spell_id == 116858 )  // Chaos bolt
      return { "ruination", "chaos_bolt" };

    if ( spell_id == 688 || spell_id == 691 )  // imp & felhunter. Stop infinite summon issue.
      return { };

    return player_t::action_names_from_spell_id( spell_id );
  }

  void warlock_t::init_blizzard_action_list()
  {
    [[maybe_unused]] action_priority_list_t* default_ = get_action_priority_list( "default" );
    player_t::init_blizzard_action_list();

    // precombat overrides
    action_priority_list_t* pre_c = get_action_priority_list( "precombat" );

    pre_c->add_action( "summon_pet" );

    switch ( specialization() )
    {
      case WARLOCK_DEMONOLOGY:
        pre_c->add_action( "power_siphon" );
        pre_c->add_action( "demonbolt,if=!buff.power_siphon.up" );
        pre_c->add_action( "shadow_bolt" );
        break;
      case WARLOCK_DESTRUCTION:
        pre_c->add_action( "grimoire_of_sacrifice,if=talent.grimoire_of_sacrifice.enabled" );
        pre_c->add_action( "soul_fire" );
        pre_c->add_action( "incinerate" );
        break;
      case WARLOCK_AFFLICTION:
        pre_c->add_action( "grimoire_of_sacrifice,if=talent.grimoire_of_sacrifice.enabled" );
        pre_c->add_action( "haunt" );
        pre_c->add_action( "unstable_affliction" );
        break;
      default:
        break;
    }

    // cooldown overrides
    action_priority_list_t* cooldowns = get_action_priority_list( "cooldowns" );
    // reset this from player.cpp
    cooldowns->action_list.clear();

    cooldowns->add_action( "potion" );
    cooldowns->add_action( "blood_fury" );
    cooldowns->add_action( "berserking" );
    cooldowns->add_action( "fireblood" );
    cooldowns->add_action( "ancestral_call" );
    cooldowns->add_action( "use_items" );

    switch ( specialization() )
    {
      case WARLOCK_DEMONOLOGY:
        cooldowns->add_action( "summon_demonic_tyrant,if=buff.dreadstalkers.up" );
        break;
      case WARLOCK_DESTRUCTION:
        cooldowns->add_action( "summon_infernal" );
        break;
      case WARLOCK_AFFLICTION:
        cooldowns->add_action( "summon_darkglare" );
        break;
      default:
        break;
    }
  }

  void warlock_t::add_rng_option( warlock_t::rng_settings_t::rng_setting_t& setting )
  {
    if ( setting.min != std::numeric_limits<double>::lowest() || setting.max != std::numeric_limits<double>::max() )
      add_option( opt_float( "warlock.rng_" + setting.option_name, setting.setting_value, setting.min, setting.max ) );
    else
      add_option( opt_float( "warlock.rng_" + setting.option_name, setting.setting_value ) );

    add_option( opt_deprecated( "rng_" + setting.option_name,  "warlock.rng_" + setting.option_name ) );
  }

  void warlock_t::create_options()
  {
    player_t::create_options();

    add_option( opt_int( "warlock.soul_shards", initial_soul_shards ) );
    add_option( opt_deprecated( "soul_shards", "warlock.soul_shards" ) );
    add_option( opt_string( "warlock.default_pet", default_pet ) );
    add_option( opt_deprecated( "default_pet", "warlock.default_pet" ) );
    add_option( opt_bool( "warlock.disable_felstorm", disable_auto_felstorm ) );
    add_option( opt_deprecated( "disable_felstorm", "warlock.disable_felstorm" ) );
    add_option( opt_bool( "warlock.normalize_destruction_mastery", normalize_destruction_mastery ) );
    add_option( opt_deprecated( "normalize_destruction_mastery", "warlock.normalize_destruction_mastery" ) );
    add_option( opt_bool( "warlock.eye_explosion_instanced_bug_cb", eye_explosion_instanced_bug_cb ) );
    add_option( opt_deprecated( "eye_explosion_instanced_bug_cb", "warlock.eye_explosion_instanced_bug_cb" ) );
    add_option( opt_bool( "warlock.eye_explosion_instanced_bug_sb", eye_explosion_instanced_bug_sb ) );
    add_option( opt_deprecated( "eye_explosion_instanced_bug_sb", "warlock.eye_explosion_instanced_bug_sb" ) );
    add_option( opt_bool( "warlock.eye_explosion_instanced_bug_rof", eye_explosion_instanced_bug_rof ) );
    add_option( opt_deprecated( "eye_explosion_instanced_bug_rof", "warlock.eye_explosion_instanced_bug_rof" ) );
    add_option( opt_float( "warlock.tyrant_antoran_armaments_target_mul", tyrant_antoran_armaments_target_mul, 0.0, 1.0 ));
    add_option( opt_deprecated( "tyrant_antoran_armaments_target_mul", "warlock.tyrant_antoran_armaments_target_mul" ) );

    rng_settings.for_each( [ this ]( auto& setting )
    {
      add_rng_option( setting );
    } );
  }

  void warlock_t::combat_begin()
  {
    player_t::combat_begin();

    if ( demonology() && buffs.inner_demons && talents.inner_demons.ok() )
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

    if ( deck_rng.dimensional_rift_summon )
      deck_rng.dimensional_rift_summon->reset();

    warlock_pet_list.active = nullptr;
    havoc_target = nullptr;
    haunt_target = nullptr;
    patient_zero_target = nullptr;
    wild_imp_spawns.clear();
    diabolic_ritual = rng().range( 0, 3 );
    demonic_art_buff_replaced = false;
  }
}
