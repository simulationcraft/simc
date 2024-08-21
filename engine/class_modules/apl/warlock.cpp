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
