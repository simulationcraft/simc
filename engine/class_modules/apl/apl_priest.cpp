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
  action_priority_list_t* def     = p->get_action_priority_list( "default" );
  action_priority_list_t* racials = p->get_action_priority_list( "racials" );

  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "smite" );

  // On-Use Items
  def->add_action( "use_items", "Default fallback for usable items: Use on cooldown in order by trinket slot." );

  // Potions
  def->add_action( "potion,if=buff.bloodlust.react|buff.power_infusion.up|target.time_to_die<=40",
                   "Sync potion usage with Bloodlust or Power Infusion." );

  // Racials
  racials->add_action( "arcane_torrent,if=mana.pct<=95" );

  if ( p->race != RACE_BLOOD_ELF )
  {
    for ( const auto& racial_action : p->get_racial_actions() )
    {
      racials->add_action( racial_action );
    }
  }

  def->add_call_action_list( racials );
  def->add_action( "power_infusion",
                   "Use Power Infusion before Shadow Covenant to make sure we don't lock out our CD." );
  def->add_action( "divine_star" );
  def->add_action( "halo" );
  def->add_action( "penance" );
  def->add_action( "power_word_solace" );
  def->add_action( "shadow_covenant" );
  def->add_action( "schism" );
  def->add_action( "mindgames" );
  def->add_action( "mindbender" );
  def->add_action( "purge_the_wicked,if=!ticking" );
  def->add_action( "shadow_word_pain,if=!ticking&!talent.purge_the_wicked.enabled" );
  def->add_action( "shadow_word_death" );
  def->add_action( "mind_blast" );
  def->add_action( "purge_the_wicked,if=refreshable" );
  def->add_action( "shadow_word_pain,if=refreshable&!talent.purge_the_wicked.enabled" );
  def->add_action( "smite,if=spell_targets.holy_nova<3", "Use Smite on up to 2 targets." );
  def->add_action( "holy_nova,if=spell_targets.holy_nova>=3" );
  def->add_action( "shadow_word_pain" );
}
//discipline_apl_end

//holy_apl_start
void holy( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
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

  default_->add_action( "run_action_list,name=main", "RUN ACTIONS" );

  main->add_action( "call_action_list,name=cooldowns", "---------------------------------------------------------------------------  Main Actions  ---------------------------------------------------------------------------" );
  main->add_action( "holy_fire,cycle_targets=1,if=(talent.empyreal_blaze|talent.harmonious_apparatus|!ticking|refreshable)&target.time_to_die>=dot.holy_fire.duration", "Always use HF if we have empyreal blaze (dot extension) or harmonious apparatus (cd reduction) or if it's not currently ticking. Otherwise, only use when refreshable to be sure we get the longest duration possible." );
  main->add_action( "shadow_word_pain,cycle_targets=1,max_cycle_targets=3,if=(refreshable|!ticking)&target.time_to_die>=dot.shadow_word_pain.duration", "Don't cast SW:P during apotheosis. More than 3 targets typically results in a DPS loss." );
  main->add_action( "divine_word,if=cooldown.holy_word_chastise.up&(!talent.empyreal_blaze|cooldown.empyreal_blaze.up)", "Divine Word only if we can sync with Chastise. If we have Empyreal Blaze, sync with that as well." );
  main->add_action( "holy_word_chastise,if=buff.divine_word.up", "Holy word chastise to trigger divine favor: chastise" );
  main->add_action( "run_action_list,name=divine_favor_chastise_active,if=buff.divine_favor_chastise.up", "Enter Divine Favor rotation with divine favor: chastise buff up" );
  main->add_action( "run_action_list,name=divine_favor_filler,if=talent.divine_word&talent.holy_word_chastise&buff.divine_favor_chastise.down", "Run divine favor fillers rotation with buff down" );
  main->add_action( "run_action_list,name=divine_image,if=talent.divine_image", "Run divine image rotation with divine image talented" );
  main->add_action( "run_action_list,name=generic", "Otherwise generic rotation" );

  divine_favor_chastise_active->add_action( "holy_word_chastise", "---------------------------------------------------------------------------  Divine Favor (Active)  ---------------------------------------------------------------------------" );
  divine_favor_chastise_active->add_action( "empyreal_blaze" );
  divine_favor_chastise_active->add_action( "apotheosis,if=cooldown.holy_word_chastise.remains>(gcd.max*3)", "Hold apotheosis if chastise will be up soon so we can get an additional reset" );
  divine_favor_chastise_active->add_action( "holy_nova,if=(spell_targets.holy_nova>=2&buff.rhapsody.stack>=13)|(spell_targets.holy_nova>=3&buff.rhapsody.stack>=5)|(spell_targets.holy_nova>=4&buff.rhapsody.stack>=2)|spell_targets.holy_nova>=5", "There are particular breakpoints combinations of rhapsody and spell targets beyond which holy nova beats everything else we can do" );
  divine_favor_chastise_active->add_action( "mindgames" );
  divine_favor_chastise_active->add_action( "shadow_word_death,if=target.health.pct<20" );
  divine_favor_chastise_active->add_action( "holy_nova,if=buff.rhapsody.stack=buff.rhapsody.max_stack", "Avoid sitting on a full rhapsody stack" );
  divine_favor_chastise_active->add_action( "halo" );
  divine_favor_chastise_active->add_action( "divine_star" );
  divine_favor_chastise_active->add_action( "smite" );

  divine_favor_filler->add_action( "holy_word_chastise,if=cooldown.holy_word_chastise.duration_expected<cooldown.divine_word.remains", "---------------------------------------------------------------------------  Divine Favor (Filler)  ---------------------------------------------------------------------------" );
  divine_favor_filler->add_action( "holy_nova,if=(spell_targets.holy_nova>=2&buff.rhapsody.stack>=13)|(spell_targets.holy_nova>=3&buff.rhapsody.stack>=5)|(spell_targets.holy_nova>=4&buff.rhapsody.stack>=2)|spell_targets.holy_nova>=5", "There are particular breakpoints combinations of rhapsody and spell targets beyond which holy nova beats everything else we can do" );
  divine_favor_filler->add_action( "mindgames" );
  divine_favor_filler->add_action( "shadow_word_death,if=target.health.pct<20" );
  divine_favor_filler->add_action( "holy_nova,if=buff.rhapsody.stack=buff.rhapsody.max_stack", "Avoid sitting on a full rhapsody stack" );
  divine_favor_filler->add_action( "halo" );
  divine_favor_filler->add_action( "divine_star" );
  divine_favor_filler->add_action( "smite" );

  divine_image->add_action( "apotheosis,if=!buff.answered_prayers.up&((cooldown.holy_word_sanctify.remains>(gcd.max*2))&(cooldown.holy_word_serenity.remains>(gcd.max*2))&(cooldown.holy_word_chastise.remains>(gcd.max*2)))", "---------------------------------------------------------------------------  Divine Image  ---------------------------------------------------------------------------  We want to apotheosis when our holy words aren't about to come off of cooldown, and when answered prayer's apotheosis is already active." );
  divine_image->add_action( "holy_word_sanctify,line_cd=9", "line_cd prevents re-casting sanctify and serenity right away, wasting GCDs that could be used to proc divine image damage. In testing, the sweet spot balance between adding more divine images, triggering apotheosis, and casting damage CDs was to let the image from each healing holy word expire before re-casting" );
  divine_image->add_action( "holy_word_serenity,line_cd=9" );
  divine_image->add_action( "holy_word_chastise" );
  divine_image->add_action( "empyreal_blaze" );
  divine_image->add_action( "holy_nova,if=(spell_targets.holy_nova>=2&buff.rhapsody.stack>=13)|(spell_targets.holy_nova>=3&buff.rhapsody.stack>=5)|(spell_targets.holy_nova>=4&buff.rhapsody.stack>=2)|spell_targets.holy_nova>=5", "There are particular breakpoints combinations of rhapsody and spell targets beyond which holy nova beats everything else we can do" );
  divine_image->add_action( "mindgames", "Mindgames also triggers divine images" );
  divine_image->add_action( "shadow_word_death,if=target.health.pct<20", "Shadow Word: Death is only worth casting during execute, otherwise it's worse than a smite" );
  divine_image->add_action( "holy_nova,if=buff.rhapsody.stack=buff.rhapsody.max_stack", "Avoid sitting on a full rhapsody stack" );
  divine_image->add_action( "halo,if=buff.divine_image.stack=0", "Halo doesn't proc image spells" );
  divine_image->add_action( "divine_star,if=buff.divine_image.stack=0", "Divine Star doesn't proc image spells" );
  divine_image->add_action( "smite" );

  generic->add_action( "holy_word_chastise", "---------------------------------------------------------------------------  Generic  ---------------------------------------------------------------------------" );
  generic->add_action( "empyreal_blaze" );
  generic->add_action( "apotheosis,if=cooldown.holy_word_chastise.remains>(gcd.max*3)", "Hold Apotheosis if chastise will be up soon" );
  generic->add_action( "holy_nova,if=(spell_targets.holy_nova>=2&buff.rhapsody.stack>=13)|(spell_targets.holy_nova>=3&buff.rhapsody.stack>=5)|(spell_targets.holy_nova>=4&buff.rhapsody.stack>=2)|spell_targets.holy_nova>=5", "There are particular breakpoints combinations of rhapsody and spell targets beyond which holy nova beats everything else we can do" );
  generic->add_action( "mindgames" );
  generic->add_action( "shadow_word_death,if=target.health.pct<20" );
  generic->add_action( "holy_nova,if=buff.rhapsody.stack=buff.rhapsody.max_stack", "Avoid sitting on a full rhapsody stack" );
  generic->add_action( "halo" );
  generic->add_action( "divine_star" );
  generic->add_action( "smite" );

  cooldowns->add_action( "power_infusion,if=(!talent.divine_word|(talent.divine_word&buff.divine_favor_chastise.up))", "---------------------------------------------------------------------------  Cooldowns  ---------------------------------------------------------------------------  Sync PI with divine favor: chastise if we took divine word" );
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
