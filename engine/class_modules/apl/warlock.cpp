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
  action_priority_list_t* items = p->get_action_priority_list( "items" );
  action_priority_list_t* ogcd = p->get_action_priority_list( "ogcd" );
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
  precombat->add_action( "variable,name=trinket_1_manual,value=trinket.1.is.belorrelos_the_suncaller|trinket.1.is.timethiefs_gambit" );
  precombat->add_action( "variable,name=trinket_2_manual,value=trinket.2.is.belorrelos_the_suncaller|trinket.2.is.timethiefs_gambit" );
  precombat->add_action( "variable,name=trinket_1_exclude,value=trinket.1.is.ruby_whelp_shell|trinket.1.is.whispering_incarnate_icon" );
  precombat->add_action( "variable,name=trinket_2_exclude,value=trinket.2.is.ruby_whelp_shell|trinket.2.is.whispering_incarnate_icon" );
  precombat->add_action( "variable,name=trinket_1_buff_duration,value=trinket.1.proc.any_dps.duration+(trinket.1.is.mirror_of_fractured_tomorrows*20)+(trinket.1.is.nymues_unraveling_spindle*2)" );
  precombat->add_action( "variable,name=trinket_2_buff_duration,value=trinket.2.proc.any_dps.duration+(trinket.2.is.mirror_of_fractured_tomorrows*20)+(trinket.2.is.nymues_unraveling_spindle*2)" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs|variable.trinket_2_buffs&((trinket.2.cooldown.duration%variable.trinket_2_buff_duration)*(1+0.5*trinket.2.has_buff.intellect)*(variable.trinket_2_sync)*(1-0.5*(trinket.2.is.mirror_of_fractured_tomorrows|trinket.2.is.ashes_of_the_embersoul)))>((trinket.1.cooldown.duration%variable.trinket_1_buff_duration)*(1+0.5*trinket.1.has_buff.intellect)*(variable.trinket_1_sync)*(1-0.5*(trinket.1.is.mirror_of_fractured_tomorrows|trinket.1.is.ashes_of_the_embersoul)))" );
  precombat->add_action( "grimoire_of_sacrifice,if=talent.grimoire_of_sacrifice.enabled" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "seed_of_corruption,if=spell_targets.seed_of_corruption_aoe>2|talent.sow_the_seeds&spell_targets.seed_of_corruption_aoe>1" );
  precombat->add_action( "haunt" );
  precombat->add_action( "unstable_affliction,if=!talent.soul_swap" );
  precombat->add_action( "shadow_bolt" );

  default_->add_action( "call_action_list,name=variables" );
  default_->add_action( "call_action_list,name=cleave,if=active_enemies!=1&active_enemies<3|variable.cleave_apl" );
  default_->add_action( "call_action_list,name=aoe,if=active_enemies>2" );
  default_->add_action( "call_action_list,name=ogcd" );
  default_->add_action( "call_action_list,name=items" );
  default_->add_action( "malefic_rapture,if=talent.dread_touch&debuff.dread_touch.remains<2&(dot.agony.remains>gcd.max&dot.corruption.ticking&(!talent.siphon_life|dot.siphon_life.ticking)&dot.unstable_affliction.ticking)&(!talent.phantom_singularity|!cooldown.phantom_singularity.ready)&(!talent.vile_taint|!cooldown.vile_taint.ready)&(!talent.soul_rot|!cooldown.soul_rot.ready)" );
  default_->add_action( "malefic_rapture,if=fight_remains<4" );
  default_->add_action( "vile_taint,if=!talent.soul_rot|(variable.min_agony<1.5|cooldown.soul_rot.remains<=execute_time+gcd.max)|talent.souleaters_gluttony.rank<1&cooldown.soul_rot.remains>=12" );
  default_->add_action( "phantom_singularity,if=(cooldown.soul_rot.remains<=execute_time|talent.souleaters_gluttony.rank<1&(!talent.soul_rot|cooldown.soul_rot.remains<=execute_time|cooldown.soul_rot.remains>=25))&dot.agony.ticking" );
  default_->add_action( "soul_rot,if=(variable.vt_up&(variable.ps_up|talent.souleaters_gluttony.rank!=1))&dot.agony.ticking" );
  default_->add_action( "agony,if=(remains<cooldown.vile_taint.remains+action.vile_taint.cast_time|!talent.vile_taint)&remains<5&fight_remains>5" );
  default_->add_action( "unstable_affliction,if=remains<5&fight_remains>3" );
  default_->add_action( "haunt,if=debuff.haunt.remains<5" );
  default_->add_action( "corruption,if=refreshable&fight_remains>5" );
  default_->add_action( "siphon_life,if=refreshable&fight_remains>5" );
  default_->add_action( "summon_darkglare,if=(!talent.shadow_embrace|debuff.shadow_embrace.stack=3)&variable.ps_up&variable.vt_up&variable.sr_up|cooldown.invoke_power_infusion_0.duration>0&cooldown.invoke_power_infusion_0.up&!talent.soul_rot" );
  default_->add_action( "drain_soul,interrupt=1,if=talent.shadow_embrace&(debuff.shadow_embrace.stack<3|debuff.shadow_embrace.remains<3)" );
  default_->add_action( "shadow_bolt,if=talent.shadow_embrace&(debuff.shadow_embrace.stack<3|debuff.shadow_embrace.remains<3)" );
  default_->add_action( "malefic_rapture,if=soul_shard>4|(talent.tormented_crescendo&buff.tormented_crescendo.stack=1&soul_shard>3)" );
  default_->add_action( "malefic_rapture,if=talent.tormented_crescendo&buff.tormented_crescendo.react&!debuff.dread_touch.react" );
  default_->add_action( "malefic_rapture,if=talent.tormented_crescendo&buff.tormented_crescendo.stack=2" );
  default_->add_action( "malefic_rapture,if=variable.cd_dots_up|variable.vt_ps_up&soul_shard>1" );
  default_->add_action( "malefic_rapture,if=talent.tormented_crescendo&talent.nightfall&buff.tormented_crescendo.react&buff.nightfall.react" );
  default_->add_action( "drain_life,if=buff.inevitable_demise.stack>48|buff.inevitable_demise.stack>20&fight_remains<4" );
  default_->add_action( "drain_soul,if=buff.nightfall.react" );
  default_->add_action( "shadow_bolt,if=buff.nightfall.react" );
  default_->add_action( "drain_soul,interrupt=1" );
  default_->add_action( "shadow_bolt" );

  aoe->add_action( "call_action_list,name=ogcd" );
  aoe->add_action( "call_action_list,name=items" );
  aoe->add_action( "cycling_variable,name=min_agony,op=min,value=dot.agony.remains+(99*!dot.agony.ticking)" );
  aoe->add_action( "cycling_variable,name=min_vt,op=min,default=10,value=dot.vile_taint.remains+(99*!dot.vile_taint.ticking)" );
  aoe->add_action( "cycling_variable,name=min_ps,op=min,default=16,value=dot.phantom_singularity.remains+(99*!dot.phantom_singularity.ticking)" );
  aoe->add_action( "variable,name=min_ps1,op=set,value=(variable.min_vt*talent.vile_taint<?variable.min_ps*talent.phantom_singularity)" );
  aoe->add_action( "haunt,if=debuff.haunt.remains<3" );
  aoe->add_action( "vile_taint,if=(talent.souleaters_gluttony.rank=2&(variable.min_agony<1.5|cooldown.soul_rot.remains<=execute_time))|((talent.souleaters_gluttony.rank=1&cooldown.soul_rot.remains<=execute_time))|(talent.souleaters_gluttony.rank=0&(cooldown.soul_rot.remains<=execute_time|cooldown.vile_taint.remains>25))" );
  aoe->add_action( "phantom_singularity,if=(cooldown.soul_rot.remains<=execute_time|talent.souleaters_gluttony.rank<1&(!talent.soul_rot|cooldown.soul_rot.remains<=execute_time|cooldown.soul_rot.remains>=25))&dot.agony.ticking" );
  aoe->add_action( "unstable_affliction,if=remains<5" );
  aoe->add_action( "agony,target_if=min:remains,if=active_dot.agony<8&(remains<cooldown.vile_taint.remains+action.vile_taint.cast_time|!talent.vile_taint)&gcd.max+action.soul_rot.cast_time+gcd.max<(variable.min_vt*talent.vile_taint<?variable.min_ps*talent.phantom_singularity)&remains<5" );
  aoe->add_action( "siphon_life,target_if=remains<5,if=active_dot.siphon_life<6&cooldown.summon_darkglare.up&time<20&gcd.max+action.soul_rot.cast_time+gcd.max<(variable.min_vt*talent.vile_taint<?variable.min_ps*talent.phantom_singularity)&dot.agony.ticking" );
  aoe->add_action( "soul_rot,if=variable.vt_up&(variable.ps_up|talent.souleaters_gluttony.rank!=1)&dot.agony.ticking" );
  aoe->add_action( "seed_of_corruption,if=dot.corruption.remains<5&!(action.seed_of_corruption.in_flight|dot.seed_of_corruption.remains>0)" );
  aoe->add_action( "corruption,target_if=min:remains,if=remains<5&!talent.seed_of_corruption" );
  aoe->add_action( "summon_darkglare,if=variable.ps_up&variable.vt_up&variable.sr_up|cooldown.invoke_power_infusion_0.duration>0&cooldown.invoke_power_infusion_0.up&!talent.soul_rot" );
  aoe->add_action( "drain_life,target_if=min:dot.soul_rot.remains,if=buff.inevitable_demise.stack>30&buff.soul_rot.up&buff.soul_rot.remains<=gcd.max&active_enemies>3" );
  aoe->add_action( "malefic_rapture,if=buff.umbrafire_kindling.up&(((active_enemies<6|time<30)&pet.darkglare.active)|!talent.doom_blossom)" );
  aoe->add_action( "seed_of_corruption,if=talent.sow_the_seeds" );
  aoe->add_action( "malefic_rapture,if=((cooldown.summon_darkglare.remains>15|soul_shard>3)&!talent.sow_the_seeds)|buff.tormented_crescendo.up" );
  aoe->add_action( "drain_life,target_if=min:dot.soul_rot.remains,if=(buff.soul_rot.up|!talent.soul_rot)&buff.inevitable_demise.stack>30" );
  aoe->add_action( "drain_soul,cycle_targets=1,if=buff.nightfall.react&talent.shadow_embrace&(debuff.shadow_embrace.stack<3|debuff.shadow_embrace.remains<3)" );
  aoe->add_action( "summon_soulkeeper,if=buff.tormented_soul.stack=10|buff.tormented_soul.stack>3&fight_remains<10" );
  aoe->add_action( "siphon_life,target_if=remains<5,if=active_dot.siphon_life<5&(active_enemies<8|!talent.doom_blossom)" );
  aoe->add_action( "drain_soul,cycle_targets=1,interrupt_global=1,if=(talent.shadow_embrace&(debuff.shadow_embrace.stack<3|debuff.shadow_embrace.remains<3))|!talent.shadow_embrace" );
  aoe->add_action( "shadow_bolt" );

  cleave->add_action( "call_action_list,name=ogcd" );
  cleave->add_action( "call_action_list,name=items" );
  cleave->add_action( "malefic_rapture,if=talent.dread_touch&debuff.dread_touch.remains<2&(dot.agony.remains>gcd.max&dot.corruption.ticking&(!talent.siphon_life|dot.siphon_life.ticking)&dot.unstable_affliction.ticking)&(!talent.phantom_singularity|!cooldown.phantom_singularity.ready)&(!talent.vile_taint|!cooldown.vile_taint.ready)&(!talent.soul_rot|!cooldown.soul_rot.ready)|soul_shard>4" );
  cleave->add_action( "vile_taint,if=!talent.soul_rot|(variable.min_agony<1.5|cooldown.soul_rot.remains<=execute_time+gcd.max)|talent.souleaters_gluttony.rank<1&cooldown.soul_rot.remains>=12" );
  cleave->add_action( "phantom_singularity,if=(cooldown.soul_rot.remains<=execute_time|talent.souleaters_gluttony.rank<1&(!talent.soul_rot|cooldown.soul_rot.remains<=execute_time|cooldown.soul_rot.remains>=25))&active_dot.agony=2" );
  cleave->add_action( "soul_rot,if=(variable.vt_up&(variable.ps_up|talent.souleaters_gluttony.rank!=1))&active_dot.agony=2" );
  cleave->add_action( "agony,target_if=min:remains,if=(remains<cooldown.vile_taint.remains+action.vile_taint.cast_time|!talent.vile_taint)&remains<5&fight_remains>5" );
  cleave->add_action( "unstable_affliction,if=remains<5&fight_remains>3" );
  cleave->add_action( "seed_of_corruption,if=!talent.absolute_corruption&dot.corruption.remains<5&talent.sow_the_seeds&can_seed" );
  cleave->add_action( "haunt,if=debuff.haunt.remains<3" );
  cleave->add_action( "corruption,target_if=min:remains,if=remains<5&!(action.seed_of_corruption.in_flight|dot.seed_of_corruption.remains>0)&fight_remains>5" );
  cleave->add_action( "siphon_life,target_if=min:remains,if=refreshable&fight_remains>5" );
  cleave->add_action( "summon_darkglare,if=(!talent.shadow_embrace|debuff.shadow_embrace.stack=3)&variable.ps_up&variable.vt_up&variable.sr_up|cooldown.invoke_power_infusion_0.duration>0&cooldown.invoke_power_infusion_0.up&!talent.soul_rot" );
  cleave->add_action( "malefic_rapture,if=talent.tormented_crescendo&buff.tormented_crescendo.stack=1&soul_shard>3" );
  cleave->add_action( "drain_soul,interrupt=1,if=talent.shadow_embrace&(debuff.shadow_embrace.stack<3|debuff.shadow_embrace.remains<3)" );
  cleave->add_action( "drain_soul,target_if=min:debuff.shadow_embrace.remains,if=buff.nightfall.react&(talent.shadow_embrace&(debuff.shadow_embrace.stack<3|debuff.shadow_embrace.remains<3)|!talent.shadow_embrace)" );
  cleave->add_action( "shadow_bolt,target_if=min:debuff.shadow_embrace.remains,if=buff.nightfall.react&(talent.shadow_embrace&(debuff.shadow_embrace.stack<3|debuff.shadow_embrace.remains<3)|!talent.shadow_embrace)" );
  cleave->add_action( "malefic_rapture,if=!talent.dread_touch&buff.tormented_crescendo.up" );
  cleave->add_action( "malefic_rapture,if=variable.cd_dots_up|variable.vt_ps_up" );
  cleave->add_action( "malefic_rapture,if=soul_shard>3" );
  cleave->add_action( "drain_life,if=buff.inevitable_demise.stack>48|buff.inevitable_demise.stack>20&fight_remains<4" );
  cleave->add_action( "drain_life,if=buff.soul_rot.up&buff.inevitable_demise.stack>30" );
  cleave->add_action( "agony,target_if=refreshable" );
  cleave->add_action( "corruption,target_if=refreshable" );
  cleave->add_action( "malefic_rapture,if=soul_shard>1" );
  cleave->add_action( "drain_soul,interrupt_global=1" );
  cleave->add_action( "shadow_bolt" );

  items->add_action( "use_item,use_off_gcd=1,name=belorrelos_the_suncaller,if=((time>20&cooldown.summon_darkglare.remains>20)|(trinket.1.is.belorrelos_the_suncaller&(trinket.2.cooldown.remains|!variable.trinket_2_buffs|trinket.1.is.time_thiefs_gambit))|(trinket.2.is.belorrelos_the_suncaller&(trinket.1.cooldown.remains|!variable.trinket_1_buffs|trinket.2.is.time_thiefs_gambit)))&(!raid_event.adds.exists|raid_event.adds.up|spell_targets.belorrelos_the_suncaller>=5)|fight_remains<20" );
  items->add_action( "use_item,slot=trinket1,if=(variable.cds_active)&(variable.trinket_priority=1|variable.trinket_2_exclude|!trinket.2.has_cooldown|(trinket.2.cooldown.remains|variable.trinket_priority=2&cooldown.summon_darkglare.remains>20&!pet.darkglare.active&trinket.2.cooldown.remains<cooldown.summon_darkglare.remains))&variable.trinket_1_buffs&!variable.trinket_1_manual|(variable.trinket_1_buff_duration+1>=fight_remains)", "We want to use trinkets with Darkglare. The trinket with highest estimated value, will be used first." );
  items->add_action( "use_item,slot=trinket2,if=(variable.cds_active)&(variable.trinket_priority=2|variable.trinket_1_exclude|!trinket.1.has_cooldown|(trinket.1.cooldown.remains|variable.trinket_priority=1&cooldown.summon_darkglare.remains>20&!pet.darkglare.active&trinket.1.cooldown.remains<cooldown.summon_darkglare.remains))&variable.trinket_2_buffs&!variable.trinket_2_manual|(variable.trinket_2_buff_duration+1>=fight_remains)" );
  items->add_action( "use_item,name=time_thiefs_gambit,if=variable.cds_active|fight_remains<15|((trinket.1.cooldown.duration<cooldown.summon_darkglare.remains_expected+5)&active_enemies=1)|(active_enemies>1&havoc_active)" );
  items->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=!variable.trinket_1_buffs&!variable.trinket_1_manual&(!variable.trinket_1_buffs&(trinket.2.cooldown.remains|!variable.trinket_2_buffs)|talent.summon_darkglare&cooldown.summon_darkglare.remains_expected>20|!talent.summon_darkglare)", "If only one on use trinket provied a buff, use the other on cooldown, Or if neither trinket provied a buff, use both on cooldown." );
  items->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=!variable.trinket_2_buffs&!variable.trinket_2_manual&(!variable.trinket_2_buffs&(trinket.1.cooldown.remains|!variable.trinket_1_buffs)|talent.summon_darkglare&cooldown.summon_darkglare.remains_expected>20|!talent.summon_darkglare)" );
  items->add_action( "use_item,use_off_gcd=1,slot=main_hand" );

  ogcd->add_action( "potion,if=variable.cds_active" );
  ogcd->add_action( "berserking,if=variable.cds_active" );
  ogcd->add_action( "blood_fury,if=variable.cds_active" );
  ogcd->add_action( "invoke_external_buff,name=power_infusion,if=variable.cds_active&(trinket.1.is.nymues_unraveling_spindle&trinket.1.cooldown.remains|trinket.2.is.nymues_unraveling_spindle&trinket.2.cooldown.remains|!equipped.nymues_unraveling_spindle)", "Uses Power Infusion together with Cooldown windows like Summon Darkglare, Soul Rot, Phantom Singularity or Vile Taint" );
  ogcd->add_action( "fireblood,if=variable.cds_active" );
  ogcd->add_action( "ancestral_call,if=variable.cds_active" );

  variables->add_action( "variable,name=ps_up,op=set,value=dot.phantom_singularity.ticking|!talent.phantom_singularity" );
  variables->add_action( "variable,name=vt_up,op=set,value=dot.vile_taint_dot.ticking|!talent.vile_taint" );
  variables->add_action( "variable,name=vt_ps_up,op=set,value=dot.vile_taint_dot.ticking|dot.phantom_singularity.ticking|(!talent.vile_taint&!talent.phantom_singularity)" );
  variables->add_action( "variable,name=sr_up,op=set,value=dot.soul_rot.ticking|!talent.soul_rot" );
  variables->add_action( "variable,name=cd_dots_up,op=set,value=variable.ps_up&variable.vt_up&variable.sr_up" );
  variables->add_action( "variable,name=has_cds,op=set,value=talent.phantom_singularity|talent.vile_taint|talent.soul_rot|talent.summon_darkglare" );
  variables->add_action( "variable,name=cds_active,op=set,value=!variable.has_cds|(variable.cd_dots_up&(cooldown.summon_darkglare.remains>20|!talent.summon_darkglare))" );
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
  action_priority_list_t* racials = p->get_action_priority_list( "racials" );
  action_priority_list_t* tyrant = p->get_action_priority_list( "tyrant" );
  action_priority_list_t* variables = p->get_action_priority_list( "variables" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "summon_pet" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=tyrant_prep_start,op=set,value=12" );
  precombat->add_action( "variable,name=next_tyrant,op=set,value=14+talent.grimoire_felguard+talent.summon_vilefiend" );
  precombat->add_action( "variable,name=shadow_timings,default=0,op=reset" );
  precombat->add_action( "variable,name=tyrant_timings,value=0" );
  precombat->add_action( "variable,name=shadow_timings,op=set,value=0,if=cooldown.invoke_power_infusion_0.duration!=120" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_use_buff" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_use_buff" );
  precombat->add_action( "variable,name=trinket_1_exclude,value=trinket.1.is.ruby_whelp_shell|trinket.1.is.whispering_incarnate_icon|trinket.1.is.timethiefs_gambit" );
  precombat->add_action( "variable,name=trinket_2_exclude,value=trinket.2.is.ruby_whelp_shell|trinket.2.is.whispering_incarnate_icon|trinket.2.is.timethiefs_gambit" );
  precombat->add_action( "variable,name=trinket_1_manual,value=trinket.1.is.nymues_unraveling_spindle" );
  precombat->add_action( "variable,name=trinket_2_manual,value=trinket.2.is.nymues_unraveling_spindle" );
  precombat->add_action( "variable,name=trinket_1_buff_duration,value=trinket.1.proc.any_dps.duration+(trinket.1.is.mirror_of_fractured_tomorrows*20)+(trinket.1.is.nymues_unraveling_spindle*2)" );
  precombat->add_action( "variable,name=trinket_2_buff_duration,value=trinket.2.proc.any_dps.duration+(trinket.2.is.mirror_of_fractured_tomorrows*20)+(trinket.2.is.nymues_unraveling_spindle*2)" );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_1_buffs&(trinket.1.cooldown.duration%%cooldown.summon_demonic_tyrant.duration=0|cooldown.summon_demonic_tyrant.duration%%trinket.1.cooldown.duration=0)" );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_2_buffs&(trinket.2.cooldown.duration%%cooldown.summon_demonic_tyrant.duration=0|cooldown.summon_demonic_tyrant.duration%%trinket.2.cooldown.duration=0)" );
  precombat->add_action( "variable,name=damage_trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&!variable.trinket_2_buffs&trinket.2.ilvl>trinket.1.ilvl" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs|variable.trinket_2_buffs&((trinket.2.cooldown.duration%variable.trinket_2_buff_duration)*(1.5+trinket.2.has_buff.intellect)*(variable.trinket_2_sync)*(1-0.5*trinket.2.is.mirror_of_fractured_tomorrows))>(((trinket.1.cooldown.duration%variable.trinket_1_buff_duration)*(1.5+trinket.1.has_buff.intellect)*(variable.trinket_1_sync)*(1-0.5*trinket.1.is.mirror_of_fractured_tomorrows))*(1+((trinket.1.ilvl-trinket.2.ilvl)%100)))" );
  precombat->add_action( "power_siphon" );
  precombat->add_action( "demonbolt,if=!buff.power_siphon.up" );
  precombat->add_action( "shadow_bolt" );

  default_->add_action( "call_action_list,name=variables" );
  default_->add_action( "call_action_list,name=racials,if=pet.demonic_tyrant.active|fight_remains<22,use_off_gcd=1" );
  default_->add_action( "call_action_list,name=items,use_off_gcd=1" );
  default_->add_action( "invoke_external_buff,name=power_infusion,if=(buff.nether_portal.up&buff.nether_portal.remains<3&talent.nether_portal)|fight_remains<20|pet.demonic_tyrant.active&fight_remains<100|fight_remains<25|(pet.demonic_tyrant.active|!talent.summon_demonic_tyrant&buff.dreadstalkers.up)" );
  default_->add_action( "call_action_list,name=fight_end,if=fight_remains<30" );
  default_->add_action( "hand_of_guldan,if=time<0.5&(fight_remains%%95>40|fight_remains%%95<15)&(talent.reign_of_tyranny|active_enemies>2)" );
  default_->add_action( "call_action_list,name=tyrant,if=cooldown.summon_demonic_tyrant.remains<15&cooldown.summon_vilefiend.remains<gcd.max*5&cooldown.call_dreadstalkers.remains<gcd.max*5&(cooldown.grimoire_felguard.remains<10|!set_bonus.tier30_2pc)&(variable.tyrant_cd<15|fight_remains<40|buff.power_infusion.up)&(!raid_event.adds.exists|raid_event.adds.exists&raid_event.adds.remains>20)|talent.summon_vilefiend.enabled&cooldown.summon_demonic_tyrant.remains<15&cooldown.summon_vilefiend.remains<gcd.max*5&cooldown.call_dreadstalkers.remains<gcd.max*5&(cooldown.grimoire_felguard.remains<10|!set_bonus.tier30_2pc)&(variable.tyrant_cd<15|fight_remains<40|buff.power_infusion.up)&(!raid_event.adds.exists|raid_event.adds.exists&raid_event.adds.remains>20)" );
  default_->add_action( "call_action_list,name=tyrant,if=cooldown.summon_demonic_tyrant.remains<15&(buff.vilefiend.up|!talent.summon_vilefiend&(buff.grimoire_felguard.up|cooldown.grimoire_felguard.up|!set_bonus.tier30_2pc))&(variable.tyrant_cd<15|buff.grimoire_felguard.up|fight_remains<40|buff.power_infusion.up)&(!raid_event.adds.exists|raid_event.adds.exists&raid_event.adds.remains>20)" );
  default_->add_action( "summon_demonic_tyrant,if=buff.vilefiend.up|buff.grimoire_felguard.up|cooldown.grimoire_felguard.remains>90" );
  default_->add_action( "summon_vilefiend,if=cooldown.summon_demonic_tyrant.remains>45" );
  default_->add_action( "demonbolt,target_if=(!debuff.doom_brand.up|action.hand_of_guldan.in_flight&debuff.doom_brand.remains<=3),if=buff.demonic_core.up&(((!talent.soul_strike|cooldown.soul_strike.remains>gcd.max*2)&soul_shard<4)|soul_shard<(4-(active_enemies>2)))&!prev_gcd.1.demonbolt&set_bonus.tier31_2pc" );
  default_->add_action( "power_siphon,if=!buff.demonic_core.up&(!debuff.doom_brand.up|(!action.hand_of_guldan.in_flight&debuff.doom_brand.remains<gcd.max+action.demonbolt.travel_time)|(action.hand_of_guldan.in_flight&debuff.doom_brand.remains<gcd.max+action.demonbolt.travel_time+3))&set_bonus.tier31_2pc" );
  default_->add_action( "demonic_strength,if=buff.nether_portal.remains<gcd.max&!(raid_event.adds.in<45-raid_event.add.duration)" );
  default_->add_action( "bilescourge_bombers" );
  default_->add_action( "guillotine,if=buff.nether_portal.remains<gcd.max&(cooldown.demonic_strength.remains|!talent.demonic_strength)&(!raid_event.adds.exists|raid_event.adds.exists&raid_event.adds.remains>6)" );
  default_->add_action( "call_dreadstalkers,if=cooldown.summon_demonic_tyrant.remains>25|variable.tyrant_cd>25|buff.nether_portal.up" );
  default_->add_action( "implosion,if=two_cast_imps>0&variable.impl&!prev_gcd.1.implosion&!raid_event.adds.exists|two_cast_imps>0&variable.impl&!prev_gcd.1.implosion&raid_event.adds.exists&(active_enemies>3|active_enemies<=3&last_cast_imps>0)", "If Tyrant is not up, it Implodes naturally. On 3-4t it waits till <6s left on Tyrant. On 5t+ it waits till <8s left on Tyrant" );
  default_->add_action( "summon_soulkeeper,if=buff.tormented_soul.stack=10&active_enemies>1" );
  default_->add_action( "hand_of_guldan,if=((soul_shard>2&cooldown.call_dreadstalkers.remains>gcd.max*4&cooldown.summon_demonic_tyrant.remains>17)|soul_shard=5|soul_shard=4&talent.soul_strike&cooldown.soul_strike.remains<gcd.max*2)&(active_enemies=1&talent.grand_warlocks_design)", "Uses HoG as long as you will have 2 shards ready for Dogs or are capped on Shards (1T and Wilf only)" );
  default_->add_action( "hand_of_guldan,if=soul_shard>2&!(active_enemies=1&talent.grand_warlocks_design)" );
  default_->add_action( "demonbolt,target_if=(!debuff.doom_brand.up)|active_enemies<4,if=buff.demonic_core.stack>1&((soul_shard<4&!talent.soul_strike|cooldown.soul_strike.remains>gcd.max*2)|soul_shard<3)&!variable.pool_cores_for_tyrant", "Demonbolt if we have more than one core" );
  default_->add_action( "demonbolt,target_if=(!debuff.doom_brand.up)|active_enemies<4,if=set_bonus.tier31_2pc&(debuff.doom_brand.remains>10&buff.demonic_core.up&soul_shard<4)&!variable.pool_cores_for_tyrant", "Demonbolt if 2pc is safe" );
  default_->add_action( "demonbolt,if=fight_remains<buff.demonic_core.stack*gcd.max" );
  default_->add_action( "demonbolt,target_if=(!debuff.doom_brand.up)|active_enemies<4,if=buff.demonic_core.up&(cooldown.power_siphon.remains<4)&(soul_shard<4)&!variable.pool_cores_for_tyrant", "Aggressive Core usage if PS is coming off CD" );
  default_->add_action( "power_siphon,if=!buff.demonic_core.up" );
  default_->add_action( "summon_vilefiend,if=fight_remains<cooldown.summon_demonic_tyrant.remains+5" );
  default_->add_action( "doom,target_if=refreshable" );
  default_->add_action( "shadow_bolt" );

  fight_end->add_action( "grimoire_felguard,if=fight_remains<20" );
  fight_end->add_action( "call_dreadstalkers,if=fight_remains<20" );
  fight_end->add_action( "summon_vilefiend,if=fight_remains<20" );
  fight_end->add_action( "nether_portal,if=fight_remains<30" );
  fight_end->add_action( "summon_demonic_tyrant,if=fight_remains<20" );
  fight_end->add_action( "demonic_strength,if=fight_remains<10" );
  fight_end->add_action( "power_siphon,if=buff.demonic_core.stack<3&fight_remains<20" );
  fight_end->add_action( "implosion,if=fight_remains<2*gcd.max" );

  items->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=variable.trinket_1_buffs&!variable.trinket_1_manual&(!pet.demonic_tyrant.active&trinket.1.cast_time>0|!trinket.1.cast_time>0)&(pet.demonic_tyrant.active|!talent.summon_demonic_tyrant|variable.trinket_priority=2&cooldown.summon_demonic_tyrant.remains>20&!pet.demonic_tyrant.active&trinket.2.cooldown.remains<cooldown.summon_demonic_tyrant.remains+5)&(variable.trinket_2_exclude|!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1&!variable.trinket_2_manual)|variable.trinket_1_buff_duration>=fight_remains" );
  items->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=variable.trinket_2_buffs&!variable.trinket_2_manual&(!pet.demonic_tyrant.active&trinket.2.cast_time>0|!trinket.2.cast_time>0)&(pet.demonic_tyrant.active|!talent.summon_demonic_tyrant|variable.trinket_priority=1&cooldown.summon_demonic_tyrant.remains>20&!pet.demonic_tyrant.active&trinket.1.cooldown.remains<cooldown.summon_demonic_tyrant.remains+5)&(variable.trinket_1_exclude|!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2&!variable.trinket_1_manual)|variable.trinket_2_buff_duration>=fight_remains" );
  items->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=!variable.trinket_1_buffs&!variable.trinket_1_manual&((variable.damage_trinket_priority=1|trinket.2.cooldown.remains)&(trinket.1.cast_time>0&!pet.demonic_tyrant.active|!trinket.1.cast_time>0)|(time<20&variable.trinket_2_buffs)|cooldown.summon_demonic_tyrant.remains_expected>20)" );
  items->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=!variable.trinket_2_buffs&!variable.trinket_2_manual&((variable.damage_trinket_priority=2|trinket.1.cooldown.remains)&(trinket.2.cast_time>0&!pet.demonic_tyrant.active|!trinket.2.cast_time>0)|(time<20&variable.trinket_1_buffs)|cooldown.summon_demonic_tyrant.remains_expected>20)" );
  items->add_action( "use_item,use_off_gcd=1,slot=main_hand" );
  items->add_action( "use_item,name=nymues_unraveling_spindle,if=trinket.1.is.nymues_unraveling_spindle&((pet.demonic_tyrant.active&(!cooldown.demonic_strength.ready|!talent.demonic_strength)&!variable.trinket_2_buffs)|(variable.trinket_2_buffs))|trinket.2.is.nymues_unraveling_spindle&((pet.demonic_tyrant.active&(!cooldown.demonic_strength.ready|!talent.demonic_strength)&!variable.trinket_1_buffs)|(variable.trinket_1_buffs))|fight_remains<22" );
  items->add_action( "use_item,name=mirror_of_fractured_tomorrows,if=trinket.1.is.mirror_of_fractured_tomorrows&variable.trinket_priority=2|trinket.2.is.mirror_of_fractured_tomorrows&variable.trinket_priority=1" );
  items->add_action( "use_item,name=timethiefs_gambit,if=pet.demonic_tyrant.active" );
  items->add_action( "use_item,slot=trinket1,if=!variable.trinket_1_buffs&(variable.damage_trinket_priority=1|trinket.2.cooldown.remains)" );
  items->add_action( "use_item,slot=trinket2,if=!variable.trinket_2_buffs&(variable.damage_trinket_priority=2|trinket.1.cooldown.remains)" );

  racials->add_action( "berserking,use_off_gcd=1" );
  racials->add_action( "blood_fury" );
  racials->add_action( "fireblood" );
  racials->add_action( "ancestral_call" );

  tyrant->add_action( "invoke_external_buff,name=power_infusion,if=variable.pet_expire>0&variable.pet_expire<action.summon_demonic_tyrant.execute_time+(buff.demonic_core.down*action.shadow_bolt.execute_time+buff.demonic_core.up*gcd.max)+gcd.max" );
  tyrant->add_action( "variable,name=tyrant_timings,op=set,value=120+time,if=variable.pet_expire>0&variable.pet_expire<action.summon_demonic_tyrant.execute_time+(buff.demonic_core.down*action.shadow_bolt.execute_time+buff.demonic_core.up*gcd.max)+gcd.max&variable.tyrant_timings<=0" );
  tyrant->add_action( "variable,name=dummyvar,value=variable.pet_expire<action.summon_demonic_tyrant.execute_time+(buff.demonic_core.down*action.shadow_bolt.execute_time+buff.demonic_core.up*gcd.max)+gcd.max" );
  tyrant->add_action( "hand_of_guldan,if=variable.pet_expire>gcd.max+action.summon_demonic_tyrant.cast_time&variable.pet_expire<gcd.max*4" );
  tyrant->add_action( "call_action_list,name=items,if=variable.pet_expire>0&variable.pet_expire<action.summon_demonic_tyrant.execute_time+(buff.demonic_core.down*action.shadow_bolt.execute_time+buff.demonic_core.up*gcd.max)+gcd.max,use_off_gcd=1" );
  tyrant->add_action( "call_action_list,name=racials,if=variable.pet_expire>0&variable.pet_expire<action.summon_demonic_tyrant.execute_time+(buff.demonic_core.down*action.shadow_bolt.execute_time+buff.demonic_core.up*gcd.max)+gcd.max,use_off_gcd=1" );
  tyrant->add_action( "potion,if=variable.pet_expire>0&variable.pet_expire<action.summon_demonic_tyrant.execute_time+(buff.demonic_core.down*action.shadow_bolt.execute_time+buff.demonic_core.up*gcd.max)+gcd.max,use_off_gcd=1" );
  tyrant->add_action( "summon_demonic_tyrant,if=variable.pet_expire>0&variable.pet_expire<action.summon_demonic_tyrant.execute_time+(buff.demonic_core.down*action.shadow_bolt.execute_time+buff.demonic_core.up*gcd.max)+gcd.max" );
  tyrant->add_action( "implosion,if=pet_count>2&(buff.dreadstalkers.down&buff.grimoire_felguard.down&buff.vilefiend.down)&(active_enemies>3|active_enemies>2&talent.grand_warlocks_design)&!prev_gcd.1.implosion" );
  tyrant->add_action( "shadow_bolt,if=prev_gcd.1.grimoire_felguard&time>30&buff.nether_portal.down&buff.demonic_core.down" );
  tyrant->add_action( "power_siphon,if=buff.demonic_core.stack<4&(!buff.vilefiend.up|!talent.summon_vilefiend&(!buff.dreadstalkers.up))&(buff.nether_portal.down)" );
  tyrant->add_action( "shadow_bolt,if=buff.vilefiend.down&buff.nether_portal.down&buff.dreadstalkers.down&soul_shard<5-buff.demonic_core.stack" );
  tyrant->add_action( "nether_portal,if=soul_shard=5" );
  tyrant->add_action( "summon_vilefiend,if=(soul_shard=5|buff.nether_portal.up)&cooldown.summon_demonic_tyrant.remains<13&variable.np" );
  tyrant->add_action( "call_dreadstalkers,if=(buff.vilefiend.up|!talent.summon_vilefiend&(!talent.nether_portal|buff.nether_portal.up|cooldown.nether_portal.remains>30)&(buff.nether_portal.up|buff.grimoire_felguard.up|soul_shard=5))&cooldown.summon_demonic_tyrant.remains<11&variable.np" );
  tyrant->add_action( "grimoire_felguard,if=buff.vilefiend.up|!talent.summon_vilefiend&(!talent.nether_portal|buff.nether_portal.up|cooldown.nether_portal.remains>30)&(buff.nether_portal.up|buff.dreadstalkers.up|soul_shard=5)&variable.np" );
  tyrant->add_action( "hand_of_guldan,if=soul_shard>2&(buff.vilefiend.up|!talent.summon_vilefiend&buff.dreadstalkers.up)&(soul_shard>2|buff.vilefiend.remains<gcd.max*2+2%spell_haste)|(!buff.dreadstalkers.up&soul_shard=5)" );
  tyrant->add_action( "demonbolt,cycle_targets=1,if=soul_shard<4&(buff.demonic_core.stack>1)&(buff.vilefiend.up|!talent.summon_vilefiend&buff.dreadstalkers.up)" );
  tyrant->add_action( "power_siphon,if=buff.demonic_core.stack<3&variable.pet_expire>action.summon_demonic_tyrant.execute_time+gcd.max*3|variable.pet_expire=0" );
  tyrant->add_action( "shadow_bolt" );

  variables->add_action( "variable,name=tyrant_timings,op=set,value=120+time,if=((buff.nether_portal.up&buff.nether_portal.remains<3&talent.nether_portal)|fight_remains<20|pet.demonic_tyrant.active&fight_remains<100|fight_remains<25|(pet.demonic_tyrant.active|!talent.summon_demonic_tyrant&buff.dreadstalkers.up))&variable.tyrant_sync<=0" );
  variables->add_action( "variable,name=tyrant_sync,value=(variable.tyrant_timings-time)" );
  variables->add_action( "variable,name=tyrant_cd,op=setif,value=variable.tyrant_sync,value_else=cooldown.summon_demonic_tyrant.remains,condition=((((fight_remains+time)%%120<=85&(fight_remains+time)%%120>=25)|time>=210))&variable.tyrant_sync>0&!talent.grand_warlocks_design" );
  variables->add_action( "variable,name=pet_expire,op=set,value=(buff.dreadstalkers.remains>?buff.vilefiend.remains)-gcd*0.5,if=buff.vilefiend.up&buff.dreadstalkers.up" );
  variables->add_action( "variable,name=pet_expire,op=set,value=(buff.dreadstalkers.remains>?buff.grimoire_felguard.remains)-gcd*0.5,if=!talent.summon_vilefiend&talent.grimoire_felguard&buff.dreadstalkers.up" );
  variables->add_action( "variable,name=pet_expire,op=set,value=(buff.dreadstalkers.remains)-gcd*0.5,if=!talent.summon_vilefiend&(!talent.grimoire_felguard|!set_bonus.tier30_2pc)&buff.dreadstalkers.up" );
  variables->add_action( "variable,name=pet_expire,op=set,value=0,if=!buff.vilefiend.up&talent.summon_vilefiend|!buff.dreadstalkers.up" );
  variables->add_action( "variable,name=np,op=set,value=(!talent.nether_portal|cooldown.nether_portal.remains>30|buff.nether_portal.up)" );
  variables->add_action( "variable,name=impl,op=set,value=buff.tyrant.down,if=active_enemies>1+(talent.sacrificed_souls.enabled)" );
  variables->add_action( "variable,name=impl,op=set,value=buff.tyrant.remains<6,if=active_enemies>2+(talent.sacrificed_souls.enabled)&active_enemies<5+(talent.sacrificed_souls.enabled)" );
  variables->add_action( "variable,name=impl,op=set,value=buff.tyrant.remains<8,if=active_enemies>4+(talent.sacrificed_souls.enabled)" );
  variables->add_action( "variable,name=pool_cores_for_tyrant,op=set,value=cooldown.summon_demonic_tyrant.remains<20&variable.tyrant_cd<20&(buff.demonic_core.stack<=2|!buff.demonic_core.up)&cooldown.summon_vilefiend.remains<gcd.max*5&cooldown.call_dreadstalkers.remains<gcd.max*5" );
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
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_use_buff" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_use_buff" );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_1_buffs&(trinket.1.cooldown.duration%%cooldown.summon_infernal.duration=0|cooldown.summon_infernal.duration%%trinket.1.cooldown.duration=0)" );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_2_buffs&(trinket.2.cooldown.duration%%cooldown.summon_infernal.duration=0|cooldown.summon_infernal.duration%%trinket.2.cooldown.duration=0)" );
  precombat->add_action( "variable,name=trinket_1_manual,value=trinket.1.is.belorrelos_the_suncaller|trinket.1.is.nymues_unraveling_spindle|trinket.1.is.timethiefs_gambit" );
  precombat->add_action( "variable,name=trinket_2_manual,value=trinket.2.is.belorrelos_the_suncaller|trinket.2.is.nymues_unraveling_spindle|trinket.2.is.timethiefs_gambit" );
  precombat->add_action( "variable,name=trinket_1_exclude,value=trinket.1.is.ruby_whelp_shell|trinket.1.is.whispering_incarnate_icon" );
  precombat->add_action( "variable,name=trinket_2_exclude,value=trinket.2.is.ruby_whelp_shell|trinket.2.is.whispering_incarnate_icon" );
  precombat->add_action( "variable,name=trinket_1_buff_duration,value=trinket.1.proc.any_dps.duration+(trinket.1.is.mirror_of_fractured_tomorrows*20)+(trinket.1.is.nymues_unraveling_spindle*2)" );
  precombat->add_action( "variable,name=trinket_2_buff_duration,value=trinket.2.proc.any_dps.duration+(trinket.2.is.mirror_of_fractured_tomorrows*20)+(trinket.2.is.nymues_unraveling_spindle*2)" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs|variable.trinket_2_buffs&((trinket.2.cooldown.duration%variable.trinket_2_buff_duration)*(1+0.5*trinket.2.has_buff.intellect)*(variable.trinket_2_sync)*(1-0.5*trinket.2.is.mirror_of_fractured_tomorrows))>((trinket.1.cooldown.duration%variable.trinket_1_buff_duration)*(1+0.5*trinket.1.has_buff.intellect)*(variable.trinket_1_sync)*(1-0.5*trinket.1.is.mirror_of_fractured_tomorrows))" );
  precombat->add_action( "grimoire_of_sacrifice,if=talent.grimoire_of_sacrifice.enabled" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "soul_fire" );
  precombat->add_action( "cataclysm,if=raid_event.adds.in>15" );
  precombat->add_action( "incinerate" );

  default_->add_action( "call_action_list,name=variables" );
  default_->add_action( "call_action_list,name=aoe,if=(active_enemies>=3-(talent.inferno&!talent.chaosbringer))&!(!talent.inferno&talent.chaosbringer&talent.chaos_incarnate&active_enemies<4)&!variable.cleave_apl" );
  default_->add_action( "call_action_list,name=cleave,if=active_enemies!=1|variable.cleave_apl" );
  default_->add_action( "call_action_list,name=ogcd" );
  default_->add_action( "call_action_list,name=items" );
  default_->add_action( "conflagrate,if=(talent.roaring_blaze&debuff.conflagrate.remains<1.5)&soul_shard>1.5|charges=max_charges" );
  default_->add_action( "dimensional_rift,if=soul_shard<4.7&(charges>2|fight_remains<cooldown.dimensional_rift.duration)" );
  default_->add_action( "cataclysm,if=raid_event.adds.in>15" );
  default_->add_action( "channel_demonfire,if=talent.raging_demonfire&(dot.immolate.remains-5*(action.chaos_bolt.in_flight&talent.internal_combustion))>cast_time&(debuff.conflagrate.remains>execute_time|!talent.roaring_blaze)" );
  default_->add_action( "soul_fire,if=soul_shard<=3.5&(debuff.conflagrate.remains>cast_time+travel_time|!talent.roaring_blaze&buff.backdraft.up)" );
  default_->add_action( "immolate,if=(((dot.immolate.remains-5*(action.chaos_bolt.in_flight&talent.internal_combustion))<dot.immolate.duration*0.3)|dot.immolate.remains<3|(dot.immolate.remains-action.chaos_bolt.execute_time)<5&talent.internal_combustion&action.chaos_bolt.usable)&(!talent.cataclysm|cooldown.cataclysm.remains>dot.immolate.remains)&(!talent.soul_fire|cooldown.soul_fire.remains+action.soul_fire.cast_time>(dot.immolate.remains-5*talent.internal_combustion))&target.time_to_die>8" );
  default_->add_action( "channel_demonfire,if=dot.immolate.remains>cast_time&set_bonus.tier30_4pc" );
  default_->add_action( "chaos_bolt,if=cooldown.summon_infernal.remains=0&soul_shard>4&talent.crashing_chaos" );
  default_->add_action( "summon_infernal" );
  default_->add_action( "chaos_bolt,if=pet.infernal.active|pet.blasphemy.active|soul_shard>=4" );
  default_->add_action( "channel_demonfire,if=talent.ruin.rank>1&!(talent.diabolic_embers&talent.avatar_of_destruction&(talent.burn_to_ashes|talent.chaos_incarnate))&dot.immolate.remains>cast_time" );
  default_->add_action( "chaos_bolt,if=buff.rain_of_chaos.remains>cast_time" );
  default_->add_action( "chaos_bolt,if=buff.backdraft.up" );
  default_->add_action( "channel_demonfire,if=!(talent.diabolic_embers&talent.avatar_of_destruction&(talent.burn_to_ashes|talent.chaos_incarnate))&dot.immolate.remains>cast_time" );
  default_->add_action( "dimensional_rift" );
  default_->add_action( "chaos_bolt,if=fight_remains<5&fight_remains>cast_time+travel_time" );
  default_->add_action( "conflagrate,if=charges>(max_charges-1)|fight_remains<gcd.max*charges" );
  default_->add_action( "incinerate" );

  aoe->add_action( "call_action_list,name=ogcd" );
  aoe->add_action( "call_action_list,name=items" );
  aoe->add_action( "call_action_list,name=havoc,if=havoc_active&havoc_remains>gcd.max&active_enemies<5+(talent.cry_havoc&!talent.inferno)&(!cooldown.summon_infernal.up|!talent.summon_infernal)" );
  aoe->add_action( "dimensional_rift,if=soul_shard<4.7&(charges>2|fight_remains<cooldown.dimensional_rift.duration)" );
  aoe->add_action( "rain_of_fire,if=pet.infernal.active|pet.blasphemy.active" );
  aoe->add_action( "rain_of_fire,if=fight_remains<12" );
  aoe->add_action( "rain_of_fire,if=soul_shard>=(4.5-0.1*active_dot.immolate)&time>5" );
  aoe->add_action( "chaos_bolt,if=soul_shard>3.5-(0.1*active_enemies)&!talent.rain_of_fire" );
  aoe->add_action( "cataclysm,if=raid_event.adds.in>15" );
  aoe->add_action( "havoc,target_if=min:((-target.time_to_die)<?-15)+dot.immolate.remains+99*(self.target=target),if=(!cooldown.summon_infernal.up|!talent.summon_infernal|(talent.inferno&active_enemies>4))&target.time_to_die>8" );
  aoe->add_action( "immolate,target_if=min:dot.immolate.remains+99*debuff.havoc.remains,if=dot.immolate.refreshable&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>dot.immolate.remains)&(!talent.raging_demonfire|cooldown.channel_demonfire.remains>remains|time<5)&active_dot.immolate<=4&target.time_to_die>18" );
  aoe->add_action( "channel_demonfire,if=dot.immolate.remains>cast_time&talent.raging_demonfire" );
  aoe->add_action( "summon_soulkeeper,if=buff.tormented_soul.stack=10|buff.tormented_soul.stack>3&fight_remains<10" );
  aoe->add_action( "call_action_list,name=ogcd" );
  aoe->add_action( "summon_infernal,if=cooldown.invoke_power_infusion_0.up|cooldown.invoke_power_infusion_0.duration=0|fight_remains>=190&!talent.grand_warlocks_design" );
  aoe->add_action( "rain_of_fire,if=debuff.pyrogenics.down&active_enemies<=4" );
  aoe->add_action( "channel_demonfire,if=dot.immolate.remains>cast_time" );
  aoe->add_action( "immolate,target_if=min:dot.immolate.remains+99*debuff.havoc.remains,if=((dot.immolate.refreshable&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>dot.immolate.remains))|active_enemies>active_dot.immolate)&target.time_to_die>10&!havoc_active" );
  aoe->add_action( "immolate,target_if=min:dot.immolate.remains+99*debuff.havoc.remains,if=((dot.immolate.refreshable&variable.havoc_immo_time<5.4)|(dot.immolate.remains<2&dot.immolate.remains<havoc_remains)|!dot.immolate.ticking|(variable.havoc_immo_time<2)*havoc_active)&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>dot.immolate.remains)&target.time_to_die>11" );
  aoe->add_action( "dimensional_rift" );
  aoe->add_action( "soul_fire,if=buff.backdraft.up" );
  aoe->add_action( "incinerate,if=talent.fire_and_brimstone.enabled&buff.backdraft.up" );
  aoe->add_action( "conflagrate,if=buff.backdraft.stack<2|!talent.backdraft" );
  aoe->add_action( "incinerate" );

  cleave->add_action( "call_action_list,name=items" );
  cleave->add_action( "call_action_list,name=ogcd" );
  cleave->add_action( "call_action_list,name=havoc,if=havoc_active&havoc_remains>gcd.max" );
  cleave->add_action( "variable,name=pool_soul_shards,value=cooldown.havoc.remains<=10|talent.mayhem" );
  cleave->add_action( "conflagrate,if=(talent.roaring_blaze.enabled&debuff.conflagrate.remains<1.5)|charges=max_charges&!variable.pool_soul_shards" );
  cleave->add_action( "dimensional_rift,if=soul_shard<4.7&(charges>2|fight_remains<cooldown.dimensional_rift.duration)" );
  cleave->add_action( "cataclysm,if=raid_event.adds.in>15" );
  cleave->add_action( "channel_demonfire,if=talent.raging_demonfire&active_dot.immolate=2" );
  cleave->add_action( "soul_fire,if=soul_shard<=3.5&(debuff.conflagrate.remains>cast_time+travel_time|!talent.roaring_blaze&buff.backdraft.up)&!variable.pool_soul_shards" );
  cleave->add_action( "immolate,target_if=min:dot.immolate.remains+99*debuff.havoc.remains,if=(dot.immolate.refreshable&(dot.immolate.remains<cooldown.havoc.remains|!dot.immolate.ticking))&(!talent.cataclysm|cooldown.cataclysm.remains>remains)&(!talent.soul_fire|cooldown.soul_fire.remains+(!talent.mayhem*action.soul_fire.cast_time)>dot.immolate.remains)&target.time_to_die>15" );
  cleave->add_action( "havoc,target_if=min:((-target.time_to_die)<?-15)+dot.immolate.remains+99*(self.target=target),if=(!cooldown.summon_infernal.up|!talent.summon_infernal)&target.time_to_die>8" );
  cleave->add_action( "dimensional_rift,if=soul_shard<4.5&variable.pool_soul_shards" );
  cleave->add_action( "chaos_bolt,if=pet.infernal.active|pet.blasphemy.active|soul_shard>=4" );
  cleave->add_action( "summon_infernal" );
  cleave->add_action( "channel_demonfire,if=talent.ruin.rank>1&!(talent.diabolic_embers&talent.avatar_of_destruction&(talent.burn_to_ashes|talent.chaos_incarnate))" );
  cleave->add_action( "chaos_bolt,if=soul_shard>3.5" );
  cleave->add_action( "chaos_bolt,if=buff.rain_of_chaos.remains>cast_time" );
  cleave->add_action( "chaos_bolt,if=buff.backdraft.up" );
  cleave->add_action( "soul_fire,if=soul_shard<=4&talent.mayhem" );
  cleave->add_action( "chaos_bolt,if=talent.eradication&debuff.eradication.remains<cast_time+action.chaos_bolt.travel_time+1&!action.chaos_bolt.in_flight" );
  cleave->add_action( "channel_demonfire,if=!(talent.diabolic_embers&talent.avatar_of_destruction&(talent.burn_to_ashes|talent.chaos_incarnate))" );
  cleave->add_action( "dimensional_rift" );
  cleave->add_action( "chaos_bolt,if=soul_shard>3.5&!variable.pool_soul_shards" );
  cleave->add_action( "chaos_bolt,if=fight_remains<5&fight_remains>cast_time+travel_time" );
  cleave->add_action( "summon_soulkeeper,if=buff.tormented_soul.stack=10|buff.tormented_soul.stack>3&fight_remains<10" );
  cleave->add_action( "conflagrate,if=charges>(max_charges-1)|fight_remains<gcd.max*charges" );
  cleave->add_action( "incinerate" );

  havoc->add_action( "conflagrate,if=talent.backdraft&buff.backdraft.down&soul_shard>=1&soul_shard<=4" );
  havoc->add_action( "soul_fire,if=cast_time<havoc_remains&soul_shard<2.5" );
  havoc->add_action( "channel_demonfire,if=soul_shard<4.5&talent.raging_demonfire.rank=2" );
  havoc->add_action( "immolate,target_if=min:dot.immolate.remains+100*debuff.havoc.remains,if=(((dot.immolate.refreshable&variable.havoc_immo_time<5.4)&target.time_to_die>5)|((dot.immolate.remains<2&dot.immolate.remains<havoc_remains)|!dot.immolate.ticking|variable.havoc_immo_time<2)&target.time_to_die>11)&soul_shard<4.5" );
  havoc->add_action( "chaos_bolt,if=((talent.cry_havoc&!talent.inferno)|!talent.rain_of_fire)&cast_time<havoc_remains" );
  havoc->add_action( "chaos_bolt,if=cast_time<havoc_remains&(active_enemies<=3-talent.inferno+(talent.chaosbringer&!talent.inferno))" );
  havoc->add_action( "rain_of_fire,if=active_enemies>=3&talent.inferno" );
  havoc->add_action( "rain_of_fire,if=(active_enemies>=4-talent.inferno+talent.chaosbringer)" );
  havoc->add_action( "rain_of_fire,if=active_enemies>2&(talent.avatar_of_destruction|(talent.rain_of_chaos&buff.rain_of_chaos.up))&talent.inferno.enabled" );
  havoc->add_action( "channel_demonfire,if=soul_shard<4.5" );
  havoc->add_action( "conflagrate,if=!talent.backdraft" );
  havoc->add_action( "dimensional_rift,if=soul_shard<4.7&(charges>2|fight_remains<cooldown.dimensional_rift.duration)" );
  havoc->add_action( "incinerate,if=cast_time<havoc_remains" );

  items->add_action( "use_item,use_off_gcd=1,name=belorrelos_the_suncaller,if=((time>20&cooldown.summon_infernal.remains>20)|(trinket.1.is.belorrelos_the_suncaller&(trinket.2.cooldown.remains|!variable.trinket_2_buffs|trinket.1.is.time_thiefs_gambit))|(trinket.2.is.belorrelos_the_suncaller&(trinket.1.cooldown.remains|!variable.trinket_1_buffs|trinket.2.is.time_thiefs_gambit)))&(!raid_event.adds.exists|raid_event.adds.up|spell_targets.belorrelos_the_suncaller>=5)|fight_remains<20" );
  items->add_action( "use_item,name=nymues_unraveling_spindle,if=(variable.infernal_active|!talent.summon_infernal|(variable.trinket_1_will_lose_cast&trinket.1.is.nymues_unraveling_spindle)|(variable.trinket_2_will_lose_cast&trinket.2.is.nymues_unraveling_spindle))|fight_remains<20" );
  items->add_action( "use_item,slot=trinket1,if=(variable.infernal_active|!talent.summon_infernal|variable.trinket_1_will_lose_cast)&(variable.trinket_priority=1|variable.trinket_2_exclude|!trinket.2.has_cooldown|(trinket.2.cooldown.remains|variable.trinket_priority=2&cooldown.summon_infernal.remains>20&!variable.infernal_active&trinket.2.cooldown.remains<cooldown.summon_infernal.remains))&variable.trinket_1_buffs&!variable.trinket_1_manual|(variable.trinket_1_buff_duration+1>=fight_remains)", "We want to use trinkets with Infernal unless we will miss a trinket use. The trinket with highest estimated value, will be used first." );
  items->add_action( "use_item,slot=trinket2,if=(variable.infernal_active|!talent.summon_infernal|variable.trinket_2_will_lose_cast)&(variable.trinket_priority=2|variable.trinket_1_exclude|!trinket.1.has_cooldown|(trinket.1.cooldown.remains|variable.trinket_priority=1&cooldown.summon_infernal.remains>20&!variable.infernal_active&trinket.1.cooldown.remains<cooldown.summon_infernal.remains))&variable.trinket_2_buffs&!variable.trinket_2_manual|(variable.trinket_2_buff_duration+1>=fight_remains)" );
  items->add_action( "use_item,name=time_thiefs_gambit,if=variable.infernal_active|!talent.summon_infernal|fight_remains<15|((trinket.1.cooldown.duration<cooldown.summon_infernal.remains_expected+5)&active_enemies=1)|(active_enemies>1&havoc_active)" );
  items->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=!variable.trinket_1_buffs&!variable.trinket_1_manual&(!variable.trinket_1_buffs&(trinket.2.cooldown.remains|!variable.trinket_2_buffs)|talent.summon_infernal&cooldown.summon_infernal.remains_expected>20&!prev_gcd.1.summon_infernal|!talent.summon_infernal)", "If only one on use trinket provied a buff, use the other on cooldown, Or if neither trinket provied a buff, use both on cooldown." );
  items->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=!variable.trinket_2_buffs&!variable.trinket_2_manual&(!variable.trinket_2_buffs&(trinket.1.cooldown.remains|!variable.trinket_1_buffs)|talent.summon_infernal&cooldown.summon_infernal.remains_expected>20&!prev_gcd.1.summon_infernal|!talent.summon_infernal)" );
  items->add_action( "use_item,use_off_gcd=1,slot=main_hand" );

  ogcd->add_action( "potion,if=variable.infernal_active|!talent.summon_infernal" );
  ogcd->add_action( "invoke_external_buff,name=power_infusion,if=variable.infernal_active|!talent.summon_infernal|(fight_remains<cooldown.summon_infernal.remains_expected+10+cooldown.invoke_power_infusion_0.duration&fight_remains>cooldown.invoke_power_infusion_0.duration)|fight_remains<cooldown.summon_infernal.remains_expected+15" );
  ogcd->add_action( "berserking,if=variable.infernal_active|!talent.summon_infernal|(fight_remains<(cooldown.summon_infernal.remains_expected+cooldown.berserking.duration)&(fight_remains>cooldown.berserking.duration))|fight_remains<cooldown.summon_infernal.remains_expected" );
  ogcd->add_action( "blood_fury,if=variable.infernal_active|!talent.summon_infernal|(fight_remains<cooldown.summon_infernal.remains_expected+10+cooldown.blood_fury.duration&fight_remains>cooldown.blood_fury.duration)|fight_remains<cooldown.summon_infernal.remains" );
  ogcd->add_action( "fireblood,if=variable.infernal_active|!talent.summon_infernal|(fight_remains<cooldown.summon_infernal.remains_expected+10+cooldown.fireblood.duration&fight_remains>cooldown.fireblood.duration)|fight_remains<cooldown.summon_infernal.remains_expected" );
  ogcd->add_action( "ancestral_call,if=variable.infernal_active|!talent.summon_infernal|(fight_remains<(cooldown.summon_infernal.remains_expected+cooldown.berserking.duration)&(fight_remains>cooldown.berserking.duration))|fight_remains<cooldown.summon_infernal.remains_expected" );

  variables->add_action( "variable,name=havoc_immo_time,op=reset" );
  variables->add_action( "cycling_variable,name=havoc_immo_time,op=add,value=dot.immolate.remains*debuff.havoc.up" );
  variables->add_action( "variable,name=infernal_active,op=set,value=pet.infernal.active|(cooldown.summon_infernal.duration-cooldown.summon_infernal.remains)<20" );
  variables->add_action( "variable,name=trinket_1_will_lose_cast,value=((floor((fight_remains%trinket.1.cooldown.duration)+1)!=floor((fight_remains+(cooldown.summon_infernal.duration-cooldown.summon_infernal.remains))%cooldown.summon_infernal.duration))&(floor((fight_remains%trinket.1.cooldown.duration)+1))!=(floor(((fight_remains-cooldown.summon_infernal.remains)%trinket.1.cooldown.duration)+1))|((floor((fight_remains%trinket.1.cooldown.duration)+1)=floor((fight_remains+(cooldown.summon_infernal.duration-cooldown.summon_infernal.remains))%cooldown.summon_infernal.duration))&(((fight_remains-cooldown.summon_infernal.remains%%trinket.1.cooldown.duration)-cooldown.summon_infernal.remains-variable.trinket_1_buff_duration)>0)))&cooldown.summon_infernal.remains>20", "If we can have more use of trinket than use of infernal, we want to it, but we want to sync if we don't lose a cast, and if we sync we don't lose it too late" );
  variables->add_action( "variable,name=trinket_2_will_lose_cast,value=((floor((fight_remains%trinket.2.cooldown.duration)+1)!=floor((fight_remains+(cooldown.summon_infernal.duration-cooldown.summon_infernal.remains))%cooldown.summon_infernal.duration))&(floor((fight_remains%trinket.2.cooldown.duration)+1))!=(floor(((fight_remains-cooldown.summon_infernal.remains)%trinket.2.cooldown.duration)+1))|((floor((fight_remains%trinket.2.cooldown.duration)+1)=floor((fight_remains+(cooldown.summon_infernal.duration-cooldown.summon_infernal.remains))%cooldown.summon_infernal.duration))&(((fight_remains-cooldown.summon_infernal.remains%%trinket.2.cooldown.duration)-cooldown.summon_infernal.remains-variable.trinket_2_buff_duration)>0)))&cooldown.summon_infernal.remains>20" );
}
//destruction_apl_end

} // namespace warlock_apl
