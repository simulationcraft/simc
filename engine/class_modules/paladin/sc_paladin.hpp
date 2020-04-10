#pragma once
#include "simulationcraft.hpp"

namespace paladin {
// Forward declarations
typedef std::pair<std::string, simple_sample_data_with_min_max_t> data_t;
typedef std::pair<std::string, simple_sample_data_t> simple_data_t;
struct paladin_t;
struct blessing_of_sacrifice_redirect_t;
namespace buffs {
                  struct avenging_wrath_buff_t;
                  struct crusade_buff_t;
                  struct holy_avenger_buff_t;
                  struct ardent_defender_buff_t;
                  struct forbearance_t;
                  struct shield_of_vengeance_buff_t;
                }
const int MAX_START_OF_COMBAT_HOLY_POWER = 1;
// ==========================================================================
// Paladin Target Data
// ==========================================================================

struct paladin_td_t : public actor_target_data_t
{
  struct dots_t
  {
  } dots;

  struct buffs_t
  {
    buff_t* execution_sentence;
    buff_t* judgment;
    buff_t* judgment_of_light;
    buff_t* blessed_hammer;
  } debuff;

  paladin_td_t( player_t* target, paladin_t* paladin );
};

struct cooldown_waste_data_t : public noncopyable
{
  const cooldown_t* cd;
  double buffer;

  extended_sample_data_t normal;
  extended_sample_data_t cumulative;

  cooldown_waste_data_t( const cooldown_t* cooldown, bool simple = true ) :
    cd( cooldown ), buffer( 0.0 ), normal( cd -> name_str + " waste", simple ),
    cumulative( cd -> name_str + " cumulative waste", simple ) {}

  virtual bool may_add( timespan_t cd_override = timespan_t::min() ) const
  {
    return ( cd -> duration > 0_ms || cd_override > 0_ms )
        && ( ( cd -> charges == 1 && cd -> up() ) || ( cd -> charges >= 2 && cd -> current_charge == cd -> charges ) )
        && ( cd -> last_charged > 0_ms && cd -> last_charged < cd -> sim.current_time() );
  }

  virtual double get_wasted_time()
  {
    return (cd -> sim.current_time() - cd -> last_charged).total_seconds();
  }

  void add( timespan_t cd_override = timespan_t::min(), timespan_t time_to_execute = 0_ms )
  {
    if ( may_add( cd_override ) )
    {
      double wasted = get_wasted_time();
      if ( cd -> charges == 1 )
      {
        wasted -= time_to_execute.total_seconds();
      }
      normal.add( wasted );
      buffer += wasted;
    }
  }

  bool active() const
  {
    return normal.count() > 0 && cumulative.sum() > 0;
  }

  void merge( const cooldown_waste_data_t& other )
  {
    normal.merge( other.normal );
    cumulative.merge( other.cumulative );
  }

  void analyze()
  {
    normal.analyze();
    cumulative.analyze();
  }

  void datacollection_begin()
  {
    buffer = 0.0;
  }

  void datacollection_end()
  {
    if ( may_add() )
      buffer += get_wasted_time();
    cumulative.add( buffer );
    buffer = 0.0;
  }

  virtual ~cooldown_waste_data_t() { }
};

struct paladin_t : public player_t
{
public:

  // waste tracking
  auto_dispose<std::vector<cooldown_waste_data_t*> > cooldown_waste_data_list;

  // Active spells
  struct active_spells_t
  {
    heal_t* beacon_of_light;
    action_t* holy_shield_damage;
    action_t* judgment_of_light;
    action_t* shield_of_vengeance_damage;
    action_t* zeal;

    action_t* inner_light_damage;
    action_t* lights_decree;

    // Required for seraphim
    action_t* sotr;

    blessing_of_sacrifice_redirect_t* blessing_of_sacrifice_redirect;
  } active;

  // Buffs
  struct buffs_t
  {
    // Shared
    buffs::avenging_wrath_buff_t* avenging_wrath;
    buff_t* avenging_wrath_autocrit;
    buff_t* divine_purpose;
    buff_t* divine_shield;
    buff_t* divine_steed;

    buff_t* avengers_might;

    // Holy
    buff_t* divine_protection;
    buff_t* holy_avenger;
    buff_t* infusion_of_light;

    // Prot
    absorb_buff_t* holy_shield_absorb; // Dummy buff to trigger spell damage "blocking" absorb effect
    buff_t* seraphim;
    buff_t* aegis_of_light;
    buff_t* ardent_defender;
    buff_t* avengers_valor;
    buff_t* grand_crusader;
    buff_t* guardian_of_ancient_kings;
    buff_t* redoubt;
    buff_t* shield_of_the_righteous;

    buff_t* inner_light;
    buff_t* inspiring_vanguard;
    buff_t* soaring_shield;

    // Ret
    buffs::crusade_buff_t* crusade;
    buffs::shield_of_vengeance_buff_t* shield_of_vengeance;
    buff_t* blade_of_wrath;
    buff_t* divine_judgment;
    buff_t* fires_of_justice;
    buff_t* inquisition;
    buff_t* righteous_verdict;
    buff_t* zeal;

    buff_t* empyrean_power;
    buff_t* relentless_inquisitor;
  } buffs;

  // Gains
  struct gains_t
  {
    // Healing/absorbs
    gain_t* holy_shield;

    // Mana
    gain_t* mana_beacon_of_light;

    // Holy Power
    gain_t* hp_templars_verdict_refund;
    gain_t* judgment;
    gain_t* hp_cs;
    gain_t* hp_memory_of_lucid_dreams;
  } gains;

  // Spec Passives
  struct spec_t
  {
    const spell_data_t* judgment_2;
    const spell_data_t* shield_of_the_righteous;
    const spell_data_t* holy_paladin;
    const spell_data_t* protection_paladin;
    const spell_data_t* retribution_paladin;
  } spec;

  // Cooldowns
  struct cooldowns_t
  {
    // Required to get various cooldown-reducing procs procs working
    cooldown_t* avenging_wrath; // Righteous Protector (prot)
    cooldown_t* hammer_of_justice;
    cooldown_t* judgment_of_light_icd;

    cooldown_t* holy_shock; // Crusader's Might, Divine Purpose
    cooldown_t* light_of_dawn; // Divine Purpose

    cooldown_t* avengers_shield; // Grand Crusader
    cooldown_t* consecration; // Precombat shenanigans
    cooldown_t* hand_of_the_protector; // Righteous Protector
    cooldown_t* inner_light_icd;
    cooldown_t* judgment; // Crusader's Judgment
    cooldown_t* light_of_the_protector;  // Righteous Protector
    cooldown_t* shield_of_the_righteous; // Judgment

    cooldown_t* blade_of_justice;
  } cooldowns;

  // Passives
  struct passives_t
  {
    const spell_data_t* paladin;
    const spell_data_t* plate_specialization;

    const spell_data_t* infusion_of_light;

    const spell_data_t* avengers_valor;
    const spell_data_t* grand_crusader;
    const spell_data_t* riposte;
    const spell_data_t* sanctuary;

    const spell_data_t* boundless_conviction;
  } passives;

  struct mastery_t
  {
    const spell_data_t* divine_bulwark; // Prot
    const spell_data_t* hand_of_light; // Ret
    const spell_data_t* lightbringer; // Holy
  } mastery;

  // Procs and RNG
  real_ppm_t* art_of_war_rppm;

  struct procs_t
  {
    proc_t* art_of_war;
    proc_t* divine_purpose;
    proc_t* fires_of_justice;
    proc_t* grand_crusader;
    proc_t* prot_lucid_dreams;
  } procs;

  // Spells
  struct spells_t
  {
    const spell_data_t* avenging_wrath;
    const spell_data_t* avenging_wrath_autocrit;
    const spell_data_t* divine_purpose_buff;
    const spell_data_t* judgment_debuff;

    const spell_data_t* sotr_buff;

    const spell_data_t* execution_sentence_debuff;
    const spell_data_t* lights_decree;
  } spells;

  // Talents
  struct talents_t
  {
    // Duplicate names are commented out

    // Holy
    // T15
    const spell_data_t* crusaders_might;
    const spell_data_t* bestow_faith;
    const spell_data_t* lights_hammer;
    // T30
    const spell_data_t* unbreakable_spirit;
    const spell_data_t* cavalier;
    const spell_data_t* rule_of_law;
    // T45
    const spell_data_t* fist_of_justice;
    const spell_data_t* repentance;
    const spell_data_t* blinding_light;
    // T60
    const spell_data_t* devotion_aura;
    const spell_data_t* aura_of_sacrifice;
    const spell_data_t* aura_of_mercy;
    // T75
    const spell_data_t* judgment_of_light;
    const spell_data_t* holy_prism;
    const spell_data_t* holy_avenger;
    // T90
    const spell_data_t* sanctified_wrath;
    const spell_data_t* avenging_crusader; // NYI
    const spell_data_t* awakening; // NYI
    // T100
    const spell_data_t* divine_purpose;
    const spell_data_t* beacon_of_faith;
    const spell_data_t* beacon_of_virtue;

    // Protection
    // T15
    const spell_data_t* holy_shield;
    const spell_data_t* redoubt;
    const spell_data_t* blessed_hammer;
    // T30
    const spell_data_t* first_avenger;
    const spell_data_t* crusaders_judgment;
    const spell_data_t* bastion_of_light;
    // skip T45, see Holy
    // T60
    const spell_data_t* retribution_aura; // NYI
    // const spell_data_t* cavalier;
    const spell_data_t* blessing_of_spellwarding;
    // T75
    // const spell_data_t* unbreakable_spirit;
    const spell_data_t* final_stand;
    const spell_data_t* hand_of_the_protector;
    // T90
    // const spell_data_t* judgment_of_light;
    const spell_data_t* consecrated_ground;
    const spell_data_t* aegis_of_light;
    // T100
    const spell_data_t* last_defender;
    const spell_data_t* righteous_protector;
    const spell_data_t* seraphim;

    // Retribution
    // T15
    const spell_data_t* zeal;
    const spell_data_t* righteous_verdict;
    const spell_data_t* execution_sentence;
    // T30
    const spell_data_t* fires_of_justice;
    const spell_data_t* blade_of_wrath;
    const spell_data_t* hammer_of_wrath;
    // Skip T45, see Holy
    // T60
    const spell_data_t* divine_judgment;
    const spell_data_t* consecration;
    const spell_data_t* wake_of_ashes;
    // T75
    // const spell_data_t* unbreakable_spirit;
    // const spell_data_t* cavalier;
    const spell_data_t* eye_for_an_eye; // Defensive, NYI
    // T90
    const spell_data_t* selfless_healer; // Healing, NYI
    const spell_data_t* justicars_vengeance;
    const spell_data_t* word_of_glory; // Healing, NYI
    // T100
    // const spell_data_t* divine_purpose;
    const spell_data_t* crusade;
    const spell_data_t* inquisition;
  } talents;

  struct azerite_t
  {
    // Shared
    azerite_power_t avengers_might;
    azerite_power_t grace_of_the_justicar; // Healing, NYI
    azerite_power_t indomitable_justice;

    // Holy

    // Protection
    azerite_power_t bulwark_of_light; // Defensive, NYI
    azerite_power_t inspiring_vanguard;
    azerite_power_t inner_light;
    azerite_power_t soaring_shield;

    // Retribution
    azerite_power_t empyrean_power;
    azerite_power_t expurgation;
    azerite_power_t lights_decree;
    azerite_power_t relentless_inquisitor;
  } azerite;

  struct {
    azerite_essence_t memory_of_lucid_dreams;
    azerite_essence_t vision_of_perfection;
  } azerite_essence;

  // Paladin options
  struct options_t
  {
    double proc_chance_ret_memory_of_lucid_dreams = 0.15;
    double proc_chance_prot_memory_of_lucid_dreams = 0.15;
    bool fake_sov = true;
    int indomitable_justice_pct = 0;
  } options;

  player_t* beacon_target;

  double lucid_dreams_accumulator;
  double lucid_dreams_minor_refund_coeff;

  paladin_t( sim_t* sim, const std::string& name, race_e r = RACE_TAUREN );

  virtual void      init_base_stats() override;
  virtual void      init_gains() override;
  virtual void      init_procs() override;
  virtual void      init() override;
  virtual void      init_scaling() override;
  virtual void      create_buffs() override;
  virtual void      init_rng() override;
  virtual void      init_spells() override;
  virtual void      init_action_list() override;
  virtual void      reset() override;
  virtual std::unique_ptr<expr_t> create_expression( const std::string& name ) override;

  // player stat functions
  virtual double    composite_attribute_multiplier( attribute_e attr ) const override;
  virtual double    composite_attack_power_multiplier() const override;
  virtual double    composite_bonus_armor() const override;
  virtual double    composite_melee_attack_power() const override;
  virtual double    composite_melee_attack_power(attack_power_type ap_type ) const override;
  virtual double    composite_melee_haste() const override;
  virtual double    composite_melee_speed() const override;
  virtual double    composite_spell_haste() const override;
  virtual double    composite_spell_power( school_e school ) const override;
  virtual double    composite_spell_power_multiplier() const override;
  virtual double    composite_crit_avoidance() const override;
  virtual double    composite_parry_rating() const override;
  virtual double    composite_block() const override;
  virtual double    composite_block_reduction( action_state_t* s ) const override;
  virtual double    temporary_movement_modifier() const override;

  virtual double    resource_gain( resource_e resource_type, double amount, gain_t* g = nullptr, action_t* a = nullptr ) override;

  // combat outcome functions
  virtual void      assess_damage( school_e, result_amount_type, action_state_t* ) override;
  virtual void      target_mitigation( school_e, result_amount_type, action_state_t* ) override;

  virtual void      invalidate_cache( cache_e ) override;
  virtual void      create_options() override;
  virtual double    matching_gear_multiplier( attribute_e attr ) const override;
  virtual void      create_actions() override;
  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override;
  virtual resource_e primary_resource() const override;
  virtual role_e    primary_role() const override;
  virtual stat_e    convert_hybrid_stat( stat_e s ) const override;
  virtual void      combat_begin() override;
  virtual void      copy_from( player_t* ) override;

  void    trigger_grand_crusader();
  void    trigger_holy_shield( action_state_t* s );
  void    trigger_forbearance( player_t* target );
  int     get_local_enemies( double distance ) const;
  bool    standing_in_consecration() const;
  double  last_defender_damage() const;
  double  last_defender_mitigation() const;
  // Returns true if AW/Crusade is up, or if the target is below 20% HP.
  // This isn't in HoW's target_ready() so it can be used in the time_to_hpg expression
  bool    get_how_availability( player_t* t ) const;

  void         trigger_memory_of_lucid_dreams( double cost );
  virtual void vision_of_perfection_proc() override;

  std::unique_ptr<expr_t> create_consecration_expression( const std::string& expr_str );

  ground_aoe_event_t* active_consecration;

  std::string default_potion() const override;
  std::string default_flask() const override;
  std::string default_food() const override;
  std::string default_rune() const override;

  void      create_buffs_retribution();
  void      init_rng_retribution();
  void      init_spells_retribution();
  void      generate_action_prio_list_ret();
  void      create_ret_actions();
  action_t* create_action_retribution( const std::string& name, const std::string& options_str );

  void      create_buffs_protection();
  void      init_spells_protection();
  void      create_prot_actions();
  action_t* create_action_protection( const std::string& name, const std::string& options_str );

  void      create_buffs_holy();
  void      init_spells_holy();
  void      create_holy_actions();
  action_t* create_action_holy( const std::string& name, const std::string& options_str );

  void    generate_action_prio_list_prot();
  void    generate_action_prio_list_holy();
  void    generate_action_prio_list_holy_dps();

  target_specific_t<paladin_td_t> target_data;

  virtual paladin_td_t* get_target_data( player_t* target ) const override;

  cooldown_waste_data_t* get_cooldown_waste_data( cooldown_t* cd, cooldown_waste_data_t *(*factory)(cooldown_t*) = nullptr )
  {
    for ( auto cdw : cooldown_waste_data_list )
    {
      if ( cdw -> cd -> name_str == cd -> name_str )
        return cdw;
    }

    cooldown_waste_data_t* cdw = nullptr;
    if ( factory == nullptr ) {
      cdw = new cooldown_waste_data_t( cd );
    } else {
      cdw = factory( cd );
    }
    cooldown_waste_data_list.push_back( cdw );
    return cdw;
  }
  virtual void merge( player_t& other ) override;
  virtual void analyze( sim_t& s ) override;
  virtual void datacollection_begin() override;
  virtual void datacollection_end() override;
};

namespace buffs {
  struct avenging_wrath_buff_t : public buff_t
  {
    avenging_wrath_buff_t( paladin_t* p );

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

  struct crusade_buff_t : public buff_t
  {
    crusade_buff_t( player_t* p );

    double get_damage_mod()
    {
      return damage_modifier * ( this -> stack() );
    }

    double get_haste_bonus()
    {
      return haste_bonus * ( this -> stack() );
    }
    private:
    double damage_modifier;
    double haste_bonus;
  };

  struct forbearance_t : public buff_t
  {
    paladin_t* paladin;

    forbearance_t( player_t* p, const char *name ) :
      buff_t( p, name, p -> find_spell( 25771 ) ),
      paladin( nullptr )
    { }

    forbearance_t( paladin_td_t* ap, const char *name ) :
      buff_t( *ap, name, ap -> source -> find_spell( 25771 ) ),
      paladin( debug_cast<paladin_t*>( ap -> source ) )
    { }
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

  bool track_cd_waste;
  cooldown_waste_data_t* cd_waste;
  cooldown_waste_data_t* (*cd_waste_factory)(cooldown_t *);

  // Damage increase whitelists
  struct affected_by_t
  {
    bool avenging_wrath, avenging_wrath_autocrit, judgment; // Shared
    bool crusade, divine_purpose, execution_sentence, hand_of_light, inquisition; // Ret
    bool last_defender; // Prot
  } affected_by;

  // haste scaling bools
  bool hasted_cd;
  bool hasted_gcd;

  paladin_action_t( const std::string& n, paladin_t* p,
                    const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, p, s ),
    track_cd_waste( s -> cooldown() > 0_ms || s -> charge_cooldown() > 0_ms ),
    cd_waste( nullptr ), cd_waste_factory( nullptr ),
    affected_by( affected_by_t() ),
    hasted_cd( false ), hasted_gcd( false )
  {
    // Spec aura damage increase
    if ( p -> specialization() == PALADIN_PROTECTION )
    {
      if ( this -> data().affected_by( p -> spec.protection_paladin -> effectN( 1 ) ) )
      { // Direct damage, global
        this -> base_dd_multiplier *= 1.0 + p -> spec.protection_paladin -> effectN( 1 ).percent();
      }
      if ( this -> data().affected_by( p -> spec.protection_paladin -> effectN( 2 ) ) )
      { // Periodic damage, global
        this -> base_td_multiplier *= 1.0 + p -> spec.protection_paladin -> effectN( 2 ).percent();
      }

      this -> affected_by.last_defender = this -> data().affected_by( p -> talents.last_defender -> effectN( 5 ) );
    }

    else if ( p -> specialization() == PALADIN_HOLY )
    {
      this -> affected_by.judgment = this -> data().affected_by( p -> spells.judgment_debuff -> effectN( 1 ) );
    }

    else // Default to Ret
    {
      if ( this -> data().affected_by( p -> spec.retribution_paladin -> effectN( 1 ) ) )
      { // Direct damage, global
        this -> base_dd_multiplier *= 1.0 + p -> spec.retribution_paladin -> effectN( 1 ).percent();
      }
      if ( this -> data().affected_by( p -> spec.retribution_paladin -> effectN( 2 ) ) )
      { // Periodic damage, global
        this -> base_td_multiplier *= 1.0 + p -> spec.retribution_paladin -> effectN( 2 ).percent();
      }

      if ( this -> data().affected_by( p -> spec.retribution_paladin -> effectN( 11 ) ) )
      { // 2019-04-01, 0% increase to judgment, keeping it here just in case
        this -> base_dd_multiplier *= 1.0 + p -> spec.retribution_paladin -> effectN( 11 ).percent();
      }

      if ( this -> data().affected_by( p -> spec.retribution_paladin -> effectN( 12 ) ) )
      { // In the same manner, 0% increase to crusader's strike
        this -> base_dd_multiplier *= 1.0 + p -> spec.retribution_paladin -> effectN( 12 ).percent();
      }

      if ( this -> data().affected_by( p -> spec.retribution_paladin -> effectN( 13 ) ) )
      { // Consecration
        this -> base_dd_multiplier *= 1.0 + p -> spec.retribution_paladin -> effectN( 13 ).percent();
      }

      // Mastery
      this -> affected_by.hand_of_light = this -> data().affected_by( p -> mastery.hand_of_light -> effectN( 1 ) );

      // Temporary damage modifiers
      this -> affected_by.crusade = this -> data().affected_by( p -> talents.crusade -> effectN( 1 ) );
      this -> affected_by.divine_purpose = this -> data().affected_by( p -> spells.divine_purpose_buff -> effectN( 2 ) );
      this -> affected_by.execution_sentence = this -> data().affected_by( p -> spells.execution_sentence_debuff -> effectN( 1 ) );
      this -> affected_by.inquisition = this -> data().affected_by( p -> talents.inquisition -> effectN( 1 ) );
      this -> affected_by.judgment = this -> data().affected_by( p -> spells.judgment_debuff -> effectN( 1 ) );
    }

    this -> affected_by.avenging_wrath = this -> data().affected_by( p -> spells.avenging_wrath -> effectN( 1 ) );
    this -> affected_by.avenging_wrath_autocrit = this -> data().affected_by( p -> spells.avenging_wrath_autocrit -> effectN( 1 ) );

    // The whitelists for spells affected by a hasted gcd/cd are spread over a lot of different effects and spells
    // This browses the given spell data to find cd/gcd affecting effects and if they affect the current spell
    auto update_hasted_cooldowns_by_passive = [&](const spell_data_t* passive) {
      for (uint32_t i = 1; i <= passive -> effect_count(); i++) {
        auto effect = passive -> effectN( i );
        if ( effect.subtype() == A_HASTED_CATEGORY ) {
          uint32_t affected_category = effect.misc_value1();
          if ( affected_category == ab::data().category() ) {
            hasted_cd = true;
          }
        } else if ( effect.subtype() == A_HASTED_COOLDOWN ) {
          if ( ab::data().affected_by( effect ) ) {
            hasted_cd = true;
          }
        } else if ( effect.subtype() == A_HASTED_GCD ) {
          if ( ab::data().affected_by( effect ) ) {
            hasted_gcd = true;
          }
        }
      }
    };
    update_hasted_cooldowns_by_passive( p -> passives.paladin );
    if ( p -> specialization() == PALADIN_RETRIBUTION ) {
      update_hasted_cooldowns_by_passive( p -> spec.retribution_paladin );
    } else if ( p -> specialization() == PALADIN_PROTECTION ) {
      update_hasted_cooldowns_by_passive( p -> spec.protection_paladin );
    } else {
      update_hasted_cooldowns_by_passive( p -> spec.holy_paladin );
    }
    if ( hasted_cd && !hasted_gcd ) hasted_gcd = true;
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

    if ( track_cd_waste && ab::sim -> report_details != 0 )
    {
      cd_waste = p() -> get_cooldown_waste_data( ab::cooldown, cd_waste_factory );
    }

    if ( hasted_cd )
    {
      ab::cooldown -> hasted = hasted_cd;
    }
    if ( hasted_gcd )
    {
      if ( p() -> specialization() == PALADIN_HOLY )
      {
        ab::gcd_type = gcd_haste_type::SPELL_HASTE;
      }
      else
      {
        ab::gcd_type = gcd_haste_type::ATTACK_HASTE;
      }
    }
  }

  void execute() override
  {
    ab::execute();

    if ( this -> affected_by.avenging_wrath_autocrit )
    {
      p() -> buffs.avenging_wrath_autocrit -> expire();
    }
  }

  void trigger_judgment_of_light( action_state_t* s )
  {
    // Don't activate if the player is at full HP
    if ( p() -> resources.current[ RESOURCE_HEALTH ] < p() -> resources.max[ RESOURCE_HEALTH ] )
    {
      p() -> active.judgment_of_light -> execute();
      td ( s -> target ) -> debuff.judgment_of_light -> decrement();
    }
  }

  void impact( action_state_t* s ) override
  {
    ab::impact( s );

    if ( td( s -> target ) -> debuff.judgment_of_light -> up() && p() -> cooldowns.judgment_of_light_icd -> up() )
    {
      trigger_judgment_of_light( s );
      p() -> cooldowns.judgment_of_light_icd -> start();
    }

    if ( ab::result_is_hit( s -> result ) && affected_by.judgment )
    {
      paladin_td_t* td = this -> td( s -> target );
      if ( td -> debuff.judgment -> up() )
        td -> debuff.judgment -> expire();
    }
  }

  virtual double action_multiplier() const override
  {
    double am = ab::action_multiplier();

    if ( p() -> specialization() == PALADIN_RETRIBUTION )
    {
      if ( affected_by.hand_of_light )
      {
        am *= 1.0 + p() -> cache.mastery_value();
      }

      if ( affected_by.crusade && p() -> buffs.crusade -> up() )
      {
        am *= 1.0 + p() -> buffs.crusade -> get_damage_mod();
      }

      // Divine purpose damage increase handled here,
      // Cost handled in holy_power_generator_t
      if ( affected_by.divine_purpose && p() -> buffs.divine_purpose -> up() )
      {
        am *= 1.0 + p() -> spells.divine_purpose_buff -> effectN( 2 ).percent();
      }

      if ( affected_by.inquisition && p() -> buffs.inquisition -> up() )
      {
        am *= 1.0 + p() -> talents.inquisition -> effectN( 1 ).percent();
      }
    }

    if ( affected_by.avenging_wrath && p() -> buffs.avenging_wrath -> up() )
    {
      am *= 1.0 + p() -> buffs.avenging_wrath -> get_damage_mod();
    }

    if ( affected_by.last_defender && p() -> talents.last_defender -> ok() )
    {
      am *= p() -> last_defender_damage();
    }

    return am;
  }

  double composite_crit_chance() const override
  {
    if ( affected_by.avenging_wrath_autocrit && p() -> buffs.avenging_wrath_autocrit -> up() )
    {
      return 1.0;
    }

    double cc = ab::composite_crit_chance();

    if ( affected_by.avenging_wrath && p() -> buffs.avenging_wrath -> up() )
    {
      cc += p() -> buffs.avenging_wrath -> get_crit_bonus();
    }

    return cc;
  }

  virtual double composite_target_multiplier( player_t* t ) const override
  {
    double ctm = ab::composite_target_multiplier( t );

    paladin_td_t* td = this -> td( t );

    // Handles both holy and ret judgment
    if ( affected_by.judgment && td -> debuff.judgment -> up() )
    {
      ctm *= 1.0 + p() -> spells.judgment_debuff -> effectN( 1 ).percent();
    }

    if ( affected_by.execution_sentence && td -> debuff.execution_sentence -> up() )
    {
      ctm *= 1.0 + p() -> spells.execution_sentence_debuff -> effectN( 1 ).percent();
    }

    return ctm;
  }

  virtual void update_ready( timespan_t cd = timespan_t::min() ) override
  {
    if ( cd_waste ) cd_waste -> add( cd, ab::time_to_execute );
    ab::update_ready( cd );
  }
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
  { }

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
  { }
};

struct paladin_heal_t : public paladin_spell_base_t<heal_t>
{
  paladin_heal_t( const std::string& n, paladin_t* p,
                  const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
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

    assert( p() -> active.beacon_of_light );

    p() -> active.beacon_of_light -> target = p() -> beacon_target;

    double amount = s -> result_amount;
    amount *= p() -> beacon_target -> buffs.beacon_of_light -> data().effectN( 1 ).percent();

    p() -> active.beacon_of_light -> base_dd_min = amount;
    p() -> active.beacon_of_light -> base_dd_max = amount;

    p() -> active.beacon_of_light -> execute();
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

struct judgment_t : public paladin_melee_attack_t
{
  int indomitable_justice_pct;
  judgment_t( paladin_t* p, const std::string& options_str );

  virtual double bonus_da( const action_state_t* s ) const override;
  proc_types proc_type() const override;
  void impact( action_state_t* s ) override;
};

struct shield_of_the_righteous_buff_t : public buff_t
{
  shield_of_the_righteous_buff_t( paladin_t* p );
  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override;
  bool trigger( int stacks, double chance, double value, timespan_t duration ) override;

  double default_av_increase;
  double avengers_valor_increase;
};

void empyrean_power( special_effect_t& effect );

}
