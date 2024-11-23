#include "class_modules/apl/warlock.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace warlock_apl{
  std::string potion( const player_t* p )
  {
    if ( p->true_level >= 80 ) return "tempered_potion_3";
    return ( p->true_level >= 70 ) ? "elemental_potion_of_ultimate_power_3" : "disabled";
  }

  std::string flask( const player_t* p )
  {
    if ( p->true_level >= 80 ) return "flask_of_alchemical_chaos_3";
    return ( p->true_level >= 70 ) ? "iced_phial_of_corrupting_rage_3" : "disabled";
  }

  std::string food( const player_t* p )
  {
    if ( p->true_level >= 80 ) return "feast_of_the_divine_day";
    return ( p->true_level >= 70 ) ? "fated_fortune_cookie" : "disabled";
  }

  std::string rune( const player_t* p )
  {
    if ( p->true_level >= 80 ) return "crystallized";
    return ( p->true_level >= 70 ) ? "draconic_augment_rune" : "disabled";
  }

  std::string temporary_enchant( const player_t* p )
  {
    if ( p->true_level >= 80 ) return "main_hand:algari_mana_oil_3";
    return ( p->true_level >= 70 ) ? "main_hand:howling_rune_3" : "disabled";
  }

//affliction_apl_start
void affliction( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* cleave = p->get_action_priority_list( "cleave" );
  action_priority_list_t* end_of_fight = p->get_action_priority_list( "end_of_fight" );
  action_priority_list_t* se_maintenance = p->get_action_priority_list( "se_maintenance" );
  action_priority_list_t* opener_cleave_se = p->get_action_priority_list( "opener_cleave_se" );
  action_priority_list_t* cleave_se_maintenance = p->get_action_priority_list( "cleave_se_maintenance" );
  action_priority_list_t* items = p->get_action_priority_list( "items" );
  action_priority_list_t* ogcd = p->get_action_priority_list( "ogcd" );
  action_priority_list_t* variables = p->get_action_priority_list( "variables" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "summon_pet" );
  precombat->add_action( "variable,name=cleave_apl,default=0,op=reset" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_use_buff", "Used to set Trinket in slot 1 as Buff Trinkets for the automatic logic" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_use_buff", "Used to set Trinkets in slot 2 as Buff Trinkets for the automatic logic" );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_1_buffs&(trinket.1.cooldown.duration%%cooldown.soul_rot.duration=0|cooldown.soul_rot.duration%%trinket.1.cooldown.duration=0)", "Automatic Logic for Buff Trinkets in Trinket Slot 1" );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_2_buffs&(trinket.2.cooldown.duration%%cooldown.soul_rot.duration=0|cooldown.soul_rot.duration%%trinket.2.cooldown.duration=0)", "Automatic Logic for Buff Trinkets in Trinket Slot 2" );
  precombat->add_action( "variable,name=trinket_1_manual,value=trinket.1.is.spymasters_web|trinket.1.is.aberrant_spellforge", " Sets a specific Trinkets in Slot 1 to follow an APL line and not the automatic logic" );
  precombat->add_action( "variable,name=trinket_2_manual,value=trinket.2.is.spymasters_web|trinket.2.is.aberrant_spellforge", " Sets a specific Trinkets in Slot 2 to follow an APL line and not the automatic logic " );
  precombat->add_action( "variable,name=trinket_1_exclude,value=trinket.1.is.ruby_whelp_shell", "For On Use Trinkets on slot 1 with on use effects you dont want to use in combat" );
  precombat->add_action( "variable,name=trinket_2_exclude,value=trinket.2.is.ruby_whelp_shell", "For On Use Trinkets on Slot 2 with on use effects you don't want to use in combat" );
  precombat->add_action( "variable,name=trinket_1_buff_duration,value=trinket.1.proc.any_dps.duration+(trinket.1.is.mirror_of_fractured_tomorrows*20)", " Sets the duration of Trinket 1 in the automatic logic" );
  precombat->add_action( "variable,name=trinket_2_buff_duration,value=trinket.2.proc.any_dps.duration+(trinket.2.is.mirror_of_fractured_tomorrows*20)", " Sets the duration of Trinket 2 in the automatic logic" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs|variable.trinket_2_buffs&((trinket.2.cooldown.duration%variable.trinket_2_buff_duration)*(1+0.5*trinket.2.has_buff.intellect)*(variable.trinket_2_sync))>((trinket.1.cooldown.duration%variable.trinket_1_buff_duration)*(1+0.5*trinket.1.has_buff.intellect)*(variable.trinket_1_sync))", "Automatic Logic in case of 2 On Use Buff Trinkets" );

  precombat->add_action( "grimoire_of_sacrifice,if=talent.grimoire_of_sacrifice.enabled" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "seed_of_corruption,if=spell_targets.seed_of_corruption_aoe>2|spell_targets.seed_of_corruption_aoe>1&talent.demonic_soul" );
  precombat->add_action( "haunt" );

  default_->add_action( "call_action_list,name=variables" );
  default_->add_action( "call_action_list,name=cleave,if=active_enemies!=1&active_enemies<3|variable.cleave_apl" );
  default_->add_action( "call_action_list,name=aoe,if=active_enemies>2" );
  default_->add_action( "call_action_list,name=ogcd" );
  default_->add_action( "call_action_list,name=items" );
  default_->add_action( "call_action_list,name=end_of_fight" );
  default_->add_action( "agony,if=(!talent.vile_taint|remains<cooldown.vile_taint.remains+action.vile_taint.cast_time)&(talent.absolute_corruption&remains<3|!talent.absolute_corruption&remains<5|cooldown.soul_rot.remains<5&remains<8)&fight_remains>dot.agony.remains+5" );
  default_->add_action( "haunt,if=talent.demonic_soul&buff.nightfall.react<2-prev_gcd.1.drain_soul&(!talent.vile_taint|cooldown.vile_taint.remains)" );
  default_->add_action( "unstable_affliction,if=(talent.absolute_corruption&remains<3|!talent.absolute_corruption&remains<5|cooldown.soul_rot.remains<5&remains<8)&(!talent.demonic_soul|buff.nightfall.react<2|prev_gcd.1.haunt&buff.nightfall.stack<2)&fight_remains>dot.unstable_affliction.remains+5" );
  default_->add_action( "haunt,if=(talent.absolute_corruption&debuff.haunt.remains<3|!talent.absolute_corruption&debuff.haunt.remains<5|cooldown.soul_rot.remains<5&debuff.haunt.remains<8)&(!talent.vile_taint|cooldown.vile_taint.remains)&fight_remains>debuff.haunt.remains+5" );
  default_->add_action( "wither,if=talent.wither&(talent.absolute_corruption&remains<3|!talent.absolute_corruption&remains<5)&fight_remains>dot.wither.remains+5" );
  default_->add_action( "corruption,if=refreshable&fight_remains>dot.corruption.remains+5" );
  default_->add_action( "drain_soul,if=buff.nightfall.react&(buff.nightfall.react>1|buff.nightfall.remains<execute_time*2)&!buff.tormented_crescendo.react&cooldown.soul_rot.remains&soul_shard<5-buff.tormented_crescendo.react&(!talent.vile_taint|cooldown.vile_taint.remains)" );
  default_->add_action( "shadow_bolt,if=buff.nightfall.react&(buff.nightfall.react>1|buff.nightfall.remains<execute_time*2)&buff.tormented_crescendo.react<2&cooldown.soul_rot.remains&soul_shard<5-buff.tormented_crescendo.react&(!talent.vile_taint|cooldown.vile_taint.remains)" );
  default_->add_action( "call_action_list,name=se_maintenance,if=talent.wither" );
  default_->add_action( "vile_taint,if=(!talent.soul_rot|cooldown.soul_rot.remains>20|cooldown.soul_rot.remains<=execute_time+gcd.max|fight_remains<cooldown.soul_rot.remains)&dot.agony.remains&(dot.corruption.remains|dot.wither.remains)&dot.unstable_affliction.remains" );
  default_->add_action( "phantom_singularity,if=(!talent.soul_rot|cooldown.soul_rot.remains<4|fight_remains<cooldown.soul_rot.remains)&dot.agony.remains&(dot.corruption.remains|dot.wither.remains)&dot.unstable_affliction.remains" );
  default_->add_action( "malevolence,if=variable.vt_ps_up" );
  default_->add_action( "soul_rot,if=variable.vt_ps_up" );
  default_->add_action( "summon_darkglare,if=variable.cd_dots_up&(debuff.shadow_embrace.stack=debuff.shadow_embrace.max_stack)" );
  default_->add_action( "call_action_list,name=se_maintenance,if=talent.demonic_soul" );
  default_->add_action( "malefic_rapture,if=soul_shard>4&(talent.demonic_soul&buff.nightfall.react<2|!talent.demonic_soul)|buff.tormented_crescendo.react>1" );
  default_->add_action( "drain_soul,if=talent.demonic_soul&buff.nightfall.react&buff.tormented_crescendo.react<2&target.health.pct<20" );
  default_->add_action( "malefic_rapture,if=talent.demonic_soul&(soul_shard>1|buff.tormented_crescendo.react&cooldown.soul_rot.remains>buff.tormented_crescendo.remains*gcd.max)&(!talent.vile_taint|soul_shard>1&cooldown.vile_taint.remains>10)&(!talent.oblivion|cooldown.oblivion.remains>10|soul_shard>2&cooldown.oblivion.remains<10)" );
  default_->add_action( "oblivion,if=dot.agony.remains&(dot.corruption.remains|dot.wither.remains)&dot.unstable_affliction.remains&debuff.haunt.remains>5" );
  default_->add_action( "malefic_rapture,if=talent.tormented_crescendo&buff.tormented_crescendo.react&(buff.tormented_crescendo.remains<gcd.max*2|buff.tormented_crescendo.react=2)" );
  default_->add_action( "malefic_rapture,if=(variable.cd_dots_up|(talent.demonic_soul|talent.phantom_singularity)&variable.vt_ps_up|talent.wither&variable.vt_ps_up&!dot.soul_rot.remains&soul_shard>2)&(!talent.oblivion|cooldown.oblivion.remains>10|soul_shard>2&cooldown.oblivion.remains<10)" );
  default_->add_action( "malefic_rapture,if=talent.tormented_crescendo&talent.nightfall&buff.tormented_crescendo.react&buff.nightfall.react|talent.demonic_soul&!buff.nightfall.react&(!talent.vile_taint|cooldown.vile_taint.remains>10|soul_shard>1&cooldown.vile_taint.remains<10)" );
  default_->add_action( "malefic_rapture,if=!talent.demonic_soul&buff.tormented_crescendo.react" );
  default_->add_action( "drain_soul,if=buff.nightfall.react" );
  default_->add_action( "shadow_bolt,if=buff.nightfall.react" );
  default_->add_action( "agony,if=refreshable" );
  default_->add_action( "unstable_affliction,if=refreshable" );
  default_->add_action( "drain_soul,chain=1,early_chain_if=buff.nightfall.react,interrupt_if=tick_time>0.5" );
  default_->add_action( "shadow_bolt" );

  aoe->add_action( "call_action_list,name=ogcd" );
  aoe->add_action( "call_action_list,name=items" );
  aoe->add_action( "cycling_variable,name=min_agony,op=min,value=dot.agony.remains+(99*!dot.agony.remains)" );
  aoe->add_action( "cycling_variable,name=min_vt,op=min,default=10,value=dot.vile_taint.remains+(99*!dot.vile_taint.remains)" );
  aoe->add_action( "cycling_variable,name=min_ps,op=min,default=16,value=dot.phantom_singularity.remains+(99*!dot.phantom_singularity.remains)" );
  aoe->add_action( "variable,name=min_ps1,op=set,value=(variable.min_vt*talent.vile_taint<?variable.min_ps*talent.phantom_singularity)" );
  aoe->add_action( "haunt,if=debuff.haunt.remains<3" );
  aoe->add_action( "vile_taint,if=(cooldown.soul_rot.remains<=execute_time|cooldown.soul_rot.remains>=25)" );
  aoe->add_action( "phantom_singularity,if=(cooldown.soul_rot.remains<=execute_time|cooldown.soul_rot.remains>=25)&dot.agony.remains" );
  aoe->add_action( "unstable_affliction,if=remains<5" );
  aoe->add_action( "agony,target_if=min:remains,if=active_dot.agony<8&(remains<cooldown.vile_taint.remains+action.vile_taint.cast_time|!talent.vile_taint)&gcd.max+action.soul_rot.cast_time+gcd.max<(variable.min_vt*talent.vile_taint<?variable.min_ps*talent.phantom_singularity)&remains<10" );
  aoe->add_action( "soul_rot,if=variable.vt_up&(variable.ps_up|variable.vt_up)&dot.agony.remains" );
  aoe->add_action( "malevolence,if=variable.ps_up&variable.vt_up&variable.sr_up|cooldown.invoke_power_infusion_0.duration>0&cooldown.invoke_power_infusion_0.up&!talent.soul_rot" );
  aoe->add_action( "seed_of_corruption,if=((!talent.wither&dot.corruption.remains<5)|(talent.wither&dot.wither.remains<5))&!(action.seed_of_corruption.in_flight|dot.seed_of_corruption.remains>0)" );
  aoe->add_action( "corruption,target_if=min:remains,if=remains<5&!talent.seed_of_corruption" );
  aoe->add_action( "wither,target_if=min:remains,if=remains<5&!talent.seed_of_corruption" );
  aoe->add_action( "summon_darkglare,if=variable.ps_up&variable.vt_up&variable.sr_up|cooldown.invoke_power_infusion_0.duration>0&cooldown.invoke_power_infusion_0.up&!talent.soul_rot" );
  aoe->add_action( "malefic_rapture,if=(cooldown.summon_darkglare.remains>15|soul_shard>3|(talent.demonic_soul&soul_shard>2))&buff.tormented_crescendo.up" );
  aoe->add_action( "malefic_rapture,if=soul_shard>4|(talent.tormented_crescendo&buff.tormented_crescendo.react=1&soul_shard>3)" );
  aoe->add_action( "malefic_rapture,if=talent.demonic_soul&(soul_shard>2|(talent.tormented_crescendo&buff.tormented_crescendo.react=1&soul_shard))" );
  aoe->add_action( "malefic_rapture,if=talent.tormented_crescendo&buff.tormented_crescendo.react" );
  aoe->add_action( "malefic_rapture,if=talent.tormented_crescendo&buff.tormented_crescendo.react=2" );
  aoe->add_action( "malefic_rapture,if=(variable.cd_dots_up|variable.vt_ps_up)&(soul_shard>2|cooldown.oblivion.remains>10|!talent.oblivion)" );
  aoe->add_action( "malefic_rapture,if=talent.tormented_crescendo&talent.nightfall&buff.tormented_crescendo.react&buff.nightfall.react" );
  aoe->add_action( "drain_soul,interrupt_if=cooldown.vile_taint.ready,if=talent.drain_soul&buff.nightfall.react&talent.shadow_embrace&(debuff.shadow_embrace.stack<4|debuff.shadow_embrace.remains<3)" );
  aoe->add_action( "drain_soul,interrupt_if=cooldown.vile_taint.ready,interrupt_global=1,if=talent.drain_soul&(talent.shadow_embrace&(debuff.shadow_embrace.stack<4|debuff.shadow_embrace.remains<3))|!talent.shadow_embrace" );
  aoe->add_action( "shadow_bolt,if=buff.nightfall.react&talent.shadow_embrace&(debuff.shadow_embrace.stack<2|debuff.shadow_embrace.remains<3)" );

  cleave->add_action( "call_action_list,name=ogcd" );
  cleave->add_action( "call_action_list,name=items" );
  cleave->add_action( "call_action_list,name=end_of_fight" );
  cleave->add_action( "agony,target_if=min:remains,if=(remains<cooldown.vile_taint.remains+action.vile_taint.cast_time|!talent.vile_taint)&(remains<gcd.max*2|talent.demonic_soul&remains<cooldown.soul_rot.remains+8&cooldown.soul_rot.remains<5)&fight_remains>remains+5" );
  cleave->add_action( "wither,target_if=min:remains,if=remains<5&!(action.seed_of_corruption.in_flight|dot.seed_of_corruption.remains>0)&fight_remains>remains+5" );
  cleave->add_action( "haunt,if=talent.demonic_soul&buff.nightfall.react<2-prev_gcd.1.drain_soul&(!talent.vile_taint|cooldown.vile_taint.remains)|debuff.haunt.remains<3" );
  cleave->add_action( "unstable_affliction,if=(remains<5|talent.demonic_soul&remains<cooldown.soul_rot.remains+8&cooldown.soul_rot.remains<5)&fight_remains>remains+5" );
  cleave->add_action( "corruption,target_if=min:remains,if=remains<5&!(action.seed_of_corruption.in_flight|dot.seed_of_corruption.remains>0)&fight_remains>remains+5" );
  cleave->add_action( "call_action_list,name=cleave_se_maintenance,if=talent.wither" );
  cleave->add_action( "vile_taint,if=!talent.soul_rot|(variable.min_agony<1.5|cooldown.soul_rot.remains<=execute_time+gcd.max)|cooldown.soul_rot.remains>=20" );
  cleave->add_action( "phantom_singularity,if=(!talent.soul_rot|cooldown.soul_rot.remains<4|fight_remains<cooldown.soul_rot.remains)&active_dot.agony=2" );
  cleave->add_action( "malevolence,if=variable.vt_ps_up" );
  cleave->add_action( "soul_rot,if=(variable.vt_ps_up)&active_dot.agony=2" );
  cleave->add_action( "summon_darkglare,if=variable.cd_dots_up" );
  cleave->add_action( "call_action_list,name=opener_cleave_se,if=talent.demonic_soul" );
  cleave->add_action( "call_action_list,name=cleave_se_maintenance,if=talent.demonic_soul" );
  cleave->add_action( "malefic_rapture,if=soul_shard>4&(talent.demonic_soul&buff.nightfall.react<2|!talent.demonic_soul)|buff.tormented_crescendo.react>1" );
  cleave->add_action( "drain_soul,if=talent.demonic_soul&buff.nightfall.react&buff.tormented_crescendo.react<2&target.health.pct<20" );
  cleave->add_action( "malefic_rapture,if=talent.demonic_soul&(soul_shard>1|buff.tormented_crescendo.react&cooldown.soul_rot.remains>buff.tormented_crescendo.remains*gcd.max)&(!talent.vile_taint|soul_shard>1&cooldown.vile_taint.remains>10)&(!talent.oblivion|cooldown.oblivion.remains>10|soul_shard>2&cooldown.oblivion.remains<10)" );
  cleave->add_action( "malefic_rapture,if=talent.tormented_crescendo&buff.tormented_crescendo.react&(buff.tormented_crescendo.remains<gcd.max*2|buff.tormented_crescendo.react=2)" );
  cleave->add_action( "malefic_rapture,if=(variable.cd_dots_up|(talent.demonic_soul|talent.phantom_singularity)&variable.vt_ps_up|talent.wither&variable.vt_ps_up&!dot.soul_rot.remains&soul_shard>1)&(!talent.oblivion|cooldown.oblivion.remains>10|soul_shard>2&cooldown.oblivion.remains<10)" );
  cleave->add_action( "malefic_rapture,if=talent.tormented_crescendo&talent.nightfall&buff.tormented_crescendo.react&buff.nightfall.react|talent.demonic_soul&!buff.nightfall.react&(!talent.vile_taint|cooldown.vile_taint.remains>10|soul_shard>1&cooldown.vile_taint.remains<10)" );
  cleave->add_action( "malefic_rapture,if=!talent.demonic_soul&buff.tormented_crescendo.react" );
  cleave->add_action( "agony,if=refreshable|cooldown.soul_rot.remains<5&remains<8" );
  cleave->add_action( "unstable_affliction,if=refreshable|cooldown.soul_rot.remains<5&remains<8" );
  cleave->add_action( "drain_soul,if=buff.nightfall.react" );
  cleave->add_action( "shadow_bolt,if=buff.nightfall.react" );
  cleave->add_action( "wither,if=refreshable" );
  cleave->add_action( "corruption,if=refreshable" );
  cleave->add_action( "drain_soul,chain=1,early_chain_if=buff.nightfall.react,interrupt_if=tick_time>0.5" );
  cleave->add_action( "shadow_bolt" );

  end_of_fight->add_action( "drain_soul,if=talent.demonic_soul&(fight_remains<5&buff.nightfall.react|prev_gcd.1.haunt&buff.nightfall.react=2&!buff.tormented_crescendo.react)" );
  end_of_fight->add_action( "oblivion,if=soul_shard>1&fight_remains<(soul_shard+buff.tormented_crescendo.react)*gcd.max+execute_time" );
  end_of_fight->add_action( "malefic_rapture,if=fight_remains<4&(!talent.demonic_soul|talent.demonic_soul&buff.nightfall.react<1)" );

  se_maintenance->add_action( "drain_soul,interrupt=1,if=talent.shadow_embrace&talent.drain_soul&(debuff.shadow_embrace.stack<debuff.shadow_embrace.max_stack|debuff.shadow_embrace.remains<3)&active_enemies<=4&fight_remains>15,interrupt_if=debuff.shadow_embrace.stack=debuff.shadow_embrace.max_stack" );
  se_maintenance->add_action( "shadow_bolt,if=talent.shadow_embrace&((debuff.shadow_embrace.stack+action.shadow_bolt.in_flight_to_target_count)<debuff.shadow_embrace.max_stack|debuff.shadow_embrace.remains<3&!action.shadow_bolt.in_flight_to_target)&active_enemies<=4&fight_remains>15" );

  opener_cleave_se->add_action( "drain_soul,if=talent.shadow_embrace&talent.drain_soul&buff.nightfall.react&(debuff.shadow_embrace.stack<debuff.shadow_embrace.max_stack|debuff.shadow_embrace.remains<3)&(fight_remains>15|time<20),interrupt_if=debuff.shadow_embrace.stack=debuff.shadow_embrace.max_stack" );

  cleave_se_maintenance->add_action( "drain_soul,target_if=min:debuff.shadow_embrace.remains,if=talent.shadow_embrace&talent.drain_soul&(talent.wither|talent.demonic_soul&buff.nightfall.react)&(debuff.shadow_embrace.stack<debuff.shadow_embrace.max_stack|debuff.shadow_embrace.remains<3)&fight_remains>15,interrupt_if=debuff.shadow_embrace.stack>3" );
  cleave_se_maintenance->add_action( "shadow_bolt,target_if=min:debuff.shadow_embrace.remains,if=talent.shadow_embrace&!talent.drain_soul&((debuff.shadow_embrace.stack+action.shadow_bolt.in_flight_to_target_count)<debuff.shadow_embrace.max_stack|debuff.shadow_embrace.remains<3&!action.shadow_bolt.in_flight_to_target)&fight_remains>15" );

  items->add_action( "use_item,name=aberrant_spellforge,use_off_gcd=1,if=gcd.remains>gcd.max*0.8" );
  items->add_action( "use_item,name=spymasters_web,if=variable.cd_dots_up&(buff.spymasters_report.stack>=38|fight_remains<=80|talent.drain_soul&target.health.pct<20)|fight_remains<20" );
  items->add_action( "use_item,slot=trinket1,if=(variable.cds_active)&(variable.trinket_priority=1|variable.trinket_2_exclude|!trinket.2.has_cooldown|(trinket.2.cooldown.remains|variable.trinket_priority=2&cooldown.summon_darkglare.remains>20&!pet.darkglare.active&trinket.2.cooldown.remains<cooldown.summon_darkglare.remains))&variable.trinket_1_buffs&!variable.trinket_1_manual|(variable.trinket_1_buff_duration+1>=fight_remains)" );
  items->add_action( "use_item,slot=trinket2,if=(variable.cds_active)&(variable.trinket_priority=2|variable.trinket_1_exclude|!trinket.1.has_cooldown|(trinket.1.cooldown.remains|variable.trinket_priority=1&cooldown.summon_darkglare.remains>20&!pet.darkglare.active&trinket.1.cooldown.remains<cooldown.summon_darkglare.remains))&variable.trinket_2_buffs&!variable.trinket_2_manual|(variable.trinket_2_buff_duration+1>=fight_remains)" );
  items->add_action( "use_item,name=time_thiefs_gambit,if=variable.cds_active|fight_remains<15|((trinket.1.cooldown.duration<cooldown.summon_darkglare.remains_expected+5)&active_enemies=1)|(active_enemies>1&havoc_active)" );
  items->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=!variable.trinket_1_buffs&!variable.trinket_1_manual&(!variable.trinket_1_buffs&(trinket.2.cooldown.remains|!variable.trinket_2_buffs)|talent.summon_darkglare&cooldown.summon_darkglare.remains_expected>20|!talent.summon_darkglare)" );
  items->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=!variable.trinket_2_buffs&!variable.trinket_2_manual&(!variable.trinket_2_buffs&(trinket.1.cooldown.remains|!variable.trinket_1_buffs)|talent.summon_darkglare&cooldown.summon_darkglare.remains_expected>20|!talent.summon_darkglare)" );
  items->add_action( "use_item,use_off_gcd=1,slot=main_hand" );

  ogcd->add_action( "potion,if=variable.cds_active|fight_remains<32|prev_gcd.1.soul_rot&time<20" );
  ogcd->add_action( "berserking,if=variable.cds_active|fight_remains<14|prev_gcd.1.soul_rot&time<20" );
  ogcd->add_action( "blood_fury,if=variable.cds_active|fight_remains<17|prev_gcd.1.soul_rot&time<20" );
  ogcd->add_action( "invoke_external_buff,name=power_infusion,if=variable.cds_active" );
  ogcd->add_action( "fireblood,if=variable.cds_active|fight_remains<10|prev_gcd.1.soul_rot&time<20" );
  ogcd->add_action( "ancestral_call,if=variable.cds_active|fight_remains<17|prev_gcd.1.soul_rot&time<20" );

  variables->add_action( "variable,name=ps_up,op=set,value=!talent.phantom_singularity|dot.phantom_singularity.remains" );
  variables->add_action( "variable,name=vt_up,op=set,value=!talent.vile_taint|dot.vile_taint_dot.remains" );
  variables->add_action( "variable,name=vt_ps_up,op=set,value=(!talent.vile_taint&!talent.phantom_singularity)|dot.vile_taint_dot.remains|dot.phantom_singularity.remains" );
  variables->add_action( "variable,name=sr_up,op=set,value=!talent.soul_rot|dot.soul_rot.remains" );
  variables->add_action( "variable,name=cd_dots_up,op=set,value=variable.ps_up&variable.vt_up&variable.sr_up" );
  variables->add_action( "variable,name=has_cds,op=set,value=talent.phantom_singularity|talent.vile_taint|talent.soul_rot|talent.summon_darkglare" );
  variables->add_action( "variable,name=cds_active,op=set,value=!variable.has_cds|(variable.cd_dots_up&(!talent.summon_darkglare|cooldown.summon_darkglare.remains>20|pet.darkglare.remains))" );
  variables->add_action( "variable,name=min_vt,op=reset,if=variable.min_vt" );
  variables->add_action( "variable,name=min_ps,op=reset,if=variable.min_ps" );
}
//affliction_apl_end

//demonology_apl_start
void demonology( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* fight_end = p->get_action_priority_list( "fight_end" );
  action_priority_list_t* items = p->get_action_priority_list( "items" );
  action_priority_list_t* opener = p->get_action_priority_list( "opener" );
  action_priority_list_t* racials = p->get_action_priority_list( "racials" );
  action_priority_list_t* tyrant = p->get_action_priority_list( "tyrant" );
  action_priority_list_t* variables = p->get_action_priority_list( "variables" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "summon_pet" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=first_tyrant_time,op=set,value=12", "Sets the expected Tyrant Setup on pull to take a total 12 seconds long" );
  precombat->add_action( "variable,name=first_tyrant_time,op=add,value=action.grimoire_felguard.execute_time,if=talent.grimoire_felguard.enabled", "Accounts for the execution time of Grimoire Felguard in the setup of Tyrant on Pull" );
  precombat->add_action( "variable,name=first_tyrant_time,op=add,value=action.summon_vilefiend.execute_time,if=talent.summon_vilefiend.enabled", "Accounts for the execution time of Vilefiend in the the setup of Tyrant on Pull" );
  precombat->add_action( "variable,name=first_tyrant_time,op=add,value=gcd.max,if=talent.grimoire_felguard.enabled|talent.summon_vilefiend.enabled", "Accounts for the execution time of both Grimoire Felguard and Vilefiend in the tyrant Setup on Pull" );
  precombat->add_action( "variable,name=first_tyrant_time,op=sub,value=action.summon_demonic_tyrant.execute_time+action.shadow_bolt.execute_time", "Accounts for Tyrant own Cast Time and an additional Shadowbolt cast time" );
  precombat->add_action( "variable,name=first_tyrant_time,op=min,value=10", "Sets an absolute minimun of 10s for the First Tyrant Setup" );
  precombat->add_action( "variable,name=in_opener,op=set,value=1" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_use_buff", "Defines if the the Trinket 1 is a buff Trinket in the trinket logic" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_use_buff", "Defines if the the Trinket 2 is a buff Trinket in the trinket logic" );
  precombat->add_action( "variable,name=trinket_1_exclude,value=trinket.1.is.ruby_whelp_shell", "For On Use Trinkets on slot 1 with on use effects you dont want to use in combat" );
  precombat->add_action( "variable,name=trinket_2_exclude,value=trinket.2.is.ruby_whelp_shell", "For On Use Trinkets on slot 2 with on use effects you dont want to use in combat" );
  precombat->add_action( "variable,name=trinket_1_manual,value=trinket.1.is.spymasters_web|trinket.1.is.imperfect_ascendancy_serum", "Sets a specific Trinkets in Slot 1 to follow an APL line and not the automatic logic" );
  precombat->add_action( "variable,name=trinket_2_manual,value=trinket.2.is.spymasters_web|trinket.2.is.imperfect_ascendancy_serum", "Sets a specific Trinkets in Slot 2 to follow an APL line and not the automatic logic" );
  precombat->add_action( "variable,name=trinket_1_buff_duration,value=trinket.1.proc.any_dps.duration+(trinket.1.is.mirror_of_fractured_tomorrows*20)", "Defines the Duration of the buff or an expected time for value of the trinket" );
  precombat->add_action( "variable,name=trinket_2_buff_duration,value=trinket.2.proc.any_dps.duration+(trinket.2.is.mirror_of_fractured_tomorrows*20)", "Defines the Duration of the buff or an expected time for value of the trinket" );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_1_buffs&(trinket.1.cooldown.duration%%cooldown.summon_demonic_tyrant.duration=0|cooldown.summon_demonic_tyrant.duration%%trinket.1.cooldown.duration=0)", "Trinket Automatic Logic for Trinket 1" );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_2_buffs&(trinket.2.cooldown.duration%%cooldown.summon_demonic_tyrant.duration=0|cooldown.summon_demonic_tyrant.duration%%trinket.2.cooldown.duration=0)", "Trinket Automatic Logic for Trinket 2" );
  precombat->add_action( "variable,name=damage_trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&!variable.trinket_2_buffs&trinket.2.ilvl>trinket.1.ilvl", "Automatic Logic in case of 2 Buff Trinkets" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs|variable.trinket_2_buffs&((trinket.2.cooldown.duration%variable.trinket_2_buff_duration)*(1.5+trinket.2.has_buff.intellect)*(variable.trinket_2_sync))>(((trinket.1.cooldown.duration%variable.trinket_1_buff_duration)*(1.5+trinket.1.has_buff.intellect)*(variable.trinket_1_sync))*(1+((trinket.1.ilvl-trinket.2.ilvl)%100)))" );
  precombat->add_action( "power_siphon" );
  precombat->add_action( "demonbolt,if=!buff.power_siphon.up" );
  precombat->add_action( "shadow_bolt" );

  default_->add_action( "call_action_list,name=variables" );
  default_->add_action( "potion,if=buff.tyrant.remains>10" );
  default_->add_action( "call_action_list,name=racials,if=pet.demonic_tyrant.active|fight_remains<22,use_off_gcd=1" );
  default_->add_action( "call_action_list,name=items,use_off_gcd=1" );
  default_->add_action( "invoke_external_buff,name=power_infusion,if=fight_remains<20|pet.demonic_tyrant.active&fight_remains<100|fight_remains<25|(pet.demonic_tyrant.active|!talent.summon_demonic_tyrant&buff.dreadstalkers.up)" );
  default_->add_action( "call_action_list,name=fight_end,if=fight_remains<30" );
  default_->add_action( "call_action_list,name=opener,if=time<variable.first_tyrant_time" );
  default_->add_action( "call_action_list,name=tyrant,if=cooldown.summon_demonic_tyrant.remains<gcd.max*14" );
  default_->add_action( "call_dreadstalkers,if=cooldown.summon_demonic_tyrant.remains>25|variable.next_tyrant_cd>25" );
  default_->add_action( "summon_vilefiend,if=cooldown.summon_demonic_tyrant.remains>30" );
  default_->add_action( "demonbolt,target_if=(!debuff.doom.up|!action.demonbolt.in_flight&debuff.doom.remains<=2),if=buff.demonic_core.up&(((!talent.soul_strike|cooldown.soul_strike.remains>gcd.max*2&talent.fel_invocation)&soul_shard<4)|soul_shard<(4-(active_enemies>2)))&!prev_gcd.1.demonbolt&talent.doom&cooldown.summon_demonic_tyrant.remains>15" );
  default_->add_action( "demonbolt,if=buff.demonic_core.stack>=3&soul_shard<=3&!variable.pool_cores_for_tyrant" );
  default_->add_action( "power_siphon,if=buff.demonic_core.stack<3&cooldown.summon_demonic_tyrant.remains>25" );
  default_->add_action( "demonic_strength,,if=active_enemies>1" );
  default_->add_action( "bilescourge_bombers,if=active_enemies>1" );
  default_->add_action( "guillotine,if=active_enemies>1&(cooldown.demonic_strength.remains|!talent.demonic_strength)&(!raid_event.adds.exists|raid_event.adds.exists&raid_event.adds.remains>6)" );
  default_->add_action( "ruination" );
  default_->add_action( "infernal_bolt,if=soul_shard<3&cooldown.summon_demonic_tyrant.remains>20" );
  default_->add_action( "implosion,if=two_cast_imps>0&variable.impl&!prev_gcd.1.implosion&!raid_event.adds.exists|two_cast_imps>0&variable.impl&!prev_gcd.1.implosion&raid_event.adds.exists&(active_enemies>3|active_enemies<=3&last_cast_imps>0)" );
  default_->add_action( "Demonbolt,if=variable.diabolic_ritual_remains>gcd.max&variable.diabolic_ritual_remains<gcd.max+gcd.max&buff.demonic_core.up&soul_shard<=3" );
  default_->add_action( "shadow_bolt,if=variable.diabolic_ritual_remains>gcd.max&variable.diabolic_ritual_remains<soul_shard.deficit*cast_time+gcd.max&soul_shard<5" );
  default_->add_action( "hand_of_guldan,if=((soul_shard>2&(cooldown.call_dreadstalkers.remains>gcd.max*4|buff.demonic_calling.remains-gcd.max>cooldown.call_dreadstalkers.remains)&cooldown.summon_demonic_tyrant.remains>17)|soul_shard=5|soul_shard=4&talent.fel_invocation)&(active_enemies=1)" );
  default_->add_action( "hand_of_guldan,if=soul_shard>2&!(active_enemies=1)" );
  default_->add_action( "demonbolt,target_if=(!debuff.doom.up)|active_enemies<4,if=buff.demonic_core.stack>1&((soul_shard<4&!talent.soul_strike|cooldown.soul_strike.remains>gcd.max*2&talent.fel_invocation)|soul_shard<3)&!variable.pool_cores_for_tyrant" );
  default_->add_action( "demonbolt,if=buff.demonic_core.react&buff.tyrant.up&soul_shard<3-talent.quietus" );
  default_->add_action( "demonbolt,if=buff.demonic_core.react>1&soul_shard<4-talent.quietus" );
  default_->add_action( "demonbolt,target_if=(!debuff.doom.up)|active_enemies<4,if=talent.doom&(debuff.doom.remains>10&buff.demonic_core.up&soul_shard<4-talent.quietus)&!variable.pool_cores_for_tyrant" );
  default_->add_action( "demonbolt,if=fight_remains<buff.demonic_core.stack*gcd.max" );
  default_->add_action( "demonbolt,target_if=(!debuff.doom.up)|active_enemies<4,if=buff.demonic_core.up&(cooldown.power_siphon.remains<4)&(soul_shard<4-talent.quietus)&!variable.pool_cores_for_tyrant" );
  default_->add_action( "power_siphon,if=!buff.demonic_core.up&cooldown.summon_demonic_tyrant.remains>25" );
  default_->add_action( "summon_vilefiend,if=fight_remains<cooldown.summon_demonic_tyrant.remains+5" );
  default_->add_action( "shadow_bolt" );
  default_->add_action( "infernal_bolt" );

  fight_end->add_action( "grimoire_felguard,if=fight_remains<20" );
  fight_end->add_action( "Ruination" );
  fight_end->add_action( "implosion,if=fight_remains<2*gcd.max&!prev_gcd.1.implosion" );
  fight_end->add_action( "demonbolt,if=fight_remains<gcd.max*2*buff.demonic_core.stack+9&buff.demonic_core.react&(soul_shard<4|fight_remains<buff.demonic_core.stack*gcd.max)" );
  fight_end->add_action( "call_dreadstalkers,if=fight_remains<20" );
  fight_end->add_action( "summon_vilefiend,if=fight_remains<20" );
  fight_end->add_action( "summon_demonic_tyrant,if=fight_remains<20" );
  fight_end->add_action( "demonic_strength,if=fight_remains<10" );
  fight_end->add_action( "power_siphon,if=buff.demonic_core.stack<3&fight_remains<20" );
  fight_end->add_action( "demonbolt,if=fight_remains<gcd.max*2*buff.demonic_core.stack+9&buff.demonic_core.react&(soul_shard<4|fight_remains<buff.demonic_core.stack*gcd.max)" );
  fight_end->add_action( "hand_of_guldan,if=soul_shard>2&fight_remains<gcd.max*2*buff.demonic_core.stack+9" );
  fight_end->add_action( "infernal_bolt" );

  items->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=variable.trinket_1_buffs&!variable.trinket_1_manual&(!pet.demonic_tyrant.active&trinket.1.cast_time>0|!trinket.1.cast_time>0)&(pet.demonic_tyrant.active|!talent.summon_demonic_tyrant|variable.trinket_priority=2&cooldown.summon_demonic_tyrant.remains>20&!pet.demonic_tyrant.active&trinket.2.cooldown.remains<cooldown.summon_demonic_tyrant.remains+5)&(variable.trinket_2_exclude|!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1&!variable.trinket_2_manual)|variable.trinket_1_buff_duration>=fight_remains" );
  items->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=variable.trinket_2_buffs&!variable.trinket_2_manual&(!pet.demonic_tyrant.active&trinket.2.cast_time>0|!trinket.2.cast_time>0)&(pet.demonic_tyrant.active|!talent.summon_demonic_tyrant|variable.trinket_priority=1&cooldown.summon_demonic_tyrant.remains>20&!pet.demonic_tyrant.active&trinket.1.cooldown.remains<cooldown.summon_demonic_tyrant.remains+5)&(variable.trinket_1_exclude|!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2&!variable.trinket_1_manual)|variable.trinket_2_buff_duration>=fight_remains" );
  items->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=!variable.trinket_1_buffs&!variable.trinket_1_manual&((variable.damage_trinket_priority=1|trinket.2.cooldown.remains)&(trinket.1.cast_time>0&!pet.demonic_tyrant.active|!trinket.1.cast_time>0)|(time<20&variable.trinket_2_buffs)|cooldown.summon_demonic_tyrant.remains_expected>20)" );
  items->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=!variable.trinket_2_buffs&!variable.trinket_2_manual&((variable.damage_trinket_priority=2|trinket.1.cooldown.remains)&(trinket.2.cast_time>0&!pet.demonic_tyrant.active|!trinket.2.cast_time>0)|(time<20&variable.trinket_1_buffs)|cooldown.summon_demonic_tyrant.remains_expected>20)" );
  items->add_action( "use_item,use_off_gcd=1,name=spymasters_web,if=pet.demonic_tyrant.active&fight_remains<=80&buff.spymasters_report.stack>=30&(!variable.trinket_1_buffs&trinket.2.is.spymasters_web|!variable.trinket_2_buffs&trinket.1.is.spymasters_web)|fight_remains<=20&(trinket.1.cooldown.remains&trinket.2.is.spymasters_web|trinket.2.cooldown.remains&trinket.1.is.spymasters_web|!variable.trinket_1_buffs|!variable.trinket_2_buffs)" );
  items->add_action( "use_item,use_off_gcd=1,name=imperfect_ascendancy_serum,if=pet.demonic_tyrant.active&gcd.remains>0|fight_remains<=30" );
  items->add_action( "use_item,use_off_gcd=1,slot=main_hand" );
  items->add_action( "use_item,name=mirror_of_fractured_tomorrows,if=trinket.1.is.mirror_of_fractured_tomorrows&variable.trinket_priority=2|trinket.2.is.mirror_of_fractured_tomorrows&variable.trinket_priority=1" );
  items->add_action( "use_item,slot=trinket1,if=!variable.trinket_1_buffs&(variable.damage_trinket_priority=1|trinket.2.cooldown.remains)" );
  items->add_action( "use_item,slot=trinket2,if=!variable.trinket_2_buffs&(variable.damage_trinket_priority=2|trinket.1.cooldown.remains)" );

  opener->add_action( "grimoire_felguard,if=soul_shard>=5-talent.fel_invocation" );
  opener->add_action( "summon_vilefiend,if=soul_shard=5" );
  opener->add_action( "shadow_bolt,if=soul_shard<5&cooldown.call_dreadstalkers.up" );
  opener->add_action( "call_dreadstalkers,if=soul_shard=5" );
  opener->add_action( "Ruination" );

  racials->add_action( "berserking,use_off_gcd=1" );
  racials->add_action( "blood_fury" );
  racials->add_action( "fireblood" );
  racials->add_action( "ancestral_call" );

  tyrant->add_action( "call_action_list,name=racials,if=variable.imp_despawn&variable.imp_despawn<time+gcd.max*2+action.summon_demonic_tyrant.cast_time&(prev_gcd.1.hand_of_guldan|prev_gcd.1.ruination)&(variable.imp_despawn&variable.imp_despawn<time+gcd.max+action.summon_demonic_tyrant.cast_time|soul_shard<2)" );
  tyrant->add_action( "potion,if=variable.imp_despawn&variable.imp_despawn<time+gcd.max*2+action.summon_demonic_tyrant.cast_time&(prev_gcd.1.hand_of_guldan|prev_gcd.1.ruination)&(variable.imp_despawn&variable.imp_despawn<time+gcd.max+action.summon_demonic_tyrant.cast_time|soul_shard<2)" );
  tyrant->add_action( "power_siphon,if=cooldown.summon_demonic_tyrant.remains<15" );
  tyrant->add_action( "ruination,if=buff.dreadstalkers.remains>gcd.max+action.summon_demonic_tyrant.cast_time&(soul_shard=5|variable.imp_despawn)" );
  tyrant->add_action( "infernal_bolt,if=!buff.demonic_core.react&variable.imp_despawn>time+gcd.max*2+action.summon_demonic_tyrant.cast_time&soul_shard<3" );
  tyrant->add_action( "shadow_bolt,if=prev_gcd.1.call_dreadstalkers&soul_shard<4&buff.demonic_core.react<4" );
  tyrant->add_action( "shadow_bolt,if=prev_gcd.2.call_dreadstalkers&prev_gcd.1.shadow_bolt&buff.bloodlust.up&soul_shard<5" );
  tyrant->add_action( "shadow_bolt,if=prev_gcd.1.summon_vilefiend&(buff.demonic_calling.down|prev_gcd.2.grimoire_felguard)" );
  tyrant->add_action( "shadow_bolt,if=prev_gcd.1.grimoire_felguard&buff.demonic_core.react<3&buff.demonic_calling.remains>gcd.max*3" );
  tyrant->add_action( "hand_of_guldan,if=variable.imp_despawn>time+gcd.max*2+action.summon_demonic_tyrant.cast_time&!buff.demonic_core.react&buff.demonic_art_pit_lord.up&variable.imp_despawn<time+gcd.max*5+action.summon_demonic_tyrant.cast_time" );
  tyrant->add_action( "hand_of_guldan,if=variable.imp_despawn>time+gcd.max+action.summon_demonic_tyrant.cast_time&variable.imp_despawn<time+gcd.max*2+action.summon_demonic_tyrant.cast_time&buff.dreadstalkers.remains>gcd.max+action.summon_demonic_tyrant.cast_time&soul_shard>1" );
  tyrant->add_action( "shadow_bolt,if=!buff.demonic_core.react&variable.imp_despawn>time+gcd.max*2+action.summon_demonic_tyrant.cast_time&variable.imp_despawn<time+gcd.max*4+action.summon_demonic_tyrant.cast_time&soul_shard<3&buff.dreadstalkers.remains>gcd.max*2+action.summon_demonic_tyrant.cast_time" );
  tyrant->add_action( "grimoire_felguard,if=cooldown.summon_demonic_tyrant.remains<13+gcd.max&cooldown.summon_vilefiend.remains<gcd.max&cooldown.call_dreadstalkers.remains<gcd.max*3.33&(soul_shard=5-(pet.felguard.cooldown.soul_strike.remains<gcd.max)&talent.fel_invocation|soul_shard=5)" );
  tyrant->add_action( "summon_vilefiend,if=(buff.grimoire_felguard.up|cooldown.grimoire_felguard.remains>10|!talent.grimoire_felguard)&cooldown.summon_demonic_tyrant.remains<13&cooldown.call_dreadstalkers.remains<gcd.max*2.33&(soul_shard=5|soul_shard=4&(buff.demonic_core.react=4)|buff.grimoire_felguard.up)" );
  tyrant->add_action( "call_dreadstalkers,if=(!talent.summon_vilefiend|buff.vilefiend.up)&cooldown.summon_demonic_tyrant.remains<10&soul_shard>=(5-(buff.demonic_core.react>=3))|prev_gcd.3.grimoire_felguard" );
  tyrant->add_action( "summon_demonic_tyrant,if=variable.imp_despawn&variable.imp_despawn<time+gcd.max*2+cast_time|buff.dreadstalkers.up&buff.dreadstalkers.remains<gcd.max*2+cast_time" );
  tyrant->add_action( "hand_of_guldan,if=(variable.imp_despawn|buff.dreadstalkers.remains)&soul_shard>=3|soul_shard=5" );
  tyrant->add_action( "infernal_bolt,if=variable.imp_despawn&soul_shard<3" );
  tyrant->add_action( "demonbolt,if=variable.imp_despawn&buff.demonic_core.react&soul_shard<4|prev_gcd.1.call_dreadstalkers&soul_shard<4&buff.demonic_core.react=4|buff.demonic_core.react=4&soul_shard<4|buff.demonic_core.react>=2&cooldown.power_siphon.remains<5" );
  tyrant->add_action( "ruination,if=variable.imp_despawn|soul_shard=5&cooldown.summon_vilefiend.remains>gcd.max*3" );
  tyrant->add_action( "shadow_bolt" );
  tyrant->add_action( "infernal_bolt" );

  
  variables->add_action( "variable,name=next_tyrant_cd,op=set,value=cooldown.summon_demonic_tyrant.remains_expected" );
  variables->add_action( "variable,name=in_opener,op=set,value=0,if=pet.demonic_tyrant.active" );
  variables->add_action( "variable,name=imp_despawn,op=set,value=2*spell_haste*6+0.58+time,if=prev_gcd.1.hand_of_guldan&buff.dreadstalkers.up&cooldown.summon_demonic_tyrant.remains<13&variable.imp_despawn=0", "Sets an expected duration of valid Wild Imps on a tyrant Setup for the sake of casting Tyrant before expiration of Imps" );
  variables->add_action( "variable,name=imp_despawn,op=set,value=(variable.imp_despawn>?buff.dreadstalkers.remains+time),if=variable.imp_despawn", "Checks the Wild Imps in a Tyrant Setup alongside Dreadstalkers for the sake of casting Tyrant before Expiration Dreadstalkers or Imps" );
  variables->add_action( "variable,name=imp_despawn,op=set,value=variable.imp_despawn>?buff.grimoire_felguard.remains+time,if=variable.imp_despawn&buff.grimoire_felguard.up", "Checks The Wild Imps in a Tyrant Setup alongside Grimoire Felguard for the sake of casting Tyrant before Expiration of Grimoire Felguard or Imps" );
  variables->add_action( "variable,name=imp_despawn,op=set,value=0,if=buff.tyrant.up" );
  variables->add_action( "variable,name=impl,op=set,value=buff.tyrant.down,if=active_enemies>1+(talent.sacrificed_souls.enabled)", "Defines the viability of Implosion when Tyrant is down" );
  variables->add_action( "variable,name=impl,op=set,value=buff.tyrant.remains<6,if=active_enemies>2+(talent.sacrificed_souls.enabled)&active_enemies<5+(talent.sacrificed_souls.enabled)", "Defines the Viability of Implosion while Tyrant is Up" );
  variables->add_action( "variable,name=impl,op=set,value=buff.tyrant.remains<8,if=active_enemies>4+(talent.sacrificed_souls.enabled)", "Defines the Viability of Implosion while Tyrant is Up" );
  variables->add_action( "variable,name=pool_cores_for_tyrant,op=set,value=cooldown.summon_demonic_tyrant.remains<20&variable.next_tyrant_cd<20&(buff.demonic_core.stack<=2|!buff.demonic_core.up)&cooldown.summon_vilefiend.remains<gcd.max*8&cooldown.call_dreadstalkers.remains<gcd.max*8", "Restricts Demonic Core usage for the sake of having 2 or more Demonic Cores on Tyrant Setup" );
  variables->add_action( "variable,name=diabolic_ritual_remains,value=buff.diabolic_ritual_mother_of_chaos.remains,if=buff.diabolic_ritual_mother_of_chaos.up" );
  variables->add_action( "variable,name=diabolic_ritual_remains,value=buff.diabolic_ritual_overlord.remains,if=buff.diabolic_ritual_overlord.up" );
  variables->add_action( "variable,name=diabolic_ritual_remains,value=buff.diabolic_ritual_pit_lord.remains,if=buff.diabolic_ritual_pit_lord.up" );
}
//demonology_apl_end

//destruction_apl_start
void destruction( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* cleave = p->get_action_priority_list( "cleave" );
  action_priority_list_t* havoc = p->get_action_priority_list( "havoc" );
  action_priority_list_t* items = p->get_action_priority_list( "items" );
  action_priority_list_t* ogcd = p->get_action_priority_list( "ogcd" );
  action_priority_list_t* variables = p->get_action_priority_list( "variables" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "summon_pet" );
  precombat->add_action( "variable,name=cleave_apl,default=0,op=reset" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_use_buff", "Automatic Logic for Buff Trinkets in Trinket Slot 1" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_use_buff", "Automatic Logic for Buff Trinkets in Trinket Slot 2" );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_1_buffs&(trinket.1.cooldown.duration%%cooldown.summon_infernal.duration=0|cooldown.summon_infernal.duration%%trinket.1.cooldown.duration=0)" );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_2_buffs&(trinket.2.cooldown.duration%%cooldown.summon_infernal.duration=0|cooldown.summon_infernal.duration%%trinket.2.cooldown.duration=0)" );
  precombat->add_action( "variable,name=trinket_1_manual,value=trinket.1.is.spymasters_web", "Sets a specific Trinkets in Slot 1 to follow an APL line and not the automatic logic" );
  precombat->add_action( "variable,name=trinket_2_manual,value=trinket.2.is.spymasters_web", "Sets a specific Trinkets in Slot 2 to follow an APL line and not the automatic logic" );
  precombat->add_action( "variable,name=trinket_1_exclude,value=trinket.1.is.whispering_incarnate_icon", "For On Use Trinkets on slot 1 with on use effects you dont want to use in combat" );
  precombat->add_action( "variable,name=trinket_2_exclude,value=trinket.2.is.whispering_incarnate_icon", "For On Use Trinkets on slot 2 with on use effects you dont want to use in combat" );
  precombat->add_action( "variable,name=trinket_1_buff_duration,value=trinket.1.proc.any_dps.duration", "Sets the duration of the trinket in the automatic logic" );
  precombat->add_action( "variable,name=trinket_2_buff_duration,value=trinket.2.proc.any_dps.duration", "Sets the duration of the trinket in the automatic logic" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs|variable.trinket_2_buffs&((trinket.2.cooldown.duration%variable.trinket_2_buff_duration)*(1+0.5*trinket.2.has_buff.intellect)*(variable.trinket_2_sync))>((trinket.1.cooldown.duration%variable.trinket_1_buff_duration)*(1+0.5*trinket.1.has_buff.intellect)*(variable.trinket_1_sync))", "Automatic Logic in case both Trinkets are on use buffs" );
  precombat->add_action( "variable,name=allow_rof_2t_spender,default=2,op=reset" );
  precombat->add_action( "variable,name=do_rof_2t,value=variable.allow_rof_2t_spender>1.99&!(talent.cataclysm&talent.improved_chaos_bolt),op=set" );
  precombat->add_action( "variable,name=disable_cb_2t,value=variable.do_rof_2t|variable.allow_rof_2t_spender>0.01&variable.allow_rof_2t_spender<0.99" );
  precombat->add_action( "grimoire_of_sacrifice,if=talent.grimoire_of_sacrifice.enabled" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "cataclysm,if=active_enemies>=2&raid_event.adds.in>15" );
  precombat->add_action( "soul_fire" );
  precombat->add_action( "incinerate" );

  default_->add_action( "call_action_list,name=variables" );
  default_->add_action( "call_action_list,name=aoe,if=(active_enemies>=3)&!variable.cleave_apl" );
  default_->add_action( "call_action_list,name=cleave,if=active_enemies!=1|variable.cleave_apl" );
  default_->add_action( "call_action_list,name=ogcd" );
  default_->add_action( "call_action_list,name=items" );
  default_->add_action( "malevolence,if=cooldown.summon_infernal.remains>=55" );
  default_->add_action( "wait,sec=((buff.diabolic_ritual_mother_of_chaos.remains+buff.diabolic_ritual_overlord.remains+buff.diabolic_ritual_pit_lord.remains)),if=(diabolic_ritual&(buff.diabolic_ritual_mother_of_chaos.remains+buff.diabolic_ritual_overlord.remains+buff.diabolic_ritual_pit_lord.remains)<gcd.max*0.25)&soul_shard>2" );
  default_->add_action( "chaos_bolt,if=demonic_art" );
  default_->add_action( "soul_fire,if=buff.decimation.react&(soul_shard<=4|buff.decimation.remains<=gcd.max*2)&debuff.conflagrate.remains>=execute_time" );
  default_->add_action( "wither,if=talent.internal_combustion&(((dot.wither.remains-5*action.chaos_bolt.in_flight)<dot.wither.duration*0.4)|dot.wither.remains<3|(dot.wither.remains-action.chaos_bolt.execute_time)<5&action.chaos_bolt.usable)&(!talent.soul_fire|cooldown.soul_fire.remains+action.soul_fire.cast_time>(dot.wither.remains-5))&target.time_to_die>8&!action.soul_fire.in_flight_to_target" );
  default_->add_action( "conflagrate,if=talent.roaring_blaze&debuff.conflagrate.remains<1.5|full_recharge_time<=gcd.max*2|recharge_time<=8&(diabolic_ritual&(buff.diabolic_ritual_mother_of_chaos.remains+buff.diabolic_ritual_overlord.remains+buff.diabolic_ritual_pit_lord.remains)<gcd.max)&soul_shard>=1.5" );
  default_->add_action( "shadowburn,if=(cooldown.shadowburn.full_recharge_time<=gcd.max*3|debuff.eradication.remains<=gcd.max&talent.eradication&!action.chaos_bolt.in_flight&!talent.diabolic_ritual)&(talent.conflagration_of_chaos|talent.blistering_atrophy)&!demonic_art|fight_remains<=8" );
  default_->add_action( "chaos_bolt,if=buff.ritual_of_ruin.up" );
  default_->add_action( "shadowburn,if=(cooldown.summon_infernal.remains>=90&talent.rain_of_chaos)|buff.malevolence.up" );
  default_->add_action( "chaos_bolt,if=(cooldown.summon_infernal.remains>=90&talent.rain_of_chaos)|buff.malevolence.up" );
  default_->add_action( "ruination,if=(debuff.eradication.remains>=execute_time|!talent.eradication|!talent.shadowburn)" );
  default_->add_action( "cataclysm,if=raid_event.adds.in>15&(dot.immolate.refreshable&!talent.wither|talent.wither&dot.wither.refreshable)" );
  default_->add_action( "channel_demonfire,if=talent.raging_demonfire&(dot.immolate.remains+dot.wither.remains-5*(action.chaos_bolt.in_flight&talent.internal_combustion))>cast_time" );
  default_->add_action( "wither,if=!talent.internal_combustion&(((dot.wither.remains-5*(action.chaos_bolt.in_flight))<dot.wither.duration*0.3)|dot.wither.remains<3)&(!talent.cataclysm|cooldown.cataclysm.remains>dot.wither.remains)&(!talent.soul_fire|cooldown.soul_fire.remains+action.soul_fire.cast_time>(dot.wither.remains))&target.time_to_die>8&!action.soul_fire.in_flight_to_target" );
  default_->add_action( "immolate,if=(((dot.immolate.remains-5*(action.chaos_bolt.in_flight&talent.internal_combustion))<dot.immolate.duration*0.3)|dot.immolate.remains<3|(dot.immolate.remains-action.chaos_bolt.execute_time)<5&talent.internal_combustion&action.chaos_bolt.usable)&(!talent.cataclysm|cooldown.cataclysm.remains>dot.immolate.remains)&(!talent.soul_fire|cooldown.soul_fire.remains+action.soul_fire.cast_time>(dot.immolate.remains-5*talent.internal_combustion))&target.time_to_die>8&!action.soul_fire.in_flight_to_target" );
  default_->add_action( "summon_infernal" );
  default_->add_action( "incinerate,if=talent.diabolic_ritual&(diabolic_ritual&(buff.diabolic_ritual_mother_of_chaos.remains+buff.diabolic_ritual_overlord.remains+buff.diabolic_ritual_pit_lord.remains-2-!variable.disable_cb_2t*action.chaos_bolt.cast_time-variable.disable_cb_2t*gcd.max)<=0)" );
  default_->add_action( "chaos_bolt,if=variable.pooling_condition_cb&(cooldown.summon_infernal.remains>=gcd.max*3|soul_shard>4|!talent.rain_of_chaos)" );
  default_->add_action( "channel_demonfire" );
  default_->add_action( "dimensional_rift" );
  default_->add_action( "infernal_bolt" );
  default_->add_action( "conflagrate,if=charges>(max_charges-1)|fight_remains<gcd.max*charges" );
  default_->add_action( "soul_fire,if=buff.backdraft.up" );
  default_->add_action( "incinerate" );

  aoe->add_action( "call_action_list,name=ogcd" );
  aoe->add_action( "call_action_list,name=items" );
  aoe->add_action( "malevolence,if=cooldown.summon_infernal.remains>=55&soul_shard<4.7&(active_enemies<=3+active_dot.wither|time>30)" );
  aoe->add_action( "rain_of_fire,if=demonic_art" );
  aoe->add_action( "wait,sec=((buff.diabolic_ritual_mother_of_chaos.remains+buff.diabolic_ritual_overlord.remains+buff.diabolic_ritual_pit_lord.remains)),if=(diabolic_ritual&(buff.diabolic_ritual_mother_of_chaos.remains+buff.diabolic_ritual_overlord.remains+buff.diabolic_ritual_pit_lord.remains)<gcd.max*0.25)&soul_shard>2" );
  aoe->add_action( "incinerate,if=(diabolic_ritual&(buff.diabolic_ritual_mother_of_chaos.remains+buff.diabolic_ritual_overlord.remains+buff.diabolic_ritual_pit_lord.remains)<=action.incinerate.cast_time&(buff.diabolic_ritual_mother_of_chaos.remains+buff.diabolic_ritual_overlord.remains+buff.diabolic_ritual_pit_lord.remains)>gcd.max*0.25)" );
  aoe->add_action( "call_action_list,name=havoc,if=havoc_active&havoc_remains>gcd.max&active_enemies<5&(!cooldown.summon_infernal.up|!talent.summon_infernal)" );
  aoe->add_action( "dimensional_rift,if=soul_shard<4.7&(charges>2|fight_remains<cooldown.dimensional_rift.duration)" );
  aoe->add_action( "rain_of_fire,if=!talent.inferno&soul_shard>=(4.5-0.1*(active_dot.immolate+active_dot.wither))|soul_shard>=(3.5-0.1*(active_dot.immolate+active_dot.wither))|buff.ritual_of_ruin.up" );
  aoe->add_action( "wither,target_if=min:dot.wither.remains+99*debuff.havoc.remains+99*!dot.wither.ticking,if=dot.wither.refreshable&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>dot.wither.remains)&(!talent.raging_demonfire|cooldown.channel_demonfire.remains>remains|time<5)&(active_dot.wither<=4|time>15)&target.time_to_die>18" );
  aoe->add_action( "channel_demonfire,if=dot.immolate.remains+dot.wither.remains>cast_time&talent.raging_demonfire" );
  aoe->add_action( "shadowburn,if=((buff.malevolence.up&((talent.cataclysm&talent.raging_demonfire&active_enemies<=10&fight_remains>=60)|(talent.cataclysm&!talent.raging_demonfire&active_enemies<=8&fight_remains>=60)|active_enemies<=5))|(!talent.wither&talent.cataclysm&active_enemies<=5)|active_enemies<=3)&((cooldown.shadowburn.full_recharge_time<=gcd.max*3|debuff.eradication.remains<=gcd.max&talent.eradication&!action.chaos_bolt.in_flight&!talent.diabolic_ritual)&(talent.conflagration_of_chaos|talent.blistering_atrophy)|fight_remains<=8)" );
  aoe->add_action( "shadowburn,target_if=min:time_to_die,if=((buff.malevolence.up&((talent.cataclysm&talent.raging_demonfire&active_enemies<=10&fight_remains>=60)|(talent.cataclysm&!talent.raging_demonfire&active_enemies<=8&fight_remains>=60)|active_enemies<=5))|(!talent.wither&talent.cataclysm&active_enemies<=5)|active_enemies<=3)&((cooldown.shadowburn.full_recharge_time<=gcd.max*3|debuff.eradication.remains<=gcd.max&talent.eradication&!action.chaos_bolt.in_flight&!talent.diabolic_ritual)&(talent.conflagration_of_chaos|talent.blistering_atrophy)&time_to_die<5|fight_remains<=8)" );
  aoe->add_action( "ruination" );
  aoe->add_action( "rain_of_fire,if=pet.infernal.active&talent.rain_of_chaos" );
  aoe->add_action( "soul_fire,target_if=min:dot.wither.remains+dot.immolate.remains-5*debuff.conflagrate.up+100*debuff.havoc.remains,if=(buff.decimation.up)&!talent.raging_demonfire&havoc_active" );
  aoe->add_action( "soul_fire,target_if=min:(dot.wither.remains+dot.immolate.remains-5*debuff.conflagrate.up+100*debuff.havoc.remains),if=buff.decimation.up&active_dot.immolate<=4" );
  aoe->add_action( "infernal_bolt,if=soul_shard<2.5" );
  aoe->add_action( "chaos_bolt,if=soul_shard>3.5-(0.1*active_enemies)&!talent.rain_of_fire" );
  aoe->add_action( "cataclysm,if=raid_event.adds.in>15|talent.wither" );
  aoe->add_action( "havoc,target_if=min:((-target.time_to_die)<?-15)+dot.immolate.remains+99*(self.target=target),if=(!cooldown.summon_infernal.up|!talent.summon_infernal|(talent.inferno&active_enemies>4))&target.time_to_die>8&(cooldown.malevolence.remains>15|!talent.malevolence)|time<5" );
  aoe->add_action( "wither,target_if=min:dot.wither.remains+99*debuff.havoc.remains,if=dot.wither.refreshable&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>dot.wither.remains)&(!talent.raging_demonfire|cooldown.channel_demonfire.remains>remains|time<5)&active_dot.wither<=active_enemies&target.time_to_die>18" );
  aoe->add_action( "immolate,target_if=min:dot.immolate.remains+99*debuff.havoc.remains,if=dot.immolate.refreshable&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>dot.immolate.remains)&(!talent.raging_demonfire|cooldown.channel_demonfire.remains>remains|time<5)&(active_dot.immolate<=6&!(talent.diabolic_ritual&talent.inferno)|active_dot.immolate<=4)&target.time_to_die>18" );
  aoe->add_action( "call_action_list,name=ogcd" );
  aoe->add_action( "summon_infernal,if=cooldown.invoke_power_infusion_0.up|cooldown.invoke_power_infusion_0.duration=0|fight_remains>=120" );
  aoe->add_action( "rain_of_fire,if=debuff.pyrogenics.down&active_enemies<=4&!talent.diabolic_ritual" );
  aoe->add_action( "channel_demonfire,if=dot.immolate.remains+dot.wither.remains>cast_time" );
  aoe->add_action( "immolate,target_if=min:dot.immolate.remains+99*debuff.havoc.remains,if=((dot.immolate.refreshable&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>dot.immolate.remains))|active_enemies>active_dot.immolate)&target.time_to_die>10&!havoc_active&!(talent.diabolic_ritual&talent.inferno)" );
  aoe->add_action( "immolate,target_if=min:dot.immolate.remains+99*debuff.havoc.remains,if=((dot.immolate.refreshable&variable.havoc_immo_time<5.4)|(dot.immolate.remains<2&dot.immolate.remains<havoc_remains)|!dot.immolate.ticking|(variable.havoc_immo_time<2)*havoc_active)&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>dot.immolate.remains)&target.time_to_die>11&!(talent.diabolic_ritual&talent.inferno)" );
  aoe->add_action( "dimensional_rift" );
  aoe->add_action( "soul_fire,target_if=min:(dot.wither.remains+dot.immolate.remains-5*debuff.conflagrate.up+100*debuff.havoc.remains),if=buff.decimation.up" );
  aoe->add_action( "incinerate,if=talent.fire_and_brimstone.enabled&buff.backdraft.up" );
  aoe->add_action( "conflagrate,if=buff.backdraft.stack<2|!talent.backdraft" );
  aoe->add_action( "incinerate" );

  cleave->add_action( "call_action_list,name=items" );
  cleave->add_action( "call_action_list,name=ogcd" );
  cleave->add_action( "call_action_list,name=havoc,if=havoc_active&havoc_remains>gcd.max" );
  cleave->add_action( "variable,name=pool_soul_shards,value=cooldown.havoc.remains<=5|talent.mayhem" );
  cleave->add_action( "malevolence,if=(!cooldown.summon_infernal.up|!talent.summon_infernal)" );
  cleave->add_action( "havoc,target_if=min:((-target.time_to_die)<?-15)+dot.immolate.remains+99*(self.target=target),if=(!cooldown.summon_infernal.up|!talent.summon_infernal)&target.time_to_die>8" );
  cleave->add_action( "chaos_bolt,if=demonic_art" );
  cleave->add_action( "soul_fire,if=buff.decimation.react&(soul_shard<=4|buff.decimation.remains<=gcd.max*2)&debuff.conflagrate.remains>=execute_time&cooldown.havoc.remains" );
  cleave->add_action( "wither,target_if=min:dot.wither.remains+99*debuff.havoc.remains,if=talent.internal_combustion&(((dot.wither.remains-5*action.chaos_bolt.in_flight)<dot.wither.duration*0.4)|dot.wither.remains<3|(dot.wither.remains-action.chaos_bolt.execute_time)<5&action.chaos_bolt.usable)&(!talent.soul_fire|cooldown.soul_fire.remains+action.soul_fire.cast_time>(dot.wither.remains-5))&target.time_to_die>8&!action.soul_fire.in_flight_to_target" );
  cleave->add_action( "wither,target_if=min:dot.wither.remains+99*debuff.havoc.remains,if=!talent.internal_combustion&(((dot.wither.remains-5*(action.chaos_bolt.in_flight))<dot.wither.duration*0.3)|dot.wither.remains<3)&(!talent.soul_fire|cooldown.soul_fire.remains+action.soul_fire.cast_time>(dot.wither.remains))&target.time_to_die>8&!action.soul_fire.in_flight_to_target" );
  cleave->add_action( "conflagrate,if=(talent.roaring_blaze.enabled&full_recharge_time<=gcd.max*2)|recharge_time<=8&(diabolic_ritual&(buff.diabolic_ritual_mother_of_chaos.remains+buff.diabolic_ritual_overlord.remains+buff.diabolic_ritual_pit_lord.remains)<gcd.max)&!variable.pool_soul_shards" );
  cleave->add_action( "shadowburn,if=(cooldown.shadowburn.full_recharge_time<=gcd.max*3|debuff.eradication.remains<=gcd.max&talent.eradication&!action.chaos_bolt.in_flight&!talent.diabolic_ritual)&(talent.conflagration_of_chaos|talent.blistering_atrophy)|fight_remains<=8" );
  cleave->add_action( "chaos_bolt,if=buff.ritual_of_ruin.up" );
  cleave->add_action( "rain_of_fire,if=cooldown.summon_infernal.remains>=90&talent.rain_of_chaos" );
  cleave->add_action( "shadowburn,if=cooldown.summon_infernal.remains>=90&talent.rain_of_chaos" );
  cleave->add_action( "chaos_bolt,if=cooldown.summon_infernal.remains>=90&talent.rain_of_chaos" );
  cleave->add_action( "ruination,if=(debuff.eradication.remains>=execute_time|!talent.eradication|!talent.shadowburn)" );
  cleave->add_action( "cataclysm,if=raid_event.adds.in>15" );
  cleave->add_action( "channel_demonfire,if=talent.raging_demonfire&(dot.immolate.remains+dot.wither.remains-5*(action.chaos_bolt.in_flight&talent.internal_combustion))>cast_time" );
  cleave->add_action( "soul_fire,if=soul_shard<=3.5&(debuff.conflagrate.remains>cast_time+travel_time|!talent.roaring_blaze&buff.backdraft.up)&!variable.pool_soul_shards" );
  cleave->add_action( "immolate,target_if=min:dot.immolate.remains+99*debuff.havoc.remains,if=(dot.immolate.refreshable&(dot.immolate.remains<cooldown.havoc.remains|!dot.immolate.ticking))&(!talent.cataclysm|cooldown.cataclysm.remains>remains)&(!talent.soul_fire|cooldown.soul_fire.remains+(!talent.mayhem*action.soul_fire.cast_time)>dot.immolate.remains)&target.time_to_die>15" );
  cleave->add_action( "summon_infernal" );
  cleave->add_action( "incinerate,if=talent.diabolic_ritual&(diabolic_ritual&(buff.diabolic_ritual_mother_of_chaos.remains+buff.diabolic_ritual_overlord.remains+buff.diabolic_ritual_pit_lord.remains-2-!variable.disable_cb_2t*action.chaos_bolt.cast_time-variable.disable_cb_2t*gcd.max)<=0)" );
  cleave->add_action( "rain_of_fire,if=variable.pooling_condition&!talent.wither&buff.rain_of_chaos.up" );
  cleave->add_action( "rain_of_fire,if=variable.allow_rof_2t_spender>=1&!talent.wither&talent.pyrogenics&debuff.pyrogenics.remains<=gcd.max&(!talent.rain_of_chaos|cooldown.summon_infernal.remains>=gcd.max*3)&variable.pooling_condition" );
  cleave->add_action( "rain_of_fire,if=variable.do_rof_2t&variable.pooling_condition&(cooldown.summon_infernal.remains>=gcd.max*3|!talent.rain_of_chaos)" );
  cleave->add_action( "soul_fire,if=soul_shard<=4&talent.mayhem" );
  cleave->add_action( "chaos_bolt,if=!variable.disable_cb_2t&variable.pooling_condition_cb&(cooldown.summon_infernal.remains>=gcd.max*3|soul_shard>4|!talent.rain_of_chaos)" );
  cleave->add_action( "channel_demonfire" );
  cleave->add_action( "dimensional_rift" );
  cleave->add_action( "infernal_bolt" );
  cleave->add_action( "conflagrate,if=charges>(max_charges-1)|fight_remains<gcd.max*charges" );
  cleave->add_action( "incinerate" );

  havoc->add_action( "conflagrate,if=talent.backdraft&buff.backdraft.down&soul_shard>=1&soul_shard<=4" );
  havoc->add_action( "soul_fire,if=cast_time<havoc_remains&soul_shard<2.5" );
  havoc->add_action( "cataclysm,if=raid_event.adds.in>15|(talent.wither&dot.wither.remains<action.wither.duration*0.3)" );
  havoc->add_action( "immolate,target_if=min:dot.immolate.remains+100*debuff.havoc.remains,if=(((dot.immolate.refreshable&variable.havoc_immo_time<5.4)&target.time_to_die>5)|((dot.immolate.remains<2&dot.immolate.remains<havoc_remains)|!dot.immolate.ticking|variable.havoc_immo_time<2)&target.time_to_die>11)&soul_shard<4.5" );
  havoc->add_action( "wither,target_if=min:dot.wither.remains+100*debuff.havoc.remains,if=(((dot.wither.refreshable&variable.havoc_immo_time<5.4)&target.time_to_die>5)|((dot.wither.remains<2&dot.wither.remains<havoc_remains)|!dot.wither.ticking|variable.havoc_immo_time<2)&target.time_to_die>11)&soul_shard<4.5" );
  havoc->add_action( "shadowburn,if=(cooldown.shadowburn.full_recharge_time<=gcd.max*3|debuff.eradication.remains<=gcd.max&talent.eradication&!action.chaos_bolt.in_flight&!talent.diabolic_ritual)&(talent.conflagration_of_chaos|talent.blistering_atrophy)" );
  havoc->add_action( "shadowburn,if=havoc_remains<=gcd.max*3" );
  havoc->add_action( "chaos_bolt,if=cast_time<havoc_remains&((!talent.improved_chaos_bolt&active_enemies<=2)|(talent.improved_chaos_bolt&((talent.wither&talent.inferno&active_enemies<=2)|(((talent.wither&talent.cataclysm)|(!talent.wither&talent.inferno))&active_enemies<=3)|(!talent.wither&talent.cataclysm&active_enemies<=4))))" );
  havoc->add_action( "rain_of_fire,if=active_enemies>=3" );
  havoc->add_action( "channel_demonfire,if=soul_shard<4.5" );
  havoc->add_action( "conflagrate,if=!talent.backdraft" );
  havoc->add_action( "dimensional_rift,if=soul_shard<4.7&(charges>2|fight_remains<cooldown.dimensional_rift.duration)" );
  havoc->add_action( "incinerate,if=cast_time<havoc_remains" );

  items->add_action( "use_item,name=spymasters_web,if=pet.infernal.remains>=10&pet.infernal.remains<=20&buff.spymasters_report.stack>=38&(fight_remains>240|fight_remains<=140)|fight_remains<=30" );
  items->add_action( "use_item,slot=trinket1,if=(variable.infernal_active|!talent.summon_infernal|variable.trinket_1_will_lose_cast)&(variable.trinket_priority=1|variable.trinket_2_exclude|!trinket.2.has_cooldown|(trinket.2.cooldown.remains|variable.trinket_priority=2&cooldown.summon_infernal.remains>20&!variable.infernal_active&trinket.2.cooldown.remains<cooldown.summon_infernal.remains))&variable.trinket_1_buffs&!variable.trinket_1_manual|(variable.trinket_1_buff_duration+1>=fight_remains)" );
  items->add_action( "use_item,slot=trinket2,if=(variable.infernal_active|!talent.summon_infernal|variable.trinket_2_will_lose_cast)&(variable.trinket_priority=2|variable.trinket_1_exclude|!trinket.1.has_cooldown|(trinket.1.cooldown.remains|variable.trinket_priority=1&cooldown.summon_infernal.remains>20&!variable.infernal_active&trinket.1.cooldown.remains<cooldown.summon_infernal.remains))&variable.trinket_2_buffs&!variable.trinket_2_manual|(variable.trinket_2_buff_duration+1>=fight_remains)" );
  items->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=!variable.trinket_1_buffs&!variable.trinket_1_manual&(!variable.trinket_1_buffs&(trinket.2.cooldown.remains|!variable.trinket_2_buffs)|talent.summon_infernal&cooldown.summon_infernal.remains_expected>20&!prev_gcd.1.summon_infernal|!talent.summon_infernal)" );
  items->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=!variable.trinket_2_buffs&!variable.trinket_2_manual&(!variable.trinket_2_buffs&(trinket.1.cooldown.remains|!variable.trinket_1_buffs)|talent.summon_infernal&cooldown.summon_infernal.remains_expected>20&!prev_gcd.1.summon_infernal|!talent.summon_infernal)" );
  items->add_action( "use_item,use_off_gcd=1,slot=main_hand" );

  ogcd->add_action( "potion,if=variable.infernal_active|!talent.summon_infernal" );
  ogcd->add_action( "invoke_external_buff,name=power_infusion,if=variable.infernal_active|!talent.summon_infernal|(fight_remains<cooldown.summon_infernal.remains_expected+10+cooldown.invoke_power_infusion_0.duration&fight_remains>cooldown.invoke_power_infusion_0.duration)|fight_remains<cooldown.summon_infernal.remains_expected+15" );
  ogcd->add_action( "berserking,if=variable.infernal_active|!talent.summon_infernal|(fight_remains<(cooldown.summon_infernal.remains_expected+cooldown.berserking.duration)&(fight_remains>cooldown.berserking.duration))|fight_remains<cooldown.summon_infernal.remains_expected" );
  ogcd->add_action( "blood_fury,if=variable.infernal_active|!talent.summon_infernal|(fight_remains<cooldown.summon_infernal.remains_expected+10+cooldown.blood_fury.duration&fight_remains>cooldown.blood_fury.duration)|fight_remains<cooldown.summon_infernal.remains" );
  ogcd->add_action( "fireblood,if=variable.infernal_active|!talent.summon_infernal|(fight_remains<cooldown.summon_infernal.remains_expected+10+cooldown.fireblood.duration&fight_remains>cooldown.fireblood.duration)|fight_remains<cooldown.summon_infernal.remains_expected" );
  ogcd->add_action( "ancestral_call,if=variable.infernal_active|!talent.summon_infernal|(fight_remains<(cooldown.summon_infernal.remains_expected+cooldown.berserking.duration)&(fight_remains>cooldown.berserking.duration))|fight_remains<cooldown.summon_infernal.remains_expected" );

  variables->add_action( "variable,name=havoc_immo_time,op=reset" );
  variables->add_action( "variable,name=pooling_condition,value=(soul_shard>=3|(talent.secrets_of_the_coven&buff.infernal_bolt.up|buff.decimation.up)&soul_shard>=3),default=1,op=set" );
  variables->add_action( "variable,name=pooling_condition_cb,value=variable.pooling_condition|pet.infernal.active&soul_shard>=3,default=1,op=set" );
  variables->add_action( "cycling_variable,name=havoc_immo_time,op=add,value=dot.immolate.remains*debuff.havoc.up<?dot.wither.remains*debuff.havoc.up" );
  variables->add_action( "variable,name=infernal_active,op=set,value=pet.infernal.active|(cooldown.summon_infernal.duration-cooldown.summon_infernal.remains)<20" );
  variables->add_action( "variable,name=trinket_1_will_lose_cast,value=((floor((fight_remains%trinket.1.cooldown.duration)+1)!=floor((fight_remains+(cooldown.summon_infernal.duration-cooldown.summon_infernal.remains))%cooldown.summon_infernal.duration))&(floor((fight_remains%trinket.1.cooldown.duration)+1))!=(floor(((fight_remains-cooldown.summon_infernal.remains)%trinket.1.cooldown.duration)+1))|((floor((fight_remains%trinket.1.cooldown.duration)+1)=floor((fight_remains+(cooldown.summon_infernal.duration-cooldown.summon_infernal.remains))%cooldown.summon_infernal.duration))&(((fight_remains-cooldown.summon_infernal.remains%%trinket.1.cooldown.duration)-cooldown.summon_infernal.remains-variable.trinket_1_buff_duration)>0)))&cooldown.summon_infernal.remains>20" );
  variables->add_action( "variable,name=trinket_2_will_lose_cast,value=((floor((fight_remains%trinket.2.cooldown.duration)+1)!=floor((fight_remains+(cooldown.summon_infernal.duration-cooldown.summon_infernal.remains))%cooldown.summon_infernal.duration))&(floor((fight_remains%trinket.2.cooldown.duration)+1))!=(floor(((fight_remains-cooldown.summon_infernal.remains)%trinket.2.cooldown.duration)+1))|((floor((fight_remains%trinket.2.cooldown.duration)+1)=floor((fight_remains+(cooldown.summon_infernal.duration-cooldown.summon_infernal.remains))%cooldown.summon_infernal.duration))&(((fight_remains-cooldown.summon_infernal.remains%%trinket.2.cooldown.duration)-cooldown.summon_infernal.remains-variable.trinket_2_buff_duration)>0)))&cooldown.summon_infernal.remains>20" );
}
//destruction_apl_end

} // namespace warlock_apl
