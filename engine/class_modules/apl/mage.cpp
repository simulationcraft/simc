#include "simulationcraft.hpp"
#include "class_modules/apl/mage.hpp"

namespace mage_apl {

std::string potion( const player_t* p )
{
  return p->true_level >= 50 ? "superior_battle_potion_of_intellect"
                             : "disabled";
}

std::string flask( const player_t* p )
{
  return p->true_level >= 50 ? "greater_flask_of_endless_fathoms"
                             : "disabled";
}

std::string food( const player_t* p )
{
  return p->true_level >= 50 ? "famine_evaluator_and_snack_table"
                             : "disabled";
}

std::string rune( const player_t* p )
{
  return p->true_level >= 50 ? "battle_scarred"
                             : "disabled";
}

//arcane_apl_start
void arcane( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* shared_cds = p->get_action_priority_list( "shared_cds" );
  action_priority_list_t* essences = p->get_action_priority_list( "essences" );
  action_priority_list_t* opener = p->get_action_priority_list( "opener" );
  action_priority_list_t* cooldowns = p->get_action_priority_list( "cooldowns" );
  action_priority_list_t* rotation = p->get_action_priority_list( "rotation" );
  action_priority_list_t* final_burn = p->get_action_priority_list( "final_burn" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* am_spam = p->get_action_priority_list( "am_spam" );
  action_priority_list_t* movement = p->get_action_priority_list( "movement" );

  precombat->add_action( "variable,name=prepull_evo,op=reset,default=0" );
  precombat->add_action( "variable,name=prepull_evo,op=set,value=1,if=variable.prepull_evo=0&runeforge.siphon_storm.equipped&active_enemies>2" );
  precombat->add_action( "variable,name=prepull_evo,op=set,value=1,if=variable.prepull_evo=0&runeforge.siphon_storm.equipped&covenant.necrolord.enabled&active_enemies>1" );
  precombat->add_action( "variable,name=prepull_evo,op=set,value=1,if=variable.prepull_evo=0&runeforge.siphon_storm.equipped&covenant.night_fae.enabled" );
  precombat->add_action( "variable,name=have_opened,op=reset,default=0" );
  precombat->add_action( "variable,name=have_opened,op=set,value=1,if=variable.have_opened=0&active_enemies>2" );
  precombat->add_action( "variable,name=have_opened,op=set,value=1,if=variable.have_opened=0&variable.prepull_evo=1" );
  precombat->add_action( "variable,name=final_burn,op=set,value=0" );
  precombat->add_action( "variable,name=rs_max_delay,op=reset,default=5" );
  precombat->add_action( "variable,name=ap_max_delay,op=reset,default=10" );
  precombat->add_action( "variable,name=rop_max_delay,op=reset,default=20" );
  precombat->add_action( "variable,name=totm_max_delay,op=reset,default=5" );
  precombat->add_action( "variable,name=totm_max_delay,op=set,value=3,if=variable.totm_max_delay=5&runeforge.disciplinary_command.equipped" );
  precombat->add_action( "variable,name=totm_max_delay,op=set,value=15,if=variable.totm_max_delay=5&covenant.night_fae.enabled" );
  precombat->add_action( "variable,name=totm_max_delay,op=set,value=15,if=variable.totm_max_delay=5&conduit.arcane_prodigy.enabled&active_enemies<3" );
  precombat->add_action( "variable,name=totm_max_delay,op=set,value=30,if=variable.totm_max_delay=5&essence.vision_of_perfection.minor" );
  precombat->add_action( "variable,name=barrage_mana_pct,op=reset,default=70" );
  precombat->add_action( "variable,name=barrage_mana_pct,op=set,value=40,if=variable.barrage_mana_pct=70&covenant.night_fae.enabled" );
  precombat->add_action( "variable,name=ap_minimum_mana_pct,op=reset,default=30" );
  precombat->add_action( "variable,name=ap_minimum_mana_pct,op=set,value=50,if=variable.ap_minimum_mana_pct=30&runeforge.disciplinary_command.equipped" );
  precombat->add_action( "variable,name=ap_minimum_mana_pct,op=set,value=50,if=variable.ap_minimum_mana_pct=30&runeforge.grisly_icicle.equipped" );
  precombat->add_action( "variable,name=totm_max_charges,op=reset,default=2" );
  precombat->add_action( "variable,name=aoe_totm_max_charges,op=reset,default=2" );
  precombat->add_action( "variable,name=am_spam,op=reset,default=0" );
  precombat->add_action( "variable,name=have_opened,op=set,value=1,if=variable.have_opened=0&variable.am_spam=1" );
  precombat->add_action( "variable,name=am_spam_evo_pct,op=reset,default=15" );
  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "arcane_familiar" );
  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "conjure_mana_gem" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "mirror_image" );
  precombat->add_action( "frostbolt,if=variable.prepull_evo<=0" );
  precombat->add_action( "evocation,if=variable.prepull_evo>0" );

  default_->add_action( "counterspell,if=target.debuff.casting.react" );
  default_->add_action( "call_action_list,name=shared_cds" );
  default_->add_action( "call_action_list,name=essences" );
  default_->add_action( "call_action_list,name=aoe,if=active_enemies>2" );
  default_->add_action( "call_action_list,name=opener,if=variable.have_opened<=0" );
  default_->add_action( "call_action_list,name=am_spam,if=variable.am_spam=1" );
  default_->add_action( "call_action_list,name=cooldowns" );
  default_->add_action( "call_action_list,name=rotation,if=variable.final_burn=0" );
  default_->add_action( "call_action_list,name=final_burn,if=variable.final_burn=1" );
  default_->add_action( "call_action_list,name=movement" );

  shared_cds->add_action( "use_mana_gem,if=(talent.enlightened.enabled&mana.pct<=80&mana.pct>=65)|(!talent.enlightened.enabled&mana.pct<=85)" );
  shared_cds->add_action( "use_items,if=buff.arcane_power.up" );
  shared_cds->add_action( "potion,if=buff.arcane_power.up" );
  shared_cds->add_action( "time_warp,if=runeforge.temporal_warp.equipped&buff.exhaustion.up" );
  shared_cds->add_action( "lights_judgment,if=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down" );
  shared_cds->add_action( "bag_of_tricks,if=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down" );
  shared_cds->add_action( "berserking,if=buff.arcane_power.up" );
  shared_cds->add_action( "blood_fury,if=buff.arcane_power.up" );
  shared_cds->add_action( "fireblood,if=buff.arcane_power.up" );
  shared_cds->add_action( "ancestral_call,if=buff.arcane_power.up" );

  essences->add_action( "blood_of_the_enemy,if=cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack<=variable.totm_max_charges&cooldown.arcane_power.remains<=gcd|fight_remains<cooldown.arcane_power.remains" );
  essences->add_action( "blood_of_the_enemy,if=cooldown.arcane_power.remains=0&(!talent.enlightened.enabled|(talent.enlightened.enabled&mana.pct>=70|variable.am_spam=1))&((cooldown.touch_of_the_magi.remains>variable.ap_max_delay&(buff.arcane_charge.stack=buff.arcane_charge.max_stack|variable.am_spam=1))|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack=0))&buff.rune_of_power.down&mana.pct>=variable.ap_minimum_mana_pct" );
  essences->add_action( "worldvein_resonance,if=cooldown.arcane_power.remains>=50&cooldown.touch_of_the_magi.remains<=gcd&buff.arcane_charge.stack<=variable.totm_max_charges&talent.rune_of_power.enabled&cooldown.rune_of_power.remains<=gcd&cooldown.arcane_power.remains>variable.totm_max_delay" );
  essences->add_action( "worldvein_resonance,if=cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack<=variable.totm_max_charges&cooldown.arcane_power.remains<=gcd|fight_remains<cooldown.arcane_power.remains" );
  essences->add_action( "worldvein_resonance,if=cooldown.arcane_power.remains=0&(!talent.enlightened.enabled|(talent.enlightened.enabled&mana.pct>=70|variable.am_spam=1))&((cooldown.touch_of_the_magi.remains>variable.ap_max_delay&(buff.arcane_charge.stack=buff.arcane_charge.max_stack|variable.am_spam=1))|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack=0))&buff.rune_of_power.down&mana.pct>=variable.ap_minimum_mana_pct" );
  essences->add_action( "guardian_of_azeroth,if=cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack<=variable.totm_max_charges&cooldown.arcane_power.remains<=gcd|fight_remains<cooldown.arcane_power.remains" );
  essences->add_action( "guardian_of_azeroth,if=cooldown.arcane_power.remains=0&(!talent.enlightened.enabled|(talent.enlightened.enabled&mana.pct>=70|variable.am_spam=1))&((cooldown.touch_of_the_magi.remains>variable.ap_max_delay&(buff.arcane_charge.stack=buff.arcane_charge.max_stack|variable.am_spam=1))|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack=0))&buff.rune_of_power.down&mana.pct>=variable.ap_minimum_mana_pct" );
  essences->add_action( "concentrated_flame,line_cd=6,if=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down&mana.time_to_max>=execute_time" );
  essences->add_action( "reaping_flames,if=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down&mana.time_to_max>=execute_time" );
  essences->add_action( "focused_azerite_beam,if=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down" );
  essences->add_action( "purifying_blast,if=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down" );
  essences->add_action( "ripple_in_space,if=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down" );
  essences->add_action( "the_unbound_force,if=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down" );
  essences->add_action( "memory_of_lucid_dreams,if=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down" );

  opener->add_action( "variable,name=have_opened,op=set,value=1,if=prev_gcd.1.evocation" );
  opener->add_action( "fire_blast,if=runeforge.disciplinary_command.equipped&buff.disciplinary_command_frost.up" );
  opener->add_action( "frost_nova,if=runeforge.grisly_icicle.equipped&mana.pct>95" );
  opener->add_action( "mirrors_of_torment" );
  opener->add_action( "deathborne" );
  opener->add_action( "radiant_spark,if=mana.pct>40" );
  opener->add_action( "cancel_action,if=action.shifting_power.channeling&gcd.remains=0" );
  opener->add_action( "shifting_power,if=soulbind.field_of_blossoms.enabled" );
  opener->add_action( "touch_of_the_magi" );
  opener->add_action( "arcane_power" );
  opener->add_action( "rune_of_power,if=buff.rune_of_power.down" );
  opener->add_action( "presence_of_mind" );
  opener->add_action( "arcane_blast,if=dot.radiant_spark.remains>5|debuff.radiant_spark_vulnerability.stack>0" );
  opener->add_action( "arcane_blast,if=buff.presence_of_mind.up&debuff.touch_of_the_magi.up&debuff.touch_of_the_magi.remains<=action.arcane_blast.execute_time" );
  opener->add_action( "arcane_barrage,if=buff.arcane_power.up&buff.arcane_power.remains<=gcd&buff.arcane_charge.stack=buff.arcane_charge.max_stack" );
  opener->add_action( "arcane_missiles,if=debuff.touch_of_the_magi.up&talent.arcane_echo.enabled&buff.deathborne.down&debuff.touch_of_the_magi.remains>action.arcane_missiles.execute_time&(!azerite.arcane_pummeling.enabled|buff.clearcasting_channel.down),chain=1" );
  opener->add_action( "arcane_missiles,if=buff.clearcasting.react,chain=1" );
  opener->add_action( "arcane_orb,if=buff.arcane_charge.stack<=variable.totm_max_charges&(cooldown.arcane_power.remains>10|active_enemies<=2)" );
  opener->add_action( "arcane_blast,if=buff.rune_of_power.up|mana.pct>15" );
  opener->add_action( "evocation,if=buff.rune_of_power.down,interrupt_if=mana.pct>=85,interrupt_immediate=1" );
  opener->add_action( "arcane_barrage" );

  cooldowns->add_action( "frost_nova,if=runeforge.grisly_icicle.equipped&cooldown.arcane_power.remains>30&cooldown.touch_of_the_magi.remains=0&(buff.arcane_charge.stack<=variable.totm_max_charges&((talent.rune_of_power.enabled&cooldown.rune_of_power.remains<=gcd&cooldown.arcane_power.remains>variable.totm_max_delay)|(!talent.rune_of_power.enabled&cooldown.arcane_power.remains>variable.totm_max_delay)|cooldown.arcane_power.remains<=gcd))", "Prioritize using grisly icicle with ap. Use it with totm otherwise." );
  cooldowns->add_action( "frost_nova,if=runeforge.grisly_icicle.equipped&cooldown.arcane_power.remains=0&(!talent.enlightened.enabled|(talent.enlightened.enabled&mana.pct>=70))&((cooldown.touch_of_the_magi.remains>10&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack=0))&buff.rune_of_power.down&mana.pct>=variable.ap_minimum_mana_pct" );
  cooldowns->add_action( "frostbolt,if=runeforge.disciplinary_command.equipped&cooldown.buff_disciplinary_command.ready&buff.disciplinary_command_frost.down&(buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down)&cooldown.touch_of_the_magi.remains=0&(buff.arcane_charge.stack<=variable.totm_max_charges&((talent.rune_of_power.enabled&cooldown.rune_of_power.remains<=gcd&cooldown.arcane_power.remains>variable.totm_max_delay)|(!talent.rune_of_power.enabled&cooldown.arcane_power.remains>variable.totm_max_delay)|cooldown.arcane_power.remains<=gcd))" );
  cooldowns->add_action( "fire_blast,if=runeforge.disciplinary_command.equipped&cooldown.buff_disciplinary_command.ready&buff.disciplinary_command_fire.down&prev_gcd.1.frostbolt" );
  cooldowns->add_action( "mirrors_of_torment,if=cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack<=variable.totm_max_charges&cooldown.arcane_power.remains<=gcd", "Always use mirrors with ap. If totm is ready as well, make sure to cast it before totm." );
  cooldowns->add_action( "mirrors_of_torment,if=cooldown.arcane_power.remains=0&(!talent.enlightened.enabled|(talent.enlightened.enabled&mana.pct>=70))&((cooldown.touch_of_the_magi.remains>variable.ap_max_delay&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack=0))&buff.rune_of_power.down&mana.pct>=variable.ap_minimum_mana_pct" );
  cooldowns->add_action( "deathborne,if=cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack<=variable.totm_max_charges&cooldown.arcane_power.remains<=gcd", "Always use deathborne with ap. If totm is ready as well, make sure to cast it before totm." );
  cooldowns->add_action( "deathborne,if=cooldown.arcane_power.remains=0&(!talent.enlightened.enabled|(talent.enlightened.enabled&mana.pct>=70))&((cooldown.touch_of_the_magi.remains>10&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack=0))&buff.rune_of_power.down&mana.pct>=variable.ap_minimum_mana_pct" );
  cooldowns->add_action( "radiant_spark,if=cooldown.touch_of_the_magi.remains>variable.rs_max_delay&cooldown.arcane_power.remains>variable.rs_max_delay&(talent.rune_of_power.enabled&cooldown.rune_of_power.remains<=gcd|talent.rune_of_power.enabled&cooldown.rune_of_power.remains>variable.rs_max_delay|!talent.rune_of_power.enabled)&buff.arcane_charge.stack>2&debuff.touch_of_the_magi.down", "Use spark if totm and ap are on cd and won't be up for longer than the max delay, making sure we have at least two arcane charges and that totm wasn't just used." );
  cooldowns->add_action( "radiant_spark,if=cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack<=variable.totm_max_charges&cooldown.arcane_power.remains<=gcd", "Use spark with ap when possible. If totm is ready as well, make sure to cast it before totm." );
  cooldowns->add_action( "radiant_spark,if=cooldown.arcane_power.remains=0&((!talent.enlightened.enabled|(talent.enlightened.enabled&mana.pct>=70))&((cooldown.touch_of_the_magi.remains>variable.ap_max_delay&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack=0))&buff.rune_of_power.down&mana.pct>=variable.ap_minimum_mana_pct)" );
  cooldowns->add_action( "touch_of_the_magi,if=cooldown.arcane_power.remains<50&essence.vision_of_perfection.minor" );
  cooldowns->add_action( "touch_of_the_magi,if=buff.arcane_charge.stack<=variable.totm_max_charges&talent.rune_of_power.enabled&cooldown.rune_of_power.remains<=gcd&cooldown.arcane_power.remains>variable.totm_max_delay&covenant.kyrian.enabled&cooldown.radiant_spark.remains<=8", "Kyrian: Use totm if ap is on cd and won't be up for longer than the max delay. Align with rop if the talent is taken. Hold a bit to make sure we can RS immediately after totm ends" );
  cooldowns->add_action( "touch_of_the_magi,if=buff.arcane_charge.stack<=variable.totm_max_charges&talent.rune_of_power.enabled&cooldown.rune_of_power.remains<=gcd&cooldown.arcane_power.remains>variable.totm_max_delay&!covenant.kyrian.enabled", "Non-Kyrian: Use totm if ap is on cd and won't be up for longer than the max delay. Align with rop if the talent is taken." );
  cooldowns->add_action( "touch_of_the_magi,if=buff.arcane_charge.stack<=variable.totm_max_charges&!talent.rune_of_power.enabled&cooldown.arcane_power.remains>variable.totm_max_delay" );
  cooldowns->add_action( "touch_of_the_magi,if=buff.arcane_charge.stack<=variable.totm_max_charges&cooldown.arcane_power.remains<=gcd" );
  cooldowns->add_action( "arcane_power,if=(!talent.enlightened.enabled|(talent.enlightened.enabled&mana.pct>=70))&cooldown.touch_of_the_magi.remains>variable.ap_max_delay&buff.arcane_charge.stack=buff.arcane_charge.max_stack&buff.rune_of_power.down&mana.pct>=variable.ap_minimum_mana_pct", "Use ap if totm is on cd and won't be up for longer than the max delay, making sure that we have enough mana and that there is not already a rune of power down." );
  cooldowns->add_action( "rune_of_power,if=buff.rune_of_power.down&cooldown.touch_of_the_magi.remains>variable.rop_max_delay&buff.arcane_charge.stack=buff.arcane_charge.max_stack&(cooldown.arcane_power.remains>15|debuff.touch_of_the_magi.up)", "Use rop if totm is on cd and won't be up for longer than the max delay, making sure there isn't already a rune down and that ap won't become available during rune." );
  cooldowns->add_action( "presence_of_mind,if=buff.arcane_charge.stack=0&covenant.kyrian.enabled", "Kyrian: RS is mana hungry and AB4s are too expensive to use pom to squeeze an extra ab in the totm window. Let's use it to make low charge ABs instant." );
  cooldowns->add_action( "presence_of_mind,if=debuff.touch_of_the_magi.up&!covenant.kyrian.enabled", "Non-Kyrian: Use pom to squeeze an extra ab in the totm window." );

  rotation->add_action( "variable,name=final_burn,op=set,value=1,if=buff.arcane_charge.stack=buff.arcane_charge.max_stack&!buff.rule_of_threes.up&fight_remains<=((mana%action.arcane_blast.cost)*action.arcane_blast.execute_time)" );
  rotation->add_action( "arcane_barrage,if=cooldown.touch_of_the_magi.remains=0&(buff.arcane_charge.stack>variable.totm_max_charges&talent.rune_of_power.enabled&cooldown.rune_of_power.remains<=gcd&cooldown.arcane_power.remains>variable.totm_max_delay&covenant.kyrian.enabled&cooldown.radiant_spark.remains<=8)" );
  rotation->add_action( "arcane_barrage,if=cooldown.touch_of_the_magi.remains=0&(buff.arcane_charge.stack>variable.totm_max_charges&talent.rune_of_power.enabled&cooldown.rune_of_power.remains<=gcd&cooldown.arcane_power.remains>variable.totm_max_delay&!covenant.kyrian.enabled)" );
  rotation->add_action( "arcane_barrage,if=cooldown.touch_of_the_magi.remains=0&(buff.arcane_charge.stack>variable.totm_max_charges&!talent.rune_of_power.enabled&cooldown.arcane_power.remains>variable.totm_max_delay)" );
  rotation->add_action( "arcane_barrage,if=cooldown.touch_of_the_magi.remains=0&(buff.arcane_charge.stack>variable.totm_max_charges&cooldown.arcane_power.remains<=gcd)" );
  rotation->add_action( "strict_sequence,if=debuff.radiant_spark_vulnerability.stack=debuff.radiant_spark_vulnerability.max_stack&buff.arcane_power.down&buff.rune_of_power.down,name=last_spark_stack:arcane_blast:arcane_barrage" );
  rotation->add_action( "arcane_barrage,if=debuff.radiant_spark_vulnerability.stack=debuff.radiant_spark_vulnerability.max_stack&(buff.arcane_power.down|buff.arcane_power.remains<=gcd)&(buff.rune_of_power.down|buff.rune_of_power.remains<=gcd)" );
  rotation->add_action( "arcane_blast,if=dot.radiant_spark.remains>5|debuff.radiant_spark_vulnerability.stack>0" );
  rotation->add_action( "arcane_blast,if=buff.presence_of_mind.up&debuff.touch_of_the_magi.up&debuff.touch_of_the_magi.remains<=action.arcane_blast.execute_time" );
  rotation->add_action( "arcane_missiles,if=debuff.touch_of_the_magi.up&talent.arcane_echo.enabled&buff.deathborne.down&(debuff.touch_of_the_magi.remains>action.arcane_missiles.execute_time|cooldown.presence_of_mind.remains>0|covenant.kyrian.enabled)&(!azerite.arcane_pummeling.enabled|buff.clearcasting_channel.down),chain=1" );
  rotation->add_action( "arcane_missiles,if=buff.clearcasting.react&buff.expanded_potential.up" );
  rotation->add_action( "arcane_missiles,if=buff.clearcasting.react&(buff.arcane_power.up|buff.rune_of_power.up|debuff.touch_of_the_magi.remains>action.arcane_missiles.execute_time),chain=1" );
  rotation->add_action( "arcane_missiles,if=buff.clearcasting.react&buff.clearcasting.stack=buff.clearcasting.max_stack,chain=1" );
  rotation->add_action( "arcane_missiles,if=buff.clearcasting.react&buff.clearcasting.remains<=((buff.clearcasting.stack*action.arcane_missiles.execute_time)+gcd),chain=1" );
  rotation->add_action( "nether_tempest,if=(refreshable|!ticking)&buff.arcane_charge.stack=buff.arcane_charge.max_stack&buff.arcane_power.down&debuff.touch_of_the_magi.down" );
  rotation->add_action( "arcane_orb,if=buff.arcane_charge.stack<=variable.totm_max_charges" );
  rotation->add_action( "supernova,if=mana.pct<=95&buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down" );
  rotation->add_action( "shifting_power,if=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down&cooldown.evocation.remains>0&cooldown.arcane_power.remains>0&cooldown.touch_of_the_magi.remains>0&(!talent.rune_of_power.enabled|(talent.rune_of_power.enabled&cooldown.rune_of_power.remains>0))" );
  rotation->add_action( "arcane_blast,if=buff.rule_of_threes.up&buff.arcane_charge.stack>3" );
  rotation->add_action( "arcane_barrage,if=mana.pct<variable.barrage_mana_pct&cooldown.evocation.remains>0&buff.arcane_power.down&buff.arcane_charge.stack=buff.arcane_charge.max_stack&essence.vision_of_perfection.minor" );
  rotation->add_action( "arcane_barrage,if=cooldown.touch_of_the_magi.remains=0&(cooldown.rune_of_power.remains=0|cooldown.arcane_power.remains=0)&buff.arcane_charge.stack=buff.arcane_charge.max_stack" );
  rotation->add_action( "arcane_barrage,if=mana.pct<=variable.barrage_mana_pct&buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down&buff.arcane_charge.stack=buff.arcane_charge.max_stack&cooldown.evocation.remains>0" );
  rotation->add_action( "arcane_barrage,if=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down&buff.arcane_charge.stack=buff.arcane_charge.max_stack&talent.arcane_orb.enabled&cooldown.arcane_orb.remains<=gcd&mana.pct<=90&cooldown.evocation.remains>0" );
  rotation->add_action( "arcane_barrage,if=buff.arcane_power.up&buff.arcane_power.remains<=gcd&buff.arcane_charge.stack=buff.arcane_charge.max_stack" );
  rotation->add_action( "arcane_barrage,if=buff.rune_of_power.up&buff.rune_of_power.remains<=gcd&buff.arcane_charge.stack=buff.arcane_charge.max_stack" );
  rotation->add_action( "arcane_barrage,if=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.up&debuff.touch_of_the_magi.remains<=gcd&buff.arcane_charge.stack=buff.arcane_charge.max_stack" );
  rotation->add_action( "arcane_blast" );
  rotation->add_action( "evocation,interrupt_if=mana.pct>=85,interrupt_immediate=1" );
  rotation->add_action( "arcane_barrage" );

  final_burn->add_action( "arcane_missiles,if=buff.clearcasting.react,chain=1" );
  final_burn->add_action( "arcane_blast" );
  final_burn->add_action( "arcane_barrage" );

  aoe->add_action( "frostbolt,if=runeforge.disciplinary_command.equipped&cooldown.buff_disciplinary_command.ready&buff.disciplinary_command_frost.down&(buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down)&cooldown.touch_of_the_magi.remains=0&(buff.arcane_charge.stack<=variable.aoe_totm_max_charges&((talent.rune_of_power.enabled&cooldown.rune_of_power.remains<=gcd&cooldown.arcane_power.remains>variable.totm_max_delay)|(!talent.rune_of_power.enabled&cooldown.arcane_power.remains>variable.totm_max_delay)|cooldown.arcane_power.remains<=gcd))" );
  aoe->add_action( "fire_blast,if=(runeforge.disciplinary_command.equipped&cooldown.buff_disciplinary_command.ready&buff.disciplinary_command_fire.down&prev_gcd.1.frostbolt)|(runeforge.disciplinary_command.equipped&time=0)" );
  aoe->add_action( "frost_nova,if=runeforge.grisly_icicle.equipped&cooldown.arcane_power.remains>30&cooldown.touch_of_the_magi.remains=0&(buff.arcane_charge.stack<=variable.aoe_totm_max_charges&((talent.rune_of_power.enabled&cooldown.rune_of_power.remains<=gcd&cooldown.arcane_power.remains>variable.totm_max_delay)|(!talent.rune_of_power.enabled&cooldown.arcane_power.remains>variable.totm_max_delay)|cooldown.arcane_power.remains<=gcd))" );
  aoe->add_action( "frost_nova,if=runeforge.grisly_icicle.equipped&cooldown.arcane_power.remains=0&(((cooldown.touch_of_the_magi.remains>variable.ap_max_delay&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack<=variable.aoe_totm_max_charges))&buff.rune_of_power.down)" );
  aoe->add_action( "touch_of_the_magi,if=runeforge.siphon_storm.equipped&prev_gcd.1.evocation" );
  aoe->add_action( "arcane_power,if=runeforge.siphon_storm.equipped&(prev_gcd.1.evocation|prev_gcd.1.touch_of_the_magi)" );
  aoe->add_action( "evocation,if=time>30&runeforge.siphon_storm.equipped&buff.arcane_charge.stack<=variable.aoe_totm_max_charges&cooldown.touch_of_the_magi.remains=0&cooldown.arcane_power.remains<=gcd" );
  aoe->add_action( "evocation,if=time>30&runeforge.siphon_storm.equipped&cooldown.arcane_power.remains=0&(((cooldown.touch_of_the_magi.remains>variable.ap_max_delay&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack<=variable.aoe_totm_max_charges))&buff.rune_of_power.down),interrupt_if=buff.siphon_storm.stack=buff.siphon_storm.max_stack,interrupt_immediate=1" );
  aoe->add_action( "mirrors_of_torment,if=(cooldown.arcane_power.remains>45|cooldown.arcane_power.remains<=3)&cooldown.touch_of_the_magi.remains=0&(buff.arcane_charge.stack<=variable.aoe_totm_max_charges&((talent.rune_of_power.enabled&cooldown.rune_of_power.remains<=gcd&cooldown.arcane_power.remains>5)|(!talent.rune_of_power.enabled&cooldown.arcane_power.remains>5)|cooldown.arcane_power.remains<=gcd))" );
  aoe->add_action( "radiant_spark,if=cooldown.touch_of_the_magi.remains>variable.rs_max_delay&cooldown.arcane_power.remains>variable.rs_max_delay&(talent.rune_of_power.enabled&cooldown.rune_of_power.remains<=gcd|talent.rune_of_power.enabled&cooldown.rune_of_power.remains>variable.rs_max_delay|!talent.rune_of_power.enabled)&buff.arcane_charge.stack<=variable.aoe_totm_max_charges&debuff.touch_of_the_magi.down" );
  aoe->add_action( "radiant_spark,if=cooldown.touch_of_the_magi.remains=0&(buff.arcane_charge.stack<=variable.aoe_totm_max_charges&((talent.rune_of_power.enabled&cooldown.rune_of_power.remains<=gcd&cooldown.arcane_power.remains>variable.totm_max_delay)|(!talent.rune_of_power.enabled&cooldown.arcane_power.remains>variable.totm_max_delay)|cooldown.arcane_power.remains<=gcd))" );
  aoe->add_action( "radiant_spark,if=cooldown.arcane_power.remains=0&(((cooldown.touch_of_the_magi.remains>variable.ap_max_delay&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack<=variable.aoe_totm_max_charges))&buff.rune_of_power.down)" );
  aoe->add_action( "deathborne,if=cooldown.arcane_power.remains=0&(((cooldown.touch_of_the_magi.remains>variable.ap_max_delay&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack<=variable.aoe_totm_max_charges))&buff.rune_of_power.down)" );
  aoe->add_action( "touch_of_the_magi,if=buff.arcane_charge.stack<=variable.aoe_totm_max_charges&((talent.rune_of_power.enabled&cooldown.rune_of_power.remains<=gcd&cooldown.arcane_power.remains>variable.totm_max_delay)|(!talent.rune_of_power.enabled&cooldown.arcane_power.remains>variable.totm_max_delay)|cooldown.arcane_power.remains<=gcd)" );
  aoe->add_action( "arcane_power,if=((cooldown.touch_of_the_magi.remains>variable.ap_max_delay&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack<=variable.aoe_totm_max_charges))&buff.rune_of_power.down" );
  aoe->add_action( "rune_of_power,if=buff.rune_of_power.down&((cooldown.touch_of_the_magi.remains>20&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack<=variable.aoe_totm_max_charges))&(cooldown.arcane_power.remains>15|debuff.touch_of_the_magi.up)" );
  aoe->add_action( "presence_of_mind,if=buff.deathborne.up&debuff.touch_of_the_magi.up&debuff.touch_of_the_magi.remains<=buff.presence_of_mind.max_stack*action.arcane_blast.execute_time" );
  aoe->add_action( "arcane_blast,if=buff.deathborne.up&((talent.resonance.enabled&active_enemies<4)|active_enemies<5)" );
  aoe->add_action( "supernova" );
  aoe->add_action( "arcane_orb,if=buff.arcane_charge.stack=0" );
  aoe->add_action( "nether_tempest,if=(refreshable|!ticking)&buff.arcane_charge.stack=buff.arcane_charge.max_stack" );
  aoe->add_action( "shifting_power,if=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down&cooldown.arcane_power.remains>0&cooldown.touch_of_the_magi.remains>0&(!talent.rune_of_power.enabled|(talent.rune_of_power.enabled&cooldown.rune_of_power.remains>0))" );
  aoe->add_action( "arcane_missiles,if=buff.clearcasting.react&runeforge.arcane_infinity.equipped&talent.amplification.enabled&active_enemies<9" );
  aoe->add_action( "arcane_missiles,if=buff.clearcasting.react&runeforge.arcane_infinity.equipped&active_enemies<6" );
  aoe->add_action( "arcane_explosion,if=buff.arcane_charge.stack<buff.arcane_charge.max_stack" );
  aoe->add_action( "arcane_explosion,if=buff.arcane_charge.stack=buff.arcane_charge.max_stack&prev_gcd.1.arcane_barrage" );
  aoe->add_action( "arcane_barrage,if=buff.arcane_charge.stack=buff.arcane_charge.max_stack" );
  aoe->add_action( "evocation,interrupt_if=mana.pct>=85,interrupt_immediate=1" );

  am_spam->add_action( "cancel_action,if=action.evocation.channeling&mana.pct>=95" );
  am_spam->add_action( "evocation,if=mana.pct<=variable.am_spam_evo_pct&(cooldown.touch_of_the_magi.remains<=action.evocation.execute_time|cooldown.arcane_power.remains<=action.evocation.execute_time|(talent.rune_of_power.enabled&cooldown.rune_of_power.remains<=action.evocation.execute_time))&buff.rune_of_power.down&buff.arcane_power.down&debuff.touch_of_the_magi.down" );
  am_spam->add_action( "rune_of_power,if=buff.rune_of_power.down&cooldown.arcane_power.remains>0" );
  am_spam->add_action( "touch_of_the_magi,if=(cooldown.arcane_power.remains=0&buff.rune_of_power.down)|prev_gcd.1.rune_of_power" );
  am_spam->add_action( "touch_of_the_magi,if=cooldown.arcane_power.remains<50&buff.rune_of_power.down&essence.vision_of_perfection.enabled" );
  am_spam->add_action( "arcane_power,if=buff.rune_of_power.down&cooldown.touch_of_the_magi.remains>variable.ap_max_delay" );
  am_spam->add_action( "arcane_barrage,if=buff.arcane_power.up&buff.arcane_power.remains<=action.arcane_missiles.execute_time&buff.arcane_charge.stack=buff.arcane_charge.max_stack" );
  am_spam->add_action( "arcane_orb,if=buff.arcane_charge.stack<buff.arcane_charge.max_stack&buff.rune_of_power.down&buff.arcane_power.down&debuff.touch_of_the_magi.down" );
  am_spam->add_action( "arcane_barrage,if=buff.rune_of_power.down&buff.arcane_power.down&debuff.touch_of_the_magi.down&buff.arcane_charge.stack=buff.arcane_charge.max_stack" );
  am_spam->add_action( "arcane_missiles,if=buff.clearcasting.react,chain=1,early_chain_if=buff.clearcasting_channel.down&(buff.arcane_power.up|buff.rune_of_power.up|cooldown.evocation.ready)" );
  am_spam->add_action( "arcane_missiles,if=!azerite.arcane_pummeling.enabled|buff.clearcasting_channel.down,chain=1,early_chain_if=buff.clearcasting_channel.down&(buff.arcane_power.up|buff.rune_of_power.up|cooldown.evocation.ready)" );
  am_spam->add_action( "evocation,if=buff.rune_of_power.down&buff.arcane_power.down&debuff.touch_of_the_magi.down" );
  am_spam->add_action( "arcane_orb,if=buff.arcane_charge.stack<buff.arcane_charge.max_stack" );
  am_spam->add_action( "arcane_barrage" );
  am_spam->add_action( "arcane_blast" );

  movement->add_action( "blink_any,if=movement.distance>=10" );
  movement->add_action( "presence_of_mind" );
  movement->add_action( "arcane_missiles,if=movement.distance<10" );
  movement->add_action( "arcane_orb" );
  movement->add_action( "fire_blast" );
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
  action_priority_list_t* rop_phase = p->get_action_priority_list( "rop_phase" );
  action_priority_list_t* standard_rotation = p->get_action_priority_list( "standard_rotation" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "variable,name=disable_combustion,op=reset", "If set to a non-zero value, the Combustion action and cooldowns that are constrained to only be used when Combustion is up will not be used during the simulation." );
  precombat->add_action( "variable,name=hot_streak_flamestrike,op=set,if=variable.hot_streak_flamestrike=0,value=2*talent.flame_patch.enabled+3*!talent.flame_patch.enabled", "This variable specifies the number of targets at which Hot Streak Flamestrikes outside of Combustion should be used." );
  precombat->add_action( "variable,name=hard_cast_flamestrike,op=set,if=variable.hard_cast_flamestrike=0,value=2*talent.flame_patch.enabled+3*!talent.flame_patch.enabled", "This variable specifies the number of targets at which Hard Cast Flamestrikes outside of Combustion should be used as filler." );
  precombat->add_action( "variable,name=combustion_flamestrike,op=set,if=variable.combustion_flamestrike=0,value=3*talent.flame_patch.enabled+6*!talent.flame_patch.enabled", "This variable specifies the number of targets at which Hot Streak Flamestrikes are used during Combustion." );
  precombat->add_action( "variable,name=arcane_explosion,op=set,if=variable.arcane_explosion=0,value=99*talent.flame_patch.enabled+2*!talent.flame_patch.enabled", "This variable specifies the number of targets at which Arcane Explosion outside of Combustion should be used." );
  precombat->add_action( "variable,name=arcane_explosion_mana,default=40,op=reset", "This variable specifies the percentage of mana below which Arcane Explosion will not be used." );
  precombat->add_action( "variable,name=kindling_reduction,default=0.2,op=reset", "With Kindling, Combustion's cooldown will be reduced by a random amount, but the number of crits starts very high after activating Combustion and slows down towards the end of Combustion's cooldown. When making decisions in the APL, Combustion's remaining cooldown is reduced by this fraction to account for Kindling." );
  precombat->add_action( "variable,name=shifting_power_reduction,op=set,value=-action.shifting_power.execute_time%action.shifting_power.new_tick_time*(dbc.effect.815503.base_value%1000+conduit.discipline_of_the_grove.time_value),if=covenant.night_fae.enabled", "The amount of cooldown reduction in seconds given by a full channel of Shifting Power. The dbc.effect.815503.base_value%1000 expression gives the number of seconds removed by each tick normally and conduit.discipline_of_the_grove.time_value gives the additional adjustment from that conduit." );
  precombat->add_action( "variable,name=skb_duration,op=set,value=dbc.effect.828420.base_value", "The duration of a Sun King's Blessing Combustion." );
  precombat->add_action( "variable,name=combustion_on_use,op=set,value=equipped.macabre_sheet_music|equipped.manifesto_of_madness|equipped.gladiators_badge|equipped.gladiators_medallion|equipped.ignition_mages_fuse|equipped.tzanes_barkspines|equipped.azurethos_singed_plumage|equipped.ancient_knot_of_wisdom|equipped.shockbiters_fang|equipped.neural_synapse_enhancer|equipped.balefire_branch" );
  precombat->add_action( "variable,name=font_double_on_use,op=set,value=equipped.azsharas_font_of_power&variable.combustion_on_use" );
  precombat->add_action( "variable,name=on_use_cutoff,op=set,value=20*variable.combustion_on_use+5*equipped.macabre_sheet_music" );
  precombat->add_action( "variable,name=font_of_power_precombat_channel,op=set,value=18,if=variable.font_double_on_use&!talent.firestarter.enabled&variable.font_of_power_precombat_channel=0", "This variable determines when Azshara's Font of Power is used before the pull if bfa.font_of_power_precombat_channel is not specified." );
  precombat->add_action( "variable,name=empyreal_ordnance_delay,default=18,op=reset", "How long before Combustion should Empyreal Ordnance be used?" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "use_item,name=azsharas_font_of_power,if=!variable.disable_combustion" );
  precombat->add_action( "mirror_image" );
  precombat->add_action( "pyroblast" );

  default_->add_action( "counterspell,if=!runeforge.disciplinary_command.equipped" );
  default_->add_action( "variable,name=time_to_combustion,op=set,value=talent.firestarter.enabled*firestarter.remains+(cooldown.combustion.remains*(1-variable.kindling_reduction*talent.kindling.enabled))*!cooldown.combustion.ready*buff.combustion.down" );
  default_->add_action( "variable,name=time_to_combustion,op=max,value=variable.empyreal_ordnance_delay-(cooldown.empyreal_ordnance.duration-cooldown.empyreal_ordnance.remains)*!cooldown.empyreal_ordnance.ready,if=equipped.empyreal_ordnance", "Make sure Combustion is delayed if needed based on the empyreal_ordnance_delay variable" );
  default_->add_action( "shifting_power,if=buff.combustion.down&buff.rune_of_power.down&cooldown.combustion.remains>0" );
  default_->add_action( "radiant_spark,if=(buff.combustion.down&buff.rune_of_power.down&(cooldown.combustion.remains<execute_time|cooldown.combustion.remains>cooldown.radiant_spark.duration))|(buff.rune_of_power.up&cooldown.combustion.remains>30)" );
  default_->add_action( "deathborne,if=buff.combustion.down&buff.rune_of_power.down&cooldown.combustion.remains<execute_time" );
  default_->add_action( "mirror_image,if=buff.combustion.down&debuff.radiant_spark_vulnerability.down" );
  default_->add_action( "use_item,effect_name=gladiators_badge,if=variable.time_to_combustion>cooldown-5" );
  default_->add_action( "use_item,name=empyreal_ordnance,if=variable.time_to_combustion<=variable.empyreal_ordnance_delay" );
  default_->add_action( "use_item,name=soul_igniter,if=variable.time_to_combustion>=variable.on_use_cutoff" );
  default_->add_action( "use_item,name=glyph_of_assimilation,if=variable.time_to_combustion>=variable.on_use_cutoff" );
  default_->add_action( "use_item,name=macabre_sheet_music,if=variable.time_to_combustion<=5" );
  default_->add_action( "use_item,name=dreadfire_vessel,if=variable.time_to_combustion>=variable.on_use_cutoff" );
  default_->add_action( "use_item,name=azsharas_font_of_power,if=variable.time_to_combustion<=5+15*variable.font_double_on_use&variable.time_to_combustion>0&!variable.disable_combustion" );
  default_->add_action( "guardian_of_azeroth,if=(variable.time_to_combustion<10|fight_remains<variable.time_to_combustion)&!variable.disable_combustion" );
  default_->add_action( "concentrated_flame" );
  default_->add_action( "reaping_flames" );
  default_->add_action( "focused_azerite_beam" );
  default_->add_action( "purifying_blast" );
  default_->add_action( "ripple_in_space" );
  default_->add_action( "the_unbound_force" );
  default_->add_action( "counterspell,if=runeforge.disciplinary_command.equipped&cooldown.buff_disciplinary_command.ready&buff.disciplinary_command_arcane.down&cooldown.combustion.remains>30&!buff.disciplinary_command.up", "Get the disciplinary_command buff up, unless combustion is soon." );
  default_->add_action( "arcane_explosion,if=runeforge.disciplinary_command.equipped&cooldown.buff_disciplinary_command.ready&buff.disciplinary_command_arcane.down&cooldown.combustion.remains>30&!buff.disciplinary_command.up" );
  default_->add_action( "frostbolt,if=runeforge.disciplinary_command.equipped&cooldown.buff_disciplinary_command.ready&buff.disciplinary_command_frost.down&cooldown.combustion.remains>30&!buff.disciplinary_command.up" );
  default_->add_action( "rune_of_power,if=buff.rune_of_power.down&(variable.time_to_combustion>buff.rune_of_power.duration&variable.time_to_combustion>action.fire_blast.full_recharge_time|variable.time_to_combustion>fight_remains|variable.disable_combustion)" );
  default_->add_action( "call_action_list,name=combustion_phase,if=!variable.disable_combustion&variable.time_to_combustion<=0" );
  default_->add_action( "variable,name=fire_blast_pooling,value=!variable.disable_combustion&variable.time_to_combustion<action.fire_blast.full_recharge_time-variable.shifting_power_reduction*(cooldown.shifting_power.remains<variable.time_to_combustion)&variable.time_to_combustion<fight_remains" );
  default_->add_action( "variable,name=phoenix_pooling,value=!variable.disable_combustion&variable.time_to_combustion<action.phoenix_flames.full_recharge_time-variable.shifting_power_reduction*(cooldown.shifting_power.remains<variable.time_to_combustion)&variable.time_to_combustion<fight_remains|runeforge.sun_kings_blessing.equipped" );
  default_->add_action( "call_action_list,name=rop_phase,if=buff.rune_of_power.up&(variable.time_to_combustion>0|variable.disable_combustion)" );
  default_->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=!variable.fire_blast_pooling&(variable.time_to_combustion>0|variable.disable_combustion)&active_enemies>=variable.hard_cast_flamestrike&!firestarter.active&!buff.hot_streak.react&(buff.heating_up.react&action.flamestrike.execute_remains<0.5|charges_fractional>=2)", "When Hardcasting Flame Strike, Fire Blasts should be used to generate Hot Streaks and to extend Blaster Master." );
  default_->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=firestarter.active&charges>=1&!variable.fire_blast_pooling&(!action.fireball.executing&!action.pyroblast.in_flight&buff.heating_up.react|action.fireball.executing&!buff.hot_streak.react|action.pyroblast.in_flight&buff.heating_up.react&!buff.hot_streak.react)", "During Firestarter, Fire Blasts are used similarly to during Combustion. Generally, they are used to generate Hot Streaks when crits will not be wasted and with Blaster Master, they should be spread out to maintain the Blaster Master buff." );
  default_->add_action( "call_action_list,name=standard_rotation,if=(variable.time_to_combustion>0|variable.disable_combustion)&buff.rune_of_power.down" );

  active_talents->add_action( "living_bomb,if=active_enemies>1&buff.combustion.down&(variable.time_to_combustion>cooldown.living_bomb.duration|variable.time_to_combustion<=0|variable.disable_combustion)" );
  active_talents->add_action( "meteor,if=!variable.disable_combustion&variable.time_to_combustion<=0|(cooldown.meteor.duration<variable.time_to_combustion&!talent.rune_of_power.enabled)|talent.rune_of_power.enabled&buff.rune_of_power.up&variable.time_to_combustion>action.meteor.cooldown|fight_remains<variable.time_to_combustion|variable.disable_combustion" );
  active_talents->add_action( "dragons_breath,if=talent.alexstraszas_fury.enabled&(buff.combustion.down&!buff.hot_streak.react)" );

  combustion_cooldowns->add_action( "potion" );
  combustion_cooldowns->add_action( "blood_fury" );
  combustion_cooldowns->add_action( "berserking" );
  combustion_cooldowns->add_action( "fireblood" );
  combustion_cooldowns->add_action( "ancestral_call" );
  combustion_cooldowns->add_action( "use_items" );
  combustion_cooldowns->add_action( "use_item,use_off_gcd=1,effect_name=gladiators_badge,if=action.meteor.in_flight_remains<=0.5" );
  combustion_cooldowns->add_action( "time_warp,if=runeforge.temporal_warp.equipped&buff.exhaustion.up" );

  combustion_phase->add_action( "lights_judgment,if=buff.combustion.down" );
  combustion_phase->add_action( "variable,name=extended_combustion_remains,op=set,value=buff.combustion.remains+buff.combustion.duration*(cooldown.combustion.remains<buff.combustion.remains)", "Estimate how long Combustion will last thanks to Sun King's Blessing to determine how Fire Blasts should be used." );
  combustion_phase->add_action( "variable,name=extended_combustion_remains,op=add,value=variable.skb_duration,if=buff.sun_kings_blessing_ready.up|variable.extended_combustion_remains>1.5*gcd.max*(buff.sun_kings_blessing.max_stack-buff.sun_kings_blessing.stack)", "Adds the duration of the Sun King's Blessing Combustion to the end of the current Combustion if the cast would complete during this Combustion." );
  combustion_phase->add_action( "bag_of_tricks,if=buff.combustion.down" );
  combustion_phase->add_action( "living_bomb,if=active_enemies>1&buff.combustion.down" );
  combustion_phase->add_action( "mirrors_of_torment,if=buff.combustion.down&buff.rune_of_power.down" );
  combustion_phase->add_action( "use_item,name=hyperthread_wristwraps,if=buff.combustion.up&action.fire_blast.charges=0&action.fire_blast.recharge_time>gcd.max" );
  combustion_phase->add_action( "blood_of_the_enemy" );
  combustion_phase->add_action( "memory_of_lucid_dreams" );
  combustion_phase->add_action( "worldvein_resonance" );
  combustion_phase->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=!azerite.blaster_master.enabled&(active_enemies<=active_dot.ignite|!cooldown.phoenix_flames.ready)&conduit.infernal_cascade.enabled&charges>=1&((action.fire_blast.charges_fractional+(variable.extended_combustion_remains-buff.infernal_cascade.duration)%cooldown.fire_blast.duration-variable.extended_combustion_remains%(buff.infernal_cascade.duration-0.5))>=0|variable.extended_combustion_remains<=buff.infernal_cascade.duration|buff.infernal_cascade.remains<0.5)&buff.combustion.up&!buff.firestorm.react&!buff.hot_streak.react&hot_streak_spells_in_flight+buff.heating_up.react<2", "BFA Fire Blast usage: During Combustion, Fire Blasts are used to generate Hot Streaks and minimize the amount of time spent executing other spells. For standard Fire, Fire Blasts are only used when Heating Up is active or when a Scorch cast is in progress and Heating Up and Hot Streak are not active. With Blaster Master and Flame On, Fire Blasts can additionally be used while Hot Streak and Heating Up are not active and a Pyroblast is in the air and also while casting Scorch even if Heating Up is already active. The latter allows two Hot Streak Pyroblasts to be cast in succession after the Scorch. Additionally with Blaster Master and Flame On, Fire Blasts should not be used unless Blaster Master is about to expire or there are more than enough Fire Blasts to extend Blaster Master to the end of Combustion." );
  combustion_phase->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=azerite.blaster_master.enabled&charges>=1&((action.fire_blast.charges_fractional+(buff.combustion.remains-buff.blaster_master.duration)%cooldown.fire_blast.duration-(buff.combustion.remains)%(buff.blaster_master.duration-0.5))>=0|!azerite.blaster_master.enabled|!talent.flame_on.enabled|buff.combustion.remains<=buff.blaster_master.duration|buff.blaster_master.remains<0.5|equipped.hyperthread_wristwraps&cooldown.hyperthread_wristwraps_300142.remains<5)&buff.combustion.up&(!action.scorch.executing&!action.pyroblast.in_flight&buff.heating_up.up|action.scorch.executing&buff.hot_streak.down&(buff.heating_up.down|azerite.blaster_master.enabled)|azerite.blaster_master.enabled&talent.flame_on.enabled&action.pyroblast.in_flight&buff.heating_up.down&buff.hot_streak.down)", "Without Infernal Cascade, just use Fire Blasts when they won't munch crits and when Firestorm is down." );
  combustion_phase->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=!azerite.blaster_master.enabled&(active_enemies<=active_dot.ignite|!cooldown.phoenix_flames.ready)&!conduit.infernal_cascade.enabled&charges>=1&buff.combustion.up&!buff.firestorm.react&!buff.hot_streak.react&hot_streak_spells_in_flight+buff.heating_up.react<2", "With Infernal Cascade, Fire Blast use should be additionaly constrained so that it is not be used unless Infernal Cascade is about to expire or there are more than enough Fire Blasts to extend Infernal Cascade to the end of Combustion." );
  combustion_phase->add_action( "counterspell,if=runeforge.disciplinary_command.equipped&buff.disciplinary_command.down&buff.disciplinary_command_arcane.down&cooldown.buff_disciplinary_command.ready" );
  combustion_phase->add_action( "arcane_explosion,if=runeforge.disciplinary_command.equipped&buff.disciplinary_command.down&buff.disciplinary_command_arcane.down&cooldown.buff_disciplinary_command.ready" );
  combustion_phase->add_action( "frostbolt,if=runeforge.disciplinary_command.equipped&buff.disciplinary_command.down&buff.disciplinary_command_frost.down" );
  combustion_phase->add_action( "call_action_list,name=active_talents" );
  combustion_phase->add_action( "combustion,use_off_gcd=1,use_while_casting=1,if=buff.combustion.down&(runeforge.disciplinary_command.equipped=buff.disciplinary_command.up)&(action.meteor.in_flight&action.meteor.in_flight_remains<=0.5|action.scorch.executing&action.scorch.execute_remains<0.5|action.fireball.executing&action.fireball.execute_remains<0.5|action.pyroblast.executing&action.pyroblast.execute_remains<0.5)" );
  combustion_phase->add_action( "call_action_list,name=combustion_cooldowns,if=buff.combustion.last_expire<=action.combustion.last_used", "Other cooldowns that should be used with Combustion should only be used with an actual Combustion cast and not with a Sun King's Blessing proc." );
  combustion_phase->add_action( "flamestrike,if=(buff.hot_streak.react|buff.firestorm.react)&active_enemies>=variable.combustion_flamestrike" );
  combustion_phase->add_action( "pyroblast,if=buff.sun_kings_blessing_ready.up&buff.sun_kings_blessing_ready.remains>cast_time" );
  combustion_phase->add_action( "pyroblast,if=buff.firestorm.react" );
  combustion_phase->add_action( "pyroblast,if=buff.pyroclasm.react&buff.pyroclasm.remains>cast_time&(buff.combustion.remains>cast_time|buff.combustion.down)&active_enemies<variable.combustion_flamestrike" );
  combustion_phase->add_action( "pyroblast,if=buff.hot_streak.react&buff.combustion.up" );
  combustion_phase->add_action( "pyroblast,if=prev_gcd.1.scorch&buff.heating_up.react&active_enemies<variable.combustion_flamestrike" );
  combustion_phase->add_action( "phoenix_flames,if=buff.combustion.up&((action.fire_blast.charges<1&talent.pyroclasm.enabled&active_enemies=1)|!talent.pyroclasm.enabled|active_enemies>1)" );
  combustion_phase->add_action( "fireball,if=buff.combustion.down&cooldown.combustion.remains<cast_time&!conduit.flame_accretion.enabled" );
  combustion_phase->add_action( "scorch,if=buff.combustion.remains>cast_time&buff.combustion.up|buff.combustion.down&cooldown.combustion.remains<cast_time" );
  combustion_phase->add_action( "living_bomb,if=buff.combustion.remains<gcd.max&active_enemies>1" );
  combustion_phase->add_action( "dragons_breath,if=buff.combustion.remains<gcd.max&buff.combustion.up" );
  combustion_phase->add_action( "scorch,if=target.health.pct<=30&talent.searing_touch.enabled" );

  rop_phase->add_action( "flamestrike,if=active_enemies>=variable.hot_streak_flamestrike&(buff.hot_streak.react|buff.firestorm.react)" );
  rop_phase->add_action( "pyroblast,if=buff.sun_kings_blessing_ready.up&buff.sun_kings_blessing_ready.remains>cast_time" );
  rop_phase->add_action( "pyroblast,if=buff.firestorm.react" );
  rop_phase->add_action( "pyroblast,if=buff.hot_streak.react" );
  rop_phase->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=buff.sun_kings_blessing_ready.down&active_enemies<variable.hard_cast_flamestrike&!firestarter.active&(!buff.heating_up.react&!buff.hot_streak.react&!prev_off_gcd.fire_blast&(action.fire_blast.charges>=2|(talent.alexstraszas_fury.enabled&cooldown.dragons_breath.ready)|(talent.searing_touch.enabled&target.health.pct<=30)))", "Use one Fire Blast early in RoP if you don't have either Heating Up or Hot Streak yet and either: (a) have more than two already, (b) have Alexstrasza's Fury ready to use, or (c) Searing Touch is active. Don't do this while hard casting Flametrikes or when Sun King's Blessing is ready." );
  rop_phase->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=!firestarter.active&(((action.fireball.executing&(action.fireball.execute_remains<0.5|!runeforge.firestorm.equipped)|action.pyroblast.executing&(action.pyroblast.execute_remains<0.5|!runeforge.firestorm.equipped))&buff.heating_up.react)|(talent.searing_touch.enabled&target.health.pct<=30&(buff.heating_up.react&!action.scorch.executing|!buff.hot_streak.react&!buff.heating_up.react&action.scorch.executing&!hot_streak_spells_in_flight)))", "Use Fire Blast either during a Fireball/Pyroblast cast when Heating Up is active or during execute with Searing Touch." );
  rop_phase->add_action( "call_action_list,name=active_talents" );
  rop_phase->add_action( "pyroblast,if=buff.pyroclasm.react&cast_time<buff.pyroclasm.remains&cast_time<buff.rune_of_power.remains" );
  rop_phase->add_action( "pyroblast,if=prev_gcd.1.scorch&buff.heating_up.react&talent.searing_touch.enabled&target.health.pct<=30&active_enemies<variable.hot_streak_flamestrike" );
  rop_phase->add_action( "phoenix_flames,if=!variable.phoenix_pooling&buff.heating_up.react&!buff.hot_streak.react&(active_dot.ignite<2|active_enemies>=variable.hard_cast_flamestrike|active_enemies>=variable.hot_streak_flamestrike)" );
  rop_phase->add_action( "scorch,if=target.health.pct<=30&talent.searing_touch.enabled" );
  rop_phase->add_action( "dragons_breath,if=active_enemies>2" );
  rop_phase->add_action( "arcane_explosion,if=active_enemies>=variable.arcane_explosion&mana.pct>=variable.arcane_explosion_mana" );
  rop_phase->add_action( "flamestrike,if=active_enemies>=variable.hard_cast_flamestrike" );
  rop_phase->add_action( "fireball" );

  standard_rotation->add_action( "flamestrike,if=active_enemies>=variable.hot_streak_flamestrike&(buff.hot_streak.react|buff.firestorm.react)" );
  standard_rotation->add_action( "pyroblast,if=buff.firestorm.react" );
  standard_rotation->add_action( "pyroblast,if=buff.hot_streak.react&buff.hot_streak.remains<action.fireball.execute_time" );
  standard_rotation->add_action( "pyroblast,if=buff.hot_streak.react&(prev_gcd.1.fireball|firestarter.active|action.pyroblast.in_flight)" );
  standard_rotation->add_action( "pyroblast,if=buff.sun_kings_blessing_ready.up&(cooldown.rune_of_power.remains+action.rune_of_power.execute_time+cast_time>buff.sun_kings_blessing_ready.remains|!talent.rune_of_power.enabled)&variable.time_to_combustion+cast_time>buff.sun_kings_blessing_ready.remains", "Try to get SKB procs inside RoP phases or Combustion phases when possible." );
  standard_rotation->add_action( "pyroblast,if=buff.hot_streak.react&target.health.pct<=30&talent.searing_touch.enabled" );
  standard_rotation->add_action( "pyroblast,if=buff.pyroclasm.react&cast_time<buff.pyroclasm.remains" );
  standard_rotation->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=!firestarter.active&!variable.fire_blast_pooling&(((action.fireball.executing&(action.fireball.execute_remains<0.5|!runeforge.firestorm.equipped)|action.pyroblast.executing&(action.pyroblast.execute_remains<0.5|!runeforge.firestorm.equipped))&buff.heating_up.react)|(talent.searing_touch.enabled&target.health.pct<=30&(buff.heating_up.react&!action.scorch.executing|!buff.hot_streak.react&!buff.heating_up.react&action.scorch.executing&!hot_streak_spells_in_flight)))", "During the standard rotation, only use Fire Blasts when they are not being pooled for RoP or Combustion. Use Fire Blast either during a Fireball/Pyroblast cast when Heating Up is active or during execute with Searing Touch." );
  standard_rotation->add_action( "pyroblast,if=prev_gcd.1.scorch&buff.heating_up.react&talent.searing_touch.enabled&target.health.pct<=30&active_enemies<variable.hot_streak_flamestrike" );
  standard_rotation->add_action( "phoenix_flames,if=!variable.phoenix_pooling&(!talent.from_the_ashes.enabled|active_enemies>1)&(active_dot.ignite<2|active_enemies>=variable.hard_cast_flamestrike|active_enemies>=variable.hot_streak_flamestrike)" );
  standard_rotation->add_action( "call_action_list,name=active_talents" );
  standard_rotation->add_action( "dragons_breath,if=active_enemies>1" );
  standard_rotation->add_action( "scorch,if=target.health.pct<=30&talent.searing_touch.enabled" );
  standard_rotation->add_action( "arcane_explosion,if=active_enemies>=variable.arcane_explosion&mana.pct>=variable.arcane_explosion_mana", "With enough targets, it is a gain to cast Flamestrike as filler instead of Fireball." );
  standard_rotation->add_action( "flamestrike,if=active_enemies>=variable.hard_cast_flamestrike" );
  standard_rotation->add_action( "fireball" );
  standard_rotation->add_action( "scorch" );
}
//fire_apl_end

//frost_apl_start
void frost( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* essences = p->get_action_priority_list( "essences" );
  action_priority_list_t* movement = p->get_action_priority_list( "movement" );
  action_priority_list_t* st = p->get_action_priority_list( "st" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "summon_water_elemental" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "frostbolt" );

  default_->add_action( "counterspell" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=essences" );
  default_->add_action( "call_action_list,name=aoe,if=active_enemies>=5" );
  default_->add_action( "call_action_list,name=st,if=active_enemies<5" );
  default_->add_action( "call_action_list,name=movement" );

  aoe->add_action( "frozen_orb" );
  aoe->add_action( "blizzard" );
  aoe->add_action( "flurry,if=(remaining_winters_chill=0|debuff.winters_chill.down)&(prev_gcd.1.ebonbolt|buff.brain_freeze.react&buff.fingers_of_frost.react=0)" );
  aoe->add_action( "ice_nova" );
  aoe->add_action( "comet_storm" );
  aoe->add_action( "ice_lance,if=buff.fingers_of_frost.react|debuff.frozen.remains>travel_time|remaining_winters_chill&debuff.winters_chill.remains>travel_time" );
  aoe->add_action( "radiant_spark" );
  aoe->add_action( "mirrors_of_torment" );
  aoe->add_action( "shifting_power" );
  aoe->add_action( "frost_nova,if=runeforge.grisly_icicle.equipped&target.level<=level&debuff.frozen.down" );
  aoe->add_action( "fire_blast,if=runeforge.disciplinary_command.equipped&cooldown.buff_disciplinary_command.ready&buff.disciplinary_command_fire.down" );
  aoe->add_action( "arcane_explosion,if=mana.pct>30" );
  aoe->add_action( "ebonbolt" );
  aoe->add_action( "ice_lance,if=runeforge.glacial_fragments.equipped&talent.splitting_ice.enabled" );
  aoe->add_action( "frostbolt" );

  cds->add_action( "potion,if=prev_off_gcd.icy_veins|fight_remains<30" );
  cds->add_action( "deathborne" );
  cds->add_action( "mirrors_of_torment,if=active_enemies<3&(conduit.siphoned_malice.enabled|soulbind.wasteland_propriety.enabled)" );
  cds->add_action( "rune_of_power,if=cooldown.icy_veins.remains>15&buff.rune_of_power.down" );
  cds->add_action( "icy_veins,if=buff.rune_of_power.down" );
  cds->add_action( "time_warp,if=runeforge.temporal_warp.equipped&buff.exhaustion.up&(prev_off_gcd.icy_veins|fight_remains<30)" );
  cds->add_action( "use_items" );
  cds->add_action( "blood_fury" );
  cds->add_action( "berserking" );
  cds->add_action( "lights_judgment" );
  cds->add_action( "fireblood" );
  cds->add_action( "ancestral_call" );
  cds->add_action( "bag_of_tricks" );

  essences->add_action( "guardian_of_azeroth" );
  essences->add_action( "focused_azerite_beam" );
  essences->add_action( "memory_of_lucid_dreams" );
  essences->add_action( "blood_of_the_enemy" );
  essences->add_action( "purifying_blast" );
  essences->add_action( "ripple_in_space" );
  essences->add_action( "concentrated_flame,line_cd=6" );
  essences->add_action( "reaping_flames" );
  essences->add_action( "the_unbound_force,if=buff.reckless_force.up" );
  essences->add_action( "worldvein_resonance" );

  movement->add_action( "blink_any,if=movement.distance>10" );
  movement->add_action( "ice_floes,if=buff.ice_floes.down" );
  movement->add_action( "arcane_explosion,if=mana.pct>30&active_enemies>=2" );
  movement->add_action( "fire_blast" );
  movement->add_action( "ice_lance" );

  st->add_action( "flurry,if=(remaining_winters_chill=0|debuff.winters_chill.down)&(prev_gcd.1.ebonbolt|buff.brain_freeze.react&(prev_gcd.1.glacial_spike|prev_gcd.1.frostbolt|prev_gcd.1.radiant_spark|buff.fingers_of_frost.react=0&(debuff.mirrors_of_torment.up|buff.freezing_winds.up|buff.expanded_potential.react)))" );
  st->add_action( "frozen_orb" );
  st->add_action( "blizzard,if=buff.freezing_rain.up|active_enemies>=2" );
  st->add_action( "ray_of_frost,if=remaining_winters_chill=1&debuff.winters_chill.remains" );
  st->add_action( "glacial_spike,if=remaining_winters_chill&debuff.winters_chill.remains>cast_time+travel_time" );
  st->add_action( "ice_lance,if=remaining_winters_chill&remaining_winters_chill>buff.fingers_of_frost.react&debuff.winters_chill.remains>travel_time" );
  st->add_action( "comet_storm" );
  st->add_action( "ice_nova" );
  st->add_action( "radiant_spark,if=buff.freezing_winds.up&active_enemies=1" );
  st->add_action( "ice_lance,if=buff.fingers_of_frost.react|debuff.frozen.remains>travel_time" );
  st->add_action( "ebonbolt" );
  st->add_action( "radiant_spark,if=(!runeforge.freezing_winds.equipped|active_enemies>=2)&buff.brain_freeze.react" );
  st->add_action( "mirrors_of_torment" );
  st->add_action( "shifting_power,if=buff.rune_of_power.down&(soulbind.grove_invigoration.enabled|soulbind.field_of_blossoms.enabled|active_enemies>=2)" );
  st->add_action( "frost_nova,if=runeforge.grisly_icicle.equipped&target.level<=level&debuff.frozen.down" );
  st->add_action( "arcane_explosion,if=runeforge.disciplinary_command.equipped&cooldown.buff_disciplinary_command.ready&buff.disciplinary_command_arcane.down" );
  st->add_action( "fire_blast,if=runeforge.disciplinary_command.equipped&cooldown.buff_disciplinary_command.ready&buff.disciplinary_command_fire.down" );
  st->add_action( "glacial_spike,if=buff.brain_freeze.react" );
  st->add_action( "frostbolt" );
}
//frost_apl_end

}  // namespace mage_apl
