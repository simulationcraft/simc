// ==========================================================================
// Priest APL File
// Contact: https://github.com/orgs/simulationcraft/teams/priest/members
// Wiki: https://github.com/simulationcraft/simc/wiki/Priests
// ==========================================================================

#include "class_modules/apl/apl_priest.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace priest_apl
{
std::string potion( const player_t* p )
{
  std::string lvl50_potion = ( p->specialization() == PRIEST_SHADOW ) ? "unbridled_fury" : "battle_potion_of_intellect";

  return ( p->true_level > 50 ) ? "potion_of_spectral_intellect" : lvl50_potion;
}

std::string flask( const player_t* p )
{
  return ( p->true_level > 50 ) ? "spectral_flask_of_power" : "greater_flask_of_endless_fathoms";
}

std::string food( const player_t* p )
{
  return ( p->true_level > 50 ) ? "feast_of_gluttonous_hedonism" : "baked_port_tato";
}

std::string rune( const player_t* p )
{
  return ( p->true_level > 50 ) ? "veiled_augment_rune" : "battle_scarred";
}

std::string temporary_enchant( const player_t* p )
{
  return ( p->true_level >= 60 ) ? "main_hand:shadowcore_oil" : "disabled";
}

void shadow( player_t* p )
{
  action_priority_list_t* precombat    = p->get_action_priority_list( "precombat" );
  action_priority_list_t* default_list = p->get_action_priority_list( "default" );
  action_priority_list_t* main         = p->get_action_priority_list( "main" );
  action_priority_list_t* cds          = p->get_action_priority_list( "cds" );
  action_priority_list_t* trinkets     = p->get_action_priority_list( "trinkets" );

  // Snapshot stats
  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "fleshcraft,if=soulbind.pustule_eruption|soulbind.volatile_solvent" );
  precombat->add_action( "shadowform,if=!buff.shadowform.up" );
  precombat->add_action( "arcane_torrent" );
  precombat->add_action( "use_item,name=shadowed_orb_of_torment" );
  precombat->add_action( "variable,name=mind_sear_cutoff,op=set,value=2" );
  precombat->add_action( "shadow_crash,if=talent.shadow_crash.enabled" );
  precombat->add_action( "mind_blast,if=talent.damnation.enabled&!talent.shadow_crash.enabled" );
  precombat->add_action( "vampiric_touch,if=!talent.damnation.enabled&!talent.shadow_crash.enabled" );

  // Professions
  for ( const auto& profession_action : p->get_profession_actions() )
  {
    default_list->add_action( profession_action );
  }

  // Potions
  default_list->add_action( "potion,if=buff.voidform.up|buff.power_infusion.up|buff.dark_ascension.up" );
  default_list->add_action(
      "variable,name=dots_up,op=set,value="
      "dot.shadow_word_pain.ticking&dot.vampiric_touch.ticking" );
  default_list->add_action(
      "variable,name=all_dots_up,op=set,value="
      "dot.shadow_word_pain.ticking&dot.vampiric_touch.ticking&dot.devouring_plague.ticking" );
  default_list->add_action( "variable,name=five_minutes_viable,op=set,value=(fight_remains+time)>=60*5+20" );
  default_list->add_action(
      "variable,name=four_minutes_viable,op=set,value=!variable.five_minutes_viable&(fight_remains+time)>=60*4+20" );
  default_list->add_action(
      "variable,name=cd_management,op=set,value=(variable.five_minutes_viable&time<=200|!variable.four_minutes_viable&!"
      "variable.five_minutes_viable|pet.fiend.active&(variable.four_minutes_viable&cooldown.power_infusion.remains<="
      "gcd.max*3|variable.five_minutes_viable&time>300)|fight_remains<=25),default=0" );
  default_list->add_action( "variable,name=max_vts,op=set,default=1,value=spell_targets.vampiric_touch" );
  default_list->add_action(
      "variable,name=max_vts,op=set,value=(spell_targets.mind_sear<=5)*spell_targets.mind_sear,if=buff.voidform.up" );
  default_list->add_action(
      "variable,name=do_zoomer_opener,op=set,value=((time+fight_remains)%%60)>=13&((time+fight_remains)%%60<=25)" );
  default_list->add_action( "variable,name=is_vt_possible,op=set,value=0,default=1" );
  default_list->add_action(
      "variable,name=is_vt_possible,op=set,value=1,target_if=max:(target.time_to_die*dot.vampiric_touch.refreshable),"
      "if=target.time_to_die>=18" );
  default_list->add_action(
      "variable,name=vts_applied,op=set,value=active_dot.vampiric_touch>=variable.max_vts|!variable.is_vt_possible" );
  default_list->add_action(
      "variable,name=sfp,op=set,value=runeforge.shadowflame_prism.equipped|talent.inescapable_torment" );
  default_list->add_action(
      "variable,name=pool_for_cds,op=set,value=(cooldown.void_eruption.up&talent.void_eruption|cooldown.dark_ascension."
      "up&talent.dark_ascension)&variable.cd_management&(cooldown.void_eruption.duration>1|!buff.voidform.up)" );
  // Racials
  default_list->add_action( "fireblood,if=buff.voidform.up" );
  default_list->add_action( "berserking,if=buff.voidform.up" );
  default_list->add_action( "variable,name=pool_amount,op=set,value=85+16*(action.void_eruption.cost>=80)" );
  default_list->add_action(
      "lights_judgment,if=spell_targets.lights_judgment>=2|(!raid_event.adds.exists|raid_event.adds.in>75)" );
  default_list->add_action( "ancestral_call,if=buff.voidform.up" );
  default_list->add_run_action_list( main );

  // Trinkets
  trinkets->add_action(
      "use_item,name=scars_of_fraternal_strife,if=(!buff.scars_of_fraternal_strife_4.up&time>1)|(buff.voidform.up|buff."
      "power_infusion.up|cooldown.void_eruption.remains>10)|covenant.night_fae",
      "Use trinket after pull starts and then on CD after that until you get 4th stack. Try to delay getting 5th stack "
      "for cooldowns. Use before other on use to not trigger ICD between trinkets (bug?)." );
  trinkets->add_action(
      "use_item,name=empyreal_ordnance,if=cooldown.void_eruption.remains<=12|cooldown.void_eruption.remains>27",
      "Use on CD ASAP to get DoT ticking and expire to line up better with Voidform" );
  trinkets->add_action(
      "use_item,name=inscrutable_quantum_device,if=buff.voidform.up&buff.power_infusion.up|fight_remains<=20|buff."
      "power_infusion.up&cooldown.void_eruption.remains+15>fight_remains|buff.voidform.up&cooldown.power_infusion."
      "remains+15>fight_remains|(cooldown.power_infusion.remains>=10&cooldown.void_eruption.remains>=10)&fight_remains>"
      "=190",
      "Try to Sync IQD with Double Stacked CDs if possible. On longer fights with more IQD uses attempt to sync with "
      "any cd or just use it." );
  trinkets->add_action( "use_item,name=macabre_sheet_music,if=cooldown.void_eruption.remains>10",
                        "Sync Sheet Music with Voidform" );
  trinkets->add_action(
      "use_item,name=soulletting_ruby,if=buff.power_infusion.up|!priest.self_power_infusion|"
      "equipped.shadowed_orb_of_torment,target_if=min:target.health.pct",
      "Sync Ruby with Power Infusion usage, make sure to snipe the lowest HP target. When used with Shadowed Orb of "
      "Torment, just use on CD as much as possible." );
  trinkets->add_action(
      "use_item,name=the_first_sigil,if=buff.voidform.up|buff.power_infusion.up|!priest.self_power_infusion|cooldown."
      "void_eruption.remains>10|(equipped.soulletting_ruby&!trinket.soulletting_ruby.cooldown.up)|fight_remains<20",
      "First Sigil small optimization with Soulletting Ruby" );
  trinkets->add_action(
      "use_item,name=sinful_gladiators_badge_of_ferocity,if=cooldown.void_eruption.remains>=10",
      "Use Badge inside of VF for the first use or on CD after the first use. Short circuit if void eruption cooldown "
      "is 10s or more away." );
  trinkets->add_action(
      "use_item,name=shadowed_orb_of_torment,if=cooldown.power_infusion.remains<=10&cooldown.void_eruption.remains<=10&"
      "(covenant.necrolord|covenant.kyrian)|(covenant.venthyr|covenant.night_fae)&(!buff.voidform.up|prev_gcd.1.void_"
      "bolt)|fight_remains<=40",
      "Use Shadowed Orb of Torment when not in Voidform, or in between Void Bolt casts in Voidform. As Kyrian or "
      "Necrolord line it up with stacked cooldowns." );
  trinkets->add_action( "use_item,name=architects_ingenuity_core", "Use this on CD for max CDR" );
  trinkets->add_action(
      "use_items,if=buff.voidform.up|buff.power_infusion.up|buff.dark_ascension.up|cooldown.void_eruption.remains>10",
      "Default fallback for usable items: Use on cooldown in order by trinket slot." );

  // CDs
  cds->add_action(
      "power_infusion,if=(buff.voidform.up|buff.dark_ascension.up)&(!variable.five_minutes_viable|time>300|time<235)" );
  cds->add_action(
      "void_eruption,if=variable.cd_management&(insanity<=variable.pool_amount|buff.mind_devourer.up)&!cooldown.fiend."
      "up&(pet.fiend.active|cooldown."
      "fiend.remains>=gcd.max*5)&(cooldown.mind_blast.charges=0|time>15)&pet.fiend.active" );
  cds->add_action( "dark_ascension,if=pet.fiend.active" );
  cds->add_call_action_list( trinkets );
  cds->add_action(
      "void_torrent,if=(variable.all_dots_up|!talent.damnation&(variable.dots_up|action.shadow_crash.in_flight))&time<="
      "5&!variable.do_zoomer_opener" );
  cds->add_action( "devouring_plague,if=variable.dots_up&prev_gcd.1.void_torrent&time<=10" );
  cds->add_action(
      "mindbender,if=dot.shadow_word_pain.ticking&variable.vts_applied&(!talent.void_eruption.enabled|talent.void_"
      "eruption&cooldown.void_"
      "eruption.remains>=30|talent.dark_ascension&cooldown.dark_ascension.remains>=30|(talent.dark_ascension&cooldown."
      "dark_ascension.remains<=3*gcd.max|talent.void_eruption&cooldown.void_eruption.remains<=3*gcd.max)&(!variable."
      "five_minutes_viable|time<=230|time>=300-3*gcd.max|talent.mindbender&time<=270))" );
  cds->add_action( "desperate_prayer,if=health.pct<=75" );

  // Main APL, should cover all ranges of targets and scenarios
  main->add_call_action_list( cds );
  main->add_action(
      "shadow_word_death,if=pet.fiend.active&variable.sfp&(pet.fiend.remains<=gcd|buff.deathspeaker.up)&spell_targets."
      "mind_sear<=7" );
  main->add_action(
      "mind_blast,if=(cooldown.mind_blast.full_recharge_time<=gcd.max&buff.voidform.up|pet.fiend.remains<=cast_time+"
      "gcd.max)&pet.fiend.active&variable.sfp&pet.fiend.remains>cast_time&spell_targets.mind_sear<=7" );
  main->add_action(
      "damnation,target_if=dot.vampiric_touch.refreshable&variable.is_vt_possible|dot.shadow_word_pain.refreshable" );
  main->add_action(
      "mindgames,if=spell_targets.mind_sear<5,target_if=insanity<90&((variable.all_dots_up&(!cooldown.void_eruption.up|"
      "!variable.cd_management))|buff.voidform.up)" );
  main->add_action(
      "shadow_word_death,if=!buff.death_and_madness_reset.up,target_if=(pet.fiend.active&variable.sfp&spell_targets."
      "mind_sear<=7)" );
  main->add_action(
      "devouring_plague,if=(refreshable|insanity>75|talent.void_torrent.enabled&cooldown.void_torrent.remains<=3*gcd&!"
      "buff.voidform.up|buff.voidform.up&(cooldown.mind_blast.charges_fractional<2|buff.mind_devourer.up))&(!variable."
      "pool_for_cds|insanity>=variable.pool_amount)" );
  main->add_action(
      "void_bolt,if=variable.dots_up&(!variable.sfp|!cooldown.fiend.up&variable.sfp|variable.vts_applied)" );
  main->add_action(
      "mind_blast,if=variable.vts_applied&(buff.voidform.up|!cooldown.void_eruption.up|!variable.pool_for_cds)&(spell_"
      "targets.mind_sear=1|active_dot.vampiric_touch>=5)" );
  main->add_action( "shadow_crash,if=raid_event.adds.in>10" );
  main->add_action( "dark_void,if=raid_event.adds.in>20" );
  main->add_action( "divine_star,if=spell_targets.divine_star>1|variable.all_dots_up" );
  main->add_action( "halo,if=raid_event.adds.in>20&(spell_targets.halo>1|variable.all_dots_up)" );
  main->add_action(
      "void_torrent,if=insanity<=35&(buff.voidform.down|buff.voidform.remains<cooldown.void_bolt.remains|prev_gcd.1."
      "void_bolt),target_if=variable.dots_up" );
  main->add_action(
      "shadow_word_death,target_if=(target.health.pct<20&spell_targets.mind_sear<4)|(pet.fiend.active&variable.sfp&"
      "spell_targets.mind_sear<=7)|buff.deathspeaker.up&(cooldown.fiend.remains+gcd.max)>buff.deathspeaker.remains&("
      "cooldown.void_eruption.remains+gcd.max*4)>buff.deathspeaker.remains" );
  main->add_action(
      "void_bolt,if=variable.dots_up&(!variable.sfp|!cooldown.fiend.up&variable.sfp|variable.vts_applied)" );
  main->add_action(
      "mind_blast,if=raid_event.movement.in>cast_time+0.5&(!variable.sfp|!cooldown.fiend.up&variable.sfp|variable.vts_"
      "applied)" );
  main->add_action( "void_bolt,if=variable.dots_up" );
  main->add_action(
      "vampiric_touch,target_if=(refreshable&target.time_to_die>=18&(dot.vampiric_touch.ticking|!variable.vts_applied)&"
      "variable.max_vts>0|(talent.misery.enabled&dot.shadow_word_pain.refreshable)|buff.unfurling_darkness.up)&("
      "cooldown.shadow_crash.remains<=dot.vampiric_touch.remains)" );
  main->add_action( "shadow_word_pain,target_if=refreshable&target.time_to_die>4&!talent.misery.enabled" );
  main->add_action(
      "mind_spike,if=!talent.mind_flay_insanity|!buff.mind_flay_insanity.up&buff.mind_melt.stack<2|buff.surge_of_"
      "darkness.up|buff.dark_ascension.up" );
  main->add_action(
      "mind_sear,target_if=spell_targets.mind_sear>variable.mind_sear_cutoff,chain=1,interrupt_immediate=1,interrupt_"
      "if=ticks>=2" );
  main->add_action( "mind_flay,chain=1,interrupt_immediate=1,interrupt_if=ticks>=2" );
  main->add_action( "shadow_word_death" );
  main->add_action( "shadow_word_pain" );
}

void discipline( player_t* p )
{
  action_priority_list_t* def     = p->get_action_priority_list( "default" );
  action_priority_list_t* racials = p->get_action_priority_list( "racials" );

  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "smite" );

  // On-Use Items
  def->add_action( "use_items", "Default fallback for usable items: Use on cooldown in order by trinket slot." );

  // Potions
  def->add_action( "potion,if=buff.bloodlust.react|buff.power_infusion.up|target.time_to_die<=40",
                   "Sync potion usage with Bloodlust or Power Infusion." );

  // Racials
  racials->add_action( "arcane_torrent,if=mana.pct<=95" );

  if ( p->race != RACE_BLOOD_ELF )
  {
    for ( const auto& racial_action : p->get_racial_actions() )
    {
      racials->add_action( racial_action );
    }
  }

  def->add_call_action_list( racials );
  def->add_action( "power_infusion",
                   "Use Power Infusion before Shadow Covenant to make sure we don't lock out our CD." );
  def->add_action( "divine_star" );
  def->add_action( "halo" );
  def->add_action( "penance" );
  def->add_action( "power_word_solace" );
  def->add_action( "shadow_covenant" );
  def->add_action( "schism" );
  def->add_action( "mindgames" );
  def->add_action( "fae_guardians" );
  def->add_action( "unholy_nova" );
  def->add_action( "mindbender" );
  def->add_action( "spirit_shell" );
  def->add_action( "purge_the_wicked,if=!ticking" );
  def->add_action( "shadow_word_pain,if=!ticking&!talent.purge_the_wicked.enabled" );
  def->add_action( "shadow_word_death" );
  def->add_action( "mind_blast" );
  def->add_action( "purge_the_wicked,if=refreshable" );
  def->add_action( "shadow_word_pain,if=refreshable&!talent.purge_the_wicked.enabled" );
  def->add_action( "smite,if=spell_targets.holy_nova<3", "Use Smite on up to 2 targets." );
  def->add_action( "holy_nova,if=spell_targets.holy_nova>=3" );
  def->add_action( "shadow_word_pain" );
}

void holy( player_t* p )
{
  action_priority_list_t* precombat    = p->get_action_priority_list( "precombat" );
  action_priority_list_t* default_list = p->get_action_priority_list( "default" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "smite" );

  // On-Use Items
  default_list->add_action( "use_items" );

  // Professions
  for ( const auto& profession_action : p->get_profession_actions() )
  {
    default_list->add_action( profession_action );
  }

  // Potions
  default_list->add_action(
      "potion,if=buff.bloodlust.react|(raid_event.adds.up&(raid_event.adds.remains>20|raid_event.adds.duration<20))|"
      "target.time_to_die<=30" );

  // Default APL
  default_list->add_action(
      p, "Holy Fire",
      "if=dot.holy_fire.ticking&(dot.holy_fire.remains<=gcd|dot.holy_fire.stack<2)&spell_targets.holy_nova<7" );
  default_list->add_action( p, "Holy Word: Chastise", "if=spell_targets.holy_nova<5" );
  default_list->add_action(
      p, "Holy Fire",
      "if=dot.holy_fire.ticking&(dot.holy_fire.refreshable|dot.holy_fire.stack<2)&spell_targets.holy_nova<7" );
  default_list->add_action(
      "berserking,if=raid_event.adds.in>30|raid_event.adds.remains>8|raid_event.adds.duration<8" );
  default_list->add_action( "fireblood,if=raid_event.adds.in>20|raid_event.adds.remains>6|raid_event.adds.duration<6" );
  default_list->add_action(
      "ancestral_call,if=raid_event.adds.in>20|raid_event.adds.remains>10|raid_event.adds.duration<10" );
  default_list->add_talent(
      p, "Divine Star",
      "if=(raid_event.adds.in>5|raid_event.adds.remains>2|raid_event.adds.duration<2)&spell_targets.divine_star>1" );
  default_list->add_talent(
      p, "Halo",
      "if=(raid_event.adds.in>14|raid_event.adds.remains>2|raid_event.adds.duration<2)&spell_targets.halo>0" );
  default_list->add_action(
      "lights_judgment,if=raid_event.adds.in>50|raid_event.adds.remains>4|raid_event.adds.duration<4" );
  default_list->add_action(
      "arcane_pulse,if=(raid_event.adds.in>40|raid_event.adds.remains>2|raid_event.adds.duration<2)&spell_targets."
      "arcane_pulse>2" );
  default_list->add_action( p, "Holy Fire", "if=!dot.holy_fire.ticking&spell_targets.holy_nova<7" );
  default_list->add_action( p, "Holy Nova", "if=spell_targets.holy_nova>3" );
  default_list->add_talent(
      p, "Apotheosis", "if=active_enemies<5&(raid_event.adds.in>15|raid_event.adds.in>raid_event.adds.cooldown-5)" );
  default_list->add_action( p, "Smite" );
  default_list->add_action( p, "Holy Fire" );
  default_list->add_talent(
      p, "Divine Star",
      "if=(raid_event.adds.in>5|raid_event.adds.remains>2|raid_event.adds.duration<2)&spell_targets.divine_star>0" );
  default_list->add_action( p, "Holy Nova", "if=raid_event.movement.remains>gcd*0.3&spell_targets.holy_nova>0" );
}

void no_spec( player_t* p )
{
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* def       = p->get_action_priority_list( "default" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "smite" );
  def->add_action( "mana_potion,if=mana.pct<=75" );
  def->add_action( "shadowfiend" );
  def->add_action( "berserking" );
  def->add_action( "arcane_torrent,if=mana.pct<=90" );
  def->add_action( "shadow_word_pain,if=remains<tick_time|!ticking" );
  def->add_action( "smite" );
}

}  // namespace priest_apl
