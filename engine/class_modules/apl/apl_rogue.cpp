#include "simulationcraft.hpp"
#include "class_modules/apl/apl_rogue.hpp"

namespace rogue_apl {

std::string potion( const player_t* p )
{
  return ( ( p->true_level >= 71 ) ? "tempered_potion_3" :
           ( p->true_level >= 61 ) ? "elemental_potion_of_ultimate_power_3" :
           ( p->true_level >= 51 ) ? "potion_of_spectral_agility" :
           ( p->true_level >= 40 ) ? "potion_of_unbridled_fury" :
           ( p->true_level >= 35 ) ? "draenic_agility" :
           "disabled" );
}

std::string flask( const player_t* p )
{
  if ( p->specialization() == ROGUE_OUTLAW && p->true_level >= 71 )
    return "flask_of_tempered_versatility_3";

  return ( ( p->true_level >= 71 ) ? "flask_of_alchemical_chaos_3" :
           ( p->true_level >= 61 ) ? "iced_phial_of_corrupting_rage_3" :
           ( p->true_level >= 51 ) ? "spectral_flask_of_power" :
           ( p->true_level >= 40 ) ? "greater_flask_of_the_currents" :
           ( p->true_level >= 35 ) ? "greater_draenic_agility_flask" :
           "disabled" );
}

std::string food( const player_t* p )
{
  return ( ( p->true_level >= 71 ) ? "feast_of_the_divine_day" :
           ( p->true_level >= 61 ) ? "fated_fortune_cookie" :
           ( p->true_level >= 51 ) ? "feast_of_gluttonous_hedonism" :
           ( p->true_level >= 45 ) ? "famine_evaluator_and_snack_table" :
           ( p->true_level >= 40 ) ? "lavish_suramar_feast" :
           "disabled" );
}

std::string rune( const player_t* p )
{
  return ( ( p->true_level >= 80 ) ? "crystallized" :
           ( p->true_level >= 70 ) ? "draconic" :
           ( p->true_level >= 60 ) ? "veiled" :
           ( p->true_level >= 50 ) ? "battle_scarred" :
           ( p->true_level >= 45 ) ? "defiled" :
           ( p->true_level >= 40 ) ? "hyper" :
           "disabled" );
}

std::string temporary_enchant( const player_t* p )
{
  if ( p->specialization() == ROGUE_ASSASSINATION && p->true_level >= 71 )
    return "main_hand:algari_mana_oil_3/off_hand:algari_mana_oil_3";
  
  return ( ( p->true_level >= 71 ) ? "main_hand:ironclaw_whetstone_3/off_hand:ironclaw_whetstone_3" :
           ( p->true_level >= 61 ) ? "main_hand:buzzing_rune_3/off_hand:buzzing_rune_3" :
           ( p->true_level >= 51 ) ? "main_hand:shaded_sharpening_stone/off_hand:shaded_sharpening_stone" :
           "disabled" );
}

//assassination_apl_start
void assassination( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* vanish = p->get_action_priority_list( "vanish" );
  action_priority_list_t* core_dot = p->get_action_priority_list( "core_dot" );
  action_priority_list_t* generate = p->get_action_priority_list( "generate" );
  action_priority_list_t* spend = p->get_action_priority_list( "spend" );
  action_priority_list_t* items = p->get_action_priority_list( "items" );
  action_priority_list_t* misc_cds = p->get_action_priority_list( "misc_cds" );

  precombat->add_action( "apply_poison" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=trinket_sync_slot,value=1,if=trinket.1.has_use_buff&(!trinket.2.has_use_buff|trinket.1.cooldown.duration>=trinket.2.cooldown.duration)&!trinket.2.is.treacherous_transmitter|trinket.1.is.treacherous_transmitter|trinket.1.is.house_of_cards", "Check which trinket slots have Stat Values" );
  precombat->add_action( "variable,name=trinket_sync_slot,value=2,if=trinket.2.has_use_buff&(!trinket.1.has_use_buff|trinket.2.cooldown.duration>trinket.1.cooldown.duration)&!trinket.1.is.treacherous_transmitter|trinket.2.is.treacherous_transmitter|trinket.2.is.house_of_cards" );
  precombat->add_action( "stealth", "Pre-cast Slice and Dice if possible" );
  precombat->add_action( "slice_and_dice,precombat_seconds=1" );

  default_->add_action( "stealth", "Restealth if possible (no vulnerable enemies in combat)" );
  default_->add_action( "kick", "Interrupt on cooldown to allow simming interactions with that" );
  default_->add_action( "variable,name=single_target,value=spell_targets.fan_of_knives=1", "Helper Variable to check for single target in combat" );
  default_->add_action( "thistle_tea,if=energy.pct<50&fight_remains<10", "Edge-case check to dump thistle tea at the end of fights" );
  default_->add_action( "call_action_list,name=cds", "Cooldown list takes priority" );
  default_->add_action( "call_action_list,name=core_dot", "Maintain dots when possible" );
  default_->add_action( "call_action_list,name=generate,if=!buff.darkest_night.up&combo_points<5|buff.darkest_night.up&combo_points.deficit>0|(talent.crimson_tempest&spell_targets.fan_of_knives>=5&(active_dot.garrote<spell_targets.fan_of_knives|active_dot.rupture<spell_targets.fan_of_knives))", "Build combo points until 5, max with darkest night, or ignore finishers to spread more bleeds with CT" );
  default_->add_action( "call_action_list,name=spend,if=!buff.darkest_night.up&combo_points>=5|buff.darkest_night.up&combo_points.deficit=0", "If combo point threshold is reached, spend them" );

  cds->add_action( "deathmark,if=dot.garrote.ticking&dot.rupture.ticking&cooldown.kingsbane.remains<=2&buff.envenom.up", "Cooldown list  Deathmark if bleeds are active, kingsbane is ready, and we have envenom TODO:check envenom buff requirement when apex talents are fixed" );
  cds->add_action( "call_action_list,name=items", "Check for on-use trinket usage" );
  cds->add_action( "call_action_list,name=misc_cds", "Check for Racial abilties, potions, and any other misc cooldowns" );
  cds->add_action( "kingsbane,if=dot.garrote.ticking&dot.rupture.ticking&(dot.deathmark.ticking|cooldown.deathmark.remains>52)", "Kingsbane if bleeds are active and Deathmark is either on cooldown or active. TODO:check envenom buff requirement when apex talents are fixed" );
  cds->add_action( "call_action_list,name=vanish,if=!stealthed.rogue", "Vanish conditions for Improved Garrote" );

  vanish->add_action( "vanish,if=variable.single_target&talent.improved_garrote&dot.garrote.pmultiplier<=1&!cooldown.deathmark.ready&!raid_event.adds.in<=30", "Vanish list  Single Target vanish check to line up improved garrote with Deathmark, making sure there are no adds soon" );
  vanish->add_action( "vanish,if=!variable.single_target&talent.improved_garrote&dot.garrote.pmultiplier<=1&(raid_event.adds.remains>=10|!raid_event.adds.in<=30)", "AoE vanish check to spread improved garrote in multitarget" );

  core_dot->add_action( "garrote,if=(buff.improved_garrote.up|stealthed.rogue)&(pmultiplier<=1|remains<=14+6*talent.razor_wire+4*!variable.single_target)", "DoT list  Garrote for improved garrote when applicable" );
  core_dot->add_action( "garrote,if=combo_points.deficit>=1&(pmultiplier<=1|!variable.single_target)&refreshable&target.time_to_die-remains>12", "Normal Garrote Maintanence" );
  core_dot->add_action( "rupture,if=combo_points>=5&refreshable&target.time_to_die-remains>12&(!buff.darkest_night.up|!dot.rupture.ticking)", "Normal Rupture Maintanence, making sure to not waste Darkest Night" );

  generate->add_action( "crimson_tempest,if=!variable.single_target&(active_dot.garrote<spell_targets.fan_of_knives|active_dot.rupture<spell_targets.fan_of_knives)", "Generator List  Crimson Tempest to spread bleeds to everything in AoE" );
  generate->add_action( "shiv,if=buff.darkest_night.up&combo_points.deficit=1&spell_targets.fan_of_knives<=3&talent.toxic_stiletto", "Special Edge Case to use Shiv for Darkest Night in low target cleave as Toxic Stiletto makes it very efficient" );
  generate->add_action( "ambush,if=spell_targets.fan_of_knives<=1+talent.blindside", "Ambush on low target counts when available" );
  generate->add_action( "mutilate,if=spell_targets.fan_of_knives<=1+talent.blindside", "Mutilate on low target counts" );
  generate->add_action( "fan_of_knives,if=spell_targets.fan_of_knives>1+talent.blindside", "Fan of Knives in AoE to fill if nothing else" );

  spend->add_action( "envenom,if=buff.implacable_tracker.stack<4", "Spend List  Envenom if we are not at max stacks of the Apex talent" );
  spend->add_action( "envenom,if=energy.pct>70", "Envenom if we are going to overcap on energy" );

  items->add_action( "variable,name=base_trinket_condition,value=dot.rupture.ticking&cooldown.deathmark.remains<2&!cooldown.deathmark.ready|dot.deathmark.ticking|fight_remains<=22", "Special Case Trinkets" );
  items->add_action( "use_item,name=astral_gladiators_badge_of_ferocity,use_off_gcd=1,if=dot.kingsbane.ticking|dot.deathmkark.ticking|(cooldown.kingsbane.remains>60|cooldown.deathmark.remains>60)" );
  items->add_action( "use_item,name=treacherous_transmitter,use_off_gcd=1,if=variable.base_trinket_condition" );
  items->add_action( "use_item,name=unyielding_netherprism,use_off_gcd=1,if=dot.deathmark.ticking&(buff.latent_energy.stack>=16|fight_remains<=90|(!trinket.2.cooldown.ready|!trinket.1.cooldown.ready))|fight_remains<=20" );
  items->add_action( "use_item,name=mad_queens_mandate,if=cooldown.deathmark.remains>=30&!dot.deathmark.ticking|fight_remains<=3" );
  items->add_action( "use_item,name=junkmaestros_mega_magnet,if=cooldown.deathmark.remains>=30&!dot.deathmark.ticking&(!talent.deathstalkers_mark|buff.lingering_darkness.up&buff.junkmaestros_mega_magnet.stack>5)|fight_remains<=10" );
  items->add_action( "do_treacherous_transmitter_task,use_off_gcd=1,if=dot.deathmark.ticking&variable.single_target|buff.realigning_nexus_convergence_divergence.up&buff.realigning_nexus_convergence_divergence.remains<=2|buff.cryptic_instructions.up&buff.cryptic_instructions.remains<=2|buff.errant_manaforge_emission.up&buff.errant_manaforge_emission.remains<=2|fight_remains<=15" );
  items->add_action( "use_item,name=imperfect_ascendancy_serum,use_off_gcd=1,if=variable.base_trinket_condition" );
  items->add_action( "use_items,slots=trinket1,if=(variable.trinket_sync_slot=1&(debuff.deathmark.up)|(variable.trinket_sync_slot=2&!trinket.2.cooldown.ready&cooldown.deathmark.remains>20))|!variable.trinket_sync_slot|fight_remains<=20", "Fallback case for using stat trinkets" );
  items->add_action( "use_items,slots=trinket2,if=(variable.trinket_sync_slot=2&(debuff.deathmark.up)|(variable.trinket_sync_slot=1&!trinket.1.cooldown.ready&cooldown.deathmark.remains>20))|!variable.trinket_sync_slot|fight_remains<=20" );

  misc_cds->add_action( "potion,if=buff.bloodlust.react|fight_remains<30|debuff.deathmark.up", "Miscellaneous Cooldowns Potion" );
  misc_cds->add_action( "blood_fury,use_off_gcd=1,if=debuff.deathmark.up", "Various special racials to be synced with cooldowns" );
  misc_cds->add_action( "berserking,use_off_gcd=1,if=debuff.deathmark.up" );
  misc_cds->add_action( "fireblood,use_off_gcd=1,if=debuff.deathmark.up" );
  misc_cds->add_action( "ancestral_call,use_off_gcd=1,if=debuff.deathmark.up" );
}
//assassination_apl_end

//outlaw_apl_start
void outlaw( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* build = p->get_action_priority_list( "build" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* finish = p->get_action_priority_list( "finish" );

  precombat->add_action( "apply_poison,nonlethal=none,lethal=instant" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "stealth,precombat_seconds=2" );
  precombat->add_action( "adrenaline_rush,precombat_seconds=1,if=talent.improved_adrenaline_rush" );
  precombat->add_action( "slice_and_dice,precombat_seconds=1,if=talent.improved_adrenaline_rush" );
  precombat->add_action( "roll_the_bones,precombat_seconds=0,if=buff.loaded_dice.up" );

  default_->add_action( "stealth", "Restealth if possible (no vulnerable enemies in combat)." );
  default_->add_action( "kick", "Interrupt on cooldown to allow simming interactions with that." );
  default_->add_action( "variable,name=ambush_condition,value=(talent.hidden_opportunity|combo_points.deficit>=2+talent.improved_ambush)&energy>=50" );
  default_->add_action( "variable,name=finish_condition,value=combo_points>=cp_max_spend-1-(hero_tree.fatebound&!cooldown.between_the_eyes.ready)", "Use finishers if at -1 from max combo points, but Fatebound uses Dispatch at -2." );
  default_->add_action( "variable,name=blade_flurry_sync,value=spell_targets.blade_flurry<2&raid_event.adds.in>20|buff.blade_flurry.up" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "run_action_list,name=finish,if=variable.finish_condition" );
  default_->add_action( "call_action_list,name=build" );
  default_->add_action( "arcane_torrent,if=energy.base_deficit>=15+energy.regen" );
  default_->add_action( "arcane_pulse" );
  default_->add_action( "lights_judgment" );
  default_->add_action( "bag_of_tricks" );

  build->add_action( "ambush,if=talent.hidden_opportunity&buff.audacity.up", "Builders   High priority Ambush with Hidden Opportunity." );
  build->add_action( "blade_flurry,if=talent.deft_maneuvers&spell_targets>=4", "With Deft Maneuvers, build CPs with Blade Flurry at 4+ targets." );
  build->add_action( "coup_de_grace,if=buff.disorienting_strikes.up", "Prioritize Coup de Grace if Unseen Blade is guaranteed after Killing Spree." );
  build->add_action( "pistol_shot,if=talent.audacity&talent.hidden_opportunity&buff.opportunity.up&!buff.audacity.up", "With Audacity + Hidden Opportunity, consume Opportunity to proc Audacity any time Ambush is not available." );
  build->add_action( "pistol_shot,if=talent.fan_the_hammer&buff.opportunity.up&(buff.opportunity.stack>=buff.opportunity.max_stack|buff.opportunity.remains<2)", "With Fan the Hammer, consume Opportunity if at max stacks or if it will expire." );
  build->add_action( "pistol_shot,if=talent.fan_the_hammer&buff.opportunity.up&(combo_points.deficit>=(1+talent.quick_draw+(talent.quick_draw*talent.fan_the_hammer.rank))&(combo_points>1|rtb_buffs<2|!talent.deal_fate))", "With Fan the Hammer, consume Opportunity if it will not overcap CPs. Fatebound with stage 2 RTB tries to avoid consuming PS at 1CP." );
  build->add_action( "pistol_shot,if=!talent.fan_the_hammer&buff.opportunity.up&(energy.base_deficit>energy.regen*1.5|combo_points.deficit<=1|talent.quick_draw.enabled|talent.audacity.enabled&!buff.audacity.up)", "If not using Fan the Hammer, then consume Opportunity based on energy, when it will exactly cap CPs, or when using Quick Draw." );
  build->add_action( "pool_resource,for_next=1", "Fallback pooling just so Hidden Opportunity builds do not skip Ambush at low energy." );
  build->add_action( "ambush,if=talent.hidden_opportunity" );
  build->add_action( "sinister_strike" );

  cds->add_action( "adrenaline_rush,if=!buff.adrenaline_rush.up&(!variable.finish_condition|!talent.improved_adrenaline_rush)&(raid_event.adds.remains>5|raid_event.adds.in<5|!raid_event.adds.exists|!raid_event.adds.count)", "Cooldowns   Maintain Adrenaline Rush. With Improved AR, use at low CPs. Has a cursory check to try not to send if immediate downtime is expected." );
  cds->add_action( "blade_flurry,if=spell_targets>=2&buff.blade_flurry.remains<gcd", "Maintain Blade Flurry at 2+ targets." );
  cds->add_action( "preparation,if=cooldown.adrenaline_rush.remains>30&!cooldown.between_the_eyes.ready&(!cooldown.killing_spree.ready|!hero_tree.trickster)|fight_remains<30", "Use Preparation to reset Adrenaline Rush, Between the Eyes, and Killing Spree if Trickster." );
  cds->add_action( "keep_it_rolling,if=rtb_buffs=2&buff.roll_the_bones.remains<cooldown.adrenaline_rush.remains&!buff.loaded_dice.up&(cooldown.preparation.remains|!talent.preparation)|rtb_buffs>=3", "Use Keep it Rolling with at least stage 2 of RtB. Try not to KIR at stage 2 if your next roll is guaranteed to have Loaded Dice." );
  cds->add_action( "roll_the_bones,if=!buff.roll_the_bones.up|rtb_buffs=1", "Use Roll the Bones if not active, or reroll for stage 2." );
  cds->add_action( "blade_rush,if=set_bonus.mid1_4pc&!buff.whirl_of_blades.up|spell_targets=1&energy.base_time_to_max>2|spell_targets>=2", "Use Blade Rush if tier bonus is not active, or in AoE, or if you will not overcap energy within the gcd on ST." );
  cds->add_action( "vanish,if=!variable.finish_condition&talent.hidden_opportunity&!buff.audacity.up&!buff.opportunity.up", "Hidden Opportunity builds use Vanish or Shadowmeld for an extra Ambush in between procs." );
  cds->add_action( "shadowmeld,if=!variable.finish_condition&talent.hidden_opportunity&!buff.audacity.up&!buff.opportunity.up" );
  cds->add_action( "potion,if=buff.bloodlust.react|fight_remains<30|buff.adrenaline_rush.up" );
  cds->add_action( "blood_fury" );
  cds->add_action( "berserking" );
  cds->add_action( "fireblood" );
  cds->add_action( "ancestral_call" );
  cds->add_action( "use_items,slots=trinket1,if=buff.between_the_eyes.up|trinket.1.has_stat.any_dps|fight_remains<=20", "Default conditions for usable items." );
  cds->add_action( "use_items,slots=trinket2,if=buff.between_the_eyes.up|trinket.2.has_stat.any_dps|fight_remains<=20" );

  finish->add_action( "dispatch,if=!buff.slice_and_dice.up", "Finishers" );
  finish->add_action( "between_the_eyes" );
  finish->add_action( "pool_resource,for_next=1" );
  finish->add_action( "killing_spree" );
  finish->add_action( "coup_de_grace" );
  finish->add_action( "dispatch" );
}
//outlaw_apl_end

//subtlety_apl_start
void subtlety( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* race = p->get_action_priority_list( "race" );
  action_priority_list_t* item = p->get_action_priority_list( "item" );
  action_priority_list_t* finish = p->get_action_priority_list( "finish" );
  action_priority_list_t* build = p->get_action_priority_list( "build" );
  action_priority_list_t* fill = p->get_action_priority_list( "fill" );

  precombat->add_action( "apply_poison" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=priority_rotation,value=priority_rotation" );
  precombat->add_action( "variable,name=trinket_sync_slot,value=1,if=trinket.1.has_use_buff&(!trinket.2.has_use_buff|trinket.1.cooldown.duration>=trinket.2.cooldown.duration)" );
  precombat->add_action( "variable,name=trinket_sync_slot,value=2,if=trinket.2.has_use_buff&(!trinket.1.has_use_buff|trinket.2.cooldown.duration>trinket.1.cooldown.duration)" );
  precombat->add_action( "stealth" );

  default_->add_action( "stealth" );
  default_->add_action( "variable,name=stealth,value=buff.shadow_dance.up|buff.stealth.up|buff.vanish.up" );
  default_->add_action( "variable,name=targets,value=spell_targets.shuriken_storm" );
  default_->add_action( "variable,name=racial_sync,value=(buff.shadow_blades.up&buff.shadow_dance.up)|fight_remains<20" );
  default_->add_action( "variable,name=shd_cp,value=combo_points<=2&talent.deathstalkers_mark|talent.unseen_blade&(combo_points<=1|combo_points>=6)" );
  default_->add_action( "call_action_list,name=race" );
  default_->add_action( "call_action_list,name=item" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=finish,if=combo_points>=cp_max_spend-!buff.darkest_night.up" );
  default_->add_action( "call_action_list,name=build" );
  default_->add_action( "call_action_list,name=fill,if=!variable.stealth" );

  cds->add_action( "potion,if=buff.bloodlust.react|fight_remains<30|buff.shadow_blades.up", "Cooldowns" );
  cds->add_action( "shadow_blades,if=variable.shd_cp&cooldown.shadow_dance.ready" );
  cds->add_action( "shadow_dance,if=!variable.stealth&variable.shd_cp&(buff.shadow_blades.up|cooldown.secret_technique.ready&cooldown.shadow_blades.remains>7)" );
  cds->add_action( "thistle_tea" );
  cds->add_action( "vanish,if=!variable.stealth&energy>=40&!buff.subterfuge.up&combo_points<=1" );
  cds->add_action( "shadowmeld,if=energy>=40&combo_points.deficit>=3" );

  race->add_action( "blood_fury,if=variable.racial_sync", "Race Cooldowns" );
  race->add_action( "berserking,if=variable.racial_sync" );
  race->add_action( "fireblood,if=variable.racial_sync" );
  race->add_action( "ancestral_call,if=variable.racial_sync" );
  race->add_action( "invoke_external_buff,name=power_infusion,if=variable.racial_sync" );

  item->add_action( "use_item,name=unyielding_netherprism,use_off_gcd=1,if=buff.shadow_blades.up&(buff.latent_energy.stack>=8+8*(trinket.arazs_ritual_forge.cooldown.ready|!equipped.arazs_ritual_forge)|!equipped.arazs_ritual_forge&fight_remains<=90)|fight_remains<=20", "Trinket and Items" );
  item->add_action( "use_items,slots=trinket1,if=(variable.trinket_sync_slot=1&(buff.shadow_blades.up|fight_remains<=20+equipped.unyielding_netherprism*20)|(variable.trinket_sync_slot=2&(!trinket.2.cooldown.ready&cooldown.shadow_blades.remains>20))|!variable.trinket_sync_slot)" );
  item->add_action( "use_items,slots=trinket2,if=(variable.trinket_sync_slot=2&(buff.shadow_blades.up|fight_remains<=20+equipped.unyielding_netherprism*20)|(variable.trinket_sync_slot=1&(!trinket.1.cooldown.ready&cooldown.shadow_blades.remains>20))|!variable.trinket_sync_slot)" );

  finish->add_action( "secret_technique,if=buff.shadow_dance.up" );
  finish->add_action( "eviscerate,if=buff.darkest_night.up&!debuff.deathstalkers_mark.up" );
  finish->add_action( "coup_de_grace" );
  finish->add_action( "black_powder,if=variable.targets>=2" );
  finish->add_action( "eviscerate" );

  build->add_action( "shadowstrike,if=!debuff.deathstalkers_mark.up|variable.targets<=2" );
  build->add_action( "shuriken_storm,if=variable.targets>1" );
  build->add_action( "goremaws_bite,if=combo_points.deficit>=3" );
  build->add_action( "gloomblade" );
  build->add_action( "backstab" );

  fill->add_action( "arcane_torrent,if=energy.deficit>=15+energy.regen", "This list usually contains Cooldowns with negligible impact that causes global cooldowns" );
  fill->add_action( "arcane_pulse" );
  fill->add_action( "lights_judgment" );
  fill->add_action( "bag_of_tricks" );
}
//subtlety_apl_end

} // namespace rogue_apl
