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
  precombat->add_action( "wailing_arrow" );

  default_->add_action( "auto_shot" );
  default_->add_action( "counter_shot,line_cd=30,if=runeforge.sephuzs_proclamation|soulbind.niyas_tools_poison|(conduit.reversal_of_fortune&!runeforge.sephuzs_proclamation)" );
  default_->add_action( "newfound_resolve,if=soulbind.newfound_resolve&(buff.resonating_arrow.up|cooldown.resonating_arrow.remains>10|target.time_to_die<16)", "Delay facing your doubt until you have put Resonating Arrow down, or if the cooldown is too long to delay facing your Doubt. If none of these conditions are able to met within the 10 seconds leeway, the sim faces your Doubt automatically." );
  default_->add_action( "call_action_list,name=trinkets,if=covenant.kyrian&cooldown.bestial_wrath.remains&cooldown.resonating_arrow.remains|!covenant.kyrian&cooldown.bestial_wrath.remains" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=st,if=active_enemies<2" );
  default_->add_action( "call_action_list,name=cleave,if=active_enemies>1" );

  cds->add_action( "ancestral_call,if=cooldown.bestial_wrath.remains>30" );
  cds->add_action( "fireblood,if=cooldown.bestial_wrath.remains>30" );
  cds->add_action( "berserking,if=(buff.wild_spirits.up|!covenant.night_fae&buff.bestial_wrath.up&buff.bestial_wrath.up)&(target.time_to_die>cooldown.berserking.duration+duration|(target.health.pct<35|!talent.killer_instinct))|target.time_to_die<13" );
  cds->add_action( "blood_fury,if=(buff.wild_spirits.up|!covenant.night_fae&buff.bestial_wrath.up&buff.bestial_wrath.up)&(target.time_to_die>cooldown.blood_fury.duration+duration|(target.health.pct<35|!talent.killer_instinct))|target.time_to_die<16" );
  cds->add_action( "lights_judgment" );
  cds->add_action( "potion,if=buff.bestial_wrath.up|target.time_to_die<26" );

  cleave->add_action( "aspect_of_the_wild,if=!raid_event.adds.exists|raid_event.adds.remains>=10|active_enemies>=raid_event.adds.count*2" );
  cleave->add_action( "call_of_the_wild" );
  cleave->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=pet.main.buff.frenzy.up&pet.main.buff.frenzy.remains<=gcd|buff.wild_spirits.up&charges_fractional>1.4&runeforge.fragments_of_the_elder_antlers" );
  cleave->add_action( "multishot,if=gcd-pet.main.buff.beast_cleave.remains>0.25" );
  cleave->add_action( "kill_shot,if=runeforge.pouch_of_razor_fragments&buff.flayers_mark.up" );
  cleave->add_action( "flayed_shot,if=runeforge.pouch_of_razor_fragments" );
  cleave->add_action( "tar_trap,if=runeforge.soulforge_embers&tar_trap.remains<gcd&cooldown.flare.remains<gcd" );
  cleave->add_action( "flare,if=tar_trap.up&runeforge.soulforge_embers" );
  cleave->add_action( "explosive_shot" );
  cleave->add_action( "steel_trap" );
  cleave->add_action( "death_chakram,if=focus+cast_regen<focus.max" );
  cleave->add_action( "wild_spirits,if=!raid_event.adds.exists|raid_event.adds.remains>=10|active_enemies>=raid_event.adds.count*2" );
  cleave->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=full_recharge_time<gcd&cooldown.bestial_wrath.remains|cooldown.bestial_wrath.remains<12+gcd&talent.scent_of_blood|cooldown.kill_command.remains" );
  cleave->add_action( "bestial_wrath,if=!raid_event.adds.exists|raid_event.adds.remains>=5|active_enemies>=raid_event.adds.count*2" );
  cleave->add_action( "resonating_arrow,if=!raid_event.adds.exists|raid_event.adds.remains>=5|active_enemies>=raid_event.adds.count*2" );
  cleave->add_action( "stampede,if=buff.bestial_wrath.up|target.time_to_die<15" );
  cleave->add_action( "flayed_shot" );
  cleave->add_action( "serpent_sting,target_if=min:dot.serpent_sting.remains,if=refreshable" );
  cleave->add_action( "bloodshed" );
  cleave->add_action( "a_murder_of_crows" );
  cleave->add_action( "barrage,if=pet.main.buff.frenzy.remains>execute_time" );
  cleave->add_action( "kill_command" );
  cleave->add_action( "wailing_arrow,if=pet.main.buff.frenzy.remains>execute_time" );
  cleave->add_action( "bag_of_tricks,if=buff.bestial_wrath.down|target.time_to_die<5" );
  cleave->add_action( "dire_beast" );
  cleave->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=target.time_to_die<9|charges_fractional>1.2&conduit.bloodletting" );
  cleave->add_action( "cobra_shot,if=focus.time_to_max<gcd*2" );
  cleave->add_action( "tar_trap,if=runeforge.soulforge_embers|runeforge.nessingwarys_trapping_apparatus" );
  cleave->add_action( "freezing_trap,if=runeforge.nessingwarys_trapping_apparatus" );
  cleave->add_action( "arcane_torrent,if=(focus+focus.regen+30)<focus.max" );

  st->add_action( "aspect_of_the_wild,if=(!covenant.night_fae|cooldown.wild_spirits.remains>20)&(!raid_event.adds.exists|!raid_event.adds.up&(raid_event.adds.duration+raid_event.adds.in<20|(raid_event.adds.count=1&covenant.kyrian))|raid_event.adds.up&raid_event.adds.remains>19)" );
  st->add_action( "call_of_the_wild" );
  st->add_action( "barbed_shot,if=pet.main.buff.frenzy.up&pet.main.buff.frenzy.remains<=gcd|buff.wild_spirits.up&charges_fractional>1.4&runeforge.fragments_of_the_elder_antlers" );
  st->add_action( "tar_trap,if=runeforge.soulforge_embers&tar_trap.remains<gcd&cooldown.flare.remains<gcd" );
  st->add_action( "flare,if=tar_trap.up&runeforge.soulforge_embers" );
  st->add_action( "steel_trap" );
  st->add_action( "bloodshed" );
  st->add_action( "wild_spirits,if=!raid_event.adds.exists|!raid_event.adds.up&raid_event.adds.duration+raid_event.adds.in<20|raid_event.adds.up&raid_event.adds.remains>19" );
  st->add_action( "flayed_shot" );
  st->add_action( "explosive_shot" );
  st->add_action( "death_chakram,if=focus+cast_regen<focus.max" );
  st->add_action( "stampede,if=buff.bestial_wrath.up|target.time_to_die<15" );
  st->add_action( "a_murder_of_crows" );
  st->add_action( "resonating_arrow,if=(buff.bestial_wrath.up|target.time_to_die<10)&(!raid_event.adds.exists|!raid_event.adds.up&(raid_event.adds.duration+raid_event.adds.in<20|raid_event.adds.count=1)|raid_event.adds.up&raid_event.adds.remains>19)" );
  st->add_action( "bestial_wrath,if=(cooldown.wild_spirits.remains>15|covenant.kyrian&(cooldown.resonating_arrow.remains<5|cooldown.resonating_arrow.remains>20)|target.time_to_die<15|(!covenant.night_fae&!covenant.kyrian))&(!raid_event.adds.exists|!raid_event.adds.up&(raid_event.adds.duration+raid_event.adds.in<20|raid_event.adds.count=1)|raid_event.adds.up&raid_event.adds.remains>19)" );
  st->add_action( "kill_command" );
  st->add_action( "kill_shot" );
  st->add_action( "barbed_shot,if=talent.wild_instincts&buff.call_of_the_wild.up|fight_remains<9|talent.wild_call&charges_fractional>1.2|talent.scent_of_blood&cooldown.bestial_wrath.remains<12*(charges_fractional+gcd+(action.barbed_shot.cooldown*0.125))|full_recharge_time<gcd&cooldown.bestial_wrath.remains" );
  st->add_action( "bag_of_tricks,if=buff.bestial_wrath.down|target.time_to_die<5" );
  st->add_action( "dire_beast" );
  st->add_action( "serpent_sting,target_if=min:remains,if=refreshable&target.time_to_die>duration" );
  st->add_action( "cobra_shot,if=(focus-cost+focus.regen*(cooldown.kill_command.remains-1)>action.kill_command.cost|cooldown.kill_command.remains>1+gcd)|(buff.bestial_wrath.up|buff.nesingwarys_trapping_apparatus.up)&!runeforge.qapla_eredun_war_order|target.time_to_die<3" );
  st->add_action( "wailing_arrow,if=pet.main.buff.frenzy.remains>execute_time&(cooldown.resonating_arrow.remains<gcd&(!talent.explosive_shot|buff.bloodlust.up)|!covenant.kyrian)|target.time_to_die<5" );
  st->add_action( "arcane_pulse,if=buff.bestial_wrath.down|target.time_to_die<5" );
  st->add_action( "tar_trap,if=runeforge.soulforge_embers|runeforge.nessingwarys_trapping_apparatus" );
  st->add_action( "freezing_trap,if=runeforge.nessingwarys_trapping_apparatus" );
  st->add_action( "arcane_torrent,if=(focus+focus.regen+15)<focus.max" );

  trinkets->add_action( "variable,name=sync_up,value=buff.resonating_arrow.up|buff.bestial_wrath.up" );
  trinkets->add_action( "variable,name=strong_sync_up,value=covenant.kyrian&buff.resonating_arrow.up&buff.bestial_wrath.up|!covenant.kyrian&buff.bestial_wrath.up" );
  trinkets->add_action( "variable,name=strong_sync_remains,op=setif,condition=covenant.kyrian,value=cooldown.resonating_arrow.remains<?cooldown.bestial_wrath.remains_guess,value_else=cooldown.bestial_wrath.remains_guess,if=buff.bestial_wrath.down" );
  trinkets->add_action( "variable,name=strong_sync_remains,op=setif,condition=covenant.kyrian,value=cooldown.resonating_arrow.remains,value_else=cooldown.bestial_wrath.remains_guess,if=buff.bestial_wrath.up" );
  trinkets->add_action( "variable,name=sync_remains,op=setif,condition=covenant.kyrian,value=cooldown.resonating_arrow.remains>?cooldown.bestial_wrath.remains_guess,value_else=cooldown.bestial_wrath.remains_guess" );
  trinkets->add_action( "use_items,slots=trinket1,if=((trinket.1.has_use_buff|covenant.kyrian&trinket.1.has_cooldown)&(variable.strong_sync_up&(!covenant.kyrian&!trinket.2.has_use_buff|covenant.kyrian&!trinket.2.has_cooldown|trinket.2.cooldown.remains|trinket.1.has_use_buff&(!trinket.2.has_use_buff|trinket.1.cooldown.duration>=trinket.2.cooldown.duration)|trinket.1.has_cooldown&!trinket.2.has_use_buff&trinket.1.cooldown.duration>=trinket.2.cooldown.duration)|!variable.strong_sync_up&(!trinket.2.has_use_buff&(trinket.1.cooldown.duration-5<variable.sync_remains|variable.sync_remains>trinket.1.cooldown.duration%2)|trinket.2.has_use_buff&(trinket.1.has_use_buff&trinket.1.cooldown.duration>=trinket.2.cooldown.duration&(trinket.1.cooldown.duration-5<variable.sync_remains|variable.sync_remains>trinket.1.cooldown.duration%2)|(!trinket.1.has_use_buff|trinket.2.cooldown.duration>=trinket.1.cooldown.duration)&(trinket.2.cooldown.ready&trinket.2.cooldown.duration-5>variable.sync_remains&variable.sync_remains<trinket.2.cooldown.duration%2|!trinket.2.cooldown.ready&(trinket.2.cooldown.remains-5<variable.strong_sync_remains&variable.strong_sync_remains>20&(trinket.1.cooldown.duration-5<variable.sync_remains|trinket.2.cooldown.remains-5<variable.sync_remains&trinket.2.cooldown.duration-10+variable.sync_remains<variable.strong_sync_remains|variable.sync_remains>trinket.1.cooldown.duration%2|variable.sync_up)|trinket.2.cooldown.remains-5>variable.strong_sync_remains&(trinket.1.cooldown.duration-5<variable.strong_sync_remains|trinket.1.cooldown.duration<fight_remains&variable.strong_sync_remains+trinket.1.cooldown.duration>fight_remains|!trinket.1.has_use_buff&(variable.sync_remains>trinket.1.cooldown.duration%2|variable.sync_up))))))|target.time_to_die<variable.sync_remains)|!trinket.1.has_use_buff&!covenant.kyrian&(trinket.2.has_use_buff&((!variable.sync_up|trinket.2.cooldown.remains>5)&(variable.sync_remains>20|trinket.2.cooldown.remains-5>variable.sync_remains))|!trinket.2.has_use_buff&(!trinket.2.has_cooldown|trinket.2.cooldown.remains|trinket.2.cooldown.duration>=trinket.1.cooldown.duration)))&(!trinket.1.is.cache_of_acquired_treasures|active_enemies<2&buff.acquired_wand.up|active_enemies>1&!buff.acquired_wand.up)" );
  trinkets->add_action( "use_items,slots=trinket2,if=((trinket.2.has_use_buff|covenant.kyrian&trinket.2.has_cooldown)&(variable.strong_sync_up&(!covenant.kyrian&!trinket.1.has_use_buff|covenant.kyrian&!trinket.1.has_cooldown|trinket.1.cooldown.remains|trinket.2.has_use_buff&(!trinket.1.has_use_buff|trinket.2.cooldown.duration>=trinket.1.cooldown.duration)|trinket.2.has_cooldown&!trinket.1.has_use_buff&trinket.2.cooldown.duration>=trinket.1.cooldown.duration)|!variable.strong_sync_up&(!trinket.1.has_use_buff&(trinket.2.cooldown.duration-5<variable.sync_remains|variable.sync_remains>trinket.2.cooldown.duration%2)|trinket.1.has_use_buff&(trinket.2.has_use_buff&trinket.2.cooldown.duration>=trinket.1.cooldown.duration&(trinket.2.cooldown.duration-5<variable.sync_remains|variable.sync_remains>trinket.2.cooldown.duration%2)|(!trinket.2.has_use_buff|trinket.1.cooldown.duration>=trinket.2.cooldown.duration)&(trinket.1.cooldown.ready&trinket.1.cooldown.duration-5>variable.sync_remains&variable.sync_remains<trinket.1.cooldown.duration%2|!trinket.1.cooldown.ready&(trinket.1.cooldown.remains-5<variable.strong_sync_remains&variable.strong_sync_remains>20&(trinket.2.cooldown.duration-5<variable.sync_remains|trinket.1.cooldown.remains-5<variable.sync_remains&trinket.1.cooldown.duration-10+variable.sync_remains<variable.strong_sync_remains|variable.sync_remains>trinket.2.cooldown.duration%2|variable.sync_up)|trinket.1.cooldown.remains-5>variable.strong_sync_remains&(trinket.2.cooldown.duration-5<variable.strong_sync_remains|trinket.2.cooldown.duration<fight_remains&variable.strong_sync_remains+trinket.2.cooldown.duration>fight_remains|!trinket.2.has_use_buff&(variable.sync_remains>trinket.2.cooldown.duration%2|variable.sync_up))))))|target.time_to_die<variable.sync_remains)|!trinket.2.has_use_buff&!covenant.kyrian&(trinket.1.has_use_buff&((!variable.sync_up|trinket.1.cooldown.remains>5)&(variable.sync_remains>20|trinket.1.cooldown.remains-5>variable.sync_remains))|!trinket.1.has_use_buff&(!trinket.1.has_cooldown|trinket.1.cooldown.remains|trinket.1.cooldown.duration>=trinket.2.cooldown.duration)))&(!trinket.2.is.cache_of_acquired_treasures|active_enemies<2&buff.acquired_wand.up|active_enemies>1&!buff.acquired_wand.up)" );
}
//beast_mastery_apl_end

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
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "fleshcraft" );
  precombat->add_action( "tar_trap,if=runeforge.soulforge_embers" );
  precombat->add_action( "double_tap,precast_time=10,if=active_enemies>1|!covenant.kyrian&!talent.volley" );
  precombat->add_action( "aimed_shot,if=active_enemies<3&(!covenant.kyrian&!talent.volley|active_enemies<2)" );
  precombat->add_action( "steady_shot,if=active_enemies>2|(covenant.kyrian|talent.volley)&active_enemies=2" );

  default_->add_action( "auto_shot" );
  default_->add_action( "counter_shot,line_cd=30,if=runeforge.sephuzs_proclamation|soulbind.niyas_tools_poison|(conduit.reversal_of_fortune&!runeforge.sephuzs_proclamation)" );
  default_->add_action( "call_action_list,name=trinkets,if=covenant.kyrian&cooldown.trueshot.remains&cooldown.resonating_arrow.remains|!covenant.kyrian&cooldown.trueshot.remains" );
  default_->add_action( "newfound_resolve,if=soulbind.newfound_resolve&(buff.resonating_arrow.up|cooldown.resonating_arrow.remains>10|fight_remains<16)", "Delay facing your doubt until you have put Resonating Arrow down, or if the cooldown is too long to delay facing your Doubt. If none of these conditions are able to met within the 10 seconds leeway, the sim faces your Doubt automatically." );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=st,if=active_enemies<3|!talent.trick_shots" );
  default_->add_action( "call_action_list,name=trickshots,if=active_enemies>2" );

  cds->add_action( "berserking,if=(buff.trueshot.up&buff.resonating_arrow.up&covenant.kyrian)|(buff.trueshot.up&buff.wild_spirits.up&covenant.night_fae)|(covenant.venthyr|covenant.necrolord)&buff.trueshot.up|fight_remains<13|(covenant.kyrian&buff.resonating_arrow.up&fight_remains<73)" );
  cds->add_action( "blood_fury,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<16" );
  cds->add_action( "ancestral_call,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<16" );
  cds->add_action( "fireblood,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<9" );
  cds->add_action( "lights_judgment,if=buff.trueshot.down" );
  cds->add_action( "potion,if=buff.trueshot.up&(buff.bloodlust.up|target.health.pct<20)|fight_remains<26|(covenant.kyrian&buff.resonating_arrow.up&fight_remains<72)" );

  st->add_action( "steady_shot,if=talent.steady_focus&(prev_gcd.1.steady_shot&buff.steady_focus.remains<5|buff.steady_focus.down)&(buff.resonating_arrow.down|!covenant.kyrian)" );
  st->add_action( "kill_shot" );
  st->add_action( "steel_trap" );
  st->add_action( "double_tap,if=(covenant.kyrian&(cooldown.resonating_arrow.remains<gcd)|!covenant.kyrian&!covenant.night_fae|covenant.night_fae&(cooldown.wild_spirits.remains<gcd|cooldown.wild_spirits.remains>30)|fight_remains<15)&(!raid_event.adds.exists|raid_event.adds.up&(raid_event.adds.in<10&raid_event.adds.remains<3|raid_event.adds.in>cooldown|active_enemies>1)|!raid_event.adds.up&(raid_event.adds.count=1|raid_event.adds.in>cooldown))" );
  st->add_action( "flare,if=tar_trap.up&runeforge.soulforge_embers" );
  st->add_action( "tar_trap,if=runeforge.soulforge_embers&tar_trap.remains<gcd&cooldown.flare.remains<gcd" );
  st->add_action( "explosive_shot" );
  st->add_action( "stampede" );
  st->add_action( "wild_spirits,if=(cooldown.trueshot.remains<gcd|buff.trueshot.up)&(!raid_event.adds.exists|!raid_event.adds.up&(raid_event.adds.duration+raid_event.adds.in<20|raid_event.adds.in>60)|raid_event.adds.up&raid_event.adds.remains>19|active_enemies>1)|fight_remains<20" );
  st->add_action( "flayed_shot" );
  st->add_action( "death_chakram" );
  st->add_action( "wailing_arrow,if=cooldown.resonating_arrow.remains<gcd&(!talent.explosive_shot|buff.bloodlust.up)|!covenant.kyrian|cooldown.resonating_arrow.remains|target.time_to_die<5" );
  st->add_action( "resonating_arrow,if=(buff.double_tap.up|!talent.double_tap|fight_remains<12)&(!raid_event.adds.exists|!raid_event.adds.up&(raid_event.adds.duration+raid_event.adds.in<10|raid_event.adds.in>40|raid_event.adds.count=1)|raid_event.adds.up&raid_event.adds.remains>9|active_enemies>1)" );
  st->add_action( "volley,if=buff.resonating_arrow.up|!covenant.kyrian&(buff.precise_shots.down|!talent.chimaera_shot|active_enemies<2)" );
  st->add_action( "steady_shot,if=covenant.kyrian&focus+cast_regen<focus.max&((cooldown.resonating_arrow.remains<gcd*3&(!soulbind.effusive_anima_accelerator|!talent.double_tap))|talent.double_tap&cooldown.double_tap.remains<3)" );
  st->add_action( "rapid_fire,if=runeforge.surging_shots&talent.streamline&(cooldown.resonating_arrow.remains>10|!covenant.kyrian|!talent.double_tap|soulbind.effusive_anima_accelerator)" );
  st->add_action( "trueshot,if=((covenant.venthyr&(buff.precise_shots.down|talent.calling_the_shots)|covenant.necrolord|covenant.kyrian&(cooldown.resonating_arrow.remains>30|cooldown.resonating_arrow.remains<10)|covenant.night_fae&(cooldown.wild_spirits.remains>30|buff.wild_spirits.up))|buff.volley.up&active_enemies>1)&(!raid_event.adds.exists|!raid_event.adds.up&(raid_event.adds.duration+raid_event.adds.in<25|raid_event.adds.in>60)|raid_event.adds.up&raid_event.adds.remains>10|active_enemies>1)|fight_remains<25" );
  st->add_action( "multishot,if=buff.bombardment.up&buff.trick_shots.down&active_enemies>1|talent.salvo&buff.salvo.down&!talent.volley" );
  st->add_action( "aimed_shot,target_if=min:dot.serpent_sting.remains+action.serpent_sting.in_flight_to_target*99,if=buff.precise_shots.down|(buff.trueshot.up|full_recharge_time<gcd+cast_time)&(!talent.chimaera_shot|active_enemies<2)|buff.trick_shots.remains>execute_time&active_enemies>1" );
  st->add_action( "steady_shot,if=buff.steady_focus.remains<5&talent.steady_focus&buff.resonating_arrow.down" );
  st->add_action( "rapid_fire,if=(cooldown.resonating_arrow.remains>10|!covenant.kyrian|!talent.double_tap|soulbind.effusive_anima_accelerator)&focus+cast_regen<focus.max&(buff.double_tap.down&buff.eagletalons_true_focus.down|talent.streamline)" );
  st->add_action( "chimaera_shot,if=buff.precise_shots.up|focus>cost+action.aimed_shot.cost" );
  st->add_action( "arcane_shot,if=buff.precise_shots.up|focus>cost+action.aimed_shot.cost" );
  st->add_action( "serpent_sting,target_if=min:remains,if=refreshable&target.time_to_die>duration" );
  st->add_action( "barrage,if=active_enemies>1" );
  st->add_action( "rapid_fire,if=(cooldown.resonating_arrow.remains>10&runeforge.surging_shots|!covenant.kyrian|!talent.double_tap|soulbind.effusive_anima_accelerator)&focus+cast_regen<focus.max&(buff.double_tap.down|talent.streamline)" );
  st->add_action( "bag_of_tricks,if=buff.trueshot.down" );
  st->add_action( "fleshcraft,if=soulbind.pustule_eruption&buff.trueshot.down" );
  st->add_action( "steady_shot" );

  trickshots->add_action( "steady_shot,if=talent.steady_focus&in_flight&buff.steady_focus.remains<5" );
  trickshots->add_action( "kill_shot,if=runeforge.pouch_of_razor_fragments&buff.flayers_mark.up" );
  trickshots->add_action( "steel_trap" );
  trickshots->add_action( "flayed_shot,if=runeforge.pouch_of_razor_fragments" );
  trickshots->add_action( "double_tap,if=(covenant.kyrian&cooldown.resonating_arrow.remains<gcd|!covenant.kyrian&!covenant.night_fae|covenant.night_fae&(cooldown.wild_spirits.remains<gcd|cooldown.wild_spirits.remains>30)|target.time_to_die<10|cooldown.resonating_arrow.remains>10&active_enemies>3)&(!raid_event.adds.exists|raid_event.adds.remains>9|!covenant.kyrian)" );
  trickshots->add_action( "tar_trap,if=runeforge.soulforge_embers&tar_trap.remains<gcd&cooldown.flare.remains<gcd" );
  trickshots->add_action( "flare,if=tar_trap.up&runeforge.soulforge_embers" );
  trickshots->add_action( "explosive_shot" );
  trickshots->add_action( "stampede" );
  trickshots->add_action( "wild_spirits,if=!raid_event.adds.exists|raid_event.adds.remains>10|active_enemies>=raid_event.adds.count*2|raid_event.adds.in>60" );
  trickshots->add_action( "wailing_arrow,if=cooldown.resonating_arrow.remains<gcd&(!talent.explosive_shot|buff.bloodlust.up)|!covenant.kyrian|cooldown.resonating_arrow.remains>10|target.time_to_die<5" );
  trickshots->add_action( "resonating_arrow,if=(cooldown.volley.remains<gcd|!talent.volley|target.time_to_die<12)&(!raid_event.adds.exists|raid_event.adds.remains>9|active_enemies>=raid_event.adds.count*2)" );
  trickshots->add_action( "volley,if=buff.resonating_arrow.up|!covenant.kyrian" );
  trickshots->add_action( "barrage" );
  trickshots->add_action( "trueshot,if=covenant.kyrian&(buff.resonating_arrow.up|cooldown.resonating_arrow.remains>10)|covenant.night_fae&buff.wild_spirits.up|covenant.venthyr|covenant.necrolord|target.time_to_die<25" );
  trickshots->add_action( "rapid_fire,if=runeforge.surging_shots&(cooldown.resonating_arrow.remains>10|!covenant.kyrian|!talent.double_tap)&buff.trick_shots.remains>=execute_time" );
  trickshots->add_action( "aimed_shot,target_if=min:dot.serpent_sting.remains+action.serpent_sting.in_flight_to_target*99,if=buff.trick_shots.remains>=execute_time&(buff.precise_shots.down|full_recharge_time<cast_time+gcd|buff.trueshot.up)" );
  trickshots->add_action( "death_chakram,if=focus+cast_regen<focus.max" );
  trickshots->add_action( "rapid_fire,if=(cooldown.resonating_arrow.remains>10&runeforge.surging_shots|!covenant.kyrian|!runeforge.surging_shots|!talent.double_tap)&buff.trick_shots.remains>=execute_time" );
  trickshots->add_action( "multishot,if=buff.trick_shots.down|buff.precise_shots.up&focus>cost+action.aimed_shot.cost&(!talent.chimaera_shot|active_enemies>3)" );
  trickshots->add_action( "chimaera_shot,if=buff.precise_shots.up&focus>cost+action.aimed_shot.cost&active_enemies<4" );
  trickshots->add_action( "kill_shot" );
  trickshots->add_action( "flayed_shot" );
  trickshots->add_action( "serpent_sting,target_if=min:dot.serpent_sting.remains,if=refreshable" );
  trickshots->add_action( "multishot,if=focus>cost+action.aimed_shot.cost&(cooldown.resonating_arrow.remains>5|!covenant.kyrian)" );
  trickshots->add_action( "tar_trap,if=runeforge.nessingwarys_trapping_apparatus" );
  trickshots->add_action( "freezing_trap,if=runeforge.nessingwarys_trapping_apparatus" );
  trickshots->add_action( "bag_of_tricks,if=buff.trueshot.down" );
  trickshots->add_action( "fleshcraft,if=soulbind.pustule_eruption&buff.trueshot.down" );
  trickshots->add_action( "steady_shot" );

  trinkets->add_action( "variable,name=sync_up,value=buff.resonating_arrow.up|buff.trueshot.up" );
  trinkets->add_action( "variable,name=strong_sync_up,value=covenant.kyrian&buff.resonating_arrow.up&buff.trueshot.up|!covenant.kyrian&buff.trueshot.up" );
  trinkets->add_action( "variable,name=strong_sync_remains,op=setif,condition=covenant.kyrian,value=cooldown.resonating_arrow.remains<?cooldown.trueshot.remains_guess,value_else=cooldown.trueshot.remains_guess,if=buff.trueshot.down" );
  trinkets->add_action( "variable,name=strong_sync_remains,op=setif,condition=covenant.kyrian,value=cooldown.resonating_arrow.remains,value_else=cooldown.trueshot.remains_guess,if=buff.trueshot.up" );
  trinkets->add_action( "variable,name=sync_remains,op=setif,condition=covenant.kyrian,value=cooldown.resonating_arrow.remains>?cooldown.trueshot.remains_guess,value_else=cooldown.trueshot.remains_guess" );
  trinkets->add_action( "use_items,slots=trinket1,if=((trinket.1.has_use_buff|covenant.kyrian&trinket.1.has_cooldown)&(variable.strong_sync_up&(!covenant.kyrian&!trinket.2.has_use_buff|covenant.kyrian&!trinket.2.has_cooldown|trinket.2.cooldown.remains|trinket.1.has_use_buff&(!trinket.2.has_use_buff|trinket.1.cooldown.duration>=trinket.2.cooldown.duration)|trinket.1.has_cooldown&!trinket.2.has_use_buff&trinket.1.cooldown.duration>=trinket.2.cooldown.duration)|!variable.strong_sync_up&(!trinket.2.has_use_buff&(trinket.1.cooldown.duration-5<variable.sync_remains|variable.sync_remains>trinket.1.cooldown.duration%2)|trinket.2.has_use_buff&(trinket.1.has_use_buff&trinket.1.cooldown.duration>=trinket.2.cooldown.duration&(trinket.1.cooldown.duration-5<variable.sync_remains|variable.sync_remains>trinket.1.cooldown.duration%2)|(!trinket.1.has_use_buff|trinket.2.cooldown.duration>=trinket.1.cooldown.duration)&(trinket.2.cooldown.ready&trinket.2.cooldown.duration-5>variable.sync_remains&variable.sync_remains<trinket.2.cooldown.duration%2|!trinket.2.cooldown.ready&(trinket.2.cooldown.remains-5<variable.strong_sync_remains&variable.strong_sync_remains>20&(trinket.1.cooldown.duration-5<variable.sync_remains|trinket.2.cooldown.remains-5<variable.sync_remains&trinket.2.cooldown.duration-10+variable.sync_remains<variable.strong_sync_remains|variable.sync_remains>trinket.1.cooldown.duration%2|variable.sync_up)|trinket.2.cooldown.remains-5>variable.strong_sync_remains&(trinket.1.cooldown.duration-5<variable.strong_sync_remains|trinket.1.cooldown.duration<fight_remains&variable.strong_sync_remains+trinket.1.cooldown.duration>fight_remains|!trinket.1.has_use_buff&(variable.sync_remains>trinket.1.cooldown.duration%2|variable.sync_up))))))|target.time_to_die<variable.sync_remains)|!trinket.1.has_use_buff&!covenant.kyrian&(trinket.2.has_use_buff&((!variable.sync_up|trinket.2.cooldown.remains>5)&(variable.sync_remains>20|trinket.2.cooldown.remains-5>variable.sync_remains))|!trinket.2.has_use_buff&(!trinket.2.has_cooldown|trinket.2.cooldown.remains|trinket.2.cooldown.duration>=trinket.1.cooldown.duration)))&(!trinket.1.is.cache_of_acquired_treasures|!buff.acquired_wand.up)" );
  trinkets->add_action( "use_items,slots=trinket2,if=((trinket.2.has_use_buff|covenant.kyrian&trinket.2.has_cooldown)&(variable.strong_sync_up&(!covenant.kyrian&!trinket.1.has_use_buff|covenant.kyrian&!trinket.1.has_cooldown|trinket.1.cooldown.remains|trinket.2.has_use_buff&(!trinket.1.has_use_buff|trinket.2.cooldown.duration>=trinket.1.cooldown.duration)|trinket.2.has_cooldown&!trinket.1.has_use_buff&trinket.2.cooldown.duration>=trinket.1.cooldown.duration)|!variable.strong_sync_up&(!trinket.1.has_use_buff&(trinket.2.cooldown.duration-5<variable.sync_remains|variable.sync_remains>trinket.2.cooldown.duration%2)|trinket.1.has_use_buff&(trinket.2.has_use_buff&trinket.2.cooldown.duration>=trinket.1.cooldown.duration&(trinket.2.cooldown.duration-5<variable.sync_remains|variable.sync_remains>trinket.2.cooldown.duration%2)|(!trinket.2.has_use_buff|trinket.1.cooldown.duration>=trinket.2.cooldown.duration)&(trinket.1.cooldown.ready&trinket.1.cooldown.duration-5>variable.sync_remains&variable.sync_remains<trinket.1.cooldown.duration%2|!trinket.1.cooldown.ready&(trinket.1.cooldown.remains-5<variable.strong_sync_remains&variable.strong_sync_remains>20&(trinket.2.cooldown.duration-5<variable.sync_remains|trinket.1.cooldown.remains-5<variable.sync_remains&trinket.1.cooldown.duration-10+variable.sync_remains<variable.strong_sync_remains|variable.sync_remains>trinket.2.cooldown.duration%2|variable.sync_up)|trinket.1.cooldown.remains-5>variable.strong_sync_remains&(trinket.2.cooldown.duration-5<variable.strong_sync_remains|trinket.2.cooldown.duration<fight_remains&variable.strong_sync_remains+trinket.2.cooldown.duration>fight_remains|!trinket.2.has_use_buff&(variable.sync_remains>trinket.2.cooldown.duration%2|variable.sync_up))))))|target.time_to_die<variable.sync_remains)|!trinket.2.has_use_buff&!covenant.kyrian&(trinket.1.has_use_buff&((!variable.sync_up|trinket.1.cooldown.remains>5)&(variable.sync_remains>20|trinket.1.cooldown.remains-5>variable.sync_remains))|!trinket.1.has_use_buff&(!trinket.1.has_cooldown|trinket.1.cooldown.remains|trinket.1.cooldown.duration>=trinket.2.cooldown.duration)))&(!trinket.2.is.cache_of_acquired_treasures|!buff.acquired_wand.up)" );
}
//marksmanship_apl_end

//survival_apl_start
void survival( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* cleave = p->get_action_priority_list( "cleave" );
  action_priority_list_t* other_on_use = p->get_action_priority_list( "other_on_use" );
  action_priority_list_t* st = p->get_action_priority_list( "st" );

  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "variable,name=mb_rs_cost,op=setif,value=action.mongoose_bite.cost,value_else=action.raptor_strike.cost,condition=talent.mongoose_bite" );
  precombat->add_action( "summon_pet" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "fleshcraft" );
  precombat->add_action( "tar_trap,if=runeforge.soulforge_embers" );
  precombat->add_action( "steel_trap,precast_time=20" );

  default_->add_action( "auto_attack" );
  default_->add_action( "call_action_list,name=other_on_use" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=st,if=active_enemies<3" );
  default_->add_action( "call_action_list,name=cleave,if=active_enemies>2" );
  default_->add_action( "arcane_torrent" );

  cds->add_action( "harpoon,if=talent.terms_of_engagement.enabled&focus<focus.max" );
  cds->add_action( "blood_fury,if=buff.coordinated_assault.up|buff.spearhead.up|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "ancestral_call,if=buff.coordinated_assault.up|buff.spearhead.up|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "fireblood,if=buff.coordinated_assault.up|buff.spearhead.up|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "lights_judgment" );
  cds->add_action( "bag_of_tricks,if=cooldown.kill_command.full_recharge_time>gcd" );
  cds->add_action( "berserking,if=buff.coordinated_assault.up|buff.spearhead.up|!talent.spearhead&!talent.coordinated_assault|time_to_die<13" );
  cds->add_action( "muzzle" );
  cds->add_action( "potion,if=target.time_to_die<25|buff.coordinated_assault.up|buff.spearhead.up|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "use_items" );
  cds->add_action( "fleshcraft,cancel_if=channeling&!soulbind.pustule_eruption,if=(focus<70|cooldown.coordinated_assault.remains<gcd)&(soulbind.pustule_eruption|soulbind.volatile_solvent)" );
  cds->add_action( "tar_trap,if=focus+cast_regen<focus.max&runeforge.soulforge_embers.equipped&tar_trap.remains<gcd&cooldown.flare.remains<gcd&(active_enemies>1|active_enemies=1&time_to_die>5*gcd)" );
  cds->add_action( "flare,if=focus+cast_regen<focus.max&tar_trap.up&runeforge.soulforge_embers.equipped&time_to_die>4*gcd" );
  cds->add_action( "aspect_of_the_eagle,if=target.distance>=6" );

  cleave->add_action( "wildfire_bomb,if=full_recharge_time<gcd" );
  cleave->add_action( "wild_spirits" );
  cleave->add_action( "flayed_shot,target_if=max:target.health.pct" );
  cleave->add_action( "resonating_arrow" );
  cleave->add_action( "death_chakram,if=focus+cast_regen<focus.max" );
  cleave->add_action( "stampede" );
  cleave->add_action( "coordinated_assault" );
  cleave->add_action( "kill_shot,if=buff.flayers_mark.up|buff.coordinated_assault_empower.up" );
  cleave->add_action( "explosive_shot" );
  cleave->add_action( "carve,if=cooldown.wildfire_bomb.full_recharge_time>spell_targets%2" );
  cleave->add_action( "butchery,if=full_recharge_time<gcd" );
  cleave->add_action( "wildfire_bomb,if=!dot.wildfire_bomb.ticking" );
  cleave->add_action( "butchery,if=dot.shrapnel_bomb.ticking&(dot.internal_bleeding.stack<2|dot.shrapnel_bomb.remains<gcd)" );
  cleave->add_action( "fury_of_the_eagle" );
  cleave->add_action( "carve,if=dot.shrapnel_bomb.ticking" );
  cleave->add_action( "flanking_strike,if=focus+cast_regen<focus.max" );
  cleave->add_action( "butchery,if=(!next_wi_bomb.shrapnel|!talent.wildfire_infusion)&cooldown.wildfire_bomb.full_recharge_time>spell_targets%2" );
  cleave->add_action( "mongoose_bite,target_if=max:debuff.latent_poison.stack,if=debuff.latent_poison.stack>8" );
  cleave->add_action( "kill_command,target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max&full_recharge_time<gcd" );
  cleave->add_action( "serpent_sting,target_if=min:remains,if=refreshable&talent.hydras_bite.enabled&target.time_to_die>8" );
  cleave->add_action( "carve" );
  cleave->add_action( "kill_shot" );
  cleave->add_action( "steel_trap,if=focus+cast_regen<focus.max" );
  cleave->add_action( "serpent_sting,target_if=min:remains,if=refreshable&target.time_to_die>8" );
  cleave->add_action( "mongoose_bite,target_if=min:dot.serpent_sting.remains" );
  cleave->add_action( "raptor_strike,target_if=max:debuff.latent_poison.stack" );

  other_on_use->add_action( "use_items,slots=finger1" );
  other_on_use->add_action( "use_items,slots=finger2" );
  other_on_use->add_action( "use_item,name=jotungeirr_destinys_call,if=cooldown.coordinated_assault.remains>75|time_to_die<30" );

  st->add_action( "death_chakram,if=focus+cast_regen<focus.max" );
  st->add_action( "stampede" );
  st->add_action( "wild_spirits" );
  st->add_action( "flayed_shot" );
  st->add_action( "resonating_arrow" );
  st->add_action( "mongoose_bite,if=active_enemies=1&target.time_to_die<focus%(variable.mb_rs_cost-cast_regen)*gcd|buff.spearhead.remains&set_bonus.tier29_4pc|buff.mongoose_fury.up&buff.mongoose_fury.remains<gcd" );
  st->add_action( "raptor_strike,if=active_enemies=1&target.time_to_die<focus%(variable.mb_rs_cost-cast_regen)*gcd" );
  st->add_action( "serpent_sting,target_if=min:remains,if=!dot.serpent_sting.ticking&target.time_to_die>7&!talent.vipers_venom" );
  st->add_action( "flanking_strike,if=focus+cast_regen<focus.max" );
  st->add_action( "coordinated_assault,if=!buff.spearhead.remains&cooldown.spearhead.remains|!talent.spearhead" );
  st->add_action( "kill_shot,if=buff.coordinated_assault_empower.up|buff.flayers_mark.up" );
  st->add_action( "wildfire_bomb,if=next_wi_bomb.pheromone&!buff.mongoose_fury.up&focus+cast_regen<focus.max-action.kill_command.cast_regen*2|full_recharge_time<gcd" );
  st->add_action( "kill_command,target_if=min:bloodseeker.remains,if=full_recharge_time<gcd&focus+cast_regen<focus.max" );
  st->add_action( "mongoose_bite,if=dot.shrapnel_bomb.ticking" );
  st->add_action( "kill_shot,if=!set_bonus.tier29_4pc" );
  st->add_action( "mongoose_bite,target_if=max:debuff.latent_poison.stack,if=buff.mongoose_fury.up" );
  st->add_action( "spearhead,if=focus+action.kill_command.cast_regen>focus.max-10" );
  st->add_action( "mongoose_bite,target_if=max:debuff.latent_poison.stack,if=focus+action.kill_command.cast_regen>focus.max-10|buff.spearhead.remains" );
  st->add_action( "explosive_shot" );
  st->add_action( "kill_shot,if=set_bonus.tier29_4pc" );
  st->add_action( "raptor_strike,target_if=max:debuff.latent_poison.stack" );
  st->add_action( "steel_trap" );
  st->add_action( "wildfire_bomb,if=!dot.wildfire_bomb.ticking" );
  st->add_action( "fury_of_the_eagle,interrupt=1" );
}
//survival_apl_end

//beast_mastery_df_apl_start
void beast_mastery_df( player_t* p )
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
  precombat->add_action( "steel_trap,precast_time=1.5,if=!talent.wailing_arrow&talent.steel_trap" );

  default_->add_action( "auto_shot" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=st,if=active_enemies<2|!talent.beast_cleave&active_enemies<3" );
  default_->add_action( "call_action_list,name=cleave,if=active_enemies>2|talent.beast_cleave&active_enemies>1" );

  cds->add_action( "berserking,if=!talent.bestial_wrath|buff.bestial_wrath.up|fight_remains<16" );
  cds->add_action( "use_items,slots=trinket1,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&(buff.bestial_wrath.up&(buff.bloodlust.up|target.health.pct<20))|fight_remains<31" );
  cds->add_action( "use_items,slots=trinket2,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&(buff.bestial_wrath.up&(buff.bloodlust.up|target.health.pct<20))|fight_remains<31" );
  cds->add_action( "blood_fury,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&(buff.bestial_wrath.up&(buff.bloodlust.up|target.health.pct<20))|fight_remains<16" );
  cds->add_action( "ancestral_call,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&(buff.bestial_wrath.up&(buff.bloodlust.up|target.health.pct<20))|fight_remains<16" );
  cds->add_action( "fireblood,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&(buff.bestial_wrath.up&(buff.bloodlust.up|target.health.pct<20))|fight_remains<10" );
  cds->add_action( "potion,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&(buff.bestial_wrath.up&(buff.bloodlust.up|target.health.pct<20))|fight_remains<31" );

  cleave->add_action( "barbed_shot,target_if=max:debuff.latent_poison.stack,if=debuff.latent_poison.stack>9&(pet.main.buff.frenzy.up&pet.main.buff.frenzy.remains<=gcd+0.25|talent.scent_of_blood&cooldown.bestial_wrath.remains<12+gcd|full_recharge_time<gcd&cooldown.bestial_wrath.remains)" );
  cleave->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=pet.main.buff.frenzy.up&pet.main.buff.frenzy.remains<=gcd+0.25|talent.scent_of_blood&cooldown.bestial_wrath.remains<12+gcd|full_recharge_time<gcd&cooldown.bestial_wrath.remains" );
  cleave->add_action( "multishot,if=gcd-pet.main.buff.beast_cleave.remains>0.25" );
  cleave->add_action( "kill_command,if=full_recharge_time<gcd&talent.alpha_predator&talent.kill_cleave" );
  cleave->add_action( "call_of_the_wild" );
  cleave->add_action( "explosive_shot" );
  cleave->add_action( "stampede,if=buff.bestial_wrath.up|target.time_to_die<15" );
  cleave->add_action( "bloodshed" );
  cleave->add_action( "death_chakram" );
  cleave->add_action( "bestial_wrath" );
  cleave->add_action( "steel_trap" );
  cleave->add_action( "a_murder_of_crows" );
  cleave->add_action( "barbed_shot,target_if=max:debuff.latent_poison.stack,if=debuff.latent_poison.stack>9&(talent.wild_instincts&buff.call_of_the_wild.up|fight_remains<9|talent.wild_call&charges_fractional>1.2)" );
  cleave->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=talent.wild_instincts&buff.call_of_the_wild.up|fight_remains<9|talent.wild_call&charges_fractional>1.2" );
  cleave->add_action( "kill_command" );
  cleave->add_action( "dire_beast" );
  cleave->add_action( "serpent_sting,target_if=min:remains,if=refreshable&target.time_to_die>duration" );
  cleave->add_action( "barrage,if=pet.main.buff.frenzy.remains>execute_time" );
  cleave->add_action( "kill_shot" );
  cleave->add_action( "aspect_of_the_wild" );
  cleave->add_action( "cobra_shot,if=focus.time_to_max<gcd*2|buff.aspect_of_the_wild.up&focus.time_to_max<gcd*4" );
  cleave->add_action( "wailing_arrow,if=pet.main.buff.frenzy.remains>execute_time|fight_remains<5" );
  cleave->add_action( "bag_of_tricks,if=buff.bestial_wrath.down|target.time_to_die<5" );
  cleave->add_action( "arcane_torrent,if=(focus+focus.regen+30)<focus.max" );

  st->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=pet.main.buff.frenzy.up&pet.main.buff.frenzy.remains<=gcd+0.25|talent.scent_of_blood&pet.main.buff.frenzy.stack<3&cooldown.bestial_wrath.ready" );
  st->add_action( "kill_command,if=full_recharge_time<gcd&talent.alpha_predator" );
  st->add_action( "call_of_the_wild" );
  st->add_action( "death_chakram" );
  st->add_action( "bloodshed" );
  st->add_action( "stampede" );
  st->add_action( "a_murder_of_crows" );
  st->add_action( "steel_trap" );
  st->add_action( "explosive_shot" );
  st->add_action( "bestial_wrath" );
  st->add_action( "kill_command" );
  st->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=talent.wild_instincts&buff.call_of_the_wild.up|talent.wild_call&charges_fractional>1.4|full_recharge_time<gcd&cooldown.bestial_wrath.remains|talent.scent_of_blood&(cooldown.bestial_wrath.remains<12+gcd|full_recharge_time+gcd<8&cooldown.bestial_wrath.remains<24+(8-gcd)+full_recharge_time)|fight_remains<9" );
  st->add_action( "dire_beast" );
  st->add_action( "serpent_sting,target_if=min:remains,if=refreshable&target.time_to_die>duration" );
  st->add_action( "kill_shot" );
  st->add_action( "aspect_of_the_wild" );
  st->add_action( "cobra_shot" );
  st->add_action( "wailing_arrow,if=pet.main.buff.frenzy.remains>execute_time|target.time_to_die<5" );
  st->add_action( "bag_of_tricks,if=buff.bestial_wrath.down|target.time_to_die<5" );
  st->add_action( "arcane_pulse,if=buff.bestial_wrath.down|target.time_to_die<5" );
  st->add_action( "arcane_torrent,if=(focus+focus.regen+15)<focus.max" );
}
//beast_mastery_df_apl_end

//marksmanship_df_apl_start
void marksmanship_df( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* st = p->get_action_priority_list( "st" );
  action_priority_list_t* trickshots = p->get_action_priority_list( "trickshots" );

  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "summon_pet,if=talent.kill_command|talent.beast_master" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "double_tap,precast_time=10" );
  precombat->add_action( "aimed_shot,if=active_enemies<3&(!talent.volley|active_enemies<2)", "Precast Aimed Shot on one or two targets unless we could cleave it with Volley on two targets." );
  precombat->add_action( "wailing_arrow,if=active_enemies>2|!talent.steady_focus" );
  precombat->add_action( "steady_shot,if=active_enemies>2|talent.volley&active_enemies=2", "Precast Steady Shot on two targets if we are saving Aimed Shot to cleave with Volley, otherwise on three or more targets." );

  default_->add_action( "auto_shot" );
  default_->add_action( "use_items,slots=trinket1,if=!trinket.1.has_use_buff|buff.trueshot.up", "New trinket lines are under construction." );
  default_->add_action( "use_items,slots=trinket2,if=!trinket.2.has_use_buff|buff.trueshot.up" );
  default_->add_action( "use_items" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=st,if=active_enemies<3|!talent.trick_shots" );
  default_->add_action( "call_action_list,name=trickshots,if=active_enemies>2" );

  cds->add_action( "berserking,if=fight_remains<13" );
  cds->add_action( "blood_fury,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<16" );
  cds->add_action( "ancestral_call,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<16" );
  cds->add_action( "fireblood,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<9" );
  cds->add_action( "lights_judgment,if=buff.trueshot.down" );
  cds->add_action( "potion,if=buff.trueshot.up&(buff.bloodlust.up|target.health.pct<20)|fight_remains<26" );

  st->add_action( "steady_shot,if=talent.steady_focus&(steady_focus_count&buff.steady_focus.remains<5|buff.steady_focus.down&!buff.trueshot.up)" );
  st->add_action( "kill_shot" );
  st->add_action( "steel_trap" );
  st->add_action( "serpent_sting,target_if=min:dot.serpent_sting.remains,if=refreshable&!talent.serpentstalkers_trickery&buff.trueshot.down" );
  st->add_action( "explosive_shot" );
  st->add_action( "double_tap,if=(cooldown.rapid_fire.remains<gcd|ca_active|!talent.streamline)&(!raid_event.adds.exists|raid_event.adds.up&(raid_event.adds.in<10&raid_event.adds.remains<3|raid_event.adds.in>cooldown|active_enemies>1)|!raid_event.adds.up&(raid_event.adds.count=1|raid_event.adds.in>cooldown))", "With at least Streamline, save Double Tap for Rapid Fire unless Careful Aim is active." );
  st->add_action( "stampede" );
  st->add_action( "death_chakram" );
  st->add_action( "wailing_arrow,if=active_enemies>1" );
  st->add_action( "volley" );
  st->add_action( "rapid_fire,if=talent.surging_shots|buff.double_tap.up&talent.streamline&!ca_active", "With at least Streamline, Double Tap Rapid Fire unless Careful Aim is active." );
  st->add_action( "trueshot,if=!raid_event.adds.exists|!raid_event.adds.up&(raid_event.adds.duration+raid_event.adds.in<25|raid_event.adds.in>60)|raid_event.adds.up&raid_event.adds.remains>10|active_enemies>1|fight_remains<25" );
  st->add_action( "multishot,if=buff.bombardment.up&buff.trick_shots.down&active_enemies>1|talent.salvo&buff.salvo.down&!talent.volley", "Trigger Trick Shots from Bombardment if it isn't already up, or trigger Salvo if Volley isn't being used to trigger it." );
  st->add_action( "aimed_shot,target_if=min:dot.serpent_sting.remains+action.serpent_sting.in_flight_to_target*99,if=talent.serpentstalkers_trickery&((buff.precise_shots.down|(buff.trueshot.up|full_recharge_time<gcd+cast_time)&(!talent.chimaera_shot|active_enemies<2))|buff.trick_shots.remains>execute_time&active_enemies>1)", "For Serpentstalker's Trickery, target the lowest remaining Serpent Sting. On one target don't overwrite Precise Shots unless Trueshot is up or Aimed Shot would cap otherwise, and on two targets don't overwrite Precise Shots if you have Chimaera Shot, but ignore those general rules if we can cleave it." );
  st->add_action( "aimed_shot,target_if=max:debuff.latent_poison.stack,if=(buff.precise_shots.down|(buff.trueshot.up|full_recharge_time<gcd+cast_time)&(!talent.chimaera_shot|active_enemies<2))|buff.trick_shots.remains>execute_time&active_enemies>1", "For no Serpentstalker's Trickery, target the highest Latent Poison stack. Same general rules as the previous line." );
  st->add_action( "steady_shot,if=talent.steady_focus&buff.steady_focus.remains<execute_time*2", "Refresh Steady Focus if it would run out while refreshing it." );
  st->add_action( "rapid_fire" );
  st->add_action( "wailing_arrow,if=buff.trueshot.down" );
  st->add_action( "chimaera_shot,if=buff.precise_shots.up|focus>cost+action.aimed_shot.cost" );
  st->add_action( "arcane_shot,if=buff.precise_shots.up|focus>cost+action.aimed_shot.cost" );
  st->add_action( "bag_of_tricks,if=buff.trueshot.down" );
  st->add_action( "steady_shot" );

  trickshots->add_action( "steady_shot,if=talent.steady_focus&steady_focus_count&buff.steady_focus.remains<8" );
  trickshots->add_action( "kill_shot,if=buff.razor_fragments.up" );
  trickshots->add_action( "double_tap,if=cooldown.rapid_fire.remains<gcd|ca_active|!talent.streamline" );
  trickshots->add_action( "explosive_shot" );
  trickshots->add_action( "death_chakram" );
  trickshots->add_action( "stampede" );
  trickshots->add_action( "wailing_arrow" );
  trickshots->add_action( "serpent_sting,target_if=min:dot.serpent_sting.remains,if=refreshable&talent.hydras_bite" );
  trickshots->add_action( "barrage,if=active_enemies>7" );
  trickshots->add_action( "volley" );
  trickshots->add_action( "trueshot" );
  trickshots->add_action( "rapid_fire,if=buff.trick_shots.remains>=execute_time&(talent.surging_shots|buff.double_tap.up&talent.streamline&!ca_active)" );
  trickshots->add_action( "aimed_shot,target_if=min:dot.serpent_sting.remains+action.serpent_sting.in_flight_to_target*99,if=talent.serpentstalkers_trickery&(buff.trick_shots.remains>=execute_time&(buff.precise_shots.down|buff.trueshot.up|full_recharge_time<cast_time+gcd))", "For Serpentstalker's Trickery, target the lowest remaining Serpent Sting. Generally only cast if it would cleave with Trick Shots. Don't overwrite Precise Shots unless Trueshot is up or Aimed Shot would cap otherwise." );
  trickshots->add_action( "aimed_shot,target_if=max:debuff.latent_poison.stack,if=(buff.trick_shots.remains>=execute_time&(buff.precise_shots.down|buff.trueshot.up|full_recharge_time<cast_time+gcd))", "For no Serpentstalker's Trickery, target the highest Latent Poison stack. Same general rules as the previous line." );
  trickshots->add_action( "rapid_fire,if=buff.trick_shots.remains>=execute_time" );
  trickshots->add_action( "chimaera_shot,if=buff.trick_shots.up&buff.precise_shots.up&focus>cost+action.aimed_shot.cost&active_enemies<4" );
  trickshots->add_action( "multishot,if=buff.trick_shots.down|(buff.precise_shots.up|buff.bulletstorm.stack=10)&focus>cost+action.aimed_shot.cost" );
  trickshots->add_action( "serpent_sting,target_if=min:dot.serpent_sting.remains,if=refreshable&talent.poison_injection&!talent.serpentstalkers_trickery", "Only use baseline Serpent Sting as a filler in cleave if it's the only source of applying Latent Poison." );
  trickshots->add_action( "steel_trap" );
  trickshots->add_action( "kill_shot,if=focus>cost+action.aimed_shot.cost" );
  trickshots->add_action( "multishot,if=focus>cost+action.aimed_shot.cost" );
  trickshots->add_action( "bag_of_tricks,if=buff.trueshot.down" );
  trickshots->add_action( "steady_shot" );
}
//marksmanship_df_apl_end

//survival_df_apl_start
void survival_df( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* cleave = p->get_action_priority_list( "cleave" );
  action_priority_list_t* st = p->get_action_priority_list( "st" );

  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "variable,name=mb_rs_cost,op=setif,value=action.mongoose_bite.cost,value_else=action.raptor_strike.cost,condition=talent.mongoose_bite" );
  precombat->add_action( "summon_pet" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "steel_trap,precast_time=20" );

  default_->add_action( "auto_attack" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=st,if=active_enemies<3" );
  default_->add_action( "call_action_list,name=cleave,if=active_enemies>2" );
  default_->add_action( "arcane_torrent" );

  cds->add_action( "harpoon,if=talent.terms_of_engagement.enabled&focus<focus.max" );
  cds->add_action( "blood_fury,if=buff.coordinated_assault.up|buff.spearhead.up|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "ancestral_call,if=buff.coordinated_assault.up|buff.spearhead.up|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "fireblood,if=buff.coordinated_assault.up|buff.spearhead.up|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "lights_judgment" );
  cds->add_action( "bag_of_tricks,if=cooldown.kill_command.full_recharge_time>gcd" );
  cds->add_action( "berserking,if=buff.coordinated_assault.up|buff.spearhead.up|!talent.spearhead&!talent.coordinated_assault|time_to_die<13" );
  cds->add_action( "muzzle" );
  cds->add_action( "potion,if=target.time_to_die<30|buff.coordinated_assault.up|buff.spearhead.up|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "use_items" );
  cds->add_action( "aspect_of_the_eagle,if=target.distance>=6" );

  cleave->add_action( "wildfire_bomb,if=full_recharge_time<gcd" );
  cleave->add_action( "death_chakram,if=focus+cast_regen<focus.max" );
  cleave->add_action( "stampede" );
  cleave->add_action( "coordinated_assault" );
  cleave->add_action( "kill_shot,if=buff.coordinated_assault_empower.up" );
  cleave->add_action( "explosive_shot" );
  cleave->add_action( "carve,if=cooldown.wildfire_bomb.full_recharge_time>spell_targets%2" );
  cleave->add_action( "butchery,if=full_recharge_time<gcd" );
  cleave->add_action( "wildfire_bomb,if=!dot.wildfire_bomb.ticking" );
  cleave->add_action( "butchery,if=dot.shrapnel_bomb.ticking&(dot.internal_bleeding.stack<2|dot.shrapnel_bomb.remains<gcd)" );
  cleave->add_action( "fury_of_the_eagle" );
  cleave->add_action( "carve,if=dot.shrapnel_bomb.ticking" );
  cleave->add_action( "flanking_strike,if=focus+cast_regen<focus.max" );
  cleave->add_action( "butchery,if=(!next_wi_bomb.shrapnel|!talent.wildfire_infusion)&cooldown.wildfire_bomb.full_recharge_time>spell_targets%2" );
  cleave->add_action( "mongoose_bite,target_if=max:debuff.latent_poison.stack,if=debuff.latent_poison.stack>8" );
  cleave->add_action( "raptor_strike,target_if=max:debuff.latent_poison.stack,if=debuff.latent_poison.stack>8" );
  cleave->add_action( "kill_command,target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max&full_recharge_time<gcd" );
  cleave->add_action( "carve" );
  cleave->add_action( "kill_shot" );
  cleave->add_action( "steel_trap,if=focus+cast_regen<focus.max" );
  cleave->add_action( "serpent_sting,target_if=min:remains,if=refreshable&target.time_to_die>8" );
  cleave->add_action( "mongoose_bite,target_if=min:dot.serpent_sting.remains" );
  cleave->add_action( "raptor_strike,target_if=min:dot.serpent_sting.remains" );

  st->add_action( "death_chakram,if=focus+cast_regen<focus.max|talent.spearhead&!cooldown.spearhead.remains" );
  st->add_action( "spearhead,if=focus+action.kill_command.cast_regen>focus.max-10&(cooldown.death_chakram.remains|!talent.death_chakram)" );
  st->add_action( "kill_shot,if=buff.coordinated_assault_empower.up" );
  st->add_action( "kill_command,target_if=min:bloodseeker.remains,if=full_recharge_time<gcd&focus+cast_regen<focus.max&buff.deadly_duo.stack>1" );
  st->add_action( "mongoose_bite,if=buff.spearhead.remains" );
  st->add_action( "mongoose_bite,if=active_enemies=1&target.time_to_die<focus%(variable.mb_rs_cost-cast_regen)*gcd|buff.mongoose_fury.up&buff.mongoose_fury.remains<gcd" );
  st->add_action( "kill_shot" );
  st->add_action( "raptor_strike,if=active_enemies=1&target.time_to_die<focus%(variable.mb_rs_cost-cast_regen)*gcd" );
  st->add_action( "serpent_sting,target_if=min:remains,if=!dot.serpent_sting.ticking&target.time_to_die>7&!talent.vipers_venom" );
  st->add_action( "mongoose_bite,if=talent.alpha_predator&buff.mongoose_fury.up&buff.mongoose_fury.remains<focus%(variable.mb_rs_cost-cast_regen)*gcd" );
  st->add_action( "flanking_strike,if=focus+cast_regen<focus.max" );
  st->add_action( "coordinated_assault,if=!talent.coordinated_kill&target.health.pct<20&(!buff.spearhead.remains&cooldown.spearhead.remains|!talent.spearhead)|talent.coordinated_kill&(!buff.spearhead.remains&cooldown.spearhead.remains|!talent.spearhead)" );
  st->add_action( "wildfire_bomb,if=next_wi_bomb.pheromone&!buff.mongoose_fury.up&focus+cast_regen<focus.max-action.kill_command.cast_regen*2" );
  st->add_action( "kill_command,target_if=min:bloodseeker.remains,if=full_recharge_time<gcd&focus+cast_regen<focus.max" );
  st->add_action( "mongoose_bite,if=dot.shrapnel_bomb.ticking" );
  st->add_action( "serpent_sting,target_if=min:remains,if=refreshable&!talent.vipers_venom" );
  st->add_action( "wildfire_bomb,if=full_recharge_time<gcd&!set_bonus.tier29_2pc" );
  st->add_action( "mongoose_bite,target_if=max:debuff.latent_poison.stack,if=buff.mongoose_fury.up" );
  st->add_action( "wildfire_bomb,if=full_recharge_time<gcd" );
  st->add_action( "mongoose_bite,target_if=max:debuff.latent_poison.stack,if=focus+action.kill_command.cast_regen>focus.max-10" );
  st->add_action( "stampede" );
  st->add_action( "explosive_shot,if=talent.ranger" );
  st->add_action( "raptor_strike,target_if=max:debuff.latent_poison.stack" );
  st->add_action( "steel_trap" );
  st->add_action( "wildfire_bomb,if=!dot.wildfire_bomb.ticking" );
  st->add_action( "kill_command,target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max" );
  st->add_action( "coordinated_assault,if=!talent.coordinated_kill&time_to_die>140" );
  st->add_action( "fury_of_the_eagle,interrupt=1" );
}
//survival_df_apl_end

} // namespace hunter_apl
