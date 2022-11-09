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
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* havoc = p->get_action_priority_list( "havoc" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "summon_pet" );
  precombat->add_action( "grimoire_of_sacrifice,if=talent.grimoire_of_sacrifice.enabled" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "soul_fire" );
  precombat->add_action( "cataclysm" );
  precombat->add_action( "incinerate" );

  default_->add_action( "call_action_list,name=havoc,if=havoc_active&active_enemies>1&active_enemies<5-talent.inferno.enabled+(talent.inferno.enabled&talent.internal_combustion.enabled)&havoc_remains>gcd" );
  default_->add_action( "variable,name=pool_soul_shards,value=active_enemies>1&(cooldown.havoc.remains<=10|talent.mayhem)" );
  default_->add_action( "conflagrate,if=(talent.roaring_blaze.enabled&debuff.conflagrate.remains<1.5)|charges=max_charges" );
  default_->add_action( "use_items" );
  default_->add_action( "cataclysm" );
  default_->add_action( "channel_demonfire,if=talent.raging_demonfire" );
  default_->add_action( "soul_fire,if=soul_shard<=4&!variable.pool_soul_shards", "We want to use soul_fire in a havoc window" );
  default_->add_action( "dimensional_rift,if=soul_shard<4.5&(charges>2|time_to_die<cooldown.dimensional_rift.duration)" );
  default_->add_action( "call_action_list,name=aoe,if=active_enemies>2" );
  default_->add_action( "immolate,cycle_targets=1,if=((dot.immolate.refreshable&talent.internal_combustion)|(dot.immolate.remains<3&!talent.internal_combustion))&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>remains)&(!talent.soul_fire|cooldown.soul_fire.remains>remains)", "TODO not cast immolate if havoc is up and soul_fire too (it will apply it)" );
  default_->add_action( "call_action_list,name=cds,if=cooldown.summon_infernal.up" );
  default_->add_action( "havoc,cycle_targets=1,if=!(target=self.target)" );
  default_->add_action( "chaos_bolt,if=(pet.infernal.active|pet.blasphemy.active)|soul_shard>=4", "We don't want to cap soul_shards (might need to try with & too)" );
  default_->add_action( "channel_demonfire,if=talent.ruin.rank>1&!(talent.diabolic_embers&talent.avatar_of_destruction&(talent.burn_to_ashes|talent.chaos_incarnate))", "There is some setups where using CDF is a loss" );
  default_->add_action( "conflagrate,if=buff.backdraft.down&soul_shard>=1.5&!variable.pool_soul_shards" );
  default_->add_action( "chaos_bolt,if=pet.infernal.active|buff.rain_of_chaos.remains>cast_time", "Some conditions to prio cast CB" );
  default_->add_action( "chaos_bolt,if=buff.backdraft.up&!variable.pool_soul_shards" );
  default_->add_action( "chaos_bolt,if=talent.eradication&!variable.pool_soul_shards&debuff.eradication.remains<cast_time&!action.chaos_bolt.in_flight" );
  default_->add_action( "soul_fire,if=soul_shard<=4&talent.mayhem", "If we play mayhem, we still use Soul_fire outside of mayhem windows but with low prio" );
  default_->add_action( "channel_demonfire,if=!(talent.diabolic_embers&talent.avatar_of_destruction&(talent.burn_to_ashes|talent.chaos_incarnate))", "There is some setups where using CDF is a loss" );
  default_->add_action( "dimensional_rift" );
  default_->add_action( "shadowburn,if=charges>1&target.health.pct<20&(!talent.eradication&!talent.ashen_remains)&(talent.ruin.rank=2&talent.conflagration_of_chaos.rank=2)", "SB without refund is 99% of the time a dps loss" );
  default_->add_action( "chaos_bolt,if=!variable.pool_soul_shards&talent.soul_conduit", "Low prio on CB cast actions+=/chaos_bolt,if=soul_shard>3.5" );
  default_->add_action( "chaos_bolt,if=time_to_die<5&time_to_die>cast_time+travel_time" );
  default_->add_action( "conflagrate,if=charges>(max_charges-1)|time_to_die<gcd" );
  default_->add_action( "incinerate" );

  aoe->add_action( "rain_of_fire,if=pet.infernal.active&(!cooldown.havoc.ready|active_enemies>3)", "TODO :Has not been touched yet SL logic" );
  aoe->add_action( "rain_of_fire,if=talent.avatar_of_destruction" );
  aoe->add_action( "channel_demonfire,if=dot.immolate.remains>cast_time" );
  aoe->add_action( "immolate,cycle_targets=1,if=active_enemies<5&dot.immolate.remains<5&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>dot.immolate.remains)" );
  aoe->add_action( "call_action_list,name=cds" );
  aoe->add_action( "havoc,cycle_targets=1,if=!(target=self.target)&active_enemies<4" );
  aoe->add_action( "rain_of_fire" );
  aoe->add_action( "havoc,cycle_targets=1,if=!(self.target=target)" );
  aoe->add_action( "incinerate,if=talent.fire_and_brimstone.enabled&buff.backdraft.up&soul_shard<5-0.2*active_enemies" );
  aoe->add_action( "soul_fire" );
  aoe->add_action( "conflagrate,if=buff.backdraft.down" );
  aoe->add_action( "immolate,if=refreshable" );
  aoe->add_action( "incinerate" );

  cds->add_action( "summon_infernal", "TODO trinkets" );
  cds->add_action( "potion,if=pet.infernal.active" );
  cds->add_action( "berserking,if=pet.infernal.active" );
  cds->add_action( "blood_fury,if=pet.infernal.active" );
  cds->add_action( "fireblood,if=pet.infernal.active" );
  cds->add_action( "use_items,if=pet.infernal.active|time_to_die<21" );

  havoc->add_action( "conflagrate,if=buff.backdraft.down&soul_shard>=1&soul_shard<=4", "Havoc logic, did not touched at rain of fire yet" );
  havoc->add_action( "soul_fire,if=cast_time<havoc_remains&soul_shard<3" );
  havoc->add_action( "immolate,cycle_targets=1,if=dot.immolate.refreshable" );
  havoc->add_action( "chaos_bolt,if=cast_time<havoc_remains&!(talent.avatar_of_destruction&active_enemies>1&talent.inferno.enabled)" );
  havoc->add_action( "rain_of_fire,if=talent.avatar_of_destruction&active_enemies>1&talent.inferno.enabled" );
  havoc->add_action( "shadowburn,if=target.health.pct<20&(!talent.eradication&!talent.ashen_remains)&(talent.ruin.rank=2&talent.conflagration_of_chaos.rank=2)" );
  havoc->add_action( "incinerate,if=cast_time<havoc_remains" );
}
//destruction_apl_end

} // namespace warlock_apl