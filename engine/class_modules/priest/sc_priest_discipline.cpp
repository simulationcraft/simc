// ==========================================================================
// Discipline Priest Sim File
// Contact: https://github.com/orgs/simulationcraft/teams/priest/members
// Wiki: https://github.com/simulationcraft/simc/wiki/Priests
// ==========================================================================

#include "sc_enums.hpp"
#include "sc_priest.hpp"

#include "simulationcraft.hpp"

namespace priestspace
{
namespace actions::spells
{
struct pain_suppression_t final : public priest_spell_t
{
  pain_suppression_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "pain_suppression", p, p.find_class_spell( "Pain Suppression" ) )
  {
    parse_options( options_str );

    // If we don't specify a target, it's defaulted to the mob, so default to the player instead
    if ( target->is_enemy() || target->is_add() )
    {
      target = &p;
    }

    harmful = false;
  }

  void execute() override
  {
    priest_spell_t::execute();

    target->buffs.pain_suppression->trigger();
  }
};

// Penance damage spell
struct penance_t final : public priest_spell_t
{
  struct penance_tick_t final : public priest_spell_t
  {
    timespan_t dot_extension;

    penance_tick_t( priest_t& p, stats_t* stats ) : priest_spell_t( "penance_tick", p, p.dbc->spell( 47666 ) )
    {
      background    = true;
      dual          = true;
      direct_tick   = true;
      tick_may_crit = true;
      may_crit      = true;
      dot_extension = priest().talents.discipline.painful_punishment->effectN( 1 ).time_value();
      this->stats   = stats;
    }

    void impact( action_state_t* s ) override
    {
      priest_spell_t::execute();
      priest_td_t& td = get_td( s->target );
      td.dots.shadow_word_pain->adjust_duration( dot_extension, true );
      td.dots.purge_the_wicked->adjust_duration( dot_extension, true );
    }

    bool verify_actor_level() const override
    {
      // Tick spell data is restricted to level 60+, even though main spell is level 10+
      return true;
    }
  };

  penance_tick_t* penance_tick_action;
  timespan_t manipulation_cdr;

  penance_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "penance", p, p.find_class_spell( "Penance" ) ),
      penance_tick_action( new penance_tick_t( p, stats ) ),
      manipulation_cdr( timespan_t::from_seconds( priest().talents.manipulation->effectN( 1 ).base_value() / 2 ) )

  {
    parse_options( options_str );

    may_crit       = false;
    may_miss       = false;
    channeled      = true;
    tick_zero      = true;
    dot_duration   = timespan_t::from_seconds( 2.0 );
    base_tick_time = timespan_t::from_seconds( 1.0 );

    if ( priest().talents.castigation->ok() )
    {
      // Add 1 extra millisecond, so we only get 4 ticks instead of an extra tiny 5th tick.
      base_tick_time = timespan_t::from_seconds( 2.0 / 3 ) + timespan_t::from_millis( 1 );
    }

    dynamic_tick_action = true;
    tick_action         = penance_tick_action;
  }

  double bonus_da( const action_state_t* state ) const override
  {
    double d = priest_spell_t::bonus_da( state );
    if ( priest().buffs.power_of_the_dark_side->check() )
    {
      d *= 1.0 + priest().buffs.power_of_the_dark_side->data().effectN( 1 ).percent();
    }
    return d;
  }

  void last_tick( dot_t* d ) override
  {
    priest_spell_t::last_tick( d );
    priest().buffs.power_of_the_dark_side->expire();
  }

  void execute() override
  {
    priest_spell_t::execute();
    priest().buffs.power_of_the_dark_side->up();  // benefit tracking
    if ( priest().talents.manipulation.enabled() )
    {
      priest().cooldowns.mindgames->adjust( -manipulation_cdr );
    }
  }
};

struct power_word_solace_t final : public priest_spell_t
{
  power_word_solace_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "power_word_solace", p, p.talents.discipline.power_word_solace )
  {
    parse_options( options_str );
    cooldown->hasted = true;
    travel_speed     = 0.0;  // DBC has default travel taking 54 seconds
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );
    double amount = data().effectN( 2 ).percent() / 100.0 * priest().resources.max[ RESOURCE_MANA ];
    priest().resource_gain( RESOURCE_MANA, amount, priest().gains.power_word_solace );
  }
};

// Purge the wicked
struct purge_the_wicked_t final : public priest_spell_t
{
  purge_the_wicked_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "purge_the_wicked", p, p.talents.discipline.purge_the_wicked )
  {
    parse_options( options_str );
    may_crit  = true;
    tick_zero = false;

    // Throes of Pain: Spell Direct and Periodic 3%/5% gain
    apply_affecting_aura( priest().talents.throes_of_pain );
    apply_affecting_aura( priest().talents.discipline.painful_punishment );
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    if ( result_is_hit( d->state->result ) && d->state->result_amount > 0 )
    {
      trigger_power_of_the_dark_side();
    }
  }
};

struct schism_t final : public priest_spell_t
{
  schism_t( priest_t& p, util::string_view options_str ) : priest_spell_t( "schism", p, p.talents.discipline.schism )
  {
    parse_options( options_str );
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      priest_td_t& td = get_td( s->target );

      // Assumption scamille 2016-07-28: Schism only applied on hit
      td.buffs.schism->trigger();
    }
  }
};

// Heal allies effect not implemented
struct shadow_covenant_t final : public priest_spell_t
{
  shadow_covenant_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "shadow_covenant", p, p.talents.discipline.shadow_covenant )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().buffs.shadow_covenant->trigger();
  }
};

struct lights_wrath_t final : public priest_spell_t
{
  lights_wrath_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "lights_wrath", p, p.talents.discipline.lights_wrath )
  {
    parse_options( options_str );
  }
};

}  // namespace actions::spells

namespace buffs
{
}  // namespace buffs

void priest_t::create_buffs_discipline()
{
  buffs.power_of_the_dark_side =
      make_buff( this, "power_of_the_dark_side", talents.discipline.power_of_the_dark_side->effectN( 1 ).trigger() )
          ->set_trigger_spell( talents.discipline.power_of_the_dark_side );

  buffs.sins_of_the_many = make_buff( this, "sins_of_the_many", talents.sins_of_the_many->effectN( 1 ).trigger() )
                               ->set_default_value( talents.sins_of_the_many->effectN( 1 ).percent() )
                               ->set_duration( talents.sins_of_the_many->duration() );

  buffs.shadow_covenant =
      make_buff( this, "shadow_covenant", talents.discipline.shadow_covenant->effectN( 4 ).trigger() )
          ->set_trigger_spell( talents.discipline.shadow_covenant );
}

void priest_t::init_rng_discipline()
{
}

void priest_t::init_spells_discipline()
{
  auto ST = [ this ]( std::string_view n ) { return find_talent_spell( talent_tree::SPECIALIZATION, n ); };

  // Talents
  // Row 2
  talents.discipline.power_of_the_dark_side = ST( "Power of the Dark Side" );
  // Row 3
  talents.discipline.schism = ST( "Schism" );
  // Row 4
  talents.discipline.power_word_solace  = ST( "Power Word: Solace" );
  talents.discipline.painful_punishment = ST( "Painful Punishment" );
  // Row 5
  talents.discipline.purge_the_wicked = ST( "Purge the Wicked" );
  talents.discipline.shadow_covenant  = ST( "Shadow Covenant" );
  // Row 6
  // Row 7
  // Row 8
  talents.discipline.lights_wrath = ST( "Light's Wrath" );
  // Row 9
  // Row 10

  talents.castigation      = find_talent_spell( "Castigation" );
  talents.shining_force    = find_talent_spell( "Shining Force" );
  talents.sins_of_the_many = find_talent_spell( "Sins of the Many" );
}

action_t* priest_t::create_action_discipline( util::string_view name, util::string_view options_str )
{
  using namespace actions::spells;

  if ( name == "pain_suppression" )
  {
    return new pain_suppression_t( *this, options_str );
  }
  if ( name == "penance" )
  {
    return new penance_t( *this, options_str );
  }
  if ( name == "power_word_solace" )
  {
    return new power_word_solace_t( *this, options_str );
  }
  if ( name == "schism" )
  {
    return new schism_t( *this, options_str );
  }
  if ( name == "purge_the_wicked" )
  {
    return new purge_the_wicked_t( *this, options_str );
  }
  if ( name == "shadow_covenant" )
  {
    return new shadow_covenant_t( *this, options_str );
  }
  if ( name == "lights_wrath" )
  {
    return new lights_wrath_t( *this, options_str );
  }

  return nullptr;
}

std::unique_ptr<expr_t> priest_t::create_expression_discipline( action_t*, util::string_view /*name_str*/ )
{
  return {};
}

}  // namespace priestspace
