#include "class_modules/apl/mage.hpp"

#include "player/action_priority_list.hpp"
#include "player/sc_player.hpp"

namespace mage_apl {

std::string potion( const player_t* p )
{
  return p->true_level >= 60 ? "spectral_intellect"
       : p->true_level >= 50 ? "superior_battle_potion_of_intellect"
       :                       "disabled";
}

std::string flask( const player_t* p )
{
  return p->true_level >= 60 ? "spectral_flask_of_power"
       : p->true_level >= 50 ? "greater_flask_of_endless_fathoms"
       :                       "disabled";
}

std::string food( const player_t* p )
{
  return p->true_level >= 60 ? "feast_of_gluttonous_hedonism"
       : p->true_level >= 50 ? "famine_evaluator_and_snack_table"
       :                       "disabled";
}

std::string rune( const player_t* p )
{
  return p->true_level >= 60 ? "veiled"
       : p->true_level >= 50 ? "battle_scarred"
       :                       "disabled";
}

std::string temporary_enchant( const player_t* p )
{
  return p->true_level >= 60 ? "main_hand:shadowcore_oil"
       :                       "disabled";
}

//arcane_apl_start
void arcane( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* calculations = p->get_action_priority_list( "calculations" );
  action_priority_list_t* opener = p->get_action_priority_list( "opener" );
  action_priority_list_t* fishing_opener = p->get_action_priority_list( "fishing_opener" );
  action_priority_list_t* cooldowns = p->get_action_priority_list( "cooldowns" );
  action_priority_list_t* rotation = p->get_action_priority_list( "rotation" );
  action_priority_list_t* final_burn = p->get_action_priority_list( "final_burn" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* harmony = p->get_action_priority_list( "harmony" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "arcane_familiar" );
  precombat->add_action( "conjure_mana_gem" );
  precombat->add_action( "variable,name=evo_pct,op=reset,default=15" );
  precombat->add_action( "variable,name=prepull_evo,default=-1,op=set,if=variable.prepull_evo=-1,value=1*(runeforge.siphon_storm&active_enemies>1+1*!covenant.necrolord)" );
  precombat->add_action( "variable,name=have_opened,op=set,value=0+1*(active_enemies>2|variable.prepull_evo=1)" );
  precombat->add_action( "variable,name=final_burn,op=set,value=0" );
  precombat->add_action( "variable,name=harmony_stack_time,op=reset,default=9" );
  precombat->add_action( "variable,name=rs_max_delay_for_totm,op=reset,default=5" );
  precombat->add_action( "variable,name=rs_max_delay_for_rop,op=reset,default=5" );
  precombat->add_action( "variable,name=rs_max_delay_for_ap,op=reset,default=20" );
  precombat->add_action( "variable,name=mot_preceed_totm_by,op=reset,default=8" );
  precombat->add_action( "variable,name=mot_max_delay_for_totm,op=reset,default=10" );
  precombat->add_action( "variable,name=mot_max_delay_for_ap,op=reset,default=15" );
  precombat->add_action( "variable,name=ap_max_delay_for_totm,default=-1,op=set,if=variable.ap_max_delay_for_totm=-1,value=10+(20*conduit.arcane_prodigy)" );
  precombat->add_action( "variable,name=ap_max_delay_for_mot,op=reset,default=20" );
  precombat->add_action( "variable,name=rop_max_delay_for_totm,default=-1,op=set,if=variable.rop_max_delay_for_totm=-1,value=20-(5*conduit.arcane_prodigy)" );
  precombat->add_action( "variable,name=totm_max_delay_for_ap,default=-1,op=set,if=variable.totm_max_delay_for_ap=-1,value=5+10*(covenant.night_fae|(conduit.arcane_prodigy&active_enemies<3))+15*(covenant.kyrian&runeforge.arcane_infinity&active_enemies>2)" );
  precombat->add_action( "variable,name=totm_max_delay_for_rop,default=-1,op=set,if=variable.totm_max_delay_for_rop=-1,value=20-(8*conduit.arcane_prodigy)" );
  precombat->add_action( "variable,name=barrage_mana_pct,default=-1,op=set,if=variable.barrage_mana_pct=-1,value=((80-(20*covenant.night_fae)+(15*covenant.kyrian))-(mastery_value*100))" );
  precombat->add_action( "variable,name=ap_minimum_mana_pct,op=reset,default=15" );
  precombat->add_action( "variable,name=totm_max_charges,op=reset,default=2" );
  precombat->add_action( "variable,name=aoe_totm_max_charges,op=reset,default=2" );
  precombat->add_action( "variable,name=fishing_opener,default=-1,op=set,if=variable.fishing_opener=-1,value=1*(equipped.empyreal_ordnance|(talent.rune_of_power&(talent.arcane_echo|!covenant.kyrian)&(!covenant.necrolord|active_enemies=1|runeforge.siphon_storm)))" );
  precombat->add_action( "variable,name=ap_on_use,op=set,value=equipped.macabre_sheet_music|equipped.gladiators_badge|equipped.gladiators_medallion|equipped.darkmoon_deck_putrescence|equipped.inscrutable_quantum_device|equipped.soulletting_ruby|equipped.sunblood_amethyst|equipped.wakeners_frond|equipped.flame_of_battle" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "mirror_image" );
  precombat->add_action( "frostbolt,if=!variable.prepull_evo=1&runeforge.disciplinary_command" );
  precombat->add_action( "arcane_blast,if=!variable.prepull_evo=1&!runeforge.disciplinary_command" );
  precombat->add_action( "evocation,if=variable.prepull_evo=1" );

  default_->add_action( "counterspell" );
  default_->add_action( "use_mana_gem,if=(talent.enlightened&mana.pct<=80&mana.pct>=65)|(!talent.enlightened&mana.pct<=85)" );
  default_->add_action( "potion,if=buff.arcane_power.up" );
  default_->add_action( "time_warp,if=runeforge.temporal_warp&buff.exhaustion.up&(cooldown.arcane_power.ready|fight_remains<=40)" );
  default_->add_action( "lights_judgment,if=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down" );
  default_->add_action( "bag_of_tricks,if=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down" );
  default_->add_action( "berserking,if=buff.arcane_power.up" );
  default_->add_action( "blood_fury,if=buff.arcane_power.up" );
  default_->add_action( "fireblood,if=buff.arcane_power.up" );
  default_->add_action( "ancestral_call,if=buff.arcane_power.up" );
  default_->add_action( "use_items,if=buff.arcane_power.up" );
  default_->add_action( "use_item,effect_name=gladiators_badge,if=buff.arcane_power.up|cooldown.arcane_power.remains>=55&debuff.touch_of_the_magi.up" );
  default_->add_action( "use_item,name=empyreal_ordnance,if=cooldown.arcane_power.remains<=(13+7*variable.ap_on_use)&cooldown.touch_of_the_magi.remains<=(13+7*variable.ap_on_use)" );
  default_->add_action( "use_item,name=dreadfire_vessel,if=cooldown.arcane_power.remains>=20|!variable.ap_on_use=1|(time=0&variable.fishing_opener=1&runeforge.siphon_storm)" );
  default_->add_action( "use_item,name=soul_igniter,if=cooldown.arcane_power.remains>=30|!variable.ap_on_use=1" );
  default_->add_action( "use_item,name=glyph_of_assimilation,if=cooldown.arcane_power.remains>=20|!variable.ap_on_use=1|(time=0&variable.fishing_opener=1&runeforge.siphon_storm)" );
  default_->add_action( "use_item,name=macabre_sheet_music,if=cooldown.arcane_power.remains<=5&(!variable.fishing_opener=1|time>30)" );
  default_->add_action( "use_item,name=macabre_sheet_music,if=cooldown.arcane_power.remains<=5&variable.fishing_opener=1&buff.rune_of_power.up&buff.rune_of_power.remains<=(10-5*runeforge.siphon_storm)&time<30" );
  default_->add_action( "call_action_list,name=calculations" );
  default_->add_action( "call_action_list,name=aoe,if=active_enemies>2" );
  default_->add_action( "call_action_list,name=harmony,if=covenant.kyrian&runeforge.arcane_infinity" );
  default_->add_action( "call_action_list,name=fishing_opener,if=variable.have_opened=0&variable.fishing_opener&!(covenant.kyrian&runeforge.arcane_infinity)" );
  default_->add_action( "call_action_list,name=opener,if=variable.have_opened=0&!(covenant.kyrian&runeforge.arcane_infinity)" );
  default_->add_action( "call_action_list,name=cooldowns,if=!(covenant.kyrian&runeforge.arcane_infinity)" );
  default_->add_action( "call_action_list,name=rotation,if=variable.final_burn=0&!(covenant.kyrian&runeforge.arcane_infinity)" );
  default_->add_action( "call_action_list,name=final_burn,if=variable.final_burn=1&!(covenant.kyrian&runeforge.arcane_infinity)" );

  calculations->add_action( "variable,name=have_opened,op=set,value=1,if=variable.have_opened=0&prev_gcd.1.evocation&!(runeforge.siphon_storm|runeforge.temporal_warp)" );
  calculations->add_action( "variable,name=have_opened,op=set,value=1,if=variable.have_opened=0&buff.arcane_power.down&cooldown.arcane_power.remains&(runeforge.siphon_storm|runeforge.temporal_warp)" );
  calculations->add_action( "variable,name=final_burn,op=set,value=1,if=buff.arcane_charge.stack=buff.arcane_charge.max_stack&!buff.rule_of_threes.up&fight_remains<=((mana%action.arcane_blast.cost)*action.arcane_blast.execute_time)" );
  calculations->add_action( "variable,name=empowered_barrage,op=set,value=buff.arcane_harmony.stack=buff.arcane_harmony.max_stack|(runeforge.arcane_bombardment&target.health.pct<35)", "Either a fully stacked harmony or in execute range with Bombardment" );
  calculations->add_action( "variable,name=last_ap_use,default=0,op=set,if=buff.arcane_power.up&(variable.last_ap_use=0|time>=variable.last_ap_use+15),value=time" );
  calculations->add_action( "variable,name=estimated_ap_cooldown,op=set,value=(cooldown.arcane_power.duration*(1-(0.03*conduit.arcane_prodigy.rank)))-(time-variable.last_ap_use)", "Arcane Prodigy gives a variable amount of cdr, but we'll use a flat estimation here. The simc provided remains_expected expression does not work well for prodigy due to the bursty nature of the cdr." );
  calculations->add_action( "variable,name=time_until_ap,op=set,if=conduit.arcane_prodigy,value=variable.estimated_ap_cooldown" );
  calculations->add_action( "variable,name=time_until_ap,op=set,if=!conduit.arcane_prodigy,value=cooldown.arcane_power.remains" );
  calculations->add_action( "variable,name=time_until_ap,op=max,value=cooldown.touch_of_the_magi.remains,if=(cooldown.touch_of_the_magi.remains-variable.time_until_ap)<20", "We'll delay AP up to 20sec for TotM" );
  calculations->add_action( "variable,name=holding_totm,op=set,value=cooldown.touch_of_the_magi.ready&variable.time_until_ap<20", "We'll delay TotM up to 20sec for AP" );
  calculations->add_action( "variable,name=just_used_spark,op=set,value=(prev_gcd.1.radiant_spark|prev_gcd.2.radiant_spark|prev_gcd.3.radiant_spark)&debuff.radiant_spark_vulnerability.down", "Radiant Spark does not immediately put up the vulnerability debuff so it can be difficult to discern that we're at the zeroth vulnerability stack" );
  calculations->add_action( "variable,name=outside_of_cooldowns,op=set,value=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down&!variable.just_used_spark&debuff.radiant_spark_vulnerability.down" );
  calculations->add_action( "variable,name=stack_harmony,op=set,value=runeforge.arcane_infinity&((covenant.kyrian&cooldown.radiant_spark.remains<variable.harmony_stack_time))" );

  opener->add_action( "fire_blast,if=runeforge.disciplinary_command&buff.disciplinary_command_frost.up" );
  opener->add_action( "frost_nova,if=runeforge.grisly_icicle&mana.pct>95" );
  opener->add_action( "deathborne" );
  opener->add_action( "radiant_spark,if=mana.pct>40" );
  opener->add_action( "shifting_power,if=buff.arcane_power.down&cooldown.arcane_power.remains" );
  opener->add_action( "mirrors_of_torment" );
  opener->add_action( "touch_of_the_magi" );
  opener->add_action( "arcane_power" );
  opener->add_action( "rune_of_power,if=buff.rune_of_power.down" );
  opener->add_action( "presence_of_mind,if=!talent.arcane_echo&debuff.touch_of_the_magi.up&debuff.touch_of_the_magi.remains<=(action.arcane_blast.execute_time*buff.presence_of_mind.max_stack)", "Use PoM to squeeze an extra Arcane Blast during the TotM/AP window unless running Arcane Echo. If Arcane Echo is talented, we can't use PoM within the TotM/AP window unless we interrupt the AM channel, so we'll save it." );
  opener->add_action( "presence_of_mind,if=buff.arcane_power.up&buff.rune_of_power.remains<=(action.arcane_blast.execute_time*buff.presence_of_mind.max_stack)", "If PoM hasn't been used during the TotM window, we'll use it to squeeze an extra Arcane Blast in the RoP/AP window." );
  opener->add_action( "arcane_blast,if=dot.radiant_spark.remains>5|debuff.radiant_spark_vulnerability.stack>0" );
  opener->add_action( "arcane_barrage,if=buff.arcane_power.up&buff.arcane_power.remains<gcd&runeforge.arcane_infinity", "Always Barrage at the end of AP when running the Arcane Harmony legendary" );
  opener->add_action( "arcane_barrage,if=buff.rune_of_power.up&buff.arcane_power.down&buff.rune_of_power.remains<=gcd&runeforge.arcane_infinity", "Always Barrage at the end of the RoP window when running the Arcane Harmony legendary" );
  opener->add_action( "arcane_missiles,if=debuff.touch_of_the_magi.up&talent.arcane_echo&(buff.deathborne.down|active_enemies=1)&debuff.touch_of_the_magi.remains>action.arcane_missiles.execute_time,chain=1,early_chain_if=buff.clearcasting_channel.down&(buff.arcane_power.up|(!talent.overpowered&(buff.rune_of_power.up|cooldown.evocation.ready)))" );
  opener->add_action( "arcane_missiles,if=buff.clearcasting.react,chain=1" );
  opener->add_action( "arcane_orb,if=buff.arcane_charge.stack<=variable.totm_max_charges" );
  opener->add_action( "arcane_blast,if=buff.rune_of_power.up|mana.pct>15" );
  opener->add_action( "evocation,if=buff.rune_of_power.down&buff.arcane_power.down,interrupt_if=mana.pct>=85,interrupt_immediate=1" );
  opener->add_action( "arcane_barrage" );

  fishing_opener->add_action( "evocation,if=(runeforge.siphon_storm|runeforge.temporal_warp)&(buff.rune_of_power.down|prev_gcd.1.arcane_barrage)&cooldown.rune_of_power.remains", "When running either the SS or TW legendaries, we want to Evo before the TotM/AP window. Use it as soon as RoP has finished our we've spent our charges" );
  fishing_opener->add_action( "evocation,if=talent.rune_of_power&cooldown.rune_of_power.remains&cooldown.arcane_power.remains&buff.arcane_power.down&buff.rune_of_power.down&prev_gcd.1.arcane_barrage", "If we've finished our cooldown windows and spent our charges, prioritize Evo over everything else" );
  fishing_opener->add_action( "fire_blast,if=runeforge.disciplinary_command&buff.disciplinary_command_frost.up" );
  fishing_opener->add_action( "frost_nova,if=runeforge.grisly_icicle&mana.pct>95" );
  fishing_opener->add_action( "arcane_missiles,if=runeforge.arcane_infinity&buff.arcane_harmony.stack<buff.arcane_harmony.max_stack&((buff.arcane_power.down&cooldown.arcane_power.ready)|debuff.touch_of_the_magi.up),chain=1" );
  fishing_opener->add_action( "deathborne,if=!runeforge.siphon_storm&!runeforge.temporal_warp" );
  fishing_opener->add_action( "arcane_orb,if=cooldown.rune_of_power.ready" );
  fishing_opener->add_action( "arcane_blast,if=cooldown.rune_of_power.ready&buff.arcane_charge.stack<buff.arcane_charge.max_stack" );
  fishing_opener->add_action( "rune_of_power" );
  fishing_opener->add_action( "arcane_missiles,if=buff.clearcasting.react&buff.clearcasting.stack=buff.clearcasting.max_stack&covenant.venthyr&cooldown.mirrors_of_torment.ready&!variable.empowered_barrage&cooldown.arcane_power.ready", "If we're at max stacks of CC and we haven't used MoT yet, use a stack to prevent munching unless running the Harmony legendary" );
  fishing_opener->add_action( "potion,if=!(runeforge.siphon_storm|runeforge.temporal_warp)", "Normally we pair potion use with AP, but it will last long enough for both the RoP and AP windows unless running the SS or TW legendaries" );
  fishing_opener->add_action( "deathborne,if=buff.rune_of_power.down|prev_gcd.1.arcane_barrage" );
  fishing_opener->add_action( "radiant_spark,if=buff.rune_of_power.down|prev_gcd.1.arcane_barrage" );
  fishing_opener->add_action( "mirrors_of_torment,if=buff.rune_of_power.remains<6" );
  fishing_opener->add_action( "arcane_power,if=variable.empowered_barrage&buff.rune_of_power.up&(mana.pct<(25+(10*covenant.kyrian))|buff.clearcasting.stack=buff.clearcasting.max_stack)", "When running the Harmony legendary, use AP before TotM to dump a fully stacked bararge" );
  fishing_opener->add_action( "arcane_barrage,if=variable.empowered_barrage&buff.arcane_charge.stack=buff.arcane_charge.max_stack&buff.arcane_power.up" );
  fishing_opener->add_action( "touch_of_the_magi,if=buff.rune_of_power.down|prev_gcd.1.arcane_barrage|prev_gcd.1.radiant_spark|prev_gcd.1.deathborne" );
  fishing_opener->add_action( "arcane_power,if=prev_gcd.1.touch_of_the_magi" );
  fishing_opener->add_action( "presence_of_mind,if=!talent.arcane_echo&debuff.touch_of_the_magi.up&debuff.touch_of_the_magi.remains<=(action.arcane_blast.execute_time*buff.presence_of_mind.max_stack)", "Use PoM to squeeze an extra Arcane Blast during the TotM/AP window unless running Arcane Echo. If Arcane Echo is talented, we can't use PoM within the TotM/AP window unless we interrupt the AM channel, so we'll save it." );
  fishing_opener->add_action( "presence_of_mind,if=buff.arcane_power.up&buff.rune_of_power.remains<=(action.arcane_blast.execute_time*buff.presence_of_mind.max_stack)", "If PoM hasn't been used during the TotM window, we'll use it to squeeze an extra Arcane Blast in the RoP/AP window." );
  fishing_opener->add_action( "arcane_blast,if=dot.radiant_spark.remains>5|debuff.radiant_spark_vulnerability.stack>0" );
  fishing_opener->add_action( "arcane_barrage,if=cooldown.arcane_power.ready&mana.pct<(40+(10*covenant.kyrian))&buff.arcane_charge.stack=buff.arcane_charge.max_stack&!runeforge.siphon_storm&!runeforge.temporal_warp&!runeforge.arcane_infinity", "We want to make sure we have enough mana for the entire AP window, even if there's still time remaining on RoP. It's only better to burn to zero and Evo if running the SS or TW legendaries." );
  fishing_opener->add_action( "arcane_barrage,if=buff.arcane_power.up&buff.arcane_power.remains<=gcd&cooldown.evocation.remains", "Barrage should not be used at the end of AP if Evocation is ready as dropping charges will make it take longer to burn through remaining mana" );
  fishing_opener->add_action( "arcane_barrage,if=buff.rune_of_power.up&buff.arcane_power.down&buff.rune_of_power.remains<=gcd&!runeforge.arcane_infinity", "Barrage should always be the last cast in the RoP window as TotM will restore the charges spent" );
  fishing_opener->add_action( "arcane_missiles,if=debuff.touch_of_the_magi.up&talent.arcane_echo&(buff.deathborne.down|active_enemies=1)&debuff.touch_of_the_magi.remains>action.arcane_missiles.execute_time,chain=1,early_chain_if=buff.clearcasting_channel.down&(buff.arcane_power.up|(!talent.overpowered&(buff.rune_of_power.up|cooldown.evocation.ready)))" );
  fishing_opener->add_action( "arcane_missiles,if=covenant.venthyr&buff.clearcasting.stack=buff.clearcasting.max_stack", "If we're venthyr, immediately use CC when capped to not waste procs from MoT" );
  fishing_opener->add_action( "arcane_missiles,if=buff.clearcasting.react&cooldown.arcane_power.remains&(buff.rune_of_power.up|buff.arcane_power.up),chain=1", "Start using CC AM when AP has been used" );
  fishing_opener->add_action( "arcane_orb,if=buff.arcane_charge.stack<=variable.totm_max_charges" );
  fishing_opener->add_action( "arcane_blast,if=buff.rune_of_power.up|mana.pct>15" );
  fishing_opener->add_action( "evocation,if=buff.rune_of_power.down&buff.arcane_power.down,interrupt_if=mana.pct>=85,interrupt_immediate=1" );
  fishing_opener->add_action( "arcane_barrage" );

  cooldowns->add_action( "frost_nova,if=runeforge.grisly_icicle&cooldown.arcane_power.remains>30&cooldown.touch_of_the_magi.ready&(buff.arcane_charge.stack<=variable.totm_max_charges&((talent.rune_of_power&cooldown.rune_of_power.remains<=gcd&cooldown.arcane_power.remains>variable.totm_max_delay_for_ap)|(!talent.rune_of_power&cooldown.arcane_power.remains>variable.totm_max_delay_for_ap)|cooldown.arcane_power.remains<=gcd))", "Prioritize using grisly icicle with ap. Use it with totm otherwise." );
  cooldowns->add_action( "frost_nova,if=runeforge.grisly_icicle&cooldown.arcane_power.ready&(!talent.enlightened|(talent.enlightened&mana.pct>=70))&((cooldown.touch_of_the_magi.remains>10&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack=0))&buff.rune_of_power.down&mana.pct>=variable.ap_minimum_mana_pct" );
  cooldowns->add_action( "frostbolt,if=runeforge.disciplinary_command&cooldown.buff_disciplinary_command.ready&buff.disciplinary_command_frost.down&(buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down)&cooldown.touch_of_the_magi.remains=0&(buff.arcane_charge.stack<=variable.totm_max_charges&((talent.rune_of_power&cooldown.rune_of_power.remains<=gcd&cooldown.arcane_power.remains>variable.totm_max_delay_for_ap)|(!talent.rune_of_power&cooldown.arcane_power.remains>variable.totm_max_delay_for_ap)|cooldown.arcane_power.remains<=gcd))" );
  cooldowns->add_action( "fire_blast,if=runeforge.disciplinary_command&cooldown.buff_disciplinary_command.ready&buff.disciplinary_command_fire.down&prev_gcd.1.frostbolt" );
  cooldowns->add_action( "mirrors_of_torment,if=cooldown.touch_of_the_magi.remains<variable.mot_preceed_totm_by|(variable.time_until_ap>variable.mot_max_delay_for_ap&cooldown.touch_of_the_magi.remains>variable.mot_max_delay_for_totm)", "Pair MoT with AP if it's coming up soon. If it will be used with TotM, cast it slightly before" );
  cooldowns->add_action( "mirrors_of_torment,if=cooldown.arcane_power.ready&(!talent.enlightened|(talent.enlightened&mana.pct>=70))&((cooldown.touch_of_the_magi.remains>variable.ap_max_delay_for_totm&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack=0))&buff.rune_of_power.down&mana.pct>=variable.ap_minimum_mana_pct" );
  cooldowns->add_action( "deathborne,if=cooldown.touch_of_the_magi.ready&buff.arcane_charge.stack<=variable.totm_max_charges&cooldown.arcane_power.remains<=gcd", "Always use deathborne with ap. If totm is ready as well, make sure to cast it before totm." );
  cooldowns->add_action( "deathborne,if=cooldown.arcane_power.ready&(!talent.enlightened|(talent.enlightened&mana.pct>=70))&((cooldown.touch_of_the_magi.remains>10&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.ready&buff.arcane_charge.stack=0))&buff.rune_of_power.down&mana.pct>=variable.ap_minimum_mana_pct" );
  cooldowns->add_action( "radiant_spark,if=cooldown.touch_of_the_magi.remains>variable.rs_max_delay_for_totm&cooldown.arcane_power.remains>variable.rs_max_delay_for_ap&(talent.rune_of_power&(cooldown.rune_of_power.remains<execute_time|cooldown.rune_of_power.remains>variable.rs_max_delay_for_rop)|!talent.rune_of_power)&buff.arcane_charge.stack>2&debuff.touch_of_the_magi.down&buff.rune_of_power.down&buff.arcane_power.down", "Use spark if totm and ap are on cd and won't be up for longer than the max delay, making sure we have at least two arcane charges and that totm wasn't just used." );
  cooldowns->add_action( "radiant_spark,if=cooldown.touch_of_the_magi.remains<execute_time&buff.arcane_charge.stack<=variable.totm_max_charges&cooldown.arcane_power.remains<(execute_time+action.touch_of_the_magi.execute_time)", "Use spark with ap when possible. If totm is ready as well, make sure to cast it before totm." );
  cooldowns->add_action( "radiant_spark,if=cooldown.arcane_power.remains<execute_time&((!talent.enlightened|(talent.enlightened&mana.pct>=70))&((cooldown.touch_of_the_magi.remains>variable.ap_max_delay_for_totm&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack=0))&buff.rune_of_power.down&mana.pct>=variable.ap_minimum_mana_pct)" );
  cooldowns->add_action( "touch_of_the_magi,if=buff.arcane_charge.stack<=variable.totm_max_charges&cooldown.arcane_power.remains<=execute_time&mana.pct>variable.ap_minimum_mana_pct&buff.rune_of_power.down", "Use totm with ap if it's within the max delay. If not, use with rop if the talent is taken, and it's within the max delay." );
  cooldowns->add_action( "touch_of_the_magi,if=buff.arcane_charge.stack<=variable.totm_max_charges&talent.rune_of_power&cooldown.rune_of_power.remains<=execute_time&variable.time_until_ap>variable.totm_max_delay_for_ap" );
  cooldowns->add_action( "touch_of_the_magi,if=buff.arcane_charge.stack<=variable.totm_max_charges&(!talent.rune_of_power|cooldown.rune_of_power.remains>variable.totm_max_delay_for_rop)&variable.time_until_ap>variable.totm_max_delay_for_ap" );
  cooldowns->add_action( "arcane_power,if=cooldown.touch_of_the_magi.remains>variable.ap_max_delay_for_totm&(!covenant.venthyr|cooldown.mirrors_of_torment.remains>variable.ap_max_delay_for_mot)&buff.arcane_charge.stack=buff.arcane_charge.max_stack&buff.rune_of_power.down&mana.pct>=variable.ap_minimum_mana_pct", "Use ap if totm is on cd and won't be up for longer than the max delay, making sure that we have enough mana and that there is not already a rune of power down." );
  cooldowns->add_action( "rune_of_power,if=buff.arcane_power.down&(cooldown.touch_of_the_magi.remains>variable.rop_max_delay_for_totm|cooldown.arcane_power.remains<=variable.totm_max_delay_for_ap)&buff.arcane_charge.stack=buff.arcane_charge.max_stack&cooldown.arcane_power.remains>12", "Use rop if totm is on cd and won't be up for longer than the max delay, making sure there isn't already a rune down and that ap won't become available during rune." );
  cooldowns->add_action( "shifting_power,if=variable.outside_of_cooldowns" );
  cooldowns->add_action( "presence_of_mind,if=talent.rune_of_power&buff.arcane_power.up&buff.rune_of_power.remains<gcd.max", "Use pom to squeeze an extra ab in the next cooldown window, unless kyrian then only during arcane power due to how mana hungry radiant spark is" );
  cooldowns->add_action( "presence_of_mind,if=debuff.touch_of_the_magi.up&debuff.touch_of_the_magi.remains<action.arcane_missiles.execute_time&!covenant.kyrian" );
  cooldowns->add_action( "presence_of_mind,if=buff.rune_of_power.up&buff.rune_of_power.remains<gcd.max&cooldown.evocation.ready&cooldown.touch_of_the_magi.remains&!covenant.kyrian" );

  rotation->add_action( "cancel_action,if=action.evocation.channeling&mana.pct>=95&(!runeforge.siphon_storm|buff.siphon_storm.stack=buff.siphon_storm.max_stack)" );
  rotation->add_action( "evocation,if=mana.pct<=variable.evo_pct&(cooldown.touch_of_the_magi.remains<=action.evocation.execute_time|cooldown.arcane_power.remains<=action.evocation.execute_time|(talent.rune_of_power&cooldown.rune_of_power.remains<=action.evocation.execute_time))&buff.rune_of_power.down&buff.arcane_power.down&debuff.touch_of_the_magi.down&!prev_gcd.1.touch_of_the_magi", "Use evo if we're low on mana and cooldowns are about to be up" );
  rotation->add_action( "evocation,if=runeforge.siphon_storm&cooldown.arcane_power.remains<=action.evocation.execute_time" );
  rotation->add_action( "arcane_barrage,if=cooldown.touch_of_the_magi.ready&(buff.arcane_charge.stack>variable.totm_max_charges&cooldown.arcane_power.remains<=execute_time&mana.pct>variable.ap_minimum_mana_pct&buff.rune_of_power.down)", "Barrage if it's time to use totm and we have too many charges" );
  rotation->add_action( "arcane_barrage,if=cooldown.touch_of_the_magi.ready&(buff.arcane_charge.stack>variable.totm_max_charges&talent.rune_of_power&cooldown.rune_of_power.remains<=execute_time&variable.time_until_ap>variable.totm_max_delay_for_ap)" );
  rotation->add_action( "arcane_barrage,if=cooldown.touch_of_the_magi.ready&(buff.arcane_charge.stack>variable.totm_max_charges&(!talent.rune_of_power|cooldown.rune_of_power.remains>variable.totm_max_delay_for_rop)&variable.time_until_ap>variable.totm_max_delay_for_ap)" );
  rotation->add_action( "arcane_barrage,if=debuff.radiant_spark_vulnerability.stack=debuff.radiant_spark_vulnerability.max_stack&(buff.arcane_power.down|buff.arcane_power.remains<=gcd)&(buff.rune_of_power.down|buff.rune_of_power.remains<=gcd)" );
  rotation->add_action( "arcane_blast,if=variable.just_used_spark|(debuff.radiant_spark_vulnerability.up&debuff.radiant_spark_vulnerability.stack<debuff.radiant_spark_vulnerability.max_stack)" );
  rotation->add_action( "arcane_blast,if=buff.presence_of_mind.up&debuff.touch_of_the_magi.up&debuff.touch_of_the_magi.remains<=action.arcane_blast.execute_time" );
  rotation->add_action( "arcane_missiles,if=debuff.touch_of_the_magi.up&talent.arcane_echo&(buff.deathborne.down|active_enemies=1)&(debuff.touch_of_the_magi.remains>action.arcane_missiles.execute_time|cooldown.presence_of_mind.remains|covenant.kyrian),chain=1,early_chain_if=buff.clearcasting_channel.down&(buff.arcane_power.up|(!talent.overpowered&(buff.rune_of_power.up|cooldown.evocation.ready)))" );
  rotation->add_action( "arcane_missiles,if=buff.clearcasting.react&buff.expanded_potential.up", "Consume the expanded potential buff asap to not lose potential procs" );
  rotation->add_action( "arcane_missiles,if=buff.clearcasting.react&(buff.arcane_power.up|buff.rune_of_power.up|debuff.touch_of_the_magi.remains>action.arcane_missiles.execute_time),chain=1" );
  rotation->add_action( "arcane_missiles,if=buff.clearcasting.react&buff.clearcasting.stack=buff.clearcasting.max_stack,chain=1" );
  rotation->add_action( "arcane_missiles,if=buff.clearcasting.react&buff.clearcasting.remains<=((buff.clearcasting.stack*action.arcane_missiles.execute_time)+gcd.max),chain=1", "Always try to bank CC for cooldown windows but dump them if they're about to expire" );
  rotation->add_action( "nether_tempest,if=(refreshable|!ticking)&buff.arcane_charge.stack=buff.arcane_charge.max_stack&buff.arcane_power.down&debuff.touch_of_the_magi.down" );
  rotation->add_action( "arcane_orb,if=buff.arcane_charge.stack<=variable.totm_max_charges" );
  rotation->add_action( "supernova,if=variable.outside_of_cooldowns&mana.pct<=95" );
  rotation->add_action( "arcane_blast,if=buff.rule_of_threes.up&buff.arcane_charge.stack>3" );
  rotation->add_action( "arcane_barrage,if=variable.outside_of_cooldowns&mana.pct<=variable.barrage_mana_pct&buff.arcane_charge.stack=buff.arcane_charge.max_stack&cooldown.evocation.remains", "Dump charges if we fall below the conserve mana threshold and evocation is still on cooldown" );
  rotation->add_action( "arcane_barrage,if=variable.outside_of_cooldowns&buff.arcane_charge.stack=buff.arcane_charge.max_stack&talent.arcane_orb&cooldown.arcane_orb.remains<=gcd&mana.pct<=90&cooldown.evocation.remains", "Dump charges if orb is ready" );
  rotation->add_action( "arcane_barrage,if=buff.arcane_power.up&buff.arcane_power.remains<=gcd&buff.arcane_charge.stack=buff.arcane_charge.max_stack&(cooldown.evocation.remains|runeforge.arcane_infinity)", "Arcane Barrage should generally be the last cast of the AP window" );
  rotation->add_action( "arcane_barrage,if=buff.rune_of_power.up&buff.arcane_power.down&buff.rune_of_power.remains<=gcd&buff.arcane_charge.stack=buff.arcane_charge.max_stack&(cooldown.evocation.remains|runeforge.arcane_infinity)", "Arcane Barrage should generally be the last cast of the RoP window" );
  rotation->add_action( "arcane_barrage,if=buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.up&debuff.touch_of_the_magi.remains<=gcd&buff.arcane_charge.stack=buff.arcane_charge.max_stack", "Arcane Barrage should be the last cast of the TotM window if it is not used with RoP or AP" );
  rotation->add_action( "arcane_barrage,if=variable.empowered_barrage&buff.arcane_charge.stack>=(active_enemies-1)&active_enemies>1&buff.deathborne.down", "When the target is in execute range and there is more than one enemy, use barrage as soon as you have enough charges to hit all enemies if the Arcane Bombardment legendary is equipped" );
  rotation->add_action( "arcane_explosion,if=variable.empowered_barrage&buff.arcane_charge.stack<buff.arcane_charge.max_stack&active_enemies>1&buff.deathborne.down" );
  rotation->add_action( "arcane_blast" );
  rotation->add_action( "evocation,if=variable.outside_of_cooldowns" );
  rotation->add_action( "arcane_barrage" );

  final_burn->add_action( "arcane_missiles,if=buff.clearcasting.react,chain=1" );
  final_burn->add_action( "arcane_blast" );
  final_burn->add_action( "arcane_barrage" );

  aoe->add_action( "frostbolt,if=runeforge.disciplinary_command&cooldown.buff_disciplinary_command.ready&buff.disciplinary_command_frost.down&(buff.arcane_power.down&buff.rune_of_power.down&debuff.touch_of_the_magi.down)&cooldown.touch_of_the_magi.remains=0&(buff.arcane_charge.stack<=variable.aoe_totm_max_charges&((talent.rune_of_power&cooldown.rune_of_power.remains<=gcd&cooldown.arcane_power.remains>variable.totm_max_delay_for_ap)|(!talent.rune_of_power&cooldown.arcane_power.remains>variable.totm_max_delay_for_ap)|cooldown.arcane_power.remains<=gcd))" );
  aoe->add_action( "fire_blast,if=(runeforge.disciplinary_command&cooldown.buff_disciplinary_command.ready&buff.disciplinary_command_fire.down&prev_gcd.1.frostbolt)|(runeforge.disciplinary_command&time=0)" );
  aoe->add_action( "frost_nova,if=runeforge.grisly_icicle&cooldown.arcane_power.remains>30&cooldown.touch_of_the_magi.remains=0&(buff.arcane_charge.stack<=variable.aoe_totm_max_charges&((talent.rune_of_power&cooldown.rune_of_power.remains<=gcd&cooldown.arcane_power.remains>variable.totm_max_delay_for_ap)|(!talent.rune_of_power&cooldown.arcane_power.remains>variable.totm_max_delay_for_ap)|cooldown.arcane_power.remains<=gcd))" );
  aoe->add_action( "frost_nova,if=runeforge.grisly_icicle&cooldown.arcane_power.remains=0&(((cooldown.touch_of_the_magi.remains>variable.ap_max_delay_for_totm&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack<=variable.aoe_totm_max_charges))&buff.rune_of_power.down)" );
  aoe->add_action( "arcane_missiles,if=covenant.kyrian&runeforge.arcane_infinity&buff.arcane_harmony.stack<15&cooldown.radiant_spark.remains<=variable.harmony_stack_time+execute_time&cooldown.touch_of_the_magi.remains<=variable.harmony_stack_time+execute_time+action.radiant_spark.execute_time&cooldown.arcane_power.remains<=variable.harmony_stack_time+execute_time+action.radiant_spark.execute_time+action.touch_of_the_magi.execute_time,chain=1" );
  aoe->add_action( "touch_of_the_magi,if=runeforge.siphon_storm&prev_gcd.1.evocation" );
  aoe->add_action( "arcane_power,if=runeforge.siphon_storm&(prev_gcd.1.evocation|prev_gcd.1.touch_of_the_magi)" );
  aoe->add_action( "evocation,if=time>30&runeforge.siphon_storm&buff.arcane_charge.stack<=variable.aoe_totm_max_charges&cooldown.touch_of_the_magi.remains=0&cooldown.arcane_power.remains<=gcd" );
  aoe->add_action( "evocation,if=time>30&runeforge.siphon_storm&cooldown.arcane_power.remains=0&(((cooldown.touch_of_the_magi.remains>variable.ap_max_delay_for_totm&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack<=variable.aoe_totm_max_charges))&buff.rune_of_power.down),interrupt_if=buff.siphon_storm.stack=buff.siphon_storm.max_stack,interrupt_immediate=1" );
  aoe->add_action( "mirrors_of_torment,if=(cooldown.arcane_power.remains>45|cooldown.arcane_power.remains<=3)&cooldown.touch_of_the_magi.remains=0&(buff.arcane_charge.stack<=variable.aoe_totm_max_charges&((talent.rune_of_power&cooldown.rune_of_power.remains<=gcd&cooldown.arcane_power.remains>5)|(!talent.rune_of_power&cooldown.arcane_power.remains>5)|cooldown.arcane_power.remains<=gcd))" );
  aoe->add_action( "radiant_spark,if=cooldown.touch_of_the_magi.remains<execute_time&((talent.rune_of_power&cooldown.rune_of_power.remains<=gcd&cooldown.arcane_power.remains>variable.totm_max_delay_for_ap)|(!talent.rune_of_power&cooldown.arcane_power.remains>variable.totm_max_delay_for_ap)|cooldown.arcane_power.remains<=gcd)" );
  aoe->add_action( "radiant_spark,if=cooldown.arcane_power.remains<execute_time&(((cooldown.touch_of_the_magi.remains>variable.ap_max_delay_for_totm&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack<=variable.aoe_totm_max_charges))&buff.rune_of_power.down)" );
  aoe->add_action( "deathborne,if=cooldown.arcane_power.remains=0&(((cooldown.touch_of_the_magi.remains>variable.ap_max_delay_for_totm&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack<=variable.aoe_totm_max_charges))&buff.rune_of_power.down)" );
  aoe->add_action( "touch_of_the_magi,if=(buff.arcane_charge.stack<=variable.aoe_totm_max_charges|prev_gcd.1.radiant_spark)&((talent.rune_of_power&cooldown.rune_of_power.remains<=gcd&cooldown.arcane_power.remains>variable.totm_max_delay_for_ap)|(!talent.rune_of_power&cooldown.arcane_power.remains>variable.totm_max_delay_for_ap)|cooldown.arcane_power.remains<=gcd)" );
  aoe->add_action( "arcane_power,if=((cooldown.touch_of_the_magi.remains>variable.ap_max_delay_for_totm&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack<=variable.aoe_totm_max_charges))&buff.rune_of_power.down" );
  aoe->add_action( "rune_of_power,if=buff.rune_of_power.down&((cooldown.touch_of_the_magi.remains>20&buff.arcane_charge.stack=buff.arcane_charge.max_stack)|(cooldown.touch_of_the_magi.remains=0&buff.arcane_charge.stack<=variable.aoe_totm_max_charges))&(cooldown.arcane_power.remains>12|debuff.touch_of_the_magi.up)" );
  aoe->add_action( "shifting_power,if=cooldown.arcane_orb.remains>5|!talent.arcane_orb" );
  aoe->add_action( "arcane_blast,if=covenant.kyrian&runeforge.arcane_infinity&buff.arcane_power.up&debuff.radiant_spark_vulnerability.stack=4&prev_gcd.1.arcane_orb&active_enemies<7" );
  aoe->add_action( "arcane_barrage,if=covenant.kyrian&runeforge.arcane_infinity&buff.arcane_power.up&debuff.radiant_spark_vulnerability.stack=4" );
  aoe->add_action( "arcane_blast,if=covenant.kyrian&runeforge.arcane_infinity&buff.arcane_power.up&(dot.radiant_spark.remains>6|debuff.radiant_spark_vulnerability.up)&debuff.radiant_spark_vulnerability.stack<4&active_enemies=3" );
  aoe->add_action( "arcane_orb,if=covenant.kyrian&runeforge.arcane_infinity&buff.arcane_power.up&debuff.radiant_spark_vulnerability.stack=3" );
  aoe->add_action( "arcane_barrage,if=covenant.kyrian&runeforge.arcane_infinity&buff.arcane_power.up&debuff.radiant_spark_vulnerability.stack=2" );
  aoe->add_action( "arcane_explosion,if=covenant.kyrian&runeforge.arcane_infinity&buff.arcane_power.up&prev_gcd.2.radiant_spark&active_enemies>3" );
  aoe->add_action( "arcane_explosion,if=covenant.kyrian&runeforge.arcane_infinity&buff.arcane_power.up&debuff.radiant_spark_vulnerability.stack=1&active_enemies>3" );
  aoe->add_action( "presence_of_mind,if=buff.deathborne.up&debuff.touch_of_the_magi.up&debuff.touch_of_the_magi.remains<=buff.presence_of_mind.max_stack*action.arcane_blast.execute_time&((talent.resonance&active_enemies<4)|active_enemies<5)&(!runeforge.arcane_bombardment|target.health.pct>35)" );
  aoe->add_action( "arcane_blast,if=buff.deathborne.up&((talent.resonance&active_enemies<4)|active_enemies<5)&(!runeforge.arcane_bombardment|target.health.pct>35)" );
  aoe->add_action( "supernova" );
  aoe->add_action( "arcane_barrage,if=buff.arcane_charge.stack>=(active_enemies-1)&runeforge.arcane_bombardment&target.health.pct<35" );
  aoe->add_action( "arcane_barrage,if=buff.arcane_charge.stack=buff.arcane_charge.max_stack" );
  aoe->add_action( "arcane_orb,if=buff.arcane_charge.stack=0&(cooldown.arcane_power.remains>15|!(covenant.kyrian&runeforge.arcane_infinity))" );
  aoe->add_action( "nether_tempest,if=(refreshable|!ticking)&buff.arcane_charge.stack=buff.arcane_charge.max_stack" );
  aoe->add_action( "arcane_missiles,if=buff.clearcasting.react&runeforge.arcane_infinity&((talent.amplification&active_enemies<8)|active_enemies<5)" );
  aoe->add_action( "arcane_missiles,if=buff.clearcasting.react&talent.arcane_echo&debuff.touch_of_the_magi.up&(talent.amplification|active_enemies<9)" );
  aoe->add_action( "arcane_missiles,if=buff.clearcasting.react&talent.amplification&active_enemies<4" );
  aoe->add_action( "arcane_explosion,if=buff.arcane_charge.stack<buff.arcane_charge.max_stack" );
  aoe->add_action( "arcane_explosion,if=buff.arcane_charge.stack=buff.arcane_charge.max_stack&prev_gcd.1.arcane_barrage" );
  aoe->add_action( "evocation,interrupt_if=mana.pct>=85,interrupt_immediate=1" );

  harmony->add_action( "cancel_action,if=action.evocation.channeling&mana.pct>=95" );
  harmony->add_action( "evocation,if=mana.pct<=30&variable.outside_of_cooldowns&((talent.rune_of_power&cooldown.rune_of_power.remains<10)|(cooldown.touch_of_the_magi.remains<10&conduit.arcane_prodigy))", "If low on mana and cooldowns are coming up, go ahead and evo" );
  harmony->add_action( "arcane_missiles,if=(variable.stack_harmony|time<10)&buff.arcane_harmony.stack<buff.arcane_harmony.max_stack,chain=1" );
  harmony->add_action( "arcane_missiles,if=equipped.empyreal_ordnance&time<30&cooldown.empyreal_ordnance.remains>168", "When using Empyreal Ordnance, cast a few extra missiles while waiting for the buff at the start of the fight" );
  harmony->add_action( "radiant_spark,if=variable.empowered_barrage&cooldown.touch_of_the_magi.remains<=execute_time&cooldown.arcane_power.remains<=(execute_time*2)" );
  harmony->add_action( "touch_of_the_magi,if=variable.just_used_spark&cooldown.arcane_power.remains<=execute_time" );
  harmony->add_action( "arcane_power,if=prev_gcd.1.touch_of_the_magi" );
  harmony->add_action( "rune_of_power,if=variable.empowered_barrage&cooldown.radiant_spark.remains<=execute_time&variable.time_until_ap>=20" );
  harmony->add_action( "radiant_spark,if=variable.empowered_barrage&prev_gcd.1.rune_of_power" );
  harmony->add_action( "touch_of_the_magi,if=variable.just_used_spark&!variable.holding_totm" );
  harmony->add_action( "rune_of_power,if=buff.arcane_power.down&(variable.time_until_ap>30|cooldown.radiant_spark.remains>12)" );
  harmony->add_action( "radiant_spark,if=variable.empowered_barrage&(buff.arcane_charge.stack>=2|cooldown.arcane_orb.ready)&(!talent.rune_of_power|cooldown.rune_of_power.remains>5)&variable.estimated_ap_cooldown>=30" );
  harmony->add_action( "touch_of_the_magi,if=variable.time_until_ap<50&variable.time_until_ap>30", "When running prodigy, use totm by itself in order to align it with ap" );
  harmony->add_action( "arcane_orb,if=variable.just_used_spark&buff.arcane_charge.stack<buff.arcane_charge.max_stack" );
  harmony->add_action( "arcane_barrage,if=debuff.radiant_spark_vulnerability.stack=debuff.radiant_spark_vulnerability.max_stack" );
  harmony->add_action( "arcane_blast,if=variable.just_used_spark|(debuff.radiant_spark_vulnerability.up&debuff.radiant_spark_vulnerability.stack<debuff.radiant_spark_vulnerability.max_stack)" );
  harmony->add_action( "arcane_orb,if=buff.arcane_charge.stack<3&variable.time_until_ap>10", "Use orb on cd unless ap is coming up soon" );
  harmony->add_action( "arcane_missiles,if=buff.clearcasting.react&buff.arcane_power.up,chain=1" );
  harmony->add_action( "arcane_blast,if=buff.presence_of_mind.up" );
  harmony->add_action( "presence_of_mind,if=buff.arcane_charge.stack<buff.arcane_charge.max_stack&buff.arcane_power.up", "The best use of pom is to use it to quickly build charges during ap after they get dumped by the harmony barrage" );
  harmony->add_action( "arcane_missiles,if=buff.arcane_harmony.stack<buff.arcane_harmony.max_stack,chain=1,interrupt=1,interrupt_global=1" );
  harmony->add_action( "arcane_barrage,if=buff.arcane_charge.stack=buff.arcane_charge.max_stack&variable.empowered_barrage" );
  harmony->add_action( "evocation,if=mana.pct<15" );
  harmony->add_action( "arcane_blast,if=buff.arcane_charge.stack&buff.arcane_charge.stack<buff.arcane_charge.max_stack", "Only use blast to build charges if we already have some from orb" );
  harmony->add_action( "arcane_missiles,if=!(variable.time_until_ap<=10&mana.pct<30),chain=1,interrupt=1,interrupt_global=1", "Fill with am unless ap is coming up and we're low on mana" );
  harmony->add_action( "fire_blast", "Profiles with low master, high haste, or both may not have enough mana to support how mana intensive radiant spark can be, so we'll use fire blast and frostbolt when low on mana" );
  harmony->add_action( "frostbolt" );
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
  precombat->add_action( "variable,name=firestarter_combustion,default=-1,value=1*!talent.pyroclasm,if=variable.firestarter_combustion<0", "APL Variable Option: This variable specifies whether Combustion should be used during Firestarter." );
  precombat->add_action( "variable,name=hot_streak_flamestrike,if=variable.hot_streak_flamestrike=0,value=2*talent.flame_patch+3*!talent.flame_patch", "APL Variable Option: This variable specifies the number of targets at which Hot Streak Flamestrikes outside of Combustion should be used." );
  precombat->add_action( "variable,name=hard_cast_flamestrike,if=variable.hard_cast_flamestrike=0,value=2*talent.flame_patch+3*!talent.flame_patch", "APL Variable Option: This variable specifies the number of targets at which Hard Cast Flamestrikes outside of Combustion should be used as filler." );
  precombat->add_action( "variable,name=combustion_flamestrike,if=variable.combustion_flamestrike=0,value=3*talent.flame_patch+6*!talent.flame_patch", "APL Variable Option: This variable specifies the number of targets at which Hot Streak Flamestrikes are used during Combustion." );
  precombat->add_action( "variable,name=arcane_explosion,if=variable.arcane_explosion=0,value=99*talent.flame_patch+2*!talent.flame_patch", "APL Variable Option: This variable specifies the number of targets at which Arcane Explosion outside of Combustion should be used." );
  precombat->add_action( "variable,name=arcane_explosion_mana,default=40,op=reset", "APL Variable Option: This variable specifies the percentage of mana below which Arcane Explosion will not be used." );
  precombat->add_action( "variable,name=combustion_shifting_power,default=2,op=reset", "APL Variable Option: The number of targets Shifting Power should be used on during Combustion." );
  precombat->add_action( "variable,name=combustion_cast_remains,default=0.7,op=reset", "APL Variable Option: The time remaining on a cast when Combustion can be used in seconds." );
  precombat->add_action( "variable,name=overpool_fire_blasts,default=0,op=reset", "APL Variable Option: This variable specifies the number of seconds of Fire Blast that should be pooled past the default amount." );
  precombat->add_action( "variable,name=empyreal_ordnance_delay,default=18,op=reset", "APL Variable Option: How long before Combustion should Empyreal Ordnance be used?" );
  precombat->add_action( "variable,name=time_to_combustion,value=fight_remains+100,if=variable.disable_combustion", "If Combustion is disabled, schedule the first Combustion far after the fight ends." );
  precombat->add_action( "variable,name=skb_duration,value=dbc.effect.828420.base_value", "The duration of a Sun King's Blessing Combustion." );
  precombat->add_action( "variable,name=combustion_on_use,value=equipped.gladiators_badge|equipped.macabre_sheet_music|equipped.inscrutable_quantum_device|equipped.sunblood_amethyst|equipped.empyreal_ordnance|equipped.flame_of_battle|equipped.wakeners_frond|equipped.instructors_divine_bell", "Whether a usable item used to buff Combustion is equipped." );
  precombat->add_action( "variable,name=on_use_cutoff,value=20,if=variable.combustion_on_use", "How long before Combustion should trinkets that trigger a shared category cooldown on other trinkets not be used?" );
  precombat->add_action( "variable,name=on_use_cutoff,value=25,if=equipped.macabre_sheet_music" );
  precombat->add_action( "variable,name=on_use_cutoff,value=20+variable.empyreal_ordnance_delay,if=equipped.empyreal_ordnance" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "use_item,name=soul_igniter,if=!variable.combustion_on_use&!equipped.dreadfire_vessel&(!talent.firestarter|variable.firestarter_combustion)" );
  precombat->add_action( "mirror_image" );
  precombat->add_action( "pyroblast" );

  default_->add_action( "counterspell,if=!runeforge.disciplinary_command" );
  default_->add_action( "call_action_list,name=combustion_timing,if=!variable.disable_combustion", "The combustion_timing action list schedules when Combustion will be used and stores the result in variable.time_to_combustion." );
  default_->add_action( "variable,name=shifting_power_before_combustion,value=(active_enemies<variable.combustion_shifting_power|active_enemies<variable.combustion_flamestrike|variable.time_to_combustion-action.shifting_power.full_reduction>cooldown.shifting_power.duration)&variable.time_to_combustion-cooldown.shifting_power.remains>action.shifting_power.full_reduction&(cooldown.rune_of_power.remains-cooldown.shifting_power.remains>5|!talent.rune_of_power)", "Variable that estimates whether Shifting Power will be used before Combustion is ready." );
  default_->add_action( "shifting_power,if=buff.combustion.down&action.fire_blast.charges<=1&!(buff.infernal_cascade.up&buff.hot_streak.react)&variable.shifting_power_before_combustion" );
  default_->add_action( "radiant_spark,if=buff.combustion.down&(variable.time_to_combustion<variable.combustion_precast_time+execute_time|variable.time_to_combustion>cooldown-10)" );
  default_->add_action( "deathborne,if=buff.combustion.down&buff.rune_of_power.down&variable.time_to_combustion<variable.combustion_precast_time+execute_time" );
  default_->add_action( "mirrors_of_torment,if=variable.time_to_combustion<variable.combustion_precast_time+execute_time&buff.combustion.down" );
  default_->add_action( "fire_blast,use_while_casting=1,if=action.mirrors_of_torment.executing&full_recharge_time-action.mirrors_of_torment.execute_remains<4&!hot_streak_spells_in_flight&!buff.hot_streak.react", "For Venthyr, use a Fire Blast charge during Mirrors of Torment cast to avoid capping charges." );
  default_->add_action( "use_item,effect_name=gladiators_badge,if=variable.time_to_combustion>cooldown-5" );
  default_->add_action( "use_item,name=empyreal_ordnance,if=variable.time_to_combustion<=variable.empyreal_ordnance_delay&variable.time_to_combustion>variable.empyreal_ordnance_delay-5" );
  default_->add_action( "use_item,name=glyph_of_assimilation,if=variable.time_to_combustion>=variable.on_use_cutoff" );
  default_->add_action( "use_item,name=macabre_sheet_music,if=variable.time_to_combustion<=5" );
  default_->add_action( "use_item,name=dreadfire_vessel,if=variable.time_to_combustion>=variable.on_use_cutoff&(buff.infernal_cascade.stack=buff.infernal_cascade.max_stack|!conduit.infernal_cascade|variable.combustion_on_use|variable.time_to_combustion>interpolated_fight_remains%%(cooldown+10))", "If using a steroid on-use item, always use Dreadfire Vessel outside of Combustion. Otherwise, prioritize using Dreadfire Vessel with Combustion only if Infernal Cascade is enabled and a usage won't be lost over the duration of the fight. This adds a small value to the cooldown of Dreadfire Vessel when doing this calculation because it is unrealstic to assume that it will be used perfectly on cooldown." );
  default_->add_action( "use_item,name=soul_igniter,if=(variable.time_to_combustion>=30*(variable.on_use_cutoff>0)|cooldown.item_cd_1141.remains)&(!equipped.dreadfire_vessel|cooldown.dreadfire_vessel_349857.remains>5)", "Soul Igniter should be used in a way that doesn't interfere with other on-use trinkets. Other trinkets do not trigger a shared ICD on it, so it can be used right after any other on-use trinket." );
  default_->add_action( "cancel_buff,name=soul_ignition,if=!conduit.infernal_cascade&time<5|buff.infernal_cascade.stack=buff.infernal_cascade.max_stack", "Trigger Soul Igniter early with Infernal Cascade or when it was precast." );
  default_->add_action( "use_item,name=inscrutable_quantum_device,if=equipped.gladiators_badge&variable.time_to_combustion>=variable.on_use_cutoff", "Items that do not benefit Combustion should just be used outside of Combustion at some point." );
  default_->add_action( "use_item,name=flame_of_battle,if=equipped.gladiators_badge&variable.time_to_combustion>=variable.on_use_cutoff" );
  default_->add_action( "use_item,name=wakeners_frond,if=equipped.gladiators_badge&variable.time_to_combustion>=variable.on_use_cutoff" );
  default_->add_action( "use_item,name=instructors_divine_bell,if=equipped.gladiators_badge&variable.time_to_combustion>=variable.on_use_cutoff" );
  default_->add_action( "use_item,name=sunblood_amethyst,if=equipped.gladiators_badge&variable.time_to_combustion>=variable.on_use_cutoff" );
  default_->add_action( "use_items,if=variable.time_to_combustion>=variable.on_use_cutoff" );
  default_->add_action( "frost_nova,if=runeforge.grisly_icicle&buff.combustion.down&(variable.time_to_combustion>cooldown|variable.time_to_combustion<variable.combustion_precast_time+execute_time)", "Use Frost Nova to trigger Grisly Icicle." );
  default_->add_action( "counterspell,if=runeforge.disciplinary_command&cooldown.buff_disciplinary_command.remains<action.frostbolt.cast_time&buff.disciplinary_command_arcane.down&!buff.disciplinary_command.up&(variable.time_to_combustion+action.frostbolt.cast_time>cooldown.buff_disciplinary_command.duration|variable.time_to_combustion<5)", "Get the Disciplinary Command buff up, unless combustion is soon." );
  default_->add_action( "arcane_explosion,if=runeforge.disciplinary_command&cooldown.buff_disciplinary_command.remains<execute_time+action.frostbolt.cast_time&buff.disciplinary_command_arcane.down&!buff.disciplinary_command.up&(variable.time_to_combustion+execute_time+action.frostbolt.cast_time>cooldown.buff_disciplinary_command.duration|variable.time_to_combustion<5&!talent.rune_of_power)" );
  default_->add_action( "frostbolt,if=runeforge.disciplinary_command&cooldown.buff_disciplinary_command.remains<cast_time&buff.disciplinary_command_frost.down&!buff.disciplinary_command.up&(variable.time_to_combustion+cast_time>cooldown.buff_disciplinary_command.duration|variable.time_to_combustion<5)" );
  default_->add_action( "frost_nova,if=runeforge.disciplinary_command&cooldown.buff_disciplinary_command.ready&buff.disciplinary_command_frost.down&!buff.disciplinary_command.up&(variable.time_to_combustion>cooldown.buff_disciplinary_command.duration|variable.time_to_combustion<5)" );
  default_->add_action( "call_action_list,name=combustion_phase,if=variable.time_to_combustion<=0|variable.time_to_combustion<variable.combustion_precast_time&cooldown.combustion.remains<variable.combustion_precast_time" );
  default_->add_action( "rune_of_power,if=buff.rune_of_power.down&!buff.firestorm.react&(variable.time_to_combustion>=buff.rune_of_power.duration&variable.time_to_combustion>action.fire_blast.full_recharge_time|variable.time_to_combustion>fight_remains)" );
  default_->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=fire_blast_pooling,value=action.fire_blast.charges_fractional+(variable.time_to_combustion+action.shifting_power.full_reduction*variable.shifting_power_before_combustion)%cooldown.fire_blast.duration-1<cooldown.fire_blast.max_charges+variable.overpool_fire_blasts%cooldown.fire_blast.duration-(buff.combustion.duration%cooldown.fire_blast.duration)%%1&variable.time_to_combustion<fight_remains", "Pool as many Fire Blasts as possible for Combustion. Subtract out of the fractional component of the number of Fire Blasts that will naturally recharge during the Combustion phase because pooling anything past that will not grant an extra Fire Blast during Combustion." );
  default_->add_action( "variable,name=phoenix_pooling,if=active_enemies<variable.combustion_flamestrike,value=variable.time_to_combustion+buff.combustion.duration-5<action.phoenix_flames.full_recharge_time+cooldown.phoenix_flames.duration-action.shifting_power.full_reduction*variable.shifting_power_before_combustion&variable.time_to_combustion<fight_remains|runeforge.sun_kings_blessing|time<5", "Variable that controls Phoenix Flames usage to ensure its charges are pooled for Combustion. Only use Phoenix Flames outside of Combustion when full charges can be obtained during the next Combustion." );
  default_->add_action( "variable,name=phoenix_pooling,if=active_enemies>=variable.combustion_flamestrike,value=variable.time_to_combustion<action.phoenix_flames.full_recharge_time-action.shifting_power.full_reduction*variable.shifting_power_before_combustion&variable.time_to_combustion<fight_remains|runeforge.sun_kings_blessing|time<5", "When using Flamestrike in Combustion, save as many charges as possible for Combustion without capping." );
  default_->add_action( "call_action_list,name=rop_phase,if=buff.rune_of_power.up&variable.time_to_combustion>0" );
  default_->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=fire_blast_pooling,value=cooldown.rune_of_power.remains<action.fire_blast.full_recharge_time-action.shifting_power.full_reduction*(variable.shifting_power_before_combustion&cooldown.shifting_power.remains<cooldown.rune_of_power.remains)&cooldown.rune_of_power.remains<fight_remains,if=!variable.fire_blast_pooling&talent.rune_of_power&buff.rune_of_power.down", "Adjust the variable that controls Fire Blast usage to ensure its charges are also pooled for Rune of Power." );
  default_->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=!variable.fire_blast_pooling&variable.time_to_combustion>0&active_enemies>=variable.hard_cast_flamestrike&!firestarter.active&!buff.hot_streak.react&(buff.heating_up.react&action.flamestrike.execute_remains<0.5|charges_fractional>=2)", "When Hardcasting Flame Strike, Fire Blasts should be used to generate Hot Streaks and to extend Blaster Master." );
  default_->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=firestarter.active&charges>=1&!variable.fire_blast_pooling&(!action.fireball.executing&!action.pyroblast.in_flight&buff.heating_up.react|action.fireball.executing&!buff.hot_streak.react|action.pyroblast.in_flight&buff.heating_up.react&!buff.hot_streak.react)", "During Firestarter, Fire Blasts are used similarly to during Combustion. Generally, they are used to generate Hot Streaks when crits will not be wasted and with Blaster Master, they should be spread out to maintain the Blaster Master buff." );
  default_->add_action( "fire_blast,use_while_casting=1,if=action.shifting_power.executing&full_recharge_time<action.shifting_power.tick_reduction&buff.hot_streak.down&time>10", "Avoid capping Fire Blast charges while channeling Shifting Power" );
  default_->add_action( "call_action_list,name=standard_rotation,if=variable.time_to_combustion>0&buff.rune_of_power.down" );
  default_->add_action( "scorch" );

  active_talents->add_action( "living_bomb,if=active_enemies>1&buff.combustion.down&(variable.time_to_combustion>cooldown.living_bomb.duration|variable.time_to_combustion<=0)" );
  active_talents->add_action( "meteor,if=variable.time_to_combustion<=0|(cooldown.meteor.duration<variable.time_to_combustion&!talent.rune_of_power)|talent.rune_of_power&buff.rune_of_power.up&variable.time_to_combustion>action.meteor.cooldown|fight_remains<variable.time_to_combustion" );
  active_talents->add_action( "dragons_breath,if=talent.alexstraszas_fury&(buff.combustion.down&!buff.hot_streak.react)" );

  combustion_cooldowns->add_action( "potion" );
  combustion_cooldowns->add_action( "blood_fury" );
  combustion_cooldowns->add_action( "berserking,if=buff.combustion.up" );
  combustion_cooldowns->add_action( "fireblood" );
  combustion_cooldowns->add_action( "ancestral_call" );
  combustion_cooldowns->add_action( "time_warp,if=runeforge.temporal_warp&buff.exhaustion.up" );
  combustion_cooldowns->add_action( "use_item,effect_name=gladiators_badge" );
  combustion_cooldowns->add_action( "use_item,name=inscrutable_quantum_device" );
  combustion_cooldowns->add_action( "use_item,name=flame_of_battle" );
  combustion_cooldowns->add_action( "use_item,name=wakeners_frond" );
  combustion_cooldowns->add_action( "use_item,name=instructors_divine_bell" );
  combustion_cooldowns->add_action( "use_item,name=sunblood_amethyst" );

  combustion_phase->add_action( "lights_judgment,if=buff.combustion.down" );
  combustion_phase->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=extended_combustion_remains,value=buff.combustion.remains+buff.combustion.duration*(cooldown.combustion.remains<buff.combustion.remains),if=conduit.infernal_cascade", "Estimate how long Combustion will last thanks to Sun King's Blessing to determine how Fire Blasts should be used." );
  combustion_phase->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=extended_combustion_remains,op=add,value=variable.skb_duration,if=conduit.infernal_cascade&(buff.sun_kings_blessing_ready.up|variable.extended_combustion_remains>1.5*gcd.max*(buff.sun_kings_blessing.max_stack-buff.sun_kings_blessing.stack))", "Adds the duration of the Sun King's Blessing Combustion to the end of the current Combustion if the cast would complete during this Combustion." );
  combustion_phase->add_action( "bag_of_tricks,if=buff.combustion.down" );
  combustion_phase->add_action( "living_bomb,if=active_enemies>1&buff.combustion.down" );
  combustion_phase->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=!conduit.infernal_cascade&charges>=1&buff.combustion.up&!buff.firestorm.react&!buff.hot_streak.react&hot_streak_spells_in_flight+buff.heating_up.react<2", "Without Infernal Cascade, just use Fire Blasts when they won't munch crits and when Firestorm is down." );
  combustion_phase->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=expected_fire_blasts,value=action.fire_blast.charges_fractional+(variable.extended_combustion_remains-buff.infernal_cascade.duration)%cooldown.fire_blast.duration,if=conduit.infernal_cascade", "With Infernal Cascade, Fire Blast use should be additionally constrained so that it is not be used unless Infernal Cascade is about to expire or there are more than enough Fire Blasts to extend Infernal Cascade to the end of Combustion." );
  combustion_phase->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=needed_fire_blasts,value=ceil(variable.extended_combustion_remains%(buff.infernal_cascade.duration-gcd.max)),if=conduit.infernal_cascade" );
  combustion_phase->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=conduit.infernal_cascade&charges>=1&(variable.expected_fire_blasts>=variable.needed_fire_blasts|variable.extended_combustion_remains<=buff.infernal_cascade.duration|buff.infernal_cascade.stack<2|buff.infernal_cascade.remains<gcd.max|cooldown.shifting_power.ready&active_enemies>=variable.combustion_shifting_power&covenant.night_fae)&buff.combustion.up&(!buff.firestorm.react|buff.infernal_cascade.remains<0.5)&!buff.hot_streak.react&hot_streak_spells_in_flight+buff.heating_up.react<2" );
  combustion_phase->add_action( "call_action_list,name=active_talents" );
  combustion_phase->add_action( "combustion,use_off_gcd=1,use_while_casting=1,if=buff.combustion.down&variable.time_to_combustion<=0&(!runeforge.disciplinary_command|buff.disciplinary_command.up|buff.disciplinary_command_frost.up&talent.rune_of_power&cooldown.buff_disciplinary_command.ready)&(!runeforge.grisly_icicle|debuff.grisly_icicle.up)&(!covenant.necrolord|cooldown.deathborne.remains|buff.deathborne.up)&(action.meteor.in_flight&action.meteor.in_flight_remains<=variable.combustion_cast_remains|action.scorch.executing&action.scorch.execute_remains<variable.combustion_cast_remains|action.fireball.executing&action.fireball.execute_remains<variable.combustion_cast_remains|action.pyroblast.executing&action.pyroblast.execute_remains<variable.combustion_cast_remains|action.flamestrike.executing&action.flamestrike.execute_remains<variable.combustion_cast_remains)" );
  combustion_phase->add_action( "call_action_list,name=combustion_cooldowns,if=buff.combustion.remains>8|cooldown.combustion.remains<5", "Other cooldowns that should be used with Combustion should only be used with an actual Combustion cast and not with a Sun King's Blessing proc." );
  combustion_phase->add_action( "flamestrike,if=(buff.hot_streak.react&active_enemies>=variable.combustion_flamestrike)|(buff.firestorm.react&active_enemies>=variable.combustion_flamestrike-runeforge.firestorm)" );
  combustion_phase->add_action( "pyroblast,if=buff.sun_kings_blessing_ready.up&buff.sun_kings_blessing_ready.remains>cast_time" );
  combustion_phase->add_action( "pyroblast,if=buff.firestorm.react" );
  combustion_phase->add_action( "pyroblast,if=buff.pyroclasm.react&buff.pyroclasm.remains>cast_time&(buff.combustion.remains>cast_time|buff.combustion.down)&active_enemies<variable.combustion_flamestrike" );
  combustion_phase->add_action( "pyroblast,if=buff.hot_streak.react&buff.combustion.up" );
  combustion_phase->add_action( "pyroblast,if=prev_gcd.1.scorch&buff.heating_up.react&active_enemies<variable.combustion_flamestrike&buff.combustion.up" );
  combustion_phase->add_action( "shifting_power,if=buff.combustion.up&!action.fire_blast.charges&active_enemies>=variable.combustion_shifting_power&action.phoenix_flames.full_recharge_time>full_reduction,interrupt_if=action.fire_blast.charges=action.fire_blast.max_charges", "Using Shifting Power during Combustion to restore Fire Blast and Phoenix Flame charges can be beneficial, but usually only on AoE." );
  combustion_phase->add_action( "phoenix_flames,if=buff.combustion.up&travel_time<buff.combustion.remains&((action.fire_blast.charges<1&talent.pyroclasm&active_enemies=1)|!talent.pyroclasm|active_enemies>1)&buff.heating_up.react+hot_streak_spells_in_flight<2" );
  combustion_phase->add_action( "flamestrike,if=buff.combustion.down&cooldown.combustion.remains<cast_time&active_enemies>=variable.combustion_flamestrike" );
  combustion_phase->add_action( "fireball,if=buff.combustion.down&cooldown.combustion.remains<cast_time&!conduit.flame_accretion" );
  combustion_phase->add_action( "scorch,if=buff.combustion.remains>cast_time&buff.combustion.up|buff.combustion.down&cooldown.combustion.remains<cast_time" );
  combustion_phase->add_action( "living_bomb,if=buff.combustion.remains<gcd.max&active_enemies>1" );
  combustion_phase->add_action( "dragons_breath,if=buff.combustion.remains<gcd.max&buff.combustion.up" );

  combustion_timing->add_action( "variable,name=combustion_ready_time,value=cooldown.combustion.remains*expected_kindling_reduction", "Helper variable that contains the actual estimated time that the next Combustion will be ready." );
  combustion_timing->add_action( "variable,name=combustion_precast_time,value=(action.fireball.cast_time*!conduit.flame_accretion+action.scorch.cast_time+conduit.flame_accretion)*(active_enemies<variable.combustion_flamestrike)+action.flamestrike.cast_time*(active_enemies>=variable.combustion_flamestrike)-variable.combustion_cast_remains", "The cast time of the spell that will be precast into Combustion." );
  combustion_timing->add_action( "variable,name=combustion_time,value=variable.combustion_ready_time" );
  combustion_timing->add_action( "variable,name=combustion_time,op=max,value=firestarter.remains,if=talent.firestarter&!variable.firestarter_combustion", "Delay Combustion for after Firestarter unless variable.firestarter_combustion is set." );
  combustion_timing->add_action( "variable,name=combustion_time,op=max,value=cooldown.radiant_spark.remains,if=covenant.kyrian&cooldown.radiant_spark.remains-10<variable.combustion_time", "Delay Combustion for Radiant Spark if it will come off cooldown soon." );
  combustion_timing->add_action( "variable,name=combustion_time,op=max,value=cooldown.deathborne.remains,if=covenant.necrolord&cooldown.deathborne.remains-10<variable.combustion_time", "Delay Combustion for Deathborne." );
  combustion_timing->add_action( "variable,name=combustion_time,op=max,value=variable.empyreal_ordnance_delay-(cooldown.empyreal_ordnance.duration-cooldown.empyreal_ordnance.remains)*!cooldown.empyreal_ordnance.ready,if=equipped.empyreal_ordnance", "Delay Combustion for the Empyreal Ordnance buff if the player is using that trinket." );
  combustion_timing->add_action( "variable,name=combustion_time,op=max,value=cooldown.gladiators_badge_345228.remains,if=equipped.gladiators_badge&cooldown.gladiators_badge_345228.remains-20<variable.combustion_time", "Delay Combustion for Gladiators Badge, unless it would be delayed longer than 20 seconds." );
  combustion_timing->add_action( "variable,name=combustion_time,op=max,value=buff.rune_of_power.remains,if=talent.rune_of_power&buff.combustion.down", "Delay Combustion until RoP expires if it's up." );
  combustion_timing->add_action( "variable,name=combustion_time,op=max,value=cooldown.rune_of_power.remains+buff.rune_of_power.duration,if=talent.rune_of_power&buff.combustion.down&cooldown.rune_of_power.remains+5<variable.combustion_time", "Delay Combustion for an extra Rune of Power if the Rune of Power would come off cooldown at least 5 seconds before Combustion would." );
  combustion_timing->add_action( "variable,name=combustion_time,op=max,value=cooldown.buff_disciplinary_command.remains,if=runeforge.disciplinary_command&buff.disciplinary_command.down", "Delay Combustion if Disciplinary Command would not be ready for it yet." );
  combustion_timing->add_action( "variable,name=combustion_time,op=max,value=raid_event.adds.in,if=raid_event.adds.exists&raid_event.adds.count>=3&raid_event.adds.duration>15", "Raid Events: Delay Combustion for add spawns of 3 or more adds that will last longer than 15 seconds. These values aren't necessarily optimal in all cases." );
  combustion_timing->add_action( "variable,name=combustion_time,value=raid_event.vulnerable.in*!raid_event.vulnerable.up,if=raid_event.vulnerable.exists&variable.combustion_ready_time<raid_event.vulnerable.in", "Raid Events: Always use Combustion with vulnerability raid events, override any delays listed above to make sure it gets used here." );
  combustion_timing->add_action( "variable,name=combustion_time,value=variable.combustion_ready_time,if=variable.combustion_ready_time+cooldown.combustion.duration*(1-(0.6+0.2*talent.firestarter)*talent.kindling)<=variable.combustion_time|variable.combustion_time>fight_remains-20", "Use the next Combustion on cooldown if it would not be expected to delay the scheduled one or the scheduled one would happen less than 20 seconds before the fight ends." );
  combustion_timing->add_action( "variable,name=combustion_time,op=add,value=time", "Add the current time to the scheduled Combustion to put it in absolute time so that it is still accurate after a little time passes." );
  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=time_to_combustion,value=(variable.combustion_time-time)*buff.combustion.down", "Finally, convert from absolute time and store the relative time in variable.time_to_combustion. Unlike the rest of the calculations, which happen less frequently to speed up the simulation, this happens off-GCD and while casting." );

  rop_phase->add_action( "flamestrike,if=active_enemies>=variable.hot_streak_flamestrike&(buff.hot_streak.react|buff.firestorm.react)" );
  rop_phase->add_action( "pyroblast,if=buff.sun_kings_blessing_ready.up&buff.sun_kings_blessing_ready.remains>cast_time" );
  rop_phase->add_action( "pyroblast,if=buff.firestorm.react" );
  rop_phase->add_action( "pyroblast,if=buff.hot_streak.react" );
  rop_phase->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=!variable.fire_blast_pooling&buff.sun_kings_blessing_ready.down&active_enemies<variable.hard_cast_flamestrike&!firestarter.active&(!buff.heating_up.react&!buff.hot_streak.react&!prev_off_gcd.fire_blast&(action.fire_blast.charges>=2|(talent.alexstraszas_fury&cooldown.dragons_breath.ready)|searing_touch.active))", "Use one Fire Blast early in RoP if you don't have either Heating Up or Hot Streak yet and either: (a) have more than two already, (b) have Alexstrasza's Fury ready to use, or (c) Searing Touch is active. Don't do this while hard casting Flamestrikes or when Sun King's Blessing is ready." );
  rop_phase->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=!variable.fire_blast_pooling&!firestarter.active&(((action.fireball.executing&(action.fireball.execute_remains<0.5|!runeforge.firestorm)|action.pyroblast.executing&(action.pyroblast.execute_remains<0.5|!runeforge.firestorm))&buff.heating_up.react)|(searing_touch.active&(buff.heating_up.react&!action.scorch.executing|!buff.hot_streak.react&!buff.heating_up.react&action.scorch.executing&!hot_streak_spells_in_flight)))", "Use Fire Blast either during a Fireball/Pyroblast cast when Heating Up is active or during execute with Searing Touch." );
  rop_phase->add_action( "call_action_list,name=active_talents" );
  rop_phase->add_action( "pyroblast,if=buff.pyroclasm.react&cast_time<buff.pyroclasm.remains&cast_time<buff.rune_of_power.remains" );
  rop_phase->add_action( "pyroblast,if=prev_gcd.1.scorch&buff.heating_up.react&searing_touch.active&active_enemies<variable.hot_streak_flamestrike" );
  rop_phase->add_action( "phoenix_flames,if=!variable.phoenix_pooling&buff.heating_up.react&!buff.hot_streak.react&(active_dot.ignite<2|active_enemies>=variable.hard_cast_flamestrike|active_enemies>=variable.hot_streak_flamestrike)" );
  rop_phase->add_action( "scorch,if=searing_touch.active" );
  rop_phase->add_action( "dragons_breath,if=active_enemies>2" );
  rop_phase->add_action( "arcane_explosion,if=active_enemies>=variable.arcane_explosion&mana.pct>=variable.arcane_explosion_mana" );
  rop_phase->add_action( "flamestrike,if=active_enemies>=variable.hard_cast_flamestrike", "With enough targets, it is a gain to cast Flamestrike as filler instead of Fireball." );
  rop_phase->add_action( "fireball" );

  standard_rotation->add_action( "flamestrike,if=active_enemies>=variable.hot_streak_flamestrike&(buff.hot_streak.react|buff.firestorm.react)" );
  standard_rotation->add_action( "pyroblast,if=buff.firestorm.react" );
  standard_rotation->add_action( "pyroblast,if=buff.hot_streak.react&buff.hot_streak.remains<action.fireball.execute_time" );
  standard_rotation->add_action( "pyroblast,if=buff.hot_streak.react&(prev_gcd.1.fireball|firestarter.active|action.pyroblast.in_flight)" );
  standard_rotation->add_action( "pyroblast,if=buff.sun_kings_blessing_ready.up&(cooldown.rune_of_power.remains+action.rune_of_power.execute_time+cast_time>buff.sun_kings_blessing_ready.remains|!talent.rune_of_power)&variable.time_to_combustion+cast_time>buff.sun_kings_blessing_ready.remains", "Try to get SKB procs inside RoP phases or Combustion phases when possible." );
  standard_rotation->add_action( "pyroblast,if=buff.hot_streak.react&searing_touch.active" );
  standard_rotation->add_action( "pyroblast,if=buff.pyroclasm.react&cast_time<buff.pyroclasm.remains" );
  standard_rotation->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=!firestarter.active&!variable.fire_blast_pooling&(((action.fireball.executing&(action.fireball.execute_remains<0.5|!runeforge.firestorm)|action.pyroblast.executing&(action.pyroblast.execute_remains<0.5|!runeforge.firestorm))&buff.heating_up.react)|(searing_touch.active&(buff.heating_up.react&!action.scorch.executing|!buff.hot_streak.react&!buff.heating_up.react&action.scorch.executing&!hot_streak_spells_in_flight)))", "During the standard rotation, only use Fire Blasts when they are not being pooled for RoP or Combustion. Use Fire Blast either during a Fireball/Pyroblast cast when Heating Up is active or during execute with Searing Touch." );
  standard_rotation->add_action( "pyroblast,if=prev_gcd.1.scorch&buff.heating_up.react&searing_touch.active&active_enemies<variable.hot_streak_flamestrike" );
  standard_rotation->add_action( "phoenix_flames,if=!variable.phoenix_pooling&(!talent.from_the_ashes|active_enemies>1)&(active_dot.ignite<2|active_enemies>=variable.hard_cast_flamestrike|active_enemies>=variable.hot_streak_flamestrike)" );
  standard_rotation->add_action( "call_action_list,name=active_talents" );
  standard_rotation->add_action( "dragons_breath,if=active_enemies>1" );
  standard_rotation->add_action( "scorch,if=searing_touch.active" );
  standard_rotation->add_action( "arcane_explosion,if=active_enemies>=variable.arcane_explosion&mana.pct>=variable.arcane_explosion_mana" );
  standard_rotation->add_action( "flamestrike,if=active_enemies>=variable.hard_cast_flamestrike", "With enough targets, it is a gain to cast Flamestrike as filler instead of Fireball." );
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
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "blizzard,if=active_enemies>=2" );
  precombat->add_action( "frostbolt,if=active_enemies=1" );

  default_->add_action( "counterspell,if=!runeforge.disciplinary_command|cooldown.buff_disciplinary_command.ready&buff.disciplinary_command_arcane.down" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=aoe,if=active_enemies>=3" );
  default_->add_action( "call_action_list,name=st,if=active_enemies<3" );
  default_->add_action( "call_action_list,name=movement" );

  aoe->add_action( "frozen_orb" );
  aoe->add_action( "blizzard" );
  aoe->add_action( "flurry,if=(remaining_winters_chill=0|debuff.winters_chill.down)&(prev_gcd.1.ebonbolt|buff.brain_freeze.react&buff.fingers_of_frost.react=0)" );
  aoe->add_action( "ice_nova" );
  aoe->add_action( "comet_storm" );
  aoe->add_action( "ice_lance,if=buff.fingers_of_frost.react|debuff.frozen.remains>travel_time|remaining_winters_chill&debuff.winters_chill.remains>travel_time" );
  aoe->add_action( "radiant_spark,if=soulbind.combat_meditation" );
  aoe->add_action( "mirrors_of_torment" );
  aoe->add_action( "shifting_power" );
  aoe->add_action( "fire_blast,if=runeforge.disciplinary_command&cooldown.buff_disciplinary_command.ready&buff.disciplinary_command_fire.down" );
  aoe->add_action( "arcane_explosion,if=mana.pct>30&active_enemies>=6&!runeforge.glacial_fragments" );
  aoe->add_action( "ebonbolt" );
  aoe->add_action( "ice_lance,if=runeforge.glacial_fragments&talent.splitting_ice&travel_time<ground_aoe.blizzard.remains" );
  aoe->add_action( "wait,sec=0.1,if=runeforge.glacial_fragments&talent.splitting_ice" );
  aoe->add_action( "frostbolt" );

  cds->add_action( "potion,if=prev_off_gcd.icy_veins|fight_remains<30" );
  cds->add_action( "deathborne" );
  cds->add_action( "mirrors_of_torment,if=active_enemies<3&(conduit.siphoned_malice|soulbind.wasteland_propriety)" );
  cds->add_action( "rune_of_power,if=cooldown.icy_veins.remains>12&buff.rune_of_power.down" );
  cds->add_action( "icy_veins,if=buff.rune_of_power.down&(buff.icy_veins.down|talent.rune_of_power)&(buff.slick_ice.down|active_enemies>=2)" );
  cds->add_action( "time_warp,if=runeforge.temporal_warp&buff.exhaustion.up&(prev_off_gcd.icy_veins|fight_remains<40)" );
  cds->add_action( "use_items" );
  cds->add_action( "blood_fury" );
  cds->add_action( "berserking" );
  cds->add_action( "lights_judgment" );
  cds->add_action( "fireblood" );
  cds->add_action( "ancestral_call" );
  cds->add_action( "bag_of_tricks" );

  movement->add_action( "blink_any,if=movement.distance>10" );
  movement->add_action( "ice_floes,if=buff.ice_floes.down" );
  movement->add_action( "arcane_explosion,if=mana.pct>30&active_enemies>=2" );
  movement->add_action( "fire_blast" );
  movement->add_action( "ice_lance" );

  st->add_action( "flurry,if=(remaining_winters_chill=0|debuff.winters_chill.down)&(prev_gcd.1.ebonbolt|buff.brain_freeze.react&(prev_gcd.1.glacial_spike|prev_gcd.1.frostbolt&(!conduit.ire_of_the_ascended|cooldown.radiant_spark.remains|runeforge.freezing_winds)|prev_gcd.1.radiant_spark|buff.fingers_of_frost.react=0&(debuff.mirrors_of_torment.up|buff.freezing_winds.up|buff.expanded_potential.react)))" );
  st->add_action( "frozen_orb" );
  st->add_action( "blizzard,if=buff.freezing_rain.up|active_enemies>=2" );
  st->add_action( "ray_of_frost,if=remaining_winters_chill=1&debuff.winters_chill.remains" );
  st->add_action( "glacial_spike,if=remaining_winters_chill&debuff.winters_chill.remains>cast_time+travel_time" );
  st->add_action( "ice_lance,if=remaining_winters_chill&remaining_winters_chill>buff.fingers_of_frost.react&debuff.winters_chill.remains>travel_time" );
  st->add_action( "comet_storm" );
  st->add_action( "ice_nova" );
  st->add_action( "radiant_spark,if=buff.freezing_winds.up&active_enemies=1" );
  st->add_action( "radiant_spark,if=buff.brain_freeze.react&talent.glacial_spike&conduit.ire_of_the_ascended&buff.icicles.stack>=4" );
  st->add_action( "ice_lance,if=buff.fingers_of_frost.react|debuff.frozen.remains>travel_time" );
  st->add_action( "ebonbolt" );
  st->add_action( "radiant_spark,if=(!talent.glacial_spike|!conduit.ire_of_the_ascended)&(!runeforge.freezing_winds|active_enemies>=2)&buff.brain_freeze.react" );
  st->add_action( "mirrors_of_torment" );
  st->add_action( "shifting_power,if=buff.rune_of_power.down&(soulbind.grove_invigoration|soulbind.field_of_blossoms|runeforge.freezing_winds&buff.freezing_winds.down|active_enemies>=2)" );
  st->add_action( "arcane_explosion,if=runeforge.disciplinary_command&cooldown.buff_disciplinary_command.ready&buff.disciplinary_command_arcane.down" );
  st->add_action( "fire_blast,if=runeforge.disciplinary_command&cooldown.buff_disciplinary_command.ready&buff.disciplinary_command_fire.down" );
  st->add_action( "glacial_spike,if=buff.brain_freeze.react" );
  st->add_action( "frostbolt" );
}
//frost_apl_end

}  // namespace mage_apl
