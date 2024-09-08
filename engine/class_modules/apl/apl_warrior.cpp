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

  action_priority_list_t* slayer_st = p->get_action_priority_list( "slayer_st" );
  action_priority_list_t* slayer_mt  = p->get_action_priority_list( "slayer_mt" );
  action_priority_list_t* thane_st = p->get_action_priority_list( "thane_st" );
  action_priority_list_t* thane_mt = p->get_action_priority_list( "thane_mt" );
  action_priority_list_t* trinkets  = p->get_action_priority_list( "trinkets" );
  action_priority_list_t* variables = p->get_action_priority_list( "variables" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "variable,name=trinket_1_exclude,value=trinket.1.is.treacherous_transmitter" );
  precombat->add_action( "variable,name=trinket_2_exclude,value=trinket.2.is.treacherous_transmitter" );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=trinket.1.has_use_buff&(trinket.1.cooldown.duration%%cooldown.avatar.duration=0|trinket.1.cooldown.duration%%cooldown.odyns_fury.duration=0)", "Evaluates a trinkets cooldown, divided by avatar or odyns fury. If it's value has no remainder return 1, else return 0.5." );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=trinket.2.has_use_buff&(trinket.2.cooldown.duration%%cooldown.avatar.duration=0|trinket.2.cooldown.duration%%cooldown.odyns_fury.duration=0)" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_use_buff|(trinket.1.has_stat.any_dps&!variable.trinket_1_exclude)" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_use_buff|(trinket.2.has_stat.any_dps&!variable.trinket_2_exclude)" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs|variable.trinket_2_buffs&((trinket.2.cooldown.duration%trinket.2.proc.any_dps.duration)*(1.5+trinket.2.has_buff.strength)*(variable.trinket_2_sync))>((trinket.1.cooldown.duration%trinket.1.proc.any_dps.duration)*(1.5+trinket.1.has_buff.strength)*(variable.trinket_1_sync))" );
  precombat->add_action( "variable,name=trinket_1_manual,value=trinket.1.is.algethar_puzzle_box" );
  precombat->add_action( "variable,name=trinket_2_manual,value=trinket.2.is.algethar_puzzle_box" );
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
  default_->add_action( "lights_judgment,if=variable.on_gcd_racials" );
  default_->add_action( "bag_of_tricks,if=variable.on_gcd_racials" );
  default_->add_action( "berserking,if=buff.recklessness.up" );
  default_->add_action( "blood_fury" );
  default_->add_action( "fireblood" );
  default_->add_action( "ancestral_call" );
  default_->add_action( "invoke_external_buff,name=power_infusion,if=buff.avatar.remains>15&fight_remains>=135|variable.execute_phase&buff.avatar.up|fight_remains<=25" );
  default_->add_action( "run_action_list,name=slayer_st,if=talent.slayers_dominance&active_enemies=1" );
  default_->add_action( "run_action_list,name=slayer_mt,if=talent.slayers_dominance&active_enemies>1" ); 
  default_->add_action( "run_action_list,name=thane_st,if=talent.lightning_strikes&active_enemies=1" );
  default_->add_action( "run_action_list,name=thane_mt,if=talent.lightning_strikes&active_enemies>1" ); 

  slayer_st->add_action( "recklessness,if=(!talent.anger_management&cooldown.avatar.remains<1&talent.titans_torment)|talent.anger_management|!talent.titans_torment" );
  slayer_st->add_action( "avatar,if=(talent.titans_torment&(buff.enrage.up|talent.titanic_rage)&(debuff.champions_might.up|!talent.champions_might))|!talent.titans_torment" );
  slayer_st->add_action( "thunderous_roar,if=buff.enrage.up" );
  slayer_st->add_action( "champions_spear,if=(buff.enrage.up&talent.titans_torment&cooldown.avatar.remains<gcd)|(buff.enrage.up&!talent.titans_torment)" );
  slayer_st->add_action( "odyns_fury,if=dot.odyns_fury_torment_mh.remains<1&(buff.enrage.up|talent.titanic_rage)&cooldown.avatar.remains" );
  slayer_st->add_action( "execute,if=talent.ashen_juggernaut&buff.ashen_juggernaut.remains<=gcd&buff.enrage.up" );
  slayer_st->add_action( "rampage,if=talent.bladestorm&cooldown.bladestorm.remains<=gcd&!debuff.champions_might.up" );
  slayer_st->add_action( "bladestorm,if=buff.enrage.up&cooldown.avatar.remains>=9" );
  slayer_st->add_action( "onslaught,if=talent.tenderize&buff.brutal_finish.up" );
  slayer_st->add_action( "rampage,if=talent.anger_management" );
  slayer_st->add_action( "crushing_blow" );
  slayer_st->add_action( "onslaught,if=talent.tenderize" );
  slayer_st->add_action( "bloodbath,if=buff.enrage.up" );
  slayer_st->add_action( "raging_blow,if=talent.slaughtering_strikes&rage<110&talent.reckless_abandon" );
  slayer_st->add_action( "execute,if=buff.enrage.up&debuff.marked_for_execution.up" );
  slayer_st->add_action( "rampage,if=talent.reckless_abandon" );
  slayer_st->add_action( "bloodthirst,if=!talent.reckless_abandon&buff.enrage.up" );
  slayer_st->add_action( "raging_blow" );
  slayer_st->add_action( "onslaught" );
  slayer_st->add_action( "execute" );
  slayer_st->add_action( "bloodthirst" );
  slayer_st->add_action( "whirlwind,if=talent.meat_cleaver" );
  slayer_st->add_action( "slam" );
  slayer_st->add_action( "storm_bolt,if=buff.bladestorm.up" );

  slayer_mt->add_action( "recklessness,if=(!talent.anger_management&cooldown.avatar.remains<1&talent.titans_torment)|talent.anger_management|!talent.titans_torment" );
  slayer_mt->add_action( "avatar,if=(talent.titans_torment&(buff.enrage.up|talent.titanic_rage)&(debuff.champions_might.up|!talent.champions_might))|!talent.titans_torment" );
  slayer_mt->add_action( "thunderous_roar,if=buff.enrage.up" );
  slayer_mt->add_action( "champions_spear,if=(buff.enrage.up&talent.titans_torment&cooldown.avatar.remains<gcd)|(buff.enrage.up&!talent.titans_torment)" );
  slayer_mt->add_action( "odyns_fury,if=dot.odyns_fury_torment_mh.remains<1&(buff.enrage.up|talent.titanic_rage)&cooldown.avatar.remains" );
  slayer_mt->add_action( "whirlwind,if=buff.meat_cleaver.stack=0&talent.meat_cleaver" );
  slayer_mt->add_action( "execute,if=talent.ashen_juggernaut&buff.ashen_juggernaut.remains<=gcd&buff.enrage.up" );
  slayer_mt->add_action( "rampage,if=talent.bladestorm&cooldown.bladestorm.remains<=gcd&!debuff.champions_might.up" );
  slayer_mt->add_action( "bladestorm,if=buff.enrage.up&cooldown.avatar.remains>=9" );
  slayer_mt->add_action( "onslaught,if=talent.tenderize&buff.brutal_finish.up" );
  slayer_mt->add_action( "rampage,if=talent.anger_management" );
  slayer_mt->add_action( "crushing_blow" );
  slayer_mt->add_action( "onslaught,if=talent.tenderize" );
  slayer_mt->add_action( "bloodbath,if=buff.enrage.up" );
  slayer_mt->add_action( "rampage,if=talent.reckless_abandon" );
  slayer_mt->add_action( "execute,if=buff.enrage.up&debuff.marked_for_execution.up" );
  slayer_mt->add_action( "bloodbath" );
  slayer_mt->add_action( "raging_blow,if=talent.slaughtering_strikes" );
  slayer_mt->add_action( "onslaught" );
  slayer_mt->add_action( "execute" );
  slayer_mt->add_action( "bloodthirst" );
  slayer_mt->add_action( "raging_blow" );
  slayer_mt->add_action( "whirlwind" );
  slayer_mt->add_action( "storm_bolt,if=buff.bladestorm.up" );

  thane_st->add_action( "recklessness,if=(!talent.anger_management&cooldown.avatar.remains<1&talent.titans_torment)|talent.anger_management|!talent.titans_torment" );
  thane_st->add_action( "thunder_blast,if=buff.enrage.up" );
  thane_st->add_action( "avatar,if=(talent.titans_torment&(buff.enrage.up|talent.titanic_rage)&(debuff.champions_might.up|!talent.champions_might))|!talent.titans_torment" );
  thane_st->add_action( "ravager" );
  thane_st->add_action( "thunderous_roar,if=buff.enrage.up" );
  thane_st->add_action( "champions_spear,if=buff.enrage.up&(cooldown.avatar.remains<gcd|!talent.titans_torment)" );
  thane_st->add_action( "odyns_fury,if=dot.odyns_fury_torment_mh.remains<1&(buff.enrage.up|talent.titanic_rage)&cooldown.avatar.remains" );
  thane_st->add_action( "execute,if=talent.ashen_juggernaut&buff.ashen_juggernaut.remains<=gcd&buff.enrage.up" );
  thane_st->add_action( "rampage,if=talent.bladestorm&cooldown.bladestorm.remains<=gcd&!debuff.champions_might.up" );
  thane_st->add_action( "bladestorm,if=buff.enrage.up&talent.unhinged" );
  thane_st->add_action( "rampage,if=talent.anger_management" );
  thane_st->add_action( "crushing_blow" );
  thane_st->add_action( "onslaught,if=talent.tenderize" );
  thane_st->add_action( "bloodbath" );
  thane_st->add_action( "rampage,if=talent.reckless_abandon" );
  thane_st->add_action( "raging_blow" );
  thane_st->add_action( "execute" );
  thane_st->add_action( "bloodthirst,if=buff.enrage.up&(!buff.burst_of_power.up|!talent.reckless_abandon)" );
  thane_st->add_action( "onslaught" );
  thane_st->add_action( "bloodthirst" );
  thane_st->add_action( "thunder_clap" );
  thane_st->add_action( "whirlwind,if=talent.meat_cleaver" );
  thane_st->add_action( "slam" );

  thane_mt->add_action( "recklessness,if=(!talent.anger_management&cooldown.avatar.remains<1&talent.titans_torment)|talent.anger_management|!talent.titans_torment" );
  thane_mt->add_action( "thunder_blast,if=buff.enrage.up" );
  thane_mt->add_action( "avatar,if=(talent.titans_torment&(buff.enrage.up|talent.titanic_rage)&(debuff.champions_might.up|!talent.champions_might))|!talent.titans_torment" );
  thane_mt->add_action( "thunder_clap,if=buff.meat_cleaver.stack=0&talent.meat_cleaver" );
  thane_mt->add_action( "thunderous_roar,if=buff.enrage.up" );
  thane_mt->add_action( "ravager" );
  thane_mt->add_action( "champions_spear,if=buff.enrage.up" );
  thane_mt->add_action( "odyns_fury,if=dot.odyns_fury_torment_mh.remains<1&(buff.enrage.up|talent.titanic_rage)&cooldown.avatar.remains" );
  thane_mt->add_action( "execute,if=talent.ashen_juggernaut&buff.ashen_juggernaut.remains<=gcd&buff.enrage.up" );
  thane_mt->add_action( "rampage,if=talent.bladestorm&cooldown.bladestorm.remains<=gcd&!debuff.champions_might.up" );
  thane_mt->add_action( "bladestorm,if=buff.enrage.up" );
  thane_mt->add_action( "rampage,if=talent.anger_management" );
  thane_mt->add_action( "crushing_blow,if=buff.enrage.up" );
  thane_mt->add_action( "onslaught,if=talent.tenderize" );
  thane_mt->add_action( "bloodbath" );
  thane_mt->add_action( "rampage,if=talent.reckless_abandon" );
  thane_mt->add_action( "bloodthirst" );
  thane_mt->add_action( "thunder_clap" );
  thane_mt->add_action( "onslaught" );
  thane_mt->add_action( "execute" );
  thane_mt->add_action( "raging_blow" );
  thane_mt->add_action( "whirlwind" );


  trinkets->add_action( "do_treacherous_transmitter_task", "Trinkets" );
  trinkets->add_action( "use_item,name=treacherous_transmitter,if=variable.adds_remain|variable.st_planning" );
  trinkets->add_action( "use_item,slot=trinket1,if=variable.trinket_1_buffs&!variable.trinket_1_manual&(!buff.avatar.up&trinket.1.cast_time>0|!trinket.1.cast_time>0)&((talent.titans_torment&cooldown.avatar.ready)|(buff.avatar.up&!talent.titans_torment))&(variable.trinket_2_exclude|!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1)|trinket.1.proc.any_dps.duration>=fight_remains", "Trinkets The trinket with the highest estimated value, will be used first and paired with Avatar." );
  trinkets->add_action( "use_item,slot=trinket2,if=variable.trinket_2_buffs&!variable.trinket_2_manual&(!buff.avatar.up&trinket.2.cast_time>0|!trinket.2.cast_time>0)&((talent.titans_torment&cooldown.avatar.ready)|(buff.avatar.up&!talent.titans_torment))&(variable.trinket_1_exclude|!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2)|trinket.2.proc.any_dps.duration>=fight_remains" );
  trinkets->add_action( "use_item,slot=trinket1,if=!variable.trinket_1_buffs&(trinket.1.cast_time>0&!buff.avatar.up|!trinket.1.cast_time>0)&!variable.trinket_1_manual&(!variable.trinket_1_buffs&(trinket.2.cooldown.remains|!variable.trinket_2_buffs)|(trinket.1.cast_time>0&!buff.avatar.up|!trinket.1.cast_time>0)|cooldown.avatar.remains_expected>20)", "If only one on use trinket provides a buff, use the other on cooldown. Or if neither trinket provides a buff, use both on cooldown." );
  trinkets->add_action( "use_item,slot=trinket2,if=!variable.trinket_2_buffs&(trinket.2.cast_time>0&!buff.avatar.up|!trinket.2.cast_time>0)&!variable.trinket_2_manual&(!variable.trinket_2_buffs&(trinket.1.cooldown.remains|!variable.trinket_1_buffs)|(trinket.2.cast_time>0&!buff.avatar.up|!trinket.2.cast_time>0)|cooldown.avatar.remains_expected>20)" );
  trinkets->add_action( "use_item,slot=main_hand,if=!equipped.fyralath_the_dreamrender&(!variable.trinket_1_buffs|trinket.1.cooldown.remains)&(!variable.trinket_2_buffs|trinket.2.cooldown.remains)" );

  variables->add_action( "variable,name=st_planning,value=active_enemies=1&(raid_event.adds.in>15|!raid_event.adds.exists)", "Variables" );
  variables->add_action( "variable,name=adds_remain,value=active_enemies>=2&(!raid_event.adds.exists|raid_event.adds.exists&raid_event.adds.remains>5)" );
  variables->add_action( "variable,name=execute_phase,value=(talent.massacre.enabled&target.health.pct<35)|target.health.pct<20" );
  variables->add_action( "variable,name=on_gcd_racials,value=buff.recklessness.down&buff.avatar.down&rage<80&buff.bloodbath.down&buff.crushing_blow.down&buff.sudden_death.down&!cooldown.bladestorm.ready&(!cooldown.execute.ready|!variable.execute_phase)" );
}
//fury_apl_end

//arms_apl_start
void arms( player_t* p )
{
  std::vector<std::string> racial_actions = p->get_racial_actions();

  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* colossus_aoe     = p->get_action_priority_list( "colossus_aoe" );
  action_priority_list_t* colossus_sweep = p->get_action_priority_list( "colossus_sweep" );
  action_priority_list_t* colossus_execute = p->get_action_priority_list( "colossus_execute" );
  action_priority_list_t* colossus_st = p->get_action_priority_list( "colossus_st" );
  action_priority_list_t* slayer_aoe       = p->get_action_priority_list( "slayer_aoe" );
  action_priority_list_t* slayer_sweep     = p->get_action_priority_list( "slayer_sweep" );
  action_priority_list_t* slayer_execute   = p->get_action_priority_list( "slayer_execute" );
  action_priority_list_t* slayer_st        = p->get_action_priority_list( "slayer_st" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );
  action_priority_list_t* variables = p->get_action_priority_list( "variables" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "variable,name=trinket_1_exclude,value=trinket.1.is.treacherous_transmitter" );
  precombat->add_action( "variable,name=trinket_2_exclude,value=trinket.2.is.treacherous_transmitter" );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=trinket.1.has_use_buff&(trinket.1.cooldown.duration%%cooldown.avatar.duration=0)", "Evaluates a trinkets cooldown, divided by avatar. If it's value has no remainder return 1, else return 0.5." );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=trinket.2.has_use_buff&(trinket.2.cooldown.duration%%cooldown.avatar.duration=0)" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_use_buff|(trinket.1.has_stat.any_dps&!variable.trinket_1_exclude)" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_use_buff|(trinket.2.has_stat.any_dps&!variable.trinket_2_exclude)" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs|variable.trinket_2_buffs&((trinket.2.cooldown.duration%trinket.2.proc.any_dps.duration)*(1.5+trinket.2.has_buff.strength)*(variable.trinket_2_sync))>((trinket.1.cooldown.duration%trinket.1.proc.any_dps.duration)*(1.5+trinket.1.has_buff.strength)*(variable.trinket_1_sync))" );
  precombat->add_action( "variable,name=trinket_1_manual,value=trinket.1.is.algethar_puzzle_box" );
  precombat->add_action( "variable,name=trinket_2_manual,value=trinket.2.is.algethar_puzzle_box" );
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
  default_->add_action( "run_action_list,name=colossus_aoe,if=talent.demolish&active_enemies>2" );
  default_->add_action( "run_action_list,name=colossus_execute,target_if=min:target.health.pct,if=talent.demolish&variable.execute_phase" );
  default_->add_action( "run_action_list,name=colossus_sweep,if=talent.demolish&active_enemies=2&!variable.execute_phase" );
  default_->add_action( "run_action_list,name=colossus_st,if=talent.demolish" );
  default_->add_action( "run_action_list,name=slayer_aoe,if=talent.slayers_dominance&active_enemies>2" );
  default_->add_action( "run_action_list,name=slayer_execute,target_if=min:target.health.pct,if=talent.slayers_dominance&variable.execute_phase" );
  default_->add_action( "run_action_list,name=slayer_sweep,if=talent.slayers_dominance&active_enemies=2&!variable.execute_phase" );
  default_->add_action( "run_action_list,name=slayer_st,if=talent.slayers_dominance" );

  colossus_st->add_action( "rend,if=dot.rend.remains<=gcd" );
  colossus_st->add_action( "thunderous_roar" );
  colossus_st->add_action( "champions_spear" );
  colossus_st->add_action( "ravager,if=cooldown.colossus_smash.remains<=gcd" );
  colossus_st->add_action( "avatar,if=raid_event.adds.in>15" );
  colossus_st->add_action( "colossus_smash" );
  colossus_st->add_action( "warbreaker" );
  colossus_st->add_action( "mortal_strike" );
  colossus_st->add_action( "demolish" );
  colossus_st->add_action( "skullsplitter" );
  colossus_st->add_action( "overpower,if=charges=2" );
  colossus_st->add_action( "execute" );
  colossus_st->add_action( "overpower" );
  colossus_st->add_action( "rend,if=dot.rend.remains<=gcd*5" );
  colossus_st->add_action( "slam" );

  colossus_execute->add_action( "sweeping_strikes,if=active_enemies=2" );
  colossus_execute->add_action( "rend,if=dot.rend.remains<=gcd&!talent.bloodletting" );
  colossus_execute->add_action( "thunderous_roar" );
  colossus_execute->add_action( "champions_spear" );
  colossus_execute->add_action( "ravager,if=cooldown.colossus_smash.remains<=gcd" );
  colossus_execute->add_action( "avatar" );
  colossus_execute->add_action( "colossus_smash" );
  colossus_execute->add_action( "warbreaker" );
  colossus_execute->add_action( "demolish,if=debuff.colossus_smash.up" );
  colossus_execute->add_action( "mortal_strike,if=debuff.executioners_precision.stack=2&!dot.ravager.remains&(buff.lethal_blows.stack=2|!set_bonus.tww1_4pc)" );
  colossus_execute->add_action( "execute,if=rage>=40" );
  colossus_execute->add_action( "skullsplitter" );
  colossus_execute->add_action( "overpower" );
  colossus_execute->add_action( "bladestorm" );
  colossus_execute->add_action( "execute" );
  colossus_execute->add_action( "mortal_strike" );

  colossus_sweep->add_action( "sweeping_strikes" );
  colossus_sweep->add_action( "rend,if=dot.rend.remains<=gcd&buff.sweeping_strikes.up" );
  colossus_sweep->add_action( "thunderous_roar" );
  colossus_sweep->add_action( "champions_spear" );
  colossus_sweep->add_action( "ravager,if=cooldown.colossus_smash.ready" );
  colossus_sweep->add_action( "avatar" );
  colossus_sweep->add_action( "colossus_smash" );
  colossus_sweep->add_action( "warbreaker" );
  colossus_sweep->add_action( "overpower,if=action.overpower.charges=2&talent.dreadnaught|buff.sweeping_strikes.up" );
  colossus_sweep->add_action( "mortal_strike,if=buff.sweeping_strikes.up" );
  colossus_sweep->add_action( "skullsplitter,if=buff.sweeping_strikes.up" );
  colossus_sweep->add_action( "demolish,if=buff.sweeping_strikes.up&debuff.colossus_smash.up" );
  colossus_sweep->add_action( "mortal_strike,if=buff.sweeping_strikes.down" );
  colossus_sweep->add_action( "demolish,if=buff.avatar.up|debuff.colossus_smash.up&cooldown.avatar.remains>=35" );
  colossus_sweep->add_action( "execute,if=buff.recklessness_warlords_torment.up|buff.sweeping_strikes.up" );
  colossus_sweep->add_action( "overpower,if=charges=2|buff.sweeping_strikes.up" );
  colossus_sweep->add_action( "execute" );
  colossus_sweep->add_action( "thunder_clap,if=dot.rend.remains<=8&buff.sweeping_strikes.down" );
  colossus_sweep->add_action( "rend,if=dot.rend.remains<=5" );
  colossus_sweep->add_action( "cleave,if=talent.fervor_of_battle" );
  colossus_sweep->add_action( "whirlwind,if=talent.fervor_of_battle" );
  colossus_sweep->add_action( "slam" );

  colossus_aoe->add_action( "cleave,if=buff.collateral_damage.up&buff.merciless_bonegrinder.up" );
  colossus_aoe->add_action( "thunder_clap,if=!dot.rend.remains" );
  colossus_aoe->add_action( "thunderous_roar" );
  colossus_aoe->add_action( "avatar" );
  colossus_aoe->add_action( "ravager" );
  colossus_aoe->add_action( "sweeping_strikes" );
  colossus_aoe->add_action( "skullsplitter,if=buff.sweeping_strikes.up" );
  colossus_aoe->add_action( "warbreaker" );
  colossus_aoe->add_action( "bladestorm,if=talent.unhinged|talent.merciless_bonegrinder" );
  colossus_aoe->add_action( "champions_spear" );
  colossus_aoe->add_action( "colossus_smash" );
  colossus_aoe->add_action( "cleave" );
  colossus_aoe->add_action( "demolish,if=buff.sweeping_strikes.up" );
  colossus_aoe->add_action( "bladestorm,if=talent.unhinged" );
  colossus_aoe->add_action( "overpower" );
  colossus_aoe->add_action( "mortal_strike,if=buff.sweeping_strikes.up" );
  colossus_aoe->add_action( "overpower,if=buff.sweeping_strikes.up" );
  colossus_aoe->add_action( "execute,if=buff.sweeping_strikes.up" );
  colossus_aoe->add_action( "thunder_clap" );
  colossus_aoe->add_action( "mortal_strike" );
  colossus_aoe->add_action( "execute" );
  colossus_aoe->add_action( "bladestorm" );
  colossus_aoe->add_action( "whirlwind" );

  slayer_st->add_action( "rend,if=dot.rend.remains<=gcd" );
  slayer_st->add_action( "thunderous_roar" );
  slayer_st->add_action( "champions_spear" );
  slayer_st->add_action( "avatar" );
  slayer_st->add_action( "colossus_smash" );
  slayer_st->add_action( "warbreaker" );
  slayer_st->add_action( "execute,if=debuff.marked_for_execution.stack=3" );
  slayer_st->add_action( "bladestorm" );
  slayer_st->add_action( "overpower,if=buff.opportunist.up" );
  slayer_st->add_action( "mortal_strike" );
  slayer_st->add_action( "skullsplitter" );
  slayer_st->add_action( "execute" );
  slayer_st->add_action( "overpower" );
  slayer_st->add_action( "rend,if=dot.rend.remains<=gcd*5" );
  slayer_st->add_action( "cleave,if=buff.martial_prowess.down" );
  slayer_st->add_action( "slam" );
  slayer_st->add_action( "storm_bolt,if=buff.bladestorm.up" );

  slayer_execute->add_action( "sweeping_strikes,if=active_enemies=2" );
  slayer_execute->add_action( "rend,if=dot.rend.remains<=gcd&!talent.bloodletting" );
  slayer_execute->add_action( "thunderous_roar" );
  slayer_execute->add_action( "champions_spear" );
  slayer_execute->add_action( "avatar" );
  slayer_execute->add_action( "warbreaker" );
  slayer_execute->add_action( "colossus_smash" );
  slayer_execute->add_action( "execute,if=buff.juggernaut.remains<=gcd" );
  slayer_execute->add_action( "bladestorm,if=debuff.executioners_precision.stack=2&debuff.colossus_smash.remains>4|debuff.executioners_precision.stack=2&cooldown.colossus_smash.remains>15|!talent.executioners_precision" );
  slayer_execute->add_action( "mortal_strike,if=debuff.executioners_precision.stack=2&(buff.lethal_blows.stack=2|!set_bonus.tww1_4pc)" );
  slayer_execute->add_action( "skullsplitter,if=rage<85" );
  slayer_execute->add_action( "overpower,if=buff.opportunist.up&rage<80&buff.martial_prowess.stack<2" );
  slayer_execute->add_action( "execute" );
  slayer_execute->add_action( "overpower" );
  slayer_execute->add_action( "mortal_strike,if=!talent.executioners_precision" );
  slayer_execute->add_action( "storm_bolt,if=buff.bladestorm.up" );

  slayer_sweep->add_action( "thunderous_roar" );
  slayer_sweep->add_action( "sweeping_strikes" );
  slayer_sweep->add_action( "rend,if=dot.rend.remains<=gcd" );
  slayer_sweep->add_action( "champions_spear" );
  slayer_sweep->add_action( "avatar" );
  slayer_sweep->add_action( "colossus_smash" );
  slayer_sweep->add_action( "warbreaker" );
  slayer_sweep->add_action( "skullsplitter,if=buff.sweeping_strikes.up" );
  slayer_sweep->add_action( "execute,if=debuff.marked_for_execution.stack=3" );
  slayer_sweep->add_action( "bladestorm" );
  slayer_sweep->add_action( "overpower,if=talent.dreadnaught|buff.opportunist.up" );
  slayer_sweep->add_action( "mortal_strike" );
  slayer_sweep->add_action( "cleave,if=talent.fervor_of_battle" );
  slayer_sweep->add_action( "execute" );
  slayer_sweep->add_action( "overpower" );
  slayer_sweep->add_action( "thunder_clap,if=dot.rend.remains<=8&buff.sweeping_strikes.down" );
  slayer_sweep->add_action( "rend,if=dot.rend.remains<=5" );
  slayer_sweep->add_action( "whirlwind,if=talent.fervor_of_battle" );
  slayer_sweep->add_action( "slam" );
  slayer_sweep->add_action( "storm_bolt,if=buff.bladestorm.up" );

  slayer_aoe->add_action( "thunder_clap,if=!dot.rend.remains" );
  slayer_aoe->add_action( "sweeping_strikes" );
  slayer_aoe->add_action( "thunderous_roar" );
  slayer_aoe->add_action( "avatar" );
  slayer_aoe->add_action( "champions_spear" );
  slayer_aoe->add_action( "warbreaker" );
  slayer_aoe->add_action( "colossus_smash" );
  slayer_aoe->add_action( "cleave" );
  slayer_aoe->add_action( "overpower,if=buff.sweeping_strikes.up" );
  slayer_aoe->add_action( "execute,if=buff.sudden_death.up&buff.imminent_demise.stack<3" );
  slayer_aoe->add_action( "bladestorm" );
  slayer_aoe->add_action( "skullsplitter,if=buff.sweeping_strikes.up" );
  slayer_aoe->add_action( "execute,if=buff.sweeping_strikes.up&debuff.executioners_precision.stack<2" );
  slayer_aoe->add_action( "mortal_strike,if=buff.sweeping_strikes.up&debuff.executioners_precision.stack=2" );
  slayer_aoe->add_action( "execute,if=debuff.marked_for_execution.up" );
  slayer_aoe->add_action( "mortal_strike,if=buff.sweeping_strikes.up" );
  slayer_aoe->add_action( "overpower,if=talent.dreadnaught" );
  slayer_aoe->add_action( "thunder_clap" );
  slayer_aoe->add_action( "overpower" );
  slayer_aoe->add_action( "execute" );
  slayer_aoe->add_action( "mortal_strike" );
  slayer_aoe->add_action( "whirlwind" );
  slayer_aoe->add_action( "skullsplitter" );
  slayer_aoe->add_action( "slam" );
  slayer_aoe->add_action( "storm_bolt,if=buff.bladestorm.up" );

  trinkets->add_action( "do_treacherous_transmitter_task", "Trinkets" );
  trinkets->add_action( "use_item,name=treacherous_transmitter,if=(variable.adds_remain|variable.st_planning)&cooldown.avatar.remains<3" );
  trinkets->add_action( "use_item,slot=trinket1,if=variable.trinket_1_buffs&!variable.trinket_1_manual&(!buff.avatar.up&trinket.1.cast_time>0|!trinket.1.cast_time>0)&buff.avatar.up&(variable.trinket_2_exclude|!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1)|trinket.1.proc.any_dps.duration>=fight_remains", "Trinkets The trinket with the highest estimated value, will be used first and paired with Avatar." );
  trinkets->add_action( "use_item,slot=trinket2,if=variable.trinket_2_buffs&!variable.trinket_2_manual&(!buff.avatar.up&trinket.2.cast_time>0|!trinket.2.cast_time>0)&buff.avatar.up&(variable.trinket_1_exclude|!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2)|trinket.2.proc.any_dps.duration>=fight_remains" );
  trinkets->add_action( "use_item,slot=trinket1,if=!variable.trinket_1_buffs&(trinket.1.cast_time>0&!buff.avatar.up|!trinket.1.cast_time>0)&!variable.trinket_1_manual&(!variable.trinket_1_buffs&(trinket.2.cooldown.remains|!variable.trinket_2_buffs)|(trinket.1.cast_time>0&!buff.avatar.up|!trinket.1.cast_time>0)|cooldown.avatar.remains_expected>20)", "If only one on use trinket provides a buff, use the other on cooldown. Or if neither trinket provides a buff, use both on cooldown." );
  trinkets->add_action( "use_item,slot=trinket2,if=!variable.trinket_2_buffs&(trinket.2.cast_time>0&!buff.avatar.up|!trinket.2.cast_time>0)&!variable.trinket_2_manual&(!variable.trinket_2_buffs&(trinket.1.cooldown.remains|!variable.trinket_1_buffs)|(trinket.2.cast_time>0&!buff.avatar.up|!trinket.2.cast_time>0)|cooldown.avatar.remains_expected>20)" );
  trinkets->add_action( "use_item,slot=main_hand,if=!equipped.fyralath_the_dreamrender&(!variable.trinket_1_buffs|trinket.1.cooldown.remains)&(!variable.trinket_2_buffs|trinket.2.cooldown.remains)" );

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
  default_->add_action( "avatar" );
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
  default_->add_action( "demolish,if=buff.colossal_might.stack>=3" );
  default_->add_action( "thunderous_roar" );
  default_->add_action( "shockwave,if=talent.rumbling_earth.enabled&spell_targets.shockwave>=3" );
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

  generic->add_action( "thunder_blast,if=(buff.thunder_blast.stack=2&buff.burst_of_power.stack<=1&buff.avatar.up&talent.unstoppable_force.enabled)|rage<=70&talent.demolish.enabled" );
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
