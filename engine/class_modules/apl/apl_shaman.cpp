#include "class_modules/apl/apl_shaman.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace shaman_apl
{

std::string flask_elemental( const player_t* p )
{
  return ( p->true_level >= 81 ) ? "flask_of_the_magisters_2" : "disabled";
}

std::string food_elemental( const player_t* p )
{
  return ( p->true_level >= 81 ) ? "harandar_celebration" : "disabled";
}

std::string potion_elemental( const player_t* p )
{
  return ( p->true_level >= 81 ) ? "lights_potential_2" : "disabled";
}

std::string temporary_enchant_elemental( const player_t* p )
{
  return ( p->true_level >= 81 ) ? "main_hand:thalassian_phoenix_oil_2,if=!talent.flametongue_weapon"
    : "disabled";
}

std::string rune( const player_t* p )
{
  return ( p->true_level >= 81 ) ? "void_touched" : "disabled";
}

//elemental_apl_start
void elemental( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* single_target = p->get_action_priority_list( "single_target" );

  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "flametongue_weapon,if=talent.flametongue_weapon" );
  precombat->add_action( "lightning_shield" );
  precombat->add_action( "thunderstrike_ward" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=(trinket.1.has_use_buff|trinket.1.is.funhouse_lens)" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=(trinket.2.has_use_buff|trinket.2.is.funhouse_lens)" );
  precombat->add_action( "stormkeeper" );

  default_->add_action( "spiritwalkers_grace,moving=1,if=movement.distance>6", "Enable more movement." );
  default_->add_action( "wind_shear", "Interrupt of casts." );
  default_->add_action( "blood_fury" );
  default_->add_action( "berserking" );
  default_->add_action( "fireblood" );
  default_->add_action( "ancestral_call" );
  default_->add_action( "use_item,slot=trinket1,use_off_gcd=1,if=variable.trinket_1_buffs&(cooldown.ascendance.remains>trinket.1.cooldown.duration-5|cooldown.ascendance.ready&cooldown.stormkeeper.remains>15|fight_remains<21)", "Normal buff trinkets, mimic Ascendance activation conditions" );
  default_->add_action( "use_item,slot=trinket2,use_off_gcd=1,if=variable.trinket_2_buffs&(cooldown.ascendance.remains>trinket.2.cooldown.duration-5|cooldown.ascendance.ready&cooldown.stormkeeper.remains>15|fight_remains<21)" );
  default_->add_action( "use_item,slot=main_hand,use_off_gcd=1", "Normal weapons" );
  default_->add_action( "use_item,slot=trinket1,use_off_gcd=1,if=!variable.trinket_1_buffs&(cooldown.ascendance.remains>20|trinket.2.cooldown.remains>20)", "Dmg trinkets" );
  default_->add_action( "use_item,slot=trinket2,use_off_gcd=1,if=!variable.trinket_2_buffs&(cooldown.ascendance.remains>20|trinket.1.cooldown.remains>20)" );
  default_->add_action( "lightning_shield,if=buff.lightning_shield.down" );
  default_->add_action( "natures_swiftness" );
  default_->add_action( "invoke_external_buff,name=power_infusion", "Use Power Infusion on Cooldown." );
  default_->add_action( "potion,if=buff.bloodlust.up|cooldown.ascendance.ready&cooldown.stormkeeper.remains>15|fight_remains<31" );
  default_->add_action( "run_action_list,name=aoe,if=spell_targets.chain_lightning>=3" );
  default_->add_action( "run_action_list,name=single_target" );

  aoe->add_action( "stormkeeper,if=cooldown.ascendance.remains>10|cooldown.ascendance.remains<gcd|fight_remains<20", "--- 3+ TARGET ROTATION ---  Stormkeeper on CD, unless sub 10s hold for Asc or the fight is about to end." );
  aoe->add_action( "ancestral_swiftness" );
  aoe->add_action( "flame_shock,if=!buff.master_of_the_elements.up&((dot.flame_shock.refreshable&cooldown.ascendance.remains>5)|(buff.fire_elemental.up&buff.fire_elemental.remains<2))&talent.master_of_the_elements&talent.inferno_arc&spell_targets.chain_lightning=3", "[3t] Apply Flame shock on 3t for MotE and Inferno arc." );
  aoe->add_action( "voltaic_blaze,if=!buff.master_of_the_elements.up&((dot.flame_shock.refreshable&cooldown.ascendance.remains>5)|(buff.fire_elemental.up&buff.fire_elemental.remains<2)|talent.purging_flames)", "Apply Voltaic blaze for Inferno arc or Purging flames." );
  aoe->add_action( "ascendance,if=cooldown.stormkeeper.remains>15|fight_remains<20", "Ascendance on CD, unless SK can be sync'd with it." );
  aoe->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=buff.tempest.stack<2&(buff.elemental_blast_critical_strike.up+buff.elemental_blast_haste.up+buff.elemental_blast_mastery.up=0)", "Elemental Blast if no buffs or at 3t, Earthquake to spread Lightning Rod otherwise" );
  aoe->add_action( "earthquake,if=buff.tempest.stack<2&lightning_rod<active_enemies&spell_targets.chain_lightning>=3+talent.elemental_blast&(buff.elemental_blast_critical_strike.up+buff.elemental_blast_haste.up+buff.elemental_blast_mastery.up>0|!talent.elemental_blast)" );
  aoe->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=buff.tempest.stack<2&spell_targets.chain_lightning=3" );
  aoe->add_action( "lava_burst,if=buff.purging_flames.up&(buff.lava_surge.up|cooldown.voltaic_blaze.remains<2)", "Spend Purging flames." );
  aoe->add_action( "lava_burst,if=buff.tempest.up&buff.lava_surge.up&talent.master_of_the_elements&spell_targets.chain_lightning=3", "[3t] Spend Lava Surge procs to buff Tempest with MotE." );
  aoe->add_action( "tempest,target_if=min:debuff.lightning_rod.remains,if=buff.master_of_the_elements.up", "[3t] Tempest if you have MotE." );
  aoe->add_action( "tempest,target_if=min:debuff.lightning_rod.remains,if=buff.stormkeeper.stack<4&buff.tempest.stack=2" );
  aoe->add_action( "chain_lightning,if=buff.stormkeeper.up&maelstrom.deficit>(spell_targets.chain_lightning>?5)*(2+(spell_targets.chain_lightning>?5)+2)" );
  aoe->add_action( "earthquake,if=!talent.elemental_blast&maelstrom.deficit<15+(buff.stormkeeper.up*(spell_targets.chain_lightning>?5)*(2+(spell_targets.chain_lightning>?5)))" );
  aoe->add_action( "elemental_blast" );
  aoe->add_action( "tempest,target_if=min:debuff.lightning_rod.remains" );
  aoe->add_action( "chain_lightning", "Filler spell. Always available. Always the bottom line." );
  aoe->add_action( "flame_shock,moving=1" );
  aoe->add_action( "voltaic_blaze,moving=1" );
  aoe->add_action( "frost_shock,moving=1" );

  single_target->add_action( "stormkeeper,if=cooldown.ascendance.remains>10|cooldown.ascendance.remains<gcd|fight_remains<20", "--- 1 and 2 TARGET ROTATION ---  Stormkeeper on CD, unless sub 10s hold for Asc or the fight is about to end." );
  single_target->add_action( "ancestral_swiftness" );
  single_target->add_action( "flame_shock,if=!buff.master_of_the_elements.up&((dot.flame_shock.refreshable&cooldown.ascendance.remains>5)|(buff.fire_elemental.up&buff.fire_elemental.remains<2))", "Maintain Flame shock, minor gain to refresh it when FE is about to fade." );
  single_target->add_action( "voltaic_blaze,if=!buff.master_of_the_elements.up&((dot.flame_shock.refreshable&cooldown.ascendance.remains>5)|(buff.fire_elemental.up&buff.fire_elemental.remains<2)|talent.purging_flames&spell_targets.chain_lightning=2)" );
  single_target->add_action( "ascendance,if=cooldown.stormkeeper.remains>15|fight_remains<20", "Ascendance on CD, unless SK can be sync'd with it." );
  single_target->add_action( "lava_burst,if=!buff.master_of_the_elements.up&maelstrom.deficit>15&(cooldown.lava_burst.charges_fractional>1.8|!buff.call_of_the_ancestors.up|spell_targets.chain_lightning=1)&(talent.master_of_the_elements|(talent.molten_wrath&cooldown.lava_burst.charges_fractional>0.8+talent.elemental_reverb)|buff.lava_surge.up|(talent.fusion_of_elements|talent.call_of_the_ancestors)&(!buff.storm_elemental.up|buff.wind_gust.stack=4)|buff.purging_flames.up&spell_targets.chain_lightning=2&(buff.lava_surge.up|cooldown.voltaic_blaze.remains<2))", "Lava Burst if any empowering it talent chosen OR to consume surge procs." );
  single_target->add_action( "tempest,if=buff.master_of_the_elements.up|!talent.master_of_the_elements", "Tempest and Lightning Bolt with SK if you have MotE." );
  single_target->add_action( "lightning_bolt,if=buff.stormkeeper.up&(buff.master_of_the_elements.up|!talent.master_of_the_elements)" );
  single_target->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains" );
  single_target->add_action( "earth_shock,target_if=min:debuff.lightning_rod.remains" );
  single_target->add_action( "tempest" );
  single_target->add_action( "chain_lightning,if=talent.call_of_the_ancestors&spell_targets.chain_lightning=2", "Filler spell. Always available. Always the bottom line." );
  single_target->add_action( "lightning_bolt" );
  single_target->add_action( "flame_shock,moving=1" );
  single_target->add_action( "voltaic_blaze,moving=1" );
  single_target->add_action( "frost_shock,moving=1" );
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
  precombat->add_action( "flametongue_weapon,if=talent.flametongue_weapon" );
  precombat->add_action( "lightning_shield" );
  precombat->add_action( "thunderstrike_ward" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=(trinket.1.has_use_buff|trinket.1.is.funhouse_lens)" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=(trinket.2.has_use_buff|trinket.2.is.funhouse_lens)" );
  precombat->add_action( "stormkeeper" );

  default_->add_action( "spiritwalkers_grace,moving=1,if=movement.distance>6", "Enable more movement." );
  default_->add_action( "wind_shear", "Interrupt of casts." );
  default_->add_action( "blood_fury" );
  default_->add_action( "berserking" );
  default_->add_action( "fireblood" );
  default_->add_action( "ancestral_call" );
  default_->add_action( "use_item,slot=trinket1,use_off_gcd=1,if=variable.trinket_1_buffs&(cooldown.ascendance.remains>trinket.1.cooldown.duration-5|cooldown.ascendance.ready&cooldown.stormkeeper.remains>15|fight_remains<21)", "Normal buff trinkets, mimic Ascendance activation conditions" );
  default_->add_action( "use_item,slot=trinket2,use_off_gcd=1,if=variable.trinket_2_buffs&(cooldown.ascendance.remains>trinket.2.cooldown.duration-5|cooldown.ascendance.ready&cooldown.stormkeeper.remains>15|fight_remains<21)" );
  default_->add_action( "use_item,slot=main_hand,use_off_gcd=1", "Normal weapons" );
  default_->add_action( "use_item,slot=trinket1,use_off_gcd=1,if=!variable.trinket_1_buffs&(cooldown.ascendance.remains>20|trinket.2.cooldown.remains>20)", "Dmg trinkets" );
  default_->add_action( "use_item,slot=trinket2,use_off_gcd=1,if=!variable.trinket_2_buffs&(cooldown.ascendance.remains>20|trinket.1.cooldown.remains>20)" );
  default_->add_action( "lightning_shield,if=buff.lightning_shield.down" );
  default_->add_action( "natures_swiftness" );
  default_->add_action( "invoke_external_buff,name=power_infusion", "Use Power Infusion on Cooldown." );
  default_->add_action( "potion,if=buff.bloodlust.up|cooldown.ascendance.ready&cooldown.stormkeeper.remains>15|fight_remains<31" );
  default_->add_action( "run_action_list,name=aoe,if=spell_targets.chain_lightning>=3" );
  default_->add_action( "run_action_list,name=single_target" );

  aoe->add_action( "stormkeeper,if=cooldown.ascendance.remains>10|cooldown.ascendance.remains<gcd|fight_remains<20", "--- 3+ TARGET ROTATION ---  Stormkeeper on CD, unless sub 10s hold for Asc or the fight is about to end." );
  aoe->add_action( "ancestral_swiftness" );
  aoe->add_action( "flame_shock,if=!buff.master_of_the_elements.up&((dot.flame_shock.refreshable&cooldown.ascendance.remains>5)|(buff.fire_elemental.up&buff.fire_elemental.remains<2))&talent.master_of_the_elements&talent.inferno_arc&spell_targets.chain_lightning=3", "[3t] Apply Flame shock on 3t for MotE and Inferno arc." );
  aoe->add_action( "voltaic_blaze,if=!buff.master_of_the_elements.up&((dot.flame_shock.refreshable&cooldown.ascendance.remains>5)|(buff.fire_elemental.up&buff.fire_elemental.remains<2)|talent.purging_flames)", "Apply Voltaic blaze for Inferno arc or Purging flames." );
  aoe->add_action( "ascendance,if=cooldown.stormkeeper.remains>15|fight_remains<20", "Ascendance on CD, unless SK can be sync'd with it." );
  aoe->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=buff.tempest.stack<2&(buff.elemental_blast_critical_strike.up+buff.elemental_blast_haste.up+buff.elemental_blast_mastery.up=0)", "Elemental Blast if no buffs or at 3t, Earthquake to spread Lightning Rod otherwise" );
  aoe->add_action( "earthquake,if=buff.tempest.stack<2&lightning_rod<active_enemies&spell_targets.chain_lightning>=3+talent.elemental_blast&(buff.elemental_blast_critical_strike.up+buff.elemental_blast_haste.up+buff.elemental_blast_mastery.up>0|!talent.elemental_blast)" );
  aoe->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains,if=buff.tempest.stack<2&spell_targets.chain_lightning=3" );
  aoe->add_action( "lava_burst,if=buff.purging_flames.up&(buff.lava_surge.up|cooldown.voltaic_blaze.remains<2)", "Spend Purging flames." );
  aoe->add_action( "lava_burst,if=buff.tempest.up&buff.lava_surge.up&talent.master_of_the_elements&spell_targets.chain_lightning=3", "[3t] Spend Lava Surge procs to buff Tempest with MotE." );
  aoe->add_action( "tempest,target_if=min:debuff.lightning_rod.remains,if=buff.master_of_the_elements.up", "[3t] Tempest if you have MotE." );
  aoe->add_action( "tempest,target_if=min:debuff.lightning_rod.remains,if=buff.stormkeeper.stack<4&buff.tempest.stack=2" );
  aoe->add_action( "chain_lightning,if=buff.stormkeeper.up&maelstrom.deficit>(spell_targets.chain_lightning>?5)*(2+(spell_targets.chain_lightning>?5)+2)" );
  aoe->add_action( "earthquake,if=!talent.elemental_blast&maelstrom.deficit<15+(buff.stormkeeper.up*(spell_targets.chain_lightning>?5)*(2+(spell_targets.chain_lightning>?5)))" );
  aoe->add_action( "elemental_blast" );
  aoe->add_action( "tempest,target_if=min:debuff.lightning_rod.remains" );
  aoe->add_action( "chain_lightning", "Filler spell. Always available. Always the bottom line." );
  aoe->add_action( "flame_shock,moving=1" );
  aoe->add_action( "voltaic_blaze,moving=1" );
  aoe->add_action( "frost_shock,moving=1" );

  single_target->add_action( "stormkeeper,if=cooldown.ascendance.remains>10|cooldown.ascendance.remains<gcd|fight_remains<20", "--- 1 and 2 TARGET ROTATION ---  Stormkeeper on CD, unless sub 10s hold for Asc or the fight is about to end." );
  single_target->add_action( "ancestral_swiftness" );
  single_target->add_action( "flame_shock,if=!buff.master_of_the_elements.up&((dot.flame_shock.refreshable&cooldown.ascendance.remains>5)|(buff.fire_elemental.up&buff.fire_elemental.remains<2))", "Maintain Flame shock, minor gain to refresh it when FE is about to fade." );
  single_target->add_action( "voltaic_blaze,if=!buff.master_of_the_elements.up&((dot.flame_shock.refreshable&cooldown.ascendance.remains>5)|(buff.fire_elemental.up&buff.fire_elemental.remains<2)|talent.purging_flames&spell_targets.chain_lightning=2)" );
  single_target->add_action( "ascendance,if=cooldown.stormkeeper.remains>15|fight_remains<20", "Ascendance on CD, unless SK can be sync'd with it." );
  single_target->add_action( "lava_burst,if=!buff.master_of_the_elements.up&maelstrom.deficit>15&(cooldown.lava_burst.charges_fractional>1.8|!buff.call_of_the_ancestors.up|spell_targets.chain_lightning=1)&(talent.master_of_the_elements|(talent.molten_wrath&cooldown.lava_burst.charges_fractional>0.8+talent.elemental_reverb)|buff.lava_surge.up|(talent.fusion_of_elements|talent.call_of_the_ancestors)&(!buff.storm_elemental.up|buff.wind_gust.stack=4)|buff.purging_flames.up&spell_targets.chain_lightning=2&(buff.lava_surge.up|cooldown.voltaic_blaze.remains<2))", "Lava Burst if any empowering it talent chosen OR to consume surge procs." );
  single_target->add_action( "tempest,if=buff.master_of_the_elements.up|!talent.master_of_the_elements", "Tempest and Lightning Bolt with SK if you have MotE." );
  single_target->add_action( "lightning_bolt,if=buff.stormkeeper.up&(buff.master_of_the_elements.up|!talent.master_of_the_elements)" );
  single_target->add_action( "elemental_blast,target_if=min:debuff.lightning_rod.remains" );
  single_target->add_action( "earth_shock,target_if=min:debuff.lightning_rod.remains" );
  single_target->add_action( "tempest" );
  single_target->add_action( "chain_lightning,if=talent.call_of_the_ancestors&spell_targets.chain_lightning=2", "Filler spell. Always available. Always the bottom line." );
  single_target->add_action( "lightning_bolt" );
  single_target->add_action( "flame_shock,moving=1" );
  single_target->add_action( "voltaic_blaze,moving=1" );
  single_target->add_action( "frost_shock,moving=1" );
}
//elemental_ptr_apl_end

} //namespace shaman_apl
