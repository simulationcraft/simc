#include "class_modules/apl/apl_demon_hunter.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace demon_hunter_apl
{

std::string potion_devourer( const player_t* p )
{
  return ( p->true_level > 80 ) ? "potion_of_recklessness_2" : "tempered_potion_3";
}

std::string potion_havoc( const player_t* p )
{
  return ( p->true_level > 80 ) ? "potion_of_recklessness_2" : "tempered_potion_3";
}

std::string potion_vengeance( const player_t* p )
{
  return ( p->true_level > 80 ) ? "lights_potential_2" : "tempered_potion_3";
}

std::string flask_devourer( const player_t* p )
{
  return ( p->true_level > 80 ) ? "flask_of_the_magisters_2" : "flask_of_alchemical_chaos_3";
}

std::string flask_havoc( const player_t* p )
{
  return ( p->true_level > 80 ) ? "flask_of_the_shattered_sun_2" : "flask_of_alchemical_chaos_3";
}

std::string flask_vengeance( const player_t* p )
{
  return ( p->true_level > 80 ) ? "flask_of_the_magisters_2" : "flask_of_alchemical_chaos_3";
}

std::string food_devourer( const player_t* p )
{
  return ( p->true_level > 80 ) ? "blooming_feast" : "feast_of_the_divine_day";
}

std::string food_havoc( const player_t* p )
{
  return ( p->true_level > 80 ) ? "blooming_feast" : "feast_of_the_divine_day";
}

std::string food_vengeance( const player_t* p )
{
  return ( p->true_level > 80 ) ? "silvermoon_parade" : "feast_of_the_divine_day";
}

std::string rune( const player_t* p )
{
  return ( p->true_level > 80 ) ? "void_touched" : "crystallized";
}

std::string temporary_enchant_devourer( const player_t* p )
{
  return ( p->true_level > 80 ) ? "main_hand:thalassian_phoenix_oil_2/off_hand:thalassian_phoenix_oil_2" : "main_hand:algari_mana_oil_3/off_hand:algari_mana_oil_3";
}

std::string temporary_enchant_havoc( const player_t* p )
{
  return ( p->true_level > 80 ) ? "main_hand:thalassian_phoenix_oil_2/off_hand:thalassian_phoenix_oil_2" : "main_hand:ironclaw_whetstone_3/off_hand:ironclaw_whetstone_3";
}

std::string temporary_enchant_vengeance( const player_t* p )
{
  return ( p->true_level > 80 ) ? "main_hand:thalassian_phoenix_oil_2/off_hand:thalassian_phoenix_oil_2" : "main_hand:ironclaw_whetstone_3/off_hand:ironclaw_whetstone_3";
}

// clang-format off
//devourer_apl_start
void devourer( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* illicit_doping = p->get_action_priority_list( "illicit_doping" );
  action_priority_list_t* math_for_wizards = p->get_action_priority_list( "math_for_wizards" );
  action_priority_list_t* melee_combo = p->get_action_priority_list( "melee_combo" );
  action_priority_list_t* reaps = p->get_action_priority_list( "reaps" );

  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=trinket_1_mastery,value=trinket.1.has_use_buff&trinket.1.has_buff.mastery" );
  precombat->add_action( "variable,name=trinket_2_mastery,value=trinket.2.has_use_buff&trinket.2.has_buff.mastery" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_buff.intellect|trinket.1.has_buff.mastery|trinket.1.has_buff.versatility|trinket.1.has_buff.haste|trinket.1.has_buff.crit|trinket.1.is.mirror_of_fractured_tomorrows|trinket.1.is.signet_of_the_priory" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_buff.intellect|trinket.2.has_buff.mastery|trinket.2.has_buff.versatility|trinket.2.has_buff.haste|trinket.2.has_buff.crit|trinket.2.is.mirror_of_fractured_tomorrows|trinket.2.is.signet_of_the_priory" );
  precombat->add_action( "variable,name=weapon_buffs,value=equipped.bestinslots" );
  precombat->add_action( "variable,name=weapon_sync,op=setif,value=1,value_else=0.5,condition=equipped.bestinslots" );
  precombat->add_action( "variable,name=weapon_stat_value,value=equipped.bestinslots*5142*15" );
  precombat->add_action( "variable,name=trinket_1_manual,value=trinket.1.is.belorrelos_the_suncaller|trinket.1.is.nymues_unraveling_spindle|trinket.1.is.spymasters_web" );
  precombat->add_action( "variable,name=trinket_2_manual,value=trinket.2.is.belorrelos_the_suncaller|trinket.2.is.nymues_unraveling_spindle|trinket.2.is.spymasters_web" );
  precombat->add_action( "variable,name=trinket_1_ogcd_cast,value=0" );
  precombat->add_action( "variable,name=trinket_2_ogcd_cast,value=0" );
  precombat->add_action( "variable,name=trinket_1_exclude,value=trinket.1.is.ruby_whelp_shell|trinket.1.is.whispering_incarnate_icon" );
  precombat->add_action( "variable,name=trinket_2_exclude,value=trinket.2.is.ruby_whelp_shell|trinket.2.is.whispering_incarnate_icon" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs|variable.trinket_2_buffs&((trinket.2.proc.any_dps.duration)*trinket.2.proc.any_dps.default_value)>((trinket.1.proc.any_dps.duration)*trinket.1.proc.any_dps.default_value)" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,if=variable.weapon_buffs,value=3,value_else=variable.trinket_priority,condition=!variable.trinket_1_buffs&!variable.trinket_2_buffs|variable.weapon_stat_value>(((trinket.2.proc.any_dps.duration)*trinket.2.proc.any_dps.default_value)<?((trinket.1.proc.any_dps.duration)*trinket.1.proc.any_dps.default_value))" );
  precombat->add_action( "variable,name=trinket_priority,op=set,value=trinket.1.is.signet_of_the_priory+2*trinket.2.is.signet_of_the_priory,if=equipped.signet_of_the_priory&variable.trinket_priority=3" );
  precombat->add_action( "variable,name=damage_trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&!variable.trinket_2_buffs&trinket.2.ilvl>=trinket.1.ilvl" );
  precombat->add_action( "variable,name=should_use_star,default=0,value=0,op=reset" );
  precombat->add_action( "variable,name=melee_vs,op=set,value=!talent.voidfall&talent.the_hunt&!apex.1" );
  precombat->add_action( "arcane_torrent" );
  precombat->add_action( "consume" );

  default_->add_action( "call_action_list,name=math_for_wizards" );
  default_->add_action( "call_action_list,name=illicit_doping" );
  default_->add_action( "void_ray,if=talent.eradicate&active_enemies>1&!buff.eradicate.up&talent.voidsurge" );
  default_->add_action( "voidblade,if=buff.void_metamorphosis_stack.at_max_stacks&talent.devourers_bite&talent.voidsurge" );
  default_->add_action( "the_hunt,if=buff.void_metamorphosis_stack.at_max_stacks&talent.devourers_bite&talent.voidsurge" );
  default_->add_action( "metamorphosis,if=buff.eradicate.up|!talent.eradicate|active_enemies=1|talent.voidfall" );
  default_->add_action( "call_action_list,name=reaps,if=talent.moment_of_craving&action.reap.souls_consumed>=4&buff.metamorphosis.up&!talent.voidfall&cooldown.void_ray.remains<=gcd.max&((buff.collapsing_star_stacking.stack+action.reap.souls_consumed)<=buff.collapsing_star_stacking.max_stack|!variable.should_use_star)", "Do not overcap Moment of Craving" );
  default_->add_action( "void_ray" );
  default_->add_action( "pierce_the_veil,if=buff.moment_of_craving.up&variable.should_use_star&buff.collapsing_star_stacking.stack>=30&talent.devourers_bite" );
  default_->add_action( "collapsing_star,if=variable.should_use_star" );
  default_->add_action( "call_action_list,name=reaps,if=buff.eradicate.up&active_enemies>1", "Eradichad" );
  default_->add_action( "call_action_list,name=melee_combo" );
  default_->add_action( "call_action_list,name=reaps,if=buff.voidfall_spending.stack>=3&prev_gcd.1.void_ray|buff.voidfall_spending.react>=3", "Annihilator wants to play the game too" );
  default_->add_action( "call_action_list,name=reaps,if=buff.metamorphosis.up&variable.should_use_star&(buff.collapsing_star_stacking.stack+action.reap.souls_consumed>=30&buff.collapsing_star_stacking.stack+action.reap.souls_consumed<=40)", "Star Accelerator" );
  default_->add_action( "call_action_list,name=reaps,if=!buff.metamorphosis.up&(fury+4*action.reap.souls_consumed+10*talent.scythes_embrace)>=100", "Beam Accelerator" );
  default_->add_action( "call_action_list,name=reaps,if=!talent.voidfall&(buff.metamorphosis.up|buff.moment_of_craving.up|!talent.moment_of_craving&action.reap.souls_consumed>=4)", "Just reap bro one time bro pls bro" );
  default_->add_action( "soul_immolation,if=active_dot.soul_immolation=0&!buff.metamorphosis.up" );
  default_->add_action( "devour" );
  default_->add_action( "consume" );

  illicit_doping->add_action( "invoke_external_buff,name=power_infusion,if=buff.metamorphosis.up&!buff.power_infusion.up" );
  illicit_doping->add_action( "potion,if=buff.metamorphosis.up&void_metamorphosis_base_drain_ps<30&(!variable.trinket_1_mastery&!variable.trinket_2_mastery|stat.mastery_rating>stat.haste_rating|variable.trinket_1_mastery&trinket.1.cooldown.remains>=30|variable.trinket_2_mastery&trinket.2.cooldown.remains>=30)|fight_remains<=30" );
  illicit_doping->add_action( "use_item,slot=trinket1,if=buff.metamorphosis.up&void_metamorphosis_base_drain_ps<30&(!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1|variable.trinket_2_exclude)&!variable.trinket_1_manual|trinket.1.proc.any_dps.duration>=fight_remains|fight_remains<=trinket.1.buff.any_dps.duration" );
  illicit_doping->add_action( "use_item,slot=trinket2,if=buff.metamorphosis.up&void_metamorphosis_base_drain_ps<30&(!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2|variable.trinket_1_exclude)&!variable.trinket_2_manual|trinket.2.proc.any_dps.duration>=fight_remains|fight_remains<=trinket.2.buff.any_dps.duration" );
  illicit_doping->add_action( "use_item,slot=main_hand,if=variable.weapon_buffs&(variable.trinket_2_buffs&(trinket.2.cooldown.remains|trinket.2.cooldown.duration<=20)|!variable.trinket_2_buffs|variable.trinket_2_exclude|variable.trinket_priority=3)&(variable.trinket_1_buffs&(trinket.1.cooldown.remains|trinket.1.cooldown.duration<=20)|!variable.trinket_1_buffs|variable.trinket_1_exclude|variable.trinket_priority=3)" );
  illicit_doping->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=!variable.trinket_1_buffs&!variable.trinket_1_manual&(variable.damage_trinket_priority=1|trinket.2.cooldown.remains|trinket.2.is.spymasters_web|trinket.2.cooldown.duration=0)&(gcd.remains>0.1)" );
  illicit_doping->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=!variable.trinket_2_buffs&!variable.trinket_2_manual&(variable.damage_trinket_priority=2|trinket.1.cooldown.remains|trinket.1.is.spymasters_web|trinket.1.cooldown.duration=0)&(gcd.remains>0.1)" );
  illicit_doping->add_action( "use_item,slot=trinket1,if=!variable.trinket_1_buffs&!variable.trinket_1_manual&(variable.damage_trinket_priority=1|trinket.2.cooldown.remains|trinket.2.is.spymasters_web|trinket.2.cooldown.duration=0)&(!variable.trinket_1_ogcd_cast)" );
  illicit_doping->add_action( "use_item,slot=trinket2,if=!variable.trinket_2_buffs&!variable.trinket_2_manual&(variable.damage_trinket_priority=2|trinket.1.cooldown.remains|trinket.1.is.spymasters_web|trinket.1.cooldown.duration=0)&(!variable.trinket_2_ogcd_cast)" );

  math_for_wizards->add_action( "variable,name=should_use_star,op=set,value=(active_enemies>1|apex.1|buff.dark_matter.up|talent.star_fragments)&!variable.melee_vs,if=talent.collapsing_star" );

  melee_combo->add_action( "vengeful_retreat,if=buff.voidstep.up&(buff.collapsing_star_stacking.stack<30|cooldown.voidblade.up|cooldown.predators_wake.up|buff.collapsing_star_stacking.stack<=38)", "Use Voidsteps on CD - Do not use Voidstep if you need to be stationary for Collapsing Star afterwards." );
  melee_combo->add_action( "hungering_slash,if=active_enemies>1" );
  melee_combo->add_action( "reapers_toll,if=buff.voidsurge_reapers_toll.up|active_enemies>1" );
  melee_combo->add_action( "the_hunt,if=!talent.voidsurge&!talent.devourers_bite|talent.devourers_bite&!talent.voidsurge&buff.metamorphosis.up" );
  melee_combo->add_action( "pierce_the_veil,if=buff.voidsurge_pierce_the_veil.up|talent.duty_eternal&active_enemies=1|talent.devourers_bite|talent.hungering_slash&active_enemies>1" );
  melee_combo->add_action( "predators_wake" );
  melee_combo->add_action( "voidblade,if=(talent.duty_eternal&active_enemies=1|talent.hungering_slash&active_enemies>1)&!talent.devourers_bite|talent.devourers_bite&!talent.voidsurge&buff.metamorphosis.up" );

  reaps->add_action( "eradicate" );
  reaps->add_action( "cull" );
  reaps->add_action( "reap" );
}
//devourer_apl_end
// clang-format on

// clang-format off
//devourer_ptr_apl_start
//devourer_ptr_apl_end
// clang-format on

// clang-format off
//havoc_apl_start
void havoc( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cooldown = p->get_action_priority_list( "cooldown" );
  action_priority_list_t* meta = p->get_action_priority_list( "meta" );

  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=tab_target_burning_wound,op=reset,default=1" );
  precombat->add_action( "variable,name=rg_ds,default=0,op=reset" );
  precombat->add_action( "variable,name=trinket1_special,value=trinket.1.is.algethar_puzzle_box", "Categorize on-use trinkets for cooldown alignment" );
  precombat->add_action( "variable,name=trinket2_special,value=trinket.2.is.algethar_puzzle_box" );
  precombat->add_action( "variable,name=trinket1_crit,value=!variable.trinket1_special&trinket.1.has_cooldown&trinket.1.has_use_damage" );
  precombat->add_action( "variable,name=trinket2_crit,value=!variable.trinket2_special&trinket.2.has_cooldown&trinket.2.has_use_damage" );
  precombat->add_action( "variable,name=trinket1_steroids,value=!variable.trinket1_special&trinket.1.has_cooldown&trinket.1.has_use_buff" );
  precombat->add_action( "variable,name=trinket2_steroids,value=!variable.trinket2_special&trinket.2.has_cooldown&trinket.2.has_use_buff" );
  precombat->add_action( "immolation_aura" );

  default_->add_action( "auto_attack" );
  default_->add_action( "variable,name=rg_inc,op=set,value=buff.rending_strike.down&buff.glaive_flurry.up&cooldown.blade_dance.up&gcd.remains=0|variable.rg_inc&prev_gcd.1.death_sweep" );
  default_->add_action( "cycling_variable,name=pull_remains,op=reset" );
  default_->add_action( "cycling_variable,name=pull_remains,op=max,value=target.time_to_die" );
  default_->add_action( "retarget_auto_attack,line_cd=1,target_if=min:debuff.burning_wound.remains,if=talent.burning_wound&active_dot.burning_wound<(spell_targets>?3)&variable.tab_target_burning_wound", "Spread Burning Wounds for uptime in multitarget scenarios" );
  default_->add_action( "retarget_auto_attack,line_cd=1,target_if=min:!target.is_boss,if=talent.burning_wound&active_dot.burning_wound=(spell_targets>?3)&variable.tab_target_burning_wound" );
  default_->add_action( "variable,name=fury_gen_per_sec,op=set,value=2%(attack_haste*2.6)*0.81*((talent.demonsurge&buff.metamorphosis.up)*3+9.5)+buff.immolation_aura.stack*4+buff.tactical_retreat.up*8+buff.student_of_suffering.up*2.5", "Fury generated per second for resource planning" );
  default_->add_action( "variable,name=double_on_use,value=variable.trinket1_steroids&trinket.1.cooldown.remains>20|variable.trinket1_steroids&trinket.1.cooldown.remains>20|!variable.trinket1_steroids&!variable.trinket2_steroids", "Prioritize on use trinkets for cooldown synching" );
  default_->add_action( "variable,name=use_blade_dance,op=set,value=active_enemies>=3-talent.trail_of_ruin|talent.first_blood|talent.screaming_brutality&(talent.burning_blades|talent.soulscar)", "Blade Dance threshold: use on 3+ targets (2+ with Trail of Ruin), always with First Blood or SB" );
  default_->add_action( "variable,name=pool_glaive_tempest,op=set,value=talent.glaive_tempest&active_enemies>=3", "Pool extra fury when Glaive Tempest passive will trigger from Blade Dance at 3+ targets" );
  default_->add_action( "variable,name=inertia_ready,op=set,value=talent.inertia&buff.inertia_trigger.up&!debuff.essence_break.up&(!buff.inertia.up|buff.inertia.remains<gcd.max|variable.inertia_consumer_soon|variable.inertia_consumer_soon_rush)", "Inertia trigger ready: we have the trigger buff and inertia is not yet active or is about to expire" );
  default_->add_action( "variable,name=inertia_consumer_soon,op=set,value=talent.inertia&(cooldown.the_hunt.remains<=3.5|cooldown.eye_beam.remains<=0.5&!action.annihilation.demonsurge_available&!action.death_sweep.demonsurge_available|cooldown.vengeful_retreat.remains<=1|buff.inertia_trigger.remains<gcd.max)", "Something worth consuming inertia trigger for is imminent" );
  default_->add_action( "variable,name=inertia_consumer_soon_rush,op=set,value=talent.inertia&(cooldown.the_hunt.remains<=gcd.max+0.5|cooldown.eye_beam.remains<=0.5&!action.annihilation.demonsurge_available&!action.death_sweep.demonsurge_available|cooldown.vengeful_retreat.remains<=1.5|buff.inertia_trigger.remains<gcd.max)", "Extended check for fel rush range (slightly larger timing window)" );
  default_->add_action( "variable,name=eb_aligned,op=set,value=!talent.inertia&(!talent.initiative|cooldown.vengeful_retreat.remains>=3|buff.initiative.up|buff.metamorphosis.up&cooldown.vengeful_retreat.remains>buff.metamorphosis.remains)|talent.inertia&(buff.inertia.up|cooldown.vengeful_retreat.remains>=3&(cooldown.the_hunt.remains>=3|!talent.the_hunt)&!buff.inertia_trigger.up|cooldown.metamorphosis.remains<=5)", "Eye beam alignment: safe to use EB without missing a VR/inertia window" );
  default_->add_action( "variable,name=bd_not_blocking,op=set,value=cooldown.blade_dance.remains>=gcd.max|!variable.use_blade_dance", "actions+=/variable,name=inertia_consumer_soon,op=set,value=talent.inertia&(cooldown.the_hunt.remains<=3.5|cooldown.eye_beam.remains<=0.5&!action.annihilation.demonsurge_available&!action.death_sweep.demonsurge_available|cooldown.vengeful_retreat.#remains&cooldown.vengeful_retreat.remains<=1|buff.inertia_trigger.remains<gcd.max|buff.metamorphosis.up&action.annihilation.demonsurge_available&!action.death_sweep.demonsurge_available&(cooldown.metamorphosis.remains<gcd.max*2|cooldown.#metamorphosis.up)) actions+=/variable,name=inertia_consumer_soon_rush,op=set,value=talent.inertia&(cooldown.the_hunt.remains<=gcd.max+0.5|cooldown.eye_beam.remains<=0.5&!action.annihilation.demonsurge_available&!action.death_sweep.demonsurge_available|cooldown.#vengeful_retreat.remains&cooldown.vengeful_retreat.remains<=1.5|buff.metamorphosis.up&action.annihilation.demonsurge_available&!action.death_sweep.demonsurge_available&(cooldown.metamorphosis.remains<gcd.max*2|cooldown.metamorphosis.up))  Blade Dance not blocking: BD is on cooldown or we are not using it" );
  default_->add_action( "variable,name=tg_spender,op=set,value=talent.furious_throws&talent.soulscar", "Archetype flags: TG spender (Furious Throws makes TG a fury-costing rotational ability)" );
  default_->add_action( "variable,name=cs_machine,op=set,value=talent.relentless_onslaught&talent.chaos_theory", "CS Machine: RO procs free CS, CT gives BD-crit CS bonus -- lower fury thresholds for CS" );
  default_->add_action( "variable,name=use_filler,op=set,value=cooldown.felblade.remains>=gcd.max&cooldown.immolation_aura.remains>=gcd.max&cooldown.eye_beam.remains>=gcd.max&variable.bd_not_blocking&(fury.deficit>variable.fury_gen_per_sec*gcd.max)", "Filler window: all priority spells on cooldown and we have fury to spend" );
  default_->add_action( "disrupt" );
  default_->add_action( "pick_up_fragment,type=all,use_off_gcd=1,if=fury<=40" );
  default_->add_action( "death_sweep,if=buff.eternal_hunt.up&!debuff.reavers_mark.up&buff.rending_strike.up&buff.glaive_flurry.up&time<10", "actions+=/retarget_auto_attack,target_if=max:debuff.reavers_mark.remains" );
  default_->add_action( "annihilation,target_if=max:target.health,if=buff.rending_strike.up&buff.glaive_flurry.down&time<10" );
  default_->add_action( "chaos_strike,target_if=max:target.health,if=buff.rending_strike.up&buff.glaive_flurry.up&(variable.rg_ds=2|active_enemies>1)&time>10&!debuff.reavers_mark.up" );
  default_->add_action( "annihilation,target_if=max:target.health,if=buff.rending_strike.up&buff.glaive_flurry.up&(variable.rg_ds=2|active_enemies>1|!debuff.reavers_mark.up)&!debuff.reavers_mark.up" );
  default_->add_action( "chaos_strike,target_if=max:debuff.reavers_mark.remains,if=buff.rending_strike.up&buff.glaive_flurry.up&(variable.rg_ds=2|active_enemies>1)&time>10&debuff.reavers_mark.remains" );
  default_->add_action( "annihilation,target_if=max:debuff.reavers_mark.remains,if=buff.rending_strike.up&buff.glaive_flurry.up&(variable.rg_ds=2|active_enemies>1)&debuff.reavers_mark.remains" );
  default_->add_action( "reavers_glaive,target_if=max:debuff.reavers_mark.remains,if=!buff.inertia_trigger.up&buff.glaive_flurry.down&buff.rending_strike.down&(variable.rg_ds=0|variable.rg_ds=1&cooldown.blade_dance.up|variable.rg_ds=2&cooldown.blade_dance.remains)&active_enemies<3&debuff.essence_break.down&(buff.metamorphosis.remains>2|cooldown.eye_beam.remains<10|fight_remains<10)&(variable.pull_remains>=10|fight_remains<=10)|fight_remains<=10" );
  default_->add_action( "reavers_glaive,target_if=max:debuff.reavers_mark.remains,if=buff.glaive_flurry.down&buff.rending_strike.down&(buff.thrill_of_the_fight_damage.up|!prev_gcd.1.death_sweep|!variable.rg_inc)&active_enemies>=2&(variable.pull_remains>=10|fight_remains<10)" );
  default_->add_action( "vengeful_retreat,use_off_gcd=1,if=talent.initiative&!buff.inner_demon.up&buff.metamorphosis.up&(cooldown.metamorphosis.ready|cooldown.metamorphosis.remains<=gcd.remains)&(!talent.chaotic_transformation|cooldown.eye_beam.remains&cooldown.blade_dance.remains&buff.metamorphosis.up)&!action.annihilation.demonsurge_available&!action.death_sweep.demonsurge_available&gcd.remains<=0.3", "Vengeful retreat movement canceled when using Metamorphosis" );
  default_->add_action( "immolation_aura,if=!debuff.essence_break.up&!buff.metamorphosis.up&talent.demonic_intensity&cooldown.metamorphosis.remains<5&talent.a_fire_inside&(talent.burning_wound|active_enemies>1)", "Spend Immolation auras before cooldown reset from Demonic Intensity" );
  default_->add_action( "call_action_list,name=cooldown" );
  default_->add_action( "immolation_aura,if=active_enemies>2&talent.ragefire&debuff.essence_break.down&(buff.metamorphosis.down|buff.metamorphosis.remains>5)" );
  default_->add_action( "immolation_aura,if=active_enemies>2&talent.ragefire&raid_event.adds.up&raid_event.adds.remains<15&raid_event.adds.remains>5&debuff.essence_break.down" );
  default_->add_action( "immolation_aura,if=talent.a_fire_inside&(charges=2|full_recharge_time<gcd.max*2)&variable.bd_not_blocking&!debuff.essence_break.up&(raid_event.adds.in>full_recharge_time|active_enemies>desired_targets)", "Prevent IA charge capping for A Fire Inside builds (2 charges available)" );
  default_->add_action( "immolation_aura,if=(active_enemies>(1-talent.burning_wound+buff.metamorphosis.up))&variable.bd_not_blocking&(raid_event.adds.in>full_recharge_time|active_enemies>desired_targets)", "&(!buff.metamorphosis.up|buff.demonsurge_demonic_intensity.up&!action.abyssal_gaze.demonsurge_available|demonsurge_available)" );
  default_->add_action( "felblade,if=variable.inertia_ready&(variable.inertia_consumer_soon|buff.metamorphosis.remains>5&buff.cycle_of_hatred.stack<4&cooldown.eye_beam.remains>5)&active_enemies<=2", "Felblade/Fel Rush to consume inertia trigger" );
  default_->add_action( "fel_rush,if=variable.inertia_ready&(variable.inertia_consumer_soon_rush|buff.metamorphosis.remains>5&buff.cycle_of_hatred.stack<4&cooldown.eye_beam.remains>5)&(active_enemies>2|cooldown.felblade.remains)" );
  default_->add_action( "vengeful_retreat,if=talent.inertia&!buff.inertia_trigger.up&cooldown.metamorphosis.remains>=5&((cooldown.eye_beam.remains<=gcd.max*2|cooldown.blade_dance.remains<=7&(!talent.cycle_of_hatred|buff.cycle_of_hatred.stack<3)&((cooldown.eye_beam.remains>=15-buff.cycle_of_hatred.stack*2.5)|buff.metamorphosis.remains>=5))&gcd.remains<=0.3&time>5|fight_remains<10)", "Vengeful Retreat for inertia builds" );
  default_->add_action( "vengeful_retreat,use_off_gcd=1,if=talent.initiative&!talent.inertia&((cooldown.eye_beam.remains<=gcd.remains|(cooldown.blade_dance.remains<=3&(cooldown.eye_beam.remains>=15-buff.cycle_of_hatred.stack*2.5)|buff.metamorphosis.remains>=5)&(!talent.cycle_of_hatred|buff.cycle_of_hatred.stack<4))&!buff.initiative.up&gcd.remains<=0.3&time>5|fight_remains<10)", "Vengeful Retreat for non-inertia Initiative builds" );
  default_->add_action( "run_action_list,name=meta,if=buff.metamorphosis.up" );
  default_->add_action( "fel_rush,if=talent.inertia&buff.inertia_trigger.up&variable.inertia_consumer_soon&(active_enemies>2|cooldown.felblade.remains>3|cooldown.eye_beam.up)" );
  default_->add_action( "immolation_aura,if=fight_remains<15&(variable.use_blade_dance&cooldown.blade_dance.remains|!variable.use_blade_dance)&talent.ragefire" );
  default_->add_action( "eye_beam,if=(variable.use_blade_dance&cooldown.blade_dance.remains<7|raid_event.adds.up|!variable.use_blade_dance)&(active_enemies>desired_targets*2|raid_event.adds.in>30-buff.cycle_of_hatred.stack*2.5|fight_style.dungeonroute&!raid_event.adds.in<=30-buff.cycle_of_hatred.stack*2.5)&(variable.eb_aligned|active_enemies>=5)&!buff.inner_demon.up&(!talent.eternal_hunt|cooldown.the_hunt.remains>5|hero_tree.felscarred&cooldown.metamorphosis.remains<=5|cooldown.metamorphosis.remains>=30)|fight_remains<10", "Eye Beam: at 5+ targets raw AoE damage outweighs alignment benefits, skip eb_aligned check" );
  default_->add_action( "blade_dance,if=variable.use_blade_dance&(!talent.demonic|cooldown.eye_beam.remains>=gcd.max*2|active_enemies>=5|debuff.essence_break.up)&(!variable.pool_glaive_tempest|fury>=60)", "Essence Break outside meta: softer inertia gate allows EB when trigger is down actions+=/essence_break,if=talent.essence_break&fury>=35&(buff.inertia_trigger.down|buff.inertia.up&buff.inertia.remains>=gcd.max*3|!talent.inertia)&cooldown.eye_beam.remains>5&buff.out_of_range.remains<gcd.max" );
  default_->add_action( "chaos_strike,if=debuff.essence_break.up" );
  default_->add_action( "felblade,if=!buff.inertia_trigger.up&(fury.deficit>=15+variable.fury_gen_per_sec*0.5)&(!buff.out_of_range.up|!buff.inertia.up)&(cooldown.blade_dance.remains>=0.5|!variable.use_blade_dance|fury<40|cooldown.eye_beam.remains<gcd.max*2)" );
  default_->add_action( "immolation_aura,if=active_enemies>desired_targets&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>full_recharge_time)" );
  default_->add_action( "immolation_aura,if=(raid_event.adds.in>full_recharge_time)&fury.deficit>20+variable.fury_gen_per_sec*gcd.max" );
  default_->add_action( "throw_glaive,if=talent.soulscar&(!talent.screaming_brutality|charges=2|full_recharge_time<cooldown.blade_dance.remains)&(!talent.furious_throws|variable.bd_not_blocking&(cooldown.eye_beam.remains>gcd.max*4|fury.deficit<variable.fury_gen_per_sec*gcd.max|talent.blind_fury))&!debuff.essence_break.up" );
  default_->add_action( "fel_rush,if=!buff.inertia_trigger.up&debuff.essence_break.down&variable.use_filler&active_enemies>1" );
  default_->add_action( "chaos_strike,if=(variable.bd_not_blocking|fury>=75-variable.fury_gen_per_sec*gcd.max-20*variable.cs_machine+25*variable.pool_glaive_tempest)&(cooldown.eye_beam.remains>gcd.max*4|fury.deficit<variable.fury_gen_per_sec*gcd.max|talent.blind_fury)" );
  default_->add_action( "immolation_aura,if=raid_event.adds.in>full_recharge_time|active_enemies>desired_targets&active_enemies>2" );
  default_->add_action( "felblade,if=fury<40" );
  default_->add_action( "fel_rush,if=!buff.inertia_trigger.up&debuff.essence_break.down&(variable.use_filler|active_enemies>2)" );
  default_->add_action( "throw_glaive,if=debuff.essence_break.down&variable.use_filler&!talent.furious_throws&(!buff.out_of_range.up|buff.out_of_range.remains>gcd.max)" );
  default_->add_action( "arcane_torrent,if=variable.use_filler&buff.out_of_range.down&debuff.essence_break.down&fury<35" );

  cooldown->add_action( "metamorphosis,if=((buff.metamorphosis.up|cooldown.eye_beam.remains>=10-2*talent.collective_anguish|talent.cycle_of_hatred&cooldown.eye_beam.remains>=13|raid_event.adds.remains>8&raid_event.adds.remains<cooldown.eye_beam.remains|!talent.chaotic_transformation|buff.empowered_eye_beam.up&hero_tree.felscarred)&(raid_event.adds.in>40|active_enemies>desired_targets|fight_style.dungeonroute&!raid_event.adds.in<=120)|fight_remains<30)&!buff.inner_demon.up&(cooldown.blade_dance.remains&(cooldown.blade_dance.remains>gcd.max*3|prev_gcd.1.death_sweep|prev_gcd.2.death_sweep|prev_gcd.3.death_sweep)|!talent.chaotic_transformation)&(!action.annihilation.demonsurge_available&!action.death_sweep.demonsurge_available)", "Cooldowns: metamorphosis and the_hunt" );
  cooldown->add_action( "invoke_external_buff,name=power_infusion,if=buff.metamorphosis.up|fight_remains<=20" );
  cooldown->add_action( "potion,if=fight_remains<35|cooldown.eye_beam.remains<20" );
  cooldown->add_action( "use_item,name=algethar_puzzle_box,if=!debuff.essence_break.up&(cooldown.eye_beam.remains<2&cooldown.metamorphosis.remains<6|cooldown.eye_beam.remains>6&cooldown.the_hunt.remains<2&cooldown.metamorphosis.remains<3)|fight_remains<20" );
  cooldown->add_action( "the_hunt,if=hero_tree.felscarred&!buff.metamorphosis.up&cooldown.eye_beam.remains>6&equipped.algethar_puzzle_box&cooldown.metamorphosis.remains<1&(trinket.1.is.algethar_puzzle_box&trinket.1.stat.mastery.up|trinket.2.is.algethar_puzzle_box&trinket.2.stat.mastery.up)", "Send Hunt and Metamorphis before next eyebeam due to puzzle box value" );
  cooldown->add_action( "metamorphosis,if=hero_tree.felscarred&!buff.metamorphosis.up&cooldown.eye_beam.remains>5&&equipped.algethar_puzzle_box&buff.empowered_eye_beam.up&(trinket.1.is.algethar_puzzle_box&trinket.1.stat.mastery.up|trinket.2.is.algethar_puzzle_box&trinket.2.stat.mastery.up)" );
  cooldown->add_action( "use_item,slot=trinket1,use_off_gcd=1,if=variable.trinket1_steroids&cooldown.eye_beam.up&(!variable.trinket2_special|trinket.2.cooldown.remains>20)|fight_remains<15" );
  cooldown->add_action( "use_item,slot=trinket1,use_off_gcd=1,if=variable.trinket1_crit&(buff.initiative.up|!talent.initiative)&variable.double_on_use&(!variable.trinket2_special|trinket.2.cooldown.remains>20)|fight_remains<15" );
  cooldown->add_action( "use_item,slot=trinket2,use_off_gcd=1,if=variable.trinket2_steroids&cooldown.eye_beam.up&(!variable.trinket1_special|trinket.1.cooldown.remains>20)|fight_remains<15" );
  cooldown->add_action( "use_item,slot=trinket2,use_off_gcd=1,if=variable.trinket2_crit&(buff.initiative.up|!talent.initiative)&variable.double_on_use&(!variable.trinket1_special|trinket.1.cooldown.remains>20)|fight_remains<15" );
  cooldown->add_action( "the_hunt,if=debuff.essence_break.down&!buff.reavers_glaive.up&(!talent.initiative|!buff.inertia_trigger.up&(buff.initiative.up|time>5))&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>45-talent.eternal_hunt*15)&(!talent.eternal_hunt|cooldown.eye_beam.remains<10&!hero_tree.felscarred|buff.demonsurge_demonic_intensity.up|buff.metamorphosis.up&cooldown.metamorphosis.remains<=5&talent.chaotic_transformation|cooldown.metamorphosis.remains>=30)|fight_remains<=30", "The Hunt: avoid during EB window or glaive cycle, align with Eternal Hunt EB synergy" );

  meta->add_action( "death_sweep,if=buff.metamorphosis.remains<gcd.max&(!hero_tree.felscarred|buff.demonsurge_demonic_intensity.up&cooldown.eye_beam.remains)|variable.pool_glaive_tempest&fury>=60&cooldown.eye_beam.up&cooldown.metamorphosis.remains>=5|cooldown.eye_beam.remains<gcd.max&talent.blind_fury&fury>90-variable.fury_gen_per_sec*3|debuff.essence_break.up", "actions.cooldown+=/the_hunt,if=debuff.essence_break.down&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>45)&(buff.metamorphosis.remains>5|buff.metamorphosis.down|hero_tree.felscarred)&(debuff.reavers_mark.up|raid_event.#adds.remains>=15|time>5|hero_tree.aldrachi_reaver)&(!talent.initiative|buff.initiative.up|time>5)&time>5&(!talent.inertia&buff.unbound_chaos.down|buff.inertia_trigger.down)|fight_remains<=30" );
  meta->add_action( "annihilation,if=buff.metamorphosis.remains<gcd.max&(!hero_tree.felscarred|buff.demonsurge_demonic_intensity.up&cooldown.eye_beam.remains)|debuff.essence_break.up" );
  meta->add_action( "essence_break,if=fury>=35&(cooldown.blade_dance.remains<gcd.max*2|active_enemies<3)&(!buff.inertia_trigger.up|buff.inertia.up&buff.inertia.remains>=gcd.max*3|cooldown.vengeful_retreat.remains>10|!talent.inertia)&cooldown.eye_beam.remains>5&cooldown.metamorphosis.remains>5&buff.out_of_range.remains<gcd.max|fight_remains<10", "Essence Break in meta: align with blade dance and inertia for maximum window value" );
  meta->add_action( "death_sweep,if=action.death_sweep.demonsurge_available&(buff.inertia.up|!talent.inertia)" );
  meta->add_action( "annihilation,if=action.annihilation.demonsurge_available&cooldown.blade_dance.remains&(buff.inertia.up|!talent.inertia)" );
  meta->add_action( "immolation_aura,if=demonsurge_available&buff.demonsurge.up&buff.demonsurge.remains<gcd.max", "Extend Demonsurge buff by delayed immolation aura" );
  meta->add_action( "vengeful_retreat,use_off_gcd=1,if=talent.inertia&(gcd.remains<0.3|cooldown.eye_beam.remains>gcd.remains&(buff.cycle_of_hatred.stack=2|buff.cycle_of_hatred.stack=3))&cooldown.metamorphosis.remains&!buff.inertia_trigger.up&(cooldown.eye_beam.remains>5|cooldown.eye_beam.remains<=3|cooldown.eye_beam.up)", "Vengeful Retreat during meta for inertia: proc trigger then consume on next ability" );
  meta->add_action( "eye_beam,if=!debuff.essence_break.up&!buff.inner_demon.up&!action.annihilation.demonsurge_available&!action.death_sweep.demonsurge_available&variable.eb_aligned&(!talent.eternal_hunt|cooldown.the_hunt.remains>5)|fight_remains<10", "Eye Beam in meta: avoid during essence break window, align with The Hunt via Eternal Hunt" );
  meta->add_action( "death_sweep,if=variable.use_blade_dance&!buff.chaos_theory.up&(!variable.pool_glaive_tempest|fury>=60|buff.metamorphosis.remains<=5)" );
  meta->add_action( "annihilation,if=buff.chaos_theory.up&cooldown.blade_dance.up&buff.metamorphosis.remains>=gcd.max" );
  meta->add_action( "throw_glaive,if=talent.soulscar&(!talent.screaming_brutality|charges=2|full_recharge_time<cooldown.blade_dance.remains)&(!talent.furious_throws&variable.use_filler|variable.bd_not_blocking&(fury.deficit<variable.fury_gen_per_sec*gcd.max|active_enemies>2))&!debuff.essence_break.up" );
  meta->add_action( "annihilation,if=((fury>=75-variable.fury_gen_per_sec*gcd.max-(!variable.use_blade_dance*15)-20*variable.cs_machine+25*variable.pool_glaive_tempest)|soul_fragments.total>0|talent.blind_fury&cooldown.eye_beam.remains<gcd.max*2)&(cooldown.blade_dance.remains|!variable.use_blade_dance)|buff.metamorphosis.remains<5&(!variable.use_blade_dance|variable.use_blade_dance&cooldown.blade_dance.remains>=buff.metamorphosis.remains&cooldown.blade_dance.remains>gcd.max|buff.metamorphosis.remains<gcd.max|fury>=75|buff.inertia.up)", "Annihilation filler: also cast at low fury if Blind Fury EB is about to refill" );
  meta->add_action( "felblade,if=!buff.inertia_trigger.up&(fury.deficit>15+variable.fury_gen_per_sec*0.5)&buff.metamorphosis.remains>5&(!talent.inertia|cooldown.vengeful_retreat.remains>4)&(cooldown.blade_dance.remains>=0.5|!variable.use_blade_dance)", "Felblade in meta: preserve inertia trigger for VR, skip at end of meta" );
  meta->add_action( "immolation_aura,if=buff.out_of_range.down&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>full_recharge_time)" );
  meta->add_action( "felblade,if=!buff.inertia_trigger.up&fury<35-variable.fury_gen_per_sec*0.5" );
  meta->add_action( "fel_rush,if=!buff.inertia_trigger.up&debuff.essence_break.down&variable.use_filler&(buff.metamorphosis.remains>5|active_enemies>3)" );
  meta->add_action( "throw_glaive,if=debuff.essence_break.down&variable.use_filler&!talent.furious_throws&(!buff.out_of_range.up|buff.out_of_range.remains>gcd.max)&(buff.metamorphosis.remains>5|active_enemies>3)" );
}
//havoc_apl_end
// clang-format on

// clang-format off
//havoc_ptr_apl_start
//havoc_ptr_apl_end
// clang-format on

// clang-format off
//vengeance_apl_start
void vengeance( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* externals = p->get_action_priority_list( "externals" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );
  action_priority_list_t* ar = p->get_action_priority_list( "ar" );
  action_priority_list_t* ar_fillers = p->get_action_priority_list( "ar_fillers" );
  action_priority_list_t* ar_glaive_cycle = p->get_action_priority_list( "ar_glaive_cycle" );
  action_priority_list_t* ar_cooldowns = p->get_action_priority_list( "ar_cooldowns" );
  action_priority_list_t* anni = p->get_action_priority_list( "anni" );
  action_priority_list_t* anni_voidfall = p->get_action_priority_list( "anni_voidfall" );
  action_priority_list_t* anni_meta_entry = p->get_action_priority_list( "anni_meta_entry" );
  action_priority_list_t* ur_fishing = p->get_action_priority_list( "ur_fishing" );
  action_priority_list_t* anni_meta = p->get_action_priority_list( "anni_meta" );
  action_priority_list_t* anni_cooldowns = p->get_action_priority_list( "anni_cooldowns" );
  action_priority_list_t* anni_fillers = p->get_action_priority_list( "anni_fillers" );

  precombat->add_action( "snapshot_stats", "=== Precombat ===" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_use_buff|(trinket.1.has_buff.agility|trinket.1.has_buff.mastery|trinket.1.has_buff.versatility|trinket.1.has_buff.haste|trinket.1.has_buff.crit)" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_use_buff|(trinket.2.has_buff.agility|trinket.2.has_buff.mastery|trinket.2.has_buff.versatility|trinket.2.has_buff.haste|trinket.2.has_buff.crit)" );
  precombat->add_action( "sigil_of_flame" );
  precombat->add_action( "immolation_aura" );

  default_->add_action( "variable,name=single_target,value=spell_targets.spirit_bomb=1", "=== Combat Variables ===  Target counts" );
  default_->add_action( "variable,name=aoe,value=spell_targets.spirit_bomb>=3" );
  default_->add_action( "variable,name=execute,value=fight_remains<20" );
  default_->add_action( "variable,name=is_dungeon,value=fight_style.dungeonroute|fight_style.dungeonslice", "=== Dungeon Route ===" );
  default_->add_action( "cycling_variable,name=pull_ttd,op=reset", "Per-pull max TTD (cycling across all targets in current pull)" );
  default_->add_action( "cycling_variable,name=pull_ttd,op=max,value=target.time_to_die" );
  default_->add_action( "variable,name=hold_for_next_pull,value=variable.is_dungeon&raid_event.adds.exists&raid_event.pull.remains<20&(raid_event.adds.has_boss|raid_event.adds.count>=3)", "Hold major CDs for upcoming pull if it has a boss or more enemies  Uses pull.remains (time left in current pull) instead of adds.in to avoid SimC timespan overflow bug" );
  default_->add_action( "variable,name=cd_ready,value=variable.execute|!variable.is_dungeon|(variable.pull_ttd>12&!variable.hold_for_next_pull)", "TTD guard for 40-60s CDs — also hold for next big pull (Brand/SoS/FelDev won't recharge in time)" );
  default_->add_action( "variable,name=meta_ready,value=variable.execute|!variable.is_dungeon|(variable.pull_ttd>(15-5*hero_tree.annihilator)&!variable.hold_for_next_pull)", "TTD guard for Meta — Anni gets lower bar (10) for UR proc windows + Voidfall resets" );
  default_->add_action( "variable,name=fiery_demise_active,value=talent.fiery_demise&dot.fiery_brand.ticking", "=== Global Variables ===  Fiery Demise amplification window active" );
  default_->add_action( "variable,name=fire_cd_soon,value=cooldown.soul_carver.remains>?cooldown.fel_devastation.remains>?cooldown.sigil_of_spite.remains<8", "Fire cooldown available" );
  default_->add_action( "variable,name=fragment_target,value=variable.fiery_demise_active*3+!variable.fiery_demise_active*(5-buff.metamorphosis.up)", "Fragment target: 3 during Brand, 4 in Meta, 5 baseline" );
  default_->add_action( "variable,name=fracture_cap_soon,value=cooldown.fracture.full_recharge_time<gcd.max&soul_fragments.total<6", "Fracture about to cap charges with room for more fragments" );
  default_->add_action( "auto_attack", "=== Start Actions ===" );
  default_->add_action( "disrupt,if=target.debuff.casting.react" );
  default_->add_action( "infernal_strike,use_off_gcd=1" );
  default_->add_action( "demon_spikes,use_off_gcd=1,if=!buff.demon_spikes.up&!target.cooldown.pause_action.remains&in_combat" );
  default_->add_action( "run_action_list,name=ar,if=hero_tree.aldrachi_reaver" );
  default_->add_action( "run_action_list,name=anni,if=hero_tree.annihilator" );

  externals->add_action( "invoke_external_buff,name=power_infusion", "=== Externals ===" );

  trinkets->add_action( "use_item,slot=trinket1,if=!trinket.1.is.tome_of_lights_devotion&(!variable.trinket_1_buffs|(buff.metamorphosis.up|cooldown.metamorphosis.remains<10|cooldown.metamorphosis.remains>trinket.1.cooldown.duration|(variable.trinket_2_buffs&trinket.2.cooldown.remains<cooldown.metamorphosis.remains)))", "=== Trinkets ===  Non-buff trinkets fire on cooldown; buff trinkets sync with Metamorphosis" );
  trinkets->add_action( "use_item,slot=trinket2,if=!trinket.2.is.tome_of_lights_devotion&(!variable.trinket_2_buffs|(buff.metamorphosis.up|cooldown.metamorphosis.remains<10|cooldown.metamorphosis.remains>trinket.2.cooldown.duration|(variable.trinket_1_buffs&trinket.1.cooldown.remains<cooldown.metamorphosis.remains)))" );

  ar->add_action( "call_action_list,name=trinkets", "=== Aldrachi Reaver ===" );
  ar->add_action( "potion,use_off_gcd=1,if=((buff.rending_strike.up&buff.glaive_flurry.up)|prev_gcd.1.reavers_glaive)&(!variable.is_dungeon|in_boss_encounter)" );
  ar->add_action( "call_action_list,name=externals,if=(buff.rending_strike.up&buff.glaive_flurry.up)|prev_gcd.1.reavers_glaive" );
  ar->add_action( "fiery_brand,if=!dot.fiery_brand.ticking&(cooldown.fiery_brand.charges>=2|!talent.fiery_demise)&variable.cd_ready", "Fiery brand if overcapped or not using fiery demise" );
  ar->add_action( "fiery_brand,if=talent.fiery_demise&!dot.fiery_brand.ticking&variable.meta_ready&!buff.metamorphosis.up&cooldown.metamorphosis.ready&variable.fire_cd_soon", "Fiery brand if we have demise and are about to meta or use a fire CD" );
  ar->add_action( "metamorphosis,use_off_gcd=1,if=buff.untethered_rage.up", "UR proc Meta fires unconditionally" );
  ar->add_action( "metamorphosis,use_off_gcd=1,if=!buff.metamorphosis.up&variable.meta_ready", "Hardcast Meta: enter immediately when ready" );
  ar->add_action( "call_action_list,name=ar_glaive_cycle" );
  ar->add_action( "call_action_list,name=ar_cooldowns" );
  ar->add_action( "call_action_list,name=ar_fillers", "--- Fillers ---" );

  ar_fillers->add_action( "immolation_aura,if=variable.aoe&in_combat", "=== AR Fillers — Default priority with AoE awareness ===  IA higher prio in AOE" );
  ar_fillers->add_action( "fracture,if=soul_fragments.total<variable.fragment_target" );
  ar_fillers->add_action( "spirit_bomb,if=soul_fragments>=variable.fragment_target" );
  ar_fillers->add_action( "fracture,if=buff.metamorphosis.up", "Prioritize cycling" );
  ar_fillers->add_action( "sigil_of_flame,if=variable.aoe", "AoE: SoF higher priority (free GCD with AoE damage)" );
  ar_fillers->add_action( "immolation_aura,if=!variable.is_dungeon|in_combat" );
  ar_fillers->add_action( "fracture" );
  ar_fillers->add_action( "felblade" );
  ar_fillers->add_action( "sigil_of_flame" );
  ar_fillers->add_action( "soul_cleave" );
  ar_fillers->add_action( "vengeful_retreat,use_off_gcd=1,if=talent.unhindered_assault" );
  ar_fillers->add_action( "throw_glaive" );

  ar_glaive_cycle->add_action( "reavers_glaive,if=buff.reavers_glaive.up&!buff.rending_strike.up&!buff.glaive_flurry.up", "=== AR Glaive Cycle — Art of the Glaive empowered sequence ===  AoE: Fracture first so Soul Cleave triggers 12 Bladecraft slashes on all targets  ST: Soul Cleave first so Fracture applies 2 Reaver's Mark stacks (14% damage amp)" );
  ar_glaive_cycle->add_action( "fracture,if=buff.rending_strike.up&buff.glaive_flurry.up&variable.aoe" );
  ar_glaive_cycle->add_action( "soul_cleave,if=buff.rending_strike.up&buff.glaive_flurry.up" );
  ar_glaive_cycle->add_action( "fracture,if=buff.rending_strike.up&!buff.glaive_flurry.up" );
  ar_glaive_cycle->add_action( "spirit_bomb,if=buff.glaive_flurry.up&!buff.rending_strike.up&soul_fragments>=5", "At 5+ frags, SpB outvalues SC even during empowered Glaive Flurry" );
  ar_glaive_cycle->add_action( "soul_cleave,if=buff.glaive_flurry.up&!buff.rending_strike.up" );

  ar_cooldowns->add_action( "spirit_bomb,if=variable.fiery_demise_active&soul_fragments>=3", "=== AR Cooldowns — Brand + fire CDs ===" );
  ar_cooldowns->add_action( "immolation_aura,if=variable.fiery_demise_active&talent.charred_flesh" );
  ar_cooldowns->add_action( "sigil_of_spite,if=soul_fragments.total<=2+talent.soul_sigils&(variable.fiery_demise_active|variable.cd_ready)", "Fire CDs: into active Brand (skip cd_ready) or on normal timing" );
  ar_cooldowns->add_action( "soul_carver,if=variable.fiery_demise_active|variable.cd_ready" );
  ar_cooldowns->add_action( "fel_devastation,if=!buff.rending_strike.up&!buff.glaive_flurry.up&(variable.fiery_demise_active|variable.cd_ready)", "Fel Devastation channel would interrupt the empowered cycle" );
  ar_cooldowns->add_action( "immolation_aura,if=variable.fiery_demise_active&!talent.charred_flesh", "IA in Brand window (non-Charred Flesh)" );

  anni->add_action( "variable,name=meta_entry,value=!buff.metamorphosis.up&!buff.voidfall_spending.up&buff.voidfall_building.stack<2&variable.meta_ready", "=== Annihilator ===  Meta entry conditions: not in Meta, not in Voidfall spending, building stacks low, TTD safe" );
  anni->add_action( "variable,name=burst_ready,value=variable.meta_entry&cooldown.metamorphosis.ready&(cooldown.spirit_bomb.remains<(2*gcd.max)|cooldown.spirit_bomb.remains>20)&(cooldown.soul_carver.ready|cooldown.sigil_of_spite.ready|variable.execute)", "Coordinated burst: two phases — entering (SpB nearly ready) and executing (SpB just fired, remains>20)  meta_entry check terminates burst cleanly after Meta fires (!buff.metamorphosis.up → false)" );
  anni->add_action( "variable,name=ur_fishing,value=talent.untethered_rage&buff.metamorphosis.up&buff.metamorphosis.remains<6&!buff.untethered_rage.up", "UR fishing: last 6s of Meta without proc — maximize consumption for Seething Anger BLP" );
  anni->add_action( "variable,name=hold_for_meta,value=!variable.execute&cooldown.metamorphosis.remains<=20&!buff.metamorphosis.up&cooldown.spirit_bomb.remains<=cooldown.metamorphosis.remains", "Hold CDs: Meta imminent (<20s), not yet active, SpB ready for burst entry" );
  anni->add_action( "call_action_list,name=trinkets" );
  anni->add_action( "potion,use_off_gcd=1,if=(buff.voidfall_spending.stack=3|variable.execute)&(!variable.is_dungeon|in_boss_encounter)" );
  anni->add_action( "call_action_list,name=externals,if=buff.voidfall_spending.stack=3|variable.execute" );
  anni->add_action( "call_action_list,name=anni_voidfall" );
  anni->add_action( "metamorphosis,use_off_gcd=1,if=buff.untethered_rage.up&!buff.voidfall_spending.up&variable.meta_ready", "UR Meta: consume immediately (all apex ranks)" );
  anni->add_action( "call_action_list,name=anni_meta_entry,if=variable.burst_ready", "Coordinated Meta entry: Brand → SpB → Meta(off-GCD) + SC/SoS in same cycle" );
  anni->add_action( "spirit_bomb,if=!apex.3&variable.meta_entry&cooldown.metamorphosis.ready&soul_fragments>=3&((cooldown.soul_carver.remains>5|!talent.soul_carver)&cooldown.sigil_of_spite.remains>5|variable.execute)", "Standalone pre-Meta SpB (burst not available — no SC/SoS or SpB far from ready)  apex.3 skips: enters Meta with frags for immediate Brand-amplified SpB (anni_meta)" );
  anni->add_action( "metamorphosis,use_off_gcd=1,if=variable.meta_entry&(soul_fragments>=3|!apex.3|prev_gcd.1.spirit_bomb)&((cooldown.soul_carver.remains>5|!talent.soul_carver)&cooldown.sigil_of_spite.remains>5|variable.execute)", "Standard Meta: fallback for non-burst entries" );
  anni->add_action( "call_action_list,name=ur_fishing,if=variable.ur_fishing&apex.3", "Last 6s of Meta (apex.3 only — Seething Anger BLP makes procs near-deterministic)" );
  anni->add_action( "call_action_list,name=anni_meta,if=buff.metamorphosis.up&!variable.ur_fishing" );
  anni->add_action( "call_action_list,name=anni_cooldowns" );
  anni->add_action( "call_action_list,name=anni_fillers" );

  anni_voidfall->add_action( "fiery_brand,if=talent.fiery_demise&!dot.fiery_brand.ticking&(buff.voidfall_building.stack=2|buff.voidfall_spending.stack=3)&variable.cd_ready", "=== Anni Voidfall — Building/spending cycle ===  Fiery Demise Brand at peak building (2 stacks) or peak spending (3 stacks) for maximum burst" );
  anni_voidfall->add_action( "fel_devastation,if=buff.voidfall_spending.stack=3&soul_fragments<variable.fragment_target", "Fel Devastation generates 3 fragments (Meteoric Rise) when starved at peak spending" );
  anni_voidfall->add_action( "soul_carver,if=buff.voidfall_spending.stack=3&soul_fragments<variable.fragment_target", "Fragment generators at peak spending to reach SpB threshold" );
  anni_voidfall->add_action( "sigil_of_spite,if=buff.voidfall_spending.stack=3&soul_fragments<variable.fragment_target" );
  anni_voidfall->add_action( "immolation_aura,if=buff.voidfall_spending.stack=3&talent.fallout&soul_fragments<variable.fragment_target", "Fallout: IA initial burst can shatter a fragment to reach threshold" );
  anni_voidfall->add_action( "spirit_bomb,if=buff.voidfall_spending.stack=3&soul_fragments>=variable.fragment_target" );
  anni_voidfall->add_action( "soul_cleave,if=buff.voidfall_spending.up" );
  anni_voidfall->add_action( "fracture,if=buff.voidfall_building.stack=2&fury>=70", "Pool fury so Spirit Bomb is castable immediately after spending transition" );

  anni_meta_entry->add_action( "fiery_brand,if=talent.fiery_demise&!dot.fiery_brand.ticking", "=== Anni Meta Entry — Coordinated burst: Brand → frags → SpB → Meta(off-GCD) ===  Phase 1 (burst_ready, SpB nearly ready): Brand, build frags, SpB fires, Meta off-GCD.  Phase 2: burst_ready becomes false after Meta fires (!buff.metamorphosis.up → false).  SC/SoS follow-up fires from anni_meta via prev_gcd gate (can't fire here — Mass Acceleration  resets SpB CD, and meta_entry goes false after Meta, making this list unreachable)." );
  anni_meta_entry->add_action( "immolation_aura,if=talent.charred_flesh&dot.fiery_brand.ticking&buff.immolation_aura.remains<2" );
  anni_meta_entry->add_action( "spirit_bomb,if=soul_fragments>=3" );
  anni_meta_entry->add_action( "metamorphosis,use_off_gcd=1,if=cooldown.spirit_bomb.remains>20" );
  anni_meta_entry->add_action( "fracture,if=soul_fragments<3" );

  ur_fishing->add_action( "spirit_bomb,if=buff.seething_anger.up&soul_fragments>=3", "=== UR Fishing — Consume fragments to proc Untethered Rage before Meta expires ===" );
  ur_fishing->add_action( "spirit_bomb,if=soul_fragments>=variable.fragment_target" );
  ur_fishing->add_action( "sigil_of_spite,if=soul_fragments<=2+talent.soul_sigils" );
  ur_fishing->add_action( "soul_carver,if=soul_fragments<=2+talent.soul_sigils" );
  ur_fishing->add_action( "fracture" );
  ur_fishing->add_action( "soul_cleave,if=soul_fragments>=1" );

  anni_meta->add_action( "fiery_brand,if=talent.fiery_demise&!dot.fiery_brand.ticking", "=== Anni Meta — Fracture-SpB cycling during active Meta ===  Fracture generates 3 fragments during Meta — prioritize SpB cycling  Maintain FD amplification (may need reapplication during UR-extended Meta)" );
  anni_meta->add_action( "immolation_aura,if=talent.charred_flesh&dot.fiery_brand.ticking", "Charred Flesh extends Brand duration with each Immolation Aura tick" );
  anni_meta->add_action( "soul_carver,if=(prev_gcd.1.spirit_bomb|prev_gcd.2.spirit_bomb)&soul_fragments<=3", "Burst follow-up: SC/SoS right after entry SpB+Meta for frag gen → reset SpB  prev_gcd.2 handles Brand/IA inserting a GCD between SpB and this evaluation" );
  anni_meta->add_action( "sigil_of_spite,if=(prev_gcd.1.spirit_bomb|prev_gcd.2.spirit_bomb)&soul_fragments<=2+talent.soul_sigils&!cooldown.soul_carver.ready" );
  anni_meta->add_action( "spirit_bomb,if=soul_fragments>=variable.fragment_target" );
  anni_meta->add_action( "fracture,if=soul_fragments<variable.fragment_target&!buff.voidfall_spending.up", "Primary generator during Meta — Fracture above CDs for faster SpB cycling" );
  anni_meta->add_action( "fel_devastation,if=!buff.voidfall_spending.up&(!apex.3|talent.darkglare_boon|variable.aoe)", "FelDev: skip for apex.3 without DGB during Meta (Fracture+SpB cycling yields more damage)" );
  anni_meta->add_action( "sigil_of_spite,if=soul_fragments<=2+talent.soul_sigils&(cooldown.metamorphosis.remains>25|variable.execute)", "Fragment generators as Meta fillers when below cap" );
  anni_meta->add_action( "soul_carver,if=soul_fragments<=3&(cooldown.metamorphosis.remains>25|variable.execute)" );

  anni_cooldowns->add_action( "fiery_brand,if=!dot.fiery_brand.ticking&variable.cd_ready&(cooldown.fiery_brand.charges>=2|!talent.fiery_demise|!talent.down_in_flames|variable.execute)", "=== Anni Cooldowns ===" );
  anni_cooldowns->add_action( "immolation_aura,if=talent.charred_flesh&dot.fiery_brand.ticking", "Charred Flesh extends Brand duration with each Immolation Aura tick" );
  anni_cooldowns->add_action( "sigil_of_spite,if=soul_fragments<=2+talent.soul_sigils&variable.cd_ready&!variable.hold_for_meta" );
  anni_cooldowns->add_action( "soul_carver,if=soul_fragments<=3&variable.cd_ready&!variable.hold_for_meta" );
  anni_cooldowns->add_action( "fel_devastation,if=!buff.voidfall_spending.up&(!buff.metamorphosis.up|!apex.3|talent.darkglare_boon)&variable.cd_ready", "Skip during Voidfall spending or Meta for apex.3 without Darkglare Boon" );

  anni_fillers->add_action( "spirit_bomb,if=soul_fragments>=variable.fragment_target", "=== Anni Fillers — Default priority with AoE awareness ===" );
  anni_fillers->add_action( "fracture,if=variable.fracture_cap_soon" );
  anni_fillers->add_action( "immolation_aura,if=variable.aoe&(!variable.is_dungeon|in_combat)", "IA priority in AoE — Fallout proc for fragments + AoE damage" );
  anni_fillers->add_action( "fracture,if=!buff.voidfall_spending.up", "Deprioritize Fracture during Voidfall spending to keep GCDs free for meteor-triggering spenders" );
  anni_fillers->add_action( "sigil_of_flame,if=variable.aoe", "SoF priority in AoE — free GCD with AoE damage" );
  anni_fillers->add_action( "felblade" );
  anni_fillers->add_action( "immolation_aura,if=!variable.is_dungeon|in_combat" );
  anni_fillers->add_action( "sigil_of_flame" );
  anni_fillers->add_action( "soul_cleave" );
  anni_fillers->add_action( "fracture", "Unconditional fallback — catch-all when nothing above fires" );
  anni_fillers->add_action( "throw_glaive" );
}
//vengeance_apl_end
// clang-format on

// clang-format off
//vengeance_ptr_apl_start
//vengeance_ptr_apl_end
// clang-format on

}  // namespace demon_hunter_apl
