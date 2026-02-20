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
  // action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  // action_priority_list_t* cleave = p->get_action_priority_list( "cleave" );
  // action_priority_list_t* end_of_fight = p->get_action_priority_list( "end_of_fight" );
  action_priority_list_t* items = p->get_action_priority_list( "items" );
  action_priority_list_t* ogcd = p->get_action_priority_list( "ogcd" );
  // action_priority_list_t* variables = p->get_action_priority_list( "variables" );

  precombat->add_action( "summon_pet" );
  precombat->add_action( "grimoire_of_sacrifice,if=talent.grimoire_of_sacrifice.enabled" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "haunt" );

  default_->add_action( "call_action_list,name=ogcd,if=pet.darkglare.active" );
  default_->add_action( "call_action_list,name=items,if=pet.darkglare.active" );
  default_->add_action( "malevolence,if=!dot.haunt.refreshable&!dot.agony.refreshable&!dot.wither.refreshable" );
  default_->add_action( "summon_darkglare,if=dot.agony.ticking&(dot.corruption.ticking|dot.wither.ticking)" );
  default_->add_action( "dark_harvest,if=talent.dark_harvest&!talent.haunt|!dot.haunt.refreshable&!dot.agony.refreshable&((talent.absolute_corruption&(talent.wither&dot.wither.ticking|!talent.wither&dot.corruption.ticking))|(!talent.absolute_corruption&(talent.wither&dot.wither.refreshable|!talent.wither&dot.corruption.refreshable)))" );
  default_->add_action( "seed_of_corruption,if=active_enemies>=2&soul_shard>1" );
  default_->add_action( "unstable_affliction,if=active_enemies<2&soul_shard>1" );
  default_->add_action( "haunt,if=dot.haunt.refreshable" );
  default_->add_action( "agony,if=dot.agony.refreshable" );
  default_->add_action( "wither,if=talent.wither&(!talent.absolute_corruption&dot.wither.refreshable|talent.absolute_corruption&!dot.wither.ticking)" );
  default_->add_action( "corruption,if=!talent.wither&(!talent.absolute_corruption&dot.corruption.refreshable|talent.absolute_corruption&!dot.corruption.ticking)" );
  default_->add_action( "malefic_grasp,if=pet.darkglare.active" );
  default_->add_action( "drain_soul,interrupt=1" );
  default_->add_action( "shadow_bolt" );

  // aoe->add_action( "call_action_list,name=ogcd" );
  // aoe->add_action( "call_action_list,name=items" );
  // aoe->add_action( "call_action_list,name=end_of_fight" );
  // aoe->add_action( "cycling_variable,name=min_agony,op=min,value=dot.agony.remains+(99*!dot.agony.remains)" );
  // aoe->add_action( "haunt,if=debuff.haunt.remains<3" );
  // aoe->add_action( "agony,if=refreshable&active_enemies>10" );
  // aoe->add_action( "agony,target_if=(!(debuff.haunt.remains|dot.seed_of_corruption.remains)&refreshable),if=active_enemies>8&active_dot.agony<(active_enemies-8>?(talent.demonic_soul*1+!talent.demonic_soul*5))" );
  // aoe->add_action( "agony,cycle_targets=1,max_cycle_targets=5,if=!talent.demonic_soul&remains>0&remains<10&fight_remains>dot.agony.remains+5" );
  // aoe->add_action( "agony,cycle_targets=1,max_cycle_targets=5,if=!talent.demonic_soul&active_dot.agony<6&(remains<3)&fight_remains>dot.agony.remains+5" );
  // aoe->add_action( "agony,cycle_targets=1,max_cycle_targets=3,if=talent.demonic_soul&remains>0&remains<10&fight_remains>dot.agony.remains+5" );
  // aoe->add_action( "agony,cycle_targets=1,max_cycle_targets=3,if=talent.demonic_soul&active_dot.agony<4&(remains<3)&fight_remains>dot.agony.remains+5" );
  // aoe->add_action( "unstable_affliction,if=(remains<3|talent.demonic_soul)&fight_remains>remains+5" );
  // aoe->add_action( "dark_harvest,if=talent.dark_harvest" );
  // aoe->add_action( "seed_of_corruption,if=((dot.corruption.remains<?dot.wither.remains)<8&(dot.wither.remains<?dot.corruption.remains<15))&!(action.seed_of_corruption.in_flight|dot.seed_of_corruption.remains>0)" );
  // aoe->add_action( "summon_darkglare,if=cooldown.invoke_power_infusion_0.duration>0&cooldown.invoke_power_infusion_0.up" );
  // aoe->add_action( "malevolence" );
  // aoe->add_action( "agony,target_if=min:remains,if=remains<duration*0.5&active_dot.agony<6" );
  // aoe->add_action( "wither,target_if=min:(remains*(remains>0)),if=!talent.absolute_corruption&refreshable&!(action.seed_of_corruption.in_flight|dot.seed_of_corruption.remains>0)" );
  // aoe->add_action( "corruption,target_if=min:(remains*(remains>0)),if=!talent.absolute_corruption&refreshable&!(action.seed_of_corruption.in_flight|dot.seed_of_corruption.remains>0)" );
  // aoe->add_action( "unstable_affliction,if=remains<duration*0.3&fight_remains>remains+5" );
  // aoe->add_action( "malefic_grasp,if=talent.summon_darkglare&talent.malefic_grasp" );
  // aoe->add_action( "drain_soul,if=talent.drain_soul" );
  // aoe->add_action( "shadow_bolt" );

  // cleave->add_action( "call_action_list,name=ogcd" );
  // cleave->add_action( "call_action_list,name=items" );
  // cleave->add_action( "call_action_list,name=end_of_fight" );
  // cleave->add_action( "agony,target_if=min:remains,if=(talent.absolute_corruption&remains<3|!talent.absolute_corruption&remains<5)&fight_remains>dot.agony.remains+5" );
  // cleave->add_action( "wither,target_if=min:remains,if=(talent.wither&!talent.absolute_corruption&remains<5)&fight_remains>dot.wither.remains+5" );
  // cleave->add_action( "corruption,target_if=min:remains,if=!talent.wither&(!talent.absolute_corruption&remains<5)&!(action.seed_of_corruption.in_flight|dot.seed_of_corruption.remains>0)&fight_remains>dot.corruption.remains+5" );
  // cleave->add_action( "haunt,if=talent.demonic_soul&buff.nightfall.react<2-prev_gcd.1.drain_soul|debuff.haunt.remains<3" );
  // cleave->add_action( "unstable_affliction,if=(remains<5|talent.demonic_soul)&fight_remains>remains+5" );
  // cleave->add_action( "dark_harvest,if=talent.dark_harvest" );
  // cleave->add_action( "summon_darkglare" );
  // cleave->add_action( "malevolence" );
  // cleave->add_action( "drain_soul,if=talent.demonic_soul&buff.nightfall.react&target.health.pct<20" );
  // cleave->add_action( "agony,if=refreshable" );
  // cleave->add_action( "wither,if=refreshable" );
  // cleave->add_action( "unstable_affliction,if=refreshable" );
  // cleave->add_action( "drain_soul,if=buff.nightfall.react" );
  // cleave->add_action( "shadow_bolt,if=buff.nightfall.react" );
  // cleave->add_action( "wither,if=refreshable" );
  // cleave->add_action( "corruption,if=refreshable" );
  // cleave->add_action( "drain_soul,chain=1,early_chain_if=buff.nightfall.react,interrupt_if=tick_time>0.5" );
  // cleave->add_action( "shadow_bolt" );

  // end_of_fight->add_action( "drain_soul,if=talent.demonic_soul&active_enemies<4&(fight_remains<5&buff.nightfall.react|prev_gcd.1.haunt&buff.nightfall.react=2)" );

  items->add_action( "use_item,slot=trinket1" );
  items->add_action( "use_item,slot=trinket2" );

  ogcd->add_action( "potion,if=pet.darkglare.active" );
  ogcd->add_action( "berserking,use_off_gcd=1" );
  ogcd->add_action( "blood_fury" );
  ogcd->add_action( "fireblood" );
  ogcd->add_action( "ancestral_call" );

  // variables->add_action( "variable,name=cds_active,op=set,value=(!talent.summon_darkglare|pet.darkglare.remains|cooldown.summon_darkglare.remains>20)" );
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
  // action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
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
  default_->add_action( "soul_fire,if=(active_enemies=1)" );
  default_->add_action( "cataclysm" );
  default_->add_action( "wither,if=dot.wither.refreshable" );
  default_->add_action( "immolate,if=dot.immolate.refreshable" );
  default_->add_action( "malevolence" );
  default_->add_action( "summon_infernal" );
  default_->add_action( "ruination" );
  default_->add_action( "infernal_bolt,if=soul_shard<=3" );
  default_->add_action( "rain_of_fire,if=(active_enemies>=3)" );
  default_->add_action( "shadowburn,if=(active_enemies=1)" );
  default_->add_action( "chaos_bolt,if=(active_enemies=1)" );
  default_->add_action( "conflagrate,if=full_recharge_time<=gcd.max*2|recharge_time<=8" );
  default_->add_action( "channel_demonfire" );
  default_->add_action( "conflagrate,if=charges>(max_charges-1)|fight_remains<gcd.max*charges" );
  default_->add_action( "incinerate" );

  // aoe->add_action( "call_action_list,name=ogcd" );
  // aoe->add_action( "call_action_list,name=items" );
  // aoe->add_action( "malevolence,if=cooldown.summon_infernal.remains>=55&soul_shard<4.7&(active_enemies<=3+active_dot.wither|time>30)" );
  // aoe->add_action( "chaos_bolt,if=talent.diabolic_ritual&(((buff.diabolic_ritual_mother_of_chaos.remains+buff.diabolic_ritual_overlord.remains+buff.diabolic_ritual_pit_lord.remains)<(action.chaos_bolt.execute_time)&active_enemies<=9)|(demonic_art&active_enemies<=7))" );
  // aoe->add_action( "call_action_list,name=havoc,if=havoc_active&havoc_remains>gcd.max&active_enemies<=4&talent.wither" );
  // aoe->add_action( "dimensional_rift,if=soul_shard<4.7&(charges>2|fight_remains<cooldown.dimensional_rift.duration)" );
  // aoe->add_action( "incinerate,if=(diabolic_ritual&(buff.diabolic_ritual_mother_of_chaos.remains+buff.diabolic_ritual_overlord.remains+buff.diabolic_ritual_pit_lord.remains)<=action.incinerate.cast_time)" );
  // aoe->add_action( "rain_of_fire,if=(soul_shard>=(3.5-0.1*(active_dot.immolate+active_dot.wither))|buff.ritual_of_ruin.up)&(talent.wither|(talent.diabolic_ritual&active_enemies>=8))" );
  // aoe->add_action( "chaos_bolt,if=soul_shard>=((3.0-0.1*(active_dot.immolate))|buff.ritual_of_ruin.up)&talent.diabolic_ritual&active_enemies<=7" );
  // aoe->add_action( "wither,target_if=min:dot.wither.remains+99*debuff.havoc.remains+99*!dot.wither.ticking,if=dot.wither.refreshable&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>dot.wither.remains)&(!(talent.raging_demonfire&talent.channel_demonfire)|cooldown.channel_demonfire.remains>remains|time<5)&(active_dot.wither<=4|time>15)&target.time_to_die>18" );
  // aoe->add_action( "channel_demonfire,if=dot.immolate.remains+dot.wither.remains>cast_time&talent.raging_demonfire" );
  // aoe->add_action( "shadowburn,if=talent.wither&(buff.malevolence.up|active_enemies<=6|(fight_remains>60&active_enemies<=14))&((cooldown.shadowburn.full_recharge_time<=gcd.max*3|debuff.eradication.remains<=gcd.max&talent.eradication&!action.chaos_bolt.in_flight)|fight_remains<=8)" );
  // aoe->add_action( "shadowburn,target_if=min:time_to_die,if=talent.wither&(buff.malevolence.up|active_enemies<=6|(fight_remains>60&active_enemies<=14))&((cooldown.shadowburn.full_recharge_time<=gcd.max*3|debuff.eradication.remains<=gcd.max&talent.eradication&!action.chaos_bolt.in_flight)|fight_remains<=8)" );
  // aoe->add_action( "ruination" );
  // aoe->add_action( "rain_of_fire,if=pet.infernal.active&talent.rain_of_chaos&(talent.wither|(talent.diabolic_ritual&active_enemies>=8))" );
  // aoe->add_action( "chaos_bolt,if=pet.infernal.active&talent.rain_of_chaos&talent.diabolic_ritual&active_enemies<=7" );
  // aoe->add_action( "soul_fire,target_if=min:dot.wither.remains+dot.immolate.remains-5*debuff.conflagrate.up+100*debuff.havoc.remains,if=(buff.decimation.up)&!talent.raging_demonfire&havoc_active" );
  // aoe->add_action( "soul_fire,target_if=min:(dot.wither.remains+dot.immolate.remains-5*debuff.conflagrate.up+100*debuff.havoc.remains),if=buff.decimation.up&active_dot.immolate<=4" );
  // aoe->add_action( "infernal_bolt,if=soul_shard<2.5" );
  // aoe->add_action( "chaos_bolt,if=((soul_shard>3.0-(0.1*active_enemies))&!action.rain_of_fire.enabled)" );
  // aoe->add_action( "cataclysm,if=raid_event.adds.in>15" );
  // aoe->add_action( "havoc,target_if=min:((-target.time_to_die)<?-15)+dot.immolate.remains+99*(self.target=target),if=(!cooldown.summon_infernal.up|!talent.summon_infernal)&target.time_to_die>8&(cooldown.malevolence.remains>15|!talent.malevolence)|time<5" );
  // aoe->add_action( "wither,target_if=min:dot.wither.remains+99*debuff.havoc.remains,if=dot.wither.refreshable&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>dot.wither.remains)&(!(talent.raging_demonfire&talent.channel_demonfire)|cooldown.channel_demonfire.remains>remains|time<5)&active_dot.wither<=active_enemies&target.time_to_die>18" );
  // aoe->add_action( "immolate,target_if=min:dot.immolate.remains+99*debuff.havoc.remains,if=dot.immolate.refreshable&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>dot.immolate.remains)&(!(talent.raging_demonfire&talent.channel_demonfire)|cooldown.channel_demonfire.remains>remains|time<5)&active_dot.immolate<=6&target.time_to_die>18" );
  // aoe->add_action( "call_action_list,name=ogcd" );
  // aoe->add_action( "summon_infernal,if=cooldown.invoke_power_infusion_0.up|cooldown.invoke_power_infusion_0.duration=0|fight_remains>=120" );
  // aoe->add_action( "rain_of_fire,if=debuff.pyrogenics.down&active_enemies<=4&!talent.diabolic_ritual" );
  // aoe->add_action( "channel_demonfire,if=dot.immolate.remains+dot.wither.remains>cast_time" );
  // aoe->add_action( "immolate,target_if=min:dot.immolate.remains+99*debuff.havoc.remains,if=((dot.immolate.refreshable&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>dot.immolate.remains))|active_enemies>active_dot.immolate)&target.time_to_die>10&!havoc_active" );
  // aoe->add_action( "immolate,target_if=min:dot.immolate.remains+99*debuff.havoc.remains,if=((dot.immolate.refreshable&variable.havoc_immo_time<5.4)|(dot.immolate.remains<2&dot.immolate.remains<havoc_remains)|!dot.immolate.ticking|(variable.havoc_immo_time<2)*havoc_active)&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>dot.immolate.remains)&target.time_to_die>11" );
  // aoe->add_action( "dimensional_rift" );
  // aoe->add_action( "soul_fire,target_if=min:(dot.wither.remains+dot.immolate.remains-5*debuff.conflagrate.up+100*debuff.havoc.remains),if=buff.decimation.up" );
  // aoe->add_action( "incinerate,if=talent.fire_and_brimstone.enabled&buff.backdraft.up" );
  // aoe->add_action( "conflagrate,if=buff.backdraft.stack<2|!talent.backdraft" );
  // aoe->add_action( "incinerate" );

  // cleave->add_action( "call_action_list,name=items" );
  // cleave->add_action( "call_action_list,name=ogcd" );
  // cleave->add_action( "call_action_list,name=havoc,if=havoc_active&havoc_remains>gcd.max" );
  // cleave->add_action( "variable,name=pool_soul_shards,value=cooldown.havoc.remains<=5|talent.mayhem" );
  // cleave->add_action( "malevolence,if=(!cooldown.summon_infernal.up|!talent.summon_infernal)" );
  // cleave->add_action( "havoc,target_if=min:((-target.time_to_die)<?-15)+dot.immolate.remains+99*(self.target=target),if=(!cooldown.summon_infernal.up|!talent.summon_infernal)&target.time_to_die>8" );
  // cleave->add_action( "chaos_bolt,if=demonic_art" );
  // cleave->add_action( "soul_fire,if=buff.decimation.react&(soul_shard<=4|buff.decimation.remains<=gcd.max*2)&debuff.conflagrate.remains>=execute_time&cooldown.havoc.remains" );
  // cleave->add_action( "wither,target_if=min:dot.wither.remains+99*debuff.havoc.remains,if=talent.internal_combustion&(((dot.wither.remains-5*action.chaos_bolt.in_flight)<dot.wither.duration*0.4)|dot.wither.remains<3|(dot.wither.remains-action.chaos_bolt.execute_time)<5&action.chaos_bolt.usable)&(!talent.soul_fire|cooldown.soul_fire.remains+action.soul_fire.cast_time>(dot.wither.remains-5))&target.time_to_die>8&!action.soul_fire.in_flight_to_target" );
  // cleave->add_action( "wither,target_if=min:dot.wither.remains+99*debuff.havoc.remains,if=!talent.internal_combustion&(((dot.wither.remains-5*(action.chaos_bolt.in_flight))<dot.wither.duration*0.3)|dot.wither.remains<3)&(!talent.soul_fire|cooldown.soul_fire.remains+action.soul_fire.cast_time>(dot.wither.remains))&target.time_to_die>8&!action.soul_fire.in_flight_to_target" );
  // cleave->add_action( "conflagrate,if=(talent.roaring_blaze.enabled&full_recharge_time<=gcd.max*2)|recharge_time<=8&(diabolic_ritual&(buff.diabolic_ritual_mother_of_chaos.remains+buff.diabolic_ritual_overlord.remains+buff.diabolic_ritual_pit_lord.remains)<gcd.max)&!variable.pool_soul_shards" );
  // cleave->add_action( "shadowburn,if=(cooldown.shadowburn.full_recharge_time<=gcd.max*3|debuff.eradication.remains<=gcd.max&talent.eradication&!action.chaos_bolt.in_flight&!talent.diabolic_ritual)&(talent.conflagration_of_chaos|talent.blistering_atrophy)|fight_remains<=8" );
  // cleave->add_action( "shadowburn,if=cooldown.summon_infernal.remains>=90&talent.rain_of_chaos&!talent.diabolic_ritual" );
  // cleave->add_action( "chaos_bolt,if=cooldown.summon_infernal.remains>=90&talent.rain_of_chaos" );
  // cleave->add_action( "ruination,if=(debuff.eradication.remains>=execute_time|!talent.eradication|!talent.shadowburn)" );
  // cleave->add_action( "cataclysm,if=raid_event.adds.in>15" );
  // cleave->add_action( "channel_demonfire,if=talent.raging_demonfire&(dot.immolate.remains+dot.wither.remains-5*(action.chaos_bolt.in_flight&talent.internal_combustion))>cast_time" );
  // cleave->add_action( "soul_fire,if=soul_shard<=3.5&(debuff.conflagrate.remains>cast_time+travel_time|!talent.roaring_blaze&buff.backdraft.up)&!variable.pool_soul_shards" );
  // cleave->add_action( "immolate,target_if=min:dot.immolate.remains+99*debuff.havoc.remains,if=(dot.immolate.refreshable&(dot.immolate.remains<cooldown.havoc.remains|!dot.immolate.ticking))&(!talent.cataclysm|cooldown.cataclysm.remains>remains)&(!talent.soul_fire|cooldown.soul_fire.remains+(!talent.mayhem*action.soul_fire.cast_time)>dot.immolate.remains)&target.time_to_die>15" );
  // cleave->add_action( "summon_infernal" );
  // cleave->add_action( "incinerate,if=talent.diabolic_ritual&(diabolic_ritual&(buff.diabolic_ritual_mother_of_chaos.remains+buff.diabolic_ritual_overlord.remains+buff.diabolic_ritual_pit_lord.remains-2-!variable.disable_cb_2t*action.chaos_bolt.cast_time-variable.disable_cb_2t*gcd.max)<=0)" );
  // cleave->add_action( "soul_fire,if=soul_shard<=4&talent.mayhem" );
  // cleave->add_action( "chaos_bolt,if=(cooldown.summon_infernal.remains>=gcd.max*3|soul_shard>4|!talent.rain_of_chaos)" );
  // cleave->add_action( "channel_demonfire" );
  // cleave->add_action( "dimensional_rift" );
  // cleave->add_action( "infernal_bolt" );
  // cleave->add_action( "conflagrate,if=charges>(max_charges-1)|fight_remains<gcd.max*charges" );
  // cleave->add_action( "incinerate" );

  // havoc->add_action( "conflagrate,if=talent.backdraft&buff.backdraft.down&soul_shard>=1&soul_shard<=4" );
  // havoc->add_action( "soul_fire,if=cast_time<havoc_remains&soul_shard<2.5" );
  // havoc->add_action( "cataclysm,if=raid_event.adds.in>15|(talent.wither&dot.wither.remains<action.wither.duration*0.3)" );
  // havoc->add_action( "immolate,target_if=min:dot.immolate.remains+100*debuff.havoc.remains,if=(((dot.immolate.refreshable&variable.havoc_immo_time<5.4)&target.time_to_die>5)|((dot.immolate.remains<2&dot.immolate.remains<havoc_remains)|!dot.immolate.ticking|variable.havoc_immo_time<2)&target.time_to_die>11)&soul_shard<4.5" );
  // havoc->add_action( "wither,target_if=min:dot.wither.remains+100*debuff.havoc.remains,if=(((dot.wither.refreshable&variable.havoc_immo_time<5.4)&target.time_to_die>5)|((dot.wither.remains<2&dot.wither.remains<havoc_remains)|!dot.wither.ticking|variable.havoc_immo_time<2)&target.time_to_die>11)&soul_shard<4.5" );
  // havoc->add_action( "shadowburn,if=(cooldown.shadowburn.full_recharge_time<=gcd.max*3|debuff.eradication.remains<=gcd.max&talent.eradication&!action.chaos_bolt.in_flight)&(talent.conflagration_of_chaos|talent.blistering_atrophy)" );
  // havoc->add_action( "shadowburn,if=havoc_remains<=gcd.max*3" );
  // havoc->add_action( "chaos_bolt,if=cast_time<havoc_remains&((!talent.improved_chaos_bolt&active_enemies<=2)|(talent.improved_chaos_bolt&active_enemies<=4))" );
  // havoc->add_action( "rain_of_fire,if=active_enemies>=3" );
  // havoc->add_action( "channel_demonfire,if=soul_shard<4.5" );
  // havoc->add_action( "conflagrate,if=!talent.backdraft" );
  // havoc->add_action( "dimensional_rift,if=soul_shard<4.7&(charges>2|fight_remains<cooldown.dimensional_rift.duration)" );
  // havoc->add_action( "incinerate,if=cast_time<havoc_remains" );

  items->add_action( "use_item,slot=trinket1" );
  items->add_action( "use_item,slot=trinket2" );

  ogcd->add_action( "potion,if=variable.infernal_active|!talent.summon_infernal" );
  ogcd->add_action( "berserking,use_off_gcd=1" );
  ogcd->add_action( "blood_fury" );
  ogcd->add_action( "fireblood" );
  ogcd->add_action( "ancestral_call" );

  // variables->add_action( "variable,name=havoc_immo_time,op=reset" );
  // variables->add_action( "variable,name=pooling_condition,value=(soul_shard>=3|(talent.secrets_of_the_coven&buff.infernal_bolt.up|buff.decimation.up)&soul_shard>=3),default=1,op=set" );
  // variables->add_action( "variable,name=pooling_condition_cb,value=variable.pooling_condition|pet.infernal.active&soul_shard>=3,default=1,op=set" );
  // variables->add_action( "cycling_variable,name=havoc_immo_time,op=add,value=dot.immolate.remains*debuff.havoc.up<?dot.wither.remains*debuff.havoc.up" );
  variables->add_action( "variable,name=infernal_active,op=set,value=pet.infernal.active|(cooldown.summon_infernal.duration-cooldown.summon_infernal.remains)<20" );
  // variables->add_action( "variable,name=trinket_1_will_lose_cast,value=((floor((fight_remains%trinket.1.cooldown.duration)+1)!=floor((fight_remains+(cooldown.summon_infernal.duration-cooldown.summon_infernal.remains))%cooldown.summon_infernal.duration))&(floor((fight_remains%trinket.1.cooldown.duration)+1))!=(floor(((fight_remains-cooldown.summon_infernal.remains)%trinket.1.cooldown.duration)+1))|((floor((fight_remains%trinket.1.cooldown.duration)+1)=floor((fight_remains+(cooldown.summon_infernal.duration-cooldown.summon_infernal.remains))%cooldown.summon_infernal.duration))&(((fight_remains-cooldown.summon_infernal.remains%%trinket.1.cooldown.duration)-cooldown.summon_infernal.remains-variable.trinket_1_buff_duration)>0)))&cooldown.summon_infernal.remains>20" );
  // variables->add_action( "variable,name=trinket_2_will_lose_cast,value=((floor((fight_remains%trinket.2.cooldown.duration)+1)!=floor((fight_remains+(cooldown.summon_infernal.duration-cooldown.summon_infernal.remains))%cooldown.summon_infernal.duration))&(floor((fight_remains%trinket.2.cooldown.duration)+1))!=(floor(((fight_remains-cooldown.summon_infernal.remains)%trinket.2.cooldown.duration)+1))|((floor((fight_remains%trinket.2.cooldown.duration)+1)=floor((fight_remains+(cooldown.summon_infernal.duration-cooldown.summon_infernal.remains))%cooldown.summon_infernal.duration))&(((fight_remains-cooldown.summon_infernal.remains%%trinket.2.cooldown.duration)-cooldown.summon_infernal.remains-variable.trinket_2_buff_duration)>0)))&cooldown.summon_infernal.remains>20" );
}
//destruction_apl_end

} // namespace warlock_apl
