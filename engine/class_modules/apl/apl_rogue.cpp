#include "simulationcraft.hpp"
#include "class_modules/apl/apl_rogue.hpp"

namespace rogue_apl {

std::string potion( const player_t* p )
{
  return ( ( p->true_level >= 81 ) ? "lights_potential_2" :
           ( p->true_level >= 71 ) ? "tempered_potion_3" :
           ( p->true_level >= 61 ) ? "elemental_potion_of_ultimate_power_3" :
           ( p->true_level >= 51 ) ? "potion_of_spectral_agility" :
           ( p->true_level >= 40 ) ? "potion_of_unbridled_fury" :
           ( p->true_level >= 35 ) ? "draenic_agility" :
           "disabled" );
}

std::string flask( const player_t* p )
{
  return ( ( p->true_level >= 81 ) ? "flask_of_the_shattered_sun_2" :
           ( p->true_level >= 71 ) ? "flask_of_alchemical_chaos_3" :
           ( p->true_level >= 61 ) ? "iced_phial_of_corrupting_rage_3" :
           ( p->true_level >= 51 ) ? "spectral_flask_of_power" :
           ( p->true_level >= 40 ) ? "greater_flask_of_the_currents" :
           ( p->true_level >= 35 ) ? "greater_draenic_agility_flask" :
           "disabled" );
}

std::string food( const player_t* p )
{
  return ( ( p->true_level >= 81 ) ? "harandar_celebration" :
           ( p->true_level >= 71 ) ? "feast_of_the_divine_day" :
           ( p->true_level >= 61 ) ? "fated_fortune_cookie" :
           ( p->true_level >= 51 ) ? "feast_of_gluttonous_hedonism" :
           ( p->true_level >= 45 ) ? "famine_evaluator_and_snack_table" :
           ( p->true_level >= 40 ) ? "lavish_suramar_feast" :
           "disabled" );
}

std::string rune( const player_t* p )
{
  return ( ( p->true_level >= 90 ) ? "void_touched" :
           ( p->true_level >= 80 ) ? "crystallized" :
           ( p->true_level >= 70 ) ? "draconic" :
           ( p->true_level >= 60 ) ? "veiled" :
           ( p->true_level >= 50 ) ? "battle_scarred" :
           ( p->true_level >= 45 ) ? "defiled" :
           ( p->true_level >= 40 ) ? "hyper" :
           "disabled" );
}

std::string temporary_enchant( const player_t* p )
{
  return ( ( p->true_level >= 81 ) ? "main_hand:thalassian_phoenix_oil_2/off_hand:thalassian_phoenix_oil_2" :
           ( p->true_level >= 71 ) ? "main_hand:ironclaw_whetstone_3/off_hand:ironclaw_whetstone_3" :
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
  action_priority_list_t* core_dot = p->get_action_priority_list( "core_dot" );
  action_priority_list_t* generate = p->get_action_priority_list( "generate" );
  action_priority_list_t* items = p->get_action_priority_list( "items" );
  action_priority_list_t* misc_cds = p->get_action_priority_list( "misc_cds" );
  action_priority_list_t* spend = p->get_action_priority_list( "spend" );
  action_priority_list_t* vanish = p->get_action_priority_list( "vanish" );

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
  default_->add_action( "ambush,if=stealthed.rogue&variable.single_target&talent.blindside&talent.improved_ambush&!talent.shrouded_suffocation", "Special Ambush condition for the start of fights when applicable" );
  default_->add_action( "call_action_list,name=cds", "Cooldown list takes priority" );
  default_->add_action( "call_action_list,name=core_dot", "Maintain dots when possible" );
  default_->add_action( "call_action_list,name=generate,if=!buff.darkest_night.up&combo_points<5|buff.darkest_night.up&combo_points.deficit>0", "Build combo points until 5, max with darkest night" );
  default_->add_action( "call_action_list,name=spend,if=!buff.darkest_night.up&combo_points>=5|buff.darkest_night.up&combo_points.deficit=0", "If combo point threshold is reached, spend them" );

  cds->add_action( "deathmark,if=dot.garrote.ticking&dot.rupture.ticking&cooldown.kingsbane.remains<=2&buff.envenom.remains>2&(target.time_to_die>10|fight_remains<20)", "Cooldown list Deathmark if bleeds are active, kingsbane is ready, and we have envenom" );
  cds->add_action( "call_action_list,name=items", "Check for on-use trinket usage" );
  cds->add_action( "call_action_list,name=misc_cds", "Check for Racial abilties, potions, and any other misc cooldowns" );
  cds->add_action( "kingsbane,if=dot.garrote.ticking&dot.rupture.ticking&(dot.deathmark.ticking|cooldown.deathmark.remains>52)&buff.envenom.up&(target.time_to_die>10|fight_remains<20)", "Kingsbane if bleeds are active and Deathmark is either on cooldown or active." );
  cds->add_action( "call_action_list,name=vanish,if=!stealthed.rogue", "Vanish conditions for Improved Garrote" );

  core_dot->add_action( "garrote,if=(buff.improved_garrote.up|stealthed.rogue)&(pmultiplier<=1|remains<=14+6*talent.razor_wire+4*!variable.single_target)", "DoT list Garrote for improved garrote when applicable" );
  core_dot->add_action( "garrote,if=combo_points.deficit>=1&(pmultiplier<=1|!variable.single_target)&refreshable&target.time_to_die-remains>12", "Normal Garrote Maintanence" );
  core_dot->add_action( "garrote,cycle_targets=1,if=!talent.crimson_tempest&combo_points.deficit>=1&(pmultiplier<=1|!variable.single_target)&refreshable&target.time_to_die-remains>12", "Cycle" );
  core_dot->add_action( "rupture,if=combo_points>=5&refreshable&target.time_to_die-remains>12&(!buff.darkest_night.up|!dot.rupture.ticking)", "Normal Rupture Maintanence, making sure to not waste Darkest Night" );
  core_dot->add_action( "rupture,cycle_targets=1,if=!talent.crimson_tempest&combo_points>=5&refreshable&target.time_to_die-remains>12&(!buff.darkest_night.up|!dot.rupture.ticking)" );

  generate->add_action( "crimson_tempest,target_if=max:dot.rupture.remains,if=!variable.single_target&(active_dot.garrote<spell_targets.fan_of_knives|active_dot.rupture<spell_targets.fan_of_knives)&(dot.rupture.remains>5|energy.regen_combined>40)", "Generator List Crimson Tempest to spread bleeds to everything in AoE" );
  generate->add_action( "shiv,if=buff.darkest_night.up&combo_points.deficit=1&spell_targets.fan_of_knives<=3&talent.toxic_stiletto", "Special Edge Case to use Shiv for Darkest Night in low target cleave as Toxic Stiletto makes it very efficient" );
  generate->add_action( "ambush,if=spell_targets.fan_of_knives<=1+talent.blindside", "Ambush on low target counts when available" );
  generate->add_action( "mutilate,if=spell_targets.fan_of_knives<=1+talent.blindside", "Mutilate on low target counts" );
  generate->add_action( "fan_of_knives,if=spell_targets.fan_of_knives>1+talent.blindside", "Fan of Knives in AoE to fill if nothing else" );

  items->add_action( "variable,name=base_trinket_condition,value=dot.rupture.ticking&cooldown.deathmark.remains<2|dot.deathmark.ticking|fight_remains<=22", "Special Case Trinkets" );
  items->add_action( "use_item,name=astral_gladiators_badge_of_ferocity,use_off_gcd=1,if=dot.kingsbane.ticking|dot.deathmark.ticking|(cooldown.kingsbane.remains>60|cooldown.deathmark.remains>60)" );
  items->add_action( "use_item,name=algethar_puzzle_box,use_off_gcd=1,if=variable.base_trinket_condition&buff.envenom.up" );
  items->add_action( "use_items,slots=trinket1,if=(variable.trinket_sync_slot=1&(debuff.deathmark.up)|(variable.trinket_sync_slot=2&!trinket.2.cooldown.ready&cooldown.deathmark.remains>20))|!variable.trinket_sync_slot|fight_remains<=20" );
  items->add_action( "use_items,slots=trinket2,if=(variable.trinket_sync_slot=2&(debuff.deathmark.up)|(variable.trinket_sync_slot=1&!trinket.1.cooldown.ready&cooldown.deathmark.remains>20))|!variable.trinket_sync_slot|fight_remains<=20" );

  misc_cds->add_action( "potion,if=dot.rupture.ticking&(buff.bloodlust.react|fight_remains<30|debuff.deathmark.up)", "Miscellaneous Cooldowns Potion" );
  misc_cds->add_action( "blood_fury,use_off_gcd=1,if=debuff.deathmark.up", "Various special racials to be synced with cooldowns" );
  misc_cds->add_action( "berserking,use_off_gcd=1,if=debuff.deathmark.up" );
  misc_cds->add_action( "fireblood,use_off_gcd=1,if=debuff.deathmark.up" );
  misc_cds->add_action( "ancestral_call,use_off_gcd=1,if=debuff.deathmark.up" );

  spend->add_action( "envenom,if=buff.implacable_tracker.stack<4", "Spend List Envenom if we are not at max stacks of the Apex talent" );
  spend->add_action( "envenom,if=energy.pct>70|fight_remains<15", "Envenom if we are going to overcap on energy" );

  vanish->add_action( "vanish,if=variable.single_target&talent.improved_garrote&dot.garrote.pmultiplier<=1&(dot.deathmark.ticking|cooldown.deathmark.remains>target.time_to_die-10)&!raid_event.adds.in<=30", "Vanish list Single Target vanish check to line up improved garrote with Deathmark, making sure there are no adds soon. TODO Check after ImpGar fixes" );
  vanish->add_action( "vanish,if=!variable.single_target&talent.improved_garrote&dot.garrote.pmultiplier<=1&(raid_event.adds.remains>=10|!raid_event.adds.in<=30)", "AoE vanish check to spread improved garrote in multitarget" );
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
  default_->add_action( "variable,name=finish_condition,value=combo_points>=cp_max_spend-1-(!cooldown.between_the_eyes.ready&(hero_tree.fatebound|cooldown.killing_spree.ready))", "Use finishers if at -1 from max combo points, but Killing Spree is used at -2, and Fatebound uses Dispatch at -2." );
  default_->add_action( "variable,name=blade_flurry_sync,value=spell_targets.blade_flurry<2&raid_event.adds.in>20|buff.blade_flurry.up" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "run_action_list,name=finish,if=variable.finish_condition" );
  default_->add_action( "call_action_list,name=build" );
  default_->add_action( "arcane_torrent,if=energy.base_deficit>=15+energy.regen" );
  default_->add_action( "arcane_pulse" );
  default_->add_action( "lights_judgment" );
  default_->add_action( "bag_of_tricks" );

  build->add_action( "ambush,if=talent.hidden_opportunity&buff.audacity.up", "Builders  High priority Ambush with Hidden Opportunity." );
  build->add_action( "blade_flurry,if=talent.deft_maneuvers&spell_targets>=3", "With Deft Maneuvers, build CPs with Blade Flurry at 3+ targets." );
  build->add_action( "coup_de_grace,if=buff.disorienting_strikes.up", "Prioritize Coup de Grace if Unseen Blade is guaranteed after Killing Spree." );
  build->add_action( "pistol_shot,if=talent.audacity&talent.hidden_opportunity&buff.opportunity.up&!buff.audacity.up", "With Audacity + Hidden Opportunity, consume Opportunity to proc Audacity any time Ambush is not available." );
  build->add_action( "pistol_shot,if=talent.fan_the_hammer&buff.opportunity.up&(buff.opportunity.stack>=buff.opportunity.max_stack|buff.opportunity.remains<2)", "With Fan the Hammer, consume Opportunity if at max stacks or if it will expire." );
  build->add_action( "pistol_shot,if=talent.fan_the_hammer&buff.opportunity.up&(combo_points.deficit>=(1+talent.quick_draw+(talent.quick_draw*talent.fan_the_hammer.rank))&(combo_points>1|rtb_buffs<2|!talent.deal_fate))", "With Fan the Hammer, consume Opportunity if it will not overcap CPs. Fatebound with stage 2 RTB tries to avoid consuming PS at 1CP." );
  build->add_action( "pistol_shot,if=!talent.fan_the_hammer&buff.opportunity.up&(energy.base_deficit>energy.regen*1.5|combo_points.deficit<=1|talent.quick_draw.enabled|talent.audacity.enabled&!buff.audacity.up)", "If not using Fan the Hammer, then consume Opportunity based on energy, when it will exactly cap CPs, or when using Quick Draw." );
  build->add_action( "pool_resource,for_next=1", "Fallback pooling just so Hidden Opportunity builds do not skip Ambush at low energy." );
  build->add_action( "ambush,if=talent.hidden_opportunity" );
  build->add_action( "sinister_strike" );

  cds->add_action( "adrenaline_rush,if=!buff.adrenaline_rush.up&(!variable.finish_condition|!talent.improved_adrenaline_rush)&(raid_event.adds.remains>5|raid_event.adds.in<5|!raid_event.adds.exists|!raid_event.adds.count)", "Cooldowns  Maintain Adrenaline Rush. With Improved AR, use at low CPs. Has a cursory check to try not to send if immediate downtime is expected." );
  cds->add_action( "blade_flurry,if=spell_targets>=2&buff.blade_flurry.remains<gcd", "Maintain Blade Flurry at 2+ targets." );
  cds->add_action( "preparation,if=cooldown.adrenaline_rush.remains>30&!cooldown.between_the_eyes.ready|fight_remains<30", "Use Preparation to reset Adrenaline Rush and Between the Eyes." );
  cds->add_action( "keep_it_rolling,if=rtb_buffs>=3", "Use Keep it Rolling with at least stage 3 of RtB." );
  cds->add_action( "roll_the_bones,if=!buff.roll_the_bones.up|rtb_buffs=1+(buff.loaded_dice.up&cooldown.between_the_eyes.ready)", "Use Roll the Bones if not active, or reroll for stage 2. Roll over stage 2 if both Loaded Dice is active and KIR is ready." );
  cds->add_action( "blade_rush,if=set_bonus.mid1_2pc|spell_targets=1&energy.base_time_to_max>2|spell_targets>=2", "Use Blade Rush if using tier, or in AoE, or if you will not overcap energy within the gcd on ST." );
  cds->add_action( "vanish,if=!variable.finish_condition&talent.hidden_opportunity&!buff.audacity.up&!buff.opportunity.up", "Hidden Opportunity builds use Vanish or Shadowmeld for an extra Ambush in between procs." );
  cds->add_action( "shadowmeld,if=!variable.finish_condition&talent.hidden_opportunity&!buff.audacity.up&!buff.opportunity.up" );
  cds->add_action( "potion,if=buff.bloodlust.react|fight_remains<30|buff.adrenaline_rush.up" );
  cds->add_action( "blood_fury" );
  cds->add_action( "berserking" );
  cds->add_action( "fireblood" );
  cds->add_action( "ancestral_call" );
  cds->add_action( "use_items,slots=trinket1,if=buff.between_the_eyes.up|trinket.1.has_stat.any_dps|fight_remains<=20", "Default conditions for usable items." );
  cds->add_action( "use_items,slots=trinket2,if=buff.between_the_eyes.up|trinket.2.has_stat.any_dps|fight_remains<=20" );

  finish->add_action( "between_the_eyes,if=cooldown.adrenaline_rush.remains>30|buff.adrenaline_rush.up|!talent.supercharger|!talent.zero_in", "Finishers  With Supercharger and Zero In, hold BtE for an upcoming Adrenaline Rush" );
  finish->add_action( "pool_resource,for_next=1" );
  finish->add_action( "killing_spree,interrupt_if=energy.time_to_max<2,interrupt_global=1", "Cancel Killing Spree with a builder/finisher if approaching max energy." );
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
  precombat->add_action( "variable,name=trinket_sync_slot,value=1,if=trinket.1.has_use_buff&(!trinket.2.has_use_buff|trinket.1.cooldown.duration>=trinket.2.cooldown.duration)", "Check for on-use stat trinkets and which slot has the most powerful effect (ie longest cooldown)." );
  precombat->add_action( "variable,name=trinket_sync_slot,value=2,if=trinket.2.has_use_buff&(!trinket.1.has_use_buff|trinket.2.cooldown.duration>trinket.1.cooldown.duration)" );
  precombat->add_action( "stealth" );

  default_->add_action( "variable,name=stealth,value=buff.shadow_dance.up|buff.stealth.up|buff.vanish.up" );
  default_->add_action( "variable,name=targets,value=spell_targets.shuriken_storm" );
  default_->add_action( "variable,name=racial_sync,value=(buff.shadow_blades.up&buff.shadow_dance.up)|fight_remains<20" );
  default_->add_action( "variable,name=shd_cp,value=buff.slice_and_dice.up&combo_points<=2&talent.deathstalkers_mark|combo_points>=6&(!talent.deathstalkers_mark|variable.targets>=5)" );
  default_->add_action( "stealth" );
  default_->add_action( "call_action_list,name=race" );
  default_->add_action( "call_action_list,name=item" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "shadowstrike,if=talent.ancient_arts_3&variable.targets<=2&(buff.darkest_night.up|(talent.unseen_blade&buff.supercharge_1.up))&buff.shadow_techniques.stack>=5&!buff.ancient_arts.up" );
  default_->add_action( "shuriken_storm,if=talent.ancient_arts_3&variable.targets>=3&(buff.supercharge_1.up)&buff.shadow_techniques.stack>=5&!buff.ancient_arts.up&!cooldown.secret_technique.ready" );
  default_->add_action( "call_action_list,name=finish,if=combo_points>=cp_max_spend-!buff.darkest_night.up" );
  default_->add_action( "call_action_list,name=build,if=variable.stealth|energy>60" );
  default_->add_action( "call_action_list,name=fill,if=!variable.stealth" );

  cds->add_action( "shadow_blades,if=variable.shd_cp&cooldown.shadow_dance.charges_fractional>=1+0.8*talent.deathstalkers_mark&cooldown.secret_technique.ready&(fight_remains>90|!equipped.algethar_puzzle_box|trinket.1.proc.mastery.up|trinket.2.proc.mastery.up)|(fight_remains<=20|target.time_to_die.remains<=20)", "Cooldowns  Delay the last Shadow Blades to line up with puzzle box if its equipped." );
  cds->add_action( "shadow_dance,if=!variable.stealth&variable.shd_cp&energy>=30&((cooldown.secret_technique.ready|buff.darkest_night.up)&(cooldown.shadow_blades.remains>=30-cooldown.secret_technique.duration)|(buff.shadow_blades.up&cooldown.secret_technique.duration>=18))|(fight_remains<=10|target.time_to_die-remains<=9)" );
  cds->add_action( "shadow_dance,if=buff.shadow_blades.up&talent.unseen_blade&buff.shadow_blades.remains<=buff.shadow_dance.duration+1", "Have the second Shadow Dance in Shadow Blades line up with the end of Shadow Blades instead of back-to-back for trickster." );
  cds->add_action( "shadow_dance,if=equipped.algethar_puzzle_box&talent.unseen_blade&!variable.stealth&variable.shd_cp&energy>=30&((cooldown.secret_technique.ready|buff.darkest_night.up)&(trinket.algethar_puzzle_box.cooldown.remains>=39-30*cooldown.shadow_blades.up))", "Used for when Shadow Blades is ready but holding for Algethar Puzzlebox trinket at the end of pull" );
  cds->add_action( "vanish,if=!variable.stealth&energy>=50&!buff.subterfuge.up&combo_points<=2" );
  cds->add_action( "shadowmeld,if=energy>=50&!variable.stealth&combo_points.deficit>=2" );

  race->add_action( "blood_fury,if=variable.racial_sync", "Race Cooldowns" );
  race->add_action( "berserking,if=variable.racial_sync" );
  race->add_action( "fireblood,if=variable.racial_sync" );
  race->add_action( "ancestral_call,if=variable.racial_sync" );
  race->add_action( "invoke_external_buff,name=power_infusion,if=variable.racial_sync" );

  item->add_action( "potion,if=buff.shadow_blades.up|fight_remains<30", "Trinket and Items" );
  item->add_action( "use_item,name=algethar_puzzle_box,if=cooldown.shadow_blades.ready&cooldown.secret_technique.remains<=2&combo_points>=6" );
  item->add_action( "use_items,slots=trinket1,if=(variable.trinket_sync_slot=1&(buff.shadow_blades.up|fight_remains<=20)|(variable.trinket_sync_slot=2&(!trinket.2.cooldown.ready&cooldown.shadow_blades.remains>20))|!variable.trinket_sync_slot)" );
  item->add_action( "use_items,slots=trinket2,if=(variable.trinket_sync_slot=2&(buff.shadow_blades.up|fight_remains<=20)|(variable.trinket_sync_slot=1&(!trinket.1.cooldown.ready&cooldown.shadow_blades.remains>20))|!variable.trinket_sync_slot)" );

  finish->add_action( "eviscerate,if=buff.darkest_night.up" );
  finish->add_action( "secret_technique,if=buff.shadow_dance.up|(cooldown.secret_technique.duration<18|cooldown.shadow_dance.remains>=10)&!cooldown.shadow_dance.ready" );
  finish->add_action( "coup_de_grace,if=cooldown.secret_technique.remains>=3|buff.shadow_dance.up" );
  finish->add_action( "black_powder,if=variable.targets>=3" );
  finish->add_action( "eviscerate,if=cooldown.secret_technique.remains>=3&talent.unseen_blade|buff.shadow_dance.up|buff.shadow_blades.up|debuff.deathstalkers_mark.stack>1|debuff.deathstalkers_mark.stack=1&buff.shadow_techniques.stack>=5", "Pool some Shadow Technique Stacks before entering Shadow Dance by not finishing right before." );

  build->add_action( "shuriken_storm,if=prev.shadow_dance&buff.premeditation.up&talent.danse_macabre" );
  build->add_action( "shadowstrike,if=!debuff.deathstalkers_mark.up&talent.deathstalkers_mark&!buff.darkest_night.up|variable.targets<=3|variable.priority_rotation" );
  build->add_action( "shuriken_storm,if=variable.targets>1" );
  build->add_action( "goremaws_bite,if=combo_points.deficit>=3" );
  build->add_action( "gloomblade,if=variable.targets<2&!variable.stealth" );
  build->add_action( "backstab,if=variable.targets<2&!variable.stealth" );

  fill->add_action( "arcane_torrent,if=energy.deficit>=15+energy.regen", "This list usually contains Cooldowns with negligible impact that causes global cooldowns" );
  fill->add_action( "arcane_pulse" );
  fill->add_action( "lights_judgment" );
  fill->add_action( "bag_of_tricks" );
}
//subtlety_apl_end

} // namespace rogue_apl
