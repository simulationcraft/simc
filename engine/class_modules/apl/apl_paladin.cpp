#include "class_modules/apl/apl_paladin.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"
#include "dbc/dbc.hpp"
#include "sim/sim.hpp"

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

  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_buff.strength|trinket.1.has_buff.mastery|trinket.1.has_buff.versatility|trinket.1.has_buff.haste|trinket.1.has_buff.crit" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_buff.strength|trinket.2.has_buff.mastery|trinket.2.has_buff.versatility|trinket.2.has_buff.haste|trinket.2.has_buff.crit" );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_1_buffs&(trinket.1.cooldown.duration%%cooldown.avenging_wrath.duration=0|cooldown.avenging_wrath.duration%%trinket.1.cooldown.duration=0)" );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_2_buffs&(trinket.2.cooldown.duration%%cooldown.avenging_wrath.duration=0|cooldown.avenging_wrath.duration%%trinket.2.cooldown.duration=0)" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs|variable.trinket_2_buffs&((trinket.2.cooldown.duration%trinket.2.proc.any_dps.duration)*(1.5+trinket.2.has_buff.strength)*(variable.trinket_2_sync))>((trinket.1.cooldown.duration%trinket.1.proc.any_dps.duration)*(1.5+trinket.1.has_buff.strength)*(variable.trinket_1_sync))" );
  precombat->add_action( "use_item,name=algethar_puzzle_box,if=(trinket.1.is.algethar_puzzle_box|trinket.2.is.algethar_puzzle_box)" );

  default_->add_action( "auto_attack" );
  default_->add_action( "rebuke" );
  default_->add_action( "call_action_list,name=cooldowns" );
  default_->add_action( "call_action_list,name=generators" );

  cooldowns->add_action( "use_item,name=algethar_puzzle_box,use_off_gcd=1,if=(cooldown.avenging_wrath.remains=0&!talent.radiant_glory|(!talent.execution_sentence&cooldown.wake_of_ashes.remains=0|cooldown.execution_sentenc.remains=0)&talent.radiant_glory)" );
  cooldowns->add_action( "use_item,slot=trinket1,if=((buff.avenging_wrath.up&cooldown.avenging_wrath.remains>40)&!talent.radiant_glory|talent.radiant_glory&(!talent.execution_sentence&cooldown.wake_of_ashes.remains=0|debuff.execution_sentence_debuff.up))&(!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1)|trinket.1.proc.any_dps.duration>=fight_remains" );
  cooldowns->add_action( "use_item,slot=trinket2,if=((buff.avenging_wrath.up&cooldown.avenging_wrath.remains>40)&!talent.radiant_glory|talent.radiant_glory&(!talent.execution_sentence&cooldown.wake_of_ashes.remains=0|debuff.execution_sentence_debuff.up))&(!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2)|trinket.2.proc.any_dps.duration>=fight_remains" );
  cooldowns->add_action( "use_item,slot=trinket1,if=!variable.trinket_1_buffs&(trinket.2.cooldown.remains|!variable.trinket_2_buffs|!buff.avenging_wrath.up&cooldown.avenging_wrath.remains>20)" );
  cooldowns->add_action( "use_item,slot=trinket2,if=!variable.trinket_2_buffs&(trinket.1.cooldown.remains|!variable.trinket_1_buffs|!buff.avenging_wrath.up&cooldown.avenging_wrath.remains>20)" );
  cooldowns->add_action( "potion,if=buff.avenging_wrath.up|fight_remains<30|talent.radiant_glory&cooldown.wake_of_ashes.remains=0&(!talent.holy_flames|dot.expurgation.ticking)" );
  cooldowns->add_action( "invoke_external_buff,name=power_infusion,if=buff.avenging_wrath.up|talent.radiant_glory&cooldown.wake_of_ashes.remains=0&(!talent.holy_flames|dot.expurgation.ticking)" );
  cooldowns->add_action( "lights_judgment,if=!raid_event.adds.exists|raid_event.adds.in>75|raid_event.adds.up" );
  cooldowns->add_action( "fireblood,if=buff.avenging_wrath.up|talent.radiant_glory&cooldown.wake_of_ashes.remains=0&(!talent.holy_flames|dot.expurgation.ticking)" );
  cooldowns->add_action( "execution_sentence,if=(cooldown.avenging_wrath.remains>15|talent.radiant_glory)&(target.time_to_die>10)&cooldown.wake_of_ashes.remains<gcd&(!talent.holy_flames|dot.expurgation.ticking)" );
  cooldowns->add_action( "avenging_wrath,if=(!raid_event.adds.up|target.time_to_die>10)&(!talent.holy_flames|dot.expurgation.ticking)&(!equipped.algethar_puzzle_box|trinket.1.is.algethar_puzzle_box&trinket.1.cooldown.remains>5|trinket.2.is.algethar_puzzle_box&trinket.2.cooldown.remains>5)&(!talent.lights_guidance|debuff.judgment.up|time>5)" );

  finishers->add_action( "variable,name=ds_castable,value=(active_enemies>=3|buff.empyrean_power.up)&!buff.empyrean_legacy.up" );
  finishers->add_action( "hammer_of_light,if=!buff.hammer_of_light_free.up|buff.hammer_of_light_free.up&(buff.undisputed_ruling.remains<gcd*1.5&(talent.radiant_glory|cooldown.avenging_wrath.remains>4)|buff.avenging_wrath.up&(buff.avenging_wrath.remains<gcd*2|cooldown.wake_of_ashes.remains=0)|buff.hammer_of_light_free.remains<gcd*2|target.time_to_die<gcd*2)" );
  finishers->add_action( "divine_storm,if=variable.ds_castable&(!buff.hammer_of_light_ready.up|buff.hammer_of_light_free.up)" );
  finishers->add_action( "templars_verdict,if=(!buff.hammer_of_light_ready.up|buff.hammer_of_light_free.up)" );

  generators->add_action( "call_action_list,name=finishers,if=holy_power=5&cooldown.wake_of_ashes.remains|buff.hammer_of_light_free.remains<gcd*2" );
  generators->add_action( "blade_of_justice,if=talent.holy_flames&!dot.expurgation.ticking&time<5" );
  generators->add_action( "judgment,if=talent.lights_guidance&!debuff.judgment.up&time<5" );
  generators->add_action( "wake_of_ashes,if=(cooldown.avenging_wrath.remains>6|talent.radiant_glory)&(!talent.execution_sentence|cooldown.execution_sentence.remains>4|target.time_to_die<10)&(!raid_event.adds.exists|raid_event.adds.in>10|raid_event.adds.up)" );
  generators->add_action( "divine_toll,if=(!raid_event.adds.exists|raid_event.adds.in>10|raid_event.adds.up)&(cooldown.avenging_wrath.remains>15|talent.radiant_glory|fight_remains<8)" );
  generators->add_action( "blade_of_justice,if=(buff.art_of_war.up|buff.righteous_cause.up)&(!talent.walk_into_light|!buff.avenging_wrath.up)" );
  generators->add_action( "call_action_list,name=finishers" );
  generators->add_action( "hammer_of_wrath,if=talent.walk_into_light" );
  generators->add_action( "blade_of_justice" );
  generators->add_action( "hammer_of_wrath" );
  generators->add_action( "judgment" );
  generators->add_action( "templar_strike" );
  generators->add_action( "templar_slash" );
  generators->add_action( "crusader_strike" );
  generators->add_action( "arcane_torrent" );
}
//retribution_apl_end

//protection_apl_start
void protection( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );

  precombat->add_action( "rite_of_sanctification" );
  precombat->add_action( "rite_of_adjuration" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "devotion_aura" );
  precombat->add_action( "lights_judgment" );
  precombat->add_action( "consecration" );

  default_->add_action( "auto_attack" );
  default_->add_action( "use_item,name=algethar_puzzle_box" );
  default_->add_action( "use_items" );
  default_->add_action( "potion,if=buff.avenging_wrath.up" );
  default_->add_action( "avenging_wrath,if=cooldown.divine_toll.remains<=10" );
  default_->add_action( "fireblood,if=buff.avenging_wrath.up" );
  default_->add_action( "divine_toll,if=buff.avenging_wrath.up|(!talent.righteous_protector.enabled&cooldown.avenging_wrath.remains<30)" );
  default_->add_action( "hammer_of_light,if=(!buff.undisputed_ruling.up|buff.hammer_of_light_ready.remains<5)&debuff.judgment.up" );
  default_->add_action( "shield_of_the_righteous,if=!buff.hammer_of_light_ready.up|(!buff.hammer_of_light_ready.remains<5&buff.undisputed_ruling.up)|buff.hammer_of_light_free.up|prev_gcd.1.divine_toll" );
  default_->add_action( "holy_armaments,if=next_armament=sacred_weapon&(buff.sacred_weapon.remains<6|!buff.sacred_weapon.up)" );
  default_->add_action( "hammer_of_wrath,if=buff.hammer_of_light_ready.up&!debuff.judgment.up" );
  default_->add_action( "judgment,if=buff.hammer_of_light_ready.up&!debuff.judgment.up" );
  default_->add_action( "avengers_shield,if=buff.vanguard.up|(buff.avenging_wrath.up&apex.3)" );
  default_->add_action( "holy_armaments,if=next_armament=holy_bulwark&cooldown.avenging_wrath.remains<5" );
  default_->add_action( "consecration,if=buff.divine_guidance.stack>=5" );
  default_->add_action( "hammer_of_wrath" );
  default_->add_action( "judgment,if=full_recharge_time<=gcd*2" );
  default_->add_action( "avengers_shield" );
  default_->add_action( "hammer_of_the_righteous,if=buff.blessed_assurance.up" );
  default_->add_action( "blessed_hammer,if=buff.blessed_assurance.up" );
  default_->add_action( "judgment" );
  default_->add_action( "holy_armaments,if=next_armament=holy_bulwark&charges=2" );
  default_->add_action( "consecration,if=!consecration.up" );
  default_->add_action( "blessed_hammer" );
  default_->add_action( "hammer_of_the_righteous" );
  default_->add_action( "arcane_torrent" );
  default_->add_action( "word_of_glory,if=buff.shining_light_free.up" );
  default_->add_action( "consecration" );
}
//protection_apl_end
}  // namespace paladin_apl
