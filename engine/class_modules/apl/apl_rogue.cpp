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
  action_priority_list_t* stealthed = p->get_action_priority_list( "stealthed" );
  action_priority_list_t* vanish = p->get_action_priority_list( "vanish" );

  precombat->add_action( "apply_poison" );
  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "marked_for_death,precombat_seconds=10,if=!covenant.venthyr&raid_event.adds.in>15" );
  precombat->add_action( "fleshcraft,if=soulbind.pustule_eruption|soulbind.volatile_solvent" );
  precombat->add_action( "variable,name=deathmark_cdr,value=1-(runeforge.duskwalkers_patch*0.45)" );
  precombat->add_action( "variable,name=flagellation_cdr,value=1-(runeforge.obedience*0.44)", "The average CDR is 0.22 but due to the RNG nature of CP gen, 2x this value is optimal for syncing logic" );
  precombat->add_action( "variable,name=trinket_sync_slot,value=1,if=trinket.1.has_stat.any_dps&(!trinket.2.has_stat.any_dps|trinket.1.cooldown.duration>=trinket.2.cooldown.duration)|trinket.1.is.inscrutable_quantum_device|(covenant.venthyr&!trinket.2.has_stat.any_dps&trinket.1.is.shadowgrasp_totem)", "Determine which (if any) stat buff trinket we want to attempt to sync with Deathmark." );
  precombat->add_action( "variable,name=trinket_sync_slot,value=2,if=trinket.2.has_stat.any_dps&(!trinket.1.has_stat.any_dps|trinket.2.cooldown.duration>trinket.1.cooldown.duration)|trinket.2.is.inscrutable_quantum_device|(covenant.venthyr&!trinket.1.has_stat.any_dps&trinket.2.is.shadowgrasp_totem)" );
  precombat->add_action( "stealth" );
  precombat->add_action( "slice_and_dice,precombat_seconds=1" );

  default_->add_action( "stealth", "Restealth if possible (no vulnerable enemies in combat)" );
  default_->add_action( "kick", "Interrupt on cooldown to allow simming interactions with that" );
  default_->add_action( "variable,name=single_target,value=spell_targets.fan_of_knives<2" );
  default_->add_action( "variable,name=regen_saturated,value=energy.regen_combined>35", "Combined Energy Regen needed to saturate" );
  default_->add_action( "variable,name=deathmark_cooldown_remains,value=cooldown.deathmark.remains*variable.deathmark_cdr" );
  default_->add_action( "call_action_list,name=stealthed,if=stealthed.rogue|stealthed.improved_garrote" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "slice_and_dice,if=!buff.slice_and_dice.up&combo_points>=2", "Put SnD up initially for Cut to the Chase, refresh with Envenom if at low duration" );
  default_->add_action( "envenom,if=buff.slice_and_dice.up&buff.slice_and_dice.remains<5&combo_points>=4", "Higher priority Envenom casts for refreshing SnD or at the end of Flagellation" );
  default_->add_action( "envenom,if=buff.flagellation_buff.up&buff.flagellation_buff.remains<1&buff.flagellation_buff.stack<30&combo_points>=2" );
  default_->add_action( "call_action_list,name=dot" );
  default_->add_action( "call_action_list,name=direct" );
  default_->add_action( "arcane_torrent,if=energy.deficit>=15+energy.regen_combined" );
  default_->add_action( "arcane_pulse" );
  default_->add_action( "lights_judgment" );
  default_->add_action( "bag_of_tricks" );

  cds->add_action( "marked_for_death,line_cd=1.5,target_if=min:target.time_to_die,if=raid_event.adds.up&(!variable.single_target|target.time_to_die<30)&(target.time_to_die<combo_points.deficit*1.5|combo_points.deficit>=cp_max_spend)", "Cooldowns  If adds are up, snipe the one with lowest TTD. Use when dying faster than CP deficit or without any CP." );
  cds->add_action( "marked_for_death,if=raid_event.adds.in>30-raid_event.adds.duration&combo_points.deficit>=cp_max_spend&(!covenant.venthyr|(buff.flagellation_buff.up|cooldown.flagellation.remains>15)&(talent.crimson_tempest.enabled|!cooldown.shiv.ready))", "If no adds will die within the next 30s, use MfD for max CP. Attempt to sync with Flagellation if possible." );
  cds->add_action( "variable,name=deathmark_exsanguinate_condition,value=!talent.exsanguinate|cooldown.exsanguinate.remains>15|exsanguinated.rupture|exsanguinated.garrote", "Sync Deathmark window with Exsanguinate if applicable" );
  cds->add_action( "variable,name=deathmark_ma_condition,value=!talent.master_assassin.enabled|dot.garrote.ticking|covenant.venthyr&combo_points.deficit=0", "Wait on Deathmark for Garrote with MA, unless we are at max CP for Flagellation" );
  cds->add_action( "variable,name=deathmark_covenant_condition,if=!covenant.venthyr,value=1", "Sync Deathmark with Flagellation as long as we won't lose a cast over the fight duration" );
  cds->add_action( "variable,name=deathmark_covenant_condition,if=covenant.venthyr,value=floor((fight_remains-20)%(120*variable.deathmark_cdr))>floor((fight_remains-20-cooldown.flagellation.remains)%(120*variable.deathmark_cdr))&cooldown.flagellation.remains>10|buff.flagellation_buff.up|debuff.flagellation.up|fight_remains<20" );
  cds->add_action( "fleshcraft,if=(soulbind.pustule_eruption|soulbind.volatile_solvent)&!stealthed.all&!debuff.deathmark.up&master_assassin_remains=0&(energy.time_to_max_combined>2|!debuff.shiv.up)", "Fleshcraft for Pustule Eruption if not stealthed or in a cooldown cycle" );
  cds->add_action( "flagellation,if=!stealthed.rogue&(variable.deathmark_cooldown_remains<3&variable.deathmark_ma_condition&effective_combo_points>=4&target.time_to_die>10|debuff.deathmark.up|fight_remains<24)", "Sync Flagellation with Deathmark as long as we won't lose a cast over the fight duration" );
  cds->add_action( "flagellation,if=!stealthed.rogue&effective_combo_points>=4&(floor((fight_remains-24)%(cooldown*variable.flagellation_cdr))>floor((fight_remains-24-variable.deathmark_cooldown_remains)%(cooldown*variable.flagellation_cdr)))" );
  cds->add_action( "sepsis,if=!stealthed.rogue&dot.garrote.ticking&(target.time_to_die>10|fight_remains<10)" );
  cds->add_action( "variable,name=deathmark_condition,value=!stealthed.rogue&dot.rupture.ticking&!debuff.deathmark.up&variable.deathmark_exsanguinate_condition&variable.deathmark_ma_condition&variable.deathmark_covenant_condition", "Deathmark to be used if not stealthed, Rupture is up, and all other talent/covenant conditions are satisfied" );
  cds->add_action( "use_item,name=wraps_of_electrostatic_potential" );
  cds->add_action( "use_item,name=ring_of_collapsing_futures,if=buff.temptation.down|fight_remains<30" );
  cds->add_action( "use_items,slots=trinket1,if=(variable.trinket_sync_slot=1&(debuff.deathmark.up|fight_remains<=20)|(variable.trinket_sync_slot=2&(!trinket.2.cooldown.ready|variable.deathmark_cooldown_remains>20))|!variable.trinket_sync_slot)", "Sync the priority stat buff trinket with Deathmark, otherwise use on cooldown" );
  cds->add_action( "use_items,slots=trinket2,if=(variable.trinket_sync_slot=2&(debuff.deathmark.up|fight_remains<=20)|(variable.trinket_sync_slot=1&(!trinket.1.cooldown.ready|variable.deathmark_cooldown_remains>20))|!variable.trinket_sync_slot)" );
  cds->add_action( "deathmark,if=variable.deathmark_condition" );
  cds->add_action( "kingsbane,if=(debuff.shiv.up|cooldown.shiv.remains<6)&buff.envenom.up&(cooldown.deathmark.remains>=50|dot.deathmark.ticking)" );
  cds->add_action( "exsanguinate,if=!stealthed.rogue&!stealthed.improved_garrote&!dot.deathmark.ticking&(!dot.garrote.refreshable&dot.rupture.remains>4+4*cp_max_spend|dot.rupture.remains*0.5>target.time_to_die)&target.time_to_die>4", "Exsanguinate when not stealthed and both Rupture and Garrote are up for long enough." );
  cds->add_action( "shiv,if=talent.kingsbane&!debuff.shiv.up&dot.kingsbane.ticking&dot.garrote.ticking&dot.rupture.ticking&(!talent.crimson_tempest.enabled|variable.single_target|dot.crimson_tempest.ticking)", "Shiv if DoTs are up; if Night Fae attempt to sync with Sepsis or Deathmark if we won't waste more than half Shiv's cooldown" );
  cds->add_action( "shiv,if=!talent.kingsbane&!covenant.night_fae&!debuff.shiv.up&dot.garrote.ticking&dot.rupture.ticking&(!talent.crimson_tempest.enabled|variable.single_target|dot.crimson_tempest.ticking)" );
  cds->add_action( "shiv,if=!talent.kingsbane&covenant.night_fae&!debuff.shiv.up&dot.garrote.ticking&dot.rupture.ticking&((cooldown.sepsis.ready|cooldown.sepsis.remains>12)+(cooldown.deathmark.ready|variable.deathmark_cooldown_remains>12)=2)" );
  cds->add_action( "thistle_tea,if=energy.deficit>=100&!buff.thistle_tea.up&(charges=3|debuff.deathmark.up|fight_remains<cooldown.deathmark.remains)" );
  cds->add_action( "indiscriminate_carnage,if=(spell_targets.fan_of_knives>desired_targets|spell_targets.fan_of_knives>1&raid_event.adds.in>60)&(!talent.improved_garrote|cooldown.vanish.remains>45)" );
  cds->add_action( "potion,if=buff.bloodlust.react|fight_remains<30|debuff.deathmark.up" );
  cds->add_action( "blood_fury,if=debuff.deathmark.up" );
  cds->add_action( "berserking,if=debuff.deathmark.up" );
  cds->add_action( "fireblood,if=debuff.deathmark.up" );
  cds->add_action( "ancestral_call,if=debuff.deathmark.up" );
  cds->add_action( "call_action_list,name=vanish,if=!stealthed.all&master_assassin_remains=0" );
  cds->add_action( "use_item,name=windscar_whetstone,if=spell_targets.fan_of_knives>desired_targets|raid_event.adds.in>60|fight_remains<7" );
  cds->add_action( "use_item,name=cache_of_acquired_treasures,if=buff.acquired_axe.up&(spell_targets.fan_of_knives=1&raid_event.adds.in>60|spell_targets.fan_of_knives>1)|fight_remains<25" );
  cds->add_action( "use_item,name=bloodstained_handkerchief,target_if=max:target.time_to_die*(!dot.cruel_garrote.ticking),if=!dot.cruel_garrote.ticking" );
  cds->add_action( "use_item,name=scars_of_fraternal_strife,if=!buff.scars_of_fraternal_strife_4.up|fight_remains<30" );

  direct->add_action( "envenom,if=effective_combo_points>=4+talent.deeper_stratagem.enabled&(debuff.deathmark.up|debuff.shiv.up|debuff.amplifying_poison.stack>=10|buff.flagellation_buff.up|energy.deficit<=25+energy.regen_combined|!variable.single_target|effective_combo_points>cp_max_spend)&(!talent.exsanguinate.enabled|cooldown.exsanguinate.remains>2)", "Direct damage abilities  Envenom at 4+ (5+ with DS) CP. Immediately on 2+ targets, with Deathmark, or with TB; otherwise wait for some energy. Also wait if Exsg combo is coming up." );
  direct->add_action( "variable,name=use_filler,value=combo_points.deficit>1|energy.deficit<=25+energy.regen_combined|!variable.single_target" );
  direct->add_action( "serrated_bone_spike,if=variable.use_filler&!dot.serrated_bone_spike_dot.ticking", "Apply SBS to all targets without a debuff as priority, preferring targets dying sooner after the primary target" );
  direct->add_action( "serrated_bone_spike,target_if=min:target.time_to_die+(dot.serrated_bone_spike_dot.ticking*600),if=variable.use_filler&!dot.serrated_bone_spike_dot.ticking" );
  direct->add_action( "serrated_bone_spike,if=variable.use_filler&master_assassin_remains<0.8&(fight_remains<=5|cooldown.serrated_bone_spike.max_charges-charges_fractional<=0.25)", "Keep from capping charges or burn at the end of fights" );
  direct->add_action( "serrated_bone_spike,if=variable.use_filler&master_assassin_remains<0.8&(soulbind.lead_by_example.enabled&!buff.lead_by_example.up&debuff.deathmark.up|buff.marrowed_gemstone_enhancement.up|!variable.single_target&debuff.shiv.up)", "When MA is not at high duration, sync with damage buffs without overwriting Lead by Example" );
  direct->add_action( "fan_of_knives,if=variable.use_filler&(!priority_rotation&spell_targets.fan_of_knives>=3+stealthed.rogue+talent.dragontempered_blades)", "Fan of Knives at 19+ stacks of Hidden Blades or against 4+ targets." );
  direct->add_action( "fan_of_knives,target_if=!dot.deadly_poison_dot.ticking&(!priority_rotation|dot.garrote.ticking|dot.rupture.ticking),if=variable.use_filler&spell_targets.fan_of_knives>=3", "Fan of Knives to apply poisons if inactive on any target (or any bleeding targets with priority rotation) at 3T" );
  direct->add_action( "echoing_reprimand,if=variable.use_filler&variable.deathmark_cooldown_remains>10" );
  direct->add_action( "ambush,if=variable.use_filler&(master_assassin_remains=0&!runeforge.doomblade|buff.blindside.up)" );
  direct->add_action( "mutilate,target_if=!dot.deadly_poison_dot.ticking,if=variable.use_filler&spell_targets.fan_of_knives=2", "Tab-Mutilate to apply Deadly Poison at 2 targets" );
  direct->add_action( "mutilate,if=variable.use_filler" );

  dot->add_action( "variable,name=skip_cycle_garrote,value=priority_rotation&(dot.garrote.remains<cooldown.garrote.duration|variable.regen_saturated)", "Damage over time abilities  Limit secondary Garrotes for priority rotation if we have 35 energy regen or Garrote will expire on the primary target" );
  dot->add_action( "variable,name=skip_cycle_rupture,value=priority_rotation&(debuff.shiv.up&spell_targets.fan_of_knives>2|variable.regen_saturated)", "Limit secondary Ruptures for priority rotation if we have 35 energy regen or Shiv is up on 2T+" );
  dot->add_action( "variable,name=skip_rupture,value=debuff.deathmark.up&(debuff.shiv.up|master_assassin_remains>0)&dot.rupture.remains>2", "Limit Ruptures if Deathmark+Shiv/Master Assassin is up and we have 2+ seconds left on the Rupture DoT" );
  dot->add_action( "garrote,if=talent.exsanguinate.enabled&!will_lose_exsanguinate&dot.garrote.pmultiplier<=1&cooldown.exsanguinate.remains<2&spell_targets.fan_of_knives=1&raid_event.adds.in>6&dot.garrote.remains*0.5<target.time_to_die", "Special Garrote and Rupture setup prior to Exsanguinate cast" );
  dot->add_action( "rupture,if=talent.exsanguinate.enabled&!will_lose_exsanguinate&dot.rupture.pmultiplier<=1&(effective_combo_points>=cp_max_spend&cooldown.exsanguinate.remains<1&dot.rupture.remains*0.5<target.time_to_die)" );
  dot->add_action( "pool_resource,for_next=1", "Garrote upkeep, also tries to use it as a special generator for the last CP before a finisher" );
  dot->add_action( "garrote,if=refreshable&combo_points.deficit>=1&(pmultiplier<=1|remains<=tick_time&spell_targets.fan_of_knives>=3)&(!will_lose_exsanguinate|remains<=tick_time*2&spell_targets.fan_of_knives>=3)&(target.time_to_die-remains)>4&master_assassin_remains=0" );
  dot->add_action( "pool_resource,for_next=1" );
  dot->add_action( "garrote,cycle_targets=1,if=!variable.skip_cycle_garrote&target!=self.target&refreshable&combo_points.deficit>=1&(pmultiplier<=1|remains<=tick_time&spell_targets.fan_of_knives>=3)&(!will_lose_exsanguinate|remains<=tick_time*2&spell_targets.fan_of_knives>=3)&(target.time_to_die-remains)>12&master_assassin_remains=0" );
  dot->add_action( "crimson_tempest,target_if=min:remains,if=spell_targets>=2&effective_combo_points>=4&energy.regen_combined>20&(!cooldown.deathmark.ready|dot.rupture.ticking)&remains<(2+3*(spell_targets>=4))", "Crimson Tempest on multiple targets at 4+ CP when running out in 2-5s as long as we have enough regen and aren't setting up for Deathmark" );
  dot->add_action( "rupture,if=!variable.skip_rupture&effective_combo_points>=4&refreshable&(pmultiplier<=1|remains<=tick_time&spell_targets.fan_of_knives>=3)&(!will_lose_exsanguinate|remains<=tick_time*2&spell_targets.fan_of_knives>=3)&target.time_to_die-remains>(4+(runeforge.dashing_scoundrel*5)+(runeforge.doomblade*5)+(variable.regen_saturated*6))", "Keep up Rupture at 4+ on all targets (when living long enough and not snapshot)" );
  dot->add_action( "rupture,cycle_targets=1,if=!variable.skip_cycle_rupture&!variable.skip_rupture&target!=self.target&effective_combo_points>=4&refreshable&(pmultiplier<=1|remains<=tick_time&spell_targets.fan_of_knives>=3)&(!will_lose_exsanguinate|remains<=tick_time*2&spell_targets.fan_of_knives>=3)&target.time_to_die-remains>(4+(runeforge.dashing_scoundrel*5)+(runeforge.doomblade*5)+(variable.regen_saturated*6))" );
  dot->add_action( "crimson_tempest,if=spell_targets>=2&effective_combo_points>=4&remains<2+3*(spell_targets>=4)", "Fallback AoE Crimson Tempest with the same logic as above, but ignoring the energy conditions if we aren't using Rupture" );
  dot->add_action( "crimson_tempest,if=spell_targets=1&(!runeforge.dashing_scoundrel|rune_word.frost.enabled)&effective_combo_points>=(cp_max_spend-1)&refreshable&!will_lose_exsanguinate&!debuff.shiv.up&debuff.amplifying_poison.stack<15&target.time_to_die-remains>4", "Crimson Tempest on ST if in pandemic and nearly max energy and if Envenom won't do more damage due to TB/MA" );

  stealthed->add_action( "indiscriminate_carnage,if=spell_targets.fan_of_knives>desired_targets|spell_targets.fan_of_knives>1&raid_event.adds.in>60", "Stealthed Actions" );
  stealthed->add_action( "pool_resource,for_next=1", "Improved Garrote: Apply or Refresh with buffed Garrotes" );
  stealthed->add_action( "garrote,target_if=min:remains,if=stealthed.improved_garrote&!will_lose_exsanguinate&(remains<12%exsanguinated_rate|pmultiplier<=1)&target.time_to_die-remains>2" );
  stealthed->add_action( "pool_resource,for_next=1", "Improved Garrote + Exsg on 1T: Refresh Garrote at the end of stealth to get max duration before Exsanguinate" );
  stealthed->add_action( "garrote,if=talent.exsanguinate.enabled&stealthed.improved_garrote&active_enemies=1&!will_lose_exsanguinate&improved_garrote_remains<1.3" );

  vanish->add_action( "pool_resource,for_next=1,extra_amount=45", "Vanish  Vanish Sync for Improved Garrote with Deathmark" );
  vanish->add_action( "vanish,if=talent.improved_garrote&cooldown.garrote.up&!exsanguinated.garrote&dot.garrote.pmultiplier<=1&(debuff.deathmark.up|cooldown.deathmark.remains<4)&combo_points.deficit>=(spell_targets.fan_of_knives>?4)" );
  vanish->add_action( "pool_resource,for_next=1,extra_amount=45", "Vanish for Indiscriminate Carnage or Improved Garrote at 2-3+ targets" );
  vanish->add_action( "vanish,if=talent.improved_garrote&cooldown.garrote.up&!exsanguinated.garrote&dot.garrote.pmultiplier<=1&spell_targets.fan_of_knives>(3-talent.indiscriminate_carnage)&(!talent.indiscriminate_carnage|cooldown.indiscriminate_carnage.ready)" );
  vanish->add_action( "vanish,if=!talent.improved_garrote&(talent.master_assassin.enabled|runeforge.mark_of_the_master_assassin)&!dot.rupture.refreshable&dot.garrote.remains>3&debuff.deathmark.up&(debuff.shiv.up|debuff.deathmark.remains<4|dot.sepsis.ticking)&dot.sepsis.remains<3", "Vanish with Master Assasin: Rupture+Garrote not in refresh range, during Deathmark+Shiv. Sync with Sepsis final hit if possible." );
  vanish->add_action( "pool_resource,for_next=1,extra_amount=45" );
  vanish->add_action( "shadow_dance,if=talent.improved_garrote&cooldown.garrote.up&!exsanguinated.garrote&dot.garrote.pmultiplier<=1&(debuff.deathmark.up|cooldown.deathmark.remains<4|cooldown.deathmark.remains>60)&combo_points.deficit>=(spell_targets.fan_of_knives>?4)" );
  vanish->add_action( "shadow_dance,if=!talent.improved_garrote&(talent.master_assassin.enabled|runeforge.mark_of_the_master_assassin)&!dot.rupture.refreshable&dot.garrote.remains>3&(debuff.deathmark.up|cooldown.deathmark.remains>60)&(debuff.shiv.up|debuff.deathmark.remains<4|dot.sepsis.ticking)&dot.sepsis.remains<3", "Shadow Dance with Master Assasin: Rupture+Garrote not in refresh range, during Deathmark+Shiv. Sync with Sepsis final hit if possible." );
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

  precombat->add_action( "apply_poison" );
  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "marked_for_death,precombat_seconds=10,if=raid_event.adds.in>25" );
  precombat->add_action( "fleshcraft,if=soulbind.pustule_eruption|soulbind.volatile_solvent" );
  precombat->add_action( "adrenaline_rush,precombat_seconds=3,if=talent.improved_adrenaline_rush" );
  precombat->add_action( "roll_the_bones,precombat_seconds=2" );
  precombat->add_action( "slice_and_dice,precombat_seconds=1" );
  precombat->add_action( "stealth" );

  default_->add_action( "stealth", "Restealth if possible (no vulnerable enemies in combat)" );
  default_->add_action( "kick", "Interrupt on cooldown to allow simming interactions with that" );
  default_->add_action( "variable,name=rtb_reroll,value=rtb_buffs<2&(!buff.broadside.up&(!runeforge.concealed_blunderbuss&!talent.fan_the_hammer|!buff.skull_and_crossbones.up)&(!runeforge.invigorating_shadowdust|!buff.true_bearing.up))|rtb_buffs=2&buff.buried_treasure.up&buff.grand_melee.up", "Reroll BT + GM or single buffs early other than Broadside, TB with Shadowdust, or SnC with Blunderbuss" );
  default_->add_action( "variable,name=ambush_condition,value=combo_points.deficit>=2+buff.broadside.up&energy>=50&(!conduit.count_the_odds&!talent.count_the_odds|buff.roll_the_bones.remains>=10)", "Ensure we get full Ambush CP gains and aren't rerolling Count the Odds buffs away" );
  default_->add_action( "variable,name=finish_condition,value=combo_points>=cp_max_spend-buff.broadside.up-(buff.opportunity.up*(talent.quick_draw|talent.fan_the_hammer)|buff.concealed_blunderbuss.up)|effective_combo_points>=cp_max_spend", "Finish at max possible CP without overflowing bonus combo points, unless for BtE which always should be 5+ CP" );
  default_->add_action( "variable,name=finish_condition,op=reset,if=cooldown.between_the_eyes.ready&effective_combo_points<5", "Always attempt to use BtE at 5+ CP, regardless of CP gen waste" );
  default_->add_action( "variable,name=finish_condition,value=1,if=buff.flagellation_buff.up&buff.flagellation_buff.remains<1&effective_combo_points>=2", "Finish at 2+ in the last GCD of Flagellation" );
  default_->add_action( "variable,name=blade_flurry_sync,value=spell_targets.blade_flurry<2&raid_event.adds.in>20|buff.blade_flurry.remains>1+talent.killing_spree.enabled", "With multiple targets, this variable is checked to decide whether some CDs should be synced with Blade Flurry" );
  default_->add_action( "run_action_list,name=stealth,if=stealthed.all" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "run_action_list,name=finish,if=variable.finish_condition" );
  default_->add_action( "call_action_list,name=build" );
  default_->add_action( "arcane_torrent,if=energy.base_deficit>=15+energy.regen" );
  default_->add_action( "arcane_pulse" );
  default_->add_action( "lights_judgment" );
  default_->add_action( "bag_of_tricks" );

  build->add_action( "sepsis,target_if=max:target.time_to_die*debuff.between_the_eyes.up,if=target.time_to_die>11&debuff.between_the_eyes.up|fight_remains<11", "Builders" );
  build->add_action( "ghostly_strike,if=debuff.ghostly_strike.remains<=3" );
  build->add_action( "shiv,if=runeforge.tiny_toxic_blade" );
  build->add_action( "echoing_reprimand,if=!soulbind.effusive_anima_accelerator|variable.blade_flurry_sync" );
  build->add_action( "ambush" );
  build->add_action( "cold_blood,if=buff.opportunity.up&buff.greenskins_wickers.up|buff.greenskins_wickers.up&buff.greenskins_wickers.remains<1.5", "Use Pistol Shot when buffed by bonuses as a priority" );
  build->add_action( "pistol_shot,if=buff.opportunity.up&(buff.greenskins_wickers.up&!talent.fan_the_hammer|buff.concealed_blunderbuss.up)|buff.greenskins_wickers.up&buff.greenskins_wickers.remains<1.5" );
  build->add_action( "pistol_shot,if=talent.fan_the_hammer&buff.opportunity.up&(buff.opportunity.stack>=buff.opportunity.max_stack|buff.opportunity.remains<2)", "With Fan the Hammer, consume Opportunity at max stacks or if we will get 4+ CP and Dreadblades is not up" );
  build->add_action( "pistol_shot,if=talent.fan_the_hammer&buff.opportunity.up&combo_points.deficit>4&!buff.dreadblades.up" );
  build->add_action( "pool_resource,for_next=1" );
  build->add_action( "ambush" );
  build->add_action( "serrated_bone_spike,if=!dot.serrated_bone_spike_dot.ticking", "Apply SBS to all targets without a debuff as priority, preferring targets dying sooner after the primary target" );
  build->add_action( "serrated_bone_spike,target_if=min:target.time_to_die+(dot.serrated_bone_spike_dot.ticking*600),if=!dot.serrated_bone_spike_dot.ticking" );
  build->add_action( "serrated_bone_spike,if=fight_remains<=5|cooldown.serrated_bone_spike.max_charges-charges_fractional<=0.25|combo_points.deficit=cp_gain&!buff.skull_and_crossbones.up&energy.base_time_to_max>1", "Attempt to use when it will cap combo points and SnD is down, otherwise keep from capping charges" );
  build->add_action( "pistol_shot,if=!talent.fan_the_hammer&buff.opportunity.up&(energy.base_deficit>energy.regen*1.5|!talent.weaponmaster&combo_points.deficit<=1+buff.broadside.up|talent.quick_draw.enabled|talent.audacity.enabled&!buff.audacity.up)", "Use Pistol Shot with Opportunity if Combat Potency won't overcap energy, when it will exactly cap CP, or when using Quick Draw" );
  build->add_action( "sinister_strike,target_if=min:dot.vicious_wound.remains,if=buff.acquired_axe_driver.up", "Use Sinister Strike on targets without the Cache DoT if the trinket is up" );
  build->add_action( "sinister_strike" );

  cds->add_action( "blade_flurry,if=spell_targets>=2&!buff.blade_flurry.up", "Cooldowns  Blade Flurry on 2+ enemies" );
  cds->add_action( "roll_the_bones,if=master_assassin_remains=0&buff.dreadblades.down&(!buff.roll_the_bones.up|variable.rtb_reroll)" );
  cds->add_action( "keep_it_rolling,if=!variable.rtb_reroll&(buff.broadside.up+buff.true_bearing.up+buff.skull_and_crossbones.up+buff.ruthless_precision.up)>2" );
  cds->add_action( "flagellation,target_if=max:target.time_to_die,if=!stealthed.all&(variable.finish_condition&target.time_to_die>10|fight_remains<13)" );
  cds->add_action( "shadow_dance,if=!runeforge.mark_of_the_master_assassin&!runeforge.invigorating_shadowdust&!runeforge.deathly_shadows&!stealthed.all&!buff.take_em_by_surprise.up&(variable.finish_condition&buff.slice_and_dice.up|variable.ambush_condition&!buff.slice_and_dice.up)" );
  cds->add_action( "vanish,if=!runeforge.mark_of_the_master_assassin&!runeforge.invigorating_shadowdust&!runeforge.deathly_shadows&!stealthed.all&!buff.take_em_by_surprise.up&(variable.finish_condition&buff.slice_and_dice.up|variable.ambush_condition&!buff.slice_and_dice.up)" );
  cds->add_action( "vanish,if=runeforge.deathly_shadows&!stealthed.all&buff.deathly_shadows.down&combo_points<=2&variable.ambush_condition", "With Deathly Shadows, optimize for combo point generation when the buff is down" );
  cds->add_action( "variable,name=vanish_ma_condition,if=runeforge.mark_of_the_master_assassin&!talent.marked_for_death.enabled,value=(!cooldown.between_the_eyes.ready&variable.finish_condition)|(cooldown.between_the_eyes.ready&variable.ambush_condition)", "With Master Asssassin, sync Vanish with a finisher or Ambush depending on BtE cooldown, or always a finisher with MfD" );
  cds->add_action( "variable,name=vanish_ma_condition,if=runeforge.mark_of_the_master_assassin&talent.marked_for_death.enabled,value=variable.finish_condition" );
  cds->add_action( "vanish,if=variable.vanish_ma_condition&master_assassin_remains=0&variable.blade_flurry_sync" );
  cds->add_action( "adrenaline_rush,if=!buff.adrenaline_rush.up&(!talent.improved_adrenaline_rush|combo_points<=2)" );
  cds->add_action( "fleshcraft,if=(soulbind.pustule_eruption|soulbind.volatile_solvent)&!stealthed.all&(!buff.blade_flurry.up|spell_targets.blade_flurry<2)&(!buff.adrenaline_rush.up|energy.base_time_to_max>2)", "Fleshcraft for Pustule Eruption if not stealthed and not with Blade Flurry" );
  cds->add_action( "dreadblades,if=!stealthed.all&combo_points<=2&(!covenant.venthyr|buff.flagellation_buff.up)&(!talent.marked_for_death|!cooldown.marked_for_death.ready)" );
  cds->add_action( "marked_for_death,line_cd=1.5,target_if=min:target.time_to_die,if=raid_event.adds.up&(target.time_to_die<combo_points.deficit|!stealthed.rogue&combo_points.deficit>=cp_max_spend-1)", "If adds are up, snipe the one with lowest TTD. Use when dying faster than CP deficit or without any CP." );
  cds->add_action( "marked_for_death,if=raid_event.adds.in>30-raid_event.adds.duration&!stealthed.rogue&combo_points.deficit>=cp_max_spend-1&(!covenant.venthyr|cooldown.flagellation.remains>10|buff.flagellation_buff.up)", "If no adds will die within the next 30s, use MfD on boss without any CP." );
  cds->add_action( "variable,name=killing_spree_vanish_sync,value=!runeforge.mark_of_the_master_assassin|cooldown.vanish.remains>10|master_assassin_remains>2", "Attempt to sync Killing Spree with Vanish for Master Assassin" );
  cds->add_action( "killing_spree,if=variable.blade_flurry_sync&variable.killing_spree_vanish_sync&!stealthed.rogue&(debuff.between_the_eyes.up&buff.dreadblades.down&energy.base_deficit>(energy.regen*2+15)|spell_targets.blade_flurry>(2-buff.deathly_shadows.up)|master_assassin_remains>0)", "Use in 1-2T if BtE is up and won't cap Energy, or at 3T+ (2T+ with Deathly Shadows) or when Master Assassin is up." );
  cds->add_action( "blade_rush,if=variable.blade_flurry_sync&(energy.base_time_to_max>2&!buff.dreadblades.up&!buff.flagellation_buff.up|energy<=30|spell_targets>2)" );
  cds->add_action( "vanish,if=runeforge.invigorating_shadowdust&covenant.venthyr&!stealthed.all&variable.finish_condition&(!cooldown.flagellation.ready&(!talent.dreadblades|!cooldown.dreadblades.ready|!buff.flagellation_buff.up))", "If using Invigorating Shadowdust, use normal logic in addition to checking major CDs." );
  cds->add_action( "vanish,if=runeforge.invigorating_shadowdust&!covenant.venthyr&!stealthed.all&variable.finish_condition&(cooldown.echoing_reprimand.remains>6|!cooldown.sepsis.ready|cooldown.serrated_bone_spike.full_recharge_time>20)" );
  cds->add_action( "shadowmeld,if=!stealthed.all&((conduit.count_the_odds|talent.count_the_odds)&variable.finish_condition|!talent.weaponmaster.enabled&variable.ambush_condition)" );
  cds->add_action( "thistle_tea,if=energy.deficit>=100&!buff.thistle_tea.up&(charges=3|buff.adrenaline_rush.up|fight_remains<charges*6)" );
  cds->add_action( "potion,if=buff.bloodlust.react|fight_remains<30|buff.adrenaline_rush.up" );
  cds->add_action( "blood_fury" );
  cds->add_action( "berserking" );
  cds->add_action( "fireblood" );
  cds->add_action( "ancestral_call" );
  cds->add_action( "use_item,name=wraps_of_electrostatic_potential" );
  cds->add_action( "use_item,name=ring_of_collapsing_futures,if=buff.temptation.down|fight_remains<30" );
  cds->add_action( "use_item,name=windscar_whetstone,if=spell_targets.blade_flurry>desired_targets|raid_event.adds.in>60|fight_remains<7" );
  cds->add_action( "use_item,name=cache_of_acquired_treasures,if=buff.acquired_axe.up|fight_remains<25" );
  cds->add_action( "use_item,name=bloodstained_handkerchief,target_if=max:target.time_to_die*(!dot.cruel_garrote.ticking),if=!dot.cruel_garrote.ticking" );
  cds->add_action( "use_item,name=scars_of_fraternal_strife,if=!buff.scars_of_fraternal_strife_4.up|fight_remains<30" );
  cds->add_action( "use_items,slots=trinket1,if=debuff.between_the_eyes.up|trinket.1.has_stat.any_dps|fight_remains<=20", "Default conditions for usable items." );
  cds->add_action( "use_items,slots=trinket2,if=debuff.between_the_eyes.up|trinket.2.has_stat.any_dps|fight_remains<=20" );

  finish->add_action( "between_the_eyes,if=target.time_to_die>3&(debuff.between_the_eyes.remains<4|(runeforge.greenskins_wickers|talent.greenskins_wickers)&!buff.greenskins_wickers.up|!runeforge.greenskins_wickers&!talent.greenskins_wickers&buff.ruthless_precision.up)", "Finishers  BtE to keep the Crit debuff up, if RP is up, or for Greenskins, unless the target is about to die." );
  finish->add_action( "slice_and_dice,if=buff.slice_and_dice.remains<fight_remains&refreshable&(!talent.swift_slasher|combo_points>=cp_max_spend)" );
  finish->add_action( "cold_blood,if=!(runeforge.greenskins_wickers|talent.greenskins_wickers)" );
  finish->add_action( "dispatch" );

  stealth->add_action( "dispatch,if=variable.finish_condition", "Stealth" );
  stealth->add_action( "ambush" );
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
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "fleshcraft,if=soulbind.pustule_eruption|soulbind.volatile_solvent" );
  precombat->add_action( "stealth" );
  precombat->add_action( "marked_for_death,precombat_seconds=15" );
  precombat->add_action( "slice_and_dice,precombat_seconds=1" );
  precombat->add_action( "shadow_blades,if=runeforge.mark_of_the_master_assassin" );

  default_->add_action( "stealth" );
  default_->add_action( "kick" );
  default_->add_action( "variable,name=snd_condition,value=buff.slice_and_dice.up|spell_targets.shuriken_storm>=6" );
  default_->add_action( "variable,name=is_next_cp_animacharged,if=covenant.kyrian,value=combo_points=1&buff.echoing_reprimand_2.up|combo_points=2&buff.echoing_reprimand_3.up|combo_points=3&buff.echoing_reprimand_4.up|combo_points=4&buff.echoing_reprimand_5.up" );
  default_->add_action( "variable,name=effective_combo_points,value=effective_combo_points" );
  default_->add_action( "variable,name=effective_combo_points,if=covenant.kyrian&effective_combo_points>combo_points&combo_points.deficit>2&time_to_sht.4.plus<0.5&!variable.is_next_cp_animacharged,value=combo_points" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "slice_and_dice,if=spell_targets.shuriken_storm<6&fight_remains>6&buff.slice_and_dice.remains<gcd.max&combo_points>=4-(time<10)*2" );
  default_->add_action( "run_action_list,name=stealthed,if=stealthed.all" );
  default_->add_action( "variable,name=use_priority_rotation,value=priority_rotation&spell_targets.shuriken_storm>=2" );
  default_->add_action( "call_action_list,name=stealth_cds,if=variable.use_priority_rotation" );
  default_->add_action( "variable,name=stealth_threshold,value=25+talent.vigor.enabled*20+talent.master_of_shadows.enabled*20+talent.shadow_focus.enabled*25+talent.alacrity.enabled*20+25*(spell_targets.shuriken_storm>=4)" );
  default_->add_action( "call_action_list,name=stealth_cds,if=energy.deficit<=variable.stealth_threshold" );
  default_->add_action( "call_action_list,name=finish,if=variable.effective_combo_points>=cp_max_spend" );
  default_->add_action( "call_action_list,name=finish,if=combo_points.deficit<=1|fight_remains<=1&variable.effective_combo_points>=3" );
  default_->add_action( "call_action_list,name=finish,if=spell_targets.shuriken_storm>=4&variable.effective_combo_points>=4" );
  default_->add_action( "call_action_list,name=build,if=energy.deficit<=variable.stealth_threshold" );
  default_->add_action( "arcane_torrent,if=energy.deficit>=15+energy.regen" );
  default_->add_action( "arcane_pulse" );
  default_->add_action( "lights_judgment" );
  default_->add_action( "bag_of_tricks" );

  build->add_action( "shiv,if=!talent.nightstalker.enabled&runeforge.tiny_toxic_blade&spell_targets.shuriken_storm<5" );
  build->add_action( "shuriken_storm,if=spell_targets>=2&(!covenant.necrolord|cooldown.serrated_bone_spike.max_charges-charges_fractional>=0.25|spell_targets.shuriken_storm>4)&(buff.perforated_veins.stack<=4|spell_targets.shuriken_storm>4&!variable.use_priority_rotation)" );
  build->add_action( "serrated_bone_spike,if=buff.perforated_veins.stack<=2&(cooldown.serrated_bone_spike.max_charges-charges_fractional<=0.25|soulbind.lead_by_example.enabled&!buff.lead_by_example.up|soulbind.kevins_oozeling.enabled&!debuff.kevins_wrath.up)" );
  build->add_action( "gloomblade" );
  build->add_action( "backstab,if=!covenant.kyrian|!(variable.is_next_cp_animacharged&(time_to_sht.3.plus<0.5|time_to_sht.4.plus<1)&energy<60)" );

  cds->add_action( "shadow_dance,use_off_gcd=1,if=!buff.shadow_dance.up&buff.shuriken_tornado.up&buff.shuriken_tornado.remains<=3.5" );
  cds->add_action( "symbols_of_death,use_off_gcd=1,if=buff.shuriken_tornado.up&buff.shuriken_tornado.remains<=3.5" );
  cds->add_action( "flagellation,target_if=max:target.time_to_die,if=variable.snd_condition&combo_points>=5&target.time_to_die>10" );
  cds->add_action( "vanish,if=(runeforge.mark_of_the_master_assassin&combo_points.deficit<=1-talent.deeper_strategem.enabled|runeforge.deathly_shadows&combo_points<1)&buff.symbols_of_death.up&buff.shadow_dance.up&master_assassin_remains=0&buff.deathly_shadows.down" );
  cds->add_action( "pool_resource,for_next=1,if=talent.shuriken_tornado.enabled&!talent.shadow_focus.enabled" );
  cds->add_action( "shuriken_tornado,if=spell_targets.shuriken_storm<=1&energy>=60&variable.snd_condition&cooldown.symbols_of_death.up&cooldown.shadow_dance.charges>=1&(!runeforge.obedience|buff.flagellation_buff.up|spell_targets.shuriken_storm>=(1+4*(!talent.nightstalker.enabled&!talent.dark_shadow.enabled)))&combo_points<=2&!buff.premeditation.up&(!covenant.venthyr&!talent.flagellation|!cooldown.flagellation.up)" );
  cds->add_action( "serrated_bone_spike,cycle_targets=1,if=variable.snd_condition&!dot.serrated_bone_spike_dot.ticking&target.time_to_die>=21&(combo_points.deficit>=(cp_gain>?4))&!buff.shuriken_tornado.up&(!buff.premeditation.up|spell_targets.shuriken_storm>4)|fight_remains<=5&spell_targets.shuriken_storm<3" );
  cds->add_action( "sepsis,if=variable.snd_condition&combo_points.deficit>=1&target.time_to_die>=16" );
  cds->add_action( "symbols_of_death,if=variable.snd_condition&(!talent.flagellation&!covenant.venthyr|cooldown.flagellation.remains>10|cooldown.flagellation.up&combo_points>=5)" );
  cds->add_action( "marked_for_death,line_cd=1.5,target_if=min:target.time_to_die,if=raid_event.adds.up&(target.time_to_die<combo_points.deficit|!stealthed.all&combo_points.deficit>=cp_max_spend)" );
  cds->add_action( "marked_for_death,if=raid_event.adds.in>30-raid_event.adds.duration&combo_points.deficit>=cp_max_spend" );
  cds->add_action( "shadow_blades,if=variable.snd_condition&combo_points.deficit>=2&(buff.symbols_of_death.up|fight_remains<=20)" );
  cds->add_action( "echoing_reprimand,if=(!talent.shadow_focus.enabled|!stealthed.all|spell_targets.shuriken_storm>=4)&variable.snd_condition&combo_points.deficit>=2&(variable.use_priority_rotation|spell_targets.shuriken_storm<=4|runeforge.resounding_clarity|talent.resounding_clarity)" );
  cds->add_action( "shuriken_tornado,if=(talent.shadow_focus.enabled|spell_targets.shuriken_storm>=2)&variable.snd_condition&buff.symbols_of_death.up&combo_points<=2&(!buff.premeditation.up|spell_targets.shuriken_storm>4)" );
  cds->add_action( "shadow_dance,if=!buff.shadow_dance.up&fight_remains<=8+talent.subterfuge.enabled" );
  cds->add_action( "thistle_tea,if=cooldown.symbols_of_death.remains>=3&!buff.thistle_tea.up&(energy.deficit>=100|cooldown.thistle_tea.charges_fractional>=2.75&energy.deficit>=40)" );
  cds->add_action( "fleshcraft,if=(soulbind.pustule_eruption|soulbind.volatile_solvent)&energy.deficit>=30&!stealthed.all&buff.symbols_of_death.down" );
  cds->add_action( "potion,if=buff.bloodlust.react|fight_remains<30|buff.symbols_of_death.up&(buff.shadow_blades.up|cooldown.shadow_blades.remains<=10)" );
  cds->add_action( "blood_fury,if=buff.symbols_of_death.up" );
  cds->add_action( "berserking,if=buff.symbols_of_death.up" );
  cds->add_action( "fireblood,if=buff.symbols_of_death.up" );
  cds->add_action( "ancestral_call,if=buff.symbols_of_death.up" );
  cds->add_action( "use_item,name=wraps_of_electrostatic_potential" );
  cds->add_action( "use_item,name=ring_of_collapsing_futures,if=buff.temptation.down|fight_remains<30" );
  cds->add_action( "use_item,name=cache_of_acquired_treasures,if=(covenant.venthyr&buff.acquired_axe.up|!covenant.venthyr&buff.acquired_wand.up)&(spell_targets.shuriken_storm=1&raid_event.adds.in>60|fight_remains<25|variable.use_priority_rotation)|buff.acquired_axe.up&spell_targets.shuriken_storm>1" );
  cds->add_action( "use_item,name=bloodstained_handkerchief,target_if=max:target.time_to_die*(!dot.cruel_garrote.ticking),if=!dot.cruel_garrote.ticking" );
  cds->add_action( "use_item,name=scars_of_fraternal_strife,if=!buff.scars_of_fraternal_strife_4.up|fight_remains<30" );
  cds->add_action( "use_items,if=buff.symbols_of_death.up|fight_remains<20" );

  finish->add_action( "variable,name=premed_snd_condition,value=talent.premeditation.enabled&spell_targets.shuriken_storm<(5-covenant.necrolord)&!covenant.kyrian" );
  finish->add_action( "slice_and_dice,if=!variable.premed_snd_condition&spell_targets.shuriken_storm<6&!buff.shadow_dance.up&buff.slice_and_dice.remains<fight_remains&refreshable" );
  finish->add_action( "slice_and_dice,if=variable.premed_snd_condition&cooldown.shadow_dance.charges_fractional<1.75&buff.slice_and_dice.remains<cooldown.symbols_of_death.remains&(cooldown.shadow_dance.ready&buff.symbols_of_death.remains-buff.shadow_dance.remains<1.2)" );
  finish->add_action( "variable,name=skip_rupture,value=master_assassin_remains>0" );
  finish->add_action( "rupture,if=(!variable.skip_rupture|variable.use_priority_rotation)&target.time_to_die-remains>6&refreshable" );
  finish->add_action( "secret_technique,if=cooldown.shadow_dance.remains>40|buff.shadow_dance.up" );
  finish->add_action( "rupture,cycle_targets=1,if=!variable.skip_rupture&!variable.use_priority_rotation&spell_targets.shuriken_storm>=2&target.time_to_die>=(2*combo_points)&refreshable" );
  finish->add_action( "rupture,if=!variable.skip_rupture&remains<cooldown.symbols_of_death.remains+10&cooldown.symbols_of_death.remains<=5&target.time_to_die-remains>cooldown.symbols_of_death.remains+5" );
  finish->add_action( "black_powder,if=!variable.use_priority_rotation&spell_targets>=3" );
  finish->add_action( "eviscerate" );

  stealth_cds->add_action( "variable,name=shd_threshold,value=cooldown.shadow_dance.charges_fractional>=0.75+talent.shadow_dance" );
  stealth_cds->add_action( "variable,name=shd_threshold,if=runeforge.the_rotten|talent.the_rotten,value=cooldown.shadow_dance.charges_fractional>=1.75|cooldown.symbols_of_death.remains>=16" );
  stealth_cds->add_action( "vanish,if=(!variable.shd_threshold|!talent.nightstalker.enabled&talent.dark_shadow.enabled)&combo_points.deficit>1&!runeforge.mark_of_the_master_assassin" );
  stealth_cds->add_action( "pool_resource,for_next=1,extra_amount=40,if=race.night_elf" );
  stealth_cds->add_action( "shadowmeld,if=energy>=40&energy.deficit>=10&!variable.shd_threshold&combo_points.deficit>1" );
  stealth_cds->add_action( "variable,name=shd_combo_points,value=combo_points.deficit>=2+buff.shadow_blades.up" );
  stealth_cds->add_action( "variable,name=shd_combo_points,value=combo_points.deficit>=3,if=covenant.kyrian" );
  stealth_cds->add_action( "variable,name=shd_combo_points,value=combo_points.deficit<=1,if=variable.use_priority_rotation&spell_targets.shuriken_storm>=4" );
  stealth_cds->add_action( "variable,name=shd_combo_points,value=combo_points.deficit<=1,if=spell_targets.shuriken_storm=4" );
  stealth_cds->add_action( "shadow_dance,if=(variable.shd_combo_points&(buff.symbols_of_death.remains>=(1.2+talent.flagellation.enabled)|variable.shd_threshold)|buff.flagellation.up|buff.flagellation_persist.remains>=6|spell_targets.shuriken_storm>=4&cooldown.symbols_of_death.remains>10)&(buff.perforated_veins.stack<4|spell_targets.shuriken_storm>3)" );
  stealth_cds->add_action( "shadow_dance,if=variable.shd_combo_points&fight_remains<cooldown.symbols_of_death.remains|!talent.shadow_dance&debuff.find_weakness.remains>=6" );

  stealthed->add_action( "shadowstrike,if=(buff.stealth.up|buff.vanish.up)&(spell_targets.shuriken_storm<4|variable.use_priority_rotation)&master_assassin_remains=0" );
  stealthed->add_action( "gloomblade,if=combo_points=4&spell_targets.shuriken_storm<=4" );
  stealthed->add_action( "call_action_list,name=finish,if=variable.effective_combo_points>=cp_max_spend" );
  stealthed->add_action( "call_action_list,name=finish,if=buff.shuriken_tornado.up&combo_points.deficit<=2" );
  stealthed->add_action( "call_action_list,name=finish,if=spell_targets.shuriken_storm>=4&variable.effective_combo_points>=4" );
  stealthed->add_action( "call_action_list,name=finish,if=combo_points.deficit<=(talent.deeper_stratagem.enabled+talent.secret_stratagem.enabled+talent.seal_fate.enabled)" );
  stealthed->add_action( "shadowstrike,if=stealthed.sepsis&spell_targets.shuriken_storm<4" );
  stealthed->add_action( "backstab,if=conduit.perforated_veins.rank>=8&buff.perforated_veins.stack>=5&buff.shadow_dance.remains>=3&buff.shadow_blades.up&(spell_targets.shuriken_storm<=3|variable.use_priority_rotation)&(buff.shadow_blades.remains<=buff.shadow_dance.remains+2|!covenant.venthyr&!talent.flagellation)" );
  stealthed->add_action( "shiv,if=talent.nightstalker.enabled&runeforge.tiny_toxic_blade&spell_targets.shuriken_storm<5" );
  stealthed->add_action( "shadowstrike,cycle_targets=1,if=!variable.use_priority_rotation&debuff.find_weakness.remains<1&spell_targets.shuriken_storm<=3&target.time_to_die-remains>6" );
  stealthed->add_action( "shadowstrike,if=variable.use_priority_rotation&(debuff.find_weakness.remains<1|talent.weaponmaster.enabled&spell_targets.shuriken_storm<=4)" );
  stealthed->add_action( "shuriken_storm,if=spell_targets>=3+(buff.the_rotten.up|runeforge.akaaris_soul_fragment)&(!buff.premeditation.up|spell_targets>=5)" );
  stealthed->add_action( "shadowstrike,if=debuff.find_weakness.remains<=1|cooldown.symbols_of_death.remains<18&debuff.find_weakness.remains<cooldown.symbols_of_death.remains" );
  stealthed->add_action( "gloomblade,if=buff.perforated_veins.stack>=5&conduit.perforated_veins.rank>=13" );
  stealthed->add_action( "shadowstrike" );
  stealthed->add_action( "cheap_shot,if=!target.is_boss&combo_points.deficit>=1&buff.shot_in_the_dark.up&energy.time_to_40>gcd.max" );
}
//subtlety_apl_end
 
//assassination_df_apl_start
void assassination_df( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* direct = p->get_action_priority_list( "direct" );
  action_priority_list_t* dot = p->get_action_priority_list( "dot" );
  action_priority_list_t* stealthed = p->get_action_priority_list( "stealthed" );
  action_priority_list_t* vanish = p->get_action_priority_list( "vanish" );

  precombat->add_action( "apply_poison" );
  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "marked_for_death,precombat_seconds=10,if=raid_event.adds.in>15" );
  precombat->add_action( "variable,name=trinket_sync_slot,value=1,if=trinket.1.has_stat.any_dps&(!trinket.2.has_stat.any_dps|trinket.1.cooldown.duration>=trinket.2.cooldown.duration)", "Determine which (if any) stat buff trinket we want to attempt to sync with Deathmark." );
  precombat->add_action( "variable,name=trinket_sync_slot,value=2,if=trinket.2.has_stat.any_dps&(!trinket.1.has_stat.any_dps|trinket.2.cooldown.duration>trinket.1.cooldown.duration)" );
  precombat->add_action( "variable,name=exsanguinate_rupture_cp,value=cp_max_spend<?(talent.resounding_clarity*7)", "Determine if we should be be casting our pre-Exsanguinate Rupture with Echoing Reprimand CP" );
  precombat->add_action( "stealth" );
  precombat->add_action( "slice_and_dice,precombat_seconds=1" );

  default_->add_action( "stealth", "Restealth if possible (no vulnerable enemies in combat)" );
  default_->add_action( "kick", "Interrupt on cooldown to allow simming interactions with that" );
  default_->add_action( "variable,name=single_target,value=spell_targets.fan_of_knives<2" );
  default_->add_action( "variable,name=regen_saturated,value=energy.regen_combined>35", "Combined Energy Regen needed to saturate" );
  default_->add_action( "variable,name=exsang_sync_remains,op=setif,condition=cooldown.deathmark.remains>cooldown.exsanguinate.remains&cooldown.deathmark.remains<fight_remains,value=cooldown.deathmark.remains,value_else=cooldown.exsanguinate.remains", "Next Exsanguinate cooldown time based on Deathmark syncing logic and remaining fight duration" );
  default_->add_action( "variable,name=sepsis_sync_remains,op=setif,condition=cooldown.deathmark.remains>cooldown.sepsis.remains&cooldown.deathmark.remains<fight_remains,value=cooldown.deathmark.remains,value_else=cooldown.sepsis.remains", "Next Sepsis cooldown time based on Deathmark syncing logic and remaining fight duration" );
  default_->add_action( "call_action_list,name=stealthed,if=stealthed.rogue|stealthed.improved_garrote" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "slice_and_dice,if=!buff.slice_and_dice.up&combo_points>=2|!talent.cut_to_the_chase&refreshable&combo_points>=4", "Put SnD up initially for Cut to the Chase, refresh with Envenom if at low duration" );
  default_->add_action( "envenom,if=talent.cut_to_the_chase&buff.slice_and_dice.up&buff.slice_and_dice.remains<5&combo_points>=4" );
  default_->add_action( "call_action_list,name=dot" );
  default_->add_action( "call_action_list,name=direct" );
  default_->add_action( "arcane_torrent,if=energy.deficit>=15+energy.regen_combined" );
  default_->add_action( "arcane_pulse" );
  default_->add_action( "lights_judgment" );
  default_->add_action( "bag_of_tricks" );

  cds->add_action( "marked_for_death,line_cd=1.5,target_if=min:target.time_to_die,if=raid_event.adds.up&(!variable.single_target|target.time_to_die<30)&(target.time_to_die<combo_points.deficit*1.5|combo_points.deficit>=cp_max_spend)", "Cooldowns If adds are up, snipe the one with lowest TTD. Use when dying faster than CP deficit or without any CP." );
  cds->add_action( "marked_for_death,if=raid_event.adds.in>30-raid_event.adds.duration&combo_points.deficit>=cp_max_spend", "If no adds will die within the next 30s, use MfD for max CP." );
  cds->add_action( "variable,name=deathmark_exsanguinate_condition,value=!talent.exsanguinate|cooldown.exsanguinate.remains>15|exsanguinated.rupture|exsanguinated.garrote", "Sync Deathmark window with Exsanguinate if applicable" );
  cds->add_action( "variable,name=deathmark_ma_condition,value=!talent.master_assassin.enabled|dot.garrote.ticking", "Wait on Deathmark for Garrote with MA" );
  cds->add_action( "sepsis,if=!stealthed.rogue&!stealthed.improved_garrote&dot.rupture.ticking&(!talent.exsanguinate|variable.exsang_sync_remains>7|dot.rupture.remains>20)&(!talent.improved_garrote&dot.garrote.ticking|talent.improved_garrote&cooldown.garrote.up)&(target.time_to_die>10|fight_remains<10)", "Sepsis unless we're preparing for exsanguinate" );
  cds->add_action( "variable,name=deathmark_condition,value=!stealthed.rogue&dot.rupture.ticking&!debuff.deathmark.up&variable.deathmark_exsanguinate_condition&variable.deathmark_ma_condition", "Deathmark to be used if not stealthed, Rupture is up, and all other talent conditions are satisfied" );
  cds->add_action( "use_item,name=algethar_puzzle_box,use_off_gcd=1,if=(!talent.exsanguinate|cooldown.exsanguinate.remains>15|exsanguinated.rupture|exsanguinated.garrote)&dot.rupture.ticking&cooldown.deathmark.remains<2|fight_remains<=22", "Sync the priority stat buff trinket with Deathmark, otherwise use on cooldown" );
  cds->add_action( "use_items,slots=trinket1,if=(variable.trinket_sync_slot=1&(debuff.deathmark.up|fight_remains<=20)|(variable.trinket_sync_slot=2&(!trinket.2.cooldown.ready|!debuff.deathmark.up&cooldown.deathmark.remains>20))|!variable.trinket_sync_slot)" );
  cds->add_action( "use_items,slots=trinket2,if=(variable.trinket_sync_slot=2&(debuff.deathmark.up|fight_remains<=20)|(variable.trinket_sync_slot=1&(!trinket.1.cooldown.ready|!debuff.deathmark.up&cooldown.deathmark.remains>20))|!variable.trinket_sync_slot)" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=variable.deathmark_condition" );
  cds->add_action( "deathmark,if=variable.deathmark_condition" );
  cds->add_action( "kingsbane,if=(debuff.shiv.up|cooldown.shiv.remains<6)&buff.envenom.up&(cooldown.deathmark.remains>=50|dot.deathmark.ticking)" );
  cds->add_action( "variable,name=exsanguinate_condition,value=talent.exsanguinate&!stealthed.rogue&(!stealthed.improved_garrote|dot.garrote.pmultiplier>1)&!dot.deathmark.ticking&target.time_to_die>variable.exsang_sync_remains+4&variable.exsang_sync_remains<4", "Exsanguinate when not stealthed and both Rupture and Garrote are up for long enough. Attempt to sync with Deathmark and also Echoing Reprimand if using Resounding Clarity." );
  cds->add_action( "echoing_reprimand,if=talent.exsanguinate&talent.resounding_clarity&(variable.exsanguinate_condition&combo_points<=2&variable.exsang_sync_remains<=2&!dot.garrote.refreshable&dot.rupture.remains>9.6)" );
  cds->add_action( "exsanguinate,if=variable.exsanguinate_condition&(!dot.garrote.refreshable&dot.rupture.remains>4+4*variable.exsanguinate_rupture_cp|dot.rupture.remains*0.5>target.time_to_die)" );
  cds->add_action( "shiv,if=talent.kingsbane&!debuff.shiv.up&dot.kingsbane.ticking&dot.garrote.ticking&dot.rupture.ticking&(!talent.crimson_tempest.enabled|variable.single_target|dot.crimson_tempest.ticking)", "Shiv if DoTs are up; Always Shiv with Kingsbane, otherwise attempt to sync with Sepsis or Deathmark if we won't waste more than half Shiv's cooldown" );
  cds->add_action( "shiv,if=talent.arterial_precision&!debuff.shiv.up&dot.garrote.ticking&dot.rupture.ticking&(debuff.deathmark.up|cooldown.shiv.charges_fractional>max_charges-0.5&cooldown.deathmark.remains>10)" );
  cds->add_action( "shiv,if=!talent.kingsbane&!talent.arterial_precision&!talent.sepsis&!debuff.shiv.up&dot.garrote.ticking&dot.rupture.ticking&(!talent.crimson_tempest.enabled|variable.single_target|dot.crimson_tempest.ticking)&(!talent.exsanguinate|variable.exsang_sync_remains>2)" );
  cds->add_action( "shiv,if=talent.sepsis&!talent.kingsbane&!talent.arterial_precision&!debuff.shiv.up&dot.garrote.ticking&dot.rupture.ticking&((cooldown.shiv.charges_fractional>0.9+talent.lightweight_shiv.enabled&variable.sepsis_sync_remains>5)|dot.sepsis.ticking|dot.deathmark.ticking|fight_remains<20)", "Pool Shiv charges for Sepsis/Deathmark, unless we're about to cap on charges" );
  cds->add_action( "thistle_tea,if=!buff.thistle_tea.up&(energy.deficit>=100|charges=3&(dot.kingsbane.ticking|debuff.deathmark.up)|fight_remains<charges*6)" );
  cds->add_action( "indiscriminate_carnage,if=(spell_targets.fan_of_knives>desired_targets|spell_targets.fan_of_knives>1&raid_event.adds.in>60)&(!talent.improved_garrote|cooldown.vanish.remains>45)" );
  cds->add_action( "potion,if=buff.bloodlust.react|fight_remains<30|debuff.deathmark.up" );
  cds->add_action( "blood_fury,if=debuff.deathmark.up" );
  cds->add_action( "berserking,if=debuff.deathmark.up" );
  cds->add_action( "fireblood,if=debuff.deathmark.up" );
  cds->add_action( "ancestral_call,if=debuff.deathmark.up" );
  cds->add_action( "call_action_list,name=vanish,if=!stealthed.all&master_assassin_remains=0" );
  cds->add_action( "cold_blood,if=combo_points>=4" );

  direct->add_action( "envenom,if=effective_combo_points>=4+talent.deeper_stratagem.enabled&(debuff.deathmark.up|debuff.shiv.up|debuff.amplifying_poison.stack>=10|energy.deficit<=25+energy.regen_combined|!variable.single_target|effective_combo_points>cp_max_spend)&(!talent.exsanguinate.enabled|variable.exsang_sync_remains>2|talent.resounding_clarity&(cooldown.echoing_reprimand.ready&combo_points>2|effective_combo_points>5))", "Direct damage abilities Envenom at 4+ (5+ with DS) CP. Immediately on 2+ targets, with Deathmark, or with TB; otherwise wait for some energy. Also wait if Exsg combo is coming up." );
  direct->add_action( "variable,name=use_filler,value=combo_points.deficit>1|energy.deficit<=25+energy.regen_combined|!variable.single_target" );
  direct->add_action( "serrated_bone_spike,if=variable.use_filler&!dot.serrated_bone_spike_dot.ticking", "Apply SBS to all targets without a debuff as priority, preferring targets dying sooner after the primary target" );
  direct->add_action( "serrated_bone_spike,target_if=min:target.time_to_die+(dot.serrated_bone_spike_dot.ticking*600),if=variable.use_filler&!dot.serrated_bone_spike_dot.ticking" );
  direct->add_action( "serrated_bone_spike,if=variable.use_filler&master_assassin_remains<0.8&(fight_remains<=5|cooldown.serrated_bone_spike.max_charges-charges_fractional<=0.25)", "Keep from capping charges or burn at the end of fights" );
  direct->add_action( "serrated_bone_spike,if=variable.use_filler&master_assassin_remains<0.8&!variable.single_target&debuff.shiv.up", "When MA is not at high duration, sync with Shiv" );
  direct->add_action( "echoing_reprimand,if=(!talent.exsanguinate|!talent.resounding_clarity|variable.exsang_sync_remains>40)&variable.use_filler|fight_remains<20" );
  direct->add_action( "fan_of_knives,if=variable.use_filler&(!priority_rotation&spell_targets.fan_of_knives>=3+stealthed.rogue+talent.dragontempered_blades)", "Fan of Knives at 3+ targets or 4+ with DTB" );
  direct->add_action( "fan_of_knives,target_if=!dot.deadly_poison_dot.ticking&(!priority_rotation|dot.garrote.ticking|dot.rupture.ticking),if=variable.use_filler&spell_targets.fan_of_knives>=3", "Fan of Knives to apply poisons if inactive on any target (or any bleeding targets with priority rotation) at 3T" );
  direct->add_action( "ambush,if=variable.use_filler&(buff.blindside.up|buff.sepsis_buff.remains<=1|stealthed.rogue)", "Ambush on Blindside/Shadow Dance, or a last resort usage of Sepsis" );
  direct->add_action( "mutilate,target_if=!dot.deadly_poison_dot.ticking&!debuff.amplifying_poison.up,if=variable.use_filler&spell_targets.fan_of_knives=2", "Tab-Mutilate to apply Deadly Poison at 2 targets" );
  direct->add_action( "mutilate,if=variable.use_filler" );

  dot->add_action( "variable,name=skip_cycle_garrote,value=priority_rotation&(dot.garrote.remains<cooldown.garrote.duration|variable.regen_saturated)", "Damage over time abilities Limit secondary Garrotes for priority rotation if we have 35 energy regen or Garrote will expire on the primary target" );
  dot->add_action( "variable,name=skip_cycle_rupture,value=priority_rotation&(debuff.shiv.up&spell_targets.fan_of_knives>2|variable.regen_saturated)", "Limit secondary Ruptures for priority rotation if we have 35 energy regen or Shiv is up on 2T+" );
  dot->add_action( "variable,name=skip_rupture,value=0", "Limit Ruptures when appropriate, not currently used" );
  dot->add_action( "rupture,if=talent.exsanguinate.enabled&!will_lose_exsanguinate&dot.rupture.pmultiplier<=1&!dot.rupture.ticking&effective_combo_points<=3&variable.single_target", "Special Garrote and Rupture setup prior to Exsanguinate cast" );
  dot->add_action( "garrote,if=talent.exsanguinate.enabled&!will_lose_exsanguinate&dot.garrote.pmultiplier<=1&variable.exsang_sync_remains<2&spell_targets.fan_of_knives=1&raid_event.adds.in>6&dot.garrote.remains*0.5<target.time_to_die" );
  dot->add_action( "rupture,if=talent.exsanguinate.enabled&!will_lose_exsanguinate&dot.rupture.pmultiplier<=1&variable.exsang_sync_remains<1&effective_combo_points>=variable.exsanguinate_rupture_cp&dot.rupture.remains*0.5<target.time_to_die" );
  dot->add_action( "pool_resource,for_next=1", "Garrote upkeep, also tries to use it as a special generator for the last CP before a finisher" );
  dot->add_action( "garrote,if=refreshable&combo_points.deficit>=1&(pmultiplier<=1|remains<=tick_time&spell_targets.fan_of_knives>=3)&(!will_lose_exsanguinate|remains<=tick_time*2&spell_targets.fan_of_knives>=3)&(target.time_to_die-remains)>4&master_assassin_remains=0" );
  dot->add_action( "pool_resource,for_next=1" );
  dot->add_action( "garrote,cycle_targets=1,if=!variable.skip_cycle_garrote&target!=self.target&refreshable&combo_points.deficit>=1&(pmultiplier<=1|remains<=tick_time&spell_targets.fan_of_knives>=3)&(!will_lose_exsanguinate|remains<=tick_time*2&spell_targets.fan_of_knives>=3)&(target.time_to_die-remains)>12&master_assassin_remains=0" );
  dot->add_action( "crimson_tempest,target_if=min:remains,if=spell_targets>=2&effective_combo_points>=4&energy.regen_combined>20&(!cooldown.deathmark.ready|dot.rupture.ticking)&remains<(2+3*(spell_targets>=4))", "Crimson Tempest on multiple targets at 4+ CP when running out in 2-5s as long as we have enough regen and aren't setting up for Deathmark" );
  dot->add_action( "rupture,if=!variable.skip_rupture&effective_combo_points>=4&refreshable&(pmultiplier<=1|remains<=tick_time&spell_targets.fan_of_knives>=3)&(!will_lose_exsanguinate|remains<=tick_time*2&spell_targets.fan_of_knives>=3)&target.time_to_die-remains>(4+(talent.dashing_scoundrel*5)+(talent.doomblade*5)+(variable.regen_saturated*6))", "Keep up Rupture at 4+ on all targets (when living long enough and not snapshot)" );
  dot->add_action( "rupture,cycle_targets=1,if=!variable.skip_cycle_rupture&!variable.skip_rupture&target!=self.target&effective_combo_points>=4&refreshable&(pmultiplier<=1|remains<=tick_time&spell_targets.fan_of_knives>=3)&(!will_lose_exsanguinate|remains<=tick_time*2&spell_targets.fan_of_knives>=3)&target.time_to_die-remains>(4+(talent.dashing_scoundrel*5)+(talent.doomblade*5)+(variable.regen_saturated*6))" );
  dot->add_action( "crimson_tempest,if=spell_targets>=2&effective_combo_points>=4&remains<2+3*(spell_targets>=4)", "Fallback AoE Crimson Tempest with the same logic as above, but ignoring the energy conditions if we aren't using Rupture" );
  dot->add_action( "crimson_tempest,if=spell_targets=1&!talent.dashing_scoundrel&effective_combo_points>=(cp_max_spend-1)&refreshable&!will_lose_exsanguinate&!debuff.shiv.up&debuff.amplifying_poison.stack<15&(!talent.kingsbane|buff.envenom.up|!cooldown.kingsbane.up)&target.time_to_die-remains>4", "Crimson Tempest on ST if in pandemic and nearly max energy and if Envenom won't do more damage due to TB/MA" );

  stealthed->add_action( "indiscriminate_carnage,if=spell_targets.fan_of_knives>desired_targets|spell_targets.fan_of_knives>1&raid_event.adds.in>60", "Stealthed Actions" );
  stealthed->add_action( "pool_resource,for_next=1", "Improved Garrote: Apply or Refresh with buffed Garrotes, accounting for Sepsis buff time" );
  stealthed->add_action( "garrote,target_if=min:remains,if=stealthed.improved_garrote&!will_lose_exsanguinate&(remains<(12-buff.sepsis_buff.remains)%exsanguinated_rate|pmultiplier<=1)&target.time_to_die-remains>2" );
  stealthed->add_action( "pool_resource,for_next=1", "Improved Garrote + Exsg on 1T: Refresh Garrote at the end of stealth to get max duration before Exsanguinate" );
  stealthed->add_action( "garrote,if=talent.exsanguinate.enabled&stealthed.improved_garrote&active_enemies=1&!will_lose_exsanguinate&(remains<18%exsanguinated_rate|pmultiplier<=1)&variable.exsang_sync_remains<18&improved_garrote_remains<1.3" );

  vanish->add_action( "pool_resource,for_next=1,extra_amount=45", "Stealth Cooldowns Vanish Sync for Improved Garrote with Deathmark" );
  vanish->add_action( "vanish,if=talent.improved_garrote&cooldown.garrote.up&!exsanguinated.garrote&(dot.garrote.pmultiplier<=1|dot.garrote.refreshable)&(debuff.deathmark.up|cooldown.deathmark.remains<4)&combo_points.deficit>=(spell_targets.fan_of_knives>?4)" );
  vanish->add_action( "pool_resource,for_next=1,extra_amount=45", "Vanish for Indiscriminate Carnage or Improved Garrote at 2-3+ targets" );
  vanish->add_action( "vanish,if=talent.improved_garrote&cooldown.garrote.up&!exsanguinated.garrote&(dot.garrote.pmultiplier<=1|dot.garrote.refreshable)&spell_targets.fan_of_knives>(3-talent.indiscriminate_carnage)&(!talent.indiscriminate_carnage|cooldown.indiscriminate_carnage.ready)" );
  vanish->add_action( "vanish,if=!talent.improved_garrote&talent.master_assassin&!dot.rupture.refreshable&dot.garrote.remains>3&debuff.deathmark.up&(debuff.shiv.up|debuff.deathmark.remains<4|dot.sepsis.ticking)&dot.sepsis.remains<3", "Vanish with Master Assassin: Rupture+Garrote not in refresh range, during Deathmark+Shiv. Sync with Sepsis final hit if possible." );
  vanish->add_action( "pool_resource,for_next=1,extra_amount=45" );
  vanish->add_action( "shadow_dance,if=talent.improved_garrote&cooldown.garrote.up&!exsanguinated.garrote&(dot.garrote.pmultiplier<=1|dot.garrote.refreshable)&(debuff.deathmark.up|cooldown.deathmark.remains<12|cooldown.deathmark.remains>60)&combo_points.deficit>=(spell_targets.fan_of_knives>?4)", "Shadow Dance for Improved Garrote with Deathmark" );
  vanish->add_action( "shadow_dance,if=!talent.improved_garrote&talent.master_assassin&!dot.rupture.refreshable&dot.garrote.remains>3&(debuff.deathmark.up|cooldown.deathmark.remains>60)&(debuff.shiv.up|debuff.deathmark.remains<4|dot.sepsis.ticking)&dot.sepsis.remains<3", "Shadow Dance with Master Assassin: Rupture+Garrote not in refresh range, during Deathmark+Shiv. Sync with Sepsis final hit if possible." );
}
//assassination_df_apl_end

//outlaw_df_apl_start
void outlaw_df( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* build = p->get_action_priority_list( "build" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* stealth_cds = p->get_action_priority_list( "stealth_cds" );
  action_priority_list_t* finish = p->get_action_priority_list( "finish" );
  action_priority_list_t* stealth = p->get_action_priority_list( "stealth" );

  precombat->add_action( "apply_poison" );
  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "marked_for_death,precombat_seconds=10,if=raid_event.adds.in>25" );
  precombat->add_action( "adrenaline_rush,precombat_seconds=3,if=talent.improved_adrenaline_rush" );
  precombat->add_action( "roll_the_bones,precombat_seconds=2" );
  precombat->add_action( "slice_and_dice,precombat_seconds=1" );
  precombat->add_action( "stealth" );

  default_->add_action( "stealth", "Restealth if possible (no vulnerable enemies in combat)" );
  default_->add_action( "kick", "Interrupt on cooldown to allow simming interactions with that" );
  default_->add_action( "variable,name=stealthed_cto,value=talent.count_the_odds&(stealthed.basic|buff.shadowmeld.up|buff.shadow_dance.up)", "Checks if we are in an appropriate Stealth state for triggering the Count the Odds bonus" );
  default_->add_action( "variable,name=rtb_reroll,if=!talent.hidden_opportunity,value=rtb_buffs<2&(!buff.broadside.up&(!talent.fan_the_hammer|!buff.skull_and_crossbones.up)&!buff.true_bearing.up|buff.loaded_dice.up)|rtb_buffs=2&(buff.buried_treasure.up&buff.grand_melee.up|!buff.broadside.up&!buff.true_bearing.up&buff.loaded_dice.up)", "Roll the Bones Reroll Conditions" );
  default_->add_action( "variable,name=rtb_reroll,if=!talent.hidden_opportunity&(talent.keep_it_rolling|talent.count_the_odds),value=variable.rtb_reroll|((rtb_buffs.normal=0&rtb_buffs.longer>=1)&!(buff.broadside.up&buff.true_bearing.up&buff.skull_and_crossbones.up)&!(buff.broadside.remains>39|buff.true_bearing.remains>39|buff.ruthless_precision.remains>39|buff.skull_and_crossbones.remains>39))", "Additional Reroll Conditions for Keep it Rolling or Count the Odds" );
  default_->add_action( "variable,name=rtb_reroll,if=talent.hidden_opportunity,value=!rtb_buffs.will_lose.skull_and_crossbones&(rtb_buffs.will_lose-rtb_buffs.will_lose.grand_melee)<2&buff.shadow_dance.down&buff.subterfuge.down", "With Hidden Opportunity, prioritize rerolling for Skull and Crossbones over everything else" );
  default_->add_action( "variable,name=rtb_reroll,op=reset,if=!(raid_event.adds.remains>12|raid_event.adds.up&(raid_event.adds.in-raid_event.adds.remains)<6|target.time_to_die>12)|fight_remains<12", "Avoid rerolls when we will not have time remaining on the fight or add wave to recoup the opportunity cost of the global" );
  default_->add_action( "variable,name=ambush_condition,value=(talent.hidden_opportunity|combo_points.deficit>=2+talent.improved_ambush+buff.broadside.up|buff.vicious_followup.up)&energy>=50", "Ensure we want to cast Ambush prior to triggering a Stealth cooldown" );
  default_->add_action( "variable,name=finish_condition,value=combo_points>=((cp_max_spend-1)<?(6-talent.summarily_dispatched))|effective_combo_points>=cp_max_spend", "Finish at 6 (5 with Summarily Dispatched talented) CP or CP Max-1, whichever is greater of the two" );
  default_->add_action( "variable,name=blade_flurry_sync,value=spell_targets.blade_flurry<2&raid_event.adds.in>20|buff.blade_flurry.remains>1+talent.killing_spree.enabled", "With multiple targets, this variable is checked to decide whether some CDs should be synced with Blade Flurry" );
  default_->add_action( "call_action_list,name=stealth,if=stealthed.basic|buff.shadowmeld.up", "Higher priority Stealth list for Count the Odds or true Stealth/Vanish that will break in a single global" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=stealth,if=variable.stealthed_cto", "Lower priority Stealth list for Shadow Dance" );
  default_->add_action( "run_action_list,name=finish,if=variable.finish_condition" );
  default_->add_action( "call_action_list,name=build" );
  default_->add_action( "arcane_torrent,if=energy.base_deficit>=15+energy.regen" );
  default_->add_action( "arcane_pulse" );
  default_->add_action( "lights_judgment" );
  default_->add_action( "bag_of_tricks" );

  build->add_action( "sepsis,target_if=max:target.time_to_die*debuff.between_the_eyes.up,if=target.time_to_die>11&debuff.between_the_eyes.up|fight_remains<11", "Builders" );
  build->add_action( "ghostly_strike,if=debuff.ghostly_strike.remains<=3&(spell_targets.blade_flurry<=2|buff.dreadblades.up)&!buff.subterfuge.up&target.time_to_die>=5" );
  build->add_action( "ambush,if=(talent.hidden_opportunity|talent.keep_it_rolling)&(buff.audacity.up|buff.sepsis_buff.up|buff.subterfuge.up&cooldown.keep_it_rolling.ready)|talent.find_weakness&debuff.find_weakness.down", "High priority Ambush line to apply Find Weakness or consume Audacity/Sepsis buff before Pistol Shot" );
  build->add_action( "pistol_shot,if=talent.fan_the_hammer&talent.audacity&talent.hidden_opportunity&buff.opportunity.up&!buff.audacity.up&!buff.subterfuge.up&!buff.shadow_dance.up", "With Audacity + Hidden Opportunity + Fan the Hammer, use Pistol Shot to proc Audacity any time Ambush is not available" );
  build->add_action( "pistol_shot,if=buff.greenskins_wickers.up&(!talent.fan_the_hammer&buff.opportunity.up|buff.greenskins_wickers.remains<1.5)", "Use Greenskins Wickers buff immediately with Opportunity unless running Fan the Hammer" );
  build->add_action( "pistol_shot,if=talent.fan_the_hammer&buff.opportunity.up&(buff.opportunity.stack>=buff.opportunity.max_stack|buff.opportunity.remains<2)", "With Fan the Hammer, consume Opportunity at max stacks or if we will get max 4+ CP and Dreadblades is not up" );
  build->add_action( "pistol_shot,if=talent.fan_the_hammer&buff.opportunity.up&combo_points.deficit>((1+talent.quick_draw)*talent.fan_the_hammer.rank)&!buff.dreadblades.up&(!talent.hidden_opportunity|!buff.subterfuge.up&!buff.shadow_dance.up)" );
  build->add_action( "echoing_reprimand,if=!buff.dreadblades.up" );
  build->add_action( "pool_resource,for_next=1" );
  build->add_action( "ambush,if=talent.hidden_opportunity|talent.find_weakness&debuff.find_weakness.down" );
  build->add_action( "pistol_shot,if=!talent.fan_the_hammer&buff.opportunity.up&(energy.base_deficit>energy.regen*1.5|!talent.weaponmaster&combo_points.deficit<=1+buff.broadside.up|talent.quick_draw.enabled|talent.audacity.enabled&!buff.audacity.up)", "Use Pistol Shot with Opportunity if Combat Potency won't overcap energy, when it will exactly cap CP, or when using Quick Draw" );
  build->add_action( "sinister_strike" );

  cds->add_action( "adrenaline_rush,if=!buff.adrenaline_rush.up&(!talent.improved_adrenaline_rush|combo_points<=2)", "Cooldowns" );
  cds->add_action( "blade_flurry,if=spell_targets>=(2-(ptr*buff.grand_melee.up))&buff.blade_flurry.remains<gcd" );
  cds->add_action( "roll_the_bones,if=buff.dreadblades.down&(rtb_buffs.total=0|variable.rtb_reroll)" );
  cds->add_action( "keep_it_rolling,if=!variable.rtb_reroll&(buff.broadside.up+buff.true_bearing.up+buff.skull_and_crossbones.up+buff.ruthless_precision.up)>2&(buff.shadow_dance.down|rtb_buffs>=6)" );
  cds->add_action( "blade_rush,if=variable.blade_flurry_sync&!buff.dreadblades.up&(energy.base_time_to_max>4+stealthed.rogue-spell_targets%3)" );
  cds->add_action( "call_action_list,name=stealth_cds,if=!stealthed.all|talent.count_the_odds&!talent.hidden_opportunity&!variable.stealthed_cto" );
  cds->add_action( "dreadblades,if=!(variable.stealthed_cto|stealthed.basic|talent.hidden_opportunity&stealthed.rogue)&combo_points<=2&(!talent.marked_for_death|!cooldown.marked_for_death.ready)&target.time_to_die>=10" );
  cds->add_action( "marked_for_death,line_cd=1.5,target_if=min:target.time_to_die,if=raid_event.adds.up&(target.time_to_die<combo_points.deficit|combo_points.deficit>=cp_max_spend-1)&!buff.dreadblades.up", "If adds are up, snipe the one with lowest TTD. Use when dying faster than CP deficit or without any CP." );
  cds->add_action( "marked_for_death,if=raid_event.adds.in>30-raid_event.adds.duration&combo_points.deficit>=cp_max_spend-1&!buff.dreadblades.up", "If no adds will die within the next 30s, use MfD on boss without any CP." );
  cds->add_action( "thistle_tea,if=!buff.thistle_tea.up&(energy.base_deficit>=100|fight_remains<charges*6)" );
  cds->add_action( "killing_spree,if=variable.blade_flurry_sync&!stealthed.rogue&debuff.between_the_eyes.up&energy.base_time_to_max>4" );
  cds->add_action( "shadowmeld,if=!stealthed.all&(talent.count_the_odds&variable.finish_condition|!talent.weaponmaster.enabled&variable.ambush_condition)" );
  cds->add_action( "potion,if=buff.bloodlust.react|fight_remains<30|buff.adrenaline_rush.up" );
  cds->add_action( "blood_fury" );
  cds->add_action( "berserking" );
  cds->add_action( "fireblood" );
  cds->add_action( "ancestral_call" );
  cds->add_action( "use_item,name=manic_grieftorch,use_off_gcd=1,if=gcd.remains>gcd.max-0.1&!stealthed.all&debuff.between_the_eyes.up&(!talent.ghostly_strike|debuff.ghostly_strike.up|spell_targets.blade_flurry>2)|fight_remains<=5", "Default conditions for usable items." );
  cds->add_action( "use_item,name=beacon_to_the_beyond,use_off_gcd=1,if=gcd.remains>gcd.max-0.1&!stealthed.all&debuff.between_the_eyes.up&(!talent.ghostly_strike|debuff.ghostly_strike.up|spell_targets.blade_flurry>2)|fight_remains<=5" );
  cds->add_action( "use_item,name=stormeaters_boon,if=spell_targets.blade_flurry>desired_targets|raid_event.adds.in>60|fight_remains<10" );
  cds->add_action( "use_item,name=windscar_whetstone,if=spell_targets.blade_flurry>desired_targets|raid_event.adds.in>60|fight_remains<7" );
  cds->add_action( "use_items,slots=trinket1,if=debuff.between_the_eyes.up|trinket.1.has_stat.any_dps|fight_remains<=20" );
  cds->add_action( "use_items,slots=trinket2,if=debuff.between_the_eyes.up|trinket.2.has_stat.any_dps|fight_remains<=20" );

  stealth_cds->add_action( "variable,name=vanish_condition,value=talent.hidden_opportunity|!talent.shadow_dance|!cooldown.shadow_dance.ready", "Stealth Cooldowns" );
  stealth_cds->add_action( "variable,name=vanish_opportunity_condition,value=!talent.shadow_dance&talent.fan_the_hammer.rank+talent.quick_draw+talent.audacity<talent.count_the_odds+talent.keep_it_rolling" );
  stealth_cds->add_action( "vanish,if=talent.find_weakness&!talent.audacity&debuff.find_weakness.down&variable.ambush_condition&variable.vanish_condition" );
  stealth_cds->add_action( "vanish,if=talent.hidden_opportunity&!buff.audacity.up&(variable.vanish_opportunity_condition|buff.opportunity.stack<buff.opportunity.max_stack)&variable.ambush_condition&variable.vanish_condition" );
  stealth_cds->add_action( "vanish,if=(!talent.find_weakness|talent.audacity)&!talent.hidden_opportunity&variable.finish_condition&variable.vanish_condition" );
  stealth_cds->add_action( "variable,name=shadow_dance_condition,value=talent.shadow_dance&debuff.between_the_eyes.up&(!talent.ghostly_strike|debuff.ghostly_strike.up)&(!talent.dreadblades|!cooldown.dreadblades.ready)&(!talent.hidden_opportunity|!buff.audacity.up&(talent.fan_the_hammer.rank<2|!buff.opportunity.up))" );
  stealth_cds->add_action( "shadow_dance,if=!talent.keep_it_rolling&variable.shadow_dance_condition&buff.slice_and_dice.up&(variable.finish_condition|talent.hidden_opportunity)&(!talent.hidden_opportunity|!cooldown.vanish.ready)" );
  stealth_cds->add_action( "shadow_dance,if=talent.keep_it_rolling&variable.shadow_dance_condition&(cooldown.keep_it_rolling.remains<=30|cooldown.keep_it_rolling.remains>120&(variable.finish_condition|talent.hidden_opportunity))" );

  finish->add_action( "between_the_eyes,if=target.time_to_die>3&(debuff.between_the_eyes.remains<4|talent.greenskins_wickers&!buff.greenskins_wickers.up|!talent.greenskins_wickers&talent.improved_between_the_eyes&buff.ruthless_precision.up)", "Finishers  BtE to keep the Crit debuff up, if RP is up, or for Greenskins, unless the target is about to die." );
  finish->add_action( "slice_and_dice,if=buff.slice_and_dice.remains<fight_remains&refreshable&(buff.grand_melee.down|ptr)&(!talent.swift_slasher|combo_points>=cp_max_spend)" );
  finish->add_action( "cold_blood" );
  finish->add_action( "dispatch" );

  stealth->add_action( "blade_flurry,if=talent.subterfuge&talent.hidden_opportunity&spell_targets>=2&!buff.blade_flurry.up", "Stealth" );
  stealth->add_action( "cold_blood,if=variable.finish_condition" );
  stealth->add_action( "dispatch,if=variable.finish_condition" );
  stealth->add_action( "ambush,if=variable.stealthed_cto|stealthed.basic&talent.find_weakness&!debuff.find_weakness.up|talent.hidden_opportunity" );
}
//outlaw_df_apl_end

//subtlety_df_apl_start
void subtlety_df( player_t* p )
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
  precombat->add_action( "marked_for_death,precombat_seconds=15" );
  precombat->add_action( "variable,name=algethar_puzzle_box_precombat_cast,value=3" );
  precombat->add_action( "use_item,name=algethar_puzzle_box" );
  precombat->add_action( "slice_and_dice,precombat_seconds=1" );

  default_->add_action( "stealth", "Restealth if possible (no vulnerable enemies in combat)" );
  default_->add_action( "kick", "Interrupt on cooldown to allow simming interactions with that" );
  default_->add_action( "variable,name=snd_condition,value=buff.slice_and_dice.up|spell_targets.shuriken_storm>=cp_max_spend", "Used to determine whether cooldowns wait for SnD based on targets." );
  default_->add_action( "variable,name=is_next_cp_animacharged,if=talent.echoing_reprimand.enabled,value=combo_points=1&buff.echoing_reprimand_2.up|combo_points=2&buff.echoing_reprimand_3.up|combo_points=3&buff.echoing_reprimand_4.up|combo_points=4&buff.echoing_reprimand_5.up", "Check to see if the next CP (in the event of a ShT proc) is Animacharged" );
  default_->add_action( "variable,name=effective_combo_points,value=effective_combo_points", "Account for ShT reaction time by ignoring low-CP animacharged matches in the 0.5s preceeding a potential ShT proc" );
  default_->add_action( "variable,name=effective_combo_points,if=talent.echoing_reprimand.enabled&effective_combo_points>combo_points&combo_points.deficit>2&time_to_sht.4.plus<0.5&!variable.is_next_cp_animacharged,value=combo_points" );
  default_->add_action( "call_action_list,name=cds", "Check CDs at first" );
  default_->add_action( "slice_and_dice,if=spell_targets.shuriken_storm<cp_max_spend&buff.slice_and_dice.remains<gcd.max&fight_remains>6&combo_points>=4", "Apply Slice and Dice at 4+ CP if it expires within the next GCD or is not up" );
  default_->add_action( "run_action_list,name=stealthed,if=stealthed.all", "Run fully switches to the Stealthed Rotation (by doing so, it forces pooling if nothing is available)." );
  default_->add_action( "variable,name=priority_rotation,value=priority_rotation", "Only change rotation if we have priority_rotation set." );
  default_->add_action( "variable,name=stealth_threshold,value=25+talent.vigor.enabled*20+talent.master_of_shadows.enabled*20+talent.shadow_focus.enabled*25+talent.alacrity.enabled*20+25*(spell_targets.shuriken_storm>=4)", "Used to define when to use stealth CDs or builders" );
  default_->add_action( "call_action_list,name=stealth_cds,if=energy.deficit<=variable.stealth_threshold", "Consider using a Stealth CD when reaching the energy threshold" );
  default_->add_action( "call_action_list,name=finish,if=variable.effective_combo_points>=cp_max_spend" );
  default_->add_action( "call_action_list,name=finish,if=combo_points.deficit<=1+buff.the_rotten.up|fight_remains<=1&variable.effective_combo_points>=3", "Finish at maximum or close to maximum combo point value" );
  default_->add_action( "call_action_list,name=finish,if=spell_targets.shuriken_storm>=(4-talent.seal_fate)&variable.effective_combo_points>=4", "Finish at 4+ against 4 targets (outside stealth)" );
  default_->add_action( "call_action_list,name=build,if=energy.deficit<=variable.stealth_threshold", "Use a builder when reaching the energy threshold" );
  default_->add_action( "arcane_torrent,if=energy.deficit>=15+energy.regen", "Lowest priority in all of the APL because it causes a GCD" );
  default_->add_action( "arcane_pulse" );
  default_->add_action( "lights_judgment" );
  default_->add_action( "bag_of_tricks" );

  build->add_action( "shuriken_storm,if=spell_targets>=2+(talent.gloomblade&buff.lingering_shadow.remains>=6|buff.perforated_veins.up)", "Builders" );
  build->add_action( "variable,name=anima_helper,value=!talent.echoing_reprimand.enabled|!(variable.is_next_cp_animacharged&(time_to_sht.3.plus<0.5|time_to_sht.4.plus<1)&energy<60)", "Build immediately unless the next CP is Animacharged and we won't cap energy waiting for it." );
  build->add_action( "gloomblade,if=variable.anima_helper" );
  build->add_action( "backstab,if=variable.anima_helper" );

  cds->add_action( "variable,name=rotten_condition,value=!buff.premeditation.up&spell_targets.shuriken_storm=1|!talent.the_rotten|spell_targets.shuriken_storm>1", "Cooldowns" );
  cds->add_action( "shadow_dance,use_off_gcd=1,if=!buff.shadow_dance.up&buff.shuriken_tornado.up&buff.shuriken_tornado.remains<=3.5", "Cooldowns Use Dance off-gcd before the first Shuriken Storm from Tornado comes in." );
  cds->add_action( "symbols_of_death,use_off_gcd=1,if=buff.shuriken_tornado.up&buff.shuriken_tornado.remains<=3.5", "(Unless already up because we took Shadow Focus) use Symbols off-gcd before the first Shuriken Storm from Tornado comes in." );
  cds->add_action( "vanish,if=buff.danse_macabre.stack>3&combo_points<=2&(cooldown.secret_technique.remains>=30|!talent.secret_technique)", "Vanish for Shadowstrike with Danse Macabre at adaquate stacks" );
  cds->add_action( "cold_blood,if=!talent.secret_technique&combo_points>=5", "Cold Blood on 5 combo points when not playing Secret Technique" );
  cds->add_action( "flagellation,target_if=max:target.time_to_die,if=variable.snd_condition&combo_points>=5&target.time_to_die>10" );
  cds->add_action( "pool_resource,for_next=1,if=talent.shuriken_tornado.enabled&!talent.shadow_focus.enabled", "Pool for Tornado pre-SoD with ShD ready when not running SF." );
  cds->add_action( "shuriken_tornado,if=spell_targets.shuriken_storm<=1&energy>=60&variable.snd_condition&cooldown.symbols_of_death.up&cooldown.shadow_dance.charges>=1&(!talent.flagellation.enabled&!cooldown.flagellation.up|buff.flagellation_buff.up|spell_targets.shuriken_storm>=5)&combo_points<=2&!buff.premeditation.up", "Use Tornado pre SoD when we have the energy whether from pooling without SF or just generally." );
  cds->add_action( "sepsis,if=variable.snd_condition&combo_points.deficit>=1&target.time_to_die>=16" );
  cds->add_action( "symbols_of_death,if=(buff.symbols_of_death.remains<=3&!cooldown.shadow_dance.ready|!set_bonus.tier30_2pc)&variable.rotten_condition&variable.snd_condition&(!talent.flagellation&(combo_points<=1|!talent.the_rotten)|cooldown.flagellation.remains>10|cooldown.flagellation.up&combo_points>=5)", "Use Symbols on cooldown (after first SnD) unless we are going to pop Tornado and do not have Shadow Focus." );
  cds->add_action( "marked_for_death,line_cd=1.5,target_if=min:target.time_to_die,if=raid_event.adds.up&(target.time_to_die<combo_points.deficit|!stealthed.all&combo_points.deficit>=cp_max_spend)", "If adds are up, snipe the one with lowest TTD. Use when dying faster than CP deficit or not stealthed without any CP." );
  cds->add_action( "marked_for_death,if=raid_event.adds.in>30-raid_event.adds.duration&combo_points.deficit>=cp_max_spend", "If no adds will die within the next 30s, use MfD on boss without any CP." );
  cds->add_action( "shadow_blades,if=variable.snd_condition&combo_points.deficit>=2&target.time_to_die>=10&(dot.sepsis.ticking|cooldown.sepsis.remains<=8|!talent.sepsis)|fight_remains<=20" );
  cds->add_action( "echoing_reprimand,if=variable.snd_condition&combo_points.deficit>=3&(variable.priority_rotation|spell_targets.shuriken_storm<=4|talent.resounding_clarity)&(buff.shadow_dance.up|!talent.danse_macabre)" );
  cds->add_action( "shuriken_tornado,if=variable.snd_condition&buff.symbols_of_death.up&combo_points<=2&(!buff.premeditation.up|spell_targets.shuriken_storm>4)", "With SF, if not already done, use Tornado with SoD up." );
  cds->add_action( "shuriken_tornado,if=cooldown.shadow_dance.ready&!stealthed.all&spell_targets.shuriken_storm>=3&!talent.flagellation.enabled" );
  cds->add_action( "shadow_dance,if=!buff.shadow_dance.up&fight_remains<=8+talent.subterfuge.enabled" );
  cds->add_action( "thistle_tea,if=(cooldown.symbols_of_death.remains>=3|buff.symbols_of_death.up)&!buff.thistle_tea.up&(energy.deficit>=100&(combo_points.deficit>=2|spell_targets.shuriken_storm>=3)|cooldown.thistle_tea.charges_fractional>=2.75&buff.shadow_dance.up)|buff.shadow_dance.remains>=4&!buff.thistle_tea.up&spell_targets.shuriken_storm>=3|!buff.thistle_tea.up&fight_remains<=(6*cooldown.thistle_tea.charges)" );
  cds->add_action( "potion,if=buff.bloodlust.react|fight_remains<30|buff.symbols_of_death.up&(buff.shadow_blades.up|cooldown.shadow_blades.remains<=10)" );
  cds->add_action( "blood_fury,if=buff.symbols_of_death.up" );
  cds->add_action( "berserking,if=buff.symbols_of_death.up" );
  cds->add_action( "fireblood,if=buff.symbols_of_death.up" );
  cds->add_action( "ancestral_call,if=buff.symbols_of_death.up" );
  cds->add_action( "use_item,name=beacon_to_the_beyond,use_off_gcd=1,if=!stealthed.all&(buff.deeper_daggers.up|!talent.deeper_daggers)&(!raid_event.adds.up|!equipped.stormeaters_boon|trinket.stormeaters_boon.cooldown.remains>20)" );
  cds->add_action( "use_item,name=manic_grieftorch,use_off_gcd=1,if=!stealthed.all&(!raid_event.adds.up|!equipped.stormeaters_boon|trinket.stormeaters_boon.cooldown.remains>20)" );
  cds->add_action( "use_items,if=!stealthed.all|fight_remains<10", "Default fallback for usable items: Use with Symbols of Death." );

  finish->add_action( "variable,name=secret_condition,value=buff.shadow_dance.up&(buff.danse_macabre.stack>=3|!talent.danse_macabre)&(!buff.premeditation.up|spell_targets.shuriken_storm!=2)", "Finishers While using Premeditation, avoid casting Slice and Dice when Shadow Dance is soon to be used, except for Kyrian" );
  finish->add_action( "variable,name=premed_snd_condition,value=talent.premeditation.enabled&spell_targets.shuriken_storm<5" );
  finish->add_action( "slice_and_dice,if=!variable.premed_snd_condition&spell_targets.shuriken_storm<6&!buff.shadow_dance.up&buff.slice_and_dice.remains<fight_remains&refreshable" );
  finish->add_action( "slice_and_dice,if=variable.premed_snd_condition&cooldown.shadow_dance.charges_fractional<1.75&buff.slice_and_dice.remains<cooldown.symbols_of_death.remains&(cooldown.shadow_dance.ready&buff.symbols_of_death.remains-buff.shadow_dance.remains<1.2)" );
  finish->add_action( "variable,name=skip_rupture,value=buff.thistle_tea.up&spell_targets.shuriken_storm=1|buff.shadow_dance.up&(spell_targets.shuriken_storm=1|dot.rupture.ticking&spell_targets.shuriken_storm>=2)" );
  finish->add_action( "rupture,if=(!variable.skip_rupture|variable.priority_rotation)&target.time_to_die-remains>6&refreshable", "Keep up Rupture if it is about to run out." );
  finish->add_action( "rupture,if=!variable.skip_rupture&buff.finality_rupture.up&cooldown.shadow_dance.remains<12&cooldown.shadow_dance.charges_fractional<=1&spell_targets.shuriken_storm=1&(talent.dark_brew|talent.danse_macabre)", "Refresh Rupture early for Finality" );
  finish->add_action( "cold_blood,if=variable.secret_condition&cooldown.secret_technique.ready", "Sync Cold Blood with Secret Technique when possible" );
  finish->add_action( "secret_technique,if=variable.secret_condition&(!talent.cold_blood|cooldown.cold_blood.remains>buff.shadow_dance.remains-2)" );
  finish->add_action( "rupture,cycle_targets=1,if=!variable.skip_rupture&!variable.priority_rotation&spell_targets.shuriken_storm>=2&target.time_to_die>=(2*combo_points)&refreshable", "Multidotting targets that will live for the duration of Rupture, refresh during pandemic." );
  finish->add_action( "rupture,if=!variable.skip_rupture&remains<cooldown.symbols_of_death.remains+10&cooldown.symbols_of_death.remains<=5&target.time_to_die-remains>cooldown.symbols_of_death.remains+5", "Refresh Rupture early if it will expire during Symbols. Do that refresh if SoD gets ready in the next 5s." );
  finish->add_action( "black_powder,if=!variable.priority_rotation&spell_targets>=3|!used_for_danse&buff.shadow_dance.up&spell_targets.shuriken_storm=2&talent.danse_macabre" );
  finish->add_action( "eviscerate" );

  stealth_cds->add_action( "variable,name=shd_threshold,value=cooldown.shadow_dance.charges_fractional>=0.75+talent.shadow_dance", "Stealth Cooldowns Helper Variable" );
  stealth_cds->add_action( "variable,name=rotten_threshold,value=!buff.the_rotten.up|spell_targets.shuriken_storm>1|combo_points<=2&buff.the_rotten.up&!set_bonus.tier30_2pc" );
  stealth_cds->add_action( "vanish,if=(!talent.danse_macabre|spell_targets.shuriken_storm>=3)&!variable.shd_threshold&combo_points.deficit>1&(cooldown.flagellation.remains>=60|!talent.flagellation|fight_remains<=(30*cooldown.vanish.charges))" );
  stealth_cds->add_action( "pool_resource,for_next=1,extra_amount=40,if=race.night_elf", "Pool for Shadowmeld + Shadowstrike unless we are about to cap on Dance charges. Only when Find Weakness is about to run out." );
  stealth_cds->add_action( "shadowmeld,if=energy>=40&energy.deficit>=10&!variable.shd_threshold&combo_points.deficit>4" );
  stealth_cds->add_action( "variable,name=shd_combo_points,value=combo_points<=1", "CP thresholds for entering Shadow Dance Default to start dance with 0 or 1 combo point" );
  stealth_cds->add_action( "variable,name=shd_combo_points,value=combo_points.deficit<=1,if=spell_targets.shuriken_storm>(4-2*talent.shuriken_tornado.enabled)|variable.priority_rotation&spell_targets.shuriken_storm>=4", "Use stealth cooldowns with high combo points when playing shuriken tornado or with high target counts" );
  stealth_cds->add_action( "variable,name=shd_combo_points,value=1,if=spell_targets.shuriken_storm=(4-talent.seal_fate)", "Use stealth cooldowns on any combo point on 4 targets" );
  stealth_cds->add_action( "shadow_dance,if=(variable.shd_combo_points&(!talent.shadow_dance&buff.symbols_of_death.remains>=(2.2-talent.flagellation.enabled)|variable.shd_threshold)|talent.shadow_dance&cooldown.secret_technique.remains<=9&(spell_targets.shuriken_storm<=3|talent.danse_macabre)|buff.flagellation.up|buff.flagellation_persist.remains>=6|spell_targets.shuriken_storm>=4&cooldown.symbols_of_death.remains>10)&variable.rotten_threshold", "Dance during Symbols or above threshold." );
  stealth_cds->add_action( "shadow_dance,if=variable.shd_combo_points&fight_remains<cooldown.symbols_of_death.remains|!talent.shadow_dance&dot.rupture.ticking&spell_targets.shuriken_storm<=4&variable.rotten_threshold", "Burn Dances charges if before the fight ends if SoD won't be ready in time." );

  stealthed->add_action( "shadowstrike,if=(buff.stealth.up|buff.vanish.up)&(spell_targets.shuriken_storm<4|variable.priority_rotation)", "Stealthed Rotation If Stealth/vanish are up, use Shadowstrike to benefit from the passive bonus and Find Weakness, even if we are at max CP (unless using Master Assassin)" );
  stealthed->add_action( "variable,name=gloomblade_condition,value=buff.danse_macabre.stack<5&(combo_points.deficit=2|combo_points.deficit=3)&(buff.premeditation.up|effective_combo_points<7)&(spell_targets.shuriken_storm<=8|talent.lingering_shadow)", "Variable to Gloomblade / Backstab when on 4 or 5 combo points with premediation and when the combo point is not anima charged" );
  stealthed->add_action( "shuriken_storm,if=variable.gloomblade_condition&buff.silent_storm.up&!debuff.find_weakness.remains&talent.improved_shuriken_storm.enabled|combo_points<=1&!used_for_danse&spell_targets.shuriken_storm=2&talent.danse_macabre" );
  stealthed->add_action( "gloomblade,if=variable.gloomblade_condition&(!used_for_danse|spell_targets.shuriken_storm!=2)|combo_points<=2&buff.the_rotten.up&spell_targets.shuriken_storm<=3" );
  stealthed->add_action( "backstab,if=variable.gloomblade_condition&talent.danse_macabre&buff.danse_macabre.stack<=2&spell_targets.shuriken_storm<=2" );
  stealthed->add_action( "call_action_list,name=finish,if=variable.effective_combo_points>=cp_max_spend" );
  stealthed->add_action( "call_action_list,name=finish,if=buff.shuriken_tornado.up&combo_points.deficit<=2", "Finish earlier with Shuriken tornado up." );
  stealthed->add_action( "call_action_list,name=finish,if=spell_targets.shuriken_storm>=4-talent.seal_fate&variable.effective_combo_points>=4", "Also safe to finish at 4+ CP with exactly 4 targets. (Same as outside stealth.)" );
  stealthed->add_action( "call_action_list,name=finish,if=combo_points.deficit<=1+(talent.seal_fate|talent.deeper_stratagem|talent.secret_stratagem)", "Finish at lower combo points if you are talented in DS, SS or Seal Fate" );
  stealthed->add_action( "gloomblade,if=buff.perforated_veins.stack>=5&spell_targets.shuriken_storm<3", "Use Gloomblade or Backstab when close to hitting max PV stacks" );
  stealthed->add_action( "backstab,if=buff.perforated_veins.stack>=5&spell_targets.shuriken_storm<3" );
  stealthed->add_action( "shadowstrike,if=stealthed.sepsis&spell_targets.shuriken_storm<4" );
  stealthed->add_action( "shuriken_storm,if=spell_targets>=3+buff.the_rotten.up&(!buff.premeditation.up|spell_targets>=7&!variable.priority_rotation)" );
  stealthed->add_action( "shadowstrike,if=debuff.find_weakness.remains<=1|cooldown.symbols_of_death.remains<18&debuff.find_weakness.remains<cooldown.symbols_of_death.remains", "Shadowstrike to refresh Find Weakness and to ensure we can carry over a full FW into the next SoD if possible." );
  stealthed->add_action( "shadowstrike" );
}
//subtlety_df_apl_end

} // namespace rogue_apl
