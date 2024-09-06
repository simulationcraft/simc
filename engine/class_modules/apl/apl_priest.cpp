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
  return ( p->true_level > 70 ) ? "tempered_potion_3" : "elemental_potion_of_ultimate_power_3";
}

std::string flask( const player_t* p )
{
  return ( p->true_level > 70 ) ? "flask_of_alchemical_chaos_3" : "iced_phial_of_corrupting_rage_3";
}

std::string food( const player_t* p )
{
  return ( p->true_level > 70 ) ? "feast_of_the_divine_day" : "fated_fortune_cookie";
}

std::string rune( const player_t* p )
{
  return ( p->true_level > 70 ) ? "crystallized" : "draconic";
}

std::string temporary_enchant( const player_t* p )
{
  return ( p->true_level > 70 ) ? "main_hand:algari_mana_oil_3" : "main_hand:howling_rune_3";
}

//shadow_apl_start
void shadow( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* main = p->get_action_priority_list( "main" );
  action_priority_list_t* aoe_variables = p->get_action_priority_list( "aoe_variables" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* filler = p->get_action_priority_list( "filler" );
  action_priority_list_t* empowered_filler = p->get_action_priority_list( "empowered_filler" );
  action_priority_list_t* heal_for_tof = p->get_action_priority_list( "heal_for_tof" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "shadowform,if=!buff.shadowform.up" );
  precombat->add_action( "variable,name=dr_force_prio,default=0,op=reset" );
  precombat->add_action( "variable,name=me_force_prio,default=1,op=reset" );
  precombat->add_action( "variable,name=max_vts,default=0,op=reset" );
  precombat->add_action( "variable,name=is_vt_possible,default=0,op=reset" );
  precombat->add_action( "variable,name=pooling_mindblasts,default=0,op=reset" );
  precombat->add_action( "arcane_torrent" );
  precombat->add_action( "use_item,name=aberrant_spellforge" );
  precombat->add_action( "shadow_crash,if=raid_event.adds.in>=25&spell_targets.shadow_crash<=8&!fight_style.dungeonslice&(!set_bonus.tier31_4pc|spell_targets.shadow_crash>1)" );
  precombat->add_action( "vampiric_touch,if=!talent.shadow_crash.enabled|raid_event.adds.in<25|spell_targets.shadow_crash>8|fight_style.dungeonslice|set_bonus.tier31_4pc&spell_targets.shadow_crash=1" );

  default_->add_action( "variable,name=holding_crash,op=set,value=raid_event.adds.in<15" );
  default_->add_action( "variable,name=pool_for_cds,op=set,value=(cooldown.void_eruption.remains<=gcd.max*3&talent.void_eruption|cooldown.dark_ascension.up&talent.dark_ascension)|talent.void_torrent&talent.psychic_link&cooldown.void_torrent.remains<=4&(!raid_event.adds.exists&spell_targets.vampiric_touch>1|raid_event.adds.in<=5|raid_event.adds.remains>=6&!variable.holding_crash)&!buff.voidform.up" );
  default_->add_action( "call_action_list,name=aoe,if=active_enemies>2" );
  default_->add_action( "run_action_list,name=main" );

  aoe->add_action( "call_action_list,name=aoe_variables" );
  aoe->add_action( "vampiric_touch,target_if=refreshable&target.time_to_die>=18&(dot.vampiric_touch.ticking|!variable.dots_up),if=(variable.max_vts>0&!variable.manual_vts_applied&!action.shadow_crash.in_flight|!talent.whispering_shadows)&!buff.entropic_rift.up", "High Priority action to put out Vampiric Touch on enemies that will live at least 18 seconds, up to 12 targets manually while prepping AoE" );
  aoe->add_action( "shadow_crash,if=!variable.holding_crash,target_if=dot.vampiric_touch.refreshable|dot.vampiric_touch.remains<=target.time_to_die&!buff.voidform.up&(raid_event.adds.in-dot.vampiric_touch.remains)<15", "Use Shadow Crash to apply Vampiric Touch to as many adds as possible while being efficient with Vampiric Touch refresh windows" );

  main->add_action( "variable,name=dots_up,op=set,value=active_dot.vampiric_touch=active_enemies|action.shadow_crash.in_flight&talent.whispering_shadows,if=active_enemies<3" );
  main->add_action( "variable,name=pooling_mindblasts,op=setif,value=1,value_else=0,condition=(cooldown.void_torrent.remains<?(variable.holding_crash*raid_event.adds.in))<=gcd.max*(1+talent.mind_melt*3),if=talent.void_blast", "Are we pooling mindblasts? Currently only used for Voidweaver." );
  main->add_action( "call_action_list,name=cds,if=fight_remains<30|target.time_to_die>15&(!variable.holding_crash|active_enemies>2)" );
  main->add_action( "mindbender,if=(dot.shadow_word_pain.ticking&variable.dots_up|action.shadow_crash.in_flight&talent.whispering_shadows)&(fight_remains<30|target.time_to_die>15)&(!talent.dark_ascension|cooldown.dark_ascension.remains<gcd.max|fight_remains<15)", "Use Shadowfiend and Mindbender on cooldown as long as Vampiric Touch and Shadow Word: Pain are active and sync with Dark Ascension" );
  main->add_action( "shadow_word_death,target_if=max:(target.health.pct<=20)*100+dot.devouring_plague.ticking,if=priest.force_devour_matter&talent.devour_matter", "High Priority Shadow Word: Death when you are forcing the bonus from Devour Matter" );
  main->add_action( "void_blast,target_if=max:(dot.devouring_plague.remains*1000+target.time_to_die),if=(dot.devouring_plague.remains>=execute_time|buff.entropic_rift.remains<=gcd.max|action.void_torrent.channeling&talent.void_empowerment)&(insanity.deficit>=16|cooldown.mind_blast.full_recharge_time<=gcd.max)&(!talent.mind_devourer|!buff.mind_devourer.up|buff.entropic_rift.remains<=gcd.max)", "Blast more burst :wicked:" );
  main->add_action( "wait,sec=cooldown.mind_blast.recharge_time,if=cooldown.mind_blast.recharge_time<buff.entropic_rift.remains&buff.entropic_rift.up&buff.entropic_rift.remains<gcd.max&cooldown.mind_blast.charges<1" );
  main->add_action( "mind_blast,if=buff.voidform.up&full_recharge_time<=gcd.max&(!talent.insidious_ire|dot.devouring_plague.remains>=execute_time)&(cooldown.void_bolt.remains%gcd.max-cooldown.void_bolt.remains%%gcd.max)*gcd.max<=0.25&(cooldown.void_bolt.remains%gcd.max-cooldown.void_bolt.remains%%gcd.max)>=0.01", "Complicated do not overcap mindblast and use it to protect against void bolt cd desync" );
  main->add_action( "void_bolt,target_if=max:target.time_to_die,if=insanity.deficit>16&cooldown.void_bolt.remains<=0.1", "Use Voidbolt on the enemy with the largest time to die. We do no care about dots because Voidbolt is only accessible inside voidform which guarantees maximum effect" );
  main->add_action( "devouring_plague,target_if=max:target.time_to_die*(dot.devouring_plague.remains<=gcd.max|variable.dr_force_prio|!talent.distorted_reality&variable.me_force_prio),if=active_dot.devouring_plague<=1&dot.devouring_plague.remains<=gcd.max&(!talent.void_eruption|cooldown.void_eruption.remains>=gcd.max*3)|insanity.deficit<=16", "Do not overcap on insanity" );
  main->add_action( "void_torrent,target_if=max:(dot.devouring_plague.remains*1000+target.time_to_die),if=(dot.devouring_plague.ticking|talent.void_eruption&cooldown.void_eruption.up)&talent.entropic_rift&!variable.holding_crash", "Cast Void Torrent at very high priority if Voidweaver" );
  main->add_action( "shadow_word_death,target_if=max:(target.health.pct<=20)*100+dot.devouring_plague.ticking,if=talent.depth_of_shadows&(target.health.pct<=20|buff.deathspeaker.up&talent.deathspeaker)", "Snipe SWDs with Depth of Shadows to spawn pets. Prefer targets with Devouring Plague on them." );
  main->add_action( "mind_blast,target_if=max:dot.devouring_plague.remains,if=(cooldown.mind_blast.full_recharge_time<=gcd.max+execute_time|pet.fiend.remains<=execute_time+gcd.max)&pet.fiend.active&talent.inescapable_torment&pet.fiend.remains>=execute_time&active_enemies<=7&(!buff.mind_devourer.up|!talent.mind_devourer)&dot.devouring_plague.remains>execute_time&!variable.pooling_mindblasts", "Use Mind Blasts if using Inescapable Torment and you are capping charges or it will expire soon. Do not use if pooling Mindblast." );
  main->add_action( "shadow_word_death,target_if=max:dot.devouring_plague.remains,if=pet.fiend.remains<=(gcd.max+1)&pet.fiend.active&talent.inescapable_torment&active_enemies<=7", "High Priority Shadow Word: Death is Mindbender is expiring in less than a gcd plus wiggle room" );
  main->add_action( "void_bolt,target_if=max:target.time_to_die,if=cooldown.void_bolt.remains<=0.1", "Use Voidbolt on the enemy with the largest time to die. Force a cooldown check here to make sure SimC doesn't wait too long (i.e. weird MF:I desync with GCD)" );
  main->add_action( "call_action_list,name=empowered_filler,if=(buff.mind_spike_insanity.stack>2&talent.mind_spike|buff.mind_flay_insanity.stack>2&!talent.mind_spike)&talent.empowered_surges&!cooldown.void_eruption.up", "Do not overcap MSI or MFI during Empowered Surges (Archon)." );
  main->add_action( "call_action_list,name=heal_for_tof,if=!buff.twist_of_fate.up&buff.twist_of_fate_can_trigger_on_ally_heal.up&(talent.rhapsody|talent.divine_star|talent.halo)", "Hyper cringe optimisations that fish for TOF using heals. Set priest.twist_of_fate_heal_rppm=<rppm> to make this be used." );
  main->add_action( "devouring_plague,if=fight_remains<=duration+4", "Spend your Insanity on Devouring Plague at will if the fight will end in less than 10s" );
  main->add_action( "devouring_plague,target_if=max:target.time_to_die*(dot.devouring_plague.remains<=gcd.max|variable.dr_force_prio|!talent.distorted_reality&variable.me_force_prio),if=insanity.deficit<=35&talent.distorted_reality|buff.dark_ascension.up|buff.mind_devourer.up&cooldown.mind_blast.up&(cooldown.void_eruption.remains>=3*gcd.max|!talent.void_eruption)|buff.entropic_rift.up", "Use Devouring Plague to maximize uptime. Short circuit if you are capping on Insanity within 35 With Distorted Reality can maintain more than one at a time in multi-target." );
  main->add_action( "void_torrent,target_if=max:(dot.devouring_plague.remains*1000+target.time_to_die),if=!variable.holding_crash&!talent.entropic_rift,target_if=dot.devouring_plague.remains>=2.5", "Use Void Torrent if it will get near full Mastery Value and you have Cthun and Void Eruption. Prune this action for Entropic Rift Builds." );
  main->add_action( "shadow_crash,target_if=dot.vampiric_touch.refreshable,if=!variable.holding_crash", "Use Shadow Crash as long as you are not holding for adds and Vampiric Touch is within pandemic range" );
  main->add_action( "vampiric_touch,target_if=max:(refreshable*10000+target.time_to_die)*(dot.vampiric_touch.ticking|!variable.dots_up),if=refreshable&target.time_to_die>12&(dot.vampiric_touch.ticking|!variable.dots_up)&(variable.max_vts>0|active_enemies=1)&(cooldown.shadow_crash.remains>=dot.vampiric_touch.remains|variable.holding_crash|!talent.whispering_shadows)&(!action.shadow_crash.in_flight|!talent.whispering_shadows)", "Put out Vampiric Touch on enemies that will live at least 12s and Shadow Crash is not available soon" );
  main->add_action( "shadow_word_death,target_if=max:dot.devouring_plague.remains,if=variable.dots_up&buff.deathspeaker.up", "Spend Deathspeaker Procs" );
  main->add_action( "mind_blast,target_if=max:dot.devouring_plague.remains,if=(!buff.mind_devourer.up|!talent.mind_devourer|cooldown.void_eruption.up&talent.void_eruption)&!variable.pooling_mindblasts", "Use all charges of Mind Blast if Vampiric Touch and Shadow Word: Pain are active and Mind Devourer is not active or you are prepping Void Eruption" );
  main->add_action( "call_action_list,name=filler" );

  aoe_variables->add_action( "variable,name=max_vts,op=set,default=12,value=spell_targets.vampiric_touch>?12" );
  aoe_variables->add_action( "variable,name=is_vt_possible,op=set,value=0,default=1" );
  aoe_variables->add_action( "variable,name=is_vt_possible,op=set,value=1,target_if=max:(target.time_to_die*dot.vampiric_touch.refreshable),if=target.time_to_die>=18" );
  aoe_variables->add_action( "variable,name=dots_up,op=set,value=(active_dot.vampiric_touch+8*(action.shadow_crash.in_flight&talent.whispering_shadows))>=variable.max_vts|!variable.is_vt_possible", "TODO: Revamp to fix undesired behaviour with unstacked fights" );
  aoe_variables->add_action( "variable,name=holding_crash,op=set,value=(variable.max_vts-active_dot.vampiric_touch)<4&raid_event.adds.in>15|raid_event.adds.in<10&raid_event.adds.count>(variable.max_vts-active_dot.vampiric_touch),if=variable.holding_crash&talent.whispering_shadows&raid_event.adds.exists" );
  aoe_variables->add_action( "variable,name=manual_vts_applied,op=set,value=(active_dot.vampiric_touch+8*!variable.holding_crash)>=variable.max_vts|!variable.is_vt_possible" );

  cds->add_action( "potion,if=(buff.voidform.up|buff.power_infusion.up|buff.dark_ascension.up&(fight_remains<=cooldown.power_infusion.remains+15))&(fight_remains>=320|time_to_bloodlust>=320|buff.bloodlust.react)|fight_remains<=30", "TODO: Check VE/DA enter conditions based on dots" );
  cds->add_action( "fireblood,if=buff.power_infusion.up|fight_remains<=8" );
  cds->add_action( "berserking,if=buff.power_infusion.up|fight_remains<=12" );
  cds->add_action( "blood_fury,if=buff.power_infusion.up|fight_remains<=15" );
  cds->add_action( "ancestral_call,if=buff.power_infusion.up|fight_remains<=15" );
  cds->add_action( "use_item,name=nymues_unraveling_spindle,if=variable.dots_up&(fight_remains<30|target.time_to_die>15)&(!talent.dark_ascension|cooldown.dark_ascension.remains<3+gcd.max|fight_remains<15)", "Use Nymue's before we go into our cooldowns" );
  cds->add_action( "power_infusion,if=(buff.voidform.up|buff.dark_ascension.up&(fight_remains<=80|fight_remains>=140)|active_allied_augmentations)", "Sync Power Infusion with Voidform or Dark Ascension" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=(buff.voidform.up|buff.dark_ascension.up)&!buff.power_infusion.up", "Use <a href='https://www.wowhead.com/spell=10060/power-infusion'>Power Infusion</a> while <a href='https://www.wowhead.com/spell=194249/voidform'>Voidform</a> or <a href='https://www.wowhead.com/spell=391109/dark-ascension'>Dark Ascension</a> is active. Chain directly after your own <a href='https://www.wowhead.com/spell=10060/power-infusion'>Power Infusion</a>." );
  cds->add_action( "invoke_external_buff,name=bloodlust,if=buff.power_infusion.up&fight_remains<120|fight_remains<=40" );
  cds->add_action( "halo,if=talent.power_surge&(pet.fiend.active&cooldown.fiend.remains>=4&talent.mindbender|!talent.mindbender&!cooldown.fiend.up|active_enemies>2&!talent.inescapable_torment|!talent.dark_ascension)&(cooldown.mind_blast.charges=0|!talent.void_eruption|cooldown.void_eruption.remains>=gcd.max*4)", "Make sure Mindbender is active before popping Dark Ascension unless you have insignificant talent points or too many targets" );
  cds->add_action( "void_eruption,if=!cooldown.fiend.up&(pet.fiend.active&cooldown.fiend.remains>=4|!talent.mindbender|active_enemies>2&!talent.inescapable_torment.rank)&(cooldown.mind_blast.charges=0|time>15)", "Make sure Mindbender is active before popping Void Eruption and dump charges of Mind Blast before casting" );
  cds->add_action( "dark_ascension,if=pet.fiend.active&cooldown.fiend.remains>=4|!talent.mindbender&!cooldown.fiend.up|active_enemies>2&!talent.inescapable_torment" );
  cds->add_action( "call_action_list,name=trinkets" );
  cds->add_action( "desperate_prayer,if=health.pct<=75", "Use Desperate Prayer to heal up should Shadow Word: Death or other damage bring you below 75%" );

  filler->add_action( "vampiric_touch,target_if=min:remains,if=buff.unfurling_darkness.up", "Cast Vampiric Touch to consume Unfurling Darkness, prefering the target with the lowest DoT duration active" );
  filler->add_action( "call_action_list,name=heal_for_tof,if=!buff.twist_of_fate.up&buff.twist_of_fate_can_trigger_on_ally_heal.up&(talent.rhapsody|talent.divine_star|talent.halo)" );
  filler->add_action( "power_word_shield,if=!buff.twist_of_fate.up&buff.twist_of_fate_can_trigger_on_ally_heal.up&talent.crystalline_reflection", "Use PWS with CR talented to trigger TOF if there are no better alternatives available to do this as we still get insanity for a PWS cast." );
  filler->add_action( "call_action_list,name=empowered_filler,if=dot.devouring_plague.remains>action.mind_spike.cast_time|!talent.mind_spike" );
  filler->add_action( "shadow_word_death,target_if=target.health.pct<20|(buff.deathspeaker.up|set_bonus.tier31_2pc)&dot.devouring_plague.ticking" );
  filler->add_action( "shadow_word_death,target_if=min:target.time_to_die,if=talent.inescapable_torment&pet.fiend.active" );
  filler->add_action( "devouring_plague,if=buff.voidform.up|cooldown.dark_ascension.up|buff.mind_devourer.up" );
  filler->add_action( "halo,if=spell_targets>1", "Save up to 20s if adds are coming soon." );
  filler->add_action( "power_word_life,if=!buff.twist_of_fate.up&buff.twist_of_fate_can_trigger_on_ally_heal.up", "Using a heal with no damage kickbacks for TOF is damage neutral, so we will do it." );
  filler->add_action( "call_action_list,name=empowered_filler" );
  filler->add_action( "call_action_list,name=heal_for_tof,if=equipped.rashoks_molten_heart&(active_allies-(10-buff.molten_radiance.value))>=10&buff.molten_radiance.up,line_cd=5" );
  filler->add_action( "mind_spike,target_if=max:dot.devouring_plague.remains" );
  filler->add_action( "mind_flay,target_if=max:dot.devouring_plague.remains,chain=1,interrupt_immediate=1,interrupt_if=ticks>=2" );
  filler->add_action( "divine_star" );
  filler->add_action( "shadow_crash,if=raid_event.adds.in>20&!set_bonus.tier31_4pc", "Use Shadow Crash while moving as a low-priority action when adds will not come in 20 seconds." );
  filler->add_action( "shadow_word_death,target_if=target.health.pct<20", "Use Shadow Word: Death while moving as a low-priority action in execute" );
  filler->add_action( "shadow_word_death,target_if=max:dot.devouring_plague.remains", "Use Shadow Word: Death while moving as a low-priority action" );
  filler->add_action( "shadow_word_pain,target_if=max:dot.devouring_plague.remains,if=set_bonus.tier31_4pc", "Use Shadow Word: Pain while moving as a low-priority action with T31 4pc" );
  filler->add_action( "shadow_word_pain,target_if=min:remains,if=!set_bonus.tier31_4pc", "Use Shadow Word: Pain while moving as a low-priority action without T31 4pc" );

  empowered_filler->add_action( "mind_spike_insanity,target_if=max:dot.devouring_plague.remains" );
  empowered_filler->add_action( "mind_flay,target_if=max:dot.devouring_plague.remains,if=buff.mind_flay_insanity.up" );

  heal_for_tof->add_action( "halo", "Use Halo to acquire Twist of Fate if an ally can be healed for it and it is not currently up." );
  heal_for_tof->add_action( "divine_star", "Use Divine Star to acquire Twist of Fate if an ally can be healed for it and it is not currently up." );
  heal_for_tof->add_action( "holy_nova,if=buff.rhapsody.stack=20&talent.rhapsody", "Use Holy Nova when Rhapsody is fully stacked to acquire Twist of Fate if an ally can be healed for it and it is not currently up." );

  trinkets->add_action( "use_item,name=darkmoon_deck_box_inferno,if=equipped.darkmoon_deck_box_inferno" );
  trinkets->add_action( "use_item,name=darkmoon_deck_box_rime,if=equipped.darkmoon_deck_box_rime" );
  trinkets->add_action( "use_item,name=darkmoon_deck_box_dance,if=equipped.darkmoon_deck_box_dance" );
  trinkets->add_action( "use_item,name=dreambinder_loom_of_the_great_cycle,use_off_gcd=1,if=gcd.remains>0|fight_remains<20" );
  trinkets->add_action( "use_item,name=conjured_chillglobe" );
  trinkets->add_action( "use_item,name=iceblood_deathsnare,if=(!raid_event.adds.exists|raid_event.adds.up|spell_targets.iceblood_deathsnare>=5)|fight_remains<20" );
  trinkets->add_action( "use_item,name=erupting_spear_fragment,if=(buff.power_infusion.up|raid_event.adds.up|fight_remains<20)&equipped.erupting_spear_fragment", "Use Erupting Spear Fragment with cooldowns, adds are currently active, or the fight will end in less than 20 seconds" );
  trinkets->add_action( "use_item,name=belorrelos_the_suncaller,if=(!raid_event.adds.exists|raid_event.adds.up|spell_targets.belorrelos_the_suncaller>=5|fight_remains<20)&equipped.belorrelos_the_suncaller", "Use Belor'relos on cooldown except to hold for incoming adds or if already facing 5 or more targets" );
  trinkets->add_action( "use_item,name=beacon_to_the_beyond,if=(!raid_event.adds.exists|raid_event.adds.up|spell_targets.beacon_to_the_beyond>=5|fight_remains<20)&equipped.beacon_to_the_beyond", "Use Beacon to the Beyond on cooldown except to hold for incoming adds or if already facing 5 or more targets" );
  trinkets->add_action( "use_item,use_off_gcd=1,name=aberrant_spellforge,if=gcd.remains>0&buff.aberrant_spellforge.stack<=4" );
  trinkets->add_action( "use_item,name=spymasters_web,if=buff.spymasters_report.stack=1&buff.power_infusion.up&!buff.spymasters_web.up|buff.power_infusion.up&(fight_remains<120)|(fight_remains<=20|buff.dark_ascension.up&fight_remains<=60|buff.entropic_rift.up&talent.entropic_rift&fight_remains<=30)&!buff.spymasters_web.up" );
  trinkets->add_action( "use_items,if=(buff.voidform.up|buff.power_infusion.up|buff.dark_ascension.up|(cooldown.void_eruption.remains>10&trinket.cooldown.duration<=60))|fight_remains<20" );
  trinkets->add_action( "use_item,name=desperate_invokers_codex,if=equipped.desperate_invokers_codex" );
}
//shadow_apl_end
//shadow_ptr_apl_start
void shadow_ptr( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* main = p->get_action_priority_list( "main" );
  action_priority_list_t* aoe_variables = p->get_action_priority_list( "aoe_variables" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* filler = p->get_action_priority_list( "filler" );
  action_priority_list_t* empowered_filler = p->get_action_priority_list( "empowered_filler" );
  action_priority_list_t* heal_for_tof = p->get_action_priority_list( "heal_for_tof" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "shadowform,if=!buff.shadowform.up" );
  precombat->add_action( "variable,name=dr_force_prio,default=0,op=reset" );
  precombat->add_action( "variable,name=me_force_prio,default=1,op=reset" );
  precombat->add_action( "variable,name=max_vts,default=0,op=reset" );
  precombat->add_action( "variable,name=is_vt_possible,default=0,op=reset" );
  precombat->add_action( "variable,name=pooling_mindblasts,default=0,op=reset" );
  precombat->add_action( "arcane_torrent" );
  precombat->add_action( "use_item,name=aberrant_spellforge" );
  precombat->add_action( "shadow_crash,if=raid_event.adds.in>=25&spell_targets.shadow_crash<=8&!fight_style.dungeonslice&(!set_bonus.tier31_4pc|spell_targets.shadow_crash>1)" );
  precombat->add_action( "vampiric_touch,if=!talent.shadow_crash.enabled|raid_event.adds.in<25|spell_targets.shadow_crash>8|fight_style.dungeonslice|set_bonus.tier31_4pc&spell_targets.shadow_crash=1" );

  default_->add_action( "variable,name=holding_crash,op=set,value=raid_event.adds.in<15" );
  default_->add_action( "variable,name=pool_for_cds,op=set,value=(cooldown.void_eruption.remains<=gcd.max*3&talent.void_eruption|cooldown.dark_ascension.up&talent.dark_ascension)|talent.void_torrent&talent.psychic_link&cooldown.void_torrent.remains<=4&(!raid_event.adds.exists&spell_targets.vampiric_touch>1|raid_event.adds.in<=5|raid_event.adds.remains>=6&!variable.holding_crash)&!buff.voidform.up" );
  default_->add_action( "call_action_list,name=aoe,if=active_enemies>2" );
  default_->add_action( "run_action_list,name=main" );

  aoe->add_action( "call_action_list,name=aoe_variables" );
  aoe->add_action( "vampiric_touch,target_if=refreshable&target.time_to_die>=18&(dot.vampiric_touch.ticking|!variable.dots_up),if=(variable.max_vts>0&!variable.manual_vts_applied&!action.shadow_crash.in_flight|!talent.whispering_shadows)&!buff.entropic_rift.up", "High Priority action to put out Vampiric Touch on enemies that will live at least 18 seconds, up to 12 targets manually while prepping AoE" );
  aoe->add_action( "shadow_crash,if=!variable.holding_crash,target_if=dot.vampiric_touch.refreshable|dot.vampiric_touch.remains<=target.time_to_die&!buff.voidform.up&(raid_event.adds.in-dot.vampiric_touch.remains)<15", "Use Shadow Crash to apply Vampiric Touch to as many adds as possible while being efficient with Vampiric Touch refresh windows" );

  main->add_action( "variable,name=dots_up,op=set,value=active_dot.vampiric_touch=active_enemies|action.shadow_crash.in_flight&talent.whispering_shadows,if=active_enemies<3" );
  main->add_action( "variable,name=pooling_mindblasts,op=setif,value=1,value_else=0,condition=(cooldown.void_torrent.remains<?(variable.holding_crash*raid_event.adds.in))<=gcd.max*(1+talent.mind_melt*3),if=talent.void_blast", "Are we pooling mindblasts? Currently only used for Voidweaver." );
  main->add_action( "call_action_list,name=cds,if=fight_remains<30|target.time_to_die>15&(!variable.holding_crash|active_enemies>2)" );
  main->add_action( "mindbender,if=(dot.shadow_word_pain.ticking&variable.dots_up|action.shadow_crash.in_flight&talent.whispering_shadows)&(fight_remains<30|target.time_to_die>15)&(!talent.dark_ascension|cooldown.dark_ascension.remains<gcd.max|fight_remains<15)", "Use Shadowfiend and Mindbender on cooldown as long as Vampiric Touch and Shadow Word: Pain are active and sync with Dark Ascension" );
  main->add_action( "shadow_word_death,target_if=max:(target.health.pct<=20)*100+dot.devouring_plague.ticking,if=priest.force_devour_matter&talent.devour_matter", "High Priority Shadow Word: Death when you are forcing the bonus from Devour Matter" );
  main->add_action( "void_blast,target_if=max:(dot.devouring_plague.remains*1000+target.time_to_die),if=(dot.devouring_plague.remains>=execute_time|buff.entropic_rift.remains<=gcd.max|action.void_torrent.channeling&talent.void_empowerment)&(insanity.deficit>=16|cooldown.mind_blast.full_recharge_time<=gcd.max)&(!talent.mind_devourer|!buff.mind_devourer.up|buff.entropic_rift.remains<=gcd.max)", "Blast more burst :wicked:" );
  main->add_action( "wait,sec=cooldown.mind_blast.recharge_time,if=cooldown.mind_blast.recharge_time<buff.entropic_rift.remains&buff.entropic_rift.up&buff.entropic_rift.remains<gcd.max&cooldown.mind_blast.charges<1" );
  main->add_action( "mind_blast,if=buff.voidform.up&full_recharge_time<=gcd.max&(!talent.insidious_ire|dot.devouring_plague.remains>=execute_time)&(cooldown.void_bolt.remains%gcd.max-cooldown.void_bolt.remains%%gcd.max)*gcd.max<=0.25&(cooldown.void_bolt.remains%gcd.max-cooldown.void_bolt.remains%%gcd.max)>=0.01", "Complicated do not overcap mindblast and use it to protect against void bolt cd desync" );
  main->add_action( "void_bolt,target_if=max:target.time_to_die,if=insanity.deficit>16&cooldown.void_bolt.remains<=0.1", "Use Voidbolt on the enemy with the largest time to die. We do no care about dots because Voidbolt is only accessible inside voidform which guarantees maximum effect" );
  main->add_action( "devouring_plague,target_if=max:target.time_to_die*(dot.devouring_plague.remains<=gcd.max|variable.dr_force_prio|!talent.distorted_reality&variable.me_force_prio),if=active_dot.devouring_plague<=1&dot.devouring_plague.remains<=gcd.max&(!talent.void_eruption|cooldown.void_eruption.remains>=gcd.max*3)|insanity.deficit<=16", "Do not overcap on insanity" );
  main->add_action( "void_torrent,target_if=max:(dot.devouring_plague.remains*1000+target.time_to_die),if=(dot.devouring_plague.ticking|talent.void_eruption&cooldown.void_eruption.up)&talent.entropic_rift&!variable.holding_crash", "Cast Void Torrent at very high priority if Voidweaver" );
  main->add_action( "shadow_word_death,target_if=max:(target.health.pct<=20)*100+dot.devouring_plague.ticking,if=talent.depth_of_shadows&(target.health.pct<=20|buff.deathspeaker.up&talent.deathspeaker)", "Snipe SWDs with Depth of Shadows to spawn pets. Prefer targets with Devouring Plague on them." );
  main->add_action( "mind_blast,target_if=max:dot.devouring_plague.remains,if=(cooldown.mind_blast.full_recharge_time<=gcd.max+execute_time|pet.fiend.remains<=execute_time+gcd.max)&pet.fiend.active&talent.inescapable_torment&pet.fiend.remains>=execute_time&active_enemies<=7&(!buff.mind_devourer.up|!talent.mind_devourer)&dot.devouring_plague.remains>execute_time&!variable.pooling_mindblasts", "Use Mind Blasts if using Inescapable Torment and you are capping charges or it will expire soon. Do not use if pooling Mindblast." );
  main->add_action( "shadow_word_death,target_if=max:dot.devouring_plague.remains,if=pet.fiend.remains<=(gcd.max+1)&pet.fiend.active&talent.inescapable_torment&active_enemies<=7", "High Priority Shadow Word: Death is Mindbender is expiring in less than a gcd plus wiggle room" );
  main->add_action( "void_bolt,target_if=max:target.time_to_die,if=cooldown.void_bolt.remains<=0.1", "Use Voidbolt on the enemy with the largest time to die. Force a cooldown check here to make sure SimC doesn't wait too long (i.e. weird MF:I desync with GCD)" );
  main->add_action( "call_action_list,name=empowered_filler,if=(buff.mind_spike_insanity.stack>2&talent.mind_spike|buff.mind_flay_insanity.stack>2&!talent.mind_spike)&talent.empowered_surges&!cooldown.void_eruption.up", "Do not overcap MSI or MFI during Empowered Surges (Archon)." );
  main->add_action( "call_action_list,name=heal_for_tof,if=!buff.twist_of_fate.up&buff.twist_of_fate_can_trigger_on_ally_heal.up&(talent.rhapsody|talent.divine_star|talent.halo)", "Hyper cringe optimisations that fish for TOF using heals. Set priest.twist_of_fate_heal_rppm=<rppm> to make this be used." );
  main->add_action( "devouring_plague,if=fight_remains<=duration+4", "Spend your Insanity on Devouring Plague at will if the fight will end in less than 10s" );
  main->add_action( "devouring_plague,target_if=max:target.time_to_die*(dot.devouring_plague.remains<=gcd.max|variable.dr_force_prio|!talent.distorted_reality&variable.me_force_prio),if=insanity.deficit<=35&talent.distorted_reality|buff.dark_ascension.up|buff.mind_devourer.up&cooldown.mind_blast.up&(cooldown.void_eruption.remains>=3*gcd.max|!talent.void_eruption)|buff.entropic_rift.up", "Use Devouring Plague to maximize uptime. Short circuit if you are capping on Insanity within 35 With Distorted Reality can maintain more than one at a time in multi-target." );
  main->add_action( "void_torrent,target_if=max:(dot.devouring_plague.remains*1000+target.time_to_die),if=!variable.holding_crash&!talent.entropic_rift,target_if=dot.devouring_plague.remains>=2.5", "Use Void Torrent if it will get near full Mastery Value and you have Cthun and Void Eruption. Prune this action for Entropic Rift Builds." );
  main->add_action( "shadow_crash,target_if=dot.vampiric_touch.refreshable,if=!variable.holding_crash", "Use Shadow Crash as long as you are not holding for adds and Vampiric Touch is within pandemic range" );
  main->add_action( "vampiric_touch,target_if=max:(refreshable*10000+target.time_to_die)*(dot.vampiric_touch.ticking|!variable.dots_up),if=refreshable&target.time_to_die>12&(dot.vampiric_touch.ticking|!variable.dots_up)&(variable.max_vts>0|active_enemies=1)&(cooldown.shadow_crash.remains>=dot.vampiric_touch.remains|variable.holding_crash|!talent.whispering_shadows)&(!action.shadow_crash.in_flight|!talent.whispering_shadows)", "Put out Vampiric Touch on enemies that will live at least 12s and Shadow Crash is not available soon" );
  main->add_action( "shadow_word_death,target_if=max:dot.devouring_plague.remains,if=variable.dots_up&buff.deathspeaker.up", "Spend Deathspeaker Procs" );
  main->add_action( "mind_blast,target_if=max:dot.devouring_plague.remains,if=(!buff.mind_devourer.up|!talent.mind_devourer|cooldown.void_eruption.up&talent.void_eruption)&!variable.pooling_mindblasts", "Use all charges of Mind Blast if Vampiric Touch and Shadow Word: Pain are active and Mind Devourer is not active or you are prepping Void Eruption" );
  main->add_action( "call_action_list,name=filler" );

  aoe_variables->add_action( "variable,name=max_vts,op=set,default=12,value=spell_targets.vampiric_touch>?12" );
  aoe_variables->add_action( "variable,name=is_vt_possible,op=set,value=0,default=1" );
  aoe_variables->add_action( "variable,name=is_vt_possible,op=set,value=1,target_if=max:(target.time_to_die*dot.vampiric_touch.refreshable),if=target.time_to_die>=18" );
  aoe_variables->add_action( "variable,name=dots_up,op=set,value=(active_dot.vampiric_touch+8*(action.shadow_crash.in_flight&talent.whispering_shadows))>=variable.max_vts|!variable.is_vt_possible", "TODO: Revamp to fix undesired behaviour with unstacked fights" );
  aoe_variables->add_action( "variable,name=holding_crash,op=set,value=(variable.max_vts-active_dot.vampiric_touch)<4&raid_event.adds.in>15|raid_event.adds.in<10&raid_event.adds.count>(variable.max_vts-active_dot.vampiric_touch),if=variable.holding_crash&talent.whispering_shadows&raid_event.adds.exists" );
  aoe_variables->add_action( "variable,name=manual_vts_applied,op=set,value=(active_dot.vampiric_touch+8*!variable.holding_crash)>=variable.max_vts|!variable.is_vt_possible" );

  cds->add_action( "potion,if=(buff.voidform.up|buff.power_infusion.up|buff.dark_ascension.up&(fight_remains<=cooldown.power_infusion.remains+15))&(fight_remains>=320|time_to_bloodlust>=320|buff.bloodlust.react)|fight_remains<=30", "TODO: Check VE/DA enter conditions based on dots" );
  cds->add_action( "fireblood,if=buff.power_infusion.up|fight_remains<=8" );
  cds->add_action( "berserking,if=buff.power_infusion.up|fight_remains<=12" );
  cds->add_action( "blood_fury,if=buff.power_infusion.up|fight_remains<=15" );
  cds->add_action( "ancestral_call,if=buff.power_infusion.up|fight_remains<=15" );
  cds->add_action( "use_item,name=nymues_unraveling_spindle,if=variable.dots_up&(fight_remains<30|target.time_to_die>15)&(!talent.dark_ascension|cooldown.dark_ascension.remains<3+gcd.max|fight_remains<15)", "Use Nymue's before we go into our cooldowns" );
  cds->add_action( "power_infusion,if=(buff.voidform.up|buff.dark_ascension.up&(fight_remains<=80|fight_remains>=140)|active_allied_augmentations)", "Sync Power Infusion with Voidform or Dark Ascension" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=(buff.voidform.up|buff.dark_ascension.up)&!buff.power_infusion.up", "Use <a href='https://www.wowhead.com/spell=10060/power-infusion'>Power Infusion</a> while <a href='https://www.wowhead.com/spell=194249/voidform'>Voidform</a> or <a href='https://www.wowhead.com/spell=391109/dark-ascension'>Dark Ascension</a> is active. Chain directly after your own <a href='https://www.wowhead.com/spell=10060/power-infusion'>Power Infusion</a>." );
  cds->add_action( "invoke_external_buff,name=bloodlust,if=buff.power_infusion.up&fight_remains<120|fight_remains<=40" );
  cds->add_action( "halo,if=talent.power_surge&(pet.fiend.active&cooldown.fiend.remains>=4&talent.mindbender|!talent.mindbender&!cooldown.fiend.up|active_enemies>2&!talent.inescapable_torment|!talent.dark_ascension)&(cooldown.mind_blast.charges=0|!talent.void_eruption|cooldown.void_eruption.remains>=gcd.max*4)", "Make sure Mindbender is active before popping Dark Ascension unless you have insignificant talent points or too many targets" );
  cds->add_action( "void_eruption,if=!cooldown.fiend.up&(pet.fiend.active&cooldown.fiend.remains>=4|!talent.mindbender|active_enemies>2&!talent.inescapable_torment.rank)&(cooldown.mind_blast.charges=0|time>15)", "Make sure Mindbender is active before popping Void Eruption and dump charges of Mind Blast before casting" );
  cds->add_action( "dark_ascension,if=pet.fiend.active&cooldown.fiend.remains>=4|!talent.mindbender&!cooldown.fiend.up|active_enemies>2&!talent.inescapable_torment" );
  cds->add_action( "call_action_list,name=trinkets" );
  cds->add_action( "desperate_prayer,if=health.pct<=75", "Use Desperate Prayer to heal up should Shadow Word: Death or other damage bring you below 75%" );

  filler->add_action( "vampiric_touch,target_if=min:remains,if=buff.unfurling_darkness.up", "Cast Vampiric Touch to consume Unfurling Darkness, prefering the target with the lowest DoT duration active" );
  filler->add_action( "call_action_list,name=heal_for_tof,if=!buff.twist_of_fate.up&buff.twist_of_fate_can_trigger_on_ally_heal.up&(talent.rhapsody|talent.divine_star|talent.halo)" );
  filler->add_action( "power_word_shield,if=!buff.twist_of_fate.up&buff.twist_of_fate_can_trigger_on_ally_heal.up&talent.crystalline_reflection", "Use PWS with CR talented to trigger TOF if there are no better alternatives available to do this as we still get insanity for a PWS cast." );
  filler->add_action( "call_action_list,name=empowered_filler,if=dot.devouring_plague.remains>action.mind_spike.cast_time|!talent.mind_spike" );
  filler->add_action( "shadow_word_death,target_if=target.health.pct<20|(buff.deathspeaker.up|set_bonus.tier31_2pc)&dot.devouring_plague.ticking" );
  filler->add_action( "shadow_word_death,target_if=min:target.time_to_die,if=talent.inescapable_torment&pet.fiend.active" );
  filler->add_action( "devouring_plague,if=buff.voidform.up|cooldown.dark_ascension.up|buff.mind_devourer.up" );
  filler->add_action( "halo,if=spell_targets>1", "Save up to 20s if adds are coming soon." );
  filler->add_action( "power_word_life,if=!buff.twist_of_fate.up&buff.twist_of_fate_can_trigger_on_ally_heal.up", "Using a heal with no damage kickbacks for TOF is damage neutral, so we will do it." );
  filler->add_action( "call_action_list,name=empowered_filler" );
  filler->add_action( "call_action_list,name=heal_for_tof,if=equipped.rashoks_molten_heart&(active_allies-(10-buff.molten_radiance.value))>=10&buff.molten_radiance.up,line_cd=5" );
  filler->add_action( "mind_spike,target_if=max:dot.devouring_plague.remains" );
  filler->add_action( "mind_flay,target_if=max:dot.devouring_plague.remains,chain=1,interrupt_immediate=1,interrupt_if=ticks>=2" );
  filler->add_action( "divine_star" );
  filler->add_action( "shadow_crash,if=raid_event.adds.in>20&!set_bonus.tier31_4pc", "Use Shadow Crash while moving as a low-priority action when adds will not come in 20 seconds." );
  filler->add_action( "shadow_word_death,target_if=target.health.pct<20", "Use Shadow Word: Death while moving as a low-priority action in execute" );
  filler->add_action( "shadow_word_death,target_if=max:dot.devouring_plague.remains", "Use Shadow Word: Death while moving as a low-priority action" );
  filler->add_action( "shadow_word_pain,target_if=max:dot.devouring_plague.remains,if=set_bonus.tier31_4pc", "Use Shadow Word: Pain while moving as a low-priority action with T31 4pc" );
  filler->add_action( "shadow_word_pain,target_if=min:remains,if=!set_bonus.tier31_4pc", "Use Shadow Word: Pain while moving as a low-priority action without T31 4pc" );

  empowered_filler->add_action( "mind_spike_insanity,target_if=max:dot.devouring_plague.remains" );
  empowered_filler->add_action( "mind_flay,target_if=max:dot.devouring_plague.remains,if=buff.mind_flay_insanity.up" );

  heal_for_tof->add_action( "halo", "Use Halo to acquire Twist of Fate if an ally can be healed for it and it is not currently up." );
  heal_for_tof->add_action( "divine_star", "Use Divine Star to acquire Twist of Fate if an ally can be healed for it and it is not currently up." );
  heal_for_tof->add_action( "holy_nova,if=buff.rhapsody.stack=20&talent.rhapsody", "Use Holy Nova when Rhapsody is fully stacked to acquire Twist of Fate if an ally can be healed for it and it is not currently up." );

  trinkets->add_action( "use_item,name=darkmoon_deck_box_inferno,if=equipped.darkmoon_deck_box_inferno" );
  trinkets->add_action( "use_item,name=darkmoon_deck_box_rime,if=equipped.darkmoon_deck_box_rime" );
  trinkets->add_action( "use_item,name=darkmoon_deck_box_dance,if=equipped.darkmoon_deck_box_dance" );
  trinkets->add_action( "use_item,name=dreambinder_loom_of_the_great_cycle,use_off_gcd=1,if=gcd.remains>0|fight_remains<20" );
  trinkets->add_action( "use_item,name=conjured_chillglobe" );
  trinkets->add_action( "use_item,name=iceblood_deathsnare,if=(!raid_event.adds.exists|raid_event.adds.up|spell_targets.iceblood_deathsnare>=5)|fight_remains<20" );
  trinkets->add_action( "use_item,name=erupting_spear_fragment,if=(buff.power_infusion.up|raid_event.adds.up|fight_remains<20)&equipped.erupting_spear_fragment", "Use Erupting Spear Fragment with cooldowns, adds are currently active, or the fight will end in less than 20 seconds" );
  trinkets->add_action( "use_item,name=belorrelos_the_suncaller,if=(!raid_event.adds.exists|raid_event.adds.up|spell_targets.belorrelos_the_suncaller>=5|fight_remains<20)&equipped.belorrelos_the_suncaller", "Use Belor'relos on cooldown except to hold for incoming adds or if already facing 5 or more targets" );
  trinkets->add_action( "use_item,name=beacon_to_the_beyond,if=(!raid_event.adds.exists|raid_event.adds.up|spell_targets.beacon_to_the_beyond>=5|fight_remains<20)&equipped.beacon_to_the_beyond", "Use Beacon to the Beyond on cooldown except to hold for incoming adds or if already facing 5 or more targets" );
  trinkets->add_action( "use_item,use_off_gcd=1,name=aberrant_spellforge,if=gcd.remains>0&buff.aberrant_spellforge.stack<=4" );
  trinkets->add_action( "use_item,name=spymasters_web,if=buff.spymasters_report.stack=1&buff.power_infusion.up&!buff.spymasters_web.up|buff.power_infusion.up&(fight_remains<120)|(fight_remains<=20|buff.dark_ascension.up&fight_remains<=60|buff.entropic_rift.up&talent.entropic_rift&fight_remains<=30)&!buff.spymasters_web.up" );
  trinkets->add_action( "use_items,if=(buff.voidform.up|buff.power_infusion.up|buff.dark_ascension.up|(cooldown.void_eruption.remains>10&trinket.cooldown.duration<=60))|fight_remains<20" );
  trinkets->add_action( "use_item,name=desperate_invokers_codex,if=equipped.desperate_invokers_codex" );
}
//shadow_ptr_apl_end
//discipline_apl_start
void discipline( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* main = p->get_action_priority_list( "main" );
  action_priority_list_t* cooldowns = p->get_action_priority_list( "cooldowns" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "smite" );

  default_->add_action( "run_action_list,name=main" );

  main->add_action( "call_action_list,name=cooldowns" );
  main->add_action( "purge_the_wicked,if=refreshable" );
  main->add_action( "shadow_word_pain,if=refreshable" );
  main->add_action( "shadow_word_death,if=target.health.pct<20" );
  main->add_action( "penance" );
  main->add_action( "mind_blast" );
  main->add_action( "shadow_word_death,if=talent.expiation&(target.time_to_pct_20>(0.5*cooldown.shadow_word_death.duration))" );
  main->add_action( "halo" );
  main->add_action( "divine_star" );
  main->add_action( "shadow_word_death,if=target.time_to_pct_20>(0.5*cooldown.shadow_word_death.duration)" );
  main->add_action( "smite" );

  cooldowns->add_action( "shadowfiend,if=!talent.mindbender.enabled" );
  cooldowns->add_action( "mindbender,if=talent.mindbender.enabled" );
  cooldowns->add_action( "power_infusion" );
  cooldowns->add_action( "potion,if=buff.power_infusion.up", "sync potion with PI" );
  cooldowns->add_action( "use_items,if=buff.power_infusion.up|cooldown.power_infusion.remains>=cooldown", "sync trinkets with PI" );
}
//discipline_apl_end
//holy_apl_start
void holy( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* main = p->get_action_priority_list( "main" );
  action_priority_list_t* divine_favor_chastise_prep = p->get_action_priority_list( "divine_favor_chastise_prep" );
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
  main->add_action( "holy_fire,cycle_targets=1,target_if=min:dot.holy_fire.remains,if=(talent.empyreal_blaze|talent.harmonious_apparatus|!ticking|refreshable)&!(buff.empyreal_blaze.up&(cooldown.divine_word.up|buff.divine_word.up)&cooldown.holy_word_chastise.up)", "Always use HF if we have empyreal blaze (dot extension) or harmonious apparatus (cd reduction) or if it's not currently ticking. Otherwise, only use when refreshable to be sure we get the longest duration possible. It's worth it to cast even when the target will die soon based on just the initial hit damage. We also don't want to cast immediately once empyreal blaze is up when we are prepping divine word" );
  main->add_action( "shadow_word_pain,if=(refreshable|!ticking)&(target.time_to_die>=dot.shadow_word_pain.duration)&!buff.divine_favor_chastise.up&!buff.apotheosis.up", "Don't cast SW:P during apotheosis or divine favor: chastise. We also don't cycle targets because it isn't worth the GCDs, since Smite deals slightly more damage than a full SW:P." );
  main->add_action( "call_action_list,name=divine_favor_chastise_prep,if=talent.divine_word&talent.holy_word_chastise&buff.divine_favor_chastise.down", "Prepare to enter divine favor: chastise" );
  main->add_action( "run_action_list,name=divine_favor_chastise_active,if=buff.divine_favor_chastise.up", "Enter Divine Favor rotation with divine favor: chastise buff up" );
  main->add_action( "run_action_list,name=divine_favor_filler,if=talent.divine_word&talent.holy_word_chastise&buff.divine_favor_chastise.down", "Run divine favor fillers rotation with buff down" );
  main->add_action( "run_action_list,name=divine_image,if=talent.divine_image", "Run divine image rotation with divine image talented" );
  main->add_action( "run_action_list,name=generic", "Otherwise generic rotation" );

  divine_favor_chastise_prep->add_action( "variable,name=empyreal_exec_time,op=setif,condition=talent.empyreal_blaze,value=action.empyreal_blaze.execute_time,value_else=0", "---------------------------------------------------------------------------  Divine Favor (Prep)  ---------------------------------------------------------------------------  empyreal_exec_time: Store how long EB will take to execute" );
  divine_favor_chastise_prep->add_action( "variable,name=apotheosis_exec_time,op=setif,condition=talent.apotheosis,value=action.apotheosis.execute_time,value_else=0", "apotheosis_exec_time: Store how long Apotheosis will take to execute" );
  divine_favor_chastise_prep->add_action( "apotheosis,if=(cooldown.holy_word_chastise.remains>cooldown.divine_word.remains)&(cooldown.divine_word.remains<=(variable.empyreal_exec_time+variable.apotheosis_exec_time))", "Use apotheosis to get Chastise back if its on cooldown and Divine Word will be up soon. We can use Apotheosis (2min CD) to reset Chastise for every other Divine Word (1min CD)." );
  divine_favor_chastise_prep->add_action( "empyreal_blaze,if=cooldown.divine_word.remains<=action.empyreal_blaze.execute_time", "If we're about to cast divine favor, cast empyreal blaze first so we don't waste a GCD on non-damage during the buff window" );
  divine_favor_chastise_prep->add_action( "divine_word,if=cooldown.holy_word_chastise.up&(!talent.empyreal_blaze|buff.empyreal_blaze.up)", "Divine Word only if we can sync with Chastise and (if talented) Empyreal Blaze" );
  divine_favor_chastise_prep->add_action( "holy_word_chastise,if=buff.divine_word.up", "Holy word chastise to trigger divine favor: chastise" );

  divine_favor_chastise_active->add_action( "halo,if=spell_targets.halo>=2", "---------------------------------------------------------------------------  Divine Favor (Active)  ---------------------------------------------------------------------------" );
  divine_favor_chastise_active->add_action( "divine_star,if=spell_targets.divine_star>=2" );
  divine_favor_chastise_active->add_action( "holy_nova,if=(spell_targets.holy_nova>=2&buff.rhapsody.stack>=18)|(spell_targets.holy_nova>=3&buff.rhapsody.stack>=9)|(spell_targets.holy_nova>=4&buff.rhapsody.stack>=4)|spell_targets.holy_nova>=5", "There are particular breakpoints combinations of rhapsody and spell targets beyond which holy nova beats everything else we can do" );
  divine_favor_chastise_active->add_action( "shadow_word_death,if=target.health.pct<20" );
  divine_favor_chastise_active->add_action( "holy_word_chastise" );
  divine_favor_chastise_active->add_action( "smite,cycle_targets=1,target_if=min:dot.holy_fire.remains,if=spell_targets.holy_nova>=2", "We want to cycle smite to different targets to spread holy fire dots in AOE situations, this will buff holy nova's damage" );
  divine_favor_chastise_active->add_action( "smite" );

  divine_favor_filler->add_action( "halo,if=spell_targets.halo>=2", "---------------------------------------------------------------------------  Divine Favor (Filler)  ---------------------------------------------------------------------------" );
  divine_favor_filler->add_action( "divine_star,if=spell_targets.divine_star>=2" );
  divine_favor_filler->add_action( "holy_nova,if=(spell_targets.holy_nova>=2&buff.rhapsody.stack>=18)|(spell_targets.holy_nova>=3&buff.rhapsody.stack>=9)|(spell_targets.holy_nova>=4&buff.rhapsody.stack>=4)|spell_targets.holy_nova>=5", "There are particular breakpoints combinations of rhapsody and spell targets beyond which holy nova beats everything else we can do" );
  divine_favor_filler->add_action( "shadow_word_death,if=target.health.pct<20" );
  divine_favor_filler->add_action( "holy_word_chastise,if=(cooldown.apotheosis.remains<cooldown.divine_word.remains)|(cooldown.holy_word_chastise.duration_expected<=cooldown.divine_word.remains)", "We can use chastise for damage as long as we will have apotheosis available before the next divine word, otherwise only use it when it will be back up at the same time as divine word" );
  divine_favor_filler->add_action( "smite" );

  divine_image->add_action( "apotheosis,if=buff.answered_prayers.down&!(cooldown.holy_word_sanctify.up|cooldown.holy_word_serenity.up|cooldown.holy_word_chastise.up)", "---------------------------------------------------------------------------  Divine Image  ---------------------------------------------------------------------------  We want to apotheosis when our holy words aren't about to come off of cooldown, and when answered prayers apotheosis is not already active." );
  divine_image->add_action( "holy_word_sanctify,line_cd=9", "line_cd prevents re-casting sanctify and serenity right away, wasting GCDs that could be used to proc divine image damage. In testing, the sweet spot balance between adding more divine images, triggering apotheosis, and casting damage CDs was to let the image from each healing holy word expire before re-casting" );
  divine_image->add_action( "holy_word_serenity,line_cd=9" );
  divine_image->add_action( "holy_word_chastise" );
  divine_image->add_action( "empyreal_blaze" );
  divine_image->add_action( "halo,if=spell_targets.halo>=2" );
  divine_image->add_action( "divine_star,if=spell_targets.divine_star>=2" );
  divine_image->add_action( "holy_nova,if=(spell_targets.holy_nova>=2&buff.rhapsody.stack>=18)|(spell_targets.holy_nova>=3&buff.rhapsody.stack>=9)|(spell_targets.holy_nova>=4&buff.rhapsody.stack>=4)|spell_targets.holy_nova>=5", "There are particular breakpoints combinations of rhapsody and spell targets beyond which holy nova beats everything else we can do" );
  divine_image->add_action( "shadow_word_death,if=target.health.pct<20" );
  divine_image->add_action( "smite" );

  generic->add_action( "holy_word_chastise", "---------------------------------------------------------------------------  Generic  ---------------------------------------------------------------------------" );
  generic->add_action( "empyreal_blaze" );
  generic->add_action( "apotheosis,if=cooldown.holy_word_chastise.remains>(gcd.max*3)", "Hold Apotheosis if chastise will be up soon" );
  generic->add_action( "halo,if=spell_targets.halo>=2" );
  generic->add_action( "divine_star,if=spell_targets.divine_star>=2" );
  generic->add_action( "holy_nova,if=(spell_targets.holy_nova>=2&buff.rhapsody.stack>=18)|(spell_targets.holy_nova>=3&buff.rhapsody.stack>=9)|(spell_targets.holy_nova>=4&buff.rhapsody.stack>=4)|spell_targets.holy_nova>=5", "There are particular breakpoints combinations of rhapsody and spell targets beyond which holy nova beats everything else we can do" );
  generic->add_action( "shadow_word_death,if=target.health.pct<20" );
  generic->add_action( "smite" );

  cooldowns->add_action( "shadowfiend", "---------------------------------------------------------------------------  Cooldowns  ---------------------------------------------------------------------------" );
  cooldowns->add_action( "power_infusion,if=(!talent.divine_word|(cooldown.divine_word.up&cooldown.holy_word_chastise.up))", "Sync PI with divine favor: chastise if we took divine word" );
  cooldowns->add_action( "potion,if=buff.power_infusion.up", "Only potion in sync with power infusion" );
  cooldowns->add_action( "use_items,if=buff.power_infusion.up", "hold trinkets to use with PI" );
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
