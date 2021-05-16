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
                  struct execution_sentence_debuff_t;
                }
const int MAX_START_OF_COMBAT_HOLY_POWER = 1;

enum season : unsigned int {
  SUMMER = 0,
  AUTUMN = 1,
  WINTER = 2,
  SPRING = 3,
  NUM_SEASONS = 4,
};

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
    buffs::execution_sentence_debuff_t* execution_sentence;
    buff_t* judgment;
    buff_t* judgment_of_light;
    buff_t* blessed_hammer;
    buff_t* final_reckoning;
    buff_t* reckoning;
    buff_t* vengeful_shock;
  } debuff;

  paladin_td_t( player_t* target, paladin_t* paladin );
};

struct paladin_t : public player_t
{
public:
  // Active spells
  struct active_spells_t
  {
    heal_t* beacon_of_light;
    action_t* holy_shield_damage;
    action_t* judgment_of_light;
    action_t* shield_of_vengeance_damage;
    action_t* zeal;
    action_t* reckoning;

    action_t* inner_light_damage;
    action_t* lights_decree;
    action_t* sanctified_wrath;

    // Required for seraphim
    action_t* sotr;

    blessing_of_sacrifice_redirect_t* blessing_of_sacrifice_redirect;

    // Covenant stuff
    action_t* necrolord_divine_storm;
    action_t* necrolord_shield_of_the_righteous;
    action_t* divine_toll;
    action_t* seasons[NUM_SEASONS];

    // Conduit stuff
    action_t* virtuous_command;
  } active;

  // Buffs
  struct buffs_t
  {
    // Shared
    buffs::avenging_wrath_buff_t* avenging_wrath;
    buff_t* divine_purpose;
    buff_t* divine_shield;
    buff_t* divine_steed;
    buff_t* devotion_aura;

    buff_t* avengers_might;

    // Holy
    buff_t* divine_protection;
    buff_t* holy_avenger;
    buff_t* avenging_crusader;
    buff_t* infusion_of_light;

    // Prot
    absorb_buff_t* holy_shield_absorb; // Dummy buff to trigger spell damage "blocking" absorb effect
    absorb_buff_t* blessed_hammer_absorb; // ^
    buff_t* first_avenger_absorb;
    buff_t* seraphim;
    buff_t* ardent_defender;
    buff_t* grand_crusader;
    buff_t* guardian_of_ancient_kings;
    buff_t* redoubt;
    buff_t* shield_of_the_righteous;
    buff_t* moment_of_glory;
    buff_t* shielding_words;
    buff_t* shining_light_stacks;
    buff_t* shining_light_free;
    buff_t* royal_decree;

    buff_t* inner_light;
    buff_t* inspiring_vanguard;
    buff_t* soaring_shield;

    // Ret
    buffs::crusade_buff_t* crusade;
    buffs::shield_of_vengeance_buff_t* shield_of_vengeance;
    buff_t* blade_of_wrath;
    buff_t* fires_of_justice;
    buff_t* righteous_verdict;
    buff_t* zeal;

    buff_t* empyrean_power;
    buff_t* empyrean_power_azerite;
    buff_t* relentless_inquisitor_azerite;

    // Covenants
    buff_t* vanquishers_hammer;

    // Legendaries
    buff_t* vanguards_momentum;
    buff_t* bulwark_of_righteous_fury;
    buff_t* blessing_of_dusk;
    buff_t* blessing_of_dawn;
    buff_t* relentless_inquisitor;
    buff_t* the_magistrates_judgment;
    buff_t* final_verdict;
    buff_t* virtuous_command;
    buff_t* reign_of_ancient_kings;
  } buffs;

  // Gains
  struct gains_t
  {
    // Healing/absorbs
    gain_t* holy_shield;
    gain_t* first_avenger;
    gain_t* blessed_hammer;

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
    const spell_data_t* judgment_3;
    const spell_data_t* judgment_4;
    const spell_data_t* consecration_2;
    const spell_data_t* consecration_3;
    const spell_data_t* shield_of_the_righteous;
    const spell_data_t* holy_paladin;
    const spell_data_t* protection_paladin;
    const spell_data_t* retribution_paladin;
    const spell_data_t* hammer_of_the_righteous_2;
    const spell_data_t* word_of_glory_2;
    const spell_data_t* holy_shock_2;
  } spec;

  // Cooldowns
  struct cooldowns_t
  {
    // Required to get various cooldown-reducing procs procs working
    cooldown_t* avenging_wrath; // Righteous Protector (prot)
    cooldown_t* hammer_of_justice;
    cooldown_t* judgment_of_light_icd;
    cooldown_t* the_magistrates_judgment_icd;

    cooldown_t* holy_shock; // Crusader's Might, Divine Purpose
    cooldown_t* light_of_dawn; // Divine Purpose

    cooldown_t* avengers_shield; // Grand Crusader
    cooldown_t* consecration; // Precombat shenanigans
    cooldown_t* inner_light_icd;
    cooldown_t* judgment; // Crusader's Judgment
    cooldown_t* shield_of_the_righteous; // Judgment
    cooldown_t* guardian_of_ancient_kings; // Righteous Protector
    cooldown_t* ardent_defender; // Resolute Defender

    cooldown_t* blade_of_justice;
    cooldown_t* final_reckoning;
    cooldown_t* hammer_of_wrath;

    cooldown_t* blessing_of_the_seasons;
  } cooldowns;

  // Passives
  struct passives_t
  {
    const spell_data_t* paladin;
    const spell_data_t* plate_specialization;

    const spell_data_t* infusion_of_light;

    const spell_data_t* grand_crusader;
    const spell_data_t* riposte;
    const spell_data_t* sanctuary;

    const spell_data_t* boundless_conviction;

    const spell_data_t* art_of_war;
    const spell_data_t* art_of_war_2;
  } passives;

  struct mastery_t
  {
    const spell_data_t* divine_bulwark; // Prot
    const spell_data_t* divine_bulwark_2; // Rank 2 - consecration DR
    const spell_data_t* hand_of_light; // Ret
    const spell_data_t* lightbringer; // Holy
  } mastery;

  // Procs and RNG
  real_ppm_t* final_reckoning_rppm;

  struct procs_t
  {
    proc_t* art_of_war;
    proc_t* divine_purpose;
    proc_t* fires_of_justice;
    proc_t* final_reckoning;
    proc_t* prot_lucid_dreams;
    proc_t* empyrean_power;

    proc_t* as_grand_crusader;
    proc_t* as_grand_crusader_wasted;
    proc_t* as_engraved_sigil;
    proc_t* as_engraved_sigil_wasted;
    proc_t* as_moment_of_glory;
    proc_t* as_moment_of_glory_wasted;
  } procs;

  // Spells
  struct spells_t
  {
    const spell_data_t* avenging_wrath;
    const spell_data_t* divine_purpose_buff;
    const spell_data_t* judgment_debuff;

    const spell_data_t* sotr_buff;

    const spell_data_t* reckoning;
    const spell_data_t* lights_decree;
    const spell_data_t* sanctified_wrath_damage;

    const spell_data_t* judgment_2;
    const spell_data_t* avenging_wrath_2;
    const spell_data_t* avenging_wrath_3;
    const spell_data_t* hammer_of_wrath_2;

    const spell_data_t* ashen_hallow_how;
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
    // T25
    const spell_data_t* saved_by_the_light;
    const spell_data_t* judgment_of_light;
    const spell_data_t* holy_prism;
    // T30
    const spell_data_t* fist_of_justice;
    const spell_data_t* repentance;
    const spell_data_t* blinding_light;
    // T35
    const spell_data_t* unbreakable_spirit;
    const spell_data_t* cavalier;
    const spell_data_t* rule_of_law;
    // T40
    const spell_data_t* divine_purpose;
    const spell_data_t* holy_avenger;
    const spell_data_t* seraphim;
    // T45
    const spell_data_t* holy_sanctified_wrath;
    const spell_data_t* avenging_crusader;
    const spell_data_t* awakening;
    // T50
    const spell_data_t* glimmer_of_light;
    const spell_data_t* beacon_of_faith;
    const spell_data_t* beacon_of_virtue;

    // Protection
    // T15
    const spell_data_t* holy_shield;
    const spell_data_t* redoubt;
    const spell_data_t* blessed_hammer;
    // T25
    const spell_data_t* first_avenger;
    const spell_data_t* crusaders_judgment;
    const spell_data_t* moment_of_glory;
    // skip T30, see Holy
    // T35
    // unbreakable spirit + cavalier in holy
    const spell_data_t* blessing_of_spellwarding;
    // skip T40, see Holy
    // T45
    // const spell_data_t* hand_of_the_protector;
    const spell_data_t* consecrated_ground;
    // const spell_data_t* judgment_of_light;
    // T50
    const spell_data_t* prot_sanctified_wrath;
    const spell_data_t* righteous_protector;
    const spell_data_t* final_stand;

    // Retribution
    // T15
    const spell_data_t* zeal;
    const spell_data_t* righteous_verdict;
    const spell_data_t* execution_sentence;
    // T25
    const spell_data_t* fires_of_justice;
    const spell_data_t* blade_of_wrath;
    const spell_data_t* empyrean_power;
    // Skip T30, see Holy
    // T35
    // unbreakable spirit + cavalier in holy
    const spell_data_t* eye_for_an_eye;
    // Skip T40, see Holy
    // T45
    const spell_data_t* selfless_healer;
    const spell_data_t* justicars_vengeance;
    const spell_data_t* healing_hands;
    // T50
    const spell_data_t* ret_sanctified_wrath;
    const spell_data_t* crusade;
    const spell_data_t* final_reckoning;
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

  struct conduits_t {
    conduit_data_t ringing_clarity;
    conduit_data_t vengeful_shock;
    conduit_data_t focused_light;
    conduit_data_t expurgation;
    conduit_data_t templars_vindication;
    conduit_data_t the_long_summer;
    conduit_data_t truths_wake;
    conduit_data_t virtuous_command;
    conduit_data_t righteous_might;
    conduit_data_t hallowed_discernment;
    conduit_data_t punish_the_guilty;
    conduit_data_t resolute_defender;
    conduit_data_t shielding_words;
    conduit_data_t golden_path;
    conduit_data_t royal_decree;
  } conduit;

  struct convenants_t {
    const spell_data_t* kyrian;
    const spell_data_t* venthyr;
    const spell_data_t* necrolord;
    const spell_data_t* night_fae;
  } covenant;

  struct legendaries_t {
    item_runeforge_t vanguards_momentum;
    item_runeforge_t the_mad_paragon;
    item_runeforge_t final_verdict;
    item_runeforge_t of_dusk_and_dawn;
    item_runeforge_t the_magistrates_judgment;
    item_runeforge_t bulwark_of_righteous_fury;
    item_runeforge_t holy_avengers_engraved_sigil;
    item_runeforge_t the_ardent_protectors_sanctum;
    item_runeforge_t relentless_inquisitor;
    item_runeforge_t tempest_of_the_lightbringer;
    item_runeforge_t reign_of_endless_kings;
  } legendary;

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

  season next_season;

  paladin_t( sim_t* sim, util::string_view name, race_e r = RACE_TAUREN );

  virtual void      init_assessors() override;
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
  virtual std::unique_ptr<expr_t> create_expression( util::string_view name ) override;

  // player stat functions
  virtual double    composite_player_multiplier( school_e ) const override;
  virtual double    composite_attribute_multiplier( attribute_e attr ) const override;
  virtual double    composite_attack_power_multiplier() const override;
  virtual double    composite_bonus_armor() const override;
  virtual double    composite_melee_attack_power() const override;
  virtual double    composite_melee_attack_power_by_type(attack_power_type ap_type ) const override;
  virtual double    composite_melee_crit_chance() const override;
  virtual double    composite_spell_crit_chance() const override;
  virtual double    composite_damage_versatility() const override;
  virtual double    composite_heal_versatility() const override;
  virtual double    composite_mitigation_versatility() const override;
  virtual double    composite_mastery() const override;
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
  virtual double 	  composite_player_target_multiplier ( player_t *target, school_e school ) const override;
  virtual double    composite_base_armor_multiplier() const override;

  virtual double resource_gain( resource_e resource_type, double amount, gain_t* source = nullptr,
                                action_t* action = nullptr ) override;
  virtual double resource_loss( resource_e resource_type, double amount, gain_t* source = nullptr,
                                action_t* action = nullptr ) override;

  // combat outcome functions
  virtual void      assess_damage( school_e, result_amount_type, action_state_t* ) override;
  virtual void      target_mitigation( school_e, result_amount_type, action_state_t* ) override;

  virtual void      invalidate_cache( cache_e ) override;
  virtual void      create_options() override;
  virtual double    matching_gear_multiplier( attribute_e attr ) const override;
  virtual void      create_actions() override;
  virtual action_t* create_action( util::string_view name, const std::string& options_str ) override;
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
  bool    standing_in_hallow() const;
  // Returns true if AW/Crusade is up, or if the target is below 20% HP.
  // This isn't in HoW's target_ready() so it can be used in the time_to_hpg expression
  bool    get_how_availability( player_t* t ) const;

  void         trigger_memory_of_lucid_dreams( double cost );
  virtual void vision_of_perfection_proc() override;

  std::unique_ptr<expr_t> create_consecration_expression( util::string_view expr_str );

  ground_aoe_event_t* active_consecration;
  ground_aoe_event_t* active_hallow;
  buff_t* active_aura;

  std::string default_potion() const override;
  std::string default_flask() const override;
  std::string default_food() const override;
  std::string default_rune() const override;
  std::string default_temporary_enchant() const override;

  void apply_affecting_auras( action_t& action ) override;

  void      create_buffs_retribution();
  void      init_rng_retribution();
  void      init_spells_retribution();
  void      generate_action_prio_list_ret();
  void      create_ret_actions();
  action_t* create_action_retribution( util::string_view name, const std::string& options_str );

  void      create_buffs_protection();
  void      init_spells_protection();
  void      create_prot_actions();
  action_t* create_action_protection( util::string_view name, const std::string& options_str );

  void      create_buffs_holy();
  void      init_spells_holy();
  void      create_holy_actions();
  action_t* create_action_holy( util::string_view name, const std::string& options_str );

  void    generate_action_prio_list_prot();
  void    generate_action_prio_list_holy();
  void    generate_action_prio_list_holy_dps();

  target_specific_t<paladin_td_t> target_data;

  virtual const paladin_td_t* find_target_data( const player_t* target ) const override;
  virtual paladin_td_t* get_target_data( player_t* target ) const override;
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

    double get_damage_mod() const
    {
      return damage_modifier * ( this -> check() );
    }

    double get_haste_bonus() const
    {
      return haste_bonus * ( this -> check() );
    }
    private:
    double damage_modifier;
    double haste_bonus;
  };

  struct execution_sentence_debuff_t : public buff_t
  {
    execution_sentence_debuff_t( paladin_td_t* td ) :
      buff_t( *td, "execution_sentence", debug_cast<paladin_t*>( td -> source ) -> talents.execution_sentence ),
      accumulated_damage( 0.0 ) {
      set_cooldown( 0_ms ); // handled by the ability
    }

    void reset() override
    {
      buff_t::reset();
      accumulated_damage = 0.0;
    }

    void expire_override( int stacks, timespan_t duration ) override
    {
      buff_t::expire_override( stacks, duration );

      accumulated_damage = 0.0;
    }

    void accumulate_damage( const action_state_t* s )
    {
      sim -> print_debug(
        "{}'s {} accumulates {} additional damage: {} -> {}",
        player -> name(), name(), s -> result_total,
        accumulated_damage, accumulated_damage + s -> result_total );

      accumulated_damage += s -> result_total;
    }

    double get_accumulated_damage() const
    {
      return accumulated_damage;
    }

  private:
    double accumulated_damage;
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

  // Damage increase whitelists
  struct affected_by_t
  {
    bool avenging_wrath, judgment, blessing_of_dawn, the_magistrates_judgment; // Shared
    bool crusade, divine_purpose, divine_purpose_cost, hand_of_light, final_reckoning, reckoning; // Ret
    bool avenging_crusader; // Holy
  } affected_by;

  // haste scaling bools
  bool hasted_cd;
  bool hasted_gcd;

  paladin_action_t( util::string_view n, paladin_t* p,
                    const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, p, s ),
    track_cd_waste( s -> cooldown() > 0_ms || s -> charge_cooldown() > 0_ms ),
    cd_waste( nullptr ), 
    affected_by( affected_by_t() ),
    hasted_cd( false ), hasted_gcd( false )
  {
    // Spec aura damage increase
    if ( p -> specialization() == PALADIN_RETRIBUTION )
    {
      // Mastery
      this -> affected_by.hand_of_light = this -> data().affected_by( p -> mastery.hand_of_light -> effectN( 1 ) );

      // Temporary damage modifiers
      this -> affected_by.crusade = this -> data().affected_by( p -> talents.crusade -> effectN( 1 ) );
      this -> affected_by.reckoning = this -> data().affected_by( p -> spells.reckoning -> effectN( 1 ) );
      this -> affected_by.final_reckoning = this -> data().affected_by( p -> talents.final_reckoning -> effectN( 3 ) );
    }
    if ( p->specialization() == PALADIN_HOLY )
    {
      this->affected_by.avenging_crusader = this->data().affected_by( p->talents.avenging_crusader->effectN(1) );
    }

    this -> affected_by.judgment = this -> data().affected_by( p -> spells.judgment_debuff -> effectN( 1 ) );
    this -> affected_by.avenging_wrath = this -> data().affected_by( p -> spells.avenging_wrath -> effectN( 1 ) );
    this -> affected_by.divine_purpose_cost = this -> data().affected_by( p -> spells.divine_purpose_buff -> effectN( 1 ) );
    this -> affected_by.divine_purpose = this -> data().affected_by( p -> spells.divine_purpose_buff -> effectN( 2 ) );
    this -> affected_by.blessing_of_dawn = this -> data().affected_by( p -> legendary.of_dusk_and_dawn -> effectN( 1 ).trigger() -> effectN( 1 ) );
    this -> affected_by.the_magistrates_judgment = this -> data().affected_by( p -> buffs.the_magistrates_judgment -> data().effectN( 1 ) );
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
      cd_waste = p() -> get_cooldown_waste_data( ab::cooldown );
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


    if ( ab::result_is_hit( s -> result ) )
    {
      if ( affected_by.judgment )
      {
        paladin_td_t* td = this -> td( s -> target );
        if ( td -> debuff.judgment -> up() )
          td -> debuff.judgment -> expire();
      }

      if ( affected_by.reckoning )
      {
        paladin_td_t* td = this -> td( s -> target );
        if ( td -> debuff.reckoning -> up() )
          td -> debuff.reckoning -> expire();
      }

      if ( ab::harmful )
      {
        if ( p() -> talents.final_reckoning -> ok() && p() -> cooldowns.final_reckoning -> up() )
        {
          if ( p() -> final_reckoning_rppm -> trigger() )
          {
            p() -> procs.final_reckoning -> occur();

            p() -> active.reckoning -> set_target( s -> target );
            p() -> active.reckoning -> schedule_execute();
          }
        }
      }


      paladin_td_t* td = this -> td( s -> target );
      if ( td -> debuff.reckoning -> up() )
        td -> debuff.reckoning -> expire();
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
    }

    if ( affected_by.avenging_wrath && p() -> buffs.avenging_wrath -> up() )
    {
      am *= 1.0 + p() -> buffs.avenging_wrath -> get_damage_mod();
    }

    if ( affected_by.avenging_crusader )
    {
      am *= 1.0 + p()->buffs.avenging_crusader->check_value();
    }

    // Divine purpose damage increase handled here,
    // Cost handled in holy_power_consumer_t
    if ( affected_by.divine_purpose && p() -> buffs.divine_purpose -> up() )
    {
      am *= 1.0 + p() -> spells.divine_purpose_buff -> effectN( 2 ).percent();
    }

    if ( affected_by.blessing_of_dawn && p() -> buffs.blessing_of_dawn -> up() )
    {
      am *= 1.0 + p() -> legendary.of_dusk_and_dawn -> effectN ( 1 ).trigger() -> effectN ( 1 ).percent();
    }

    return am;
  }

  double composite_crit_chance() const override
  {
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

    if ( affected_by.reckoning && td -> debuff.reckoning -> up() )
    {
      ctm *= 1.0 + p() -> spells.reckoning -> effectN( 2 ).percent();
    }

    if ( affected_by.final_reckoning && td -> debuff.final_reckoning -> up() )
    {
      ctm *= 1.0 + p() -> talents.final_reckoning -> effectN( 3 ).percent();
    }

    return ctm;
  }

  virtual void update_ready( timespan_t cd = timespan_t::min() ) override
  {
    if ( cd_waste ) cd_waste -> add( cd, ab::time_to_execute );
    ab::update_ready( cd );
  }

  virtual void assess_damage( result_amount_type typ, action_state_t* s ) override
  {
    ab::assess_damage( typ, s );

    paladin_td_t* td = this -> td( s -> target );
    if ( td -> debuff.execution_sentence -> check() )
    {
      td -> debuff.execution_sentence -> accumulate_damage( s );
    }
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

  paladin_spell_base_t( util::string_view n, paladin_t* player,
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
  paladin_spell_t( util::string_view n, paladin_t* p,
                   const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  { }
};

struct paladin_heal_t : public paladin_spell_base_t<heal_t>
{
  paladin_heal_t( util::string_view n, paladin_t* p,
                  const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  {
    may_crit          = true;
    tick_may_crit     = true;
    harmful = false;
    // WARNING: When harmful = false, if you try to cast at time=0
    // then the ability has no cost and no gcd, so it just spams it indefinitely

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
  paladin_absorb_t( util::string_view n, paladin_t* p,
                    const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  { }
};

struct paladin_melee_attack_t: public paladin_action_t < melee_attack_t >
{
  paladin_melee_attack_t( util::string_view n, paladin_t* p,
                          const spell_data_t* s = spell_data_t::nil()) :
    base_t( n, p, s )
  {
    may_crit = true;
    special = true;
    weapon = &( p -> main_hand_weapon );
  }
};

// holy power consumption
// TODO(mserrano): figure out the right way to organize this longer term
struct lights_decree_t : public paladin_spell_t
{
  int last_holy_power_cost;

  lights_decree_t( paladin_t* p ) :
    paladin_spell_t( "lights_decree", p, p -> find_spell( 286232 ) ),
    last_holy_power_cost( 0 )
  {
    base_dd_min = base_dd_max = p -> azerite.lights_decree.value();
    aoe = -1;
    background = may_crit = true;
  }

  double action_multiplier() const override
  {
    return paladin_spell_t::action_multiplier() * last_holy_power_cost;
  }
};

struct sanctified_wrath_t : public paladin_spell_t
{
  int last_holy_power_cost;

  sanctified_wrath_t( paladin_t* p ) :
    paladin_spell_t( "sanctified_wrath", p, p -> find_spell( 326731 ) ),
    last_holy_power_cost( 0 )
  {
    aoe = -1;
    background = may_crit = true;
  }

  double action_multiplier() const override
  {
    return paladin_spell_t::action_multiplier() * last_holy_power_cost;
  }
};

template <class Base >
struct holy_power_consumer_t : public Base
{
  private:
    typedef Base ab; // action base, eg. spell_t
  public:
    typedef holy_power_consumer_t base_t;
  bool is_divine_storm;
  bool is_wog;
  holy_power_consumer_t( util::string_view n, paladin_t* player, const spell_data_t* s ) :
    ab( n, player, s ),
    is_divine_storm ( false ),
    is_wog( false )
  { }

  double cost() const override
  {
    // paladin_t* p = paladin_action_t<Base>::template p();

    if ( ab::background )
    {
      return 0.0;
    }

    if ( ( is_divine_storm && ( ab::p() -> buffs.empyrean_power_azerite -> check() || ab::p() -> buffs.empyrean_power -> check() ) ) ||
         ( ab::affected_by.divine_purpose_cost && ab::p() -> buffs.divine_purpose -> check() ) )
    {
      return 0.0;
    }

    double c = ab::cost();

    if ( ab::p() -> buffs.fires_of_justice -> check() )
    {
      c += ab::p() -> buffs.fires_of_justice -> data().effectN( 1 ).base_value();
    }

    if ( this -> affected_by.the_magistrates_judgment && ab::p() -> buffs.the_magistrates_judgment -> up() )
      c += ab::p() -> buffs.the_magistrates_judgment -> value();

    return std::max( c, 0.0 );
  }

  void execute() override
  {
    double hp_used = cost();
    //p variable just to make this look neater
    paladin_t* p = this -> p();

    ab::execute();

    // if this is a vanq-hammer-based DS, don't do this stuff
    if ( ab::background && is_divine_storm )
      return;

    // as of 11/8, according to Skeletor, crusade and RI trigger at full value now
    int num_stacks = as<int>( ab::base_costs[ RESOURCE_HOLY_POWER ] );

    // Royal Decree doesn't proc RP 2020-11-01
    if ( is_wog && p -> buffs.royal_decree -> up() &&
        !p -> buffs.divine_purpose -> up() && !p -> buffs.shining_light_free -> up() )
      num_stacks = as<int>( hp_used );

    // as of 2020-11-08 magistrate's causes *extra* stacks?
    // fixed at least for ret as of 9.0.5
    if ( p -> bugs && p -> buffs.the_magistrates_judgment -> up() )
    {
      if ( p -> specialization() == PALADIN_PROTECTION &&
          is_wog && !p -> buffs.divine_purpose -> up() &&
          ( p -> buffs.shining_light_free -> up() || p -> buffs.royal_decree -> up() ) )
        num_stacks += 1;
    }

    if ( p -> azerite.relentless_inquisitor.ok() )
      p -> buffs.relentless_inquisitor_azerite -> trigger( num_stacks );

    if ( p -> legendary.relentless_inquisitor -> ok() )
    {
      p -> buffs.relentless_inquisitor -> trigger();
      // 2020-12-06 Shining Light and Divine Purpose proc 2 stacks of Relentless Inquisitor
      if ( p -> bugs && ( p -> buffs.divine_purpose -> up() || ( is_wog && p -> buffs.shining_light_free -> up() )))
        p -> buffs.relentless_inquisitor -> trigger();
    }

    if ( p -> buffs.crusade -> check() )
    {
      p -> buffs.crusade -> trigger( num_stacks );
    }

    // Free sotr from vanq does not proc RP 2020-09-10
    if ( p -> talents.righteous_protector -> ok() && !ab::background )
    {
      timespan_t reduction = timespan_t::from_seconds(
        // Why is this in deciseconds?
         -1.0 * p -> talents.righteous_protector -> effectN( 1 ).base_value() / 10
       );
      reduction *= num_stacks;
      ab::sim -> print_debug( "Righteous protector reduced the cooldown of Avenging Wrath and Guardian of Ancient Kings by {} sec", num_stacks );
      p -> cooldowns.avenging_wrath -> adjust( reduction );
      p -> cooldowns.guardian_of_ancient_kings -> adjust( reduction );
    }

    // Consume Empyrean Power on Divine Storm, handled here for interaction with DP/FoJ
    // Cost reduction is still in divine_storm_t
    bool should_continue = true;
    if ( is_divine_storm && p -> bugs )
    {
      if ( p -> buffs.empyrean_power_azerite -> up() )
      {
        p -> buffs.empyrean_power_azerite -> expire();
        should_continue = false;
      }

      if ( p -> buffs.empyrean_power -> up() )
      {
        p -> buffs.empyrean_power -> expire();
        should_continue = false;
      }
    }
    else if ( is_divine_storm )
    {
      if ( p -> buffs.empyrean_power_azerite -> up() )
      {
        p -> buffs.empyrean_power_azerite -> expire();
        should_continue = false;
      }
      else if ( p -> buffs.empyrean_power -> up() )
      {
        p -> buffs.empyrean_power -> expire();
        should_continue = false;
      }
    }

    // 2020-11-01 Royal Decree always expires, even when other holy power reductions are up
    if ( is_wog && p -> buffs.royal_decree -> up() )
      p -> buffs.royal_decree -> expire();

    // For prot (2020-11-01). Magistrate's does not get consumed when DP is up.
    // For ret (2020-10-29), Magistrate's does not get consumed with DP or EP up but does
    // with FoJ.
    if ( this -> affected_by.the_magistrates_judgment && !p -> buffs.divine_purpose -> up() && should_continue )
      p -> buffs.the_magistrates_judgment -> decrement( 1 );

    if ( is_wog && p -> buffs.shining_light_free -> up() )
    {
      // Shining Light is now consumed before Divine Purpose 2020-11-01
      p -> buffs.shining_light_free -> expire();
      should_continue = false;
    }
    else if ( p -> bugs && p -> specialization() == PALADIN_PROTECTION &&
      ( ab::background || ( is_wog && p -> buffs.vanquishers_hammer -> up () ) ) )
    {
      // Vanquisher's Hammer's auto-sotr is not consuming divine purpose.
      // Vanquisher's Hammer's wog is not consuming divine purpose for some reason.
      should_continue = false;
    }

    // We should only have should_continue false in the event that we're a divine storm
    // assert-check here for safety
    assert( is_divine_storm || should_continue || p -> specialization() != PALADIN_RETRIBUTION );

    // Divine Purpose isn't consumed on DS if EP was consumed
    if ( should_continue )
    {
      if ( p -> buffs.divine_purpose -> up() )
      {
        p -> buffs.divine_purpose -> expire();
      }
      // FoJ isn't consumed if EP or DP were consumed
      else if ( p -> buffs.fires_of_justice -> up() )
      {
        p -> buffs.fires_of_justice -> expire();
      }
    }

    // Roll for Divine Purpose
    if ( p -> talents.divine_purpose -> ok() &&
         this -> rng().roll( p -> talents.divine_purpose -> effectN( 1 ).percent() )
      )
    {
      p -> buffs.divine_purpose -> trigger();
      p -> procs.divine_purpose -> occur();
    }

    if ( p -> buffs.avenging_wrath -> up() || p -> buffs.crusade -> up() )
    {
      if ( p -> azerite.lights_decree.ok() )
      {
        lights_decree_t* ld = debug_cast<lights_decree_t*>( p -> active.lights_decree );
        ld -> last_holy_power_cost = as<int>( ab::base_costs[ RESOURCE_HOLY_POWER ] );
        ld -> execute();
      }

      if ( p -> specialization() == PALADIN_RETRIBUTION && p -> talents.ret_sanctified_wrath -> ok() )
      {
        sanctified_wrath_t* st = debug_cast<sanctified_wrath_t*>( p -> active.sanctified_wrath );
        st -> last_holy_power_cost = as<int>( ab::base_costs[ RESOURCE_HOLY_POWER ] );
        st -> execute();
      }
    }
  }

  void consume_resource() override
  {
    ab::consume_resource();

    if ( ab::current_resource() == RESOURCE_HOLY_POWER)
    {
      ab::p() -> trigger_memory_of_lucid_dreams( ab::last_resource_cost );
    }
  }
};

struct judgment_t : public paladin_melee_attack_t
{
  int indomitable_justice_pct;
  judgment_t( paladin_t* p, util::string_view options_str );
  judgment_t( paladin_t* p );

  virtual double bonus_da( const action_state_t* s ) const override;
  proc_types proc_type() const override;
  void impact( action_state_t* s ) override;
  void execute() override;

private:
  void do_ctor_common( paladin_t* p );
};

struct shield_of_the_righteous_buff_t : public buff_t
{
  shield_of_the_righteous_buff_t( paladin_t* p );
  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override;
};

void empyrean_power( special_effect_t& effect );

}
