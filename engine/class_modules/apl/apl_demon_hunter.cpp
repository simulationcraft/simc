#include "class_modules/apl/apl_demon_hunter.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace demon_hunter_apl
{

std::string potion( const player_t* p )
{
  return ( p->true_level >= 71 ) ? "tempered_potion_3" : "elemental_potion_of_ultimate_power_3";
}

std::string flask_havoc( const player_t* p )
{
  return ( p->true_level >= 71 ) ? "flask_of_alchemical_chaos_3" : "iced_phial_of_corrupting_rage_3";
}

std::string flask_vengeance( const player_t* p )
{
  return ( p->true_level >= 71 ) ? "flask_of_alchemical_chaos_3" : "iced_phial_of_corrupting_rage_3";
}

std::string food_havoc( const player_t* p )
{
  return ( p->true_level >= 71 ) ? "authentic_undermine_clam_chowder" : "fated_fortune_cookie";
}

std::string food_vengeance( const player_t* p )
{
  return ( p->true_level >= 71 ) ? "feast_of_the_divine_day" : "feisty_fish_sticks";
}

std::string rune( const player_t* p )
{
  return ( p->true_level >= 71 ) ? "crystallized" : "draconic";
}

std::string temporary_enchant_havoc( const player_t* p )
{
  return ( p->true_level >= 71 ) ? "main_hand:algari_mana_oil_3/off_hand:algari_mana_oil_3"
                                 : "main_hand:buzzing_rune_3/off_hand:buzzing_rune_3";
}

std::string temporary_enchant_vengeance( const player_t* p )
{
  return ( p->true_level >= 71 ) ? "main_hand:ironclaw_whetstone_3/off_hand:ironclaw_whetstone_3"
                                 : "main_hand:buzzing_rune_3/off_hand:buzzing_rune_3";
}

//havoc_apl_start
void havoc( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* ar = p->get_action_priority_list( "ar" );
  action_priority_list_t* ar_cooldown = p->get_action_priority_list( "ar_cooldown" );
  action_priority_list_t* ar_fel_barrage = p->get_action_priority_list( "ar_fel_barrage" );
  action_priority_list_t* ar_meta = p->get_action_priority_list( "ar_meta" );
  action_priority_list_t* ar_opener = p->get_action_priority_list( "ar_opener" );
  action_priority_list_t* fs = p->get_action_priority_list( "fs" );
  action_priority_list_t* fs_cooldown = p->get_action_priority_list( "fs_cooldown" );
  action_priority_list_t* fs_fel_barrage = p->get_action_priority_list( "fs_fel_barrage" );
  action_priority_list_t* fs_meta = p->get_action_priority_list( "fs_meta" );
  action_priority_list_t* fs_opener = p->get_action_priority_list( "fs_opener" );

  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=trinket1_steroids,value=trinket.1.has_cooldown&trinket.1.has_stat.any_dps&!trinket.1.is.improvised_seaforium_pacemaker", "Variables for on use trinkets and filtering out Seaforium" );
  precombat->add_action( "variable,name=trinket2_steroids,value=trinket.2.has_cooldown&trinket.2.has_stat.any_dps&!trinket.2.is.improvised_seaforium_pacemaker" );
  precombat->add_action( "variable,name=trinket1_crit,value=trinket.1.is.mad_queens_mandate|trinket.1.is.junkmaestros_mega_magnet|trinket.1.is.geargrinders_spare_keys|trinket.1.is.ravenous_honey_buzzer|trinket.1.is.grim_codex|trinket.1.is.ratfang_toxin|trinket.1.is.blastmaster3000|trinket.1.is.cursed_stone_idol|trinket.1.is.perfidious_projector|trinket.1.is.chaotic_nethergate", "Blacklist for trinkets to hold trinket cooldowns for Initiative and Necessary Strike line-up outside standard trinket implementation  TODO fix to work off generic conditions instead of specifying individual trinkets for futureproof" );
  precombat->add_action( "variable,name=trinket2_crit,value=trinket.2.is.mad_queens_mandate|trinket.2.is.junkmaestros_mega_magnet|trinket.2.is.geargrinders_spare_keys|trinket.2.is.ravenous_honey_buzzer|trinket.2.is.grim_codex|trinket.2.is.ratfang_toxin|trinket.2.is.blastmaster3000|trinket.2.is.cursed_stone_idol|trinket.2.is.perfidious_projector|trinket.2.is.chaotic_nethergate" );
  precombat->add_action( "variable,name=fs_tier34_2piece,value=set_bonus.thewarwithin_season_3_2pc" );
  precombat->add_action( "variable,name=rg_ds,default=0,op=reset" );
  precombat->add_action( "sigil_of_flame" );
  precombat->add_action( "immolation_aura" );

  default_->add_action( "auto_attack,if=!buff.out_of_range.up", "Default actions regardless of hero tree" );
  default_->add_action( "disrupt" );
  default_->add_action( "retarget_auto_attack,line_cd=1,target_if=min:debuff.burning_wound.remains,if=talent.burning_wound&talent.demon_blades&active_dot.burning_wound<(spell_targets>?3)", "Spread Burning Wounds for uptime in multitarget scenarios" );
  default_->add_action( "retarget_auto_attack,line_cd=1,target_if=min:!target.is_boss,if=talent.burning_wound&talent.demon_blades&active_dot.burning_wound=(spell_targets>?3)" );
  default_->add_action( "variable,name=fury_gen,op=set,value=talent.demon_blades*(1%(2.6*attack_haste)*((talent.demonsurge&buff.metamorphosis.up)*3+12))+buff.immolation_aura.stack*6+buff.tactical_retreat.up*10", "Fury generated per second" );
  default_->add_action( "variable,name=trinket_pacemaker_proc,value=trinket.1.is.improvised_seaforium_pacemaker&trinket.1.stat.crit.up|trinket.2.is.improvised_seaforium_pacemaker&trinket.2.stat.crit.up|!equipped.improvised_seaforium_pacemaker", "Special check for Seaforium Pacemaker buff being active for Magnet Synching" );
  default_->add_action( "variable,name=tier33_4piece,value=(buff.initiative.up|!talent.initiative|buff.necessary_sacrifice.stack>=5&buff.necessary_sacrifice.remains<0.5+cooldown.vengeful_retreat.remains)&(buff.necessary_sacrifice.up|!set_bonus.thewarwithin_season_2_4pc|cooldown.eye_beam.remains+2>buff.initiative.remains)", "Tier 33 tier set check for trinket lineups withs Necessary Sacrifice" );
  default_->add_action( "variable,name=tier33_4piece_magnet,value=(buff.initiative.up|!talent.initiative)&(buff.necessary_sacrifice.up|!set_bonus.thewarwithin_season_2_4pc)&variable.trinket_pacemaker_proc&(trinket.1.is.junkmaestros_mega_magnet&(!trinket.2.has_cooldown|trinket.2.cooldown.remains>20))|(trinket.2.is.junkmaestros_mega_magnet&(!trinket.1.has_cooldown|trinket.1.cooldown.remains>20))", "Tier 33 tier set special case check for magnet due to being able to hold" );
  default_->add_action( "variable,name=double_on_use,value=!equipped.signet_of_the_priory&!equipped.house_of_cards&!equipped.funhouse_lens&!equipped.cursed_stone_idol&!equipped.lily_of_the_eternal_weave&!equipped.arazs_ritual_forge&!equipped.unyielding_netherprism|(trinket.1.is.house_of_cards|trinket.1.is.signet_of_the_priory|trinket.1.is.funhouse_lens|trinket.1.is.cursed_stone_idol|trinket.1.is.lily_of_the_eternal_weave|trinket.1.is.arazs_ritual_forge)&trinket.1.cooldown.remains>20|(trinket.2.is.house_of_cards|trinket.2.is.signet_of_the_priory|trinket.2.is.funhouse_lens|trinket.2.is.cursed_stone_idol|trinket.2.is.lily_of_the_eternal_weave|trinket.2.is.arazs_ritual_forge)&trinket.2.cooldown.remains>20|equipped.unyielding_netherprism&(buff.latent_energy.stack<10|cooldown.metamorphosis.remains>20)", "Double on use trinket holding for using a stat cooldown trinket and an on use damage trinket" );
  default_->add_action( "run_action_list,name=ar,if=hero_tree.aldrachi_reaver", "Separate actionlists for each hero tree" );
  default_->add_action( "run_action_list,name=fs,if=hero_tree.felscarred" );

  ar->add_action( "variable,name=rg_inc,op=set,value=buff.rending_strike.down&buff.glaive_flurry.up&cooldown.blade_dance.up&gcd.remains=0|variable.rg_inc&prev_gcd.1.death_sweep", "Aldrachi Reaver" );
  ar->add_action( "cycling_variable,name=pull_remains,op=reset" );
  ar->add_action( "cycling_variable,name=pull_remains,op=max,value=target.time_to_die" );
  ar->add_action( "retarget_auto_attack,target_if=max:debuff.reavers_mark.remains" );
  ar->add_action( "pick_up_fragment,type=all,use_off_gcd=1,if=fury<=90" );
  ar->add_action( "variable,name=fel_barrage,op=set,value=talent.fel_barrage&(cooldown.fel_barrage.remains<gcd.max*7&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in<gcd.max*7|raid_event.adds.in>90)&(cooldown.metamorphosis.remains|active_enemies>2)|buff.fel_barrage.up)&!(active_enemies=1&!raid_event.adds.exists)" );
  ar->add_action( "chaos_strike,target_if=max:target.health_pct,if=buff.rending_strike.up&buff.glaive_flurry.up&(variable.rg_ds=2|active_enemies>1)&time>10&!debuff.reavers_mark.up" );
  ar->add_action( "annihilation,target_if=max:target.health_pct,if=buff.rending_strike.up&buff.glaive_flurry.up&(variable.rg_ds=2|active_enemies>1)&!debuff.reavers_mark.up" );
  ar->add_action( "chaos_strike,target_if=max:debuff.reavers_mark.remains,if=buff.rending_strike.up&buff.glaive_flurry.up&(variable.rg_ds=2|active_enemies>1)&time>10&debuff.reavers_mark.remains" );
  ar->add_action( "annihilation,target_if=max:debuff.reavers_mark.remains,if=buff.rending_strike.up&buff.glaive_flurry.up&(variable.rg_ds=2|active_enemies>1)&debuff.reavers_mark.remains" );
  ar->add_action( "reavers_glaive,target_if=max:debuff.reavers_mark.remains,if=buff.glaive_flurry.down&buff.rending_strike.down&buff.thrill_of_the_fight_damage.remains<gcd.max*4+(variable.rg_ds=2)+(cooldown.the_hunt.remains<gcd.max*3)*3+(cooldown.eye_beam.remains<gcd.max*3&talent.shattered_destiny)*3&(variable.rg_ds=0|variable.rg_ds=1&cooldown.blade_dance.up|variable.rg_ds=2&cooldown.blade_dance.remains)&(buff.thrill_of_the_fight_damage.up|!prev_gcd.1.death_sweep|!variable.rg_inc)&active_enemies<3&!action.reavers_glaive.last_used<5&debuff.essence_break.down&(buff.metamorphosis.remains>2|cooldown.eye_beam.remains<10|fight_remains<10)&(variable.pull_remains>=10|fight_remains<=10)|fight_remains<=10" );
  ar->add_action( "reavers_glaive,target_if=max:debuff.reavers_mark.remains,if=buff.glaive_flurry.down&buff.rending_strike.down&buff.thrill_of_the_fight_damage.remains<4&(buff.thrill_of_the_fight_damage.up|!prev_gcd.1.death_sweep|!variable.rg_inc)&active_enemies>=2&(variable.pull_remains>=10|fight_remains<10)" );
  ar->add_action( "call_action_list,name=ar_cooldown" );
  ar->add_action( "run_action_list,name=ar_opener,if=(cooldown.eye_beam.up|cooldown.metamorphosis.up|cooldown.essence_break.up)&time<15&(raid_event.adds.in>20|talent.cycle_of_hatred)" );
  ar->add_action( "sigil_of_spite,if=debuff.essence_break.down&cooldown.blade_dance.remains&debuff.reavers_mark.remains>=2-talent.quickened_sigils&(buff.necessary_sacrifice.remains>=2-talent.quickened_sigils|!set_bonus.thewarwithin_season_2_4pc|cooldown.eye_beam.remains>8)&(buff.metamorphosis.down|buff.metamorphosis.remains+talent.shattered_destiny>=buff.necessary_sacrifice.remains+2-talent.quickened_sigils)|fight_remains<20", "Lineup Sigil of Spite with initiative and 4-piece while preferring to use outside of metamorphosis" );
  ar->add_action( "run_action_list,name=ar_fel_barrage,if=variable.fel_barrage&raid_event.adds.up" );
  ar->add_action( "immolation_aura,if=active_enemies>2&talent.ragefire&(!talent.fel_barrage|cooldown.fel_barrage.remains>recharge_time)&debuff.essence_break.down&(buff.metamorphosis.down|buff.metamorphosis.remains>5)" );
  ar->add_action( "immolation_aura,if=active_enemies>2&talent.ragefire&raid_event.adds.up&raid_event.adds.remains<15&raid_event.adds.remains>5&debuff.essence_break.down" );
  ar->add_action( "vengeful_retreat,if=talent.initiative&talent.tactical_retreat&time>20&(cooldown.eye_beam.up&(talent.restless_hunter|cooldown.metamorphosis.remains>10))&(!talent.inertia&buff.unbound_chaos.down|buff.inertia_trigger.down&buff.metamorphosis.down)", "actions.ar+=/fel_rush,if=buff.unbound_chaos.up&active_enemies>2&(!talent.inertia|cooldown.eye_beam.remains+2>buff.unbound_chaos.remains)  Lineup Vengeful retreat with Eyebeam casts for Tactical retreat builds" );
  ar->add_action( "vengeful_retreat,use_off_gcd=1,if=talent.initiative&!talent.tactical_retreat&(cooldown.eye_beam.remains>15&gcd.remains<0.3|gcd.remains<0.2&cooldown.eye_beam.remains<=gcd.remains&cooldown.metamorphosis.remains>10)&(!variable.trinket1_steroids&!variable.trinket2_steroids|variable.trinket1_steroids&(trinket.1.stat.any.cooldown_remains<gcd.max*3|trinket.1.stat.any.cooldown_remains>30)|variable.trinket2_steroids&(trinket.2.stat.any.cooldown_remains<gcd.max*3|trinket.2.stat.any.cooldown_remains>30))&time>20&(!talent.inertia&buff.unbound_chaos.down|buff.inertia_trigger.down&buff.metamorphosis.down)" );
  ar->add_action( "run_action_list,name=ar_fel_barrage,if=variable.fel_barrage|!talent.demon_blades&talent.fel_barrage&(buff.fel_barrage.up|cooldown.fel_barrage.up)&buff.metamorphosis.down", "talent.initiative&(cooldown.eye_beam.remains>15&gcd.remains<0.3|gcd.remains<0.2&cooldown.eye_beam.remains<=gcd.remains&(buff.unbound_chaos.up|action.immolation_aura.recharge_time>6|!talent.inertia|talent.momentum)&(cooldown.metamorphosis.remains>10|cooldown.blade_dance.remains<gcd.max*2&(talent.inertia|talent.momentum|buff.metamorphosis.up)))&(!talent.student_of_suffering|cooldown.sigil_of_flame.remains)&time>10&(!variable.trinket1_steroids&!variable.trinket2_steroids|variable.trinket1_steroids&(trinket.1.cooldown.remains<gcd.max*3|trinket.1.cooldown.remains>20)|variable.trinket2_steroids&(trinket.2.cooldown.remains<gcd.max*3|trinket.2.cooldown.remains>20|talent.shattered_destiny))&(cooldown.metamorphosis.remains|hero_tree.aldrachi_reaver)&time>20" );
  ar->add_action( "felblade,target_if=max:debuff.reavers_mark.remains,if=!talent.inertia&active_enemies=1&buff.unbound_chaos.up&buff.initiative.up&debuff.essence_break.down&buff.metamorphosis.down" );
  ar->add_action( "felblade,target_if=max:debuff.reavers_mark.remains,if=buff.inertia_trigger.up&talent.inertia&cooldown.eye_beam.remains<=0.5&(cooldown.metamorphosis.remains&talent.looks_can_kill|active_enemies>1)" );
  ar->add_action( "run_action_list,name=ar_meta,if=buff.metamorphosis.up" );
  ar->add_action( "felblade,target_if=max:debuff.reavers_mark.remains,if=buff.inertia_trigger.up&talent.inertia&buff.inertia.down&cooldown.blade_dance.remains<4&(cooldown.eye_beam.remains>5&cooldown.eye_beam.remains>buff.unbound_chaos.remains|cooldown.eye_beam.remains<=gcd.max&cooldown.vengeful_retreat.remains<=gcd.max+1)" );
  ar->add_action( "immolation_aura,if=talent.a_fire_inside&talent.burning_wound&full_recharge_time<gcd.max*2&(raid_event.adds.in>full_recharge_time|active_enemies>desired_targets)" );
  ar->add_action( "immolation_aura,if=active_enemies>desired_targets&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>full_recharge_time)" );
  ar->add_action( "immolation_aura,if=fight_remains<15&cooldown.blade_dance.remains&talent.ragefire" );
  ar->add_action( "eye_beam,if=(cooldown.blade_dance.remains<7|raid_event.adds.up)&(active_enemies>desired_targets*2&(buff.thrill_of_the_fight_damage.up|buff.rending_strike.down&buff.glaive_flurry.down)|raid_event.adds.in>30-buff.cycle_of_hatred.stack*5|fight_style.dungeonroute&!raid_event.adds.in<=40-buff.cycle_of_hatred.stack*5)&(!variable.trinket1_steroids&!variable.trinket2_steroids|variable.trinket1_steroids&(trinket.1.stat.any.cooldown_remains<gcd.max*3|trinket.1.stat.any.cooldown_remains>30-buff.cycle_of_hatred.stack*5)|variable.trinket2_steroids&(trinket.2.stat.any.cooldown_remains<gcd.max*3|trinket.2.stat.any.cooldown_remains>30-buff.cycle_of_hatred.stack*5))|fight_remains<10", "actions.ar+=/blade_dance,if=buff.rending_strike.down&buff.glaive_flurry.up&active_enemies>2&cooldown.eye_beam.remains<=4&buff.thrill_of_the_fight_damage.remains<gcd.max&raid_event.adds.remains>10&(cooldown.immolation_aura.remains|!talent.burning_wound) actions.ar+=/eye_beam,if=!talent.essence_break&(!talent.chaotic_transformation|cooldown.metamorphosis.remains<5+3*talent.shattered_destiny|cooldown.metamorphosis.remains>10)&(active_enemies>desired_targets*2|raid_event.adds.in>30-talent.cycle_of_hatred.rank*2.5*buff.cycle_of_hatred.stack)&(!talent.initiative|cooldown.vengeful_retreat.remains>5|cooldown.vengeful_retreat.up&active_enemies>2|talent.shattered_destiny)" );
  ar->add_action( "blade_dance,target_if=max:debuff.reavers_mark.remains,if=(cooldown.eye_beam.remains>=gcd.max*2|active_enemies>=2&buff.glaive_flurry.up&(raid_event.adds.in>30-buff.cycle_of_hatred.stack*5|raid_event.adds.remains>=cooldown.eye_beam.remains&cooldown.eye_beam.remains<gcd.max*2))&buff.rending_strike.down", "talent.essence_break&(cooldown.essence_break.remains<gcd.max*2+5*talent.shattered_destiny|talent.shattered_destiny&cooldown.essence_break.remains>10)&(cooldown.blade_dance.remains<7|raid_event.adds.up)&(!talent.initiative|cooldown.vengeful_retreat.remains>10|!talent.inertia&!talent.momentum|raid_event.adds.up)&(active_enemies+3>=desired_targets+raid_event.adds.count|raid_event.adds.in>30-talent.cycle_of_hatred.rank*6)&(!talent.inertia|buff.inertia_trigger.up|action.immolation_aura.charges=0&action.immolation_aura.recharge_time>5)&(!raid_event.adds.up|raid_event.adds.remains>8)&(!variable.trinket1_steroids&!variable.trinket2_steroids|variable.trinket1_steroids&(trinket.1.cooldown.remains<gcd.max*3|trinket.1.cooldown.remains>20)|variable.trinket2_steroids&(trinket.2.cooldown.remains<gcd.max*3|trinket.2.cooldown.remains>20))|fight_remains<10" );
  ar->add_action( "chaos_strike,target_if=max:debuff.reavers_mark.remains,if=buff.rending_strike.up" );
  ar->add_action( "sigil_of_flame,if=active_enemies>3|debuff.essence_break.down" );
  ar->add_action( "felblade,target_if=max:debuff.reavers_mark.remains,if=fury.deficit>=40+variable.fury_gen*0.5&!buff.inertia_trigger.up&(!talent.blind_fury|cooldown.eye_beam.remains>5)" );
  ar->add_action( "glaive_tempest,if=active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>10" );
  ar->add_action( "chaos_strike,target_if=max:debuff.reavers_mark.remains,if=debuff.essence_break.up" );
  ar->add_action( "chaos_nova,if=talent.chaos_fragments&active_enemies>4" );
  ar->add_action( "throw_glaive,target_if=max:debuff.reavers_mark.remains,if=active_enemies>2&talent.furious_throws&talent.soulscar&(!talent.screaming_brutality|charges=2|full_recharge_time<cooldown.blade_dance.remains)" );
  ar->add_action( "chaos_strike,if=cooldown.eye_beam.remains>gcd.max*4|fury>=70-variable.fury_gen*gcd.max-talent.blind_fury.rank*15" );
  ar->add_action( "felblade,target_if=max:debuff.reavers_mark.remains,if=!talent.a_fire_inside&fury<40" );
  ar->add_action( "immolation_aura,if=raid_event.adds.in>full_recharge_time|active_enemies>desired_targets&active_enemies>2" );
  ar->add_action( "sigil_of_flame,if=buff.out_of_range.down&debuff.essence_break.down&(!talent.fel_barrage|cooldown.fel_barrage.remains>25|active_enemies=1&!raid_event.adds.exists)" );
  ar->add_action( "demons_bite,target_if=max:debuff.reavers_mark.remains" );
  ar->add_action( "throw_glaive,target_if=max:debuff.reavers_mark.remains,if=buff.unbound_chaos.down&recharge_time<cooldown.eye_beam.remains&debuff.essence_break.down&(cooldown.eye_beam.remains>8|charges_fractional>1.01)&buff.out_of_range.down&active_enemies>1&!talent.furious_throws" );
  ar->add_action( "fel_rush,if=buff.unbound_chaos.down&recharge_time<cooldown.eye_beam.remains&debuff.essence_break.down&(cooldown.eye_beam.remains>8|charges_fractional>1.01)&active_enemies>1" );
  ar->add_action( "arcane_torrent,if=buff.out_of_range.down&debuff.essence_break.down&fury<100" );

  ar_cooldown->add_action( "metamorphosis,if=(((cooldown.eye_beam.remains>=20|talent.cycle_of_hatred&cooldown.eye_beam.remains>=13|raid_event.adds.remains>8&raid_event.adds.remains<cooldown.eye_beam.remains)&(!talent.essence_break|debuff.essence_break.up)&buff.fel_barrage.down&(raid_event.adds.in>40|(raid_event.adds.remains>8|!talent.fel_barrage)&active_enemies>desired_targets|fight_style.dungeonroute&!raid_event.adds.in<=120)|!talent.chaotic_transformation|fight_remains<30)&buff.inner_demon.down&(!talent.restless_hunter&cooldown.blade_dance.remains>gcd.max*3|prev_gcd.1.death_sweep|prev_gcd.2.death_sweep|prev_gcd.3.death_sweep))&!talent.inertia&!talent.essence_break&time>15" );
  ar_cooldown->add_action( "metamorphosis,if=(cooldown.blade_dance.remains&((prev_gcd.1.death_sweep|prev_gcd.2.death_sweep|prev_gcd.3.death_sweep|buff.metamorphosis.up&buff.metamorphosis.remains<gcd.max)&cooldown.eye_beam.remains&buff.fel_barrage.down&(raid_event.adds.in>40|(raid_event.adds.remains>8|!talent.fel_barrage)&active_enemies>desired_targets|fight_style.dungeonroute&!raid_event.adds.in<=120)|!talent.chaotic_transformation|fight_remains<30)&(buff.inner_demon.down&(buff.rending_strike.down|!talent.restless_hunter|prev_gcd.1.death_sweep)))&(talent.inertia|talent.essence_break)&time>15" );
  ar_cooldown->add_action( "potion,if=fight_remains<35|(buff.metamorphosis.up|debuff.essence_break.up)&time>10" );
  ar_cooldown->add_action( "invoke_external_buff,name=power_infusion,if=buff.metamorphosis.up|fight_remains<=20" );
  ar_cooldown->add_action( "variable,name=special_trinket,op=set,value=equipped.mad_queens_mandate|equipped.treacherous_transmitter|equipped.skardyns_grace|equipped.signet_of_the_priory|equipped.cursed_stone_idol" );
  ar_cooldown->add_action( "use_item,name=mad_queens_mandate,if=((!talent.initiative|buff.initiative.up|time>5)&(buff.metamorphosis.remains>5|buff.metamorphosis.down)&(trinket.1.is.mad_queens_mandate&(trinket.2.cooldown.duration<10|trinket.2.cooldown.remains>10|!trinket.2.has_buff.any)|trinket.2.is.mad_queens_mandate&(trinket.1.cooldown.duration<10|trinket.1.cooldown.remains>10|!trinket.1.has_buff.any))&fight_remains>120|fight_remains<10&fight_remains<buff.metamorphosis.remains)&debuff.essence_break.down|fight_remains<5" );
  ar_cooldown->add_action( "use_item,name=cursed_stone_idol,if=((buff.metamorphosis.remains>5|buff.metamorphosis.down)&(!buff.inertia.up|!talent.inertia)&(debuff.essence_break.down|!talent.essence_break)&(trinket.1.is.cursed_stone_idol&(trinket.2.cooldown.duration<120|trinket.2.cooldown.remains>10|!trinket.2.has_buff.any|trinket.2.is.signet_of_the_priory|trinket.2.is.unyielding_netherprism)|trinket.2.is.cursed_stone_idol&(trinket.1.cooldown.duration<120|trinket.1.cooldown.remains>10|!trinket.1.has_buff.any|trinket.1.is.signet_of_the_priory|trinket.1.is.unyielding_netherprism))|fight_remains<10&fight_remains<buff.metamorphosis.remains)|fight_remains<5" );
  ar_cooldown->add_action( "use_item,name=treacherous_transmitter,if=!equipped.mad_queens_mandate|equipped.mad_queens_mandate&(trinket.1.is.mad_queens_mandate&trinket.1.cooldown.remains>fight_remains|trinket.2.is.mad_queens_mandate&trinket.2.cooldown.remains>fight_remains)|fight_remains>25" );
  ar_cooldown->add_action( "use_item,name=skardyns_grace,if=(!equipped.mad_queens_mandate|fight_remains>25|trinket.2.is.skardyns_grace&trinket.1.cooldown.remains>fight_remains|trinket.1.is.skardyns_grace&trinket.2.cooldown.remains>fight_remains|trinket.1.cooldown.duration<10|trinket.2.cooldown.duration<10)&buff.metamorphosis.up" );
  ar_cooldown->add_action( "use_item,name=house_of_cards,if=(cooldown.eye_beam.remains<gcd.max|buff.metamorphosis.up)&(raid_event.adds.remains>8|raid_event.adds.in>15)|fight_remains<25" );
  ar_cooldown->add_action( "use_item,name=signet_of_the_priory,if=time<20&(!talent.inertia|buff.inertia.up)&!equipped.cursed_stone_idol|(cooldown.eye_beam.remains<gcd.max|buff.metamorphosis.remains>8|cooldown.metamorphosis.up&buff.metamorphosis.up)&(raid_event.adds.remains>15|raid_event.adds.in>115|fight_style.dungeonroute&!raid_event.adds.in<=120)&(!equipped.cursed_stone_idol|(trinket.1.is.signet_of_the_priory&trinket.2.cooldown.remains>20|trinket.2.is.signet_of_the_priory&trinket.1.cooldown.remains>20))|fight_remains<25" );
  ar_cooldown->add_action( "use_item,name=perfidious_projector,if=variable.tier33_4piece&variable.double_on_use|fight_remains<15" );
  ar_cooldown->add_action( "use_item,name=chaotic_nethergate,if=variable.tier33_4piece&variable.double_on_use|fight_remains<15" );
  ar_cooldown->add_action( "use_item,name=ratfang_toxin,if=variable.tier33_4piece&variable.double_on_use|fight_remains<5" );
  ar_cooldown->add_action( "use_item,name=geargrinders_spare_keys,if=variable.tier33_4piece&variable.double_on_use|fight_remains<10" );
  ar_cooldown->add_action( "use_item,name=grim_codex,if=variable.tier33_4piece&variable.double_on_use|fight_remains<10" );
  ar_cooldown->add_action( "use_item,name=ravenous_honey_buzzer,if=(variable.tier33_4piece&(buff.inertia.down&(cooldown.essence_break.remains&debuff.essence_break.down|!talent.essence_break))&(trinket.1.is.ravenous_honey_buzzer&(trinket.2.cooldown.duration<10|trinket.2.cooldown.remains>10|!trinket.2.has_buff.any)|trinket.2.is.ravenous_honey_buzzer&(trinket.1.cooldown.duration<10|trinket.1.cooldown.remains>10|!trinket.1.has_buff.any))&fight_remains>120|fight_remains<10&fight_remains<buff.metamorphosis.remains)|fight_remains<5" );
  ar_cooldown->add_action( "use_item,name=blastmaster3000,if=variable.tier33_4piece&variable.double_on_use|fight_remains<10" );
  ar_cooldown->add_action( "use_item,name=junkmaestros_mega_magnet,if=variable.tier33_4piece_magnet&variable.double_on_use&time>10|fight_remains<5" );
  ar_cooldown->add_action( "do_treacherous_transmitter_task,if=cooldown.eye_beam.remains>15|cooldown.eye_beam.remains<5|fight_remains<20|buff.metamorphosis.up" );
  ar_cooldown->add_action( "use_item,name=unyielding_netherprism,if=((cooldown.eye_beam.remains<gcd.max&(active_enemies>1|talent.looks_can_kill)&((trinket.1.is.unyielding_netherprism&trinket.2.cooldown.duration>90&variable.trinket2_steroids|cooldown.metamorphosis.remains<=5&buff.latent_energy.stack>10)|(trinket.2.is.unyielding_netherprism&trinket.1.cooldown.duration>90&variable.trinket1_steroids|cooldown.metamorphosis.remains<=5&buff.latent_energy.stack>10))|(buff.metamorphosis.up&((trinket.1.is.unyielding_netherprism&trinket.2.cooldown.duration>90&variable.trinket2_steroids)|(trinket.2.is.unyielding_netherprism&trinket.1.cooldown.duration>90&variable.trinket1_steroids)&!equipped.improvised_seaforium_pacemaker&!equipped.soleahs_secret_technique)))&(raid_event.adds.in>105|raid_event.adds.remains>8)|fight_remains<25)&((trinket.1.is.unyielding_netherprism&(!variable.trinket2_steroids|trinket.2.cooldown.duration<120|trinket.2.cooldown.remains>20)|trinket.2.is.unyielding_netherprism&(!variable.trinket1_steroids|trinket.1.cooldown.duration<120|trinket.1.cooldown.remains>20))|equipped.improvised_seaforium_pacemaker|equipped.soleahs_secret_technique)", "actions.ar_cooldown+=/use_item,name=unyielding_netherprism,if=((cooldown.eye_beam.remains<gcd.max&(active_enemies>1|talent.looks_can_kill)&(buff.latent_energy.stack>11)&((trinket.1.is.unyielding_netherprism&trinket.2.cooldown.duration>=90|cooldown.metamorphosis.remains<=5)|(trinket.2.is.unyielding_netherprism&trinket.1.cooldown.duration>=90|cooldown.metamorphosis.remains<=5)))&(raid_event.adds.in>105|raid_event.adds.remains>8)|fight_remains<25)&((trinket.1.is.unyielding_netherprism&(!variable.trinket2_steroids&!trinket.2.has_cooldown|trinket.2.cooldown.remains>20)|trinket.2.is.unyielding_netherprism&(!variable.trinket1_steroids&!trinket.1.has_cooldown|trinket.1.cooldown.remains>20))|equipped.improvised_seaforium_pacemaker)" );
  ar_cooldown->add_action( "use_item,slot=trinket1,if=((cooldown.eye_beam.remains<gcd.max&active_enemies>1|buff.metamorphosis.up&(cooldown.metamorphosis.remains<buff.metamorphosis.remains|cooldown.metamorphosis.remains>=20|cooldown.metamorphosis.up))&(raid_event.adds.in>trinket.1.cooldown.duration-15|raid_event.adds.remains>8|fight_style.dungeonroute&!raid_event.adds.in<=trinket.1.cooldown.duration)|!trinket.1.has_buff.any|fight_remains<25)&!trinket.1.is.mister_locknstalk&!variable.trinket1_crit&!trinket.1.is.skardyns_grace&!trinket.1.is.treacherous_transmitter&!trinket.1.is.unyielding_netherprism&!trinket.1.is.signet_of_the_priory&(!variable.special_trinket|trinket.2.cooldown.remains>20|(trinket.1.cooldown.duration>90&trinket.1.has_buff.agility))" );
  ar_cooldown->add_action( "use_item,slot=trinket2,if=((cooldown.eye_beam.remains<gcd.max&active_enemies>1|buff.metamorphosis.up&(cooldown.metamorphosis.remains<buff.metamorphosis.remains|cooldown.metamorphosis.remains>=20|cooldown.metamorphosis.up))&(raid_event.adds.in>trinket.2.cooldown.duration-15|raid_event.adds.remains>8|fight_style.dungeonroute&!raid_event.adds.in<=trinket.2.cooldown.duration)|!trinket.2.has_buff.any|fight_remains<25)&!trinket.2.is.mister_locknstalk&!variable.trinket2_crit&!trinket.2.is.skardyns_grace&!trinket.2.is.treacherous_transmitter&!trinket.2.is.unyielding_netherprism&!trinket.2.is.signet_of_the_priory&(!variable.special_trinket|trinket.1.cooldown.remains>20|(trinket.2.cooldown.duration>90&trinket.2.has_buff.agility))" );
  ar_cooldown->add_action( "the_hunt,target_if=max:debuff.reavers_mark.remains,if=debuff.essence_break.down&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>45)&(debuff.reavers_mark.up|raid_event.adds.remains>=15)&buff.reavers_glaive.down&(buff.metamorphosis.remains>5|buff.metamorphosis.down)&(!talent.initiative|buff.initiative.up|time>5)&time>5&(!talent.inertia&buff.unbound_chaos.down|buff.inertia_trigger.down)|fight_remains<=30" );
  ar_cooldown->add_action( "sigil_of_spite,if=debuff.essence_break.down&(debuff.reavers_mark.remains>=2-talent.quickened_sigils)&cooldown.blade_dance.remains&time>15" );

  ar_fel_barrage->add_action( "variable,name=generator_up,op=set,value=cooldown.felblade.remains<gcd.max|cooldown.sigil_of_flame.remains<gcd.max" );
  ar_fel_barrage->add_action( "variable,name=gcd_drain,op=set,value=gcd.max*32" );
  ar_fel_barrage->add_action( "annihilation,target_if=max:debuff.reavers_mark.remains,if=buff.inner_demon.up" );
  ar_fel_barrage->add_action( "eye_beam,if=(buff.fel_barrage.down|fury>45&talent.blind_fury)&(active_enemies>1&raid_event.adds.up|raid_event.adds.in>40-buff.cycle_of_hatred.stack*5)" );
  ar_fel_barrage->add_action( "essence_break,if=buff.fel_barrage.down&buff.metamorphosis.up" );
  ar_fel_barrage->add_action( "death_sweep,target_if=max:debuff.reavers_mark.remains,if=buff.fel_barrage.down" );
  ar_fel_barrage->add_action( "immolation_aura,if=(active_enemies>2|buff.fel_barrage.up)&(cooldown.eye_beam.remains>recharge_time+3)" );
  ar_fel_barrage->add_action( "glaive_tempest,if=buff.fel_barrage.down&active_enemies>1" );
  ar_fel_barrage->add_action( "blade_dance,target_if=max:debuff.reavers_mark.remains,if=buff.fel_barrage.down" );
  ar_fel_barrage->add_action( "fel_barrage,if=fury>100&(raid_event.adds.in>90|raid_event.adds.in<gcd.max|raid_event.adds.remains>4&active_enemies>2)" );
  ar_fel_barrage->add_action( "felblade,target_if=max:debuff.reavers_mark.remains,if=buff.inertia_trigger.up&buff.fel_barrage.up" );
  ar_fel_barrage->add_action( "sigil_of_flame,if=fury.deficit>40&buff.fel_barrage.up", "actions.ar_fel_barrage+=/fel_rush,if=buff.unbound_chaos.up&fury>20&buff.fel_barrage.up" );
  ar_fel_barrage->add_action( "felblade,target_if=max:debuff.reavers_mark.remains,if=buff.fel_barrage.up&fury.deficit>40" );
  ar_fel_barrage->add_action( "death_sweep,target_if=max:debuff.reavers_mark.remains,if=fury-variable.gcd_drain-35>0&(buff.fel_barrage.remains<3|variable.generator_up|fury>80|variable.fury_gen>18)" );
  ar_fel_barrage->add_action( "glaive_tempest,if=fury-variable.gcd_drain-30>0&(buff.fel_barrage.remains<3|variable.generator_up|fury>80|variable.fury_gen>18)" );
  ar_fel_barrage->add_action( "blade_dance,target_if=max:debuff.reavers_mark.remains,if=fury-variable.gcd_drain-35>0&(buff.fel_barrage.remains<3|variable.generator_up|fury>80|variable.fury_gen>18)" );
  ar_fel_barrage->add_action( "arcane_torrent,if=fury.deficit>40&buff.fel_barrage.up" );
  ar_fel_barrage->add_action( "the_hunt,target_if=max:debuff.reavers_mark.remains,if=fury>40&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>80)", "actions.ar_fel_barrage+=/fel_rush,if=buff.unbound_chaos.up" );
  ar_fel_barrage->add_action( "annihilation,target_if=max:debuff.reavers_mark.remains,if=fury-variable.gcd_drain-40>20&(buff.fel_barrage.remains<3|variable.generator_up|fury>80|variable.fury_gen>18)" );
  ar_fel_barrage->add_action( "chaos_strike,target_if=max:debuff.reavers_mark.remains,if=fury-variable.gcd_drain-40>20&(cooldown.fel_barrage.remains&cooldown.fel_barrage.remains<10&fury>100|buff.fel_barrage.up&(buff.fel_barrage.remains*variable.fury_gen-buff.fel_barrage.remains*32)>0)" );
  ar_fel_barrage->add_action( "demons_bite" );

  ar_meta->add_action( "death_sweep,target_if=max:debuff.reavers_mark.remains,if=buff.metamorphosis.remains<gcd.max|debuff.essence_break.up|cooldown.metamorphosis.up&!talent.restless_hunter" );
  ar_meta->add_action( "vengeful_retreat,use_off_gcd=1,if=talent.initiative&(cooldown.metamorphosis.remains&(cooldown.essence_break.remains<=0.6|cooldown.essence_break.remains>10|!talent.essence_break)|talent.restless_hunter)&cooldown.eye_beam.remains&(!talent.inertia&buff.unbound_chaos.down|buff.inertia_trigger.down)" );
  ar_meta->add_action( "felblade,target_if=max:debuff.reavers_mark.remains,if=talent.inertia&buff.inertia_trigger.up&cooldown.essence_break.remains<=1&cooldown.blade_dance.remains<=gcd.max*2&cooldown.metamorphosis.remains<=gcd.max*3", "actions.ar_meta+=/annihilation,if=talent.restless_hunter&buff.rending_strike.up&cooldown.essence_break.up&cooldown.metamorphosis.up" );
  ar_meta->add_action( "essence_break,if=fury>=30&talent.restless_hunter&cooldown.metamorphosis.up&(talent.inertia&buff.inertia.up|!talent.inertia)&cooldown.blade_dance.remains<=gcd.max", "actions.ar_meta+=/fel_rush,if=talent.inertia&buff.inertia_trigger.up&cooldown.essence_break.remains<=1&cooldown.blade_dance.remains<=gcd.max*2&cooldown.metamorphosis.remains<=gcd.max*3" );
  ar_meta->add_action( "annihilation,target_if=max:debuff.reavers_mark.remains,if=buff.metamorphosis.remains<gcd.max|debuff.essence_break.remains&debuff.essence_break.remains<0.5&cooldown.blade_dance.remains|buff.inner_demon.up&cooldown.essence_break.up&cooldown.metamorphosis.up" );
  ar_meta->add_action( "felblade,target_if=max:debuff.reavers_mark.remains,if=buff.inertia_trigger.up&talent.inertia&cooldown.metamorphosis.remains&(cooldown.eye_beam.remains<=0.5|cooldown.essence_break.remains<=0.5|cooldown.blade_dance.remains<=5.5|buff.initiative.remains<gcd.remains)" );
  ar_meta->add_action( "fel_rush,if=buff.inertia_trigger.up&talent.inertia&cooldown.metamorphosis.remains&active_enemies>2" );
  ar_meta->add_action( "fel_rush,if=buff.inertia_trigger.up&talent.inertia&cooldown.blade_dance.remains<gcd.max*3&cooldown.metamorphosis.remains&active_enemies>2", "actions.ar_meta+=/felblade,if=buff.inertia_trigger.up&talent.inertia&cooldown.blade_dance.remains<gcd.max*3&cooldown.metamorphosis.remains" );
  ar_meta->add_action( "immolation_aura,if=charges=2&active_enemies>1&debuff.essence_break.down" );
  ar_meta->add_action( "annihilation,target_if=max:debuff.reavers_mark.remains,if=buff.inner_demon.up&(cooldown.eye_beam.remains<gcd.max*3&cooldown.blade_dance.remains|cooldown.metamorphosis.remains<gcd.max*3)" );
  ar_meta->add_action( "essence_break,if=time<20&buff.thrill_of_the_fight_damage.remains>gcd.max*4&buff.metamorphosis.remains>=gcd.max*2&cooldown.metamorphosis.up&cooldown.death_sweep.remains<=gcd.max&buff.inertia.up" );
  ar_meta->add_action( "essence_break,if=fury>20&(cooldown.blade_dance.remains<gcd.max*3|cooldown.blade_dance.up)&(buff.unbound_chaos.down&!talent.inertia|buff.inertia.up)&buff.out_of_range.remains<gcd.max&(!talent.shattered_destiny|cooldown.eye_beam.remains>4)|fight_remains<10" );
  ar_meta->add_action( "death_sweep,target_if=max:debuff.reavers_mark.remains" );
  ar_meta->add_action( "eye_beam,if=debuff.essence_break.down&buff.inner_demon.down" );
  ar_meta->add_action( "glaive_tempest,if=debuff.essence_break.down&(cooldown.blade_dance.remains>gcd.max*2|fury>60)&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>10)" );
  ar_meta->add_action( "sigil_of_flame,if=active_enemies>2&debuff.essence_break.down" );
  ar_meta->add_action( "throw_glaive,target_if=max:debuff.reavers_mark.remains,if=talent.soulscar&talent.furious_throws&active_enemies=3&debuff.essence_break.down&(charges=2|full_recharge_time<cooldown.blade_dance.remains)" );
  ar_meta->add_action( "annihilation,target_if=max:debuff.reavers_mark.remains,if=cooldown.blade_dance.remains|fury>60|soul_fragments.total>0|buff.metamorphosis.remains<5&cooldown.felblade.up|debuff.essence_break.up" );
  ar_meta->add_action( "sigil_of_flame,if=buff.metamorphosis.remains>5&buff.out_of_range.down&fury.deficit>=30+variable.fury_gen*gcd.max+active_enemies*talent.flames_of_fury.rank" );
  ar_meta->add_action( "felblade,target_if=max:debuff.reavers_mark.remains,if=fury.deficit>=40+variable.fury_gen*0.5&!buff.inertia_trigger.up" );
  ar_meta->add_action( "sigil_of_flame,if=debuff.essence_break.down&buff.out_of_range.down&fury.deficit>=30+variable.fury_gen*gcd.max+active_enemies*talent.flames_of_fury.rank" );
  ar_meta->add_action( "immolation_aura,if=buff.out_of_range.down&recharge_time<(cooldown.eye_beam.remains<?buff.metamorphosis.remains)&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>full_recharge_time)" );
  ar_meta->add_action( "annihilation,target_if=max:debuff.reavers_mark.remains" );
  ar_meta->add_action( "throw_glaive,target_if=max:debuff.reavers_mark.remains,if=buff.unbound_chaos.down&recharge_time<cooldown.eye_beam.remains&debuff.essence_break.down&(cooldown.eye_beam.remains>8|charges_fractional>1.01)&buff.out_of_range.down&active_enemies>1&!talent.furious_throws" );
  ar_meta->add_action( "fel_rush,if=recharge_time<cooldown.eye_beam.remains&debuff.essence_break.down&(cooldown.eye_beam.remains>8|charges_fractional>1.01)&buff.out_of_range.down&active_enemies>1" );
  ar_meta->add_action( "demons_bite,target_if=max:debuff.reavers_mark.remains" );

  ar_opener->add_action( "potion" );
  ar_opener->add_action( "the_hunt,target_if=max:debuff.reavers_mark.remains" );
  ar_opener->add_action( "vengeful_retreat,use_off_gcd=1,if=talent.initiative&time>4&buff.metamorphosis.up&(!talent.inertia|buff.inertia_trigger.down)&buff.inner_demon.down&cooldown.blade_dance.remains&gcd.remains<0.1" );
  ar_opener->add_action( "death_sweep,target_if=max:debuff.reavers_mark.remains,if=!talent.chaotic_transformation&cooldown.metamorphosis.up&buff.glaive_flurry.up" );
  ar_opener->add_action( "annihilation,target_if=max:debuff.reavers_mark.remains,if=buff.rending_strike.up&buff.thrill_of_the_fight_damage.down" );
  ar_opener->add_action( "felblade,target_if=max:debuff.reavers_mark.remains,if=!talent.inertia&talent.unbound_chaos&buff.unbound_chaos.up&buff.initiative.up&debuff.essence_break.down&active_enemies<=2" );
  ar_opener->add_action( "fel_rush,if=!talent.inertia&talent.unbound_chaos&buff.unbound_chaos.up&buff.initiative.up&debuff.essence_break.down&active_enemies>2" );
  ar_opener->add_action( "annihilation,target_if=max:debuff.reavers_mark.remains,if=talent.inner_demon&buff.inner_demon.up&(!talent.essence_break|cooldown.essence_break.up)" );
  ar_opener->add_action( "essence_break,if=(buff.inertia.up|!talent.inertia)&buff.metamorphosis.up&cooldown.blade_dance.remains<=gcd.max&debuff.reavers_mark.up" );
  ar_opener->add_action( "felblade,target_if=max:debuff.reavers_mark.remains,if=buff.inertia_trigger.up&talent.inertia&talent.restless_hunter&cooldown.essence_break.up&cooldown.metamorphosis.up&buff.metamorphosis.up&cooldown.blade_dance.remains<=gcd.max" );
  ar_opener->add_action( "felblade,target_if=max:debuff.reavers_mark.remains,if=talent.inertia&buff.inertia_trigger.up&(buff.inertia.down&buff.metamorphosis.up)&debuff.essence_break.down&active_enemies<=2", "actions.ar_opener+=/fel_rush,if=buff.inertia_trigger.up&talent.inertia&talent.restless_hunter&cooldown.essence_break.up&cooldown.metamorphosis.up&buff.metamorphosis.up&cooldown.blade_dance.remains<=gcd.max" );
  ar_opener->add_action( "fel_rush,if=talent.inertia&buff.inertia_trigger.up&(buff.inertia.down&buff.metamorphosis.up)&debuff.essence_break.down&(cooldown.felblade.remains|active_enemies>2)" );
  ar_opener->add_action( "felblade,target_if=max:debuff.reavers_mark.remains,if=talent.inertia&buff.inertia_trigger.up&buff.metamorphosis.up&cooldown.metamorphosis.remains&debuff.essence_break.down" );
  ar_opener->add_action( "the_hunt,target_if=max:debuff.reavers_mark.remains,if=(buff.metamorphosis.up&hero_tree.aldrachi_reaver&talent.shattered_destiny|!talent.shattered_destiny&hero_tree.aldrachi_reaver|hero_tree.felscarred)&(!talent.initiative|talent.inertia|buff.initiative.up|time>5)", "actions.ar_opener+=/fel_rush,if=talent.inertia&buff.inertia_trigger.up&buff.metamorphosis.up&cooldown.metamorphosis.remains" );
  ar_opener->add_action( "felblade,target_if=max:debuff.reavers_mark.remains,if=fury<40&buff.inertia_trigger.down&debuff.essence_break.down" );
  ar_opener->add_action( "reavers_glaive,target_if=max:debuff.reavers_mark.remains,if=debuff.reavers_mark.down&debuff.essence_break.down" );
  ar_opener->add_action( "chaos_strike,target_if=max:debuff.reavers_mark.remains,if=buff.rending_strike.up&active_enemies>2" );
  ar_opener->add_action( "blade_dance,target_if=max:debuff.reavers_mark.remains,if=buff.glaive_flurry.up&active_enemies>2" );
  ar_opener->add_action( "immolation_aura,if=talent.a_fire_inside&talent.burning_wound&buff.metamorphosis.down" );
  ar_opener->add_action( "metamorphosis,if=buff.metamorphosis.up&cooldown.blade_dance.remains>gcd.max*2&buff.inner_demon.down&(!talent.restless_hunter|prev_gcd.1.death_sweep)&(cooldown.essence_break.remains|!talent.essence_break|!talent.chaotic_transformation)" );
  ar_opener->add_action( "sigil_of_spite,if=debuff.reavers_mark.up&(cooldown.eye_beam.remains&cooldown.metamorphosis.remains)&debuff.essence_break.down" );
  ar_opener->add_action( "eye_beam,if=buff.metamorphosis.down|debuff.essence_break.down&buff.inner_demon.down&(cooldown.blade_dance.remains|talent.essence_break&cooldown.essence_break.up)" );
  ar_opener->add_action( "essence_break,if=cooldown.blade_dance.remains<gcd.max&!hero_tree.felscarred&!talent.shattered_destiny&buff.metamorphosis.up|cooldown.eye_beam.remains&cooldown.metamorphosis.remains" );
  ar_opener->add_action( "death_sweep,target_if=max:debuff.reavers_mark.remains" );
  ar_opener->add_action( "annihilation,target_if=max:debuff.reavers_mark.remains" );
  ar_opener->add_action( "demons_bite,target_if=max:debuff.reavers_mark.remains" );

  fs->add_action( "pick_up_fragment,type=all,use_off_gcd=1", "Fel-Scarred" );
  fs->add_action( "variable,name=fel_barrage,op=set,value=talent.fel_barrage&(cooldown.fel_barrage.remains<gcd.max*7&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in<gcd.max*7|raid_event.adds.in>90)&(cooldown.metamorphosis.remains|active_enemies>2)|buff.fel_barrage.up)&!(active_enemies=1&!raid_event.adds.exists)" );
  fs->add_action( "call_action_list,name=fs_cooldown" );
  fs->add_action( "run_action_list,name=fs_opener,if=(cooldown.eye_beam.up|cooldown.metamorphosis.up|cooldown.essence_break.up|buff.demonsurge.stack<3+talent.student_of_suffering+talent.a_fire_inside)&time<15&raid_event.adds.in>40-buff.cycle_of_hatred.stack*5" );
  fs->add_action( "run_action_list,name=fs_fel_barrage,if=variable.fel_barrage&raid_event.adds.up" );
  fs->add_action( "immolation_aura,if=active_enemies>2&talent.ragefire&(!talent.fel_barrage|cooldown.fel_barrage.remains>recharge_time)&debuff.essence_break.down&(buff.metamorphosis.down|buff.metamorphosis.remains>5)" );
  fs->add_action( "immolation_aura,if=active_enemies>2&talent.ragefire&raid_event.adds.up&raid_event.adds.remains<15&raid_event.adds.remains>5&debuff.essence_break.down" );
  fs->add_action( "felblade,if=talent.unbound_chaos&buff.unbound_chaos.up&!talent.inertia&active_enemies<=2&(talent.student_of_suffering&cooldown.eye_beam.remains-gcd.max*2<=buff.unbound_chaos.remains|hero_tree.aldrachi_reaver)" );
  fs->add_action( "fel_rush,if=talent.unbound_chaos&buff.unbound_chaos.up&!talent.inertia&active_enemies>3&(talent.student_of_suffering&cooldown.eye_beam.remains-gcd.max*2<=buff.unbound_chaos.remains)" );
  fs->add_action( "run_action_list,name=fs_meta,if=buff.metamorphosis.up" );
  fs->add_action( "vengeful_retreat,use_off_gcd=1,if=talent.initiative&(cooldown.eye_beam.remains>15&gcd.remains<0.3|gcd.remains<0.2&cooldown.eye_beam.remains<=gcd.remains&(cooldown.metamorphosis.remains>10|cooldown.blade_dance.remains<gcd.max*3))&(!talent.student_of_suffering|cooldown.sigil_of_flame.remains)&(cooldown.essence_break.remains<=gcd.max*2&talent.student_of_suffering&cooldown.sigil_of_flame.remains|cooldown.essence_break.remains>=18|!talent.student_of_suffering)&cooldown.metamorphosis.remains>10&time>20&(!talent.inertia|buff.inertia_trigger.down)" );
  fs->add_action( "run_action_list,name=fs_fel_barrage,if=variable.fel_barrage|!talent.demon_blades&talent.fel_barrage&(buff.fel_barrage.up|cooldown.fel_barrage.up)&buff.metamorphosis.down" );
  fs->add_action( "immolation_aura,if=variable.fs_tier34_2piece&(full_recharge_time<gcd.max*3|buff.immolation_aura.down&(cooldown.eye_beam.remains<3&(!talent.essence_break|buff.cycle_of_hatred.stack<4)|talent.essence_break&cooldown.essence_break.remains<=5|talent.essence_break&((cooldown.eye_beam.remains<3)*cooldown.essence_break.remains)>recharge_time))&(!talent.dancing_with_fate.rank=2|cooldown.blade_dance.remains>=gcd.max|cooldown.eye_beam.remains<3)", "actions.fs+=/felblade,if=!talent.inertia&active_enemies=1&buff.unbound_chaos.up&buff.initiative.up&debuff.essence_break.down&buff.metamorphosis.down actions.fs+=/felblade,if=buff.inertia_trigger.up&talent.inertia&buff.inertia.down&cooldown.blade_dance.remains<4&cooldown.eye_beam.remains>5&cooldown.eye_beam.remains>buff.unbound_chaos.remains-2 actions.fs+=/fel_rush,if=buff.unbound_chaos.up&talent.inertia&buff.inertia.down&cooldown.blade_dance.remains<4&cooldown.eye_beam.remains>5&(action.immolation_aura.charges>0|action.immolation_aura.recharge_time+2<cooldown.eye_beam.remains|cooldown.eye_beam.remains>buff.unbound_chaos.remains-2)" );
  fs->add_action( "immolation_aura,if=variable.fs_tier34_2piece&((cooldown.eye_beam.remains+cooldown.metamorphosis.remains)<10)&(!talent.dancing_with_fate.rank=2|cooldown.blade_dance.remains>=gcd.max|cooldown.eye_beam.remains<3)" );
  fs->add_action( "immolation_aura,if=talent.a_fire_inside&talent.burning_wound&full_recharge_time<gcd.max*2&(raid_event.adds.in>full_recharge_time|active_enemies>desired_targets)" );
  fs->add_action( "immolation_aura,if=active_enemies>desired_targets&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>full_recharge_time)" );
  fs->add_action( "immolation_aura,if=fight_remains<15&cooldown.blade_dance.remains&talent.ragefire" );
  fs->add_action( "sigil_of_flame,if=talent.student_of_suffering&(cooldown.eye_beam.remains<=gcd.max|!talent.initiative)&(cooldown.essence_break.remains<gcd.max*3|!talent.essence_break)&(cooldown.metamorphosis.remains>10|cooldown.blade_dance.remains<gcd.max*2)" );
  fs->add_action( "eye_beam,if=(!talent.initiative|buff.initiative.up|cooldown.vengeful_retreat.remains>=10|cooldown.metamorphosis.up|talent.initiative&!talent.tactical_retreat)&(cooldown.blade_dance.remains<7|raid_event.adds.up)&(active_enemies>desired_targets*2|raid_event.adds.in>30-buff.cycle_of_hatred.stack*5|fight_style.dungeonroute&!raid_event.adds.in<=40-buff.cycle_of_hatred.stack*5)&(!variable.trinket1_steroids&!variable.trinket2_steroids|variable.trinket1_steroids&(trinket.1.stat.any.cooldown_remains<gcd.max*3|trinket.1.stat.any.cooldown_remains>30-buff.cycle_of_hatred.stack*5)|variable.trinket2_steroids&(trinket.2.stat.any.cooldown_remains<gcd.max*3|trinket.2.stat.any.cooldown_remains>30-buff.cycle_of_hatred.stack*5))|fight_remains<10", "actions.fs+=/eye_beam,if=!talent.essence_break&(!talent.chaotic_transformation|cooldown.metamorphosis.remains<5+3*talent.shattered_destiny|cooldown.metamorphosis.remains>10)&(active_enemies>desired_targets*2|raid_event.adds.in>30-talent.cycle_of_hatred.rank*2.5*buff.cycle_of_hatred.stack)&(!talent.initiative|cooldown.vengeful_retreat.remains>5|cooldown.vengeful_retreat.up&active_enemies>2|talent.shattered_destiny)&(!talent.student_of_suffering|cooldown.sigil_of_flame.remains)" );
  fs->add_action( "felblade,if=variable.fs_tier34_2piece&talent.inertia&buff.inertia_trigger.up&(buff.immolation_aura.up|buff.inertia_trigger.remains<=0.5|cooldown.the_hunt.remains<=0.5|active_enemies>1&cooldown.eye_beam.remains<=0.5)&active_enemies<=2" );
  fs->add_action( "fel_rush,if=variable.fs_tier34_2piece&talent.inertia&buff.inertia_trigger.up&(buff.immolation_aura.up|buff.inertia_trigger.remains<=gcd.max|cooldown.the_hunt.remains<=gcd.max|active_enemies>1&cooldown.eye_beam.remains<=gcd)&(active_enemies>2|cooldown.felblade.remains)" );
  fs->add_action( "essence_break,if=!talent.initiative&cooldown.eye_beam.remains>5" );
  fs->add_action( "blade_dance,if=cooldown.eye_beam.remains>=gcd.max*4&(active_enemies>3|talent.screaming_brutality&talent.soulscar|!variable.fs_tier34_2piece|variable.fs_tier34_2piece&(talent.dancing_with_fate.rank=2|buff.immolation_aura.down&!debuff.essence_break.up))" );
  fs->add_action( "chaos_strike,if=variable.fs_tier34_2piece&buff.immolation_aura.up&((cooldown.eye_beam.remains>=gcd.max*4|(fury>=70-30*(talent.student_of_suffering&(cooldown.sigil_of_flame.remains<=gcd.max|cooldown.sigil_of_flame.up))-buff.chaos_theory.up*20-variable.fury_gen))|talent.blind_fury)" );
  fs->add_action( "glaive_tempest,if=active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>10" );
  fs->add_action( "sigil_of_flame,if=active_enemies>3&!talent.student_of_suffering" );
  fs->add_action( "chaos_strike,if=debuff.essence_break.up" );
  fs->add_action( "felblade,if=fury.deficit>40+variable.fury_gen*(0.5%gcd.max)&(cooldown.vengeful_retreat.remains>=action.felblade.cooldown+0.5&talent.inertia&active_enemies=1|!talent.inertia|hero_tree.aldrachi_reaver|cooldown.essence_break.remains)&cooldown.metamorphosis.remains&cooldown.eye_beam.remains>=0.5+gcd.max*(talent.student_of_suffering&cooldown.sigil_of_flame.remains<=gcd.max)&(!variable.fs_tier34_2piece|variable.fs_tier34_2piece&buff.immolation_aura.down&cooldown.immolation_aura.remains)", "actions.fs+=/sigil_of_flame,if=talent.student_of_suffering&((cooldown.eye_beam.remains<4&cooldown.metamorphosis.remains>20)|(cooldown.eye_beam.remains<gcd.max&cooldown.metamorphosis.up)) actions.fs+=/felblade,if=buff.out_of_range.up&buff.inertia_trigger.down  actions.fs+=/throw_glaive,if=active_enemies>2&talent.furious_throws&(!talent.screaming_brutality|charges=2|full_recharge_time<cooldown.blade_dance.remains) actions.fs+=/immolation_aura,if=talent.a_fire_inside&talent.isolated_prey&talent.flamebound&active_enemies=1&cooldown.eye_beam.remains>=gcd.max" );
  fs->add_action( "chaos_strike,if=cooldown.eye_beam.remains>=gcd.max*4|(fury>=70-30*(talent.student_of_suffering&(cooldown.sigil_of_flame.remains<=gcd.max|cooldown.sigil_of_flame.up))-buff.chaos_theory.up*20-variable.fury_gen)" );
  fs->add_action( "immolation_aura,if=!variable.fs_tier34_2piece&raid_event.adds.in>full_recharge_time&cooldown.eye_beam.remains>=gcd.max*(1+talent.student_of_suffering&(cooldown.sigil_of_flame.remains<=gcd.max|cooldown.sigil_of_flame.up))|active_enemies>desired_targets&active_enemies>2", "actions.fs+=/chaos_strike,if=cooldown.eye_beam.remains>=gcd.max*3|(fury>=70+(talent.untethered_fury*50-20*talent.blind_fury.rank)*hero_tree.felscarred-38*(talent.student_of_suffering&(cooldown.sigil_of_flame.remains<=gcd.max|cooldown.sigil_of_flame.up))-buff.chaos_theory.up*20-variable.fury_gen) actions.fs+=/chaos_strike,if=cooldown.eye_beam.remains>=gcd.max*2|(cooldown.eye_beam.remains>=gcd+gcd.max*(talent.student_of_suffering&(cooldown.sigil_of_flame.remains<=5|cooldown.sigil_of_flame.up))&(fury>=70-20*talent.blind_fury.rank-38*(talent.student_of_suffering&(cooldown.sigil_of_flame.remains<=gcd.max|cooldown.sigil_of_flame.up))-(talent.essence_break&talent.inertia&cooldown.felblade.up*40)-variable.fury_gen*2))" );
  fs->add_action( "felblade,if=buff.out_of_range.down&buff.inertia_trigger.down&cooldown.eye_beam.remains>=gcd.max*(1+talent.student_of_suffering&(cooldown.sigil_of_flame.remains<=gcd.max|cooldown.sigil_of_flame.up))" );
  fs->add_action( "sigil_of_flame,if=buff.out_of_range.down&debuff.essence_break.down&!talent.student_of_suffering&(!talent.fel_barrage|cooldown.fel_barrage.remains>25|(active_enemies=1&!raid_event.adds.exists))" );
  fs->add_action( "demons_bite", "actions.fs+=/felblade,if=cooldown.blade_dance.remains>=0.5&cooldown.blade_dance.remains<gcd.max" );
  fs->add_action( "throw_glaive,if=recharge_time<cooldown.eye_beam.remains&debuff.essence_break.down&(cooldown.eye_beam.remains>8|charges_fractional>1.01)&buff.out_of_range.down&active_enemies>1&!talent.furious_throws" );
  fs->add_action( "fel_rush,if=buff.unbound_chaos.down&recharge_time<cooldown.eye_beam.remains&debuff.essence_break.down&(cooldown.eye_beam.remains>8|charges_fractional>1.01)&active_enemies>1" );
  fs->add_action( "arcane_torrent,if=buff.out_of_range.down&debuff.essence_break.down&fury<100" );

  fs_cooldown->add_action( "metamorphosis,if=(((cooldown.eye_beam.remains>=20|talent.cycle_of_hatred&cooldown.eye_beam.remains>=13|raid_event.adds.remains>8&raid_event.adds.remains<cooldown.eye_beam.remains)&(!talent.essence_break|debuff.essence_break.up)&buff.fel_barrage.down&(raid_event.adds.in>40|(raid_event.adds.remains>8|!talent.fel_barrage)&active_enemies>desired_targets|fight_style.dungeonroute&!raid_event.adds.in<=120)|fight_remains<30)&buff.inner_demon.down&(!talent.restless_hunter&cooldown.blade_dance.remains>gcd.max*3|prev_gcd.1.death_sweep))&!talent.inertia&!talent.essence_break&time>15" );
  fs_cooldown->add_action( "metamorphosis,if=(cooldown.blade_dance.remains&((prev_gcd.1.death_sweep|prev_gcd.2.death_sweep|prev_gcd.3.death_sweep|buff.metamorphosis.up&buff.metamorphosis.remains<gcd.max)&cooldown.eye_beam.remains&buff.fel_barrage.down&(raid_event.adds.in>40|(raid_event.adds.remains>8|!talent.fel_barrage)&active_enemies>desired_targets|fight_style.dungeonroute&!raid_event.adds.in<=120)|fight_remains<30)&(buff.inner_demon.down&(!talent.restless_hunter|prev_gcd.1.death_sweep)))&(talent.inertia|talent.essence_break)&time>15" );
  fs_cooldown->add_action( "potion,if=fight_remains<35|(buff.metamorphosis.up|debuff.essence_break.up)&time>10" );
  fs_cooldown->add_action( "invoke_external_buff,name=power_infusion,if=buff.metamorphosis.up|fight_remains<=20" );
  fs_cooldown->add_action( "variable,name=special_trinket,op=set,value=equipped.mad_queens_mandate|equipped.treacherous_transmitter|equipped.skardyns_grace|equipped.signet_of_the_priory|equipped.cursed_stone_idol" );
  fs_cooldown->add_action( "use_item,name=mad_queens_mandate,if=((!talent.initiative|buff.initiative.up|time>5)&(buff.metamorphosis.remains>5|buff.metamorphosis.down)&(trinket.1.is.mad_queens_mandate&(trinket.2.cooldown.duration<10|trinket.2.cooldown.remains>10|!trinket.2.has_buff.any)|trinket.2.is.mad_queens_mandate&(trinket.1.cooldown.duration<10|trinket.1.cooldown.remains>10|!trinket.1.has_buff.any))&fight_remains>120|fight_remains<10&fight_remains<buff.metamorphosis.remains)&debuff.essence_break.down|fight_remains<5" );
  fs_cooldown->add_action( "use_item,name=cursed_stone_idol,if=((buff.metamorphosis.remains>5|buff.metamorphosis.down)&(!buff.inertia.up|!talent.inertia)&(debuff.essence_break.down|!talent.essence_break)&(trinket.1.is.cursed_stone_idol&(trinket.2.cooldown.duration<120|trinket.2.cooldown.remains>10|!trinket.2.has_buff.any|trinket.2.is.signet_of_the_priory|trinket.2.is.unyielding_netherprism)|trinket.2.is.cursed_stone_idol&(trinket.1.cooldown.duration<120|trinket.1.cooldown.remains>10|!trinket.1.has_buff.any|trinket.1.is.signet_of_the_priory|trinket.1.is.unyielding_netherprism))|fight_remains<10&fight_remains<buff.metamorphosis.remains)|fight_remains<5" );
  fs_cooldown->add_action( "use_item,name=treacherous_transmitter,if=!equipped.mad_queens_mandate|equipped.mad_queens_mandate&(trinket.1.is.mad_queens_mandate&trinket.1.cooldown.remains>fight_remains|trinket.2.is.mad_queens_mandate&trinket.2.cooldown.remains>fight_remains)|fight_remains>25" );
  fs_cooldown->add_action( "use_item,name=skardyns_grace,if=(!equipped.mad_queens_mandate|fight_remains>25|trinket.2.is.skardyns_grace&trinket.1.cooldown.remains>fight_remains|trinket.1.is.skardyns_grace&trinket.2.cooldown.remains>fight_remains|trinket.1.cooldown.duration<10|trinket.2.cooldown.duration<10)&buff.metamorphosis.up" );
  fs_cooldown->add_action( "use_item,name=house_of_cards,if=(cooldown.eye_beam.remains<gcd.max|buff.metamorphosis.up)&(raid_event.adds.remains>8|raid_event.adds.in>15)|fight_remains<25" );
  fs_cooldown->add_action( "use_item,name=signet_of_the_priory,if=time<20&(!talent.inertia|buff.inertia.up)&!equipped.cursed_stone_idol|(cooldown.eye_beam.remains<gcd.max|buff.metamorphosis.remains>8|cooldown.metamorphosis.up&buff.metamorphosis.up)&(raid_event.adds.remains>15|raid_event.adds.in>115|fight_style.dungeonroute&!raid_event.adds.in<=120)&(!equipped.cursed_stone_idol|(trinket.1.is.signet_of_the_priory&trinket.2.cooldown.remains>20|trinket.2.is.signet_of_the_priory&trinket.1.cooldown.remains>20))|fight_remains<25" );
  fs_cooldown->add_action( "use_item,name=perfidious_projector,if=variable.tier33_4piece&variable.double_on_use|fight_remains<15" );
  fs_cooldown->add_action( "use_item,name=chaotic_nethergate,if=variable.tier33_4piece&variable.double_on_use|fight_remains<15" );
  fs_cooldown->add_action( "use_item,name=ratfang_toxin,if=variable.tier33_4piece&variable.double_on_use|fight_remains<5" );
  fs_cooldown->add_action( "use_item,name=geargrinders_spare_keys,if=variable.tier33_4piece&variable.double_on_use|fight_remains<10" );
  fs_cooldown->add_action( "use_item,name=grim_codex,if=variable.tier33_4piece&variable.double_on_use|fight_remains<10" );
  fs_cooldown->add_action( "use_item,name=ravenous_honey_buzzer,if=(variable.tier33_4piece&(buff.inertia.down&(cooldown.essence_break.remains&debuff.essence_break.down|!talent.essence_break))&(trinket.1.is.ravenous_honey_buzzer&(trinket.2.cooldown.duration<10|trinket.2.cooldown.remains>10|!trinket.2.has_buff.any)|trinket.2.is.ravenous_honey_buzzer&(trinket.1.cooldown.duration<10|trinket.1.cooldown.remains>10|!trinket.1.has_buff.any))&fight_remains>120|fight_remains<10&fight_remains<buff.metamorphosis.remains)|fight_remains<5" );
  fs_cooldown->add_action( "use_item,name=blastmaster3000,if=variable.tier33_4piece&variable.double_on_use|fight_remains<10" );
  fs_cooldown->add_action( "use_item,name=junkmaestros_mega_magnet,if=variable.tier33_4piece_magnet&variable.double_on_use&time>10|fight_remains<5" );
  fs_cooldown->add_action( "do_treacherous_transmitter_task,if=cooldown.eye_beam.remains>15|cooldown.eye_beam.remains<5|fight_remains<20|buff.metamorphosis.up" );
  fs_cooldown->add_action( "use_item,name=unyielding_netherprism,if=(((cooldown.eye_beam.remains<gcd.max&(active_enemies>1|talent.looks_can_kill)|!talent.chaotic_transformation&buff.metamorphosis.up)&((trinket.1.is.unyielding_netherprism&trinket.2.cooldown.duration>90&variable.trinket2_steroids|cooldown.metamorphosis.remains<=5&buff.latent_energy.stack>10)|(trinket.2.is.unyielding_netherprism&trinket.1.cooldown.duration>90&variable.trinket1_steroids|cooldown.metamorphosis.remains<=5&buff.latent_energy.stack>10))|(buff.metamorphosis.up&((trinket.1.is.unyielding_netherprism&trinket.2.cooldown.duration>90&variable.trinket2_steroids)|(trinket.2.is.unyielding_netherprism&trinket.1.cooldown.duration>90&variable.trinket1_steroids)&!equipped.improvised_seaforium_pacemaker&!equipped.soleahs_secret_technique)))&(raid_event.adds.in>105|raid_event.adds.remains>8)|fight_remains<25)&((trinket.1.is.unyielding_netherprism&(!variable.trinket2_steroids|trinket.2.cooldown.duration<120|trinket.2.cooldown.remains>20)|trinket.2.is.unyielding_netherprism&(!variable.trinket1_steroids|trinket.1.cooldown.duration<120|trinket.1.cooldown.remains>20))|equipped.improvised_seaforium_pacemaker|equipped.soleahs_secret_technique)", "actions.fs_cooldown+=/use_item,name=unyielding_netherprism,if=((cooldown.eye_beam.remains<gcd.max&(active_enemies>1|talent.looks_can_kill)&(buff.latent_energy.stack>11)&((trinket.1.is.unyielding_netherprism&trinket.2.cooldown.duration>=90|cooldown.metamorphosis.remains<=5)|(trinket.2.is.unyielding_netherprism&trinket.1.cooldown.duration>=90|cooldown.metamorphosis.remains<=5)))&(raid_event.adds.in>105|raid_event.adds.remains>8)|fight_remains<25)&((trinket.1.is.unyielding_netherprism&(!variable.trinket2_steroids&!trinket.2.has_cooldown|trinket.2.cooldown.remains>20)|trinket.2.is.unyielding_netherprism&(!variable.trinket1_steroids&!trinket.1.has_cooldown|trinket.1.cooldown.remains>20))|equipped.improvised_seaforium_pacemaker)" );
  fs_cooldown->add_action( "use_item,slot=trinket1,if=((cooldown.eye_beam.remains<gcd.max&active_enemies>1|buff.metamorphosis.up&(cooldown.metamorphosis.remains<buff.metamorphosis.remains|cooldown.metamorphosis.remains>=20|cooldown.metamorphosis.up))&(raid_event.adds.in>trinket.1.cooldown.duration-15|raid_event.adds.remains>8|fight_style.dungeonroute&!raid_event.adds.in<=trinket.1.cooldown.duration)|!trinket.1.has_buff.any|fight_remains<25)&!trinket.1.is.mister_locknstalk&!variable.trinket1_crit&!trinket.1.is.skardyns_grace&!trinket.1.is.treacherous_transmitter&!trinket.1.is.unyielding_netherprism&!trinket.1.is.signet_of_the_priory&(!variable.special_trinket|trinket.2.cooldown.remains>20|(trinket.1.cooldown.duration>90&trinket.1.has_buff.agility))" );
  fs_cooldown->add_action( "use_item,slot=trinket2,if=((cooldown.eye_beam.remains<gcd.max&active_enemies>1|buff.metamorphosis.up&(cooldown.metamorphosis.remains<buff.metamorphosis.remains|cooldown.metamorphosis.remains>=20|cooldown.metamorphosis.up))&(raid_event.adds.in>trinket.2.cooldown.duration-15|raid_event.adds.remains>8|fight_style.dungeonroute&!raid_event.adds.in<=trinket.2.cooldown.duration)|!trinket.2.has_buff.any|fight_remains<25)&!trinket.2.is.mister_locknstalk&!variable.trinket2_crit&!trinket.2.is.skardyns_grace&!trinket.2.is.treacherous_transmitter&!trinket.2.is.unyielding_netherprism&!trinket.2.is.signet_of_the_priory&(!variable.special_trinket|trinket.1.cooldown.remains>20|(trinket.2.cooldown.duration>90&trinket.2.has_buff.agility))" );
  fs_cooldown->add_action( "the_hunt,if=debuff.essence_break.down&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>45)&(buff.metamorphosis.remains>5|buff.metamorphosis.down)&(!talent.initiative|buff.initiative.up|time>5)&time>5&(!talent.inertia&buff.unbound_chaos.down|buff.inertia_trigger.down)&buff.metamorphosis.down|fight_remains<=30" );
  fs_cooldown->add_action( "sigil_of_spite,if=debuff.essence_break.down&cooldown.blade_dance.remains&time>15", "actions.fs_cooldown+=/the_hunt,if=debuff.essence_break.down&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>90)&(debuff.reavers_mark.up|!hero_tree.aldrachi_reaver)&buff.reavers_glaive.down&(buff.metamorphosis.remains>5|buff.metamorphosis.down)&(!talent.initiative|buff.initiative.up|time>5)&time>5&(!talent.inertia&buff.unbound_chaos.down|buff.inertia_trigger.down)&(!talent.inertia&(hero_tree.aldrachi_reaver|buff.metamorphosis.down)|hero_tree.felscarred&cooldown.metamorphosis.up|fight_remains<cooldown.metamorphosis.remains)" );

  fs_fel_barrage->add_action( "variable,name=generator_up,op=set,value=cooldown.felblade.remains<gcd.max|cooldown.sigil_of_flame.remains<gcd.max" );
  fs_fel_barrage->add_action( "variable,name=gcd_drain,op=set,value=gcd.max*32" );
  fs_fel_barrage->add_action( "annihilation,if=buff.inner_demon.up" );
  fs_fel_barrage->add_action( "eye_beam,if=(buff.fel_barrage.down|fury>45&talent.blind_fury)&(active_enemies>1&raid_event.adds.up|raid_event.adds.in>40-buff.cycle_of_hatred.stack*5)" );
  fs_fel_barrage->add_action( "essence_break,if=buff.fel_barrage.down&buff.metamorphosis.up" );
  fs_fel_barrage->add_action( "death_sweep,if=buff.fel_barrage.down" );
  fs_fel_barrage->add_action( "immolation_aura,if=(active_enemies>2|buff.fel_barrage.up)&(cooldown.eye_beam.remains>recharge_time+3)" );
  fs_fel_barrage->add_action( "glaive_tempest,if=buff.fel_barrage.down&active_enemies>1" );
  fs_fel_barrage->add_action( "blade_dance,if=buff.fel_barrage.down" );
  fs_fel_barrage->add_action( "fel_barrage,if=fury>100&(raid_event.adds.in>90|raid_event.adds.in<gcd.max|raid_event.adds.remains>4&active_enemies>2)" );
  fs_fel_barrage->add_action( "felblade,if=buff.inertia_trigger.up&buff.fel_barrage.up" );
  fs_fel_barrage->add_action( "fel_rush,if=buff.unbound_chaos.up&fury>20&buff.fel_barrage.up" );
  fs_fel_barrage->add_action( "sigil_of_flame,if=fury.deficit>40&buff.fel_barrage.up&(!talent.student_of_suffering|cooldown.eye_beam.remains>30)" );
  fs_fel_barrage->add_action( "sigil_of_flame,if=buff.demonsurge_hardcast.up&fury.deficit>40&buff.fel_barrage.up" );
  fs_fel_barrage->add_action( "felblade,if=buff.fel_barrage.up&fury.deficit>40&action.felblade.cooldown_react" );
  fs_fel_barrage->add_action( "death_sweep,if=fury-variable.gcd_drain-35>0&(buff.fel_barrage.remains<3|variable.generator_up|fury>80|variable.fury_gen>18)" );
  fs_fel_barrage->add_action( "glaive_tempest,if=fury-variable.gcd_drain-30>0&(buff.fel_barrage.remains<3|variable.generator_up|fury>80|variable.fury_gen>18)" );
  fs_fel_barrage->add_action( "blade_dance,if=fury-variable.gcd_drain-35>0&(buff.fel_barrage.remains<3|variable.generator_up|fury>80|variable.fury_gen>18)" );
  fs_fel_barrage->add_action( "arcane_torrent,if=fury.deficit>40&buff.fel_barrage.up" );
  fs_fel_barrage->add_action( "fel_rush,if=buff.unbound_chaos.up" );
  fs_fel_barrage->add_action( "the_hunt,if=fury>40&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>80)" );
  fs_fel_barrage->add_action( "annihilation,if=fury-variable.gcd_drain-40>20&(buff.fel_barrage.remains<3|variable.generator_up|fury>80|variable.fury_gen>18)" );
  fs_fel_barrage->add_action( "chaos_strike,if=fury-variable.gcd_drain-40>20&(cooldown.fel_barrage.remains&cooldown.fel_barrage.remains<10&fury>100|buff.fel_barrage.up&(buff.fel_barrage.remains*variable.fury_gen-buff.fel_barrage.remains*32)>0)" );
  fs_fel_barrage->add_action( "demons_bite" );

  fs_meta->add_action( "death_sweep,if=buff.metamorphosis.remains<gcd.max|debuff.essence_break.up&((buff.immolation_aura.down|!variable.fs_tier34_2piece)&(buff.demon_soul_tww3.down|!set_bonus.thewarwithin_season_3_4pc)|talent.dancing_with_fate.rank=2)|prev_gcd.1.metamorphosis&(!variable.fs_tier34_2piece|talent.dancing_with_fate.rank=2)|buff.demonsurge_death_sweep.up&variable.fs_tier34_2piece&buff.demonsurge.remains<5|(variable.fs_tier34_2piece&cooldown.metamorphosis.up&talent.inertia)|active_enemies>=3&buff.demonsurge_death_sweep.up&(!talent.inertia|buff.inertia_trigger.down&cooldown.vengeful_retreat.remains|buff.inertia.up)&(!talent.essence_break|debuff.essence_break.up|cooldown.essence_break.remains>=5)" );
  fs_meta->add_action( "sigil_of_flame,if=buff.demonsurge_hardcast.up&talent.student_of_suffering&debuff.essence_break.down&(talent.student_of_suffering&((talent.essence_break&cooldown.essence_break.remains>30-gcd.max|cooldown.essence_break.remains<=gcd.max+talent.inertia&(cooldown.vengeful_retreat.remains<=gcd|buff.initiative.up)+gcd.max*(cooldown.eye_beam.remains<=gcd.max))|(!talent.essence_break&(cooldown.eye_beam.remains>=10|cooldown.eye_beam.remains<=gcd.max))))" );
  fs_meta->add_action( "sigil_of_flame,if=buff.demonsurge_hardcast.up&buff.demonsurge_sigil_of_doom.up&(buff.demonsurge.remains<5|cooldown.blade_dance.remains&buff.metamorphosis.remains<=gcd.max)&!debuff.essence_break.up" );
  fs_meta->add_action( "vengeful_retreat,use_off_gcd=1,if=talent.initiative&(gcd.remains<0.3|talent.inertia&cooldown.eye_beam.remains>gcd.remains&(buff.cycle_of_hatred.stack=2|buff.cycle_of_hatred.stack=3))&(cooldown.metamorphosis.remains&(buff.demonsurge_annihilation.down&buff.demonsurge_death_sweep.down)|talent.restless_hunter&buff.demonsurge_annihilation.down)&(!talent.inertia&buff.unbound_chaos.down|buff.inertia_trigger.down)&(!talent.essence_break|cooldown.essence_break.remains>18|cooldown.essence_break.remains<=gcd.remains+talent.inertia*1.5&(!talent.student_of_suffering|(buff.student_of_suffering.up|cooldown.sigil_of_flame.remains>5)))&(cooldown.eye_beam.remains>5|cooldown.eye_beam.remains<=gcd.remains|cooldown.eye_beam.up)|cooldown.metamorphosis.up&buff.demonsurge.stack>1&talent.initiative&!talent.inertia&gcd.remains<0.3" );
  fs_meta->add_action( "vengeful_retreat,use_off_gcd=1,if=variable.fs_tier34_2piece&buff.inertia_trigger.down&talent.initiative" );
  fs_meta->add_action( "felblade,if=talent.inertia&variable.fs_tier34_2piece&buff.inertia_trigger.up" );
  fs_meta->add_action( "death_sweep,if=(talent.essence_break&buff.demonsurge_death_sweep.up&(buff.inertia.up&(cooldown.essence_break.remains>buff.inertia.remains|!talent.essence_break)|cooldown.metamorphosis.remains<=5&buff.inertia_trigger.down|buff.inertia.up&buff.demonsurge_abyssal_gaze.up)|talent.inertia&buff.inertia_trigger.down&cooldown.vengeful_retreat.remains>=gcd.max&buff.inertia.down)&(!variable.fs_tier34_2piece|(variable.fs_tier34_2piece&(!talent.inertia|active_enemies>=3&debuff.essence_break.up|talent.dancing_with_fate=2)))", "&active_enemies<3 actions.fs_meta+=/fel_rush,if=talent.inertia&variable.fs_tier34_2piece&buff.inertia_trigger.up&(active_enemies>=3|cooldown.felblade.remains) actions.fs_meta+=/felblade,if=talent.inertia&buff.inertia_trigger.up&cooldown.essence_break.remains<=1&hero_tree.aldrachi_reaver&cooldown.blade_dance.remains<=gcd.max*2&cooldown.metamorphosis.remains<=gcd.max*3 actions.fs_meta+=/felblade,if=talent.inertia&buff.inertia_trigger.up&debuff.essence_break.down&buff.demonsurge_hardcast.up&buff.demonsurge.stack=0&buff.demonsurge_death_sweep.up actions.fs_meta+=/fel_rush,if=talent.inertia&buff.inertia_trigger.up&debuff.essence_break.down&buff.demonsurge_hardcast.up&buff.demonsurge.stack=0&buff.demonsurge_death_sweep.up&cooldown.felblade.remains actions.fs_meta+=/fel_rush,if=talent.inertia&buff.inertia_trigger.up&cooldown.essence_break.remains<=1&hero_tree.aldrachi_reaver&cooldown.blade_dance.remains<=gcd.max*2&cooldown.metamorphosis.remains<=gcd.max*3 actions.fs_meta+=/essence_break,if=fury>=30&talent.restless_hunter&cooldown.metamorphosis.up&(talent.inertia&buff.inertia.up|!talent.inertia)&cooldown.blade_dance.remains<=gcd.max&(hero_tree.felscarred&buff.demonsurge_annihilation.down|hero_tree.aldrachi_reaver)" );
  fs_meta->add_action( "annihilation,if=buff.metamorphosis.remains<gcd.max&cooldown.blade_dance.remains<buff.metamorphosis.remains|debuff.essence_break.remains&debuff.essence_break.remains<0.5|talent.restless_hunter&(buff.demonsurge_annihilation.up|hero_tree.aldrachi_reaver&buff.inner_demon.up)&cooldown.essence_break.up&cooldown.metamorphosis.up" );
  fs_meta->add_action( "annihilation,if=(buff.demonsurge_annihilation.up&talent.restless_hunter)&(cooldown.eye_beam.remains<gcd.max*3&cooldown.blade_dance.remains|cooldown.metamorphosis.remains<gcd.max*3)" );
  fs_meta->add_action( "felblade,if=buff.inertia_trigger.up&talent.inertia&debuff.essence_break.down&cooldown.metamorphosis.remains&cooldown.eye_beam.remains&(cooldown.blade_dance.remains<=5.5&(talent.essence_break&cooldown.essence_break.remains<=0.5|!talent.essence_break|cooldown.essence_break.remains>=buff.inertia_trigger.remains&cooldown.blade_dance.remains<=4.5&(cooldown.blade_dance.remains|cooldown.blade_dance.remains<=0.5))|buff.metamorphosis.remains<=5.5+talent.shattered_destiny*2)" );
  fs_meta->add_action( "fel_rush,if=buff.inertia_trigger.up&talent.inertia&debuff.essence_break.down&cooldown.metamorphosis.remains&cooldown.eye_beam.remains&(cooldown.felblade.remains&cooldown.essence_break.remains<=0.6|active_enemies>2)" );
  fs_meta->add_action( "immolation_aura,if=(active_enemies>1|talent.a_fire_inside&(talent.isolated_prey|variable.fs_tier34_2piece))&debuff.essence_break.down&(active_enemies>=3|full_recharge_time<gcd.max*2|variable.fs_tier34_2piece&buff.immolation_aura.remains<=gcd.max|variable.fs_tier34_2piece&buff.immolation_aura.down)", "|cooldown.felblade.remains&buff.metamorphosis.remains<=5.6-talent.shattered_destiny*gcd.max*2) actions.fs_meta+=/felblade,if=buff.inertia_trigger.up&talent.inertia&debuff.essence_break.down&cooldown.metamorphosis.remains&(!hero_tree.felscarred|cooldown.eye_beam.remains&(!buff.demonsurge_hardcast.up|cooldown.essence_break.remains<=0.5)|buff.demonsurge_hardcast.up&cooldown.eye_beam.remains<=0.6) actions.fs_meta+=/fel_rush,if=buff.inertia_trigger.up&talent.inertia&debuff.essence_break.down&cooldown.metamorphosis.remains&(!hero_tree.felscarred|cooldown.eye_beam.remains&(!buff.demonsurge_hardcast.up|cooldown.essence_break.remains<=0.5)|buff.demonsurge_hardcast.up&cooldown.eye_beam.remains<=gcd.max)&(active_enemies>2|hero_tree.felscarred)&cooldown.felblade.remains actions.fs_meta+=/felblade,if=buff.inertia_trigger.up&talent.inertia&debuff.essence_break.down&cooldown.blade_dance.remains<gcd.max*3&(!hero_tree.felscarred|cooldown.eye_beam.remains)&cooldown.metamorphosis.remains actions.fs_meta+=/fel_rush,if=buff.inertia_trigger.up&talent.inertia&debuff.essence_break.down&cooldown.blade_dance.remains<gcd.max*3&(!hero_tree.felscarred|cooldown.eye_beam.remains)&cooldown.metamorphosis.remains&(active_enemies>2|hero_tree.felscarred)" );
  fs_meta->add_action( "annihilation,if=buff.inner_demon.up&cooldown.blade_dance.remains&(cooldown.eye_beam.remains<gcd.max*3|cooldown.metamorphosis.remains<gcd.max*3)" );
  fs_meta->add_action( "essence_break,if=fury>20&(cooldown.metamorphosis.remains>10|cooldown.blade_dance.remains<gcd.max*2&!variable.fs_tier34_2piece|variable.fs_tier34_2piece&buff.immolation_aura.up)&(buff.inertia_trigger.down|buff.inertia.up&buff.inertia.remains>=gcd.max*3|!talent.inertia|active_enemies>desired_targets&raid_event.adds.remains<cooldown.vengeful_retreat.remains+5)&buff.out_of_range.remains<gcd.max&(!talent.shattered_destiny|cooldown.eye_beam.remains>4)&(active_enemies>1|cooldown.metamorphosis.remains>5&cooldown.eye_beam.remains)&(!buff.cycle_of_hatred.stack=3|buff.initiative.up|!talent.initiative|!talent.cycle_of_hatred|talent.inertia)|fight_remains<5", "actions.fs_meta+=/sigil_of_doom,if=debuff.essence_break.down&(buff.demonsurge_sigil_of_doom.up&cooldown.blade_dance.remains|talent.student_of_suffering&((talent.essence_break&cooldown.essence_break.remains>30-gcd.max|cooldown.essence_break.remains<=gcd.max*3&(!talent.inertia|buff.inertia_trigger.up))|(!talent.essence_break&(cooldown.eye_beam.remains>=30|cooldown.eye_beam.remains<=gcd.max))))" );
  fs_meta->add_action( "sigil_of_flame,if=buff.demonsurge_hardcast.up&buff.demonsurge_death_sweep.down&debuff.essence_break.down&(cooldown.eye_beam.remains>=20|cooldown.eye_beam.remains<=gcd.max)&(!talent.student_of_suffering|buff.demonsurge_sigil_of_doom.up)" );
  fs_meta->add_action( "immolation_aura,if=!variable.fs_tier34_2piece&buff.demonsurge.up&debuff.essence_break.down&buff.demonsurge_consuming_fire.up&cooldown.blade_dance.remains>=gcd.max&cooldown.eye_beam.remains>=gcd.max&fury.deficit>10+variable.fury_gen" );
  fs_meta->add_action( "eye_beam,if=debuff.essence_break.down&buff.inner_demon.down&(buff.metamorphosis.remains>=7|!set_bonus.thewarwithin_season_3_4pc)" );
  fs_meta->add_action( "eye_beam,if=buff.demonsurge_hardcast.up&debuff.essence_break.down&buff.inner_demon.down&(buff.cycle_of_hatred.stack<4|cooldown.essence_break.remains>=20-gcd.max*talent.student_of_suffering|cooldown.sigil_of_flame.remains&talent.student_of_suffering|cooldown.essence_break.remains<=gcd.max|!talent.essence_break)&(buff.metamorphosis.remains>=7|!set_bonus.thewarwithin_season_3_4pc)" );
  fs_meta->add_action( "death_sweep,if=(cooldown.essence_break.remains>=gcd.max*2+talent.student_of_suffering*gcd.max|debuff.essence_break.up|!talent.essence_break)&(buff.immolation_aura.down|!variable.fs_tier34_2piece|talent.screaming_brutality&talent.soulscar|talent.dancing_with_fate.rank=2)&(buff.demon_soul_tww3.down|!set_bonus.thewarwithin_season_3_4pc|active_enemies>=3|talent.screaming_brutality&talent.soulscar|talent.dancing_with_fate.rank=2)" );
  fs_meta->add_action( "glaive_tempest,if=debuff.essence_break.down&(cooldown.blade_dance.remains>gcd.max*2|fury>60)&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>10)" );
  fs_meta->add_action( "sigil_of_flame,if=active_enemies>2&debuff.essence_break.down" );
  fs_meta->add_action( "annihilation,if=cooldown.blade_dance.remains|fury>60|soul_fragments.total>0|buff.metamorphosis.remains<5", "actions.fs_meta+=/throw_glaive,if=talent.soulscar&talent.furious_throws&active_enemies>1&debuff.essence_break.down" );
  fs_meta->add_action( "sigil_of_flame,if=buff.metamorphosis.remains>5&buff.out_of_range.down&!talent.student_of_suffering" );
  fs_meta->add_action( "immolation_aura,if=!variable.fs_tier34_2piece&buff.out_of_range.down&recharge_time<(cooldown.eye_beam.remains<?buff.metamorphosis.remains)&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>full_recharge_time)", "actions.fs_meta+=/felblade,if=(buff.out_of_range.down|fury.deficit>40+variable.fury_gen*(0.5%gcd.max))&!buff.inertia.up actions.fs_meta+=/sigil_of_flame,if=debuff.essence_break.down&buff.out_of_range.down" );
  fs_meta->add_action( "felblade,if=(buff.out_of_range.down|fury.deficit>40+variable.fury_gen*(0.5%gcd.max))&!buff.inertia_trigger.up" );
  fs_meta->add_action( "annihilation" );
  fs_meta->add_action( "throw_glaive,if=buff.unbound_chaos.down&recharge_time<cooldown.eye_beam.remains&debuff.essence_break.down&(cooldown.eye_beam.remains>8|charges_fractional>1.01)&buff.out_of_range.down&active_enemies>1&!talent.furious_throws" );
  fs_meta->add_action( "fel_rush,if=recharge_time<cooldown.eye_beam.remains&debuff.essence_break.down&(cooldown.eye_beam.remains>8|charges_fractional>1.01)&buff.out_of_range.down&active_enemies>1" );
  fs_meta->add_action( "demons_bite" );

  fs_opener->add_action( "potion,if=buff.initiative.up|!talent.initiative" );
  fs_opener->add_action( "felblade,if=cooldown.the_hunt.up&!talent.a_fire_inside&fury<40" );
  fs_opener->add_action( "the_hunt,if=talent.inertia|buff.initiative.up|!talent.initiative" );
  fs_opener->add_action( "felblade,if=talent.inertia&buff.inertia_trigger.up&set_bonus.thewarwithin_season_3_4pc&buff.metamorphosis.up&debuff.essence_break.down&active_enemies<=2" );
  fs_opener->add_action( "fel_rush,if=talent.inertia&buff.inertia_trigger.up&set_bonus.thewarwithin_season_3_4pc&buff.metamorphosis.up&debuff.essence_break.down&(active_enemies>=2|cooldown.felblade.remains)" );
  fs_opener->add_action( "immolation_aura,if=variable.fs_tier34_2piece&buff.demonsurge_hardcast.up&(buff.demonsurge_consuming_fire.up|charges=2)" );
  fs_opener->add_action( "annihilation,if=variable.fs_tier34_2piece&debuff.essence_break.down&cooldown.metamorphosis.remains&buff.demonsurge_annihilation.up&cooldown.eye_beam.up" );
  fs_opener->add_action( "felblade,if=talent.inertia&buff.inertia_trigger.up&active_enemies=1&buff.metamorphosis.up&cooldown.metamorphosis.up&cooldown.essence_break.up&buff.inner_demon.down&buff.demonsurge_annihilation.down" );
  fs_opener->add_action( "fel_rush,if=talent.inertia&buff.inertia_trigger.up&(cooldown.felblade.remains|active_enemies>1)&buff.metamorphosis.up&cooldown.metamorphosis.up&cooldown.essence_break.up&buff.inner_demon.down&buff.demonsurge_annihilation.down" );
  fs_opener->add_action( "essence_break,if=buff.metamorphosis.up&(!talent.inertia|buff.inertia.up&(buff.inner_demon.down|!talent.chaotic_transformation))&(buff.demonsurge_annihilation.down|!talent.chaotic_transformation)&(!variable.fs_tier34_2piece|buff.demonsurge_hardcast.up&cooldown.eye_beam.remains&buff.demonsurge_consuming_fire.down)" );
  fs_opener->add_action( "vengeful_retreat,use_off_gcd=1,if=talent.initiative&time>4&buff.metamorphosis.up&(!talent.inertia|buff.inertia_trigger.down)&talent.essence_break&buff.inner_demon.down&(buff.initiative.down|gcd.remains<0.1)&cooldown.blade_dance.remains" );
  fs_opener->add_action( "felblade,if=talent.inertia&buff.inertia_trigger.up&hero_tree.felscarred&debuff.essence_break.down&talent.essence_break&cooldown.metamorphosis.remains&active_enemies<=2&cooldown.sigil_of_flame.remains", "actions.fs_opener+=/felblade,if=!talent.inertia&active_enemies=1&buff.unbound_chaos.up&buff.initiative.up&debuff.essence_break.down" );
  fs_opener->add_action( "sigil_of_flame,if=buff.demonsurge_hardcast.up&(buff.inner_demon.down|buff.out_of_range.up)&debuff.essence_break.down&(!variable.fs_tier34_2piece|cooldown.essence_break.remains&buff.inertia.down&(!talent.inertia|buff.immolation_aura.down)|!talent.essence_break|fury<=40)", "actions.fs_opener+=/immolation_aura,if=hero_tree.felscarred&charges=2&buff.student_of_suffering.up&talent.a_fire_inside&cooldown.sigil_of_flame.remains&debuff.essence_break.down actions.fs_opener+=/immolation_aura,if=hero_tree.felscarred&debuff.essence_break.down&talent.a_fire_inside&buff.metamorphosis.remains&charges=2 actions.fs_opener+=/felblade,if=buff.inertia_trigger.up&talent.inertia&talent.restless_hunter&cooldown.essence_break.up&cooldown.metamorphosis.up&(buff.demonsurge_annihilation.down|hero_tree.aldrachi_reaver)&buff.metamorphosis.up&cooldown.blade_dance.remains<=gcd.max actions.fs_opener+=/fel_rush,if=buff.inertia_trigger.up&talent.inertia&talent.restless_hunter&cooldown.essence_break.up&cooldown.metamorphosis.up&(buff.demonsurge_annihilation.down|hero_tree.aldrachi_reaver)&buff.metamorphosis.up&cooldown.blade_dance.remains<=gcd.max actions.fs_opener+=/felblade,if=talent.inertia&buff.inertia_trigger.up&(buff.inertia.down&buff.metamorphosis.up&!hero_tree.felscarred|hero_tree.felscarred&(buff.metamorphosis.down&charges>1|prev_gcd.1.eye_beam|buff.demonsurge.stack>=5))&debuff.essence_break.down actions.fs_opener+=/fel_rush,if=talent.inertia&buff.unbound_chaos.up&talent.a_fire_inside&(buff.inertia.down&buff.metamorphosis.up&!hero_tree.felscarred|hero_tree.felscarred&(buff.metamorphosis.down&charges>1|prev_gcd.1.eye_beam|buff.demonsurge.stack>=5|charges=2&buff.unbound_chaos.down))&debuff.essence_break.down actions.fs_opener+=/the_hunt,if=(buff.metamorphosis.up&hero_tree.aldrachi_reaver&talent.shattered_destiny|!talent.shattered_destiny&hero_tree.aldrachi_reaver|hero_tree.felscarred)&(!talent.initiative|talent.inertia|buff.initiative.up|time>5)" );
  fs_opener->add_action( "annihilation,if=(buff.inner_demon.up|buff.demonsurge_annihilation.up)&(cooldown.metamorphosis.up|!talent.essence_break&cooldown.blade_dance.remains)" );
  fs_opener->add_action( "death_sweep,if=buff.demonsurge_death_sweep.up&!talent.restless_hunter&(!variable.fs_tier34_2piece|buff.demonsurge_hardcast.down)" );
  fs_opener->add_action( "annihilation,if=buff.demonsurge_annihilation.up&(!talent.essence_break|buff.inner_demon.up)" );
  fs_opener->add_action( "immolation_aura,if=talent.a_fire_inside&talent.burning_wound&buff.metamorphosis.down" );
  fs_opener->add_action( "felblade,if=fury<40&debuff.essence_break.down&buff.inertia_trigger.down&cooldown.metamorphosis.up" );
  fs_opener->add_action( "metamorphosis,if=buff.metamorphosis.up&buff.inner_demon.down&buff.demonsurge_annihilation.down&cooldown.blade_dance.remains" );
  fs_opener->add_action( "eye_beam,if=buff.metamorphosis.down|debuff.essence_break.down&buff.inner_demon.down&(cooldown.blade_dance.remains|talent.essence_break&cooldown.essence_break.up)&(!talent.a_fire_inside|action.immolation_aura.charges=0)", "actions.fs_opener+=/sigil_of_spite,if=hero_tree.felscarred" );
  fs_opener->add_action( "eye_beam,if=buff.demonsurge_hardcast.up&debuff.essence_break.down&buff.inner_demon.down" );
  fs_opener->add_action( "annihilation,if=variable.fs_tier34_2piece&(buff.immolation_aura.up|buff.demon_soul_tww3.up)", "actions.fs_opener+=/essence_break,if=cooldown.blade_dance.remains<gcd.max&!hero_tree.felscarred&!talent.shattered_destiny&buff.metamorphosis.up|(cooldown.eye_beam.remains|buff.demonsurge_abyssal_gaze.down)&cooldown.metamorphosis.remains&(!talent.inertia|buff.inertia_trigger.down)" );
  fs_opener->add_action( "death_sweep" );
  fs_opener->add_action( "annihilation" );
  fs_opener->add_action( "demons_bite" );
}
//havoc_apl_end

//havoc_ptr_apl_start
//havoc_ptr_apl_end

//vengeance_apl_start
void vengeance( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* ar = p->get_action_priority_list( "ar" );
  action_priority_list_t* externals = p->get_action_priority_list( "externals" );
  action_priority_list_t* fel_dev = p->get_action_priority_list( "fel_dev" );
  action_priority_list_t* fel_dev_prep = p->get_action_priority_list( "fel_dev_prep" );
  action_priority_list_t* fs = p->get_action_priority_list( "fs" );
  action_priority_list_t* fs_execute = p->get_action_priority_list( "fs_execute" );
  action_priority_list_t* meta_prep = p->get_action_priority_list( "meta_prep" );
  action_priority_list_t* metamorphosis = p->get_action_priority_list( "metamorphosis" );

  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=single_target,value=spell_targets.spirit_bomb=1" );
  precombat->add_action( "variable,name=small_aoe,value=spell_targets.spirit_bomb>=2&spell_targets.spirit_bomb<=5" );
  precombat->add_action( "variable,name=big_aoe,value=spell_targets.spirit_bomb>=6" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_use_buff|(trinket.1.has_buff.agility|trinket.1.has_buff.mastery|trinket.1.has_buff.versatility|trinket.1.has_buff.haste|trinket.1.has_buff.crit)" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_use_buff|(trinket.2.has_buff.agility|trinket.2.has_buff.mastery|trinket.2.has_buff.versatility|trinket.2.has_buff.haste|trinket.2.has_buff.crit)" );
  precombat->add_action( "arcane_torrent" );
  precombat->add_action( "sigil_of_flame,if=hero_tree.aldrachi_reaver|(hero_tree.felscarred&talent.student_of_suffering)" );
  precombat->add_action( "immolation_aura" );

  default_->add_action( "variable,name=num_spawnable_souls,op=reset,default=0" );
  default_->add_action( "variable,name=num_spawnable_souls,op=max,value=1,if=talent.soul_sigils&cooldown.sigil_of_flame.up" );
  default_->add_action( "variable,name=num_spawnable_souls,op=max,value=2,if=talent.fracture&cooldown.fracture.charges_fractional>=1&!buff.metamorphosis.up" );
  default_->add_action( "variable,name=num_spawnable_souls,op=max,value=3,if=talent.fracture&cooldown.fracture.charges_fractional>=1&buff.metamorphosis.up" );
  default_->add_action( "variable,name=num_spawnable_souls,op=add,value=1,if=talent.soul_carver&(cooldown.soul_carver.remains>(cooldown.soul_carver.duration-3))" );
  default_->add_action( "auto_attack" );
  default_->add_action( "disrupt,if=target.debuff.casting.react" );
  default_->add_action( "infernal_strike,use_off_gcd=1" );
  default_->add_action( "demon_spikes,use_off_gcd=1,if=!buff.demon_spikes.up&!cooldown.pause_action.remains" );
  default_->add_action( "run_action_list,name=ar,if=hero_tree.aldrachi_reaver" );
  default_->add_action( "run_action_list,name=fs,if=hero_tree.felscarred" );

  ar->add_action( "use_item,slot=trinket1,if=!trinket.1.is.tome_of_lights_devotion&(!variable.trinket_1_buffs|(variable.trinket_1_buffs&((buff.metamorphosis.up)|(buff.metamorphosis.up&cooldown.metamorphosis.remains<10)|(cooldown.metamorphosis.remains>trinket.1.cooldown.duration)|(variable.trinket_2_buffs&trinket.2.cooldown.remains<cooldown.metamorphosis.remains))))" );
  ar->add_action( "use_item,slot=trinket2,if=!trinket.2.is.tome_of_lights_devotion&(!variable.trinket_2_buffs|(variable.trinket_2_buffs&((buff.metamorphosis.up)|(buff.metamorphosis.up&cooldown.metamorphosis.remains<10)|(cooldown.metamorphosis.remains>trinket.2.cooldown.duration)|(variable.trinket_1_buffs&trinket.1.cooldown.remains<cooldown.metamorphosis.remains))))" );
  ar->add_action( "use_item,name=tome_of_lights_devotion,if=buff.inner_resilience.up" );
  ar->add_action( "potion,use_off_gcd=1,if=(buff.rending_strike.up&buff.glaive_flurry.up)|prev_gcd.1.reavers_glaive" );
  ar->add_action( "call_action_list,name=externals,if=(buff.rending_strike.up&buff.glaive_flurry.up)|prev_gcd.1.reavers_glaive" );
  ar->add_action( "metamorphosis,use_off_gcd=1,if=!buff.metamorphosis.up" );
  ar->add_action( "fel_devastation,use_off_gcd=1,if=!buff.rending_strike.up&!buff.glaive_flurry.up" );
  ar->add_action( "soul_cleave,if=!buff.rending_strike.up&buff.glaive_flurry.up", "Always Soul Cleave if Rending Strike isn't up and Glaive Flurry is" );
  ar->add_action( "shear,if=talent.fracture&buff.glaive_flurry.up", "Spend Rending Strike or generate Fury for empowered Soul Cleave" );
  ar->add_action( "shear,if=!talent.fracture&buff.glaive_flurry.up", "Spend Rending Strike or generate Fury for empowered Soul Cleave" );
  ar->add_action( "reavers_glaive,if=!buff.rending_strike.up&!buff.glaive_flurry.up" );
  ar->add_action( "the_hunt,if=!buff.reavers_glaive.up&(buff.art_of_the_glaive.stack+soul_fragments.total)<20" );
  ar->add_action( "fiery_brand,if=talent.fiery_demise&!dot.fiery_brand.ticking" );
  ar->add_action( "soul_carver,if=!talent.fiery_demise|(talent.fiery_demise&dot.fiery_brand.ticking)" );
  ar->add_action( "sigil_of_spite" );
  ar->add_action( "immolation_aura,if=talent.fallout", "Immolation Aura is one of our best generators if Fallout is talented" );
  ar->add_action( "bulk_extraction,if=spell_targets>=3" );
  ar->add_action( "shear,if=talent.fracture&buff.metamorphosis.up" );
  ar->add_action( "sigil_of_flame" );
  ar->add_action( "shear,if=talent.fracture" );
  ar->add_action( "spirit_bomb,if=spell_targets>=12&soul_fragments>=4" );
  ar->add_action( "soul_cleave" );
  ar->add_action( "immolation_aura" );
  ar->add_action( "felblade" );
  ar->add_action( "vengeful_retreat,if=talent.unhindered_assault" );
  ar->add_action( "throw_glaive" );
  ar->add_action( "shear,if=!talent.fracture" );

  externals->add_action( "invoke_external_buff,name=symbol_of_hope" );
  externals->add_action( "invoke_external_buff,name=power_infusion" );

  fel_dev->add_action( "spirit_bomb,if=buff.demonsurge_spirit_burst.up&(variable.can_spburst|soul_fragments>=4|(buff.metamorphosis.remains<(gcd.max*2)))" );
  fel_dev->add_action( "soul_cleave,if=buff.demonsurge_soul_sunder.up&(!buff.demonsurge_spirit_burst.up|(buff.metamorphosis.remains<(gcd.max*2)))" );
  fel_dev->add_action( "sigil_of_spite,if=(!talent.cycle_of_binding|(cooldown.sigil_of_spite.duration<(cooldown.metamorphosis.remains+18)))&(soul_fragments.total<=2&buff.demonsurge_spirit_burst.up)" );
  fel_dev->add_action( "soul_carver,if=soul_fragments.total<=2&!prev_gcd.1.sigil_of_spite&buff.demonsurge_spirit_burst.up" );
  fel_dev->add_action( "shear,if=talent.fracture&soul_fragments.total<=2&buff.demonsurge_spirit_burst.up" );
  fel_dev->add_action( "felblade,if=buff.demonsurge_spirit_burst.up|buff.demonsurge_soul_sunder.up" );
  fel_dev->add_action( "shear,if=talent.fracture&buff.demonsurge_spirit_burst.up|buff.demonsurge_soul_sunder.up" );

  fel_dev_prep->add_action( "potion,use_off_gcd=1,if=prev_gcd.1.fiery_brand" );
  fel_dev_prep->add_action( "sigil_of_flame,if=!variable.hold_sof_for_precombat&!variable.hold_sof_for_student&!variable.hold_sof_for_dot" );
  fel_dev_prep->add_action( "fiery_brand,if=talent.fiery_demise&((fury+variable.fel_dev_passive_fury_gen)>=120)&(variable.can_spburst|variable.can_spburst_soon|soul_fragments.total>=4)&active_dot.fiery_brand=0&((cooldown.metamorphosis.remains<(execute_time+action.fel_devastation.execute_time+(gcd.max*2)))|variable.fiery_brand_back_before_meta)" );
  fel_dev_prep->add_action( "fel_devastation,if=((fury+variable.fel_dev_passive_fury_gen)>=120)&(variable.can_spburst|variable.can_spburst_soon|soul_fragments.total>=4)" );
  fel_dev_prep->add_action( "sigil_of_spite,if=(!talent.cycle_of_binding|(cooldown.sigil_of_spite.duration<(cooldown.metamorphosis.remains+18)))&(soul_fragments.total<=1|(!(variable.can_spburst|variable.can_spburst_soon|soul_fragments.total>=4)&(!talent.fracture|action.shear.charges_fractional<1)))" );
  fel_dev_prep->add_action( "soul_carver,if=(!talent.cycle_of_binding|cooldown.metamorphosis.remains>20)&(soul_fragments.total<=1|(!(variable.can_spburst|variable.can_spburst_soon|soul_fragments.total>=4)&(!talent.fracture|action.shear.charges_fractional<1)))&!prev_gcd.1.sigil_of_spite&!prev_gcd.2.sigil_of_spite" );
  fel_dev_prep->add_action( "felblade,if=!((fury+variable.fel_dev_passive_fury_gen)>=120)&(variable.can_spburst|variable.can_spburst_soon|soul_fragments.total>=4)" );
  fel_dev_prep->add_action( "shear,if=talent.fracture&!(variable.can_spburst|variable.can_spburst_soon|soul_fragments.total>=4)|!((fury+variable.fel_dev_passive_fury_gen)>=120)" );
  fel_dev_prep->add_action( "felblade" );
  fel_dev_prep->add_action( "shear,if=talent.fracture" );
  fel_dev_prep->add_action( "wait,sec=0.1,if=(!(variable.can_spburst|variable.can_spburst_soon|soul_fragments.total>=4)|!((fury+variable.fel_dev_passive_fury_gen)>=120))&(!talent.fracture|action.shear.charges_fractional>=0.7)" );
  fel_dev_prep->add_action( "fel_devastation" );
  fel_dev_prep->add_action( "soul_cleave,if=((fury+variable.fel_dev_passive_fury_gen)>=150)" );
  fel_dev_prep->add_action( "throw_glaive" );

  fs->add_action( "variable,name=crit_pct,op=set,value=(dot.sigil_of_flame.crit_pct+(talent.aura_of_pain*6))%100,if=active_dot.sigil_of_flame>0&talent.volatile_flameblood" );
  fs->add_action( "variable,name=fel_dev_sequence_time,op=set,value=2+(2*gcd.max)" );
  fs->add_action( "variable,name=fel_dev_sequence_time,op=add,value=gcd.max,if=talent.fiery_demise&cooldown.fiery_brand.up" );
  fs->add_action( "variable,name=fel_dev_sequence_time,op=add,value=gcd.max,if=cooldown.sigil_of_flame.up|cooldown.sigil_of_flame.remains<variable.fel_dev_sequence_time" );
  fs->add_action( "variable,name=fel_dev_sequence_time,op=add,value=gcd.max,if=cooldown.immolation_aura.up|cooldown.immolation_aura.remains<variable.fel_dev_sequence_time" );
  fs->add_action( "variable,name=fel_dev_passive_fury_gen,op=set,value=0" );
  fs->add_action( "variable,name=fel_dev_passive_fury_gen,op=add,value=2.5*floor((buff.student_of_suffering.remains>?variable.fel_dev_sequence_time)),if=talent.student_of_suffering.enabled&(buff.student_of_suffering.remains>1|prev_gcd.1.sigil_of_flame)" );
  fs->add_action( "variable,name=fel_dev_passive_fury_gen,op=add,value=30+(2*talent.flames_of_fury*spell_targets.sigil_of_flame),if=(cooldown.sigil_of_flame.remains<variable.fel_dev_sequence_time)" );
  fs->add_action( "variable,name=fel_dev_passive_fury_gen,op=add,value=8,if=cooldown.immolation_aura.remains<variable.fel_dev_sequence_time" );
  fs->add_action( "variable,name=fel_dev_passive_fury_gen,op=add,value=2*floor((buff.immolation_aura.remains>?variable.fel_dev_sequence_time)),if=buff.immolation_aura.remains>1" );
  fs->add_action( "variable,name=fel_dev_passive_fury_gen,op=add,value=7.5*variable.crit_pct*floor((buff.immolation_aura.remains>?variable.fel_dev_sequence_time)),if=talent.volatile_flameblood&buff.immolation_aura.remains>1" );
  fs->add_action( "variable,name=fel_dev_passive_fury_gen,op=add,value=22,if=talent.darkglare_boon.enabled" );
  fs->add_action( "variable,name=spbomb_threshold,op=setif,condition=talent.fiery_demise&dot.fiery_brand.ticking,value=(variable.single_target*5)+(variable.small_aoe*5)+(variable.big_aoe*4),value_else=(variable.single_target*5)+(variable.small_aoe*5)+(variable.big_aoe*4)" );
  fs->add_action( "variable,name=can_spbomb,op=setif,condition=talent.spirit_bomb,value=soul_fragments>=variable.spbomb_threshold,value_else=0" );
  fs->add_action( "variable,name=can_spbomb_soon,op=setif,condition=talent.spirit_bomb,value=soul_fragments.total>=variable.spbomb_threshold,value_else=0" );
  fs->add_action( "variable,name=can_spbomb_one_gcd,op=setif,condition=talent.spirit_bomb,value=(soul_fragments.total+variable.num_spawnable_souls)>=variable.spbomb_threshold,value_else=0" );
  fs->add_action( "variable,name=spburst_threshold,op=setif,condition=talent.fiery_demise&dot.fiery_brand.ticking,value=(variable.single_target*5)+(variable.small_aoe*5)+(variable.big_aoe*4),value_else=(variable.single_target*5)+(variable.small_aoe*5)+(variable.big_aoe*4)" );
  fs->add_action( "variable,name=can_spburst,op=setif,condition=talent.spirit_bomb,value=soul_fragments>=variable.spburst_threshold,value_else=0" );
  fs->add_action( "variable,name=can_spburst_soon,op=setif,condition=talent.spirit_bomb,value=soul_fragments.total>=variable.spburst_threshold,value_else=0" );
  fs->add_action( "variable,name=can_spburst_one_gcd,op=setif,condition=talent.spirit_bomb,value=(soul_fragments.total+variable.num_spawnable_souls)>=variable.spburst_threshold,value_else=0" );
  fs->add_action( "variable,name=meta_prep_time,op=set,value=0" );
  fs->add_action( "variable,name=meta_prep_time,op=add,value=action.fiery_brand.execute_time,if=talent.fiery_demise&cooldown.fiery_brand.up" );
  fs->add_action( "variable,name=meta_prep_time,op=add,value=action.sigil_of_flame.execute_time*action.sigil_of_flame.charges" );
  fs->add_action( "variable,name=dont_soul_cleave,op=setif,condition=buff.metamorphosis.up&buff.demonsurge_hardcast.up,value=buff.demonsurge_spirit_burst.up|(buff.metamorphosis.remains<(gcd.max*2)&(!((fury+variable.fel_dev_passive_fury_gen)>=120)|!(variable.can_spburst|variable.can_spburst_soon|soul_fragments.total>=4))),value_else=(cooldown.fel_devastation.remains<(gcd.max*3)&(!((fury+variable.fel_dev_passive_fury_gen)>=120)|!(variable.can_spburst|variable.can_spburst_soon|soul_fragments.total>=4)))" );
  fs->add_action( "variable,name=fiery_brand_back_before_meta,op=setif,condition=talent.down_in_flames,value=charges>=max_charges|(charges_fractional>=1&cooldown.fiery_brand.full_recharge_time<=gcd.remains+execute_time)|(charges_fractional>=1&((1-(charges_fractional-1))*cooldown.fiery_brand.duration)<=cooldown.metamorphosis.remains),value_else=(cooldown.fiery_brand.duration<=cooldown.metamorphosis.remains)" );
  fs->add_action( "variable,name=hold_sof_for_meta,op=setif,condition=talent.illuminated_sigils,value=(charges_fractional>=1&((1-(charges_fractional-1))*cooldown.sigil_of_flame.duration)>cooldown.metamorphosis.remains),value_else=cooldown.sigil_of_flame.duration>cooldown.metamorphosis.remains" );
  fs->add_action( "variable,name=hold_sof_for_fel_dev,op=setif,condition=talent.illuminated_sigils,value=(charges_fractional>=1&((1-(charges_fractional-1))*cooldown.sigil_of_flame.duration)>cooldown.fel_devastation.remains),value_else=cooldown.sigil_of_flame.duration>cooldown.fel_devastation.remains" );
  fs->add_action( "variable,name=hold_sof_for_student,op=setif,condition=talent.student_of_suffering,value=prev_gcd.1.sigil_of_flame|(buff.student_of_suffering.remains>(4-talent.quickened_sigils)),value_else=0" );
  fs->add_action( "variable,name=hold_sof_for_dot,op=setif,condition=talent.ascending_flame,value=0,value_else=prev_gcd.1.sigil_of_flame|(dot.sigil_of_flame.remains>(4-talent.quickened_sigils))" );
  fs->add_action( "variable,name=hold_sof_for_precombat,value=(talent.illuminated_sigils&time<(2-talent.quickened_sigils))" );
  fs->add_action( "use_item,slot=trinket1,if=!trinket.1.is.tome_of_lights_devotion&(!variable.trinket_1_buffs|(variable.trinket_1_buffs&((buff.metamorphosis.up&buff.demonsurge_hardcast.up)|(buff.metamorphosis.up&!buff.demonsurge_hardcast.up&cooldown.metamorphosis.remains<10)|(cooldown.metamorphosis.remains>trinket.1.cooldown.duration)|(variable.trinket_2_buffs&trinket.2.cooldown.remains<cooldown.metamorphosis.remains))))" );
  fs->add_action( "use_item,slot=trinket2,if=!trinket.2.is.tome_of_lights_devotion&(!variable.trinket_2_buffs|(variable.trinket_2_buffs&((buff.metamorphosis.up&buff.demonsurge_hardcast.up)|(buff.metamorphosis.up&!buff.demonsurge_hardcast.up&cooldown.metamorphosis.remains<10)|(cooldown.metamorphosis.remains>trinket.2.cooldown.duration)|(variable.trinket_1_buffs&trinket.1.cooldown.remains<cooldown.metamorphosis.remains))))" );
  fs->add_action( "use_item,name=tome_of_lights_devotion,if=buff.inner_resilience.up" );
  fs->add_action( "immolation_aura,if=time<4" );
  fs->add_action( "immolation_aura,if=!(cooldown.metamorphosis.up&prev_gcd.1.sigil_of_flame)&!(talent.fallout&talent.spirit_bomb&spell_targets.spirit_bomb>=3&((buff.metamorphosis.up&(variable.can_spburst|variable.can_spburst_soon))|(!buff.metamorphosis.up&(variable.can_spbomb|variable.can_spbomb_soon))))&!(buff.metamorphosis.up&buff.demonsurge_hardcast.up)" );
  fs->add_action( "sigil_of_flame,if=!talent.student_of_suffering&!variable.hold_sof_for_dot&!variable.hold_sof_for_precombat" );
  fs->add_action( "sigil_of_flame,if=!variable.hold_sof_for_precombat&(charges=max_charges|(!variable.hold_sof_for_student&!variable.hold_sof_for_dot&!variable.hold_sof_for_meta&!variable.hold_sof_for_fel_dev))" );
  fs->add_action( "fiery_brand,if=active_dot.fiery_brand=0&(!talent.fiery_demise|((talent.down_in_flames&charges>=max_charges)|variable.fiery_brand_back_before_meta))" );
  fs->add_action( "call_action_list,name=fs_execute,if=fight_remains<20" );
  fs->add_action( "run_action_list,name=fel_dev,if=buff.metamorphosis.up&!buff.demonsurge_hardcast.up&(buff.demonsurge_soul_sunder.up|buff.demonsurge_spirit_burst.up)" );
  fs->add_action( "run_action_list,name=metamorphosis,if=buff.metamorphosis.up&buff.demonsurge_hardcast.up" );
  fs->add_action( "run_action_list,name=fel_dev_prep,if=!buff.demonsurge_hardcast.up&(cooldown.fel_devastation.up|(cooldown.fel_devastation.remains<=(gcd.max*3)))" );
  fs->add_action( "run_action_list,name=meta_prep,if=(cooldown.metamorphosis.remains<=variable.meta_prep_time)&!cooldown.fel_devastation.up&!cooldown.fel_devastation.remains<10&!buff.demonsurge_soul_sunder.up&!buff.demonsurge_spirit_burst.up" );
  fs->add_action( "the_hunt" );
  fs->add_action( "felblade,if=((cooldown.sigil_of_spite.remains<execute_time|cooldown.soul_carver.remains<execute_time)&cooldown.fel_devastation.remains<(execute_time+gcd.max)&fury<50)" );
  fs->add_action( "soul_carver,if=(!talent.fiery_demise|talent.fiery_demise&dot.fiery_brand.ticking)&((!talent.spirit_bomb|variable.single_target)|(talent.spirit_bomb&!prev_gcd.1.sigil_of_spite&((soul_fragments.total+3<=5&fury>=40)|(soul_fragments.total=0&fury>=15))))" );
  fs->add_action( "sigil_of_spite,if=(!talent.cycle_of_binding|(cooldown.sigil_of_spite.duration<(cooldown.metamorphosis.remains+18)))&(!talent.spirit_bomb|(fury>=80&(variable.can_spbomb|variable.can_spbomb_soon))|(soul_fragments.total<=(2-talent.soul_sigils.rank)))" );
  fs->add_action( "spirit_bomb,if=variable.can_spburst&talent.fiery_demise&dot.fiery_brand.ticking&!(cooldown.fel_devastation.remains<(gcd.max*3))" );
  fs->add_action( "spirit_bomb,if=variable.can_spbomb&talent.fiery_demise&dot.fiery_brand.ticking&!(cooldown.fel_devastation.remains<(gcd.max*3))" );
  fs->add_action( "soul_cleave,if=variable.single_target&!variable.dont_soul_cleave" );
  fs->add_action( "spirit_bomb,if=variable.can_spburst&!(cooldown.fel_devastation.remains<(gcd.max*3))" );
  fs->add_action( "spirit_bomb,if=variable.can_spbomb&!(cooldown.fel_devastation.remains<(gcd.max*3))" );
  fs->add_action( "felblade,if=((fury<40&((buff.metamorphosis.up&(variable.can_spburst|variable.can_spburst_soon))|(!buff.metamorphosis.up&(variable.can_spbomb|variable.can_spbomb_soon)))))" );
  fs->add_action( "shear,if=talent.fracture&((fury<40&((buff.metamorphosis.up&(variable.can_spburst|variable.can_spburst_soon))|(!buff.metamorphosis.up&(variable.can_spbomb|variable.can_spbomb_soon))))|(buff.metamorphosis.up&variable.can_spburst_one_gcd)|(!buff.metamorphosis.up&variable.can_spbomb_one_gcd))" );
  fs->add_action( "felblade,if=fury.deficit>=40" );
  fs->add_action( "soul_cleave,if=!variable.dont_soul_cleave" );
  fs->add_action( "shear,if=talent.fracture" );
  fs->add_action( "throw_glaive" );

  fs_execute->add_action( "metamorphosis,use_off_gcd=1" );
  fs_execute->add_action( "the_hunt" );
  fs_execute->add_action( "sigil_of_flame" );
  fs_execute->add_action( "fiery_brand" );
  fs_execute->add_action( "sigil_of_spite" );
  fs_execute->add_action( "soul_carver" );
  fs_execute->add_action( "fel_devastation" );

  meta_prep->add_action( "metamorphosis,use_off_gcd=1,if=cooldown.sigil_of_flame.charges<1" );
  meta_prep->add_action( "fiery_brand,if=talent.fiery_demise&((talent.down_in_flames&charges>=max_charges)|active_dot.fiery_brand=0)" );
  meta_prep->add_action( "potion,use_off_gcd=1" );
  meta_prep->add_action( "sigil_of_flame" );

  metamorphosis->add_action( "call_action_list,name=externals" );
  metamorphosis->add_action( "fel_devastation,if=buff.metamorphosis.remains<(gcd.max*3)" );
  metamorphosis->add_action( "felblade,if=fury<50&(buff.metamorphosis.remains<(gcd.max*3))&cooldown.fel_devastation.up" );
  metamorphosis->add_action( "shear,if=talent.fracture&fury<50&!cooldown.felblade.up&(buff.metamorphosis.remains<(gcd.max*3))&cooldown.fel_devastation.up" );
  metamorphosis->add_action( "sigil_of_flame,if=talent.illuminated_sigils&talent.cycle_of_binding&charges=max_charges" );
  metamorphosis->add_action( "immolation_aura" );
  metamorphosis->add_action( "sigil_of_flame,if=!talent.student_of_suffering&(talent.ascending_flame|(!talent.ascending_flame&!prev_gcd.1.sigil_of_flame&(dot.sigil_of_doom.remains<(4-talent.quickened_sigils))))" );
  metamorphosis->add_action( "sigil_of_flame,if=talent.student_of_suffering&!prev_gcd.1.sigil_of_flame&!prev_gcd.1.sigil_of_flame&(buff.student_of_suffering.remains<(4-talent.quickened_sigils))" );
  metamorphosis->add_action( "sigil_of_flame,if=buff.metamorphosis.remains<((2-talent.quickened_sigils)+(charges*gcd.max))" );
  metamorphosis->add_action( "fel_devastation,if=soul_fragments<=3&(soul_fragments.inactive>=2|prev_gcd.1.sigil_of_spite)" );
  metamorphosis->add_action( "felblade,if=((cooldown.sigil_of_spite.remains<execute_time|cooldown.soul_carver.remains<execute_time)&cooldown.fel_devastation.remains<(execute_time+gcd.max)&fury<50)" );
  metamorphosis->add_action( "soul_carver,if=(!talent.spirit_bomb|(variable.single_target&!buff.demonsurge_spirit_burst.up))|(((soul_fragments.total+3)<=6)&fury>=40&!prev_gcd.1.sigil_of_spite)" );
  metamorphosis->add_action( "sigil_of_spite,if=!talent.spirit_bomb|(fury>=80&(variable.can_spburst|variable.can_spburst_soon))|(soul_fragments.total<=(2-talent.soul_sigils.rank))" );
  metamorphosis->add_action( "spirit_bomb,if=variable.can_spburst&buff.demonsurge_spirit_burst.up" );
  metamorphosis->add_action( "fel_devastation" );
  metamorphosis->add_action( "the_hunt" );
  metamorphosis->add_action( "soul_cleave,if=buff.demonsurge_soul_sunder.up&!buff.demonsurge_spirit_burst.up&!variable.can_spburst_one_gcd" );
  metamorphosis->add_action( "spirit_bomb,if=variable.can_spburst&(talent.fiery_demise&dot.fiery_brand.ticking|variable.big_aoe)&buff.metamorphosis.remains>(gcd.max*2)" );
  metamorphosis->add_action( "felblade,if=fury<40&(variable.can_spburst|variable.can_spburst_soon)&(buff.demonsurge_spirit_burst.up|talent.fiery_demise&dot.fiery_brand.ticking|variable.big_aoe)" );
  metamorphosis->add_action( "shear,if=talent.fracture&fury<40&(variable.can_spburst|variable.can_spburst_soon|variable.can_spburst_one_gcd)&(buff.demonsurge_spirit_burst.up|talent.fiery_demise&dot.fiery_brand.ticking|variable.big_aoe)" );
  metamorphosis->add_action( "shear,if=talent.fracture&variable.can_spburst_one_gcd&(buff.demonsurge_spirit_burst.up|variable.big_aoe)&!prev_gcd.1.shear" );
  metamorphosis->add_action( "soul_cleave,if=variable.single_target&!variable.dont_soul_cleave" );
  metamorphosis->add_action( "spirit_bomb,if=variable.can_spburst&buff.metamorphosis.remains>(gcd.max*2)" );
  metamorphosis->add_action( "felblade,if=fury.deficit>=40" );
  metamorphosis->add_action( "soul_cleave,if=!variable.dont_soul_cleave&!(variable.big_aoe&(variable.can_spburst|variable.can_spburst_soon))" );
  metamorphosis->add_action( "felblade" );
  metamorphosis->add_action( "shear,if=talent.fracture&!prev_gcd.1.shear" );
}
//vengeance_apl_end

//vengeance_ptr_apl_start
//vengeance_ptr_apl_end

}  // namespace demon_hunter_apl
