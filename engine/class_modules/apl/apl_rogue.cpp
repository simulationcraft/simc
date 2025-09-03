#include "simulationcraft.hpp"
#include "class_modules/apl/apl_rogue.hpp"

namespace rogue_apl {

std::string potion( const player_t* p )
{
  return ( ( p->true_level >= 71 ) ? "tempered_potion_3" :
           ( p->true_level >= 61 ) ? "elemental_potion_of_ultimate_power_3" :
           ( p->true_level >= 51 ) ? "potion_of_spectral_agility" :
           ( p->true_level >= 40 ) ? "potion_of_unbridled_fury" :
           ( p->true_level >= 35 ) ? "draenic_agility" :
           "disabled" );
}

std::string flask( const player_t* p )
{
  if ( p->specialization() == ROGUE_OUTLAW && p->true_level >= 71 )
    return "flask_of_tempered_versatility_3";

  return ( ( p->true_level >= 71 ) ? "flask_of_alchemical_chaos_3" :
           ( p->true_level >= 61 ) ? "iced_phial_of_corrupting_rage_3" :
           ( p->true_level >= 51 ) ? "spectral_flask_of_power" :
           ( p->true_level >= 40 ) ? "greater_flask_of_the_currents" :
           ( p->true_level >= 35 ) ? "greater_draenic_agility_flask" :
           "disabled" );
}

std::string food( const player_t* p )
{
  return ( ( p->true_level >= 71 ) ? "feast_of_the_divine_day" :
           ( p->true_level >= 61 ) ? "fated_fortune_cookie" :
           ( p->true_level >= 51 ) ? "feast_of_gluttonous_hedonism" :
           ( p->true_level >= 45 ) ? "famine_evaluator_and_snack_table" :
           ( p->true_level >= 40 ) ? "lavish_suramar_feast" :
           "disabled" );
}

std::string rune( const player_t* p )
{
  return ( ( p->true_level >= 80 ) ? "crystallized" :
           ( p->true_level >= 70 ) ? "draconic" :
           ( p->true_level >= 60 ) ? "veiled" :
           ( p->true_level >= 50 ) ? "battle_scarred" :
           ( p->true_level >= 45 ) ? "defiled" :
           ( p->true_level >= 40 ) ? "hyper" :
           "disabled" );
}

std::string temporary_enchant( const player_t* p )
{
  if ( p->specialization() == ROGUE_ASSASSINATION && p->true_level >= 71 )
    return "main_hand:algari_mana_oil_3/off_hand:algari_mana_oil_3";
  
  return ( ( p->true_level >= 71 ) ? "main_hand:ironclaw_whetstone_3/off_hand:ironclaw_whetstone_3" :
           ( p->true_level >= 61 ) ? "main_hand:buzzing_rune_3/off_hand:buzzing_rune_3" :
           ( p->true_level >= 51 ) ? "main_hand:shaded_sharpening_stone/off_hand:shaded_sharpening_stone" :
           "disabled" );
}

//assassination_apl_start
void assassination( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe_dot = p->get_action_priority_list( "aoe_dot" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* core_dot = p->get_action_priority_list( "core_dot" );
  action_priority_list_t* direct = p->get_action_priority_list( "direct" );
  action_priority_list_t* items = p->get_action_priority_list( "items" );
  action_priority_list_t* misc_cds = p->get_action_priority_list( "misc_cds" );
  action_priority_list_t* shiv = p->get_action_priority_list( "shiv" );
  action_priority_list_t* stealthed = p->get_action_priority_list( "stealthed" );
  action_priority_list_t* vanish = p->get_action_priority_list( "vanish" );

  precombat->add_action( "apply_poison" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=trinket_sync_slot,value=1,if=trinket.1.has_use_buff&(!trinket.2.has_use_buff|trinket.1.cooldown.duration>=trinket.2.cooldown.duration)&!trinket.2.is.treacherous_transmitter|trinket.1.is.treacherous_transmitter|trinket.1.is.house_of_cards", "Check which trinket slots have Stat Values" );
  precombat->add_action( "variable,name=trinket_sync_slot,value=2,if=trinket.2.has_use_buff&(!trinket.1.has_use_buff|trinket.2.cooldown.duration>trinket.1.cooldown.duration)&!trinket.1.is.treacherous_transmitter|trinket.2.is.treacherous_transmitter|trinket.2.is.house_of_cards" );
  precombat->add_action( "variable,name=effective_spend_cp,value=cp_max_spend-2<?5", "Determine combo point finish condition" );
  precombat->add_action( "stealth", "Pre-cast Slice and Dice if possible" );
  precombat->add_action( "slice_and_dice,precombat_seconds=1" );

  default_->add_action( "stealth", "Restealth if possible (no vulnerable enemies in combat)" );
  default_->add_action( "kick", "Interrupt on cooldown to allow simming interactions with that" );
  default_->add_action( "variable,name=single_target,value=spell_targets.fan_of_knives=1", "Conditional to check if there is only one enemy" );
  default_->add_action( "variable,name=regen_saturated,value=energy.regen_combined>30+10*!talent.dashing_scoundrel", "Combined Energy Regen needed to saturate, with additional check to account for m+ build archetypes" );
  default_->add_action( "variable,name=in_cooldowns,value=dot.kingsbane.ticking|debuff.shiv.up", "Pooling Setup, check for cooldowns" );
  default_->add_action( "variable,name=upper_limit_energy,value=energy.pct>=(80-10*talent.vicious_venoms.rank-30*talent.amplifying_poison)", "Check upper bounds of energy to begin spending" );
  default_->add_action( "variable,name=cd_soon,value=talent.kingsbane&cooldown.kingsbane.remains<3&!cooldown.kingsbane.ready", "Checking for cooldowns soon" );
  default_->add_action( "variable,name=not_pooling,value=variable.in_cooldowns|buff.darkest_night.up|variable.upper_limit_energy|fight_remains<=20", "Pooling Condition all together" );
  default_->add_action( "variable,name=scent_effective_max_stacks,value=(spell_targets.fan_of_knives*talent.scent_of_blood.rank*2)>?20", "Check what the maximum Scent of Blood stacks is currently" );
  default_->add_action( "variable,name=scent_saturation,value=buff.scent_of_blood.stack>=variable.scent_effective_max_stacks", "We are Scent Saturated when our stack count is hitting the maximum" );
  default_->add_action( "call_action_list,name=stealthed,if=stealthed.rogue|buff.indiscriminate_carnage.up|stealthed.improved_garrote|master_assassin_remains>0", "Call Stealthed Actions" );
  default_->add_action( "call_action_list,name=cds", "Call Cooldowns" );
  default_->add_action( "call_action_list,name=core_dot", "Call Core DoT effects" );
  default_->add_action( "call_action_list,name=aoe_dot,if=!variable.single_target", "Call AoE DoTs when in AoE" );
  default_->add_action( "call_action_list,name=direct", "Call Direct Damage Abilities" );
  default_->add_action( "arcane_torrent,if=energy.deficit>=15+energy.regen_combined", "Misc Low-value Cooldowns" );
  default_->add_action( "arcane_pulse" );
  default_->add_action( "lights_judgment" );
  default_->add_action( "bag_of_tricks" );

  aoe_dot->add_action( "variable,name=dot_finisher_condition,value=combo_points>=variable.effective_spend_cp", "AoE Damage over time abilities Helper Variable to check basic finisher conditions" );
  aoe_dot->add_action( "crimson_tempest,target_if=min:remains,if=spell_targets>=2&variable.dot_finisher_condition&refreshable&target.time_to_die-remains>6&!buff.darkest_night.up", "Crimson Tempest on 2+ Targets" );
  aoe_dot->add_action( "garrote,cycle_targets=1,if=combo_points.deficit>=1&pmultiplier<=1&refreshable&!variable.regen_saturated&spell_targets.fan_of_knives<=3&!talent.dashing_scoundrel&target.time_to_die-remains>12", "Garrote upkeep in AoE to reach energy saturation" );
  aoe_dot->add_action( "rupture,cycle_targets=1,if=variable.dot_finisher_condition&refreshable&(!dot.kingsbane.ticking|buff.cold_blood.up)&(!variable.regen_saturated|!variable.scent_saturation)&target.time_to_die>(7+(talent.dashing_scoundrel*5)+(variable.regen_saturated*6))&!buff.darkest_night.up", "Rupture upkeep in AoE to reach energy/scent saturation or to spread for damage" );
  aoe_dot->add_action( "garrote,if=refreshable&combo_points.deficit=1&(pmultiplier<=1|remains<=tick_time&spell_targets.fan_of_knives>=3)&(remains<=tick_time*2&spell_targets.fan_of_knives>=3)&(target.time_to_die-remains)>4&master_assassin_remains=0", "Garrote as a special generator for the last CP before a finisher for edge case handling" );

  cds->add_action( "variable,name=deathmark_kingsbane_condition,value=cooldown.kingsbane.remains<=2&buff.envenom.up", "Cooldowns Wait on Deathmark for Garrote with MA and check for Kingsbane" );
  cds->add_action( "variable,name=deathmark_condition,value=dot.rupture.ticking&(variable.deathmark_kingsbane_condition|spell_targets.fan_of_knives>1&buff.slice_and_dice.remains>5|!talent.kingsbane&dot.crimson_tempest.ticking)&!debuff.deathmark.up", "Deathmark to be used if not stealthed, Rupture is up, and all other talent conditions are satisfied" );
  cds->add_action( "call_action_list,name=items", "Usages for various special-case Trinkets and other Cantrips if applicable" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=dot.deathmark.ticking", "Invoke Externals to Deathmark" );
  cds->add_action( "call_action_list,name=shiv,if=!buff.darkest_night.up", "Check for Applicable Shiv usage" );
  cds->add_action( "deathmark,if=(variable.deathmark_condition&target.time_to_die>=10)|fight_remains<=20", "Cast Deathmark if the target will survive long enough" );
  cds->add_action( "kingsbane,if=(debuff.shiv.up|cooldown.shiv.remains<6)&(buff.envenom.up|spell_targets.fan_of_knives>1)&(cooldown.deathmark.remains>=50-15*(set_bonus.tww3_fatebound_4pc)|dot.deathmark.ticking)|fight_remains<=15" );
  cds->add_action( "thistle_tea,if=hero_tree.deathstalker&(!buff.thistle_tea.up&debuff.shiv.remains>=6|!buff.thistle_tea.up&dot.kingsbane.ticking&dot.kingsbane.remains<=6|!buff.thistle_tea.up&fight_remains<=cooldown.thistle_tea.charges*6)", "Use with shiv or in niche cases at the end of Kingsbane if not already up" );
  cds->add_action( "call_action_list,name=misc_cds", "Potion/Racials/Other misc cooldowns" );
  cds->add_action( "call_action_list,name=vanish,if=!stealthed.all&master_assassin_remains=0|talent.indiscriminate_carnage&!talent.improved_garrote&!variable.scent_saturation&active_dot.rupture<spell_targets.fan_of_knives&spell_targets.fan_of_knives>=3" );
  cds->add_action( "cold_blood,use_off_gcd=1,if=(buff.fatebound_coin_tails.stack>0&buff.fatebound_coin_heads.stack>0)|debuff.shiv.up&(cooldown.deathmark.remains>50&(!set_bonus.tww3_fatebound_4pc)|dot.kingsbane.ticking&(set_bonus.tww3_fatebound_4pc)|!talent.inevitabile_end&effective_combo_points>=variable.effective_spend_cp)", "Cold Blood for Edge Case or Envenoms during shiv" );

  core_dot->add_action( "garrote,if=combo_points.deficit>=1&(pmultiplier<=1)&refreshable&target.time_to_die-remains>12", "Core damage over time abilities used everywhere Maintain Garrote" );
  core_dot->add_action( "rupture,if=combo_points>=variable.effective_spend_cp&(pmultiplier<=1)&refreshable&!buff.cold_blood.up&target.time_to_die-remains>(4+(talent.dashing_scoundrel*5)+(variable.regen_saturated*6))&(!buff.darkest_night.up|talent.caustic_spatter&!debuff.caustic_spatter.up)", "Maintain Rupture unless darkest night is up" );
  core_dot->add_action( "crimson_tempest,if=combo_points>=variable.effective_spend_cp&refreshable&pmultiplier<=persistent_multiplier&!buff.darkest_night.up&!talent.amplifying_poison&spell_targets.fan_of_knives=1", "Maintain Crimson Tempest unless it would remove a stronger cast" );

  direct->add_action( "variable,name=use_caustic_filler,value=talent.caustic_spatter&dot.rupture.ticking&(!debuff.caustic_spatter.up|debuff.caustic_spatter.remains<=2)&combo_points.deficit>=1&!variable.single_target", "Direct Damage Abilities Envenom at applicable cp if not pooling, capped on amplifying poison stacks, on an animacharged CP, or in aoe. Maintain Caustic Spatter" );
  direct->add_action( "mutilate,if=variable.use_caustic_filler" );
  direct->add_action( "ambush,if=variable.use_caustic_filler" );
  direct->add_action( "envenom,if=!buff.darkest_night.up&combo_points>=variable.effective_spend_cp&(variable.not_pooling|debuff.amplifying_poison.stack>=20|!variable.single_target)", "Base Envenom Condition" );
  direct->add_action( "envenom,if=buff.darkest_night.up&effective_combo_points>=cp_max_spend", "Special Envenom handling for Darkest Night" );
  direct->add_action( "variable,name=use_filler,value=combo_points<=variable.effective_spend_cp&!variable.cd_soon|variable.not_pooling|!variable.single_target", "Various Checks to see if we need to use a generator" );
  direct->add_action( "fan_of_knives,if=buff.clear_the_witnesses.up&(spell_targets.fan_of_knives>=2-(buff.lingering_darkness.up|!talent.vicious_venoms))" );
  direct->add_action( "variable,name=fok_target_count,value=spell_targets.fan_of_knives>=3-(talent.momentum_of_despair&talent.thrown_precision)+talent.vicious_venoms+talent.blindside" );
  direct->add_action( "fan_of_knives,if=buff.darkest_night.up&combo_points=6&(!talent.vicious_venoms|spell_targets.fan_of_knives>=2)", "Fan of Knives at 6cp for special case Darkest Night" );
  direct->add_action( "fan_of_knives,if=variable.use_filler&!priority_rotation&variable.fok_target_count", "Fan of Knives at 3+ targets, accounting for various edge cases" );
  direct->add_action( "ambush,if=variable.use_filler&(buff.blindside.up|stealthed.rogue)&(!dot.kingsbane.ticking|debuff.deathmark.down|buff.blindside.up)", "Ambush on Blindside/Subterfuge. Do not use Ambush from stealth during Kingsbane & Deathmark if possible." );
  direct->add_action( "mutilate,target_if=!dot.deadly_poison_dot.ticking&!debuff.amplifying_poison.up,if=variable.use_filler&spell_targets.fan_of_knives=2", "Tab-Mutilate to apply Deadly Poison at 2 targets if not using Fan of Knives" );
  direct->add_action( "mutilate,if=variable.use_filler", "Fallback Mutilate if all else fails" );

  items->add_action( "variable,name=base_trinket_condition,value=dot.rupture.ticking&cooldown.deathmark.remains<2&!cooldown.deathmark.ready|dot.deathmark.ticking|fight_remains<=22", "Special Case Trinkets" );
  items->add_action( "use_item,name=astral_gladiators_badge_of_ferocity,use_off_gcd=1,if=dot.kingsbane.ticking|dot.deathmkark.ticking|(cooldown.kingsbane.remains>60|cooldown.deathmark.remains>60)" );
  items->add_action( "use_item,name=treacherous_transmitter,use_off_gcd=1,if=variable.base_trinket_condition" );
  items->add_action( "use_item,name=unyielding_netherprism,use_off_gcd=1,if=dot.deathmark.ticking&(buff.latent_energy.stack>=16|fight_remains<=90|(!trinket.2.cooldown.ready|!trinket.1.cooldown.ready))|fight_remains<=20" );
  items->add_action( "use_item,name=mad_queens_mandate,if=cooldown.deathmark.remains>=30&!dot.deathmark.ticking|fight_remains<=3" );
  items->add_action( "use_item,name=junkmaestros_mega_magnet,if=cooldown.deathmark.remains>=30&!dot.deathmark.ticking&!debuff.shiv.up&(!talent.deathstalkers_mark|buff.lingering_darkness.up&buff.junkmaestros_mega_magnet.stack>5)|fight_remains<=10" );
  items->add_action( "do_treacherous_transmitter_task,use_off_gcd=1,if=dot.deathmark.ticking&variable.single_target|buff.realigning_nexus_convergence_divergence.up&buff.realigning_nexus_convergence_divergence.remains<=2|buff.cryptic_instructions.up&buff.cryptic_instructions.remains<=2|buff.errant_manaforge_emission.up&buff.errant_manaforge_emission.remains<=2|fight_remains<=15" );
  items->add_action( "use_item,name=imperfect_ascendancy_serum,use_off_gcd=1,if=variable.base_trinket_condition" );
  items->add_action( "use_items,slots=trinket1,if=(variable.trinket_sync_slot=1&(debuff.deathmark.up)|(variable.trinket_sync_slot=2&!trinket.2.cooldown.ready&cooldown.deathmark.remains>20))|!variable.trinket_sync_slot|fight_remains<=20", "Fallback case for using stat trinkets" );
  items->add_action( "use_items,slots=trinket2,if=(variable.trinket_sync_slot=2&(debuff.deathmark.up)|(variable.trinket_sync_slot=1&!trinket.1.cooldown.ready&cooldown.deathmark.remains>20))|!variable.trinket_sync_slot|fight_remains<=20" );

  misc_cds->add_action( "potion,if=buff.bloodlust.react|fight_remains<30|debuff.deathmark.up", "Miscellaneous Cooldowns Potion" );
  misc_cds->add_action( "blood_fury,if=debuff.deathmark.up", "Various special racials to be synced with cooldowns" );
  misc_cds->add_action( "berserking,if=debuff.deathmark.up" );
  misc_cds->add_action( "fireblood,if=debuff.deathmark.up" );
  misc_cds->add_action( "ancestral_call,if=debuff.deathmark.up" );

  shiv->add_action( "variable,name=shiv_condition,value=!debuff.shiv.up&dot.garrote.ticking&dot.rupture.ticking&spell_targets.fan_of_knives<=5", "Shiv conditions Generic Variables to check for basic shiv eligibility" );
  shiv->add_action( "variable,name=shiv_kingsbane_condition,value=talent.kingsbane&buff.envenom.up&variable.shiv_condition" );
  shiv->add_action( "shiv,if=talent.lightweight_shiv&variable.shiv_kingsbane_condition&(cooldown.deathmark.ready|cooldown.deathmark.remains<=1)&(cooldown.kingsbane.ready|cooldown.kingsbane.remains<=2)&set_bonus.tww3_fatebound_2pc", "Shiv for Fatebound Edge Case Coins Before Deathmark + Kingsbane with new Tier Set" );
  shiv->add_action( "shiv,if=talent.arterial_precision&!debuff.shiv.up&dot.garrote.ticking&dot.rupture.ticking&spell_targets.fan_of_knives>=4&dot.crimson_tempest.ticking&(target.health.pct<=35&talent.zoldyck_recipe|cooldown.shiv.charges_fractional>=1.9)", "Shiv for aoe with Arterial Precision" );
  shiv->add_action( "shiv,if=!talent.lightweight_shiv.enabled&variable.shiv_kingsbane_condition&(dot.kingsbane.ticking&dot.kingsbane.remains<(8+3*(set_bonus.tww3_deathstalker_4pc))|!dot.kingsbane.ticking&cooldown.kingsbane.remains>=20)&(!talent.crimson_tempest.enabled|variable.single_target|dot.crimson_tempest.ticking)", "Single-charge Shiv case for Kingsbane" );
  shiv->add_action( "shiv,if=debuff.deathstalkers_mark.stack<=2&combo_points>=variable.effective_spend_cp&buff.lingering_darkness.up", "Shiv for big Darkest Night Envenom during Lingering Darkness" );
  shiv->add_action( "shiv,if=talent.lightweight_shiv.enabled&variable.shiv_kingsbane_condition&(dot.kingsbane.ticking&dot.kingsbane.remains<(8+3*(set_bonus.tww3_deathstalker_4pc))&dot.kingsbane.remains>4|cooldown.kingsbane.remains<=1&cooldown.shiv.charges_fractional>=1.7)", "Double-charge Shiv case for Kingsbane" );
  shiv->add_action( "shiv,if=debuff.deathmark.up&talent.arterial_precision&!debuff.shiv.up&dot.garrote.ticking&dot.rupture.ticking", "Fallback shiv for arterial during deathmark - WIP needs checking when Fatebound Kingsbane stacks are fixed, as it currently is munching shiv before the last 8 seconds of KB." );
  shiv->add_action( "shiv,if=!debuff.deathmark.up&!talent.kingsbane&variable.shiv_condition&(dot.crimson_tempest.ticking|talent.amplifying_poison)&(((talent.lightweight_shiv+1)-cooldown.shiv.charges_fractional)*30<cooldown.deathmark.remains)&raid_event.adds.in>20", "Fallback if no special cases apply" );
  shiv->add_action( "shiv,if=!talent.kingsbane&!talent.arterial_precision&variable.shiv_condition&(!talent.crimson_tempest.enabled|variable.single_target|dot.crimson_tempest.ticking)" );
  shiv->add_action( "shiv,if=fight_remains<=cooldown.shiv.charges*(8+3*(set_bonus.tww3_deathstalker_4pc))", "Dump Shiv on fight end" );

  stealthed->add_action( "pool_resource,for_next=1", "Stealthed Actions" );
  stealthed->add_action( "ambush,if=!debuff.deathstalkers_mark.up&hero_tree.deathstalker&combo_points<variable.effective_spend_cp&(dot.rupture.ticking|variable.single_target|!talent.subterfuge)", "Apply Deathstalkers Mark if it has fallen off or waiting for Rupture in AoE" );
  stealthed->add_action( "shiv,if=talent.kingsbane&dot.kingsbane.ticking&dot.kingsbane.remains<8&(!debuff.shiv.up&debuff.shiv.remains<1)&buff.envenom.up", "Make sure to have Shiv up during Kingsbane as a final check" );
  stealthed->add_action( "envenom,if=effective_combo_points>=variable.effective_spend_cp&dot.kingsbane.ticking&buff.envenom.remains<=3&(debuff.deathstalkers_mark.up|buff.cold_blood.up|buff.darkest_night.up&combo_points=7)", "Envenom to maintain the buff during Subterfuge" );
  stealthed->add_action( "envenom,if=effective_combo_points>=variable.effective_spend_cp&buff.master_assassin_aura.up&variable.single_target&(debuff.deathstalkers_mark.up|buff.cold_blood.up|buff.darkest_night.up&combo_points=7)", "Envenom during Master Assassin in single target" );
  stealthed->add_action( "rupture,target_if=effective_combo_points>=variable.effective_spend_cp&buff.indiscriminate_carnage.up&refreshable&((talent.caustic_spatter&!debuff.caustic_spatter.up&!dot.rupture.ticking)|!buff.darkest_night.up)&(!variable.regen_saturated|!variable.scent_saturation|((!talent.dashing_scoundrel|!talent.poison_bomb)&buff.indiscriminate_carnage.up&!dot.rupture.ticking))&target.time_to_die>15", "Rupture during Indiscriminate Carnage" );
  stealthed->add_action( "garrote,target_if=min:remains,if=stealthed.improved_garrote&(remains<12|pmultiplier<=1|(buff.indiscriminate_carnage.up&active_dot.garrote<spell_targets.fan_of_knives))&!variable.single_target&target.time_to_die-remains>2&combo_points.deficit>2-buff.darkest_night.up*2", "Improved Garrote: Apply or Refresh with buffed Garrotes, accounting for Indiscriminate Carnage" );
  stealthed->add_action( "garrote,if=stealthed.improved_garrote&(pmultiplier<=1|refreshable)&combo_points.deficit>=1+2*talent.shrouded_suffocation", "Improve Garrote: Apply or Refresh Improved Garrotes as a final check" );

  vanish->add_action( "pool_resource,for_next=1,extra_amount=45", "Stealth Cooldowns Vanish Sync for Improved Garrote with Deathmark" );
  vanish->add_action( "vanish,if=dot.deathmark.ticking&buff.cold_blood.up&buff.fatebound_coin_tails.stack>=1&buff.fatebound_coin_heads.stack>=1", "Vanish to fish for Fateful Ending" );
  vanish->add_action( "vanish,if=!talent.master_assassin&!talent.indiscriminate_carnage&talent.improved_garrote&cooldown.garrote.up&(dot.garrote.pmultiplier<=1|dot.garrote.refreshable)&(debuff.deathmark.up|cooldown.deathmark.remains<4)&combo_points.deficit>=(spell_targets.fan_of_knives>?4)", "Vanish to spread Garrote during Deathmark without Indiscriminate Carnage" );
  vanish->add_action( "pool_resource,for_next=1,extra_amount=45" );
  vanish->add_action( "vanish,if=talent.indiscriminate_carnage&talent.improved_garrote&cooldown.garrote.up&(dot.garrote.pmultiplier<=1|dot.garrote.refreshable)&spell_targets.fan_of_knives>2&(target.time_to_die-remains>15|raid_event.adds.in>20)", "Vanish for cleaving Improved Garrotes with Indiscriminate Carnage" );
  vanish->add_action( "vanish,if=talent.indiscriminate_carnage&!talent.improved_garrote&!variable.scent_saturation&spell_targets.fan_of_knives>2&(target.time_to_die-remains>15|raid_event.adds.in>20)", "Vanish for cleaving Ruptures with Indiscriminate Carnage if not talented into Improved Garrote - WIP needs checking for possible helper line for rupture cleaving with improved garrote as a niche case" );
  vanish->add_action( "vanish,if=talent.master_assassin&debuff.deathmark.up&dot.kingsbane.remains<=6+3*talent.subterfuge.rank", "Vanish fallback for Master Assassin during Deathmark" );
  vanish->add_action( "vanish,if=talent.improved_garrote&cooldown.garrote.up&(dot.garrote.pmultiplier<=1|dot.garrote.refreshable)&(debuff.deathmark.up)&raid_event.adds.in>30", "Vanish fallback for Improved Garrote during Deathmark if no add waves are expected" );
}
//assassination_apl_end

//outlaw_apl_start
void outlaw( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* build = p->get_action_priority_list( "build" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* finish = p->get_action_priority_list( "finish" );
  action_priority_list_t* items = p->get_action_priority_list( "items" );
  action_priority_list_t* roll_the_bones = p->get_action_priority_list( "roll_the_bones" );
  action_priority_list_t* stealth = p->get_action_priority_list( "stealth" );
  action_priority_list_t* vanish = p->get_action_priority_list( "vanish" );

  precombat->add_action( "apply_poison,nonlethal=none,lethal=instant" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "use_item,name=imperfect_ascendancy_serum" );
  precombat->add_action( "stealth,precombat_seconds=2" );
  precombat->add_action( "adrenaline_rush,precombat_seconds=1,if=!hero_tree.fatebound&talent.keep_it_rolling&talent.loaded_dice", "Prepull Adrenaline Rush if using Trickster+KIR+Loaded Dice. Fatebound would rather AR on pull for Coin damage." );
  precombat->add_action( "roll_the_bones,precombat_seconds=1,if=!hero_tree.fatebound|!talent.loaded_dice", "Prepull Roll the Bones if using Trickster or not using Loaded Dice." );

  default_->add_action( "stealth", "Restealth if possible (no vulnerable enemies in combat)." );
  default_->add_action( "kick", "Interrupt on cooldown to allow simming interactions with that." );
  default_->add_action( "variable,name=ambush_condition,value=(talent.hidden_opportunity|combo_points.deficit>=2+talent.improved_ambush+buff.broadside.up)&energy>=50" );
  default_->add_action( "variable,name=finish_condition,value=combo_points>=cp_max_spend-1-(stealthed.all&talent.crackshot|(hero_tree.fatebound|hero_tree.trickster)&talent.hidden_opportunity&(buff.audacity.up|buff.opportunity.up))", "Use finishers if at -1 from max combo points, or -2 in Stealth with Crackshot. With the hero trees, Hidden Opportunity builds also finish at -2 if Audacity or Opportunity is active." );
  default_->add_action( "variable,name=buffs_above_pandemic,value=(buff.broadside.remains>39)+(buff.ruthless_precision.remains>39)+(buff.true_bearing.remains>39)+(buff.grand_melee.remains>39)+(buff.buried_treasure.remains>39)+(buff.skull_and_crossbones.remains>39)" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=stealth,if=stealthed.all", "High priority stealth list, will fall through if no conditions are met." );
  default_->add_action( "run_action_list,name=finish,if=variable.finish_condition" );
  default_->add_action( "call_action_list,name=build" );
  default_->add_action( "arcane_torrent,if=energy.base_deficit>=15+energy.regen" );
  default_->add_action( "arcane_pulse" );
  default_->add_action( "lights_judgment" );
  default_->add_action( "bag_of_tricks" );

  build->add_action( "ambush,if=talent.hidden_opportunity&buff.audacity.up", "Builders  High priority Ambush with Hidden Opportunity." );
  build->add_action( "sinister_strike,if=buff.disorienting_strikes.up&!stealthed.all&!talent.hidden_opportunity&buff.escalating_blade.stack<4&!buff.tww3_trickster_4pc.up", "Outside of stealth, Trickster builds should prioritize Sinister Strike when Unseen Blade is guaranteed. This is mostly neutral/irrelevant for Hidden Opportunity builds." );
  build->add_action( "pistol_shot,if=talent.fan_the_hammer&talent.audacity&talent.hidden_opportunity&buff.opportunity.up&!buff.audacity.up", "With Audacity + Hidden Opportunity + Fan the Hammer, consume Opportunity to proc Audacity any time Ambush is not available." );
  build->add_action( "blade_flurry,if=talent.deft_maneuvers&spell_targets>=4&(combo_points<=2|!buff.adrenaline_rush.up|!hero_tree.trickster)", "Without Hidden Opportunity, prioritize building CPs with Blade Flurry at 4+ targets. Trickster shoulds prefer to use this at low CPs unless AR isn't active." );
  build->add_action( "blade_flurry,if=talent.deft_maneuvers&combo_points.deficit=spell_targets+buff.broadside.up&spell_targets>=3-hero_tree.fatebound&talent.fan_the_hammer.rank=1", "At sustain 3 targets (2 target for Fatebound 1FTH), Blade Flurry can be used to build CPs if we are missing CPs equal to the amount it will give." );
  build->add_action( "pistol_shot,if=talent.fan_the_hammer.rank=2&buff.opportunity.up&(buff.opportunity.stack>=buff.opportunity.max_stack|buff.opportunity.remains<2)", "With 2 ranks in Fan the Hammer, consume Opportunity as if at max stacks or if it will expire." );
  build->add_action( "pistol_shot,if=talent.fan_the_hammer&buff.opportunity.up&(combo_points.deficit>=(1+(talent.quick_draw+buff.broadside.up)*(talent.fan_the_hammer.rank+1))|combo_points<=talent.ruthlessness)", "With Fan the Hammer, consume Opportunity if it will not overcap CPs, or with 1 CP at minimum." );
  build->add_action( "pistol_shot,if=!talent.fan_the_hammer&buff.opportunity.up&(energy.base_deficit>energy.regen*1.5|combo_points.deficit<=1+buff.broadside.up|talent.quick_draw.enabled|talent.audacity.enabled&!buff.audacity.up)", "If not using Fan the Hammer, then consume Opportunity based on energy, when it will exactly cap CPs, or when using Quick Draw." );
  build->add_action( "coup_de_grace,if=!stealthed.all", "Use Coup de Grace at low CPs if Sinister Strike would otherwise be used." );
  build->add_action( "pool_resource,for_next=1", "Fallback pooling just so Sinister Strike is never casted if Ambush is available with Hidden Opportunity." );
  build->add_action( "ambush,if=talent.hidden_opportunity" );
  build->add_action( "sinister_strike" );

  cds->add_action( "adrenaline_rush,if=!buff.adrenaline_rush.up&(!variable.finish_condition|!talent.improved_adrenaline_rush)|buff.adrenaline_rush.up&talent.improved_adrenaline_rush&combo_points<=2&(cooldown.vanish.charges=0|buff.double_jeopardy.up|!set_bonus.tww3_fatebound_2pc)|fight_remains<2", "Cooldowns  Maintain Adrenaline Rush. With Improved AR, recast at low CPs even if already active. With TWW3 Fatebound, attempt to send AR alongside Vanish if there is a Vanish charge available." );
  cds->add_action( "ghostly_strike,if=hero_tree.fatebound&(buff.double_jeopardy.up|fight_remains<2|cooldown.vanish.charges=0|!set_bonus.tww3_fatebound_2pc)|hero_tree.trickster&(combo_points<cp_max_spend|talent.fan_the_hammer.rank>1)", "High priority Ghostly Strike as it is off-gcd. Trickster builds with 1 point in Fan the Hammer prefer not to use it at max CPs." );
  cds->add_action( "sprint,if=(trinket.1.is.scroll_of_momentum|trinket.2.is.scroll_of_momentum)&buff.full_momentum.up", "Use Sprint to further benefit from the Scroll of Momentum trinket." );
  cds->add_action( "blade_flurry,if=spell_targets>=2&buff.blade_flurry.remains<gcd", "Maintain Blade Flurry at 2+ targets." );
  cds->add_action( "keep_it_rolling,if=rtb_buffs>=4&rtb_buffs.normal<=2|rtb_buffs.normal>=5&rtb_buffs=6", "Use Keep it Rolling immediately with any 4 RTB buffs. If a natural 5 buff is rolled, then wait until the final 6th buff is obtained from Count the Odds." );
  cds->add_action( "call_action_list,name=roll_the_bones", "Call the various Roll the Bones rules." );
  cds->add_action( "call_action_list,name=items", "Call items before Vanish, as some items should not be used in stealth and have priority over stealth." );
  cds->add_action( "vanish,if=talent.underhanded_upper_hand&talent.subterfuge&buff.adrenaline_rush.up&!stealthed.all&buff.adrenaline_rush.remains<2&cooldown.adrenaline_rush.remains>30", "If necessary, standard builds prioritize using Vanish at any CP to prevent Adrenaline Rush downtime." );
  cds->add_action( "run_action_list,name=finish,if=!stealthed.all&(cooldown.killing_spree.ready&talent.killing_spree|buff.escalating_blade.stack>=4|buff.tww3_trickster_4pc.up)&variable.finish_condition", "If not at risk of losing Adrenaline Rush, run finishers to use Killing Spree or Coup de Grace as a higher priority than Vanish." );
  cds->add_action( "call_action_list,name=vanish,if=!stealthed.all&talent.crackshot&talent.underhanded_upper_hand&talent.subterfuge&(buff.adrenaline_rush.up&variable.finish_condition&(!cooldown.adrenaline_rush.ready|!set_bonus.tww3_fatebound_2pc)|set_bonus.tww3_fatebound_2pc&cooldown.adrenaline_rush.ready&combo_points<=2)", "If not at risk of losing Adrenaline Rush, call flexible Vanish rules to be used at finisher CPs, or Fatebound TWW3 can Vanish at low CPs if AR is ready." );
  cds->add_action( "vanish,if=!stealthed.all&(variable.finish_condition|!talent.crackshot)&(!talent.underhanded_upper_hand|!talent.subterfuge|!talent.crackshot)&(buff.adrenaline_rush.up&talent.subterfuge&talent.underhanded_upper_hand|((!talent.subterfuge|!talent.underhanded_upper_hand)&talent.hidden_opportunity&!buff.audacity.up&buff.opportunity.stack<buff.opportunity.max_stack&variable.ambush_condition|(!talent.hidden_opportunity&(talent.take_em_by_surprise|talent.double_jeopardy))))", "Fallback Vanish for builds lacking one of the mandatory stealth talents. If possible, Vanish for AR, otherwise for Ambush when Audacity isn't active, or otherwise to proc Take 'em By Surprise or Fatebound coins." );
  cds->add_action( "shadowmeld,if=variable.finish_condition&!cooldown.vanish.ready&!stealthed.all", "Generic catch-all for Shadowmeld. Technically, usage in DungeonSlice or DungeonRoute sims could mirror Vanish usage on packs." );
  cds->add_action( "blade_rush,if=energy.base_time_to_max>4&!stealthed.all", "Use Blade Rush at minimal energy outside of stealth." );
  cds->add_action( "potion,if=buff.bloodlust.react|fight_remains<30|buff.adrenaline_rush.up" );
  cds->add_action( "blood_fury" );
  cds->add_action( "berserking" );
  cds->add_action( "fireblood" );
  cds->add_action( "ancestral_call" );

  finish->add_action( "cold_blood", "Finishers" );
  finish->add_action( "pool_resource,for_next=1" );
  finish->add_action( "killing_spree,interrupt_if=talent.keep_it_rolling&talent.fan_the_hammer.rank=2&combo_points>=cp_max_spend,interrupt_global=1", "Keep it Rolling builds with 2FTH should cancel Killing Spree after reaching max CPs during the animation." );
  finish->add_action( "coup_de_grace" );
  finish->add_action( "between_the_eyes,if=(buff.ruthless_precision.up|buff.between_the_eyes.remains<4|!hero_tree.fatebound)&(!buff.greenskins_wickers.up|!talent.greenskins_wickers)", "Outside of stealth, use Between the Eyes to maintain the buff, or with Ruthless Precision active, or to proc Greenskins Wickers if not active. Trickster builds can also send BtE on cooldown." );
  finish->add_action( "dispatch" );

  items->add_action( "use_item,name=imperfect_ascendancy_serum,if=!stealthed.all|fight_remains<=22", "Trinkets" );
  items->add_action( "use_item,name=mad_queens_mandate,if=!stealthed.all|fight_remains<=5" );
  items->add_action( "use_item,name=cursed_stone_idol,if=!stealthed.all|fight_remains<=15" );
  items->add_action( "use_item,name=unyielding_netherprism,if=(rtb_buffs>=4|!talent.keep_it_rolling)&(buff.vanish.up|!talent.subterfuge)|fight_remains<=20", "Send Unyielding Netherprism alongside a Vanish window after KIR is used." );
  items->add_action( "use_item,name=junkmaestros_mega_magnet,if=buff.between_the_eyes.up&buff.junkmaestros_mega_magnet.stack>25|fight_remains<=5", "Let the magnet trinket stack up just so it does not disrupt a 2nd on-use trinket." );
  items->add_action( "use_items,slots=trinket1,if=buff.between_the_eyes.up|trinket.1.has_stat.any_dps|fight_remains<=20", "Default conditions for usable items." );
  items->add_action( "use_items,slots=trinket2,if=buff.between_the_eyes.up|trinket.2.has_stat.any_dps|fight_remains<=20" );

  roll_the_bones->add_action( "roll_the_bones,if=rtb_buffs=0", "Maintain Roll the Bones: roll with 0 buffs." );
  roll_the_bones->add_action( "roll_the_bones,if=(set_bonus.tww2_4pc|talent.sleight_of_hand|talent.supercharger)&rtb_buffs.will_lose<=1&(variable.buffs_above_pandemic<5|rtb_buffs.max_remains<42|!set_bonus.tww2_4pc)", "With TWW2, Sleight of Hand, or Supercharger: roll if you will lose 0 or 1 buffs. This includes rolling immediately after KIR. With TWW2, don't roll immediately after a natural 5 buff KIR." );
  roll_the_bones->add_action( "roll_the_bones,if=(set_bonus.tww2_4pc|talent.supercharger&(buff.loaded_dice.up|talent.sleight_of_hand&!talent.keep_it_rolling))&rtb_buffs<=2", "With TWW2, or Supercharger with either Loaded Dice or Sleight of Hand without KIR: roll over any 2 buffs." );
  roll_the_bones->add_action( "roll_the_bones,if=set_bonus.tww2_4pc&rtb_buffs.will_lose<5&(rtb_buffs.max_remains<11|!talent.keep_it_rolling)", "With TWW2, roll over 3-4 buffs, but KIR builds only if all buffs are under ~10 seconds remaining." );
  roll_the_bones->add_action( "roll_the_bones,if=!set_bonus.tww2_4pc&!talent.keep_it_rolling&!talent.supercharger&buff.loaded_dice.up&rtb_buffs<=2&!buff.broadside.up&!buff.ruthless_precision.up&!buff.true_bearing.up", "Without TWW2, HO builds without Supercharger can roll over 2 buffs with Loaded Dice active and you won't lose Broadside, Ruthless Precision, or True Bearing." );

  stealth->add_action( "cold_blood,if=variable.finish_condition", "Stealth" );
  stealth->add_action( "pool_resource,for_next=1", "Ensure Crackshot Between the Eyes is not skipped at low energy." );
  stealth->add_action( "between_the_eyes,if=variable.finish_condition&talent.crackshot&(!buff.shadowmeld.up|stealthed.rogue)", "High priority Between the Eyes for Crackshot, except not directly out of Shadowmeld." );
  stealth->add_action( "dispatch,if=variable.finish_condition" );
  stealth->add_action( "pistol_shot,if=talent.crackshot&talent.fan_the_hammer.rank>=2&buff.opportunity.stack>=6&(buff.broadside.up&combo_points<=1|buff.greenskins_wickers.up)", "Inside stealth, 2FTH builds can consume Opportunity for Greenskins, or with max stacks + Broadside active + minimal CPs." );
  stealth->add_action( "ambush,if=talent.hidden_opportunity" );

  vanish->add_action( "vanish,if=set_bonus.tww3_fatebound_2pc&cooldown.ghostly_strike.ready&talent.ghostly_strike", "Vanish usage for standard builds  TWW3 Fatebound always attempts to align Vanish with Ghostly Strike." );
  vanish->add_action( "vanish,if=(hero_tree.fatebound|!talent.killing_spree)&(!cooldown.between_the_eyes.ready&buff.ruthless_precision.remains>4|buff.supercharge_1.up)&(!set_bonus.tww3_fatebound_2pc|!talent.ghostly_strike)", "Fatebound without TWW3, or builds without Killing Spree attempt to hold Vanish for when BtE is on cooldown and Ruthless Precision is active." );
  vanish->add_action( "vanish,if=hero_tree.trickster&talent.killing_spree&cooldown.killing_spree.remains>30&(time-action.coup_de_grace.last_used<=10|!set_bonus.tww3_trickster_4pc)", "Trickster builds with Killing Spree should Vanish if Killing Spree is not up soon. With TWW3 Trickster, attempt to align Vanish with a recently used Coup de Grace." );
  vanish->add_action( "vanish,if=cooldown.vanish.full_recharge_time<15&(!set_bonus.tww3_fatebound_2pc|!talent.ghostly_strike)|fight_remains<charges*8", "Vanish if about to cap charges or sim duration is ending soon. TWW3 Fatebound will sit on max charges for an upcoming Ghostly Strike." );
}
//outlaw_apl_end

//subtlety_apl_start
void subtlety( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* race = p->get_action_priority_list( "race" );
  action_priority_list_t* item = p->get_action_priority_list( "item" );
  action_priority_list_t* stealth_cds = p->get_action_priority_list( "stealth_cds" );
  action_priority_list_t* finish = p->get_action_priority_list( "finish" );
  action_priority_list_t* build = p->get_action_priority_list( "build" );
  action_priority_list_t* fill = p->get_action_priority_list( "fill" );

  precombat->add_action( "apply_poison", "Consumables" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=priority_rotation,value=priority_rotation" );
  precombat->add_action( "variable,name=trinket_sync_slot,value=1,if=trinket.1.has_use_buff&(!trinket.2.has_use_buff|trinket.1.is.treacherous_transmitter|trinket.1.cooldown.duration>=trinket.2.cooldown.duration)" );
  precombat->add_action( "variable,name=trinket_sync_slot,value=2,if=trinket.2.has_use_buff&(!trinket.1.has_use_buff|trinket.2.cooldown.duration>trinket.1.cooldown.duration)" );
  precombat->add_action( "stealth" );

  default_->add_action( "stealth" );
  default_->add_action( "variable,name=stealth,value=buff.shadow_dance.up|buff.stealth.up|buff.vanish.up", "Variables" );
  default_->add_action( "variable,name=targets,value=spell_targets.shuriken_storm" );
  default_->add_action( "variable,name=skip_rupture,value=buff.shadow_dance.up|buff.darkest_night.up|variable.targets>=4&(!talent.replicating_shadows&talent.unseen_blade|raid_event.adds.up)" );
  default_->add_action( "variable,name=maintenance,value=(dot.rupture.ticking|variable.skip_rupture)&(buff.slice_and_dice.up|variable.targets<=2)" );
  default_->add_action( "variable,name=secret,value=buff.shadow_dance.up&!buff.darkest_night.up|(cooldown.flagellation.remains<60&cooldown.flagellation.remains>30&talent.death_perception&talent.unseen_blade)" );
  default_->add_action( "variable,name=racial_sync,value=(buff.shadow_blades.up&buff.shadow_dance.up)|!talent.shadow_blades&buff.symbols_of_death.up|fight_remains<20" );
  default_->add_action( "variable,name=shd_cp,value=combo_points<=1|buff.darkest_night.up&combo_points>=7|effective_combo_points>=6&talent.unseen_blade" );
  default_->add_action( "call_action_list,name=cds", "Cooldowns" );
  default_->add_action( "call_action_list,name=race", "Racials" );
  default_->add_action( "call_action_list,name=item", "Items (Trinkets)" );
  default_->add_action( "call_action_list,name=stealth_cds,if=!variable.stealth", "Cooldowns for Stealth" );
  default_->add_action( "call_action_list,name=finish,if=!buff.darkest_night.up&effective_combo_points>=6|buff.darkest_night.up&combo_points==cp_max_spend|action.coup_de_grace.ready&cooldown.secret_technique.remains>0", "Finishing Rules" );
  default_->add_action( "call_action_list,name=build", "Combo Point Builder" );
  default_->add_action( "call_action_list,name=fill,if=!variable.stealth", "Filler, Spells used if you can use nothing else." );

  cds->add_action( "cold_blood,if=cooldown.secret_technique.up&buff.shadow_dance.up&combo_points>=6&variable.secret&(buff.flagellation_persist.up|buff.flagellation_buff.remains<=3)", "Cooldowns" );
  cds->add_action( "potion,if=buff.bloodlust.react|fight_remains<30|buff.flagellation_buff.up" );
  cds->add_action( "symbols_of_death,if=(buff.symbols_of_death.remains<=3.5&variable.maintenance&(variable.targets>1|raid_event.adds.up|!buff.flagellation_buff.up|dot.rupture.remains>=30)&(!talent.flagellation|cooldown.flagellation.remains>=30-15*!talent.death_perception&cooldown.secret_technique.remains<8|!talent.death_perception)|fight_remains<=15)" );
  cds->add_action( "shadow_blades,if=variable.maintenance&variable.shd_cp&buff.shadow_dance.up&!buff.premeditation.up" );
  cds->add_action( "thistle_tea,if=buff.shadow_dance.remains>4&!buff.thistle_tea.up" );
  cds->add_action( "flagellation,if=combo_points>=5&cooldown.shadow_blades.remains<=3|fight_remains<=25" );

  race->add_action( "blood_fury,if=variable.racial_sync", "Race Cooldowns" );
  race->add_action( "berserking,if=variable.racial_sync" );
  race->add_action( "fireblood,if=variable.racial_sync&buff.shadow_dance.up" );
  race->add_action( "ancestral_call,if=variable.racial_sync" );
  race->add_action( "invoke_external_buff,name=power_infusion,if=buff.shadow_dance.up" );

  item->add_action( "use_item,name=treacherous_transmitter,if=cooldown.flagellation.remains<=2|fight_remains<=15", "Trinket and Items" );
  item->add_action( "do_treacherous_transmitter_task,if=buff.shadow_dance.up|fight_remains<=15" );
  item->add_action( "use_item,name=imperfect_ascendancy_serum,use_off_gcd=1,if=dot.rupture.ticking&buff.flagellation_buff.up" );
  item->add_action( "use_item,name=cursed_stone_idol,use_off_gcd=1,if=dot.rupture.remains>=25&buff.flagellation_buff.up|fight_remains<=20" );
  item->add_action( "use_item,name=unyielding_netherprism,use_off_gcd=1,if=buff.shadow_blades.up&(buff.latent_energy.stack>=8+8*(trinket.arazs_ritual_forge.cooldown.ready|!equipped.arazs_ritual_forge)|!equipped.arazs_ritual_forge&fight_remains<=90)|fight_remains<=20" );
  item->add_action( "use_item,name=mad_queens_mandate,if=(!talent.lingering_darkness|buff.lingering_darkness.up|equipped.treacherous_transmitter)&(!equipped.treacherous_transmitter|trinket.treacherous_transmitter.cooldown.remains>20)|fight_remains<=15" );
  item->add_action( "use_items,slots=trinket1,if=(variable.trinket_sync_slot=1&(buff.shadow_blades.up|fight_remains<=20)|(variable.trinket_sync_slot=2&(!trinket.2.cooldown.ready&cooldown.shadow_blades.remains>20))|!variable.trinket_sync_slot)" );
  item->add_action( "use_items,slots=trinket2,if=(variable.trinket_sync_slot=2&(buff.shadow_blades.up|fight_remains<=20)|(variable.trinket_sync_slot=1&(!trinket.1.cooldown.ready&cooldown.shadow_blades.remains>20))|!variable.trinket_sync_slot)" );

  stealth_cds->add_action( "shadow_dance,if=(variable.shd_cp|!talent.premeditation)&variable.maintenance&(cooldown.secret_technique.remains<=24|talent.the_first_dance&buff.shadow_blades.up)&(buff.symbols_of_death.remains>=6|buff.shadow_blades.remains>=6)|fight_remains<=10", "Shadow Dance, Vanish, Shadowmeld" );
  stealth_cds->add_action( "vanish,if=energy>=40&!buff.subterfuge.up&effective_combo_points<=3" );
  stealth_cds->add_action( "shadowmeld,if=energy>=40&combo_points.deficit>=3" );

  finish->add_action( "secret_technique,if=variable.secret" );
  finish->add_action( "rupture,if=!variable.skip_rupture&(!dot.rupture.ticking|refreshable|buff.flagellation_buff.up&!buff.symbols_of_death.up&variable.targets<=2)&target.time_to_die-remains>6&cooldown.flagellation.remains>=10", "Maintenance Finisher" );
  finish->add_action( "rupture,cycle_targets=1,if=!variable.skip_rupture&!variable.priority_rotation&target.time_to_die>=(2*combo_points)&refreshable&variable.targets>=2" );
  finish->add_action( "coup_de_grace,if=debuff.fazed.up&cooldown.flagellation.remains>=20|fight_remains<=10", "Direct Damage Finisher" );
  finish->add_action( "black_powder,if=!variable.priority_rotation&variable.maintenance&(((variable.targets>=2&talent.deathstalkers_mark&(!buff.darkest_night.up|buff.shadow_dance.up&variable.targets>=5))|talent.unseen_blade&variable.targets>=4)|action.coup_de_grace.ready&variable.targets>=3)" );
  finish->add_action( "eviscerate,if=cooldown.flagellation.remains>=10|variable.targets>=3" );

  build->add_action( "backstab,if=(talent.unseen_blade|variable.targets<=2)&(buff.shadow_dance.up&(buff.premeditation.up|buff.shadow_blades.up)&!used_for_danse|!variable.stealth&buff.shadow_blades.up)" );
  build->add_action( "gloomblade,if=buff.shadow_dance.up&!used_for_danse|!variable.stealth&buff.shadow_blades.up" );
  build->add_action( "shadowstrike,cycle_targets=1,if=debuff.find_weakness.remains<=2&variable.targets=2&talent.unseen_blade|!used_for_danse&!talent.premeditation" );
  build->add_action( "shuriken_tornado,if=buff.lingering_darkness.up|talent.deathstalkers_mark&cooldown.shadow_blades.remains>=32&variable.targets>=3" );
  build->add_action( "shuriken_tornado,if=talent.unseen_blade&!buff.stealth.up&((buff.shadow_dance.up&!talent.shadowcraft&variable.targets>=3)|(talent.shadowcraft&variable.targets>=3)|!variable.stealth&variable.targets<=2)&(buff.symbols_of_death.up|!raid_event.adds.up)" );
  build->add_action( "shuriken_storm,if=buff.clear_the_witnesses.up&(variable.targets>=2|!buff.symbols_of_death.up)" );
  build->add_action( "shadowstrike,cycle_targets=1,if=talent.deathstalkers_mark&!debuff.deathstalkers_mark.up&variable.targets>=3&(buff.shadow_blades.up|buff.premeditation.up|talent.the_rotten)" );
  build->add_action( "shuriken_storm,if=talent.deathstalkers_mark&variable.targets>=(2+3*buff.shadow_dance.up)" );
  build->add_action( "shuriken_storm,if=talent.unseen_blade&(buff.flawless_form.up&variable.targets>=3&!variable.stealth|buff.silent_storm.up&variable.targets>=5&buff.shadow_dance.up)" );
  build->add_action( "shuriken_storm,if=(buff.tww3_trickster_4pc.up|buff.escalating_blade.stack=4)&!used_for_danse&(buff.shadow_blades.up|variable.targets>=4)" );
  build->add_action( "shadowstrike" );
  build->add_action( "goremaws_bite,if=combo_points.deficit>=3" );
  build->add_action( "gloomblade" );
  build->add_action( "backstab" );

  fill->add_action( "arcane_torrent,if=energy.deficit>=15+energy.regen", "This list usually contains Cooldowns with neglectable impact that causes global cooldowns" );
  fill->add_action( "arcane_pulse" );
  fill->add_action( "lights_judgment" );
  fill->add_action( "bag_of_tricks" );
}
//subtlety_apl_end

} // namespace rogue_apl
