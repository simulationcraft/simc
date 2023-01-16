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
  return ( ( p->true_level >= 61 ) ? "phial_of_elemental_chaos_3" :
           ( p->true_level >= 51 ) ? "spectral_flask_of_power" :
           ( p->true_level >= 40 ) ? "greater_flask_of_the_currents" :
           ( p->true_level >= 35 ) ? "greater_draenic_agility_flask" :
           "disabled" );
}

std::string flask_vengeance( const player_t* p )
{
  return ( ( p->true_level >= 61 ) ? "phial_of_glacial_fury_3" :
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
  action_priority_list_t* cooldown = p->get_action_priority_list( "cooldown" );

  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "variable,name=trinket_sync_slot,value=1,if=trinket.1.has_stat.any_dps&(!trinket.2.has_stat.any_dps|trinket.1.cooldown.duration>=trinket.2.cooldown.duration)" );
  precombat->add_action( "variable,name=trinket_sync_slot,value=2,if=trinket.2.has_stat.any_dps&(!trinket.1.has_stat.any_dps|trinket.2.cooldown.duration>trinket.1.cooldown.duration)" );
  precombat->add_action( "arcane_torrent" );
  precombat->add_action( "use_item,name=algethar_puzzle_box" );
  precombat->add_action( "sigil_of_flame" );
  precombat->add_action( "immolation_aura" );

  default_->add_action( "auto_attack" );
  default_->add_action( "retarget_auto_attack,line_cd=1,target_if=min:debuff.burning_wound.remains,if=talent.burning_wound&talent.demon_blades&active_dot.burning_wound<(spell_targets>?3)" );
  default_->add_action( "variable,name=blade_dance,value=talent.first_blood|talent.trail_of_ruin|talent.chaos_theory&buff.chaos_theory.down|spell_targets.blade_dance1>1", "Blade Dance with First Blood, Trail of Ruin, or at 2+ targets" );
  default_->add_action( "variable,name=pooling_for_blade_dance,value=variable.blade_dance&fury<(75-talent.demon_blades*20)&cooldown.blade_dance.remains<gcd.max" );
  default_->add_action( "variable,name=pooling_for_eye_beam,value=talent.demonic&!talent.blind_fury&cooldown.eye_beam.remains<(gcd.max*2)&fury.deficit>20" );
  default_->add_action( "variable,name=waiting_for_momentum,value=talent.momentum&!buff.momentum.up" );
  default_->add_action( "disrupt" );
  default_->add_action( "call_action_list,name=cooldown,if=gcd.remains=0" );
  default_->add_action( "pick_up_fragment,type=demon,if=demon_soul_fragments>0" );
  default_->add_action( "pick_up_fragment,mode=nearest,if=talent.demonic_appetite&fury.deficit>=35&(!cooldown.eye_beam.ready|fury<30)" );
  default_->add_action( "vengeful_retreat,use_off_gcd=1,if=talent.initiative&talent.essence_break&time>1&(cooldown.essence_break.remains>15|cooldown.essence_break.remains<gcd.max&(!talent.demonic|buff.metamorphosis.up|cooldown.eye_beam.remains>15+(10*talent.cycle_of_hatred)))" );
  default_->add_action( "vengeful_retreat,use_off_gcd=1,if=talent.initiative&!talent.essence_break&time>1&!buff.momentum.up" );
  default_->add_action( "fel_rush,if=(buff.unbound_chaos.up|variable.waiting_for_momentum&(!talent.unbound_chaos|!cooldown.immolation_aura.ready))&(charges=2|(raid_event.movement.in>10&raid_event.adds.in>10))" );
  default_->add_action( "essence_break,if=(active_enemies>desired_targets|raid_event.adds.in>40)&!variable.waiting_for_momentum&fury>40&(cooldown.eye_beam.remains>8|buff.metamorphosis.up)&(!talent.tactical_retreat|buff.tactical_retreat.up)" );
  default_->add_action( "death_sweep,if=variable.blade_dance&(!talent.essence_break|cooldown.essence_break.remains>(cooldown.death_sweep.duration-4))" );
  default_->add_action( "fel_barrage,if=active_enemies>desired_targets|raid_event.adds.in>30" );
  default_->add_action( "glaive_tempest,if=active_enemies>desired_targets|raid_event.adds.in>10" );
  default_->add_action( "eye_beam,if=active_enemies>desired_targets|raid_event.adds.in>(40-talent.cycle_of_hatred*15)&!debuff.essence_break.up" );
  default_->add_action( "blade_dance,if=variable.blade_dance&(cooldown.eye_beam.remains>5|!talent.demonic|(raid_event.adds.in>cooldown&raid_event.adds.in<25))" );
  default_->add_action( "throw_glaive,if=talent.soulrend&(active_enemies>desired_targets|raid_event.adds.in>full_recharge_time+9)&spell_targets>=(2-talent.furious_throws)&!debuff.essence_break.up" );
  default_->add_action( "annihilation,if=!variable.pooling_for_blade_dance" );
  default_->add_action( "throw_glaive,if=talent.serrated_glaive&cooldown.eye_beam.remains<4&!debuff.serrated_glaive.up&!debuff.essence_break.up" );
  default_->add_action( "immolation_aura,if=!buff.immolation_aura.up&(!talent.ragefire|active_enemies>desired_targets|raid_event.adds.in>15)" );
  default_->add_action( "felblade,if=fury.deficit>=40" );
  default_->add_action( "sigil_of_flame,if=active_enemies>desired_targets" );
  default_->add_action( "chaos_strike,if=!variable.pooling_for_blade_dance&!variable.pooling_for_eye_beam" );
  default_->add_action( "fel_rush,if=!talent.momentum&talent.demon_blades&!cooldown.eye_beam.ready&(charges=2|(raid_event.movement.in>10&raid_event.adds.in>10))" );
  default_->add_action( "demons_bite,target_if=min:debuff.burning_wound.remains,if=talent.burning_wound&debuff.burning_wound.remains<4&active_dot.burning_wound<(spell_targets>?3)" );
  default_->add_action( "fel_rush,if=!talent.momentum&!talent.demon_blades&spell_targets>1&(charges=2|(raid_event.movement.in>10&raid_event.adds.in>10))" );
  default_->add_action( "sigil_of_flame,if=raid_event.adds.in>15&fury.deficit>=30" );
  default_->add_action( "demons_bite" );
  default_->add_action( "fel_rush,if=movement.distance>15|(buff.out_of_range.up&!talent.momentum)" );
  default_->add_action( "vengeful_retreat,if=!talent.initiative&movement.distance>15" );
  default_->add_action( "throw_glaive,if=(talent.demon_blades|buff.out_of_range.up)&!debuff.essence_break.up" );

  cooldown->add_action( "metamorphosis,if=!talent.demonic&((!talent.chaotic_transformation|cooldown.eye_beam.remains>20)&active_enemies>desired_targets|raid_event.adds.in>60|fight_remains<25)", "Cast Metamorphosis if we will get a full Eye Beam refresh or if the encounter is almost over" );
  cooldown->add_action( "metamorphosis,if=talent.demonic&(!talent.chaotic_transformation|cooldown.eye_beam.remains>20&(!variable.blade_dance|cooldown.blade_dance.remains>gcd.max)|fight_remains<25)" );
  cooldown->add_action( "potion,if=buff.metamorphosis.remains>25|buff.metamorphosis.up&cooldown.metamorphosis.ready|fight_remains<60" );
  cooldown->add_action( "use_items,slots=trinket1,if=variable.trinket_sync_slot=1&(buff.metamorphosis.up|(!talent.demonic.enabled&cooldown.metamorphosis.remains>(fight_remains>?trinket.1.cooldown.duration%2))|fight_remains<=20)|(variable.trinket_sync_slot=2&!trinket.2.cooldown.ready)|!variable.trinket_sync_slot", "Default use item logic" );
  cooldown->add_action( "use_items,slots=trinket2,if=variable.trinket_sync_slot=2&(buff.metamorphosis.up|(!talent.demonic.enabled&cooldown.metamorphosis.remains>(fight_remains>?trinket.2.cooldown.duration%2))|fight_remains<=20)|(variable.trinket_sync_slot=1&!trinket.1.cooldown.ready)|!variable.trinket_sync_slot" );
  cooldown->add_action( "the_hunt,if=(!talent.momentum|!buff.momentum.up)" );
  cooldown->add_action( "elysian_decree,if=(active_enemies>desired_targets|raid_event.adds.in>30)" );
}
//havoc_apl_end

//vengeance_apl_start
void vengeance( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* the_hunt_ramp = p->get_action_priority_list( "the_hunt_ramp" );
  action_priority_list_t* elysian_decree_ramp = p->get_action_priority_list( "elysian_decree_ramp" );
  action_priority_list_t* soul_carver_without_fiery_demise_ramp = p->get_action_priority_list( "soul_carver_without_fiery_demise_ramp" );
  action_priority_list_t* fiery_demise_window_with_soul_carver = p->get_action_priority_list( "fiery_demise_window_with_soul_carver" );
  action_priority_list_t* fiery_demise_window_without_soul_carver = p->get_action_priority_list( "fiery_demise_window_without_soul_carver" );

  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "variable,name=the_hunt_ramp_in_progress,value=0", "Variables for specific sub-APLs" );
  precombat->add_action( "variable,name=elysian_decree_ramp_in_progress,value=0" );
  precombat->add_action( "variable,name=soul_carver_ramp_in_progress,value=0" );
  precombat->add_action( "variable,name=fiery_demise_with_soul_carver_in_progress,value=0" );
  precombat->add_action( "variable,name=fiery_demise_without_soul_carver_in_progress,value=0" );
  precombat->add_action( "variable,name=shear_fury_gain_not_in_meta_no_shear_fury,op=setif,value=12,value_else=10,condition=set_bonus.tier29_2pc", "Variables for complex derived values" );
  precombat->add_action( "variable,name=shear_fury_gain_not_in_meta_shear_fury,op=setif,value=24,value_else=20,condition=set_bonus.tier29_4pc" );
  precombat->add_action( "variable,name=shear_fury_gain_not_in_meta,op=setif,value=variable.shear_fury_gain_not_in_meta_shear_fury,value_else=variable.shear_fury_gain_not_in_meta_no_shear_fury,condition=talent.shear_fury.enabled" );
  precombat->add_action( "variable,name=shear_fury_gain_in_meta_no_shear_fury,op=setif,value=36,value_else=30,condition=set_bonus.tier29_2pc" );
  precombat->add_action( "variable,name=shear_fury_gain_in_meta_shear_fury,op=setif,value=48,value_else=40,condition=set_bonus.tier29_2pc" );
  precombat->add_action( "variable,name=shear_fury_gain_in_meta,op=setif,value=variable.shear_fury_gain_in_meta_shear_fury,value_else=variable.shear_fury_gain_in_meta_no_shear_fury,condition=talent.shear_fury.enabled" );
  precombat->add_action( "variable,name=fracture_fury_gain_not_in_meta,op=setif,value=30,value_else=25,condition=set_bonus.tier29_2pc" );
  precombat->add_action( "variable,name=fracture_fury_gain_in_meta,op=setif,value=54,value_else=45,condition=set_bonus.tier29_2pc" );
  precombat->add_action( "variable,name=spirit_bomb_soul_fragments_not_in_meta,op=setif,value=4,value_else=5,condition=talent.fracture.enabled" );
  precombat->add_action( "variable,name=spirit_bomb_soul_fragments_in_meta,op=setif,value=3,value_else=4,condition=talent.fracture.enabled" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "sigil_of_flame" );
  precombat->add_action( "immolation_aura,if=active_enemies=1|active_enemies>1&!talent.fallout.enabled" );

  default_->add_action( "auto_attack" );
  default_->add_action( "disrupt,if=target.debuff.casting.react" );
  default_->add_action( "infernal_strike,use_off_gcd=1" );
  default_->add_action( "demon_spikes,use_off_gcd=1,if=!buff.demon_spikes.up&!cooldown.pause_action.remains" );
  default_->add_action( "metamorphosis,if=!buff.metamorphosis.up&!dot.fiery_brand.ticking" );
  default_->add_action( "fiery_brand,if=!talent.fiery_demise.enabled&!dot.fiery_brand.ticking" );
  default_->add_action( "bulk_extraction" );
  default_->add_action( "potion" );
  default_->add_action( "use_item,slot=trinket1" );
  default_->add_action( "use_item,slot=trinket2" );
  default_->add_action( "variable,name=fracture_fury_gain,op=setif,value=variable.fracture_fury_gain_in_meta,value_else=variable.fracture_fury_gain_not_in_meta,condition=buff.metamorphosis.up" );
  default_->add_action( "variable,name=shear_fury_gain,op=setif,value=variable.shear_fury_gain_in_meta,value_else=variable.shear_fury_gain_not_in_meta,condition=buff.metamorphosis.up" );
  default_->add_action( "variable,name=spirit_bomb_soul_fragments,op=setif,value=variable.spirit_bomb_soul_fragments_in_meta,value_else=variable.spirit_bomb_soul_fragments_not_in_meta,condition=buff.metamorphosis.up" );
  default_->add_action( "variable,name=frailty_target_requirement,op=setif,value=5,value_else=6,condition=spell_targets.spirit_bomb>1" );
  default_->add_action( "variable,name=frailty_dump_fury_requirement,op=setif,value=action.spirit_bomb.cost+(action.soul_cleave.cost*2),value_else=action.soul_cleave.cost*3,condition=spell_targets.spirit_bomb>1" );
  default_->add_action( "variable,name=pooling_for_the_hunt,value=talent.the_hunt.enabled&cooldown.the_hunt.remains<(gcd.max*2)&fury<variable.frailty_dump_fury_requirement&debuff.frailty.stack<=1" );
  default_->add_action( "variable,name=pooling_for_elysian_decree,value=talent.elysian_decree.enabled&cooldown.elysian_decree.remains<(gcd.max*2)&fury<variable.frailty_dump_fury_requirement&debuff.frailty.stack<=1" );
  default_->add_action( "variable,name=pooling_for_soul_carver,value=talent.soul_carver.enabled&cooldown.soul_carver.remains<(gcd.max*2)&fury<variable.frailty_dump_fury_requirement&debuff.frailty.stack<=1" );
  default_->add_action( "variable,name=pooling_for_fiery_demise_fel_devastation,value=talent.fiery_demise.enabled&cooldown.fel_devastation.remains_expected<(gcd.max*2)&dot.fiery_brand.ticking&fury<action.fel_devastation.cost" );
  default_->add_action( "variable,name=pooling_fury,value=variable.pooling_for_the_hunt|variable.pooling_for_elysian_decree|variable.pooling_for_soul_carver|variable.pooling_for_fiery_demise_fel_devastation" );
  default_->add_action( "variable,name=the_hunt_ramp_in_progress,value=1,if=talent.the_hunt.enabled&cooldown.the_hunt.up&!dot.fiery_brand.ticking" );
  default_->add_action( "variable,name=the_hunt_ramp_in_progress,value=0,if=cooldown.the_hunt.remains" );
  default_->add_action( "variable,name=elysian_decree_ramp_in_progress,value=1,if=talent.elysian_decree.enabled&cooldown.elysian_decree.up&!dot.fiery_brand.ticking" );
  default_->add_action( "variable,name=elysian_decree_ramp_in_progress,value=0,if=cooldown.elysian_decree.remains" );
  default_->add_action( "variable,name=soul_carver_ramp_in_progress,value=1,if=talent.soul_carver.enabled&cooldown.soul_carver.up&!talent.fiery_demise.enabled&!dot.fiery_brand.ticking" );
  default_->add_action( "variable,name=soul_carver_ramp_in_progress,value=0,if=cooldown.soul_carver.remains" );
  default_->add_action( "variable,name=fiery_demise_with_soul_carver_in_progress,value=1,if=talent.fiery_demise.enabled&talent.soul_carver.enabled&cooldown.soul_carver.up&cooldown.fiery_brand.up&cooldown.immolation_aura.up&cooldown.fel_devastation.remains_expected<10&!dot.fiery_brand.ticking" );
  default_->add_action( "variable,name=fiery_demise_with_soul_carver_in_progress,value=0,if=cooldown.soul_carver.remains&(cooldown.fiery_brand.remains|(cooldown.fiery_brand.max_charges=2&cooldown.fiery_brand.charges=1))&cooldown.fel_devastation.remains_expected" );
  default_->add_action( "variable,name=fiery_demise_without_soul_carver_in_progress,value=1,if=talent.fiery_demise.enabled&((talent.soul_carver.enabled&!cooldown.soul_carver.up)|!talent.soul_carver.enabled)&cooldown.fiery_brand.up&cooldown.immolation_aura.up&cooldown.fel_devastation.remains_expected<10&((talent.darkglare_boon.enabled&darkglare_boon_cdr_high_roll)|!talent.darkglare_boon.enabled)" );
  default_->add_action( "variable,name=fiery_demise_without_soul_carver_in_progress,value=0,if=(cooldown.fiery_brand.remains|(cooldown.fiery_brand.max_charges=2&cooldown.fiery_brand.charges=1))&cooldown.fel_devastation.remains_expected>35" );
  default_->add_action( "run_action_list,name=the_hunt_ramp,if=variable.the_hunt_ramp_in_progress" );
  default_->add_action( "run_action_list,name=elysian_decree_ramp,if=variable.elysian_decree_ramp_in_progress" );
  default_->add_action( "run_action_list,name=soul_carver_without_fiery_demise_ramp,if=variable.soul_carver_ramp_in_progress" );
  default_->add_action( "run_action_list,name=fiery_demise_window_with_soul_carver,if=variable.fiery_demise_with_soul_carver_in_progress" );
  default_->add_action( "run_action_list,name=fiery_demise_window_without_soul_carver,if=variable.fiery_demise_without_soul_carver_in_progress" );
  default_->add_action( "fel_devastation,if=!talent.down_in_flames.enabled" );
  default_->add_action( "spirit_bomb,if=soul_fragments>=variable.spirit_bomb_soul_fragments&spell_targets>1" );
  default_->add_action( "soul_cleave,if=(talent.spirit_bomb.enabled&soul_fragments<=1&spell_targets>1)|(spell_targets<2&((talent.fracture.enabled&fury>=55)|(!talent.fracture.enabled&fury>=70)|(buff.metamorphosis.up&((talent.fracture.enabled&fury>=35)|(!talent.fracture.enabled&fury>=50)))))|(!talent.spirit_bomb.enabled)&((talent.fracture.enabled&fury>=55)|(!talent.fracture.enabled&fury>=70)|(buff.metamorphosis.up&((talent.fracture.enabled&fury>=35)|(!talent.fracture.enabled&fury>=50))))" );
  default_->add_action( "immolation_aura,if=(talent.fiery_demise.enabled&fury.deficit>=10&(cooldown.soul_carver.remains>15))|(!talent.fiery_demise.enabled&fury.deficit>=10)" );
  default_->add_action( "felblade,if=fury.deficit>=40" );
  default_->add_action( "fracture,if=(talent.spirit_bomb.enabled&(soul_fragments<=3&spell_targets>1|spell_targets<2&fury.deficit>=variable.fracture_fury_gain))|(!talent.spirit_bomb.enabled&fury.deficit>=variable.fracture_fury_gain)" );
  default_->add_action( "sigil_of_flame,if=fury.deficit>=30" );
  default_->add_action( "shear" );
  default_->add_action( "throw_glaive" );

  the_hunt_ramp->add_action( "the_hunt,if=debuff.frailty.stack>=variable.frailty_target_requirement" );
  the_hunt_ramp->add_action( "spirit_bomb,if=!variable.pooling_fury&soul_fragments>=variable.spirit_bomb_soul_fragments&spell_targets>1" );
  the_hunt_ramp->add_action( "soul_cleave,if=!variable.pooling_fury&(soul_fragments<=1&spell_targets>1|spell_targets<2)" );
  the_hunt_ramp->add_action( "sigil_of_flame" );
  the_hunt_ramp->add_action( "fracture" );
  the_hunt_ramp->add_action( "shear" );

  elysian_decree_ramp->add_action( "elysian_decree,if=debuff.frailty.stack>=variable.frailty_target_requirement" );
  elysian_decree_ramp->add_action( "spirit_bomb,if=!variable.pooling_fury&soul_fragments>=variable.spirit_bomb_soul_fragments&spell_targets>1" );
  elysian_decree_ramp->add_action( "soul_cleave,if=!variable.pooling_fury&(soul_fragments<=1&spell_targets>1|spell_targets<2)" );
  elysian_decree_ramp->add_action( "sigil_of_flame" );
  elysian_decree_ramp->add_action( "fracture" );
  elysian_decree_ramp->add_action( "shear" );

  soul_carver_without_fiery_demise_ramp->add_action( "soul_carver,if=debuff.frailty.stack>=variable.frailty_target_requirement" );
  soul_carver_without_fiery_demise_ramp->add_action( "spirit_bomb,if=!variable.pooling_fury&soul_fragments>=variable.spirit_bomb_soul_fragments&spell_targets>1" );
  soul_carver_without_fiery_demise_ramp->add_action( "soul_cleave,if=!variable.pooling_fury&(soul_fragments<=1&spell_targets>1|spell_targets<2)" );
  soul_carver_without_fiery_demise_ramp->add_action( "sigil_of_flame" );
  soul_carver_without_fiery_demise_ramp->add_action( "fracture" );
  soul_carver_without_fiery_demise_ramp->add_action( "shear" );

  fiery_demise_window_with_soul_carver->add_action( "fiery_brand,if=!fiery_brand_dot_primary_ticking&fury>=30" );
  fiery_demise_window_with_soul_carver->add_action( "immolation_aura,if=fiery_brand_dot_primary_ticking" );
  fiery_demise_window_with_soul_carver->add_action( "soul_carver,if=fiery_brand_dot_primary_ticking&debuff.frailty.stack>=variable.frailty_target_requirement&soul_fragments<=3" );
  fiery_demise_window_with_soul_carver->add_action( "fel_devastation,if=fiery_brand_dot_primary_ticking&fiery_brand_dot_primary_remains<=2" );
  fiery_demise_window_with_soul_carver->add_action( "spirit_bomb,if=!variable.pooling_fury&soul_fragments>=variable.spirit_bomb_soul_fragments&(spell_targets>1|dot.fiery_brand.ticking)" );
  fiery_demise_window_with_soul_carver->add_action( "soul_cleave,if=!variable.pooling_fury&(soul_fragments<=1&spell_targets>1|spell_targets<2)" );
  fiery_demise_window_with_soul_carver->add_action( "sigil_of_flame,if=dot.fiery_brand.ticking" );
  fiery_demise_window_with_soul_carver->add_action( "fracture" );
  fiery_demise_window_with_soul_carver->add_action( "shear" );

  fiery_demise_window_without_soul_carver->add_action( "fiery_brand,if=!fiery_brand_dot_primary_ticking&fury>=30" );
  fiery_demise_window_without_soul_carver->add_action( "immolation_aura,if=fiery_brand_dot_primary_ticking" );
  fiery_demise_window_without_soul_carver->add_action( "fel_devastation,if=fiery_brand_dot_primary_ticking&fiery_brand_dot_primary_remains<=2" );
  fiery_demise_window_without_soul_carver->add_action( "spirit_bomb,if=!variable.pooling_fury&soul_fragments>=variable.spirit_bomb_soul_fragments&(spell_targets>1|dot.fiery_brand.ticking)" );
  fiery_demise_window_without_soul_carver->add_action( "soul_cleave,if=!variable.pooling_fury&(soul_fragments<=1&spell_targets>1|spell_targets<2)" );
  fiery_demise_window_without_soul_carver->add_action( "sigil_of_flame,if=fiery_brand_dot_primary_ticking" );
  fiery_demise_window_without_soul_carver->add_action( "fracture" );
  fiery_demise_window_without_soul_carver->add_action( "shear" );
}
//vengeance_apl_end

}