#include "class_modules/apl/apl_paladin.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace paladin_apl
{

//retribution_apl_start
void retribution( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cooldowns = p->get_action_priority_list( "cooldowns" );
  action_priority_list_t* finishers = p->get_action_priority_list( "finishers" );
  action_priority_list_t* generators = p->get_action_priority_list( "generators" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "shield_of_vengeance" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_buff.strength|trinket.1.has_buff.mastery|trinket.1.has_buff.versatility|trinket.1.has_buff.haste|trinket.1.has_buff.crit" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_buff.strength|trinket.2.has_buff.mastery|trinket.2.has_buff.versatility|trinket.2.has_buff.haste|trinket.2.has_buff.crit" );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_1_buffs&(trinket.1.cooldown.duration%%cooldown.crusade.duration=0|cooldown.crusade.duration%%trinket.1.cooldown.duration=0|trinket.1.cooldown.duration%%cooldown.avenging_wrath.duration=0|cooldown.avenging_wrath.duration%%trinket.1.cooldown.duration=0)" );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_2_buffs&(trinket.2.cooldown.duration%%cooldown.crusade.duration=0|cooldown.crusade.duration%%trinket.2.cooldown.duration=0|trinket.2.cooldown.duration%%cooldown.avenging_wrath.duration=0|cooldown.avenging_wrath.duration%%trinket.2.cooldown.duration=0)" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs|variable.trinket_2_buffs&((trinket.2.cooldown.duration%trinket.2.proc.any_dps.duration)*(1.5+trinket.2.has_buff.strength)*(variable.trinket_2_sync))>((trinket.1.cooldown.duration%trinket.1.proc.any_dps.duration)*(1.5+trinket.1.has_buff.strength)*(variable.trinket_1_sync))" );

  default_->add_action( "auto_attack" );
  default_->add_action( "rebuke" );
  default_->add_action( "call_action_list,name=cooldowns" );
  default_->add_action( "call_action_list,name=generators" );

  cooldowns->add_action( "potion,if=buff.avenging_wrath.up|buff.crusade.up|debuff.execution_sentence.up|fight_remains<30" );
  cooldowns->add_action( "invoke_external_buff,name=power_infusion,if=buff.avenging_wrath.up|buff.crusade.up" );
  cooldowns->add_action( "lights_judgment,if=spell_targets.lights_judgment>=2|!raid_event.adds.exists|raid_event.adds.in>75|raid_event.adds.up" );
  cooldowns->add_action( "fireblood,if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack=10" );
  cooldowns->add_action( "use_item,name=algethar_puzzle_box,if=(cooldown.avenging_wrath.remains<5&!talent.crusade|cooldown.crusade.remains<5&talent.crusade)&(holy_power>=4&time<5|holy_power>=3&time>5)" );
  cooldowns->add_action( "use_item,slot=trinket1,if=(buff.avenging_wrath.up&cooldown.avenging_wrath.remains>40|buff.crusade.up&buff.crusade.stack=10)&(!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1)|trinket.1.proc.any_dps.duration>=fight_remains" );
  cooldowns->add_action( "use_item,slot=trinket2,if=(buff.avenging_wrath.up&cooldown.avenging_wrath.remains>40|buff.crusade.up&buff.crusade.stack=10)&(!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2)|trinket.2.proc.any_dps.duration>=fight_remains" );
  cooldowns->add_action( "use_item,slot=trinket1,if=!variable.trinket_1_buffs&(trinket.2.cooldown.remains|!variable.trinket_2_buffs|!buff.crusade.up&cooldown.crusade.remains>20|!buff.avenging_wrath.up&cooldown.avenging_wrath.remains>20)" );
  cooldowns->add_action( "use_item,slot=trinket2,if=!variable.trinket_2_buffs&(trinket.1.cooldown.remains|!variable.trinket_1_buffs|!buff.crusade.up&cooldown.crusade.remains>20|!buff.avenging_wrath.up&cooldown.avenging_wrath.remains>20)" );
  cooldowns->add_action( "use_item,name=shadowed_razing_annihilator,if=(trinket.2.cooldown.remains|!variable.trinket_2_buffs)&(trinket.2.cooldown.remains|!variable.trinket_2_buffs)" );
  cooldowns->add_action( "use_item,name=fyralath_the_dreamrender,if=dot.mark_of_fyralath.ticking&!buff.avenging_wrath.up&!buff.crusade.up" );
  cooldowns->add_action( "shield_of_vengeance,if=fight_remains>15&(!talent.execution_sentence|!debuff.execution_sentence.up)" );
  cooldowns->add_action( "execution_sentence,if=(!buff.crusade.up&cooldown.crusade.remains>15|buff.crusade.stack=10|cooldown.avenging_wrath.remains<0.75|cooldown.avenging_wrath.remains>15|talent.radiant_glory)&(holy_power>=4&time<5|holy_power>=3&time>5|holy_power>=2&talent.divine_auxiliary)&(target.time_to_die>8&!talent.executioners_will|target.time_to_die>12)" );
  cooldowns->add_action( "avenging_wrath,if=(holy_power>=4&time<5|holy_power>=3&time>5|holy_power>=2&talent.divine_auxiliary&(cooldown.execution_sentence.remains=0|cooldown.final_reckoning.remains=0))&(!raid_event.adds.up|target.time_to_die>10)" );
  cooldowns->add_action( "crusade,if=holy_power>=5&time<5|holy_power>=3&time>5" );
  cooldowns->add_action( "final_reckoning,if=(holy_power>=4&time<8|holy_power>=3&time>=8|holy_power>=2&talent.divine_auxiliary)&(cooldown.avenging_wrath.remains>10|cooldown.crusade.remains&(!buff.crusade.up|buff.crusade.stack>=10)|talent.radiant_glory&(buff.avenging_wrath.up|buff.crusade.up))&(!raid_event.adds.exists|raid_event.adds.up|raid_event.adds.in>40)" );

  finishers->add_action( "variable,name=ds_castable,value=(spell_targets.divine_storm>=3|spell_targets.divine_storm>=2&!talent.divine_arbiter|buff.empyrean_power.up)&!buff.empyrean_legacy.up&!(buff.divine_arbiter.up&buff.divine_arbiter.stack>24)" );
  finishers->add_action( "divine_hammer" );
  finishers->add_action( "divine_storm,if=variable.ds_castable&(!talent.crusade|cooldown.crusade.remains>gcd*3|buff.crusade.up&buff.crusade.stack<10|talent.radiant_glory)" );
  finishers->add_action( "justicars_vengeance,if=!talent.crusade|cooldown.crusade.remains>gcd*3|buff.crusade.up&buff.crusade.stack<10|talent.radiant_glory" );
  finishers->add_action( "templars_verdict,if=!talent.crusade|cooldown.crusade.remains>gcd*3|buff.crusade.up&buff.crusade.stack<10|talent.radiant_glory" );

  generators->add_action( "call_action_list,name=finishers,if=holy_power=5|buff.echoes_of_wrath.up&set_bonus.tier31_4pc&talent.crusading_strikes|(debuff.judgment.up|holy_power=4)&buff.divine_resonance.up&!set_bonus.tier31_2pc" );
  generators->add_action( "wake_of_ashes,if=holy_power<=2&(cooldown.avenging_wrath.remains>6|cooldown.crusade.remains>6|talent.radiant_glory)&(!talent.execution_sentence|cooldown.execution_sentence.remains>4|target.time_to_die<8)&(!raid_event.adds.exists|raid_event.adds.in>20|raid_event.adds.up)" );
  generators->add_action( "blade_of_justice,if=!dot.expurgation.ticking&set_bonus.tier31_2pc" );
  generators->add_action( "divine_toll,if=holy_power<=2&(!raid_event.adds.exists|raid_event.adds.in>30|raid_event.adds.up)&(cooldown.avenging_wrath.remains>15|cooldown.crusade.remains>15|talent.radiant_glory|fight_remains<8)" );
  generators->add_action( "judgment,if=dot.expurgation.ticking&!buff.echoes_of_wrath.up&set_bonus.tier31_2pc" );
  generators->add_action( "call_action_list,name=finishers,if=holy_power>=3&buff.crusade.up&buff.crusade.stack<10" );
  generators->add_action( "templar_slash,if=buff.templar_strikes.remains<gcd&spell_targets.divine_storm>=2" );
  generators->add_action( "blade_of_justice,if=(holy_power<=3|!talent.holy_blade)&(spell_targets.divine_storm>=2&!talent.crusading_strikes|spell_targets.divine_storm>=4)" );
  generators->add_action( "hammer_of_wrath,if=(spell_targets.divine_storm<2|!talent.blessed_champion)&(holy_power<=3|target.health.pct>20|!talent.vanguards_momentum)" );
  generators->add_action( "templar_slash,if=buff.templar_strikes.remains<gcd" );
  generators->add_action( "judgment,if=!debuff.judgment.up&(holy_power<=3|!talent.boundless_judgment)" );
  generators->add_action( "blade_of_justice,if=holy_power<=3|!talent.holy_blade" );
  generators->add_action( "judgment,if=holy_power<=3|!talent.boundless_judgment" );
  generators->add_action( "call_action_list,name=finishers,if=(target.health.pct<=20|buff.avenging_wrath.up|buff.crusade.up|buff.empyrean_power.up)" );
  generators->add_action( "crusader_strike,if=cooldown.crusader_strike.charges_fractional>=1.75&(holy_power<=2|holy_power<=3&cooldown.blade_of_justice.remains>gcd*2|holy_power=4&cooldown.blade_of_justice.remains>gcd*2&cooldown.judgment.remains>gcd*2)" );
  generators->add_action( "call_action_list,name=finishers" );
  generators->add_action( "templar_slash" );
  generators->add_action( "templar_strike" );
  generators->add_action( "hammer_of_wrath,if=holy_power<=3|target.health.pct>20|!talent.vanguards_momentum" );
  generators->add_action( "crusader_strike" );
  generators->add_action( "arcane_torrent" );
}
//retribution_apl_end

//protection_apl_start
void protection( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cooldowns = p->get_action_priority_list( "cooldowns" );
  action_priority_list_t* standard = p->get_action_priority_list( "standard" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "devotion_aura" );
  precombat->add_action( "lights_judgment" );
  precombat->add_action( "arcane_torrent" );
  precombat->add_action( "consecration" );
  precombat->add_action( "variable,name=trinket_sync_slot,value=1,if=trinket.1.has_cooldown&trinket.1.has_stat.any_dps&(!trinket.2.has_stat.any_dps|trinket.1.cooldown.duration>=trinket.2.cooldown.duration)|!trinket.2.has_cooldown" );
  precombat->add_action( "variable,name=trinket_sync_slot,value=2,if=trinket.2.has_cooldown&trinket.2.has_stat.any_dps&(!trinket.1.has_stat.any_dps|trinket.2.cooldown.duration>trinket.1.cooldown.duration)|!trinket.1.has_cooldown" );

  default_->add_action( "auto_attack" );
  default_->add_action( "call_action_list,name=cooldowns" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "call_action_list,name=standard" );

  cooldowns->add_action( "lights_judgment,if=spell_targets.lights_judgment>=2|!raid_event.adds.exists|raid_event.adds.in>75|raid_event.adds.up" );
  cooldowns->add_action( "avenging_wrath" );
  cooldowns->add_action( "potion,if=buff.avenging_wrath.up" );
  cooldowns->add_action( "moment_of_glory,if=(buff.avenging_wrath.remains<15|(time>10|(cooldown.avenging_wrath.remains>15))&(cooldown.avengers_shield.remains&cooldown.judgment.remains&cooldown.hammer_of_wrath.remains))" );
  cooldowns->add_action( "divine_toll,if=spell_targets.shield_of_the_righteous>=3" );
  cooldowns->add_action( "bastion_of_light,if=buff.avenging_wrath.up|cooldown.avenging_wrath.remains<=30" );
  cooldowns->add_action( "invoke_external_buff,name=power_infusion,if=buff.avenging_wrath.up" );
  cooldowns->add_action( "fireblood,if=buff.avenging_wrath.remains>8" );

  standard->add_action( "hammer_of_light,if=!buff.shake_the_heavens.up|(buff.shake_the_heavens.up&buff.shake_the_heavens.remains<2.4)|(buff.hammer_of_light_free.up&buff.hammer_of_light_free.remains<3)|(buff.hammer_of_light_ready.up&buff.hammer_of_light_ready.remains<3)" );
  standard->add_action( "shield_of_the_righteous,if=(((!talent.righteous_protector.enabled|cooldown.righteous_protector_icd.remains=0)&holy_power>2)|buff.bastion_of_light.up|buff.divine_purpose.up)&!buff.hammer_of_light_ready.up", "Use Shield of the Righteous according to Righteous Protector's ICD, but use it asap if it's a free proc (Bugged interaction, this ignores ICD)." );
  standard->add_action( "holy_armaments,if=next_armament=sacred_weapon&(!buff.sacred_weapon.up|(buff.sacred_weapon.remains<6&!buff.avenging_wrath.up))" );
  standard->add_action( "judgment,target_if=min:debuff.judgment.remains,if=spell_targets.shield_of_the_righteous>3&buff.bulwark_of_righteous_fury.stack>=3&holy_power<3" );
  standard->add_action( "eye_of_tyr,if=talent.lights_guidance.enabled" );
  standard->add_action( "hammer_of_wrath" );
  standard->add_action( "judgment,target_if=min:debuff.judgment.remains,if=charges>=2|full_recharge_time<=gcd.max" );
  standard->add_action( "avengers_shield,if=(spell_targets.avengers_shield>2|buff.moment_of_glory.up)&!buff.shake_the_heavens.up" );
  standard->add_action( "divine_toll,if=(!raid_event.adds.exists|raid_event.adds.in>10)" );
  standard->add_action( "judgment,target_if=min:debuff.judgment.remains" );
  standard->add_action( "avengers_shield" );
  standard->add_action( "consecration,if=!consecration.up" );
  standard->add_action( "eye_of_tyr,if=talent.inmost_light.enabled&raid_event.adds.in>=45|spell_targets.shield_of_the_righteous>=3" );
  standard->add_action( "holy_armaments,if=next_armament=holy_bulwark&charges=2&cooldown.avenging_wrath.remains>=30", "Takes advantage of a bug which guarantees Divine Inspiration to give a sacred weapon if the current armament is bulwark" );
  standard->add_action( "holy_armaments,if=next_armament=sacred_weapon&(buff.sacred_weapon.remains<6&buff.avenging_wrath.up)" );
  standard->add_action( "blessed_hammer" );
  standard->add_action( "hammer_of_the_righteous" );
  standard->add_action( "crusader_strike" );
  standard->add_action( "eye_of_tyr,if=!talent.inmost_light.enabled&raid_event.adds.in>=60|spell_targets.shield_of_the_righteous>=3" );
  standard->add_action( "word_of_glory,if=buff.shining_light_free.up" );
  standard->add_action( "arcane_torrent,if=holy_power<5" );
  standard->add_action( "consecration" );

  trinkets->add_action( "use_items,slots=trinket1,if=(variable.trinket_sync_slot=1&(buff.avenging_wrath.up|fight_remains<=40)|(variable.trinket_sync_slot=2&(!trinket.2.cooldown.ready|!buff.avenging_wrath.up))|!variable.trinket_sync_slot)" );
  trinkets->add_action( "use_items,slots=trinket2,if=(variable.trinket_sync_slot=2&(buff.avenging_wrath.up|fight_remains<=40)|(variable.trinket_sync_slot=1&(!trinket.1.cooldown.ready|!buff.avenging_wrath.up))|!variable.trinket_sync_slot)" );
}
//protection_apl_end
}  // namespace paladin_apl
