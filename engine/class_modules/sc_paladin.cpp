// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
/*
  TODO (Holy):
    - everything, pretty much :(

  TODO (ret):
    - Eye for an Eye
    - verify Divine Purpose interactions with The Fires of Justice and Seal of Light
    - bugfixes & cleanup
    - BoK/BoW
    - Check mana/mana regen for ret, sword of light has been significantly changed to no longer have the mana regen stuff, or the bonus to healing, reduction in mana costs, etc.
  TODO (prot):
    - If J's sotr effect changes, fix up hardcode multiplier.
    - Breastplate of the Golden Val'kyr (leg)
    - Chain of Thrayn (leg)
    - Tyelca, Ferren Marcus' Stature (leg)
    - Aggramar's Stride (leg)
    - Uther's Guard (leg)
    - Aegisjalmur, the Armguards of Awe (leg)
    - Tyr's Hand of Faith (leg)
    - Heathcliff's Immortality (leg)
    - Ilterendi, Crown Jewel of Silvermoon (leg - Holy?)
    - Aegis of Light: Convert from self-buff to totem/pet with area buff? (low priority)

    - Everything below this line is super-low priority and can probably be ignored ======
    - Final Stand?? (like Hand of Reckoning, but AOE)
    - Blessing of Spellweaving??
    - Retribution Aura?? (like HS, but would need to be in player_t to trigger off of other players being hit)
*/
#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE

// Forward declarations
struct paladin_t;
struct blessing_of_sacrifice_redirect_t;
struct paladin_ground_aoe_t;
namespace buffs {
                  struct avenging_wrath_buff_t;
                  struct crusade_buff_t;
                  struct sephuzs_secret_buff_t;
                  struct ardent_defender_buff_t;
                  struct wings_of_liberty_driver_t;
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

// ==========================================================================
// Paladin
// ==========================================================================

struct paladin_t : public player_t
{
public:

  // Active
  heal_t*   active_beacon_of_light;
  heal_t*   active_enlightened_judgments;
  action_t* active_shield_of_vengeance_proc;
  action_t* active_holy_shield_proc;
  action_t* active_painful_truths_proc;
  action_t* active_tyrs_enforcer_proc;
  action_t* active_judgment_of_light_proc;
  action_t* active_sotr;
  heal_t*   active_protector_of_the_innocent;

  const special_effect_t* retribution_trinket;
  player_t* last_retribution_trinket_target;
  const special_effect_t* whisper_of_the_nathrezim;
  const special_effect_t* liadrins_fury_unleashed;
  const special_effect_t* justice_gaze;
  const special_effect_t* chain_of_thrayn;
  const special_effect_t* ashes_to_dust;
  const spell_data_t* sephuz;

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
    buffs::shield_of_vengeance_buff_t* shield_of_vengeance;
    buff_t* divine_protection;
    buff_t* divine_shield;
    buff_t* guardian_of_ancient_kings;
    buff_t* grand_crusader;
    buff_t* infusion_of_light;
    buff_t* shield_of_the_righteous;
    absorb_buff_t* bulwark_of_order;

    // talents
    absorb_buff_t* holy_shield_absorb; // Dummy buff to trigger spell damage "blocking" absorb effect

    buff_t* zeal;
    buff_t* seal_of_light;
    buff_t* the_fires_of_justice;
    buff_t* blade_of_wrath;
    buff_t* divine_purpose;
    buff_t* divine_steed;
    buff_t* aegis_of_light;
    stat_buff_t* seraphim;
    buff_t* whisper_of_the_nathrezim;
    buffs::liadrins_fury_unleashed_t* liadrins_fury_unleashed;

    // Set Bonuses
    buff_t* vindicators_fury;     // WoD Ret PVP 4-piece
    buff_t* wings_of_liberty;     // Most roleplay name. T18 4P Ret bonus
    buffs::wings_of_liberty_driver_t* wings_of_liberty_driver;
    buff_t* retribution_trinket; // 6.2 Spec-Specific Trinket

    // artifact
    buff_t* painful_truths;
    buff_t* righteous_verdict;
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
    cooldown_t* light_of_the_protector;  // Righteous Protector (prot)
    cooldown_t* hand_of_the_protector;   // Righteous Protector (prot)
  cooldown_t* judgment;				 // Grand Crusader + Crusader's Judgment

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
    const spell_data_t* hand_of_light;
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
    const spell_data_t* sword_of_light;
    const spell_data_t* sword_of_light_value;
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
  } procs;

  // Spells
  struct spells_t
  {
    const spell_data_t* holy_light;
    const spell_data_t* sanctified_wrath; // needed to pull out cooldown reductions
    const spell_data_t* divine_purpose_ret;
    const spell_data_t* liadrins_fury_unleashed;
    const spell_data_t* justice_gaze;
    const spell_data_t* chain_of_thrayn;
    const spell_data_t* ashes_to_dust;
    const spell_data_t* consecration_bonus;
    const spell_data_t* blessing_of_the_ashbringer;
  } spells;

  // Talents
  struct talents_t
  {
    // Ignore fist of justice/repentance/blinding light

    // Holy
    const spell_data_t* holy_bolt;
    const spell_data_t* lights_hammer;
    const spell_data_t* crusaders_might;
    // TODO: Beacon of Hope seems like a pain
    const spell_data_t* beacon_of_hope;
    const spell_data_t* unbreakable_spirit;
    const spell_data_t* shield_of_vengeance;
    const spell_data_t* devotion_aura;
    const spell_data_t* aura_of_light;
    const spell_data_t* aura_of_mercy;
    const spell_data_t* holy_prism;
    const spell_data_t* stoicism;
    const spell_data_t* daybreak;
    // TODO: test
    const spell_data_t* sanctified_wrath;
    const spell_data_t* judgment_of_light;
    const spell_data_t* beacon_of_faith;
    const spell_data_t* beacon_of_the_lightbringer;
    const spell_data_t* beacon_of_the_savior;

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
    const spell_data_t* seal_of_light;
    const spell_data_t* divine_purpose;
    const spell_data_t* crusade;
    const spell_data_t* crusade_talent;
    const spell_data_t* holy_wrath;
  } talents;

  struct artifact_spell_data_t
  {
    // Ret
    artifact_power_t wake_of_ashes;
    artifact_power_t blade_of_light;
    artifact_power_t deliver_the_justice;
    artifact_power_t sharpened_edge;
    artifact_power_t deflection;
    artifact_power_t echo_of_the_highlord;
    artifact_power_t wrath_of_the_ashbringer;
    artifact_power_t healing_storm;                 // NYI
    artifact_power_t highlords_judgment;
    artifact_power_t embrace_the_light;             // NYI
    artifact_power_t divine_tempest;                // Implemented 20% damage gain but not AoE shift
    artifact_power_t endless_resolve;
    artifact_power_t might_of_the_templar;
    artifact_power_t ashes_to_ashes;
    artifact_power_t protector_of_the_ashen_blade;  // NYI
    artifact_power_t unbreakable_will;              // NYI
    artifact_power_t righteous_blade;
    artifact_power_t ashbringers_light;
    artifact_power_t ferocity_of_the_silver_hand;
    artifact_power_t righteous_verdict;
    artifact_power_t judge_unworthy;
    artifact_power_t blessing_of_the_ashbringer;

    // Prot
    artifact_power_t eye_of_tyr;
    artifact_power_t truthguards_light;
    artifact_power_t faiths_armor;
    artifact_power_t scatter_the_shadows;
    artifact_power_t righteous_crusader;
    artifact_power_t unflinching_defense;
    artifact_power_t sacrifice_of_the_just;
    artifact_power_t hammer_time;
    artifact_power_t bastion_of_truth;
    artifact_power_t resolve_of_truth;
    artifact_power_t painful_truths;
    artifact_power_t forbearant_faithful;
    artifact_power_t consecration_in_flame;
    artifact_power_t stern_judgment;
    artifact_power_t bulwark_of_order;
    artifact_power_t light_of_the_titans;
    artifact_power_t tyrs_enforcer;
    artifact_power_t unrelenting_light;

  } artifact;

  player_t* beacon_target;

  timespan_t last_extra_regen;
  timespan_t extra_regen_period;
  double extra_regen_percent;

  timespan_t last_jol_proc;

  double fixed_holy_wrath_health_pct;
  bool fake_sov;

  paladin_t( sim_t* sim, const std::string& name, race_e r = RACE_TAUREN ) :
    player_t( sim, PALADIN, name, r ),
    active( active_actions_t() ),
    buffs( buffs_t() ),
    gains( gains_t() ),
    cooldowns( cooldowns_t() ),
    passives( passives_t() ),
    procs( procs_t() ),
    spells( spells_t() ),
    talents( talents_t() ),
    beacon_target( nullptr ),
    last_extra_regen( timespan_t::from_seconds( 0.0 ) ),
    extra_regen_period( timespan_t::from_seconds( 0.0 ) ),
    extra_regen_percent( 0.0 ),
    last_jol_proc( timespan_t::from_seconds( 0.0 ) ),
    fixed_holy_wrath_health_pct( -1.0 ),
    fake_sov( true )
  {
    last_retribution_trinket_target = nullptr;
    retribution_trinket = nullptr;
    whisper_of_the_nathrezim = nullptr;
    liadrins_fury_unleashed = nullptr;
    chain_of_thrayn = nullptr;
    ashes_to_dust = nullptr;
    justice_gaze = nullptr;
    sephuz = nullptr;
    active_beacon_of_light             = nullptr;
    active_enlightened_judgments       = nullptr;
    active_shield_of_vengeance_proc    = nullptr;
    active_holy_shield_proc            = nullptr;
    active_tyrs_enforcer_proc          = nullptr;
    active_painful_truths_proc         = nullptr;
    active_judgment_of_light_proc      = nullptr;
    active_sotr                        = nullptr;
    active_protector_of_the_innocent   = nullptr;

    cooldowns.avengers_shield         = get_cooldown( "avengers_shield" );
    cooldowns.judgment                = get_cooldown("judgment");
    cooldowns.shield_of_the_righteous = get_cooldown( "shield_of_the_righteous" );
    cooldowns.avenging_wrath          = get_cooldown( "avenging_wrath" );
    cooldowns.light_of_the_protector  = get_cooldown( "light_of_the_protector" );
    cooldowns.hand_of_the_protector   = get_cooldown( "hand_of_the_protector" );
    cooldowns.hammer_of_justice       = get_cooldown( "hammer_of_justice" );
    cooldowns.blade_of_justice        = get_cooldown( "blade_of_justice" );
    cooldowns.blade_of_wrath          = get_cooldown( "blade_of_wrath" );
    cooldowns.divine_hammer           = get_cooldown( "divine_hammer" );

    beacon_target = nullptr;
    regen_type = REGEN_DYNAMIC;
  }

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
  virtual resource_e primary_resource() const override { return RESOURCE_MANA; }
  virtual role_e    primary_role() const override;
  virtual stat_e    convert_hybrid_stat( stat_e s ) const override;
  virtual void      regen( timespan_t periodicity ) override;
  virtual void      combat_begin() override;
  virtual void      copy_from( player_t* ) override;

  virtual double current_health() const override;

  double  get_divine_judgment() const;
  void    trigger_grand_crusader();
  void    trigger_holy_shield( action_state_t* s );
  void    trigger_painful_truths( action_state_t* s );
  void    trigger_tyrs_enforcer( action_state_t* s );
  int     get_local_enemies( double distance ) const;
  double  get_forbearant_faithful_recharge_multiplier() const;
  bool    standing_in_consecration() const;

  std::vector<paladin_ground_aoe_t*> active_consecrations;


  void    update_forbearance_recharge_multipliers() const;
  virtual bool has_t18_class_trinket() const override;
  void    generate_action_prio_list_prot();
  void    generate_action_prio_list_ret();
  void    generate_action_prio_list_holy();
  void    generate_action_prio_list_holy_dps();

  target_specific_t<paladin_td_t> target_data;

  virtual paladin_td_t* get_target_data( player_t* target ) const override
  {
    paladin_td_t*& td = target_data[ target ];
    if ( ! td )
    {
      td = new paladin_td_t( target, const_cast<paladin_t*>(this) );
    }
    return td;
  }
};



// ==========================================================================
// Paladin Buffs, Part One
// ==========================================================================
// Paladin buffs are split up into two sections. This one is for ones that
// need to be defined before action_t definitions, because those action_t
// definitions call methods of these buffs. Generic buffs that can be defined
// anywhere are also put here. There's a second buffs section near the end
// containing ones that require action_t definitions to function properly.

namespace buffs {
  struct wings_of_liberty_driver_t: public buff_t
  {
    wings_of_liberty_driver_t( player_t* p ):
      buff_t( buff_creator_t( p, "wings_of_liberty_driver", p -> find_spell( 185655 ) )
      .chance( p -> sets.has_set_bonus( PALADIN_RETRIBUTION, T18, B4 ) )
      .quiet( true )
      .tick_callback( [ p ]( buff_t*, int, const timespan_t& ) {
        paladin_t* paladin = debug_cast<paladin_t*>( p );
        paladin -> buffs.wings_of_liberty -> trigger( 1 );
      } ) )
    { }

    void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
    {
      buff_t::expire_override( expiration_stacks, remaining_duration );

      paladin_t* p = static_cast<paladin_t*>( player );
      p -> buffs.wings_of_liberty -> expire(); // Force the damage buff to fade.
    }
  };

  struct liadrins_fury_unleashed_t : public buff_t
  {
    liadrins_fury_unleashed_t( player_t* p ):
      buff_t( buff_creator_t( p, "liadrins_fury_unleashed", p -> find_spell( 208410 ) )
      .tick_zero( true )
      .tick_callback( [ this, p ]( buff_t*, int, const timespan_t& ) {
        paladin_t* paladin = debug_cast<paladin_t*>( p );
        paladin -> resource_gain( RESOURCE_HOLY_POWER, data().effectN( 1 ).base_value(), paladin -> gains.hp_liadrins_fury_unleashed );
      } ) )
    { }
  };

  struct ardent_defender_buff_t: public buff_t
  {
    bool oneup_triggered;

    ardent_defender_buff_t( player_t* p ):
      buff_t( buff_creator_t( p, "ardent_defender", p -> find_specialization_spell( "Ardent Defender" ) ) ),
      oneup_triggered( false )
    {

    }

    void use_oneup()
    {
      oneup_triggered = true;
    }

  };

  struct avenging_wrath_buff_t: public buff_t
  {
    avenging_wrath_buff_t( player_t* p ):
      buff_t( buff_creator_t( p, "avenging_wrath", p -> specialization() == PALADIN_HOLY ? p -> find_spell( 31842 ) : p -> find_spell( 31884 ) ) ),
      damage_modifier( 0.0 ),
      healing_modifier( 0.0 ),
      crit_bonus( 0.0 )
    {
      // Map modifiers appropriately based on spec
      paladin_t* paladin = static_cast<paladin_t*>( player );

      if ( p -> specialization() == PALADIN_HOLY )
      {
        healing_modifier = data().effectN( 1 ).percent();
        crit_bonus = data().effectN( 2 ).percent();
        damage_modifier = data().effectN( 3 ).percent();
        // holy shock cooldown reduction handled in holy_shock_t

        // invalidate crit
        add_invalidate( CACHE_CRIT_CHANCE );

        // Lengthen duration if Sanctified Wrath is taken
        buff_duration *= 1.0 + paladin -> talents.sanctified_wrath -> effectN( 1 ).percent();
      }
      else // we're Ret or Prot
      {
        damage_modifier = data().effectN( 1 ).percent();
        healing_modifier = data().effectN( 2 ).percent();

        if ( paladin -> artifact.wrath_of_the_ashbringer.rank() )
          buff_duration += timespan_t::from_millis(paladin -> artifact.wrath_of_the_ashbringer.value());
      }

      // let the ability handle the cooldown
      cooldown -> duration = timespan_t::zero();

      // invalidate Damage and Healing for both specs
      add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
      add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );
    }

    void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
    {
      buff_t::expire_override( expiration_stacks, remaining_duration );

      paladin_t* p = static_cast<paladin_t*>( player );
      p -> buffs.liadrins_fury_unleashed -> expire(); // Force Liadrin's Fury to fade
    }

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
      haste_buff_t(haste_buff_creator_t(p, "sephuzs_secret", p -> find_spell(208052))
        .default_value(p -> find_spell(208052) -> effectN(2).percent())
        .add_invalidate(CACHE_HASTE))
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

  struct crusade_buff_t : public buff_t
  {
    crusade_buff_t( player_t* p ):
      buff_t( buff_creator_t( p, "crusade", p -> find_spell( 231895 ) )
        .refresh_behavior( BUFF_REFRESH_DISABLED ) ),
      damage_modifier( 0.0 ),
      healing_modifier( 0.0 ),
      haste_bonus( 0.0 )
    {
      // TODO(mserrano): fix this when Blizzard turns the spelldata back to sane
      //  values
      damage_modifier = data().effectN( 1 ).percent() / 10.0;
      haste_bonus = data().effectN( 2 ).percent() / 10.0;
      healing_modifier = 0;

      paladin_t* paladin = static_cast<paladin_t*>( player );
      if ( paladin -> artifact.wrath_of_the_ashbringer.rank() )
      {
        buff_duration += timespan_t::from_millis(paladin -> artifact.wrath_of_the_ashbringer.value());
      }

      // let the ability handle the cooldown
      cooldown -> duration = timespan_t::zero();

      // invalidate Damage and Healing for both specs
      add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
      add_invalidate( CACHE_HASTE );
    }

    void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
    {
      buff_t::expire_override( expiration_stacks, remaining_duration );

      paladin_t* p = static_cast<paladin_t*>( player );
      p -> buffs.liadrins_fury_unleashed -> expire(); // Force Liadrin's Fury to fade
    }

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

  struct shield_of_vengeance_buff_t : public absorb_buff_t
  {
    shield_of_vengeance_buff_t( player_t* p ):
      absorb_buff_t( absorb_buff_creator_t( p, "shield_of_vengeance", p -> find_spell( 184662 ) ) )
    {
      paladin_t* pal = static_cast<paladin_t*>( p );
      if ( pal -> artifact.deflection.rank() )
      {
        cooldown -> duration += timespan_t::from_millis( pal -> artifact.deflection.value() );
      }
    }

    void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
    {
      absorb_buff_t::expire_override( expiration_stacks, remaining_duration );

      paladin_t* p = static_cast<paladin_t*>( player );
      // do thing
      if ( p -> fake_sov )
      {
        // TODO(mserrano): This is a horrible hack
        p -> active_shield_of_vengeance_proc -> base_dd_max = p -> active_shield_of_vengeance_proc -> base_dd_min = current_value;
        p -> active_shield_of_vengeance_proc -> execute();
      }
    }
  };

  struct forbearance_t : public buff_t
  {
    paladin_t* paladin;

    forbearance_t( paladin_t* p, const char *name ) :
      buff_t( buff_creator_t( p, name, p -> find_spell( 25771 ) ) ), paladin( p )
    { }

    forbearance_t( paladin_t* p, paladin_td_t *ap, const char *name ) :
      buff_t( buff_creator_t( *ap, name, p -> find_spell( 25771 ) ) ), paladin( p )
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

} // end namespace buffs
// ==========================================================================
// End Paladin Buffs, Part One
// ==========================================================================


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
    double c = ( this -> current_resource() == RESOURCE_HOLY_POWER ) ? this -> cost() : -1.0;

    ab::execute();

    // if the ability uses Holy Power, handle Divine Purpose and other freebie effects
    if ( c >= 0 )
    {
      // trigger WoD Ret PvP 4-P
      p() -> buffs.vindicators_fury -> trigger( 1, c > 0 ? c : 3 );
    }

    // Handle benefit tracking
    if ( this -> harmful )
    {
      p() -> buffs.avenging_wrath -> up();
      p() -> buffs.wings_of_liberty -> up();
      p() -> buffs.retribution_trinket -> up();
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

// Aegis of Light (Protection) ================================================

struct aegis_of_light_t : public paladin_spell_t
{
  aegis_of_light_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "aegis_of_light", p, p -> find_talent_spell( "Aegis of Light" ) )
  {
    parse_options( options_str );

    harmful = false;
  }

  virtual void execute() override
  {
    paladin_spell_t::execute();

    p() -> buffs.aegis_of_light -> trigger();
  }
};

// Ardent Defender (Protection) ===============================================

struct ardent_defender_t : public paladin_spell_t
{
  ardent_defender_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "ardent_defender", p, p -> find_specialization_spell( "Ardent Defender" ) )
  {
    parse_options( options_str );

    harmful = false;
    use_off_gcd = true;
    trigger_gcd = timespan_t::zero();

    cooldown -> duration += timespan_t::from_millis( p -> artifact.unflinching_defense.value( 1 ) );
  }

  virtual void execute() override
  {
    paladin_spell_t::execute();

    p() -> buffs.ardent_defender -> trigger();

    if ( p() -> artifact.painful_truths.rank() )
      p() -> buffs.painful_truths -> trigger();
  }
};

// Avengers Shield ==========================================================

struct avengers_shield_t : public paladin_spell_t
{
  avengers_shield_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "avengers_shield", p, p -> find_class_spell( "Avenger's Shield" ) )
  {
    parse_options( options_str );

    if ( ! p -> has_shield_equipped() )
    {
      sim -> errorf( "%s: %s only usable with shield equipped in offhand\n", p -> name(), name() );
      background = true;
    }
    may_crit     = true;

    // TODO: add and legendary bonuses
    base_multiplier *= 1.0 + p -> talents.first_avenger -> effectN( 1 ).percent();
  base_aoe_multiplier *= 1.0;
  if ( p ->talents.first_avenger->ok() )
    base_aoe_multiplier *= 2.0 / 3.0;
  aoe = 3;
    aoe = std::max( aoe, 0 );


    // link needed for trigger_grand_crusader
    cooldown = p -> cooldowns.avengers_shield;
    cooldown -> duration = data().cooldown();
  }

  void impact( action_state_t* s ) override
  {
    paladin_spell_t::impact( s );

    // Bulwark of Order absorb shield. Amount is additive per hit.
    if ( p() -> artifact.bulwark_of_order.rank() )
      p() -> buffs.bulwark_of_order -> trigger( 1, p() -> buffs.bulwark_of_order -> value() + s -> result_amount * p() -> artifact.bulwark_of_order.percent() );

    p() -> trigger_tyrs_enforcer( s );
  }
};

// Bastion of Light ========================================================

struct bastion_of_light_t : public paladin_spell_t
{
  bastion_of_light_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "bastion_of_light", p, p -> find_talent_spell( "Bastion of Light" ) )
  {
    parse_options( options_str );

    harmful = false;
    use_off_gcd = true;
  }

  void execute() override
  {
    paladin_spell_t::execute();

    p()->cooldowns.shield_of_the_righteous->reset(false, true);
  }
};

// Blessing of Protection =====================================================

struct blessing_of_protection_t : public paladin_spell_t
{
  blessing_of_protection_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "blessing_of_protection", p, p -> find_class_spell( "Blessing of Protection" ) )
  {
    parse_options( options_str );
  }

  bool init_finished() override
  {
    p() -> forbearant_faithful_cooldowns.push_back( cooldown );

    return paladin_spell_t::init_finished();
  }

  virtual void execute() override
  {
    paladin_spell_t::execute();

    if ( p() -> artifact.endless_resolve.rank() )
      // Don't trigger forbearance with endless resolve
      return;

    // apply forbearance, track locally for forbearant faithful & force recharge recalculation
    target -> debuffs.forbearance -> trigger();
    td( target ) -> buffs.forbearant_faithful -> trigger();
    p() -> update_forbearance_recharge_multipliers();
  }

  virtual bool ready() override
  {
    if ( target -> debuffs.forbearance -> check() )
      return false;

    return paladin_spell_t::ready();
  }

  double recharge_multiplier() const override
  {
    double cdr = paladin_spell_t::recharge_multiplier();

    // BoP is bugged on beta - doesnot benefit from this, but the forbearance debuff it applies does affect LoH/DS.
    // cdr *= p() -> get_forbearant_faithful_recharge_multiplier();

    return cdr;
  }

};

// Blessing of Spellwarding =====================================================

struct blessing_of_spellwarding_t : public paladin_spell_t
{
  blessing_of_spellwarding_t(paladin_t* p, const std::string& options_str)
    : paladin_spell_t("blessing_of_spellwarding", p, p -> find_talent_spell("Blessing of Spellwarding"))
  {
    parse_options(options_str);
  }

  bool init_finished() override
  {
    p()->forbearant_faithful_cooldowns.push_back(cooldown);

    return paladin_spell_t::init_finished();
  }

  virtual void execute() override
  {
    paladin_spell_t::execute();

    if (p()->artifact.endless_resolve.rank())
      // Don't trigger forbearance with endless resolve
      return;

    // apply forbearance, track locally for forbearant faithful & force recharge recalculation
    target->debuffs.forbearance->trigger();
    td(target)->buffs.forbearant_faithful->trigger();
    p()->update_forbearance_recharge_multipliers();
  }

  virtual bool ready() override
  {
    if (target->debuffs.forbearance->check())
      return false;

    return paladin_spell_t::ready();
  }

  double recharge_multiplier() const override
  {
    double cdr = paladin_spell_t::recharge_multiplier();

    // BoP is bugged on beta - doesnot benefit from this, but the forbearance debuff it applies does affect LoH/DS.
    // cdr *= p() -> get_forbearant_faithful_recharge_multiplier();

    return cdr;
  }

};

// Crusade
struct crusade_t : public paladin_heal_t
{
  crusade_t( paladin_t* p, const std::string& options_str )
    : paladin_heal_t( "crusade", p, p -> talents.crusade )
  {
    parse_options( options_str );

    if ( ! ( p -> talents.crusade_talent -> ok() ) )
      background = true;

    cooldown -> charges += p -> sets.set( PALADIN_RETRIBUTION, T18, B2 ) -> effectN( 1 ).base_value();
  }

  void tick( dot_t* d ) override
  {
    // override for this just in case Avenging Wrath were to get canceled or removed
    // early, or if there's a duration mismatch (unlikely, but...)
    if ( p() -> buffs.crusade -> check() )
    {
      // call tick()
      heal_t::tick( d );
    }
  }

  void execute() override
  {
    paladin_heal_t::execute();

    p() -> buffs.crusade -> trigger();
    if ( p() -> sets.has_set_bonus( PALADIN_RETRIBUTION, T18, B4 ) )
    {
      p() -> buffs.wings_of_liberty -> trigger( 1 ); // We have to trigger a stack here.
      p() -> buffs.wings_of_liberty_driver -> trigger();
    }

    if ( p() -> liadrins_fury_unleashed )
    {
      p() -> buffs.liadrins_fury_unleashed -> trigger();
    }
  }

  bool ready() override
  {
    if ( p() -> buffs.crusade -> check() )
      return false;
    else
      return paladin_heal_t::ready();
  }
};

// Avenging Wrath ===========================================================
// AW is actually two spells (31884 for Ret/Prot, 31842 for Holy) and the effects are all jumbled.
// Thus, we need to use some ugly hacks to get it to work seamlessly for both specs within the same spell.
// Most of them can be found in buffs::avenging_wrath_buff_t, this spell just triggers the buff

struct avenging_wrath_t : public paladin_spell_t
{
  avenging_wrath_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "avenging_wrath", p, p -> specialization() == PALADIN_HOLY ? p -> find_spell( 31842 ) : p -> find_spell( 31884 ) )
  {
    parse_options( options_str );

    if ( p -> specialization() == PALADIN_RETRIBUTION )
    {
      if ( p -> talents.crusade_talent -> ok() )
        background = true;
      cooldown -> charges += p -> sets.set( PALADIN_RETRIBUTION, T18, B2 ) -> effectN( 1 ).base_value();
    }

    harmful = false;
    trigger_gcd = timespan_t::zero();

    // link needed for Righteous Protector / SotR cooldown reduction
    cooldown = p -> cooldowns.avenging_wrath;
  }

  void execute() override
  {
    paladin_spell_t::execute();

    p() -> buffs.avenging_wrath -> trigger();
    if ( p() -> sets.has_set_bonus( PALADIN_RETRIBUTION, T18, B4 ) )
    {
      p() -> buffs.wings_of_liberty -> trigger( 1 ); // We have to trigger a stack here.
      p() -> buffs.wings_of_liberty_driver -> trigger();
    }

    if ( p() -> liadrins_fury_unleashed )
    {
      p() -> buffs.liadrins_fury_unleashed -> trigger();
    }
  }

  // TODO: is this needed? Question for Ret dev, since I don't think it is for Prot/Holy
  bool ready() override
  {
    if ( p() -> buffs.avenging_wrath -> check() )
      return false;
    else
      return paladin_spell_t::ready();
  }
};

// Beacon of Light ==========================================================

struct beacon_of_light_t : public paladin_heal_t
{
  beacon_of_light_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "beacon_of_light", p, p -> find_class_spell( "Beacon of Light" ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;

    // Target is required for Beacon
    if ( option.target_str.empty() )
    {
      sim -> errorf( "Warning %s's \"%s\" needs a target", p -> name(), name() );
      sim -> cancel();
    }

    // Remove the 'dot'
    dot_duration = timespan_t::zero();
  }

  virtual void execute() override
  {
    paladin_heal_t::execute();

    p() -> beacon_target = target;
    target -> buffs.beacon_of_light -> trigger();
  }
};

struct beacon_of_light_heal_t : public heal_t
{
  beacon_of_light_heal_t( paladin_t* p ) :
    heal_t( "beacon_of_light_heal", p, p -> find_spell( 53652 ) )
  {
    background = true;
    may_crit = false;
    proc = true;
    trigger_gcd = timespan_t::zero();

    target = p -> beacon_target;
  }
};

// Blessed Hammer (Protection) ================================================

struct blessed_hammer_tick_t : public paladin_spell_t
{
  blessed_hammer_tick_t( paladin_t* p ) :
    paladin_spell_t( "blessed_hammer_tick", p, p -> find_spell( 204301 ) )
  {
    aoe = -1;
    background = dual = direct_tick = true;
    callbacks = false;
    radius = 9.0; // Guess, must be > 8 (cons) but < 10 (HoJ)
    may_crit = true;
  }

  virtual void impact( action_state_t* s ) override
  {
    paladin_spell_t::impact( s );

    // apply BH debuff to target_data structure
    td( s -> target ) -> buffs.blessed_hammer_debuff -> trigger();
  }
};

struct blessed_hammer_t : public paladin_spell_t
{
  blessed_hammer_tick_t* hammer;
  int num_strikes;

  blessed_hammer_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "blessed_hammer", p, p -> find_talent_spell( "Blessed Hammer" ) ),
    hammer( new blessed_hammer_tick_t( p ) ), num_strikes( 3 )
  {
    add_option( opt_int( "strikes", num_strikes ) );
    parse_options( options_str );

    // Sane bounds for num_strikes - only makes three revolutions, impossible to hit one target more than 3 times.
    // Likewise calling the pell with 0 strikes is sort of pointless.
    if ( num_strikes < 1 ) { num_strikes = 1; sim -> out_debug.printf( "%s tried to hit less than one time with blessed_hammer", p -> name() ); }
    if ( num_strikes > 3 ) { num_strikes = 3; sim -> out_debug.printf( "%s tried to hit more than three times with blessed_hammer", p -> name() ); }

    dot_duration = timespan_t::zero(); // The periodic event is handled by ground_aoe_event_t
    may_miss = false;
    base_tick_time = timespan_t::from_seconds( 1.667 ); // Rough estimation based on stopwatch testing
    cooldown -> hasted = true;

    tick_may_crit = true;

    add_child( hammer );
  }

  void execute() override
  {
    paladin_spell_t::execute();

    timespan_t initial_delay = num_strikes < 3 ? base_tick_time * 0.25 : timespan_t::zero();

    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
        .target( execute_state -> target )
        // spawn at feet of player
        .x( execute_state -> action -> player -> x_position )
        .y( execute_state -> action -> player -> y_position )
        .pulse_time( base_tick_time )
        .duration( base_tick_time * ( num_strikes - 0.5 ) )
        .start_time( sim -> current_time() + initial_delay )
        .action( hammer ), true );

    // Oddly, Grand Crusader seems to proc on cast whether it hits or not (tested 6/19/2016 by Theck on PTR)
    p() -> trigger_grand_crusader();
  }

};

// Consecration =============================================================

struct consecration_tick_t: public paladin_spell_t {
  consecration_tick_t( paladin_t* p )
    : paladin_spell_t( "consecration_tick", p, p -> find_spell( 81297 ) )
  {
    aoe = -1;
    dual = true;
    direct_tick = true;
    background = true;
    may_crit = true;
    ground_aoe = true;
    if ( p -> specialization() == PALADIN_RETRIBUTION )
    {
      base_multiplier *= 1.0 + p -> passives.retribution_paladin -> effectN( 8 ).percent();
    }
  }
};

// healing tick from Consecrated Ground talent
struct consecrated_ground_tick_t : public paladin_heal_t
{
  consecrated_ground_tick_t( paladin_t* p )
    : paladin_heal_t( "consecrated_ground", p, p -> find_spell( 204241 ) )
  {
    aoe = 6;
    ground_aoe = true;
    background = true;
    direct_tick = true;
  }
};

struct consecration_t : public paladin_spell_t
{
  consecration_tick_t* damage_tick;
  consecrated_ground_tick_t* heal_tick;
  timespan_t ground_effect_duration;

  consecration_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "consecration", p, p -> specialization() == PALADIN_RETRIBUTION ? p -> find_spell( 205228 ) : p -> find_class_spell( "Consecration" ) ),
    damage_tick( new consecration_tick_t( p ) ), heal_tick( new consecrated_ground_tick_t( p ) ),
    ground_effect_duration( data().duration() )
  {
    parse_options( options_str );

    // disable if Ret and not talented
    if ( p -> specialization() == PALADIN_RETRIBUTION )
      background = ! ( p -> talents.consecration -> ok() );

    dot_duration = timespan_t::zero(); // the periodic event is handled by ground_aoe_event_t
    may_miss       = false;

    add_child( damage_tick );

    // Consecrated In Flame extends duration
    ground_effect_duration += timespan_t::from_millis( p -> artifact.consecration_in_flame.value() );
  }

  virtual void execute() override
  {
    paladin_spell_t::execute();

    // create a new ground aoe event
    paladin_ground_aoe_t* alt_aoe =
        make_event<paladin_ground_aoe_t>( *sim, p(), ground_aoe_params_t()
        .target( execute_state -> target )
        // spawn at feet of player
        .x( execute_state -> action -> player -> x_position )
        .y( execute_state -> action -> player -> y_position )
        // TODO: this is a hack that doesn't work properly, fix this correctly
        .duration( ground_effect_duration )
        .start_time( sim -> current_time()  )
        .action( damage_tick )
        .hasted( ground_aoe_params_t::NOTHING ), true );

    // push the pointer to the list of active consecrations
    // execute() and schedule_event() methods of paladin_ground_aoe_t handle updating the list
    p() -> active_consecrations.push_back( alt_aoe );

    // if we've talented consecrated ground, make a second healing ground effect for that
    // this can be a normal ground_aoe_event_t, since we don't need the extra stuff
    if ( p() -> talents.consecrated_ground -> ok() )
    {
      make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
          .target( execute_state -> action -> player )
          // spawn at feet of player
          .x( execute_state -> action -> player -> x_position )
          .y( execute_state -> action -> player -> y_position )
          .duration( ground_effect_duration )
          .start_time( sim -> current_time()  )
          .action( heal_tick )
          .hasted( ground_aoe_params_t::NOTHING ), true );
    }
  }
};

// Divine Protection ========================================================

struct divine_protection_t : public paladin_spell_t
{
  divine_protection_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "divine_protection", p, p -> find_class_spell( "Divine Protection" ) )
  {
    parse_options( options_str );

    harmful = false;
    use_off_gcd = true;
    trigger_gcd = timespan_t::zero();

    // unbreakable spirit reduces cooldown
    if ( p -> talents.unbreakable_spirit -> ok() )
      cooldown -> duration = data().cooldown() * ( 1 + p -> talents.unbreakable_spirit -> effectN( 1 ).percent() );
  }

  virtual void execute() override
  {
    paladin_spell_t::execute();

    p() -> buffs.divine_protection -> trigger();
  }
};

// SoV

struct shield_of_vengeance_t : public paladin_absorb_t
{
  shield_of_vengeance_t( paladin_t* p, const std::string& options_str ) :
    paladin_absorb_t( "shield_of_vengeance", p, p -> find_spell( 184662 ) )
  {
    parse_options( options_str );

    harmful = false;
    use_off_gcd = true;
    trigger_gcd = timespan_t::zero();

    may_crit = true;
    // TODO: figure out where this is from
    // While the tooltip says 10x, in practice the spell has been doing 20x.
    attack_power_mod.direct = 20;
    if ( p -> artifact.deflection.rank() )
    {
      cooldown -> duration += timespan_t::from_millis( p -> artifact.deflection.value() );
    }
  }

  virtual void execute() override
  {
    paladin_absorb_t::execute();

    p() -> buffs.shield_of_vengeance -> trigger();
  }
};

// Divine Shield ==============================================================

struct divine_shield_t : public paladin_spell_t
{
  divine_shield_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "divine_shield", p, p -> find_class_spell( "Divine Shield" ) )
  {
    parse_options( options_str );

    harmful = false;

    // unbreakable spirit reduces cooldown
    if ( p -> talents.unbreakable_spirit -> ok() )
      cooldown -> duration = data().cooldown() * ( 1 + p -> talents.unbreakable_spirit -> effectN( 1 ).percent() );
  }

  bool init_finished() override
  {
    p() -> forbearant_faithful_cooldowns.push_back( cooldown );

    return paladin_spell_t::init_finished();
  }

  virtual void execute() override
  {
    paladin_spell_t::execute();

    // Technically this should also drop you from the mob's threat table,
    // but we'll assume the MT isn't using it for now
    p() -> buffs.divine_shield -> trigger();

    // in this sim, the only debuffs we care about are enemy DoTs.
    // Check for them and remove them when cast
    int num_destroyed = 0;
    for ( size_t i = 0, size = p() -> dot_list.size(); i < size; i++ )
    {
      dot_t* d = p() -> dot_list[ i ];

      if ( d -> source != p() && d -> source -> is_enemy() && d -> is_ticking() )
      {
        d -> cancel();
        num_destroyed++;
      }
    }

    // trigger forbearance
    if ( p() -> artifact.endless_resolve.rank() )
      // But not if we have endless resolve!
      return;
    timespan_t duration = p() -> debuffs.forbearance -> data().duration();
    p() -> debuffs.forbearance -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration  );
    // add ourselves to the target_data list for tracking of forbearant faithful
    td( p() ) -> buffs.forbearant_faithful -> trigger();
  }

  double recharge_multiplier() const override
  {
    double cdr = paladin_spell_t::recharge_multiplier();

    cdr *= p() -> get_forbearant_faithful_recharge_multiplier();

    return cdr;
  }

  virtual bool ready() override
  {
    if ( player -> debuffs.forbearance -> check() )
      return false;

    return paladin_spell_t::ready();
  }
};

// Divine Steed (Protection, Retribution) =====================================

struct divine_steed_t : public paladin_spell_t
{
  divine_steed_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "divine_steed", p, p -> find_spell( "Divine Steed" ) )
  {
    parse_options( options_str );

    // disable for Ret unless talent is taken
    if ( p -> specialization() == PALADIN_RETRIBUTION && ! p -> talents.divine_steed -> ok() )
      background = true;

    // adjust cooldown based on Knight Templar talent for prot
    if ( p -> talents.knight_templar -> ok() )
      cooldown -> duration *= 1.0 + p -> talents.knight_templar -> effectN( 1 ).percent();

  }

  void execute() override
  {
    paladin_spell_t::execute();

    p() -> buffs.divine_steed -> trigger();
  }

};

// Denounce =================================================================

struct denounce_t : public paladin_spell_t
{
  denounce_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "denounce", p, p -> find_specialization_spell( "Denounce" ) )
  {
    parse_options( options_str );
    may_crit = true;
  }

  void execute() override
  {
    paladin_spell_t::execute();
  }
};

// Execution Sentence =======================================================

struct execution_sentence_t : public paladin_spell_t
{
  execution_sentence_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "execution_sentence", p, p -> find_talent_spell( "Execution Sentence" ) )
  {
    parse_options( options_str );
    hasted_ticks   = true;
    travel_speed   = 0;
    tick_may_crit  = true;

    // disable if not talented
    if ( ! ( p -> talents.execution_sentence -> ok() ) )
      background = true;

    if ( p -> sets.has_set_bonus( PALADIN_RETRIBUTION, T19, B2 ) )
    {
      base_multiplier *= 1.0 + p -> sets.set( PALADIN_RETRIBUTION, T19, B2 ) -> effectN( 2 ).percent();
    }
  }

  void init() override
  {
    paladin_spell_t::init();

    update_flags &= ~STATE_HASTE;
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    return dot_duration * ( tick_time( s ) / base_tick_time );
  }

  virtual double cost() const override
  {
    double base_cost = paladin_spell_t::cost();
    if ( p() -> buffs.the_fires_of_justice -> up() && base_cost > 0 )
      return base_cost - 1;
    return base_cost;
  }

  void execute() override
  {
    double c = cost();
    paladin_spell_t::execute();

    if ( c <= 0.0 )
    {
      if ( p() -> buffs.divine_purpose -> check() )
      {
        p() -> buffs.divine_purpose -> expire();
      }
    }
    else if ( p() -> buffs.the_fires_of_justice -> up() )
    {
      p() -> buffs.the_fires_of_justice -> expire();
    }

    if ( p() -> buffs.crusade -> check() )
    {
      int num_stacks = (int)base_cost();
      p() -> buffs.crusade -> trigger( num_stacks );
    }
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = paladin_spell_t::composite_target_multiplier( t );

    paladin_td_t* td = this -> td( t );

    if ( td -> buffs.debuffs_judgment -> up() )
    {
      double judgment_multiplier = 1.0 + td -> buffs.debuffs_judgment -> data().effectN( 1 ).percent() + p() -> get_divine_judgment();
      judgment_multiplier += p() -> passives.judgment -> effectN( 1 ).percent();
      m *= judgment_multiplier;
    }

    return m;
  }
};

// Flash of Light Spell =====================================================

struct flash_of_light_t : public paladin_heal_t
{
  flash_of_light_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "flash_of_light", p, p -> find_class_spell( "Flash of Light" ) )
  {
    parse_options( options_str );
  }

  virtual void impact( action_state_t* s ) override
  {
    paladin_heal_t::impact( s );

    // Grant Mana if healing the beacon target
    if ( s -> target == p() -> beacon_target ){
      int g = static_cast<int>( tower_of_radiance -> effectN(1).percent() * cost() );
      p() -> resource_gain( RESOURCE_MANA, g, p() -> gains.mana_beacon_of_light );
    }
  }
};

// Guardian of Ancient Kings ============================================

struct guardian_of_ancient_kings_t : public paladin_spell_t
{
  guardian_of_ancient_kings_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "guardian_of_ancient_kings", p, p -> find_specialization_spell( "Guardian of Ancient Kings" ) )
  {
    parse_options( options_str );
    use_off_gcd = true;
    trigger_gcd = timespan_t::zero();
  }

  virtual void execute() override
  {
    paladin_spell_t::execute();

    p() -> buffs.guardian_of_ancient_kings -> trigger();

  }
};

// Blessing of Sacrifice ========================================================

struct blessing_of_sacrifice_redirect_t : public paladin_spell_t
{
  blessing_of_sacrifice_redirect_t( paladin_t* p ) :
    paladin_spell_t( "blessing_of_sacrifice_redirect", p, p -> find_class_spell( "Blessing of Sacrifice" ) )
  {
    background = true;
    trigger_gcd = timespan_t::zero();
    may_crit = false;
    may_miss = false;
    base_multiplier = data().effectN( 1 ).percent();
    target = p;
  }

  void trigger( double redirect_value )
  {
    // set the redirect amount based on the result of the action
    base_dd_min = redirect_value;
    base_dd_max = redirect_value;
  }

  // Blessing of Sacrifice's Execute function is defined after Paladin Buffs, Part Deux because it requires
  // the definition of the buffs_t::blessing_of_sacrifice_t object.
};

struct blessing_of_sacrifice_t : public paladin_spell_t
{
  blessing_of_sacrifice_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "blessing_of_sacrifice", p, p-> find_class_spell( "Blessing of Sacrifice" ) )
  {
    parse_options( options_str );

    cooldown -> duration += timespan_t::from_millis( p -> artifact.sacrifice_of_the_just.value( 1 ) );

    harmful = false;
    may_miss = false;
//    p -> active.blessing_of_sacrifice_redirect = new blessing_of_sacrifice_redirect_t( p );


    // Create redirect action conditionalized on the existence of HoS.
    if ( ! p -> active.blessing_of_sacrifice_redirect )
      p -> active.blessing_of_sacrifice_redirect = new blessing_of_sacrifice_redirect_t( p );
  }

  virtual void execute() override;

};

// Holy Light Spell =========================================================

struct holy_light_t : public paladin_heal_t
{
  holy_light_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "holy_light", p, p -> find_class_spell( "Holy Light" ) )
  {
    parse_options( options_str );
  }

  virtual void execute() override
  {
    paladin_heal_t::execute();

    p() -> buffs.infusion_of_light -> expire();
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t t = paladin_heal_t::execute_time();

    if ( p() -> buffs.infusion_of_light -> check() )
      t += p() -> buffs.infusion_of_light -> data().effectN( 1 ).time_value();

    return t;
  }

  virtual void schedule_execute( action_state_t* state = nullptr ) override
  {
    paladin_heal_t::schedule_execute( state );

    p() -> buffs.infusion_of_light -> up(); // Buff uptime tracking
  }

  virtual void impact( action_state_t* s ) override
  {
    paladin_heal_t::impact( s );

    // Grant Mana if healing the beacon target
    if ( s -> target == p() -> beacon_target ){
      int g = static_cast<int>( tower_of_radiance -> effectN(1).percent() * cost() );
      p() -> resource_gain( RESOURCE_MANA, g, p() -> gains.mana_beacon_of_light );
    }
  }

};

// Holy Shield damage proc ====================================================

struct holy_shield_proc_t : public paladin_spell_t
{
  holy_shield_proc_t( paladin_t* p )
    : paladin_spell_t( "holy_shield", p, p -> find_spell( 157122 ) ) // damage data stored in 157122
  {
    background = true;
    proc = true;
    may_miss = false;
    may_crit = true;
  }

};

// Painful Truths proc ========================================================

struct painful_truths_proc_t : public paladin_spell_t
{
  painful_truths_proc_t( paladin_t* p )
    : paladin_spell_t( "painful_truths", p, p -> find_spell( 209331 ) ) // damage stored in 209331
  {
    background = true;
    proc = true;
    may_miss = false;
    may_crit = true;
  }

};

// Tyr's Enforcer proc ========================================================

struct tyrs_enforcer_proc_t : public paladin_spell_t
{
  tyrs_enforcer_proc_t( paladin_t* p )
    : paladin_spell_t( "tyrs_enforcer", p, p -> find_spell( 209478 ) ) // damage stored in 209478
  {
    background = true;
    proc = true;
    may_miss = false;
    aoe = -1;
    may_crit = true;
  }
};

// Holy Prism ===============================================================

// Holy Prism AOE (damage) - This is the aoe damage proc that triggers when holy_prism_heal is cast.

struct holy_prism_aoe_damage_t : public paladin_spell_t
{
  holy_prism_aoe_damage_t( paladin_t* p )
    : paladin_spell_t( "holy_prism_aoe_damage", p, p->find_spell( 114871 ) )
  {
    background = true;
    may_crit = true;
    may_miss = false;
    aoe = 5;

  }
};

// Holy Prism AOE (heal) -  This is the aoe healing proc that triggers when holy_prism_damage is cast

struct holy_prism_aoe_heal_t : public paladin_heal_t
{
  holy_prism_aoe_heal_t( paladin_t* p )
    : paladin_heal_t( "holy_prism_aoe_heal", p, p->find_spell( 114871 ) )
  {
    background = true;
    aoe = 5;
  }

};

// Holy Prism (damage) - This is the damage version of the spell; for the heal version, see holy_prism_heal_t

struct holy_prism_damage_t : public paladin_spell_t
{
  holy_prism_damage_t( paladin_t* p )
    : paladin_spell_t( "holy_prism_damage", p, p->find_spell( 114852 ) )
  {
    background = true;
    may_crit = true;
    may_miss = false;
    // this updates the spell coefficients appropriately for single-target damage
    parse_effect_data( p -> find_spell( 114852 ) -> effectN( 1 ) );

    // on impact, trigger a holy_prism_aoe_heal cast
    impact_action = new holy_prism_aoe_heal_t( p );
  }
};


// Holy Prism (heal) - This is the healing version of the spell; for the damage version, see holy_prism_damage_t

struct holy_prism_heal_t : public paladin_heal_t
{
  holy_prism_heal_t( paladin_t* p ) :
    paladin_heal_t( "holy_prism_heal", p, p -> find_spell( 114871 ) )
  {
    background = true;

    // update spell coefficients appropriately for single-target healing
    parse_effect_data( p -> find_spell( 114871 ) -> effectN( 1 ) );

    // on impact, trigger a holy_prism_aoe cast
    impact_action = new holy_prism_aoe_damage_t( p );
  }

};

// Holy Prism - This is the base spell, it chooses the effects to trigger based on the target

struct holy_prism_t : public paladin_spell_t
{
  holy_prism_damage_t* damage;
  holy_prism_heal_t* heal;

  holy_prism_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "holy_prism", p, p->find_spell( 114165 ) )
  {
    parse_options( options_str );

    // create the damage and healing spell effects, designate them as children for reporting
    damage = new holy_prism_damage_t( p );
    add_child( damage );
    heal = new holy_prism_heal_t( p );
    add_child( heal );

    // disable if not talented
    if ( ! ( p -> talents.holy_prism -> ok() ) )
      background = true;
  }

  virtual void execute() override
  {
    if ( target -> is_enemy() )
    {
      // damage enemy
      damage -> target = target;
      damage -> schedule_execute();
    }
    else
    {
      // heal friendly
      heal -> target = target;
      heal -> schedule_execute();
    }

    paladin_spell_t::consume_resource();
    paladin_spell_t::update_ready();
  }
};

// Holy Shock ===============================================================

// Holy Shock is another one of those "heals or does damage depending on target" spells.
// The holy_shock_t structure handles global stuff, and calls one of two children
// (holy_shock_damage_t or holy_shock_heal_t) depending on target to handle healing/damage
// find_class_spell returns spell 20473, which has the cooldown/cost/etc stuff, but the actual
// damage and healing information is in spells 25912 and 25914, respectively.

struct holy_shock_damage_t : public paladin_spell_t
{
  double crit_chance_multiplier;

  holy_shock_damage_t( paladin_t* p )
    : paladin_spell_t( "holy_shock_damage", p, p -> find_spell( 25912 ) )
  {
    background = may_crit = true;
    trigger_gcd = timespan_t::zero();

    // this grabs the 100% base crit bonus from 20473
    crit_chance_multiplier = p -> find_class_spell( "Holy Shock" ) -> effectN( 1 ).base_value() / 10.0;
  }

  virtual double composite_crit_chance() const override
  {
    double cc = paladin_spell_t::composite_crit_chance();

    // effect 1 doubles crit chance
    cc *= crit_chance_multiplier;

    return cc;
  }
};

// Holy Shock Heal Spell ====================================================

struct holy_shock_heal_t : public paladin_heal_t
{
  double crit_chance_multiplier;

  holy_shock_heal_t( paladin_t* p ) :
    paladin_heal_t( "holy_shock_heal", p, p -> find_spell( 25914 ) )
  {
    background = true;
    trigger_gcd = timespan_t::zero();

    // this grabs the crit multiplier bonus from 20473
    crit_chance_multiplier = p -> find_class_spell( "Holy Shock" ) -> effectN( 1 ).base_value() / 10.0;
  }

  virtual double composite_crit_chance() const override
  {
    double cc = paladin_heal_t::composite_crit_chance();

    // effect 1 doubles crit chance
    cc *= crit_chance_multiplier;

    return cc;
  }

  virtual void execute() override
  {
    paladin_heal_t::execute();

    if ( execute_state -> result == RESULT_CRIT )
      p() -> buffs.infusion_of_light -> trigger();

  }
};

struct holy_shock_t : public paladin_heal_t
{
  holy_shock_damage_t* damage;
  holy_shock_heal_t* heal;
  timespan_t cd_duration;
  double cooldown_mult;
  bool dmg;

  holy_shock_t( paladin_t* p, const std::string& options_str )
    : paladin_heal_t( "holy_shock", p, p -> find_specialization_spell( "Holy Shock" ) ),
    cooldown_mult( 1.0 ), dmg( false )
  {
    add_option( opt_bool( "damage", dmg ) );
    check_spec( PALADIN_HOLY );
    parse_options( options_str );

    cd_duration = cooldown -> duration;

    // Bonuses from Sanctified Wrath need to be stored for future use
    if ( ( p -> specialization() == PALADIN_HOLY ) && p -> talents.sanctified_wrath -> ok()  )
      cooldown_mult = p -> talents.sanctified_wrath -> effectN( 2 ).percent();

    // create the damage and healing spell effects, designate them as children for reporting
    damage = new holy_shock_damage_t( p );
    add_child( damage );
    heal = new holy_shock_heal_t( p );
    add_child( heal );
  }

  virtual void execute() override
  {
    if ( dmg )
    {
      // damage enemy
      damage -> target = target;
      damage -> schedule_execute();
    }
    else
    {
      // heal friendly
      heal -> target = target;
      heal -> schedule_execute();
    }

    cooldown -> duration = cd_duration;

    paladin_heal_t::execute();
  }

  double cooldown_multiplier() override
  {
    double cdm = paladin_heal_t::cooldown_multiplier();

    if ( p() -> buffs.avenging_wrath -> check() )
      cdm += cooldown_mult;

    return cdm;
  }
};

// Judgment of Light proc =====================================================

struct judgment_of_light_proc_t : public paladin_heal_t
{
  judgment_of_light_proc_t( paladin_t* p )
    : paladin_heal_t( "judgment_of_light", p, p -> find_spell( 183811 ) ) // proc data stored in 183811
  {
    background = true;
    proc = true;
    may_miss = false;
    may_crit = true;

    // it seems this updates dynamically with SP changes and AW because the heal comes from the paladin
    // thus, we can treat it as a heal cast by the paladin and not worry about snapshotting.

    // NOTE: this is implemented in SimC as a self-heal only. It does NOT proc for other players attacking the boss.
    // This is mostly done because it's much simpler to code, and for the most part Prot doesn't care about raid healing efficiency.
    // If Holy wants this to work like the in-game implementation, they'll have to go through the pain of moving things to player_t
  }

  virtual void execute() override
  {
    paladin_heal_t::execute();

    p() -> last_jol_proc = p() -> sim -> current_time();
  }
};

// Lay on Hands Spell =======================================================

struct lay_on_hands_t : public paladin_heal_t
{
  double mana_return_pct;
  lay_on_hands_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "lay_on_hands", p, p -> find_class_spell( "Lay on Hands" ) ), mana_return_pct( 0 )
  {
      parse_options( options_str );

      // unbreakable spirit reduces cooldown
      if ( p -> talents.unbreakable_spirit -> ok() )
          cooldown -> duration = data().cooldown() * ( 1 + p -> talents.unbreakable_spirit -> effectN( 1 ).percent() );

      may_crit = false;
      use_off_gcd = true;
      trigger_gcd = timespan_t::zero();
  }

  bool init_finished() override
  {
    p() -> forbearant_faithful_cooldowns.push_back( cooldown );

    return paladin_heal_t::init_finished();
  }

  virtual void execute() override
  {
    base_dd_min = base_dd_max = p() -> resources.max[ RESOURCE_HEALTH ];

    paladin_heal_t::execute();

    if ( p() -> artifact.endless_resolve.rank() )
      // Don't trigger forbearance with endless resolve
      return;

    // apply forbearance, track locally for forbearant faithful & force recharge recalculation
    target -> debuffs.forbearance -> trigger();
    td( target ) -> buffs.forbearant_faithful -> trigger();
    p() -> update_forbearance_recharge_multipliers();
  }

  virtual bool ready() override
  {
    if ( target -> debuffs.forbearance -> check() )
      return false;

    return paladin_heal_t::ready();
  }

  double recharge_multiplier() const override
  {
    double cdr = paladin_heal_t::recharge_multiplier();

    cdr *= p() -> get_forbearant_faithful_recharge_multiplier();

    return cdr;
  }
};

// Light of the Titans proc ===================================================

struct light_of_the_titans_t : public paladin_heal_t
{
  light_of_the_titans_t( paladin_t* p )
    : paladin_heal_t( "light_of_the_titans", p, p -> find_spell( 209540 ) ) // hot stored in 209540
  {
    background = true;
    proc = true;
  }
};

// Light of the Protector (Protection) ========================================

struct light_of_the_protector_t : public paladin_heal_t
{
  double health_diff_pct;
  light_of_the_titans_t* titans_proc;

  light_of_the_protector_t( paladin_t* p, const std::string& options_str )
    : paladin_heal_t( "light_of_the_protector", p, p -> find_specialization_spell( "Light of the Protector" ) ), health_diff_pct( 0 )
  {
    parse_options( options_str );

    may_crit = false;
    use_off_gcd = true;
    target = p;

    // pct of missing health returned
    health_diff_pct = data().effectN( 1 ).percent();

    // link needed for Righteous Protector / SotR cooldown reduction
    cooldown = p -> cooldowns.light_of_the_protector;

    // prevent spamming
    internal_cooldown -> duration = timespan_t::from_seconds( 1.0 );

    // disable if Hand of the Protector talented
    if ( p -> talents.hand_of_the_protector -> ok() )
      background = true;

    // light of the titans object attached to this
    if ( p -> artifact.light_of_the_titans.rank() ){
      titans_proc = new light_of_the_titans_t( p );
    } else {
      titans_proc = nullptr;
    }
  }

  double action_multiplier() const override
  {
    double am = paladin_heal_t::action_multiplier();

    if ( p() -> standing_in_consecration() || p() -> talents.consecrated_hammer -> ok() )
      am *= 1.0 + p() -> spells.consecration_bonus -> effectN( 2 ).percent();

    am *= 1.0 + p() -> artifact.scatter_the_shadows.percent();

    return am;
  }

  virtual void execute() override
  {
    // heals for 25% of missing health.
    base_dd_min = base_dd_max = health_diff_pct * ( p() -> resources.max[ RESOURCE_HEALTH ] - std::max( p() -> resources.current[ RESOURCE_HEALTH ], 0.0 ) );

    paladin_heal_t::execute();
    if ( titans_proc ){
      titans_proc -> schedule_execute();
    }
  }

};

// Hand of the Protector (Protection) =========================================

struct hand_of_the_protector_t : public paladin_heal_t
{
  double health_diff_pct;
  light_of_the_titans_t* titans_proc;

  hand_of_the_protector_t( paladin_t* p, const std::string& options_str )
    : paladin_heal_t( "hand_of_the_protector", p, p -> find_talent_spell( "Hand of the Protector" ) ), health_diff_pct( 0 )
  {
    parse_options( options_str );

    may_crit = false;
    use_off_gcd = true;

    // pct of missing health returned
    health_diff_pct = data().effectN( 1 ).percent();

    // link needed for Righteous Protector / SotR cooldown reduction
    cooldown = p -> cooldowns.hand_of_the_protector;

    // prevent spamming
    internal_cooldown -> duration = timespan_t::from_seconds( 1.0 );

    // disable if Hand of the Protector is not talented
    if ( ! p -> talents.hand_of_the_protector -> ok() )
      background = true;

    // light of the titans object attached to this
    if ( p -> artifact.light_of_the_titans.rank() ){
      titans_proc = new light_of_the_titans_t( p );
    } else {
      titans_proc = nullptr;
    }
  }

  double action_multiplier() const override
  {
    double am = paladin_heal_t::action_multiplier();

    if ( p() -> standing_in_consecration() || p() -> talents.consecrated_hammer -> ok() )
      am *= 1.0 + p() -> spells.consecration_bonus -> effectN( 2 ).percent();

    am *= 1.0 + p() -> artifact.scatter_the_shadows.percent();

    return am;
  }

  virtual void execute() override
  {
    // heals for % of missing health.
    base_dd_min = base_dd_max = health_diff_pct * (target->resources.max[RESOURCE_HEALTH] - std::max(target->resources.current[RESOURCE_HEALTH], 0.0) );

    paladin_heal_t::execute();

    // Light of the Titans only works if self-cast
    if ( titans_proc && target == p() )
      titans_proc -> schedule_execute();
  }

};

// Light's Hammer =============================================================

struct lights_hammer_damage_tick_t : public paladin_spell_t
{
  lights_hammer_damage_tick_t( paladin_t* p )
    : paladin_spell_t( "lights_hammer_damage_tick", p, p -> find_spell( 114919 ) )
  {
    //dual = true;
    background = true;
    aoe = -1;
    may_crit = true;
    ground_aoe = true;
  }
};

struct lights_hammer_heal_tick_t : public paladin_heal_t
{
  lights_hammer_heal_tick_t( paladin_t* p )
    : paladin_heal_t( "lights_hammer_heal_tick", p, p -> find_spell( 114919 ) )
  {
    dual = true;
    background = true;
    aoe = 6;
    may_crit = true;
  }

  std::vector< player_t* >& target_list() const override
  {
    target_cache.list = paladin_heal_t::target_list();
    target_cache.list = find_lowest_players( aoe );
    return target_cache.list;
  }
};

struct lights_hammer_t : public paladin_spell_t
{
  const timespan_t travel_time_;
  lights_hammer_heal_tick_t* lh_heal_tick;
  lights_hammer_damage_tick_t* lh_damage_tick;

  lights_hammer_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "lights_hammer", p, p -> find_talent_spell( "Light's Hammer" ) ),
      travel_time_( timespan_t::from_seconds( 1.5 ) )
  {
    // 114158: Talent spell, cooldown
    // 114918: Periodic 2s dummy, no duration!
    // 114919: Damage/Scale data
    // 122773: 15.5s duration, 1.5s for hammer to land = 14s aoe dot
    parse_options( options_str );
    may_miss = false;

    school = SCHOOL_HOLY; // Setting this allows the tick_action to benefit from Inquistion

    base_tick_time = p -> find_spell( 114918 ) -> effectN( 1 ).period();
    dot_duration      = p -> find_spell( 122773 ) -> duration() - travel_time_;
    cooldown -> duration = p -> find_spell( 114158 ) -> cooldown();
    hasted_ticks   = false;
    tick_zero = true;
    ignore_false_positive = true;

    dynamic_tick_action = true;
    //tick_action = new lights_hammer_tick_t( p, p -> find_spell( 114919 ) );
    lh_heal_tick = new lights_hammer_heal_tick_t( p );
    add_child( lh_heal_tick );
    lh_damage_tick = new lights_hammer_damage_tick_t( p );
    add_child( lh_damage_tick );

    // disable if not talented
    if ( ! ( p -> talents.lights_hammer -> ok() ) )
      background = true;
  }

  virtual timespan_t travel_time() const override
  { return travel_time_; }

  virtual void tick( dot_t* d ) override
  {
    // trigger healing and damage ticks
    lh_heal_tick -> schedule_execute();
    lh_damage_tick -> schedule_execute();

    paladin_spell_t::tick( d );
  }
};

// Seraphim ( Protection ) ====================================================

struct seraphim_t : public paladin_spell_t
{
  seraphim_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "seraphim", p, p -> find_talent_spell( "Seraphim" ) )
  {
    parse_options( options_str );

    harmful = false;
    may_miss = false;

    // ugly hack for if SotR hasn't been put in the APL yet
    if ( p -> cooldowns.shield_of_the_righteous -> charges < 3 )
    {
      const spell_data_t* sotr = p -> find_class_spell( "Shield of the Righteous" );
      p -> cooldowns.shield_of_the_righteous -> charges = p -> cooldowns.shield_of_the_righteous -> current_charge = sotr -> _charges;
      p -> cooldowns.shield_of_the_righteous -> duration = timespan_t::from_millis( sotr -> _charge_cooldown );
    }
  }

  virtual void execute() override
  {
    paladin_spell_t::execute();

    // duration depends on sotr charges, need to do some math
    timespan_t duration = timespan_t::zero();
    int available_charges = p() -> cooldowns.shield_of_the_righteous -> current_charge;
    int full_charges_used = 0;
    timespan_t remains = p() -> cooldowns.shield_of_the_righteous -> current_charge_remains();

    if ( available_charges >= 1 )
    {
      full_charges_used = std::min( available_charges, 2 );
      duration = full_charges_used * p() -> talents.seraphim -> duration();
      for ( int i = 0; i < full_charges_used; i++ )
        p() -> cooldowns.shield_of_the_righteous -> start( p() -> active_sotr  );
    }
    if ( full_charges_used < 2 )
    {
      double partial_charges_used = 0.0;
      partial_charges_used = ( p() -> cooldowns.shield_of_the_righteous -> duration.total_seconds() - remains.total_seconds() ) / p() -> cooldowns.shield_of_the_righteous -> duration.total_seconds();
      duration += partial_charges_used * p() -> talents.seraphim -> duration();
      p() -> cooldowns.shield_of_the_righteous -> adjust( p() -> cooldowns.shield_of_the_righteous -> duration - remains );
    }

    if ( duration > timespan_t::zero() )
      p() -> buffs.seraphim -> trigger( 1, -1.0, -1.0, duration );

  }

};

// Eye of Tyr (Protection) ====================================================

struct eye_of_tyr_t : public paladin_spell_t
{
  eye_of_tyr_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "eye_of_tyr", p, p -> artifact.eye_of_tyr )
  {
    parse_options( options_str );

    aoe = -1;
    may_crit = true;
  }

  virtual void impact( action_state_t* s ) override
  {
    paladin_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
      td( s -> target ) -> buffs.eye_of_tyr_debuff -> trigger();
  }
};

// Wake of Ashes (Retribution) ================================================

struct wake_of_ashes_t : public paladin_spell_t
{
  wake_of_ashes_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "wake_of_ashes", p, p -> find_spell( 205273 ) )
  {
    parse_options( options_str );

    may_crit = true;
    aoe = -1;
    hasted_ticks = false;
    tick_may_crit = true;

    if ( p -> artifact.ashes_to_ashes.rank() )
    {
      energize_type = ENERGIZE_ON_HIT;
      energize_resource = RESOURCE_HOLY_POWER;
      energize_amount = p -> find_spell( 218001 ) -> effectN( 1 ).resource( RESOURCE_HOLY_POWER );
    }
    else
    {
      attack_power_mod.tick = 0;
      dot_duration = timespan_t::zero();
    }
  }

  bool ready() override
  {
    if ( ! player -> artifact_enabled() )
    {
      return false;
    }

    if ( p() -> artifact.wake_of_ashes.rank() == 0 )
    {
      return false;
    }

    return paladin_spell_t::ready();
  }
};

// Holy Wrath (Retribution) ===================================================

struct holy_wrath_t : public paladin_spell_t
{
  holy_wrath_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "holy_wrath", p, p -> find_talent_spell( "Holy Wrath" ) )
  {
    parse_options( options_str );

    aoe = data().effectN( 2 ).base_value();

    if ( ! ( p -> talents.holy_wrath -> ok() ) )
      background = true;
  }

  double calculate_direct_amount( action_state_t* state ) const override
  {
    double base_amount = 0;
    if ( p() -> fixed_holy_wrath_health_pct > 0 )
      base_amount = p() -> max_health() * ( 100 - p() -> fixed_holy_wrath_health_pct ) / 100.0;
    else
      base_amount = p() -> max_health() - p() -> current_health();
    double amount = base_amount * data().effectN( 3 ).percent();

    state -> result_total = amount;
    return amount;
  }

  bool ready() override
  {
    if ( p() -> fixed_holy_wrath_health_pct > 0 && p() -> fixed_holy_wrath_health_pct < 100 )
      return paladin_spell_t::ready();

    if ( player -> current_health() >= player -> max_health() )
      return false;

    return paladin_spell_t::ready();
  }
};

// Blinding Light (Holy/Prot/Retribution) =====================================

struct blinding_light_effect_t : public paladin_spell_t
{
  blinding_light_effect_t( paladin_t* p )
    : paladin_spell_t( "blinding_light_effect", p, dbc::find_spell( p, 105421 ) )
  {
    background = true;
  }
};

struct blinding_light_t : public paladin_spell_t
{
  blinding_light_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "blinding_light", p, p -> find_talent_spell( "Blinding Light" ) )
  {
    parse_options( options_str );

    aoe = -1;
    execute_action = new blinding_light_effect_t( p );
  }
};

// Light of Dawn (Holy) =======================================================

struct light_of_dawn_t : public paladin_heal_t
{
  light_of_dawn_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "light_of_dawn", p, p -> find_class_spell( "Light of Dawn" ) )
  {
    parse_options( options_str );

    aoe = 6;

    // Holy Insight buffs all healing by 25% & WoG/EF/LoD by 50%.
    // The 25% buff is already in paladin_heal_t, so we need to divide by that first & then apply 50%
    base_multiplier /= 1.0 + p -> passives.holy_insight -> effectN( 6 ).percent();
    base_multiplier *= 1.0 + p -> passives.holy_insight -> effectN( 9 ).percent();
  }
};

// ==========================================================================
// End Spells, Heals, and Absorbs
// ==========================================================================

// ==========================================================================
// Paladin Attacks
// ==========================================================================

// paladin-specific melee_attack_t class for inheritance

struct paladin_melee_attack_t: public paladin_action_t < melee_attack_t >
{
  bool use2hspec;

  paladin_melee_attack_t( const std::string& n, paladin_t* p,
                          const spell_data_t* s = spell_data_t::nil(),
                          bool u2h = true ):
    base_t( n, p, s ),
    use2hspec( u2h )
  {
    may_crit = true;
    special = true;
    weapon = &( p -> main_hand_weapon );
  }

  void retribution_trinket_trigger()
  {
    if ( ! p() -> retribution_trinket )
      return;

    if ( ! ( p() -> last_retribution_trinket_target // Target check
          && p() -> last_retribution_trinket_target == execute_state -> target ) )
    {
      // Wrong target, expire stacks and set new target.
      if ( p() -> last_retribution_trinket_target )
      {
        p() -> procs.focus_of_vengeance_reset -> occur();
        p() -> buffs.retribution_trinket -> expire();
      }
      p() -> last_retribution_trinket_target = execute_state -> target;
    }

    p() -> buffs.retribution_trinket -> trigger();
  }
};

struct holy_power_generator_t : public paladin_melee_attack_t
{
  holy_power_generator_t( const std::string& n, paladin_t* p,
                          const spell_data_t* s = spell_data_t::nil(),
                          bool u2h = true):
                          paladin_melee_attack_t( n, p, s, u2h )
  {

  }

  virtual void execute() override
  {
    paladin_melee_attack_t::execute();

    if ( p() -> sets.has_set_bonus( PALADIN_RETRIBUTION, T19, B4 ) )
    {
      // for some reason this is the same spell as the talent
      // leftover nonsense from when this was Conviction?
      if ( p() -> rng().roll( p() -> sets.set( PALADIN_RETRIBUTION, T19, B4 ) -> proc_chance() ) )
      {
        p() -> resource_gain( RESOURCE_HOLY_POWER, 1, p() -> gains.hp_t19_4p );
        p() -> procs.tfoj_set_bonus -> occur();
      }
    }
  }
};

struct holy_power_consumer_t : public paladin_melee_attack_t
{
  holy_power_consumer_t( const std::string& n, paladin_t* p,
                          const spell_data_t* s = spell_data_t::nil(),
                          bool u2h = true):
                          paladin_melee_attack_t( n, p, s, u2h )
  {
    if ( p -> sets.has_set_bonus( PALADIN_RETRIBUTION, T19, B2 ) )
    {
      base_multiplier *= 1.0 + p -> sets.set( PALADIN_RETRIBUTION, T19, B2 ) -> effectN( 1 ).percent();
    }
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = paladin_melee_attack_t::composite_target_multiplier( t );

    paladin_td_t* td = this -> td( t );

    if ( td -> buffs.debuffs_judgment -> up() )
    {
      double judgment_multiplier = 1.0 + td -> buffs.debuffs_judgment -> data().effectN( 1 ).percent() + p() -> get_divine_judgment();
      judgment_multiplier += p() -> passives.judgment -> effectN( 1 ).percent();
      m *= judgment_multiplier;
    }

    return m;
  }

  virtual void execute() override
  {
    double c = cost();
    paladin_melee_attack_t::execute();
    if ( c <= 0.0 )
    {
      if ( p() -> buffs.divine_purpose -> check() )
      {
        p() -> buffs.divine_purpose -> expire();
      }
    }

    if ( p() -> talents.divine_purpose -> ok() )
    {
      bool success = p() -> buffs.divine_purpose -> trigger( 1,
        p() -> buffs.divine_purpose -> default_value,
        p() -> spells.divine_purpose_ret -> proc_chance() );
      if ( success )
        p() -> procs.divine_purpose -> occur();
    }

    if ( p() -> buffs.crusade -> check() )
    {
      int num_stacks = (int)base_cost();
      p() -> buffs.crusade -> trigger( num_stacks );
    }

    if ( p() -> artifact.righteous_verdict.rank() )
    {
      p() -> buffs.righteous_verdict -> trigger();
    }
  }
};


// Custom events
struct whisper_of_the_nathrezim_event_t : public event_t
{
  paladin_t* paladin;

  whisper_of_the_nathrezim_event_t( paladin_t* p, timespan_t delay ) :
    event_t( *p, delay ), paladin( p )
  {
  }

  const char* name() const override
  { return "whisper_of_the_nathrezim_delay"; }

  void execute() override
  {
    paladin -> buffs.whisper_of_the_nathrezim -> trigger();
  }
};

struct echoed_spell_event_t : public event_t
{
  paladin_melee_attack_t* echo;
  paladin_t* paladin;

  echoed_spell_event_t( paladin_t* p, paladin_melee_attack_t* spell, timespan_t delay ) :
    event_t( *p, delay ), echo( spell ), paladin( p )
  {
  }

  const char* name() const override
  { return "echoed_spell_delay"; }

  void execute() override
  {
    echo -> schedule_execute();
  }
};

// Melee Attack =============================================================

struct melee_t : public paladin_melee_attack_t
{
  bool first;
  melee_t( paladin_t* p ) :
    paladin_melee_attack_t( "melee", p, spell_data_t::nil(), true ),
    first( true )
  {
    school = SCHOOL_PHYSICAL;
    special               = false;
    background            = true;
    repeating             = true;
    trigger_gcd           = timespan_t::zero();
    base_execute_time     = p -> main_hand_weapon.swing_time;
  }

  virtual timespan_t execute_time() const override
  {
    if ( ! player -> in_combat ) return timespan_t::from_seconds( 0.01 );
    if ( first )
      return timespan_t::zero();
    else
      return paladin_melee_attack_t::execute_time();
  }

  virtual void execute() override
  {
    if ( first )
      first = false;

    paladin_melee_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      // Check for BoW procs
      if ( p() -> talents.blade_of_wrath -> ok() )
      {
        bool procced = p() -> blade_of_wrath_rppm -> trigger();

        if ( procced )
        {
          p() -> procs.blade_of_wrath -> occur();
          p() -> buffs.blade_of_wrath -> trigger();
          p() -> cooldowns.blade_of_justice -> reset( true );
        }
      }
    }
  }
};

// Auto Attack ==============================================================

struct auto_melee_attack_t : public paladin_melee_attack_t
{
  auto_melee_attack_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "auto_attack", p, spell_data_t::nil(), true )
  {
    school = SCHOOL_PHYSICAL;
    assert( p -> main_hand_weapon.type != WEAPON_NONE );
    p -> main_hand_attack = new melee_t( p );

    // does not incur a GCD
    trigger_gcd = timespan_t::zero();

    parse_options( options_str );
  }

  void execute() override
  {
    p() -> main_hand_attack -> schedule_execute();
  }

  bool ready() override
  {
    if ( p() -> is_moving() )
      return false;

    player_t* potential_target = select_target_if_target();
    if ( potential_target && potential_target != p() -> main_hand_attack -> target )
      p() -> main_hand_attack -> target = potential_target;

    return( p() -> main_hand_attack -> execute_event == nullptr ); // not swinging
  }
};

// Crusader Strike ==========================================================

struct crusader_strike_t : public holy_power_generator_t
{
  crusader_strike_t( paladin_t* p, const std::string& options_str )
    : holy_power_generator_t( "crusader_strike", p, p -> find_class_spell( "Crusader Strike" ), true )
  {
    parse_options( options_str );

    base_multiplier *= 1.0 + p -> artifact.blade_of_light.percent();
    base_crit       += p -> artifact.sharpened_edge.percent();

    if ( p -> talents.fires_of_justice -> ok() )
    {
      cooldown -> duration += timespan_t::from_millis( p -> talents.fires_of_justice -> effectN( 2 ).base_value() );
    }
    const spell_data_t* crusader_strike_2 = p -> find_specialization_spell( 231667 );
    if ( crusader_strike_2 )
    {
      cooldown -> charges += crusader_strike_2 -> effectN( 1 ).base_value();
    }

    background = ( p -> talents.zeal -> ok() );
  }

  void execute() override
  {
    holy_power_generator_t::execute();
    retribution_trinket_trigger();
  }

  void impact( action_state_t* s ) override
  {
    holy_power_generator_t::impact( s );

    // Special things that happen when CS connects
    if ( result_is_hit( s -> result ) )
    {
      // fires of justice
      if ( p() -> talents.fires_of_justice -> ok() )
      {
        bool success = p() -> buffs.the_fires_of_justice -> trigger( 1,
          p() -> buffs.the_fires_of_justice -> default_value,
          p() -> talents.fires_of_justice -> proc_chance() );
        if ( success )
          p() -> procs.the_fires_of_justice -> occur();
      }
    }
  }
};

// Zeal ==========================================================

struct zeal_t : public holy_power_generator_t
{
  zeal_t( paladin_t* p, const std::string& options_str )
    : holy_power_generator_t( "zeal", p, p -> find_talent_spell( "Zeal" ), true )
  {
    parse_options( options_str );

    base_multiplier *= 1.0 + p -> artifact.blade_of_light.percent();
    base_crit += p -> artifact.sharpened_edge.percent();
    chain_multiplier = data().effectN( 1 ).chain_multiplier();

    // TODO: figure out wtf happened to this spell data
    hasted_cd = hasted_gcd = true;
  }

  int n_targets() const override
  {
    if ( p() -> buffs.zeal -> stack() )
      return 1 + p() -> buffs.zeal -> stack();
    return holy_power_generator_t::n_targets();
  }

  void execute() override
  {
    holy_power_generator_t::execute();
    if ( ! ( p() -> bugs ) )
      retribution_trinket_trigger();

    // Special things that happen when Zeal connects
    // Apply Zeal stacks
    p() -> buffs.zeal -> trigger();
  }
};

// Blade of Justice =========================================================

struct blade_of_justice_t : public holy_power_generator_t
{
  blade_of_justice_t( paladin_t* p, const std::string& options_str )
    : holy_power_generator_t( "blade_of_justice", p, p -> find_class_spell( "Blade of Justice" ), true )
  {
    parse_options( options_str );

    // Guarded by the Light and Sword of Light reduce base mana cost; spec-limited so only one will ever be active
    base_costs[ RESOURCE_MANA ] *= 1.0 +  p -> passives.guarded_by_the_light -> effectN( 5 ).percent();
    base_costs[ RESOURCE_MANA ] = floor( base_costs[ RESOURCE_MANA ] + 0.5 );

    base_multiplier *= 1.0 + p -> artifact.deliver_the_justice.percent();

    background = ( p -> talents.divine_hammer -> ok() );

    if ( p -> talents.virtues_blade -> ok() )
      crit_bonus_multiplier += p -> talents.virtues_blade -> effectN( 1 ).percent();
  }

  virtual double action_multiplier() const override
  {
    double am = holy_power_generator_t::action_multiplier();
    if ( p() -> buffs.righteous_verdict -> up() )
      am *= 1.0 + p() -> artifact.righteous_verdict.rank() * 0.08; // todo: fix
    return am;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = holy_power_generator_t::composite_target_multiplier( t );

    if ( p() -> sets.has_set_bonus( PALADIN_RETRIBUTION, T20, B4 ) )
    {
      paladin_td_t* td = this -> td( t );
      if ( td -> buffs.debuffs_judgment -> up() )
      {
        double judgment_multiplier = 1.0 + td -> buffs.debuffs_judgment -> data().effectN( 1 ).percent() + p() -> get_divine_judgment();
        judgment_multiplier += p() -> passives.judgment -> effectN( 1 ).percent();
        m *= judgment_multiplier;
      }
    }

    return m;
  }

  virtual void execute() override
  {
    holy_power_generator_t::execute();
    if ( p() -> buffs.righteous_verdict -> up() )
      p() -> buffs.righteous_verdict -> expire();

    if ( p() -> sets.has_set_bonus( PALADIN_RETRIBUTION, T20, B2 ) )
      p() -> resource_gain( RESOURCE_HOLY_POWER, 1, p() -> gains.hp_t20_2p );
  }
};

// Divine Hammer =========================================================

struct divine_hammer_tick_t : public paladin_melee_attack_t
{

  divine_hammer_tick_t( paladin_t* p )
    : paladin_melee_attack_t( "divine_hammer_tick", p, p -> find_spell( 198137 ) )
  {
    aoe         = -1;
    dual        = true;
    direct_tick = true;
    background  = true;
    may_crit    = true;
    ground_aoe = true;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = paladin_melee_attack_t::composite_target_multiplier( t );

    if ( p() -> sets.has_set_bonus( PALADIN_RETRIBUTION, T20, B4 ) )
    {
      paladin_td_t* td = this -> td( t );
      if ( td -> buffs.debuffs_judgment -> up() )
      {
        double judgment_multiplier = 1.0 + td -> buffs.debuffs_judgment -> data().effectN( 1 ).percent() + p() -> get_divine_judgment();
        judgment_multiplier += p() -> passives.judgment -> effectN( 1 ).percent();
        m *= judgment_multiplier;
      }
    }

    return m;
  }
};

struct divine_hammer_t : public paladin_spell_t
{

  divine_hammer_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "divine_hammer", p, p -> find_talent_spell( "Divine Hammer" ) )
  {
    parse_options( options_str );

    hasted_ticks   = true;
    may_miss       = false;
    tick_may_crit  = true;
    tick_zero      = true;
    energize_type      = ENERGIZE_ON_CAST;
    energize_resource  = RESOURCE_HOLY_POWER;
    energize_amount    = data().effectN( 2 ).base_value();

    // TODO: figure out wtf happened to this spell data
    hasted_cd = hasted_gcd = true;

    base_multiplier *= 1.0 + p -> artifact.deliver_the_justice.percent();

    tick_action = new divine_hammer_tick_t( p );
  }

  void init() override
  {
    paladin_spell_t::init();

    update_flags &= ~STATE_HASTE;
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    return dot_duration * ( tick_time( s ) / base_tick_time );
  }

  virtual double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    double am = paladin_spell_t::composite_persistent_multiplier( s );
    if ( p() -> buffs.righteous_verdict -> up() )
      am *= 1.0 + p() -> artifact.righteous_verdict.rank() * 0.08; // todo: fix
    return am;
  }

  virtual void execute() override
  {
    paladin_spell_t::execute();
    if ( p() -> buffs.righteous_verdict -> up() )
      p() -> buffs.righteous_verdict -> expire();

    if ( p() -> sets.has_set_bonus( PALADIN_RETRIBUTION, T20, B2 ) )
      p() -> resource_gain( RESOURCE_HOLY_POWER, 1, p() -> gains.hp_t20_2p );
  }
};

// Divine Storm =============================================================

struct echoed_divine_storm_t: public paladin_melee_attack_t
{
  echoed_divine_storm_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "echoed_divine_storm", p, p -> find_spell( 224239 ), true )
  {
    parse_options( options_str );

    weapon = &( p -> main_hand_weapon );

    base_multiplier *= p -> artifact.echo_of_the_highlord.percent();

    base_multiplier *= 1.0 + p -> artifact.righteous_blade.percent();
    base_multiplier *= 1.0 + p -> artifact.divine_tempest.percent( 2 );
    if ( p -> talents.final_verdict -> ok() )
      base_multiplier *= 1.0 + p -> talents.final_verdict -> effectN( 2 ).percent();

    ret_damage_increase = true;

    aoe = -1;
    background = true;
  }

  virtual double cost() const override
  {
    return 0;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = paladin_melee_attack_t::composite_target_multiplier( t );

    paladin_td_t* td = this -> td( t );

    if ( td -> buffs.debuffs_judgment -> up() )
    {
      double judgment_multiplier = 1.0 + td -> buffs.debuffs_judgment -> data().effectN( 1 ).percent() + p() -> get_divine_judgment();
      judgment_multiplier += p() -> passives.judgment -> effectN( 1 ).percent();
      m *= judgment_multiplier;
    }

    return m;
  }


  virtual double action_multiplier() const override
  {
    double am = paladin_melee_attack_t::action_multiplier();
    if ( p() -> buffs.whisper_of_the_nathrezim -> check() )
      am *= 1.0 + p() -> buffs.whisper_of_the_nathrezim -> data().effectN( 1 ).percent();
    return am;
  }
};

struct divine_storm_t: public holy_power_consumer_t
{
  echoed_divine_storm_t* echoed_spell;

  struct divine_storm_damage_t : public paladin_melee_attack_t
  {
    divine_storm_damage_t( paladin_t* p )
      : paladin_melee_attack_t( "divine_storm_dmg", p, p -> find_spell( 224239 ) )
    {
      dual = background = true;
      may_miss = may_dodge = may_parry = false;
    }
  };

  divine_storm_t( paladin_t* p, const std::string& options_str )
    : holy_power_consumer_t( "divine_storm", p, p -> find_class_spell( "Divine Storm" ) ),
      echoed_spell( new echoed_divine_storm_t( p, options_str ) )
  {
    parse_options( options_str );

    hasted_gcd = true;

    may_block = false;
    impact_action = new divine_storm_damage_t( p );
    impact_action -> stats = stats;

    weapon = &( p -> main_hand_weapon );

    base_multiplier *= 1.0 + p -> artifact.righteous_blade.percent();
    base_multiplier *= 1.0 + p -> artifact.divine_tempest.percent( 2 );
    if ( p -> talents.final_verdict -> ok() )
      base_multiplier *= 1.0 + p -> talents.final_verdict -> effectN( 2 ).percent();

    aoe = -1;

    ret_damage_increase = true;

    // TODO: Okay, when did this get reset to 1?
    weapon_multiplier = 0;
  }

  virtual double cost() const override
  {
    double base_cost = holy_power_consumer_t::cost();
    if ( p() -> buffs.the_fires_of_justice -> up() && base_cost > 0 )
      return base_cost - 1;
    return base_cost;
  }

  virtual double action_multiplier() const override
  {
    double am = holy_power_consumer_t::action_multiplier();
    if ( p() -> buffs.whisper_of_the_nathrezim -> check() )
      am *= 1.0 + p() -> buffs.whisper_of_the_nathrezim -> data().effectN( 1 ).percent();
    return am;
  }

  virtual void execute() override
  {
    double c = cost();
    holy_power_consumer_t::execute();

    if ( p() -> buffs.the_fires_of_justice -> up() && c > 0 )
      p() -> buffs.the_fires_of_justice -> expire();

    if ( p() -> artifact.echo_of_the_highlord.rank() )
    {
      make_event<echoed_spell_event_t>( *sim, p(), echoed_spell, timespan_t::from_millis( 800 ) );
    }

    if ( p() -> whisper_of_the_nathrezim )
    {
      if ( p() -> buffs.whisper_of_the_nathrezim -> up() )
        p() -> buffs.whisper_of_the_nathrezim -> expire();

      make_event<whisper_of_the_nathrezim_event_t>( *sim, p(), timespan_t::from_millis( 300 ) );
    }
  }

  void record_data( action_state_t* ) override {}
};

// Hammer of Justice, Fist of Justice =======================================

struct hammer_of_justice_damage_spell_t : public paladin_melee_attack_t
{
  hammer_of_justice_damage_spell_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "hammer_of_justice_damage", p, p -> find_spell( 211561 ), true )
  {
    parse_options( options_str );
    weapon = &( p -> main_hand_weapon );
    background = true;
  }
};

struct hammer_of_justice_t : public paladin_melee_attack_t
{
  hammer_of_justice_damage_spell_t* damage_spell;
  hammer_of_justice_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "hammer_of_justice", p, p -> find_class_spell( "Hammer of Justice" ) ),
      damage_spell( new hammer_of_justice_damage_spell_t( p, options_str ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;

    // TODO: this is a hack; figure out what's really going on here.
    if ( ( p -> specialization() == PALADIN_RETRIBUTION ) )
      base_costs[ RESOURCE_MANA ] = 0;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    // TODO this is a hack; figure out a better way to do this
    if ( t -> health_percentage() < p() -> spells.justice_gaze -> effectN( 1 ).base_value() )
      return 0;
    return paladin_melee_attack_t::composite_target_multiplier( t );
  }

  virtual void execute() override
  {
    paladin_melee_attack_t::execute();

    if ( p() -> justice_gaze )
    {
      damage_spell -> schedule_execute();
      if ( target -> health_percentage() > p() -> spells.justice_gaze -> effectN( 1 ).base_value() )
        p() -> cooldowns.hammer_of_justice -> ready -= ( p() -> cooldowns.hammer_of_justice -> duration * p() -> spells.justice_gaze -> effectN( 2 ).percent() );

      p() -> resource_gain( RESOURCE_HOLY_POWER, 1, p() -> gains.hp_justice_gaze );
    }
  }
};

// Hammer of the Righteous ==================================================

struct hammer_of_the_righteous_aoe_t : public paladin_melee_attack_t
{
  hammer_of_the_righteous_aoe_t( paladin_t* p )
    : paladin_melee_attack_t( "hammer_of_the_righteous_aoe", p, p -> find_spell( 88263 ), false )
  {
    // AoE effect always hits if single-target attack succeeds
    // Doesn't proc Grand Crusader
    may_dodge  = false;
    may_parry  = false;
    may_miss   = false;
    background = true;
    aoe        = -1;
    trigger_gcd = timespan_t::zero(); // doesn't incur GCD (HotR does that already)
  }

  size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    paladin_melee_attack_t::available_targets( tl );

    for ( size_t i = 0; i < tl.size(); i++ )
    {
      if ( tl[i] == target ) // Cannot hit the original target.
      {
        tl.erase( tl.begin() + i );
        break;
      }
    }
    return tl.size();
  }

  double action_multiplier() const override
  {
    double am = paladin_melee_attack_t::action_multiplier();

    am *= 1.0 + p() -> artifact.hammer_time.percent( 1 );

    return am;
  }
};

struct hammer_of_the_righteous_t : public paladin_melee_attack_t
{
  hammer_of_the_righteous_aoe_t* hotr_aoe;
  hammer_of_the_righteous_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "hammer_of_the_righteous", p, p -> find_class_spell( "Hammer of the Righteous" ), true )
  {
    parse_options( options_str );

    // eliminate cooldown and infinite charges if consecrated hammer is taken
    if ( p -> talents.consecrated_hammer -> ok() )
    {
      cooldown -> charges = 1;
      cooldown -> duration = timespan_t::zero();
    }
    if ( p -> talents.blessed_hammer -> ok() )
      background = true;

    hotr_aoe = new hammer_of_the_righteous_aoe_t( p );
    // Attach AoE proc as a child
    add_child( hotr_aoe );
  }

  void execute() override
  {
    paladin_melee_attack_t::execute();

    retribution_trinket_trigger();

    // Special things that happen when HotR connects
    if ( result_is_hit( execute_state -> result ) )
    {
      // Grand Crusader
      p() -> trigger_grand_crusader();

      if ( hotr_aoe -> target != execute_state -> target )
        hotr_aoe -> target_cache.is_valid = false;

      if ( p() -> talents.consecrated_hammer -> ok() || p() -> standing_in_consecration() )
      {
        hotr_aoe -> target = execute_state -> target;
        hotr_aoe -> execute();
      }
    }
  }

  double action_multiplier() const override
  {
    double am = paladin_melee_attack_t::action_multiplier();

    am *= 1.0 + p() -> artifact.hammer_time.percent( 1 );

    return am;
  }
};


struct shield_of_vengeance_proc_t : public paladin_spell_t
{
  shield_of_vengeance_proc_t( paladin_t* p )
    : paladin_spell_t( "shield_of_vengeance_proc", p, spell_data_t::nil() )
  {
    school = SCHOOL_HOLY;
    may_miss    = false;
    may_dodge   = false;
    may_parry   = false;
    may_glance  = false;
    background  = true;
    trigger_gcd = timespan_t::zero();
    id = 184689;

    split_aoe_damage = true;
    may_crit = true;
    aoe = -1;
  }

  proc_types proc_type() const
  {
    return PROC1_MELEE_ABILITY;
  }
};

// Judgment =================================================================

struct judgment_aoe_t : public paladin_melee_attack_t
{
  judgment_aoe_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "judgment_aoe", p, p -> find_spell( 228288 ), true )
  {
    parse_options( options_str );

    may_glance = may_block = may_parry = may_dodge = false;
    weapon_multiplier = 0;
    background = true;

    if ( p -> specialization() == PALADIN_RETRIBUTION )
    {
      aoe = 1 + p -> spec.judgment_2 -> effectN( 1 ).base_value();

      base_multiplier *= 1.0 + p -> artifact.highlords_judgment.percent();

      if ( p -> talents.greater_judgment -> ok() )
      {
        aoe += p -> talents.greater_judgment -> effectN( 2 ).base_value();
      }
    }
    else
    {
      assert( false );
    }
  }

  bool impact_targeting( action_state_t* s ) const override
  {
    // this feels like some kind of horrifying hack
    if ( s -> chain_target == 0 )
      return false;
    return paladin_melee_attack_t::impact_targeting( s );
  }

  virtual double composite_target_crit_chance( player_t* t ) const override
  {
    double cc = paladin_melee_attack_t::composite_target_crit_chance( t );

    if ( p() -> talents.greater_judgment -> ok() )
    {
      double threshold = p() -> talents.greater_judgment -> effectN( 1 ).base_value();
      if ( ( t -> health_percentage() > threshold ) )
      {
        // TODO: is this correct? where does this come from?
        return 1.0;
      }
    }

    return cc;
  }

  // Special things that happen when Judgment damages target
  virtual void impact( action_state_t* s ) override
  {
    paladin_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      td( s -> target ) -> buffs.debuffs_judgment -> trigger();
    }
  }

  double action_multiplier() const override
  {
    double am = paladin_melee_attack_t::action_multiplier();
    // todo: refer to actual spelldata instead of magic constant
    am *= 1.0 + 2 * p() -> get_divine_judgment();
    return am;
  }

  proc_types proc_type() const override
  {
    return PROC1_MELEE_ABILITY;
  }
};

struct judgment_t : public paladin_melee_attack_t
{
  timespan_t sotr_cdr; // needed for sotr interaction for protection
  judgment_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "judgment", p, p -> find_spell( 20271 ), true )
  {
    parse_options( options_str );

    // no weapon multiplier
    weapon_multiplier = 0.0;
    may_block = may_parry = may_dodge = false;
    cooldown -> charges = 1;
    hasted_cd = true;

    // TODO: this is a hack; figure out what's really going on here.
    if ( p -> specialization() == PALADIN_RETRIBUTION )
    {
      base_costs[RESOURCE_MANA] = 0;
      base_multiplier *= 1.0 + p -> artifact.highlords_judgment.percent();
      impact_action = new judgment_aoe_t( p, options_str );
    }
    else if ( p -> specialization() == PALADIN_HOLY )
    {
      base_multiplier *= 1.0 + p -> passives.holy_paladin -> effectN( 6 ).percent();
    }
    else if ( p -> specialization() == PALADIN_PROTECTION )
    {
      cooldown -> charges *= 1.0 + p->talents.crusaders_judgment->effectN( 1 ).base_value();
      cooldown -> duration *= 1.0 + p -> passives.guarded_by_the_light -> effectN( 5 ).percent();
      base_multiplier *= 1.0 + p -> passives.protection_paladin -> effectN( 3 ).percent();
      sotr_cdr = -1.0 * timespan_t::from_seconds( 2 ); // hack for p -> spec.judgment_2 -> effectN( 1 ).base_value()
    }
  }

  virtual void execute() override
  {
    paladin_melee_attack_t::execute();

    if ( p() -> talents.fist_of_justice -> ok() )
    {
      double reduction = p() -> talents.fist_of_justice -> effectN( 1 ).base_value();
      p() -> cooldowns.hammer_of_justice -> ready -= timespan_t::from_seconds( reduction );
    }
  }

  proc_types proc_type() const override
  {
    return PROC1_MELEE_ABILITY;
  }

  double composite_target_crit_chance( player_t* t ) const override
  {
    double cc = paladin_melee_attack_t::composite_target_crit_chance( t );

    if ( p() -> talents.greater_judgment -> ok() )
    {
      double threshold = p() -> talents.greater_judgment -> effectN( 1 ).base_value();
      if ( ( t -> health_percentage() > threshold ) )
      {
        // TODO: is this correct? where does this come from?
        return 1.0;
      }
    }

    return cc;
  }

  double composite_crit_chance() const override
  {
    double cc = paladin_melee_attack_t::composite_crit_chance();

    // Stern Judgment increases crit chance by 10% - assume additive
    cc += p() -> artifact.stern_judgment.percent( 1 );

    return cc;
  }

  // Special things that happen when Judgment damages target
  void impact( action_state_t* s ) override
  {
    if ( result_is_hit( s -> result ) )
    {
      td( s -> target ) -> buffs.debuffs_judgment -> trigger();

      if ( p() -> talents.judgment_of_light -> ok() )
        td( s -> target ) -> buffs.judgment_of_light -> trigger( 40 );

      // Judgment hits/crits reduce SotR recharge time
      if ( p() -> specialization() == PALADIN_PROTECTION )
      {
        p() -> cooldowns.shield_of_the_righteous -> adjust( s -> result == RESULT_CRIT ? 2.0 * sotr_cdr : sotr_cdr );
      }
    }

    paladin_melee_attack_t::impact( s );
  }

  double action_multiplier() const override
  {
    double am = paladin_melee_attack_t::action_multiplier();
    // todo: refer to actual spelldata instead of magic constant
    am *= 1.0 + 2 * p() -> get_divine_judgment();
    return am;
  }
};

// Rebuke ===================================================================

struct rebuke_t : public paladin_melee_attack_t
{
  rebuke_t( paladin_t* p, const std::string& options_str ) :
    paladin_melee_attack_t( "rebuke", p, p -> find_class_spell( "Rebuke" ) )
  {
    parse_options( options_str );

    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
    ignore_false_positive = true;

    // no weapon multiplier
    weapon_multiplier = 0.0;
  }

  virtual bool ready() override
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return paladin_melee_attack_t::ready();
  }

  virtual void execute() override
  {
    paladin_melee_attack_t::execute();

    if ( p() -> sephuz )
    {
      p() -> buffs.sephuz -> trigger();
    }
  }
};

// Reckoning ==================================================================

struct reckoning_t: public paladin_melee_attack_t
{
  reckoning_t( paladin_t* p, const std::string& options_str ):
    paladin_melee_attack_t( "reckoning", p, p -> find_class_spell( "Reckoning" ) )
  {
    parse_options( options_str );
    use_off_gcd = true;
  }

  virtual void impact( action_state_t* s ) override
  {
    if ( s -> target -> is_enemy() )
      target -> taunt( player );

    paladin_melee_attack_t::impact( s );
  }
};

// Shield of the Righteous ==================================================

struct shield_of_the_righteous_t : public paladin_melee_attack_t
{
  shield_of_the_righteous_t( paladin_t* p, const std::string& options_str ) :
    paladin_melee_attack_t( "shield_of_the_righteous", p, p -> find_class_spell( "Shield of the Righteous" ) )
  {
    parse_options( options_str );

    if ( ! p -> has_shield_equipped() )
    {
      sim -> errorf( "%s: %s only usable with shield equipped in offhand\n", p -> name(), name() );
      background = true;
    }

    // not on GCD, usable off-GCD
    trigger_gcd = timespan_t::zero();
    use_off_gcd = true;

    // no weapon multiplier
    weapon_multiplier = 0.0;

    // link needed for Judgment cooldown reduction
    cooldown = p -> cooldowns.shield_of_the_righteous;
    p -> active_sotr = this;
  }

  double action_multiplier() const override
  {
    double am = paladin_melee_attack_t::action_multiplier();

    if ( p() -> standing_in_consecration() || p() -> talents.consecrated_hammer -> ok() )
      am *= 1.0 + p() -> spells.consecration_bonus -> effectN( 2 ).percent();

    am *= 1.0 + p() -> artifact.righteous_crusader.percent( 1 );

    return am;
  }


  virtual void execute() override
  {
    paladin_melee_attack_t::execute();

    //Buff granted regardless of combat roll result
    //Duration is additive on refresh, stats not recalculated
    if ( p() -> buffs.shield_of_the_righteous -> check() )
    {
      p() -> buffs.shield_of_the_righteous -> extend_duration( p(), p() -> buffs.shield_of_the_righteous -> buff_duration );
    }
    else
      p() -> buffs.shield_of_the_righteous -> trigger();
  }

  // Special things that happen when SotR damages target
  virtual void impact( action_state_t* s ) override
  {
    if ( result_is_hit( s -> result ) ) // TODO: not needed anymore? Can we even miss?
    {
      // SotR hits reduce Light of the Protector and Avenging Wrath cooldown times if Righteous Protector is talented
      if ( p() -> talents.righteous_protector -> ok() )
      {
        timespan_t reduction = timespan_t::from_seconds( -1.0 * p() -> talents.righteous_protector -> effectN( 1 ).base_value() );
        p() -> cooldowns.avenging_wrath -> adjust( reduction );
        p() -> cooldowns.light_of_the_protector -> adjust( reduction );
        p() -> cooldowns.hand_of_the_protector -> adjust( reduction );
      }
    }

    paladin_melee_attack_t::impact( s );
  }
};

// Templar's Verdict ========================================================================

struct echoed_templars_verdict_t : public paladin_melee_attack_t
{
  echoed_templars_verdict_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "echoed_verdict", p, p -> find_spell( 224266 ), true )
  {
    parse_options( options_str );

    base_multiplier *= p -> artifact.echo_of_the_highlord.percent();
    background = true;
    base_multiplier *= 1.0 + p -> artifact.might_of_the_templar.percent();
    if ( p -> talents.final_verdict -> ok() )
      base_multiplier *= 1.0 + p -> talents.final_verdict -> effectN( 1 ).percent();

    ret_damage_increase = true;
  }

  virtual double action_multiplier() const override
  {
    double am = paladin_melee_attack_t::action_multiplier();
    if ( p() -> buffs.whisper_of_the_nathrezim -> check() )
      am *= 1.0 + p() -> buffs.whisper_of_the_nathrezim -> data().effectN( 1 ).percent();
    return am;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = paladin_melee_attack_t::composite_target_multiplier( t );

    paladin_td_t* td = this -> td( t );

    if ( td -> buffs.debuffs_judgment -> up() )
    {
      double judgment_multiplier = 1.0 + td -> buffs.debuffs_judgment -> data().effectN( 1 ).percent() + p() -> get_divine_judgment();
      judgment_multiplier += p() -> passives.judgment -> effectN( 1 ).percent();
      m *= judgment_multiplier;
    }

    return m;
  }

  virtual double cost() const override
  {
    return 0;
  }
};

struct templars_verdict_t : public holy_power_consumer_t
{
  echoed_templars_verdict_t* echoed_spell;

  struct templars_verdict_damage_t : public paladin_melee_attack_t
  {
    templars_verdict_damage_t( paladin_t *p )
      : paladin_melee_attack_t( "templars_verdict_dmg", p, p -> find_spell( 224266 ) )
    {
      dual = background = true;
      may_miss = may_dodge = may_parry = false;
    }
  };

  templars_verdict_t( paladin_t* p, const std::string& options_str )
    : holy_power_consumer_t( "templars_verdict", p, p -> find_specialization_spell( "Templar's Verdict" ), true ),
    echoed_spell( new echoed_templars_verdict_t( p, options_str ) )
  {
    parse_options( options_str );

    hasted_gcd = true;

    may_block = false;
    impact_action = new templars_verdict_damage_t( p );
    impact_action -> stats = stats;

    base_multiplier *= 1.0 + p -> artifact.might_of_the_templar.percent();
    if ( p -> talents.final_verdict -> ok() )
      base_multiplier *= 1.0 + p -> talents.final_verdict -> effectN( 1 ).percent();

    ret_damage_increase = true;

  // Okay, when did this get reset to 1?
    weapon_multiplier = 0;
  }

  void record_data( action_state_t* ) override {}

  virtual double cost() const override
  {
    double base_cost = holy_power_consumer_t::cost();
    if ( p() -> buffs.the_fires_of_justice -> up() && base_cost > 0 )
      return base_cost - 1;
    return base_cost;
  }

  virtual double action_multiplier() const override
  {
    double am = holy_power_consumer_t::action_multiplier();
    if ( p() -> buffs.whisper_of_the_nathrezim -> check() )
      am *= 1.0 + p() -> buffs.whisper_of_the_nathrezim -> data().effectN( 1 ).percent();
    return am;
  }

  virtual void execute() override
  {
    // store cost for potential refunding (see below)
    double c = cost();

    holy_power_consumer_t::execute();

    // TODO: do misses consume fires of justice?
    if ( p() -> buffs.the_fires_of_justice -> up() && c > 0 )
      p() -> buffs.the_fires_of_justice -> expire();

    // missed/dodged/parried TVs do not consume Holy Power
    // check for a miss, and refund the appropriate amount of HP if we spent any
    if ( result_is_miss( execute_state -> result ) && c > 0 )
    {
      p() -> resource_gain( RESOURCE_HOLY_POWER, c, p() -> gains.hp_templars_verdict_refund );
    }

    if ( p() -> artifact.echo_of_the_highlord.rank() )
    {
      make_event<echoed_spell_event_t>( *sim, p(), echoed_spell, timespan_t::from_millis( 800 ) );
    }

    if ( p() -> whisper_of_the_nathrezim )
    {
      if ( p() -> buffs.whisper_of_the_nathrezim -> up() )
        p() -> buffs.whisper_of_the_nathrezim -> expire();
      make_event<whisper_of_the_nathrezim_event_t>( *sim, p(), timespan_t::from_millis( 300 ) );
    }
  }
};

// Justicar's Vengeance
struct justicars_vengeance_t : public holy_power_consumer_t
{
  justicars_vengeance_t( paladin_t* p, const std::string& options_str )
    : holy_power_consumer_t( "justicars_vengeance", p, p -> talents.justicars_vengeance, true )
  {
    parse_options( options_str );

    hasted_gcd = true;

    weapon_multiplier = 0; // why is this needed?
  }

  virtual double cost() const override
  {
    double base_cost = holy_power_consumer_t::cost();
    if ( p() -> buffs.the_fires_of_justice -> up() && base_cost > 0 )
      return base_cost - 1;
    return base_cost;
  }

  virtual void execute() override
  {
    // store cost for potential refunding (see below)
    double c = cost();

    holy_power_consumer_t::execute();

    // TODO: do misses consume fires of justice?
    if ( p() -> buffs.the_fires_of_justice -> up() && c > 0 )
      p() -> buffs.the_fires_of_justice -> expire();

    // missed/dodged/parried TVs do not consume Holy Power
    // check for a miss, and refund the appropriate amount of HP if we spent any
    if ( result_is_miss( execute_state -> result ) && c > 0 )
    {
      p() -> resource_gain( RESOURCE_HOLY_POWER, c, p() -> gains.hp_templars_verdict_refund );
    }
  }
};

// Seal of Light ========================================================================
struct seal_of_light_t : public paladin_spell_t
{
  seal_of_light_t( paladin_t* p, const std::string& options_str  )
    : paladin_spell_t( "seal_of_light", p, p -> find_talent_spell( "Seal of Light" ) )
  {
    parse_options( options_str );

    may_miss = false;
  }

  virtual double cost() const override
  {
    double base_cost = std::max( base_costs[ RESOURCE_HOLY_POWER ], p() -> resources.current[ RESOURCE_HOLY_POWER ] );
    if ( p() -> buffs.the_fires_of_justice -> up() && base_cost > 0 )
      return base_cost - 1;
    return base_cost;
  }

  virtual void execute() override
  {
    double c = cost();
    paladin_spell_t::execute();

    // TODO: verify this
    if ( p() -> buffs.the_fires_of_justice -> up() )
    {
      p() -> buffs.the_fires_of_justice -> expire();
      c += 1.0;
    }

    p() -> buffs.seal_of_light -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0,
      c * ( p() -> buffs.seal_of_light -> data().duration() ) );
  }
};

// ==========================================================================
// End Attacks
// ==========================================================================

// ==========================================================================
// Paladin Buffs, Part Deux
// ==========================================================================
// Certain buffs need to be defined  after actions, because they call methods
// found in action_t definitions.

namespace buffs {

struct blessing_of_sacrifice_t : public buff_t
{
  paladin_t* source; // Assumption: Only one paladin can cast HoS per target
  double source_health_pool;

  blessing_of_sacrifice_t( player_t* p ) :
    buff_t( buff_creator_t( p, "blessing_of_sacrifice", p -> find_spell( 6940 ) ) ),
    source( nullptr ),
    source_health_pool( 0.0 )
  {

  }

  // Trigger function for the paladin applying HoS on the target
  bool trigger_hos( paladin_t& source )
  {
    if ( this -> source )
      return false;

    this -> source = &source;
    source_health_pool = source.resources.max[ RESOURCE_HEALTH ];

    return buff_t::trigger( 1 );
  }

  // Misuse functions as the redirect callback for damage onto the source
  virtual bool trigger( int, double value, double, timespan_t ) override
  {
    assert( source );

    value = std::min( source_health_pool, value );
    source -> active.blessing_of_sacrifice_redirect -> trigger( value );
    source_health_pool -= value;

    // If the health pool is fully consumed, expire the buff early
    if ( source_health_pool <= 0 )
    {
      expire();
    }

    return true;
  }

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    source = nullptr;
    source_health_pool = 0.0;
  }

  virtual void reset() override
  {
    buff_t::reset();

    source = nullptr;
    source_health_pool = 0.0;
  }
};

// Divine Protection buff
struct divine_protection_t : public buff_t
{

  divine_protection_t( paladin_t* p ) :
    buff_t( buff_creator_t( p, "divine_protection", p -> find_class_spell( "Divine Protection" ) ) )
  {
    cooldown -> duration = timespan_t::zero();
  }

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );
  }

};

} // end namespace buffs
// ==========================================================================
// End Paladin Buffs, Part Deux
// ==========================================================================

// Blessing of Sacrifice execute function

void blessing_of_sacrifice_t::execute()
{
  paladin_spell_t::execute();

  buffs::blessing_of_sacrifice_t* b = debug_cast<buffs::blessing_of_sacrifice_t*>( target -> buffs.blessing_of_sacrifice );

  b -> trigger_hos( *p() );
}

// ==========================================================================
// Paladin Character Definition
// ==========================================================================

paladin_td_t::paladin_td_t( player_t* target, paladin_t* paladin ) :
  actor_target_data_t( target, paladin )
{
  dots.execution_sentence = target -> get_dot( "execution_sentence", paladin );
  dots.wake_of_ashes = target -> get_dot( "wake_of_ashes", paladin );
  buffs.debuffs_judgment = buff_creator_t( *this, "judgment", paladin -> find_spell( 197277 ));
  buffs.judgment_of_light = buff_creator_t( *this, "judgment_of_light", paladin -> find_spell( 196941 ) );
  buffs.eye_of_tyr_debuff = buff_creator_t( *this, "eye_of_tyr", paladin -> find_class_spell( "Eye of Tyr" ) ).cd( timespan_t::zero() );
  buffs.forbearant_faithful = new buffs::forbearance_t( paladin, this, "forbearant_faithful" );
  buffs.blessed_hammer_debuff = buff_creator_t( *this, "blessed_hammer", paladin -> find_spell( 204301 ) );
}

// paladin_t::create_action =================================================

action_t* paladin_t::create_action( const std::string& name, const std::string& options_str )
{
  if ( name == "auto_attack"               ) return new auto_melee_attack_t        ( this, options_str );
  if ( name == "aegis_of_light"            ) return new aegis_of_light_t           ( this, options_str );
  if ( name == "ardent_defender"           ) return new ardent_defender_t          ( this, options_str );
  if ( name == "avengers_shield"           ) return new avengers_shield_t          ( this, options_str );
  if ( name == "avenging_wrath"            ) return new avenging_wrath_t           ( this, options_str );
  if ( name == "crusade"                   ) return new crusade_t                  ( this, options_str );
  if ( name == "bastion_of_light"          ) return new bastion_of_light_t         ( this, options_str );
  if ( name == "blessed_hammer"            ) return new blessed_hammer_t           ( this, options_str );
  if ( name == "blessing_of_protection"    ) return new blessing_of_protection_t   ( this, options_str );
  if ( name == "blessing_of_spellwarding"  ) return new blessing_of_spellwarding_t ( this, options_str );
  if ( name == "blinding_light"            ) return new blinding_light_t           ( this, options_str );
  if ( name == "beacon_of_light"           ) return new beacon_of_light_t          ( this, options_str );
  if ( name == "consecration"              ) return new consecration_t             ( this, options_str );
  if ( name == "crusader_strike"           ) return new crusader_strike_t          ( this, options_str );
  if ( name == "zeal"                      ) return new zeal_t                     ( this, options_str );
  if ( name == "blade_of_justice"          ) return new blade_of_justice_t         ( this, options_str );
  if ( name == "divine_hammer"             ) return new divine_hammer_t            ( this, options_str );
  if ( name == "denounce"                  ) return new denounce_t                 ( this, options_str );
  if ( name == "divine_protection"         ) return new divine_protection_t        ( this, options_str );
  if ( name == "divine_steed"              ) return new divine_steed_t             ( this, options_str );
  if ( name == "divine_shield"             ) return new divine_shield_t            ( this, options_str );
  if ( name == "divine_storm"              ) return new divine_storm_t             ( this, options_str );
  if ( name == "execution_sentence"        ) return new execution_sentence_t       ( this, options_str );
  if ( name == "blessing_of_sacrifice"     ) return new blessing_of_sacrifice_t    ( this, options_str );
  if ( name == "hammer_of_justice"         ) return new hammer_of_justice_t        ( this, options_str );
  if ( name == "hammer_of_the_righteous"   ) return new hammer_of_the_righteous_t  ( this, options_str );
  if ( name == "holy_shock"                ) return new holy_shock_t               ( this, options_str );
  if ( name == "guardian_of_ancient_kings" ) return new guardian_of_ancient_kings_t( this, options_str );
  if ( name == "judgment"                  ) return new judgment_t                 ( this, options_str );
  if ( name == "light_of_the_protector"    ) return new light_of_the_protector_t   ( this, options_str );
  if ( name == "hand_of_the_protector"     ) return new hand_of_the_protector_t    ( this, options_str );
  if ( name == "light_of_dawn"             ) return new light_of_dawn_t            ( this, options_str );
  if ( name == "lights_hammer"             ) return new lights_hammer_t            ( this, options_str );
  if ( name == "rebuke"                    ) return new rebuke_t                   ( this, options_str );
  if ( name == "reckoning"                 ) return new reckoning_t                ( this, options_str );
  if ( name == "shield_of_the_righteous"   ) return new shield_of_the_righteous_t  ( this, options_str );
  if ( name == "templars_verdict"          ) return new templars_verdict_t         ( this, options_str );
  if ( name == "holy_wrath"                ) return new holy_wrath_t               ( this, options_str );
  if ( name == "holy_prism"                ) return new holy_prism_t               ( this, options_str );
  if ( name == "wake_of_ashes"             ) return new wake_of_ashes_t            ( this, options_str );
  if ( name == "eye_of_tyr"                ) return new eye_of_tyr_t               ( this, options_str );
  if ( name == "seraphim"                  ) return new seraphim_t                 ( this, options_str );
  if ( name == "seal_of_light"             ) return new seal_of_light_t            ( this, options_str );
  if ( name == "holy_light"                ) return new holy_light_t               ( this, options_str );
  if ( name == "flash_of_light"            ) return new flash_of_light_t           ( this, options_str );
  if ( name == "lay_on_hands"              ) return new lay_on_hands_t             ( this, options_str );
  if ( name == "justicars_vengeance"       ) return new justicars_vengeance_t      ( this, options_str );
  if ( name == "shield_of_vengeance"       ) return new shield_of_vengeance_t      ( this, options_str );

  return player_t::create_action( name, options_str );
}

void paladin_t::trigger_grand_crusader()
{
  // escape if we don't have Grand Crusader
  if ( ! passives.grand_crusader -> ok() )
    return;

  // attempt to proc the buff, returns true if successful
  if ( buffs.grand_crusader -> trigger() )
  {
    // reset AS cooldown
    cooldowns.avengers_shield -> reset( true );

  if (talents.crusaders_judgment -> ok() && cooldowns.judgment ->current_charge < cooldowns.judgment->charges)
  {
    cooldowns.judgment->adjust(-cooldowns.judgment->duration); //decrease remaining time by the duration of one charge, i.e., add one charge
  }

  }
}

void paladin_t::trigger_holy_shield( action_state_t* s )
{
  // escape if we don't have Holy Shield
  if ( ! talents.holy_shield -> ok() )
    return;

  // sanity check - no friendly-fire
  if ( ! s -> action -> player -> is_enemy() )
    return;

  // Check for proc
  if ( rng().roll( talents.holy_shield -> proc_chance() ) )
  {
    active_holy_shield_proc -> target = s -> action -> player;
    active_holy_shield_proc -> schedule_execute();
  }
}

void paladin_t::trigger_painful_truths( action_state_t* s )
{
  // escape if we don't have artifact
  if ( artifact.painful_truths.rank() == 0 )
    return;

  // sanity check - no friendly-fire
  if ( ! s -> action -> player -> is_enemy() )
    return;

  if ( ! buffs.painful_truths -> up() )
    return;

  active_painful_truths_proc -> target = s -> action -> player;
  active_painful_truths_proc -> schedule_execute();
}

void paladin_t::trigger_tyrs_enforcer( action_state_t* s )
{
  // escape if we don't have artifact
  if ( artifact.tyrs_enforcer.rank() == 0 )
    return;

  // sanity check - no friendly-fire
  if ( ! s -> target -> is_enemy() )
    return;

  active_tyrs_enforcer_proc -> target = s -> target;
  active_tyrs_enforcer_proc -> schedule_execute();
}

int paladin_t::get_local_enemies( double distance ) const
{
  int num_nearby = 0;
  for ( size_t i = 0, actors = sim -> target_non_sleeping_list.size(); i < actors; i++ )
  {
    player_t* p = sim -> target_non_sleeping_list[ i ];
    if ( p -> is_enemy() && get_player_distance( *p ) <= distance + p -> combat_reach )
      num_nearby += 1;
  }
  return num_nearby;
}

bool paladin_t::standing_in_consecration() const
{
  // new
  for ( size_t i = 0; i < active_consecrations.size(); i++ )
  {
    // calculate current distance to each consecration
    paladin_ground_aoe_t* cons_to_test = active_consecrations[ i ];

    double distance = get_position_distance( cons_to_test -> params -> x(), cons_to_test -> params -> y() );

    // exit with true if we're in range of any one Cons center
    if ( distance <= cons_to_test -> radius )
      return true;
  }
  // if we're not in range of any of them
  return false;
}

double paladin_t::get_forbearant_faithful_recharge_multiplier() const
{
  double m = 1.0;

  if ( artifact.forbearant_faithful.rank() )
  {
    int num_buffs = 0;

    // loop over player list to check those
    for ( size_t i = 0; i < sim -> player_no_pet_list.size(); i++ )
    {
      player_t* q = sim -> player_no_pet_list[ i ];

      if ( get_target_data( q ) -> buffs.forbearant_faithful -> check() )
        num_buffs++;
    }

    // Forbearant Faithful is not... faithful... to its tooltip.
    // Testing in-game reveals that the cooldown multiplier is ( 1.0 * num_buffs * 0.25 ) - tested 6/20/2016
    // It looks like we're getting half of the described effect
    m -= num_buffs * artifact.forbearant_faithful.percent( 2 ) / 2;
  }

  return m;

}

void paladin_t::update_forbearance_recharge_multipliers() const
{
  range::for_each( forbearant_faithful_cooldowns, []( cooldown_t* cd ) { cd -> adjust_recharge_multiplier(); } );
}

// paladin_t::init_base =====================================================

void paladin_t::init_base_stats()
{
  if ( base.distance < 1 )
  {
    base.distance = 5;
    // move holy paladins to range
    if ( specialization() == PALADIN_HOLY && primary_role() == ROLE_HEAL )
      base.distance = 30;
  }

  player_t::init_base_stats();

  base.attack_power_per_agility = 0.0;
  base.attack_power_per_strength = 1.0;
  base.spell_power_per_intellect = 1.0;

  // Boundless Conviction raises max holy power to 5
  resources.base[ RESOURCE_HOLY_POWER ] = 3 + passives.boundless_conviction -> effectN( 1 ).base_value();

  // Avoidance diminishing Returns constants/conversions now handled in player_t::init_base_stats().
  // Base miss, dodge, parry, and block are set in player_t::init_base_stats().
  // Just need to add class- or spec-based modifiers here.

  // add Sanctuary dodge
  base.dodge += passives.sanctuary -> effectN( 3 ).percent();
  // add Sanctuary expertise
  base.expertise += passives.sanctuary -> effectN( 4 ).percent();

  // Resolve of Truth max HP multiplier
  resources.initial_multiplier[ RESOURCE_HEALTH ] *= 1.0 + artifact.resolve_of_truth.percent( 1 );

  // Holy Insight grants mana regen from spirit during combat
  base.mana_regen_from_spirit_multiplier = passives.holy_insight -> effectN( 3 ).percent();

  // Holy Insight increases max mana for Holy
  resources.base_multiplier[ RESOURCE_MANA ] = 1.0 + passives.holy_insight -> effectN( 1 ).percent();
}

// paladin_t::reset =========================================================

void paladin_t::reset()
{
  player_t::reset();

  active_consecrations.clear();
  last_retribution_trinket_target = nullptr;
  last_extra_regen = timespan_t::zero();
  last_jol_proc = timespan_t::zero();
}

// paladin_t::init_gains ====================================================

void paladin_t::init_gains()
{
  player_t::init_gains();

  // Mana
  gains.extra_regen                 = get_gain( ( specialization() == PALADIN_RETRIBUTION ) ? "sword_of_light" : "guarded_by_the_light" );
  gains.mana_beacon_of_light        = get_gain( "beacon_of_light" );

  // Health
  gains.holy_shield                 = get_gain( "holy_shield_absorb" );
  gains.bulwark_of_order            = get_gain( "bulwark_of_order" );

  // Holy Power
  gains.hp_templars_verdict_refund  = get_gain( "templars_verdict_refund" );
  gains.hp_liadrins_fury_unleashed  = get_gain( "liadrins_fury_unleashed" );
  gains.judgment                    = get_gain( "judgment" );
  gains.hp_t19_4p                   = get_gain( "t19_4p" );
  gains.hp_t20_2p                   = get_gain( "t20_2p" );
  gains.hp_justice_gaze             = get_gain( "justice_gaze" );

  if ( ! retribution_trinket )
  {
    buffs.retribution_trinket = buff_creator_t( this, "focus_of_vengeance" )
      .chance( 0 );
  }
}

// paladin_t::init_procs ====================================================

void paladin_t::init_procs()
{
  player_t::init_procs();

  procs.eternal_glory             = get_proc( "eternal_glory"                  );
  procs.focus_of_vengeance_reset  = get_proc( "focus_of_vengeance_reset"       );
  procs.divine_purpose            = get_proc( "divine_purpose"                 );
  procs.the_fires_of_justice      = get_proc( "the_fires_of_justice"           );
  procs.tfoj_set_bonus            = get_proc( "t19_4p"                         );
  procs.blade_of_wrath            = get_proc( "blade_of_wrath"                 );
}

// paladin_t::init_scaling ==================================================

void paladin_t::init_scaling()
{
  player_t::init_scaling();

  specialization_e tree = specialization();

  // Only Holy cares about INT/SPI/SP.
  scales_with[ STAT_INTELLECT   ] = ( tree == PALADIN_HOLY );
  scales_with[ STAT_SPELL_POWER ] = ( tree == PALADIN_HOLY );
  scales_with[ STAT_BONUS_ARMOR    ] = ( tree == PALADIN_PROTECTION );

  scales_with[STAT_AGILITY] = false;
}

// paladin_t::init_buffs ====================================================

void paladin_t::create_buffs()
{
  player_t::create_buffs();

  // Talents
  buffs.holy_shield_absorb     = absorb_buff_creator_t( this, "holy_shield", find_spell( 157122 ) )
                                 .school( SCHOOL_MAGIC )
                                 .source( get_stats( "holy_shield_absorb" ) )
                                 .gain( get_gain( "holy_shield_absorb" ) );

  // General
  buffs.avenging_wrath         = new buffs::avenging_wrath_buff_t( this );
  buffs.crusade                = new buffs::crusade_buff_t( this );
  buffs.sephuz                 = new buffs::sephuzs_secret_buff_t( this );
  buffs.divine_protection      = new buffs::divine_protection_t( this );
  buffs.divine_shield          = buff_creator_t( this, "divine_shield", find_class_spell( "Divine Shield" ) )
                                 .cd( timespan_t::zero() ) // Let the ability handle the CD
                                 .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  // Holy
  buffs.infusion_of_light      = buff_creator_t( this, "infusion_of_light", find_spell( 54149 ) );

  // Prot
  buffs.guardian_of_ancient_kings      = buff_creator_t( this, "guardian_of_ancient_kings", find_specialization_spell( "Guardian of Ancient Kings" ) )
                                          .cd( timespan_t::zero() ); // let the ability handle the CD
  buffs.grand_crusader                 = buff_creator_t( this, "grand_crusader", passives.grand_crusader -> effectN( 1 ).trigger() )
                                          .chance( passives.grand_crusader -> proc_chance() + ( 0.0 + talents.first_avenger -> effectN( 2 ).percent() ) );
  buffs.shield_of_the_righteous        = buff_creator_t( this, "shield_of_the_righteous", find_spell( 132403 ) );
  buffs.ardent_defender                = new buffs::ardent_defender_buff_t( this );
  buffs.painful_truths                 = buff_creator_t( this, "painful_truths", find_spell( 209332 ) );
  buffs.aegis_of_light                 = buff_creator_t( this, "aegis_of_light", find_talent_spell( "Aegis of Light" ) );
  buffs.seraphim                       = stat_buff_creator_t( this, "seraphim", talents.seraphim )
                                          .add_stat( STAT_HASTE_RATING, talents.seraphim -> effectN( 1 ).average( this ) )
                                          .add_stat( STAT_CRIT_RATING, talents.seraphim -> effectN( 1 ).average( this ) )
                                          .add_stat( STAT_MASTERY_RATING, talents.seraphim -> effectN( 1 ).average( this ) )
                                          .add_stat( STAT_VERSATILITY_RATING, talents.seraphim -> effectN( 1 ).average( this ) )
                                          .cd( timespan_t::zero() ); // let the ability handle the cooldown
  buffs.bulwark_of_order               = absorb_buff_creator_t( this, "bulwark_of_order", find_spell( 209388 ) )
                                         .source( get_stats( "bulwark_of_order" ) )
                                         .gain( get_gain( "bulwark_of_order" ) )
                                         .max_stack( 1 ); // not sure why data says 3 stacks

  // Ret
  buffs.zeal                           = buff_creator_t( this, "zeal", find_spell( 217020 ) );
  buffs.seal_of_light                  = buff_creator_t( this, "seal_of_light", find_spell( 202273 ) );
  buffs.the_fires_of_justice           = buff_creator_t( this, "the_fires_of_justice", find_spell( 209785 ) );
  buffs.blade_of_wrath               = buff_creator_t( this, "blade_of_wrath", find_spell( 231843 ) );
  buffs.divine_purpose                 = buff_creator_t( this, "divine_purpose", find_spell( 223819 ) );
  buffs.divine_steed                   = buff_creator_t( this, "divine_steed", find_spell( "Divine Steed" ) )
                                          .duration( timespan_t::from_seconds( 3.0 ) ).chance( 1.0 ).default_value( 1.0 ); // TODO: change this to spellid 221883 & see if that automatically captures details
  buffs.whisper_of_the_nathrezim       = buff_creator_t( this, "whisper_of_the_nathrezim", find_spell( 207635 ) );
  buffs.liadrins_fury_unleashed        = new buffs::liadrins_fury_unleashed_t( this );
  buffs.shield_of_vengeance            = new buffs::shield_of_vengeance_buff_t( this );
  buffs.righteous_verdict              = buff_creator_t( this, "righteous_verdict", find_spell( 238996 ) );

  // Tier Bonuses

  buffs.vindicators_fury       = buff_creator_t( this, "vindicators_fury", find_spell( 165903 ) )
                                 .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                                 .add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER )
                                 .chance( sets.has_set_bonus( PALADIN_RETRIBUTION, PVP, B4 ) )
                                 .duration( timespan_t::from_seconds( 4 ) );
  buffs.wings_of_liberty       = buff_creator_t( this, "wings_of_liberty", find_spell( 185655 ) -> effectN( 1 ).trigger() )
                                 .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                                 .default_value( find_spell( 185655 ) -> effectN( 1 ).trigger() -> effectN( 1 ).percent() )
                                 .chance( sets.has_set_bonus( PALADIN_RETRIBUTION, T18, B4 ) );
  buffs.wings_of_liberty_driver = new buffs::wings_of_liberty_driver_t( this );
}

// paladin_t::has_t18_class_trinket ==============================================

bool paladin_t::has_t18_class_trinket() const
{
  if ( specialization() == PALADIN_RETRIBUTION )
  {
    return retribution_trinket != nullptr;
  }
  return false;
}

// ==========================================================================
// Action Priority List Generation - Protection
// ==========================================================================
void paladin_t::generate_action_prio_list_prot()
{
  ///////////////////////
  // Precombat List
  ///////////////////////

  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  //Flask
  if ( sim -> allow_flasks )
  {
    if (true_level > 100)
    {
      precombat->add_action("flask,type=flask_of_ten_thousand_scars");
      precombat->add_action("flask,type=flask_of_the_countless_armies,if=role.attack|using_apl.max_dps");
    }
    else if ( true_level > 90 )
    {
      precombat -> add_action( "flask,type=greater_draenic_stamina_flask" );
      precombat -> add_action( "flask,type=greater_draenic_strength_flask,if=role.attack|using_apl.max_dps" );
    }
    else if ( true_level > 85 )
      precombat -> add_action( "flask,type=earth" );
    else if ( true_level >= 80 )
      precombat -> add_action( "flask,type=steelskin" );
  }

  // Food
  if ( sim -> allow_food )
  {
    if (level() > 100)
    {
      precombat->add_action("food,type=seedbattered_fish_plate");
      precombat->add_action("food,type=azshari_salad,if=role.attack|using_apl.max_dps");
    }
    else if ( level() > 90 )
    {
      precombat -> add_action( "food,type=whiptail_fillet" );
      precombat -> add_action( "food,type=pickled_eel,if=role.attack|using_apl.max_dps" );
    }
    else if ( level() > 85 )
      precombat -> add_action( "food,type=chun_tian_spring_rolls" );
    else if ( level() >= 80 )
      precombat -> add_action( "food,type=seafood_magnifique_feast" );
  }

  // Snapshot stats
  precombat -> add_action( "snapshot_stats",  "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  // Pre-potting
  std::string potion_type = "";
  if ( sim -> allow_potions )
  {
    // no need for off/def pot options - Draenic Armor gives more AP than Draenic STR,
    // and Mountains potion is pathetic at L90
    if (true_level > 100)
      potion_type = "unbending_potion";
    else if ( true_level > 90 )
      potion_type = "draenic_strength";
    else if ( true_level >= 80 )
      potion_type = "mogu_power";

    if ( potion_type.length() > 0 )
      precombat -> add_action( "potion,name=" + potion_type );
  }

  ///////////////////////
  // Action Priority List
  ///////////////////////

  action_priority_list_t* def = get_action_priority_list("default");
  action_priority_list_t* prot = get_action_priority_list("prot");
  //action_priority_list_t* prot_aoe = get_action_priority_list("prot_aoe");
  action_priority_list_t* dps = get_action_priority_list("max_dps");
  action_priority_list_t* surv = get_action_priority_list("max_survival");

  dps->action_list_comment_str = "This is a high-DPS (but low-survivability) configuration.\n# Invoke by adding \"actions+=/run_action_list,name=max_dps\" to the beginning of the default APL.";
  surv->action_list_comment_str = "This is a high-survivability (but low-DPS) configuration.\n# Invoke by adding \"actions+=/run_action_list,name=max_survival\" to the beginning of the default APL.";

  def->add_action("auto_attack");

  // usable items
  int num_items = (int)items.size();
  for (int i = 0; i < num_items; i++)
    if (items[i].has_special_effect(SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE))
      def->add_action("use_item,name=" + items[i].name_str);

  // profession actions
  std::vector<std::string> profession_actions = get_profession_actions();
  for (size_t i = 0; i < profession_actions.size(); i++)
    def->add_action(profession_actions[i]);

  // racial actions
  std::vector<std::string> racial_actions = get_racial_actions();
  for (size_t i = 0; i < racial_actions.size(); i++)
    def->add_action(racial_actions[i]);

  // clone all of the above to the other two action lists
  dps->action_list = def->action_list;
  surv->action_list = def->action_list;

  // TODO: Create this.

  //threshold for defensive abilities
  std::string threshold = "incoming_damage_2500ms>health.max*0.4";
  std::string threshold_lotp = "incoming_damage_13000ms<health.max*1.6";
  std::string threshold_lotp_rp = "incoming_damage_10000ms<health.max*1.25";
  std::string threshold_hotp = "incoming_damage_9000ms<health.max*1.2";
  std::string threshold_hotp_rp = "incoming_damage_6000ms<health.max*0.7";

  for (size_t i = 0; i < racial_actions.size(); i++)
    def->add_action(racial_actions[i]);
  def->add_action("call_action_list,name=prot");

  //defensive
  prot->add_talent(this, "Seraphim", "if=talent.seraphim.enabled&action.shield_of_the_righteous.charges>=2");
  prot->add_action(this, "Shield of the Righteous", "if=(!talent.seraphim.enabled|action.shield_of_the_righteous.charges>2)&!(debuff.eye_of_tyr.up&buff.aegis_of_light.up&buff.ardent_defender.up&buff.guardian_of_ancient_kings.up&buff.divine_shield.up&buff.potion.up)");
  prot->add_action(this, "Shield of the Righteous", "if=(talent.bastion_of_light.enabled&talent.seraphim.enabled&buff.seraphim.up&cooldown.bastion_of_light.up)&!(debuff.eye_of_tyr.up&buff.aegis_of_light.up&buff.ardent_defender.up&buff.guardian_of_ancient_kings.up&buff.divine_shield.up&buff.potion.up)");
  prot->add_action(this, "Shield of the Righteous", "if=(talent.bastion_of_light.enabled&!talent.seraphim.enabled&cooldown.bastion_of_light.up)&!(debuff.eye_of_tyr.up&buff.aegis_of_light.up&buff.ardent_defender.up&buff.guardian_of_ancient_kings.up&buff.divine_shield.up&buff.potion.up)");
  prot->add_talent(this, "Bastion of Light", "if=talent.bastion_of_light.enabled&action.shield_of_the_righteous.charges<1");
  prot->add_action(this, "Light of the Protector", "if=(health.pct<40)");
  prot->add_talent(this, "Hand of the Protector",  "if=(health.pct<40)");
  prot->add_action(this, "Light of the Protector", "if=("+threshold_lotp_rp+")&health.pct<55&talent.righteous_protector.enabled");
  prot->add_action(this, "Light of the Protector", "if=("+threshold_lotp+")&health.pct<55");
  prot->add_talent(this, "Hand of the Protector",  "if=("+threshold_hotp_rp+")&health.pct<65&talent.righteous_protector.enabled");
  prot->add_talent(this, "Hand of the Protector",  "if=("+threshold_hotp+")&health.pct<55");
  prot->add_action(this, "Divine Steed", "if=talent.knight_templar.enabled&" + threshold + "&!(debuff.eye_of_tyr.up|buff.aegis_of_light.up|buff.ardent_defender.up|buff.guardian_of_ancient_kings.up|buff.divine_shield.up|buff.potion.up)");
  prot->add_action(this, "Eye of Tyr", "if=" + threshold + "&!(debuff.eye_of_tyr.up|buff.aegis_of_light.up|buff.ardent_defender.up|buff.guardian_of_ancient_kings.up|buff.divine_shield.up|buff.potion.up)");
  prot->add_talent(this, "Aegis of Light", "if=" + threshold + "&!(debuff.eye_of_tyr.up|buff.aegis_of_light.up|buff.ardent_defender.up|buff.guardian_of_ancient_kings.up|buff.divine_shield.up|buff.potion.up)");
  prot->add_action(this, "Guardian of Ancient Kings", "if=" + threshold + "&!(debuff.eye_of_tyr.up|buff.aegis_of_light.up|buff.ardent_defender.up|buff.guardian_of_ancient_kings.up|buff.divine_shield.up|buff.potion.up)");
  prot->add_action(this, "Divine Shield", "if=talent.final_stand.enabled&" + threshold + "&!(debuff.eye_of_tyr.up|buff.aegis_of_light.up|buff.ardent_defender.up|buff.guardian_of_ancient_kings.up|buff.divine_shield.up|buff.potion.up)");
  prot->add_action(this, "Ardent Defender", "if=" + threshold + "&!(debuff.eye_of_tyr.up|buff.aegis_of_light.up|buff.ardent_defender.up|buff.guardian_of_ancient_kings.up|buff.divine_shield.up|buff.potion.up)");
  prot->add_action(this, "Lay on Hands", "if=health.pct<15");

  //potion
  if (sim->allow_potions)
  {
    if (level() > 100)
    {
      //prot->add_action("potion,name=the_old_war,if=role.attack|using_apl.max_dps");
      prot->add_action("potion,name=unbending_potion");
    }
    if (true_level > 90)
    {
      prot->add_action("potion,name=draenic_strength,if=" + threshold + "&&!(debuff.eye_of_tyr.up|buff.aegis_of_light.up|buff.ardent_defender.up|buff.guardian_of_ancient_kings.up|buff.divine_shield.up|buff.potion.up)|target.time_to_die<=25");
    }
    else if (true_level >= 80)
    {
      prot->add_action("potion,name=mountains,if=" + threshold + "&!(debuff.eye_of_tyr.up|buff.aegis_of_light.up|buff.ardent_defender.up|buff.guardian_of_ancient_kings.up|buff.divine_shield.up|buff.potion.up)|target.time_to_die<=25");
    }
  }

  //stoneform
  prot->add_action("stoneform,if=" + threshold + "&!(debuff.eye_of_tyr.up|buff.aegis_of_light.up|buff.ardent_defender.up|buff.guardian_of_ancient_kings.up|buff.divine_shield.up|buff.potion.up)");

  //dps-single-target
  prot->add_action(this, "Avenging Wrath", "if=!talent.seraphim.enabled");
  prot->add_action(this, "Avenging Wrath", "if=talent.seraphim.enabled&buff.seraphim.up");
  //prot -> add_action( "call_action_list,name=prot_aoe,if=spell_targets.avenger_shield>3" );
  prot->add_action(this, "Judgment");
  prot->add_action(this, "Avenger's Shield","if=talent.crusaders_judgment.enabled&buff.grand_crusader.up");
  prot->add_talent(this, "Blessed Hammer");
  prot->add_action(this, "Avenger's Shield");
  prot->add_action(this, "Consecration" );
  prot->add_talent(this, "Blinding Light");
  prot->add_action(this, "Hammer of the Righteous");



  //dps-aoe
  //prot_aoe->add_action(this, "Avenger's Shield");
  //prot_aoe->add_talent(this, "Blessed Hammer");
  //prot_aoe->add_action(this, "Judgment");
  //prot_aoe->add_action(this, "Consecration");
  //prot_aoe->add_action(this, "Hammer of the Righteous");
}



// ==========================================================================
// Action Priority List Generation - Retribution
// ==========================================================================

void paladin_t::generate_action_prio_list_ret()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  //Flask
  if ( sim -> allow_flasks && true_level >= 80 )
  {
    std::string flask_action = "flask,type=";
    if ( true_level > 100 )
      flask_action += "flask_of_the_countless_armies";
    else if ( true_level > 90 )
      flask_action += "greater_draenic_strength_flask";
    else if ( true_level > 85 )
      flask_action += "winters_bite";
    else
      flask_action += "titanic_strength";

    precombat -> add_action( flask_action );
  }

  // Food
  if ( sim -> allow_food && level() >= 80 )
  {
    std::string food_action = "food,type=";
    if ( level() > 100 )
      food_action += "azshari_salad";
    else if ( level() > 90 )
      food_action += "sleeper_sushi";
    else
      food_action += ( level() > 85 ) ? "black_pepper_ribs_and_shrimp" : "beer_basted_crocolisk";

    precombat -> add_action( food_action );
  }

  if ( true_level > 100 )
    precombat -> add_action( "augmentation,type=defiled" );

  // Snapshot stats
  precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  // Pre-potting
  if ( sim -> allow_potions && true_level >= 80 )
  {
    if ( true_level > 100 )
      precombat -> add_action( "potion,name=old_war" );
    else if ( true_level > 90 )
      precombat -> add_action( "potion,name=draenic_strength" );
    else
      precombat -> add_action( ( true_level > 85 ) ? "potion,name=mogu_power" : "potion,name=golemblood" );
  }

  ///////////////////////
  // Action Priority List
  ///////////////////////

  action_priority_list_t* def = get_action_priority_list( "default" );

  def -> add_action( "auto_attack" );
  def -> add_action( this, "Rebuke" );

  if ( sim -> allow_potions )
  {
    if ( true_level > 100 )
      def -> add_action( "potion,name=old_war,if=(buff.bloodlust.react|buff.avenging_wrath.up|buff.crusade.up&buff.crusade.remains<25|target.time_to_die<=40)" );
    else if ( true_level > 90 )
      def -> add_action( "potion,name=draenic_strength,if=(buff.bloodlust.react|buff.avenging_wrath.up|target.time_to_die<=40)" );
    else if ( true_level > 85 )
      def -> add_action( "potion,name=mogu_power,if=(buff.bloodlust.react|buff.avenging_wrath.up|target.time_to_die<=40)" );
    else if ( true_level >= 80 )
      def -> add_action( "potion,name=golemblood,if=buff.bloodlust.react|buff.avenging_wrath.up|target.time_to_die<=40" );
  }

  // Items
  int num_items = ( int ) items.size();
  for ( int i = 0; i < num_items; i++ )
  {
    if ( items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
    {
      std::string item_str;
      if ( items[i].name_str == "draught_of_souls" )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=(buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack>=15|cooldown.crusade.remains>20&!buff.crusade.up)";
        def -> add_action( item_str );
      }
      else if ( items[i].name_str == "might_of_krosus" )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=(buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack>=15|cooldown.crusade.remains>5&!buff.crusade.up)";
        def -> add_action( item_str );
      }
      else if ( items[i].name_str == "kiljaedens_burning_wish" )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=!raid_event.adds.exists|raid_event.adds.in>75";
        def -> add_action( item_str );
      }
      else if ( items[i].slot != SLOT_WAIST )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=(buff.avenging_wrath.up|buff.crusade.up)";
        def -> add_action( item_str );
      }
    }
  }

  std::vector<std::string> racial_actions = get_racial_actions();
  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[i] == "arcane_torrent" )
    {
      def -> add_action( "arcane_torrent,if=holy_power<5&(buff.crusade.up|buff.avenging_wrath.up|time<2)" );
    }
    else
    {
      def -> add_action( racial_actions[ i ] );
    }
  }

  def -> add_action( this, "Judgment", "if=time<2" );
  def -> add_action( this, "Blade of Justice", "if=time<2&(equipped.137048|race.blood_elf)" );
  def -> add_talent( this, "Divine Hammer", "if=time<2&(equipped.137048|race.blood_elf)" );
  def -> add_action( this, "Wake of Ashes", "if=holy_power<=1&time<2" );
  def -> add_talent( this, "Holy Wrath" );
  def -> add_action( this, "Avenging Wrath" );
  def -> add_action( this, "Shield of Vengeance" );
  def -> add_talent( this, "Crusade", "if=holy_power>=5&!equipped.137048|((equipped.137048|race.blood_elf)&holy_power>=2)" );
  def -> add_talent( this, "Execution Sentence", "if=spell_targets.divine_storm<=3&(cooldown.judgment.remains<gcd*4.5|debuff.judgment.remains>gcd*4.5)&(!talent.crusade.enabled|cooldown.crusade.remains>gcd*2)" );

  def -> add_action( this, "Divine Storm", "if=debuff.judgment.up&spell_targets.divine_storm>=2&buff.divine_purpose.up&buff.divine_purpose.remains<gcd*2" );
  def -> add_action( this, "Divine Storm", "if=debuff.judgment.up&spell_targets.divine_storm>=2&holy_power>=5&buff.divine_purpose.react" );
  def -> add_action( this, "Divine Storm", "if=debuff.judgment.up&spell_targets.divine_storm>=2&holy_power>=3&(buff.crusade.up&(buff.crusade.stack<15|buff.bloodlust.up)|buff.liadrins_fury_unleashed.up)" );
  def -> add_action( this, "Divine Storm", "if=debuff.judgment.up&spell_targets.divine_storm>=2&holy_power>=5&(!talent.crusade.enabled|cooldown.crusade.remains>gcd*3)" );
  def -> add_action( this, "Templar's Verdict", "if=debuff.judgment.up&buff.divine_purpose.up&buff.divine_purpose.remains<gcd*2" );
  def -> add_action( this, "Templar's Verdict", "if=debuff.judgment.up&holy_power>=5&buff.divine_purpose.react" );
  def -> add_action( this, "Templar's Verdict", "if=debuff.judgment.up&holy_power>=3&(buff.crusade.up&(buff.crusade.stack<15|buff.bloodlust.up)|buff.liadrins_fury_unleashed.up)" );
  def -> add_action( this, "Templar's Verdict", "if=debuff.judgment.up&holy_power>=5&(!talent.crusade.enabled|cooldown.crusade.remains>gcd*3)&(!talent.execution_sentence.enabled|cooldown.execution_sentence.remains>gcd)" );
  def -> add_action( this, "Divine Storm", "if=debuff.judgment.up&holy_power>=3&spell_targets.divine_storm>=2&(cooldown.wake_of_ashes.remains<gcd*2&artifact.wake_of_ashes.enabled|buff.whisper_of_the_nathrezim.up&buff.whisper_of_the_nathrezim.remains<gcd)&(!talent.crusade.enabled|cooldown.crusade.remains>gcd*4)" );
  def -> add_action( this, "Templar's Verdict", "if=debuff.judgment.up&holy_power>=3&(cooldown.wake_of_ashes.remains<gcd*2&artifact.wake_of_ashes.enabled|buff.whisper_of_the_nathrezim.up&buff.whisper_of_the_nathrezim.remains<gcd)&(!talent.crusade.enabled|cooldown.crusade.remains>gcd*4)" );
  def -> add_action( this, "Judgment", "if=dot.execution_sentence.ticking&dot.execution_sentence.remains<gcd*2&debuff.judgment.remains<gcd*2" );
  def -> add_action( this, "Wake of Ashes", "if=(!raid_event.adds.exists|raid_event.adds.in>15)&(holy_power=0|holy_power=1&(cooldown.blade_of_justice.remains>gcd|cooldown.divine_hammer.remains>gcd)|holy_power=2&(cooldown.zeal.charges_fractional<=0.65|cooldown.crusader_strike.charges_fractional<=0.65))" );
  def -> add_action( this, "Blade of Justice", "if=(holy_power<=2&set_bonus.tier20_2pc=1|holy_power<=3&set_bonus.tier20_2pc=0)" );
  def -> add_talent( this, "Divine Hammer", "if=(holy_power<=2&set_bonus.tier20_2pc=1|holy_power<=3&set_bonus.tier20_2pc=0)" );
  def -> add_action( this, "Hammer of Justice", "if=equipped.137065&target.health.pct>=75&holy_power<=4" );
  def -> add_action( this, "Judgment" );
  def -> add_talent( this, "Zeal", "if=charges=2&(set_bonus.tier20_2pc=0&holy_power<=2|(holy_power<=4&(cooldown.divine_hammer.remains>gcd*2|cooldown.blade_of_justice.remains>gcd*2)&cooldown.judgment.remains>gcd*2))|(set_bonus.tier20_2pc=1&holy_power<=1|(holy_power<=4&(cooldown.divine_hammer.remains>gcd*2|cooldown.blade_of_justice.remains>gcd*2)&cooldown.judgment.remains>gcd*2))" );
  def -> add_action( this, "Crusader Strike", "if=charges=2&(set_bonus.tier20_2pc=0&holy_power<=2|(holy_power<=4&(cooldown.divine_hammer.remains>gcd*2|cooldown.blade_of_justice.remains>gcd*2)&cooldown.judgment.remains>gcd*2))|(set_bonus.tier20_2pc=1&holy_power<=1|(holy_power<=4&(cooldown.divine_hammer.remains>gcd*2|cooldown.blade_of_justice.remains>gcd*2)&cooldown.judgment.remains>gcd*2))" );
  def -> add_talent( this, "Consecration" );
  def -> add_action( this, "Divine Storm", "if=debuff.judgment.up&spell_targets.divine_storm>=2&buff.divine_purpose.react" );
  def -> add_action( this, "Divine Storm", "if=debuff.judgment.up&spell_targets.divine_storm>=2&buff.the_fires_of_justice.react&(!talent.crusade.enabled|cooldown.crusade.remains>gcd*3)" );
  def -> add_action( this, "Divine Storm", "if=debuff.judgment.up&spell_targets.divine_storm>=2&holy_power>=4&(!talent.crusade.enabled|cooldown.crusade.remains>gcd*4)" );
  def -> add_action( this, "Templar's Verdict", "if=debuff.judgment.up&buff.divine_purpose.react" );
  def -> add_action( this, "Templar's Verdict", "if=debuff.judgment.up&buff.the_fires_of_justice.react&(!talent.crusade.enabled|cooldown.crusade.remains>gcd*3)" );
  def -> add_action( this, "Templar's Verdict", "if=debuff.judgment.up&holy_power>=4&(!talent.crusade.enabled|cooldown.crusade.remains>gcd*4)&(!talent.execution_sentence.enabled|cooldown.execution_sentence.remains>gcd*2)" );
  def -> add_talent( this, "Zeal", "if=holy_power<=4" );
  def -> add_action( this, "Crusader Strike", "if=holy_power<=4" );
  def -> add_action( this, "Divine Storm", "if=debuff.judgment.up&holy_power>=3&spell_targets.divine_storm>=2&(!talent.crusade.enabled|cooldown.crusade.remains>gcd*5)" );
  def -> add_action( this, "Templar's Verdict", "if=debuff.judgment.up&holy_power>=3&(!talent.crusade.enabled|cooldown.crusade.remains>gcd*5)" );
  // def -> add_talent( this, "Blinding Light" );
}

// ==========================================================================
// Action Priority List Generation - Holy
// ==========================================================================

void paladin_t::generate_action_prio_list_holy_dps()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  if ( sim -> allow_flasks && true_level >= 80 )
  {
    std::string flask_action = "flask,type=";
    if ( true_level > 90 )
      flask_action += "greater_draenic_intellect_flask";
    else
      flask_action += ( true_level > 85 ) ? "warm_sun" : "draconic_mind";

    precombat -> add_action( flask_action );
  }

  if ( sim -> allow_food && level() >= 80 )
  {
    std::string food_action = "food,type=";
    if ( level() > 90 )
      food_action += "pickled_eel";
    else
      food_action += ( level() > 85 ) ? "mogu_fish_stew" : "seafood_magnifique_feast";
    precombat -> add_action( food_action );
  }

  precombat -> add_action( this, "Seal of Insight" );
  precombat -> add_action( this, "Beacon of Light" , "target=healing_target");

  // Snapshot stats
  precombat -> add_action( "snapshot_stats",  "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat -> add_action( "potion,name=draenic_intellect" );

  // action priority list
  action_priority_list_t* def = get_action_priority_list( "default" );

  def -> add_action( "potion,name=draenic_intellect,if=buff.bloodlust.react|target.time_to_die<=40" );
  def -> add_action( "auto_attack" );
  int num_items = ( int ) items.size();
  for ( int i = 0; i < num_items; i++ )
  {
    if ( items[ i ].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
    {
      def -> add_action ( "/use_item,name=" + items[ i ].name_str );
    }
  }

  std::vector<std::string> racial_actions = get_racial_actions();
  for ( size_t i = 0; i < racial_actions.size(); i++ )
    def -> add_action( racial_actions[ i ] );

  def -> add_action( this, "Avenging Wrath" );
  def -> add_talent( this, "Execution Sentence" );
  def -> add_action( "holy_shock,damage=1" );
  def -> add_action( this, "Denounce" );
}

void paladin_t::generate_action_prio_list_holy()
{
  // currently unsupported

  //precombat first
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  //Flask
  if ( sim -> allow_flasks && true_level >= 80 )
  {
    std::string flask_action = "flask,type=";
    if ( true_level > 90 )
      flask_action += "greater_draenic_intellect_flask";
    else
      flask_action += ( true_level > 85 ) ? "warm_sun" : "draconic_mind";

    precombat -> add_action( flask_action );
  }

  // Food
  if ( sim -> allow_food && level() >= 80 )
  {
    std::string food_action = "food,type=";
    if ( level() > 90 )
      food_action += "pickled_eel";
    else
      food_action += ( level() > 85 ) ? "mogu_fish_stew" : "seafood_magnifique_feast";
    precombat -> add_action( food_action );
  }

  precombat -> add_action( this, "Seal of Insight" );
  precombat -> add_action( this, "Beacon of Light" , "target=healing_target");
  // Beacon probably goes somewhere here?
  // Damn right it does, Theckie-poo.

  // Snapshot stats
  precombat -> add_action( "snapshot_stats",  "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  // action priority list
  action_priority_list_t* def = get_action_priority_list( "default" );

  def -> add_action( "mana_potion,if=mana.pct<=75" );

  def -> add_action( "auto_attack" );

  int num_items = ( int ) items.size();
  for ( int i = 0; i < num_items; i++ )
  {
    if ( items[ i ].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
    {
      def -> add_action ( "/use_item,name=" + items[ i ].name_str );
    }
  }

  std::vector<std::string> profession_actions = get_profession_actions();
  for ( size_t i = 0; i < profession_actions.size(); i++ )
    def -> add_action( profession_actions[ i ] );

  std::vector<std::string> racial_actions = get_racial_actions();
  for ( size_t i = 0; i < racial_actions.size(); i++ )
    def -> add_action( racial_actions[ i ] );

  // this is just sort of made up to test things - a real Holy dev should probably come up with something useful here eventually
  // Workin on it. Phillipuh to-do
  def -> add_action( this, "Avenging Wrath" );
  def -> add_action( this, "Lay on Hands","if=incoming_damage_5s>health.max*0.7" );
  def -> add_action( "wait,if=target.health.pct>=75&mana.pct<=10" );
  def -> add_action( this, "Flash of Light", "if=target.health.pct<=30" );
  def -> add_action( this, "Lay on Hands", "if=mana.pct<5" );
  def -> add_action( this, "Holy Light" );

}

void paladin_t::init_assessors()
{
  player_t::init_assessors();
  if ( artifact.judge_unworthy.rank() )
  {
    paladin_t* p = this;
    assessor_out_damage.add(
      assessor::TARGET_DAMAGE - 1,
      [ p ]( dmg_e, action_state_t* state )
      {
        buff_t* judgment = p -> get_target_data( state -> target ) -> buffs.debuffs_judgment;
        if ( judgment -> check() )
        {
          if ( p -> rng().roll(0.5) )
            return assessor::CONTINUE;

          // Do the spread
          double max_distance  = 10.0;
          unsigned max_targets = 1;
          std::vector<player_t*> valid_targets;
          range::remove_copy_if(
              p->sim->target_list.data(), std::back_inserter( valid_targets ),
              [p, state, max_distance]( player_t* plr ) {
                paladin_td_t* td = p -> get_target_data( plr );
                if ( td -> buffs.debuffs_judgment -> check() )
                  return true;
                return state -> target -> get_player_distance( *plr ) > max_distance;
              } );
          if ( valid_targets.size() > max_targets )
          {
            valid_targets.resize( max_targets );
          }
          for ( player_t* target : valid_targets )
          {
            p -> get_target_data( target ) -> buffs.debuffs_judgment -> trigger(
                judgment -> check(),
                judgment -> check_value(),
                -1.0,
                judgment -> remains()
              );
          }
        }
        return assessor::CONTINUE;
      }
    );
  }
}

// paladin_t::init_actions ==================================================

void paladin_t::init_action_list()
{
#ifdef NDEBUG // Only restrict on release builds.
  // Holy isn't fully supported atm
  if ( specialization() == PALADIN_HOLY )
  {
    if ( ! quiet )
      sim -> errorf( "Paladin holy healing for player %s is not currently supported.", name() );

    quiet = true;
    return;
  }
#endif
  // sanity check - Prot/Ret can't do anything w/o main hand weapon equipped
  if ( main_hand_weapon.type == WEAPON_NONE && ( specialization() == PALADIN_RETRIBUTION || specialization() == PALADIN_PROTECTION ) )
  {
    if ( !quiet )
      sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  active_shield_of_vengeance_proc    = new shield_of_vengeance_proc_t   ( this );

  // create action priority lists
  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    switch ( specialization() )
    {
      case PALADIN_RETRIBUTION:
        generate_action_prio_list_ret(); // RET
        break;
        // for prot, call subroutine
      case PALADIN_PROTECTION:
        generate_action_prio_list_prot(); // PROT
        break;
      case PALADIN_HOLY:
        if ( primary_role() == ROLE_HEAL )
          generate_action_prio_list_holy(); // HOLY
        else
          generate_action_prio_list_holy_dps();
        break;
      default:
        if ( true_level > 80 )
        {
          action_list_str = "flask,type=draconic_mind/food,type=severed_sagefish_head";
          action_list_str += "/potion,name=volcanic_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<=60";
        }
        action_list_str += "/snapshot_stats";
        action_list_str += "/auto_attack";
        break;
    }
    use_default_action_list = true;
  }
  else
  {
    // if an apl is provided (i.e. from a simc file), set it as the default so it can be validated
    // precombat APL is automatically stored in the new format elsewhere, no need to fix that
    get_action_priority_list( "default" ) -> action_list_str = action_list_str;
    // clear action_list_str to avoid an assert error in player_t::init_actions()
    action_list_str.clear();
  }

  player_t::init_action_list();
}

void paladin_t::init_rng()
{
  player_t::init_rng();

  blade_of_wrath_rppm = get_rppm( "blade_of_wrath", find_spell( 231832 ) );
}

void paladin_t::init()
{
  player_t::init();

  if ( specialization() == PALADIN_HOLY )
    sim -> errorf( "%s is using an unsupported spec.", name() );
}

void paladin_t::init_spells()
{
  player_t::init_spells();

  // Talents
  talents.holy_bolt                  = find_talent_spell( "Holy Bolt" );
  talents.lights_hammer              = find_talent_spell( "Light's Hammer" );
  talents.crusaders_might            = find_talent_spell( "Crusader's Might" );
  talents.beacon_of_hope             = find_talent_spell( "Beacon of Hope" );
  talents.unbreakable_spirit         = find_talent_spell( "Unbreakable Spirit" );
  talents.shield_of_vengeance        = find_talent_spell( "Shield of Vengeance" );
  talents.devotion_aura              = find_talent_spell( "Devotion Aura" );
  talents.aura_of_light              = find_talent_spell( "Aura of Light" );
  talents.aura_of_mercy              = find_talent_spell( "Aura of Mercy" );
  talents.sanctified_wrath           = find_talent_spell( "Sanctified Wrath" );
  talents.holy_prism                 = find_talent_spell( "Holy Prism" );
  talents.stoicism                   = find_talent_spell( "Stoicism" );
  talents.daybreak                   = find_talent_spell( "Daybreak" );
  talents.judgment_of_light          = find_talent_spell( "Judgment of Light" );
  talents.beacon_of_faith            = find_talent_spell( "Beacon of Faith" );
  talents.beacon_of_the_lightbringer = find_talent_spell( "Beacon of the Lightbringer" );
  talents.beacon_of_the_savior       = find_talent_spell( "Beacon of the Savior" );

  talents.first_avenger              = find_talent_spell( "First Avenger" );
  talents.bastion_of_light           = find_talent_spell( "Bastion of Light" );
  talents.crusaders_judgment         = find_talent_spell( "Crusader's Judgment" );
  talents.holy_shield                = find_talent_spell( "Holy Shield" );
  talents.blessed_hammer             = find_talent_spell( "Blessed Hammer" );
  talents.consecrated_hammer         = find_talent_spell( "Consecrated Hammer" );
  talents.blessing_of_spellwarding   = find_talent_spell( "Blessing of Spellwarding" );
  talents.blessing_of_salvation      = find_talent_spell( "Blessing of Salvation" );
  talents.retribution_aura           = find_talent_spell( "Retribution Aura" );
  talents.hand_of_the_protector      = find_talent_spell( "Hand of the Protector" );
  talents.knight_templar             = find_talent_spell( "Knight Templar" );
  talents.final_stand                = find_talent_spell( "Final Stand" );
  talents.aegis_of_light             = find_talent_spell( "Aegis of Light" );
  //talents.judgment_of_light          = find_talent_spell( "Judgment of Light" );
  talents.consecrated_ground         = find_talent_spell( "Consecrated Ground" );
  talents.righteous_protector        = find_talent_spell( "Righteous Protector" );
  talents.seraphim                   = find_talent_spell( "Seraphim" );
  talents.last_defender              = find_talent_spell( "Last Defender" );

  talents.final_verdict              = find_talent_spell( "Final Verdict" );
  talents.execution_sentence         = find_talent_spell( "Execution Sentence" );
  talents.consecration               = find_talent_spell( "Consecration" );
  talents.fires_of_justice           = find_talent_spell( "The Fires of Justice" );
  talents.greater_judgment           = find_talent_spell( "Greater Judgment" );
  talents.zeal                       = find_talent_spell( "Zeal" );
  talents.fist_of_justice            = find_talent_spell( "Fist of Justice" );
  talents.repentance                 = find_talent_spell( "Repentance" );
  talents.blinding_light             = find_talent_spell( "Blinding Light" );
  talents.virtues_blade              = find_talent_spell( "Virtue's Blade" );
  talents.blade_of_wrath             = find_talent_spell( "Blade of Wrath" );
  talents.divine_hammer              = find_talent_spell( "Divine Hammer" );
  talents.justicars_vengeance        = find_talent_spell( "Justicar's Vengeance" );
  talents.eye_for_an_eye             = find_talent_spell( "Eye for an Eye" );
  talents.word_of_glory              = find_talent_spell( "Word of Glory" );
  talents.divine_intervention        = find_talent_spell( "Divine Intervention" );
  talents.divine_steed               = find_talent_spell( "Divine Steed" );
  talents.seal_of_light              = find_talent_spell( "Seal of Light" );
  talents.divine_purpose             = find_talent_spell( "Divine Purpose" ); // TODO: fix this
  talents.crusade                    = find_spell( 231895 );
  talents.crusade_talent             = find_talent_spell( "Crusade" );
  talents.holy_wrath                 = find_talent_spell( "Holy Wrath" );

  artifact.wake_of_ashes           = find_artifact_spell( "Wake of Ashes" );
  artifact.deliver_the_justice     = find_artifact_spell( "Deliver the Justice" );
  artifact.highlords_judgment      = find_artifact_spell( "Highlord's Judgment" );
  artifact.righteous_blade         = find_artifact_spell( "Righteous Blade" );
  artifact.divine_tempest          = find_artifact_spell( "Divine Tempest" );
  artifact.might_of_the_templar    = find_artifact_spell( "Might of the Templar" );
  artifact.sharpened_edge          = find_artifact_spell( "Sharpened Edge" );
  artifact.blade_of_light          = find_artifact_spell( "Blade of Light" );
  artifact.echo_of_the_highlord    = find_artifact_spell( "Echo of the Highlord" );
  artifact.wrath_of_the_ashbringer = find_artifact_spell( "Wrath of the Ashbringer" );
  artifact.endless_resolve         = find_artifact_spell( "Endless Resolve" );
  artifact.deflection              = find_artifact_spell( "Deflection" );
  artifact.ashbringers_light       = find_artifact_spell( "Ashbringer's Light" );
  artifact.ferocity_of_the_silver_hand = find_artifact_spell( "Ferocity of the Silver Hand" );
  artifact.ashes_to_ashes          = find_artifact_spell( "Ashes to Ashes" );
  artifact.righteous_verdict       = find_artifact_spell( "Righteous Verdict" );
  artifact.judge_unworthy          = find_artifact_spell( "Judge Unworthy" );
  artifact.blessing_of_the_ashbringer = find_artifact_spell( "Blessing of the Ashbringer" );

  artifact.eye_of_tyr              = find_artifact_spell( "Eye of Tyr" );
  artifact.truthguards_light       = find_artifact_spell( "Truthguard's Light" );
  artifact.faiths_armor            = find_artifact_spell( "Faith's Armor" );
  artifact.scatter_the_shadows     = find_artifact_spell( "Scatter the Shadows" );
  artifact.righteous_crusader      = find_artifact_spell( "Righteous Crusader" );
  artifact.unflinching_defense     = find_artifact_spell( "Unflinching Defense" );
  artifact.sacrifice_of_the_just   = find_artifact_spell( "Sacrifice of the Just" );
  artifact.hammer_time             = find_artifact_spell( "Hammer Time" );
  artifact.bastion_of_truth        = find_artifact_spell( "Bastion of Truth" );
  artifact.resolve_of_truth        = find_artifact_spell( "Resolve of Truth" );
  artifact.painful_truths          = find_artifact_spell( "Painful Truths" );
  artifact.forbearant_faithful     = find_artifact_spell( "Forbearant Faithful" );
  artifact.consecration_in_flame   = find_artifact_spell( "Consecration in Flame" );
  artifact.stern_judgment          = find_artifact_spell( "Stern Judgment" );
  artifact.bulwark_of_order        = find_artifact_spell( "Bulwark of Order" );
  artifact.light_of_the_titans     = find_artifact_spell( "Light of the Titans" );
  artifact.tyrs_enforcer           = find_artifact_spell( "Tyr's Enforcer" );
  artifact.unrelenting_light       = find_artifact_spell( "Unrelenting Light" );

  // Spells
  spells.holy_light                    = find_specialization_spell( "Holy Light" );
  spells.divine_purpose_ret            = find_spell( 223817 );
  spells.liadrins_fury_unleashed       = find_spell( 208408 );
  spells.justice_gaze                  = find_spell( 211557 );
  spells.chain_of_thrayn               = find_spell( 206338 );
  spells.ashes_to_dust                 = find_spell( 236106 );
  spells.consecration_bonus            = find_spell( 188370 );
  spells.blessing_of_the_ashbringer = find_spell( 242981 );

  // Masteries
  passives.divine_bulwark         = find_mastery_spell( PALADIN_PROTECTION );
  passives.hand_of_light          = find_mastery_spell( PALADIN_RETRIBUTION );
  passives.lightbringer           = find_mastery_spell( PALADIN_HOLY );

  // Specializations
  switch ( specialization() )
  {
    case PALADIN_HOLY:
    spec.judgment_2 = find_specialization_spell( 231644 );
    case PALADIN_PROTECTION:
    spec.judgment_2 = find_specialization_spell( 231657 );
    case PALADIN_RETRIBUTION:
    spec.judgment_2 = find_specialization_spell( 231661 );
    spec.judgment_3 = find_specialization_spell( 231663 );
    default:
    break;
  }

  spec.retribution_paladin = find_specialization_spell( "Retribution Paladin" );

  // Passives

  // Shared Passives
  passives.boundless_conviction   = find_spell( 115675 ); // find_spell fails here
  passives.plate_specialization   = find_specialization_spell( "Plate Specialization" );
  passives.paladin                = find_spell( 137026 );  // find_spell fails here
  passives.retribution_paladin    = find_spell( 137027 );
  passives.protection_paladin     = find_spell( 137028 );
  passives.holy_paladin           = find_spell( 137029 );

  // Holy Passives
  passives.holy_insight           = find_specialization_spell( "Holy Insight" );
  passives.infusion_of_light      = find_specialization_spell( "Infusion of Light" );

  // Prot Passives
  passives.bladed_armor           = find_specialization_spell( "Bladed Armor" );
  passives.grand_crusader         = find_specialization_spell( "Grand Crusader" );
  passives.guarded_by_the_light   = find_specialization_spell( "Guarded by the Light" );
  passives.sanctuary              = find_specialization_spell( "Sanctuary" );
  passives.riposte                = find_specialization_spell( "Riposte" );
  passives.improved_block         = find_specialization_spell( "Improved Block" );

  // Ret Passives
  passives.sword_of_light         = find_specialization_spell( "Sword of Light" );
  passives.sword_of_light_value   = find_spell( passives.sword_of_light -> ok() ? 20113 : 0 );
  passives.judgment             = find_spell( 231663 );

  if ( specialization() == PALADIN_PROTECTION )
  {
    extra_regen_period  = passives.sanctuary -> effectN( 5 ).period();
    extra_regen_percent = passives.sanctuary -> effectN( 5 ).percent();
  }

  if ( find_class_spell( "Beacon of Light" ) -> ok() )
    active_beacon_of_light = new beacon_of_light_heal_t( this );

  if ( talents.holy_shield -> ok() )
    active_holy_shield_proc = new holy_shield_proc_t( this );

  if ( artifact.painful_truths.rank() )
    active_painful_truths_proc = new painful_truths_proc_t( this );

  if ( artifact.tyrs_enforcer.rank() )
    active_tyrs_enforcer_proc = new tyrs_enforcer_proc_t( this );

  if ( talents.judgment_of_light -> ok() )
    active_judgment_of_light_proc = new judgment_of_light_proc_t( this );
}

// paladin_t::primary_role ==================================================

role_e paladin_t::primary_role() const
{
  if ( player_t::primary_role() != ROLE_NONE )
    return player_t::primary_role();

  if ( specialization() == PALADIN_RETRIBUTION )
    return ROLE_ATTACK;

  if ( specialization() == PALADIN_PROTECTION  )
    return ROLE_TANK;

  if ( specialization() == PALADIN_HOLY )
    return ROLE_HEAL;

  return ROLE_HYBRID;
}

// paladin_t::convert_hybrid_stat ===========================================

stat_e paladin_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats.
  stat_e converted_stat = s;

  switch ( s )
  {
    case STAT_STR_AGI_INT:
      switch ( specialization() )
      {
        case PALADIN_HOLY:
          return STAT_INTELLECT;
        case PALADIN_RETRIBUTION:
        case PALADIN_PROTECTION:
          return STAT_STRENGTH;
        default:
          return STAT_NONE;
      }
    // Guess at how AGI/INT mail or leather will be handled for plate - probably INT?
    case STAT_AGI_INT:
      converted_stat = STAT_INTELLECT;
      break;
    // This is a guess at how AGI/STR gear will work for Holy, TODO: confirm
    case STAT_STR_AGI:
      converted_stat = STAT_STRENGTH;
      break;
    case STAT_STR_INT:
      if ( specialization() == PALADIN_HOLY )
        converted_stat = STAT_INTELLECT;
      else
        converted_stat = STAT_STRENGTH;
      break;
    default:
      break;
  }

  // Now disable stats that aren't applicable to a given spec.
  switch ( converted_stat )
  {
    case STAT_STRENGTH:
      if ( specialization() == PALADIN_HOLY )
        converted_stat = STAT_NONE;  // STR disabled for Holy
      break;
    case STAT_INTELLECT:
      if ( specialization() != PALADIN_HOLY )
        converted_stat = STAT_NONE; // INT disabled for Ret/Prot
      break;
    case STAT_AGILITY:
      converted_stat = STAT_NONE; // AGI disabled for all paladins
      break;
    case STAT_SPIRIT:
      if ( specialization() != PALADIN_HOLY )
        converted_stat = STAT_NONE;
      break;
    case STAT_BONUS_ARMOR:
      if ( specialization() != PALADIN_PROTECTION )
        converted_stat = STAT_NONE;
      break;
    default:
      break;
  }

  return converted_stat;
}

// paladin_t::composite_attribute
double paladin_t::composite_attribute( attribute_e attr ) const
{
  double m = player_t::composite_attribute( attr );

  if ( attr == ATTR_STRENGTH )
  {
    if ( artifact.blessing_of_the_ashbringer.rank() )
    {
      // TODO(mserrano): fix this once spelldata gets extracted
      m += 2000; // spells.blessing_of_the_ashbringer -> effectN( 1 ).value();
    }
  }

  return m;
}

// paladin_t::composite_attribute_multiplier ================================

double paladin_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double m = player_t::composite_attribute_multiplier( attr );

  // Guarded by the Light buffs STA
  if ( attr == ATTR_STAMINA )
  {
    m *= 1.0 + passives.guarded_by_the_light -> effectN( 2 ).percent();
  }

  return m;
}

// paladin_t::composite_rating_multiplier ==================================

double paladin_t::composite_rating_multiplier( rating_e r ) const
{
  double m = player_t::composite_rating_multiplier( r );

  return m;

}

// paladin_t::composite_melee_crit_chance =========================================

double paladin_t::composite_melee_crit_chance() const
{
  double m = player_t::composite_melee_crit_chance();

  // This should only give a nonzero boost for Holy
  if ( buffs.avenging_wrath -> check() )
    m += buffs.avenging_wrath -> get_crit_bonus();

  return m;
}

// paladin_t::composite_melee_expertise =====================================

double paladin_t::composite_melee_expertise( const weapon_t* w ) const
{
  double expertise = player_t::composite_melee_expertise( w );

  return expertise;
}

// paladin_t::composite_melee_haste =========================================

double paladin_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  if ( buffs.crusade -> check() )
    h /= 1.0 + buffs.crusade -> get_haste_bonus();

  if ( buffs.sephuz -> check() )
    h /= 1.0 + buffs.sephuz -> check_value();

  if ( sephuz )
    h /= 1.0 + sephuz -> effectN( 3 ).percent() ;

  // Infusion of Light (Holy) adds 10% haste
  //h /= 1.0 + passives.infusion_of_light -> effectN( 2 ).percent();

  return h;
}

// paladin_t::composite_melee_speed =========================================

double paladin_t::composite_melee_speed() const
{
  return player_t::composite_melee_speed();
}

// paladin_t::composite_spell_crit_chance ==========================================

double paladin_t::composite_spell_crit_chance() const
{
  double m = player_t::composite_spell_crit_chance();

  // This should only give a nonzero boost for Holy
  if ( buffs.avenging_wrath -> check() )
    m += buffs.avenging_wrath -> get_crit_bonus();

  return m;
}

// paladin_t::composite_spell_haste ==========================================

double paladin_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( buffs.crusade -> check() )
    h /= 1.0 + buffs.crusade -> get_haste_bonus();

  if ( buffs.sephuz -> check() )
    h /= 1.0 + buffs.sephuz -> check_value();

  if ( sephuz )
    h /= 1.0 + sephuz -> effectN( 3 ).percent() ;

  // Infusion of Light (Holy) adds 10% haste
  //h /= 1.0 + passives.infusion_of_light -> effectN( 2 ).percent();

  return h;
}

// paladin_t::composite_mastery =============================================

double paladin_t::composite_mastery() const
{
  double m = player_t::composite_mastery();
  return m;
}

double paladin_t::composite_mastery_rating() const
{
  double m = player_t::composite_mastery_rating();
  return m;
}

// paladin_t::composite_armor_multiplier ======================================

double paladin_t::composite_armor_multiplier() const
{
  double a = player_t::composite_armor_multiplier();

  // Faith's Armor boosts armor when below 40% health
  if ( resources.current[ RESOURCE_HEALTH ] / resources.max[ RESOURCE_HEALTH ] < artifact.faiths_armor.percent( 1 ) )
    a *= 1.0 + artifact.faiths_armor.percent( 2 );

  a *= 1.0 + artifact.unrelenting_light.percent();

  return a;
}


// paladin_t::composite_bonus_armor =========================================

double paladin_t::composite_bonus_armor() const
{
  double ba = player_t::composite_bonus_armor();

  return ba;
}

// paladin_t::composite_player_multiplier ===================================

double paladin_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  // These affect all damage done by the paladin

  // "Sword of Light" buffs everything now
  if ( specialization() == PALADIN_RETRIBUTION )
  {
    m *= 1.0 + passives.retribution_paladin -> effectN( 6 ).percent();
  }

  // Avenging Wrath buffs everything
  if ( buffs.avenging_wrath -> check() )
  {
    double aw_multiplier = buffs.avenging_wrath -> get_damage_mod();
    if ( chain_of_thrayn )
    {
      aw_multiplier += spells.chain_of_thrayn -> effectN( 4 ).percent();
    }
    m *= 1.0 + aw_multiplier;
  }

  if ( buffs.crusade -> check() )
  {
    double aw_multiplier = buffs.crusade -> get_damage_mod();
    m *= 1.0 + aw_multiplier;
    if ( chain_of_thrayn )
    {
      m *= 1.0 + spells.chain_of_thrayn -> effectN( 4 ).percent();
    }
  }

  m *= 1.0 + buffs.wings_of_liberty -> current_stack * buffs.wings_of_liberty -> current_value;

  if ( retribution_trinket )
    m *= 1.0 + buffs.retribution_trinket -> current_stack * buffs.retribution_trinket -> current_value;

  // WoD Ret PvP 4-piece buffs everything
  if ( buffs.vindicators_fury -> check() )
    m *= 1.0 + buffs.vindicators_fury -> value() * buffs.vindicators_fury -> data().effectN( 1 ).percent();

  // Last Defender
  if ( talents.last_defender -> ok() )
  {
    // Last defender gives the same amount of damage increase as it gives mitigation.
    // Mitigation is 0.97^n, or (1-0.03)^n, where the 0.03 is in the spell data.
    // The damage buff is then 1+(1-0.97^n), or 2-(1-0.03)^n.
    int num_enemies = get_local_enemies( talents.last_defender -> effectN( 1 ).base_value() );
    m *= 2.0 - std::pow( 1.0 - talents.last_defender -> effectN( 2 ).percent(), num_enemies );
  }

  // artifacts
  m *= 1.0 + artifact.ashbringers_light.percent();
  m *= 1.0 + artifact.ferocity_of_the_silver_hand.percent();

  if ( school == SCHOOL_HOLY )
    m *= 1.0 + artifact.truthguards_light.percent();

  return m;
}


double paladin_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double m = player_t::composite_player_target_multiplier( target, school );

  paladin_td_t* td = get_target_data( target );

  if ( td -> dots.wake_of_ashes -> is_ticking() )
  {
    if ( ashes_to_dust )
    {
      m *= 1.0 + spells.ashes_to_dust -> effectN( 1 ).percent();
    }
  }

  return m;
}

// paladin_t::composite_player_heal_multiplier ==============================

double paladin_t::composite_player_heal_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_heal_multiplier( s );

  if ( buffs.avenging_wrath -> check() )
  {
    m *= 1.0 + buffs.avenging_wrath -> get_healing_mod();
    if ( chain_of_thrayn )
    {
      // TODO: fix this for holy
      m *= 1.0 + spells.chain_of_thrayn -> effectN( 2 ).percent();
    }
  }

  if ( buffs.crusade -> check() )
  {
    m *= 1.0 + buffs.crusade -> get_healing_mod();
    if ( chain_of_thrayn )
    {
      m *= 1.0 + spells.chain_of_thrayn -> effectN( 2 ).percent();
    }
  }

  // WoD Ret PvP 4-piece buffs everything
  if ( buffs.vindicators_fury -> check() )
    m *= 1.0 + buffs.vindicators_fury -> value() * buffs.vindicators_fury -> data().effectN( 2 ).percent();

  return m;
}

// paladin_t::composite_spell_power =========================================

double paladin_t::composite_spell_power( school_e school ) const
{
  double sp = player_t::composite_spell_power( school );

  // For Protection and Retribution, SP is fixed to AP by passives
  switch ( specialization() )
  {
    case PALADIN_PROTECTION:
      sp = passives.guarded_by_the_light -> effectN( 1 ).percent() * cache.attack_power() * composite_attack_power_multiplier();
      break;
    case PALADIN_RETRIBUTION:
        sp = passives.retribution_paladin -> effectN( 5 ).percent() * cache.attack_power() * composite_attack_power_multiplier();
      break;
    default:
      break;
  }
  return sp;
}

// paladin_t::composite_melee_attack_power ==================================

double paladin_t::composite_melee_attack_power() const
{
  double ap = player_t::composite_melee_attack_power();

  ap += passives.bladed_armor -> effectN( 1 ).percent() * current.stats.get_stat( STAT_BONUS_ARMOR );

  return ap;
}

// paladin_t::composite_attack_power_multiplier =============================

double paladin_t::composite_attack_power_multiplier() const
{
  double ap = player_t::composite_attack_power_multiplier();

  // Mastery bonus is multiplicative with other effects
  ap *= 1.0 + cache.mastery() * passives.divine_bulwark -> effectN( 5 ).mastery_value();

  return ap;
}

// paladin_t::composite_spell_power_multiplier ==============================

double paladin_t::composite_spell_power_multiplier() const
{
  if ( passives.sword_of_light -> ok() || specialization() == PALADIN_RETRIBUTION || passives.guarded_by_the_light -> ok() )
    return 1.0;

  return player_t::composite_spell_power_multiplier();
}

// paladin_t::composite_block ==========================================

double paladin_t::composite_block() const
{
  // this handles base block and and all block subject to diminishing returns
  double block_subject_to_dr = cache.mastery() * passives.divine_bulwark -> effectN( 1 ).mastery_value();
  double b = player_t::composite_block_dr( block_subject_to_dr );

  // Guarded by the Light block not affected by diminishing returns
  // TODO: spell data broken (0%, tooltip values pointing at wrong effects) - revisit once spell data updated
  b += passives.guarded_by_the_light -> effectN( 4 ).percent();

  // Holy Shield (assuming for now that it's not affected by DR)
  b += talents.holy_shield -> effectN( 1 ).percent();

  // Bastion of Truth (assuming not affected by DR)
  b += artifact.bastion_of_truth.percent( 1 );

  return b;
}

// paladin_t::composite_block_reduction =======================================

double paladin_t::composite_block_reduction() const
{
  double br = player_t::composite_block_reduction();

  br += passives.improved_block -> effectN( 1 ).percent();

  return br;
}

// paladin_t::composite_crit_avoidance ========================================

double paladin_t::composite_crit_avoidance() const
{
  double c = player_t::composite_crit_avoidance();

  // Guarded by the Light grants -6% crit chance
  c += passives.guarded_by_the_light -> effectN( 3 ).percent();

  return c;
}

// paladin_t::composite_parry_rating ==========================================

double paladin_t::composite_parry_rating() const
{
  double p = player_t::composite_parry_rating();

  // add Riposte
  if ( passives.riposte -> ok() )
    p += composite_melee_crit_rating();

  return p;
}

// paladin_t::temporary_movement_modifier =====================================

double paladin_t::temporary_movement_modifier() const
{
  double temporary = player_t::temporary_movement_modifier();

  // shamelessly stolen from warrior_t - see that module for how to add more buffs

  // These are ordered in the highest speed movement increase to the lowest, there's no reason to check the rest as they will just be overridden.
  // Also gives correct benefit numbers.
  if ( buffs.divine_steed -> up() )
  {
    // TODO: replace with commented version once we have spell data
    temporary = std::max( buffs.divine_steed -> value(), temporary );
    // temporary = std::max( buffs.divine_steed -> data().effectN( 1 ).percent(), temporary );
  }

  return temporary;
}

// paladin_t::target_mitigation ===============================================

void paladin_t::target_mitigation( school_e school,
                                   dmg_e    dt,
                                   action_state_t* s )
{
  player_t::target_mitigation( school, dt, s );

  // various mitigation effects, Ardent Defender goes last due to absorb/heal mechanics

  // Passive sources (Sanctuary)
  s -> result_amount *= 1.0 + passives.sanctuary -> effectN( 1 ).percent();

  // Last Defender
  if ( talents.last_defender -> ok() )
  {
    // Last Defender gives a multiplier of 0.97^N - coded using spell data in case that changes
    s -> result_amount *= std::pow( 1.0 - talents.last_defender -> effectN( 2 ).percent(), get_local_enemies( talents.last_defender -> effectN( 1 ).base_value() ) );
  }


  if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
    sim -> out_debug.printf( "Damage to %s after passive mitigation is %f", s -> target -> name(), s -> result_amount );

  // Damage Reduction Cooldowns

  // Guardian of Ancient Kings
  if ( buffs.guardian_of_ancient_kings -> up() && specialization() == PALADIN_PROTECTION )
  {
    s -> result_amount *= 1.0 + buffs.guardian_of_ancient_kings -> data().effectN( 3 ).percent();
    if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
      sim -> out_debug.printf( "Damage to %s after GAnK is %f", s -> target -> name(), s -> result_amount );
  }

  // Divine Protection
  if ( buffs.divine_protection -> up() )
  {
    if ( util::school_type_component( school, SCHOOL_MAGIC ) )
    {
      s -> result_amount *= 1.0 + buffs.divine_protection -> data().effectN( 1 ).percent();
    }
    if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
      sim -> out_debug.printf( "Damage to %s after Divine Protection is %f", s -> target -> name(), s -> result_amount );
  }

  // Other stuff

  // Blessed Hammer
  if ( talents.blessed_hammer -> ok() )
  {
    buff_t* b = get_target_data( s -> action -> player ) -> buffs.blessed_hammer_debuff;

    // BH only reduces auto-attack damage. The only distinguishing feature of auto attacks is that
    // our enemies call them "melee_main_hand" and "melee_off_hand", so we need to check for "hand" in name_str
    if ( b -> up() && util::str_in_str_ci( s -> action -> name_str, "_hand" ) )
    {
      // apply mitigation and expire the BH buff
      s -> result_amount *= 1.0 - b -> data().effectN( 2 ).percent();
      b -> expire();
    }
  }

  // Knight Templar
  if ( talents.knight_templar -> ok() && buffs.divine_steed -> up() )
    s -> result_amount *= 1.0 + talents.knight_templar -> effectN( 2 ).percent();

  // Aegis of Light
  if ( talents.aegis_of_light -> ok() && buffs.aegis_of_light -> up() )
    s -> result_amount *= 1.0 + talents.aegis_of_light -> effectN( 1 ).percent();

  // Eye of Tyr
  if ( artifact.eye_of_tyr.rank() && get_target_data( s -> action -> player ) -> buffs.eye_of_tyr_debuff -> up() )
    s -> result_amount *= 1.0 + artifact.eye_of_tyr.percent( 1 );

  if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
    sim -> out_debug.printf( "Damage to %s after other mitigation effects but before SotR is %f", s -> target -> name(), s -> result_amount );

  // Shield of the Righteous
  if ( buffs.shield_of_the_righteous -> check())
  {
    // sotr has a lot going on, so we'll be verbose
    double sotr_mitigation;

    // base effect
    sotr_mitigation = buffs.shield_of_the_righteous -> data().effectN( 1 ).percent();

    // mastery bonus
    sotr_mitigation += cache.mastery() * passives.divine_bulwark -> effectN( 4 ).mastery_value();

    // 3% more reduction with artifact trait
    // TODO: test if this is multiplicative or additive. Assumed additive
    sotr_mitigation += artifact.righteous_crusader.percent( 2 );

    // 20% more effective if standing in Cons
    // TODO: test if this is multiplicative or additive. Assumed multiplicative.
    if ( standing_in_consecration() || talents.consecrated_hammer -> ok() )
      sotr_mitigation *= 1.0 + spells.consecration_bonus -> effectN( 3 ).percent();

    // clamp is hardcoded in tooltip, not shown in effects
    sotr_mitigation = std::max( -0.80, sotr_mitigation );
    sotr_mitigation = std::min( -0.25, sotr_mitigation );

    s -> result_amount *= 1.0 + sotr_mitigation;

    if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
      sim -> out_debug.printf( "Damage to %s after SotR mitigation is %f", s -> target -> name(), s -> result_amount );
  }

  // Ardent Defender
  if ( buffs.ardent_defender -> check() )
  {
    if ( s -> result_amount > 0 && s -> result_amount >= resources.current[ RESOURCE_HEALTH ] )
    {
      // Ardent defender is a little odd - it doesn't heal you *for* 12%, it heals you *to* 12%.
      // It does this by either absorbing all damage and healing you for the difference between 12% and your current health (if current < 12%)
      // or absorbing any damage that would take you below 12% (if current > 12%).
      // To avoid complications with absorb modeling, we're just going to kludge it by adjusting the amount gained or lost accordingly.
      // Also arbitrarily capping at 3x max health because if you're seriously simming bosses that hit for >300% player health I hate you.
      s -> result_amount = 0.0;
      double AD_health_threshold = resources.max[ RESOURCE_HEALTH ] * buffs.ardent_defender -> data().effectN( 2 ).percent();
      if ( resources.current[ RESOURCE_HEALTH ] >= AD_health_threshold )
      {
        resource_loss( RESOURCE_HEALTH,
                       resources.current[ RESOURCE_HEALTH ] - AD_health_threshold,
                       nullptr,
                       s -> action );
      }
      else
      {
        // completely arbitrary heal-capping here at 3x max health, done just so paladin health timelines don't look so insane
        resource_gain( RESOURCE_HEALTH,
                       std::min( AD_health_threshold - resources.current[ RESOURCE_HEALTH ], 3 * resources.max[ RESOURCE_HEALTH ] ),
                       nullptr,
                       s -> action );
      }
      buffs.ardent_defender -> use_oneup();
      buffs.ardent_defender -> expire();
    }

    if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
      sim -> out_debug.printf( "Damage to %s after Ardent Defender is %f", s -> target -> name(), s -> result_amount );
  }
}

// paladin_t::invalidate_cache ==============================================

void paladin_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  if ( ( passives.sword_of_light -> ok() || specialization() == PALADIN_RETRIBUTION || passives.guarded_by_the_light -> ok() || passives.divine_bulwark -> ok() )
       && ( c == CACHE_STRENGTH || c == CACHE_ATTACK_POWER )
     )
  {
    player_t::invalidate_cache( CACHE_SPELL_POWER );
  }

  if ( c == CACHE_ATTACK_CRIT_CHANCE && specialization() == PALADIN_PROTECTION )
    player_t::invalidate_cache( CACHE_PARRY );

  if ( c == CACHE_BONUS_ARMOR && passives.bladed_armor -> ok() )
  {
    player_t::invalidate_cache( CACHE_ATTACK_POWER );
    player_t::invalidate_cache( CACHE_SPELL_POWER );
  }

  if ( c == CACHE_MASTERY && passives.divine_bulwark -> ok() )
  {
    player_t::invalidate_cache( CACHE_BLOCK );
    player_t::invalidate_cache( CACHE_ATTACK_POWER );
    player_t::invalidate_cache( CACHE_SPELL_POWER );
  }
}

// paladin_t::matching_gear_multiplier ======================================

double paladin_t::matching_gear_multiplier( attribute_e attr ) const
{
  double mult = 0.01 * passives.plate_specialization -> effectN( 1 ).base_value();

  switch ( specialization() )
  {
    case PALADIN_PROTECTION:
      if ( attr == ATTR_STAMINA )
        return mult;
      break;
    case PALADIN_RETRIBUTION:
      if ( attr == ATTR_STRENGTH )
        return mult;
      break;
    case PALADIN_HOLY:
      if ( attr == ATTR_INTELLECT )
        return mult;
      break;
    default:
      break;
  }
  return 0.0;
}

// paladin_t::regen  ========================================================

void paladin_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

  // Guarded by the Light / Sword of Light regen.
  if ( extra_regen_period > timespan_t::from_seconds( 0.0 ) )
  {
    last_extra_regen += periodicity;
    while ( last_extra_regen >= extra_regen_period )
    {
      resource_gain( RESOURCE_MANA, resources.max[ RESOURCE_MANA ] * extra_regen_percent, gains.extra_regen );

      last_extra_regen -= extra_regen_period;
    }
  }
}

// paladin_t::assess_damage =================================================

void paladin_t::assess_damage( school_e school,
                               dmg_e    dtype,
                               action_state_t* s )
{
  if ( buffs.divine_shield -> up() )
  {
    s -> result_amount = 0;

    // Return out, as you don't get to benefit from anything else
    player_t::assess_damage( school, dtype, s );
    return;
  }

  // On a block event, trigger Holy Shield
  if ( s -> block_result == BLOCK_RESULT_BLOCKED )
  {
    trigger_holy_shield( s );
  }

  // Also trigger Grand Crusader on an avoidance event (TODO: test if it triggers on misses)
  if ( s -> result == RESULT_DODGE || s -> result == RESULT_PARRY || s -> result == RESULT_MISS )
  {
    trigger_grand_crusader();
  }

  // Trigger Painful Truths artifact damage event
  if ( action_t::result_is_hit( s -> result ) )
    trigger_painful_truths( s );

  player_t::assess_damage( school, dtype, s );
}

// paladin_t::assess_damage_imminent ========================================

void paladin_t::assess_damage_imminent( school_e school, dmg_e, action_state_t* s )
{
  // Holy Shield happens here, after all absorbs are accounted for (see player_t::assess_damage())
  if ( talents.holy_shield -> ok() && s -> result_amount > 0.0 && school != SCHOOL_PHYSICAL )
  {
    // Block code mimics attack_t::block_chance()
    // cache.block() contains our block chance
    double block = cache.block();
    // add or subtract 1.5% per level difference
    block += ( level() - s -> action -> player -> level() ) * 0.015;


    if ( block > 0 )
    {
      // Roll for "block"
      if ( rng().roll( block ) )
      {
        double block_amount = s -> result_amount * composite_block_reduction();

        if ( sim->debug )
          sim -> out_debug.printf( "%s Holy Shield absorbs %f", name(), block_amount );

        // update the relevant counters
        iteration_absorb_taken += block_amount;
        s -> self_absorb_amount += block_amount;
        s -> result_amount -= block_amount;
        s -> result_absorbed = s -> result_amount;

        // hack to register this on the abilities table
        buffs.holy_shield_absorb -> trigger( 1, block_amount );
        buffs.holy_shield_absorb -> consume( block_amount );

        // Trigger the damage event
        trigger_holy_shield( s );
      }
      else
      {
        if ( sim->debug )
          sim -> out_debug.printf( "%s Holy Shield fails to activate", name() );
      }
    }

    if ( sim->debug )
      sim -> out_debug.printf( "Damage to %s after Holy Shield mitigation is %f", name(), s -> result_amount );
  }
}

// paladin_t::assess_heal ===================================================

void paladin_t::assess_heal( school_e school, dmg_e dmg_type, action_state_t* s )
{
  player_t::assess_heal( school, dmg_type, s );
}

// paladin_t::create_options ================================================

void paladin_t::create_options()
{
  // TODO: figure out a better solution for this.
  add_option( opt_float( "paladin_fixed_holy_wrath_health_pct", fixed_holy_wrath_health_pct ) );
  add_option( opt_bool( "paladin_fake_sov", fake_sov ) );
  player_t::create_options();
}

// paladin_t::copy_from =====================================================

void paladin_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  paladin_t* p = debug_cast<paladin_t*>( source );

  fixed_holy_wrath_health_pct = p -> fixed_holy_wrath_health_pct;
  fake_sov = p -> fake_sov;
}

// paladin_t::current_health =================================================

double paladin_t::current_health() const
{
  return player_t::current_health();
}

// paladin_t::combat_begin ==================================================

void paladin_t::combat_begin()
{
  player_t::combat_begin();

  resources.current[ RESOURCE_HOLY_POWER ] = 0;
}

// paladin_t::get_divine_judgment =============================================

double paladin_t::get_divine_judgment() const
{
  if ( specialization() != PALADIN_RETRIBUTION ) return 0.0;

  double handoflight;
  handoflight = cache.mastery_value(); // HoL modifier is in effect #1

  return handoflight;
}

// player_t::create_expression ==============================================

expr_t* paladin_t::create_expression( action_t* a,
                                      const std::string& name_str )
{
  struct paladin_expr_t : public expr_t
  {
    paladin_t& paladin;
    paladin_expr_t( const std::string& n, paladin_t& p ) :
      expr_t( n ), paladin( p ) {}
  };


  std::vector<std::string> splits = util::string_split( name_str, "." );

  struct time_to_hpg_expr_t : public paladin_expr_t
  {
    cooldown_t* cs_cd;
    cooldown_t* boj_cd;
    cooldown_t* j_cd;

    time_to_hpg_expr_t( const std::string& n, paladin_t& p ) :
      paladin_expr_t( n, p ), cs_cd( p.get_cooldown( "crusader_strike" ) ),
      boj_cd ( p.get_cooldown( "blade_of_justice" )),
      j_cd( p.get_cooldown( "judgment" ) )
    { }

    virtual double evaluate() override
    {
      timespan_t gcd_ready = paladin.gcd_ready - paladin.sim -> current_time();
      gcd_ready = std::max( gcd_ready, timespan_t::zero() );

      timespan_t shortest_hpg_time = cs_cd -> remains();

      if ( boj_cd -> remains() < shortest_hpg_time )
        shortest_hpg_time = boj_cd -> remains();

      if ( gcd_ready > shortest_hpg_time )
        return gcd_ready.total_seconds();
      else
        return shortest_hpg_time.total_seconds();
    }
  };

  if ( splits[ 0 ] == "time_to_hpg" )
  {
    return new time_to_hpg_expr_t( name_str, *this );
  }

  return player_t::create_expression( a, name_str );
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class paladin_report_t : public player_report_extension_t
{
public:
  paladin_report_t( paladin_t& player ) :
      p( player )
  {

  }

  virtual void html_customsection( report::sc_html_stream& /* os*/ ) override
  {
    (void) p;
    /*// Custom Class Section
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n"
        << "\t\t\t\t\t<h3 class=\"toggle open\">Custom Section</h3>\n"
        << "\t\t\t\t\t<div class=\"toggle-content\">\n";

    os << p.name();

    os << "\t\t\t\t\t\t</div>\n" << "\t\t\t\t\t</div>\n";*/
  }
private:
  paladin_t& p;
};

static void do_trinket_init( paladin_t*                player,
                             specialization_e         spec,
                             const special_effect_t*& ptr,
                             const special_effect_t&  effect )
{
  // Ensure we have the spell data. This will prevent the trinket effect from working on live
  // Simulationcraft. Also ensure correct specialization.
  if ( ! player -> find_spell( effect.spell_id ) -> ok() ||
       player -> specialization() != spec )
  {
    return;
  }
  // Set pointer, module considers non-null pointer to mean the effect is "enabled"
  ptr = &( effect );
}

static void retribution_trinket( special_effect_t& effect )
{
  paladin_t* s = debug_cast<paladin_t*>( effect.player );
  do_trinket_init( s, PALADIN_RETRIBUTION, s -> retribution_trinket, effect );

  if ( s -> retribution_trinket )
  {
    const spell_data_t* buff_spell = s -> retribution_trinket -> driver() -> effectN( 1 ).trigger();
    s -> buffs.retribution_trinket = buff_creator_t( s, "focus_of_vengeance", buff_spell )
      .default_value( buff_spell -> effectN( 1 ).average( s -> retribution_trinket -> item ) / 100.0 )
      .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }
}

// Legiondaries
static void whisper_of_the_nathrezim( special_effect_t& effect )
{
  paladin_t* s = debug_cast<paladin_t*>( effect.player );
  do_trinket_init( s, PALADIN_RETRIBUTION, s -> whisper_of_the_nathrezim, effect );
}

static void liadrins_fury_unleashed( special_effect_t& effect )
{
  paladin_t* s = debug_cast<paladin_t*>( effect.player );
  do_trinket_init( s, PALADIN_RETRIBUTION, s -> liadrins_fury_unleashed, effect );
}

static void justice_gaze( special_effect_t& effect )
{
  paladin_t* s = debug_cast<paladin_t*>( effect.player );
  do_trinket_init( s, PALADIN_RETRIBUTION, s -> justice_gaze, effect );
}

static void chain_of_thrayn( special_effect_t& effect )
{
  paladin_t* s = debug_cast<paladin_t*>( effect.player );
  do_trinket_init( s, PALADIN_RETRIBUTION, s -> chain_of_thrayn, effect );
}

static void ashes_to_dust( special_effect_t& effect )
{
  paladin_t* s = debug_cast<paladin_t*>( effect.player );
  do_trinket_init( s, PALADIN_RETRIBUTION, s -> ashes_to_dust, effect );
}

struct sephuzs_secret_enabler_t : public unique_gear::scoped_actor_callback_t<paladin_t>
{
  sephuzs_secret_enabler_t() : scoped_actor_callback_t( PALADIN )
  { }

  void manipulate( paladin_t* paladin, const special_effect_t& e ) override
  { paladin -> sephuz = e.driver(); }
};

// PALADIN MODULE INTERFACE =================================================

struct paladin_module_t : public module_t
{
  paladin_module_t() : module_t( PALADIN ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const override
  {
    auto  p = new paladin_t( sim, name, r );
    p -> report_extension = std::unique_ptr<player_report_extension_t>( new paladin_report_t( *p ) );
    return p;
  }

  virtual bool valid() const override { return true; }

  virtual void static_init() const override
  {
    unique_gear::register_special_effect( 184911, retribution_trinket );
    unique_gear::register_special_effect( 207633, whisper_of_the_nathrezim );
    unique_gear::register_special_effect( 208408, liadrins_fury_unleashed );
    unique_gear::register_special_effect( 206338, chain_of_thrayn );
    unique_gear::register_special_effect( 236106, ashes_to_dust );
    unique_gear::register_special_effect( 211557, justice_gaze );
    unique_gear::register_special_effect( 208051, sephuzs_secret_enabler_t(), true );
  }

  virtual void init( player_t* p ) const override
  {
    p -> buffs.beacon_of_light          = buff_creator_t( p, "beacon_of_light", p -> find_spell( 53563 ) );
    p -> buffs.blessing_of_sacrifice    = new buffs::blessing_of_sacrifice_t( p );
    p -> debuffs.forbearance            = new buffs::forbearance_t( static_cast<paladin_t*>( p ), "forbearance" );
  }

  virtual void register_hotfixes() const override
  {
    hotfix::register_effect( "Paladin", "2017-03-29", "Righteous Verdict bonus increased to 8% per point (was 5% per point)", 360747 )
       .field( "base_value" )
       .operation( hotfix::HOTFIX_SET )
       .modifier( 8 )
       .verification_value( 5 );
  }

  virtual void combat_begin( sim_t* ) const override {}

  virtual void combat_end  ( sim_t* ) const override {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::paladin()
{
  static paladin_module_t m;
  return &m;
}


