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
  action_priority_list_t* spark_phase = p->get_action_priority_list( "spark_phase" );
  action_priority_list_t* aoe_spark_phase = p->get_action_priority_list( "aoe_spark_phase" );
  action_priority_list_t* touch_phase = p->get_action_priority_list( "touch_phase" );
  action_priority_list_t* aoe_touch_phase = p->get_action_priority_list( "aoe_touch_phase" );
  action_priority_list_t* rop_phase = p->get_action_priority_list( "rop_phase" );
  action_priority_list_t* standard_cooldowns = p->get_action_priority_list( "standard_cooldowns" );
  action_priority_list_t* rotation = p->get_action_priority_list( "rotation" );
  action_priority_list_t* aoe_rotation = p->get_action_priority_list( "aoe_rotation" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "arcane_familiar" );
  precombat->add_action( "conjure_mana_gem" );
  precombat->add_action( "variable,name=aoe_target_count,op=reset,default=3" );
  precombat->add_action( "variable,name=conserve_mana,op=set,value=0" );
  precombat->add_action( "variable,name=opener,op=set,value=1" );
  precombat->add_action( "variable,name=opener_min_mana,default=-1,op=set,if=variable.opener_min_mana=-1,value=225000-(25000*!talent.arcane_harmony)" );
  precombat->add_action( "variable,name=steroid_trinket_equipped,op=set,value=equipped.gladiators_badge|equipped.irideus_fragment|equipped.erupting_spear_fragment|equipped.spoils_of_neltharus|equipped.tome_of_unstable_power|equipped.timebreaching_talon|equipped.horn_of_valor" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "mirror_image" );
  precombat->add_action( "arcane_blast,if=!talent.siphon_storm" );
  precombat->add_action( "evocation,if=talent.siphon_storm" );

  default_->add_action( "counterspell" );
  default_->add_action( "potion,if=cooldown.arcane_surge.ready" );
  default_->add_action( "time_warp,if=talent.temporal_warp&buff.exhaustion.up&(cooldown.arcane_surge.ready|fight_remains<=40|buff.arcane_surge.up&fight_remains<=80)" );
  default_->add_action( "lights_judgment,if=buff.arcane_surge.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down" );
  default_->add_action( "berserking,if=(prev_gcd.1.arcane_surge&!(buff.temporal_warp.up&buff.bloodlust.up))|(buff.arcane_surge.up&debuff.touch_of_the_magi.up)" );
  default_->add_action( "blood_fury,if=prev_gcd.1.arcane_surge" );
  default_->add_action( "fireblood,if=prev_gcd.1.arcane_surge" );
  default_->add_action( "ancestral_call,if=prev_gcd.1.arcane_surge" );
  default_->add_action( "invoke_external_buff,name=power_infusion,if=!talent.radiant_spark&prev_gcd.1.arcane_surge" );
  default_->add_action( "use_items,if=prev_gcd.1.arcane_surge" );
  default_->add_action( "use_item,name=tinker_breath_of_neltharion,if=cooldown.arcane_surge.remains&buff.rune_of_power.down&buff.arcane_surge.down&debuff.touch_of_the_magi.down" );
  default_->add_action( "use_item,name=conjured_chillglobe,if=mana.pct>65&(!variable.steroid_trinket_equipped|cooldown.arcane_surge.remains>20)" );
  default_->add_action( "use_item,name=darkmoon_deck_rime,if=!variable.steroid_trinket_equipped|cooldown.arcane_surge.remains>20" );
  default_->add_action( "use_item,name=darkmoon_deck_dance,if=!variable.steroid_trinket_equipped|cooldown.arcane_surge.remains>20" );
  default_->add_action( "use_item,name=darkmoon_deck_inferno,if=!variable.steroid_trinket_equipped|cooldown.arcane_surge.remains>20" );
  default_->add_action( "use_item,name=desperate_invokers_codex,if=!variable.steroid_trinket_equipped|cooldown.arcane_surge.remains>20" );
  default_->add_action( "use_item,name=iceblood_deathsnare,if=!variable.steroid_trinket_equipped|cooldown.arcane_surge.remains>20" );
  default_->add_action( "variable,name=aoe_spark_phase,op=set,value=1,if=active_enemies>=variable.aoe_target_count&(action.arcane_orb.charges>0|buff.arcane_charge.stack>=3)&(!talent.rune_of_power|cooldown.rune_of_power.ready)&cooldown.radiant_spark.ready&cooldown.touch_of_the_magi.remains<=(gcd.max*2)" );
  default_->add_action( "variable,name=aoe_spark_phase,op=set,value=0,if=variable.aoe_spark_phase&debuff.radiant_spark_vulnerability.down&dot.radiant_spark.remains<5&cooldown.radiant_spark.remains" );
  default_->add_action( "variable,name=spark_phase,op=set,value=1,if=buff.arcane_charge.stack>=3&active_enemies<variable.aoe_target_count&(!talent.rune_of_power|cooldown.rune_of_power.ready)&cooldown.radiant_spark.ready&cooldown.touch_of_the_magi.remains<=(gcd.max*7)" );
  default_->add_action( "variable,name=spark_phase,op=set,value=0,if=variable.spark_phase&debuff.radiant_spark_vulnerability.down&dot.radiant_spark.remains<5&cooldown.radiant_spark.remains" );
  default_->add_action( "variable,name=rop_phase,op=set,value=1,if=talent.rune_of_power&!talent.radiant_spark&buff.arcane_charge.stack>=3&cooldown.rune_of_power.ready&active_enemies<variable.aoe_target_count" );
  default_->add_action( "variable,name=rop_phase,op=set,value=0,if=debuff.touch_of_the_magi.up|!talent.rune_of_power" );
  default_->add_action( "variable,name=opener,op=set,if=debuff.touch_of_the_magi.up&variable.opener,value=0" );
  default_->add_action( "cancel_action,if=action.evocation.channeling&mana.pct>=95&!talent.siphon_storm" );
  default_->add_action( "cancel_action,if=action.evocation.channeling&(mana.pct>fight_remains*4)&!(fight_remains>10&cooldown.arcane_surge.remains<1)" );
  default_->add_action( "arcane_barrage,if=fight_remains<2" );
  default_->add_action( "evocation,if=buff.rune_of_power.down&buff.arcane_surge.down&debuff.touch_of_the_magi.down&((mana.pct<10&cooldown.touch_of_the_magi.remains<25)|cooldown.touch_of_the_magi.remains<20)&(mana.pct<fight_remains*4)" );
  default_->add_action( "conjure_mana_gem,if=buff.rune_of_power.down&debuff.touch_of_the_magi.down&buff.arcane_surge.down&cooldown.arcane_surge.remains<fight_remains&!mana_gem_charges" );
  default_->add_action( "use_mana_gem,if=talent.cascading_power&buff.clearcasting.stack<2&buff.arcane_surge.up" );
  default_->add_action( "use_mana_gem,if=!talent.cascading_power&prev_gcd.1.arcane_surge" );
  default_->add_action( "call_action_list,name=aoe_spark_phase,if=talent.radiant_spark&variable.aoe_spark_phase" );
  default_->add_action( "call_action_list,name=spark_phase,if=talent.radiant_spark&variable.spark_phase" );
  default_->add_action( "call_action_list,name=aoe_touch_phase,if=debuff.touch_of_the_magi.up&active_enemies>=variable.aoe_target_count" );
  default_->add_action( "call_action_list,name=touch_phase,if=debuff.touch_of_the_magi.up&active_enemies<variable.aoe_target_count" );
  default_->add_action( "call_action_list,name=rop_phase,if=variable.rop_phase" );
  default_->add_action( "call_action_list,name=standard_cooldowns,if=!talent.radiant_spark&(!talent.rune_of_power|active_enemies>=variable.aoe_target_count)" );
  default_->add_action( "call_action_list,name=aoe_rotation,if=active_enemies>=variable.aoe_target_count" );
  default_->add_action( "call_action_list,name=rotation" );

  spark_phase->add_action( "nether_tempest,if=!ticking&variable.opener&buff.bloodlust.up,line_cd=45" );
  spark_phase->add_action( "rune_of_power" );
  spark_phase->add_action( "arcane_blast,if=variable.opener&cooldown.arcane_surge.ready&buff.bloodlust.up&mana>=variable.opener_min_mana&buff.rune_of_power.remains>gcd.max*4" );
  spark_phase->add_action( "arcane_missiles,if=variable.opener&buff.bloodlust.up&buff.clearcasting.react&buff.clearcasting.stack>=2&cooldown.radiant_spark.remains<5&buff.nether_precision.down,chain=1" );
  spark_phase->add_action( "arcane_missiles,if=talent.arcane_harmony&buff.arcane_harmony.stack<15&((variable.opener&buff.bloodlust.up)|buff.clearcasting.react&cooldown.radiant_spark.remains<5)&cooldown.arcane_surge.remains<30,chain=1" );
  spark_phase->add_action( "radiant_spark" );
  spark_phase->add_action( "use_item,name=timebreaching_talon,if=cooldown.arcane_surge.remains<=(gcd.max*3)" );
  spark_phase->add_action( "invoke_external_buff,name=power_infusion,if=prev_gcd.1.radiant_spark&cooldown.arcane_surge.remains<=(gcd.max*3)" );
  spark_phase->add_action( "nether_tempest,if=(prev_gcd.4.radiant_spark&cooldown.arcane_surge.remains<=execute_time)|prev_gcd.5.radiant_spark,line_cd=15" );
  spark_phase->add_action( "arcane_surge,if=(!talent.nether_tempest&prev_gcd.4.radiant_spark)|prev_gcd.1.nether_tempest" );
  spark_phase->add_action( "meteor,if=(talent.nether_tempest&prev_gcd.6.radiant_spark)|(!talent.nether_tempest&prev_gcd.5.radiant_spark)" );
  spark_phase->add_action( "arcane_blast,if=cast_time>=gcd&execute_time<debuff.radiant_spark_vulnerability.remains&(!talent.arcane_bombardment|target.health.pct>=35)&(talent.nether_tempest&prev_gcd.6.radiant_spark|!talent.nether_tempest&prev_gcd.5.radiant_spark)&!talent.meteor" );
  spark_phase->add_action( "wait,sec=0.05,if=!talent.meteor&(talent.nether_tempest&prev_gcd.6.radiant_spark)|(!talent.nether_tempest&prev_gcd.5.radiant_spark),line_cd=15" );
  spark_phase->add_action( "arcane_barrage,if=debuff.radiant_spark_vulnerability.stack=4" );
  spark_phase->add_action( "touch_of_the_magi,use_off_gcd=1,if=prev_gcd.1.arcane_barrage&(action.arcane_barrage.in_flight_remains<=0.2|gcd.remains<=0.2)" );
  spark_phase->add_action( "arcane_blast" );
  spark_phase->add_action( "arcane_barrage" );

  aoe_spark_phase->add_action( "cancel_buff,name=presence_of_mind,if=prev_gcd.1.arcane_blast&cooldown.arcane_surge.remains>75" );
  aoe_spark_phase->add_action( "rune_of_power,if=cooldown.arcane_surge.remains<75&cooldown.arcane_surge.remains>30" );
  aoe_spark_phase->add_action( "radiant_spark" );
  aoe_spark_phase->add_action( "use_item,name=timebreaching_talon,if=cooldown.arcane_surge.remains<=(gcd.max*2)" );
  aoe_spark_phase->add_action( "arcane_explosion,if=buff.arcane_charge.stack>=3&prev_gcd.1.radiant_spark" );
  aoe_spark_phase->add_action( "arcane_orb,if=buff.arcane_charge.stack<3,line_cd=15" );
  aoe_spark_phase->add_action( "nether_tempest,if=talent.arcane_echo,line_cd=15" );
  aoe_spark_phase->add_action( "arcane_surge" );
  aoe_spark_phase->add_action( "wait,sec=0.05,if=cooldown.arcane_surge.remains>75&prev_gcd.1.arcane_blast&!talent.presence_of_mind,line_cd=15" );
  aoe_spark_phase->add_action( "wait,sec=0.05,if=prev_gcd.1.arcane_surge,line_cd=15" );
  aoe_spark_phase->add_action( "wait,sec=0.05,if=cooldown.arcane_surge.remains<75&debuff.radiant_spark_vulnerability.stack=3&!talent.presence_of_mind,line_cd=15" );
  aoe_spark_phase->add_action( "arcane_barrage,if=cooldown.arcane_surge.remains<75&debuff.radiant_spark_vulnerability.stack=4" );
  aoe_spark_phase->add_action( "arcane_barrage,if=(debuff.radiant_spark_vulnerability.stack=2&cooldown.arcane_surge.remains>75)|(debuff.radiant_spark_vulnerability.stack=1&cooldown.arcane_surge.remains<75)" );
  aoe_spark_phase->add_action( "touch_of_the_magi,use_off_gcd=1,if=prev_gcd.1.arcane_barrage" );
  aoe_spark_phase->add_action( "presence_of_mind" );
  aoe_spark_phase->add_action( "arcane_blast,if=debuff.radiant_spark_vulnerability.stack=2|debuff.radiant_spark_vulnerability.stack=3" );
  aoe_spark_phase->add_action( "arcane_barrage,if=(debuff.radiant_spark_vulnerability.stack=4&buff.arcane_surge.up)|(debuff.radiant_spark_vulnerability.stack=3&buff.arcane_surge.down)" );

  touch_phase->add_action( "variable,name=conserve_mana,op=set,if=debuff.touch_of_the_magi.remains>9,value=1-variable.conserve_mana" );
  touch_phase->add_action( "meteor" );
  touch_phase->add_action( "presence_of_mind,if=(!talent.arcane_bombardment|target.health.pct>35)&buff.arcane_surge.up&debuff.touch_of_the_magi.remains<=gcd.max" );
  touch_phase->add_action( "arcane_blast,if=buff.presence_of_mind.up&buff.arcane_charge.stack=buff.arcane_charge.max_stack" );
  touch_phase->add_action( "arcane_barrage,if=(buff.arcane_harmony.up|(talent.arcane_bombardment&target.health.pct<35))&debuff.touch_of_the_magi.remains<=gcd.max" );
  touch_phase->add_action( "arcane_missiles,if=buff.clearcasting.stack>1&talent.conjure_mana_gem&cooldown.use_mana_gem.ready,chain=1" );
  touch_phase->add_action( "arcane_blast,if=buff.nether_precision.up" );
  touch_phase->add_action( "cancel_action,if=debuff.touch_of_the_magi.up&action.arcane_missiles.channeling&gcd.remains=0&(buff.arcane_surge.up|talent.conjure_mana_gem|set_bonus.tier30_4pc)&mana.pct>30" );
  touch_phase->add_action( "arcane_missiles,if=buff.clearcasting.react&(debuff.touch_of_the_magi.remains>execute_time|!talent.presence_of_mind),chain=1" );
  touch_phase->add_action( "arcane_blast" );
  touch_phase->add_action( "arcane_barrage" );

  aoe_touch_phase->add_action( "variable,name=conserve_mana,op=set,if=debuff.touch_of_the_magi.remains>9,value=1-variable.conserve_mana" );
  aoe_touch_phase->add_action( "meteor" );
  aoe_touch_phase->add_action( "arcane_barrage,if=talent.arcane_bombardment&target.health.pct<35&debuff.touch_of_the_magi.remains<=gcd.max" );
  aoe_touch_phase->add_action( "arcane_barrage,if=buff.arcane_charge.stack=buff.arcane_charge.max_stack&active_enemies>=variable.aoe_target_count&cooldown.arcane_orb.remains<=execute_time" );
  aoe_touch_phase->add_action( "arcane_missiles,if=buff.clearcasting.react&(talent.arcane_echo|talent.arcane_harmony),chain=1" );
  aoe_touch_phase->add_action( "arcane_missiles,if=buff.clearcasting.stack>1&talent.conjure_mana_gem&cooldown.use_mana_gem.ready,chain=1" );
  aoe_touch_phase->add_action( "arcane_orb,if=buff.arcane_charge.stack<2" );
  aoe_touch_phase->add_action( "arcane_barrage,if=buff.arcane_charge.stack=buff.arcane_charge.max_stack" );
  aoe_touch_phase->add_action( "arcane_explosion" );

  rop_phase->add_action( "rune_of_power" );
  rop_phase->add_action( "arcane_blast,if=prev_gcd.1.rune_of_power" );
  rop_phase->add_action( "arcane_blast,if=prev_gcd.1.arcane_blast&prev_gcd.2.rune_of_power" );
  rop_phase->add_action( "arcane_blast,if=prev_gcd.1.arcane_blast&prev_gcd.3.rune_of_power" );
  rop_phase->add_action( "arcane_surge" );
  rop_phase->add_action( "arcane_blast,if=prev_gcd.1.arcane_blast&prev_gcd.4.rune_of_power" );
  rop_phase->add_action( "nether_tempest,if=talent.arcane_echo,line_cd=15" );
  rop_phase->add_action( "meteor" );
  rop_phase->add_action( "arcane_barrage" );
  rop_phase->add_action( "touch_of_the_magi,use_off_gcd=1,if=prev_gcd.1.arcane_barrage" );

  standard_cooldowns->add_action( "arcane_surge,if=buff.arcane_charge.stack=buff.arcane_charge.max_stack" );
  standard_cooldowns->add_action( "nether_tempest,if=prev_gcd.1.arcane_surge&talent.arcane_echo" );
  standard_cooldowns->add_action( "meteor,if=buff.arcane_surge.up&cooldown.touch_of_the_magi.ready" );
  standard_cooldowns->add_action( "arcane_barrage,if=buff.arcane_surge.up&cooldown.touch_of_the_magi.ready" );
  standard_cooldowns->add_action( "rune_of_power,if=cooldown.touch_of_the_magi.remains<=(gcd.max*2)&buff.arcane_charge.stack=buff.arcane_charge.max_stack" );
  standard_cooldowns->add_action( "meteor,if=cooldown.touch_of_the_magi.remains<=(gcd.max*2)&buff.arcane_charge.stack=buff.arcane_charge.max_stack" );
  standard_cooldowns->add_action( "touch_of_the_magi,use_off_gcd=1,if=prev_gcd.1.arcane_barrage" );

  rotation->add_action( "arcane_orb,if=buff.arcane_charge.stack<3&(buff.bloodlust.down|mana.pct>70|cooldown.touch_of_the_magi.remains>30)" );
  rotation->add_action( "shifting_power,if=(!talent.evocation|cooldown.evocation.remains>12)&(!talent.arcane_surge|cooldown.arcane_surge.remains>12)&(!talent.touch_of_the_magi|cooldown.touch_of_the_magi.remains>12)&buff.arcane_surge.down&fight_remains>15" );
  rotation->add_action( "presence_of_mind,if=buff.arcane_charge.stack<3&target.health.pct<35&talent.arcane_bombardment" );
  rotation->add_action( "arcane_blast,if=buff.presence_of_mind.up&target.health.pct<35&talent.arcane_bombardment&buff.arcane_charge.stack<3" );
  rotation->add_action( "arcane_missiles,if=buff.clearcasting.react&buff.clearcasting.stack=buff.clearcasting.max_stack" );
  rotation->add_action( "nether_tempest,if=(refreshable|!ticking)&buff.arcane_charge.stack=buff.arcane_charge.max_stack&(buff.temporal_warp.up|mana.pct<10|!talent.shifting_power)&buff.arcane_surge.down&fight_remains>=12" );
  rotation->add_action( "arcane_barrage,if=buff.arcane_charge.stack=buff.arcane_charge.max_stack&mana.pct<50&!talent.evocation&fight_remains>20" );
  rotation->add_action( "arcane_barrage,if=buff.arcane_charge.stack=buff.arcane_charge.max_stack&mana.pct<70&variable.conserve_mana&buff.bloodlust.up&cooldown.touch_of_the_magi.remains>5&fight_remains>20" );
  rotation->add_action( "arcane_missiles,if=buff.clearcasting.react&buff.concentration.up&buff.arcane_charge.stack=buff.arcane_charge.max_stack" );
  rotation->add_action( "arcane_blast,if=buff.arcane_charge.stack=buff.arcane_charge.max_stack&buff.nether_precision.up" );
  rotation->add_action( "arcane_barrage,if=buff.arcane_charge.stack=buff.arcane_charge.max_stack&mana.pct<60&variable.conserve_mana&(!talent.rune_of_power|cooldown.rune_of_power.remains>5)&cooldown.touch_of_the_magi.remains>10&cooldown.evocation.remains>40&fight_remains>20" );
  rotation->add_action( "arcane_missiles,if=buff.clearcasting.react&buff.nether_precision.down" );
  rotation->add_action( "arcane_blast" );
  rotation->add_action( "arcane_barrage" );

  aoe_rotation->add_action( "arcane_orb,if=buff.arcane_charge.stack<2" );
  aoe_rotation->add_action( "shifting_power,if=(!talent.evocation|cooldown.evocation.remains>12)&(!talent.arcane_surge|cooldown.arcane_surge.remains>12)&(!talent.touch_of_the_magi|cooldown.touch_of_the_magi.remains>12)&buff.arcane_surge.down" );
  aoe_rotation->add_action( "ice_nova,if=buff.arcane_surge.down" );
  aoe_rotation->add_action( "nether_tempest,if=(refreshable|!ticking)&buff.arcane_charge.stack=buff.arcane_charge.max_stack&buff.arcane_surge.down" );
  aoe_rotation->add_action( "arcane_missiles,if=buff.clearcasting.react&talent.arcane_harmony&talent.rune_of_power&cooldown.rune_of_power.remains<(gcd.max*2)" );
  aoe_rotation->add_action( "arcane_barrage,if=buff.arcane_charge.stack=buff.arcane_charge.max_stack|mana.pct<10" );
  aoe_rotation->add_action( "arcane_explosion" );
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
  action_priority_list_t* rop_phase = p->get_action_priority_list( "rop_phase" );
  action_priority_list_t* standard_rotation = p->get_action_priority_list( "standard_rotation" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "variable,name=disable_combustion,op=reset", "APL Variable Option: If set to a non-zero value, the Combustion action and cooldowns that are constrained to only be used when Combustion is up will not be used during the simulation." );
  precombat->add_action( "variable,name=firestarter_combustion,default=-1,value=talent.sun_kings_blessing,if=variable.firestarter_combustion<0", "APL Variable Option: This variable specifies whether Combustion should be used during Firestarter." );
  precombat->add_action( "variable,name=hot_streak_flamestrike,if=variable.hot_streak_flamestrike=0,value=2*talent.flame_patch+999*!talent.flame_patch", "APL Variable Option: This variable specifies the number of targets at which Hot Streak Flamestrikes outside of Combustion should be used." );
  precombat->add_action( "variable,name=hard_cast_flamestrike,if=variable.hard_cast_flamestrike=0,value=3*talent.flame_patch+999*!talent.flame_patch", "APL Variable Option: This variable specifies the number of targets at which Hard Cast Flamestrikes outside of Combustion should be used as filler." );
  precombat->add_action( "variable,name=combustion_flamestrike,if=variable.combustion_flamestrike=0,value=3*talent.flame_patch+999*!talent.flame_patch", "APL Variable Option: This variable specifies the number of targets at which Hot Streak Flamestrikes are used during Combustion." );
  precombat->add_action( "variable,name=arcane_explosion,if=variable.arcane_explosion=0,value=999", "APL Variable Option: This variable specifies the number of targets at which Arcane Explosion outside of Combustion should be used." );
  precombat->add_action( "variable,name=arcane_explosion_mana,default=40,op=reset", "APL Variable Option: This variable specifies the percentage of mana below which Arcane Explosion will not be used." );
  precombat->add_action( "variable,name=combustion_shifting_power,if=variable.combustion_shifting_power=0,value=variable.combustion_flamestrike", "APL Variable Option: The number of targets at which Shifting Power can used during Combustion." );
  precombat->add_action( "variable,name=combustion_cast_remains,default=0.7,op=reset", "APL Variable Option: The time remaining on a cast when Combustion can be used in seconds." );
  precombat->add_action( "variable,name=overpool_fire_blasts,default=0,op=reset", "APL Variable Option: This variable specifies the number of seconds of Fire Blast that should be pooled past the default amount." );
  precombat->add_action( "variable,name=time_to_combustion,value=fight_remains+100,if=variable.disable_combustion", "If Combustion is disabled, schedule the first Combustion far after the fight ends." );
  precombat->add_action( "variable,name=skb_duration,value=dbc.effect.1016075.base_value", "The duration of a Sun King's Blessing Combustion." );
  precombat->add_action( "variable,name=combustion_on_use,value=equipped.gladiators_badge|equipped.moonlit_prism|equipped.irideus_fragment|equipped.spoils_of_neltharus|equipped.tome_of_unstable_power|equipped.timebreaching_talon|equipped.horn_of_valor", "Whether a usable item used to buff Combustion is equipped." );
  precombat->add_action( "variable,name=on_use_cutoff,value=20,if=variable.combustion_on_use", "How long before Combustion should trinkets that trigger a shared category cooldown on other trinkets not be used?" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "mirror_image" );
  precombat->add_action( "pyroblast" );

  default_->add_action( "counterspell" );
  default_->add_action( "call_action_list,name=combustion_timing,if=!variable.disable_combustion", "The combustion_timing action list schedules the approximate time when Combustion should be used and stores the number of seconds until then in variable.time_to_combustion." );
  default_->add_action( "variable,name=shifting_power_before_combustion,value=variable.time_to_combustion>cooldown.shifting_power.remains|firestarter.active", "Variable that estimates whether Shifting Power will be used before the next Combustion. TODO: During Firestarter, it sims higher to always set this to 1 even when Shifting Power has already been used. This needs further investigation." );
  default_->add_action( "shifting_power,if=buff.combustion.down&action.fire_blast.charges<=1&(cooldown.rune_of_power.remains|!talent.rune_of_power)&!buff.hot_streak.react&variable.shifting_power_before_combustion" );
  default_->add_action( "variable,name=item_cutoff_active,value=(variable.time_to_combustion<variable.on_use_cutoff|buff.combustion.remains>variable.skb_duration&!cooldown.item_cd_1141.remains)&((trinket.1.has_cooldown&trinket.1.cooldown.remains<variable.on_use_cutoff)+(trinket.2.has_cooldown&trinket.2.cooldown.remains<variable.on_use_cutoff)>1)" );
  default_->add_action( "use_item,effect_name=gladiators_badge,if=variable.time_to_combustion>cooldown-5" );
  default_->add_action( "use_item,name=moonlit_prism,if=variable.time_to_combustion<=5|fight_remains<variable.time_to_combustion" );
  default_->add_action( "use_items,if=!variable.item_cutoff_active" );
  default_->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=fire_blast_pooling,value=(variable.extended_combustion_remains<variable.time_to_combustion)&action.fire_blast.charges_fractional+(variable.time_to_combustion+action.shifting_power.full_reduction*variable.shifting_power_before_combustion)%cooldown.fire_blast.duration-1<cooldown.fire_blast.max_charges+variable.overpool_fire_blasts%cooldown.fire_blast.duration-(buff.combustion.duration%cooldown.fire_blast.duration)%%1&variable.time_to_combustion<fight_remains", "Pool as many Fire Blasts as possible for Combustion. Subtract out of the fractional component of the number of Fire Blasts that will naturally recharge during the Combustion phase because pooling anything past that will not grant an extra Fire Blast during Combustion." );
  default_->add_action( "call_action_list,name=combustion_phase,if=variable.time_to_combustion<=0|buff.combustion.up|variable.time_to_combustion<variable.combustion_precast_time&cooldown.combustion.remains<variable.combustion_precast_time" );
  default_->add_action( "rune_of_power,if=buff.combustion.down&buff.rune_of_power.down&!buff.hyperthermia.react&(variable.time_to_combustion>=buff.rune_of_power.duration&variable.time_to_combustion>action.fire_blast.full_recharge_time|variable.time_to_combustion>fight_remains)&(!talent.sun_kings_blessing|active_enemies>=variable.hard_cast_flamestrike|buff.sun_kings_blessing_ready.up|buff.sun_kings_blessing.react>=buff.sun_kings_blessing.max_stack-1|fight_remains<buff.rune_of_power.duration|firestarter.active)" );
  default_->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=fire_blast_pooling,value=searing_touch.active&action.fire_blast.full_recharge_time>3*gcd.max,if=!variable.fire_blast_pooling&talent.sun_kings_blessing", "Adjust the variable that controls Fire Blast usage to save Fire Blasts while Searing Touch is active with Sun King's Blessing." );
  default_->add_action( "variable,name=phoenix_pooling,if=active_enemies<variable.combustion_flamestrike,value=(variable.time_to_combustion+buff.combustion.duration-5<action.phoenix_flames.full_recharge_time+cooldown.phoenix_flames.duration-action.shifting_power.full_reduction*variable.shifting_power_before_combustion&variable.time_to_combustion<fight_remains|talent.sun_kings_blessing|time<5)&!talent.alexstraszas_fury", "Variable that controls Phoenix Flames usage to ensure its charges are pooled for Combustion. Only use Phoenix Flames outside of Combustion when full charges can be obtained during the next Combustion." );
  default_->add_action( "variable,name=phoenix_pooling,if=active_enemies>=variable.combustion_flamestrike,value=(variable.time_to_combustion<action.phoenix_flames.full_recharge_time-action.shifting_power.full_reduction*variable.shifting_power_before_combustion&variable.time_to_combustion<fight_remains|talent.sun_kings_blessing|time<5)&!talent.alexstraszas_fury", "When using Flamestrike in Combustion, save as many charges as possible for Combustion without capping." );
  default_->add_action( "call_action_list,name=rop_phase,if=buff.rune_of_power.up&buff.combustion.down&variable.time_to_combustion>0" );
  default_->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=fire_blast_pooling,value=(!talent.sun_kings_blessing|buff.sun_kings_blessing.stack>buff.sun_kings_blessing.max_stack-1)&cooldown.rune_of_power.remains<action.fire_blast.full_recharge_time-action.shifting_power.full_reduction*(variable.shifting_power_before_combustion&cooldown.shifting_power.remains<cooldown.rune_of_power.remains)&cooldown.rune_of_power.remains<fight_remains,if=!variable.fire_blast_pooling&talent.rune_of_power&buff.rune_of_power.down", "Adjust the variable that controls Fire Blast usage to ensure its charges are also pooled for Rune of Power." );
  default_->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=!variable.fire_blast_pooling&variable.time_to_combustion>0&active_enemies>=variable.hard_cast_flamestrike&!firestarter.active&!buff.hot_streak.react&(buff.heating_up.react&action.flamestrike.execute_remains<0.5|charges_fractional>=2)", "When Hardcasting Flamestrike, Fire Blasts should be used to generate Hot Streaks and to extend Feel the Burn." );
  default_->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=firestarter.active&variable.time_to_combustion>0&!variable.fire_blast_pooling&(!action.fireball.executing&!action.pyroblast.in_flight&buff.heating_up.react|action.fireball.executing&!buff.hot_streak.react|action.pyroblast.in_flight&buff.heating_up.react&!buff.hot_streak.react)", "During Firestarter, Fire Blasts are used similarly to during Combustion. Generally, they are used to generate Hot Streaks when crits will not be wasted and with Feel the Burn, they should be spread out to maintain the Feel the Burn buff." );
  default_->add_action( "fire_blast,use_while_casting=1,if=action.shifting_power.executing&full_recharge_time<action.shifting_power.tick_reduction", "Avoid capping Fire Blast charges while channeling Shifting Power" );
  default_->add_action( "call_action_list,name=standard_rotation,if=variable.time_to_combustion>0&buff.rune_of_power.down&buff.combustion.down" );
  default_->add_action( "ice_nova,if=!searing_touch.active", "Ice Nova can be used during movement when Searing Touch is not active." );
  default_->add_action( "scorch" );

  active_talents->add_action( "living_bomb,if=active_enemies>1&buff.combustion.down&(variable.time_to_combustion>cooldown.living_bomb.duration|variable.time_to_combustion<=0)" );
  active_talents->add_action( "meteor,if=variable.time_to_combustion<=0|buff.combustion.remains>travel_time|(cooldown.meteor.duration<variable.time_to_combustion&!talent.rune_of_power)|talent.rune_of_power&buff.rune_of_power.up&variable.time_to_combustion>action.meteor.cooldown|fight_remains<variable.time_to_combustion" );
  active_talents->add_action( "dragons_breath,if=talent.alexstraszas_fury&(buff.combustion.down&!buff.hot_streak.react)" );

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

  combustion_phase->add_action( "lights_judgment,if=buff.combustion.down" );
  combustion_phase->add_action( "bag_of_tricks,if=buff.combustion.down" );
  combustion_phase->add_action( "living_bomb,if=active_enemies>1&buff.combustion.down" );
  combustion_phase->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=extended_combustion_remains,value=buff.combustion.remains+buff.combustion.duration*(cooldown.combustion.remains<buff.combustion.remains)", "Estimate how long Combustion will last thanks to Sun King's Blessing to determine how Fire Blasts should be used." );
  combustion_phase->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=extended_combustion_remains,op=add,value=variable.skb_duration,if=talent.sun_kings_blessing&(buff.sun_kings_blessing_ready.up|variable.extended_combustion_remains>gcd.remains+1.5*gcd.max*(buff.sun_kings_blessing.max_stack-buff.sun_kings_blessing.stack))", "Adds the duration of the Sun King's Blessing Combustion to the end of the current Combustion if the cast would start during this Combustion." );
  combustion_phase->add_action( "call_action_list,name=combustion_cooldowns,if=buff.combustion.remains>variable.skb_duration|fight_remains<20", "Other cooldowns that should be used with Combustion should only be used with an actual Combustion cast and not with a Sun King's Blessing proc." );
  combustion_phase->add_action( "use_item,name=hyperthread_wristwraps,if=hyperthread_wristwraps.fire_blast>=2&action.fire_blast.charges=0" );
  combustion_phase->add_action( "use_item,name=neural_synapse_enhancer,if=variable.time_to_combustion>60" );
  combustion_phase->add_action( "call_action_list,name=active_talents" );
  combustion_phase->add_action( "cancel_buff,name=sun_kings_blessing,if=buff.combustion.down&buff.sun_kings_blessing.stack>2&talent.rune_of_power&cooldown.rune_of_power.remains<20", "If Sun King's Blessing stacks are high, cancel them before Combustion so that the Sun King's Blessing proc can be safely delayed until after Combustion without risk of expiration." );
  combustion_phase->add_action( "flamestrike,if=buff.combustion.down&cooldown.combustion.remains<cast_time&active_enemies>=variable.combustion_flamestrike", "If Combustion is down, precast something before activating it." );
  combustion_phase->add_action( "pyroblast,if=buff.combustion.down&buff.sun_kings_blessing_ready.up&buff.sun_kings_blessing_ready.remains>cast_time&buff.sun_kings_blessing_ready.expiration_delay_remains=0" );
  combustion_phase->add_action( "pyroblast,if=buff.combustion.down&buff.pyroclasm.react&buff.pyroclasm.remains>cast_time" );
  combustion_phase->add_action( "fireball,if=buff.combustion.down&cooldown.combustion.remains<cast_time" );
  combustion_phase->add_action( "scorch,if=buff.combustion.down&cooldown.combustion.remains<cast_time" );
  combustion_phase->add_action( "combustion,use_off_gcd=1,use_while_casting=1,if=hot_streak_spells_in_flight=0&buff.combustion.down&variable.time_to_combustion<=0&(action.scorch.executing&action.scorch.execute_remains<variable.combustion_cast_remains|action.fireball.executing&action.fireball.execute_remains<variable.combustion_cast_remains|action.pyroblast.executing&action.pyroblast.execute_remains<variable.combustion_cast_remains|action.flamestrike.executing&action.flamestrike.execute_remains<variable.combustion_cast_remains)", "Combustion should be used when the precast is almost finished." );
  combustion_phase->add_action( "rune_of_power,if=buff.rune_of_power.down&variable.extended_combustion_remains>variable.skb_duration", "Rune of Power can be used in Combustion if it is down, but this should only be done if there is more Combustion time to benefit from than there would be from an SKB Combustion later." );
  combustion_phase->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=!talent.feel_the_burn&!variable.fire_blast_pooling&(buff.sun_kings_blessing_ready.down|action.pyroblast.executing)&buff.combustion.up&!buff.hyperthermia.react&!buff.hot_streak.react&hot_streak_spells_in_flight+buff.heating_up.react*(gcd.remains>0)<2", "Without Feel the Burn, just use Fire Blasts when they won't munch crits and when Hyperthermia is down." );
  combustion_phase->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=expected_fire_blasts,value=action.fire_blast.charges_fractional+(variable.extended_combustion_remains-buff.feel_the_burn.duration)%cooldown.fire_blast.duration,if=talent.feel_the_burn", "With Feel the Burn, Fire Blast use should be additionally constrained so that it is not be used unless Feel the Burn is about to expire or there are more than enough Fire Blasts to extend Feel the Burn to the end of Combustion." );
  combustion_phase->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=needed_fire_blasts,value=ceil(variable.extended_combustion_remains%(buff.feel_the_burn.duration-gcd.max)),if=talent.feel_the_burn" );
  combustion_phase->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=use_shifting_power,value=firestarter.remains<variable.extended_combustion_remains&(talent.feel_the_burn&variable.expected_fire_blasts<variable.needed_fire_blasts)&(!talent.rune_of_power|cooldown.rune_of_power.remains>variable.extended_combustion_remains)|active_enemies>=variable.combustion_shifting_power,if=talent.shifting_power", "Use Shifting Power during Combustion when there are not enough Fire Blasts available to fully extend Feel the Burn and only when Rune of Power is on cooldown." );
  combustion_phase->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=talent.feel_the_burn&!variable.fire_blast_pooling&(buff.sun_kings_blessing_ready.down|action.pyroblast.executing)&(!equipped.hyperthread_wristwraps|!cooldown.hyperthread_wristwraps_300142.ready|charges>1|hyperthread_wristwraps.fire_blast.first_remains>1)&(variable.expected_fire_blasts>=variable.needed_fire_blasts|buff.combustion.remains<gcd.max|variable.extended_combustion_remains<=buff.feel_the_burn.duration|buff.feel_the_burn.stack<2|buff.feel_the_burn.remains<gcd.max|cooldown.shifting_power.ready&variable.use_shifting_power|equipped.hyperthread_wristwraps&cooldown.hyperthread_wristwraps_300142.ready)&buff.combustion.up&(!buff.hyperthermia.react|buff.feel_the_burn.remains<0.5)&!buff.hot_streak.react&hot_streak_spells_in_flight+buff.heating_up.react*(gcd.remains>0)<2" );
  combustion_phase->add_action( "flamestrike,if=(buff.hot_streak.react&active_enemies>=variable.combustion_flamestrike)|(buff.hyperthermia.react&active_enemies>=variable.combustion_flamestrike-talent.hyperthermia)", "Spend Hot Streaks during Combustion at high priority." );
  combustion_phase->add_action( "pyroblast,if=buff.hyperthermia.react" );
  combustion_phase->add_action( "pyroblast,if=buff.hot_streak.react&buff.combustion.up" );
  combustion_phase->add_action( "pyroblast,if=prev_gcd.1.scorch&buff.heating_up.react&active_enemies<variable.combustion_flamestrike&buff.combustion.up" );
  combustion_phase->add_action( "shifting_power,if=variable.use_shifting_power&buff.combustion.up&!action.fire_blast.charges&(action.phoenix_flames.charges<action.phoenix_flames.max_charges|talent.alexstraszas_fury)", "Using Shifting Power during Combustion to restore Fire Blast and Phoenix Flame charges can be beneficial, but usually only on AoE." );
  combustion_phase->add_action( "rune_of_power,if=buff.sun_kings_blessing_ready.up&buff.sun_kings_blessing_ready.remains>execute_time+action.flamestrike.cast_time&buff.rune_of_power.remains<action.flamestrike.cast_time&active_enemies>=variable.combustion_flamestrike", "If a Sun King's Blessing proc would be used, Rune of Power should be used first if the existing Rune of Power will expire during the cast." );
  combustion_phase->add_action( "flamestrike,if=buff.sun_kings_blessing_ready.up&buff.sun_kings_blessing_ready.remains>cast_time&active_enemies>=variable.combustion_flamestrike&buff.sun_kings_blessing_ready.expiration_delay_remains=0" );
  combustion_phase->add_action( "rune_of_power,if=buff.sun_kings_blessing_ready.up&buff.sun_kings_blessing_ready.remains>execute_time+action.pyroblast.cast_time&buff.rune_of_power.remains<action.pyroblast.cast_time" );
  combustion_phase->add_action( "pyroblast,if=buff.sun_kings_blessing_ready.up&buff.sun_kings_blessing_ready.remains>cast_time&buff.sun_kings_blessing_ready.expiration_delay_remains=0" );
  combustion_phase->add_action( "pyroblast,if=buff.pyroclasm.react&buff.pyroclasm.remains>cast_time&buff.combustion.remains>cast_time&active_enemies<variable.combustion_flamestrike&(!talent.feel_the_burn|buff.feel_the_burn.remains>execute_time|buff.heating_up.react+hot_streak_spells_in_flight<2)", "Pyroclasm procs should be used in Combustion at higher priority than Phoenix Flames and Scorch." );
  combustion_phase->add_action( "fireball,if=buff.combustion.remains>cast_time&buff.flame_accelerant.react" );
  combustion_phase->add_action( "phoenix_flames,if=!talent.alexstraszas_fury&buff.combustion.up&travel_time<buff.combustion.remains&buff.heating_up.react+hot_streak_spells_in_flight<2&(!talent.from_the_ashes|variable.extended_combustion_remains<10)", "Use Phoenix Flames and Scorch in Combustion to help generate Hot Streaks when Fire Blasts are not available or need to be conserved." );
  combustion_phase->add_action( "scorch,if=buff.combustion.remains>cast_time&cast_time>=gcd.max" );
  combustion_phase->add_action( "fireball,if=buff.combustion.remains>cast_time" );
  combustion_phase->add_action( "living_bomb,if=buff.combustion.remains<gcd.max&active_enemies>1", "If there isn't enough time left in Combustion for a Phoenix Flames or Scorch to hit inside of Combustion, use something else." );
  combustion_phase->add_action( "ice_nova,if=buff.combustion.remains<gcd.max" );

  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=combustion_ready_time,value=cooldown.combustion.remains*expected_kindling_reduction", "Helper variable that contains the actual estimated time that the next Combustion will be ready." );
  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=combustion_precast_time,value=action.fireball.cast_time*(active_enemies<variable.combustion_flamestrike)+action.flamestrike.cast_time*(active_enemies>=variable.combustion_flamestrike)-variable.combustion_cast_remains", "The cast time of the spell that will be precast into Combustion." );
  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=time_to_combustion,value=variable.combustion_ready_time" );
  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=time_to_combustion,op=max,value=firestarter.remains,if=talent.firestarter&!variable.firestarter_combustion", "Delay Combustion for after Firestarter unless variable.firestarter_combustion is set." );
  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=time_to_combustion,op=max,value=(buff.sun_kings_blessing.max_stack-buff.sun_kings_blessing.stack)*(action.fireball.execute_time+gcd.max),if=talent.sun_kings_blessing&firestarter.active&buff.sun_kings_blessing_ready.down", "Delay Combustion until SKB is ready during Firestarter" );
  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=time_to_combustion,op=max,value=cooldown.gladiators_badge_345228.remains,if=equipped.gladiators_badge&cooldown.gladiators_badge_345228.remains-20<variable.time_to_combustion", "Delay Combustion for Gladiators Badge, unless it would be delayed longer than 20 seconds." );
  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=time_to_combustion,op=max,value=buff.combustion.remains", "Delay Combustion until Combustion expires if it's up." );
  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=time_to_combustion,op=max,value=buff.rune_of_power.remains,if=talent.rune_of_power&buff.combustion.down", "Delay Combustion until RoP expires if it's up." );
  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=time_to_combustion,op=max,value=cooldown.rune_of_power.remains+buff.rune_of_power.duration,if=talent.rune_of_power&buff.combustion.down&cooldown.rune_of_power.remains+5<variable.time_to_combustion", "Delay Combustion for an extra Rune of Power if the Rune of Power would come off cooldown at least 5 seconds before Combustion would." );
  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=time_to_combustion,op=max,value=raid_event.adds.in,if=raid_event.adds.exists&raid_event.adds.count>=3&raid_event.adds.duration>15", "Raid Events: Delay Combustion for add spawns of 3 or more adds that will last longer than 15 seconds. These values aren't necessarily optimal in all cases." );
  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=time_to_combustion,value=raid_event.vulnerable.in*!raid_event.vulnerable.up,if=raid_event.vulnerable.exists&variable.combustion_ready_time<raid_event.vulnerable.in", "Raid Events: Always use Combustion with vulnerability raid events, override any delays listed above to make sure it gets used here." );
  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=time_to_combustion,value=variable.combustion_ready_time,if=variable.combustion_ready_time+cooldown.combustion.duration*(1-(0.4+0.2*talent.firestarter)*talent.kindling)<=variable.time_to_combustion|variable.time_to_combustion>fight_remains-20", "Use the next Combustion on cooldown if it would not be expected to delay the scheduled one or the scheduled one would happen less than 20 seconds before the fight ends." );

  rop_phase->add_action( "flamestrike,if=active_enemies>=variable.hot_streak_flamestrike&(buff.hot_streak.react|buff.hyperthermia.react)" );
  rop_phase->add_action( "flamestrike,if=active_enemies>=variable.hard_cast_flamestrike&buff.sun_kings_blessing_ready.up&buff.sun_kings_blessing_ready.remains>cast_time&buff.sun_kings_blessing_ready.expiration_delay_remains=0" );
  rop_phase->add_action( "pyroblast,if=buff.sun_kings_blessing_ready.up&buff.sun_kings_blessing_ready.remains>cast_time&buff.sun_kings_blessing_ready.expiration_delay_remains=0" );
  rop_phase->add_action( "pyroblast,if=buff.hyperthermia.react" );
  rop_phase->add_action( "pyroblast,if=buff.hot_streak.react" );
  rop_phase->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=!variable.fire_blast_pooling&buff.sun_kings_blessing_ready.down&active_enemies<variable.hard_cast_flamestrike&!firestarter.active&(!buff.heating_up.react&!buff.hot_streak.react&!prev_off_gcd.fire_blast&(action.fire_blast.charges>=2|(talent.alexstraszas_fury&cooldown.dragons_breath.ready)|searing_touch.active))", "Use one Fire Blast early in RoP if you don't have either Heating Up or Hot Streak yet and either: (a) have more than two already, (b) have Alexstrasza's Fury ready to use, or (c) Searing Touch is active. Don't do this while hard casting Flamestrikes or when Sun King's Blessing is ready." );
  rop_phase->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=!variable.fire_blast_pooling&!firestarter.active&buff.sun_kings_blessing_ready.down&(((action.fireball.executing&(action.fireball.execute_remains<0.5|!talent.hyperthermia)|action.pyroblast.executing&(action.pyroblast.execute_remains<0.5|!talent.hyperthermia))&buff.heating_up.react)|(searing_touch.active&(buff.heating_up.react&!action.scorch.executing|!buff.hot_streak.react&!buff.heating_up.react&action.scorch.executing&!hot_streak_spells_in_flight)))", "Use Fire Blast either during a Fireball/Pyroblast cast when Heating Up is active or during execute with Searing Touch." );
  rop_phase->add_action( "call_action_list,name=active_talents" );
  rop_phase->add_action( "pyroblast,if=buff.pyroclasm.react&cast_time<buff.pyroclasm.remains&cast_time<buff.rune_of_power.remains&(!talent.sun_kings_blessing|buff.pyroclasm.remains<action.fireball.cast_time+cast_time*buff.pyroclasm.react)" );
  rop_phase->add_action( "pyroblast,if=prev_gcd.1.scorch&buff.heating_up.react&searing_touch.active&active_enemies<variable.hot_streak_flamestrike" );
  rop_phase->add_action( "phoenix_flames,if=!variable.phoenix_pooling&(!hot_streak_spells_in_flight&(buff.heating_up.react|charges>1))" );
  rop_phase->add_action( "scorch,if=searing_touch.active" );
  rop_phase->add_action( "dragons_breath,if=active_enemies>2" );
  rop_phase->add_action( "arcane_explosion,if=active_enemies>=variable.arcane_explosion&mana.pct>=variable.arcane_explosion_mana" );
  rop_phase->add_action( "flamestrike,if=active_enemies>=variable.hard_cast_flamestrike", "With enough targets, it is a gain to cast Flamestrike as filler instead of Fireball." );
  rop_phase->add_action( "pyroblast,if=talent.tempered_flames&!buff.flame_accelerant.react" );
  rop_phase->add_action( "fireball" );

  standard_rotation->add_action( "flamestrike,if=active_enemies>=variable.hot_streak_flamestrike&(buff.hot_streak.react|buff.hyperthermia.react)" );
  standard_rotation->add_action( "pyroblast,if=buff.hyperthermia.react" );
  standard_rotation->add_action( "pyroblast,if=buff.hot_streak.react&buff.hot_streak.remains<action.fireball.execute_time" );
  standard_rotation->add_action( "pyroblast,if=buff.hot_streak.react&(prev_gcd.1.fireball|firestarter.active|action.pyroblast.in_flight)" );
  standard_rotation->add_action( "flamestrike,if=active_enemies>=variable.hard_cast_flamestrike&buff.sun_kings_blessing_ready.up&(cooldown.rune_of_power.remains+action.rune_of_power.execute_time+cast_time>buff.sun_kings_blessing_ready.remains|!talent.rune_of_power)&variable.time_to_combustion+cast_time>buff.sun_kings_blessing_ready.remains&buff.sun_kings_blessing_ready.expiration_delay_remains=0", "Try to get SKB procs inside RoP phases or Combustion phases when possible." );
  standard_rotation->add_action( "pyroblast,if=buff.sun_kings_blessing_ready.up&(cooldown.rune_of_power.remains+action.rune_of_power.execute_time+cast_time>buff.sun_kings_blessing_ready.remains|!talent.rune_of_power)&variable.time_to_combustion+cast_time>buff.sun_kings_blessing_ready.remains&buff.sun_kings_blessing_ready.expiration_delay_remains=0" );
  standard_rotation->add_action( "pyroblast,if=buff.hot_streak.react&searing_touch.active" );
  standard_rotation->add_action( "pyroblast,if=buff.pyroclasm.react&cast_time<buff.pyroclasm.remains&(!talent.sun_kings_blessing|buff.pyroclasm.remains<action.fireball.cast_time+cast_time*buff.pyroclasm.react)" );
  standard_rotation->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=!firestarter.active&!variable.fire_blast_pooling&buff.sun_kings_blessing_ready.down&(((action.fireball.executing&(action.fireball.execute_remains<0.5|!talent.hyperthermia)|action.pyroblast.executing&(action.pyroblast.execute_remains<0.5|!talent.hyperthermia))&buff.heating_up.react)|(searing_touch.active&(buff.heating_up.react&!action.scorch.executing|!buff.hot_streak.react&!buff.heating_up.react&action.scorch.executing&!hot_streak_spells_in_flight)))", "During the standard rotation, only use Fire Blasts when they are not being pooled for RoP or Combustion. Use Fire Blast either during a Fireball/Pyroblast cast when Heating Up is active or during execute with Searing Touch." );
  standard_rotation->add_action( "pyroblast,if=prev_gcd.1.scorch&buff.heating_up.react&searing_touch.active&active_enemies<variable.hot_streak_flamestrike" );
  standard_rotation->add_action( "phoenix_flames,if=!variable.phoenix_pooling&(!hot_streak_spells_in_flight&(buff.heating_up.react))" );
  standard_rotation->add_action( "call_action_list,name=active_talents" );
  standard_rotation->add_action( "dragons_breath,if=active_enemies>1" );
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
  action_priority_list_t* st = p->get_action_priority_list( "st" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "summon_water_elemental" );
  precombat->add_action( "variable,name=use_fof,default=1,op=reset" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "blizzard,if=active_enemies>=2" );
  precombat->add_action( "frostbolt,if=active_enemies=1" );

  default_->add_action( "counterspell" );
  default_->add_action( "water_jet,if=cooldown.flurry.charges_fractional<1" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=aoe,if=active_enemies>=7|active_enemies>=3&talent.ice_caller" );
  default_->add_action( "call_action_list,name=st,if=active_enemies<7&(active_enemies<3|!talent.ice_caller)" );
  default_->add_action( "call_action_list,name=movement" );

  aoe->add_action( "cone_of_cold,if=buff.snowstorm.stack=buff.snowstorm.max_stack&debuff.frozen.up&(prev_gcd.1.frost_nova|prev_gcd.1.ice_nova|prev_off_gcd.freeze)" );
  aoe->add_action( "frozen_orb" );
  aoe->add_action( "blizzard" );
  aoe->add_action( "comet_storm" );
  aoe->add_action( "freeze,if=(target.level<level+3|target.is_add)&(!talent.snowstorm&debuff.frozen.down|cooldown.cone_of_cold.ready&buff.snowstorm.stack=buff.snowstorm.max_stack)" );
  aoe->add_action( "ice_nova,if=(target.level<level+3|target.is_add)&(prev_gcd.1.comet_storm|cooldown.cone_of_cold.ready&buff.snowstorm.stack=buff.snowstorm.max_stack&gcd.max<1)" );
  aoe->add_action( "frost_nova,if=(target.level<level+3|target.is_add)&active_enemies>=5&cooldown.cone_of_cold.ready&buff.snowstorm.stack=buff.snowstorm.max_stack&gcd.max<1" );
  aoe->add_action( "cone_of_cold,if=buff.snowstorm.stack=buff.snowstorm.max_stack" );
  aoe->add_action( "flurry,if=cooldown_react&remaining_winters_chill=0&(!variable.use_fof|debuff.winters_chill.down&(prev_gcd.1.frostbolt|(active_enemies>=7|charges=max_charges)&buff.fingers_of_frost.react=0))" );
  aoe->add_action( "ice_lance,if=buff.fingers_of_frost.react|debuff.frozen.remains>travel_time|remaining_winters_chill" );
  aoe->add_action( "shifting_power" );
  aoe->add_action( "ice_nova" );
  aoe->add_action( "meteor" );
  aoe->add_action( "dragons_breath,if=active_enemies>=7" );
  aoe->add_action( "arcane_explosion,if=mana.pct>30&active_enemies>=7" );
  aoe->add_action( "ebonbolt" );
  aoe->add_action( "frostbolt" );

  cds->add_action( "time_warp,if=buff.exhaustion.up&buff.bloodlust.down" );
  cds->add_action( "potion,if=prev_off_gcd.icy_veins|fight_remains<60" );
  cds->add_action( "icy_veins,if=buff.rune_of_power.down&(buff.icy_veins.down|talent.rune_of_power)" );
  cds->add_action( "rune_of_power,if=buff.rune_of_power.down&cooldown.icy_veins.remains>20" );
  cds->add_action( "use_items" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.power_infusion.down" );
  cds->add_action( "invoke_external_buff,name=blessing_of_summer,if=buff.blessing_of_summer.down" );
  cds->add_action( "blood_fury" );
  cds->add_action( "berserking" );
  cds->add_action( "lights_judgment" );
  cds->add_action( "fireblood" );
  cds->add_action( "ancestral_call" );

  movement->add_action( "blink_any,if=movement.distance>10" );
  movement->add_action( "ice_floes,if=buff.ice_floes.down" );
  movement->add_action( "ice_nova" );
  movement->add_action( "arcane_explosion,if=mana.pct>30&active_enemies>=2" );
  movement->add_action( "fire_blast" );
  movement->add_action( "ice_lance" );

  st->add_action( "meteor,if=prev_gcd.1.flurry" );
  st->add_action( "comet_storm,if=prev_gcd.1.flurry" );
  st->add_action( "flurry,if=cooldown_react&remaining_winters_chill=0&debuff.winters_chill.down&(prev_gcd.1.frostbolt|prev_gcd.1.glacial_spike)" );
  st->add_action( "ray_of_frost,if=remaining_winters_chill=1&buff.freezing_winds.down" );
  st->add_action( "glacial_spike,if=remaining_winters_chill" );
  st->add_action( "cone_of_cold,if=buff.snowstorm.stack=buff.snowstorm.max_stack&remaining_winters_chill" );
  st->add_action( "frozen_orb" );
  st->add_action( "blizzard,if=active_enemies>=2&talent.ice_caller&talent.freezing_rain" );
  st->add_action( "shifting_power,if=(variable.use_fof&buff.rune_of_power.down)|(!variable.use_fof&(buff.rune_of_power.down&buff.icy_veins.down|cooldown.icy_veins.remains<20))" );
  st->add_action( "ice_lance,if=(variable.use_fof&(buff.fingers_of_frost.react&!prev_gcd.1.glacial_spike|remaining_winters_chill))|(!variable.use_fof&(remaining_winters_chill=2|remaining_winters_chill=1&buff.brain_freeze.react|(remaining_winters_chill|buff.fingers_of_frost.react)&(buff.icy_veins.remains<10&cooldown.icy_veins.remains>30|buff.icy_veins.remains<15&cooldown.icy_veins.remains>70)))" );
  st->add_action( "ice_nova,if=active_enemies>=4" );
  st->add_action( "glacial_spike,if=action.flurry.cooldown_react" );
  st->add_action( "ebonbolt,if=cooldown.flurry.charges_fractional<1" );
  st->add_action( "bag_of_tricks" );
  st->add_action( "frostbolt" );
}
//frost_apl_end

}  // namespace mage_apl
