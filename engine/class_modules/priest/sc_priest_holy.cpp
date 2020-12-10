// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
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
  talents.trail_of_light   = find_talent_spell( "Trail of Light" );
  talents.enduring_renewal = find_talent_spell( "Enduring Renewal" );
  // T25
  talents.angels_mercy    = find_talent_spell( "Angel's Mercy" );
  talents.body_and_soul   = find_talent_spell( "Body and Soul" );
  talents.angelic_feather = find_talent_spell( "Angelic Feather" );
  // T30
  talents.cosmic_ripple  = find_talent_spell( "Cosmic Ripple" );
  talents.guardian_angel = find_talent_spell( "Guardian Angel" );
  talents.after_life     = find_talent_spell( "After Life" );
  // T35
  talents.psychic_voice = find_talent_spell( "Psychic Voice" );
  talents.censure       = find_talent_spell( "Censure" );
  talents.shining_force = find_talent_spell( "Shining Force" );
  // T40
  talents.surge_of_light = find_talent_spell( "Surge of Light" );
  talents.binding_heal   = find_talent_spell( "Binding Heal" );
  talents.prayer_circle  = find_talent_spell( "Prayer Circle" );
  // T45
  talents.benediction = find_talent_spell( "Benediction" );
  talents.divine_star = find_talent_spell( "Divine Star" );
  talents.halo        = find_talent_spell( "Halo" );
  // T50
  talents.light_of_the_naaru  = find_talent_spell( "Light of the Naaru" );
  talents.apotheosis          = find_talent_spell( "Apotheosis" );
  talents.holy_word_salvation = find_talent_spell( "Holy Word: Salvation" );

  // General Spells
  specs.serendipity        = find_specialization_spell( "Serendipity" );
  specs.rapid_renewal      = find_specialization_spell( "Rapid Renewal" );
  specs.divine_providence  = find_specialization_spell( "Divine Providence" );
  specs.focused_will       = find_specialization_spell( "Focused Will" );
  specs.holy_words         = find_specialization_spell( "Holy Words" );
  specs.holy_word_serenity = find_specialization_spell( "Holy Word: Serenity" );

  // Spec Core
  specs.holy_priest = find_specialization_spell( "Holy Priest" );
}

action_t* priest_t::create_action_holy( util::string_view name, util::string_view options_str )
{
  using namespace actions::spells;
  using namespace actions::heals;

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

/** Holy Damage Combat Action Priority List */
void priest_t::generate_apl_holy_d()
{
  action_priority_list_t* default_list = get_action_priority_list( "default" );
  action_priority_list_t* precombat    = get_action_priority_list( "precombat" );

  // Precombat actions
  precombat->add_action( "potion" );
  precombat->add_action( this, "Smite" );

  // On-Use Items
  default_list->add_action( "use_items" );

  // Professions
  for ( const auto& profession_action : get_profession_actions() )
  {
    default_list->add_action( profession_action );
  }

  // Potions
  default_list->add_action(
      "potion,if=buff.bloodlust.react|(raid_event.adds.up&(raid_event.adds.remains>20|raid_event.adds.duration<20))|"
      "target.time_to_die<=30" );

  // Default APL
  default_list->add_action(
      this, "Holy Fire",
      "if=dot.holy_fire.ticking&(dot.holy_fire.remains<=gcd|dot.holy_fire.stack<2)&spell_targets.holy_nova<7" );
  default_list->add_action( this, "Holy Word: Chastise", "if=spell_targets.holy_nova<5" );
  default_list->add_action(
      this, "Holy Fire",
      "if=dot.holy_fire.ticking&(dot.holy_fire.refreshable|dot.holy_fire.stack<2)&spell_targets.holy_nova<7" );
  default_list->add_action(
      "berserking,if=raid_event.adds.in>30|raid_event.adds.remains>8|raid_event.adds.duration<8" );
  default_list->add_action( "fireblood,if=raid_event.adds.in>20|raid_event.adds.remains>6|raid_event.adds.duration<6" );
  default_list->add_action(
      "ancestral_call,if=raid_event.adds.in>20|raid_event.adds.remains>10|raid_event.adds.duration<10" );
  default_list->add_talent(
      this, "Divine Star",
      "if=(raid_event.adds.in>5|raid_event.adds.remains>2|raid_event.adds.duration<2)&spell_targets.divine_star>1" );
  default_list->add_talent(
      this, "Halo",
      "if=(raid_event.adds.in>14|raid_event.adds.remains>2|raid_event.adds.duration<2)&spell_targets.halo>0" );
  default_list->add_action(
      "lights_judgment,if=raid_event.adds.in>50|raid_event.adds.remains>4|raid_event.adds.duration<4" );
  default_list->add_action(
      "arcane_pulse,if=(raid_event.adds.in>40|raid_event.adds.remains>2|raid_event.adds.duration<2)&spell_targets."
      "arcane_pulse>2" );
  default_list->add_action( this, "Holy Fire", "if=!dot.holy_fire.ticking&spell_targets.holy_nova<7" );
  default_list->add_action( this, "Holy Nova", "if=spell_targets.holy_nova>3" );
  default_list->add_talent(
      this, "Apotheosis", "if=active_enemies<5&(raid_event.adds.in>15|raid_event.adds.in>raid_event.adds.cooldown-5)" );
  default_list->add_action( this, "Smite" );
  default_list->add_action( this, "Holy Fire" );
  default_list->add_talent(
      this, "Divine Star",
      "if=(raid_event.adds.in>5|raid_event.adds.remains>2|raid_event.adds.duration<2)&spell_targets.divine_star>0" );
  default_list->add_action( this, "Holy Nova", "if=raid_event.movement.remains>gcd*0.3&spell_targets.holy_nova>0" );
}

/** Holy Heal Combat Action Priority List */
void priest_t::generate_apl_holy_h()
{
  create_apl_default();
}

}  // namespace priestspace
