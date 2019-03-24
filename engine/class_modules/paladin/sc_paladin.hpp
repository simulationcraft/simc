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
    buff_t* debuffs_judgment;
    buff_t* judgment_of_light;
    buff_t* blessed_hammer_debuff;
  } buffs;

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
    return ( cd -> duration > timespan_t::zero() || cd_override > timespan_t::zero() )
        && ( ( cd -> charges == 1 && cd -> up() ) || ( cd -> charges >= 2 && cd -> current_charge == cd -> charges ) )
        && ( cd -> last_charged > timespan_t::zero() && cd -> last_charged < cd -> sim.current_time() );
  }

  virtual double get_wasted_time()
  {
    return (cd -> sim.current_time() - cd -> last_charged).total_seconds();
  }

  void add( timespan_t cd_override = timespan_t::min(), timespan_t time_to_execute = timespan_t::zero() )
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

  // Active
  heal_t*   active_beacon_of_light;
  action_t* active_shield_of_vengeance_proc;
  action_t* active_holy_shield_proc;
  action_t* active_judgment_of_light_proc;
  action_t* active_sotr;

  action_t* active_zeal;
  action_t* active_inner_light_damage;
  
  struct active_actions_t
  {
    blessing_of_sacrifice_redirect_t* blessing_of_sacrifice_redirect;
  } active;

  // Buffs
  struct buffs_t
  {
    // core
    buffs::avenging_wrath_buff_t* avenging_wrath;
    buff_t* avenging_wrath_crit;
    buffs::crusade_buff_t* crusade;
    buff_t* holy_avenger;
    buffs::shield_of_vengeance_buff_t* shield_of_vengeance;
    buff_t* divine_protection;
    buff_t* divine_shield;
    buff_t* guardian_of_ancient_kings;
    buff_t* grand_crusader;
    buff_t* infusion_of_light;
    buff_t* shield_of_the_righteous;
    buff_t* avengers_valor;
    buff_t* ardent_defender;

    // talents
    absorb_buff_t* holy_shield_absorb; // Dummy buff to trigger spell damage "blocking" absorb effect

    buff_t* the_fires_of_justice;
    buff_t* blade_of_wrath;
    buff_t* divine_purpose;
    buff_t* righteous_verdict;
    buff_t* zeal;
    buff_t* inquisition;
    buff_t* divine_judgment;

    buff_t* divine_steed;
    buff_t* aegis_of_light;
    stat_buff_t* seraphim;
    buff_t* redoubt;

    // azerite
    // Ret
    buff_t* avengers_might;
    buff_t* relentless_inquisitor;
    buff_t* empyrean_power;
    // Prot
    buff_t* inspiring_vanguard;
    buff_t* inner_light;
    buff_t* soaring_shield;
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
    cooldown_t* avengers_shield;         // Grand Crusader (prot)
    cooldown_t* shield_of_the_righteous; // Judgment (prot)
    cooldown_t* avenging_wrath;          // Righteous Protector (prot)
    cooldown_t* light_of_the_protector;  // Righteous Protector (prot) / Saruin
    cooldown_t* hand_of_the_protector;   // Righteous Protector (prot) / Saruin
    cooldown_t* judgment;         // Grand Crusader + Crusader's Judgment
    cooldown_t* holy_shock; // Holy Shock for Crusader's Might && DP
    cooldown_t* light_of_dawn; // Light of Dawn for DP
    cooldown_t* inner_light;

    // whoo fist of justice
    cooldown_t* hammer_of_justice;

    cooldown_t* blade_of_justice;
    cooldown_t* divine_hammer;
    cooldown_t* consecration;
  } cooldowns;

  // Passives
  struct passives_t
  {
    const spell_data_t* boundless_conviction;
    const spell_data_t* divine_bulwark;
    const spell_data_t* grand_crusader;
    const spell_data_t* hand_of_light;
    const spell_data_t* infusion_of_light;
    const spell_data_t* lightbringer;
    const spell_data_t* plate_specialization;
    const spell_data_t* riposte;   // hidden
    const spell_data_t* paladin;
    const spell_data_t* sanctuary;
    const spell_data_t* avengers_valor;

    const spell_data_t* judgment; // mystery, hidden
    const spell_data_t* execution_sentence;
  } passives;

  // Procs and RNG
  real_ppm_t* art_of_war_rppm;
  struct procs_t
  {
    proc_t* defender_of_the_light;

    proc_t* divine_purpose;
    proc_t* the_fires_of_justice;
    proc_t* art_of_war;
    proc_t* grand_crusader;
  } procs;

  // Spells
  struct spells_t
  {
    const spell_data_t* divine_purpose_ret;
    const spell_data_t* divine_purpose_holy;
    const spell_data_t* avenging_wrath;
    const spell_data_t* shield_of_the_righteous;
    const spell_data_t* blade_of_wrath;
    const spell_data_t* lights_decree;
    const spell_data_t* avenging_wrath_crit;
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
    // const spell_data_t* divine_purpose;
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
    const spell_data_t* redoubt;
    // skip T45
    const spell_data_t* blessing_of_spellwarding;
    const spell_data_t* blessing_of_salvation;
    const spell_data_t* retribution_aura;
    const spell_data_t* hand_of_the_protector;
    const spell_data_t* final_stand;
    // skip unbreakable spirit
    const spell_data_t* aegis_of_light;
    // Judgment of Light seems to be a recopy from Holy.
    //const spell_data_t* judgment_of_light;
    const spell_data_t* consecrated_ground;
    const spell_data_t* righteous_protector;
    const spell_data_t* seraphim;
    const spell_data_t* last_defender;

    // Retribution
    const spell_data_t* zeal;
    const spell_data_t* righteous_verdict;
    const spell_data_t* execution_sentence;
    const spell_data_t* fires_of_justice;
    const spell_data_t* blade_of_wrath;
    const spell_data_t* hammer_of_wrath;
    const spell_data_t* fist_of_justice;
    const spell_data_t* repentance;
    const spell_data_t* blinding_light;
    const spell_data_t* divine_judgment;
    const spell_data_t* consecration;
    const spell_data_t* wake_of_ashes;
    // skip cavalier
    // skip unbreakable spirit
    const spell_data_t* eye_for_an_eye;
    // skip JoL
    const spell_data_t* word_of_glory;
    const spell_data_t* justicars_vengeance;
    const spell_data_t* divine_purpose;
    const spell_data_t* crusade;
    const spell_data_t* inquisition;
  } talents;

  struct azerite_t
  {
    // shared
    azerite_power_t indomitable_justice;

    // holy
    // protection
    azerite_power_t inspiring_vanguard;
    azerite_power_t inner_light;
    azerite_power_t soaring_shield;

    // retribution
    azerite_power_t avengers_might;
    azerite_power_t expurgation;
    azerite_power_t grace_of_the_justicar; // healing
    azerite_power_t relentless_inquisitor;
    azerite_power_t empyrean_power;
    azerite_power_t lights_decree;
  } azerite;

  player_t* beacon_target;
  timespan_t last_jol_proc;

  bool fake_sov;
  int indomitable_justice_pct;

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
  virtual expr_t*   create_expression( const std::string& name ) override;

  // player stat functions
  virtual double    composite_attribute_multiplier( attribute_e attr ) const override;
  virtual double    composite_attack_power_multiplier() const override;
  virtual double    composite_bonus_armor() const override;
  virtual double    composite_melee_attack_power() const override;
  virtual double    composite_melee_attack_power( attack_power_e ap_type ) const override;
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

  // combat outcome functions
  virtual void      assess_damage( school_e, dmg_e, action_state_t* ) override;
  virtual void      target_mitigation( school_e, dmg_e, action_state_t* ) override;

  virtual void      invalidate_cache( cache_e ) override;
  virtual void      create_options() override;
  virtual double    matching_gear_multiplier( attribute_e attr ) const override;
  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override;
  virtual resource_e primary_resource() const override;
  virtual role_e    primary_role() const override;
  virtual stat_e    convert_hybrid_stat( stat_e s ) const override;
  virtual void      combat_begin() override;
  virtual void      copy_from( player_t* ) override;

  bool    get_how_availability( player_t* t ) const;
  void    trigger_grand_crusader();
  void    trigger_holy_shield( action_state_t* s );
  void    trigger_forbearance( player_t* target );
  int     get_local_enemies( double distance ) const;
  bool    standing_in_consecration() const;
  double  last_defender_damage() const;
  double  last_defender_mitigation() const;

  expr_t*   create_consecration_expression( const std::string& expr_str );

  ground_aoe_event_t* active_consecration;

  std::string default_potion() const override;
  std::string default_flask() const override;
  std::string default_food() const override;
  std::string default_rune() const override;

  void      create_buffs_retribution();
  void      init_rng_retribution();
  void      init_spells_retribution();
  void      generate_action_prio_list_ret();
  action_t* create_action_retribution( const std::string& name, const std::string& options_str );

  void      create_buffs_protection();
  void      init_spells_protection();
  action_t* create_action_protection( const std::string& name, const std::string& options_str );

  void      create_buffs_holy();
  void      init_spells_holy();
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
    avenging_wrath_buff_t( player_t* p );

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

  struct sephuzs_secret_buff_t : public buff_t
  {
    cooldown_t* icd;
    sephuzs_secret_buff_t(paladin_t* p) :
      buff_t(p, "sephuzs_secret", p -> find_spell( 208052 ))
    {
      set_default_value(p -> find_spell( 208052 ) -> effectN(2).percent());
      add_invalidate( CACHE_HASTE );
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

  // haste scaling bools
  bool hasted_cd;
  bool hasted_gcd;
  bool ret_damage_increase;
  bool ret_dot_increase;
  bool ret_damage_increase_two;
  bool ret_damage_increase_three;
  bool ret_damage_increase_four;
  bool ret_mastery_direct;
  bool ret_execution_sentence;
  bool ret_inquisition;
  bool ret_crusade;
  bool avenging_wrath;
  bool last_defender_increase;
  bool avenging_wrath_crit;

  paladin_action_t( const std::string& n, paladin_t* player,
                    const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s ),
    track_cd_waste( s -> cooldown() > timespan_t::zero() || s -> charge_cooldown() > timespan_t::zero() ),
    cd_waste( nullptr ), cd_waste_factory( nullptr ),
    ret_damage_increase( ab::data().affected_by( player -> spec.retribution_paladin -> effectN( 1 ) ) ),
    ret_dot_increase( ab::data().affected_by( player -> spec.retribution_paladin -> effectN( 2 ) ) ),
    ret_damage_increase_two( ab::data().affected_by( player -> spec.retribution_paladin -> effectN( 11 ) ) ),
    ret_damage_increase_three( ab::data().affected_by( player -> spec.retribution_paladin -> effectN( 12 ) ) ),
    ret_damage_increase_four( ab::data().affected_by( player -> spec.retribution_paladin -> effectN( 13 ) ) ),
    ret_mastery_direct( ab::data().affected_by( player -> passives.hand_of_light -> effectN( 1 ) ) || ab::data().affected_by( player -> passives.hand_of_light -> effectN( 2 ) ) ),
    last_defender_increase( ab::data().affected_by( player -> talents.last_defender -> effectN( 5 ) ) )
  {
    // Aura buff to protection paladin added in 7.3
    if ( p() -> specialization() == PALADIN_PROTECTION && this -> data().affected_by( p() -> spec.protection_paladin -> effectN( 1 ) ) )
    {
      this -> base_dd_multiplier *= 1.0 + p() -> spec.protection_paladin -> effectN( 1 ).percent();
    }
    if ( p() -> specialization() == PALADIN_PROTECTION && this -> data().affected_by( p() -> spec.protection_paladin -> effectN( 2 ) ) )
    {
      this -> base_td_multiplier *= 1.0 + p() -> spec.protection_paladin -> effectN( 2 ).percent();
    }


    if ( p() -> specialization() == PALADIN_RETRIBUTION ) {
      ret_execution_sentence = ab::data().affected_by( p() -> passives.execution_sentence -> effectN( 1 ) );
      ret_inquisition = ab::data().affected_by( p() -> talents.inquisition -> effectN( 1 ) );
      ret_crusade = ab::data().affected_by( p() -> talents.crusade -> effectN( 1 ) );
    } else {
      ret_execution_sentence = false;
      ret_inquisition = false;
      ret_crusade = false;
    }
    avenging_wrath = ab::data().affected_by( player -> spells.avenging_wrath -> effectN( 1 ) );
    avenging_wrath_crit = ab::data().affected_by( player -> spells.avenging_wrath_crit -> effectN( 1 ) );

    hasted_cd = hasted_gcd = false;
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
    update_hasted_cooldowns_by_passive( player -> passives.paladin );
    if ( p() -> specialization() == PALADIN_RETRIBUTION ) {
      update_hasted_cooldowns_by_passive( player -> spec.retribution_paladin );
    } else if ( p() -> specialization() == PALADIN_PROTECTION ) {
      update_hasted_cooldowns_by_passive( player -> spec.protection_paladin );
    } else {
      update_hasted_cooldowns_by_passive( player -> spec.holy_paladin );
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
      ab::base_dd_multiplier *= 1.0 + p() -> spec.retribution_paladin -> effectN( 11 ).percent();
    if ( ret_damage_increase_three )
      ab::base_dd_multiplier *= 1.0 + p() -> spec.retribution_paladin -> effectN( 12 ).percent();
    if ( ret_damage_increase_four )
      ab::base_dd_multiplier *= 1.0 + p() -> spec.retribution_paladin -> effectN( 13 ).percent();
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

    if ( avenging_wrath_crit )
    {
      p() -> buffs.avenging_wrath_crit -> expire();
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

  virtual double action_multiplier() const override
  {
    double am = ab::action_multiplier();
    if ( p() -> specialization() == PALADIN_RETRIBUTION ) {
      if ( ret_mastery_direct )
        am *= 1.0 + p() -> cache.mastery_value();
      if ( ret_inquisition ) {
        if ( p() -> buffs.inquisition -> up() )
          am *= 1.0 + p() -> buffs.inquisition -> data().effectN( 1 ).percent();
      }
      if ( ret_crusade ) {
        if ( p() -> buffs.crusade -> check() ) {
          am *= 1.0 + p() -> buffs.crusade -> get_damage_mod();
        }
      }
    }

    if ( avenging_wrath ) {
      if ( p() -> buffs.avenging_wrath -> check() ) {
        am *= 1.0 + p() -> buffs.avenging_wrath -> get_damage_mod();
      }
    }

    if ( p() -> talents.last_defender -> ok() && last_defender_increase )
    {
      am *= p() -> last_defender_damage();
    }

    return am;
  }

  double composite_crit_chance() const override
  {
    double cc = ab::composite_crit_chance();

    if ( avenging_wrath_crit && p() -> buffs.avenging_wrath_crit -> up() )
    {
      return 1.0;
    }

    if ( avenging_wrath && p() -> buffs.avenging_wrath -> check() )
    {
      cc += p() -> buffs.avenging_wrath -> get_crit_bonus();
    }

    return cc;
  }

  virtual double composite_target_multiplier( player_t* t ) const override
  {
    double ctm = ab::composite_target_multiplier( t );
    if ( ret_execution_sentence )
    {
      if ( td( t ) -> buffs.execution_sentence -> up() )
        ctm *= 1.0 + p() -> passives.execution_sentence -> effectN( 1 ).percent();
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
  {
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

    assert( p() -> active_beacon_of_light );

    p() -> active_beacon_of_light -> target = p() -> beacon_target;

    double amount = s -> result_amount;
    amount *= p() -> beacon_target -> buffs.beacon_of_light -> data().effectN( 1 ).percent();

    p() -> active_beacon_of_light -> base_dd_min = amount;
    p() -> active_beacon_of_light -> base_dd_max = amount;

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
