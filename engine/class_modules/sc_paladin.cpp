// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
/*
	To Do:
	Add aoe holy prism, healing holy prism, aoe healing holy prism, stay of execution, light's hammer dmg, light's hammer healing
	Add optimized default action lists
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
  SEAL_OF_COMMAND,
  SEAL_MAX
};

struct paladin_td_t : public actor_pair_t
{
  dot_t* dots_word_of_glory;
  dot_t* dots_holy_radiance;
  dot_t* dots_censure;
  dot_t* dots_execution_sentence;

  buff_t* debuffs_censure;

  paladin_td_t( player_t* target, paladin_t* paladin );
};

struct paladin_t : public player_t
{
public:

  // Active
  seal_e active_seal;
  heal_t*   active_beacon_of_light;
  heal_t*   active_enlightened_judgments;
  action_t* active_hand_of_light_proc;
  absorb_t* active_illuminated_healing;
  heal_t*   active_protector_of_the_innocent;
  action_t* active_seal_of_insight_proc;
  action_t* active_seal_of_justice_proc;
  action_t* active_seal_of_righteousness_proc;
  action_t* active_seal_of_truth_dot;
  action_t* active_seal_of_truth_proc;
  action_t* active_seal_of_command_proc;
  action_t* ancient_fury_explosion;

  // Buffs
  struct buffs_t
  {
    buff_t* ancient_power;
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
    buff_t* glyph_exorcism;
    buff_t* holy_avenger;
    buff_t* infusion_of_light;
    buff_t* inquisition;
    buff_t* judgments_of_the_pure;
    buff_t* judgments_of_the_wise;
    buff_t* sacred_duty;
    buff_t* glyph_of_word_of_glory;
  } buffs;

  // Gains
  struct gains_t
  {
    gain_t* divine_plea;
    gain_t* extra_regen;
    gain_t* judgments_of_the_wise;
    gain_t* sanctuary;
    gain_t* seal_of_command_glyph;
    gain_t* seal_of_insight;
    gain_t* glyph_divine_storm;

    // Holy Power
    gain_t* hp_blessed_life;
    gain_t* hp_crusader_strike;
    gain_t* hp_divine_plea;
    gain_t* hp_divine_purpose;
    gain_t* hp_divine_storm;
    gain_t* hp_exorcism;
    gain_t* hp_grand_crusader;
    gain_t* hp_hammer_of_the_righteous;
    gain_t* hp_hammer_of_wrath;
    gain_t* hp_holy_avenger;
    gain_t* hp_holy_shock;
    gain_t* hp_judgments_of_the_bold;
    gain_t* hp_pursuit_of_justice;
    gain_t* hp_tower_of_radiance;
    gain_t* hp_judgment;
  } gains;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* avengers_shield;
    cooldown_t* exorcism;
  } cooldowns;

  // Passives
  struct passives_t
  {
    const spell_data_t* boundless_conviction;
    const spell_data_t* divine_bulwark;
    const spell_data_t* ancient_fury;
    const spell_data_t* ancient_power;
    const spell_data_t* hand_of_light;
    const spell_data_t* illuminated_healing;
    const spell_data_t* judgments_of_the_bold;
    const spell_data_t* judgments_of_the_wise;
    const spell_data_t* plate_specialization;
    const spell_data_t* guarded_by_the_light;
    const spell_data_t* sword_of_light;
    const spell_data_t* sword_of_light_value;
    const spell_data_t* vengeance;
    const spell_data_t* the_art_of_war;
    const spell_data_t* sanctity_of_battle;
    const spell_data_t* seal_of_insight;
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
  } talents;

  // Glyphs
  struct glyphs_t
  {
    const spell_data_t* blessed_life;
    const spell_data_t* divine_protection;
    const spell_data_t* divine_storm;
    const spell_data_t* double_jeopardy;
    const spell_data_t* exorcism;
    const spell_data_t* harsh_words;
    const spell_data_t* immediate_truth;
    const spell_data_t* inquisition;
    const spell_data_t* word_of_glory;
  } glyphs;
private:
  target_specific_t<paladin_td_t> target_data;
public:
  player_t* beacon_target;
  int ret_pvp_gloves;

  bool bok_up;
  bool bom_up;
  timespan_t last_extra_regen;
  timespan_t extra_regen_period;
  double extra_regen_percent;

  paladin_t( sim_t* sim, const std::string& name, race_e r = RACE_TAUREN ) :
    player_t( sim, PALADIN, name, r ),
    buffs( buffs_t() ),
    gains( gains_t() ),
    cooldowns( cooldowns_t() ),
    passives( passives_t() ),
    procs( procs_t() ),
    spells( spells_t() ),
    talents( talents_t() ),
    glyphs( glyphs_t() ),
    last_extra_regen( timespan_t::from_seconds( 0.0 ) ),
    extra_regen_period( timespan_t::from_seconds( 0.0 ) ),
    extra_regen_percent( 0.0 )
  {
    target_data.init( "target_data", this );

    active_beacon_of_light             = 0;
    active_enlightened_judgments       = 0;
    active_hand_of_light_proc          = 0;
    active_illuminated_healing         = 0;
    active_protector_of_the_innocent   = 0;
    active_seal                        = SEAL_NONE;
    active_seal_of_command_proc        = 0;
    active_seal_of_justice_proc        = 0;
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
    ret_pvp_gloves = -1;

    initial.distance = ( specialization() == PALADIN_HOLY ) ? 30 : 3;
  }

  virtual void      init_defense();
  virtual void      init_base();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_scaling();
  virtual void      init_buffs();
  virtual void      init_spells();
  virtual void      init_actions();
  virtual void      init_items();
  virtual void      reset();
  virtual expr_t*   create_expression( action_t*, const std::string& name );
  virtual double    composite_attribute_multiplier( attribute_e attr );
  virtual double    composite_attack_crit( weapon_t* = 0 );
  virtual double    composite_spell_crit();
  virtual double    composite_player_multiplier( school_e school, action_t* a = NULL );
  virtual double    composite_spell_power( school_e school );
  virtual double    composite_spell_power_multiplier();
  virtual double    composite_spell_haste();
  virtual double    composite_tank_block();
  virtual double    composite_tank_crit( school_e school );
  virtual void      create_options();
  virtual double    matching_gear_multiplier( attribute_e attr );
  virtual action_t* create_action( const std::string& name, const std::string& options_str );
  virtual int       decode_set( item_t& );
  virtual resource_e primary_resource() { return RESOURCE_MANA; }
  virtual role_e primary_role();
  virtual void      regen( timespan_t periodicity );
  virtual void      assess_damage( school_e school, dmg_e, action_state_t* s );
  virtual cooldown_t* get_cooldown( const std::string& name );
  virtual pet_t*    create_pet    ( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets   ();
  virtual void      combat_begin();

  int               holy_power_stacks();
  double            get_divine_bulwark();
  double            get_hand_of_light();
  double            jotp_haste();

  virtual paladin_td_t* get_target_data( player_t* target )
  {
    paladin_td_t*& td = target_data[ target ];
    if ( ! td ) td = new paladin_td_t( target, this );
    return td;
  }

  // Temporary
  virtual std::string set_default_talents()
  {
    switch ( specialization() )
    {
    case PALADIN_RETRIBUTION: return "221223"; break;
    default: break;
    }

    return player_t::set_default_talents();
  }

  virtual std::string set_default_glyphs()
  {
    switch ( specialization() )
    {
    case SPEC_NONE: break;
    case PALADIN_RETRIBUTION: return "templars_verdict/double_jeopardy/mass_exorcism";
    default: break;
    }

    return player_t::set_default_glyphs();
  }
};

paladin_td_t::paladin_td_t( player_t* target, paladin_t* paladin ) :
  actor_pair_t( target, paladin )
{
  dots_word_of_glory      = target -> get_dot( "word_of_glory",      paladin );
  dots_holy_radiance      = target -> get_dot( "holy_radiance",      paladin );
  dots_censure            = target -> get_dot( "censure",            paladin );
  dots_execution_sentence = target -> get_dot( "execution_sentence", paladin );

  debuffs_censure = buff_creator_t( *this, "censure", paladin -> find_spell( 31803 ) );
}

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

  virtual void init_base()
  {
    pet_t::init_base();
    melee = new melee_t( this );
  }

  virtual void dismiss()
  {
    // Only trigger the explosion if we're not sleeping
    if ( current.sleeping ) return;

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

// Template for common paladin action code. See priest_action_t.
template <class Base>
struct paladin_action_t : public Base
{
  typedef Base ab; // action base, eg. spell_t
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

};

// ==========================================================================
// Paladin Heal
// ==========================================================================

struct paladin_heal_t : public paladin_action_t<heal_t>
{
  paladin_heal_t( const std::string& n, paladin_t* p,
                  const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  {
    may_crit          = true;
    tick_may_crit     = true;

    dot_behavior      = DOT_REFRESH;
    weapon_multiplier = 0.0;
  }

  virtual void execute();

  virtual double action_multiplier()
  {
    double am = base_t::action_multiplier();

    if ( p() -> active_seal == SEAL_OF_INSIGHT )
      am *= 1.0 + data().effectN( 2 ).percent();

    return am;
  }

  virtual void impact( action_state_t* );
};

// ==========================================================================
// Paladin Attacks
// ==========================================================================

struct paladin_melee_attack_t : public paladin_action_t< melee_attack_t >
{
  bool trigger_seal;
  bool trigger_seal_of_righteousness;
  bool sanctity_of_battle;  //separated from use_spell_haste because sanctity is now on melee haste
  bool use_spell_haste; // Some attacks (censure) use spell haste. sigh.

  paladin_melee_attack_t( const std::string& n, paladin_t* p,
                          const spell_data_t* s = spell_data_t::nil(),
                          bool use2hspec = true ) :
    base_t( n, p, s ),
    trigger_seal( false ), trigger_seal_of_righteousness( false ), use_spell_haste( false )
  {
    may_crit = true;
    class_flag1 = ! use2hspec;
    special = true;
  }

  virtual double composite_haste()
  {
    return use_spell_haste ? p() -> composite_spell_haste() : base_t::composite_haste();
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

      t *= p()->composite_attack_haste();
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
      if ( ! p() -> guardian_of_ancient_kings -> current.sleeping )
      {
        p() -> buffs.ancient_power -> trigger();
      }

      if ( trigger_seal || ( trigger_seal_of_righteousness && ( p() -> active_seal == SEAL_OF_RIGHTEOUSNESS ) ) )
      {
        switch ( p() -> active_seal )
        {
        case SEAL_OF_COMMAND:
          p() -> active_seal_of_command_proc       -> execute();
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
    }
  }

  virtual double action_multiplier()
  {
    double am = base_t::action_multiplier();

    if ( p() -> buffs.divine_shield -> up() )
      am *= 1.0 + p() -> buffs.divine_shield -> data().effectN( 1 ).percent();

    if ( p() -> buffs.glyph_of_word_of_glory -> up() )
      am *= 1.0 + p() -> buffs.glyph_of_word_of_glory -> value();

    return am;
  }
};

// ==========================================================================
// Paladin Spells
// ==========================================================================

struct paladin_spell_t : public paladin_action_t<spell_t>
{
  paladin_spell_t( const std::string& n, paladin_t* p,
                   const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  {
  }

  virtual double action_multiplier()
  {
    double am = base_t::action_multiplier();

    if ( p() -> buffs.divine_shield -> up() )
      am *= 1.0 + p() -> buffs.divine_shield -> data().effectN( 1 ).percent();

    if ( p() -> buffs.glyph_of_word_of_glory -> up() )
      am *= 1.0 + p() -> buffs.glyph_of_word_of_glory -> value();

    return am;
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
        // p() -> resource_gain( RESOURCE_HOLY_POWER, 3, p() -> gains.hp_divine_purpose );
        p() -> buffs.divine_purpose -> trigger();
      }
    }
  }
};

// trigger_beacon_of_light ==================================================
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
static void trigger_beacon_of_light( paladin_heal_t* h, action_state_t* s )
{
  paladin_t* p = h -> p();

  if ( ! p -> beacon_target )
    return;

  if ( ! p -> beacon_target -> buffs.beacon_of_light -> up() )
    return;

  if ( h -> proc )
    return;

  assert( p -> active_beacon_of_light );

  p -> active_beacon_of_light -> target = p -> beacon_target;


  p -> active_beacon_of_light -> base_dd_min = s -> result_amount * p -> beacon_target -> buffs.beacon_of_light -> data().effectN( 1 ).percent();
  p -> active_beacon_of_light -> base_dd_max = s -> result_amount * p -> beacon_target -> buffs.beacon_of_light -> data().effectN( 1 ).percent();

  // Holy light heals for 100% instead of 50%
  if ( h -> data().id() == p -> spells.holy_light -> id() )
  {
    p -> active_beacon_of_light -> base_dd_min *= 2.0;
    p -> active_beacon_of_light -> base_dd_max *= 2.0;
  }

  p -> active_beacon_of_light -> execute();
};

// trigger_hand_of_light ====================================================

static void trigger_hand_of_light( paladin_melee_attack_t* a, action_state_t* s )
{
  paladin_t* p = a -> p();

  if ( p -> passives.hand_of_light -> ok() )
  {
    p -> active_hand_of_light_proc -> base_dd_max = p -> active_hand_of_light_proc-> base_dd_min = s -> result_amount;
    p -> active_hand_of_light_proc -> execute();
  }
}
struct illuminated_healing_t : public absorb_t
{
  illuminated_healing_t( paladin_t* p ) :
    absorb_t( "illuminated_healing", p, p -> find_spell( 86273 ) )
  {
    background = true;
    proc = true;
    trigger_gcd = timespan_t::zero();
  }
};
// trigger_illuminated_healing ==============================================

static void trigger_illuminated_healing( paladin_heal_t* h, action_state_t* s )
{
  if ( s -> result_amount <= 0 )
    return;

  if ( h -> proc )
    return;

  paladin_t* p = h -> p();

  if ( ! p -> passives.illuminated_healing -> ok() )
    return;

  // FIXME: Each player can have their own bubble, so this should probably be a vector as well
  assert( p -> active_illuminated_healing );

  // FIXME: This should stack when the buff is present already

  double bubble_value = p -> passives.illuminated_healing -> effectN( 2 ).base_value() / 10000.0
                        * p -> composite_mastery()
                        * s -> result_amount;

  p -> active_illuminated_healing -> base_dd_min = p -> active_illuminated_healing -> base_dd_max = bubble_value;
  p -> active_illuminated_healing -> execute();
}

// Melee Attack =============================================================

struct melee_t : public paladin_melee_attack_t
{
  melee_t( paladin_t* p )
    : paladin_melee_attack_t( "melee", p, spell_data_t::nil(), true )
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
      if ( p() -> passives.the_art_of_war -> ok() && sim -> roll( p() -> passives.the_art_of_war -> proc_chance() ) )
      {
        if ( p() -> cooldowns.exorcism -> remains() <= timespan_t::zero() )
        {
          p() -> procs.wasted_art_of_war -> occur();
        }
        p() -> procs.the_art_of_war -> occur();
        p() -> cooldowns.exorcism -> reset();
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

// Avengers Shield ==========================================================

struct avengers_shield_t : public paladin_melee_attack_t
{
  avengers_shield_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "avengers_shield", p, p -> find_class_spell( "Avenger's Shield" ) )
  {
    parse_options( NULL, options_str );

    trigger_seal = false;
    aoe          = 2;
    may_parry    = false;
    may_dodge    = false;
    may_block    = false;

    cooldown = p -> cooldowns.avengers_shield;
    cooldown -> duration = data().cooldown();

    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0.0;

    base_spell_power_multiplier  = direct_power_mod;
    base_attack_power_multiplier = data().extra_coeff();
    direct_power_mod = 1.0;
  }

  virtual void execute()
  {
    paladin_melee_attack_t::execute();

    if ( p() -> buffs.grand_crusader -> up() )
    {
      int g = 1;
      p() -> resource_gain( RESOURCE_HOLY_POWER, g, p() -> gains.hp_grand_crusader );
      if ( p() -> buffs.holy_avenger -> check() )
      {
        p() -> resource_gain( RESOURCE_HOLY_POWER, p() -> buffs.holy_avenger -> value() - g, p() -> gains.hp_holy_avenger );
      }
      p() -> buffs.grand_crusader -> expire();
    }
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

// Crusader Strike ==========================================================

struct crusader_strike_t : public paladin_melee_attack_t
{
  timespan_t save_cooldown;
  const spell_data_t* sword_of_light;
  crusader_strike_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "crusader_strike", p, p -> find_class_spell( "Crusader Strike" ), true )
  {
    parse_options( NULL, options_str );
    trigger_seal = true;
    sanctity_of_battle = p -> passives.sanctity_of_battle -> ok();

    save_cooldown = cooldown -> duration;

    base_multiplier *= 1.0 + ( ( p -> set_bonus.tier13_2pc_melee() ) ? p -> sets -> set( SET_T13_2PC_MELEE ) -> effectN( 1 ).percent() : 0.0 );
    sword_of_light = p -> find_specialization_spell( "Sword of Light" );
  }
  virtual double action_multiplier()
  {
    double am = paladin_melee_attack_t::action_multiplier();

    if ( p() -> buffs.holy_avenger -> check() )
    {
      am *= 1.0 + p() -> buffs.holy_avenger -> data().effectN( 4 ).percent();
    }

    return am;
  }
  virtual double cost()
  {
    double c = paladin_melee_attack_t::cost();
    c *= 1.0 + sword_of_light -> effectN( 5 ).percent();
    return c;
  }
  virtual void execute()
  {
    //sanctity of battle reduces the CD with haste for ret/prot
    if ( p() -> specialization() == PALADIN_RETRIBUTION || p() -> specialization() == PALADIN_PROTECTION )
    {
      cooldown -> duration = save_cooldown * p()->composite_attack_haste();
    }
    paladin_melee_attack_t::execute();
  }
  virtual void impact( action_state_t* s )
  {
    paladin_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      int g = 1;
      p() -> resource_gain( RESOURCE_HOLY_POWER, g, p() -> gains.hp_crusader_strike );
      if ( p() -> buffs.holy_avenger -> check() )
      {
        p() -> resource_gain( RESOURCE_HOLY_POWER, p() -> buffs.holy_avenger -> value() - g, p() -> gains.hp_holy_avenger );
      }

      trigger_hand_of_light( this, s );
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
      trigger_hand_of_light( this, s );
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
    may_dodge = false;
    may_parry = false;
    may_miss  = false;
    background = true;
    aoe       = -1;
    sanctity_of_battle = ( p -> specialization() == PALADIN_RETRIBUTION || p -> specialization() == PALADIN_PROTECTION )
                         && p -> passives.sanctity_of_battle -> ok();

    direct_power_mod = data().extra_coeff();
  }
  virtual double action_multiplier()
  {
    double am = paladin_melee_attack_t::action_multiplier();

    if ( p() -> buffs.holy_avenger -> check() )
    {
      am *= 1.0 + p() -> buffs.holy_avenger -> data().effectN( 4 ).percent();
    }

    return am;
  }
};

struct hammer_of_the_righteous_t : public paladin_melee_attack_t
{
  timespan_t save_cooldown;
  hammer_of_the_righteous_aoe_t *proc;

  hammer_of_the_righteous_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "hammer_of_the_righteous", p, p -> find_class_spell( "Hammer of the Righteous" ), true ), proc( 0 )
  {
    parse_options( NULL, options_str );

    sanctity_of_battle = ( p -> specialization() == PALADIN_RETRIBUTION || p -> specialization() == PALADIN_PROTECTION )
                         && p -> passives.sanctity_of_battle -> ok();
    trigger_seal_of_righteousness = true;
    proc = new hammer_of_the_righteous_aoe_t( p );
    save_cooldown = cooldown -> duration;
  }
  virtual void execute()
  {
    //sanctity of battle reduces the CD with haste for ret/prot
    if ( p() -> specialization() == PALADIN_RETRIBUTION || p() -> specialization() == PALADIN_PROTECTION )
    {
      cooldown -> duration = save_cooldown *  p()->composite_attack_haste();
    }
    paladin_melee_attack_t::execute();
  }
  virtual double action_multiplier()
  {
    double am = paladin_melee_attack_t::action_multiplier();

    if ( p() -> buffs.holy_avenger -> check() )
    {
      am *= 1.0 + p() -> buffs.holy_avenger -> data().effectN( 4 ).percent();
    }

    return am;
  }
  virtual void impact( action_state_t* s )
  {
    paladin_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      proc -> execute();

      int g = 1;
      p() -> resource_gain( RESOURCE_HOLY_POWER, g, p() -> gains.hp_hammer_of_the_righteous );
      if ( p() -> buffs.holy_avenger -> check() )
      {
        p() -> resource_gain( RESOURCE_HOLY_POWER, p() -> buffs.holy_avenger -> value() - g, p() -> gains.hp_holy_avenger );
      }

      trigger_hand_of_light( this, s );

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

    sanctity_of_battle = p -> passives.sanctity_of_battle -> ok();
    may_parry    = false;
    may_dodge    = false;
    may_block    = false;

    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0.0;

    base_spell_power_multiplier  = direct_power_mod;
    base_attack_power_multiplier = data().extra_coeff();
    direct_power_mod             = 1.0;

    if ( ( p -> specialization() == PALADIN_RETRIBUTION ) && p -> find_talent_spell( "Sanctified Wrath" ) -> ok()  )
    {
      cooldown_mult = p -> find_talent_spell( "Sanctified Wrath" ) -> effectN( 1 ).percent();
    }
  }

  virtual void impact( action_state_t* s )
  {
    paladin_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> passives.sword_of_light -> ok() )
      {
        int g = 1;
        p() -> resource_gain( RESOURCE_HOLY_POWER, g, p() -> gains.hp_hammer_of_wrath );
        if ( p() -> buffs.holy_avenger -> check() )
        {
          p() -> resource_gain( RESOURCE_HOLY_POWER, p() -> buffs.holy_avenger -> value() - g, p() -> gains.hp_holy_avenger );
        }
      }
      trigger_hand_of_light( this, s );
    }
  }
  virtual double action_multiplier()
  {
    double am = paladin_melee_attack_t::action_multiplier();

    if ( p() -> buffs.holy_avenger -> check() )
    {
      am *= 1.0 + p() -> buffs.holy_avenger -> data().effectN( 4 ).percent();
    }

    return am;
  }
  virtual void update_ready()
  {
    timespan_t save_cooldown = cooldown -> duration;

    if ( p() -> buffs.avenging_wrath -> up() )
    {
      cooldown -> duration *= cooldown_mult;
    }
    cooldown -> duration *= p() -> composite_attack_haste();
    paladin_melee_attack_t::update_ready();

    cooldown -> duration = save_cooldown;
  }

  virtual bool ready()
  {
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
    double am = 0.0;
    //am = melee_attack_t::action_multiplier();
    // not *= since we don't want to double dip, just calling base to initialize variables
    am = static_cast<paladin_t*>( player ) -> get_hand_of_light();
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
    base_costs[ current_resource() ]  = p -> resources.base[ current_resource() ] * 0.164;
  }

  virtual resource_e current_resource() { return RESOURCE_MANA; }

  virtual void execute()
  {
    if ( sim -> log ) sim -> output( "%s performs %s", player -> name(), name() );
    consume_resource();
    p() -> active_seal = seal_type;
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

// Seal of Insight ==========================================================

struct seal_of_insight_proc_t : public paladin_heal_t
{
  double proc_regen;
  double proc_chance;

  seal_of_insight_proc_t( paladin_t* p ) :
    paladin_heal_t( "seal_of_insight_proc", p, p -> find_class_spell( "Seal of Insight" ) ), proc_regen( 0.0 ), proc_chance( 0.0 )
  {
    background  = true;
    proc        = true;
    trigger_gcd = timespan_t::zero();

    direct_power_mod             = 1.0;
    base_attack_power_multiplier = 0.15;
    base_spell_power_multiplier  = 0.15;

    weapon            = &( p -> main_hand_weapon );
    weapon_multiplier = 0.0;

    proc_regen  = data().effectN( 1 ).trigger() ? data().effectN( 1 ).trigger() -> effectN( 2 ).resource( RESOURCE_MANA ) : 0.0;
    proc_chance = ppm_proc_chance( data().effectN( 1 ).base_value() );

    target = player;
  }

  virtual void execute()
  {
    if ( sim -> roll( proc_chance ) )
    {
      paladin_heal_t::execute();
      p() -> resource_gain( RESOURCE_MANA, p() -> resources.base[ RESOURCE_MANA ] * proc_regen, p() -> gains.seal_of_insight );
    }
    else
    {
      update_ready();
    }
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
  }
};

// Seal of Righteousness ====================================================

struct seal_of_righteousness_proc_t : public paladin_melee_attack_t
{
  seal_of_righteousness_proc_t( paladin_t* p ) :
    paladin_melee_attack_t( "seal_of_righteousness_proc", p, p -> find_spell( p -> find_class_spell( "Seal of Righteousness" ) -> ok() ? 101423 : 0 ) )
  {
    background  = true;
    may_crit    = true;
    proc        = true;
    trigger_gcd = timespan_t::zero();

    weapon            = &( p -> main_hand_weapon );

    // TO-DO: implement the aoe stuff.
  }
};


// Seal of Truth ============================================================

struct seal_of_truth_dot_t : public paladin_melee_attack_t
{
  seal_of_truth_dot_t( paladin_t* p ) :
    paladin_melee_attack_t( "censure", p, p -> find_spell( p -> find_class_spell( "Seal of Truth" ) -> ok() ? 31803 : 0 ), false )
  {
    background       = true;
    proc             = true;
    hasted_ticks     = true;
    use_spell_haste  = p -> passives.sanctity_of_battle -> ok();
    tick_may_crit    = true;
    may_crit         = false;
    may_dodge        = false;
    may_parry        = false;
    may_block        = false;
    may_glance       = false;
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
    paladin_melee_attack_t::init();

    snapshot_flags |= STATE_HASTE;
  }

  virtual void impact( action_state_t* s )
  {
    if ( result_is_hit( s -> result ) )
    {
      td( s -> target ) -> debuffs_censure -> trigger();
    }

    paladin_melee_attack_t::impact( s );
  }

  virtual double composite_target_multiplier( player_t* t )
  {
    double am = paladin_melee_attack_t::composite_target_multiplier( t );

    am *= td( t ) -> debuffs_censure -> stack();

    return am;
  }

  virtual void last_tick( dot_t* d )
  {
    td( d -> state -> target ) -> debuffs_censure -> expire();

    paladin_melee_attack_t::last_tick( d );
  }
};

struct seal_of_truth_proc_t : public paladin_melee_attack_t
{
  seal_of_truth_proc_t( paladin_t* p )
    : paladin_melee_attack_t( "seal_of_truth_proc", p, p -> find_class_spell( "Seal of Truth" ), true )
  {
    background  = true;
    proc        = true;
    may_block   = false;
    may_glance  = false;
    may_miss    = false;
    may_dodge   = false;
    may_parry   = false;

    weapon                 = &( p -> main_hand_weapon );

    if ( data().ok() )
    {
      const spell_data_t* s = p -> find_spell( 42463 );
      if ( s && s -> ok() )
      {
        weapon_multiplier      = s -> effectN( 1 ).percent();
      }
    }

    if ( p -> glyphs.immediate_truth -> ok() )
    {
      base_multiplier *= 1.0 + p -> glyphs.immediate_truth -> effectN( 1 ).percent();
    }
  }
};

// Seal of Command proc ====================================================

struct seal_of_command_proc_t : public paladin_melee_attack_t
{
  seal_of_command_proc_t( paladin_t* p )
    : paladin_melee_attack_t( "seal_of_command_proc", p, p -> find_class_spell( "Seal of Command" ) )
  {
    background  = true;
    proc        = true;
  }
};

// judgment ================================================================

struct judgment_t : public paladin_melee_attack_t
{
  player_t* old_target;
  double cooldown_mult;
  timespan_t save_cooldown;

  judgment_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "judgment", p, p -> find_spell( "Judgment" ), true ), old_target( 0 ), //as of 8/11/2012 judgment is benefiting from sword of light
      cooldown_mult( 1.0 )
  {
    parse_options( NULL, options_str );

    sanctity_of_battle = p -> passives.sanctity_of_battle -> ok();

    base_spell_power_multiplier  = direct_power_mod;
    base_attack_power_multiplier = data().extra_coeff();
    direct_power_mod             = 1.0;
    may_glance                   = false;
    may_block                    = false;
    may_parry                    = false;
    may_dodge                    = false;

    save_cooldown                = cooldown -> duration;

    if ( ( p -> specialization() == PALADIN_PROTECTION ) && p -> find_talent_spell( "Sanctified Wrath" ) -> ok()  )
    {
      cooldown_mult = p -> find_talent_spell( "Sanctified Wrath" ) -> effectN( 1 ).percent();
    }
  }

  virtual void reset()
  {
    paladin_melee_attack_t::reset();
    old_target = 0;
  }
  virtual void execute()
  {
    //sanctity of battle reduces the CD with haste for ret/prot
    if ( p() -> specialization() == PALADIN_RETRIBUTION || p() -> specialization() == PALADIN_PROTECTION )
    {
      cooldown -> duration = save_cooldown * p() -> composite_attack_haste();
    }

    paladin_melee_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p() -> passives.judgments_of_the_bold -> ok() )
      {
        int g = 1;
        p() -> resource_gain( RESOURCE_HOLY_POWER, 1, p() -> gains.hp_judgments_of_the_bold );
        if ( p() -> buffs.holy_avenger -> check() )
        {
          p() -> resource_gain( RESOURCE_HOLY_POWER, p() -> buffs.holy_avenger -> value() - g, p() -> gains.hp_holy_avenger );
        }
      }
      p() -> buffs.double_jeopardy -> trigger();
    }
  }

  virtual void impact( action_state_t* s )
  {
    paladin_melee_attack_t::impact( s );

    if ( ! sim -> overrides.physical_vulnerability && p() -> passives.judgments_of_the_bold -> ok() )
      s -> target -> debuffs.physical_vulnerability -> trigger();
  }
  virtual double action_multiplier()
  {
    double am = paladin_melee_attack_t::action_multiplier();

    if ( p() -> buffs.holy_avenger -> check() )
    {
      am *= 1.0 + p() -> buffs.holy_avenger -> data().effectN( 4 ).percent();
    }
    if ( target != old_target && p() -> buffs.double_jeopardy -> check() )
    {
      am *= 1.0 + p() -> buffs.double_jeopardy -> value();
      old_target = target;
    }

    return am;
  }

  virtual void update_ready()
  {
    timespan_t save_cooldown = cooldown -> duration;

    if ( p() -> buffs.avenging_wrath -> up() )
    {
      cooldown -> duration *= cooldown_mult;
    }

    paladin_melee_attack_t::update_ready();

    cooldown -> duration = save_cooldown;
  }

  virtual bool ready()
  {
    if ( p() -> active_seal == SEAL_NONE ) return false;
    return paladin_melee_attack_t::ready();
  }
};

// Shield of Righteousness ==================================================

struct shield_of_the_righteous_t : public paladin_melee_attack_t
{
  shield_of_the_righteous_t( paladin_t* p, const std::string& options_str ) :
    paladin_melee_attack_t( "shield_of_the_righteous", p, p -> find_class_spell( "Shield of the Righteous" ) )
  {
    parse_options( NULL, options_str );

    may_parry = false;
    may_dodge = false;
    may_block = false;

    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0.0;

    direct_power_mod = data().extra_coeff();
  }

  virtual void execute()
  {
    paladin_melee_attack_t::execute();
    if ( result_is_hit( execute_state -> result ) )
    {
      p() -> buffs.sacred_duty -> expire();
    }
  }

  virtual double action_multiplier()
  {
    double am = paladin_melee_attack_t::action_multiplier();

    static const double holypower_pm[] = { 0, 1.0, 3.0, 6.0 };
#ifndef NDEBUG
    assert( static_cast<unsigned>( p() -> holy_power_stacks() ) < sizeof_array( holypower_pm ) );
#endif
    am *= holypower_pm[ p() -> holy_power_stacks() ];


    return am;
  }

  virtual double composite_crit()
  {
    double cc = paladin_melee_attack_t::composite_crit();

    if ( p() -> buffs.sacred_duty -> up() )
    {
      cc += 1.0;
    }

    return cc;
  }

  virtual bool ready()
  {
    if ( p() -> main_hand_weapon.group() == WEAPON_2H )
      return false;

    return paladin_melee_attack_t::ready();
  }
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

  virtual void impact( action_state_t* s )
  {
    paladin_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      trigger_hand_of_light( this, s );
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
// Avenging Wrath ===========================================================

struct avenging_wrath_t : public paladin_spell_t
{
  avenging_wrath_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "avenging_wrath", p, p -> find_class_spell( "Avenging Wrath" ) )
  {
    parse_options( NULL, options_str );
    cooldown -> duration += p -> sets -> set( SET_T14_4PC_MELEE ) -> effectN( 1 ).time_value();
    harmful = false;
  }

  virtual void execute()
  {
    paladin_spell_t::execute();

    p() -> buffs.avenging_wrath -> trigger( 1, data().effectN( 1 ).percent() );
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
    hasted_ticks = false;
    base_spell_power_multiplier  = direct_power_mod;
    base_attack_power_multiplier = data().extra_coeff();
    direct_power_mod             = 1.0;
  }
};

struct consecration_t : public paladin_spell_t
{
  spell_t* tick_spell;

  consecration_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "consecration", p, p -> find_class_spell( "Consecration" ) ), tick_spell( 0 )
  {
    parse_options( NULL, options_str );

    hasted_ticks   = false;
    may_miss       = false;
    num_ticks      = 10;
    base_tick_time = timespan_t::from_seconds( 1.0 );
    tick_spell = new consecration_tick_t( p );
  }

  virtual void init()
  {
    paladin_spell_t::init();

    tick_spell -> stats = stats;
  }

  virtual void impact( action_state_t* s )
  {
    if ( s -> target -> debuffs.flying -> check() )
    {
      if ( sim -> debug ) sim -> output( "Ground effect %s can not hit flying target %s", name(), s -> target -> name() );
    }
    else
    {
      paladin_spell_t::impact( s );
    }

  }

  virtual void tick( dot_t* d )
  {
    if ( sim -> debug ) sim -> output( "%s ticks (%d of %d)", name(), d -> current_tick, d -> num_ticks );
    tick_spell -> execute();
    stats -> add_tick( d -> time_to_tick );
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
  double tick_multiplier[ 11 ];
  execution_sentence_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "execution_sentence", p, p -> find_talent_spell( "Execution Sentence" ) )
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


    tick_multiplier[ 0 ] = 1.0;
    for ( int i = 1; i < num_ticks; i++ )
    {
      tick_multiplier[ i ] = tick_multiplier[ i-1 ] * 1.1;
    }
    tick_multiplier[ 10 ] = tick_multiplier[ 9 ] * 5;
  }

  virtual double calculate_tick_damage( result_e r, double p, double m, player_t* t )
  {
    return paladin_spell_t::calculate_tick_damage( r, p, m, t ) * tick_multiplier[ td( t ) -> dots_execution_sentence -> current_tick ];
  }
};

// Exorcism =================================================================

struct exorcism_t : public paladin_spell_t
{
  timespan_t save_cooldown;
  exorcism_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "exorcism", p, p -> find_class_spell( "Exorcism" ) )
  {
    parse_options( NULL, options_str );

    base_attack_power_multiplier = 1.0;
    base_spell_power_multiplier  = 0.0;
    direct_power_mod             = data().extra_coeff();
    may_crit                     = true;

    save_cooldown = cooldown -> duration;
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
  virtual void execute()
  {
    //sanctity of battle reduces the CD with haste for ret/prot
    if ( p() -> specialization() == PALADIN_RETRIBUTION || p() -> specialization() == PALADIN_PROTECTION )
    {
      cooldown -> duration = save_cooldown *  p()->composite_attack_haste();
    }
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
    if ( p() -> glyphs.exorcism -> ok() )
    {
      p() -> buffs.glyph_exorcism -> trigger();
    }
  }
};

// Guardian of the Ancient Kings ============================================

struct guardian_of_ancient_kings_t : public paladin_spell_t
{
  guardian_of_ancient_kings_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "guardian_of_ancient_kings", p, p -> find_class_spell( "Guardian of Ancient Kings" ) )
  {
    parse_options( NULL, options_str );
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
        p() -> resource_gain( RESOURCE_HOLY_POWER, p() -> buffs.holy_avenger -> value() - g, p() -> gains.hp_holy_avenger );
      }
    }
  }

  virtual void update_ready()
  {
    timespan_t save_cooldown = cooldown -> duration;

    if ( p() -> buffs.avenging_wrath -> up() )
    {
      cooldown -> duration *= cooldown_mult;
    }

    paladin_spell_t::update_ready();

    cooldown -> duration = save_cooldown;
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
    direct_power_mod = 0.61;
  }
};

//Holy Prism ===============================================================

struct holy_prism_t : public paladin_spell_t
{
  holy_prism_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "holy_prism", p, p->find_spell( 114852 ) )
  {
    parse_options( NULL, options_str );
    may_crit=true;
    parse_effect_data( p -> find_spell( 114852 ) -> effectN( 1 ) );
    base_spell_power_multiplier = 1.0;
    base_attack_power_multiplier = 0.0;
    direct_power_mod= p->find_spell( 114852 ) -> effectN( 1 ).coeff();
  }
  virtual void execute()
  {
    paladin_spell_t::execute();
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
  double m;

  inquisition_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "inquisition", p, p -> find_class_spell( "Inquisition" ) ),
      base_duration( data().duration() ), m( data().effectN( 1 ).percent() )
  {
    parse_options( NULL, options_str );
    may_miss     = false;
    harmful      = false;
    hasted_ticks = false;
    num_ticks    = 0;

    if ( p -> glyphs.inquisition -> ok() )
    {
      m += p -> glyphs.inquisition -> effectN( 1 ).percent();
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
    p() -> buffs.inquisition -> trigger( 1, m, -1.0, duration );

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
  timespan_t travel_time_;
  lights_hammer_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "lights_hammer", p, p -> find_talent_spell( "Light's Hammer" ) ),
      travel_time_( timespan_t::zero() )
  {
    // 114158: Talent spell
    // 114918: Periodic 2s dummy, no duration!
    // 114919: Damage/Scale data
    // 122773: 17.5s duration, 1.5s for hammer to land = 16s aoe dot
    parse_options( NULL, options_str );
    may_miss = false;

    travel_time_ = timespan_t::from_seconds( 1.5 );

    base_tick_time = p -> find_spell( 114918 ) -> effectN( 1 ).period();
    num_ticks      = ( int ) ( ( p -> find_spell( 122773 ) -> duration() - travel_time_ ) / base_tick_time );
    hasted_ticks   = false;

    dynamic_tick_action = true;
    tick_action = new lights_hammer_tick_t( p, p -> find_spell( 114919 ) );
  }

  virtual timespan_t travel_time()
  {
    return travel_time_;
  }
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


// ==========================================================================
// Paladin Heals
// ==========================================================================

void paladin_heal_t::execute()
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
      //dp() -> resource_gain( RESOURCE_HOLY_POWER, 3, p() -> gains.hp_divine_purpose );
      p() -> buffs.divine_purpose -> trigger();
    }
  }
}

void paladin_heal_t::impact( action_state_t* s )
{
  base_t::impact( s );

  trigger_illuminated_healing( this, s );

  if ( s -> target != p() -> beacon_target )
    trigger_beacon_of_light( this, s );
}

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

    if ( p() -> buffs.infusion_of_light -> up() )
      t += p() -> buffs.infusion_of_light -> data().effectN( 1 ).time_value();

    return t;
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

  virtual timespan_t execute_time()
  {
    timespan_t t = paladin_heal_t::execute_time();

    if ( p() -> buffs.infusion_of_light -> up() )
      t += p() -> buffs.infusion_of_light -> data().effectN( 1 ).time_value();

    return t;
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

    if ( p() -> buffs.infusion_of_light -> up() )
      t += p() -> buffs.infusion_of_light -> data().effectN( 1 ).time_value();

    return t;
  }
};

// Holy Radiance ============================================================

struct holy_radiance_hot_t : public paladin_heal_t
{
  holy_radiance_hot_t( paladin_t* p, uint32_t spell_id ) :
    paladin_heal_t( "holy_radiance", p, p -> find_spell( spell_id ) )
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
    paladin_heal_t( "holy_radiance", p, p -> find_class_spell( "Holy Radiance" ) )
  {
    parse_options( NULL, options_str );

    // FIXME: This is an AoE Hot, which isn't supported currently
    aoe = data().effectN( 2 ).base_value();

    hot = new holy_radiance_hot_t( p, data().effectN( 1 ).trigger_spell_id() );
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

    if ( p() -> buffs.infusion_of_light -> up() )
      t += p() -> buffs.infusion_of_light -> data().effectN( 1 ).time_value();

    return t;
  }
};

// Holy Shock Heal Spell ====================================================

struct holy_shock_heal_t : public paladin_heal_t
{
  timespan_t cd_duration;

  holy_shock_heal_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "holy_shock_heal", p, p -> find_spell( 20473 ) ), cd_duration( timespan_t::zero() )
  {
    check_spec( PALADIN_HOLY );

    parse_options( NULL, options_str );

    // Heal info is in 25914
    parse_effect_data( ( *player -> dbc.effect( 25914 ) ) );

    cd_duration = cooldown -> duration;
  }

  virtual void execute()
  {
    if ( p() -> buffs.daybreak -> up() )
      cooldown -> duration = timespan_t::zero();

    paladin_heal_t::execute();

    int g = p() -> dbc.spell( 25914 ) -> effectN( 2 ).base_value();
    p() -> resource_gain( RESOURCE_HOLY_POWER,
                          g,
                          p() -> gains.hp_holy_shock );
    if ( p() -> buffs.holy_avenger -> check() )
    {
      p() -> resource_gain( RESOURCE_HOLY_POWER, std::max( ( int ) 0, ( int )( p() -> buffs.holy_avenger -> value() - g ) ), p() -> gains.hp_holy_avenger );
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

  virtual void execute()
  {
    // Heal is based on paladin's current max health
    base_dd_min = base_dd_max = p() -> resources.max[ RESOURCE_HEALTH ];

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

    aoe = 5;
    aoe++;
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

    if ( p() -> spells.glyph_of_word_of_glory_damage -> ok() )
    {
      p() -> buffs.glyph_of_word_of_glory -> trigger( 1, p() -> spells.glyph_of_word_of_glory_damage -> effectN( 1 ).percent() * hopo );
    }
  }

};

// ==========================================================================
// Paladin Character Definition
// ==========================================================================

// paladin_t::create_action =================================================

action_t* paladin_t::create_action( const std::string& name, const std::string& options_str )
{
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

  action_t* a = 0;
  if ( name == "seal_of_command"           ) { a = new paladin_seal_t( this, "seal_of_command",       SEAL_OF_COMMAND,       options_str );
                                               active_seal_of_command_proc       = new seal_of_command_proc_t       ( this ); return a; }
  if ( name == "seal_of_justice"           ) { a = new paladin_seal_t( this, "seal_of_justice",       SEAL_OF_JUSTICE,       options_str );
                                               active_seal_of_justice_proc       = new seal_of_justice_proc_t       ( this ); return a; }
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

// paladin_t::init_defense ==================================================

void paladin_t::init_defense()
{
  player_t::init_defense();

  initial.parry_rating_per_strength = 0.27;
}

// paladin_t::init_base =====================================================

void paladin_t::init_base()
{
  player_t::init_base();

  initial.attack_power_per_strength = 2.0;
  initial.spell_power_per_intellect = 1.0;

  base.attack_power = level * 3;

  resources.base[ RESOURCE_HOLY_POWER ] = 3 + passives.boundless_conviction -> effectN( 1 ).base_value();

  // FIXME! Level-specific!
  base.miss    = 0.060;
  base.parry   = 0.044; // 85
  base.block   = 0.030; // 85

  diminished_kfactor    = 0.009560;
  diminished_dodge_capi = 0.01523660;
  diminished_parry_capi = 0.01523660;

  switch ( specialization() )
  {
  case PALADIN_HOLY:
    base.attack_hit += 0; // TODO spirit -> hit talents.enlightened_judgments
    base.spell_hit  += 0; // TODO spirit -> hit talents.enlightened_judgments
    break;

  case PALADIN_PROTECTION:
    break;

  case PALADIN_RETRIBUTION:
    break;
  default:
    break;
  }
}

// paladin_t::reset =========================================================

void paladin_t::reset()
{
  player_t::reset();

  active_seal = SEAL_NONE;
  bok_up      = false;
  bom_up      = false;
  last_extra_regen = timespan_t::from_seconds( 0.0 );
}

// paladin_t::init_gains ====================================================

void paladin_t::init_gains()
{
  player_t::init_gains();

  gains.divine_plea                 = get_gain( "divine_plea"            );
  gains.extra_regen                 = get_gain( ( specialization() == PALADIN_RETRIBUTION ) ? "sword_of_light" : "guarded_by_the_light" );
  gains.judgments_of_the_wise       = get_gain( "judgments_of_the_wise"  );
  gains.sanctuary                   = get_gain( "sanctuary"              );
  gains.seal_of_command_glyph       = get_gain( "seal_of_command_glyph"  );
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
  scales_with[ STAT_INTELLECT   ] = tree == PALADIN_HOLY;
  scales_with[ STAT_SPIRIT      ] = tree == PALADIN_HOLY;
  scales_with[ STAT_SPELL_POWER ] = tree == PALADIN_HOLY;

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

  if ( strstr( s, "gladiators_ornamented_"  ) ) return SET_PVP_HEAL;
  if ( strstr( s, "gladiators_scaled_"      ) ) return SET_PVP_MELEE;

  return SET_NONE;
}

// paladin_t::init_buffs ====================================================

void paladin_t::init_buffs()
{
  player_t::init_buffs();

  // Glyphs
  buffs.blessed_life           = buff_creator_t( this, "glyph_blessed_life", glyphs.blessed_life )
                                 .cd( timespan_t::from_seconds( glyphs.blessed_life -> effectN( 2 ).base_value() ) );
  buffs.double_jeopardy        = buff_creator_t( this, "glyph_double_jeopardy", glyphs.double_jeopardy )
                                 .duration( find_spell( glyphs.double_jeopardy -> effectN( 1 ).trigger_spell_id() ) -> duration() )
                                 .default_value( find_spell( glyphs.double_jeopardy -> effectN( 1 ).trigger_spell_id() ) -> effectN( 1 ).percent() );
  buffs.glyph_exorcism         = buff_creator_t( this, "glyph_exorcism", glyphs.exorcism )
                                 .duration( find_spell( glyphs.exorcism -> effectN( 1 ).trigger_spell_id() ) -> duration() )
                                 .default_value( find_spell( glyphs.exorcism -> effectN( 1 ).trigger_spell_id() ) -> effectN( 1 ).percent() );

  buffs.glyph_of_word_of_glory = buff_creator_t( this, "glyph_word_of_glory", spells.glyph_of_word_of_glory_damage );

  // Talents
  buffs.divine_purpose         = buff_creator_t( this, "divine_purpose", find_talent_spell( "Divine Purpose" ) )
                                 .duration( find_spell( find_talent_spell( "Divine Purpose" ) -> effectN( 1 ).trigger_spell_id() ) -> duration() );
  buffs.holy_avenger           = buff_creator_t( this, "holy_avenger", find_talent_spell( "Holy Avenger" ) ).cd( timespan_t::zero() ); // Let the ability handle the CD

  // General
  buffs.avenging_wrath         = buff_creator_t( this, "avenging_wrath", find_class_spell( "Avenging Wrath" ) ).cd( timespan_t::zero() ); // Let the ability handle the CD
  if ( find_talent_spell( "Sanctified Wrath" ) -> ok() )
  {
    buffs.avenging_wrath -> buff_duration *= 1.0 + find_talent_spell( "Sanctified Wrath" ) -> effectN( 2 ).percent();
  }

  buffs.divine_protection      = buff_creator_t( this, "divine_protection", find_class_spell( "Divine Protection" ) ).cd( timespan_t::zero() ); // Let the ability handle the CD
  buffs.divine_shield          = buff_creator_t( this, "divine_shield", find_class_spell( "Divine Shield" ) ).cd( timespan_t::zero() ); // Let the ability handle the CD

  // Holy
  buffs.daybreak               = buff_creator_t( this, "daybreak", find_class_spell( "Daybreak" ) );
  buffs.divine_plea            = buff_creator_t( this, "divine_plea", find_class_spell( "Divine Plea" ) ).cd( timespan_t::zero() ); // Let the ability handle the CD
  buffs.infusion_of_light      = buff_creator_t( this, "infusion_of_light", find_class_spell( "Infusion of Light" ) );

  // Prot
  buffs.gotak_prot             = buff_creator_t( this, "guardian_of_the_ancient_kings", find_class_spell( "Guardian of Ancient Kings", std::string(), PALADIN_PROTECTION ) );

  // Ret
  buffs.ancient_power          = buff_creator_t( this, "ancient_power", passives.ancient_power );
  buffs.inquisition            = buff_creator_t( this, "inquisition", find_class_spell( "Inquisition" ) );
  buffs.judgments_of_the_wise  = buff_creator_t( this, "judgments_of_the_wise", find_specialization_spell( "Judgments of the Wise" ) );
}

// paladin_t::init_actions ==================================================

void paladin_t::init_actions()
{
  if ( ! ( primary_role() == ROLE_HYBRID && specialization() == PALADIN_RETRIBUTION ) )
  {
    if ( ! quiet )
      sim -> errorf( "Player %s's role or spec isn't supported yet.", name() );
    quiet = true;
    return;
  }

  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( !quiet )
      sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  active_hand_of_light_proc          = new hand_of_light_proc_t         ( this );
  ancient_fury_explosion             = new ancient_fury_t               ( this );

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
        precombat_list += "/seal_of_truth";
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

      if ( find_class_spell( "Seal of Truth" ) -> ok() )
      {
        action_list_str += "/seal_of_truth";
        if ( find_class_spell( "Seal of Insight" ) -> ok() )
          action_list_str += ",if=mana.pct>=90|seal.none";
      }
      if ( find_class_spell( "Seal of Insight" ) -> ok() )
      {
        action_list_str += "/seal_of_insight";
        if ( find_class_spell( "Seal of Truth" ) -> ok() )
          action_list_str += ",if=mana.pct<=30";
      }

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
        action_list_str += "/inquisition,if=(buff.inquisition.down|buff.inquisition.remains<=2)&(holy_power>=3";
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

        int num_items = ( int ) items.size();
        int j = 0;
        for ( int i=0; i < num_items; i++ )
        {
          if ( ( items[ i ].name_str() == "lei_shins_final_orders" ) ||
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
        }
      }

      if ( find_class_spell( "Guardian Of Ancient Kings", std::string(), PALADIN_RETRIBUTION ) -> ok() )
      {
        action_list_str += "/guardian_of_ancient_kings";

        if ( find_class_spell( "Avenging Wrath" ) -> ok() & !( find_talent_spell( "Sanctified Wrath" ) -> ok() ) )
        {
          action_list_str += ",if=cooldown.avenging_wrath.remains<10&buff.inquisition.up";
        }
        if ( find_class_spell( "Avenging Wrath" ) -> ok() & find_talent_spell( "Sanctified Wrath" ) -> ok() )
        {
          action_list_str += ",if=buff.inquisition.up&buff.avenging_wrath.up";
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
        if ( items[ i ].use.active() )
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
        action_list_str += "/exorcism";

      if ( find_class_spell( "Crusader Strike" ) -> ok() )
        action_list_str += "/crusader_strike";

      if ( find_class_spell( "Judgment" ) -> ok() )
        action_list_str += "/judgment";

      if ( find_class_spell( "Templar's Verdict" ) -> ok() )
        action_list_str += "/templars_verdict,if=holy_power>=3";

      if ( find_talent_spell( "Holy Prism" ) -> ok() )
        action_list_str += "/holy_prism";
    }
    break;
    case PALADIN_PROTECTION:
    {
#if 0
      if ( level > 75 )
      {
        if ( level > 80 )
        {
          action_list_str = "flask,type=steelskin/food,type=beer_basted_crocolisk";
          action_list_str += "/earthen_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<=60";
        }
        else
        {
          action_list_str = "flask,type=stoneblood/food,type=dragonfin_filet";
          action_list_str += "/indestructible_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<=60";
        }
        action_list_str += "/seal_of_truth";
      }
      else
      {
        action_list_str = "seal_of_truth";
      }
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
      action_list_str += "/guardian_of_ancient_kings,if=health_pct<=30,use_off_gcd=1";
      action_list_str += "/shield_of_the_righteous,if=holy_power=3&(buff.sacred_duty.up|buff.inquisition.up)";
      action_list_str += "/judgment,if=holy_power=3";
      action_list_str += "/inquisition,if=holy_power=3&(buff.inquisition.down|buff.inquisition.remains<5)";
      action_list_str += "/divine_plea,if=holy_power<2";
      action_list_str += "/avengers_shield,if=buff.grand_crusader.up&holy_power<3";
      action_list_str += "/judgment,if=buff.judgments_of_the_pure.down";
      action_list_str += "/crusader_strike,if=holy_power<3";
      action_list_str += "/hammer_of_wrath";
      action_list_str += "/avengers_shield,if=cooldown.crusader_strike.remains>=0.2";
      action_list_str += "/judgment";
      action_list_str += "/consecration,not_flying=1";
      action_list_str += "/holy_wrath";
      action_list_str += "/divine_plea,if=holy_power<1";
#endif
    }
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
  if ( passives.vengeance -> ok() )
    vengeance = true;

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
  glyphs.exorcism                 = find_glyph_spell( "Glyph of Exorcism" );
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
    active_beacon_of_light = new beacon_of_light_heal_t( this );

  if ( passives.illuminated_healing -> ok() )
    active_illuminated_healing = new illuminated_healing_t( this );

  // Tier Bonuses
  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //  C2P    C4P     M2P     M4P     T2P     T4P     H2P     H4P
    {     0,     0, 105765, 105820, 105800, 105744, 105743, 105798 }, // Tier13
    {     0,     0, 123108, 123110, 123104, 123107, 123102, 123103 }, // Tier14
    {     0,     0,      0,      0,      0,      0,      0,      0 },
  };

  sets = new set_bonus_array_t( this, set_bonuses );
}

void paladin_t::init_items()
{
  player_t::init_items();

  items.size();
  for ( size_t i = 0; i < items.size(); ++i )
  {
    item_t& item = items[ i ];
    if ( item.slot == SLOT_HANDS && ret_pvp_gloves == -1 )  // i.e. hasn't been overriden by option
    {
      ret_pvp_gloves = strstr( item.name(), "gladiators_scaled_gauntlets" ) && item.ilevel > 140;
    }
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
  if ( attr == ATTR_STRENGTH && buffs.ancient_power -> check() )
  {
    m *= 1.0 + buffs.ancient_power -> stack() * passives.ancient_power ->effectN( 1 ).percent();
  }
  return m;
}

// paladin_t::composite_attack_crit ===================================

double paladin_t::composite_attack_crit( weapon_t* w )
{
  double m = player_t::composite_attack_crit( w );

  if ( buffs.inquisition -> check() )
  {
    m += buffs.inquisition -> s_data -> effectN( 3 ).percent();
  }

  return m;
}


// paladin_t::composite_spell_crit ===================================

double paladin_t::composite_spell_crit()
{
  double m = player_t::composite_spell_crit();

  if ( buffs.inquisition -> check() )
  {
    m += buffs.inquisition -> s_data -> effectN( 3 ).percent();
  }

  return m;
}

// paladin_t::composite_player_multiplier ===================================

double paladin_t::composite_player_multiplier( school_e school, action_t* a )
{
  double m = player_t::composite_player_multiplier( school, a );

  // These affect all damage done by the paladin
  m *= 1.0 + buffs.avenging_wrath -> value();

  if ( spell_data_t::is_school( school, SCHOOL_HOLY ) )
  {
    if ( buffs.inquisition -> up() )
    {
      m *= 1.0 + buffs.inquisition -> value();
    }
  }

  if ( a && a -> type == ACTION_ATTACK && ! a -> class_flag1 && ( passives.sword_of_light -> ok() ) && ( main_hand_weapon.group() == WEAPON_2H ) )
  {
    m *= 1.0 + passives.sword_of_light_value -> effectN( 1 ).percent();
  }
  return m;
}

// paladin_t::composite_spell_power =========================================

double paladin_t::composite_spell_power( school_e school )
{
  double sp = player_t::composite_spell_power( school );
  switch ( specialization() )
  {
  case PALADIN_PROTECTION:
    break;
  case PALADIN_RETRIBUTION:
    sp = passives.sword_of_light -> effectN( 1 ).percent() * composite_attack_power() * composite_attack_power_multiplier();
    break;
  default:
    break;
  }
  return sp;
}

// paladin_t::composite_spell_power_multiplier ==============================

double paladin_t::composite_spell_power_multiplier()
{
  if ( passives.sword_of_light -> ok() ) return 1.0;

  return player_t::composite_spell_power_multiplier();
}


// paladin_t::composite_spell_haste ==============================

double paladin_t::composite_spell_haste()
{
  double m = player_t::composite_spell_haste();

  if ( active_seal == SEAL_OF_INSIGHT )
  {
    m /= ( 1.0 + passives.seal_of_insight -> effectN( 3 ).percent() );
  }

  return m;
}

// paladin_t::composite_tank_block ==========================================

double paladin_t::composite_tank_block()
{
  double b = player_t::composite_tank_block();
  b += get_divine_bulwark();
  return b;
}

// paladin_t::composite_tank_crit ===========================================

double paladin_t::composite_tank_crit( school_e school )
{
  double c = player_t::composite_tank_crit( school );

  return c;
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

  if ( buffs.gotak_prot -> up() )
    s -> result_amount *= 1.0 + dbc.spell( 86657 ) -> effectN( 2 ).percent(); // Value of the buff is stored in another spell

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

  if ( buffs.glyph_exorcism -> check() )
  {
    s -> result_amount *= 1.0 + buffs.glyph_exorcism -> value();
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

  player_t::assess_damage( school, dtype, s );
}

// paladin_t::get_cooldown ==================================================

cooldown_t* paladin_t::get_cooldown( const std::string& name )
{
  if ( name == "hammer_of_the_righteous" ) return player_t::get_cooldown( "crusader_strike" );

  return player_t::get_cooldown( name );
}

// paladin_t::create_options ================================================

void paladin_t::create_options()
{
  player_t::create_options();

  option_t paladin_options[] =
  {
    { "pvp_gloves", OPT_BOOL,    &( ret_pvp_gloves ) },
    { NULL, OPT_UNKNOWN, NULL }
  };

  option_t::copy( options, paladin_options );
}


// paladin_t::create_pet ====================================================

pet_t* paladin_t::create_pet( const std::string& pet_name,
                              const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );
  if ( p ) return p;

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
  if ( specialization() != PALADIN_PROTECTION ) return 0.0;

  // block rating, 2.25% per point of mastery
  return composite_mastery() * ( passives.divine_bulwark -> effectN( 1 ).coeff() / 100.0 );
}

// paladin_t::get_hand_of_light =============================================

double paladin_t::get_hand_of_light()
{
  if ( specialization() != PALADIN_RETRIBUTION ) return 0.0;

  return composite_mastery() * ( passives.hand_of_light -> effectN( 1 ).coeff() / 100.0 );
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

  std::vector<std::string> splits;
  int num_splits = util::string_split( splits, name_str, "." );

  if ( ( num_splits == 2 ) && ( splits[ 0 ] == "seal" ) )
  {
    seal_e s = SEAL_NONE;

    if      ( splits[ 1 ] == "truth"         ) s = SEAL_OF_TRUTH;
    else if ( splits[ 1 ] == "insight"       ) s = SEAL_OF_INSIGHT;
    else if ( splits[ 1 ] == "none"          ) s = SEAL_NONE;
    else if ( splits[ 1 ] == "righteousness" ) s = SEAL_OF_RIGHTEOUSNESS;
    else if ( splits[ 1 ] == "justice"       ) s = SEAL_OF_JUSTICE;
    else if ( splits[ 1 ] == "command"       ) s = SEAL_OF_COMMAND;
    return new seal_expr_t( name_str, *this, s );
  }

  return player_t::create_expression( a, name_str );
}

// PALADIN MODULE INTERFACE ================================================

struct paladin_module_t : public module_t
{
  paladin_module_t() : module_t( PALADIN ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE )
  {
    return new paladin_t( sim, name, r );
  }
  virtual bool valid() { return true; }
  virtual void init( sim_t* sim )
  {
    for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
    {
      player_t* p = sim -> actor_list[i];
      p -> buffs.beacon_of_light          = buff_creator_t( p, "beacon_of_light", p -> find_spell( 53563 ) );
      p -> buffs.illuminated_healing      = buff_creator_t( p, "illuminated_healing", p -> find_spell( 86273 ) );
      p -> debuffs.forbearance            = buff_creator_t( p, "forbearance", p -> find_spell( 25771 ) );
    }
  }
  virtual void combat_begin( sim_t* ) {}
  virtual void combat_end  ( sim_t* ) {}
};

} // UNNAMED NAMESPACE

module_t* module_t::paladin()
{
  static module_t* m = 0;
  if ( ! m ) m = new paladin_module_t();
  return m;
}
