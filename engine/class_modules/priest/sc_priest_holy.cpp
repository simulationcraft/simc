// ==========================================================================
// Holy Priest Sim File
// Contact: https://github.com/orgs/simulationcraft/teams/priest/members
// Wiki: https://github.com/simulationcraft/simc/wiki/Priests
// ==========================================================================

#include "sc_priest.hpp"

#include "simulationcraft.hpp"

namespace priestspace
{
namespace actions::heals
{
struct echo_of_light_t final : public residual_action::residual_periodic_action_t<priest_heal_t>
{
  using ab = residual_action::residual_periodic_action_t<priest_heal_t>;

  echo_of_light_t( priest_t& p ) : ab( "echo_of_light", p, p.specs.echo_of_light )
  {
    harmful      = false;
    background   = true;
    holy_mastery = false;
  }
};
struct holy_word_sanctify_t final : public priest_heal_t
{
  holy_word_sanctify_t( priest_t& p, util::string_view options_str )
    : priest_heal_t( "holy_word_sanctify", p, p.talents.holy.holy_word_sanctify )
  {
    parse_options( options_str );

    harmful      = false;
    holy_mastery = true;

    if ( data().ok() )
      aoe = as<int>( data().effectN( 2 ).base_value() );
  }

  double cost() const override
  {
    if ( priest().buffs.apotheosis->check() || priest().buffs.answered_prayers->check() )
    {
      return 0;
    }

    return priest_heal_t::cost();
  }

  void execute() override
  {
    priest_heal_t::execute();

    if ( priest().talents.holy.eternal_sanctity.enabled() )
    {
      priest().buffs.apotheosis->extend_duration( player,
                                                  priest().talents.holy.eternal_sanctity->effectN( 1 ).time_value() );
    }

    if ( priest().talents.holy.divine_image.enabled() )
    {
      priest().buffs.divine_image->trigger();
      priest().procs.divine_image->occur();
    }
  }
};

struct holy_word_serenity_t final : public priest_heal_t
{
  holy_word_serenity_t( priest_t& p, util::string_view options_str )
    : priest_heal_t( "holy_word_serenity", p, p.talents.holy.holy_word_serenity )
  {
    parse_options( options_str );
    harmful      = false;
    holy_mastery = true;
  }

  double cost() const override
  {
    if ( priest().buffs.apotheosis->check() || priest().buffs.answered_prayers->check() )
    {
      return 0;
    }

    return priest_heal_t::cost();
  }

  void execute() override
  {
    priest_heal_t::execute();

    if ( priest().talents.holy.eternal_sanctity.enabled() )
    {
      priest().buffs.apotheosis->extend_duration( player,
                                                  priest().talents.holy.eternal_sanctity->effectN( 1 ).time_value() );
    }

    if ( priest().talents.holy.divine_image.enabled() )
    {
      priest().buffs.divine_image->trigger();
      priest().procs.divine_image->occur();
    }
  }
};
}  // namespace actions::heals
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
    priest().cooldowns.holy_word_chastise->reset( false );
    priest().cooldowns.holy_word_serenity->reset( false, 1 );
    priest().cooldowns.holy_word_sanctify->reset( false, 1 );
  }
};

struct divine_word_t final : public priest_spell_t
{
  divine_word_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "divine_word", p, p.talents.holy.divine_word )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().buffs.divine_word->trigger();
  }
};

// holy word cast -> Divine Image [Talent] (392988) -> Divine Image [Buff] (405963) -> Divine Image [Summon] (392990) ->
// Searing Light (196811) procs from Holy Fire, Chastise, Shadow Word: Pain, Shadow Word: Death, and Smite
struct searing_light_t final : public priest_spell_t
{
  searing_light_t( priest_t& p ) : priest_spell_t( "searing_light", p, p.talents.holy.divine_image_searing_light )
  {
    background = true;
    may_miss   = false;
  }
};

// holy word cast -> Divine Image [Talent] (392988) -> Divine Image [Buff] (405963) -> Divine Image [Summon] (392990) ->
// Light Eruption (196811) procs from Holy Nova casts
struct light_eruption_t final : public priest_spell_t
{
  light_eruption_t( priest_t& p ) : priest_spell_t( "light_eruption", p, p.talents.holy.divine_image_light_eruption )
  {
    background = true;
    may_miss   = false;
    aoe        = -1;
    radius     = 8;  // Base in spell data is incorrect (100yd)
  }
};

struct burning_vehemence_t final : public priest_spell_t
{
  burning_vehemence_t( priest_t& p ) : priest_spell_t( "burning_vehemence", p, p.talents.holy.burning_vehemence_damage )
  {
    background          = true;
    may_crit            = false;
    may_miss            = false;
    aoe                 = -1;
    full_amount_targets = as<int>( priest().talents.holy.burning_vehemence_damage->effectN( 2 ).base_value() );
    reduced_aoe_targets = priest().talents.holy.burning_vehemence_damage->effectN( 2 ).base_value();
    base_multiplier     = priest().talents.holy.burning_vehemence->effectN( 3 ).percent();
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    priest_spell_t::available_targets( tl );

    // Remove the main target, this only hits everything else in range.
    range::erase_remove( tl, target );

    return tl.size();
  }

  void trigger( double initial_hit_damage )
  {
    if ( target_list().size() == 0 )
      return;

    double bv_damage = initial_hit_damage * base_multiplier;
    sim->print_debug(
        "burning_vehemence splash damage calculating: initial holy_fire hit: {}, multiplier: {}, result: {}",
        initial_hit_damage, base_multiplier, bv_damage );
    base_dd_min = base_dd_max = bv_damage;
    execute();
  }
};

struct holy_fire_t final : public priest_spell_t
{
  propagate_const<burning_vehemence_t*> child_burning_vehemence;
  action_t* child_searing_light;
  cooldown_t* dummy_cooldown;

  holy_fire_t( priest_t& p, util::string_view options_str  )
    : priest_spell_t( "holy_fire", p, p.specs.holy_fire ),
      child_burning_vehemence( nullptr ),
      child_searing_light( priest().background_actions.searing_light )
  {
    if ( proc )
    {
      base_dd_max            = 0.0;
      base_dd_min            = 0.0;
      spell_power_mod.direct = 0;
    }

    if ( p.talents.holy.empyreal_blaze.enabled() )
    {
      dot_behavior = DOT_EXTEND;
    }

    if ( priest().talents.holy.burning_vehemence.enabled() && !proc )
    {
      child_burning_vehemence = new burning_vehemence_t( priest() );
      add_child( child_burning_vehemence );
    }

    dummy_cooldown = p.get_cooldown( "holy_fire_empyreal", this );
    dummy_cooldown->add_execute_type( execute_type::FOREGROUND );

    parse_options( options_str );
  }

  void queue_execute( execute_type type ) override
  {
    cooldown_t* original_cd = cooldown;
    if ( p().buffs.empyreal_blaze->check() )
    {
      cooldown = dummy_cooldown;
    }
    priest_spell_t::queue_execute( type );
    cooldown = original_cd;
  }

  double cost() const override
  {
    if ( priest().buffs.empyreal_blaze->check() || proc )
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

  // Brutal implementation of Ignore Spell Cooldown.
  bool ready() override
  {
    // Check conditions that do NOT pertain to the target before cycle_targets
    if ( !cooldown->is_ready() && !p().buffs.empyreal_blaze->check() )
      return false;

    if ( internal_cooldown->down() )
      return false;

    if ( player->is_moving() && !usable_moving() )
      return false;

    auto resource = current_resource();
    if ( resource != RESOURCE_NONE && !player->resource_available( resource, cost() ) )
    {
      if ( starved_proc )
        starved_proc->occur();
      return false;
    }

    if ( usable_while_casting )
    {
      if ( execute_time() > timespan_t::zero() )
      {
        return false;
      }

      // Don't allow cast-while-casting spells that trigger the GCD to be ready if the GCD is still
      // ongoing (during the cast)
      if ( ( player->executing || player->channeling ) && gcd() > timespan_t::zero() &&
           player->gcd_ready > sim->current_time() )
      {
        return false;
      }
    }

    return true;
  }

  void update_ready( timespan_t cd_duration /* = timespan_t::min() */ ) override
  {
    if ( proc )
      return;

    if ( cd_waste_data )
      cd_waste_data->add( cd_duration, time_to_execute );

    if ( ( cd_duration > 0_ms || ( cd_duration == timespan_t::min() && cooldown_duration() > 0_ms ) ) && !dual )
    {
      timespan_t delay = 0_ms;

      if ( !background && !proc )
      { /*This doesn't happen anymore due to the gcd queue, in WoD if an ability has a cooldown of 20 seconds,
        it is usable exactly every 20 seconds with proper Lag tolerance set in game.
        The only situation that this could happen is when world lag is over 400, as blizzard does not allow
        custom lag tolerance to go over 400.
        */
        delay = rng().gauss( player->world_lag );
        if ( delay > 400_ms )
        {
          delay -= 400_ms;  // Even high latency players get some benefit from CLT.
          sim->print_debug( "{} delaying the cooldown finish of {} by {}", *player, *this, delay );
        }
        else
          delay = 0_ms;
      }

      if ( !p().buffs.empyreal_blaze->check() )
      {
        cooldown->start( this, cd_duration, delay );

        sim->print_debug(
            "{} starts cooldown for {} ({}, {}/{}). Duration={} Delay={}. Will "
            "be ready at {}",
            *player, *this, *cooldown, cooldown->current_charge, cooldown->charges, cd_duration, delay,
            cooldown->ready );
      }

      if ( internal_cooldown->duration > 0_ms )
      {
        internal_cooldown->start( this );

        sim->print_debug( "{} starts internal_cooldown for {} ({}). Will be ready at {}", *player, *this,
                          *internal_cooldown, internal_cooldown->ready );
      }
    }
  }

  void execute() override
  {
    cooldown_t* original_cd = cooldown;

    if ( p().buffs.empyreal_blaze->up() )
    {
      cooldown = dummy_cooldown;
    }

    priest_spell_t::execute();

    if ( !proc )
      priest().buffs.empyreal_blaze->decrement();

    cooldown = original_cd;

    if ( priest().talents.holy.holy_word_chastise.enabled() && priest().talents.holy.voice_of_harmony.enabled() )
    {
      timespan_t chastise_cdr =
          timespan_t::from_seconds( priest().talents.holy.voice_of_harmony->effectN( 3 ).base_value() );

      priest().do_holy_word_cdr( priest().cooldowns.holy_word_chastise, chastise_cdr );
    }
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );
    if ( result_is_hit( s->result ) )
    {
      if ( child_searing_light && priest().buffs.divine_image->up() )
      {
        for ( int i = 1; i <= priest().buffs.divine_image->stack(); i++ )
        {
          child_searing_light->execute();
        }
      }
    }

    if ( child_burning_vehemence )
    {
      child_burning_vehemence->trigger( s->result_amount );
    }
  }
};

struct holy_word_chastise_t final : public priest_spell_t
{
  action_t* child_searing_light;
  holy_word_chastise_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "holy_word_chastise", p, p.talents.holy.holy_word_chastise ),
      child_searing_light( priest().background_actions.searing_light )
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

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double d = priest_spell_t::composite_da_multiplier( s );
    if ( priest().buffs.divine_word->check() )
    {
      d *= 1.0 + priest().talents.holy.divine_word->effectN( 1 ).percent();
      sim->print_debug( "divine_favor increasing holy_word_chastise damage by {}, total: {}",
                        priest().talents.holy.divine_word->effectN( 1 ).percent(), d );
    }
    return d;
  }

  void execute() override
  {
    priest_spell_t::execute();

    if ( priest().talents.holy.eternal_sanctity.enabled() )
    {
      priest().buffs.apotheosis->extend_duration( player,
                                                  priest().talents.holy.eternal_sanctity->effectN( 1 ).time_value() );
    }

    if ( priest().talents.holy.divine_word.enabled() && priest().buffs.divine_word->check() )
    {
      priest().buffs.divine_word->expire();
      priest().buffs.divine_favor_chastise->trigger();
    }

    if ( priest().talents.holy.divine_image.enabled() )
    {
      priest().buffs.divine_image->trigger();
      priest().procs.divine_image->occur();
      // Activating cast also immediately executes searing light
      for ( int i = 1; i <= priest().buffs.divine_image->stack(); i++ )
      {
        child_searing_light->execute();
      }
    }

    if ( priest().talents.holy.empyreal_blaze.ok() )
    {
      priest().buffs.empyreal_blaze->trigger();
    }
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );
    sim->print_debug( "divine_image_buff: {}", priest().buffs.divine_image->up() );
    if ( child_searing_light && priest().buffs.divine_image->up() )
    {
      for ( int i = 1; i <= priest().buffs.divine_image->stack(); i++ )
      {
        child_searing_light->execute();
      }
    }
  }
};

}  // namespace actions::spells

namespace buffs
{
symbol_of_hope_t::symbol_of_hope_t( player_t* p )
  : buff_t( p, "symbol_of_hope", p->find_spell( 64901 ) ), affected_actions_initialized( false )
{
  set_cooldown( 0_ms );
  s_data_reporting = data().effectN( 1 ).trigger();

  set_stack_change_callback( [ this ]( buff_t* /* b */, int /* old */, int new_ ) {
    if ( !affected_actions_initialized )
    {
      affected_actions.clear();

      for ( auto a : player->action_list )
      {
        for ( const spelleffect_data_t& effect : data().effectN( 1 ).trigger()->effects() )
        {
          if ( a->data().affected_by_label( effect ) || a->data().affected_by_category( effect ) )
          {
            auto affected_action = range::find_if(
                affected_actions,
                [ a ]( std::pair<action_t*, double> affected_action ) { return a == affected_action.first; } );
            if ( affected_action == affected_actions.end() )
            {
              affected_actions.emplace_back( a, effect.default_value() );
            }
          }
        }
      }

      affected_actions_initialized = true;
    }

    update_cooldowns( new_ );
  } );
}

void symbol_of_hope_t::update_cooldowns( int new_ )
{
  assert( affected_actions_initialized );

  for ( auto a : affected_actions )
  {
    double recharge_rate_multiplier = 1.0 / ( 1 + a.second );
    if ( new_ > 0 )
    {
      a.first->dynamic_recharge_rate_multiplier *= recharge_rate_multiplier;
    }
    else
    {
      a.first->dynamic_recharge_rate_multiplier /= recharge_rate_multiplier;
    }
    if ( a.first->cooldown->action == a.first )
      a.first->cooldown->adjust_recharge_multiplier();
    if ( a.first->internal_cooldown->action == a.first )
      a.first->internal_cooldown->adjust_recharge_multiplier();
  }
}
}  // namespace buffs

void priest_t::do_holy_word_cdr( cooldown_t* cd, timespan_t amount, bool affected_by_apotheosis, bool affected_by_naaru )
{
  if ( !cd )
    return;

  if ( affected_by_apotheosis && buffs.apotheosis->check() )
  {
    amount *= 1 + buffs.apotheosis->value();
  }

  if ( affected_by_naaru && talents.holy.light_of_the_naaru.enabled() )
  {
    amount *= 1 + talents.holy.light_of_the_naaru->effectN( 1 ).percent();
  }

  if ( sim->debug )
    sim->print_debug( "{} adjusted cooldown of {}, by {}, with light_of_the_naaru: {} - {}, apotheosis: {} - {}.",
                      *this, *cd, amount, talents.holy.light_of_the_naaru.rank(), affected_by_naaru,
                      buffs.apotheosis->check(), affected_by_apotheosis );

  cd->adjust( -amount );
}

void priest_t::create_buffs_holy()
{
  buffs.apotheosis = make_buff( this, "apotheosis", talents.holy.apotheosis )->set_default_value_from_effect( 1, 0.01 );

  buffs.answered_prayers =
      make_buff( this, "answered_prayers", talents.holy.apotheosis )
          ->set_cooldown( timespan_t::from_seconds( talents.holy.answered_prayers->effectN( 2 ).base_value() ) )
          ->set_duration( timespan_t::from_seconds( talents.holy.answered_prayers->effectN( 2 ).base_value() ) );

  buffs.answered_prayers_timer =
      make_buff( this, "answered_prayers_timer", talents.holy.answered_prayers )
          ->set_quiet( true )
          // Option default: Prayer of mending bounces onces every 1.5 seconds
          // duration = Required # of bounces * bounce rate
          ->set_duration( timespan_t::from_seconds( talents.holy.answered_prayers->effectN( 1 ).base_value() *
                                                    options.prayer_of_mending_bounce_rate ) )
          ->set_stack_change_callback( ( [ this ]( buff_t*, int, int new_ ) {
            if ( new_ == 0 )
            {
              buffs.answered_prayers->trigger();
              buffs.answered_prayers_timer->trigger();
            }
          } ) );

  buffs.empyreal_blaze = make_buff( this, "empyreal_blaze", talents.holy.empyreal_blaze_buff )->set_reverse( true );

  buffs.divine_word = make_buff( this, "divine_word", talents.holy.divine_word );

  buffs.divine_favor_chastise = make_buff( this, "divine_favor_chastise", talents.holy.divine_favor_chastise );

  buffs.divine_image = make_buff( this, "divine_image", talents.holy.divine_image_buff )
                           ->set_refresh_behavior( buff_refresh_behavior::DURATION )
                           ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );
}

void priest_t::init_rng_holy()
{
}

void priest_t::init_spells_holy()
{
  auto ST = [ this ]( std::string_view n ) { return find_talent_spell( talent_tree::SPECIALIZATION, n ); };
  // Row 1
  talents.holy.holy_word_serenity = ST( "Holy Word: Serenity" );
  // Row 2
  talents.holy.holy_word_sanctify = ST( "Holy Word: Sanctify" );
  talents.holy.guardian_spirit    = ST( "Guardian Spirit" );
  talents.holy.holy_word_chastise = ST( "Holy Word: Chastise" );
  // Row 3
  talents.holy.prayer_of_healing   = ST( "Prayer of Healing" );
  talents.holy.guardian_angel      = ST( "Guardian Angel" );
  talents.holy.restitution         = ST( "Restitution" );
  talents.holy.censure             = ST( "Censure" );
  talents.holy.empyreal_blaze      = ST( "Empyreal Blaze" );
  talents.holy.empyreal_blaze_buff = find_spell( 372617 );
  // Row 4
  talents.holy.prayer_circle            = ST( "Prayer Circle" );
  talents.holy.cosmic_ripple            = ST( "Cosmic Ripple" );
  talents.holy.afterlife                = ST( "Afterlife" );
  talents.holy.voice_of_harmony         = ST( "Voice of Harmony" );
  talents.holy.searing_light            = ST( "Searing Light" );
  talents.holy.burning_vehemence        = ST( "Burning Vehemence" );
  talents.holy.burning_vehemence_damage = find_spell( 400370 );
  // Row 5
  talents.holy.everlasting_light = ST( "Everlasting Light" );
  talents.holy.holy_mending      = ST( "Holy Mending" );
  talents.holy.divine_hymn       = ST( "Divine Hymn" );
  talents.holy.enlightenment     = ST( "Enlightenment" );
  talents.holy.benediction       = ST( "Benediction" );
  // Row 6
  talents.holy.prayerful_litany   = ST( "Prayerful Litany" );
  talents.holy.renewed_faith      = ST( "Renewed Faith" );
  talents.holy.seraphic_crescendo = ST( "Seraphic Crescendo" );
  talents.holy.gales_of_song      = ST( "Gales of Song" );
  talents.holy.symbol_of_hope     = ST( "Symbol of Hope" );
  talents.holy.divine_service     = ST( "Divine Service" );
  // Row 7
  talents.holy.crisis_management       = ST( "Crisis Management" );
  talents.holy.empowered_renew         = ST( "Empowered Renew" );
  talents.holy.apotheosis              = ST( "Apotheosis" );
  talents.holy.prayers_of_the_virtuous = ST( "Prayers of the Virtuous" );
  // Row 8
  talents.holy.resonant_words   = ST( "Resonant Words" );
  talents.holy.miracle_worker   = ST( "Miracle Worker" );
  talents.holy.divinity         = ST( "Divinity" );
  talents.holy.eternal_sanctity = ST( "Eternal Sanctity" );
  talents.holy.holy_celerity    = ST( "Holy Celerity" );
  talents.holy.say_your_prayers = ST( "Say Your Prayers" );
  // Row 9
  talents.holy.trail_of_light        = ST( "Trail of Light" );
  talents.holy.dispersing_light      = ST( "Dispersing Light" );
  talents.holy.light_of_the_naaru    = ST( "Light of the Naaru" );
  talents.holy.light_in_the_darkness = ST( "Light in the Darkness" );
  talents.holy.prismatic_echoes      = ST( "Prismatic Echoes" );
  talents.holy.desperate_times       = ST( "Desperate Times" );
  talents.holy.epiphany              = ST( "Epiphany" );
  // Row 10
  talents.holy.lightweaver                 = ST( "Lightweaver" );
  talents.holy.lightwell                   = ST( "Lightwell" );
  talents.holy.divine_image                = ST( "Divine Image" );
  talents.holy.divine_image_buff           = find_spell( 405963 );
  talents.holy.divine_image_summon         = find_spell( 392990 );
  talents.holy.divine_image_searing_light  = find_spell( 196811 );
  talents.holy.divine_image_light_eruption = find_spell( 196812 );
  talents.holy.lasting_words               = ST( "Lasting Words" );
  talents.holy.divine_word                 = ST( "Divine Word" );
  talents.holy.divine_favor_chastise       = find_spell( 372761 );
  talents.holy.answered_prayers            = ST( "Answered Prayers" );

  // General Spells
  specs.holy_fire     = find_specialization_spell( "Holy Fire" );
  specs.echo_of_light = find_spell( 77489 );
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
  if ( name == "holy_word_chastise" )
  {
    return new holy_word_chastise_t( *this, options_str );
  }
  if ( name == "holy_word_serenity" )
  {
    return new holy_word_serenity_t( *this, options_str );
  }
  if ( name == "holy_word_sanctify" )
  {
    return new holy_word_sanctify_t( *this, options_str );
  }
  if ( name == "divine_word" )
  {
    return new divine_word_t( *this, options_str );
  }
  if ( name == "searing_light" )
  {
    return new searing_light_t( *this );
  }
  if ( name == "light_eruption" )
  {
    return new light_eruption_t( *this );
  }

  return nullptr;
}

void priest_t::init_background_actions_holy()
{
  if ( talents.holy.divine_image.enabled() )
  {
    background_actions.searing_light  = new actions::spells::searing_light_t( *this );
    background_actions.light_eruption = new actions::spells::light_eruption_t( *this );
  }

  background_actions.echo_of_light = new actions::heals::echo_of_light_t( *this );
}

expr_t* priest_t::create_expression_holy( action_t*, util::string_view /*name_str*/ )
{
  return nullptr;
}

}  // namespace priestspace
