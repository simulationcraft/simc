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
  action_priority_list_t* es_fr_active = p->get_action_priority_list( "es_fr_active" );
  action_priority_list_t* es_fr_pooling = p->get_action_priority_list( "es_fr_pooling" );
  action_priority_list_t* finishers = p->get_action_priority_list( "finishers" );
  action_priority_list_t* generators = p->get_action_priority_list( "generators" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "retribution_aura" );
  precombat->add_action( "arcane_torrent,if=talent.final_reckoning&talent.seraphim" );
  precombat->add_action( "shield_of_vengeance" );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=trinket.1.has_use_buff&(trinket.1.cooldown.duration%%cooldown.crusade.duration=0|cooldown.crusade.duration%%trinket.1.cooldown.duration=0|trinket.1.cooldown.duration%%cooldown.avenging_wrath.duration=0|cooldown.avenging_wrath.duration%%trinket.1.cooldown.duration=0)" );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=trinket.2.has_use_buff&(trinket.2.cooldown.duration%%cooldown.crusade.duration=0|cooldown.crusade.duration%%trinket.2.cooldown.duration=0|trinket.2.cooldown.duration%%cooldown.avenging_wrath.duration=0|cooldown.avenging_wrath.duration%%trinket.2.cooldown.duration=0)" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!trinket.1.has_use_buff&trinket.2.has_use_buff|trinket.2.has_use_buff&((trinket.2.cooldown.duration%trinket.2.proc.any_dps.duration)*(1.5+trinket.2.has_buff.strength)*(variable.trinket_2_sync))>((trinket.1.cooldown.duration%trinket.1.proc.any_dps.duration)*(1.5+trinket.1.has_buff.strength)*(variable.trinket_1_sync))" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_buff.strength|trinket.1.has_buff.mastery|trinket.1.has_buff.versatility|trinket.1.has_buff.haste|trinket.1.has_buff.crit" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_buff.strength|trinket.2.has_buff.mastery|trinket.2.has_buff.versatility|trinket.2.has_buff.haste|trinket.2.has_buff.crit" );

  default_->add_action( "auto_attack" );
  default_->add_action( "rebuke" );
  default_->add_action( "call_action_list,name=cooldowns" );
  default_->add_action( "call_action_list,name=es_fr_pooling,if=(!raid_event.adds.exists|raid_event.adds.up|raid_event.adds.in<9|raid_event.adds.in>30)&(talent.execution_sentence&cooldown.execution_sentence.remains<9&spell_targets.divine_storm<5|talent.final_reckoning&cooldown.final_reckoning.remains<9)&target.time_to_die>8" );
  default_->add_action( "call_action_list,name=es_fr_active,if=debuff.execution_sentence.up|debuff.final_reckoning.up" );
  default_->add_action( "call_action_list,name=generators" );

  cooldowns->add_action( "potion,if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack=10|fight_remains<25" );
  cooldowns->add_action( "lights_judgment,if=spell_targets.lights_judgment>=2|!raid_event.adds.exists|raid_event.adds.in>75|raid_event.adds.up" );
  cooldowns->add_action( "fireblood,if=(buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack=10)&!talent.execution_sentence" );
  cooldowns->add_action( "use_item,slot=trinket1,if=(buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack=10)&(!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1)|trinket.1.proc.any_dps.duration>=fight_remains" );
  cooldowns->add_action( "use_item,slot=trinket2,if=(buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack=10)&(!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2)|trinket.2.proc.any_dps.duration>=fight_remains" );
  cooldowns->add_action( "use_item,slot=trinket1,if=!variable.trinket_1_buffs&(trinket.2.cooldown.remains|!variable.trinket_2_buffs|cooldown.crusade.remains>20|cooldown.avenging_wrath.remains>20)" );
  cooldowns->add_action( "use_item,slot=trinket2,if=!variable.trinket_2_buffs&(trinket.1.cooldown.remains|!variable.trinket_1_buffs|cooldown.crusade.remains>20|cooldown.avenging_wrath.remains>20)" );
  cooldowns->add_action( "shield_of_vengeance,if=(!talent.execution_sentence|cooldown.execution_sentence.remains<52)&fight_remains>15" );
  cooldowns->add_action( "avenging_wrath,if=((holy_power>=4&time<5|holy_power>=3&time>5)|talent.holy_avenger&cooldown.holy_avenger.remains=0)&(!talent.seraphim|!talent.final_reckoning|cooldown.seraphim.remains>0)" );
  cooldowns->add_action( "crusade,if=holy_power>=5&time<5|holy_power>=3&time>5" );
  cooldowns->add_action( "holy_avenger,if=time_to_hpg=0&holy_power<=2&(buff.avenging_wrath.up|talent.crusade&(cooldown.crusade.remains=0|buff.crusade.up)|fight_remains<20)" );
  cooldowns->add_action( "final_reckoning,if=(holy_power>=4&time<8|holy_power>=3&time>=8)&(cooldown.avenging_wrath.remains>gcd|cooldown.crusade.remains&(!buff.crusade.up|buff.crusade.stack>=10))&(time_to_hpg>0|holy_power=5)&(!talent.seraphim|buff.seraphim.up)&(!raid_event.adds.exists|raid_event.adds.up|raid_event.adds.in>40)&(!buff.avenging_wrath.up|holy_power=5|cooldown.hammer_of_wrath.remains)" );

  es_fr_active->add_action( "fireblood" );
  es_fr_active->add_action( "call_action_list,name=finishers,if=holy_power=5|debuff.judgment.up|debuff.final_reckoning.up&(debuff.final_reckoning.remains<gcd.max|spell_targets.divine_storm>=2&!talent.execution_sentence)|debuff.execution_sentence.up&debuff.execution_sentence.remains<gcd.max" );
  es_fr_active->add_action( "divine_toll,if=holy_power<=2" );
  es_fr_active->add_action( "wake_of_ashes,if=holy_power<=2&(debuff.final_reckoning.up&debuff.final_reckoning.remains<gcd*2&!talent.divine_resonance|debuff.execution_sentence.up&debuff.execution_sentence.remains<gcd|spell_targets.divine_storm>=5&talent.divine_resonance&talent.execution_sentence)" );
  es_fr_active->add_action( "blade_of_justice,if=talent.expurgation&(!talent.divine_resonance&holy_power<=3|holy_power<=2)" );
  es_fr_active->add_action( "judgment,if=!debuff.judgment.up&holy_power>=2" );
  es_fr_active->add_action( "call_action_list,name=finishers" );
  es_fr_active->add_action( "wake_of_ashes,if=holy_power<=2" );
  es_fr_active->add_action( "blade_of_justice,if=holy_power<=3" );
  es_fr_active->add_action( "judgment,if=!debuff.judgment.up" );
  es_fr_active->add_action( "hammer_of_wrath" );
  es_fr_active->add_action( "crusader_strike" );
  es_fr_active->add_action( "arcane_torrent" );
  es_fr_active->add_action( "exorcism" );
  es_fr_active->add_action( "consecration" );

  es_fr_pooling->add_action( "seraphim,if=holy_power=5&(!talent.final_reckoning|cooldown.final_reckoning.remains<=gcd*3)&(!talent.execution_sentence|cooldown.execution_sentence.remains<=gcd*3|talent.final_reckoning)" );
  es_fr_pooling->add_action( "call_action_list,name=finishers,if=holy_power=5|debuff.final_reckoning.up|buff.crusade.up&buff.crusade.stack<10" );
  es_fr_pooling->add_action( "hammer_of_wrath,if=talent.vanguards_momentum" );
  es_fr_pooling->add_action( "wake_of_ashes,if=holy_power<=2&talent.ashes_to_dust&(cooldown.crusade.remains|cooldown.avenging_wrath.remains)" );
  es_fr_pooling->add_action( "blade_of_justice,if=holy_power<=3" );
  es_fr_pooling->add_action( "judgment,if=!debuff.judgment.up" );
  es_fr_pooling->add_action( "hammer_of_wrath" );
  es_fr_pooling->add_action( "crusader_strike,if=cooldown.crusader_strike.charges_fractional>=1.75&(holy_power<=2|holy_power<=3&cooldown.blade_of_justice.remains>gcd*2|holy_power=4&cooldown.blade_of_justice.remains>gcd*2&cooldown.judgment.remains>gcd*2)" );
  es_fr_pooling->add_action( "seraphim,if=!talent.final_reckoning&cooldown.execution_sentence.remains<=gcd*3" );
  es_fr_pooling->add_action( "call_action_list,name=finishers" );
  es_fr_pooling->add_action( "crusader_strike" );
  es_fr_pooling->add_action( "arcane_torrent,if=holy_power<=4" );
  es_fr_pooling->add_action( "exorcism" );
  es_fr_pooling->add_action( "seraphim,if=(!talent.final_reckoning|cooldown.final_reckoning.remains<=gcd*3)&(!talent.execution_sentence|cooldown.execution_sentence.remains<=gcd*3|talent.final_reckoning)" );
  es_fr_pooling->add_action( "consecration" );

  finishers->add_action( "variable,name=ds_castable,value=spell_targets.divine_storm>=2|buff.empyrean_power.up&!debuff.judgment.up&!buff.divine_purpose.up|buff.crusade.up&buff.crusade.stack<10&buff.empyrean_legacy.up&!talent.justicars_vengeance" );
  finishers->add_action( "seraphim,if=(cooldown.avenging_wrath.remains>15|cooldown.crusade.remains>15)&!talent.final_reckoning&(!talent.execution_sentence|spell_targets.divine_storm>=5)&(!raid_event.adds.exists|raid_event.adds.in>40|raid_event.adds.in<gcd|raid_event.adds.up)|fight_remains<15&fight_remains>5|buff.crusade.up&buff.crusade.stack<10" );
  finishers->add_action( "execution_sentence,if=(buff.crusade.down&cooldown.crusade.remains>10|buff.crusade.stack>=3|cooldown.avenging_wrath.remains>10)&(!talent.final_reckoning|cooldown.final_reckoning.remains>10)&target.time_to_die>8&(spell_targets.divine_storm<5|talent.executioners_wrath)" );
  finishers->add_action( "radiant_decree,if=(buff.crusade.down&cooldown.crusade.remains>5|buff.crusade.stack>=3|cooldown.avenging_wrath.remains>5)&(!talent.final_reckoning|cooldown.final_reckoning.remains>5)" );
  finishers->add_action( "divine_storm,if=variable.ds_castable&(!buff.empyrean_legacy.up|buff.crusade.up&buff.crusade.stack<10)&((!talent.crusade|cooldown.crusade.remains>gcd*3)&(!talent.execution_sentence|cooldown.execution_sentence.remains>gcd*6|cooldown.execution_sentence.remains>gcd*4&holy_power>=4|target.time_to_die<8|spell_targets.divine_storm>=5|!talent.seraphim&cooldown.execution_sentence.remains>gcd*2)&(!talent.final_reckoning|cooldown.final_reckoning.remains>gcd*6|cooldown.final_reckoning.remains>gcd*4&holy_power>=4|!talent.seraphim&cooldown.final_reckoning.remains>gcd*2)|talent.holy_avenger&cooldown.holy_avenger.remains<gcd*3|buff.holy_avenger.up|buff.crusade.up&buff.crusade.stack<10)" );
  finishers->add_action( "justicars_vengeance,if=((!talent.crusade|cooldown.crusade.remains>gcd*3)&(!talent.execution_sentence|cooldown.execution_sentence.remains>gcd*6|cooldown.execution_sentence.remains>gcd*4&holy_power>=4|target.time_to_die<8|!talent.seraphim&cooldown.execution_sentence.remains>gcd*2)&(!talent.final_reckoning|cooldown.final_reckoning.remains>gcd*6|cooldown.final_reckoning.remains>gcd*4&holy_power>=4|!talent.seraphim&cooldown.final_reckoning.remains>gcd*2)|talent.holy_avenger&cooldown.holy_avenger.remains<gcd*3|buff.holy_avenger.up|buff.crusade.up&buff.crusade.stack<10)&!buff.empyrean_legacy.up" );
  finishers->add_action( "templars_verdict,if=(!talent.crusade|cooldown.crusade.remains>gcd*3)&(!talent.execution_sentence|cooldown.execution_sentence.remains>gcd*6|cooldown.execution_sentence.remains>gcd*4&holy_power>=4|target.time_to_die<8|!talent.seraphim&cooldown.execution_sentence.remains>gcd*2)&(!talent.final_reckoning|cooldown.final_reckoning.remains>gcd*6|cooldown.final_reckoning.remains>gcd*4&holy_power>=4|!talent.seraphim&cooldown.final_reckoning.remains>gcd*2)|talent.holy_avenger&cooldown.holy_avenger.remains<gcd*3|buff.holy_avenger.up|buff.crusade.up&buff.crusade.stack<10" );

  generators->add_action( "call_action_list,name=finishers,if=holy_power=5|(debuff.judgment.up|holy_power=4)&buff.divine_resonance.up|buff.holy_avenger.up" );
  generators->add_action( "hammer_of_wrath,if=talent.zealots_paragon" );
  generators->add_action( "wake_of_ashes,if=holy_power<=2&talent.ashes_to_dust&(cooldown.avenging_wrath.remains|cooldown.crusade.remains)" );
  generators->add_action( "divine_toll,if=holy_power<=2&!debuff.judgment.up&(!talent.seraphim|buff.seraphim.up)&(!raid_event.adds.exists|raid_event.adds.in>30|raid_event.adds.up)&!talent.final_reckoning&(!talent.execution_sentence|fight_remains<8|spell_targets.divine_storm>=5)&(cooldown.avenging_wrath.remains>15|cooldown.crusade.remains>15|fight_remains<8)" );
  generators->add_action( "judgment,if=!debuff.judgment.up&holy_power>=2" );
  generators->add_action( "wake_of_ashes,if=(holy_power=0|holy_power<=2&cooldown.blade_of_justice.remains>gcd*2)&(!raid_event.adds.exists|raid_event.adds.in>20|raid_event.adds.up)&(!talent.seraphim|cooldown.seraphim.remains>5)&(!talent.execution_sentence|cooldown.execution_sentence.remains>15|target.time_to_die<8|spell_targets.divine_storm>=5)&(!talent.final_reckoning|cooldown.final_reckoning.remains>15|fight_remains<8)&(cooldown.avenging_wrath.remains|cooldown.crusade.remains)" );
  generators->add_action( "call_action_list,name=finishers,if=holy_power>=3&buff.crusade.up&buff.crusade.stack<10" );
  generators->add_action( "exorcism" );
  generators->add_action( "judgment,if=!debuff.judgment.up" );
  generators->add_action( "hammer_of_wrath" );
  generators->add_action( "blade_of_justice,if=holy_power<=3" );
  generators->add_action( "call_action_list,name=finishers,if=(target.health.pct<=20|buff.avenging_wrath.up|buff.crusade.up|buff.empyrean_power.up)" );
  generators->add_action( "consecration,if=!consecration.up&spell_targets.divine_storm>=2" );
  generators->add_action( "crusader_strike,if=cooldown.crusader_strike.charges_fractional>=1.75&(holy_power<=2|holy_power<=3&cooldown.blade_of_justice.remains>gcd*2|holy_power=4&cooldown.blade_of_justice.remains>gcd*2&cooldown.judgment.remains>gcd*2)" );
  generators->add_action( "call_action_list,name=finishers" );
  generators->add_action( "consecration,if=!consecration.up" );
  generators->add_action( "crusader_strike" );
  generators->add_action( "arcane_torrent" );
  generators->add_action( "consecration" );
}
//retribution_apl_end

//protection_apl_start
void protection( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cooldowns = p->get_action_priority_list( "cooldowns" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );
  action_priority_list_t* standard = p->get_action_priority_list( "standard" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "lights_judgment" );
  precombat->add_action( "arcane_torrent" );
  precombat->add_action( "consecration" );
  // TODO: Trinkets break if you have no MoG talented
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=trinket.1.has_use_buff&(talent.moment_of_glory&trinket.1.cooldown.duration%%cooldown.moment_of_glory.duration=0|(talent.avenging_wrath&!talent.moment_of_glory)&trinket.1.cooldown.duration%%cooldown.avenging_wrath.duration<=20)", "Evaluates a trinkets cooldown, divided by moment of glory or avenging wraths's cooldown. If it's value has no remainder return 1, else return 0.5." );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=trinket.2.has_use_buff&(talent.moment_of_glory&trinket.2.cooldown.duration%%cooldown.moment_of_glory.duration=0|(talent.avenging_wrath&!talent.moment_of_glory)&trinket.2.cooldown.duration%%cooldown.avenging_wrath.duration<=20)" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!trinket.1.has_use_buff&trinket.2.has_use_buff|trinket.2.has_use_buff&((trinket.2.cooldown.duration%trinket.2.proc.any_dps.duration)*(1.5+trinket.2.has_buff.strength)*(variable.trinket_2_sync))>((trinket.1.cooldown.duration%trinket.1.proc.any_dps.duration)*(1.5+trinket.1.has_buff.strength)*(variable.trinket_1_sync))" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_buff.strength|trinket.1.has_buff.mastery|trinket.1.has_buff.versatility|trinket.1.has_buff.haste|trinket.1.has_buff.crit" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_buff.strength|trinket.2.has_buff.mastery|trinket.2.has_buff.versatility|trinket.2.has_buff.haste|trinket.2.has_buff.crit" );

  default_->add_action( "auto_attack" );
  default_->add_action( "variable,name=might_or_sentinel,value=(!talent.avenging_wrath.enabled&(!talent.avenging_wrath_might.enabled|talent.sentinel_enabled))" );
  default_->add_action( "call_action_list,name=cooldowns" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "call_action_list,name=standard" );

  cooldowns->add_action( "seraphim" );
  cooldowns->add_action( "avenging_wrath,if=(buff.seraphim.up|!talent.seraphim.enabled)" );
  cooldowns->add_action( "sentinel,if=(buff.seraphim.up|!talent.seraphim.enabled)" );
  cooldowns->add_action( "potion,if=buff.avenging_wrath.up|buff.sentinel.up|variable.might_or_sentinel" );
  cooldowns->add_action( "moment_of_glory,if=(buff.avenging_wrath.remains<15|buff.sentinel.remains<15|(time>10|(cooldown.avenging_wrath.remains>15|cooldown.sentinel.remains>15))&(cooldown.avengers_shield.remains&cooldown.judgment.remains&cooldown.hammer_of_wrath.remains))" );
  cooldowns->add_action( "holy_avenger,if=buff.avenging_wrath.up|buff.sentinel.up|variable.might_or_sentinel|cooldown.avenging_wrath.remains>60|cooldown.sentinel.remains>60" );
  cooldowns->add_action( "bastion_of_light,if=buff.avenging_wrath.up|buff.sentinel.up" );

  trinkets->add_action( "use_item,slot=trinket1,if=buff.moment_of_glory.up&(!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1)|trinket.1.proc.any_dps.duration>=fight_remains" );
  trinkets->add_action( "use_item,slot=trinket2,if=buff.moment_of_glory.up&(!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2)|trinket.2.proc.any_dps.duration>=fight_remains" );
  trinkets->add_action( "use_item,slot=trinket1,if=!variable.trinket_1_buffs&(trinket.2.cooldown.remains|!variable.trinket_2_buffs|cooldown.moment_of_glory.remains>20)" );
  trinkets->add_action( "use_item,slot=trinket2,if=!variable.trinket_2_buffs&(trinket.1.cooldown.remains|!variable.trinket_1_buffs|cooldown.moment_of_glory.remains>20)" );

  standard->add_action( "shield_of_the_righteous,if=(cooldown.seraphim.remains>=5|!talent.seraphim.enabled)&(((holy_power=3&!buff.blessing_of_dusk.up&!buff.holy_avenger.up)|(holy_power=5)|buff.bastion_of_light.up|buff.divine_purpose.up))" );
  standard->add_action( "avengers_shield,if=buff.moment_of_glory.up|!talent.moment_of_glory.enabled" );
  standard->add_action( "hammer_of_wrath,if=buff.avenging_wrath.up|buff.sentinel.up|(!talent.avenging_wrath.enabled&(!talent.avenging_wrath_might.enabled|talent.sentinel.enabled))" );
  standard->add_action( "judgment,target_if=min:debuff.judgment.remains,if=charges=2|!talent.crusaders_judgment.enabled" );
  standard->add_action( "divine_toll,if=time>20|((!talent.seraphim.enabled|buff.seraphim.up)&(buff.avenging_wrath.up|buff.sentinel.up|(!talent.avenging_wrath.enabled&(!talent.avenging_wrath_might.enabled|talent.sentinel.enabled)))&(buff.moment_of_glory.up|!talent.moment_of_glory.enabled))" );
  standard->add_action( "avengers_shield" );
  standard->add_action( "hammer_of_wrath" );
  standard->add_action( "judgment,target_if=min:debuff.judgment.remains" );
  standard->add_action( "consecration,if=!consecration.up" );
  standard->add_action( "eye_of_tyr" );
  standard->add_action( "blessed_hammer" );
  standard->add_action( "hammer_of_the_righteous" );
  standard->add_action( "crusader_strike" );
  standard->add_action( "word_of_glory,if=buff.shining_light_free.up" );
  standard->add_action( "consecration" );
}
//protection_apl_end
}  // namespace paladin_apl