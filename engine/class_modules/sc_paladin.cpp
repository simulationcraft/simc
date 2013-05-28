// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
/*
  To Do:
  Add healing holy prism, aoe healing holy prism, stay of execution, light's hammer healing
*/
#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE

// ==========================================================================
// Paladin
// ==========================================================================

struct paladin_t;

enum seal_e
{
  SEAL_NONE=0,
  SEAL_OF_JUSTICE,
  SEAL_OF_INSIGHT,
  SEAL_OF_RIGHTEOUSNESS,
  SEAL_OF_TRUTH,
  SEAL_MAX
};

struct paladin_td_t : public actor_pair_t
{
  struct dots_t
  {
    dot_t* word_of_glory;
    dot_t* holy_radiance;
    dot_t* censure;
    dot_t* execution_sentence;
  } dots;

  buff_t* debuffs_censure;

  paladin_td_t( player_t* target, paladin_t* paladin );
};

struct paladin_t : public player_t
{
public:

  // Active
  seal_e    active_seal;
  heal_t*   active_beacon_of_light;
  heal_t*   active_enlightened_judgments;
  action_t* active_hand_of_light_proc;
  absorb_t* active_illuminated_healing;
  heal_t*   active_protector_of_the_innocent;
  action_t* active_seal_of_insight_proc;
  action_t* active_seal_of_justice_proc;
  action_t* active_seal_of_justice_aoe_proc;
  action_t* active_seal_of_righteousness_proc;
  action_t* active_seal_of_truth_dot;
  action_t* active_seal_of_truth_proc;
  action_t* ancient_fury_explosion;
  player_t* last_judgement_target;

  // Buffs
  struct buffs_t
  {
    buff_t* ancient_power;
    buff_t* ardent_defender;
    buff_t* avenging_wrath;
    buff_t* blessed_life;
    buff_t* daybreak;
    buff_t* divine_plea;
    buff_t* divine_protection;
    buff_t* divine_purpose;
    buff_t* divine_shield;
    buff_t* double_jeopardy;
    buff_t* gotak_prot;
    buff_t* grand_crusader;
    buff_t* glyph_templars_verdict;
    buff_t* holy_avenger;
    buff_t* infusion_of_light;
    buff_t* inquisition;
    buff_t* judgments_of_the_wise;
    buff_t* glyph_of_word_of_glory;
    buff_t* tier15_2pc_melee;
    buff_t* tier15_4pc_melee;
    buff_t* shield_of_glory;
    buff_t* shield_of_the_righteous;
  } buffs;

  // Gains
  struct gains_t
  {
    gain_t* divine_plea;
    gain_t* extra_regen;
    gain_t* judgments_of_the_wise;
    gain_t* sanctuary;
    gain_t* seal_of_insight;
    gain_t* glyph_divine_storm;

    // Holy Power
    gain_t* hp_blessed_life;
    gain_t* hp_crusader_strike;
    gain_t* hp_divine_plea;
    gain_t* hp_divine_purpose;
    gain_t* hp_exorcism;
    gain_t* hp_grand_crusader;
    gain_t* hp_hammer_of_the_righteous;
    gain_t* hp_hammer_of_wrath;
    gain_t* hp_holy_avenger;
    gain_t* hp_holy_shock;
    gain_t* hp_judgments_of_the_bold;
    gain_t* hp_judgments_of_the_wise;
    gain_t* hp_pursuit_of_justice;
    gain_t* hp_tower_of_radiance;
    gain_t* hp_judgment;
    //todo: add divine protection (T15 prot 4pc)
  } gains;

  // Cooldowns
  struct cooldowns_t
  {
    // these seem to be required to get Art of War and Grand Crusader procs working
    cooldown_t* avengers_shield;
    cooldown_t* exorcism;
  } cooldowns;

  // Passives
  struct passives_t
  {
    const spell_data_t* ancient_fury;
    const spell_data_t* ancient_power;
    const spell_data_t* boundless_conviction;
    const spell_data_t* divine_bulwark;
    const spell_data_t* grand_crusader;
    const spell_data_t* guarded_by_the_light;
    const spell_data_t* hand_of_light;
    const spell_data_t* illuminated_healing;
    const spell_data_t* judgments_of_the_bold;
    const spell_data_t* judgments_of_the_wise;
    const spell_data_t* plate_specialization;
    const spell_data_t* sanctity_of_battle;
    const spell_data_t* sanctuary;
    const spell_data_t* seal_of_insight;
    const spell_data_t* sword_of_light;
    const spell_data_t* sword_of_light_value;
    const spell_data_t* the_art_of_war;
    const spell_data_t* vengeance;
  } passives;

  // Pets
  pet_t* guardian_of_ancient_kings;

  // Procs
  struct procs_t
  {
    proc_t* eternal_glory;
    proc_t* judgments_of_the_bold;
    proc_t* parry_haste;
    proc_t* the_art_of_war;
    proc_t* wasted_art_of_war;
  } procs;

  // Spells
  struct spells_t
  {
    const spell_data_t* guardian_of_ancient_kings_ret;
    const spell_data_t* holy_light;
    const spell_data_t* glyph_of_word_of_glory_damage;
  } spells;

  // Talents
  struct talents_t
  {
    const spell_data_t* divine_purpose;
    //todo: Sacred Shield, Holy Avenger, Sanctified Wrath
  } talents;

  // Glyphs
  struct glyphs_t
  {
    const spell_data_t* blessed_life;
    const spell_data_t* divine_protection;
    const spell_data_t* divine_storm;
    const spell_data_t* double_jeopardy;
    const spell_data_t* focused_shield;
    const spell_data_t* harsh_words;
    const spell_data_t* immediate_truth;
    const spell_data_t* inquisition;
    const spell_data_t* mass_exorcism;
    const spell_data_t* templars_verdict;
    const spell_data_t* word_of_glory;
    //todo: battle healer, alabaster shield, focused wrath, final wrath
  } glyphs;

  player_t* beacon_target;
  int ret_pvp_gloves;

  bool bok_up;
  bool bom_up;
  timespan_t last_extra_regen;
  timespan_t extra_regen_period;
  double extra_regen_percent;

  paladin_t( sim_t* sim, const std::string& name, race_e r = RACE_TAUREN ) :
    player_t( sim, PALADIN, name, r ),
    last_judgement_target(),
    buffs( buffs_t() ),
    gains( gains_t() ),
    cooldowns( cooldowns_t() ),
    passives( passives_t() ),
    guardian_of_ancient_kings( nullptr ),
    procs( procs_t() ),
    spells( spells_t() ),
    talents( talents_t() ),
    glyphs( glyphs_t() ),
    beacon_target( nullptr ),
    ret_pvp_gloves( 0 ),
    bok_up( false ),
    bom_up( false ),
    last_extra_regen( timespan_t::from_seconds( 0.0 ) ),
    extra_regen_period( timespan_t::from_seconds( 0.0 ) ),
    extra_regen_percent( 0.0 )
  {
    active_beacon_of_light             = 0;
    active_enlightened_judgments       = 0;
    active_hand_of_light_proc          = 0;
    active_illuminated_healing         = 0;
    active_protector_of_the_innocent   = 0;
    active_seal                        = SEAL_NONE;
    active_seal_of_justice_proc        = 0;
    active_seal_of_justice_aoe_proc    = 0;
    active_seal_of_insight_proc        = 0;
    active_seal_of_righteousness_proc  = 0;
    active_seal_of_truth_proc          = 0;
    active_seal_of_truth_dot           = 0;
    ancient_fury_explosion             = 0;
    bok_up                             = false;
    bom_up                             = false;

    cooldowns.avengers_shield = get_cooldown( "avengers_shield" );
    cooldowns.exorcism = get_cooldown( "exorcism" );
    
    beacon_target = 0;

    base.distance = ( specialization() == PALADIN_HOLY ) ? 30 : 3;
  }

  virtual void      init_defense();
  virtual void      init_base_stats();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_scaling();
  virtual void      create_buffs();
  virtual void      init_spells();
  virtual void      init_actions();
  virtual void      reset();
  virtual expr_t*   create_expression( action_t*, const std::string& name );
  virtual double    composite_attribute_multiplier( attribute_e attr );
  virtual double    composite_melee_crit();
  virtual double    composite_spell_crit();
  virtual double    composite_player_multiplier( school_e school );
  virtual double    composite_spell_power( school_e school );
  virtual double    composite_spell_power_multiplier();
  virtual double    composite_spell_speed();
  virtual double    composite_block();
  virtual double    composite_crit_avoidance();
  virtual double    composite_dodge();
  virtual void      assess_damage( school_e, dmg_e, action_state_t* );
  virtual void      target_mitigation( school_e, dmg_e, action_state_t* );
  virtual void      invalidate_cache( cache_e );
  virtual void      create_options();
  virtual double    matching_gear_multiplier( attribute_e attr );
  virtual action_t* create_action( const std::string& name, const std::string& options_str );
  virtual int       decode_set( item_t& );
  virtual resource_e primary_resource() { return RESOURCE_MANA; }
  virtual role_e primary_role();
  virtual void      regen( timespan_t periodicity );
  virtual pet_t*    create_pet    ( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets   ();
  virtual void      combat_begin();

  int     holy_power_stacks();
  double  get_divine_bulwark();
  double  get_hand_of_light();
  double  jotp_haste();
  void    trigger_grand_crusader();
  void    generate_action_prio_list_prot();
  void    generate_action_prio_list_ret();

  target_specific_t<paladin_td_t*> target_data;

  virtual paladin_td_t* get_target_data( player_t* target )
  {
    paladin_td_t*& td = target_data[ target ];
    if ( ! td )
    {
      td = new paladin_td_t( target, this );
    }
    return td;
  }
};

namespace pets {

// Guardian of Ancient Kings Pet ============================================

// TODO: melee attack
struct guardian_of_ancient_kings_ret_t : public pet_t
{
  melee_attack_t* melee;

  struct melee_t : public melee_attack_t
  {
    paladin_t* owner;

    melee_t( guardian_of_ancient_kings_ret_t* p )
      : melee_attack_t( "melee", p, spell_data_t::nil() ), owner( 0 )
    {
      school = SCHOOL_PHYSICAL;
      weapon = &( p -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      background = true;
      repeating  = true;
      may_crit = true;
      trigger_gcd = timespan_t::zero();
      owner = p -> o();
    }

    virtual void execute()
    {
      melee_attack_t::execute();
      if ( result_is_hit( execute_state -> result ) )
      {
        owner -> buffs.ancient_power -> trigger();
      }
    }
  };

  guardian_of_ancient_kings_ret_t( sim_t *sim, paladin_t *p )
    : pet_t( sim, p, "guardian_of_ancient_kings", true ), melee( 0 )
  {
    main_hand_weapon.type = WEAPON_BEAST;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
    main_hand_weapon.min_dmg = dbc.spell_scaling( o() -> type, level ) * 6.1;
    main_hand_weapon.max_dmg = dbc.spell_scaling( o() -> type, level ) * 6.1;
    main_hand_weapon.damage  = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    owner_coeff.ap_from_ap = 6.1;
  }

  paladin_t* o()
  { return debug_cast<paladin_t*>( owner ); }

  virtual void init_base_stats()
  {
    pet_t::init_base_stats();
    melee = new melee_t( this );
  }

  virtual void dismiss()
  {
    // Only trigger the explosion if we're not sleeping
    if ( is_sleeping() ) return;

    pet_t::dismiss();

    if ( o() -> ancient_fury_explosion )
      o() -> ancient_fury_explosion -> execute();
  }

  virtual void arise()
  {
    pet_t::arise();
    schedule_ready();
  }

  virtual void schedule_ready( timespan_t delta_time=timespan_t::zero(), bool waiting=false )
  {
    pet_t::schedule_ready( delta_time, waiting );
    if ( ! melee -> execute_event ) melee -> execute();
  }
};

} // end namespace pets

// Template for common paladin action code. See priest_action_t.
template <class Base>
struct paladin_action_t : public Base
{
private:
  typedef Base ab; // action base, eg. spell_t
public:
  typedef paladin_action_t base_t;

  paladin_action_t( const std::string& n, paladin_t* player,
                    const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s )
  {
  }

  paladin_t* p() const { return static_cast<paladin_t*>( ab::player ); }

  paladin_td_t* td( player_t* t = 0 ) { return p() -> get_target_data( t ? t : ab::target ); }

  virtual double cost()
  {
    if ( ab::current_resource() == RESOURCE_HOLY_POWER )
    {
      if ( p() -> buffs.divine_purpose -> check() )
      {
        return 0.0;
      }
      return std::max( ab::base_costs[ RESOURCE_HOLY_POWER ], std::min( 3.0, p() -> resources.current[ RESOURCE_HOLY_POWER ] ) );
    }

    return ab::cost();
  }

  // trigger_hand_of_light ====================================================

  void trigger_hand_of_light( action_state_t* s )
  {
    if ( p() -> passives.hand_of_light -> ok() )
    {
      p() -> active_hand_of_light_proc -> base_dd_max = p() -> active_hand_of_light_proc-> base_dd_min = s -> result_amount;
      p() -> active_hand_of_light_proc -> execute();
    }
  }

};

namespace attacks {

// ==========================================================================
// Paladin Attacks
// ==========================================================================

struct paladin_melee_attack_t : public paladin_action_t< melee_attack_t >
{
  // booleans to allow individual control of seal proc triggers
  bool trigger_seal;
  bool trigger_seal_of_righteousness;
  bool trigger_seal_of_justice;
  bool trigger_seal_of_truth;
  bool trigger_seal_of_justice_aoe; // probably not needed anymore, this change was reverted

  // spell cooldown reduction flags
  bool sanctity_of_battle;  //separated from use_spell_haste because sanctity is now on melee haste
  bool use_spell_haste; // Some attacks (censure) use spell haste. sigh.

  bool use2hspec;

  paladin_melee_attack_t( const std::string& n, paladin_t* p,
                          const spell_data_t* s = spell_data_t::nil(),
                          bool u2h = true ) :
    base_t( n, p, s ),
    trigger_seal( false ),
    trigger_seal_of_righteousness( false ),
    trigger_seal_of_justice( false ),
    trigger_seal_of_truth( false ),
    trigger_seal_of_justice_aoe( false ),
    sanctity_of_battle( false ),
    use_spell_haste( false ),
    use2hspec( u2h )
  {
    may_crit = true;
    special = true;
  }

  virtual double composite_haste()
  {
    return use_spell_haste ? p() -> cache.spell_speed() : base_t::composite_haste();
  }

  virtual timespan_t gcd()
  {
    if ( use_spell_haste )
    {
      timespan_t t = action_t::gcd();
      if ( t == timespan_t::zero() ) return timespan_t::zero();

      t *= composite_haste();
      if ( t < min_gcd ) t = min_gcd;

      return t;
    }
    else if ( sanctity_of_battle )
    {
      timespan_t t = action_t::gcd();
      if ( t == timespan_t::zero() ) return timespan_t::zero();

      t *= p() -> cache.attack_haste();
      if ( t < min_gcd ) t = min_gcd;

      return t;
    }
    else
      return base_t::gcd();
  }

  virtual void execute()
  {
    double c = ( current_resource() == RESOURCE_HOLY_POWER ) ? cost() : -1.0;

    base_t::execute();

    if ( p() -> talents.divine_purpose -> ok() )
    {
      if ( c > 0.0 )
      {
        p() -> buffs.divine_purpose -> trigger();
      }
      else if ( c == 0.0 )
      {
        p() -> buffs.divine_purpose -> expire();
        //p() -> resource_gain( RESOURCE_HOLY_POWER, 3, p() -> gains.hp_divine_purpose );
        p() -> buffs.divine_purpose -> trigger();
      }
    }

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( ! p() -> guardian_of_ancient_kings -> is_sleeping() )
      {
        p() -> buffs.ancient_power -> trigger();
      }

      if ( trigger_seal || ( trigger_seal_of_righteousness && ( p() -> active_seal == SEAL_OF_RIGHTEOUSNESS ) )
                        || ( trigger_seal_of_justice && ( p() -> active_seal == SEAL_OF_JUSTICE ) ) 
                        || ( trigger_seal_of_truth && ( p() -> active_seal == SEAL_OF_TRUTH ) ) )
      {
        switch ( p() -> active_seal )
        {
        case SEAL_OF_JUSTICE:
          p() -> active_seal_of_justice_proc       -> execute();
          break;
        case SEAL_OF_INSIGHT:
          p() -> active_seal_of_insight_proc       -> execute();
          break;
        case SEAL_OF_RIGHTEOUSNESS:
          p() -> active_seal_of_righteousness_proc -> execute();
          break;
        case SEAL_OF_TRUTH:
          p() -> active_seal_of_truth_dot -> execute();
          if ( td() -> debuffs_censure -> stack() >= 1 ) p() -> active_seal_of_truth_proc -> execute();
          break;
        default: break;
        }
      }
      if ( trigger_seal_of_justice_aoe && ( p() -> active_seal == SEAL_OF_JUSTICE ) )
        p() -> active_seal_of_justice_aoe_proc -> execute();

    }
  }

  virtual double action_multiplier()
  {
    double am = base_t::action_multiplier();

    if ( use2hspec && ( p() -> passives.sword_of_light -> ok() ) && ( p() -> main_hand_weapon.group() == WEAPON_2H ) )
    {
      am *= 1.0 + p() -> passives.sword_of_light_value -> effectN( 1 ).percent();
    }

    return am;
  }
};

// Melee Attack =============================================================

struct melee_t : public paladin_melee_attack_t
{
  melee_t( paladin_t* p ) :
    paladin_melee_attack_t( "melee", p, spell_data_t::nil(), true )
  {
    school = SCHOOL_PHYSICAL;
    special           = false;
    trigger_seal      = true;
    background        = true;
    repeating         = true;
    trigger_gcd       = timespan_t::zero();
    weapon            = &( p -> main_hand_weapon );
    base_execute_time = p -> main_hand_weapon.swing_time;
  }

  virtual timespan_t execute_time()
  {
    if ( ! player -> in_combat ) return timespan_t::from_seconds( 0.01 );
    return paladin_melee_attack_t::execute_time();
  }

  virtual void execute()
  {
    paladin_melee_attack_t::execute();
    if ( result_is_hit( execute_state -> result ) )
    {
      // Check for Art of War procs
      if ( p() -> passives.the_art_of_war -> ok() && sim -> roll( p() -> passives.the_art_of_war -> proc_chance() ) )
      {
        // if Exorcism was already off-cooldown, count the proc as wasted
        if ( p() -> cooldowns.exorcism -> remains() <= timespan_t::zero() )
        {
          p() -> procs.wasted_art_of_war -> occur();
        }
        // trigger proc, reset Exorcism cooldown
        p() -> procs.the_art_of_war -> occur();
        p() -> cooldowns.exorcism -> reset( true );
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

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    p() -> main_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    if ( p() -> is_moving() ) return false;
    return( p() -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Crusader Strike ==========================================================

struct crusader_strike_t : public paladin_melee_attack_t
{
  const spell_data_t* sword_of_light;

  crusader_strike_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "crusader_strike", p, p -> find_class_spell( "Crusader Strike" ), true ),
      sword_of_light( p -> find_specialization_spell( "Sword of Light" ) )
  {
    parse_options( NULL, options_str );

    // CS triggers all seals
    trigger_seal = true;

    // Sanctity of Battle reduces melee GCD
    sanctity_of_battle = p -> passives.sanctity_of_battle -> ok();

    // multiplier modification for T13 Retribution 2-piece bonus
    base_multiplier *= 1.0 + ( ( p -> set_bonus.tier13_2pc_melee() ) ? p -> sets -> set( SET_T13_2PC_MELEE ) -> effectN( 1 ).percent() : 0.0 );

    // Guarded by the Light and Sword of Light reduce base mana cost; spec-limited so only one will ever be active
    base_costs[ RESOURCE_MANA ] *= 1.0 +  p -> passives.guarded_by_the_light -> effectN( 7 ).percent()
                                       +  p -> passives.sword_of_light -> effectN( 4 ).percent();
    base_costs[ RESOURCE_MANA ] = floor( base_costs[ RESOURCE_MANA ] + 0.5 );
  }

  virtual double action_multiplier()
  {
    double am = paladin_melee_attack_t::action_multiplier();

    // Holy Avenger buffs CS damage by 30% while active
    if ( p() -> buffs.holy_avenger -> check() )
    {
      am *= 1.0 + p() -> buffs.holy_avenger -> data().effectN( 4 ).percent();
    }

    return am;
  }
  
  virtual void update_ready( timespan_t cd_duration )
  {
    // reduce cooldown if Sanctity of Battle present
    if ( p() -> passives.sanctity_of_battle -> ok() )
    {
      cd_duration = cooldown -> duration * p() -> cache.attack_haste();
    }
    
    paladin_melee_attack_t::update_ready( cd_duration );
  }

  virtual void impact( action_state_t* s )
  {
    paladin_melee_attack_t::impact( s );

    // Special things that happen when CS connects
    if ( result_is_hit( s -> result ) )
    {
      // Holy Power gains, only relevant if CS connects
      int g = 1; // default is a gain of 1 Holy Power
      p() -> resource_gain( RESOURCE_HOLY_POWER, g, p() -> gains.hp_crusader_strike ); // apply gain, record as due to CS

      // If Holy Avenger active, grant 2 more
      if ( p() -> buffs.holy_avenger -> check() )
      {
        //apply gain, record as due to Holy Avenger
        p() -> resource_gain( RESOURCE_HOLY_POWER, p() -> buffs.holy_avenger -> value() - g, p() -> gains.hp_holy_avenger );
      }

      // Trigger Hand of Light procs
      trigger_hand_of_light( s );

      // Trigger Grand Crusader procs
      p() -> trigger_grand_crusader();

      // Check for T15 Ret 4-piece bonus proc
      if ( p() -> set_bonus.tier15_4pc_melee() )
        p() -> buffs.tier15_4pc_melee -> trigger();

    }
  }
};

// Divine Storm =============================================================

struct divine_storm_t : public paladin_melee_attack_t
{
  double heal_percentage;

  divine_storm_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "divine_storm", p, p -> find_class_spell( "Divine Storm" ) ), heal_percentage( 0.0 )
  {
    parse_options( NULL, options_str );

    weapon = &( p -> main_hand_weapon );

    aoe                = -1;
    trigger_seal       = false;
    sanctity_of_battle = true;
    trigger_seal_of_righteousness = true;

    if ( p -> glyphs.divine_storm -> ok() )
    {
      heal_percentage = p -> find_spell( 115515 ) -> effectN( 1 ).percent();
    }
  }

  virtual void impact( action_state_t* s )
  {
    paladin_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      trigger_hand_of_light( s );
      if ( p() -> glyphs.divine_storm -> ok() )
      {
        p() -> resource_gain( RESOURCE_HEALTH, heal_percentage * p() -> resources.max[ RESOURCE_HEALTH ], p() -> gains.glyph_divine_storm, this );
      }
    }
  }
};

// Hammer of Justice ========================================================

struct hammer_of_justice_t : public paladin_melee_attack_t
{
  hammer_of_justice_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "hammer_of_justice", p, p -> find_class_spell( "Hammer of Justice" ) )
  {
    parse_options( NULL, options_str );
  }
};

// Fist of Justice ========================================================

struct fist_of_justice_t : public paladin_melee_attack_t
{
  fist_of_justice_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "fist_of_justice", p, p -> find_talent_spell( "Fist of Justice" ) )
  {
    parse_options( NULL, options_str );
  }
};


// Hammer of the Righteous ==================================================

struct hammer_of_the_righteous_aoe_t : public paladin_melee_attack_t
{
  hammer_of_the_righteous_aoe_t( paladin_t* p )
    : paladin_melee_attack_t( "hammer_of_the_righteous_aoe", p, p -> find_spell( 88263 ), false )
  {
    // AoE effect always hits if single-target attack succeeds
    // Doesn't proc seals or Grand Crusader
    may_dodge = false;
    may_parry = false;
    may_miss  = false;
    background = true;
    aoe       = -1;
    trigger_gcd = timespan_t::zero(); // doesn't incur GCD (HotR does that already)

    // Non-weapon-based AP/SP scaling, zero now, but has been non-zero in past iterations
    direct_power_mod = data().extra_coeff();
  }
  virtual double action_multiplier()
  {
    double am = paladin_melee_attack_t::action_multiplier();

    // Holy Avenger buffs HotR damage by 30% while active
    if ( p() -> buffs.holy_avenger -> check() )
    {
      am *= 1.0 + p() -> buffs.holy_avenger -> data().effectN( 4 ).percent();
    }

    return am;
  }

  virtual void impact( action_state_t* s)
  {
    paladin_melee_attack_t::impact( s );

    // Everything hit by Hammer of the Righteous AoE triggers Weakened Blows
      if ( ! sim -> overrides.weakened_blows )
        target -> debuffs.weakened_blows -> trigger();
      
      // Trigger Hand of Light procs
      trigger_hand_of_light( s );

  }
};

struct hammer_of_the_righteous_t : public paladin_melee_attack_t
{
  hammer_of_the_righteous_aoe_t* proc;

  hammer_of_the_righteous_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "hammer_of_the_righteous", p, p -> find_class_spell( "Hammer of the Righteous" ), true ),
      proc( new hammer_of_the_righteous_aoe_t( p ) )
  {
    parse_options( NULL, options_str );

    // link with Crusader Strike's cooldown
    cooldown = p -> get_cooldown( "crusader_strike" );
    cooldown -> duration = data().cooldown();

    // Sanctity of Battle reduces melee GCD
    sanctity_of_battle = p -> passives.sanctity_of_battle -> ok();

    // HotR triggers all seals
    trigger_seal = true; 

    // Implement AoE proc
    add_child( proc );
  }

  virtual void update_ready( timespan_t cd_duration )
  {
    // reduce cooldown if Sanctity of Battle present
    if ( p() -> passives.sanctity_of_battle -> ok() )
    {
      cd_duration = cooldown -> duration * p() -> cache.attack_haste();
    }
    paladin_melee_attack_t::update_ready( cd_duration );
  }

  virtual double action_multiplier()
  {
    double am = paladin_melee_attack_t::action_multiplier();

    // Holy Avenger buffs HotR damage by 30% while active
    if ( p() -> buffs.holy_avenger -> check() )
    {
      am *= 1.0 + p() -> buffs.holy_avenger -> data().effectN( 4 ).percent();
    }

    return am;
  }

  virtual void impact( action_state_t* s )
  {
    paladin_melee_attack_t::impact( s );

    // Special things that happen when HotR connects
    if ( result_is_hit( s -> result ) )
    {
      // Trigger AoE proc
      proc -> execute();

      // Holy Power gains, only relevant if CS connects
      int g = 1; // default is a gain of 1 Holy Power
      p() -> resource_gain( RESOURCE_HOLY_POWER, g, p() -> gains.hp_hammer_of_the_righteous ); // apply gain, record as due to CS
     
      // If Holy Avenger active, grant 2 more
      if ( p() -> buffs.holy_avenger -> check() )
      {
        //apply gain, record as due to Holy Avenger
        p() -> resource_gain( RESOURCE_HOLY_POWER, p() -> buffs.holy_avenger -> value() - g, p() -> gains.hp_holy_avenger );
      }
      
      // Trigger Hand of Light procs
      trigger_hand_of_light( s );

      // Trigger Grand Crusader procs
      p() -> trigger_grand_crusader();

      // Mists of Pandaria: Hammer of the Righteous triggers Weakened Blows
      if ( ! sim -> overrides.weakened_blows )
        target -> debuffs.weakened_blows -> trigger();
    }
  }
};

// Hammer of Wrath ==========================================================

struct hammer_of_wrath_t : public paladin_melee_attack_t
{
  double cooldown_mult;

  hammer_of_wrath_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "hammer_of_wrath", p, p -> find_class_spell( "Hammer of Wrath" ), true ),
      cooldown_mult( 1.0 )
  {
    parse_options( NULL, options_str );

    // Sanctity of Battle reduces melee GCD
    sanctity_of_battle = p -> passives.sanctity_of_battle -> ok();

    // Cannot be parried or blocked, but can be dodged
    may_parry    = false;
    may_block    = false;

    // no weapon multiplier
    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0.0;

    // shift coefficients around for formula consistency
    base_spell_power_multiplier  = direct_power_mod;
    base_attack_power_multiplier = data().extra_coeff();
    direct_power_mod             = 1.0;

    // define cooldown multiplier for use with Sanctified Wrath talent for retribution only
    if ( ( p -> specialization() == PALADIN_RETRIBUTION ) && p -> find_talent_spell( "Sanctified Wrath" ) -> ok()  )
    {
      cooldown_mult = p -> find_talent_spell( "Sanctified Wrath" ) -> effectN( 1 ).percent();
    }
  }

  // Special things that happen with Hammer of Wrath damages target
  virtual void impact( action_state_t* s )
  {
    paladin_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      // if Ret spec, grant Holy Power
      if ( p() -> passives.sword_of_light -> ok() )
      {
        int g = 1; // base gain is 1 Holy Power
       //apply gain, attribute to Hammer of Wrath
        p() -> resource_gain( RESOURCE_HOLY_POWER, g, p() -> gains.hp_hammer_of_wrath );

        // Holy Avenger adds 2 more Holy Power if active
        if ( p() -> buffs.holy_avenger -> check() )
        {
       //apply gain, attribute to Holy Avenger
          p() -> resource_gain( RESOURCE_HOLY_POWER, p() -> buffs.holy_avenger -> value() - g, p() -> gains.hp_holy_avenger );
        }
      }
      
      // Trigger Hand of Light procs
      trigger_hand_of_light( s );
    }
  }

  virtual double action_multiplier()
  {
    double am = paladin_melee_attack_t::action_multiplier();
    
    // Holy Avenger buffs CS damage by 30% while active
    if ( p() -> buffs.holy_avenger -> check() )
    {
      am *= 1.0 + p() -> buffs.holy_avenger -> data().effectN( 4 ).percent();
    }

    return am;
  }

  virtual void update_ready( timespan_t cd_override )
  {
    cd_override = cooldown -> duration;
    
    // Reduction during AW - does nothing if Sanctified Wrath not talented
    if ( p() -> buffs.avenging_wrath -> up() )
    {
      cd_override *= cooldown_mult;
    }

    // reduce cooldown if Sanctity of Battle present
    if ( p() -> passives.sanctity_of_battle -> ok() )
    {
      cd_override *= p() -> cache.attack_haste();
    }

    paladin_melee_attack_t::update_ready( cd_override );
  }

  virtual bool ready()
  {
    // not available if target is above 20% health unless Sword of Light present and Avenging Wrath up
    if ( target -> health_percentage() > 20 && ! ( p() -> passives.sword_of_light -> ok() && p() -> buffs.avenging_wrath -> check() ) )
      return false;

    return paladin_melee_attack_t::ready();
  }
};

// Hand of Light proc =======================================================

struct hand_of_light_proc_t : public paladin_melee_attack_t
{
  hand_of_light_proc_t( paladin_t* p )
    : paladin_melee_attack_t( "hand_of_light", p, spell_data_t::nil(), false )
  {
    school = SCHOOL_HOLY;
    may_crit    = false;
    may_miss    = false;
    may_dodge   = false;
    may_parry   = false;
    may_glance  = false;
    proc        = true;
    background  = true;
    trigger_gcd = timespan_t::zero();
    id          = 96172;
  }

  virtual double action_multiplier()
  {
    //am = melee_attack_t::action_multiplier();
    // not *= since we don't want to double dip, just calling base to initialize variables
    double am = static_cast<paladin_t*>( player ) -> get_hand_of_light();
    //am *= 1.0 + static_cast<paladin_t*>( player ) -> buffs.inquisition -> value();  //was double dipping on inquisition -
    //once from the paladin composite player multiplier with inq on all holy damage and once from this line of code
    //was double dipping on avenging wrath - hand of light is not affected by avenging wrath so that it does not double dip
    //easier to remove it here than try to add an exception at the global avenging wrath buff level
    if ( p() -> buffs.avenging_wrath -> check() )
    {
      am /= 1.0 + p() -> buffs.avenging_wrath -> value();
    }
    return am;
  }
};

// Paladin Seals ============================================================

struct paladin_seal_t : public paladin_melee_attack_t
{
  seal_e seal_type;

  paladin_seal_t( paladin_t* p, const std::string& n, seal_e st, const std::string& options_str )
    : paladin_melee_attack_t( n, p ), seal_type( st )
  {
    parse_options( NULL, options_str );

    harmful    = false;
    resource_current = RESOURCE_MANA;
    // all seals cost 16.4% of base mana
    base_costs[ current_resource() ]  = p -> resources.base[ current_resource() ] * 0.164;
  }

  // current_resource() returns none since we're not passing the constructor spell information
 // virtual resource_e current_resource()
  //{ return RESOURCE_MANA; }

  virtual void execute()
  {
    if ( sim -> log ) sim -> output( "%s performs %s", player -> name(), name() );
    consume_resource();
    seal_e seal_orig = p() -> active_seal;
    if ( p() -> specialization() == PALADIN_PROTECTION && seal_type == SEAL_OF_JUSTICE )
    {
      sim -> errorf( "Protection attempted to cast Seal of Justice, defaulting to no seal" );
      p() -> active_seal = SEAL_NONE;
    }
    else
      p() -> active_seal = seal_type; // set the new seal

    // if we've swapped to or from Seal of Insight, we'll need to refresh spell haste cache
    if ( seal_orig != seal_type )
      if ( seal_orig == SEAL_OF_INSIGHT ||
           seal_type == SEAL_OF_INSIGHT )
        p() -> invalidate_cache( CACHE_SPELL_SPEED );
  }

  virtual bool ready()
  {
    if ( p() -> active_seal == seal_type ) return false;
    return paladin_melee_attack_t::ready();
  }
};

// Rebuke ===================================================================

struct rebuke_t : public paladin_melee_attack_t
{
  rebuke_t( paladin_t* p, const std::string& options_str ) :
    paladin_melee_attack_t( "rebuke", p, p -> find_class_spell( "Rebuke" ) )
  {
    parse_options( NULL, options_str );

    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
  }

  virtual bool ready()
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return paladin_melee_attack_t::ready();
  }
};

// Seal of Justice ==========================================================

struct seal_of_justice_proc_t : public paladin_melee_attack_t
{
  seal_of_justice_proc_t( paladin_t* p ) :
    paladin_melee_attack_t( "seal_of_justice_proc", p, p -> find_spell( p -> find_class_spell( "Seal of Justice" ) -> ok() ? 20170 : 0 ) )
  {
    background        = true;
    proc              = true;
    trigger_gcd       = timespan_t::zero();
    weapon            = &( p -> main_hand_weapon );

    base_multiplier *= 1.0 + p -> sets -> set( SET_T14_4PC_MELEE ) -> effectN( 1 ).percent();
  }
};

struct seal_of_justice_aoe_proc_t : public paladin_melee_attack_t
{
  seal_of_justice_aoe_proc_t( paladin_t* p ) :
    paladin_melee_attack_t( "seal_of_justice_aoe_proc", p, p -> find_spell( p -> find_class_spell( "Seal of Justice" ) -> ok() ? 20170 : 0 ) )
  {
    background        = true;
    proc              = true;
    aoe               = -1;
    trigger_gcd       = timespan_t::zero();
    weapon            = &( p -> main_hand_weapon );

    base_multiplier *= 1.0 + p -> sets -> set( SET_T14_4PC_MELEE ) -> effectN( 1 ).percent();
  }
};

// Seal of Righteousness ====================================================

struct seal_of_righteousness_proc_t : public paladin_melee_attack_t
{
  seal_of_righteousness_proc_t( paladin_t* p ) :
    paladin_melee_attack_t( "seal_of_righteousness_proc", p, p -> find_spell( p -> find_class_spell( "Seal of Righteousness" ) -> ok() ? 101423 : 0 ) )
  {
    background  = true;
    proc        = true;
    trigger_gcd = timespan_t::zero();

    weapon      = &( p -> main_hand_weapon );
    aoe         = -1;

    // T14 Retribution 4-piece increases seal damage
    base_multiplier *= 1.0 + p -> sets -> set( SET_T14_4PC_MELEE ) -> effectN( 1 ).percent();
  }
};

// Seal of Truth ============================================================

struct seal_of_truth_proc_t : public paladin_melee_attack_t
{
  seal_of_truth_proc_t( paladin_t* p )
    : paladin_melee_attack_t( "seal_of_truth_proc", p, p -> find_class_spell( "Seal of Truth" ), true )
  {
    background  = true;
    proc        = true;
    // automatically connects when triggered
    may_block   = false;
    may_glance  = false;
    may_miss    = false;
    may_dodge   = false;
    may_parry   = false;
    trigger_gcd = timespan_t::zero();

    weapon      = &( p -> main_hand_weapon );

    if ( data().ok() )
    {
      // proc info stored in spell 42463. 
      // Note that effect #2 is a lie, SoT uses unnormalized weapon speed, so we leave that as default (false)
      const spell_data_t* s = p -> find_spell( 42463 );
      if ( s && s -> ok() )
      {
        // Undocumented in spell info: proc is 80% smaller for protection
        weapon_multiplier      = s -> effectN( 1 ).percent();
        if ( p -> specialization() == PALADIN_PROTECTION )
        {
          weapon_multiplier /= 5;
        }
      }
    }

    // Glyph of Immediate Truth increases direct damage
    if ( p -> glyphs.immediate_truth -> ok() )
    {
      base_multiplier *= 1.0 + p -> glyphs.immediate_truth -> effectN( 1 ).percent();
    }

    // Retribution T14 4-piece boosts seal damage
    base_multiplier *= 1.0 + p -> sets -> set( SET_T14_4PC_MELEE ) -> effectN( 1 ).percent();
  }
};

// Judgment ================================================================

struct judgment_t : public paladin_melee_attack_t
{
  double cooldown_mult; // used to handle Sanctified Wrath cooldown reduction

  judgment_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "judgment", p, p -> find_spell( "Judgment" ), true ),
      cooldown_mult( 1.0 )
  {
    parse_options( NULL, options_str );

    // Sanctity of Battle reduces melee GCD
    sanctity_of_battle = p -> passives.sanctity_of_battle -> ok();

    // Spell database info jumbled, re-arrange to make correct
    base_spell_power_multiplier  = direct_power_mod;
    base_attack_power_multiplier = data().extra_coeff();
    direct_power_mod             = 1.0;

    // Special melee attack that can only miss
    may_glance                   = false;
    may_block                    = false;
    may_parry                    = false;
    may_dodge                    = false;

    // Only triggers Seal of Truth
    trigger_seal_of_truth        = true;

    // define cooldown multiplier for use with Sanctified Wrath talent for protection only
    if ( ( p -> specialization() == PALADIN_PROTECTION ) && p -> find_talent_spell( "Sanctified Wrath" ) -> ok()  )
    {
      cooldown_mult = p -> find_talent_spell( "Sanctified Wrath" ) -> effectN( 1 ).percent();
    }

    // damage multiplier from T14 Retribution 4-piece bonus
    base_multiplier *= 1.0 + p -> sets -> set( SET_T14_4PC_MELEE ) -> effectN( 1 ).percent();
  }

  virtual void execute()
  {
    paladin_melee_attack_t::execute();

    // Special things that happen when Judgment succeeds
    if ( result_is_hit( execute_state -> result ) )
    {
      // +1 Holy Power for Ret
      if ( p() -> passives.judgments_of_the_bold -> ok() )
      {
        // apply gain, attribute gain to Judgments of the Bold
        p() -> resource_gain( RESOURCE_HOLY_POWER, 1, p() -> gains.hp_judgments_of_the_bold );

      }
      // +1 Holy Power for Prot
      else if ( p() -> passives.judgments_of_the_wise -> ok() )
      {
        // apply gain, attribute gain to Judgments of the Wise
        p() -> resource_gain( RESOURCE_HOLY_POWER, 1, p() -> gains.hp_judgments_of_the_wise );
      }

      // Holy Avenger adds 2 more Holy Power if active
      if ( p() -> buffs.holy_avenger -> check() )
      {
       //apply gain, attribute to Holy Avenger
        p() -> resource_gain( RESOURCE_HOLY_POWER, p() -> buffs.holy_avenger -> value() - 1, p() -> gains.hp_holy_avenger );
      }
    }
  }

  // Special things that happen when Judgment damages target
  virtual void impact( action_state_t* s )
  {
    paladin_melee_attack_t::impact( s );

    // Glyph of Double Jeopardy handling ===========================================================
    // original implementation was incorrect - would only grant benefit ~ half the time.
    // proper implemenation: buff granted on every J, target set to new value each time.
    // action_multiplier() gets called before impact(), so we can just set the buff parameters here 
    // and let action_multiplier() take care of figuring out if we should get a damage boost or not.

    // if the glyph is present, trigger the buff and store the target information
    if ( p() -> glyphs.double_jeopardy -> ok() )
    {
      p() -> buffs.double_jeopardy -> trigger();
      p() -> last_judgement_target = s -> target;
    }
    // end Double Jeopardy ==========================================================================

    // Physical Vulnerability debuff
    if ( ! sim -> overrides.physical_vulnerability && p() -> passives.judgments_of_the_bold -> ok() )
      s -> target -> debuffs.physical_vulnerability -> trigger();
  }

  virtual double action_multiplier()
  {
    double am = paladin_melee_attack_t::action_multiplier();
    
    // Holy Avenger buffs J damage by 30% while active
    if ( p() -> buffs.holy_avenger -> check() )
    {
      am *= 1.0 + p() -> buffs.holy_avenger -> data().effectN( 4 ).percent();
    }

    // Double Jeopardy damage boost, only if we hit a different target with it
    // this gets called before impact(), so last_judgment_target won't be updated until after this resolves
    if (  p() -> glyphs.double_jeopardy -> ok()              // glyph equipped
              && target != p() -> last_judgement_target      // current target is different than last_j_target
              && p() -> buffs.double_jeopardy -> check() )   // double_jeopardy buff is still active
    {
      am *= 1.0 + p() -> buffs.double_jeopardy -> value();
    }

    return am;
  }

  // Cooldown reduction effects
  virtual void update_ready( timespan_t cd_duration )
  {
    cd_duration = cooldown -> duration;

    // Sanctity of Battle
    if ( p() -> passives.sanctity_of_battle -> ok() )
    {
      cd_duration *= p() -> cache.attack_haste();
    }

    // Reduction during AW - does nothing if Sanctified Wrath not talented
    if ( p() -> buffs.avenging_wrath -> up() )
    {
      cd_duration *= cooldown_mult;
    }

    paladin_melee_attack_t::update_ready( cd_duration );
  }

  virtual bool ready()
  {
    // Only usable if a seal is active
    if ( p() -> active_seal == SEAL_NONE ) return false;

    return paladin_melee_attack_t::ready();
  }
};

// Shield of the Righteous ==================================================

struct shield_of_the_righteous_t : public paladin_melee_attack_t
{
  shield_of_the_righteous_t( paladin_t* p, const std::string& options_str ) :
    paladin_melee_attack_t( "shield_of_the_righteous", p, p -> find_class_spell( "Shield of the Righteous" ) )
  {
    parse_options( NULL, options_str );

    if ( ! p -> has_shield_equipped() )
    {
      sim -> errorf( "%s: %s only usable with shield equiped in offhand\n", p -> name(), name() );
      background = true;
    }

    // Triggers all seals
    trigger_seal = true;

    // not on GCD, usable off-GCD
    trigger_gcd = timespan_t::zero();
    use_off_gcd = true;
    
    // no weapon scaling
    weapon_multiplier = 0.0;

    //set AP scaling coefficient, set direct power mod equal to 1 (defaults to 0)
    base_attack_power_multiplier = data().extra_coeff();
    direct_power_mod = 1;
  }

  virtual void execute()
  {
    paladin_melee_attack_t::execute();

    //Buff granted regardless of combat roll result 
    //Duration is additive on refresh, stats not recalculated
    if ( p() -> buffs.shield_of_the_righteous -> check() )
    {
      p() -> buffs.shield_of_the_righteous -> extend_duration( p(), timespan_t::from_seconds( p() -> find_class_spell( "Shield of the Righteous" ) -> _duration) );
    }
    else
      p() -> buffs.shield_of_the_righteous -> trigger();
  }

  virtual void update_ready( timespan_t cd_duration )
  {
    // reduce cooldown if Sanctity of Battle present
    if ( p() -> passives.sanctity_of_battle -> ok() )
    {
      cd_duration = cooldown -> duration * p() -> cache.attack_haste();
    }
    
    paladin_melee_attack_t::update_ready( cd_duration );
  }

  //todo: Alabaster Shield damage buff
};

// Templar's Verdict ========================================================

struct templars_verdict_t : public paladin_melee_attack_t
{
  templars_verdict_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "templars_verdict", p, p -> find_class_spell( "Templar's Verdict" ), true )
  {
    parse_options( NULL, options_str );
    sanctity_of_battle = true;
    trigger_seal       = true;
  }

  virtual void execute ()
  {
    school = p() -> buffs.tier15_4pc_melee -> up() ? SCHOOL_HOLY : SCHOOL_PHYSICAL;

    paladin_melee_attack_t::execute();
  }

  virtual void impact( action_state_t* s )
  {
    paladin_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      trigger_hand_of_light( s );
      p() -> buffs.tier15_4pc_melee -> expire();
    }
  }

  virtual double action_multiplier()
  {
    double am = paladin_melee_attack_t::action_multiplier();

    if ( p() -> set_bonus.tier13_4pc_melee() )
    {
      am *= 1.0 + p() -> sets -> set( SET_T13_4PC_MELEE ) -> effectN( 1 ).percent();
    }
    if ( p() -> set_bonus.tier14_2pc_melee() )
    {
      am *= 1.0 + p() -> sets -> set( SET_T14_2PC_MELEE ) -> effectN( 1 ).percent();
    }

    return am;
  }
};

} // end namespace attacks


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

  virtual void execute()
  {
    double c = ( this -> current_resource() == RESOURCE_HOLY_POWER ) ? this -> cost() : -1.0;

    ab::execute();

    paladin_t& p = *this -> p();

    if ( p.talents.divine_purpose -> ok() )
    {
      if ( c > 0.0 )
      {
        p.buffs.divine_purpose -> trigger();
      }
      else if ( c == 0.0 )
      {
        p.buffs.divine_purpose -> expire();
        // p() -> resource_gain( RESOURCE_HOLY_POWER, 3, p() -> gains.hp_divine_purpose );
        p.buffs.divine_purpose -> trigger();
      }
    }
  }

};

namespace spells {

// ==========================================================================
// Paladin Spells
// ==========================================================================

struct paladin_spell_t : public paladin_spell_base_t<spell_t>
{
  paladin_spell_t( const std::string& n, paladin_t* p,
                   const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  {
  }
};

// Ancient Fury =============================================================

struct ancient_fury_t : public paladin_spell_t
{
  ancient_fury_t( paladin_t* p ) :
    paladin_spell_t( "ancient_fury", p, p -> passives.ancient_fury )
  {
    // TODO meteor stuff
    background = true;
    callbacks  = false;
    may_crit   = true;
    crit_bonus = 1.0; // Ancient Fury crits for 200%
  }

  virtual void execute()
  {
    paladin_spell_t::execute();

    p() -> buffs.ancient_power -> expire();
  }

  virtual double action_multiplier()
  {
    double am = paladin_spell_t::action_multiplier();

    am *= p() -> buffs.ancient_power -> stack();

    return am;
  }
};

// Ardent Defender ===========================================================

struct ardent_defender_t : public paladin_spell_t
{
  ardent_defender_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "ardent_defender", p, p -> find_specialization_spell( "Ardent Defender" ) )
  {
    parse_options( nullptr, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    paladin_spell_t::execute();

    p() -> buffs.ardent_defender -> trigger();
  }
};

// Avengers Shield ==========================================================

struct avengers_shield_t : public paladin_spell_t
{
  avengers_shield_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "avengers_shield", p, p -> find_class_spell( "Avenger's Shield" ) )
  {
    parse_options( NULL, options_str );

    if ( ! p -> has_shield_equipped() )
    {
      sim -> errorf( "%s: %s only usable with shield equiped in offhand\n", p -> name(), name() );
      background = true;
    }

    // hits 1 target if FS glyphed, 3 otherwise
    aoe = ( p -> glyphs.focused_shield -> ok() ) ? 1 : 3;
    may_crit     = true;
    
    // no weapon multiplier
    weapon_multiplier = 0.0;

    // link needed for trigger_grand_crusader
    cooldown = p -> cooldowns.avengers_shield;
    cooldown -> duration = data().cooldown();
    
    //AS spell data is weird - direct_power_mod holds the SP scaling, extra_coeff() the AP scaling
    base_spell_power_multiplier  = direct_power_mod;
    base_attack_power_multiplier = data().extra_coeff();
    direct_power_mod = 1.0; // reset direct power coefficient to 1 (see action_t::calculate_direct_amount in sc_action.cpp)
  }

  // Multiplicative damage effects
  virtual double action_multiplier()
  {
    double am = paladin_spell_t::action_multiplier();

    //Holy Avenger buffs damage iff Grand Crusader is active
    if ( p() -> buffs.holy_avenger -> check() && p() -> buffs.grand_crusader -> check() )
    {
      am *= 1.0 + p() -> buffs.holy_avenger -> data().effectN( 4 ).percent();
    }

    //Focused Shield gives another multiplicative factor of 1.3 (tested 5/26/2013, multiplicative with HA)
    if ( p() -> glyphs.focused_shield -> ok() )
    {
      am *= 1.0 + p() -> glyphs.focused_shield -> effectN( 2 ).percent();
    }

    return am;
  }

  //Sanctity of Battle reduces cooldown
  virtual void update_ready( timespan_t cd_duration )
  {
    if ( p() -> passives.sanctity_of_battle -> ok() )
    {
      cd_duration = cooldown -> duration * p() -> cache.attack_haste();
    }
    paladin_spell_t::update_ready( cd_duration );
  }

  virtual void execute()
  {
    paladin_spell_t::execute();

    //Holy Power gains - only if Grand Crusader is active
    if ( p() -> buffs.grand_crusader -> up() )
    {
      //base gain is 1 Holy Power
      int g = 1;
      //apply gain, attribute to Grand Crusader
      p() -> resource_gain( RESOURCE_HOLY_POWER, g, p() -> gains.hp_grand_crusader );

      //Holy Avenger adds another 2 Holy Power
      if ( p() -> buffs.holy_avenger -> check() )
      {
        //apply gain, attribute to Holy Avenger
        p() -> resource_gain( RESOURCE_HOLY_POWER,
                              p() -> buffs.holy_avenger -> value() - g,
                              p() -> gains.hp_holy_avenger );
      }

      //Remove Grand Crsuader buff
      p() -> buffs.grand_crusader -> expire();
    }
  }
};

// Avenging Wrath ===========================================================

struct avenging_wrath_t : public paladin_spell_t
{
  avenging_wrath_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "avenging_wrath", p, p -> find_class_spell( "Avenging Wrath" ) )
  {
    parse_options( NULL, options_str );

    cooldown -> duration += p -> passives.sword_of_light -> effectN( 7 ).time_value();

    harmful = false;
  }

  virtual void execute()
  {
    paladin_spell_t::execute();

    p() -> buffs.avenging_wrath -> trigger( 1, data().effectN( 1 ).percent() );
  }
};

// Blessing of Kings ========================================================

struct blessing_of_kings_t : public paladin_spell_t
{
  blessing_of_kings_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "blessing_of_kings", p, p -> find_class_spell( "Blessing of Kings" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;

    background = ( sim -> overrides.str_agi_int != 0 );
  }

  virtual void execute()
  {
    paladin_spell_t::execute();

    if ( ! sim -> overrides.str_agi_int )
    {
      sim -> auras.str_agi_int -> trigger();
      p() -> bok_up = true;
    }

    if ( ! sim -> overrides.mastery && p() -> bom_up )
    {
      sim -> auras.mastery -> decrement();
      p() -> bom_up = false;
    }
  }
};

// Blessing of Might ========================================================

struct blessing_of_might_t : public paladin_spell_t
{
  blessing_of_might_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "blessing_of_might", p, p -> find_class_spell( "Blessing of Might" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;

    background = ( sim -> overrides.mastery != 0 );
  }

  virtual void execute()
  {
    paladin_spell_t::execute();

    if ( ! sim -> overrides.mastery )
    {
      double mastery_rating = data().effectN( 1 ).average( player );
      if ( ! sim -> auras.mastery -> check() || sim -> auras.mastery -> current_value < mastery_rating )
        sim -> auras.mastery -> trigger( 1, mastery_rating );
      p() -> bom_up = true;
    }

    if ( ! sim -> overrides.str_agi_int && p() -> bok_up )
    {
      sim -> auras.str_agi_int -> decrement();
      p() -> bok_up = false;
    }
  }
};

// Consecration =============================================================

struct consecration_tick_t : public paladin_spell_t
{
  consecration_tick_t( paladin_t* p )
    : paladin_spell_t( "consecration_tick", p, p -> find_spell( 81297 ) )
  {
    aoe         = -1;
    dual        = true;
    direct_tick = true;
    background  = true;
    may_crit    = true;
    may_miss    = true;
    hasted_ticks = false;  // fixed 1s tick interval

    // Spell data jumbled, re-arrange modifiers appropriately
    base_spell_power_multiplier  = direct_power_mod;
    base_attack_power_multiplier = data().extra_coeff();
    direct_power_mod             = 1.0;
    weapon_multiplier            = 0;
  }
};

struct consecration_t : public paladin_spell_t
{
  consecration_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "consecration", p, p -> find_class_spell( "Consecration" ) )
  {
    parse_options( NULL, options_str );

    hasted_ticks   = false;
    may_miss       = false;
    num_ticks      = 9;
    base_tick_time = timespan_t::from_seconds( 1.0 );

    tick_action = new consecration_tick_t( p );
  }

  virtual void tick( dot_t* d )
  {
    if ( d -> state -> target -> debuffs.flying -> check() )
    {
      if ( sim -> debug ) sim -> output( "Ground effect %s can not hit flying target %s", name(), d -> state -> target -> name() );
    }
    else
    {
      paladin_spell_t::tick( d );
    }
  }
};

// Divine Plea ==============================================================

struct divine_plea_t : public paladin_spell_t
{
  divine_plea_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "divine_plea", p, p -> find_class_spell( "Divine Plea" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    paladin_spell_t::execute();

    p() -> buffs.divine_plea -> trigger();
  }
};

// Divine Protection ========================================================

struct divine_protection_t : public paladin_spell_t
{
  divine_protection_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "divine_protection", p, p -> find_class_spell( "Divine Protection" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    paladin_spell_t::execute();

    p() -> buffs.divine_protection -> trigger();
  }
};

// Divine Shield ============================================================

struct divine_shield_t : public paladin_spell_t
{
  divine_shield_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "divine_shield", p, p -> find_class_spell( "Divine Shield" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    paladin_spell_t::execute();

    // Technically this should also drop you from the mob's threat table,
    // but we'll assume the MT isn't using it for now
    p() -> buffs.divine_shield -> trigger();
    p() -> debuffs.forbearance -> trigger();
  }

  virtual bool ready()
  {
    if ( player -> debuffs.forbearance -> check() )
      return false;

    return paladin_spell_t::ready();
  }
};

// Execution Sentence =======================================================

struct execution_sentence_t : public paladin_spell_t
{
  std::array<double,11> tick_multiplier;

  execution_sentence_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "execution_sentence", p, p -> find_talent_spell( "Execution Sentence" ) ),
      tick_multiplier()
  {
    parse_options( NULL, options_str );
    hasted_ticks   = false;
    travel_speed   = 0;
    tick_may_crit  = 1;

    // Where the 0.0374151195 comes from
    // The whole dots scales with data().effectN( 2 ).base_value()/1000 * SP
    // Tick 1-9 grow exponentionally by 10% each time, 10th deals 5x the
    // damage of the 9th.
    // 1.1 + 1.1^2 + ... + 1.1^9 + 1.1^9 * 5 = 26.727163056
    // 1 / 26,727163056 = 0.0374151195, which is the factor to get from the
    // whole spells SP scaling to the base scaling of the 0th tick
    // 1st tick is already 0th * 1.1!
    if ( data().ok() )
    {
      parse_effect_data( ( p -> find_spell( 114916 ) -> effectN( 1 ) ) );
      tick_power_mod = p -> find_spell( 114916 ) -> effectN( 2 ).base_value()/1000.0 * 0.0374151195;
    }

    assert( as<unsigned>( num_ticks ) < sizeof_array( tick_multiplier ) );
    tick_multiplier[ 0 ] = 1.0;
    for ( int i = 1; i < num_ticks; ++i )
      tick_multiplier[ i ] = tick_multiplier[ i - 1 ] * 1.1;
    tick_multiplier[ 10 ] = tick_multiplier[ 9 ] * 5;
  }

  double composite_target_multiplier( player_t* target )
  {
    double m = paladin_spell_t::composite_target_multiplier( target );
    m *= tick_multiplier[ td( target ) -> dots.execution_sentence -> current_tick ];
    return m;
  }
};

// Exorcism =================================================================

struct exorcism_t : public paladin_spell_t
{
  exorcism_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "exorcism", p, p -> find_class_spell( "Exorcism" ) )
  {
    parse_options( NULL, options_str );

    base_attack_power_multiplier = 1.0;
    base_spell_power_multiplier  = 0.0;
    direct_power_mod             = data().extra_coeff();
    may_crit                     = true;

    if ( p -> glyphs.mass_exorcism -> ok() )
    {
      aoe = -1;
      base_aoe_multiplier = 0.25;
    }

    cooldown = p -> cooldowns.exorcism;
    cooldown -> duration = data().cooldown();
  }

  virtual double action_multiplier()
  {
    double am = paladin_spell_t::action_multiplier();

    if ( p() -> buffs.holy_avenger -> check() )
    {
      am *= 1.0 + p() -> buffs.holy_avenger -> data().effectN( 4 ).percent();
    }

    return am;
  }

  virtual void update_ready( timespan_t cd_duration )
  {
    if ( p() -> passives.sanctity_of_battle -> ok() )
    {
      cd_duration = cooldown -> duration * p() -> cache.attack_haste();
    }
    paladin_spell_t::update_ready( cd_duration );
  }

  virtual void execute()
  {
    paladin_spell_t::execute();
    if ( result_is_hit( execute_state -> result ) )
    {
      int g = 1;
      p() -> resource_gain( RESOURCE_HOLY_POWER, g, p() -> gains.hp_exorcism );
      if ( p() -> buffs.holy_avenger -> check() )
      {
        p() -> resource_gain( RESOURCE_HOLY_POWER, p() -> buffs.holy_avenger -> value() - g, p() -> gains.hp_holy_avenger );
      }
    }
  }

  virtual void impact( action_state_t* s )
  {
    if ( result_is_hit( s -> result ) && p() -> set_bonus.tier15_2pc_melee() )
    {
      p() -> buffs.tier15_2pc_melee -> trigger();
    }

    paladin_spell_t::impact( s );
  }
};

// Guardian of the Ancient Kings ============================================

struct guardian_of_ancient_kings_t : public paladin_spell_t
{
  guardian_of_ancient_kings_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "guardian_of_ancient_kings", p, p -> find_class_spell( "Guardian of Ancient Kings" ) )
  {
    parse_options( NULL, options_str );
    use_off_gcd = true;
  }

  virtual void execute()
  {
    paladin_spell_t::execute();

    if ( p() -> specialization() == PALADIN_RETRIBUTION )
      p() -> guardian_of_ancient_kings -> summon( p() -> spells.guardian_of_ancient_kings_ret -> duration() );
    else if ( p() -> specialization() == PALADIN_PROTECTION )
      p() -> buffs.gotak_prot -> trigger();
  }
};

// Holy Shock ===============================================================

// TODO: fix the fugly hack
struct holy_shock_t : public paladin_spell_t
{
  double cooldown_mult;

  holy_shock_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "holy_shock", p, p -> find_class_spell( "Holy Shock" ) ),
      cooldown_mult( 1.0 )
  {
    parse_options( NULL, options_str );

    // hack! spell 20473 has the cooldown/cost/etc stuff, but the actual spell cast
    // to do damage is 25912
    parse_effect_data( ( *player -> dbc.effect( 25912 ) ) );

    base_crit += 0.25;

    if ( ( p -> specialization() == PALADIN_HOLY ) && p -> find_talent_spell( "Sanctified Wrath" ) -> ok()  )
    {
      cooldown_mult = p -> find_talent_spell( "Sanctified Wrath" ) -> effectN( 1 ).percent();
    }
  }

  virtual void execute()
  {
    paladin_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      int g = 1;
      p() -> resource_gain( RESOURCE_HOLY_POWER, g, p() -> gains.hp_holy_shock );
      if ( p() -> buffs.holy_avenger -> check() )
      {
        p() -> resource_gain( RESOURCE_HOLY_POWER,
                              p() -> buffs.holy_avenger -> value() - g,
                              p() -> gains.hp_holy_avenger );
      }
    }
  }

  virtual void update_ready( timespan_t cd_override )
  {
    cd_override = cooldown -> duration;
    if ( p() -> buffs.avenging_wrath -> up() )
      cd_override *= cooldown_mult;

    paladin_spell_t::update_ready( cd_override );
  }
};

// Holy Wrath ===============================================================

struct holy_wrath_t : public paladin_spell_t
{
  holy_wrath_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "holy_wrath", p, p -> find_class_spell( "Holy Wrath" ) )
  {
    parse_options( NULL, options_str );

    // aoe = -1; FIXME disabled until we have meteor support
    may_crit   = true;

    // no weapon component
    weapon_multiplier = 0;

    //Holy Wrath is an oddball - spell database entry doesn't properly contain damage details
    //Tooltip formula is wrong as well.  We're gong to hardcode it here based on empirical testing
    //see http://maintankadin.failsafedesign.com/forum/viewtopic.php?p=743800#p743800

    base_dd_min = 4300; // fixed base damage (no range)
    base_dd_max = 4300;
    base_attack_power_multiplier = 0.91; // 0.91 AP scaling
    base_spell_power_multiplier = 0;     // no SP scaling
    direct_power_mod = 1; 

  }

  //Sanctity of Battle reduces cooldown
  virtual void update_ready( timespan_t cd_duration )
  {
    if ( p() -> passives.sanctity_of_battle -> ok() )
    {
      cd_duration = cooldown -> duration * p() -> cache.attack_haste();
    }
    paladin_spell_t::update_ready( cd_duration );
  }
};

//Holy Prism ===============================================================

struct holy_prism_t : public paladin_spell_t
{
  holy_prism_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "holy_prism", p, p->find_spell( 114852 ) )
  {
    parse_options( NULL, options_str );
    school=SCHOOL_HOLY;
    may_crit=true;
    parse_effect_data( p -> find_spell( 114852 ) -> effectN( 1 ) );
    base_spell_power_multiplier = 1.0;
    base_attack_power_multiplier = 0.0;
    direct_power_mod= p->find_spell( 114852 ) -> effectN( 1 ).coeff();
  }
};

//Holy Prism AOE===============================================================

struct holy_prism_aoe_t : public paladin_spell_t
{
  holy_prism_aoe_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "holy_prism_aoe", p, p->find_spell( 114871 ) )
  {
    parse_options( NULL, options_str );
    may_crit=true;
    school=SCHOOL_HOLY;
    parse_effect_data( p -> find_spell( 114871 ) -> effectN( 2 ) );
    base_spell_power_multiplier = 1.0;
    base_attack_power_multiplier = 0.0;
    direct_power_mod= p->find_spell( 114871 ) -> effectN( 2 ).coeff();
    aoe=5;
  }
};

// Holy Avenger ===========================================================

struct holy_avenger_t : public paladin_spell_t
{
  holy_avenger_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "holy_avenger", p, p -> find_talent_spell( "Holy Avenger" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    paladin_spell_t::execute();

    p() -> buffs.holy_avenger -> trigger( 1, 3 );
  }
};

// Inquisition ==============================================================

struct inquisition_t : public paladin_spell_t
{
  timespan_t base_duration;
  double multiplier;

  inquisition_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "inquisition", p, p -> find_class_spell( "Inquisition" ) ),
      base_duration( data().duration() ),
      multiplier( data().effectN( 1 ).percent() )
  {
    parse_options( NULL, options_str );
    may_miss     = false;
    harmful      = false;
    hasted_ticks = false;
    num_ticks    = 0;

    if ( p -> glyphs.inquisition -> ok() )
    {
      multiplier += p -> glyphs.inquisition -> effectN( 1 ).percent();
      base_duration *= 1.0 + p -> glyphs.inquisition -> effectN( 2 ).percent();
    }
  }

  virtual void execute()
  {
    timespan_t duration = base_duration;
    if ( p() -> buffs.inquisition -> check() )
    {
      // Inquisition behaves like a dot with DOT_REFRESH.
      // You will not lose your current 'tick' when refreshing.
      int result = static_cast<int>( p() -> buffs.inquisition -> remains() / base_tick_time );
      timespan_t carryover = p() -> buffs.inquisition -> remains();
      carryover -= base_tick_time * result;
      duration += carryover;
    }

    duration += base_duration * ( p() -> holy_power_stacks() <=3 ? p() -> holy_power_stacks() - 1: 2 );
    p() -> buffs.inquisition -> trigger( 1, multiplier, -1.0, duration );

    paladin_spell_t::execute();
  }
};

// Light's Hammer ===========================================================

struct lights_hammer_tick_t : public paladin_spell_t
{
  lights_hammer_tick_t( paladin_t* p, const spell_data_t* s )
    : paladin_spell_t( "lights_hammer_tick", p, s )
  {
    dual = true;
    background = true;
    aoe = -1;
    may_crit = true;
  }
};

struct lights_hammer_t : public paladin_spell_t
{
  const timespan_t travel_time_;

  lights_hammer_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "lights_hammer", p, p -> find_talent_spell( "Light's Hammer" ) ),
      travel_time_( timespan_t::from_seconds( 1.5 ) )
  {
    // 114158: Talent spell
    // 114918: Periodic 2s dummy, no duration!
    // 114919: Damage/Scale data
    // 122773: 17.5s duration, 1.5s for hammer to land = 16s aoe dot
    parse_options( NULL, options_str );
    may_miss = false;

    school = SCHOOL_HOLY; // Setting this allows the tick_action to benefit from Inquistion

    base_tick_time = p -> find_spell( 114918 ) -> effectN( 1 ).period();
    num_ticks      = ( int ) ( ( p -> find_spell( 122773 ) -> duration() - travel_time_ ) / base_tick_time );
    hasted_ticks   = false;

    dynamic_tick_action = true;
    tick_action = new lights_hammer_tick_t( p, p -> find_spell( 114919 ) );
  }

  virtual timespan_t travel_time()
  { return travel_time_; }
};

// Word of Glory Damage Spell ======================================================

struct word_of_glory_damage_t : public paladin_spell_t
{
  word_of_glory_damage_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "word_of_glory_damage", p,
                     ( p -> find_class_spell( "Word of Glory" ) -> ok() && p -> glyphs.harsh_words -> ok() ) ? p -> find_spell( 130552 ) : spell_data_t::not_found() )
  {
    parse_options( NULL, options_str );
    trigger_gcd = timespan_t::from_seconds( 1.5 );
    resource_consumed = RESOURCE_HOLY_POWER;
    resource_current = RESOURCE_HOLY_POWER;
    base_costs[RESOURCE_HOLY_POWER] = 1;
  }

  virtual double action_multiplier()
  {
    double am = paladin_spell_t::action_multiplier();

    am *= ( p() -> holy_power_stacks() <=3 ? p() -> holy_power_stacks() : 3 );

    return am;
  }

  virtual double cost()
  {
    if ( p() -> buffs.divine_purpose -> check() )
      return 0.0;
    else if ( ( p() -> holy_power_stacks() <=3 ? p() -> holy_power_stacks() : 3 ) > 1 )
      return ( double )( p() -> holy_power_stacks() <=3 ? p() -> holy_power_stacks() : 3 );
    else
      return 1.0;
  }

  virtual void execute()
  {
    double hopo = ( p() -> holy_power_stacks() <=3 ? p() -> holy_power_stacks() : 3 );

    paladin_spell_t::execute();

    if ( p() -> spells.glyph_of_word_of_glory_damage -> ok() )
    {
      p() -> buffs.glyph_of_word_of_glory -> trigger( 1, p() -> spells.glyph_of_word_of_glory_damage -> effectN( 1 ).percent() * hopo );
    }
  }
};

// Seal of Truth ============================================================

struct seal_of_truth_dot_t : public paladin_spell_t
{
  seal_of_truth_dot_t( paladin_t* p ) :
    paladin_spell_t( "censure", p, p -> find_spell( p -> find_class_spell( "Seal of Truth" ) -> ok() ? 31803 : 0 ) )
  {
    background       = true;
    proc             = true;
    hasted_ticks     = true;
    tick_may_crit    = true;
    may_crit         = false;
    may_miss         = true;
    dot_behavior     = DOT_REFRESH;


    base_spell_power_multiplier  = tick_power_mod;
    base_attack_power_multiplier = data().extra_coeff();
    tick_power_mod               = 1.0;

    if ( p -> glyphs.immediate_truth -> ok() )
    {
      base_multiplier *= 1.0 + p -> glyphs.immediate_truth -> effectN( 2 ).percent();
    }
  }

  virtual void init()
  {
    paladin_spell_t::init();

    snapshot_flags |= STATE_HASTE;
  }

  virtual void impact( action_state_t* s )
  {
    if ( result_is_hit( s -> result ) )
    {
      td( s -> target ) -> debuffs_censure -> trigger();
    }

    paladin_spell_t::impact( s );
  }

  virtual double composite_target_multiplier( player_t* t )
  {
    double am = paladin_spell_t::composite_target_multiplier( t );

    am *= td( t ) -> debuffs_censure -> stack();

    return am;
  }

  virtual void last_tick( dot_t* d )
  {
    td( d -> state -> target ) -> debuffs_censure -> expire();

    paladin_spell_t::last_tick( d );
  }
};

} // end namespace spells


// ==========================================================================
// Paladin Heals
// ==========================================================================

namespace heals {

struct paladin_heal_t : public paladin_spell_base_t<heal_t>
{
  paladin_heal_t( const std::string& n, paladin_t* p,
                  const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  {
    may_crit          = true;
    tick_may_crit     = true;

    weapon_multiplier = 0.0;
  }

  virtual double action_multiplier()
  {
    double am = base_t::action_multiplier();

    if ( p() -> active_seal == SEAL_OF_INSIGHT )
      am *= 1.0 + data().effectN( 2 ).percent();

    return am;
  }

  virtual void impact( action_state_t* s )
  {
    base_t::impact( s );

    trigger_illuminated_healing( s );

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


    p() -> active_beacon_of_light -> base_dd_min = s -> result_amount * p() -> beacon_target -> buffs.beacon_of_light -> data().effectN( 1 ).percent();
    p() -> active_beacon_of_light -> base_dd_max = s -> result_amount * p() -> beacon_target -> buffs.beacon_of_light -> data().effectN( 1 ).percent();

    // Holy light heals for 100% instead of 50%
    if ( data().id() == p() -> spells.holy_light -> id() )
    {
      p() -> active_beacon_of_light -> base_dd_min *= 2.0;
      p() -> active_beacon_of_light -> base_dd_max *= 2.0;
    }

    p() -> active_beacon_of_light -> execute();
  };

  void trigger_illuminated_healing( action_state_t* s )
  {
    if ( s -> result_amount <= 0 )
      return;

    if ( proc )
      return;

    if ( ! p() -> passives.illuminated_healing -> ok() )
      return;

    // FIXME: Each player can have their own bubble, so this should probably be a vector as well
    assert( p() -> active_illuminated_healing );

    // FIXME: This should stack when the buff is present already

    double bubble_value = p() -> cache.mastery_value()
                          * s -> result_amount;

    p() -> active_illuminated_healing -> base_dd_min = p() -> active_illuminated_healing -> base_dd_max = bubble_value;
    p() -> active_illuminated_healing -> execute();
  }
};

// Seal of Insight ==========================================================

struct seal_of_insight_proc_t : public paladin_heal_t
{
  double proc_regen;
  double proc_chance;
  rng_t* rng;
  proc_t* proc;

  seal_of_insight_proc_t( paladin_t* p ) :
    paladin_heal_t( "seal_of_insight_proc", p, p -> find_class_spell( "Seal of Insight" ) ),
    proc_regen( 0.0 ), proc_chance( 0.0 ),
    rng( p -> get_rng( name_str ) ),
    proc( p -> get_proc( name_str ) )
  {
    background  = true;
    paladin_heal_t::proc = true;
    trigger_gcd = timespan_t::zero();
    may_crit = false; //cannot crit

    // spell database info is mostly in effects
    direct_power_mod             = 1.0;
    base_attack_power_multiplier = data().effectN( 1 ).percent();
    base_spell_power_multiplier  = data().effectN( 1 ).percent();

    // needed for weapon speed, I assume
    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0.0;

    // regen is 4% mana
    proc_regen  = data().effectN( 1 ).trigger() ? data().effectN( 1 ).trigger() -> effectN( 2 ).resource( RESOURCE_MANA ) : 0.0;

    // proc chance is now 20 PPM, spell database info still says 15.  
    // Best guess is that the 4th effect describes the additional 5 PPM.
    proc_chance = ppm_proc_chance( data().effectN( 1 ).base_value() + data().effectN(4).base_value() );

    target = player;
  }

  virtual void execute()
  {
    if ( rng -> roll( proc_chance ) )
    {
      proc -> occur();
      paladin_heal_t::execute();
      p() -> resource_gain( RESOURCE_MANA,
                            p() -> resources.base[ RESOURCE_MANA ] * proc_regen,
                            p() -> gains.seal_of_insight );
    }
    else
    {
      update_ready();
    }
  }
};

// Beacon of Light ==========================================================

struct beacon_of_light_t : public paladin_heal_t
{
  beacon_of_light_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "beacon_of_light", p, p -> find_class_spell( "Beacon of Light" ) )
  {
    parse_options( NULL, options_str );

    // Target is required for Beacon
    if ( target_str.empty() )
    {
      sim -> errorf( "Warning %s's \"%s\" needs a target", p -> name(), name() );
      sim -> cancel();
    }

    // Remove the 'dot'
    num_ticks = 0;
  }

  virtual void execute()
  {
    paladin_heal_t::execute();

    p() -> beacon_target = target;
    target -> buffs.beacon_of_light -> trigger();
  }
};

// Divine Light Spell =======================================================

struct divine_light_t : public paladin_heal_t
{
  divine_light_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "divine_light", p, p -> find_class_spell( "Divine Light" ) )
  {
    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    paladin_heal_t::execute();

    p() -> buffs.daybreak -> trigger();
    p() -> buffs.infusion_of_light -> expire();
  }

  virtual timespan_t execute_time()
  {
    timespan_t t = paladin_heal_t::execute_time();

    if ( p() -> buffs.infusion_of_light -> check() )
      t += p() -> buffs.infusion_of_light -> data().effectN( 1 ).time_value();

    return t;
  }

  virtual void schedule_execute( action_state_t* state = 0 )
  {
    paladin_heal_t::schedule_execute( state );

    p() -> buffs.infusion_of_light -> up(); // Buff uptime tracking
  }
};

// Flash of Light Spell =====================================================

struct flash_of_light_t : public paladin_heal_t
{
  flash_of_light_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "flash_of_light", p, p -> find_class_spell( "Flash of Light" ) )
  {
    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    paladin_heal_t::execute();

    p() -> buffs.daybreak -> trigger();
    p() -> buffs.infusion_of_light -> expire();
  }
};

// Holy Light Spell =========================================================

struct holy_light_t : public paladin_heal_t
{
  holy_light_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "holy_light", p, p -> find_class_spell( "Holy Light" ) )
  {
    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    paladin_heal_t::execute();

    p() -> buffs.daybreak -> trigger();
    p() -> buffs.infusion_of_light -> expire();
  }

  virtual timespan_t execute_time()
  {
    timespan_t t = paladin_heal_t::execute_time();

    if ( p() -> buffs.infusion_of_light -> check() )
      t += p() -> buffs.infusion_of_light -> data().effectN( 1 ).time_value();

    return t;
  }

  virtual void schedule_execute( action_state_t* state = 0 )
  {
    paladin_heal_t::schedule_execute( state );

    p() -> buffs.infusion_of_light -> up(); // Buff uptime tracking
  }
};

// Holy Radiance ============================================================

struct holy_radiance_hot_t : public paladin_heal_t
{
  holy_radiance_hot_t( paladin_t* p, const spell_data_t* s ) :
    paladin_heal_t( "holy_radiance", p, s )
  {
    background = true;
    dual = true;
    direct_tick = true;
  }
};

struct holy_radiance_t : public paladin_heal_t
{
  holy_radiance_hot_t* hot;

  holy_radiance_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "holy_radiance", p, p -> find_class_spell( "Holy Radiance" ) ),
    hot( new holy_radiance_hot_t( p, data().effectN( 1 ).trigger() ) )
  {
    parse_options( NULL, options_str );

    // FIXME: This is an AoE Hot, which isn't supported currently
    aoe = data().effectN( 2 ).base_value();
  }

  virtual void tick( dot_t* d )
  {
    paladin_heal_t::tick( d );

    hot -> execute();
  }

  virtual void execute()
  {
    paladin_heal_t::execute();

    p() -> buffs.infusion_of_light -> expire();
  }

  virtual timespan_t execute_time()
  {
    timespan_t t = paladin_heal_t::execute_time();

    if ( p() -> buffs.infusion_of_light -> check() )
      t += p() -> buffs.infusion_of_light -> data().effectN( 1 ).time_value();

    return t;
  }

  virtual void schedule_execute( action_state_t* state = 0 )
  {
    paladin_heal_t::schedule_execute( state );

    p() -> buffs.infusion_of_light -> up(); // Buff uptime tracking
  }
};

// Holy Shock Heal Spell ====================================================

struct holy_shock_heal_t : public paladin_heal_t
{
  timespan_t cd_duration;
  const spell_data_t* scaling_data;

  holy_shock_heal_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "holy_shock_heal", p, p -> find_spell( 20473 ) ), cd_duration( timespan_t::zero() ),
    scaling_data( p -> find_spell( 25914 ) )
  {
    check_spec( PALADIN_HOLY );

    parse_options( NULL, options_str );

    // Heal info is in 25914
    parse_spell_data( *scaling_data );

    cd_duration = cooldown -> duration;
  }

  virtual void execute()
  {
    if ( p() -> buffs.daybreak -> up() )
      cooldown -> duration = timespan_t::zero();

    paladin_heal_t::execute();

    int g = scaling_data -> effectN( 2 ).base_value();
    p() -> resource_gain( RESOURCE_HOLY_POWER,
                          g,
                          p() -> gains.hp_holy_shock );
    if ( p() -> buffs.holy_avenger -> check() )
    {
      p() -> resource_gain( RESOURCE_HOLY_POWER,
                            std::max( ( int ) 0, ( int )( p() -> buffs.holy_avenger -> value() - g ) ),
                            p() -> gains.hp_holy_avenger );
    }

    p() -> buffs.daybreak -> expire();
    cooldown -> duration = cd_duration;

    if ( result == RESULT_CRIT )
      p() -> buffs.infusion_of_light -> trigger();
  }
};

// Lay on Hands Spell =======================================================

struct lay_on_hands_t : public paladin_heal_t
{
  lay_on_hands_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "lay_on_hands", p, p -> find_class_spell( "Lay on Hands" ) )
  {
    parse_options( NULL, options_str );

  }

  virtual double calculate_direct_damage( result_e, int, double, double, double, player_t* )
  {
    return player -> resources.max[ RESOURCE_HEALTH ];
  }

  virtual void execute()
  {
    paladin_heal_t::execute();

    target -> debuffs.forbearance -> trigger();
  }

  virtual bool ready()
  {
    if ( target -> debuffs.forbearance -> check() )
      return false;

    return paladin_heal_t::ready();
  }
};

// Light of Dawn ============================================================

struct light_of_dawn_t : public paladin_heal_t
{
  light_of_dawn_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "light_of_dawn", p, p -> find_class_spell( "Light of Dawn" ) )
  {
    parse_options( NULL, options_str );

    aoe = 6;
  }

  virtual double action_multiplier()
  {
    double am = paladin_heal_t::action_multiplier();

    am *= p() -> holy_power_stacks();

    return am;
  }
};

// Word of Glory Spell ======================================================

struct word_of_glory_t : public paladin_heal_t
{
  word_of_glory_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "word_of_glory", p, p -> find_class_spell( "Word of Glory" ) )
  {
    parse_options( NULL, options_str );

    base_attack_power_multiplier = 0.198;
    base_spell_power_multiplier  = direct_power_mod;
    direct_power_mod = 1.0;

    // Hot is built into the spell, but only becomes active with the glyph
    base_td = 0;
    base_tick_time = timespan_t::zero();
    num_ticks = 0;

    if ( p -> passives.guarded_by_the_light -> ok() )
      trigger_gcd = timespan_t::zero();
  }

  virtual double action_multiplier()
  {
    double am = paladin_heal_t::action_multiplier();

    am *= p() -> holy_power_stacks();

    return am;
  }

  virtual void execute()
  {
    double hopo = p() -> holy_power_stacks();

    paladin_heal_t::execute();

    // Glyph of Word of Glory handling
    if ( p() -> spells.glyph_of_word_of_glory_damage -> ok() )
    {
      p() -> buffs.glyph_of_word_of_glory -> trigger( 1, p() -> spells.glyph_of_word_of_glory_damage -> effectN( 1 ).percent() * hopo );
    }

    // Shield of Glory (Tier 15 protection 2-piece bonus)
  }
};

// Beacon of Light

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

} // end namespace heals

namespace absorbs {

struct paladin_absorb_t : public paladin_spell_base_t< absorb_t >
{
  paladin_absorb_t( const std::string& n, paladin_t* p,
                    const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  { }
};

struct illuminated_healing_t : public paladin_absorb_t
{
  illuminated_healing_t( paladin_t* p ) :
    paladin_absorb_t( "illuminated_healing", p, p -> find_spell( 86273 ) )
  {
    background = true;
    proc = true;
    trigger_gcd = timespan_t::zero();
  }
};

} // end namespace absorbs

// ==========================================================================
// Paladin Character Definition
// ==========================================================================

paladin_td_t::paladin_td_t( player_t* target, paladin_t* paladin ) :
  actor_pair_t( target, paladin )
{
  dots.word_of_glory      = target -> get_dot( "word_of_glory",      paladin );
  dots.holy_radiance      = target -> get_dot( "holy_radiance",      paladin );
  dots.censure            = target -> get_dot( "censure",            paladin );
  dots.execution_sentence = target -> get_dot( "execution_sentence", paladin );

  debuffs_censure = buff_creator_t( *this, "censure", paladin -> find_spell( 31803 ) );
}

// paladin_t::create_action =================================================

action_t* paladin_t::create_action( const std::string& name, const std::string& options_str )
{
  using namespace attacks;
  using namespace spells;
  using namespace heals;
  using namespace absorbs;

  if ( name == "auto_attack"               ) return new auto_melee_attack_t        ( this, options_str );
  if ( name == "avengers_shield"           ) return new avengers_shield_t          ( this, options_str );
  if ( name == "avenging_wrath"            ) return new avenging_wrath_t           ( this, options_str );
  if ( name == "beacon_of_light"           ) return new beacon_of_light_t          ( this, options_str );
  if ( name == "blessing_of_kings"         ) return new blessing_of_kings_t        ( this, options_str );
  if ( name == "blessing_of_might"         ) return new blessing_of_might_t        ( this, options_str );
  if ( name == "consecration"              ) return new consecration_t             ( this, options_str );
  if ( name == "crusader_strike"           ) return new crusader_strike_t          ( this, options_str );
  if ( name == "divine_plea"               ) return new divine_plea_t              ( this, options_str );
  if ( name == "divine_protection"         ) return new divine_protection_t        ( this, options_str );
  if ( name == "divine_shield"             ) return new divine_shield_t            ( this, options_str );
  if ( name == "divine_storm"              ) return new divine_storm_t             ( this, options_str );
  if ( name == "execution_sentence"        ) return new execution_sentence_t       ( this, options_str );
  if ( name == "exorcism"                  ) return new exorcism_t                 ( this, options_str );
  if ( name == "fist_of_justice"           ) return new fist_of_justice_t          ( this, options_str );
  if ( name == "hammer_of_justice"         ) return new hammer_of_justice_t        ( this, options_str );
  if ( name == "hammer_of_wrath"           ) return new hammer_of_wrath_t          ( this, options_str );
  if ( name == "hammer_of_the_righteous"   ) return new hammer_of_the_righteous_t  ( this, options_str );
  if ( name == "holy_avenger"              ) return new holy_avenger_t             ( this, options_str );
  if ( name == "holy_radiance"             ) return new holy_radiance_t            ( this, options_str );
  if ( name == "holy_shock"                ) return new holy_shock_t               ( this, options_str );
  if ( name == "holy_shock_heal"           ) return new holy_shock_heal_t          ( this, options_str );
  if ( name == "holy_wrath"                ) return new holy_wrath_t               ( this, options_str );
  if ( name == "guardian_of_ancient_kings" ) return new guardian_of_ancient_kings_t( this, options_str );
  if ( name == "inquisition"               ) return new inquisition_t              ( this, options_str );
  if ( name == "judgment"                  ) return new judgment_t                 ( this, options_str );
  if ( name == "light_of_dawn"             ) return new light_of_dawn_t            ( this, options_str );
  if ( name == "lights_hammer"             ) return new lights_hammer_t            ( this, options_str );
  if ( name == "rebuke"                    ) return new rebuke_t                   ( this, options_str );
  if ( name == "shield_of_the_righteous"   ) return new shield_of_the_righteous_t  ( this, options_str );
  if ( name == "templars_verdict"          ) return new templars_verdict_t         ( this, options_str );
  if ( name == "holy_prism"                ) return new holy_prism_t               ( this, options_str );
  if ( name == "holy_prism_aoe"            ) return new holy_prism_aoe_t           ( this, options_str );

  action_t* a = 0;
  if ( name == "seal_of_justice"           ) { a = new paladin_seal_t( this, "seal_of_justice",       SEAL_OF_JUSTICE,       options_str );
                                               active_seal_of_justice_proc       = new seal_of_justice_proc_t       ( this );
                                               active_seal_of_justice_aoe_proc   = new seal_of_justice_aoe_proc_t   ( this ); return a; }
  if ( name == "seal_of_insight"           ) { a = new paladin_seal_t( this, "seal_of_insight",       SEAL_OF_INSIGHT,       options_str );
                                               active_seal_of_insight_proc       = new seal_of_insight_proc_t       ( this ); return a; }
  if ( name == "seal_of_righteousness"     ) { a = new paladin_seal_t( this, "seal_of_righteousness", SEAL_OF_RIGHTEOUSNESS, options_str );
                                               active_seal_of_righteousness_proc = new seal_of_righteousness_proc_t ( this ); return a; }
  if ( name == "seal_of_truth"             ) { a = new paladin_seal_t( this, "seal_of_truth",         SEAL_OF_TRUTH,         options_str );
                                               active_seal_of_truth_proc         = new seal_of_truth_proc_t         ( this );
                                               active_seal_of_truth_dot          = new seal_of_truth_dot_t          ( this ); return a; }

  if ( name == "word_of_glory"             ) return new word_of_glory_t            ( this, options_str );
  if ( name == "word_of_glory_damage"      ) return new word_of_glory_damage_t     ( this, options_str );
  if ( name == "holy_light"                ) return new holy_light_t               ( this, options_str );
  if ( name == "flash_of_light"            ) return new flash_of_light_t           ( this, options_str );
  if ( name == "divine_light"              ) return new divine_light_t             ( this, options_str );
  if ( name == "lay_on_hands"              ) return new lay_on_hands_t             ( this, options_str );

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
  }
}

// paladin_t::init_defense ==================================================

void paladin_t::init_defense()
{
  gear.armor *= 1.0 + passives.sanctuary -> effectN( 2 ).percent();

  player_t::init_defense();

  initial.parry_rating_per_strength = initial_rating().parry / 95115.8596; // exact value given by Blizzard

  if ( passives.vengeance -> ok() )
    vengeance_init();
}

// paladin_t::init_base =====================================================

void paladin_t::init_base_stats()
{
  player_t::init_base_stats();

  base.attack_power_per_strength = 2.0;
  base.spell_power_per_intellect = 1.0;

  base.stats.attack_power = level * 3;

  resources.base[ RESOURCE_HOLY_POWER ] = 3 + passives.boundless_conviction -> effectN( 1 ).base_value();

  // FIXME! Level-specific!
  base.miss    = 0.060;
  base.dodge   = 0.0501; //90
  base.parry   = 0.030;  //90
  base.block   = 0.030;  // 90

  //based on http://sacredduty.net/2012/09/14/avoidance-diminishing-returns-in-mop-followup/
  diminished_kfactor    = 0.886;

  diminished_parry_cap = 2.37186;
  diminished_block_cap = 1.5037594692967;
  diminished_dodge_cap = 0.6656744;

  switch ( specialization() )
  {
  case PALADIN_HOLY:
    //base.attack_hit += 0; // TODO spirit -> hit talents.enlightened_judgments
    //base.spell_hit  += 0; // TODO spirit -> hit talents.enlightened_judgments
    break;
  default:
    break;
  }
}

// paladin_t::reset =========================================================

void paladin_t::reset()
{
  player_t::reset();

  last_judgement_target = 0;
  active_seal = SEAL_NONE;
  bok_up      = false;
  bom_up      = false;
  last_extra_regen = timespan_t::zero();
}

// paladin_t::init_gains ====================================================

void paladin_t::init_gains()
{
  player_t::init_gains();

  gains.divine_plea                 = get_gain( "divine_plea"            );
  gains.extra_regen                 = get_gain( ( specialization() == PALADIN_RETRIBUTION ) ? "sword_of_light" : "guarded_by_the_light" );
  gains.judgments_of_the_wise       = get_gain( "judgments_of_the_wise"  );
  gains.sanctuary                   = get_gain( "sanctuary"              );
  gains.seal_of_insight             = get_gain( "seal_of_insight"        );
  gains.glyph_divine_storm          = get_gain( "glyph_of_divine_storm"  );

  // Holy Power
  gains.hp_blessed_life             = get_gain( "holy_power_blessed_life" );
  gains.hp_crusader_strike          = get_gain( "holy_power_crusader_strike" );
  gains.hp_divine_plea              = get_gain( "holy_power_divine_plea" );
  gains.hp_divine_purpose           = get_gain( "holy_power_divine_purpose" );
  gains.hp_exorcism                 = get_gain( "holy_power_exorcism" );
  gains.hp_grand_crusader           = get_gain( "holy_power_grand_crusader" );
  gains.hp_hammer_of_the_righteous  = get_gain( "holy_power_hammer_of_the_righteous" );
  gains.hp_hammer_of_wrath          = get_gain( "holy_power_hammer_of_wrath" );
  gains.hp_holy_avenger             = get_gain( "holy_power_holy_avenger" );
  gains.hp_holy_shock               = get_gain( "holy_power_holy_shock" );
  gains.hp_judgments_of_the_bold    = get_gain( "holy_power_judgments_of_the_bold" );
  gains.hp_judgments_of_the_wise    = get_gain( "holy_power_judgments_of_the_wise" );
  gains.hp_pursuit_of_justice       = get_gain( "holy_power_pursuit_of_justice" );
  gains.hp_tower_of_radiance        = get_gain( "holy_power_tower_of_radiance" );
  gains.hp_judgment                 = get_gain( "holy_power_judgment" );
}

// paladin_t::init_procs ====================================================

void paladin_t::init_procs()
{
  player_t::init_procs();

  procs.eternal_glory            = get_proc( "eternal_glory"                  );
  procs.judgments_of_the_bold    = get_proc( "judgments_of_the_bold"          );
  procs.parry_haste              = get_proc( "parry_haste"                    );
  procs.the_art_of_war           = get_proc( "the_art_of_war"                 );
  procs.wasted_art_of_war        = get_proc( "wasted_art_of_war"              );
}

// paladin_t::init_scaling ==================================================

void paladin_t::init_scaling()
{
  player_t::init_scaling();

  specialization_e tree = specialization();

  // Technically prot and ret scale with int and sp too, but it's so minor it's not worth the sim time.
  scales_with[ STAT_INTELLECT   ] = ( tree == PALADIN_HOLY );
  scales_with[ STAT_SPIRIT      ] = ( tree == PALADIN_HOLY );
  scales_with[ STAT_SPELL_POWER ] = ( tree == PALADIN_HOLY );

  if ( primary_role() == ROLE_TANK )
  {
    scales_with[ STAT_PARRY_RATING ] = true;
    scales_with[ STAT_BLOCK_RATING ] = true;
    scales_with[ STAT_STRENGTH     ] = true;
  }
}

// paladin_t::decode_set ====================================================

int paladin_t::decode_set( item_t& item )
{
  if ( item.slot != SLOT_HEAD      &&
       item.slot != SLOT_SHOULDERS &&
       item.slot != SLOT_CHEST     &&
       item.slot != SLOT_HANDS     &&
       item.slot != SLOT_LEGS      )
  {
    return SET_NONE;
  }

  const char* s = item.name();

  if ( strstr( s, "_of_radiant_glory" ) )
  {
    bool is_melee = ( strstr( s, "helmet"        ) ||
                      strstr( s, "pauldrons"     ) ||
                      strstr( s, "battleplate"   ) ||
                      strstr( s, "legplates"     ) ||
                      strstr( s, "gauntlets"     ) );

    bool is_tank = ( strstr( s, "faceguard"      ) ||
                     strstr( s, "shoulderguards" ) ||
                     strstr( s, "chestguard"     ) ||
                     strstr( s, "legguards"      ) ||
                     strstr( s, "handguards"     ) );

    bool is_heal = ( strstr( s, "headguard"      ) ||
                     strstr( s, "mantle"         ) ||
                     strstr( s, "breastplate"    ) ||
                     strstr( s, "greaves"        ) ||
                     strstr( s, "gloves"         ) );

    if ( is_melee  ) return SET_T13_MELEE;
    if ( is_tank   ) return SET_T13_TANK;
    if ( is_heal   ) return SET_T13_HEAL;
  }

  if ( strstr( s, "white_tiger_" ) )
  {
    bool is_melee = ( strstr( s, "helmet"        ) ||
                      strstr( s, "pauldrons"     ) ||
                      strstr( s, "battleplate"   ) ||
                      strstr( s, "legplates"     ) ||
                      strstr( s, "gauntlets"     ) );

    bool is_tank = ( strstr( s, "faceguard"      ) ||
                     strstr( s, "shoulderguards" ) ||
                     strstr( s, "chestguard"     ) ||
                     strstr( s, "legguards"      ) ||
                     strstr( s, "handguards"     ) );

    bool is_heal = ( strstr( s, "headguard"      ) ||
                     strstr( s, "mantle"         ) ||
                     strstr( s, "breastplate"    ) ||
                     strstr( s, "greaves"        ) ||
                     strstr( s, "gloves"         ) );

    if ( is_melee  ) return SET_T14_MELEE;
    if ( is_tank   ) return SET_T14_TANK;
    if ( is_heal   ) return SET_T14_HEAL;
  }

  if ( strstr( s, "lightning_emperor" ) )
  {
    bool is_melee = ( strstr( s, "helmet"        ) ||
                      strstr( s, "pauldrons"     ) ||
                      strstr( s, "battleplate"   ) ||
                      strstr( s, "legplates"     ) ||
                      strstr( s, "gauntlets"     ) );

    bool is_tank = ( strstr( s, "faceguard"      ) ||
                     strstr( s, "shoulderguards" ) ||
                     strstr( s, "chestguard"     ) ||
                     strstr( s, "legguards"      ) ||
                     strstr( s, "handguards"     ) );

    bool is_heal = ( strstr( s, "headguard"      ) ||
                     strstr( s, "mantle"         ) ||
                     strstr( s, "breastplate"    ) ||
                     strstr( s, "greaves"        ) ||
                     strstr( s, "gloves"         ) );

    if ( is_melee  ) return SET_T15_MELEE;
    if ( is_tank   ) return SET_T15_TANK;
    if ( is_heal   ) return SET_T15_HEAL;
  }

  if ( strstr( s, "gladiators_ornamented_"  ) ) return SET_PVP_HEAL;
  if ( strstr( s, "gladiators_scaled_"      ) ) return SET_PVP_MELEE;

  return SET_NONE;
}

// paladin_t::init_buffs ====================================================

void paladin_t::create_buffs()
{
  player_t::create_buffs();

  // Glyphs
  buffs.blessed_life           = buff_creator_t( this, "glyph_blessed_life", glyphs.blessed_life )
                                 .cd( timespan_t::from_seconds( glyphs.blessed_life -> effectN( 2 ).base_value() ) );
  buffs.double_jeopardy        = buff_creator_t( this, "glyph_double_jeopardy", glyphs.double_jeopardy )
                                 .duration( find_spell( glyphs.double_jeopardy -> effectN( 1 ).trigger_spell_id() ) -> duration() )
                                 .default_value( find_spell( glyphs.double_jeopardy -> effectN( 1 ).trigger_spell_id() ) -> effectN( 1 ).percent() );
  buffs.glyph_templars_verdict       = buff_creator_t( this, "glyph_templars_verdict", glyphs.templars_verdict )
                                       .duration( find_spell( glyphs.templars_verdict -> effectN( 1 ).trigger_spell_id() ) -> duration() )
                                       .default_value( find_spell( glyphs.templars_verdict -> effectN( 1 ).trigger_spell_id() ) -> effectN( 1 ).percent() );

  buffs.glyph_of_word_of_glory = buff_creator_t( this, "glyph_word_of_glory", spells.glyph_of_word_of_glory_damage ).add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  // Talents
  buffs.divine_purpose         = buff_creator_t( this, "divine_purpose", find_talent_spell( "Divine Purpose" ) )
                                 .duration( find_spell( find_talent_spell( "Divine Purpose" ) -> effectN( 1 ).trigger_spell_id() ) -> duration() );
  buffs.holy_avenger           = buff_creator_t( this, "holy_avenger", find_talent_spell( "Holy Avenger" ) ).cd( timespan_t::zero() ); // Let the ability handle the CD

  // General
  buffs.avenging_wrath         = buff_creator_t( this, "avenging_wrath", find_class_spell( "Avenging Wrath" ) )
                                 .cd( timespan_t::zero() ) // Let the ability handle the CD
                                 .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  if ( find_talent_spell( "Sanctified Wrath" ) -> ok() )
  {
    buffs.avenging_wrath -> buff_duration *= 1.0 + find_talent_spell( "Sanctified Wrath" ) -> effectN( 2 ).percent();
  }

  buffs.divine_protection      = buff_creator_t( this, "divine_protection", find_class_spell( "Divine Protection" ) ).cd( timespan_t::zero() ); // Let the ability handle the CD
  buffs.divine_shield          = buff_creator_t( this, "divine_shield", find_class_spell( "Divine Shield" ) )
                                 .cd( timespan_t::zero() ) // Let the ability handle the CD
                                 .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  // Holy
  buffs.daybreak               = buff_creator_t( this, "daybreak", find_class_spell( "Daybreak" ) );
  buffs.divine_plea            = buff_creator_t( this, "divine_plea", find_class_spell( "Divine Plea" ) ).cd( timespan_t::zero() ); // Let the ability handle the CD
  buffs.infusion_of_light      = buff_creator_t( this, "infusion_of_light", find_class_spell( "Infusion of Light" ) );

  // Prot
  buffs.gotak_prot             = buff_creator_t( this, "guardian_of_the_ancient_kings", find_class_spell( "Guardian of Ancient Kings", std::string(), PALADIN_PROTECTION ) );
  buffs.grand_crusader = buff_creator_t( this, "grand_crusader" ).spell( passives.grand_crusader -> effectN( 1 ).trigger() ).chance( passives.grand_crusader -> proc_chance() );
  buffs.shield_of_the_righteous = buff_creator_t( this, "shield_of_the_righteous" ).spell( find_spell( 132403 ) );
  buffs.ardent_defender = buff_creator_t( this, "ardent_defender" ).spell( find_specialization_spell( "Ardent Defender" ) );
  buffs.shield_of_glory = buff_creator_t( this, "shield_of_glory" ).spell( find_spell( 138242 ) );

  // Ret
  buffs.ancient_power          = buff_creator_t( this, "ancient_power", passives.ancient_power ).add_invalidate( CACHE_STRENGTH );
  buffs.inquisition            = buff_creator_t( this, "inquisition", find_class_spell( "Inquisition" ) ).add_invalidate( CACHE_CRIT ).add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.judgments_of_the_wise  = buff_creator_t( this, "judgments_of_the_wise", find_specialization_spell( "Judgments of the Wise" ) );
  buffs.tier15_2pc_melee       = buff_creator_t( this, "tier15_2pc_melee", find_spell( 138162 ) )
                                 .default_value( find_spell( 138162 ) -> effectN( 1 ).percent() )
                                 .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.tier15_4pc_melee       = buff_creator_t( this, "tier15_4pc_melee", find_spell( 138164 ) )
                                 .chance( find_spell( 138164 ) -> effectN( 1 ).percent() );
}

// Protection Combat Action Priority List

void paladin_t::generate_action_prio_list_prot()
{
  //precombat first
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  //Flask
  if ( sim -> allow_flasks && level >= 80 )
  {
    std::string flask_action = "flask,type=";
    flask_action += (level > 85) ? "earth" : "steelskin";
    precombat -> add_action( flask_action );
  }

  // Food
  if (sim -> allow_food && level >= 80 )
  {
    std::string food_action = "food,type=";
      food_action += ( level > 85 ) ? "chun_tian_spring_rolls" : "seafood_magnifique_feast";
      precombat -> add_action( food_action );
  }

  precombat -> add_action( this, "Blessing of Kings", "if=(!aura.str_agi_int.up)&(aura.mastery.up)" );
  precombat -> add_action( this, "Blessing of Might", "if=!aura.mastery.up" );
  precombat -> add_action( this, "Seal of Insight");

  // Snapshot stats
  precombat -> add_action( "snapshot_stats",  "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  // Pre-potting (disabled for now)
  /*
  if (sim -> allow_potions && level >= 80 )
    precombat -> add_action( ( level > 85 ) ? "mogu_power_potion" : "golemblood_potion" );

  */

  // action priority list
  action_priority_list_t* def = get_action_priority_list( "default" );

  // potion placeholder; need to think about realistic conditions
  // def -> add_action( "potion_of_the_mountains,if=!in_combat|buff.bloodlust.react|target.time_to_die<=60" );

  def -> add_action( "/auto_attack" );

  int num_items = ( int ) items.size();
  for ( int i=0; i < num_items; i++ )
  {
    if ( items[ i ].parsed.use.active() )
    {
      def -> add_action ( "/use_item,name=" + items[ i ].name_str );
    }
  }

  std::vector<std::string> profession_actions = get_profession_actions(); 
  for ( size_t i=0; i < profession_actions.size(); i++ )
    def -> add_action( profession_actions[ i ] );

  std::vector<std::string> racial_actions = get_racial_actions();
  for ( size_t i=0; i< racial_actions.size(); i++ )
    def -> add_action( racial_actions[ i ] );

  def -> add_action( this, "Avenging Wrath" );
  def -> add_action( this, "Guardian of Ancient Kings", "if=health.pct<=30" );
  def -> add_action( this, "Shield of the Righteous", "if=holy_power>=3" );
  def -> add_action( this, "Hammer of the Righteous", "if=target.debuff.weakened_blows.down" );
  def -> add_action( this, "Crusader Strike" );
  def -> add_action( this, "Judgment" );
  def -> add_action( this, "Avenger's Shield" );
  def -> add_action( this, "Hammer of Wrath" );
  def -> add_action( this, "Holy Wrath" );
  def -> add_action( this, "Consecration", "if=target.debuff.flying.down&!ticking" );
}

void paladin_t::generate_action_prio_list_ret()
{
  // placeholder, eventually going to move the monstrosity in init_actions() into here.
}

// paladin_t::init_actions ==================================================

void paladin_t::init_actions()
{
  // sanity check - Holy not implemented yet
  if ( specialization() != PALADIN_RETRIBUTION && specialization() != PALADIN_PROTECTION )
  {
    if ( ! quiet )
      sim -> errorf( "Player %s's role (%s) or spec(%s) isn't supported yet.",
                     name(), util::role_type_string( primary_role() ), dbc::specialization_string( specialization() ).c_str() );
    quiet = true;
    return;
  }

  // sanity check - can't do anything w/o main hand weapon equipped
  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( !quiet )
      sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  active_hand_of_light_proc          = new attacks::hand_of_light_proc_t         ( this );
  ancient_fury_explosion             = new spells::ancient_fury_t               ( this );

  // create action priority lists
  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    std::string& precombat_list = get_action_priority_list( "precombat" ) -> action_list_str;

    switch ( specialization() )
    {
    case PALADIN_RETRIBUTION:
    {
      if ( level > 85 )
      {
        if ( sim -> allow_flasks )
          precombat_list += "/flask,type=winters_bite";
        if ( sim -> allow_food )
          precombat_list += "/food,type=black_pepper_ribs_and_shrimp";
      }
      else if ( level >= 80 )
      {
        if ( sim -> allow_flasks )
          precombat_list += "/flask,type=titanic_strength";
        if ( sim -> allow_food )
          precombat_list += "/food,type=beer_basted_crocolisk";
      }

      if ( find_class_spell( "Blessing of Kings" ) -> ok() )
        precombat_list += "/blessing_of_kings,if=!aura.str_agi_int.up";
      if ( find_class_spell( "Blessing of Might" ) -> ok() )
      {
        precombat_list += "/blessing_of_might,if=!aura.mastery.up";
        if ( find_class_spell( "Blessing of Kings" ) -> ok() )
          precombat_list += "&!aura.str_agi_int.up";
      }

      if ( find_class_spell( "Seal of Truth" ) -> ok() )
      {
        if ( find_class_spell( "Seal of Righteousness" ) -> ok() )
        {
          precombat_list += "/seal_of_righteousness,if=active_enemies>=4";
          precombat_list += "/seal_of_truth,if=active_enemies<4";
        }
        else
        {
          precombat_list += "/seal_of_truth";
        }
      }

      precombat_list += "/snapshot_stats";

      if ( sim -> allow_potions )
      {
        if ( level > 85 )
        {
          precombat_list += "/mogu_power_potion";
        }
        else if ( level >= 80 )
        {
          precombat_list += "/golemblood_potion";
        }
      }

      if ( find_class_spell( "Rebuke" ) -> ok() )
        action_list_str = "/rebuke";

      if ( sim -> allow_potions )
      {
        if ( level > 85 )
        {
          action_list_str += "/mogu_power_potion,if=(buff.bloodlust.react|(buff.ancient_power.up&buff.avenging_wrath.up)|target.time_to_die<=40)";
        }
        else if ( level >= 80 )
        {
          action_list_str += "/golemblood_potion,if=buff.bloodlust.react|(buff.ancient_power.up&buff.avenging_wrath.up)|target.time_to_die<=40";
        }
      }

      // This should<tm> get Censure up before the auto attack lands
      action_list_str += "/auto_attack";

      /*if ( find_class_spell( "Judgment" ) -> ok() && find_specialization_spell( "Judgments of the Bold" ) -> ok() )
      {
        action_list_str += "/judgment,if=!target.debuff.physical_vulnerability.up|target.debuff.physical_vulnerability.remains<6";
      }*/

      if ( find_class_spell( "Inquisition" ) -> ok() && find_talent_spell( "Divine Purpose" ) -> ok() )
      {
        action_list_str += "/inquisition,if=buff.inquisition.down&(holy_power>=1|buff.divine_purpose.react)";
      }

      if ( find_class_spell( "Inquisition" ) -> ok() )
      {
        action_list_str += "/inquisition,if=(buff.inquisition.down|buff.inquisition.remains<=2)&(holy_power>=3|target.time_to_die<holy_power*10";
        if ( find_talent_spell( "Divine Purpose" ) -> ok() )
          action_list_str += "|buff.divine_purpose.react)";
        else
          action_list_str += ")";
      }

      if ( find_class_spell( "Avenging Wrath" ) -> ok() )
      {
        action_list_str += "/avenging_wrath,if=buff.inquisition.up";
        if ( find_class_spell( "Guardian Of Ancient Kings", std::string(), PALADIN_RETRIBUTION ) -> ok() & !( find_talent_spell( "Sanctified Wrath" ) -> ok() ) )
          action_list_str += "&(cooldown.guardian_of_ancient_kings.remains<291)";

        /*int num_items = ( int ) items.size();
        int j = 0;
        for ( int i=0; i < num_items; i++ )
        {
          if ( ( items[ i ].name_str() == "lei_shens_final_orders" ) ||
               ( items[ i ].name_str() == "darkmist_vortex"        ) )
          {
            if ( j == 0 )
            {
              action_list_str += "&(";
            }
            else
            {
              action_list_str += "|";
            }
            action_list_str += "buff.";
            action_list_str += items[ i ].name_str();
            action_list_str += ".up";
            j++;
          }
        }
        if ( j > 0 )
        {
          action_list_str += ")";
        }*/
      }

      if ( find_class_spell( "Guardian Of Ancient Kings", std::string(), PALADIN_RETRIBUTION ) -> ok() )
      {
        action_list_str += "/guardian_of_ancient_kings";

        if ( find_class_spell( "Avenging Wrath" ) -> ok() & !( find_talent_spell( "Sanctified Wrath" ) -> ok() ) )
        {
          action_list_str += ",if=cooldown.avenging_wrath.remains<10";
        }
        if ( find_class_spell( "Avenging Wrath" ) -> ok() & find_talent_spell( "Sanctified Wrath" ) -> ok() )
        {
          action_list_str += ",if=buff.avenging_wrath.up";
        }
      }

      if ( find_talent_spell( "Holy Avenger" ) -> ok() )
      {
        action_list_str += "/holy_avenger,if=buff.inquisition.up";
        if ( find_class_spell( "Guardian Of Ancient Kings", std::string(), PALADIN_RETRIBUTION ) -> ok() )
          action_list_str += "&(cooldown.guardian_of_ancient_kings.remains<289)";
        action_list_str += "&holy_power<=2";
      }

      int num_items = ( int ) items.size();
      for ( int i=0; i < num_items; i++ )
      {
        if ( items[ i ].parsed.use.active() )
        {
          action_list_str += "/use_item,name=";
          action_list_str += items[ i ].name();
          if ( find_talent_spell( "Sanctified Wrath" ) -> ok() )
          {
            action_list_str += ",if=buff.inquisition.up";
          }
          else
          {
            action_list_str += ",if=buff.inquisition.up&time>=14";
          }
        }
      }
      action_list_str += init_use_profession_actions();
      action_list_str += init_use_racial_actions();

      if ( find_talent_spell( "Execution Sentence" ) -> ok() )
      {
        if ( find_talent_spell( "Sanctified Wrath" ) -> ok() )
        {
          action_list_str += "/execution_sentence,if=buff.inquisition.up";
        }
        else
        {
          action_list_str += "/execution_sentence,if=buff.inquisition.up&time>=15";
        }
      }

      if ( find_talent_spell( "Light's Hammer" ) -> ok() )
      {
        if ( find_talent_spell( "Sanctified Wrath" ) -> ok() )
        {
          action_list_str += "/lights_hammer,if=buff.inquisition.up";
        }
        else
        {
          action_list_str += "/lights_hammer,if=buff.inquisition.up&time>=15";
        }
      }

      if ( find_class_spell( "Divine Storm" ) -> ok() )
      {
        action_list_str += "/divine_storm,if=active_enemies>=2&(holy_power=5";
        if ( find_talent_spell( "Divine Purpose" ) -> ok() )
        {
          action_list_str += "|buff.divine_purpose.react";
        }
        if ( find_talent_spell( "Holy Avenger" ) -> ok() )
        {
          action_list_str +="|(buff.holy_avenger.up&holy_power>=3)";
        }
        action_list_str += ")";
      }

      if ( find_class_spell( "Templar's Verdict" ) -> ok() )
      {
        action_list_str += "/templars_verdict,if=holy_power=5";
        if ( find_talent_spell( "Divine Purpose" ) -> ok() )
        {
          action_list_str += "|buff.divine_purpose.react";
        }
        if ( find_talent_spell( "Holy Avenger" ) -> ok() )
        {
          action_list_str +="|(buff.holy_avenger.up&holy_power>=3)";
        }
      }

      if ( find_class_spell( "Hammer of Wrath" ) -> ok() )
      {
        action_list_str += "/hammer_of_wrath";
        action_list_str += "/wait,sec=cooldown.hammer_of_wrath.remains,if=cooldown.hammer_of_wrath.remains>0&cooldown.hammer_of_wrath.remains<=";
        if ( find_talent_spell( "Sanctified Wrath" ) -> ok() )
          action_list_str += "0.2";
        else
          action_list_str += "0.1";
      }

      if ( find_class_spell( "Exorcism" ) -> ok() )
      {
        action_list_str += "/exorcism";
        action_list_str += "/wait,sec=cooldown.exorcism.remains,if=cooldown.exorcism.remains>0&cooldown.exorcism.remains<=0.2";
      }

      if ( find_class_spell( "Judgment" ) -> ok() )
        action_list_str += "/judgment,if=!(set_bonus.tier15_4pc_melee)&(target.health.pct<=20|buff.avenging_wrath.up)&active_enemies<2";

      if ( find_class_spell( "Hammer of the Righteous" ) -> ok() )
        action_list_str += "/hammer_of_the_righteous,if=active_enemies>=4";

      if ( find_class_spell( "Crusader Strike" ) -> ok() )
      {
        action_list_str += "/crusader_strike";
        action_list_str += "/wait,sec=cooldown.crusader_strike.remains,if=cooldown.crusader_strike.remains>0&cooldown.crusader_strike.remains<=0.2";
      }

      if ( find_class_spell( "Judgment" ) -> ok() )
      {
        action_list_str += "/judgment,target=2,if=active_enemies>=2&buff.glyph_double_jeopardy.up";
        action_list_str += "/judgment";
      }

      if ( find_class_spell( "Divine Storm" ) -> ok() )
        action_list_str += "/divine_storm,if=active_enemies>=2&buff.inquisition.remains>4";

      if ( find_class_spell( "Templar's Verdict" ) -> ok() )
        action_list_str += "/templars_verdict,if=buff.inquisition.remains>4";

      if ( find_talent_spell( "Holy Prism" ) -> ok() )
        action_list_str += "/holy_prism";
    }
    break;
    // for prot, call subroutine
    case PALADIN_PROTECTION:
		  generate_action_prio_list_prot(); // PROT
		  break;
    case PALADIN_HOLY:
    {
#if 0
      if ( level > 80 )
      {
        action_list_str = "flask,type=draconic_mind/food,type=severed_sagefish_head";
        action_list_str += "/volcanic_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<=60";
      }
      else
      {
        action_list_str = "flask,type=stoneblood/food,type=dragonfin_filet";
        action_list_str += "/indestructible_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<=60";
      }
      action_list_str += "/seal_of_insight";
      action_list_str += "/snapshot_stats";
      action_list_str += "/auto_attack";
      int num_items = ( int ) items.size();
      for ( int i=0; i < num_items; i++ )
      {
        if ( items[ i ].use.active() )
        {
          action_list_str += "/use_item,name=";
          action_list_str += items[ i ].name();
        }
      }
      action_list_str += init_use_profession_actions();
      action_list_str += init_use_racial_actions();
      action_list_str += "/avenging_wrath";
      action_list_str += "/judgment,if=buff.judgments_of_the_pure.remains<10";
      action_list_str += "/word_of_glory,if=holy_power>2";
      action_list_str += "/holy_shock_heal";
      action_list_str += "/divine_light,if=mana_pct>75";
      action_list_str += "/divine_plea,if=mana_pct<75";
      action_list_str += "/holy_light";
#endif
    }
    break;
    default:
      if ( level > 80 )
      {
        action_list_str = "flask,type=draconic_mind/food,type=severed_sagefish_head";
        action_list_str += "/volcanic_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<=60";
      }
      action_list_str += "/snapshot_stats";
      action_list_str += "/auto_attack";
      break;
    }
    action_list_default = 1;
  }

  player_t::init_actions();
}

void paladin_t::init_spells()
{
  player_t::init_spells();

  // Talents
  talents.divine_purpose          = find_talent_spell( "Divine Purpose" );

  // Spells
  spells.guardian_of_ancient_kings_ret = find_class_spell( "Guardian Of Ancient Kings", std::string(), PALADIN_RETRIBUTION );

  // Masteries
  passives.divine_bulwark         = find_mastery_spell( PALADIN_PROTECTION );
  passives.hand_of_light          = find_mastery_spell( PALADIN_RETRIBUTION );
  passives.illuminated_healing    = find_mastery_spell( PALADIN_HOLY );
  // Passives

  // Shared Passives
  passives.boundless_conviction   = find_spell( 115675 ); // find_specialization_spell( "Boundless Conviction" ); FIX-ME: (not in our spell lists for some reason)
  passives.plate_specialization   = find_specialization_spell( "Plate Specialization" );
  passives.sanctity_of_battle     = find_spell( 25956 ); // FIX-ME: find_specialization_spell( "Sanctity of Battle" )   (not in spell lists yet)
  passives.seal_of_insight        = find_class_spell( "Seal of Insight" );

  // Holy Passives

  // Prot Passives
  passives.judgments_of_the_wise  = find_specialization_spell( "Judgments of the Wise" );
  passives.guarded_by_the_light   = find_specialization_spell( "Guarded by the Light" );
  passives.vengeance              = find_specialization_spell( "Vengeance" );
  passives.sanctuary              = find_specialization_spell( "Sanctuary" );
  passives.grand_crusader         = find_specialization_spell( "Grand Crusader" );

  // Ret Passives
  passives.ancient_fury           = find_spell( spells.guardian_of_ancient_kings_ret -> ok() ? 86704 : 0 );
  passives.ancient_power          = find_spell( spells.guardian_of_ancient_kings_ret -> ok() ? 86700 : 0 );
  passives.judgments_of_the_bold  = find_specialization_spell( "Judgments of the Bold" );
  passives.sword_of_light         = find_specialization_spell( "Sword of Light" );
  passives.sword_of_light_value   = find_spell( passives.sword_of_light -> ok() ? 20113 : 0 );
  passives.the_art_of_war         = find_specialization_spell( "The Art of War" );

  // Glyphs
  glyphs.blessed_life             = find_glyph_spell( "Glyph of Blessed Life" );
  glyphs.divine_protection        = find_glyph_spell( "Glyph of Divine Protection" );
  glyphs.divine_storm             = find_glyph_spell( "Glyph of Divine Storm" );
  glyphs.double_jeopardy          = find_glyph_spell( "Glyph of Double Jeopardy" );
  glyphs.mass_exorcism            = find_glyph_spell( "Glyph of Mass Exorcism" );
  glyphs.templars_verdict         = find_glyph_spell( "Glyph of Templar's Verdict" );
  glyphs.focused_shield           = find_glyph_spell( "Glyph of Focused Shield" );
  glyphs.harsh_words              = find_glyph_spell( "Glyph of Harsh Words" );
  glyphs.immediate_truth          = find_glyph_spell( "Glyph of Immediate Truth" );
  glyphs.inquisition              = find_glyph_spell( "Glyph of Inquisition"     );
  glyphs.word_of_glory            = find_glyph_spell( "Glyph of Word of Glory"   );

  spells.glyph_of_word_of_glory_damage = glyphs.word_of_glory -> ok() ? find_spell( 115522 ) : spell_data_t::not_found();

  if ( specialization() == PALADIN_RETRIBUTION )
  {
    extra_regen_period  = passives.sword_of_light -> effectN( 2 ).period();
    extra_regen_percent = passives.sword_of_light -> effectN( 2 ).percent();
  }
  else if ( specialization() == PALADIN_PROTECTION )
  {
    extra_regen_period  = passives.guarded_by_the_light -> effectN( 3 ).period();
    extra_regen_percent = passives.guarded_by_the_light -> effectN( 3 ).percent();
  }

  if ( find_class_spell( "Beacon of Light" ) -> ok() )
    active_beacon_of_light = new heals::beacon_of_light_heal_t( this );

  if ( passives.illuminated_healing -> ok() )
    active_illuminated_healing = new absorbs::illuminated_healing_t( this );

  // Tier Bonuses
  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //  C2P    C4P     M2P     M4P     T2P     T4P     H2P     H4P
    {     0,     0, 105765, 105820, 105800, 105744, 105743, 105798 }, // Tier13
    {     0,     0, 123108,  70762, 123104, 123107, 123102, 123103 }, // Tier14
    {     0,     0, 138159, 138164, 138238, 138244, 138291, 138292 }, // Tier15
  };

  sets = new set_bonus_array_t( this, set_bonuses );

  // Holy Mastery uses effect#2 by default
  if ( specialization() == PALADIN_HOLY )
  {
    _mastery = &find_mastery_spell( specialization() ) -> effectN( 2 );
  }
}

// paladin_t::primary_role ==================================================

role_e paladin_t::primary_role()
{
  if ( player_t::primary_role() == ROLE_DPS || specialization() == PALADIN_RETRIBUTION )
    return ROLE_HYBRID;

  if ( player_t::primary_role() == ROLE_TANK || specialization() == PALADIN_PROTECTION  )
    return ROLE_TANK;

  if ( player_t::primary_role() == ROLE_HEAL || specialization() == PALADIN_HOLY )
    return ROLE_HEAL;

  return ROLE_HYBRID;
}

// paladin_t::composite_attribute_multiplier ================================

double paladin_t::composite_attribute_multiplier( attribute_e attr )
{
  double m = player_t::composite_attribute_multiplier( attr );

  // Ancient Power buffs STR
  if ( attr == ATTR_STRENGTH && buffs.ancient_power -> check() )
  {
    m *= 1.0 + buffs.ancient_power -> stack() * passives.ancient_power -> effectN( 1 ).percent();
  }
  // Guarded by the Light buffs STA
  if ( attr == ATTR_STAMINA )
  {
    m *= 1.0 + passives.guarded_by_the_light -> effectN( 2 ).percent();
  }

  return m;
}

// paladin_t::composite_attack_crit ===================================

double paladin_t::composite_melee_crit()
{
  double m = player_t::composite_melee_crit();

  if ( buffs.inquisition -> check() )
  {
    m += buffs.inquisition -> data().effectN( 3 ).percent();
  }

  return m;
}

// paladin_t::composite_spell_crit ===================================

double paladin_t::composite_spell_crit()
{
  double m = player_t::composite_spell_crit();

  if ( buffs.inquisition -> check() )
  {
    m += buffs.inquisition -> data().effectN( 3 ).percent();
  }

  return m;
}

// paladin_t::composite_player_multiplier ===================================

double paladin_t::composite_player_multiplier( school_e school )
{
  double m = player_t::composite_player_multiplier( school );

  // These affect all damage done by the paladin
  // Avenging Wrath buffs everything
  m *= 1.0 + buffs.avenging_wrath -> value();

  // Inquisition buffs holy damage only
  if ( dbc::is_school( school, SCHOOL_HOLY ) )
  {
    if ( buffs.inquisition -> up() )
    {
      m *= 1.0 + buffs.inquisition -> value();
    }

    m *= 1.0 + buffs.tier15_2pc_melee -> value();
  }

  // Divine Shield reduces everything
  if ( buffs.divine_shield -> up() )
    m *= 1.0 + buffs.divine_shield -> data().effectN( 1 ).percent();

  // Glyph of Word of Glory buffs everything
  if ( buffs.glyph_of_word_of_glory -> check() )
    m *= 1.0 + buffs.glyph_of_word_of_glory -> value();

  return m;
}

// paladin_t::composite_spell_power =========================================

double paladin_t::composite_spell_power( school_e school )
{
  double sp = player_t::composite_spell_power( school );
  
  // For Protection and Retribution, SP is fixed to AP/2 by passives
  switch ( specialization() )
  {
  case PALADIN_PROTECTION:
    sp = passives.guarded_by_the_light -> effectN( 1 ).percent() * cache.attack_power() * composite_attack_power_multiplier();
    break;
  case PALADIN_RETRIBUTION:
    sp = passives.sword_of_light -> effectN( 1 ).percent() * cache.attack_power() * composite_attack_power_multiplier();
    break;
  default:
    break;
  }
  return sp;
}

// paladin_t::composite_spell_power_multiplier ==============================

double paladin_t::composite_spell_power_multiplier()
{
  if ( passives.sword_of_light -> ok() || passives.guarded_by_the_light -> ok() )
    return 1.0;

  return player_t::composite_spell_power_multiplier();
}

// paladin_t::composite_spell_speed ==============================

double paladin_t::composite_spell_speed()
{
  double m = player_t::composite_spell_speed();

  if ( active_seal == SEAL_OF_INSIGHT )
  {
    m /= ( 1.0 + passives.seal_of_insight -> effectN( 3 ).percent() );
  }

  return m;
}

// paladin_t::composite_tank_block ==========================================

double paladin_t::composite_block()
{
  // need to reproduce most of player_t::composite_block() because mastery -> block conversion is affected by DR
  // could modify player_t::composite_block() to take one argument if willing to change references in all other files
  double block_by_rating = current.stats.block_rating / current_rating().block;

  // add block from Divine Bulwark
  block_by_rating += get_divine_bulwark();

  double b = current.block;

  // calculate diminishing returns and add to b
  if ( block_by_rating > 0 )
  {
    //the block by rating gets rounded because that's how blizzard rolls...
    b += 1/ ( 1 / diminished_block_cap + diminished_kfactor / ( util::round( 12800 * block_by_rating ) / 12800 ) );
  }

  // Guarded by the Light block not affected by diminishing returns
  b += passives.guarded_by_the_light -> effectN( 6 ).percent();

  return b;
}

double paladin_t::composite_crit_avoidance()
{
  double c = player_t::composite_crit_avoidance();

  // Guarded by the Light grants -6% crit chance
  c += passives.guarded_by_the_light -> effectN( 5 ).percent();

  return c;
}

double paladin_t::composite_dodge()
{
  double d = player_t::composite_dodge();

  // add Sanctuary dodge
  d += passives.sanctuary -> effectN( 3 ).percent();

  return d;
}

// paladin_t::target_mitigation ==============================================

void paladin_t::target_mitigation( school_e school,
                                  dmg_e    dt,
                                  action_state_t* s )
{
  player_t::target_mitigation( school, dt, s );
  
  // various mitigation effects, Ardent Defender goes last due to absorb/heal mechanics
  // Guardian of Ancient Kings
  if ( buffs.gotak_prot -> up() )
    s -> result_amount *= 1.0 + dbc.spell( 86657 ) -> effectN( 2 ).percent(); // Value of the buff is stored in another spell

  // Divine Protection
  if ( buffs.divine_protection -> up() )
  {
    if ( util::school_type_component( school, SCHOOL_MAGIC ) )
    {
      s -> result_amount *= 1.0 + buffs.divine_protection -> data().effectN( 1 ).percent() * ( 1.0 + glyphs.divine_protection -> effectN( 1 ).percent() );
    }
    else
    {
      s -> result_amount *= 1.0 + buffs.divine_protection -> data().effectN( 2 ).percent() + glyphs.divine_protection -> effectN( 2 ).percent();
    }
  }

  // Glyph of Templar's Verdict
  if ( buffs.glyph_templars_verdict -> check() )
  {
    s -> result_amount *= 1.0 + buffs.glyph_templars_verdict -> value();
  }

  // Shield of the Righteous
  if ( buffs.shield_of_the_righteous -> check() )
  { s -> result_amount *= 1.0 + buffs.shield_of_the_righteous -> data().effectN( 1 ).percent() - get_divine_bulwark(); }

  s -> result_amount *= 1.0 + passives.sanctuary -> effectN( 1 ).percent();

  // Ardent Defender
  if ( buffs.ardent_defender -> check() )
  {
    s -> result_amount *= 1.0 - buffs.ardent_defender -> data().effectN( 1 ).percent();

    if ( s -> result_amount >= resources.current[ RESOURCE_HEALTH ] ) // Keep at the end of target_mitigation
    {
      s -> result_amount = 0.0;
      resource_gain( RESOURCE_HEALTH,
                     resources.max[ RESOURCE_HEALTH ] * buffs.ardent_defender -> data().effectN( 2 ).percent(),
                     nullptr,
                     s -> action );
      buffs.ardent_defender -> expire(); // assumption
    }
  }
}

// paladin_t::invalidate_cache ==============================================

void paladin_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  if ( ( passives.sword_of_light -> ok() || passives.guarded_by_the_light -> ok() )
       && ( c == CACHE_STRENGTH || c == CACHE_ATTACK_POWER )
     )
  {
    player_t::invalidate_cache( CACHE_SPELL_POWER );
  }

  if ( c == CACHE_MASTERY && passives.divine_bulwark -> ok() )
    player_t::invalidate_cache( CACHE_BLOCK );
}

// paladin_t::matching_gear_multiplier ======================================

double paladin_t::matching_gear_multiplier( attribute_e attr )
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

  if ( buffs.divine_plea -> up() )
  {
    double tick_pct = ( buffs.divine_plea -> data().effectN( 1 ).base_value() ) * 0.01;
    double tick_amount = resources.max[ RESOURCE_MANA ] * tick_pct;
    double amount = periodicity.total_seconds() * tick_amount / 3;
    resource_gain( RESOURCE_MANA, amount, gains.divine_plea );
  }
  if ( buffs.judgments_of_the_wise -> up() )
  {
    double tot_amount = resources.base[ RESOURCE_MANA ] * buffs.judgments_of_the_wise -> data().effectN( 1 ).percent();
    double amount = periodicity.total_seconds() * tot_amount / buffs.judgments_of_the_wise -> buff_duration.total_seconds();
    resource_gain( RESOURCE_MANA, amount, gains.judgments_of_the_wise );
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

  if ( s -> result == RESULT_PARRY )
  {
    if ( main_hand_attack && main_hand_attack -> execute_event )
    {
      timespan_t swing_time = main_hand_attack -> time_to_execute;
      timespan_t max_reschedule = ( main_hand_attack -> execute_event -> occurs() - 0.20 * swing_time ) - sim -> current_time;

      if ( max_reschedule > timespan_t::zero() )
      {
        main_hand_attack -> reschedule_execute( std::min( ( 0.40 * swing_time ), max_reschedule ) );
        procs.parry_haste -> occur();
      }
    }
  }
  if ( s -> result == RESULT_DODGE || s -> result == RESULT_PARRY )
  {
    trigger_grand_crusader();
  }

  player_t::assess_damage( school, dtype, s );
}

// paladin_t::create_options ================================================

void paladin_t::create_options()
{
  player_t::create_options();

  option_t paladin_options[] =
  {
    opt_null()
  };

  option_t::copy( options, paladin_options );
}

// paladin_t::create_pet ====================================================

pet_t* paladin_t::create_pet( const std::string& pet_name,
                              const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );
  if ( p ) return p;

  using namespace pets;

  if ( pet_name == "guardian_of_ancient_kings_ret" )
  {
    return new guardian_of_ancient_kings_ret_t( sim, this );
  }
  return 0;
}

// paladin_t::create_pets ===================================================

// FIXME: Not possible to check spec at this point, but in the future when all
// three versions of the guardian are implemented, it would be fugly to have to
// give them different names just for the lookup

void paladin_t::create_pets()
{
  guardian_of_ancient_kings = create_pet( "guardian_of_ancient_kings_ret" );
}

// paladin_t::combat_begin ==================================================

void paladin_t::combat_begin()
{
  player_t::combat_begin();

  resources.current[ RESOURCE_HOLY_POWER ] = 0;

  if ( passives.vengeance -> ok() )
    vengeance_start();
}

// paladin_t::holy_power_stacks =============================================

int paladin_t::holy_power_stacks()
{
  if ( buffs.divine_purpose -> check() )
  {
    return std::min( ( int ) 3, ( int ) resources.current[ RESOURCE_HOLY_POWER ] );
  }
  return ( int ) resources.current[ RESOURCE_HOLY_POWER ];
}

// paladin_t::get_divine_bulwark ============================================
double paladin_t::get_divine_bulwark()
{
  if ( ! passives.divine_bulwark -> ok() )
    return 0.0;

  // block rating, 2.25% per point of mastery
  return cache.mastery_value();
}

// paladin_t::get_hand_of_light =============================================

double paladin_t::get_hand_of_light()
{
  if ( specialization() != PALADIN_RETRIBUTION ) return 0.0;

  return cache.mastery_value();
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

  struct seal_expr_t : public paladin_expr_t
  {
    seal_e rt;
    seal_expr_t( const std::string& n, paladin_t& p, seal_e r ) :
      paladin_expr_t( n, p ), rt( r ) {}
    virtual double evaluate() { return paladin.active_seal == rt; }
  };

  std::vector<std::string> splits = util::string_split( name_str, "." );

  if ( ( splits.size() == 2 ) && ( splits[ 0 ] == "seal" ) )
  {
    seal_e s = SEAL_NONE;

    if      ( splits[ 1 ] == "truth"         ) s = SEAL_OF_TRUTH;
    else if ( splits[ 1 ] == "insight"       ) s = SEAL_OF_INSIGHT;
    else if ( splits[ 1 ] == "none"          ) s = SEAL_NONE;
    else if ( splits[ 1 ] == "righteousness" ) s = SEAL_OF_RIGHTEOUSNESS;
    else if ( splits[ 1 ] == "justice"       ) s = SEAL_OF_JUSTICE;
    return new seal_expr_t( name_str, *this, s );
  }

  return player_t::create_expression( a, name_str );
}

// PALADIN MODULE INTERFACE ================================================

struct paladin_module_t : public module_t
{
  paladin_module_t() : module_t( PALADIN ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const
  {
    return new paladin_t( sim, name, r );
  }

  virtual bool valid() const { return true; }

  virtual void init( sim_t* sim ) const
  {
    for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
    {
      player_t* p = sim -> actor_list[i];
      p -> buffs.beacon_of_light          = buff_creator_t( p, "beacon_of_light", p -> find_spell( 53563 ) );
      p -> buffs.illuminated_healing      = buff_creator_t( p, "illuminated_healing", p -> find_spell( 86273 ) );
      p -> debuffs.forbearance            = buff_creator_t( p, "forbearance", p -> find_spell( 25771 ) );
    }
  }

  virtual void combat_begin( sim_t* ) const {}

  virtual void combat_end  ( sim_t* ) const {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::paladin()
{
  static paladin_module_t m;
  return &m;
}
