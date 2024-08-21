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

  default_->add_action( "shadow_bolt" );
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
