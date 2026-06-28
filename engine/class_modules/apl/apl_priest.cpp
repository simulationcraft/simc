// ==========================================================================
// Priest APL File
// Contact: https://github.com/orgs/simulationcraft/teams/priest/members
// Wiki: https://github.com/simulationcraft/simc/wiki/Priests
// ==========================================================================

#include "class_modules/apl/apl_priest.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace priest_apl
{
std::string potion( const player_t* p )
{
  return ( p->true_level > 80 ) ? "lights_potential_2" : "tempered_potion_3";
}

std::string flask( const player_t* p )
{
  return ( p->true_level > 80 ) ? "flask_of_the_magisters_2" : "flask_of_alchemical_chaos_3";
}

std::string food( const player_t* p )
{
  return ( p->true_level > 80 ) ? "silvermoon_parade" : "feast_of_the_divine_day";
}

std::string rune( const player_t* p )
{
  return ( p->true_level > 80 ) ? "void_touched" : "crystallized";
}

std::string temporary_enchant( const player_t* p )
{
  return ( p->true_level > 80 ) ? "main_hand:thalassian_phoenix_oil_2" : "main_hand:algari_mana_oil_3";
}

//shadow_apl_start
void shadow( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* aoe_variables = p->get_action_priority_list( "aoe_variables" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* heal_for_tof = p->get_action_priority_list( "heal_for_tof" );
  action_priority_list_t* main = p->get_action_priority_list( "main" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );

  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "shadowform,if=!buff.shadowform.up" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=(trinket.1.has_buff.intellect|trinket.1.has_buff.mastery|trinket.1.has_buff.versatility|trinket.1.has_buff.haste|trinket.1.has_buff.crit|trinket.1.is.signet_of_the_priory)&(trinket.1.cooldown.duration>=20)" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=(trinket.2.has_buff.intellect|trinket.2.has_buff.mastery|trinket.2.has_buff.versatility|trinket.2.has_buff.haste|trinket.2.has_buff.crit|trinket.2.is.signet_of_the_priory)&(trinket.2.cooldown.duration>=20)" );
  precombat->add_action( "variable,name=dr_force_prio,default=0,op=reset" );
  precombat->add_action( "variable,name=me_force_prio,default=0,op=reset" );
  precombat->add_action( "variable,name=max_vts,default=12,op=reset" );
  precombat->add_action( "variable,name=is_vt_possible,default=0,op=reset" );
  precombat->add_action( "arcane_torrent" );
  precombat->add_action( "tentacle_slam" );

  default_->add_action( "variable,name=holding_tentacle_slam,op=set,value=raid_event.adds.in<15" );
  default_->add_action( "call_action_list,name=aoe,if=active_enemies>2" );
  default_->add_action( "run_action_list,name=main" );

  aoe->add_action( "call_action_list,name=aoe_variables" );

  aoe_variables->add_action( "variable,name=max_vts,op=set,default=12,value=spell_targets.vampiric_touch>?12" );
  aoe_variables->add_action( "variable,name=is_vt_possible,op=set,value=0,default=1" );
  aoe_variables->add_action( "variable,name=is_vt_possible,op=set,value=1,target_if=max:(target.time_to_die*dot.vampiric_touch.refreshable),if=target.time_to_die>=18" );
  aoe_variables->add_action( "variable,name=dots_up,op=set,value=(active_dot.vampiric_touch>=variable.max_vts|!variable.is_vt_possible)&(active_dot.shadow_word_pain>=active_dot.vampiric_touch)", "TODO: Revamp to fix undesired behavior with unstacked fights" );
  aoe_variables->add_action( "variable,name=holding_tentacle_slam,op=set,value=(variable.max_vts-active_dot.vampiric_touch)<4&raid_event.adds.in>15|raid_event.adds.in<10&raid_event.adds.count>(variable.max_vts-active_dot.vampiric_touch),if=variable.holding_tentacle_slam&action.tentacle_slam.enabled&raid_event.adds.exists" );
  aoe_variables->add_action( "variable,name=manual_vts_applied,op=set,value=(active_dot.vampiric_touch+6*!variable.holding_tentacle_slam)>=variable.max_vts|!variable.is_vt_possible" );

  cds->add_action( "potion,if=((buff.voidform.up|!talent.voidform)&buff.power_infusion.up)&(fight_remains>=320|time_to_bloodlust>=320|buff.bloodlust.react)|fight_remains<=30", "TODO: Add holding condition for weird fight times to potion with execute" );
  cds->add_action( "fireblood,if=((buff.voidform.up|!talent.voidform)&buff.power_infusion.up)|fight_remains<=8" );
  cds->add_action( "berserking,if=((buff.voidform.up|!talent.voidform)&buff.power_infusion.up)|fight_remains<=12" );
  cds->add_action( "blood_fury,if=((buff.voidform.up|!talent.voidform)&buff.power_infusion.up)|fight_remains<=15" );
  cds->add_action( "ancestral_call,if=((buff.voidform.up|!talent.voidform)&buff.power_infusion.up)|fight_remains<=15" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=(buff.voidform.up|!talent.voidform)&!buff.power_infusion.up" );
  cds->add_action( "invoke_external_buff,name=bloodlust,if=buff.power_infusion.up&fight_remains<120|fight_remains<=40" );
  cds->add_action( "flash_heal,if=equipped.nexuskings_command&buff.oathbound.up&(!buff.boon_of_the_oathsworn.up|buff.boon_of_the_oathsworn.remains<3)&((talent.voidform&(buff.voidform.up|cooldown.voidform.up))|cooldown.halo.up|cooldown.void_torrent.up)", "Use Flash Heal to proc Nexus-King's Command trinket" );
  cds->add_action( "power_infusion,if=(buff.voidform.up|!talent.voidform)&!buff.power_infusion.up", "Sync Power Infusion with Voidform or Dark Ascension" );
  cds->add_action( "halo" );
  cds->add_action( "voidform,if=active_dot.shadow_word_pain>=active_dot.vampiric_touch" );
  cds->add_action( "call_action_list,name=trinkets" );
  cds->add_action( "desperate_prayer,if=health.pct<=75", "Use Desperate Prayer to heal up should Shadow Word: Death or other damage bring you below 75%" );

  heal_for_tof->add_action( "holy_nova,if=talent.lightburst", "Use Halo to acquire Twist of Fate if an ally can be healed for it and it is not currently up." );

  main->add_action( "variable,name=dots_up,op=set,value=active_dot.vampiric_touch=active_enemies&active_dot.shadow_word_pain>=active_dot.vampiric_touch,if=active_enemies<3" );
  main->add_action( "call_action_list,name=cds,if=fight_remains<30|target.time_to_die>15&(!variable.holding_tentacle_slam|active_enemies>2)&variable.dots_up" );
  main->add_action( "shadow_word_death,target_if=max:(target.health.pct<=20)*100+dot.shadow_word_madness.ticking,if=(priest.force_devour_matter|target.has_absorb)&talent.devour_matter", "High Priority Shadow Word: Death when Devour Matter is active (target shielded or forced)" );
  main->add_action( "shadow_word_madness,target_if=max:target.time_to_die*(dot.shadow_word_madness.remains<=gcd.max|variable.dr_force_prio|!talent.distorted_reality&variable.me_force_prio),if=active_dot.shadow_word_madness<=1&dot.shadow_word_madness.remains<=gcd.max|insanity.deficit<=35|buff.mind_devourer.react|!raid_event.adds.exists&target.time_to_die<=10|buff.entropic_rift.up&action.shadow_word_madness.cost>0", "Do not overcap on insanity" );
  main->add_action( "void_volley" );
  main->add_action( "void_blast,target_if=max:(dot.shadow_word_madness.remains*1000+target.time_to_die)", "Blast more burst :wicked:" );
  main->add_action( "tentacle_slam,target_if=min:dot.vampiric_touch.remains,if=dot.vampiric_touch.refreshable|cooldown.tentacle_slam.full_recharge_time<=gcd.max*2", "Use Tentacle Slam to prevent capping charges or to refresh Vampiric Touch" );
  main->add_action( "void_torrent,target_if=max:(dot.shadow_word_madness.remains*1000+target.time_to_die),if=!variable.holding_tentacle_slam&variable.dots_up", "Use Void Torrent if it will get near full Mastery Value" );
  main->add_action( "shadow_word_pain,target_if=max:(refreshable*100000+target.time_to_die+dot.vampiric_touch.ticking*10000),if=talent.invoked_nightmare&refreshable&target.time_to_die>12&dot.vampiric_touch.ticking", "Put out Shadow Word: Pain on enemies that will live at least 12s as a filler when talented into Invoked Nightmare." );
  main->add_action( "mind_blast,target_if=max:dot.shadow_word_madness.remains,if=(!buff.mind_devourer.react|!talent.mind_devourer)", "Use all charges of Mind Blast if Vampiric Touch and Shadow Word: Pain are active and Mind Devourer is not active or you are prepping Void Eruption" );
  main->add_action( "mind_flay_insanity,target_if=max:dot.shadow_word_madness.remains", "MFI is a good button" );
  main->add_action( "tentacle_slam,target_if=min:dot.vampiric_touch.remains,if=(talent.void_apparitions|talent.maddening_tentacles)&(raid_event.adds.in>30|raid_event.adds.in>5&cooldown.tentacle_slam.full_recharge_time<=gcd.max*2)", "Use Tentacle Slam for Void Apparitions or Maddening Tentacles value, holding for adds if needed" );
  main->add_action( "vampiric_touch,target_if=max:(refreshable*10000+target.time_to_die)*(dot.vampiric_touch.ticking|!variable.dots_up),if=refreshable&target.time_to_die>12&(dot.vampiric_touch.ticking|!variable.dots_up)&(variable.max_vts>0|active_enemies=1)&(action.tentacle_slam.usable_in>=dot.vampiric_touch.remains|variable.holding_tentacle_slam|!action.tentacle_slam.enabled)", "Put out Vampiric Touch on enemies that will live at least 12s and Tentacle Slam is not available soon" );
  main->add_action( "call_action_list,name=heal_for_tof,if=!buff.twist_of_fate.up&buff.twist_of_fate_can_trigger_on_ally_heal.up&talent.halo", "Healing spell action list for proccing Twist of Fate. Set priest.twist_of_fate_heal_rppm=<rppm> to make this be used." );
  main->add_action( "vampiric_touch,target_if=max:(refreshable*10000+target.time_to_die),if=refreshable&target.time_to_die>12", "Put out Vampiric Touch on enemies that will live at least 12s as a filler action." );
  main->add_action( "shadow_word_death,target_if=min:target.health.pct,if=(pet.mindbender.active|pet.voidwraith.active|pet.shadowfiend.active)&talent.inescapable_torment|target.health.pct<(20+15*talent.deathspeaker)&talent.shadowfiend&talent.idol_of_yshaarj" );
  main->add_action( "shadow_word_death,target_if=min:target.health.pct,if=(target.health.pct<(20+15*talent.deathspeaker))" );
  main->add_action( "mind_flay,target_if=max:dot.shadow_word_madness.remains,chain=1,interrupt_immediate=1,interrupt_if=ticks>=3,interrupt_global=1" );
  main->add_action( "tentacle_slam,if=raid_event.adds.in>20", "Use Tentacle Slam while moving as a low-priority action when adds will not spawn in 20 seconds." );
  main->add_action( "shadow_word_death,target_if=target.health.pct<20", "Use Shadow Word: Death while moving as a low-priority action in execute" );
  main->add_action( "shadow_word_death,target_if=max:dot.shadow_word_madness.remains", "Use Shadow Word: Death while moving as a low-priority action" );
  main->add_action( "shadow_word_pain,target_if=min:remains", "Use Shadow Word: Pain while moving as a low-priority action" );

  trinkets->add_action( "use_item,name=galactic_gladiators_badge_of_ferocity,if=(buff.voidform.up|buff.power_infusion.remains>=10|(talent.voidform&cooldown.voidform.remains>10))|fight_remains<20" );
  trinkets->add_action( "use_items,if=(buff.voidform.up|buff.power_infusion.remains>=10|equipped.neural_synapse_enhancer&buff.entropic_rift.up)|fight_remains<20" );
}
//shadow_apl_end
//shadow_ptr_apl_start
void shadow_ptr( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* aoe_variables = p->get_action_priority_list( "aoe_variables" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* heal_for_tof = p->get_action_priority_list( "heal_for_tof" );
  action_priority_list_t* main = p->get_action_priority_list( "main" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );

  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "shadowform,if=!buff.shadowform.up" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=(trinket.1.has_buff.intellect|trinket.1.has_buff.mastery|trinket.1.has_buff.versatility|trinket.1.has_buff.haste|trinket.1.has_buff.crit|trinket.1.is.signet_of_the_priory)&(trinket.1.cooldown.duration>=20)" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=(trinket.2.has_buff.intellect|trinket.2.has_buff.mastery|trinket.2.has_buff.versatility|trinket.2.has_buff.haste|trinket.2.has_buff.crit|trinket.2.is.signet_of_the_priory)&(trinket.2.cooldown.duration>=20)" );
  precombat->add_action( "variable,name=dr_force_prio,default=0,op=reset" );
  precombat->add_action( "variable,name=me_force_prio,default=0,op=reset" );
  precombat->add_action( "variable,name=max_vts,default=12,op=reset" );
  precombat->add_action( "variable,name=is_vt_possible,default=0,op=reset" );
  precombat->add_action( "arcane_torrent" );
  precombat->add_action( "tentacle_slam" );

  default_->add_action( "variable,name=holding_tentacle_slam,op=set,value=raid_event.adds.in<15" );
  default_->add_action( "call_action_list,name=aoe,if=active_enemies>2" );
  default_->add_action( "run_action_list,name=main" );

  aoe->add_action( "call_action_list,name=aoe_variables" );

  aoe_variables->add_action( "variable,name=max_vts,op=set,default=12,value=spell_targets.vampiric_touch>?12" );
  aoe_variables->add_action( "variable,name=is_vt_possible,op=set,value=0,default=1" );
  aoe_variables->add_action( "variable,name=is_vt_possible,op=set,value=1,target_if=max:(target.time_to_die*dot.vampiric_touch.refreshable),if=target.time_to_die>=18" );
  aoe_variables->add_action( "variable,name=dots_up,op=set,value=(active_dot.vampiric_touch>=variable.max_vts|!variable.is_vt_possible)&(active_dot.shadow_word_pain>=active_dot.vampiric_touch)", "TODO: Revamp to fix undesired behavior with unstacked fights" );
  aoe_variables->add_action( "variable,name=holding_tentacle_slam,op=set,value=(variable.max_vts-active_dot.vampiric_touch)<4&raid_event.adds.in>15|raid_event.adds.in<10&raid_event.adds.count>(variable.max_vts-active_dot.vampiric_touch),if=variable.holding_tentacle_slam&action.tentacle_slam.enabled&raid_event.adds.exists" );
  aoe_variables->add_action( "variable,name=manual_vts_applied,op=set,value=(active_dot.vampiric_touch+6*!variable.holding_tentacle_slam)>=variable.max_vts|!variable.is_vt_possible" );

  cds->add_action( "potion,if=((buff.voidform.up|!talent.voidform)&buff.power_infusion.up)&(fight_remains>=320|time_to_bloodlust>=320|buff.bloodlust.react)|fight_remains<=30", "TODO: Add holding condition for weird fight times to potion with execute" );
  cds->add_action( "fireblood,if=((buff.voidform.up|!talent.voidform)&buff.power_infusion.up)|fight_remains<=8" );
  cds->add_action( "berserking,if=((buff.voidform.up|!talent.voidform)&buff.power_infusion.up)|fight_remains<=12" );
  cds->add_action( "blood_fury,if=((buff.voidform.up|!talent.voidform)&buff.power_infusion.up)|fight_remains<=15" );
  cds->add_action( "ancestral_call,if=((buff.voidform.up|!talent.voidform)&buff.power_infusion.up)|fight_remains<=15" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=(buff.voidform.up|!talent.voidform)&!buff.power_infusion.up" );
  cds->add_action( "invoke_external_buff,name=bloodlust,if=buff.power_infusion.up&fight_remains<120|fight_remains<=40" );
  cds->add_action( "flash_heal,if=equipped.nexuskings_command&buff.oathbound.up&(!buff.boon_of_the_oathsworn.up|buff.boon_of_the_oathsworn.remains<3)&((talent.voidform&(buff.voidform.up|cooldown.voidform.up))|cooldown.halo.up|cooldown.void_torrent.up)", "Use Flash Heal to proc Nexus-King's Command trinket" );
  cds->add_action( "power_infusion,if=(buff.voidform.up|!talent.voidform)&!buff.power_infusion.up", "Sync Power Infusion with Voidform or Dark Ascension" );
  cds->add_action( "halo" );
  cds->add_action( "voidform,if=active_dot.shadow_word_pain>=active_dot.vampiric_touch" );
  cds->add_action( "call_action_list,name=trinkets" );
  cds->add_action( "desperate_prayer,if=health.pct<=75", "Use Desperate Prayer to heal up should Shadow Word: Death or other damage bring you below 75%" );

  heal_for_tof->add_action( "holy_nova,if=talent.lightburst", "Use Halo to acquire Twist of Fate if an ally can be healed for it and it is not currently up." );

  main->add_action( "variable,name=dots_up,op=set,value=active_dot.vampiric_touch=active_enemies&active_dot.shadow_word_pain>=active_dot.vampiric_touch,if=active_enemies<3" );
  main->add_action( "call_action_list,name=cds,if=fight_remains<30|target.time_to_die>15&(!variable.holding_tentacle_slam|active_enemies>2)&variable.dots_up" );
  main->add_action( "shadow_word_death,target_if=max:(target.health.pct<=20)*100+dot.shadow_word_madness.ticking,if=(priest.force_devour_matter|target.has_absorb)&talent.devour_matter", "High Priority Shadow Word: Death when Devour Matter is active (target shielded or forced)" );
  main->add_action( "shadow_word_madness,target_if=max:target.time_to_die*(dot.shadow_word_madness.remains<=gcd.max|variable.dr_force_prio|!talent.distorted_reality&variable.me_force_prio),if=active_dot.shadow_word_madness<=1&dot.shadow_word_madness.remains<=gcd.max|insanity.deficit<=35|buff.mind_devourer.react|!raid_event.adds.exists&target.time_to_die<=10|buff.entropic_rift.up&action.shadow_word_madness.cost>0", "Do not overcap on insanity" );
  main->add_action( "void_blast,target_if=max:(dot.shadow_word_madness.remains*1000+target.time_to_die)", "Blast more burst :wicked:" );
  main->add_action( "tentacle_slam,target_if=min:dot.vampiric_touch.remains,if=dot.vampiric_touch.refreshable|cooldown.tentacle_slam.full_recharge_time<=gcd.max*2", "Use Tentacle Slam to prevent capping charges or to refresh Vampiric Touch" );
  main->add_action( "void_torrent,target_if=max:(dot.shadow_word_madness.remains*1000+target.time_to_die),if=!variable.holding_tentacle_slam&variable.dots_up", "Use Void Torrent if it will get near full Mastery Value" );
  main->add_action( "shadow_word_pain,target_if=max:(refreshable*100000+target.time_to_die+dot.vampiric_touch.ticking*10000),if=talent.invoked_nightmare&refreshable&target.time_to_die>12&dot.vampiric_touch.ticking", "Put out Shadow Word: Pain on enemies that will live at least 12s as a filler when talented into Invoked Nightmare." );
  main->add_action( "mind_blast,target_if=max:dot.shadow_word_madness.remains,if=(!buff.mind_devourer.react|!talent.mind_devourer)", "Use all charges of Mind Blast if Vampiric Touch and Shadow Word: Pain are active and Mind Devourer is not active or you are prepping Void Eruption" );
  main->add_action( "vampiric_touch,if=(buff.vampiric_insight.up&buff.vampiric_insight.remains<3)|buff.vampiric_insight.at_max_stacks" );
  main->add_action( "mind_flay_insanity,target_if=max:dot.shadow_word_madness.remains" );
  main->add_action( "void_volley", "TODO: optimize this" );
  main->add_action( "tentacle_slam,target_if=min:dot.vampiric_touch.remains,if=(talent.void_apparitions|talent.maddening_tentacles)&(raid_event.adds.in>30|raid_event.adds.in>5&cooldown.tentacle_slam.full_recharge_time<=gcd.max*2)", "Use Tentacle Slam for Void Apparitions or Maddening Tentacles value, holding for adds if needed" );
  main->add_action( "vampiric_touch,target_if=max:(refreshable*10000+target.time_to_die)*(dot.vampiric_touch.ticking|!variable.dots_up),if=refreshable&target.time_to_die>12&(dot.vampiric_touch.ticking|!variable.dots_up)&(variable.max_vts>0|active_enemies=1)&(action.tentacle_slam.usable_in>=dot.vampiric_touch.remains|variable.holding_tentacle_slam|!action.tentacle_slam.enabled)", "Put out Vampiric Touch on enemies that will live at least 12s and Tentacle Slam is not available soon" );
  main->add_action( "call_action_list,name=heal_for_tof,if=!buff.twist_of_fate.up&buff.twist_of_fate_can_trigger_on_ally_heal.up&talent.halo", "Healing spell action list for proccing Twist of Fate. Set priest.twist_of_fate_heal_rppm=<rppm> to make this be used." );
  main->add_action( "vampiric_touch,target_if=max:(refreshable*10000+target.time_to_die),if=refreshable&target.time_to_die>12", "Put out Vampiric Touch on enemies that will live at least 12s as a filler action." );
  main->add_action( "shadow_word_death,target_if=min:target.health.pct,if=(pet.mindbender.active|pet.voidwraith.active|pet.shadowfiend.active)&talent.inescapable_torment|target.health.pct<(20+15*talent.deathspeaker)&talent.shadowfiend&talent.idol_of_yshaarj" );
  main->add_action( "shadow_word_death,target_if=min:target.health.pct,if=(target.health.pct<(20+15*talent.deathspeaker))" );
  main->add_action( "vampiric_touch,if=buff.vampiric_insight.up" );
  main->add_action( "mind_flay,target_if=max:dot.shadow_word_madness.remains,chain=1,interrupt_immediate=1,interrupt_if=ticks>=3,interrupt_global=1" );
  main->add_action( "tentacle_slam,if=raid_event.adds.in>20", "Use Tentacle Slam while moving as a low-priority action when adds will not spawn in 20 seconds." );
  main->add_action( "shadow_word_death,target_if=target.health.pct<20", "Use Shadow Word: Death while moving as a low-priority action in execute" );
  main->add_action( "shadow_word_death,target_if=max:dot.shadow_word_madness.remains", "Use Shadow Word: Death while moving as a low-priority action" );
  main->add_action( "shadow_word_pain,target_if=min:remains", "Use Shadow Word: Pain while moving as a low-priority action" );

  trinkets->add_action( "use_item,name=galactic_gladiators_badge_of_ferocity,if=(buff.voidform.up|buff.power_infusion.remains>=10|(talent.voidform&cooldown.voidform.remains>10))|fight_remains<20" );
  trinkets->add_action( "use_items,if=(buff.voidform.up|buff.power_infusion.remains>=10|equipped.neural_synapse_enhancer&buff.entropic_rift.up)|fight_remains<20" );
}
//shadow_ptr_apl_end
//discipline_apl_start
void discipline( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* main = p->get_action_priority_list( "main" );
  action_priority_list_t* cooldowns = p->get_action_priority_list( "cooldowns" );

  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "smite" );

  default_->add_action( "run_action_list,name=main" );

  main->add_action( "call_action_list,name=cooldowns" );
  main->add_action( "purge_the_wicked,if=refreshable" );
  main->add_action( "shadow_word_pain,if=refreshable" );
  main->add_action( "shadow_word_death,if=target.health.pct<20" );
  main->add_action( "penance" );
  main->add_action( "mind_blast" );
  main->add_action( "shadow_word_death,if=talent.expiation&(target.time_to_pct_20>(0.5*cooldown.shadow_word_death.duration))" );
  main->add_action( "halo" );
  main->add_action( "divine_star" );
  main->add_action( "shadow_word_death,if=target.time_to_pct_20>(0.5*cooldown.shadow_word_death.duration)" );
  main->add_action( "smite" );

  cooldowns->add_action( "power_infusion" );
  cooldowns->add_action( "potion,if=buff.power_infusion.up", "sync potion with PI" );
  cooldowns->add_action( "use_items,if=buff.power_infusion.up|cooldown.power_infusion.remains>=cooldown", "sync trinkets with PI" );
}
//discipline_apl_end
//holy_apl_start
void holy( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* main = p->get_action_priority_list( "main" );
  action_priority_list_t* divine_favor_chastise_prep = p->get_action_priority_list( "divine_favor_chastise_prep" );
  action_priority_list_t* divine_favor_chastise_active = p->get_action_priority_list( "divine_favor_chastise_active" );
  action_priority_list_t* divine_favor_filler = p->get_action_priority_list( "divine_favor_filler" );
  action_priority_list_t* divine_image = p->get_action_priority_list( "divine_image" );
  action_priority_list_t* generic = p->get_action_priority_list( "generic" );
  action_priority_list_t* cooldowns = p->get_action_priority_list( "cooldowns" );

  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  default_->add_action( "run_action_list,name=main", "RUN ACTIONS" );

  main->add_action( "call_action_list,name=cooldowns", "---------------------------------------------------------------------------  Main Actions  ---------------------------------------------------------------------------" );
  main->add_action( "holy_fire,cycle_targets=1,target_if=min:dot.holy_fire.remains,if=(talent.empyreal_blaze|talent.harmonious_apparatus|!ticking|refreshable)&!(buff.empyreal_blaze.up&(cooldown.divine_word.up|buff.divine_word.up)&cooldown.holy_word_chastise.up)", "Always use HF if we have empyreal blaze (dot extension) or harmonious apparatus (cd reduction) or if it's not currently ticking. Otherwise, only use when refreshable to be sure we get the longest duration possible. It's worth it to cast even when the target will die soon based on just the initial hit damage. We also don't want to cast immediately once empyreal blaze is up when we are prepping divine word" );
  main->add_action( "shadow_word_pain,if=(refreshable|!ticking)&(target.time_to_die>=dot.shadow_word_pain.duration)&!buff.divine_favor_chastise.up&!buff.apotheosis.up", "Don't cast SW:P during apotheosis or divine favor: chastise. We also don't cycle targets because it isn't worth the GCDs, since Smite deals slightly more damage than a full SW:P." );
  main->add_action( "call_action_list,name=divine_favor_chastise_prep,if=talent.divine_word&talent.holy_word_chastise&buff.divine_favor_chastise.down", "Prepare to enter divine favor: chastise" );
  main->add_action( "run_action_list,name=divine_favor_chastise_active,if=buff.divine_favor_chastise.up", "Enter Divine Favor rotation with divine favor: chastise buff up" );
  main->add_action( "run_action_list,name=divine_favor_filler,if=talent.divine_word&talent.holy_word_chastise&buff.divine_favor_chastise.down", "Run divine favor fillers rotation with buff down" );
  main->add_action( "run_action_list,name=divine_image,if=talent.divine_image", "Run divine image rotation with divine image talented" );
  main->add_action( "run_action_list,name=generic", "Otherwise generic rotation" );

  divine_favor_chastise_prep->add_action( "variable,name=empyreal_exec_time,op=setif,condition=talent.empyreal_blaze,value=action.empyreal_blaze.execute_time,value_else=0", "---------------------------------------------------------------------------  Divine Favor (Prep)  ---------------------------------------------------------------------------  empyreal_exec_time: Store how long EB will take to execute" );
  divine_favor_chastise_prep->add_action( "variable,name=apotheosis_exec_time,op=setif,condition=talent.apotheosis,value=action.apotheosis.execute_time,value_else=0", "apotheosis_exec_time: Store how long Apotheosis will take to execute" );
  divine_favor_chastise_prep->add_action( "apotheosis,if=(cooldown.holy_word_chastise.remains>cooldown.divine_word.remains)&(cooldown.divine_word.remains<=(variable.empyreal_exec_time+variable.apotheosis_exec_time))", "Use apotheosis to get Chastise back if its on cooldown and Divine Word will be up soon. We can use Apotheosis (2min CD) to reset Chastise for every other Divine Word (1min CD)." );
  divine_favor_chastise_prep->add_action( "empyreal_blaze,if=cooldown.divine_word.remains<=action.empyreal_blaze.execute_time", "If we're about to cast divine favor, cast empyreal blaze first so we don't waste a GCD on non-damage during the buff window" );
  divine_favor_chastise_prep->add_action( "divine_word,if=cooldown.holy_word_chastise.up&(!talent.empyreal_blaze|buff.empyreal_blaze.up)", "Divine Word only if we can sync with Chastise and (if talented) Empyreal Blaze" );
  divine_favor_chastise_prep->add_action( "holy_word_chastise,if=buff.divine_word.up", "Holy word chastise to trigger divine favor: chastise" );

  divine_favor_chastise_active->add_action( "halo,if=spell_targets.halo>=2", "---------------------------------------------------------------------------  Divine Favor (Active)  ---------------------------------------------------------------------------" );
  divine_favor_chastise_active->add_action( "divine_star,if=spell_targets.divine_star>=2" );
  divine_favor_chastise_active->add_action( "holy_nova,if=(spell_targets.holy_nova>=2&buff.rhapsody.stack>=18)|(spell_targets.holy_nova>=3&buff.rhapsody.stack>=9)|(spell_targets.holy_nova>=4&buff.rhapsody.stack>=4)|spell_targets.holy_nova>=5", "There are particular breakpoints combinations of rhapsody and spell targets beyond which holy nova beats everything else we can do" );
  divine_favor_chastise_active->add_action( "shadow_word_death,if=target.health.pct<20" );
  divine_favor_chastise_active->add_action( "holy_word_chastise" );
  divine_favor_chastise_active->add_action( "smite,cycle_targets=1,target_if=min:dot.holy_fire.remains,if=spell_targets.holy_nova>=2", "We want to cycle smite to different targets to spread holy fire dots in AOE situations, this will buff holy nova's damage" );
  divine_favor_chastise_active->add_action( "smite" );

  divine_favor_filler->add_action( "halo,if=spell_targets.halo>=2", "---------------------------------------------------------------------------  Divine Favor (Filler)  ---------------------------------------------------------------------------" );
  divine_favor_filler->add_action( "divine_star,if=spell_targets.divine_star>=2" );
  divine_favor_filler->add_action( "holy_nova,if=(spell_targets.holy_nova>=2&buff.rhapsody.stack>=18)|(spell_targets.holy_nova>=3&buff.rhapsody.stack>=9)|(spell_targets.holy_nova>=4&buff.rhapsody.stack>=4)|spell_targets.holy_nova>=5", "There are particular breakpoints combinations of rhapsody and spell targets beyond which holy nova beats everything else we can do" );
  divine_favor_filler->add_action( "shadow_word_death,if=target.health.pct<20" );
  divine_favor_filler->add_action( "holy_word_chastise,if=(cooldown.apotheosis.remains<cooldown.divine_word.remains)|(cooldown.holy_word_chastise.duration_expected<=cooldown.divine_word.remains)", "We can use chastise for damage as long as we will have apotheosis available before the next divine word, otherwise only use it when it will be back up at the same time as divine word" );
  divine_favor_filler->add_action( "smite" );

  divine_image->add_action( "apotheosis,if=buff.answered_prayers.down&!(cooldown.holy_word_sanctify.up|cooldown.holy_word_serenity.up|cooldown.holy_word_chastise.up)", "---------------------------------------------------------------------------  Divine Image  ---------------------------------------------------------------------------  We want to apotheosis when our holy words aren't about to come off of cooldown, and when answered prayers apotheosis is not already active." );
  divine_image->add_action( "holy_word_sanctify,line_cd=9", "line_cd prevents re-casting sanctify and serenity right away, wasting GCDs that could be used to proc divine image damage. In testing, the sweet spot balance between adding more divine images, triggering apotheosis, and casting damage CDs was to let the image from each healing holy word expire before re-casting" );
  divine_image->add_action( "holy_word_serenity,line_cd=9" );
  divine_image->add_action( "holy_word_chastise" );
  divine_image->add_action( "empyreal_blaze" );
  divine_image->add_action( "halo,if=spell_targets.halo>=2" );
  divine_image->add_action( "divine_star,if=spell_targets.divine_star>=2" );
  divine_image->add_action( "holy_nova,if=(spell_targets.holy_nova>=2&buff.rhapsody.stack>=18)|(spell_targets.holy_nova>=3&buff.rhapsody.stack>=9)|(spell_targets.holy_nova>=4&buff.rhapsody.stack>=4)|spell_targets.holy_nova>=5", "There are particular breakpoints combinations of rhapsody and spell targets beyond which holy nova beats everything else we can do" );
  divine_image->add_action( "shadow_word_death,if=target.health.pct<20" );
  divine_image->add_action( "smite" );

  generic->add_action( "holy_word_chastise", "---------------------------------------------------------------------------  Generic  ---------------------------------------------------------------------------" );
  generic->add_action( "empyreal_blaze" );
  generic->add_action( "apotheosis,if=cooldown.holy_word_chastise.remains>(gcd.max*3)", "Hold Apotheosis if chastise will be up soon" );
  generic->add_action( "halo,if=spell_targets.halo>=2" );
  generic->add_action( "divine_star,if=spell_targets.divine_star>=2" );
  generic->add_action( "holy_nova,if=(spell_targets.holy_nova>=2&buff.rhapsody.stack>=18)|(spell_targets.holy_nova>=3&buff.rhapsody.stack>=9)|(spell_targets.holy_nova>=4&buff.rhapsody.stack>=4)|spell_targets.holy_nova>=5", "There are particular breakpoints combinations of rhapsody and spell targets beyond which holy nova beats everything else we can do" );
  generic->add_action( "shadow_word_death,if=target.health.pct<20" );
  generic->add_action( "smite" );

  cooldowns->add_action( "power_infusion,if=(!talent.divine_word|(cooldown.divine_word.up&cooldown.holy_word_chastise.up))", "Sync PI with divine favor: chastise if we took divine word" );
  cooldowns->add_action( "potion,if=buff.power_infusion.up", "Only potion in sync with power infusion" );
  cooldowns->add_action( "use_items,if=buff.power_infusion.up", "hold trinkets to use with PI" );
}
//holy_apl_end
//nospec_apl_start
void no_spec( player_t* p )
{
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* def       = p->get_action_priority_list( "default" );

  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "smite" );
  def->add_action( "mana_potion,if=mana.pct<=75" );
  def->add_action( "berserking" );
  def->add_action( "arcane_torrent,if=mana.pct<=90" );
  def->add_action( "shadow_word_pain,if=remains<tick_time|!ticking" );
  def->add_action( "smite" );
}
//nospec_apl_end
}  // namespace priest_apl
