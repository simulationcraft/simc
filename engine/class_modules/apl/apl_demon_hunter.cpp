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

std::string flask( const player_t* p )
{
  return ( ( p->true_level >= 61 ) ? "phial_of_elemental_chaos_3" :
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

  default_->add_action( "auto_attack" );
  default_->add_action( "retarget_auto_attack,line_cd=1,target_if=min:debuff.burning_wound.remains,if=talent.burning_wound&talent.demon_blades&active_dot.burning_wound<(spell_targets>?3)" );
  default_->add_action( "immolation_aura,if=time=0", "Precombat Immolation Aura" );
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
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "variable,name=the_hunt_ramp_in_progress,value=0" );
  precombat->add_action( "variable,name=elysian_decree_ramp_in_progress,value=0" );
  precombat->add_action( "variable,name=soul_carver_ramp_in_progress,value=0" );
  precombat->add_action( "variable,name=fiery_demise_with_soul_carver_in_progress,value=0" );
  precombat->add_action( "variable,name=fiery_demise_without_soul_carver_available,value=0" );
  precombat->add_action( "variable,name=darkglare_boon_high_roll,value=0", "A 'high-roll' from Darkglare boon is any roll that gives us an 18+s reduction on Fel Dev's cooldown." );

  default_->add_action( "auto_attack" );
  default_->add_action( "infernal_strike" );
  default_->add_action( "demon_spikes,if=!buff.demon_spikes.up&!cooldown.pause_action.remains" );
  default_->add_action( "fiery_brand,if=!talent.fiery_demise.enabled&!dot.fiery_brand.ticking" );
  default_->add_action( "bulk_extraction" );
  default_->add_action( "potion" );
  default_->add_action( "use_item,slot=trinket1" );
  default_->add_action( "use_item,slot=trinket2" );
  default_->add_action( "run_action_list,name=the_hunt_ramp,if=variable.the_hunt_ramp_in_progress|talent.the_hunt.enabled&cooldown.the_hunt.remains<5&!dot.fiery_brand.ticking" );
  default_->add_action( "run_action_list,name=elysian_decree_ramp,if=variable.elysian_decree_ramp_in_progress|talent.elysian_decree.enabled&cooldown.elysian_decree.remains<5&!dot.fiery_brand.ticking" );
  default_->add_action( "run_action_list,name=soul_carver_without_fiery_demise_ramp,if=variable.soul_carver_ramp_in_progress|talent.soul_carver.enabled&cooldown.soul_carver.remains<5&!talent.fiery_demise.enabled&!dot.fiery_brand.ticking" );
  default_->add_action( "run_action_list,name=fiery_demise_window_with_soul_carver,if=variable.fiery_demise_with_soul_carver_in_progress|talent.fiery_demise.enabled&talent.soul_carver.enabled&cooldown.soul_carver.up&cooldown.fiery_brand.up&cooldown.immolation_aura.up&cooldown.fel_devastation.remains<10" );
  default_->add_action( "run_action_list,name=fiery_demise_window_without_soul_carver,if=variable.fiery_demise_without_soul_carver_in_progress|talent.fiery_demise.enabled&((talent.soul_carver.enabled&!cooldown.soul_carver.up)|!talent.soul_carver.enabled)&cooldown.fiery_brand.up&cooldown.immolation_aura.up&cooldown.fel_devastation.remains<10&((talent.darkglare_boon.enabled&variable.darkglare_boon_high_roll)|!talent.darkglare_boon.enabled)" );
  default_->add_action( "metamorphosis,if=!buff.metamorphosis.up&!dot.fiery_brand.ticking" );
  default_->add_action( "fel_devastation,if=!talent.down_in_flames.enabled" );
  default_->add_action( "spirit_bomb,if=((buff.metamorphosis.up&talent.fracture.enabled&soul_fragments>=3&spell_targets>1)|soul_fragments>=4&spell_targets>1)" );
  default_->add_action( "soul_cleave,if=(talent.spirit_bomb.enabled&soul_fragments<=1&spell_targets>1)|(spell_targets<2&((talent.fracture.enabled&fury>=55)|(!talent.fracture.enabled&fury>=70)|(buff.metamorphosis.up&((talent.fracture.enabled&fury>=35)|(!talent.fracture.enabled&fury>=50)))))|(!talent.spirit_bomb.enabled)&((talent.fracture.enabled&fury>=55)|(!talent.fracture.enabled&fury>=70)|(buff.metamorphosis.up&((talent.fracture.enabled&fury>=35)|(!talent.fracture.enabled&fury>=50))))" );
  default_->add_action( "immolation_aura,if=(talent.fiery_demise.enabled&fury.deficit>=10&(cooldown.soul_carver.remains>15))|(!talent.fiery_demise.enabled&fury.deficit>=10)" );
  default_->add_action( "felblade,if=fury.deficit>=40" );
  default_->add_action( "fracture,if=(talent.spirit_bomb.enabled&(soul_fragments<=3&spell_targets>1|spell_targets<2&fury.deficit>=30))|(!talent.spirit_bomb.enabled&((buff.metamorphosis.up&fury.deficit>=45)|(buff.metamorphosis.down&fury.deficit>=30)))" );
  default_->add_action( "sigil_of_flame,if=fury.deficit>=30" );
  default_->add_action( "shear" );
  default_->add_action( "throw_glaive" );

  the_hunt_ramp->add_action( "variable,name=the_hunt_ramp_in_progress,value=1,if=!variable.the_hunt_ramp_in_progress" );
  the_hunt_ramp->add_action( "variable,name=the_hunt_ramp_in_progress,value=0,if=cooldown.the_hunt.remains" );
  the_hunt_ramp->add_action( "fracture,if=fury.deficit>=30&debuff.frailty.stack<=5" );
  the_hunt_ramp->add_action( "sigil_of_flame,if=fury.deficit>=30" );
  the_hunt_ramp->add_action( "shear,if=fury.deficit<=90" );
  the_hunt_ramp->add_action( "spirit_bomb,if=soul_fragments>=4&spell_targets>1" );
  the_hunt_ramp->add_action( "soul_cleave,if=(soul_fragments<=1&spell_targets>1)|(spell_targets<2)|debuff.frailty.stack>=0" );
  the_hunt_ramp->add_action( "the_hunt" );

  elysian_decree_ramp->add_action( "variable,name=elysian_decree_ramp_in_progress,value=1,if=!variable.elysian_decree_ramp_in_progress" );
  elysian_decree_ramp->add_action( "variable,name=elysian_decree_ramp_in_progress,value=0,if=cooldown.elysian_decree.remains" );
  elysian_decree_ramp->add_action( "fracture,if=fury.deficit>=30&debuff.frailty.stack<=5" );
  elysian_decree_ramp->add_action( "sigil_of_flame,if=fury.deficit>=30" );
  elysian_decree_ramp->add_action( "shear,if=fury.deficit<=90&debuff.frailty.stack>=0" );
  elysian_decree_ramp->add_action( "spirit_bomb,if=soul_fragments>=4&spell_targets>1" );
  elysian_decree_ramp->add_action( "soul_cleave,if=(soul_fragments<=1&spell_targets>1)|(spell_targets<2)|debuff.frailty.stack>=0" );
  elysian_decree_ramp->add_action( "elysian_decree" );

  soul_carver_without_fiery_demise_ramp->add_action( "variable,name=soul_carver_ramp_in_progress,value=1,if=!variable.soul_carver_ramp_in_progress" );
  soul_carver_without_fiery_demise_ramp->add_action( "variable,name=soul_carver_ramp_in_progress,value=0,if=cooldown.soul_carver.remains" );
  soul_carver_without_fiery_demise_ramp->add_action( "fracture,if=fury.deficit>=30&debuff.frailty.stack<=5" );
  soul_carver_without_fiery_demise_ramp->add_action( "sigil_of_flame,if=fury.deficit>=30" );
  soul_carver_without_fiery_demise_ramp->add_action( "shear,if=fury.deficit<=90&debuff.frailty.stack>=0" );
  soul_carver_without_fiery_demise_ramp->add_action( "spirit_bomb,if=soul_fragments>=4&spell_targets>1" );
  soul_carver_without_fiery_demise_ramp->add_action( "soul_cleave,if=(soul_fragments<=1&spell_targets>1)|(spell_targets<2)|debuff.frailty.stack>=0" );
  soul_carver_without_fiery_demise_ramp->add_action( "soul_carver" );

  fiery_demise_window_with_soul_carver->add_action( "variable,name=fiery_demise_with_soul_carver_in_progress,value=1,if=!variable.fiery_demise_with_soul_carver_in_progress" );
  fiery_demise_window_with_soul_carver->add_action( "variable,name=fiery_demise_with_soul_carver_in_progress,value=0,if=cooldown.soul_carver.remains&cooldown.fiery_brand.remains&cooldown.immolation_aura.remains&cooldown.fel_devastation.remains" );
  fiery_demise_window_with_soul_carver->add_action( "variable,name=darkglare_boon_high_roll,op=setif,value=1,value_else=0,condition=darkglare_boon_cdr_roll>=18,if=prev_gcd.1.fel_devastation", "A 'high-roll' from Darkglare boon is any roll that gives us an 18+s reduction on Fel Dev's cooldown." );
  fiery_demise_window_with_soul_carver->add_action( "fracture,if=fury.deficit>=30&!dot.fiery_brand.ticking" );
  fiery_demise_window_with_soul_carver->add_action( "fiery_brand,if=!dot.fiery_brand.ticking&fury>=30" );
  fiery_demise_window_with_soul_carver->add_action( "fel_devastation,if=dot.fiery_brand.remains<=3" );
  fiery_demise_window_with_soul_carver->add_action( "immolation_aura,if=dot.fiery_brand.ticking" );
  fiery_demise_window_with_soul_carver->add_action( "spirit_bomb,if=soul_fragments>=4&dot.fiery_brand.remains>=4" );
  fiery_demise_window_with_soul_carver->add_action( "soul_carver,if=soul_fragments<=3" );
  fiery_demise_window_with_soul_carver->add_action( "fracture,if=soul_fragments<=3&dot.fiery_brand.remains>=5|dot.fiery_brand.remains<=5&fury<50" );
  fiery_demise_window_with_soul_carver->add_action( "sigil_of_flame,if=dot.fiery_brand.remains<=3&fury<50" );
  fiery_demise_window_with_soul_carver->add_action( "throw_glaive" );

  fiery_demise_window_without_soul_carver->add_action( "variable,name=fiery_demise_without_soul_carver_in_progress,value=1,if=!variable.fiery_demise_without_soul_carver_in_progress" );
  fiery_demise_window_without_soul_carver->add_action( "variable,name=fiery_demise_without_soul_carver_in_progress,value=0,if=cooldown.fiery_brand.remains&cooldown.immolation_aura.remains&cooldown.fel_devastation.remains" );
  fiery_demise_window_without_soul_carver->add_action( "variable,name=darkglare_boon_high_roll,op=setif,value=1,value_else=0,condition=darkglare_boon_cdr_roll>=18,if=prev_gcd.1.fel_devastation" );
  fiery_demise_window_without_soul_carver->add_action( "fracture,if=fury.deficit>=30&!dot.fiery_brand.ticking" );
  fiery_demise_window_without_soul_carver->add_action( "fiery_brand,if=!dot.fiery_brand.ticking&fury>=30" );
  fiery_demise_window_without_soul_carver->add_action( "immolation_aura,if=dot.fiery_brand.ticking" );
  fiery_demise_window_without_soul_carver->add_action( "spirit_bomb,if=soul_fragments>=4&dot.fiery_brand.remains>=4" );
  fiery_demise_window_without_soul_carver->add_action( "fracture,if=soul_fragments<=3&dot.fiery_brand.remains>=5|dot.fiery_brand.remains<=5&fury<50" );
  fiery_demise_window_without_soul_carver->add_action( "fel_devastation,if=dot.fiery_brand.remains<=3" );
  fiery_demise_window_without_soul_carver->add_action( "sigil_of_flame,if=dot.fiery_brand.remains<=3&fury<50" );
}
//vengeance_apl_end

}