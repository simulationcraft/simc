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
  std::string lvl50_potion = ( p->specialization() == PRIEST_SHADOW ) ? "unbridled_fury" : "battle_potion_of_intellect";

  return ( p->true_level > 50 ) ? "potion_of_spectral_intellect" : lvl50_potion;
}

std::string flask( const player_t* p )
{
  return ( p->true_level > 50 ) ? "spectral_flask_of_power" : "greater_flask_of_endless_fathoms";
}

std::string food( const player_t* p )
{
  return ( p->true_level > 50 ) ? "feast_of_gluttonous_hedonism" : "baked_port_tato";
}

std::string rune( const player_t* p )
{
  return ( p->true_level > 50 ) ? "veiled_augment_rune" : "battle_scarred";
}

std::string temporary_enchant( const player_t* p )
{
  return ( p->true_level >= 60 ) ? "main_hand:shadowcore_oil" : "disabled";
}

//shadow_apl_start
void shadow( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* boon = p->get_action_priority_list( "boon" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* cwc = p->get_action_priority_list( "cwc" );
  action_priority_list_t* main = p->get_action_priority_list( "main" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "fleshcraft,if=soulbind.pustule_eruption|soulbind.volatile_solvent" );
  precombat->add_action( "shadowform,if=!buff.shadowform.up" );
  precombat->add_action( "arcane_torrent" );
  precombat->add_action( "use_item,name=shadowed_orb_of_torment" );
  precombat->add_action( "variable,name=mind_sear_cutoff,op=set,value=2" );
  precombat->add_action( "vampiric_touch,if=!talent.damnation.enabled" );
  precombat->add_action( "mind_blast,if=talent.damnation.enabled" );

  default_->add_action( "potion,if=buff.power_infusion.up&(buff.bloodlust.up|(time+fight_remains)>=320)" );
  default_->add_action( "antumbra_swap,if=buff.singularity_supreme_lockout.up&!buff.power_infusion.up&!buff.voidform.up&!pet.fiend.active&!buff.singularity_supreme.up&!buff.swap_stat_compensation.up&!buff.bloodlust.up&!((fight_remains+time)>=330&time<=200|(fight_remains+time)<=250&(fight_remains+time)>=200)" );
  default_->add_action( "antumbra_swap,if=buff.swap_stat_compensation.up&!buff.singularity_supreme_lockout.up&(cooldown.power_infusion.remains<=30&cooldown.void_eruption.remains<=30&!((time>80&time<100)&((fight_remains+time)>=330&time<=200|(fight_remains+time)<=250&(fight_remains+time)>=200))|fight_remains<=40)", "Swap onto Antumbra approximately 30s before CDS if the cds will be 2 minutes apart not 90/180s" );
  default_->add_action( "variable,name=dots_up,op=set,value=dot.shadow_word_pain.ticking&dot.vampiric_touch.ticking" );
  default_->add_action( "variable,name=all_dots_up,op=set,value=dot.shadow_word_pain.ticking&dot.vampiric_touch.ticking&dot.devouring_plague.ticking" );
  default_->add_action( "variable,name=searing_nightmare_cutoff,op=set,value=spell_targets.mind_sear>2+buff.voidform.up", "Start using Searing Nightmare at 3+ targets or 4+ if you are in Voidform" );
  default_->add_action( "variable,name=five_minutes_viable,op=set,value=(fight_remains+time)>=60*5+20", "CD management control variables to make it easier to follow whats happening, this is if there is enough time in a sim to do 5minute cds" );
  default_->add_action( "variable,name=four_minutes_viable,op=set,value=!variable.five_minutes_viable&(fight_remains+time)>=60*4+20", "Time for 4 minute CDs and it wont do 5." );
  default_->add_action( "variable,name=do_three_mins,op=set,value=(variable.five_minutes_viable|!variable.five_minutes_viable&!variable.four_minutes_viable)&time<=200", "Control to do use VF on cd for the first 3minutes" );
  default_->add_action( "variable,name=cd_management,op=set,value=variable.do_three_mins|(variable.four_minutes_viable&cooldown.power_infusion.remains<=gcd.max*3|variable.five_minutes_viable&time>300)|fight_remains<=25,default=0" );
  default_->add_action( "variable,name=max_vts,op=set,default=1,value=spell_targets.vampiric_touch" );
  default_->add_action( "variable,name=max_vts,op=set,value=5+2*(variable.cd_management&cooldown.void_eruption.remains<=10)&talent.hungering_void.enabled,if=talent.searing_nightmare.enabled&spell_targets.mind_sear=7" );
  default_->add_action( "variable,name=max_vts,op=set,value=0,if=talent.searing_nightmare.enabled&spell_targets.mind_sear>7" );
  default_->add_action( "variable,name=max_vts,op=set,value=4,if=talent.searing_nightmare.enabled&spell_targets.mind_sear=8&!talent.shadow_crash.enabled" );
  default_->add_action( "variable,name=max_vts,op=set,value=(spell_targets.mind_sear<=5)*spell_targets.mind_sear,if=buff.voidform.up" );
  default_->add_action( "variable,name=is_vt_possible,op=set,value=0,default=1" );
  default_->add_action( "variable,name=is_vt_possible,op=set,value=1,target_if=max:(target.time_to_die*dot.vampiric_touch.refreshable),if=target.time_to_die>=18" );
  default_->add_action( "variable,name=vts_applied,op=set,value=active_dot.vampiric_touch>=variable.max_vts|!variable.is_vt_possible" );
  default_->add_action( "variable,name=pool_for_cds,op=set,value=cooldown.void_eruption.up&variable.cd_management", "Cooldown Pool Variable, Used to pool before activating Voidform." );
  default_->add_action( "variable,name=on_use_trinket,value=equipped.shadowed_orb_of_torment+equipped.moonlit_prism+equipped.neural_synapse_enhancer+equipped.fleshrenders_meathook+equipped.scars_of_fraternal_strife+equipped.the_first_sigil+equipped.soulletting_ruby+equipped.inscrutable_quantum_device", "Helper variable for managing Scars Double On Use Logic, not inclusive of all trinkets." );
  default_->add_action( "blood_fury,if=buff.power_infusion.up" );
  default_->add_action( "fireblood,if=buff.power_infusion.up" );
  default_->add_action( "berserking,if=buff.power_infusion.up" );
  default_->add_action( "lights_judgment,if=spell_targets.lights_judgment>=2|(!raid_event.adds.exists|raid_event.adds.in>75)", "Use Light's Judgment if there are 2 or more targets, or adds aren't spawning for more than 75s." );
  default_->add_action( "ancestral_call,if=buff.power_infusion.up" );
  default_->add_action( "call_action_list,name=cwc" );
  default_->add_action( "run_action_list,name=main" );

  boon->add_action( "ascended_blast,if=spell_targets.mind_sear<=3" );
  boon->add_action( "ascended_nova,if=spell_targets.ascended_nova>1&spell_targets.mind_sear>1&!talent.searing_nightmare.enabled", "Only use Ascended Nova when not talented into Searing Nightmare on 2+ targets." );

  cds->add_action( "power_infusion,if=buff.voidform.up&(!variable.five_minutes_viable|time>300|time<235)|fight_remains<=25", "Use Power Infusion with Voidform. Hold for Voidform comes off cooldown in the next 10 seconds otherwise use on cd unless the player is part of the kyrian covenant, or if there will not be another Void Eruption this fight. Attempt to sync the last power infusion of the fight to void eruption for non Kyrian's." );
  cds->add_action( "fleshcraft,if=soulbind.volatile_solvent&buff.volatile_solvent_humanoid.remains<=3*gcd.max,cancel_if=buff.volatile_solvent_humanoid.up", "Fleshcraft to refresh Volatile Solvent." );
  cds->add_action( "silence,target_if=runeforge.sephuzs_proclamation.equipped&(target.is_add|target.debuff.casting.react)", "Use Silence on CD to proc Sephuz's Proclamation." );
  cds->add_action( "fae_guardians,if=!buff.voidform.up&(!cooldown.void_torrent.up|!talent.void_torrent.enabled)&(variable.dots_up&spell_targets.vampiric_touch==1|variable.vts_applied&spell_targets.vampiric_touch>1)|buff.voidform.up&(soulbind.grove_invigoration.enabled|soulbind.field_of_blossoms.enabled)", "Use Fae Guardians on CD outside of Voidform. Use Fae Guardiands in Voidform if you have either Grove Invigoration or Field of Blossoms. Wait for dots to be up before activating Fae Guardians to maximise the buff." );
  cds->add_action( "unholy_nova,if=!talent.hungering_void&variable.dots_up|debuff.hungering_void.up&buff.voidform.up|(cooldown.void_eruption.remains>15|!variable.cd_management)&!buff.voidform.up", "Use Unholy Nova on CD, holding briefly to wait for power infusion or add spawns." );
  cds->add_action( "boon_of_the_ascended,if=variable.dots_up&(cooldown.fiend.up|!runeforge.shadowflame_prism)", "Use on CD but prioritise using Void Eruption first, if used inside of VF on ST use after a voidbolt for cooldown efficiency and for hungering void uptime if talented." );
  cds->add_action( "void_eruption,if=variable.cd_management&(!soulbind.volatile_solvent|buff.volatile_solvent_humanoid.up)&(insanity<=85|talent.searing_nightmare.enabled&variable.searing_nightmare_cutoff)&!cooldown.fiend.up&(pet.fiend.active&!cooldown.shadow_word_death.up|cooldown.fiend.remains>=gcd.max*5|!runeforge.shadowflame_prism)&(cooldown.mind_blast.charges=0|time>=15)", "Use Void Eruption on cooldown, spend insanity before entering if you will overcap instantly. Make sure Shadowfiend/Mindbender and Shadow Word: Death is on cooldown before VE if Shadowflame is equipped. Ensure Mindblast is on cooldown." );
  cds->add_action( "call_action_list,name=trinkets" );
  cds->add_action( "mindbender,if=(talent.searing_nightmare.enabled&spell_targets.mind_sear>variable.mind_sear_cutoff|dot.shadow_word_pain.ticking)&variable.vts_applied", "Activate mindbender with dots up, if using shadowflame prism make sure vampiric touches are applied prior to use." );
  cds->add_action( "desperate_prayer,if=health.pct<=75" );

  cwc->add_action( "mind_blast,only_cwc=1,target_if=set_bonus.tier28_4pc&buff.dark_thought.up&pet.fiend.active&runeforge.shadowflame_prism.equipped&!buff.voidform.up&pet.your_shadow.remains<fight_remains|buff.dark_thought.up&pet.your_shadow.remains<gcd.max*(3+(!buff.voidform.up)*16)&pet.your_shadow.remains<fight_remains", "T28 4-set conditional for CWC Mind Blast" );
  cwc->add_action( "searing_nightmare,use_while_casting=1,target_if=(variable.searing_nightmare_cutoff&!variable.pool_for_cds)|(dot.shadow_word_pain.refreshable&spell_targets.mind_sear>1)", "Use Searing Nightmare if you will hit enough targets and Power Infusion and Voidform are not ready, or to refresh SW:P on two or more targets." );
  cwc->add_action( "searing_nightmare,use_while_casting=1,target_if=talent.searing_nightmare.enabled&dot.shadow_word_pain.refreshable&spell_targets.mind_sear>2", "Short Circuit Searing Nightmare condition to keep SW:P up in AoE" );
  cwc->add_action( "mind_blast,only_cwc=1", "Only_cwc makes the action only usable during channeling and not as a regular action." );

  main->add_action( "call_action_list,name=boon,if=buff.boon_of_the_ascended.up" );
  main->add_action( "shadow_word_pain,if=buff.fae_guardians.up&!debuff.wrathful_faerie.up&spell_targets.mind_sear<4", "Make sure you put up SW:P ASAP on the target if Wrathful Faerie isn't active when fighting 1-3 targets." );
  main->add_action( "mind_sear,target_if=talent.searing_nightmare.enabled&spell_targets.mind_sear>variable.mind_sear_cutoff&!dot.shadow_word_pain.ticking&!cooldown.fiend.up&spell_targets.mind_sear>=4" );
  main->add_action( "call_action_list,name=cds" );
  main->add_action( "mind_sear,target_if=talent.searing_nightmare.enabled&spell_targets.mind_sear>variable.mind_sear_cutoff&!dot.shadow_word_pain.ticking&!cooldown.fiend.up", "High Priority Mind Sear action to refresh DoTs with Searing Nightmare" );
  main->add_action( "damnation,target_if=(dot.vampiric_touch.refreshable|dot.shadow_word_pain.refreshable|(!buff.mind_devourer.up&insanity<50))&(buff.dark_thought.stack<buff.dark_thought.max_stack|!set_bonus.tier28_2pc)", "Prefer to use Damnation ASAP if SW:P or VT is not up or you cannot cast a normal Devouring Plague (including Mind Devourer procs) and you will not cap Dark Thoughts stacks if using T28 2pc." );
  main->add_action( "shadow_word_death,if=pet.fiend.active&runeforge.shadowflame_prism.equipped&pet.fiend.remains<=gcd&spell_targets.mind_sear<=7", "Use Shadow Word Death if using Shadowflame Prism and bender will expire during the next gcd." );
  main->add_action( "mind_blast,if=(cooldown.mind_blast.full_recharge_time<=gcd.max*2&(debuff.hungering_void.up|!talent.hungering_void.enabled)|pet.fiend.remains<=cast_time+gcd)&pet.fiend.active&runeforge.shadowflame_prism.equipped&pet.fiend.remains>cast_time&spell_targets.mind_sear<=7|buff.dark_thought.up&buff.voidform.up&!cooldown.void_bolt.up&(!runeforge.shadowflame_prism.equipped|!pet.fiend.active)&set_bonus.tier28_4pc", "Always use mindblasts if capped and hungering void is up and using Shadowflame Prism and bender is up.Additionally, cast mindblast if you would be unable to get the rift by waiting a gcd." );
  main->add_action( "mindgames,target_if=insanity<90&((variable.all_dots_up&(!cooldown.void_eruption.up|!variable.cd_management))|buff.voidform.up)&(!talent.hungering_void.enabled|debuff.hungering_void.remains>cast_time|!buff.voidform.up)", "Use Mindgames when all 3 DoTs are up, or you are in Voidform. Ensure Hungering Void will still be up when the cast time finishes. Stop using at 5+ targets with Searing Nightmare." );
  main->add_action( "void_bolt,if=talent.hungering_void&(insanity<=85&talent.searing_nightmare&spell_targets.mind_sear<=6|!talent.searing_nightmare|spell_targets.mind_sear=1)", "With Hungering Void, use voidbolt at a higher priority, but only as a builder if on AOE with Searing Nightmare." );
  main->add_action( "devouring_plague,if=(set_bonus.tier28_4pc|talent.hungering_void.enabled)&talent.searing_nightmare.enabled&pet.fiend.active&runeforge.shadowflame_prism.equipped&buff.voidform.up&spell_targets.mind_sear<=6", "Special Devouring Plague with Searing Nightmare when usage during Cooldowns" );
  main->add_action( "devouring_plague,if=(refreshable|insanity>75|talent.void_torrent.enabled&cooldown.void_torrent.remains<=3*gcd&!buff.voidform.up|buff.voidform.up&(cooldown.mind_blast.charges_fractional<2|buff.mind_devourer.up))&(!variable.pool_for_cds|insanity>=85)&(!talent.searing_nightmare|!variable.searing_nightmare_cutoff)", "Don't use Devouring Plague if you can get into Voidform instead, or if Searing Nightmare is talented and will hit enough targets." );
  main->add_action( "void_bolt,if=talent.hungering_void.enabled&(spell_targets.mind_sear<(4+conduit.dissonant_echoes.enabled)&insanity<=85&talent.searing_nightmare.enabled|!talent.searing_nightmare.enabled)", "With Hungering Void only, Use VB on CD if you don't need to cast Devouring Plague, and there are less than 4 targets out (5 with conduit)." );
  main->add_action( "shadow_word_death,target_if=(target.health.pct<20&spell_targets.mind_sear<4)|(pet.fiend.active&runeforge.shadowflame_prism.equipped&spell_targets.mind_sear<=7)", "Use Shadow Word: Death if the target is about to die or you have Shadowflame Prism equipped with Mindbender or Shadowfiend active." );
  main->add_action( "surrender_to_madness,target_if=target.time_to_die<25&buff.voidform.down", "Use Surrender to Madness on a target that is going to die at the right time." );
  main->add_action( "void_torrent,target_if=variable.dots_up&(buff.voidform.down|buff.voidform.remains<cooldown.void_bolt.remains|prev_gcd.1.void_bolt&!buff.bloodlust.react&spell_targets.mind_sear<3)&variable.vts_applied&spell_targets.mind_sear<(5+(6*talent.twist_of_fate.enabled))", "Use Void Torrent only if SW:P and VT are active and the target won't die during the channel." );
  main->add_action( "shadow_word_death,if=runeforge.painbreaker_psalm.equipped&variable.dots_up&target.time_to_pct_20>(cooldown.shadow_word_death.duration+gcd)", "Use SW:D with Painbreaker Psalm unless the target will be below 20% before the cooldown comes back." );
  main->add_action( "shadow_crash,if=raid_event.adds.in>10", "Use Shadow Crash on CD unless there are adds incoming." );
  main->add_action( "mind_sear,target_if=spell_targets.mind_sear>variable.mind_sear_cutoff&buff.dark_thought.up,chain=1,interrupt_immediate=1,interrupt_if=ticks>=4", "Use Mind Sear to consume Dark Thoughts procs on AOE. TODO Confirm is this is a higher priority than redotting on AOE unless dark thoughts is about to time out" );
  main->add_action( "mind_flay,if=buff.dark_thought.up&variable.dots_up&!buff.voidform.up&!variable.pool_for_cds&cooldown.mind_blast.full_recharge_time>=gcd.max,chain=1,interrupt_immediate=1,interrupt_if=ticks>=4&!buff.dark_thought.up", "Use Mind Flay to consume Dark Thoughts procs on ST outside of VF." );
  main->add_action( "mind_blast,if=variable.dots_up&raid_event.movement.in>cast_time+0.5&spell_targets.mind_sear<(4+2*talent.misery.enabled+active_dot.vampiric_touch*talent.psychic_link.enabled+(spell_targets.mind_sear>?5)*(pet.fiend.active&runeforge.shadowflame_prism.equipped))&(!runeforge.shadowflame_prism.equipped|!cooldown.fiend.up&runeforge.shadowflame_prism.equipped|variable.vts_applied)", "Use Mind Blast if you don't need to refresh DoTs. Stop casting at 4 or more targets with Searing Nightmare talented and you are not using Shadowflame Prism or Psychic Link.spell_targets.mind_sear>?5 gets the minimum of 5 and the number of targets. Also, do not press mindblast until all targets are dotted with VT when using shadowflame prism if bender is available." );
  main->add_action( "void_bolt,if=variable.dots_up", "Low Priority Void Bolt Fall Through" );
  main->add_action( "vampiric_touch,target_if=refreshable&target.time_to_die>=18&(dot.vampiric_touch.ticking|!variable.vts_applied)&variable.max_vts>0|(talent.misery.enabled&dot.shadow_word_pain.refreshable)|buff.unfurling_darkness.up", "Refresh Vampiric Touch wisely based on Damnation and other Talents." );
  main->add_action( "shadow_word_pain,if=refreshable&target.time_to_die>4&!talent.misery.enabled&talent.psychic_link.enabled&spell_targets.mind_sear>2", "Special condition to stop casting SW:P on off-targets when fighting 3 or more stacked mobs and using Psychic Link and NOT Misery." );
  main->add_action( "shadow_word_pain,target_if=refreshable&target.time_to_die>4&!talent.misery.enabled&!(talent.searing_nightmare.enabled&spell_targets.mind_sear>variable.mind_sear_cutoff)&(!talent.psychic_link.enabled|(talent.psychic_link.enabled&spell_targets.mind_sear<=2))", "Keep SW:P up on as many targets as possible, except when fighting 3 or more stacked mobs with Psychic Link." );
  main->add_action( "mind_sear,target_if=spell_targets.mind_sear>variable.mind_sear_cutoff,chain=1,interrupt_immediate=1,interrupt_if=ticks>=2" );
  main->add_action( "mind_flay,chain=1,interrupt_immediate=1,interrupt_if=ticks>=2&(!buff.dark_thought.up|cooldown.void_bolt.up&(buff.voidform.up|!buff.dark_thought.up&buff.dissonant_echoes.up))" );
  main->add_action( "shadow_word_death", "Use SW:D as last resort if on the move" );
  main->add_action( "shadow_word_pain", "Use SW:P as last resort if on the move and SW:D is on CD" );

  trinkets->add_action( "use_item,name=scars_of_fraternal_strife,if=!buff.scars_of_fraternal_strife_4.up", "Use trinket after pull starts and then on CD after that until you get 4th stack. Use before other on use to not trigger ICD between trinkets (bug?)." );
  trinkets->add_action( "use_item,name=empyreal_ordnance,if=cooldown.void_eruption.remains<=12|cooldown.void_eruption.remains>27", "Use on CD ASAP to get DoT ticking and expire to line up better with Voidform" );
  trinkets->add_action( "use_item,name=inscrutable_quantum_device,if=buff.voidform.up&buff.power_infusion.up|fight_remains<=20|buff.power_infusion.up&cooldown.void_eruption.remains+15>fight_remains|buff.voidform.up&cooldown.power_infusion.remains+15>fight_remains|(cooldown.power_infusion.remains>=10&cooldown.void_eruption.remains>=10)&fight_remains>=190", "Try to Sync IQD with Double Stacked CDs if possible. On longer fights with more IQD uses attempt to sync with any cd or just use it." );
  trinkets->add_action( "use_item,name=macabre_sheet_music,if=cooldown.void_eruption.remains>10", "Sync Sheet Music with Voidform" );
  trinkets->add_action( "use_item,name=soulletting_ruby,if=buff.power_infusion.up|!priest.self_power_infusion|equipped.shadowed_orb_of_torment,target_if=min:target.health.pct", "Sync Ruby with Power Infusion usage, make sure to snipe the lowest HP target. When used with Shadowed Orb of Torment, just use on CD as much as possible." );
  trinkets->add_action( "use_item,name=the_first_sigil,if=buff.voidform.up|buff.power_infusion.up|!priest.self_power_infusion|cooldown.void_eruption.remains>10|(equipped.soulletting_ruby&!trinket.soulletting_ruby.cooldown.up)|fight_remains<20", "First Sigil small optimization with Soulletting Ruby" );
  trinkets->add_action( "use_item,name=scars_of_fraternal_strife,if=buff.scars_of_fraternal_strife_4.up&((variable.on_use_trinket>=2&!equipped.shadowed_orb_of_torment)&cooldown.power_infusion.remains<=20&cooldown.void_eruption.remains<=(20-5*talent.ancient_madness)|buff.voidform.up&buff.power_infusion.up&(equipped.shadowed_orb_of_torment|variable.on_use_trinket<=1))&fight_remains<=80|fight_remains<=30", "Use scars as a double on use trinket if two trinkets are equipped, else use at the end of a fight in last cds." );
  trinkets->add_action( "use_item,name=neural_synapse_enhancer,if=buff.voidform.up&buff.power_infusion.up|pet.fiend.active&cooldown.power_infusion.remains>=10*gcd.max", "Use Neural Synapse Enhance to Buff Shadowfiends or in full cds." );
  trinkets->add_action( "use_item,name=sinful_gladiators_badge_of_ferocity,if=cooldown.void_eruption.remains>=10", "Use Badge inside of VF for the first use or on CD after the first use. Short circuit if void eruption cooldown is 10s or more away." );
  trinkets->add_action( "use_item,name=shadowed_orb_of_torment,if=cooldown.power_infusion.remains<=10&cooldown.void_eruption.remains<=10|covenant.night_fae&(!buff.voidform.up|prev_gcd.1.void_bolt)|fight_remains<=40", "Use Shadowed Orb of Torment when not in Voidform, or in between Void Bolt casts in Voidform. As Kyrian or Necrolord line it up with stacked cooldowns." );
  trinkets->add_action( "use_item,name=architects_ingenuity_core", "Use this on CD for max CDR" );
  trinkets->add_action( "use_items,if=buff.voidform.up|buff.power_infusion.up|cooldown.void_eruption.remains>10", "Default fallback for usable items: Use on cooldown in order by trinket slot." );
}
//shadow_apl_end

void discipline( player_t* p )
{
  action_priority_list_t* def     = p->get_action_priority_list( "default" );
  action_priority_list_t* boon    = p->get_action_priority_list( "boon" );
  action_priority_list_t* racials = p->get_action_priority_list( "racials" );

  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "smite" );

  boon->add_action( "ascended_blast" );
  boon->add_action( "ascended_nova" );

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
  def->add_action(
      "shadow_covenant,if=!covenant.kyrian|(!cooldown.boon_of_the_ascended.up&!buff.boon_of_the_ascended.up)",
      "Hold Shadow Covenant if Boon of the Ascended cooldown is up or the buff is active." );
  def->add_action( "schism" );
  def->add_action( "mindgames" );
  def->add_action( "fae_guardians" );
  def->add_action( "unholy_nova" );
  def->add_action( "boon_of_the_ascended" );
  def->add_call_action_list( boon, "if=buff.boon_of_the_ascended.up" );
  def->add_action( "mindbender" );
  def->add_action( "spirit_shell" );
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

void holy( player_t* p )
{
  action_priority_list_t* precombat    = p->get_action_priority_list( "precombat" );
  action_priority_list_t* default_list = p->get_action_priority_list( "default" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "smite" );

  // On-Use Items
  default_list->add_action( "use_items" );

  // Professions
  for ( const auto& profession_action : p->get_profession_actions() )
  {
    default_list->add_action( profession_action );
  }

  // Potions
  default_list->add_action(
      "potion,if=buff.bloodlust.react|(raid_event.adds.up&(raid_event.adds.remains>20|raid_event.adds.duration<20))|"
      "target.time_to_die<=30" );

  // Default APL
  default_list->add_action(
      p, "Holy Fire",
      "if=dot.holy_fire.ticking&(dot.holy_fire.remains<=gcd|dot.holy_fire.stack<2)&spell_targets.holy_nova<7" );
  default_list->add_action( p, "Holy Word: Chastise", "if=spell_targets.holy_nova<5" );
  default_list->add_action(
      p, "Holy Fire",
      "if=dot.holy_fire.ticking&(dot.holy_fire.refreshable|dot.holy_fire.stack<2)&spell_targets.holy_nova<7" );
  default_list->add_action(
      "berserking,if=raid_event.adds.in>30|raid_event.adds.remains>8|raid_event.adds.duration<8" );
  default_list->add_action( "fireblood,if=raid_event.adds.in>20|raid_event.adds.remains>6|raid_event.adds.duration<6" );
  default_list->add_action(
      "ancestral_call,if=raid_event.adds.in>20|raid_event.adds.remains>10|raid_event.adds.duration<10" );
  default_list->add_talent(
      p, "Divine Star",
      "if=(raid_event.adds.in>5|raid_event.adds.remains>2|raid_event.adds.duration<2)&spell_targets.divine_star>1" );
  default_list->add_talent(
      p, "Halo",
      "if=(raid_event.adds.in>14|raid_event.adds.remains>2|raid_event.adds.duration<2)&spell_targets.halo>0" );
  default_list->add_action(
      "lights_judgment,if=raid_event.adds.in>50|raid_event.adds.remains>4|raid_event.adds.duration<4" );
  default_list->add_action(
      "arcane_pulse,if=(raid_event.adds.in>40|raid_event.adds.remains>2|raid_event.adds.duration<2)&spell_targets."
      "arcane_pulse>2" );
  default_list->add_action( p, "Holy Fire", "if=!dot.holy_fire.ticking&spell_targets.holy_nova<7" );
  default_list->add_action( p, "Holy Nova", "if=spell_targets.holy_nova>3" );
  default_list->add_talent(
      p, "Apotheosis", "if=active_enemies<5&(raid_event.adds.in>15|raid_event.adds.in>raid_event.adds.cooldown-5)" );
  default_list->add_action( p, "Smite" );
  default_list->add_action( p, "Holy Fire" );
  default_list->add_talent(
      p, "Divine Star",
      "if=(raid_event.adds.in>5|raid_event.adds.remains>2|raid_event.adds.duration<2)&spell_targets.divine_star>0" );
  default_list->add_action( p, "Holy Nova", "if=raid_event.movement.remains>gcd*0.3&spell_targets.holy_nova>0" );
}

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

}  // namespace priest_apl
