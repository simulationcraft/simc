#include "simulationcraft.hpp"
#include "class_modules/apl/apl_hunter.hpp"

namespace hunter_apl {

std::string potion( const player_t* p )
{
  return ( p -> true_level > 60 ) ? "elemental_potion_of_ultimate_power_3" : 
         ( p -> true_level > 50 ) ? "spectral_agility" :
         ( p -> true_level >= 40 ) ? "unbridled_fury" :
         "disabled";
}

std::string flask( const player_t* p )
{
  return ( p -> true_level > 60 ) ? "iced_phial_of_corrupting_rage_3" : 
         ( p -> true_level > 50 ) ? "spectral_flask_of_power" :
         ( p -> true_level >= 40 ) ? "greater_flask_of_the_currents" :
         "disabled";
}

std::string food( const player_t* p )
{
  return ( p -> true_level >= 70 ) ? "fated_fortune_cookie" : 
         ( p -> true_level >= 60 ) ? "feast_of_gluttonous_hedonism" :
         ( p -> true_level >= 45 ) ? "bountiful_captains_feast" :
         "disabled";
}

std::string rune( const player_t* p )
{
  return ( p -> true_level >= 70 ) ? "draconic" :
         ( p -> true_level >= 60 ) ? "veiled" :
         ( p -> true_level >= 50 ) ? "battle_scarred" :
         "disabled";
}

std::string temporary_enchant( const player_t* p )
{
  std::string lvl70_temp_enchant = ( p -> specialization() == HUNTER_SURVIVAL ) ? "main_hand:howling_rune_3" : "main_hand:completely_safe_rockets_3";
  std::string lvl60_temp_enchant = ( p -> specialization() == HUNTER_SURVIVAL ) ? "main_hand:shaded_sharpening_stone" : "main_hand:shadowcore_oil";

  return ( p -> true_level >= 70 ) ? lvl70_temp_enchant :
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

  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "summon_pet" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=trinket_1_stronger,value=!trinket.2.has_cooldown|trinket.1.has_use_buff&(!trinket.2.has_use_buff|!trinket.1.is.mirror_of_fractured_tomorrows&(trinket.2.is.mirror_of_fractured_tomorrows|trinket.2.cooldown.duration<trinket.1.cooldown.duration|trinket.2.cast_time<trinket.1.cast_time|trinket.2.cast_time=trinket.1.cast_time&trinket.2.cooldown.duration=trinket.1.cooldown.duration))|!trinket.1.has_use_buff&(!trinket.2.has_use_buff&(trinket.2.cooldown.duration<trinket.1.cooldown.duration|trinket.2.cast_time<trinket.1.cast_time|trinket.2.cast_time=trinket.1.cast_time&trinket.2.cooldown.duration=trinket.1.cooldown.duration))", "Determine the stronger trinket to sync with cooldowns. In descending priority: buff effects > damage effects, longer > shorter cooldowns, longer > shorter cast times. Special case to consider Mirror of Fractured Tomorrows weaker than other buff effects since its power is split between the dmg effect and the buff effect." );
  precombat->add_action( "variable,name=trinket_2_stronger,value=!variable.trinket_1_stronger" );

  default_->add_action( "auto_shot" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "call_action_list,name=st,if=active_enemies<2|!talent.beast_cleave&active_enemies<3" );
  default_->add_action( "call_action_list,name=cleave,if=active_enemies>2|talent.beast_cleave&active_enemies>1" );

  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&(buff.bestial_wrath.up|cooldown.bestial_wrath.remains<30)|fight_remains<16" );
  cds->add_action( "berserking,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<13" );
  cds->add_action( "blood_fury,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<16" );
  cds->add_action( "ancestral_call,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<16" );
  cds->add_action( "fireblood,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<9" );
  cds->add_action( "potion,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<31" );

  cleave->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=pet.main.buff.frenzy.up&pet.main.buff.frenzy.remains<=gcd+0.25|talent.scent_of_blood&cooldown.bestial_wrath.remains<12+gcd|pet.main.buff.frenzy.stack<3&(cooldown.bestial_wrath.ready|cooldown.call_of_the_wild.ready)|full_recharge_time<gcd&cooldown.bestial_wrath.remains" );
  cleave->add_action( "multishot,if=pet.main.buff.beast_cleave.remains<0.25+gcd&(!talent.bloody_frenzy|cooldown.call_of_the_wild.remains)" );
  cleave->add_action( "bestial_wrath" );
  cleave->add_action( "call_of_the_wild" );
  cleave->add_action( "kill_command,if=talent.kill_cleave" );
  cleave->add_action( "explosive_shot" );
  cleave->add_action( "bloodshed" );
  cleave->add_action( "kill_shot,target_if=min:dot.sepent_sting.remains,if=talent.venoms_bite&dot.serpent_sting.remains<gcd&target.time_to_die>10" );
  cleave->add_action( "dire_beast" );
  cleave->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=buff.call_of_the_wild.up|fight_remains<9|talent.wild_call&charges_fractional>1.2|talent.savagery" );
  cleave->add_action( "kill_command" );
  cleave->add_action( "multishot,if=pet.main.buff.beast_cleave.remains<gcd*2" );
  cleave->add_action( "lights_judgment,if=buff.bestial_wrath.down|target.time_to_die<5" );
  cleave->add_action( "kill_shot" );
  cleave->add_action( "cobra_shot,if=focus.time_to_max<gcd*2" );
  cleave->add_action( "bag_of_tricks,if=buff.bestial_wrath.down|target.time_to_die<5" );
  cleave->add_action( "arcane_torrent,if=(focus+focus.regen+30)<focus.max" );

  st->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=pet.main.buff.frenzy.up&pet.main.buff.frenzy.remains<=gcd+0.25|pet.main.buff.frenzy.stack<3&(talent.scent_of_blood&(cooldown.bestial_wrath.ready|cooldown.call_of_the_wild.ready)|!cooldown.bestial_wrath.ready)" );
  st->add_action( "bestial_wrath" );
  st->add_action( "kill_command,if=(full_recharge_time<gcd&talent.alpha_predator)|talent.call_of_the_wild" );
  st->add_action( "dire_beast,if=talent.huntmasters_call&(!buff.bestial_wrath.up&talent.killer_cobra|cooldown.call_of_the_wild.ready)" );
  st->add_action( "kill_shot,target_if=min:dot.serpent_sting.remains,if=talent.venoms_bite&dot.serpent_sting.refreshable" );
  st->add_action( "call_of_the_wild" );
  st->add_action( "bloodshed" );
  st->add_action( "kill_command" );
  st->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=talent.wild_call&charges_fractional>1.4|buff.call_of_the_wild.up|full_recharge_time<gcd&cooldown.bestial_wrath.remains|talent.scent_of_blood&(cooldown.bestial_wrath.remains<12+gcd)|talent.savagery|fight_remains<9" );
  st->add_action( "cobra_shot,if=buff.bestial_wrath.up&talent.killer_cobra" );
  st->add_action( "dire_beast" );
  st->add_action( "explosive_shot,if=!buff.bestial_wrath.up&talent.killer_cobra|!talent.killer_cobra" );
  st->add_action( "kill_shot,if=buff.hunters_prey.remains<gcd*2&talent.venoms_bite|target.health.pct<20" );
  st->add_action( "lights_judgment,if=buff.bestial_wrath.down|target.time_to_die<5" );
  st->add_action( "cobra_shot" );
  st->add_action( "bag_of_tricks,if=buff.bestial_wrath.down|target.time_to_die<5" );
  st->add_action( "arcane_pulse,if=buff.bestial_wrath.down|target.time_to_die<5" );
  st->add_action( "arcane_torrent,if=(focus+focus.regen+15)<focus.max" );

  trinkets->add_action( "variable,name=sync_ready,value=talent.call_of_the_wild&(prev_gcd.1.call_of_the_wild)|!talent.call_of_the_wild&(buff.bestial_wrath.up|cooldown.bestial_wrath.remains_guess<5)", "True if effects that are desirable to sync a trinket buff with are ready." );
  trinkets->add_action( "variable,name=sync_active,value=talent.call_of_the_wild&buff.call_of_the_wild.up|!talent.call_of_the_wild&buff.bestial_wrath.up", "True if effecs that are desirable to sync a trinket buff with are active." );
  trinkets->add_action( "variable,name=sync_remains,op=setif,value=cooldown.bestial_wrath.remains_guess,value_else=cooldown.call_of_the_wild.remains,condition=!talent.call_of_the_wild", "Time until the effects that are desirable to sync a trinket buff with will be ready." );
  trinkets->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=trinket.1.has_use_buff&(variable.sync_ready&(variable.trinket_1_stronger|trinket.2.cooldown.remains)|!variable.sync_ready&(variable.trinket_1_stronger&(variable.sync_remains>trinket.1.cooldown.duration%3&fight_remains>trinket.1.cooldown.duration+20|trinket.2.has_use_buff&trinket.2.cooldown.remains>variable.sync_remains-15&trinket.2.cooldown.remains-5<variable.sync_remains&variable.sync_remains+45>fight_remains)|variable.trinket_2_stronger&(trinket.2.cooldown.remains&(trinket.2.cooldown.remains-5<variable.sync_remains&variable.sync_remains>=20|trinket.2.cooldown.remains-5>=variable.sync_remains&(variable.sync_remains>trinket.1.cooldown.duration%3|trinket.1.cooldown.duration<fight_remains&(variable.sync_remains+trinket.1.cooldown.duration>fight_remains)))|trinket.2.cooldown.ready&variable.sync_remains>20&variable.sync_remains<trinket.2.cooldown.duration%3)))|!trinket.1.has_use_buff&(trinket.1.cast_time=0|!variable.sync_active)&(!trinket.2.has_use_buff&(variable.trinket_1_stronger|trinket.2.cooldown.remains)|trinket.2.has_use_buff&(!variable.sync_active&variable.sync_remains>20|trinket.2.cooldown.remains>20))|fight_remains<25&(variable.trinket_1_stronger|trinket.2.cooldown.remains)", "Uses buff effect trinkets with cooldowns and is willing to delay usage up to half the trinket cooldown if it won't lose a usage in the fight. Fills in downtime with weaker buff effects if they won't also be saved for later cooldowns (happens if it won't delay over half the trinket cooldown and a stronger trinket won't be up in time) or damage effects if they won't inferfere with any buff effect usage. Intended to be slot-agnostic so that any order of the same trinket pair should result in the same usage." );
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

  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "summon_pet" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=trinket_1_stronger,value=!trinket.2.has_cooldown|trinket.1.has_use_buff&(!trinket.2.has_use_buff|!trinket.1.is.mirror_of_fractured_tomorrows&(trinket.2.is.mirror_of_fractured_tomorrows|trinket.2.cooldown.duration<trinket.1.cooldown.duration|trinket.2.cast_time<trinket.1.cast_time|trinket.2.cast_time=trinket.1.cast_time&trinket.2.cooldown.duration=trinket.1.cooldown.duration))|!trinket.1.has_use_buff&(!trinket.2.has_use_buff&(trinket.2.cooldown.duration<trinket.1.cooldown.duration|trinket.2.cast_time<trinket.1.cast_time|trinket.2.cast_time=trinket.1.cast_time&trinket.2.cooldown.duration=trinket.1.cooldown.duration))", "Determine the stronger trinket to sync with cooldowns. In descending priority: buff effects > damage effects, longer > shorter cooldowns, longer > shorter cast times. Special case to consider Mirror of Fractured Tomorrows weaker than other buff effects since its power is split between the dmg effect and the buff effect." );
  precombat->add_action( "variable,name=trinket_2_stronger,value=!variable.trinket_1_stronger" );

  default_->add_action( "auto_shot" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "call_action_list,name=st,if=active_enemies<2|!talent.beast_cleave&active_enemies<3" );
  default_->add_action( "call_action_list,name=cleave,if=active_enemies>2|talent.beast_cleave&active_enemies>1" );

  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&(buff.bestial_wrath.up|cooldown.bestial_wrath.remains<30)|fight_remains<16" );
  cds->add_action( "berserking,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<13" );
  cds->add_action( "blood_fury,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<16" );
  cds->add_action( "ancestral_call,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<16" );
  cds->add_action( "fireblood,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<9" );
  cds->add_action( "potion,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<31" );

  cleave->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=pet.main.buff.frenzy.up&pet.main.buff.frenzy.remains<=gcd+0.25|talent.scent_of_blood&cooldown.bestial_wrath.remains<12+gcd|pet.main.buff.frenzy.stack<3&(cooldown.bestial_wrath.ready|cooldown.call_of_the_wild.ready)|full_recharge_time<gcd&cooldown.bestial_wrath.remains" );
  cleave->add_action( "multishot,if=pet.main.buff.beast_cleave.remains<0.25+gcd&(!talent.bloody_frenzy|cooldown.call_of_the_wild.remains)" );
  cleave->add_action( "bestial_wrath" );
  cleave->add_action( "call_of_the_wild" );
  cleave->add_action( "kill_command,if=talent.kill_cleave" );
  cleave->add_action( "explosive_shot" );
  cleave->add_action( "bloodshed" );
  cleave->add_action( "kill_shot,target_if=min:dot.sepent_sting.remains,if=talent.venoms_bite&dot.serpent_sting.remains<gcd&target.time_to_die>10" );
  cleave->add_action( "dire_beast" );
  cleave->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=buff.call_of_the_wild.up|fight_remains<9|talent.wild_call&charges_fractional>1.2|talent.savagery" );
  cleave->add_action( "kill_command" );
  cleave->add_action( "multishot,if=pet.main.buff.beast_cleave.remains<gcd*2" );
  cleave->add_action( "lights_judgment,if=buff.bestial_wrath.down|target.time_to_die<5" );
  cleave->add_action( "kill_shot" );
  cleave->add_action( "cobra_shot,if=focus.time_to_max<gcd*2" );
  cleave->add_action( "bag_of_tricks,if=buff.bestial_wrath.down|target.time_to_die<5" );
  cleave->add_action( "arcane_torrent,if=(focus+focus.regen+30)<focus.max" );

  st->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=pet.main.buff.frenzy.up&pet.main.buff.frenzy.remains<=gcd+0.25|pet.main.buff.frenzy.stack<3&(talent.scent_of_blood&(cooldown.bestial_wrath.ready|cooldown.call_of_the_wild.ready)|!cooldown.bestial_wrath.ready)" );
  st->add_action( "bestial_wrath" );
  st->add_action( "kill_command,if=(full_recharge_time<gcd&talent.alpha_predator)|talent.call_of_the_wild" );
  st->add_action( "dire_beast,if=talent.huntmasters_call&(!buff.bestial_wrath.up&talent.killer_cobra|cooldown.call_of_the_wild.ready)" );
  st->add_action( "kill_shot,target_if=min:dot.serpent_sting.remains,if=talent.venoms_bite&dot.serpent_sting.refreshable" );
  st->add_action( "call_of_the_wild" );
  st->add_action( "bloodshed" );
  st->add_action( "kill_command" );
  st->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=talent.wild_call&charges_fractional>1.4|buff.call_of_the_wild.up|full_recharge_time<gcd&cooldown.bestial_wrath.remains|talent.scent_of_blood&(cooldown.bestial_wrath.remains<12+gcd)|talent.savagery|fight_remains<9" );
  st->add_action( "cobra_shot,if=buff.bestial_wrath.up&talent.killer_cobra" );
  st->add_action( "dire_beast" );
  st->add_action( "explosive_shot,if=!buff.bestial_wrath.up&talent.killer_cobra|!talent.killer_cobra" );
  st->add_action( "kill_shot,if=buff.hunters_prey.remains<gcd*2&talent.venoms_bite|target.health.pct<20" );
  st->add_action( "lights_judgment,if=buff.bestial_wrath.down|target.time_to_die<5" );
  st->add_action( "cobra_shot" );
  st->add_action( "bag_of_tricks,if=buff.bestial_wrath.down|target.time_to_die<5" );
  st->add_action( "arcane_pulse,if=buff.bestial_wrath.down|target.time_to_die<5" );
  st->add_action( "arcane_torrent,if=(focus+focus.regen+15)<focus.max" );

  trinkets->add_action( "variable,name=sync_ready,value=talent.call_of_the_wild&(prev_gcd.1.call_of_the_wild)|!talent.call_of_the_wild&(buff.bestial_wrath.up|cooldown.bestial_wrath.remains_guess<5)", "True if effects that are desirable to sync a trinket buff with are ready." );
  trinkets->add_action( "variable,name=sync_active,value=talent.call_of_the_wild&buff.call_of_the_wild.up|!talent.call_of_the_wild&buff.bestial_wrath.up", "True if effecs that are desirable to sync a trinket buff with are active." );
  trinkets->add_action( "variable,name=sync_remains,op=setif,value=cooldown.bestial_wrath.remains_guess,value_else=cooldown.call_of_the_wild.remains,condition=!talent.call_of_the_wild", "Time until the effects that are desirable to sync a trinket buff with will be ready." );
  trinkets->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=trinket.1.has_use_buff&(variable.sync_ready&(variable.trinket_1_stronger|trinket.2.cooldown.remains)|!variable.sync_ready&(variable.trinket_1_stronger&(variable.sync_remains>trinket.1.cooldown.duration%3&fight_remains>trinket.1.cooldown.duration+20|trinket.2.has_use_buff&trinket.2.cooldown.remains>variable.sync_remains-15&trinket.2.cooldown.remains-5<variable.sync_remains&variable.sync_remains+45>fight_remains)|variable.trinket_2_stronger&(trinket.2.cooldown.remains&(trinket.2.cooldown.remains-5<variable.sync_remains&variable.sync_remains>=20|trinket.2.cooldown.remains-5>=variable.sync_remains&(variable.sync_remains>trinket.1.cooldown.duration%3|trinket.1.cooldown.duration<fight_remains&(variable.sync_remains+trinket.1.cooldown.duration>fight_remains)))|trinket.2.cooldown.ready&variable.sync_remains>20&variable.sync_remains<trinket.2.cooldown.duration%3)))|!trinket.1.has_use_buff&(trinket.1.cast_time=0|!variable.sync_active)&(!trinket.2.has_use_buff&(variable.trinket_1_stronger|trinket.2.cooldown.remains)|trinket.2.has_use_buff&(!variable.sync_active&variable.sync_remains>20|trinket.2.cooldown.remains>20))|fight_remains<25&(variable.trinket_1_stronger|trinket.2.cooldown.remains)", "Uses buff effect trinkets with cooldowns and is willing to delay usage up to half the trinket cooldown if it won't lose a usage in the fight. Fills in downtime with weaker buff effects if they won't also be saved for later cooldowns (happens if it won't delay over half the trinket cooldown and a stronger trinket won't be up in time) or damage effects if they won't inferfere with any buff effect usage. Intended to be slot-agnostic so that any order of the same trinket pair should result in the same usage." );
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

  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "summon_pet,if=!talent.lone_wolf" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=trinket_1_stronger,value=!trinket.2.has_cooldown|trinket.1.has_use_buff&(!trinket.2.has_use_buff|!trinket.1.is.mirror_of_fractured_tomorrows&(trinket.2.is.mirror_of_fractured_tomorrows|trinket.2.cooldown.duration<trinket.1.cooldown.duration|trinket.2.cast_time<trinket.1.cast_time|trinket.2.cast_time=trinket.1.cast_time&trinket.2.cooldown.duration=trinket.1.cooldown.duration))|!trinket.1.has_use_buff&(!trinket.2.has_use_buff&(trinket.2.cooldown.duration<trinket.1.cooldown.duration|trinket.2.cast_time<trinket.1.cast_time|trinket.2.cast_time=trinket.1.cast_time&trinket.2.cooldown.duration=trinket.1.cooldown.duration))", "Determine the stronger trinket to sync with cooldowns. In descending priority: buff effects > damage effects, longer > shorter cooldowns, longer > shorter cast times. Special case to consider Mirror of Fractured Tomorrows weaker than other buff effects since its power is split between the dmg effect and the buff effect." );
  precombat->add_action( "variable,name=trinket_2_stronger,value=!variable.trinket_1_stronger" );
  precombat->add_action( "salvo,precast_time=10" );
  precombat->add_action( "use_item,name=algethar_puzzle_box" );
  precombat->add_action( "aimed_shot,if=active_enemies<3&(!talent.volley|active_enemies<2)", "Precast Aimed Shot on one or two targets unless we could cleave it with Volley on two targets." );
  precombat->add_action( "steady_shot,if=active_enemies>2|talent.volley&active_enemies=2", "Precast Steady Shot on two targets if we are saving Aimed Shot to cleave with Volley, otherwise on three or more targets." );

  default_->add_action( "variable,name=trueshot_ready,value=cooldown.trueshot.ready&(!raid_event.adds.exists&(!talent.bullseye|fight_remains>cooldown.trueshot.duration_guess+buff.trueshot.duration%2|buff.bullseye.stack=buff.bullseye.max_stack)&(!trinket.1.has_use_buff|trinket.1.cooldown.remains>30|trinket.1.cooldown.ready)&(!trinket.2.has_use_buff|trinket.2.cooldown.remains>30|trinket.2.cooldown.ready)|raid_event.adds.exists&(!raid_event.adds.up&(raid_event.adds.duration+raid_event.adds.in<25|raid_event.adds.in>60)|raid_event.adds.up&raid_event.adds.remains>10)|fight_remains<25)", "Determine if it is a good time to use Trueshot. Raid event optimization takes priority so usage is saved for multiple targets as long as it won't delay over half its duration. Otherwise allow for small delays to line up buff effect trinkets, and when using Bullseye, delay the last usage of the fight for max stacks." );
  default_->add_action( "auto_shot" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "call_action_list,name=st,if=active_enemies<3|!talent.trick_shots" );
  default_->add_action( "call_action_list,name=trickshots,if=active_enemies>2" );

  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.trueshot.remains>12", "Call for Power Infusion when Trueshot is up." );
  cds->add_action( "berserking,if=buff.trueshot.up|fight_remains<13" );
  cds->add_action( "blood_fury,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<16" );
  cds->add_action( "ancestral_call,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<16" );
  cds->add_action( "fireblood,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<9" );
  cds->add_action( "lights_judgment,if=buff.trueshot.down" );
  cds->add_action( "potion,if=buff.trueshot.up&(buff.bloodlust.up|target.health.pct<20)|fight_remains<26" );
  cds->add_action( "salvo,if=active_enemies>2|cooldown.volley.remains<10" );

  st->add_action( "steady_shot,if=talent.steady_focus&steady_focus_count&buff.steady_focus.remains<8" );
  st->add_action( "kill_shot,if=buff.razor_fragments.up" );
  st->add_action( "rapid_fire,if=talent.surging_shots|action.aimed_shot.full_recharge_time>action.aimed_shot.cast_time+cast_time" );
  st->add_action( "volley,if=buff.salvo.up|variable.trueshot_ready|cooldown.trueshot.remains>45|fight_remains<12" );
  st->add_action( "explosive_shot,if=active_enemies>1" );
  st->add_action( "trueshot,if=variable.trueshot_ready" );
  st->add_action( "multishot,if=buff.salvo.up&!talent.volley", "Trigger Salvo if Volley isn't being used to trigger it." );
  st->add_action( "wailing_arrow,if=buff.precise_shots.down|buff.trueshot.up|active_enemies>1", "Don't overwrite Precise Shots unless Trueshot is active or it can cleave." );
  st->add_action( "aimed_shot,target_if=min:dot.serpent_sting.remains+action.serpent_sting.in_flight_to_target*99,if=buff.precise_shots.down|(buff.trueshot.up|full_recharge_time<gcd+cast_time)|(buff.trick_shots.remains>execute_time&active_enemies>1)", "With Serpentstalker's Trickery target the lowest remaining Serpent Sting. Don't overwrite Precise Shots unless either Trueshot is active or Aimed Shot would cap before its next cast. Overwrite freely if it can cleave." );
  st->add_action( "kill_shot" );
  st->add_action( "explosive_shot" );
  st->add_action( "chimaera_shot,if=buff.precise_shots.up" );
  st->add_action( "arcane_shot,if=buff.precise_shots.up" );
  st->add_action( "barrage,if=talent.rapid_fire_barrage" );
  st->add_action( "arcane_shot,if=focus>cost+action.aimed_shot.cost" );
  st->add_action( "bag_of_tricks,if=buff.trueshot.down" );
  st->add_action( "steady_shot" );

  trickshots->add_action( "steady_shot,if=talent.steady_focus&steady_focus_count&buff.steady_focus.remains<8" );
  trickshots->add_action( "kill_shot,if=buff.razor_fragments.up" );
  trickshots->add_action( "explosive_shot" );
  trickshots->add_action( "volley" );
  trickshots->add_action( "barrage,if=talent.rapid_fire_barrage" );
  trickshots->add_action( "rapid_fire,if=buff.trick_shots.remains>=execute_time&talent.surging_shots" );
  trickshots->add_action( "wailing_arrow,if=buff.precise_shots.down|buff.trueshot.up" );
  trickshots->add_action( "trueshot,if=variable.trueshot_ready" );
  trickshots->add_action( "aimed_shot,target_if=min:dot.serpent_sting.remains+action.serpent_sting.in_flight_to_target*99,if=(buff.trick_shots.remains>=execute_time&(buff.precise_shots.down|buff.trueshot.up|full_recharge_time<cast_time+gcd))", "For Serpentstalker's Trickery, target the lowest remaining Serpent Sting. Generally only cast if it would cleave with Trick Shots. Don't overwrite Precise Shots unless Trueshot is up or Aimed Shot would cap otherwise." );
  trickshots->add_action( "rapid_fire,if=buff.trick_shots.remains>=execute_time" );
  trickshots->add_action( "chimaera_shot,if=buff.trick_shots.up&buff.precise_shots.up&focus>cost+action.aimed_shot.cost&active_enemies<4" );
  trickshots->add_action( "multishot,if=buff.trick_shots.down|(buff.precise_shots.up|buff.bulletstorm.stack=10)&focus>cost+action.aimed_shot.cost" );
  trickshots->add_action( "kill_shot,if=focus>cost+action.aimed_shot.cost" );
  trickshots->add_action( "multishot,if=focus>cost+action.aimed_shot.cost" );
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

  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "summon_pet,if=!talent.lone_wolf" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=trinket_1_stronger,value=!trinket.2.has_cooldown|trinket.1.has_use_buff&(!trinket.2.has_use_buff|!trinket.1.is.mirror_of_fractured_tomorrows&(trinket.2.is.mirror_of_fractured_tomorrows|trinket.2.cooldown.duration<trinket.1.cooldown.duration|trinket.2.cast_time<trinket.1.cast_time|trinket.2.cast_time=trinket.1.cast_time&trinket.2.cooldown.duration=trinket.1.cooldown.duration))|!trinket.1.has_use_buff&(!trinket.2.has_use_buff&(trinket.2.cooldown.duration<trinket.1.cooldown.duration|trinket.2.cast_time<trinket.1.cast_time|trinket.2.cast_time=trinket.1.cast_time&trinket.2.cooldown.duration=trinket.1.cooldown.duration))", "Determine the stronger trinket to sync with cooldowns. In descending priority: buff effects > damage effects, longer > shorter cooldowns, longer > shorter cast times. Special case to consider Mirror of Fractured Tomorrows weaker than other buff effects since its power is split between the dmg effect and the buff effect." );
  precombat->add_action( "variable,name=trinket_2_stronger,value=!variable.trinket_1_stronger" );
  precombat->add_action( "salvo,precast_time=10" );
  precombat->add_action( "use_item,name=algethar_puzzle_box" );
  precombat->add_action( "aimed_shot,if=active_enemies<3&(!talent.volley|active_enemies<2)", "Precast Aimed Shot on one or two targets unless we could cleave it with Volley on two targets." );
  precombat->add_action( "steady_shot,if=active_enemies>2|talent.volley&active_enemies=2", "Precast Steady Shot on two targets if we are saving Aimed Shot to cleave with Volley, otherwise on three or more targets." );

  default_->add_action( "variable,name=trueshot_ready,value=cooldown.trueshot.ready&(!raid_event.adds.exists&(!talent.bullseye|fight_remains>cooldown.trueshot.duration_guess+buff.trueshot.duration%2|buff.bullseye.stack=buff.bullseye.max_stack)&(!trinket.1.has_use_buff|trinket.1.cooldown.remains>30|trinket.1.cooldown.ready)&(!trinket.2.has_use_buff|trinket.2.cooldown.remains>30|trinket.2.cooldown.ready)|raid_event.adds.exists&(!raid_event.adds.up&(raid_event.adds.duration+raid_event.adds.in<25|raid_event.adds.in>60)|raid_event.adds.up&raid_event.adds.remains>10)|fight_remains<25)", "Determine if it is a good time to use Trueshot. Raid event optimization takes priority so usage is saved for multiple targets as long as it won't delay over half its duration. Otherwise allow for small delays to line up buff effect trinkets, and when using Bullseye, delay the last usage of the fight for max stacks." );
  default_->add_action( "auto_shot" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "call_action_list,name=st,if=active_enemies<3|!talent.trick_shots" );
  default_->add_action( "call_action_list,name=trickshots,if=active_enemies>2" );

  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.trueshot.remains>12", "Call for Power Infusion when Trueshot is up." );
  cds->add_action( "berserking,if=buff.trueshot.up|fight_remains<13" );
  cds->add_action( "blood_fury,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<16" );
  cds->add_action( "ancestral_call,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<16" );
  cds->add_action( "fireblood,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<9" );
  cds->add_action( "lights_judgment,if=buff.trueshot.down" );
  cds->add_action( "potion,if=buff.trueshot.up&(buff.bloodlust.up|target.health.pct<20)|fight_remains<26" );
  cds->add_action( "salvo,if=active_enemies>2|cooldown.volley.remains<10" );

  st->add_action( "steady_shot,if=talent.steady_focus&steady_focus_count&buff.steady_focus.remains<8" );
  st->add_action( "kill_shot,if=buff.razor_fragments.up" );
  st->add_action( "rapid_fire,if=talent.surging_shots|action.aimed_shot.full_recharge_time>action.aimed_shot.cast_time+cast_time" );
  st->add_action( "volley,if=buff.salvo.up|variable.trueshot_ready|cooldown.trueshot.remains>45|fight_remains<12" );
  st->add_action( "explosive_shot,if=active_enemies>1" );
  st->add_action( "trueshot,if=variable.trueshot_ready" );
  st->add_action( "multishot,if=buff.salvo.up&!talent.volley", "Trigger Salvo if Volley isn't being used to trigger it." );
  st->add_action( "wailing_arrow,if=buff.precise_shots.down|buff.trueshot.up|active_enemies>1", "Don't overwrite Precise Shots unless Trueshot is active or it can cleave." );
  st->add_action( "aimed_shot,target_if=min:dot.serpent_sting.remains+action.serpent_sting.in_flight_to_target*99,if=buff.precise_shots.down|(buff.trueshot.up|full_recharge_time<gcd+cast_time)|(buff.trick_shots.remains>execute_time&active_enemies>1)", "With Serpentstalker's Trickery target the lowest remaining Serpent Sting. Don't overwrite Precise Shots unless either Trueshot is active or Aimed Shot would cap before its next cast. Overwrite freely if it can cleave." );
  st->add_action( "kill_shot" );
  st->add_action( "explosive_shot" );
  st->add_action( "chimaera_shot,if=buff.precise_shots.up" );
  st->add_action( "arcane_shot,if=buff.precise_shots.up" );
  st->add_action( "barrage,if=talent.rapid_fire_barrage" );
  st->add_action( "arcane_shot,if=focus>cost+action.aimed_shot.cost" );
  st->add_action( "bag_of_tricks,if=buff.trueshot.down" );
  st->add_action( "steady_shot" );

  trickshots->add_action( "steady_shot,if=talent.steady_focus&steady_focus_count&buff.steady_focus.remains<8" );
  trickshots->add_action( "kill_shot,if=buff.razor_fragments.up" );
  trickshots->add_action( "explosive_shot" );
  trickshots->add_action( "volley" );
  trickshots->add_action( "barrage,if=talent.rapid_fire_barrage" );
  trickshots->add_action( "rapid_fire,if=buff.trick_shots.remains>=execute_time&talent.surging_shots" );
  trickshots->add_action( "wailing_arrow,if=buff.precise_shots.down|buff.trueshot.up" );
  trickshots->add_action( "trueshot,if=variable.trueshot_ready" );
  trickshots->add_action( "aimed_shot,target_if=min:dot.serpent_sting.remains+action.serpent_sting.in_flight_to_target*99,if=(buff.trick_shots.remains>=execute_time&(buff.precise_shots.down|buff.trueshot.up|full_recharge_time<cast_time+gcd))", "For Serpentstalker's Trickery, target the lowest remaining Serpent Sting. Generally only cast if it would cleave with Trick Shots. Don't overwrite Precise Shots unless Trueshot is up or Aimed Shot would cap otherwise." );
  trickshots->add_action( "rapid_fire,if=buff.trick_shots.remains>=execute_time" );
  trickshots->add_action( "chimaera_shot,if=buff.trick_shots.up&buff.precise_shots.up&focus>cost+action.aimed_shot.cost&active_enemies<4" );
  trickshots->add_action( "multishot,if=buff.trick_shots.down|(buff.precise_shots.up|buff.bulletstorm.stack=10)&focus>cost+action.aimed_shot.cost" );
  trickshots->add_action( "kill_shot,if=focus>cost+action.aimed_shot.cost" );
  trickshots->add_action( "multishot,if=focus>cost+action.aimed_shot.cost" );
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
  action_priority_list_t* cleave = p->get_action_priority_list( "cleave" );
  action_priority_list_t* st = p->get_action_priority_list( "st" );

  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "summon_pet" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "use_item,name=algethar_puzzle_box" );

  default_->add_action( "auto_attack" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=cleave,if=active_enemies>2" );
  default_->add_action( "call_action_list,name=st,if=active_enemies<3" );
  default_->add_action( "arcane_torrent" );
  default_->add_action( "bag_of_tricks" );

  cds->add_action( "blood_fury,if=buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "harpoon,if=prev.kill_command" );
  cds->add_action( "ancestral_call,if=buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "fireblood,if=buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "lights_judgment" );
  cds->add_action( "berserking,if=buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault|time_to_die<13" );
  cds->add_action( "muzzle" );
  cds->add_action( "potion,if=target.time_to_die<25|buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "use_item,name=algethar_puzzle_box,use_off_gcd=1,if=cooldown.coordinated_assault.remains<2*gcd" );
  cds->add_action( "use_item,name=manic_grieftorch,if=cooldown.coordinated_assault.remains<2*gcd" );
  cds->add_action( "use_item,name=beacon_to_the_beyond,if=cooldown.coordinated_assault.remains<2*gcd" );
  cds->add_action( "use_items,if=cooldown.coordinated_assault.remains|cooldown.spearhead.remains" );
  cds->add_action( "aspect_of_the_eagle,if=target.distance>=6" );

  cleave->add_action( "spearhead,if=cooldown.coordinated_assault.remains" );
  cleave->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0&cooldown.wildfire_bomb.charges_fractional>1.7|cooldown.wildfire_bomb.charges_fractional>1.9|cooldown.coordinated_assault.remains<2*gcd" );
  cleave->add_action( "fury_of_the_eagle" );
  cleave->add_action( "coordinated_assault,if=!talent.bombardier|talent.bombardier&cooldown.wildfire_bomb.charges_fractional<1" );
  cleave->add_action( "flanking_strike,if=buff.tip_of_the_spear.stack<2" );
  cleave->add_action( "explosive_shot,if=buff.tip_of_the_spear.stack>0" );
  cleave->add_action( "kill_command,target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max" );
  cleave->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0" );
  cleave->add_action( "mongoose_bite,if=buff.merciless_blows.up" );
  cleave->add_action( "raptor_strike,if=buff.merciless_blows.up" );
  cleave->add_action( "butchery" );
  cleave->add_action( "kill_shot" );
  cleave->add_action( "mongoose_bite" );
  cleave->add_action( "raptor_strike" );

  st->add_action( "spearhead,if=cooldown.coordinated_assault.remains" );
  st->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0&cooldown.wildfire_bomb.charges_fractional>1.7|cooldown.wildfire_bomb.charges_fractional>1.9|cooldown.coordinated_assault.remains<2*gcd" );
  st->add_action( "coordinated_assault,if=!talent.bombardier|talent.bombardier&cooldown.wildfire_bomb.charges_fractional<1" );
  st->add_action( "fury_of_the_eagle,interrupt=1,if=set_bonus.tier31_2pc" );
  st->add_action( "flanking_strike,if=buff.tip_of_the_spear.stack<2" );
  st->add_action( "kill_shot,if=buff.tip_of_the_spear.stack>0|talent.sic_em" );
  st->add_action( "explosive_shot,if=buff.tip_of_the_spear.stack>0" );
  st->add_action( "kill_command,target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max" );
  st->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0" );
  st->add_action( "mongoose_bite" );
  st->add_action( "raptor_strike" );
}
//survival_apl_end

//survival_ptr_apl_start
void survival_ptr( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* cleave = p->get_action_priority_list( "cleave" );
  action_priority_list_t* st = p->get_action_priority_list( "st" );

  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "summon_pet" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "use_item,name=algethar_puzzle_box" );

  default_->add_action( "auto_attack" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=cleave,if=active_enemies>2" );
  default_->add_action( "call_action_list,name=st,if=active_enemies<3" );
  default_->add_action( "arcane_torrent" );
  default_->add_action( "bag_of_tricks" );

  cds->add_action( "blood_fury,if=buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "harpoon,if=prev.kill_command" );
  cds->add_action( "ancestral_call,if=buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "fireblood,if=buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "lights_judgment" );
  cds->add_action( "berserking,if=buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault|time_to_die<13" );
  cds->add_action( "muzzle" );
  cds->add_action( "potion,if=target.time_to_die<25|buff.coordinated_assault.up|!talent.coordinated_assault&cooldown.spearhead.remains|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "use_item,name=algethar_puzzle_box,use_off_gcd=1,if=cooldown.coordinated_assault.remains<2*gcd" );
  cds->add_action( "use_item,name=manic_grieftorch,if=cooldown.coordinated_assault.remains<2*gcd" );
  cds->add_action( "use_item,name=beacon_to_the_beyond,if=cooldown.coordinated_assault.remains<2*gcd" );
  cds->add_action( "use_items,if=cooldown.coordinated_assault.remains|cooldown.spearhead.remains" );
  cds->add_action( "aspect_of_the_eagle,if=target.distance>=6" );

  cleave->add_action( "spearhead,if=cooldown.coordinated_assault.remains" );
  cleave->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0&cooldown.wildfire_bomb.charges_fractional>1.7|cooldown.wildfire_bomb.charges_fractional>1.9|cooldown.coordinated_assault.remains<2*gcd" );
  cleave->add_action( "fury_of_the_eagle" );
  cleave->add_action( "coordinated_assault,if=!talent.bombardier|talent.bombardier&cooldown.wildfire_bomb.charges_fractional<1" );
  cleave->add_action( "flanking_strike,if=buff.tip_of_the_spear.stack<2" );
  cleave->add_action( "explosive_shot,if=buff.tip_of_the_spear.stack>0" );
  cleave->add_action( "kill_command,target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max" );
  cleave->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0" );
  cleave->add_action( "mongoose_bite,if=buff.merciless_blows.up" );
  cleave->add_action( "raptor_strike,if=buff.merciless_blows.up" );
  cleave->add_action( "butchery" );
  cleave->add_action( "kill_shot" );
  cleave->add_action( "mongoose_bite" );
  cleave->add_action( "raptor_strike" );

  st->add_action( "spearhead,if=cooldown.coordinated_assault.remains" );
  st->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0&cooldown.wildfire_bomb.charges_fractional>1.7|cooldown.wildfire_bomb.charges_fractional>1.9|cooldown.coordinated_assault.remains<2*gcd" );
  st->add_action( "coordinated_assault,if=!talent.bombardier|talent.bombardier&cooldown.wildfire_bomb.charges_fractional<1" );
  st->add_action( "fury_of_the_eagle,interrupt=1,if=set_bonus.tier31_2pc" );
  st->add_action( "flanking_strike,if=buff.tip_of_the_spear.stack<2" );
  st->add_action( "kill_shot,if=buff.tip_of_the_spear.stack>0|talent.sic_em" );
  st->add_action( "explosive_shot,if=buff.tip_of_the_spear.stack>0" );
  st->add_action( "kill_command,target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max" );
  st->add_action( "wildfire_bomb,if=buff.tip_of_the_spear.stack>0" );
  st->add_action( "mongoose_bite" );
  st->add_action( "raptor_strike" );
}
//survival_ptr_apl_end

} // namespace hunter_apl
