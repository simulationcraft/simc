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
struct pain_suppression_t final : public priest_spell_t
{
  pain_suppression_t( priest_t& p, const std::string& options_str )
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

    target->buffs.pain_supression->trigger();
  }
};

/// Penance damage spell
struct penance_t final : public priest_spell_t
{
  struct penance_tick_t final : public priest_spell_t
  {
    penance_tick_t( priest_t& p, stats_t* stats ) : priest_spell_t( "penance_tick", p, p.dbc.spell( 47666 ) )
    {
      background  = true;
      dual        = true;
      direct_tick = true;

      this->stats = stats;
    }

    bool verify_actor_level() const override
    {
      // Tick spell data is restricted to level 60+, even though main spell is level 10+
      return true;
    }
  };

  penance_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "penance", p, p.find_class_spell( "Penance" ) )
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
    dot_duration += priest().sets->set( PRIEST_DISCIPLINE, T17, B2 )->effectN( 1 ).time_value();

    dynamic_tick_action = true;
    tick_action         = new penance_tick_t( p, stats );
  }

  double bonus_da( const action_state_t* state ) const override
  {
    double d = priest_spell_t::bonus_da( state );

    if ( priest().buffs.power_of_the_dark_side->check() )
    {
      d *= 1.0 + priest().specs.power_of_the_dark_side->effectN( 1 ).percent();
    }

    return d;
  }

  timespan_t tick_time( const action_state_t* ) const override
  {
    // Do not haste ticks!
    return base_tick_time;
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

    // re-checked 2014/07/07: offensive penance grants evangelism stacks, even though not mentioned in the tooltip.
    priest().buffs.holy_evangelism->trigger();

    // Set bonus says "...and generates 2 stacks of Evangelism." Need to check if this happens up front or after a
    // particular tick. -Twintop 2014-07-11
    if ( priest().sets->has_set_bonus( PRIEST_DISCIPLINE, T17, B2 ) )
    {
      priest().buffs.holy_evangelism->trigger();
    }
  }
};

// ==========================================================================
// Shadow Word: Pain
// ==========================================================================
struct shadow_word_pain_disc_t final : public priest_spell_t
{
  double insanity_gain;
  bool casted;

  shadow_word_pain_disc_t( priest_t& p, const std::string& options_str, bool _casted = true )
    : priest_spell_t( "shadow_word_pain", p, p.find_class_spell( "Shadow Word: Pain" ) ),
      insanity_gain( data().effectN( 3 ).resource( RESOURCE_INSANITY ) )
  {
    parse_options( options_str );
    casted    = _casted;
    may_crit  = true;
    tick_zero = false;
    if ( !casted )
    {
      base_dd_max = 0.0;
      base_dd_min = 0.0;
    }
    energize_type = ENERGIZE_NONE;  // disable resource generation from spell data
  }

  double spell_direct_power_coefficient( const action_state_t* s ) const override
  {
    return casted ? priest_spell_t::spell_direct_power_coefficient( s ) : 0.0;
  }

  double bonus_ta( const action_state_t* state ) const override
  {
    double d = priest_spell_t::bonus_ta( state );

    if ( priest().azerite.death_throes.enabled() )
    {
      d += priest().azerite.death_throes.value( 1 );
    }

    return d;
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    if ( d->state->result_amount > 0 )
    {
      if ( priest().rppm.power_of_the_dark_side->trigger() )
      {
        trigger_power_of_the_dark_side();
      }
    }
  }
};

struct power_word_solace_t final : public priest_spell_t
{
  power_word_solace_t( priest_t& player, const std::string& options_str )
    : priest_spell_t( "power_word_solace", player, player.find_spell( 129250 ) )
  {
    parse_options( options_str );

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
      energize_type = ENERGIZE_NONE;  // disable resource generation from spell data
      background    = true;
    }

    void tick( dot_t* d ) override
    {
      priest_spell_t::tick( d );

      if ( d->state->result_amount > 0 )
      {
        if ( priest().rppm.power_of_the_dark_side->trigger() )
        {
          trigger_power_of_the_dark_side();
        }
      }
    }
  };

  purge_the_wicked_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "purge_the_wicked", p, p.talents.purge_the_wicked )
  {
    parse_options( options_str );

    may_crit       = true;
    energize_type  = ENERGIZE_NONE;  // disable resource generation from spell data
    execute_action = new purge_the_wicked_dot_t( p );
  }
};

struct schism_t final : public priest_spell_t
{
  schism_t( priest_t& player, const std::string& options_str )
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

}  // namespace spells

}  // namespace actions

namespace buffs
{
/**
 * Custom archangel buff
 * snapshots evangelism stacks and expires it
 */
struct archangel_t final : public priest_buff_t<buff_t>
{
  archangel_t( priest_t& p ) : base_t( p, "archangel", p.specs.archangel )
  {
    set_max_stack( 5 );
    default_value = data().effectN( 1 ).percent();
  }

  bool trigger( int stacks, double /* value */, double chance, timespan_t duration ) override
  {
    stacks                 = priest().buffs.holy_evangelism->stack();
    double archangel_value = default_value * stacks;
    bool success           = base_t::trigger( stacks, archangel_value, chance, duration );

    priest().buffs.holy_evangelism->expire();

    return success;
  }
};

}  // namespace buffs

void priest_t::create_buffs_discipline()
{
  // Discipline
  buffs.archangel = new buffs::archangel_t( *this );

  buffs.borrowed_time = make_buff( this, "borrowed_time", find_spell( 59889 ) )
                            ->set_chance( specs.borrowed_time->ok() )
                            ->set_default_value( find_spell( 59889 )->effectN( 1 ).percent() )
                            ->add_invalidate( CACHE_HASTE );

  buffs.holy_evangelism = make_buff( this, "holy_evangelism", find_spell( 81661 ) )
                              ->set_chance( specs.evangelism->ok() )
                              ->set_activated( false );

  buffs.power_of_the_dark_side =
      make_buff( this, "power_of_the_dark_side", find_specialization_spell( "Power of the Dark Side" ) )
          ->set_duration( find_spell( 198069 )->duration() )
          ->set_default_value( find_spell( 198069 )
                                   ->effectN( 1 )
                                   .percent() );  // Power of the Dark Side has 2 spell IDs, this one is for the damage.

  buffs.sins_of_the_many = make_buff( this, "sins_of_the_many", talents.sins_of_the_many->effectN( 1 ).trigger() )
                               ->set_default_value( talents.sins_of_the_many->effectN( 1 ).percent() )
                               ->set_duration( talents.sins_of_the_many->duration() );
}

void priest_t::init_rng_discipline()
{
  rppm.power_of_the_dark_side = get_rppm( "Power of the Dark Side", find_spell( 198068 ) );
}

void priest_t::init_spells_discipline()
{
  // Talents
  // T15
  talents.castigation   = find_talent_spell( "Castigation" );
  talents.twist_of_fate = find_talent_spell( "Twist of Fate" );
  talents.schism        = find_talent_spell( "Schism" );
  // T30
  talents.angelic_feather = find_talent_spell( "Angelic Feather" );
  talents.body_and_soul   = find_talent_spell( "Body and Soul" );
  talents.masochism       = find_talent_spell( "Masochism" );
  // T45
  talents.power_word_solace = find_talent_spell( "Power Word: Solace" );
  talents.shield_discipline = find_talent_spell( "Shield Discipline" );
  talents.mindbender        = find_talent_spell( "Mindbender" );
  // T60
  talents.psychic_voice = find_talent_spell( "Psychic Voice" );
  talents.shining_force = find_talent_spell( "Shining Force" );
  talents.dominant_mind = find_talent_spell( "Dominant Mind" );
  // T75
  talents.sanctuary        = find_talent_spell( "Sanctuary" );
  talents.sins_of_the_many = find_talent_spell( "Sins of the Many" );
  talents.clarity_of_will  = find_talent_spell( "Clarity of Will" );
  talents.shadow_covenant  = find_talent_spell( "Shadow Covenant" );
  // T90
  talents.purge_the_wicked = find_talent_spell( "Purge the Wicked" );
  talents.divine_star      = find_talent_spell( "Divine Star" );
  talents.halo             = find_talent_spell( "Halo" );
  // T100
  talents.grace          = find_talent_spell( "Grace" );
  talents.evangelism     = find_talent_spell( "Evangelism" );

  // General Spells
  specs.priest                 = dbc::get_class_passive( *this, SPEC_NONE );
  specs.holy                   = dbc::get_class_passive( *this, PRIEST_HOLY );
  specs.discipline             = dbc::get_class_passive( *this, PRIEST_DISCIPLINE );
  specs.shadow                 = dbc::get_class_passive( *this, PRIEST_SHADOW );
  specs.atonement              = find_specialization_spell( "Atonement" );
  specs.archangel              = find_specialization_spell( "Archangel" );
  specs.borrowed_time          = find_specialization_spell( "Borrowed Time" );
  specs.divine_aegis           = find_specialization_spell( "Divine Aegis" );
  specs.evangelism             = find_specialization_spell( "Evangelism" );
  specs.grace                  = find_specialization_spell( "Grace" );
  specs.mysticism              = find_specialization_spell( "Mysticism" );
  specs.spirit_shell           = find_specialization_spell( "Spirit Shell" );
  specs.enlightenment          = find_specialization_spell( "Enlightenment" );
  specs.discipline_priest      = find_specialization_spell( "Discipline Priest" );
  specs.power_of_the_dark_side = find_spell( 198069 );  // Damage ID of Power of the Dark Side

  // Azerite
  azerite.death_throes = find_azerite_spell( "Death Throes" );

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

/**
 * Copy stats from the trigger spell to the atonement spell
 * to get proper HPR and HPET reports.
 */
void priest_t::fixup_atonement_stats( const std::string& trigger_spell_name, const std::string& atonement_spell_name )
{
  if ( stats_t* trigger = find_stats( trigger_spell_name ) )
  {
    if ( stats_t* atonement = get_stats( atonement_spell_name ) )
    {
      atonement->resource_gain.merge( trigger->resource_gain );
      atonement->total_execute_time = trigger->total_execute_time;
      atonement->total_tick_time    = trigger->total_tick_time;
      atonement->num_ticks          = trigger->num_ticks;
    }
  }
}

action_t* priest_t::create_action_discipline( const std::string& name, const std::string& options_str )
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
  if ( ( name == "shadow_word_pain" ) || ( name == "purge_the_wicked" ) )
  {
    if ( talents.purge_the_wicked->ok() )
    {
      return new purge_the_wicked_t( *this, options_str );
    }
    else
    {
      return new shadow_word_pain_disc_t( *this, options_str );
    }
  }

  return nullptr;
}

expr_t* priest_t::create_expression_discipline( action_t*, const std::string& /*name_str*/ )
{
  return nullptr;
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
  action_priority_list_t* def = get_action_priority_list( "default" );

  // On-Use Items
  for ( const std::string& item_action : get_item_actions() )
  {
    def->add_action( item_action );
  }

  // Professions
  for ( const std::string& profession_action : get_profession_actions() )
  {
    def->add_action( profession_action );
  }

  // Potions
  if ( sim->allow_potions && true_level >= 80 )
  {
    def->add_action( "potion,if=buff.bloodlust.react|target.time_to_die<=40" );
  }

  if ( race == RACE_BLOOD_ELF )
  {
    def->add_action( "arcane_torrent,if=mana.pct<=95" );
  }

  if ( find_class_spell( "Shadowfiend" )->ok() )
  {
    def->add_action( "mindbender,if=talent.mindbender.enabled" );
    def->add_action( "shadowfiend,if=!talent.mindbender.enabled" );
  }

  if ( race != RACE_BLOOD_ELF )
  {
    for ( const std::string& racial_action : get_racial_actions() )
    {
      def->add_action( racial_action );
    }
  }

  def->add_talent( this, "Purge the Wicked", "if=!ticking" );
  def->add_action( this, "Shadow Word: Pain", "if=!ticking&!talent.purge_the_wicked.enabled" );

  def->add_talent( this, "Schism" );
  def->add_action( this, "Penance" );
  def->add_talent( this, "Purge the Wicked", "if=remains<(duration*0.3)" );
  def->add_action( this, "Shadow Word: Pain", "if=remains<(duration*0.3)&!talent.purge_the_wicked.enabled" );
  def->add_talent( this, "Divine Star", "if=mana.pct>80" );
  def->add_action( this, "Smite" );
  def->add_action( this, "Shadow Word: Pain" );
}

}  // namespace priestspace
