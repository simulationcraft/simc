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

std::string temporary_enchant( const player_t* p )
{
  return ( ( p->true_level >= 61 ) ? "main_hand:buzzing_rune_3/off_hand:buzzing_rune_3" :
           ( p->true_level >= 51 ) ? "main_hand:shaded_sharpening_stone/off_hand:shaded_sharpening_stone" :
           "disabled" );
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
  default_->add_action( "essence_break,if=!variable.waiting_for_momentum&(cooldown.eye_beam.remains>4|buff.metamorphosis.up)&(!talent.tactical_retreat|buff.tactical_retreat.up)" );
  default_->add_action( "essence_break,if=(active_enemies>desired_targets|raid_event.adds.in>40)&!variable.waiting_for_momentum&fury>50&(cooldown.eye_beam.remains>8|buff.metamorphosis.up)&(!talent.tactical_retreat|buff.tactical_retreat.up)" );
  default_->add_action( "death_sweep,if=variable.blade_dance&(!talent.essence_break|cooldown.essence_break.remains>(cooldown.death_sweep.duration-4))" );
  default_->add_action( "fel_barrage,if=active_enemies>desired_targets|raid_event.adds.in>30" );
  default_->add_action( "glaive_tempest,if=active_enemies>desired_targets|raid_event.adds.in>10" );
  default_->add_action( "eye_beam,if=active_enemies>desired_targets|raid_event.adds.in>(40-talent.cycle_of_hatred*15)&!debuff.essence_break.up" );
  default_->add_action( "blade_dance,if=variable.blade_dance&!cooldown.metamorphosis.ready&(cooldown.eye_beam.remains>5|!talent.demonic|(raid_event.adds.in>cooldown&raid_event.adds.in<25))" );
  default_->add_action( "throw_glaive,if=talent.soulrend&spell_targets>=(2-talent.furious_throws)&!debuff.essence_break.up" );
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
  cooldown->add_action( "potion,if=buff.metamorphosis.remains>25|fight_remains<60" );
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
  action_priority_list_t* brand = p->get_action_priority_list( "brand" );
  action_priority_list_t* defensives = p->get_action_priority_list( "defensives" );
  action_priority_list_t* cooldowns = p->get_action_priority_list( "cooldowns" );
  action_priority_list_t* normal = p->get_action_priority_list( "normal" );

  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  default_->add_action( "auto_attack" );
  default_->add_action( "variable,name=brand_build,value=talent.fiery_demise" );
  default_->add_action( "disrupt" );
  default_->add_action( "call_action_list,name=brand,if=variable.brand_build" );
  default_->add_action( "call_action_list,name=defensives" );
  default_->add_action( "call_action_list,name=cooldowns" );
  default_->add_action( "call_action_list,name=normal" );

  brand->add_action( "fiery_brand" );
  brand->add_action( "immolation_aura,if=dot.fiery_brand.ticking" );
  brand->add_action( "soul_carver,if=dot.fiery_brand.ticking" );

  defensives->add_action( "demon_spikes" );
  defensives->add_action( "metamorphosis,if=!buff.metamorphosis.up|target.time_to_die<15" );
  defensives->add_action( "fiery_brand" );

  cooldowns->add_action( "potion" );
  cooldowns->add_action( "use_items" );
  cooldowns->add_action( "the_hunt" );
  cooldowns->add_action( "elysian_decree" );
  cooldowns->add_action( "soul_carver" );

  normal->add_action( "infernal_strike" );
  normal->add_action( "bulk_extraction" );
  normal->add_action( "spirit_bomb,if=((buff.metamorphosis.up&talent.fracture.enabled&soul_fragments>=3)|soul_fragments>=4)" );
  normal->add_action( "fel_devastation" );
  normal->add_action( "soul_cleave,if=((talent.spirit_bomb.enabled&soul_fragments=0)|!talent.spirit_bomb.enabled)&((talent.fracture.enabled&fury>=55)|(!talent.fracture.enabled&fury>=70)|cooldown.fel_devastation.remains>target.time_to_die|(buff.metamorphosis.up&((talent.fracture.enabled&fury>=35)|(!talent.fracture.enabled&fury>=50))))" );
  normal->add_action( "immolation_aura,if=((variable.brand_build&cooldown.fiery_brand.remains>10)|!variable.brand_build)&fury.deficit>=10" );
  normal->add_action( "felblade,if=fury.deficit>=40" );
  normal->add_action( "fracture,if=((talent.spirit_bomb.enabled&soul_fragments<=3)|(!talent.spirit_bomb.enabled&((buff.metamorphosis.up&fury.deficit>=45)|(buff.metamorphosis.down&fury.deficit>=30))))" );
  normal->add_action( "sigil_of_flame" );
  normal->add_action( "shear" );
  normal->add_action( "throw_glaive" );
}
//vengeance_apl_end

}