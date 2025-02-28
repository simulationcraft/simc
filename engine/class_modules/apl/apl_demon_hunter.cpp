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
  return ( p->true_level >= 71 ) ? "feast_of_the_divine_day" : "fated_fortune_cookie";
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
  return ( p->true_level >= 71 ) ? "main_hand:ironclaw_whetstone_3/off_hand:ironclaw_whetstone_3"
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
  action_priority_list_t* fs_meta = p->get_action_priority_list( "fs_meta" );
  action_priority_list_t* fs_opener = p->get_action_priority_list( "fs_opener" );

  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=trinket1_steroids,value=trinket.1.has_stat.any_dps" );
  precombat->add_action( "variable,name=trinket2_steroids,value=trinket.2.has_stat.any_dps" );
  precombat->add_action( "variable,name=rg_ds,default=0,op=reset" );
  precombat->add_action( "sigil_of_flame" );
  precombat->add_action( "immolation_aura" );

  default_->add_action( "auto_attack,if=!buff.out_of_range.up", "Default actions regardless of hero tree" );
  default_->add_action( "disrupt" );
  default_->add_action( "retarget_auto_attack,line_cd=1,target_if=min:debuff.burning_wound.remains,if=talent.burning_wound&talent.demon_blades&active_dot.burning_wound<(spell_targets>?3)", "Spread Burning Wounds for uptime in multitarget scenarios" );
  default_->add_action( "retarget_auto_attack,line_cd=1,target_if=min:!target.is_boss,if=talent.burning_wound&talent.demon_blades&active_dot.burning_wound=(spell_targets>?3)" );
  default_->add_action( "variable,name=fury_gen,op=set,value=talent.demon_blades*(1%(2.6*attack_haste)*((talent.demonsurge&buff.metamorphosis.up)*3+12))+buff.immolation_aura.stack*6+buff.tactical_retreat.up*10" );
  default_->add_action( "run_action_list,name=ar,if=hero_tree.aldrachi_reaver", "Separate actionlists for each hero tree" );
  default_->add_action( "run_action_list,name=fs,if=hero_tree.felscarred" );

  ar->add_action( "variable,name=rg_inc,op=set,value=buff.rending_strike.down&buff.glaive_flurry.up&cooldown.blade_dance.up&gcd.remains=0|variable.rg_inc&prev_gcd.1.death_sweep", "Aldrachi Reaver" );
  ar->add_action( "pick_up_fragment,use_off_gcd=1,if=fury<=90" );
  ar->add_action( "variable,name=fel_barrage,op=set,value=talent.fel_barrage&(cooldown.fel_barrage.remains<gcd.max*7&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in<gcd.max*7|raid_event.adds.in>90)&(cooldown.metamorphosis.remains|active_enemies>2)|buff.fel_barrage.up)&!(active_enemies=1&!raid_event.adds.exists)" );
  ar->add_action( "chaos_strike,if=buff.rending_strike.up&buff.glaive_flurry.up&(variable.rg_ds=2|active_enemies>2)&time>10" );
  ar->add_action( "annihilation,if=buff.rending_strike.up&buff.glaive_flurry.up&(variable.rg_ds=2|active_enemies>2)" );
  ar->add_action( "reavers_glaive,if=buff.glaive_flurry.down&buff.rending_strike.down&buff.thrill_of_the_fight_damage.remains<gcd.max*4+(variable.rg_ds=2)+(cooldown.the_hunt.remains<gcd.max*3)*3+(cooldown.eye_beam.remains<gcd.max*3&talent.shattered_destiny)*3&(variable.rg_ds=0|variable.rg_ds=1&cooldown.blade_dance.up|variable.rg_ds=2&cooldown.blade_dance.remains)&(buff.thrill_of_the_fight_damage.up|!prev_gcd.1.death_sweep|!variable.rg_inc)&active_enemies<3&!action.reavers_glaive.last_used<5&debuff.essence_break.down&(buff.metamorphosis.remains>2|cooldown.eye_beam.remains<10|fight_remains<10)" );
  ar->add_action( "reavers_glaive,if=buff.glaive_flurry.down&buff.rending_strike.down&buff.thrill_of_the_fight_damage.remains<4&(buff.thrill_of_the_fight_damage.up|!prev_gcd.1.death_sweep|!variable.rg_inc)&active_enemies>2|fight_remains<10" );
  ar->add_action( "call_action_list,name=ar_cooldown" );
  ar->add_action( "run_action_list,name=ar_opener,if=(cooldown.eye_beam.up|cooldown.metamorphosis.up|cooldown.essence_break.up)&time<15&(raid_event.adds.in>40)" );
  ar->add_action( "sigil_of_spite,if=debuff.essence_break.down&debuff.reavers_mark.remains>=2-talent.quickened_sigils" );
  ar->add_action( "run_action_list,name=ar_fel_barrage,if=variable.fel_barrage&raid_event.adds.up" );
  ar->add_action( "immolation_aura,if=active_enemies>2&talent.ragefire&(!talent.fel_barrage|cooldown.fel_barrage.remains>recharge_time)&debuff.essence_break.down&(buff.metamorphosis.down|buff.metamorphosis.remains>5)" );
  ar->add_action( "immolation_aura,if=active_enemies>2&talent.ragefire&raid_event.adds.up&raid_event.adds.remains<15&raid_event.adds.remains>5&debuff.essence_break.down" );
  ar->add_action( "vengeful_retreat,use_off_gcd=1,if=talent.initiative&(cooldown.eye_beam.remains>15&gcd.remains<0.3|gcd.remains<0.2&cooldown.eye_beam.remains<=gcd.remains&cooldown.metamorphosis.remains>10)&(!talent.student_of_suffering|cooldown.sigil_of_flame.remains)&time>10&(!variable.trinket1_steroids&!variable.trinket2_steroids|variable.trinket1_steroids&(trinket.1.cooldown.remains<gcd.max*3|trinket.1.cooldown.remains>20)|variable.trinket2_steroids&(trinket.2.cooldown.remains<gcd.max*3|trinket.2.cooldown.remains>20|talent.shattered_destiny))&&time>20&(!talent.inertia&buff.unbound_chaos.down|buff.inertia_trigger.down&buff.metamorphosis.down)" );
  ar->add_action( "run_action_list,name=ar_fel_barrage,if=variable.fel_barrage|!talent.demon_blades&talent.fel_barrage&(buff.fel_barrage.up|cooldown.fel_barrage.up)&buff.metamorphosis.down" );
  ar->add_action( "felblade,if=!talent.inertia&active_enemies=1&buff.unbound_chaos.up&buff.initiative.up&debuff.essence_break.down&buff.metamorphosis.down" );
  ar->add_action( "felblade,if=buff.inertia_trigger.up&talent.inertia&cooldown.eye_beam.remains<=0.5&(cooldown.metamorphosis.remains&talent.looks_can_kill|active_enemies>1)" );
  ar->add_action( "run_action_list,name=ar_meta,if=buff.metamorphosis.up" );
  ar->add_action( "felblade,if=buff.inertia_trigger.up&talent.inertia&buff.inertia.down&cooldown.blade_dance.remains<4&(cooldown.eye_beam.remains>5&cooldown.eye_beam.remains>buff.unbound_chaos.remains|cooldown.eye_beam.remains<=gcd.max&cooldown.vengeful_retreat.remains<=gcd.max+1)" );
  ar->add_action( "immolation_aura,if=talent.a_fire_inside&talent.burning_wound&full_recharge_time<gcd.max*2&(raid_event.adds.in>full_recharge_time|active_enemies>desired_targets)" );
  ar->add_action( "immolation_aura,if=active_enemies>desired_targets&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>full_recharge_time)" );
  ar->add_action( "immolation_aura,if=fight_remains<15&cooldown.blade_dance.remains&talent.ragefire" );
  ar->add_action( "sigil_of_flame,if=talent.student_of_suffering&cooldown.eye_beam.remains<gcd.max&(cooldown.essence_break.remains<gcd.max*4|!talent.essence_break)&(cooldown.metamorphosis.remains>10|cooldown.blade_dance.remains<gcd.max*3)&(!variable.trinket1_steroids&!variable.trinket2_steroids|variable.trinket1_steroids&(trinket.1.cooldown.remains<gcd.max*3|trinket.1.cooldown.remains>20)|variable.trinket2_steroids&(trinket.2.cooldown.remains<gcd.max*3|trinket.2.cooldown.remains>20))" );
  ar->add_action( "eye_beam,if=(cooldown.blade_dance.remains<7|raid_event.adds.up)&(!variable.trinket1_steroids&!variable.trinket2_steroids|variable.trinket1_steroids&(trinket.1.cooldown.remains<gcd.max*3|trinket.1.cooldown.remains>20)|variable.trinket2_steroids&(trinket.2.cooldown.remains<gcd.max*3|trinket.2.cooldown.remains>20))|fight_remains<10" );
  ar->add_action( "blade_dance,if=cooldown.eye_beam.remains>=gcd.max*3&buff.rending_strike.down" );
  ar->add_action( "chaos_strike,if=buff.rending_strike.up" );
  ar->add_action( "felblade,if=buff.metamorphosis.down&fury.deficit>40&action.felblade.cooldown_react&!buff.inertia_trigger.down" );
  ar->add_action( "glaive_tempest,if=active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>10" );
  ar->add_action( "sigil_of_flame,if=active_enemies>3&!talent.student_of_suffering|buff.out_of_range.down&talent.art_of_the_glaive" );
  ar->add_action( "chaos_strike,if=debuff.essence_break.up" );
  ar->add_action( "felblade,if=(buff.out_of_range.down|fury.deficit>40)&action.felblade.cooldown_react&!buff.inertia_trigger.up" );
  ar->add_action( "throw_glaive,if=active_enemies>1&talent.furious_throws" );
  ar->add_action( "sigil_of_flame,if=buff.out_of_range.down&debuff.essence_break.down&!talent.student_of_suffering&fury.deficit>32" );
  ar->add_action( "chaos_strike,if=cooldown.eye_beam.remains>gcd.max*2|fury>80" );
  ar->add_action( "immolation_aura,if=raid_event.adds.in>full_recharge_time|active_enemies>desired_targets&active_enemies>2" );
  ar->add_action( "sigil_of_flame,if=buff.out_of_range.down&debuff.essence_break.down&!talent.student_of_suffering&(!talent.fel_barrage|cooldown.fel_barrage.remains>25|(active_enemies=1&!raid_event.adds.exists))" );
  ar->add_action( "demons_bite" );
  ar->add_action( "throw_glaive,if=buff.unbound_chaos.down&recharge_time<cooldown.eye_beam.remains&debuff.essence_break.down&(cooldown.eye_beam.remains>8|charges_fractional>1.01)&buff.out_of_range.down&active_enemies>1" );
  ar->add_action( "fel_rush,if=buff.unbound_chaos.down&recharge_time<cooldown.eye_beam.remains&debuff.essence_break.down&(cooldown.eye_beam.remains>8|charges_fractional>1.01)&active_enemies>1" );
  ar->add_action( "arcane_torrent,if=buff.out_of_range.down&debuff.essence_break.down&fury<100" );

  ar_cooldown->add_action( "metamorphosis,if=(((cooldown.eye_beam.remains>=20|talent.cycle_of_hatred&cooldown.eye_beam.remains>=13)&(!talent.essence_break|debuff.essence_break.up)&buff.fel_barrage.down&(raid_event.adds.in>40|(raid_event.adds.remains>8|!talent.fel_barrage)&active_enemies>2)|!talent.chaotic_transformation|fight_remains<30)&buff.inner_demon.down&(!talent.restless_hunter&cooldown.blade_dance.remains>gcd.max*3|prev_gcd.1.death_sweep))&!talent.inertia&!talent.essence_break&time>15" );
  ar_cooldown->add_action( "metamorphosis,if=(cooldown.blade_dance.remains&((prev_gcd.1.death_sweep|prev_gcd.2.death_sweep|prev_gcd.3.death_sweep|buff.metamorphosis.up&buff.metamorphosis.remains<gcd.max)&cooldown.eye_beam.remains&(!talent.essence_break|debuff.essence_break.up|cooldown.essence_break.remains>15)&buff.fel_barrage.down&(raid_event.adds.in>40|(raid_event.adds.remains>8|!talent.fel_barrage)&active_enemies>2)|!talent.chaotic_transformation|fight_remains<30)&(buff.inner_demon.down&(buff.rending_strike.down|!talent.restless_hunter|prev_gcd.1.death_sweep)))&(talent.inertia|talent.essence_break)&time>15" );
  ar_cooldown->add_action( "potion,if=fight_remains<35|buff.metamorphosis.up|debuff.essence_break.up" );
  ar_cooldown->add_action( "invoke_external_buff,name=power_infusion,if=buff.metamorphosis.up|fight_remains<=20" );
  ar_cooldown->add_action( "variable,name=special_trinket,op=set,value=equipped.mad_queens_mandate|equipped.treacherous_transmitter|equipped.skardyns_grace" );
  ar_cooldown->add_action( "use_item,name=mad_queens_mandate,if=((!talent.initiative|buff.initiative.up|time>5)&(buff.metamorphosis.remains>5|buff.metamorphosis.down)&(trinket.1.is.mad_queens_mandate&(trinket.2.cooldown.duration<10|trinket.2.cooldown.remains>10|!trinket.2.has_buff.any)|trinket.2.is.mad_queens_mandate&(trinket.1.cooldown.duration<10|trinket.1.cooldown.remains>10|!trinket.1.has_buff.any))&fight_remains>120|fight_remains<10&fight_remains<buff.metamorphosis.remains)&debuff.essence_break.down|fight_remains<5" );
  ar_cooldown->add_action( "use_item,name=treacherous_transmitter,if=!equipped.mad_queens_mandate|equipped.mad_queens_mandate&(trinket.1.is.mad_queens_mandate&trinket.1.cooldown.remains>fight_remains|trinket.2.is.mad_queens_mandate&trinket.2.cooldown.remains>fight_remains)|fight_remains>25" );
  ar_cooldown->add_action( "use_item,name=skardyns_grace,if=(!equipped.mad_queens_mandate|fight_remains>25|trinket.2.is.skardyns_grace&trinket.1.cooldown.remains>fight_remains|trinket.1.is.skardyns_grace&trinket.2.cooldown.remains>fight_remains|trinket.1.cooldown.duration<10|trinket.2.cooldown.duration<10)&buff.metamorphosis.up" );
  ar_cooldown->add_action( "do_treacherous_transmitter_task,if=cooldown.eye_beam.remains>15|cooldown.eye_beam.remains<5|fight_remains<20" );
  ar_cooldown->add_action( "use_item,slot=trinket1,if=((cooldown.eye_beam.remains<gcd.max&active_enemies>1|buff.metamorphosis.up)&(raid_event.adds.in>trinket.1.cooldown.duration-15|raid_event.adds.remains>8)|!trinket.1.has_buff.any|fight_remains<25)&!trinket.1.is.skardyns_grace&!trinket.1.is.mad_queens_mandate&!trinket.1.is.treacherous_transmitter&(!variable.special_trinket|trinket.2.cooldown.remains>20)" );
  ar_cooldown->add_action( "use_item,slot=trinket2,if=((cooldown.eye_beam.remains<gcd.max&active_enemies>1|buff.metamorphosis.up)&(raid_event.adds.in>trinket.2.cooldown.duration-15|raid_event.adds.remains>8)|!trinket.2.has_buff.any|fight_remains<25)&!trinket.2.is.skardyns_grace&!trinket.2.is.mad_queens_mandate&!trinket.2.is.treacherous_transmitter&(!variable.special_trinket|trinket.1.cooldown.remains>20)" );
  ar_cooldown->add_action( "the_hunt,if=debuff.essence_break.down&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>90)&(debuff.reavers_mark.up|!hero_tree.aldrachi_reaver)&buff.reavers_glaive.down&(buff.metamorphosis.remains>5|buff.metamorphosis.down)&(!talent.initiative|buff.initiative.up|time>5)&time>5&(!talent.inertia&buff.unbound_chaos.down|buff.inertia_trigger.down)" );
  ar_cooldown->add_action( "sigil_of_spite,if=debuff.essence_break.down&(debuff.reavers_mark.remains>=2-talent.quickened_sigils|!hero_tree.aldrachi_reaver)&cooldown.blade_dance.remains&time>15" );

  ar_fel_barrage->add_action( "variable,name=generator_up,op=set,value=cooldown.felblade.remains<gcd.max|cooldown.sigil_of_flame.remains<gcd.max" );
  ar_fel_barrage->add_action( "variable,name=fury_gen,op=set,value=talent.demon_blades*(1%(2.6*attack_haste)*12*((hero_tree.felscarred&buff.metamorphosis.up)*0.33+1))+buff.immolation_aura.stack*6+buff.tactical_retreat.up*10" );
  ar_fel_barrage->add_action( "variable,name=gcd_drain,op=set,value=gcd.max*32" );
  ar_fel_barrage->add_action( "annihilation,if=buff.inner_demon.up" );
  ar_fel_barrage->add_action( "eye_beam,if=buff.fel_barrage.down&(active_enemies>1&raid_event.adds.up|raid_event.adds.in>40)" );
  ar_fel_barrage->add_action( "abyssal_gaze,if=buff.fel_barrage.down&(active_enemies>1&raid_event.adds.up|raid_event.adds.in>40)" );
  ar_fel_barrage->add_action( "essence_break,if=buff.fel_barrage.down&buff.metamorphosis.up" );
  ar_fel_barrage->add_action( "death_sweep,if=buff.fel_barrage.down" );
  ar_fel_barrage->add_action( "immolation_aura,if=(active_enemies>2|buff.fel_barrage.up)&(cooldown.eye_beam.remains>recharge_time+3)" );
  ar_fel_barrage->add_action( "glaive_tempest,if=buff.fel_barrage.down&active_enemies>1" );
  ar_fel_barrage->add_action( "blade_dance,if=buff.fel_barrage.down" );
  ar_fel_barrage->add_action( "fel_barrage,if=fury>100&(raid_event.adds.in>90|raid_event.adds.in<gcd.max|raid_event.adds.remains>4&active_enemies>2)" );
  ar_fel_barrage->add_action( "felblade,if=buff.inertia_trigger.up&buff.fel_barrage.up" );
  ar_fel_barrage->add_action( "sigil_of_flame,if=fury.deficit>40&buff.fel_barrage.up&(!talent.student_of_suffering|cooldown.eye_beam.remains>30)" );
  ar_fel_barrage->add_action( "sigil_of_doom,if=fury.deficit>40&buff.fel_barrage.up" );
  ar_fel_barrage->add_action( "felblade,if=buff.fel_barrage.up&fury.deficit>40&action.felblade.cooldown_react" );
  ar_fel_barrage->add_action( "death_sweep,if=fury-variable.gcd_drain-35>0&(buff.fel_barrage.remains<3|variable.generator_up|fury>80|variable.fury_gen>18)" );
  ar_fel_barrage->add_action( "glaive_tempest,if=fury-variable.gcd_drain-30>0&(buff.fel_barrage.remains<3|variable.generator_up|fury>80|variable.fury_gen>18)" );
  ar_fel_barrage->add_action( "blade_dance,if=fury-variable.gcd_drain-35>0&(buff.fel_barrage.remains<3|variable.generator_up|fury>80|variable.fury_gen>18)" );
  ar_fel_barrage->add_action( "arcane_torrent,if=fury.deficit>40&buff.fel_barrage.up" );
  ar_fel_barrage->add_action( "the_hunt,if=fury>40&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>80)" );
  ar_fel_barrage->add_action( "annihilation,if=fury-variable.gcd_drain-40>20&(buff.fel_barrage.remains<3|variable.generator_up|fury>80|variable.fury_gen>18)" );
  ar_fel_barrage->add_action( "chaos_strike,if=fury-variable.gcd_drain-40>20&(cooldown.fel_barrage.remains&cooldown.fel_barrage.remains<10&fury>100|buff.fel_barrage.up&(buff.fel_barrage.remains*variable.fury_gen-buff.fel_barrage.remains*32)>0)" );
  ar_fel_barrage->add_action( "demons_bite" );

  ar_meta->add_action( "death_sweep,if=buff.metamorphosis.remains<gcd.max" );
  ar_meta->add_action( "vengeful_retreat,use_off_gcd=1,if=talent.initiative&(cooldown.metamorphosis.remains&(cooldown.essence_break.remains<=0.6|cooldown.essence_break.remains>10|!talent.essence_break)|cooldown.metamorphosis.up&hero_tree.aldrachi_reaver&cooldown.essence_break.remains<=0.6&buff.inner_demon.down&cooldown.blade_dance.remains<=gcd.max+0.5|talent.restless_hunter&(!hero_tree.felscarred|buff.demonsurge_annihilation.down))&cooldown.eye_beam.remains&(!talent.inertia&buff.unbound_chaos.down|buff.inertia_trigger.down)" );
  ar_meta->add_action( "felblade,if=talent.inertia&buff.inertia_trigger.up&cooldown.essence_break.remains<=1&hero_tree.aldrachi_reaver&cooldown.blade_dance.remains<=gcd.max*2&cooldown.metamorphosis.remains<=gcd.max*3" );
  ar_meta->add_action( "essence_break,if=fury>=30&talent.restless_hunter&cooldown.metamorphosis.up&(talent.inertia&buff.inertia.up|!talent.inertia)&cooldown.blade_dance.remains<=gcd.max" );
  ar_meta->add_action( "annihilation,if=buff.metamorphosis.remains<gcd.max|debuff.essence_break.remains&debuff.essence_break.remains<0.5&cooldown.blade_dance.remains|talent.restless_hunter&(buff.demonsurge_annihilation.up|hero_tree.aldrachi_reaver&buff.inner_demon.up)&cooldown.essence_break.up&cooldown.metamorphosis.up" );
  ar_meta->add_action( "felblade,if=buff.inertia_trigger.up&talent.inertia&cooldown.metamorphosis.remains&(!hero_tree.felscarred|cooldown.eye_beam.remains)&(cooldown.eye_beam.remains<=0.5|cooldown.essence_break.remains<=0.5|cooldown.blade_dance.remains<=5.5|buff.initiative.remains<gcd.remains)" );
  ar_meta->add_action( "fel_rush,if=buff.inertia_trigger.up&talent.inertia&cooldown.metamorphosis.remains&active_enemies>2" );
  ar_meta->add_action( "fel_rush,if=buff.inertia_trigger.up&talent.inertia&cooldown.blade_dance.remains<gcd.max*3&(!hero_tree.felscarred|cooldown.eye_beam.remains)&cooldown.metamorphosis.remains&(active_enemies>2|hero_tree.felscarred)" );
  ar_meta->add_action( "immolation_aura,if=charges=2&active_enemies>1&debuff.essence_break.down" );
  ar_meta->add_action( "annihilation,if=buff.inner_demon.up&(cooldown.eye_beam.remains<gcd.max*3&cooldown.blade_dance.remains|cooldown.metamorphosis.remains<gcd.max*3)" );
  ar_meta->add_action( "essence_break,if=time<20&buff.thrill_of_the_fight_damage.remains>gcd.max*4&buff.metamorphosis.remains>=gcd.max*2&cooldown.metamorphosis.up&cooldown.death_sweep.remains<=gcd.max&buff.inertia.up" );
  ar_meta->add_action( "essence_break,if=fury>20&(cooldown.blade_dance.remains<gcd.max*3|cooldown.blade_dance.up)&(buff.unbound_chaos.down&!talent.inertia|buff.inertia.up)&buff.out_of_range.remains<gcd.max&(!talent.shattered_destiny|cooldown.eye_beam.remains>4)&(!hero_tree.felscarred|active_enemies>1|cooldown.metamorphosis.remains>5&cooldown.eye_beam.remains)|fight_remains<10" );
  ar_meta->add_action( "death_sweep" );
  ar_meta->add_action( "eye_beam,if=debuff.essence_break.down&buff.inner_demon.down" );
  ar_meta->add_action( "glaive_tempest,if=debuff.essence_break.down&(cooldown.blade_dance.remains>gcd.max*2|fury>60)&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>10)" );
  ar_meta->add_action( "sigil_of_flame,if=active_enemies>2&debuff.essence_break.down" );
  ar_meta->add_action( "throw_glaive,if=talent.soulscar&talent.furious_throws&active_enemies>1&debuff.essence_break.down&(charges=2|full_recharge_time<cooldown.blade_dance.remains)" );
  ar_meta->add_action( "annihilation,if=cooldown.blade_dance.remains|fury>60|soul_fragments.total>0|buff.metamorphosis.remains<5&cooldown.felblade.up" );
  ar_meta->add_action( "sigil_of_flame,if=buff.metamorphosis.remains>5&buff.out_of_range.down" );
  ar_meta->add_action( "felblade,if=(buff.out_of_range.down|fury.deficit>40)&action.felblade.cooldown_react" );
  ar_meta->add_action( "sigil_of_flame,if=debuff.essence_break.down&buff.out_of_range.down" );
  ar_meta->add_action( "immolation_aura,if=buff.out_of_range.down&recharge_time<(cooldown.eye_beam.remains<?buff.metamorphosis.remains)&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>full_recharge_time)" );
  ar_meta->add_action( "annihilation" );
  ar_meta->add_action( "fel_rush,if=recharge_time<cooldown.eye_beam.remains&debuff.essence_break.down&(cooldown.eye_beam.remains>8|charges_fractional>1.01)&buff.out_of_range.down&active_enemies>1" );
  ar_meta->add_action( "demons_bite" );

  ar_opener->add_action( "potion" );
  ar_opener->add_action( "the_hunt" );
  ar_opener->add_action( "vengeful_retreat,use_off_gcd=1,if=talent.initiative&time>4&buff.metamorphosis.up&(!talent.inertia|buff.inertia_trigger.down)&buff.inner_demon.down&gcd.remains<0.1&cooldown.blade_dance.remains" );
  ar_opener->add_action( "felblade,if=!talent.inertia&active_enemies=1&buff.unbound_chaos.up&buff.initiative.up&debuff.essence_break.down" );
  ar_opener->add_action( "essence_break,if=(buff.inertia.up|!talent.inertia)&buff.metamorphosis.up&cooldown.blade_dance.remains<=gcd.max&buff.inner_demon.down" );
  ar_opener->add_action( "annihilation,if=buff.inner_demon.up,line_cd=10" );
  ar_opener->add_action( "felblade,if=buff.inertia_trigger.up&talent.inertia&talent.restless_hunter&cooldown.essence_break.up&cooldown.metamorphosis.up&(buff.demonsurge_annihilation.down|buff.inner_demon.down)&buff.metamorphosis.up&cooldown.blade_dance.remains<=gcd.max" );
  ar_opener->add_action( "felblade,if=talent.inertia&buff.inertia_trigger.up&(buff.inertia.down&buff.metamorphosis.up)&debuff.essence_break.down" );
  ar_opener->add_action( "felblade,if=talent.inertia&buff.inertia_trigger.up&buff.metamorphosis.up&cooldown.metamorphosis.remains&debuff.essence_break.down" );
  ar_opener->add_action( "the_hunt,if=(buff.metamorphosis.up&hero_tree.aldrachi_reaver&talent.shattered_destiny|!talent.shattered_destiny&hero_tree.aldrachi_reaver|hero_tree.felscarred)&(!talent.initiative|talent.inertia|buff.initiative.up|time>5)" );
  ar_opener->add_action( "felblade,if=fury<40&action.felblade.cooldown_react&buff.inertia_trigger.down" );
  ar_opener->add_action( "reavers_glaive,if=debuff.reavers_mark.down&debuff.essence_break.down" );
  ar_opener->add_action( "chaos_strike,if=buff.rending_strike.up&active_enemies>2" );
  ar_opener->add_action( "blade_dance,if=buff.glaive_flurry.up&active_enemies>2" );
  ar_opener->add_action( "immolation_aura,if=talent.a_fire_inside&talent.burning_wound&buff.metamorphosis.down" );
  ar_opener->add_action( "metamorphosis,if=buff.metamorphosis.up&cooldown.blade_dance.remains>gcd.max*2&buff.inner_demon.down&(!talent.restless_hunter|prev_gcd.1.death_sweep)&(cooldown.essence_break.remains|!talent.essence_break)" );
  ar_opener->add_action( "sigil_of_spite,if=hero_tree.felscarred|debuff.reavers_mark.up&(!talent.cycle_of_hatred|cooldown.eye_beam.remains&cooldown.metamorphosis.remains)" );
  ar_opener->add_action( "eye_beam,if=buff.metamorphosis.down|debuff.essence_break.down&buff.inner_demon.down&(cooldown.blade_dance.remains|talent.essence_break&cooldown.essence_break.up)" );
  ar_opener->add_action( "essence_break,if=cooldown.blade_dance.remains<gcd.max&!hero_tree.felscarred&!talent.shattered_destiny&buff.metamorphosis.up|(cooldown.eye_beam.remains|buff.demonsurge_abyssal_gaze.down)&cooldown.metamorphosis.remains" );
  ar_opener->add_action( "death_sweep" );
  ar_opener->add_action( "annihilation" );
  ar_opener->add_action( "demons_bite" );

  fs->add_action( "pick_up_fragment,use_off_gcd=1,if=fury.deficit>=30", "Fel-Scarred" );
  fs->add_action( "call_action_list,name=fs_cooldown" );
  fs->add_action( "run_action_list,name=fs_opener,if=(cooldown.eye_beam.up|cooldown.metamorphosis.up|cooldown.essence_break.up|buff.demonsurge.stack<3+talent.student_of_suffering+talent.a_fire_inside)&time<15&raid_event.adds.in>40" );
  fs->add_action( "felblade,if=talent.unbound_chaos&buff.unbound_chaos.up&!talent.inertia&active_enemies<=2&(talent.student_of_suffering&cooldown.eye_beam.remains-gcd.max*2<=buff.unbound_chaos.remains|hero_tree.aldrachi_reaver)" );
  fs->add_action( "fel_rush,if=talent.unbound_chaos&buff.unbound_chaos.up&!talent.inertia&active_enemies>3&(talent.student_of_suffering&cooldown.eye_beam.remains-gcd.max*2<=buff.unbound_chaos.remains)" );
  fs->add_action( "run_action_list,name=fs_meta,if=buff.metamorphosis.up" );
  fs->add_action( "vengeful_retreat,use_off_gcd=1,if=talent.initiative&(cooldown.eye_beam.remains>15&gcd.remains<0.3|gcd.remains<0.2&cooldown.eye_beam.remains<=gcd.remains&(cooldown.metamorphosis.remains>10|cooldown.blade_dance.remains<gcd.max*3))&(!talent.student_of_suffering|cooldown.sigil_of_flame.remains)&(cooldown.essence_break.remains<=gcd.max*2&talent.student_of_suffering&cooldown.sigil_of_flame.remains|cooldown.essence_break.remains>=18|!talent.student_of_suffering)&(cooldown.metamorphosis.remains>10|hero_tree.aldrachi_reaver)&time>20" );
  fs->add_action( "sigil_of_flame,if=talent.student_of_suffering&cooldown.eye_beam.remains<=gcd.max&(cooldown.essence_break.remains<gcd.max*3|!talent.essence_break)&(cooldown.metamorphosis.remains>10|cooldown.blade_dance.remains<gcd.max*2)" );
  fs->add_action( "eye_beam,if=(!talent.initiative|buff.initiative.up|cooldown.vengeful_retreat.remains>=10|cooldown.metamorphosis.up)&(cooldown.blade_dance.remains<7|raid_event.adds.up)|fight_remains<10" );
  fs->add_action( "blade_dance,if=cooldown.eye_beam.remains>=gcd.max*4|debuff.essence_break.up" );
  fs->add_action( "chaos_strike,if=debuff.essence_break.up" );
  fs->add_action( "immolation_aura,if=talent.a_fire_inside&talent.isolated_prey&talent.flamebound&active_enemies=1&cooldown.eye_beam.remains>=gcd.max" );
  fs->add_action( "felblade,if=fury.deficit>40+variable.fury_gen*(0.5%gcd.max)&(cooldown.vengeful_retreat.remains>=action.felblade.cooldown+0.5&talent.inertia&active_enemies=1|!talent.inertia|hero_tree.aldrachi_reaver|cooldown.essence_break.remains)&cooldown.metamorphosis.remains&cooldown.eye_beam.remains>=0.5+gcd.max*(talent.student_of_suffering&cooldown.sigil_of_flame.remains<=gcd.max)" );
  fs->add_action( "chaos_strike,if=cooldown.eye_beam.remains>=gcd.max*4|(fury>=70-30*(talent.student_of_suffering&(cooldown.sigil_of_flame.remains<=gcd.max|cooldown.sigil_of_flame.up))-buff.chaos_theory.up*20-variable.fury_gen)" );
  fs->add_action( "immolation_aura,if=raid_event.adds.in>full_recharge_time&cooldown.eye_beam.remains>=gcd.max*(1+talent.student_of_suffering&(cooldown.sigil_of_flame.remains<=gcd.max|cooldown.sigil_of_flame.up))|active_enemies>desired_targets&active_enemies>2" );
  fs->add_action( "felblade,if=buff.out_of_range.down&buff.inertia_trigger.down&cooldown.eye_beam.remains>=gcd.max*(1+talent.student_of_suffering&(cooldown.sigil_of_flame.remains<=gcd.max|cooldown.sigil_of_flame.up))" );
  fs->add_action( "sigil_of_flame,if=buff.out_of_range.down&debuff.essence_break.down&!talent.student_of_suffering&(!talent.fel_barrage|cooldown.fel_barrage.remains>25|(active_enemies=1&!raid_event.adds.exists))" );
  fs->add_action( "throw_glaive,if=recharge_time<cooldown.eye_beam.remains&debuff.essence_break.down&(cooldown.eye_beam.remains>8|charges_fractional>1.01)&buff.out_of_range.down&active_enemies>1" );
  fs->add_action( "fel_rush,if=buff.unbound_chaos.down&recharge_time<cooldown.eye_beam.remains&debuff.essence_break.down&(cooldown.eye_beam.remains>8|charges_fractional>1.01)&active_enemies>1" );
  fs->add_action( "arcane_torrent,if=buff.out_of_range.down&debuff.essence_break.down&fury<100" );

  fs_cooldown->add_action( "metamorphosis,if=((cooldown.eye_beam.remains>=20&(!talent.essence_break|debuff.essence_break.up)&buff.fel_barrage.down&(raid_event.adds.in>40|(raid_event.adds.remains>8|!talent.fel_barrage)&active_enemies>2)|!talent.chaotic_transformation|fight_remains<30)&buff.inner_demon.down&(!talent.restless_hunter&cooldown.blade_dance.remains>gcd.max*3|prev_gcd.1.death_sweep))&!talent.inertia&!talent.essence_break&(hero_tree.aldrachi_reaver|buff.demonsurge_death_sweep.down)&time>15" );
  fs_cooldown->add_action( "metamorphosis,if=(cooldown.blade_dance.remains&(buff.metamorphosis.up&cooldown.eye_beam.remains&(!talent.essence_break|debuff.essence_break.up|talent.shattered_destiny|hero_tree.felscarred)&buff.fel_barrage.down&(raid_event.adds.in>40|(raid_event.adds.remains>8|!talent.fel_barrage)&active_enemies>2)|!talent.chaotic_transformation|fight_remains<30)&(buff.inner_demon.down&(!talent.restless_hunter|prev_gcd.1.death_sweep)))&(talent.inertia|talent.essence_break)&(hero_tree.aldrachi_reaver|(buff.demonsurge_death_sweep.down&buff.metamorphosis.up|buff.metamorphosis.remains<gcd.max)&(buff.demonsurge_annihilation.down))&time>15" );
  fs_cooldown->add_action( "potion,if=fight_remains<35|buff.metamorphosis.up|debuff.essence_break.up" );
  fs_cooldown->add_action( "invoke_external_buff,name=power_infusion,if=buff.metamorphosis.up|fight_remains<=20" );
  fs_cooldown->add_action( "variable,name=special_trinket,op=set,value=equipped.mad_queens_mandate|equipped.treacherous_transmitter|equipped.skardyns_grace" );
  fs_cooldown->add_action( "use_item,name=mad_queens_mandate,if=((!talent.initiative|buff.initiative.up|time>5)&(buff.metamorphosis.remains>5|buff.metamorphosis.down)&(trinket.1.is.mad_queens_mandate&(trinket.2.cooldown.duration<10|trinket.2.cooldown.remains>10|!trinket.2.has_buff.any)|trinket.2.is.mad_queens_mandate&(trinket.1.cooldown.duration<10|trinket.1.cooldown.remains>10|!trinket.1.has_buff.any))&fight_remains>120|fight_remains<10&fight_remains<buff.metamorphosis.remains)&debuff.essence_break.down|fight_remains<5" );
  fs_cooldown->add_action( "use_item,name=treacherous_transmitter,if=!equipped.mad_queens_mandate|equipped.mad_queens_mandate&(trinket.1.is.mad_queens_mandate&trinket.1.cooldown.remains>fight_remains|trinket.2.is.mad_queens_mandate&trinket.2.cooldown.remains>fight_remains)|fight_remains>25" );
  fs_cooldown->add_action( "use_item,name=skardyns_grace,if=(!equipped.mad_queens_mandate|fight_remains>25|trinket.2.is.skardyns_grace&trinket.1.cooldown.remains>fight_remains|trinket.1.is.skardyns_grace&trinket.2.cooldown.remains>fight_remains|trinket.1.cooldown.duration<10|trinket.2.cooldown.duration<10)&buff.metamorphosis.up" );
  fs_cooldown->add_action( "do_treacherous_transmitter_task,if=cooldown.eye_beam.remains>15|cooldown.eye_beam.remains<5|fight_remains<20" );
  fs_cooldown->add_action( "use_item,slot=trinket1,if=((cooldown.eye_beam.remains<gcd.max&active_enemies>1|buff.metamorphosis.up)&(raid_event.adds.in>trinket.1.cooldown.duration-15|raid_event.adds.remains>8)|!trinket.1.has_buff.any|fight_remains<25)&!trinket.1.is.skardyns_grace&!trinket.1.is.mad_queens_mandate&!trinket.1.is.treacherous_transmitter&(!variable.special_trinket|trinket.2.cooldown.remains>20)" );
  fs_cooldown->add_action( "use_item,slot=trinket2,if=((cooldown.eye_beam.remains<gcd.max&active_enemies>1|buff.metamorphosis.up)&(raid_event.adds.in>trinket.2.cooldown.duration-15|raid_event.adds.remains>8)|!trinket.2.has_buff.any|fight_remains<25)&!trinket.2.is.skardyns_grace&!trinket.2.is.mad_queens_mandate&!trinket.2.is.treacherous_transmitter&(!variable.special_trinket|trinket.1.cooldown.remains>20)" );
  fs_cooldown->add_action( "the_hunt,if=debuff.essence_break.down&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>90)&(debuff.reavers_mark.up|!hero_tree.aldrachi_reaver)&buff.reavers_glaive.down&(buff.metamorphosis.remains>5|buff.metamorphosis.down)&(!talent.initiative|buff.initiative.up|time>5)&time>5&(!talent.inertia&buff.unbound_chaos.down|buff.inertia_trigger.down)&(hero_tree.aldrachi_reaver|buff.metamorphosis.down)|fight_remains<=30" );
  fs_cooldown->add_action( "sigil_of_spite,if=debuff.essence_break.down&cooldown.blade_dance.remains&time>15" );

  fs_meta->add_action( "death_sweep,if=buff.metamorphosis.remains<gcd.max|debuff.essence_break.up|prev_gcd.1.metamorphosis" );
  fs_meta->add_action( "sigil_of_doom,if=talent.student_of_suffering&buff.demonsurge_sigil_of_doom.down&debuff.essence_break.down&(talent.student_of_suffering&((talent.essence_break&cooldown.essence_break.remains>30-gcd.max|cooldown.essence_break.remains<=gcd.max+talent.inertia&(cooldown.vengeful_retreat.remains<=gcd|buff.initiative.up)+gcd.max*(cooldown.eye_beam.remains<=gcd.max))|(!talent.essence_break&(cooldown.eye_beam.remains>=10|cooldown.eye_beam.remains<=gcd.max))))" );
  fs_meta->add_action( "vengeful_retreat,use_off_gcd=1,if=talent.initiative&(gcd.remains<0.3|talent.inertia&cooldown.eye_beam.remains>gcd.remains&(buff.cycle_of_hatred.stack=2|buff.cycle_of_hatred.stack=3))&(cooldown.metamorphosis.remains&(buff.demonsurge_annihilation.down&buff.demonsurge_death_sweep.down)|talent.restless_hunter&(!hero_tree.felscarred|buff.demonsurge_annihilation.down))&(!talent.inertia&buff.unbound_chaos.down|buff.inertia_trigger.down)&(!talent.essence_break|cooldown.essence_break.remains>18|cooldown.essence_break.remains<=gcd.remains+talent.inertia*1.5&(!talent.student_of_suffering|(buff.student_of_suffering.up|cooldown.sigil_of_flame.remains>5)))&(cooldown.eye_beam.remains>5|cooldown.eye_beam.remains<=gcd.remains|cooldown.eye_beam.up)" );
  fs_meta->add_action( "death_sweep,if=hero_tree.felscarred&talent.essence_break&buff.demonsurge_death_sweep.up&(buff.inertia.up&(cooldown.essence_break.remains>buff.inertia.remains|!talent.essence_break)|cooldown.metamorphosis.remains<=5&buff.inertia_trigger.down|buff.inertia.up&buff.demonsurge_abyssal_gaze.up)|talent.inertia&buff.inertia_trigger.down&cooldown.vengeful_retreat.remains>=gcd.max&buff.inertia.down" );
  fs_meta->add_action( "annihilation,if=buff.metamorphosis.remains<gcd.max&cooldown.blade_dance.remains<buff.metamorphosis.remains|debuff.essence_break.remains&debuff.essence_break.remains<0.5|talent.restless_hunter&(buff.demonsurge_annihilation.up|hero_tree.aldrachi_reaver&buff.inner_demon.up)&cooldown.essence_break.up&cooldown.metamorphosis.up" );
  fs_meta->add_action( "annihilation,if=(hero_tree.felscarred&buff.demonsurge_annihilation.up&talent.restless_hunter)&(cooldown.eye_beam.remains<gcd.max*3&cooldown.blade_dance.remains|cooldown.metamorphosis.remains<gcd.max*3)" );
  fs_meta->add_action( "felblade,if=buff.inertia_trigger.up&talent.inertia&debuff.essence_break.down&cooldown.metamorphosis.remains&(!hero_tree.felscarred|cooldown.eye_beam.remains)&(cooldown.blade_dance.remains<=5.5&(talent.essence_break&cooldown.essence_break.remains<=0.5|!talent.essence_break|cooldown.essence_break.remains>=buff.inertia_trigger.remains&cooldown.blade_dance.remains<=4.5&(cooldown.blade_dance.remains|cooldown.blade_dance.remains<=0.5))|buff.metamorphosis.remains<=5.5+talent.shattered_destiny*2)" );
  fs_meta->add_action( "fel_rush,if=buff.inertia_trigger.up&talent.inertia&debuff.essence_break.down&cooldown.metamorphosis.remains&(!hero_tree.felscarred|cooldown.eye_beam.remains)&(active_enemies>2|hero_tree.felscarred)&(cooldown.felblade.remains&cooldown.essence_break.remains<=0.6)" );
  fs_meta->add_action( "annihilation,if=buff.inner_demon.up&cooldown.blade_dance.remains&(cooldown.eye_beam.remains<gcd.max*3|cooldown.metamorphosis.remains<gcd.max*3)" );
  fs_meta->add_action( "essence_break,if=fury>20&(cooldown.metamorphosis.remains>10|cooldown.blade_dance.remains<gcd.max*2)&(buff.inertia_trigger.down|buff.inertia.up&buff.inertia.remains>=gcd.max*3|!talent.inertia)&buff.out_of_range.remains<gcd.max&(!talent.shattered_destiny|cooldown.eye_beam.remains>4)&(!hero_tree.felscarred|active_enemies>1|cooldown.metamorphosis.remains>5&cooldown.eye_beam.remains)&(!buff.cycle_of_hatred.stack=3|buff.initiative.up)|fight_remains<5" );
  fs_meta->add_action( "sigil_of_doom,if=cooldown.blade_dance.remains&debuff.essence_break.down&(cooldown.eye_beam.remains>=20|cooldown.eye_beam.remains<=gcd.max)&(!talent.student_of_suffering|buff.demonsurge_sigil_of_doom.up)" );
  fs_meta->add_action( "immolation_aura,if=buff.demonsurge.up&debuff.essence_break.down&buff.demonsurge_consuming_fire.up&cooldown.blade_dance.remains>=gcd.max&cooldown.eye_beam.remains>=gcd.max&fury.deficit>10+variable.fury_gen" );
  fs_meta->add_action( "eye_beam,if=debuff.essence_break.down&buff.inner_demon.down" );
  fs_meta->add_action( "abyssal_gaze,if=debuff.essence_break.down&buff.inner_demon.down&(buff.cycle_of_hatred.stack<4|cooldown.essence_break.remains>=20-gcd.max*talent.student_of_suffering|cooldown.sigil_of_flame.remains&talent.student_of_suffering|cooldown.essence_break.remains<=gcd.max)" );
  fs_meta->add_action( "death_sweep,if=cooldown.essence_break.remains>=gcd.max*2+talent.student_of_suffering*gcd.max|debuff.essence_break.up|!talent.essence_break" );
  fs_meta->add_action( "annihilation,if=cooldown.blade_dance.remains|fury>60|soul_fragments.total>0|buff.metamorphosis.remains<5" );
  fs_meta->add_action( "immolation_aura,if=buff.out_of_range.down&recharge_time<(cooldown.eye_beam.remains<?buff.metamorphosis.remains)&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>full_recharge_time)" );
  fs_meta->add_action( "felblade,if=(buff.out_of_range.down|fury.deficit>40+variable.fury_gen*(0.5%gcd.max))&!buff.inertia_trigger.up" );
  fs_meta->add_action( "annihilation" );
  fs_meta->add_action( "throw_glaive,if=buff.unbound_chaos.down&recharge_time<cooldown.eye_beam.remains&debuff.essence_break.down&(cooldown.eye_beam.remains>8|charges_fractional>1.01)&buff.out_of_range.down&active_enemies>1" );
  fs_meta->add_action( "fel_rush,if=recharge_time<cooldown.eye_beam.remains&debuff.essence_break.down&(cooldown.eye_beam.remains>8|charges_fractional>1.01)&buff.out_of_range.down&active_enemies>1" );
  fs_meta->add_action( "demons_bite" );

  fs_opener->add_action( "potion,if=buff.initiative.up|!talent.initiative" );
  fs_opener->add_action( "felblade,if=cooldown.the_hunt.up&!talent.a_fire_inside&fury<40" );
  fs_opener->add_action( "the_hunt,if=talent.inertia|buff.initiative.up|!talent.initiative" );
  fs_opener->add_action( "felblade,if=talent.inertia&buff.inertia_trigger.up&active_enemies=1&buff.metamorphosis.up&cooldown.metamorphosis.up&cooldown.essence_break.up&buff.inner_demon.down&buff.demonsurge_annihilation.down" );
  fs_opener->add_action( "fel_rush,if=talent.inertia&buff.inertia_trigger.up&(cooldown.felblade.remains|active_enemies>1)&buff.metamorphosis.up&cooldown.metamorphosis.up&cooldown.essence_break.up&buff.inner_demon.down&buff.demonsurge_annihilation.down" );
  fs_opener->add_action( "essence_break,if=buff.metamorphosis.up&(!talent.inertia|buff.inertia.up)&buff.inner_demon.down&buff.demonsurge_annihilation.down" );
  fs_opener->add_action( "vengeful_retreat,use_off_gcd=1,if=talent.initiative&time>4&buff.metamorphosis.up&(!talent.inertia|buff.inertia_trigger.down)&talent.essence_break&buff.inner_demon.down&(buff.initiative.down|gcd.remains<0.1)&cooldown.blade_dance.remains" );
  fs_opener->add_action( "felblade,if=talent.inertia&buff.inertia_trigger.up&hero_tree.felscarred&debuff.essence_break.down&talent.essence_break&cooldown.metamorphosis.remains&active_enemies<=2&cooldown.sigil_of_flame.remains" );
  fs_opener->add_action( "sigil_of_doom,if=(buff.inner_demon.down|buff.out_of_range.up)&debuff.essence_break.down" );
  fs_opener->add_action( "annihilation,if=(buff.inner_demon.up|buff.demonsurge_annihilation.up)&(cooldown.metamorphosis.up|!talent.essence_break&cooldown.blade_dance.remains)" );
  fs_opener->add_action( "death_sweep,if=hero_tree.felscarred&buff.demonsurge_death_sweep.up&!talent.restless_hunter" );
  fs_opener->add_action( "annihilation,if=hero_tree.felscarred&buff.demonsurge_annihilation.up&(!talent.essence_break|buff.inner_demon.up)" );
  fs_opener->add_action( "immolation_aura,if=talent.a_fire_inside&talent.burning_wound&buff.metamorphosis.down" );
  fs_opener->add_action( "felblade,if=fury<40&debuff.essence_break.down&buff.inertia_trigger.down&cooldown.metamorphosis.up" );
  fs_opener->add_action( "metamorphosis,if=buff.metamorphosis.up&buff.inner_demon.down&buff.demonsurge_annihilation.down&cooldown.blade_dance.remains" );
  fs_opener->add_action( "eye_beam,if=buff.metamorphosis.down|debuff.essence_break.down&buff.inner_demon.down&(cooldown.blade_dance.remains|talent.essence_break&cooldown.essence_break.up)&(!talent.a_fire_inside|action.immolation_aura.charges=0)" );
  fs_opener->add_action( "abyssal_gaze,if=debuff.essence_break.down&buff.inner_demon.down" );
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
  action_priority_list_t* ar_execute = p->get_action_priority_list( "ar_execute" );
  action_priority_list_t* externals = p->get_action_priority_list( "externals" );
  action_priority_list_t* fel_dev = p->get_action_priority_list( "fel_dev" );
  action_priority_list_t* fel_dev_prep = p->get_action_priority_list( "fel_dev_prep" );
  action_priority_list_t* fs = p->get_action_priority_list( "fs" );
  action_priority_list_t* fs_execute = p->get_action_priority_list( "fs_execute" );
  action_priority_list_t* meta_prep = p->get_action_priority_list( "meta_prep" );
  action_priority_list_t* metamorphosis = p->get_action_priority_list( "metamorphosis" );
  action_priority_list_t* rg_overflow = p->get_action_priority_list( "rg_overflow" );
  action_priority_list_t* rg_prep = p->get_action_priority_list( "rg_prep" );
  action_priority_list_t* rg_sequence = p->get_action_priority_list( "rg_sequence" );
  action_priority_list_t* rg_sequence_filler = p->get_action_priority_list( "rg_sequence_filler" );

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

  ar->add_action( "variable,name=spb_threshold,op=setif,condition=talent.fiery_demise&dot.fiery_brand.ticking,value=(variable.single_target*5)+(variable.small_aoe*5)+(variable.big_aoe*4),value_else=(variable.single_target*5)+(variable.small_aoe*5)+(variable.big_aoe*4)" );
  ar->add_action( "variable,name=can_spb,op=setif,condition=talent.spirit_bomb,value=soul_fragments>=variable.spb_threshold,value_else=0" );
  ar->add_action( "variable,name=can_spb_soon,op=setif,condition=talent.spirit_bomb,value=soul_fragments.total>=variable.spb_threshold,value_else=0" );
  ar->add_action( "variable,name=can_spb_one_gcd,op=setif,condition=talent.spirit_bomb,value=(soul_fragments.total+variable.num_spawnable_souls)>=variable.spb_threshold,value_else=0" );
  ar->add_action( "variable,name=double_rm_remains,op=setif,condition=(variable.double_rm_expires-time)>0,value=variable.double_rm_expires-time,value_else=0" );
  ar->add_action( "variable,name=trigger_overflow,op=set,value=0,if=!buff.glaive_flurry.up&!buff.rending_strike.up&!prev_gcd.1.reavers_glaive" );
  ar->add_action( "variable,name=rg_enhance_cleave,op=setif,condition=variable.trigger_overflow|(spell_targets.spirit_bomb>=4)|(fight_remains<10|target.time_to_die<10),value=1,value_else=0" );
  ar->add_action( "variable,name=souls_before_next_rg_sequence,value=soul_fragments.total+buff.art_of_the_glaive.stack" );
  ar->add_action( "variable,name=souls_before_next_rg_sequence,op=add,value=(1.1*(1+raw_haste_pct))*(variable.double_rm_remains-(action.reavers_glaive.execute_time+action.fracture.execute_time+action.soul_cleave.execute_time+gcd.remains+(0.5*gcd.max)))" );
  ar->add_action( "variable,name=souls_before_next_rg_sequence,op=add,value=3+talent.soul_sigils,if=cooldown.sigil_of_spite.remains<(variable.double_rm_remains-gcd.max-(2-talent.soul_sigils))" );
  ar->add_action( "variable,name=souls_before_next_rg_sequence,op=add,value=3,if=cooldown.soul_carver.remains<(variable.double_rm_remains-gcd.max)" );
  ar->add_action( "variable,name=souls_before_next_rg_sequence,op=add,value=3,if=cooldown.soul_carver.remains<(variable.double_rm_remains-gcd.max-3)" );
  ar->add_action( "use_item,slot=trinket1,if=!variable.trinket_1_buffs|(variable.trinket_1_buffs&((buff.rending_strike.up&buff.glaive_flurry.up)|(prev_gcd.1.reavers_glaive)|(buff.thrill_of_the_fight_damage.remains>8)|(buff.reavers_glaive.up&cooldown.the_hunt.remains<5)))" );
  ar->add_action( "use_item,slot=trinket2,if=!variable.trinket_2_buffs|(variable.trinket_2_buffs&((buff.rending_strike.up&buff.glaive_flurry.up)|(prev_gcd.1.reavers_glaive)|(buff.thrill_of_the_fight_damage.remains>8)|(buff.reavers_glaive.up&cooldown.the_hunt.remains<5)))" );
  ar->add_action( "potion,use_off_gcd=1,if=(buff.rending_strike.up&buff.glaive_flurry.up)|prev_gcd.1.reavers_glaive" );
  ar->add_action( "call_action_list,name=externals,if=(buff.rending_strike.up&buff.glaive_flurry.up)|prev_gcd.1.reavers_glaive" );
  ar->add_action( "run_action_list,name=rg_sequence,if=buff.glaive_flurry.up|buff.rending_strike.up|prev_gcd.1.reavers_glaive" );
  ar->add_action( "metamorphosis,use_off_gcd=1,if=time<5|cooldown.fel_devastation.remains>=20" );
  ar->add_action( "the_hunt,if=!buff.reavers_glaive.up&(buff.art_of_the_glaive.stack+soul_fragments.total)<20" );
  ar->add_action( "spirit_bomb,if=variable.can_spb&(soul_fragments.inactive>2|prev_gcd.1.sigil_of_spite|prev_gcd.1.soul_carver|(spell_targets.spirit_bomb>=4&talent.fallout&cooldown.immolation_aura.remains<gcd.max))" );
  ar->add_action( "immolation_aura,if=(spell_targets.spirit_bomb>=4)|(!buff.reavers_glaive.up|(variable.double_rm_remains>((action.reavers_glaive.execute_time+action.fracture.execute_time+action.soul_cleave.execute_time+gcd.remains+(0.5*gcd.max))+gcd.max)))" );
  ar->add_action( "sigil_of_flame,if=(talent.ascending_flame|(!prev_gcd.1.sigil_of_flame&dot.sigil_of_flame.remains<(4-talent.quickened_sigils)))&(!buff.reavers_glaive.up|(variable.double_rm_remains>((action.reavers_glaive.execute_time+action.fracture.execute_time+action.soul_cleave.execute_time+gcd.remains+(0.5*gcd.max))+gcd.max)))" );
  ar->add_action( "run_action_list,name=rg_overflow,if=buff.reavers_glaive.up&!(spell_targets.spirit_bomb>=4)&debuff.reavers_mark.up&(variable.double_rm_remains>(action.reavers_glaive.execute_time+action.fracture.execute_time+action.soul_cleave.execute_time+gcd.remains+(0.5*gcd.max)))&(!buff.thrill_of_the_fight_damage.up|(buff.thrill_of_the_fight_damage.remains<(action.reavers_glaive.execute_time+action.fracture.execute_time+action.soul_cleave.execute_time+gcd.remains+(0.5*gcd.max))))&((variable.double_rm_remains-(action.reavers_glaive.execute_time+action.fracture.execute_time+action.soul_cleave.execute_time+gcd.remains+(0.5*gcd.max)))>(action.reavers_glaive.execute_time+action.fracture.execute_time+action.soul_cleave.execute_time+gcd.remains+(0.5*gcd.max)))&((variable.souls_before_next_rg_sequence>=20)|(variable.double_rm_remains>((action.reavers_glaive.execute_time+action.fracture.execute_time+action.soul_cleave.execute_time+gcd.remains+(0.5*gcd.max))+cooldown.the_hunt.remains+action.the_hunt.execute_time)))" );
  ar->add_action( "call_action_list,name=ar_execute,if=(fight_remains<10|target.time_to_die<10)" );
  ar->add_action( "soul_cleave,if=!buff.reavers_glaive.up&(variable.double_rm_remains<=(execute_time+(action.reavers_glaive.execute_time+action.fracture.execute_time+action.soul_cleave.execute_time+gcd.remains+(0.5*gcd.max))))&(soul_fragments<3&((buff.art_of_the_glaive.stack+soul_fragments)>=20))" );
  ar->add_action( "spirit_bomb,if=!buff.reavers_glaive.up&(variable.double_rm_remains<=(execute_time+(action.reavers_glaive.execute_time+action.fracture.execute_time+action.soul_cleave.execute_time+gcd.remains+(0.5*gcd.max))))&((buff.art_of_the_glaive.stack+soul_fragments)>=20)" );
  ar->add_action( "bulk_extraction,if=!buff.reavers_glaive.up&(variable.double_rm_remains<=(execute_time+(action.reavers_glaive.execute_time+action.fracture.execute_time+action.soul_cleave.execute_time+gcd.remains+(0.5*gcd.max))))&((buff.art_of_the_glaive.stack+(spell_targets>?5))>=20)" );
  ar->add_action( "reavers_glaive,if=(fury+(variable.rg_enhance_cleave*25)+(talent.keen_engagement*20))>=30&((!buff.thrill_of_the_fight_attack_speed.up|(variable.double_rm_remains<=(action.reavers_glaive.execute_time+action.fracture.execute_time+action.soul_cleave.execute_time+gcd.remains+(0.5*gcd.max))))|(spell_targets.spirit_bomb>=4))&!(buff.rending_strike.up|buff.glaive_flurry.up)" );
  ar->add_action( "call_action_list,name=rg_prep,if=!(fury+(variable.rg_enhance_cleave*25)+(talent.keen_engagement*20))>=30&((!buff.thrill_of_the_fight_attack_speed.up|(variable.double_rm_remains<=(action.reavers_glaive.execute_time+action.fracture.execute_time+action.soul_cleave.execute_time+gcd.remains+(0.5*gcd.max))))|(spell_targets.spirit_bomb>=4))" );
  ar->add_action( "fiery_brand,if=(!talent.fiery_demise&active_dot.fiery_brand=0)|(talent.down_in_flames&(full_recharge_time<gcd.max))|(talent.fiery_demise&active_dot.fiery_brand=0&(buff.reavers_glaive.up|cooldown.the_hunt.remains<5|buff.art_of_the_glaive.stack>=15|buff.thrill_of_the_fight_damage.remains>5))" );
  ar->add_action( "sigil_of_spite,if=buff.thrill_of_the_fight_damage.up|(fury>=80&(variable.can_spb|variable.can_spb_soon))|((soul_fragments.total+buff.art_of_the_glaive.stack+((1.1*(1+raw_haste_pct))*(variable.double_rm_remains-(action.reavers_glaive.execute_time+action.fracture.execute_time+action.soul_cleave.execute_time+gcd.remains+(0.5*gcd.max)))))<20)" );
  ar->add_action( "spirit_bomb,if=variable.can_spb" );
  ar->add_action( "felblade,if=(variable.can_spb|variable.can_spb_soon)&fury<40" );
  ar->add_action( "vengeful_retreat,use_off_gcd=1,if=(variable.can_spb|variable.can_spb_soon)&fury<40&!cooldown.felblade.up&talent.unhindered_assault" );
  ar->add_action( "fracture,if=(variable.can_spb|variable.can_spb_soon|variable.can_spb_one_gcd)&fury<40" );
  ar->add_action( "soul_carver,if=buff.thrill_of_the_fight_damage.up|((soul_fragments.total+buff.art_of_the_glaive.stack+((1.1*(1+raw_haste_pct))*(variable.double_rm_remains-(action.reavers_glaive.execute_time+action.fracture.execute_time+action.soul_cleave.execute_time+gcd.remains+(0.5*gcd.max)))))<20)" );
  ar->add_action( "fel_devastation,if=!buff.metamorphosis.up&((variable.double_rm_remains>((action.reavers_glaive.execute_time+action.fracture.execute_time+action.soul_cleave.execute_time+gcd.remains+(0.5*gcd.max))+2))|(spell_targets.spirit_bomb>=4))&((action.fracture.full_recharge_time<(2+gcd.max))|(!variable.single_target&buff.thrill_of_the_fight_damage.up))" );
  ar->add_action( "felblade,if=cooldown.fel_devastation.remains<gcd.max&fury<50" );
  ar->add_action( "vengeful_retreat,use_off_gcd=1,if=cooldown.fel_devastation.remains<gcd.max&fury<50&!cooldown.felblade.up&talent.unhindered_assault" );
  ar->add_action( "fracture,if=cooldown.fel_devastation.remains<gcd.max&fury<50" );
  ar->add_action( "fracture,if=(full_recharge_time<gcd.max)|buff.metamorphosis.up|variable.can_spb|variable.can_spb_soon|buff.warblades_hunger.stack>=5" );
  ar->add_action( "soul_cleave,if=soul_fragments>=1" );
  ar->add_action( "bulk_extraction,if=spell_targets>=3" );
  ar->add_action( "fracture" );
  ar->add_action( "soul_cleave" );
  ar->add_action( "shear" );
  ar->add_action( "felblade" );
  ar->add_action( "throw_glaive" );

  ar_execute->add_action( "metamorphosis,use_off_gcd=1" );
  ar_execute->add_action( "reavers_glaive,if=(fury+(variable.rg_enhance_cleave*25)+(talent.keen_engagement*20))>=30&!(buff.rending_strike.up|buff.glaive_flurry.up)" );
  ar_execute->add_action( "call_action_list,name=rg_prep,if=buff.reavers_glaive.up&!(fury+(variable.rg_enhance_cleave*25)+(talent.keen_engagement*20))>=30" );
  ar_execute->add_action( "the_hunt,if=!buff.reavers_glaive.up" );
  ar_execute->add_action( "bulk_extraction,if=spell_targets>=3&buff.art_of_the_glaive.stack>=20" );
  ar_execute->add_action( "sigil_of_flame" );
  ar_execute->add_action( "fiery_brand" );
  ar_execute->add_action( "sigil_of_spite" );
  ar_execute->add_action( "soul_carver" );
  ar_execute->add_action( "fel_devastation" );

  externals->add_action( "invoke_external_buff,name=symbol_of_hope" );
  externals->add_action( "invoke_external_buff,name=power_infusion" );

  fel_dev->add_action( "spirit_burst,if=buff.demonsurge_spirit_burst.up&(variable.can_spburst|soul_fragments>=4|(buff.metamorphosis.remains<(gcd.max*2)))" );
  fel_dev->add_action( "soul_sunder,if=buff.demonsurge_soul_sunder.up&(!buff.demonsurge_spirit_burst.up|(buff.metamorphosis.remains<(gcd.max*2)))" );
  fel_dev->add_action( "sigil_of_spite,if=(!talent.cycle_of_binding|(cooldown.sigil_of_spite.duration<(cooldown.metamorphosis.remains+18)))&(soul_fragments.total<=2&buff.demonsurge_spirit_burst.up)" );
  fel_dev->add_action( "soul_carver,if=soul_fragments.total<=2&!prev_gcd.1.sigil_of_spite&buff.demonsurge_spirit_burst.up" );
  fel_dev->add_action( "fracture,if=soul_fragments.total<=2&buff.demonsurge_spirit_burst.up" );
  fel_dev->add_action( "felblade,if=buff.demonsurge_spirit_burst.up|buff.demonsurge_soul_sunder.up" );
  fel_dev->add_action( "fracture,if=buff.demonsurge_spirit_burst.up|buff.demonsurge_soul_sunder.up" );

  fel_dev_prep->add_action( "potion,use_off_gcd=1,if=prev_gcd.1.fiery_brand" );
  fel_dev_prep->add_action( "sigil_of_flame,if=!variable.hold_sof_for_precombat&!variable.hold_sof_for_student&!variable.hold_sof_for_dot" );
  fel_dev_prep->add_action( "fiery_brand,if=talent.fiery_demise&((fury+variable.fel_dev_passive_fury_gen)>=120)&(variable.can_spburst|variable.can_spburst_soon|soul_fragments.total>=4)&active_dot.fiery_brand=0&((cooldown.metamorphosis.remains<(execute_time+action.fel_devastation.execute_time+(gcd.max*2)))|variable.fiery_brand_back_before_meta)" );
  fel_dev_prep->add_action( "fel_devastation,if=((fury+variable.fel_dev_passive_fury_gen)>=120)&(variable.can_spburst|variable.can_spburst_soon|soul_fragments.total>=4)" );
  fel_dev_prep->add_action( "sigil_of_spite,if=(!talent.cycle_of_binding|(cooldown.sigil_of_spite.duration<(cooldown.metamorphosis.remains+18)))&(soul_fragments.total<=1|(!(variable.can_spburst|variable.can_spburst_soon|soul_fragments.total>=4)&action.fracture.charges_fractional<1))" );
  fel_dev_prep->add_action( "soul_carver,if=(!talent.cycle_of_binding|cooldown.metamorphosis.remains>20)&(soul_fragments.total<=1|(!(variable.can_spburst|variable.can_spburst_soon|soul_fragments.total>=4)&action.fracture.charges_fractional<1))&!prev_gcd.1.sigil_of_spite&!prev_gcd.2.sigil_of_spite" );
  fel_dev_prep->add_action( "felblade,if=!((fury+variable.fel_dev_passive_fury_gen)>=120)&(variable.can_spburst|variable.can_spburst_soon|soul_fragments.total>=4)" );
  fel_dev_prep->add_action( "fracture,if=!(variable.can_spburst|variable.can_spburst_soon|soul_fragments.total>=4)|!((fury+variable.fel_dev_passive_fury_gen)>=120)" );
  fel_dev_prep->add_action( "felblade" );
  fel_dev_prep->add_action( "fracture" );
  fel_dev_prep->add_action( "wait,sec=0.1,if=(!(variable.can_spburst|variable.can_spburst_soon|soul_fragments.total>=4)|!((fury+variable.fel_dev_passive_fury_gen)>=120))&action.fracture.charges_fractional>=0.7" );
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
  fs->add_action( "use_item,slot=trinket1,if=!variable.trinket_1_buffs|(variable.trinket_1_buffs&((buff.metamorphosis.up&buff.demonsurge_hardcast.up)|(buff.metamorphosis.up&!buff.demonsurge_hardcast.up&cooldown.metamorphosis.remains<10)|(cooldown.metamorphosis.remains>trinket.1.cooldown.duration)|(variable.trinket_2_buffs&trinket.2.cooldown.remains<cooldown.metamorphosis.remains)))" );
  fs->add_action( "use_item,slot=trinket2,if=!variable.trinket_2_buffs|(variable.trinket_2_buffs&((buff.metamorphosis.up&buff.demonsurge_hardcast.up)|(buff.metamorphosis.up&!buff.demonsurge_hardcast.up&cooldown.metamorphosis.remains<10)|(cooldown.metamorphosis.remains>trinket.2.cooldown.duration)|(variable.trinket_1_buffs&trinket.1.cooldown.remains<cooldown.metamorphosis.remains)))" );
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
  fs->add_action( "spirit_burst,if=variable.can_spburst&talent.fiery_demise&dot.fiery_brand.ticking&!(cooldown.fel_devastation.remains<(gcd.max*3))" );
  fs->add_action( "spirit_bomb,if=variable.can_spbomb&talent.fiery_demise&dot.fiery_brand.ticking&!(cooldown.fel_devastation.remains<(gcd.max*3))" );
  fs->add_action( "soul_sunder,if=variable.single_target&!variable.dont_soul_cleave" );
  fs->add_action( "soul_cleave,if=variable.single_target&!variable.dont_soul_cleave" );
  fs->add_action( "spirit_burst,if=variable.can_spburst&!(cooldown.fel_devastation.remains<(gcd.max*3))" );
  fs->add_action( "spirit_bomb,if=variable.can_spbomb&!(cooldown.fel_devastation.remains<(gcd.max*3))" );
  fs->add_action( "felblade,if=((fury<40&((buff.metamorphosis.up&(variable.can_spburst|variable.can_spburst_soon))|(!buff.metamorphosis.up&(variable.can_spbomb|variable.can_spbomb_soon)))))" );
  fs->add_action( "fracture,if=((fury<40&((buff.metamorphosis.up&(variable.can_spburst|variable.can_spburst_soon))|(!buff.metamorphosis.up&(variable.can_spbomb|variable.can_spbomb_soon))))|(buff.metamorphosis.up&variable.can_spburst_one_gcd)|(!buff.metamorphosis.up&variable.can_spbomb_one_gcd))" );
  fs->add_action( "felblade,if=fury.deficit>=40" );
  fs->add_action( "soul_sunder,if=!variable.dont_soul_cleave" );
  fs->add_action( "soul_cleave,if=!variable.dont_soul_cleave" );
  fs->add_action( "fracture" );
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
  metamorphosis->add_action( "fel_desolation,if=buff.metamorphosis.remains<(gcd.max*3)" );
  metamorphosis->add_action( "felblade,if=fury<50&(buff.metamorphosis.remains<(gcd.max*3))&cooldown.fel_desolation.up" );
  metamorphosis->add_action( "fracture,if=fury<50&!cooldown.felblade.up&(buff.metamorphosis.remains<(gcd.max*3))&cooldown.fel_desolation.up" );
  metamorphosis->add_action( "sigil_of_doom,if=talent.illuminated_sigils&talent.cycle_of_binding&charges=max_charges" );
  metamorphosis->add_action( "immolation_aura" );
  metamorphosis->add_action( "sigil_of_doom,if=!talent.student_of_suffering&(talent.ascending_flame|(!talent.ascending_flame&!prev_gcd.1.sigil_of_doom&(dot.sigil_of_doom.remains<(4-talent.quickened_sigils))))" );
  metamorphosis->add_action( "sigil_of_doom,if=talent.student_of_suffering&!prev_gcd.1.sigil_of_flame&!prev_gcd.1.sigil_of_doom&(buff.student_of_suffering.remains<(4-talent.quickened_sigils))" );
  metamorphosis->add_action( "sigil_of_doom,if=buff.metamorphosis.remains<((2-talent.quickened_sigils)+(charges*gcd.max))" );
  metamorphosis->add_action( "fel_desolation,if=soul_fragments<=3&(soul_fragments.inactive>=2|prev_gcd.1.sigil_of_spite)" );
  metamorphosis->add_action( "felblade,if=((cooldown.sigil_of_spite.remains<execute_time|cooldown.soul_carver.remains<execute_time)&cooldown.fel_desolation.remains<(execute_time+gcd.max)&fury<50)" );
  metamorphosis->add_action( "soul_carver,if=(!talent.spirit_bomb|(variable.single_target&!buff.demonsurge_spirit_burst.up))|(((soul_fragments.total+3)<=6)&fury>=40&!prev_gcd.1.sigil_of_spite)" );
  metamorphosis->add_action( "sigil_of_spite,if=!talent.spirit_bomb|(fury>=80&(variable.can_spburst|variable.can_spburst_soon))|(soul_fragments.total<=(2-talent.soul_sigils.rank))" );
  metamorphosis->add_action( "spirit_burst,if=variable.can_spburst&buff.demonsurge_spirit_burst.up" );
  metamorphosis->add_action( "fel_desolation" );
  metamorphosis->add_action( "the_hunt" );
  metamorphosis->add_action( "soul_sunder,if=buff.demonsurge_soul_sunder.up&!buff.demonsurge_spirit_burst.up&!variable.can_spburst_one_gcd" );
  metamorphosis->add_action( "spirit_burst,if=variable.can_spburst&(talent.fiery_demise&dot.fiery_brand.ticking|variable.big_aoe)&buff.metamorphosis.remains>(gcd.max*2)" );
  metamorphosis->add_action( "felblade,if=fury<40&(variable.can_spburst|variable.can_spburst_soon)&(buff.demonsurge_spirit_burst.up|talent.fiery_demise&dot.fiery_brand.ticking|variable.big_aoe)" );
  metamorphosis->add_action( "fracture,if=fury<40&(variable.can_spburst|variable.can_spburst_soon|variable.can_spburst_one_gcd)&(buff.demonsurge_spirit_burst.up|talent.fiery_demise&dot.fiery_brand.ticking|variable.big_aoe)" );
  metamorphosis->add_action( "fracture,if=variable.can_spburst_one_gcd&(buff.demonsurge_spirit_burst.up|variable.big_aoe)&!prev_gcd.1.fracture" );
  metamorphosis->add_action( "soul_sunder,if=variable.single_target&!variable.dont_soul_cleave" );
  metamorphosis->add_action( "spirit_burst,if=variable.can_spburst&buff.metamorphosis.remains>(gcd.max*2)" );
  metamorphosis->add_action( "felblade,if=fury.deficit>=40" );
  metamorphosis->add_action( "soul_sunder,if=!variable.dont_soul_cleave&!(variable.big_aoe&(variable.can_spburst|variable.can_spburst_soon))" );
  metamorphosis->add_action( "felblade" );
  metamorphosis->add_action( "fracture,if=!prev_gcd.1.fracture" );

  rg_overflow->add_action( "variable,name=trigger_overflow,op=set,value=1" );
  rg_overflow->add_action( "variable,name=rg_enhance_cleave,op=set,value=1" );
  rg_overflow->add_action( "reavers_glaive,if=(fury+(variable.rg_enhance_cleave*25)+(talent.keen_engagement*20))>=30&!buff.rending_strike.up&!buff.glaive_flurry.up" );
  rg_overflow->add_action( "call_action_list,name=rg_prep,if=!(fury+(variable.rg_enhance_cleave*25)+(talent.keen_engagement*20))>=30" );

  rg_prep->add_action( "felblade" );
  rg_prep->add_action( "vengeful_retreat,use_off_gcd=1,if=!cooldown.felblade.up&talent.unhindered_assault" );
  rg_prep->add_action( "sigil_of_flame" );
  rg_prep->add_action( "immolation_aura" );
  rg_prep->add_action( "fracture" );

  rg_sequence->add_action( "variable,name=double_rm_expires,value=time+action.fracture.execute_time+20,if=!buff.glaive_flurry.up&buff.rending_strike.up" );
  rg_sequence->add_action( "call_action_list,name=rg_sequence_filler,if=(fury<30&((!variable.rg_enhance_cleave&buff.glaive_flurry.up&buff.rending_strike.up)|(variable.rg_enhance_cleave&!buff.rending_strike.up)))|(action.fracture.charges_fractional<1&((variable.rg_enhance_cleave&buff.rending_strike.up&buff.glaive_flurry.up)|(!variable.rg_enhance_cleave&!buff.glaive_flurry.up)))" );
  rg_sequence->add_action( "fracture,if=((variable.rg_enhance_cleave&buff.rending_strike.up&buff.glaive_flurry.up)|(!variable.rg_enhance_cleave&!buff.glaive_flurry.up))" );
  rg_sequence->add_action( "shear,if=((variable.rg_enhance_cleave&buff.rending_strike.up&buff.glaive_flurry.up)|(!variable.rg_enhance_cleave&!buff.glaive_flurry.up))" );
  rg_sequence->add_action( "soul_cleave,if=((!variable.rg_enhance_cleave&buff.glaive_flurry.up&buff.rending_strike.up)|(variable.rg_enhance_cleave&!buff.rending_strike.up))" );

  rg_sequence_filler->add_action( "felblade" );
  rg_sequence_filler->add_action( "fracture,if=!buff.rending_strike.up" );
  rg_sequence_filler->add_action( "wait,sec=0.1,if=action.fracture.charges_fractional>=0.8&((variable.rg_enhance_cleave&buff.rending_strike.up&buff.glaive_flurry.up)|(!variable.rg_enhance_cleave&!buff.glaive_flurry.up))" );
  rg_sequence_filler->add_action( "sigil_of_flame" );
  rg_sequence_filler->add_action( "sigil_of_spite" );
  rg_sequence_filler->add_action( "soul_carver" );
  rg_sequence_filler->add_action( "fel_devastation" );
  rg_sequence_filler->add_action( "throw_glaive" );
}
//vengeance_apl_end

//vengeance_ptr_apl_start
//vengeance_ptr_apl_end

}  // namespace demon_hunter_apl
