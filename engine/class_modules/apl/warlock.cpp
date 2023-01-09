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
    if ( p->true_level >= 70 ) return "phial_of_static_empowerment_3";
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
  action_priority_list_t* cleave = p->get_action_priority_list( "cleave" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* ogcd = p->get_action_priority_list( "ogcd" );
  action_priority_list_t* items = p->get_action_priority_list( "items" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "summon_pet" );
  precombat->add_action( "variable,name=cleave_apl,default=0,op=reset" );
  precombat->add_action( "grimoire_of_sacrifice,if=talent.grimoire_of_sacrifice.enabled" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "inquisitors_gaze" );
  precombat->add_action( "seed_of_corruption,if=spell_targets.seed_of_corruption_aoe>3" );
  precombat->add_action( "haunt" );
  precombat->add_action( "unstable_affliction,if=!talent.soul_swap" );
  precombat->add_action( "shadow_bolt" );

  default_->add_action( "call_action_list,name=cleave,if=active_enemies!=1&active_enemies<4|variable.cleave_apl" );
  default_->add_action( "call_action_list,name=aoe,if=active_enemies>3" );
  default_->add_action( "call_action_list,name=ogcd" );
  default_->add_action( "call_action_list,name=items" );
  default_->add_action( "malefic_rapture,if=soul_shard=5" );
  default_->add_action( "haunt" );
  default_->add_action( "soul_swap,if=dot.unstable_affliction.remains<5" );
  default_->add_action( "unstable_affliction,if=remains<5" );
  default_->add_action( "agony,if=remains<5" );
  default_->add_action( "siphon_life,if=remains<5" );
  default_->add_action( "corruption,if=remains<5" );
  default_->add_action( "soul_tap" );
  default_->add_action( "phantom_singularity" );
  default_->add_action( "vile_taint" );
  default_->add_action( "soul_rot" );
  default_->add_action( "summon_darkglare" );
  default_->add_action( "malefic_rapture,if=talent.malefic_affliction&buff.malefic_affliction.stack<3" );
  default_->add_action( "malefic_rapture,if=talent.dread_touch&debuff.dread_touch.remains<gcd" );
  default_->add_action( "malefic_rapture,if=!talent.dread_touch&buff.tormented_crescendo.up" );
  default_->add_action( "malefic_rapture,if=!talent.dread_touch&(dot.soul_rot.remains>cast_time|dot.phantom_singularity.remains>cast_time|dot.vile_taint.remains>cast_time|pet.darkglare.active)" );
  default_->add_action( "drain_soul,if=buff.nightfall.react|talent.shadow_embrace&(debuff.shadow_embrace.stack<3|debuff.shadow_embrace.remains<2)" );
  default_->add_action( "shadow_bolt,if=buff.nightfall.react|talent.shadow_embrace&(debuff.shadow_embrace.stack<3|debuff.shadow_embrace.remains<2)" );
  default_->add_action( "drain_life,if=buff.inevitable_demise.stack>48|buff.inevitable_demise.stack>20&time_to_die<4" );
  default_->add_action( "agony,if=refreshable" );
  default_->add_action( "corruption,if=refreshable" );
  default_->add_action( "drain_soul,interrupt=1" );
  default_->add_action( "shadow_bolt" );

  cleave->add_action( "call_action_list,name=ogcd" );
  cleave->add_action( "call_action_list,name=items" );
  cleave->add_action( "malefic_rapture,if=soul_shard=5" );
  cleave->add_action( "haunt" );
  cleave->add_action( "soul_swap,if=dot.unstable_affliction.remains<5" );
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
  cleave->add_action( "malefic_rapture,if=!talent.dread_touch&(dot.soul_rot.remains>cast_time|dot.phantom_singularity.remains>cast_time|dot.vile_taint.remains>cast_time|pet.darkglare.active)" );
  cleave->add_action( "drain_soul,if=buff.nightfall.react" );
  cleave->add_action( "shadow_bolt,if=buff.nightfall.react" );
  cleave->add_action( "drain_life,if=buff.inevitable_demise.stack>48|buff.inevitable_demise.stack>20&time_to_die<4" );
  cleave->add_action( "drain_life,if=buff.soul_rot.up&buff.inevitable_demise.stack>10" );
  cleave->add_action( "agony,target_if=refreshable" );
  cleave->add_action( "corruption,target_if=refreshable" );
  cleave->add_action( "drain_soul,interrupt_global=1" );
  cleave->add_action( "shadow_bolt" );

  aoe->add_action( "call_action_list,name=ogcd" );
  aoe->add_action( "call_action_list,name=items" );
  aoe->add_action( "haunt" );
  aoe->add_action( "vile_taint" );
  aoe->add_action( "phantom_singularity" );
  aoe->add_action( "soul_rot" );
  aoe->add_action( "seed_of_corruption,if=dot.corruption.remains<5" );
  aoe->add_action( "agony,target_if=remains<5,if=active_dot.agony<5" );
  aoe->add_action( "summon_darkglare" );
  aoe->add_action( "seed_of_corruption,if=talent.sow_the_seeds" );
  aoe->add_action( "malefic_rapture" );
  aoe->add_action( "drain_life,if=(buff.soul_rot.up|!talent.soul_rot)&buff.inevitable_demise.stack>10" );
  aoe->add_action( "summon_soulkeeper,if=buff.tormented_soul.stack=10" );
  aoe->add_action( "siphon_life,target_if=remains<5,if=active_dot.siphon_life<3" );
  aoe->add_action( "drain_soul,interrupt_global=1" );
  aoe->add_action( "shadow_bolt" );

  ogcd->add_action( "potion,if=pet.darkglare.active|dot.soul_rot.ticking|!talent.summon_darkglare&!talent.soul_rot" );
  ogcd->add_action( "berserking,if=pet.darkglare.active|dot.soul_rot.ticking|!talent.summon_darkglare&!talent.soul_rot" );
  ogcd->add_action( "blood_fury,if=pet.darkglare.active|dot.soul_rot.ticking|!talent.summon_darkglare&!talent.soul_rot" );
  ogcd->add_action( "fireblood,if=pet.darkglare.active|dot.soul_rot.ticking|!talent.summon_darkglare&!talent.soul_rot" );

  items->add_action( "use_items,if=pet.darkglare.active|dot.soul_rot.ticking|!talent.summon_darkglare&!talent.soul_rot|time_to_die<21" );
}
//affliction_apl_end

//demonology_apl_start
void demonology( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* tyrant = p->get_action_priority_list( "tyrant" );
  action_priority_list_t* ogcd = p->get_action_priority_list( "ogcd" );
  action_priority_list_t* items = p->get_action_priority_list( "items" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "summon_pet" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "inquisitors_gaze" );
  precombat->add_action( "variable,name=tyrant_prep_start,op=set,value=12" );

  precombat->add_action( "variable,name=next_tyrant,op=set,value=14+talent.grimoire_felguard+talent.summon_vilefiend" );
  precombat->add_action( "power_siphon" );
  precombat->add_action( "demonbolt,if=!buff.power_siphon.up" );
  precombat->add_action( "shadow_bolt" );

  default_->add_action( "call_action_list,name=tyrant,if=talent.summon_demonic_tyrant&(time-variable.next_tyrant)<=(variable.tyrant_prep_start+2)&cooldown.summon_demonic_tyrant.up" );
  default_->add_action( "call_action_list,name=tyrant,if=talent.summon_demonic_tyrant&cooldown.summon_demonic_tyrant.remains_expected<=variable.tyrant_prep_start" );
  default_->add_action( "implosion,if=time_to_die<2*gcd" );
  default_->add_action( "nether_portal,if=!talent.summon_demonic_tyrant&soul_shard>2|time_to_die<30" );
  default_->add_action( "hand_of_guldan,if=buff.nether_portal.up" );
  default_->add_action( "call_action_list,name=items" );
  default_->add_action( "call_action_list,name=ogcd,if=buff.demonic_power.up|!talent.summon_demonic_tyrant&(buff.nether_portal.up|!talent.nether_portal)" );
  default_->add_action( "call_dreadstalkers,if=cooldown.summon_demonic_tyrant.remains_expected>cooldown" );
  default_->add_action( "call_dreadstalkers,if=!talent.summon_demonic_tyrant|time_to_die<14" );
  default_->add_action( "grimoire_felguard,if=!talent.summon_demonic_tyrant|time_to_die<cooldown.summon_demonic_tyrant.remains_expected" );
  default_->add_action( "summon_vilefiend,if=!talent.summon_demonic_tyrant|cooldown.summon_demonic_tyrant.remains_expected>cooldown+variable.tyrant_prep_start|time_to_die<cooldown.summon_demonic_tyrant.remains_expected" );
  default_->add_action( "guillotine,if=cooldown.demonic_strength.remains" );
  default_->add_action( "demonic_strength" );
  default_->add_action( "bilescourge_bombers,if=!pet.demonic_tyrant.active" );
  default_->add_action( "shadow_bolt,if=soul_shard<5&talent.fel_covenant&buff.fel_covenant.remains<5" );
  default_->add_action( "implosion,if=two_cast_imps>0&buff.tyrant.down&active_enemies>1+(talent.sacrificed_souls.enabled)" );
  default_->add_action( "implosion,if=buff.wild_imps.stack>9&buff.tyrant.up&active_enemies>2+(1*talent.sacrificed_souls.enabled)&cooldown.call_dreadstalkers.remains>17&talent.the_expendables" );
  default_->add_action( "implosion,if=active_enemies=1&last_cast_imps>0&buff.tyrant.down&talent.imp_gang_boss.enabled&!talent.sacrificed_souls" );
  default_->add_action( "soul_strike,if=soul_shard<5&active_enemies>1" );
  default_->add_action( "summon_soulkeeper,if=active_enemies>1&buff.tormented_soul.stack=10" );
  default_->add_action( "demonbolt,if=buff.demonic_core.up&soul_shard<4" );
  default_->add_action( "power_siphon,if=buff.demonic_core.stack<1&(buff.dreadstalkers.remains>3|buff.dreadstalkers.down)" );
  default_->add_action( "hand_of_guldan,if=soul_shard>2&(!talent.summon_demonic_tyrant|cooldown.summon_demonic_tyrant.remains_expected>variable.tyrant_prep_start+2)" );
  default_->add_action( "doom,target_if=refreshable" );
  default_->add_action( "soul_strike,if=soul_shard<5" );
  default_->add_action( "shadow_bolt" );

  items->add_action( "use_item,name=timebreaching_talon,if=buff.demonic_power.up|!talent.summon_demonic_tyrant&(buff.nether_portal.up|!talent.nether_portal)" );
  items->add_action( "use_items" );

  ogcd->add_action( "potion" );
  ogcd->add_action( "berserking" );
  ogcd->add_action( "blood_fury" );
  ogcd->add_action( "fireblood" );

  tyrant->add_action( "variable,name=next_tyrant,op=set,value=time+14+cooldown.grimoire_felguard.ready+cooldown.summon_vilefiend.ready,if=variable.next_tyrant<=time" );
  tyrant->add_action( "shadow_bolt,if=time<2&soul_shard<5" );
  tyrant->add_action( "nether_portal" );
  tyrant->add_action( "grimoire_felguard" );
  tyrant->add_action( "summon_vilefiend" );
  tyrant->add_action( "call_dreadstalkers" );
  tyrant->add_action( "soulburn,if=buff.nether_portal.up&soul_shard>=2,line_cd=40" );
  tyrant->add_action( "hand_of_guldan,if=variable.next_tyrant-time>2&(buff.nether_portal.up|soul_shard>2&variable.next_tyrant-time<12|soul_shard=5)" );
  tyrant->add_action( "hand_of_guldan,if=talent.soulbound_tyrant&variable.next_tyrant-time<4&variable.next_tyrant-time>action.summon_demonic_tyrant.cast_time" );
  tyrant->add_action( "summon_demonic_tyrant,if=variable.next_tyrant-time<cast_time*2" );
  tyrant->add_action( "demonbolt,if=buff.demonic_core.up" );
  tyrant->add_action( "power_siphon,if=buff.wild_imps.stack>1&!buff.nether_portal.up" );
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
  precombat->add_action( "inquisitors_gaze" );
  precombat->add_action( "soul_fire" );
  precombat->add_action( "cataclysm" );
  precombat->add_action( "incinerate" );

  default_->add_action( "call_action_list,name=cleave,if=active_enemies!=1&active_enemies<=2+(!talent.inferno&talent.madness_of_the_azjaqir&talent.ashen_remains)|variable.cleave_apl" );
  default_->add_action( "call_action_list,name=aoe,if=active_enemies>=3" );
  default_->add_action( "call_action_list,name=items" );
  default_->add_action( "call_action_list,name=ogcd" );
  default_->add_action( "conflagrate,if=(talent.roaring_blaze&debuff.conflagrate.remains<1.5)|charges=max_charges" );
  default_->add_action( "dimensional_rift,if=soul_shard<4.7&(charges>2|time_to_die<cooldown.dimensional_rift.duration)" );
  default_->add_action( "cataclysm" );
  default_->add_action( "channel_demonfire,if=talent.raging_demonfire" );
  default_->add_action( "soul_fire,if=soul_shard<=4" );
  default_->add_action( "immolate,if=((dot.immolate.refreshable&talent.internal_combustion)|dot.immolate.remains<3)&(!talent.cataclysm|cooldown.cataclysm.remains>dot.immolate.remains)&(!talent.soul_fire|cooldown.soul_fire.remains>dot.immolate.remains)" );
  default_->add_action( "havoc,if=talent.cry_havoc&(buff.ritual_of_ruin.up|pet.infernal.active)" );
  default_->add_action( "chaos_bolt,if=pet.infernal.active|pet.blasphemy.active|soul_shard>=4" );
  default_->add_action( "summon_infernal" );
  default_->add_action( "channel_demonfire,if=talent.ruin.rank>1&!(talent.diabolic_embers&talent.avatar_of_destruction&(talent.burn_to_ashes|talent.chaos_incarnate))&dot.immolate.remains>cast_time" );
  default_->add_action( "conflagrate,if=buff.backdraft.down&soul_shard>=1.5&!talent.roaring_blaze" );
  default_->add_action( "incinerate,if=buff.burn_to_ashes.up&cast_time+action.chaos_bolt.cast_time<buff.madness_cb.remains" );
  default_->add_action( "chaos_bolt,if=buff.rain_of_chaos.remains>cast_time" );
  default_->add_action( "chaos_bolt,if=buff.backdraft.up&!talent.eradication&!talent.madness_of_the_azjaqir" );
  default_->add_action( "chaos_bolt,if=buff.madness_cb.up" );
  default_->add_action( "channel_demonfire,if=!(talent.diabolic_embers&talent.avatar_of_destruction&(talent.burn_to_ashes|talent.chaos_incarnate))&dot.immolate.remains>cast_time" );
  default_->add_action( "dimensional_rift" );
  default_->add_action( "chaos_bolt,if=soul_shard>3.5" );
  default_->add_action( "chaos_bolt,if=talent.soul_conduit&!talent.madness_of_the_azjaqir|!talent.backdraft" );
  default_->add_action( "chaos_bolt,if=time_to_die<5&time_to_die>cast_time+travel_time" );
  default_->add_action( "conflagrate,if=charges>(max_charges-1)|time_to_die<gcd*charges" );
  default_->add_action( "incinerate" );

  aoe->add_action( "call_action_list,name=ogcd" );
  aoe->add_action( "call_action_list,name=items" );
  aoe->add_action( "call_action_list,name=havoc,if=havoc_active&havoc_remains>gcd&active_enemies<5+(talent.cry_havoc&!talent.inferno)" );
  aoe->add_action( "rain_of_fire,if=pet.infernal.active" );
  aoe->add_action( "rain_of_fire,if=talent.avatar_of_destruction" );
  aoe->add_action( "rain_of_fire,if=soul_shard=5" );
  aoe->add_action( "chaos_bolt,if=soul_shard>3.5-(0.1*active_enemies)&!talent.rain_of_fire" );
  aoe->add_action( "cataclysm" );
  aoe->add_action( "channel_demonfire,if=dot.immolate.remains>cast_time&talent.raging_demonfire" );
  aoe->add_action( "immolate,cycle_targets=1,if=dot.immolate.remains<5&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>dot.immolate.remains)&(!talent.raging_demonfire|cooldown.channel_demonfire.remains>remains)&active_dot.immolate<=6" );
  aoe->add_action( "havoc,cycle_targets=1,if=!(self.target=target)&!talent.rain_of_fire" );
  aoe->add_action( "summon_soulkeeper,if=buff.tormented_soul.stack=10" );
  aoe->add_action( "call_action_list,name=ogcd" );
  aoe->add_action( "summon_infernal" );
  aoe->add_action( "rain_of_fire" );
  aoe->add_action( "havoc,cycle_targets=1,if=!(self.target=target)" );
  aoe->add_action( "channel_demonfire,if=dot.immolate.remains>cast_time" );
  aoe->add_action( "immolate,cycle_targets=1,if=dot.immolate.remains<5&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>dot.immolate.remains)" );
  aoe->add_action( "soul_fire,if=buff.backdraft.up" );
  aoe->add_action( "incinerate,if=talent.fire_and_brimstone.enabled&buff.backdraft.up" );
  aoe->add_action( "conflagrate,if=buff.backdraft.down|!talent.backdraft" );
  aoe->add_action( "dimensional_rift" );
  aoe->add_action( "immolate,if=dot.immolate.refreshable" );
  aoe->add_action( "incinerate" );

  cleave->add_action( "call_action_list,name=items" );
  cleave->add_action( "call_action_list,name=ogcd" );
  cleave->add_action( "call_action_list,name=havoc,if=havoc_active&havoc_remains>gcd" );
  cleave->add_action( "variable,name=pool_soul_shards,value=cooldown.havoc.remains<=10|talent.mayhem" );
  cleave->add_action( "conflagrate,if=(talent.roaring_blaze.enabled&debuff.conflagrate.remains<1.5)|charges=max_charges" );
  cleave->add_action( "dimensional_rift,if=soul_shard<4.7&(charges>2|time_to_die<cooldown.dimensional_rift.duration)" );
  cleave->add_action( "cataclysm" );
  cleave->add_action( "channel_demonfire,if=talent.raging_demonfire" );
  cleave->add_action( "soul_fire,if=soul_shard<=4&!variable.pool_soul_shards" );
  cleave->add_action( "immolate,cycle_targets=1,if=((talent.internal_combustion&dot.immolate.refreshable)|dot.immolate.remains<3)&(!talent.cataclysm|cooldown.cataclysm.remains>remains)&(!talent.soul_fire|cooldown.soul_fire.remains>remains)" );
  cleave->add_action( "havoc,cycle_targets=1,if=!(target=self.target)&(!cooldown.summon_infernal.up|!talent.summon_infernal)" );
  cleave->add_action( "chaos_bolt,if=pet.infernal.active|pet.blasphemy.active|soul_shard>=4" );
  cleave->add_action( "summon_infernal" );
  cleave->add_action( "channel_demonfire,if=talent.ruin.rank>1&!(talent.diabolic_embers&talent.avatar_of_destruction&(talent.burn_to_ashes|talent.chaos_incarnate))" );
  cleave->add_action( "conflagrate,if=buff.backdraft.down&soul_shard>=1.5&!variable.pool_soul_shards" );
  cleave->add_action( "incinerate,if=buff.burn_to_ashes.up&cast_time+action.chaos_bolt.cast_time<buff.madness_cb.remains" );
  cleave->add_action( "chaos_bolt,if=buff.rain_of_chaos.remains>cast_time" );
  cleave->add_action( "chaos_bolt,if=buff.backdraft.up&!variable.pool_soul_shards" );
  cleave->add_action( "chaos_bolt,if=talent.eradication&!variable.pool_soul_shards&debuff.eradication.remains<cast_time&!action.chaos_bolt.in_flight" );
  cleave->add_action( "chaos_bolt,if=buff.madness_cb.up" );
  cleave->add_action( "soul_fire,if=soul_shard<=4&talent.mayhem" );
  cleave->add_action( "channel_demonfire,if=!(talent.diabolic_embers&talent.avatar_of_destruction&(talent.burn_to_ashes|talent.chaos_incarnate))" );
  cleave->add_action( "dimensional_rift" );
  cleave->add_action( "chaos_bolt,if=soul_shard>3.5" );
  cleave->add_action( "chaos_bolt,if=!variable.pool_soul_shards&(talent.soul_conduit&!talent.madness_of_the_azjaqir|!talent.backdraft)" );
  cleave->add_action( "chaos_bolt,if=time_to_die<5&time_to_die>cast_time+travel_time" );
  cleave->add_action( "conflagrate,if=charges>(max_charges-1)|time_to_die<gcd*charges" );
  cleave->add_action( "incinerate" );

  havoc->add_action( "conflagrate,if=talent.backdraft&buff.backdraft.down&soul_shard>=1&soul_shard<=4" );
  havoc->add_action( "soul_fire,if=cast_time<havoc_remains&soul_shard<3" );
  havoc->add_action( "channel_demonfire,if=soul_shard<4.5&talent.raging_demonfire.rank=2&active_enemies>2" );
  havoc->add_action( "immolate,cycle_targets=1,if=dot.immolate.refreshable&dot.immolate.remains<havoc_remains&soul_shard<4.5&(debuff.havoc.down|!dot.immolate.ticking)" );
  havoc->add_action( "chaos_bolt,if=talent.cry_havoc&!talent.inferno&cast_time<havoc_remains" );
  havoc->add_action( "chaos_bolt,if=cast_time<havoc_remains&(active_enemies<4-talent.inferno+talent.madness_of_the_azjaqir-(talent.inferno&(talent.rain_of_chaos|talent.avatar_of_destruction)&buff.rain_of_chaos.up))" );
  havoc->add_action( "rain_of_fire,if=(active_enemies>=4-talent.inferno+talent.madness_of_the_azjaqir)" );
  havoc->add_action( "rain_of_fire,if=active_enemies>1&(talent.avatar_of_destruction|(talent.rain_of_chaos&buff.rain_of_chaos.up))&talent.inferno.enabled" );
  havoc->add_action( "conflagrate,if=!talent.backdraft" );
  havoc->add_action( "incinerate,if=cast_time<havoc_remains" );

  items->add_action( "use_item,slot=trinket1,if=pet.infernal.active|!talent.summon_infernal|time_to_die<21|trinket.1.cooldown.duration<cooldown.summon_infernal.remains" );
  items->add_action( "use_item,slot=trinket2,if=pet.infernal.active|!talent.summon_infernal|time_to_die<21|trinket.2.cooldown.duration<cooldown.summon_infernal.remains" );
  items->add_action( "use_item,slot=trinket1,if=(!talent.rain_of_chaos&time_to_die<cooldown.summon_infernal.remains+trinket.1.cooldown.duration&time_to_die>trinket.1.cooldown.duration)|time_to_die<cooldown.summon_infernal.remains|(trinket.2.cooldown.remains>0&trinket.2.cooldown.remains<cooldown.summon_infernal.remains)", "We might use trinket outside of infernal to not lose a use if we don't have RoC" );
  items->add_action( "use_item,slot=trinket2,if=(!talent.rain_of_chaos&time_to_die<cooldown.summon_infernal.remains+trinket.2.cooldown.duration&time_to_die>trinket.2.cooldown.duration)|time_to_die<cooldown.summon_infernal.remains|(trinket.1.cooldown.remains>0&trinket.1.cooldown.remains<cooldown.summon_infernal.remains)" );
  items->add_action( "use_item,name=erupting_spear_fragment,if=(!talent.rain_of_chaos&time_to_die<cooldown.summon_infernal.remains+trinket.erupting_spear_fragment.cooldown.duration&time_to_die>trinket.erupting_spear_fragment.cooldown.duration)|time_to_die<cooldown.summon_infernal.remains|trinket.erupting_spear_fragment.cooldown.duration<cooldown.summon_infernal.remains+5" );
  items->add_action( "use_item,name=desperate_invokers_codex" );
  items->add_action( "use_item,name=iceblood_deathsnare" );
  items->add_action( "use_item,name=conjured_chillglobe" );

  ogcd->add_action( "potion,if=pet.infernal.active|!talent.summon_infernal" );
  ogcd->add_action( "berserking,if=pet.infernal.active|!talent.summon_infernal|(time_to_die<(cooldown.summon_infernal.remains+cooldown.berserking.duration)&(time_to_die>cooldown.berserking.duration))|time_to_die<cooldown.summon_infernal.remains" );
  ogcd->add_action( "blood_fury,if=pet.infernal.active|!talent.summon_infernal|(time_to_die<cooldown.summon_infernal.remains+10+cooldown.blood_fury.duration&time_to_die>cooldown.blood_fury.duration)|time_to_die<cooldown.summon_infernal.remains" );
  ogcd->add_action( "fireblood,if=pet.infernal.active|!talent.summon_infernal|(time_to_die<cooldown.summon_infernal.remains+10+cooldown.fireblood.duration&time_to_die>cooldown.fireblood.duration)|time_to_die<cooldown.summon_infernal.remains" );
}
//destruction_apl_end

} // namespace warlock_apl
