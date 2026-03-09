#include "class_modules/apl/warlock.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"
#include "dbc/dbc.hpp"
#include "sim/sim.hpp"

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
  action_priority_list_t* st = p->get_action_priority_list( "st" );
  action_priority_list_t* end_of_fight = p->get_action_priority_list( "end_of_fight" );
  action_priority_list_t* items = p->get_action_priority_list( "items" );
  action_priority_list_t* ogcd = p->get_action_priority_list( "ogcd" );
  action_priority_list_t* variables = p->get_action_priority_list( "variables" );
  action_priority_list_t* soul_harvester = p->get_action_priority_list( "soul_harvester" );
  action_priority_list_t* hellcaller = p->get_action_priority_list( "hellcaller" );
  action_priority_list_t* SH_aoe = p->get_action_priority_list( "SH_aoe" );
  action_priority_list_t* SH_cleave = p->get_action_priority_list( "SH_cleave" );
  action_priority_list_t* HC_aoe = p->get_action_priority_list( "HC_aoe" );
  action_priority_list_t* HC_cleave = p->get_action_priority_list( "HC_cleave" );

  precombat->add_action( "summon_pet" );
  precombat->add_action( "grimoire_of_sacrifice,if=talent.grimoire_of_sacrifice.enabled" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "seed_of_corruption,if=talent.sow_the_seeds&active_enemies>1" );
  precombat->add_action( "haunt" );

  default_->add_action( "call_action_list,name=variables" );
  default_->add_action( "call_action_list,name=end_of_fight" );
  default_->add_action( "call_action_list,name=ogcd" );
  default_->add_action( "call_action_list,name=items" );
  default_->add_action( "call_action_list,name=aoe,if=active_enemies>2" );
  default_->add_action( "call_action_list,name=cleave,if=active_enemies=2" );
  default_->add_action( "call_action_list,name=st,if=active_enemies<2" );
  default_->add_action( "seed_of_corruption,if=talent.nocturnal_yield&active_enemies>1&buff.nightfall.react&(buff.nightfall.react=buff.nightfall.max_stack|buff.nightfall.remains<execute_time*buff.nightfall.max_stack)" );
  default_->add_action( "malefic_grasp,if=pet.darkglare.active&buff.nightfall.react&(buff.nightfall.react=buff.nightfall.max_stack|buff.nightfall.remains<execute_time*buff.nightfall.max_stack)" );
  default_->add_action( "drain_soul,if=buff.nightfall.react&(buff.nightfall.react=buff.nightfall.max_stack|buff.nightfall.remains<execute_time*buff.nightfall.max_stack)" );
  default_->add_action( "shadow_bolt,if=buff.nightfall.react&(buff.nightfall.react=buff.nightfall.max_stack|buff.nightfall.remains<execute_time*buff.nightfall.max_stack)" );
  default_->add_action( "malefic_grasp,chain=1,early_chain_if=buff.nightfall.react,if=pet.darkglare.active" );
  default_->add_action( "drain_soul,chain=1,early_chain_if=buff.nightfall.react,interrupt_if=tick_time>0.5" );
  default_->add_action( "shadow_bolt" );

  st->add_action( "call_action_list,name=soul_harvester,if=talent.demonic_soul" );
  st->add_action( "call_action_list,name=hellcaller,if=talent.wither" );

  cleave->add_action( "call_action_list,name=SH_cleave,if=talent.demonic_soul" );
  cleave->add_action( "call_action_list,name=HC_cleave,if=talent.wither" );

  aoe->add_action( "call_action_list,name=SH_aoe,if=talent.demonic_soul" );
  aoe->add_action( "call_action_list,name=HC_aoe,if=talent.wither" );

  soul_harvester->add_action( "haunt,if=buff.nightfall.react<2" );
  soul_harvester->add_action( "agony,if=!ticking|refreshable" );
  soul_harvester->add_action( "corruption,if=!ticking|refreshable" );
  soul_harvester->add_action( "summon_darkglare,if=soul_shard<3|cooldown.dark_harvest.remains" );
  soul_harvester->add_action( "dark_harvest,if=soul_shard<3&execute_time<(dot.agony.remains<?dot.corruption.remains)&buff.cascading_calamity.remains" );
  soul_harvester->add_action( "malefic_grasp,if=talent.malefic_grasp&pet.darkglare.active&pet.darkglare.remains<gcd" );
  soul_harvester->add_action( "drain_soul,if=buff.nightfall.react>1" );
  soul_harvester->add_action( "shadow_bolt,if=buff.nightfall.react>1" );
  soul_harvester->add_action( "unstable_affliction,if=pet.darkglare.active|soul_shard>1|(talent.shard_instability&buff.shard_instability.react)|buff.cascading_calamity.remains<gcd.max" );

  SH_cleave->add_action( "haunt,if=buff.nightfall.react<2" );
  SH_cleave->add_action( "seed_of_corruption,if=!dot.corruption.ticking&!dot.seed_of_corruption.ticking&!prev.seed_of_corruption&!action.seed_of_corruption.in_flight" );
  SH_cleave->add_action( "dark_harvest" );
  SH_cleave->add_action( "agony,if=refreshable" );
  SH_cleave->add_action( "summon_darkglare" );
  SH_cleave->add_action( "malefic_grasp,if=talent.malefic_grasp&pet.darkglare.active&!talent.patient_zero&!talent.sow_the_seeds&buff.nightfall.react>1" );
  SH_cleave->add_action( "drain_soul,if=!talent.patient_zero&!talent.sow_the_seeds&buff.nightfall.react>1" );
  SH_cleave->add_action( "shadow_bolt,if=!talent.patient_zero&!talent.sow_the_seeds&buff.nightfall.react>1" );
  SH_cleave->add_action( "unstable_affliction,if=pet.darkglare.active|(!talent.patient_zero&!talent.sow_the_seeds)" );
  SH_cleave->add_action( "seed_of_corruption" );

  SH_aoe->add_action( "haunt,if=buff.nightfall.react<2" );
  SH_aoe->add_action( "seed_of_corruption,if=!dot.corruption.ticking&!dot.seed_of_corruption.ticking&!prev.seed_of_corruption&!action.seed_of_corruption.in_flight" );
  SH_aoe->add_action( "dark_harvest" );
  SH_aoe->add_action( "agony,target_if=min:remains,if=active_dot.agony<5&remains<5" );
  SH_aoe->add_action( "summon_darkglare" );
  SH_aoe->add_action( "seed_of_corruption" );
  SH_aoe->add_action( "agony,target_if=min:remains,if=remains<duration*0.5" );

  hellcaller->add_action( "haunt,if=cooldown.haunt.ready" );
  hellcaller->add_action( "agony,if=!ticking|refreshable" );
  hellcaller->add_action( "wither,if=!ticking|refreshable" );
  hellcaller->add_action( "dark_harvest,if=execute_time<(dot.agony.remains<?dot.corruption.remains)" );
  hellcaller->add_action( "agony,if=dot.agony.remains<20&cooldown.summon_darkglare.remains<gcd" );
  hellcaller->add_action( "summon_darkglare" );
  hellcaller->add_action( "malevolence" );
  hellcaller->add_action( "malefic_grasp,if=talent.malefic_grasp&pet.darkglare.active&pet.darkglare.remains<gcd" );
  hellcaller->add_action( "unstable_affliction,if=pet.darkglare.active|buff.malevolence.remains|soul_shard>4|(talent.shard_instability&buff.shard_instability.react)|buff.cascading_calamity.remains<gcd.max" );
  hellcaller->add_action( "drain_soul,if=buff.nightfall.react>1" );
  hellcaller->add_action( "shadow_bolt,if=buff.nightfall.react>1" );

  HC_cleave->add_action( "haunt,if=cooldown.haunt.ready" );
  HC_cleave->add_action( "seed_of_corruption,if=talent.patient_zero&talent.sow_the_seeds&!dot.wither.ticking&!dot.seed_of_corruption.ticking&!prev.seed_of_corruption" );
  HC_cleave->add_action( "wither,target_if=min:remains,if=remains<5&!(action.seed_of_corruption.in_flight|dot.seed_of_corruption.remains>0)&fight_remains>remains+5" );
  HC_cleave->add_action( "agony,if=refreshable" );
  HC_cleave->add_action( "dark_harvest" );
  HC_cleave->add_action( "summon_darkglare" );
  HC_cleave->add_action( "malevolence" );
  HC_cleave->add_action( "malefic_grasp,if=talent.malefic_grasp&pet.darkglare.active&pet.darkglare.remains<gcd" );
  HC_cleave->add_action( "unstable_affliction,if=(pet.darkglare.active|buff.malevolence.remains|soul_shard>4|buff.shard_instability.react|buff.cascading_calamity.remains<gcd.max)&!talent.patient_zero&!talent.sow_the_seeds" );
  HC_cleave->add_action( "seed_of_corruption,if=talent.patient_zero&talent.sow_the_seeds" );
  HC_cleave->add_action( "unstable_affliction,if=buff.shard_instability.react|(talent.cascading_calamity&buff.cascading_calamity.remains<gcd.max)" );
  HC_cleave->add_action( "drain_soul,if=buff.nightfall.react>1" );
  HC_cleave->add_action( "shadow_bolt,if=buff.nightfall.react>1" );

  HC_aoe->add_action( "haunt,if=cooldown.haunt.ready" );
  HC_aoe->add_action( "seed_of_corruption,if=!dot.wither.ticking&!dot.seed_of_corruption.ticking&!prev.seed_of_corruption&!action.seed_of_corruption.in_flight" );
  HC_aoe->add_action( "dark_harvest" );
  HC_aoe->add_action( "agony,target_if=min:remains,if=active_dot.agony<active_enemies&remains<5" );
  HC_aoe->add_action( "summon_darkglare" );
  HC_aoe->add_action( "malevolence" );
  HC_aoe->add_action( "seed_of_corruption" );
  HC_aoe->add_action( "unstable_affliction,if=buff.shard_instability.react" );
  HC_aoe->add_action( "agony,target_if=min:remains,if=remains<duration*0.5" );
  HC_aoe->add_action( "malefic_grasp,if=talent.malefic_grasp&pet.darkglare.active" );
  HC_aoe->add_action( "drain_soul,if=buff.nightfall.react>1" );
  HC_aoe->add_action( "shadow_bolt,if=buff.nightfall.react>1" );

  end_of_fight->add_action( "unstable_affliction,if=soul_shard&fight_remains<8&(!talent.patient_zero&!talent.sow_the_seeds)" );
  end_of_fight->add_action( "drain_soul,if=buff.nightfall.react&fight_remains<5" );
  end_of_fight->add_action( "shadow_bolt,if=buff.nightfall.react&fight_remains<5" );

  ogcd->add_action( "potion,use_off_gcd=1,if=!talent.summon_darkglare|pet.darkglare.active|fight_remains<32" );
  ogcd->add_action( "berserking,use_off_gcd=1,if=!talent.summon_darkglare|pet.darkglare.active|fight_remains<14" );
  ogcd->add_action( "blood_fury,if=!talent.summon_darkglare|pet.darkglare.active|fight_remains<17" );
  ogcd->add_action( "fireblood,if=!talent.summon_darkglare|pet.darkglare.active|fight_remains<10" );
  ogcd->add_action( "ancestral_call,if=!talent.summon_darkglare|pet.darkglare.active|fight_remains<17" );

  items->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=variable.cds_active" );
  items->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=variable.cds_active" );

  variables->add_action( "variable,name=cds_active,op=set,value=!talent.summon_darkglare|cooldown.summon_darkglare.remains>20|pet.darkglare.remains" );
}
//affliction_apl_end

//demonology_apl_start
void demonology( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* items = p->get_action_priority_list( "items" );
  action_priority_list_t* racials = p->get_action_priority_list( "racials" );
  action_priority_list_t* variables = p->get_action_priority_list( "variables" );

  precombat->add_action( "summon_pet" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "power_siphon" );
  precombat->add_action( "demonbolt,if=!buff.power_siphon.up" );
  precombat->add_action( "shadow_bolt" );

  default_->add_action( "potion,if=pet.demonic_tyrant.active" );
  default_->add_action( "call_action_list,name=racials,if=pet.demonic_tyrant.active|fight_remains<22,use_off_gcd=1" );
  default_->add_action( "call_action_list,name=items,use_off_gcd=1" );
  default_->add_action( "invoke_external_buff,name=power_infusion,if=variable.imp_despawn&variable.imp_despawn<time+gcd.max*6+cast_time" );
  default_->add_action( "grimoire_imp_lord" );
  default_->add_action( "grimoire_fel_ravager" );
  default_->add_action( "summon_doomguard" );
  default_->add_action( "call_dreadstalkers" );
  default_->add_action( "summon_demonic_tyrant" );
  default_->add_action( "implosion,if=buff.wild_imps.stack>=6" );
  default_->add_action( "ruination" );
  default_->add_action( "demonbolt,target_if=(!debuff.doom.up),if=soul_shard<4&buff.demonic_core.stack>=3&talent.doom" );
  default_->add_action( "demonbolt,if=soul_shard<4&buff.demonic_core.stack>=3&!talent.doom" );
  default_->add_action( "power_siphon,if=!buff.demonic_core.up" );
  default_->add_action( "infernal_bolt,if=soul_shard<3" );
  default_->add_action( "hand_of_guldan" );
  default_->add_action( "demonbolt,if=soul_shard<4&buff.demonic_core.react" );
  default_->add_action( "shadow_bolt" );
  default_->add_action( "infernal_bolt" );

  items->add_action( "use_item,slot=trinket1" );
  items->add_action( "use_item,slot=trinket2" );

  racials->add_action( "berserking,use_off_gcd=1" );
  racials->add_action( "blood_fury" );
  racials->add_action( "fireblood" );
  racials->add_action( "ancestral_call" );

  variables->add_action( "variable,name=next_tyrant_cd,op=set,value=cooldown.summon_demonic_tyrant.remains_expected" );
  variables->add_action( "variable,name=in_opener,op=set,value=0,if=pet.demonic_tyrant.active" );
  variables->add_action( "variable,name=imp_despawn,op=set,value=2*spell_haste*6+0.58+time,if=prev_gcd.1.hand_of_guldan&buff.dreadstalkers.up&cooldown.summon_demonic_tyrant.remains<13&variable.imp_despawn=0" );
  variables->add_action( "variable,name=imp_despawn,op=set,value=buff.dreadstalkers.remains+time,if=prev_gcd.1.hand_of_guldan&buff.dreadstalkers.up&cooldown.summon_demonic_tyrant.remains<13&variable.imp_despawn=0" );
  variables->add_action( "variable,name=imp_despawn,op=set,value=(variable.imp_despawn>?buff.dreadstalkers.remains+time),if=variable.imp_despawn" );
  variables->add_action( "variable,name=imp_despawn,op=set,value=variable.imp_despawn>?buff.vilefiend.remains+time,if=variable.imp_despawn&buff.vilefiend.up" );
  variables->add_action( "variable,name=imp_despawn,op=set,value=0,if=buff.tyrant.up" );
  variables->add_action( "variable,name=impl,op=set,value=buff.tyrant.down,if=active_enemies>1+(talent.sacrificed_souls.enabled)" );
  variables->add_action( "variable,name=impl,op=set,value=buff.tyrant.remains<6,if=active_enemies>2+(talent.sacrificed_souls.enabled)&active_enemies<5+(talent.sacrificed_souls.enabled)" );
  variables->add_action( "variable,name=impl,op=set,value=buff.tyrant.remains<8,if=active_enemies>4+(talent.sacrificed_souls.enabled)" );
  variables->add_action( "variable,name=pool_cores_for_tyrant,op=set,value=cooldown.summon_demonic_tyrant.remains<20&variable.next_tyrant_cd<20&(buff.demonic_core.stack<=2|!buff.demonic_core.up)&cooldown.summon_vilefiend.remains<gcd.max*8&cooldown.call_dreadstalkers.remains<gcd.max*8" );
  variables->add_action( "variable,name=last_ds,default=0,value=time,if=prev_gcd.1.call_dreadstalkers" );
  variables->add_action( "variable,name=last_ds,value=0,if=buff.tyrant.up" );
  variables->add_action( "variable,name=last_hog,default=0,value=time,if=prev_gcd.1.hand_of_guldan" );
  variables->add_action( "variable,name=last_hog,value=0,if=buff.tyrant.up" );
  variables->add_action( "variable,name=hog_after_ds,value=variable.last_ds>0&variable.last_hog>0&variable.last_hog>variable.last_ds" );
}
//demonology_apl_end

//destruction_apl_start
void destruction( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe_hc = p->get_action_priority_list( "aoe_hc" );
  // action_priority_list_t* cleave = p->get_action_priority_list( "cleave" );
  // action_priority_list_t* havoc = p->get_action_priority_list( "havoc" );
  action_priority_list_t* items = p->get_action_priority_list( "items" );
  action_priority_list_t* ogcd = p->get_action_priority_list( "ogcd" );
  action_priority_list_t* variables = p->get_action_priority_list( "variables" );

  precombat->add_action( "summon_pet" );
  precombat->add_action( "grimoire_of_sacrifice,if=talent.grimoire_of_sacrifice.enabled" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "cataclysm,if=active_enemies>=2&raid_event.adds.in>15" );
  precombat->add_action( "soul_fire" );
  precombat->add_action( "incinerate" );

  default_->add_action( "call_action_list,name=variables" );
  default_->add_action( "call_action_list,name=ogcd" );
  default_->add_action( "call_action_list,name=items" );
  default_->add_action( "call_action_list,name=aoe_hc,if=active_enemies>=2&talent.wither" );
  default_->add_action( "soul_fire,if=soul_shard<=4" );
  default_->add_action( "conflagrate,if=(variable.ritual_length<(action.chaos_bolt.execute_time*0.7+gcd.max))&!buff.backdraft.up" );
  default_->add_action( "chaos_bolt,if=talent.diabolic_ritual&(demonic_art|(variable.ritual_length<action.chaos_bolt.execute_time))" );
  default_->add_action( "summon_infernal" );
  default_->add_action( "malevolence" );
  default_->add_action( "incinerate,if=buff.chaotic_inferno_buff.up&soul_shard<=4.6" );
  default_->add_action( "shadowburn,if=(!demonic_art|target.health.pct<=20)&(buff.fiendish_cruelty.up|talent.conflagration_of_chaos)" );
  default_->add_action( "wither,if=!talent.internal_combustion&(((dot.wither.remains-5*(action.chaos_bolt.in_flight))<dot.wither.duration*0.3)|dot.wither.remains<3)&(!talent.cataclysm|cooldown.cataclysm.remains>dot.wither.remains)&(!talent.soul_fire|cooldown.soul_fire.remains+action.soul_fire.cast_time>(dot.wither.remains))&target.time_to_die>8" );
  default_->add_action( "immolate,if=(((dot.immolate.remains-5*(action.chaos_bolt.in_flight&talent.internal_combustion))<dot.immolate.duration*0.3)|refreshable|(dot.immolate.remains-action.chaos_bolt.execute_time)<5&talent.internal_combustion&action.chaos_bolt.usable)&(!talent.soul_fire|cooldown.soul_fire.remains+action.soul_fire.cast_time>(dot.immolate.remains-5*talent.internal_combustion))&(!talent.cataclysm|cooldown.cataclysm.remains>dot.immolate.remains)&target.time_to_die>8" );
  default_->add_action( "ruination" );
  default_->add_action( "conflagrate,if=soul_shard<=4.4&buff.backdraft.stack<2" );
  default_->add_action( "cataclysm,if=talent.lake_of_fire" );
  default_->add_action( "chaos_bolt,if=talent.wither" );
  default_->add_action( "chaos_bolt,if=((variable.ritual_length>(action.chaos_bolt.execute_time+gcd.max))&variable.ritual_length<(2+2*action.chaos_bolt.execute_time))&soul_shard>=4" );
  default_->add_action( "chaos_bolt,if=variable.ritual_length>(2*action.chaos_bolt.execute_time+2)" );
  default_->add_action( "infernal_bolt,if=soul_shard<=3" );
  default_->add_action( "channel_demonfire" );
  default_->add_action( "incinerate" );

  aoe_hc->add_action( "summon_infernal" );
  aoe_hc->add_action( "malevolence" );
  aoe_hc->add_action( "rain_of_fire,if=(soul_shard>=(4.0-0.1*(active_dot.wither)))&active_enemies>=4" );
  aoe_hc->add_action( "conflagrate,target_if=max:(dot.wither.remains-99*debuff.havoc.remains),if=dot_refreshable_count.wither>0&!dot.wither.refreshable" );
  aoe_hc->add_action( "shadowburn,target_if=min:(time_to_die+999*debuff.havoc.remains),if=buff.malevolence.up||buff.fiendish_cruelty.up|active_enemies<=3|(talent.conflagration_of_chaos&((active_enemies<=5&talent.destructive_rapidity)|(active_enemies<=6&!talent.destructive_rapidity)))" );
  aoe_hc->add_action( "cataclysm,if=raid_event.adds.in>15" );
  aoe_hc->add_action( "havoc,target_if=min:((-target.time_to_die)<?-15)+dot.wither.remains+99*(self.target=target),if=(!cooldown.summon_infernal.up|!talent.summon_infernal)&target.time_to_die>8&(cooldown.malevolence.remains>15|!talent.malevolence)|time<5" );
  aoe_hc->add_action( "rain_of_fire,if=active_enemies>=4" );
  aoe_hc->add_action( "chaos_bolt,if=active_enemies<=(3+(havoc_active*!talent.destructive_rapidity))" );
  aoe_hc->add_action( "soul_fire,target_if=min:(dot.wither.remains+100*debuff.havoc.remains),if=soul_shard<4&(active_enemies<=8|talent.avatar_of_destruction)" );
  aoe_hc->add_action( "wither,target_if=min:dot.wither.remains+99*debuff.havoc.remains,if=dot.wither.refreshable&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>dot.wither.remains)&active_dot.wither<=active_enemies&target.time_to_die>8" );
  aoe_hc->add_action( "incinerate,if=talent.fire_and_brimstone&buff.backdraft.up" );
  aoe_hc->add_action( "conflagrate,target_if=max:(dot.wither.remains-99*debuff.havoc.remains),if=buff.backdraft.stack<2|!talent.backdraft" );
  aoe_hc->add_action( "incinerate" );

  items->add_action( "use_item,slot=trinket1" );
  items->add_action( "use_item,slot=trinket2" );

  ogcd->add_action( "potion,if=variable.infernal_active|!talent.summon_infernal" );
  ogcd->add_action( "berserking,use_off_gcd=1" );
  ogcd->add_action( "blood_fury" );
  ogcd->add_action( "fireblood" );
  ogcd->add_action( "ancestral_call" );


  variables->add_action( "variable,name=infernal_active,op=set,value=pet.infernal.active|(cooldown.summon_infernal.duration-cooldown.summon_infernal.remains)<20" );
  variables->add_action( "variable,name=ritual_length,value=buff.diabolic_ritual_mother_of_chaos.remains+buff.diabolic_ritual_overlord.remains+buff.diabolic_ritual_pit_lord.remains,default=0,op=set" );
}
//destruction_apl_end

} // namespace warlock_apl
