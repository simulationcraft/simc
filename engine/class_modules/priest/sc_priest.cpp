// ==========================================================================
// Priest Sim File
// Contact: https://github.com/orgs/simulationcraft/teams/priest/members
// Wiki: https://github.com/simulationcraft/simc/wiki/Priests
// ==========================================================================

#include "sc_priest.hpp"

#include "class_modules/apl/apl_priest.hpp"
#include "sc_enums.hpp"
#include "sim/sc_option.hpp"
#include "tcb/span.hpp"

#include "simulationcraft.hpp"

namespace priestspace
{
namespace actions
{
namespace spells
{
// ==========================================================================
// Angelic Feather
// ==========================================================================
struct angelic_feather_t final : public priest_spell_t
{
  angelic_feather_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "angelic_feather", p, p.find_class_spell( "Angelic Feather" ) )
  {
    parse_options( options_str );
    harmful = may_hit = may_crit = false;
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( s->target->buffs.angelic_feather )
    {
      s->target->buffs.angelic_feather->trigger();
    }
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( !candidate_target->buffs.angelic_feather )
    {
      return false;
    }

    return priest_spell_t::target_ready( candidate_target );
  }
};

// ==========================================================================
// Divine Star
// ==========================================================================

/// Divine Star Base Spell, used for both heal and damage spell.
template <class Base>
struct divine_star_base_t : public Base
{
private:
  using ab = Base;  // the action base ("ab") type (priest_spell_t or priest_heal_t)
public:
  using base_t = divine_star_base_t<Base>;

  propagate_const<divine_star_base_t*> return_spell;

  divine_star_base_t( util::string_view n, priest_t& p, const spell_data_t* spell_data, bool is_return_spell = false )
    : ab( n, p, spell_data ),
      return_spell( ( is_return_spell ? nullptr : new divine_star_base_t( n, p, spell_data, true ) ) )
  {
    ab::aoe = -1;

    ab::proc = ab::background = true;
  }

  // Divine Star will damage and heal targets twice, once on the way out and again on the way back. This is determined
  // by distance from the target. If we are too far away, it misses completely. If we are at the very edge distance
  // wise, it will only hit once. If we are within range (and aren't moving such that it would miss the target on the
  // way out and/or back), it will hit twice. Threshold is 24 yards, per tooltip and tests for 2 hits. 28 yards is the
  // threshold for 1 hit.
  void execute() override
  {
    double distance;

    distance = ab::player->get_player_distance( *ab::target );

    if ( distance <= 28 )
    {
      ab::execute();

      if ( return_spell && distance <= 24 )
      {
        return_spell->execute();
      }
    }
  }
};

struct divine_star_t final : public priest_spell_t
{
  divine_star_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "divine_star", p, p.talents.divine_star ),
      _heal_spell( new divine_star_base_t<priest_heal_t>( "divine_star_heal", p, p.find_spell( 110745 ) ) ),
      _dmg_spell( new divine_star_base_t<priest_spell_t>( "divine_star_damage", p, p.find_spell( 122128 ) ) )
  {
    parse_options( options_str );

    dot_duration = base_tick_time = timespan_t::zero();

    add_child( _heal_spell );
    add_child( _dmg_spell );
  }

  void execute() override
  {
    priest_spell_t::execute();

    _heal_spell->execute();
    _dmg_spell->execute();
  }

private:
  action_t* _heal_spell;
  action_t* _dmg_spell;
};

// ==========================================================================
// Halo
// ==========================================================================
/// Halo Base Spell, used for both damage and heal spell.
template <class Base>
struct halo_base_t : public Base
{
public:
  halo_base_t( util::string_view n, priest_t& p, const spell_data_t* s ) : Base( n, p, s )
  {
    Base::aoe        = -1;
    Base::background = true;

    if ( Base::data().ok() )
    {
      // Parse the correct effect number, because we have two competing ones ( were 2 > 1 always wins out )
      Base::parse_effect_data( Base::data().effectN( 1 ) );
    }
    Base::radius       = 30;
    Base::range        = 0;
    Base::travel_speed = 15;  // Rough estimate, 2021-01-03
  }
};

struct halo_t final : public priest_spell_t
{
  halo_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "halo", p, p.talents.halo ),
      _heal_spell( new halo_base_t<priest_heal_t>( "halo_heal", p, p.find_spell( 120692 ) ) ),
      _dmg_spell( new halo_base_t<priest_spell_t>( "halo_damage", p, p.find_spell( 120696 ) ) )
  {
    parse_options( options_str );

    add_child( _heal_spell );
    add_child( _dmg_spell );
  }

  void execute() override
  {
    priest_spell_t::execute();

    _heal_spell->execute();
    _dmg_spell->execute();
  }

private:
  propagate_const<action_t*> _heal_spell;
  propagate_const<action_t*> _dmg_spell;
};

// ==========================================================================
// Levitate
// ==========================================================================
struct levitate_t final : public priest_spell_t
{
  levitate_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "levitate", p, p.find_class_spell( "Levitate" ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;
  }
};

// ==========================================================================
// Power Word: Fortitude
// ==========================================================================
struct power_word_fortitude_t final : public priest_spell_t
{
  power_word_fortitude_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "power_word_fortitude", p, p.find_class_spell( "Power Word: Fortitude" ) )
  {
    parse_options( options_str );
    harmful               = false;
    ignore_false_positive = true;

    background = sim->overrides.power_word_fortitude != 0;
  }

  void execute() override
  {
    priest_spell_t::execute();

    if ( !sim->overrides.power_word_fortitude )
      sim->auras.power_word_fortitude->trigger();
  }
};

// ==========================================================================
// Smite
// ==========================================================================
struct smite_t final : public priest_spell_t
{
  const spell_data_t* holy_fire_rank2;
  const spell_data_t* holy_word_chastise;
  const spell_data_t* smite_rank2;
  propagate_const<cooldown_t*> holy_word_chastise_cooldown;

  smite_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "smite", p, p.find_class_spell( "Smite" ) ),
      holy_fire_rank2( priest().find_rank_spell( "Holy Fire", "Rank 2" ) ),
      holy_word_chastise( priest().find_specialization_spell( 88625 ) ),
      smite_rank2( priest().find_rank_spell( "Smite", "Rank 2" ) ),
      holy_word_chastise_cooldown( p.get_cooldown( "holy_word_chastise" ) )
  {
    parse_options( options_str );
    if ( smite_rank2->ok() )
    {
      base_multiplier *= 1.0 + smite_rank2->effectN( 1 ).percent();
    }
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );
    if ( holy_fire_rank2->ok() && s->result_amount > 0 )
    {
      double hf_proc_chance = holy_fire_rank2->effectN( 1 ).percent();
      if ( rng().roll( hf_proc_chance ) )
      {
        sim->print_debug( "{} reset holy fire cooldown, using smite.", priest() );
        priest().cooldowns.holy_fire->reset( true );
      }
    }

    sim->print_debug( "{} checking for Apotheosis buff and Light of the Naaru talent.", priest() );
    auto cooldown_base_reduction = -timespan_t::from_seconds( holy_word_chastise->effectN( 2 ).base_value() );
    if ( s->result_amount > 0 && priest().buffs.apotheosis->up() )
    {
      auto cd1 = cooldown_base_reduction * ( 100 + priest().talents.apotheosis->effectN( 1 ).base_value() ) / 100.0;
      holy_word_chastise_cooldown->adjust( cd1 );

      sim->print_debug( "{} adjusted cooldown of Chastise, by {}, with Apotheosis.", priest(), cd1 );
    }
    else if ( s->result_amount > 0 && priest().talents.light_of_the_naaru->ok() )
    {
      auto cd2 =
          cooldown_base_reduction * ( 100 + priest().talents.light_of_the_naaru->effectN( 1 ).base_value() ) / 100.0;
      holy_word_chastise_cooldown->adjust( cd2 );
      sim->print_debug( "{} adjusted cooldown of Chastise, by {}, with Light of the Naaru.", priest(), cd2 );
    }
    else if ( s->result_amount > 0 )
    {
      holy_word_chastise_cooldown->adjust( cooldown_base_reduction );
      sim->print_debug( "{} adjusted cooldown of Chastise, by {}, without Apotheosis.", priest(),
                        cooldown_base_reduction );
    }
  }

  bool ready() override
  {
    // Ascended Blast replaces Smite when Boon of the Ascended is active
    if ( priest().buffs.boon_of_the_ascended->check() )
    {
      return false;
    }

    return priest_spell_t::ready();
  }
};

// ==========================================================================
// Power Infusion
// ==========================================================================
struct power_infusion_t final : public priest_spell_t
{
  power_infusion_t( priest_t& p, util::string_view options_str, util::string_view name )
    : priest_spell_t( name, p, p.find_class_spell( "Power Infusion" ) )
  {
    parse_options( options_str );
    harmful = false;

    // Adjust the cooldown if using the conduit and not casting PI on yourself
    if ( priest().conduits.power_unto_others->ok() &&
         ( priest().legendary.twins_of_the_sun_priestess->ok() || !priest().options.self_power_infusion ) )
    {
      cooldown->duration -= timespan_t::from_seconds( priest().conduits.power_unto_others.value() );
    }
  }

  void execute() override
  {
    priest_spell_t::execute();

    // Trigger PI on the actor only if casting on itself or having the legendary
    if ( priest().options.self_power_infusion || priest().legendary.twins_of_the_sun_priestess->ok() )
      player->buffs.power_infusion->trigger();
  }
};

// ==========================================================================
// Fae Guardians - Night Fae Covenant
// ==========================================================================
struct fae_guardians_t final : public priest_spell_t
{
  fae_guardians_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "fae_guardians", p, p.covenant.fae_guardians )
  {
    parse_options( options_str );
    harmful     = false;
    use_off_gcd = false;
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().buffs.fae_guardians->trigger();
    if ( priest().options.self_benevolent_faerie )
      player->buffs.benevolent_faerie->trigger();
  }

  void impact( action_state_t* s ) override
  {
    // Currently impossible to give someone else the benevolent faeire while still auto applying the wrathful faerie
    // This will cause the APL to cast Shadow Word: Pain manually to apply the faerie after using Fae Guardians
    if ( !priest().options.self_benevolent_faerie )
      return;

    priest_spell_t::impact( s );
    priest_td_t& td = get_td( s->target );
    td.buffs.wrathful_faerie->trigger();
  }
};

struct wrathful_faerie_t final : public priest_spell_t
{
  double insanity_gain;
  bool bwonsamdis_pact_wrathful;

  wrathful_faerie_t( priest_t& p )
    : priest_spell_t( "wrathful_faerie", p, p.find_spell( 342132 ) ),
      insanity_gain( p.find_spell( 327703 )->effectN( 2 ).resource( RESOURCE_INSANITY ) )
  {
    energize_type      = action_energize::ON_HIT;
    energize_resource  = RESOURCE_INSANITY;
    energize_amount    = insanity_gain;
    background         = true;
    cooldown->duration = data().internal_cooldown();
    bwonsamdis_pact_wrathful = util::str_compare_ci( priest().options.bwonsamdis_pact_mask_type, "wrathful" );
  }

  void adjust_energize_amount()
  {
    if ( !priest().legendary.bwonsamdis_pact->ok() )
      return;

    if ( bwonsamdis_pact_wrathful )
    {
      energize_amount = insanity_gain * 2;
      sim->print_debug( "Bwonsamdi's Pact adjusts Wrathful Faerie insanity gain to {}", energize_amount );
    }
    else
    {
      energize_amount = insanity_gain;
    }
  }

  void trigger()
  {
    if ( priest().cooldowns.wrathful_faerie->is_ready() )
    {
      adjust_energize_amount();
      execute();
      priest().cooldowns.wrathful_faerie->start();
    }
  }
};

struct wrathful_faerie_fermata_t final : public priest_spell_t
{
  double insanity_gain;
  wrathful_faerie_fermata_t( priest_t& p )
    : priest_spell_t( "wrathful_faerie_fermata", p, p.find_spell( 345452 ) ),
      insanity_gain( data().effectN( 3 ).resource( RESOURCE_INSANITY ) )
  {
    energize_type     = action_energize::ON_HIT;
    energize_resource = RESOURCE_INSANITY;
    energize_amount   = insanity_gain;
    background        = true;

    cooldown->duration = data().internal_cooldown();
  }

  void adjust_energize_amount()
  {
    // TODO: check if Bwonsamdi's Pact affects the Fae Fermata faerie
    energize_amount = insanity_gain;
  }

  void trigger()
  {
    if ( priest().cooldowns.wrathful_faerie_fermata->is_ready() )
    {
      adjust_energize_amount();
      execute();
      priest().cooldowns.wrathful_faerie_fermata->start();
    }
  }
};

// ==========================================================================
// Unholy Nova - Necrolord Covenant
// ==========================================================================
struct unholy_transfusion_t final : public priest_spell_t
{
  double parent_targets = 1;

  unholy_transfusion_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "unholy_transfusion", p, p.covenant.unholy_nova->effectN( 2 ).trigger() )
  {
    parse_options( options_str );
    background                 = true;
    hasted_ticks               = true;
    tick_may_crit              = true;
    tick_zero                  = false;
    affected_by_shadow_weaving = true;

    if ( priest().conduits.festering_transfusion->ok() )
    {
      dot_duration += priest().conduits.festering_transfusion->effectN( 2 ).time_value();
      base_td_multiplier *= ( 1.0 + priest().conduits.festering_transfusion.percent() );
    }
  }

  double action_ta_multiplier() const override
  {
    double m = priest_spell_t::action_ta_multiplier();

    double scaled_m = m / std::sqrt( parent_targets );

    sim->print_debug( "{} {} updates ta multiplier: Before: {} After: {} with {} targets from the parent spell.",
                      *player, *this, m, scaled_m, parent_targets );

    return scaled_m;
  }
};

struct unholy_transfusion_healing_t final : public priest_heal_t
{
  unholy_transfusion_healing_t( priest_t& p )
    : priest_heal_t( "unholy_transfusion_healing", p,
                     p.covenant.unholy_nova->effectN( 2 ).trigger()->effectN( 2 ).trigger() )
  {
    background = true;
    harmful    = false;

    // TODO: Confirm if this leech healing can proc trinkets/etc
    callbacks = false;

    if ( priest().conduits.festering_transfusion->ok() )
    {
      base_dd_multiplier *= ( 1.0 + priest().conduits.festering_transfusion.percent() );
    }
  }

  void trigger()
  {
    execute();
  }
};

struct unholy_nova_healing_t final : public priest_heal_t
{
  unholy_nova_healing_t( priest_t& p )
    : priest_heal_t( "unholy_nova_healing", p, p.covenant.unholy_nova->effectN( 1 ).trigger() )
  {
    background = true;
    harmful    = false;
    aoe        = -1;

    if ( priest().conduits.festering_transfusion->ok() )
    {
      base_dd_multiplier *= ( 1.0 + priest().conduits.festering_transfusion.percent() );
    }
  }

  void impact( action_state_t* s ) override
  {
    // Should only heal allies/pets and not enemies, even if you target an enemy
    if ( s->target->is_enemy() )
      return;

    priest_heal_t::impact( s );
  }
};

struct unholy_nova_t final : public priest_spell_t
{
  propagate_const<unholy_transfusion_t*> child_unholy_transfusion;
  propagate_const<unholy_nova_healing_t*> child_unholy_nova_healing;

  unholy_nova_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "unholy_nova", p, p.covenant.unholy_nova ),
      child_unholy_transfusion( nullptr ),
      child_unholy_nova_healing( nullptr )
  {
    parse_options( options_str );
    aoe      = -1;
    may_crit = false;

    // Radius for damage spell is stored in the DoT's spell radius
    radius = data().effectN( 2 ).trigger()->effectN( 1 ).radius_max();

    // Create child for DoT spell trigger
    child_unholy_transfusion = new unholy_transfusion_t( p, options_str );
    add_child( child_unholy_transfusion );

    // Create child for Healing AoE trigger
    child_unholy_nova_healing = new unholy_nova_healing_t( p );
    add_child( child_unholy_nova_healing );

    // Unholy Nova itself does NOT do damage, just the DoT
    base_dd_min = base_dd_max = spell_power_mod.direct = 0;
  }

  void impact( action_state_t* s ) override
  {
    // Pass in parent state to the DoT so we know how to scale the damage of the DoT
    if ( child_unholy_transfusion )
    {
      child_unholy_transfusion->target         = s->target;
      child_unholy_transfusion->parent_targets = s->n_targets;
      child_unholy_transfusion->execute();
    }

    // The AoE heal happens at your current target in an AoE radius around them
    if ( child_unholy_nova_healing )
    {
      child_unholy_nova_healing->target = s->target;
      child_unholy_nova_healing->execute();
    }

    if ( priest().legendary.pallid_command->ok() && !s->chain_target)
    {
      if ( priest().specialization() == PRIEST_SHADOW )
      {
        auto spawned_pets = priest().pets.rattling_mage.spawn();
      }
      else if ( priest().specialization() == PRIEST_DISCIPLINE )
      {
        auto spawned_pets = priest().pets.cackling_chemist.spawn();
      }
      else
      {
        sim->print_debug( "{} in current spec: {} does not have support for Pallid Command.", priest(),
                          priest().specialization() );
      }
    }

    priest_spell_t::impact( s );
  }
};

// ==========================================================================
// Mindgames - Venthyr Covenant
// ==========================================================================
struct mindgames_healing_reversal_t final : public priest_spell_t
{
  mindgames_healing_reversal_t( priest_t& p )
    : priest_spell_t( "mindgames_healing_reversal", p, p.covenant.mindgames_healing_reversal )
  {
    background        = true;
    may_crit          = false;
    energize_type     = action_energize::NONE;  // disable insanity gain (parent spell covers this)
    energize_amount   = 0;
    energize_resource = RESOURCE_NONE;

    // Formula found in parent spelldata for $healing
    // $healing=${($SPS*$s5/100)*(1+$@versadmg)*$m3/100}
    spell_power_mod.direct = ( priest().covenant.mindgames->effectN( 5 ).base_value() / 100 ) *
                             ( priest().covenant.mindgames->effectN( 3 ).base_value() / 100 );

    if ( priest().conduits.shattered_perceptions->ok() )
    {
      base_dd_multiplier *= ( 1.0 + priest().conduits.shattered_perceptions.percent() );
    }

    if ( priest().legendary.shadow_word_manipulation->ok() )
    {
      base_dd_multiplier *= ( 1.0 + priest().legendary.shadow_word_manipulation->effectN( 2 ).percent() );
    }
  }
};

struct mindgames_damage_reversal_t final : public priest_heal_t
{
  mindgames_damage_reversal_t( priest_t& p )
    : priest_heal_t( "mindgames_damage_reversal", p, p.covenant.mindgames_damage_reversal )
  {
    background        = true;
    harmful           = false;
    may_crit          = false;
    energize_type     = action_energize::NONE;  // disable insanity gain (parent spell covers this)
    energize_amount   = 0;
    energize_resource = RESOURCE_NONE;

    // Formula found in parent spelldata for $damage
    // $damage=${($SPS*$s2/100)*(1+$@versadmg)*$m3/100}
    spell_power_mod.direct = ( priest().covenant.mindgames->effectN( 2 ).base_value() / 100 ) *
                             ( priest().covenant.mindgames->effectN( 3 ).base_value() / 100 );

    if ( priest().conduits.shattered_perceptions->ok() )
    {
      base_dd_multiplier *= ( 1.0 + priest().conduits.shattered_perceptions.percent() );
    }

    if ( priest().legendary.shadow_word_manipulation->ok() )
    {
      base_dd_multiplier *= ( 1.0 + priest().legendary.shadow_word_manipulation->effectN( 2 ).percent() );
    }
  }
};

struct mindgames_t final : public priest_spell_t
{
  propagate_const<mindgames_healing_reversal_t*> child_mindgames_healing_reversal;
  propagate_const<mindgames_damage_reversal_t*> child_mindgames_damage_reversal;
  double insanity_gain;
  timespan_t shattered_perceptions_increase;

  mindgames_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "mindgames", p, p.covenant.mindgames ),
      child_mindgames_healing_reversal( nullptr ),
      child_mindgames_damage_reversal( nullptr ),
      insanity_gain( p.find_spell( 323706 )->effectN( 2 ).base_value() ),
      shattered_perceptions_increase( p.conduits.shattered_perceptions->effectN( 3 ).time_value() )
  {
    parse_options( options_str );

    affected_by_shadow_weaving = true;

    if ( priest().conduits.shattered_perceptions->ok() )
    {
      base_dd_multiplier *= ( 1.0 + priest().conduits.shattered_perceptions.percent() );
    }
    if ( priest().options.mindgames_healing_reversal )
    {
      child_mindgames_healing_reversal = new mindgames_healing_reversal_t( priest() );
      add_child( child_mindgames_healing_reversal );
    }
    if ( priest().options.mindgames_damage_reversal )
    {
      child_mindgames_damage_reversal = new mindgames_damage_reversal_t( priest() );
      add_child( child_mindgames_damage_reversal );
    }
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    // Mindgames gives a total of 20 insanity
    // 10 if the target deals enough dmg to break the shield
    // 10 if the targets heals enough to break the shield
    double insanity = 0;
    // Healing reversal creates damage
    if ( child_mindgames_healing_reversal )
    {
      insanity += insanity_gain;
      child_mindgames_healing_reversal->target = s->target;
      child_mindgames_healing_reversal->execute();
    }
    // Damage reversal creates healing
    if ( child_mindgames_damage_reversal )
    {
      insanity += insanity_gain;
      child_mindgames_damage_reversal->execute();
    }

    // TODO: Determine what happens if you break both shields
    if ( priest().legendary.shadow_word_manipulation->ok() )
    {
      timespan_t max_time_seconds = priest().covenant.mindgames->duration() +
                                    priest().legendary.shadow_word_manipulation->effectN( 1 ).time_value() +
                                    ( priest().conduits.shattered_perceptions->ok() * shattered_perceptions_increase );
      // You get 1 stack for each second remaining on Mindgames as a shield expires
      timespan_t stacks = timespan_t::from_seconds( priest().options.shadow_word_manipulation_seconds_remaining ) +
                          ( priest().conduits.shattered_perceptions->ok() * shattered_perceptions_increase );
      // the delay value is the inverse of how much time is remaining
      // i.e. if you have 6s remaining we need to delay the buff by 2 seconds
      timespan_t delay = max_time_seconds - stacks;
      make_event( *sim, delay,
                  [ this, stacks ] { priest().buffs.shadow_word_manipulation->trigger( stacks.total_seconds() ); } );
    }

    priest().generate_insanity( insanity, priest().gains.insanity_mindgames, s->action );
  }
};

// ==========================================================================
// Boon of the Ascended - Kyrian Covenant
// ==========================================================================
struct boon_of_the_ascended_t final : public priest_spell_t
{
  boon_of_the_ascended_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "boon_of_the_ascended", p, p.covenant.boon_of_the_ascended )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest().buffs.boon_of_the_ascended->trigger();
  }
};

struct ascended_nova_heal_t final : public priest_heal_t
{
  ascended_nova_heal_t( priest_t& p )
    : priest_heal_t( "ascended_nova_heal", p, p.covenant.ascended_nova->effectN( 2 ).trigger() )
  {
    background = true;
    aoe        = as<int>( data().effectN( 2 ).base_value() );

    // TODO: Confirm if this healing can proc trinkets/etc
    callbacks = false;
  }
};

struct ascended_nova_t final : public priest_spell_t
{
  propagate_const<ascended_nova_heal_t*> child_ascended_nova_heal;
  int grants_stacks;

  ascended_nova_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "ascended_nova", p, p.covenant.ascended_nova ),
      child_ascended_nova_heal( nullptr ),
      grants_stacks( as<int>( data().effectN( 3 ).base_value() ) )
  {
    parse_options( options_str );
    aoe                        = -1;
    radius                     = data().effectN( 1 ).radius_max();
    affected_by_shadow_weaving = true;

    child_ascended_nova_heal = new ascended_nova_heal_t( priest() );
    add_child( child_ascended_nova_heal );
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    // gain 1 stack for each target damaged
    if ( priest().buffs.boon_of_the_ascended->check() )
    {
      priest().buffs.boon_of_the_ascended->increment( grants_stacks );
    }
  }

  bool ready() override
  {
    if ( !priest().buffs.boon_of_the_ascended->check() || !priest().options.use_ascended_nova )
    {
      return false;
    }

    return priest_spell_t::ready();
  }

  double composite_aoe_multiplier( const action_state_t* state ) const override
  {
    double cam = priest_spell_t::composite_aoe_multiplier( state );

    return cam / std::sqrt( state->n_targets );
  }

  void execute() override
  {
    priest_spell_t::execute();

    if ( child_ascended_nova_heal )
    {
      child_ascended_nova_heal->execute();
    }
  }
};

struct ascended_blast_heal_t final : public priest_heal_t
{
  ascended_blast_heal_t( priest_t& p ) : priest_heal_t( "ascended_blast_heal", p, p.covenant.ascended_blast_heal )
  {
    background = true;
    may_crit   = false;

    // TODO: Confirm if this healing can proc trinkets/etc
    callbacks = false;
  }

  void trigger( double original_amount )
  {
    base_dd_min = base_dd_max = original_amount * priest().covenant.ascended_blast->effectN( 2 ).percent();
    execute();
  }
};

struct ascended_blast_t final : public priest_spell_t
{
  int grants_stacks;

  ascended_blast_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "ascended_blast", p, p.covenant.ascended_blast ),
      grants_stacks( as<int>( data().effectN( 3 ).base_value() ) )
  {
    parse_options( options_str );

    if ( priest().conduits.courageous_ascension->ok() )
    {
      base_dd_multiplier *= ( 1.0 + priest().conduits.courageous_ascension.percent() );
    }
    cooldown->hasted           = true;
    affected_by_shadow_weaving = true;
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    // gain 5 stacks on impact
    if ( priest().buffs.boon_of_the_ascended->check() )
    {
      priest().buffs.boon_of_the_ascended->increment( grants_stacks );
    }

    if ( result_is_hit( s->result ) )
    {
      priest().background_actions.ascended_blast_heal->trigger( s->result_amount );
    }
  }

  bool ready() override
  {
    if ( !priest().buffs.boon_of_the_ascended->check() )
    {
      return false;
    }

    return priest_spell_t::ready();
  }
};

struct ascended_eruption_heal_t final : public priest_heal_t
{
  int trigger_stacks;  // Set as action variable since this will not be triggered multiple times.
  double base_da_increase;

  ascended_eruption_heal_t( priest_t& p )
    : priest_heal_t( "ascended_eruption_heal", p, p.covenant.ascended_eruption->effectN( 2 ).trigger() ),
      base_da_increase( p.covenant.boon_of_the_ascended->effectN( 5 ).percent() +
                        p.conduits.courageous_ascension->effectN( 2 ).percent() )
  {
    aoe        = -1;
    background = true;

    // TODO: Confirm if this healing can proc trinkets/etc
    callbacks = false;
  }

  void trigger_eruption( int stacks )
  {
    assert( stacks > 0 );
    trigger_stacks = stacks;

    execute();
  }

  double action_da_multiplier() const override
  {
    double m = priest_heal_t::action_da_multiplier();

    m *= 1 + base_da_increase * trigger_stacks;

    return m;
  }

  double composite_aoe_multiplier( const action_state_t* state ) const override
  {
    double cam  = priest_heal_t::composite_aoe_multiplier( state );
    int targets = state->n_targets;
    sim->print_debug( "{} {} sets damage multiplier as if it hit an additional {} targets.", *player, *this,
                      priest().options.ascended_eruption_additional_targets );
    targets += priest().options.ascended_eruption_additional_targets;
    return cam / std::sqrt( targets );
  }
};

struct ascended_eruption_t final : public priest_spell_t
{
  int trigger_stacks;  // Set as action variable since this will not be triggered multiple times.
  double base_da_increase;

  ascended_eruption_t( priest_t& p )
    : priest_spell_t( "ascended_eruption", p, p.covenant.ascended_eruption ),
      base_da_increase( p.covenant.boon_of_the_ascended->effectN( 5 ).percent() +
                        p.conduits.courageous_ascension->effectN( 2 ).percent() )
  {
    aoe        = -1;
    background = true;
    radius     = data().effectN( 1 ).radius_max();
    // By default the spell tries to use the healing SP Coeff
    spell_power_mod.direct     = data().effectN( 1 ).sp_coeff();
    affected_by_shadow_weaving = true;
  }

  void trigger_eruption( int stacks )
  {
    assert( stacks > 0 );
    trigger_stacks = stacks;

    execute();
  }

  double action_da_multiplier() const override
  {
    double m = priest_spell_t::action_da_multiplier();

    m *= 1 + base_da_increase * trigger_stacks;

    return m;
  }

  double composite_aoe_multiplier( const action_state_t* state ) const override
  {
    double cam  = priest_spell_t::composite_aoe_multiplier( state );
    int targets = state->n_targets;
    sim->print_debug( "{} {} sets damage multiplier as if it hit an additional {} targets.", *player, *this,
                      priest().options.ascended_eruption_additional_targets );
    targets += priest().options.ascended_eruption_additional_targets;
    return cam / std::sqrt( targets );
  }
};

// ==========================================================================
// Summon Pet
// ==========================================================================
/// Priest Pet Summon Base Spell
struct summon_pet_t : public priest_spell_t
{
  timespan_t summoning_duration;
  std::string pet_name;
  propagate_const<pet_t*> pet;

public:
  summon_pet_t( util::string_view n, priest_t& p, const spell_data_t* sd = spell_data_t::nil() )
    : priest_spell_t( n, p, sd ), summoning_duration( timespan_t::zero() ), pet_name( n ), pet( nullptr )
  {
    harmful = false;
  }

  void init_finished() override
  {
    pet = player->find_pet( pet_name );

    priest_spell_t::init_finished();
  }

  void execute() override
  {
    if ( pet->is_sleeping() )
    {
      pet->summon( summoning_duration );
    }
    else
    {
      auto new_duration = pet->expiration->remains();
      new_duration += summoning_duration;
      pet->expiration->reschedule( new_duration );
    }

    priest_spell_t::execute();
  }

  bool ready() override
  {
    if ( !pet )
    {
      return false;
    }

    return priest_spell_t::ready();
  }
};

// ==========================================================================
// Summon Shadowfiend
// ==========================================================================
struct summon_shadowfiend_t final : public summon_pet_t
{
  summon_shadowfiend_t( priest_t& p, util::string_view options_str )
    : summon_pet_t( "shadowfiend", p, p.find_class_spell( "Shadowfiend" ) )
  {
    parse_options( options_str );
    harmful            = false;
    summoning_duration = data().duration();
  }
};

// ==========================================================================
// Summon Mindbender
// ==========================================================================
struct summon_mindbender_t final : public summon_pet_t
{
  summon_mindbender_t( priest_t& p, util::string_view options_str )
    : summon_pet_t( "mindbender", p, p.find_talent_spell( "Mindbender" ) )
  {
    parse_options( options_str );
    harmful            = false;
    summoning_duration = data().duration();
  }
};

/**
 * Discipline and shadow heal
 */
struct shadow_mend_t final : public priest_heal_t
{
  shadow_mend_t( priest_t& p, util::string_view options_str )
    : priest_heal_t( "shadow_mend", p, p.find_class_spell( "Shadow Mend" ) )
  {
    parse_options( options_str );
    harmful = false;

    // TODO: add harmful ticking effect 187464
  }
};

}  // namespace spells

namespace heals
{
// ==========================================================================
// Power Word: Shield
// ==========================================================================
struct power_word_shield_t final : public priest_absorb_t
{
  bool ignore_debuff;

  power_word_shield_t( priest_t& p, util::string_view options_str )
    : priest_absorb_t( "power_word_shield", p, p.find_class_spell( "Power Word: Shield" ) ), ignore_debuff( false )
  {
    add_option( opt_bool( "ignore_debuff", ignore_debuff ) );
    parse_options( options_str );

    spell_power_mod.direct = 5.5;  // hardcoded into tooltip, last checked 2017-03-18
  }

  void impact( action_state_t* s ) override
  {
    priest_absorb_t::impact( s );

    if ( priest().talents.body_and_soul->ok() && s->target->buffs.body_and_soul )
    {
      s->target->buffs.body_and_soul->trigger();
    }
  }
};

}  // namespace heals

}  // namespace actions

namespace buffs
{
// ==========================================================================
// Fae Guardians - Night Fae Covenant
// ==========================================================================
struct fae_guardians_t final : public priest_buff_t<buff_t>
{
  fae_guardians_t( priest_t& p ) : base_t( p, "fae_guardians", p.covenant.fae_guardians )
  {
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    priest().remove_wrathful_faerie();
    priest().remove_wrathful_faerie_fermata();
  }
};

// ==========================================================================
// Boon of the Ascended - Kyrian Covenant
// ==========================================================================
struct boon_of_the_ascended_t final : public priest_buff_t<buff_t>
{
  int stacks;
  propagate_const<cooldown_t*> boon_of_the_ascended_cooldown;

  boon_of_the_ascended_t( priest_t& p )
    : base_t( p, "boon_of_the_ascended", p.covenant.boon_of_the_ascended ),
      stacks( as<int>( data().max_stacks() ) ),
      boon_of_the_ascended_cooldown( p.get_cooldown( "boon_of_the_ascended" ) )
  {
    // Adding stacks should not refresh the duration
    set_refresh_behavior( buff_refresh_behavior::DISABLED );
    set_max_stack( stacks >= 1 ? stacks : 1 );

    // Cooldown is handled by the Boon of the Ascended spell
    set_cooldown( timespan_t::from_seconds( 0 ) );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    priest_buff_t<buff_t>::expire_override( expiration_stacks, remaining_duration );

    if ( priest().legendary.spheres_harmony->ok() && boon_of_the_ascended_cooldown )
    {
      auto max_reduction = priest().legendary.spheres_harmony->effectN( 2 ).base_value();
      auto adjust_amount = priest().legendary.spheres_harmony->effectN( 1 ).base_value() * expiration_stacks;

      if ( adjust_amount > max_reduction )
      {
        adjust_amount = max_reduction;
      }
      boon_of_the_ascended_cooldown->adjust( -timespan_t::from_seconds( adjust_amount ) );
      sim->print_debug(
          "{} adjusted cooldown of Boon of the Ascended, by {}, with Spheres' Harmony after having {} stacks.",
          priest(), adjust_amount, expiration_stacks );
    }

    if ( priest().options.use_ascended_eruption )
    {
      priest().background_actions.ascended_eruption->trigger_eruption( expiration_stacks );
      priest().background_actions.ascended_eruption_heal->trigger_eruption( expiration_stacks );
    }
  }
};

// ==========================================================================
// Surrender to Madness Debuff
// ==========================================================================
struct surrender_to_madness_debuff_t final : public priest_buff_t<buff_t>
{
  surrender_to_madness_debuff_t( priest_td_t& actor_pair )
    : base_t( actor_pair, "surrender_to_madness_death_check", actor_pair.priest().talents.surrender_to_madness )
  {
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    // Fake-detect target demise by checking if buff was expired early
    if ( remaining_duration > timespan_t::zero() )
    {
      priest().buffs.surrender_to_madness->expire();
    }
    else
    {
      make_event( sim, [ this ]() {
        if ( sim->log )
        {
          sim->out_log.printf( "%s %s: Surrender to Madness kills you. You die. Horribly.", priest().name(), name() );
        }
        priest().demise();
        priest().arise();
        priest().buffs.surrender_to_madness_death->trigger();
      } );
    }

    buff_t::expire_override( expiration_stacks, remaining_duration );
  }
};

// ==========================================================================
// Death and Madness Debuff
// ==========================================================================
struct death_and_madness_debuff_t final : public priest_buff_t<buff_t>
{
  propagate_const<cooldown_t*> swd_cooldown;
  death_and_madness_debuff_t( priest_td_t& actor_pair )
    : base_t( actor_pair, "death_and_madness_death_check",
              actor_pair.priest().talents.death_and_madness->effectN( 3 ).trigger() ),
      swd_cooldown( actor_pair.priest().get_cooldown( "shadow_word_death" ) )
  {
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    // Fake-detect target demise by checking if buff was expired early
    if ( remaining_duration > timespan_t::zero() )
    {
      sim->print_debug( "{} death_and_madness insanity gain buff triggered", priest() );

      priest().buffs.death_and_madness_buff->trigger();

      swd_cooldown->reset( true );
    }

    buff_t::expire_override( expiration_stacks, remaining_duration );
  }
};

struct benevolent_faerie_t final : public buff_t
{
private:
  std::vector<action_t*> affected_actions;
  bool affected_actions_initialized;
  priest_t* priest;

public:
  benevolent_faerie_t( player_t* p )
    : buff_t( p, "benevolent_faerie", p->find_spell( 327710 ) ),
      affected_actions_initialized( false ),
      priest( dynamic_cast<priest_t*>( player ) )
  {
    set_default_value_from_effect( 1 );

    set_stack_change_callback( [ this ]( buff_t*, int /* old_stack */, int current_stack ) {
      handle_benevolent_faerie_stack_change( current_stack );
    } );
  }

private:
  void handle_benevolent_faerie_stack_change( int new_stack )
  {
    double recharge_rate_multiplier = get_recharge_multiplier();

    // When going from 0 to 1 stacks, the recharge rate gets increase
    // When going back to 0 stacks, the recharge rate decreases again to its original level
    if ( new_stack == 0 )
      recharge_rate_multiplier = 1.0 / recharge_rate_multiplier;

    adjust_action_cooldown_rates( recharge_rate_multiplier );
  }

  void adjust_action_cooldown_rates( double rate_change )
  {
    if ( !affected_actions_initialized )
    {
      int label = data().effectN( 1 ).misc_value1();
      affected_actions.clear();
      for ( auto a : player->action_list )
      {
        if ( a->data().affected_by_label( label ) )
        {
          if ( range::find( affected_actions, a ) == affected_actions.end() )
          {
            affected_actions.push_back( a );
          }
        }
      }

      affected_actions_initialized = true;
    }
    for ( auto a : affected_actions )
    {
      a->base_recharge_rate_multiplier /= rate_change;

      sim->print_debug( "{} recharge_rate_multiplier set to {}", a->name_str, a->base_recharge_rate_multiplier );

      if ( a->cooldown->action == a )
        a->cooldown->adjust_recharge_multiplier();
      if ( a->internal_cooldown->action == a )
        a->internal_cooldown->adjust_recharge_multiplier();
    }
  }

  double get_recharge_multiplier()
  {
    double modifier = 1.0;
    if ( priest != nullptr && priest->legendary.bwonsamdis_pact->ok() &&
         util::str_compare_ci( priest->options.bwonsamdis_pact_mask_type, "benevolent" ) )
    {
      modifier += ( default_value * 2 );
      
      sim->print_debug( "Bwonsamdi's Pact Modifier set to {}", modifier );
    }
    else
    {
      modifier += default_value;
    }
    return modifier;
  }
};

struct rigor_mortis_t final : public priest_buff_t<buff_t>
{
  timespan_t rigor_mortis_duration;
  rigor_mortis_t( priest_t& p )
    : base_t( p, "rigor_mortis", p.find_spell( 357165 ) ), rigor_mortis_duration( p.find_spell( 356467 )->duration() )
  {
    // 1st effect is for healing
    set_default_value_from_effect( 2 );
    // Duration is in the legendary spell itself, base aura is infinite
    set_duration( rigor_mortis_duration );
  }
};
}  // namespace buffs

namespace items
{
void do_trinket_init( const priest_t* priest, specialization_e spec, const special_effect_t*& ptr,
                      const special_effect_t& effect )
{
  // Ensure we have the spell data. This will prevent the trinket effect from working on live Simulationcraft. Also
  // ensure correct specialization.
  if ( !priest->find_spell( effect.spell_id )->ok() || priest->specialization() != spec )
  {
    return;
  }

  // Set pointer, module considers non-null pointer to mean the effect is "enabled"
  ptr = &( effect );
}

using namespace unique_gear;

void init()
{
}

}  // namespace items

// ==========================================================================
// Priest Targetdata Definitions
// ==========================================================================

priest_td_t::priest_td_t( player_t* target, priest_t& p ) : actor_target_data_t( target, &p ), dots(), buffs()
{
  dots.shadow_word_pain   = target->get_dot( "shadow_word_pain", &p );
  dots.vampiric_touch     = target->get_dot( "vampiric_touch", &p );
  dots.devouring_plague   = target->get_dot( "devouring_plague", &p );
  dots.unholy_transfusion = target->get_dot( "unholy_transfusion", &p );

  buffs.schism                      = make_buff( *this, "schism", p.talents.schism );
  buffs.death_and_madness_debuff    = make_buff<buffs::death_and_madness_debuff_t>( *this );
  buffs.surrender_to_madness_debuff = make_buff<buffs::surrender_to_madness_debuff_t>( *this );
  buffs.wrathful_faerie             = make_buff( *this, "wrathful_faerie", p.find_spell( 327703 ) );
  buffs.wrathful_faerie_fermata     = make_buff( *this, "wrathful_faerie_fermata", p.find_spell( 345452 ) )
                                      ->set_cooldown( timespan_t::zero() )
                                      ->set_duration( priest().conduits.fae_fermata.time_value() );
  buffs.hungering_void = make_buff( *this, "hungering_void", p.find_spell( 345219 ) );

  target->register_on_demise_callback( &p, [ this ]( player_t* ) { target_demise(); } );
}

void priest_td_t::reset()
{
}

void priest_td_t::target_demise()
{
  priest().sim->print_debug( "{} demised. Priest {} resets targetdata for him.", *target, priest() );

  reset();
}

// ==========================================================================
// Priest Definitions
// ==========================================================================

priest_t::priest_t( sim_t* sim, util::string_view name, race_e r )
  : player_t( sim, PRIEST, name, r ),
    buffs(),
    talents(),
    specs(),
    dot_spells(),
    mastery_spells(),
    cooldowns(),
    rppm(),
    gains(),
    benefits(),
    procs(),
    background_actions(),
    active_items(),
    pets( *this ),
    options(),
    legendary(),
    conduits(),
    covenant()
{
  create_cooldowns();
  create_gains();
  create_procs();
  create_benefits();

  resource_regeneration = regen_type::DYNAMIC;
}

/** Construct priest cooldowns */
void priest_t::create_cooldowns()
{
  cooldowns.wrathful_faerie         = get_cooldown( "wrathful_faerie" );
  cooldowns.wrathful_faerie_fermata = get_cooldown( "wrathful_faerie_fermata" );
  cooldowns.holy_fire               = get_cooldown( "holy_fire" );
  cooldowns.holy_word_serenity      = get_cooldown( "holy_word_serenity" );
  cooldowns.void_bolt               = get_cooldown( "void_bolt" );
  cooldowns.mind_blast              = get_cooldown( "mind_blast" );
  cooldowns.void_eruption           = get_cooldown( "void_eruption" );
}

/** Construct priest gains */
void priest_t::create_gains()
{
  gains.cauterizing_shadows_health  = get_gain( "Health from Cauterizing Shadows" );
  gains.insanity_auspicious_spirits = get_gain( "Insanity Gained from Auspicious Spirits" );
  gains.insanity_death_and_madness  = get_gain( "Insanity Gained from Death and Madness" );
  gains.insanity_eternal_call_to_the_void_mind_flay =
      get_gain( "Insanity Gained from Eternal Call to the Void Mind Flay's" );
  gains.insanity_eternal_call_to_the_void_mind_sear =
      get_gain( "Insanity Gained from Eternal Call to the Void Mind Sear's" );
  gains.insanity_mindgames = get_gain( "Insanity Gained from Mindgames" );

  gains.insanity_mind_sear            = get_gain( "Insanity Gained from Mind Sear" );
  gains.insanity_pet                  = get_gain( "Insanity Gained from Shadowfiend" );
  gains.insanity_surrender_to_madness = get_gain( "Insanity Gained from Surrender to Madness" );
  gains.mindbender                    = get_gain( "Mana Gained from Mindbender" );
  gains.painbreaker_psalm             = get_gain( "Insanity Gained from Painbreaker Psalm" );
  gains.power_word_solace             = get_gain( "Mana Gained from Power Word: Solace" );
  gains.shadow_word_death_self_damage = get_gain( "Shadow Word: Death self inflicted damage" );
}

/** Construct priest procs */
void priest_t::create_procs()
{
  procs.shadowy_apparition              = get_proc( "Shadowy Apparition" );
  procs.shadowy_apparition_overflow     = get_proc( "Shadowy Apparition Insanity lost to overflow" );
  procs.surge_of_light                  = get_proc( "Surge of Light" );
  procs.surge_of_light_overflow         = get_proc( "Surge of Light lost to overflow" );
  procs.serendipity                     = get_proc( "Serendipity (Non-Tier 17 4pc)" );
  procs.serendipity_overflow            = get_proc( "Serendipity lost to overflow (Non-Tier 17 4pc)" );
  procs.power_of_the_dark_side          = get_proc( "Power of the Dark Side Penance damage buffed" );
  procs.power_of_the_dark_side_overflow = get_proc( "Power of the Dark Side lost to overflow" );
  procs.dissonant_echoes                = get_proc( "Void Bolt resets from Dissonant Echoes" );
  procs.mind_devourer                   = get_proc( "Mind Devourer free Devouring Plague proc" );
  procs.void_tendril                    = get_proc( "Void Tendril proc from Eternal Call to the Void" );
  procs.void_lasher                     = get_proc( "Void Lasher proc from Eternal Call to the Void" );
  procs.dark_thoughts_flay              = get_proc( "Dark Thoughts proc from Mind Flay" );
  procs.dark_thoughts_sear              = get_proc( "Dark Thoughts proc from Mind Sear" );
  procs.dark_thoughts_missed            = get_proc( "Dark Thoughts proc not consumed" );
}

/** Construct priest benefits */
void priest_t::create_benefits()
{
}

/**
 * Define the acting role of the priest().
 *
 * If base_t::primary_role() has a valid role defined, use it, otherwise select spec-based default.
 */
role_e priest_t::primary_role() const
{
  switch ( base_t::primary_role() )
  {
    case ROLE_HEAL:
      return ROLE_HEAL;
    case ROLE_DPS:
    case ROLE_SPELL:
      return ROLE_SPELL;
    case ROLE_ATTACK:
      return ROLE_SPELL;
    default:
      if ( specialization() == PRIEST_HOLY )
      {
        return ROLE_HEAL;
      }
      break;
  }

  return ROLE_SPELL;
}

/**
 * @brief Convert hybrid stats
 *
 *  Converts hybrid stats that either morph based on spec or only work
 *  for certain specs into the appropriate "basic" stats
 */
stat_e priest_t::convert_hybrid_stat( stat_e s ) const
{
  switch ( s )
  {
    case STAT_STR_AGI_INT:
    case STAT_STR_INT:
    case STAT_AGI_INT:
      return STAT_INTELLECT;
    case STAT_STR_AGI:
      return STAT_NONE;
    case STAT_SPIRIT:
      if ( specialization() != PRIEST_SHADOW )
      {
        return s;
      }
      return STAT_NONE;
    case STAT_BONUS_ARMOR:
      return STAT_NONE;
    default:
      return s;
  }
}

std::unique_ptr<expr_t> priest_t::create_expression( util::string_view expression_str )
{
  auto shadow_expression = create_expression_shadow( expression_str );
  if ( shadow_expression )
  {
    return shadow_expression;
  }

  auto splits = util::string_split<util::string_view>( expression_str, "." );

  if ( auto pet_expr = create_pet_expression( expression_str, splits ) )
  {
    return pet_expr;
  }

  if ( splits.size() >= 2 )
  {
    if ( util::str_compare_ci( splits[ 0 ], "priest" ) )
    {
      if ( util::str_compare_ci( splits[ 1 ], "self_power_infusion" ) )
      {
        return expr_t::create_constant( "self_power_infusion", options.self_power_infusion );
      }
      throw std::invalid_argument( fmt::format( "Unsupported priest expression '{}'.", splits[ 1 ] ) );
    }
  }

  return player_t::create_expression( expression_str );
}  // namespace priestspace

void priest_t::assess_damage( school_e school, result_amount_type dtype, action_state_t* s )
{
  player_t::assess_damage( school, dtype, s );
}

double priest_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  return h;
}

double priest_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  return h;
}

double priest_t::composite_spell_crit_chance() const
{
  double sc = player_t::composite_spell_crit_chance();

  if ( talents.ancient_madness->ok() )
  {
    if ( buffs.ancient_madness->check() )
    {
      sim->print_debug( "Ancient Madness - adjusting crit increase to {}", buffs.ancient_madness->check_stack_value() );
      sc += buffs.ancient_madness->check_stack_value();
    }
  }
  return sc;
}

double priest_t::composite_player_pet_damage_multiplier( const action_state_t* s, bool guardian ) const
{
  double m = player_t::composite_player_pet_damage_multiplier( s, guardian );
  m *= ( 1.0 + specs.shadow_priest->effectN( 3 ).percent() );
  return m;
}

double priest_t::composite_player_heal_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_heal_multiplier( s );

  if ( buffs.twist_of_fate->check() )
  {
    m *= 1.0 + buffs.twist_of_fate->current_value;
  }

  if ( mastery_spells.grace->ok() )
  {
    m *= 1 + cache.mastery_value();
  }

  return m;
}

double priest_t::composite_player_absorb_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_absorb_multiplier( s );

  if ( mastery_spells.grace->ok() )
  {
    m *= 1 + cache.mastery_value();
  }

  return m;
}

double priest_t::composite_player_target_multiplier( player_t* t, school_e school ) const
{
  double m = player_t::composite_player_target_multiplier( t, school );

  if ( hungering_void_active( t ) )
  {
    m *= ( 1 + talents.hungering_void_buff->effectN( 1 ).percent() );
  }

  return m;
}

double priest_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( attr == ATTR_INTELLECT )
  {
    return 0.05;
  }

  return 0.0;
}

action_t* priest_t::create_action( util::string_view name, const std::string& options_str )
{
  using namespace actions::spells;
  using namespace actions::heals;

  action_t* shadow_action = create_action_shadow( name, options_str );
  if ( shadow_action )
  {
    return shadow_action;
  }

  action_t* discipline_action = create_action_discipline( name, options_str );
  if ( discipline_action )
  {
    return discipline_action;
  }

  action_t* holy_action = create_action_holy( name, options_str );
  if ( holy_action )
  {
    return holy_action;
  }

  if ( name == "smite" )
  {
    return new smite_t( *this, options_str );
  }
  if ( name == "angelic_feather" )
  {
    return new angelic_feather_t( *this, options_str );
  }
  if ( name == "halo" )
  {
    return new halo_t( *this, options_str );
  }
  if ( name == "divine_star" )
  {
    return new divine_star_t( *this, options_str );
  }
  if ( name == "levitate" )
  {
    return new levitate_t( *this, options_str );
  }
  if ( name == "power_word_shield" )
  {
    return new power_word_shield_t( *this, options_str );
  }
  if ( name == "power_word_fortitude" )
  {
    return new power_word_fortitude_t( *this, options_str );
  }
  if ( ( name == "shadowfiend" ) || ( name == "mindbender" ) )
  {
    if ( talents.mindbender->ok() )
    {
      return new summon_mindbender_t( *this, options_str );
    }
    else
    {
      return new summon_shadowfiend_t( *this, options_str );
    }
  }
  if ( name == "shadow_mend" )
  {
    return new shadow_mend_t( *this, options_str );
  }
  if ( name == "power_infusion" )
  {
    return new power_infusion_t( *this, options_str, "power_infusion" );
  }
  if ( name == "power_infusion_other" )
  {
    return new power_infusion_t( *this, options_str, "power_infusion_other" );
  }
  if ( name == "fae_guardians" )
  {
    return new fae_guardians_t( *this, options_str );
  }
  if ( name == "unholy_nova" )
  {
    return new unholy_nova_t( *this, options_str );
  }
  if ( name == "mindgames" )
  {
    return new mindgames_t( *this, options_str );
  }
  if ( name == "boon_of_the_ascended" )
  {
    return new boon_of_the_ascended_t( *this, options_str );
  }
  if ( name == "ascended_nova" )
  {
    return new ascended_nova_t( *this, options_str );
  }
  if ( name == "ascended_blast" )
  {
    return new ascended_blast_t( *this, options_str );
  }

  return base_t::create_action( name, options_str );
}

void priest_t::create_pets()
{
  base_t::create_pets();

  if ( find_action( "shadowfiend" ) && !talents.mindbender->ok() )
  {
    pets.shadowfiend = create_pet( "shadowfiend" );
  }

  if ( ( find_action( "mindbender" ) || find_action( "shadowfiend" ) ) && talents.mindbender->ok() )
  {
    pets.mindbender = create_pet( "mindbender" );
  }
}

void priest_t::trigger_unholy_transfusion_healing()
{
  background_actions.unholy_transfusion_healing->trigger();
}

void priest_t::trigger_wrathful_faerie()
{
  background_actions.wrathful_faerie->trigger();
}

void priest_t::trigger_wrathful_faerie_fermata()
{
  background_actions.wrathful_faerie_fermata->trigger();
}

void priest_t::init_base_stats()
{
  if ( base.distance < 1 )
  {
    if ( specialization() == PRIEST_DISCIPLINE || specialization() == PRIEST_HOLY )
    {
      // Range Based on Talents
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
    else if ( specialization() == PRIEST_SHADOW )
    {
      // Need to be 8 yards away for Ascended Nova
      // Need to be 10 yards - SC radius for Shadow Crash
      base.distance = 8.0;
    }
  }

  base_t::init_base_stats();

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility  = 0.0;
  base.spell_power_per_intellect = 1.0;

  if ( specialization() == PRIEST_SHADOW )
  {
    resources.base[ RESOURCE_INSANITY ] = 100.0;
  }

  resources.base_regen_per_second[ RESOURCE_MANA ] *= 1.0 + talents.enlightenment->effectN( 1 ).percent();
}

void priest_t::init_resources( bool force )
{
  base_t::init_resources( force );
}

void priest_t::init_scaling()
{
  base_t::init_scaling();
}

void priest_t::init_finished()
{
  base_t::init_finished();

  buffs.sephuzs_proclamation = buff_t::find( this, "sephuzs_proclamation" );
}

void priest_t::init_spells()
{
  base_t::init_spells();

  init_spells_shadow();
  init_spells_discipline();
  init_spells_holy();

  // Generic Spells
  specs.mind_blast         = find_class_spell( "Mind Blast" );
  specs.mind_sear          = find_class_spell( "Mind Sear" );
  specs.mind_sear_insanity = find_spell( 208232 );  // Insanity is stored here, not in any spell triggers
  specs.shadow_word_death  = find_class_spell( "Shadow Word: Death" );

  // Class passives
  specs.priest            = dbc::get_class_passive( *this, SPEC_NONE );
  specs.holy_priest       = dbc::get_class_passive( *this, PRIEST_HOLY );
  specs.discipline_priest = dbc::get_class_passive( *this, PRIEST_DISCIPLINE );
  specs.shadow_priest     = dbc::get_class_passive( *this, PRIEST_SHADOW );

  // DoT Spells
  dot_spells.devouring_plague = find_class_spell( "Devouring Plague" );
  dot_spells.shadow_word_pain = find_class_spell( "Shadow Word: Pain" );
  dot_spells.vampiric_touch   = find_class_spell( "Vampiric Touch" );

  // Mastery Spells
  mastery_spells.grace          = find_mastery_spell( PRIEST_DISCIPLINE );
  mastery_spells.echo_of_light  = find_mastery_spell( PRIEST_HOLY );
  mastery_spells.shadow_weaving = find_mastery_spell( PRIEST_SHADOW );

  // Generic Legendaries
  legendary.sephuzs_proclamation = find_runeforge_legendary( "Sephuz's Proclamation" );
  // Shared Legendaries
  legendary.cauterizing_shadows        = find_runeforge_legendary( "Cauterizing Shadows" );
  legendary.twins_of_the_sun_priestess = find_runeforge_legendary( "Twins of the Sun Priestess" );
  legendary.bwonsamdis_pact            = find_runeforge_legendary( "Bwonsamdi's Pact" );
  legendary.shadow_word_manipulation   = find_runeforge_legendary( "Shadow Word: Manipulation" );
  legendary.spheres_harmony            = find_runeforge_legendary( "Spheres' Harmony" );
  legendary.pallid_command             = find_runeforge_legendary( "Pallid Command" );
  // Disc legendaries
  legendary.kiss_of_death    = find_runeforge_legendary( "Kiss of Death" );
  legendary.the_penitent_one = find_runeforge_legendary( "The Penitent One" );
  // Shadow Legendaries
  legendary.eternal_call_to_the_void = find_runeforge_legendary( "Eternal Call to the Void" );
  legendary.painbreaker_psalm        = find_runeforge_legendary( "Painbreaker Psalm" );
  legendary.shadowflame_prism        = find_runeforge_legendary( "Shadowflame Prism" );
  legendary.talbadars_stratagem      = find_runeforge_legendary( "Talbadar's Stratagem" );

  // Generic Conduits
  conduits.power_unto_others = find_conduit_spell( "Power Unto Others" );
  // Shadow Conduits
  conduits.dissonant_echoes     = find_conduit_spell( "Dissonant Echoes" );
  conduits.haunting_apparitions = find_conduit_spell( "Haunting Apparitions" );
  conduits.mind_devourer        = find_conduit_spell( "Mind Devourer" );
  conduits.rabid_shadows        = find_conduit_spell( "Rabid Shadows" );
  // Covenant Conduits
  conduits.courageous_ascension  = find_conduit_spell( "Courageous Ascension" );
  conduits.festering_transfusion = find_conduit_spell( "Festering Transfusion" );
  conduits.fae_fermata           = find_conduit_spell( "Fae Fermata" );
  conduits.shattered_perceptions = find_conduit_spell( "Shattered Perceptions" );

  // Covenant Abilities
  covenant.ascended_blast             = find_spell( 325283 );
  covenant.ascended_blast_heal        = find_spell( 325315 );
  covenant.ascended_eruption          = find_spell( 325326 );
  covenant.ascended_nova              = find_spell( 325020 );
  covenant.boon_of_the_ascended       = find_covenant_spell( "Boon of the Ascended" );
  covenant.fae_guardians              = find_covenant_spell( "Fae Guardians" );
  covenant.mindgames                  = find_covenant_spell( "Mindgames" );
  covenant.mindgames_healing_reversal = find_spell( 323707 );
  covenant.mindgames_damage_reversal  = find_spell( 323706 );
  covenant.unholy_nova                = find_covenant_spell( "Unholy Nova" );
}

void priest_t::create_buffs()
{
  base_t::create_buffs();

  // Shared talent buffs
  buffs.twist_of_fate = make_buff( this, "twist_of_fate", talents.twist_of_fate->effectN( 1 ).trigger() )
                            ->set_trigger_spell( talents.twist_of_fate )
                            ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                            ->add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );

  // Shared buffs
  buffs.the_penitent_one = make_buff( this, "the_penitent_one", legendary.the_penitent_one->effectN( 1 ).trigger() )
                               ->set_trigger_spell( legendary.the_penitent_one );

  // Runeforge Buffs
  buffs.shadow_word_manipulation = make_buff( this, "shadow_word_manipulation", find_spell( 357028 ) )
                                       ->set_pct_buff_type( STAT_PCT_BUFF_CRIT )
                                       ->set_default_value_from_effect_type( A_MOD_ALL_CRIT_CHANCE );
  buffs.rigor_mortis = make_buff<buffs::rigor_mortis_t>( *this )->set_default_value_from_effect( 2 );

  // Covenant Buffs
  buffs.fae_guardians        = make_buff<buffs::fae_guardians_t>( *this );
  buffs.boon_of_the_ascended = make_buff<buffs::boon_of_the_ascended_t>( *this );

  create_buffs_shadow();
  create_buffs_discipline();
  create_buffs_holy();
}

void priest_t::init_rng()
{
  init_rng_shadow();
  init_rng_discipline();
  init_rng_holy();

  player_t::init_rng();
}

void priest_t::init_background_actions()
{
  background_actions.ascended_blast_heal = new actions::spells::ascended_blast_heal_t( *this );

  background_actions.ascended_eruption = new actions::spells::ascended_eruption_t( *this );

  background_actions.ascended_eruption_heal = new actions::spells::ascended_eruption_heal_t( *this );

  background_actions.wrathful_faerie = new actions::spells::wrathful_faerie_t( *this );

  background_actions.wrathful_faerie_fermata = new actions::spells::wrathful_faerie_fermata_t( *this );

  background_actions.unholy_transfusion_healing = new actions::spells::unholy_transfusion_healing_t( *this );

  init_background_actions_shadow();
}

void priest_t::do_dynamic_regen( bool forced )
{
  player_t::do_dynamic_regen( forced );
}

void priest_t::apply_affecting_auras( action_t& action )
{
  player_t::apply_affecting_auras( action );

  action.apply_affecting_aura( specs.shadow_priest );
  action.apply_affecting_aura( specs.holy_priest );
  action.apply_affecting_aura( specs.discipline_priest );
}

void priest_t::invalidate_cache( cache_e cache )
{
  player_t::invalidate_cache( cache );

  switch ( cache )
  {
    case CACHE_MASTERY:
      if ( mastery_spells.grace->ok() )
        player_t::invalidate_cache( CACHE_PLAYER_HEAL_MULTIPLIER );
      break;
    default:
      break;
  }
}

std::string priest_t::default_potion() const
{
  return priest_apl::potion( this );
}

std::string priest_t::default_flask() const
{
  return priest_apl::flask( this );
}

std::string priest_t::default_food() const
{
  return priest_apl::food( this );
}

std::string priest_t::default_rune() const
{
  return priest_apl::rune( this );
}

std::string priest_t::default_temporary_enchant() const
{
  return priest_apl::temporary_enchant( this );
}

const priest_td_t* priest_t::find_target_data( const player_t* target ) const
{
  return _target_data[ target ];
}

/**
 * Obtain target_data for given target.
 * Always returns non-null targetdata pointer
 */
priest_td_t* priest_t::get_target_data( player_t* target ) const
{
  priest_td_t*& td = _target_data[ target ];
  if ( !td )
  {
    td = new priest_td_t( target, const_cast<priest_t&>( *this ) );
  }
  return td;
}

void priest_t::init_action_list()
{
  // 2020-12-31: Healing is outdated and not supported (both discipline and holy)
  if ( !sim->allow_experimental_specializations && primary_role() == ROLE_HEAL )
  {
    if ( !quiet )
      sim->error( "Role heal for priest '{}' is currently not supported.", name() );

    quiet = true;
    return;
  }

  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }
  clear_action_priority_lists();

  switch ( specialization() )
  {
    case PRIEST_SHADOW:
      priest_apl::shadow( this );
      break;
    case PRIEST_DISCIPLINE:
      priest_apl::discipline( this );
      break;
    case PRIEST_HOLY:
      priest_apl::holy( this );
      break;
    default:
      priest_apl::no_spec( this );
      break;
  }

  use_default_action_list = true;

  base_t::init_action_list();
}

void priest_t::combat_begin()
{
  player_t::combat_begin();
}

// priest_t::reset ==========================================================

void priest_t::reset()
{
  base_t::reset();

  // Reset Target Data
  for ( player_t* target : sim->target_list )
  {
    if ( auto td = _target_data[ target ] )
    {
      td->reset();
    }
  }
}

void priest_t::target_mitigation( school_e school, result_amount_type dt, action_state_t* s )
{
  base_t::target_mitigation( school, dt, s );

  if ( buffs.dispersion->check() )
  {
    s->result_amount *= 1.0 + ( buffs.dispersion->data().effectN( 1 ).percent() );
  }
}

void priest_t::create_options()
{
  base_t::create_options();

  add_option( opt_deprecated( "autounshift", "priest.autounshift" ) );
  add_option( opt_deprecated( "priest_fixed_time", "priest.fixed_time" ) );
  add_option( opt_deprecated( "priest_use_ascended_nova", "priest.use_ascended_nova" ) );
  add_option( opt_deprecated( "priest_use_ascended_eruption", "priest.use_ascended_eruption" ) );
  add_option( opt_deprecated( "priest_mindgames_healing_reversal", "priest.mindgames_healing_reversal" ) );
  add_option( opt_deprecated( "priest_mindgames_damage_reversal", "priest.mindgames_damage_reversal" ) );
  add_option( opt_deprecated( "priest_self_power_infusion", "priest.self_power_infusion" ) );
  add_option( opt_deprecated( "priest_self_benevolent_faerie", "priest.self_benevolent_faerie" ) );
  add_option(
      opt_deprecated( "priest_ascended_eruption_additional_targets", "priest.ascended_eruption_additional_targets" ) );
  add_option( opt_deprecated( "priest_cauterizing_shadows_allies", "priest.cauterizing_shadows_allies" ) );

  add_option( opt_bool( "priest.autounshift", options.autoUnshift ) );
  add_option( opt_bool( "priest.fixed_time", options.fixed_time ) );
  add_option( opt_bool( "priest.use_ascended_nova", options.use_ascended_nova ) );
  add_option( opt_bool( "priest.use_ascended_eruption", options.use_ascended_eruption ) );
  add_option( opt_bool( "priest.mindgames_healing_reversal", options.mindgames_healing_reversal ) );
  add_option( opt_bool( "priest.mindgames_damage_reversal", options.mindgames_damage_reversal ) );
  add_option( opt_bool( "priest.self_power_infusion", options.self_power_infusion ) );
  add_option( opt_bool( "priest.self_benevolent_faerie", options.self_benevolent_faerie ) );
  add_option( opt_int( "priest.ascended_eruption_additional_targets", options.ascended_eruption_additional_targets ) );
  add_option( opt_int( "priest.cauterizing_shadows_allies", options.cauterizing_shadows_allies ) );
  add_option( opt_string( "priest.bwonsamdis_pact_mask_type", options.bwonsamdis_pact_mask_type ) );
  add_option( opt_int( "priest.shadow_word_manipulation_seconds_remaining",
                       options.shadow_word_manipulation_seconds_remaining, 0, 8 ) );
  add_option( opt_int( "priest.pallid_command_allies", options.pallid_command_allies, 0, 50 ) );
}

std::string priest_t::create_profile( save_e type )
{
  std::string profile_str = base_t::create_profile( type );

  if ( type & SAVE_PLAYER )
  {
    if ( !options.autoUnshift )
    {
      profile_str += fmt::format( "priest.autounshift={}\n", options.autoUnshift );
    }

    if ( !options.fixed_time )
    {
      profile_str += fmt::format( "priest.fixed_time={}\n", options.fixed_time );
    }
  }

  return profile_str;
}

void priest_t::copy_from( player_t* source )
{
  base_t::copy_from( source );

  priest_t* source_p = debug_cast<priest_t*>( source );

  options = source_p->options;
}

void priest_t::arise()
{
  base_t::arise();
}

// Legendary Eternal Call to the Void trigger
void priest_t::trigger_eternal_call_to_the_void( action_state_t* s )
{
  auto mind_sear_id = specs.mind_sear->effectN( 1 ).trigger()->id();
  auto mind_flay_id = specs.mind_flay->id();
  auto action_id    = s->action->id;
  if ( !legendary.eternal_call_to_the_void->ok() )
    return;

  if ( rppm.eternal_call_to_the_void->trigger() )
  {
    if ( action_id == mind_flay_id )
    {
      procs.void_tendril->occur();
      auto spawned_pets = pets.void_tendril.spawn();
    }
    else if ( action_id == mind_sear_id )
    {
      procs.void_lasher->occur();
      auto spawned_pets = pets.void_lasher.spawn();
    }
  }
}

// Fae Guardian Wrathful Faerie helper
void priest_t::remove_wrathful_faerie()
{
  for ( priest_td_t* priest_td : _target_data.get_entries() )
  {
    if ( priest_td && priest_td->buffs.wrathful_faerie->check() )
    {
      priest_td->buffs.wrathful_faerie->expire();

      // If you have the conduit enabled, clear out the conduit buff and trigger it on the old Wrathful Faerie target
      if ( conduits.fae_fermata->ok() && buffs.fae_guardians->check() )
      {
        priest_td->buffs.wrathful_faerie_fermata->trigger();
      }
    }
  }
}

// Fae Guardian Wrathful Faerie conduit buff helper
// TODO: this might not be needed anymore
void priest_t::remove_wrathful_faerie_fermata()
{
  if ( !conduits.fae_fermata->ok() )
  {
    return;
  }

  for ( priest_td_t* priest_td : _target_data.get_entries() )
  {
    if ( priest_td && priest_td->buffs.wrathful_faerie->check() )
    {
      priest_td->buffs.wrathful_faerie_fermata->expire();
    }
  }
}

int priest_t::shadow_weaving_active_dots( const player_t* target, const unsigned int spell_id ) const
{
  int dots = 0;

  if ( buffs.voidform->check() )
  {
    dots = 3;
  }
  else
  {
    if ( const priest_td_t* td = find_target_data( target ) )
    {
      const dot_t* swp = td->dots.shadow_word_pain;
      const dot_t* vt  = td->dots.vampiric_touch;
      const dot_t* dp  = td->dots.devouring_plague;

      // You get mastery benefit for a DoT as if it was active, if you are actively putting that DoT up
      // So to get the mastery benefit you either have that DoT ticking, or you are casting it
      bool swp_ticking = ( spell_id == dot_spells.shadow_word_pain->id() ) || swp->is_ticking();
      bool vt_ticking  = ( spell_id == dot_spells.vampiric_touch->id() ) || vt->is_ticking();
      bool dp_ticking  = ( spell_id == dot_spells.devouring_plague->id() ) || dp->is_ticking();

      dots = swp_ticking + vt_ticking + dp_ticking;
    }
  }

  return dots;
}

double priest_t::shadow_weaving_multiplier( const player_t* target, const unsigned int spell_id ) const
{
  double multiplier = 1.0;

  if ( mastery_spells.shadow_weaving->ok() )
  {
    auto dots = shadow_weaving_active_dots( target, spell_id );

    if ( dots > 0 )
    {
      multiplier *= 1 + dots * cache.mastery_value();
    }
  }

  return multiplier;
}

struct priest_module_t final : public module_t
{
  priest_module_t() : module_t( PRIEST )
  {
  }

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    return new priest_t( sim, name, r );
  }
  bool valid() const override
  {
    return true;
  }
  void init( player_t* p ) const override
  {
    p->buffs.guardian_spirit   = make_buff( p, "guardian_spirit",
                                          p->find_spell( 47788 ) );  // Let the ability handle the CD
    p->buffs.pain_suppression  = make_buff( p, "pain_suppression",
                                           p->find_spell( 33206 ) );  // Let the ability handle the CD
    p->buffs.benevolent_faerie = make_buff<buffs::benevolent_faerie_t>( p );
  }
  void static_init() const override
  {
    items::init();
  }
  void register_hotfixes() const override
  {
  }
  void combat_begin( sim_t* ) const override
  {
  }
  void combat_end( sim_t* ) const override
  {
  }
};

}  // namespace priestspace

const module_t* module_t::priest()
{
  static priestspace::priest_module_t m;
  return &m;
}
