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

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "summon_pet" );
  precombat->add_action( "grimoire_of_sacrifice,if=talent.grimoire_of_sacrifice.enabled" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "soul_fire" );
  precombat->add_action( "cataclysm" );
  precombat->add_action( "incinerate" );

  default_->add_action( "chaos_bolt,if=soul_shard=5" );
  default_->add_action( "soul_fire,if=soul_shard<=4&(buff.backdraft.up|!talent.backdraft)" );
  default_->add_action( "cataclysm" );
  default_->add_action( "immolate,if=dot.immolate.remains<5" );
  default_->add_action( "channel_demonfire" );
  default_->add_action( "use_items" );
  default_->add_action( "summon_infernal" );
  default_->add_action( "chaos_bolt,if=buff.backdraft.up" );
  default_->add_action( "conflagrate,if=talent.backdraft&buff.backdraft.down" );
  default_->add_action( "conflagrate,if=soul_shard<=4.5" );
  default_->add_action( "chaos_bolt,if=soul_shard>=4.5|!talent.backdraft" );
  default_->add_action( "immolate,if=dot.immolate.refreshable&(!talent.soul_fire&!talent.cataclysm)" );
  default_->add_action( "dimensional_rift" );
  default_->add_action( "shadowburn,if=target.health.pct<=20" );
  default_->add_action( "incinerate" );
}
//destruction_apl_end

} // namespace warlock_apl