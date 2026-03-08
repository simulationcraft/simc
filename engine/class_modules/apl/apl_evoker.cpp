#include "class_modules/apl/apl_evoker.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace evoker_apl
{

std::string potion( const player_t* p )
{
  if ( p->specialization() == EVOKER_AUGMENTATION )
    return ( p->true_level > 89 ) ? "lights_potential_2" : "tempered_potion_3";

  return ( p->true_level > 89 ) ? "potion_of_recklessness_2" : "tempered_potion_3";
}

std::string flask( const player_t* p )
{
  return ( p->true_level > 89 ) ? "flask_of_the_blood_knights_2" : "flask_of_alchemical_chaos_3";
}

std::string food( const player_t* p )
{
  if ( p->specialization() == EVOKER_AUGMENTATION )
    return ( p->true_level > 89 ) ? "silvermoon_parade" : "feast_of_the_divine_day";

  return ( p->true_level > 89 ) ? "blooming_feast" : "feast_of_the_divine_day";
}

std::string rune( const player_t* p )
{
  return ( p->true_level > 89 ) ? "void_touched" : "crystallized";
}

std::string temporary_enchant( const player_t* p )
{
  return ( p->true_level > 89 ) ? "main_hand:thalassian_phoenix_oil_2" : "main_hand:algari_mana_oil_3";
}

//devastation_apl_start
void devastation( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe_fs = p->get_action_priority_list( "aoe_fs" );
  action_priority_list_t* st_fs = p->get_action_priority_list( "st_fs" );
  action_priority_list_t* aoe_sc = p->get_action_priority_list( "aoe_sc" );
  action_priority_list_t* st_sc = p->get_action_priority_list( "st_sc" );
  action_priority_list_t* es = p->get_action_priority_list( "es" );
  action_priority_list_t* green = p->get_action_priority_list( "green" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );

  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_buff.intellect|trinket.1.has_buff.mastery|trinket.1.has_buff.versatility|trinket.1.has_buff.haste|trinket.1.has_buff.crit|trinket.1.is.mirror_of_fractured_tomorrows|trinket.1.is.signet_of_the_priory" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_buff.intellect|trinket.2.has_buff.mastery|trinket.2.has_buff.versatility|trinket.2.has_buff.haste|trinket.2.has_buff.crit|trinket.2.is.mirror_of_fractured_tomorrows|trinket.2.is.signet_of_the_priory" );
  precombat->add_action( "variable,name=weapon_buffs,value=equipped.bestinslots" );
  precombat->add_action( "variable,name=weapon_sync,op=setif,value=1,value_else=0.5,condition=equipped.bestinslots" );
  precombat->add_action( "variable,name=weapon_stat_value,value=equipped.bestinslots*5142*15" );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_1_buffs&(trinket.1.cooldown.duration%%cooldown.dragonrage.duration=0|cooldown.dragonrage.duration%%trinket.1.cooldown.duration=0|trinket.1.is.house_of_cards)" );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_2_buffs&(trinket.2.cooldown.duration%%cooldown.dragonrage.duration=0|cooldown.dragonrage.duration%%trinket.2.cooldown.duration=0|trinket.2.is.house_of_cards)" );
  precombat->add_action( "variable,name=trinket_1_manual,value=trinket.1.is.belorrelos_the_suncaller|trinket.1.is.nymues_unraveling_spindle|trinket.1.is.spymasters_web" );
  precombat->add_action( "variable,name=trinket_2_manual,value=trinket.2.is.belorrelos_the_suncaller|trinket.2.is.nymues_unraveling_spindle|trinket.2.is.spymasters_web" );
  precombat->add_action( "variable,name=trinket_1_ogcd_cast,value=0" );
  precombat->add_action( "variable,name=trinket_2_ogcd_cast,value=0" );
  precombat->add_action( "variable,name=trinket_1_exclude,value=trinket.1.is.ruby_whelp_shell|trinket.1.is.whispering_incarnate_icon" );
  precombat->add_action( "variable,name=trinket_2_exclude,value=trinket.2.is.ruby_whelp_shell|trinket.2.is.whispering_incarnate_icon" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs|variable.trinket_2_buffs&((trinket.2.proc.any_dps.duration)*(variable.trinket_2_sync)*trinket.2.proc.any_dps.default_value)>((trinket.1.proc.any_dps.duration)*(variable.trinket_1_sync)*trinket.1.proc.any_dps.default_value)" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,if=variable.weapon_buffs,value=3,value_else=variable.trinket_priority,condition=!variable.trinket_1_buffs&!variable.trinket_2_buffs|variable.weapon_stat_value*variable.weapon_sync>(((trinket.2.proc.any_dps.duration)*(variable.trinket_2_sync)*trinket.2.proc.any_dps.default_value)<?((trinket.1.proc.any_dps.duration)*(variable.trinket_1_sync)*trinket.1.proc.any_dps.default_value))" );
  precombat->add_action( "variable,name=trinket_priority,op=set,value=trinket.1.is.signet_of_the_priory+2*trinket.2.is.signet_of_the_priory,if=equipped.signet_of_the_priory&variable.trinket_priority=3" );
  precombat->add_action( "variable,name=damage_trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&!variable.trinket_2_buffs&trinket.2.ilvl>=trinket.1.ilvl" );
  precombat->add_action( "variable,name=r1_cast_time,value=1.0*spell_haste" );
  precombat->add_action( "variable,name=dr_prep_time,default=6,op=reset" );
  precombat->add_action( "variable,name=dr_prep_time_aoe,default=4,op=reset" );
  precombat->add_action( "variable,name=has_external_pi,value=cooldown.invoke_power_infusion_0.duration>0" );
  precombat->add_action( "variable,name=can_use_empower,value=1,default=1,if=!talent.animosity|!talent.dragonrage" );
  precombat->add_action( "verdant_embrace,if=talent.scarlet_adaptation" );
  precombat->add_action( "hover,if=talent.slipstream" );
  precombat->add_action( "hover,if=talent.slipstream" );
  precombat->add_action( "living_flame" );

  default_->add_action( "potion,if=(!talent.dragonrage|buff.dragonrage.up)|fight_remains<35" );
  default_->add_action( "variable,name=next_dragonrage,value=cooldown.dragonrage.remains<?((cooldown.eternity_surge.remains-8)>?(cooldown.fire_breath.remains-8))" );
  default_->add_action( "invoke_external_buff,name=power_infusion,if=buff.dragonrage.up|fight_remains<35" );
  default_->add_action( "variable,name=can_use_empower,op=set,value=cooldown.dragonrage.remains>=gcd.max*variable.dr_prep_time,if=talent.animosity&talent.dragonrage" );
  default_->add_action( "quell,use_off_gcd=1,if=target.debuff.casting.react" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "run_action_list,name=aoe_sc,if=active_enemies>=3&talent.mass_disintegrate" );
  default_->add_action( "run_action_list,name=aoe_fs,if=active_enemies>=3" );
  default_->add_action( "run_action_list,name=st_sc,if=talent.mass_disintegrate" );
  default_->add_action( "run_action_list,name=st_fs" );

  aoe_fs->add_action( "hover,use_off_gcd=1,if=raid_event.movement.in<6&!buff.hover.up&gcd.remains>=0.5&active_enemies<=4", "Fameshaper 3+ Target List" );
  aoe_fs->add_action( "fire_breath,target_if=max:target.health.pct,empower_to=1,if=cooldown.dragonrage.remains<gcd.max*2&active_dot.fire_breath_damage=0&(target.time_to_die>=15|!raid_event.adds.exists)" );
  aoe_fs->add_action( "tip_the_scales,use_off_gcd=1,if=buff.dragonrage.up&cooldown.eternity_surge.remains<=action.fire_breath.usable_in" );
  aoe_fs->add_action( "call_action_list,name=es,if=buff.tip_the_scales.up" );
  aoe_fs->add_action( "fire_breath,target_if=max:target.health.pct,empower_to=1,if=talent.consume_flame&variable.can_use_empower&dot.fire_breath_damage.refreshable" );
  aoe_fs->add_action( "dragonrage,target_if=max:target.time_to_die,if=target.time_to_die>=15|!raid_event.adds.exists" );
  aoe_fs->add_action( "call_action_list,name=es,if=(buff.dragonrage.up|cooldown.dragonrage.remains>variable.dr_prep_time_aoe)&(buff.dragonrage.up|talent.azure_sweep&!buff.azure_sweep.up)&(active_dot.fire_breath_damage=0|active_enemies<=3)" );
  aoe_fs->add_action( "pyre,target_if=max:target.health.pct,if=(cooldown.dragonrage.remains>gcd.max*4)&(buff.charged_blast.stack>=12|active_enemies>=4|active_enemies>=3&(talent.feed_the_flames|talent.volatility))" );
  aoe_fs->add_action( "deep_breath,if=talent.imminent_destruction&active_dot.fire_breath_damage=0,cancel_if=gcd.remains=0,interrupt_if=gcd.remains=0" );
  aoe_fs->add_action( "azure_sweep,target_if=max:target.health.pct" );
  aoe_fs->add_action( "living_flame,target_if=max:target.health.pct,if=buff.leaping_flames.up&(!talent.burnout|buff.burnout.up|active_dot.fire_breath_damage=0|buff.scarlet_adaptation.up|buff.ancient_flame.up)&(!buff.essence_burst.up&essence.deficit>1|cooldown.fire_breath.remains<=gcd.max*3&buff.essence_burst.stack<buff.essence_burst.max_stack)" );
  aoe_fs->add_action( "call_action_list,name=es,if=(buff.dragonrage.up|cooldown.dragonrage.remains>variable.dr_prep_time_aoe)&(talent.azure_sweep&!buff.azure_sweep.up)" );
  aoe_fs->add_action( "living_flame,target_if=max:target.health.pct,if=talent.engulfing_blaze&(buff.leaping_flames.up|buff.burnout.up|buff.scarlet_adaptation.up|buff.ancient_flame.up)" );
  aoe_fs->add_action( "azure_strike,target_if=max:target.health.pct" );

  st_fs->add_action( "dragonrage,if=target.time_to_die>=30&raid_event.adds.in>=60|!raid_event.adds.exists|raid_event.adds.in=0", "Fameshaper 1 / 2 Target List" );
  st_fs->add_action( "hover,use_off_gcd=1,if=raid_event.movement.in<6&!buff.hover.up&gcd.remains>=0.5|talent.slipstream&gcd.remains>=0.5" );
  st_fs->add_action( "tip_the_scales,use_off_gcd=1,if=buff.dragonrage.up&action.eternity_surge.usable_in<=action.fire_breath.usable_in" );
  st_fs->add_action( "eternity_surge,target_if=max:target.health.pct,empower_to=2,if=active_enemies=2&!talent.eternitys_span&variable.can_use_empower" );
  st_fs->add_action( "eternity_surge,target_if=max:target.health.pct,empower_to=1,if=variable.can_use_empower|set_bonus.mid1_2pc&talent.azure_sweep" );
  st_fs->add_action( "fire_breath,target_if=max:target.health.pct,empower_to=1,if=variable.can_use_empower&!buff.tip_the_scales.up&dot.fire_breath_damage.refreshable&(cooldown.dragonrage.remains>full_recharge_time|buff.dragonrage.up|full_recharge_time<gcd.max*5)" );
  st_fs->add_action( "pyre,target_if=min:dot.fire_breath_damage.remains-100*dot.fire_breath_damage.ticking,if=active_enemies>1&dot.fire_breath_damage.remains<=8&talent.feed_the_flames&talent.volatility" );
  st_fs->add_action( "disintegrate,target_if=max:dot.fire_breath_damage.remains,chain=1,if=(raid_event.movement.in>2|buff.hover.up),early_chain_if=ticks_remain<=1,interrupt_if=ticks_remain<=1" );
  st_fs->add_action( "azure_sweep" );
  st_fs->add_action( "living_flame,if=buff.burnout.up|(buff.leaping_flames.up|buff.ancient_flame.up)&raid_event.movement.in>execute_time" );
  st_fs->add_action( "azure_strike,if=active_enemies>1" );
  st_fs->add_action( "living_flame,if=raid_event.movement.in>execute_time" );
  st_fs->add_action( "call_action_list,name=green,if=talent.ancient_flame&!buff.ancient_flame.up&talent.scarlet_adaptation&!buff.dragonrage.up" );
  st_fs->add_action( "azure_strike" );

  aoe_sc->add_action( "hover,use_off_gcd=1,if=raid_event.movement.in<6&!buff.hover.up&gcd.remains>=0.5", "Scalemander 3+ Target List" );
  aoe_sc->add_action( "deep_breath,if=talent.imminent_destruction&talent.strafing_run&!buff.strafing_run.up,cancel_if=gcd.remains=0" );
  aoe_sc->add_action( "tip_the_scales,use_off_gcd=1,if=buff.dragonrage.up" );
  aoe_sc->add_action( "dragonrage,target_if=max:target.time_to_die,if=target.time_to_die>=15|!raid_event.adds.exists" );
  aoe_sc->add_action( "eternity_surge,target_if=max:target.health.pct,empower_to=1" );
  aoe_sc->add_action( "fire_breath,target_if=max:target.health.pct,empower_to=1" );
  aoe_sc->add_action( "deep_breath,if=talent.imminent_destruction,cancel_if=gcd.remains=0,interrupt_if=gcd.remains=0" );
  aoe_sc->add_action( "pyre,target_if=max:target.health.pct,if=cooldown.dragonrage.remains>gcd.max*4&!buff.mass_disintegrate_stacks.up&(active_enemies>=4|active_enemies>=3&(talent.feed_the_flames|talent.volatility))" );
  aoe_sc->add_action( "disintegrate,target_if=min:debuff.bombardments.remains,if=talent.mass_disintegrate&(!talent.volatility|buff.mass_disintegrate_stacks.up),interrupt=1,interrupt_if=talent.volatility&active_enemies>=8" );
  aoe_sc->add_action( "azure_sweep,target_if=max:target.health.pct" );
  aoe_sc->add_action( "living_flame,target_if=max:target.health.pct,if=buff.leaping_flames.up&(!talent.burnout|buff.burnout.up|buff.scarlet_adaptation.up|buff.ancient_flame.up)&(!buff.essence_burst.up&essence.deficit>1)", "Check" );
  aoe_sc->add_action( "call_action_list,name=es,if=(buff.dragonrage.up|cooldown.dragonrage.remains>variable.dr_prep_time_aoe)&(talent.azure_sweep&!buff.azure_sweep.up)" );
  aoe_sc->add_action( "azure_strike,target_if=max:target.health.pct" );

  st_sc->add_action( "deep_breath,if=buff.strafing_run.remains<=gcd.max*2,cancel_if=gcd.remains=0", "Scalemander 1 / 2 Target List" );
  st_sc->add_action( "dragonrage,if=target.time_to_die>=30&raid_event.adds.in>=60|!raid_event.adds.exists|raid_event.adds.in=0" );
  st_sc->add_action( "hover,use_off_gcd=1,if=raid_event.movement.in<6&!buff.hover.up&gcd.remains>=0.5|talent.slipstream&gcd.remains>=0.5" );
  st_sc->add_action( "tip_the_scales,use_off_gcd=1,if=buff.dragonrage.up" );
  st_sc->add_action( "eternity_surge,empower_to=1" );
  st_sc->add_action( "fire_breath,empower_to=1" );
  st_sc->add_action( "disintegrate,target_if=min:debuff.bombardments.remains,early_chain_if=ticks_remain<=1&buff.mass_disintegrate_stacks.up,if=(raid_event.movement.in>2|buff.hover.up)&buff.mass_disintegrate_stacks.up&talent.mass_disintegrate" );
  st_sc->add_action( "disintegrate,target_if=max:dot.fire_breath_damage.remains,chain=1,if=(raid_event.movement.in>2|buff.hover.up),early_chain_if=ticks_remain<=1,interrupt_if=ticks_remain<=1" );
  st_sc->add_action( "azure_sweep" );
  st_sc->add_action( "living_flame,if=buff.burnout.up|(buff.leaping_flames.up|buff.ancient_flame.up)&raid_event.movement.in>execute_time" );
  st_sc->add_action( "azure_strike,if=active_enemies>1" );
  st_sc->add_action( "living_flame,if=raid_event.movement.in>execute_time" );
  st_sc->add_action( "call_action_list,name=green,if=talent.ancient_flame&!buff.ancient_flame.up&talent.scarlet_adaptation&!buff.dragonrage.up" );
  st_sc->add_action( "azure_strike" );

  es->add_action( "eternity_surge,empower_to=1,target_if=max:target.health.pct,if=active_enemies<=1+talent.eternitys_span|active_enemies>4+4*talent.eternitys_span|talent.mass_disintegrate|buff.dragonrage.up", "Eternity Surge" );
  es->add_action( "eternity_surge,empower_to=2,target_if=max:target.health.pct,if=active_enemies<=2+2*talent.eternitys_span" );
  es->add_action( "eternity_surge,empower_to=3,target_if=max:target.health.pct,if=active_enemies<=3+3*talent.eternitys_span" );
  es->add_action( "eternity_surge,empower_to=4,target_if=max:target.health.pct,if=active_enemies<=4+4*talent.eternitys_span" );

  green->add_action( "emerald_blossom", "And that one's still green" );
  green->add_action( "verdant_embrace" );

  trinkets->add_action( "use_item,slot=trinket1,if=buff.dragonrage.up&(buff.rising_fury.stack>=4|talent.legacy_of_the_lifebinder)&((variable.trinket_2_buffs&!cooldown.fire_breath.up&trinket.2.cooldown.remains)|buff.tip_the_scales.up&variable.trinket_priority=1|(!cooldown.fire_breath.up)|active_enemies>=3)&(!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1|variable.trinket_2_exclude)&!variable.trinket_1_manual|trinket.1.proc.any_dps.duration>=fight_remains|trinket.1.cooldown.duration<=60&(variable.next_dragonrage>20|!talent.dragonrage)&(!buff.dragonrage.up|variable.trinket_priority=1)&!variable.trinket_1_manual", "Trinket Spaghetti" );
  trinkets->add_action( "use_item,slot=trinket2,if=trinket.2.is.vaelgors_final_stare&buff.dragonrage.up&active_enemies>=3", "other spagetti is so complicated. Just Trinket in AoE without a delay." );
  trinkets->add_action( "use_item,slot=trinket2,if=buff.dragonrage.up&(buff.rising_fury.stack>=4|talent.legacy_of_the_lifebinder)&((variable.trinket_1_buffs&!cooldown.fire_breath.up&trinket.1.cooldown.remains)|buff.tip_the_scales.up&variable.trinket_priority=2|(!cooldown.fire_breath.up)|active_enemies>=3)&(!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2|variable.trinket_1_exclude)&!variable.trinket_2_manual|trinket.2.proc.any_dps.duration>=fight_remains|trinket.2.cooldown.duration<=60&(variable.next_dragonrage>20|!talent.dragonrage)&(!buff.dragonrage.up|variable.trinket_priority=2)&!variable.trinket_2_manual" );
  trinkets->add_action( "use_item,slot=main_hand,if=variable.weapon_buffs&((variable.trinket_2_buffs&(trinket.2.cooldown.remains|trinket.2.cooldown.duration<=20)|!variable.trinket_2_buffs|variable.trinket_2_exclude|variable.trinket_priority=3)&(variable.trinket_1_buffs&(trinket.1.cooldown.remains|trinket.1.cooldown.duration<=20)|!variable.trinket_1_buffs|variable.trinket_1_exclude|variable.trinket_priority=3)&(!cooldown.fire_breath.up|(!cooldown.fire_breath.up)|active_enemies>=3))&(variable.next_dragonrage>20|!talent.dragonrage)&(!buff.dragonrage.up|variable.trinket_priority=3|variable.trinket_priority=1&trinket.1.cooldown.remains|variable.trinket_priority=2&trinket.2.cooldown.remains)" );
  trinkets->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=!variable.trinket_1_buffs&!variable.trinket_1_manual&(variable.damage_trinket_priority=1|trinket.2.cooldown.remains|trinket.2.is.spymasters_web|trinket.2.cooldown.duration=0)&(gcd.remains>0.1&!prev_gcd.1.deep_breath)&(variable.next_dragonrage>20|!talent.dragonrage|!variable.trinket_2_buffs|trinket.2.is.spymasters_web&(buff.spymasters_report.stack<5|fight_remains>=130+variable.next_dragonrage))" );
  trinkets->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=!variable.trinket_2_buffs&!variable.trinket_2_manual&(variable.damage_trinket_priority=2|trinket.1.cooldown.remains|trinket.1.is.spymasters_web|trinket.1.cooldown.duration=0)&(gcd.remains>0.1&!prev_gcd.1.deep_breath)&(variable.next_dragonrage>20|!talent.dragonrage|!variable.trinket_1_buffs|trinket.1.is.spymasters_web&(buff.spymasters_report.stack<5|fight_remains>=130+variable.next_dragonrage))" );
  trinkets->add_action( "use_item,slot=trinket1,if=!variable.trinket_1_buffs&!variable.trinket_1_manual&(variable.damage_trinket_priority=1|trinket.2.cooldown.remains|trinket.2.is.spymasters_web|trinket.2.cooldown.duration=0)&(!variable.trinket_1_ogcd_cast)&(variable.next_dragonrage>20|!talent.dragonrage|!variable.trinket_2_buffs|trinket.2.is.spymasters_web&(buff.spymasters_report.stack<5|fight_remains>=130+variable.next_dragonrage))" );
  trinkets->add_action( "use_item,slot=trinket2,if=!variable.trinket_2_buffs&!variable.trinket_2_manual&(variable.damage_trinket_priority=2|trinket.1.cooldown.remains|trinket.1.is.spymasters_web|trinket.1.cooldown.duration=0)&(!variable.trinket_2_ogcd_cast)&(variable.next_dragonrage>20|!talent.dragonrage|!variable.trinket_1_buffs|trinket.1.is.spymasters_web&(buff.spymasters_report.stack<5|fight_remains>=130+variable.next_dragonrage))" );
}
//devastation_apl_end

//devastation_ptr_apl_start
void devastation_ptr( player_t* )
{
}
//devastation_ptr_apl_end

void preservation( player_t* /*p*/ )
{
}

//augmentation_apl_start
void augmentation( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* fb = p->get_action_priority_list( "fb" );
  action_priority_list_t* filler = p->get_action_priority_list( "filler" );
  action_priority_list_t* items = p->get_action_priority_list( "items" );
  action_priority_list_t* opener_filler = p->get_action_priority_list( "opener_filler" );

  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=spam_heal,default=0,op=reset" );
  precombat->add_action( "variable,name=minimum_opener_delay,op=reset,default=0" );
  precombat->add_action( "variable,name=opener_delay,value=variable.minimum_opener_delay,if=!talent.interwoven_threads" );
  precombat->add_action( "variable,name=opener_delay,value=variable.minimum_opener_delay+variable.opener_delay,if=talent.interwoven_threads" );
  precombat->add_action( "variable,name=opener_cds_detected,op=reset,default=0" );
  precombat->add_action( "variable,name=trinket_1_exclude,value=trinket.1.is.ruby_whelp_shell|trinket.1.is.whispering_incarnate_icon|trinket.1.is.ovinaxs_mercurial_egg|trinket.1.is.aberrant_spellforge" );
  precombat->add_action( "variable,name=trinket_2_exclude,value=trinket.2.is.ruby_whelp_shell|trinket.2.is.whispering_incarnate_icon|trinket.2.is.ovinaxs_mercurial_egg|trinket.2.is.aberrant_spellforge" );
  precombat->add_action( "variable,name=trinket_1_manual,value=trinket.1.is.nymues_unraveling_spindle|trinket.1.is.spymasters_web|trinket.1.is.treacherous_transmitter|trinket.1.is.house_of_cards", "Nymues is complicated, Manual Handle" );
  precombat->add_action( "variable,name=trinket_2_manual,value=trinket.2.is.nymues_unraveling_spindle|trinket.2.is.spymasters_web|trinket.2.is.treacherous_transmitter|trinket.2.is.house_of_cards" );
  precombat->add_action( "variable,name=trinket_1_ogcd_cast,value=trinket.1.is.beacon_to_the_beyond" );
  precombat->add_action( "variable,name=trinket_2_ogcd_cast,value=trinket.2.is.beacon_to_the_beyond" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=(trinket.1.has_use_buff|(trinket.1.has_buff.intellect|trinket.1.has_buff.mastery|trinket.1.has_buff.versatility|trinket.1.has_buff.haste|trinket.1.has_buff.crit)&!variable.trinket_1_exclude)&(!trinket.1.is.flarendos_pilot_light)" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=(trinket.2.has_use_buff|(trinket.2.has_buff.intellect|trinket.2.has_buff.mastery|trinket.2.has_buff.versatility|trinket.2.has_buff.haste|trinket.2.has_buff.crit)&!variable.trinket_2_exclude)&(!trinket.2.is.flarendos_pilot_light)" );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_1_buffs&(trinket.1.cooldown.duration%%120=0)" );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_2_buffs&(trinket.2.cooldown.duration%%120=0)" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs&(trinket.2.has_cooldown&!variable.trinket_2_exclude|!trinket.1.has_cooldown)|variable.trinket_2_buffs&((trinket.2.cooldown.duration%trinket.2.proc.any_dps.duration)*(0.5+trinket.2.has_buff.intellect*3+trinket.2.has_buff.mastery)*(variable.trinket_2_sync))>((trinket.1.cooldown.duration%trinket.1.proc.any_dps.duration)*(0.5+trinket.1.has_buff.intellect*3+trinket.1.has_buff.mastery)*(variable.trinket_1_sync)*(1+((trinket.1.ilvl-trinket.2.ilvl)%100)))" );
  precombat->add_action( "variable,name=damage_trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&!variable.trinket_2_buffs&trinket.2.ilvl>=trinket.1.ilvl" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=trinket.1.is.nymues_unraveling_spindle&trinket.2.has_buff.intellect|trinket.2.is.nymues_unraveling_spindle&!trinket.1.has_buff.intellect,if=(trinket.1.is.nymues_unraveling_spindle|trinket.2.is.nymues_unraveling_spindle)&(variable.trinket_1_buffs&variable.trinket_2_buffs)", "Double on use - Priotize Intellect on use trinkets over Nymues, force overwriting the normal logic to guarantee it is correct." );
  precombat->add_action( "variable,name=hold_empower_for,op=reset,default=6" );
  precombat->add_action( "variable,name=ebon_might_pandemic_threshold,op=reset,default=0.4" );
  precombat->add_action( "variable,name=wingleader_force_timings,op=reset,default=0" );
  precombat->add_action( "variable,name=enforce_timings,op=reset,default=0" );
  precombat->add_action( "variable,name=spam_on_use_trinket,op=reset,default=0" );
  precombat->add_action( "use_item,name=aberrant_spellforge" );
  precombat->add_action( "blistering_scales,target_if=target.role.tank" );
  precombat->add_action( "living_flame" );

  default_->add_action( "variable,name=temp_wound,value=debuff.temporal_wound.remains,target_if=max:debuff.temporal_wound.remains" );
  default_->add_action( "variable,name=eons_remains,op=setif,value=cooldown.allied_virtual_cd_time.remains,value_else=cooldown.breath_of_eons.remains,condition=!talent.wingleader&(!set_bonus.tww3_2pc|!talent.temporal_burst.enabled|variable.enforce_timings)&!apex.1|variable.wingleader_force_timings,if=talent.breath_of_eons" );
  default_->add_action( "variable,name=eons_remains,op=set,value=cooldown.allied_virtual_cd_time.remains,if=!talent.breath_of_eons" );
  default_->add_action( "variable,name=pool_for_id,default=0,op=set,value=(variable.eons_remains<8&talent.breath_of_eons&(target.time_to_die>13&raid_event.adds.in>15|fight_remains<30&!fight_style.dungeonroute)|cooldown.deep_breath.remains<8&!talent.breath_of_eons)&essence.deficit>=2&!buff.essence_burst.react,if=(!set_bonus.tww3_4pc|!hero_tree.chronowarden)&talent.imminent_destruction" );
  default_->add_action( "hover,use_off_gcd=1,if=gcd.remains>=0.5&(!raid_event.movement.exists&(trinket.1.is.ovinaxs_mercurial_egg|trinket.2.is.ovinaxs_mercurial_egg)|raid_event.movement.in<=6)" );
  default_->add_action( "ebon_might,if=((buff.ebon_might_self.remains-cast_time)<=buff.ebon_might_self.duration*variable.ebon_might_pandemic_threshold)&(active_enemies>0|raid_event.adds.in<=3)&(buff.ebon_might_self.value<=0.05|(buff.ebon_might_self.remains-cast_time)<=buff.ebon_might_self.duration*0.3)" );
  default_->add_action( "prescience,target_if=min:(debuff.prescience.remains-200*(target.role.attack|target.role.spell|target.role.dps)+50*target.spec.augmentation),if=debuff.prescience.remains<gcd.max*2&(!talent.anachronism|buff.essence_burst.stack<buff.essence_burst.max_stack|time<=10)", "actions+=/time_skip,target_if=max:debuff.bombardments.remains,if=cooldown.fire_breath.full_recharge_time>0&cooldown.prescience.full_recharge_time>0&cooldown.breath_of_eons.full_recharge_time>0&cooldown.upheaval.full_recharge_time>0 actions+=/prescience,target_if=min:(debuff.prescience.remains-200*(target.role.attack|target.role.spell|target.role.dps)+50*target.spec.augmentation),if=((full_recharge_time<=gcd.max*3|cooldown.ebon_might.remains<=gcd.max*3&(buff.ebon_might_self.remains-gcd.max*3)<=buff.ebon_might_self.duration*variable.ebon_might_pandemic_threshold|fight_remains<=30)|variable.eons_remains<=8|talent.anachronism&buff.imminent_destruction.up&essence<1&!cooldown.fire_breath.up&!cooldown.upheaval.up)&debuff.prescience.remains<gcd.max*2&(!talent.anachronism|buff.essence_burst.stack<buff.essence_burst.max_stack|time<=10)&full_recharge_time<=gcd.max" );
  default_->add_action( "potion,if=variable.eons_remains<=0|cooldown.breath_of_eons.remains>=90|fight_remains<=30&!fight_style.dungeonroute", "actions+=/prescience,target_if=min:(debuff.prescience.remains-200*(target.spec.augmentation|target.role.tank)),if=debuff.prescience.remains<gcd.max*2&(target.spec.augmentation|target.role.tank)&(!talent.anachronism|buff.essence_burst.stack<buff.essence_burst.max_stack|time<=5)" );
  default_->add_action( "cancel_buff,name=tip_the_scales,if=cooldown.upheaval.remains>0&cooldown.fire_breath.remains>0&(talent.energy_cycles|talent.temporal_burst)" );
  default_->add_action( "call_action_list,name=items" );
  default_->add_action( "run_action_list,name=opener_filler,if=variable.opener_delay>0&!fight_style.dungeonroute" );
  default_->add_action( "eruption,if=buff.imminent_destruction.up&essence.deficit=0" );
  default_->add_action( "fury_of_the_aspects,if=talent.time_convergence&!buff.time_convergence_intellect.up&(essence>=2|buff.essence_burst.react)&variable.eons_remains>=8" );
  default_->add_action( "tip_the_scales,if=!cooldown.breath_of_eons.up&(buff.duplicate.up|!talent.energy_cycles)&(action.upheaval.usable_in<action.fire_breath.usable_in|!talent.molten_embers)|talent.energy_cycles&(action.upheaval.usable_in<action.fire_breath.usable_in|!talent.molten_embers|action.upheaval.usable_in>gcd.max*2)&!cooldown.breath_of_eons.up" );
  default_->add_action( "deep_breath,cancel_if=gcd.remains<=0" );
  default_->add_action( "breath_of_eons,if=talent.wingleader&(fight_style.dungeonroute|fight_style.dungeonslice)&target.time_to_die>=13&evoker.allied_cds_up>0,cancel_if=gcd.remains<=0&ticks>=10" );
  default_->add_action( "breath_of_eons,if=talent.wingleader&(target.time_to_die>=13&(raid_event.adds.in>=25|!raid_event.adds.exists|raid_event.adds.remains>=13&raid_event.adds.count%1.5<active_enemies))&!variable.wingleader_force_timings&(time%%240<=230&time%%240>=3|fight_style.dungeonroute|fight_style.dungeonslice)|fight_remains<=30&!fight_style.dungeonroute,cancel_if=gcd.remains<=0&ticks>=10" );
  default_->add_action( "breath_of_eons,if=(target.time_to_die>13&raid_event.adds.in>15|fight_remains<30&!fight_style.dungeonroute)&variable.eons_remains<=0|fight_remains<=15&(talent.imminent_destruction|talent.melt_armor)&!fight_style.dungeonroute,cancel_if=gcd.remains<=0&ticks>=10", "&(time%%30<=12)" );
  default_->add_action( "breath_of_eons,if=fight_style.dungeonroute&(target.time_to_die>=13&(raid_event.adds.in>=25|!raid_event.adds.exists|raid_event.adds.remains>=13&raid_event.adds.count%1.5<active_enemies)),cancel_if=gcd.remains<=0&ticks>=10" );
  default_->add_action( "call_action_list,name=fb,if=(raid_event.adds.remains>6|raid_event.adds.in>20|evoker.allied_cds_up>0|!raid_event.adds.exists)&(!cooldown.breath_of_eons.up|!talent.temporal_burst)&(!buff.tip_the_scales.up|!talent.molten_embers)" );
  default_->add_action( "upheaval,target_if=target.time_to_die>duration+0.2,empower_to=1,if=buff.ebon_might_self.remains>duration&(raid_event.adds.remains>10|evoker.allied_cds_up>0|!raid_event.adds.exists|raid_event.adds.in>20)" );
  default_->add_action( "time_skip,if=((((cooldown.fire_breath.remains>?20)+(cooldown.upheaval.remains>?20)))>=30&(cooldown.breath_of_eons.remains>=20&talent.breath_of_eons|!talent.breath_of_eons&cooldown.deep_breath.remains>=20))&(!talent.interwoven_threads&!talent.chronoboon)|talent.chronoboon&cooldown.tip_the_scales.remains>=6&!buff.tip_the_scales.up" );
  default_->add_action( "emerald_blossom,if=talent.dream_of_spring&buff.essence_burst.react&(variable.spam_heal=2|variable.spam_heal=1&!buff.ancient_flame.up&talent.ancient_flame)&(buff.ebon_might_self.up|essence.deficit=0|buff.essence_burst.stack=buff.essence_burst.max_stack&cooldown.ebon_might.remains>4)" );
  default_->add_action( "emerald_blossom,if=talent.dream_of_spring&mana>=650000&talent.ancient_flame&!buff.ancient_flame.up&buff.essence_burst.up&(buff.temporal_burst.up&set_bonus.tww3_4pc&(!buff.imminent_destruction.up|mana>=2000000)|equipped.diamantine_voidcore&mana.pct>=50)", "actions+=/time_spiral,if=talent.time_convergence&!buff.time_convergence_intellect.up&(essence>=2|buff.essence_burst.react) actions+=/oppressing_roar,if=talent.time_convergence&!buff.time_convergence_intellect.up&(essence>=2|buff.essence_burst.react)" );
  default_->add_action( "eruption,target_if=min:debuff.bombardments.remains,if=(buff.ebon_might_self.remains>execute_time|essence.deficit=0|buff.essence_burst.stack=buff.essence_burst.max_stack&cooldown.ebon_might.remains>4|buff.essence_burst.react&set_bonus.tww2_4pc)&!variable.pool_for_id&(buff.imminent_destruction.up|essence.deficit<=2|buff.essence_burst.up|variable.ebon_might_pandemic_threshold>0|buff.duplicate.up)" );
  default_->add_action( "run_action_list,name=filler", "actions+=/blistering_scales,target_if=target.role.tank,if=!evoker.scales_up&buff.ebon_might_self.down" );

  fb->add_action( "fire_breath,empower_to=1,target_if=target.time_to_die>16,if=buff.ebon_might_self.remains>duration&talent.molten_embers", "actions.fb=tip_the_scales,if=cooldown.fire_breath.ready&buff.ebon_might_self.up" );
  fb->add_action( "fire_breath,empower_to=2,target_if=target.time_to_die>12,if=buff.ebon_might_self.remains>duration" );
  fb->add_action( "fire_breath,empower_to=3,target_if=target.time_to_die>8,if=buff.ebon_might_self.remains>duration" );
  fb->add_action( "fire_breath,empower_to=4,target_if=target.time_to_die>4,if=talent.font_of_magic&buff.ebon_might_self.remains>duration" );

  filler->add_action( "living_flame,if=(buff.ancient_flame.up|mana>=200000|!talent.dream_of_spring|variable.spam_heal=0)&(talent.pupil_of_alexstrasza&active_enemies>1|!talent.echoing_strike|talent.chrono_flame)|buff.leaping_flames.up" );
  filler->add_action( "azure_strike" );

  items->add_action( "use_item,name=nymues_unraveling_spindle,if=cooldown.breath_of_eons.remains<=3&(trinket.1.is.nymues_unraveling_spindle&variable.trinket_priority=1|trinket.2.is.nymues_unraveling_spindle&variable.trinket_priority=2)|(cooldown.fire_breath.remains<=4|cooldown.upheaval.remains<=4)&cooldown.breath_of_eons.remains>10&!(debuff.temporal_wound.up|prev_gcd.1.breath_of_eons)&(trinket.1.is.nymues_unraveling_spindle&variable.trinket_priority=2|trinket.2.is.nymues_unraveling_spindle&variable.trinket_priority=1)" );
  items->add_action( "use_item,name=aberrant_spellforge" );
  items->add_action( "use_item,name=flarendos_pilot_light,if=!variable.trinket_1_buffs&!variable.trinket_1_manual&trinket.2.is.flarendos_pilot_light|!variable.trinket_2_buffs&!variable.trinket_2_manual&trinket.1.is.flarendos_pilot_light" );
  items->add_action( "use_item,name=treacherous_transmitter,if=cooldown.allied_virtual_cd_time.remains<=10|cooldown.breath_of_eons.remains<=10&talent.wingleader|fight_remains<=15" );
  items->add_action( "do_treacherous_transmitter_task,use_off_gcd=1,if=(debuff.temporal_wound.up|prev_gcd.1.breath_of_eons|fight_remains<=15)" );
  items->add_action( "use_item,name=spymasters_web,if=(debuff.temporal_wound.up|prev_gcd.1.breath_of_eons)&(fight_remains<=130-(30+12*talent.interwoven_threads)*talent.wingleader-20*talent.time_skip*(cooldown.time_skip.remains<=90)*!talent.interwoven_threads)|(fight_remains<=20|evoker.allied_cds_up>0&fight_remains<=60)&(trinket.1.is.spymasters_web&(trinket.2.cooldown.duration=0|trinket.2.cooldown.remains>=10|variable.trinket_2_exclude)|trinket.2.is.spymasters_web&(trinket.1.cooldown.duration=0|trinket.1.cooldown.remains>=10|variable.trinket_1_exclude))&!buff.spymasters_web.up" );
  items->add_action( "use_item,name=house_of_cards,if=evoker.shifting_buffs>=2|evoker.shifting_buffs>=1&(cooldown.fire_breath.remains<=7|cooldown.upheaval.remains<=7)" );
  items->add_action( "use_item,slot=trinket1,if=variable.trinket_1_buffs&!variable.trinket_1_manual&!variable.trinket_1_exclude&((debuff.temporal_wound.up|prev_gcd.1.breath_of_eons|!talent.breath_of_eons&buff.ebon_might_self.up&active_enemies>=1|variable.spam_on_use_trinket&(!cooldown.breath_of_eons.up|variable.eons_remains>=10))|variable.trinket_2_buffs&!trinket.2.cooldown.up&(prev_gcd.1.fire_breath|prev_gcd.1.upheaval)&buff.ebon_might_self.up)&(variable.trinket_2_exclude|!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1)|trinket.1.proc.any_dps.duration>=fight_remains" );
  items->add_action( "use_item,slot=trinket2,if=variable.trinket_2_buffs&!variable.trinket_2_manual&!variable.trinket_2_exclude&((debuff.temporal_wound.up|prev_gcd.1.breath_of_eons|!talent.breath_of_eons&buff.ebon_might_self.up&active_enemies>=1|variable.spam_on_use_trinket&(!cooldown.breath_of_eons.up|variable.eons_remains>=10))|variable.trinket_1_buffs&!trinket.1.cooldown.up&(prev_gcd.1.fire_breath|prev_gcd.1.upheaval)&buff.ebon_might_self.up)&(variable.trinket_1_exclude|!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2)|trinket.2.proc.any_dps.duration>=fight_remains" );
  items->add_action( "azure_strike,if=cooldown.item_cd_1141.up&(variable.trinket_1_ogcd_cast&trinket.1.cooldown.up&(variable.damage_trinket_priority=1|trinket.2.cooldown.remains)|variable.trinket_2_ogcd_cast&trinket.2.cooldown.up&(variable.damage_trinket_priority=2|trinket.1.cooldown.remains))", "Azure Strike for OGCD trinkets. Ideally this would be Prescience casts in reality but this is simpler and seems to have no noticeable diferrence in DPS." );
  items->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=!variable.trinket_1_buffs&!variable.trinket_1_manual&!variable.trinket_1_exclude&(variable.damage_trinket_priority=1|trinket.2.cooldown.remains|trinket.2.is.spymasters_web&buff.spymasters_report.stack<30|variable.eons_remains>=20|!talent.breath_of_eons|trinket.2.cooldown.duration=0|variable.trinket_2_exclude)&(gcd.remains>0.1&variable.trinket_1_ogcd_cast)", "If only one on use trinket provides a buff, use the other on cooldown. Or if neither trinket provides a buff, use both on cooldown." );
  items->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=!variable.trinket_2_buffs&!variable.trinket_2_manual&!variable.trinket_2_exclude&(variable.damage_trinket_priority=2|trinket.1.cooldown.remains|trinket.1.is.spymasters_web&buff.spymasters_report.stack<30|variable.eons_remains>=20|!talent.breath_of_eons|trinket.1.cooldown.duration=0|variable.trinket_1_exclude)&(gcd.remains>0.1&variable.trinket_2_ogcd_cast)" );
  items->add_action( "use_item,slot=trinket1,if=!variable.trinket_1_buffs&!variable.trinket_1_manual&!variable.trinket_1_exclude&(variable.damage_trinket_priority=1|trinket.2.cooldown.remains|trinket.2.is.spymasters_web&buff.spymasters_report.stack<30|variable.eons_remains>=20|!talent.breath_of_eons|trinket.2.cooldown.duration=0|variable.trinket_2_exclude)&(!variable.trinket_1_ogcd_cast)" );
  items->add_action( "use_item,slot=trinket2,if=!variable.trinket_2_buffs&!variable.trinket_2_manual&!variable.trinket_2_exclude&(variable.damage_trinket_priority=2|trinket.1.cooldown.remains|trinket.1.is.spymasters_web&buff.spymasters_report.stack<30|variable.eons_remains>=20|!talent.breath_of_eons|trinket.1.cooldown.duration=0|variable.trinket_1_exclude)&(!variable.trinket_2_ogcd_cast)" );
  items->add_action( "use_item,name=bestinslots,use_off_gcd=1,if=buff.ebon_might_self.up&(!variable.trinket_1_buffs|trinket.1.cooldown.duration<=20|trinket.1.cooldown.remains>=10)&(!variable.trinket_2_buffs|trinket.2.cooldown.duration<=20|trinket.2.cooldown.remains>=10)" );
  items->add_action( "use_item,slot=main_hand,use_off_gcd=1,if=gcd.remains>=gcd.max*0.6&!equipped.bestinslots", "Use on use weapons" );

  opener_filler->add_action( "variable,name=opener_delay,value=variable.opener_delay>?variable.minimum_opener_delay,if=!variable.opener_cds_detected&evoker.allied_cds_up>0" );
  opener_filler->add_action( "variable,name=opener_delay,value=variable.opener_delay-1" );
  opener_filler->add_action( "variable,name=opener_cds_detected,value=1,if=!variable.opener_cds_detected&evoker.allied_cds_up>0" );
  opener_filler->add_action( "eruption,if=variable.opener_delay>=3" );
  opener_filler->add_action( "living_flame,if=active_enemies=1|talent.pupil_of_alexstrasza" );
  opener_filler->add_action( "azure_strike" );
}
//augmentation_apl_end

//augmentation_ptr_apl_start
void augmentation_ptr( player_t* )
{
}
//augmentation_ptr_apl_end

void no_spec( player_t* /*p*/ )
{
}

}  // namespace evoker_apl
