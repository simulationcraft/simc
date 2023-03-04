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

// ==========================================================================
// Penance & Dark Reprimand
// Penance:
// - Base(47540) ? (47758) -> (47666)
// Dark Reprimand:
// - Base(373129) -> (373130)
// ==========================================================================
struct penance_base_t final : public priest_spell_t
{
  struct penance_tick_t final : public priest_spell_t
  {
    timespan_t dot_extension;

    penance_tick_t( util::string_view n, priest_t& p, stats_t* stats, const spell_data_t* s )
      : priest_spell_t( n, p, s )
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
      priest_spell_t::impact( s );
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

  penance_base_t( util::string_view n, priest_t& p, const spell_data_t* s, const spell_data_t* ts,
                  util::string_view tn )
    : priest_spell_t( n, p, s ), penance_tick_action( new penance_tick_t( tn, p, stats, ts ) )
  {
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
  }
};

struct penance_t final : public priest_spell_t
{
  timespan_t manipulation_cdr;

  penance_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "penance", p, p.specs.penance ),
      _base_spell( new penance_base_t( "penance", p, p.specs.penance, p.specs.penance_tick, "penance_tick" ) ),
      _dark_reprimand_spell( new penance_base_t( "dark_reprimand", p, p.talents.discipline.dark_reprimand,
                                                 p.talents.discipline.dark_reprimand->effectN( 2 ).trigger(),
                                                 "dark_reprimand_tick" ) ),
      manipulation_cdr( timespan_t::from_seconds( priest().talents.manipulation->effectN( 1 ).base_value() / 2 ) )
  {
    parse_options( options_str );

    add_child( _base_spell );
  }

  void execute() override
  {
    if ( priest().talents.manipulation.enabled() )
    {
      priest().cooldowns.mindgames->adjust( -manipulation_cdr );
    }

    if ( priest().buffs.shadow_covenant->check() )
    {
      _dark_reprimand_spell->execute();
      // TODO: figure out why the cooldown isn't starting automatically
      priest().cooldowns.penance->start();
    }
    else
    {
      _base_spell->execute();
    }
  }

private:
  propagate_const<action_t*> _base_spell;
  propagate_const<action_t*> _dark_reprimand_spell;
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
  struct purge_the_wicked_dot_t final : public priest_spell_t
  {
    // Manually create the dot effect because "ticking" is not present on
    // primary spell
    purge_the_wicked_dot_t( priest_t& p )
      : priest_spell_t( "purge_the_wicked", p, p.talents.discipline.purge_the_wicked->effectN( 2 ).trigger() )
    {
      background = true;
      apply_affecting_aura( priest().talents.throes_of_pain );
    }

    void tick( dot_t* d ) override
    {
      priest_spell_t::tick( d );

      if ( d->state->result_amount > 0 )
      {
        trigger_power_of_the_dark_side();
      }
    }
  };

  purge_the_wicked_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "purge_the_wicked", p, p.talents.discipline.purge_the_wicked )
  {
    parse_options( options_str );
    tick_zero      = false;
    execute_action = new purge_the_wicked_dot_t( p );
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
      timespan_t schism_duration =
          td.buffs.schism->data().duration() + priest().talents.discipline.malicious_intent->effectN( 1 ).time_value();
      td.buffs.schism->trigger( schism_duration );
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

  buffs.shadow_covenant =
      make_buff( this, "shadow_covenant", talents.discipline.shadow_covenant->effectN( 4 ).trigger() )
          ->set_trigger_spell( talents.discipline.shadow_covenant );

  // 280391 has the correct 40% damage increase value, but does not apply it to any spells.
  // 280398 applies the damage to the correct spells, but does not contain the correct value (12% instead of 40%).
  // That 12% represents the damage REDUCTION taken from the 40% buff for each attonement that has been applied.
  buffs.sins_of_the_many = make_buff( this, "sins_of_the_many", specs.sins_of_the_many )
                               ->set_default_value( find_spell( 280391 )->effectN( 1 ).percent() );
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
  talents.discipline.schism          = ST( "Schism" );
  talents.discipline.dark_indulgence = ST( "Dark Indulgence" );
  // Row 4
  talents.discipline.malicious_intent   = ST( "Malicious Intent" );
  talents.discipline.power_word_solace  = ST( "Power Word: Solace" );
  talents.discipline.painful_punishment = ST( "Painful Punishment" );
  // Row 5
  talents.discipline.purge_the_wicked = ST( "Purge the Wicked" );
  talents.discipline.shadow_covenant  = ST( "Shadow Covenant" );
  talents.discipline.dark_reprimand   = find_spell( 373129 );
  // Row 6
  // Row 7
  // Row 8
  talents.discipline.lights_wrath = ST( "Light's Wrath" );
  // Row 9
  // Row 10

  talents.castigation   = find_talent_spell( "Castigation" );
  talents.shining_force = find_talent_spell( "Shining Force" );

  // General Spells
  specs.sins_of_the_many = find_spell( 280398 );
  specs.penance          = find_spell( 47540 );
  specs.penance_tick     = find_spell( 47666 );  // Not triggered from 47540, only 47758
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
