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
// ==========================================================================
// Angelic Feather
// ==========================================================================
struct angelic_feather_t final : public priest_spell_t
{
  angelic_feather_t( priest_t& p, const std::string& options_str )
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
  typedef Base ab;  // the action base ("ab") type (priest_spell_t or priest_heal_t)
public:
  typedef divine_star_base_t base_t;

  propagate_const<divine_star_base_t*> return_spell;

  divine_star_base_t( const std::string& n, priest_t& p, const spell_data_t* spell_data, bool is_return_spell = false )
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
  divine_star_t( priest_t& p, const std::string& options_str )
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
  halo_base_t( const std::string& n, priest_t& p, const spell_data_t* s ) : Base( n, p, s )
  {
    Base::aoe        = -1;
    Base::background = true;

    if ( Base::data().ok() )
    {
      // Reparse the correct effect number, because we have two competing ones ( were 2 > 1 always wins out )
      Base::parse_effect_data( Base::data().effectN( 1 ) );
    }
    Base::radius = 30;
    Base::range  = 0;
  }

  timespan_t distance_targeting_travel_time( action_state_t* s ) const override
  {
    return timespan_t::from_seconds( s->action->player->get_player_distance( *s->target ) / Base::travel_speed );
  }

  double calculate_direct_amount( action_state_t* s ) const override
  {
    double cda = Base::calculate_direct_amount( s );

    // Source: Ghostcrawler 2012-06-20 http://us.battle.net/wow/en/forum/topic/5889309137?page=5#97

    double distance;
    distance = s->action->player->get_player_distance( *s->target );

    // double mult = 0.5 * pow( 1.01, -1 * pow( ( distance - 25 ) / 2, 4 ) ) + 0.1 + 0.015 * distance;
    double mult = 0.5 * exp( -0.00995 * pow( distance / 2 - 12.5, 4 ) ) + 0.1 + 0.015 * distance;

    return cda * mult;
  }
};

struct halo_t final : public priest_spell_t
{
  halo_t( priest_t& p, const std::string& options_str )
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
  levitate_t( priest_t& p, const std::string& options_str )
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
  power_word_fortitude_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "power_word_fortitude", p, p.find_class_spell("Power Word: Fortitude") )
  {
    parse_options( options_str );
    harmful = false;
    ignore_false_positive = true;

    background = sim -> overrides.power_word_fortitude != 0;
  }

  virtual void execute() override
  {
    priest_spell_t::execute();

    if ( ! sim -> overrides.power_word_fortitude )
      sim -> auras.power_word_fortitude -> trigger();
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
    smite_t(priest_t& p, const std::string& options_str)
      : priest_spell_t("smite", p, p.find_class_spell("Smite")),
      holy_fire_rank2(priest().find_specialization_spell(231687)),
      holy_word_chastise(priest().find_specialization_spell(88625)),
      smite_rank2(priest().find_specialization_spell(262861))
    {
       parse_options(options_str);
       if( smite_rank2 -> ok() )
       {
       base_multiplier *= 1.0 + smite_rank2->effectN(1).percent();
       }
    }

    void impact(action_state_t* s) override
    {
         priest_spell_t::impact(s);
        if (holy_fire_rank2->ok() && s->result_amount > 0)
        {
            double hf_proc_chance = holy_fire_rank2->effectN(1).percent();
            if (rng().roll(hf_proc_chance))
            {
                if (sim->debug)
                {
                    sim->out_debug.printf("%s reset holy fire %s cooldown, using smite. ", priest().name(), name());
                }
                priest().cooldowns.holy_fire->reset(true);
            }
        }
        if (sim->debug)
        {
             sim->out_debug.printf("%s checking for Apotheosis buff and Light of the Naaru talent. ", priest().name(), name());
        }
        if (s->result_amount > 0 && priest().buffs.apotheosis->up())
        {
            priest().cooldowns.holy_word_chastise->adjust((-10 * priest().talents.apotheosis->effectN(1).base_value() * holy_word_chastise->effectN(2).time_value()));
            double cd1 = -10 * priest().talents.apotheosis->effectN(1).base_value() * holy_word_chastise->effectN(2).base_value();
            if (sim->debug)
            {
                sim->out_debug.printf("%s adjusted cooldown of Chastise, by %f mS, with Apotheosis.", priest().name(), cd1);
            }
		}
		else if (s->result_amount > 0 && priest().talents.light_of_the_naaru->ok())
		{
			priest().cooldowns.holy_word_chastise->adjust((-10 * (priest().talents.light_of_the_naaru->effectN(1).base_value() + 100) * holy_word_chastise->effectN(2).time_value()));
			double cd2 = -10 * (priest().talents.light_of_the_naaru->effectN(1).base_value() + 100) * holy_word_chastise->effectN(2).base_value();
			if (sim->debug)
			{
				sim->out_debug.printf("%s adjusted cooldown of Chastise, by %f mS, with Light of the Naaru.", priest().name(), cd2);
			}
		}
		else if (s->result_amount > 0) {
        priest().cooldowns.holy_word_chastise->adjust((-1000 * holy_word_chastise->effectN(2).time_value()));
        double cd3 = -1000 * holy_word_chastise->effectN(2).base_value();
        if (sim->debug){
           sim->out_debug.printf("%s adjusted cooldown of Chastise, by %f mS, without Apotheosis", priest().name(), cd3);
			           }
			 }
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
  summon_pet_t( const std::string& n, priest_t& p, const spell_data_t* sd = spell_data_t::nil() )
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
    pet->summon( summoning_duration );

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
  summon_shadowfiend_t( priest_t& p, const std::string& options_str )
    : summon_pet_t( "shadowfiend", p, p.find_class_spell( "Shadowfiend" ) )
  {
    parse_options( options_str );
    harmful            = false;
    summoning_duration = data().duration();
    cooldown           = p.cooldowns.shadowfiend;
    cooldown->duration = data().cooldown();
    cooldown->duration += priest().sets->set( PRIEST_SHADOW, T18, B2 )->effectN( 1 ).time_value();
  }
};

// ==========================================================================
// Summon Mindbender
// ==========================================================================
struct summon_mindbender_t final : public summon_pet_t
{
  summon_mindbender_t( priest_t& p, const std::string& options_str )
    : summon_pet_t( "mindbender", p, p.find_talent_spell( "Mindbender" ) )
  {
    parse_options( options_str );
    harmful            = false;
    summoning_duration = data().duration();
    cooldown           = p.cooldowns.mindbender;
    cooldown->duration = data().cooldown();
    cooldown->duration += priest().sets->set( PRIEST_SHADOW, T18, B2 )->effectN( 2 ).time_value();
  }
};

/**
 * Discipline and shadow heal
 */
struct shadow_mend_t final : public priest_heal_t
{
  shadow_mend_t( priest_t& p, const std::string& options_str )
    : priest_heal_t( "shadow_mend", p, p.find_class_spell( "Shadow Mend" ) )
  {
    parse_options( options_str );
    harmful            = false;

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

  power_word_shield_t( priest_t& p, const std::string& options_str )
    : priest_absorb_t( "power_word_shield", p, p.find_class_spell( "Power Word: Shield" ) ), ignore_debuff( false )
  {
    add_option( opt_bool( "ignore_debuff", ignore_debuff ) );
    parse_options( options_str );

    spell_power_mod.direct = 5.5;  // hardcoded into tooltip, last checked 2017-03-18
  }

  void impact( action_state_t* s ) override
  {
    priest_absorb_t::impact( s );

    priest().buffs.borrowed_time->trigger();

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
  // Priest Leyshock's Grand Compendium basic hooks

  // Haste
  // Dark Void
  expansion::bfa::register_leyshocks_trigger( 263346, STAT_HASTE_RATING );
  // Dispersion
  expansion::bfa::register_leyshocks_trigger( 47585, STAT_HASTE_RATING );
  // Void Torrent
  expansion::bfa::register_leyshocks_trigger( 263165, STAT_HASTE_RATING );
  // Mind Bomb
  expansion::bfa::register_leyshocks_trigger( 205369, STAT_HASTE_RATING );
  // Holy Nova
  expansion::bfa::register_leyshocks_trigger( 132157, STAT_HASTE_RATING );
  // Angelic Feather
  expansion::bfa::register_leyshocks_trigger( 121536, STAT_HASTE_RATING );
  // Smite
  expansion::bfa::register_leyshocks_trigger( 585, STAT_HASTE_RATING );

  // Critical Strike
  // Psychic Horror
  expansion::bfa::register_leyshocks_trigger( 64044, STAT_CRIT_RATING );
  // Void Eruption
  expansion::bfa::register_leyshocks_trigger( 228260, STAT_CRIT_RATING );
  // Dispel Magic
  expansion::bfa::register_leyshocks_trigger( 528, STAT_CRIT_RATING );
  // Holy Word: Chastise
  expansion::bfa::register_leyshocks_trigger( 88625, STAT_CRIT_RATING );
  // Halo
  expansion::bfa::register_leyshocks_trigger( 120517, STAT_CRIT_RATING );

  // Mastery
  // Vampiric Embrace
  expansion::bfa::register_leyshocks_trigger( 15286, STAT_MASTERY_RATING );
  // Psychic Scream
  expansion::bfa::register_leyshocks_trigger( 8122, STAT_MASTERY_RATING );
  // Fade
  expansion::bfa::register_leyshocks_trigger( 586, STAT_MASTERY_RATING );
  // Purify Disease
  expansion::bfa::register_leyshocks_trigger( 213634, STAT_MASTERY_RATING );
  // Divine Star
  expansion::bfa::register_leyshocks_trigger( 110744, STAT_MASTERY_RATING);
  // Holy Fire
  expansion::bfa::register_leyshocks_trigger( 14914, STAT_MASTERY_RATING );
  // Halo
  expansion::bfa::register_leyshocks_trigger( 120517, STAT_MASTERY_RATING );

  // Versatility
  // Mind Flay
  expansion::bfa::register_leyshocks_trigger( 15407, STAT_VERSATILITY_RATING );
  // Shadow Mend
  expansion::bfa::register_leyshocks_trigger( 186263, STAT_VERSATILITY_RATING );
  // Shadow Word: Death
  expansion::bfa::register_leyshocks_trigger( 32379, STAT_VERSATILITY_RATING );
  // Silence
  expansion::bfa::register_leyshocks_trigger( 15487, STAT_VERSATILITY_RATING );
  // Surrender to Madness
  expansion::bfa::register_leyshocks_trigger( 193223, STAT_VERSATILITY_RATING );
  // Dark Ascension
  expansion::bfa::register_leyshocks_trigger( 280711, STAT_VERSATILITY_RATING );
  // Apotheosis
  expansion::bfa::register_leyshocks_trigger( 200183, STAT_VERSATILITY_RATING );
}

}  // namespace items

namespace pets
{
namespace fiend
{
void base_fiend_pet_t::init_action_list()
{
  main_hand_attack = new actions::fiend_melee_t( *this );

  if ( action_list_str.empty() )
  {
    action_priority_list_t* precombat = get_action_priority_list( "precombat" );
    precombat->add_action( "snapshot_stats",
                           "Snapshot raid buffed stats before combat begins and "
                           "pre-potting is done." );

    action_priority_list_t* def = get_action_priority_list( "default" );
    def->add_action( "wait" );
  }

  priest_pet_t::init_action_list();
}

action_t* base_fiend_pet_t::create_action( const std::string& name, const std::string& options_str )
{
  return priest_pet_t::create_action( name, options_str );
}
}  // namespace fiend
}  // namespace pets

// ==========================================================================
// Priest Targetdata Definitions
// ==========================================================================

priest_td_t::priest_td_t( player_t* target, priest_t& p ) : actor_target_data_t( target, &p ), dots(), buffs()
{
  dots.holy_fire         = target->get_dot( "holy_fire", &p );
  dots.power_word_solace = target->get_dot( "power_word_solace", &p );
  dots.shadow_word_pain  = target->get_dot( "shadow_word_pain", &p );
  dots.vampiric_touch    = target->get_dot( "vampiric_touch", &p );

  buffs.schism = make_buff( *this, "schism", p.talents.schism );

  target->callbacks_on_demise.push_back( [this]( player_t* ) { target_demise(); } );
}

void priest_td_t::reset()
{
}

void priest_td_t::target_demise()
{
  if ( priest().azerite.death_throes.enabled() && dots.shadow_word_pain->is_ticking() )
  {
    priest().generate_insanity( priest().azerite.death_throes.value( 2 ), priest().gains.insanity_death_throes,
                                nullptr );
  }

  priest().sim->print_debug( "Player '{}' demised. Priest '{}' resets targetdata for him.", target->name(),
                             priest().name() );

  reset();
}

// ==========================================================================
// Priest Definitions
// ==========================================================================

priest_t::priest_t( sim_t* sim, const std::string& name, race_e r )
  : player_t( sim, PRIEST, name, r ),
    buffs(),
    talents(),
    specs(),
    mastery_spells(),
    cooldowns(),
    gains(),
    benefits(),
    procs(),
    rppm(),
    active_spells(),
    active_items(),
    pets(),
    options(),
    insanity( *this )
{
  create_cooldowns();
  create_gains();
  create_procs();
  create_benefits();

  regen_type = REGEN_DYNAMIC;
}

/** Construct priest cooldowns */
void priest_t::create_cooldowns()
{
  cooldowns.chakra             = get_cooldown( "chakra" );
  cooldowns.mindbender         = get_cooldown( "mindbender" );
  cooldowns.penance            = get_cooldown( "penance" );
  cooldowns.apotheosis         = get_cooldown("apotheosis ");
  cooldowns.holy_fire          = get_cooldown( "holy_fire" );
  cooldowns.holy_word_chastise = get_cooldown( "holy_word_chastise" );
  cooldowns.holy_word_serenity = get_cooldown( "holy_word_serenity" );
  cooldowns.power_word_shield  = get_cooldown( "power_word_shield" );
  cooldowns.shadowfiend        = get_cooldown( "shadowfiend" );
  cooldowns.silence            = get_cooldown( "silence" );
  cooldowns.mind_blast         = get_cooldown( "mind_blast" );
  cooldowns.void_bolt          = get_cooldown( "void_bolt" );
  cooldowns.mind_bomb          = get_cooldown( "mind_bomb" );
  cooldowns.psychic_horror     = get_cooldown( "psychic_horror" );
  cooldowns.dark_ascension     = get_cooldown( "dark_ascension" );

  if ( specialization() == PRIEST_DISCIPLINE )
  {
    cooldowns.power_word_shield->duration = timespan_t::zero();
  }
  else
  {
    cooldowns.power_word_shield->duration = timespan_t::from_seconds( 4.0 );
  }
}

/** Construct priest gains */
void priest_t::create_gains()
{
  gains.mindbender                             = get_gain( "Mana Gained from Mindbender" );
  gains.power_word_solace                      = get_gain( "Mana Gained from Power Word: Solace" );
  gains.insanity_auspicious_spirits            = get_gain( "Insanity Gained from Auspicious Spirits" );
  gains.insanity_dispersion                    = get_gain( "Insanity Saved by Dispersion" );
  gains.insanity_drain                         = get_gain( "Insanity Drained by Voidform" );
  gains.insanity_mind_blast                    = get_gain( "Insanity Gained from Mind Blast" );
  gains.insanity_mind_flay                     = get_gain( "Insanity Gained from Mind Flay" );
  gains.insanity_mind_sear                     = get_gain( "Insanity Gained from Mind Sear" );
  gains.insanity_pet                           = get_gain( "Insanity Gained from Shadowfiend" );
  gains.insanity_shadow_crash                  = get_gain( "Insanity Gained from Shadow Crash" );
  gains.insanity_shadow_word_death             = get_gain( "Insanity Gained from Shadow Word: Death" );
  gains.insanity_shadow_word_pain_onhit        = get_gain( "Insanity Gained from Shadow Word: Pain Casts" );
  gains.insanity_shadow_word_void              = get_gain( "Insanity Gained from Shadow Word: Void" );
  gains.insanity_surrender_to_madness          = get_gain( "Insanity Gained from Surrender to Madness" );
  gains.insanity_wasted_surrendered_to_madness = get_gain( "Insanity Wasted from Surrendered to Madness" );
  gains.insanity_vampiric_touch_ondamage       = get_gain( "Insanity Gained from Vampiric Touch Damage (T19 2P)" );
  gains.insanity_vampiric_touch_onhit          = get_gain( "Insanity Gained from Vampiric Touch Casts" );
  gains.insanity_void_bolt                     = get_gain( "Insanity Gained from Void Bolt" );
  gains.insanity_void_torrent                  = get_gain( "Insanity Gained from Void Torrent" );
  gains.vampiric_touch_health                  = get_gain( "Health from Vampiric Touch Ticks" );
  gains.insanity_dark_void                     = get_gain( "Insanity Gained from Dark Void" );
}

/** Construct priest procs */
void priest_t::create_procs()
{
  procs.shadowy_apparition              = get_proc( "Shadowy Apparition Procced" );
  procs.shadowy_apparition              = get_proc( "Shadowy Apparition Insanity lost to overflow" );
  procs.shadowy_insight                 = get_proc( "Shadowy Insight Mind Blast CD Reset from Shadow Word: Pain" );
  procs.shadowy_insight_overflow        = get_proc( "Shadowy Insight Mind Blast CD Reset lost to overflow" );
  procs.surge_of_light                  = get_proc( "Surge of Light" );
  procs.surge_of_light_overflow         = get_proc( "Surge of Light lost to overflow" );
  procs.serendipity                     = get_proc( "Serendipity (Non-Tier 17 4pc)" );
  procs.serendipity_overflow            = get_proc( "Serendipity lost to overflow (Non-Tier 17 4pc)" );
  procs.power_of_the_dark_side          = get_proc( "Power of the Dark Side Penance damage buffed" );
  procs.power_of_the_dark_side_overflow = get_proc( "Power of the Dark Side lost to overflow" );
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

expr_t* priest_t::create_expression( const std::string& name_str )
{
  auto shadow_expression = create_expression_shadow( name_str );
  if ( shadow_expression )
  {
    return shadow_expression;
  }

  return player_t::create_expression( name_str );
}

void priest_t::assess_damage( school_e school, dmg_e dtype, action_state_t* s )
{
  player_t::assess_damage( school, dtype, s );
}

double priest_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( buffs.lingering_insanity->check() )
  {
    h /= 1.0 + ( buffs.lingering_insanity->check() ) * buffs.lingering_insanity->data().effectN( 1 ).percent();
  }

  if ( buffs.voidform->check() )
  {
    h /= 1.0 + ( buffs.voidform->check() * find_spell( 228264 )->effectN( 2 ).percent() / 10.0 );
  }

  return h;
}

double priest_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  if ( buffs.lingering_insanity->check() )
  {
    h /= 1.0 + ( buffs.lingering_insanity->check() - 1 ) * buffs.lingering_insanity->data().effectN( 1 ).percent();
  }

  if ( buffs.voidform->check() )
  {
    h /= 1.0 + ( buffs.voidform->check() * find_spell( 228264 )->effectN( 2 ).percent() / 10.0 );
  }

  return h;
}

double priest_t::composite_spell_speed() const
{
  double h = player_t::composite_spell_speed();

  if ( buffs.borrowed_time->check() )
  {
    h /= 1.0 + buffs.borrowed_time->data().effectN( 1 ).percent();
  }

  return h;
}

double priest_t::composite_melee_speed() const
{
  double h = player_t::composite_melee_speed();

  if ( buffs.borrowed_time->check() )
  {
    h /= 1.0 + buffs.borrowed_time->data().effectN( 1 ).percent();
  }

  return h;
}

double priest_t::composite_player_pet_damage_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_pet_damage_multiplier( s );
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

  if ( specs.grace->ok() )
  {
    m *= 1.0 + specs.grace->effectN( 1 ).percent();
  }

  return m;
}

double priest_t::composite_player_absorb_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_absorb_multiplier( s );

  if ( specs.grace->ok() )
  {
    m *= 1.0 + specs.grace->effectN( 2 ).percent();
  }

  return m;
}

double priest_t::composite_player_target_multiplier( player_t* t, school_e school ) const
{
  double m = player_t::composite_player_target_multiplier( t, school );

  auto target_data = get_target_data( t );
  if ( target_data->buffs.schism->check() )
  {
    m *= 1.0 + target_data->buffs.schism->data().effectN( 2 ).percent();
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

action_t* priest_t::create_action( const std::string& name, const std::string& options_str )
{
  using namespace actions::spells;
  using namespace actions::heals;

  action_t* shadow_action = create_action_shadow( name, options_str );
  if ( shadow_action && specialization() == PRIEST_SHADOW )
  {
    return shadow_action;
  }

  action_t* discipline_action = create_action_discipline( name, options_str );
  if ( discipline_action && specialization() == PRIEST_DISCIPLINE )
  {
    return discipline_action;
  }

  action_t* holy_action = create_action_holy( name, options_str );
  if ( holy_action && specialization() == PRIEST_HOLY )
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

  return base_t::create_action( name, options_str );
}

pet_t* priest_t::create_pet( const std::string& pet_name, const std::string& /* pet_type */ )
{
  // pet_t* p = find_pet( pet_name );

  if ( pet_name == "shadowfiend" )
  {
    return new pets::fiend::shadowfiend_pet_t( sim, *this );
  }
  if ( pet_name == "mindbender" )
  {
    return new pets::fiend::mindbender_pet_t( sim, *this );
  }

  sim->errorf( "Tried to create priest pet %s.", pet_name.c_str() );

  return nullptr;
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

void priest_t::init_base_stats()
{
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

  // Atonement heals are capped at a percentage of the Priest's health, so there may be scaling with stamina.
  if ( specialization() == PRIEST_DISCIPLINE && specs.atonement->ok() && primary_role() == ROLE_HEAL )
  {
    scaling->enable( STAT_STAMINA );
  }

  if ( specialization() == PRIEST_SHADOW )
  {
    // Just hook insanity init in here when actor set bonuses are ready
    insanity.init();
  }
}

void priest_t::init_spells()
{
  base_t::init_spells();

  init_spells_shadow();
  init_spells_discipline();
  init_spells_holy();

  // Mastery Spells
  mastery_spells.grace          = find_mastery_spell( PRIEST_DISCIPLINE );
  mastery_spells.echo_of_light  = find_mastery_spell( PRIEST_HOLY );
  mastery_spells.madness        = find_mastery_spell( PRIEST_SHADOW );
}

void priest_t::create_buffs()
{
  base_t::create_buffs();

  // Shared talent buffs
  buffs.twist_of_fate = make_buff( this, "twist_of_fate", talents.twist_of_fate->effectN( 1 ).trigger() )
                            ->set_trigger_spell( talents.twist_of_fate )
                            ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                            ->add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );
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

/// ALL Spec Pre-Combat Action Priority List
void priest_t::create_apl_precombat()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );
  // Snapshot stats
  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats",
                         "Snapshot raid buffed stats before combat begins and "
                         "pre-potting is done." );

  // do all kinds of calculations here to reduce CPU time
  if ( specialization() == PRIEST_SHADOW )
  {
    precombat->add_action( "potion" );
  }

  // Precast
  switch ( specialization() )
  {
    case PRIEST_DISCIPLINE:
      break;
    case PRIEST_HOLY:
      break;
    case PRIEST_SHADOW:
    default:
      // Calculate these variables once to reduce sim time
      precombat->add_action( "variable,name=mind_blast_targets,op=set,value="
                             "floor((4.5+azerite.whispers_of_the_damned.rank)%(1+0.27*azerite.searing_dialogue.rank))" );
      precombat->add_action( "variable,name=swp_trait_ranks_check,op=set,value="
                             "(1-0.07*azerite.death_throes.rank+0.2*azerite.thought_harvester.rank)"
                             "*(1-0.09*azerite.thought_harvester.rank*azerite.searing_dialogue.rank)" );
      precombat->add_action( "variable,name=vt_trait_ranks_check,op=set,value="
                             "(1-0.04*azerite.thought_harvester.rank-0.05*azerite.spiteful_apparitions.rank)" );
      precombat->add_action( "variable,name=vt_mis_trait_ranks_check,op=set,value="
                             "(1-0.07*azerite.death_throes.rank-0.03*azerite.thought_harvester.rank-0.055*azerite.spiteful_apparitions.rank)"
                             "*(1-0.027*azerite.thought_harvester.rank*azerite.searing_dialogue.rank)" );
      precombat->add_action( "variable,name=vt_mis_sd_check,op=set,value=1-0.014*azerite.searing_dialogue.rank" );
      precombat->add_action( this, "Shadowform", "if=!buff.shadowform.up" );
      precombat->add_action( this, "Mind Blast", "if=spell_targets.mind_sear<2|azerite.thought_harvester.rank=0" );
      precombat->add_action( this, "Vampiric Touch" );
      break;
  }
}

std::string priest_t::default_potion() const
{
  std::string lvl120_potion =
    ( specialization() == PRIEST_SHADOW ) ? "rising_death" :
                                            "battle_potion_of_intellect";

  return ( true_level >  110 ) ? lvl120_potion :
         ( true_level >= 100 ) ? "prolonged_power" :
         ( true_level >=  90 ) ? "draenic_intellect" :
         ( true_level >=  85 ) ? "jade_serpent" :
         ( true_level >=  80 ) ? "volcanic" :
                                 "disabled";
}

std::string priest_t::default_flask() const
{
  return ( true_level >  110 ) ? "endless_fathoms" :
         ( true_level >= 100 ) ? "whispered_pact" :
         ( true_level >=  90 ) ? "greater_draenic_intellect_flask" :
         ( true_level >=  85 ) ? "warm_sun" :
         ( true_level >=  80 ) ? "draconic_mind" :
                                 "disabled";
}

std::string priest_t::default_food() const
{
  return ( true_level > 110 ) ? "honeyglazed_haunches" :
         ( true_level > 100 ) ? "azshari_salad" :
         ( true_level >  90 ) ? "buttered_sturgeon" :
         ( true_level >= 90 ) ? "mogu_fish_stew" :
         ( true_level >= 80 ) ? "seafood_magnifique_feast" :
                                "disabled";
}

std::string priest_t::default_rune() const
{
  return ( true_level >= 120 ) ? "battle_scarred" :
         ( true_level >= 110 ) ? "defiled" :
         ( true_level >= 100 ) ? "focus" :
                                 "disabled";
}

/** NO Spec Combat Action Priority List */
void priest_t::create_apl_default()
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
  def->add_action( this, "Shadow Word: Pain", ",if=remains<tick_time|!ticking" );
  def->add_action( this, "Smite" );
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

  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }
  clear_action_priority_lists();

  create_apl_precombat();

  switch ( specialization() )
  {
    case PRIEST_SHADOW:
      generate_apl_shadow();
      break;
    case PRIEST_DISCIPLINE:
      if ( primary_role() != ROLE_HEAL )
      {
        generate_apl_discipline_d();
      }
      else
      {
        generate_apl_discipline_h();
      }
      break;
    case PRIEST_HOLY:
      if ( primary_role() != ROLE_HEAL )
      {
        generate_apl_holy_d();
      }
      else
      {
        generate_apl_holy_h();
      }
      break;
    default:
      create_apl_default();
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

  insanity.reset();
}

/**
 * Fixup Atonement Stats HPE, HPET and HPR
 */
void priest_t::pre_analyze_hook()
{
  base_t::pre_analyze_hook();

  if ( specs.atonement->ok() )
  {
    fixup_atonement_stats( "smite", "atonement_smite" );
    fixup_atonement_stats( "holy_fire", "atonement_holy_fire" );
    fixup_atonement_stats( "penance", "atonement_penance" );
  }

  if ( specialization() == PRIEST_DISCIPLINE || specialization() == PRIEST_HOLY )
  {
    fixup_atonement_stats( "power_word_solace", "atonement_power_word_solace" );
  }
}

void priest_t::target_mitigation( school_e school, dmg_e dt, action_state_t* s )
{
  base_t::target_mitigation( school, dt, s );

  if ( buffs.dispersion->check() )
  {
    s->result_amount *= 1.0 + ( buffs.dispersion->data().effectN( 1 ).percent() );
  }
}

action_t* priest_t::create_proc_action( const std::string& /*name*/, const special_effect_t& effect )
{
  return nullptr;
}

void priest_t::create_options()
{
  base_t::create_options();

  add_option( opt_deprecated( "double_dot", "action_list=double_dot" ) );
  add_option( opt_bool( "autounshift", options.autoUnshift ) );
  add_option( opt_bool( "priest_fixed_time", options.priest_fixed_time ) );
  add_option( opt_bool( "priest_ignore_healing", options.priest_ignore_healing ) );
  add_option( opt_int( "priest_set_voidform_duration", options.priest_set_voidform_duration ) );
}

std::string priest_t::create_profile( save_e type )
{
  std::string profile_str = base_t::create_profile( type );

  if ( type & SAVE_PLAYER )
  {
    if ( !options.autoUnshift )
    {
      profile_str += fmt::format( "autounshift={}\n", options.autoUnshift );
    }

    if ( !options.priest_fixed_time )
    {
      profile_str += fmt::format( "priest_fixed_time={}\n", options.priest_fixed_time );
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

}  // namespace priestspace

const module_t* module_t::priest()
{
  static priestspace::priest_module_t m;
  return &m;
}
