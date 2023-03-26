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

struct holy_word_sanctify_t final : public priest_spell_t
{
  holy_word_sanctify_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "holy_word_sanctify", p, p.talents.holy.holy_word_sanctify )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    priest_spell_t::execute();
    if ( priest().talents.holy.divine_image.enabled() )
    {
      priest().buffs.divine_image->trigger();
    }
  }
};

struct holy_word_serenity_t final : public priest_spell_t
{
  holy_word_serenity_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "holy_word_serenity", p, p.talents.holy.holy_word_serenity )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    priest_spell_t::execute();
    if ( priest().talents.holy.divine_image.enabled() )
    {
      priest().buffs.divine_image->trigger();
    }
  }
};

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
    sim->print_debug( "starting divine_word." );
  }
};

// holy word cast -> Divine Image [Talent] (392988) -> Divine Image [Buff] (405963) -> Divine Image [Summon] (392990) ->
// Searing Light (196811)
struct searing_light_t final : public priest_spell_t
{
  searing_light_t( priest_t& p ) : priest_spell_t( "searing_light", p, p.talents.holy.divine_image_searing_light )
  {
    background = true;
    may_miss   = false;
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

struct burning_vehemence_t final : public priest_spell_t
{
  burning_vehemence_t( priest_t& p ) : priest_spell_t( "burning_vehemence", p, p.talents.holy.burning_vehemence_damage )
  {
    background          = true;
    dual                = true;
    may_crit            = false;
    may_miss            = false;
    aoe                 = -1;
    reduced_aoe_targets = priest().talents.holy.burning_vehemence_damage->effectN( 2 ).base_value();
    base_multiplier     = priest().talents.holy.burning_vehemence->effectN( 3 ).percent();
  }
  void trigger( double initial_hit_damage )
  {
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
  timespan_t manipulation_cdr;
  propagate_const<burning_vehemence_t*> child_burning_vehemence;
  propagate_const<searing_light_t*> child_searing_light;

  holy_fire_t( priest_t& p )
    : priest_spell_t( "holy_fire", p, p.specs.holy_fire ),
      manipulation_cdr( timespan_t::from_seconds( priest().talents.manipulation->effectN( 1 ).base_value() / 2 ) ),
      child_burning_vehemence( nullptr ),
      child_searing_light( nullptr )
  {
    apply_affecting_aura( p.talents.holy.burning_vehemence );
    if ( p.talents.holy.empyreal_blaze.enabled() )
    {
      dot_behavior = DOT_EXTEND;
    }
    if ( priest().talents.holy.burning_vehemence.enabled() )
    {
      child_burning_vehemence = new burning_vehemence_t( priest() );
      add_child( child_burning_vehemence );
    }
    if ( priest().talents.holy.divine_image.enabled() )
    {
      child_searing_light = new searing_light_t( priest() );
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

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );
    if ( result_is_hit( s->result ) )
    {
      if ( priest().talents.holy.holy_word_chastise.enabled() && priest().talents.holy.harmonious_apparatus.enabled() )
      {
        timespan_t chastise_cdr =
            timespan_t::from_seconds( priest().talents.holy.harmonious_apparatus->effectN( 1 ).base_value() );
        if ( priest().talents.holy.apotheosis.enabled() && priest().buffs.apotheosis->up() )
        {
          chastise_cdr *= ( 1 + priest().talents.holy.apotheosis->effectN( 1 ).percent() );
        }
        if ( priest().talents.holy.light_of_the_naaru.enabled() )
        {
          chastise_cdr *= ( 1 + priest().talents.holy.light_of_the_naaru->effectN( 1 ).percent() );
        }
        sim->print_debug( "{} adjusted cooldown of Chastise, by {}, with harmonious_apparatus: {}, apotheosis: {}",
                          priest(), chastise_cdr, priest().talents.holy.harmonious_apparatus.enabled(),
                          ( priest().talents.holy.apotheosis.enabled() && priest().buffs.apotheosis->up() ) );

        priest().cooldowns.holy_word_chastise->adjust( chastise_cdr );
      }
      if ( child_searing_light && priest().buffs.divine_image->up() )
      {
        sim->print_debug( "searing_light triggered by holy_fire" );
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
  propagate_const<searing_light_t*> child_searing_light;
  holy_word_chastise_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "holy_word_chastise", p, p.talents.holy.holy_word_chastise ), child_searing_light( nullptr )
  {
    parse_options( options_str );
    if ( priest().talents.holy.divine_image.enabled() )
    {
      child_searing_light = new searing_light_t( priest() );
    }
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
      d *= 1.0 + priest().buffs.divine_word->data().effectN( 1 ).percent();
      sim->print_debug( "divine_favor increasing holy_word_chastise damage by {}, total: {}",
                        priest().buffs.divine_word->data().effectN( 1 ).percent(), d );
    }
    return d;
  }

  void execute() override
  {
    priest_spell_t::execute();
    if ( priest().talents.holy.divine_word.enabled() && priest().buffs.divine_word->check() )
    {
      priest().buffs.divine_word->expire();
      priest().buffs.divine_favor_chastise->trigger();
    }
    if ( priest().talents.holy.divine_image.enabled() )
    {
      priest().buffs.divine_image->trigger();
      // Activating cast also immediately executes searing light
      sim->print_debug( "searing_light triggered by holy_word_chastise" );
      for ( int i = 1; i <= priest().buffs.divine_image->stack(); i++ )
      {
        child_searing_light->execute();
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );
    sim->print_debug( "divine_image_buff: {}", priest().buffs.divine_image->up() );
    if ( child_searing_light && priest().buffs.divine_image->up() )
    {
      sim->print_debug( "searing_light triggered by holy_word_chastise" );
      for ( int i = 1; i <= priest().buffs.divine_image->stack(); i++ )
      {
        child_searing_light->execute();
      }
    }
  }
};

}  // namespace actions::spells

void priest_t::create_buffs_holy()
{
  buffs.apotheosis = make_buff( this, "apotheosis", talents.holy.apotheosis );

  buffs.empyreal_blaze = make_buff( this, "empyreal_blaze", talents.holy.empyreal_blaze->effectN( 2 ).trigger() )
                             ->set_trigger_spell( talents.holy.empyreal_blaze )
                             ->set_reverse( true )
                             ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
                               if ( new_ > 0 )
                               {
                                 cooldowns.holy_fire->reset( false );
                               }
                             } );

  buffs.divine_word = make_buff( this, "divine_word", talents.holy.divine_word );

  buffs.divine_favor_chastise = make_buff( this, "divine_favor_chastise", talents.holy.divine_favor_chastise );

  buffs.divine_image = make_buff( this, "divine_image", talents.holy.divine_image_buff );
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
  talents.holy.holy_word_chastise = ST( "Holy Word: Chastise" );
  // Row 3
  talents.holy.empyreal_blaze     = ST( "Empyreal Blaze" );
  talents.holy.holy_word_sanctify = ST( "Holy Word: Sanctify" );
  // Row 4
  talents.holy.searing_light = ST( "Searing Light" );
  // Row 8
  talents.holy.apotheosis = ST( "Apotheosis" );
  // Row 9
  talents.holy.burning_vehemence        = ST( "Burning Vehemence" );
  talents.holy.burning_vehemence_damage = find_spell( 400370 );
  talents.holy.harmonious_apparatus     = ST( "harmonious Apparatus" );
  talents.holy.light_of_the_naaru       = ST( "Light of the Naaru" );
  // Row 10
  talents.holy.divine_word                = ST( "Divine Word" );
  talents.holy.divine_favor_chastise      = find_spell( 372761 );
  talents.holy.divine_image               = ST( "Divine Image" );
  talents.holy.divine_image_buff          = find_spell( 405963 );
  talents.holy.divine_image_summon        = find_spell( 392990 );
  talents.holy.divine_image_searing_light = find_spell( 196811 );
  talents.holy.miracle_worker             = ST( "Miracle Worker" );

  // General Spells
  specs.holy_fire = find_specialization_spell( "Holy Fire" );
}

action_t* priest_t::create_action_holy( util::string_view name, util::string_view options_str )
{
  using namespace actions::spells;

  if ( name == "holy_fire" )
  {
    return new holy_fire_t( *this );
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
  if ( name == "empyreal_blaze" )
  {
    return new empyreal_blaze_t( *this, options_str );
  }
  if ( name == "divine_word" )
  {
    return new divine_word_t( *this, options_str );
  }
  if ( name == "searing_light" )
  {
    return new searing_light_t( *this );
  }

  return nullptr;
}

void priest_t::init_background_actions_holy()
{
  background_actions.holy_fire     = new actions::spells::holy_fire_t( *this );
  background_actions.searing_light = new actions::spells::searing_light_t( *this );
}

expr_t* priest_t::create_expression_holy( action_t*, util::string_view /*name_str*/ )
{
  return nullptr;
}

}  // namespace priestspace
