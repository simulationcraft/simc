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

  default_->add_action( "shadow_bolt" );
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
  precombat->add_action( "variable,name=trinket_1_manual,value=trinket.1.is.belorrelos_the_suncaller|trinket.1.is.nymues_unraveling_spindle|trinket.1.is.timethiefs_gambit|trinket.1.is.spymasters_web" );
  precombat->add_action( "variable,name=trinket_2_manual,value=trinket.2.is.belorrelos_the_suncaller|trinket.2.is.nymues_unraveling_spindle|trinket.2.is.timethiefs_gambit|trinket.2.is.spymasters_web" );
  precombat->add_action( "variable,name=trinket_1_exclude,value=trinket.1.is.ruby_whelp_shell|trinket.1.is.whispering_incarnate_icon" );
  precombat->add_action( "variable,name=trinket_2_exclude,value=trinket.2.is.ruby_whelp_shell|trinket.2.is.whispering_incarnate_icon" );
  precombat->add_action( "variable,name=trinket_1_buff_duration,value=trinket.1.proc.any_dps.duration+(trinket.1.is.mirror_of_fractured_tomorrows*20)+(trinket.1.is.nymues_unraveling_spindle*2)" );
  precombat->add_action( "variable,name=trinket_2_buff_duration,value=trinket.2.proc.any_dps.duration+(trinket.2.is.mirror_of_fractured_tomorrows*20)+(trinket.2.is.nymues_unraveling_spindle*2)" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs|variable.trinket_2_buffs&((trinket.2.cooldown.duration%variable.trinket_2_buff_duration)*(1+0.5*trinket.2.has_buff.intellect)*(variable.trinket_2_sync)*(1-0.5*trinket.2.is.mirror_of_fractured_tomorrows))>((trinket.1.cooldown.duration%variable.trinket_1_buff_duration)*(1+0.5*trinket.1.has_buff.intellect)*(variable.trinket_1_sync)*(1-0.5*trinket.1.is.mirror_of_fractured_tomorrows))" );
  precombat->add_action( "variable,name=allow_rof_2t_spender,default=2,op=reset" );
  precombat->add_action( "variable,name=do_rof_2t,value=variable.allow_rof_2t_spender>1.99&!(talent.cataclysm&talent.improved_chaos_bolt),op=set" );
  precombat->add_action( "variable,name=disable_cb_2t,value=variable.do_rof_2t|variable.allow_rof_2t_spender>0.01&variable.allow_rof_2t_spender<0.99" );
  precombat->add_action( "grimoire_of_sacrifice,if=talent.grimoire_of_sacrifice.enabled" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "soul_fire" );
  precombat->add_action( "cataclysm,if=raid_event.adds.in>15" );
  precombat->add_action( "incinerate" );

  default_->add_action( "call_action_list,name=variables" );
  default_->add_action( "call_action_list,name=aoe,if=(active_enemies>=3)&!variable.cleave_apl" );
  default_->add_action( "call_action_list,name=cleave,if=active_enemies!=1|variable.cleave_apl" );
  default_->add_action( "call_action_list,name=ogcd" );
  default_->add_action( "call_action_list,name=items" );
  default_->add_action( "malevolence,if=cooldown.summon_infernal.remains>=55" );
  default_->add_action( "chaos_bolt,if=demonic_art" );
  default_->add_action( "soul_fire,if=buff.decimation.react&(soul_shard<=4|buff.decimation.remains<=gcd.max*2)&debuff.conflagrate.remains>=execute_time" );
  default_->add_action( "wither,if=talent.internal_combustion&(((dot.wither.remains-5*action.chaos_bolt.in_flight)<dot.wither.duration*0.4)|dot.wither.remains<3|(dot.wither.remains-action.chaos_bolt.execute_time)<5&action.chaos_bolt.usable)&(!talent.soul_fire|cooldown.soul_fire.remains+action.soul_fire.cast_time>(dot.wither.remains-5))&target.time_to_die>8&!action.soul_fire.in_flight_to_target" );
  default_->add_action( "conflagrate,if=talent.roaring_blaze&debuff.conflagrate.remains<1.5|full_recharge_time<=gcd.max*2|recharge_time<=8&(diabolic_ritual&(buff.diabolic_ritual_mother_of_chaos.remains+buff.diabolic_ritual_overlord.remains+buff.diabolic_ritual_pit_lord.remains)<gcd.max)&soul_shard>=1.5" );
  default_->add_action( "shadowburn,if=(cooldown.shadowburn.full_recharge_time<=gcd.max*3|debuff.eradication.remains<=gcd.max&talent.eradication&!action.chaos_bolt.in_flight&!talent.diabolic_ritual)&(talent.conflagration_of_chaos|talent.blistering_atrophy)|fight_remains<=8" );
  default_->add_action( "chaos_bolt,if=buff.ritual_of_ruin.up" );
  default_->add_action( "shadowburn,if=cooldown.summon_infernal.remains>=90&talent.rain_of_chaos" );
  default_->add_action( "chaos_bolt,if=cooldown.summon_infernal.remains>=90&talent.rain_of_chaos" );
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
  aoe->add_action( "call_action_list,name=havoc,if=havoc_active&havoc_remains>gcd.max&active_enemies<5&(!cooldown.summon_infernal.up|!talent.summon_infernal)" );
  aoe->add_action( "malevolence,if=cooldown.summon_infernal.remains>=55&soul_shard<4.7&(active_enemies<=3+active_dot.wither|time>30)" );
  aoe->add_action( "dimensional_rift,if=soul_shard<4.7&(charges>2|fight_remains<cooldown.dimensional_rift.duration)" );
  aoe->add_action( "rain_of_fire,if=soul_shard>=(4.5-0.1*active_dot.immolate)" );
  aoe->add_action( "wither,target_if=min:dot.wither.remains+99*debuff.havoc.remains+99*!dot.wither.ticking,if=dot.wither.refreshable&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>dot.wither.remains)&(!talent.raging_demonfire|cooldown.channel_demonfire.remains>remains|time<5)&(active_dot.wither<=4|time>15)&target.time_to_die>18" );
  aoe->add_action( "channel_demonfire,if=dot.immolate.remains+dot.wither.remains>cast_time&talent.raging_demonfire" );
  aoe->add_action( "shadowburn,if=(active_enemies<4+(talent.cataclysm+4*talent.cataclysm)*talent.wither)&((cooldown.shadowburn.full_recharge_time<=gcd.max*3|debuff.eradication.remains<=gcd.max&talent.eradication&!action.chaos_bolt.in_flight&!talent.diabolic_ritual)&(talent.conflagration_of_chaos|talent.blistering_atrophy)&(active_enemies<5+(talent.wither&talent.cataclysm)+havoc_active)|fight_remains<=8)" );
  aoe->add_action( "ruination" );
  aoe->add_action( "rain_of_fire,if=pet.infernal.active|pet.overfiend.active" );
  aoe->add_action( "rain_of_fire,if=fight_remains<12|time>5" );
  aoe->add_action( "infernal_bolt,if=soul_shard<2.5" );
  aoe->add_action( "chaos_bolt,if=soul_shard>3.5-(0.1*active_enemies)&!talent.rain_of_fire" );
  aoe->add_action( "cataclysm,if=raid_event.adds.in>15|talent.wither" );
  aoe->add_action( "havoc,target_if=min:((-target.time_to_die)<?-15)+dot.immolate.remains+99*(self.target=target),if=(!cooldown.summon_infernal.up|!talent.summon_infernal|(talent.inferno&active_enemies>4))&target.time_to_die>8&cooldown.malevolence.remains>15|time<5" );
  aoe->add_action( "wither,target_if=min:dot.wither.remains+99*debuff.havoc.remains,if=dot.wither.refreshable&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>dot.wither.remains)&(!talent.raging_demonfire|cooldown.channel_demonfire.remains>remains|time<5)&active_dot.wither<=active_enemies&target.time_to_die>18" );
  aoe->add_action( "immolate,target_if=min:dot.immolate.remains+99*debuff.havoc.remains,if=dot.immolate.refreshable&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>dot.immolate.remains)&(!talent.raging_demonfire|cooldown.channel_demonfire.remains>remains|time<5)&active_dot.immolate<=4&target.time_to_die>18" );
  aoe->add_action( "call_action_list,name=ogcd" );
  aoe->add_action( "summon_infernal,if=cooldown.invoke_power_infusion_0.up|cooldown.invoke_power_infusion_0.duration=0|fight_remains>=120" );
  aoe->add_action( "rain_of_fire,if=debuff.pyrogenics.down&active_enemies<=4" );
  aoe->add_action( "channel_demonfire,if=dot.immolate.remains+dot.wither.remains>cast_time" );
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
  cleave->add_action( "rain_of_fire,if=variable.pooling_condition&!talent.wither&buff.rain_of_chaos.up&dbc.effect.33883.sp_coefficient>0.2" );
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
  havoc->add_action( "channel_demonfire,if=soul_shard<4.5&talent.raging_demonfire.rank=2" );
  havoc->add_action( "cataclysm,if=raid_event.adds.in>15|(talent.wither&variable.min_wither<action.wither.duration*0.3)" );
  havoc->add_action( "immolate,target_if=min:dot.immolate.remains+100*debuff.havoc.remains,if=(((dot.immolate.refreshable&variable.havoc_immo_time<5.4)&target.time_to_die>5)|((dot.immolate.remains<2&dot.immolate.remains<havoc_remains)|!dot.immolate.ticking|variable.havoc_immo_time<2)&target.time_to_die>11)&soul_shard<4.5" );
  havoc->add_action( "wither,target_if=min:dot.wither.remains+100*debuff.havoc.remains,if=(((dot.wither.refreshable&variable.havoc_immo_time<5.4)&target.time_to_die>5)|((dot.wither.remains<2&dot.wither.remains<havoc_remains)|!dot.wither.ticking|variable.havoc_immo_time<2)&target.time_to_die>11)&soul_shard<4.5" );
  havoc->add_action( "shadowburn,if=(cooldown.shadowburn.full_recharge_time<=gcd.max*3|debuff.eradication.remains<=gcd.max&talent.eradication&!action.chaos_bolt.in_flight&!talent.diabolic_ritual)&(talent.conflagration_of_chaos|talent.blistering_atrophy)" );
  havoc->add_action( "shadowburn,if=havoc_remains<=gcd.max*3" );
  havoc->add_action( "chaos_bolt,if=cast_time<havoc_remains&(active_enemies<=2-(talent.inferno-talent.improved_chaos_bolt-talent.cataclysm)*talent.wither+(talent.cataclysm&talent.improved_chaos_bolt)*!talent.wither)" );
  havoc->add_action( "rain_of_fire,if=active_enemies>=3-talent.wither" );
  havoc->add_action( "channel_demonfire,if=soul_shard<4.5" );
  havoc->add_action( "conflagrate,if=!talent.backdraft" );
  havoc->add_action( "dimensional_rift,if=soul_shard<4.7&(charges>2|fight_remains<cooldown.dimensional_rift.duration)" );
  havoc->add_action( "incinerate,if=cast_time<havoc_remains" );

  items->add_action( "use_item,use_off_gcd=1,name=belorrelos_the_suncaller,if=((time>20&cooldown.summon_infernal.remains>20)|(trinket.1.is.belorrelos_the_suncaller&(trinket.2.cooldown.remains|!variable.trinket_2_buffs|trinket.1.is.time_thiefs_gambit))|(trinket.2.is.belorrelos_the_suncaller&(trinket.1.cooldown.remains|!variable.trinket_1_buffs|trinket.2.is.time_thiefs_gambit)))&(!raid_event.adds.exists|raid_event.adds.up|spell_targets.belorrelos_the_suncaller>=5)|fight_remains<20" );
  items->add_action( "use_item,name=spymasters_web,if=pet.infernal.remains>=10&pet.infernal.remains<=20&(fight_remains>240|fight_remains<=140)|fight_remains<=30" );
  items->add_action( "use_item,slot=trinket1,if=(variable.infernal_active|!talent.summon_infernal|variable.trinket_1_will_lose_cast)&(variable.trinket_priority=1|variable.trinket_2_exclude|!trinket.2.has_cooldown|(trinket.2.cooldown.remains|variable.trinket_priority=2&cooldown.summon_infernal.remains>20&!variable.infernal_active&trinket.2.cooldown.remains<cooldown.summon_infernal.remains))&variable.trinket_1_buffs&!variable.trinket_1_manual|(variable.trinket_1_buff_duration+1>=fight_remains)" );
  items->add_action( "use_item,slot=trinket2,if=(variable.infernal_active|!talent.summon_infernal|variable.trinket_2_will_lose_cast)&(variable.trinket_priority=2|variable.trinket_1_exclude|!trinket.1.has_cooldown|(trinket.1.cooldown.remains|variable.trinket_priority=1&cooldown.summon_infernal.remains>20&!variable.infernal_active&trinket.1.cooldown.remains<cooldown.summon_infernal.remains))&variable.trinket_2_buffs&!variable.trinket_2_manual|(variable.trinket_2_buff_duration+1>=fight_remains)" );
  items->add_action( "use_item,name=time_thiefs_gambit,if=variable.infernal_active|!talent.summon_infernal|fight_remains<15|((trinket.1.cooldown.duration<cooldown.summon_infernal.remains_expected+5)&active_enemies=1)|(active_enemies>1&havoc_active)" );
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
  variables->add_action( "variable,name=min_wither,default=21,op=reset" );
  variables->add_action( "cycling_variable,name=min_immo,default=21,op=min,value=dot.immolate.remains+(99*!dot.immolate.remains)" );
  variables->add_action( "cycling_variable,name=min_wither,default=21,op=min,value=dot.wither.remains+(99*!dot.wither.remains)" );
}
//destruction_apl_end

} // namespace warlock_apl
