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
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_use_buff" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_use_buff" );
  precombat->add_action( "variable,name=trinket_1_duration,op=setif,value=0,value_else=trinket.1.proc.any_dps.duration,condition=0" );
  precombat->add_action( "variable,name=trinket_2_duration,op=setif,value=0,value_else=trinket.2.proc.any_dps.duration,condition=0" );
  precombat->add_action( "variable,name=trinket_1_high_value,op=setif,value=2,value_else=1,condition=trinket.1.is.treacherous_transmitter" );
  precombat->add_action( "variable,name=trinket_2_high_value,op=setif,value=2,value_else=1,condition=trinket.2.is.treacherous_transmitter" );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_1_buffs&talent.recklessness&trinket.1.cooldown.duration%%cooldown.recklessness.duration=0" );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_2_buffs&talent.recklessness&trinket.2.cooldown.duration%%cooldown.recklessness.duration=0" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs&(trinket.2.has_cooldown|!trinket.1.has_cooldown)|variable.trinket_2_buffs&((trinket.2.cooldown.duration%variable.trinket_2_duration)*(1.5+trinket.2.has_buff.strength)*(variable.trinket_2_sync)*(variable.trinket_2_high_value)*(1+((trinket.2.ilvl-trinket.1.ilvl)%100)))>((trinket.1.cooldown.duration%variable.trinket_1_duration)*(1.5+trinket.1.has_buff.strength)*(variable.trinket_1_sync)*(variable.trinket_1_high_value)*(1+((trinket.1.ilvl-trinket.2.ilvl)%100)))" );
  precombat->add_action( "variable,name=damage_trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&!variable.trinket_2_buffs&trinket.2.ilvl>=trinket.1.ilvl" );
  precombat->add_action( "recklessness" );
  precombat->add_action( "avatar,if=!equipped.cursed_stone_idol" );

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
  default_->add_action( "invoke_external_buff,name=power_infusion,if=buff.recklessness.remains>15&fight_remains>=135|variable.execute_phase&buff.recklessness.up|fight_remains<=25" );
  default_->add_action( "run_action_list,name=slayer,if=talent.slayers_dominance" );
  default_->add_action( "run_action_list,name=thane,if=talent.lightning_strikes" );

  slayer->add_action( "recklessness" );
  slayer->add_action( "avatar" );
  slayer->add_action( "execute,if=buff.sudden_death.stack=2&buff.enrage.up" );
  slayer->add_action( "rampage,if=buff.enrage.remains<gcd" );
  slayer->add_action( "bladestorm,if=buff.enrage.remains>1" );
  slayer->add_action( "whirlwind,if=active_enemies>=2&talent.improved_whirlwind&buff.whirlwind.stack=0" );
  slayer->add_action( "execute" );
  slayer->add_action( "rampage,if=rage>115" );
  slayer->add_action( "crushing_blow" );
  slayer->add_action( "raging_blow" );
  slayer->add_action( "odyns_fury" );
  slayer->add_action( "champions_leap,if=buff.enrage.up" );
  slayer->add_action( "champions_spear,if=buff.enrage.up" );
  slayer->add_action( "rampage" );
  slayer->add_action( "bloodbath" );
  slayer->add_action( "bloodthirst" );
  slayer->add_action( "rend,if=dot.rend.duration<6" );
  slayer->add_action( "wrecking_throw" );
  slayer->add_action( "whirlwind" );
  slayer->add_action( "storm_bolt,if=buff.bladestorm.up" );

  thane->add_action( "recklessness" );
  thane->add_action( "avatar,if=!buff.thunder_blast.up" );
  thane->add_action( "odyns_fury,if=buff.enrage.down&!talent.reckless_abandon&active_enemies>=2" );
  thane->add_action( "thunder_clap,if=buff.whirlwind.stack=0&talent.improved_whirlwind&active_enemies>=2" );
  thane->add_action( "thunder_blast,if=buff.whirlwind.stack=0&buff.enrage.up&talent.improved_whirlwind&active_enemies>=2" );
  thane->add_action( "thunder_blast,if=cooldown.avatar.remains<5" );
  thane->add_action( "rampage,if=buff.enrage.remains<gcd" );
  thane->add_action( "rampage,if=buff.recklessness.up&rage>=130" );
  thane->add_action( "bloodbath,if=active_enemies>=3&talent.vicious_contempt&buff.thunder_blast.stack<2&target.health.pct<35" );
  thane->add_action( "thunder_blast,if=talent.meat_cleaver&active_enemies>=3&buff.avatar.up&buff.thunder_blast.stack=2" );
  thane->add_action( "crushing_blow,if=action.raging_blow.charges=2&(buff.ragedrinker.up|(talent.surge_of_adrenaline&!buff.surge_of_adrenaline.up))" );
  thane->add_action( "rampage,if=buff.recklessness.up&rage>=115" );
  thane->add_action( "thunder_blast,if=buff.enrage.up&(buff.thunder_blast.stack=2|buff.thunder_blast.remains<=2|buff.avatar.remains<=2)" );
  thane->add_action( "bloodbath,if=active_enemies>=3&buff.thunder_blast.stack<2" );
  thane->add_action( "thunder_blast,if=talent.meat_cleaver&active_enemies>=3&buff.avatar.up" );
  thane->add_action( "bloodbath,if=talent.vicious_contempt&target.health.pct<35" );
  thane->add_action( "thunder_clap,if=talent.meat_cleaver&active_enemies>=3&buff.avatar.up" );
  thane->add_action( "rampage,if=action.raging_blow.charges=0&talent.hack_and_slash" );
  thane->add_action( "rampage,if=buff.enrage.down" );
  thane->add_action( "odyns_fury,if=active_enemies>=2" );
  thane->add_action( "bloodbath" );
  thane->add_action( "crushing_blow" );
  thane->add_action( "odyns_fury" );
  thane->add_action( "thunder_clap,if=talent.meat_cleaver&active_enemies>=3" );
  thane->add_action( "bloodthirst,if=talent.vicious_contempt&target.health.pct<35" );
  thane->add_action( "rampage,if=rage>=130" );
  thane->add_action( "bloodthirst,if=active_enemies>=3" );
  thane->add_action( "thunder_blast,if=buff.enrage.up&buff.avatar.up&!buff.recklessness.up" );
  thane->add_action( "execute,if=talent.deep_wounds" );
  thane->add_action( "rampage,if=rage>=100" );
  thane->add_action( "raging_blow,if=action.raging_blow.charges=2" );
  thane->add_action( "rampage,if=rage>=90" );
  thane->add_action( "execute,if=talent.executioners_wrath" );
  thane->add_action( "bloodthirst" );
  thane->add_action( "thunder_blast,if=buff.enrage.up" );
  thane->add_action( "rampage" );
  thane->add_action( "raging_blow" );
  thane->add_action( "execute" );
  thane->add_action( "champions_leap" );
  thane->add_action( "champions_spear" );
  thane->add_action( "thunder_clap" );
  thane->add_action( "wrecking_throw" );
  thane->add_action( "whirlwind" );

  trinkets->add_action( "use_item,name=cursed_stone_idol,if=cooldown.recklessness.remains<2" );
  trinkets->add_action( "use_item,name=unyielding_netherprism,if=cooldown.recklessness.remains<=85" );
  trinkets->add_action( "use_item,slot=trinket1,if=variable.trinket_1_buffs&(variable.trinket_priority=1|!variable.trinket_2_buffs|!trinket.2.has_cooldown)&(buff.recklessness.up)", "Trinkets" );
  trinkets->add_action( "use_item,slot=trinket2,if=variable.trinket_2_buffs&(variable.trinket_priority=2|!variable.trinket_1_buffs|!trinket.1.has_cooldown)&(buff.recklessness.up)" );
  trinkets->add_action( "use_item,slot=trinket1,if=!variable.trinket_1_buffs&(variable.damage_trinket_priority=1|!variable.trinket_2_buffs|!trinket.2.has_cooldown)" );
  trinkets->add_action( "use_item,slot=trinket2,if=!variable.trinket_2_buffs&(variable.damage_trinket_priority=2|!variable.trinket_1_buffs|!trinket.1.has_cooldown)" );

  variables->add_action( "variable,name=st_planning,value=active_enemies=1&(raid_event.adds.in>15|!raid_event.adds.exists)", "Variables" );
  variables->add_action( "variable,name=adds_remain,value=active_enemies>=2&(!raid_event.adds.exists|raid_event.adds.exists&raid_event.adds.remains>5)" );
  variables->add_action( "variable,name=execute_phase,value=(talent.massacre.enabled&target.health.pct<35)|target.health.pct<20" );
  variables->add_action( "variable,name=on_gcd_racials,value=buff.recklessness.down&buff.recklessness.down&rage<80&buff.sudden_death.down&!cooldown.bladestorm.ready&(!cooldown.execute.ready|!variable.execute_phase)" );
}
//fury_apl_end

//arms_apl_start
void arms( player_t* p )
{
  std::vector<std::string> racial_actions = p->get_racial_actions();

  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* colossus_aoe     = p->get_action_priority_list( "colossus_aoe" );
  //action_priority_list_t* colossus_sweep = p->get_action_priority_list( "colossus_sweep" );
  action_priority_list_t* colossus_execute = p->get_action_priority_list( "colossus_execute" );
  action_priority_list_t* colossus_st = p->get_action_priority_list( "colossus_st" );
  action_priority_list_t* slayer_aoe       = p->get_action_priority_list( "slayer_aoe" );
  //action_priority_list_t* slayer_sweep     = p->get_action_priority_list( "slayer_sweep" );
  action_priority_list_t* slayer_execute   = p->get_action_priority_list( "slayer_execute" );
  action_priority_list_t* slayer_st        = p->get_action_priority_list( "slayer_st" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );
  action_priority_list_t* variables = p->get_action_priority_list( "variables" );

  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_use_buff" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_use_buff" );
  precombat->add_action( "variable,name=trinket_1_duration,op=setif,value=0,value_else=trinket.1.proc.any_dps.duration,condition=0" );
  precombat->add_action( "variable,name=trinket_2_duration,op=setif,value=0,value_else=trinket.2.proc.any_dps.duration,condition=0" );
  precombat->add_action( "variable,name=trinket_1_high_value,op=setif,value=2,value_else=1,condition=trinket.1.is.treacherous_transmitter" );
  precombat->add_action( "variable,name=trinket_2_high_value,op=setif,value=2,value_else=1,condition=trinket.2.is.treacherous_transmitter" );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_1_buffs&talent.avatar&trinket.1.cooldown.duration%%cooldown.avatar.duration=0" );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_2_buffs&talent.avatar&trinket.2.cooldown.duration%%cooldown.avatar.duration=0" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs&(trinket.2.has_cooldown|!trinket.1.has_cooldown)|variable.trinket_2_buffs&((trinket.2.cooldown.duration%variable.trinket_2_duration)*(1.5+trinket.2.has_buff.strength)*(variable.trinket_2_sync)*(variable.trinket_2_high_value)*(1+((trinket.2.ilvl-trinket.1.ilvl)%100)))>((trinket.1.cooldown.duration%variable.trinket_1_duration)*(1.5+trinket.1.has_buff.strength)*(variable.trinket_1_sync)*(variable.trinket_1_high_value)*(1+((trinket.1.ilvl-trinket.2.ilvl)%100)))" );
  precombat->add_action( "variable,name=damage_trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&!variable.trinket_2_buffs&trinket.2.ilvl>=trinket.1.ilvl" );
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
  default_->add_action( "berserking,if=target.time_to_die>180&debuff.colossus_smash.up|target.time_to_die<180&variable.execute_phase&debuff.colossus_smash.up|target.time_to_die<20" );
  default_->add_action( "blood_fury,if=debuff.colossus_smash.up" );
  default_->add_action( "fireblood,if=debuff.colossus_smash.up" );
  default_->add_action( "ancestral_call,if=debuff.colossus_smash.up" );
  default_->add_action( "invoke_external_buff,name=power_infusion,if=debuff.colossus_smash.up&fight_remains>=135|variable.execute_phase&buff.avatar.up|fight_remains<=25" );
  default_->add_action( "run_action_list,name=colossus_aoe,if=talent.demolish&active_enemies>2" );
  default_->add_action( "run_action_list,name=colossus_execute,target_if=min:target.health.pct,if=talent.demolish&variable.execute_phase" );
  default_->add_action( "run_action_list,name=colossus_st,if=talent.demolish" );
  default_->add_action( "run_action_list,name=slayer_aoe,if=talent.slayers_dominance&active_enemies>2" );
  default_->add_action( "run_action_list,name=slayer_execute,target_if=min:target.health.pct,if=talent.slayers_dominance&variable.execute_phase" );
  default_->add_action( "run_action_list,name=slayer_st,if=talent.slayers_dominance" );

  colossus_aoe->add_action( "rend,if=!dot.rend.remains<=gcd" );
  colossus_aoe->add_action( "sweeping_strikes,if=(!buff.sweeping_strikes.up&cooldown.colossus_smash.remains>4)|!talent.broad_strokes" );
  colossus_aoe->add_action( "ravager" );
  colossus_aoe->add_action( "colossus_smash" );
  colossus_aoe->add_action( "avatar" );
  colossus_aoe->add_action( "demolish,if=buff.colossal_might.stack=10&(debuff.colossus_smash.remains>=2|cooldown.colossus_smash.remains>=10)" );
  colossus_aoe->add_action( "overpower,if=charges=2" );
  colossus_aoe->add_action( "cleave" );
  colossus_aoe->add_action( "mortal_strike" );
  colossus_aoe->add_action( "execute,if=buff.sudden_death.up" );
  colossus_aoe->add_action( "champions_leap" );
  colossus_aoe->add_action( "champions_spear" );
  colossus_aoe->add_action( "overpower" );
  colossus_aoe->add_action( "wrecking_throw" );
  colossus_aoe->add_action( "rend" );
  colossus_aoe->add_action( "execute,if=talent.improved_execute" );
  colossus_aoe->add_action( "whirlwind" );

  colossus_execute->add_action( "sweeping_strikes,if=active_enemies=2" );
  colossus_execute->add_action( "rend,if=dot.rend.remains<=gcd&!talent.bloodletting" );
  colossus_execute->add_action( "ravager,if=cooldown.colossus_smash.remains<=gcd" );
  colossus_execute->add_action( "avatar" );
  colossus_execute->add_action( "colossus_smash" );
  colossus_execute->add_action( "demolish,if=debuff.colossus_smash.up&buff.colossal_might.stack=10" );
  colossus_execute->add_action( "mortal_strike,if=buff.executioners_precision.stack=2|!talent.executioners_precision" );
  colossus_execute->add_action( "overpower,if=charges=2" );
  colossus_execute->add_action( "execute,if=rage>=40&talent.executioners_precision" );
  colossus_execute->add_action( "champions_leap" );
  colossus_execute->add_action( "champions_spear" );
  colossus_execute->add_action( "overpower" );
  colossus_execute->add_action( "wrecking_throw" );
  colossus_execute->add_action( "execute" );

  colossus_st->add_action( "rend,if=dot.rend.remains<=gcd" );
  colossus_st->add_action( "ravager" );
  colossus_st->add_action( "avatar" );
  colossus_st->add_action( "colossus_smash" );
  colossus_st->add_action( "demolish,if=debuff.colossus_smash.up&buff.colossal_might.stack>0" );
  colossus_st->add_action( "overpower,if=buff.tactical_edge.up&charges=2" );
  colossus_st->add_action( "mortal_strike" );
  colossus_st->add_action( "execute" );
  colossus_st->add_action( "champions_leap" );
  colossus_st->add_action( "champions_spear" );
  colossus_st->add_action( "overpower" );
  colossus_st->add_action( "wrecking_throw" );
  colossus_st->add_action( "rend,if=dot.rend.remains<=gcd*5" );
  colossus_st->add_action( "slam" );

  slayer_aoe->add_action( "rend,if=!dot.rend.remains&talent.rend" );
  slayer_aoe->add_action( "sweeping_strikes,if=(!buff.sweeping_strikes.up&cooldown.colossus_smash.remains>4)|!talent.broad_strokes" );
  slayer_aoe->add_action( "avatar" );
  slayer_aoe->add_action( "colossus_smash" );
  slayer_aoe->add_action( "cleave" );
  slayer_aoe->add_action( "whirlwind,if=talent.fervor_of_battle&buff.collateral_damage.stack=3" );
  slayer_aoe->add_action( "execute,if=buff.sudden_death.up" );
  slayer_aoe->add_action( "bladestorm" );
  slayer_aoe->add_action( "overpower,if=buff.opportunist.up&talent.dreadnaught|talent.dreadnaught&charges=2" );
  slayer_aoe->add_action( "mortal_strike,if=buff.executioners_precision.stack=2" );
  slayer_aoe->add_action( "overpower,if=talent.dreadnaught" );
  slayer_aoe->add_action( "whirlwind,if=talent.fervor_of_battle" );
  slayer_aoe->add_action( "champions_leap" );
  slayer_aoe->add_action( "champions_spear" );
  slayer_aoe->add_action( "mortal_strike" );
  slayer_aoe->add_action( "overpower" );
  slayer_aoe->add_action( "execute" );
  slayer_aoe->add_action( "rend" );
  slayer_aoe->add_action( "wrecking_throw" );
  slayer_aoe->add_action( "whirlwind" );
  slayer_aoe->add_action( "slam" );
  slayer_aoe->add_action( "storm_bolt,if=buff.bladestorm.up" );

  slayer_execute->add_action( "sweeping_strikes,if=active_enemies=2" );
  slayer_execute->add_action( "rend,if=dot.rend.remains<=gcd&!talent.bloodletting" );
  slayer_execute->add_action( "avatar" );
  slayer_execute->add_action( "champions_leap" );
  slayer_execute->add_action( "champions_spear" );
  slayer_execute->add_action( "colossus_smash" );
  slayer_execute->add_action( "bladestorm,if=(buff.executioners_precision.stack=2&(debuff.colossus_smash.remains>4|cooldown.colossus_smash.remains>15))|!talent.executioners_precision" );
  slayer_execute->add_action( "mortal_strike,if=buff.executioners_precision.stack=2" );
  slayer_execute->add_action( "execute,if=rage>=40|buff.sudden_death.up" );
  slayer_execute->add_action( "overpower" );
  slayer_execute->add_action( "execute" );
  slayer_execute->add_action( "wrecking_throw" );
  slayer_execute->add_action( "storm_bolt,if=buff.bladestorm.up" );

  slayer_st->add_action( "sweeping_strikes,if=active_enemies=2" );
  slayer_st->add_action( "avatar" );
  slayer_st->add_action( "colossus_smash" );
  slayer_st->add_action( "mortal_strike,if=buff.executioners_precision.stack=2" );
  slayer_st->add_action( "execute,if=buff.sudden_death.up" );
  slayer_st->add_action( "overpower,if=charges=2" );
  slayer_st->add_action( "mortal_strike" );
  slayer_st->add_action( "bladestorm" );
  slayer_st->add_action( "champions_leap" );
  slayer_st->add_action( "champions_spear" );
  slayer_st->add_action( "overpower" );
  slayer_st->add_action( "wrecking_throw" );
  slayer_st->add_action( "rend,if=dot.rend.remains<=5" );
  slayer_st->add_action( "slam" );
  slayer_st->add_action( "storm_bolt,if=buff.bladestorm.up" );

  trinkets->add_action( "use_item,name=cursed_stone_idol,if=cooldown.avatar.remains<2" );
  trinkets->add_action( "use_item,slot=trinket1,if=variable.trinket_1_buffs&(variable.trinket_priority=1|!variable.trinket_2_buffs|!trinket.2.has_cooldown)&(buff.avatar.up)", "Trinkets" );
  trinkets->add_action( "use_item,slot=trinket2,if=variable.trinket_2_buffs&(variable.trinket_priority=2|!variable.trinket_1_buffs|!trinket.1.has_cooldown)&(buff.avatar.up)" );
  trinkets->add_action( "use_item,slot=trinket1,if=!variable.trinket_1_buffs&(variable.damage_trinket_priority=1|!variable.trinket_2_buffs|!trinket.2.has_cooldown)" );
  trinkets->add_action( "use_item,slot=trinket2,if=!variable.trinket_2_buffs&(variable.damage_trinket_priority=2|!variable.trinket_1_buffs|!trinket.1.has_cooldown)" );

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
  default_->add_action( "use_item,name=tome_of_lights_devotion,if=buff.inner_resilience.up" );
  default_->add_action( "use_items" );
  default_->add_action( "avatar,if=buff.thunder_blast.down|buff.thunder_blast.stack<=2" );
  default_->add_action( "shield_wall" );
  default_->add_action( "blood_fury" );
  default_->add_action( "berserking" );
  default_->add_action( "arcane_torrent" );
  default_->add_action( "lights_judgment" );
  default_->add_action( "fireblood" );
  default_->add_action( "ancestral_call" );
  default_->add_action( "bag_of_tricks" );
  default_->add_action( "potion,if=buff.avatar.up|buff.avatar.up&target.health.pct<=20" );
  default_->add_action( "ignore_pain,if=target.health.pct>=20&(rage.deficit<=15&cooldown.shield_slam.ready|rage.deficit<=40&cooldown.shield_charge.ready|rage.deficit<=20&cooldown.shield_charge.ready|rage.deficit<=30&cooldown.demoralizing_shout.ready&talent.booming_voice.enabled|rage.deficit<=20&cooldown.avatar.ready|rage.deficit<=45&cooldown.demoralizing_shout.ready&talent.booming_voice.enabled&buff.last_stand.up&rage.deficit<=30&cooldown.avatar.ready&buff.last_stand.up&rage.deficit<=20|rage.deficit<=40&cooldown.shield_slam.ready&buff.violent_outburst.up&talent.heavy_repercussions.enabled&talent.impenetrable_wall.enabled|rage.deficit<=55&cooldown.shield_slam.ready&buff.violent_outburst.up&buff.last_stand.up&talent.heavy_repercussions.enabled&talent.impenetrable_wall.enabled|rage.deficit<=17&cooldown.shield_slam.ready&talent.heavy_repercussions.enabled|rage.deficit<=18&cooldown.shield_slam.ready&talent.impenetrable_wall.enabled)|(rage>=70|buff.seeing_red.stack=7&rage>=35)&cooldown.shield_slam.remains<=1&buff.shield_block.remains>=4,use_off_gcd=1" );
  default_->add_action( "ravager" );
  default_->add_action( "demoralizing_shout,if=talent.booming_voice.enabled" );
  default_->add_action( "champions_leap" );
  default_->add_action( "champions_spear" );
  default_->add_action( "thunder_blast,if=spell_targets.thunder_blast>=2&buff.thunder_blast.stack=2" );
  default_->add_action( "demolish,if=buff.colossal_might.stack>=3" );
  default_->add_action( "shield_charge" );
  default_->add_action( "shield_block,if=buff.shield_block.remains<=10" );
  default_->add_action( "run_action_list,name=aoe,if=spell_targets.thunder_clap>=3" );
  default_->add_action( "call_action_list,name=generic" );

  aoe->add_action( "thunder_blast,if=dot.rend.remains<=1" );
  aoe->add_action( "thunder_clap,if=dot.rend.remains<=1" );
  aoe->add_action( "thunder_blast,if=buff.violent_outburst.up&spell_targets.thunderclap>=2&buff.avatar.up" );
  aoe->add_action( "execute,if=spell_targets.execute>=2&(rage>=50|buff.sudden_death.up)&talent.heavy_handed.enabled" );
  aoe->add_action( "thunder_clap,if=buff.violent_outburst.up&spell_targets.thunderclap>=4&buff.avatar.up&talent.crashing_thunder.enabled|buff.violent_outburst.up&spell_targets.thunderclap>6&buff.avatar.up" );
  aoe->add_action( "revenge,if=rage>=70&spell_targets.revenge>=3" );
  aoe->add_action( "shield_slam,if=rage<=60|buff.violent_outburst.up&spell_targets.thunderclap<=4&talent.crashing_thunder.enabled" );
  aoe->add_action( "thunder_blast" );
  aoe->add_action( "thunder_clap" );
  aoe->add_action( "revenge,if=rage>=30|rage>=40&talent.barbaric_training.enabled" );

  generic->add_action( "thunder_blast,if=(buff.thunder_blast.stack=2&buff.burst_of_power.stack<=1&buff.avatar.up)" );
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
