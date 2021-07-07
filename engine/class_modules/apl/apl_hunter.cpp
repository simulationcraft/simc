#include "simulationcraft.hpp"
#include "class_modules/apl/apl_hunter.hpp"

namespace hunter_apl {

std::string potion( const player_t* p )
{
  return ( p -> true_level >  50 ) ? "spectral_agility" :
         ( p -> true_level >= 40 ) ? "unbridled_fury" :
         "disabled";
}

std::string flask( const player_t* p )
{
  return ( p -> true_level >= 51 ) ? "spectral_flask_of_power" :
         ( p -> true_level >= 40 ) ? "greater_flask_of_the_currents" :
         "disabled";
}

std::string food( const player_t* p )
{
  return ( p -> true_level >= 60 ) ? "feast_of_gluttonous_hedonism" :
         ( p -> true_level >= 45 ) ? "bountiful_captains_feast" :
         "disabled";
}

std::string rune( const player_t* p )
{
  return ( p -> true_level >= 60 ) ? "veiled" :
         ( p -> true_level >= 50 ) ? "battle_scarred" :
         "disabled";
}

std::string temporary_enchant( const player_t* p )
{
  std::string lvl60_temp_enchant = ( p -> specialization() == HUNTER_SURVIVAL ) ? "main_hand:shaded_sharpening_stone" : "main_hand:shadowcore_oil";

  return ( p -> true_level >= 60 ) ? lvl60_temp_enchant :
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
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "fleshcraft" );
  precombat->add_action( "tar_trap,precast_time=1.5,if=runeforge.soulforge_embers|runeforge.nessingwarys_trapping_apparatus" );
  precombat->add_action( "bestial_wrath,precast_time=1.5,if=!talent.scent_of_blood&!runeforge.soulforge_embers" );

  default_->add_action( "auto_shot" );
  default_->add_action( "counter_shot,line_cd=30,if=runeforge.sephuzs_proclamation|soulbind.niyas_tools_poison|(conduit.reversal_of_fortune&!runeforge.sephuzs_proclamation)" );
  default_->add_action( "newfound_resolve,if=soulbind.newfound_resolve&(buff.resonating_arrow.up|cooldown.resonating_arrow.remains>10|target.time_to_die<16)", "Delay facing your doubt until you have put Resonating Arrow down, or if the cooldown is too long to delay facing your Doubt. If none of these conditions are able to met within the 10 seconds leeway, the sim faces your Doubt automatically." );
  default_->add_action( "call_action_list,name=trinkets,if=covenant.kyrian&cooldown.aspect_of_the_wild.remains&cooldown.resonating_arrow.remains|!covenant.kyrian&cooldown.aspect_of_the_wild.remains" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=st,if=active_enemies<2" );
  default_->add_action( "call_action_list,name=cleave,if=active_enemies>1" );

  cds->add_action( "ancestral_call,if=cooldown.bestial_wrath.remains>30" );
  cds->add_action( "fireblood,if=cooldown.bestial_wrath.remains>30" );
  cds->add_action( "berserking,if=(buff.wild_spirits.up|!covenant.night_fae&buff.aspect_of_the_wild.up&buff.bestial_wrath.up)&(target.time_to_die>cooldown.berserking.duration+duration|(target.health.pct<35|!talent.killer_instinct))|target.time_to_die<13" );
  cds->add_action( "blood_fury,if=(buff.wild_spirits.up|!covenant.night_fae&buff.aspect_of_the_wild.up&buff.bestial_wrath.up)&(target.time_to_die>cooldown.blood_fury.duration+duration|(target.health.pct<35|!talent.killer_instinct))|target.time_to_die<16" );
  cds->add_action( "lights_judgment" );
  cds->add_action( "potion,if=buff.aspect_of_the_wild.up|target.time_to_die<26" );

  cleave->add_action( "aspect_of_the_wild" );
  cleave->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=pet.main.buff.frenzy.up&pet.main.buff.frenzy.remains<=gcd" );
  cleave->add_action( "multishot,if=gcd-pet.main.buff.beast_cleave.remains>0.25" );
  cleave->add_action( "tar_trap,if=runeforge.soulforge_embers&tar_trap.remains<gcd&cooldown.flare.remains<gcd" );
  cleave->add_action( "flare,if=tar_trap.up&runeforge.soulforge_embers" );
  cleave->add_action( "death_chakram,if=focus+cast_regen<focus.max" );
  cleave->add_action( "wild_spirits" );
  cleave->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=full_recharge_time<gcd&cooldown.bestial_wrath.remains|cooldown.bestial_wrath.remains<12+gcd&talent.scent_of_blood" );
  cleave->add_action( "bestial_wrath" );
  cleave->add_action( "resonating_arrow" );
  cleave->add_action( "stampede,if=buff.aspect_of_the_wild.up|target.time_to_die<15" );
  cleave->add_action( "wailing_arrow" );
  cleave->add_action( "flayed_shot" );
  cleave->add_action( "kill_shot" );
  cleave->add_action( "chimaera_shot" );
  cleave->add_action( "bloodshed" );
  cleave->add_action( "a_murder_of_crows" );
  cleave->add_action( "barrage,if=pet.main.buff.frenzy.remains>execute_time" );
  cleave->add_action( "kill_command,if=focus>cost+action.multishot.cost" );
  cleave->add_action( "bag_of_tricks,if=buff.bestial_wrath.down|target.time_to_die<5" );
  cleave->add_action( "dire_beast" );
  cleave->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=target.time_to_die<9|charges_fractional>1.2&conduit.bloodletting" );
  cleave->add_action( "cobra_shot,if=focus.time_to_max<gcd*2" );
  cleave->add_action( "tar_trap,if=runeforge.soulforge_embers|runeforge.nessingwarys_trapping_apparatus" );
  cleave->add_action( "freezing_trap,if=runeforge.nessingwarys_trapping_apparatus" );
  cleave->add_action( "arcane_torrent,if=(focus+focus.regen+30)<focus.max" );

  st->add_action( "aspect_of_the_wild" );
  st->add_action( "barbed_shot,if=pet.main.buff.frenzy.up&pet.main.buff.frenzy.remains<=gcd" );
  st->add_action( "tar_trap,if=runeforge.soulforge_embers&tar_trap.remains<gcd&cooldown.flare.remains<gcd" );
  st->add_action( "flare,if=tar_trap.up&runeforge.soulforge_embers" );
  st->add_action( "bloodshed" );
  st->add_action( "wild_spirits" );
  st->add_action( "flayed_shot" );
  st->add_action( "kill_shot" );
  st->add_action( "wailing_arrow,if=cooldown.resonating_arrow.remains<gcd&(!talent.explosive_shot|buff.bloodlust.up)|!covenant.kyrian|cooldown.resonating_arrow.remains|target.time_to_die<5" );
  st->add_action( "barbed_shot,if=cooldown.bestial_wrath.remains<12*charges_fractional+gcd&talent.scent_of_blood|full_recharge_time<gcd&cooldown.bestial_wrath.remains|target.time_to_die<9" );
  st->add_action( "death_chakram,if=focus+cast_regen<focus.max" );
  st->add_action( "stampede,if=buff.aspect_of_the_wild.up|target.time_to_die<15" );
  st->add_action( "a_murder_of_crows" );
  st->add_action( "resonating_arrow,if=buff.bestial_wrath.up|target.time_to_die<10" );
  st->add_action( "bestial_wrath,if=cooldown.wild_spirits.remains>15|covenant.kyrian&(cooldown.resonating_arrow.remains<5|cooldown.resonating_arrow.remains>20)|target.time_to_die<15|(!covenant.night_fae&!covenant.kyrian)" );
  st->add_action( "chimaera_shot" );
  st->add_action( "kill_command" );
  st->add_action( "bag_of_tricks,if=buff.bestial_wrath.down|target.time_to_die<5" );
  st->add_action( "dire_beast" );
  st->add_action( "cobra_shot,if=(focus-cost+focus.regen*(cooldown.kill_command.remains-1)>action.kill_command.cost|cooldown.kill_command.remains>1+gcd)|(buff.bestial_wrath.up|buff.nesingwarys_trapping_apparatus.up)&!runeforge.qapla_eredun_war_order|target.time_to_die<3" );
  st->add_action( "barbed_shot,if=buff.wild_spirits.up|charges_fractional>1.2&conduit.bloodletting" );
  st->add_action( "arcane_pulse,if=buff.bestial_wrath.down|target.time_to_die<5" );
  st->add_action( "tar_trap,if=runeforge.soulforge_embers|runeforge.nessingwarys_trapping_apparatus" );
  st->add_action( "freezing_trap,if=runeforge.nessingwarys_trapping_apparatus" );
  st->add_action( "arcane_torrent,if=(focus+focus.regen+15)<focus.max" );

  trinkets->add_action( "variable,name=sync_up,value=buff.resonating_arrow.up|buff.aspect_of_the_wild.up" );
  trinkets->add_action( "variable,name=strong_sync_up,value=covenant.kyrian&buff.resonating_arrow.up&buff.aspect_of_the_wild.up|!covenant.kyrian&buff.aspect_of_the_wild.up" );
  trinkets->add_action( "variable,name=strong_sync_remains,op=setif,condition=covenant.kyrian,value=cooldown.resonating_arrow.remains<?cooldown.aspect_of_the_wild.remains,value_else=cooldown.aspect_of_the_wild.remains,if=buff.aspect_of_the_wild.down" );
  trinkets->add_action( "variable,name=strong_sync_remains,op=setif,condition=covenant.kyrian,value=cooldown.resonating_arrow.remains,value_else=cooldown.aspect_of_the_wild.remains,if=buff.aspect_of_the_wild.up" );
  trinkets->add_action( "variable,name=sync_remains,op=setif,condition=covenant.kyrian,value=cooldown.resonating_arrow.remains>?cooldown.aspect_of_the_wild.remains,value_else=cooldown.aspect_of_the_wild.remains" );
  trinkets->add_action( "use_items,slots=trinket1,if=(trinket.1.has_use_buff|covenant.kyrian&trinket.1.has_cooldown)&(variable.strong_sync_up&(!covenant.kyrian&!trinket.2.has_use_buff|covenant.kyrian&!trinket.2.has_cooldown|trinket.2.cooldown.remains|trinket.1.has_use_buff&(!trinket.2.has_use_buff|trinket.1.cooldown.duration>=trinket.2.cooldown.duration)|trinket.1.has_cooldown&!trinket.2.has_use_buff&trinket.1.cooldown.duration>=trinket.2.cooldown.duration)|!variable.strong_sync_up&(!trinket.2.has_use_buff&(trinket.1.cooldown.duration-5<variable.sync_remains|variable.sync_remains>trinket.1.cooldown.duration%2)|trinket.2.has_use_buff&(trinket.1.has_use_buff&trinket.1.cooldown.duration>=trinket.2.cooldown.duration&(trinket.1.cooldown.duration-5<variable.sync_remains|variable.sync_remains>trinket.1.cooldown.duration%2)|(!trinket.1.has_use_buff|trinket.2.cooldown.duration>=trinket.1.cooldown.duration)&(trinket.2.cooldown.ready&trinket.2.cooldown.duration-5>variable.sync_remains&variable.sync_remains<trinket.2.cooldown.duration%2|!trinket.2.cooldown.ready&(trinket.2.cooldown.remains-5<variable.strong_sync_remains&variable.strong_sync_remains>20&(trinket.1.cooldown.duration-5<variable.sync_remains|trinket.2.cooldown.remains-5<variable.sync_remains&trinket.2.cooldown.duration-10+variable.sync_remains<variable.strong_sync_remains|variable.sync_remains>trinket.1.cooldown.duration%2|variable.sync_up)|trinket.2.cooldown.remains-5>variable.strong_sync_remains&(trinket.1.cooldown.duration-5<variable.strong_sync_remains|!trinket.1.has_use_buff&(variable.sync_remains>trinket.1.cooldown.duration%2|variable.sync_up))))))|target.time_to_die<variable.sync_remains)|!trinket.1.has_use_buff&!covenant.kyrian&(trinket.2.has_use_buff&((!variable.sync_up|trinket.2.cooldown.remains>5)&(variable.sync_remains>20|trinket.2.cooldown.remains-5>variable.sync_remains))|!trinket.2.has_use_buff&(!trinket.2.has_cooldown|trinket.2.cooldown.remains|trinket.2.cooldown.duration>=trinket.1.cooldown.duration))" );
  trinkets->add_action( "use_items,slots=trinket2,if=(trinket.2.has_use_buff|covenant.kyrian&trinket.2.has_cooldown)&(variable.strong_sync_up&(!covenant.kyrian&!trinket.1.has_use_buff|covenant.kyrian&!trinket.1.has_cooldown|trinket.1.cooldown.remains|trinket.2.has_use_buff&(!trinket.1.has_use_buff|trinket.2.cooldown.duration>=trinket.1.cooldown.duration)|trinket.2.has_cooldown&!trinket.1.has_use_buff&trinket.2.cooldown.duration>=trinket.1.cooldown.duration)|!variable.strong_sync_up&(!trinket.1.has_use_buff&(trinket.2.cooldown.duration-5<variable.sync_remains|variable.sync_remains>trinket.2.cooldown.duration%2)|trinket.1.has_use_buff&(trinket.2.has_use_buff&trinket.2.cooldown.duration>=trinket.1.cooldown.duration&(trinket.2.cooldown.duration-5<variable.sync_remains|variable.sync_remains>trinket.2.cooldown.duration%2)|(!trinket.2.has_use_buff|trinket.1.cooldown.duration>=trinket.2.cooldown.duration)&(trinket.1.cooldown.ready&trinket.1.cooldown.duration-5>variable.sync_remains&variable.sync_remains<trinket.1.cooldown.duration%2|!trinket.1.cooldown.ready&(trinket.1.cooldown.remains-5<variable.strong_sync_remains&variable.strong_sync_remains>20&(trinket.2.cooldown.duration-5<variable.sync_remains|trinket.1.cooldown.remains-5<variable.sync_remains&trinket.1.cooldown.duration-10+variable.sync_remains<variable.strong_sync_remains|variable.sync_remains>trinket.2.cooldown.duration%2|variable.sync_up)|trinket.1.cooldown.remains-5>variable.strong_sync_remains&(trinket.2.cooldown.duration-5<variable.strong_sync_remains|!trinket.2.has_use_buff&(variable.sync_remains>trinket.2.cooldown.duration%2|variable.sync_up))))))|target.time_to_die<variable.sync_remains)|!trinket.2.has_use_buff&!covenant.kyrian&(trinket.1.has_use_buff&((!variable.sync_up|trinket.1.cooldown.remains>5)&(variable.sync_remains>20|trinket.1.cooldown.remains-5>variable.sync_remains))|!trinket.1.has_use_buff&(!trinket.1.has_cooldown|trinket.1.cooldown.remains|trinket.1.cooldown.duration>=trinket.2.cooldown.duration))" );
}
//beast_mastery_apl_end

//marksmanship_apl_start
void marksmanship( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* st = p->get_action_priority_list( "st" );
  action_priority_list_t* trickshots = p->get_action_priority_list( "trickshots" );

  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "fleshcraft" );
  precombat->add_action( "tar_trap,if=runeforge.soulforge_embers" );
  precombat->add_action( "double_tap,precast_time=10,if=active_enemies>1|!covenant.kyrian&!talent.volley|variable.etf_precast" );
  precombat->add_action( "variable,name=etf_precast,value=0", "Change to 1 to simulate ETF/SSF gearswap Trueshot precast." );
  precombat->add_action( "trueshot,precast_etf_equip=1,precast_time=2,if=variable.etf_precast" );
  precombat->add_action( "aimed_shot,if=active_enemies<3&(!covenant.kyrian&!talent.volley|active_enemies<2)&!variable.etf_precast" );
  precombat->add_action( "steady_shot,if=active_enemies>2|(covenant.kyrian|talent.volley)&active_enemies=2|variable.etf_precast" );

  default_->add_action( "auto_shot" );
  default_->add_action( "counter_shot,line_cd=30,if=runeforge.sephuzs_proclamation|soulbind.niyas_tools_poison|(conduit.reversal_of_fortune&!runeforge.sephuzs_proclamation)" );
  default_->add_action( "call_action_list,name=trinkets,if=covenant.kyrian&cooldown.trueshot.remains&cooldown.resonating_arrow.remains|!covenant.kyrian&cooldown.trueshot.remains" );
  default_->add_action( "newfound_resolve,if=soulbind.newfound_resolve&(buff.resonating_arrow.up|cooldown.resonating_arrow.remains>10|target.time_to_die<16)", "Delay facing your doubt until you have put Resonating Arrow down, or if the cooldown is too long to delay facing your Doubt. If none of these conditions are able to met within the 10 seconds leeway, the sim faces your Doubt automatically." );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=st,if=active_enemies<3" );
  default_->add_action( "call_action_list,name=trickshots,if=active_enemies>2" );

  trinkets->add_action( "variable,name=sync_up,value=buff.resonating_arrow.up|buff.trueshot.up" );
  trinkets->add_action( "variable,name=strong_sync_up,value=covenant.kyrian&buff.resonating_arrow.up&buff.trueshot.up|!covenant.kyrian&buff.trueshot.up" );
  trinkets->add_action( "variable,name=strong_sync_remains,op=setif,condition=covenant.kyrian,value=cooldown.resonating_arrow.remains<?cooldown.trueshot.remains,value_else=cooldown.trueshot.remains,if=buff.trueshot.down" );
  trinkets->add_action( "variable,name=strong_sync_remains,op=setif,condition=covenant.kyrian,value=cooldown.resonating_arrow.remains,value_else=cooldown.trueshot.remains,if=buff.trueshot.up" );
  trinkets->add_action( "variable,name=sync_remains,op=setif,condition=covenant.kyrian,value=cooldown.resonating_arrow.remains>?cooldown.trueshot.remains,value_else=cooldown.trueshot.remains" );
  trinkets->add_action( "use_items,slots=trinket1,if=(trinket.1.has_use_buff|covenant.kyrian&trinket.1.has_cooldown)&(variable.strong_sync_up&(!covenant.kyrian&!trinket.2.has_use_buff|covenant.kyrian&!trinket.2.has_cooldown|trinket.2.cooldown.remains|trinket.1.has_use_buff&(!trinket.2.has_use_buff|trinket.1.cooldown.duration>=trinket.2.cooldown.duration)|trinket.1.has_cooldown&!trinket.2.has_use_buff&trinket.1.cooldown.duration>=trinket.2.cooldown.duration)|!variable.strong_sync_up&(!trinket.2.has_use_buff&(trinket.1.cooldown.duration-5<variable.sync_remains|variable.sync_remains>trinket.1.cooldown.duration%2)|trinket.2.has_use_buff&(trinket.1.has_use_buff&trinket.1.cooldown.duration>=trinket.2.cooldown.duration&(trinket.1.cooldown.duration-5<variable.sync_remains|variable.sync_remains>trinket.1.cooldown.duration%2)|(!trinket.1.has_use_buff|trinket.2.cooldown.duration>=trinket.1.cooldown.duration)&(trinket.2.cooldown.ready&trinket.2.cooldown.duration-5>variable.sync_remains&variable.sync_remains<trinket.2.cooldown.duration%2|!trinket.2.cooldown.ready&(trinket.2.cooldown.remains-5<variable.strong_sync_remains&variable.strong_sync_remains>20&(trinket.1.cooldown.duration-5<variable.sync_remains|trinket.2.cooldown.remains-5<variable.sync_remains&trinket.2.cooldown.duration-10+variable.sync_remains<variable.strong_sync_remains|variable.sync_remains>trinket.1.cooldown.duration%2|variable.sync_up)|trinket.2.cooldown.remains-5>variable.strong_sync_remains&(trinket.1.cooldown.duration-5<variable.strong_sync_remains|!trinket.1.has_use_buff&(variable.sync_remains>trinket.1.cooldown.duration%2|variable.sync_up))))))|target.time_to_die<variable.sync_remains)|!trinket.1.has_use_buff&!covenant.kyrian&(trinket.2.has_use_buff&((!variable.sync_up|trinket.2.cooldown.remains>5)&(variable.sync_remains>20|trinket.2.cooldown.remains-5>variable.sync_remains))|!trinket.2.has_use_buff&(!trinket.2.has_cooldown|trinket.2.cooldown.remains|trinket.2.cooldown.duration>=trinket.1.cooldown.duration))" );
  trinkets->add_action( "use_items,slots=trinket2,if=(trinket.2.has_use_buff|covenant.kyrian&trinket.2.has_cooldown)&(variable.strong_sync_up&(!covenant.kyrian&!trinket.1.has_use_buff|covenant.kyrian&!trinket.1.has_cooldown|trinket.1.cooldown.remains|trinket.2.has_use_buff&(!trinket.1.has_use_buff|trinket.2.cooldown.duration>=trinket.1.cooldown.duration)|trinket.2.has_cooldown&!trinket.1.has_use_buff&trinket.2.cooldown.duration>=trinket.1.cooldown.duration)|!variable.strong_sync_up&(!trinket.1.has_use_buff&(trinket.2.cooldown.duration-5<variable.sync_remains|variable.sync_remains>trinket.2.cooldown.duration%2)|trinket.1.has_use_buff&(trinket.2.has_use_buff&trinket.2.cooldown.duration>=trinket.1.cooldown.duration&(trinket.2.cooldown.duration-5<variable.sync_remains|variable.sync_remains>trinket.2.cooldown.duration%2)|(!trinket.2.has_use_buff|trinket.1.cooldown.duration>=trinket.2.cooldown.duration)&(trinket.1.cooldown.ready&trinket.1.cooldown.duration-5>variable.sync_remains&variable.sync_remains<trinket.1.cooldown.duration%2|!trinket.1.cooldown.ready&(trinket.1.cooldown.remains-5<variable.strong_sync_remains&variable.strong_sync_remains>20&(trinket.2.cooldown.duration-5<variable.sync_remains|trinket.1.cooldown.remains-5<variable.sync_remains&trinket.1.cooldown.duration-10+variable.sync_remains<variable.strong_sync_remains|variable.sync_remains>trinket.2.cooldown.duration%2|variable.sync_up)|trinket.1.cooldown.remains-5>variable.strong_sync_remains&(trinket.2.cooldown.duration-5<variable.strong_sync_remains|!trinket.2.has_use_buff&(variable.sync_remains>trinket.2.cooldown.duration%2|variable.sync_up))))))|target.time_to_die<variable.sync_remains)|!trinket.2.has_use_buff&!covenant.kyrian&(trinket.1.has_use_buff&((!variable.sync_up|trinket.1.cooldown.remains>5)&(variable.sync_remains>20|trinket.1.cooldown.remains-5>variable.sync_remains))|!trinket.1.has_use_buff&(!trinket.1.has_cooldown|trinket.1.cooldown.remains|trinket.1.cooldown.duration>=trinket.2.cooldown.duration))" );

  cds->add_action( "berserking,if=(buff.trueshot.up&buff.resonating_arrow.up&covenant.kyrian)|(buff.trueshot.up&buff.wild_spirits.up&covenant.night_fae)|(covenant.venthyr|covenant.necrolord)&buff.trueshot.up|target.time_to_die<13|(covenant.kyrian&buff.resonating_arrow.up&target.time_to_die<73)" );
  cds->add_action( "blood_fury,if=buff.trueshot.up|cooldown.trueshot.remains>30|target.time_to_die<16" );
  cds->add_action( "ancestral_call,if=buff.trueshot.up|cooldown.trueshot.remains>30|target.time_to_die<16" );
  cds->add_action( "fireblood,if=buff.trueshot.up|cooldown.trueshot.remains>30|target.time_to_die<9" );
  cds->add_action( "lights_judgment,if=buff.trueshot.down" );
  cds->add_action( "potion,if=buff.trueshot.up&(buff.bloodlust.up|target.health.pct<20)|target.time_to_die<26|(covenant.kyrian&buff.resonating_arrow.up&target.time_to_die<72)" );

  st->add_action( "steady_shot,if=talent.steady_focus&(prev_gcd.1.steady_shot&buff.steady_focus.remains<5|buff.steady_focus.down)&(buff.resonating_arrow.down|!covenant.kyrian)" );
  st->add_action( "kill_shot" );
  st->add_action( "double_tap,if=covenant.kyrian&(cooldown.resonating_arrow.remains<gcd)|!covenant.kyrian&!covenant.night_fae|covenant.night_fae&(cooldown.wild_spirits.remains<gcd|cooldown.wild_spirits.remains>30)|target.time_to_die<15" );
  st->add_action( "flare,if=tar_trap.up&runeforge.soulforge_embers" );
  st->add_action( "tar_trap,if=runeforge.soulforge_embers&tar_trap.remains<gcd&cooldown.flare.remains<gcd" );
  st->add_action( "explosive_shot" );
  st->add_action( "wild_spirits" );
  st->add_action( "flayed_shot" );
  st->add_action( "death_chakram,if=focus+cast_regen<focus.max" );
  st->add_action( "a_murder_of_crows" );
  st->add_action( "wailing_arrow,if=cooldown.resonating_arrow.remains<gcd&(!talent.explosive_shot|buff.bloodlust.up)|!covenant.kyrian|cooldown.resonating_arrow.remains|target.time_to_die<5" );
  st->add_action( "resonating_arrow,if=buff.double_tap.up|!talent.double_tap|target.time_to_die<12" );
  st->add_action( "volley,if=buff.resonating_arrow.up|!covenant.kyrian&(buff.precise_shots.down|!talent.chimaera_shot|active_enemies<2)" );
  st->add_action( "steady_shot,if=covenant.kyrian&focus+cast_regen<focus.max&((cooldown.resonating_arrow.remains<gcd*3&(!soulbind.effusive_anima_accelerator|!talent.double_tap))|talent.double_tap&cooldown.double_tap.remains<3)" );
  st->add_action( "trueshot,if=buff.precise_shots.down&(covenant.venthyr|covenant.necrolord|talent.calling_the_shots)|buff.resonating_arrow.up|buff.wild_spirits.up|buff.volley.up&active_enemies>1|target.time_to_die<25" );
  st->add_action( "rapid_fire,if=runeforge.surging_shots&talent.streamline&(cooldown.resonating_arrow.remains>10|!covenant.kyrian|!talent.double_tap|soulbind.effusive_anima_accelerator)" );
  st->add_action( "aimed_shot,target_if=min:dot.serpent_sting.remains+action.serpent_sting.in_flight_to_target*99,if=buff.precise_shots.down|(buff.trueshot.up|full_recharge_time<gcd+cast_time)&(!talent.chimaera_shot|active_enemies<2)|buff.trick_shots.remains>execute_time&active_enemies>1" );
  st->add_action( "rapid_fire,if=(cooldown.resonating_arrow.remains>10|!covenant.kyrian|!talent.double_tap|soulbind.effusive_anima_accelerator)&focus+cast_regen<focus.max&(buff.double_tap.down|talent.streamline)" );
  st->add_action( "chimaera_shot,if=buff.precise_shots.up|focus>cost+action.aimed_shot.cost" );
  st->add_action( "arcane_shot,if=buff.precise_shots.up|focus>cost+action.aimed_shot.cost" );
  st->add_action( "serpent_sting,target_if=min:remains,if=refreshable&target.time_to_die>duration" );
  st->add_action( "barrage,if=active_enemies>1" );
  st->add_action( "rapid_fire,if=(cooldown.resonating_arrow.remains>10&runeforge.surging_shots|!covenant.kyrian|!talent.double_tap|soulbind.effusive_anima_accelerator)&focus+cast_regen<focus.max&(buff.double_tap.down|talent.streamline)" );
  st->add_action( "bag_of_tricks,if=buff.trueshot.down" );
  st->add_action( "fleshcraft,if=soulbind.pustule_eruption&buff.trueshot.down" );
  st->add_action( "steady_shot" );

  trickshots->add_action( "steady_shot,if=talent.steady_focus&in_flight&buff.steady_focus.remains<5" );
  trickshots->add_action( "double_tap,if=covenant.kyrian&cooldown.resonating_arrow.remains<gcd|!covenant.kyrian&!covenant.night_fae|covenant.night_fae&(cooldown.wild_spirits.remains<gcd|cooldown.wild_spirits.remains>30)|target.time_to_die<10|cooldown.resonating_arrow.remains>10&active_enemies>3" );
  trickshots->add_action( "tar_trap,if=runeforge.soulforge_embers&tar_trap.remains<gcd&cooldown.flare.remains<gcd" );
  trickshots->add_action( "flare,if=tar_trap.up&runeforge.soulforge_embers" );
  trickshots->add_action( "explosive_shot" );
  trickshots->add_action( "wild_spirits" );
  trickshots->add_action( "wailing_arrow,if=cooldown.resonating_arrow.remains<gcd&(!talent.explosive_shot|buff.bloodlust.up)|!covenant.kyrian|cooldown.resonating_arrow.remains>10|target.time_to_die<5" );
  trickshots->add_action( "resonating_arrow,if=cooldown.volley.remains<gcd|!talent.volley|target.time_to_die<12" );
  trickshots->add_action( "volley,if=buff.resonating_arrow.up|!covenant.kyrian" );
  trickshots->add_action( "barrage" );
  trickshots->add_action( "trueshot,if=buff.resonating_arrow.up|cooldown.resonating_arrow.remains>10|!covenant.kyrian|target.time_to_die<20" );
  trickshots->add_action( "rapid_fire,if=runeforge.surging_shots&(cooldown.resonating_arrow.remains>10|!covenant.kyrian|!talent.double_tap)&buff.trick_shots.remains>=execute_time" );
  trickshots->add_action( "aimed_shot,target_if=min:dot.serpent_sting.remains+action.serpent_sting.in_flight_to_target*99,if=buff.trick_shots.remains>=execute_time&(buff.precise_shots.down|full_recharge_time<cast_time+gcd|buff.trueshot.up)" );
  trickshots->add_action( "death_chakram,if=focus+cast_regen<focus.max" );
  trickshots->add_action( "rapid_fire,if=(cooldown.resonating_arrow.remains>10&runeforge.surging_shots|!covenant.kyrian|!runeforge.surging_shots|!talent.double_tap)&buff.trick_shots.remains>=execute_time" );
  trickshots->add_action( "multishot,if=buff.trick_shots.down|buff.precise_shots.up&focus>cost+action.aimed_shot.cost&(!talent.chimaera_shot|active_enemies>3)" );
  trickshots->add_action( "chimaera_shot,if=buff.precise_shots.up&focus>cost+action.aimed_shot.cost&active_enemies<4" );
  trickshots->add_action( "kill_shot,if=buff.dead_eye.down" );
  trickshots->add_action( "a_murder_of_crows" );
  trickshots->add_action( "flayed_shot" );
  trickshots->add_action( "serpent_sting,target_if=min:dot.serpent_sting.remains,if=refreshable" );
  trickshots->add_action( "multishot,if=focus>cost+action.aimed_shot.cost&(cooldown.resonating_arrow.remains>5|!covenant.kyrian)" );
  trickshots->add_action( "tar_trap,if=runeforge.nessingwarys_trapping_apparatus" );
  trickshots->add_action( "freezing_trap,if=runeforge.nessingwarys_trapping_apparatus" );
  trickshots->add_action( "bag_of_tricks,if=buff.trueshot.down" );
  trickshots->add_action( "fleshcraft,if=soulbind.pustule_eruption&buff.trueshot.down" );
  trickshots->add_action( "steady_shot" );
}
//marksmanship_apl_end

//survival_apl_start
void survival( player_t* p )
{
  action_priority_list_t* default_  = p -> get_action_priority_list( "default" );
  action_priority_list_t* precombat = p -> get_action_priority_list( "precombat" );
  action_priority_list_t* cds       = p -> get_action_priority_list( "cds" );
  action_priority_list_t* st        = p -> get_action_priority_list( "st" );
  action_priority_list_t* bop       = p -> get_action_priority_list( "bop" );
  action_priority_list_t* nta       = p -> get_action_priority_list( "nta" );
  action_priority_list_t* cleave    = p -> get_action_priority_list( "cleave" );

  precombat -> add_action( "flask" );
  precombat -> add_action( "augmentation" );
  precombat -> add_action( "food" );
  precombat -> add_action( "variable,name=mb_rs_cost,op=setif,value=action.mongoose_bite.cost,value_else=action.raptor_strike.cost,condition=talent.mongoose_bite" );
  precombat -> add_action( "summon_pet" );
  precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat -> add_action( "fleshcraft" );
  precombat -> add_action( "tar_trap,if=runeforge.soulforge_embers" );
  precombat -> add_action( "steel_trap,precast_time=20" );

  default_ -> add_action( "auto_attack" );
  default_ -> add_action( "use_items" );
  default_ -> add_action( "newfound_resolve,if=soulbind.newfound_resolve&(buff.resonating_arrow.up|cooldown.resonating_arrow.remains>10|target.time_to_die<16)", "Delay facing your doubt until you have put Resonating Arrow down, or if the cooldown is too long to delay facing your Doubt. If none of these conditions are able to met within the 10 seconds leeway, the sim faces your Doubt automatically." ); 
  default_ -> add_action( "call_action_list,name=cds" );
  default_ -> add_action( "call_action_list,name=bop,if=active_enemies<3&talent.birds_of_prey.enabled" );
  default_ -> add_action( "call_action_list,name=st,if=active_enemies<3&!talent.birds_of_prey.enabled" );
  default_ -> add_action( "call_action_list,name=cleave,if=active_enemies>2" );
  default_ -> add_action( "arcane_torrent" );

  cds -> add_action( "harpoon,if=talent.terms_of_engagement.enabled&focus<focus.max" );
  cds -> add_action( "use_item,name=dreadfire_vessel,if=covenant.kyrian&cooldown.resonating_arrow.remains>10|!covenant.kyrian" );
  cds -> add_action( "use_item,name=jotungeirr_destinys_call,if=buff.coordinated_assault.up|time_to_die<31" );
  cds -> add_action( "blood_fury,if=buff.coordinated_assault.up" );
  cds -> add_action( "ancestral_call,if=buff.coordinated_assault.up" );
  cds -> add_action( "fireblood,if=buff.coordinated_assault.up" );
  cds -> add_action( "lights_judgment" );
  cds -> add_action( "bag_of_tricks,if=cooldown.kill_command.full_recharge_time>gcd" );
  cds -> add_action( "berserking,if=buff.coordinated_assault.up|time_to_die<13" );
  cds -> add_action( "muzzle" );
  cds -> add_action( "potion,if=target.time_to_die<60|buff.coordinated_assault.up" );
  cds -> add_action( "fleshcraft,cancel_if=channeling&!soulbind.pustule_eruption,if=(focus<70|cooldown.coordinated_assault.remains<gcd)&(soulbind.pustule_eruption|soulbind.volatile_solvent)" );
  cds -> add_action( "tar_trap,if=focus+cast_regen<focus.max&runeforge.soulforge_embers.equipped&tar_trap.remains<gcd&cooldown.flare.remains<gcd&(active_enemies>1|active_enemies=1&time_to_die>5*gcd)" );
  cds -> add_action( "flare,if=focus+cast_regen<focus.max&tar_trap.up&runeforge.soulforge_embers.equipped&time_to_die>4*gcd" );
  cds -> add_action( "kill_shot,if=active_enemies=1&target.time_to_die<focus%(variable.mb_rs_cost-cast_regen)*gcd" );
  cds -> add_action( "mongoose_bite,if=active_enemies=1&target.time_to_die<focus%(variable.mb_rs_cost-cast_regen)*gcd" );
  cds -> add_action( "raptor_strike,if=active_enemies=1&target.time_to_die<focus%(variable.mb_rs_cost-cast_regen)*gcd" );
  cds -> add_action( "aspect_of_the_eagle,if=target.distance>=6" );
  
  nta -> add_action( "steel_trap" );
  nta -> add_action( "freezing_trap,if=!buff.wild_spirits.remains|buff.wild_spirits.remains&cooldown.kill_command.remains" );
  nta -> add_action( "tar_trap,if=!buff.wild_spirits.remains|buff.wild_spirits.remains&cooldown.kill_command.remains" );

  st -> add_action( "death_chakram,if=focus+cast_regen<focus.max" );
  st -> add_action( "serpent_sting,target_if=min:remains,if=!dot.serpent_sting.ticking&target.time_to_die>7|buff.vipers_venom.up&buff.vipers_venom.remains<gcd" );
  st -> add_action( "flayed_shot" );
  st -> add_action( "resonating_arrow" );
  st -> add_action( "wild_spirits" );
  st -> add_action( "coordinated_assault" );
  st -> add_action( "kill_shot" );
  st -> add_action( "flanking_strike,if=focus+cast_regen<focus.max" );
  st -> add_action( "a_murder_of_crows" );
  st -> add_action( "wildfire_bomb,if=full_recharge_time<gcd|focus+cast_regen<focus.max&(next_wi_bomb.volatile&dot.serpent_sting.ticking&dot.serpent_sting.refreshable|next_wi_bomb.pheromone&!buff.mongoose_fury.up&focus+cast_regen<focus.max-action.kill_command.cast_regen*3)|time_to_die<10" );
  st -> add_action( "carve,if=active_enemies>1&!runeforge.rylakstalkers_confounding_strikes.equipped" );
  st -> add_action( "butchery,if=active_enemies>1&!runeforge.rylakstalkers_confounding_strikes.equipped&cooldown.wildfire_bomb.full_recharge_time>spell_targets&(charges_fractional>2.5|dot.shrapnel_bomb.ticking)" );
  st -> add_action( "steel_trap,if=focus+cast_regen<focus.max" );
  st -> add_action( "mongoose_bite,target_if=max:debuff.latent_poison_injection.stack,if=talent.alpha_predator.enabled&(buff.mongoose_fury.up&buff.mongoose_fury.remains<focus%(variable.mb_rs_cost-cast_regen)*gcd&!buff.wild_spirits.remains|buff.mongoose_fury.remains&next_wi_bomb.pheromone)" );
  st -> add_action( "kill_command,target_if=min:bloodseeker.remains,if=full_recharge_time<gcd&focus+cast_regen<focus.max" );
  st -> add_action( "raptor_strike,target_if=max:debuff.latent_poison_injection.stack,if=buff.tip_of_the_spear.stack=3|dot.shrapnel_bomb.ticking" );
  st -> add_action( "mongoose_bite,if=dot.shrapnel_bomb.ticking" );
  st -> add_action( "serpent_sting,target_if=min:remains,if=refreshable&target.time_to_die>7|buff.vipers_venom.up" );
  st -> add_action( "wildfire_bomb,if=next_wi_bomb.shrapnel&focus>variable.mb_rs_cost*2&dot.serpent_sting.remains>5*gcd" );
  st -> add_action( "chakrams" );
  st -> add_action( "kill_command,target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max" );
  st -> add_action( "wildfire_bomb,if=runeforge.rylakstalkers_confounding_strikes.equipped" );
  st -> add_action( "mongoose_bite,target_if=max:debuff.latent_poison_injection.stack,if=buff.mongoose_fury.up|focus+action.kill_command.cast_regen>focus.max-15|dot.shrapnel_bomb.ticking|buff.wild_spirits.remains" );
  st -> add_action( "raptor_strike,target_if=max:debuff.latent_poison_injection.stack" );
  st -> add_action( "wildfire_bomb,if=next_wi_bomb.volatile&dot.serpent_sting.ticking|next_wi_bomb.pheromone|next_wi_bomb.shrapnel&focus>50" );

  bop -> add_action( "serpent_sting,target_if=min:remains,if=buff.vipers_venom.remains&(buff.vipers_venom.remains<gcd|refreshable)" );
  bop -> add_action( "kill_command,target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max&buff.nesingwarys_trapping_apparatus.up|focus+cast_regen<focus.max+10&buff.nesingwarys_trapping_apparatus.up&buff.nesingwarys_trapping_apparatus.remains<gcd" );
  bop -> add_action( "kill_shot" );
  bop -> add_action( "wildfire_bomb,if=focus+cast_regen<focus.max&!ticking&full_recharge_time<gcd" );
  bop -> add_action( "flanking_strike,if=focus+cast_regen<focus.max" );
  bop -> add_action( "flayed_shot" );
  bop -> add_action( "call_action_list,name=nta,if=runeforge.nessingwarys_trapping_apparatus.equipped&focus<variable.mb_rs_cost" ); 
  bop -> add_action( "death_chakram,if=focus+cast_regen<focus.max" );
  bop -> add_action( "raptor_strike,target_if=max:debuff.latent_poison_injection.stack,if=buff.coordinated_assault.up&buff.coordinated_assault.remains<1.5*gcd" );
  bop -> add_action( "mongoose_bite,target_if=max:debuff.latent_poison_injection.stack,if=buff.coordinated_assault.up&buff.coordinated_assault.remains<1.5*gcd" );
  bop -> add_action( "a_murder_of_crows" );
  bop -> add_action( "raptor_strike,target_if=max:debuff.latent_poison_injection.stack,if=buff.tip_of_the_spear.stack=3" );
  bop -> add_action( "wildfire_bomb,if=focus+cast_regen<focus.max&!ticking&(full_recharge_time<gcd|!dot.wildfire_bomb.ticking&buff.mongoose_fury.remains>full_recharge_time-1*gcd|!dot.wildfire_bomb.ticking&!buff.mongoose_fury.remains)|time_to_die<18&!dot.wildfire_bomb.ticking" );
  bop -> add_action( "kill_command,target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max&(!runeforge.nessingwarys_trapping_apparatus|focus<variable.mb_rs_cost)" , "If you don't have Nessingwary's Trapping Apparatus, simply cast Kill Command if you won't overcap on Focus from doing so. If you do have Nessingwary's Trapping Apparatus you should cast Kill Command if your focus is below the cost of Mongoose Bite or Raptor Strike" ); 
  bop -> add_action( "kill_command,target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max&runeforge.nessingwarys_trapping_apparatus&cooldown.freezing_trap.remains>(focus%(variable.mb_rs_cost-cast_regen)*gcd)&cooldown.tar_trap.remains>(focus%(variable.mb_rs_cost-cast_regen)*gcd)&(!talent.steel_trap|talent.steel_trap&cooldown.steel_trap.remains>(focus%(variable.mb_rs_cost-cast_regen)*gcd))", "With Nessingwary's Trapping Apparatus only Kill Command if your traps are on cooldown, otherwise stop using Kill Command if your current focus amount is enough to sustain the amount of time left for any of your traps to come off cooldown" );
  bop -> add_action( "steel_trap,if=focus+cast_regen<focus.max" );
  bop -> add_action( "serpent_sting,target_if=min:remains,if=dot.serpent_sting.refreshable&!buff.coordinated_assault.up" );
  bop -> add_action( "resonating_arrow" );
  bop -> add_action( "wild_spirits" );
  bop -> add_action( "coordinated_assault,if=!buff.coordinated_assault.up" );
  bop -> add_action( "mongoose_bite,if=buff.mongoose_fury.up|focus+action.kill_command.cast_regen>focus.max|buff.coordinated_assault.up" );
  bop -> add_action( "raptor_strike,target_if=max:debuff.latent_poison_injection.stack" );
  bop -> add_action( "wildfire_bomb,if=dot.wildfire_bomb.refreshable" );
  bop -> add_action( "serpent_sting,target_if=min:remains,if=buff.vipers_venom.up" );

  cleave -> add_action( "serpent_sting,target_if=min:remains,if=talent.hydras_bite.enabled&buff.vipers_venom.remains&buff.vipers_venom.remains<gcd" );
  cleave -> add_action( "wild_spirits" );
  cleave -> add_action( "resonating_arrow" );
  cleave -> add_action( "wildfire_bomb,if=full_recharge_time<gcd" );
  cleave -> add_action( "chakrams" );
  cleave -> add_action( "butchery,if=dot.shrapnel_bomb.ticking&(dot.internal_bleeding.stack<2|dot.shrapnel_bomb.remains<gcd)" );
  cleave -> add_action( "carve,if=dot.shrapnel_bomb.ticking" );
  cleave -> add_action( "death_chakram,if=focus+cast_regen<focus.max" );
  cleave -> add_action( "coordinated_assault" );
  cleave -> add_action( "butchery,if=charges_fractional>2.5&cooldown.wildfire_bomb.full_recharge_time>spell_targets%2" );
  cleave -> add_action( "flanking_strike,if=focus+cast_regen<focus.max" );
  cleave -> add_action( "carve,if=cooldown.wildfire_bomb.full_recharge_time>spell_targets%2&talent.alpha_predator.enabled" );
  cleave -> add_action( "kill_command,target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max&full_recharge_time<gcd&(runeforge.nessingwarys_trapping_apparatus.equipped&cooldown.freezing_trap.remains&cooldown.tar_trap.remains|!runeforge.nessingwarys_trapping_apparatus.equipped)" );
  cleave -> add_action( "wildfire_bomb,if=!dot.wildfire_bomb.ticking" );
  cleave -> add_action( "butchery,if=(!next_wi_bomb.shrapnel|!talent.wildfire_infusion.enabled)&cooldown.wildfire_bomb.full_recharge_time>spell_targets%2" );
  cleave -> add_action( "carve,if=cooldown.wildfire_bomb.full_recharge_time>spell_targets%2" );
  cleave -> add_action( "kill_shot" );
  cleave -> add_action( "flayed_shot" );
  cleave -> add_action( "a_murder_of_crows" );
  cleave -> add_action( "steel_trap" );
  cleave -> add_action( "serpent_sting,target_if=min:remains,if=refreshable&talent.hydras_bite.enabled&target.time_to_die>8" );
  cleave -> add_action( "carve" );
  cleave -> add_action( "kill_command,target_if=focus+cast_regen<focus.max&(runeforge.nessingwarys_trapping_apparatus.equipped&cooldown.freezing_trap.remains&cooldown.tar_trap.remains|!runeforge.nessingwarys_trapping_apparatus.equipped)" );
  cleave -> add_action( "serpent_sting,target_if=min:remains,if=refreshable" );
  cleave -> add_action( "mongoose_bite,target_if=max:debuff.latent_poison_injection.stack" );
  cleave -> add_action( "raptor_strike,target_if=max:debuff.latent_poison_injection.stack" );
}
//survival_apl_end

} // namespace hunter_apl
