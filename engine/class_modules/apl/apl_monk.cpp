#include "class_modules/monk/sc_monk.hpp"
#include "player/action_priority_list.hpp"
#include "player/player.hpp"

#include <string>

using monk::monk_t;

// Per Specialization Defaults
namespace
{
namespace brewmaster
{
std::string default_potion( const monk_t* player )
{
  if ( player->true_level >= 80 )
    return "tempered_potion_3";
  return "disabled";
}

std::string default_flask( const monk_t* player )
{
  if ( player->true_level >= 80 )
    return "flask_of_alchemical_chaos_3";
  return "disabled";
}

std::string default_food( const monk_t* player )
{
  if ( player->true_level >= 80 )
    return "feast_of_the_midnight_masquerade";
  return "disabled";
}

std::string default_rune( const monk_t* player )
{
  if ( player->true_level >= 80 )
    return "crystallized";
  return "disabled";
}

std::string default_temporary_enchant( const monk_t* player )
{
  if ( player->true_level >= 80 )
    return "main_hand:ironclaw_whetstone_3/off_hand:ironclaw_whetstone_3";
  return "disabled";
}

void default_apl( monk_t* /* player */ )
{
}
};  // namespace brewmaster

namespace windwalker
{
std::string default_potion( const monk_t* player )
{
  if ( player->true_level >= 80 )
    return "tempered_potion_3";
  return "disabled";
}

std::string default_flask( const monk_t* player )
{
  if ( player->true_level >= 80 )
    return "flask_of_alchemical_chaos_3";
  return "disabled";
}

std::string default_food( const monk_t* player )
{
  if ( player->true_level >= 80 )
    return "authentic_undermine_clam_chowder";
  return "disabled";
}

std::string default_rune( const monk_t* player )
{
  if ( player->true_level >= 80 )
    return "crystallized";
  return "disabled";
}

std::string default_temporary_enchant( const monk_t* /* player */ )
{
  // if ( player->true_level >= 80 )
  //   return "main_hand:algari_mana_oil_3/off_hand:algari_mana_oil_3";
  return "disabled";
}

void live_apl( monk_t* /* player */ )
{
}

void ptr_apl( monk_t* player )
{
  live_apl( player );
}

void default_apl( monk_t* player )
{
  if ( !player->is_ptr() )
    live_apl( player );
  else
    ptr_apl( player );
}
};  // namespace windwalker
};  // namespace

// Shared Defaults
namespace monk
{
void monk_t::init_blizzard_action_list()
{
  action_priority_list_t* default_ = get_action_priority_list( "default" );

  switch ( specialization() )
  {
    case MONK_BREWMASTER:
    case MONK_WINDWALKER:
      default_->add_action( "auto_attack", "Overridden" );
      break;
    default:
      assert( false );
      break;
  }

  base_t::init_blizzard_action_list();

  action_priority_list_t* cooldowns = get_action_priority_list( "cooldowns" );

  switch ( specialization() )
  {
    case MONK_BREWMASTER:
      cooldowns->add_action( "invoke_niuzao_the_black_ox" );
      break;
    case MONK_WINDWALKER:
      cooldowns->add_action( "invoke_xuen_the_white_tiger" );
      cooldowns->add_action( "touch_of_karma" );
      break;
    default:
      assert( false );
      break;
  }
}

void monk_t::parse_assisted_combat_step( const assisted_combat_step_data_t& step,
                                         action_priority_list_t* assisted_combat )
{
  if ( step.spell_id == 388193 )
    return;

  base_t::parse_assisted_combat_step( step, assisted_combat );
}

parsed_assisted_combat_rule_t monk_t::parse_assisted_combat_rule( const assisted_combat_rule_data_t& rule,
                                                                  const assisted_combat_step_data_t& step ) const
{
  // Assisted Combat APL is partially updated and still includes references to Emperor's Capacitor
  if ( step.spell_id == 117952 && rule.condition_type == AC_PLAYER_AURA_APPLICATION_GREATER &&
       rule.condition_value_1 == 393039 && rule.condition_value_2 == 20 && rule.condition_value_3 == 0 )
    return "1";

  if ( step.spell_id == 152175 && rule.condition_type == AC_TARGET_DISTANCE_LESS )
  {
    assisted_combat_rule_data_t rule_copy = rule;
    rule_copy.condition_value_1           = 10;

    return { base_t::parse_assisted_combat_rule( rule_copy, step ), "Extended range check to 10 yards (from 5)." };
  }

  if ( step.spell_id == 100784 && rule.condition_type == AC_AURA_ON_PLAYER && rule.condition_value_1 == 116768 &&
       rule.condition_value_2 == 0 && rule.condition_value_3 == 0 )
    return {
        "buff.combo_breaker.up",
        "The name `Combo Breaker` is used instead of `Blackout Kick` due to standard tokenization rules and clarity.",
        true };

  if ( step.spell_id == 100780 && rule.condition_type == AC_AURA_MISSING_PLAYER && rule.condition_value_1 == 261916 &&
       rule.condition_value_2 == 0 && rule.condition_value_3 == 0 )
    return { "level<17", "Checks for Blackout Kick Rank 2 not being known, which is learned at level 17.", true };

  if ( rule.condition_type == AC_AURA_MISSING_PLAYER && rule.condition_value_2 == 0 && rule.condition_value_3 == 0 )
  {
    switch ( rule.condition_value_1 )
    {
      case 1249753:
      case 1249754:
      case 1249756:
      case 1249757:
      case 1249758:
      case 1249764:
      case 1249765:
      case 1249766:
        return { "combo_strike",
                 fmt::format( "Spell id {} is a helper buff to avoid breaking Combo Strikes for {}.",
                              rule.condition_value_1, find_spell( step.spell_id )->name_cstr() ),
                 false };
    }
  }

  if ( rule.condition_type == AC_TARGET_COUNT_NEAR_PLAYER_GREATER && rule.condition_value_1 == 1 &&
       rule.condition_value_2 == 15 && rule.condition_value_3 == 0 )
    return { "1", "Counts valid targets for action, including player." };

  return base_t::parse_assisted_combat_rule( rule, step );
}

std::vector<std::string> monk_t::action_names_from_spell_id( unsigned int spell_id ) const
{
  if ( spell_id == 467307 )
    return { "rising_sun_kick" };

  return base_t::action_names_from_spell_id( spell_id );
}

std::string monk_t::aura_expr_from_spell_id( unsigned int spell_id, bool on_self ) const
{
  if ( on_self )
  {
    switch ( spell_id )
    {
      case 443616:
        return "buff.heart_of_the_jade_serpent_unity_within";
      case 1238904:
        return "buff.heart_of_the_jade_serpent_yulons_avatar";
    }
  }

  return base_t::aura_expr_from_spell_id( spell_id, on_self );
}
};  // namespace monk

namespace monk
{
std::string monk_t::default_potion() const
{
  switch ( specialization() )
  {
    case MONK_BREWMASTER:
      return brewmaster::default_potion( this );
    case MONK_WINDWALKER:
      return windwalker::default_potion( this );
    default:
      return "disabled";
  }
}

std::string monk_t::default_flask() const
{
  switch ( specialization() )
  {
    case MONK_BREWMASTER:
      return brewmaster::default_flask( this );
    case MONK_WINDWALKER:
      return windwalker::default_flask( this );
    default:
      return "disabled";
  }
}

std::string monk_t::default_food() const
{
  switch ( specialization() )
  {
    case MONK_BREWMASTER:
      return brewmaster::default_food( this );
    case MONK_WINDWALKER:
      return windwalker::default_food( this );
    default:
      return "disabled";
  }
}

std::string monk_t::default_rune() const
{
  switch ( specialization() )
  {
    case MONK_BREWMASTER:
      return brewmaster::default_rune( this );
    case MONK_WINDWALKER:
      return windwalker::default_rune( this );
    default:
      return "disabled";
  }
}

std::string monk_t::default_temporary_enchant() const
{
  switch ( specialization() )
  {
    case MONK_BREWMASTER:
      return brewmaster::default_temporary_enchant( this );
    case MONK_WINDWALKER:
      return windwalker::default_temporary_enchant( this );
    default:
      return "disabled";
  }
}

void monk_t::init_action_list()
{
  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    switch ( specialization() )
    {
      case MONK_BREWMASTER:
        brewmaster::default_apl( this );
        break;
      case MONK_WINDWALKER:
        windwalker::default_apl( this );
        break;
      default:
        assert( false );
        break;
    }

    use_default_action_list = true;
  }

  base_t::init_action_list();
}
};  // namespace monk
