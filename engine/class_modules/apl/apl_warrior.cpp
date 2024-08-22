#include "class_modules/apl/apl_warrior.hpp"

#include "simulationcraft.hpp"

namespace warrior_apl
{
//fury_apl_start
void fury( player_t* p )
{
  std::vector<std::string> racial_actions = p->get_racial_actions();

  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* multi_target = p->get_action_priority_list( "multi_target" );
  action_priority_list_t* single_target = p->get_action_priority_list( "single_target" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );
  action_priority_list_t* variables = p->get_action_priority_list( "variables" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "variable,name=trinket_1_exclude,value=trinket.1.is.ruby_whelp_shell|trinket.1.is.whispering_incarnate_icon" );
  precombat->add_action( "variable,name=trinket_2_exclude,value=trinket.2.is.ruby_whelp_shell|trinket.2.is.whispering_incarnate_icon" );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=trinket.1.has_use_buff&(trinket.1.cooldown.duration%%cooldown.avatar.duration=0|trinket.1.cooldown.duration%%cooldown.odyns_fury.duration=0)", "Evaluates a trinkets cooldown, divided by avatar or odyns fur. If it's value has no remainder return 1, else return 0.5." );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=trinket.2.has_use_buff&(trinket.2.cooldown.duration%%cooldown.avatar.duration=0|trinket.2.cooldown.duration%%cooldown.odyns_fury.duration=0)" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_use_buff|(trinket.1.has_buff.strength|trinket.1.has_buff.mastery|trinket.1.has_buff.versatility|trinket.1.has_buff.haste|trinket.1.has_buff.crit&!variable.trinket_1_exclude)" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_use_buff|(trinket.2.has_buff.strength|trinket.2.has_buff.mastery|trinket.2.has_buff.versatility|trinket.2.has_buff.haste|trinket.2.has_buff.crit&!variable.trinket_2_exclude)" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs|variable.trinket_2_buffs&((trinket.2.cooldown.duration%trinket.2.proc.any_dps.duration)*(1.5+trinket.2.has_buff.strength)*(variable.trinket_2_sync))>((trinket.1.cooldown.duration%trinket.1.proc.any_dps.duration)*(1.5+trinket.1.has_buff.strength)*(variable.trinket_1_sync))" );
  precombat->add_action( "variable,name=trinket_1_manual,value=trinket.1.is.algethar_puzzle_box" );
  precombat->add_action( "variable,name=trinket_2_manual,value=trinket.2.is.algethar_puzzle_box" );
  precombat->add_action( "use_item,name=algethar_puzzle_box" );
  precombat->add_action( "berserker_stance,toggle=on" );
  precombat->add_action( "recklessness,if=!equipped.fyralath_the_dreamrender" );
  precombat->add_action( "avatar,if=!talent.titans_torment" );

  default_->add_action( "auto_attack" );
  default_->add_action( "charge,if=time<=0.5|movement.distance>5" );
  default_->add_action( "heroic_leap,if=(raid_event.movement.distance>25&raid_event.movement.in>45)" );
  default_->add_action( "potion" );
  default_->add_action( "pummel,if=target.debuff.casting.react" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "call_action_list,name=variables" );
  default_->add_action( "lights_judgment,if=buff.recklessness.down" );
  default_->add_action( "berserking,if=buff.recklessness.up" );
  default_->add_action( "blood_fury" );
  default_->add_action( "fireblood" );
  default_->add_action( "ancestral_call" );
  default_->add_action( "invoke_external_buff,name=power_infusion,if=buff.avatar.remains>15&fight_remains>=135|variable.execute_phase&buff.avatar.up|fight_remains<=25" );
  default_->add_action( "run_action_list,name=multi_target,if=active_enemies>=2" );
  default_->add_action( "run_action_list,name=single_target,if=active_enemies=1" ); 

  multi_target->add_action( "recklessness,if=(!talent.anger_management&cooldown.avatar.remains<1&talent.titans_torment)|talent.anger_management|!talent.titans_torment" );
  multi_target->add_action( "avatar,if=talent.titans_torment&(buff.enrage.up|talent.titanic_rage)|!talent.titans_torment" );
  multi_target->add_action( "thunderous_roar,if=buff.enrage.up" );
  multi_target->add_action( "champions_spear,if=buff.enrage.up" );
  multi_target->add_action( "odyns_fury,if=dot.odyns_fury_torment_mh.remains<1&(buff.enrage.up|talent.titanic_rage)&cooldown.avatar.remains" );
  multi_target->add_action( "whirlwind,if=buff.meat_cleaver.stack=0&talent.improved_whirlwind" );
  multi_target->add_action( "execute,if=buff.enrage.up&buff.ashen_juggernaut.remains<=gcd&talent.ashen_juggernaut" );
  multi_target->add_action( "rampage,if=rage.pct>85&cooldown.bladestorm.remains<=gcd&!debuff.champions_might.up" );
  multi_target->add_action( "bladestorm,if=buff.enrage.up&cooldown.avatar.remains>=9" );
  multi_target->add_action( "ravager,if=buff.enrage.up" );
  multi_target->add_action( "rampage,if=talent.anger_management" );
  multi_target->add_action( "bloodbath,if=buff.furious_bloodthirst.up" );
  multi_target->add_action( "crushing_blow" );
  multi_target->add_action( "onslaught,if=talent.tenderize|buff.enrage.up" );
  multi_target->add_action( "bloodbath,if=!dot.gushing_wound.remains" );
  multi_target->add_action( "rampage,if=talent.reckless_abandon" );
  multi_target->add_action( "execute,if=buff.enrage.up&((target.health.pct>35&talent.massacre|target.health.pct>20)&buff.sudden_death.remains<=gcd)" );
  multi_target->add_action( "bloodbath" );
  multi_target->add_action( "bloodthirst" );
  multi_target->add_action( "raging_blow" );
  multi_target->add_action( "execute" );
  multi_target->add_action( "whirlwind" );

  single_target->add_action( "ravager,if=cooldown.recklessness.remains<gcd|buff.recklessness.up" );
  single_target->add_action( "recklessness,if=!talent.anger_management|(talent.anger_management&cooldown.avatar.ready|cooldown.avatar.remains<gcd|cooldown.avatar.remains>30)" );
  single_target->add_action( "avatar,if=!talent.titans_torment|(talent.titans_torment&(buff.enrage.up|talent.titanic_rage))" );
  single_target->add_action( "champions_spear,if=buff.enrage.up&((buff.furious_bloodthirst.up&talent.titans_torment)|!talent.titans_torment|target.time_to_die<20|active_enemies>1|!set_bonus.tier31_2pc)&raid_event.adds.in>15" );
  single_target->add_action( "whirlwind,if=spell_targets.whirlwind>1&talent.improved_whirlwind&!buff.meat_cleaver.up|raid_event.adds.in<2&talent.improved_whirlwind&!buff.meat_cleaver.up" );
  single_target->add_action( "execute,if=buff.ashen_juggernaut.up&buff.ashen_juggernaut.remains<gcd" );
  single_target->add_action( "bladestorm,if=buff.enrage.up&(buff.avatar.up|buff.recklessness.up&talent.anger_management)" );
  single_target->add_action( "odyns_fury,if=buff.enrage.up&(spell_targets.whirlwind>1|raid_event.adds.in>15)&(talent.dancing_blades&buff.dancing_blades.remains<5|!talent.dancing_blades)" );
  single_target->add_action( "rampage,if=talent.anger_management&(buff.recklessness.up|buff.enrage.remains<gcd|rage.pct>85)" );
  single_target->add_action( "bloodbath,if=set_bonus.tier30_4pc&action.bloodthirst.crit_pct_current>=95" );
  single_target->add_action( "bloodthirst,if=(set_bonus.tier30_4pc&action.bloodthirst.crit_pct_current>=95)|(!talent.reckless_abandon&buff.furious_bloodthirst.up&buff.enrage.up&(!dot.gushing_wound.remains|buff.champions_might.up))" );
  single_target->add_action( "bloodbath,if=buff.furious_bloodthirst.up" );
  single_target->add_action( "thunderous_roar,if=buff.enrage.up&(spell_targets.whirlwind>1|raid_event.adds.in>15)" );
  single_target->add_action( "onslaught,if=buff.enrage.up|talent.tenderize" );
  single_target->add_action( "crushing_blow,if=buff.enrage.up" );
  single_target->add_action( "rampage,if=talent.reckless_abandon&(buff.recklessness.up|buff.enrage.remains<gcd|rage.pct>85)" );
  single_target->add_action( "execute,if=buff.enrage.up&!buff.furious_bloodthirst.up&buff.ashen_juggernaut.up|buff.sudden_death.remains<=gcd&(target.health.pct>35&talent.massacre|target.health.pct>20)" );
  single_target->add_action( "execute,if=buff.enrage.up" );
  single_target->add_action( "rampage,if=talent.anger_management" );
  single_target->add_action( "bloodbath,if=buff.enrage.up&talent.reckless_abandon" );
  single_target->add_action( "rampage,if=target.health.pct<35&talent.massacre.enabled" );
  single_target->add_action( "bloodthirst,if=buff.enrage.down|!buff.furious_bloodthirst.up" );
  single_target->add_action( "raging_blow,if=charges>1" );
  single_target->add_action( "crushing_blow,if=charges>1" );
  single_target->add_action( "bloodbath,if=buff.enrage.down" );
  single_target->add_action( "crushing_blow,if=buff.enrage.up&talent.reckless_abandon" );
  single_target->add_action( "bloodthirst,if=!buff.furious_bloodthirst.up" );
  single_target->add_action( "raging_blow,if=charges>1" );
  single_target->add_action( "rampage" );
  single_target->add_action( "bloodbath" );
  single_target->add_action( "raging_blow" );
  single_target->add_action( "crushing_blow" );
  single_target->add_action( "bloodthirst" );
  single_target->add_action( "slam" );

  trinkets->add_action( "use_item,name=fyralath_the_dreamrender,if=dot.mark_of_fyralath.ticking", "Trinkets" );
  trinkets->add_action( "use_item,use_off_gcd=1,name=algethar_puzzle_box,if=cooldown.recklessness.remains<3|(talent.anger_management&cooldown.avatar.remains<3)" );
  trinkets->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=variable.trinket_1_buffs&!variable.trinket_1_manual&(!buff.avatar.up&trinket.1.cast_time>0|!trinket.1.cast_time>0)&(buff.avatar.up)&(variable.trinket_2_exclude|!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1)|trinket.1.proc.any_dps.duration>=fight_remains", "Trinkets The trinket with the highest estimated value, will be used first and paired with Avatar." );
  trinkets->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=variable.trinket_2_buffs&!variable.trinket_2_manual&(!buff.avatar.up&trinket.2.cast_time>0|!trinket.2.cast_time>0)&(buff.avatar.up)&(variable.trinket_1_exclude|!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2)|trinket.2.proc.any_dps.duration>=fight_remains" );
  trinkets->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=!variable.trinket_1_buffs&!variable.trinket_1_manual&(!variable.trinket_1_buffs&(trinket.2.cooldown.remains|!variable.trinket_2_buffs)|(trinket.1.cast_time>0&!buff.avatar.up|!trinket.1.cast_time>0)|cooldown.avatar.remains_expected>20)", "If only one on use trinket provides a buff, use the other on cooldown. Or if neither trinket provides a buff, use both on cooldown." );
  trinkets->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=!variable.trinket_2_buffs&!variable.trinket_2_manual&(!variable.trinket_2_buffs&(trinket.1.cooldown.remains|!variable.trinket_1_buffs)|(trinket.2.cast_time>0&!buff.avatar.up|!trinket.2.cast_time>0)|cooldown.avatar.remains_expected>20)" );
  trinkets->add_action( "use_item,use_off_gcd=1,slot=main_hand,if=!equipped.fyralath_the_dreamrender&(!variable.trinket_1_buffs|trinket.1.cooldown.remains)&(!variable.trinket_2_buffs|trinket.2.cooldown.remains)" );

  variables->add_action( "variable,name=st_planning,value=active_enemies=1&(raid_event.adds.in>15|!raid_event.adds.exists)", "Variables" );
  variables->add_action( "variable,name=adds_remain,value=active_enemies>=2&(!raid_event.adds.exists|raid_event.adds.exists&raid_event.adds.remains>5)" );
  variables->add_action( "variable,name=execute_phase,value=(talent.massacre.enabled&target.health.pct<35)|target.health.pct<20" );
}
//fury_apl_end

//arms_apl_start
void arms( player_t* p )
{
  std::vector<std::string> racial_actions = p->get_racial_actions();

  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* execute = p->get_action_priority_list( "execute" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* single_target = p->get_action_priority_list( "single_target" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );
  action_priority_list_t* variables = p->get_action_priority_list( "variables" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "variable,name=trinket_1_exclude,value=trinket.1.is.ruby_whelp_shell|trinket.1.is.whispering_incarnate_icon" );
  precombat->add_action( "variable,name=trinket_2_exclude,value=trinket.2.is.ruby_whelp_shell|trinket.2.is.whispering_incarnate_icon" );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=trinket.1.has_use_buff&(trinket.1.cooldown.duration%%cooldown.avatar.duration=0)", "Evaluates a trinkets cooldown, divided by avatar. If it's value has no remainder return 1, else return 0.5." );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=trinket.2.has_use_buff&(trinket.2.cooldown.duration%%cooldown.avatar.duration=0)" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_use_buff|(trinket.1.has_buff.strength|trinket.1.has_buff.mastery|trinket.1.has_buff.versatility|trinket.1.has_buff.haste|trinket.1.has_buff.crit&!variable.trinket_1_exclude)" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_use_buff|(trinket.2.has_buff.strength|trinket.2.has_buff.mastery|trinket.2.has_buff.versatility|trinket.2.has_buff.haste|trinket.2.has_buff.crit&!variable.trinket_2_exclude)" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs|variable.trinket_2_buffs&((trinket.2.cooldown.duration%trinket.2.proc.any_dps.duration)*(1.5+trinket.2.has_buff.strength)*(variable.trinket_2_sync))>((trinket.1.cooldown.duration%trinket.1.proc.any_dps.duration)*(1.5+trinket.1.has_buff.strength)*(variable.trinket_1_sync))" );
  precombat->add_action( "variable,name=trinket_1_manual,value=trinket.1.is.algethar_puzzle_box" );
  precombat->add_action( "variable,name=trinket_2_manual,value=trinket.2.is.algethar_puzzle_box" );
  precombat->add_action( "use_item,name=algethar_puzzle_box" );
  precombat->add_action( "battle_stance,toggle=on" );

  default_->add_action( "charge,if=time<=0.5|movement.distance>5" );
  default_->add_action( "auto_attack" );
  default_->add_action( "potion,if=gcd.remains=0&debuff.colossus_smash.remains>8|target.time_to_die<25" );
  default_->add_action( "pummel,if=target.debuff.casting.react" );
  default_->add_action( "call_action_list,name=variables" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "arcane_torrent,if=cooldown.mortal_strike.remains>1.5&rage<50" );
  default_->add_action( "lights_judgment,if=debuff.colossus_smash.down&cooldown.mortal_strike.remains" );
  default_->add_action( "bag_of_tricks,if=debuff.colossus_smash.down&cooldown.mortal_strike.remains" );
  default_->add_action( "berserking,if=target.time_to_die>180&buff.avatar.up|target.time_to_die<180&variable.execute_phase&buff.avatar.up|target.time_to_die<20" );
  default_->add_action( "blood_fury,if=debuff.colossus_smash.up" );
  default_->add_action( "fireblood,if=debuff.colossus_smash.up" );
  default_->add_action( "ancestral_call,if=debuff.colossus_smash.up" );
  default_->add_action( "invoke_external_buff,name=power_infusion,if=debuff.colossus_smash.up&fight_remains>=135|variable.execute_phase&buff.avatar.up|fight_remains<=25" );
  default_->add_action( "run_action_list,name=aoe,if=active_enemies>2|talent.fervor_of_battle.enabled&variable.execute_phase&!raid_event.adds.up&active_enemies>1" );
  default_->add_action( "run_action_list,name=execute,target_if=min:target.health.pct,if=variable.execute_phase" );
  default_->add_action( "run_action_list,name=single_target" );

  execute->add_action( "sweeping_strikes,if=active_enemies>1" );
  execute->add_action( "thunderous_roar" );
  execute->add_action( "champions_spear" );
  execute->add_action( "skullsplitter" );
  execute->add_action( "ravager" );
  execute->add_action( "avatar" );
  execute->add_action( "colossus_smash" );
  execute->add_action( "warbreaker" );
  execute->add_action( "mortal_strike,if=debuff.executioners_precision.stack=2" );
  execute->add_action( "overpower,if=rage<60" );
  execute->add_action( "execute" );
  execute->add_action( "bladestorm" );
  execute->add_action( "overpower" );

  aoe->add_action( "cleave,if=buff.strike_vulnerabilities.down|buff.collateral_damage.up&buff.merciless_bonegrinder.up" );
  aoe->add_action( "thunder_clap,if=dot.rend.duration<3&active_enemies>=3" );
  aoe->add_action( "thunderous_roar" );
  aoe->add_action( "avatar" );
  aoe->add_action( "ravager,if=cooldown.sweeping_strikes.remains<=1|buff.sweeping_strikes.up" );
  aoe->add_action( "sweeping_strikes" );
  aoe->add_action( "skullsplitter,if=buff.sweeping_strikes.up" );
  aoe->add_action( "warbreaker" );
  aoe->add_action( "bladestorm,if=talent.unhinged|talent.merciless_bonegrinder" );
  aoe->add_action( "champions_spear" );
  aoe->add_action( "colossus_smash" );
  aoe->add_action( "overpower,if=buff.sweeping_strikes.up&charges=2" );
  aoe->add_action( "cleave" );
  aoe->add_action( "mortal_strike,if=buff.sweeping_strikes.up" );
  aoe->add_action( "overpower,if=buff.sweeping_strikes.up" );
  aoe->add_action( "execute,if=buff.sweeping_strikes.up" );
  aoe->add_action( "bladestorm" );
  aoe->add_action( "overpower" );
  aoe->add_action( "thunder_clap" );
  aoe->add_action( "mortal_strike" );
  aoe->add_action( "execute" );
  aoe->add_action( "whirlwind" );

  single_target->add_action( "thunder_clap,if=dot.rend.remains<=gcd&active_enemies>=2&buff.sweeping_strikes.down" );
  single_target->add_action( "sweeping_strikes,if=active_enemies>1" );
  single_target->add_action( "rend,if=dot.rend.remains<=gcd" );
  single_target->add_action( "thunderous_roar" );
  single_target->add_action( "champions_spear" );
  single_target->add_action( "ravager" );
  single_target->add_action( "avatar" );
  single_target->add_action( "colossus_smash" );
  single_target->add_action( "warbreaker" );
  single_target->add_action( "cleave,if=active_enemies>=3" );
  single_target->add_action( "overpower,if=active_enemies>1&(buff.sweeping_strikes.up|talent.dreadnaught)&charges=2" );
  single_target->add_action( "mortal_strike" );
  single_target->add_action( "skullsplitter" );
  single_target->add_action( "execute" );
  single_target->add_action( "overpower" );
  single_target->add_action( "rend,if=dot.rend.remains<=8" );
  single_target->add_action( "cleave,if=active_enemies>=2&talent.fervor_of_battle" );
  single_target->add_action( "slam" );

  trinkets->add_action( "use_item,name=fyralath_the_dreamrender,,if=dot.mark_of_fyralath.ticking&!talent.blademasters_torment|dot.mark_of_fyralath.ticking&cooldown.avatar.remains>3&cooldown.bladestorm.remains>3&!debuff.colossus_smash.up", "Trinkets" );
  trinkets->add_action( "use_item,use_off_gcd=1,name=algethar_puzzle_box,if=cooldown.avatar.remains<=3" );
  trinkets->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=variable.trinket_1_buffs&!variable.trinket_1_manual&(!buff.avatar.up&trinket.1.cast_time>0|!trinket.1.cast_time>0)&buff.avatar.up&(variable.trinket_2_exclude|!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1)|trinket.1.proc.any_dps.duration>=fight_remains", "Trinkets The trinket with the highest estimated value, will be used first and paired with Avatar." );
  trinkets->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=variable.trinket_2_buffs&!variable.trinket_2_manual&(!buff.avatar.up&trinket.2.cast_time>0|!trinket.2.cast_time>0)&buff.avatar.up&(variable.trinket_1_exclude|!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2)|trinket.2.proc.any_dps.duration>=fight_remains" );
  trinkets->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=!variable.trinket_1_buffs&!variable.trinket_1_manual&(!variable.trinket_1_buffs&(trinket.2.cooldown.remains|!variable.trinket_2_buffs)|(trinket.1.cast_time>0&!buff.avatar.up|!trinket.1.cast_time>0)|cooldown.avatar.remains_expected>20)", "If only one on use trinket provides a buff, use the other on cooldown. Or if neither trinket provides a buff, use both on cooldown." );
  trinkets->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=!variable.trinket_2_buffs&!variable.trinket_2_manual&(!variable.trinket_2_buffs&(trinket.1.cooldown.remains|!variable.trinket_1_buffs)|(trinket.2.cast_time>0&!buff.avatar.up|!trinket.2.cast_time>0)|cooldown.avatar.remains_expected>20)" );
  trinkets->add_action( "use_item,use_off_gcd=1,slot=main_hand,if=!equipped.fyralath_the_dreamrender&(!variable.trinket_1_buffs|trinket.1.cooldown.remains)&(!variable.trinket_2_buffs|trinket.2.cooldown.remains)" );

  variables->add_action( "variable,name=st_planning,value=active_enemies=1&(raid_event.adds.in>15|!raid_event.adds.exists)", "Variables" );
  variables->add_action( "variable,name=adds_remain,value=active_enemies>=2&(!raid_event.adds.exists|raid_event.adds.exists&raid_event.adds.remains>5)" );
  variables->add_action( "variable,name=execute_phase,value=(talent.massacre.enabled&target.health.pct<35)|target.health.pct<20" );
}
//arms_apl_end

//protection_apl_start
void protection( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* generic = p->get_action_priority_list( "generic" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "battle_stance,toggle=on" );

  default_->add_action( "auto_attack" );
  default_->add_action( "charge,if=time=0" );
  default_->add_action( "use_items" );
  default_->add_action( "avatar,if=buff.thunder_blast.down|buff.thunder_blast.stack<=2" );
  default_->add_action( "shield_wall,if=talent.immovable_object.enabled&buff.avatar.down" );
  default_->add_action( "blood_fury" );
  default_->add_action( "berserking" );
  default_->add_action( "arcane_torrent" );
  default_->add_action( "lights_judgment" );
  default_->add_action( "fireblood" );
  default_->add_action( "ancestral_call" );
  default_->add_action( "bag_of_tricks" );
  default_->add_action( "potion,if=buff.avatar.up|buff.avatar.up&target.health.pct<=20" );
  default_->add_action( "ignore_pain,if=target.health.pct>=20&(rage.deficit<=15&cooldown.shield_slam.ready|rage.deficit<=40&cooldown.shield_charge.ready&talent.champions_bulwark.enabled|rage.deficit<=20&cooldown.shield_charge.ready|rage.deficit<=30&cooldown.demoralizing_shout.ready&talent.booming_voice.enabled|rage.deficit<=20&cooldown.avatar.ready|rage.deficit<=45&cooldown.demoralizing_shout.ready&talent.booming_voice.enabled&buff.last_stand.up&talent.unnerving_focus.enabled|rage.deficit<=30&cooldown.avatar.ready&buff.last_stand.up&talent.unnerving_focus.enabled|rage.deficit<=20|rage.deficit<=40&cooldown.shield_slam.ready&buff.violent_outburst.up&talent.heavy_repercussions.enabled&talent.impenetrable_wall.enabled|rage.deficit<=55&cooldown.shield_slam.ready&buff.violent_outburst.up&buff.last_stand.up&talent.unnerving_focus.enabled&talent.heavy_repercussions.enabled&talent.impenetrable_wall.enabled|rage.deficit<=17&cooldown.shield_slam.ready&talent.heavy_repercussions.enabled|rage.deficit<=18&cooldown.shield_slam.ready&talent.impenetrable_wall.enabled)|(rage>=70|buff.seeing_red.stack=7&rage>=35)&cooldown.shield_slam.remains<=1&buff.shield_block.remains>=4&set_bonus.tier31_2pc,use_off_gcd=1" );
  default_->add_action( "last_stand,if=(target.health.pct>=90&talent.unnerving_focus.enabled|target.health.pct<=20&talent.unnerving_focus.enabled)|talent.bolster.enabled|set_bonus.tier30_2pc|set_bonus.tier30_4pc" );
  default_->add_action( "ravager" );
  default_->add_action( "demoralizing_shout,if=talent.booming_voice.enabled" );
  default_->add_action( "champions_spear" );
  default_->add_action( "thunder_blast,if=spell_targets.thunder_blast>=2&buff.thunder_blast.stack=2" );
  default_->add_action( "demolish,if=buff.colossal_might.stack>=3" );
  default_->add_action( "thunderous_roar" );
  default_->add_action( "shield_charge" );
  default_->add_action( "shield_block,if=buff.shield_block.duration<=10" );
  default_->add_action( "run_action_list,name=aoe,if=spell_targets.thunder_clap>=3" );
  default_->add_action( "call_action_list,name=generic" );

  aoe->add_action( "thunder_blast,if=dot.rend.remains<=1" );
  aoe->add_action( "thunder_clap,if=dot.rend.remains<=1" );
  aoe->add_action( "thunder_blast,if=buff.violent_outburst.up&spell_targets.thunderclap>=2&buff.avatar.up&talent.unstoppable_force.enabled" );
  aoe->add_action( "thunder_clap,if=buff.violent_outburst.up&spell_targets.thunderclap>=4&buff.avatar.up&talent.unstoppable_force.enabled&talent.crashing_thunder.enabled|buff.violent_outburst.up&spell_targets.thunderclap>6&buff.avatar.up&talent.unstoppable_force.enabled" );
  aoe->add_action( "revenge,if=rage>=70&talent.seismic_reverberation.enabled&spell_targets.revenge>=3" );
  aoe->add_action( "shield_slam,if=rage<=60|buff.violent_outburst.up&spell_targets.thunderclap<=4&talent.crashing_thunder.enabled" );
  aoe->add_action( "thunder_blast" );
  aoe->add_action( "thunder_clap" );
  aoe->add_action( "revenge,if=rage>=30|rage>=40&talent.barbaric_training.enabled" );

  generic->add_action( "thunder_blast,if=(buff.thunder_blast.stack=2&buff.burst_of_power.stack<=1&buff.avatar.up&talent.unstoppable_force.enabled)" );
  generic->add_action( "shield_slam,if=(buff.burst_of_power.stack=2&buff.thunder_blast.stack<=1|buff.violent_outburst.up)|rage<=70&talent.demolish.enabled" );
  generic->add_action( "execute,if=rage>=70|(rage>=40&cooldown.shield_slam.remains&talent.demolish.enabled|rage>=50&cooldown.shield_slam.remains)|buff.sudden_death.up&talent.sudden_death.enabled" );
  generic->add_action( "shield_slam" );
  generic->add_action( "thunder_blast,if=dot.rend.remains<=2&buff.violent_outburst.down" );
  generic->add_action( "thunder_blast" );
  generic->add_action( "thunder_clap,if=dot.rend.remains<=2&buff.violent_outburst.down" );
  generic->add_action( "thunder_blast,if=(spell_targets.thunder_clap>1|cooldown.shield_slam.remains&!buff.violent_outburst.up)" );
  generic->add_action( "thunder_clap,if=(spell_targets.thunder_clap>1|cooldown.shield_slam.remains&!buff.violent_outburst.up)" );
  generic->add_action( "revenge,if=(rage>=80&target.health.pct>20|buff.revenge.up&target.health.pct<=20&rage<=18&cooldown.shield_slam.remains|buff.revenge.up&target.health.pct>20)|(rage>=80&target.health.pct>35|buff.revenge.up&target.health.pct<=35&rage<=18&cooldown.shield_slam.remains|buff.revenge.up&target.health.pct>35)&talent.massacre.enabled" );
  generic->add_action( "execute" );
  generic->add_action( "revenge,if=target.health>20" );
  generic->add_action( "thunder_blast,if=(spell_targets.thunder_clap>=1|cooldown.shield_slam.remains&buff.violent_outburst.up)" );
  generic->add_action( "thunder_clap,if=(spell_targets.thunder_clap>=1|cooldown.shield_slam.remains&buff.violent_outburst.up)" );
  generic->add_action( "devastate" );
}
//protection_apl_end
}  // namespace warrior_apl
