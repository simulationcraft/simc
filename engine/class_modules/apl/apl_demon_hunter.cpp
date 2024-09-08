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
  action_priority_list_t* cooldown = p->get_action_priority_list( "cooldown" );
  action_priority_list_t* fel_barrage = p->get_action_priority_list( "fel_barrage" );
  action_priority_list_t* meta = p->get_action_priority_list( "meta" );
  action_priority_list_t* opener = p->get_action_priority_list( "opener" );

  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=trinket1_steroids,value=trinket.1.has_stat.any_dps" );
  precombat->add_action( "variable,name=trinket2_steroids,value=trinket.2.has_stat.any_dps" );
  precombat->add_action( "variable,name=rg_ds,default=0,op=reset" );
  precombat->add_action( "sigil_of_flame" );
  precombat->add_action( "immolation_aura" );

  default_->add_action( "auto_attack,if=!buff.out_of_range.up" );
  default_->add_action( "retarget_auto_attack,line_cd=1,target_if=min:debuff.burning_wound.remains,if=talent.burning_wound&talent.demon_blades&active_dot.burning_wound<(spell_targets>?3)" );
  default_->add_action( "retarget_auto_attack,line_cd=1,target_if=min:!target.is_boss,if=talent.burning_wound&talent.demon_blades&active_dot.burning_wound=(spell_targets>?3)" );
  default_->add_action( "variable,name=rg_inc,op=set,value=buff.rending_strike.down&buff.glaive_flurry.up&cooldown.blade_dance.up&gcd.remains=0|variable.rg_inc&prev_gcd.1.death_sweep" );
  default_->add_action( "pick_up_fragment,use_off_gcd=1" );
  default_->add_action( "variable,name=fel_barrage,op=set,value=talent.fel_barrage&(cooldown.fel_barrage.remains<gcd.max*7&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in<gcd.max*7|raid_event.adds.in>90)&(cooldown.metamorphosis.remains|active_enemies>2)|buff.fel_barrage.up)&!(active_enemies=1&!raid_event.adds.exists)" );
  default_->add_action( "disrupt" );
  default_->add_action( "fel_rush,if=buff.unbound_chaos.up&buff.unbound_chaos.remains<gcd.max*2" );
  default_->add_action( "chaos_strike,if=buff.rending_strike.up&buff.glaive_flurry.up&(variable.rg_ds=2|active_enemies>2)" );
  default_->add_action( "annihilation,if=buff.rending_strike.up&buff.glaive_flurry.up&(variable.rg_ds=2|active_enemies>2)" );
  default_->add_action( "reavers_glaive,if=buff.glaive_flurry.down&buff.rending_strike.down&buff.thrill_of_the_fight_damage.remains<gcd.max*4+(variable.rg_ds=2)+(cooldown.the_hunt.remains<gcd.max*3)*3+(cooldown.eye_beam.remains<gcd.max*3&talent.shattered_destiny)*3&(variable.rg_ds=0|variable.rg_ds=1&cooldown.blade_dance.up|variable.rg_ds=2&cooldown.blade_dance.remains)&(buff.thrill_of_the_fight_damage.up|!prev_gcd.1.death_sweep|!variable.rg_inc)&active_enemies<3&!action.reavers_glaive.last_used<5&debuff.essence_break.down|fight_remains<10" );
  default_->add_action( "reavers_glaive,if=buff.glaive_flurry.down&buff.rending_strike.down&buff.thrill_of_the_fight_damage.remains<4&(buff.thrill_of_the_fight_damage.up|!prev_gcd.1.death_sweep|!variable.rg_inc)&active_enemies>2|fight_remains<10" );
  default_->add_action( "call_action_list,name=cooldown" );
  default_->add_action( "run_action_list,name=opener,if=(cooldown.eye_beam.up|cooldown.metamorphosis.up|cooldown.essence_break.up)&time<15&(raid_event.adds.in>40)&buff.demonsurge.stack<5" );
  default_->add_action( "sigil_of_spite,if=debuff.essence_break.down&debuff.reavers_mark.remains>=2-talent.quickened_sigils" );
  default_->add_action( "run_action_list,name=fel_barrage,if=variable.fel_barrage&raid_event.adds.up" );
  default_->add_action( "immolation_aura,if=active_enemies>2&talent.ragefire&buff.unbound_chaos.down&(!talent.fel_barrage|cooldown.fel_barrage.remains>recharge_time)&debuff.essence_break.down&cooldown.eye_beam.remains>recharge_time+5&(buff.metamorphosis.down|buff.metamorphosis.remains>5)" );
  default_->add_action( "immolation_aura,if=hero_tree.felscarred&cooldown.metamorphosis.remains<10&cooldown.eye_beam.remains<10&(buff.unbound_chaos.down|action.fel_rush.charges=0|(cooldown.eye_beam.remains<?cooldown.metamorphosis.remains)<5)&talent.a_fire_inside" );
  default_->add_action( "immolation_aura,if=active_enemies>2&talent.ragefire&raid_event.adds.up&raid_event.adds.remains<15&raid_event.adds.remains>5&debuff.essence_break.down" );
  default_->add_action( "fel_rush,if=buff.unbound_chaos.up&active_enemies>2&(!talent.inertia|cooldown.eye_beam.remains+2>buff.unbound_chaos.remains)" );
  default_->add_action( "vengeful_retreat,use_off_gcd=1,if=talent.initiative&(cooldown.eye_beam.remains>15&gcd.remains<0.3|gcd.remains<0.2&cooldown.eye_beam.remains<=gcd.remains&(buff.unbound_chaos.up|action.immolation_aura.recharge_time>6|!talent.inertia|talent.momentum)&(cooldown.metamorphosis.remains>10|cooldown.blade_dance.remains<gcd.max*2&(talent.inertia|talent.momentum|buff.metamorphosis.up)))&(!talent.student_of_suffering|cooldown.sigil_of_flame.remains)&time>10&(!variable.trinket1_steroids&!variable.trinket2_steroids|variable.trinket1_steroids&(trinket.1.cooldown.remains<gcd.max*3|trinket.1.cooldown.remains>20)|variable.trinket2_steroids&(trinket.2.cooldown.remains<gcd.max*3|trinket.2.cooldown.remains>20|talent.shattered_destiny))" );
  default_->add_action( "run_action_list,name=fel_barrage,if=variable.fel_barrage|!talent.demon_blades&talent.fel_barrage&(buff.fel_barrage.up|cooldown.fel_barrage.up)&buff.metamorphosis.down" );
  default_->add_action( "run_action_list,name=meta,if=buff.metamorphosis.up" );
  default_->add_action( "fel_rush,if=buff.unbound_chaos.up&talent.inertia&buff.inertia.down&cooldown.blade_dance.remains<4&cooldown.eye_beam.remains>5&(action.immolation_aura.charges>0|action.immolation_aura.recharge_time+2<cooldown.eye_beam.remains|cooldown.eye_beam.remains>buff.unbound_chaos.remains-2)" );
  default_->add_action( "fel_rush,if=talent.momentum&cooldown.eye_beam.remains<gcd.max*2" );
  default_->add_action( "immolation_aura,if=talent.a_fire_inside&(talent.unbound_chaos|talent.burning_wound)&buff.unbound_chaos.down&full_recharge_time<gcd.max*2&(raid_event.adds.in>full_recharge_time|active_enemies>desired_targets)" );
  default_->add_action( "immolation_aura,if=active_enemies>desired_targets&buff.unbound_chaos.down&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>full_recharge_time)" );
  default_->add_action( "immolation_aura,if=talent.inertia&buff.unbound_chaos.down&cooldown.eye_beam.remains<5&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>full_recharge_time&(!talent.essence_break|cooldown.essence_break.remains<5))&(!variable.trinket1_steroids&!variable.trinket2_steroids|variable.trinket1_steroids&(trinket.1.cooldown.remains<gcd.max*3|trinket.1.cooldown.remains>20)|variable.trinket2_steroids&(trinket.2.cooldown.remains<gcd.max*3|trinket.2.cooldown.remains>20))" );
  default_->add_action( "immolation_aura,if=talent.inertia&buff.inertia.down&buff.unbound_chaos.down&recharge_time+5<cooldown.eye_beam.remains&cooldown.blade_dance.remains&cooldown.blade_dance.remains<4&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>full_recharge_time)&charges_fractional>1.00" );
  default_->add_action( "immolation_aura,if=fight_remains<15&cooldown.blade_dance.remains&(talent.inertia|talent.ragefire)" );
  default_->add_action( "sigil_of_flame,if=talent.student_of_suffering&cooldown.eye_beam.remains<gcd.max&(cooldown.essence_break.remains<gcd.max*4|!talent.essence_break)&(cooldown.metamorphosis.remains>10|cooldown.blade_dance.remains<gcd.max*3)&(!variable.trinket1_steroids&!variable.trinket2_steroids|variable.trinket1_steroids&(trinket.1.cooldown.remains<gcd.max*3|trinket.1.cooldown.remains>20)|variable.trinket2_steroids&(trinket.2.cooldown.remains<gcd.max*3|trinket.2.cooldown.remains>20))" );
  default_->add_action( "eye_beam,if=!talent.essence_break&(!talent.chaotic_transformation|cooldown.metamorphosis.remains<5+3*talent.shattered_destiny|cooldown.metamorphosis.remains>10)&(active_enemies>desired_targets*2|raid_event.adds.in>30-talent.cycle_of_hatred.rank*13)&(!talent.initiative|cooldown.vengeful_retreat.remains>5|cooldown.vengeful_retreat.up|talent.shattered_destiny)&(!talent.student_of_suffering|cooldown.sigil_of_flame.remains)" );
  default_->add_action( "eye_beam,if=talent.essence_break&(cooldown.essence_break.remains<gcd.max*2+5*talent.shattered_destiny|talent.shattered_destiny&cooldown.essence_break.remains>10)&(cooldown.blade_dance.remains<7|raid_event.adds.up)&(!talent.initiative|cooldown.vengeful_retreat.remains>10|!talent.inertia&!talent.momentum|raid_event.adds.up)&(active_enemies+3>=desired_targets+raid_event.adds.count|raid_event.adds.in>30-talent.cycle_of_hatred.rank*6)&(!talent.inertia|buff.unbound_chaos.up|action.immolation_aura.charges=0&action.immolation_aura.recharge_time>5)&(!raid_event.adds.up|raid_event.adds.remains>8)&(!variable.trinket1_steroids&!variable.trinket2_steroids|variable.trinket1_steroids&(trinket.1.cooldown.remains<gcd.max*3|trinket.1.cooldown.remains>20)|variable.trinket2_steroids&(trinket.2.cooldown.remains<gcd.max*3|trinket.2.cooldown.remains>20))|fight_remains<10" );
  default_->add_action( "blade_dance,if=cooldown.eye_beam.remains>gcd.max*2&buff.rending_strike.down" );
  default_->add_action( "chaos_strike,if=buff.rending_strike.up" );
  default_->add_action( "felblade,if=buff.metamorphosis.down&fury.deficit>40&action.felblade.cooldown_react" );
  default_->add_action( "glaive_tempest,if=active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>10" );
  default_->add_action( "sigil_of_flame,if=active_enemies>3&!talent.student_of_suffering|buff.out_of_range.down&talent.art_of_the_glaive" );
  default_->add_action( "chaos_strike,if=debuff.essence_break.up" );
  default_->add_action( "felblade,if=(buff.out_of_range.down|fury.deficit>40)&action.felblade.cooldown_react", "actions+=/sigil_of_flame,if=talent.student_of_suffering&((cooldown.eye_beam.remains<4&cooldown.metamorphosis.remains>20)|(cooldown.eye_beam.remains<gcd.max&cooldown.metamorphosis.up))" );
  default_->add_action( "throw_glaive,if=active_enemies>1&talent.furious_throws" );
  default_->add_action( "chaos_strike,if=cooldown.eye_beam.remains>gcd.max*2|fury>80|talent.cycle_of_hatred" );
  default_->add_action( "immolation_aura,if=!talent.inertia&(raid_event.adds.in>full_recharge_time|active_enemies>desired_targets&active_enemies>2)" );
  default_->add_action( "sigil_of_flame,if=buff.out_of_range.down&debuff.essence_break.down&(!talent.fel_barrage|cooldown.fel_barrage.remains>25|(active_enemies=1&!raid_event.adds.exists))" );
  default_->add_action( "demons_bite" );
  default_->add_action( "throw_glaive,if=buff.unbound_chaos.down&recharge_time<cooldown.eye_beam.remains&debuff.essence_break.down&(cooldown.eye_beam.remains>8|charges_fractional>1.01)&buff.out_of_range.down&active_enemies>1" );
  default_->add_action( "fel_rush,if=buff.unbound_chaos.down&recharge_time<cooldown.eye_beam.remains&debuff.essence_break.down&(cooldown.eye_beam.remains>8|charges_fractional>1.01)&active_enemies>1" );
  default_->add_action( "arcane_torrent,if=buff.out_of_range.down&debuff.essence_break.down&fury<100" );

  cooldown->add_action( "metamorphosis,if=((!talent.initiative|cooldown.vengeful_retreat.remains)&(cooldown.eye_beam.remains>=20&(!talent.essence_break|debuff.essence_break.up)&buff.fel_barrage.down&(raid_event.adds.in>40|(raid_event.adds.remains>8|!talent.fel_barrage)&active_enemies>2)|!talent.chaotic_transformation|fight_remains<30)&buff.inner_demon.down&(!talent.restless_hunter&cooldown.blade_dance.remains>gcd.max*3|prev_gcd.1.death_sweep))&!talent.inertia&!talent.essence_break&(hero_tree.aldrachi_reaver|buff.demonsurge_death_sweep.down)&time>15" );
  cooldown->add_action( "metamorphosis,if=((!talent.initiative|cooldown.vengeful_retreat.remains)&cooldown.blade_dance.remains&((prev_gcd.1.death_sweep|prev_gcd.2.death_sweep|prev_gcd.3.death_sweep|buff.metamorphosis.up&buff.metamorphosis.remains<gcd.max)&cooldown.eye_beam.remains&(!talent.essence_break|debuff.essence_break.up|talent.shattered_destiny|hero_tree.felscarred)&buff.fel_barrage.down&(raid_event.adds.in>40|(raid_event.adds.remains>8|!talent.fel_barrage)&active_enemies>2)|!talent.chaotic_transformation|fight_remains<30)&(buff.inner_demon.down&(buff.rending_strike.down|!talent.restless_hunter|prev_gcd.1.death_sweep)))&(talent.inertia|talent.essence_break)&(hero_tree.aldrachi_reaver|(buff.demonsurge_death_sweep.down|buff.metamorphosis.remains<gcd.max)&(buff.demonsurge_annihilation.down))&time>15" );
  cooldown->add_action( "potion,if=fight_remains<35|buff.metamorphosis.up|debuff.essence_break.up" );
  cooldown->add_action( "invoke_external_buff,name=power_infusion,if=buff.metamorphosis.up|fight_remains<=20" );
  cooldown->add_action( "variable,name=special_trinket,op=set,value=equipped.mad_queens_mandate|equipped.treacherous_transmitter|equipped.skardyns_grace" );
  cooldown->add_action( "use_item,name=mad_queens_mandate,if=(!talent.initiative|buff.initiative.up|time>5)&(buff.metamorphosis.remains>5|buff.metamorphosis.down)&(trinket.1.is.mad_queens_mandate&(trinket.2.cooldown.duration<10|trinket.2.cooldown.remains>10|!trinket.2.has_buff.any)|trinket.2.is.mad_queens_mandate&(trinket.1.cooldown.duration<10|trinket.1.cooldown.remains>10|!trinket.1.has_buff.any))&fight_remains>120|fight_remains<10" );
  cooldown->add_action( "use_item,name=treacherous_transmitter,if=!equipped.mad_queens_mandate|equipped.mad_queens_mandate&(trinket.1.is.mad_queens_mandate&trinket.1.cooldown.remains>fight_remains|trinket.2.is.mad_queens_mandate&trinket.2.cooldown.remains>fight_remains)|fight_remains>25" );
  cooldown->add_action( "use_item,name=skardyns_grace,if=(!equipped.mad_queens_mandate|fight_remains>25|trinket.2.is.skardyns_grace&trinket.1.cooldown.remains>fight_remains|trinket.1.is.skardyns_grace&trinket.2.cooldown.remains>fight_remains|trinket.1.cooldown.duration<10|trinket.2.cooldown.duration<10)&buff.metamorphosis.up" );
  cooldown->add_action( "do_treacherous_transmitter_task,if=cooldown.eye_beam.remains>15|cooldown.eye_beam.remains<5|fight_remains<20" );
  cooldown->add_action( "use_item,slot=trinket1,if=((cooldown.eye_beam.remains<gcd.max&active_enemies>1|buff.metamorphosis.up)&(raid_event.adds.in>trinket.1.cooldown.duration-15|raid_event.adds.remains>8)|!trinket.1.has_buff.any|fight_remains<25)&!trinket.1.is.skardyns_grace&!trinket.1.is.mad_queens_mandate&!trinket.1.is.treacherous_transmitter&(!variable.special_trinket|trinket.2.cooldown.remains>20)" );
  cooldown->add_action( "use_item,slot=trinket2,if=((cooldown.eye_beam.remains<gcd.max&active_enemies>1|buff.metamorphosis.up)&(raid_event.adds.in>trinket.2.cooldown.duration-15|raid_event.adds.remains>8)|!trinket.2.has_buff.any|fight_remains<25)&!trinket.2.is.skardyns_grace&!trinket.2.is.mad_queens_mandate&!trinket.2.is.treacherous_transmitter&(!variable.special_trinket|trinket.1.cooldown.remains>20)" );
  cooldown->add_action( "the_hunt,if=debuff.essence_break.down&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>90)&(debuff.reavers_mark.up|!hero_tree.aldrachi_reaver)&buff.reavers_glaive.down&(buff.metamorphosis.remains>5|buff.metamorphosis.down)" );
  cooldown->add_action( "sigil_of_spite,if=debuff.essence_break.down&(debuff.reavers_mark.remains>=2-talent.quickened_sigils|!hero_tree.aldrachi_reaver)&cooldown.blade_dance.remains&time>15" );

  fel_barrage->add_action( "variable,name=generator_up,op=set,value=cooldown.felblade.remains<gcd.max|cooldown.sigil_of_flame.remains<gcd.max" );
  fel_barrage->add_action( "variable,name=fury_gen,op=set,value=talent.demon_blades*(1%(2.6*attack_haste)*12*((hero_tree.felscarred&buff.metamorphosis.up)*0.33+1))+buff.immolation_aura.stack*6+buff.tactical_retreat.up*10" );
  fel_barrage->add_action( "variable,name=gcd_drain,op=set,value=gcd.max*32" );
  fel_barrage->add_action( "annihilation,if=buff.inner_demon.up" );
  fel_barrage->add_action( "eye_beam,if=buff.fel_barrage.down&(active_enemies>1&raid_event.adds.up|raid_event.adds.in>40)" );
  fel_barrage->add_action( "abyssal_gaze,if=buff.fel_barrage.down&(active_enemies>1&raid_event.adds.up|raid_event.adds.in>40)" );
  fel_barrage->add_action( "essence_break,if=buff.fel_barrage.down&buff.metamorphosis.up" );
  fel_barrage->add_action( "death_sweep,if=buff.fel_barrage.down" );
  fel_barrage->add_action( "immolation_aura,if=buff.unbound_chaos.down&(active_enemies>2|buff.fel_barrage.up)&(!talent.inertia|cooldown.eye_beam.remains>recharge_time+3)" );
  fel_barrage->add_action( "glaive_tempest,if=buff.fel_barrage.down&active_enemies>1" );
  fel_barrage->add_action( "blade_dance,if=buff.fel_barrage.down" );
  fel_barrage->add_action( "fel_barrage,if=fury>100&(raid_event.adds.in>90|raid_event.adds.in<gcd.max|raid_event.adds.remains>4&active_enemies>2)" );
  fel_barrage->add_action( "fel_rush,if=buff.unbound_chaos.up&fury>20&buff.fel_barrage.up" );
  fel_barrage->add_action( "sigil_of_flame,if=fury.deficit>40&buff.fel_barrage.up&(!talent.student_of_suffering|cooldown.eye_beam.remains>30)" );
  fel_barrage->add_action( "sigil_of_doom,if=fury.deficit>40&buff.fel_barrage.up" );
  fel_barrage->add_action( "felblade,if=buff.fel_barrage.up&fury.deficit>40&action.felblade.cooldown_react" );
  fel_barrage->add_action( "death_sweep,if=fury-variable.gcd_drain-35>0&(buff.fel_barrage.remains<3|variable.generator_up|fury>80|variable.fury_gen>18)" );
  fel_barrage->add_action( "glaive_tempest,if=fury-variable.gcd_drain-30>0&(buff.fel_barrage.remains<3|variable.generator_up|fury>80|variable.fury_gen>18)" );
  fel_barrage->add_action( "blade_dance,if=fury-variable.gcd_drain-35>0&(buff.fel_barrage.remains<3|variable.generator_up|fury>80|variable.fury_gen>18)" );
  fel_barrage->add_action( "arcane_torrent,if=fury.deficit>40&buff.fel_barrage.up" );
  fel_barrage->add_action( "fel_rush,if=buff.unbound_chaos.up" );
  fel_barrage->add_action( "the_hunt,if=fury>40&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>80)" );
  fel_barrage->add_action( "annihilation,if=fury-variable.gcd_drain-40>20&(buff.fel_barrage.remains<3|variable.generator_up|fury>80|variable.fury_gen>18)" );
  fel_barrage->add_action( "chaos_strike,if=fury-variable.gcd_drain-40>20&(cooldown.fel_barrage.remains&cooldown.fel_barrage.remains<10&fury>100|buff.fel_barrage.up&(buff.fel_barrage.remains*variable.fury_gen-buff.fel_barrage.remains*32)>0)" );
  fel_barrage->add_action( "demons_bite" );

  meta->add_action( "death_sweep,if=buff.metamorphosis.remains<gcd.max|(hero_tree.felscarred&talent.chaos_theory&talent.essence_break&((cooldown.metamorphosis.up&(!buff.unbound_chaos.up|!talent.inertia))|prev_gcd.1.metamorphosis)&buff.demonsurge.stack=0)" );
  meta->add_action( "annihilation,if=buff.metamorphosis.remains<gcd.max|debuff.essence_break.remains&debuff.essence_break.remains<0.5" );
  meta->add_action( "annihilation,if=(hero_tree.felscarred&buff.demonsurge_annihilation.up&(talent.restless_hunter|!talent.chaos_theory|buff.chaos_theory.up))&(cooldown.eye_beam.remains<gcd.max*3&cooldown.blade_dance.remains|cooldown.metamorphosis.remains<gcd.max*3)" );
  meta->add_action( "fel_rush,if=buff.unbound_chaos.up&talent.inertia&cooldown.metamorphosis.up&(!hero_tree.felscarred|cooldown.eye_beam.remains)" );
  meta->add_action( "fel_rush,if=buff.unbound_chaos.up&talent.inertia&cooldown.blade_dance.remains<gcd.max*3&(!hero_tree.felscarred|cooldown.eye_beam.remains)" );
  meta->add_action( "fel_rush,if=talent.momentum&buff.momentum.remains<gcd.max*2" );
  meta->add_action( "immolation_aura,if=charges=2&active_enemies>1&debuff.essence_break.down" );
  meta->add_action( "annihilation,if=(buff.inner_demon.up)&(cooldown.eye_beam.remains<gcd.max*3&cooldown.blade_dance.remains|cooldown.metamorphosis.remains<gcd.max*3)" );
  meta->add_action( "essence_break,if=time<20&buff.thrill_of_the_fight_damage.remains>gcd.max*4&buff.metamorphosis.remains>=gcd.max*2&cooldown.metamorphosis.up&cooldown.death_sweep.remains<=gcd.max&buff.inertia.up" );
  meta->add_action( "essence_break,if=fury>20&(cooldown.metamorphosis.remains>10|cooldown.blade_dance.remains<gcd.max*2)&(buff.unbound_chaos.down|buff.inertia.up|!talent.inertia)&buff.out_of_range.remains<gcd.max&(!talent.shattered_destiny|cooldown.eye_beam.remains>4)&(!hero_tree.felscarred|active_enemies>1|cooldown.metamorphosis.remains>5&cooldown.eye_beam.remains)|fight_remains<10" );
  meta->add_action( "immolation_aura,if=cooldown.blade_dance.remains&talent.inertia&buff.metamorphosis.remains>10&hero_tree.felscarred&full_recharge_time<gcd.max*2&buff.unbound_chaos.down&debuff.essence_break.down" );
  meta->add_action( "sigil_of_doom,if=cooldown.blade_dance.remains&debuff.essence_break.down" );
  meta->add_action( "immolation_aura,if=buff.demonsurge.up&buff.demonsurge.remains<gcd.max*3&buff.demonsurge_consuming_fire.up" );
  meta->add_action( "immolation_aura,if=debuff.essence_break.down&cooldown.blade_dance.remains>gcd.max+0.5&buff.unbound_chaos.down&talent.inertia&buff.inertia.down&full_recharge_time+3<cooldown.eye_beam.remains&buff.metamorphosis.remains>5" );
  meta->add_action( "death_sweep" );
  meta->add_action( "eye_beam,if=debuff.essence_break.down&buff.inner_demon.down" );
  meta->add_action( "abyssal_gaze,if=debuff.essence_break.down&buff.inner_demon.down" );
  meta->add_action( "glaive_tempest,if=debuff.essence_break.down&(cooldown.blade_dance.remains>gcd.max*2|fury>60)&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>10)" );
  meta->add_action( "sigil_of_flame,if=active_enemies>2&debuff.essence_break.down" );
  meta->add_action( "throw_glaive,if=talent.soulscar&talent.furious_throws&active_enemies>1&debuff.essence_break.down" );
  meta->add_action( "annihilation,if=cooldown.blade_dance.remains|fury>60|soul_fragments.total>0|buff.metamorphosis.remains<5&cooldown.felblade.up" );
  meta->add_action( "sigil_of_flame,if=buff.metamorphosis.remains>5&buff.out_of_range.down" );
  meta->add_action( "felblade,if=(buff.out_of_range.down|fury.deficit>40)&action.felblade.cooldown_react" );
  meta->add_action( "sigil_of_flame,if=debuff.essence_break.down&buff.out_of_range.down" );
  meta->add_action( "immolation_aura,if=buff.out_of_range.down&recharge_time<(cooldown.eye_beam.remains<?buff.metamorphosis.remains)&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>full_recharge_time)" );
  meta->add_action( "fel_rush,if=talent.momentum" );
  meta->add_action( "annihilation" );
  meta->add_action( "throw_glaive,if=buff.unbound_chaos.down&recharge_time<cooldown.eye_beam.remains&debuff.essence_break.down&(cooldown.eye_beam.remains>8|charges_fractional>1.01)&buff.out_of_range.down&active_enemies>1" );
  meta->add_action( "fel_rush,if=buff.unbound_chaos.down&recharge_time<cooldown.eye_beam.remains&debuff.essence_break.down&(cooldown.eye_beam.remains>8|charges_fractional>1.01)&buff.out_of_range.down&active_enemies>1" );
  meta->add_action( "demons_bite" );

  opener->add_action( "potion" );
  opener->add_action( "vengeful_retreat,use_off_gcd=1,if=(prev_gcd.1.death_sweep&buff.initiative.remains<2&buff.inner_demon.down|buff.initiative.remains<0.5&debuff.initiative_tracker.up&(!talent.inertia|buff.unbound_chaos.down))&((!talent.essence_break&talent.shattered_destiny)|(talent.essence_break&!talent.shattered_destiny)|(!talent.shattered_destiny&!talent.essence_break))|prev_gcd.2.death_sweep&prev_gcd.1.annihilation" );
  opener->add_action( "vengeful_retreat,use_off_gcd=1,if=buff.metamorphosis.up&cooldown.metamorphosis.remains&cooldown.essence_break.up&cooldown.eye_beam.remains&talent.shattered_destiny&talent.essence_break&(cooldown.sigil_of_spite.remains|!hero_tree.aldrachi_reaver)" );
  opener->add_action( "annihilation,if=hero_tree.felscarred&buff.demonsurge_annihilation.up&talent.restless_hunter,line_cd=10", "actions.opener+=/sigil_of_doom,if=talent.essence_break&prev_gcd.1.metamorphosis" );
  opener->add_action( "fel_rush,if=talent.momentum&buff.momentum.remains<6|talent.inertia&buff.unbound_chaos.up&talent.a_fire_inside&(buff.inertia.down&buff.metamorphosis.up&!hero_tree.felscarred|hero_tree.felscarred&(buff.metamorphosis.down|buff.demonsurge.stack>=3))&debuff.essence_break.down" );
  opener->add_action( "fel_rush,if=talent.inertia&buff.unbound_chaos.up&!talent.a_fire_inside&buff.metamorphosis.up&(cooldown.metamorphosis.up|cooldown.eye_beam.remains|talent.essence_break&cooldown.essence_break.remains)" );
  opener->add_action( "the_hunt,if=buff.metamorphosis.up|!talent.shattered_destiny" );
  opener->add_action( "death_sweep,if=hero_tree.felscarred&talent.chaos_theory&buff.metamorphosis.up&buff.demonsurge.stack=0&!talent.restless_hunter" );
  opener->add_action( "annihilation,if=hero_tree.felscarred&buff.demonsurge_annihilation.up&!talent.essence_break,line_cd=10" );
  opener->add_action( "felblade,if=(buff.metamorphosis.down|fury<40)&(!talent.a_fire_inside|hero_tree.aldrachi_reaver)&action.felblade.cooldown_react" );
  opener->add_action( "reavers_glaive,if=debuff.reavers_mark.down&debuff.essence_break.down" );
  opener->add_action( "immolation_aura,if=(charges=2-!talent.a_fire_inside&cooldown.blade_dance.remains|hero_tree.felscarred)&(!hero_tree.felscarred&buff.unbound_chaos.down&(buff.metamorphosis.down|cooldown.metamorphosis.remains)|hero_tree.felscarred&(cooldown.metamorphosis.up|buff.inertia.remains<gcd.max*2&(!talent.essence_break|cooldown.blade_dance.remains&(buff.demonsurge_annihilation.down|cooldown.abyssal_gaze.remains))))&(talent.unbound_chaos|talent.burning_wound)&debuff.essence_break.down&buff.immolation_aura.stack<5" );
  opener->add_action( "blade_dance,if=buff.glaive_flurry.up&!talent.shattered_destiny" );
  opener->add_action( "chaos_strike,if=buff.rending_strike.up&(!talent.shattered_destiny)" );
  opener->add_action( "metamorphosis,if=buff.metamorphosis.up&cooldown.blade_dance.remains>gcd.max*2&buff.inner_demon.down&(!hero_tree.felscarred&(!talent.restless_hunter|prev_gcd.1.death_sweep)|buff.demonsurge.stack=2)&(cooldown.essence_break.remains|hero_tree.felscarred|talent.shattered_destiny|!talent.essence_break)" );
  opener->add_action( "sigil_of_spite,if=hero_tree.felscarred|debuff.reavers_mark.up&(!talent.cycle_of_hatred|cooldown.eye_beam.remains&cooldown.metamorphosis.remains)" );
  opener->add_action( "sigil_of_doom,if=buff.inner_demon.down&debuff.essence_break.down&cooldown.blade_dance.remains" );
  opener->add_action( "eye_beam,if=buff.metamorphosis.down|debuff.essence_break.down&buff.inner_demon.down&(cooldown.blade_dance.remains|talent.essence_break&cooldown.essence_break.up)" );
  opener->add_action( "abyssal_gaze,if=debuff.essence_break.down&cooldown.blade_dance.remains&buff.inner_demon.down" );
  opener->add_action( "essence_break,if=(cooldown.blade_dance.remains<gcd.max&!hero_tree.felscarred&!talent.shattered_destiny&buff.metamorphosis.up|cooldown.eye_beam.remains&cooldown.metamorphosis.remains)&(!hero_tree.felscarred|buff.demonsurge_annihilation.down)" );
  opener->add_action( "essence_break,if=talent.restless_hunter&buff.metamorphosis.up&buff.inner_demon.down&(!hero_tree.felscarred|buff.demonsurge_annihilation.down)" );
  opener->add_action( "death_sweep" );
  opener->add_action( "annihilation" );
  opener->add_action( "demons_bite" );
}
//havoc_apl_end

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
  action_priority_list_t* rg_sequence = p->get_action_priority_list( "rg_sequence" );
  action_priority_list_t* rg_sequence_filler = p->get_action_priority_list( "rg_sequence_filler" );

  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=single_target,value=spell_targets.spirit_bomb=1" );
  precombat->add_action( "variable,name=small_aoe,value=spell_targets.spirit_bomb>=2&spell_targets.spirit_bomb<=5" );
  precombat->add_action( "variable,name=big_aoe,value=spell_targets.spirit_bomb>=6" );
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

  ar->add_action( "variable,name=spb_threshold,op=setif,condition=talent.fiery_demise&dot.fiery_brand.ticking,value=(variable.single_target*5)+(variable.small_aoe*4)+(variable.big_aoe*4),value_else=(variable.single_target*5)+(variable.small_aoe*5)+(variable.big_aoe*4)" );
  ar->add_action( "variable,name=can_spb,op=setif,condition=talent.spirit_bomb,value=soul_fragments>=variable.spb_threshold,value_else=0" );
  ar->add_action( "variable,name=can_spb_soon,op=setif,condition=talent.spirit_bomb,value=soul_fragments.total>=variable.spb_threshold,value_else=0" );
  ar->add_action( "variable,name=can_spb_one_gcd,op=setif,condition=talent.spirit_bomb,value=(soul_fragments.total+variable.num_spawnable_souls)>=variable.spb_threshold,value_else=0" );
  ar->add_action( "variable,name=dont_soul_cleave,value=talent.spirit_bomb&((variable.can_spb|variable.can_spb_soon|variable.can_spb_one_gcd)|prev_gcd.1.fracture)" );
  ar->add_action( "variable,name=double_rm_expires,op=set,value=time+20,if=prev_gcd.1.fracture&debuff.reavers_mark.stack=2&debuff.reavers_mark.remains>(20-gcd.max)&!buff.rending_strike.up&!buff.glaive_flurry.up" );
  ar->add_action( "variable,name=double_rm_remains,op=setif,condition=(variable.double_rm_expires-time)>0,value=variable.double_rm_expires-time,value_else=0" );
  ar->add_action( "variable,name=double_rm_remains,op=print" );
  ar->add_action( "variable,name=rg_sequence_duration,op=set,value=action.fracture.execute_time+action.soul_cleave.execute_time+action.reavers_glaive.execute_time" );
  ar->add_action( "variable,name=rg_sequence_duration,op=add,value=gcd.max,if=!talent.keen_engagement" );
  ar->add_action( "variable,name=trigger_overflow,op=set,value=0,if=!buff.glaive_flurry.up&!buff.rending_strike.up" );
  ar->add_action( "variable,name=rg_enhance_cleave,op=setif,condition=spell_targets.spirit_bomb>4|fight_remains<8|variable.trigger_overflow,value=1,value_else=0" );
  ar->add_action( "variable,name=cooldown_sync,value=(debuff.reavers_mark.remains>gcd.max&debuff.reavers_mark.stack=2&buff.thrill_of_the_fight_damage.remains>gcd.max)|fight_remains<20" );
  ar->add_action( "potion,use_off_gcd=1,if=gcd.remains=0&(variable.cooldown_sync|(buff.rending_strike.up&buff.glaive_flurry.up))" );
  ar->add_action( "use_items,use_off_gcd=1,if=variable.cooldown_sync" );
  ar->add_action( "call_action_list,name=externals,if=variable.cooldown_sync" );
  ar->add_action( "run_action_list,name=rg_sequence,if=buff.glaive_flurry.up|buff.rending_strike.up" );
  ar->add_action( "metamorphosis,use_off_gcd=1,if=!buff.metamorphosis.up&gcd.remains=0&cooldown.the_hunt.remains>5&!(buff.rending_strike.up&buff.glaive_flurry.up)" );
  ar->add_action( "vengeful_retreat,use_off_gcd=1,if=talent.unhindered_assault&!cooldown.felblade.up&(((talent.spirit_bomb&(fury<40&(variable.can_spb|variable.can_spb_soon)))|(talent.spirit_bomb&(cooldown.sigil_of_spite.remains<gcd.max|cooldown.soul_carver.remains<gcd.max)&(cooldown.fel_devastation.remains<(gcd.max*2))&fury<50))|(fury<30&(soul_fragments<=2|cooldown.fracture.charges_fractional<1)))" );
  ar->add_action( "the_hunt,if=!buff.reavers_glaive.up&(buff.art_of_the_glaive.stack+soul_fragments.total)<20" );
  ar->add_action( "immolation_aura,if=!(buff.glaive_flurry.up|buff.rending_strike.up)" );
  ar->add_action( "sigil_of_flame,if=!(buff.glaive_flurry.up|buff.rending_strike.up)&(talent.ascending_flame|(!talent.ascending_flame&!prev_gcd.1.sigil_of_flame&(dot.sigil_of_flame.remains<(1+talent.quickened_sigils))))" );
  ar->add_action( "call_action_list,name=rg_overflow,if=buff.reavers_glaive.up&buff.thrill_of_the_fight_damage.up&buff.thrill_of_the_fight_damage.remains<variable.rg_sequence_duration&(((((1.2*(1+raw_haste_pct))*(variable.double_rm_remains-variable.rg_sequence_duration))+soul_fragments.total+buff.art_of_the_glaive.stack)>=20)|((cooldown.the_hunt.remains)<(variable.double_rm_remains-(variable.rg_sequence_duration+action.the_hunt.execute_time))))" );
  ar->add_action( "call_action_list,name=ar_execute,if=fight_remains<20" );
  ar->add_action( "soul_cleave,if=(variable.double_rm_remains<=(execute_time+variable.rg_sequence_duration))&(soul_fragments.total>=2&buff.art_of_the_glaive.stack>=(20-2))&(fury<40|!variable.can_spb)" );
  ar->add_action( "spirit_bomb,if=(variable.double_rm_remains<=(execute_time+variable.rg_sequence_duration))&(buff.art_of_the_glaive.stack+soul_fragments.total>=20)" );
  ar->add_action( "bulk_extraction,if=(variable.double_rm_remains<=(execute_time+variable.rg_sequence_duration))&(buff.art_of_the_glaive.stack+(spell_targets>?5)>=20)" );
  ar->add_action( "reavers_glaive,if=(variable.rg_enhance_cleave&fury>=5|!variable.rg_enhance_cleave&fury>=30)&buff.thrill_of_the_fight_damage.remains<variable.rg_sequence_duration&(!buff.thrill_of_the_fight_attack_speed.up|(variable.double_rm_remains<=variable.rg_sequence_duration)|variable.rg_enhance_cleave)" );
  ar->add_action( "fiery_brand,if=!talent.fiery_demise|(talent.fiery_demise&((talent.down_in_flames&charges>=max_charges)|(active_dot.fiery_brand=0)))" );
  ar->add_action( "sigil_of_spite,if=!talent.spirit_bomb|(talent.spirit_bomb&fury>=40&(variable.can_spb|variable.can_spb_soon|soul_fragments.total<=(2-talent.soul_sigils.rank)))" );
  ar->add_action( "spirit_bomb,if=variable.can_spb" );
  ar->add_action( "fel_devastation,if=talent.spirit_bomb&!variable.can_spb&(variable.can_spb_soon|soul_fragments.inactive>=1|prev_gcd.1.sigil_of_spite|prev_gcd.1.soul_carver)" );
  ar->add_action( "soul_carver,if=!talent.spirit_bomb|((soul_fragments.total+3)<=5)" );
  ar->add_action( "soul_cleave,if=fury.deficit<25" );
  ar->add_action( "fracture,if=talent.spirit_bomb&(variable.can_spb|variable.can_spb_soon|variable.can_spb_one_gcd)&fury<40&!cooldown.felblade.up&(!talent.unhindered_assault|(talent.unhindered_assault&!cooldown.vengeful_retreat.up))" );
  ar->add_action( "fel_devastation,if=!variable.single_target|buff.thrill_of_the_fight_damage.up" );
  ar->add_action( "bulk_extraction,if=spell_targets>=5" );
  ar->add_action( "felblade,if=(((talent.spirit_bomb&(fury<40&(variable.can_spb|variable.can_spb_soon)))|(talent.spirit_bomb&(cooldown.sigil_of_spite.remains<gcd.max|cooldown.soul_carver.remains<gcd.max)&(cooldown.fel_devastation.remains<(gcd.max*2))&fury<50))|(fury<30&(soul_fragments<=2|cooldown.fracture.charges_fractional<1)))" );
  ar->add_action( "soul_cleave,if=fury.deficit<=25|(!talent.spirit_bomb|!variable.dont_soul_cleave)" );
  ar->add_action( "fracture" );
  ar->add_action( "shear" );
  ar->add_action( "felblade" );
  ar->add_action( "throw_glaive" );

  ar_execute->add_action( "metamorphosis,use_off_gcd=1" );
  ar_execute->add_action( "reavers_glaive,if=fury>=30" );
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
  fel_dev->add_action( "sigil_of_spite,if=soul_fragments.total<=2&buff.demonsurge_spirit_burst.up" );
  fel_dev->add_action( "soul_carver,if=soul_fragments.total<=2&!prev_gcd.1.sigil_of_spite&buff.demonsurge_spirit_burst.up" );
  fel_dev->add_action( "fracture,if=soul_fragments.total<=2&buff.demonsurge_spirit_burst.up" );
  fel_dev->add_action( "felblade" );
  fel_dev->add_action( "fracture" );

  fel_dev_prep->add_action( "potion,use_off_gcd=1,if=prev_gcd.1.fiery_brand" );
  fel_dev_prep->add_action( "fiery_brand,if=talent.fiery_demise&((fury+variable.fel_dev_passive_fury_gen)>=115)&(variable.can_spburst|variable.can_spburst_soon|soul_fragments.total>=4)&active_dot.fiery_brand=0&(cooldown.metamorphosis.remains<(execute_time+action.fel_devastation.execute_time+(gcd.max*2)))" );
  fel_dev_prep->add_action( "fel_devastation,if=((fury+variable.fel_dev_passive_fury_gen)>=115)&(variable.can_spburst|variable.can_spburst_soon|soul_fragments.total>=4)" );
  fel_dev_prep->add_action( "sigil_of_spite,if=soul_fragments.total<=1|(!(variable.can_spburst|variable.can_spburst_soon|soul_fragments.total>=4)&action.fracture.charges_fractional<1)" );
  fel_dev_prep->add_action( "soul_carver,if=(soul_fragments.total<=1|(!(variable.can_spburst|variable.can_spburst_soon|soul_fragments.total>=4)&action.fracture.charges_fractional<1))&!prev_gcd.1.sigil_of_spite&!prev_gcd.2.sigil_of_spite" );
  fel_dev_prep->add_action( "felblade,if=!((fury+variable.fel_dev_passive_fury_gen)>=115)" );
  fel_dev_prep->add_action( "fracture,if=!(variable.can_spburst|variable.can_spburst_soon|soul_fragments.total>=4)|!((fury+variable.fel_dev_passive_fury_gen)>=115)" );
  fel_dev_prep->add_action( "felblade" );
  fel_dev_prep->add_action( "fracture" );
  fel_dev_prep->add_action( "fel_devastation" );

  fs->add_action( "variable,name=spbomb_threshold,op=setif,condition=talent.fiery_demise&dot.fiery_brand.ticking,value=(variable.single_target*5)+(variable.small_aoe*4)+(variable.big_aoe*4),value_else=(variable.single_target*5)+(variable.small_aoe*4)+(variable.big_aoe*4)" );
  fs->add_action( "variable,name=can_spbomb,op=setif,condition=talent.spirit_bomb,value=soul_fragments>=variable.spbomb_threshold,value_else=0" );
  fs->add_action( "variable,name=can_spbomb_soon,op=setif,condition=talent.spirit_bomb,value=soul_fragments.total>=variable.spbomb_threshold,value_else=0" );
  fs->add_action( "variable,name=can_spbomb_one_gcd,op=setif,condition=talent.spirit_bomb,value=(soul_fragments.total+variable.num_spawnable_souls)>=variable.spbomb_threshold,value_else=0" );
  fs->add_action( "variable,name=spburst_threshold,op=setif,condition=talent.fiery_demise&dot.fiery_brand.ticking,value=(variable.single_target*5)+(variable.small_aoe*4)+(variable.big_aoe*4),value_else=(variable.single_target*5)+(variable.small_aoe*4)+(variable.big_aoe*4)" );
  fs->add_action( "variable,name=can_spburst,op=setif,condition=talent.spirit_bomb,value=soul_fragments>=variable.spburst_threshold,value_else=0" );
  fs->add_action( "variable,name=can_spburst_soon,op=setif,condition=talent.spirit_bomb,value=soul_fragments.total>=variable.spburst_threshold,value_else=0" );
  fs->add_action( "variable,name=can_spburst_one_gcd,op=setif,condition=talent.spirit_bomb,value=(soul_fragments.total+variable.num_spawnable_souls)>=variable.spburst_threshold,value_else=0" );
  fs->add_action( "variable,name=dont_soul_cleave,op=setif,condition=buff.metamorphosis.up&buff.demonsurge_hardcast.up,value=(buff.metamorphosis.remains<(gcd.max*2)&(!((fury+variable.fel_dev_passive_fury_gen)>=115)|!(variable.can_spburst|variable.can_spburst_soon|soul_fragments.total>=4))),value_else=(cooldown.fel_devastation.remains<(gcd.max*4)&(!((fury+variable.fel_dev_passive_fury_gen)>=115)|!(variable.can_spburst|variable.can_spburst_soon|soul_fragments.total>=4)))" );
  fs->add_action( "variable,name=fiery_brand_back_before_meta,op=setif,condition=talent.down_in_flames,value=charges>=max_charges|(charges_fractional>=1&cooldown.fiery_brand.full_recharge_time<=gcd.remains+execute_time)|(charges_fractional>=1&((max_charges-(charges_fractional-1))*cooldown.fiery_brand.duration)<=cooldown.metamorphosis.remains),value_else=cooldown.fiery_brand.duration<=cooldown.metamorphosis.remains" );
  fs->add_action( "variable,name=hold_sof,op=setif,condition=talent.student_of_suffering,value=(buff.student_of_suffering.remains>(4-talent.quickened_sigils))|(!talent.ascending_flame&(dot.sigil_of_flame.remains>(4-talent.quickened_sigils)))|prev_gcd.1.sigil_of_flame|(talent.illuminated_sigils&charges=1&time<(2-talent.quickened_sigils))|cooldown.metamorphosis.up,value_else=(cooldown.metamorphosis.up&(talent.ascending_flame|(!talent.ascending_flame&cooldown.sigil_of_flame.charges_fractional<2)))|(cooldown.sigil_of_flame.max_charges>1&talent.ascending_flame&((cooldown.sigil_of_flame.max_charges-(cooldown.sigil_of_flame.charges_fractional-1))*cooldown.sigil_of_flame.duration)>cooldown.metamorphosis.remains)" );
  fs->add_action( "variable,name=crit_pct,op=set,value=(dot.sigil_of_flame.crit_pct+(talent.aura_of_pain*6))%100,if=active_dot.sigil_of_flame>0&talent.volatile_flameblood" );
  fs->add_action( "variable,name=fel_dev_sequence_time,op=set,value=2+(2*gcd.max)" );
  fs->add_action( "variable,name=fel_dev_sequence_time,op=add,value=gcd.max,if=talent.fiery_demise&cooldown.fiery_brand.up" );
  fs->add_action( "variable,name=fel_dev_sequence_time,op=add,value=gcd.max,if=cooldown.sigil_of_flame.up|cooldown.sigil_of_flame.remains<variable.fel_dev_sequence_time" );
  fs->add_action( "variable,name=fel_dev_sequence_time,op=add,value=gcd.max,if=cooldown.immolation_aura.up|cooldown.immolation_aura.remains<variable.fel_dev_sequence_time" );
  fs->add_action( "variable,name=fel_dev_passive_fury_gen,op=set,value=0" );
  fs->add_action( "variable,name=fel_dev_passive_fury_gen,op=add,value=2.5*floor((buff.student_of_suffering.remains>?variable.fel_dev_sequence_time)),if=talent.student_of_suffering.enabled&(buff.student_of_suffering.remains>1|prev_gcd.1.sigil_of_flame)" );
  fs->add_action( "variable,name=fel_dev_passive_fury_gen,op=add,value=30+(2*talent.flames_of_fury),if=(cooldown.sigil_of_flame.remains<variable.fel_dev_sequence_time)&!variable.hold_sof" );
  fs->add_action( "variable,name=fel_dev_passive_fury_gen,op=add,value=8,if=cooldown.immolation_aura.remains<variable.fel_dev_sequence_time" );
  fs->add_action( "variable,name=fel_dev_passive_fury_gen,op=add,value=2*floor((buff.immolation_aura.remains>?variable.fel_dev_sequence_time)),if=buff.immolation_aura.remains>1" );
  fs->add_action( "variable,name=fel_dev_passive_fury_gen,op=add,value=7.5*variable.crit_pct*floor((buff.immolation_aura.remains>?variable.fel_dev_sequence_time)),if=talent.volatile_flameblood&buff.immolation_aura.remains>1" );
  fs->add_action( "variable,name=fel_dev_passive_fury_gen,op=add,value=22,if=talent.darkglare_boon.enabled" );
  fs->add_action( "use_items,use_off_gcd=1,if=!buff.metamorphosis.up" );
  fs->add_action( "immolation_aura,if=!(cooldown.metamorphosis.up&prev_gcd.1.sigil_of_flame)&!((variable.small_aoe|variable.big_aoe)&(variable.can_spb|variable.can_spb_soon))" );
  fs->add_action( "sigil_of_flame,if=!variable.hold_sof" );
  fs->add_action( "fiery_brand,if=!talent.fiery_demise|talent.fiery_demise&((talent.down_in_flames&charges>=max_charges)|(active_dot.fiery_brand=0&variable.fiery_brand_back_before_meta))" );
  fs->add_action( "call_action_list,name=fs_execute,if=fight_remains<20" );
  fs->add_action( "run_action_list,name=fel_dev,if=buff.metamorphosis.up&!buff.demonsurge_hardcast.up&(buff.demonsurge_soul_sunder.up|buff.demonsurge_spirit_burst.up)" );
  fs->add_action( "run_action_list,name=metamorphosis,if=buff.metamorphosis.up&buff.demonsurge_hardcast.up" );
  fs->add_action( "run_action_list,name=fel_dev_prep,if=!buff.demonsurge_hardcast.up&(cooldown.fel_devastation.up|(cooldown.fel_devastation.remains<=(gcd.max*2)))" );
  fs->add_action( "run_action_list,name=meta_prep,if=(cooldown.metamorphosis.remains<=(gcd.max*2))&!cooldown.fel_devastation.up&!buff.demonsurge_soul_sunder.up&!buff.demonsurge_spirit_burst.up" );
  fs->add_action( "the_hunt" );
  fs->add_action( "felblade,if=((cooldown.sigil_of_spite.remains<execute_time|cooldown.soul_carver.remains<execute_time)&cooldown.fel_devastation.remains<(execute_time+gcd.max)&fury<50)" );
  fs->add_action( "soul_carver,if=(!talent.fiery_demise|talent.fiery_demise&dot.fiery_brand.ticking)&((!talent.spirit_bomb|variable.single_target)|(talent.spirit_bomb&!prev_gcd.1.sigil_of_spite&((soul_fragments.total+3<=5&fury>=40)|(soul_fragments.total=0&fury>=15))))" );
  fs->add_action( "sigil_of_spite,if=(!talent.spirit_bomb|variable.single_target)|(talent.spirit_bomb&soul_fragments<=1)|(fury>=40&(variable.can_spbomb|(buff.metamorphosis.up&variable.can_spburst)|variable.can_spbomb_soon|(buff.metamorphosis.up&variable.can_spburst_soon)))" );
  fs->add_action( "soul_sunder,if=variable.single_target&!variable.dont_soul_cleave" );
  fs->add_action( "soul_cleave,if=variable.single_target&!variable.dont_soul_cleave" );
  fs->add_action( "spirit_burst,if=variable.can_spburst" );
  fs->add_action( "spirit_bomb,if=variable.can_spbomb" );
  fs->add_action( "soul_sunder,if=fury.deficit<25&!variable.dont_soul_cleave" );
  fs->add_action( "soul_cleave,if=fury.deficit<25&!variable.dont_soul_cleave" );
  fs->add_action( "felblade,if=(fury<40&((buff.metamorphosis.up&(variable.can_spburst|variable.can_spburst_soon))|(!buff.metamorphosis.up&(variable.can_spbomb|variable.can_spbomb_soon))))|fury<30" );
  fs->add_action( "fracture,if=!prev_gcd.1.fracture&talent.spirit_bomb&((fury<40&((buff.metamorphosis.up&(variable.can_spburst|variable.can_spburst_soon))|(!buff.metamorphosis.up&(variable.can_spbomb|variable.can_spbomb_soon))))|(buff.metamorphosis.up&variable.can_spburst_one_gcd)|(!buff.metamorphosis.up&variable.can_spbomb_one_gcd))" );
  fs->add_action( "soul_sunder,if=!variable.dont_soul_cleave" );
  fs->add_action( "soul_cleave,if=!variable.dont_soul_cleave" );
  fs->add_action( "felblade,if=fury.deficit>=40" );
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
  metamorphosis->add_action( "immolation_aura" );
  metamorphosis->add_action( "sigil_of_doom,if=!talent.student_of_suffering&(talent.ascending_flame|(dot.sigil_of_flame.remains<(4-talent.quickened_sigils)&!prev_gcd.1.sigil_of_flame))" );
  metamorphosis->add_action( "fel_desolation,if=soul_fragments<=3&(soul_fragments.inactive>=2|prev_gcd.1.sigil_of_spite)" );
  metamorphosis->add_action( "sigil_of_doom,if=(talent.student_of_suffering&!prev_gcd.1.sigil_of_flame&!prev_gcd.1.sigil_of_doom&(buff.student_of_suffering.remains<(4-talent.quickened_sigils)))|(!buff.demonsurge_soul_sunder.up&!buff.demonsurge_spirit_burst.up&!buff.demonsurge_consuming_fire.up&!buff.demonsurge_fel_desolation.up&(buff.demonsurge_sigil_of_doom.up|(!buff.demonsurge_sigil_of_doom.up&charges_fractional>=1)))|buff.metamorphosis.remains<(gcd.max*3)" );
  metamorphosis->add_action( "felblade,if=((cooldown.sigil_of_spite.remains<execute_time|cooldown.soul_carver.remains<execute_time)&cooldown.fel_desolation.remains<(execute_time+gcd.max)&fury<50)" );
  metamorphosis->add_action( "sigil_of_spite,if=!talent.spirit_bomb|(fury>=40&(variable.can_spb|variable.can_spb_soon|soul_fragments.total<=(2-talent.soul_sigils.rank)))" );
  metamorphosis->add_action( "spirit_burst,if=variable.can_spburst&buff.demonsurge_spirit_burst.up" );
  metamorphosis->add_action( "soul_carver,if=(!talent.spirit_bomb|variable.single_target)|(((soul_fragments.total+3)<=5)&fury>=40&!prev_gcd.1.sigil_of_spite)" );
  metamorphosis->add_action( "sigil_of_spite,if=soul_fragments.total<=(2-talent.soul_sigils.rank)" );
  metamorphosis->add_action( "fel_desolation" );
  metamorphosis->add_action( "soul_sunder,if=buff.demonsurge_soul_sunder.up|(variable.single_target&!variable.dont_soul_cleave)" );
  metamorphosis->add_action( "spirit_burst,if=variable.can_spburst" );
  metamorphosis->add_action( "felblade,if=fury<40&(variable.can_spburst|variable.can_spburst_soon)" );
  metamorphosis->add_action( "fracture,if=variable.big_aoe&talent.spirit_bomb&(soul_fragments>=2&soul_fragments<=3)" );
  metamorphosis->add_action( "felblade,if=fury<30" );
  metamorphosis->add_action( "soul_sunder,if=!variable.dont_soul_cleave" );
  metamorphosis->add_action( "felblade" );
  metamorphosis->add_action( "fracture" );

  rg_overflow->add_action( "variable,name=trigger_overflow,op=set,value=1" );
  rg_overflow->add_action( "variable,name=rg_enhance_cleave,op=set,value=1" );
  rg_overflow->add_action( "reavers_glaive,if=(variable.rg_enhance_cleave&fury>=5|!variable.rg_enhance_cleave&fury>=30)&!buff.rending_strike.up&!buff.glaive_flurry.up" );

  rg_sequence->add_action( "call_action_list,name=rg_sequence_filler,if=(fury<30&(!variable.rg_enhance_cleave&buff.glaive_flurry.up&buff.rending_strike.up|variable.rg_enhance_cleave&!buff.rending_strike.up))|(action.fracture.charges_fractional<1&(variable.rg_enhance_cleave&buff.rending_strike.up&buff.glaive_flurry.up|!variable.rg_enhance_cleave&!buff.glaive_flurry.up))" );
  rg_sequence->add_action( "fracture,if=(variable.rg_enhance_cleave&buff.rending_strike.up&buff.glaive_flurry.up|!variable.rg_enhance_cleave&!buff.glaive_flurry.up)" );
  rg_sequence->add_action( "shear,if=(variable.rg_enhance_cleave&buff.rending_strike.up&buff.glaive_flurry.up|!variable.rg_enhance_cleave&!buff.glaive_flurry.up)" );
  rg_sequence->add_action( "soul_cleave,if=(!variable.rg_enhance_cleave&buff.glaive_flurry.up&buff.rending_strike.up|variable.rg_enhance_cleave&!buff.rending_strike.up)" );

  rg_sequence_filler->add_action( "felblade,if=fury.deficit>30" );
  rg_sequence_filler->add_action( "fracture,if=!buff.rending_strike.up" );
  rg_sequence_filler->add_action( "wait,sec=0.1,if=action.fracture.charges_fractional>=0.8&(variable.rg_enhance_cleave&buff.rending_strike.up&buff.glaive_flurry.up|!variable.rg_enhance_cleave&!buff.glaive_flurry.up)" );
  rg_sequence_filler->add_action( "sigil_of_flame" );
  rg_sequence_filler->add_action( "sigil_of_spite" );
  rg_sequence_filler->add_action( "soul_carver" );
  rg_sequence_filler->add_action( "fel_devastation" );
  rg_sequence_filler->add_action( "throw_glaive" );
}
//vengeance_apl_end

}  // namespace demon_hunter_apl
