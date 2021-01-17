// ==========================================================================
// Priest APL File
// Contact: https://github.com/orgs/simulationcraft/teams/priest/members
// Wiki: https://github.com/simulationcraft/simc/wiki/Priests
// ==========================================================================

#include "class_modules/apl/apl_priest.hpp"

#include "class_modules/priest/sc_priest.hpp"
#include "player/action_priority_list.hpp"
#include "player/sc_player.hpp"

namespace priest_apl
{
std::string potion( const player_t* p )
{
  std::string lvl60_potion =
      ( p->specialization() == PRIEST_SHADOW ) ? "potion_of_phantom_fire" : "potion_of_spectral_intellect";
  std::string lvl50_potion = ( p->specialization() == PRIEST_SHADOW ) ? "unbridled_fury" : "battle_potion_of_intellect";

  return ( p->true_level > 50 ) ? lvl60_potion : lvl50_potion;
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
  auto priest = debug_cast<priestspace::priest_t*>( p );

  action_priority_list_t* precombat    = p->get_action_priority_list( "precombat" );
  action_priority_list_t* default_list = p->get_action_priority_list( "default" );
  action_priority_list_t* main         = p->get_action_priority_list( "main" );
  action_priority_list_t* cwc          = p->get_action_priority_list( "cwc" );
  action_priority_list_t* cds          = p->get_action_priority_list( "cds" );
  action_priority_list_t* boon         = p->get_action_priority_list( "boon" );
  action_priority_list_t* trinkets     = p->get_action_priority_list( "trinkets" );
  action_priority_list_t* dmg_trinkets = p->get_action_priority_list( "dmg_trinkets" );

  // Snapshot stats
  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  // Calculate these variables once to reduce sim time
  precombat->add_action( "shadowform,if=!buff.shadowform.up" );
  precombat->add_action( "arcane_torrent" );
  precombat->add_action( "use_item,name=azsharas_font_of_power" );
  precombat->add_action( "variable,name=mind_sear_cutoff,op=set,value=2" );
  precombat->add_action( "vampiric_touch" );

  // Professions
  for ( const auto& profession_action : p->get_profession_actions() )
  {
    default_list->add_action( profession_action );
  }

  // Potions
  default_list->add_action( "potion,if=buff.voidform.up|buff.power_infusion.up" );
  default_list->add_action(
      "variable,name=dots_up,op=set,value="
      "dot.shadow_word_pain.ticking&dot.vampiric_touch.ticking" );
  default_list->add_action(
      "variable,name=all_dots_up,op=set,value="
      "dot.shadow_word_pain.ticking&dot.vampiric_touch.ticking&dot.devouring_plague.ticking" );
  default_list->add_action(
      "variable,name=searing_nightmare_cutoff,op=set,value=spell_targets.mind_sear>2+buff.voidform.up",
      "Start using Searing Nightmare at 3+ targets or 4+ if you are in Voidform" );
  default_list->add_action(
      "variable,name=pool_for_cds,op=set,value=cooldown.void_eruption.up&(!raid_event.adds.up|raid_event.adds.duration<"
      "=10|raid_event.adds.remains>=10+5*(talent.hungering_void.enabled|covenant.kyrian))&((raid_event.adds.in>20|"
      "spell_targets.void_eruption>=5)|talent.hungering_void.enabled|covenant.kyrian)",
      "Cooldown Pool Variable, Used to pool before activating voidform. Currently used to control when to activate "
      "voidform with incoming adds." );

  // Racials
  default_list->add_action( "fireblood,if=buff.voidform.up" );
  default_list->add_action( "berserking,if=buff.voidform.up" );
  default_list->add_action(
      "lights_judgment,if=spell_targets.lights_judgment>=2|(!raid_event.adds.exists|raid_event.adds.in>75)",
      "Use Light's Judgment if there are 2 or more targets, or adds aren't spawning for more than 75s." );
  default_list->add_action( "ancestral_call,if=buff.voidform.up" );

  default_list->add_call_action_list( cwc );
  default_list->add_run_action_list( main );

  // DMG Trinkets, specifically tied together with Hungering Void
  dmg_trinkets->add_action( "use_item,name=darkmoon_deck__putrescence" );
  dmg_trinkets->add_action( "use_item,name=sunblood_amethyst" );
  dmg_trinkets->add_action( "use_item,name=glyph_of_assimilation" );
  dmg_trinkets->add_action( "use_item,name=dreadfire_vessel" );

  // Trinkets
  trinkets->add_action(
      "use_item,name=empyreal_ordnance,if=cooldown.void_eruption.remains<=12|cooldown.void_eruption.remains>27",
      "Use on CD ASAP to get DoT ticking and expire to line up better with Voidform" );
  trinkets->add_action( "use_item,name=inscrutable_quantum_device,if=cooldown.void_eruption.remains>10",
                        "Sync IQD with Voidform" );
  trinkets->add_action( "use_item,name=macabre_sheet_music,if=cooldown.void_eruption.remains>10",
                        "Sync Sheet Music with Voidform" );
  trinkets->add_action(
      "use_item,name=soulletting_ruby,if=buff.power_infusion.up|!priest.self_power_infusion,target_if=min:target."
      "health.pct",
      "Sync Ruby with Power Infusion usage, make sure to snipe the lowest HP target" );
  trinkets->add_action(
      "use_item,name=sinful_gladiators_badge_of_ferocity,if=cooldown.void_eruption.remains>=10",
      "Use Badge inside of VF for the first use or on CD after the first use. Short circuit if void eruption cooldown "
      "is 10s or more away." );
  trinkets->add_call_action_list(
      dmg_trinkets,
      "if=(!talent.hungering_void.enabled|debuff.hungering_void.up)&(buff.voidform.up|cooldown.void_eruption.remains>"
      "10)",
      "Use list of on-use damage trinkets only if Hungering Void Debuff is active, or you are not talented into it." );
  trinkets->add_action( "use_items,if=buff.voidform.up|buff.power_infusion.up|cooldown.void_eruption.remains>10",
                        "Default fallback for usable items: Use on cooldown in order by trinket slot." );

  // CDs
  cds->add_action(
      p, "Power Infusion",
      "if=priest.self_power_infusion&(buff.voidform.up|!soulbind.grove_invigoration.enabled&!soulbind.combat_"
      "meditation.enabled&cooldown.void_eruption.remains>=10|fight_remains<cooldown.void_eruption.remains|soulbind."
      "grove_invigoration.enabled&(buff.redirected_anima.stack>=12|cooldown.fae_guardians.remains>10))",
      "Use Power Infusion with Voidform. Hold for Voidform comes off cooldown in the next 10 seconds "
      "otherwise use on cd unless the Pelagos Trait Combat Meditation is talented, or if there will not "
      "be another Void Eruption this fight." );
  cds->add_action( p, "Silence",
                   "target_if=runeforge.sephuzs_proclamation.equipped&(target.is_add|target.debuff.casting.react)",
                   "Use Silence on CD to proc Sephuz's Proclamation." );
  cds->add_action( p, priest->covenant.fae_guardians, "fae_guardians",
                   "if=!buff.voidform.up&(!cooldown.void_torrent.up|!talent.void_torrent.enabled)|buff.voidform.up&("
                   "soulbind.grove_invigoration.enabled|soulbind.field_of_blossoms.enabled)",
                   "Use Fae Guardians on CD outside of Voidform. Use Fae Guardiands in Voidform if you have either "
                   "Grove Invigoration or Field of Blossoms" );
  cds->add_action( p, priest->covenant.mindgames, "mindgames",
                   "target_if=insanity<90&((variable.all_dots_up&(!cooldown.void_eruption.up|!talent.hungering_void."
                   "enabled))|buff.voidform.up)&(!talent.hungering_void.enabled|debuff.hungering_void.up|!buff."
                   "voidform.up)&(!talent.searing_nightmare.enabled|spell_targets.mind_sear<5)",
                   "Use Mindgames when all 3 DoTs are up, or you are in Voidform. Ensure Hungering Void is active on "
                   "the target if talented. Stop using at 5+ targets with Searing Nightmare." );
  cds->add_action(
      p, priest->covenant.unholy_nova, "unholy_nova",
      "if=((!raid_event.adds.up&raid_event.adds.in>20)|raid_event.adds.remains>=15|raid_event.adds.duration<"
      "15)&(buff.power_infusion.up|cooldown.power_infusion.remains>=10|!priest.self_power_infusion)&(!talent.hungering_"
      "void.enabled|debuff.hungering_void.up|!buff.voidform.up)",
      "Use Unholy Nova on CD, holding briefly to wait for power infusion or add spawns." );
  cds->add_action(
      p, priest->covenant.boon_of_the_ascended, "boon_of_the_ascended",
      "if=!buff.voidform.up&!cooldown.void_eruption.up&spell_targets.mind_sear>1&!talent.searing_nightmare.enabled|("
      "buff.voidform.up&spell_targets.mind_sear<2&!talent.searing_nightmare.enabled&(prev_gcd.1.void_bolt&(!equipped."
      "empyreal_ordnance|!talent.hungering_void.enabled)|equipped.empyreal_ordnance&trinket.empyreal_ordnance.cooldown."
      "remains<=162&debuff.hungering_void.up))|(buff.voidform.up&talent.searing_nightmare.enabled)",
      "Use on CD but prioritise using Void Eruption first, if used inside of VF on ST use after a "
      "voidbolt for cooldown efficiency and for hungering void uptime if talented." );
  cds->add_call_action_list( trinkets );

  // APL to use when Boon of the Ascended is active
  boon->add_action( p, priest->covenant.boon_of_the_ascended, "ascended_blast", "if=spell_targets.mind_sear<=3" );
  boon->add_action( p, priest->covenant.boon_of_the_ascended, "ascended_nova",
                    "if=spell_targets.ascended_nova>1&spell_targets.mind_sear>1+talent.searing_nightmare.enabled" );

  // Cast While Casting actions. Set at higher priority to short circuit interrupt conditions on Mind Sear/Flay
  cwc->add_talent( p, "Searing Nightmare",
                   "use_while_casting=1,target_if=(variable.searing_nightmare_cutoff&!variable.pool_for_cds)|(dot."
                   "shadow_word_pain.refreshable&spell_targets.mind_sear>1)",
                   "Use Searing Nightmare if you will hit enough targets and Power Infusion and Voidform are not "
                   "ready, or to refresh SW:P on two or more targets." );
  cwc->add_talent( p, "Searing Nightmare",
                   "use_while_casting=1,target_if=talent.searing_nightmare.enabled&dot.shadow_word_pain.refreshable&"
                   "spell_targets.mind_sear>2",
                   "Short Circuit Searing Nightmare condition to keep SW:P up in AoE" );
  cwc->add_action( p, "Mind Blast", "only_cwc=1",
                   "Only_cwc makes the action only usable during channeling and not as a regular action." );

  // Main APL, should cover all ranges of targets and scenarios
  main->add_call_action_list( p, priest->covenant.boon_of_the_ascended, boon, "if=buff.boon_of_the_ascended.up" );
  main->add_action( p, "Void Eruption",
                    "if=variable.pool_for_cds&insanity>=40&(insanity<=85|talent.searing_nightmare.enabled&variable."
                    "searing_nightmare_cutoff)&!cooldown.fiend.up&(!cooldown.mind_blast.up|spell_targets.mind_sear>2)",
                    "Use Void Eruption on cooldown pooling at least 40 insanity but not if you will overcap insanity "
                    "in VF. Make sure shadowfiend/mindbender and Mind Blast is on cooldown before VE." );
  main->add_action( p, "Shadow Word: Pain", "if=buff.fae_guardians.up&!debuff.wrathful_faerie.up",
                    "Make sure you put up SW:P ASAP on the target if Wrathful Faerie isn't active." );
  main->add_call_action_list( cds );
  main->add_action( p, "Mind Sear",
                    "target_if=talent.searing_nightmare.enabled&spell_targets.mind_sear>variable.mind_sear_cutoff&!dot."
                    "shadow_word_pain.ticking&!cooldown.fiend.up",
                    "High Priority Mind Sear action to refresh DoTs with Searing Nightmare" );
  main->add_talent( p, "Damnation", "target_if=!variable.all_dots_up",
                    "Prefer to use Damnation ASAP if any DoT is not up." );
  main->add_action(
      p, "Void Bolt",
      "if=insanity<=85&talent.hungering_void.enabled&talent.searing_nightmare.enabled&spell_targets.mind_sear<=6|(("
      "talent.hungering_void.enabled&!talent.searing_nightmare.enabled)|spell_targets.mind_sear=1)",
      "Use Void Bolt at higher priority with Hungering Void up to 4 targets, or other talents on ST." );
  main->add_action( p, "Devouring Plague",
                    "if=(refreshable|insanity>75)&(!variable.pool_for_cds|insanity>=85)&(!talent.searing_nightmare."
                    "enabled|(talent.searing_nightmare.enabled&!variable.searing_nightmare_cutoff))",
                    "Don't use Devouring Plague if you can get into Voidform instead, or if Searing Nightmare is "
                    "talented and will hit enough targets." );
  main->add_action( p, "Void Bolt",
                    "if=spell_targets.mind_sear<(4+conduit.dissonant_echoes.enabled)&insanity<=85&talent.searing_"
                    "nightmare.enabled|!talent.searing_nightmare.enabled",
                    "Use VB on CD if you don't need to cast Devouring Plague, and there are less than 4 targets out (5 "
                    "with conduit)." );
  main->add_action( p, "Shadow Word: Death",
                    "target_if=(target.health.pct<20&spell_targets.mind_sear<4)|(pet.fiend.active&runeforge."
                    "shadowflame_prism.equipped)",
                    "Use Shadow Word: Death if the target is about to die or you have Shadowflame Prism equipped with "
                    "Mindbender or Shadowfiend active." );
  main->add_talent( p, "Surrender to Madness", "target_if=target.time_to_die<25&buff.voidform.down",
                    "Use Surrender to Madness on a target that is going to die at the right time." );
  main->add_talent( p, "Void Torrent",
                    "target_if=variable.dots_up&target.time_to_die>3&(buff.voidform.down|buff.voidform.remains<"
                    "cooldown.void_bolt.remains)&active_dot.vampiric_touch==spell_targets.vampiric_touch&spell_targets."
                    "mind_sear<(5+(6*talent.twist_of_fate.enabled))",
                    "Use Void Torrent only if SW:P and VT are active and the target won't die during the channel." );
  main->add_talent( p, "Mindbender",
                    "if=dot.vampiric_touch.ticking&(talent.searing_nightmare.enabled&spell_targets.mind_sear>variable."
                    "mind_sear_cutoff|dot.shadow_word_pain.ticking)" );
  main->add_action(
      p, "Shadow Word: Death",
      "if=runeforge.painbreaker_psalm.equipped&variable.dots_up&target.time_to_pct_20>(cooldown.shadow_word_death."
      "duration+gcd)",
      "Use SW:D with Painbreaker Psalm unless the target will be below 20% before the cooldown comes back" );
  main->add_talent( p, "Shadow Crash", "if=raid_event.adds.in>10",
                    "Use Shadow Crash on CD unless there are adds incoming." );
  main->add_action(
      p, "Mind Sear",
      "target_if=spell_targets.mind_sear>variable.mind_sear_cutoff&buff.dark_thought.up,chain=1,interrupt_immediate=1,"
      "interrupt_if=ticks>=2",
      "Use Mind Sear to consume Dark Thoughts procs on AOE. TODO Confirm is this is a higher priority than redotting "
      "on AOE unless dark thoughts is about to time out" );
  main->add_action( p, "Mind Flay",
                    "if=buff.dark_thought.up&variable.dots_up,chain=1,interrupt_immediate=1,interrupt_if=ticks>=2&"
                    "cooldown.void_bolt.up",
                    "Use Mind Flay to consume Dark Thoughts procs on ST. TODO Confirm if this is a higher priority "
                    "than redotting unless dark thoughts is about to time out" );
  main->add_action( p, "Mind Blast",
                    "if=variable.dots_up&raid_event.movement.in>cast_time+0.5&(spell_targets.mind_sear<4&!talent."
                    "misery.enabled|spell_targets.mind_sear<6&talent.misery.enabled)",
                    "Use Mind Blast if you don't need to refresh DoTs. Stop casting at 4 or more targets with Searing "
                    "Nightmare talented." );
  main->add_action( p, "Vampiric Touch",
                    "target_if=refreshable&target.time_to_die>6|(talent.misery.enabled&dot.shadow_word_pain."
                    "refreshable)|buff.unfurling_darkness.up" );
  main->add_action( p, "Shadow Word: Pain",
                    "if=refreshable&target.time_to_die>4&!talent.misery.enabled&talent.psychic_link.enabled&spell_"
                    "targets.mind_sear>2",
                    "Special condition to stop casting SW:P on off-targets when fighting 3 or more stacked mobs and "
                    "using Psychic Link and NOT Misery." );
  main->add_action(
      p, "Shadow Word: Pain",
      "target_if=refreshable&target.time_to_die>4&!talent.misery.enabled&!(talent.searing_nightmare.enabled&spell_"
      "targets.mind_sear>variable.mind_sear_cutoff)&(!talent.psychic_link.enabled|(talent.psychic_link.enabled&spell_"
      "targets.mind_sear<=2))",
      "Keep SW:P up on as many targets as possible, except when fighting 3 or more stacked mobs with Psychic Link." );
  main->add_action( p, "Mind Sear",
                    "target_if=spell_targets.mind_sear>variable.mind_sear_cutoff,chain=1,interrupt_immediate=1,"
                    "interrupt_if=ticks>=2" );
  main->add_action( p, "Mind Flay", "chain=1,interrupt_immediate=1,interrupt_if=ticks>=2&cooldown.void_bolt.up" );
  main->add_action( p, "Shadow Word: Death", "", "Use SW:D as last resort if on the move" );
  main->add_action( p, "Shadow Word: Pain", "", "Use SW:P as last resort if on the move and SW:D is on CD" );
}

void discipline( player_t* p )
{
  action_priority_list_t* def     = p->get_action_priority_list( "default" );
  action_priority_list_t* boon    = p->get_action_priority_list( "boon" );
  action_priority_list_t* racials = p->get_action_priority_list( "racials" );

  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "smite" );

  boon->add_action( "ascended_blast" );
  boon->add_action( "ascended_nova" );

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
  def->add_action(
      "shadow_covenant,if=!covenant.kyrian|(!cooldown.boon_of_the_ascended.up&!buff.boon_of_the_ascended.up)",
      "Hold Shadow Covenant if Boon of the Ascended cooldown is up or the buff is active." );
  def->add_action( "schism" );
  def->add_action( "mindgames" );
  def->add_action( "fae_guardians" );
  def->add_action( "unholy_nova" );
  def->add_action( "boon_of_the_ascended" );
  def->add_call_action_list( boon, "if=buff.boon_of_the_ascended.up" );
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
