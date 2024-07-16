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
  return ( p->true_level >= 71 ) ? "main_hand:algari_mana_oil_3/off_hand:algari_mana_oil_3"
                                 : "main_hand:buzzing_rune_3/off_hand:buzzing_rune_3";
}

std::string temporary_enchant_vengeance( const player_t* p )
{
  return ( p->true_level >= 71 ) ? "main_hand:algari_mana_oil_3/off_hand:algari_mana_oil_3"
                                 : "main_hand:buzzing_rune_3/off_hand:buzzing_rune_3";
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
  cooldown->add_action( "sigil_of_spite,if=debuff.essence_break.down" );

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
  action_priority_list_t* ar = p->get_action_priority_list( "ar" );
  action_priority_list_t* ar_execute = p->get_action_priority_list( "ar_execute" );
  action_priority_list_t* externals = p->get_action_priority_list( "externals" );
  action_priority_list_t* fel_dev = p->get_action_priority_list( "fel_dev" );
  action_priority_list_t* fel_dev_prep = p->get_action_priority_list( "fel_dev_prep" );
  action_priority_list_t* fs = p->get_action_priority_list( "fs" );
  action_priority_list_t* fs_execute = p->get_action_priority_list( "fs_execute" );
  action_priority_list_t* meta_prep = p->get_action_priority_list( "meta_prep" );
  action_priority_list_t* metamorphosis = p->get_action_priority_list( "metamorphosis" );
  action_priority_list_t* rg_active = p->get_action_priority_list( "rg_active" );

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
  default_->add_action( "variable,name=num_spawnable_souls,op=max,value=2,if=talent.fracture&cooldown.fracture.charges_fractional>=1&!buff.metamorphosis.up" );
  default_->add_action( "variable,name=num_spawnable_souls,op=max,value=3,if=talent.fracture&cooldown.fracture.charges_fractional>=1&buff.metamorphosis.up" );
  default_->add_action( "variable,name=num_spawnable_souls,op=max,value=1,if=talent.soul_sigils&cooldown.sigil_of_flame.up" );
  default_->add_action( "variable,name=num_spawnable_souls,op=max,value=3+talent.soul_sigils.rank,if=talent.sigil_of_spite&cooldown.sigil_of_spite.up" );
  default_->add_action( "variable,name=num_spawnable_souls,op=max,value=floor(0.6*spell_targets.immolation_aura)>?5,if=talent.fallout&cooldown.immolation_aura.up" );
  default_->add_action( "variable,name=num_spawnable_souls,op=max,value=spell_targets.bulk_extraction>?5,if=talent.bulk_extraction&cooldown.bulk_extraction.up" );
  default_->add_action( "variable,name=num_spawnable_souls,op=max,value=3,if=talent.soul_carver&cooldown.soul_carver.up" );
  default_->add_action( "variable,name=num_spawnable_souls,op=add,value=1,if=talent.soul_carver&(cooldown.soul_carver.remains>(cooldown.soul_carver.duration-3))" );
  default_->add_action( "auto_attack" );
  default_->add_action( "retarget_auto_attack,line_cd=1,target_if=min:debuff.reavers_mark.remains,if=hero_tree.aldrachi_reaver" );
  default_->add_action( "disrupt,if=target.debuff.casting.react" );
  default_->add_action( "infernal_strike,use_off_gcd=1" );
  default_->add_action( "demon_spikes,use_off_gcd=1,if=!buff.demon_spikes.up&!cooldown.pause_action.remains" );
  default_->add_action( "run_action_list,name=ar,if=hero_tree.aldrachi_reaver" );
  default_->add_action( "run_action_list,name=fs,if=hero_tree.felscarred" );

  ar->add_action( "variable,name=spb_threshold,op=setif,condition=talent.fiery_demise&dot.fiery_brand.ticking,value=(variable.single_target*5)+(variable.small_aoe*5)+(variable.big_aoe*4),value_else=(variable.single_target*5)+(variable.small_aoe*5)+(variable.big_aoe*4)" );
  ar->add_action( "variable,name=can_spb,value=soul_fragments>=variable.spb_threshold" );
  ar->add_action( "variable,name=can_spb_soon,value=soul_fragments.total>=variable.spb_threshold" );
  ar->add_action( "variable,name=can_spb_one_gcd,value=(soul_fragments.total+variable.num_spawnable_souls)>=variable.spb_threshold" );
  ar->add_action( "variable,name=dont_soul_cleave,value=soul_fragments>=2&(variable.can_spb|variable.can_spb_soon|variable.can_spb_one_gcd)&!prev_gcd.1.sigil_of_spite&!prev_gcd.1.soul_carver" );
  ar->add_action( "variable,name=rg_enhance_cleave,op=setif,condition=variable.big_aoe|fight_remains<10,value=1,value_else=0" );
  ar->add_action( "variable,name=cooldown_sync,value=(debuff.reavers_mark.up&buff.thrill_of_the_fight_damage.up)|fight_remains<20" );
  ar->add_action( "potion,use_off_gcd=1,if=variable.cooldown_sync" );
  ar->add_action( "use_items,use_off_gcd=1,if=variable.cooldown_sync" );
  ar->add_action( "call_action_list,name=externals,if=variable.cooldown_sync" );
  ar->add_action( "immolation_aura" );
  ar->add_action( "sigil_of_flame,if=talent.ascending_flame|(!talent.ascending_flame&!prev_gcd.1.sigil_of_flame&(dot.sigil_of_flame.remains<(1+talent.quickened_sigils)))" );
  ar->add_action( "call_action_list,name=rg_active,if=buff.glaive_flurry.up|buff.rending_strike.up" );
  ar->add_action( "call_action_list,name=ar_execute,if=fight_remains<20" );
  ar->add_action( "metamorphosis,use_off_gcd=1,if=!buff.metamorphosis.up&!(cooldown.the_hunt.up|buff.reavers_glaive.up)" );
  ar->add_action( "soul_cleave,if=(debuff.reavers_mark.remains<=(gcd.remains+execute_time+(gcd.max*2)))&(buff.art_of_the_glaive.stack+soul_fragments>=30&buff.art_of_the_glaive.stack>=28)" );
  ar->add_action( "spirit_bomb,if=(debuff.reavers_mark.remains<=(gcd.remains+execute_time+(gcd.max*2)))&(buff.art_of_the_glaive.stack+soul_fragments>=30)" );
  ar->add_action( "bulk_extraction,if=(debuff.reavers_mark.remains<=(gcd.remains+execute_time+(gcd.max*2)))&(buff.art_of_the_glaive.stack+(spell_targets.bulk_extraction>?5)>=30)" );
  ar->add_action( "reavers_glaive,if=(buff.art_of_the_glaive.stack+soul_fragments>=30)|(debuff.reavers_mark.remains<=(gcd.remains+execute_time+(gcd.max*4)))|cooldown.the_hunt.remains<(gcd.remains+execute_time+(gcd.max*4))|variable.rg_enhance_cleave" );
  ar->add_action( "the_hunt,if=!buff.reavers_glaive.up" );
  ar->add_action( "fiery_brand,if=!talent.fiery_demise|(talent.fiery_demise&((talent.down_in_flames&charges>=max_charges)|(active_dot.fiery_brand=0)))" );
  ar->add_action( "fel_devastation,if=!variable.single_target&!variable.can_spb" );
  ar->add_action( "bulk_extraction,if=spell_targets>=3" );
  ar->add_action( "sigil_of_spite,if=variable.can_spb|soul_fragments.total<=2" );
  ar->add_action( "felblade,if=!variable.can_spb&variable.can_spb_soon" );
  ar->add_action( "spirit_bomb,if=variable.can_spb" );
  ar->add_action( "soul_carver,if=!prev_gcd.2.sigil_of_spite&soul_fragments<=2" );
  ar->add_action( "fracture,if=spell_targets.spirit_bomb>1&(soul_fragments>=2&soul_fragments<=3)&fury.deficit>=25" );
  ar->add_action( "fel_devastation,if=buff.thrill_of_the_fight_damage.up&(!variable.can_spb&variable.can_spb_soon)" );
  ar->add_action( "felblade,if=(fury<40&(variable.can_spb|variable.can_spb_soon))|fury<30" );
  ar->add_action( "soul_cleave,if=!variable.dont_soul_cleave&(fury>=60|fury.deficit<=30)" );
  ar->add_action( "fracture" );
  ar->add_action( "shear" );
  ar->add_action( "soul_cleave,if=!variable.dont_soul_cleave" );
  ar->add_action( "felblade" );
  ar->add_action( "throw_glaive" );

  ar_execute->add_action( "metamorphosis,use_off_gcd=1" );
  ar_execute->add_action( "reavers_glaive" );
  ar_execute->add_action( "the_hunt,if=!buff.reavers_glaive.up" );
  ar_execute->add_action( "bulk_extraction,if=spell_targets>=3&buff.art_of_the_glaive.stack>=20" );
  ar_execute->add_action( "sigil_of_flame" );
  ar_execute->add_action( "fiery_brand" );
  ar_execute->add_action( "sigil_of_spite" );
  ar_execute->add_action( "soul_carver" );
  ar_execute->add_action( "fel_devastation" );

  externals->add_action( "invoke_external_buff,name=symbol_of_hope" );
  externals->add_action( "invoke_external_buff,name=power_infusion" );

  fel_dev->add_action( "fracture,if=(!variable.can_spburst&!variable.can_spburst_soon)&buff.demonsurge_spirit_burst.up" );
  fel_dev->add_action( "spirit_burst,if=talent.spirit_bomb&(variable.can_spburst|(buff.metamorphosis.remains<(gcd.remains+execute_time+1)&buff.demonsurge_spirit_burst.up))" );
  fel_dev->add_action( "soul_sunder,if=!variable.dont_soul_cleave|(buff.metamorphosis.remains<(gcd.remains+execute_time+1)&buff.demonsurge_soul_sunder.up)" );

  fel_dev_prep->add_action( "fiery_brand,if=talent.fiery_demise&((talent.darkglare_boon&fury>=60)|(!talent.darkglare_boon&fury>=70))&(variable.can_spburst|variable.can_spburst_soon)&active_dot.fiery_brand=0&(cooldown.metamorphosis.up|cooldown.metamorphosis.remains<(gcd.remains+execute_time+action.fel_devastation.execute_time+(gcd.max*2)))" );
  fel_dev_prep->add_action( "fel_devastation,if=((talent.darkglare_boon&fury>=60)|(!talent.darkglare_boon&fury>=70))&(variable.can_spburst|variable.can_spburst_soon)" );
  fel_dev_prep->add_action( "sigil_of_spite,if=!(variable.can_spburst|variable.can_spburst_soon)&soul_fragments.total<=2" );
  fel_dev_prep->add_action( "felblade,if=!((talent.darkglare_boon&fury>=60)|(!talent.darkglare_boon&fury>=70))" );
  fel_dev_prep->add_action( "fracture,if=!(variable.can_spburst|variable.can_spburst_soon)|!((talent.darkglare_boon&fury>=60)|(!talent.darkglare_boon&fury>=70))" );
  fel_dev_prep->add_action( "felblade" );
  fel_dev_prep->add_action( "fracture" );

  fs->add_action( "variable,name=spbomb_threshold,op=setif,condition=talent.fiery_demise&dot.fiery_brand.ticking,value=(variable.single_target*5)+(variable.small_aoe*4)+(variable.big_aoe*3),value_else=(variable.single_target*5)+(variable.small_aoe*4)+(variable.big_aoe*4)" );
  fs->add_action( "variable,name=can_spbomb,value=soul_fragments>=variable.spbomb_threshold" );
  fs->add_action( "variable,name=can_spbomb_soon,value=soul_fragments.total>=variable.spbomb_threshold" );
  fs->add_action( "variable,name=can_spbomb_one_gcd,value=(soul_fragments.total+variable.num_spawnable_souls)>=variable.spbomb_threshold" );
  fs->add_action( "variable,name=spburst_threshold,op=setif,condition=talent.fiery_demise&dot.fiery_brand.ticking,value=(variable.single_target*4)+(variable.small_aoe*4)+(variable.big_aoe*3),value_else=(variable.single_target*5)+(variable.small_aoe*4)+(variable.big_aoe*3)" );
  fs->add_action( "variable,name=can_spburst,value=soul_fragments>=variable.spburst_threshold" );
  fs->add_action( "variable,name=can_spburst_soon,value=soul_fragments.total>=variable.spburst_threshold" );
  fs->add_action( "variable,name=can_spburst_one_gcd,value=(soul_fragments.total+variable.num_spawnable_souls)>=variable.spburst_threshold" );
  fs->add_action( "variable,name=dont_soul_cleave,op=setif,condition=buff.metamorphosis.up&buff.demonsurge_hardcast.up,value=((cooldown.fel_desolation.remains<=gcd.remains+execute_time)&fury<80)|(variable.can_spburst|variable.can_spburst_soon)|(prev_gcd.1.sigil_of_spite|prev_gcd.1.soul_carver),value_else=((cooldown.fel_devastation.remains<=gcd.remains+execute_time)&fury<80)|(variable.can_spbomb|variable.can_spbomb_soon)|(buff.metamorphosis.up&!buff.demonsurge_hardcast.up&buff.demonsurge_spirit_burst.up)|(prev_gcd.1.sigil_of_spite|prev_gcd.1.soul_carver)" );
  fs->add_action( "variable,name=fiery_brand_back_before_meta,op=setif,condition=talent.down_in_flames,value=charges>=max_charges|(charges_fractional>=1&cooldown.fiery_brand.full_recharge_time<=gcd.remains+execute_time)|(charges_fractional>=1&((max_charges-(charges_fractional-1))*cooldown.fiery_brand.duration)<=cooldown.metamorphosis.remains),value_else=cooldown.fiery_brand.duration<=cooldown.metamorphosis.remains" );
  fs->add_action( "variable,name=hold_sof,op=setif,condition=talent.student_of_suffering,value=(buff.student_of_suffering.remains>(1+talent.quickened_sigils))|(!talent.ascending_flame&(dot.sigil_of_flame.remains>(1+talent.quickened_sigils)))|prev_gcd.1.sigil_of_flame,value_else=cooldown.metamorphosis.up|(cooldown.sigil_of_flame.max_charges>1&talent.ascending_flame&((cooldown.sigil_of_flame.max_charges-(cooldown.sigil_of_flame.charges_fractional-1))*cooldown.sigil_of_flame.duration)>cooldown.metamorphosis.remains)|((prev_gcd.1.sigil_of_flame|dot.sigil_of_flame.remains>(1+talent.quickened_sigils)))" );
  fs->add_action( "cancel_buff,name=metamorphosis,if=!buff.demonsurge_soul_sunder.up&!buff.demonsurge_spirit_burst.up&!buff.demonsurge_fel_desolation.up&!buff.demonsurge_consuming_fire.up&!buff.demonsurge_sigil_of_doom.up&cooldown.sigil_of_doom.charges<1" );
  fs->add_action( "immolation_aura,if=!(prev_gcd.1.sigil_of_flame&cooldown.metamorphosis.up)" );
  fs->add_action( "sigil_of_flame,if=!variable.hold_sof" );
  fs->add_action( "fiery_brand,if=!talent.fiery_demise|talent.fiery_demise&((talent.down_in_flames&charges>=max_charges)|(active_dot.fiery_brand=0&variable.fiery_brand_back_before_meta))" );
  fs->add_action( "use_items,use_off_gcd=1,if=!buff.metamorphosis.up" );
  fs->add_action( "call_action_list,name=fs_execute,if=fight_remains<20" );
  fs->add_action( "call_action_list,name=fel_dev,if=buff.metamorphosis.up&!buff.demonsurge_hardcast.up&(buff.demonsurge_soul_sunder.up|buff.demonsurge_spirit_burst.up)" );
  fs->add_action( "call_action_list,name=metamorphosis,if=buff.metamorphosis.up&buff.demonsurge_hardcast.up" );
  fs->add_action( "call_action_list,name=fel_dev_prep,if=!buff.demonsurge_hardcast.up&(cooldown.fel_devastation.up|(cooldown.fel_devastation.remains<=(gcd.max*2)))" );
  fs->add_action( "call_action_list,name=meta_prep,if=(cooldown.metamorphosis.remains<=(gcd.max*3))&!cooldown.fel_devastation.up&!buff.demonsurge_soul_sunder.up&!buff.demonsurge_spirit_burst.up" );
  fs->add_action( "the_hunt" );
  fs->add_action( "soul_carver,if=(!talent.fiery_demise|talent.fiery_demise&dot.fiery_brand.ticking)&(soul_fragments.total<=2|variable.can_spbomb_one_gcd|variable.can_spburst_one_gcd)&!prev_gcd.1.sigil_of_spite" );
  fs->add_action( "sigil_of_spite,if=(soul_fragments.total<=2|variable.can_spbomb_one_gcd|variable.can_spburst_one_gcd)" );
  fs->add_action( "bulk_extraction,if=(variable.can_spburst|variable.can_spburst_soon)&!buff.soul_furnace_damage_amp.up&buff.soul_furnace_stack.stack<=6&(buff.soul_furnace_stack.stack+(spell_targets.bulk_extraction>?5)>=10)&cooldown.bulk_extraction.duration<(cooldown.metamorphosis.remains+15)" );
  fs->add_action( "felblade,if=(!variable.can_spbomb&variable.can_spbomb_soon)|(buff.metamorphosis.up&(!variable.can_spburst&variable.can_spburst_soon))" );
  fs->add_action( "spirit_burst,if=talent.spirit_bomb&variable.can_spburst" );
  fs->add_action( "spirit_bomb,if=variable.can_spbomb" );
  fs->add_action( "fracture,if=((fury<40&fury>=15)&((buff.metamorphosis.up&(variable.can_spburst|variable.can_spburst_soon))|(!buff.metamorphosis.up&(variable.can_spbomb|variable.can_spbomb_soon))))|((buff.metamorphosis.up&variable.can_spburst_one_gcd)|(!buff.metamorphosis.up&variable.can_spbomb_one_gcd))" );
  fs->add_action( "felblade,if=(fury<40&((buff.metamorphosis.up&(variable.can_spburst|variable.can_spburst_soon))|(!buff.metamorphosis.up&(variable.can_spbomb|variable.can_spbomb_soon))))|fury<30" );
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
  meta_prep->add_action( "fiery_brand,if=talent.fiery_demise&active_dot.fiery_brand=0" );
  meta_prep->add_action( "potion,use_off_gcd=1" );
  meta_prep->add_action( "sigil_of_flame" );

  metamorphosis->add_action( "call_action_list,name=externals" );
  metamorphosis->add_action( "spirit_burst,if=talent.spirit_bomb&(buff.metamorphosis.remains<(gcd.remains+execute_time+1))&buff.demonsurge_spirit_burst.up" );
  metamorphosis->add_action( "spirit_burst,if=talent.spirit_bomb&variable.can_spburst&buff.demonsurge_spirit_burst.up" );
  metamorphosis->add_action( "sigil_of_spite,if=soul_fragments.total<=2" );
  metamorphosis->add_action( "soul_carver,if=soul_fragments.total<=2&!prev_gcd.1.sigil_of_spite" );
  metamorphosis->add_action( "fel_desolation" );
  metamorphosis->add_action( "sigil_of_doom,if=talent.ascending_flame|(!talent.ascending_flame&(dot.sigil_of_doom.remains<(1+talent.quickened_sigils)&!prev_gcd.1.sigil_of_doom))" );
  metamorphosis->add_action( "bulk_extraction,if=(variable.can_spburst|variable.can_spburst_soon)&!buff.soul_furnace_damage_amp.up&buff.soul_furnace_stack.stack<=6&buff.soul_furnace_stack.stack+(spell_targets.bulk_extraction>?5)>=10" );
  metamorphosis->add_action( "fracture,if=variable.big_aoe&(soul_fragments>=2&soul_fragments<=3)" );
  metamorphosis->add_action( "felblade,if=(fury<40&(variable.can_spburst|variable.can_spburst_soon))|fury<30" );
  metamorphosis->add_action( "spirit_burst,if=talent.spirit_bomb&variable.can_spburst" );
  metamorphosis->add_action( "soul_sunder,if=!variable.dont_soul_cleave" );
  metamorphosis->add_action( "felblade" );
  metamorphosis->add_action( "fracture" );

  rg_active->add_action( "metamorphosis,use_off_gcd=1,if=!buff.metamorphosis.up&(buff.rending_strike.up&!buff.glaive_flurry.up)&soul_fragments<=1" );
  rg_active->add_action( "felblade,if=fury<30&!variable.rg_enhance_cleave&buff.rending_strike.up&buff.glaive_flurry.up" );
  rg_active->add_action( "the_hunt,if=!buff.reavers_glaive.up&(debuff.reavers_mark.remains>(gcd.remains+execute_time+action.soul_cleave.execute_time+(talent.fracture&action.fracture.execute_time|!talent.fracture&action.shear.execute_time)+gcd.max))" );
  rg_active->add_action( "fracture,if=variable.rg_enhance_cleave&buff.rending_strike.up&buff.glaive_flurry.up|!variable.rg_enhance_cleave&!buff.glaive_flurry.up" );
  rg_active->add_action( "shear,if=variable.rg_enhance_cleave&buff.rending_strike.up&buff.glaive_flurry.up|!variable.rg_enhance_cleave&!buff.glaive_flurry.up" );
  rg_active->add_action( "soul_cleave,if=!variable.rg_enhance_cleave&buff.glaive_flurry.up&buff.rending_strike.up|variable.rg_enhance_cleave&!buff.rending_strike.up" );
}
//vengeance_apl_end

}  // namespace demon_hunter_apl
