#include "class_modules/apl/mage.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace mage_apl {

std::string potion( const player_t* p )
{
  return p->true_level >= 80 ? "tempered_potion_3"
       : p->true_level >= 70 ? "elemental_potion_of_ultimate_power_3"
       : p->true_level >= 60 ? "spectral_intellect"
       : p->true_level >= 50 ? "superior_battle_potion_of_intellect"
       :                       "disabled";
}

std::string flask( const player_t* p )
{
  return p->true_level >= 80 ? "flask_of_alchemical_chaos_3"
       : p->true_level >= 70 ? "phial_of_tepid_versatility_3"
       : p->true_level >= 60 ? "spectral_flask_of_power"
       : p->true_level >= 50 ? "greater_flask_of_endless_fathoms"
       :                       "disabled";
}

std::string food( const player_t* p )
{
  return p->true_level >= 80 ? "feast_of_the_midnight_masquerade"
       : p->true_level >= 70 ? "fated_fortune_cookie"
       : p->true_level >= 60 ? "feast_of_gluttonous_hedonism"
       : p->true_level >= 50 ? "famine_evaluator_and_snack_table"
       :                       "disabled";
}

std::string rune( const player_t* p )
{
  return p->true_level >= 80 ? "crystallized"
       : p->true_level >= 70 ? "draconic"
       : p->true_level >= 60 ? "veiled"
       : p->true_level >= 50 ? "battle_scarred"
       :                       "disabled";
}

std::string temporary_enchant( const player_t* p )
{
  return p->true_level >= 80 ? "main_hand:algari_mana_oil_3"
       : p->true_level >= 70 ? "main_hand:buzzing_rune_3"
       : p->true_level >= 60 ? "main_hand:shadowcore_oil"
       :                       "disabled";
}

//arcane_apl_start
void arcane( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cooldowns = p->get_action_priority_list( "cooldowns" );
  action_priority_list_t* spellslinger = p->get_action_priority_list( "spellslinger" );
  action_priority_list_t* sunfury = p->get_action_priority_list( "sunfury" );

  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "variable,name=opener,op=set,value=1" );
  precombat->add_action( "variable,name=20ssteroid_trinket_equipped,op=set,value=equipped.signet_of_the_priory" );
  precombat->add_action( "variable,name=15ssteroid_trinket_equipped,op=set,value=equipped.lily_of_the_eternal_weave|equipped.sunblood_amethyst|equipped.astral_gladiators_badge_of_ferocity|equipped.arazs_ritual_forge" );
  precombat->add_action( "variable,name=12ssteroid_trinket_equipped,op=set,value=0" );
  precombat->add_action( "variable,name=nonsteroid_trinket_equipped,op=set,value=equipped.mereldars_toll|equipped.perfidious_projector|equipped.chaotic_nethergate" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "mirror_image" );
  precombat->add_action( "arcane_surge" );

  default_->add_action( "counterspell" );
  default_->add_action( "potion,if=buff.arcane_surge.up&(prev_gcd.1.arcane_surge|fight_remains<90|variable.opener)|fight_remains<30" );
  default_->add_action( "berserking,if=(buff.arcane_surge.up&debuff.touch_of_the_magi.up)|fight_remains<13" );
  default_->add_action( "blood_fury,if=(buff.arcane_surge.up&(debuff.touch_of_the_magi.up&talent.splintering_sorcery|(talent.spellfire_spheres&buff.arcane_surge.up&buff.arcane_surge.remains<(9+gcd.remains))))|fight_remains<16" );
  default_->add_action( "fireblood,if=(buff.arcane_surge.up&((debuff.touch_of_the_magi.up&talent.splintering_sorcery)|(talent.spellfire_spheres&buff.arcane_surge.up&buff.arcane_surge.remains<(2+gcd.remains))))|fight_remains<9" );
  default_->add_action( "ancestral_call,if=(buff.arcane_surge.up&(debuff.touch_of_the_magi.up&talent.splintering_sorcery|(talent.spellfire_spheres&buff.arcane_surge.up&buff.arcane_surge.remains<(9+gcd.remains))))|fight_remains<16" );
  default_->add_action( "use_items,if=(talent.splintering_sorcery&((buff.arcane_surge.up&((variable.12ssteroid_trinket_equipped&debuff.touch_of_the_magi.up)|variable.15ssteroid_trinket_equipped))|(cooldown.arcane_surge.ready&variable.20ssteroid_trinket_equipped)))|(talent.spellfire_spheres&buff.arcane_surge.up&((variable.12ssteroid_trinket_equipped&debuff.touch_of_the_magi.up)|(variable.15ssteroid_trinket_equipped&buff.arcane_surge.remains<9+gcd.remains))|(buff.arcane_surge.remains<14&variable.20ssteroid_trinket_equipped))|(fight_remains<13&variable.12ssteroid_trinket_equipped)|(fight_remains<16&variable.15ssteroid_trinket_equipped)|(fight_remains<21&variable.20ssteroid_trinket_equipped)" );
  default_->add_action( "arcane_missiles,if=fight_remains<(gcd.max*(2+buff.clearcasting.react))&buff.clearcasting.react&buff.arcane_salvo.stack>=13+(5*talent.spellfire_salvo),interrupt_if=!talent.high_voltage&tick_time>gcd.remains&buff.overpowered_missiles.react=0,interrupt_immediate=1,interrupt_global=1,chain=1" );
  default_->add_action( "arcane_barrage,if=fight_remains<gcd.max*2" );
  default_->add_action( "variable,name=opener,op=set,if=debuff.touch_of_the_magi.up&variable.opener,value=0" );
  default_->add_action( "call_action_list,name=cooldowns" );
  default_->add_action( "call_action_list,name=spellslinger,if=!talent.spellfire_spheres" );
  default_->add_action( "call_action_list,name=sunfury" );
  default_->add_action( "arcane_barrage" );

  cooldowns->add_action( "arcane_missiles,if=variable.opener&talent.splintering_sorcery" );
  cooldowns->add_action( "wait,sec=0.05,if=variable.opener&talent.splintering_sorcery,line_cd=15" );
  cooldowns->add_action( "touch_of_the_magi,use_off_gcd=1,if=(talent.splintering_sorcery&buff.arcane_surge.up)|(talent.spellfire_spheres&buff.arcane_surge.up&buff.arcane_surge.remains<(4+gcd.remains))|(cooldown.touch_of_the_magi.ready&cooldown.arcane_surge.remains>30&buff.arcane_surge.down)" );
  cooldowns->add_action( "arcane_surge" );
  cooldowns->add_action( "evocation,if=mana.pct<10&buff.arcane_surge.down&debuff.touch_of_the_magi.down&cooldown.arcane_surge.remains>10" );

  spellslinger->add_action( "arcane_orb,if=buff.arcane_charge.stack<2&(buff.clearcasting.react=0|!talent.high_voltage)" );
  spellslinger->add_action( "arcane_barrage,if=buff.arcane_salvo.react=20&(buff.arcane_charge.stack=4|talent.orb_barrage)&cooldown.touch_of_the_magi.remains>gcd.max*4" );
  spellslinger->add_action( "arcane_barrage,if=buff.arcane_charge.stack=4&((buff.clearcasting.react&talent.high_voltage)|cooldown.arcane_orb.charges_fractional>0.95)&buff.arcane_salvo.react<8&active_enemies>3&cooldown.touch_of_the_magi.remains>gcd.max*4" );
  spellslinger->add_action( "arcane_barrage,if=buff.arcane_salvo.react>5&buff.clearcasting.react&buff.arcane_charge.stack=4&cooldown.touch_of_the_magi.remains>gcd.max*4&buff.overpowered_missiles.react&talent.high_voltage" );
  spellslinger->add_action( "arcane_missiles,if=buff.clearcasting.react&buff.arcane_surge.up&((buff.arcane_charge.stack<3&talent.high_voltage)|buff.clearcasting.react=3)&(buff.arcane_salvo.react<14|(buff.overpowered_missiles.react&buff.arcane_salvo.react<6)),interrupt_if=tick_time>gcd.remains&buff.overpowered_missiles.react=0&buff.arcane_surge.up,interrupt_immediate=1,interrupt_global=1,chain=1" );
  spellslinger->add_action( "arcane_missiles,if=buff.clearcasting.react&buff.arcane_surge.down&((buff.arcane_charge.stack<3&talent.high_voltage)|buff.clearcasting.react>=1)&(buff.arcane_salvo.react<14|(buff.overpowered_missiles.react&buff.arcane_salvo.react<6)),chain=1" );
  spellslinger->add_action( "presence_of_mind,use_off_gcd=1,if=buff.arcane_charge.stack<2&!variable.opener" );
  spellslinger->add_action( "arcane_pulse,if=active_enemies>3" );
  spellslinger->add_action( "arcane_blast" );
  spellslinger->add_action( "arcane_barrage" );

  sunfury->add_action( "arcane_barrage,if=buff.arcane_soul.up" );
  sunfury->add_action( "arcane_missiles,if=buff.clearcasting.react&buff.arcane_surge.up&buff.arcane_surge.remains<action.arcane_missiles.execute_time,interrupt_if=tick_time>gcd.remains,interrupt_immediate=1,interrupt_global=1,chain=1" );
  sunfury->add_action( "arcane_barrage,if=prev_off_gcd.touch_of_the_magi|(debuff.touch_of_the_magi.remains<gcd.max&debuff.touch_of_the_magi.up&buff.arcane_charge.stack=4)" );
  sunfury->add_action( "arcane_missiles,if=buff.clearcasting.react&buff.arcane_surge.up&buff.arcane_surge.remains<5&(buff.clearcasting.react>1|(buff.arcane_charge.stack<4&talent.high_voltage)),interrupt_if=tick_time>gcd.remains&buff.overpowered_missiles.react=0,interrupt_immediate=1,interrupt_global=1,chain=1" );
  sunfury->add_action( "arcane_barrage,if=buff.arcane_charge.stack=4&(buff.arcane_salvo.react=25|(cooldown.arcane_orb.charges_fractional>0.95&active_enemies>=3&buff.arcane_salvo.react<16)|(talent.high_voltage&buff.clearcasting.react&buff.arcane_salvo.react<18))&((buff.arcane_surge.down&cooldown.touch_of_the_magi.remains>gcd.max*4&cooldown.arcane_surge.remains>gcd.max*4)|(buff.arcane_surge.up&((buff.arcane_surge.remains>gcd.max*9&buff.clearcasting.react)|(buff.arcane_surge.remains>gcd.max*5&buff.clearcasting.react&buff.overpowered_missiles.react))))" );
  sunfury->add_action( "arcane_missiles,if=buff.clearcasting.react&((buff.arcane_charge.stack<2&talent.high_voltage)|(buff.overpowered_missiles.react=0&buff.arcane_salvo.react<16))" );
  sunfury->add_action( "arcane_orb,if=buff.arcane_charge.stack<2" );
  sunfury->add_action( "arcane_pulse,if=active_enemies>3" );
  sunfury->add_action( "arcane_explosion,if=active_enemies>3&buff.arcane_charge.stack<2&!talent.impetus" );
  sunfury->add_action( "arcane_blast" );
}
//arcane_apl_end

//fire_apl_start
void fire( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* ff_combustion = p->get_action_priority_list( "ff_combustion" );
  action_priority_list_t* ff_filler = p->get_action_priority_list( "ff_filler" );
  action_priority_list_t* sf_combustion = p->get_action_priority_list( "sf_combustion" );
  action_priority_list_t* sf_filler = p->get_action_priority_list( "sf_filler" );

  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "variable,name=cast_remains_time,value=0.2" );
  precombat->add_action( "variable,name=pooling_time,value=15*gcd.max" );
  precombat->add_action( "variable,name=ff_combustion_flamestrike,if=talent.frostfire_bolt,value=100" );
  precombat->add_action( "variable,name=ff_filler_flamestrike,if=talent.frostfire_bolt,value=100" );
  precombat->add_action( "variable,name=sf_combustion_flamestrike,if=talent.spellfire_spheres,value=100-(50*talent.mark_of_the_firelord)-(44*talent.quickflame)" );
  precombat->add_action( "variable,name=sf_filler_flamestrike,if=talent.spellfire_spheres,value=100-(50*talent.mark_of_the_firelord)-(42*talent.quickflame)" );
  precombat->add_action( "variable,name=treacherous_transmitter_precombat_cast,value=12,if=equipped.treacherous_transmitter" );
  precombat->add_action( "use_item,name=treacherous_transmitter" );
  precombat->add_action( "use_item,name=ingenious_mana_battery,target=self" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "mirror_image" );
  precombat->add_action( "frostfire_bolt,if=talent.frostfire_bolt" );
  precombat->add_action( "combustion,if=!talent.firestarter" );
  precombat->add_action( "pyroblast" );

  default_->add_action( "call_action_list,name=cds,if=!(buff.hot_streak.up&prev_gcd.1.scorch)" );
  default_->add_action( "run_action_list,name=ff_combustion,if=talent.frostfire_bolt&!buff.hyperthermia.react&(cooldown.combustion.remains<=variable.combustion_precast_time|buff.combustion.up)" );
  default_->add_action( "run_action_list,name=sf_combustion,if=!buff.hyperthermia.react&!(buff.hyperthermia.up&buff.lesser_time_warp.up)&(cooldown.combustion.remains<=variable.combustion_precast_time|buff.combustion.up)" );
  default_->add_action( "run_action_list,name=ff_filler,if=talent.frostfire_bolt" );
  default_->add_action( "run_action_list,name=sf_filler" );

  cds->add_action( "variable,name=combustion_precast_time,value=talent.frostfire_bolt*(action.fireball.cast_time*(!improved_scorch.active|(action.scorch.cast_time<gcd.max))+action.scorch.cast_time*((action.scorch.cast_time>=gcd.max)&improved_scorch.active))+talent.spellfire_spheres*(action.fireball.cast_time*((action.scorch.cast_time<gcd.max)&buff.hot_streak.react|!talent.scorch)+action.scorch.cast_time*((action.scorch.cast_time>=gcd.max)|!buff.hot_streak.react))-variable.cast_remains_time" );
  cds->add_action( "potion,if=time=0|buff.combustion.remains>6|fight_remains<35" );
  cds->add_action( "use_item,name=arazs_ritual_forge,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=neural_synapse_enhancer,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,effect_name=gladiators_badge,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=signet_of_the_priory,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=sunblood_amethyst,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=lily_of_the_eternal_weave,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=funhouse_lens,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=mereldars_toll,if=buff.combustion.remains>6|fight_remains<15" );
  cds->add_action( "use_item,name=flarendos_pilot_light,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=house_of_cards,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=soulletting_ruby,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=quickwick_candlestick,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=hyperthread_wristwraps,if=hyperthread_wristwraps.fire_blast>=2&buff.combustion.remains&action.fire_blast.charges=0" );
  cds->add_action( "use_items" );
  cds->add_action( "ancestral_call,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "berserking,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "blood_fury,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "fireblood,if=buff.combustion.remains>6|fight_remains<10" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.power_infusion.down&(buff.combustion.remains>6|fight_remains<25)" );
  cds->add_action( "invoke_external_buff,name=blessing_of_summer,if=buff.blessing_of_summer.down" );

  ff_combustion->add_action( "combustion,use_off_gcd=1,use_while_casting=1,if=buff.combustion.down&action.fireball.executing&(action.fireball.execute_remains<variable.cast_remains_time)|action.meteor.in_flight&(action.meteor.in_flight_remains<variable.cast_remains_time)|action.pyroblast.executing&(action.pyroblast.execute_remains<variable.cast_remains_time)|action.scorch.executing&(action.scorch.execute_remains<variable.cast_remains_time)" );
  ff_combustion->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&buff.combustion.up&!action.scorch.executing&!action.fireball.executing&!action.pyroblast.executing&!buff.hot_streak.react&gcd.remains&gcd.remains<gcd.max&(hot_streak_spells_in_flight+buff.heating_up.react)<2&!buff.fury_of_the_sun_king.up" );
  ff_combustion->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&buff.combustion.up&buff.heating_up.react&action.pyroblast.executing&action.pyroblast.execute_remains<0.5" );
  ff_combustion->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&buff.combustion.up&buff.heating_up.react&action.fireball.executing&action.fireball.execute_remains<0.5" );
  ff_combustion->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&buff.combustion.up&!buff.heating_up.react&!buff.hot_streak.react&action.scorch.executing&action.scorch.execute_remains<0.5" );
  ff_combustion->add_action( "flamestrike,if=buff.fury_of_the_sun_king.up&active_enemies>=variable.ff_combustion_flamestrike" );
  ff_combustion->add_action( "pyroblast,if=buff.fury_of_the_sun_king.up" );
  ff_combustion->add_action( "meteor,if=buff.combustion.down|buff.combustion.remains>2" );
  ff_combustion->add_action( "scorch,if=buff.combustion.down&!prev_gcd.1.scorch&cast_time>=gcd.max&(buff.heat_shimmer.react&talent.improved_scorch|improved_scorch.active)" );
  ff_combustion->add_action( "fireball,if=buff.combustion.down" );
  ff_combustion->add_action( "flamestrike,if=buff.hot_streak.react&active_enemies>=variable.ff_combustion_flamestrike" );
  ff_combustion->add_action( "flamestrike,if=prev_gcd.1.scorch&buff.heating_up.react&active_enemies>=variable.ff_combustion_flamestrike" );
  ff_combustion->add_action( "pyroblast,if=buff.hot_streak.react" );
  ff_combustion->add_action( "pyroblast,if=prev_gcd.1.scorch&buff.heating_up.react" );
  ff_combustion->add_action( "phoenix_flames,if=buff.excess_frost.up&(!action.pyroblast.in_flight|!buff.heating_up.react)" );
  ff_combustion->add_action( "fireball" );

  ff_filler->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&buff.heating_up.react&action.fireball.executing&action.fireball.execute_remains<0.5&((cooldown.combustion.remains>variable.pooling_time)|talent.sun_kings_blessing)" );
  ff_filler->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&buff.heating_up.react&action.pyroblast.executing&action.pyroblast.execute_remains<0.5" );
  ff_filler->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&!buff.hot_streak.react&!buff.heating_up.react&action.scorch.executing&action.scorch.execute_remains<0.5&((cooldown.combustion.remains>variable.pooling_time)|talent.sun_kings_blessing)" );
  ff_filler->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&!buff.hot_streak.react&action.shifting_power.executing&((cooldown.combustion.remains>variable.pooling_time)|talent.sun_kings_blessing)" );
  ff_filler->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&!buff.hot_streak.react&(hot_streak_spells_in_flight+buff.heating_up.react<2)&buff.hyperthermia.react&gcd.remains<gcd.max&((cooldown.combustion.remains>variable.pooling_time)|talent.sun_kings_blessing)" );
  ff_filler->add_action( "meteor,if=((cooldown.combustion.remains>variable.pooling_time)|talent.sun_kings_blessing)" );
  ff_filler->add_action( "scorch,if=(improved_scorch.active|buff.heat_shimmer.react&talent.improved_scorch)&debuff.improved_scorch.remains<3*gcd.max&!prev_gcd.1.scorch" );
  ff_filler->add_action( "flamestrike,if=buff.fury_of_the_sun_king.up&active_enemies>=variable.ff_filler_flamestrike" );
  ff_filler->add_action( "flamestrike,if=buff.hyperthermia.react&active_enemies>=variable.ff_filler_flamestrike" );
  ff_filler->add_action( "flamestrike,if=prev_gcd.1.scorch&buff.heating_up.react&active_enemies>=variable.ff_filler_flamestrike" );
  ff_filler->add_action( "flamestrike,if=buff.hot_streak.react&active_enemies>=variable.ff_filler_flamestrike" );
  ff_filler->add_action( "pyroblast,if=buff.fury_of_the_sun_king.up" );
  ff_filler->add_action( "pyroblast,if=buff.hyperthermia.react" );
  ff_filler->add_action( "pyroblast,if=prev_gcd.1.scorch&buff.heating_up.react" );
  ff_filler->add_action( "pyroblast,if=buff.hot_streak.react" );
  ff_filler->add_action( "shifting_power,if=cooldown.combustion.remains>10" );
  ff_filler->add_action( "fireball,if=talent.sun_kings_blessing&buff.frostfire_empowerment.react" );
  ff_filler->add_action( "phoenix_flames,if=(buff.excess_frost.up|talent.sun_kings_blessing)&!(time-action.frostfire_bolt.last_used<0.5)" );
  ff_filler->add_action( "scorch,if=talent.sun_kings_blessing&(scorch_execute.active|buff.heat_shimmer.react)" );
  ff_filler->add_action( "fireball" );

  sf_combustion->add_action( "combustion,use_off_gcd=1,use_while_casting=1,if=action.fireball.executing&(action.fireball.execute_remains<variable.cast_remains_time)|action.scorch.executing&(action.scorch.execute_remains<variable.cast_remains_time)" );
  sf_combustion->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&buff.combustion.up&!action.scorch.executing&!action.fireball.executing&!action.pyroblast.executing&!buff.hot_streak.react&gcd.remains&gcd.remains<gcd.max&(!talent.glorious_incandescence|buff.glorious_incandescence.up|charges_fractional>1.7|buff.combustion.remains<(gcd.max*charges))&(hot_streak_spells_in_flight+buff.heating_up.react)<2" );
  sf_combustion->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&buff.combustion.up&buff.heating_up.react&action.pyroblast.executing&action.pyroblast.execute_remains<0.5" );
  sf_combustion->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&buff.combustion.up&buff.heating_up.react&action.fireball.executing&action.fireball.execute_remains<0.5" );
  sf_combustion->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&buff.combustion.up&!buff.heating_up.react&!buff.hot_streak.react&action.scorch.executing&action.scorch.execute_remains<0.5" );
  sf_combustion->add_action( "scorch,if=buff.combustion.down&(cast_time>=gcd.max|!buff.hot_streak.react)" );
  sf_combustion->add_action( "fireball,if=buff.combustion.down" );
  sf_combustion->add_action( "flamestrike,if=buff.hot_streak.react&active_enemies>=variable.sf_combustion_flamestrike" );
  sf_combustion->add_action( "flamestrike,if=prev_gcd.1.scorch&buff.heating_up.react&active_enemies>=variable.sf_combustion_flamestrike" );
  sf_combustion->add_action( "pyroblast,if=buff.hot_streak.react" );
  sf_combustion->add_action( "pyroblast,if=prev_gcd.1.scorch&buff.heating_up.react" );
  sf_combustion->add_action( "scorch,if=(improved_scorch.active|buff.heat_shimmer.react)&debuff.improved_scorch.remains<4*gcd.max" );
  sf_combustion->add_action( "phoenix_flames" );
  sf_combustion->add_action( "scorch" );
  sf_combustion->add_action( "fireball" );

  sf_filler->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&buff.heating_up.react&action.fireball.executing&action.fireball.execute_remains<0.5" );
  sf_filler->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&!buff.hot_streak.react&!buff.heating_up.react&action.scorch.executing&action.scorch.execute_remains<0.5&cooldown.combustion.remains>variable.pooling_time" );
  sf_filler->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&!buff.hot_streak.react&action.shifting_power.executing&cooldown.combustion.remains>variable.pooling_time" );
  sf_filler->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&!buff.hot_streak.react&(hot_streak_spells_in_flight+buff.heating_up.react<2)&(buff.hyperthermia.react|buff.hyperthermia.up&buff.lesser_time_warp.up)&gcd.remains<gcd.max&(!talent.glorious_incandescence|buff.glorious_incandescence.up|charges_fractional>1.7|buff.hyperthermia.remains<(gcd.max*charges))&cooldown.combustion.remains>variable.pooling_time" );
  sf_filler->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&active_enemies>=2&!buff.hot_streak.react&(hot_streak_spells_in_flight+buff.heating_up.react<2)&buff.glorious_incandescence.up&cooldown.combustion.remains>variable.pooling_time" );
  sf_filler->add_action( "flamestrike,if=(buff.hyperthermia.react|buff.hyperthermia.up&buff.lesser_time_warp.up)&active_enemies>=variable.sf_filler_flamestrike" );
  sf_filler->add_action( "flamestrike,if=buff.hot_streak.react&active_enemies>=variable.sf_filler_flamestrike" );
  sf_filler->add_action( "flamestrike,if=prev_gcd.1.scorch&buff.heating_up.react&active_enemies>=variable.sf_filler_flamestrike" );
  sf_filler->add_action( "pyroblast,if=buff.hyperthermia.react|buff.hyperthermia.up&buff.lesser_time_warp.up" );
  sf_filler->add_action( "pyroblast,if=buff.hot_streak.react" );
  sf_filler->add_action( "pyroblast,if=prev_gcd.1.scorch&buff.heating_up.react" );
  sf_filler->add_action( "shifting_power" );
  sf_filler->add_action( "scorch,if=buff.heat_shimmer.react" );
  sf_filler->add_action( "meteor,if=active_enemies>=2" );
  sf_filler->add_action( "phoenix_flames,if=buff.heating_up.react|action.fire_blast.cooldown_react|action.phoenix_flames.charges_fractional>1.5|buff.born_of_flame.react" );
  sf_filler->add_action( "scorch,if=scorch_execute.active" );
  sf_filler->add_action( "fireball" );
}
//fire_apl_end

//frost_apl_start
void frost( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* ff_aoe = p->get_action_priority_list( "ff_aoe" );
  action_priority_list_t* ff_st = p->get_action_priority_list( "ff_st" );
  action_priority_list_t* ss_aoe = p->get_action_priority_list( "ss_aoe" );
  action_priority_list_t* ss_st = p->get_action_priority_list( "ss_st" );

  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "use_item,name=ingenious_mana_battery,target=self" );
  precombat->add_action( "summon_water_elemental" );
  precombat->add_action( "blizzard,if=active_enemies>=3" );
  precombat->add_action( "glacial_spike" );
  precombat->add_action( "frostbolt" );

  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "run_action_list,name=ff_aoe,if=talent.frostfire_bolt&active_enemies>=3" );
  default_->add_action( "run_action_list,name=ff_st,if=talent.frostfire_bolt" );
  default_->add_action( "run_action_list,name=ss_aoe,if=active_enemies>=3" );
  default_->add_action( "run_action_list,name=ss_st" );

  cds->add_action( "potion" );
  cds->add_action( "use_items,if=prev_gcd.1.frozen_orb|fight_remains<20" );
  cds->add_action( "flurry,line_cd=9999" );
  cds->add_action( "frozen_orb,line_cd=9999" );
  cds->add_action( "ray_of_frost,line_cd=9999" );
  cds->add_action( "blood_fury" );
  cds->add_action( "berserking" );
  cds->add_action( "fireblood" );
  cds->add_action( "ancestral_call" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.power_infusion.down" );

  ff_aoe->add_action( "comet_storm,target_if=min:debuff.freezing.stack,if=debuff.freezing.stack>=16|cooldown.ray_of_frost.full_recharge_time<3|fight_remains<15|!talent.glacial_assault|talent.fractured_frost&(talent.heart_of_ice|talent.white_out)" );
  ff_aoe->add_action( "glacial_spike" );
  ff_aoe->add_action( "flurry,if=cooldown_react&buff.thermal_void.down" );
  ff_aoe->add_action( "frozen_orb" );
  ff_aoe->add_action( "blizzard" );
  ff_aoe->add_action( "ray_of_frost" );
  ff_aoe->add_action( "ice_lance,if=buff.thermal_void.up" );
  ff_aoe->add_action( "ice_lance,if=buff.fingers_of_frost.react&talent.fractured_frost&(talent.heart_of_ice|talent.white_out)" );
  ff_aoe->add_action( "ice_lance,if=debuff.freezing.stack>=10&talent.fractured_frost&(talent.heart_of_ice|talent.white_out)" );
  ff_aoe->add_action( "frostbolt" );

  ff_st->add_action( "comet_storm" );
  ff_st->add_action( "glacial_spike" );
  ff_st->add_action( "flurry,if=cooldown_react&buff.thermal_void.down" );
  ff_st->add_action( "frozen_orb" );
  ff_st->add_action( "ray_of_frost" );
  ff_st->add_action( "ice_lance,if=buff.thermal_void.up" );
  ff_st->add_action( "frostbolt" );

  ss_aoe->add_action( "comet_storm" );
  ss_aoe->add_action( "frozen_orb,if=cooldown_react&(!buff.brain_freeze.react|!talent.wintertide)" );
  ss_aoe->add_action( "blizzard,if=buff.splinterstorm.down" );
  ss_aoe->add_action( "ice_lance,if=buff.thermal_void.react" );
  ss_aoe->add_action( "glacial_spike" );
  ss_aoe->add_action( "flurry,if=cooldown_react&buff.brain_freeze.react" );
  ss_aoe->add_action( "ice_lance,if=debuff.freezing.stack>=6" );
  ss_aoe->add_action( "flurry,if=cooldown_react" );
  ss_aoe->add_action( "ray_of_frost" );
  ss_aoe->add_action( "frostbolt" );

  ss_st->add_action( "comet_storm" );
  ss_st->add_action( "frozen_orb,if=cooldown_react&(!buff.brain_freeze.react|!talent.wintertide)" );
  ss_st->add_action( "ray_of_frost" );
  ss_st->add_action( "ice_lance,if=buff.thermal_void.down" );
  ss_st->add_action( "glacial_spike" );
  ss_st->add_action( "flurry,if=cooldown_react&buff.brain_freeze.react" );
  ss_st->add_action( "ice_lance,if=debuff.freezing.stack>=6" );
  ss_st->add_action( "flurry,if=cooldown_react" );
  ss_st->add_action( "frostbolt" );
}
//frost_apl_end

}  // namespace mage_apl
