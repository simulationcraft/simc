#include "class_modules/apl/warlock.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace warlock_apl{
  std::string potion( const player_t* p )
  {
    return ( p->true_level >= 60 ) ? "spectral_intellect" : "disabled";
  }

  std::string flask( const player_t* p )
  {
    return ( p->true_level >= 60 ) ? "spectral_flask_of_power" : "disabled";
  }

  std::string food( const player_t* p )
  {
    return ( p->true_level >= 60 ) ? "feast_of_gluttonous_hedonism" : "disabled";
  }

  std::string rune( const player_t* p )
  {
    return ( p->true_level >= 60 ) ? "veiled" : "disabled";
  }

  std::string temporary_enchant( const player_t* p )
  {
    return ( p->true_level >= 60 ) ? "main_hand:shadowcore_oil" : "disabled";
  }

//affliction_apl_start
void affliction( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "summon_pet" );
  precombat->add_action( "grimoire_of_sacrifice,if=talent.grimoire_of_sacrifice.enabled" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "unstable_affliction,if=!talent.soul_swap" );
  precombat->add_action( "shadow_bolt" );

  default_->add_action( "malefic_rapture,if=soul_shard=5" );
  default_->add_action( "haunt" );
  default_->add_action( "soul_swap,if=dot.unstable_affliction.remains<5" );
  default_->add_action( "unstable_affliction,if=remains<5" );
  default_->add_action( "agony,if=remains<5" );
  default_->add_action( "siphon_life,if=remains<5" );
  default_->add_action( "corruption,if=dot.corruption_dot.remains<5" );
  default_->add_action( "soul_tap,line_cd=30" );
  default_->add_action( "phantom_singularity" );
  default_->add_action( "vile_taint" );
  default_->add_action( "soul_rot" );
  default_->add_action( "use_items" );
  default_->add_action( "summon_darkglare" );
  default_->add_action( "malefic_rapture" );
  default_->add_action( "agony,if=refreshable" );
  default_->add_action( "corruption,if=dot.corruption_dot.refreshable" );
  default_->add_action( "drain_soul,interrupt=1" );
  default_->add_action( "shadow_bolt" );
}
//affliction_apl_end

//demonology_apl_start
void demonology( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* tyrant = p->get_action_priority_list( "tyrant" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "summon_pet" );
  precombat->add_action( "grimoire_of_sacrifice,if=talent.grimoire_of_sacrifice.enabled" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=tyrant_prep_start,op=set,value=12" );
  precombat->add_action( "demonbolt" );

  default_->add_action( "call_action_list,name=tyrant,if=time<15&talent.summon_demonic_tyrant&cooldown.summon_demonic_tyrant.up" );
  default_->add_action( "call_action_list,name=tyrant,if=talent.summon_demonic_tyrant&cooldown.summon_demonic_tyrant.remains_expected<variable.tyrant_prep_start" );
  default_->add_action( "nether_portal,if=!talent.summon_demonic_tyrant&soul_shard>2" );
  default_->add_action( "call_dreadstalkers,if=cooldown.summon_demonic_tyrant.remains_expected>cooldown+variable.tyrant_prep_start" );
  default_->add_action( "grimoire_felguard,if=!talent.summon_demonic_tyrant" );
  default_->add_action( "summon_vilefiend,if=!talent.summon_demonic_tyrant|cooldown.summon_demonic_tyrant.remains_expected>cooldown+variable.tyrant_prep_start" );
  default_->add_action( "use_items,if=!talent.summon_demonic_tyrant|buff.demonic_power.up" );
  default_->add_action( "guillotine" );
  default_->add_action( "demonic_strength" );
  default_->add_action( "power_siphon" );
  default_->add_action( "demonbolt,if=buff.demonic_core.up&soul_shard<4" );
  default_->add_action( "hand_of_guldan,if=soul_shard>2" );
  default_->add_action( "doom,if=refreshable" );
  default_->add_action( "soul_strike" );
  default_->add_action( "shadow_bolt" );

  tyrant->add_action( "nether_portal" );
  tyrant->add_action( "grimoire_felguard" );
  tyrant->add_action( "summon_vilefiend" );
  tyrant->add_action( "call_dreadstalkers" );
  tyrant->add_action( "hand_of_guldan,if=buff.nether_portal.up|soul_shard>2" );
  tyrant->add_action( "summon_demonic_tyrant,if=buff.dreadstalkers.remains<3" );
  tyrant->add_action( "demonbolt,if=buff.demonic_core.up" );
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
  precombat->add_action( "cataclysm" );
  precombat->add_action( "incinerate" );

  default_->add_action( "call_action_list,name=cleave,if=active_enemies=2+(!talent.inferno&talent.madness_of_the_azjaqir&talent.ashen_remains)|variable.cleave_apl" );
  default_->add_action( "call_action_list,name=aoe,if=active_enemies>=3" );
  default_->add_action( "conflagrate,if=(talent.roaring_blaze&debuff.conflagrate.remains<1.5)|charges=max_charges" );
  default_->add_action( "dimensional_rift,if=soul_shard<4.7&(charges>2|time_to_die<cooldown.dimensional_rift.duration)" );
  default_->add_action( "cataclysm" );
  default_->add_action( "channel_demonfire,if=talent.raging_demonfire" );
  default_->add_action( "soul_fire,if=soul_shard<=4" );
  default_->add_action( "immolate,if=((dot.immolate.refreshable&talent.internal_combustion)|dot.immolate.remains<3)&(!talent.cataclysm|cooldown.cataclysm.remains>dot.immolate.remains)&(!talent.soul_fire|cooldown.soul_fire.remains>dot.immolate.remains)" );
  default_->add_action( "chaos_bolt,if=pet.infernal.active|pet.blasphemy.active|soul_shard>=4" );
  default_->add_action( "call_action_list,name=items" );
  default_->add_action( "call_action_list,name=ogcd" );
  default_->add_action( "summon_infernal" );
  default_->add_action( "channel_demonfire,if=talent.ruin.rank>1&!(talent.diabolic_embers&talent.avatar_of_destruction&(talent.burn_to_ashes|talent.chaos_incarnate))" );
  default_->add_action( "conflagrate,if=buff.backdraft.down&soul_shard>=1.5" );
  default_->add_action( "chaos_bolt,if=buff.rain_of_chaos.remains>cast_time" );
  default_->add_action( "chaos_bolt,if=buff.backdraft.up" );
  default_->add_action( "chaos_bolt,if=talent.eradication&debuff.eradication.remains<cast_time&!action.chaos_bolt.in_flight" );
  default_->add_action( "chaos_bolt,if=buff.madness_cb.up" );
  default_->add_action( "channel_demonfire,if=!(talent.diabolic_embers&talent.avatar_of_destruction&(talent.burn_to_ashes|talent.chaos_incarnate))" );
  default_->add_action( "dimensional_rift" );
  default_->add_action( "chaos_bolt,if=soul_shard>3.5" );
  default_->add_action( "chaos_bolt,if=talent.soul_conduit&!talent.madness_of_the_azjaqir|!talent.backdraft" );
  default_->add_action( "chaos_bolt,if=time_to_die<5&time_to_die>cast_time+travel_time" );
  default_->add_action( "conflagrate,if=charges>(max_charges-1)|time_to_die<gcd*charges" );
  default_->add_action( "incinerate" );

  aoe->add_action( "call_action_list,name=havoc,if=havoc_active&havoc_remains>gcd&active_enemies<5" );
  aoe->add_action( "rain_of_fire,if=pet.infernal.active" );
  aoe->add_action( "rain_of_fire,if=talent.avatar_of_destruction" );
  aoe->add_action( "rain_of_fire,if=soul_shard=5" );
  aoe->add_action( "chaos_bolt,if=soul_shard>3.5-(0.1*active_enemies)&!talent.rain_of_fire" );
  aoe->add_action( "cataclysm" );
  aoe->add_action( "channel_demonfire,if=dot.immolate.remains>cast_time&(talent.raging_demonfire.rank+talent.roaring_blaze.rank)>1" );
  aoe->add_action( "immolate,cycle_targets=1,if=dot.immolate.remains<5&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>dot.immolate.remains)&active_dot.immolate<=6" );
  aoe->add_action( "havoc,cycle_targets=1,if=!(self.target=target)&!talent.rain_of_fire" );
  aoe->add_action( "call_action_list,name=items" );
  aoe->add_action( "summon_soulkeeper,if=buff.tormented_soul.stack=10" );
  aoe->add_action( "call_action_list,name=ogcd" );
  aoe->add_action( "summon_infernal" );
  aoe->add_action( "rain_of_fire" );
  aoe->add_action( "havoc,cycle_targets=1,if=!(self.target=target)" );
  aoe->add_action( "immolate,cycle_targets=1,if=dot.immolate.remains<5&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>dot.immolate.remains)" );
  aoe->add_action( "soul_fire,if=buff.backdraft.up" );
  aoe->add_action( "incinerate,if=talent.fire_and_brimstone.enabled&buff.backdraft.up" );
  aoe->add_action( "conflagrate,if=buff.backdraft.down|!talent.backdraft" );
  aoe->add_action( "dimensional_rift" );
  aoe->add_action( "immolate,if=dot.immolate.refreshable" );
  aoe->add_action( "incinerate" );

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
  cleave->add_action( "call_action_list,name=items" );
  cleave->add_action( "call_action_list,name=ogcd" );
  cleave->add_action( "summon_infernal" );
  cleave->add_action( "channel_demonfire,if=talent.ruin.rank>1&!(talent.diabolic_embers&talent.avatar_of_destruction&(talent.burn_to_ashes|talent.chaos_incarnate))" );
  cleave->add_action( "conflagrate,if=buff.backdraft.down&soul_shard>=1.5&!variable.pool_soul_shards" );
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
  havoc->add_action( "chaos_bolt,if=cast_time<havoc_remains&(active_enemies<4-talent.inferno+talent.madness_of_the_azjaqir+(!talent.inferno&talent.ashen_remains))" );
  havoc->add_action( "rain_of_fire,if=(active_enemies>=4-talent.inferno+talent.madness_of_the_azjaqir+(!talent.inferno&talent.ashen_remains))" );
  havoc->add_action( "conflagrate,if=!talent.backdraft" );
  havoc->add_action( "incinerate,if=cast_time<havoc_remains" );

  items->add_action( "use_items,if=pet.infernal.active|!talent.summon_infernal|time_to_die<21" );

  ogcd->add_action( "potion,if=pet.infernal.active|!talent.summon_infernal" );
  ogcd->add_action( "berserking,if=pet.infernal.active|!talent.summon_infernal" );
  ogcd->add_action( "blood_fury,if=pet.infernal.active|!talent.summon_infernal" );
  ogcd->add_action( "fireblood,if=pet.infernal.active|!talent.summon_infernal" );
}
//destruction_apl_end

} // namespace warlock_apl