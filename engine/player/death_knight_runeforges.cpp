#include "death_knight_runeforges.hpp"

#include "action/absorb.hpp"
#include "action/action.hpp"
#include "action/dot.hpp"
#include "actor_target_data.hpp"
#include "buff/buff.hpp"
#include "darkmoon_deck.hpp"
#include "dbc/data_enums.hh"
#include "dbc/item_database.hpp"
#include "dbc/spell_data.hpp"
#include "ground_aoe.hpp"
#include "item/item.hpp"
#include "player/action_variable.hpp"
#include "player/consumable.hpp"
#include "player/pet_spawner.hpp"
#include "set_bonus.hpp"
#include "sim/cooldown.hpp"
#include "sim/proc_rng.hpp"
#include "sim/sim.hpp"
#include "unique_gear.hpp"
#include "unique_gear_helper.hpp"
#include "util/string_view.hpp"

namespace unique_gear::death_knight_runeforges
{
// can be called via unqualified lookup
void register_special_effect( unsigned spell_id, custom_cb_t init_callback, bool fallback = false )
{
  unique_gear::register_special_effect( spell_id, init_callback, fallback );
}

void fallen_crusader( special_effect_t& effect )
{
  struct fallen_crusader_heal_t final : public heal_t
  {
    fallen_crusader_heal_t( std::string_view name, player_t* p, const spell_data_t* data ) : heal_t( name, p, data )
    {
      background = true;
      target     = p;
      callbacks = may_crit       = false;
      base_pct_heal              = data->effectN( 2 ).percent();
      const spell_data_t* talent = p->find_talent_spell( talent_tree::CLASS, "Unholy Bond" );
      base_pct_heal *= 1.0 + talent->effectN( 2 ).percent();
    }

    // Procs by default target the target of the action that procced them.
    void execute() override
    {
      target = player;
      heal_t::execute();
    }
  };

  if ( effect.player->type != DEATH_KNIGHT )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  effect.custom_buff = create_buff<stat_buff_t>( effect.player, "unholy_strength", effect.player->find_spell( 53365 ) )
                           ->set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE )
                           ->set_pct_buff_type( STAT_PCT_BUFF_STRENGTH );

  effect.execute_action = new fallen_crusader_heal_t( "unholy_strength", effect.player, effect.driver()->effectN( 1 ).trigger() );

  new dbc_proc_callback_t( effect.player, effect );
}

//void razorice( special_effect_t& effect )
//{
//  if ( effect.player->type != DEATH_KNIGHT )
//  {
//    effect.type = SPECIAL_EFFECT_NONE;
//    return;
//  }
//
//  if ( !p->background_actions.runeforge_razorice )
//    p->background_actions.runeforge_razorice = get_action<razorice_attack_t>( "razorice", p );
//
//  // Store in which hand razorice is equipped, as it affects which abilities proc it
//  switch ( effect.item->slot )
//  {
//    case SLOT_MAIN_HAND:
//      p->runeforge.rune_of_razorice_mh = true;
//      break;
//    case SLOT_OFF_HAND:
//      p->runeforge.rune_of_razorice_oh = true;
//      break;
//    default:
//      break;
//  }
//}
//
//void stoneskin_gargoyle( special_effect_t& effect )
//{
//  if ( effect.player->type != DEATH_KNIGHT )
//  {
//    effect.type = SPECIAL_EFFECT_NONE;
//    return;
//  }
//
//  death_knight_t* p = debug_cast<death_knight_t*>( effect.player );
//
//  p->runeforge.rune_of_the_stoneskin_gargoyle = true;
//
//  if ( !p->buffs.stoneskin_gargoyle )
//    p->buffs.stoneskin_gargoyle = make_buff( p, "stoneskin_gargoyle", effect.driver() )
//                                      ->set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE );
//  else
//    p->buffs.stoneskin_gargoyle->set_max_stack( p->buffs.stoneskin_gargoyle->max_stack() + 1 );
//
//  // The buff isn't shown ingame, leave it visible in the sim for clarity
//  // p -> quiet = true;
//}
//
//void apocalypse( special_effect_t& effect )
//{
//  if ( effect.player->type != DEATH_KNIGHT )
//  {
//    effect.type = SPECIAL_EFFECT_NONE;
//    return;
//  }
//
//  death_knight_t* p = debug_cast<death_knight_t*>( effect.player );
//  // Nothing happens if the runeforge is applied on both weapons
//  if ( p->runeforge.rune_of_apocalypse )
//    return;
//
//  p->spell.apocalypse_death_debuff      = p->find_spell( 327095 );
//  p->spell.apocalypse_famine_debuff     = p->find_spell( 327092 );
//  p->spell.apocalypse_war_debuff        = p->find_spell( 327096 );
//  p->spell.apocalypse_pestilence_damage = p->find_spell( 327093 );
//  // Triggering the effects is handled in pet_melee_attack_t::impact()
//  p->runeforge.rune_of_apocalypse = true;
//  // Even though a pet procs it, the damage from Pestilence belongs directly to the player in logs
//  p->background_actions.runeforge_pestilence = get_action<runeforge_apocalypse_pestilence_t>( "pestilence", p );
//}
//
//void sanguination( special_effect_t& effect )
//{
//  if ( effect.player->type != DEATH_KNIGHT )
//  {
//    effect.type = SPECIAL_EFFECT_NONE;
//    return;
//  }
//
//  death_knight_t* p = debug_cast<death_knight_t*>( effect.player );
//  // This runeforge doesn't stack
//  if ( p->runeforge.rune_of_sanguination )
//    return;
//
//  p->spell.sanguination_cooldown = p->find_spell( 326809 );
//
//  struct sanguination_heal_t final : public death_knight_heal_t
//  {
//    sanguination_heal_t( special_effect_t& effect )
//      : death_knight_heal_t( "rune_of_sanguination", debug_cast<death_knight_t*>( effect.player ),
//                             effect.driver()->effectN( 1 ).trigger() ),
//        health_threshold( effect.driver()->effectN( 1 ).base_value() )
//    {
//      background    = true;
//      tick_pct_heal = data().effectN( 1 ).percent();
//      tick_pct_heal *= 1.0 + p()->talent.unholy_bond->effectN( 1 ).percent();
//      // Sated-type debuff, for simplicity the debuff's duration is used as a simple cooldown in simc
//      cooldown->duration = p()->spell.sanguination_cooldown->duration();
//    }
//
//    bool ready() override
//    {
//      if ( p()->health_percentage() > health_threshold )
//        return false;
//
//      return death_knight_heal_t::ready();
//    }
//
//  private:
//    double health_threshold;
//  };
//
//  p->runeforge.rune_of_sanguination = true;
//
//  p->background_actions.runeforge_sanguination = new sanguination_heal_t( effect );
//}
//
//void spellwarding( special_effect_t& effect )
//{
//  struct spellwarding_absorb_t final : public absorb_t
//  {
//    spellwarding_absorb_t( std::string_view name, death_knight_t* p, const spell_data_t* data )
//      : absorb_t( name, p, data ), health_percentage( p->spell.spellwarding_absorb->effectN( 2 ).percent() )
//    // The absorb amount is hardcoded in the effect tooltip, the only data is in the runeforging action spell
//    {
//      target     = p;
//      background = true;
//      harmful    = false;
//    }
//
//    void execute() override
//    {
//      base_dd_min = base_dd_max = health_percentage * player->resources.max[ RESOURCE_HEALTH ];
//
//      absorb_t::execute();
//    }
//
//  private:
//    double health_percentage;
//  };
//
//  if ( effect.player->type != DEATH_KNIGHT )
//  {
//    effect.type = SPECIAL_EFFECT_NONE;
//    return;
//  }
//
//  death_knight_t* p = debug_cast<death_knight_t*>( effect.player );
//
//  p->spell.spellwarding_absorb = p->find_spell( 326855 );
//
//  // Stacking the rune doubles the damage reduction, and seems to create a second proc
//  p->runeforge.rune_of_spellwarding += effect.driver()->effectN( 2 ).percent();
//  effect.execute_action =
//      get_action<spellwarding_absorb_t>( "rune_of_spellwarding", p, effect.driver()->effectN( 1 ).trigger() );
//
//  new dbc_proc_callback_t( effect.player, effect );
//}
//
//// NYI
//void unending_thirst( special_effect_t& effect )
//{
//  if ( effect.player->type != DEATH_KNIGHT )
//  {
//    effect.type = SPECIAL_EFFECT_NONE;
//    return;
//  }
//
//  // Placeholder for APL tracking purpose, effect NYI
//  debug_cast<player_t*>( effect.player )->runeforge.rune_of_unending_thirst = true;
//}

void register_special_effects()
{
  //register_special_effect( 50401, razorice );
  register_special_effect( 166441, fallen_crusader );
  //register_special_effect( 62157, stoneskin_gargoyle );
  //register_special_effect( 327087, apocalypse );
  //register_special_effect( 326801, sanguination );
  //register_special_effect( 326864, spellwarding );
  //register_special_effect( 326982, unending_thirst );
}
}  // namespace unique_gear::death_knight_runeforges