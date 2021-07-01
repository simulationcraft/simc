// ==========================================================================
// Holy Priest Sim File
// Contact: https://github.com/orgs/simulationcraft/teams/priest/members
// Wiki: https://github.com/simulationcraft/simc/wiki/Priests
// ==========================================================================

#include "sc_priest.hpp"

#include "simulationcraft.hpp"


namespace priestspace
{
namespace actions
{
namespace spells
{
/// Holy Fire Base Spell, used for both Holy Fire and its overriding spell Purge the Wicked
struct holy_fire_base_t : public priest_spell_t
{
  holy_fire_base_t( util::string_view name, priest_t& p, const spell_data_t* sd ) : priest_spell_t( name, p, sd )
  {
  }
};

struct apotheosis_t final : public priest_spell_t
{
  apotheosis_t( priest_t& p, util::string_view options_str ) : priest_spell_t( "apotheosis", p, p.talents.apotheosis )
  {
    parse_options( options_str );

    harmful = false;
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().buffs.apotheosis->trigger();
    sim->print_debug( "{} starting Apotheosis. ", priest() );
  }
};

struct holy_fire_t final : public holy_fire_base_t
{
  holy_fire_t( priest_t& player, util::string_view options_str )
    : holy_fire_base_t( "holy_fire", player, player.find_class_spell( "Holy Fire" ) )
  {
    parse_options( options_str );

    auto rank2 = priest().find_specialization_spell( "Holy Fire", "Rank 2" );
    if ( rank2->ok() )
    {
      dot_max_stack += as<int>( rank2->effectN( 2 ).base_value() );
    }
  }
};

struct holy_word_chastise_t final : public priest_spell_t
{
  holy_word_chastise_t( priest_t& player, util::string_view options_str )
    : priest_spell_t( "holy_word_chastise", player, player.find_class_spell( "Holy Word: Chastise" ) )
  {
    parse_options( options_str );
  }
  double cost() const override
  {
    if ( priest().buffs.apotheosis->check() )
      return 0;
    return priest_spell_t::cost();
  }
};

struct holy_word_serenity_t final : public priest_heal_t
{
  holy_word_serenity_t( priest_t& p, util::string_view options_str )
    : priest_heal_t( "holy_word_serenity", p, p.find_class_spell( "Holy Word: Serenity" ) )
  {
    parse_options( options_str );
    harmful = false;
  }
};

// TODO Fix targeting to start from the priest and not the target
// TODO Implement 3+ targets hit extra spawn of Holy Nova
struct holy_nova_t final : public priest_spell_t
{
  const spell_data_t* holy_fire_rank2;

  holy_nova_t( priest_t& player, util::string_view options_str )
    : priest_spell_t( "holy_nova", player, player.find_class_spell( "Holy Nova" ) ),
      holy_fire_rank2( player.find_rank_spell( "Holy Nova", "Rank 2" ) )
  {
    parse_options( options_str );
    aoe = -1;
    apply_affecting_aura( player.find_rank_spell( "Holy Nova", "Rank 2" ) );  // GCD reduction
  }
  void execute() override
  {
    priest_spell_t::execute();

    if ( holy_fire_rank2->ok() )
    {
      double hf_proc_chance = holy_fire_rank2->effectN( 1 ).percent();

      if ( rng().roll( hf_proc_chance ) )
      {
        sim->print_debug( "{} reset holy fire cooldown, using holy nova. ", priest() );
        priest().cooldowns.holy_fire->reset( true );
      }
    }
  }
};

struct flash_heal_t final : public priest_heal_t
{
  flash_heal_t( priest_t& p, util::string_view options_str )
    : priest_heal_t( "flash_heal", p, p.find_class_spell( "Flash Heal" ) )
  {
    parse_options( options_str );
    harmful = false;
  }

  void impact( action_state_t* s ) override
  {
    priest_heal_t::impact( s );

    priest().adjust_holy_word_serenity_cooldown();
  }
};

struct renew_t final : public priest_heal_t
{
  renew_t( priest_t& p, util::string_view options_str ) : priest_heal_t( "renew", p, p.find_class_spell( "Renew" ) )
  {
    parse_options( options_str );
    harmful = false;
  }
};

struct holy_heal_t final : public priest_heal_t
{
  holy_heal_t( priest_t& p, util::string_view options_str ) : priest_heal_t( "heal", p, p.find_class_spell( "Heal" ) )
  {
    parse_options( options_str );
    harmful = false;
  }

  void impact( action_state_t* s ) override
  {
    priest_heal_t::impact( s );

    priest().adjust_holy_word_serenity_cooldown();
  }
};

}  // namespace spells

}  // namespace actions

void priest_t::create_buffs_holy()
{
  // baseline
  buffs.apotheosis = make_buff( this, "apotheosis", talents.apotheosis );
}

void priest_t::init_rng_holy()
{
}

void priest_t::init_spells_holy()
{
  // Talents
  // T15
  talents.enlightenment    = find_talent_spell( "Enlightenment" );
  // T50
  talents.light_of_the_naaru  = find_talent_spell( "Light of the Naaru" );
  talents.apotheosis          = find_talent_spell( "Apotheosis" );

  // General Spells
  specs.holy_words         = find_specialization_spell( "Holy Words" );
  specs.holy_word_serenity = find_specialization_spell( "Holy Word: Serenity" );

}

action_t* priest_t::create_action_holy( util::string_view name, util::string_view options_str )
{
  using namespace actions::spells;

  if ( name == "holy_fire" )
  {
    return new holy_fire_t( *this, options_str );
  }

  if ( name == "apotheosis" )
  {
    return new apotheosis_t( *this, options_str );
  }

  if ( name == "holy_nova" )
  {
    return new holy_nova_t( *this, options_str );
  }

  if ( name == "holy_word_chastise" )
  {
    return new holy_word_chastise_t( *this, options_str );
  }

  if ( name == "holy_word_serenity" )
  {
    return new holy_word_serenity_t( *this, options_str );
  }

  if ( name == "flash_heal" )
  {
    return new flash_heal_t( *this, options_str );
  }

  if ( name == "renew" )
  {
    return new renew_t( *this, options_str );
  }

  if ( name == "heal" )
  {
    return new holy_heal_t( *this, options_str );
  }

  return nullptr;
}

expr_t* priest_t::create_expression_holy( action_t*, util::string_view /*name_str*/ )
{
  return nullptr;
}

/**
 * Adjust cooldown of Holy Word: Serenity when casting Flash Heal or Heal
 */
void priest_t::adjust_holy_word_serenity_cooldown()
{
  if ( !specs.holy_words->ok() )
  {
    return;
  }

  auto adjustment = -timespan_t::from_seconds( specs.holy_word_serenity->effectN( 2 ).base_value() );
  cooldowns.holy_word_serenity->adjust( adjustment );
}

}  // namespace priestspace
