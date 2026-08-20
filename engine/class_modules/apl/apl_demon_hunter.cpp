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
  action_priority_list_t* annihilator_ranged = p->get_action_priority_list( "annihilator_ranged" );
  action_priority_list_t* annihilator_melee = p->get_action_priority_list( "annihilator_melee" );
  action_priority_list_t* voidscarred_ranged = p->get_action_priority_list( "voidscarred_ranged" );
  action_priority_list_t* voidscarred_melee = p->get_action_priority_list( "voidscarred_melee" );
  action_priority_list_t* vsm_st = p->get_action_priority_list( "vsm_st" );
  action_priority_list_t* vsm_meta = p->get_action_priority_list( "vsm_meta" );
  action_priority_list_t* vsm_out = p->get_action_priority_list( "vsm_out" );
  action_priority_list_t* variables = p->get_action_priority_list( "variables" );
  action_priority_list_t* cooldowns = p->get_action_priority_list( "cooldowns" );
  action_priority_list_t* reaps = p->get_action_priority_list( "reaps" );

  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_buff.intellect|trinket.1.has_buff.mastery|trinket.1.has_buff.versatility|trinket.1.has_buff.haste|trinket.1.has_buff.crit" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_buff.intellect|trinket.2.has_buff.mastery|trinket.2.has_buff.versatility|trinket.2.has_buff.haste|trinket.2.has_buff.crit" );
  precombat->add_action( "variable,name=trinket_1_manual,value=trinket.1.is.font_of_venomous_rage" );
  precombat->add_action( "variable,name=trinket_2_manual,value=trinket.2.is.font_of_venomous_rage" );
  precombat->add_action( "variable,name=trinket_1_ogcd_cast,value=0" );
  precombat->add_action( "variable,name=trinket_2_ogcd_cast,value=0" );
  precombat->add_action( "variable,name=trinket_1_exclude,value=0" );
  precombat->add_action( "variable,name=trinket_2_exclude,value=0" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs|variable.trinket_2_buffs&((trinket.2.proc.any_dps.duration)*trinket.2.proc.any_dps.default_value)>((trinket.1.proc.any_dps.duration)*trinket.1.proc.any_dps.default_value)" );
  precombat->add_action( "variable,name=damage_trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&!variable.trinket_2_buffs&trinket.2.ilvl>=trinket.1.ilvl" );
  precombat->add_action( "arcane_torrent" );
  precombat->add_action( "soul_immolation" );
  precombat->add_action( "consume" );

  default_->add_action( "call_action_list,name=variables" );
  default_->add_action( "call_action_list,name=cooldowns" );
  default_->add_action( "run_action_list,name=annihilator_melee,if=hero_tree.annihilator&talent.the_hunt" );
  default_->add_action( "run_action_list,name=annihilator_ranged,if=hero_tree.annihilator" );
  default_->add_action( "run_action_list,name=voidscarred_ranged,if=hero_tree.voidscarred&!talent.the_hunt" );
  default_->add_action( "run_action_list,name=voidscarred_melee,if=hero_tree.voidscarred&talent.the_hunt" );

  annihilator_ranged->add_action( "pick_up_fragment,mode=nearest,type=all,use_off_gcd=1,line_cd=0.6,if=!buff.metamorphosis.up&!buff.void_metamorphosis_stack.at_max_stacks&buff.void_metamorphosis_stack.stack>=buff.void_metamorphosis_stack.max_stack-1-(active_enemies>1)" );
  annihilator_ranged->add_action( "pick_up_fragment,use_off_gcd=1,if=buff.metamorphosis.up&cooldown.reap.remains&soul_fragments+buff.collapsing_star_stacking.stack>=30&fury<void_metamorphosis_base_drain_ps&buff.collapsing_star_stacking.stack<30" );
  annihilator_ranged->add_action( "metamorphosis" );
  annihilator_ranged->add_action( "devour,if=buff.soulburst.up&active_enemies=1" );
  annihilator_ranged->add_action( "consume,if=buff.soulburst.up" );
  annihilator_ranged->add_action( "void_ray,if=talent.eradicate&active_enemies>1&!buff.eradicate.up" );
  annihilator_ranged->add_action( "collapsing_star,if=active_enemies>1&buff.collapsing_star_stacking.stack>=30" );
  annihilator_ranged->add_action( "call_action_list,name=reaps,if=active_enemies=1&!buff.metamorphosis.up&action.reap.souls_consumed>=4&variable.wont_drop_meta" );
  annihilator_ranged->add_action( "collapsing_star,if=active_enemies=1" );
  annihilator_ranged->add_action( "call_action_list,name=reaps,if=active_enemies=1&action.reap.souls_consumed>=4" );
  annihilator_ranged->add_action( "call_action_list,name=reaps,if=fight_remains<=6&action.reap.souls_consumed>=1" );
  annihilator_ranged->add_action( "call_action_list,name=reaps,if=active_enemies>1&buff.eradicate.up&action.reap.souls_consumed>=4+6*buff.moment_of_craving.up" );
  annihilator_ranged->add_action( "void_ray,if=!buff.eradicate.up|!buff.moment_of_craving.up|!set_bonus.midnight_season_2_4pc" );
  annihilator_ranged->add_action( "call_action_list,name=reaps,if=(!buff.eradicate.up|active_enemies=1)&(buff.voidfall_spending.stack>=3&prev_gcd.1.void_ray|buff.voidfall_spending.react>=3)" );
  annihilator_ranged->add_action( "call_action_list,name=reaps,if=buff.metamorphosis.up&talent.collapsing_star&buff.collapsing_star_stacking.stack+action.reap.souls_consumed>=30&variable.wont_overcap_cstar&void_metamorphosis_base_drain_ps>35&action.reap.souls_consumed>=4&variable.wont_drop_meta" );
  annihilator_ranged->add_action( "soul_immolation,if=active_dot.soul_immolation=0&(!buff.metamorphosis.up|fury<void_metamorphosis_base_drain_ps)" );
  annihilator_ranged->add_action( "devour" );
  annihilator_ranged->add_action( "consume" );

  annihilator_melee->add_action( "metamorphosis,if=(buff.eradicate.up|!talent.eradicate|active_enemies=1|talent.voidfall)&(active_enemies>1|buff.moment_of_craving.up|cooldown.void_ray.remains>6)" );
  annihilator_melee->add_action( "devour,if=buff.soulburst.up" );
  annihilator_melee->add_action( "consume,if=buff.soulburst.up" );
  annihilator_melee->add_action( "void_ray,if=talent.eradicate&active_enemies>1&!buff.eradicate.up" );
  annihilator_melee->add_action( "collapsing_star,if=active_enemies=1&(apex.1|buff.dark_matter.up|talent.star_fragments)" );
  annihilator_melee->add_action( "the_hunt,if=buff.metamorphosis.up" );
  annihilator_melee->add_action( "hungering_slash,if=!buff.metamorphosis.up&soul_fragments<=8" );
  annihilator_melee->add_action( "reapers_toll,if=!buff.metamorphosis.up&soul_fragments<=8" );
  annihilator_melee->add_action( "vengeful_retreat,if=talent.devourers_bite&buff.voidstep.up" );
  annihilator_melee->add_action( "hungering_slash,if=active_enemies>1" );
  annihilator_melee->add_action( "reapers_toll,if=talent.devourers_bite&(buff.voidsurge_reapers_toll.up|active_enemies>1)" );
  annihilator_melee->add_action( "voidblade,if=buff.metamorphosis.up" );
  annihilator_melee->add_action( "voidblade,if=!buff.metamorphosis.up&(active_enemies=1|!talent.eradicate&active_enemies<4)" );
  annihilator_melee->add_action( "call_action_list,name=reaps,if=active_enemies=1&action.reap.souls_consumed>=4" );
  annihilator_melee->add_action( "call_action_list,name=reaps,if=!talent.eradicate&(active_enemies>1&variable.wont_overcap_cstar&set_bonus.midnight_season_2_4pc&(buff.voidfall_spending.stack>=3&prev_gcd.1.void_ray|buff.voidfall_spending.react>=3))" );
  annihilator_melee->add_action( "call_action_list,name=reaps,if=action.reap.souls_consumed>=4&active_enemies<5&(active_enemies>1|!buff.metamorphosis.up|buff.moment_of_craving.up|buff.collapsing_star_stacking.stack+action.reap.souls_consumed>=30|void_metamorphosis_base_drain_ps>35)" );
  annihilator_melee->add_action( "call_action_list,name=reaps,if=fight_remains<=6&action.reap.souls_consumed>=1" );
  annihilator_melee->add_action( "call_action_list,name=reaps,if=talent.eradicate&active_enemies>1&buff.eradicate.up&action.reap.souls_consumed>=1" );
  annihilator_melee->add_action( "void_ray,if=!buff.eradicate.up|!buff.moment_of_craving.up|set_bonus.midnight_season_2_4pc" );
  annihilator_melee->add_action( "collapsing_star,if=active_enemies>1" );
  annihilator_melee->add_action( "call_action_list,name=reaps,if=buff.eradicate.up&active_enemies>1&action.reap.souls_consumed>=4+6*buff.moment_of_craving.up" );
  annihilator_melee->add_action( "call_action_list,name=reaps,if=!talent.eradicate&(active_enemies>1&!buff.metamorphosis.up&buff.moment_of_craving.up&talent.voidfall&(buff.voidfall_building.react<2|variable.ray_after_reap))" );
  annihilator_melee->add_action( "call_action_list,name=reaps,if=!talent.eradicate&(active_enemies>1&(buff.voidfall_spending.stack>=3&prev_gcd.1.void_ray|buff.voidfall_spending.react>=3))" );
  annihilator_melee->add_action( "call_action_list,name=reaps,if=active_enemies>1&buff.metamorphosis.up&talent.collapsing_star&(active_enemies>1|apex.1|buff.dark_matter.up|talent.star_fragments)&buff.collapsing_star_stacking.stack+action.reap.souls_consumed>=30&variable.wont_overcap_cstar&void_metamorphosis_base_drain_ps>35" );
  annihilator_melee->add_action( "soul_immolation,if=active_dot.soul_immolation=0&(!buff.metamorphosis.up|fury<void_metamorphosis_base_drain_ps)" );
  annihilator_melee->add_action( "devour" );
  annihilator_melee->add_action( "consume" );

  voidscarred_ranged->add_action( "voidblade,if=buff.void_metamorphosis_stack.at_max_stacks&talent.devourers_bite" );
  voidscarred_ranged->add_action( "metamorphosis,if=buff.eradicate.up|!talent.eradicate|active_enemies=1" );
  voidscarred_ranged->add_action( "devour,if=buff.soulburst.up&active_enemies=1" );
  voidscarred_ranged->add_action( "consume,if=buff.soulburst.up" );
  voidscarred_ranged->add_action( "collapsing_star,if=active_enemies=1&buff.collapsing_star_stacking.stack>=35" );
  voidscarred_ranged->add_action( "call_action_list,name=reaps,if=action.reap.souls_consumed>=4" );
  voidscarred_ranged->add_action( "call_action_list,name=reaps,if=fight_remains<=6&action.reap.souls_consumed>=1" );
  voidscarred_ranged->add_action( "void_ray,if=!buff.eradicate.up|!buff.moment_of_craving.up|set_bonus.midnight_season_2_4pc" );
  voidscarred_ranged->add_action( "collapsing_star,if=active_enemies>1" );
  voidscarred_ranged->add_action( "vengeful_retreat,if=buff.voidstep.up" );
  voidscarred_ranged->add_action( "reapers_toll,if=buff.voidsurge_reapers_toll.up" );
  voidscarred_ranged->add_action( "pierce_the_veil,if=buff.voidsurge_pierce_the_veil.up" );
  voidscarred_ranged->add_action( "soul_immolation,if=active_dot.soul_immolation=0&(!buff.metamorphosis.up|fury<void_metamorphosis_base_drain_ps)" );
  voidscarred_ranged->add_action( "devour" );
  voidscarred_ranged->add_action( "consume" );

  voidscarred_melee->add_action( "run_action_list,name=vsm_st,if=active_enemies=1" );
  voidscarred_melee->add_action( "run_action_list,name=vsm_meta,if=buff.metamorphosis.up" );
  voidscarred_melee->add_action( "run_action_list,name=vsm_out" );

  vsm_st->add_action( "pick_up_fragment,mode=nearest,type=all,use_off_gcd=1,line_cd=0.75,if=!buff.metamorphosis.up&!buff.void_metamorphosis_stack.at_max_stacks&buff.void_metamorphosis_stack.stack>=buff.void_metamorphosis_stack.max_stack-1" );
  vsm_st->add_action( "soul_immolation,if=active_dot.soul_immolation=0&fury<void_metamorphosis_base_drain_ps" );
  vsm_st->add_action( "devour,if=buff.soulburst.up" );
  vsm_st->add_action( "consume,if=buff.soulburst.up" );
  vsm_st->add_action( "voidblade,if=buff.void_metamorphosis_stack.at_max_stacks" );
  vsm_st->add_action( "vengeful_retreat,if=buff.void_metamorphosis_stack.at_max_stacks&cooldown.the_hunt.ready&!buff.metamorphosis.up" );
  vsm_st->add_action( "the_hunt,if=buff.void_metamorphosis_stack.at_max_stacks&buff.vengeful_retreat_movement.up" );
  vsm_st->add_action( "metamorphosis" );
  vsm_st->add_action( "the_hunt,if=!buff.metamorphosis.up&(fight_remains<15|buff.void_metamorphosis_stack.stack<buff.void_metamorphosis_stack.max_stack*0.25)" );
  vsm_st->add_action( "wait,sec=0.05,if=!buff.metamorphosis.up&!buff.void_metamorphosis_stack.at_max_stacks&buff.void_metamorphosis_stack.stack>=buff.void_metamorphosis_stack.max_stack-1&soul_fragments>=1" );
  vsm_st->add_action( "hungering_slash" );
  vsm_st->add_action( "reapers_toll,if=soul_fragments<=8" );
  vsm_st->add_action( "vengeful_retreat,if=buff.voidstep.up" );
  vsm_st->add_action( "pierce_the_veil" );
  vsm_st->add_action( "call_action_list,name=reaps,if=action.reap.souls_consumed>=4" );
  vsm_st->add_action( "predators_wake" );
  vsm_st->add_action( "void_ray,if=!buff.eradicate.up|!buff.moment_of_craving.up|set_bonus.midnight_season_2_4pc" );
  vsm_st->add_action( "soul_immolation,if=active_dot.soul_immolation=0&!buff.metamorphosis.up" );
  vsm_st->add_action( "devour" );
  vsm_st->add_action( "consume" );

  vsm_meta->add_action( "soul_immolation,if=variable.void_ray_count>=2&active_dot.soul_immolation=0&fury<=gcd.max*(void_metamorphosis_base_drain_ps-6)-6" );
  vsm_meta->add_action( "pick_up_fragment,mode=nearest,type=all,use_off_gcd=1,line_cd=0.35,if=variable.void_ray_count=1&cooldown.void_ray.remains<gcd.max*2&fury<void_metamorphosis_base_drain_ps*cooldown.void_ray.remains" );
  vsm_meta->add_action( "wait,sec=0.05,if=variable.void_ray_count>=2" );
  vsm_meta->add_action( "vengeful_retreat,use_off_gcd=1,if=buff.voidstep.up" );
  vsm_meta->add_action( "void_ray,if=variable.void_ray_count=0&!talent.eradicate" );
  vsm_meta->add_action( "eradicate,if=(buff.moment_of_craving.remains<gcd.max|cooldown.void_ray.remains<gcd.max)&!buff.soulburst.up" );
  vsm_meta->add_action( "void_ray,if=talent.eradicate&!buff.eradicate.up" );
  vsm_meta->add_action( "reapers_toll,if=buff.hungering_slash.remains<gcd.max*2" );
  vsm_meta->add_action( "devour,if=buff.soulburst.up" );
  vsm_meta->add_action( "reapers_toll,if=soul_fragments<=8" );
  vsm_meta->add_action( "pierce_the_veil,if=!buff.hungering_slash.up" );
  vsm_meta->add_action( "predators_wake,if=!buff.hungering_slash.up" );
  vsm_meta->add_action( "eradicate,if=action.reap.souls_consumed>=8&!buff.soulburst.up" );
  vsm_meta->add_action( "cull,if=action.reap.souls_consumed>=4&!buff.soulburst.up" );
  vsm_meta->add_action( "void_ray" );
  vsm_meta->add_action( "soul_immolation,if=active_dot.soul_immolation=0&fury<=gcd.max*(void_metamorphosis_base_drain_ps-6)-6&cooldown.void_ray.remains>gcd.max" );
  vsm_meta->add_action( "devour,if=fury<void_metamorphosis_base_drain_ps*(cooldown.void_ray.remains+3)" );

  vsm_out->add_action( "pick_up_fragment,mode=nearest,type=all,use_off_gcd=1,line_cd=0.3,if=!buff.void_metamorphosis_stack.at_max_stacks&buff.void_metamorphosis_stack.stack>=buff.void_metamorphosis_stack.max_stack-3" );
  vsm_out->add_action( "wait,sec=0.05,if=!buff.void_metamorphosis_stack.at_max_stacks&buff.void_metamorphosis_stack.stack>=buff.void_metamorphosis_stack.max_stack-1&time-action.pick_up_fragment.last_used<1.5" );
  vsm_out->add_action( "vengeful_retreat,use_off_gcd=1,if=buff.voidstep.up" );
  vsm_out->add_action( "voidblade,if=prev_gcd.1.void_ray" );
  vsm_out->add_action( "metamorphosis,if=buff.eradicate.up|!talent.eradicate|fight_remains<15" );
  vsm_out->add_action( "the_hunt" );
  vsm_out->add_action( "consume,if=buff.soulburst.up&buff.soulburst.remains<gcd.max" );
  vsm_out->add_action( "void_ray,if=talent.eradicate&!buff.eradicate.up" );
  vsm_out->add_action( "hungering_slash" );
  vsm_out->add_action( "soul_immolation,if=active_dot.soul_immolation=0" );
  vsm_out->add_action( "reap,if=action.reap.souls_consumed>=4&!buff.soulburst.up" );
  vsm_out->add_action( "eradicate,if=action.reap.souls_consumed>=4&!buff.soulburst.up" );
  vsm_out->add_action( "void_ray" );
  vsm_out->add_action( "consume" );

  variables->add_action( "variable,name=wont_overcap_cstar,op=set,value=!buff.metamorphosis.up|((buff.collapsing_star_stacking.stack+action.reap.souls_consumed)<=buff.collapsing_star_stacking.max_stack|!(talent.collapsing_star&(active_enemies>1|apex.1|buff.dark_matter.up|talent.star_fragments)))" );
  variables->add_action( "variable,name=wont_drop_meta,op=set,value=!buff.metamorphosis.up|!buff.moment_of_craving.up|(fury>void_metamorphosis_base_drain_ps+4*action.reap.souls_consumed+10*talent.scythes_embrace&buff.collapsing_star_stacking.stack+action.reap.souls_consumed>=30|!(talent.collapsing_star&(active_enemies>1|apex.1|buff.dark_matter.up|talent.star_fragments)))" );
  variables->add_action( "variable,name=ray_after_reap,op=set,value=fury+4*action.reap.souls_consumed+10*talent.scythes_embrace>=100" );
  variables->add_action( "variable,name=void_ray_count,op=reset,if=!buff.metamorphosis.up" );
  variables->add_action( "variable,name=void_ray_count,op=add,value=1,if=buff.metamorphosis.up&action.void_ray.last_used>variable.last_ray_mark" );
  variables->add_action( "variable,name=last_ray_mark,op=set,value=action.void_ray.last_used" );

  cooldowns->add_action( "invoke_external_buff,name=power_infusion,if=buff.metamorphosis.up&!buff.power_infusion.up" );
  cooldowns->add_action( "potion,if=buff.metamorphosis.up|fight_remains<=30" );
  cooldowns->add_action( "use_item,slot=trinket1,if=buff.metamorphosis.up&(variable.void_ray_count=0|hero_tree.annihilator)&(!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1|variable.trinket_2_exclude)&!variable.trinket_1_manual|trinket.1.proc.any_dps.duration>=fight_remains|fight_remains<=trinket.1.buff.any_dps.duration" );
  cooldowns->add_action( "use_item,slot=trinket2,if=buff.metamorphosis.up&(variable.void_ray_count=0|hero_tree.annihilator)&(!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2|variable.trinket_1_exclude)&!variable.trinket_2_manual|trinket.2.proc.any_dps.duration>=fight_remains|fight_remains<=trinket.2.buff.any_dps.duration" );
  cooldowns->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=!variable.trinket_1_buffs&!variable.trinket_1_manual&(variable.damage_trinket_priority=1|trinket.2.cooldown.remains|trinket.2.is.spymasters_web|trinket.2.cooldown.duration=0)&gcd.remains>0.1" );
  cooldowns->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=!variable.trinket_2_buffs&!variable.trinket_2_manual&(variable.damage_trinket_priority=2|trinket.1.cooldown.remains|trinket.1.is.spymasters_web|trinket.1.cooldown.duration=0)&gcd.remains>0.1" );
  cooldowns->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=trinket.1.is.font_of_venomous_rage&!buff.metamorphosis.up&buff.rolling_torment.up&gcd.remains>0.1" );
  cooldowns->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=trinket.2.is.font_of_venomous_rage&!buff.metamorphosis.up&buff.rolling_torment.up&gcd.remains>0.1" );
  cooldowns->add_action( "use_item,slot=trinket1,if=!variable.trinket_1_buffs&!variable.trinket_1_manual&(variable.damage_trinket_priority=1|trinket.2.cooldown.remains|trinket.2.is.spymasters_web|trinket.2.cooldown.duration=0)&!variable.trinket_1_ogcd_cast" );
  cooldowns->add_action( "use_item,slot=trinket2,if=!variable.trinket_2_buffs&!variable.trinket_2_manual&(variable.damage_trinket_priority=2|trinket.1.cooldown.remains|trinket.1.is.spymasters_web|trinket.1.cooldown.duration=0)&!variable.trinket_2_ogcd_cast" );

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

  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=tab_target_burning_wound,op=reset,default=1" );
  precombat->add_action( "variable,name=rg_ds,default=0,op=reset" );
  precombat->add_action( "variable,name=trinket1_special,value=trinket.1.is.algethar_puzzle_box" );
  precombat->add_action( "variable,name=trinket2_special,value=trinket.2.is.algethar_puzzle_box" );
  precombat->add_action( "variable,name=trinket1_crit,value=!variable.trinket1_special&trinket.1.has_cooldown&trinket.1.has_use_damage" );
  precombat->add_action( "variable,name=trinket2_crit,value=!variable.trinket2_special&trinket.2.has_cooldown&trinket.2.has_use_damage" );
  precombat->add_action( "variable,name=trinket1_steroids,value=!variable.trinket1_special&trinket.1.has_cooldown&trinket.1.has_use_buff" );
  precombat->add_action( "variable,name=trinket2_steroids,value=!variable.trinket2_special&trinket.2.has_cooldown&trinket.2.has_use_buff" );
  precombat->add_action( "immolation_aura" );

  default_->add_action( "auto_attack" );
  default_->add_action( "immolation_aura,if=talent.violent_transformation&talent.a_fire_inside&cooldown.metamorphosis.remains<gcd.max*3" );
  default_->add_action( "metamorphosis,if=(!talent.chaotic_transformation|!cooldown.blade_dance.up&cooldown.eye_beam.remains>8)&!action.death_sweep.demonsurge_available&!action.annihilation.demonsurge_available" );
  default_->add_action( "the_hunt,if=(!talent.eternal_hunt|cooldown.eye_beam.remains<10&cooldown.metamorphosis.remains>15|!cooldown.eye_beam.up&cooldown.metamorphosis.up|!hero_tree.felscarred)&!buff.reavers_glaive.up" );
  default_->add_action( "vengeful_retreat,use_off_gcd=1,if=gcd.remains<0.3&cooldown.metamorphosis.remains&cooldown.eye_beam.remains<gcd.max*0.3&!buff.initiative.up|cooldown.metamorphosis.up&cooldown.eye_beam.remains&cooldown.blade_dance.remains&!buff.eternal_hunt.up" );
  default_->add_action( "potion,if=cooldown.metamorphosis.up&(cooldown.eye_beam.up|!talent.chaotic_transformation)|fight_remains<=30" );
  default_->add_action( "use_items,if=cooldown.metamorphosis.up|cooldown.eye_beam.up|fight_remains<=20" );
  default_->add_action( "pick_up_fragment,type=all,use_off_gcd=1,if=fury<=40" );
  default_->add_action( "reavers_glaive,if=buff.rending_strike.down&buff.glaive_flurry.down" );
  default_->add_action( "immolation_aura,if=talent.a_fire_inside&talent.burning_wound&(charges=2|full_recharge_time<gcd.max*2)" );
  default_->add_action( "annihilation,if=buff.rending_strike.up&buff.glaive_flurry.down&debuff.reavers_mark.stack<2|buff.rending_strike.up&active_enemies>=2" );
  default_->add_action( "death_sweep,if=debuff.essence_break.up|action.death_sweep.demonsurge_available" );
  default_->add_action( "annihilation,if=action.annihilation.demonsurge_available" );
  default_->add_action( "eye_beam" );
  default_->add_action( "immolation_aura,,if=action.immolation_aura.demonsurge_available&buff.demonsurge.remains<gcd.max" );
  default_->add_action( "felblade,if=buff.inertia_trigger.up" );
  default_->add_action( "essence_break,if=cooldown.eye_beam.remains>4" );
  default_->add_action( "death_sweep" );
  default_->add_action( "blade_dance" );
  default_->add_action( "immolation_aura,if=active_enemies>1&(talent.a_fire_inside|talent.burning_wound)" );
  default_->add_action( "annihilation" );
  default_->add_action( "felblade,if=hero_tree.felscarred&fury<=100" );
  default_->add_action( "chaos_strike" );
  default_->add_action( "felblade" );
  default_->add_action( "immolation_aura" );
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
  action_priority_list_t* ar = p->get_action_priority_list( "ar" );
  action_priority_list_t* ar_quick_consume = p->get_action_priority_list( "ar_quick_consume" );
  action_priority_list_t* ar_fillers = p->get_action_priority_list( "ar_fillers" );
  action_priority_list_t* ar_glaive_cycle = p->get_action_priority_list( "ar_glaive_cycle" );
  action_priority_list_t* ar_glaive_cycle_filler = p->get_action_priority_list( "ar_glaive_cycle_filler" );
  action_priority_list_t* anni = p->get_action_priority_list( "anni" );
  action_priority_list_t* anni_cooldowns = p->get_action_priority_list( "anni_cooldowns" );
  action_priority_list_t* anni_voidfall_spending = p->get_action_priority_list( "anni_voidfall_spending" );
  action_priority_list_t* anni_meta_entry = p->get_action_priority_list( "anni_meta_entry" );
  action_priority_list_t* anni_pre_meta_spb = p->get_action_priority_list( "anni_pre_meta_spb" );
  action_priority_list_t* anni_voidfall_fishing = p->get_action_priority_list( "anni_voidfall_fishing" );
  action_priority_list_t* anni_generate_fury = p->get_action_priority_list( "anni_generate_fury" );
  action_priority_list_t* anni_filler_no_spend = p->get_action_priority_list( "anni_filler_no_spend" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );

  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "sigil_of_flame" );
  precombat->add_action( "sigil_of_spite,if=hero_tree.aldrachi_reaver|talent.soul_carver", "AR always. Anni only with Soul Carver (non-SC builds lose DPS from pre-pull frag misalignment)" );
  precombat->add_action( "immolation_aura" );

  default_->add_action( "variable,name=single_target,value=spell_targets.spirit_bomb=1" );
  default_->add_action( "variable,name=aoe,value=spell_targets.spirit_bomb>=3" );
  default_->add_action( "variable,name=execute,value=fight_remains<20" );
  default_->add_action( "variable,name=is_dungeon,value=fight_style.dungeonroute|fight_style.dungeonslice", "Dungeon Route" );
  default_->add_action( "cycling_variable,name=dung_pull_ttd,op=reset" );
  default_->add_action( "cycling_variable,name=dung_pull_ttd,op=max,value=target.time_to_die" );
  default_->add_action( "variable,name=dung_next_pull,value=variable.is_dungeon&raid_event.adds.exists&raid_event.pull.remains<12&(raid_event.adds.has_boss|raid_event.adds.count>=3)" );
  default_->add_action( "variable,name=dung_cd_ok,value=variable.execute|!variable.is_dungeon|(variable.dung_pull_ttd>12&!variable.dung_next_pull)", "Safe to use 40-60s CDs (SC, SoS, FD, buff trinkets)" );
  default_->add_action( "variable,name=dung_meta_ok,value=variable.execute|!variable.is_dungeon|(variable.dung_pull_ttd>(15-5*hero_tree.annihilator)&!variable.dung_next_pull)", "Stricter guard for Meta (2min CD) - Anni gets lower bar for UR proc windows" );
  default_->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_use_buff|(trinket.1.has_buff.agility|trinket.1.has_buff.mastery|trinket.1.has_buff.versatility|trinket.1.has_buff.haste|trinket.1.has_buff.crit|trinket.1.has_buff.attack_power)" );
  default_->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_use_buff|(trinket.2.has_buff.agility|trinket.2.has_buff.mastery|trinket.2.has_buff.versatility|trinket.2.has_buff.haste|trinket.2.has_buff.crit|trinket.2.has_buff.attack_power)" );
  default_->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs|variable.trinket_2_buffs&((trinket.2.proc.any_dps.duration)*trinket.2.proc.any_dps.default_value)>((trinket.1.proc.any_dps.duration)*trinket.1.proc.any_dps.default_value)", "Rank buff trinkets by total stat value (duration * proc value)" );
  default_->add_action( "variable,name=damage_trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&!variable.trinket_2_buffs&trinket.2.ilvl>=trinket.1.ilvl", "Non-buff damage trinkets: which slot has higher ilvl" );
  default_->add_action( "variable,name=fiery_demise_active,value=talent.fiery_demise&dot.fiery_brand.ticking" );
  default_->add_action( "variable,name=fire_cd_soon,value=cooldown.soul_carver.remains>?cooldown.fel_devastation.remains>?cooldown.sigil_of_spite.remains<(8+talent.charred_flesh.rank)" );
  default_->add_action( "variable,name=fragment_target,op=setif,value=5+apex.2,value_else=variable.fiery_demise_active*3+!variable.fiery_demise_active*(5-buff.metamorphosis.up),condition=hero_tree.aldrachi_reaver", "Fragment target: AR uses AotG scaling; Anni uses 3 during Brand, 4 in Meta, 5 baseline" );
  default_->add_action( "auto_attack" );
  default_->add_action( "retarget_auto_attack,target_if=min:debuff.reavers_mark.remains,if=hero_tree.aldrachi_reaver" );
  default_->add_action( "disrupt,if=target.debuff.casting.react" );
  default_->add_action( "infernal_strike,use_off_gcd=1" );
  default_->add_action( "demon_spikes,use_off_gcd=1,if=!buff.demon_spikes.up&in_combat" );
  default_->add_action( "run_action_list,name=ar,if=hero_tree.aldrachi_reaver" );
  default_->add_action( "run_action_list,name=anni,if=hero_tree.annihilator" );

  ar->add_action( "variable,name=frac_souls,value=2+buff.metamorphosis.up", "TTNG MODEL: GCDs until next glaive, accounting for SC, SoS, and passive frags" );
  ar->add_action( "variable,name=base_deficit,value=(20-buff.art_of_the_glaive.stack-soul_fragments.total)<?0" );
  ar->add_action( "variable,name=eff_recharge,value=cooldown.fracture.remains+(cooldown.fracture.charges>=2)*cooldown.fracture.duration" );
  ar->add_action( "variable,name=passive_per_sec,value=0.30+(talent.fallout&buff.immolation_aura.up)*0.30*spell_targets.spirit_bomb" );
  ar->add_action( "variable,name=fracs_base,value=variable.base_deficit%variable.frac_souls" );
  ar->add_action( "variable,name=fracs_base,op=ceil" );
  ar->add_action( "variable,name=base_gen_time,value=(variable.fracs_base>0)*((variable.fracs_base<=cooldown.fracture.charges)*variable.fracs_base*(1+apex.3)*gcd.max+(variable.fracs_base>cooldown.fracture.charges)*((cooldown.fracture.charges*(1+apex.3)*gcd.max<?variable.eff_recharge)+((variable.fracs_base-cooldown.fracture.charges-1)<?0)*cooldown.fracture.duration+gcd.max))" );
  ar->add_action( "variable,name=sc1,value=talent.soul_carver&cooldown.soul_carver.remains<variable.base_gen_time", "Pass 1: subtract SC (6 frags) and SoS (3 frags) if they arrive within the gen window" );
  ar->add_action( "variable,name=net1,value=(variable.base_deficit-variable.sc1*6)<?0" );
  ar->add_action( "variable,name=fracs1,value=variable.net1%variable.frac_souls" );
  ar->add_action( "variable,name=fracs1,op=ceil" );
  ar->add_action( "variable,name=gt1,value=(variable.fracs1>0)*((variable.fracs1<=cooldown.fracture.charges)*variable.fracs1*(1+apex.3)*gcd.max+(variable.fracs1>cooldown.fracture.charges)*((cooldown.fracture.charges*(1+apex.3)*gcd.max<?variable.eff_recharge)+((variable.fracs1-cooldown.fracture.charges-1)<?0)*cooldown.fracture.duration+gcd.max))" );
  ar->add_action( "variable,name=sos1,value=talent.sigil_of_spite&cooldown.sigil_of_spite.remains<variable.gt1" );
  ar->add_action( "variable,name=N1,value=(variable.net1-variable.sos1*3)<?0" );
  ar->add_action( "variable,name=fracs_np,value=variable.N1%variable.frac_souls" );
  ar->add_action( "variable,name=fracs_np,op=ceil" );
  ar->add_action( "variable,name=gt_np,value=(variable.fracs_np>0)*((variable.fracs_np<=cooldown.fracture.charges)*variable.fracs_np*(1+apex.3)*gcd.max+(variable.fracs_np>cooldown.fracture.charges)*((cooldown.fracture.charges*(1+apex.3)*gcd.max<?variable.eff_recharge)+((variable.fracs_np-cooldown.fracture.charges-1)<?0)*cooldown.fracture.duration+gcd.max))" );
  ar->add_action( "variable,name=T1,value=gcd.max+variable.gt_np+variable.sc1*gcd.max+variable.sos1*gcd.max", "T1: no-passive reference time (overhead GCD + fracture gen + SC/SoS cast GCDs)" );
  ar->add_action( "variable,name=sc_prop,value=(talent.soul_carver&cooldown.soul_carver.remains<variable.T1)*((1-cooldown.soul_carver.remains%variable.T1)<?0)", "Pass 2: proportional SC credit (scales by how early SC arrives relative to T1)" );
  ar->add_action( "variable,name=net_p,value=(variable.base_deficit-6*variable.sc_prop)<?0" );
  ar->add_action( "variable,name=fracs_p,value=variable.net_p%variable.frac_souls" );
  ar->add_action( "variable,name=fracs_p,op=ceil" );
  ar->add_action( "variable,name=gt_p,value=(variable.fracs_p>0)*((variable.fracs_p<=cooldown.fracture.charges)*variable.fracs_p*(1+apex.3)*gcd.max+(variable.fracs_p>cooldown.fracture.charges)*((cooldown.fracture.charges*(1+apex.3)*gcd.max<?variable.eff_recharge)+((variable.fracs_p-cooldown.fracture.charges-1)<?0)*cooldown.fracture.duration+gcd.max))" );
  ar->add_action( "variable,name=sos_p,value=talent.sigil_of_spite&cooldown.sigil_of_spite.remains<variable.gt_p" );
  ar->add_action( "variable,name=N_p,value=(variable.net_p-variable.sos_p*3)<?0" );
  ar->add_action( "variable,name=adj2,value=(variable.N_p-variable.passive_per_sec*variable.T1)<?0" );
  ar->add_action( "variable,name=fracs_p2,value=variable.adj2%variable.frac_souls" );
  ar->add_action( "variable,name=fracs_p2,op=ceil" );
  ar->add_action( "variable,name=gt_p2,value=(variable.fracs_p2>0)*((variable.fracs_p2<=cooldown.fracture.charges)*variable.fracs_p2*(1+apex.3)*gcd.max+(variable.fracs_p2>cooldown.fracture.charges)*((cooldown.fracture.charges*(1+apex.3)*gcd.max<?variable.eff_recharge)+((variable.fracs_p2-cooldown.fracture.charges-1)<?0)*cooldown.fracture.duration+gcd.max))" );
  ar->add_action( "variable,name=T2,value=gcd.max+variable.gt_p2+(variable.sc_prop>0)*gcd.max+variable.sos_p*gcd.max", "T2: passive-adjusted estimate, averaged with T1 to tame oscillation" );
  ar->add_action( "variable,name=T_avg,value=(variable.T1+variable.T2)%2" );
  ar->add_action( "variable,name=adj_f,value=(variable.N_p-variable.passive_per_sec*variable.T_avg)<?0", "Final pass: use T_avg as the passive window for the definitive fracture count" );
  ar->add_action( "variable,name=fracs_f,value=variable.adj_f%variable.frac_souls" );
  ar->add_action( "variable,name=fracs_f,op=ceil" );
  ar->add_action( "variable,name=gt_f,value=(variable.fracs_f>0)*((variable.fracs_f<=cooldown.fracture.charges)*variable.fracs_f*(1+apex.3)*gcd.max+(variable.fracs_f>cooldown.fracture.charges)*((cooldown.fracture.charges*(1+apex.3)*gcd.max<?variable.eff_recharge)+((variable.fracs_f-cooldown.fracture.charges-1)<?0)*cooldown.fracture.duration+gcd.max))" );
  ar->add_action( "variable,name=time_to_next_glaive,value=gcd.max+variable.gt_f+(variable.sc_prop>0)*gcd.max+variable.sos_p*gcd.max" );
  ar->add_action( "variable,name=passive_floor,value=variable.N_p%(variable.passive_per_sec*gcd.max)", "When passives alone cover the deficit, round up to GCD boundaries" );
  ar->add_action( "variable,name=passive_floor,op=ceil" );
  ar->add_action( "variable,name=passive_floor,value=variable.passive_floor*gcd.max" );
  ar->add_action( "variable,name=time_to_next_glaive,value=variable.time_to_next_glaive<?(variable.fracs_f=0&variable.N_p>0)*variable.passive_floor" );
  ar->add_action( "variable,name=time_to_next_rm_application,value=variable.time_to_next_glaive+2*gcd.max", "RM application happens 2 GCDs into the cycle (RG -> SC -> Frac applies mark)" );
  ar->add_action( "variable,name=rm_remains,value=(variable.last_rm_applied>0)*(20-(time-variable.last_rm_applied))<?0", "Manual RM tracking (debuff.remains unreliable with async stacks)" );
  ar->add_action( "variable,name=prio_slashes,value=variable.aoe|variable.execute|(variable.rm_remains>0&variable.last_refresh_at>variable.last_slash_at)", "Alternate slash and refresh cycles" );
  ar->add_action( "variable,name=last_slash_at,op=setif,value=time,value_else=variable.last_slash_at,condition=buff.reavers_glaive.up&!variable.cycle_recorded&variable.prio_slashes", "Record cycle type once when RG first stored (resets when RG buff drops)" );
  ar->add_action( "variable,name=last_refresh_at,op=setif,value=time,value_else=variable.last_refresh_at,condition=buff.reavers_glaive.up&!variable.cycle_recorded&!variable.prio_slashes" );
  ar->add_action( "variable,name=cycle_recorded,value=buff.reavers_glaive.up" );
  ar->add_action( "variable,name=rg_imminent,value=(buff.reavers_glaive.up&(variable.execute|variable.rm_remains<=variable.time_to_next_rm_application|buff.art_of_the_glaive.stack+soul_fragments>=(20-variable.frac_souls)))|(buff.art_of_the_glaive.stack+soul_fragments>=20)|(soul_fragments>=6&buff.art_of_the_glaive.stack>=(20-variable.frac_souls)&cooldown.fracture.charges>=1)", "RG imminent: stored and ready to fire, at AotG cap, or one consume from overflow" );
  ar->add_action( "felblade,if=prev_gcd.1.vengeful_retreat|prev_off_gcd.vengeful_retreat" );
  ar->add_action( "metamorphosis,use_off_gcd=1,if=buff.untethered_rage.up|(!buff.metamorphosis.up&variable.dung_meta_ok)", "UR proc meta fires unconditionally; hardcast gates on dungeon TTD" );
  ar->add_action( "call_action_list,name=trinkets", "Stat buff trinkets before RG so the buff covers the glaive cycle" );
  ar->add_action( "reavers_glaive,if=buff.reavers_glaive.up&!buff.rending_strike.up&!buff.glaive_flurry.up&(variable.execute|variable.prio_slashes|variable.rm_remains<=0|variable.rm_remains<10|buff.art_of_the_glaive.stack+soul_fragments>=(20-variable.frac_souls))", "Fire stored RG: execute, mark expired or aging, or about to overflow AotG" );
  ar->add_action( "call_action_list,name=ar_glaive_cycle,if=buff.rending_strike.up|buff.glaive_flurry.up|prev_gcd.1.reavers_glaive" );
  ar->add_action( "fiery_brand,if=charges>=2|!variable.fiery_demise_active|variable.execute", "Fiery brand: overcapped charges, or setup for fiery demise window" );
  ar->add_action( "sigil_of_spite,if=variable.dung_cd_ok&!buff.reavers_glaive.up&!buff.rending_strike.up&!buff.glaive_flurry.up", "SoS for frags, skip during glaive cycle" );
  ar->add_action( "call_action_list,name=ar_quick_consume,if=!buff.reavers_glaive.up&!buff.rending_strike.up&!buff.glaive_flurry.up&(buff.art_of_the_glaive.stack+soul_fragments>=20|(variable.aoe&soul_fragments>=6))", "Emergency consume: AotG overflow or frag cap in aoe" );
  ar->add_action( "immolation_aura,if=in_combat" );
  ar->add_action( "fel_devastation,if=variable.dung_cd_ok&fury>85&(soul_fragments.inactive>1|variable.aoe)&(!variable.rg_imminent|variable.aoe)", "FD: high fury, in-flight frags, not near RG. aoe skips the RG check" );
  ar->add_action( "sigil_of_flame" );
  ar->add_action( "soul_carver,if=variable.dung_cd_ok&(variable.fiery_demise_active|(variable.rm_remains<7&buff.art_of_the_glaive.stack+soul_fragments<20)|variable.execute)", "SC: 6 frags, prefer fiery demise. OK when mark is aging or in execute" );
  ar->add_action( "call_action_list,name=ar_fillers" );

  ar_quick_consume->add_action( "soul_cleave,if=soul_fragments<(3-variable.aoe)", "Quick consume: rush to AotG 20. aoe uses lower SpB threshold" );
  ar_quick_consume->add_action( "spirit_bomb,if=soul_fragments>=(3-variable.aoe)" );
  ar_quick_consume->add_action( "soul_cleave,if=!variable.aoe" );

  ar_fillers->add_action( "spirit_bomb,if=soul_fragments>=(variable.fragment_target-variable.aoe)", "Fillers outside glaive cycles. aoe lowers SpB threshold by 1" );
  ar_fillers->add_action( "immolation_aura,if=variable.time_to_next_glaive>3*gcd.max" );
  ar_fillers->add_action( "felblade,if=cooldown.spirit_bomb.remains<gcd.max&soul_fragments.total>=variable.fragment_target&fury<40" );
  ar_fillers->add_action( "vengeful_retreat,use_off_gcd=1,if=!cooldown.felblade.up&talent.unhindered_assault&cooldown.spirit_bomb.remains<gcd.max&soul_fragments.total>=variable.fragment_target&fury<40" );
  ar_fillers->add_action( "soul_cleave,if=((soul_fragments>=5&!variable.aoe)|soul_fragments<=1|fury.deficit<30)&(fury>=2*action.soul_cleave.cost|cooldown.fracture.charges>=1|cooldown.fracture.remains<=gcd.max)&(!buff.rending_strike.up|!buff.glaive_flurry.up|!variable.prio_slashes)", "SC: aoe skips frag>=5 trigger to save frags for SpB" );
  ar_fillers->add_action( "sigil_of_flame,if=variable.aoe" );
  ar_fillers->add_action( "fracture,if=buff.metamorphosis.up|full_recharge_time<gcd.max|buff.warblades_hunger.stack>=4" );
  ar_fillers->add_action( "immolation_aura,if=!variable.is_dungeon|in_combat" );
  ar_fillers->add_action( "sigil_of_flame" );
  ar_fillers->add_action( "soul_cleave,if=(fury>=2*action.soul_cleave.cost|cooldown.fracture.charges>=1|cooldown.fracture.remains<=gcd.max)&(!buff.rending_strike.up|!buff.glaive_flurry.up|!variable.prio_slashes)", "Unconditional SC fallback with same guards" );
  ar_fillers->add_action( "fracture" );
  ar_fillers->add_action( "felblade" );
  ar_fillers->add_action( "vengeful_retreat,use_off_gcd=1,if=talent.unhindered_assault&!cooldown.felblade.up" );
  ar_fillers->add_action( "soul_carver" );
  ar_fillers->add_action( "fel_devastation" );
  ar_fillers->add_action( "throw_glaive" );

  ar_glaive_cycle->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=!variable.trinket_1_buffs&(variable.damage_trinket_priority=1|trinket.2.cooldown.remains|trinket.2.cooldown.duration=0)&gcd.remains>0.1", "GLAIVE CYCLE: alternate RS+GF buffs after RG. Slash = frac first, refresh = SC first" );
  ar_glaive_cycle->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=!variable.trinket_2_buffs&(variable.damage_trinket_priority=2|trinket.1.cooldown.remains|trinket.1.cooldown.duration=0)&gcd.remains>0.1" );
  ar_glaive_cycle->add_action( "call_action_list,name=ar_glaive_cycle_filler,if=(variable.prio_slashes&((cooldown.fracture.charges<1&buff.rending_strike.up&buff.glaive_flurry.up)|fury<10))|(!variable.prio_slashes&((buff.rending_strike.up&buff.glaive_flurry.up&fury<35)|(buff.rending_strike.up&!buff.glaive_flurry.up&cooldown.fracture.charges<1)))", "Fill GCD when waiting for fracture charge (slash) or fury (refresh)" );
  ar_glaive_cycle->add_action( "potion,use_off_gcd=1" );
  ar_glaive_cycle->add_action( "invoke_external_buff,name=power_infusion" );
  ar_glaive_cycle->add_action( "variable,name=last_rm_applied,value=time,if=buff.rending_strike.up", "Record RM application time when fracture is about to consume RS" );
  ar_glaive_cycle->add_action( "fracture,if=buff.rending_strike.up&buff.glaive_flurry.up&variable.prio_slashes", "Slash: fracture first when both buffs up (applies 1-stack RM + triggers slash damage)" );
  ar_glaive_cycle->add_action( "soul_cleave,if=buff.rending_strike.up&buff.glaive_flurry.up&!variable.prio_slashes", "Refresh: SC first when both buffs up (subsequent fracture gets 3-stack RM)" );
  ar_glaive_cycle->add_action( "fracture,if=buff.rending_strike.up&!buff.glaive_flurry.up", "Single-buff continuation" );
  ar_glaive_cycle->add_action( "soul_cleave,if=buff.glaive_flurry.up&!buff.rending_strike.up" );
  ar_glaive_cycle->add_action( "call_action_list,name=ar_glaive_cycle_filler" );

  ar_glaive_cycle_filler->add_action( "spirit_bomb,if=fury>75&soul_fragments>=variable.fragment_target", "Glaive cycle filler: non-consuming actions while waiting for resources" );
  ar_glaive_cycle_filler->add_action( "immolation_aura" );
  ar_glaive_cycle_filler->add_action( "fel_devastation,if=fury>=85" );
  ar_glaive_cycle_filler->add_action( "sigil_of_flame" );
  ar_glaive_cycle_filler->add_action( "felblade" );
  ar_glaive_cycle_filler->add_action( "soul_carver" );
  ar_glaive_cycle_filler->add_action( "vengeful_retreat,use_off_gcd=1,if=talent.unhindered_assault&!cooldown.felblade.up" );
  ar_glaive_cycle_filler->add_action( "soul_cleave,if=!buff.glaive_flurry.up", "SC only when GF already consumed (safe during slash cycles)" );
  ar_glaive_cycle_filler->add_action( "throw_glaive" );

  anni->add_action( "variable,name=anni_meta_entry_time,value=3*gcd.max", "Pre-meta setup window, typically ~3 GCDs" );
  anni->add_action( "potion,use_off_gcd=1,if=variable.execute&(!variable.is_dungeon|in_boss_encounter)" );
  anni->add_action( "invoke_external_buff,name=power_infusion,if=buff.voidfall_spending.stack=3|variable.execute" );
  anni->add_action( "call_action_list,name=anni_voidfall_spending,if=buff.voidfall_spending.up" );
  anni->add_action( "call_action_list,name=anni_voidfall_fishing,if=buff.voidfall_building.stack>=2&!buff.voidfall_spending.up" );
  anni->add_action( "call_action_list,name=anni_meta_entry,if=buff.untethered_rage.up|(variable.dung_meta_ok&cooldown.metamorphosis.remains<variable.anni_meta_entry_time)", "Prepare to enter hardcast meta (UR procs enter unconditionally)" );
  anni->add_action( "call_action_list,name=anni_cooldowns,if=variable.dung_cd_ok&(!talent.fiery_demise|variable.fiery_demise_active|cooldown.fiery_brand.remains>20|variable.execute)", "Use cooldowns, reserving at least one for the next meta" );
  anni->add_action( "fiery_brand,if=charges>=2|(!variable.fiery_demise_active&(!talent.fiery_demise|variable.fire_cd_soon|variable.execute))" );
  anni->add_action( "fracture,if=full_recharge_time<gcd.max" );
  anni->add_action( "immolation_aura,if=(talent.fallout&variable.aoe)|(talent.charred_flesh&variable.fiery_demise_active)", "Priority IA: Fallout generates frags in aoe, Charred Flesh extends brand" );
  anni->add_action( "spirit_bomb,if=soul_fragments>=variable.fragment_target" );
  anni->add_action( "immolation_aura" );
  anni->add_action( "sigil_of_flame" );
  anni->add_action( "fracture,if=soul_fragments.total<=4|fury<40" );
  anni->add_action( "soul_cleave,if=soul_fragments<=1|fury.deficit<=15" );
  anni->add_action( "soul_cleave,if=!(apex.3&!buff.untethered_rage.up&buff.seething_anger.stack>=10)&!cooldown.metamorphosis.up" );
  anni->add_action( "fracture" );
  anni->add_action( "felblade" );
  anni->add_action( "throw_glaive" );

  anni_cooldowns->add_action( "spirit_bomb,if=soul_fragments>=variable.fragment_target" );
  anni_cooldowns->add_action( "soul_carver,if=soul_fragments<=3" );
  anni_cooldowns->add_action( "sigil_of_spite,if=soul_fragments<=2+talent.soul_sigils" );
  anni_cooldowns->add_action( "fel_devastation" );
  anni_cooldowns->add_action( "call_action_list,name=anni_generate_fury,if=cooldown.fel_devastation.up&fury<50" );

  anni_voidfall_spending->add_action( "fiery_brand,if=charges>=2|!variable.fiery_demise_active" );
  anni_voidfall_spending->add_action( "felblade,if=buff.voidfall_spending.stack=buff.voidfall_spending.max_stack&cooldown.spirit_bomb.ready&soul_fragments.total>=variable.fragment_target&fury>=30&fury<45&cooldown.fracture.charges>=1", "Felblade to bridge fury into fracture bridge range (30+15=45)" );
  anni_voidfall_spending->add_action( "fracture,if=buff.voidfall_spending.stack=buff.voidfall_spending.max_stack&cooldown.spirit_bomb.ready&soul_fragments.total>=variable.fragment_target&fury>=45&fury<75", "Fracture to bridge fury for SB+SC combo when 1 fracture away (45+30=75)" );
  anni_voidfall_spending->add_action( "soul_cleave,if=cooldown.spirit_bomb.remains>gcd.max*4" );
  anni_voidfall_spending->add_action( "spirit_bomb,if=soul_fragments>=variable.fragment_target" );
  anni_voidfall_spending->add_action( "felblade,if=(fury<40&cooldown.spirit_bomb.remains<=gcd.max)|(fury<25&cooldown.spirit_bomb.remains>gcd.max)" );
  anni_voidfall_spending->add_action( "immolation_aura,if=(fury<40&cooldown.spirit_bomb.remains<=gcd.max)|(fury<25&cooldown.spirit_bomb.remains>gcd.max)" );
  anni_voidfall_spending->add_action( "soul_carver,if=(cooldown.spirit_bomb.remains<=gcd.max)&soul_fragments.total<variable.fragment_target&(!talent.sigil_of_spite|!action.sigil_of_spite.placed)" );
  anni_voidfall_spending->add_action( "fel_devastation,if=(cooldown.spirit_bomb.remains<=gcd.max)&soul_fragments.total<variable.fragment_target&(!talent.sigil_of_spite|!action.sigil_of_spite.placed)" );
  anni_voidfall_spending->add_action( "immolation_aura,if=variable.aoe&(cooldown.spirit_bomb.remains<=gcd.max)&soul_fragments.total<variable.fragment_target&(!talent.sigil_of_spite|!action.sigil_of_spite.placed)" );
  anni_voidfall_spending->add_action( "sigil_of_spite,if=(cooldown.spirit_bomb.remains<=gcd.max)&soul_fragments.total<variable.fragment_target" );
  anni_voidfall_spending->add_action( "call_action_list,name=anni_filler_no_spend" );

  anni_meta_entry->add_action( "potion,use_off_gcd=1,if=(variable.fiery_demise_active|variable.execute)&(!variable.is_dungeon|in_boss_encounter)" );
  anni_meta_entry->add_action( "call_action_list,name=trinkets" );
  anni_meta_entry->add_action( "invoke_external_buff,name=power_infusion" );
  anni_meta_entry->add_action( "metamorphosis,use_off_gcd=1,if=gcd.remains=0&buff.untethered_rage.up&!buff.voidfall_spending.up" );
  anni_meta_entry->add_action( "call_action_list,name=anni_pre_meta_spb,if=cooldown.spirit_bomb.remains<variable.anni_meta_entry_time&soul_fragments.total<variable.fragment_target" );
  anni_meta_entry->add_action( "fiery_brand,if=charges>=2|!variable.fiery_demise_active" );
  anni_meta_entry->add_action( "sigil_of_spite,if=soul_fragments>=variable.fragment_target&cooldown.spirit_bomb.up&cooldown.metamorphosis.up" );
  anni_meta_entry->add_action( "spirit_bomb,if=soul_fragments>=variable.fragment_target&fury>=60" );
  anni_meta_entry->add_action( "sigil_of_spite,if=soul_fragments.total<variable.fragment_target" );
  anni_meta_entry->add_action( "call_action_list,name=anni_generate_fury,if=fury<75&cooldown.metamorphosis.up&cooldown.spirit_bomb.remains>gcd.max*3", "Pool to 75 fury for SB+SC voidfall combo after meta entry" );
  anni_meta_entry->add_action( "metamorphosis,use_off_gcd=1,if=variable.dung_meta_ok&gcd.remains=0&cooldown.spirit_bomb.remains>gcd.max*3&(soul_fragments.total>=variable.fragment_target|(talent.sigil_of_spite&action.sigil_of_spite.placed))" );
  anni_meta_entry->add_action( "call_action_list,name=anni_filler_no_spend" );

  anni_pre_meta_spb->add_action( "fracture" );
  anni_pre_meta_spb->add_action( "immolation_aura,if=variable.aoe" );
  anni_pre_meta_spb->add_action( "fiery_brand,if=charges>=2|!variable.fiery_demise_active" );
  anni_pre_meta_spb->add_action( "soul_carver,if=(cooldown.soul_carver.up+cooldown.sigil_of_spite.up+cooldown.fel_devastation.up)>=2" );
  anni_pre_meta_spb->add_action( "fel_devastation,if=(cooldown.soul_carver.up+cooldown.sigil_of_spite.up+cooldown.fel_devastation.up)>=2" );
  anni_pre_meta_spb->add_action( "felblade" );
  anni_pre_meta_spb->add_action( "call_action_list,name=anni_filler_no_spend" );

  anni_voidfall_fishing->add_action( "fracture" );
  anni_voidfall_fishing->add_action( "call_action_list,name=anni_generate_fury,if=cooldown.fracture.charges_fractional>=0.75" );

  anni_generate_fury->add_action( "immolation_aura" );
  anni_generate_fury->add_action( "sigil_of_flame" );
  anni_generate_fury->add_action( "felblade" );
  anni_generate_fury->add_action( "fracture" );

  anni_filler_no_spend->add_action( "soul_cleave,if=soul_fragments=0&!action.sigil_of_flame.placed&(!talent.sigil_of_spite|(talent.sigil_of_spite&!action.sigil_of_spite.placed))&!prev_gcd.2.soul_carver" );
  anni_filler_no_spend->add_action( "immolation_aura" );
  anni_filler_no_spend->add_action( "sigil_of_flame" );
  anni_filler_no_spend->add_action( "felblade" );
  anni_filler_no_spend->add_action( "fracture,if=!buff.voidfall_spending.up" );
  anni_filler_no_spend->add_action( "soul_carver,if=(!talent.sigil_of_spite|(talent.sigil_of_spite&!action.sigil_of_spite.placed))" );
  anni_filler_no_spend->add_action( "fel_devastation,if=(!talent.sigil_of_spite|(talent.sigil_of_spite&!action.sigil_of_spite.placed))" );
  anni_filler_no_spend->add_action( "sigil_of_spite" );
  anni_filler_no_spend->add_action( "fracture" );
  anni_filler_no_spend->add_action( "throw_glaive" );

  trinkets->add_action( "use_item,slot=trinket1,if=variable.trinket_1_buffs&variable.dung_cd_ok&(!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1)" );
  trinkets->add_action( "use_item,slot=trinket2,if=variable.trinket_2_buffs&variable.dung_cd_ok&(!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2)" );
  trinkets->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=!variable.trinket_1_buffs&(variable.damage_trinket_priority=1|trinket.2.cooldown.remains|trinket.2.cooldown.duration=0)&gcd.remains>0.1", "Non-buff on-use trinkets (direct damage): fire on cooldown, off-GCD" );
  trinkets->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=!variable.trinket_2_buffs&(variable.damage_trinket_priority=2|trinket.1.cooldown.remains|trinket.1.cooldown.duration=0)&gcd.remains>0.1" );
  trinkets->add_action( "use_item,slot=trinket1,if=variable.execute", "End of fight: dump everything" );
  trinkets->add_action( "use_item,slot=trinket2,if=variable.execute" );
}
//vengeance_apl_end
// clang-format on

// clang-format off
//vengeance_ptr_apl_start
//vengeance_ptr_apl_end
// clang-format on

}  // namespace demon_hunter_apl
