// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "player/covenant.hpp"
#include "sc_enums.hpp"
#include "sc_priest.hpp"

#include "simulationcraft.hpp"

namespace priestspace
{
namespace actions
{
namespace spells
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

/// Penance damage spell
struct penance_t final : public priest_spell_t
{
  struct penance_tick_t final : public priest_spell_t
  {
    bool first_tick;
    const conduit_data_t swift_penitence;

    penance_tick_t( priest_t& p, stats_t* stats )
      : priest_spell_t( "penance_tick", p, p.dbc->spell( 47666 ) ),
        first_tick( false ),
        swift_penitence( p.find_conduit_spell( "Swift Penitence" ) )
    {
      background  = true;
      dual        = true;
      direct_tick = true;

      this->stats = stats;
    }

    double action_da_multiplier() const override
    {
      double m = priest_spell_t::action_da_multiplier();

      if ( first_tick && swift_penitence.ok() )
      {
        m *= 1 + swift_penitence.percent();
      }

      return m;
    }

    void execute() override
    {
      priest_spell_t::execute();

      first_tick = false;
    }

    bool verify_actor_level() const override
    {
      // Tick spell data is restricted to level 60+, even though main spell is level 10+
      return true;
    }
  };

  penance_tick_t* penance_tick_action;

  penance_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "penance", p, p.find_class_spell( "Penance" ) ),
      penance_tick_action( new penance_tick_t( p, stats ) )
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

  double cost() const override
  {
    auto cost = base_t::cost();
    if ( priest().buffs.the_penitent_one->check() )
    {
      cost *= ( 100 + priest().buffs.the_penitent_one->data().effectN( 2 ).base_value() ) / 100.0;
    }
    return cost;
  }

  timespan_t tick_time( const action_state_t* ) const override
  {
    // Do not haste ticks!
    auto tt = base_tick_time;
    if ( priest().buffs.the_penitent_one->check() )
    {
      tt *= ( 100 + priest().buffs.the_penitent_one->data().effectN( 1 ).base_value() ) / 100.0;
    }
    return tt;
  }

  void last_tick( dot_t* d ) override
  {
    priest_spell_t::last_tick( d );

    priest().buffs.power_of_the_dark_side->expire();
    priest().buffs.the_penitent_one->decrement();
  }

  void execute() override
  {
    penance_tick_action->first_tick = true;

    priest_spell_t::execute();

    priest().buffs.the_penitent_one->up();        // benefit tracking
    priest().buffs.power_of_the_dark_side->up();  // benefit tracking
  }
};

struct power_word_solace_t final : public priest_spell_t
{
  power_word_solace_t( priest_t& player, util::string_view options_str )
    : priest_spell_t( "power_word_solace", player, player.talents.power_word_solace )
  {
    parse_options( options_str );

    cooldown->hasted = true;

    travel_speed = 0.0;  // DBC has default travel taking 54seconds.....
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    double amount = data().effectN( 2 ).percent() / 100.0 * priest().resources.max[ RESOURCE_MANA ];
    priest().resource_gain( RESOURCE_MANA, amount, priest().gains.power_word_solace );
  }
};

struct purge_the_wicked_t final : public priest_spell_t
{
  struct purge_the_wicked_dot_t final : public priest_spell_t
  {
    purge_the_wicked_dot_t( priest_t& p )
      : priest_spell_t( "purge_the_wicked", p, p.talents.purge_the_wicked->effectN( 2 ).trigger() )
    {
      may_crit = true;
      // tick_zero = false;
      energize_type = action_energize::NONE;  // disable resource generation from spell data
      background    = true;
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
    : priest_spell_t( "purge_the_wicked", p, p.talents.purge_the_wicked )
  {
    parse_options( options_str );

    may_crit       = true;
    energize_type  = action_energize::NONE;  // disable resource generation from spell data
    execute_action = new purge_the_wicked_dot_t( p );
  }
};

struct schism_t final : public priest_spell_t
{
  schism_t( priest_t& player, util::string_view options_str )
    : priest_spell_t( "schism", player, player.talents.schism )
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
  shadow_covenant_t( priest_t& player, util::string_view options_str )
    : priest_spell_t( "shadow_covenant", player, player.talents.shadow_covenant )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().buffs.shadow_covenant->trigger();
  }
};

// Implemented as a dummy effect, without providing absorbs
struct spirit_shell_t final : public priest_spell_t
{
  spirit_shell_t( priest_t& player, util::string_view options_str )
    : priest_spell_t( "spirit_shell", player, player.talents.spirit_shell )
  {
    parse_options( options_str );

    harmful = false;
  }

  virtual void execute() override
  {
    priest_spell_t::execute();

    priest().buffs.spirit_shell->trigger();
  }
};

}  // namespace spells

}  // namespace actions

namespace buffs
{
}  // namespace buffs

void priest_t::create_buffs_discipline()
{
  buffs.power_of_the_dark_side =
      make_buff( this, "power_of_the_dark_side", specs.power_of_the_dark_side->effectN( 1 ).trigger() )
          ->set_trigger_spell( specs.power_of_the_dark_side );

  buffs.sins_of_the_many = make_buff( this, "sins_of_the_many", talents.sins_of_the_many->effectN( 1 ).trigger() )
                               ->set_default_value( talents.sins_of_the_many->effectN( 1 ).percent() )
                               ->set_duration( talents.sins_of_the_many->duration() );

  buffs.shadow_covenant = make_buff( this, "shadow_covenant", talents.shadow_covenant->effectN( 4 ).trigger() )
                              ->set_trigger_spell( talents.shadow_covenant );

  buffs.spirit_shell = make_buff( this, "spirit_shell", talents.spirit_shell );
}

void priest_t::init_rng_discipline()
{
}

void priest_t::init_spells_discipline()
{
  // Talents
  // T15
  talents.castigation   = find_talent_spell( "Castigation" );
  talents.twist_of_fate = find_talent_spell( "Twist of Fate" );
  talents.schism        = find_talent_spell( "Schism" );
  // T25
  talents.body_and_soul   = find_talent_spell( "Body and Soul" );
  talents.masochism       = find_talent_spell( "Masochism" );
  talents.angelic_feather = find_talent_spell( "Angelic Feather" );
  // T30
  talents.shield_discipline = find_talent_spell( "Shield Discipline" );
  talents.mindbender        = find_talent_spell( "Mindbender" );
  talents.power_word_solace = find_talent_spell( "Power Word: Solace" );
  // T35
  talents.psychic_voice = find_talent_spell( "Psychic Voice" );
  talents.dominant_mind = find_talent_spell( "Dominant Mind" );
  talents.shining_force = find_talent_spell( "Shining Force" );
  // T40
  talents.sins_of_the_many = find_talent_spell( "Sins of the Many" );
  talents.contrition       = find_talent_spell( "Contrition" );
  talents.shadow_covenant  = find_talent_spell( "Shadow Covenant" );
  // T45
  talents.purge_the_wicked = find_talent_spell( "Purge the Wicked" );
  talents.divine_star      = find_talent_spell( "Divine Star" );
  talents.halo             = find_talent_spell( "Halo" );
  // T50
  talents.lenience     = find_talent_spell( "Lenience" );
  talents.spirit_shell = find_talent_spell( "Spirit Shell" );
  talents.evangelism   = find_talent_spell( "Evangelism" );

  // Passive spell data
  specs.discipline_priest      = find_specialization_spell( "Discipline Priest" );
  specs.power_of_the_dark_side = find_specialization_spell( "Power of the Dark Side" );
}

action_t* priest_t::create_action_discipline( util::string_view name, util::string_view options_str )
{
  using namespace actions::spells;
  using namespace actions::heals;

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
  if ( name == "spirit_shell" )
  {
    return new spirit_shell_t( *this, options_str );
  }

  return nullptr;
}

std::unique_ptr<expr_t> priest_t::create_expression_discipline( action_t*, util::string_view /*name_str*/ )
{
  return {};
}

void priest_t::generate_apl_discipline_h()
{
  action_priority_list_t* def = get_action_priority_list( "default" );

  // DEFAULT
  if ( sim->allow_potions )
  {
    def->add_action( "mana_potion,if=mana.pct<=75" );
  }

  if ( find_class_spell( "Shadowfiend" )->ok() )
  {
    def->add_action( this, "Shadowfiend" );
  }

  if ( race == RACE_TROLL )
  {
    def->add_action( "berserking" );
  }
  if ( race == RACE_BLOOD_ELF )
  {
    def->add_action( "arcane_torrent,if=mana.pct<=90" );
  }
  def->add_action( this, "Penance" );
  def->add_action( this, "Shadow Mend" );
}

/** Discipline Damage Combat Action Priority List */
void priest_t::generate_apl_discipline_d()
{
  action_priority_list_t* def     = get_action_priority_list( "default" );
  action_priority_list_t* boon    = get_action_priority_list( "boon" );
  action_priority_list_t* racials = get_action_priority_list( "racials" );

  boon->add_action( "ascended_blast" );
  boon->add_action( "ascended_nova" );

  // On-Use Items
  def->add_action( "use_items", "Default fallback for usable items: Use on cooldown in order by trinket slot." );

  // Potions
  if ( sim->allow_potions )
  {
    def->add_action( "potion,if=buff.bloodlust.react|buff.power_infusion.up|target.time_to_die<=40",
                     "Sync potion usage with Bloodlust or Power Infusion." );
  }

  // Racials
  racials->add_action( "arcane_torrent,if=mana.pct<=95" );

  if ( race != RACE_BLOOD_ELF )
  {
    for ( const auto& racial_action : get_racial_actions() )
    {
      racials->add_action( racial_action );
    }
  }

  def->add_call_action_list( racials );
  def->add_action( this, "Power Infusion", "",
                   "Use Power Infusion before Shadow Covenant to make sure we don't lock out our CD." );
  def->add_talent( this, "Divine Star" );
  def->add_talent( this, "Halo" );
  def->add_action( this, "Penance" );
  def->add_talent( this, "Power Word: Solace" );
  def->add_talent( this, "Shadow Covenant",
                   "if=!covenant.kyrian|(!cooldown.boon_of_the_ascended.up&!buff.boon_of_the_ascended.up)",
                   "Hold Shadow Covenant if Boon of the Ascended cooldown is up or the buff is active." );
  def->add_talent( this, "Schism" );
  def->add_action( this, covenant.mindgames, "mindgames" );
  def->add_action( this, covenant.fae_guardians, "fae_guardians" );
  def->add_action( this, covenant.unholy_nova, "unholy_nova" );
  def->add_action( this, covenant.boon_of_the_ascended, "boon_of_the_ascended" );
  def->add_call_action_list( this, covenant.boon_of_the_ascended, boon, "if=buff.boon_of_the_ascended.up" );
  def->add_action( "mindbender" );
  def->add_talent( this, "Spirit Shell" );
  def->add_talent( this, "Purge the Wicked", "if=!ticking" );
  def->add_action( this, "Shadow Word: Pain", "if=!ticking&!talent.purge_the_wicked.enabled" );
  def->add_action( this, "Shadow Word: Death" );
  def->add_action( this, "Mind Blast" );
  def->add_talent( this, "Purge the Wicked", "if=refreshable" );
  def->add_action( this, "Shadow Word: Pain", "if=refreshable&!talent.purge_the_wicked.enabled" );
  def->add_action( this, "Smite", "if=spell_targets.holy_nova<3", "Use Smite on up to 2 targets." );
  def->add_action( this, "Holy Nova", "if=spell_targets.holy_nova>=3" );
  def->add_action( this, "Shadow Word: Pain" );
}

}  // namespace priestspace
