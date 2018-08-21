// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "sc_priest.hpp"

namespace priestspace
{
namespace actions
{
namespace spells
{
/// Holy Fire Base Spell, used for both Holy Fire and its overriding spell Purge the Wicked
struct holy_fire_base_t : public priest_spell_t
{
  holy_fire_base_t( const std::string& name, priest_t& p, const spell_data_t* sd ) : priest_spell_t( name, p, sd )
  {
  }
};

struct apotheosis_t final : public priest_spell_t
{
    apotheosis_t(priest_t& p, const std::string& options_str)
        : priest_spell_t("apotheosis", p, p.talents.apotheosis)
    {
        parse_options(options_str);

        harmful = false;
    }

    void execute() override
    {
        priest_spell_t::execute();

        priest().buffs.apotheosis->trigger();
        if (sim->debug)
        {
             sim->out_debug.printf("%s starting Apotheosis. ", priest().name());
        }
    }
};

struct holy_fire_t final : public holy_fire_base_t
{
  holy_fire_t( priest_t& player, const std::string& options_str )
    : holy_fire_base_t( "holy_fire", player, player.find_class_spell( "Holy Fire" ) )
  {
    parse_options( options_str );

    auto rank2 = priest().find_specialization_spell( 231687 );
    if ( rank2->ok() )
    {
      dot_max_stack += rank2->effectN( 2 ).base_value();
    }
  }
};

struct holy_word_chastise_t final : public priest_spell_t
{
  holy_word_chastise_t( priest_t& player, const std::string& options_str )
    : priest_spell_t( "holy_word_chastise", player, player.find_class_spell( "Holy Word: Chastise" ) )
  {
    parse_options( options_str );
  }
  double cost() const override
  {
      if (priest().buffs.apotheosis->check())
      return 0;
      return priest_spell_t::cost();
  }
};

//TODO Fix targeting to start from the priest and not the target
struct holy_nova_t final : public priest_spell_t
{
  const spell_data_t* holy_fire_rank2;

  holy_nova_t( priest_t& player, const std::string& options_str )
    : priest_spell_t( "holy_nova", player, player.find_class_spell( "Holy Nova" ) ),
      holy_fire_rank2( player.find_specialization_spell( 231687 ) )
  {
    parse_options( options_str );
    aoe = -1;
  }
  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( holy_fire_rank2->ok() && s->result_amount > 0 )
    {
      double hf_proc_chance = holy_fire_rank2->effectN( 1 ).percent();
      
      if ( rng().roll( hf_proc_chance ) )
      {
        if ( sim->debug )
        {
          sim->out_debug.printf( "%s reset holy fire cooldown, using holy nova. ", priest().name() );
        }
        priest().cooldowns.holy_fire->reset( true );
      }
    }
  }
};
}  // namespace spells

}  // namespace actions

void priest_t::create_buffs_holy()
{
// baseline
    buffs.apotheosis = make_buff(this, "apotheosis", talents.apotheosis);
}

void priest_t::init_rng_holy()
{
}

void priest_t::init_spells_holy()
{
  // Talents
  // T15
  talents.enlightenment    = find_talent_spell("Enlightenment");
  talents.trail_of_light   = find_talent_spell( "Trail of Light" );
  talents.enduring_renewal = find_talent_spell( "Enduring Renewal" );
  // T30
  talents.angels_mercy    = find_talent_spell( "Angel's Mercy" );
  talents.perseverance    = find_talent_spell( "Perseverance" );
  talents.angelic_feather = find_talent_spell("Angelic Feather");
  // T45
  talents.cosmic_ripple      = find_talent_spell("Cosmic Ripple");
  talents.guardian_angel     = find_talent_spell( "Guardian Angel" );
  talents.after_life         = find_talent_spell( "After Life" );
  // T60
  talents.psychic_voice = find_talent_spell( "Psychic Voice" );
  talents.censure       = find_talent_spell( "Censure" );
  talents.shining_force = find_talent_spell("Shining Force");
  // T75
  talents.surge_of_light    = find_talent_spell( "Surge of Light" );
  talents.binding_heal      = find_talent_spell( "Binding Heal" );
  talents.circle_of_healing = find_talent_spell( "Circle of Healing" );
  // T90
  talents.benediction = find_talent_spell( "Benediction" );
  talents.divine_star = find_talent_spell( "Divine Star" );
  talents.halo        = find_talent_spell( "Halo" );
  // T100
  talents.light_of_the_naaru  = find_talent_spell("Light of the Naaru");
  talents.apotheosis          = find_talent_spell( "Apotheosis" );
  talents.holy_word_salvation = find_talent_spell( "Holy Word: Salvation" );

  // General Spells
  specs.serendipity       = find_specialization_spell( "Serendipity" );
  specs.rapid_renewal     = find_specialization_spell( "Rapid Renewal" );
  specs.divine_providence = find_specialization_spell( "Divine Providence" );
  specs.focused_will      = find_specialization_spell( "Focused Will" );

  // Spec Core
  specs.holy_priest = find_specialization_spell( "Holy Priest" );

  // Range Based on Talents
  if ( base.distance != 5 )
  {
    if ( talents.divine_star->ok() )
    {
      base.distance = 24.0;
    }
    else if ( talents.halo->ok() )
    {
      base.distance = 27.0;
    }
    else
    {
      base.distance = 27.0;
    }
  }
}

action_t* priest_t::create_action_holy( const std::string& name, const std::string& options_str )
{
  using namespace actions::spells;
  using namespace actions::heals;

  if ( name == "holy_fire" )
  {
    return new holy_fire_t( *this, options_str );
  }

  if (name == "apotheosis")
  {
	  return new apotheosis_t(*this, options_str);
  }

  if ( name == "holy_nova" )
  {
    return new holy_nova_t( *this, options_str );
  }

  if ( name == "holy_word_chastise" )
  {
    return new holy_word_chastise_t( *this, options_str );
  }
  return nullptr;
}

expr_t* priest_t::create_expression_holy( action_t*, const std::string& /*name_str*/ )
{
  return nullptr;
}

/** Holy Damage Combat Action Priority List */
void priest_t::generate_apl_holy_d()
{
    action_priority_list_t* default_list = get_action_priority_list("default");
    action_priority_list_t* precombat = get_action_priority_list("precombat");

    // Precombat actions
    precombat->add_action(this, "Smite");

    // On-Use Items
    for (const std::string& item_action : get_item_actions())
    {
        default_list->add_action(item_action);
    }

    // Professions
    for (const std::string& profession_action : get_profession_actions())
    {
        default_list->add_action(profession_action);
    }

    // Potions
    default_list->add_action(
         "potion,if=buff.bloodlust.react|target.time_to_die<=80");

    // Racials
    if (race == RACE_DARK_IRON_DWARF)
        default_list->add_action("fireblood");
    if (race == RACE_TROLL)
        default_list->add_action("berserking");
    if (race == RACE_LIGHTFORGED_DRAENEI)
        default_list->add_action("lights_judgment");
    if (race == RACE_MAGHAR_ORC)
        default_list->add_action("ancestral_call");

    // Default APL
    default_list->add_action( this, "Holy Fire");
    default_list->add_action(this, "Holy word: Chastise");
    default_list->add_talent( this, "Apotheosis");
    default_list->add_talent( this, "Divine Star");
    default_list->add_talent( this, "Halo");
    default_list->add_action( this, "Holy Nova", "if=active_enemies>1");
    default_list->add_action( this, "Smite");
}

/** Holy Heal Combat Action Priority List */
void priest_t::generate_apl_holy_h()
{
  create_apl_default();
}

}  // namespace priestspace
