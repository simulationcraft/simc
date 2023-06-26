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
  action_priority_list_t* variables = p->get_action_priority_list( "variables" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* cleave = p->get_action_priority_list( "cleave" );
  action_priority_list_t* items = p->get_action_priority_list( "items" );
  action_priority_list_t* ogcd = p->get_action_priority_list( "ogcd" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "summon_pet" );
  precombat->add_action( "variable,name=cleave_apl,default=0,op=reset" );
  precombat->add_action( "grimoire_of_sacrifice,if=talent.grimoire_of_sacrifice.enabled" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "seed_of_corruption,if=spell_targets.seed_of_corruption_aoe>3" );
  precombat->add_action( "haunt" );
  precombat->add_action( "unstable_affliction,if=!talent.soul_swap" );
  precombat->add_action( "shadow_bolt" );

  default_->add_action( "call_action_list,name=variables" );
  default_->add_action( "call_action_list,name=cleave,if=active_enemies!=1&active_enemies<4|variable.cleave_apl" );
  default_->add_action( "call_action_list,name=aoe,if=active_enemies>3" );
  default_->add_action( "call_action_list,name=ogcd" );
  default_->add_action( "call_action_list,name=items" );
  default_->add_action( "malefic_rapture,if=talent.dread_touch&talent.malefic_affliction&debuff.dread_touch.remains<2&buff.malefic_affliction.stack=3" );
  default_->add_action( "unstable_affliction,if=remains<5" );
  default_->add_action( "agony,if=remains<5" );
  default_->add_action( "corruption,if=remains<5" );
  default_->add_action( "siphon_life,if=remains<5" );
  default_->add_action( "haunt" );
  default_->add_action( "drain_soul,if=talent.shadow_embrace&(debuff.shadow_embrace.stack<3|debuff.shadow_embrace.remains<3)" );
  default_->add_action( "shadow_bolt,if=talent.shadow_embrace&(debuff.shadow_embrace.stack<3|debuff.shadow_embrace.remains<3)" );
  default_->add_action( "phantom_singularity,if=!talent.soul_rot|cooldown.soul_rot.remains<=execute_time|cooldown.soul_rot.remains>=25" );
  default_->add_action( "vile_taint,if=!talent.soul_rot|cooldown.soul_rot.remains<=execute_time|talent.souleaters_gluttony.rank<2&cooldown.soul_rot.remains>=12" );
  default_->add_action( "soul_rot,if=variable.vt_up&variable.ps_up" );
  default_->add_action( "summon_darkglare,if=variable.ps_up&variable.vt_up&variable.sr_up|cooldown.invoke_power_infusion_0.duration>0&cooldown.invoke_power_infusion_0.up&!talent.soul_rot" );
  default_->add_action( "malefic_rapture,if=soul_shard>4|(talent.tormented_crescendo&buff.tormented_crescendo.stack=1&soul_shard>3)" );
  
  default_->add_action( "malefic_rapture,if=talent.malefic_affliction&buff.malefic_affliction.stack<3" );
  default_->add_action( "malefic_rapture,if=talent.tormented_crescendo&buff.tormented_crescendo.react&!debuff.dread_touch.react" );
  default_->add_action( "malefic_rapture,if=talent.tormented_crescendo&buff.tormented_crescendo.stack=2" );
  default_->add_action( "malefic_rapture,if=variable.cd_dots_up|variable.vt_up&soul_shard>1" );
  default_->add_action( "malefic_rapture,if=talent.tormented_crescendo&talent.nightfall&buff.tormented_crescendo.react&buff.nightfall.react" );
  default_->add_action( "drain_life,if=buff.inevitable_demise.stack>48|buff.inevitable_demise.stack>20&fight_remains<4" );
  default_->add_action( "drain_soul,if=buff.nightfall.react" );
  default_->add_action( "shadow_bolt,if=buff.nightfall.react" );
  default_->add_action( "agony,if=refreshable" );
  default_->add_action( "corruption,if=refreshable" );
  default_->add_action( "drain_soul,interrupt=1" );
  default_->add_action( "shadow_bolt" );

  variables->add_action( "variable,name=ps_up,op=set,value=dot.phantom_singularity.ticking|!talent.phantom_singularity" );
  variables->add_action( "variable,name=vt_up,op=set,value=dot.vile_taint_dot.ticking|!talent.vile_taint" );
  variables->add_action( "variable,name=sr_up,op=set,value=dot.soul_rot.ticking|!talent.soul_rot" );
  variables->add_action( "variable,name=cd_dots_up,op=set,value=variable.ps_up&variable.vt_up&variable.sr_up" );
  variables->add_action( "variable,name=has_cds,op=set,value=talent.phantom_singularity|talent.vile_taint|talent.soul_rot|talent.summon_darkglare" );
  variables->add_action( "variable,name=cds_active,op=set,value=!variable.has_cds|(pet.darkglare.active|variable.cd_dots_up|buff.power_infusion.react)" );

  aoe->add_action( "call_action_list,name=ogcd" );
  aoe->add_action( "call_action_list,name=items" );
  aoe->add_action( "haunt" );
  aoe->add_action( "vile_taint" );
  aoe->add_action( "phantom_singularity" );
  aoe->add_action( "soul_rot" );
  aoe->add_action( "unstable_affliction,if=remains<5" );
  aoe->add_action( "seed_of_corruption,if=dot.corruption.remains<5" );
  aoe->add_action( "malefic_rapture,if=talent.malefic_affliction&buff.malefic_affliction.stack<3&talent.doom_blossom");
  aoe->add_action( "agony,target_if=remains<5,if=active_dot.agony<5" );
  aoe->add_action( "summon_darkglare" );
  aoe->add_action( "seed_of_corruption,if=talent.sow_the_seeds" );
  aoe->add_action( "malefic_rapture" );
  aoe->add_action( "drain_life,if=(buff.soul_rot.up|!talent.soul_rot)&buff.inevitable_demise.stack>10" );
  aoe->add_action( "summon_soulkeeper,if=buff.tormented_soul.stack=10|buff.tormented_soul.stack>3&fight_remains<10" );
  aoe->add_action( "siphon_life,target_if=remains<5,if=active_dot.siphon_life<3" );
  aoe->add_action( "drain_soul,interrupt_global=1" );
  aoe->add_action( "shadow_bolt" );

  cleave->add_action( "call_action_list,name=ogcd" );
  cleave->add_action( "call_action_list,name=items" );
  cleave->add_action( "malefic_rapture,if=soul_shard=5" );
  cleave->add_action( "haunt" );
  cleave->add_action( "unstable_affliction,if=remains<5" );
  cleave->add_action( "agony,if=remains<5" );
  cleave->add_action( "agony,target_if=!(target=self.target)&remains<5" );
  cleave->add_action( "siphon_life,if=remains<5" );
  cleave->add_action( "siphon_life,target_if=!(target=self.target)&remains<3" );
  cleave->add_action( "seed_of_corruption,if=!talent.absolute_corruption&dot.corruption.remains<5" );
  cleave->add_action( "corruption,target_if=remains<5&(talent.absolute_corruption|!talent.seed_of_corruption)" );
  cleave->add_action( "phantom_singularity" );
  cleave->add_action( "vile_taint" );
  cleave->add_action( "soul_rot" );
  cleave->add_action( "summon_darkglare" );
  cleave->add_action( "malefic_rapture,if=talent.malefic_affliction&buff.malefic_affliction.stack<3" );
  cleave->add_action( "malefic_rapture,if=talent.dread_touch&debuff.dread_touch.remains<gcd" );
  cleave->add_action( "malefic_rapture,if=!talent.dread_touch&buff.tormented_crescendo.up" );
  cleave->add_action( "malefic_rapture,if=!talent.dread_touch&(dot.soul_rot.remains>cast_time|dot.phantom_singularity.remains>cast_time|dot.vile_taint_dot.remains>cast_time|pet.darkglare.active)" );
  cleave->add_action( "drain_soul,if=buff.nightfall.react" );
  cleave->add_action( "shadow_bolt,if=buff.nightfall.react" );
  cleave->add_action( "drain_life,if=buff.inevitable_demise.stack>48|buff.inevitable_demise.stack>20&fight_remains<4" );
  cleave->add_action( "drain_life,if=buff.soul_rot.up&buff.inevitable_demise.stack>10" );
  cleave->add_action( "agony,target_if=refreshable" );
  cleave->add_action( "corruption,target_if=refreshable" );
  cleave->add_action( "drain_soul,interrupt_global=1" );
  cleave->add_action( "shadow_bolt" );

  items->add_action( "use_items,if=variable.cds_active" );
  items->add_action( "use_item,name=desperate_invokers_codex" );
  items->add_action( "use_item,name=conjured_chillglobe" );

  ogcd->add_action( "potion,if=variable.cds_active" );
  ogcd->add_action( "berserking,if=variable.cds_active" );
  ogcd->add_action( "blood_fury,if=variable.cds_active" );
  ogcd->add_action( "invoke_external_buff,name=power_infusion,if=variable.cds_active", " Uses Power Infusion together with Cooldown windows like Summon Darkglare, Soul Rot, Phantom Singularity or Vile Taint");
  ogcd->add_action( "fireblood,if=variable.cds_active" );
}
//affliction_apl_end

//demonology_apl_start
void demonology( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* variables = p->get_action_priority_list( "variables" );
  action_priority_list_t* items = p->get_action_priority_list( "items" );
  action_priority_list_t* ogcd = p->get_action_priority_list( "ogcd" );
  action_priority_list_t* tyrant = p->get_action_priority_list( "tyrant" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "summon_pet" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=tyrant_prep_start,op=set,value=12" );
  precombat->add_action( "variable,name=next_tyrant,op=set,value=14+talent.grimoire_felguard+talent.summon_vilefiend" );
  precombat->add_action( "variable,name=shadow_timings,default=0,op=reset" );
  precombat->add_action( "variable,name=shadow_timings,op=set,value=0,if=cooldown.invoke_power_infusion_0.duration!=120" );
  precombat->add_action( "power_siphon" );
  precombat->add_action( "demonbolt,if=!buff.power_siphon.up" );
  precombat->add_action( "shadow_bolt" );

  variables->add_action( "variable,name=tyrant_cd,op=setif,value=cooldown.invoke_power_infusion_0.remains,value_else=cooldown.summon_demonic_tyrant.remains_expected,condition=((((fight_remains+time)%%120<=85&(fight_remains+time)%%120>=25)|time>=210)&variable.shadow_timings)&cooldown.invoke_power_infusion_0.duration>0&!talent.grand_warlocks_design" );
  variables->add_action( "variable,name=np_condition,op=set,value=cooldown.nether_portal.up|buff.nether_portal.up|pet.pit_lord.active|!talent.nether_portal|cooldown.nether_portal.remains>30" );

  default_->add_action( "call_action_list,name=variables" );
  default_->add_action( "call_action_list,name=tyrant,if=talent.summon_demonic_tyrant&(time-variable.next_tyrant)<=(variable.tyrant_prep_start+2)&cooldown.summon_demonic_tyrant.up&variable.np_condition" );
  default_->add_action( "call_action_list,name=tyrant,if=talent.summon_demonic_tyrant&(variable.tyrant_cd<=variable.tyrant_prep_start|cooldown.summon_demonic_tyrant.up&(buff.power_infusion.up|buff.nether_portal.up))&variable.np_condition" );
  default_->add_action( "invoke_external_buff,name=power_infusion,if=!talent.nether_portal&!talent.summon_demonic_tyrant|fight_remains<25|(buff.tyrant.up&variable.shadow_timings)");
  default_->add_action( "implosion,if=fight_remains<2*gcd" );
  default_->add_action( "nether_portal,if=!talent.summon_demonic_tyrant&soul_shard>2|fight_remains<30" );
  default_->add_action( "call_action_list,name=items" );
  default_->add_action( "call_action_list,name=ogcd,if=buff.demonic_power.up|!talent.summon_demonic_tyrant&(buff.nether_portal.up|!talent.nether_portal)" );
  default_->add_action( "hand_of_guldan,if=buff.nether_portal.remains>cast_time" );
  default_->add_action( "call_dreadstalkers,if=variable.tyrant_cd>cooldown+8*variable.shadow_timings" );
  default_->add_action( "call_dreadstalkers,if=!talent.summon_demonic_tyrant|fight_remains<14" );
  default_->add_action( "grimoire_felguard,if=!talent.summon_demonic_tyrant|fight_remains<cooldown.summon_demonic_tyrant.remains_expected" );
  default_->add_action( "summon_vilefiend,if=!talent.summon_demonic_tyrant|variable.tyrant_cd>cooldown+variable.tyrant_prep_start|fight_remains<cooldown.summon_demonic_tyrant.remains_expected" );
  default_->add_action( "guillotine,if=cooldown.demonic_strength.remains|!talent.demonic_strength" );
  default_->add_action( "demonic_strength" );
  default_->add_action( "bilescourge_bombers,if=!pet.demonic_tyrant.active" );
  default_->add_action( "shadow_bolt,if=soul_shard<5&talent.fel_covenant&buff.fel_covenant.remains<5" );
  default_->add_action( "implosion,if=two_cast_imps>0&buff.tyrant.down&active_enemies>1+(talent.sacrificed_souls.enabled)" );
  default_->add_action( "implosion,if=buff.wild_imps.stack>9&buff.tyrant.up&active_enemies>2+(1*talent.sacrificed_souls.enabled)&cooldown.call_dreadstalkers.remains>17&talent.the_expendables" );
  default_->add_action( "soul_strike,if=soul_shard<5&active_enemies>1" );
  default_->add_action( "summon_soulkeeper,if=buff.tormented_soul.stack=10&active_enemies>1" );
  default_->add_action( "demonbolt,if=buff.demonic_core.up&soul_shard<4&variable.tyrant_cd>5" );
  default_->add_action( "power_siphon,if=buff.demonic_core.stack<2&(buff.dreadstalkers.remains>gcd*3|buff.dreadstalkers.down)" );
  default_->add_action( "hand_of_guldan,if=soul_shard>2&(!talent.summon_demonic_tyrant|variable.tyrant_cd>variable.tyrant_prep_start+2)&(buff.demonic_calling.up|soul_shard>4|cooldown.call_dreadstalkers.remains>gcd)" );
  default_->add_action( "doom,target_if=refreshable" );
  default_->add_action( "soul_strike,if=soul_shard<5" );
  default_->add_action( "shadow_bolt" );

  items->add_action( "use_item,name=irideus_fragment,if=buff.demonic_power.up|!talent.summon_demonic_tyrant&(buff.nether_portal.up|!talent.nether_portal)|fight_remains<=21" );
  items->add_action( "use_item,name=timebreaching_talon,if=buff.demonic_power.up|!talent.summon_demonic_tyrant&(buff.nether_portal.up|!talent.nether_portal)|fight_remains<=21" );
  items->add_action( "use_item,name=spoils_of_neltharus,if=buff.demonic_power.up|!talent.summon_demonic_tyrant&(buff.nether_portal.up|!talent.nether_portal)|fight_remains<=21" );
  items->add_action( "use_item,name=voidmenders_shadowgem,if=!variable.shadow_timings|(variable.shadow_timings&(buff.demonic_power.up|!talent.summon_demonic_tyrant&(buff.nether_portal.up|!talent.nether_portal)))" );  
  items->add_action( "use_item,name=erupting_spear_fragment,if=buff.demonic_power.up|!talent.summon_demonic_tyrant&(buff.nether_portal.up|!talent.nether_portal)|fight_remains<=11" );
  items->add_action( "use_items,if=(buff.demonic_power.up|!talent.summon_demonic_tyrant&(buff.nether_portal.up|!talent.nether_portal))&(!equipped.irideus_fragment&!equipped.timebreaching_talon&!equipped.spoils_of_neltharus&!equipped.erupting_spear_fragment&!equipped.voidmenders_shadowgem)" );
  items->add_action( "use_item,name=rotcrusted_voodoo_doll" );
  items->add_action( "use_item,name=beacon_to_the_beyond" );

  ogcd->add_action( "potion" );
  ogcd->add_action( "berserking" );
  ogcd->add_action( "blood_fury" );
  ogcd->add_action( "fireblood" );

  tyrant->add_action( "variable,name=next_tyrant,op=set,value=time+13+cooldown.grimoire_felguard.ready+cooldown.summon_vilefiend.ready,if=variable.next_tyrant<=time&!equipped.neltharions_call_to_dominance" );
  tyrant->add_action( "invoke_external_buff,name=power_infusion,if=(buff.nether_portal.up&buff.nether_portal.remains<8&talent.nether_portal)|(buff.dreadstalkers.up&variable.next_tyrant-time<=6&(!talent.nether_portal|variable.shadow_timings))", "Power Infusion is used 6 seconds or less before Demonic Tyrant, without Demonic Tyrant, it will be used 8s before Nether Portal Ends and without both, its used on Cooldown"  );
  tyrant->add_action( "power_siphon,if=buff.wild_imps.stack>1&!buff.nether_portal.up&buff.demonic_core.stack<3" );
  tyrant->add_action( "shadow_bolt,if=time<2&soul_shard<5" );
  tyrant->add_action( "nether_portal" );
  tyrant->add_action( "variable,name=next_tyrant,op=set,value=time+13+cooldown.grimoire_felguard.ready+cooldown.summon_vilefiend.ready,if=variable.next_tyrant<=time&equipped.neltharions_call_to_dominance" );
  tyrant->add_action( "grimoire_felguard" );
  tyrant->add_action( "summon_vilefiend" );
  tyrant->add_action( "call_dreadstalkers" );
  tyrant->add_action( "soulburn,if=buff.nether_portal.up&soul_shard>=2,line_cd=40" );
  tyrant->add_action( "hand_of_guldan,if=variable.next_tyrant-time>2&(buff.nether_portal.up|soul_shard>2&variable.next_tyrant-time<12|soul_shard=5)&!cooldown.call_dreadstalkers.up" );
  tyrant->add_action( "hand_of_guldan,if=talent.soulbound_tyrant&variable.next_tyrant-time<4&variable.next_tyrant-time>action.summon_demonic_tyrant.cast_time" );
  tyrant->add_action( "summon_demonic_tyrant,if=variable.next_tyrant-time<cast_time*2+1|(buff.dreadstalkers.remains<cast_time+gcd&buff.dreadstalkers.up)" );
  tyrant->add_action( "demonbolt,if=buff.demonic_core.up" );
  tyrant->add_action( "soul_strike" );
  tyrant->add_action( "shadow_bolt" );
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

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "summon_pet" );
  precombat->add_action( "variable,name=cleave_apl,default=0,op=reset" );
  precombat->add_action( "grimoire_of_sacrifice,if=talent.grimoire_of_sacrifice.enabled" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "soul_fire" );
  precombat->add_action( "cataclysm,if=raid_event.adds.in>15" );
  precombat->add_action( "incinerate" );

  default_->add_action( "variable,name=havoc_immo_time,op=reset" );
  default_->add_action( "cycling_variable,name=havoc_immo_time,op=add,value=dot.immolate.remains*debuff.havoc.up" );
  default_->add_action( "call_action_list,name=aoe,if=(active_enemies>=3-(talent.inferno&!talent.madness_of_the_azjaqir))&!(!talent.inferno&talent.madness_of_the_azjaqir&talent.chaos_incarnate&active_enemies<4)&!variable.cleave_apl" );
  default_->add_action( "call_action_list,name=cleave,if=active_enemies!=1|variable.cleave_apl" );
  default_->add_action( "call_action_list,name=items" );
  default_->add_action( "call_action_list,name=ogcd" );
  default_->add_action( "conflagrate,if=(talent.roaring_blaze&debuff.conflagrate.remains<1.5)|charges=max_charges" );
  default_->add_action( "dimensional_rift,if=soul_shard<4.7&(charges>2|fight_remains<cooldown.dimensional_rift.duration)" );
  default_->add_action( "cataclysm,if=raid_event.adds.in>15" );
  default_->add_action( "channel_demonfire,if=talent.raging_demonfire&(dot.immolate.remains-5*(action.chaos_bolt.in_flight&talent.internal_combustion))>cast_time&(debuff.conflagrate.remains>execute_time|!talent.roaring_blaze)" );
  default_->add_action( "soul_fire,if=soul_shard<=3.5&(debuff.conflagrate.remains>cast_time+travel_time|!talent.roaring_blaze&buff.backdraft.up)" );
  default_->add_action( "immolate,if=(((dot.immolate.remains-5*(action.chaos_bolt.in_flight&talent.internal_combustion))<dot.immolate.duration*0.3)|dot.immolate.remains<3|(dot.immolate.remains-action.chaos_bolt.execute_time)<5&talent.infernal_combustion&action.chaos_bolt.usable)&(!talent.cataclysm|cooldown.cataclysm.remains>dot.immolate.remains)&(!talent.soul_fire|cooldown.soul_fire.remains+action.soul_fire.cast_time>(dot.immolate.remains-5*talent.internal_combustion))&target.time_to_die>8" );
  default_->add_action( "channel_demonfire,if=dot.immolate.remains>cast_time&set_bonus.tier30_4pc" );
  default_->add_action( "chaos_bolt,if=pet.infernal.active|pet.blasphemy.active|soul_shard>=4" );
  default_->add_action( "summon_infernal" );
  default_->add_action( "channel_demonfire,if=talent.ruin.rank>1&!(talent.diabolic_embers&talent.avatar_of_destruction&(talent.burn_to_ashes|talent.chaos_incarnate))&dot.immolate.remains>cast_time" );
  default_->add_action( "conflagrate,if=buff.backdraft.down&soul_shard>=1.5&!talent.roaring_blaze" );
  default_->add_action( "incinerate,if=cast_time+action.chaos_bolt.cast_time<buff.madness_cb.remains" );
  default_->add_action( "chaos_bolt,if=buff.rain_of_chaos.remains>cast_time" );
  default_->add_action( "chaos_bolt,if=buff.backdraft.up&!talent.eradication&!talent.madness_of_the_azjaqir" );
  default_->add_action( "chaos_bolt,if=buff.madness_cb.up" );
  default_->add_action( "channel_demonfire,if=!(talent.diabolic_embers&talent.avatar_of_destruction&(talent.burn_to_ashes|talent.chaos_incarnate))&dot.immolate.remains>cast_time" );
  default_->add_action( "dimensional_rift" );
  default_->add_action( "chaos_bolt,if=soul_shard>3.5" );
  default_->add_action( "chaos_bolt,if=talent.soul_conduit&!talent.madness_of_the_azjaqir|!talent.backdraft" );
  default_->add_action( "chaos_bolt,if=fight_remains<5&fight_remains>cast_time+travel_time" );
  default_->add_action( "conflagrate,if=charges>(max_charges-1)|fight_remains<gcd.max*charges" );
  default_->add_action( "incinerate" );

  aoe->add_action( "call_action_list,name=ogcd" );
  aoe->add_action( "call_action_list,name=items" );
  aoe->add_action( "call_action_list,name=havoc,if=havoc_active&havoc_remains>gcd.max&active_enemies<5+(talent.cry_havoc&!talent.inferno)&(!cooldown.summon_infernal.up|!talent.summon_infernal)" );
  aoe->add_action( "rain_of_fire,if=pet.infernal.active|pet.blasphemy.active" );
  aoe->add_action( "rain_of_fire,if=fight_remains<12" );
  aoe->add_action( "rain_of_fire,if=gcd.max>buff.madness_rof.remains&buff.madness_rof.up" );
  aoe->add_action( "rain_of_fire,if=soul_shard>=(4.5-0.1*active_dot.immolate)&time>5" );
  aoe->add_action( "chaos_bolt,if=soul_shard>3.5-(0.1*active_enemies)&!talent.rain_of_fire" );
  aoe->add_action( "cataclysm,if=raid_event.adds.in>15" );
  aoe->add_action( "havoc,target_if=min:((-target.time_to_die)<?-15)+dot.immolate.remains+99*(self.target=target),if=(!cooldown.summon_infernal.up|!talent.summon_infernal|(talent.inferno&active_enemies>4))&target.time_to_die>8" );
  aoe->add_action( "immolate,target_if=min:dot.immolate.remains+99*debuff.havoc.remains,if=dot.immolate.refreshable&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>dot.immolate.remains)&(!talent.raging_demonfire|cooldown.channel_demonfire.remains>remains|time<5)&active_dot.immolate<=4&target.time_to_die>18" );
  aoe->add_action( "channel_demonfire,if=dot.immolate.remains>cast_time&talent.raging_demonfire" );
  aoe->add_action( "summon_soulkeeper,if=buff.tormented_soul.stack=10|buff.tormented_soul.stack>3&fight_remains<10" );
  aoe->add_action( "call_action_list,name=ogcd" );
  aoe->add_action( "summon_infernal" );
  aoe->add_action( "rain_of_fire,if=debuff.pyrogenics.down&active_enemies<=4" );
  aoe->add_action( "channel_demonfire,if=dot.immolate.remains>cast_time" );
  aoe->add_action( "immolate,target_if=min:dot.immolate.remains+99*debuff.havoc.remains,if=((dot.immolate.refreshable&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>dot.immolate.remains))|active_enemies>active_dot.immolate)&target.time_to_die>10&!havoc_active" );
  aoe->add_action( "immolate,target_if=min:dot.immolate.remains+99*debuff.havoc.remains,if=((dot.immolate.refreshable&variable.havoc_immo_time<5.4)|(dot.immolate.remains<2&dot.immolate.remains<havoc_remains)|!dot.immolate.ticking|(variable.havoc_immo_time<2)*havoc_active)&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>dot.immolate.remains)&target.time_to_die>11" );
  aoe->add_action( "soul_fire,if=buff.backdraft.up" );
  aoe->add_action( "incinerate,if=talent.fire_and_brimstone.enabled&buff.backdraft.up" );
  aoe->add_action( "conflagrate,if=buff.backdraft.stack<2|!talent.backdraft" );
  aoe->add_action( "dimensional_rift" );
  aoe->add_action( "incinerate" );

  cleave->add_action( "call_action_list,name=items" );
  cleave->add_action( "call_action_list,name=ogcd" );
  cleave->add_action( "call_action_list,name=havoc,if=havoc_active&havoc_remains>gcd.max" );
  cleave->add_action( "variable,name=pool_soul_shards,value=cooldown.havoc.remains<=10|talent.mayhem" );
  cleave->add_action( "conflagrate,if=(talent.roaring_blaze.enabled&debuff.conflagrate.remains<1.5)|charges=max_charges" );
  cleave->add_action( "dimensional_rift,if=soul_shard<4.7&(charges>2|fight_remains<cooldown.dimensional_rift.duration)" );
  cleave->add_action( "cataclysm,if=raid_event.adds.in>15" );
  cleave->add_action( "channel_demonfire,if=talent.raging_demonfire&active_dot.immolate=2" );
  cleave->add_action( "soul_fire,if=soul_shard<=3.5&(debuff.conflagrate.remains>cast_time+travel_time|!talent.roaring_blaze&buff.backdraft.up)&!variable.pool_soul_shards" );
  cleave->add_action( "immolate,target_if=min:dot.immolate.remains+99*debuff.havoc.remains,if=(dot.immolate.refreshable&(dot.immolate.remains<cooldown.havoc.remains|!dot.immolate.ticking))&(!talent.cataclysm|cooldown.cataclysm.remains>remains)&(!talent.soul_fire|cooldown.soul_fire.remains+(!talent.mayhem*action.soul_fire.cast_time)>dot.immolate.remains)&target.time_to_die>15" );
  cleave->add_action( "havoc,target_if=min:((-target.time_to_die)<?-15)+dot.immolate.remains+99*(self.target=target),if=(!cooldown.summon_infernal.up|!talent.summon_infernal)&target.time_to_die>8" );
  cleave->add_action( "chaos_bolt,if=pet.infernal.active|pet.blasphemy.active|soul_shard>=4" );
  cleave->add_action( "summon_infernal" );
  cleave->add_action( "channel_demonfire,if=talent.ruin.rank>1&!(talent.diabolic_embers&talent.avatar_of_destruction&(talent.burn_to_ashes|talent.chaos_incarnate))" );
  cleave->add_action( "conflagrate,if=buff.backdraft.down&soul_shard>=1.5&!variable.pool_soul_shards" );
  cleave->add_action( "incinerate,if=cast_time+action.chaos_bolt.cast_time<buff.madness_cb.remains" );
  cleave->add_action( "chaos_bolt,if=buff.rain_of_chaos.remains>cast_time" );
  cleave->add_action( "chaos_bolt,if=buff.backdraft.up&!variable.pool_soul_shards" );
  cleave->add_action( "chaos_bolt,if=talent.eradication&!variable.pool_soul_shards&debuff.eradication.remains<cast_time&!action.chaos_bolt.in_flight" );
  cleave->add_action( "chaos_bolt,if=buff.madness_cb.up&(!variable.pool_soul_shards|(talent.burn_to_ashes&buff.burn_to_ashes.stack=0)|talent.soul_fire)" );
  cleave->add_action( "soul_fire,if=soul_shard<=4&talent.mayhem" );
  cleave->add_action( "channel_demonfire,if=!(talent.diabolic_embers&talent.avatar_of_destruction&(talent.burn_to_ashes|talent.chaos_incarnate))" );
  cleave->add_action( "dimensional_rift" );
  cleave->add_action( "chaos_bolt,if=soul_shard>3.5&!variable.pool_soul_shards" );
  cleave->add_action( "chaos_bolt,if=!variable.pool_soul_shards&(talent.soul_conduit&!talent.madness_of_the_azjaqir|!talent.backdraft)" );
  cleave->add_action( "chaos_bolt,if=fight_remains<5&fight_remains>cast_time+travel_time" );
  cleave->add_action( "summon_soulkeeper,if=buff.tormented_soul.stack=10|buff.tormented_soul.stack>3&fight_remains<10" );
  cleave->add_action( "conflagrate,if=charges>(max_charges-1)|fight_remains<gcd.max*charges" );
  cleave->add_action( "incinerate" );

  havoc->add_action( "conflagrate,if=talent.backdraft&buff.backdraft.down&soul_shard>=1&soul_shard<=4" );
  havoc->add_action( "soul_fire,if=cast_time<havoc_remains&soul_shard<2.5" );
  havoc->add_action( "channel_demonfire,if=soul_shard<4.5&talent.raging_demonfire.rank=2" );
  havoc->add_action( "immolate,target_if=min:dot.immolate.remains+100*debuff.havoc.remains,if=(((dot.immolate.refreshable&variable.havoc_immo_time<5.4)&target.time_to_die>5)|((dot.immolate.remains<2&dot.immolate.remains<havoc_remains)|!dot.immolate.ticking|variable.havoc_immo_time<2)&target.time_to_die>11)&soul_shard<4.5" );
  havoc->add_action( "chaos_bolt,if=((talent.cry_havoc&!talent.inferno)|!talent.rain_of_fire)&cast_time<havoc_remains" );
  havoc->add_action( "chaos_bolt,if=cast_time<havoc_remains&(active_enemies<=3-talent.inferno+(talent.madness_of_the_azjaqir&!talent.inferno))" );
  havoc->add_action( "rain_of_fire,if=active_enemies>=3&talent.inferno" );
  havoc->add_action( "rain_of_fire,if=(active_enemies>=4-talent.inferno+talent.madness_of_the_azjaqir)" );
  havoc->add_action( "rain_of_fire,if=active_enemies>2&(talent.avatar_of_destruction|(talent.rain_of_chaos&buff.rain_of_chaos.up))&talent.inferno.enabled" );
  havoc->add_action( "channel_demonfire,if=soul_shard<4.5" );
  havoc->add_action( "conflagrate,if=!talent.backdraft" );
  havoc->add_action( "incinerate,if=cast_time<havoc_remains" );

  items->add_action( "use_item,slot=trinket1,if=pet.infernal.active|!talent.summon_infernal|fight_remains<21|trinket.1.cooldown.duration<cooldown.summon_infernal.remains+5" );
  items->add_action( "use_item,slot=trinket2,if=pet.infernal.active|!talent.summon_infernal|fight_remains<21|trinket.2.cooldown.duration<cooldown.summon_infernal.remains+5" );
  items->add_action( "use_item,slot=trinket1,if=(!talent.rain_of_chaos&fight_remains<cooldown.summon_infernal.remains+trinket.1.cooldown.duration&fight_remains>trinket.1.cooldown.duration)|fight_remains<cooldown.summon_infernal.remains|(trinket.2.cooldown.remains>0&trinket.2.cooldown.remains<cooldown.summon_infernal.remains)" );
  items->add_action( "use_item,slot=trinket2,if=(!talent.rain_of_chaos&fight_remains<cooldown.summon_infernal.remains+trinket.2.cooldown.duration&fight_remains>trinket.2.cooldown.duration)|fight_remains<cooldown.summon_infernal.remains|(trinket.1.cooldown.remains>0&trinket.1.cooldown.remains<cooldown.summon_infernal.remains)" );
  items->add_action( "use_item,name=erupting_spear_fragment,if=(!talent.rain_of_chaos&fight_remains<cooldown.summon_infernal.remains+trinket.erupting_spear_fragment.cooldown.duration&fight_remains>trinket.erupting_spear_fragment.cooldown.duration)|fight_remains<cooldown.summon_infernal.remains|trinket.erupting_spear_fragment.cooldown.duration<cooldown.summon_infernal.remains+5" );
  items->add_action( "use_item,name=desperate_invokers_codex" );
  items->add_action( "use_item,name=iceblood_deathsnare" );
  items->add_action( "use_item,name=conjured_chillglobe" );

  ogcd->add_action( "potion,if=pet.infernal.active|!talent.summon_infernal" );
  ogcd->add_action( "invoke_external_buff,name=power_infusion,if=pet.infernal.active|!talent.summon_infernal|(fight_remains<cooldown.summon_infernal.remains+10+cooldown.invoke_power_infusion_0.duration&fight_remains>cooldown.invoke_power_infusion_0.duration)|fight_remains<cooldown.summon_infernal.remains+15", "Attempts to use Power Infusion alongside Summon Infernal, but it will be used on Cooldown if the fight length is too short or long enough to fit an extra PI" );
  ogcd->add_action( "berserking,if=pet.infernal.active|!talent.summon_infernal|(fight_remains<(cooldown.summon_infernal.remains+cooldown.berserking.duration)&(fight_remains>cooldown.berserking.duration))|fight_remains<cooldown.summon_infernal.remains" );
  ogcd->add_action( "blood_fury,if=pet.infernal.active|!talent.summon_infernal|(fight_remains<cooldown.summon_infernal.remains+10+cooldown.blood_fury.duration&fight_remains>cooldown.blood_fury.duration)|fight_remains<cooldown.summon_infernal.remains" );
  ogcd->add_action( "fireblood,if=pet.infernal.active|!talent.summon_infernal|(fight_remains<cooldown.summon_infernal.remains+10+cooldown.fireblood.duration&fight_remains>cooldown.fireblood.duration)|fight_remains<cooldown.summon_infernal.remains" );
}
//destruction_apl_end

} // namespace warlock_apl
