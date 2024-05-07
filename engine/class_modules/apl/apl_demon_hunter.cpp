#include "class_modules/apl/apl_demon_hunter.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace demon_hunter_apl
{

std::string potion( const player_t* p )
{
  return ( ( p->true_level >= 61 )   ? "elemental_potion_of_ultimate_power_3"
           : ( p->true_level >= 51 ) ? "potion_of_spectral_agility"
           : ( p->true_level >= 40 ) ? "potion_of_unbridled_fury"
           : ( p->true_level >= 35 ) ? "draenic_agility"
                                     : "disabled" );
}

std::string flask_havoc( const player_t* p )
{
  return ( ( p->true_level >= 61 )   ? "iced_phial_of_corrupting_rage_3"
           : ( p->true_level >= 51 ) ? "spectral_flask_of_power"
           : ( p->true_level >= 40 ) ? "greater_flask_of_the_currents"
           : ( p->true_level >= 35 ) ? "greater_draenic_agility_flask"
                                     : "disabled" );
}

std::string flask_vengeance( const player_t* p )
{
  return ( ( p->true_level >= 61 )   ? "iced_phial_of_corrupting_rage_3"
           : ( p->true_level >= 51 ) ? "spectral_flask_of_power"
           : ( p->true_level >= 40 ) ? "greater_flask_of_the_currents"
           : ( p->true_level >= 35 ) ? "greater_draenic_agility_flask"
                                     : "disabled" );
}

std::string food_havoc( const player_t* p )
{
  return ( ( p->true_level >= 61 )   ? "fated_fortune_cookie"
           : ( p->true_level >= 51 ) ? "feast_of_gluttonous_hedonism"
           : ( p->true_level >= 45 ) ? "famine_evaluator_and_snack_table"
           : ( p->true_level >= 40 ) ? "lavish_suramar_feast"
                                     : "disabled" );
}

std::string food_vengeance( const player_t* p )
{
  return ( ( p->true_level >= 61 )   ? "feisty_fish_sticks"
           : ( p->true_level >= 51 ) ? "feast_of_gluttonous_hedonism"
           : ( p->true_level >= 45 ) ? "famine_evaluator_and_snack_table"
           : ( p->true_level >= 40 ) ? "lavish_suramar_feast"
                                     : "disabled" );
}

std::string rune( const player_t* p )
{
  return ( ( p->true_level >= 70 )   ? "draconic"
           : ( p->true_level >= 60 ) ? "veiled"
           : ( p->true_level >= 50 ) ? "battle_scarred"
           : ( p->true_level >= 45 ) ? "defiled"
           : ( p->true_level >= 40 ) ? "hyper"
                                     : "disabled" );
}

std::string temporary_enchant_havoc( const player_t* p )
{
  return ( ( p->true_level >= 61 )   ? "main_hand:buzzing_rune_3/off_hand:buzzing_rune_3"
           : ( p->true_level >= 51 ) ? "main_hand:shaded_sharpening_stone/off_hand:shaded_sharpening_stone"
                                     : "disabled" );
}

std::string temporary_enchant_vengeance( const player_t* p )
{
  return ( ( p->true_level >= 61 )   ? "main_hand:buzzing_rune_3/off_hand:buzzing_rune_3"
           : ( p->true_level >= 51 ) ? "main_hand:shaded_sharpening_stone/off_hand:shaded_sharpening_stone"
                                     : "disabled" );
}

//havoc_apl_start
void havoc( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* meta = p->get_action_priority_list( "meta" );
  action_priority_list_t* cooldown = p->get_action_priority_list( "cooldown" );
  action_priority_list_t* opener = p->get_action_priority_list( "opener" );
  action_priority_list_t* fel_barrage = p->get_action_priority_list( "fel_barrage" );

  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=3min_trinket,value=trinket.1.cooldown.duration=180|trinket.2.cooldown.duration=180" );
  precombat->add_action( "variable,name=trinket_sync_slot,value=1,if=trinket.1.has_stat.any_dps&(!trinket.2.has_stat.any_dps|trinket.1.cooldown.duration>=trinket.2.cooldown.duration)" );
  precombat->add_action( "variable,name=trinket_sync_slot,value=2,if=trinket.2.has_stat.any_dps&(!trinket.1.has_stat.any_dps|trinket.2.cooldown.duration>trinket.1.cooldown.duration)" );
  precombat->add_action( "arcane_torrent" );
  precombat->add_action( "immolation_aura" );

  default_->add_action( "auto_attack,if=!buff.out_of_range.up" );
  default_->add_action( "retarget_auto_attack,line_cd=1,target_if=min:debuff.burning_wound.remains,if=talent.burning_wound&talent.demon_blades&active_dot.burning_wound<(spell_targets>?3)" );
  default_->add_action( "retarget_auto_attack,line_cd=1,target_if=min:!target.is_boss,if=talent.burning_wound&talent.demon_blades&active_dot.burning_wound=(spell_targets>?3)" );
  default_->add_action( "variable,name=fel_barrage,op=set,value=talent.fel_barrage&(cooldown.fel_barrage.remains<gcd.max*7&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in<gcd.max*7|raid_event.adds.in>90)&(cooldown.metamorphosis.remains|active_enemies>2)|buff.fel_barrage.up)&!(active_enemies=1&!raid_event.adds.exists)" );
  default_->add_action( "disrupt" );
  default_->add_action( "call_action_list,name=cooldown" );
  default_->add_action( "fel_rush,if=buff.unbound_chaos.up&buff.unbound_chaos.remains<gcd.max*2" );
  default_->add_action( "pick_up_fragment,mode=nearest,type=lesser,if=fury.deficit>=45&(!cooldown.eye_beam.ready|fury<30)" );
  default_->add_action( "run_action_list,name=opener,if=(cooldown.eye_beam.up|cooldown.metamorphosis.up)&time<15&(raid_event.adds.in>40)" );
  default_->add_action( "run_action_list,name=fel_barrage,if=variable.fel_barrage&raid_event.adds.up" );
  default_->add_action( "immolation_aura,if=active_enemies>2&talent.ragefire&buff.unbound_chaos.down&(!talent.fel_barrage|cooldown.fel_barrage.remains>recharge_time)&debuff.essence_break.down" );
  default_->add_action( "immolation_aura,if=active_enemies>2&talent.ragefire&raid_event.adds.up&raid_event.adds.remains<15&raid_event.adds.remains>5&debuff.essence_break.down" );
  default_->add_action( "fel_rush,if=buff.unbound_chaos.up&active_enemies>2&(!talent.inertia|cooldown.eye_beam.remains+2>buff.unbound_chaos.remains)" );
  default_->add_action( "vengeful_retreat,use_off_gcd=1,if=talent.initiative&(cooldown.eye_beam.remains>15&gcd.remains<0.3|gcd.remains<0.1&cooldown.eye_beam.remains<=gcd.remains&(cooldown.metamorphosis.remains>10|cooldown.blade_dance.remains<gcd.max*2))&time>4" );
  default_->add_action( "run_action_list,name=fel_barrage,if=variable.fel_barrage|!talent.demon_blades&talent.fel_barrage&(buff.fel_barrage.up|cooldown.fel_barrage.up)&buff.metamorphosis.down" );
  default_->add_action( "run_action_list,name=meta,if=buff.metamorphosis.up" );
  default_->add_action( "fel_rush,if=buff.unbound_chaos.up&talent.inertia&buff.inertia.down&cooldown.blade_dance.remains<4&cooldown.eye_beam.remains>5&(action.immolation_aura.charges>0|action.immolation_aura.recharge_time+2<cooldown.eye_beam.remains|cooldown.eye_beam.remains>buff.unbound_chaos.remains-2)" );
  default_->add_action( "fel_rush,if=talent.momentum&cooldown.eye_beam.remains<gcd.max*2" );
  default_->add_action( "immolation_aura,if=buff.unbound_chaos.down&full_recharge_time<gcd.max*2&(raid_event.adds.in>full_recharge_time|active_enemies>desired_targets)" );
  default_->add_action( "immolation_aura,if=immolation_aura,if=active_enemies>desired_targets&buff.unbound_chaos.down&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>full_recharge_time)" );
  default_->add_action( "immolation_aura,if=talent.inertia&buff.unbound_chaos.down&cooldown.eye_beam.remains<5&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>full_recharge_time)" );
  default_->add_action( "immolation_aura,if=talent.inertia&buff.inertia.down&buff.unbound_chaos.down&recharge_time+5<cooldown.eye_beam.remains&cooldown.blade_dance.remains&cooldown.blade_dance.remains<4&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>full_recharge_time)&charges_fractional>1.00" );
  default_->add_action( "immolation_aura,if=fight_remains<15&cooldown.blade_dance.remains" );
  default_->add_action( "eye_beam,if=!talent.essence_break&(!talent.chaotic_transformation|cooldown.metamorphosis.remains<5+3*talent.shattered_destiny|cooldown.metamorphosis.remains>15)&(active_enemies>desired_targets*2|raid_event.adds.in>30-talent.cycle_of_hatred.rank*13)" );
  default_->add_action( "eye_beam,if=talent.essence_break&(cooldown.essence_break.remains<gcd.max*2+5*talent.shattered_destiny|talent.shattered_destiny&cooldown.essence_break.remains>10)&(cooldown.blade_dance.remains<7|raid_event.adds.up)&(!talent.initiative|cooldown.vengeful_retreat.remains>10|raid_event.adds.up)&(active_enemies+3>=desired_targets+raid_event.adds.count|raid_event.adds.in>30-talent.cycle_of_hatred.rank*6)&(!talent.inertia|buff.unbound_chaos.up|action.immolation_aura.charges=0&action.immolation_aura.recharge_time>5)&(!raid_event.adds.up|raid_event.adds.remains>8)|fight_remains<10" );
  default_->add_action( "blade_dance,if=cooldown.eye_beam.remains>gcd.max|cooldown.eye_beam.up" );
  default_->add_action( "glaive_tempest,if=active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>10" );
  default_->add_action( "sigil_of_flame,if=active_enemies>3" );
  default_->add_action( "chaos_strike,if=debuff.essence_break.up" );
  default_->add_action( "felblade" );
  default_->add_action( "throw_glaive,if=full_recharge_time<=cooldown.blade_dance.remains&cooldown.metamorphosis.remains>5&talent.soulscar&set_bonus.tier31_2pc" );
  default_->add_action( "throw_glaive,if=!set_bonus.tier31_2pc&(active_enemies>1|talent.soulscar)" );
  default_->add_action( "chaos_strike,if=cooldown.eye_beam.remains>gcd.max*2|fury>80" );
  default_->add_action( "immolation_aura,if=!talent.inertia&(raid_event.adds.in>full_recharge_time|active_enemies>desired_targets&active_enemies>2)" );
  default_->add_action( "sigil_of_flame,if=buff.out_of_range.down&debuff.essence_break.down&(!talent.fel_barrage|cooldown.fel_barrage.remains>25|(active_enemies=1&!raid_event.adds.exists))" );
  default_->add_action( "demons_bite" );
  default_->add_action( "fel_rush,if=buff.unbound_chaos.down&recharge_time<cooldown.eye_beam.remains&debuff.essence_break.down&(cooldown.eye_beam.remains>8|charges_fractional>1.01)" );
  default_->add_action( "arcane_torrent,if=buff.out_of_range.down&debuff.essence_break.down&fury<100" );

  meta->add_action( "death_sweep,if=buff.metamorphosis.remains<gcd.max" );
  meta->add_action( "annihilation,if=buff.metamorphosis.remains<gcd.max" );
  meta->add_action( "fel_rush,if=buff.unbound_chaos.up&talent.inertia" );
  meta->add_action( "fel_rush,if=talent.momentum&buff.momentum.remains<gcd.max*2" );
  meta->add_action( "annihilation,if=buff.inner_demon.up&(cooldown.eye_beam.remains<gcd.max*3&cooldown.blade_dance.remains|cooldown.metamorphosis.remains<gcd.max*3)" );
  meta->add_action( "essence_break,if=fury>20&(cooldown.metamorphosis.remains>10|cooldown.blade_dance.remains<gcd.max*2)&(buff.unbound_chaos.down|buff.inertia.up|!talent.inertia)|fight_remains<10" );
  meta->add_action( "immolation_aura,if=debuff.essence_break.down&cooldown.blade_dance.remains>gcd.max+0.5&buff.unbound_chaos.down&talent.inertia&buff.inertia.down&full_recharge_time+3<cooldown.eye_beam.remains&buff.metamorphosis.remains>5" );
  meta->add_action( "death_sweep" );
  meta->add_action( "eye_beam,if=debuff.essence_break.down&buff.inner_demon.down" );
  meta->add_action( "glaive_tempest,if=debuff.essence_break.down&(cooldown.blade_dance.remains>gcd.max*2|fury>60)&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>10)" );
  meta->add_action( "sigil_of_flame,if=active_enemies>2" );
  meta->add_action( "annihilation,if=cooldown.blade_dance.remains>gcd.max*2|fury>60|buff.metamorphosis.remains<5&cooldown.felblade.up" );
  meta->add_action( "sigil_of_flame,if=buff.metamorphosis.remains>5" );
  meta->add_action( "felblade" );
  meta->add_action( "sigil_of_flame,if=debuff.essence_break.down" );
  meta->add_action( "immolation_aura,if=buff.out_of_range.down&recharge_time<(cooldown.eye_beam.remains<?buff.metamorphosis.remains)&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>full_recharge_time)" );
  meta->add_action( "fel_rush,if=talent.momentum" );
  meta->add_action( "fel_rush,if=buff.unbound_chaos.down&recharge_time<cooldown.eye_beam.remains&debuff.essence_break.down&(cooldown.eye_beam.remains>8|charges_fractional>1.01)&buff.out_of_range.down" );
  meta->add_action( "demons_bite" );

  cooldown->add_action( "metamorphosis,if=(!talent.initiative|cooldown.vengeful_retreat.remains)&((!talent.demonic|prev_gcd.1.death_sweep|prev_gcd.2.death_sweep|prev_gcd.3.death_sweep)&cooldown.eye_beam.remains&(!talent.essence_break|debuff.essence_break.up)&buff.fel_barrage.down&(raid_event.adds.in>40|(raid_event.adds.remains>8|!talent.fel_barrage)&active_enemies>2)|!talent.chaotic_transformation|fight_remains<30)" );
  cooldown->add_action( "potion,if=fight_remains<35|buff.metamorphosis.up" );
  cooldown->add_action( "invoke_external_buff,name=power_infusion,if=buff.metamorphosis.up|fight_remains<=20" );
  cooldown->add_action( "use_item,slot=trinket1,use_off_gcd=1,if=((cooldown.eye_beam.remains<gcd.max&active_enemies>1|buff.metamorphosis.up)&(raid_event.adds.in>trinket.1.cooldown.duration-15|raid_event.adds.remains>8)|!trinket.1.has_buff.any|fight_remains<25)&(!equipped.witherbarks_branch|trinket.2.cooldown.remains>20)&time>0" );
  cooldown->add_action( "use_item,slot=trinket2,use_off_gcd=1,if=((cooldown.eye_beam.remains<gcd.max&active_enemies>1|buff.metamorphosis.up)&(raid_event.adds.in>trinket.2.cooldown.duration-15|raid_event.adds.remains>8)|!trinket.2.has_buff.any|fight_remains<25)&(!equipped.witherbarks_branch|trinket.1.cooldown.remains>20)&time>0" );
  cooldown->add_action( "use_item,name=witherbarks_branch,if=(talent.essence_break&cooldown.essence_break.remains<gcd.max|!talent.essence_break)&(active_enemies+3>=desired_targets+raid_event.adds.count|raid_event.adds.in>105)|fight_remains<25" );
  cooldown->add_action( "the_hunt,if=debuff.essence_break.down&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>(1+!set_bonus.tier31_2pc)*45)&time>5" );
  cooldown->add_action( "elysian_decree,if=debuff.essence_break.down" );

  opener->add_action( "use_items" );
  opener->add_action( "vengeful_retreat,if=prev_gcd.1.death_sweep" );
  opener->add_action( "metamorphosis,if=prev_gcd.1.death_sweep|(!talent.chaotic_transformation)&(!talent.initiative|cooldown.vengeful_retreat.remains>2)|!talent.demonic" );
  opener->add_action( "felblade,if=debuff.essence_break.down,line_cd=60" );
  opener->add_action( "potion" );
  opener->add_action( "immolation_aura,if=charges=2&buff.unbound_chaos.down&(buff.inertia.down|active_enemies>2)" );
  opener->add_action( "annihilation,if=buff.inner_demon.up&(!talent.chaotic_transformation|cooldown.metamorphosis.up)" );
  opener->add_action( "eye_beam,if=debuff.essence_break.down&buff.inner_demon.down&(!buff.metamorphosis.up|cooldown.blade_dance.remains)" );
  opener->add_action( "fel_rush,if=talent.inertia&(buff.inertia.down|active_enemies>2)&buff.unbound_chaos.up" );
  opener->add_action( "the_hunt,if=active_enemies>desired_targets|raid_event.adds.in>40+50*!set_bonus.tier31_2pc" );
  opener->add_action( "essence_break" );
  opener->add_action( "death_sweep" );
  opener->add_action( "annihilation" );
  opener->add_action( "demons_bite" );

  fel_barrage->add_action( "variable,name=generator_up,op=set,value=cooldown.felblade.remains<gcd.max|cooldown.sigil_of_flame.remains<gcd.max" );
  fel_barrage->add_action( "variable,name=fury_gen,op=set,value=1%(2.6*attack_haste)*12+buff.immolation_aura.stack*6+buff.tactical_retreat.up*10" );
  fel_barrage->add_action( "variable,name=gcd_drain,op=set,value=gcd.max*32" );
  fel_barrage->add_action( "annihilation,if=buff.inner_demon.up" );
  fel_barrage->add_action( "eye_beam,if=buff.fel_barrage.down&(active_enemies>1&raid_event.adds.up|raid_event.adds.in>40)" );
  fel_barrage->add_action( "essence_break,if=buff.fel_barrage.down&buff.metamorphosis.up" );
  fel_barrage->add_action( "death_sweep,if=buff.fel_barrage.down" );
  fel_barrage->add_action( "immolation_aura,if=buff.unbound_chaos.down&(active_enemies>2|buff.fel_barrage.up)" );
  fel_barrage->add_action( "glaive_tempest,if=buff.fel_barrage.down&active_enemies>1" );
  fel_barrage->add_action( "blade_dance,if=buff.fel_barrage.down" );
  fel_barrage->add_action( "fel_barrage,if=fury>100&(raid_event.adds.in>90|raid_event.adds.in<gcd.max|raid_event.adds.remains>4&active_enemies>2)" );
  fel_barrage->add_action( "fel_rush,if=buff.unbound_chaos.up&fury>20&buff.fel_barrage.up" );
  fel_barrage->add_action( "sigil_of_flame,if=fury.deficit>40&buff.fel_barrage.up" );
  fel_barrage->add_action( "felblade,if=buff.fel_barrage.up&fury.deficit>40" );
  fel_barrage->add_action( "death_sweep,if=fury-variable.gcd_drain-35>0&(buff.fel_barrage.remains<3|variable.generator_up|fury>80|variable.fury_gen>18)" );
  fel_barrage->add_action( "glaive_tempest,if=fury-variable.gcd_drain-30>0&(buff.fel_barrage.remains<3|variable.generator_up|fury>80|variable.fury_gen>18)" );
  fel_barrage->add_action( "blade_dance,if=fury-variable.gcd_drain-35>0&(buff.fel_barrage.remains<3|variable.generator_up|fury>80|variable.fury_gen>18)" );
  fel_barrage->add_action( "arcane_torrent,if=fury.deficit>40&buff.fel_barrage.up" );
  fel_barrage->add_action( "fel_rush,if=buff.unbound_chaos.up" );
  fel_barrage->add_action( "the_hunt,if=fury>40&(active_enemies>=desired_targets+raid_event.adds.count|raid_event.adds.in>(1+set_bonus.tier31_2pc)*40)" );
  fel_barrage->add_action( "demons_bite" );
}
//havoc_apl_end

//vengeance_apl_start
void vengeance( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* maintenance = p->get_action_priority_list( "maintenance" );
  action_priority_list_t* fiery_demise = p->get_action_priority_list( "fiery_demise" );
  action_priority_list_t* single_target = p->get_action_priority_list( "single_target" );
  action_priority_list_t* small_aoe = p->get_action_priority_list( "small_aoe" );
  action_priority_list_t* big_aoe = p->get_action_priority_list( "big_aoe" );
  action_priority_list_t* filler = p->get_action_priority_list( "filler" );
  action_priority_list_t* externals = p->get_action_priority_list( "externals" );

  default_->add_action( "variable,name=fd_ready,value=talent.fiery_brand&talent.fiery_demise&active_dot.fiery_brand>0", "Check if fiery demise is active and spread" );
  default_->add_action( "variable,name=dont_cleave,value=(cooldown.fel_devastation.remains<=(action.soul_cleave.execute_time+gcd.remains))&fury<80", "Don't spend fury when fel dev soon to maximize fel dev uptime" );
  default_->add_action( "variable,name=single_target,value=spell_targets.spirit_bomb=1", "When to use Spirit Bomb with Focused Cleave" );
  default_->add_action( "variable,name=small_aoe,value=spell_targets.spirit_bomb>=2&spell_targets.spirit_bomb<=5" );
  default_->add_action( "variable,name=big_aoe,value=spell_targets.spirit_bomb>=6" );
  default_->add_action( "variable,name=can_spb,op=setif,condition=variable.fd_ready,value=(variable.single_target&soul_fragments>=5)|(variable.small_aoe&soul_fragments>=4)|(variable.big_aoe&soul_fragments>=3),value_else=(variable.small_aoe&soul_fragments>=4)|(variable.big_aoe&soul_fragments>=3)" );

  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "sigil_of_flame" );
  precombat->add_action( "immolation_aura" );

  default_->add_action( "auto_attack" );
  default_->add_action( "disrupt,if=target.debuff.casting.react" );
  default_->add_action( "infernal_strike,use_off_gcd=1" );
  default_->add_action( "demon_spikes,use_off_gcd=1,if=!buff.demon_spikes.up&!cooldown.pause_action.remains" );
  default_->add_action( "metamorphosis,use_off_gcd=1,if=!buff.metamorphosis.up&cooldown.fel_devastation.remains>12" );
  default_->add_action( "potion,use_off_gcd=1" );
  default_->add_action( "call_action_list,name=externals" );
  default_->add_action( "use_items,use_off_gcd=1" );
  default_->add_action( "call_action_list,name=fiery_demise,if=talent.fiery_brand&talent.fiery_demise&active_dot.fiery_brand>0" );
  default_->add_action( "call_action_list,name=maintenance" );
  default_->add_action( "run_action_list,name=single_target,if=variable.single_target" );
  default_->add_action( "run_action_list,name=small_aoe,if=variable.small_aoe" );
  default_->add_action( "run_action_list,name=big_aoe,if=variable.big_aoe" );

  maintenance->add_action( "fiery_brand,if=talent.fiery_brand&((active_dot.fiery_brand=0&(cooldown.sigil_of_flame.remains<=(execute_time+gcd.remains)|cooldown.soul_carver.remains<=(execute_time+gcd.remains)|cooldown.fel_devastation.remains<=(execute_time+gcd.remains)))|(talent.down_in_flames&full_recharge_time<=(execute_time+gcd.remains)))", "Maintenance & upkeep" );
  maintenance->add_action( "sigil_of_flame,if=talent.ascending_flame|(active_dot.sigil_of_flame=0&!in_flight)" );
  maintenance->add_action( "immolation_aura" );
  maintenance->add_action( "bulk_extraction,if=((5-soul_fragments)<=spell_targets)&soul_fragments<=2" );
  maintenance->add_action( "spirit_bomb,if=variable.can_spb" );
  maintenance->add_action( "felblade,if=((!talent.spirit_bomb|active_enemies=1)&fury.deficit>=40)|((cooldown.fel_devastation.remains<=(execute_time+gcd.remains))&fury<50)" );
  maintenance->add_action( "fracture,if=(cooldown.fel_devastation.remains<=(execute_time+gcd.remains))&fury<50" );
  maintenance->add_action( "shear,if=(cooldown.fel_devastation.remains<=(execute_time+gcd.remains))&fury<50" );
  maintenance->add_action( "spirit_bomb,if=fury.deficit<=30&spell_targets>1&soul_fragments>=4", "Don't overcap fury" );
  maintenance->add_action( "soul_cleave,if=fury.deficit<=40" );

  fiery_demise->add_action( "immolation_aura", "Fiery demise window" );
  fiery_demise->add_action( "sigil_of_flame,if=talent.ascending_flame|active_dot.sigil_of_flame=0" );
  fiery_demise->add_action( "felblade,if=(!talent.spirit_bomb|(cooldown.fel_devastation.remains<=(execute_time+gcd.remains)))&fury<50" );
  fiery_demise->add_action( "fel_devastation" );
  fiery_demise->add_action( "soul_carver,if=soul_fragments.total<3" );
  fiery_demise->add_action( "the_hunt" );
  fiery_demise->add_action( "elysian_decree,if=fury>=40&!prev_gcd.1.elysian_decree" );
  fiery_demise->add_action( "spirit_bomb,if=variable.can_spb" );

  single_target->add_action( "the_hunt", "Single Target" );
  single_target->add_action( "soul_carver" );
  single_target->add_action( "fel_devastation,if=talent.collective_anguish|(talent.stoke_the_flames&talent.burning_blood)" );
  single_target->add_action( "elysian_decree,if=!prev_gcd.1.elysian_decree" );
  single_target->add_action( "fel_devastation" );
  single_target->add_action( "soul_cleave,if=!variable.dont_cleave" );
  single_target->add_action( "fracture" );
  single_target->add_action( "call_action_list,name=filler" );

  small_aoe->add_action( "the_hunt", "2-5 targets" );
  small_aoe->add_action( "fel_devastation,if=talent.collective_anguish.enabled|(talent.stoke_the_flames.enabled&talent.burning_blood.enabled)" );
  small_aoe->add_action( "elysian_decree,if=fury>=40&(soul_fragments.total<=1|soul_fragments.total>=4)&!prev_gcd.1.elysian_decree" );
  small_aoe->add_action( "fel_devastation" );
  small_aoe->add_action( "soul_carver,if=soul_fragments.total<3" );
  small_aoe->add_action( "soul_cleave,if=(soul_fragments<=1|!talent.spirit_bomb)&!variable.dont_cleave" );
  small_aoe->add_action( "fracture" );
  small_aoe->add_action( "call_action_list,name=filler" );

  big_aoe->add_action( "fel_devastation,if=talent.collective_anguish|talent.stoke_the_flames", "6+ targets" );
  big_aoe->add_action( "the_hunt" );
  big_aoe->add_action( "elysian_decree,if=fury>=40&(soul_fragments.total<=1|soul_fragments.total>=4)&!prev_gcd.1.elysian_decree" );
  big_aoe->add_action( "fel_devastation" );
  big_aoe->add_action( "soul_carver,if=soul_fragments.total<3" );
  big_aoe->add_action( "spirit_bomb,if=soul_fragments>=4" );
  big_aoe->add_action( "soul_cleave,if=!talent.spirit_bomb&!variable.dont_cleave" );
  big_aoe->add_action( "fracture" );
  big_aoe->add_action( "soul_cleave,if=!variable.dont_cleave" );
  big_aoe->add_action( "call_action_list,name=filler" );

  filler->add_action( "sigil_of_chains,if=talent.cycle_of_binding&talent.sigil_of_chains", "Filler" );
  filler->add_action( "sigil_of_misery,if=talent.cycle_of_binding&talent.sigil_of_misery" );
  filler->add_action( "sigil_of_silence,if=talent.cycle_of_binding&talent.sigil_of_silence" );
  filler->add_action( "felblade" );
  filler->add_action( "shear" );
  filler->add_action( "throw_glaive" );

  externals->add_action( "invoke_external_buff,name=symbol_of_hope", "External buffs" );
  externals->add_action( "invoke_external_buff,name=power_infusion" );
}
//vengeance_apl_end

}  // namespace demon_hunter_apl
