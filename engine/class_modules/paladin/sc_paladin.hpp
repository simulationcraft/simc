#pragma once
#include "simulationcraft.hpp"

namespace paladin {
// Forward declarations
struct paladin_t;
struct blessing_of_sacrifice_redirect_t;
struct paladin_ground_aoe_t;
namespace buffs {
                  struct avenging_wrath_buff_t;
                  struct crusade_buff_t;
                  struct sephuzs_secret_buff_t;
                  struct holy_avenger_buff_t;
                  struct ardent_defender_buff_t;
                  struct liadrins_fury_unleashed_t;
                  struct forbearance_t;
                  struct shield_of_vengeance_buff_t;
                }

// ==========================================================================
// Paladin Target Data
// ==========================================================================

struct paladin_td_t : public actor_target_data_t
{
  struct dots_t
  {
    dot_t* execution_sentence;
    dot_t* wake_of_ashes;
  } dots;

  struct buffs_t
  {
    buff_t* debuffs_judgment;
    buff_t* judgment_of_light;
    buff_t* eye_of_tyr_debuff;
    buffs::forbearance_t* forbearant_faithful;
    buff_t* blessed_hammer_debuff;
  } buffs;

  paladin_td_t( player_t* target, paladin_t* paladin );
};

struct paladin_t : public player_t
{
public:

  // Active
  heal_t*   active_beacon_of_light;
  heal_t*   active_enlightened_judgments;
  action_t* active_shield_of_vengeance_proc;
  action_t* active_holy_shield_proc;
  action_t* active_judgment_of_light_proc;
  action_t* active_sotr;
  heal_t*   active_protector_of_the_innocent;

  const special_effect_t* whisper_of_the_nathrezim;
  const special_effect_t* liadrins_fury_unleashed;
  const special_effect_t* justice_gaze;
  const special_effect_t* chain_of_thrayn;
  const special_effect_t* ashes_to_dust;
  const special_effect_t* scarlet_inquisitors_expurgation;
  const special_effect_t* ferren_marcuss_strength;
  const special_effect_t* saruans_resolve;
  const special_effect_t* gift_of_the_golden_valkyr;
  const special_effect_t* heathcliffs_immortality;
  const special_effect_t* pillars_of_inmost_light;
  const spell_data_t* sephuz;
  const spell_data_t* topless_tower;

  struct active_actions_t
  {
    blessing_of_sacrifice_redirect_t* blessing_of_sacrifice_redirect;
  } active;

  // Forbearant Faithful cooldown shenanigans
  std::vector<cooldown_t*> forbearant_faithful_cooldowns;

  // Buffs
  struct buffs_t
  {
    // core
    buffs::ardent_defender_buff_t* ardent_defender;
    buffs::avenging_wrath_buff_t* avenging_wrath;
    buffs::crusade_buff_t* crusade;
    buffs::sephuzs_secret_buff_t* sephuz;
    buff_t* holy_avenger;
    buffs::shield_of_vengeance_buff_t* shield_of_vengeance;
    buff_t* divine_protection;
    buff_t* divine_shield;
    buff_t* guardian_of_ancient_kings;
    buff_t* grand_crusader;
    buff_t* infusion_of_light;
    buff_t* shield_of_the_righteous;
    absorb_buff_t* bulwark_of_order;
    buff_t* last_defender;

    // talents
    absorb_buff_t* holy_shield_absorb; // Dummy buff to trigger spell damage "blocking" absorb effect

    buff_t* zeal;
    buff_t* the_fires_of_justice;
    buff_t* blade_of_wrath;
    buff_t* divine_purpose;
    buff_t* divine_steed;
    buff_t* aegis_of_light;
    stat_buff_t* seraphim;
    buff_t* whisper_of_the_nathrezim;
    buffs::liadrins_fury_unleashed_t* liadrins_fury_unleashed;
    buff_t* scarlet_inquisitors_expurgation_driver;
    buff_t* scarlet_inquisitors_expurgation;

    // Set Bonuses
    buff_t* sacred_judgment;
    buff_t* ret_t21_4p;
  } buffs;

  // Gains
  struct gains_t
  {
    // Healing/absorbs
    gain_t* holy_shield;
    gain_t* bulwark_of_order;

    // Mana
    gain_t* extra_regen;
    gain_t* mana_beacon_of_light;

    // Holy Power
    gain_t* hp_templars_verdict_refund;
    gain_t* hp_liadrins_fury_unleashed;
    gain_t* judgment;
    gain_t* hp_t19_4p;
    gain_t* hp_t20_2p;
    gain_t* hp_justice_gaze;
  } gains;

  // Spec Passives
  struct spec_t
  {
    const spell_data_t* judgment_2;
    const spell_data_t* judgment_3;
    const spell_data_t* retribution_paladin;
  } spec;


  // Cooldowns
  struct cooldowns_t
  {
    // Required to get various cooldown-reducing procs procs working
    cooldown_t* avengers_shield;         // Grand Crusader (prot)
    cooldown_t* shield_of_the_righteous; // Judgment (prot)
    cooldown_t* avenging_wrath;          // Righteous Protector (prot)
    cooldown_t* light_of_the_protector;  // Righteous Protector (prot) / Saruin
    cooldown_t* hand_of_the_protector;   // Righteous Protector (prot) / Saruin
    cooldown_t* judgment;         // Grand Crusader + Crusader's Judgment
    cooldown_t* guardian_of_ancient_kings; // legen chest
    cooldown_t* eye_of_tyr; // legen shoulders
    cooldown_t* holy_shock; // Holy Shock for Crusader's Might && DP
    cooldown_t* light_of_dawn; // Light of Dawn for DP

    // whoo fist of justice
    cooldown_t* hammer_of_justice;

    cooldown_t* blade_of_justice;
    cooldown_t* blade_of_wrath;
    cooldown_t* divine_hammer;
  } cooldowns;

  // Passives
  struct passives_t
  {
    const spell_data_t* bladed_armor;
    const spell_data_t* boundless_conviction;
    const spell_data_t* divine_bulwark;
    const spell_data_t* grand_crusader;
    const spell_data_t* guarded_by_the_light;
    const spell_data_t* divine_judgment;
    const spell_data_t* holy_insight;
    const spell_data_t* infusion_of_light;
    const spell_data_t* lightbringer;
    const spell_data_t* plate_specialization;
    const spell_data_t* riposte;   // hidden
    const spell_data_t* paladin;
    const spell_data_t* retribution_paladin;
    const spell_data_t* protection_paladin;
    const spell_data_t* holy_paladin;
    const spell_data_t* sanctuary;
    const spell_data_t* improved_block; //hidden

    const spell_data_t* judgment; // mystery, hidden
  } passives;

  // Procs and RNG
  real_ppm_t* blade_of_wrath_rppm;
  struct procs_t
  {
    proc_t* eternal_glory;
    proc_t* focus_of_vengeance_reset;
    proc_t* defender_of_the_light;

    proc_t* divine_purpose;
    proc_t* the_fires_of_justice;
    proc_t* tfoj_set_bonus;
    proc_t* blade_of_wrath;
    proc_t* topless_tower;
  } procs;

    struct shuffled_rngs_t
    {
        // Holy
        shuffled_rng_t* topless_tower;
    } shuffled_rngs;


  // Spells
  struct spells_t
  {
    const spell_data_t* holy_light;
    const spell_data_t* sanctified_wrath; // needed to pull out cooldown reductions
    const spell_data_t* divine_purpose_ret;
    const spell_data_t* divine_purpose_holy;
    const spell_data_t* liadrins_fury_unleashed;
    const spell_data_t* justice_gaze;
    const spell_data_t* pillars_of_inmost_light;
    const spell_data_t* chain_of_thrayn;
    const spell_data_t* ashes_to_dust;
    const spell_data_t* ferren_marcuss_strength;
    const spell_data_t* saruans_resolve;
    const spell_data_t* gift_of_the_golden_valkyr;
    const spell_data_t* heathcliffs_immortality;
    const spell_data_t* consecration_bonus;
    const spell_data_t* blessing_of_the_ashbringer;
  } spells;

  // Talents
  struct talents_t
  {
    // Ignore fist of justice/repentance/blinding light

    // Holy
      // T15
    const spell_data_t* bestow_faith;
    const spell_data_t* lights_hammer;
    const spell_data_t* crusaders_might;
      // T30
    const spell_data_t* cavalier;
    const spell_data_t* unbreakable_spirit;
    const spell_data_t* rule_of_law;
      // Skip T45
      // T60
    const spell_data_t* devotion_aura;
    const spell_data_t* aura_of_sacrifice;
    const spell_data_t* aura_of_mercy;
      // T75
//    const spell_data_t* divine_purpose;
    const spell_data_t* holy_avenger;
    const spell_data_t* holy_prism;
      // T90
    const spell_data_t* fervent_martyr;
    const spell_data_t* sanctified_wrath;
    const spell_data_t* judgment_of_light;
      // T100
    const spell_data_t* beacon_of_faith;
    const spell_data_t* beacon_of_the_lightbringer;
    const spell_data_t* beacon_of_virtue;

    // Protection
    const spell_data_t* first_avenger;
    const spell_data_t* bastion_of_light;
    const spell_data_t* crusaders_judgment;
    const spell_data_t* holy_shield;
    const spell_data_t* blessed_hammer;
    const spell_data_t* consecrated_hammer;
    // skip T45
    const spell_data_t* blessing_of_spellwarding;
    const spell_data_t* blessing_of_salvation;
    const spell_data_t* retribution_aura;
    const spell_data_t* hand_of_the_protector;
    const spell_data_t* knight_templar;
    const spell_data_t* final_stand;
    const spell_data_t* aegis_of_light;
    // Judgment of Light seems to be a recopy from Holy.
    //const spell_data_t* judgment_of_light;
    const spell_data_t* consecrated_ground;
    const spell_data_t* righteous_protector;
    const spell_data_t* seraphim;
    const spell_data_t* last_defender;

    // Retribution
    const spell_data_t* final_verdict;
    const spell_data_t* execution_sentence;
    const spell_data_t* consecration;
    const spell_data_t* fires_of_justice;
    const spell_data_t* greater_judgment;
    const spell_data_t* zeal;
    const spell_data_t* fist_of_justice;
    const spell_data_t* repentance;
    const spell_data_t* blinding_light;
    const spell_data_t* virtues_blade;
    const spell_data_t* blade_of_wrath;
    const spell_data_t* divine_hammer;
    const spell_data_t* justicars_vengeance;
    const spell_data_t* eye_for_an_eye;
    const spell_data_t* word_of_glory;
    const spell_data_t* divine_intervention;
    const spell_data_t* divine_steed;
    const spell_data_t* divine_purpose;
    const spell_data_t* crusade;
    const spell_data_t* crusade_talent;
    const spell_data_t* wake_of_ashes;
  } talents;

  player_t* beacon_target;

  timespan_t last_extra_regen;
  timespan_t extra_regen_period;
  double extra_regen_percent;

  timespan_t last_jol_proc;

  bool fake_sov;

  paladin_t( sim_t* sim, const std::string& name, race_e r = RACE_TAUREN );

  virtual void      init_base_stats() override;
  virtual void      init_gains() override;
  virtual void      init_procs() override;
  virtual void      init() override;
  virtual void      init_scaling() override;
  virtual void      create_buffs() override;
  virtual void      init_rng() override;
  virtual void      init_spells() override;
  virtual void      init_assessors() override;
  virtual void      init_action_list() override;
  virtual void      reset() override;
  virtual expr_t*   create_expression( action_t*, const std::string& name ) override;

  // player stat functions
  virtual double    composite_attribute( attribute_e attr ) const override;
  virtual double    composite_attribute_multiplier( attribute_e attr ) const override;
  virtual double    composite_rating_multiplier( rating_e rating ) const override;
  virtual double    composite_attack_power_multiplier() const override;
  virtual double    composite_mastery() const override;
  virtual double    composite_mastery_rating() const override;
  virtual double    composite_armor_multiplier() const override;
  virtual double    composite_bonus_armor() const override;
  virtual double    composite_melee_attack_power() const override;
  virtual double    composite_melee_crit_chance() const override;
  virtual double    composite_melee_expertise( const weapon_t* weapon ) const override;
  virtual double    composite_melee_haste() const override;
  virtual double    composite_melee_speed() const override;
  virtual double    composite_spell_crit_chance() const override;
  virtual double    composite_spell_haste() const override;
  virtual double    composite_player_multiplier( school_e school ) const override;
  virtual double    composite_player_target_multiplier( player_t* target, school_e school ) const override;
  virtual double    composite_player_heal_multiplier( const action_state_t* s ) const override;
  virtual double    composite_spell_power( school_e school ) const override;
  virtual double    composite_spell_power_multiplier() const override;
  virtual double    composite_crit_avoidance() const override;
  virtual double    composite_parry_rating() const override;
  virtual double    composite_parry() const override;
  virtual double    composite_block() const override;
  virtual double    composite_block_reduction() const override;
  virtual double    temporary_movement_modifier() const override;

  // combat outcome functions
  virtual void      assess_damage( school_e, dmg_e, action_state_t* ) override;
  virtual void      assess_damage_imminent( school_e, dmg_e, action_state_t* ) override;
  virtual void      assess_heal( school_e, dmg_e, action_state_t* ) override;
  virtual void      target_mitigation( school_e, dmg_e, action_state_t* ) override;

  virtual void      invalidate_cache( cache_e ) override;
  virtual void      create_options() override;
  virtual double    matching_gear_multiplier( attribute_e attr ) const override;
  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override;
  void       activate() override;
  virtual resource_e primary_resource() const override { return RESOURCE_MANA; }
  virtual role_e    primary_role() const override;
  virtual stat_e    convert_hybrid_stat( stat_e s ) const override;
  virtual stat_e    primary_stat() const override;
  virtual void      regen( timespan_t periodicity ) override;
  virtual void      combat_begin() override;
  virtual void      copy_from( player_t* ) override;

  virtual double current_health() const override;

  double  get_divine_judgment( bool is_judgment = false ) const;
  void    trigger_grand_crusader();
  void    trigger_holy_shield( action_state_t* s );
  void    trigger_forbearance( player_t* target, bool update_multiplier = true );
  int     get_local_enemies( double distance ) const;
  double  get_forbearant_faithful_recharge_multiplier() const;
  bool    standing_in_consecration() const;

  std::vector<paladin_ground_aoe_t*> active_consecrations;


  void      create_buffs_retribution();
  void      init_assessors_retribution();
  void      init_rng_retribution();
  void      init_spells_retribution();
  void      generate_action_prio_list_ret();
  action_t* create_action_retribution( const std::string& name, const std::string& options_str );

  void      create_buffs_protection();
  void      init_spells_protection();
  action_t* create_action_protection( const std::string& name, const std::string& options_str );

  void    update_forbearance_recharge_multipliers() const;
  void    generate_action_prio_list_prot();
  void    generate_action_prio_list_holy();
  void    generate_action_prio_list_holy_dps();

  target_specific_t<paladin_td_t> target_data;

  virtual paladin_td_t* get_target_data( player_t* target ) const override;
};

namespace buffs {
  struct liadrins_fury_unleashed_t : public buff_t
  {
    liadrins_fury_unleashed_t( player_t* p );
  };

  struct avenging_wrath_buff_t : public buff_t
  {
    avenging_wrath_buff_t( player_t* p );

    void expire_override( int expiration_stacks, timespan_t remaining_duration ) override;

    double get_damage_mod() const
    {
      return damage_modifier;
    }

    double get_healing_mod() const
    {
      return healing_modifier;
    }

    double get_crit_bonus() const
    {
      return crit_bonus;
    }

    private:
    double damage_modifier;
    double healing_modifier;
    double crit_bonus;
  };

  struct sephuzs_secret_buff_t : public haste_buff_t
  {
    cooldown_t* icd;
    sephuzs_secret_buff_t(paladin_t* p) :
      haste_buff_t(haste_buff_creator_t(p, "sephuzs_secret", p -> find_spell( 208052 ))
        .default_value(p -> find_spell( 208052 ) -> effectN(2).percent())
        .add_invalidate( CACHE_HASTE ))
    {
      icd = p->get_cooldown("sephuzs_secret_cooldown");
      icd->duration = p->find_spell(226262)->duration();
    }

    void execute(int stacks, double value, timespan_t duration) override
    {
      if (icd->down())
        return;
      buff_t::execute(stacks, value, duration);
      icd->start();
    }
  };

  struct crusade_buff_t : public haste_buff_t
  {
    crusade_buff_t( player_t* p );

    void expire_override( int expiration_stacks, timespan_t remaining_duration ) override;

    double get_damage_mod()
    {
      return damage_modifier * ( this -> stack() );
    }

    double get_healing_mod()
    {
      return healing_modifier * ( this -> stack() );
    }

    double get_haste_bonus()
    {
      return haste_bonus * ( this -> stack() );
    }
    private:
    double damage_modifier;
    double healing_modifier;
    double haste_bonus;
  };

  struct holy_avenger_buff_t : public haste_buff_t
  {
      holy_avenger_buff_t( player_t* p ):
      haste_buff_t( haste_buff_creator_t( p, "holy_avenger", p -> find_spell( 105809 ) )
                    .default_value(1.0 / (1.0 + p -> find_spell(105809) -> effectN(1).percent()))
                    .add_invalidate(CACHE_HASTE))

      {
      }
  };

  struct forbearance_t : public buff_t
  {
    paladin_t* paladin;

    forbearance_t( player_t* p, const char *name ) :
      buff_t( buff_creator_t( p, name, p -> find_spell( 25771 ) ) ),
      paladin( nullptr )
    { }

    forbearance_t( paladin_td_t* ap, const char *name ) :
      buff_t( buff_creator_t( *ap, name, ap -> source -> find_spell( 25771 ) ) ),
      paladin( debug_cast<paladin_t*>( ap -> source ) )
    { }

    void execute( int stacks, double value, timespan_t duration ) override
    {
      buff_t::execute( stacks, value, duration );

      paladin -> update_forbearance_recharge_multipliers();
    }

    void expire( timespan_t delay ) override
    {
      bool expired = check() != 0;

      buff_t::expire( delay );

      if ( expired )
      {
        paladin -> update_forbearance_recharge_multipliers();
      }
    }
  };

}

// ==========================================================================
// Paladin Ability Templates
// ==========================================================================

// Template for common paladin action code. See priest_action_t.
template <class Base>
struct paladin_action_t : public Base
{
private:
  typedef Base ab; // action base, eg. spell_t
public:
  typedef paladin_action_t base_t;

  // haste scaling bools
  bool hasted_cd;
  bool hasted_gcd;
  bool ret_damage_increase;
  bool ret_dot_increase;
  bool ret_damage_increase_two;

  paladin_action_t( const std::string& n, paladin_t* player,
                    const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s ),
    hasted_cd( ab::data().affected_by( player -> passives.paladin -> effectN( 2 ) ) ),
    hasted_gcd( ab::data().affected_by( player -> passives.paladin -> effectN( 3 ) ) ),
    ret_damage_increase( ab::data().affected_by( player -> spec.retribution_paladin -> effectN( 1 ) ) ),
    ret_dot_increase( ab::data().affected_by( player -> spec.retribution_paladin -> effectN( 2 ) ) ),
    ret_damage_increase_two( ab::data().affected_by( player -> spec.retribution_paladin -> effectN( 7 ) ) )
  {
    // Aura buff to protection paladin added in 7.3
    if ( this -> data().affected_by( p() -> passives.protection_paladin -> effectN( 10 ) ) && p() -> specialization() == PALADIN_PROTECTION )
    {
      this -> base_dd_multiplier *= 1.0 + p() -> passives.protection_paladin -> effectN( 10 ).percent();
    }
  }

  paladin_t* p()
  { return static_cast<paladin_t*>( ab::player ); }
  const paladin_t* p() const
  { return static_cast<paladin_t*>( ab::player ); }

  paladin_td_t* td( player_t* t ) const
  { return p() -> get_target_data( t ); }

  void init() override
  {
    ab::init();

    if ( hasted_cd )
    {
      ab::cooldown -> hasted = hasted_cd;
    }
    if ( hasted_gcd )
    {
      if ( p() -> specialization() == PALADIN_HOLY )
      {
        ab::gcd_haste = HASTE_SPELL;
      }
      else
      {
        ab::gcd_haste = HASTE_ATTACK;
      }
    }
    if ( ret_damage_increase )
      ab::base_dd_multiplier *= 1.0 + p() -> spec.retribution_paladin -> effectN( 1 ).percent();
    if ( ret_dot_increase )
      ab::base_td_multiplier *= 1.0 + p() -> spec.retribution_paladin -> effectN( 2 ).percent();
    if ( ret_damage_increase_two )
      ab::base_dd_multiplier *= 1.0 + p() -> spec.retribution_paladin -> effectN( 7 ).percent();
  }

  double cost() const override
  {
    if ( ab::current_resource() == RESOURCE_HOLY_POWER )
    {
      if ( p() -> buffs.divine_purpose -> check() )
        return 0.0;

      return ab::base_costs[ RESOURCE_HOLY_POWER ];
    }

    return ab::cost();
  }

  void execute() override
  {
    ab::execute();

    // Handle benefit tracking
    if ( this -> harmful )
    {
      p() -> buffs.avenging_wrath -> up();
    }
  }

  void trigger_judgment_of_light( action_state_t* s )
  {
    if ( p() -> resources.current[ RESOURCE_HEALTH ] < p() -> resources.max[ RESOURCE_HEALTH ] && ( p() -> sim -> current_time() - p() -> last_jol_proc ) >= timespan_t::from_seconds( 1.0 ) )
    {
      p() -> active_judgment_of_light_proc -> execute();
      td ( s -> target ) -> buffs.judgment_of_light -> decrement();
    }
  }

  void impact( action_state_t* s ) override
  {
    ab::impact( s );

    if ( td( s -> target ) -> buffs.judgment_of_light -> up() )
      trigger_judgment_of_light( s );
  }

  virtual double cooldown_multiplier() { return 1.0; }
};

// paladin "Spell" Base for paladin_spell_t, paladin_heal_t and paladin_absorb_t

template <class Base>
struct paladin_spell_base_t : public paladin_action_t< Base >
{
private:
  typedef paladin_action_t< Base > ab;
public:
  typedef paladin_spell_base_t base_t;

  paladin_spell_base_t( const std::string& n, paladin_t* player,
                        const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s )
  {
  }

};

// paladin_ground_aoe_t for consecration and blessed hammer

struct paladin_ground_aoe_t : public ground_aoe_event_t
{
  double radius;
  paladin_t* paladin;

public:
  paladin_ground_aoe_t( paladin_t* p, const ground_aoe_params_t* param, action_state_t* ps, bool first_tick = false ):
    ground_aoe_event_t( p, param, ps, first_tick ), radius( param -> action() -> radius ), paladin( p )
  {}

  paladin_ground_aoe_t( paladin_t* p, const ground_aoe_params_t& param, bool first_tick = false ) :
    ground_aoe_event_t( p, param, first_tick ), radius( param.action() -> radius ), paladin( p )
  {}


  void schedule_event() override
  {
    paladin_ground_aoe_t* foo = make_event<paladin_ground_aoe_t>( sim(), paladin, params, pulse_state );
    paladin -> active_consecrations.push_back( foo );
    // If the ground-aoe event is a pulse-based one, increase the current pulse of the newly created
    // event.
    if ( params -> n_pulses() > 0 )
    {
      foo -> set_current_pulse( current_pulse + 1 );
    }
  }

  void execute() override
  {
    auto it = range::find( paladin -> active_consecrations, this );
    paladin -> active_consecrations.erase( it );
    ground_aoe_event_t::execute();
  }
};

// ==========================================================================
// The damage formula in action_t::calculate_direct_amount in sc_action.cpp is documented here:
// https://github.com/simulationcraft/simc/wiki/DevelopersDocumentation#damage-calculations
// ==========================================================================


// ==========================================================================
// Paladin Spells, Heals, and Absorbs
// ==========================================================================

// paladin-specific spell_t, heal_t, and absorb_t classes for inheritance ===

struct paladin_spell_t : public paladin_spell_base_t<spell_t>
{
  paladin_spell_t( const std::string& n, paladin_t* p,
                   const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  {
    // Holy Insight - Holy passive
    base_multiplier *= 1.0 + p -> passives.holy_insight -> effectN( 6 ).percent();
  }
};

struct paladin_heal_t : public paladin_spell_base_t<heal_t>
{
  const spell_data_t* tower_of_radiance;
  paladin_heal_t( const std::string& n, paladin_t* p,
                  const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s ), tower_of_radiance( p -> find_spell( 88852 ) )
  {
    may_crit          = true;
    tick_may_crit     = true;
    harmful = false;

    weapon_multiplier = 0.0;
  }

  virtual void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( s -> target != p() -> beacon_target )
      trigger_beacon_of_light( s );
  }

  void trigger_beacon_of_light( action_state_t* s )
  {
    if ( ! p() -> beacon_target )
      return;

    if ( ! p() -> beacon_target -> buffs.beacon_of_light -> up() )
      return;

    if ( proc )
      return;

    assert( p() -> active_beacon_of_light );

    p() -> active_beacon_of_light -> target = p() -> beacon_target;

    double amount = s -> result_amount;
    amount *= p() -> beacon_target -> buffs.beacon_of_light -> data().effectN( 1 ).percent();

    p() -> active_beacon_of_light -> base_dd_min = amount;
    p() -> active_beacon_of_light -> base_dd_max = amount;

    // Holy light heals for 100% instead of 50%
    if ( data().id() == p() -> spells.holy_light -> id() )
    {
      p() -> active_beacon_of_light -> base_dd_min *= 2.0;
      p() -> active_beacon_of_light -> base_dd_max *= 2.0;
    }

    p() -> active_beacon_of_light -> execute();
  }
};

struct paladin_absorb_t : public paladin_spell_base_t< absorb_t >
{
  paladin_absorb_t( const std::string& n, paladin_t* p,
                    const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  { }
};

struct paladin_melee_attack_t: public paladin_action_t < melee_attack_t >
{
  paladin_melee_attack_t( const std::string& n, paladin_t* p,
                          const spell_data_t* s = spell_data_t::nil()) :
    base_t( n, p, s )
  {
    may_crit = true;
    special = true;
    weapon = &( p -> main_hand_weapon );
  }
};

struct holy_power_generator_t : public paladin_melee_attack_t
{
  holy_power_generator_t( const std::string& n, paladin_t* p,
                          const spell_data_t* s = spell_data_t::nil() ):
                          paladin_melee_attack_t( n, p, s )
  {}

  virtual void execute() override;
};

struct holy_power_consumer_t : public paladin_melee_attack_t
{
  holy_power_consumer_t( const std::string& n, paladin_t* p,
                          const spell_data_t* s = spell_data_t::nil() );

  double composite_target_multiplier( player_t* t ) const override;

  virtual void execute() override;
};

}
