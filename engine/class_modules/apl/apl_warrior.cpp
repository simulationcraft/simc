#include "class_modules/apl/apl_warrior.hpp"

#include "simulationcraft.hpp"

namespace warrior_apl
{
//fury_apl_start
void fury( player_t* p )
{
  std::vector<std::string> racial_actions = p->get_racial_actions();

  action_priority_list_t* default_  = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );

  action_priority_list_t* slayer    = p->get_action_priority_list( "slayer" );
  action_priority_list_t* thane     = p->get_action_priority_list( "thane" );
  action_priority_list_t* trinkets  = p->get_action_priority_list( "trinkets" );
  action_priority_list_t* variables = p->get_action_priority_list( "variables" );

  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "berserker_stance,toggle=on" );
  precombat->add_action( "variable,name=trinket_1_exclude,value=trinket.1.is.treacherous_transmitter" );
  precombat->add_action( "variable,name=trinket_2_exclude,value=trinket.2.is.treacherous_transmitter" );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=trinket.1.has_use_buff&(trinket.1.cooldown.duration%%cooldown.avatar.duration=0|trinket.1.cooldown.duration%%cooldown.odyns_fury.duration=0)", "Evaluates a trinkets cooldown, divided by avatar or odyns fury. If it's value has no remainder return 1, else return 0.5." );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=trinket.2.has_use_buff&(trinket.2.cooldown.duration%%cooldown.avatar.duration=0|trinket.2.cooldown.duration%%cooldown.odyns_fury.duration=0)" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_use_buff|(trinket.1.has_stat.any_dps&!variable.trinket_1_exclude)" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_use_buff|(trinket.2.has_stat.any_dps&!variable.trinket_2_exclude)" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs|variable.trinket_2_buffs&((trinket.2.cooldown.duration%trinket.2.proc.any_dps.duration)*(1.5+trinket.2.has_buff.strength)*(variable.trinket_2_sync))>((trinket.1.cooldown.duration%trinket.1.proc.any_dps.duration)*(1.5+trinket.1.has_buff.strength)*(variable.trinket_1_sync))" );
  precombat->add_action( "variable,name=trinket_1_manual,value=trinket.1.is.algethar_puzzle_box" );
  precombat->add_action( "variable,name=trinket_2_manual,value=trinket.2.is.algethar_puzzle_box" );
  precombat->add_action( "variable,name=treacherous_transmitter_precombat_cast,value=2" );
  precombat->add_action( "use_item,name=treacherous_transmitter" );
  precombat->add_action( "recklessness,if=!equipped.fyralath_the_dreamrender" );
  precombat->add_action( "avatar" );

  default_->add_action( "auto_attack" );
  default_->add_action( "charge,if=time<=0.5|movement.distance>5" );
  default_->add_action( "heroic_leap,if=(raid_event.movement.distance>25&raid_event.movement.in>45)" );
  default_->add_action( "potion,if=target.time_to_die>300|target.time_to_die<300&target.health.pct<35&buff.recklessness.up|target.time_to_die<25" );
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
  default_->add_action( "run_action_list,name=slayer,if=talent.slayers_dominance" );
  default_->add_action( "run_action_list,name=thane,if=talent.lightning_strikes" );

  slayer->add_action( "recklessness" );
  slayer->add_action( "avatar,if=cooldown.recklessness.remains" );
  slayer->add_action( "execute,if=buff.ashen_juggernaut.up&buff.ashen_juggernaut.remains<=gcd" );
  slayer->add_action( "champions_spear,if=buff.enrage.up&(cooldown.bladestorm.remains>=2|cooldown.bladestorm.remains>=16&debuff.marked_for_execution.stack=3)" );
  slayer->add_action( "bladestorm,if=buff.enrage.up&(talent.reckless_abandon&cooldown.avatar.remains>=24|talent.anger_management&cooldown.recklessness.remains>=24)" );
  slayer->add_action( "odyns_fury,if=(buff.enrage.up|talent.titanic_rage)&cooldown.avatar.remains" );
  slayer->add_action( "whirlwind,if=active_enemies>=2&talent.meat_cleaver&buff.meat_cleaver.stack=0" );
  slayer->add_action( "execute,if=buff.sudden_death.stack=2&buff.sudden_death.remains<7" );
  slayer->add_action( "execute,if=buff.sudden_death.up&buff.sudden_death.remains<2" );
  slayer->add_action( "execute,if=buff.sudden_death.up&buff.imminent_demise.stack<3&cooldown.bladestorm.remains<25" );
  slayer->add_action( "onslaught,if=talent.tenderize" );
  slayer->add_action( "rampage,if=!buff.enrage.up|buff.slaughtering_strikes.stack>=4" );
  slayer->add_action( "crushing_blow,if=action.raging_blow.charges=2|buff.brutal_finish.up&(!debuff.champions_might.up|debuff.champions_might.up&debuff.champions_might.remains>gcd)" );
  slayer->add_action( "thunderous_roar,if=buff.enrage.up&!buff.brutal_finish.up" );
  slayer->add_action( "execute,if=debuff.marked_for_execution.stack=3" );
  slayer->add_action( "bloodbath,if=buff.bloodcraze.stack>=1|(talent.uproar&dot.bloodbath_dot.remains<40&talent.bloodborne)|buff.enrage.up&buff.enrage.remains<gcd" );
  slayer->add_action( "raging_blow,if=buff.brutal_finish.up&buff.slaughtering_strikes.stack<5&(!debuff.champions_might.up|debuff.champions_might.up&debuff.champions_might.remains>gcd)" );
  slayer->add_action( "rampage,if=action.raging_blow.charges<=1&rage>=100&talent.anger_management&buff.recklessness.down" );
  slayer->add_action( "rampage,if=rage>=120|talent.reckless_abandon&buff.recklessness.up&buff.slaughtering_strikes.stack>=3" );
  slayer->add_action( "bloodbath,if=(buff.bloodcraze.stack>=4|crit_pct_current>=85)" );
  slayer->add_action( "crushing_blow" );
  slayer->add_action( "bloodbath" );
  slayer->add_action( "raging_blow,if=buff.opportunist.up" );
  slayer->add_action( "bloodthirst,if=target.health.pct<35&talent.vicious_contempt&buff.bloodcraze.stack>=2" );
  slayer->add_action( "rampage,if=rage>=100&talent.anger_management&buff.recklessness.up" );
  slayer->add_action( "bloodthirst,if=buff.bloodcraze.stack>=4|crit_pct_current>=85" );
  slayer->add_action( "raging_blow" );
  slayer->add_action( "bloodthirst" );
  slayer->add_action( "rampage" );
  slayer->add_action( "execute" );
  slayer->add_action( "whirlwind,if=talent.improved_whirlwind" );
  slayer->add_action( "slam,if=!talent.improved_whirlwind" );
  slayer->add_action( "storm_bolt,if=buff.bladestorm.up" );

  thane->add_action( "recklessness" );
  thane->add_action( "avatar" );
  thane->add_action( "ravager" );
  thane->add_action( "thunder_blast,if=buff.enrage.up&talent.meat_cleaver" );
  thane->add_action( "thunder_clap,if=buff.meat_cleaver.stack=0&talent.meat_cleaver&active_enemies>=2" );
  thane->add_action( "thunderous_roar,if=buff.enrage.up" );
  thane->add_action( "champions_spear,if=buff.enrage.up" );
  thane->add_action( "odyns_fury,if=(buff.enrage.up|talent.titanic_rage)&cooldown.avatar.remains" );
  thane->add_action( "rampage,if=buff.enrage.down" );
  thane->add_action( "execute,if=talent.ashen_juggernaut&buff.ashen_juggernaut.remains<=gcd" );
  thane->add_action( "rampage,if=talent.bladestorm&cooldown.bladestorm.remains<=gcd&!debuff.champions_might.up" );
  thane->add_action( "bladestorm,if=buff.enrage.up&talent.unhinged" );
  thane->add_action( "bloodbath,if=buff.bloodcraze.stack>=2" );
  thane->add_action( "rampage,if=rage>=115&talent.reckless_abandon&buff.recklessness.up&buff.slaughtering_strikes.stack>=3" );
  thane->add_action( "crushing_blow" );
  thane->add_action( "bloodbath" );
  thane->add_action( "onslaught,if=talent.tenderize" );
  thane->add_action( "rampage" );
  thane->add_action( "bloodthirst,if=talent.vicious_contempt&target.health.pct<35&buff.bloodcraze.stack>=2|!buff.ravager.up&buff.bloodcraze.stack>=3|active_enemies>=6" );
  thane->add_action( "raging_blow" );
  thane->add_action( "execute,if=talent.ashen_juggernaut" );
  thane->add_action( "thunder_blast" );
  thane->add_action( "bloodthirst" );
  thane->add_action( "execute" );
  thane->add_action( "thunder_clap" );

  trinkets->add_action( "do_treacherous_transmitter_task", "Trinkets" );
  trinkets->add_action( "use_item,name=treacherous_transmitter,if=variable.adds_remain|variable.st_planning" );
  trinkets->add_action( "use_item,slot=trinket1,if=variable.trinket_1_buffs&!variable.trinket_1_manual&(!buff.avatar.up&trinket.1.cast_time>0|!trinket.1.cast_time>0)&buff.avatar.up&(variable.trinket_2_exclude|!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1)|trinket.1.proc.any_dps.duration>=fight_remains", "Trinkets The trinket with the highest estimated value, will be used first and paired with Avatar." );
  trinkets->add_action( "use_item,slot=trinket2,if=variable.trinket_2_buffs&!variable.trinket_2_manual&(!buff.avatar.up&trinket.2.cast_time>0|!trinket.2.cast_time>0)&buff.avatar.up&(variable.trinket_1_exclude|!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2)|trinket.2.proc.any_dps.duration>=fight_remains" );
  trinkets->add_action( "use_item,slot=trinket1,if=!variable.trinket_1_buffs&(trinket.1.cast_time>0&!buff.avatar.up|!trinket.1.cast_time>0)&!variable.trinket_1_manual&(!variable.trinket_1_buffs&(trinket.2.cooldown.remains|!variable.trinket_2_buffs)|(trinket.1.cast_time>0&!buff.avatar.up|!trinket.1.cast_time>0)|cooldown.avatar.remains_expected>20)", "If only one on use trinket provides a buff, use the other on cooldown. Or if neither trinket provides a buff, use both on cooldown." );
  trinkets->add_action( "use_item,slot=trinket2,if=!variable.trinket_2_buffs&(trinket.2.cast_time>0&!buff.avatar.up|!trinket.2.cast_time>0)&!variable.trinket_2_manual&(!variable.trinket_2_buffs&(trinket.1.cooldown.remains|!variable.trinket_1_buffs)|(trinket.2.cast_time>0&!buff.avatar.up|!trinket.2.cast_time>0)|cooldown.avatar.remains_expected>20)" );
  trinkets->add_action( "use_item,slot=main_hand,if=!equipped.fyralath_the_dreamrender&!equipped.bestinslots&(!variable.trinket_1_buffs|trinket.1.cooldown.remains)&(!variable.trinket_2_buffs|trinket.2.cooldown.remains)" );
  trinkets->add_action( "use_item,name=bestinslots,if=target.time_to_die>120&(cooldown.avatar.remains>20&(trinket.1.cooldown.remains|trinket.2.cooldown.remains)|cooldown.avatar.remains>20&(!trinket.1.has_cooldown|!trinket.2.has_cooldown))|target.time_to_die<=120&target.health.pct<35&cooldown.avatar.remains>85|target.time_to_die<15" );

  variables->add_action( "variable,name=st_planning,value=active_enemies=1&(raid_event.adds.in>15|!raid_event.adds.exists)", "Variables" );
  variables->add_action( "variable,name=adds_remain,value=active_enemies>=2&(!raid_event.adds.exists|raid_event.adds.exists&raid_event.adds.remains>5)" );
  variables->add_action( "variable,name=execute_phase,value=(talent.massacre.enabled&target.health.pct<35)|target.health.pct<20" );
  variables->add_action( "variable,name=on_gcd_racials,value=buff.recklessness.down&buff.avatar.down&rage<80&buff.sudden_death.down&!cooldown.bladestorm.ready&(!cooldown.execute.ready|!variable.execute_phase)" );
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

  colossus_aoe->add_action( "cleave,if=!dot.deep_wounds.remains" );
  colossus_aoe->add_action( "thunder_clap,if=!dot.rend.remains" );
  colossus_aoe->add_action( "thunderous_roar" );
  colossus_aoe->add_action( "avatar" );
  colossus_aoe->add_action( "sweeping_strikes" );
  colossus_aoe->add_action( "ravager" );
  colossus_aoe->add_action( "warbreaker" );
  colossus_aoe->add_action( "champions_spear" );
  colossus_aoe->add_action( "colossus_smash" );
  colossus_aoe->add_action( "cleave" );
  colossus_aoe->add_action( "bladestorm,if=talent.unhinged|talent.merciless_bonegrinder" );
  colossus_aoe->add_action( "thunder_clap,if=dot.rend.remains<5" );
  colossus_aoe->add_action( "demolish,if=buff.colossal_might.stack=10&(debuff.colossus_smash.remains>=2|cooldown.colossus_smash.remains>=7)" );
  colossus_aoe->add_action( "overpower,if=talent.dreadnaught" );
  colossus_aoe->add_action( "mortal_strike" );
  colossus_aoe->add_action( "overpower" );
  colossus_aoe->add_action( "thunder_clap" );
  colossus_aoe->add_action( "skullsplitter" );
  colossus_aoe->add_action( "execute" );
  colossus_aoe->add_action( "bladestorm" );
  colossus_aoe->add_action( "whirlwind" );

  colossus_execute->add_action( "sweeping_strikes,if=active_enemies=2" );
  colossus_execute->add_action( "rend,if=dot.rend.remains<=gcd&!talent.bloodletting" );
  colossus_execute->add_action( "thunderous_roar" );
  colossus_execute->add_action( "champions_spear" );
  colossus_execute->add_action( "ravager,if=cooldown.colossus_smash.remains<=gcd" );
  colossus_execute->add_action( "avatar" );
  colossus_execute->add_action( "colossus_smash" );
  colossus_execute->add_action( "warbreaker" );
  colossus_execute->add_action( "execute,if=buff.juggernaut.remains<=gcd&talent.juggernaut" );
  colossus_execute->add_action( "skullsplitter,if=rage<40" );
  colossus_execute->add_action( "demolish,if=debuff.colossus_smash.up" );
  colossus_execute->add_action( "mortal_strike,if=debuff.executioners_precision.stack=2&!buff.ravager.up|!talent.executioners_precision|talent.battlelord&debuff.executioners_precision.stack>0" );
  colossus_execute->add_action( "overpower,if=rage<90" );
  colossus_execute->add_action( "execute,if=rage>=40&talent.executioners_precision" );
  colossus_execute->add_action( "overpower" );
  colossus_execute->add_action( "bladestorm" );
  colossus_execute->add_action( "execute" );

  colossus_st->add_action( "rend,if=dot.rend.remains<=gcd" );
  colossus_st->add_action( "thunderous_roar" );
  colossus_st->add_action( "ravager,if=cooldown.colossus_smash.remains<=gcd" );
  colossus_st->add_action( "champions_spear" );
  colossus_st->add_action( "avatar,if=raid_event.adds.in>15" );
  colossus_st->add_action( "colossus_smash" );
  colossus_st->add_action( "warbreaker" );
  colossus_st->add_action( "mortal_strike" );
  colossus_st->add_action( "demolish" );
  colossus_st->add_action( "skullsplitter" );
  colossus_st->add_action( "execute" );
  colossus_st->add_action( "overpower" );
  colossus_st->add_action( "rend,if=dot.rend.remains<=gcd*5" );
  colossus_st->add_action( "slam" );

  colossus_sweep->add_action( "thunder_clap,if=!dot.rend.remains&!buff.sweeping_strikes.up" );
  colossus_sweep->add_action( "rend,if=dot.rend.remains<=gcd&buff.sweeping_strikes.up" );
  colossus_sweep->add_action( "thunderous_roar" );
  colossus_sweep->add_action( "sweeping_strikes" );
  colossus_sweep->add_action( "champions_spear" );
  colossus_sweep->add_action( "ravager,if=cooldown.colossus_smash.ready" );
  colossus_sweep->add_action( "avatar" );
  colossus_sweep->add_action( "colossus_smash" );
  colossus_sweep->add_action( "warbreaker" );
  colossus_sweep->add_action( "mortal_strike" );
  colossus_sweep->add_action( "demolish,if=debuff.colossus_smash.up" );
  colossus_sweep->add_action( "overpower" );
  colossus_sweep->add_action( "execute" );
  colossus_sweep->add_action( "whirlwind,if=talent.fervor_of_battle" );
  colossus_sweep->add_action( "cleave,if=talent.fervor_of_battle" );
  colossus_sweep->add_action( "thunder_clap,if=dot.rend.remains<=8&buff.sweeping_strikes.down" );
  colossus_sweep->add_action( "rend,if=dot.rend.remains<=5" );
  colossus_sweep->add_action( "slam" );

  slayer_aoe->add_action( "thunder_clap,if=!dot.rend.remains" );
  slayer_aoe->add_action( "sweeping_strikes" );
  slayer_aoe->add_action( "thunderous_roar" );
  slayer_aoe->add_action( "avatar" );
  slayer_aoe->add_action( "champions_spear" );
  slayer_aoe->add_action( "ravager,if=cooldown.colossus_smash.remains<=gcd" );
  slayer_aoe->add_action( "warbreaker" );
  slayer_aoe->add_action( "colossus_smash" );
  slayer_aoe->add_action( "cleave" );
  slayer_aoe->add_action( "execute,if=buff.sudden_death.up&buff.imminent_demise.stack<3|buff.juggernaut.remains<3&talent.juggernaut" );
  slayer_aoe->add_action( "bladestorm" );
  slayer_aoe->add_action( "overpower,if=buff.sweeping_strikes.up&(buff.opportunist.up|talent.dreadnaught&!talent.juggernaut)" );
  slayer_aoe->add_action( "mortal_strike,if=buff.sweeping_strikes.up" );
  slayer_aoe->add_action( "execute,if=buff.sweeping_strikes.up&debuff.executioners_precision.stack<2&talent.executioners_precision|debuff.marked_for_execution.up" );
  slayer_aoe->add_action( "skullsplitter,if=buff.sweeping_strikes.up" );
  slayer_aoe->add_action( "overpower,if=buff.opportunist.up|talent.dreadnaught" );
  slayer_aoe->add_action( "mortal_strike" );
  slayer_aoe->add_action( "overpower" );
  slayer_aoe->add_action( "thunder_clap" );
  slayer_aoe->add_action( "execute" );
  slayer_aoe->add_action( "whirlwind" );
  slayer_aoe->add_action( "skullsplitter" );
  slayer_aoe->add_action( "slam" );
  slayer_aoe->add_action( "storm_bolt,if=buff.bladestorm.up" );

  slayer_execute->add_action( "sweeping_strikes,if=active_enemies=2" );
  slayer_execute->add_action( "rend,if=dot.rend.remains<=gcd&!talent.bloodletting" );
  slayer_execute->add_action( "thunderous_roar" );
  slayer_execute->add_action( "avatar,if=cooldown.colossus_smash.remains<=5|debuff.colossus_smash.up" );
  slayer_execute->add_action( "champions_spear,if=debuff.colossus_smash.up|buff.avatar.up" );
  slayer_execute->add_action( "ravager,if=cooldown.colossus_smash.remains<=gcd" );
  slayer_execute->add_action( "warbreaker" );
  slayer_execute->add_action( "colossus_smash" );
  slayer_execute->add_action( "execute,if=buff.juggernaut.remains<=gcd*2&talent.juggernaut" );
  slayer_execute->add_action( "bladestorm,if=(debuff.executioners_precision.stack=2&(debuff.colossus_smash.remains>4|cooldown.colossus_smash.remains>15))|!talent.executioners_precision" );
  slayer_execute->add_action( "skullsplitter,if=rage<=40" );
  slayer_execute->add_action( "overpower,if=buff.martial_prowess.stack<2&buff.opportunist.up&talent.opportunist&(talent.bladestorm|talent.ravager&rage<85)" );
  slayer_execute->add_action( "mortal_strike,if=dot.rend.remains<2|debuff.executioners_precision.stack=2&!buff.ravager.up" );
  slayer_execute->add_action( "overpower,if=rage<=40&buff.martial_prowess.stack<2&talent.fierce_followthrough" );
  slayer_execute->add_action( "execute" );
  slayer_execute->add_action( "overpower" );
  slayer_execute->add_action( "storm_bolt,if=buff.bladestorm.up" );

  slayer_st->add_action( "rend,if=dot.rend.remains<=gcd" );
  slayer_st->add_action( "thunderous_roar" );
  slayer_st->add_action( "avatar,if=cooldown.colossus_smash.remains<=5|debuff.colossus_smash.up" );
  slayer_st->add_action( "champions_spear,if=debuff.colossus_smash.up|buff.avatar.up" );
  slayer_st->add_action( "ravager,if=cooldown.colossus_smash.remains<=gcd" );
  slayer_st->add_action( "colossus_smash" );
  slayer_st->add_action( "warbreaker" );
  slayer_st->add_action( "execute,if=buff.juggernaut.remains<=gcd*2&talent.juggernaut|buff.sudden_death.stack=2|buff.sudden_death.remains<=gcd*3|debuff.marked_for_execution.stack=3" );
  slayer_st->add_action( "overpower,if=buff.opportunist.up" );
  slayer_st->add_action( "mortal_strike" );
  slayer_st->add_action( "bladestorm,if=(cooldown.colossus_smash.remains>=gcd*4|cooldown.warbreaker.remains>=gcd*4)|debuff.colossus_smash.remains>=gcd*4" );
  slayer_st->add_action( "skullsplitter" );
  slayer_st->add_action( "overpower" );
  slayer_st->add_action( "rend,if=dot.rend.remains<=8" );
  slayer_st->add_action( "execute,if=!talent.juggernaut" );
  slayer_st->add_action( "cleave" );
  slayer_st->add_action( "slam" );
  slayer_st->add_action( "storm_bolt,if=buff.bladestorm.up" );

  slayer_sweep->add_action( "thunder_clap,if=!dot.rend.remains&!buff.sweeping_strikes.up" );
  slayer_sweep->add_action( "thunderous_roar" );
  slayer_sweep->add_action( "sweeping_strikes" );
  slayer_sweep->add_action( "rend,if=dot.rend.remains<=gcd" );
  slayer_sweep->add_action( "champions_spear" );
  slayer_sweep->add_action( "avatar" );
  slayer_sweep->add_action( "colossus_smash" );
  slayer_sweep->add_action( "warbreaker" );
  slayer_sweep->add_action( "skullsplitter,if=buff.sweeping_strikes.up" );
  slayer_sweep->add_action( "execute,if=buff.juggernaut.remains<=gcd*2|debuff.marked_for_execution.stack=3|buff.sudden_death.stack=2|buff.sudden_death.remains<=gcd*3" );
  slayer_sweep->add_action( "bladestorm,if=(cooldown.colossus_smash.remains>=gcd*4|cooldown.warbreaker.remains>=gcd*4)|debuff.colossus_smash.remains>=gcd*4" );
  slayer_sweep->add_action( "overpower,if=buff.opportunist.up" );
  slayer_sweep->add_action( "mortal_strike" );
  slayer_sweep->add_action( "overpower" );
  slayer_sweep->add_action( "thunder_clap,if=dot.rend.remains<=8&buff.sweeping_strikes.down" );
  slayer_sweep->add_action( "rend,if=dot.rend.remains<=5" );
  slayer_sweep->add_action( "cleave,if=talent.fervor_of_battle&!buff.martial_prowess.up" );
  slayer_sweep->add_action( "whirlwind,if=talent.fervor_of_battle" );
  slayer_sweep->add_action( "execute,if=!talent.juggernaut" );
  slayer_sweep->add_action( "slam" );
  slayer_sweep->add_action( "storm_bolt,if=buff.bladestorm.up" );

  trinkets->add_action( "do_treacherous_transmitter_task", "Trinkets" );
  trinkets->add_action( "use_item,name=treacherous_transmitter,if=(variable.adds_remain|variable.st_planning)&cooldown.avatar.remains<3" );
  trinkets->add_action( "use_item,slot=trinket1,if=variable.trinket_1_buffs&!variable.trinket_1_manual&(!buff.avatar.up&trinket.1.cast_time>0|!trinket.1.cast_time>0)&buff.avatar.up&(variable.trinket_2_exclude|!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1)|trinket.1.proc.any_dps.duration>=fight_remains", "Trinkets The trinket with the highest estimated value, will be used first and paired with Avatar." );
  trinkets->add_action( "use_item,slot=trinket2,if=variable.trinket_2_buffs&!variable.trinket_2_manual&(!buff.avatar.up&trinket.2.cast_time>0|!trinket.2.cast_time>0)&buff.avatar.up&(variable.trinket_1_exclude|!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2)|trinket.2.proc.any_dps.duration>=fight_remains" );
  trinkets->add_action( "use_item,slot=trinket1,if=!variable.trinket_1_buffs&(trinket.1.cast_time>0&!buff.avatar.up|!trinket.1.cast_time>0)&!variable.trinket_1_manual&(!variable.trinket_1_buffs&(trinket.2.cooldown.remains|!variable.trinket_2_buffs)|(trinket.1.cast_time>0&!buff.avatar.up|!trinket.1.cast_time>0)|cooldown.avatar.remains_expected>20)", "If only one on use trinket provides a buff, use the other on cooldown. Or if neither trinket provides a buff, use both on cooldown." );
  trinkets->add_action( "use_item,slot=trinket2,if=!variable.trinket_2_buffs&(trinket.2.cast_time>0&!buff.avatar.up|!trinket.2.cast_time>0)&!variable.trinket_2_manual&(!variable.trinket_2_buffs&(trinket.1.cooldown.remains|!variable.trinket_1_buffs)|(trinket.2.cast_time>0&!buff.avatar.up|!trinket.2.cast_time>0)|cooldown.avatar.remains_expected>20)" );
  trinkets->add_action( "use_item,slot=main_hand,if=!equipped.fyralath_the_dreamrender&!equipped.bestinslots&(!variable.trinket_1_buffs|trinket.1.cooldown.remains)&(!variable.trinket_2_buffs|trinket.2.cooldown.remains)" );
  trinkets->add_action( "use_item,name=bestinslots,if=cooldown.avatar.remains>20|(buff.avatar.up&(!trinket.1.has_cooldown&!trinket.2.has_cooldown))" );

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
  default_->add_action( "shield_block,if=buff.shield_block.remains<=10" );
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
  generic->add_action( "revenge" );
  generic->add_action( "thunder_blast,if=(spell_targets.thunder_clap>=1|cooldown.shield_slam.remains&buff.violent_outburst.up)" );
  generic->add_action( "thunder_clap,if=(spell_targets.thunder_clap>=1|cooldown.shield_slam.remains&buff.violent_outburst.up)" );
  generic->add_action( "devastate" );
}
//protection_apl_end
}  // namespace warrior_apl
