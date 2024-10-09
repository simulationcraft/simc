#include "class_modules/apl/apl_shaman.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace shaman_apl
{

std::string flask_elemental( const player_t* p )
{
  return ( p->true_level >= 71 ) ? "flask_of_tempered_swiftness_3" : "disabled";
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

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "flametongue_weapon,if=talent.improved_flametongue_weapon.enabled", "Ensure weapon enchant is applied if you've selected Improved Flametongue Weapon." );
  precombat->add_action( "lightning_shield" );
  precombat->add_action( "thunderstrike_ward" );
  precombat->add_action( "stormkeeper" );
  precombat->add_action( "variable,name=mael_cap,value=100+50*talent.swelling_maelstrom.enabled+25*talent.primordial_capacity.enabled,op=set" );
  precombat->add_action( "variable,name=spymaster_in_1st,value=trinket.1.is.spymasters_web" );
  precombat->add_action( "variable,name=spymaster_in_2nd,value=trinket.2.is.spymasters_web" );

  default_->add_action( "spiritwalkers_grace,moving=1,if=movement.distance>6", "Enable more movement." );
  default_->add_action( "wind_shear", "Interrupt of casts." );
  default_->add_action( "blood_fury,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50" );
  default_->add_action( "berserking,if=!talent.ascendance.enabled|buff.ascendance.up" );
  default_->add_action( "fireblood,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50" );
  default_->add_action( "ancestral_call,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50" );
  default_->add_action( "use_item,slot=trinket1,if=!variable.spymaster_in_1st|(fight_remains<45|time<fight_remains&buff.spymasters_report.stack=40)&cooldown.stormkeeper.remains<5|fight_remains<22" );
  default_->add_action( "use_item,slot=trinket2,if=!variable.spymaster_in_2nd|(fight_remains<45|time<fight_remains&buff.spymasters_report.stack=40)&cooldown.stormkeeper.remains<5|fight_remains<22" );
  default_->add_action( "use_item,slot=main_hand" );
  default_->add_action( "lightning_shield,if=buff.lightning_shield.down" );
  default_->add_action( "natures_swiftness" );
  default_->add_action( "invoke_external_buff,name=power_infusion", "Use Power Infusion on Cooldown." );
  default_->add_action( "potion" );
  default_->add_action( "run_action_list,name=aoe,if=spell_targets.chain_lightning>2" );
  default_->add_action( "run_action_list,name=single_target" );

  aoe->add_action( "fire_elemental,if=!buff.fire_elemental.up" );
  aoe->add_action( "storm_elemental,if=!buff.storm_elemental.up" );
  aoe->add_action( "stormkeeper,if=!buff.stormkeeper.up" );
  aoe->add_action( "totemic_recall,if=cooldown.liquid_magma_totem.remains>15&(active_dot.flame_shock<(spell_targets.chain_lightning>?6)-2|talent.fire_elemental.enabled)", "Reset LMT CD as early as possible. Always for Fire, only if you can dot up 3 more targets for Lightning." );
  aoe->add_action( "liquid_magma_totem" );
  aoe->add_action( "primordial_wave,target_if=min:dot.flame_shock.remains,if=buff.surge_of_power.up|!talent.surge_of_power.enabled|maelstrom<60-5*talent.eye_of_the_storm.enabled", "Spread Flame Shock via Primordial Wave using Surge of Power if possible." );
  aoe->add_action( "ancestral_swiftness" );
  aoe->add_action( "flame_shock,target_if=refreshable,if=buff.surge_of_power.up&talent.lightning_rod.enabled&dot.flame_shock.remains<target.time_to_die-16&active_dot.flame_shock<(spell_targets.chain_lightning>?6)&!talent.liquid_magma_totem.enabled", "{Lightning} Spread Flame Shock using Surge of Power if LMT is not picked." );
  aoe->add_action( "flame_shock,target_if=min:dot.flame_shock.remains,if=buff.primordial_wave.up&buff.stormkeeper.up&maelstrom<60-5*talent.eye_of_the_storm.enabled-(8+2*talent.flow_of_power.enabled)*active_dot.flame_shock&spell_targets.chain_lightning>=6&active_dot.flame_shock<6", "{Lightning} Cast extra Flame Shock to help getting to next spender for SK+SoP on 6+ targets. Mostly opener." );
  aoe->add_action( "flame_shock,target_if=refreshable,if=talent.fire_elemental.enabled&(buff.surge_of_power.up|!talent.surge_of_power.enabled)&dot.flame_shock.remains<target.time_to_die-5&(active_dot.flame_shock<6|dot.flame_shock.remains>0)", "{Fire} Spread and refresh Flame Shock using Surge of Power (if talented) up to 6." );
  aoe->add_action( "tempest,target_if=min:debuff.lightning_rod.remains,if=!buff.arc_discharge.up" );
  aoe->add_action( "ascendance", "JUST DO IT! https://i.kym-cdn.com/entries/icons/mobile/000/018/147/Shia_LaBeouf__Just_Do_It__Motivational_Speech_(Original_Video_by_LaBeouf__R%C3%B6nkk%C3%B6___Turner)_0-4_screenshot.jpg" );
  aoe->add_action( "lava_beam,if=active_enemies>=6&buff.surge_of_power.up&buff.ascendance.remains>cast_time", "Against 6 targets or more Surge of Power should be used with Lava Beam rather than Lava Burst." );
  aoe->add_action( "chain_lightning,if=active_enemies>=6&buff.surge_of_power.up", "Against 6 targets or more Surge of Power should be used with Chain Lightning rather than Lava Burst." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=buff.primordial_wave.up&buff.stormkeeper.up&maelstrom<60-5*talent.eye_of_the_storm.enabled&spell_targets.chain_lightning>=6&talent.surge_of_power.enabled", "Consume Primordial Wave buff immediately if you have Stormkeeper buff, fighting 6+ enemies and need maelstrom to spend." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=buff.primordial_wave.up&(buff.primordial_wave.remains<4|buff.lava_surge.up)", "Cast Lava burst to consume Primordial Wave proc. Wait for Lava Surge proc if possible." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains,if=cooldown_react&buff.lava_surge.up&!buff.master_of_the_elements.up&talent.master_of_the_elements.enabled&talent.fire_elemental.enabled", "{Fire} Use Lava Surge proc to buff <anything> with Master of the Elements." );
  aoe->add_action( "earthquake,if=cooldown.primordial_wave.remains<gcd&talent.surge_of_power.enabled&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering.enabled)", "Activate Surge of Power if next global is Primordial wave. Respect Echoes of Great Sundering." );
  aoe->add_action( "earthquake,if=(lightning_rod=0&talent.lightning_rod.enabled|maelstrom>variable.mael_cap-30)&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering.enabled)", "Spend if all Lightning Rods ran out or you are close to overcaping. Respect Echoes of Great Sundering." );
  aoe->add_action( "earthquake,if=buff.stormkeeper.up&spell_targets.chain_lightning>=6&talent.surge_of_power.enabled&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering.enabled)", "Spend to buff your follow-up Stormkeeper with Surge of Power on 6+ targets. Respect Echoes of Great Sundering." );
  aoe->add_action( "earthquake,if=(buff.master_of_the_elements.up|spell_targets.chain_lightning>=5)&(buff.fusion_of_elements_nature.up|buff.ascendance.remains>9|!buff.ascendance.up)&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering.enabled)&talent.fire_elemental.enabled", "{Fire} Spend if you have Master of the elements buff or fighting 5+ enemies. Bank maelstrom during the end of Ascendance. Respect Echoes of Great Sundering." );
  aoe->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=talent.echoes_of_great_sundering.enabled&!buff.echoes_of_great_sundering_eb.up&(lightning_rod=0|maelstrom>variable.mael_cap-30)", "Use the talents you selected. Spread Lightning Rod to as many targets as possible." );
  aoe->add_action( "earth_shock,target_if=min:debuff.lightning_rod.remains,if=talent.echoes_of_great_sundering.enabled&!buff.echoes_of_great_sundering_es.up&(lightning_rod=0|maelstrom>variable.mael_cap-30)", "Use the talents you selected. Spread Lightning Rod to as many targets as possible." );
  aoe->add_action( "icefury,if=talent.fusion_of_elements.enabled&!(buff.fusion_of_elements_nature.up|buff.fusion_of_elements_fire.up)", "Use Icefury for Fusion of Elements proc." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=talent.master_of_the_elements.enabled&!buff.master_of_the_elements.up&!buff.ascendance.up&talent.fire_elemental.enabled", "{Fire} Proc Master of the Elements outside Ascendance." );
  aoe->add_action( "lava_beam,if=buff.stormkeeper.up&(buff.surge_of_power.up|spell_targets.lava_beam<6)", "Stormkeeper is strong and should be used." );
  aoe->add_action( "chain_lightning,if=buff.stormkeeper.up&(buff.surge_of_power.up|spell_targets.chain_lightning<6)", "Stormkeeper is strong and should be used." );
  aoe->add_action( "lava_beam,if=buff.power_of_the_maelstrom.up&buff.ascendance.remains>cast_time&!buff.stormkeeper.up", "Power of the Maelstrom is strong and should be used." );
  aoe->add_action( "chain_lightning,if=buff.power_of_the_maelstrom.up&!buff.stormkeeper.up", "Power of the Maelstrom is strong and should be used." );
  aoe->add_action( "lava_beam,if=(buff.master_of_the_elements.up&spell_targets.lava_beam>=4|spell_targets.lava_beam>=5)&buff.ascendance.remains>cast_time&!buff.stormkeeper.up", "Consume Master of the Elements with Lava Beam on 4+ targets. Just spam it over hardcasted Lava Burst on 5+ targets." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=talent.deeply_rooted_elements.enabled", "Gamble away for Deeply Rooted Elements procs." );
  aoe->add_action( "lava_beam,if=buff.ascendance.remains>cast_time" );
  aoe->add_action( "chain_lightning" );
  aoe->add_action( "flame_shock,moving=1,target_if=refreshable" );
  aoe->add_action( "frost_shock,moving=1" );

  single_target->add_action( "fire_elemental,if=!buff.fire_elemental.up" );
  single_target->add_action( "storm_elemental,if=!buff.storm_elemental.up" );
  single_target->add_action( "stormkeeper,if=!buff.ascendance.up&!buff.stormkeeper.up", "Just use Stormkeeper." );
  single_target->add_action( "totemic_recall,if=cooldown.liquid_magma_totem.remains>15&spell_targets.chain_lightning>1&talent.fire_elemental.enabled", "{Fire} Reset LMT CD as early as possible." );
  single_target->add_action( "liquid_magma_totem,if=!buff.ascendance.up&(talent.fire_elemental.enabled|spell_targets.chain_lightning>1)", "Use LMT outside Ascendance in fire builds and on 2 targets for lightning." );
  single_target->add_action( "primordial_wave,target_if=min:dot.flame_shock.remains,if=spell_targets.chain_lightning=1|buff.surge_of_power.up|!talent.surge_of_power.enabled|maelstrom<60-5*talent.eye_of_the_storm.enabled|talent.liquid_magma_totem.enabled", "Use Primordial Wave as much as possible. Buff with Surge of power on 2 targets if not playing LMT." );
  single_target->add_action( "ancestral_swiftness,if=!buff.primordial_wave.up|!buff.stormkeeper.up|!talent.elemental_blast.enabled", "Use Ancestral Swiftness as much as possible. Use on EB instead of LvB where possible." );
  single_target->add_action( "flame_shock,target_if=min:dot.flame_shock.remains,if=active_enemies=1&(dot.flame_shock.remains<2|active_dot.flame_shock=0)&(dot.flame_shock.remains<cooldown.primordial_wave.remains|!talent.primordial_wave.enabled)&(dot.flame_shock.remains<cooldown.liquid_magma_totem.remains|!talent.liquid_magma_totem.enabled)&!buff.surge_of_power.up&talent.fire_elemental.enabled", "{Fire} Manually refresh Flame shock if better options are not available." );
  single_target->add_action( "flame_shock,target_if=min:dot.flame_shock.remains,if=active_dot.flame_shock<active_enemies&spell_targets.chain_lightning>1&(talent.deeply_rooted_elements.enabled|talent.ascendance.enabled|talent.primordial_wave.enabled|talent.searing_flames.enabled|talent.magma_chamber.enabled)&(!buff.surge_of_power.up&buff.stormkeeper.up|!talent.surge_of_power.enabled|cooldown.ascendance.remains=0&talent.ascendance.enabled)&!talent.liquid_magma_totem.enabled", "Use Flame shock without Surge of Power if you can't spread it with SoP because it is going to be used on Stormkeeper or Surge of Power is not talented." );
  single_target->add_action( "flame_shock,target_if=min:dot.flame_shock.remains,if=spell_targets.chain_lightning>1&(talent.deeply_rooted_elements.enabled|talent.ascendance.enabled|talent.primordial_wave.enabled|talent.searing_flames.enabled|talent.magma_chamber.enabled)&(buff.surge_of_power.up&!buff.stormkeeper.up|!talent.surge_of_power.enabled)&dot.flame_shock.remains<6&talent.fire_elemental.enabled,cycle_targets=1", "{Fire} Spread Flame Shock to multiple targets only if talents were selected that benefit from it." );
  single_target->add_action( "tempest,target_if=min:debuff.lightning_rod.remains,if=!buff.arc_discharge.up" );
  single_target->add_action( "lightning_bolt,if=buff.stormkeeper.up&buff.surge_of_power.up", "Stormkeeper is strong and should be used." );
  single_target->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=buff.stormkeeper.up&!buff.master_of_the_elements.up&!talent.surge_of_power.enabled&talent.master_of_the_elements.enabled", "Buff stormkeeper with MotE when not using Surge of Power." );
  single_target->add_action( "lightning_bolt,if=buff.stormkeeper.up&!talent.surge_of_power.enabled&(buff.master_of_the_elements.up|!talent.master_of_the_elements.enabled)", "Buff Stormkeeper with at least something if you can." );
  single_target->add_action( "lightning_bolt,if=buff.surge_of_power.up&!buff.ascendance.up&talent.echo_chamber.enabled", "Surge of Power is strong and should be used." );
  single_target->add_action( "ascendance,if=cooldown.lava_burst.charges_fractional<1.0" );
  single_target->add_action( "lava_burst,target_if=dot.flame_shock.remains,if=cooldown_react&buff.lava_surge.up&talent.fire_elemental.enabled", "{Fire} Lava Surge is neat. Utilize it." );
  single_target->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=buff.primordial_wave.up", "Consume Primordial wave buff." );
  single_target->add_action( "earthquake,if=buff.master_of_the_elements.up&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|spell_targets.chain_lightning>1&!talent.echoes_of_great_sundering.enabled&!talent.elemental_blast.enabled)&(buff.fusion_of_elements_nature.up|maelstrom>variable.mael_cap-15|buff.ascendance.remains>9|!buff.ascendance.up)&talent.fire_elemental.enabled", "{Fire} Spend if you have MotE buff and: not in Ascendance OR Ascendance gona last so long you will need to spend anyway OR nature fusion buff up OR close to maelstrom cap. Respect Echoes of Great Sundering." );
  single_target->add_action( "elemental_blast,if=buff.master_of_the_elements.up&(buff.fusion_of_elements_nature.up|buff.fusion_of_elements_fire.up|maelstrom>variable.mael_cap-15|buff.ascendance.remains>6|!buff.ascendance.up)&talent.fire_elemental.enabled", "{Fire} Spend if you have MotE buff and: not in Ascendance OR Ascendance gona last so long you will need to spend anyway OR any fusion buff up OR close to maelstrom cap." );
  single_target->add_action( "earth_shock,if=buff.master_of_the_elements.up&(buff.fusion_of_elements_nature.up|maelstrom>variable.mael_cap-15|buff.ascendance.remains>9|!buff.ascendance.up)&talent.fire_elemental.enabled", "{Fire} Spend if you have MotE buff and: not in Ascendance OR Ascendance gona last so long you will need to spend anyway OR nature fusion buff up OR close to maelstrom cap." );
  single_target->add_action( "earthquake,if=(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|spell_targets.chain_lightning>1&!talent.echoes_of_great_sundering.enabled&!talent.elemental_blast.enabled)&(buff.stormkeeper.up|cooldown.primordial_wave.remains<gcd&talent.surge_of_power.enabled&!talent.liquid_magma_totem.enabled)&talent.storm_elemental.enabled", "{Lightning} Spend if Stormkeeper is active OR Pwave is coming next gcd and you arent specced into LMT. Respect Echoes of Great Sundering." );
  single_target->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=buff.stormkeeper.up&talent.storm_elemental.enabled", "{Lightning} Spend if Stormkeeper is active. Spread Lightning Rod to as many targets as possible." );
  single_target->add_action( "earth_shock,if=((buff.master_of_the_elements.up|lightning_rod=0)&cooldown.stormkeeper.remains>10&(rolling_thunder.next_tick>5|!talent.rolling_thunder.enabled)|buff.stormkeeper.up)&talent.storm_elemental.enabled&spell_targets.chain_lightning=1", "{Lightning}[1t] Spend if you have Master of the Elements buff and Stormkeeper is not coming up soon OR Stormkeeper is active OR Lightning Rod ran out." );
  single_target->add_action( "earth_shock,target_if=min:debuff.lightning_rod.remains,if=(cooldown.primordial_wave.remains<gcd&talent.surge_of_power.enabled&!talent.liquid_magma_totem.enabled|buff.stormkeeper.up)&talent.storm_elemental.enabled&spell_targets.chain_lightning>1&talent.echoes_of_great_sundering.enabled&!buff.echoes_of_great_sundering_es.up", "{Lightning}[2t] Spend if Stormkeeper is active OR Pwave is coming next gcd and you arent specced into LMT. Spread Lightning Rod to as many targets as possible. Use Echoes of Great Sundering." );
  single_target->add_action( "icefury,if=!(buff.fusion_of_elements_nature.up|buff.fusion_of_elements_fire.up)&buff.icefury.stack=2&(talent.fusion_of_elements.enabled|!buff.ascendance.up)", "Don't waste Icefury stacks even during Ascendance." );
  single_target->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=buff.ascendance.up", "Spam Lava burst in Ascendance." );
  single_target->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=talent.master_of_the_elements.enabled&!buff.master_of_the_elements.up&talent.fire_elemental.enabled", "{Fire} Buff your next <anything> with MotE." );
  single_target->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=buff.stormkeeper.up&talent.elemental_reverb.enabled&talent.earth_shock.enabled&time<10", "{Farseer/ES} Spend all Lava Burst charges in opener to get one Stormkeeper buffed with Surge of Power." );
  single_target->add_action( "earthquake,if=(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|spell_targets.chain_lightning>1&!talent.echoes_of_great_sundering.enabled&!talent.elemental_blast.enabled)&(maelstrom>variable.mael_cap-35|fight_remains<5)", "Spend if close to overcaping. Respect Echoes of Great Sundering." );
  single_target->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=maelstrom>variable.mael_cap-15|fight_remains<5", "Spend if close to overcaping." );
  single_target->add_action( "earth_shock,target_if=min:debuff.lightning_rod.remains,if=maelstrom>variable.mael_cap-15|fight_remains<5", "Spend if close to overcaping." );
  single_target->add_action( "lightning_bolt,if=buff.surge_of_power.up" );
  single_target->add_action( "icefury,if=!(buff.fusion_of_elements_nature.up|buff.fusion_of_elements_fire.up)", "Use Icefury if you won't overwrite Fusion of Elements buffs." );
  single_target->add_action( "frost_shock,if=buff.icefury_dmg.up&(spell_targets.chain_lightning=1|buff.stormkeeper.up&talent.surge_of_power.enabled)", "Use Icefury-buffed Frost Shock against 1 target or if you need to generate for SoP buff on Stormkeeper." );
  single_target->add_action( "chain_lightning,if=buff.power_of_the_maelstrom.up&spell_targets.chain_lightning>1&!buff.stormkeeper.up", "Utilize the Power of the Maelstrom buff." );
  single_target->add_action( "lightning_bolt,if=buff.power_of_the_maelstrom.up&!buff.stormkeeper.up", "Utilize the Power of the Maelstrom buff." );
  single_target->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=talent.deeply_rooted_elements.enabled", "Fish for DRE procs." );
  single_target->add_action( "chain_lightning,if=spell_targets.chain_lightning>1&!buff.stormkeeper.up", "Casting Chain Lightning at two targets is more efficient than Lightning Bolt." );
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

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "flametongue_weapon,if=talent.improved_flametongue_weapon.enabled", "Ensure weapon enchant is applied if you've selected Improved Flametongue Weapon." );
  precombat->add_action( "lightning_shield" );
  precombat->add_action( "thunderstrike_ward" );
  precombat->add_action( "stormkeeper" );
  precombat->add_action( "variable,name=mael_cap,value=100+50*talent.swelling_maelstrom.enabled+25*talent.primordial_capacity.enabled,op=set" );
  precombat->add_action( "variable,name=spymaster_in_1st,value=trinket.1.is.spymasters_web" );
  precombat->add_action( "variable,name=spymaster_in_2nd,value=trinket.2.is.spymasters_web" );

  default_->add_action( "spiritwalkers_grace,moving=1,if=movement.distance>6", "Enable more movement." );
  default_->add_action( "wind_shear", "Interrupt of casts." );
  default_->add_action( "blood_fury,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50" );
  default_->add_action( "berserking,if=!talent.ascendance.enabled|buff.ascendance.up" );
  default_->add_action( "fireblood,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50" );
  default_->add_action( "ancestral_call,if=!talent.ascendance.enabled|buff.ascendance.up|cooldown.ascendance.remains>50" );
  default_->add_action( "use_item,slot=trinket1,if=!variable.spymaster_in_1st|(fight_remains<45|time<fight_remains&buff.spymasters_report.stack=40)&cooldown.stormkeeper.remains<5|fight_remains<22" );
  default_->add_action( "use_item,slot=trinket2,if=!variable.spymaster_in_2nd|(fight_remains<45|time<fight_remains&buff.spymasters_report.stack=40)&cooldown.stormkeeper.remains<5|fight_remains<22" );
  default_->add_action( "use_item,slot=main_hand" );
  default_->add_action( "lightning_shield,if=buff.lightning_shield.down" );
  default_->add_action( "natures_swiftness" );
  default_->add_action( "invoke_external_buff,name=power_infusion", "Use Power Infusion on Cooldown." );
  default_->add_action( "potion" );
  default_->add_action( "run_action_list,name=aoe,if=spell_targets.chain_lightning>=2" );
  default_->add_action( "run_action_list,name=single_target" );

  aoe->add_action( "fire_elemental" );
  aoe->add_action( "storm_elemental" );
  aoe->add_action( "stormkeeper" );
  aoe->add_action( "totemic_recall,if=cooldown.liquid_magma_totem.remains>15&(active_dot.flame_shock<(spell_targets.chain_lightning>?6)-2|talent.fire_elemental.enabled)", "Reset LMT CD as early as possible. Always for Fire, only if you can dot up 3 more targets for Lightning." );
  aoe->add_action( "liquid_magma_totem" );
  aoe->add_action( "primordial_wave,target_if=min:dot.flame_shock.remains,if=buff.surge_of_power.up|!talent.surge_of_power.enabled|maelstrom<60-5*talent.eye_of_the_storm.enabled", "Spread Flame Shock via Primordial Wave using Surge of Power if possible." );
  aoe->add_action( "ancestral_swiftness" );
  aoe->add_action( "flame_shock,target_if=refreshable,if=buff.surge_of_power.up&dot.flame_shock.remains<target.time_to_die-16&active_dot.flame_shock<(spell_targets.chain_lightning>?6)&!talent.liquid_magma_totem.enabled", "Spread Flame Shock using Surge of Power if LMT is not picked." );
  aoe->add_action( "flame_shock,target_if=refreshable,if=talent.fire_elemental.enabled&(buff.surge_of_power.up|!talent.surge_of_power.enabled)&dot.flame_shock.remains<target.time_to_die-5&(active_dot.flame_shock<6|dot.flame_shock.remains>0)", "Spread and refresh Flame Shock using Surge of Power (if talented) up to 6." );
  aoe->add_action( "ascendance", "JUST DO IT! https://i.kym-cdn.com/entries/icons/mobile/000/018/147/Shia_LaBeouf__Just_Do_It__Motivational_Speech_(Original_Video_by_LaBeouf__R%C3%B6nkk%C3%B6___Turner)_0-4_screenshot.jpg" );
  aoe->add_action( "tempest,target_if=min:debuff.lightning_rod.remains,if=!buff.arc_discharge.up&(buff.surge_of_power.up|!talent.surge_of_power.enabled)", "###buff tempest with sop" );

  single_target->add_action( "lightning_bolt,if=buff.stormkeeper.up&buff.surge_of_power.up&spell_targets.chain_lightning=2", "###2t" );

  aoe->add_action( "chain_lightning,if=active_enemies>=6&buff.surge_of_power.up", "Against 6 targets or more Surge of Power should be used with Chain Lightning rather than Lava Burst." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=buff.primordial_wave.up&(buff.stormkeeper.up&spell_targets.chain_lightning>=6|buff.tempest.up)&maelstrom<60-5*talent.eye_of_the_storm.enabled&talent.surge_of_power.enabled", "Consume Primordial Wave buff immediately if you have Stormkeeper buff, fighting 6+ enemies and need maelstrom to spend." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=buff.primordial_wave.up&(buff.primordial_wave.remains<4|buff.lava_surge.up)", "Cast Lava burst to consume Primordial Wave proc. Wait for Lava Surge proc if possible." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains,if=cooldown_react&buff.lava_surge.up&!buff.master_of_the_elements.up&talent.master_of_the_elements.enabled&talent.fire_elemental.enabled", "*{Fire} Use Lava Surge proc to buff <anything> with Master of the Elements." );
  aoe->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=spell_targets.chain_lightning=2&(maelstrom>variable.mael_cap-30|cooldown.primordial_wave.remains<gcd&talent.surge_of_power.enabled|(buff.stormkeeper.up&spell_targets.chain_lightning>=6|buff.tempest.up)&talent.surge_of_power.enabled)", "###2t" );
  aoe->add_action( "earthquake,if=cooldown.primordial_wave.remains<gcd&talent.surge_of_power.enabled&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering.enabled)", "Activate Surge of Power if next global is Primordial wave. Respect Echoes of Great Sundering." );
  aoe->add_action( "earthquake,if=(lightning_rod=0&talent.lightning_rod.enabled|maelstrom>variable.mael_cap-30)&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering.enabled)", "Spend if all Lightning Rods ran out or you are close to overcaping. Respect Echoes of Great Sundering." );
  aoe->add_action( "earthquake,if=(buff.stormkeeper.up&spell_targets.chain_lightning>=6|buff.tempest.up)&talent.surge_of_power.enabled&(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up|!talent.echoes_of_great_sundering.enabled)", "Spend to buff your follow-up Stormkeeper with Surge of Power on 6+ targets. Respect Echoes of Great Sundering." );
  aoe->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=talent.echoes_of_great_sundering.enabled&!buff.echoes_of_great_sundering_eb.up&(lightning_rod=0|maelstrom>variable.mael_cap-30|(buff.stormkeeper.up&spell_targets.chain_lightning>=6|buff.tempest.up)&talent.surge_of_power.enabled)", "Use the talents you selected. Spread Lightning Rod to as many targets as possible." );
  aoe->add_action( "earth_shock,target_if=min:debuff.lightning_rod.remains,if=talent.echoes_of_great_sundering.enabled&!buff.echoes_of_great_sundering_es.up&(lightning_rod=0|maelstrom>variable.mael_cap-30|(buff.stormkeeper.up&spell_targets.chain_lightning>=6|buff.tempest.up)&talent.surge_of_power.enabled)", "Use the talents you selected. Spread Lightning Rod to as many targets as possible." );
  aoe->add_action( "icefury,if=talent.fusion_of_elements.enabled&!(buff.fusion_of_elements_nature.up|buff.fusion_of_elements_fire.up)", "Use Icefury for Fusion of Elements proc." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=talent.master_of_the_elements.enabled&!buff.master_of_the_elements.up&!buff.ascendance.up&talent.fire_elemental.enabled", "*{Fire} Proc Master of the Elements outside Ascendance." );
  aoe->add_action( "chain_lightning" );
  aoe->add_action( "flame_shock,moving=1,target_if=refreshable" );
  aoe->add_action( "frost_shock,moving=1" );

  single_target->add_action( "fire_elemental" );
  single_target->add_action( "storm_elemental" );
  single_target->add_action( "stormkeeper,if=!talent.ascendance.enabled|cooldown.ascendance.remains<gcd|cooldown.ascendance.remains>10", "Just use Stormkeeper." );
  single_target->add_action( "totemic_recall,if=cooldown.liquid_magma_totem.remains>15&spell_targets.chain_lightning>1&talent.fire_elemental.enabled", "{Fire} Reset LMT CD as early as possible. ##suspend?" );
  single_target->add_action( "primordial_wave,if=!buff.surge_of_power.up", "Use Primordial Wave as much as possible." );
  single_target->add_action( "ancestral_swiftness,if=!buff.primordial_wave.up|!buff.stormkeeper.up|!talent.elemental_blast.enabled", "*Use Ancestral Swiftness as much as possible. Use on EB instead of LvB where possible." );
  single_target->add_action( "ascendance" );
  single_target->add_action( "tempest,if=buff.surge_of_power.up", "Surge of Power is strong and should be used. ©" );
  single_target->add_action( "lightning_bolt,if=buff.surge_of_power.up" );
  single_target->add_action( "liquid_magma_totem,if=active_dot.flame_shock=0", "Use LMT to apply Flame Shock." );
  single_target->add_action( "flame_shock,if=active_dot.flame_shock=0|(dot.flame_shock.remains<6&(dot.flame_shock.remains<cooldown.primordial_wave.remains|!talent.primordial_wave.enabled)&(dot.flame_shock.remains<cooldown.liquid_magma_totem.remains|!talent.liquid_magma_totem.enabled)&!buff.surge_of_power.up&!buff.master_of_the_elements.up)", "Manually refresh Flame shock if better options are not available." );
  single_target->add_action( "earthquake,if=(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up)&(maelstrom>variable.mael_cap-15|fight_remains<5)", "Spend if close to overcaping. Respect Echoes of Great Sundering." );
  single_target->add_action( "elemental_blast,if=maelstrom>variable.mael_cap-15|fight_remains<5" );
  single_target->add_action( "earth_shock,if=maelstrom>variable.mael_cap-15|fight_remains<5" );
  single_target->add_action( "icefury,if=!(buff.fusion_of_elements_nature.up|buff.fusion_of_elements_fire.up)&talent.fusion_of_elements.enabled", "Use Icefury to proc Fusion of Elements if Master of the Elements buff is not up or cant be used on smth better." );
  single_target->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=charges_fractional>=max_charges-0.8" );
  single_target->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=!talent.master_of_the_elements.enabled" );
  single_target->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=buff.primordial_wave.up&buff.primordial_wave.remains<4", "Dont waste Primordial Wave buff." );
  single_target->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=talent.master_of_the_elements.enabled&!buff.master_of_the_elements.up&((buff.tempest.up|buff.stormkeeper.up)&maelstrom>52-5*talent.eye_of_the_storm.enabled-3*buff.ascendance.up+(25-5*talent.eye_of_the_storm.enabled)*talent.elemental_blast.enabled|maelstrom>variable.mael_cap-15)", "Buff your next spender with MotE." );
  single_target->add_action( "earthquake,if=(buff.echoes_of_great_sundering_es.up|buff.echoes_of_great_sundering_eb.up)&(buff.tempest.up|buff.stormkeeper.up)&talent.surge_of_power.enabled", "Spend to activate Surge of Power buff for Tempest or Stormkeeper." );
  single_target->add_action( "elemental_blast,if=(buff.tempest.up|buff.stormkeeper.up)&talent.surge_of_power.enabled" );
  single_target->add_action( "earth_shock,if=(buff.tempest.up|buff.stormkeeper.up)&talent.surge_of_power.enabled" );
  single_target->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=!buff.master_of_the_elements.up&(buff.tempest.up|buff.stormkeeper.up)", "Use Icefury-buffed Frost Shock. actions.single_target+=/frost_shock,if=buff.icefury_dmg.up" );
  single_target->add_action( "tempest" );
  single_target->add_action( "icefury,if=!(buff.fusion_of_elements_nature.up|buff.fusion_of_elements_fire.up)", "Use Icefury if you won't overwrite Fusion of Elements buffs." );
  single_target->add_action( "lightning_bolt", "Filler spell. Always available. Always the bottom line." );
  single_target->add_action( "flame_shock,moving=1,target_if=refreshable" );
  single_target->add_action( "flame_shock,moving=1,if=movement.distance>6" );
  single_target->add_action( "frost_shock,moving=1", "Frost Shock is our movement filler." );
}
//elemental_ptr_apl_end

} //namespace shaman_apl
