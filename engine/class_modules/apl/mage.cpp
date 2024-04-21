#include "class_modules/apl/mage.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace mage_apl {

std::string potion( const player_t* p )
{
  return p->true_level >= 70 ? "elemental_potion_of_ultimate_power_3"
       : p->true_level >= 60 ? "spectral_intellect"
       : p->true_level >= 50 ? "superior_battle_potion_of_intellect"
       :                       "disabled";
}

std::string flask( const player_t* p )
{
  return p->true_level >= 70 ? "phial_of_tepid_versatility_3"
       : p->true_level >= 60 ? "spectral_flask_of_power"
       : p->true_level >= 50 ? "greater_flask_of_endless_fathoms"
       :                       "disabled";
}

std::string food( const player_t* p )
{
  return p->true_level >= 70 ? "fated_fortune_cookie"
       : p->true_level >= 60 ? "feast_of_gluttonous_hedonism"
       : p->true_level >= 50 ? "famine_evaluator_and_snack_table"
       :                       "disabled";
}

std::string rune( const player_t* p )
{
  return p->true_level >= 70 ? "draconic"
       : p->true_level >= 60 ? "veiled"
       : p->true_level >= 50 ? "battle_scarred"
       :                       "disabled";
}

std::string temporary_enchant( const player_t* p )
{
  std::string lvl70_temp_enchant = p->specialization() == MAGE_FIRE ? "main_hand:howling_rune_3" : "main_hand:buzzing_rune_3";

  return p->true_level >= 70 ? lvl70_temp_enchant
       : p->true_level >= 60 ? "main_hand:shadowcore_oil"
       :                       "disabled";
}

//arcane_apl_start
void arcane( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe_cooldown_phase = p->get_action_priority_list( "aoe_cooldown_phase" );
  action_priority_list_t* aoe_rotation = p->get_action_priority_list( "aoe_rotation" );
  action_priority_list_t* cooldown_phase = p->get_action_priority_list( "cooldown_phase" );
  action_priority_list_t* rotation = p->get_action_priority_list( "rotation" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "arcane_familiar" );
  precombat->add_action( "conjure_mana_gem" );
  precombat->add_action( "variable,name=aoe_target_count,op=reset,default=3" );
  precombat->add_action( "variable,name=aoe_target_count,op=set,value=9,if=!talent.arcing_cleave" );
  precombat->add_action( "variable,name=aoe_target_count,op=set,value=5,if=talent.arcing_cleave&(!talent.orb_barrage|!talent.arcane_bombardment)" );
  precombat->add_action( "variable,name=opener,op=set,value=1" );
  precombat->add_action( "variable,name=steroid_trinket_equipped,op=set,value=equipped.gladiators_badge|equipped.irideus_fragment|equipped.spoils_of_neltharus|equipped.timebreaching_talon|equipped.ashes_of_the_embersoul|equipped.nymues_unraveling_spindle", "Variable indicates use of a trinket that boosts stats during burst" );
  precombat->add_action( "variable,name=talon_double_on_use,op=set,value=((equipped.ashes_of_the_embersoul&equipped.timebreaching_talon)|(equipped.irideus_fragment&equipped.timebreaching_talon))", "Variable indicates double on use trinket setups" );
  precombat->add_action( "variable,name=nymues_double_on_use,op=set,value=((equipped.ashes_of_the_embersoul&equipped.nymues_unraveling_spindle)|(equipped.irideus_fragment&equipped.nymues_unraveling_spindle)|(equipped.balefire_branch&equipped.nymues_unraveling_spindle)|(equipped.mirror_of_fractured_tomorrows&equipped.nymues_unraveling_spindle))" );
  precombat->add_action( "variable,name=ashes_double_on_use,op=set,value=(equipped.irideus_fragment&equipped.ashes_of_the_embersoul)" );
  precombat->add_action( "variable,name=belor_extended_opener,default=0,op=set,if=variable.belor_extended_opener=1,value=equipped.belorrelos_the_suncaller" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "mirror_image" );
  precombat->add_action( "time_warp,if=!talent.siphon_storm|(variable.belor_extended_opener&time_to_bloodlust>10)" );
  precombat->add_action( "arcane_blast,if=!talent.siphon_storm|(variable.belor_extended_opener&time_to_bloodlust>10)" );
  precombat->add_action( "evocation,if=talent.siphon_storm&!(variable.belor_extended_opener&time_to_bloodlust>10)" );

  default_->add_action( "counterspell" );
  default_->add_action( "potion,if=buff.siphon_storm.up|(!talent.siphon_storm&cooldown.arcane_surge.ready)" );
  default_->add_action( "time_warp,if=talent.temporal_warp&buff.exhaustion.up&(cooldown.arcane_surge.ready|fight_remains<=40|(buff.arcane_surge.up&fight_remains<=(cooldown.arcane_surge.remains+14)))" );
  default_->add_action( "lights_judgment,if=buff.arcane_surge.down&debuff.touch_of_the_magi.down&active_enemies>=2" );
  default_->add_action( "berserking,if=(prev_gcd.1.arcane_surge&!(buff.temporal_warp.up&buff.bloodlust.up))|(buff.arcane_surge.up&debuff.touch_of_the_magi.up)", "Zerking gets used after surge outside of double lust, but after touch with double lust" );
  default_->add_action( "blood_fury,if=prev_gcd.1.arcane_surge" );
  default_->add_action( "fireblood,if=prev_gcd.1.arcane_surge" );
  default_->add_action( "ancestral_call,if=prev_gcd.1.arcane_surge" );
  default_->add_action( "invoke_external_buff,name=power_infusion,if=((!talent.radiant_spark&prev_gcd.1.arcane_surge)|(talent.radiant_spark&prev_gcd.1.radiant_spark&cooldown.arcane_surge.remains<=(gcd.max*3)))", "PI/Summer after Radiant Spark when cooldowns are coming up, Autumn after Touch of the Magi cd starts" );
  default_->add_action( "invoke_external_buff,name=blessing_of_summer,if=(!talent.radiant_spark&prev_gcd.1.arcane_surge)|(talent.radiant_spark&prev_gcd.1.radiant_spark&cooldown.arcane_surge.remains<=(gcd.max*3))" );
  default_->add_action( "invoke_external_buff,name=blessing_of_autumn,if=cooldown.touch_of_the_magi.remains>5" );
  default_->add_action( "use_item,name=irideus_fragment,if=cooldown.arcane_surge.ready&prev_gcd.1.nether_tempest|fight_remains<=20", "Use trinkets in single target after surge without t30, after touch with t30, and before Surge in AOE, except 20-second trinkets which are used with spark without t30. Non-steroid trinkets are used whenever you don't have cooldowns active and double steroid trinkets are used in order of power level in sims with max ilevel." );
  default_->add_action( "use_item,name=ashes_of_the_embersoul,if=(prev_gcd.1.arcane_surge&!equipped.belorrelos_the_suncaller&(!variable.ashes_double_on_use|!variable.opener))|fight_remains<=20|((active_enemies>=variable.aoe_target_count)&cooldown.arcane_surge.ready&prev_gcd.1.nether_tempest)|(equipped.belorrelos_the_suncaller&(buff.arcane_surge.remains>12|(prev_gcd.1.arcane_surge&variable.opener))&cooldown.evocation.remains>60)" );
  default_->add_action( "use_item,name=nymues_unraveling_spindle,if=((!variable.opener&cooldown.arcane_surge.remains<=(gcd.max*4)&cooldown.radiant_spark.ready)|(variable.opener&(mana.pct<=10|buff.siphon_storm.remains<19))&(!variable.nymues_double_on_use|!variable.opener))|fight_remains<=24|((active_enemies>=variable.aoe_target_count)&cooldown.arcane_surge.ready&prev_gcd.1.nether_tempest)|((equipped.belorrelos_the_suncaller)&cooldown.touch_of_the_magi.remains<(gcd.max*6))" );
  default_->add_action( "use_item,name=timebreaching_talon,if=((cooldown.arcane_surge.remains<=(gcd.max*4)&cooldown.radiant_spark.remains)&(!variable.talon_double_on_use|!variable.opener))|fight_remains<=20|((active_enemies>=variable.aoe_target_count)&cooldown.arcane_surge.ready&prev_gcd.1.nether_tempest)" );
  default_->add_action( "use_items,if=prev_gcd.1.arcane_surge|((active_enemies>=variable.aoe_target_count)&cooldown.arcane_surge.ready&prev_gcd.1.nether_tempest)|fight_remains<=15" );
  default_->add_action( "use_item,name=tinker_breath_of_neltharion,if=cooldown.arcane_surge.remains&buff.arcane_surge.down&debuff.touch_of_the_magi.down" );
  default_->add_action( "use_item,name=belorrelos_the_suncaller,if=!variable.steroid_trinket_equipped|buff.siphon_storm.down|equipped.nymues_unraveling_spindle" );
  default_->add_action( "use_item,name=beacon_to_the_beyond,if=!variable.steroid_trinket_equipped|(buff.siphon_storm.down&buff.arcane_surge.remains<10)" );
  default_->add_action( "use_item,name=iceblood_deathsnare,if=!variable.steroid_trinket_equipped|buff.siphon_storm.down" );
  default_->add_action( "use_item,name=desperate_invokers_codex,if=!variable.steroid_trinket_equipped|buff.siphon_storm.down" );
  default_->add_action( "use_item,name=conjured_chillglobe,if=mana.pct>65&(!variable.steroid_trinket_equipped|buff.siphon_storm.down)" );
  default_->add_action( "use_item,name=darkmoon_deck_rime,if=!variable.steroid_trinket_equipped|buff.siphon_storm.down" );
  default_->add_action( "use_item,name=darkmoon_deck_dance,if=!variable.steroid_trinket_equipped|buff.siphon_storm.down" );
  default_->add_action( "use_item,name=darkmoon_deck_inferno,if=!variable.steroid_trinket_equipped|buff.siphon_storm.down" );
  default_->add_action( "use_item,name=dreambinder_loom_of_the_great_cycle" );
  default_->add_action( "use_item,name=iridal_the_earths_master,use_off_gcd=1,if=gcd.remains" );
  default_->add_action( "variable,name=aoe_cooldown_phase,op=set,value=1,if=active_enemies>=variable.aoe_target_count&(action.arcane_orb.charges>0|buff.arcane_charge.stack>=3)&(cooldown.radiant_spark.ready|!talent.radiant_spark)&(cooldown.touch_of_the_magi.remains<=(gcd.max*2)|!talent.touch_of_the_magi)" );
  default_->add_action( "variable,name=aoe_cooldown_phase,op=set,value=0,if=variable.aoe_cooldown_phase&((debuff.radiant_spark_vulnerability.down&dot.radiant_spark.remains<7&cooldown.radiant_spark.remains)|!talent.radiant_spark&debuff.touch_of_the_magi.up)" );
  default_->add_action( "variable,name=opener,op=set,if=debuff.touch_of_the_magi.up&variable.opener,value=0" );
  default_->add_action( "variable,name=blast_below_gcd,op=set,value=action.arcane_blast.cast_time<gcd.max" );
  default_->add_action( "cancel_action,if=action.evocation.channeling&mana.pct>=95&!talent.siphon_storm", "Cancel Evo if we have enough mana and don't have Siphon Storm talented or if the fight duration is running out" );
  default_->add_action( "cancel_action,if=action.evocation.channeling&(mana.pct>fight_remains*4)&!(fight_remains>10&cooldown.arcane_surge.remains<1)" );
  default_->add_action( "arcane_barrage,if=fight_remains<2" );
  default_->add_action( "evocation,if=buff.arcane_surge.down&debuff.touch_of_the_magi.down&((mana.pct<10&cooldown.touch_of_the_magi.remains<20)|cooldown.touch_of_the_magi.remains<15)&((buff.bloodlust.remains<31&buff.bloodlust.up)|!variable.belor_extended_opener|!variable.opener)" );
  default_->add_action( "conjure_mana_gem,if=debuff.touch_of_the_magi.down&buff.arcane_surge.down&cooldown.arcane_surge.remains<30&cooldown.arcane_surge.remains<fight_remains&!mana_gem_charges", "Make a new gem if the encounter is long enough and use it after surge to recoup mana quickly" );
  default_->add_action( "use_mana_gem,if=talent.cascading_power&buff.clearcasting.stack<2&buff.arcane_surge.up" );
  default_->add_action( "use_mana_gem,if=!talent.cascading_power&prev_gcd.1.arcane_surge" );
  default_->add_action( "call_action_list,name=cooldown_phase,if=(cooldown.arcane_surge.remains<=(gcd.max*(1+(talent.nether_tempest&talent.arcane_echo)))|(buff.arcane_surge.remains>(3*(set_bonus.tier30_2pc&!set_bonus.tier30_4pc)))|buff.arcane_overload.up)&cooldown.evocation.remains>45&((cooldown.touch_of_the_magi.remains<gcd.max*4)|cooldown.touch_of_the_magi.remains>20)&active_enemies<variable.aoe_target_count", "Enter cooldown phase when cds are available or coming off cooldown otherwise default to rotation priority" );
  default_->add_action( "call_action_list,name=cooldown_phase,if=cooldown.arcane_surge.remains>30&(cooldown.radiant_spark.ready|dot.radiant_spark.remains|debuff.radiant_spark_vulnerability.up)&(cooldown.touch_of_the_magi.remains<=(gcd.max*3)|debuff.touch_of_the_magi.up)&active_enemies<variable.aoe_target_count" );
  default_->add_action( "call_action_list,name=aoe_cooldown_phase,if=variable.aoe_cooldown_phase&(cooldown.arcane_surge.remains<(gcd.max*4)|cooldown.arcane_surge.remains>40)" );
  default_->add_action( "call_action_list,name=aoe_rotation,if=active_enemies>=variable.aoe_target_count" );
  default_->add_action( "call_action_list,name=rotation" );

  aoe_cooldown_phase->add_action( "cancel_buff,name=presence_of_mind,if=prev_gcd.1.arcane_blast&cooldown.arcane_surge.remains>75" );
  aoe_cooldown_phase->add_action( "touch_of_the_magi,use_off_gcd=1,if=prev_gcd.1.arcane_barrage" );
  aoe_cooldown_phase->add_action( "radiant_spark" );
  aoe_cooldown_phase->add_action( "arcane_orb,if=buff.arcane_charge.stack<3,line_cd=1" );
  aoe_cooldown_phase->add_action( "nether_tempest,if=talent.arcane_echo,line_cd=30" );
  aoe_cooldown_phase->add_action( "arcane_surge" );
  aoe_cooldown_phase->add_action( "wait,sec=0.05,if=cooldown.arcane_surge.remains>75&prev_gcd.1.arcane_blast&!talent.presence_of_mind,line_cd=15", "Waits are used to simulate players allowing radiant spark to increment in stacks" );
  aoe_cooldown_phase->add_action( "wait,sec=0.05,if=prev_gcd.1.arcane_surge,line_cd=15" );
  aoe_cooldown_phase->add_action( "wait,sec=0.05,if=cooldown.arcane_surge.remains<75&debuff.radiant_spark_vulnerability.stack=3&!talent.presence_of_mind,line_cd=15" );
  aoe_cooldown_phase->add_action( "arcane_barrage,if=cooldown.arcane_surge.remains<75&debuff.radiant_spark_vulnerability.stack=4&!talent.orb_barrage" );
  aoe_cooldown_phase->add_action( "arcane_barrage,if=(debuff.radiant_spark_vulnerability.stack=2&cooldown.arcane_surge.remains>75)|(debuff.radiant_spark_vulnerability.stack=1&cooldown.arcane_surge.remains<75)&!talent.orb_barrage" );
  aoe_cooldown_phase->add_action( "arcane_barrage,if=(debuff.radiant_spark_vulnerability.stack=1|debuff.radiant_spark_vulnerability.stack=2|(debuff.radiant_spark_vulnerability.stack=3&active_enemies>5)|debuff.radiant_spark_vulnerability.stack=4)&buff.arcane_charge.stack=buff.arcane_charge.max_stack&talent.orb_barrage", "Optimize orb barrage procs during spark at the cost of vulnerabilities, except at 5 or fewer targets where you arcane blast on the 3rd spark stack if its up and you have charges" );
  aoe_cooldown_phase->add_action( "presence_of_mind" );
  aoe_cooldown_phase->add_action( "arcane_blast,if=((debuff.radiant_spark_vulnerability.stack=2|debuff.radiant_spark_vulnerability.stack=3)&!talent.orb_barrage)|(debuff.radiant_spark_vulnerability.remains&talent.orb_barrage)" );
  aoe_cooldown_phase->add_action( "arcane_barrage,if=(debuff.radiant_spark_vulnerability.stack=4&buff.arcane_surge.up)|(debuff.radiant_spark_vulnerability.stack=3&buff.arcane_surge.down)&!talent.orb_barrage" );

  aoe_rotation->add_action( "shifting_power,if=(!talent.evocation|cooldown.evocation.remains>12)&(!talent.arcane_surge|cooldown.arcane_surge.remains>12)&(!talent.touch_of_the_magi|cooldown.touch_of_the_magi.remains>12)&buff.arcane_surge.down&((!talent.charged_orb&cooldown.arcane_orb.remains>12)|(action.arcane_orb.charges=0|cooldown.arcane_orb.remains>12))&!debuff.touch_of_the_magi.up" );
  aoe_rotation->add_action( "nether_tempest,if=(refreshable|!ticking)&buff.arcane_charge.stack=buff.arcane_charge.max_stack&buff.arcane_surge.down&(active_enemies>6|!talent.orb_barrage)&!debuff.touch_of_the_magi.up" );
  aoe_rotation->add_action( "arcane_missiles,if=buff.arcane_artillery.up&(cooldown.touch_of_the_magi.remains+5)>buff.arcane_artillery.remains" );
  aoe_rotation->add_action( "arcane_barrage,if=(active_enemies<=4&buff.arcane_charge.stack=3)|buff.arcane_charge.stack=buff.arcane_charge.max_stack|mana.pct<9" );
  aoe_rotation->add_action( "arcane_orb,if=buff.arcane_charge.stack<2&cooldown.touch_of_the_magi.remains>18" );
  aoe_rotation->add_action( "arcane_explosion" );

  cooldown_phase->add_action( "touch_of_the_magi,use_off_gcd=1,if=prev_gcd.1.arcane_barrage" );
  cooldown_phase->add_action( "shifting_power,if=buff.arcane_surge.down&!talent.radiant_spark" );
  cooldown_phase->add_action( "arcane_orb,if=(cooldown.radiant_spark.ready|(active_enemies>=2&debuff.radiant_spark_vulnerability.down))&buff.arcane_charge.stack<buff.arcane_charge.max_stack" );
  cooldown_phase->add_action( "arcane_missiles,if=variable.opener&buff.clearcasting.react&buff.clearcasting.stack>0&cooldown.radiant_spark.remains<5&buff.nether_precision.down&(!buff.arcane_artillery.up|buff.arcane_artillery.remains<=(gcd.max*6)),interrupt_if=!gcd.remains&mana.pct>30&buff.nether_precision.up&!buff.arcane_artillery.up,interrupt_immediate=1,interrupt_global=1,chain=1" );
  cooldown_phase->add_action( "arcane_blast,if=variable.opener&cooldown.arcane_surge.ready&mana.pct>10&buff.siphon_storm.remains>17&!set_bonus.tier30_4pc" );
  cooldown_phase->add_action( "arcane_missiles,if=cooldown.radiant_spark.ready&buff.clearcasting.react&(talent.nether_precision&(buff.nether_precision.down|buff.nether_precision.remains<gcd.max*3))&(!buff.arcane_artillery.up|buff.arcane_artillery.remains<=(gcd.max*6)),interrupt_if=!gcd.remains&mana.pct>30&buff.nether_precision.up&!buff.arcane_artillery.up,interrupt_immediate=1,interrupt_global=1,chain=1" );
  cooldown_phase->add_action( "radiant_spark" );
  cooldown_phase->add_action( "nether_tempest,if=talent.arcane_echo,line_cd=30" );
  cooldown_phase->add_action( "arcane_surge" );
  cooldown_phase->add_action( "wait,sec=0.05,if=prev_gcd.1.arcane_surge,line_cd=15" );
  cooldown_phase->add_action( "arcane_barrage,if=prev_gcd.1.arcane_surge|prev_gcd.1.nether_tempest|prev_gcd.1.radiant_spark|(active_enemies>=(4-(2*talent.orb_barrage))&debuff.radiant_spark_vulnerability.stack=4&talent.arcing_cleave)" );
  cooldown_phase->add_action( "arcane_blast,if=debuff.radiant_spark_vulnerability.stack>0&(debuff.radiant_spark_vulnerability.stack<4|(variable.blast_below_gcd&debuff.radiant_spark_vulnerability.stack=4))" );
  cooldown_phase->add_action( "presence_of_mind,if=debuff.touch_of_the_magi.remains<=gcd.max" );
  cooldown_phase->add_action( "arcane_blast,if=buff.presence_of_mind.up" );
  cooldown_phase->add_action( "arcane_missiles,if=((buff.nether_precision.down&buff.clearcasting.react)|(buff.clearcasting.stack>2&debuff.touch_of_the_magi.up))&(debuff.radiant_spark_vulnerability.down|(debuff.radiant_spark_vulnerability.stack=4&prev_gcd.1.arcane_blast)),interrupt_if=!gcd.remains&mana.pct>30&buff.nether_precision.up&!buff.arcane_artillery.up,interrupt_immediate=1,interrupt_global=1,chain=1" );
  cooldown_phase->add_action( "arcane_blast" );

  rotation->add_action( "arcane_orb,if=buff.arcane_charge.stack<3&(buff.bloodlust.down|mana.pct>70)" );
  rotation->add_action( "shifting_power,if=buff.arcane_surge.down&cooldown.arcane_surge.remains>45&fight_remains>15" );
  rotation->add_action( "nether_tempest,if=(refreshable|!ticking)&buff.arcane_charge.stack=buff.arcane_charge.max_stack&(((buff.temporal_warp.up|mana.pct<10|!talent.shifting_power)&buff.arcane_surge.down)|equipped.neltharions_call_to_chaos)&!variable.opener&fight_remains>=12" );
  rotation->add_action( "arcane_barrage,if=buff.arcane_charge.stack=buff.arcane_charge.max_stack&mana.pct<70&(((cooldown.arcane_surge.remains>30&cooldown.touch_of_the_magi.remains>10)&buff.bloodlust.up&cooldown.touch_of_the_magi.remains>5&fight_remains>30)|(!talent.evocation&fight_remains>20))" );
  rotation->add_action( "presence_of_mind,if=buff.arcane_charge.stack<3&target.health.pct<35&talent.arcane_bombardment" );
  rotation->add_action( "arcane_blast,if=(buff.arcane_charge.stack=buff.arcane_charge.max_stack&buff.nether_precision.up)|(talent.time_anomaly&buff.arcane_surge.up&buff.arcane_surge.remains<=6)" );
  rotation->add_action( "arcane_missiles,if=buff.clearcasting.react&buff.nether_precision.down&(!variable.opener|(equipped.belorrelos_the_suncaller&variable.steroid_trinket_equipped)),interrupt_if=!gcd.remains&buff.nether_precision.up&mana.pct>30&!buff.arcane_artillery.up,interrupt_immediate=1,interrupt_global=1,chain=1" );
  rotation->add_action( "arcane_blast" );
  rotation->add_action( "arcane_barrage" );
}
//arcane_apl_end

//fire_apl_start
void fire( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* active_talents = p->get_action_priority_list( "active_talents" );
  action_priority_list_t* combustion_cooldowns = p->get_action_priority_list( "combustion_cooldowns" );
  action_priority_list_t* combustion_phase = p->get_action_priority_list( "combustion_phase" );
  action_priority_list_t* combustion_timing = p->get_action_priority_list( "combustion_timing" );
  action_priority_list_t* firestarter_fire_blasts = p->get_action_priority_list( "firestarter_fire_blasts" );
  action_priority_list_t* standard_rotation = p->get_action_priority_list( "standard_rotation" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "variable,name=steroid_trinket_equipped,op=set,value=equipped.gladiators_badge|equipped.irideus_fragment|equipped.erupting_spear_fragment|equipped.spoils_of_neltharus|equipped.timebreaching_talon|equipped.horn_of_valor|equipped.mirror_of_fractured_tomorrows|equipped.ashes_of_the_embersoul|equipped.balefire_branch|equipped.time_theifs_gambit|equipped.nymues_unraveling_spindle", "defining a group of trinkets as Steroids" );
  precombat->add_action( "variable,name=disable_combustion,op=reset", "APL Variable Option: If set to a non-zero value, the Combustion action and cooldowns that are constrained to only be used when Combustion is up will not be used during the simulation." );
  precombat->add_action( "variable,name=firestarter_combustion,default=-1,value=talent.sun_kings_blessing,if=variable.firestarter_combustion<0", "APL Variable Option: This variable specifies whether Combustion should be used during Firestarter." );
  precombat->add_action( "variable,name=hot_streak_flamestrike,if=variable.hot_streak_flamestrike=0,value=4*talent.flame_patch+999*!talent.flame_patch", "APL Variable Option: This variable specifies the number of targets at which Hot Streak Flamestrikes outside of Combustion should be used." );
  precombat->add_action( "variable,name=hard_cast_flamestrike,if=variable.hard_cast_flamestrike=0,value=999", "APL Variable Option: This variable specifies the number of targets at which Hard Cast Flamestrikes outside of Combustion should be used as filler." );
  precombat->add_action( "variable,name=combustion_flamestrike,if=variable.combustion_flamestrike=0,value=4*talent.flame_patch+999*!talent.flame_patch", "APL Variable Option: This variable specifies the number of targets at which Hot Streak Flamestrikes are used during Combustion." );
  precombat->add_action( "variable,name=skb_flamestrike,if=variable.skb_flamestrike=0,value=3*talent.fuel_the_fire+999*!talent.fuel_the_fire", "APL Variable Option: This variable specifies the number of targets at which Flamestrikes should be used to consume Fury of the Sun King.  Restricting this variable to be true only if Fuel the Fire is talented." );
  precombat->add_action( "variable,name=arcane_explosion,if=variable.arcane_explosion=0,value=999", "APL Variable Option: This variable specifies the number of targets at which Arcane Explosion outside of Combustion should be used." );
  precombat->add_action( "variable,name=arcane_explosion_mana,default=40,op=reset", "APL Variable Option: This variable specifies the percentage of mana below which Arcane Explosion will not be used." );
  precombat->add_action( "variable,name=combustion_shifting_power,if=variable.combustion_shifting_power=0,value=999", "APL Variable Option: The number of targets at which Shifting Power can used during Combustion." );
  precombat->add_action( "variable,name=combustion_cast_remains,default=0.3,op=reset", "APL Variable Option: The time remaining on a cast when Combustion can be used in seconds." );
  precombat->add_action( "variable,name=overpool_fire_blasts,default=0,op=reset", "APL Variable Option: This variable specifies the number of seconds of Fire Blast that should be pooled past the default amount." );
  precombat->add_action( "variable,name=time_to_combustion,value=fight_remains+100,if=variable.disable_combustion", "If Combustion is disabled, schedule the first Combustion far after the fight ends." );
  precombat->add_action( "variable,name=skb_duration,value=dbc.effect.1016075.base_value", "The duration of a Sun King's Blessing Combustion." );
  precombat->add_action( "variable,name=combustion_on_use,value=equipped.gladiators_badge|equipped.moonlit_prism|equipped.irideus_fragment|equipped.spoils_of_neltharus|equipped.timebreaching_talon|equipped.horn_of_valor", "Whether a usable item used to buff Combustion is equipped." );
  precombat->add_action( "variable,name=on_use_cutoff,value=20,if=variable.combustion_on_use", "How long before Combustion should trinkets that trigger a shared category cooldown on other trinkets not be used?" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "mirror_image" );
  precombat->add_action( "flamestrike,if=active_enemies>=variable.hot_streak_flamestrike" );
  precombat->add_action( "pyroblast" );

  default_->add_action( "counterspell" );
  default_->add_action( "call_action_list,name=combustion_timing,if=!variable.disable_combustion", "The combustion_timing action list schedules the approximate time when Combustion should be used and stores the number of seconds until then in variable.time_to_combustion." );
  default_->add_action( "time_warp,if=buff.exhaustion.up&talent.temporal_warp&(firestarter.active|interpolated_fight_remains<buff.bloodlust.duration)", "The second Time Warp is usually best used immediately on pull with Temporal Warp so that it comes off cooldown earlier; this can sometimes be slightly worse if doing so will not result in additional Time Warp(s) throughout the encounter. Additionally, it should always be used at the end of the fight regardless of Combustion being ready if it is available." );
  default_->add_action( "potion,if=buff.potion.duration>variable.time_to_combustion+buff.combustion.duration" );
  default_->add_action( "variable,name=shifting_power_before_combustion,value=variable.time_to_combustion>cooldown.shifting_power.remains", "Variable that estimates whether Shifting Power will be used before the next Combustion." );
  default_->add_action( "variable,name=item_cutoff_active,value=(variable.time_to_combustion<variable.on_use_cutoff|buff.combustion.remains>variable.skb_duration&!cooldown.item_cd_1141.remains)&((trinket.1.has_cooldown&trinket.1.cooldown.remains<variable.on_use_cutoff)+(trinket.2.has_cooldown&trinket.2.cooldown.remains<variable.on_use_cutoff)>1)" );
  default_->add_action( "use_item,effect_name=gladiators_badge,if=variable.time_to_combustion>cooldown-5" );
  default_->add_action( "use_item,name=mirror_of_fractured_tomorrows,if=buff.combustion.up&buff.combustion.remains>11", "New trinket and weapon lines  need special line for Nymue trinket which would delay combustion under certain conditions?" );
  default_->add_action( "use_item,name=timethiefs_gambit,if=buff.combustion.up" );
  default_->add_action( "use_item,name=balefire_branch,if=(variable.time_to_combustion<=3&buff.fury_of_the_sun_king.up)|(buff.combustion.up&buff.combustion.remains>11)" );
  default_->add_action( "use_item,name=ashes_of_the_embersoul,if=(variable.time_to_combustion<=3&buff.fury_of_the_sun_king.up)|(buff.combustion.up&buff.combustion.remains>11)" );
  default_->add_action( "use_item,name=nymues_unraveling_spindle,if=variable.time_to_combustion<=9" );
  default_->add_action( "use_item,name=dreambinder_loom_of_the_great_cycle" );
  default_->add_action( "use_item,name=iridal_the_earths_master,use_off_gcd=1,slot=main_hand,if=gcd.remains>=0.6*gcd.max", "Iridal can and should be used while on the GCD" );
  default_->add_action( "use_item,name=belorrelos_the_suncaller,if=(!variable.steroid_trinket_equipped&buff.combustion.down)|(variable.steroid_trinket_equipped&trinket.1.has_cooldown&trinket.1.cooldown.remains>20&buff.combustion.down)|(variable.steroid_trinket_equipped&trinket.2.has_cooldown&trinket.2.cooldown.remains>20&buff.combustion.down)" );
  default_->add_action( "use_items,if=!variable.item_cutoff_active" );
  default_->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=fire_blast_pooling,value=buff.combustion.down&action.fire_blast.charges_fractional+(variable.time_to_combustion+action.shifting_power.full_reduction*variable.shifting_power_before_combustion)%cooldown.fire_blast.duration-1<cooldown.fire_blast.max_charges+variable.overpool_fire_blasts%cooldown.fire_blast.duration-(buff.combustion.duration%cooldown.fire_blast.duration)%%1&variable.time_to_combustion<fight_remains", "Pool as many Fire Blasts as possible for Combustion." );
  default_->add_action( "call_action_list,name=combustion_phase,if=variable.time_to_combustion<=0|buff.combustion.up|variable.time_to_combustion<variable.combustion_precast_time&cooldown.combustion.remains<variable.combustion_precast_time" );
  default_->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=fire_blast_pooling,value=searing_touch.active&action.fire_blast.full_recharge_time>3*gcd.max,if=!variable.fire_blast_pooling&talent.sun_kings_blessing", "Adjust the variable that controls Fire Blast usage to save Fire Blasts while Searing Touch is active with Sun King's Blessing." );
  default_->add_action( "shifting_power,if=buff.combustion.down&(action.fire_blast.charges=0|variable.fire_blast_pooling)&(!improved_scorch.active|debuff.improved_scorch.remains>cast_time+action.scorch.cast_time&!buff.fury_of_the_sun_king.up)&!buff.hot_streak.react&variable.shifting_power_before_combustion" );
  default_->add_action( "variable,name=phoenix_pooling,if=active_enemies<variable.combustion_flamestrike,value=(variable.time_to_combustion+buff.combustion.duration-5<action.phoenix_flames.full_recharge_time+cooldown.phoenix_flames.duration-action.shifting_power.full_reduction*variable.shifting_power_before_combustion&variable.time_to_combustion<fight_remains|talent.sun_kings_blessing)&!talent.alexstraszas_fury", "Variable that controls Phoenix Flames usage to ensure its charges are pooled for Combustion when needed. Only use Phoenix Flames outside of Combustion when full charges can be obtained during the next Combustion." );
  default_->add_action( "variable,name=phoenix_pooling,if=active_enemies>=variable.combustion_flamestrike,value=(variable.time_to_combustion<action.phoenix_flames.full_recharge_time-action.shifting_power.full_reduction*variable.shifting_power_before_combustion&variable.time_to_combustion<fight_remains|talent.sun_kings_blessing)&!talent.alexstraszas_fury", "When using Flamestrike in Combustion, save as many charges as possible for Combustion without capping." );
  default_->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=!variable.fire_blast_pooling&variable.time_to_combustion>0&active_enemies>=variable.hard_cast_flamestrike&!firestarter.active&!buff.hot_streak.react&(buff.heating_up.react&action.flamestrike.execute_remains<0.5|charges_fractional>=2)", "When Hardcasting Flamestrike, Fire Blasts should be used to generate Hot Streaks and to extend Feel the Burn." );
  default_->add_action( "call_action_list,name=firestarter_fire_blasts,if=buff.combustion.down&firestarter.active&variable.time_to_combustion>0" );
  default_->add_action( "fire_blast,use_while_casting=1,if=action.shifting_power.executing&full_recharge_time<action.shifting_power.tick_reduction", "Avoid capping Fire Blast charges while channeling Shifting Power" );
  default_->add_action( "call_action_list,name=standard_rotation,if=variable.time_to_combustion>0&buff.combustion.down" );
  default_->add_action( "ice_nova,if=!searing_touch.active", "Ice Nova can be used during movement when Searing Touch is not active." );
  default_->add_action( "scorch" );

  active_talents->add_action( "living_bomb,if=active_enemies>1&buff.combustion.down&(variable.time_to_combustion>cooldown.living_bomb.duration|variable.time_to_combustion<=0)" );
  active_talents->add_action( "meteor,if=variable.time_to_combustion<=0|buff.combustion.remains>travel_time|!talent.sun_kings_blessing&(cooldown.meteor.duration<variable.time_to_combustion|fight_remains<variable.time_to_combustion)" );
  active_talents->add_action( "dragons_breath,if=talent.alexstraszas_fury&(buff.combustion.down&!buff.hot_streak.react)&(buff.feel_the_burn.up|time>15)&(!improved_scorch.active)&!firestarter.remains&!talent.tempered_flames", "With Alexstrasza's Fury when Combustion is not active, Dragon's Breath should be used to convert Heating Up to a Hot Streak." );
  active_talents->add_action( "dragons_breath,if=talent.alexstraszas_fury&(buff.combustion.down&!buff.hot_streak.react)&(buff.feel_the_burn.up|time>15)&(!improved_scorch.active)&talent.tempered_flames" );

  combustion_cooldowns->add_action( "potion" );
  combustion_cooldowns->add_action( "blood_fury" );
  combustion_cooldowns->add_action( "berserking,if=buff.combustion.up" );
  combustion_cooldowns->add_action( "fireblood" );
  combustion_cooldowns->add_action( "ancestral_call" );
  combustion_cooldowns->add_action( "invoke_external_buff,name=power_infusion,if=buff.power_infusion.down" );
  combustion_cooldowns->add_action( "invoke_external_buff,name=blessing_of_summer,if=buff.blessing_of_summer.down" );
  combustion_cooldowns->add_action( "time_warp,if=talent.temporal_warp&buff.exhaustion.up" );
  combustion_cooldowns->add_action( "use_item,effect_name=gladiators_badge" );
  combustion_cooldowns->add_action( "use_item,name=irideus_fragment" );
  combustion_cooldowns->add_action( "use_item,name=spoils_of_neltharus" );
  combustion_cooldowns->add_action( "use_item,name=tome_of_unstable_power" );
  combustion_cooldowns->add_action( "use_item,name=timebreaching_talon" );
  combustion_cooldowns->add_action( "use_item,name=voidmenders_shadowgem" );
  combustion_cooldowns->add_action( "use_item,name=horn_of_valor" );
  combustion_cooldowns->add_action( "use_item,name=timethiefs_gambit" );
  combustion_cooldowns->add_action( "use_item,name=balefire_branch" );
  combustion_cooldowns->add_action( "use_item,name=ashes_of_the_embersoul" );
  combustion_cooldowns->add_action( "use_item,name=mirror_of_fractured_tomorrows" );

  combustion_phase->add_action( "lights_judgment,if=buff.combustion.down" );
  combustion_phase->add_action( "bag_of_tricks,if=buff.combustion.down" );
  combustion_phase->add_action( "living_bomb,if=active_enemies>1&buff.combustion.down" );
  combustion_phase->add_action( "call_action_list,name=combustion_cooldowns,if=buff.combustion.remains>variable.skb_duration|fight_remains<20", "Other cooldowns that should be used with Combustion should only be used with an actual Combustion cast and not with a Sun King's Blessing proc." );
  combustion_phase->add_action( "use_item,name=hyperthread_wristwraps,if=hyperthread_wristwraps.fire_blast>=2&action.fire_blast.charges=0" );
  combustion_phase->add_action( "use_item,name=neural_synapse_enhancer,if=variable.time_to_combustion>60" );
  combustion_phase->add_action( "phoenix_flames,if=buff.combustion.down&set_bonus.tier30_2pc&!action.phoenix_flames.in_flight&debuff.charring_embers.remains<4*gcd.max&!buff.hot_streak.react", "In Charring embers is not up before Combustion, make sure to apply it." );
  combustion_phase->add_action( "call_action_list,name=active_talents" );
  combustion_phase->add_action( "flamestrike,if=buff.combustion.down&buff.fury_of_the_sun_king.up&buff.fury_of_the_sun_king.remains>cast_time&buff.fury_of_the_sun_king.expiration_delay_remains=0&cooldown.combustion.remains<cast_time&active_enemies>=variable.skb_flamestrike", "If Combustion is down, precast something before activating it." );
  combustion_phase->add_action( "pyroblast,if=buff.combustion.down&buff.fury_of_the_sun_king.up&buff.fury_of_the_sun_king.remains>cast_time&buff.fury_of_the_sun_king.expiration_delay_remains=0" );
  combustion_phase->add_action( "fireball,if=buff.combustion.down&cooldown.combustion.remains<cast_time&active_enemies<2&!improved_scorch.active" );
  combustion_phase->add_action( "scorch,if=buff.combustion.down&cooldown.combustion.remains<cast_time" );
  combustion_phase->add_action( "combustion,use_off_gcd=1,use_while_casting=1,if=hot_streak_spells_in_flight=0&buff.combustion.down&variable.time_to_combustion<=0&(action.scorch.executing&action.scorch.execute_remains<variable.combustion_cast_remains|action.fireball.executing&action.fireball.execute_remains<variable.combustion_cast_remains|action.pyroblast.executing&action.pyroblast.execute_remains<variable.combustion_cast_remains|action.flamestrike.executing&action.flamestrike.execute_remains<variable.combustion_cast_remains|action.meteor.in_flight&action.meteor.in_flight_remains<variable.combustion_cast_remains)", "Combustion should be used when the precast is almost finished or when Meteor is about to land." );
  combustion_phase->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=!variable.fire_blast_pooling&(!improved_scorch.active|action.scorch.executing|debuff.improved_scorch.remains>4*gcd.max)&(buff.fury_of_the_sun_king.down|action.pyroblast.executing)&buff.combustion.up&!buff.hyperthermia.react&!buff.hot_streak.react&hot_streak_spells_in_flight+buff.heating_up.react*(gcd.remains>0)<2", "Without Feel the Burn, just use Fire Blasts when they won't munch crits and when Hyperthermia is down." );
  combustion_phase->add_action( "flamestrike,if=(buff.hot_streak.react&active_enemies>=variable.combustion_flamestrike)|(buff.hyperthermia.react&active_enemies>=variable.combustion_flamestrike-talent.hyperthermia)", "Spend Hot Streaks during Combustion at high priority." );
  combustion_phase->add_action( "pyroblast,if=buff.hyperthermia.react" );
  combustion_phase->add_action( "pyroblast,if=buff.hot_streak.react&buff.combustion.up" );
  combustion_phase->add_action( "pyroblast,if=prev_gcd.1.scorch&buff.heating_up.react&active_enemies<variable.combustion_flamestrike&buff.combustion.up" );
  combustion_phase->add_action( "shifting_power,if=buff.combustion.up&!action.fire_blast.charges&(action.phoenix_flames.charges<action.phoenix_flames.max_charges|talent.alexstraszas_fury)&variable.combustion_shifting_power<=active_enemies", "Using Shifting Power during Combustion to restore Fire Blast and Phoenix Flame charges can be beneficial, but usually only on AoE." );
  combustion_phase->add_action( "flamestrike,if=buff.fury_of_the_sun_king.up&buff.fury_of_the_sun_king.remains>cast_time&active_enemies>=variable.skb_flamestrike&buff.fury_of_the_sun_king.expiration_delay_remains=0", "Spend Fury of the Sun King procs." );
  combustion_phase->add_action( "pyroblast,if=buff.fury_of_the_sun_king.up&buff.fury_of_the_sun_king.remains>cast_time&buff.fury_of_the_sun_king.expiration_delay_remains=0" );
  combustion_phase->add_action( "scorch,if=improved_scorch.active&(debuff.improved_scorch.remains<4*gcd.max)&active_enemies<variable.combustion_flamestrike", "Maintain Improved Scorch when not casting Flamestrikes in Combustion." );
  combustion_phase->add_action( "phoenix_flames,if=set_bonus.tier30_2pc&travel_time<buff.combustion.remains&buff.heating_up.react+hot_streak_spells_in_flight<2&(debuff.charring_embers.remains<4*gcd.max|buff.flames_fury.stack>1|buff.flames_fury.up)", "With the T30 set, Phoenix Flames should be used to maintain Charring Embers during Combustion and Flame's Fury procs should be spent." );
  combustion_phase->add_action( "fireball,if=buff.combustion.remains>cast_time&buff.flame_accelerant.react" );
  combustion_phase->add_action( "phoenix_flames,if=(!talent.alexstraszas_fury&travel_time<buff.combustion.remains|talent.alexstraszas_fury)&buff.heating_up.react+hot_streak_spells_in_flight<2", "Use Phoenix Flames and Scorch in Combustion to help generate Hot Streaks when Fire Blasts are not available or need to be conserved." );
  combustion_phase->add_action( "scorch,if=buff.combustion.remains>cast_time&cast_time>=gcd.max" );
  combustion_phase->add_action( "fireball,if=buff.combustion.remains>cast_time" );
  combustion_phase->add_action( "living_bomb,if=buff.combustion.remains<gcd.max&active_enemies>1", "If there isn't enough time left in Combustion for a Phoenix Flames or Scorch to hit inside of Combustion, use something else." );

  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=combustion_ready_time,value=cooldown.combustion.remains*expected_kindling_reduction", "Helper variable that contains the actual estimated time that the next Combustion will be ready." );
  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=combustion_precast_time,value=action.fireball.cast_time*(active_enemies<variable.combustion_flamestrike)+action.flamestrike.cast_time*(active_enemies>=variable.combustion_flamestrike)-variable.combustion_cast_remains", "The cast time of the spell that will be precast into Combustion." );
  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=time_to_combustion,value=variable.combustion_ready_time" );
  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=time_to_combustion,op=max,value=firestarter.remains,if=talent.firestarter&!variable.firestarter_combustion", "Delay Combustion for after Firestarter unless variable.firestarter_combustion is set." );
  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=time_to_combustion,op=max,value=(buff.sun_kings_blessing.max_stack-buff.sun_kings_blessing.stack)*(3*gcd.max),if=talent.sun_kings_blessing&firestarter.active&buff.fury_of_the_sun_king.down", "Delay Combustion until SKB is ready during Firestarter" );
  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=time_to_combustion,op=max,value=cooldown.gladiators_badge_345228.remains,if=equipped.gladiators_badge&cooldown.gladiators_badge_345228.remains-20<variable.time_to_combustion", "Delay Combustion for Gladiators Badge, unless it would be delayed longer than 20 seconds." );
  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=time_to_combustion,op=max,value=buff.combustion.remains", "Delay Combustion until Combustion expires if it's up." );
  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=time_to_combustion,op=max,value=raid_event.adds.in,if=raid_event.adds.exists&raid_event.adds.count>=3&raid_event.adds.duration>15", "Raid Events: Delay Combustion for add spawns of 3 or more adds that will last longer than 15 seconds. These values aren't necessarily optimal in all cases." );
  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=time_to_combustion,value=raid_event.vulnerable.in*!raid_event.vulnerable.up,if=raid_event.vulnerable.exists&variable.combustion_ready_time<raid_event.vulnerable.in", "Raid Events: Always use Combustion with vulnerability raid events, override any delays listed above to make sure it gets used here." );
  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=time_to_combustion,value=variable.combustion_ready_time,if=variable.combustion_ready_time+cooldown.combustion.duration*(1-(0.4+0.2*talent.firestarter)*talent.kindling)<=variable.time_to_combustion|variable.time_to_combustion>fight_remains-20", "Use the next Combustion on cooldown if it would not be expected to delay the scheduled one or the scheduled one would happen less than 20 seconds before the fight ends." );

  firestarter_fire_blasts->add_action( "fire_blast,use_while_casting=1,if=!variable.fire_blast_pooling&!buff.hot_streak.react&(action.fireball.execute_remains>gcd.remains|action.pyroblast.executing)&buff.heating_up.react+hot_streak_spells_in_flight=1&(cooldown.shifting_power.ready|charges>1|buff.feel_the_burn.remains<2*gcd.max)", "While casting Fireball or Pyroblast, convert Heating Up to a Hot Streak!" );
  firestarter_fire_blasts->add_action( "fire_blast,use_off_gcd=1,if=!variable.fire_blast_pooling&buff.heating_up.react+hot_streak_spells_in_flight=1&(talent.feel_the_burn&buff.feel_the_burn.remains<gcd.remains|cooldown.shifting_power.ready&(!set_bonus.tier30_2pc|debuff.charring_embers.remains>2*gcd.max))", "If not casting anything, use Fire Blast to trigger Hot Streak! only if Feel the Burn is talented and would expire before the GCD ends or if Shifting Power is available." );

  standard_rotation->add_action( "flamestrike,if=active_enemies>=variable.hot_streak_flamestrike&(buff.hot_streak.react|buff.hyperthermia.react)" );
  standard_rotation->add_action( "pyroblast,if=buff.hyperthermia.react" );
  standard_rotation->add_action( "pyroblast,if=buff.hot_streak.react&(buff.hot_streak.remains<action.fireball.execute_time)" );
  standard_rotation->add_action( "pyroblast,if=buff.hot_streak.react&(hot_streak_spells_in_flight|firestarter.active|talent.alexstraszas_fury&action.phoenix_flames.charges)" );
  standard_rotation->add_action( "pyroblast,if=buff.hot_streak.react&searing_touch.active" );
  standard_rotation->add_action( "flamestrike,if=active_enemies>=variable.skb_flamestrike&buff.fury_of_the_sun_king.up&buff.fury_of_the_sun_king.expiration_delay_remains=0" );
  standard_rotation->add_action( "scorch,if=improved_scorch.active&debuff.improved_scorch.remains<action.pyroblast.cast_time+5*gcd.max&buff.fury_of_the_sun_king.up&!action.scorch.in_flight" );
  standard_rotation->add_action( "pyroblast,if=buff.fury_of_the_sun_king.up&buff.fury_of_the_sun_king.expiration_delay_remains=0" );
  standard_rotation->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=!firestarter.active&!variable.fire_blast_pooling&buff.fury_of_the_sun_king.down&(((action.fireball.executing&(action.fireball.execute_remains<0.5|!talent.hyperthermia)|action.pyroblast.executing&(action.pyroblast.execute_remains<0.5|!talent.hyperthermia))&buff.heating_up.react)|(searing_touch.active&(!improved_scorch.active|debuff.improved_scorch.stack=debuff.improved_scorch.max_stack|full_recharge_time<3)&(buff.heating_up.react&!action.scorch.executing|!buff.hot_streak.react&!buff.heating_up.react&action.scorch.executing&!hot_streak_spells_in_flight)))", "During the standard rotation, only use Fire Blasts when they are not being pooled for RoP or Combustion. Use Fire Blast either during a Fireball/Pyroblast cast when Heating Up is active or during execute with Searing Touch." );
  standard_rotation->add_action( "pyroblast,if=prev_gcd.1.scorch&buff.heating_up.react&searing_touch.active&active_enemies<variable.hot_streak_flamestrike" );
  standard_rotation->add_action( "scorch,if=improved_scorch.active&debuff.improved_scorch.remains<4*gcd.max" );
  standard_rotation->add_action( "phoenix_flames,if=talent.alexstraszas_fury&(!talent.feel_the_burn|buff.feel_the_burn.remains<2*gcd.max)" );
  standard_rotation->add_action( "phoenix_flames,if=set_bonus.tier30_2pc&debuff.charring_embers.remains<2*gcd.max&!buff.hot_streak.react" );
  standard_rotation->add_action( "scorch,if=improved_scorch.active&debuff.improved_scorch.stack<debuff.improved_scorch.max_stack" );
  standard_rotation->add_action( "phoenix_flames,if=!talent.alexstraszas_fury&!buff.hot_streak.react&!variable.phoenix_pooling&buff.flames_fury.up" );
  standard_rotation->add_action( "phoenix_flames,if=talent.alexstraszas_fury&!buff.hot_streak.react&hot_streak_spells_in_flight=0&(!variable.phoenix_pooling&buff.flames_fury.up|charges_fractional>2.5|charges_fractional>1.5&(!talent.feel_the_burn|buff.feel_the_burn.remains<3*gcd.max))" );
  standard_rotation->add_action( "call_action_list,name=active_talents" );
  standard_rotation->add_action( "dragons_breath,if=active_enemies>1&talent.alexstraszas_fury", "Dragon's Breath is no longer a gain to be cast unless Alexstrazas is talented (need to check cutoff, was true on 10t)" );
  standard_rotation->add_action( "scorch,if=searing_touch.active" );
  standard_rotation->add_action( "arcane_explosion,if=active_enemies>=variable.arcane_explosion&mana.pct>=variable.arcane_explosion_mana" );
  standard_rotation->add_action( "flamestrike,if=active_enemies>=variable.hard_cast_flamestrike", "With enough targets, it is a gain to cast Flamestrike as filler instead of Fireball." );
  standard_rotation->add_action( "pyroblast,if=talent.tempered_flames&!buff.flame_accelerant.react" );
  standard_rotation->add_action( "fireball" );
}
//fire_apl_end

//frost_apl_start
void frost( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* movement = p->get_action_priority_list( "movement" );
  action_priority_list_t* cleave = p->get_action_priority_list( "cleave" );
  action_priority_list_t* st = p->get_action_priority_list( "st" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "blizzard,if=active_enemies>=2&talent.ice_caller|active_enemies>=3" );
  precombat->add_action( "frostbolt,if=active_enemies<=2" );

  default_->add_action( "counterspell" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "run_action_list,name=aoe,if=active_enemies>=7&!set_bonus.tier30_2pc|active_enemies>=3&talent.ice_caller" );
  default_->add_action( "run_action_list,name=cleave,if=active_enemies=2" );
  default_->add_action( "run_action_list,name=st" );

  aoe->add_action( "cone_of_cold,if=talent.coldest_snap&(prev_gcd.1.comet_storm|prev_gcd.1.frozen_orb&!talent.comet_storm)" );
  aoe->add_action( "frozen_orb,if=!prev_gcd.1.glacial_spike|!freezable" );
  aoe->add_action( "blizzard,if=!prev_gcd.1.glacial_spike|!freezable" );
  aoe->add_action( "comet_storm,if=!prev_gcd.1.glacial_spike&(!talent.coldest_snap|cooldown.cone_of_cold.ready&cooldown.frozen_orb.remains>25|cooldown.cone_of_cold.remains>20)" );
  aoe->add_action( "freeze,if=freezable&debuff.frozen.down&(!talent.glacial_spike&!talent.snowstorm|prev_gcd.1.glacial_spike|cooldown.cone_of_cold.ready&buff.snowstorm.stack=buff.snowstorm.max_stack)" );
  aoe->add_action( "ice_nova,if=freezable&!prev_off_gcd.freeze&(prev_gcd.1.glacial_spike|cooldown.cone_of_cold.ready&buff.snowstorm.stack=buff.snowstorm.max_stack&gcd.max<1)" );
  aoe->add_action( "frost_nova,if=freezable&!prev_off_gcd.freeze&(prev_gcd.1.glacial_spike&!remaining_winters_chill|cooldown.cone_of_cold.ready&buff.snowstorm.stack=buff.snowstorm.max_stack&gcd.max<1)" );
  aoe->add_action( "cone_of_cold,if=buff.snowstorm.stack=buff.snowstorm.max_stack" );
  aoe->add_action( "shifting_power" );
  aoe->add_action( "glacial_spike,if=buff.icicles.react=5&cooldown.blizzard.remains>gcd.max" );
  aoe->add_action( "flurry,if=!freezable&cooldown_react&!debuff.winters_chill.remains&(prev_gcd.1.glacial_spike|charges_fractional>1.8)" );
  aoe->add_action( "flurry,if=cooldown_react&!debuff.winters_chill.remains&(buff.brain_freeze.react|!buff.fingers_of_frost.react)" );
  aoe->add_action( "ice_lance,if=buff.fingers_of_frost.react|debuff.frozen.remains>travel_time|remaining_winters_chill" );
  aoe->add_action( "ice_nova,if=active_enemies>=4&(!talent.snowstorm&!talent.glacial_spike|!freezable)" );
  aoe->add_action( "dragons_breath,if=active_enemies>=7" );
  aoe->add_action( "arcane_explosion,if=mana.pct>30&active_enemies>=7" );
  aoe->add_action( "frostbolt" );
  aoe->add_action( "call_action_list,name=movement" );

  cds->add_action( "time_warp,if=buff.exhaustion.up&talent.temporal_warp&buff.bloodlust.down&(prev_off_gcd.icy_veins|(buff.icy_veins.up&fight_remains<=110|buff.icy_veins.up&fight_remains>=280)|fight_remains<40)" );
  cds->add_action( "use_item,name=spoils_of_neltharus,if=buff.spoils_of_neltharus_mastery.up|buff.spoils_of_neltharus_haste.up&buff.bloodlust.down&buff.temporal_warp.down&time>0|buff.spoils_of_neltharus_vers.up&(buff.bloodlust.up|buff.temporal_warp.up)" );
  cds->add_action( "potion,if=prev_off_gcd.icy_veins|fight_remains<60" );
  cds->add_action( "use_item,name=dreambinder_loom_of_the_great_cycle,if=(equipped.nymues_unraveling_spindle&prev_gcd.1.nymues_unraveling_spindle)|fight_remains>2" );
  cds->add_action( "use_item,name=belorrelos_the_suncaller,if=time>5&!prev_gcd.1.flurry" );
  cds->add_action( "use_item,name=balefire_branch,if=(!talent.ray_of_frost&active_enemies<=2&buff.icy_veins.up&prev_gcd.1.glacial_spike|remaining_winters_chill=1&cooldown.ray_of_frost.up&time>1&active_enemies<=2|cooldown.cone_of_cold.up&prev_gcd.1.comet_storm&active_enemies>=3)|fight_remains<20" );
  cds->add_action( "flurry,if=time=0&active_enemies<=2" );
  cds->add_action( "icy_veins" );
  cds->add_action( "use_items,if=!equipped.balefire_branch|time>5" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.power_infusion.down" );
  cds->add_action( "invoke_external_buff,name=blessing_of_summer,if=buff.blessing_of_summer.down" );
  cds->add_action( "blood_fury" );
  cds->add_action( "berserking" );
  cds->add_action( "lights_judgment" );
  cds->add_action( "fireblood" );
  cds->add_action( "ancestral_call" );

  movement->add_action( "any_blink,if=movement.distance>10" );
  movement->add_action( "ice_floes,if=buff.ice_floes.down" );
  movement->add_action( "ice_nova" );
  movement->add_action( "arcane_explosion,if=mana.pct>30&active_enemies>=2" );
  movement->add_action( "fire_blast" );
  movement->add_action( "ice_lance" );

  cleave->add_action( "comet_storm,if=prev_gcd.1.flurry|prev_gcd.1.cone_of_cold" );
  cleave->add_action( "flurry,target_if=min:debuff.winters_chill.stack,if=cooldown_react&((prev_gcd.1.frostbolt&buff.icicles.react>=3)|prev_gcd.1.glacial_spike|(buff.icicles.react>=3&buff.icicles.react<5&charges_fractional=2))" );
  cleave->add_action( "ice_lance,target_if=max:debuff.winters_chill.stack,if=talent.glacial_spike&debuff.winters_chill.down&buff.icicles.react=4&buff.fingers_of_frost.react" );
  cleave->add_action( "ray_of_frost,target_if=max:debuff.winters_chill.stack,if=remaining_winters_chill=1" );
  cleave->add_action( "glacial_spike,if=buff.icicles.react=5&(action.flurry.cooldown_react|remaining_winters_chill)" );
  cleave->add_action( "frozen_orb,if=buff.fingers_of_frost.react<2&(!talent.ray_of_frost|cooldown.ray_of_frost.remains)" );
  cleave->add_action( "cone_of_cold,if=talent.coldest_snap&cooldown.comet_storm.remains>10&cooldown.frozen_orb.remains>10&remaining_winters_chill=0&active_enemies>=3" );
  cleave->add_action( "blizzard,if=active_enemies>=2&talent.ice_caller&talent.freezing_rain&(!talent.splintering_cold&!talent.ray_of_frost|buff.freezing_rain.up|active_enemies>=3)" );
  cleave->add_action( "shifting_power,if=cooldown.frozen_orb.remains>10&(!talent.comet_storm|cooldown.comet_storm.remains>10)&(!talent.ray_of_frost|cooldown.ray_of_frost.remains>10)|cooldown.icy_veins.remains<20" );
  cleave->add_action( "glacial_spike,if=buff.icicles.react=5" );
  cleave->add_action( "ice_lance,target_if=max:debuff.winters_chill.stack,if=buff.fingers_of_frost.react&!prev_gcd.1.glacial_spike|remaining_winters_chill" );
  cleave->add_action( "ice_nova,if=active_enemies>=4" );
  cleave->add_action( "frostbolt" );
  cleave->add_action( "call_action_list,name=movement" );

  st->add_action( "comet_storm,if=prev_gcd.1.flurry|prev_gcd.1.cone_of_cold" );
  st->add_action( "flurry,if=cooldown_react&remaining_winters_chill=0&debuff.winters_chill.down&((prev_gcd.1.frostbolt&buff.icicles.react>=3|prev_gcd.1.frostbolt&buff.brain_freeze.react)|prev_gcd.1.glacial_spike|talent.glacial_spike&buff.icicles.react=4&!buff.fingers_of_frost.react)" );
  st->add_action( "ice_lance,if=talent.glacial_spike&debuff.winters_chill.down&buff.icicles.react=4&buff.fingers_of_frost.react" );
  st->add_action( "ray_of_frost,if=remaining_winters_chill=1" );
  st->add_action( "glacial_spike,if=buff.icicles.react=5&(action.flurry.cooldown_react|remaining_winters_chill)" );
  st->add_action( "frozen_orb,if=buff.fingers_of_frost.react<2&(!talent.ray_of_frost|cooldown.ray_of_frost.remains)" );
  st->add_action( "cone_of_cold,if=talent.coldest_snap&cooldown.comet_storm.remains>10&cooldown.frozen_orb.remains>10&remaining_winters_chill=0&active_enemies>=3" );
  st->add_action( "blizzard,if=active_enemies>=2&talent.ice_caller&talent.freezing_rain&(!talent.splintering_cold&!talent.ray_of_frost|buff.freezing_rain.up|active_enemies>=3)" );
  st->add_action( "shifting_power,if=cooldown.frozen_orb.remains>10&(!talent.comet_storm|cooldown.comet_storm.remains>10)&(!talent.ray_of_frost|cooldown.ray_of_frost.remains>10)|cooldown.icy_veins.remains<20" );
  st->add_action( "glacial_spike,if=buff.icicles.react=5" );
  st->add_action( "ice_lance,if=buff.fingers_of_frost.react&!prev_gcd.1.glacial_spike|remaining_winters_chill" );
  st->add_action( "ice_nova,if=active_enemies>=4" );
  st->add_action( "frostbolt" );
  st->add_action( "call_action_list,name=movement" );
}
//frost_apl_end

}  // namespace mage_apl
