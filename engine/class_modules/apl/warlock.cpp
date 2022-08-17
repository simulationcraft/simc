#include "class_modules/apl/warlock.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace warlock_apl{
  std::string potion( const player_t* p )
  {
    return ( p->true_level >= 60 ) ? "spectral_intellect" : "disabled";
  }

  std::string flask( const player_t* p )
  {
    return ( p->true_level >= 60 ) ? "spectral_flask_of_power" : "disabled";
  }

  std::string food( const player_t* p )
  {
    return ( p->true_level >= 60 ) ? "feast_of_gluttonous_hedonism" : "disabled";
  }

  std::string rune( const player_t* p )
  {
    return ( p->true_level >= 60 ) ? "veiled" : "disabled";
  }

  std::string temporary_enchant( const player_t* p )
  {
    return ( p->true_level >= 60 ) ? "main_hand:shadowcore_oil" : "disabled";
  }

//affliction_apl_start
void affliction( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* covenant = p->get_action_priority_list( "covenant" );
  action_priority_list_t* damage_trinkets = p->get_action_priority_list( "damage_trinkets" );
  action_priority_list_t* darkglare_prep = p->get_action_priority_list( "darkglare_prep" );
  action_priority_list_t* delayed_trinkets = p->get_action_priority_list( "delayed_trinkets" );
  action_priority_list_t* dot_prep = p->get_action_priority_list( "dot_prep" );
  action_priority_list_t* item = p->get_action_priority_list( "item" );
  action_priority_list_t* necro_mw = p->get_action_priority_list( "necro_mw" );
  action_priority_list_t* se = p->get_action_priority_list( "se" );
  action_priority_list_t* stat_trinkets = p->get_action_priority_list( "stat_trinkets" );
  action_priority_list_t* trinket_split_check = p->get_action_priority_list( "trinket_split_check" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "summon_pet" );
  precombat->add_action( "use_item,name=tome_of_monstrous_constructions" );
  precombat->add_action( "use_item,name=soleahs_secret_technique" );
  precombat->add_action( "grimoire_of_sacrifice,if=talent.grimoire_of_sacrifice.enabled" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "fleshcraft" );
  precombat->add_action( "seed_of_corruption,if=spell_targets.seed_of_corruption_aoe>=3" );
  precombat->add_action( "haunt" );
  precombat->add_action( "unstable_affliction" );

  default_->add_action( "call_action_list,name=aoe,if=active_enemies>3" );
  default_->add_action( "malefic_rapture,if=buff.calamitous_crescendo.up" );
  default_->add_action( "run_action_list,name=necro_mw,if=covenant.necrolord&runeforge.malefic_wrath&active_enemies=1&talent.phantom_singularity", "Call separate action list for Necrolord MW in ST. Currently only optimized for use with PS." );
  default_->add_action( "call_action_list,name=trinket_split_check,if=time<1", "Action lists for trinket behavior. Stats are saved for before Soul Rot/Impending Catastrophe/Phantom Singularity, otherwise on cooldown" );
  default_->add_action( "call_action_list,name=delayed_trinkets" );
  default_->add_action( "call_action_list,name=stat_trinkets,if=(dot.soul_rot.ticking|dot.impending_catastrophe_dot.ticking|dot.phantom_singularity.ticking)&soul_shard>3|dot.vile_taint.ticking|talent.sow_the_seeds" );
  default_->add_action( "call_action_list,name=damage_trinkets,if=covenant.night_fae&(!variable.trinket_split|cooldown.soul_rot.remains>20|(variable.trinket_one&cooldown.soul_rot.remains<trinket.1.cooldown.remains)|(variable.trinket_two&cooldown.soul_rot.remains<trinket.2.cooldown.remains))" );
  default_->add_action( "call_action_list,name=damage_trinkets,if=covenant.venthyr&(!variable.trinket_split|cooldown.impending_catastrophe.remains>20|(variable.trinket_one&cooldown.impending_catastrophe.remains<trinket.1.cooldown.remains)|(variable.trinket_two&cooldown.impending_catastrophe.remains<trinket.2.cooldown.remains))" );
  default_->add_action( "call_action_list,name=damage_trinkets,if=(covenant.necrolord|covenant.kyrian|covenant.none)&(!variable.trinket_split|cooldown.phantom_singularity.remains>20|(variable.trinket_one&cooldown.phantom_singularity.remains<trinket.1.cooldown.remains)|(variable.trinket_two&cooldown.phantom_singularity.remains<trinket.2.cooldown.remains))" );
  default_->add_action( "call_action_list,name=damage_trinkets,if=!talent.phantom_singularity.enabled&(!variable.trinket_split|cooldown.summon_darkglare.remains>20|(variable.trinket_one&cooldown.summon_darkglare.remains<trinket.1.cooldown.remains)|(variable.trinket_two&cooldown.summon_darkglare.remains<trinket.2.cooldown.remains))" );
  default_->add_action( "malefic_rapture,if=time_to_die<execute_time*soul_shard&dot.unstable_affliction.ticking", "Burn soul shards if fight is almost over" );
  default_->add_action( "call_action_list,name=darkglare_prep,if=covenant.venthyr&dot.impending_catastrophe_dot.ticking&cooldown.summon_darkglare.remains<2&(dot.phantom_singularity.remains>2|!talent.phantom_singularity)", "If covenant dot/Phantom Singularity is running, use Darkglare to extend the current set" );
  default_->add_action( "call_action_list,name=darkglare_prep,if=covenant.night_fae&dot.soul_rot.ticking&cooldown.summon_darkglare.remains<2&(dot.phantom_singularity.remains>2|!talent.phantom_singularity)" );
  default_->add_action( "call_action_list,name=darkglare_prep,if=(covenant.necrolord|covenant.kyrian|covenant.none)&dot.phantom_singularity.ticking&dot.phantom_singularity.remains<2", "If using Phantom Singularity on cooldown, make sure to extend it before it runs out" );
  default_->add_action( "call_action_list,name=dot_prep,if=covenant.night_fae&!dot.soul_rot.ticking&cooldown.soul_rot.remains<4", "Refresh dots early if going into a shard spending phase" );
  default_->add_action( "call_action_list,name=dot_prep,if=covenant.venthyr&!dot.impending_catastrophe_dot.ticking&cooldown.impending_catastrophe.remains<4" );
  default_->add_action( "call_action_list,name=dot_prep,if=(covenant.necrolord|covenant.kyrian|covenant.none)&talent.phantom_singularity&!dot.phantom_singularity.ticking&cooldown.phantom_singularity.remains<4" );
  default_->add_action( "dark_soul,if=dot.phantom_singularity.ticking", "If Phantom Singularity is ticking, it is safe to use Dark Soul" );
  default_->add_action( "dark_soul,if=!talent.phantom_singularity&(dot.soul_rot.ticking|dot.impending_catastrophe_dot.ticking)" );
  default_->add_action( "phantom_singularity,if=covenant.night_fae&time>5&cooldown.soul_rot.remains<1&(trinket.empyreal_ordnance.cooldown.remains<162|!equipped.empyreal_ordnance)", "Sync Phantom Singularity with Venthyr/Night Fae covenant dot, otherwise use on cooldown. If Empyreal Ordnance buff is incoming, hold until it's ready (18 seconds after use)" );
  default_->add_action( "phantom_singularity,if=covenant.venthyr&time>5&cooldown.impending_catastrophe.remains<1&(trinket.empyreal_ordnance.cooldown.remains<162|!equipped.empyreal_ordnance)" );
  default_->add_action( "phantom_singularity,if=covenant.necrolord&runeforge.malefic_wrath&time>5&cooldown.decimating_bolt.remains<3&(trinket.empyreal_ordnance.cooldown.remains<162|!equipped.empyreal_ordnance)", "Necrolord with Malefic Wrath casts phantom singularity in line with Decimating Bolt" );
  default_->add_action( "phantom_singularity,if=(covenant.kyrian|covenant.none|(covenant.necrolord&!runeforge.malefic_wrath))&(trinket.empyreal_ordnance.cooldown.remains<162|!equipped.empyreal_ordnance)", "Other covenants (including non-MW Necro) cast PS on cooldown" );
  default_->add_action( "phantom_singularity,if=time_to_die<16" );
  default_->add_action( "call_action_list,name=covenant,if=dot.phantom_singularity.ticking&(covenant.night_fae|covenant.venthyr)", "If Phantom Singularity is ticking, it's time to use other major dots" );
  default_->add_action( "agony,cycle_targets=1,target_if=dot.agony.remains<4" );
  default_->add_action( "haunt" );
  default_->add_action( "seed_of_corruption,if=active_enemies>2&talent.sow_the_seeds&!dot.seed_of_corruption.ticking&!in_flight", "Sow the Seeds on 3 targets if it isn't currently in flight or on the target. With Siphon Life it's also better to use Seed over manually applying 3 Corruptions." );
  default_->add_action( "seed_of_corruption,if=active_enemies>2&talent.siphon_life&!dot.seed_of_corruption.ticking&!in_flight&dot.corruption.remains<4" );
  default_->add_action( "vile_taint,if=(soul_shard>1|active_enemies>2)&cooldown.summon_darkglare.remains>12" );
  default_->add_action( "unstable_affliction,if=dot.unstable_affliction.remains<4" );
  default_->add_action( "siphon_life,cycle_targets=1,target_if=dot.siphon_life.remains<4" );
  default_->add_action( "call_action_list,name=covenant,if=!covenant.necrolord", "If not using Phantom Singularity, don't apply covenant dots until other core dots are safe" );
  default_->add_action( "corruption,cycle_targets=1,if=active_enemies<4-(talent.sow_the_seeds|talent.siphon_life),target_if=dot.corruption.remains<2", "Apply Corruption manually on 1-2 targets, or on 3 with Absolute Corruption" );
  default_->add_action( "malefic_rapture,if=soul_shard>4&time>21", "After the opener, spend a shard when at 5 on Malefic Rapture to avoid overcapping" );
  default_->add_action( "call_action_list,name=darkglare_prep,if=covenant.venthyr&!talent.phantom_singularity&dot.impending_catastrophe_dot.ticking&cooldown.summon_darkglare.ready", "When not syncing Phantom Singularity to Venthyr/Night Fae, Summon Darkglare if all dots are applied" );
  default_->add_action( "call_action_list,name=darkglare_prep,if=covenant.night_fae&!talent.phantom_singularity&dot.soul_rot.ticking&cooldown.summon_darkglare.ready" );
  default_->add_action( "call_action_list,name=darkglare_prep,if=(covenant.necrolord|covenant.kyrian|covenant.none)&cooldown.summon_darkglare.ready" );
  default_->add_action( "dark_soul,if=cooldown.summon_darkglare.remains>time_to_die&(!talent.phantom_singularity|cooldown.phantom_singularity.remains>time_to_die)", "Use Dark Soul if Darkglare won't be ready again, or if there will be at least 2 more Darkglare uses" );
  default_->add_action( "dark_soul,if=!talent.phantom_singularity&cooldown.summon_darkglare.remains+cooldown.summon_darkglare.duration<time_to_die" );
  default_->add_action( "call_action_list,name=item", "Catch-all item usage for anything not specified elsewhere" );
  default_->add_action( "call_action_list,name=se,if=talent.shadow_embrace&(debuff.shadow_embrace.stack<(2-action.shadow_bolt.in_flight)|debuff.shadow_embrace.remains<3)" );
  default_->add_action( "malefic_rapture,if=(dot.vile_taint.ticking|dot.impending_catastrophe_dot.ticking|dot.soul_rot.ticking)&(!runeforge.malefic_wrath|buff.malefic_wrath.stack<3|soul_shard>1)", "Use Malefic Rapture when major dots are up, or if there will be significant time until the next Phantom Singularity. If utilizing Malefic Wrath, hold a shard to refresh the buff" );
  default_->add_action( "malefic_rapture,if=runeforge.malefic_wrath&cooldown.soul_rot.remains>20&buff.malefic_wrath.remains<4", "Use Malefic Rapture to maintain the malefic wrath buff until shards need to be generated for the next burst window (20 seconds is more than sufficient to generate 3 shards)" );
  default_->add_action( "malefic_rapture,if=runeforge.malefic_wrath&(covenant.necrolord|covenant.kyrian)&buff.malefic_wrath.remains<4", "Maintain Malefic Wrath at all times for the Necrolord or Kyrian covenant" );
  default_->add_action( "malefic_rapture,if=talent.phantom_singularity&(dot.phantom_singularity.ticking|cooldown.phantom_singularity.remains>25|time_to_die<cooldown.phantom_singularity.remains)&(!runeforge.malefic_wrath|buff.malefic_wrath.stack<3|soul_shard>1)", "Use Malefic Rapture on Phantom Singularity casts, making sure to save a shard to stack Malefic Wrath if using it" );
  default_->add_action( "malefic_rapture,if=talent.sow_the_seeds" );
  default_->add_action( "drain_life,if=buff.inevitable_demise.stack>40|buff.inevitable_demise.up&time_to_die<4", "Drain Life is only a DPS gain with Inevitable Demise near max stacks. If fight is about to end do not miss spending the stacks" );
  default_->add_action( "call_action_list,name=covenant" );
  default_->add_action( "agony,cycle_targets=1,target_if=refreshable" );
  default_->add_action( "unstable_affliction,if=refreshable" );
  default_->add_action( "siphon_life,cycle_targets=1,target_if=refreshable" );
  default_->add_action( "corruption,cycle_targets=1,if=active_enemies<4-(talent.sow_the_seeds|talent.siphon_life),target_if=refreshable" );
  default_->add_action( "fleshcraft,if=soulbind.volatile_solvent,cancel_if=buff.volatile_solvent_humanoid.up" );
  default_->add_action( "drain_soul,interrupt=1" );
  default_->add_action( "shadow_bolt" );

  aoe->add_action( "phantom_singularity" );
  aoe->add_action( "haunt" );
  aoe->add_action( "call_action_list,name=darkglare_prep,if=covenant.venthyr&dot.impending_catastrophe_dot.ticking&cooldown.summon_darkglare.ready&(dot.phantom_singularity.remains>2|!talent.phantom_singularity)" );
  aoe->add_action( "call_action_list,name=darkglare_prep,if=covenant.night_fae&dot.soul_rot.ticking&cooldown.summon_darkglare.ready&(dot.phantom_singularity.remains>2|!talent.phantom_singularity)" );
  aoe->add_action( "call_action_list,name=darkglare_prep,if=(covenant.necrolord|covenant.kyrian|covenant.none)&dot.phantom_singularity.ticking&dot.phantom_singularity.remains<2" );
  aoe->add_action( "seed_of_corruption,if=talent.sow_the_seeds&can_seed" );
  aoe->add_action( "seed_of_corruption,if=!talent.sow_the_seeds&!dot.seed_of_corruption.ticking&!in_flight&dot.corruption.refreshable" );
  aoe->add_action( "agony,cycle_targets=1,if=active_dot.agony<4,target_if=!dot.agony.ticking" );
  aoe->add_action( "agony,cycle_targets=1,if=active_dot.agony>=4,target_if=refreshable&dot.agony.ticking" );
  aoe->add_action( "unstable_affliction,if=dot.unstable_affliction.refreshable" );
  aoe->add_action( "vile_taint,if=soul_shard>1" );
  aoe->add_action( "call_action_list,name=covenant,if=!covenant.necrolord" );
  aoe->add_action( "call_action_list,name=darkglare_prep,if=covenant.venthyr&(cooldown.impending_catastrophe.ready|dot.impending_catastrophe_dot.ticking)&cooldown.summon_darkglare.ready&(dot.phantom_singularity.remains>2|!talent.phantom_singularity)" );
  aoe->add_action( "call_action_list,name=darkglare_prep,if=(covenant.necrolord|covenant.kyrian|covenant.none)&cooldown.summon_darkglare.remains<2&(dot.phantom_singularity.remains>2|!talent.phantom_singularity)" );
  aoe->add_action( "call_action_list,name=darkglare_prep,if=covenant.night_fae&(cooldown.soul_rot.ready|dot.soul_rot.ticking)&cooldown.summon_darkglare.remains<2&(dot.phantom_singularity.remains>2|!talent.phantom_singularity)" );
  aoe->add_action( "dark_soul,if=cooldown.summon_darkglare.remains>time_to_die&(!talent.phantom_singularity|cooldown.phantom_singularity.remains>time_to_die)" );
  aoe->add_action( "dark_soul,if=cooldown.summon_darkglare.remains+cooldown.summon_darkglare.duration<time_to_die" );
  aoe->add_action( "call_action_list,name=item" );
  aoe->add_action( "call_action_list,name=delayed_trinkets" );
  aoe->add_action( "call_action_list,name=damage_trinkets" );
  aoe->add_action( "call_action_list,name=stat_trinkets,if=dot.phantom_singularity.ticking|!talent.phantom_singularity" );
  aoe->add_action( "malefic_rapture,if=dot.vile_taint.ticking" );
  aoe->add_action( "malefic_rapture,if=dot.soul_rot.ticking&!talent.sow_the_seeds" );
  aoe->add_action( "malefic_rapture,if=!talent.vile_taint" );
  aoe->add_action( "malefic_rapture,if=soul_shard>4" );
  aoe->add_action( "siphon_life,cycle_targets=1,if=active_dot.siphon_life<=3,target_if=!dot.siphon_life.ticking" );
  aoe->add_action( "call_action_list,name=covenant" );
  aoe->add_action( "drain_life,if=buff.inevitable_demise.stack>=50|buff.inevitable_demise.up&time_to_die<5|buff.inevitable_demise.stack>=35&dot.soul_rot.ticking" );
  aoe->add_action( "fleshcraft,if=soulbind.volatile_solvent,cancel_if=buff.volatile_solvent_humanoid.up" );
  aoe->add_action( "drain_soul,interrupt=1" );
  aoe->add_action( "shadow_bolt" );

  covenant->add_action( "impending_catastrophe,if=!talent.phantom_singularity&(cooldown.summon_darkglare.remains<10|cooldown.summon_darkglare.remains>50|cooldown.summon_darkglare.remains>25&conduit.corrupting_leer)" );
  covenant->add_action( "impending_catastrophe,if=talent.phantom_singularity&dot.phantom_singularity.ticking" );
  covenant->add_action( "decimating_bolt,if=cooldown.summon_darkglare.remains>5&(debuff.haunt.remains>4|!talent.haunt)" );
  covenant->add_action( "soul_rot,if=!talent.phantom_singularity&(cooldown.summon_darkglare.remains<5|cooldown.summon_darkglare.remains>50|cooldown.summon_darkglare.remains>25&conduit.corrupting_leer)" );
  covenant->add_action( "soul_rot,if=talent.phantom_singularity&dot.phantom_singularity.ticking" );
  covenant->add_action( "scouring_tithe" );

  damage_trinkets->add_action( "use_item,name=soul_igniter" );
  damage_trinkets->add_action( "use_item,name=dreadfire_vessel" );
  damage_trinkets->add_action( "use_item,name=glyph_of_assimilation" );
  damage_trinkets->add_action( "use_item,name=unchained_gladiators_shackles" );
  damage_trinkets->add_action( "use_item,name=ebonsoul_vice" );
  damage_trinkets->add_action( "use_item,name=resonant_reservoir" );
  damage_trinkets->add_action( "use_item,name=architects_ingenuity_core" );
  damage_trinkets->add_action( "use_item,name=grim_eclipse" );
  damage_trinkets->add_action( "use_item,name=toe_knees_promise" );
  damage_trinkets->add_action( "use_item,name=mrrgrias_favor" );
  damage_trinkets->add_action( "use_item,name=cosmic_gladiators_resonator" );

  darkglare_prep->add_action( "vile_taint" );
  darkglare_prep->add_action( "dark_soul" );
  darkglare_prep->add_action( "potion" );
  darkglare_prep->add_action( "fireblood" );
  darkglare_prep->add_action( "blood_fury" );
  darkglare_prep->add_action( "berserking" );
  darkglare_prep->add_action( "call_action_list,name=covenant,if=!covenant.necrolord" );
  darkglare_prep->add_action( "summon_darkglare" );

  delayed_trinkets->add_action( "use_item,name=grim_eclipse,if=(covenant.night_fae&cooldown.soul_rot.remains<6)|(covenant.venthyr&cooldown.impending_catastrophe.remains<6)|(covenant.necrolord|covenant.kyrian|covenant.none)" );
  delayed_trinkets->add_action( "use_item,name=empyreal_ordnance,if=(covenant.night_fae&cooldown.soul_rot.remains<20)|(covenant.venthyr&cooldown.impending_catastrophe.remains<20)|(covenant.necrolord|covenant.kyrian|covenant.none)" );
  delayed_trinkets->add_action( "use_item,name=sunblood_amethyst,if=(covenant.night_fae&cooldown.soul_rot.remains<6)|(covenant.venthyr&cooldown.impending_catastrophe.remains<6)|(covenant.necrolord|covenant.kyrian|covenant.none)" );
  delayed_trinkets->add_action( "use_item,name=soulletting_ruby,if=(covenant.night_fae&cooldown.soul_rot.remains<8)|(covenant.venthyr&cooldown.impending_catastrophe.remains<8)|(covenant.necrolord|covenant.kyrian|covenant.none)" );
  delayed_trinkets->add_action( "use_item,name=shadowed_orb_of_torment,if=(covenant.night_fae&cooldown.soul_rot.remains<4)|(covenant.venthyr&cooldown.impending_catastrophe.remains<4)|(covenant.necrolord|covenant.kyrian|covenant.none)" );

  dot_prep->add_action( "agony,if=dot.agony.remains<8&cooldown.summon_darkglare.remains>dot.agony.remains" );
  dot_prep->add_action( "siphon_life,if=dot.siphon_life.remains<8&cooldown.summon_darkglare.remains>dot.siphon_life.remains" );
  dot_prep->add_action( "unstable_affliction,if=dot.unstable_affliction.remains<8&cooldown.summon_darkglare.remains>dot.unstable_affliction.remains" );
  dot_prep->add_action( "corruption,if=dot.corruption.remains<8&cooldown.summon_darkglare.remains>dot.corruption.remains" );

  item->add_action( "use_items" );

  necro_mw->add_action( "variable,name=dots_ticking,value=dot.corruption.remains>2&dot.agony.remains>2&dot.unstable_affliction.remains>2&(!talent.siphon_life|dot.siphon_life.remains>2)" );
  necro_mw->add_action( "variable,name=trinket_delay,value=cooldown.phantom_singularity.remains,value_else=cooldown.decimating_bolt.remains,op=setif,condition=talent.shadow_embrace,if=covenant.necrolord", "Trinkets align with PS for Shadow Embrace, DB for Haunt." );
  necro_mw->add_action( "malefic_rapture,if=time_to_die<execute_time*soul_shard&dot.unstable_affliction.ticking", "Burn soul shards if the fight will be ending soon." );
  necro_mw->add_action( "haunt,if=dot.haunt.remains<2+execute_time", "Cast haunt to refresh before falloff." );
  necro_mw->add_action( "malefic_rapture,if=time>7&buff.malefic_wrath.remains<gcd.max+execute_time", "High - priority MW refresh if spending one global would cause us to miss the opportunity to refresh MW." );
  necro_mw->add_action( "use_item,name=empyreal_ordnance,if=variable.trinket_delay<20", "Fire delayed trinkets in anticipation of Decimating Bolt." );
  necro_mw->add_action( "use_item,name=sunblood_amethyst,if=variable.trinket_delay<6" );
  necro_mw->add_action( "use_item,name=soulletting_ruby,if=variable.trinket_delay<8" );
  necro_mw->add_action( "use_item,name=shadowed_orb_of_torment,if=variable.trinket_delay<4" );
  necro_mw->add_action( "phantom_singularity,if=!talent.shadow_embrace&variable.dots_ticking", "If the player is using Haunt or Gosac, fire PS on cooldown then follow with DB" );
  necro_mw->add_action( "decimating_bolt,if=!talent.shadow_embrace&cooldown.phantom_singularity.remains>0" );
  necro_mw->add_action( "decimating_bolt,if=talent.shadow_embrace&variable.dots_ticking", "If the player is using SE, fire DB on cooldown then following with PS" );
  necro_mw->add_action( "phantom_singularity,if=talent.shadow_embrace&cooldown.decimating_bolt.remains>0" );
  necro_mw->add_action( "unstable_affliction,if=dot.unstable_affliction.remains<6" );
  necro_mw->add_action( "agony,if=dot.agony.remains<4" );
  necro_mw->add_action( "siphon_life,if=dot.siphon_life.remains<4" );
  necro_mw->add_action( "corruption,if=dot.corruption.remains<4" );
  necro_mw->add_action( "malefic_rapture,if=time>7&buff.malefic_wrath.remains<2*gcd.max+execute_time", "Refresh MW after the opener if darkglare_prep would cause us to miss a MW refresh" );
  necro_mw->add_action( "call_action_list,name=darkglare_prep,if=dot.phantom_singularity.ticking", "Call darkglare_prep if Phantom Singularity is currently ticking" );
  necro_mw->add_action( "call_action_list,name=stat_trinkets,if=dot.phantom_singularity.ticking", "Utilize any other stat trinkets if Phantom Singularity is ticking" );
  necro_mw->add_action( "malefic_rapture,if=time>7&(buff.malefic_wrath.stack<3|buff.malefic_wrath.remains<4.5)", "Stack Malefic Wrath to 3, or refresh when getting low (ideally looking for a calculated number, but 4.5s remaining is the result of testing with T27)" );
  necro_mw->add_action( "malefic_rapture,if=(dot.phantom_singularity.ticking|time_to_die<cooldown.phantom_singularity.remains)&(buff.malefic_wrath.stack<3|soul_shard>1)", "Additional MR spends when extra shards are available and either Phantom Singularity is ticking, or the fight is ending." );
  necro_mw->add_action( "drain_soul,if=dot.phantom_singularity.ticking", "Additional Drain Soul cast when PS is ticking" );
  necro_mw->add_action( "agony,if=refreshable", "Low - priority dot refresh when refreshable." );
  necro_mw->add_action( "unstable_affliction,if=refreshable" );
  necro_mw->add_action( "corruption,if=refreshable" );
  necro_mw->add_action( "siphon_life,if=talent.siphon_life&refreshable" );
  necro_mw->add_action( "fleshcraft,if=soulbind.volatile_solvent,cancel_if=buff.volatile_solvent_humanoid.up", "Fleshcraft to maintain Volatile Solvent." );
  necro_mw->add_action( "haunt,if=dot.haunt.remains<3", "Low-priority haunt refresh." );
  necro_mw->add_action( "drain_soul,if=buff.decimating_bolt.up", "Uninterruptible DS channel if we have the DB buff." );
  necro_mw->add_action( "drain_soul,if=talent.shadow_embrace&debuff.shadow_embrace.remains<3|debuff.shadow_embrace.stack<3,interrupt_if=debuff.shadow_embrace.stack>=3&debuff.shadow_embrace.remains>3" );
  necro_mw->add_action( "drain_soul,interrupt=1" );
  necro_mw->add_action( "shadow_bolt" );

  se->add_action( "haunt" );
  se->add_action( "drain_soul,interrupt_global=1,interrupt_if=debuff.shadow_embrace.stack>=3" );
  se->add_action( "shadow_bolt" );

  stat_trinkets->add_action( "use_item,name=the_first_sigil" );
  stat_trinkets->add_action( "use_item,name=scars_of_fraternal_strife" );
  stat_trinkets->add_action( "use_item,name=inscrutable_quantum_device" );
  stat_trinkets->add_action( "use_item,name=instructors_divine_bell" );
  stat_trinkets->add_action( "use_item,name=overflowing_anima_cage" );
  stat_trinkets->add_action( "use_item,name=darkmoon_deck_putrescence" );
  stat_trinkets->add_action( "use_item,name=macabre_sheet_music" );
  stat_trinkets->add_action( "use_item,name=flame_of_battle" );
  stat_trinkets->add_action( "use_item,name=wakeners_frond" );
  stat_trinkets->add_action( "use_item,name=tablet_of_despair" );
  stat_trinkets->add_action( "use_item,name=sinful_aspirants_badge_of_ferocity" );
  stat_trinkets->add_action( "use_item,name=cosmic_aspirants_badge_of_ferocity" );
  stat_trinkets->add_action( "use_item,name=sinful_gladiators_badge_of_ferocity" );
  stat_trinkets->add_action( "use_item,name=cosmic_gladiators_badge_of_ferocity" );
  stat_trinkets->add_action( "use_item,name=obelisk_of_the_void" );
  stat_trinkets->add_action( "use_item,name=horn_of_valor" );
  stat_trinkets->add_action( "use_item,name=moonlit_prism" );
  stat_trinkets->add_action( "use_item,name=figurehead_of_the_naglfar" );
  stat_trinkets->add_action( "blood_fury" );
  stat_trinkets->add_action( "fireblood" );
  stat_trinkets->add_action( "berserking" );

  trinket_split_check->add_action( "variable,name=special_equipped,value=(((equipped.empyreal_ordnance^equipped.inscrutable_quantum_device)^equipped.soulletting_ruby)^equipped.sunblood_amethyst)" );
  trinket_split_check->add_action( "variable,name=trinket_one,value=(trinket.1.has_proc.any&trinket.1.has_cooldown)" );
  trinket_split_check->add_action( "variable,name=trinket_two,value=(trinket.2.has_proc.any&trinket.2.has_cooldown)" );
  trinket_split_check->add_action( "variable,name=damage_trinket,value=(!(trinket.1.has_proc.any&trinket.1.has_cooldown))|(!(trinket.2.has_proc.any&trinket.2.has_cooldown))|equipped.glyph_of_assimilation" );
  trinket_split_check->add_action( "variable,name=trinket_split,value=(variable.trinket_one&variable.damage_trinket)|(variable.trinket_two&variable.damage_trinket)|(variable.trinket_one^variable.special_equipped)|(variable.trinket_two^variable.special_equipped)" );
}
//affliction_apl_end

//demonology_apl_start
void demonology( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* slow_trinkets = p->get_action_priority_list( "slow_trinkets" );
  action_priority_list_t* covenant_ability = p->get_action_priority_list( "covenant_ability" );
  action_priority_list_t* hp_trinks = p->get_action_priority_list( "hp_trinks" );
  action_priority_list_t* ogcd = p->get_action_priority_list( "ogcd" );
  action_priority_list_t* opener = p->get_action_priority_list( "opener" );
  action_priority_list_t* pure_damage_trinks = p->get_action_priority_list( "pure_damage_trinks" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );
  action_priority_list_t* tyrant_setup = p->get_action_priority_list( "tyrant_setup" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "summon_pet" );
  precombat->add_action( "use_item,name=tome_of_monstrous_constructions" );
  precombat->add_action( "use_item,name=soleahs_secret_technique" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "fleshcraft" );
  precombat->add_action( "variable,name=first_tyrant_time,op=set,value=12" );
  precombat->add_action( "variable,name=first_tyrant_time,op=add,value=action.grimoire_felguard.execute_time,if=talent.grimoire_felguard.enabled" );
  precombat->add_action( "variable,name=first_tyrant_time,op=add,value=action.summon_vilefiend.execute_time,if=talent.summon_vilefiend.enabled" );
  precombat->add_action( "variable,name=first_tyrant_time,op=add,value=gcd.max,if=talent.grimoire_felguard.enabled|talent.summon_vilefiend.enabled" );
  precombat->add_action( "variable,name=first_tyrant_time,op=sub,value=action.summon_demonic_tyrant.execute_time+action.shadow_bolt.execute_time" );
  precombat->add_action( "variable,name=first_tyrant_time,op=min,value=10" );
  precombat->add_action( "variable,name=in_opener,op=set,value=1" );
  precombat->add_action( "variable,name=use_bolt_timings,op=set,value=runeforge.shard_of_annihilation&(runeforge.balespiders_burning_core+talent.sacrificed_souls.enabled+talent.power_siphon.enabled>1)" );
  precombat->add_action( "use_item,name=shadowed_orb_of_torment" );
  precombat->add_action( "demonbolt" );

  default_->add_action( "variable,name=next_tyrant_cd,op=set,value=cooldown.summon_demonic_tyrant.remains_expected,if=!soulbind.field_of_blossoms|cooldown.summon_demonic_tyrant.remains_expected>cooldown.soul_rot.remains_expected" );
  default_->add_action( "variable,name=next_tyrant_cd,op=set,value=cooldown.soul_rot.remains_expected,if=(soulbind.field_of_blossoms|runeforge.decaying_soul_satchel)&cooldown.summon_demonic_tyrant.remains_expected<cooldown.soul_rot.remains_expected" );
  default_->add_action( "variable,name=in_opener,op=set,value=0,if=pet.demonic_tyrant.active" );
  default_->add_action( "variable,name=buff_sync_cd,op=set,value=variable.next_tyrant_cd,if=!variable.use_bolt_timings&!variable.in_opener" );
  default_->add_action( "variable,name=buff_sync_cd,op=set,value=12,if=!variable.use_bolt_timings&variable.in_opener&!pet.dreadstalker.active" );
  default_->add_action( "variable,name=buff_sync_cd,op=set,value=0,if=!variable.use_bolt_timings&variable.in_opener&pet.dreadstalker.active&buff.wild_imps.stack>0&!talent.vilefiend.enabled" );
  default_->add_action( "variable,name=buff_sync_cd,op=set,value=0,if=!variable.use_bolt_timings&variable.in_opener&pet.dreadstalker.active&prev_gcd.1.hand_of_guldan&talent.vilefiend.enabled" );
  default_->add_action( "variable,name=buff_sync_cd,op=set,value=cooldown.decimating_bolt.remains_expected,if=variable.use_bolt_timings" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "call_action_list,name=ogcd,if=(!variable.use_bolt_timings&pet.demonic_tyrant.active)|(variable.use_bolt_timings&buff.shard_of_annihilation.up&(!talent.power_siphon.enabled|buff.power_siphon.up))" );
  default_->add_action( "implosion,if=time_to_die<2*gcd" );
  default_->add_action( "call_action_list,name=opener,if=time<variable.first_tyrant_time" );
  default_->add_action( "interrupt,if=target.debuff.casting.react" );
  default_->add_action( "call_action_list,name=covenant_ability,if=soulbind.grove_invigoration|soulbind.field_of_blossoms|soulbind.combat_meditation|covenant.necrolord" );
  default_->add_action( "potion,if=(!variable.use_bolt_timings&variable.next_tyrant_cd<gcd.max&time>variable.first_tyrant_time|soulbind.refined_palate&variable.next_tyrant_cd<38)|(variable.use_bolt_timings&buff.shard_of_annihilation.up)" );
  default_->add_action( "call_action_list,name=tyrant_setup" );
  default_->add_action( "demonic_strength,if=(!runeforge.wilfreds_sigil_of_superior_summoning&variable.next_tyrant_cd>9)|(pet.demonic_tyrant.active&pet.demonic_tyrant.remains<6*gcd.max)" );
  default_->add_action( "call_dreadstalkers,if=variable.use_bolt_timings&cooldown.summon_demonic_tyrant.remains_expected>22" );
  default_->add_action( "call_dreadstalkers,if=!variable.use_bolt_timings&(variable.next_tyrant_cd>20-5*!runeforge.wilfreds_sigil_of_superior_summoning)" );
  default_->add_action( "bilescourge_bombers,if=buff.tyrant.down&variable.next_tyrant_cd>5" );
  default_->add_action( "implosion,if=active_enemies>1+(1*talent.sacrificed_souls.enabled)&buff.wild_imps.stack>=6&buff.tyrant.down&variable.next_tyrant_cd>5" );
  default_->add_action( "implosion,if=active_enemies>2&buff.wild_imps.stack>=6&buff.tyrant.down&variable.next_tyrant_cd>5&!runeforge.implosive_potential&(!talent.from_the_shadows.enabled|debuff.from_the_shadows.up)" );
  default_->add_action( "implosion,if=active_enemies>2&buff.wild_imps.stack>=6&buff.implosive_potential.remains<2&runeforge.implosive_potential" );
  default_->add_action( "implosion,if=buff.wild_imps.stack>=12&talent.soul_conduit.enabled&talent.from_the_shadows.enabled&runeforge.implosive_potential&buff.tyrant.down&variable.next_tyrant_cd>5" );
  default_->add_action( "grimoire_felguard,if=time_to_die<30" );
  default_->add_action( "summon_vilefiend,if=time_to_die<28" );
  default_->add_action( "summon_demonic_tyrant,if=time_to_die<15" );
  default_->add_action( "hand_of_guldan,if=soul_shard=5" );
  default_->add_action( "shadow_bolt,if=soul_shard<5&runeforge.balespiders_burning_core&buff.balespiders_burning_core.remains<5" );
  default_->add_action( "doom,if=refreshable" );
  default_->add_action( "hand_of_guldan,if=soul_shard>=3&(pet.dreadstalker.active|pet.demonic_tyrant.active)", "If Dreadstalkers are already active, no need to save shards" );
  default_->add_action( "hand_of_guldan,if=soul_shard>=1&buff.nether_portal.up&cooldown.call_dreadstalkers.remains>2*gcd.max" );
  default_->add_action( "hand_of_guldan,if=soul_shard>=1&variable.next_tyrant_cd<gcd.max&time>variable.first_tyrant_time-gcd.max" );
  default_->add_action( "call_action_list,name=covenant_ability,if=!covenant.venthyr" );
  default_->add_action( "soul_strike,if=!talent.sacrificed_souls.enabled", "Without Sacrificed Souls, Soul Strike is stronger than Demonbolt, so it has a higher priority" );
  default_->add_action( "power_siphon,if=!variable.use_bolt_timings&buff.wild_imps.stack>1&buff.demonic_core.stack<3" );
  default_->add_action( "power_siphon,if=variable.use_bolt_timings&buff.shard_of_annihilation.up&buff.shard_of_annihilation.stack<3" );
  default_->add_action( "demonbolt,if=buff.demonic_core.react&soul_shard<4&variable.next_tyrant_cd>20", "Spend Demonic Cores for Soul Shards until Tyrant cooldown is close to ready" );
  default_->add_action( "demonbolt,if=buff.demonic_core.react&soul_shard<4&variable.next_tyrant_cd<12", "During Tyrant setup, spend Demonic Cores for Soul Shards" );
  default_->add_action( "demonbolt,if=buff.demonic_core.react&soul_shard<4&(buff.demonic_core.stack>2|talent.sacrificed_souls.enabled)" );
  default_->add_action( "power_siphon,if=variable.use_bolt_timings&buff.shard_of_annihilation.up" );
  default_->add_action( "demonbolt,if=set_bonus.tier28_2pc&soul_shard<4&((6-soul_shard)*action.shadow_bolt.execute_time>pet.dreadstalker.remains-action.hand_of_guldan.execute_time-action.demonbolt.execute_time)&buff.demonic_core.stack>=1" );
  default_->add_action( "demonbolt,if=buff.demonic_core.react&soul_shard<4&active_enemies>1" );
  default_->add_action( "soul_strike" );
  default_->add_action( "call_action_list,name=covenant_ability" );
  default_->add_action( "hand_of_guldan,if=soul_shard>=3&variable.next_tyrant_cd>25&(talent.demonic_calling.enabled|cooldown.call_dreadstalkers.remains>((5-soul_shard)*action.shadow_bolt.execute_time)+action.hand_of_guldan.execute_time)", "If you can get back to 5 Soul Shards before Dreadstalkers cooldown is ready, it's okay to spend them now" );
  default_->add_action( "doom,cycle_targets=1,if=refreshable&time>variable.first_tyrant_time" );
  default_->add_action( "shadow_bolt" );

  slow_trinkets->add_action( "use_item,name=soulletting_ruby,target_if=min:target.health.pct,if=variable.buff_sync_cd<target.distance%5-(2*gcd.max*variable.use_bolt_timings)" );
  slow_trinkets->add_action( "use_item,name=sunblood_amethyst,if=variable.buff_sync_cd<target.distance%5+(2*variable.use_bolt_timings)" );
  slow_trinkets->add_action( "use_item,name=empyreal_ordnance,if=variable.buff_sync_cd<(target.distance%5)+12+(2*variable.use_bolt_timings)" );

  covenant_ability->add_action( "soul_rot,if=(soulbind.field_of_blossoms|runeforge.decaying_soul_satchel)&pet.demonic_tyrant.active" );
  covenant_ability->add_action( "soul_rot,if=soulbind.grove_invigoration&!runeforge.decaying_soul_satchel&(variable.next_tyrant_cd<20|variable.next_tyrant_cd>30)" );
  covenant_ability->add_action( "soul_rot,if=soulbind.wild_hunt_tactics&!runeforge.decaying_soul_satchel&!pet.demonic_tyrant.active&variable.next_tyrant_cd>18" );
  covenant_ability->add_action( "decimating_bolt,target_if=min:target.health.pct,if=!variable.use_bolt_timings&soulbind.lead_by_example&(pet.demonic_tyrant.active&soul_shard<2|!pet.demonic_tyrant.active&variable.next_tyrant_cd>40)" );
  covenant_ability->add_action( "decimating_bolt,target_if=min:target.health.pct,if=!variable.use_bolt_timings&soulbind.kevins_oozeling&(pet.demonic_tyrant.active|!pet.demonic_tyrant.active&variable.next_tyrant_cd>40)" );
  covenant_ability->add_action( "decimating_bolt,target_if=min:target.health.pct,if=!variable.use_bolt_timings&(soulbind.forgeborne_reveries|(soulbind.volatile_solvent&!soulbind.kevins_oozeling))&!pet.demonic_tyrant.active" );
  covenant_ability->add_action( "decimating_bolt,target_if=min:target.health.pct,if=variable.use_bolt_timings&(!talent.power_siphon|cooldown.power_siphon.remains<action.decimating_bolt.execute_time)&!cooldown.summon_demonic_tyrant.up&(pet.demonic_tyrant.remains<8|cooldown.summon_demonic_tyrant.remains_expected<30)" );
  covenant_ability->add_action( "fleshcraft,if=soulbind.volatile_solvent&buff.volatile_solvent_humanoid.down,cancel_if=buff.volatile_solvent_humanoid.up" );
  covenant_ability->add_action( "scouring_tithe,if=soulbind.combat_meditation&pet.demonic_tyrant.active" );
  covenant_ability->add_action( "scouring_tithe,if=!soulbind.combat_meditation" );
  covenant_ability->add_action( "impending_catastrophe,if=pet.demonic_tyrant.active&soul_shard=0" );

  hp_trinks->add_action( "use_item,name=sinful_gladiators_emblem" );
  hp_trinks->add_action( "use_item,name=sinful_aspirants_emblem" );

  ogcd->add_action( "berserking" );
  ogcd->add_action( "blood_fury" );
  ogcd->add_action( "fireblood" );
  ogcd->add_action( "use_items" );

  opener->add_action( "soul_rot,if=soulbind.grove_invigoration,if=!runeforge.decaying_soul_satchel" );
  opener->add_action( "nether_portal" );
  opener->add_action( "grimoire_felguard" );
  opener->add_action( "summon_vilefiend" );
  opener->add_action( "shadow_bolt,if=soul_shard<5&cooldown.call_dreadstalkers.up" );
  opener->add_action( "shadow_bolt,if=variable.use_bolt_timings&soul_shard<5&buff.balespiders_burning_core.stack<4" );
  opener->add_action( "call_dreadstalkers" );

  pure_damage_trinks->add_action( "use_item,name=resonant_reservoir" );
  pure_damage_trinks->add_action( "use_item,name=architects_ingenuity_core" );
  pure_damage_trinks->add_action( "use_item,name=cosmic_gladiators_resonator" );
  pure_damage_trinks->add_action( "use_item,name=dreadfire_vessel" );
  pure_damage_trinks->add_action( "use_item,name=soul_igniter" );
  pure_damage_trinks->add_action( "use_item,name=glyph_of_assimilation,if=active_enemies=1" );
  pure_damage_trinks->add_action( "use_item,name=darkmoon_deck_putrescence" );
  pure_damage_trinks->add_action( "use_item,name=ebonsoul_vise" );
  pure_damage_trinks->add_action( "use_item,name=unchained_gladiators_shackles" );
  pure_damage_trinks->add_action( "use_item,slot=trinket1,if=!trinket.1.has_use_buff" );
  pure_damage_trinks->add_action( "use_item,slot=trinket2,if=!trinket.2.has_use_buff" );

  trinkets->add_action( "variable,name=use_buff_trinkets,value=(!variable.use_bolt_timings&pet.demonic_tyrant.active)|(variable.use_bolt_timings&buff.shard_of_annihilation.up)" );
  trinkets->add_action( "use_item,name=scars_of_fraternal_strife,if=!buff.scars_of_fraternal_strife_4.up" );
  trinkets->add_action( "use_item,name=scars_of_fraternal_strife,if=buff.scars_of_fraternal_strife_4.up&pet.demonic_tyrant.active" );
  trinkets->add_action( "use_item,name=shadowed_orb_of_torment,if=variable.buff_sync_cd<22" );
  trinkets->add_action( "use_item,name=moonlit_prism,if=variable.use_bolt_timings&pet.demonic_tyrant.active" );
  trinkets->add_action( "use_item,name=grim_eclipse,if=variable.buff_sync_cd<7" );
  trinkets->add_action( "call_action_list,name=hp_trinks,if=talent.demonic_consumption.enabled&variable.next_tyrant_cd<20" );
  trinkets->add_action( "call_action_list,name=slow_trinkets", "Effects that travel slowly from the target require additional, separate handling" );
  trinkets->add_action( "use_item,name=overflowing_anima_cage,if=variable.use_buff_trinkets" );
  trinkets->add_action( "use_item,slot=trinket1,if=trinket.1.has_use_buff&variable.use_buff_trinkets" );
  trinkets->add_action( "use_item,slot=trinket2,if=trinket.2.has_use_buff&variable.use_buff_trinkets" );
  trinkets->add_action( "use_item,name=neural_synapse_enhancer,if=variable.buff_sync_cd>45|variable.use_buff_trinkets" );
  trinkets->add_action( "call_action_list,name=pure_damage_trinks,if=time>variable.first_tyrant_time&variable.buff_sync_cd>20" );

  tyrant_setup->add_action( "nether_portal,if=variable.next_tyrant_cd<15" );
  tyrant_setup->add_action( "grimoire_felguard,if=variable.next_tyrant_cd<17-(action.summon_demonic_tyrant.execute_time+action.shadow_bolt.execute_time)&(cooldown.call_dreadstalkers.remains<17-(action.summon_demonic_tyrant.execute_time+action.summon_vilefiend.execute_time+action.shadow_bolt.execute_time)|pet.dreadstalker.remains>variable.next_tyrant_cd+action.summon_demonic_tyrant.execute_time)" );
  tyrant_setup->add_action( "summon_vilefiend,if=(variable.next_tyrant_cd<15-(action.summon_demonic_tyrant.execute_time)&(cooldown.call_dreadstalkers.remains<15-(action.summon_demonic_tyrant.execute_time+action.summon_vilefiend.execute_time)|pet.dreadstalker.remains>variable.next_tyrant_cd+action.summon_demonic_tyrant.execute_time))|(!runeforge.wilfreds_sigil_of_superior_summoning&variable.next_tyrant_cd>40)" );
  tyrant_setup->add_action( "call_dreadstalkers,if=variable.next_tyrant_cd<12-(action.summon_demonic_tyrant.execute_time+action.shadow_bolt.execute_time)" );
  tyrant_setup->add_action( "summon_demonic_tyrant,if=time>variable.first_tyrant_time&(pet.dreadstalker.active&pet.dreadstalker.remains>action.summon_demonic_tyrant.execute_time)&(!talent.summon_vilefiend.enabled|pet.vilefiend.active)&(soul_shard=0|(pet.dreadstalker.active&pet.dreadstalker.remains<action.summon_demonic_tyrant.execute_time+action.shadow_bolt.execute_time)|(pet.vilefiend.active&pet.vilefiend.remains<action.summon_demonic_tyrant.execute_time+action.shadow_bolt.execute_time)|(buff.grimoire_felguard.up&buff.grimoire_felguard.remains<action.summon_demonic_tyrant.execute_time+action.shadow_bolt.execute_time))" );
}
//demonology_apl_end

//destruction_apl_start
void destruction( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* havoc = p->get_action_priority_list( "havoc" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "summon_pet" );
  precombat->add_action( "use_item,name=tome_of_monstrous_constructions" );
  precombat->add_action( "use_item,name=soleahs_secret_technique" );
  precombat->add_action( "grimoire_of_sacrifice,if=talent.grimoire_of_sacrifice.enabled" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "fleshcraft" );
  precombat->add_action( "use_item,name=shadowed_orb_of_torment" );
  precombat->add_action( "soul_fire" );
  precombat->add_action( "incinerate" );

  default_->add_action( "call_action_list,name=havoc,if=havoc_active&active_enemies>1&active_enemies<5-talent.inferno.enabled+(talent.inferno.enabled&talent.internal_combustion.enabled)" );
  default_->add_action( "fleshcraft,if=soulbind.volatile_solvent,cancel_if=buff.volatile_solvent_humanoid.up" );
  default_->add_action( "conflagrate,if=talent.roaring_blaze.enabled&debuff.roaring_blaze.remains<1.5" );
  default_->add_action( "cataclysm" );
  default_->add_action( "call_action_list,name=aoe,if=active_enemies>2-set_bonus.tier28_4pc" );
  default_->add_action( "soul_fire,cycle_targets=1,if=refreshable&soul_shard<=4&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>remains)" );
  default_->add_action( "immolate,cycle_targets=1,if=remains<3&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>remains)" );
  default_->add_action( "immolate,if=talent.internal_combustion.enabled&action.chaos_bolt.in_flight&remains<duration*0.5" );
  default_->add_action( "chaos_bolt,if=(pet.infernal.active|pet.blasphemy.active)&soul_shard>=4" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "channel_demonfire" );
  default_->add_action( "scouring_tithe" );
  default_->add_action( "decimating_bolt" );
  default_->add_action( "havoc,cycle_targets=1,if=!(target=self.target)&(dot.immolate.remains>dot.immolate.duration*0.5|!talent.internal_combustion.enabled)" );
  default_->add_action( "impending_catastrophe" );
  default_->add_action( "soul_rot" );
  default_->add_action( "havoc,if=runeforge.odr_shawl_of_the_ymirjar.equipped" );
  default_->add_action( "variable,name=pool_soul_shards,value=active_enemies>1&cooldown.havoc.remains<=10|buff.ritual_of_ruin.up&talent.rain_of_chaos" );
  default_->add_action( "conflagrate,if=buff.backdraft.down&soul_shard>=1.5-0.3*talent.flashover.enabled&!variable.pool_soul_shards" );
  default_->add_action( "chaos_bolt,if=pet.infernal.active|buff.rain_of_chaos.remains>cast_time" );
  default_->add_action( "chaos_bolt,if=buff.backdraft.up&!variable.pool_soul_shards" );
  default_->add_action( "chaos_bolt,if=talent.eradication&!variable.pool_soul_shards&debuff.eradication.remains<cast_time" );
  default_->add_action( "shadowburn,if=!variable.pool_soul_shards|soul_shard>=4.5" );
  default_->add_action( "chaos_bolt,if=soul_shard>3.5" );
  default_->add_action( "chaos_bolt,if=time_to_die<5&time_to_die>cast_time+travel_time" );
  default_->add_action( "conflagrate,if=charges>1|time_to_die<gcd" );
  default_->add_action( "incinerate" );

  aoe->add_action( "rain_of_fire,if=pet.infernal.active&(!cooldown.havoc.ready|active_enemies>3)" );
  aoe->add_action( "rain_of_fire,if=set_bonus.tier28_4pc" );
  aoe->add_action( "soul_rot" );
  aoe->add_action( "impending_catastrophe" );
  aoe->add_action( "channel_demonfire,if=dot.immolate.remains>cast_time" );
  aoe->add_action( "immolate,cycle_targets=1,if=active_enemies<5&remains<5&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>remains)" );
  aoe->add_action( "call_action_list,name=cds" );
  aoe->add_action( "havoc,cycle_targets=1,if=!(target=self.target)&active_enemies<4" );
  aoe->add_action( "rain_of_fire" );
  aoe->add_action( "havoc,cycle_targets=1,if=!(self.target=target)" );
  aoe->add_action( "decimating_bolt" );
  aoe->add_action( "incinerate,if=talent.fire_and_brimstone.enabled&buff.backdraft.up&soul_shard<5-0.2*active_enemies" );
  aoe->add_action( "soul_fire" );
  aoe->add_action( "conflagrate,if=buff.backdraft.down" );
  aoe->add_action( "shadowburn,if=target.health.pct<20" );
  aoe->add_action( "immolate,if=refreshable" );
  aoe->add_action( "scouring_tithe" );
  aoe->add_action( "incinerate" );

  cds->add_action( "use_item,name=shadowed_orb_of_torment,if=cooldown.summon_infernal.remains<3|time_to_die<42" );
  cds->add_action( "summon_infernal" );
  cds->add_action( "dark_soul_instability,if=pet.infernal.active|cooldown.summon_infernal.remains_expected>time_to_die" );
  cds->add_action( "potion,if=pet.infernal.active" );
  cds->add_action( "berserking,if=pet.infernal.active" );
  cds->add_action( "blood_fury,if=pet.infernal.active" );
  cds->add_action( "fireblood,if=pet.infernal.active" );
  cds->add_action( "use_item,name=scars_of_fraternal_strife,if=!buff.scars_of_fraternal_strife_4.up" );
  cds->add_action( "use_item,name=scars_of_fraternal_strife,if=buff.scars_of_fraternal_strife_4.up&pet.infernal.active" );
  cds->add_action( "use_items,if=pet.infernal.active|time_to_die<21" );

  havoc->add_action( "conflagrate,if=buff.backdraft.down&soul_shard>=1&soul_shard<=4" );
  havoc->add_action( "soul_fire,if=cast_time<havoc_remains" );
  havoc->add_action( "decimating_bolt,if=cast_time<havoc_remains&soulbind.lead_by_example.enabled" );
  havoc->add_action( "scouring_tithe,if=cast_time<havoc_remains" );
  havoc->add_action( "immolate,if=talent.internal_combustion.enabled&remains<duration*0.5|!talent.internal_combustion.enabled&refreshable" );
  havoc->add_action( "chaos_bolt,if=cast_time<havoc_remains&!(set_bonus.tier28_4pc&active_enemies>1&talent.inferno.enabled)" );
  havoc->add_action( "rain_of_fire,if=set_bonus.tier28_4pc&active_enemies>1&talent.inferno.enabled" );
  havoc->add_action( "shadowburn" );
  havoc->add_action( "incinerate,if=cast_time<havoc_remains" );
}
//destruction_apl_end

} // namespace warlock_apl