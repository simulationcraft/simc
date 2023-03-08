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
// Penance damage spell
struct penance_damage_t : public priest_spell_t
{
  timespan_t dot_extension;

  penance_damage_t( priest_t& p, util::string_view n, const spell_data_t* s ) : priest_spell_t( n, p, s )
  {
    background = dual = direct_tick = tick_may_crit = may_crit = true;
    dot_extension = priest().talents.discipline.painful_punishment->effectN( 1 ).time_value();

    // This is not found in the affected spells for Shadow Covenant, overriding it manually
    // Final two params allow us to override the 25% damage buff when twilight corruption is selected (25% -> 35%)
    force_buff_effect( p.buffs.shadow_covenant, 1, false, true );
    apply_affecting_aura( priest().talents.discipline.blaze_of_light );
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double d = priest_spell_t::composite_da_multiplier( s );
    if ( priest().buffs.power_of_the_dark_side->check() )
    {
      d *= 1.0 + priest().buffs.power_of_the_dark_side->data().effectN( 1 ).percent();
    }
    return d;
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );
    priest_td_t& td = get_td( s->target );
    td.dots.shadow_word_pain->adjust_duration( dot_extension, true );
    td.dots.purge_the_wicked->adjust_duration( dot_extension, true );
  }
};

// Penance channeled spell
struct penance_channel_t final : public priest_spell_t
{
  penance_channel_t( priest_t& p, util::string_view n, const spell_data_t* s )
    : priest_spell_t( n, p, s ),
      damage( new penance_damage_t( p, "penance_tick", p.specs.penance_tick ) ),
      shadow_covenant_damage( new penance_damage_t( p, "dark_reprimand_tick",
                                                    p.talents.discipline.dark_reprimand->effectN( 2 ).trigger() ) )
  {
    dual = channeled = dynamic_tick_action = direct_tick = true;
    may_miss = may_crit = false;
    add_child( damage );
    add_child( shadow_covenant_damage );
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    if ( p().buffs.shadow_covenant->up() )
    {
      shadow_covenant_damage->execute();
    }
    else
    {
      damage->execute();
    }
  }

  void last_tick( dot_t* d ) override
  {
    priest_spell_t::last_tick( d );
    if ( priest().buffs.harsh_discipline_ready->up() )
    {
      priest().buffs.harsh_discipline_ready->expire();
    }
  }

  void execute() override
  {
    double value = priest().specs.harsh_discipline_value->effectN( 1 ).percent();

    // For some cursed reason tick_time overrides do not work with this spell. Overriding base tick time in execute
    // instead. Ideally someone smarter than me can figure out why.
    base_tick_time = priest().specs.penance_channel->effectN( 2 ).period();

    // Modifying base_tick_time when castigation talent is selected
    base_tick_time *= 1.0 + priest().talents.discipline.castigation->effectN( 1 ).percent();

    // When harsh discipline is talented, and the buff is up, modify the tick time. If castigation is also talented,
    // modify the value and set tick on application to false.
    if ( priest().talents.discipline.harsh_discipline.ok() && priest().buffs.harsh_discipline_ready->up() )
    {
      if ( priest().talents.discipline.castigation.ok() )
      {
        value = value + 0.1;  // Appears to take a 10% hit to the reduction with castigation talented
      }
      base_tick_time *= 1.0 + value;
    }

    double num_ticks = floor( dot_duration / base_tick_time );
    dot_duration     = num_ticks * base_tick_time;
    priest_spell_t::execute();
  }

private:
  propagate_const<action_t*> damage;
  propagate_const<action_t*> shadow_covenant_damage;
};

// Main penance action spell
struct penance_t : public priest_spell_t
{
  penance_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "penance", p, p.specs.penance ),
      channel( new penance_channel_t( p, "penance", p.specs.penance_channel ) ),
      shadow_covenant_channel( new penance_channel_t( p, "dark_reprimand", p.talents.discipline.dark_reprimand ) )
  {
    parse_options( options_str );
    cooldown->duration = p.specs.penance->cooldown();
    apply_affecting_aura( priest().talents.discipline.blaze_of_light );
    add_child( channel );
    add_child( shadow_covenant_channel );
  }

  void execute() override
  {
    priest_spell_t::execute();

    if ( p().buffs.shadow_covenant->up() )
    {
      shadow_covenant_channel->execute();
    }
    else
    {
      channel->execute();
    }
    if ( p().talents.manipulation.ok() )
    {
      p().cooldowns.mindgames->adjust(
          -timespan_t::from_seconds( p().talents.manipulation->effectN( 1 ).base_value() / 2 ) );
    }
  }

private:
  propagate_const<action_t*> channel;
  propagate_const<action_t*> shadow_covenant_channel;
};

// Power Word Solace =========================================================
struct power_word_solace_t final : public priest_spell_t
{
  power_word_solace_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "power_word_solace", p, p.talents.discipline.power_word_solace )
  {
    parse_options( options_str );
    cooldown->hasted = true;
    travel_speed     = 0.0;  // DBC has default travel taking 54 seconds
    apply_affecting_aura( priest().talents.discipline.blaze_of_light );
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );
    double amount = data().effectN( 2 ).percent() / 100.0 * priest().resources.max[ RESOURCE_MANA ];
    priest().resource_gain( RESOURCE_MANA, amount, priest().gains.power_word_solace );

    if ( priest().talents.discipline.harsh_discipline.enabled() )
    {
      priest().buffs.harsh_discipline->increment();
    }
    if ( priest().talents.discipline.train_of_thought.enabled() )
    {
      timespan_t train_of_thought_reduction = priest().talents.discipline.train_of_thought->effectN( 2 ).time_value();
      priest().cooldowns.penance->adjust( train_of_thought_reduction );
    }
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
      // 3% / 5% damage increase
      apply_affecting_aura( priest().talents.throes_of_pain );
      // 5% damage increase
      // TODO: Implement the spreading of Purge the Wicked via penance
      apply_affecting_aura( p.talents.discipline.revel_in_purity );
      // 8% / 15% damage increase
      apply_affecting_aura( priest().talents.discipline.pain_and_suffering );
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
    // 3% / 5% damage increase
    apply_affecting_aura( priest().talents.throes_of_pain );
    // 5% damage increase
    apply_affecting_aura( priest().talents.discipline.revel_in_purity );
    // 8% / 15% damage increase
    apply_affecting_aura( priest().talents.discipline.pain_and_suffering );
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
          ->set_default_value( talents.discipline.shadow_covenant->effectN( 4 ).trigger()->effectN( 1 ).percent() +
                               talents.discipline.twilight_corruption->effectN( 1 ).percent() )
          ->modify_duration( talents.discipline.shadow_covenant->duration() +
                             talents.discipline.embrace_shadow->effectN( 1 ).time_value() )
          ->set_trigger_spell( talents.discipline.shadow_covenant );

  // 280391 has the correct 40% damage increase value, but does not apply it to any spells.
  // 280398 applies the damage to the correct spells, but does not contain the correct value (12% instead of 40%).
  // That 12% represents the damage REDUCTION taken from the 40% buff for each attonement that has been applied.
  // TODO: Add support for atonement reductions
  buffs.sins_of_the_many = make_buff( this, "sins_of_the_many", specs.sins_of_the_many )
                               ->set_default_value( find_spell( 280391 )->effectN( 1 ).percent() );

  buffs.twilight_equilibrium_holy_amp =
      make_buff( this, "twilight_equilibrium_holy_amp", talents.discipline.twilight_equilibrium_holy_amp );

  buffs.twilight_equilibrium_shadow_amp =
      make_buff( this, "twilight_equilibrium_shadow_amp", talents.discipline.twilight_equilibrium_shadow_amp );

  buffs.harsh_discipline = make_buff( this, "harsh_discipline", talents.discipline.harsh_discipline )
                               ->set_max_stack( talents.discipline.harsh_discipline->effectN( 1 ).base_value() )
                               ->set_stack_change_callback( [ this ]( buff_t*, int, int ) {
                                 if ( buffs.harsh_discipline->at_max_stacks() )
                                 {
                                   // The stacks only expire when creating the harsh discipline ready buff, otherwise
                                   // they persist at max
                                   if ( !buffs.harsh_discipline_ready->up() )
                                   {
                                     buffs.harsh_discipline_ready->trigger();
                                     buffs.harsh_discipline->expire();
                                   }
                                 }
                               } );

  buffs.harsh_discipline_ready = make_buff( this, "harsh_discipline_ready", find_spell( 373183 ) );
  buffs.borrowed_time = make_buff( this, "borrowed_time", find_spell( 390692 ) )->set_trigger_spell( find_spell( 17 ) );
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
  talents.discipline.embrace_shadow      = ST( "Embrace Shadow" );
  talents.discipline.twilight_corruption = ST( "Twilight Corruption" );
  talents.discipline.revel_in_purity     = ST( "Revel in Purity" );
  talents.discipline.pain_and_suffering  = ST( "Pain and Suffering" );
  // Row 7
  talents.discipline.castigation   = ST( "Castigation" );
  talents.discipline.borrowed_time = ST( "Borrowed Time" );
  // Row 8
  talents.discipline.lights_wrath     = ST( "Light's Wrath" );
  talents.discipline.train_of_thought = ST( "Train of Thought" );  // TODO: implement PS:S reduction as well
  // Row 9
  talents.discipline.harsh_discipline       = ST( "Harsh Discipline" );
  talents.discipline.harsh_discipline_ready = find_spell( 373183 );
  talents.discipline.blaze_of_light         = find_spell( 215768 );
  // Row 10
  talents.discipline.twilight_equilibrium            = ST( "Twilight Equilibrium" );
  talents.discipline.twilight_equilibrium_holy_amp   = find_spell( 390706 );
  talents.discipline.twilight_equilibrium_shadow_amp = find_spell( 390707 );

  // General Spells
  specs.sins_of_the_many       = find_spell( 280398 );
  specs.penance                = find_spell( 47540 );
  specs.penance_channel        = find_spell( 47758 );  // Channel spell, triggered by 47540, executes 47666 every tick
  specs.penance_tick           = find_spell( 47666 );  // Not triggered from 47540, only 47758
  specs.harsh_discipline_value = find_spell( 373183 );
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
