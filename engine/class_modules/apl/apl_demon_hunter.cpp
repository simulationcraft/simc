#include "class_modules/apl/apl_demon_hunter.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace demon_hunter_apl {

std::string potion( const player_t* p )
{
  return ( ( p->true_level >= 61 ) ? "elemental_potion_of_ultimate_power_3" :
           ( p->true_level >= 51 ) ? "potion_of_spectral_agility" :
           ( p->true_level >= 40 ) ? "potion_of_unbridled_fury" :
           ( p->true_level >= 35 ) ? "draenic_agility" :
           "disabled" );
}

std::string flask_havoc( const player_t* p )
{
  return ( ( p->true_level >= 61 ) ? "iced_phial_of_corrupting_rage_3" :
           ( p->true_level >= 51 ) ? "spectral_flask_of_power" :
           ( p->true_level >= 40 ) ? "greater_flask_of_the_currents" :
           ( p->true_level >= 35 ) ? "greater_draenic_agility_flask" :
           "disabled" );
}

std::string flask_vengeance( const player_t* p )
{
  return ( ( p->true_level >= 61 ) ? "phial_of_tepid_versatility_3" :
           ( p->true_level >= 51 ) ? "spectral_flask_of_power" :
           ( p->true_level >= 40 ) ? "greater_flask_of_the_currents" :
           ( p->true_level >= 35 ) ? "greater_draenic_agility_flask" :
                                   "disabled" );
}

std::string food( const player_t* p )
{
  return ( ( p->true_level >= 61 ) ? "fated_fortune_cookie" :
           ( p->true_level >= 51 ) ? "feast_of_gluttonous_hedonism" :
           ( p->true_level >= 45 ) ? "famine_evaluator_and_snack_table" :
           ( p->true_level >= 40 ) ? "lavish_suramar_feast" :
           "disabled" );
}

std::string rune( const player_t* p )
{
  return ( ( p->true_level >= 70 ) ? "draconic" :
           ( p->true_level >= 60 ) ? "veiled" :
           ( p->true_level >= 50 ) ? "battle_scarred" :
           ( p->true_level >= 45 ) ? "defiled" :
           ( p->true_level >= 40 ) ? "hyper" :
           "disabled" );
}

std::string temporary_enchant_havoc( const player_t* p )
{
  return ( ( p->true_level >= 61 )   ? "main_hand:buzzing_rune_3/off_hand:buzzing_rune_3"
           : ( p->true_level >= 51 ) ? "main_hand:shaded_sharpening_stone/off_hand:shaded_sharpening_stone"
                                     : "disabled" );
}

std::string temporary_enchant_vengeance( const player_t* p )
{
  return ( ( p->true_level >= 61 )   ? "main_hand:howling_rune_3/off_hand:howling_rune_3"
           : ( p->true_level >= 51 ) ? "main_hand:shaded_sharpening_stone/off_hand:shaded_sharpening_stone"
                                     : "disabled" );
}

//havoc_apl_start
void havoc( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* meta_end = p->get_action_priority_list( "meta_end" );
  action_priority_list_t* cooldown = p->get_action_priority_list( "cooldown" );

  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "variable,name=3min_trinket,value=trinket.1.cooldown.duration=180|trinket.2.cooldown.duration=180" );
  precombat->add_action( "variable,name=trinket_sync_slot,value=1,if=trinket.1.has_stat.any_dps&(!trinket.2.has_stat.any_dps|trinket.1.cooldown.duration>=trinket.2.cooldown.duration)" );
  precombat->add_action( "variable,name=trinket_sync_slot,value=2,if=trinket.2.has_stat.any_dps&(!trinket.1.has_stat.any_dps|trinket.2.cooldown.duration>trinket.1.cooldown.duration)" );
  precombat->add_action( "arcane_torrent" );
  precombat->add_action( "use_item,name=algethar_puzzle_box" );
  precombat->add_action( "sigil_of_flame,if=!equipped.algethar_puzzle_box" );
  precombat->add_action( "immolation_aura" );

  default_->add_action( "auto_attack" );
  default_->add_action( "retarget_auto_attack,line_cd=1,target_if=min:debuff.burning_wound.remains,if=talent.burning_wound&talent.demon_blades&active_dot.burning_wound<(spell_targets>?3)" );
  default_->add_action( "retarget_auto_attack,line_cd=1,target_if=min:!target.is_boss,if=talent.burning_wound&talent.demon_blades&active_dot.burning_wound=(spell_targets>?3)" );
  default_->add_action( "variable,name=blade_dance,value=talent.first_blood|talent.trail_of_ruin|talent.chaos_theory&buff.chaos_theory.down|spell_targets.blade_dance1>1", "Blade Dance with First Blood, Trail of Ruin, Chaos Theory, or at 2+ targets" );
  default_->add_action( "variable,name=pooling_for_blade_dance,value=variable.blade_dance&fury<(75-talent.demon_blades*20)&cooldown.blade_dance.remains<gcd.max" );
  default_->add_action( "variable,name=pooling_for_eye_beam,value=talent.demonic&!talent.blind_fury&cooldown.eye_beam.remains<(gcd.max*2)&fury<60" );
  default_->add_action( "variable,name=waiting_for_momentum,value=talent.momentum&!buff.momentum.up" );
  default_->add_action( "variable,name=holding_meta,value=(talent.demonic&talent.essence_break)&variable.3min_trinket&fight_remains>cooldown.metamorphosis.remains+30+talent.shattered_destiny*60&cooldown.metamorphosis.remains<20&cooldown.metamorphosis.remains>action.eye_beam.execute_time+gcd.max*(talent.inner_demon+2)" );
  default_->add_action( "invoke_external_buff,name=power_infusion" );
  default_->add_action( "immolation_aura,if=talent.ragefire&active_enemies>=3&(cooldown.blade_dance.remains|debuff.essence_break.down)" );
  default_->add_action( "throw_glaive,if=talent.serrated_glaive&(buff.metamorphosis.remains>gcd.max*6&(debuff.serrated_glaive.down|debuff.serrated_glaive.remains<cooldown.essence_break.remains+5&cooldown.essence_break.remains<gcd.max*2)&(cooldown.blade_dance.remains|cooldown.essence_break.remains<gcd.max*2)|time<0.5)&debuff.essence_break.down&target.time_to_die>gcd.max*8" );
  default_->add_action( "throw_glaive,if=talent.serrated_glaive&cooldown.eye_beam.remains<gcd.max*2&debuff.serrated_glaive.remains<(2+buff.metamorphosis.down*6)&(cooldown.blade_dance.remains|buff.metamorphosis.down)&debuff.essence_break.down&target.time_to_die>gcd.max*8" );
  default_->add_action( "disrupt" );
  default_->add_action( "fel_rush,if=buff.unbound_chaos.up&(buff.unbound_chaos.remains<gcd.max*2|target.time_to_die<gcd.max*2)" );
  default_->add_action( "call_action_list,name=cooldown" );
  default_->add_action( "call_action_list,name=meta_end,if=buff.metamorphosis.up&buff.metamorphosis.remains<gcd.max&active_enemies<3" );
  default_->add_action( "pick_up_fragment,type=demon,if=demon_soul_fragments>0&(cooldown.eye_beam.remains<6|buff.metamorphosis.remains>5)&buff.empowered_demon_soul.remains<3|fight_remains<17" );
  default_->add_action( "pick_up_fragment,mode=nearest,type=lesser,if=talent.demonic_appetite&fury.deficit>=45&(!cooldown.eye_beam.ready|fury<30)" );
  default_->add_action( "annihilation,if=buff.inner_demon.up&cooldown.metamorphosis.remains<=gcd*3" );
  default_->add_action( "vengeful_retreat,use_off_gcd=1,if=talent.initiative&talent.essence_break&time>1&(cooldown.essence_break.remains>15|cooldown.essence_break.remains<gcd.max&(!talent.demonic|buff.metamorphosis.up|cooldown.eye_beam.remains>15+(10*talent.cycle_of_hatred)))&(time<30|gcd.remains-1<0)&!talent.any_means_necessary&(!talent.initiative|buff.initiative.remains<gcd.max|time>4)" );
  default_->add_action( "vengeful_retreat,use_off_gcd=1,if=talent.initiative&talent.essence_break&time>1&(cooldown.essence_break.remains>15|cooldown.essence_break.remains<gcd.max*2&(buff.initiative.remains<gcd.max&!variable.holding_meta&cooldown.eye_beam.remains=gcd.remains&(raid_event.adds.in>(40-talent.cycle_of_hatred*15))&fury>30|!talent.demonic|buff.metamorphosis.up|cooldown.eye_beam.remains>15+(10*talent.cycle_of_hatred)))&talent.any_means_necessary" );
  default_->add_action( "vengeful_retreat,use_off_gcd=1,if=talent.initiative&!talent.essence_break&time>1&!buff.momentum.up" );
  default_->add_action( "fel_rush,if=talent.momentum.enabled&buff.momentum.remains<gcd.max*2&(charges_fractional>1.8|cooldown.eye_beam.remains<3)&debuff.essence_break.down&cooldown.blade_dance.remains" );
  default_->add_action( "essence_break,if=(active_enemies>desired_targets|raid_event.adds.in>40)&!variable.waiting_for_momentum&(buff.metamorphosis.remains>gcd.max*3|cooldown.eye_beam.remains>10)&(!talent.tactical_retreat|buff.tactical_retreat.up|time<10)&buff.vengeful_retreat_movement.remains<gcd.max*0.5&cooldown.blade_dance.remains<=3.1*gcd.max|fight_remains<6" );
  default_->add_action( "death_sweep,if=variable.blade_dance&(!talent.essence_break|cooldown.essence_break.remains>gcd.max*2)" );
  default_->add_action( "fel_barrage,if=active_enemies>desired_targets|raid_event.adds.in>30" );
  default_->add_action( "glaive_tempest,if=(active_enemies>desired_targets|raid_event.adds.in>10)&(debuff.essence_break.down|active_enemies>1)" );
  default_->add_action( "annihilation,if=buff.inner_demon.up&cooldown.eye_beam.remains<=gcd" );
  default_->add_action( "fel_rush,if=talent.momentum.enabled&cooldown.eye_beam.remains<gcd.max*3&buff.momentum.remains<5&buff.metamorphosis.down" );
  default_->add_action( "the_hunt,if=debuff.essence_break.down&(time<10|cooldown.metamorphosis.remains>10|!equipped.algethar_puzzle_box)&(raid_event.adds.in>90|active_enemies>3|time_to_die<10)&(time>6&debuff.essence_break.down&(!talent.furious_gaze|buff.furious_gaze.up)|!set_bonus.tier30_2pc)" );
  default_->add_action( "eye_beam,if=active_enemies>desired_targets|raid_event.adds.in>(40-talent.cycle_of_hatred*15)&!debuff.essence_break.up&(cooldown.metamorphosis.remains>40-talent.cycle_of_hatred*15|!variable.holding_meta)&(buff.metamorphosis.down|buff.metamorphosis.remains>gcd.max|!talent.restless_hunter)|fight_remains<15" );
  default_->add_action( "blade_dance,if=variable.blade_dance&(cooldown.eye_beam.remains>5|equipped.algethar_puzzle_box&cooldown.metamorphosis.remains>(cooldown.blade_dance.duration)|!talent.demonic|(raid_event.adds.in>cooldown&raid_event.adds.in<25))" );
  default_->add_action( "sigil_of_flame,if=talent.any_means_necessary&debuff.essence_break.down&active_enemies>=4" );
  default_->add_action( "throw_glaive,if=talent.soulrend&(active_enemies>desired_targets|raid_event.adds.in>full_recharge_time+9)&spell_targets>=(2-talent.furious_throws)&!debuff.essence_break.up&(full_recharge_time<gcd.max*3|active_enemies>1)" );
  default_->add_action( "sigil_of_flame,if=talent.any_means_necessary&debuff.essence_break.down" );
  default_->add_action( "immolation_aura,if=active_enemies>=2&fury<70&debuff.essence_break.down" );
  default_->add_action( "annihilation,if=!variable.pooling_for_blade_dance|set_bonus.tier30_2pc" );
  default_->add_action( "throw_glaive,if=talent.soulrend&(active_enemies>desired_targets|raid_event.adds.in>full_recharge_time+9)&spell_targets>=(2-talent.furious_throws)&!debuff.essence_break.up" );
  default_->add_action( "immolation_aura,if=!buff.immolation_aura.up&(!talent.ragefire|active_enemies>desired_targets|raid_event.adds.in>15)&buff.out_of_range.down" );
  default_->add_action( "fel_rush,if=talent.isolated_prey&active_enemies=1&fury.deficit>=35" );
  default_->add_action( "chaos_strike,if=!variable.pooling_for_blade_dance&!variable.pooling_for_eye_beam" );
  default_->add_action( "sigil_of_flame,if=raid_event.adds.in>15&fury.deficit>=30&buff.out_of_range.down" );
  default_->add_action( "felblade,if=fury.deficit>=40&buff.out_of_range.down" );
  default_->add_action( "fel_rush,if=!talent.momentum&talent.demon_blades&!cooldown.eye_beam.ready&(charges=2|(raid_event.movement.in>10&raid_event.adds.in>10))" );
  default_->add_action( "demons_bite,target_if=min:debuff.burning_wound.remains,if=talent.burning_wound&debuff.burning_wound.remains<4&active_dot.burning_wound<(spell_targets>?3)" );
  default_->add_action( "fel_rush,if=!talent.momentum&!talent.demon_blades&spell_targets>1&(charges=2|(raid_event.movement.in>10&raid_event.adds.in>10))" );
  default_->add_action( "sigil_of_flame,if=raid_event.adds.in>15&fury.deficit>=30&buff.out_of_range.down" );
  default_->add_action( "demons_bite" );
  default_->add_action( "fel_rush,if=movement.distance>15|(buff.out_of_range.up&!talent.momentum)" );
  default_->add_action( "vengeful_retreat,if=!talent.initiative&movement.distance>15" );
  default_->add_action( "throw_glaive,if=(talent.demon_blades|buff.out_of_range.up)&!debuff.essence_break.up&buff.out_of_range.down" );

  meta_end->add_action( "death_sweep" );
  meta_end->add_action( "annihilation" );

  cooldown->add_action( "metamorphosis,if=!talent.demonic&((!talent.chaotic_transformation|cooldown.eye_beam.remains>20)&active_enemies>desired_targets|raid_event.adds.in>60|fight_remains<25)", "Cast Metamorphosis if we will get a full Eye Beam refresh or if the encounter is almost over" );
  cooldown->add_action( "metamorphosis,if=talent.demonic&(!talent.chaotic_transformation|cooldown.eye_beam.remains>20&(!variable.blade_dance|cooldown.blade_dance.remains>gcd.max)|fight_remains<25+talent.shattered_destiny*70&cooldown.eye_beam.remains&cooldown.blade_dance.remains)" );
  cooldown->add_action( "potion,if=buff.metamorphosis.remains>25|buff.metamorphosis.up&cooldown.metamorphosis.ready|fight_remains<60|time>0.1&time<10" );
  cooldown->add_action( "elysian_decree,if=(active_enemies>desired_targets|raid_event.adds.in>30)" );
  cooldown->add_action( "use_item,name=manic_grieftorch,use_off_gcd=1,if=buff.vengeful_retreat_movement.down&((buff.initiative.remains>2&debuff.essence_break.down&cooldown.essence_break.remains>gcd.max&time>14|time_to_die<10|time<1&!equipped.algethar_puzzle_box|fight_remains%%120<5)&!prev_gcd.1.essence_break)" );
  cooldown->add_action( "use_item,name=algethar_puzzle_box,use_off_gcd=1,if=cooldown.metamorphosis.remains<=gcd.max*5|fight_remains%%180>10&fight_remains%%180<22|fight_remains<25" );
  cooldown->add_action( "use_item,name=irideus_fragment,use_off_gcd=1,if=cooldown.metamorphosis.remains<=gcd.max&time>2|fight_remains%%180>10&fight_remains%%180<22|fight_remains<22" );
  cooldown->add_action( "use_items,slots=trinket1,if=(variable.trinket_sync_slot=1&(buff.metamorphosis.up|(!talent.demonic.enabled&cooldown.metamorphosis.remains>(fight_remains>?trinket.1.cooldown.duration%2))|fight_remains<=20)|(variable.trinket_sync_slot=2&!trinket.2.cooldown.ready)|!variable.trinket_sync_slot)&(!talent.initiative|buff.initiative.up)", "Default use item logic" );
  cooldown->add_action( "use_items,slots=trinket2,if=(variable.trinket_sync_slot=2&(buff.metamorphosis.up|(!talent.demonic.enabled&cooldown.metamorphosis.remains>(fight_remains>?trinket.2.cooldown.duration%2))|fight_remains<=20)|(variable.trinket_sync_slot=1&!trinket.1.cooldown.ready)|!variable.trinket_sync_slot)&(!talent.initiative|buff.initiative.up)" );
  cooldown->add_action( "use_item,name=stormeaters_boon,use_off_gcd=1,if=cooldown.metamorphosis.remains&(!talent.momentum|buff.momentum.remains>5)&(active_enemies>1|raid_event.adds.in>140)" );
  cooldown->add_action( "use_item,name=beacon_to_the_beyond,use_off_gcd=1,if=buff.vengeful_retreat_movement.down&debuff.essence_break.down&!prev_gcd.1.essence_break&(!equipped.irideus_fragment|trinket.1.cooldown.remains>20|trinket.2.cooldown.remains>20)" );
  cooldown->add_action( "use_item,name=dragonfire_bomb_dispenser,use_off_gcd=1,if=(time_to_die<30|cooldown.vengeful_retreat.remains<5|equipped.beacon_to_the_beyond|equipped.irideus_fragment)&(trinket.1.cooldown.remains>10|trinket.2.cooldown.remains>10|trinket.1.cooldown.duration=0|trinket.2.cooldown.duration=0|equipped.elementium_pocket_anvil|equipped.screaming_black_dragonscale|equipped.mark_of_dargrul)|(trinket.1.cooldown.duration>0|trinket.2.cooldown.duration>0)&(trinket.1.cooldown.remains|trinket.2.cooldown.remains)&!equipped.elementium_pocket_anvil&time<25" );
  cooldown->add_action( "use_item,name=elementium_pocket_anvil,use_off_gcd=1,if=!prev_gcd.1.fel_rush&gcd.remains" );
  cooldown->add_action( "use_items,slots=trinket1,if=(variable.trinket_sync_slot=1&(buff.metamorphosis.up|(!talent.demonic.enabled&cooldown.metamorphosis.remains>(fight_remains>?trinket.1.cooldown.duration%2))|fight_remains<=20)|(variable.trinket_sync_slot=2&!trinket.2.cooldown.ready)|!variable.trinket_sync_slot)&(!talent.initiative|buff.initiative.up)" );
  cooldown->add_action( "use_items,slots=trinket2,if=(variable.trinket_sync_slot=2&(buff.metamorphosis.up|(!talent.demonic.enabled&cooldown.metamorphosis.remains>(fight_remains>?trinket.2.cooldown.duration%2))|fight_remains<=20)|(variable.trinket_sync_slot=1&!trinket.1.cooldown.ready)|!variable.trinket_sync_slot)&(!talent.initiative|buff.initiative.up)" );
  cooldown->add_action( "use_item,name=stormeaters_boon,use_off_gcd=1,if=cooldown.metamorphosis.remains&(!talent.momentum|buff.momentum.remains>5)&(active_enemies>1|raid_event.adds.in>140)" );
}
//havoc_apl_end

//vengeance_apl_start
void vengeance( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );
  action_priority_list_t* maintenance = p->get_action_priority_list( "maintenance" );
  action_priority_list_t* single_target = p->get_action_priority_list( "single_target" );
  action_priority_list_t* small_aoe = p->get_action_priority_list( "small_aoe" );
  action_priority_list_t* big_aoe = p->get_action_priority_list( "big_aoe" );
  action_priority_list_t* filler = p->get_action_priority_list( "filler" );

  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "sigil_of_flame" );
  precombat->add_action( "immolation_aura" );

  default_->add_action( "variable,name=next_cd_time,value=cooldown.fel_devastation.remains", "Time to CD: When will the next cooldown be available?" );
  default_->add_action( "variable,name=next_cd_time,op=min,value=cooldown.elysian_decree.remains,if=talent.elysian_decree" );
  default_->add_action( "variable,name=next_cd_time,op=min,value=cooldown.the_hunt.remains,if=talent.the_hunt" );
  default_->add_action( "variable,name=next_cd_time,op=min,value=cooldown.soul_carver.remains,if=talent.soul_carver" );
  default_->add_action( "variable,name=next_fire_cd_time,value=cooldown.fel_devastation.remains", "When will the next fire damage cooldown be available?" );
  default_->add_action( "variable,name=next_fire_cd_time,op=min,value=cooldown.soul_carver.remains,if=talent.soul_carver" );
  default_->add_action( "variable,name=fd,value=talent.fiery_demise&dot.fiery_brand.ticking", "Fiery Demise is up" );
  default_->add_action( "variable,name=frailty_ready,value=!talent.soulcrush|debuff.frailty.stack>=2", "Number of required frailty stacks for CDs" );
  default_->add_action( "auto_attack" );
  default_->add_action( "disrupt,if=target.debuff.casting.react" );
  default_->add_action( "infernal_strike,use_off_gcd=1" );
  default_->add_action( "demon_spikes,use_off_gcd=1,if=!buff.demon_spikes.up&!cooldown.pause_action.remains" );
  default_->add_action( "call_action_list,name=maintenance" );
  default_->add_action( "run_action_list,name=single_target,if=active_enemies=1" );
  default_->add_action( "run_action_list,name=small_aoe,if=active_enemies>1&active_enemies<=5" );
  default_->add_action( "run_action_list,name=big_aoe,if=active_enemies>=6" );

  trinkets->add_action( "use_item,name=elementium_pocket_anvil,use_off_gcd=1" );
  trinkets->add_action( "use_item,name=dragonfire_bomb_dispenser,use_off_gcd=1" );
  trinkets->add_action( "use_items" );

  maintenance->add_action( "call_action_list,name=trinkets" );
  maintenance->add_action( "potion" );
  maintenance->add_action( "metamorphosis,if=talent.first_of_the_illidari" );
  maintenance->add_action( "fiery_brand,if=charges>=2|(!ticking&((variable.next_fire_cd_time<7)|(variable.next_fire_cd_time>28)))" );
  maintenance->add_action( "spirit_bomb,if=soul_fragments>=5" );
  maintenance->add_action( "fracture,target_if=max:dot.fiery_brand.remains,if=dot.fiery_brand.ticking&buff.recrimination.up" );
  maintenance->add_action( "fracture,if=buff.recrimination.up" );
  maintenance->add_action( "fracture,if=(full_recharge_time<=cast_time+gcd.remains)" );
  maintenance->add_action( "immolation_aura" );
  maintenance->add_action( "sigil_of_flame,if=dot.fiery_brand.ticking" );
  maintenance->add_action( "metamorphosis,if=talent.demonic&!buff.metamorphosis.up&!cooldown.fel_devastation.up" );

  single_target->add_action( "soul_carver,if=variable.fd&variable.frailty_ready&soul_fragments<=3" );
  single_target->add_action( "the_hunt,if=variable.frailty_ready" );
  single_target->add_action( "soul_carver,if=variable.frailty_ready&soul_fragments<=3" );
  single_target->add_action( "fel_devastation,if=variable.frailty_ready&(variable.fd|talent.stoke_the_flames)&!(talent.demonic&buff.metamorphosis.up)" );
  single_target->add_action( "elysian_decree,if=variable.frailty_ready" );
  single_target->add_action( "fracture,if=set_bonus.tier30_4pc&variable.fd&(soul_fragments<=3|(buff.metamorphosis.up&soul_fragments<=2))" );
  single_target->add_action( "fel_devastation,if=!(talent.demonic&buff.metamorphosis.up)" );
  single_target->add_action( "spirit_bomb,if=((variable.fd&soul_fragments>=4)|soul_fragments>=5)" );
  single_target->add_action( "fracture,if=set_bonus.tier30_4pc&variable.fd" );
  single_target->add_action( "spirit_bomb,if=((variable.fd&soul_fragments>=3)|soul_fragments>=4)" );
  single_target->add_action( "soul_cleave,if=talent.focused_cleave" );
  single_target->add_action( "fracture" );
  single_target->add_action( "spirit_bomb,if=((variable.fd&soul_fragments>=2)|soul_fragments>=3)" );
  single_target->add_action( "sigil_of_flame" );
  single_target->add_action( "soul_cleave" );
  single_target->add_action( "call_action_list,name=filler" );

  small_aoe->add_action( "elysian_decree,if=variable.frailty_ready" );
  small_aoe->add_action( "fel_devastation,if=variable.frailty_ready&variable.fd&talent.stoke_the_flames&!(talent.demonic&buff.metamorphosis.up)" );
  small_aoe->add_action( "the_hunt,if=variable.frailty_ready" );
  small_aoe->add_action( "fel_devastation,if=variable.frailty_ready&(variable.fd|talent.stoke_the_flames)&!(talent.demonic&buff.metamorphosis.up)" );
  small_aoe->add_action( "spirit_bomb,if=((variable.fd&soul_fragments>=4)|soul_fragments>=5)" );
  small_aoe->add_action( "soul_carver,if=variable.frailty_ready&variable.fd&soul_fragments<=3" );
  small_aoe->add_action( "fel_devastation,if=!(talent.demonic&buff.metamorphosis.up)" );
  small_aoe->add_action( "fracture,if=soul_fragments<=3&soul_fragments>=1" );
  small_aoe->add_action( "sigil_of_flame" );
  small_aoe->add_action( "soul_carver,if=variable.frailty_ready&soul_fragments<=3" );
  small_aoe->add_action( "spirit_bomb,if=((variable.fd&soul_fragments>=3)|soul_fragments>=4)" );
  small_aoe->add_action( "soul_cleave,if=talent.focused_cleave" );
  small_aoe->add_action( "spirit_bomb,if=((variable.fd&soul_fragments>=2)|soul_fragments>=3)" );
  small_aoe->add_action( "soul_cleave" );
  small_aoe->add_action( "spirit_bomb,if=((variable.fd&soul_fragments>=1)|soul_fragments>=2)" );
  small_aoe->add_action( "fracture" );
  small_aoe->add_action( "call_action_list,name=filler" );

  big_aoe->add_action( "fel_devastation,if=variable.frailty_ready&variable.fd&talent.stoke_the_flames&!(talent.demonic&buff.metamorphosis.up)" );
  big_aoe->add_action( "elysian_decree,if=variable.frailty_ready" );
  big_aoe->add_action( "fel_devastation,if=variable.frailty_ready&(variable.fd|talent.stoke_the_flames)&!(talent.demonic&buff.metamorphosis.up)" );
  big_aoe->add_action( "the_hunt,if=variable.frailty_ready" );
  big_aoe->add_action( "fel_devastation,if=!(talent.demonic&buff.metamorphosis.up)" );
  big_aoe->add_action( "spirit_bomb,if=((variable.fd&soul_fragments>=4)|soul_fragments>=5)" );
  big_aoe->add_action( "fracture,if=soul_fragments<=3&soul_fragments>=1" );
  big_aoe->add_action( "soul_carver,if=variable.fd&variable.frailty_ready&soul_fragments<=3" );
  big_aoe->add_action( "spirit_bomb,if=((variable.fd&soul_fragments>=3)|soul_fragments>=4)" );
  big_aoe->add_action( "soul_carver,if=soul_fragments<=3" );
  big_aoe->add_action( "soul_cleave,if=talent.focused_cleave" );
  big_aoe->add_action( "spirit_bomb,if=((variable.fd&soul_fragments>=2)|soul_fragments>=3)" );
  big_aoe->add_action( "soul_cleave" );
  big_aoe->add_action( "fracture,if=soul_fragments<=3" );
  big_aoe->add_action( "call_action_list,name=filler" );

  filler->add_action( "bulk_extraction" );
  filler->add_action( "soul_cleave" );
  filler->add_action( "spirit_bomb" );
  filler->add_action( "felblade" );
  filler->add_action( "fracture" );
  filler->add_action( "shear" );
  filler->add_action( "throw_glaive" );
}
//vengeance_apl_end

}
