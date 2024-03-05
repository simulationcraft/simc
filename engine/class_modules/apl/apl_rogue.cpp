#include "simulationcraft.hpp"
#include "class_modules/apl/apl_rogue.hpp"

namespace rogue_apl {

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
  return ( ( p->true_level >= 61 ) ? "iced_phial_of_corrupting_rage_3" :
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

//assassination_apl_start
void assassination( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* direct = p->get_action_priority_list( "direct" );
  action_priority_list_t* dot = p->get_action_priority_list( "dot" );
  action_priority_list_t* items = p->get_action_priority_list( "items" );
  action_priority_list_t* misc_cds = p->get_action_priority_list( "misc_cds" );
  action_priority_list_t* shiv = p->get_action_priority_list( "shiv" );
  action_priority_list_t* stealthed = p->get_action_priority_list( "stealthed" );
  action_priority_list_t* vanish = p->get_action_priority_list( "vanish" );

  precombat->add_action( "apply_poison" );
  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=trinket_sync_slot,value=1,if=trinket.1.has_stat.any_dps&(!trinket.2.has_stat.any_dps|trinket.1.cooldown.duration>=trinket.2.cooldown.duration)&!trinket.2.is.witherbarks_branch|trinket.1.is.witherbarks_branch", "Check which trinket slots have Stat Values" );
  precombat->add_action( "variable,name=trinket_sync_slot,value=2,if=trinket.2.has_stat.any_dps&(!trinket.1.has_stat.any_dps|trinket.2.cooldown.duration>trinket.1.cooldown.duration)&!trinket.1.is.witherbarks_branch|trinket.2.is.witherbarks_branch" );
  precombat->add_action( "stealth" );
  precombat->add_action( "slice_and_dice,precombat_seconds=1", "Pre-cast Slice and Dice if possible" );

  default_->add_action( "stealth", "Restealth if possible (no vulnerable enemies in combat)" );
  default_->add_action( "kick", "Interrupt on cooldown to allow simming interactions with that" );
  default_->add_action( "variable,name=single_target,value=spell_targets.fan_of_knives<2", "Conditional to check if there is only one enemy" );
  default_->add_action( "variable,name=regen_saturated,value=energy.regen_combined>35", "Combined Energy Regen needed to saturate" );
  default_->add_action( "variable,name=not_pooling,value=(dot.deathmark.ticking|dot.kingsbane.ticking|buff.shadow_dance.up|debuff.shiv.up|cooldown.thistle_tea.full_recharge_time<20)|(buff.envenom.up&buff.envenom.remains<=2)|energy.pct>=80|fight_remains<=90,if=set_bonus.tier31_4pc", "Check if we should be using our energy" );
  default_->add_action( "variable,name=not_pooling,value=(dot.deathmark.ticking|dot.kingsbane.ticking|buff.shadow_dance.up|debuff.shiv.up|cooldown.thistle_tea.full_recharge_time<20)|energy.pct>=80,if=!set_bonus.tier31_4pc" );
  default_->add_action( "variable,name=sepsis_sync_remains,op=setif,condition=cooldown.deathmark.remains>cooldown.sepsis.remains&cooldown.deathmark.remains<fight_remains,value=cooldown.deathmark.remains,value_else=cooldown.sepsis.remains", "Next Sepsis cooldown time based on Deathmark syncing logic and remaining fight duration" );
  default_->add_action( "call_action_list,name=stealthed,if=stealthed.rogue|stealthed.improved_garrote|master_assassin_remains>0" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "slice_and_dice,if=!buff.slice_and_dice.up&dot.rupture.ticking&combo_points>=2|!talent.cut_to_the_chase&refreshable&combo_points>=4", "Put SnD up initially for Cut to the Chase, refresh with Envenom if at low duration" );
  default_->add_action( "envenom,if=talent.cut_to_the_chase&buff.slice_and_dice.up&buff.slice_and_dice.remains<5&combo_points>=4" );
  default_->add_action( "call_action_list,name=dot" );
  default_->add_action( "call_action_list,name=direct" );
  default_->add_action( "arcane_torrent,if=energy.deficit>=15+energy.regen_combined" );
  default_->add_action( "arcane_pulse" );
  default_->add_action( "lights_judgment" );
  default_->add_action( "bag_of_tricks" );

  cds->add_action( "variable,name=deathmark_ma_condition,value=!talent.master_assassin.enabled|dot.garrote.ticking", "Cooldowns Wait on Deathmark for Garrote with MA and check for Kingsbane" );
  cds->add_action( "variable,name=deathmark_kingsbane_condition,value=!talent.kingsbane|cooldown.kingsbane.remains<=2" );
  cds->add_action( "variable,name=deathmark_condition,value=!stealthed.rogue&dot.rupture.ticking&buff.envenom.up&!debuff.deathmark.up&variable.deathmark_ma_condition&variable.deathmark_kingsbane_condition", "Deathmark to be used if not stealthed, Rupture is up, and all other talent conditions are satisfied" );
  cds->add_action( "sepsis,if=dot.rupture.remains>20&(!talent.improved_garrote&dot.garrote.ticking|talent.improved_garrote&cooldown.garrote.up&dot.garrote.pmultiplier<=1)&(target.time_to_die>10|fight_remains<10)" );
  cds->add_action( "call_action_list,name=items", "Usages for various special-case Trinkets and other Cantrips if applicable" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=dot.deathmark.ticking", "Invoke Externals to Deathmark" );
  cds->add_action( "deathmark,if=variable.deathmark_condition|fight_remains<=20" );
  cds->add_action( "call_action_list,name=shiv", "Check for Applicable Shiv usage" );
  cds->add_action( "shadow_dance,if=talent.kingsbane&buff.envenom.up&(cooldown.deathmark.remains>=50|variable.deathmark_condition)", "Special Handling to Sync Shadow Dance to Kingsbane" );
  cds->add_action( "kingsbane,if=(debuff.shiv.up|cooldown.shiv.remains<6)&buff.envenom.up&(cooldown.deathmark.remains>=50|dot.deathmark.ticking)|fight_remains<=15" );
  cds->add_action( "thistle_tea,if=!buff.thistle_tea.up&(energy.deficit>=100+energy.regen_combined&(!talent.kingsbane|charges>=2)|(dot.kingsbane.ticking&dot.kingsbane.remains<6|!talent.kingsbane&dot.deathmark.ticking)|fight_remains<charges*6)", "Avoid overcapped energy, use on final 6 seconds of kingsbane, or use a charge during cooldowns when capped on charges" );
  cds->add_action( "call_action_list,name=misc_cds", "Potion/Racials/Other misc cooldowns" );
  cds->add_action( "call_action_list,name=vanish,if=!stealthed.all&master_assassin_remains=0" );
  cds->add_action( "cold_blood,if=combo_points>=4" );

  direct->add_action( "envenom,if=effective_combo_points>=4+cooldown.deathmark.ready&(variable.not_pooling|debuff.amplifying_poison.stack>=20|effective_combo_points>cp_max_spend|!variable.single_target)", "Direct Damage Abilities   Envenom at 4+ CP if not pooling, capped on amplifying poison stacks, on an animacharged CP, or in aoe" );
  direct->add_action( "variable,name=use_filler,value=combo_points.deficit>1|variable.not_pooling|!variable.single_target", "Check if we should be using a filler" );
  direct->add_action( "mutilate,if=talent.caustic_spatter&dot.rupture.ticking&(!debuff.caustic_spatter.up|debuff.caustic_spatter.remains<=2)&variable.use_filler&!variable.single_target", "Maintain Caustic Spatter" );
  direct->add_action( "ambush,if=talent.caustic_spatter&dot.rupture.ticking&(!debuff.caustic_spatter.up|debuff.caustic_spatter.remains<=2)&variable.use_filler&!variable.single_target" );
  direct->add_action( "serrated_bone_spike,if=variable.use_filler&!dot.serrated_bone_spike_dot.ticking", "Apply SBS to all targets without a debuff as priority, preferring targets dying sooner after the primary target" );
  direct->add_action( "serrated_bone_spike,target_if=min:target.time_to_die+(dot.serrated_bone_spike_dot.ticking*600),if=variable.use_filler&!dot.serrated_bone_spike_dot.ticking" );
  direct->add_action( "serrated_bone_spike,if=variable.use_filler&master_assassin_remains<0.8&(fight_remains<=5|cooldown.serrated_bone_spike.max_charges-charges_fractional<=0.25)", "Keep from capping charges or burn at the end of fights" );
  direct->add_action( "serrated_bone_spike,if=variable.use_filler&master_assassin_remains<0.8&!variable.single_target&debuff.shiv.up", "When MA is not at high duration, sync with Shiv" );
  direct->add_action( "echoing_reprimand,if=variable.use_filler|fight_remains<20" );
  direct->add_action( "fan_of_knives,if=variable.use_filler&(!priority_rotation&spell_targets.fan_of_knives>=2+stealthed.rogue+talent.dragontempered_blades)", "Fan of Knives at 2+ targets or 3+ with DTB" );
  direct->add_action( "fan_of_knives,target_if=!dot.deadly_poison_dot.ticking&(!priority_rotation|dot.garrote.ticking|dot.rupture.ticking),if=variable.use_filler&spell_targets.fan_of_knives>=3", "Fan of Knives to apply poisons if inactive on any target (or any bleeding targets with priority rotation) at 3T" );
  direct->add_action( "ambush,if=variable.use_filler&(buff.blindside.up|buff.sepsis_buff.remains<=1|stealthed.rogue)&(!dot.kingsbane.ticking|debuff.deathmark.down|buff.blindside.up)", "Ambush on Blindside/Shadow Dance, or a last resort usage of Sepsis. Do not use Ambush during Kingsbane & Deathmark." );
  direct->add_action( "mutilate,target_if=!dot.deadly_poison_dot.ticking&!debuff.amplifying_poison.up,if=variable.use_filler&spell_targets.fan_of_knives=2", "Tab-Mutilate to apply Deadly Poison at 2 targets" );
  direct->add_action( "mutilate,if=variable.use_filler", "Fallback Mutilate" );

  dot->add_action( "variable,name=scent_effective_max_stacks,value=(spell_targets.fan_of_knives*talent.scent_of_blood.rank*2)>?20", "Damage over time abilities   Check what the maximum Scent of Blood stacks is currently" );
  dot->add_action( "variable,name=scent_saturation,value=buff.scent_of_blood.stack>=variable.scent_effective_max_stacks", "We are Scent Saturated when our stack count is hitting the maximum" );
  dot->add_action( "crimson_tempest,target_if=min:remains,if=spell_targets>=3+set_bonus.tier31_4pc&refreshable&pmultiplier<=1&effective_combo_points>=4&energy.regen_combined>25&!cooldown.deathmark.ready&target.time_to_die-remains>6", "Crimson Tempest on 4+ Targets if we have enough energy regen and it is not snapshot from stealth already" );
  dot->add_action( "garrote,if=combo_points.deficit>=1&(pmultiplier<=1)&refreshable&target.time_to_die-remains>12", "Garrote upkeep, also uses it in AoE to reach energy saturation" );
  dot->add_action( "garrote,cycle_targets=1,if=combo_points.deficit>=1&(pmultiplier<=1)&refreshable&!variable.regen_saturated&spell_targets.fan_of_knives>=2&target.time_to_die-remains>12" );
  dot->add_action( "rupture,if=effective_combo_points>=4&(pmultiplier<=1)&refreshable&target.time_to_die-remains>(4+(talent.dashing_scoundrel*5)+(variable.regen_saturated*6))", "Rupture upkeep, also uses it in AoE to reach energy saturation" );
  dot->add_action( "rupture,cycle_targets=1,if=effective_combo_points>=4&(pmultiplier<=1)&refreshable&(!variable.regen_saturated|!variable.scent_saturation)&target.time_to_die-remains>(4+(talent.dashing_scoundrel*5)+(variable.regen_saturated*6))" );
  dot->add_action( "garrote,if=refreshable&combo_points.deficit>=1&(pmultiplier<=1|remains<=tick_time&spell_targets.fan_of_knives>=3)&(remains<=tick_time*2&spell_targets.fan_of_knives>=3)&(target.time_to_die-remains)>4&master_assassin_remains=0", "Garrote as a special generator for the last CP before a finisher for edge case handling" );

  items->add_action( "use_item,name=ashes_of_the_embersoul,use_off_gcd=1,if=(dot.kingsbane.ticking&dot.kingsbane.remains<=11)|fight_remains<=22", "Special Case Trinkets" );
  items->add_action( "use_item,name=algethar_puzzle_box,use_off_gcd=1,if=dot.rupture.ticking&cooldown.deathmark.remains<2|fight_remains<=22" );
  items->add_action( "use_items,slots=trinket1,if=(variable.trinket_sync_slot=1&(debuff.deathmark.up|fight_remains<=20)|(variable.trinket_sync_slot=2&(!trinket.2.cooldown.ready|!debuff.deathmark.up&cooldown.deathmark.remains>20))|!variable.trinket_sync_slot)", "Fallback case for using stat trinkets" );
  items->add_action( "use_items,slots=trinket2,if=(variable.trinket_sync_slot=2&(debuff.deathmark.up|fight_remains<=20)|(variable.trinket_sync_slot=1&(!trinket.1.cooldown.ready|!debuff.deathmark.up&cooldown.deathmark.remains>20))|!variable.trinket_sync_slot)" );

  misc_cds->add_action( "potion,if=buff.bloodlust.react|fight_remains<30|debuff.deathmark.up", "Miscellaneous Cooldowns Potion" );
  misc_cds->add_action( "blood_fury,if=debuff.deathmark.up", "Various special racials to be synced with cooldowns" );
  misc_cds->add_action( "berserking,if=debuff.deathmark.up" );
  misc_cds->add_action( "fireblood,if=debuff.deathmark.up" );
  misc_cds->add_action( "ancestral_call,if=(!talent.kingsbane&debuff.deathmark.up&debuff.shiv.up)|(talent.kingsbane&debuff.deathmark.up&dot.kingsbane.ticking&dot.kingsbane.remains<8)" );

  shiv->add_action( "shiv,if=talent.kingsbane&!talent.lightweight_shiv.enabled&buff.envenom.up&!debuff.shiv.up&dot.garrote.ticking&dot.rupture.ticking&(dot.kingsbane.ticking&dot.kingsbane.remains<8|cooldown.kingsbane.remains>=24)&(!talent.crimson_tempest.enabled|variable.single_target|dot.crimson_tempest.ticking)|fight_remains<=charges*8", "Shiv   Shiv if talented into Kingsbane; Always sync, or prioritize the last 8 seconds" );
  shiv->add_action( "shiv,if=talent.kingsbane&talent.lightweight_shiv.enabled&buff.envenom.up&!debuff.shiv.up&dot.garrote.ticking&dot.rupture.ticking&(dot.kingsbane.ticking|cooldown.kingsbane.remains<=1)|fight_remains<=charges*8" );
  shiv->add_action( "shiv,if=talent.arterial_precision&!debuff.shiv.up&dot.garrote.ticking&dot.rupture.ticking&debuff.deathmark.up|fight_remains<=charges*8", "Shiv cases for Sepsis/Arterial in special circumstances" );
  shiv->add_action( "shiv,if=talent.sepsis&!talent.kingsbane&!talent.arterial_precision&!debuff.shiv.up&dot.garrote.ticking&dot.rupture.ticking&((cooldown.shiv.charges_fractional>0.9+talent.lightweight_shiv.enabled&variable.sepsis_sync_remains>5)|dot.sepsis.ticking|dot.deathmark.ticking|fight_remains<=charges*8)" );
  shiv->add_action( "shiv,if=!talent.kingsbane&!talent.arterial_precision&!talent.sepsis&!debuff.shiv.up&dot.garrote.ticking&dot.rupture.ticking&(!talent.crimson_tempest.enabled|variable.single_target|dot.crimson_tempest.ticking)|fight_remains<=charges*8", "Fallback if no special cases apply" );

  stealthed->add_action( "pool_resource,for_next=1", "Stealthed Actions" );
  stealthed->add_action( "shiv,if=talent.kingsbane&(dot.kingsbane.ticking|cooldown.kingsbane.up)&(!debuff.shiv.up&debuff.shiv.remains<1)&buff.envenom.up", "Make sure to have Shiv up during Kingsbane as a final check" );
  stealthed->add_action( "kingsbane,if=buff.shadow_dance.remains>=2&buff.envenom.up", "Kingsbane in Shadow Dance for snapshotting Nightstalker" );
  stealthed->add_action( "envenom,if=effective_combo_points>=4&dot.kingsbane.ticking&buff.envenom.remains<=3", "Envenom to maintain the buff during Shadow Dance" );
  stealthed->add_action( "envenom,if=effective_combo_points>=4&buff.master_assassin_aura.up&!buff.shadow_dance.up&variable.single_target", "Envenom during Master Assassin in single target" );
  stealthed->add_action( "crimson_tempest,target_if=min:remains,if=spell_targets>=3+set_bonus.tier31_4pc&refreshable&effective_combo_points>=4&!cooldown.deathmark.ready&target.time_to_die-remains>6", "Crimson Tempest on 4+ targets to snapshot Nightstalker" );
  stealthed->add_action( "garrote,target_if=min:remains,if=stealthed.improved_garrote&(remains<(12-buff.sepsis_buff.remains)|pmultiplier<=1|(buff.indiscriminate_carnage.up&active_dot.garrote<spell_targets.fan_of_knives))&!variable.single_target&target.time_to_die-remains>2", "Improved Garrote: Apply or Refresh with buffed Garrotes, accounting for Sepsis buff time and Indiscriminate Carnage" );
  stealthed->add_action( "garrote,if=stealthed.improved_garrote&(pmultiplier<=1|remains<14|!variable.single_target&buff.master_assassin_aura.remains<3)&combo_points.deficit>=1+2*talent.shrouded_suffocation" );
  stealthed->add_action( "rupture,if=effective_combo_points>=4&(pmultiplier<=1)&(buff.shadow_dance.up|debuff.deathmark.up)", "Rupture in Shadow Dance to snapshot Nightstalker as a final resort" );

  vanish->add_action( "pool_resource,for_next=1,extra_amount=45", "Stealth Cooldowns   Vanish Sync for Improved Garrote with Deathmark" );
  vanish->add_action( "shadow_dance,if=!talent.kingsbane&talent.improved_garrote&cooldown.garrote.up&(dot.garrote.pmultiplier<=1|dot.garrote.refreshable)&(debuff.deathmark.up|cooldown.deathmark.remains<12|cooldown.deathmark.remains>60)&combo_points.deficit>=(spell_targets.fan_of_knives>?4)", "Shadow Dance for non-Kingsbane setups" );
  vanish->add_action( "shadow_dance,if=!talent.kingsbane&!talent.improved_garrote&talent.master_assassin&!dot.rupture.refreshable&dot.garrote.remains>3&(debuff.deathmark.up|cooldown.deathmark.remains>60)&(debuff.shiv.up|debuff.deathmark.remains<4|dot.sepsis.ticking)&dot.sepsis.remains<3" );
  vanish->add_action( "vanish,if=!talent.master_assassin&!talent.indiscriminate_carnage&talent.improved_garrote&cooldown.garrote.up&(dot.garrote.pmultiplier<=1|dot.garrote.refreshable)&(debuff.deathmark.up|cooldown.deathmark.remains<4)&combo_points.deficit>=(spell_targets.fan_of_knives>?4)", "Vanish to spread Garrote during Deathmark without Indiscriminate Carnage" );
  vanish->add_action( "pool_resource,for_next=1,extra_amount=45" );
  vanish->add_action( "vanish,if=!talent.master_assassin&talent.indiscriminate_carnage&talent.improved_garrote&cooldown.garrote.up&(dot.garrote.pmultiplier<=1|dot.garrote.refreshable)&spell_targets.fan_of_knives>2", "Vanish for cleaving Garrotes with Indiscriminate Carnage" );
  vanish->add_action( "vanish,if=talent.master_assassin&talent.kingsbane&dot.kingsbane.remains<=3&dot.kingsbane.ticking&debuff.deathmark.remains<=3&dot.deathmark.ticking", "Vanish for Master Assassin during Kingsbane" );
  vanish->add_action( "vanish,if=!talent.improved_garrote&talent.master_assassin&!dot.rupture.refreshable&dot.garrote.remains>3&debuff.deathmark.up&(debuff.shiv.up|debuff.deathmark.remains<4|dot.sepsis.ticking)&dot.sepsis.remains<3", "Vanish fallback for Master Assassin" );
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
  action_priority_list_t* stealth = p->get_action_priority_list( "stealth" );
  action_priority_list_t* stealth_cds = p->get_action_priority_list( "stealth_cds" );

  precombat->add_action( "apply_poison" );
  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "blade_flurry,precombat_seconds=3,if=talent.underhanded_upper_hand" );
  precombat->add_action( "roll_the_bones,precombat_seconds=2" );
  precombat->add_action( "adrenaline_rush,precombat_seconds=1,if=talent.improved_adrenaline_rush" );
  precombat->add_action( "slice_and_dice,precombat_seconds=1" );
  precombat->add_action( "stealth" );

  default_->add_action( "stealth", "Restealth if possible (no vulnerable enemies in combat)" );
  default_->add_action( "kick", "Interrupt on cooldown to allow simming interactions with that" );
  default_->add_action( "variable,name=rtb_reroll,value=rtb_buffs.will_lose=(rtb_buffs.will_lose.buried_treasure+rtb_buffs.will_lose.grand_melee&spell_targets.blade_flurry<2&raid_event.adds.in>10)", "Default Roll the Bones reroll rule: reroll for any buffs that aren't Buried Treasure, excluding Grand Melee in single target" );
  default_->add_action( "variable,name=rtb_reroll,if=talent.crackshot&!set_bonus.tier31_4pc,value=(!rtb_buffs.will_lose.true_bearing&talent.hidden_opportunity|!rtb_buffs.will_lose.broadside&!talent.hidden_opportunity)&rtb_buffs.will_lose<=1", "Crackshot builds without T31 should reroll for True Bearing (or Broadside without Hidden Opportunity) if we won't lose over 1 buff" );
  default_->add_action( "variable,name=rtb_reroll,if=talent.crackshot&set_bonus.tier31_4pc,value=(rtb_buffs.will_lose<=1+buff.loaded_dice.up)", "Crackshot builds with T31 should reroll if we won't lose over 1 buff (2 with Loaded Dice)" );
  default_->add_action( "variable,name=rtb_reroll,if=!talent.crackshot&talent.hidden_opportunity,value=!rtb_buffs.will_lose.skull_and_crossbones&(rtb_buffs.will_lose<2+rtb_buffs.will_lose.grand_melee&spell_targets.blade_flurry<2&raid_event.adds.in>10)", "Hidden Opportunity builds without Crackshot should reroll for Skull and Crossbones or any 2 buffs excluding Grand Melee in single target" );
  default_->add_action( "variable,name=rtb_reroll,value=variable.rtb_reroll&rtb_buffs.longer=0|rtb_buffs.normal=0&rtb_buffs.longer>=1&rtb_buffs<6&rtb_buffs.max_remains<=39&!stealthed.all&buff.loaded_dice.up", "Additional reroll rules if all active buffs will not be rolled away, not in stealth, Loaded Dice is active, and we have less than 6 buffs" );
  default_->add_action( "variable,name=rtb_reroll,op=reset,if=!(raid_event.adds.remains>12|raid_event.adds.up&(raid_event.adds.in-raid_event.adds.remains)<6|target.time_to_die>12)|fight_remains<12", "Avoid rerolls when we will not have time remaining on the fight or add wave to recoup the opportunity cost of the global" );
  default_->add_action( "variable,name=ambush_condition,value=(talent.hidden_opportunity|combo_points.deficit>=2+talent.improved_ambush+buff.broadside.up)&energy>=50" );
  default_->add_action( "variable,name=finish_condition,value=effective_combo_points>=cp_max_spend-1-(stealthed.all&talent.crackshot)", "Use finishers if at -1 from max combo points, or -2 in Stealth with Crackshot" );
  default_->add_action( "variable,name=blade_flurry_sync,value=spell_targets.blade_flurry<2&raid_event.adds.in>20|buff.blade_flurry.remains>gcd", "With multiple targets, this variable is checked to decide whether some CDs should be synced with Blade Flurry" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=stealth,if=stealthed.all", "High priority stealth list, will fall through if no conditions are met" );
  default_->add_action( "run_action_list,name=finish,if=variable.finish_condition" );
  default_->add_action( "call_action_list,name=build" );
  default_->add_action( "arcane_torrent,if=energy.base_deficit>=15+energy.regen" );
  default_->add_action( "arcane_pulse" );
  default_->add_action( "lights_judgment" );
  default_->add_action( "bag_of_tricks" );

  build->add_action( "echoing_reprimand", "Builders" );
  build->add_action( "ambush,if=talent.hidden_opportunity&buff.audacity.up", "High priority Ambush for Hidden Opportunity builds" );
  build->add_action( "pistol_shot,if=talent.fan_the_hammer&talent.audacity&talent.hidden_opportunity&buff.opportunity.up&!buff.audacity.up", "With Audacity + Hidden Opportunity + Fan the Hammer, consume Opportunity to proc Audacity any time Ambush is not available" );
  build->add_action( "pistol_shot,if=talent.fan_the_hammer&buff.opportunity.up&(buff.opportunity.stack>=buff.opportunity.max_stack|buff.opportunity.remains<2)", "With Fan the Hammer, consume Opportunity as a higher priority if at max stacks or if it will expire" );
  build->add_action( "pistol_shot,if=talent.fan_the_hammer&buff.opportunity.up&(combo_points.deficit>=(1+(talent.quick_draw+buff.broadside.up)*(talent.fan_the_hammer.rank+1))|combo_points<=talent.ruthlessness)", "With Fan the Hammer, consume Opportunity if it will not overcap CPs, or with 1 CP at minimum" );
  build->add_action( "pistol_shot,if=!talent.fan_the_hammer&buff.opportunity.up&(energy.base_deficit>energy.regen*1.5|combo_points.deficit<=1+buff.broadside.up|talent.quick_draw.enabled|talent.audacity.enabled&!buff.audacity.up)", "If not using Fan the Hammer, then consume Opportunity based on energy, when it will exactly cap CPs, or when using Quick Draw" );
  build->add_action( "pool_resource,for_next=1", "Fallback pooling just so Sinister Strike is never casted if Ambush is available for Hidden Opportunity builds" );
  build->add_action( "ambush,if=talent.hidden_opportunity" );
  build->add_action( "sinister_strike" );

  cds->add_action( "adrenaline_rush,if=!buff.adrenaline_rush.up&(!variable.finish_condition|!talent.improved_adrenaline_rush)|stealthed.all&talent.crackshot&talent.improved_adrenaline_rush&combo_points<=2", "Cooldowns  Use Adrenaline Rush if it is not active and the finisher condition is not met, but Crackshot builds can refresh it with 2cp or lower inside stealth" );
  cds->add_action( "blade_flurry,if=spell_targets>=2-(talent.underhanded_upper_hand&!stealthed.all&buff.adrenaline_rush.up)&buff.blade_flurry.remains<gcd", "Maintain Blade Flurry on 2+ targets, and on single target with Underhanded during Adrenaline Rush" );
  cds->add_action( "blade_flurry,if=talent.deft_maneuvers&!variable.finish_condition&(spell_targets>=3&combo_points.deficit=spell_targets+buff.broadside.up|spell_targets>=5)", "With Deft Maneuvers, use Blade Flurry on cooldown at 5+ targets, or at 3-4 targets if missing combo points equal to the amount given" );
  cds->add_action( "roll_the_bones,if=variable.rtb_reroll|rtb_buffs=0|rtb_buffs.max_remains<=2&set_bonus.tier31_4pc|rtb_buffs.max_remains<=7&(cooldown.shadow_dance.ready|cooldown.vanish.ready)", "Use Roll the Bones if reroll conditions are met, or with no buffs, or 2s before buffs expire with T31, or 7s before buffs expire with Vanish/Dance ready" );
  cds->add_action( "keep_it_rolling,if=!variable.rtb_reroll&rtb_buffs>=3+set_bonus.tier31_4pc&(buff.shadow_dance.down|rtb_buffs>=6)", "Use Keep it Rolling with at least 3 buffs (4 with T31)" );
  cds->add_action( "ghostly_strike,if=effective_combo_points<cp_max_spend" );
  cds->add_action( "sepsis,if=talent.crackshot&cooldown.between_the_eyes.ready&variable.finish_condition&!stealthed.all|!talent.crackshot&target.time_to_die>11&buff.between_the_eyes.up|fight_remains<11", "Use Sepsis to trigger Crackshot or if the target will survive its DoT" );
  cds->add_action( "call_action_list,name=stealth_cds,if=!stealthed.all&(!talent.crackshot|cooldown.between_the_eyes.ready)", "Crackshot builds use stealth cooldowns if Between the Eyes is ready" );
  cds->add_action( "thistle_tea,if=!buff.thistle_tea.up&(energy.base_deficit>=100|fight_remains<charges*6)" );
  cds->add_action( "blade_rush,if=energy.base_time_to_max>4&!stealthed.all", "Use Blade Rush at minimal energy outside of stealth" );
  cds->add_action( "potion,if=buff.bloodlust.react|fight_remains<30|buff.adrenaline_rush.up" );
  cds->add_action( "blood_fury" );
  cds->add_action( "berserking" );
  cds->add_action( "fireblood" );
  cds->add_action( "ancestral_call" );
  cds->add_action( "use_item,name=manic_grieftorch,use_off_gcd=1,if=gcd.remains>gcd.max-0.1&!stealthed.all&buff.between_the_eyes.up|fight_remains<=5", "Default conditions for usable items." );
  cds->add_action( "use_item,name=dragonfire_bomb_dispenser,use_off_gcd=1,if=(!trinket.1.is.dragonfire_bomb_dispenser&trinket.1.cooldown.remains>10|trinket.2.cooldown.remains>10)|cooldown.dragonfire_bomb_dispenser.charges>2|fight_remains<20|!trinket.2.has_cooldown|!trinket.1.has_cooldown" );
  cds->add_action( "use_item,name=beacon_to_the_beyond,use_off_gcd=1,if=gcd.remains>gcd.max-0.1&!stealthed.all&buff.between_the_eyes.up|fight_remains<=5" );
  cds->add_action( "use_item,name=stormeaters_boon,if=spell_targets.blade_flurry>desired_targets|raid_event.adds.in>60|fight_remains<10" );
  cds->add_action( "use_item,name=windscar_whetstone,if=spell_targets.blade_flurry>desired_targets|raid_event.adds.in>60|fight_remains<7" );
  cds->add_action( "use_items,slots=trinket1,if=buff.between_the_eyes.up|trinket.1.has_stat.any_dps|fight_remains<=20" );
  cds->add_action( "use_items,slots=trinket2,if=buff.between_the_eyes.up|trinket.2.has_stat.any_dps|fight_remains<=20" );

  finish->add_action( "between_the_eyes,if=!talent.crackshot&(buff.between_the_eyes.remains<4|talent.improved_between_the_eyes|talent.greenskins_wickers|set_bonus.tier30_4pc)&!buff.greenskins_wickers.up", "Finishers  Use Between the Eyes to keep the crit buff up, but on cooldown if Improved/Greenskins/T30, and avoid overriding Greenskins" );
  finish->add_action( "between_the_eyes,if=talent.crackshot&cooldown.vanish.remains>45&cooldown.shadow_dance.remains>12", "Crackshot builds use Between the Eyes outside of Stealth if Vanish or Dance will not come off cooldown within the next cast" );
  finish->add_action( "slice_and_dice,if=buff.slice_and_dice.remains<fight_remains&refreshable" );
  finish->add_action( "killing_spree,if=debuff.ghostly_strike.up|!talent.ghostly_strike" );
  finish->add_action( "cold_blood" );
  finish->add_action( "dispatch" );

  stealth->add_action( "blade_flurry,if=talent.subterfuge&talent.hidden_opportunity&spell_targets>=2&buff.blade_flurry.remains<gcd", "Stealth" );
  stealth->add_action( "cold_blood,if=variable.finish_condition" );
  stealth->add_action( "between_the_eyes,if=variable.finish_condition&talent.crackshot&(!buff.shadowmeld.up|stealthed.rogue)", "High priority Between the Eyes for Crackshot, except not directly out of Shadowmeld" );
  stealth->add_action( "dispatch,if=variable.finish_condition" );
  stealth->add_action( "pistol_shot,if=talent.crackshot&talent.fan_the_hammer.rank>=2&buff.opportunity.stack>=6&(buff.broadside.up&combo_points<=1|buff.greenskins_wickers.up)", "2 Fan the Hammer Crackshot builds can consume Opportunity in stealth with max stacks, Broadside, and low CPs, or with Greenskins active" );
  stealth->add_action( "ambush,if=talent.hidden_opportunity" );

  stealth_cds->add_action( "variable,name=vanish_opportunity_condition,value=!talent.shadow_dance&talent.fan_the_hammer.rank+talent.quick_draw+talent.audacity<talent.count_the_odds+talent.keep_it_rolling", "Stealth Cooldowns" );
  stealth_cds->add_action( "vanish,if=talent.hidden_opportunity&!talent.crackshot&!buff.audacity.up&(variable.vanish_opportunity_condition|buff.opportunity.stack<buff.opportunity.max_stack)&variable.ambush_condition", "Hidden Opportunity builds without Crackshot use Vanish if Audacity is not active and when under max Opportunity stacks" );
  stealth_cds->add_action( "vanish,if=(!talent.hidden_opportunity|talent.crackshot)&variable.finish_condition", "Crackshot builds or builds without Hidden Opportunity use Vanish at finish condition" );
  stealth_cds->add_action( "shadow_dance,if=talent.crackshot&variable.finish_condition", "Crackshot builds use Dance at finish condition" );
  stealth_cds->add_action( "variable,name=shadow_dance_condition,value=buff.between_the_eyes.up&(!talent.hidden_opportunity|!buff.audacity.up&(talent.fan_the_hammer.rank<2|!buff.opportunity.up))&!talent.crackshot", "Hidden Opportunity builds without Crackshot use Dance if Audacity and Opportunity are not active" );
  stealth_cds->add_action( "shadow_dance,if=!talent.keep_it_rolling&variable.shadow_dance_condition&buff.slice_and_dice.up&(variable.finish_condition|talent.hidden_opportunity)&(!talent.hidden_opportunity|!cooldown.vanish.ready)" );
  stealth_cds->add_action( "shadow_dance,if=talent.keep_it_rolling&variable.shadow_dance_condition&(cooldown.keep_it_rolling.remains<=30|cooldown.keep_it_rolling.remains>120&(variable.finish_condition|talent.hidden_opportunity))", "Keep it Rolling builds without Crackshot use Dance at finish condition but hold it for an upcoming Keep it Rolling" );
  stealth_cds->add_action( "shadowmeld,if=variable.finish_condition&!cooldown.vanish.ready&!cooldown.shadow_dance.ready" );
}
//outlaw_apl_end

//subtlety_apl_start
void subtlety( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* build = p->get_action_priority_list( "build" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* finish = p->get_action_priority_list( "finish" );
  action_priority_list_t* stealth_cds = p->get_action_priority_list( "stealth_cds" );
  action_priority_list_t* stealthed = p->get_action_priority_list( "stealthed" );

  precombat->add_action( "apply_poison" );
  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "stealth" );
  precombat->add_action( "variable,name=algethar_puzzle_box_precombat_cast,value=3" );
  precombat->add_action( "use_item,name=algethar_puzzle_box" );
  precombat->add_action( "slice_and_dice,precombat_seconds=1" );

  default_->add_action( "stealth", "Restealth if possible (no vulnerable enemies in combat)" );
  default_->add_action( "kick", "Interrupt on cooldown to allow simming interactions with that" );
  default_->add_action( "variable,name=snd_condition,value=buff.slice_and_dice.up|spell_targets.shuriken_storm>=cp_max_spend", "Used to determine whether cooldowns wait for SnD based on targets." );
  default_->add_action( "call_action_list,name=cds", "Check CDs at first" );
  default_->add_action( "slice_and_dice,if=spell_targets.shuriken_storm<cp_max_spend&buff.slice_and_dice.remains<gcd.max&fight_remains>6&combo_points>=4", "Apply Slice and Dice at 4+ CP if it expires within the next GCD or is not up" );
  default_->add_action( "run_action_list,name=stealthed,if=stealthed.all", "Run fully switches to the Stealthed Rotation (by doing so, it forces pooling if nothing is available)." );
  default_->add_action( "variable,name=priority_rotation,value=priority_rotation", "Only change rotation if we have priority_rotation set." );
  default_->add_action( "variable,name=stealth_threshold,value=20+talent.vigor.rank*25+talent.thistle_tea*20+talent.shadowcraft*20", "Used to define when to use stealth CDs or builders" );
  default_->add_action( "variable,name=stealth_helper,value=energy>=variable.stealth_threshold" );
  default_->add_action( "variable,name=stealth_helper,value=energy.deficit<=variable.stealth_threshold,if=!talent.vigor|talent.shadowcraft" );
  default_->add_action( "call_action_list,name=stealth_cds,if=variable.stealth_helper|talent.invigorating_shadowdust", "Consider using a Stealth CD when reaching the energy threshold" );
  default_->add_action( "call_action_list,name=finish,if=effective_combo_points>=cp_max_spend" );
  default_->add_action( "call_action_list,name=finish,if=combo_points.deficit<=1|fight_remains<=1&effective_combo_points>=3", "Finish at maximum or close to maximum combo point value" );
  default_->add_action( "call_action_list,name=finish,if=spell_targets.shuriken_storm>=4&effective_combo_points>=4" );
  default_->add_action( "call_action_list,name=build,if=energy.deficit<=variable.stealth_threshold", "Use a builder when reaching the energy threshold" );
  default_->add_action( "arcane_torrent,if=energy.deficit>=15+energy.regen", "Lowest priority in all of the APL because it causes a GCD" );
  default_->add_action( "arcane_pulse" );
  default_->add_action( "lights_judgment" );
  default_->add_action( "bag_of_tricks" );

  build->add_action( "shuriken_storm,if=spell_targets>=2+(talent.gloomblade&buff.lingering_shadow.remains>=6|buff.perforated_veins.up)", "Builders  Keep using Shuriken Storm for Lingering Shadows on high stacks." );
  build->add_action( "gloomblade" );
  build->add_action( "backstab" );

  cds->add_action( "variable,name=trinket_conditions,value=(!equipped.witherbarks_branch|equipped.witherbarks_branch&trinket.witherbarks_branch.cooldown.remains<=8|equipped.bandolier_of_twisted_blades|talent.invigorating_shadowdust)", "Cooldowns  Helper Variable for Flagellation for trinket synchronisation" );
  cds->add_action( "cold_blood,if=!talent.secret_technique&combo_points>=5", "Cold Blood on 5 combo points when not playing Secret Technique" );
  cds->add_action( "sepsis,if=variable.snd_condition&target.time_to_die>=16&(buff.perforated_veins.up|!talent.perforated_veins)" );
  cds->add_action( "flagellation,target_if=max:target.time_to_die,if=variable.snd_condition&combo_points>=5&target.time_to_die>10&(variable.trinket_conditions&cooldown.shadow_blades.remains<=3|fight_remains<=28|cooldown.shadow_blades.remains>=14&talent.invigorating_shadowdust&talent.shadow_dance)&(!talent.invigorating_shadowdust|talent.sepsis|!talent.shadow_dance|talent.invigorating_shadowdust.rank=2&spell_targets.shuriken_storm>=2|cooldown.symbols_of_death.remains<=3|buff.symbols_of_death.remains>3)", "Defines Flagellation use in a stacked manner with trinkets and Shadow Blades" );
  cds->add_action( "symbols_of_death,if=variable.snd_condition&(!buff.the_rotten.up|!set_bonus.tier30_2pc)&buff.symbols_of_death.remains<=3&(!talent.flagellation|cooldown.flagellation.remains>10|buff.shadow_dance.remains>=2&talent.invigorating_shadowdust|cooldown.flagellation.up&combo_points>=5&!talent.invigorating_shadowdust)", "Align Symbols of Death to Flagellation." );
  cds->add_action( "shadow_blades,if=variable.snd_condition&(combo_points<=1|set_bonus.tier31_4pc)&(buff.flagellation_buff.up|buff.flagellation_persist.up|!talent.flagellation)", "Align Shadow Blades to Flagellation." );
  cds->add_action( "echoing_reprimand,if=variable.snd_condition&combo_points.deficit>=3", "ER during Shadow Dance." );
  cds->add_action( "shuriken_tornado,if=variable.snd_condition&buff.symbols_of_death.up&combo_points<=2&!buff.premeditation.up&(!talent.flagellation|cooldown.flagellation.remains>20)&spell_targets.shuriken_storm>=3", "Shuriken Tornado with Symbols of Death on 3 and more targets" );
  cds->add_action( "shuriken_tornado,if=variable.snd_condition&!buff.shadow_dance.up&!buff.flagellation_buff.up&!buff.flagellation_persist.up&!buff.shadow_blades.up&spell_targets.shuriken_storm<=2&!raid_event.adds.up", "Shuriken Tornado only outside of cooldowns" );
  cds->add_action( "shadow_dance,if=!buff.shadow_dance.up&fight_remains<=8+talent.subterfuge.enabled" );
  cds->add_action( "goremaws_bite,if=variable.snd_condition&combo_points.deficit>=3&(!cooldown.shadow_dance.up|talent.shadow_dance&buff.shadow_dance.up&!talent.invigorating_shadowdust|spell_targets.shuriken_storm<4&!talent.invigorating_shadowdust|talent.the_rotten|raid_event.adds.up)", "Goremaws Bite during Shadow Dance if possible." );
  cds->add_action( "thistle_tea,if=!buff.thistle_tea.up&cooldown.thistle_tea.charges_fractional>=2.5&buff.shadow_dance.remains>=4", "Thistle Tea during Shadow Dance when close to max stacks." );
  cds->add_action( "thistle_tea,if=!buff.thistle_tea.up&buff.shadow_dance.remains>=4&cooldown.secret_technique.remains<=10", "Thistle Tea during Shadow Dance when Secret Techniques is up." );
  cds->add_action( "thistle_tea,if=!buff.thistle_tea.up&(energy.deficit>=(100)|!buff.thistle_tea.up&fight_remains<=(6*cooldown.thistle_tea.charges))&(cooldown.symbols_of_death.remains>=3|buff.symbols_of_death.up)&combo_points.deficit>=2", "Thistle Tea for energy" );
  cds->add_action( "potion,if=buff.bloodlust.react|fight_remains<30|buff.symbols_of_death.up&(buff.shadow_blades.up|cooldown.shadow_blades.remains<=10)" );
  cds->add_action( "variable,name=racial_sync,value=buff.shadow_blades.up|!talent.shadow_blades&buff.symbols_of_death.up|fight_remains<20" );
  cds->add_action( "blood_fury,if=variable.racial_sync" );
  cds->add_action( "berserking,if=variable.racial_sync" );
  cds->add_action( "fireblood,if=variable.racial_sync" );
  cds->add_action( "ancestral_call,if=variable.racial_sync" );
  cds->add_action( "use_item,name=ashes_of_the_embersoul,if=(buff.cold_blood.up|(!talent.danse_macabre&buff.shadow_dance.up|buff.danse_macabre.stack>=3)&!talent.cold_blood)|fight_remains<10", "Sync specific trinkets to Flagellation or Shadow Dance." );
  cds->add_action( "use_item,name=witherbarks_branch,if=buff.flagellation_buff.up&talent.invigorating_shadowdust|buff.shadow_blades.up|equipped.bandolier_of_twisted_blades&raid_event.adds.up" );
  cds->add_action( "use_item,name=mirror_of_fractured_tomorrows,if=buff.shadow_dance.up&(target.time_to_die>=15|equipped.ashes_of_the_embersoul)" );
  cds->add_action( "use_item,name=beacon_to_the_beyond,use_off_gcd=1,if=!stealthed.all&(buff.deeper_daggers.up|!talent.deeper_daggers)&(!raid_event.adds.up|!equipped.stormeaters_boon|trinket.stormeaters_boon.cooldown.remains>20)" );
  cds->add_action( "use_item,name=manic_grieftorch,use_off_gcd=1,if=!stealthed.all&(!raid_event.adds.up|!equipped.stormeaters_boon|trinket.stormeaters_boon.cooldown.remains>20)" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.shadow_dance.up", "PI" );
  cds->add_action( "use_items,if=!stealthed.all&(!trinket.mirror_of_fractured_tomorrows.cooldown.ready|!equipped.mirror_of_fractured_tomorrows)&(!trinket.ashes_of_the_embersoul.cooldown.ready|!equipped.ashes_of_the_embersoul)|fight_remains<10", "Default fallback for usable items: Use outside of Stealth/Shadow Dance." );

  finish->add_action( "variable,name=secret_condition,value=(action.gloomblade.used_for_danse|action.shadowstrike.used_for_danse|action.backstab.used_for_danse|action.shuriken_storm.used_for_danse)&(action.eviscerate.used_for_danse|action.black_powder.used_for_danse|action.rupture.used_for_danse)|!talent.danse_macabre", "Finisher  Defines what abilities need to be used for DM stacks before casting Secret Technique." );
  finish->add_action( "rupture,if=!dot.rupture.ticking&target.time_to_die-remains>6", "Apply Rupture if its not up." );
  finish->add_action( "variable,name=premed_snd_condition,value=talent.premeditation.enabled&spell_targets.shuriken_storm<5" );
  finish->add_action( "slice_and_dice,if=!stealthed.all&!variable.premed_snd_condition&spell_targets.shuriken_storm<6&!buff.shadow_dance.up&buff.slice_and_dice.remains<fight_remains&refreshable", "Refresh Slice and Dice outside of Shadow Dance." );
  finish->add_action( "variable,name=skip_rupture,value=buff.thistle_tea.up&spell_targets.shuriken_storm=1|buff.shadow_dance.up&(spell_targets.shuriken_storm=1|dot.rupture.ticking&spell_targets.shuriken_storm>=2)", "Variable to decide when not to use Rupture." );
  finish->add_action( "rupture,if=(!variable.skip_rupture|variable.priority_rotation)&target.time_to_die-remains>6&refreshable" );
  finish->add_action( "rupture,if=buff.finality_rupture.up&buff.shadow_dance.up&spell_targets.shuriken_storm<=4&!action.rupture.used_for_danse", "Refresh Rupture during Shadow Dance with Finality." );
  finish->add_action( "cold_blood,if=variable.secret_condition&cooldown.secret_technique.ready" );
  finish->add_action( "secret_technique,if=variable.secret_condition&(!talent.cold_blood|cooldown.cold_blood.remains>buff.shadow_dance.remains-2|!talent.improved_shadow_dance)", "Synchronizes Secret to Cold Blood if possible. Defaults to use once a builder and finisher is used." );
  finish->add_action( "rupture,cycle_targets=1,if=!variable.skip_rupture&!variable.priority_rotation&spell_targets.shuriken_storm>=2&target.time_to_die>=(2*combo_points)&refreshable", "Multidotting targets that will live long enough, refresh during pandemic." );
  finish->add_action( "rupture,if=!variable.skip_rupture&remains<cooldown.symbols_of_death.remains+10&cooldown.symbols_of_death.remains<=5&target.time_to_die-remains>cooldown.symbols_of_death.remains+5", "Refresh Rupture early if it will expire during Symbols. Do that refresh if SoD gets ready in the next 5s." );
  finish->add_action( "black_powder,if=!variable.priority_rotation&spell_targets>=3" );
  finish->add_action( "eviscerate" );

  stealth_cds->add_action( "variable,name=shd_threshold,value=cooldown.shadow_dance.charges_fractional>=0.75+talent.shadow_dance", "Stealth Cooldowns  Helper Variable for Shadow Dance." );
  stealth_cds->add_action( "variable,name=rotten_cb,value=(!buff.the_rotten.up|!set_bonus.tier30_2pc)&(!talent.cold_blood|cooldown.cold_blood.remains<4|cooldown.cold_blood.remains>10)", "Helper variable to check for Cold Blood and The Rotten buff." );
  stealth_cds->add_action( "vanish,if=(combo_points.deficit>1|buff.shadow_blades.up&talent.invigorating_shadowdust)&!variable.shd_threshold&(cooldown.flagellation.remains>=60|!talent.flagellation|fight_remains<=(30*cooldown.vanish.charges))&(cooldown.symbols_of_death.remains>3|!set_bonus.tier30_2pc)&(cooldown.secret_technique.remains>=10|!talent.secret_technique|cooldown.vanish.charges>=2&talent.invigorating_shadowdust&(buff.the_rotten.up|!talent.the_rotten)&!raid_event.adds.up)", "Consider Flagellation, Symbols and Secret Technique cooldown when using Vanish with Shadow Dust." );
  stealth_cds->add_action( "pool_resource,for_next=1,extra_amount=40,if=race.night_elf", "Pool for Shadowmeld unless we are about to cap on Dance charges." );
  stealth_cds->add_action( "shadowmeld,if=energy>=40&energy.deficit>=10&!variable.shd_threshold&combo_points.deficit>4" );
  stealth_cds->add_action( "variable,name=shd_combo_points,value=combo_points.deficit>=3" );
  stealth_cds->add_action( "shadow_dance,if=(dot.rupture.ticking|talent.invigorating_shadowdust)&variable.rotten_cb&(!talent.the_first_dance|combo_points.deficit>=4|buff.shadow_blades.up)&(variable.shd_combo_points&variable.shd_threshold|(buff.shadow_blades.up|cooldown.symbols_of_death.up&!talent.sepsis|buff.symbols_of_death.remains>=4&!set_bonus.tier30_2pc|!buff.symbols_of_death.remains&set_bonus.tier30_2pc)&cooldown.secret_technique.remains<10+12*(!talent.invigorating_shadowdust|set_bonus.tier30_2pc))", "Shadow dance when Rupture is up and synchronize depending on talent choice." );

  stealthed->add_action( "shadowstrike,if=buff.stealth.up&(spell_targets.shuriken_storm<4|variable.priority_rotation)", "Stealthed Rotation  Always Strike from Stealth" );
  stealthed->add_action( "call_action_list,name=finish,if=effective_combo_points>=cp_max_spend", "Finish when on Animacharged combo points or max combo points." );
  stealthed->add_action( "call_action_list,name=finish,if=buff.shuriken_tornado.up&combo_points.deficit<=2" );
  stealthed->add_action( "call_action_list,name=finish,if=combo_points.deficit<=1+(talent.deeper_stratagem|talent.secret_stratagem)" );
  stealthed->add_action( "backstab,if=!buff.premeditation.up&buff.shadow_dance.remains>=3&buff.shadow_blades.up&!used_for_danse&talent.danse_macabre&spell_targets.shuriken_storm<=3&!buff.the_rotten.up", "Backstab for Danse Macabre stack generation during Shadowblades." );
  stealthed->add_action( "gloomblade,if=!buff.premeditation.up&buff.shadow_dance.remains>=3&buff.shadow_blades.up&!used_for_danse&talent.danse_macabre&spell_targets.shuriken_storm<=4", "Gloomblade for Danse Macabre stack generation during Shadowblades." );
  stealthed->add_action( "shadowstrike,if=!used_for_danse&buff.shadow_blades.up", "Shadow Strike for Danse Macabre stack generation during Shadowblades." );
  stealthed->add_action( "shuriken_storm,if=!buff.premeditation.up&spell_targets>=4" );
  stealthed->add_action( "shadowstrike" );
}
//subtlety_apl_end

} // namespace rogue_apl
