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
  return ( p->true_level > 60 ) ? "iced_phial_of_corrupting_rage_3" : "greater_flask_of_endless_fathoms";
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
  action_priority_list_t* filler = p->get_action_priority_list( "filler" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );

  precombat->add_action( "flask", "otion=elemental_potion_of_ultimate_power_ lask=iced_phial_of_corrupting_rage_ ood=fated_fortune_cooki ugmentation=draconic_augment_run emporary_enchant=main_hand:howling_rune_" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "shadowform,if=!buff.shadowform.up" );
  precombat->add_action( "arcane_torrent" );
  precombat->add_action( "shadow_crash,if=raid_event.adds.in>=25&spell_targets.shadow_crash<=8&!fight_style.dungeonslice&(spell_targets.shadow_crash>1|talent.mental_decay)" );
  precombat->add_action( "vampiric_touch,if=!talent.shadow_crash.enabled|raid_event.adds.in<25|spell_targets.shadow_crash>8|spell_targets.shadow_crash=1&!talent.mental_decay|fight_style.dungeonslice" );

  default_->add_action( "variable,name=holding_crash,op=set,value=raid_event.adds.in<15" );
  default_->add_action( "variable,name=pool_for_cds,op=set,value=(cooldown.void_eruption.remains<=gcd.max*3&talent.void_eruption|cooldown.dark_ascension.up&talent.dark_ascension)|talent.void_torrent&talent.psychic_link&cooldown.void_torrent.remains<=4&(!raid_event.adds.exists&spell_targets.vampiric_touch>1|raid_event.adds.in<=5|raid_event.adds.remains>=6&!variable.holding_crash)&!buff.voidform.up" );
  default_->add_action( "run_action_list,name=aoe,if=active_enemies>2|spell_targets.vampiric_touch>3" );
  default_->add_action( "run_action_list,name=main" );

  main_variables->add_action( "variable,name=dots_up,op=set,value=(dot.shadow_word_pain.ticking&dot.vampiric_touch.ticking)|action.shadow_crash.in_flight&talent.whispering_shadows" );
  main_variables->add_action( "variable,name=all_dots_up,op=set,value=dot.shadow_word_pain.ticking&dot.vampiric_touch.ticking&dot.devouring_plague.ticking" );
  main_variables->add_action( "variable,name=pool_for_cds,op=set,value=(cooldown.void_eruption.remains<=gcd.max*3&talent.void_eruption|cooldown.dark_ascension.up&talent.dark_ascension)|talent.void_torrent&talent.psychic_link&cooldown.void_torrent.remains<=4&(!raid_event.adds.exists&spell_targets.vampiric_touch>1|raid_event.adds.in<=5|raid_event.adds.remains>=6&!variable.holding_crash)&!buff.voidform.up" );

  aoe_variables->add_action( "variable,name=max_vts,op=set,default=12,value=spell_targets.vampiric_touch>?12" );
  aoe_variables->add_action( "variable,name=is_vt_possible,op=set,value=0,default=1" );
  aoe_variables->add_action( "variable,name=is_vt_possible,op=set,value=1,target_if=max:(target.time_to_die*dot.vampiric_touch.refreshable),if=target.time_to_die>=18" );
  aoe_variables->add_action( "variable,name=vts_applied,op=set,value=(active_dot.vampiric_touch+8*(action.shadow_crash.in_flight&talent.whispering_shadows))>=variable.max_vts|!variable.is_vt_possible", "TODO: Revamp to fix undesired behaviour with unstacked fights" );
  aoe_variables->add_action( "variable,name=holding_crash,op=set,value=(variable.max_vts-active_dot.vampiric_touch)<4|raid_event.adds.in<10&raid_event.adds.count>(variable.max_vts-active_dot.vampiric_touch),if=variable.holding_crash&talent.whispering_shadows" );
  aoe_variables->add_action( "variable,name=manual_vts_applied,op=set,value=(active_dot.vampiric_touch+8*!variable.holding_crash)>=variable.max_vts|!variable.is_vt_possible" );

  cds->add_action( "potion,if=buff.voidform.up|buff.power_infusion.up|buff.dark_ascension.up&(fight_remains<=cooldown.power_infusion.remains+15)|fight_remains<=30", "TODO: Check VE/DA enter conditions based on dots" );
  cds->add_action( "fireblood,if=buff.power_infusion.up|fight_remains<=8" );
  cds->add_action( "berserking,if=buff.power_infusion.up|fight_remains<=12" );
  cds->add_action( "blood_fury,if=buff.power_infusion.up|fight_remains<=15" );
  cds->add_action( "ancestral_call,if=buff.power_infusion.up|fight_remains<=15" );
  cds->add_action( "power_infusion,if=(buff.voidform.up|buff.dark_ascension.up)", "Sync Power Infusion with Voidform or Dark Ascension" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=(buff.voidform.up|buff.dark_ascension.up)&!buff.power_infusion.up", "Use <a href='https://www.wowhead.com/spell=10060/power-infusion'>Power Infusion</a> while <a href='https://www.wowhead.com/spell=194249/voidform'>Voidform</a> or <a href='https://www.wowhead.com/spell=391109/dark-ascension'>Dark Ascension</a> is active. Chain directly after your own <a href='https://www.wowhead.com/spell=10060/power-infusion'>Power Infusion</a>." );
  cds->add_action( "void_eruption,if=!cooldown.fiend.up&(pet.fiend.active&cooldown.fiend.remains>=4|!talent.mindbender|active_enemies>2&!talent.inescapable_torment.rank)&(cooldown.mind_blast.charges=0|time>15)", "Make sure Mindbender is active before popping Void Eruption and dump charges of Mind Blast before casting" );
  cds->add_action( "dark_ascension,if=pet.fiend.active&cooldown.fiend.remains>=4|!talent.mindbender&!cooldown.fiend.up|active_enemies>2&!talent.inescapable_torment", "Make sure Mindbender is active before popping Dark Ascension unless you have insignificant talent points or too many targets" );
  cds->add_action( "call_action_list,name=trinkets" );
  cds->add_action( "desperate_prayer,if=health.pct<=75", "Use Desperate Prayer to heal up should Shadow Word: Death or other damage bring you below 75%" );

  main->add_action( "call_action_list,name=main_variables" );
  main->add_action( "call_action_list,name=cds,if=fight_remains<30|time_to_die>15&(!variable.holding_crash|active_enemies>2)" );
  main->add_action( "mindbender,if=variable.dots_up&(fight_remains<30|time_to_die>15)", "Use Shadowfiend and Mindbender on cooldown as long as Vampiric Touch and Shadow Word: Pain are active" );
  main->add_action( "mind_blast,target_if=(dot.devouring_plague.ticking&(cooldown.mind_blast.full_recharge_time<=gcd.max+cast_time)|pet.fiend.remains<=cast_time+gcd.max)&pet.fiend.active&talent.inescapable_torment&pet.fiend.remains>cast_time&active_enemies<=7", "High priority Mind Blast action when using Inescapable Torment" );
  main->add_action( "void_bolt,if=variable.dots_up" );
  main->add_action( "devouring_plague,target_if=refreshable|!talent.distorted_reality,if=refreshable&!variable.pool_for_cds|insanity.deficit<=20|buff.voidform.up&cooldown.void_bolt.remains>buff.voidform.remains&cooldown.void_bolt.remains<(buff.voidform.remains+2)", "Use Devouring Plague to maximize uptime. Short circuit if you are capping on Insanity within 20 or to get out an extra Void Bolt by extending Voidform. With Distorted Reality can maintain more than one at a time in multi-target." );
  main->add_action( "shadow_crash,if=!variable.holding_crash&dot.vampiric_touch.refreshable", "Use Shadow Crash as long as you are not holding for adds and Vampiric Touch is within pandemic range" );
  main->add_action( "vampiric_touch,target_if=min:remains,if=refreshable&target.time_to_die>=12&(cooldown.shadow_crash.remains>=dot.vampiric_touch.remains&!action.shadow_crash.in_flight|variable.holding_crash|!talent.whispering_shadows)", "Put out Vampiric Touch on enemies that will live at least 12s and Shadow Crash is not available soon" );
  main->add_action( "shadow_word_death,target_if=target.health.pct<20&(cooldown.fiend.remains>=10|!talent.inescapable_torment)|pet.fiend.active>1&talent.inescapable_torment|buff.deathspeaker.up", "High priority Shadow Word: Death action during execute, while Mindbender is active with Inescapable Torment, or Deathspeaker is active" );
  main->add_action( "mind_spike_insanity,if=variable.dots_up&cooldown.mind_blast.full_recharge_time>=gcd*3&talent.idol_of_cthun&(!cooldown.void_torrent.up|!talent.void_torrent)", "High Priority Mind Spike: Insanity to fish for C'Thun procs when Mind Blast is not capped and Void Torrent is not available and Mindbender is not active" );
  main->add_action( "mind_flay,if=buff.mind_flay_insanity.up&variable.dots_up&(!pet.fiend.active)&cooldown.mind_blast.full_recharge_time>=gcd*3&talent.idol_of_cthun&(!cooldown.void_torrent.up|!talent.void_torrent)", "High Priority Mind Flay: Insanity to fish for C'Thun procs when Mind Blast is not capped and Void Torrent is not available and Mindbender is not active" );
  main->add_action( "mind_blast,if=variable.dots_up&(!buff.mind_devourer.up|cooldown.void_eruption.up&talent.void_eruption)", "Use all charges of Mind Blast if Vampiric Touch and Shadow Word: Pain are active and Mind Devourer is not active or you are prepping Void Eruption" );
  main->add_action( "void_torrent,if=!variable.holding_crash,target_if=variable.all_dots_up&dot.devouring_plague.remains>=2", "Void Torrent if you are not holding Shadow Crash for an add pack coming, prefer the target with the most DoTs active. Only cast if Devouring Plague is on that target and will last at least 2 seconds" );
  main->add_action( "mindgames,target_if=variable.all_dots_up&dot.devouring_plague.remains>=cast_time", "Cast Mindgames if all DoTs will be active by the time the cast finishes" );
  main->add_action( "call_action_list,name=filler" );

  aoe->add_action( "call_action_list,name=aoe_variables" );
  aoe->add_action( "vampiric_touch,target_if=refreshable&target.time_to_die>=18&(dot.vampiric_touch.ticking|!variable.vts_applied),if=variable.max_vts>0&!variable.manual_vts_applied&!action.shadow_crash.in_flight|!talent.whispering_shadows", "High Priority action to put out Vampiric Touch on enemies that will live at least 18 seconds, up to 12 targets manually while prepping AoE" );
  aoe->add_action( "shadow_crash,if=!variable.holding_crash,target_if=dot.vampiric_touch.refreshable|dot.vampiric_touch.remains<=target.time_to_die&!buff.voidform.up&(raid_event.adds.in-dot.vampiric_touch.remains)<15", "Use Shadow Crash to apply Vampiric Touch to as many adds as possible while being efficient with Vampiric Touch refresh windows" );
  aoe->add_action( "call_action_list,name=cds,if=fight_remains<30|time_to_die>15&(!variable.holding_crash|active_enemies>2)" );
  aoe->add_action( "mindbender,if=(dot.shadow_word_pain.ticking&variable.vts_applied|action.shadow_crash.in_flight&talent.whispering_shadows)&(fight_remains<30|time_to_die>15)", "Use Shadowfiend or Mindbender on cooldown if DoTs are active" );
  aoe->add_action( "mind_blast,if=(cooldown.mind_blast.full_recharge_time<=gcd.max+cast_time|pet.fiend.remains<=cast_time+gcd.max)&pet.fiend.active&talent.inescapable_torment&pet.fiend.remains>cast_time&active_enemies<=7&!buff.mind_devourer.up", "Use Mind Blast when capped on charges and talented into Mind Devourer to fish for the buff or if Inescapable Torment is talented with Mindbender active. Only use when facing 3-7 targets." );
  aoe->add_action( "void_bolt" );
  aoe->add_action( "devouring_plague,target_if=refreshable|!talent.distorted_reality,if=refreshable&!variable.pool_for_cds|insanity.deficit<=20|buff.voidform.up&cooldown.void_bolt.remains>buff.voidform.remains&cooldown.void_bolt.remains<(buff.voidform.remains+2)", "Use Devouring Plague to maximize uptime. Short circuit if you are capping on Insanity within 20 or to get out an extra Void Bolt by extending Voidform. With Distorted Reality can maintain more than one at a time in multi-target." );
  aoe->add_action( "vampiric_touch,target_if=refreshable&target.time_to_die>=18&(dot.vampiric_touch.ticking|!variable.vts_applied),if=variable.max_vts>0&(cooldown.shadow_crash.remains>=dot.vampiric_touch.remains|variable.holding_crash)&!action.shadow_crash.in_flight|!talent.whispering_shadows" );
  aoe->add_action( "shadow_word_death,target_if=target.health.pct<20&(cooldown.fiend.remains>=10|!talent.inescapable_torment)|pet.fiend.active>1&talent.inescapable_torment|buff.deathspeaker.up", "High priority Shadow Word: Death action during execute, while Mindbender is active with Inescapable Torment, or Deathspeaker is active" );
  aoe->add_action( "mind_spike_insanity,if=variable.dots_up&cooldown.mind_blast.full_recharge_time>=gcd*3&talent.idol_of_cthun&(!cooldown.void_torrent.up|!talent.void_torrent)", "High Priority Mind Spike: Insanity to fish for C'Thun procs when Mind Blast is not capped and Void Torrent is not available and Mindbender is not active" );
  aoe->add_action( "mind_flay,if=buff.mind_flay_insanity.up&variable.dots_up&(!pet.fiend.active)&cooldown.mind_blast.full_recharge_time>=gcd*3&talent.idol_of_cthun&(!cooldown.void_torrent.up|!talent.void_torrent)", "High Priority Mind Flay: Insanity to fish for C'Thun procs when Mind Blast is not capped and Void Torrent is not available and Mindbender is not active" );
  aoe->add_action( "mind_blast,if=variable.vts_applied&(!buff.mind_devourer.up|cooldown.void_eruption.up&talent.void_eruption)", "# Use all charges of Mind Blast if Vampiric Touch and Shadow Word: Pain are active and Mind Devourer is not active or you are prepping Void Eruption" );
  aoe->add_action( "call_action_list,name=pl_torrent,target_if=talent.void_torrent&talent.psychic_link&cooldown.void_torrent.remains<=3&(!variable.holding_crash|raid_event.adds.count%(active_dot.vampiric_touch+raid_event.adds.count)<1.5)&((insanity>=50|dot.devouring_plague.ticking|buff.dark_reveries.up)|buff.voidform.up|buff.dark_ascension.up)", "Void Torrent action list for AoE" );
  aoe->add_action( "mindgames,if=active_enemies<5&dot.devouring_plague.ticking|talent.psychic_link" );
  aoe->add_action( "void_torrent,if=!talent.psychic_link,target_if=variable.dots_up" );
  aoe->add_action( "mind_flay,if=buff.mind_flay_insanity.up&talent.idol_of_cthun,interrupt_if=ticks>=2,interrupt_immediate=1", "High priority action for Mind Flay: Insanity to fish for Idol of C'Thun procs, cancel as soon as something else is more important and most of the channel has completed" );
  aoe->add_action( "call_action_list,name=filler" );

  pl_torrent->add_action( "void_bolt" );
  pl_torrent->add_action( "vampiric_touch,if=remains<=6&cooldown.void_torrent.remains<gcd*2" );
  pl_torrent->add_action( "devouring_plague,if=remains<=4&cooldown.void_torrent.remains<gcd*2", "Use Devouring Plague before Void Torrent cast" );
  pl_torrent->add_action( "mind_blast,if=!talent.mindgames|cooldown.mindgames.remains>=3&!prev_gcd.1.mind_blast" );
  pl_torrent->add_action( "void_torrent,if=dot.vampiric_touch.ticking&dot.shadow_word_pain.ticking|buff.voidform.up" );
  pl_torrent->add_action( "mindgames,target_if=variable.all_dots_up|buff.voidform.up" );

  filler->add_action( "vampiric_touch,target_if=min:remains,if=buff.unfurling_darkness.up", "Cast Vampiric Touch to consume Unfurling Darkness, prefering the target with the lowest DoT duration active" );
  filler->add_action( "mind_spike_insanity" );
  filler->add_action( "mind_flay,if=buff.mind_flay_insanity.up" );
  filler->add_action( "halo,if=raid_event.adds.in>20", "Save up to 20s if adds are coming soon." );
  filler->add_action( "shadow_word_death,target_if=min:target.time_to_die,if=target.health.pct<20&active_enemies<4|talent.inescapable_torment&pet.fiend.active" );
  filler->add_action( "divine_star,if=raid_event.adds.in>10", "Save up to 10s if adds are coming soon." );
  filler->add_action( "devouring_plague,if=buff.voidform.up&variable.dots_up" );
  filler->add_action( "mind_spike" );
  filler->add_action( "mind_flay,chain=1,interrupt_immediate=1,interrupt_if=ticks>=2" );
  filler->add_action( "shadow_crash,if=raid_event.adds.in>20", "Use Shadow Crash while moving as a low-priority action when adds will not come in 20 seconds." );
  filler->add_action( "shadow_word_death,target_if=target.health.pct<20", "Use Shadow Word: Death while moving as a low-priority action in execute" );
  filler->add_action( "divine_star", "Use Divine Star while moving as a low-priority action" );
  filler->add_action( "shadow_word_death", "Use Shadow Word: Death while moving as a low-priority action" );
  filler->add_action( "shadow_word_pain,target_if=min:remains", "Use Shadow Word: Pain while moving as a low-priority action" );

  trinkets->add_action( "use_item,name=voidmenders_shadowgem,if=buff.power_infusion.up|fight_remains<20" );
  trinkets->add_action( "use_item,name=darkmoon_deck_box_inferno" );
  trinkets->add_action( "use_item,name=darkmoon_deck_box_rime" );
  trinkets->add_action( "use_item,name=darkmoon_deck_box_dance" );
  trinkets->add_action( "use_item,name=erupting_spear_fragment,if=buff.power_infusion.up|raid_event.adds.up|fight_remains<20", "Use Erupting Spear Fragment with cooldowns, adds are currently active, or the fight will end in less than 20 seconds" );
  trinkets->add_action( "use_item,name=beacon_to_the_beyond,if=!raid_event.adds.exists|raid_event.adds.up|spell_targets.beacon_to_the_beyond>=5|fight_remains<20", "Use Beacon to the Beyond on cooldown except to hold for incoming adds or if already facing 5 or more targets" );
  trinkets->add_action( "use_items,if=buff.voidform.up|buff.power_infusion.up|buff.dark_ascension.up|(cooldown.void_eruption.remains>10&trinket.cooldown.duration<=60)|fight_remains<20" );
  trinkets->add_action( "use_item,name=desperate_invokers_codex" );
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
