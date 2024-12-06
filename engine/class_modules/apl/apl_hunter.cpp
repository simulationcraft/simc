#include "simulationcraft.hpp"
#include "class_modules/apl/apl_hunter.hpp"

namespace hunter_apl {

std::string potion( const player_t* p )
{
  return ( p -> true_level > 70 ) ? "tempered_potion_3" : 
         ( p -> true_level > 60 ) ? "elemental_potion_of_ultimate_power_3" : 
         ( p -> true_level > 50 ) ? "spectral_agility" :
         ( p -> true_level >= 40 ) ? "unbridled_fury" :
         "disabled";
}

std::string flask( const player_t* p )
{
  return ( p -> true_level > 70 ) ? "flask_of_alchemical_chaos_3" : 
         ( p -> true_level > 60 ) ? "iced_phial_of_corrupting_rage_3" : 
         ( p -> true_level > 50 ) ? "spectral_flask_of_power" :
         ( p -> true_level >= 40 ) ? "greater_flask_of_the_currents" :
         "disabled";
}

std::string food( const player_t* p )
{
  return ( p -> true_level > 70 ) ? "the_sushi_special" : 
         ( p -> true_level > 60 ) ? "fated_fortune_cookie" : 
         ( p -> true_level > 50 ) ? "feast_of_gluttonous_hedonism" :
         ( p -> true_level >= 45 ) ? "bountiful_captains_feast" :
         "disabled";
}

std::string rune( const player_t* p )
{
  return ( p -> true_level > 70 ) ? "crystallized" : 
         ( p -> true_level > 60 ) ? "draconic" :
         ( p -> true_level > 50 ) ? "veiled" :
         ( p -> true_level >= 40 ) ? "battle_scarred" :
         "disabled";
}

std::string temporary_enchant( const player_t* p )
{
  std::string lvl80_temp_enchant = ( p -> specialization() == HUNTER_SURVIVAL ) ? "main_hand:ironclaw_whetstone_3" : "main_hand:algari_mana_oil_3";
  std::string lvl70_temp_enchant = ( p -> specialization() == HUNTER_SURVIVAL ) ? "main_hand:howling_rune_3" : "main_hand:completely_safe_rockets_3";
  std::string lvl60_temp_enchant = ( p -> specialization() == HUNTER_SURVIVAL ) ? "main_hand:shaded_sharpening_stone" : "main_hand:shadowcore_oil";

  return ( p -> true_level >= 80 ) ? lvl80_temp_enchant :
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
  action_priority_list_t* cleave = p->get_action_priority_list( "cleave" );
  action_priority_list_t* st = p->get_action_priority_list( "st" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );

  precombat->add_action( "summon_pet" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=trinket_1_stronger,value=!trinket.2.has_cooldown|trinket.1.has_use_buff&(!trinket.2.has_use_buff|!trinket.1.is.mirror_of_fractured_tomorrows&(trinket.2.is.mirror_of_fractured_tomorrows|trinket.2.cooldown.duration<trinket.1.cooldown.duration|trinket.2.cast_time<trinket.1.cast_time|trinket.2.cast_time=trinket.1.cast_time&trinket.2.cooldown.duration=trinket.1.cooldown.duration))|!trinket.1.has_use_buff&(!trinket.2.has_use_buff&(trinket.2.cooldown.duration<trinket.1.cooldown.duration|trinket.2.cast_time<trinket.1.cast_time|trinket.2.cast_time=trinket.1.cast_time&trinket.2.cooldown.duration=trinket.1.cooldown.duration))" );
  precombat->add_action( "variable,name=trinket_2_stronger,value=!variable.trinket_1_stronger" );
  precombat->add_action( "bestial_wrath,if=talent.vicious_hunt&talent.call_of_the_wild" );

  default_->add_action( "auto_shot" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "call_action_list,name=st,if=active_enemies<2|!talent.beast_cleave&active_enemies<3" );
  default_->add_action( "call_action_list,name=cleave,if=active_enemies>2|talent.beast_cleave&active_enemies>1" );

  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.call_of_the_wild.up|talent.bloodshed&(prev_gcd.1.bloodshed)|!talent.call_of_the_wild&(buff.bestial_wrath.up|cooldown.bestial_wrath.remains<30)|fight_remains<16" );
  cds->add_action( "berserking,if=buff.call_of_the_wild.up|talent.bloodshed&(prev_gcd.1.bloodshed)|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<13" );
  cds->add_action( "blood_fury,if=buff.call_of_the_wild.up|talent.bloodshed&(prev_gcd.1.bloodshed)|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<16" );
  cds->add_action( "ancestral_call,if=buff.call_of_the_wild.up|talent.bloodshed&(prev_gcd.1.bloodshed)|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<16" );
  cds->add_action( "fireblood,if=buff.call_of_the_wild.up|talent.bloodshed&(prev_gcd.1.bloodshed)|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<9" );
  cds->add_action( "potion,if=buff.call_of_the_wild.up|talent.bloodshed&(prev_gcd.1.bloodshed)|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<31" );

  cleave->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=pet.main.buff.frenzy.up&pet.main.buff.frenzy.remains<=gcd+0.25|pet.main.buff.frenzy.stack<3&(cooldown.bestial_wrath.ready&(!pet.main.buff.frenzy.up|talent.scent_of_blood)|talent.call_of_the_wild&cooldown.call_of_the_wild.ready)|talent.wild_call&charges_fractional>1.8" );
  cleave->add_action( "multishot,if=pet.main.buff.beast_cleave.remains<0.25+gcd&(!talent.bloody_frenzy|cooldown.call_of_the_wild.remains)" );
  cleave->add_action( "black_arrow,if=buff.beast_cleave.remains" );
  cleave->add_action( "dire_beast,if=talent.shadow_hounds" );
  cleave->add_action( "call_of_the_wild" );
  cleave->add_action( "bestial_wrath" );
  cleave->add_action( "bloodshed" );
  cleave->add_action( "kill_command,target_if=max:(target.health.pct<35|!talent.killer_instinct)*2+dot.a_murder_of_crows.refreshable" );
  cleave->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=buff.call_of_the_wild.up|talent.furious_assault|talent.black_arrow&(talent.barbed_scales|talent.savagery)|fight_remains<9" );
  cleave->add_action( "kill_shot" );
  cleave->add_action( "explosive_shot" );
  cleave->add_action( "dire_beast" );
  cleave->add_action( "lights_judgment,if=buff.bestial_wrath.down|target.time_to_die<5" );
  cleave->add_action( "cobra_shot,if=focus.time_to_max<gcd*2" );
  cleave->add_action( "bag_of_tricks,if=buff.bestial_wrath.down|target.time_to_die<5" );
  cleave->add_action( "arcane_torrent,if=(focus+focus.regen+30)<focus.max" );

  st->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=pet.main.buff.frenzy.up&pet.main.buff.frenzy.remains<=gcd+0.25|pet.main.buff.frenzy.stack<3&(cooldown.bestial_wrath.ready&(!pet.main.buff.frenzy.up|talent.scent_of_blood)|talent.call_of_the_wild&cooldown.call_of_the_wild.ready)" );
  st->add_action( "dire_beast" );
  st->add_action( "kill_command,if=talent.call_of_the_wild&cooldown.call_of_the_wild.remains<gcd+0.25" );
  st->add_action( "black_arrow,target_if=min:dot.serpent_sting.remains,if=talent.venoms_bite&dot.serpent_sting.refreshable" );
  st->add_action( "kill_shot,target_if=min:dot.serpent_sting.remains,if=talent.venoms_bite&dot.serpent_sting.refreshable" );
  st->add_action( "call_of_the_wild" );
  st->add_action( "bloodshed" );
  st->add_action( "bestial_wrath" );
  st->add_action( "kill_command" );
  st->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=talent.wild_call&charges_fractional>1.4|buff.call_of_the_wild.up|full_recharge_time<gcd&cooldown.bestial_wrath.remains|talent.scent_of_blood&(cooldown.bestial_wrath.remains<12+gcd)|talent.furious_assault|talent.black_arrow&(talent.barbed_scales|talent.savagery)|fight_remains<9" );
  st->add_action( "black_arrow" );
  st->add_action( "kill_shot" );
  st->add_action( "lights_judgment,if=buff.bestial_wrath.down|target.time_to_die<5" );
  st->add_action( "cobra_shot" );
  st->add_action( "bag_of_tricks,if=buff.bestial_wrath.down|target.time_to_die<5" );
  st->add_action( "arcane_pulse,if=buff.bestial_wrath.down|target.time_to_die<5" );
  st->add_action( "arcane_torrent,if=(focus+focus.regen+15)<focus.max" );

  trinkets->add_action( "variable,name=sync_ready,value=talent.call_of_the_wild&(prev_gcd.1.call_of_the_wild)|talent.bloodshed&(prev_gcd.1.bloodshed)|(!talent.call_of_the_wild&!talent.bloodshed)&(buff.bestial_wrath.up|cooldown.bestial_wrath.remains_guess<5)" );
  trinkets->add_action( "variable,name=sync_active,value=talent.call_of_the_wild&buff.call_of_the_wild.up|talent.bloodshed&prev_gcd.1.bloodshed|(!talent.call_of_the_wild&!talent.bloodshed)&buff.bestial_wrath.up" );
  trinkets->add_action( "variable,name=sync_remains,op=setif,value=cooldown.bestial_wrath.remains_guess,value_else=cooldown.call_of_the_wild.remains|cooldown.bloodshed.remains,condition=!talent.call_of_the_wild&!talent.bloodshed" );
  trinkets->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=trinket.1.has_use_buff&(variable.sync_ready&(variable.trinket_1_stronger|trinket.2.cooldown.remains)|!variable.sync_ready&(variable.trinket_1_stronger&(variable.sync_remains>trinket.1.cooldown.duration%3&fight_remains>trinket.1.cooldown.duration+20|trinket.2.has_use_buff&trinket.2.cooldown.remains>variable.sync_remains-15&trinket.2.cooldown.remains-5<variable.sync_remains&variable.sync_remains+45>fight_remains)|variable.trinket_2_stronger&(trinket.2.cooldown.remains&(trinket.2.cooldown.remains-5<variable.sync_remains&variable.sync_remains>=20|trinket.2.cooldown.remains-5>=variable.sync_remains&(variable.sync_remains>trinket.1.cooldown.duration%3|trinket.1.cooldown.duration<fight_remains&(variable.sync_remains+trinket.1.cooldown.duration>fight_remains)))|trinket.2.cooldown.ready&variable.sync_remains>20&variable.sync_remains<trinket.2.cooldown.duration%3)))|!trinket.1.has_use_buff&(trinket.1.cast_time=0|!variable.sync_active)&(!trinket.2.has_use_buff&(variable.trinket_1_stronger|trinket.2.cooldown.remains)|trinket.2.has_use_buff&(!variable.sync_active&variable.sync_remains>20|trinket.2.cooldown.remains>20))|fight_remains<25&(variable.trinket_1_stronger|trinket.2.cooldown.remains)" );
  trinkets->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=trinket.2.has_use_buff&(variable.sync_ready&(variable.trinket_2_stronger|trinket.1.cooldown.remains)|!variable.sync_ready&(variable.trinket_2_stronger&(variable.sync_remains>trinket.2.cooldown.duration%3&fight_remains>trinket.2.cooldown.duration+20|trinket.1.has_use_buff&trinket.1.cooldown.remains>variable.sync_remains-15&trinket.1.cooldown.remains-5<variable.sync_remains&variable.sync_remains+45>fight_remains)|variable.trinket_1_stronger&(trinket.1.cooldown.remains&(trinket.1.cooldown.remains-5<variable.sync_remains&variable.sync_remains>=20|trinket.1.cooldown.remains-5>=variable.sync_remains&(variable.sync_remains>trinket.2.cooldown.duration%3|trinket.2.cooldown.duration<fight_remains&(variable.sync_remains+trinket.2.cooldown.duration>fight_remains)))|trinket.1.cooldown.ready&variable.sync_remains>20&variable.sync_remains<trinket.1.cooldown.duration%3)))|!trinket.2.has_use_buff&(trinket.2.cast_time=0|!variable.sync_active)&(!trinket.1.has_use_buff&(variable.trinket_2_stronger|trinket.1.cooldown.remains)|trinket.1.has_use_buff&(!variable.sync_active&variable.sync_remains>20|trinket.1.cooldown.remains>20))|fight_remains<25&(variable.trinket_2_stronger|trinket.1.cooldown.remains)" );
}
//beast_mastery_apl_end

//beast_mastery_ptr_apl_start
void beast_mastery_ptr( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* cleave = p->get_action_priority_list( "cleave" );
  action_priority_list_t* st = p->get_action_priority_list( "st" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );

  precombat->add_action( "summon_pet" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=trinket_1_stronger,value=!trinket.2.has_cooldown|trinket.1.has_use_buff&(!trinket.2.has_use_buff|!trinket.1.is.mirror_of_fractured_tomorrows&(trinket.2.is.mirror_of_fractured_tomorrows|trinket.2.cooldown.duration<trinket.1.cooldown.duration|trinket.2.cast_time<trinket.1.cast_time|trinket.2.cast_time=trinket.1.cast_time&trinket.2.cooldown.duration=trinket.1.cooldown.duration))|!trinket.1.has_use_buff&(!trinket.2.has_use_buff&(trinket.2.cooldown.duration<trinket.1.cooldown.duration|trinket.2.cast_time<trinket.1.cast_time|trinket.2.cast_time=trinket.1.cast_time&trinket.2.cooldown.duration=trinket.1.cooldown.duration))" );
  precombat->add_action( "variable,name=trinket_2_stronger,value=!variable.trinket_1_stronger" );
  precombat->add_action( "bestial_wrath,if=talent.vicious_hunt&talent.call_of_the_wild" );

  default_->add_action( "auto_shot" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "call_action_list,name=st,if=active_enemies<2|!talent.beast_cleave&active_enemies<3" );
  default_->add_action( "call_action_list,name=cleave,if=active_enemies>2|talent.beast_cleave&active_enemies>1" );

  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.call_of_the_wild.up|talent.bloodshed&(prev_gcd.1.bloodshed)|!talent.call_of_the_wild&(buff.bestial_wrath.up|cooldown.bestial_wrath.remains<30)|fight_remains<16" );
  cds->add_action( "berserking,if=buff.call_of_the_wild.up|talent.bloodshed&(prev_gcd.1.bloodshed)|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<13" );
  cds->add_action( "blood_fury,if=buff.call_of_the_wild.up|talent.bloodshed&(prev_gcd.1.bloodshed)|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<16" );
  cds->add_action( "ancestral_call,if=buff.call_of_the_wild.up|talent.bloodshed&(prev_gcd.1.bloodshed)|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<16" );
  cds->add_action( "fireblood,if=buff.call_of_the_wild.up|talent.bloodshed&(prev_gcd.1.bloodshed)|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<9" );
  cds->add_action( "potion,if=buff.call_of_the_wild.up|talent.bloodshed&(prev_gcd.1.bloodshed)|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<31" );

  cleave->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=pet.main.buff.frenzy.up&pet.main.buff.frenzy.remains<=gcd+0.25|pet.main.buff.frenzy.stack<3&(cooldown.bestial_wrath.ready&(!pet.main.buff.frenzy.up|talent.scent_of_blood)|talent.call_of_the_wild&cooldown.call_of_the_wild.ready)|talent.wild_call&charges_fractional>1.8" );
  cleave->add_action( "multishot,if=pet.main.buff.beast_cleave.remains<0.25+gcd&(!talent.bloody_frenzy|cooldown.call_of_the_wild.remains)" );
  cleave->add_action( "black_arrow,if=buff.beast_cleave.remains" );
  cleave->add_action( "dire_beast,if=talent.shadow_hounds" );
  cleave->add_action( "call_of_the_wild" );
  cleave->add_action( "bestial_wrath" );
  cleave->add_action( "bloodshed" );
  cleave->add_action( "kill_command,target_if=max:(target.health.pct<35|!talent.killer_instinct)*2+dot.a_murder_of_crows.refreshable" );
  cleave->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=buff.call_of_the_wild.up|talent.furious_assault|talent.black_arrow&(talent.barbed_scales|talent.savagery)|fight_remains<9" );
  cleave->add_action( "kill_shot" );
  cleave->add_action( "explosive_shot" );
  cleave->add_action( "dire_beast" );
  cleave->add_action( "lights_judgment,if=buff.bestial_wrath.down|target.time_to_die<5" );
  cleave->add_action( "cobra_shot,if=focus.time_to_max<gcd*2" );
  cleave->add_action( "bag_of_tricks,if=buff.bestial_wrath.down|target.time_to_die<5" );
  cleave->add_action( "arcane_torrent,if=(focus+focus.regen+30)<focus.max" );

  st->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=pet.main.buff.frenzy.up&pet.main.buff.frenzy.remains<=gcd+0.25|pet.main.buff.frenzy.stack<3&(cooldown.bestial_wrath.ready&(!pet.main.buff.frenzy.up|talent.scent_of_blood)|talent.call_of_the_wild&cooldown.call_of_the_wild.ready)" );
  st->add_action( "dire_beast" );
  st->add_action( "kill_command,if=talent.call_of_the_wild&cooldown.call_of_the_wild.remains<gcd+0.25" );
  st->add_action( "black_arrow,target_if=min:dot.serpent_sting.remains,if=talent.venoms_bite&dot.serpent_sting.refreshable" );
  st->add_action( "kill_shot,target_if=min:dot.serpent_sting.remains,if=talent.venoms_bite&dot.serpent_sting.refreshable" );
  st->add_action( "call_of_the_wild" );
  st->add_action( "bloodshed" );
  st->add_action( "bestial_wrath" );
  st->add_action( "kill_command" );
  st->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=talent.wild_call&charges_fractional>1.4|buff.call_of_the_wild.up|full_recharge_time<gcd&cooldown.bestial_wrath.remains|talent.scent_of_blood&(cooldown.bestial_wrath.remains<12+gcd)|talent.furious_assault|talent.black_arrow&(talent.barbed_scales|talent.savagery)|fight_remains<9" );
  st->add_action( "black_arrow" );
  st->add_action( "kill_shot" );
  st->add_action( "lights_judgment,if=buff.bestial_wrath.down|target.time_to_die<5" );
  st->add_action( "cobra_shot" );
  st->add_action( "bag_of_tricks,if=buff.bestial_wrath.down|target.time_to_die<5" );
  st->add_action( "arcane_pulse,if=buff.bestial_wrath.down|target.time_to_die<5" );
  st->add_action( "arcane_torrent,if=(focus+focus.regen+15)<focus.max" );

  trinkets->add_action( "variable,name=sync_ready,value=talent.call_of_the_wild&(prev_gcd.1.call_of_the_wild)|talent.bloodshed&(prev_gcd.1.bloodshed)|(!talent.call_of_the_wild&!talent.bloodshed)&(buff.bestial_wrath.up|cooldown.bestial_wrath.remains_guess<5)" );
  trinkets->add_action( "variable,name=sync_active,value=talent.call_of_the_wild&buff.call_of_the_wild.up|talent.bloodshed&prev_gcd.1.bloodshed|(!talent.call_of_the_wild&!talent.bloodshed)&buff.bestial_wrath.up" );
  trinkets->add_action( "variable,name=sync_remains,op=setif,value=cooldown.bestial_wrath.remains_guess,value_else=cooldown.call_of_the_wild.remains|cooldown.bloodshed.remains,condition=!talent.call_of_the_wild&!talent.bloodshed" );
  trinkets->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=trinket.1.has_use_buff&(variable.sync_ready&(variable.trinket_1_stronger|trinket.2.cooldown.remains)|!variable.sync_ready&(variable.trinket_1_stronger&(variable.sync_remains>trinket.1.cooldown.duration%3&fight_remains>trinket.1.cooldown.duration+20|trinket.2.has_use_buff&trinket.2.cooldown.remains>variable.sync_remains-15&trinket.2.cooldown.remains-5<variable.sync_remains&variable.sync_remains+45>fight_remains)|variable.trinket_2_stronger&(trinket.2.cooldown.remains&(trinket.2.cooldown.remains-5<variable.sync_remains&variable.sync_remains>=20|trinket.2.cooldown.remains-5>=variable.sync_remains&(variable.sync_remains>trinket.1.cooldown.duration%3|trinket.1.cooldown.duration<fight_remains&(variable.sync_remains+trinket.1.cooldown.duration>fight_remains)))|trinket.2.cooldown.ready&variable.sync_remains>20&variable.sync_remains<trinket.2.cooldown.duration%3)))|!trinket.1.has_use_buff&(trinket.1.cast_time=0|!variable.sync_active)&(!trinket.2.has_use_buff&(variable.trinket_1_stronger|trinket.2.cooldown.remains)|trinket.2.has_use_buff&(!variable.sync_active&variable.sync_remains>20|trinket.2.cooldown.remains>20))|fight_remains<25&(variable.trinket_1_stronger|trinket.2.cooldown.remains)" );
  trinkets->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=trinket.2.has_use_buff&(variable.sync_ready&(variable.trinket_2_stronger|trinket.1.cooldown.remains)|!variable.sync_ready&(variable.trinket_2_stronger&(variable.sync_remains>trinket.2.cooldown.duration%3&fight_remains>trinket.2.cooldown.duration+20|trinket.1.has_use_buff&trinket.1.cooldown.remains>variable.sync_remains-15&trinket.1.cooldown.remains-5<variable.sync_remains&variable.sync_remains+45>fight_remains)|variable.trinket_1_stronger&(trinket.1.cooldown.remains&(trinket.1.cooldown.remains-5<variable.sync_remains&variable.sync_remains>=20|trinket.1.cooldown.remains-5>=variable.sync_remains&(variable.sync_remains>trinket.2.cooldown.duration%3|trinket.2.cooldown.duration<fight_remains&(variable.sync_remains+trinket.2.cooldown.duration>fight_remains)))|trinket.1.cooldown.ready&variable.sync_remains>20&variable.sync_remains<trinket.1.cooldown.duration%3)))|!trinket.2.has_use_buff&(trinket.2.cast_time=0|!variable.sync_active)&(!trinket.1.has_use_buff&(variable.trinket_2_stronger|trinket.1.cooldown.remains)|trinket.1.has_use_buff&(!variable.sync_active&variable.sync_remains>20|trinket.1.cooldown.remains>20))|fight_remains<25&(variable.trinket_2_stronger|trinket.1.cooldown.remains)" );
}
//beast_mastery_ptr_apl_end

//marksmanship_apl_start
void marksmanship( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* st = p->get_action_priority_list( "st" );
  action_priority_list_t* trickshots = p->get_action_priority_list( "trickshots" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );

  precombat->add_action( "summon_pet,if=!talent.lone_wolf" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=trinket_1_stronger,value=!trinket.2.has_cooldown|trinket.1.has_use_buff&(!trinket.2.has_use_buff|!trinket.1.is.mirror_of_fractured_tomorrows&(trinket.2.is.mirror_of_fractured_tomorrows|trinket.2.cooldown.duration<trinket.1.cooldown.duration|trinket.2.cast_time<trinket.1.cast_time|trinket.2.cast_time=trinket.1.cast_time&trinket.2.cooldown.duration=trinket.1.cooldown.duration))|!trinket.1.has_use_buff&(!trinket.2.has_use_buff&(trinket.2.cooldown.duration<trinket.1.cooldown.duration|trinket.2.cast_time<trinket.1.cast_time|trinket.2.cast_time=trinket.1.cast_time&trinket.2.cooldown.duration=trinket.1.cooldown.duration))", "Determine the stronger trinket to sync with cooldowns. In descending priority: buff effects > damage effects, longer > shorter cooldowns, longer > shorter cast times." );
  precombat->add_action( "variable,name=trinket_2_stronger,value=!variable.trinket_1_stronger" );
  precombat->add_action( "salvo,precast_time=10" );
  precombat->add_action( "aimed_shot,if=active_enemies<3&(!talent.volley|active_enemies<2)", "Precast Aimed Shot on one or two targets unless we could cleave it with Volley on two targets." );
  precombat->add_action( "steady_shot,if=active_enemies>2|talent.volley&active_enemies=2", "Otherwise precast Steady Shot on two targets if we are saving Aimed Shot to cleave with Volley, otherwise on three or more targets." );

  default_->add_action( "variable,name=trueshot_ready,value=cooldown.trueshot.ready&(!raid_event.adds.exists&(!talent.bullseye|fight_remains>cooldown.trueshot.duration_guess+buff.trueshot.duration%2|buff.bullseye.stack=buff.bullseye.max_stack)&(!trinket.1.has_use_buff|trinket.1.cooldown.remains>30|trinket.1.cooldown.ready)&(!trinket.2.has_use_buff|trinket.2.cooldown.remains>30|trinket.2.cooldown.ready)|raid_event.adds.exists&(!raid_event.adds.up&(raid_event.adds.duration+raid_event.adds.in<25|raid_event.adds.in>60)|raid_event.adds.up&raid_event.adds.remains>10)|fight_remains<25)", "Determine if it is a good time to use Trueshot. Raid event optimization takes priority so usage is saved for multiple targets as long as it won't delay over half its duration. Otherwise allow for small delays to line up buff effect trinkets, and when using Bullseye, delay the last usage of the fight for max stacks." );
  default_->add_action( "auto_shot" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "call_action_list,name=st,if=active_enemies<3|!talent.trick_shots" );
  default_->add_action( "call_action_list,name=trickshots,if=active_enemies>2" );

  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.trueshot.remains>12|fight_remains<13", "Call for Power Infusion when Trueshot is up." );
  cds->add_action( "berserking,if=buff.trueshot.up|fight_remains<13" );
  cds->add_action( "blood_fury,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<16" );
  cds->add_action( "ancestral_call,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<16" );
  cds->add_action( "fireblood,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<9" );
  cds->add_action( "lights_judgment,if=buff.trueshot.down" );
  cds->add_action( "potion,if=buff.trueshot.up&(buff.bloodlust.up|target.health.pct<20)|fight_remains<31" );
  cds->add_action( "salvo,if=active_enemies>2|cooldown.volley.remains<10" );

  st->add_action( "black_arrow,if=buff.trueshot.down" );
  st->add_action( "kill_shot,if=buff.razor_fragments.up" );
  st->add_action( "steady_shot,if=talent.steady_focus&buff.steady_focus.remains<execute_time&buff.trueshot.down" );
  st->add_action( "volley,if=active_enemies>1|buff.salvo.up" );
  st->add_action( "explosive_shot,if=active_enemies>1&buff.trick_shots.down" );
  st->add_action( "trueshot,if=variable.trueshot_ready&talent.multishot", "TODO: figure out why this is such a big gain for aoe builds in ST" );
  st->add_action( "multishot,if=buff.salvo.up&!talent.volley", "Trigger Salvo if Volley isn't being used to trigger it." );
  st->add_action( "aimed_shot,target_if=min:dot.serpent_sting.remains+action.serpent_sting.in_flight_to_target*99,if=talent.black_arrow&talent.readiness&buff.trueshot.up", "As a Dark Ranger with Readiness, Aimed Shot over Rapid Fire in Trueshot to get more Deathblows." );
  st->add_action( "rapid_fire,if=!talent.lunar_storm|(!cooldown.lunar_storm.remains|cooldown.lunar_storm.remains>8|(action.wailing_arrow.ready&talent.readiness)|buff.trueshot.up)", "Hold Rapid Fire for up to 8s to proc Lunar Storm. Ignore Lunar Storm if Rapid Fire's cooldown can be reset with Wailing Arrow. Ignore all of this in Trueshot or as a Dark Ranger." );
  st->add_action( "wailing_arrow,target_if=min:dot.serpent_sting.remains+action.serpent_sting.in_flight_to_target*99", "With Wailing Arrow and Aimed Shot, target enemies without Serpent Sting or the enemy with the lowest remaining Serpent Sting." );
  st->add_action( "aimed_shot,target_if=min:dot.serpent_sting.remains+action.serpent_sting.in_flight_to_target*99" );
  st->add_action( "black_arrow" );
  st->add_action( "kill_shot" );
  st->add_action( "multishot,if=buff.precise_shots.up&active_enemies>1&(talent.symphonic_arsenal|talent.small_game_hunter)" );
  st->add_action( "arcane_shot,if=buff.precise_shots.up" );
  st->add_action( "trueshot,if=variable.trueshot_ready" );
  st->add_action( "volley" );
  st->add_action( "explosive_shot" );
  st->add_action( "steady_shot" );

  trickshots->add_action( "black_arrow,if=buff.trick_shots.remains>execute_time&buff.razor_fragments.up" );
  trickshots->add_action( "steady_shot,if=talent.steady_focus&buff.steady_focus.remains<execute_time&buff.trueshot.down" );
  trickshots->add_action( "explosive_shot" );
  trickshots->add_action( "volley" );
  trickshots->add_action( "black_arrow,if=!talent.razor_fragments&buff.trick_shots.remains>execute_time" );
  trickshots->add_action( "kill_shot,if=buff.razor_fragments.up" );
  trickshots->add_action( "trueshot,if=variable.trueshot_ready" );
  trickshots->add_action( "barrage,if=talent.rapid_fire_barrage&buff.trick_shots.remains>execute_time" );
  trickshots->add_action( "rapid_fire,if=buff.trick_shots.remains>execute_time&(!talent.lunar_storm|(!cooldown.lunar_storm.remains|cooldown.lunar_storm.remains>5|(action.wailing_arrow.ready&talent.readiness)|buff.trueshot.up))", "Hold Rapid Fire for up to 5s to proc Lunar Storm. Ignore Lunar Storm if Rapid Fire's cooldown can be reset with Wailing Arrow. Ignore all of this in Trueshot or as a Dark Ranger." );
  trickshots->add_action( "wailing_arrow,target_if=min:dot.serpent_sting.remains+action.serpent_sting.in_flight_to_target*99", "With Wailing Arrow and Aimed Shot, target enemies without Serpent Sting or the enemy with the lowest remaining Serpent Sting." );
  trickshots->add_action( "aimed_shot,target_if=min:dot.serpent_sting.remains+action.serpent_sting.in_flight_to_target*99,if=buff.trick_shots.remains>execute_time&buff.precise_shots.down" );
  trickshots->add_action( "multishot,if=buff.precise_shots.up|buff.trick_shots.down" );
  trickshots->add_action( "bag_of_tricks,if=buff.trueshot.down" );
  trickshots->add_action( "steady_shot" );

  trinkets->add_action( "variable,name=sync_ready,value=variable.trueshot_ready", "True if effects that are desirable to sync a trinket buff with are ready." );
  trinkets->add_action( "variable,name=sync_active,value=buff.trueshot.up", "True if effecs that are desirable to sync a trinket buff with are active." );
  trinkets->add_action( "variable,name=sync_remains,value=cooldown.trueshot.remains_guess", "Time until the effects that are desirable to sync a trinket buff with will be ready." );
  trinkets->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=trinket.1.has_use_buff&(variable.sync_ready&(variable.trinket_1_stronger|trinket.2.cooldown.remains)|!variable.sync_ready&(variable.trinket_1_stronger&(variable.sync_remains>trinket.1.cooldown.duration%3&fight_remains>trinket.1.cooldown.duration+20|trinket.2.has_use_buff&trinket.2.cooldown.remains>variable.sync_remains-15&trinket.2.cooldown.remains-5<variable.sync_remains&variable.sync_remains+45>fight_remains)|variable.trinket_2_stronger&(trinket.2.cooldown.remains&(trinket.2.cooldown.remains-5<variable.sync_remains&variable.sync_remains>=20|trinket.2.cooldown.remains-5>=variable.sync_remains&(variable.sync_remains>trinket.1.cooldown.duration%3|trinket.1.cooldown.duration<fight_remains&(variable.sync_remains+trinket.1.cooldown.duration>fight_remains)))|trinket.2.cooldown.ready&variable.sync_remains>20&variable.sync_remains<trinket.2.cooldown.duration%3)))|!trinket.1.has_use_buff&(trinket.1.cast_time=0|!variable.sync_active)&(!trinket.2.has_use_buff&(variable.trinket_1_stronger|trinket.2.cooldown.remains)|trinket.2.has_use_buff&(variable.sync_remains>20|trinket.2.cooldown.remains>20))|fight_remains<25&(variable.trinket_1_stronger|trinket.2.cooldown.remains)", "Uses buff effect trinkets with cooldowns and is willing to delay usage up to half the trinket cooldown if it won't lose a usage in the fight. Fills in downtime with weaker buff effects if they won't also be saved for later cooldowns (happens if it won't delay over half the trinket cooldown and a stronger trinket won't be up in time) or damage effects if they won't inferfere with any buff effect usage. Intended to be slot-agnostic so that any order of the same trinket pair should result in the same usage." );
  trinkets->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=trinket.2.has_use_buff&(variable.sync_ready&(variable.trinket_2_stronger|trinket.1.cooldown.remains)|!variable.sync_ready&(variable.trinket_2_stronger&(variable.sync_remains>trinket.2.cooldown.duration%3&fight_remains>trinket.2.cooldown.duration+20|trinket.1.has_use_buff&trinket.1.cooldown.remains>variable.sync_remains-15&trinket.1.cooldown.remains-5<variable.sync_remains&variable.sync_remains+45>fight_remains)|variable.trinket_1_stronger&(trinket.1.cooldown.remains&(trinket.1.cooldown.remains-5<variable.sync_remains&variable.sync_remains>=20|trinket.1.cooldown.remains-5>=variable.sync_remains&(variable.sync_remains>trinket.2.cooldown.duration%3|trinket.2.cooldown.duration<fight_remains&(variable.sync_remains+trinket.2.cooldown.duration>fight_remains)))|trinket.1.cooldown.ready&variable.sync_remains>20&variable.sync_remains<trinket.1.cooldown.duration%3)))|!trinket.2.has_use_buff&(trinket.2.cast_time=0|!variable.sync_active)&(!trinket.1.has_use_buff&(variable.trinket_2_stronger|trinket.1.cooldown.remains)|trinket.1.has_use_buff&(variable.sync_remains>20|trinket.1.cooldown.remains>20))|fight_remains<25&(variable.trinket_2_stronger|trinket.1.cooldown.remains)" );
}
//marksmanship_apl_end

//marksmanship_ptr_apl_start
void marksmanship_ptr( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* st = p->get_action_priority_list( "st" );
  action_priority_list_t* trickshots = p->get_action_priority_list( "trickshots" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );

  precombat->add_action( "summon_pet,if=!talent.lone_wolf" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=trinket_1_stronger,value=!trinket.2.has_cooldown|trinket.1.has_use_buff&(!trinket.2.has_use_buff|!trinket.1.is.mirror_of_fractured_tomorrows&(trinket.2.is.mirror_of_fractured_tomorrows|trinket.2.cooldown.duration<trinket.1.cooldown.duration|trinket.2.cast_time<trinket.1.cast_time|trinket.2.cast_time=trinket.1.cast_time&trinket.2.cooldown.duration=trinket.1.cooldown.duration))|!trinket.1.has_use_buff&(!trinket.2.has_use_buff&(trinket.2.cooldown.duration<trinket.1.cooldown.duration|trinket.2.cast_time<trinket.1.cast_time|trinket.2.cast_time=trinket.1.cast_time&trinket.2.cooldown.duration=trinket.1.cooldown.duration))", "Determine the stronger trinket to sync with cooldowns. In descending priority: buff effects > damage effects, longer > shorter cooldowns, longer > shorter cast times." );
  precombat->add_action( "variable,name=trinket_2_stronger,value=!variable.trinket_1_stronger" );
  precombat->add_action( "salvo,precast_time=10" );
  precombat->add_action( "aimed_shot,if=active_enemies<3&(!talent.volley|active_enemies<2)", "Precast Aimed Shot on one or two targets unless we could cleave it with Volley on two targets." );
  precombat->add_action( "steady_shot,if=active_enemies>2|talent.volley&active_enemies=2", "Otherwise precast Steady Shot on two targets if we are saving Aimed Shot to cleave with Volley, otherwise on three or more targets." );

  default_->add_action( "variable,name=trueshot_ready,value=cooldown.trueshot.ready&(!raid_event.adds.exists&(!talent.bullseye|fight_remains>cooldown.trueshot.duration_guess+buff.trueshot.duration%2|buff.bullseye.stack=buff.bullseye.max_stack)&(!trinket.1.has_use_buff|trinket.1.cooldown.remains>30|trinket.1.cooldown.ready)&(!trinket.2.has_use_buff|trinket.2.cooldown.remains>30|trinket.2.cooldown.ready)|raid_event.adds.exists&(!raid_event.adds.up&(raid_event.adds.duration+raid_event.adds.in<25|raid_event.adds.in>60)|raid_event.adds.up&raid_event.adds.remains>10)|fight_remains<25)", "Determine if it is a good time to use Trueshot. Raid event optimization takes priority so usage is saved for multiple targets as long as it won't delay over half its duration. Otherwise allow for small delays to line up buff effect trinkets, and when using Bullseye, delay the last usage of the fight for max stacks." );
  default_->add_action( "auto_shot" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "call_action_list,name=st,if=active_enemies<3|!talent.trick_shots" );
  default_->add_action( "call_action_list,name=trickshots,if=active_enemies>2" );

  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.trueshot.remains>12|fight_remains<13", "Call for Power Infusion when Trueshot is up." );
  cds->add_action( "berserking,if=buff.trueshot.up|fight_remains<13" );
  cds->add_action( "blood_fury,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<16" );
  cds->add_action( "ancestral_call,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<16" );
  cds->add_action( "fireblood,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<9" );
  cds->add_action( "lights_judgment,if=buff.trueshot.down" );
  cds->add_action( "potion,if=buff.trueshot.up&(buff.bloodlust.up|target.health.pct<20)|fight_remains<31" );
  cds->add_action( "salvo,if=active_enemies>2|cooldown.volley.remains<10" );

  st->add_action( "black_arrow,if=buff.trueshot.down" );
  st->add_action( "kill_shot,if=buff.razor_fragments.up" );
  st->add_action( "steady_shot,if=talent.steady_focus&buff.steady_focus.remains<execute_time&buff.trueshot.down" );
  st->add_action( "volley,if=active_enemies>1|buff.salvo.up" );
  st->add_action( "explosive_shot,if=active_enemies>1&buff.trick_shots.down" );
  st->add_action( "trueshot,if=variable.trueshot_ready&talent.multishot", "TODO: figure out why this is such a big gain for aoe builds in ST" );
  st->add_action( "multishot,if=buff.salvo.up&!talent.volley", "Trigger Salvo if Volley isn't being used to trigger it." );
  st->add_action( "aimed_shot,target_if=min:dot.serpent_sting.remains+action.serpent_sting.in_flight_to_target*99,if=talent.black_arrow&talent.readiness&buff.trueshot.up", "As a Dark Ranger with Readiness, Aimed Shot over Rapid Fire in Trueshot to get more Deathblows." );
  st->add_action( "rapid_fire,if=!talent.lunar_storm|(!cooldown.lunar_storm.remains|cooldown.lunar_storm.remains>8|(action.wailing_arrow.ready&talent.readiness)|buff.trueshot.up)", "Hold Rapid Fire for up to 8s to proc Lunar Storm. Ignore Lunar Storm if Rapid Fire's cooldown can be reset with Wailing Arrow. Ignore all of this in Trueshot or as a Dark Ranger." );
  st->add_action( "wailing_arrow,target_if=min:dot.serpent_sting.remains+action.serpent_sting.in_flight_to_target*99", "With Wailing Arrow and Aimed Shot, target enemies without Serpent Sting or the enemy with the lowest remaining Serpent Sting." );
  st->add_action( "aimed_shot,target_if=min:dot.serpent_sting.remains+action.serpent_sting.in_flight_to_target*99" );
  st->add_action( "black_arrow" );
  st->add_action( "kill_shot" );
  st->add_action( "multishot,if=buff.precise_shots.up&active_enemies>1&(talent.symphonic_arsenal|talent.small_game_hunter)" );
  st->add_action( "arcane_shot,if=buff.precise_shots.up" );
  st->add_action( "trueshot,if=variable.trueshot_ready" );
  st->add_action( "volley" );
  st->add_action( "explosive_shot" );
  st->add_action( "steady_shot" );

  trickshots->add_action( "black_arrow,if=buff.trick_shots.remains>execute_time&buff.razor_fragments.up" );
  trickshots->add_action( "steady_shot,if=talent.steady_focus&buff.steady_focus.remains<execute_time&buff.trueshot.down" );
  trickshots->add_action( "explosive_shot" );
  trickshots->add_action( "volley" );
  trickshots->add_action( "black_arrow,if=!talent.razor_fragments&buff.trick_shots.remains>execute_time" );
  trickshots->add_action( "kill_shot,if=buff.razor_fragments.up" );
  trickshots->add_action( "trueshot,if=variable.trueshot_ready" );
  trickshots->add_action( "barrage,if=talent.rapid_fire_barrage&buff.trick_shots.remains>execute_time" );
  trickshots->add_action( "rapid_fire,if=buff.trick_shots.remains>execute_time&(!talent.lunar_storm|(!cooldown.lunar_storm.remains|cooldown.lunar_storm.remains>5|(action.wailing_arrow.ready&talent.readiness)|buff.trueshot.up))", "Hold Rapid Fire for up to 5s to proc Lunar Storm. Ignore Lunar Storm if Rapid Fire's cooldown can be reset with Wailing Arrow. Ignore all of this in Trueshot or as a Dark Ranger." );
  trickshots->add_action( "wailing_arrow,target_if=min:dot.serpent_sting.remains+action.serpent_sting.in_flight_to_target*99", "With Wailing Arrow and Aimed Shot, target enemies without Serpent Sting or the enemy with the lowest remaining Serpent Sting." );
  trickshots->add_action( "aimed_shot,target_if=min:dot.serpent_sting.remains+action.serpent_sting.in_flight_to_target*99,if=buff.trick_shots.remains>execute_time&buff.precise_shots.down" );
  trickshots->add_action( "multishot,if=buff.precise_shots.up|buff.trick_shots.down" );
  trickshots->add_action( "bag_of_tricks,if=buff.trueshot.down" );
  trickshots->add_action( "steady_shot" );

  trinkets->add_action( "variable,name=sync_ready,value=variable.trueshot_ready", "True if effects that are desirable to sync a trinket buff with are ready." );
  trinkets->add_action( "variable,name=sync_active,value=buff.trueshot.up", "True if effecs that are desirable to sync a trinket buff with are active." );
  trinkets->add_action( "variable,name=sync_remains,value=cooldown.trueshot.remains_guess", "Time until the effects that are desirable to sync a trinket buff with will be ready." );
  trinkets->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=trinket.1.has_use_buff&(variable.sync_ready&(variable.trinket_1_stronger|trinket.2.cooldown.remains)|!variable.sync_ready&(variable.trinket_1_stronger&(variable.sync_remains>trinket.1.cooldown.duration%3&fight_remains>trinket.1.cooldown.duration+20|trinket.2.has_use_buff&trinket.2.cooldown.remains>variable.sync_remains-15&trinket.2.cooldown.remains-5<variable.sync_remains&variable.sync_remains+45>fight_remains)|variable.trinket_2_stronger&(trinket.2.cooldown.remains&(trinket.2.cooldown.remains-5<variable.sync_remains&variable.sync_remains>=20|trinket.2.cooldown.remains-5>=variable.sync_remains&(variable.sync_remains>trinket.1.cooldown.duration%3|trinket.1.cooldown.duration<fight_remains&(variable.sync_remains+trinket.1.cooldown.duration>fight_remains)))|trinket.2.cooldown.ready&variable.sync_remains>20&variable.sync_remains<trinket.2.cooldown.duration%3)))|!trinket.1.has_use_buff&(trinket.1.cast_time=0|!variable.sync_active)&(!trinket.2.has_use_buff&(variable.trinket_1_stronger|trinket.2.cooldown.remains)|trinket.2.has_use_buff&(variable.sync_remains>20|trinket.2.cooldown.remains>20))|fight_remains<25&(variable.trinket_1_stronger|trinket.2.cooldown.remains)", "Uses buff effect trinkets with cooldowns and is willing to delay usage up to half the trinket cooldown if it won't lose a usage in the fight. Fills in downtime with weaker buff effects if they won't also be saved for later cooldowns (happens if it won't delay over half the trinket cooldown and a stronger trinket won't be up in time) or damage effects if they won't inferfere with any buff effect usage. Intended to be slot-agnostic so that any order of the same trinket pair should result in the same usage." );
  trinkets->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=trinket.2.has_use_buff&(variable.sync_ready&(variable.trinket_2_stronger|trinket.1.cooldown.remains)|!variable.sync_ready&(variable.trinket_2_stronger&(variable.sync_remains>trinket.2.cooldown.duration%3&fight_remains>trinket.2.cooldown.duration+20|trinket.1.has_use_buff&trinket.1.cooldown.remains>variable.sync_remains-15&trinket.1.cooldown.remains-5<variable.sync_remains&variable.sync_remains+45>fight_remains)|variable.trinket_1_stronger&(trinket.1.cooldown.remains&(trinket.1.cooldown.remains-5<variable.sync_remains&variable.sync_remains>=20|trinket.1.cooldown.remains-5>=variable.sync_remains&(variable.sync_remains>trinket.2.cooldown.duration%3|trinket.2.cooldown.duration<fight_remains&(variable.sync_remains+trinket.2.cooldown.duration>fight_remains)))|trinket.1.cooldown.ready&variable.sync_remains>20&variable.sync_remains<trinket.1.cooldown.duration%3)))|!trinket.2.has_use_buff&(trinket.2.cast_time=0|!variable.sync_active)&(!trinket.1.has_use_buff&(variable.trinket_2_stronger|trinket.1.cooldown.remains)|trinket.1.has_use_buff&(variable.sync_remains>20|trinket.1.cooldown.remains>20))|fight_remains<25&(variable.trinket_2_stronger|trinket.1.cooldown.remains)" );
}
//marksmanship_ptr_apl_end

//survival_apl_start
void survival( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* plst = p->get_action_priority_list( "plst" );
  action_priority_list_t* plcleave = p->get_action_priority_list( "plcleave" );
  action_priority_list_t* sentst = p->get_action_priority_list( "sentst" );
  action_priority_list_t* sentcleave = p->get_action_priority_list( "sentcleave" );

  precombat->add_action( "summon_pet" );
  precombat->add_action( "use_item,name=imperfect_ascendancy_serum" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  default_->add_action( "auto_attack" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=plst,if=active_enemies<3&talent.vicious_hunt" );
  default_->add_action( "call_action_list,name=plcleave,if=active_enemies>2&talent.vicious_hunt" );
  default_->add_action( "call_action_list,name=sentst,if=active_enemies<3&!talent.vicious_hunt" );
  default_->add_action( "call_action_list,name=sentcleave,if=active_enemies>2&!talent.vicious_hunt" );
  default_->add_action( "arcane_torrent", "simply fires off if there is absolutely nothing else to press." );
  default_->add_action( "bag_of_tricks" );
  default_->add_action( "lights_judgment" );

  cds->add_action( "blood_fury,if=buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault", "COOLDOWNS ACTIONLIST" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "harpoon,if=prev.kill_command" );
  cds->add_action( "ancestral_call,if=buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "fireblood,if=buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "berserking,if=buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault|time_to_die<13" );
  cds->add_action( "muzzle" );
  cds->add_action( "potion,if=target.time_to_die<25|buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "use_item,name=imperfect_ascendancy_serum,use_off_gcd=1,if=gcd.remains>gcd.max-0.1" );
  cds->add_action( "use_item,name=mad_queens_mandate,if=(time_to_die<10|time_to_die>120)&(trinket.skardyns_grace.cooldown.remains|!equipped.skardyns_grace)|time_to_die<10" );
  cds->add_action( "use_items,if=cooldown.coordinated_assault.remains|cooldown.spearhead.remains" );
  cds->add_action( "aspect_of_the_eagle,if=target.distance>=6" );

  plst->add_action( "raptor_bite,target_if=max:dot.serpent_sting.remains,if=buff.howl_of_the_pack.up&pet.main.buff.pack_coordination.up&buff.howl_of_the_pack.remains<gcd", "PL PACK LEADER SINGLE TARGET ACTIONLIST." );
  plst->add_action( "kill_command,target_if=min:bloodseeker.remains,if=(buff.relentless_primal_ferocity.up&buff.tip_of_the_spear.stack<1)" );
  plst->add_action( "butchery,if=buff.scattered_prey.up&buff.scattered_prey.remains<gcd" );
  plst->add_action( "spearhead,if=cooldown.coordinated_assault.remains" );
  plst->add_action( "raptor_bite,target_if=min:dot.serpent_sting.remains,if=!dot.serpent_sting.ticking&target.time_to_die>12&(!talent.contagious_reagents|active_dot.serpent_sting=0)" );
  plst->add_action( "raptor_bite,target_if=max:dot.serpent_sting.remains,if=talent.contagious_reagents&active_dot.serpent_sting<active_enemies&dot.serpent_sting.remains" );
  plst->add_action( "butchery" );
  plst->add_action( "flanking_strike,if=buff.tip_of_the_spear.stack=2|buff.tip_of_the_spear.stack=1" );
  plst->add_action( "kill_shot,if=buff.tip_of_the_spear.stack>0" );
  plst->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0&cooldown.wildfire_bomb.charges_fractional>1.4|cooldown.wildfire_bomb.charges_fractional>1.9|cooldown.coordinated_assault.remains<2*gcd&talent.bombardier" );
  plst->add_action( "explosive_shot" );
  plst->add_action( "coordinated_assault,if=!talent.bombardier|talent.bombardier&cooldown.wildfire_bomb.charges_fractional<1" );
  plst->add_action( "fury_of_the_eagle,if=buff.tip_of_the_spear.stack>0&(!raid_event.adds.exists|raid_event.adds.exists&raid_event.adds.in>40)" );
  plst->add_action( "raptor_bite,if=buff.furious_assault.up" );
  plst->add_action( "kill_command,target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max&(!buff.relentless_primal_ferocity.up|(buff.relentless_primal_ferocity.up&buff.tip_of_the_spear.stack<1|focus<30))" );
  plst->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0&(!raid_event.adds.exists|raid_event.adds.exists&raid_event.adds.in>15)" );
  plst->add_action( "raptor_bite,target_if=min:dot.serpent_sting.remains,if=!talent.contagious_reagents" );
  plst->add_action( "raptor_bite,target_if=max:dot.serpent_sting.remains" );

  plcleave->add_action( "spearhead,if=cooldown.coordinated_assault.remains", "PACK LEADER AOE ACTIONLIST" );
  plcleave->add_action( "kill_command,target_if=min:bloodseeker.remains,if=buff.relentless_primal_ferocity.up&buff.tip_of_the_spear.stack<1" );
  plcleave->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0&cooldown.wildfire_bomb.charges_fractional>1.7|cooldown.wildfire_bomb.charges_fractional>1.9|cooldown.coordinated_assault.remains<2*gcd|talent.butchery&cooldown.butchery.remains<gcd" );
  plcleave->add_action( "flanking_strike,if=buff.tip_of_the_spear.stack=2|buff.tip_of_the_spear.stack=1" );
  plcleave->add_action( "butchery" );
  plcleave->add_action( "explosive_shot" );
  plcleave->add_action( "coordinated_assault,if=!talent.bombardier|talent.bombardier&cooldown.wildfire_bomb.charges_fractional<1" );
  plcleave->add_action( "fury_of_the_eagle,if=buff.tip_of_the_spear.stack>0" );
  plcleave->add_action( "kill_shot,if=buff.deathblow.remains" );
  plcleave->add_action( "kill_command,target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max" );
  plcleave->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0" );
  plcleave->add_action( "kill_shot" );
  plcleave->add_action( "kill_command,target_if=min:bloodseeker.remains" );
  plcleave->add_action( "raptor_bite" );

  sentst->add_action( "wildfire_bomb,if=!cooldown.lunar_storm.remains", "SENT SENTINEL | DEFAULT SINGLE TARGET ACTIONLIST." );
  sentst->add_action( "kill_command,target_if=min:bloodseeker.remains,if=(buff.relentless_primal_ferocity.up&buff.tip_of_the_spear.stack<1)" );
  sentst->add_action( "spearhead,if=cooldown.coordinated_assault.remains" );
  sentst->add_action( "raptor_bite,target_if=min:dot.serpent_sting.remains,if=!dot.serpent_sting.ticking&target.time_to_die>12&(!talent.contagious_reagents|active_dot.serpent_sting=0)" );
  sentst->add_action( "raptor_bite,target_if=max:dot.serpent_sting.remains,if=talent.contagious_reagents&active_dot.serpent_sting<active_enemies&dot.serpent_sting.remains" );
  sentst->add_action( "flanking_strike,if=buff.tip_of_the_spear.stack=2|buff.tip_of_the_spear.stack=1" );
  sentst->add_action( "wildfire_bomb,if=(cooldown.lunar_storm.remains>full_recharge_time-gcd)&(buff.tip_of_the_spear.stack>0&cooldown.wildfire_bomb.charges_fractional>1.7|cooldown.wildfire_bomb.charges_fractional>1.9)|(talent.bombardier&cooldown.coordinated_assault.remains<2*gcd)" );
  sentst->add_action( "butchery" );
  sentst->add_action( "coordinated_assault,if=!talent.bombardier|talent.bombardier&cooldown.wildfire_bomb.charges_fractional<1" );
  sentst->add_action( "explosive_shot" );
  sentst->add_action( "fury_of_the_eagle,if=buff.tip_of_the_spear.stack>0" );
  sentst->add_action( "kill_shot" );
  sentst->add_action( "kill_command,target_if=min:bloodseeker.remains,if=buff.tip_of_the_spear.stack<1&cooldown.flanking_strike.remains<gcd" );
  sentst->add_action( "kill_command,target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max&(!buff.relentless_primal_ferocity.up|(buff.relentless_primal_ferocity.up&(buff.tip_of_the_spear.stack<2|focus<30)))" );
  sentst->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0&cooldown.lunar_storm.remains>full_recharge_time&(!raid_event.adds.exists|raid_event.adds.exists&raid_event.adds.in>15)" );
  sentst->add_action( "raptor_bite,target_if=min:dot.serpent_sting.remains,if=!talent.contagious_reagents" );
  sentst->add_action( "raptor_bite,target_if=max:dot.serpent_sting.remains" );

  sentcleave->add_action( "wildfire_bomb,if=!cooldown.lunar_storm.remains", "SENTINEL | DEFAULT AOE ACTIONLIST" );
  sentcleave->add_action( "kill_command,target_if=min:bloodseeker.remains,if=buff.relentless_primal_ferocity.up&buff.tip_of_the_spear.stack<1" );
  sentcleave->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0&cooldown.wildfire_bomb.charges_fractional>1.7|cooldown.wildfire_bomb.charges_fractional>1.9|(talent.bombardier&cooldown.coordinated_assault.remains<2*gcd)|talent.butchery&cooldown.butchery.remains<gcd" );
  sentcleave->add_action( "butchery" );
  sentcleave->add_action( "explosive_shot" );
  sentcleave->add_action( "coordinated_assault,if=!talent.bombardier|talent.bombardier&cooldown.wildfire_bomb.charges_fractional<1" );
  sentcleave->add_action( "fury_of_the_eagle,if=buff.tip_of_the_spear.stack>0" );
  sentcleave->add_action( "flanking_strike,if=(buff.tip_of_the_spear.stack=2|buff.tip_of_the_spear.stack=1)" );
  sentcleave->add_action( "kill_shot,if=buff.deathblow.remains&talent.sic_em" );
  sentcleave->add_action( "kill_command,target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max" );
  sentcleave->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0" );
  sentcleave->add_action( "kill_command,target_if=min:bloodseeker.remains" );
  sentcleave->add_action( "raptor_bite,target_if=min:dot.serpent_sting.remains,if=!talent.contagious_reagents" );
  sentcleave->add_action( "raptor_bite,target_if=max:dot.serpent_sting.remains" );
}
//survival_apl_end

//survival_ptr_apl_start
void survival_ptr( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* plst = p->get_action_priority_list( "plst" );
  action_priority_list_t* plcleave = p->get_action_priority_list( "plcleave" );
  action_priority_list_t* sentst = p->get_action_priority_list( "sentst" );
  action_priority_list_t* sentcleave = p->get_action_priority_list( "sentcleave" );

  precombat->add_action( "summon_pet" );
  precombat->add_action( "use_item,name=imperfect_ascendancy_serum" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  default_->add_action( "auto_attack" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=plst,if=active_enemies<3&talent.vicious_hunt" );
  default_->add_action( "call_action_list,name=plcleave,if=active_enemies>2&talent.vicious_hunt" );
  default_->add_action( "call_action_list,name=sentst,if=active_enemies<3&!talent.vicious_hunt" );
  default_->add_action( "call_action_list,name=sentcleave,if=active_enemies>2&!talent.vicious_hunt" );
  default_->add_action( "arcane_torrent", "simply fires off if there is absolutely nothing else to press." );
  default_->add_action( "bag_of_tricks" );
  default_->add_action( "lights_judgment" );

  cds->add_action( "blood_fury,if=buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault", "COOLDOWNS ACTIONLIST" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "harpoon,if=prev.kill_command" );
  cds->add_action( "ancestral_call,if=buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "fireblood,if=buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "berserking,if=buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault|time_to_die<13" );
  cds->add_action( "muzzle" );
  cds->add_action( "potion,if=target.time_to_die<25|buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "use_item,name=imperfect_ascendancy_serum,use_off_gcd=1,if=gcd.remains>gcd.max-0.1" );
  cds->add_action( "use_item,name=mad_queens_mandate,if=(time_to_die<10|time_to_die>120)&(trinket.skardyns_grace.cooldown.remains|!equipped.skardyns_grace)|time_to_die<10" );
  cds->add_action( "use_items,if=cooldown.coordinated_assault.remains|cooldown.spearhead.remains" );
  cds->add_action( "aspect_of_the_eagle,if=target.distance>=6" );

  plst->add_action( "raptor_bite,target_if=max:dot.serpent_sting.remains,if=buff.howl_of_the_pack.up&pet.main.buff.pack_coordination.up&buff.howl_of_the_pack.remains<gcd", "PL PACK LEADER SINGLE TARGET ACTIONLIST." );
  plst->add_action( "kill_command,target_if=min:bloodseeker.remains,if=(buff.relentless_primal_ferocity.up&buff.tip_of_the_spear.stack<1)" );
  plst->add_action( "butchery,if=buff.scattered_prey.up&buff.scattered_prey.remains<gcd" );
  plst->add_action( "spearhead,if=cooldown.coordinated_assault.remains" );
  plst->add_action( "raptor_bite,target_if=min:dot.serpent_sting.remains,if=!dot.serpent_sting.ticking&target.time_to_die>12&(!talent.contagious_reagents|active_dot.serpent_sting=0)" );
  plst->add_action( "raptor_bite,target_if=max:dot.serpent_sting.remains,if=talent.contagious_reagents&active_dot.serpent_sting<active_enemies&dot.serpent_sting.remains" );
  plst->add_action( "butchery" );
  plst->add_action( "flanking_strike,if=buff.tip_of_the_spear.stack=2|buff.tip_of_the_spear.stack=1" );
  plst->add_action( "kill_shot,if=buff.tip_of_the_spear.stack>0" );
  plst->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0&cooldown.wildfire_bomb.charges_fractional>1.4|cooldown.wildfire_bomb.charges_fractional>1.9|cooldown.coordinated_assault.remains<2*gcd&talent.bombardier" );
  plst->add_action( "explosive_shot" );
  plst->add_action( "coordinated_assault,if=!talent.bombardier|talent.bombardier&cooldown.wildfire_bomb.charges_fractional<1" );
  plst->add_action( "fury_of_the_eagle,if=buff.tip_of_the_spear.stack>0&(!raid_event.adds.exists|raid_event.adds.exists&raid_event.adds.in>40)" );
  plst->add_action( "raptor_bite,if=buff.furious_assault.up" );
  plst->add_action( "kill_command,target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max&(!buff.relentless_primal_ferocity.up|(buff.relentless_primal_ferocity.up&buff.tip_of_the_spear.stack<1|focus<30))" );
  plst->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0&(!raid_event.adds.exists|raid_event.adds.exists&raid_event.adds.in>15)" );
  plst->add_action( "raptor_bite,target_if=min:dot.serpent_sting.remains,if=!talent.contagious_reagents" );
  plst->add_action( "raptor_bite,target_if=max:dot.serpent_sting.remains" );

  plcleave->add_action( "spearhead,if=cooldown.coordinated_assault.remains", "PACK LEADER AOE ACTIONLIST" );
  plcleave->add_action( "kill_command,target_if=min:bloodseeker.remains,if=buff.relentless_primal_ferocity.up&buff.tip_of_the_spear.stack<1" );
  plcleave->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0&cooldown.wildfire_bomb.charges_fractional>1.7|cooldown.wildfire_bomb.charges_fractional>1.9|cooldown.coordinated_assault.remains<2*gcd|talent.butchery&cooldown.butchery.remains<gcd" );
  plcleave->add_action( "flanking_strike,if=buff.tip_of_the_spear.stack=2|buff.tip_of_the_spear.stack=1" );
  plcleave->add_action( "butchery" );
  plcleave->add_action( "explosive_shot" );
  plcleave->add_action( "coordinated_assault,if=!talent.bombardier|talent.bombardier&cooldown.wildfire_bomb.charges_fractional<1" );
  plcleave->add_action( "fury_of_the_eagle,if=buff.tip_of_the_spear.stack>0" );
  plcleave->add_action( "kill_shot,if=buff.deathblow.remains" );
  plcleave->add_action( "kill_command,target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max" );
  plcleave->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0" );
  plcleave->add_action( "kill_shot" );
  plcleave->add_action( "kill_command,target_if=min:bloodseeker.remains" );
  plcleave->add_action( "raptor_bite" );

  sentst->add_action( "wildfire_bomb,if=!cooldown.lunar_storm.remains", "SENT SENTINEL | DEFAULT SINGLE TARGET ACTIONLIST." );
  sentst->add_action( "kill_command,target_if=min:bloodseeker.remains,if=(buff.relentless_primal_ferocity.up&buff.tip_of_the_spear.stack<1)" );
  sentst->add_action( "spearhead,if=cooldown.coordinated_assault.remains" );
  sentst->add_action( "raptor_bite,target_if=min:dot.serpent_sting.remains,if=!dot.serpent_sting.ticking&target.time_to_die>12&(!talent.contagious_reagents|active_dot.serpent_sting=0)" );
  sentst->add_action( "raptor_bite,target_if=max:dot.serpent_sting.remains,if=talent.contagious_reagents&active_dot.serpent_sting<active_enemies&dot.serpent_sting.remains" );
  sentst->add_action( "flanking_strike,if=buff.tip_of_the_spear.stack=2|buff.tip_of_the_spear.stack=1" );
  sentst->add_action( "wildfire_bomb,if=(cooldown.lunar_storm.remains>full_recharge_time-gcd)&(buff.tip_of_the_spear.stack>0&cooldown.wildfire_bomb.charges_fractional>1.7|cooldown.wildfire_bomb.charges_fractional>1.9)|(talent.bombardier&cooldown.coordinated_assault.remains<2*gcd)" );
  sentst->add_action( "butchery" );
  sentst->add_action( "coordinated_assault,if=!talent.bombardier|talent.bombardier&cooldown.wildfire_bomb.charges_fractional<1" );
  sentst->add_action( "explosive_shot" );
  sentst->add_action( "fury_of_the_eagle,if=buff.tip_of_the_spear.stack>0" );
  sentst->add_action( "kill_shot" );
  sentst->add_action( "kill_command,target_if=min:bloodseeker.remains,if=buff.tip_of_the_spear.stack<1&cooldown.flanking_strike.remains<gcd" );
  sentst->add_action( "kill_command,target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max&(!buff.relentless_primal_ferocity.up|(buff.relentless_primal_ferocity.up&(buff.tip_of_the_spear.stack<2|focus<30)))" );
  sentst->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0&cooldown.lunar_storm.remains>full_recharge_time&(!raid_event.adds.exists|raid_event.adds.exists&raid_event.adds.in>15)" );
  sentst->add_action( "raptor_bite,target_if=min:dot.serpent_sting.remains,if=!talent.contagious_reagents" );
  sentst->add_action( "raptor_bite,target_if=max:dot.serpent_sting.remains" );

  sentcleave->add_action( "wildfire_bomb,if=!cooldown.lunar_storm.remains", "SENTINEL | DEFAULT AOE ACTIONLIST" );
  sentcleave->add_action( "kill_command,target_if=min:bloodseeker.remains,if=buff.relentless_primal_ferocity.up&buff.tip_of_the_spear.stack<1" );
  sentcleave->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0&cooldown.wildfire_bomb.charges_fractional>1.7|cooldown.wildfire_bomb.charges_fractional>1.9|(talent.bombardier&cooldown.coordinated_assault.remains<2*gcd)|talent.butchery&cooldown.butchery.remains<gcd" );
  sentcleave->add_action( "butchery" );
  sentcleave->add_action( "explosive_shot" );
  sentcleave->add_action( "coordinated_assault,if=!talent.bombardier|talent.bombardier&cooldown.wildfire_bomb.charges_fractional<1" );
  sentcleave->add_action( "fury_of_the_eagle,if=buff.tip_of_the_spear.stack>0" );
  sentcleave->add_action( "flanking_strike,if=(buff.tip_of_the_spear.stack=2|buff.tip_of_the_spear.stack=1)" );
  sentcleave->add_action( "kill_shot,if=buff.deathblow.remains&talent.sic_em" );
  sentcleave->add_action( "kill_command,target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max" );
  sentcleave->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0" );
  sentcleave->add_action( "kill_command,target_if=min:bloodseeker.remains" );
  sentcleave->add_action( "raptor_bite,target_if=min:dot.serpent_sting.remains,if=!talent.contagious_reagents" );
  sentcleave->add_action( "raptor_bite,target_if=max:dot.serpent_sting.remains" );
}
//survival_ptr_apl_end

} // namespace hunter_apl
