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
  return ( ( p->true_level >= 61 )   ? "phial_of_tepid_versatility_3"
           : ( p->true_level >= 51 ) ? "spectral_flask_of_power"
           : ( p->true_level >= 40 ) ? "greater_flask_of_the_currents"
           : ( p->true_level >= 35 ) ? "greater_draenic_agility_flask"
                                     : "disabled" );
}

std::string flask_vengeance_ptr( const player_t* p )
{
  return ( ( p->true_level >= 61 )   ? "iced_phial_of_corrupting_rage_3"
           : ( p->true_level >= 51 ) ? "spectral_flask_of_power"
           : ( p->true_level >= 40 ) ? "greater_flask_of_the_currents"
           : ( p->true_level >= 35 ) ? "greater_draenic_agility_flask"
                                     : "disabled" );
}

std::string food( const player_t* p )
{
  return ( ( p->true_level >= 61 )   ? "fated_fortune_cookie"
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
  action_priority_list_t* cooldown = p->get_action_priority_list( "cooldown" );
  action_priority_list_t* meta_end = p->get_action_priority_list( "meta_end" );

  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=3min_trinket,value=trinket.1.cooldown.duration=180|trinket.2.cooldown.duration=180" );
  precombat->add_action( "variable,name=trinket_sync_slot,value=1,if=trinket.1.has_stat.any_dps&(!trinket.2.has_stat.any_dps|trinket.1.cooldown.duration>=trinket.2.cooldown.duration)" );
  precombat->add_action( "variable,name=trinket_sync_slot,value=2,if=trinket.2.has_stat.any_dps&(!trinket.1.has_stat.any_dps|trinket.2.cooldown.duration>trinket.1.cooldown.duration)" );
  precombat->add_action( "arcane_torrent" );
  precombat->add_action( "use_item,name=algethar_puzzle_box" );
  precombat->add_action( "immolation_aura" );
  precombat->add_action( "sigil_of_flame,if=!equipped.algethar_puzzle_box" );

  default_->add_action( "auto_attack,if=!buff.out_of_range.up" );
  default_->add_action( "retarget_auto_attack,line_cd=1,target_if=min:debuff.burning_wound.remains,if=talent.burning_wound&talent.demon_blades&active_dot.burning_wound<(spell_targets>?3)" );
  default_->add_action( "retarget_auto_attack,line_cd=1,target_if=min:!target.is_boss,if=talent.burning_wound&talent.demon_blades&active_dot.burning_wound=(spell_targets>?3)" );
  default_->add_action( "variable,name=blade_dance,value=talent.first_blood|talent.trail_of_ruin|talent.chaos_theory&buff.chaos_theory.down|spell_targets.blade_dance1>1" );
  default_->add_action( "variable,name=pooling_for_blade_dance,value=variable.blade_dance&fury<(75-talent.demon_blades*20)&cooldown.blade_dance.remains<gcd.max" );
  default_->add_action( "variable,name=pooling_for_eye_beam,value=talent.demonic&!talent.blind_fury&cooldown.eye_beam.remains<(gcd.max*3)&fury.deficit>30" );
  default_->add_action( "variable,name=waiting_for_momentum,value=talent.momentum&!buff.momentum.up|talent.inertia&!buff.inertia.up" );
  default_->add_action( "variable,name=holding_meta,value=(talent.demonic&talent.essence_break)&variable.3min_trinket&fight_remains>cooldown.metamorphosis.remains+30+talent.shattered_destiny*60&cooldown.metamorphosis.remains<20&cooldown.metamorphosis.remains>action.eye_beam.execute_time+gcd.max*(talent.inner_demon+2)" );
  default_->add_action( "invoke_external_buff,name=power_infusion,if=buff.metamorphosis.up" );
  default_->add_action( "immolation_aura,if=talent.ragefire&active_enemies>=3&(cooldown.blade_dance.remains|debuff.essence_break.down)" );
  default_->add_action( "disrupt" );
  default_->add_action( "immolation_aura,if=talent.a_fire_inside&talent.inertia&buff.unbound_chaos.down&full_recharge_time<gcd.max*2&debuff.essence_break.down" );
  default_->add_action( "fel_rush,if=buff.unbound_chaos.up&(action.immolation_aura.charges=2&debuff.essence_break.down|prev_gcd.1.eye_beam&buff.inertia.up&buff.inertia.remains<3)" );
  default_->add_action( "the_hunt,if=time<10&buff.potion.up&(!talent.inertia|buff.metamorphosis.up&debuff.essence_break.down)" );
  default_->add_action( "immolation_aura,if=talent.inertia&(cooldown.eye_beam.remains<gcd.max*2|buff.metamorphosis.up)&cooldown.essence_break.remains<gcd.max*3&buff.unbound_chaos.down&buff.inertia.down&debuff.essence_break.down" );
  default_->add_action( "immolation_aura,if=talent.inertia&buff.unbound_chaos.down&(full_recharge_time<cooldown.essence_break.remains|!talent.essence_break)&debuff.essence_break.down&(buff.metamorphosis.down|buff.metamorphosis.remains>6)&cooldown.blade_dance.remains&(fury<75|cooldown.blade_dance.remains<gcd.max*2)" );
  default_->add_action( "fel_rush,if=buff.unbound_chaos.up&(buff.unbound_chaos.remains<gcd.max*2|target.time_to_die<gcd.max*2)" );
  default_->add_action( "fel_rush,if=talent.inertia&buff.inertia.down&buff.unbound_chaos.up&cooldown.eye_beam.remains+3>buff.unbound_chaos.remains&(cooldown.blade_dance.remains|cooldown.essence_break.up)" );
  default_->add_action( "fel_rush,if=buff.unbound_chaos.up&talent.inertia&buff.inertia.down&(buff.metamorphosis.up|cooldown.essence_break.remains>10)" );
  default_->add_action( "call_action_list,name=cooldown" );
  default_->add_action( "call_action_list,name=meta_end,if=buff.metamorphosis.up&buff.metamorphosis.remains<gcd.max&active_enemies<3" );
  default_->add_action( "pick_up_fragment,type=demon,if=demon_soul_fragments>0&(cooldown.eye_beam.remains<6|buff.metamorphosis.remains>5)&buff.empowered_demon_soul.remains<3|fight_remains<17" );
  default_->add_action( "pick_up_fragment,mode=nearest,type=lesser,if=fury.deficit>=45&(!cooldown.eye_beam.ready|fury<30)" );
  default_->add_action( "annihilation,if=buff.inner_demon.up&cooldown.metamorphosis.remains<=gcd*3" );
  default_->add_action( "vengeful_retreat,use_off_gcd=1,if=cooldown.eye_beam.remains<0.3&cooldown.essence_break.remains<gcd.max*2&time>5&fury>=30&gcd.remains<0.1&talent.inertia" );
  default_->add_action( "vengeful_retreat,use_off_gcd=1,if=talent.initiative&talent.essence_break&time>1&(cooldown.essence_break.remains>15|cooldown.essence_break.remains<gcd.max&(!talent.demonic|buff.metamorphosis.up|cooldown.eye_beam.remains>15+(10*talent.cycle_of_hatred)))&(time<30|gcd.remains-1<0)&(!talent.initiative|buff.initiative.remains<gcd.max|time>4)" );
  default_->add_action( "vengeful_retreat,use_off_gcd=1,if=talent.initiative&talent.essence_break&time>1&(cooldown.essence_break.remains>15|cooldown.essence_break.remains<gcd.max*2&(buff.initiative.remains<gcd.max&!variable.holding_meta&cooldown.eye_beam.remains<=gcd.remains&(raid_event.adds.in>(40-talent.cycle_of_hatred*15))&fury>30|!talent.demonic|buff.metamorphosis.up|cooldown.eye_beam.remains>15+(10*talent.cycle_of_hatred)))&(buff.unbound_chaos.down|buff.inertia.up)" );
  default_->add_action( "vengeful_retreat,use_off_gcd=1,if=talent.initiative&!talent.essence_break&time>1&((!buff.initiative.up|prev_gcd.1.death_sweep&cooldown.metamorphosis.up&talent.chaotic_transformation)&talent.initiative)" );
  default_->add_action( "fel_rush,if=talent.momentum.enabled&buff.momentum.remains<gcd.max*2&cooldown.eye_beam.remains<=gcd.max&debuff.essence_break.down&cooldown.blade_dance.remains" );
  default_->add_action( "fel_rush,if=talent.inertia.enabled&!buff.inertia.up&buff.unbound_chaos.up&(buff.metamorphosis.up|cooldown.eye_beam.remains>action.immolation_aura.recharge_time&cooldown.eye_beam.remains>4)&debuff.essence_break.down&cooldown.blade_dance.remains" );
  default_->add_action( "essence_break,if=(active_enemies>desired_targets|raid_event.adds.in>40)&(buff.metamorphosis.remains>gcd.max*3|cooldown.eye_beam.remains>10)&(!talent.tactical_retreat|buff.tactical_retreat.up|time<10)&(buff.vengeful_retreat_movement.remains<gcd.max*0.5|time>0)&cooldown.blade_dance.remains<=3.1*gcd.max|fight_remains<6" );
  default_->add_action( "death_sweep,if=variable.blade_dance&(!talent.essence_break|cooldown.essence_break.remains>gcd.max*2)&buff.fel_barrage.down" );
  default_->add_action( "the_hunt,if=debuff.essence_break.down&(time<10|cooldown.metamorphosis.remains>10|!equipped.algethar_puzzle_box)&(raid_event.adds.in>90|active_enemies>3|time_to_die<10)&(debuff.essence_break.down&(!talent.furious_gaze|buff.furious_gaze.up|set_bonus.tier31_4pc)|!set_bonus.tier30_2pc)&time>10" );
  default_->add_action( "fel_barrage,if=active_enemies>desired_targets|raid_event.adds.in>30&fury.deficit<20&buff.metamorphosis.down" );
  default_->add_action( "glaive_tempest,if=(active_enemies>desired_targets|raid_event.adds.in>10)&(debuff.essence_break.down|active_enemies>1)&buff.fel_barrage.down" );
  default_->add_action( "annihilation,if=buff.inner_demon.up&cooldown.eye_beam.remains<=gcd&buff.fel_barrage.down" );
  default_->add_action( "fel_rush,if=talent.momentum.enabled&cooldown.eye_beam.remains<=gcd.max&buff.momentum.remains<5&buff.metamorphosis.down" );
  default_->add_action( "eye_beam,if=active_enemies>desired_targets|raid_event.adds.in>(40-talent.cycle_of_hatred*15)&!debuff.essence_break.up&(cooldown.metamorphosis.remains>30-talent.cycle_of_hatred*15|cooldown.metamorphosis.remains<gcd.max*2&(!talent.essence_break|cooldown.essence_break.remains<gcd.max*1.5))&(buff.metamorphosis.down|buff.metamorphosis.remains>gcd.max|!talent.restless_hunter)&(talent.cycle_of_hatred|!talent.initiative|cooldown.vengeful_retreat.remains>5|time<10)&buff.inner_demon.down|fight_remains<15" );
  default_->add_action( "blade_dance,if=variable.blade_dance&(cooldown.eye_beam.remains>5|equipped.algethar_puzzle_box&cooldown.metamorphosis.remains>(cooldown.blade_dance.duration)|!talent.demonic|(raid_event.adds.in>cooldown&raid_event.adds.in<25))&buff.fel_barrage.down|set_bonus.tier31_2pc" );
  default_->add_action( "sigil_of_flame,if=talent.any_means_necessary&debuff.essence_break.down&active_enemies>=4" );
  default_->add_action( "throw_glaive,if=talent.soulscar&(active_enemies>desired_targets|raid_event.adds.in>full_recharge_time+9)&spell_targets>=(2-talent.furious_throws)&!debuff.essence_break.up&(full_recharge_time<gcd.max*3|active_enemies>1)&!set_bonus.tier31_2pc" );
  default_->add_action( "immolation_aura,if=active_enemies>=2&fury<70&debuff.essence_break.down" );
  default_->add_action( "annihilation,if=!variable.pooling_for_blade_dance&(cooldown.essence_break.remains|!talent.essence_break)&buff.fel_barrage.down|set_bonus.tier30_2pc" );
  default_->add_action( "felblade,if=fury.deficit>=40&talent.any_means_necessary&debuff.essence_break.down|talent.any_means_necessary&debuff.essence_break.down" );
  default_->add_action( "sigil_of_flame,if=fury.deficit>=40&talent.any_means_necessary" );
  default_->add_action( "throw_glaive,if=talent.soulscar&(active_enemies>desired_targets|raid_event.adds.in>full_recharge_time+9)&spell_targets>=(2-talent.furious_throws)&!debuff.essence_break.up&!set_bonus.tier31_2pc" );
  default_->add_action( "immolation_aura,if=buff.immolation_aura.stack<buff.immolation_aura.max_stack&(!talent.ragefire|active_enemies>desired_targets|raid_event.adds.in>15)&buff.out_of_range.down&(!buff.unbound_chaos.up|!talent.unbound_chaos)&(recharge_time<cooldown.essence_break.remains|!talent.essence_break&cooldown.eye_beam.remains>recharge_time)" );
  default_->add_action( "throw_glaive,if=talent.soulscar&cooldown.throw_glaive.full_recharge_time<cooldown.blade_dance.remains&set_bonus.tier31_2pc&buff.fel_barrage.down&!variable.pooling_for_eye_beam" );
  default_->add_action( "chaos_strike,if=!variable.pooling_for_blade_dance&!variable.pooling_for_eye_beam&buff.fel_barrage.down" );
  default_->add_action( "sigil_of_flame,if=raid_event.adds.in>15&fury.deficit>=30&buff.out_of_range.down" );
  default_->add_action( "felblade,if=fury.deficit>=40" );
  default_->add_action( "fel_rush,if=!talent.momentum&talent.demon_blades&!cooldown.eye_beam.ready&(charges=2|(raid_event.movement.in>10&raid_event.adds.in>10))&(buff.unbound_chaos.down)&(recharge_time<cooldown.essence_break.remains|!talent.essence_break)" );
  default_->add_action( "demons_bite,target_if=min:debuff.burning_wound.remains,if=talent.burning_wound&debuff.burning_wound.remains<4&active_dot.burning_wound<(spell_targets>?3)" );
  default_->add_action( "fel_rush,if=!talent.momentum&!talent.demon_blades&spell_targets>1&(charges=2|(raid_event.movement.in>10&raid_event.adds.in>10))&(buff.unbound_chaos.down)" );
  default_->add_action( "sigil_of_flame,if=raid_event.adds.in>15&fury.deficit>=30&buff.out_of_range.down" );
  default_->add_action( "demons_bite" );
  default_->add_action( "fel_rush,if=talent.momentum&buff.momentum.remains<=20" );
  default_->add_action( "fel_rush,if=movement.distance>15|(buff.out_of_range.up&!talent.momentum)" );
  default_->add_action( "vengeful_retreat,if=!talent.initiative&movement.distance>15" );
  default_->add_action( "throw_glaive,if=(talent.demon_blades|buff.out_of_range.up)&!debuff.essence_break.up&buff.out_of_range.down&!set_bonus.tier31_2pc" );

  cooldown->add_action( "metamorphosis,if=!talent.demonic&((!talent.chaotic_transformation|cooldown.eye_beam.remains>20)&active_enemies>desired_targets|raid_event.adds.in>60|fight_remains<25)" );
  cooldown->add_action( "metamorphosis,if=talent.demonic&(!talent.chaotic_transformation&cooldown.eye_beam.remains|cooldown.eye_beam.remains>20&(!variable.blade_dance|prev_gcd.1.death_sweep|prev_gcd.2.death_sweep)|fight_remains<25+talent.shattered_destiny*70&cooldown.eye_beam.remains&cooldown.blade_dance.remains)&buff.inner_demon.down" );
  cooldown->add_action( "potion,if=buff.metamorphosis.remains>25|buff.metamorphosis.up&cooldown.metamorphosis.ready|fight_remains<60|time>0.1&time<10" );
  cooldown->add_action( "elysian_decree,if=(active_enemies>desired_targets|raid_event.adds.in>30)&debuff.essence_break.down" );
  cooldown->add_action( "use_item,name=manic_grieftorch,use_off_gcd=1,if=buff.vengeful_retreat_movement.down&((buff.initiative.remains>2&debuff.essence_break.down&cooldown.essence_break.remains>gcd.max&time>14|time_to_die<10|time<1&!equipped.algethar_puzzle_box|fight_remains%%120<5)&!prev_gcd.1.essence_break)" );
  cooldown->add_action( "use_item,name=algethar_puzzle_box,use_off_gcd=1,if=cooldown.metamorphosis.remains<=gcd.max*5|fight_remains%%180>10&fight_remains%%180<22|fight_remains<25" );
  cooldown->add_action( "use_item,name=irideus_fragment,use_off_gcd=1,if=cooldown.metamorphosis.remains<=gcd.max&time>2|fight_remains%%180>10&fight_remains%%180<22|fight_remains<22" );
  cooldown->add_action( "use_item,name=stormeaters_boon,use_off_gcd=1,if=cooldown.metamorphosis.remains&(!talent.momentum|buff.momentum.remains>5)&(active_enemies>1|raid_event.adds.in>140)" );
  cooldown->add_action( "use_item,name=beacon_to_the_beyond,use_off_gcd=1,if=buff.vengeful_retreat_movement.down&debuff.essence_break.down&!prev_gcd.1.essence_break&(!equipped.irideus_fragment|trinket.1.cooldown.remains>20|trinket.2.cooldown.remains>20)" );
  cooldown->add_action( "use_item,name=dragonfire_bomb_dispenser,use_off_gcd=1,if=(time_to_die<30|cooldown.vengeful_retreat.remains<5|equipped.beacon_to_the_beyond|equipped.irideus_fragment)&(trinket.1.cooldown.remains>10|trinket.2.cooldown.remains>10|trinket.1.cooldown.duration=0|trinket.2.cooldown.duration=0|equipped.elementium_pocket_anvil|equipped.screaming_black_dragonscale|equipped.mark_of_dargrul)|(trinket.1.cooldown.duration>0|trinket.2.cooldown.duration>0)&(trinket.1.cooldown.remains|trinket.2.cooldown.remains)&!equipped.elementium_pocket_anvil&time<25" );
  cooldown->add_action( "use_item,name=elementium_pocket_anvil,use_off_gcd=1,if=!prev_gcd.1.fel_rush&gcd.remains" );
  cooldown->add_action( "use_items,slots=trinket1,if=(variable.trinket_sync_slot=1&(buff.metamorphosis.up|(!talent.demonic.enabled&cooldown.metamorphosis.remains>(fight_remains>?trinket.1.cooldown.duration%2))|fight_remains<=20)|(variable.trinket_sync_slot=2&!trinket.2.cooldown.ready)|!variable.trinket_sync_slot)&(!talent.initiative|buff.initiative.up)" );
  cooldown->add_action( "use_items,slots=trinket2,if=(variable.trinket_sync_slot=2&(buff.metamorphosis.up|(!talent.demonic.enabled&cooldown.metamorphosis.remains>(fight_remains>?trinket.2.cooldown.duration%2))|fight_remains<=20)|(variable.trinket_sync_slot=1&!trinket.1.cooldown.ready)|!variable.trinket_sync_slot)&(!talent.initiative|buff.initiative.up)" );

  meta_end->add_action( "death_sweep,if=buff.fel_barrage.down" );
  meta_end->add_action( "annihilation,if=buff.fel_barrage.down" );
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
  default_->add_action( "variable,name=can_spb,op=setif,condition=variable.fd_ready,value=(variable.single_target&soul_fragments>=5)|(variable.small_aoe&soul_fragments>=4)|(variable.big_aoe&soul_fragments>=3),value_else=(variable.small_aoe&soul_fragments>=5)|(variable.big_aoe&soul_fragments>=4)" );

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
  default_->add_action( "metamorphosis,use_off_gcd=1,if=!buff.metamorphosis.up" );
  default_->add_action( "potion,use_off_gcd=1" );
  default_->add_action( "call_action_list,name=externals" );
  default_->add_action( "use_items,use_off_gcd=1" );
  default_->add_action( "call_action_list,name=fiery_demise,if=talent.fiery_brand&talent.fiery_demise&active_dot.fiery_brand>0" );
  default_->add_action( "call_action_list,name=maintenance" );
  default_->add_action( "run_action_list,name=single_target,if=variable.single_target" );
  default_->add_action( "run_action_list,name=small_aoe,if=variable.small_aoe" );
  default_->add_action( "run_action_list,name=big_aoe,if=variable.big_aoe" );

  maintenance->add_action( "fiery_brand,if=talent.fiery_brand&((active_dot.fiery_brand=0&(cooldown.sigil_of_flame.remains<=(execute_time+gcd.remains)|cooldown.soul_carver.remains<=(execute_time+gcd.remains)|cooldown.fel_devastation.remains<=(execute_time+gcd.remains)))|(talent.down_in_flames&full_recharge_time<=(execute_time+gcd.remains)))", "Maintenance & upkeep" );
  maintenance->add_action( "sigil_of_flame,if=talent.ascending_flame|active_dot.sigil_of_flame=0" );
  maintenance->add_action( "immolation_aura" );
  maintenance->add_action( "bulk_extraction,if=((5-soul_fragments)<=spell_targets)&soul_fragments<=2" );
  maintenance->add_action( "spirit_bomb,if=variable.can_spb" );
  maintenance->add_action( "felblade,if=(fury.deficit>=40&active_enemies=1)|((cooldown.fel_devastation.remains<=(execute_time+gcd.remains))&fury<50)" );
  maintenance->add_action( "fracture,if=(cooldown.fel_devastation.remains<=(execute_time+gcd.remains))&fury<50" );
  maintenance->add_action( "shear,if=(cooldown.fel_devastation.remains<=(execute_time+gcd.remains))&fury<50" );
  maintenance->add_action( "spirit_bomb,if=fury.deficit<=30&spell_targets>1&soul_fragments>=4", "Don't overcap fury" );
  maintenance->add_action( "soul_cleave,if=fury.deficit<=40" );

  fiery_demise->add_action( "immolation_aura", "Fiery demise window" );
  fiery_demise->add_action( "sigil_of_flame" );
  fiery_demise->add_action( "felblade,if=(cooldown.fel_devastation.remains<=(execute_time+gcd.remains))&fury<50" );
  fiery_demise->add_action( "fel_devastation" );
  fiery_demise->add_action( "soul_carver,if=soul_fragments.total<3" );
  fiery_demise->add_action( "the_hunt" );
  fiery_demise->add_action( "elysian_decree,line_cd=1.85,if=fury>=40" );
  fiery_demise->add_action( "spirit_bomb,if=variable.can_spb" );

  single_target->add_action( "the_hunt", "Single Target" );
  single_target->add_action( "soul_carver" );
  single_target->add_action( "fel_devastation,if=talent.collective_anguish|(talent.stoke_the_flames&talent.burning_blood)" );
  single_target->add_action( "elysian_decree" );
  single_target->add_action( "fel_devastation" );
  single_target->add_action( "soul_cleave,if=!variable.dont_cleave" );
  single_target->add_action( "fracture" );
  single_target->add_action( "shear" );
  single_target->add_action( "call_action_list,name=filler" );

  small_aoe->add_action( "the_hunt", "2-5 targets" );
  small_aoe->add_action( "fel_devastation,if=talent.collective_anguish.enabled|(talent.stoke_the_flames.enabled&talent.burning_blood.enabled)" );
  small_aoe->add_action( "elysian_decree,line_cd=1.85,if=fury>=40&(soul_fragments.total<=1|soul_fragments.total>=4)" );
  small_aoe->add_action( "fel_devastation" );
  small_aoe->add_action( "soul_carver,if=soul_fragments.total<3" );
  small_aoe->add_action( "soul_cleave,if=soul_fragments<=1&!variable.dont_cleave" );
  small_aoe->add_action( "fracture" );
  small_aoe->add_action( "shear" );
  small_aoe->add_action( "call_action_list,name=filler" );

  big_aoe->add_action( "fel_devastation,if=talent.collective_anguish|talent.stoke_the_flames", "6+ targets" );
  big_aoe->add_action( "the_hunt" );
  big_aoe->add_action( "elysian_decree,line_cd=1.85,if=fury>=40&(soul_fragments.total<=1|soul_fragments.total>=4)" );
  big_aoe->add_action( "fel_devastation" );
  big_aoe->add_action( "soul_carver,if=soul_fragments.total<3" );
  big_aoe->add_action( "spirit_bomb,if=soul_fragments>=4" );
  big_aoe->add_action( "fracture" );
  big_aoe->add_action( "shear" );
  big_aoe->add_action( "soul_cleave,if=!variable.dont_cleave" );
  big_aoe->add_action( "call_action_list,name=filler" );

  filler->add_action( "sigil_of_chains,if=talent.cycle_of_binding&talent.sigil_of_chains", "Filler" );
  filler->add_action( "sigil_of_misery,if=talent.cycle_of_binding&talent.sigil_of_misery" );
  filler->add_action( "sigil_of_silence,if=talent.cycle_of_binding&talent.sigil_of_silence" );
  filler->add_action( "felblade" );
  filler->add_action( "throw_glaive" );

  externals->add_action( "invoke_external_buff,name=symbol_of_hope", "External buffs" );
  externals->add_action( "invoke_external_buff,name=power_infusion" );
}
//vengeance_apl_end

}  // namespace demon_hunter_apl
