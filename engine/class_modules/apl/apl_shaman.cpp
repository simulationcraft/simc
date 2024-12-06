#include "class_modules/apl/apl_shaman.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace shaman_apl
{

std::string flask_elemental( const player_t* p )
{
  return ( p->true_level >= 71 ) ? "flask_of_alchemical_chaos_3" : "disabled";
}

std::string food_elemental( const player_t* p )
{
  return ( p->true_level >= 71 ) ? "feast_of_the_divine_day" : "disabled";
}

std::string potion_elemental( const player_t* p )
{
  return ( p->true_level >= 71 ) ? "tempered_potion_3" : "disabled";
}

std::string temporary_enchant_elemental( const player_t* p )
{
  return ( p->true_level >= 71 ) ? "main_hand:algari_mana_oil_3,if=!talent.improved_flametongue_weapon"
    : "disabled";
}

std::string rune( const player_t* p )
{
  return ( p->true_level >= 71 ) ? "crystallized" : "disabled";
}

//elemental_apl_start
void elemental( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* single_target = p->get_action_priority_list( "single_target" );

  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "flametongue_weapon,if=talent.improved_flametongue_weapon.enabled", "Ensure weapon enchant is applied if you've selected Improved Flametongue Weapon." );
  precombat->add_action( "lightning_shield" );
  precombat->add_action( "thunderstrike_ward" );
  precombat->add_action( "variable,name=mael_cap,value=100+50*talent.swelling_maelstrom.enabled+25*talent.primordial_capacity.enabled,op=set" );
  precombat->add_action( "variable,name=spymaster_in_1st,value=trinket.1.is.spymasters_web" );
  precombat->add_action( "variable,name=spymaster_in_2nd,value=trinket.2.is.spymasters_web" );
  precombat->add_action( "stormkeeper" );

  default_->add_action( "spiritwalkers_grace,moving=1,if=movement.distance>6", "Enable more movement." );
  default_->add_action( "wind_shear", "Interrupt of casts." );
  default_->add_action( "blood_fury,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50" );
  default_->add_action( "berserking,if=!talent.ascendance.enabled|buff.ascendance.up" );
  default_->add_action( "fireblood,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50" );
  default_->add_action( "ancestral_call,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50" );
  default_->add_action( "use_item,slot=trinket1,if=!variable.spymaster_in_1st|(fight_remains>180-60*talent.first_ascendant&(buff.spymasters_report.stack>25|buff.spymasters_report.stack<5)|buff.spymasters_report.stack>35|fight_remains<80)&cooldown.ascendance.ready&(buff.stormkeeper.up&cooldown.stormkeeper.remains>40|!talent.fury_of_the_storms)&(buff.primordial_wave.up|!talent.primordial_wave)|fight_remains<21" );
  default_->add_action( "use_item,slot=trinket2,if=!variable.spymaster_in_2nd|(fight_remains>180-60*talent.first_ascendant&(buff.spymasters_report.stack>25|buff.spymasters_report.stack<5)|buff.spymasters_report.stack>35|fight_remains<80)&cooldown.ascendance.ready&(buff.stormkeeper.up&cooldown.stormkeeper.remains>40|!talent.fury_of_the_storms)&(buff.primordial_wave.up|!talent.primordial_wave)|fight_remains<21" );
  default_->add_action( "use_item,slot=main_hand" );
  default_->add_action( "lightning_shield,if=buff.lightning_shield.down" );
  default_->add_action( "natures_swiftness" );
  default_->add_action( "invoke_external_buff,name=power_infusion", "Use Power Infusion on Cooldown." );
  default_->add_action( "potion,if=buff.bloodlust.up|buff.spymasters_web.up|buff.ascendance.remains>12|fight_remains<31" );
  default_->add_action( "run_action_list,name=aoe,if=spell_targets.chain_lightning>=2" );
  default_->add_action( "run_action_list,name=single_target" );

  aoe->add_action( "fire_elemental" );
  aoe->add_action( "storm_elemental" );
  aoe->add_action( "stormkeeper" );
  aoe->add_action( "totemic_recall,if=cooldown.liquid_magma_totem.remains>15&(active_dot.flame_shock<(spell_targets.chain_lightning>?6)-2|talent.fire_elemental.enabled)", "Reset LMT CD as early as possible. Always for Fire, only if you can dot up 3 more targets for Lightning." );
  aoe->add_action( "liquid_magma_totem,if=cooldown.ascendance.remains>15&!buff.ascendance.up" );
  aoe->add_action( "primordial_wave,target_if=min:dot.flame_shock.remains,if=buff.surge_of_power.up|!talent.surge_of_power.enabled|maelstrom<60-5*talent.eye_of_the_storm.enabled", "Spread Flame Shock via Primordial Wave using Surge of Power if possible." );
  aoe->add_action( "ancestral_swiftness" );
  aoe->add_action( "ascendance", "JUST DO IT! https://i.kym-cdn.com/entries/icons/mobile/000/018/147/Shia_LaBeouf__Just_Do_It__Motivational_Speech_(Original_Video_by_LaBeouf__R%C3%B6nkk%C3%B6___Turner)_0-4_screenshot.jpg" );
  aoe->add_action( "flame_shock,target_if=refreshable,if=buff.surge_of_power.up&dot.flame_shock.remains<target.time_to_die-16&active_dot.flame_shock<(spell_targets.chain_lightning>?6)&!talent.liquid_magma_totem.enabled", "Spread Flame Shock using Surge of Power if LMT is not picked." );
  aoe->add_action( "flame_shock,target_if=refreshable,if=talent.fire_elemental.enabled&(buff.surge_of_power.up|!talent.surge_of_power.enabled)&dot.flame_shock.remains<target.time_to_die-5&(active_dot.flame_shock<6|dot.flame_shock.remains>0)", "Spread and refresh Flame Shock using Surge of Power (if talented) up to 6." );
  aoe->add_action( "tempest,target_if=min:debuff.lightning_rod.remains,if=!buff.arc_discharge.up&(buff.surge_of_power.up|!talent.surge_of_power.enabled)", "Buff Tempest with SoP or LB if two target" );
  aoe->add_action( "lightning_bolt,if=buff.stormkeeper.up&buff.surge_of_power.up&spell_targets.chain_lightning=2" );
  aoe->add_action( "chain_lightning,if=active_enemies>=6&buff.surge_of_power.up", "Against 6 targets or more Surge of Power should be used with Chain Lightning rather than Lava Burst." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=buff.primordial_wave.up&(buff.stormkeeper.up&spell_targets.chain_lightning>=6|buff.tempest.up)&maelstrom<60-5*talent.eye_of_the_storm.enabled&talent.surge_of_power.enabled", "Consume Primordial Wave buff immediately if you have Stormkeeper buff, fighting 6+ enemies and need maelstrom to spend." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=buff.primordial_wave.up", "Cast Lava burst to consume Primordial Wave proc. Wait for Lava Surge proc if possible." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains,if=cooldown_react&buff.lava_surge.up&!buff.master_of_the_elements.up&talent.master_of_the_elements.enabled&talent.fire_elemental.enabled", "*{Fire} Use Lava Surge proc to buff <anything> with Master of the Elements." );
  aoe->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=spell_targets.chain_lightning=2&(maelstrom>variable.mael_cap-30|cooldown.primordial_wave.remains<gcd&talent.surge_of_power.enabled|(buff.stormkeeper.up&spell_targets.chain_lightning>=6|buff.tempest.up)&talent.surge_of_power.enabled)", "Two Target" );
  aoe->add_action( "earthquake,if=cooldown.primordial_wave.remains<gcd&talent.surge_of_power.enabled&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering.enabled)", "Activate Surge of Power if next global is Primordial wave. Respect Echoes of Great Sundering." );
  aoe->add_action( "earthquake,if=(lightning_rod=0&talent.lightning_rod.enabled|maelstrom>variable.mael_cap-30)&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering.enabled)", "Spend if all Lightning Rods ran out or you are close to overcaping. Respect Echoes of Great Sundering." );
  aoe->add_action( "earthquake,if=(buff.stormkeeper.up&spell_targets.chain_lightning>=6|buff.tempest.up)&talent.surge_of_power.enabled&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering.enabled)", "Spend to buff your follow-up Stormkeeper with Surge of Power on 6+ targets. Respect Echoes of Great Sundering." );
  aoe->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=talent.echoes_of_great_sundering.enabled&!buff.echoes_of_great_sundering_eb.up&(lightning_rod=0|maelstrom>variable.mael_cap-30|(buff.stormkeeper.up&spell_targets.chain_lightning>=6|buff.tempest.up)&talent.surge_of_power.enabled)", "Use the talents you selected. Spread Lightning Rod to as many targets as possible." );
  aoe->add_action( "earth_shock,target_if=min:debuff.lightning_rod.remains,if=talent.echoes_of_great_sundering.enabled&!buff.echoes_of_great_sundering_es.up&(lightning_rod=0|maelstrom>variable.mael_cap-30|(buff.stormkeeper.up&spell_targets.chain_lightning>=6|buff.tempest.up)&talent.surge_of_power.enabled)" );
  aoe->add_action( "icefury,if=talent.fusion_of_elements.enabled&!(buff.fusion_of_elements_nature.up|buff.fusion_of_elements_fire.up)&(cooldown.primordial_wave.remains<5|((spell_targets.chain_lightning=2|(!spell_targets.chain_lightning>=5&talent.echoes_of_great_sundering.enabled&!buff.echoes_of_great_sundering_eb.up))&talent.elemental_blast.enabled))", "Use Icefury for Fusion of Elements proc." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=talent.master_of_the_elements.enabled&!buff.master_of_the_elements.up&!buff.ascendance.up&talent.fire_elemental.enabled", "*{Fire} Proc Master of the Elements outside Ascendance." );
  aoe->add_action( "chain_lightning" );
  aoe->add_action( "flame_shock,moving=1,target_if=refreshable" );
  aoe->add_action( "frost_shock,moving=1" );

  single_target->add_action( "fire_elemental" );
  single_target->add_action( "storm_elemental" );
  single_target->add_action( "stormkeeper", "Just use Stormkeeper." );
  single_target->add_action( "primordial_wave,if=!buff.surge_of_power.up", "Use Primordial Wave as much as possible." );
  single_target->add_action( "ancestral_swiftness" );
  single_target->add_action( "ascendance,if=(time<10|buff.spymasters_web.up|!(variable.spymaster_in_1st|variable.spymaster_in_2nd))&(buff.stormkeeper.up&cooldown.stormkeeper.remains>40|!talent.fury_of_the_storms)&(buff.primordial_wave.up|!talent.primordial_wave)" );
  single_target->add_action( "tempest,if=buff.surge_of_power.up", "Surge of Power is strong and should be used.©" );
  single_target->add_action( "lightning_bolt,if=buff.surge_of_power.up" );
  single_target->add_action( "tempest,if=buff.storm_frenzy.stack=2&!talent.surge_of_power.enabled", "Dont waste Storm Frenzy (minimal gain)." );
  single_target->add_action( "lightning_bolt,if=buff.storm_frenzy.stack=2&!talent.surge_of_power.enabled" );
  single_target->add_action( "liquid_magma_totem,if=dot.flame_shock.refreshable&!buff.master_of_the_elements.up&cooldown.primordial_wave.remains>dot.flame_shock.remains+3", "Use LMT to apply Flame Shock." );
  single_target->add_action( "flame_shock,if=dot.flame_shock.refreshable&!buff.surge_of_power.up&!buff.master_of_the_elements.up&!talent.primordial_wave&!talent.liquid_magma_totem", "Manually refresh Flame shock if better options are not talented." );
  single_target->add_action( "flame_shock,if=dot.flame_shock.refreshable&!buff.surge_of_power.up&!buff.master_of_the_elements.up&talent.master_of_the_elements&talent.erupting_lava&(cooldown.primordial_wave.remains>dot.flame_shock.remains|!talent.primordial_wave)&(cooldown.liquid_magma_totem.remains>dot.flame_shock.remains|!talent.liquid_magma_totem)", "Maintain Flame shock if talented into both Erupting Lava and Master of the elements." );
  single_target->add_action( "earthquake,if=(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up)&(maelstrom>variable.mael_cap-15|buff.master_of_the_elements.up|buff.ascendance.up&buff.ascendance.remains<3|fight_remains<5)", "Spend if close to overcaping, MotE buff is up or Ascendance is about to expire. Respect Echoes of Great Sundering." );
  single_target->add_action( "elemental_blast,if=maelstrom>variable.mael_cap-15|buff.master_of_the_elements.up|buff.ascendance.up&buff.ascendance.remains<3|fight_remains<5" );
  single_target->add_action( "earth_shock,if=maelstrom>variable.mael_cap-15|buff.master_of_the_elements.up|buff.ascendance.up&buff.ascendance.remains<3|fight_remains<5" );
  single_target->add_action( "icefury,if=!(buff.fusion_of_elements_nature.up|buff.fusion_of_elements_fire.up)", "Use Icefury to proc Fusion of Elements." );
  single_target->add_action( "lava_burst,target_if=dot.flame_shock.remains>=execute_time,if=!buff.master_of_the_elements.up&(!talent.master_of_the_elements|buff.lava_surge.up|cooldown.lava_burst.charges_fractional>1.5|maelstrom>82-10*talent.eye_of_the_storm|maelstrom>52-5*talent.eye_of_the_storm&(buff.echoes_of_great_sundering_eb.up|!talent.elemental_blast))", "Use Lava Burst to proc Master of the Elements." );
  single_target->add_action( "earthquake,if=(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up)&(buff.tempest.up|buff.stormkeeper.up)&talent.surge_of_power&!talent.master_of_the_elements", "Spend to activate Surge of Power buff for Tempest or Stormkeeper." );
  single_target->add_action( "elemental_blast,if=(buff.tempest.up|buff.stormkeeper.up)&talent.surge_of_power&!talent.master_of_the_elements" );
  single_target->add_action( "earth_shock,if=(buff.tempest.up|buff.stormkeeper.up)&talent.surge_of_power&!talent.master_of_the_elements" );
  single_target->add_action( "tempest" );
  single_target->add_action( "frost_shock,if=buff.icefury_dmg.up&!buff.ascendance.up&!buff.stormkeeper.up", "Use Icefury-empowered Frost Shocks outside of Ascendance (neutral to minimal gain)." );
  single_target->add_action( "lightning_bolt", "Filler spell. Always available. Always the bottom line." );
  single_target->add_action( "flame_shock,moving=1,target_if=refreshable" );
  single_target->add_action( "flame_shock,moving=1,if=movement.distance>6" );
  single_target->add_action( "frost_shock,moving=1", "Frost Shock is our movement filler." );
}
//elemental_apl_end

//elemental_ptr_apl_start
void elemental_ptr( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* single_target = p->get_action_priority_list( "single_target" );

  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "flametongue_weapon,if=talent.improved_flametongue_weapon.enabled", "Ensure weapon enchant is applied if you've selected Improved Flametongue Weapon." );
  precombat->add_action( "lightning_shield" );
  precombat->add_action( "thunderstrike_ward" );
  precombat->add_action( "variable,name=mael_cap,value=100+50*talent.swelling_maelstrom.enabled+25*talent.primordial_capacity.enabled,op=set" );
  precombat->add_action( "variable,name=spymaster_in_1st,value=trinket.1.is.spymasters_web" );
  precombat->add_action( "variable,name=spymaster_in_2nd,value=trinket.2.is.spymasters_web" );
  precombat->add_action( "stormkeeper" );

  default_->add_action( "spiritwalkers_grace,moving=1,if=movement.distance>6", "Enable more movement." );
  default_->add_action( "wind_shear", "Interrupt of casts." );
  default_->add_action( "blood_fury,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50" );
  default_->add_action( "berserking,if=!talent.ascendance.enabled|buff.ascendance.up" );
  default_->add_action( "fireblood,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50" );
  default_->add_action( "ancestral_call,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50" );
  default_->add_action( "use_item,slot=trinket1,if=!variable.spymaster_in_1st|(fight_remains>180-60*talent.first_ascendant&(buff.spymasters_report.stack>25|buff.spymasters_report.stack<5)|buff.spymasters_report.stack>35|fight_remains<80)&cooldown.ascendance.ready&(buff.stormkeeper.up&cooldown.stormkeeper.remains>40|!talent.fury_of_the_storms)&(buff.primordial_wave.up|!talent.primordial_wave)|fight_remains<21" );
  default_->add_action( "use_item,slot=trinket2,if=!variable.spymaster_in_2nd|(fight_remains>180-60*talent.first_ascendant&(buff.spymasters_report.stack>25|buff.spymasters_report.stack<5)|buff.spymasters_report.stack>35|fight_remains<80)&cooldown.ascendance.ready&(buff.stormkeeper.up&cooldown.stormkeeper.remains>40|!talent.fury_of_the_storms)&(buff.primordial_wave.up|!talent.primordial_wave)|fight_remains<21" );
  default_->add_action( "use_item,slot=main_hand" );
  default_->add_action( "lightning_shield,if=buff.lightning_shield.down" );
  default_->add_action( "natures_swiftness" );
  default_->add_action( "invoke_external_buff,name=power_infusion", "Use Power Infusion on Cooldown." );
  default_->add_action( "potion,if=buff.bloodlust.up|buff.spymasters_web.up|buff.ascendance.remains>12|fight_remains<31" );
  default_->add_action( "run_action_list,name=aoe,if=spell_targets.chain_lightning>=2" );
  default_->add_action( "run_action_list,name=single_target" );

  aoe->add_action( "fire_elemental" );
  aoe->add_action( "storm_elemental" );
  aoe->add_action( "stormkeeper" );
  aoe->add_action( "totemic_recall,if=cooldown.liquid_magma_totem.remains>15&(active_dot.flame_shock<(spell_targets.chain_lightning>?6)-2|talent.fire_elemental.enabled)", "Reset LMT CD as early as possible. Always for Fire, only if you can dot up 3 more targets for Lightning." );
  aoe->add_action( "liquid_magma_totem,if=cooldown.ascendance.remains>15&!buff.ascendance.up" );
  aoe->add_action( "primordial_wave,target_if=min:dot.flame_shock.remains,if=buff.surge_of_power.up|!talent.surge_of_power.enabled|maelstrom<60-5*talent.eye_of_the_storm.enabled", "Spread Flame Shock via Primordial Wave using Surge of Power if possible." );
  aoe->add_action( "ancestral_swiftness" );
  aoe->add_action( "ascendance", "JUST DO IT! https://i.kym-cdn.com/entries/icons/mobile/000/018/147/Shia_LaBeouf__Just_Do_It__Motivational_Speech_(Original_Video_by_LaBeouf__R%C3%B6nkk%C3%B6___Turner)_0-4_screenshot.jpg" );
  aoe->add_action( "flame_shock,target_if=refreshable,if=buff.surge_of_power.up&dot.flame_shock.remains<target.time_to_die-16&active_dot.flame_shock<(spell_targets.chain_lightning>?6)&!talent.liquid_magma_totem.enabled", "Spread Flame Shock using Surge of Power if LMT is not picked." );
  aoe->add_action( "flame_shock,target_if=refreshable,if=talent.fire_elemental.enabled&(buff.surge_of_power.up|!talent.surge_of_power.enabled)&dot.flame_shock.remains<target.time_to_die-5&(active_dot.flame_shock<6|dot.flame_shock.remains>0)", "Spread and refresh Flame Shock using Surge of Power (if talented) up to 6." );
  aoe->add_action( "tempest,target_if=min:debuff.lightning_rod.remains,if=!buff.arc_discharge.up&(buff.surge_of_power.up|!talent.surge_of_power.enabled)", "Buff Tempest with SoP or LB if two target" );
  aoe->add_action( "lightning_bolt,if=buff.stormkeeper.up&buff.surge_of_power.up&spell_targets.chain_lightning=2" );
  aoe->add_action( "chain_lightning,if=active_enemies>=6&buff.surge_of_power.up", "Against 6 targets or more Surge of Power should be used with Chain Lightning rather than Lava Burst." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=buff.primordial_wave.up&(buff.stormkeeper.up&spell_targets.chain_lightning>=6|buff.tempest.up)&maelstrom<60-5*talent.eye_of_the_storm.enabled&talent.surge_of_power.enabled", "Consume Primordial Wave buff immediately if you have Stormkeeper buff, fighting 6+ enemies and need maelstrom to spend." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=buff.primordial_wave.up", "Cast Lava burst to consume Primordial Wave proc. Wait for Lava Surge proc if possible." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains,if=cooldown_react&buff.lava_surge.up&!buff.master_of_the_elements.up&talent.master_of_the_elements.enabled&talent.fire_elemental.enabled", "*{Fire} Use Lava Surge proc to buff <anything> with Master of the Elements." );
  aoe->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=spell_targets.chain_lightning=2&(maelstrom>variable.mael_cap-30|cooldown.primordial_wave.remains<gcd&talent.surge_of_power.enabled|(buff.stormkeeper.up&spell_targets.chain_lightning>=6|buff.tempest.up)&talent.surge_of_power.enabled)", "Two Target" );
  aoe->add_action( "earthquake,if=cooldown.primordial_wave.remains<gcd&talent.surge_of_power.enabled&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering.enabled)", "Activate Surge of Power if next global is Primordial wave. Respect Echoes of Great Sundering." );
  aoe->add_action( "earthquake,if=(lightning_rod=0&talent.lightning_rod.enabled|maelstrom>variable.mael_cap-30)&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering.enabled)", "Spend if all Lightning Rods ran out or you are close to overcaping. Respect Echoes of Great Sundering." );
  aoe->add_action( "earthquake,if=(buff.stormkeeper.up&spell_targets.chain_lightning>=6|buff.tempest.up)&talent.surge_of_power.enabled&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering.enabled)", "Spend to buff your follow-up Stormkeeper with Surge of Power on 6+ targets. Respect Echoes of Great Sundering." );
  aoe->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=talent.echoes_of_great_sundering.enabled&!buff.echoes_of_great_sundering_eb.up&(lightning_rod=0|maelstrom>variable.mael_cap-30|(buff.stormkeeper.up&spell_targets.chain_lightning>=6|buff.tempest.up)&talent.surge_of_power.enabled)", "Use the talents you selected. Spread Lightning Rod to as many targets as possible." );
  aoe->add_action( "earth_shock,target_if=min:debuff.lightning_rod.remains,if=talent.echoes_of_great_sundering.enabled&!buff.echoes_of_great_sundering_es.up&(lightning_rod=0|maelstrom>variable.mael_cap-30|(buff.stormkeeper.up&spell_targets.chain_lightning>=6|buff.tempest.up)&talent.surge_of_power.enabled)" );
  aoe->add_action( "icefury,if=talent.fusion_of_elements.enabled&!(buff.fusion_of_elements_nature.up|buff.fusion_of_elements_fire.up)&(cooldown.primordial_wave.remains<5|((spell_targets.chain_lightning=2|(!spell_targets.chain_lightning>=5&talent.echoes_of_great_sundering.enabled&!buff.echoes_of_great_sundering_eb.up))&talent.elemental_blast.enabled))", "Use Icefury for Fusion of Elements proc." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=talent.master_of_the_elements.enabled&!buff.master_of_the_elements.up&!buff.ascendance.up&talent.fire_elemental.enabled", "*{Fire} Proc Master of the Elements outside Ascendance." );
  aoe->add_action( "chain_lightning" );
  aoe->add_action( "flame_shock,moving=1,target_if=refreshable" );
  aoe->add_action( "frost_shock,moving=1" );

  single_target->add_action( "fire_elemental" );
  single_target->add_action( "storm_elemental" );
  single_target->add_action( "stormkeeper", "Just use Stormkeeper." );
  single_target->add_action( "primordial_wave,if=!buff.surge_of_power.up", "Use Primordial Wave as much as possible." );
  single_target->add_action( "ancestral_swiftness" );
  single_target->add_action( "ascendance,if=(time<10|buff.spymasters_web.up|!(variable.spymaster_in_1st|variable.spymaster_in_2nd))&(buff.stormkeeper.up&cooldown.stormkeeper.remains>40|!talent.fury_of_the_storms)&(buff.primordial_wave.up|!talent.primordial_wave)" );
  single_target->add_action( "tempest,if=buff.surge_of_power.up", "Surge of Power is strong and should be used.©" );
  single_target->add_action( "lightning_bolt,if=buff.surge_of_power.up" );
  single_target->add_action( "tempest,if=buff.storm_frenzy.stack=2&!talent.surge_of_power.enabled", "Dont waste Storm Frenzy (minimal gain)." );
  single_target->add_action( "lightning_bolt,if=buff.storm_frenzy.stack=2&!talent.surge_of_power.enabled" );
  single_target->add_action( "liquid_magma_totem,if=dot.flame_shock.refreshable&!buff.master_of_the_elements.up&cooldown.primordial_wave.remains>dot.flame_shock.remains+3", "Use LMT to apply Flame Shock." );
  single_target->add_action( "flame_shock,if=dot.flame_shock.refreshable&!buff.surge_of_power.up&!buff.master_of_the_elements.up&!talent.primordial_wave&!talent.liquid_magma_totem", "Manually refresh Flame shock if better options are not talented." );
  single_target->add_action( "flame_shock,if=dot.flame_shock.refreshable&!buff.surge_of_power.up&!buff.master_of_the_elements.up&talent.master_of_the_elements&talent.erupting_lava&(cooldown.primordial_wave.remains>dot.flame_shock.remains|!talent.primordial_wave)&(cooldown.liquid_magma_totem.remains>dot.flame_shock.remains|!talent.liquid_magma_totem)", "Maintain Flame shock if talented into both Erupting Lava and Master of the elements." );
  single_target->add_action( "earthquake,if=(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up)&(maelstrom>variable.mael_cap-15|buff.master_of_the_elements.up|buff.ascendance.up&buff.ascendance.remains<3|fight_remains<5)", "Spend if close to overcaping, MotE buff is up or Ascendance is about to expire. Respect Echoes of Great Sundering." );
  single_target->add_action( "elemental_blast,if=maelstrom>variable.mael_cap-15|buff.master_of_the_elements.up|buff.ascendance.up&buff.ascendance.remains<3|fight_remains<5" );
  single_target->add_action( "earth_shock,if=maelstrom>variable.mael_cap-15|buff.master_of_the_elements.up|buff.ascendance.up&buff.ascendance.remains<3|fight_remains<5" );
  single_target->add_action( "icefury,if=!(buff.fusion_of_elements_nature.up|buff.fusion_of_elements_fire.up)", "Use Icefury to proc Fusion of Elements." );
  single_target->add_action( "lava_burst,target_if=dot.flame_shock.remains>=execute_time,if=!buff.master_of_the_elements.up&(!talent.master_of_the_elements|buff.lava_surge.up|cooldown.lava_burst.charges_fractional>1.5|maelstrom>82-10*talent.eye_of_the_storm|maelstrom>52-5*talent.eye_of_the_storm&(buff.echoes_of_great_sundering_eb.up|!talent.elemental_blast))", "Use Lava Burst to proc Master of the Elements." );
  single_target->add_action( "earthquake,if=(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up)&(buff.tempest.up|buff.stormkeeper.up)&talent.surge_of_power&!talent.master_of_the_elements", "Spend to activate Surge of Power buff for Tempest or Stormkeeper." );
  single_target->add_action( "elemental_blast,if=(buff.tempest.up|buff.stormkeeper.up)&talent.surge_of_power&!talent.master_of_the_elements" );
  single_target->add_action( "earth_shock,if=(buff.tempest.up|buff.stormkeeper.up)&talent.surge_of_power&!talent.master_of_the_elements" );
  single_target->add_action( "tempest" );
  single_target->add_action( "frost_shock,if=buff.icefury_dmg.up&!buff.ascendance.up&!buff.stormkeeper.up", "Use Icefury-empowered Frost Shocks outside of Ascendance (neutral to minimal gain)." );
  single_target->add_action( "lightning_bolt", "Filler spell. Always available. Always the bottom line." );
  single_target->add_action( "flame_shock,moving=1,target_if=refreshable" );
  single_target->add_action( "flame_shock,moving=1,if=movement.distance>6" );
  single_target->add_action( "frost_shock,moving=1", "Frost Shock is our movement filler." );
}
//elemental_ptr_apl_end

} //namespace shaman_apl
