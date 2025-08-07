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
  precombat->add_action( "flametongue_weapon,if=talent.improved_flametongue_weapon", "Ensure weapon enchant is applied if you've selected Improved Flametongue Weapon." );
  precombat->add_action( "lightning_shield" );
  precombat->add_action( "thunderstrike_ward" );
  precombat->add_action( "variable,name=mael_cap,value=100+50*talent.swelling_maelstrom+25*talent.primordial_capacity" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=(trinket.1.has_use_buff|trinket.1.is.funhouse_lens)" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=(trinket.2.has_use_buff|trinket.2.is.funhouse_lens)" );
  precombat->add_action( "stormkeeper" );

  default_->add_action( "spiritwalkers_grace,moving=1,if=movement.distance>6", "Enable more movement." );
  default_->add_action( "wind_shear", "Interrupt of casts." );
  default_->add_action( "blood_fury,if=!talent.ascendance|buff.ascendance.up|cooldown.ascendance.remains>50" );
  default_->add_action( "berserking,if=!talent.ascendance|buff.ascendance.up" );
  default_->add_action( "fireblood,if=!talent.ascendance|buff.ascendance.up|cooldown.ascendance.remains>50" );
  default_->add_action( "ancestral_call,if=!talent.ascendance|buff.ascendance.up|cooldown.ascendance.remains>50" );
  default_->add_action( "use_item,name=neural_synapse_enhancer,use_off_gcd=1,if=buff.ascendance.remains>12|cooldown.ascendance.remains>10", "Neural Synapse Enhancer" );
  default_->add_action( "use_item,slot=trinket1,use_off_gcd=1,if=variable.trinket_1_buffs&((cooldown.primordial_wave.remains>25|!talent.primordial_wave|spell_targets.chain_lightning>=2)&cooldown.ascendance.remains>trinket.1.cooldown.duration-5|buff.ascendance.remains>12|fight_remains<21)", "Normal buff trinkets" );
  default_->add_action( "use_item,slot=trinket2,use_off_gcd=1,if=variable.trinket_2_buffs&((cooldown.primordial_wave.remains>25|!talent.primordial_wave|spell_targets.chain_lightning>=2)&cooldown.ascendance.remains>trinket.2.cooldown.duration-5|buff.ascendance.remains>12|fight_remains<21)" );
  default_->add_action( "use_item,slot=main_hand,use_off_gcd=1,if=(buff.fury_of_storms.up|!talent.fury_of_the_storms|cooldown.stormkeeper.remains>10)&(cooldown.primordial_wave.remains>25|!talent.primordial_wave)&cooldown.ascendance.remains>15|buff.ascendance.remains>12", "Normal weapons" );
  default_->add_action( "use_item,slot=trinket1,use_off_gcd=1,if=!variable.trinket_1_buffs&(cooldown.ascendance.remains>20|trinket.2.cooldown.remains>20&cooldown.neural_synapse_enhancer.remains>20&cooldown.bestinslots.remains>20)", "Dmg trinkets" );
  default_->add_action( "use_item,slot=trinket2,use_off_gcd=1,if=!variable.trinket_2_buffs&(cooldown.ascendance.remains>20|trinket.1.cooldown.remains>20&cooldown.neural_synapse_enhancer.remains>20&cooldown.bestinslots.remains>20)" );
  default_->add_action( "lightning_shield,if=buff.lightning_shield.down" );
  default_->add_action( "natures_swiftness" );
  default_->add_action( "invoke_external_buff,name=power_infusion,if=buff.ascendance.up|cooldown.ascendance.remains>30", "Use Power Infusion on Cooldown." );
  default_->add_action( "potion,if=buff.bloodlust.up|buff.ascendance.remains>12|fight_remains<31" );
  default_->add_action( "run_action_list,name=aoe,if=spell_targets.chain_lightning>=2" );
  default_->add_action( "run_action_list,name=single_target" );

  aoe->add_action( "fire_elemental" );
  aoe->add_action( "storm_elemental,if=(!buff.storm_elemental.up|!talent.echo_of_the_elementals)&!buff.ancestral_wisdom.up" );
  aoe->add_action( "stormkeeper,if=talent.herald_of_the_storms|cooldown.primordial_wave.remains<gcd|!talent.primordial_wave" );
  aoe->add_action( "liquid_magma_totem,if=(cooldown.primordial_wave.remains<5*gcd|!talent.primordial_wave)&(active_dot.flame_shock<=active_enemies-3|active_dot.flame_shock<(active_enemies>?3))", "Spread Flame shocks for Pwave." );
  aoe->add_action( "flame_shock,target_if=min:debuff.lightning_rod.remains,if=cooldown.primordial_wave.remains<gcd&active_dot.flame_shock=0&(talent.primordial_wave|spell_targets.chain_lightning<=3)&cooldown.ascendance.remains>10" );
  aoe->add_action( "primordial_wave,if=active_dot.flame_shock=active_enemies>?6|(cooldown.liquid_magma_totem.remains>15|!talent.liquid_magma_totem)&cooldown.ascendance.remains>15" );
  aoe->add_action( "ancestral_swiftness" );
  aoe->add_action( "ascendance,if=(talent.first_ascendant|fight_remains>200|fight_remains<80|variable.trinket_1_buffs&trinket.1.ready_cooldown|variable.trinket_2_buffs&trinket.2.ready_cooldown|equipped.neural_synapse_enhancer&cooldown.neural_synapse_enhancer.remains=0|equipped.bestinslots&cooldown.bestinslots.remains=0)&(buff.fury_of_storms.up|!talent.fury_of_the_storms)" );
  aoe->add_action( "tempest,target_if=min:debuff.lightning_rod.remains,if=buff.arc_discharge.stack<2&(buff.surge_of_power.up|!talent.surge_of_power)", "Surge of Power is strong and should be used. ©" );
  aoe->add_action( "lightning_bolt,if=buff.stormkeeper.up&buff.surge_of_power.up&spell_targets.chain_lightning=2" );
  aoe->add_action( "chain_lightning,if=active_enemies>=6&buff.surge_of_power.up" );
  aoe->add_action( "lightning_bolt,if=buff.storm_frenzy.stack=2&!talent.surge_of_power&maelstrom<variable.mael_cap-(15+buff.stormkeeper.up*spell_targets.chain_lightning*spell_targets.chain_lightning)&buff.stormkeeper.up&!buff.call_of_the_ancestors.up&spell_targets.chain_lightning=2" );
  aoe->add_action( "chain_lightning,if=buff.storm_frenzy.stack=2&!talent.surge_of_power&maelstrom<variable.mael_cap-(15+buff.stormkeeper.up*spell_targets.chain_lightning*spell_targets.chain_lightning)" );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains,if=cooldown_react&buff.lava_surge.up&buff.fusion_of_elements_fire.up&!buff.master_of_the_elements.up&(maelstrom>52-5*talent.eye_of_the_storm&(buff.echoes_of_great_sundering_es.up|!talent.echoes_of_great_sundering))", "Use Lava Surge procs to consume fire part of fusion if you can also buff Earthquake with it." );
  aoe->add_action( "earthquake,if=(maelstrom>variable.mael_cap-10*(spell_targets.chain_lightning+1)|buff.master_of_the_elements.up|buff.ascendance.up&buff.ascendance.remains<3|fight_remains<5)&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering&(!talent.elemental_blast|active_enemies>1+3*talent.tempest))&(cooldown.primordial_wave.remains>8|!(set_bonus.tww3_4pc&talent.ancestral_swiftness)|maelstrom>variable.mael_cap-20)", "Spend if you are close to cap, Master of the Elements buff is up or Ascendance is about to expire." );
  aoe->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=(maelstrom>variable.mael_cap-10*(spell_targets.chain_lightning+1)|buff.master_of_the_elements.up|buff.ascendance.up&buff.ascendance.remains<3|fight_remains<5)&(cooldown.primordial_wave.remains>8|!(set_bonus.tww3_4pc&talent.ancestral_swiftness)|maelstrom>variable.mael_cap-20)" );
  aoe->add_action( "earth_shock,target_if=min:debuff.lightning_rod.remains,if=(maelstrom>variable.mael_cap-10*(spell_targets.chain_lightning+1)|buff.master_of_the_elements.up|buff.ascendance.up&buff.ascendance.remains<3|fight_remains<5)&(cooldown.primordial_wave.remains>8|!(set_bonus.tww3_4pc&talent.ancestral_swiftness)|maelstrom>variable.mael_cap-20)" );
  aoe->add_action( "earthquake,if=talent.lightning_rod&lightning_rod<active_enemies&(buff.stormkeeper.up|buff.tempest.up|!talent.surge_of_power)&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering&(!talent.elemental_blast|active_enemies>1+3*talent.tempest))", "Spend to spread Lightning Rod if Tempest or Stormkeeper is up." );
  aoe->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=talent.lightning_rod&lightning_rod<active_enemies&(buff.stormkeeper.up|buff.tempest.up|!talent.surge_of_power)" );
  aoe->add_action( "earth_shock,target_if=min:debuff.lightning_rod.remains,if=talent.lightning_rod&lightning_rod<active_enemies&(buff.stormkeeper.up|buff.tempest.up|!talent.surge_of_power)" );
  aoe->add_action( "icefury,if=talent.fusion_of_elements&!(buff.fusion_of_elements_nature.up|buff.fusion_of_elements_fire.up)&(active_enemies<=4|!talent.elemental_blast|!talent.echoes_of_great_sundering)", "Use Icefury to proc Fusion of Elements." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains,if=cooldown_react&buff.lava_surge.up&!buff.master_of_the_elements.up&talent.master_of_the_elements&active_enemies<=3", "[2-3t] Use Lava Surge procs to buff <anything> with MotE on 2-3 targets." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=!buff.master_of_the_elements.up&talent.master_of_the_elements&(buff.stormkeeper.up|buff.tempest.up|maelstrom>82-10*talent.eye_of_the_storm|maelstrom>52-5*talent.eye_of_the_storm&(buff.echoes_of_great_sundering_eb.up|!talent.elemental_blast))&active_enemies<=3&!talent.lightning_rod&talent.call_of_the_ancestors", "[2-3t]{Farseer} Use all Lava bursts to buff spenders, SK_CL and Tempest with MotE on 2-3 targets if not talented into Lightning Rod." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=!buff.master_of_the_elements.up&active_enemies=2", "[2t] Use all Lava bursts to buff <anything> with MotE on 2 targets." );
  aoe->add_action( "flame_shock,target_if=min:debuff.lightning_rod.remains,if=active_dot.flame_shock=0&buff.fusion_of_elements_fire.up&(!talent.elemental_blast|!talent.echoes_of_great_sundering&active_enemies>1+3*talent.tempest)" );
  aoe->add_action( "earthquake,if=((buff.stormkeeper.up&spell_targets.chain_lightning>=6|buff.tempest.up)&talent.surge_of_power)&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering&(!talent.elemental_blast|active_enemies>1+3*talent.tempest))", "Spend to buff SK_CL (on 6+) or Tempest with SoP." );
  aoe->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=(buff.stormkeeper.up&active_enemies>=6|buff.tempest.up)&talent.surge_of_power" );
  aoe->add_action( "earth_shock,target_if=min:debuff.lightning_rod.remains,if=(buff.stormkeeper.up&active_enemies>=6|buff.tempest.up)&talent.surge_of_power" );
  aoe->add_action( "frost_shock,if=buff.icefury_dmg.up&!buff.ascendance.up&!buff.stormkeeper.up&(talent.call_of_the_ancestors|spell_targets.chain_lightning<=3)" );
  aoe->add_action( "chain_lightning" );
  aoe->add_action( "flame_shock,moving=1,target_if=refreshable" );
  aoe->add_action( "frost_shock,moving=1" );

  single_target->add_action( "fire_elemental" );
  single_target->add_action( "storm_elemental,if=(!buff.storm_elemental.up|!talent.echo_of_the_elementals)&!buff.ancestral_wisdom.up" );
  single_target->add_action( "stormkeeper,if=talent.herald_of_the_storms|cooldown.primordial_wave.remains<gcd|!talent.primordial_wave", "Just use Stormkeeper." );
  single_target->add_action( "liquid_magma_totem,if=active_dot.flame_shock=0&!buff.surge_of_power.up&!buff.master_of_the_elements.up&!(set_bonus.tww3_2pc&talent.ancestral_swiftness)", "Apply Flame shock if it is not up. s3 Farseer opts into a bit more complex LMT usage to take advantage of ancestors casting 2 spells." );
  single_target->add_action( "liquid_magma_totem,if=dot.flame_shock.refreshable&!buff.surge_of_power.up&!buff.master_of_the_elements.up&cooldown.ascendance.ready" );
  single_target->add_action( "flame_shock,if=active_dot.flame_shock=0&!buff.surge_of_power.up&!buff.master_of_the_elements.up" );
  single_target->add_action( "primordial_wave", "Use Primordial Wave as much as possible." );
  single_target->add_action( "ancestral_swiftness" );
  single_target->add_action( "ascendance,if=(talent.first_ascendant|fight_remains>200|fight_remains<80|variable.trinket_1_buffs&trinket.1.ready_cooldown|variable.trinket_2_buffs&trinket.2.ready_cooldown|equipped.neural_synapse_enhancer&cooldown.neural_synapse_enhancer.remains=0|equipped.bestinslots&cooldown.bestinslots.remains=0)&(buff.fury_of_storms.up|!talent.fury_of_the_storms)&(cooldown.primordial_wave.remains>25|!talent.primordial_wave)" );
  single_target->add_action( "tempest,if=buff.surge_of_power.up", "Surge of Power is strong and should be used.©" );
  single_target->add_action( "lightning_bolt,if=buff.surge_of_power.up" );
  single_target->add_action( "tempest,if=buff.storm_frenzy.stack=2&!talent.surge_of_power.enabled" );
  single_target->add_action( "liquid_magma_totem,if=dot.flame_shock.refreshable&!buff.master_of_the_elements.up&!talent.call_of_the_ancestors", "Use LMT to benefit from ancestors multicasting." );
  single_target->add_action( "liquid_magma_totem,if=cooldown.primordial_wave.remains>24&!buff.ascendance.up&maelstrom<variable.mael_cap-10&!buff.ancestral_swiftness.up&!buff.master_of_the_elements.up" );
  single_target->add_action( "flame_shock,if=dot.flame_shock.refreshable&!buff.surge_of_power.up&!buff.master_of_the_elements.up&talent.erupting_lava", "Maintain Flame shock if talented into Erupting Lava." );
  single_target->add_action( "elemental_blast,if=maelstrom>variable.mael_cap-15|buff.master_of_the_elements.up|buff.ancestral_wisdom.up&buff.ancestral_wisdom.remains<2", "Spend if close to overcaping or MotE buff is up. Friendship ended with Echoes of Great Sundering." );
  single_target->add_action( "earth_shock,if=maelstrom>variable.mael_cap-15|buff.master_of_the_elements.up|buff.ancestral_wisdom.up&buff.ancestral_wisdom.remains<2" );
  single_target->add_action( "icefury,if=!(buff.fusion_of_elements_nature.up|buff.fusion_of_elements_fire.up)", "Use Icefury to proc Fusion of Elements." );
  single_target->add_action( "lava_burst,target_if=dot.flame_shock.remains>=2,if=!buff.master_of_the_elements.up&(buff.lava_surge.up|buff.tempest.up|buff.stormkeeper.up|cooldown.lava_burst.charges_fractional>1.8|maelstrom>variable.mael_cap-30|(maelstrom>52-5*talent.eye_of_the_storm*(1+talent.elemental_blast)+30*talent.elemental_blast)&(cooldown.primordial_wave.remains>8|!(set_bonus.tww3_4pc&talent.ancestral_swiftness)))", "Use Lava Burst to proc Master of the Elements." );
  single_target->add_action( "earthquake,if=buff.echoes_of_great_sundering_eb.up&(buff.tempest.up|buff.stormkeeper.up)&talent.surge_of_power&!talent.master_of_the_elements", "Spend to activate Surge of Power buff for Tempest or Stormkeeper." );
  single_target->add_action( "elemental_blast,if=(buff.tempest.up|buff.stormkeeper.up)&talent.surge_of_power&!talent.master_of_the_elements" );
  single_target->add_action( "earth_shock,if=(buff.tempest.up|buff.stormkeeper.up)&talent.surge_of_power&!talent.master_of_the_elements" );
  single_target->add_action( "tempest" );
  single_target->add_action( "lightning_bolt,if=buff.storm_elemental.up&buff.wind_gust.stack<4" );
  single_target->add_action( "frost_shock,if=buff.icefury_dmg.up&!buff.ascendance.up&!buff.stormkeeper.up&talent.call_of_the_ancestors", "Use Icefury-empowered Frost Shocks outside of Ascendance." );
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
  precombat->add_action( "flametongue_weapon,if=talent.improved_flametongue_weapon", "Ensure weapon enchant is applied if you've selected Improved Flametongue Weapon." );
  precombat->add_action( "lightning_shield" );
  precombat->add_action( "thunderstrike_ward" );
  precombat->add_action( "variable,name=mael_cap,value=100+50*talent.swelling_maelstrom+25*talent.primordial_capacity" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=(trinket.1.has_use_buff|trinket.1.is.funhouse_lens)" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=(trinket.2.has_use_buff|trinket.2.is.funhouse_lens)" );
  precombat->add_action( "variable,name=special_trinket1,value=(trinket.1.is.house_of_cards|trinket.1.is.funhouse_lens)&!(trinket.2.has_use_buff|trinket.2.is.funhouse_lens)&talent.first_ascendant" );
  precombat->add_action( "variable,name=special_trinket2,value=(trinket.2.is.house_of_cards|trinket.2.is.funhouse_lens)&!(trinket.1.has_use_buff|trinket.1.is.funhouse_lens)&talent.first_ascendant" );
  precombat->add_action( "stormkeeper" );

  default_->add_action( "spiritwalkers_grace,moving=1,if=movement.distance>6", "Enable more movement." );
  default_->add_action( "wind_shear", "Interrupt of casts." );
  default_->add_action( "blood_fury,if=!talent.ascendance|buff.ascendance.up|cooldown.ascendance.remains>50" );
  default_->add_action( "berserking,if=!talent.ascendance|buff.ascendance.up" );
  default_->add_action( "fireblood,if=!talent.ascendance|buff.ascendance.up|cooldown.ascendance.remains>50" );
  default_->add_action( "ancestral_call,if=!talent.ascendance|buff.ascendance.up|cooldown.ascendance.remains>50" );
  default_->add_action( "use_item,name=spymasters_web,if=(fight_remains>180&buff.spymasters_report.stack>25|buff.spymasters_report.stack>35|fight_remains<80)&cooldown.ascendance.ready&(buff.fury_of_storms.up|!talent.fury_of_the_storms)&(cooldown.primordial_wave.remains>25|!talent.primordial_wave|spell_targets.chain_lightning>=2)|buff.ascendance.remains>12&buff.spymasters_report.stack>25|fight_remains<21", "Spymaster's Web" );
  default_->add_action( "use_item,name=spymasters_web,use_off_gcd=1,if=buff.ascendance.remains>12&buff.spymasters_report.stack>25" );
  default_->add_action( "use_item,name=neural_synapse_enhancer,use_off_gcd=1,if=buff.ascendance.remains>12|cooldown.ascendance.remains>10", "Neural Synapse Enhancer" );
  default_->add_action( "use_item,name=house_of_cards,use_off_gcd=1,if=(variable.special_trinket1|variable.special_trinket2)&(buff.ascendance.remains>12|cooldown.ascendance.remains>90)|fight_remains<16", "House of Cards + 2 minute Ascendance" );
  default_->add_action( "use_item,name=funhouse_lens,use_off_gcd=1,if=(variable.special_trinket1|variable.special_trinket2)&(buff.ascendance.remains>12|cooldown.ascendance.remains>90)|fight_remains<16", "Funhouse Lens + 2 minute Ascendance" );
  default_->add_action( "use_item,slot=trinket1,use_off_gcd=1,if=!trinket.1.is.spymasters_web&!variable.special_trinket1&variable.trinket_1_buffs&((cooldown.primordial_wave.remains>25|!talent.primordial_wave|spell_targets.chain_lightning>=2)&(cooldown.ascendance.remains>trinket.1.cooldown.duration-5|buff.spymasters_report.stack>25)|buff.ascendance.remains>12|fight_remains<21)", "Normal buff trinkets" );
  default_->add_action( "use_item,slot=trinket2,use_off_gcd=1,if=!trinket.2.is.spymasters_web&!variable.special_trinket2&variable.trinket_2_buffs&((cooldown.primordial_wave.remains>25|!talent.primordial_wave|spell_targets.chain_lightning>=2)&(cooldown.ascendance.remains>trinket.2.cooldown.duration-5|buff.spymasters_report.stack>25)|buff.ascendance.remains>12|fight_remains<21)" );
  default_->add_action( "use_item,slot=main_hand,use_off_gcd=1,if=(buff.fury_of_storms.up|!talent.fury_of_the_storms|cooldown.stormkeeper.remains>10)&(cooldown.primordial_wave.remains>25|!talent.primordial_wave)&cooldown.ascendance.remains>15|buff.ascendance.remains>12", "Normal weapons" );
  default_->add_action( "use_item,slot=trinket1,use_off_gcd=1,if=!variable.trinket_1_buffs&(cooldown.ascendance.remains>20|trinket.2.cooldown.remains>20&cooldown.neural_synapse_enhancer.remains>20&cooldown.bestinslots.remains>20)", "Dmg trinkets" );
  default_->add_action( "use_item,slot=trinket2,use_off_gcd=1,if=!variable.trinket_2_buffs&(cooldown.ascendance.remains>20|trinket.1.cooldown.remains>20&cooldown.neural_synapse_enhancer.remains>20&cooldown.bestinslots.remains>20)" );
  default_->add_action( "lightning_shield,if=buff.lightning_shield.down" );
  default_->add_action( "natures_swiftness" );
  default_->add_action( "invoke_external_buff,name=power_infusion,if=buff.ascendance.up|cooldown.ascendance.remains>30", "Use Power Infusion on Cooldown." );
  default_->add_action( "potion,if=buff.bloodlust.up|buff.spymasters_web.up|buff.ascendance.remains>12|fight_remains<31" );
  default_->add_action( "run_action_list,name=aoe,if=spell_targets.chain_lightning>=2" );
  default_->add_action( "run_action_list,name=single_target" );

  aoe->add_action( "fire_elemental" );
  aoe->add_action( "storm_elemental,if=!buff.storm_elemental.up|!talent.echo_of_the_elementals" );
  aoe->add_action( "stormkeeper" );
  aoe->add_action( "liquid_magma_totem,if=(cooldown.primordial_wave.remains<5*gcd|!talent.primordial_wave)&(active_dot.flame_shock<=active_enemies-3|active_dot.flame_shock<(active_enemies>?3))", "Spread Flame shocks for Pwave." );
  aoe->add_action( "flame_shock,target_if=min:debuff.lightning_rod.remains,if=cooldown.primordial_wave.remains<gcd&active_dot.flame_shock=0&(talent.primordial_wave|spell_targets.chain_lightning<=3)&cooldown.ascendance.remains>10" );
  aoe->add_action( "primordial_wave,if=active_dot.flame_shock=active_enemies>?6|(cooldown.liquid_magma_totem.remains>15|!talent.liquid_magma_totem)&cooldown.ascendance.remains>15" );
  aoe->add_action( "ancestral_swiftness" );
  aoe->add_action( "ascendance,if=(talent.first_ascendant|fight_remains>200|fight_remains<80|buff.spymasters_web.up|variable.trinket_1_buffs&!trinket.1.is.spymasters_web&trinket.1.ready_cooldown|variable.trinket_2_buffs&!trinket.2.is.spymasters_web&trinket.2.ready_cooldown|equipped.neural_synapse_enhancer&cooldown.neural_synapse_enhancer.remains=0|equipped.bestinslots&cooldown.bestinslots.remains=0)&(buff.fury_of_storms.up|!talent.fury_of_the_storms)" );
  aoe->add_action( "tempest,target_if=min:debuff.lightning_rod.remains,if=buff.arc_discharge.stack<2&(buff.surge_of_power.up|!talent.surge_of_power)", "Surge of Power is strong and should be used. ©" );
  aoe->add_action( "lightning_bolt,if=buff.stormkeeper.up&buff.surge_of_power.up&spell_targets.chain_lightning=2" );
  aoe->add_action( "chain_lightning,if=active_enemies>=6&buff.surge_of_power.up" );
  aoe->add_action( "chain_lightning,if=buff.storm_frenzy.stack=2&!talent.surge_of_power&maelstrom<variable.mael_cap-(15+buff.stormkeeper.up*spell_targets.chain_lightning*spell_targets.chain_lightning)" );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains,if=cooldown_react&buff.lava_surge.up&buff.fusion_of_elements_fire.up&!buff.master_of_the_elements.up&(maelstrom>52-5*talent.eye_of_the_storm&(buff.echoes_of_great_sundering_es.up|!talent.echoes_of_great_sundering))", "Use Lava Surge procs to consume fire part of fusion if you can also buff Earthquake with it." );
  aoe->add_action( "earthquake,if=(maelstrom>variable.mael_cap-10*(spell_targets.chain_lightning+1)|buff.master_of_the_elements.up|buff.ascendance.up&buff.ascendance.remains<3|fight_remains<5)&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering&(!talent.elemental_blast|active_enemies>1+talent.tempest))", "Spend if you are close to cap, Master of the Elements buff is up or Ascendance is about to expire." );
  aoe->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=(maelstrom>variable.mael_cap-10*(spell_targets.chain_lightning+1)|buff.master_of_the_elements.up|buff.ascendance.up&buff.ascendance.remains<3|fight_remains<5)&(active_enemies<=1+talent.tempest|talent.echoes_of_great_sundering&!buff.echoes_of_great_sundering_eb.up)" );
  aoe->add_action( "earth_shock,target_if=min:debuff.lightning_rod.remains,if=(maelstrom>variable.mael_cap-10*(spell_targets.chain_lightning+1)|buff.master_of_the_elements.up|buff.ascendance.up&buff.ascendance.remains<3|fight_remains<5)&talent.echoes_of_great_sundering&!buff.echoes_of_great_sundering_es.up" );
  aoe->add_action( "earthquake,if=talent.lightning_rod&lightning_rod<active_enemies&(buff.stormkeeper.up|buff.tempest.up|!talent.surge_of_power)&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering&(!talent.elemental_blast|active_enemies>1+talent.tempest))", "Spend to spread Lightning Rod if Tempest or Stormkeeper is up." );
  aoe->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=talent.lightning_rod&lightning_rod<active_enemies&(buff.stormkeeper.up|buff.tempest.up|!talent.surge_of_power)&(active_enemies<=1+talent.tempest|talent.echoes_of_great_sundering&!buff.echoes_of_great_sundering_eb.up)" );
  aoe->add_action( "earth_shock,target_if=min:debuff.lightning_rod.remains,if=talent.lightning_rod&lightning_rod<active_enemies&(buff.stormkeeper.up|buff.tempest.up|!talent.surge_of_power)&talent.echoes_of_great_sundering&!buff.echoes_of_great_sundering_es.up" );
  aoe->add_action( "icefury,if=talent.fusion_of_elements&!(buff.fusion_of_elements_nature.up|buff.fusion_of_elements_fire.up)&(active_enemies<=4|!talent.elemental_blast|!talent.echoes_of_great_sundering)", "Use Icefury to proc Fusion of Elements." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains,if=cooldown_react&buff.lava_surge.up&!buff.master_of_the_elements.up&talent.master_of_the_elements&active_enemies<=3", "[2-3t] Use Lava Surge procs to buff <anything> with MotE on 2-3 targets." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=!buff.master_of_the_elements.up&talent.master_of_the_elements&(buff.stormkeeper.up|buff.tempest.up|maelstrom>82-10*talent.eye_of_the_storm|maelstrom>52-5*talent.eye_of_the_storm&(buff.echoes_of_great_sundering_eb.up|!talent.elemental_blast))&active_enemies<=3&!talent.lightning_rod&talent.call_of_the_ancestors", "[2-3t]{Farseer} Use all Lava bursts to buff spenders, SK_CL and Tempest with MotE on 2-3 targets if not talented into Lightning Rod." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=!buff.master_of_the_elements.up&active_enemies=2", "[2t] Use all Lava bursts to buff <anything> with MotE on 2 targets." );
  aoe->add_action( "flame_shock,target_if=min:debuff.lightning_rod.remains,if=active_dot.flame_shock=0&buff.fusion_of_elements_fire.up&(!talent.elemental_blast|!talent.echoes_of_great_sundering&active_enemies>1+talent.tempest)" );
  aoe->add_action( "earthquake,if=((buff.stormkeeper.up&spell_targets.chain_lightning>=6|buff.tempest.up)&talent.surge_of_power)&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering&(!talent.elemental_blast|active_enemies>1+talent.tempest))", "Spend to buff SK_CL (on 6+) or Tempest with SoP." );
  aoe->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=((buff.stormkeeper.up&active_enemies>=6|buff.tempest.up)&talent.surge_of_power)&(active_enemies<=1+talent.tempest|talent.echoes_of_great_sundering&!buff.echoes_of_great_sundering_eb.up)" );
  aoe->add_action( "earth_shock,target_if=min:debuff.lightning_rod.remains,if=((buff.stormkeeper.up&active_enemies>=6|buff.tempest.up)&talent.surge_of_power)&talent.echoes_of_great_sundering&!buff.echoes_of_great_sundering_es.up" );
  aoe->add_action( "frost_shock,if=buff.icefury_dmg.up&!buff.ascendance.up&!buff.stormkeeper.up&talent.call_of_the_ancestors" );
  aoe->add_action( "chain_lightning" );
  aoe->add_action( "flame_shock,moving=1,target_if=refreshable" );
  aoe->add_action( "frost_shock,moving=1" );

  single_target->add_action( "fire_elemental" );
  single_target->add_action( "storm_elemental,if=!buff.storm_elemental.up|!talent.echo_of_the_elementals" );
  single_target->add_action( "stormkeeper,if=!talent.fury_of_the_storms|cooldown.primordial_wave.remains<gcd|!talent.primordial_wave", "Just use Stormkeeper." );
  single_target->add_action( "liquid_magma_totem,if=!dot.flame_shock.ticking&!buff.surge_of_power.up&!buff.master_of_the_elements.up", "Apply Flame shock if it is not up." );
  single_target->add_action( "flame_shock,if=!dot.flame_shock.ticking&!buff.surge_of_power.up&!buff.master_of_the_elements.up" );
  single_target->add_action( "primordial_wave", "Use Primordial Wave as much as possible." );
  single_target->add_action( "ancestral_swiftness" );
  single_target->add_action( "ascendance,if=(talent.first_ascendant|fight_remains>200|fight_remains<80|buff.spymasters_web.up|variable.trinket_1_buffs&!trinket.1.is.spymasters_web&trinket.1.ready_cooldown|variable.trinket_2_buffs&!trinket.2.is.spymasters_web&trinket.2.ready_cooldown|equipped.neural_synapse_enhancer&cooldown.neural_synapse_enhancer.remains=0|equipped.bestinslots&cooldown.bestinslots.remains=0)&(buff.fury_of_storms.up|!talent.fury_of_the_storms)&(cooldown.primordial_wave.remains>25|!talent.primordial_wave)" );
  single_target->add_action( "tempest,if=buff.surge_of_power.up", "Surge of Power is strong and should be used.©" );
  single_target->add_action( "lightning_bolt,if=buff.surge_of_power.up" );
  single_target->add_action( "tempest,if=buff.storm_frenzy.stack=2&!talent.surge_of_power.enabled" );
  single_target->add_action( "liquid_magma_totem,if=dot.flame_shock.refreshable&!buff.master_of_the_elements.up", "Use LMT to apply Flame Shock." );
  single_target->add_action( "flame_shock,if=dot.flame_shock.refreshable&!buff.surge_of_power.up&!buff.master_of_the_elements.up&talent.erupting_lava", "Maintain Flame shock if talented into Erupting Lava." );
  single_target->add_action( "elemental_blast,if=maelstrom>variable.mael_cap-15|buff.master_of_the_elements.up", "Spend if close to overcaping or MotE buff is up. Friendship ended with Echoes of Great Sundering." );
  single_target->add_action( "earth_shock,if=maelstrom>variable.mael_cap-15|buff.master_of_the_elements.up" );
  single_target->add_action( "icefury,if=!(buff.fusion_of_elements_nature.up|buff.fusion_of_elements_fire.up)", "Use Icefury to proc Fusion of Elements." );
  single_target->add_action( "lava_burst,target_if=dot.flame_shock.remains>=2,if=!buff.master_of_the_elements.up&(!talent.master_of_the_elements|buff.lava_surge.up|buff.tempest.up|buff.stormkeeper.up|cooldown.lava_burst.charges_fractional>1.8|maelstrom>82-10*talent.eye_of_the_storm|maelstrom>52-5*talent.eye_of_the_storm&(buff.echoes_of_great_sundering_eb.up|!talent.elemental_blast))", "Use Lava Burst to proc Master of the Elements." );
  single_target->add_action( "earthquake,if=buff.echoes_of_great_sundering_eb.up&(buff.tempest.up|buff.stormkeeper.up)&talent.surge_of_power&!talent.master_of_the_elements", "Spend to activate Surge of Power buff for Tempest or Stormkeeper." );
  single_target->add_action( "elemental_blast,if=(buff.tempest.up|buff.stormkeeper.up)&talent.surge_of_power&!talent.master_of_the_elements" );
  single_target->add_action( "earth_shock,if=(buff.tempest.up|buff.stormkeeper.up)&talent.surge_of_power&!talent.master_of_the_elements" );
  single_target->add_action( "tempest" );
  single_target->add_action( "lightning_bolt,if=buff.storm_elemental.up&buff.wind_gust.stack<4" );
  single_target->add_action( "frost_shock,if=buff.icefury_dmg.up&!buff.ascendance.up&!buff.stormkeeper.up&talent.call_of_the_ancestors", "Use Icefury-empowered Frost Shocks outside of Ascendance." );
  single_target->add_action( "lightning_bolt", "Filler spell. Always available. Always the bottom line." );
  single_target->add_action( "flame_shock,moving=1,target_if=refreshable" );
  single_target->add_action( "flame_shock,moving=1,if=movement.distance>6" );
  single_target->add_action( "frost_shock,moving=1", "Frost Shock is our movement filler." );
}
//elemental_ptr_apl_end

} //namespace shaman_apl
