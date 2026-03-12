#include "class_modules/apl/apl_shaman.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace shaman_apl
{

std::string flask_elemental( const player_t* p )
{
  return ( p->true_level >= 81 ) ? "flask_of_the_magister_2" : "disabled";
}

std::string food_elemental( const player_t* p )
{
  return ( p->true_level >= 81 ) ? "harandar_celebration" : "disabled";
}

std::string potion_elemental( const player_t* p )
{
  return ( p->true_level >= 81 ) ? "lights_potential_2" : "disabled";
}

std::string temporary_enchant_elemental( const player_t* p )
{
  return ( p->true_level >= 81 ) ? "main_hand:thalassian_phoenix_oil_2,if=!talent.flametongue_weapon"
    : "disabled";
}

std::string rune( const player_t* p )
{
  return ( p->true_level >= 81 ) ? "void_touched" : "disabled";
}

//elemental_apl_start
void elemental( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* single_target = p->get_action_priority_list( "single_target" );

  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "flametongue_weapon" );
  precombat->add_action( "lightning_shield" );
  precombat->add_action( "thunderstrike_ward" );
  precombat->add_action( "variable,name=mael_cap,value=100+50*talent.swelling_maelstrom+25*talent.primordial_capacity" );
  precombat->add_action( "stormkeeper" );

  default_->add_action( "spiritwalkers_grace,moving=1,if=movement.distance>6", "Enable more movement." );
  default_->add_action( "wind_shear", "Interrupt of casts." );
  default_->add_action( "blood_fury" );
  default_->add_action( "berserking" );
  default_->add_action( "fireblood" );
  default_->add_action( "ancestral_call" );
  default_->add_action( "use_item,slot=trinket1,use_off_gcd=1", "Normal buff trinkets" );
  default_->add_action( "use_item,slot=trinket2,use_off_gcd=1" );
  default_->add_action( "use_item,slot=main_hand,use_off_gcd=1", "Normal weapons" );
  default_->add_action( "lightning_shield,if=buff.lightning_shield.down" );
  default_->add_action( "natures_swiftness" );
  default_->add_action( "invoke_external_buff,name=power_infusion", "Use Power Infusion on Cooldown." );
  default_->add_action( "potion" );
  default_->add_action( "run_action_list,name=aoe,if=spell_targets.chain_lightning>=2" );
  default_->add_action( "run_action_list,name=single_target" );

  aoe->add_action( "fire_elemental" );
  aoe->add_action( "stormkeeper" );
  aoe->add_action( "ancestral_swiftness" );
  aoe->add_action( "flame_shock,if=active_dot.flame_shock=0&spell_targets.chain_lightning<=3&cooldown.ascendance.remains>10" );
  aoe->add_action( "voltaic_blaze,if=active_dot.flame_shock=0|talent.purging_flames" );
  aoe->add_action( "ascendance,if=cooldown.stormkeeper.remains>10" );
  aoe->add_action( "tempest,target_if=min:debuff.lightning_rod.remains,if=buff.tempest.stack=2" );
  aoe->add_action( "earthquake", "Spend if you are close to cap, Master of the Elements buff is up or Ascendance is about to expire." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=buff.purging_flames.up&(buff.ascendance.up&buff.ascendance.remains<3|buff.lava_surge.up)" );
  aoe->add_action( "lightning_bolt,if=buff.stormkeeper.up&!buff.call_of_the_ancestors.up&spell_targets.chain_lightning=2" );
  aoe->add_action( "chain_lightning" );
  aoe->add_action( "flame_shock,moving=1,target_if=refreshable" );
  aoe->add_action( "voltaic_blaze,moving=1,target_if=refreshable" );
  aoe->add_action( "frost_shock,moving=1" );

  single_target->add_action( "fire_elemental" );
  single_target->add_action( "stormkeeper" );
  single_target->add_action( "ancestral_swiftness" );
  single_target->add_action( "flame_shock,if=active_dot.flame_shock=0&!buff.master_of_the_elements.up", "Apply Flame shock if it is not up." );
  single_target->add_action( "voltaic_blaze,if=active_dot.flame_shock=0" );
  single_target->add_action( "ascendance,if=cooldown.stormkeeper.remains>10" );
  single_target->add_action( "elemental_blast,if=maelstrom>variable.mael_cap-15|buff.master_of_the_elements.up", "Spend if close to overcaping or MotE buff is up. Friendship ended with Echoes of Great Sundering." );
  single_target->add_action( "earth_shock,if=maelstrom>variable.mael_cap-15|buff.master_of_the_elements.up" );
  single_target->add_action( "lava_burst,target_if=dot.flame_shock.remains>=2,if=!buff.master_of_the_elements.up", "Use Lava Burst to proc Master of the Elements." );
  single_target->add_action( "tempest" );
  single_target->add_action( "lightning_bolt", "Filler spell. Always available. Always the bottom line." );
  single_target->add_action( "flame_shock,moving=1,target_if=refreshable" );
  single_target->add_action( "voltaic_blaze,moving=1,target_if=refreshable" );
  single_target->add_action( "flame_shock,moving=1,if=movement.distance>6" );
  single_target->add_action( "voltaic_blaze,moving=1,if=movement.distance>6" );
  single_target->add_action( "frost_shock,moving=1", "Frost Shock is our movement filler." );
}
//elemental_apl_end

//elemental_ptr_apl_start
void elemental_ptr( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* single_target = p->get_action_priority_list( "single_target" );

  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "flametongue_weapon" );
  precombat->add_action( "lightning_shield" );
  precombat->add_action( "thunderstrike_ward" );
  precombat->add_action( "variable,name=mael_cap,value=100+50*talent.swelling_maelstrom+25*talent.primordial_capacity" );
  precombat->add_action( "stormkeeper" );

  default_->add_action( "spiritwalkers_grace,moving=1,if=movement.distance>6", "Enable more movement." );
  default_->add_action( "wind_shear", "Interrupt of casts." );
  default_->add_action( "blood_fury" );
  default_->add_action( "berserking" );
  default_->add_action( "fireblood" );
  default_->add_action( "ancestral_call" );
  default_->add_action( "use_item,slot=trinket1,use_off_gcd=1", "Normal buff trinkets" );
  default_->add_action( "use_item,slot=trinket2,use_off_gcd=1" );
  default_->add_action( "use_item,slot=main_hand,use_off_gcd=1", "Normal weapons" );
  default_->add_action( "lightning_shield,if=buff.lightning_shield.down" );
  default_->add_action( "natures_swiftness" );
  default_->add_action( "invoke_external_buff,name=power_infusion", "Use Power Infusion on Cooldown." );
  default_->add_action( "potion" );
  default_->add_action( "run_action_list,name=aoe,if=spell_targets.chain_lightning>=2" );
  default_->add_action( "run_action_list,name=single_target" );

  aoe->add_action( "fire_elemental" );
  aoe->add_action( "stormkeeper" );
  aoe->add_action( "ancestral_swiftness" );
  aoe->add_action( "flame_shock,if=active_dot.flame_shock=0&spell_targets.chain_lightning<=3&cooldown.ascendance.remains>10" );
  aoe->add_action( "voltaic_blaze,if=active_dot.flame_shock=0|talent.purging_flames" );
  aoe->add_action( "ascendance,if=cooldown.stormkeeper.remains>10" );
  aoe->add_action( "tempest,target_if=min:debuff.lightning_rod.remains,if=buff.tempest.stack=2" );
  aoe->add_action( "earthquake", "Spend if you are close to cap, Master of the Elements buff is up or Ascendance is about to expire." );
  aoe->add_action( "lava_burst,target_if=dot.flame_shock.remains>2,if=buff.purging_flames.up&(buff.ascendance.up&buff.ascendance.remains<3|buff.lava_surge.up)" );
  aoe->add_action( "lightning_bolt,if=buff.stormkeeper.up&!buff.call_of_the_ancestors.up&spell_targets.chain_lightning=2" );
  aoe->add_action( "chain_lightning" );
  aoe->add_action( "flame_shock,moving=1,target_if=refreshable" );
  aoe->add_action( "voltaic_blaze,moving=1,target_if=refreshable" );
  aoe->add_action( "frost_shock,moving=1" );

  single_target->add_action( "fire_elemental" );
  single_target->add_action( "stormkeeper" );
  single_target->add_action( "ancestral_swiftness" );
  single_target->add_action( "flame_shock,if=active_dot.flame_shock=0&!buff.master_of_the_elements.up", "Apply Flame shock if it is not up." );
  single_target->add_action( "voltaic_blaze,if=active_dot.flame_shock=0" );
  single_target->add_action( "ascendance,if=cooldown.stormkeeper.remains>10" );
  single_target->add_action( "elemental_blast,if=maelstrom>variable.mael_cap-15|buff.master_of_the_elements.up", "Spend if close to overcaping or MotE buff is up. Friendship ended with Echoes of Great Sundering." );
  single_target->add_action( "earth_shock,if=maelstrom>variable.mael_cap-15|buff.master_of_the_elements.up" );
  single_target->add_action( "lava_burst,target_if=dot.flame_shock.remains>=2,if=!buff.master_of_the_elements.up", "Use Lava Burst to proc Master of the Elements." );
  single_target->add_action( "tempest" );
  single_target->add_action( "lightning_bolt", "Filler spell. Always available. Always the bottom line." );
  single_target->add_action( "flame_shock,moving=1,target_if=refreshable" );
  single_target->add_action( "voltaic_blaze,moving=1,target_if=refreshable" );
  single_target->add_action( "flame_shock,moving=1,if=movement.distance>6" );
  single_target->add_action( "voltaic_blaze,moving=1,if=movement.distance>6" );
  single_target->add_action( "frost_shock,moving=1", "Frost Shock is our movement filler." );
}
//elemental_ptr_apl_end

} //namespace shaman_apl
