// ==========================================================================
// Priest APL File
// Contact: https://github.com/orgs/simulationcraft/teams/priest/members
// Wiki: https://github.com/simulationcraft/simc/wiki/Priests
// ==========================================================================

#include "class_modules/apl/apl_priest.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace priest_apl
{
std::string potion( const player_t* p )
{
  return ( p->true_level > 60 ) ? "elemental_potion_of_ultimate_power_3" : "potion_of_spectral_intellect";
}

std::string flask( const player_t* p )
{
  return ( p->true_level > 60 ) ? "phial_of_tepid_versatility_3" : "greater_flask_of_endless_fathoms";
}

std::string food( const player_t* p )
{
  return ( p->true_level > 60 ) ? "fated_fortune_cookie" : "feast_of_gluttonous_hedonism";
}

std::string rune( const player_t* p )
{
  return ( p->true_level > 60 ) ? "draconic_augment_rune" : "veiled_augment_rune";
}

std::string temporary_enchant( const player_t* p )
{
  return ( p->true_level > 60 ) ? "main_hand:howling_rune_3" : "main_hand:shadowcore_oil";
}

//shadow_apl_start
void shadow( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* main_variables = p->get_action_priority_list( "main_variables" );
  action_priority_list_t* aoe_variables = p->get_action_priority_list( "aoe_variables" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* main = p->get_action_priority_list( "main" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* pl_torrent = p->get_action_priority_list( "pl_torrent" );
  action_priority_list_t* aoe_pl_ire = p->get_action_priority_list( "aoe_pl_ire" );
  action_priority_list_t* filler = p->get_action_priority_list( "filler" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );

  precombat->add_action( "flask", "otion=elemental_potion_of_ultimate_power_ lask=phial_of_tepid_versatility_ ood=fated_fortune_cooki ugmentation=draconic_augment_run emporary_enchant=main_hand:howling_rune_" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "shadowform,if=!buff.shadowform.up" );
  precombat->add_action( "arcane_torrent" );
  precombat->add_action( "variable,name=mind_sear_cutoff,op=set,value=2" );
  precombat->add_action( "variable,name=pool_amount,op=set,value=60" );
  precombat->add_action( "shadow_crash,if=raid_event.adds.in>=25&spell_targets.shadow_crash<=8&!fight_style.dungeonslice" );
  precombat->add_action( "mind_blast,if=talent.damnation.enabled&(!talent.shadow_crash.enabled|raid_event.adds.in>=25&spell_targets.shadow_crash<=8|fight_style.dungeonslice)" );
  precombat->add_action( "vampiric_touch,if=!talent.damnation.enabled&(!talent.shadow_crash.enabled|raid_event.adds.in<25|spell_targets.shadow_crash>8|fight_style.dungeonslice)" );

  default_->add_action( "variable,name=dp_cutoff,op=set,value=!talent.mind_sear|(spell_targets.mind_sear<=variable.mind_sear_cutoff&(!buff.mind_devourer.up|spell_targets.mind_sear=1))" );
  default_->add_action( "variable,name=holding_crash,op=set,value=raid_event.adds.in<20" );
  default_->add_action( "run_action_list,name=aoe,if=spell_targets.mind_sear>2|spell_targets.vampiric_touch>3" );
  default_->add_action( "run_action_list,name=main" );

  main_variables->add_action( "variable,name=dots_up,op=set,value=(dot.shadow_word_pain.ticking&dot.vampiric_touch.ticking)|action.shadow_crash.in_flight" );
  main_variables->add_action( "variable,name=all_dots_up,op=set,value=dot.shadow_word_pain.ticking&dot.vampiric_touch.ticking&dot.devouring_plague.ticking" );
  main_variables->add_action( "variable,name=pool_for_cds,op=set,value=(cooldown.void_eruption.remains<=gcd.max*3&talent.void_eruption|cooldown.dark_ascension.up&talent.dark_ascension)|talent.void_torrent&talent.psychic_link&cooldown.void_torrent.remains<=4&(!raid_event.adds.exists&spell_targets.vampiric_touch>1|raid_event.adds.in<=5|raid_event.adds.remains>=6&!variable.holding_crash)&!buff.voidform.up" );

  aoe_variables->add_action( "variable,name=max_vts,op=set,default=12,value=spell_targets.vampiric_touch>?12" );
  aoe_variables->add_action( "variable,name=is_vt_possible,op=set,value=0,default=1" );
  aoe_variables->add_action( "variable,name=is_vt_possible,op=set,value=1,target_if=max:(target.time_to_die*dot.vampiric_touch.refreshable),if=target.time_to_die>=18" );
  aoe_variables->add_action( "variable,name=vts_applied,op=set,value=(active_dot.vampiric_touch+8*action.shadow_crash.in_flight)>=variable.max_vts|!variable.is_vt_possible", "Todo Revamp to fix undesired behaviour with unstacked fights" );
  aoe_variables->add_action( "variable,name=holding_crash,op=set,value=(variable.max_vts-active_dot.vampiric_touch)<4|raid_event.adds.in<10&raid_event.adds.count>(variable.max_vts-active_dot.vampiric_touch),if=variable.holding_crash" );
  aoe_variables->add_action( "variable,name=manual_vts_applied,op=set,value=(active_dot.vampiric_touch+8*!variable.holding_crash)>=variable.max_vts|!variable.is_vt_possible" );
  aoe_variables->add_action( "variable,name=pool_for_cds,op=set,value=(cooldown.void_eruption.remains<=gcd.max*3&talent.void_eruption|cooldown.dark_ascension.up&talent.dark_ascension)|talent.void_torrent&talent.psychic_link&cooldown.void_torrent.remains<=4&(!raid_event.adds.exists&spell_targets.vampiric_touch>1|raid_event.adds.in<=5|raid_event.adds.remains>=6&!variable.holding_crash)&!buff.voidform.up" );

  cds->add_action( "potion,if=buff.voidform.up|buff.power_infusion.up|buff.dark_ascension.up&(fight_remains<=cooldown.power_infusion.remains+15)|fight_remains<=30", "Todo Check VE/DA enter conditions based on dots" );
  cds->add_action( "fireblood,if=buff.power_infusion.up|fight_remains<=8" );
  cds->add_action( "berserking,if=buff.power_infusion.up|fight_remains<=12" );
  cds->add_action( "blood_fury,if=buff.power_infusion.up|fight_remains<=15" );
  cds->add_action( "ancestral_call,if=buff.power_infusion.up|fight_remains<=15" );
  cds->add_action( "power_infusion,if=(buff.voidform.up|buff.dark_ascension.up)", "Sync Power Infusion with Voidform or Dark Ascension" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=(buff.voidform.up|buff.dark_ascension.up)&!buff.power_infusion.up", "Use <a href='https://www.wowhead.com/spell=10060/power-infusion'>Power Infusion</a> while <a href='https://www.wowhead.com/spell=194249/voidform'>Voidform</a> or <a href='https://www.wowhead.com/spell=391109/dark-ascension'>Dark Ascension</a> is active. Chain directly after your own <a href='https://www.wowhead.com/spell=10060/power-infusion'>Power Infusion</a>." );
  cds->add_action( "void_eruption,if=!cooldown.fiend.up&(pet.fiend.active|!talent.mindbender|spell_targets.mind_sear>2&talent.inescapable_torment.rank<2)&(cooldown.mind_blast.charges=0|time>15|buff.shadowy_insight.up&cooldown.mind_blast.charges=buff.shadowy_insight.stack)", "Make sure Mindbender is active before popping Void Eruption and dump charges of Mind Blast before casting" );
  cds->add_action( "dark_ascension,if=pet.fiend.active|!talent.mindbender&!cooldown.fiend.up|spell_targets.mind_sear>2&talent.inescapable_torment.rank<2", "Make sure Mindbender is active before popping Dark Ascension unless you have insignificant talent points or too many targets" );
  cds->add_action( "call_action_list,name=trinkets" );
  cds->add_action( "desperate_prayer,if=health.pct<=75", "Use Desperate Prayer to heal up should Shadow Word: Death or other damage bring you below 75%" );

  main->add_action( "call_action_list,name=main_variables" );
  main->add_action( "call_action_list,name=cds,if=fight_remains<30|time_to_die>15&(!variable.holding_crash|spell_targets.mind_sear>2)" );
  main->add_action( "mindbender,if=variable.dots_up&(fight_remains<30|time_to_die>15)" );
  main->add_action( "mind_blast,if=(cooldown.mind_blast.full_recharge_time<=gcd.max+cast_time|pet.fiend.remains<=cast_time+gcd.max)&pet.fiend.active&talent.inescapable_torment&pet.fiend.remains>cast_time&spell_targets.mind_sear<=7", "High priority Mind Blast action when using Inescapable Torment" );
  main->add_action( "damnation,target_if=dot.vampiric_touch.refreshable|dot.shadow_word_pain.refreshable" );
  main->add_action( "void_bolt,if=variable.dots_up&insanity<=85" );
  main->add_action( "mind_sear,target_if=spell_targets.mind_sear>1&buff.mind_devourer.up", "Use Mind Devourer Procs on Mind Sear when facing 2 or more targets" );
  main->add_action( "devouring_plague,if=(refreshable&!variable.pool_for_cds|insanity>75|talent.void_torrent&cooldown.void_torrent.remains<=3*gcd)&variable.dp_cutoff" );
  main->add_action( "vampiric_touch,target_if=min:remains,if=refreshable&target.time_to_die>=12&(cooldown.shadow_crash.remains>=dot.vampiric_touch.remains&!action.shadow_crash.in_flight|variable.holding_crash)" );
  main->add_action( "shadow_word_pain,target_if=min:remains,if=refreshable&target.time_to_die>=18&!talent.misery.enabled" );
  main->add_action( "mind_flay,if=buff.mind_flay_insanity.up&variable.dots_up&(talent.inescapable_torment.rank<2|!pet.fiend.active)&cooldown.mind_blast.full_recharge_time>=gcd*3&talent.idol_of_cthun&(!cooldown.void_torrent.up|!talent.void_torrent)", "High Priority Mind Flay: Insanity to fish for C'Thun procs when Mind Blast is not capped and Void Torrent is not available and Mindbender is not active" );
  main->add_action( "shadow_word_death,target_if=(target.health.pct<20&spell_targets.mind_sear<4)&(talent.inescapable_torment.rank<2|cooldown.fiend.remains>=10)|(pet.fiend.active&talent.inescapable_torment.rank>1&spell_targets.mind_sear<=7)|buff.deathspeaker.up" );
  main->add_action( "mind_blast,if=variable.dots_up&(!buff.mind_devourer.up|cooldown.void_eruption.up&talent.void_eruption)" );
  main->add_action( "mindgames,if=spell_targets.mind_sear<5&variable.all_dots_up", "TODO: Dont use if add events are coming soon when talented into PL" );
  main->add_action( "shadow_crash,if=!variable.holding_crash" );
  main->add_action( "dark_void,if=raid_event.adds.in>20" );
  main->add_action( "devouring_plague,if=buff.voidform.up&variable.dots_up&variable.dp_cutoff" );
  main->add_action( "void_torrent,if=insanity<=35&!variable.holding_crash,target_if=variable.all_dots_up", "TODO: Dont use if add events are coming soon when talented into PL" );
  main->add_action( "call_action_list,name=filler" );

  aoe->add_action( "call_action_list,name=aoe_variables" );
  aoe->add_action( "vampiric_touch,target_if=refreshable&target.time_to_die>=18&(dot.vampiric_touch.ticking|!variable.vts_applied),if=variable.max_vts>0&!variable.manual_vts_applied&!action.shadow_crash.in_flight" );
  aoe->add_action( "shadow_crash,if=!variable.holding_crash" );
  aoe->add_action( "call_action_list,name=cds,if=fight_remains<30|time_to_die>15&(!variable.holding_crash|spell_targets.mind_sear>2)" );
  aoe->add_action( "dark_void,if=raid_event.adds.in>10" );
  aoe->add_action( "mindbender,if=(dot.shadow_word_pain.ticking&variable.vts_applied|action.shadow_crash.in_flight)&(fight_remains<30|time_to_die>15)" );
  aoe->add_action( "mind_blast,if=cooldown.mind_blast.full_recharge_time<=gcd.max+cast_time&talent.mind_devourer.rank=2&spell_targets.mind_sear>=3&!buff.mind_devourer.up&spell_targets.mind_sear<=7", "actions.aoe+=/run_action_list,name=aoe_pl_ire,if=talent.psychic_link.rank=2&talent.insidious_ire.rank=2  Use Mind Blast when capped on charges and talented into Mind Devourer to fish for the buff. Only use when facing 3-7 targets." );
  aoe->add_action( "mind_blast,if=(cooldown.mind_blast.full_recharge_time<=gcd.max+cast_time|pet.fiend.remains<=cast_time+gcd.max)&pet.fiend.active&talent.inescapable_torment&pet.fiend.remains>cast_time&spell_targets.mind_sear<=7&!buff.mind_devourer.up" );
  aoe->add_action( "void_bolt,if=insanity<=85" );
  aoe->add_action( "mind_sear,target_if=max:spell_targets.mind_sear,if=buff.mind_devourer.up&spell_targets.mind_sear>1|spell_targets.mind_sear>variable.mind_sear_cutoff&(insanity>=75|((!set_bonus.tier29_4pc&!set_bonus.tier29_2pc)|!buff.dark_reveries.up)|(!set_bonus.tier29_2pc|buff.gathering_shadows.stack=3))&!variable.pool_for_cds,early_chain_if=ticks>=2&!buff.mind_devourer_ms_active.up,interrupt_immediate=1,interrupt_if=ticks>=2&!buff.mind_devourer_ms_active.up", "Use Mind Sear on 3+ targets and either you have at least 75 insanity, 4pc buff is inactive, or 2pc buff is at 3 stacks, or mind devourer is up on 2+ targets. If Mind Devourer is up do not cancel mind sear." );
  aoe->add_action( "call_action_list,name=pl_torrent,if=talent.void_torrent&talent.psychic_link&cooldown.void_torrent.remains<=3&(!variable.holding_crash|raid_event.adds.count%(active_dot.vampiric_touch+raid_event.adds.count)<1.5)&((insanity>=50|dot.devouring_plague.ticking|buff.dark_reveries.up)|buff.voidform.up|buff.dark_ascension.up)" );
  aoe->add_action( "devouring_plague,if=(refreshable&!variable.pool_for_cds|insanity>75&(!buff.mind_flay_insanity.up&talent.mind_flay_insanity|!talent.psychic_link))&variable.dp_cutoff" );
  aoe->add_action( "vampiric_touch,target_if=refreshable&target.time_to_die>=18&(dot.vampiric_touch.ticking|!variable.vts_applied),if=variable.max_vts>0&(cooldown.shadow_crash.remains>=dot.vampiric_touch.remains|variable.holding_crash)&!action.shadow_crash.in_flight" );
  aoe->add_action( "shadow_word_pain,if=refreshable&target.time_to_die>=18&!talent.misery.enabled" );
  aoe->add_action( "shadow_word_death,target_if=min:target.time_to_die,if=target.time_to_die<=5&insanity<=80&talent.death_and_madness", "TODO: Check Yshaarj Gains for pressing this during Inescapable Torment." );
  aoe->add_action( "damnation,target_if=dot.vampiric_touch.refreshable&variable.is_vt_possible|dot.shadow_word_pain.refreshable" );
  aoe->add_action( "mind_blast,if=variable.vts_applied&(!buff.mind_devourer.up|cooldown.void_eruption.up&talent.void_eruption)" );
  aoe->add_action( "mindgames,if=spell_targets.mind_sear<5&dot.devouring_plague.ticking|talent.psychic_link" );
  aoe->add_action( "void_torrent,if=insanity<=35&!talent.psychic_link,target_if=variable.dots_up" );
  aoe->add_action( "mind_flay,if=buff.mind_flay_insanity.up&buff.surge_of_darkness.remains>=5&talent.idol_of_cthun&buff.surge_of_darkness.stack<=2,interrupt_if=ticks>=2,interrupt_immediate=1", "High priority action for Mind Flay: Insanity to fish for Idol of C'Thun procs" );
  aoe->add_action( "call_action_list,name=filler" );

  pl_torrent->add_action( "void_bolt" );
  pl_torrent->add_action( "vampiric_touch,if=remains<=6&cooldown.void_torrent.remains<gcd*2" );
  pl_torrent->add_action( "devouring_plague,if=remains<=4&cooldown.void_torrent.remains<gcd*2&!buff.voidform.up&(!talent.mind_sear|spell_targets.mind_sear<=4|!talent.surge_of_darkness&cooldown.mind_blast.full_recharge_time>=3)&!buff.mind_devourer.up", "Use Devouring Plague before Void Torrent cast if Voidform is not active and Mind Devourer is not active and fighting 4 or less targets or less not talented into Mind Sear" );
  pl_torrent->add_action( "mind_sear,if=!variable.dp_cutoff|buff.mind_devourer.up" );
  pl_torrent->add_action( "mind_blast,if=!talent.mindgames|cooldown.mindgames.remains>=3&!prev_gcd.1.mind_blast" );
  pl_torrent->add_action( "void_torrent,if=dot.vampiric_touch.ticking&dot.shadow_word_pain.ticking|buff.voidform.up" );
  pl_torrent->add_action( "mindgames,if=dot.vampiric_touch.ticking&dot.shadow_word_pain.ticking&dot.devouring_plague.ticking|buff.voidform.up" );

  aoe_pl_ire->add_action( "", "TODO: Play the game" );

  filler->add_action( "mind_flay,if=buff.mind_flay_insanity.up&dot.vampiric_touch.ticking&dot.shadow_word_pain.ticking&(!buff.surge_of_darkness.up|talent.screams_of_the_void)" );
  filler->add_action( "vampiric_touch,target_if=min:remains,if=buff.unfurling_darkness.up" );
  filler->add_action( "mind_spike,if=buff.surge_of_darkness.up" );
  filler->add_action( "lights_judgment,if=!raid_event.adds.exists|raid_event.adds.in>75|spell_targets.lights_judgment>1" );
  filler->add_action( "halo,if=raid_event.adds.in>20", "Save up to 20s if adds are coming soon." );
  filler->add_action( "shadow_word_death,target_if=min:target.time_to_die,if=target.health.pct<20&(spell_targets.mind_sear<4|talent.inescapable_torment.rank=2&pet.fiend.active)" );
  filler->add_action( "divine_star,if=raid_event.adds.in>10", "Save up to 10s if adds are coming soon." );
  filler->add_action( "mind_spike,if=(!talent.mental_decay|dot.vampiric_touch.remains>=(cooldown.shadow_crash.remains+action.shadow_crash.travel_time))&!talent.idol_of_cthun" );
  filler->add_action( "mind_flay,chain=1,interrupt_immediate=1,interrupt_if=ticks>=2" );
  filler->add_action( "shadow_crash,if=raid_event.adds.in>30", "Use Shadow Crash while moving as a low-priority action when adds will not come in 30 seconds." );
  filler->add_action( "shadow_word_death,target_if=target.health.pct<20", "Use Shadow Word: Death while moving as a low-priority action in execute" );
  filler->add_action( "divine_star", "Use Divine Star while moving as a low-priority action" );
  filler->add_action( "shadow_word_death", "Use Shadow Word: Death while moving as a low-priority action" );
  filler->add_action( "shadow_word_pain,target_if=min:remains", "Use Shadow Word: Pain while moving as a low-priority action" );

  trinkets->add_action( "use_item,name=voidmenders_shadowgem,if=buff.power_infusion.up|fight_remains<20" );
  trinkets->add_action( "use_item,name=darkmoon_deck_box_inferno" );
  trinkets->add_action( "use_item,name=darkmoon_deck_box_rime" );
  trinkets->add_action( "use_item,name=darkmoon_deck_box_dance" );
  trinkets->add_action( "use_items,if=buff.voidform.up|buff.power_infusion.up|buff.dark_ascension.up|(cooldown.void_eruption.remains>10&trinket.cooldown.duration<=60)|fight_remains<20" );
  trinkets->add_action( "use_item,name=desperate_invokers_codex,if=fight_remains<20|!talent.ancient_madness|(cooldown.dark_ascension.remains>10&talent.dark_ascension)|(cooldown.void_eruption.remains>10&talent.void_eruption)|(!talent.void_eruption&!talent.dark_ascension)", "Sync with cooldowns for Ancient Madness or use when the fight will end soon or at full stacks" );
}
//shadow_apl_end
//discipline_apl_start
void discipline( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* variables = p->get_action_priority_list( "variables" );
  action_priority_list_t* main = p->get_action_priority_list( "main" );
  action_priority_list_t* scov_prep = p->get_action_priority_list( "scov_prep" );
  action_priority_list_t* short_scov = p->get_action_priority_list( "short_scov" );
  action_priority_list_t* long_scov = p->get_action_priority_list( "long_scov" );
  action_priority_list_t* cooldowns = p->get_action_priority_list( "cooldowns" );

  precombat->add_action( "flask", "otion=elemental_potion_of_ultimate_power_ ood=fated_fortune_cooki lask=phial_of_tepid_versatility_ ugmentation=draconic_augment_run emporary_enchant=main_hand:howling_rune_" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  variables->add_action( "variable,name=te_none,op=set,value=!buff.twilight_equilibrium_holy_amp.up&!buff.twilight_equilibrium_shadow_amp.up", "----------------------------------------------------------------------------  GENERAL VARIABLES  ----------------------------------------------------------------------------  No Twilight Equilibrium active" );
  variables->add_action( "variable,name=te_shadow,op=set,value=buff.twilight_equilibrium_shadow_amp.up|variable.te_none", "Twilight Equilibrium is buffing shadow damage" );
  variables->add_action( "variable,name=te_holy,op=set,value=buff.twilight_equilibrium_holy_amp.up|variable.te_none", "Twilight Equilibrium is buffing holy damage" );
  variables->add_action( "variable,name=long_scov,op=set,value=talent.shadow_covenant&talent.embrace_shadow", "Long Shadow covenant is enabled" );
  variables->add_action( "variable,name=short_scov,op=set,value=talent.shadow_covenant&!talent.embrace_shadow", "Short shadow covenant is enabled" );
  variables->add_action( "variable,name=hd_stacks_required,op=set,value=0", "Harsh discipline has been stacked and is ready for shadow covenant.  Zero stacks for no shadow covenant  Full stacks for short results in one HD penance (4 total)  Full stacks + 1 extra results in two HD penances (5 total)" );
  variables->add_action( "variable,name=hd_stacks_required,op=setif,condition=variable.short_scov,value=4,value_else=5" );
  variables->add_action( "variable,name=hd_stacks_current,op=set,value=buff.harsh_discipline.stack+(buff.harsh_discipline.max_stack*buff.harsh_discipline_ready.stack)" );
  variables->add_action( "variable,name=harsh_discipline_ready,op=set,value=!talent.harsh_discipline|(variable.hd_stacks_current>=variable.hd_stacks_required)" );
  variables->add_action( "variable,name=can_enter_scov,op=set,value=(cooldown.shadow_covenant.up&variable.harsh_discipline_ready)|buff.shadow_covenant.up", "Ready to start shadow covenant phase" );
  variables->add_action( "variable,name=shadow_spells_duration_max,op=set,value=0", "----------------------------------------------------------------------------  TWILIGHT EQUILIBRIUM VARIABLES  ----------------------------------------------------------------------------  Count how much execute time is available for all of the talented shadow spells. This is used to help determine if we have time to weave in Twilight Equilibrium proccing holy spells." );
  variables->add_action( "variable,name=shadow_spells_duration_max,op=add,value=action.schism.execute_time,if=talent.schism" );
  variables->add_action( "variable,name=shadow_spells_duration_max,op=add,value=2", "penance" );
  variables->add_action( "variable,name=shadow_spells_duration_max,op=add,value=action.mind_blast.execute_time" );
  variables->add_action( "variable,name=shadow_spells_duration_max,op=add,value=action.mind_blast.execute_time,if=talent.dark_indulgence" );
  variables->add_action( "variable,name=shadow_spells_duration_max,op=add,value=action.shadow_word_death.execute_time,if=talent.shadow_word_death" );
  variables->add_action( "variable,name=shadow_spells_duration_max,op=add,value=action.shadow_word_death.execute_time,if=talent.shadow_word_death&talent.death_and_madness&target.health.pct<20" );
  variables->add_action( "variable,name=shadow_spells_duration_max,op=add,value=action.mindgames.execute_time,if=talent.mindgames" );
  variables->add_action( "variable,name=shadow_spells_duration_max,op=add,value=action.divine_star.execute_time,if=talent.divine_star" );
  variables->add_action( "variable,name=shadow_spells_duration_max,op=add,value=action.halo.execute_time,if=talent.halo" );
  variables->add_action( "variable,name=max_te_holy,op=floor,value=(buff.shadow_covenant.duration-variable.shadow_spells_duration_max)%gcd.max", "(scov duration - shadow spells duration) / GCD time = estimate of holy spells we should need to use during the next scov window  Long scov example: (15 - 12) / 1.5 = 2 GCDs to spend on holy spells  Short scov example: (7 - 12) / 1.5 = -3.3 GCDs to spend on holy spells (none)" );
  variables->add_action( "variable,name=remaining_te_holy,op=set,value=0", "Counting variable, the number of non-fractional remaining holy casts available" );
  variables->add_action( "variable,name=remaining_te_holy,op=add,value=variable.max_te_holy" );
  variables->add_action( "variable,name=expected_penance_reduction,op=setif,condition=talent.train_of_thought,value=1.5,value_else=0", "TODO: duration_expected is buggy on penance/dark reprimand, using a static reduction as a stand-in for now" );
  variables->add_action( "variable,name=shadow_spells_duration,op=set,value=0", "Calculate how much cast time worth of shadow spells we have currently available. We use shadow_spells_duration to determine if we have time to weave in any holy spells to proc twilight equilibrium." );
  variables->add_action( "variable,name=shadow_spells_duration,op=add,value=action.schism.execute_time,if=cooldown.schism.up|(cooldown.schism.remains<buff.shadow_covenant.remains)", "Schism" );
  variables->add_action( "variable,name=shadow_spells_duration,op=add,value=2,if=cooldown.penance.up|((cooldown.penance.remains-variable.expected_penance_reduction)<buff.shadow_covenant.remains)", "Penance" );
  variables->add_action( "variable,name=shadow_spells_duration,op=add,value=action.mindgames.execute_time,if=cooldown.mindgames.up|(cooldown.mindgames.remains_expected<buff.shadow_covenant.remains)", "Mindgames" );
  variables->add_action( "variable,name=shadow_spells_duration,op=add,value=action.shadow_word_death.execute_time,if=cooldown.shadow_word_death.up|(cooldown.shadow_word_death.remains<buff.shadow_covenant.remains)", "We always get the first SW:D" );
  variables->add_action( "variable,name=shadow_spells_duration,op=add,value=action.shadow_word_death.execute_time,if=(cooldown.shadow_word_death.up|(cooldown.shadow_word_death.remains<buff.shadow_covenant.remains))&(target.health.pct<20|target.time_to_pct_20<cooldown.shadow_word_death.remains)&talent.death_and_madness", "Second SW:D only when talented and target health in execute range" );
  variables->add_action( "variable,name=shadow_spells_duration,op=add,value=action.mind_blast.charges*action.mind_blast.execute_time,if=action.mind_blast.charges>=1", "Add any whole charges of mind blast" );
  variables->add_action( "variable,name=shadow_spells_duration,op=add,value=action.mind_blast.execute_time,if=((action.mind_blast.charges_fractional>=1&action.mind_blast.charges_fractional<2)&(((1-(action.mind_blast.charges_fractional-1))*action.mind_blast.recharge_time)<buff.shadow_covenant.remains))|((action.mind_blast.charges_fractional<1)&(((1-action.mind_blast.charges_fractional)*action.mind_blast.recharge_time)<buff.shadow_covenant.remains))", "Add any fractional charges of mindblast that will recharge in time" );
  variables->add_action( "variable,name=shadow_spells_duration,op=add,value=action.divine_star.execute_time,if=cooldown.divine_star.up|(cooldown.divine_star.remains<buff.shadow_covenant.remains)", "Divine Star" );
  variables->add_action( "variable,name=shadow_spells_duration,op=add,value=action.halo.execute_time,if=cooldown.halo.up|(cooldown.halo.remains<buff.shadow_covenant.remains)", "Halo" );
  variables->add_action( "variable,name=should_te,op=setif,condition=talent.twilight_equilibrium,value=(buff.shadow_covenant.remains-variable.shadow_spells_duration)>action.smite.execute_time,value_else=0", "Determine if we have enough scov time remaining to weave in a twilight equilibrium activation. If we don't have TE talented, always false. Using smite's execution time to represent a worst-case scenario." );
  variables->add_action( "variable,name=hd_prep_time,op=setif,condition=!variable.harsh_discipline_ready,value=action.smite.execute_time*(variable.hd_stacks_required-variable.hd_stacks_current),value_else=0", "----------------------------------------------------------------------------  VARIABLES FOR SPELLS BETWEEN SCOV WINDOWS  ----------------------------------------------------------------------------  How many GCDs do we need to spend preparing harsh discipline before entering shadow covenant? Using smite's execution time to represent a worst-case scenario." );
  variables->add_action( "variable,name=next_penance_time,op=set,value=cooldown.shadow_covenant.remains+variable.hd_prep_time+action.shadow_covenant.execute_time+action.schism.execute_time", "First casts should always be HD prep->covenant->schism->HD dark reprimand" );
  variables->add_action( "variable,name=next_penance_time,op=add,value=gcd.max,if=variable.remaining_te_holy>0", "If we have enough scov duration, add a holy cast to proc twilight equilibrium" );
  variables->add_action( "variable,name=remaining_te_holy,op=sub,value=1", "Reduce our counting variable" );
  variables->add_action( "variable,name=can_penance,op=set,value=(cooldown.penance.duration-variable.expected_penance_reduction)<variable.next_penance_time" );
  variables->add_action( "variable,name=next_swd_time,op=set,value=variable.next_penance_time+2", "Shadow Word: Death  SW:D is better than other spells if we are in execute phase." );
  variables->add_action( "variable,name=next_swd_time,op=add,value=gcd.max,if=variable.remaining_te_holy>0", "If we have enough scov duration, add a holy cast to proc twilight equilibrium" );
  variables->add_action( "variable,name=remaining_te_holy,op=sub,value=1", "Reduce our counting variable" );
  variables->add_action( "variable,name=next_swd_time,op=add,value=action.mindgames.execute_time,if=((talent.mindgames&talent.shattered_perceptions)|(talent.mindgames&!talent.expiation))&target.health.pct>=20", "indgame  ette ha W:   ren'  xecut has n  av hattere erception  on' av xpiatio" );
  variables->add_action( "variable,name=next_swd_time,op=add,value=action.mind_blast.execute_time,if=target.health.pct>=20", "Mind blast is better than SW:D if we aren't in execute" );
  variables->add_action( "variable,name=next_swd_time,op=add,value=action.mind_blast.execute_time,if=target.health.pct>=20&talent.dark_indulgence", "Second mindblast when talented" );
  variables->add_action( "variable,name=next_swd_time,op=add,value=action.divine_star.execute_time,if=talent.divine_star&target.health.pct>=20&!talent.expiation", "Divine Star and halo are better than SW:D if we aren't in execute and don't have expiation" );
  variables->add_action( "variable,name=next_swd_time,op=add,value=action.halo.execute_time,if=talent.halo&target.health.pct>=20&!talent.expiation" );
  variables->add_action( "variable,name=can_swd,op=set,value=cooldown.shadow_word_death.duration_expected<variable.next_swd_time" );
  variables->add_action( "variable,name=next_mind_blast_time,op=set,value=variable.next_penance_time+2", "Mindblast  Mindblast is better than other spells if we aren't in execute phase" );
  variables->add_action( "variable,name=next_mind_blast_time,op=add,value=gcd.max,if=variable.remaining_te_holy>0", "If we have enough scov duration, add a holy cast to proc twilight equilibrium" );
  variables->add_action( "variable,name=remaining_te_holy,op=sub,value=1", "Reduce our counting variable" );
  variables->add_action( "variable,name=next_mind_blast_time,op=add,value=action.shadow_word_death.execute_time,if=target.health.pct<20", "Add SW:D if we're in execute phase" );
  variables->add_action( "variable,name=next_mind_blast_time,op=add,value=action.shadow_word_death.execute_time,if=talent.death_and_madness&target.health.pct<20", "Add a second SW:D if we have death and madness" );
  variables->add_action( "variable,name=next_mind_blast_time,op=add,value=action.mindgames.execute_time,if=talent.mindgames&!talent.expiation", "Add mindgames if we have it talented and don't have expiation" );
  variables->add_action( "variable,name=can_mind_blast,op=setif,condition=action.mind_blast.charges_fractional>=1,value=((action.mind_blast.max_charges-(action.mind_blast.charges_fractional-1))*action.mind_blast.recharge_time)<variable.next_mind_blast_time,value_else=0", "TODO: This is a little bit simplistic, requiring both charges of mindblast to be available at the moment the first charge needs to be used. Slight optimization should be possible." );
  variables->add_action( "variable,name=next_mindgames_time,op=set,value=variable.next_penance_time+2", "Mindgames  Mindgames is a better option than other spells if we aren't in execute and don't have expiation" );
  variables->add_action( "variable,name=next_mindgames_time,op=add,value=gcd.max,if=variable.remaining_te_holy>0", "If we have enough scov duration, add a holy cast to proc twilight equilibrium" );
  variables->add_action( "variable,name=remaining_te_holy,op=sub,value=1", "Reduce our counting variable" );
  variables->add_action( "variable,name=next_mindgames_time,op=add,value=action.shadow_word_death.execute_time,if=target.health.pct<20|(talent.expiation&!talent.shattered_perceptions)", "SW:D is better than mindgames in execute phase, or outside of execute phase if we don't have shattered perceptions but do have expiation" );
  variables->add_action( "variable,name=next_mindgames_time,op=add,value=action.shadow_word_death.execute_time,if=target.health.pct<20&talent.death_and_madness", "Add a second SW:D if we have death and madness" );
  variables->add_action( "variable,name=next_mindgames_time,op=add,value=action.mind_blast.execute_time,if=talent.expiation", "Mind blast is better than mindgames if we have expiation" );
  variables->add_action( "variable,name=next_mindgames_time,op=add,value=action.mind_blast.execute_time,if=talent.expiation&talent.dark_indulgence", "Second mindblast when talented" );
  variables->add_action( "variable,name=can_mindgames,op=set,value=cooldown.mindgames.duration_expected<variable.next_mindgames_time" );
  variables->add_action( "variable,name=next_divine_star_time,op=set,value=variable.next_penance_time+2", "Divine Star" );
  variables->add_action( "variable,name=next_divine_star_time,op=add,value=gcd.max,if=variable.remaining_te_holy>0", "If we have enough scov duration, add a holy cast to proc twilight equilibrium" );
  variables->add_action( "variable,name=remaining_te_holy,op=sub,value=1", "Reduce our counting variable" );
  variables->add_action( "variable,name=next_divine_star_time,op=add,value=action.shadow_word_death.execute_time,if=target.health.pct<20|talent.expiation", "SW:D is better unless outside of execute phase when expiation is untalented" );
  variables->add_action( "variable,name=next_divine_star_time,op=add,value=action.shadow_word_death.execute_time,if=target.health.pct<20&talent.death_and_madness", "Add a second SW:D if we have death and madness" );
  variables->add_action( "variable,name=next_divine_star_time,op=add,value=action.mind_blast.execute_time,if=talent.expiation", "Mind blast is always better" );
  variables->add_action( "variable,name=next_divine_star_time,op=add,value=action.mind_blast.execute_time,if=talent.dark_indulgence", "Second mindblast when talented" );
  variables->add_action( "variable,name=next_divine_star_time,op=add,value=action.mindgames.execute_time,if=talent.mindgames", "Mindgames is always better" );
  variables->add_action( "variable,name=can_divine_star,op=set,value=cooldown.divine_star.duration<variable.next_divine_star_time" );

  default_->add_action( "run_action_list,name=main", "----------------------------------------------------------------------------  RUN ACTIONS  ----------------------------------------------------------------------------" );

  main->add_action( "call_action_list,name=variables", "START MAIN" );
  main->add_action( "call_action_list,name=cooldowns" );
  main->add_action( "run_action_list,name=scov_prep,if=cooldown.shadow_covenant.up&(!variable.harsh_discipline_ready|(variable.short_scov&((talent.purge_the_wicked&dot.purge_the_wicked.remains<20)|(!talent.purge_the_wicked&dot.shadow_word_pain.remains<20))))" );
  main->add_action( "run_action_list,name=short_scov,if=variable.short_scov&variable.can_enter_scov" );
  main->add_action( "run_action_list,name=long_scov,if=variable.long_scov&variable.can_enter_scov" );
  main->add_action( "purge_the_wicked,if=talent.purge_the_wicked&(!ticking|(refreshable&(!talent.painful_punishment|(talent.painful_punishment&(dot.purge_the_wicked.remains<(cooldown.penance.remains-variable.expected_penance_reduction))))))" );
  main->add_action( "shadow_word_pain,if=!talent.purge_the_wicked&(!ticking|(refreshable&(!talent.painful_punishment|(talent.painful_punishment&(dot.shadow_word_pain.remains<(cooldown.penance.remains-variable.expected_penance_reduction))))))" );
  main->add_action( "schism,if=!talent.shadow_covenant" );
  main->add_action( "shadow_word_death,if=(!talent.shadow_covenant|variable.can_swd)&target.health.pct<20" );
  main->add_action( "penance,if=(!talent.shadow_covenant|variable.can_penance)&buff.harsh_discipline_ready.up" );
  main->add_action( "lights_wrath,if=talent.wrath_unleashed", "For DPS, Lights wrath should always be used outside of scov when you take wrath unleashed to maximize the number of smites it buffs. In a real raid setting, you may want to cast it at the beginning of a scov cycle for additional healing." );
  main->add_action( "mind_blast,if=!talent.shadow_covenant|variable.can_mind_blast" );
  main->add_action( "mindgames,if=(!talent.shadow_covenant|variable.can_mindgames)&talent.shattered_perceptions" );
  main->add_action( "shadow_word_death,if=(!talent.shadow_covenant|variable.can_swd)&talent.expiation&(target.time_to_pct_20>(gcd.max*2))" );
  main->add_action( "mindgames,if=(!talent.shadow_covenant|variable.can_mindgames)&!talent.shattered_perceptions" );
  main->add_action( "halo" );
  main->add_action( "divine_star,if=(!talent.shadow_covenant|variable.can_divine_star)" );
  main->add_action( "power_word_solace" );
  main->add_action( "shadow_word_death,if=(!talent.shadow_covenant|variable.can_swd)&(target.time_to_pct_20>(gcd.max*2))" );
  main->add_action( "smite" );

  scov_prep->add_action( "purge_the_wicked,if=!ticking", "Prepare to enter shadow covenant" );
  scov_prep->add_action( "mind_blast,if=!variable.harsh_discipline_ready&variable.can_mind_blast" );
  scov_prep->add_action( "power_word_solace,if=!variable.harsh_discipline_ready" );
  scov_prep->add_action( "smite,if=!variable.harsh_discipline_ready" );
  scov_prep->add_action( "purge_the_wicked" );

  short_scov->add_action( "shadow_covenant", "Short Shadow Covenant  We want to use entirely shadow spells to optimize our time with the buff" );
  short_scov->add_action( "schism" );
  short_scov->add_action( "shadow_word_death,if=target.health.pct<20&talent.expiation" );
  short_scov->add_action( "penance" );
  short_scov->add_action( "shadow_word_death,if=target.health.pct<20" );
  short_scov->add_action( "mind_blast,if=talent.expiation" );
  short_scov->add_action( "mindgames,if=talent.shattered_perceptions" );
  short_scov->add_action( "shadow_word_death,if=talent.expiation" );
  short_scov->add_action( "mindgames" );
  short_scov->add_action( "halo" );
  short_scov->add_action( "mind_blast" );
  short_scov->add_action( "divine_star" );
  short_scov->add_action( "shadow_word_death" );
  short_scov->add_action( "purge_the_wicked,if=(!ticking|refreshable)", "just in case we run out of shadow spells" );
  short_scov->add_action( "lights_wrath" );
  short_scov->add_action( "power_word_solace" );
  short_scov->add_action( "smite" );

  long_scov->add_action( "shadow_covenant", "Long Shadow Covenant" );
  long_scov->add_action( "schism" );
  long_scov->add_action( "shadow_word_death,if=target.health.pct<20&talent.expiation&(!variable.should_te|(variable.should_te&variable.te_shadow))" );
  long_scov->add_action( "penance,if=(!variable.should_te|(variable.should_te&variable.te_shadow))" );
  long_scov->add_action( "shadow_word_death,if=target.health.pct<20&(!variable.should_te|(variable.should_te&variable.te_shadow))" );
  long_scov->add_action( "mind_blast,if=talent.expiation&(!variable.should_te|(variable.should_te&variable.te_shadow))" );
  long_scov->add_action( "mindgames,if=talent.shattered_perceptions&(!variable.should_te|(variable.should_te&variable.te_shadow))" );
  long_scov->add_action( "shadow_word_death,if=talent.expiation&(!variable.should_te|(variable.should_te&variable.te_shadow))" );
  long_scov->add_action( "mindgames,if=(!variable.should_te|(variable.should_te&variable.te_shadow))" );
  long_scov->add_action( "mind_blast,if=(!variable.should_te|(variable.should_te&variable.te_shadow))" );
  long_scov->add_action( "halo,if=(!variable.should_te|(variable.should_te&variable.te_shadow))" );
  long_scov->add_action( "divine_star,if=(!variable.should_te|(variable.should_te&variable.te_shadow))" );
  long_scov->add_action( "shadow_word_death,if=(!variable.should_te|(variable.should_te&variable.te_shadow))" );
  long_scov->add_action( "purge_the_wicked,if=(!variable.should_te|(variable.should_te&variable.te_holy))&(!ticking|refreshable)" );
  long_scov->add_action( "lights_wrath,if=(!variable.should_te|(variable.should_te&variable.te_holy))" );
  long_scov->add_action( "power_word_solace,if=(!variable.should_te|(variable.should_te&variable.te_holy))" );
  long_scov->add_action( "smite,if=(!variable.should_te|(variable.should_te&variable.te_holy))" );
  long_scov->add_action( "penance", "just in case we run out of shadow spells with the above conditions" );
  long_scov->add_action( "mindgames" );
  long_scov->add_action( "mind_blast" );
  long_scov->add_action( "shadow_word_death" );
  long_scov->add_action( "divine_star" );
  long_scov->add_action( "halo" );
  long_scov->add_action( "power_word_solace" );
  long_scov->add_action( "smite" );

  cooldowns->add_action( "shadowfiend,if=!talent.mindbender.enabled&!buff.shadow_covenant.up", "Cooldowns  Don't use pets during shadow covenant windows, wasting GCDs" );
  cooldowns->add_action( "mindbender,if=talent.mindbender.enabled&!buff.shadow_covenant.up" );
  cooldowns->add_action( "power_infusion,if=!talent.shadow_covenant.enabled|(talent.shadow_covenant.enabled&(cooldown.shadow_covenant.up|buff.shadow_covenant.up))", "hold PI to use with shadow covenant if we have it" );
  cooldowns->add_action( "potion,if=buff.power_infusion.up", "sync potion with PI" );
  cooldowns->add_action( "use_items,if=buff.power_infusion.up|cooldown.power_infusion.remains>=cooldown", "sync trinkets with PI" );
}
//discipline_apl_end
//holy_apl_start
void holy( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* main_variables = p->get_action_priority_list( "main_variables" );
  action_priority_list_t* main = p->get_action_priority_list( "main" );
  action_priority_list_t* divine_favor_chastise_active = p->get_action_priority_list( "divine_favor_chastise_active" );
  action_priority_list_t* divine_favor_filler = p->get_action_priority_list( "divine_favor_filler" );
  action_priority_list_t* divine_image = p->get_action_priority_list( "divine_image" );
  action_priority_list_t* generic = p->get_action_priority_list( "generic" );
  action_priority_list_t* cooldowns = p->get_action_priority_list( "cooldowns" );

  precombat->add_action( "flask", "otion=elemental_potion_of_ultimate_power_ ood=fated_fortune_cooki lask=phial_of_tepid_versatility_ ugmentation=draconic_augment_run emporary_enchant=main_hand:howling_rune_" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  main_variables->add_action( "variable,name=chastise_cdr,op=set,value=((cooldown.divine_word.remains%action.smite.execute_time)*4)", "VARIABLES" );

  default_->add_action( "run_action_list,name=main", "RUN ACTIONS" );

  main->add_action( "call_action_list,name=main_variables", "MAIN" );
  main->add_action( "call_action_list,name=cooldowns" );
  main->add_action( "holy_fire,if=(talent.empyreal_blaze|talent.harmonious_apparatus)|(!ticking|refreshable)", "Always use HF if we have empyreal blaze or harmonious apparatus  Otherwise, only use it if not ticking or refreshable" );
  main->add_action( "shadow_word_pain,if=(refreshable|!ticking)&buff.apotheosis.down", "Don't cast SW:P during apotheosis" );
  main->add_action( "divine_word,if=cooldown.holy_word_chastise.up&(!talent.empyreal_blaze|cooldown.empyreal_blaze.up)", "Divine Word only if we can sync with Chastise.  If we have Empyreal Blaze, sync with that as well." );
  main->add_action( "holy_word_chastise,if=buff.divine_word.up", "Holy word chastise early to trigger divine favor: chastise" );
  main->add_action( "run_action_list,name=divine_favor_chastise_active,if=buff.divine_favor_chastise.up", "Enter Divine Favor rotation with divine favor: chastise buff up" );
  main->add_action( "run_action_list,name=divine_favor_filler,if=talent.divine_word&talent.holy_word_chastise&buff.divine_favor_chastise.down", "Run divine favor fillers rotation with buff down" );
  main->add_action( "run_action_list,name=divine_image,if=talent.divine_image", "Run divine image rotation with divine image" );
  main->add_action( "run_action_list,name=generic", "Otherwise generic rotation" );

  divine_favor_chastise_active->add_action( "holy_word_chastise", "Divine Favor (Active)" );
  divine_favor_chastise_active->add_action( "empyreal_blaze" );
  divine_favor_chastise_active->add_action( "apotheosis,if=cooldown.holy_word_chastise.remains>10" );
  divine_favor_chastise_active->add_action( "shadow_word_death,if=target.health.pct<50" );
  divine_favor_chastise_active->add_action( "mindgames" );
  divine_favor_chastise_active->add_action( "holy_nova,if=talent.rhapsody&buff.rhapsody.stack=buff.rhapsody.max_stack&spell_targets.holy_nova>=3" );
  divine_favor_chastise_active->add_action( "divine_star" );
  divine_favor_chastise_active->add_action( "halo" );
  divine_favor_chastise_active->add_action( "smite" );

  divine_favor_filler->add_action( "holy_word_chastise,if=(cooldown.holy_word_chastise.duration-variable.chastise_cdr)<cooldown.divine_word.remains", "Divine Favor (Filler)" );
  divine_favor_filler->add_action( "shadow_word_death,if=target.health.pct<50" );
  divine_favor_filler->add_action( "mindgames" );
  divine_favor_filler->add_action( "holy_nova,if=talent.rhapsody&buff.rhapsody.stack=buff.rhapsody.max_stack&spell_targets>=3" );
  divine_favor_filler->add_action( "divine_star" );
  divine_favor_filler->add_action( "halo" );
  divine_favor_filler->add_action( "smite" );

  divine_image->add_action( "holy_word_sanctify", "Divine Image" );
  divine_image->add_action( "holy_word_serenity" );
  divine_image->add_action( "holy_word_chastise" );
  divine_image->add_action( "empyreal_blaze" );
  divine_image->add_action( "apotheosis,if=cooldown.holy_word_chastise.remains>10" );
  divine_image->add_action( "shadow_word_death,if=target.health.pct<50&!(buff.apotheosis.up|buff.answered_prayers.up)" );
  divine_image->add_action( "mindgames,if=!(buff.apotheosis.up|buff.answered_prayers.up)" );
  divine_image->add_action( "holy_nova,if=talent.rhapsody&buff.rhapsody.stack=buff.rhapsody.max_stack&spell_targets>=3&!(buff.apotheosis.up|buff.answered_prayers.up)" );
  divine_image->add_action( "divine_star,if=!(buff.apotheosis.up|buff.answered_prayers.up)" );
  divine_image->add_action( "halo,if=!(buff.apotheosis.up|buff.answered_prayers.up)" );
  divine_image->add_action( "smite" );

  generic->add_action( "holy_word_chastise", "Generic" );
  generic->add_action( "empyreal_blaze" );
  generic->add_action( "apotheosis,if=cooldown.holy_word_chastise.remains>10" );
  generic->add_action( "shadow_word_death,if=target.health.pct<50&!(buff.apotheosis.up|buff.answered_prayers.up)" );
  generic->add_action( "mindgames,if=!(buff.apotheosis.up|buff.answered_prayers.up)" );
  generic->add_action( "holy_nova,if=talent.rhapsody&buff.rhapsody.stack=buff.rhapsody.max_stack&spell_targets>=3&!(buff.apotheosis.up|buff.answered_prayers.up)" );
  generic->add_action( "divine_star,if=!(buff.apotheosis.up|buff.answered_prayers.up)" );
  generic->add_action( "halo,if=!(buff.apotheosis.up|buff.answered_prayers.up)" );
  generic->add_action( "smite" );

  cooldowns->add_action( "power_infusion,if=!talent.divine_word|(talent.divine_word&buff.divine_favor_chastise.up)", "Cooldowns  Sync PI with divine favor: chastise if we took divine word" );
  cooldowns->add_action( "potion,if=buff.power_infusion.up", "Only potion in sync with power infusion" );
  cooldowns->add_action( "use_items,if=buff.power_infusion.up", "hold trinkets to use with PI" );
  cooldowns->add_action( "shadowfiend" );
}
//holy_apl_end
//nospec_apl_start
void no_spec( player_t* p )
{
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* def       = p->get_action_priority_list( "default" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "smite" );
  def->add_action( "mana_potion,if=mana.pct<=75" );
  def->add_action( "shadowfiend" );
  def->add_action( "berserking" );
  def->add_action( "arcane_torrent,if=mana.pct<=90" );
  def->add_action( "shadow_word_pain,if=remains<tick_time|!ticking" );
  def->add_action( "smite" );
}
//nospec_apl_end
}  // namespace priest_apl
