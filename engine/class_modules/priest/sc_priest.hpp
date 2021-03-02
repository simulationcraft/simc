// ==========================================================================
// Priest Definitions Sim File
// Contact: https://github.com/orgs/simulationcraft/teams/priest/members
// Wiki: https://github.com/simulationcraft/simc/wiki/Priests
// ==========================================================================
//
// This file contains all definitions for priests. Implementations should
// be done in sc_priest.cpp if they are shared by more than one spec or
// in the respective spec file if they are limited to one spec only.

#pragma once
#include "player/pet_spawner.hpp"
#include "sc_enums.hpp"

#include "simulationcraft.hpp"

namespace priestspace
{
/* Forward declarations
 */
struct priest_t;

namespace actions
{
namespace spells
{
struct levitate_t;
struct smite_t;
struct summon_pet_t;
struct summon_shadowfiend_t;
}  // namespace spells
namespace heals
{
struct power_word_shield_t;
}  // namespace heals
}  // namespace actions

namespace buffs
{
}

/**
 * Priest target data
 * Contains target specific things
 */
struct priest_td_t final : public actor_target_data_t
{
public:
  struct dots_t
  {
    propagate_const<dot_t*> shadow_word_pain;
    propagate_const<dot_t*> vampiric_touch;
    propagate_const<dot_t*> devouring_plague;
    propagate_const<dot_t*> vampiric_embrace;
  } dots;

  struct buffs_t
  {
    propagate_const<buff_t*> shadow_weaving;  // NYI
    propagate_const<buff_t*> misery;          // NYI
  } buffs;

  priest_t& priest()
  {
    return *debug_cast<priest_t*>( source );
  }
  const priest_t& priest() const
  {
    return *debug_cast<priest_t*>( source );
  }

  priest_td_t( player_t* target, priest_t& p );
  void reset();
  void target_demise();
};

/**
 * Priest class definition
 * Derived from player_t. Contains everything that defines the priest class.
 */
struct priest_t final : public player_t
{
public:
  using base_t = player_t;

  // Buffs
  struct
  {
    // Talents
    // Discipline
    propagate_const<buff_t*> inner_focus;     // NYI
    propagate_const<buff_t*> divine_spirit;   // NYI
    propagate_const<buff_t*> power_infusion;  // NYI

    // Holy
    propagate_const<buff_t*> surge_of_light;  // NYI

    // Shadow
    propagate_const<buff_t*> shadowform;
    propagate_const<buff_t*> shadowform_state;  // Dummy buff to track whether player entered Shadowform initially
    propagate_const<buff_t*> spirit_tap;        // NYI

  } buffs;

  // Talents
  struct
  {
    // Discipline
    const spell_data_t* wand_specialization;            // NYI
    const spell_data_t* improved_power_word_fortitude;  // NYI
    const spell_data_t* improved_power_word_shield;     // NYI
    const spell_data_t* inner_focus;                    // NYI
    const spell_data_t* meditation;                     // NYI
    const spell_data_t* mental_agility;                 // NYI
    const spell_data_t* mental_strength;                // NYI
    const spell_data_t* divine_spirit;                  // NYI
    const spell_data_t* improved_divine_spirit;         // NYI
    const spell_data_t* focused_power;                  // NYI
    const spell_data_t* force_of_will;                  // NYI
    const spell_data_t* power_infusion;                 // NYI
    const spell_data_t* enlightenment;                  // NYI

    // Holy
    const spell_data_t* holy_specialization;   // NYI
    const spell_data_t* divine_fury;           // NYI
    const spell_data_t* holy_nova;             // NYI
    const spell_data_t* holy_reach;            // NYI
    const spell_data_t* searing_light;         // NYI
    const spell_data_t* spirit_of_redemption;  // NYI
    const spell_data_t* spiritual_guidance;    // NYI
    const spell_data_t* surge_of_light;        // NYI

    // Shadow
    const spell_data_t* spirit_tap;                 // NYI
    const spell_data_t* blackout;                   // NYI
    const spell_data_t* improved_shadow_word_pain;  // NYI
    const spell_data_t* shadow_focus;               // NYI
    const spell_data_t* improved_psychic_scream;    // NYI
    const spell_data_t* improved_mind_blast;        // NYI
    const spell_data_t* mind_flay;                  // NYI
    const spell_data_t* improved_fade;              // NYI
    const spell_data_t* shadow_reach;               // NYI
    const spell_data_t* shadow_weaving;             // NYI
    const spell_data_t* silence;                    // NYI
    const spell_data_t* vampiric_embrace;           // NYI
    const spell_data_t* improved_vampiric_embrace;  // NYI
    const spell_data_t* focused_mind;               // NYI
    const spell_data_t* darkness;                   // NYI
    const spell_data_t* shadowform;                 // NYI
    const spell_data_t* shadow_power;               // NYI
    const spell_data_t* misery;                     // NYI
    const spell_data_t* vampiric_touch;             // NYI
  } talents;

  // Specialization Spells
  struct
  {
    // Misc
    const spell_data_t* consume_magic;  // NYI
    const spell_data_t* starshards;     // NYI
    const spell_data_t* chastise;       // NYI

    // Discipline
    const spell_data_t* power_word_shield;
    const spell_data_t* levitate;
    const spell_data_t* power_word_fortitude;

    // Holy
    const spell_data_t* holy_fire;  // NYI
    const spell_data_t* smite;

    // Shadow
    const spell_data_t* fade;       // NYI
    const spell_data_t* mana_burn;  // NYI
    const spell_data_t* mind_blast;
    const spell_data_t* shadow_word_death;
    const spell_data_t* shadowfiend;
  } specs;

  // DoT Spells
  struct
  {
    const spell_data_t* shadow_word_pain;
    const spell_data_t* vampiric_touch;
    const spell_data_t* devouring_plague;
    const spell_data_t* vampiric_embrace;
  } dot_spells;

  // Cooldowns
  struct
  {
    // Shadow
    propagate_const<cooldown_t*> mind_blast;

    // Holy
    propagate_const<cooldown_t*> holy_fire;
  } cooldowns;

  // Gains
  struct
  {
    propagate_const<gain_t*> mindbender;
    propagate_const<gain_t*> shadow_word_death_self_damage;
  } gains;

  // Benefits
  struct
  {
  } benefits;

  // Procs
  struct
  {
    propagate_const<proc_t*> holy_fire_cd;
  } procs;

  // Special
  struct
  {
  } background_actions;

  // Items
  struct
  {
  } active_items;

  // Pets
  struct priest_pets_t
  {
    propagate_const<pet_t*> shadowfiend;

    priest_pets_t( priest_t& p );
  } pets;

  // Options
  struct
  {
    bool autoUnshift = true;  // Shift automatically out of stance/form
    bool fixed_time  = true;

    // Default param to set if you should cast Power Infusion on yourself
    bool self_power_infusion = true;
  } options;

  priest_t( sim_t* sim, util::string_view name, race_e r );

  // player_t overrides
  void init_base_stats() override;
  void init_resources( bool force ) override;
  void init_spells() override;
  void create_buffs() override;
  void init_scaling() override;
  void init_finished() override;
  void init_background_actions() override;
  void reset() override;
  void create_options() override;
  std::string create_profile( save_e ) override;
  action_t* create_action( util::string_view name, const std::string& options ) override;
  pet_t* create_pet( util::string_view name, util::string_view type = "" ) override;
  void create_pets() override;
  void copy_from( player_t* source ) override;
  resource_e primary_resource() const override
  {
    return RESOURCE_MANA;
  }
  role_e primary_role() const override;
  stat_e convert_hybrid_stat( stat_e s ) const override;
  void assess_damage( school_e school, result_amount_type dtype, action_state_t* s ) override;
  double composite_melee_haste() const override;
  double composite_spell_haste() const override;
  double composite_spell_crit_chance() const override;
  double composite_player_pet_damage_multiplier( const action_state_t* ) const override;
  double composite_player_absorb_multiplier( const action_state_t* s ) const override;
  double composite_player_heal_multiplier( const action_state_t* s ) const override;
  double composite_player_target_multiplier( player_t* t, school_e school ) const override;
  double matching_gear_multiplier( attribute_e attr ) const override;
  void target_mitigation( school_e, result_amount_type, action_state_t* ) override;
  void init_action_list() override;
  void combat_begin() override;
  void init_rng() override;
  const priest_td_t* find_target_data( const player_t* target ) const override;
  priest_td_t* get_target_data( player_t* target ) const override;
  std::unique_ptr<expr_t> create_expression( util::string_view expression_str ) override;
  std::unique_ptr<expr_t> create_pet_expression( util::string_view expression_str,
                                                 util::span<util::string_view> splits );
  void arise() override;
  void do_dynamic_regen( bool ) override;
  void apply_affecting_auras( action_t& ) override;
  void invalidate_cache( cache_e ) override;

private:
  void create_cooldowns();
  void create_gains();
  void create_procs();
  void create_benefits();
  target_specific_t<priest_td_t> _target_data;

public:
  double tick_damage_over_time( timespan_t duration, const dot_t* dot ) const;

  std::string default_potion() const override;
  std::string default_flask() const override;
  std::string default_food() const override;
  std::string default_rune() const override;
  std::string default_temporary_enchant() const override;
};

namespace actions
{
/**
 * Priest action base class
 *
 * This is a template for common code between priest_spell_t, priest_heal_t and priest_absorb_t.
 * The template is instantiated with either spell_t, heal_t or absorb_t as the 'Base' class.
 * Make sure you keep the inheritance hierarchy and use base_t in the derived class, don't skip it and call
 * spell_t/heal_t or absorb_t directly.
 */
template <typename Base>
struct priest_action_t : public Base
{
  struct
  {
    bool shadowform_da;
    bool shadowform_ta;
  } affected_by;

public:
  priest_action_t( util::string_view name, priest_t& p, const spell_data_t* s = spell_data_t::nil() )
    : ab( name, &p, s ), affected_by()
  {
    init_affected_by();
    ab::may_crit          = true;
    ab::tick_may_crit     = true;
    ab::weapon_multiplier = 0.0;
  }

  /**
   * Initialize all affected_by members and print out debug info
   */
  void init_affected_by()
  {
    struct affect_init_t
    {
      const spelleffect_data_t& effect;
      bool& affects;
    } affects[] = { { priest().buffs.shadowform->data().effectN( 1 ), affected_by.shadowform_da },
                    { priest().buffs.shadowform->data().effectN( 4 ), affected_by.shadowform_ta } };

    for ( const auto& a : affects )
    {
      a.affects = base_t::data().affected_by( a.effect );
      if ( a.affects && ab::sim->debug )
      {
        ab::sim->print_debug( "{} {} ({}) affected by {} (idx={}).", *ab::player, *this, ab::data().id(),
                              a.effect.spell()->name_cstr(), a.effect.spell_effect_num() + 1 );
      }
    }
  }

  priest_td_t& get_td( player_t* t )
  {
    return *( priest().get_target_data( t ) );
  }

  const priest_td_t* find_td( const player_t* t ) const
  {
    return priest().find_target_data( t );
  }

  double cost() const override
  {
    double c = ab::cost();

    return c;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = ab::composite_target_multiplier( target );

    return m;
  }

  double action_da_multiplier() const override
  {
    double m = ab::action_da_multiplier();

    if ( affected_by.shadowform_da && priest().buffs.shadowform->check() )
    {
      m *= 1.0 + priest().buffs.shadowform->data().effectN( 1 ).percent();
    }

    return m;
  }

  double action_ta_multiplier() const override
  {
    double m = ab::action_ta_multiplier();

    if ( affected_by.shadowform_ta && priest().buffs.shadowform->check() )
    {
      m *= 1.0 + priest().buffs.shadowform->data().effectN( 4 ).percent();
    }

    return m;
  }

  void gain_energize_resource( resource_e resource_type, double amount, gain_t* gain ) override
  {
    ab::gain_energize_resource( resource_type, amount, gain );
  }

protected:
  priest_t& priest()
  {
    return *debug_cast<priest_t*>( ab::player );
  }
  const priest_t& priest() const
  {
    return *debug_cast<priest_t*>( ab::player );
  }

  // typedef for priest_action_t<action_base_t>
  using base_t = priest_action_t;

private:
  // typedef for the templated action type, eg. spell_t, attack_t, heal_t
  using ab = Base;
};  // namespace actions

struct priest_absorb_t : public priest_action_t<absorb_t>
{
public:
  priest_absorb_t( util::string_view name, priest_t& player ) : base_t( name, player )
  {
    may_crit      = true;
    tick_may_crit = false;
    may_miss      = false;
  }
};

struct priest_heal_t : public priest_action_t<heal_t>
{
  priest_heal_t( util::string_view name, priest_t& player ) : base_t( name, player )
  {
  }

  void impact( action_state_t* s ) override
  {
    double save_health_percentage = s->target->health_percentage();

    base_t::impact( s );
  }
};

struct priest_spell_t : public priest_action_t<spell_t>
{
  priest_spell_t( util::string_view name, priest_t& player ) : base_t( name, player )
  {
    weapon_multiplier = 0.0;
  }
};

}  // namespace actions

namespace buffs
{
/**
 * This is a template for common code between priest buffs.
 * The template is instantiated with any type of buff ( buff_t, debuff_t,
 * absorb_buff_t, etc. ) as the 'Base' class.
 * Make sure you keep the inheritance hierarchy and use base_t in the derived
 * class,
 * don't skip it and call buff_t/absorb_buff_t/etc. directly.
 */
template <typename Base = buff_t>
struct priest_buff_t : public Base
{
public:
  using base_t = priest_buff_t;  // typedef for priest_buff_t<buff_base_t>

  priest_buff_t( priest_td_t& td, util::string_view name, const spell_data_t* s = spell_data_t::nil(),
                 const item_t* item = nullptr )
    : Base( td, name, s, item )
  {
  }

  priest_buff_t( priest_t& p, util::string_view name, const spell_data_t* s = spell_data_t::nil(),
                 const item_t* item = nullptr )
    : Base( &p, name, s, item )
  {
  }

protected:
  priest_t& priest()
  {
    return *debug_cast<priest_t*>( Base::source );
  }
  const priest_t& priest() const
  {
    return *debug_cast<priest_t*>( Base::source );
  }
};

}  // namespace buffs

namespace items
{
void init();
}  // namespace items

/**
 * Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
struct priest_report_t final : public player_report_extension_t
{
public:
  priest_report_t( priest_t& player ) : p( player )
  {
  }

  void html_customsection( report::sc_html_stream& /* os*/ ) override
  {
    (void)p;
    /*// Custom Class Section
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n"
    << "\t\t\t\t\t<h3 class=\"toggle open\">Custom Section</h3>\n"
    << "\t\t\t\t\t<div class=\"toggle-content\">\n";

    os << p.name();

    os << "\t\t\t\t\t\t</div>\n" << "\t\t\t\t\t</div>\n";*/
  }

private:
  priest_t& p;
};

struct priest_module_t final : public module_t
{
  priest_module_t() : module_t( PRIEST )
  {
  }

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p              = new priest_t( sim, name, r );
    p->report_extension = std::unique_ptr<player_report_extension_t>( new priest_report_t( *p ) );
    return p;
  }
  bool valid() const override
  {
    return true;
  }
  void init( player_t* p ) const override
  {
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
