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
  default_->add_action( "use_item,slot=trinket1,if=!variable.spymaster_in_1st|(fight_remains>180-60*talent.first_ascendant&(buff.spymasters_report.stack>25)|buff.spymasters_report.stack>35|fight_remains<80)&cooldown.ascendance.ready&(buff.fury_of_storms.up|!talent.fury_of_the_storms)&(buff.primordial_wave.up|!talent.primordial_wave)|fight_remains<21" );
  default_->add_action( "use_item,slot=trinket2,if=!variable.spymaster_in_2nd|(fight_remains>180-60*talent.first_ascendant&(buff.spymasters_report.stack>25)|buff.spymasters_report.stack>35|fight_remains<80)&cooldown.ascendance.ready&(buff.fury_of_storms.up|!talent.fury_of_the_storms)&(buff.primordial_wave.up|!talent.primordial_wave)|fight_remains<21" );
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
  aoe->add_action( "primordial_wave,target_if=min:dot.flame_shock.remains,if=buff.surge_of_power.up|!talent.surge_of_power|maelstrom<60-5*talent.eye_of_the_storm", "Spread Flame Shock via Primordial Wave using Surge of Power if possible." );
  aoe->add_action( "ancestral_swiftness" );
  aoe->add_action( "liquid_magma_totem,if=buff.primordial_wave.up&buff.call_of_the_ancestors.up&!buff.ascendance.up" );
  aoe->add_action( "ascendance,if=(time<10|buff.spymasters_web.up|talent.first_ascendant|!(variable.spymaster_in_1st|variable.spymaster_in_2nd))&(buff.fury_of_storms.up|!talent.fury_of_the_storms)&(buff.primordial_wave.up|!talent.primordial_wave)", "JUST DO IT! https://i.kym-cdn.com/entries/icons/mobile/000/018/147/Shia_LaBeouf__Just_Do_It__Motivational_Speech_(Original_Video_by_LaBeouf__R%C3%B6nkk%C3%B6___Turner)_0-4_screenshot.jpg" );
  aoe->add_action( "liquid_magma_totem,if=buff.primordial_wave.up&(active_dot.flame_shock<=active_enemies-3|active_dot.flame_shock<(active_enemies>?3))", "Add more Flame shocks for better Pwave value." );
  aoe->add_action( "flame_shock,target_if=min:dot.flame_shock.remains,if=buff.primordial_wave.up&active_dot.flame_shock<2&spell_targets.chain_lightning<=3" );
  aoe->add_action( "tempest,target_if=min:debuff.lightning_rod.remains,if=!buff.arc_discharge.up&(buff.surge_of_power.up|!talent.surge_of_power)", "Surge of Power is strong and should be used. ©" );
  aoe->add_action( "lightning_bolt,if=buff.stormkeeper.up&buff.surge_of_power.up&spell_targets.chain_lightning=2" );
  aoe->add_action( "chain_lightning,if=active_enemies>=6&buff.surge_of_power.up" );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains,if=buff.primordial_wave.up&(maelstrom<variable.mael_cap-10*(active_dot.flame_shock+1)|buff.primordial_wave.remains<4)", "Cast Lava burst to consume Primordial Wave proc." );
  aoe->add_action( "chain_lightning,if=buff.storm_frenzy.stack=2&!talent.surge_of_power&maelstrom<variable.mael_cap-(15+buff.stormkeeper.up*spell_targets.chain_lightning*spell_targets.chain_lightning)" );
  aoe->add_action( "earthquake,if=cooldown.primordial_wave.remains<gcd&talent.surge_of_power&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering&(!talent.elemental_blast|active_enemies>1+talent.tempest))", "Activate Surge of Power if next global is Primordial wave. Respect Echoes of Great Sundering." );
  aoe->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=cooldown.primordial_wave.remains<gcd&talent.surge_of_power&(active_enemies<=1+talent.tempest|talent.echoes_of_great_sundering&!buff.echoes_of_great_sundering_eb.up)" );
  aoe->add_action( "earth_shock,target_if=min:debuff.lightning_rod.remains,if=cooldown.primordial_wave.remains<gcd&talent.surge_of_power&talent.echoes_of_great_sundering&!buff.echoes_of_great_sundering_es.up" );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains,if=cooldown_react&buff.lava_surge.up&buff.fusion_of_elements_fire.up&!buff.master_of_the_elements.up&(maelstrom>52-5*talent.eye_of_the_storm&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering))", "Use Lava Surge procs to consume fire part of fusion if you can also buff Earthquake with it." );
  aoe->add_action( "earthquake,if=(maelstrom>variable.mael_cap-10*(spell_targets.chain_lightning+1)|buff.master_of_the_elements.up|buff.ascendance.up&buff.ascendance.remains<3|fight_remains<5)&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering&(!talent.elemental_blast|active_enemies>1+talent.tempest))", "Spend if you are close to cap, Master of the Elements buff is up or Ascendance is about to expire." );
  aoe->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=(maelstrom>variable.mael_cap-10*(spell_targets.chain_lightning+1)|buff.master_of_the_elements.up|buff.ascendance.up&buff.ascendance.remains<3|fight_remains<5)&(active_enemies<=1+talent.tempest|talent.echoes_of_great_sundering&!buff.echoes_of_great_sundering_eb.up)" );
  aoe->add_action( "earth_shock,target_if=min:debuff.lightning_rod.remains,if=(maelstrom>variable.mael_cap-10*(spell_targets.chain_lightning+1)|buff.master_of_the_elements.up|buff.ascendance.up&buff.ascendance.remains<3|fight_remains<5)&talent.echoes_of_great_sundering&!buff.echoes_of_great_sundering_es.up" );
  aoe->add_action( "earthquake,if=talent.lightning_rod&lightning_rod<active_enemies&(buff.stormkeeper.up|buff.tempest.up|!talent.surge_of_power)&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering&(!talent.elemental_blast|active_enemies>1+talent.tempest))", "Spend to spread Lightning Rod if Tempest or Stormkeeper is up." );
  aoe->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=talent.lightning_rod&lightning_rod<active_enemies&(buff.stormkeeper.up|buff.tempest.up|!talent.surge_of_power)&(active_enemies<=1+talent.tempest|talent.echoes_of_great_sundering&!buff.echoes_of_great_sundering_eb.up)" );
  aoe->add_action( "earth_shock,target_if=min:debuff.lightning_rod.remains,if=talent.lightning_rod&lightning_rod<active_enemies&(buff.stormkeeper.up|buff.tempest.up|!talent.surge_of_power)&talent.echoes_of_great_sundering&!buff.echoes_of_great_sundering_es.up" );
  aoe->add_action( "icefury,if=talent.fusion_of_elements&!(buff.fusion_of_elements_nature.up|buff.fusion_of_elements_fire.up)&(active_enemies<=4|!talent.elemental_blast|!talent.echoes_of_great_sundering)", "Use Icefury to proc Fusion of Elements." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains,if=cooldown_react&buff.lava_surge.up&!buff.master_of_the_elements.up&talent.master_of_the_elements&active_enemies<=3", "[2-3t] Use Lava Surge procs to buff <anything> with MotE on 2-3 targets." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains>execute_time,if=!buff.master_of_the_elements.up&talent.master_of_the_elements&(buff.stormkeeper.up|buff.tempest.up|maelstrom>82-10*talent.eye_of_the_storm|maelstrom>52-5*talent.eye_of_the_storm&(buff.echoes_of_great_sundering_eb.up|!talent.elemental_blast))&active_enemies<=3&!talent.lightning_rod&talent.call_of_the_ancestors", "[2-3t]{Farseer} Use all Lava bursts to buff spenders, SK_CL and Tempest with MotE on 2-3 targets if not talented into Lightning Rod." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains>execute_time,if=!buff.master_of_the_elements.up&active_enemies=2", "[2t] Use all Lava bursts to buff <anything> with MotE on 2 targets." );
  aoe->add_action( "flame_shock,target_if=min:debuff.lightning_rod.remains,if=active_dot.flame_shock=0&buff.fusion_of_elements_fire.up&!talent.primordial_wave" );
  aoe->add_action( "earthquake,if=((buff.stormkeeper.up&spell_targets.chain_lightning>=6|buff.tempest.up)&talent.surge_of_power)&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering&(!talent.elemental_blast|active_enemies>1+talent.tempest))", "Spend to buff SK_CL (on 6+) or Tempest with SoP." );
  aoe->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=((buff.stormkeeper.up&active_enemies>=6|buff.tempest.up)&talent.surge_of_power)&(active_enemies<=1+talent.tempest|talent.echoes_of_great_sundering&!buff.echoes_of_great_sundering_eb.up)" );
  aoe->add_action( "earth_shock,target_if=min:debuff.lightning_rod.remains,if=((buff.stormkeeper.up&active_enemies>=6|buff.tempest.up)&talent.surge_of_power)&talent.echoes_of_great_sundering&!buff.echoes_of_great_sundering_es.up" );
  aoe->add_action( "frost_shock,if=buff.icefury_dmg.up&!buff.ascendance.up&!buff.stormkeeper.up&(active_enemies=2|talent.call_of_the_ancestors)" );
  aoe->add_action( "chain_lightning" );
  aoe->add_action( "flame_shock,moving=1,target_if=refreshable" );
  aoe->add_action( "frost_shock,moving=1" );

  single_target->add_action( "fire_elemental" );
  single_target->add_action( "storm_elemental" );
  single_target->add_action( "stormkeeper", "Just use Stormkeeper." );
  single_target->add_action( "primordial_wave,if=!buff.surge_of_power.up", "Use Primordial Wave as much as possible." );
  single_target->add_action( "ancestral_swiftness" );
  single_target->add_action( "ascendance,if=(time<10|buff.spymasters_web.up|talent.first_ascendant|!(variable.spymaster_in_1st|variable.spymaster_in_2nd))&(buff.fury_of_storms.up|!talent.fury_of_the_storms)&(buff.primordial_wave.up|!talent.primordial_wave)" );
  single_target->add_action( "tempest,if=buff.surge_of_power.up", "Surge of Power is strong and should be used.©" );
  single_target->add_action( "lightning_bolt,if=buff.surge_of_power.up" );
  single_target->add_action( "tempest,if=buff.storm_frenzy.stack=2&!talent.surge_of_power.enabled" );
  single_target->add_action( "liquid_magma_totem,if=dot.flame_shock.refreshable&!buff.master_of_the_elements.up&cooldown.primordial_wave.remains>dot.flame_shock.remains+3", "Use LMT to apply Flame Shock." );
  single_target->add_action( "flame_shock,if=dot.flame_shock.refreshable&!buff.surge_of_power.up&!buff.master_of_the_elements.up&!talent.primordial_wave&!talent.liquid_magma_totem", "Manually refresh Flame shock if better options are not talented." );
  single_target->add_action( "flame_shock,if=dot.flame_shock.refreshable&!buff.surge_of_power.up&!buff.master_of_the_elements.up&talent.master_of_the_elements&talent.erupting_lava&(cooldown.primordial_wave.remains>dot.flame_shock.remains|!talent.primordial_wave)&(cooldown.liquid_magma_totem.remains>dot.flame_shock.remains|!talent.liquid_magma_totem)", "Maintain Flame shock if talented into both Erupting Lava and Master of the elements." );
  single_target->add_action( "earthquake,if=(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up)&(maelstrom>variable.mael_cap-15|buff.master_of_the_elements.up|buff.ascendance.up&buff.ascendance.remains<3|fight_remains<5)", "Spend if close to overcaping, MotE buff is up or Ascendance is about to expire. Respect Echoes of Great Sundering." );
  single_target->add_action( "elemental_blast,if=maelstrom>variable.mael_cap-15|buff.master_of_the_elements.up|buff.ascendance.up&buff.ascendance.remains<3|fight_remains<5" );
  single_target->add_action( "earth_shock,if=maelstrom>variable.mael_cap-15|buff.master_of_the_elements.up|buff.ascendance.up&buff.ascendance.remains<3|fight_remains<5" );
  single_target->add_action( "icefury,if=!(buff.fusion_of_elements_nature.up|buff.fusion_of_elements_fire.up)", "Use Icefury to proc Fusion of Elements." );
  single_target->add_action( "lava_burst,target_if=dot.flame_shock.remains>=execute_time,if=!buff.master_of_the_elements.up&(!talent.master_of_the_elements|buff.lava_surge.up|buff.tempest.up|cooldown.lava_burst.charges_fractional>1.8|maelstrom>82-10*talent.eye_of_the_storm|maelstrom>52-5*talent.eye_of_the_storm&(buff.echoes_of_great_sundering_eb.up|!talent.elemental_blast))", "Use Lava Burst to proc Master of the Elements." );
  single_target->add_action( "earthquake,if=(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up)&(buff.tempest.up|buff.stormkeeper.up)&talent.surge_of_power&!talent.master_of_the_elements", "Spend to activate Surge of Power buff for Tempest or Stormkeeper." );
  single_target->add_action( "elemental_blast,if=(buff.tempest.up|buff.stormkeeper.up)&talent.surge_of_power&!talent.master_of_the_elements" );
  single_target->add_action( "earth_shock,if=(buff.tempest.up|buff.stormkeeper.up)&talent.surge_of_power&!talent.master_of_the_elements" );
  single_target->add_action( "tempest" );
  single_target->add_action( "frost_shock,if=buff.icefury_dmg.up&!buff.ascendance.up&!buff.stormkeeper.up", "Use Icefury-empowered Frost Shocks outside of Ascendance." );
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
  default_->add_action( "use_item,slot=trinket1,if=!variable.spymaster_in_1st|(fight_remains>180-60*talent.first_ascendant&(buff.spymasters_report.stack>25)|buff.spymasters_report.stack>35|fight_remains<80)&cooldown.ascendance.ready&(buff.fury_of_storms.up|!talent.fury_of_the_storms)&(buff.primordial_wave.up|!talent.primordial_wave)|fight_remains<21" );
  default_->add_action( "use_item,slot=trinket2,if=!variable.spymaster_in_2nd|(fight_remains>180-60*talent.first_ascendant&(buff.spymasters_report.stack>25)|buff.spymasters_report.stack>35|fight_remains<80)&cooldown.ascendance.ready&(buff.fury_of_storms.up|!talent.fury_of_the_storms)&(buff.primordial_wave.up|!talent.primordial_wave)|fight_remains<21" );
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
  aoe->add_action( "primordial_wave,target_if=min:dot.flame_shock.remains,if=buff.surge_of_power.up|!talent.surge_of_power|maelstrom<60-5*talent.eye_of_the_storm", "Spread Flame Shock via Primordial Wave using Surge of Power if possible." );
  aoe->add_action( "ancestral_swiftness" );
  aoe->add_action( "liquid_magma_totem,if=buff.primordial_wave.up&buff.call_of_the_ancestors.up&!buff.ascendance.up" );
  aoe->add_action( "ascendance,if=(time<10|buff.spymasters_web.up|talent.first_ascendant|!(variable.spymaster_in_1st|variable.spymaster_in_2nd))&(buff.fury_of_storms.up|!talent.fury_of_the_storms)&(buff.primordial_wave.up|!talent.primordial_wave)", "JUST DO IT! https://i.kym-cdn.com/entries/icons/mobile/000/018/147/Shia_LaBeouf__Just_Do_It__Motivational_Speech_(Original_Video_by_LaBeouf__R%C3%B6nkk%C3%B6___Turner)_0-4_screenshot.jpg" );
  aoe->add_action( "liquid_magma_totem,if=buff.primordial_wave.up&(active_dot.flame_shock<=active_enemies-3|active_dot.flame_shock<(active_enemies>?3))", "Add more Flame shocks for better Pwave value." );
  aoe->add_action( "flame_shock,target_if=min:dot.flame_shock.remains,if=buff.primordial_wave.up&active_dot.flame_shock<2&spell_targets.chain_lightning<=3" );
  aoe->add_action( "tempest,target_if=min:debuff.lightning_rod.remains,if=!buff.arc_discharge.up&(buff.surge_of_power.up|!talent.surge_of_power)", "Surge of Power is strong and should be used. ©" );
  aoe->add_action( "lightning_bolt,if=buff.stormkeeper.up&buff.surge_of_power.up&spell_targets.chain_lightning=2" );
  aoe->add_action( "chain_lightning,if=active_enemies>=6&buff.surge_of_power.up" );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains,if=buff.primordial_wave.up&(maelstrom<variable.mael_cap-10*(active_dot.flame_shock+1)|buff.primordial_wave.remains<4)", "Cast Lava burst to consume Primordial Wave proc." );
  aoe->add_action( "chain_lightning,if=buff.storm_frenzy.stack=2&!talent.surge_of_power&maelstrom<variable.mael_cap-(15+buff.stormkeeper.up*spell_targets.chain_lightning*spell_targets.chain_lightning)" );
  aoe->add_action( "earthquake,if=cooldown.primordial_wave.remains<gcd&talent.surge_of_power&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering&(!talent.elemental_blast|active_enemies>1+talent.tempest))", "Activate Surge of Power if next global is Primordial wave. Respect Echoes of Great Sundering." );
  aoe->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=cooldown.primordial_wave.remains<gcd&talent.surge_of_power&(active_enemies<=1+talent.tempest|talent.echoes_of_great_sundering&!buff.echoes_of_great_sundering_eb.up)" );
  aoe->add_action( "earth_shock,target_if=min:debuff.lightning_rod.remains,if=cooldown.primordial_wave.remains<gcd&talent.surge_of_power&talent.echoes_of_great_sundering&!buff.echoes_of_great_sundering_es.up" );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains,if=cooldown_react&buff.lava_surge.up&buff.fusion_of_elements_fire.up&!buff.master_of_the_elements.up&(maelstrom>52-5*talent.eye_of_the_storm&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering))", "Use Lava Surge procs to consume fire part of fusion if you can also buff Earthquake with it." );
  aoe->add_action( "earthquake,if=(maelstrom>variable.mael_cap-10*(spell_targets.chain_lightning+1)|buff.master_of_the_elements.up|buff.ascendance.up&buff.ascendance.remains<3|fight_remains<5)&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering&(!talent.elemental_blast|active_enemies>1+talent.tempest))", "Spend if you are close to cap, Master of the Elements buff is up or Ascendance is about to expire." );
  aoe->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=(maelstrom>variable.mael_cap-10*(spell_targets.chain_lightning+1)|buff.master_of_the_elements.up|buff.ascendance.up&buff.ascendance.remains<3|fight_remains<5)&(active_enemies<=1+talent.tempest|talent.echoes_of_great_sundering&!buff.echoes_of_great_sundering_eb.up)" );
  aoe->add_action( "earth_shock,target_if=min:debuff.lightning_rod.remains,if=(maelstrom>variable.mael_cap-10*(spell_targets.chain_lightning+1)|buff.master_of_the_elements.up|buff.ascendance.up&buff.ascendance.remains<3|fight_remains<5)&talent.echoes_of_great_sundering&!buff.echoes_of_great_sundering_es.up" );
  aoe->add_action( "earthquake,if=talent.lightning_rod&lightning_rod<active_enemies&(buff.stormkeeper.up|buff.tempest.up|!talent.surge_of_power)&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering&(!talent.elemental_blast|active_enemies>1+talent.tempest))", "Spend to spread Lightning Rod if Tempest or Stormkeeper is up." );
  aoe->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=talent.lightning_rod&lightning_rod<active_enemies&(buff.stormkeeper.up|buff.tempest.up|!talent.surge_of_power)&(active_enemies<=1+talent.tempest|talent.echoes_of_great_sundering&!buff.echoes_of_great_sundering_eb.up)" );
  aoe->add_action( "earth_shock,target_if=min:debuff.lightning_rod.remains,if=talent.lightning_rod&lightning_rod<active_enemies&(buff.stormkeeper.up|buff.tempest.up|!talent.surge_of_power)&talent.echoes_of_great_sundering&!buff.echoes_of_great_sundering_es.up" );
  aoe->add_action( "icefury,if=talent.fusion_of_elements&!(buff.fusion_of_elements_nature.up|buff.fusion_of_elements_fire.up)&(active_enemies<=4|!talent.elemental_blast|!talent.echoes_of_great_sundering)", "Use Icefury to proc Fusion of Elements." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains,if=cooldown_react&buff.lava_surge.up&!buff.master_of_the_elements.up&talent.master_of_the_elements&active_enemies<=3", "[2-3t] Use Lava Surge procs to buff <anything> with MotE on 2-3 targets." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains>execute_time,if=!buff.master_of_the_elements.up&talent.master_of_the_elements&(buff.stormkeeper.up|buff.tempest.up|maelstrom>82-10*talent.eye_of_the_storm|maelstrom>52-5*talent.eye_of_the_storm&(buff.echoes_of_great_sundering_eb.up|!talent.elemental_blast))&active_enemies<=3&!talent.lightning_rod&talent.call_of_the_ancestors", "[2-3t]{Farseer} Use all Lava bursts to buff spenders, SK_CL and Tempest with MotE on 2-3 targets if not talented into Lightning Rod." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains>execute_time,if=!buff.master_of_the_elements.up&active_enemies=2", "[2t] Use all Lava bursts to buff <anything> with MotE on 2 targets." );
  aoe->add_action( "flame_shock,target_if=min:debuff.lightning_rod.remains,if=active_dot.flame_shock=0&buff.fusion_of_elements_fire.up&!talent.primordial_wave" );
  aoe->add_action( "earthquake,if=((buff.stormkeeper.up&spell_targets.chain_lightning>=6|buff.tempest.up)&talent.surge_of_power)&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering&(!talent.elemental_blast|active_enemies>1+talent.tempest))", "Spend to buff SK_CL (on 6+) or Tempest with SoP." );
  aoe->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=((buff.stormkeeper.up&active_enemies>=6|buff.tempest.up)&talent.surge_of_power)&(active_enemies<=1+talent.tempest|talent.echoes_of_great_sundering&!buff.echoes_of_great_sundering_eb.up)" );
  aoe->add_action( "earth_shock,target_if=min:debuff.lightning_rod.remains,if=((buff.stormkeeper.up&active_enemies>=6|buff.tempest.up)&talent.surge_of_power)&talent.echoes_of_great_sundering&!buff.echoes_of_great_sundering_es.up" );
  aoe->add_action( "frost_shock,if=buff.icefury_dmg.up&!buff.ascendance.up&!buff.stormkeeper.up&(active_enemies=2|talent.call_of_the_ancestors)" );
  aoe->add_action( "chain_lightning" );
  aoe->add_action( "flame_shock,moving=1,target_if=refreshable" );
  aoe->add_action( "frost_shock,moving=1" );

  single_target->add_action( "fire_elemental" );
  single_target->add_action( "storm_elemental" );
  single_target->add_action( "stormkeeper", "Just use Stormkeeper." );
  single_target->add_action( "primordial_wave,if=!buff.surge_of_power.up", "Use Primordial Wave as much as possible." );
  single_target->add_action( "ancestral_swiftness" );
  single_target->add_action( "ascendance,if=(time<10|buff.spymasters_web.up|talent.first_ascendant|!(variable.spymaster_in_1st|variable.spymaster_in_2nd))&(buff.fury_of_storms.up|!talent.fury_of_the_storms)&(buff.primordial_wave.up|!talent.primordial_wave)" );
  single_target->add_action( "tempest,if=buff.surge_of_power.up", "Surge of Power is strong and should be used.©" );
  single_target->add_action( "lightning_bolt,if=buff.surge_of_power.up" );
  single_target->add_action( "tempest,if=buff.storm_frenzy.stack=2&!talent.surge_of_power.enabled" );
  single_target->add_action( "liquid_magma_totem,if=dot.flame_shock.refreshable&!buff.master_of_the_elements.up&cooldown.primordial_wave.remains>dot.flame_shock.remains+3", "Use LMT to apply Flame Shock." );
  single_target->add_action( "flame_shock,if=dot.flame_shock.refreshable&!buff.surge_of_power.up&!buff.master_of_the_elements.up&!talent.primordial_wave&!talent.liquid_magma_totem", "Manually refresh Flame shock if better options are not talented." );
  single_target->add_action( "flame_shock,if=dot.flame_shock.refreshable&!buff.surge_of_power.up&!buff.master_of_the_elements.up&talent.master_of_the_elements&talent.erupting_lava&(cooldown.primordial_wave.remains>dot.flame_shock.remains|!talent.primordial_wave)&(cooldown.liquid_magma_totem.remains>dot.flame_shock.remains|!talent.liquid_magma_totem)", "Maintain Flame shock if talented into both Erupting Lava and Master of the elements." );
  single_target->add_action( "earthquake,if=(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up)&(maelstrom>variable.mael_cap-15|buff.master_of_the_elements.up|buff.ascendance.up&buff.ascendance.remains<3|fight_remains<5)", "Spend if close to overcaping, MotE buff is up or Ascendance is about to expire. Respect Echoes of Great Sundering." );
  single_target->add_action( "elemental_blast,if=maelstrom>variable.mael_cap-15|buff.master_of_the_elements.up|buff.ascendance.up&buff.ascendance.remains<3|fight_remains<5" );
  single_target->add_action( "earth_shock,if=maelstrom>variable.mael_cap-15|buff.master_of_the_elements.up|buff.ascendance.up&buff.ascendance.remains<3|fight_remains<5" );
  single_target->add_action( "icefury,if=!(buff.fusion_of_elements_nature.up|buff.fusion_of_elements_fire.up)", "Use Icefury to proc Fusion of Elements." );
  single_target->add_action( "lava_burst,target_if=dot.flame_shock.remains>=execute_time,if=!buff.master_of_the_elements.up&(!talent.master_of_the_elements|buff.lava_surge.up|buff.tempest.up|cooldown.lava_burst.charges_fractional>1.8|maelstrom>82-10*talent.eye_of_the_storm|maelstrom>52-5*talent.eye_of_the_storm&(buff.echoes_of_great_sundering_eb.up|!talent.elemental_blast))", "Use Lava Burst to proc Master of the Elements." );
  single_target->add_action( "earthquake,if=(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up)&(buff.tempest.up|buff.stormkeeper.up)&talent.surge_of_power&!talent.master_of_the_elements", "Spend to activate Surge of Power buff for Tempest or Stormkeeper." );
  single_target->add_action( "elemental_blast,if=(buff.tempest.up|buff.stormkeeper.up)&talent.surge_of_power&!talent.master_of_the_elements" );
  single_target->add_action( "earth_shock,if=(buff.tempest.up|buff.stormkeeper.up)&talent.surge_of_power&!talent.master_of_the_elements" );
  single_target->add_action( "tempest" );
  single_target->add_action( "frost_shock,if=buff.icefury_dmg.up&!buff.ascendance.up&!buff.stormkeeper.up", "Use Icefury-empowered Frost Shocks outside of Ascendance." );
  single_target->add_action( "lightning_bolt", "Filler spell. Always available. Always the bottom line." );
  single_target->add_action( "flame_shock,moving=1,target_if=refreshable" );
  single_target->add_action( "flame_shock,moving=1,if=movement.distance>6" );
  single_target->add_action( "frost_shock,moving=1", "Frost Shock is our movement filler." );
}
//elemental_ptr_apl_end

} //namespace shaman_apl
