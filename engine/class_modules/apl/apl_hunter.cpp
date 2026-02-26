#include "simulationcraft.hpp"
#include "class_modules/apl/apl_hunter.hpp"

namespace hunter_apl {

std::string potion( const player_t* p )
{
  // Spec-specific logic for Level 90 Potions
  std::string lvl90_potion = ( p -> specialization() == HUNTER_SURVIVAL )      ? "lights_potential_2" :
                             ( p -> specialization() == HUNTER_MARKSMANSHIP )  ? "lights_potential_2" :
                             "draught_of_rampant_abandon_2"; // Beast Mastery

  return ( p -> true_level > 80 ) ? lvl90_potion :
         ( p -> true_level > 70 ) ? "tempered_potion_3" : 
         ( p -> true_level > 60 ) ? "elemental_potion_of_ultimate_power_3" : 
         ( p -> true_level > 50 ) ? "spectral_agility" :
         ( p -> true_level >= 40 ) ? "unbridled_fury" :
         "disabled";
}

std::string flask( const player_t* p )
{
  // Spec-specific logic for Level 90 Flasks
  std::string lvl90_flask = ( p -> specialization() == HUNTER_SURVIVAL )      ? "flask_of_the_magisters_2" :
                            ( p -> specialization() == HUNTER_MARKSMANSHIP )  ? "flask_of_the_shattered_sun_2" :
                            "flask_of_the_magisters_2"; // Beast Mastery

  return ( p -> true_level > 80 ) ? lvl90_flask :
         ( p -> true_level > 70 ) ? "flask_of_alchemical_chaos_3" : 
         ( p -> true_level > 60 ) ? "iced_phial_of_corrupting_rage_3" : 
         ( p -> true_level > 50 ) ? "spectral_flask_of_power" :
         ( p -> true_level >= 40 ) ? "greater_flask_of_the_currents" :
         "disabled";
}

std::string food( const player_t* p )
{
  return ( p -> true_level > 80 ) ? "silvermoon_parade" :
         ( p -> true_level > 70 ) ? "the_sushi_special" : 
         ( p -> true_level > 60 ) ? "fated_fortune_cookie" : 
         ( p -> true_level > 50 ) ? "feast_of_gluttonous_hedonism" :
         ( p -> true_level >= 45 ) ? "bountiful_captains_feast" :
         "disabled";
}

std::string rune( const player_t* p )
{
  return ( p -> true_level > 80 ) ? "void_touched" :
         ( p -> true_level > 70 ) ? "crystallized" : 
         ( p -> true_level > 60 ) ? "draconic" :
         ( p -> true_level > 50 ) ? "veiled" :
         ( p -> true_level >= 40 ) ? "battle_scarred" :
         "disabled";
}

std::string temporary_enchant( const player_t* p )
{
  std::string lvl90_temp_enchant = ( p -> specialization() == HUNTER_SURVIVAL ) ? "main_hand:thalassian_phoenix_oil_2" : "main_hand:thalassian_phoenix_oil_2";
  std::string lvl80_temp_enchant = ( p -> specialization() == HUNTER_SURVIVAL ) ? "main_hand:ironclaw_whetstone_3" : "main_hand:algari_mana_oil_3";
  std::string lvl70_temp_enchant = ( p -> specialization() == HUNTER_SURVIVAL ) ? "main_hand:howling_rune_3" : "main_hand:completely_safe_rockets_3";
  std::string lvl60_temp_enchant = ( p -> specialization() == HUNTER_SURVIVAL ) ? "main_hand:shaded_sharpening_stone" : "main_hand:shadowcore_oil";

  return ( p -> true_level >= 90 ) ? lvl90_temp_enchant :
         ( p -> true_level >= 80 ) ? lvl80_temp_enchant :
         ( p -> true_level >= 70 ) ? lvl70_temp_enchant :
         ( p -> true_level >= 60 ) ? lvl60_temp_enchant :
         "disabled";
}

//beast_mastery_apl_start
void beast_mastery( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );
  action_priority_list_t* st = p->get_action_priority_list( "st" );
  action_priority_list_t* cleave = p->get_action_priority_list( "cleave" );
  action_priority_list_t* drst = p->get_action_priority_list( "drst" );
  action_priority_list_t* drcleave = p->get_action_priority_list( "drcleave" );

  precombat->add_action( "summon_pet" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "use_item,name=algethar_puzzle_box" );

  default_->add_action( "auto_shot" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "call_action_list,name=drst,if=talent.black_arrow&(active_enemies<2|!talent.beast_cleave&active_enemies<3)" );
  default_->add_action( "call_action_list,name=drcleave,if=talent.black_arrow&(active_enemies>2|talent.beast_cleave&active_enemies>1)" );
  default_->add_action( "call_action_list,name=st,if=!talent.black_arrow&(active_enemies<2|!talent.beast_cleave&active_enemies<3)" );
  default_->add_action( "call_action_list,name=cleave,if=!talent.black_arrow&(active_enemies>2|talent.beast_cleave&active_enemies>1)" );

  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.bestial_wrath.up|cooldown.bestial_wrath.remains<30|fight_remains<16" );
  cds->add_action( "berserking,if=buff.bestial_wrath.up|fight_remains<13" );
  cds->add_action( "blood_fury,if=buff.bestial_wrath.up|fight_remains<16" );
  cds->add_action( "ancestral_call,if=buff.bestial_wrath.up|fight_remains<16" );
  cds->add_action( "fireblood,if=buff.bestial_wrath.up|fight_remains<9" );
  cds->add_action( "potion,if=buff.bestial_wrath.up|fight_remains<31" );

  trinkets->add_action( "use_item,name=light_company_guidon,if=buff.bestial_wrath.up|fight_remains<21" );
  trinkets->add_action( "use_item,name=void_execution_mandate,if=buff.bestial_wrath.up|fight_remains<21" );
  trinkets->add_action( "use_item,name=algethar_puzzle_box,if=cooldown.bestial_wrath.remains<2|fight_remains<23" );
  trinkets->add_action( "use_item,name=emberwing_feather,if=buff.bestial_wrath.up|fight_remains<16" );
  trinkets->add_action( "use_item,name=freightrunners_flask,if=cooldown.bestial_wrath.ready|fight_remains<16" );
  trinkets->add_action( "use_item,name=sealed_chaos_urn,if=cooldown.bestial_wrath.ready|fight_remains<21" );
  trinkets->add_action( "use_item,name=evercollapsing_void_fissure,if=cooldown.bestial_wrath.ready|fight_remains<11" );
  trinkets->add_action( "use_item,name=rangercaptains_iridescent_insignia" );
  trinkets->add_action( "use_item,name=void_stalkers_contract" );
  trinkets->add_action( "use_item,name=latchs_crooked_hook" );

  st->add_action( "barbed_shot,if=cooldown.bestial_wrath.remains<gcd" );
  st->add_action( "bestial_wrath" );
  st->add_action( "kill_command,if=cooldown.bestial_wrath.remains>full_recharge_time+gcd&(buff.natures_ally.up|howl_summon.ready)|!apex.3" );
  st->add_action( "barbed_shot" );
  st->add_action( "cobra_shot" );

  cleave->add_action( "barbed_shot,if=cooldown.bestial_wrath.remains<gcd" );
  cleave->add_action( "wild_thrash" );
  cleave->add_action( "bestial_wrath" );
  cleave->add_action( "kill_command" );
  cleave->add_action( "cobra_shot,if=cooldown.wild_thrash.remains>gcd&buff.hogstrider.up&active_enemies<4" );
  cleave->add_action( "barbed_shot" );
  cleave->add_action( "cobra_shot,if=cooldown.wild_thrash.remains>gcd" );

  drst->add_action( "bestial_wrath" );
  drst->add_action( "kill_command,if=cooldown.bestial_wrath.remains>full_recharge_time+gcd&buff.natures_ally.up|!apex.3" );
  drst->add_action( "black_arrow,if=buff.withering_fire.up" );
  drst->add_action( "cobra_shot,if=talent.killer_cobra&buff.bestial_wrath.up&cooldown.barbed_shot.charges_fractional<1.4" );
  drst->add_action( "wailing_arrow,if=buff.withering_fire.remains<execute_time+gcd|time_to_die.remains<execute_time+gcd" );
  drst->add_action( "barbed_shot" );
  drst->add_action( "black_arrow" );
  drst->add_action( "cobra_shot" );

  drcleave->add_action( "black_arrow,if=buff.beast_cleave.remains<gcd" );
  drcleave->add_action( "bestial_wrath" );
  drcleave->add_action( "wailing_arrow,if=buff.bestial_wrath.remains<execute_time+gcd|fight_remains<execute_time+gcd" );
  drcleave->add_action( "wild_thrash" );
  drcleave->add_action( "kill_command,if=cooldown.bestial_wrath.remains>full_recharge_time+gcd&buff.natures_ally.up|!apex.3" );
  drcleave->add_action( "black_arrow,if=buff.withering_fire.up" );
  drcleave->add_action( "barbed_shot" );
  drcleave->add_action( "wailing_arrow" );
  drcleave->add_action( "black_arrow" );
  drcleave->add_action( "cobra_shot" );
}
//beast_mastery_apl_end

//beast_mastery_ptr_apl_start
void beast_mastery_ptr( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );
  action_priority_list_t* st = p->get_action_priority_list( "st" );
  action_priority_list_t* cleave = p->get_action_priority_list( "cleave" );
  action_priority_list_t* drst = p->get_action_priority_list( "drst" );
  action_priority_list_t* drcleave = p->get_action_priority_list( "drcleave" );

  precombat->add_action( "summon_pet" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "use_item,name=algethar_puzzle_box" );

  default_->add_action( "auto_shot" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "call_action_list,name=drst,if=talent.black_arrow&(active_enemies<2|!talent.beast_cleave&active_enemies<3)" );
  default_->add_action( "call_action_list,name=drcleave,if=talent.black_arrow&(active_enemies>2|talent.beast_cleave&active_enemies>1)" );
  default_->add_action( "call_action_list,name=st,if=!talent.black_arrow&(active_enemies<2|!talent.beast_cleave&active_enemies<3)" );
  default_->add_action( "call_action_list,name=cleave,if=!talent.black_arrow&(active_enemies>2|talent.beast_cleave&active_enemies>1)" );

  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.bestial_wrath.up|cooldown.bestial_wrath.remains<30|fight_remains<16" );
  cds->add_action( "berserking,if=buff.bestial_wrath.up|fight_remains<13" );
  cds->add_action( "blood_fury,if=buff.bestial_wrath.up|fight_remains<16" );
  cds->add_action( "ancestral_call,if=buff.bestial_wrath.up|fight_remains<16" );
  cds->add_action( "fireblood,if=buff.bestial_wrath.up|fight_remains<9" );
  cds->add_action( "potion,if=buff.bestial_wrath.up|fight_remains<31" );

  trinkets->add_action( "use_item,name=light_company_guidon,if=buff.bestial_wrath.up|fight_remains<21" );
  trinkets->add_action( "use_item,name=void_execution_mandate,if=buff.bestial_wrath.up|fight_remains<21" );
  trinkets->add_action( "use_item,name=algethar_puzzle_box,if=cooldown.bestial_wrath.remains<2|fight_remains<23" );
  trinkets->add_action( "use_item,name=emberwing_feather,if=buff.bestial_wrath.up|fight_remains<16" );
  trinkets->add_action( "use_item,name=freightrunners_flask,if=cooldown.bestial_wrath.ready|fight_remains<16" );
  trinkets->add_action( "use_item,name=sealed_chaos_urn,if=cooldown.bestial_wrath.ready|fight_remains<21" );
  trinkets->add_action( "use_item,name=evercollapsing_void_fissure,if=cooldown.bestial_wrath.ready|fight_remains<11" );
  trinkets->add_action( "use_item,name=rangercaptains_iridescent_insignia" );
  trinkets->add_action( "use_item,name=void_stalkers_contract" );
  trinkets->add_action( "use_item,name=latchs_crooked_hook" );

  st->add_action( "barbed_shot,if=cooldown.bestial_wrath.remains<gcd" );
  st->add_action( "bestial_wrath" );
  st->add_action( "kill_command,if=cooldown.bestial_wrath.remains>full_recharge_time+gcd&(buff.natures_ally.up|howl_summon.ready)|!apex.3" );
  st->add_action( "barbed_shot" );
  st->add_action( "cobra_shot" );

  cleave->add_action( "barbed_shot,if=cooldown.bestial_wrath.remains<gcd" );
  cleave->add_action( "wild_thrash" );
  cleave->add_action( "bestial_wrath" );
  cleave->add_action( "kill_command" );
  cleave->add_action( "cobra_shot,if=cooldown.wild_thrash.remains>gcd&buff.hogstrider.up&active_enemies<4" );
  cleave->add_action( "barbed_shot" );
  cleave->add_action( "cobra_shot,if=cooldown.wild_thrash.remains>gcd" );

  drst->add_action( "bestial_wrath" );
  drst->add_action( "kill_command,if=cooldown.bestial_wrath.remains>full_recharge_time+gcd&buff.natures_ally.up|!apex.3" );
  drst->add_action( "black_arrow,if=buff.withering_fire.up" );
  drst->add_action( "cobra_shot,if=talent.killer_cobra&buff.bestial_wrath.up&cooldown.barbed_shot.charges_fractional<1.4" );
  drst->add_action( "wailing_arrow,if=buff.withering_fire.remains<execute_time+gcd|time_to_die.remains<execute_time+gcd" );
  drst->add_action( "barbed_shot" );
  drst->add_action( "black_arrow" );
  drst->add_action( "cobra_shot" );

  drcleave->add_action( "black_arrow,if=buff.beast_cleave.remains<gcd" );
  drcleave->add_action( "bestial_wrath" );
  drcleave->add_action( "wailing_arrow,if=buff.bestial_wrath.remains<execute_time+gcd|fight_remains<execute_time+gcd" );
  drcleave->add_action( "wild_thrash" );
  drcleave->add_action( "kill_command,if=cooldown.bestial_wrath.remains>full_recharge_time+gcd&buff.natures_ally.up|!apex.3" );
  drcleave->add_action( "black_arrow,if=buff.withering_fire.up" );
  drcleave->add_action( "barbed_shot" );
  drcleave->add_action( "wailing_arrow" );
  drcleave->add_action( "black_arrow" );
  drcleave->add_action( "cobra_shot" );
}
//beast_mastery_ptr_apl_end

//marksmanship_apl_start
void marksmanship( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* draoe = p->get_action_priority_list( "draoe" );
  action_priority_list_t* drst = p->get_action_priority_list( "drst" );
  action_priority_list_t* sentaoe = p->get_action_priority_list( "sentaoe" );
  action_priority_list_t* sentst = p->get_action_priority_list( "sentst" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );

  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "summon_pet,if=talent.unbreakable_bond" );
  precombat->add_action( "use_item,name=algethar_puzzle_box" );
  precombat->add_action( "aimed_shot,if=active_enemies<3|talent.black_arrow&talent.headshot" );
  precombat->add_action( "steady_shot" );

  default_->add_action( "variable,name=trueshot_ready,value=!talent.bullseye|fight_remains>cooldown.trueshot.duration+10|buff.bullseye.stack=buff.bullseye.max_stack|fight_remains<25" );
  default_->add_action( "auto_shot" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "call_action_list,name=draoe,if=active_enemies>2&talent.trick_shots&hero_tree.dark_ranger" );
  default_->add_action( "call_action_list,name=sentaoe,if=active_enemies>2&talent.trick_shots&hero_tree.sentinel" );
  default_->add_action( "call_action_list,name=drst,if=hero_tree.dark_ranger" );
  default_->add_action( "call_action_list,name=sentst,if=hero_tree.sentinel" );

  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.trueshot.remains>12|fight_remains<13" );
  cds->add_action( "berserking,if=buff.trueshot.up|fight_remains<13" );
  cds->add_action( "blood_fury,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<16" );
  cds->add_action( "ancestral_call,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<16" );
  cds->add_action( "fireblood,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<9" );
  cds->add_action( "lights_judgment,if=buff.trueshot.down" );
  cds->add_action( "potion,if=buff.trueshot.up&(buff.bloodlust.up|target.health.pct<20)|fight_remains<31" );

  draoe->add_action( "black_arrow" );
  draoe->add_action( "multishot,if=buff.precise_shots.up|buff.trick_shots.down" );
  draoe->add_action( "rapid_fire,if=buff.bulletstorm.remains<action.aimed_shot.execute_time" );
  draoe->add_action( "trueshot,if=!buff.double_tap.up" );
  draoe->add_action( "volley,if=!buff.double_tap.up" );
  draoe->add_action( "aimed_shot" );
  draoe->add_action( "wailing_arrow" );
  draoe->add_action( "rapid_fire" );
  draoe->add_action( "steady_shot" );

  drst->add_action( "black_arrow" );
  drst->add_action( "rapid_fire,if=talent.unload&buff.withering_fire.up" );
  drst->add_action( "arcane_shot,if=buff.precise_shots.up" );
  drst->add_action( "rapid_fire,if=buff.bulletstorm.remains<action.aimed_shot.execute_time" );
  drst->add_action( "trueshot,if=!buff.double_tap.up" );
  drst->add_action( "volley,if=!buff.double_tap.up" );
  drst->add_action( "aimed_shot" );
  drst->add_action( "wailing_arrow" );
  drst->add_action( "rapid_fire" );
  drst->add_action( "steady_shot" );

  sentaoe->add_action( "multishot,if=buff.precise_shots.up|buff.trick_shots.down" );
  sentaoe->add_action( "rapid_fire,if=buff.bulletstorm.remains<action.aimed_shot.execute_time" );
  sentaoe->add_action( "trueshot,if=!buff.double_tap.up" );
  sentaoe->add_action( "volley,if=!buff.double_tap.up" );
  sentaoe->add_action( "aimed_shot" );
  sentaoe->add_action( "rapid_fire" );
  sentaoe->add_action( "moonlight_chakram,if=buff.trueshot.up" );
  sentaoe->add_action( "steady_shot" );

  sentst->add_action( "arcane_shot,if=buff.precise_shots.up" );
  sentst->add_action( "rapid_fire,if=buff.bulletstorm.remains<action.aimed_shot.execute_time" );
  sentst->add_action( "trueshot,if=!buff.double_tap.up" );
  sentst->add_action( "volley,if=!buff.double_tap.up" );
  sentst->add_action( "aimed_shot" );
  sentst->add_action( "moonlight_chakram,if=buff.trueshot.up" );
  sentst->add_action( "rapid_fire" );
  sentst->add_action( "steady_shot" );

  trinkets->add_action( "use_items,slots=trinket1:trinket2,if=cooldown.trueshot.remains<2|!this_trinket.has_use_buff|buff.trueshot.remains>17|cooldown.trueshot.remains>45" );
  trinkets->add_action( "use_item,name=algethar_puzzle_box,if=cooldown.trueshot.remains<2|fight_remains<23" );
}
//marksmanship_apl_end

//marksmanship_ptr_apl_start
void marksmanship_ptr( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* draoe = p->get_action_priority_list( "draoe" );
  action_priority_list_t* drst = p->get_action_priority_list( "drst" );
  action_priority_list_t* sentaoe = p->get_action_priority_list( "sentaoe" );
  action_priority_list_t* sentst = p->get_action_priority_list( "sentst" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );

  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "summon_pet,if=talent.unbreakable_bond" );
  precombat->add_action( "use_item,name=algethar_puzzle_box" );
  precombat->add_action( "aimed_shot,if=active_enemies<3|talent.black_arrow&talent.headshot" );
  precombat->add_action( "steady_shot" );

  default_->add_action( "variable,name=trueshot_ready,value=!talent.bullseye|fight_remains>cooldown.trueshot.duration+10|buff.bullseye.stack=buff.bullseye.max_stack|fight_remains<25" );
  default_->add_action( "auto_shot" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "call_action_list,name=draoe,if=active_enemies>2&talent.trick_shots&hero_tree.dark_ranger" );
  default_->add_action( "call_action_list,name=sentaoe,if=active_enemies>2&talent.trick_shots&hero_tree.sentinel" );
  default_->add_action( "call_action_list,name=drst,if=hero_tree.dark_ranger" );
  default_->add_action( "call_action_list,name=sentst,if=hero_tree.sentinel" );

  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.trueshot.remains>12|fight_remains<13" );
  cds->add_action( "berserking,if=buff.trueshot.up|fight_remains<13" );
  cds->add_action( "blood_fury,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<16" );
  cds->add_action( "ancestral_call,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<16" );
  cds->add_action( "fireblood,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<9" );
  cds->add_action( "lights_judgment,if=buff.trueshot.down" );
  cds->add_action( "potion,if=buff.trueshot.up&(buff.bloodlust.up|target.health.pct<20)|fight_remains<31" );

  draoe->add_action( "black_arrow" );
  draoe->add_action( "multishot,if=buff.precise_shots.up|buff.trick_shots.down" );
  draoe->add_action( "rapid_fire,if=buff.bulletstorm.remains<action.aimed_shot.execute_time" );
  draoe->add_action( "trueshot,if=!buff.double_tap.up" );
  draoe->add_action( "volley,if=!buff.double_tap.up" );
  draoe->add_action( "aimed_shot" );
  draoe->add_action( "wailing_arrow" );
  draoe->add_action( "rapid_fire" );
  draoe->add_action( "steady_shot" );

  drst->add_action( "black_arrow" );
  drst->add_action( "rapid_fire,if=talent.unload&buff.withering_fire.up" );
  drst->add_action( "arcane_shot,if=buff.precise_shots.up" );
  drst->add_action( "rapid_fire,if=buff.bulletstorm.remains<action.aimed_shot.execute_time" );
  drst->add_action( "trueshot,if=!buff.double_tap.up" );
  drst->add_action( "volley,if=!buff.double_tap.up" );
  drst->add_action( "aimed_shot" );
  drst->add_action( "wailing_arrow" );
  drst->add_action( "rapid_fire" );
  drst->add_action( "steady_shot" );

  sentaoe->add_action( "multishot,if=buff.precise_shots.up|buff.trick_shots.down" );
  sentaoe->add_action( "rapid_fire,if=buff.bulletstorm.remains<action.aimed_shot.execute_time" );
  sentaoe->add_action( "trueshot,if=!buff.double_tap.up" );
  sentaoe->add_action( "volley,if=!buff.double_tap.up" );
  sentaoe->add_action( "aimed_shot" );
  sentaoe->add_action( "rapid_fire" );
  sentaoe->add_action( "moonlight_chakram,if=buff.trueshot.up" );
  sentaoe->add_action( "steady_shot" );

  sentst->add_action( "arcane_shot,if=buff.precise_shots.up" );
  sentst->add_action( "rapid_fire,if=buff.bulletstorm.remains<action.aimed_shot.execute_time" );
  sentst->add_action( "trueshot,if=!buff.double_tap.up" );
  sentst->add_action( "volley,if=!buff.double_tap.up" );
  sentst->add_action( "aimed_shot" );
  sentst->add_action( "moonlight_chakram,if=buff.trueshot.up" );
  sentst->add_action( "rapid_fire" );
  sentst->add_action( "steady_shot" );

  trinkets->add_action( "use_items,slots=trinket1:trinket2,if=cooldown.trueshot.remains<2|!this_trinket.has_use_buff|buff.trueshot.remains>17|cooldown.trueshot.remains>45" );
  trinkets->add_action( "use_item,name=algethar_puzzle_box,if=cooldown.trueshot.remains<2|fight_remains<23" );
}
//marksmanship_ptr_apl_end

//survival_apl_start
void survival( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* plst = p->get_action_priority_list( "plst" );
  action_priority_list_t* sentst = p->get_action_priority_list( "sentst" );
  action_priority_list_t* plcleave = p->get_action_priority_list( "plcleave" );
  action_priority_list_t* sentcleave = p->get_action_priority_list( "sentcleave" );

  precombat->add_action( "summon_pet" );
  precombat->add_action( "snapshot_stats" );

  default_->add_action( "auto_attack" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=plst,if=active_enemies<3&talent.howl_of_the_pack_leader" );
  default_->add_action( "call_action_list,name=plcleave,if=active_enemies>2&talent.howl_of_the_pack_leader" );
  default_->add_action( "call_action_list,name=sentst,if=active_enemies<3&!talent.howl_of_the_pack_leader" );
  default_->add_action( "call_action_list,name=sentcleave,if=active_enemies>2&!talent.howl_of_the_pack_leader" );

  cds->add_action( "blood_fury,if=buff.takedown.up|cooldown.takedown.ready", "CDS" );
  cds->add_action( "use_items,if=buff.takedown.up|cooldown.takedown.ready|!talent.takedown" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.takedown.up&!buff.power_infusion.up|fight_remains<16" );
  cds->add_action( "harpoon,if=prev.kill_command" );
  cds->add_action( "ancestral_call,if=buff.takedown.up|cooldown.takedown.ready" );
  cds->add_action( "fireblood,if=buff.takedown.up|cooldown.takedown.ready" );
  cds->add_action( "berserking,if=buff.takedown.up|cooldown.takedown.ready" );
  cds->add_action( "muzzle" );
  cds->add_action( "potion,if=target.time_to_die<25|cooldown.takedown.ready" );
  cds->add_action( "aspect_of_the_eagle,if=target.distance>=6" );

  plst->add_action( "kill_command,if=buff.tip_of_the_spear.stack<2&(buff.howl_of_the_pack_leader_wyvern.remains|buff.howl_of_the_pack_leader_boar.remains|buff.howl_of_the_pack_leader_bear.remains)", "ST - PL" );
  plst->add_action( "kill_command,if=cooldown.takedown.remains<gcd&buff.tip_of_the_spear.stack<2&!talent.twin_fangs" );
  plst->add_action( "takedown,if=buff.tip_of_the_spear.stack>0&!talent.twin_fangs|buff.tip_of_the_spear.stack=0&talent.twin_fangs" );
  plst->add_action( "flamefang_pitch" );
  plst->add_action( "boomstick,if=buff.tip_of_the_spear.up" );
  plst->add_action( "wildfire_bomb,if=fury_of_the_wyvern_extendable&buff.tip_of_the_spear.up" );
  plst->add_action( "raptor_strike,if=buff.tip_of_the_spear.up|!buff.raptor_swipe.up" );
  plst->add_action( "kill_command,if=cooldown.takedown.remains" );
  plst->add_action( "wildfire_bomb" );
  plst->add_action( "takedown" );

  sentst->add_action( "kill_command,if=buff.tip_of_the_spear.stack=0&(cooldown.takedown.remains|!talent.twin_fangs)", "ST - Sent" );
  sentst->add_action( "boomstick,if=buff.tip_of_the_spear.up&!cooldown.takedown.ready&!debuff.sentinels_mark.remains" );
  sentst->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.up&(debuff.sentinels_mark.remains|full_recharge_time<4+gcd)" );
  sentst->add_action( "kill_command,if=cooldown.takedown.remains<gcd&buff.tip_of_the_spear.stack<2&!talent.twin_fangs" );
  sentst->add_action( "takedown,if=buff.tip_of_the_spear.stack>0&!talent.twin_fangs|buff.tip_of_the_spear.stack=0&talent.twin_fangs" );
  sentst->add_action( "boomstick,if=buff.tip_of_the_spear.up" );
  sentst->add_action( "moonlight_chakram,if=buff.tip_of_the_spear.up" );
  sentst->add_action( "flamefang_pitch,if=talent.flamefang_pitch" );
  sentst->add_action( "raptor_strike,if=buff.tip_of_the_spear.up|!buff.raptor_swipe.up" );
  sentst->add_action( "takedown" );

  plcleave->add_action( "kill_command,if=buff.tip_of_the_spear.stack<2&(buff.howl_of_the_pack_leader_wyvern.remains|buff.howl_of_the_pack_leader_boar.remains|buff.howl_of_the_pack_leader_bear.remains)", "AOE - PL" );
  plcleave->add_action( "kill_command,if=cooldown.takedown.remains<gcd&buff.tip_of_the_spear.stack<2&!talent.twin_fangs" );
  plcleave->add_action( "takedown,if=buff.tip_of_the_spear.stack=0" );
  plcleave->add_action( "flamefang_pitch" );
  plcleave->add_action( "wildfire_bomb,if=full_recharge_time<gcd" );
  plcleave->add_action( "boomstick,if=buff.tip_of_the_spear.up" );
  plcleave->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.up" );
  plcleave->add_action( "raptor_strike,if=buff.tip_of_the_spear.up|!buff.raptor_swipe.up" );
  plcleave->add_action( "kill_command,if=cooldown.takedown.remains" );
  plcleave->add_action( "wildfire_bomb" );
  plcleave->add_action( "takedown" );

  sentcleave->add_action( "kill_command,if=buff.tip_of_the_spear.stack=0", "AOE - Sent" );
  sentcleave->add_action( "wildfire_bomb,if=talent.wildfire_shells&(buff.tip_of_the_spear.up&!debuff.sentinels_mark.remains&cooldown.boomstick.remains<11&cooldown.boomstick.remains>1)" );
  sentcleave->add_action( "boomstick,if=buff.tip_of_the_spear.up" );
  sentcleave->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.up&(debuff.sentinels_mark.remains|full_recharge_time<4+gcd)" );
  sentcleave->add_action( "kill_command,if=cooldown.takedown.remains<gcd&buff.tip_of_the_spear.stack<2&!talent.twin_fangs" );
  sentcleave->add_action( "takedown,if=buff.tip_of_the_spear.up" );
  sentcleave->add_action( "moonlight_chakram,if=buff.tip_of_the_spear.up" );
  sentcleave->add_action( "flamefang_pitch,if=talent.flamefang_pitch&buff.tip_of_the_spear.up" );
  sentcleave->add_action( "raptor_strike,if=buff.tip_of_the_spear.up&buff.raptor_swipe.up|!buff.raptor_swipe.up" );
  sentcleave->add_action( "kill_command" );
}
//survival_apl_end

//survival_ptr_apl_start
void survival_ptr( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* plst = p->get_action_priority_list( "plst" );
  action_priority_list_t* sentst = p->get_action_priority_list( "sentst" );
  action_priority_list_t* plcleave = p->get_action_priority_list( "plcleave" );
  action_priority_list_t* sentcleave = p->get_action_priority_list( "sentcleave" );

  precombat->add_action( "summon_pet" );
  precombat->add_action( "snapshot_stats" );

  default_->add_action( "auto_attack" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=plst,if=active_enemies<3&talent.howl_of_the_pack_leader" );
  default_->add_action( "call_action_list,name=plcleave,if=active_enemies>2&talent.howl_of_the_pack_leader" );
  default_->add_action( "call_action_list,name=sentst,if=active_enemies<3&!talent.howl_of_the_pack_leader" );
  default_->add_action( "call_action_list,name=sentcleave,if=active_enemies>2&!talent.howl_of_the_pack_leader" );

  cds->add_action( "blood_fury,if=buff.takedown.up|cooldown.takedown.ready", "CDS" );
  cds->add_action( "use_items,if=buff.takedown.up|cooldown.takedown.ready|!talent.takedown" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.takedown.up&!buff.power_infusion.up|fight_remains<16" );
  cds->add_action( "harpoon,if=prev.kill_command" );
  cds->add_action( "ancestral_call,if=buff.takedown.up|cooldown.takedown.ready" );
  cds->add_action( "fireblood,if=buff.takedown.up|cooldown.takedown.ready" );
  cds->add_action( "berserking,if=buff.takedown.up|cooldown.takedown.ready" );
  cds->add_action( "muzzle" );
  cds->add_action( "potion,if=target.time_to_die<25|cooldown.takedown.ready" );
  cds->add_action( "aspect_of_the_eagle,if=target.distance>=6" );

  plst->add_action( "kill_command,if=buff.tip_of_the_spear.stack<2&(buff.howl_of_the_pack_leader_wyvern.remains|buff.howl_of_the_pack_leader_boar.remains|buff.howl_of_the_pack_leader_bear.remains)", "ST - PL" );
  plst->add_action( "kill_command,if=cooldown.takedown.remains<gcd&buff.tip_of_the_spear.stack<2&!talent.twin_fangs" );
  plst->add_action( "takedown,if=buff.tip_of_the_spear.stack>0&!talent.twin_fangs|buff.tip_of_the_spear.stack=0&talent.twin_fangs" );
  plst->add_action( "flamefang_pitch" );
  plst->add_action( "boomstick,if=buff.tip_of_the_spear.up" );
  plst->add_action( "wildfire_bomb,if=fury_of_the_wyvern_extendable&buff.tip_of_the_spear.up" );
  plst->add_action( "raptor_strike,if=buff.tip_of_the_spear.up|!buff.raptor_swipe.up" );
  plst->add_action( "kill_command,if=cooldown.takedown.remains" );
  plst->add_action( "wildfire_bomb" );
  plst->add_action( "takedown" );

  sentst->add_action( "kill_command,if=buff.tip_of_the_spear.stack=0&(cooldown.takedown.remains|!talent.twin_fangs)", "ST - Sent" );
  sentst->add_action( "boomstick,if=buff.tip_of_the_spear.up&!cooldown.takedown.ready&!debuff.sentinels_mark.remains" );
  sentst->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.up&(debuff.sentinels_mark.remains|full_recharge_time<4+gcd)" );
  sentst->add_action( "kill_command,if=cooldown.takedown.remains<gcd&buff.tip_of_the_spear.stack<2&!talent.twin_fangs" );
  sentst->add_action( "takedown,if=buff.tip_of_the_spear.stack>0&!talent.twin_fangs|buff.tip_of_the_spear.stack=0&talent.twin_fangs" );
  sentst->add_action( "boomstick,if=buff.tip_of_the_spear.up" );
  sentst->add_action( "moonlight_chakram,if=buff.tip_of_the_spear.up" );
  sentst->add_action( "flamefang_pitch,if=talent.flamefang_pitch" );
  sentst->add_action( "raptor_strike,if=buff.tip_of_the_spear.up|!buff.raptor_swipe.up" );
  sentst->add_action( "takedown" );

  plcleave->add_action( "kill_command,if=buff.tip_of_the_spear.stack<2&(buff.howl_of_the_pack_leader_wyvern.remains|buff.howl_of_the_pack_leader_boar.remains|buff.howl_of_the_pack_leader_bear.remains)", "AOE - PL" );
  plcleave->add_action( "kill_command,if=cooldown.takedown.remains<gcd&buff.tip_of_the_spear.stack<2&!talent.twin_fangs" );
  plcleave->add_action( "takedown,if=buff.tip_of_the_spear.stack=0" );
  plcleave->add_action( "flamefang_pitch" );
  plcleave->add_action( "wildfire_bomb,if=full_recharge_time<gcd" );
  plcleave->add_action( "boomstick,if=buff.tip_of_the_spear.up" );
  plcleave->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.up" );
  plcleave->add_action( "raptor_strike,if=buff.tip_of_the_spear.up|!buff.raptor_swipe.up" );
  plcleave->add_action( "kill_command,if=cooldown.takedown.remains" );
  plcleave->add_action( "wildfire_bomb" );
  plcleave->add_action( "takedown" );

  sentcleave->add_action( "kill_command,if=buff.tip_of_the_spear.stack=0", "AOE - Sent" );
  sentcleave->add_action( "wildfire_bomb,if=talent.wildfire_shells&(buff.tip_of_the_spear.up&!debuff.sentinels_mark.remains&cooldown.boomstick.remains<11&cooldown.boomstick.remains>1)" );
  sentcleave->add_action( "boomstick,if=buff.tip_of_the_spear.up" );
  sentcleave->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.up&(debuff.sentinels_mark.remains|full_recharge_time<4+gcd)" );
  sentcleave->add_action( "kill_command,if=cooldown.takedown.remains<gcd&buff.tip_of_the_spear.stack<2&!talent.twin_fangs" );
  sentcleave->add_action( "takedown,if=buff.tip_of_the_spear.up" );
  sentcleave->add_action( "moonlight_chakram,if=buff.tip_of_the_spear.up" );
  sentcleave->add_action( "flamefang_pitch,if=talent.flamefang_pitch&buff.tip_of_the_spear.up" );
  sentcleave->add_action( "raptor_strike,if=buff.tip_of_the_spear.up&buff.raptor_swipe.up|!buff.raptor_swipe.up" );
  sentcleave->add_action( "kill_command" );
}
//survival_ptr_apl_end

} // namespace hunter_apl

