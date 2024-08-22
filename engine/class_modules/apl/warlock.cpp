#include "class_modules/apl/warlock.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace warlock_apl{
  std::string potion( const player_t* p )
  {
    if ( p->true_level >= 70 ) return "elemental_potion_of_ultimate_power_3";
    return ( p->true_level >= 60 ) ? "spectral_intellect" : "disabled";
  }

  std::string flask( const player_t* p )
  {
    if ( p->true_level >= 70 ) return "iced_phial_of_corrupting_rage_3";
    return ( p->true_level >= 60 ) ? "spectral_flask_of_power" : "disabled";
  }

  std::string food( const player_t* p )
  {
    if ( p->true_level >= 70 ) return "fated_fortune_cookie";
    return ( p->true_level >= 60 ) ? "feast_of_gluttonous_hedonism" : "disabled";
  }

  std::string rune( const player_t* p )
  {
    if ( p->true_level >= 70 ) return "draconic_augment_rune";
    return ( p->true_level >= 60 ) ? "veiled" : "disabled";
  }

  std::string temporary_enchant( const player_t* p )
  {
    if ( p->true_level >= 70 ) return "main_hand:howling_rune_3";
    return ( p->true_level >= 60 ) ? "main_hand:shadowcore_oil" : "disabled";
  }

//affliction_apl_start
void affliction( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* cleave = p->get_action_priority_list( "cleave" );
  action_priority_list_t* end_of_fight = p->get_action_priority_list( "end_of_fight" );
  action_priority_list_t* items = p->get_action_priority_list( "items" );
  action_priority_list_t* ogcd = p->get_action_priority_list( "ogcd" );
  action_priority_list_t* se_maintenance = p->get_action_priority_list( "se_maintenance" );
  action_priority_list_t* variables = p->get_action_priority_list( "variables" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "summon_pet" );
  precombat->add_action( "variable,name=cleave_apl,default=0,op=reset" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_use_buff" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_use_buff" );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_1_buffs&(trinket.1.cooldown.duration%%cooldown.soul_rot.duration=0|cooldown.soul_rot.duration%%trinket.1.cooldown.duration=0)" );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_2_buffs&(trinket.2.cooldown.duration%%cooldown.soul_rot.duration=0|cooldown.soul_rot.duration%%trinket.2.cooldown.duration=0)" );
  precombat->add_action( "variable,name=trinket_1_manual,value=trinket.1.is.belorrelos_the_suncaller|trinket.1.is.timethiefs_gambit|trinket.1.is.spymasters_web|trinket.1.is.aberrant_spellforge" );
  precombat->add_action( "variable,name=trinket_2_manual,value=trinket.2.is.belorrelos_the_suncaller|trinket.2.is.timethiefs_gambit|trinket.2.is.spymasters_web|trinket.2.is.aberrant_spellforge" );
  precombat->add_action( "variable,name=trinket_1_exclude,value=trinket.1.is.ruby_whelp_shell|trinket.1.is.whispering_incarnate_icon" );
  precombat->add_action( "variable,name=trinket_2_exclude,value=trinket.2.is.ruby_whelp_shell|trinket.2.is.whispering_incarnate_icon" );
  precombat->add_action( "variable,name=trinket_1_buff_duration,value=trinket.1.proc.any_dps.duration+(trinket.1.is.mirror_of_fractured_tomorrows*20)+(trinket.1.is.nymues_unraveling_spindle*2)" );
  precombat->add_action( "variable,name=trinket_2_buff_duration,value=trinket.2.proc.any_dps.duration+(trinket.2.is.mirror_of_fractured_tomorrows*20)+(trinket.2.is.nymues_unraveling_spindle*2)" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs|variable.trinket_2_buffs&((trinket.2.cooldown.duration%variable.trinket_2_buff_duration)*(1+0.5*trinket.2.has_buff.intellect)*(variable.trinket_2_sync)*(1-0.5*(trinket.2.is.mirror_of_fractured_tomorrows|trinket.2.is.ashes_of_the_embersoul)))>((trinket.1.cooldown.duration%variable.trinket_1_buff_duration)*(1+0.5*trinket.1.has_buff.intellect)*(variable.trinket_1_sync)*(1-0.5*(trinket.1.is.mirror_of_fractured_tomorrows|trinket.1.is.ashes_of_the_embersoul)))" );
  precombat->add_action( "grimoire_of_sacrifice,if=talent.grimoire_of_sacrifice.enabled" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "seed_of_corruption,if=spell_targets.seed_of_corruption_aoe>2|spell_targets.seed_of_corruption_aoe>1" );
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
  default_->add_action( "summon_darkglare,if=variable.cd_dots_up" );
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
  aoe->add_action( "malefic_rapture,if=(cooldown.summon_darkglare.remains>15|soul_shard>3)&buff.tormented_crescendo.up" );
  aoe->add_action( "malefic_rapture,if=soul_shard>4|(talent.tormented_crescendo&buff.tormented_crescendo.react=1&soul_shard>3)" );
  aoe->add_action( "malefic_rapture,if=talent.tormented_crescendo&buff.tormented_crescendo.react" );
  aoe->add_action( "malefic_rapture,if=talent.tormented_crescendo&buff.tormented_crescendo.react=2" );
  aoe->add_action( "malefic_rapture,if=(variable.cd_dots_up|variable.vt_ps_up)&(soul_shard>2|cooldown.oblivion.remains>10|!talent.oblivion)" );
  aoe->add_action( "malefic_rapture,if=talent.tormented_crescendo&talent.nightfall&buff.tormented_crescendo.react&buff.nightfall.react" );
  aoe->add_action( "drain_soul,cycle_targets=1,if=talent.drain_soul&buff.nightfall.react&talent.shadow_embrace&(debuff.shadow_embrace.stack<3|debuff.shadow_embrace.remains<3)" );
  aoe->add_action( "drain_soul,cycle_targets=1,interrupt_global=1,if=talent.drain_soul&(talent.shadow_embrace&(debuff.shadow_embrace.stack<3|debuff.shadow_embrace.remains<3))|!talent.shadow_embrace" );
  aoe->add_action( "shadow_bolt,cycle_targets=1,if=buff.nightfall.react&talent.shadow_embrace&(debuff.shadow_embrace.stack<3|debuff.shadow_embrace.remains<3)" );

  cleave->add_action( "call_action_list,name=ogcd" );
  cleave->add_action( "call_action_list,name=items" );
  cleave->add_action( "vile_taint,if=!talent.soul_rot|(variable.min_agony<1.5|cooldown.soul_rot.remains<=execute_time+gcd.max)|cooldown.soul_rot.remains>=12" );
  cleave->add_action( "phantom_singularity,if=(cooldown.soul_rot.remains<=execute_time|cooldown.soul_rot.remains>=25)&active_dot.agony=2" );
  cleave->add_action( "soul_rot,if=(variable.vt_up|variable.ps_up)&active_dot.agony=2" );
  cleave->add_action( "agony,target_if=min:remains,if=(remains<cooldown.vile_taint.remains+action.vile_taint.cast_time|!talent.vile_taint)&remains<5&fight_remains>5" );
  cleave->add_action( "unstable_affliction,if=remains<5&fight_remains>3" );
  cleave->add_action( "haunt,if=debuff.haunt.remains<3" );
  cleave->add_action( "wither,target_if=min:remains,if=remains<5&!(action.seed_of_corruption.in_flight|dot.seed_of_corruption.remains>0)&fight_remains>5" );
  cleave->add_action( "corruption,target_if=min:remains,if=remains<5&!(action.seed_of_corruption.in_flight|dot.seed_of_corruption.remains>0)&fight_remains>5" );
  cleave->add_action( "summon_darkglare,if=(!talent.shadow_embrace|debuff.shadow_embrace.stack=3)&variable.ps_up&variable.vt_up&variable.sr_up|cooldown.invoke_power_infusion_0.duration>0&cooldown.invoke_power_infusion_0.up&!talent.soul_rot" );
  cleave->add_action( "malefic_rapture,if=talent.tormented_crescendo&buff.tormented_crescendo.react=1&soul_shard>3" );
  cleave->add_action( "malefic_rapture,if=talent.tormented_crescendo&buff.tormented_crescendo.react" );
  cleave->add_action( "malefic_rapture,if=talent.tormented_crescendo&buff.tormented_crescendo.react=2" );
  cleave->add_action( "malefic_rapture,if=(variable.cd_dots_up|variable.vt_ps_up)&(soul_shard>2|cooldown.oblivion.remains>10|!talent.oblivion)" );
  cleave->add_action( "malefic_rapture,if=talent.tormented_crescendo&talent.nightfall&buff.tormented_crescendo.react&buff.nightfall.react" );
  cleave->add_action( "drain_soul,interrupt=1,if=talent.shadow_embrace&(debuff.shadow_embrace.stack<3|debuff.shadow_embrace.remains<3)" );
  cleave->add_action( "drain_soul,target_if=min:debuff.shadow_embrace.remains,if=buff.nightfall.react&(talent.shadow_embrace&(debuff.shadow_embrace.stack<3|debuff.shadow_embrace.remains<3)|!talent.shadow_embrace)" );
  cleave->add_action( "shadow_bolt,target_if=min:debuff.shadow_embrace.remains,if=buff.nightfall.react&(talent.shadow_embrace&(debuff.shadow_embrace.stack<3|debuff.shadow_embrace.remains<3)|!talent.shadow_embrace)" );
  cleave->add_action( "malefic_rapture,if=variable.cd_dots_up|variable.vt_ps_up" );
  cleave->add_action( "malefic_rapture,if=soul_shard>3" );
  cleave->add_action( "agony,target_if=refreshable" );
  cleave->add_action( "wither,target_if=refreshable" );
  cleave->add_action( "corruption,target_if=refreshable" );
  cleave->add_action( "malefic_rapture,if=soul_shard>1" );
  cleave->add_action( "drain_soul,interrupt_global=1" );
  cleave->add_action( "shadow_bolt" );

  end_of_fight->add_action( "drain_soul,if=talent.demonic_soul&(fight_remains<5&buff.nightfall.react|prev_gcd.1.haunt&buff.nightfall.react=2&!buff.tormented_crescendo.react)" );
  end_of_fight->add_action( "oblivion,if=soul_shard>1&fight_remains<(soul_shard+buff.tormented_crescendo.react)*gcd.max+execute_time" );
  end_of_fight->add_action( "malefic_rapture,if=fight_remains<4&(!talent.demonic_soul|talent.demonic_soul&buff.nightfall.react<1)" );

  items->add_action( "use_item,name=aberrant_spellforge,use_off_gcd=1,if=gcd.remains>gcd.max*0.8" );
  items->add_action( "use_item,name=spymasters_web,if=variable.cd_dots_up&(fight_remains<=80|talent.drain_soul&target.health.pct<20)|fight_remains<20" );
  items->add_action( "use_item,slot=trinket1,if=(variable.cds_active)&(variable.trinket_priority=1|variable.trinket_2_exclude|!trinket.2.has_cooldown|(trinket.2.cooldown.remains|variable.trinket_priority=2&cooldown.summon_darkglare.remains>20&!pet.darkglare.active&trinket.2.cooldown.remains<cooldown.summon_darkglare.remains))&variable.trinket_1_buffs&!variable.trinket_1_manual|(variable.trinket_1_buff_duration+1>=fight_remains)" );
  items->add_action( "use_item,slot=trinket2,if=(variable.cds_active)&(variable.trinket_priority=2|variable.trinket_1_exclude|!trinket.1.has_cooldown|(trinket.1.cooldown.remains|variable.trinket_priority=1&cooldown.summon_darkglare.remains>20&!pet.darkglare.active&trinket.1.cooldown.remains<cooldown.summon_darkglare.remains))&variable.trinket_2_buffs&!variable.trinket_2_manual|(variable.trinket_2_buff_duration+1>=fight_remains)" );
  items->add_action( "use_item,name=time_thiefs_gambit,if=variable.cds_active|fight_remains<15|((trinket.1.cooldown.duration<cooldown.summon_darkglare.remains_expected+5)&active_enemies=1)|(active_enemies>1&havoc_active)" );
  items->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=!variable.trinket_1_buffs&!variable.trinket_1_manual&(!variable.trinket_1_buffs&(trinket.2.cooldown.remains|!variable.trinket_2_buffs)|talent.summon_darkglare&cooldown.summon_darkglare.remains_expected>20|!talent.summon_darkglare)" );
  items->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=!variable.trinket_2_buffs&!variable.trinket_2_manual&(!variable.trinket_2_buffs&(trinket.1.cooldown.remains|!variable.trinket_1_buffs)|talent.summon_darkglare&cooldown.summon_darkglare.remains_expected>20|!talent.summon_darkglare)" );
  items->add_action( "use_item,use_off_gcd=1,slot=main_hand" );

  ogcd->add_action( "potion,if=variable.cds_active|fight_remains<32|dot.soul_rot.ticking&time<20" );
  ogcd->add_action( "berserking,if=variable.cds_active|fight_remains<14|dot.soul_rot.ticking&time<20" );
  ogcd->add_action( "blood_fury,if=variable.cds_active|fight_remains<17|dot.soul_rot.ticking&time<20" );
  ogcd->add_action( "invoke_external_buff,name=power_infusion,if=variable.cds_active&(trinket.1.is.nymues_unraveling_spindle&trinket.1.cooldown.remains|trinket.2.is.nymues_unraveling_spindle&trinket.2.cooldown.remains|!equipped.nymues_unraveling_spindle)" );
  ogcd->add_action( "fireblood,if=variable.cds_active|fight_remains<10|dot.soul_rot.ticking&time<20" );
  ogcd->add_action( "ancestral_call,if=variable.cds_active|fight_remains<17|dot.soul_rot.ticking&time<20" );

  se_maintenance->add_action( "drain_soul,interrupt=1,if=talent.shadow_embrace&talent.drain_soul&(debuff.shadow_embrace.stack<debuff.shadow_embrace.max_stack|debuff.shadow_embrace.remains<3)&active_enemies<=4&fight_remains>15,interrupt_if=debuff.shadow_embrace.stack=debuff.shadow_embrace.max_stack" );
  se_maintenance->add_action( "shadow_bolt,if=talent.shadow_embrace&((debuff.shadow_embrace.stack+action.shadow_bolt.in_flight_to_target_count)<debuff.shadow_embrace.max_stack|debuff.shadow_embrace.remains<3&!action.shadow_bolt.in_flight_to_target)&active_enemies<=4&fight_remains>15" );

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
  precombat->add_action( "variable,name=first_tyrant_time,op=set,value=12" );
  precombat->add_action( "variable,name=first_tyrant_time,op=add,value=action.grimoire_felguard.execute_time,if=talent.grimoire_felguard.enabled" );
  precombat->add_action( "variable,name=first_tyrant_time,op=add,value=action.summon_vilefiend.execute_time,if=talent.summon_vilefiend.enabled" );
  precombat->add_action( "variable,name=first_tyrant_time,op=add,value=gcd.max,if=talent.grimoire_felguard.enabled|talent.summon_vilefiend.enabled" );
  precombat->add_action( "variable,name=first_tyrant_time,op=sub,value=action.summon_demonic_tyrant.execute_time+action.shadow_bolt.execute_time" );
  precombat->add_action( "variable,name=first_tyrant_time,op=min,value=10" );
  precombat->add_action( "variable,name=in_opener,op=set,value=1" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_use_buff" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_use_buff" );
  precombat->add_action( "variable,name=trinket_1_exclude,value=trinket.1.is.ruby_whelp_shell" );
  precombat->add_action( "variable,name=trinket_2_exclude,value=trinket.2.is.ruby_whelp_shell" );
  precombat->add_action( "variable,name=trinket_1_manual,value=trinket.1.is.spymasters_web|trinket.1.is.imperfect_ascendancy_serum" );
  precombat->add_action( "variable,name=trinket_2_manual,value=trinket.2.is.spymasters_web|trinket.2.is.imperfect_ascendancy_serum" );
  precombat->add_action( "variable,name=trinket_1_buff_duration,value=trinket.1.proc.any_dps.duration+(trinket.1.is.mirror_of_fractured_tomorrows*20)" );
  precombat->add_action( "variable,name=trinket_2_buff_duration,value=trinket.2.proc.any_dps.duration+(trinket.2.is.mirror_of_fractured_tomorrows*20)" );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_1_buffs&(trinket.1.cooldown.duration%%cooldown.summon_demonic_tyrant.duration=0|cooldown.summon_demonic_tyrant.duration%%trinket.1.cooldown.duration=0)" );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_2_buffs&(trinket.2.cooldown.duration%%cooldown.summon_demonic_tyrant.duration=0|cooldown.summon_demonic_tyrant.duration%%trinket.2.cooldown.duration=0)" );
  precombat->add_action( "variable,name=damage_trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&!variable.trinket_2_buffs&trinket.2.ilvl>trinket.1.ilvl" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs|variable.trinket_2_buffs&((trinket.2.cooldown.duration%variable.trinket_2_buff_duration)*(1.5+trinket.2.has_buff.intellect)*(variable.trinket_2_sync)*(1-0.5*trinket.2.is.mirror_of_fractured_tomorrows))>(((trinket.1.cooldown.duration%variable.trinket_1_buff_duration)*(1.5+trinket.1.has_buff.intellect)*(variable.trinket_1_sync)*(1-0.5*trinket.1.is.mirror_of_fractured_tomorrows))*(1+((trinket.1.ilvl-trinket.2.ilvl)%100)))" );
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
  default_->add_action( "hand_of_guldan,if=time<0.5&(fight_remains%%95>40|fight_remains%%95<15)&(talent.reign_of_tyranny|active_enemies>2)" );
  default_->add_action( "call_dreadstalkers,if=cooldown.summon_demonic_tyrant.remains>25|variable.next_tyrant_cd>25" );
  default_->add_action( "summon_demonic_tyrant,if=buff.vilefiend.up|buff.grimoire_felguard.up|cooldown.grimoire_felguard.remains>60" );
  default_->add_action( "summon_vilefiend,if=cooldown.summon_demonic_tyrant.remains>30" );
  default_->add_action( "demonbolt,target_if=(!debuff.doom.up|!action.demonbolt.in_flight&debuff.doom.remains<=2),if=buff.demonic_core.up&(((!talent.soul_strike|cooldown.soul_strike.remains>gcd.max*2&talent.fel_invocation)&soul_shard<4)|soul_shard<(4-(active_enemies>2)))&!prev_gcd.1.demonbolt&talent.doom&cooldown.summon_demonic_tyrant.remains>15" );
  default_->add_action( "demonbolt,if=buff.demonic_core.stack>=3&soul_shard<=3&!variable.pool_cores_for_tyrant" );
  default_->add_action( "power_siphon,if=buff.demonic_core.stack<3&cooldown.summon_demonic_tyrant.remains>25" );
  default_->add_action( "demonic_strength,if=!(raid_event.adds.in<45-raid_event.add.duration)" );
  default_->add_action( "bilescourge_bombers" );
  default_->add_action( "guillotine,if=(cooldown.demonic_strength.remains|!talent.demonic_strength)&(!raid_event.adds.exists|raid_event.adds.exists&raid_event.adds.remains>6)" );
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
  variables->add_action( "variable,name=imp_despawn,op=set,value=2*spell_haste*6+0.58+time,if=prev_gcd.1.hand_of_guldan&buff.dreadstalkers.up&cooldown.summon_demonic_tyrant.remains<13&variable.imp_despawn=0" );
  variables->add_action( "variable,name=imp_despawn,op=set,value=(variable.imp_despawn>?buff.dreadstalkers.remains+time),if=variable.imp_despawn" );
  variables->add_action( "variable,name=imp_despawn,op=set,value=variable.imp_despawn>?buff.grimoire_felguard.remains+time,if=variable.imp_despawn&buff.grimoire_felguard.up" );
  variables->add_action( "variable,name=imp_despawn,op=set,value=0,if=buff.tyrant.up" );
  variables->add_action( "variable,name=buff_sync_cd,op=set,value=variable.next_tyrant_cd,if=!variable.in_opener" );
  variables->add_action( "variable,name=buff_sync_cd,op=set,value=12,if=!pet.dreadstalker.active" );
  variables->add_action( "variable,name=buff_sync_cd,op=set,value=0,if=variable.in_opener&pet.dreadstalker.active&buff.wild_imps.stack>0&!talent.summon_vilefiend.enabled" );
  variables->add_action( "variable,name=buff_sync_cd,op=set,value=0,if=variable.in_opener&pet.dreadstalker.active&prev_gcd.1.hand_of_guldan&talent.summon_vilefiend.enabled" );
  variables->add_action( "variable,name=impl,op=set,value=buff.tyrant.down,if=active_enemies>1+(talent.sacrificed_souls.enabled)" );
  variables->add_action( "variable,name=impl,op=set,value=buff.tyrant.remains<6,if=active_enemies>2+(talent.sacrificed_souls.enabled)&active_enemies<5+(talent.sacrificed_souls.enabled)" );
  variables->add_action( "variable,name=impl,op=set,value=buff.tyrant.remains<8,if=active_enemies>4+(talent.sacrificed_souls.enabled)" );
  variables->add_action( "variable,name=pool_cores_for_tyrant,op=set,value=cooldown.summon_demonic_tyrant.remains<20&variable.next_tyrant_cd<20&(buff.demonic_core.stack<=2|!buff.demonic_core.up)&cooldown.summon_vilefiend.remains<gcd.max*8&cooldown.call_dreadstalkers.remains<gcd.max*8" );
  variables->add_action( "variable,name=hogshard,value=3,if=soul_shard>=3" );
  variables->add_action( "variable,name=hogshard,value=soul_shard,if=soul_shard<3" );
  variables->add_action( "variable,name=all_pets,op=set,value=buff.wild_imps.stack>10&prev_gcd.1.hand_of_guldan&buff.vilefiend.up&buff.dreadstalkers.up" );
  variables->add_action( "variable,name=diabolic_ritual_remains,value=buff.diabolic_ritual_mother_of_chaos.remains,if=buff.diabolic_ritual_mother_of_chaos.up" );
  variables->add_action( "variable,name=diabolic_ritual_remains,value=buff.diabolic_ritual_overlord.remains,if=buff.diabolic_ritual_overlord.up" );
  variables->add_action( "variable,name=diabolic_ritual_remains,value=buff.diabolic_ritual_pit_lord.remains,if=buff.diabolic_ritual_pit_lord.up" );
  variables->add_action( "variable,name=demonic_art_up,value=buff.demonic_art_pit_lord.up|buff.demonic_art_overlord.up|buff.demonic_art_mother_of_chaos.up,if=talent.diabolic_ritual,default=0,op=set" );
}
//demonology_apl_end

//destruction_apl_start
void destruction( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );

  default_->add_action( "incinerate" );
}
//destruction_apl_end

} // namespace warlock_apl
