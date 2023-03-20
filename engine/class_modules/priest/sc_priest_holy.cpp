// ==========================================================================
// Holy Priest Sim File
// Contact: https://github.com/orgs/simulationcraft/teams/priest/members
// Wiki: https://github.com/simulationcraft/simc/wiki/Priests
// ==========================================================================

#include "sc_priest.hpp"

#include "simulationcraft.hpp"

namespace priestspace
{
namespace actions::spells
{

struct apotheosis_t final : public priest_spell_t
{
  apotheosis_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "apotheosis", p, p.talents.holy.apotheosis )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().buffs.apotheosis->trigger();
    sim->print_debug( "starting apotheosis. ", priest() );
    priest().cooldowns.holy_word_chastise->reset( false );
    sim->print_debug( "apotheosis reset holy_word_chastise cooldown. ", priest() );
  }
};

struct empyreal_blaze_t final : public priest_spell_t
{
  empyreal_blaze_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "empyreal_blaze", p, p.talents.holy.empyreal_blaze )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    priest_spell_t::execute();

    sim->print_debug( "{} starting empyreal_blaze. ", priest() );
    priest().buffs.empyreal_blaze->trigger();
  }
};

struct holy_fire_t final : public priest_spell_t
{
  timespan_t manipulation_cdr;

  holy_fire_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "holy_fire", p, p.specs.holy_fire ),
      manipulation_cdr( timespan_t::from_seconds( priest().talents.manipulation->effectN( 1 ).base_value() / 2 ) )
  {
    parse_options( options_str );
    if ( p.talents.holy.empyreal_blaze.enabled() )
    {
      dot_behavior = DOT_EXTEND;
    }
  }

  double cost() const override
  {
    if ( priest().buffs.empyreal_blaze->check() )
    {
      return 0;
    }

    return priest_spell_t::cost();
  }

  timespan_t execute_time() const override
  {
    if ( priest().buffs.empyreal_blaze->check() )
    {
      return 0_ms;
    }
    return priest_spell_t::execute_time();
  }

  void execute() override
  {
    priest_spell_t::execute();
    if ( priest().talents.holy.empyreal_blaze.enabled() && priest().buffs.empyreal_blaze->check() )
    {
      priest().cooldowns.holy_fire->reset( false );
      priest().buffs.empyreal_blaze->trigger();
    }

    if ( priest().talents.manipulation.enabled() )
    {
      priest().cooldowns.mindgames->adjust( -manipulation_cdr );
    }
  }
};

struct holy_word_chastise_t final : public priest_spell_t
{
  holy_word_chastise_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "holy_word_chastise", p, p.talents.holy.holy_word_chastise )
  {
    parse_options( options_str );
  }
  double cost() const override
  {
    if ( priest().buffs.apotheosis->check() )
    {
      return 0;
    }

    return priest_spell_t::cost();
  }
};

}  // namespace actions::spells

void priest_t::create_buffs_holy()
{
  buffs.apotheosis     = make_buff( this, "apotheosis", talents.holy.apotheosis );
  buffs.empyreal_blaze = make_buff( this, "empyreal_blaze", talents.holy.empyreal_blaze->effectN( 2 ).trigger() )
                             ->set_trigger_spell( talents.holy.empyreal_blaze )
                             ->set_reverse( true )
                             ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
                               if ( new_ > 0 )
                               {
                                 cooldowns.holy_fire->reset( false );
                               }
                             } );
}

void priest_t::init_rng_holy()
{
}

void priest_t::init_spells_holy()
{
  auto ST = [ this ]( std::string_view n ) { return find_talent_spell( talent_tree::SPECIALIZATION, n ); };
  // Row 2
  talents.holy.holy_word_chastise = ST( "Holy Word: Chastise" );
  // Row 3
  talents.holy.empyreal_blaze = ST( "Empyreal Blaze" );
  // Row 4
  talents.holy.searing_light = ST( "Searing Light" );
  // Row 8
  talents.holy.apotheosis = ST( "Apotheosis" );
  // Row 9
  talents.holy.burning_vehemence   = ST( "Burning Vehemence" );
  talents.holy.harmonius_apparatus = ST( "Harmonius Apparatus" );
  talents.holy.light_of_the_naaru  = ST( "Light of the Naaru" );
  // Row 10
  talents.holy.divine_word    = ST( "Divine Word" );
  talents.holy.divine_image   = ST( "Divine Image" );
  talents.holy.miracle_worker = ST( "Miracle Worker" );

  // General Spells
  specs.holy_fire = find_specialization_spell( "Holy Fire" );
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

  if ( name == "holy_word_chastise" )
  {
    return new holy_word_chastise_t( *this, options_str );
  }
  if ( name == "empyreal_blaze" )
  {
    return new empyreal_blaze_t( *this, options_str );
  }

  return nullptr;
}

expr_t* priest_t::create_expression_holy( action_t*, util::string_view /*name_str*/ )
{
  return nullptr;
}

}  // namespace priestspace
