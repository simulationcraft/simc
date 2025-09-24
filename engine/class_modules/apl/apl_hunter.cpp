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
  action_priority_list_t* drcleave = p->get_action_priority_list( "drcleave" );
  action_priority_list_t* drst = p->get_action_priority_list( "drst" );
  action_priority_list_t* st = p->get_action_priority_list( "st" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );

  precombat->add_action( "summon_pet" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=stronger_trinket_slot,op=setif,value=1,value_else=2,condition=!trinket.2.has_cooldown|trinket.1.has_use_buff&(!trinket.2.has_use_buff|trinket.2.cooldown.duration<trinket.1.cooldown.duration|trinket.2.cast_time<trinket.1.cast_time|trinket.2.cast_time=trinket.1.cast_time&trinket.2.cooldown.duration=trinket.1.cooldown.duration)|!trinket.1.has_use_buff&(!trinket.2.has_use_buff&(trinket.2.cooldown.duration<trinket.1.cooldown.duration|trinket.2.cast_time<trinket.1.cast_time|trinket.2.cast_time=trinket.1.cast_time&trinket.2.cooldown.duration=trinket.1.cooldown.duration))" );

  default_->add_action( "auto_shot" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "call_action_list,name=drst,if=talent.black_arrow&(active_enemies<2|!talent.beast_cleave&active_enemies<3)" );
  default_->add_action( "call_action_list,name=drcleave,if=talent.black_arrow&(active_enemies>2|talent.beast_cleave&active_enemies>1)" );
  default_->add_action( "call_action_list,name=st,if=!talent.black_arrow&(active_enemies<2|!talent.beast_cleave&active_enemies<3)" );
  default_->add_action( "call_action_list,name=cleave,if=!talent.black_arrow&(active_enemies>2|talent.beast_cleave&active_enemies>1)" );

  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&(buff.bestial_wrath.up|cooldown.bestial_wrath.remains<30)|fight_remains<16" );
  cds->add_action( "berserking,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<13" );
  cds->add_action( "blood_fury,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<16" );
  cds->add_action( "ancestral_call,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<16" );
  cds->add_action( "fireblood,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<9" );
  cds->add_action( "potion,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<31" );

  cleave->add_action( "bestial_wrath,if=buff.howl_of_the_pack_leader_cooldown.remains-buff.lead_from_the_front.duration<buff.lead_from_the_front.duration%gcd*0.5|!set_bonus.tww3_4pc|talent.multishot" );
  cleave->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=full_recharge_time<gcd|charges_fractional>=cooldown.kill_command.charges_fractional|talent.call_of_the_wild&cooldown.call_of_the_wild.ready|howl_summon.ready&full_recharge_time<8" );
  cleave->add_action( "bloodshed" );
  cleave->add_action( "multishot,if=pet.main.buff.beast_cleave.down&(!talent.bloody_frenzy|cooldown.call_of_the_wild.remains)" );
  cleave->add_action( "call_of_the_wild" );
  cleave->add_action( "explosive_shot,if=talent.thundering_hooves" );
  cleave->add_action( "kill_command" );
  cleave->add_action( "cobra_shot,if=focus.time_to_max<gcd*2|buff.hogstrider.stack>3|!talent.multishot" );

  drcleave->add_action( "kill_shot" );
  drcleave->add_action( "bestial_wrath,if=cooldown.call_of_the_wild.remains>20|!talent.call_of_the_wild" );
  drcleave->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=full_recharge_time<gcd|buff.thrill_of_the_hunt.remains<1.5*gcd" );
  drcleave->add_action( "bloodshed" );
  drcleave->add_action( "multishot,if=pet.main.buff.beast_cleave.down&(!talent.bloody_frenzy|cooldown.call_of_the_wild.remains)" );
  drcleave->add_action( "call_of_the_wild" );
  drcleave->add_action( "explosive_shot,if=talent.thundering_hooves" );
  drcleave->add_action( "kill_command,if=buff.withering_fire.tick_time_remains>gcd&cooldown.black_arrow.remains>0.5|buff.withering_fire.down" );
  drcleave->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=buff.withering_fire.tick_time_remains>0.5&cooldown.black_arrow.remains>0.5|buff.withering_fire.down" );
  drcleave->add_action( "cobra_shot,if=buff.withering_fire.down&focus.time_to_max<gcd*2" );
  drcleave->add_action( "explosive_shot" );

  drst->add_action( "kill_shot" );
  drst->add_action( "bestial_wrath,if=cooldown.call_of_the_wild.remains>30|!talent.call_of_the_wild|time_to_die.remains<cooldown.call_of_the_wild.remains" );
  drst->add_action( "bloodshed" );
  drst->add_action( "call_of_the_wild" );
  drst->add_action( "kill_command,if=buff.withering_fire.tick_time_remains>gcd&cooldown.black_arrow.remains>0.5|buff.withering_fire.down" );
  drst->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=buff.withering_fire.tick_time_remains>0.5&cooldown.black_arrow.remains>0.5|buff.withering_fire.down" );
  drst->add_action( "cobra_shot,if=buff.withering_fire.down" );

  st->add_action( "bestial_wrath,if=buff.howl_of_the_pack_leader_cooldown.remains-buff.lead_from_the_front.duration<buff.lead_from_the_front.duration%gcd*0.5|!set_bonus.tww3_4pc" );
  st->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=full_recharge_time<gcd" );
  st->add_action( "call_of_the_wild" );
  st->add_action( "bloodshed" );
  st->add_action( "kill_command,if=charges_fractional>=cooldown.barbed_shot.charges_fractional&!(buff.lead_from_the_front.remains>gcd&buff.lead_from_the_front.remains<gcd*2&!howl_summon.ready&full_recharge_time>gcd)" );
  st->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains" );
  st->add_action( "cobra_shot" );

  trinkets->add_action( "variable,name=buff_sync_ready,value=talent.call_of_the_wild&(prev_gcd.1.call_of_the_wild)|talent.bloodshed&(prev_gcd.1.bloodshed)|(!talent.call_of_the_wild&!talent.bloodshed)&(buff.bestial_wrath.up|cooldown.bestial_wrath.remains_guess<5)" );
  trinkets->add_action( "variable,name=buff_sync_remains,op=setif,value=cooldown.bestial_wrath.remains_guess,value_else=cooldown.call_of_the_wild.remains|cooldown.bloodshed.remains,condition=!talent.call_of_the_wild&!talent.bloodshed" );
  trinkets->add_action( "variable,name=buff_sync_active,value=talent.call_of_the_wild&buff.call_of_the_wild.up|talent.bloodshed&prev_gcd.1.bloodshed|(!talent.call_of_the_wild&!talent.bloodshed)&buff.bestial_wrath.up" );
  trinkets->add_action( "variable,name=damage_sync_active,value=1" );
  trinkets->add_action( "variable,name=damage_sync_remains,value=0" );
  trinkets->add_action( "use_items,slots=trinket1:trinket2,if=this_trinket.has_use_buff&(variable.buff_sync_ready&(variable.stronger_trinket_slot=this_trinket_slot|other_trinket.cooldown.remains)|!variable.buff_sync_ready&(variable.stronger_trinket_slot=this_trinket_slot&(variable.buff_sync_remains>this_trinket.cooldown.duration%3&fight_remains>this_trinket.cooldown.duration+20|other_trinket.has_use_buff&other_trinket.cooldown.remains>variable.buff_sync_remains-15&other_trinket.cooldown.remains-5<variable.buff_sync_remains&variable.buff_sync_remains+45>fight_remains)|variable.stronger_trinket_slot!=this_trinket_slot&(other_trinket.cooldown.remains&(other_trinket.cooldown.remains-5<variable.buff_sync_remains&variable.buff_sync_remains>=20|other_trinket.cooldown.remains-5>=variable.buff_sync_remains&(variable.buff_sync_remains>this_trinket.cooldown.duration%3|this_trinket.cooldown.duration<fight_remains&(variable.buff_sync_remains+this_trinket.cooldown.duration>fight_remains)))|other_trinket.cooldown.ready&variable.buff_sync_remains>20&variable.buff_sync_remains<other_trinket.cooldown.duration%3)))|!this_trinket.has_use_buff&(this_trinket.cast_time=0|!variable.buff_sync_active)&(!this_trinket.is.junkmaestros_mega_magnet|buff.junkmaestros_mega_magnet.stack>10)&(!other_trinket.has_cooldown&(variable.damage_sync_active|this_trinket.is.junkmaestros_mega_magnet&buff.junkmaestros_mega_magnet.stack>25|!this_trinket.is.junkmaestros_mega_magnet&variable.damage_sync_remains>this_trinket.cooldown.duration%3)|other_trinket.has_cooldown&(!other_trinket.has_use_buff&(variable.stronger_trinket_slot=this_trinket_slot|other_trinket.cooldown.remains)&(variable.damage_sync_active|this_trinket.is.junkmaestros_mega_magnet&buff.junkmaestros_mega_magnet.stack>25|variable.damage_sync_remains>this_trinket.cooldown.duration%3&!this_trinket.is.junkmaestros_mega_magnet|other_trinket.cooldown.remains-5<variable.damage_sync_remains&variable.damage_sync_remains>=20)|other_trinket.has_use_buff&(variable.damage_sync_active|this_trinket.is.junkmaestros_mega_magnet&buff.junkmaestros_mega_magnet.stack>25|!this_trinket.is.junkmaestros_mega_magnet&variable.damage_sync_remains>this_trinket.cooldown.duration%3)&(other_trinket.cooldown.remains>=20|other_trinket.cooldown.remains-5>variable.buff_sync_remains)))|fight_remains<25&(variable.stronger_trinket_slot=this_trinket_slot|other_trinket.cooldown.remains)" );
}
//beast_mastery_apl_end

//beast_mastery_ptr_apl_start
void beast_mastery_ptr( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* cleave = p->get_action_priority_list( "cleave" );
  action_priority_list_t* drcleave = p->get_action_priority_list( "drcleave" );
  action_priority_list_t* drst = p->get_action_priority_list( "drst" );
  action_priority_list_t* st = p->get_action_priority_list( "st" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );

  precombat->add_action( "summon_pet" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=stronger_trinket_slot,op=setif,value=1,value_else=2,condition=!trinket.2.has_cooldown|trinket.1.has_use_buff&(!trinket.2.has_use_buff|trinket.2.cooldown.duration<trinket.1.cooldown.duration|trinket.2.cast_time<trinket.1.cast_time|trinket.2.cast_time=trinket.1.cast_time&trinket.2.cooldown.duration=trinket.1.cooldown.duration)|!trinket.1.has_use_buff&(!trinket.2.has_use_buff&(trinket.2.cooldown.duration<trinket.1.cooldown.duration|trinket.2.cast_time<trinket.1.cast_time|trinket.2.cast_time=trinket.1.cast_time&trinket.2.cooldown.duration=trinket.1.cooldown.duration))" );

  default_->add_action( "auto_shot" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "call_action_list,name=drst,if=talent.black_arrow&(active_enemies<2|!talent.beast_cleave&active_enemies<3)" );
  default_->add_action( "call_action_list,name=drcleave,if=talent.black_arrow&(active_enemies>2|talent.beast_cleave&active_enemies>1)" );
  default_->add_action( "call_action_list,name=st,if=!talent.black_arrow&(active_enemies<2|!talent.beast_cleave&active_enemies<3)" );
  default_->add_action( "call_action_list,name=cleave,if=!talent.black_arrow&(active_enemies>2|talent.beast_cleave&active_enemies>1)" );

  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&(buff.bestial_wrath.up|cooldown.bestial_wrath.remains<30)|fight_remains<16" );
  cds->add_action( "berserking,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<13" );
  cds->add_action( "blood_fury,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<16" );
  cds->add_action( "ancestral_call,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<16" );
  cds->add_action( "fireblood,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<9" );
  cds->add_action( "potion,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<31" );

  cleave->add_action( "bestial_wrath,if=buff.howl_of_the_pack_leader_cooldown.remains-buff.lead_from_the_front.duration<buff.lead_from_the_front.duration%gcd*0.5|!set_bonus.tww3_4pc|talent.multishot" );
  cleave->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=full_recharge_time<gcd|charges_fractional>=cooldown.kill_command.charges_fractional|talent.call_of_the_wild&cooldown.call_of_the_wild.ready|howl_summon.ready&full_recharge_time<8" );
  cleave->add_action( "bloodshed" );
  cleave->add_action( "multishot,if=pet.main.buff.beast_cleave.down&(!talent.bloody_frenzy|cooldown.call_of_the_wild.remains)" );
  cleave->add_action( "call_of_the_wild" );
  cleave->add_action( "explosive_shot,if=talent.thundering_hooves" );
  cleave->add_action( "kill_command" );
  cleave->add_action( "cobra_shot,if=focus.time_to_max<gcd*2|buff.hogstrider.stack>3|!talent.multishot" );

  drcleave->add_action( "bestial_wrath,if=buff.call_of_the_wild.remains" );
  drcleave->add_action( "kill_shot" );
  drcleave->add_action( "bestial_wrath,if=cooldown.call_of_the_wild.remains>20|!talent.call_of_the_wild" );
  drcleave->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=full_recharge_time<gcd" );
  drcleave->add_action( "bloodshed" );
  drcleave->add_action( "multishot,if=pet.main.buff.beast_cleave.down&(!talent.bloody_frenzy|cooldown.call_of_the_wild.remains)" );
  drcleave->add_action( "call_of_the_wild" );
  drcleave->add_action( "explosive_shot,if=talent.thundering_hooves" );
  drcleave->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=charges_fractional>=cooldown.kill_command.charges_fractional" );
  drcleave->add_action( "kill_command" );
  drcleave->add_action( "cobra_shot,if=focus.time_to_max<gcd*2" );
  drcleave->add_action( "explosive_shot" );

  drst->add_action( "kill_shot" );
  drst->add_action( "bestial_wrath,if=cooldown.call_of_the_wild.remains>30|!talent.call_of_the_wild|time_to_die.remains<cooldown.call_of_the_wild.remains" );
  drst->add_action( "barbed_shot,if=buff.thrill_of_the_hunt.remains<1.5*gcd" );
  drst->add_action( "bloodshed" );
  drst->add_action( "call_of_the_wild" );
  drst->add_action( "kill_command,if=buff.withering_fire.tick_time_remains>gcd&cooldown.black_arrow.remains>0.5" );
  drst->add_action( "kill_command,if=buff.withering_fire.down" );
  drst->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=buff.withering_fire.tick_time_remains>0.5&cooldown.black_arrow.remains>0.5" );
  drst->add_action( "cobra_shot,if=buff.withering_fire.tick_time_remains>0.5&cooldown.black_arrow.remains>0.5" );

  st->add_action( "bestial_wrath,if=buff.howl_of_the_pack_leader_cooldown.remains-buff.lead_from_the_front.duration<buff.lead_from_the_front.duration%gcd*0.5|!set_bonus.tww3_4pc" );
  st->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=full_recharge_time<gcd" );
  st->add_action( "call_of_the_wild" );
  st->add_action( "bloodshed" );
  st->add_action( "kill_command,if=charges_fractional>=cooldown.barbed_shot.charges_fractional&!(buff.lead_from_the_front.remains>gcd&buff.lead_from_the_front.remains<gcd*2&!howl_summon.ready&full_recharge_time>gcd)" );
  st->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains" );
  st->add_action( "cobra_shot" );

  trinkets->add_action( "variable,name=buff_sync_ready,value=talent.call_of_the_wild&(prev_gcd.1.call_of_the_wild)|talent.bloodshed&(prev_gcd.1.bloodshed)|(!talent.call_of_the_wild&!talent.bloodshed)&(buff.bestial_wrath.up|cooldown.bestial_wrath.remains_guess<5)" );
  trinkets->add_action( "variable,name=buff_sync_remains,op=setif,value=cooldown.bestial_wrath.remains_guess,value_else=cooldown.call_of_the_wild.remains|cooldown.bloodshed.remains,condition=!talent.call_of_the_wild&!talent.bloodshed" );
  trinkets->add_action( "variable,name=buff_sync_active,value=talent.call_of_the_wild&buff.call_of_the_wild.up|talent.bloodshed&prev_gcd.1.bloodshed|(!talent.call_of_the_wild&!talent.bloodshed)&buff.bestial_wrath.up" );
  trinkets->add_action( "variable,name=damage_sync_active,value=1" );
  trinkets->add_action( "variable,name=damage_sync_remains,value=0" );
  trinkets->add_action( "use_items,slots=trinket1:trinket2,if=this_trinket.has_use_buff&(variable.buff_sync_ready&(variable.stronger_trinket_slot=this_trinket_slot|other_trinket.cooldown.remains)|!variable.buff_sync_ready&(variable.stronger_trinket_slot=this_trinket_slot&(variable.buff_sync_remains>this_trinket.cooldown.duration%3&fight_remains>this_trinket.cooldown.duration+20|other_trinket.has_use_buff&other_trinket.cooldown.remains>variable.buff_sync_remains-15&other_trinket.cooldown.remains-5<variable.buff_sync_remains&variable.buff_sync_remains+45>fight_remains)|variable.stronger_trinket_slot!=this_trinket_slot&(other_trinket.cooldown.remains&(other_trinket.cooldown.remains-5<variable.buff_sync_remains&variable.buff_sync_remains>=20|other_trinket.cooldown.remains-5>=variable.buff_sync_remains&(variable.buff_sync_remains>this_trinket.cooldown.duration%3|this_trinket.cooldown.duration<fight_remains&(variable.buff_sync_remains+this_trinket.cooldown.duration>fight_remains)))|other_trinket.cooldown.ready&variable.buff_sync_remains>20&variable.buff_sync_remains<other_trinket.cooldown.duration%3)))|!this_trinket.has_use_buff&(this_trinket.cast_time=0|!variable.buff_sync_active)&(!this_trinket.is.junkmaestros_mega_magnet|buff.junkmaestros_mega_magnet.stack>10)&(!other_trinket.has_cooldown&(variable.damage_sync_active|this_trinket.is.junkmaestros_mega_magnet&buff.junkmaestros_mega_magnet.stack>25|!this_trinket.is.junkmaestros_mega_magnet&variable.damage_sync_remains>this_trinket.cooldown.duration%3)|other_trinket.has_cooldown&(!other_trinket.has_use_buff&(variable.stronger_trinket_slot=this_trinket_slot|other_trinket.cooldown.remains)&(variable.damage_sync_active|this_trinket.is.junkmaestros_mega_magnet&buff.junkmaestros_mega_magnet.stack>25|variable.damage_sync_remains>this_trinket.cooldown.duration%3&!this_trinket.is.junkmaestros_mega_magnet|other_trinket.cooldown.remains-5<variable.damage_sync_remains&variable.damage_sync_remains>=20)|other_trinket.has_use_buff&(variable.damage_sync_active|this_trinket.is.junkmaestros_mega_magnet&buff.junkmaestros_mega_magnet.stack>25|!this_trinket.is.junkmaestros_mega_magnet&variable.damage_sync_remains>this_trinket.cooldown.duration%3)&(other_trinket.cooldown.remains>=20|other_trinket.cooldown.remains-5>variable.buff_sync_remains)))|fight_remains<25&(variable.stronger_trinket_slot=this_trinket_slot|other_trinket.cooldown.remains)" );
}
//beast_mastery_ptr_apl_end

//marksmanship_apl_start
void marksmanship( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );
  action_priority_list_t* drst = p->get_action_priority_list( "drst" );
  action_priority_list_t* sentst = p->get_action_priority_list( "sentst" );
  action_priority_list_t* drcleave = p->get_action_priority_list( "drcleave" );
  action_priority_list_t* sentcleave = p->get_action_priority_list( "sentcleave" );
  action_priority_list_t* drtrickshots = p->get_action_priority_list( "drtrickshots" );
  action_priority_list_t* senttrickshots = p->get_action_priority_list( "senttrickshots" );

  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "summon_pet,if=talent.unbreakable_bond" );
  precombat->add_action( "aimed_shot,if=active_enemies<3|talent.black_arrow&talent.headshot" );
  precombat->add_action( "steady_shot" );

  default_->add_action( "variable,name=trueshot_ready,value=!talent.bullseye|fight_remains>cooldown.trueshot.duration+10|buff.bullseye.stack=buff.bullseye.max_stack|fight_remains<25", "Hold the final Trueshot for Bullseye stacks if necessary." );
  default_->add_action( "variable,name=trueshot_ready,op=setif,condition=fight_style.dungeonroute,value_else=variable.trueshot_ready,value=raid_event.pull.remains>30|raid_event.pull.in>60", "For DungeonRoute, hold Trueshot at the end of pulls." );
  default_->add_action( "variable,name=buffer_deathblow,value=hero_tree.dark_ranger&action.aimed_shot.in_flight&!action.black_arrow.ready" );
  default_->add_action( "auto_shot" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "call_action_list,name=drtrickshots,if=active_enemies>2&talent.trick_shots&hero_tree.dark_ranger" );
  default_->add_action( "call_action_list,name=senttrickshots,if=active_enemies>2&talent.trick_shots&hero_tree.sentinel" );
  default_->add_action( "call_action_list,name=drcleave,if=active_enemies>1&hero_tree.dark_ranger" );
  default_->add_action( "call_action_list,name=sentcleave,if=active_enemies>1&hero_tree.sentinel" );
  default_->add_action( "call_action_list,name=drst,if=hero_tree.dark_ranger" );
  default_->add_action( "call_action_list,name=sentst,if=hero_tree.sentinel" );

  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.trueshot.remains>12|fight_remains<13" );
  cds->add_action( "berserking,if=buff.trueshot.up|fight_remains<13" );
  cds->add_action( "blood_fury,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<16" );
  cds->add_action( "ancestral_call,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<16" );
  cds->add_action( "fireblood,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<9" );
  cds->add_action( "lights_judgment,if=buff.trueshot.down" );
  cds->add_action( "potion,if=buff.trueshot.up&(buff.bloodlust.up|target.health.pct<20)|fight_remains<31" );

  trinkets->add_action( "use_items,check_existing=0,slots=trinket1:trinket2,if=!equipped.unyielding_netherprism&this_trinket.has_use_buff&this_trinket.cooldown.duration%%cooldown.trueshot.duration=0&buff.trueshot.remains>14", "A buff trinket that lines up cleanly with Trueshot; use with Trueshot." );
  trinkets->add_action( "use_items,check_existing=0,slots=trinket1:trinket2,if=!equipped.unyielding_netherprism&this_trinket.has_use_buff&other_trinket.cooldown.duration%%cooldown.trueshot.duration=0&(buff.trueshot.remains>14&other_trinket.cooldown.remains|cooldown.trueshot.remains>20&other_trinket.cooldown.remains<=cooldown.trueshot.remains)", "A buff trinket paired with a trinket that matches the above line; use with Trueshot if the other trinket is not ready or use without Trueshot if the other trinket will come up for the next Trueshot." );
  trinkets->add_action( "use_items,check_existing=0,slots=trinket1:trinket2,if=this_trinket.is.unyielding_netherprism&(buff.trueshot.remains>14&(buff.latent_energy.stack>(19-cooldown.trueshot.duration%10)|fight_remains<(cooldown.trueshot.duration+20))|fight_remains<22&(buff.latent_energy.stack>8|!other_trinket.has_use_buff|other_trinket.cooldown.remains))", "Netherprism higher prioroty; use with Trueshot if waiting for the next Trueshot will waste stacks or if it's the final Trueshot of the fight. Also use in the last ~20 seconds of the fight if there is no other buff trinket ready or if there are over 8 stacks." );
  trinkets->add_action( "use_items,check_existing=0,slots=trinket1:trinket2,if=!this_trinket.is.unyielding_netherprism&this_trinket.has_use_buff&(other_trinket.is.unyielding_netherprism&fight_remains<cooldown.trueshot.remains+cooldown.trueshot.duration+10&cooldown.trueshot.remains>20|buff.trueshot.remains>14|buff.trueshot.up&fight_remains<cooldown.trueshot.remains+15|fight_remains<21)", "A buff trinket that is not Netherprism; if the other trinket is Netherprism, use without Trueshot if there will only be one more Trueshot in the fight. Otherwise use with Trueshot or in the last ~20 seconds of the fight." );
  trinkets->add_action( "use_items,check_existing=0,slots=trinket1:trinket2,if=this_trinket.is.unyielding_netherprism&buff.trueshot.remains>14&buff.latent_energy.stack>3&(buff.latent_energy.stack+floor((fight_remains-20)%cooldown.trueshot.duration)*(cooldown.trueshot.duration%10))>17", "Netherprism lower priority; use with Trueshot if using it now will still allow stacking it back up for the final Trueshot of the fight." );
  trinkets->add_action( "use_items,check_existing=0,slots=trinket1:trinket2,if=this_trinket.has_use_damage&cooldown.trueshot.remains>20", "A damage trinket; use when Trueshot has at least 20 seconds remaining on its cooldown." );

  drst->add_action( "volley,if=buff.double_tap.down&(!raid_event.adds.exists|raid_event.adds.in>cooldown)&(!talent.shrapnel_shot|!talent.salvo|buff.lock_and_load.down)", "1 target" );
  drst->add_action( "explosive_shot,if=talent.precision_detonation&buff.lock_and_load.down&cooldown.aimed_shot.charges_fractional<=1.1&buff.trueshot.down" );
  drst->add_action( "steady_shot,if=variable.buffer_deathblow&buff.trueshot.down&cooldown.trueshot.remains", "Queue Steady Shot after Aimed Shot if a Deathblow hasn't already been up long enough to be reacted to." );
  drst->add_action( "trueshot,if=variable.trueshot_ready&buff.double_tap.down&!action.black_arrow.ready&(!talent.bulletstorm|buff.bulletstorm.up)" );
  drst->add_action( "black_arrow,if=talent.headshot&buff.precise_shots.up&(debuff.spotters_mark.down|buff.moving_target.down)|!talent.headshot" );
  drst->add_action( "aimed_shot,if=(buff.trueshot.up|action.black_arrow.ready)&buff.precise_shots.down|buff.lock_and_load.up&buff.moving_target.up" );
  drst->add_action( "rapid_fire,if=!action.black_arrow.ready" );
  drst->add_action( "trueshot,if=variable.trueshot_ready&buff.double_tap.down" );
  drst->add_action( "aimed_shot,if=buff.precise_shots.down|debuff.spotters_mark.up&buff.moving_target.up" );
  drst->add_action( "arcane_shot,if=buff.precise_shots.up&(debuff.spotters_mark.down|buff.moving_target.down)&(cooldown.black_arrow.remains>action.steady_shot.execute_time|target.health.pct<80&target.health.pct>20)" );
  drst->add_action( "explosive_shot,if=talent.shrapnel_shot&buff.lock_and_load.down" );
  drst->add_action( "steady_shot" );

  sentst->add_action( "volley,if=buff.double_tap.down&(!raid_event.adds.exists|raid_event.adds.in>cooldown)&(!talent.shrapnel_shot|!talent.salvo|buff.lock_and_load.down)" );
  sentst->add_action( "explosive_shot,if=talent.shrapnel_shot&buff.lock_and_load.down&cooldown.aimed_shot.charges_fractional<=1.1&buff.trueshot.down" );
  sentst->add_action( "rapid_fire,if=talent.lunar_storm&buff.lunar_storm_cooldown.down" );
  sentst->add_action( "trueshot,if=variable.trueshot_ready&buff.double_tap.down&(!talent.bulletstorm|buff.bulletstorm.up)" );
  sentst->add_action( "kill_shot,if=talent.headshot&buff.precise_shots.up&(debuff.spotters_mark.down|buff.moving_target.down)|!talent.headshot&buff.razor_fragments.up" );
  sentst->add_action( "aimed_shot,if=buff.trueshot.up&buff.precise_shots.down|buff.lock_and_load.up&buff.moving_target.up" );
  sentst->add_action( "rapid_fire,if=!talent.no_scope|buff.precise_shots.down" );
  sentst->add_action( "arcane_shot,if=buff.precise_shots.up&(debuff.spotters_mark.down|buff.moving_target.down)" );
  sentst->add_action( "rapid_fire" );
  sentst->add_action( "aimed_shot,if=buff.precise_shots.down|debuff.spotters_mark.up&buff.moving_target.up" );
  sentst->add_action( "explosive_shot,if=talent.precision_detonation|buff.trueshot.down" );
  sentst->add_action( "steady_shot" );

  drcleave->add_action( "wait,sec=0.05,if=talent.aspect_of_the_hydra&talent.shrapnel_shot&time=0&action.aimed_shot.in_flight", "2 targets (2+ without Trick Shots)  With Shrapnel Shot don't queue Explosive Shot after the precast since the Aspect of the Hydra secondary Aimed Shot will consume the Lock and Load." );
  drcleave->add_action( "explosive_shot,if=buff.trueshot.down&talent.precision_detonation&(!talent.shrapnel_shot|buff.lock_and_load.down&cooldown.aimed_shot.charges_fractional<=1.1)" );
  drcleave->add_action( "black_arrow,if=buff.precise_shots.up|!talent.headshot" );
  drcleave->add_action( "rapid_fire,if=talent.bulletstorm&buff.bulletstorm.down&(talent.aspect_of_the_hydra|!talent.volley|cooldown.volley.remains)" );
  drcleave->add_action( "volley,if=buff.double_tap.down&(!talent.double_tap|buff.precise_shots.down)&(!talent.shrapnel_shot|!talent.salvo|buff.lock_and_load.down)" );
  drcleave->add_action( "trueshot,if=variable.trueshot_ready&buff.double_tap.down&(!talent.volley|cooldown.volley.remains)" );
  drcleave->add_action( "steady_shot,if=variable.buffer_deathblow&buff.trueshot.down" );
  drcleave->add_action( "aimed_shot,if=buff.trueshot.up&buff.precise_shots.down|buff.lock_and_load.up&buff.moving_target.up" );
  drcleave->add_action( "rapid_fire,if=buff.double_tap.down" );
  drcleave->add_action( "arcane_shot,if=buff.precise_shots.up&(debuff.spotters_mark.down|buff.moving_target.down)" );
  drcleave->add_action( "aimed_shot,if=buff.precise_shots.down|debuff.spotters_mark.up&buff.moving_target.up" );
  drcleave->add_action( "explosive_shot,if=!talent.shrapnel_shot|buff.lock_and_load.down" );
  drcleave->add_action( "steady_shot" );

  sentcleave->add_action( "explosive_shot,if=talent.precision_detonation&action.aimed_shot.in_flight&(buff.trueshot.down|!talent.windrunner_quiver)" );
  sentcleave->add_action( "volley,if=(talent.double_tap&buff.double_tap.down|!talent.aspect_of_the_hydra)&(buff.precise_shots.down|buff.moving_target.up)" );
  sentcleave->add_action( "rapid_fire,if=talent.bulletstorm&buff.bulletstorm.down&(!talent.double_tap|buff.double_tap.up|!talent.aspect_of_the_hydra&buff.trick_shots.remains>execute_time)&(buff.precise_shots.down|buff.moving_target.up|!talent.volley)" );
  sentcleave->add_action( "volley,if=!talent.double_tap&(buff.precise_shots.down|buff.moving_target.up)" );
  sentcleave->add_action( "trueshot,if=variable.trueshot_ready&(buff.double_tap.down|!talent.volley)&(buff.lunar_storm_ready.down|!talent.double_tap|!talent.volley)&(buff.precise_shots.down|buff.moving_target.up|!talent.volley)" );
  sentcleave->add_action( "rapid_fire,if=talent.lunar_storm&buff.lunar_storm_cooldown.down&(buff.precise_shots.down|buff.moving_target.up|cooldown.volley.remains&cooldown.trueshot.remains|!talent.volley)" );
  sentcleave->add_action( "kill_shot,target_if=max:debuff.spotters_mark.down|action.aimed_shot.in_flight_to_target|max_prio_damage,if=talent.headshot&buff.precise_shots.up&(debuff.spotters_mark.down|buff.moving_target.down)|!talent.headshot&buff.razor_fragments.up" );
  sentcleave->add_action( "multishot,target_if=max:debuff.spotters_mark.down|action.aimed_shot.in_flight_to_target|max_prio_damage,if=buff.precise_shots.up&(debuff.spotters_mark.down|buff.moving_target.down)&!talent.aspect_of_the_hydra" );
  sentcleave->add_action( "arcane_shot,target_if=max:debuff.spotters_mark.down|action.aimed_shot.in_flight_to_target|max_prio_damage,if=buff.precise_shots.up&(debuff.spotters_mark.down|buff.moving_target.down)" );
  sentcleave->add_action( "aimed_shot,target_if=max:debuff.spotters_mark.up,if=(buff.precise_shots.down|debuff.spotters_mark.up&buff.moving_target.up)&full_recharge_time<action.rapid_fire.execute_time+cast_time&(!talent.bulletstorm|buff.bulletstorm.up)&talent.windrunner_quiver" );
  sentcleave->add_action( "rapid_fire,if=!talent.bulletstorm|buff.bulletstorm.stack<=10|talent.aspect_of_the_hydra" );
  sentcleave->add_action( "aimed_shot,target_if=max:debuff.spotters_mark.up|max_prio_damage,if=buff.precise_shots.down|debuff.spotters_mark.up&buff.moving_target.up" );
  sentcleave->add_action( "rapid_fire" );
  sentcleave->add_action( "explosive_shot,if=talent.precision_detonation|buff.trueshot.down" );
  sentcleave->add_action( "steady_shot" );

  drtrickshots->add_action( "volley,if=buff.double_tap.down&(!talent.shrapnel_shot|!talent.salvo|buff.lock_and_load.down&cooldown.aimed_shot.charges_fractional<=1.1)", "3+ targets (with Trick Shots)" );
  drtrickshots->add_action( "explosive_shot,if=talent.precision_detonation&buff.trueshot.down&(!talent.shrapnel_shot|buff.lock_and_load.down&(cooldown.aimed_shot.charges_fractional<=1.1|talent.focused_aim))" );
  drtrickshots->add_action( "black_arrow,if=buff.trick_shots.down|!talent.headshot|buff.precise_shots.up" );
  drtrickshots->add_action( "rapid_fire,if=buff.trick_shots.remains>execute_time&talent.bulletstorm&buff.bulletstorm.down" );
  drtrickshots->add_action( "trueshot,if=variable.trueshot_ready&buff.double_tap.down" );
  drtrickshots->add_action( "steady_shot,if=variable.buffer_deathblow&buff.trueshot.down" );
  drtrickshots->add_action( "multishot,target_if=max:debuff.spotters_mark.down|action.aimed_shot.in_flight_to_target,if=buff.trick_shots.down|buff.precise_shots.up&(buff.moving_target.down|debuff.spotters_mark.down)" );
  drtrickshots->add_action( "aimed_shot,if=buff.trick_shots.remains>cast_time&(buff.trueshot.up&buff.precise_shots.down|buff.lock_and_load.up&buff.moving_target.up)" );
  drtrickshots->add_action( "rapid_fire,if=buff.trick_shots.remains>execute_time&(talent.no_scope|talent.bulletstorm&buff.bulletstorm.down)" );
  drtrickshots->add_action( "aimed_shot,if=buff.trick_shots.remains>cast_time&(buff.precise_shots.down|debuff.spotters_mark.up&buff.moving_target.up)" );
  drtrickshots->add_action( "explosive_shot,if=talent.shrapnel_shot&buff.lock_and_load.down&cooldown.aimed_shot.charges_fractional<=1.1" );
  drtrickshots->add_action( "steady_shot" );

  senttrickshots->add_action( "explosive_shot,if=talent.shrapnel_shot&buff.trueshot.down&buff.lock_and_load.down&cooldown.aimed_shot.charges_fractional<=1.1" );
  senttrickshots->add_action( "volley,if=buff.double_tap.down&(!talent.shrapnel_shot|!talent.salvo|buff.lock_and_load.down)" );
  senttrickshots->add_action( "rapid_fire,if=buff.trick_shots.remains>execute_time&(talent.bulletstorm&buff.bulletstorm.down|buff.lunar_storm_cooldown.down)" );
  senttrickshots->add_action( "kill_shot,if=talent.headshot&buff.trick_shots.up&buff.razor_fragments.up&buff.precise_shots.up" );
  senttrickshots->add_action( "multishot,target_if=max:debuff.spotters_mark.down|action.aimed_shot.in_flight_to_target,if=buff.trick_shots.down|buff.precise_shots.up&(buff.moving_target.down|debuff.spotters_mark.down)" );
  senttrickshots->add_action( "trueshot,if=variable.trueshot_ready&buff.double_tap.down" );
  senttrickshots->add_action( "aimed_shot,if=buff.trick_shots.remains>cast_time&(buff.trueshot.up&buff.precise_shots.down|buff.lock_and_load.up&buff.moving_target.up)" );
  senttrickshots->add_action( "rapid_fire,if=buff.trick_shots.remains>execute_time" );
  senttrickshots->add_action( "aimed_shot,if=buff.trick_shots.remains>cast_time&(buff.precise_shots.down|debuff.spotters_mark.up&buff.moving_target.up)" );
  senttrickshots->add_action( "explosive_shot" );
  senttrickshots->add_action( "steady_shot,if=focus+cast_regen<focus.max" );
  senttrickshots->add_action( "multishot" );
}
//marksmanship_apl_end

//marksmanship_ptr_apl_start
void marksmanship_ptr( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );
  action_priority_list_t* drst = p->get_action_priority_list( "drst" );
  action_priority_list_t* sentst = p->get_action_priority_list( "sentst" );
  action_priority_list_t* drcleave = p->get_action_priority_list( "drcleave" );
  action_priority_list_t* sentcleave = p->get_action_priority_list( "sentcleave" );
  action_priority_list_t* drtrickshots = p->get_action_priority_list( "drtrickshots" );
  action_priority_list_t* senttrickshots = p->get_action_priority_list( "senttrickshots" );

  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "summon_pet,if=talent.unbreakable_bond" );
  precombat->add_action( "aimed_shot,if=active_enemies<3|talent.black_arrow&talent.headshot" );
  precombat->add_action( "steady_shot" );

  default_->add_action( "variable,name=trueshot_ready,value=!talent.bullseye|fight_remains>cooldown.trueshot.duration+10|buff.bullseye.stack=buff.bullseye.max_stack|fight_remains<25", "Hold the final Trueshot for Bullseye stacks if necessary." );
  default_->add_action( "variable,name=buffer_deathblow,value=hero_tree.dark_ranger&action.aimed_shot.in_flight&!action.black_arrow.ready" );
  default_->add_action( "auto_shot" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "call_action_list,name=drtrickshots,if=active_enemies>2&talent.trick_shots&hero_tree.dark_ranger" );
  default_->add_action( "call_action_list,name=senttrickshots,if=active_enemies>2&talent.trick_shots&hero_tree.sentinel" );
  default_->add_action( "call_action_list,name=drcleave,if=active_enemies>1&hero_tree.dark_ranger" );
  default_->add_action( "call_action_list,name=sentcleave,if=active_enemies>1&hero_tree.sentinel" );
  default_->add_action( "call_action_list,name=drst,if=hero_tree.dark_ranger" );
  default_->add_action( "call_action_list,name=sentst,if=hero_tree.sentinel" );

  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.trueshot.remains>12|fight_remains<13" );
  cds->add_action( "berserking,if=buff.trueshot.up|fight_remains<13" );
  cds->add_action( "blood_fury,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<16" );
  cds->add_action( "ancestral_call,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<16" );
  cds->add_action( "fireblood,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<9" );
  cds->add_action( "lights_judgment,if=buff.trueshot.down" );
  cds->add_action( "potion,if=buff.trueshot.up&(buff.bloodlust.up|target.health.pct<20)|fight_remains<31" );

  trinkets->add_action( "use_items,slots=trinket1:trinket2,if=!this_trinket.has_use_buff|buff.trueshot.remains>13|cooldown.trueshot.remains>45" );
  trinkets->add_action( "use_item,name=unyielding_netherprism,if=buff.latent_energy.stack>5&(buff.trueshot.remains>13|buff.latent_energy.stack=18|fight_remains<25)" );

  drst->add_action( "explosive_shot,if=talent.precision_detonation&buff.lock_and_load.down&cooldown.aimed_shot.charges_fractional<=1.1&buff.trueshot.down", "1 target" );
  drst->add_action( "volley,if=buff.double_tap.down&(!raid_event.adds.exists|raid_event.adds.in>cooldown)" );
  drst->add_action( "steady_shot,if=variable.buffer_deathblow&buff.trueshot.down&cooldown.trueshot.remains", "Queue Steady Shot after Aimed Shot if a Deathblow hasn't already been up long enough to be reacted to." );
  drst->add_action( "trueshot,if=variable.trueshot_ready&buff.double_tap.down&!action.black_arrow.cooldown_react&(!talent.bulletstorm|buff.bulletstorm.up)" );
  drst->add_action( "black_arrow,if=talent.headshot&buff.precise_shots.up&(debuff.spotters_mark.down|buff.moving_target.down)|!talent.headshot" );
  drst->add_action( "aimed_shot,if=buff.trueshot.up&buff.precise_shots.down|buff.lock_and_load.up&buff.moving_target.up" );
  drst->add_action( "rapid_fire,if=!buff.deathblow.react" );
  drst->add_action( "trueshot,if=variable.trueshot_ready&buff.double_tap.down&!buff.deathblow.react" );
  drst->add_action( "arcane_shot,if=buff.precise_shots.up&(debuff.spotters_mark.down|buff.moving_target.down)" );
  drst->add_action( "aimed_shot,if=buff.precise_shots.down|debuff.spotters_mark.up&buff.moving_target.up" );
  drst->add_action( "explosive_shot,if=talent.shrapnel_shot&buff.lock_and_load.down" );
  drst->add_action( "steady_shot" );

  sentst->add_action( "explosive_shot,if=talent.shrapnel_shot&buff.lock_and_load.down&cooldown.aimed_shot.charges_fractional<=1.1&buff.trueshot.down" );
  sentst->add_action( "volley,if=buff.double_tap.down&(!raid_event.adds.exists|raid_event.adds.in>cooldown)" );
  sentst->add_action( "rapid_fire,if=talent.lunar_storm&buff.lunar_storm_cooldown.down" );
  sentst->add_action( "trueshot,if=variable.trueshot_ready&buff.double_tap.down&(!talent.bulletstorm|buff.bulletstorm.up)" );
  sentst->add_action( "kill_shot,if=talent.headshot&buff.precise_shots.up&(debuff.spotters_mark.down|buff.moving_target.down)|!talent.headshot&buff.razor_fragments.up" );
  sentst->add_action( "aimed_shot,if=buff.trueshot.up&buff.precise_shots.down|buff.lock_and_load.up&buff.moving_target.up" );
  sentst->add_action( "rapid_fire,if=!talent.no_scope|buff.precise_shots.down" );
  sentst->add_action( "arcane_shot,if=buff.precise_shots.up&(debuff.spotters_mark.down|buff.moving_target.down)" );
  sentst->add_action( "rapid_fire" );
  sentst->add_action( "aimed_shot,if=buff.precise_shots.down|debuff.spotters_mark.up&buff.moving_target.up" );
  sentst->add_action( "explosive_shot,if=talent.precision_detonation|buff.trueshot.down" );
  sentst->add_action( "steady_shot" );

  drcleave->add_action( "explosive_shot,if=buff.trueshot.down&talent.precision_detonation&(!talent.shrapnel_shot|buff.lock_and_load.down&cooldown.aimed_shot.charges_fractional<=1.1)", "2 targets (2+ without Trick Shots)" );
  drcleave->add_action( "black_arrow,if=buff.precise_shots.up&buff.moving_target.down&variable.trueshot_ready" );
  drcleave->add_action( "volley,if=(talent.double_tap&buff.double_tap.down|!talent.aspect_of_the_hydra)&(buff.precise_shots.down|buff.moving_target.up)&(!talent.shrapnel_shot|!talent.salvo|buff.lock_and_load.down)" );
  drcleave->add_action( "rapid_fire,if=talent.bulletstorm&buff.bulletstorm.down&(!talent.double_tap|buff.double_tap.up|!talent.aspect_of_the_hydra&buff.trick_shots.remains>execute_time)&(buff.precise_shots.down|buff.moving_target.up|!talent.volley)" );
  drcleave->add_action( "volley,if=!talent.double_tap&(buff.precise_shots.down|buff.moving_target.up)&(!talent.shrapnel_shot|buff.lock_and_load.down)" );
  drcleave->add_action( "trueshot,if=variable.trueshot_ready&(buff.double_tap.down|!talent.volley)&(buff.precise_shots.down|buff.moving_target.up|!talent.volley)&(!talent.volley|!action.volley.ready)" );
  drcleave->add_action( "steady_shot,if=variable.buffer_deathblow&buff.trueshot.down&cooldown.trueshot.remains" );
  drcleave->add_action( "black_arrow,if=talent.headshot&buff.precise_shots.up&(debuff.spotters_mark.down|buff.moving_target.down)|!talent.headshot&buff.razor_fragments.up" );
  drcleave->add_action( "aimed_shot,target_if=max:debuff.spotters_mark.up,if=buff.trueshot.up&buff.precise_shots.down|buff.lock_and_load.up&buff.moving_target.up" );
  drcleave->add_action( "rapid_fire,if=!talent.bulletstorm|buff.bulletstorm.stack<=10|talent.aspect_of_the_hydra&buff.trick_shots.remains<action.aimed_shot.cast_time" );
  drcleave->add_action( "arcane_shot,target_if=max:debuff.spotters_mark.down|action.aimed_shot.in_flight_to_target|max_prio_damage,if=buff.precise_shots.up&(debuff.spotters_mark.down|buff.moving_target.down)" );
  drcleave->add_action( "aimed_shot,target_if=max:debuff.spotters_mark.up|max_prio_damage,if=buff.precise_shots.down|debuff.spotters_mark.up&buff.moving_target.up" );
  drcleave->add_action( "rapid_fire" );
  drcleave->add_action( "explosive_shot,if=talent.precision_detonation|buff.trueshot.down" );
  drcleave->add_action( "black_arrow,if=!talent.headshot" );
  drcleave->add_action( "steady_shot" );

  sentcleave->add_action( "explosive_shot,if=talent.precision_detonation&action.aimed_shot.in_flight&(buff.trueshot.down|!talent.windrunner_quiver)" );
  sentcleave->add_action( "volley,if=(talent.double_tap&buff.double_tap.down|!talent.aspect_of_the_hydra)&(buff.precise_shots.down|buff.moving_target.up)" );
  sentcleave->add_action( "rapid_fire,if=talent.bulletstorm&buff.bulletstorm.down&(!talent.double_tap|buff.double_tap.up|!talent.aspect_of_the_hydra&buff.trick_shots.remains>execute_time)&(buff.precise_shots.down|buff.moving_target.up|!talent.volley)" );
  sentcleave->add_action( "volley,if=!talent.double_tap&(buff.precise_shots.down|buff.moving_target.up)" );
  sentcleave->add_action( "trueshot,if=variable.trueshot_ready&(buff.double_tap.down|!talent.volley)&(buff.lunar_storm_ready.down|!talent.double_tap|!talent.volley)&(buff.precise_shots.down|buff.moving_target.up|!talent.volley)" );
  sentcleave->add_action( "rapid_fire,if=talent.lunar_storm&buff.lunar_storm_cooldown.down&(buff.precise_shots.down|buff.moving_target.up|cooldown.volley.remains&cooldown.trueshot.remains|!talent.volley)" );
  sentcleave->add_action( "kill_shot,target_if=max:debuff.spotters_mark.down|action.aimed_shot.in_flight_to_target|max_prio_damage,if=talent.headshot&buff.precise_shots.up&(debuff.spotters_mark.down|buff.moving_target.down)|!talent.headshot&buff.razor_fragments.up" );
  sentcleave->add_action( "multishot,target_if=max:debuff.spotters_mark.down|action.aimed_shot.in_flight_to_target|max_prio_damage,if=buff.precise_shots.up&(debuff.spotters_mark.down|buff.moving_target.down)&!talent.aspect_of_the_hydra" );
  sentcleave->add_action( "arcane_shot,target_if=max:debuff.spotters_mark.down|action.aimed_shot.in_flight_to_target|max_prio_damage,if=buff.precise_shots.up&(debuff.spotters_mark.down|buff.moving_target.down)" );
  sentcleave->add_action( "aimed_shot,target_if=max:debuff.spotters_mark.up,if=(buff.precise_shots.down|debuff.spotters_mark.up&buff.moving_target.up)&full_recharge_time<action.rapid_fire.execute_time+cast_time&(!talent.bulletstorm|buff.bulletstorm.up)&talent.windrunner_quiver" );
  sentcleave->add_action( "rapid_fire,if=!talent.bulletstorm|buff.bulletstorm.stack<=10|talent.aspect_of_the_hydra" );
  sentcleave->add_action( "aimed_shot,target_if=max:debuff.spotters_mark.up|max_prio_damage,if=buff.precise_shots.down|debuff.spotters_mark.up&buff.moving_target.up" );
  sentcleave->add_action( "rapid_fire" );
  sentcleave->add_action( "explosive_shot,if=talent.precision_detonation|buff.trueshot.down" );
  sentcleave->add_action( "steady_shot" );

  drtrickshots->add_action( "explosive_shot,if=talent.precision_detonation&buff.trueshot.down&(!talent.shrapnel_shot|buff.lock_and_load.down)", "3+ targets (with Trick Shots)" );
  drtrickshots->add_action( "volley,if=buff.double_tap.down&(!talent.shrapnel_shot|buff.lock_and_load.down)" );
  drtrickshots->add_action( "rapid_fire,if=buff.trick_shots.remains>execute_time&talent.bulletstorm&buff.bulletstorm.down" );
  drtrickshots->add_action( "steady_shot,if=variable.buffer_deathblow&buff.trueshot.down&cooldown.trueshot.remains" );
  drtrickshots->add_action( "black_arrow,if=!talent.headshot|buff.precise_shots.up|buff.trick_shots.down" );
  drtrickshots->add_action( "multishot,target_if=max:debuff.spotters_mark.down|action.aimed_shot.in_flight_to_target,if=buff.precise_shots.up&buff.moving_target.down|buff.trick_shots.down" );
  drtrickshots->add_action( "trueshot,if=variable.trueshot_ready&buff.double_tap.down" );
  drtrickshots->add_action( "volley,if=buff.double_tap.down&(!talent.salvo|!talent.precision_detonation|(buff.precise_shots.down|debuff.spotters_mark.up&buff.moving_target.up))" );
  drtrickshots->add_action( "aimed_shot,if=buff.trick_shots.remains>cast_time&(buff.trueshot.up&buff.precise_shots.down|buff.lock_and_load.up&buff.moving_target.up)" );
  drtrickshots->add_action( "rapid_fire,if=buff.trick_shots.remains>execute_time&!buff.deathblow.react&(talent.no_scope|talent.double_tap)" );
  drtrickshots->add_action( "explosive_shot,if=talent.shrapnel_shot&buff.lock_and_load.down&(buff.precise_shots.down|debuff.spotters_mark.up&buff.moving_target.up)" );
  drtrickshots->add_action( "aimed_shot,if=buff.trick_shots.remains>cast_time&(buff.precise_shots.down|debuff.spotters_mark.up&buff.moving_target.up)" );
  drtrickshots->add_action( "steady_shot" );

  senttrickshots->add_action( "explosive_shot,if=talent.shrapnel_shot&buff.lock_and_load.down&cooldown.aimed_shot.charges_fractional<=1.1&buff.trueshot.down" );
  senttrickshots->add_action( "volley,if=buff.double_tap.down&(!talent.shrapnel_shot|buff.lock_and_load.down)" );
  senttrickshots->add_action( "rapid_fire,if=buff.trick_shots.remains>execute_time&(talent.bulletstorm&buff.bulletstorm.down|buff.lunar_storm_cooldown.down)" );
  senttrickshots->add_action( "multishot,target_if=max:debuff.spotters_mark.down|action.aimed_shot.in_flight_to_target,if=buff.precise_shots.up&buff.moving_target.down|buff.trick_shots.down" );
  senttrickshots->add_action( "trueshot,if=variable.trueshot_ready&buff.double_tap.down" );
  senttrickshots->add_action( "volley,if=buff.double_tap.down&(!talent.salvo|!talent.precision_detonation|(buff.precise_shots.down|debuff.spotters_mark.up&buff.moving_target.up))" );
  senttrickshots->add_action( "aimed_shot,if=buff.trick_shots.remains>execute_time&(buff.trueshot.up&buff.precise_shots.down|buff.lock_and_load.up&buff.moving_target.up)" );
  senttrickshots->add_action( "rapid_fire,if=buff.trick_shots.remains>execute_time" );
  senttrickshots->add_action( "explosive_shot,if=talent.shrapnel_shot&buff.lock_and_load.down" );
  senttrickshots->add_action( "aimed_shot,if=buff.trick_shots.remains>execute_time&(buff.precise_shots.down|debuff.spotters_mark.up&buff.moving_target.up)" );
  senttrickshots->add_action( "explosive_shot,if=talent.precision_detonation&!talent.shrapnel_shot" );
  senttrickshots->add_action( "steady_shot,if=focus+cast_regen<focus.max" );
  senttrickshots->add_action( "multishot" );
}
//marksmanship_ptr_apl_end

//survival_apl_start
void survival( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* plcleave = p->get_action_priority_list( "plcleave" );
  action_priority_list_t* plst = p->get_action_priority_list( "plst" );
  action_priority_list_t* sentcleave = p->get_action_priority_list( "sentcleave" );
  action_priority_list_t* sentst = p->get_action_priority_list( "sentst" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );

  precombat->add_action( "summon_pet" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins." );
  precombat->add_action( "variable,name=stronger_trinket_slot,op=setif,value=1,value_else=2,condition=!trinket.2.is.house_of_cards&(trinket.1.is.house_of_cards|!trinket.2.has_cooldown|trinket.1.has_use_buff&(!trinket.2.has_use_buff|trinket.2.cooldown.duration<trinket.1.cooldown.duration|trinket.2.cast_time<trinket.1.cast_time|trinket.2.cast_time=trinket.1.cast_time&trinket.2.cooldown.duration=trinket.1.cooldown.duration)|!trinket.1.has_use_buff&(!trinket.2.has_use_buff&(trinket.2.cooldown.duration<trinket.1.cooldown.duration|trinket.2.cast_time<trinket.1.cast_time|trinket.2.cast_time=trinket.1.cast_time&trinket.2.cooldown.duration=trinket.1.cooldown.duration)))", "Determine which trinket would make for the strongest cooldown sync. In descending priority: buff effects > damage effects, longer > shorter cooldowns, longer > shorter cast times." );

  default_->add_action( "auto_attack" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "call_action_list,name=plst,if=active_enemies<3&talent.howl_of_the_pack_leader" );
  default_->add_action( "call_action_list,name=plcleave,if=active_enemies>2&talent.howl_of_the_pack_leader" );
  default_->add_action( "call_action_list,name=sentst,if=active_enemies<3&!talent.howl_of_the_pack_leader" );
  default_->add_action( "call_action_list,name=sentcleave,if=active_enemies>2&!talent.howl_of_the_pack_leader" );
  default_->add_action( "arcane_torrent", "simply fires off if there is absolutely nothing else to press." );
  default_->add_action( "bag_of_tricks" );
  default_->add_action( "lights_judgment" );

  cds->add_action( "blood_fury,if=buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault", "COOLDOWNS ACTIONLIST" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=(buff.coordinated_assault.up&buff.coordinated_assault.remains>7&!buff.power_infusion.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault)" );
  cds->add_action( "harpoon,if=prev.kill_command" );
  cds->add_action( "ancestral_call,if=buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "fireblood,if=buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "berserking,if=buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault|time_to_die<13" );
  cds->add_action( "muzzle" );
  cds->add_action( "potion,if=target.time_to_die<25|buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=buff.coordinated_assault.up&trinket.1.has_use_buff|cooldown.coordinated_assault.remains>31|!trinket.1.has_use_buff&cooldown.coordinated_assault.remains>20|time_to_die<cooldown.coordinated_assault.remains" );
  cds->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=buff.coordinated_assault.up&trinket.2.has_use_buff|cooldown.coordinated_assault.remains>31|!trinket.2.has_use_buff&cooldown.coordinated_assault.remains>20|time_to_die<cooldown.coordinated_assault.remains" );
  cds->add_action( "aspect_of_the_eagle,if=target.distance>=6" );
  cds->add_action( "use_item,name=spellstrike_warplance" );

  plcleave->add_action( "spearhead,if=cooldown.coordinated_assault.remains", "PACK LEADER | AOE ACTIONLIST" );
  plcleave->add_action( "kill_command,if=buff.relentless_primal_ferocity.up&buff.tip_of_the_spear.stack<1" );
  plcleave->add_action( "fury_of_the_eagle,if=buff.tip_of_the_spear.stack>0" );
  plcleave->add_action( "wildfire_bomb,if=cooldown.wildfire_bomb.charges_fractional>1.7" );
  plcleave->add_action( "explosive_shot,if=buff.tip_of_the_spear.stack>0" );
  plcleave->add_action( "raptor_bite,target_if=max:dot.serpent_sting.remains,if=buff.strike_it_rich.up&buff.strike_it_rich.remains<gcd|buff.hogstrider.remains&boar_charge.remains>0|buff.hogstrider.remains<gcd&buff.hogstrider.up|buff.hogstrider.remains&buff.strike_it_rich.remains|raid_event.adds.exists&raid_event.adds.remains<4" );
  plcleave->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0" );
  plcleave->add_action( "kill_command,if=(buff.howl_of_the_pack_leader_wyvern.remains|buff.howl_of_the_pack_leader_boar.remains|buff.howl_of_the_pack_leader_bear.remains)" );
  plcleave->add_action( "flanking_strike,if=buff.tip_of_the_spear.stack>0" );
  plcleave->add_action( "butchery,if=cooldown.wildfire_bomb.charges_fractional<1.5" );
  plcleave->add_action( "coordinated_assault,if=buff.howl_of_the_pack_leader_cooldown.remains-buff.lead_from_the_front.duration<buff.lead_from_the_front.duration%gcd*0.6" );
  plcleave->add_action( "kill_command,if=focus+cast_regen<focus.max" );
  plcleave->add_action( "explosive_shot" );
  plcleave->add_action( "kill_shot,if=buff.deathblow.remains&talent.sic_em" );
  plcleave->add_action( "raptor_bite,target_if=min:dot.serpent_sting.remains,if=!talent.contagious_reagents" );
  plcleave->add_action( "raptor_bite,target_if=max:dot.serpent_sting.remains" );

  plst->add_action( "kill_command,if=(buff.relentless_primal_ferocity.up&buff.tip_of_the_spear.stack<1)|(buff.howl_of_the_pack_leader_wyvern.remains|buff.howl_of_the_pack_leader_boar.remains|buff.howl_of_the_pack_leader_bear.remains)", "PACK LEADER | SINGLE TARGET ACTIONLIST." );
  plst->add_action( "spearhead,if=cooldown.coordinated_assault.remains" );
  plst->add_action( "flanking_strike,if=buff.tip_of_the_spear.stack>0&(cooldown.spearhead.remains>5|!talent.spearhead&cooldown.coordinated_assault.remains>5)" );
  plst->add_action( "raptor_bite,target_if=min:dot.serpent_sting.remains,if=!dot.serpent_sting.ticking&target.time_to_die>12&(!talent.contagious_reagents|active_dot.serpent_sting=0)" );
  plst->add_action( "raptor_bite,target_if=max:dot.serpent_sting.remains,if=talent.contagious_reagents&active_dot.serpent_sting<active_enemies&dot.serpent_sting.remains" );
  plst->add_action( "kill_command,if=buff.strike_it_rich.remains&buff.tip_of_the_spear.stack<1" );
  plst->add_action( "raptor_bite,if=buff.strike_it_rich.remains&buff.tip_of_the_spear.stack>0" );
  plst->add_action( "fury_of_the_eagle,if=buff.tip_of_the_spear.stack>0&(!raid_event.adds.exists|raid_event.adds.exists&raid_event.adds.in>40)" );
  plst->add_action( "coordinated_assault,if=buff.howl_of_the_pack_leader_cooldown.remains-buff.lead_from_the_front.duration<buff.lead_from_the_front.duration%gcd*0.6|time_to_die<20|!talent.spearhead" );
  plst->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0" );
  plst->add_action( "raptor_bite,target_if=max:dot.serpent_sting.remains,if=buff.howl_of_the_pack_leader_cooldown.up&buff.howl_of_the_pack_leader_cooldown.remains<2*gcd" );
  plst->add_action( "kill_command,if=focus+cast_regen<focus.max&(!buff.relentless_primal_ferocity.up|(buff.relentless_primal_ferocity.up&buff.tip_of_the_spear.stack<2|focus<30))" );
  plst->add_action( "explosive_shot,if=active_enemies>1" );
  plst->add_action( "kill_shot,if=talent.cull_the_herd" );
  plst->add_action( "raptor_bite,target_if=min:dot.serpent_sting.remains,if=!talent.contagious_reagents" );
  plst->add_action( "raptor_bite,target_if=max:dot.serpent_sting.remains" );
  plst->add_action( "kill_shot" );
  plst->add_action( "explosive_shot" );

  sentcleave->add_action( "wildfire_bomb,if=!buff.lunar_storm_cooldown.remains", "SENTINEL | DEFAULT AOE ACTIONLIST" );
  sentcleave->add_action( "kill_command,if=buff.relentless_primal_ferocity.up&buff.tip_of_the_spear.stack<1" );
  sentcleave->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0|cooldown.wildfire_bomb.charges_fractional>1.9|(talent.bombardier&cooldown.coordinated_assault.remains<2*gcd)|talent.butchery&cooldown.butchery.remains<gcd" );
  sentcleave->add_action( "fury_of_the_eagle,if=buff.tip_of_the_spear.stack>0" );
  sentcleave->add_action( "raptor_bite,target_if=max:dot.serpent_sting.remains,if=buff.strike_it_rich.up&buff.strike_it_rich.remains<gcd" );
  sentcleave->add_action( "butchery" );
  sentcleave->add_action( "explosive_shot,if=buff.tip_of_the_spear.stack>0" );
  sentcleave->add_action( "coordinated_assault" );
  sentcleave->add_action( "flanking_strike,if=(buff.tip_of_the_spear.stack=2|buff.tip_of_the_spear.stack=1)" );
  sentcleave->add_action( "kill_command,if=focus+cast_regen<focus.max" );
  sentcleave->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0" );
  sentcleave->add_action( "explosive_shot" );
  sentcleave->add_action( "kill_shot,if=buff.deathblow.remains&talent.sic_em" );
  sentcleave->add_action( "raptor_bite,target_if=min:dot.serpent_sting.remains,if=!talent.contagious_reagents" );
  sentcleave->add_action( "raptor_bite,target_if=max:dot.serpent_sting.remains" );

  sentst->add_action( "wildfire_bomb,if=!buff.lunar_storm_cooldown.remains&buff.tip_of_the_spear.stack>0", "SENTINEL | DEFAULT SINGLE TARGET ACTIONLIST." );
  sentst->add_action( "kill_command,if=(buff.relentless_primal_ferocity.up&buff.tip_of_the_spear.stack<1)" );
  sentst->add_action( "spearhead,if=cooldown.coordinated_assault.remains" );
  sentst->add_action( "flanking_strike,if=buff.tip_of_the_spear.stack>0" );
  sentst->add_action( "kill_command,if=buff.strike_it_rich.remains&buff.tip_of_the_spear.stack<1" );
  sentst->add_action( "mongoose_bite,if=buff.strike_it_rich.remains&buff.coordinated_assault.up" );
  sentst->add_action( "wildfire_bomb,if=cooldown.wildfire_bomb.charges_fractional>1.7" );
  sentst->add_action( "butchery" );
  sentst->add_action( "coordinated_assault,if=!talent.bombardier|talent.bombardier&cooldown.wildfire_bomb.charges_fractional<2" );
  sentst->add_action( "fury_of_the_eagle,if=buff.tip_of_the_spear.stack>0" );
  sentst->add_action( "kill_command,if=buff.tip_of_the_spear.stack<1&cooldown.flanking_strike.remains<gcd" );
  sentst->add_action( "kill_command,if=focus+cast_regen<focus.max&(!buff.relentless_primal_ferocity.up|(buff.relentless_primal_ferocity.up&(buff.tip_of_the_spear.stack<1|focus<30)))" );
  sentst->add_action( "mongoose_bite,if=buff.mongoose_fury.remains<gcd&buff.mongoose_fury.stack>0" );
  sentst->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0" );
  sentst->add_action( "explosive_shot" );
  sentst->add_action( "mongoose_bite,if=buff.mongoose_fury.remains" );
  sentst->add_action( "kill_shot" );
  sentst->add_action( "raptor_bite,target_if=min:dot.serpent_sting.remains,if=!talent.contagious_reagents" );
  sentst->add_action( "raptor_bite,target_if=max:dot.serpent_sting.remains" );

  trinkets->add_action( "variable,name=buff_sync_ready,value=buff.coordinated_assault.up", "True if effects that are desirable to sync a trinket buff with are ready." );
  trinkets->add_action( "variable,name=buff_sync_remains,value=cooldown.coordinated_assault.remains", "Time until the effects that are desirable to sync a trinket buff with will be ready." );
  trinkets->add_action( "variable,name=buff_sync_active,value=buff.coordinated_assault.up", "True if effecs that are desirable to sync a trinket buff with are active." );
  trinkets->add_action( "variable,name=damage_sync_active,value=1", "True if effects that are desirable to sync trinket damage with are active." );
  trinkets->add_action( "variable,name=damage_sync_remains,value=0", "Time until the effects that are desirable to sync trinket damage with will be ready." );
  trinkets->add_action( "use_items,slots=trinket1:trinket2,if=this_trinket.has_use_buff&(variable.buff_sync_ready&(variable.stronger_trinket_slot=this_trinket_slot|other_trinket.cooldown.remains)|!variable.buff_sync_ready&(variable.stronger_trinket_slot=this_trinket_slot&(variable.buff_sync_remains>this_trinket.cooldown.duration%3&fight_remains>this_trinket.cooldown.duration+20|other_trinket.has_use_buff&other_trinket.cooldown.remains>variable.buff_sync_remains-15&other_trinket.cooldown.remains-5<variable.buff_sync_remains&variable.buff_sync_remains+45>fight_remains)|variable.stronger_trinket_slot!=this_trinket_slot&(other_trinket.cooldown.remains&(other_trinket.cooldown.remains-5<variable.buff_sync_remains&variable.buff_sync_remains>=20|other_trinket.cooldown.remains-5>=variable.buff_sync_remains&(variable.buff_sync_remains>this_trinket.cooldown.duration%3|this_trinket.cooldown.duration<fight_remains&(variable.buff_sync_remains+this_trinket.cooldown.duration>fight_remains)))|other_trinket.cooldown.ready&variable.buff_sync_remains>20&variable.buff_sync_remains<other_trinket.cooldown.duration%3)))|!this_trinket.has_use_buff&(this_trinket.cast_time=0|!variable.buff_sync_active)&(!this_trinket.is.junkmaestros_mega_magnet|buff.junkmaestros_mega_magnet.stack>10)&(!other_trinket.has_cooldown&(variable.damage_sync_active|this_trinket.is.junkmaestros_mega_magnet&buff.junkmaestros_mega_magnet.stack>25|!this_trinket.is.junkmaestros_mega_magnet&variable.damage_sync_remains>this_trinket.cooldown.duration%3)|other_trinket.has_cooldown&(!other_trinket.has_use_buff&(variable.stronger_trinket_slot=this_trinket_slot|other_trinket.cooldown.remains)&(variable.damage_sync_active|this_trinket.is.junkmaestros_mega_magnet&buff.junkmaestros_mega_magnet.stack>25|variable.damage_sync_remains>this_trinket.cooldown.duration%3&!this_trinket.is.junkmaestros_mega_magnet|other_trinket.cooldown.remains-5<variable.damage_sync_remains&variable.damage_sync_remains>=20)|other_trinket.has_use_buff&(variable.damage_sync_active|this_trinket.is.junkmaestros_mega_magnet&buff.junkmaestros_mega_magnet.stack>25|!this_trinket.is.junkmaestros_mega_magnet&variable.damage_sync_remains>this_trinket.cooldown.duration%3)&(other_trinket.cooldown.remains>=20|other_trinket.cooldown.remains-5>variable.buff_sync_remains)))|fight_remains<25&(variable.stronger_trinket_slot=this_trinket_slot|other_trinket.cooldown.remains)", "Uses buff effect trinkets with cooldowns and is willing to delay usage up to 1/3 the trinket cooldown if it won't lose a usage in the fight. Fills in downtime with weaker buff effects if they won't also be saved for later cooldowns (happens if it won't delay over 1/3 the trinket cooldown and a stronger trinket won't be up in time) or damage effects if they won't inferfere with any buff effect usage." );
}
//survival_apl_end

//survival_ptr_apl_start
void survival_ptr( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* plcleave = p->get_action_priority_list( "plcleave" );
  action_priority_list_t* plst = p->get_action_priority_list( "plst" );
  action_priority_list_t* sentcleave = p->get_action_priority_list( "sentcleave" );
  action_priority_list_t* sentst = p->get_action_priority_list( "sentst" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );

  precombat->add_action( "summon_pet" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins." );
  precombat->add_action( "variable,name=stronger_trinket_slot,op=setif,value=1,value_else=2,condition=!trinket.2.is.house_of_cards&(trinket.1.is.house_of_cards|!trinket.2.has_cooldown|trinket.1.has_use_buff&(!trinket.2.has_use_buff|trinket.2.cooldown.duration<trinket.1.cooldown.duration|trinket.2.cast_time<trinket.1.cast_time|trinket.2.cast_time=trinket.1.cast_time&trinket.2.cooldown.duration=trinket.1.cooldown.duration)|!trinket.1.has_use_buff&(!trinket.2.has_use_buff&(trinket.2.cooldown.duration<trinket.1.cooldown.duration|trinket.2.cast_time<trinket.1.cast_time|trinket.2.cast_time=trinket.1.cast_time&trinket.2.cooldown.duration=trinket.1.cooldown.duration)))", "Determine which trinket would make for the strongest cooldown sync. In descending priority: buff effects > damage effects, longer > shorter cooldowns, longer > shorter cast times." );

  default_->add_action( "auto_attack" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "call_action_list,name=plst,if=active_enemies<3&talent.howl_of_the_pack_leader" );
  default_->add_action( "call_action_list,name=plcleave,if=active_enemies>2&talent.howl_of_the_pack_leader" );
  default_->add_action( "call_action_list,name=sentst,if=active_enemies<3&!talent.howl_of_the_pack_leader" );
  default_->add_action( "call_action_list,name=sentcleave,if=active_enemies>2&!talent.howl_of_the_pack_leader" );
  default_->add_action( "arcane_torrent", "simply fires off if there is absolutely nothing else to press." );
  default_->add_action( "bag_of_tricks" );
  default_->add_action( "lights_judgment" );

  cds->add_action( "blood_fury,if=buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault", "COOLDOWNS ACTIONLIST" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=(buff.coordinated_assault.up&buff.coordinated_assault.remains>7&!buff.power_infusion.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault)" );
  cds->add_action( "harpoon,if=prev.kill_command" );
  cds->add_action( "ancestral_call,if=buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "fireblood,if=buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "berserking,if=buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault|time_to_die<13" );
  cds->add_action( "muzzle" );
  cds->add_action( "potion,if=target.time_to_die<25|buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=buff.coordinated_assault.up&trinket.1.has_use_buff|cooldown.coordinated_assault.remains>31|!trinket.1.has_use_buff&cooldown.coordinated_assault.remains>20|time_to_die<cooldown.coordinated_assault.remains" );
  cds->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=buff.coordinated_assault.up&trinket.2.has_use_buff|cooldown.coordinated_assault.remains>31|!trinket.2.has_use_buff&cooldown.coordinated_assault.remains>20|time_to_die<cooldown.coordinated_assault.remains" );
  cds->add_action( "aspect_of_the_eagle,if=target.distance>=6" );
  cds->add_action( "use_item,name=spellstrike_warplance" );

  plcleave->add_action( "spearhead,if=cooldown.coordinated_assault.remains", "PACK LEADER | AOE ACTIONLIST" );
  plcleave->add_action( "kill_command,if=buff.relentless_primal_ferocity.up&buff.tip_of_the_spear.stack<1" );
  plcleave->add_action( "fury_of_the_eagle,if=buff.tip_of_the_spear.stack>0" );
  plcleave->add_action( "wildfire_bomb,if=cooldown.wildfire_bomb.charges_fractional>1.7" );
  plcleave->add_action( "explosive_shot,if=buff.tip_of_the_spear.stack>0" );
  plcleave->add_action( "raptor_bite,target_if=max:dot.serpent_sting.remains,if=buff.strike_it_rich.up&buff.strike_it_rich.remains<gcd|buff.hogstrider.remains&boar_charge.remains>0|buff.hogstrider.remains<gcd&buff.hogstrider.up|buff.hogstrider.remains&buff.strike_it_rich.remains|raid_event.adds.exists&raid_event.adds.remains<4" );
  plcleave->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0" );
  plcleave->add_action( "kill_command,if=(buff.howl_of_the_pack_leader_wyvern.remains|buff.howl_of_the_pack_leader_boar.remains|buff.howl_of_the_pack_leader_bear.remains)" );
  plcleave->add_action( "flanking_strike,if=buff.tip_of_the_spear.stack>0" );
  plcleave->add_action( "butchery,if=cooldown.wildfire_bomb.charges_fractional<1.5" );
  plcleave->add_action( "coordinated_assault,if=buff.howl_of_the_pack_leader_cooldown.remains-buff.lead_from_the_front.duration<buff.lead_from_the_front.duration%gcd*0.6" );
  plcleave->add_action( "kill_command,if=focus+cast_regen<focus.max" );
  plcleave->add_action( "explosive_shot" );
  plcleave->add_action( "kill_shot,if=buff.deathblow.remains&talent.sic_em" );
  plcleave->add_action( "raptor_bite,target_if=min:dot.serpent_sting.remains,if=!talent.contagious_reagents" );
  plcleave->add_action( "raptor_bite,target_if=max:dot.serpent_sting.remains" );

  plst->add_action( "kill_command,if=(buff.relentless_primal_ferocity.up&buff.tip_of_the_spear.stack<1)|(buff.howl_of_the_pack_leader_wyvern.remains|buff.howl_of_the_pack_leader_boar.remains|buff.howl_of_the_pack_leader_bear.remains)", "PACK LEADER | SINGLE TARGET ACTIONLIST." );
  plst->add_action( "spearhead,if=cooldown.coordinated_assault.remains" );
  plst->add_action( "flanking_strike,if=buff.tip_of_the_spear.stack>0&(cooldown.spearhead.remains>5|!talent.spearhead&cooldown.coordinated_assault.remains>5)" );
  plst->add_action( "raptor_bite,target_if=min:dot.serpent_sting.remains,if=!dot.serpent_sting.ticking&target.time_to_die>12&(!talent.contagious_reagents|active_dot.serpent_sting=0)" );
  plst->add_action( "raptor_bite,target_if=max:dot.serpent_sting.remains,if=talent.contagious_reagents&active_dot.serpent_sting<active_enemies&dot.serpent_sting.remains" );
  plst->add_action( "kill_command,if=buff.strike_it_rich.remains&buff.tip_of_the_spear.stack<1" );
  plst->add_action( "raptor_bite,if=buff.strike_it_rich.remains&buff.tip_of_the_spear.stack>0" );
  plst->add_action( "fury_of_the_eagle,if=buff.tip_of_the_spear.stack>0&(!raid_event.adds.exists|raid_event.adds.exists&raid_event.adds.in>40)" );
  plst->add_action( "coordinated_assault,if=buff.howl_of_the_pack_leader_cooldown.remains-buff.lead_from_the_front.duration<buff.lead_from_the_front.duration%gcd*0.6|time_to_die<20|!talent.spearhead" );
  plst->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0" );
  plst->add_action( "raptor_bite,target_if=max:dot.serpent_sting.remains,if=buff.howl_of_the_pack_leader_cooldown.up&buff.howl_of_the_pack_leader_cooldown.remains<2*gcd" );
  plst->add_action( "kill_command,if=focus+cast_regen<focus.max&(!buff.relentless_primal_ferocity.up|(buff.relentless_primal_ferocity.up&buff.tip_of_the_spear.stack<2|focus<30))" );
  plst->add_action( "explosive_shot,if=active_enemies>1" );
  plst->add_action( "kill_shot,if=talent.cull_the_herd" );
  plst->add_action( "raptor_bite,target_if=min:dot.serpent_sting.remains,if=!talent.contagious_reagents" );
  plst->add_action( "raptor_bite,target_if=max:dot.serpent_sting.remains" );
  plst->add_action( "kill_shot" );
  plst->add_action( "explosive_shot" );

  sentcleave->add_action( "wildfire_bomb,if=!buff.lunar_storm_cooldown.remains", "SENTINEL | DEFAULT AOE ACTIONLIST" );
  sentcleave->add_action( "kill_command,if=buff.relentless_primal_ferocity.up&buff.tip_of_the_spear.stack<1" );
  sentcleave->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0|cooldown.wildfire_bomb.charges_fractional>1.9|(talent.bombardier&cooldown.coordinated_assault.remains<2*gcd)|talent.butchery&cooldown.butchery.remains<gcd" );
  sentcleave->add_action( "fury_of_the_eagle,if=buff.tip_of_the_spear.stack>0" );
  sentcleave->add_action( "raptor_bite,target_if=max:dot.serpent_sting.remains,if=buff.strike_it_rich.up&buff.strike_it_rich.remains<gcd" );
  sentcleave->add_action( "butchery" );
  sentcleave->add_action( "explosive_shot,if=buff.tip_of_the_spear.stack>0" );
  sentcleave->add_action( "coordinated_assault" );
  sentcleave->add_action( "flanking_strike,if=(buff.tip_of_the_spear.stack=2|buff.tip_of_the_spear.stack=1)" );
  sentcleave->add_action( "kill_command,if=focus+cast_regen<focus.max" );
  sentcleave->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0" );
  sentcleave->add_action( "explosive_shot" );
  sentcleave->add_action( "kill_shot,if=buff.deathblow.remains&talent.sic_em" );
  sentcleave->add_action( "raptor_bite,target_if=min:dot.serpent_sting.remains,if=!talent.contagious_reagents" );
  sentcleave->add_action( "raptor_bite,target_if=max:dot.serpent_sting.remains" );

  sentst->add_action( "wildfire_bomb,if=!buff.lunar_storm_cooldown.remains&buff.tip_of_the_spear.stack>0", "SENTINEL | DEFAULT SINGLE TARGET ACTIONLIST." );
  sentst->add_action( "kill_command,if=(buff.relentless_primal_ferocity.up&buff.tip_of_the_spear.stack<1)" );
  sentst->add_action( "spearhead,if=cooldown.coordinated_assault.remains" );
  sentst->add_action( "flanking_strike,if=buff.tip_of_the_spear.stack>0" );
  sentst->add_action( "kill_command,if=buff.strike_it_rich.remains&buff.tip_of_the_spear.stack<1" );
  sentst->add_action( "mongoose_bite,if=buff.strike_it_rich.remains&buff.coordinated_assault.up" );
  sentst->add_action( "wildfire_bomb,if=cooldown.wildfire_bomb.charges_fractional>1.7" );
  sentst->add_action( "butchery" );
  sentst->add_action( "coordinated_assault,if=!talent.bombardier|talent.bombardier&cooldown.wildfire_bomb.charges_fractional<2" );
  sentst->add_action( "fury_of_the_eagle,if=buff.tip_of_the_spear.stack>0" );
  sentst->add_action( "kill_command,if=buff.tip_of_the_spear.stack<1&cooldown.flanking_strike.remains<gcd" );
  sentst->add_action( "kill_command,if=focus+cast_regen<focus.max&(!buff.relentless_primal_ferocity.up|(buff.relentless_primal_ferocity.up&(buff.tip_of_the_spear.stack<1|focus<30)))" );
  sentst->add_action( "mongoose_bite,if=buff.mongoose_fury.remains<gcd&buff.mongoose_fury.stack>0" );
  sentst->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0" );
  sentst->add_action( "explosive_shot" );
  sentst->add_action( "mongoose_bite,if=buff.mongoose_fury.remains" );
  sentst->add_action( "kill_shot" );
  sentst->add_action( "raptor_bite,target_if=min:dot.serpent_sting.remains,if=!talent.contagious_reagents" );
  sentst->add_action( "raptor_bite,target_if=max:dot.serpent_sting.remains" );

  trinkets->add_action( "variable,name=buff_sync_ready,value=buff.coordinated_assault.up", "True if effects that are desirable to sync a trinket buff with are ready." );
  trinkets->add_action( "variable,name=buff_sync_remains,value=cooldown.coordinated_assault.remains", "Time until the effects that are desirable to sync a trinket buff with will be ready." );
  trinkets->add_action( "variable,name=buff_sync_active,value=buff.coordinated_assault.up", "True if effecs that are desirable to sync a trinket buff with are active." );
  trinkets->add_action( "variable,name=damage_sync_active,value=1", "True if effects that are desirable to sync trinket damage with are active." );
  trinkets->add_action( "variable,name=damage_sync_remains,value=0", "Time until the effects that are desirable to sync trinket damage with will be ready." );
  trinkets->add_action( "use_items,slots=trinket1:trinket2,if=this_trinket.has_use_buff&(variable.buff_sync_ready&(variable.stronger_trinket_slot=this_trinket_slot|other_trinket.cooldown.remains)|!variable.buff_sync_ready&(variable.stronger_trinket_slot=this_trinket_slot&(variable.buff_sync_remains>this_trinket.cooldown.duration%3&fight_remains>this_trinket.cooldown.duration+20|other_trinket.has_use_buff&other_trinket.cooldown.remains>variable.buff_sync_remains-15&other_trinket.cooldown.remains-5<variable.buff_sync_remains&variable.buff_sync_remains+45>fight_remains)|variable.stronger_trinket_slot!=this_trinket_slot&(other_trinket.cooldown.remains&(other_trinket.cooldown.remains-5<variable.buff_sync_remains&variable.buff_sync_remains>=20|other_trinket.cooldown.remains-5>=variable.buff_sync_remains&(variable.buff_sync_remains>this_trinket.cooldown.duration%3|this_trinket.cooldown.duration<fight_remains&(variable.buff_sync_remains+this_trinket.cooldown.duration>fight_remains)))|other_trinket.cooldown.ready&variable.buff_sync_remains>20&variable.buff_sync_remains<other_trinket.cooldown.duration%3)))|!this_trinket.has_use_buff&(this_trinket.cast_time=0|!variable.buff_sync_active)&(!this_trinket.is.junkmaestros_mega_magnet|buff.junkmaestros_mega_magnet.stack>10)&(!other_trinket.has_cooldown&(variable.damage_sync_active|this_trinket.is.junkmaestros_mega_magnet&buff.junkmaestros_mega_magnet.stack>25|!this_trinket.is.junkmaestros_mega_magnet&variable.damage_sync_remains>this_trinket.cooldown.duration%3)|other_trinket.has_cooldown&(!other_trinket.has_use_buff&(variable.stronger_trinket_slot=this_trinket_slot|other_trinket.cooldown.remains)&(variable.damage_sync_active|this_trinket.is.junkmaestros_mega_magnet&buff.junkmaestros_mega_magnet.stack>25|variable.damage_sync_remains>this_trinket.cooldown.duration%3&!this_trinket.is.junkmaestros_mega_magnet|other_trinket.cooldown.remains-5<variable.damage_sync_remains&variable.damage_sync_remains>=20)|other_trinket.has_use_buff&(variable.damage_sync_active|this_trinket.is.junkmaestros_mega_magnet&buff.junkmaestros_mega_magnet.stack>25|!this_trinket.is.junkmaestros_mega_magnet&variable.damage_sync_remains>this_trinket.cooldown.duration%3)&(other_trinket.cooldown.remains>=20|other_trinket.cooldown.remains-5>variable.buff_sync_remains)))|fight_remains<25&(variable.stronger_trinket_slot=this_trinket_slot|other_trinket.cooldown.remains)", "Uses buff effect trinkets with cooldowns and is willing to delay usage up to 1/3 the trinket cooldown if it won't lose a usage in the fight. Fills in downtime with weaker buff effects if they won't also be saved for later cooldowns (happens if it won't delay over 1/3 the trinket cooldown and a stronger trinket won't be up in time) or damage effects if they won't inferfere with any buff effect usage." );
}
//survival_ptr_apl_end

} // namespace hunter_apl

