// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Paladin
// ==========================================================================

#if SC_PALADIN == 1

enum seal_type_e
{
  SEAL_NONE=0,
  SEAL_OF_JUSTICE,
  SEAL_OF_INSIGHT,
  SEAL_OF_RIGHTEOUSNESS,
  SEAL_OF_TRUTH,
  SEAL_MAX
};

struct paladin_targetdata_t : public targetdata_t
{
  dot_t* dots_exorcism;
  dot_t* dots_word_of_glory;
  dot_t* dots_holy_radiance;
  dot_t* dots_censure;

  buff_t* debuffs_censure;

  paladin_targetdata_t( paladin_t* source, player_t* target );
};

void register_paladin_targetdata( sim_t* sim )
{
  player_type_e t = PALADIN;
  typedef paladin_targetdata_t type;

  REGISTER_DOT( censure );
  REGISTER_DOT( exorcism );
  REGISTER_DOT( word_of_glory );
  REGISTER_DOT( holy_radiance );

  REGISTER_DEBUFF( censure );
}

struct paladin_t : public player_t
{
  // Active
  seal_type_e active_seal;
  heal_t*   active_beacon_of_light;
  heal_t*   active_enlightened_judgements;
  action_t* active_hand_of_light_proc;
  absorb_t* active_illuminated_healing;
  heal_t*   active_protector_of_the_innocent;
  action_t* active_seal_of_insight_proc;
  action_t* active_seal_of_justice_proc;
  action_t* active_seal_of_righteousness_proc;
  action_t* active_seal_of_truth_dot;
  action_t* active_seal_of_truth_proc;
  action_t* active_seals_of_command_proc;
  action_t* ancient_fury_explosion;

  // Buffs
  struct buffs_t
  {
    buff_t* ancient_power;
    buff_t* avenging_wrath;
    buff_t* daybreak;
    buff_t* divine_plea;
    buff_t* divine_protection;
    buff_t* divine_shield;
    buff_t* gotak_prot;
    buff_t* grand_crusader;
    buff_t* holy_shield;
    buff_t* infusion_of_light;
    buff_t* inquisition;
    buff_t* judgements_of_the_bold;
    buff_t* judgements_of_the_pure;
    buff_t* judgements_of_the_wise;
    buff_t* sacred_duty;
    buff_t* zealotry;
  } buffs;

  // Gains
  struct gains_t
  {
    gain_t* divine_plea;
    gain_t* judgements_of_the_bold;
    gain_t* judgements_of_the_wise;
    gain_t* sanctuary;
    gain_t* seal_of_command_glyph;
    gain_t* seal_of_insight;

    // Holy Power
    gain_t* hp_blessed_life;
    gain_t* hp_crusader_strike;
    gain_t* hp_divine_plea;
    gain_t* hp_divine_storm;
    gain_t* hp_grand_crusader;
    gain_t* hp_hammer_of_the_righteous;
    gain_t* hp_holy_shock;
    gain_t* hp_pursuit_of_justice;
    gain_t* hp_tower_of_radiance;
    gain_t* hp_judgement;
  } gains;

  // Cooldowns
  // Cooldowns
  struct cooldowns_t 
  {
    cooldown_t* avengers_shield;
    cooldown_t* exorcism;
  } cooldowns;

  // Passives
  struct passives_t
  {
    const spell_data_t* tier13_4pc_melee_value;
    const spell_data_t* boundless_conviction;
    const spell_data_t* crusaders_zeal;
    const spell_data_t* divine_bulwark;
    const spell_data_t* hand_of_light;
    const spell_data_t* illuminated_healing;
    const spell_data_t* judgements_of_the_bold; // passive stuff is hidden here because spells
    const spell_data_t* judgements_of_the_wise; // can only have three effects
    const spell_data_t* meditation;
    const spell_data_t* plate_specialization;
    const spell_data_t* sword_of_light;
    const spell_data_t* sword_of_light_value;
    const spell_data_t* vengeance;
    const spell_data_t* the_art_of_war;
    const spell_data_t* sanctity_of_battle;
    passives_t() { memset( ( void* ) this, 0x0, sizeof( passives_t ) ); }
  };
  passives_t passives;

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
    spells_t() { memset( ( void* ) this, 0x0, sizeof( spells_t ) ); }
  } spells;

  // Talents
  struct talents_t
  {
    // holy

    // prot

    // ret

    talents_t() { memset( ( void* ) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  // Glyphs
  struct glyphs_t
  {
    // prime

    // major

    glyphs_t() { memset( ( void* ) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  player_t* beacon_target;
  int ret_pvp_gloves;

  paladin_t( sim_t* sim, const std::string& name, race_type_e r = RACE_TAUREN ) :
    player_t( sim, PALADIN, name, r )
  {
    active_beacon_of_light             = 0;
    active_enlightened_judgements      = 0;
    active_hand_of_light_proc          = 0;
    active_illuminated_healing         = 0;
    active_protector_of_the_innocent   = 0;
    active_seal = SEAL_NONE;
    active_seals_of_command_proc       = 0;
    active_seal_of_justice_proc        = 0;
    active_seal_of_insight_proc        = 0;
    active_seal_of_righteousness_proc  = 0;
    active_seal_of_truth_proc          = 0;
    active_seal_of_truth_dot           = 0;
    ancient_fury_explosion             = 0;

    cooldowns.avengers_shield = get_cooldown( "avengers_shield" );
    cooldowns.exorcism = get_cooldown( "exorcism" );

    beacon_target = 0;
    ret_pvp_gloves = -1;

    distance = ( primary_tree() == PALADIN_HOLY ) ? 30 : 3;
    default_distance = distance;

    create_options();
  }

  virtual paladin_targetdata_t* new_targetdata( player_t* target )
  { return new paladin_targetdata_t( this, target ); }
  virtual void      init_defense();
  virtual void      init_base();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_scaling();
  virtual void      init_buffs();
  virtual void      init_spells();
  virtual void      init_values();
  virtual void      init_actions();
  virtual void      init_items();
  virtual void      reset();
  virtual double    composite_attribute_multiplier( attribute_type_e attr ) const;
  virtual double    composite_attack_speed() const;
  virtual double    composite_player_multiplier( school_type_e school, const action_t* a = NULL ) const;
  virtual double    composite_spell_power( school_type_e school ) const;
  virtual double    composite_tank_block() const;
  virtual double    composite_tank_block_reduction() const;
  virtual double    composite_tank_crit( school_type_e school ) const;
  virtual void      create_options();
  virtual double    matching_gear_multiplier( attribute_type_e attr ) const;
  virtual action_t* create_action( const std::string& name, const std::string& options_str );
  virtual int       decode_set( const item_t& ) const;
  virtual resource_type_e primary_resource() const { return RESOURCE_MANA; }
  virtual role_type_e primary_role() const;
  virtual void      regen( timespan_t periodicity );
  virtual double    assess_damage( double amount, school_type_e school, dmg_type_e, result_type_e, action_t* a );
  virtual heal_info_t assess_heal( double amount, school_type_e school, dmg_type_e, result_type_e, action_t* a );
  virtual cooldown_t* get_cooldown( const std::string& name );
  virtual pet_t*    create_pet    ( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets   ();
  virtual void      combat_begin();

  int               holy_power_stacks() const;
  double            get_divine_bulwark() const;
  double            get_hand_of_light() const;
  double            jotp_haste() const;
};

paladin_targetdata_t::paladin_targetdata_t( paladin_t* source, player_t* target ) :
  targetdata_t( source, target )
{
  debuffs_censure = add_aura( buff_creator_t( this, "censure", source -> find_spell( 31803 ) ) );
}

namespace { // ANONYMOUS NAMESPACE ==========================================

// Guardian of Ancient Kings Pet ============================================

// TODO: melee attack
struct guardian_of_ancient_kings_ret_t : public pet_t
{
  melee_attack_t* melee;

  struct melee_t : public melee_attack_t
  {
    paladin_t* owner;

    melee_t( player_t *p )
      : melee_attack_t( "melee", p, spell_data_t::nil(), SCHOOL_PHYSICAL ), owner( 0 )
    {
      weapon = &( p -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      weapon_multiplier = 1.0;
      background = true;
      repeating  = true;
      trigger_gcd = timespan_t::zero();

      owner = p -> cast_pet() -> owner -> cast_paladin();
    }

    virtual void execute()
    {
      melee_attack_t::execute();
      if ( result_is_hit() )
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
    main_hand_weapon.min_dmg = 5500; // TODO
    main_hand_weapon.max_dmg = 7000; // TODO
  }

  virtual void init_base()
  {
    pet_t::init_base();
    melee = new melee_t( this );
  }

  virtual void dismiss()
  {
    // Only trigger the explosion if we're not sleeping
    if ( sleeping ) return;

    pet_t::dismiss();

    if ( owner -> cast_paladin() -> ancient_fury_explosion )
      owner -> cast_paladin() -> ancient_fury_explosion -> execute();
  }

  virtual void schedule_ready( timespan_t delta_time=timespan_t::zero(), bool waiting=false )
  {
    pet_t::schedule_ready( delta_time, waiting );
    if ( ! melee -> execute_event ) melee -> execute();
  }
};

// ==========================================================================
// Paladin Heal
// ==========================================================================

struct paladin_heal_t : public heal_t
{
  paladin_heal_t( const std::string& n, paladin_t* p,
                  const spell_data_t* s = spell_data_t::nil(), school_type_e sc = SCHOOL_NONE ) :
    heal_t( n, p, s, sc )
  {
    may_crit          = true;
    tick_may_crit     = true;

    dot_behavior      = DOT_REFRESH;
    weapon_multiplier = 0.0;
  }

  paladin_t* p() const
  { return static_cast<paladin_t*>( player ); }

  virtual double cost() const
  {
    if ( current_resource() == RESOURCE_HOLY_POWER )
    {
      return std::max( base_costs[ RESOURCE_HOLY_POWER ], p() -> resources.current[ RESOURCE_HOLY_POWER ] );
    }

    return heal_t::cost();
  }

  virtual void execute();
};

// ==========================================================================
// Paladin Attacks
// ==========================================================================

struct paladin_melee_attack_t : public melee_attack_t
{
  bool trigger_seal;
  bool trigger_seal_of_righteousness;
  bool use_spell_haste; // Some attacks (CS w/ sanctity of battle, censure) use spell haste. sigh.

  paladin_melee_attack_t( const std::string& n, paladin_t* p,
                          const spell_data_t* s = spell_data_t::nil(), 
                          school_type_e sc = SCHOOL_NONE,
                          bool use2hspec = true ) :
    melee_attack_t( n, p, s, sc ),
    trigger_seal( false ), trigger_seal_of_righteousness( false ), use_spell_haste( false )
  {
    may_crit = true;
    class_flag1 = ! use2hspec;
    special = true;
  }

  paladin_t* p() const
  { return static_cast<paladin_t*>( player ); }

  virtual double haste() const
  {
    return use_spell_haste ? p() -> composite_spell_haste() : melee_attack_t::haste();
  }

  virtual double total_haste() const
  {
    return use_spell_haste ? p() -> composite_spell_haste() : melee_attack_t::total_haste();
  }

  virtual void execute()
  {
    melee_attack_t::execute();
    if ( result_is_hit() )
    {
      paladin_targetdata_t* td = targetdata() -> cast_paladin();
      if ( trigger_seal || ( trigger_seal_of_righteousness && ( p() -> active_seal == SEAL_OF_RIGHTEOUSNESS ) ) )
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
          if ( td -> debuffs_censure -> stack() >= 1 ) p() -> active_seal_of_truth_proc -> execute();
          break;
        default:
          ;
        }

        // TODO: does the censure stacking up happen before or after the SoT proc?
        if ( p() -> active_seal == SEAL_OF_TRUTH )
        {
          p() -> active_seal_of_truth_dot -> execute();
        }

        // It seems like it's the seal-triggering attacks that stack up ancient power
        if ( ! p() -> guardian_of_ancient_kings -> sleeping )
        {
          p() -> buffs.ancient_power -> trigger();
        }
      }
    }
  }

  virtual void player_buff()
  {
    melee_attack_t::player_buff();

    if ( p() -> buffs.divine_shield -> up() )
    {
      player_multiplier *= 1.0 + p() -> buffs.divine_shield -> data().effect1().percent();
    }
  }

  virtual double cost() const
  {
    if ( current_resource() == RESOURCE_HOLY_POWER )
    {
      return std::max( base_costs[ RESOURCE_HOLY_POWER ], p() -> resources.current[ RESOURCE_HOLY_POWER ] );
    }

    return melee_attack_t::cost();
  }
};

// ==========================================================================
// Paladin Spells
// ==========================================================================

struct paladin_spell_t : public spell_t
{
  paladin_spell_t( const std::string& n, paladin_t* p,
                   const spell_data_t* s = spell_data_t::nil(), 
                   school_type_e sc = SCHOOL_NONE )
    : spell_t( n, p, s, sc )
  {
  }

  paladin_t* p() const
  { return static_cast<paladin_t*>( player ); }

  virtual double cost() const
  {
    if ( current_resource() == RESOURCE_HOLY_POWER )
    {
      return std::max( base_costs[ RESOURCE_HOLY_POWER ], p() -> resources.current[ RESOURCE_HOLY_POWER ] );
    }

    return spell_t::cost();
  }

  virtual void player_buff()
  {
    spell_t::player_buff();

    if ( p() -> buffs.divine_shield -> up() )
    {
      player_multiplier *= 1.0 + p() -> buffs.divine_shield -> data().effect1().percent();
    }
  }
};

// trigger_beacon_of_light ==================================================

static void trigger_beacon_of_light( heal_t* h )
{
  paladin_t* p = h -> player -> cast_paladin();

  if ( ! p -> beacon_target )
    return;

  if ( ! p -> beacon_target -> buffs.beacon_of_light -> up() )
    return;

  if ( h -> proc )
    return;

  if ( ! p -> active_beacon_of_light )
  {
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

        init();
      }
    };
    p -> active_beacon_of_light = new beacon_of_light_heal_t( p );
  }

  p -> active_beacon_of_light -> base_dd_min = h -> direct_dmg * p -> beacon_target -> buffs.beacon_of_light -> data().effect1().percent();
  p -> active_beacon_of_light -> base_dd_max = h -> direct_dmg * p -> beacon_target -> buffs.beacon_of_light -> data().effect1().percent();

  // Holy light heals for 100% instead of 50%
  if ( h -> data().id() == p -> spells.holy_light -> id() )
  {
    p -> active_beacon_of_light -> base_dd_min *= 2.0;
    p -> active_beacon_of_light -> base_dd_max *= 2.0;
  }

  p -> active_beacon_of_light -> execute();
};

// trigger_hand_of_light ====================================================

static void trigger_hand_of_light( action_t* a )
{
  paladin_t* p = a -> player -> cast_paladin();

  if ( p -> primary_tree() == PALADIN_RETRIBUTION )
  {
    p -> active_hand_of_light_proc -> base_dd_max = p -> active_hand_of_light_proc-> base_dd_min = a -> direct_dmg;
    p -> active_hand_of_light_proc -> execute();
  }
}

// trigger_illuminated_healing ==============================================

static void trigger_illuminated_healing( heal_t* h )
{
  if ( h -> direct_dmg <= 0 )
    return;

  if ( h -> proc )
    return;

  paladin_t* p = h -> player -> cast_paladin();

  // FIXME: Each player can have their own bubble, so this should probably be a vector as well
  if ( ! p -> active_illuminated_healing )
  {
    struct illuminated_healing_t : public absorb_t
    {
      illuminated_healing_t( paladin_t* p ) :
        absorb_t( "illuminated_healing", p, p -> find_spell( 86273 ) )
      {
        background = true;
        proc = true;
        trigger_gcd = timespan_t::zero();

        init();
      }
    };
    p -> active_illuminated_healing = new illuminated_healing_t( p );
  }

  // FIXME: This should stack when the buff is present already

  double bubble_value = p -> passives.illuminated_healing -> effect2().base_value() / 10000.0
                        * p -> composite_mastery()
                        * h -> direct_dmg;

  p -> active_illuminated_healing -> base_dd_min = p -> active_illuminated_healing -> base_dd_max = bubble_value;
  p -> active_illuminated_healing -> execute();
}

// Melee Attack =============================================================

struct melee_t : public paladin_melee_attack_t
{
  melee_t( paladin_t* p )
    : paladin_melee_attack_t( "melee", p, spell_data_t::nil(), SCHOOL_PHYSICAL, true )
  {
    special           = false;
    trigger_seal      = true;
    background        = true;
    repeating         = true;
    trigger_gcd       = timespan_t::zero();
    weapon            = &( p -> main_hand_weapon );
    base_execute_time = p -> main_hand_weapon.swing_time;
  }

  virtual timespan_t execute_time() const
  {
    if ( ! player -> in_combat ) return timespan_t::from_seconds( 0.01 );
    return paladin_melee_attack_t::execute_time();
  }

  virtual void execute()
  {
    paladin_melee_attack_t::execute();
    if ( result_is_hit() )
    {
      if ( p() -> passives.crusaders_zeal -> ok() )
      {
        p() -> buffs.zealotry -> decrement();
        p() -> buffs.zealotry -> trigger( 3 );
      }
      if ( p() -> passives.the_art_of_war -> ok() )
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
    : paladin_melee_attack_t( "auto_attack", p, spell_data_t::nil(), SCHOOL_PHYSICAL, true )
  {
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
    paladin_spell_t( "ancient_fury", p, p -> find_spell( 86704 ) )
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

  virtual void player_buff()
  {
    paladin_spell_t::player_buff();
    player_multiplier *= p() -> buffs.ancient_power -> stack();
  }

  virtual double total_crit() const
  {
    // This doesnt inherit the player's crit
    return base_crit + target_crit;
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
      p() -> resource_gain( RESOURCE_HOLY_POWER, 1, p() -> gains.hp_grand_crusader );
      p() -> buffs.grand_crusader -> expire();
    }
  }
};

// Crusader Strike ==========================================================

struct crusader_strike_t : public paladin_melee_attack_t
{
  timespan_t base_cooldown;
  crusader_strike_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "crusader_strike", p, p -> find_class_spell( "Crusader Strike" ) ), base_cooldown( timespan_t::zero() )
  {
    parse_options( NULL, options_str );

    trigger_seal = true;
    use_spell_haste = p -> passives.sanctity_of_battle -> ok();
    // JotW decreases the CD by 1.5 seconds for Prot Pallies, but it's not in the tooltip
    cooldown -> duration += p -> passives.judgements_of_the_wise -> effectN( 1 ).time_value();
    base_cooldown         = cooldown -> duration;

    base_multiplier *= 1.0 + 0.05 * p -> ret_pvp_gloves;
    base_multiplier *= 1.0 + ( ( p -> set_bonus.tier13_2pc_melee() ) ? p -> sets -> set( SET_T13_2PC_MELEE ) -> effectN( 1 ).percent() : 0.0 );
  }

  virtual void execute()
  {
    paladin_melee_attack_t::execute();

    if ( result_is_hit() )
    {
      p() -> resource_gain( RESOURCE_HOLY_POWER, 1, p() -> gains.hp_crusader_strike );

      trigger_hand_of_light( this );
    }
  }
};

// Divine Storm =============================================================

struct divine_storm_t : public paladin_melee_attack_t
{
  timespan_t base_cooldown;

  divine_storm_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "divine_storm", p, p -> find_class_spell( "Divine Storm" ) ), base_cooldown( timespan_t::zero() )
  {
    parse_options( NULL, options_str );

    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0.0;

    aoe               = -1;
    trigger_seal      = false;
    trigger_seal_of_righteousness = true;
    base_cooldown     = cooldown -> duration;
  }

  virtual void execute()
  {
    paladin_melee_attack_t::execute();
    if ( result_is_hit() )
    {
      trigger_hand_of_light( this );
      /*
        if ( target -> cast_target() -> adds_nearby >= 4 )
          {
            p() -> resource_gain( RESOURCE_HOLY_POWER, 1,
                                      p() -> gains_hp_divine_storm );
          }*/
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

// Hammer of the Righteous ==================================================

struct hammer_of_the_righteous_aoe_t : public paladin_melee_attack_t
{
  hammer_of_the_righteous_aoe_t( paladin_t* p )
    : paladin_melee_attack_t( "hammer_of_the_righteous_aoe", p, p -> find_spell( 88263 ), SCHOOL_NONE, false )
  {
    may_dodge = false;
    may_parry = false;
    may_miss  = false;
    background = true;
    aoe       = -1;
    use_spell_haste = p -> passives.sanctity_of_battle -> ok();

    direct_power_mod = data().extra_coeff();
  }
};

struct hammer_of_the_righteous_t : public paladin_melee_attack_t
{
  hammer_of_the_righteous_aoe_t *proc;

  hammer_of_the_righteous_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "hammer_of_the_righteous", p, p -> find_class_spell( "Hammer of the Righteous" ), SCHOOL_NONE, false ), proc( 0 )
  {
    parse_options( NULL, options_str );

    use_spell_haste = p -> passives.sanctity_of_battle -> ok();
    trigger_seal_of_righteousness = true;
    proc = new hammer_of_the_righteous_aoe_t( p );
  }

  virtual void execute()
  {
    paladin_melee_attack_t::execute();
    if ( result_is_hit() )
    {
      proc -> execute();

      trigger_hand_of_light( this );

      // Mists of Pandaria: Hammer of the Righteous triggers Weakened Blows
      if ( ! sim -> overrides.weakened_blows )
        target -> debuffs.weakened_blows -> trigger();
    }
  }
};

// Hammer of Wrath ==========================================================

struct hammer_of_wrath_t : public paladin_melee_attack_t
{
  hammer_of_wrath_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "hammer_of_wrath", p, p -> find_class_spell( "Hammer of Wrath" ), SCHOOL_NONE, false )
  {
    parse_options( NULL, options_str );

    use_spell_haste = p -> passives.sanctity_of_battle -> ok();
    may_parry    = false;
    may_dodge    = false;
    may_block    = false;
    trigger_seal = 1; // TODO: only SoT or all seals?

    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0.0;

    base_spell_power_multiplier  = direct_power_mod;
    base_attack_power_multiplier = data().extra_coeff();
    direct_power_mod             = 1.0;
  }

  virtual bool ready()
  {
    if ( target -> health_percentage() > 20 && ! ( p() -> buffs.avenging_wrath -> check() ) )
      return false;

    return paladin_melee_attack_t::ready();
  }
};

// Hand of Light proc =======================================================

struct hand_of_light_proc_t : public melee_attack_t
{
  hand_of_light_proc_t( paladin_t* p )
    : melee_attack_t( "hand_of_light", p, spell_data_t::nil(), SCHOOL_HOLY )
  {
    may_crit    = false;
    may_miss    = false;
    may_dodge   = false;
    may_parry   = false;
    proc        = true;
    background  = true;
    trigger_gcd = timespan_t::zero();
  }

  virtual void player_buff()
  {
    melee_attack_t::player_buff();
    // not *= since we don't want to double dip, just calling base to initialize variables
    player_multiplier = static_cast<paladin_t*>( player ) -> get_hand_of_light();
    player_multiplier *= 1.0 + static_cast<paladin_t*>( player ) -> buffs.inquisition -> value();
  }

  virtual void target_debuff( player_t* t, dmg_type_e dt )
  {
    melee_attack_t::target_debuff( t, dt );
    // not *= since we don't want to double dip in other effects (like vunerability)
    target_multiplier = 1.0 + t -> debuffs.magic_vulnerability -> value();
  }
};

// Paladin Seals ============================================================

struct paladin_seal_t : public paladin_melee_attack_t
{
  seal_type_e seal_type;

  paladin_seal_t( paladin_t* p, const std::string& n, seal_type_e st, const std::string& options_str )
    : paladin_melee_attack_t( n, p ), seal_type( st )
  {
    parse_options( NULL, options_str );

    harmful    = false;
    base_costs[ current_resource() ]  = p -> resources.base[ current_resource() ] * 0.164;
  }

  virtual resource_type_e current_resource() const { return RESOURCE_MANA; }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
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
  seal_of_insight_proc_t( paladin_t* p ) :
    paladin_heal_t( "seal_of_insight_proc", p, p -> find_spell( 20167 ) )
  {
    background  = true;
    proc        = true;
    trigger_gcd = timespan_t::zero();

    target = player;
  }

  virtual void execute()
  {
    base_dd_min = base_dd_max = ( p() -> spell_power[ SCHOOL_HOLY ] * 0.15 ) + ( 0.15 * p() -> composite_attack_power() );
    paladin_heal_t::execute();
    p() -> resource_gain( RESOURCE_MANA, p() -> resources.base[ RESOURCE_MANA ] * data().effect2().resource( RESOURCE_MANA ), p() -> gains.seal_of_insight );
  }
};

struct seal_of_insight_judgement_t : public paladin_melee_attack_t
{
  seal_of_insight_judgement_t( paladin_t* p ) :
    paladin_melee_attack_t( "judgement_of_insight", p, spell_data_t::nil(), SCHOOL_HOLY )
  {
    background = true;

    may_parry = false;
    may_dodge = false;
    may_block = false;

    base_dd_min = base_dd_max    = 1.0;
    direct_power_mod             = 1.0;
    base_spell_power_multiplier  = 0.25;
    base_attack_power_multiplier = 0.16;

    weapon            = &( p -> main_hand_weapon );
    weapon_multiplier = 0.0;

    cooldown -> duration = timespan_t::from_seconds( 8 );
  }
};

// Seal of Justice ==========================================================

struct seal_of_justice_proc_t : public paladin_melee_attack_t
{
  seal_of_justice_proc_t( paladin_t* p ) :
    paladin_melee_attack_t( "seal_of_justice", p, spell_data_t::nil(), SCHOOL_HOLY )
  {
    background        = true;
    proc              = true;
    trigger_gcd       = timespan_t::zero();
    weapon            = &( p -> main_hand_weapon );
    weapon_multiplier = 0.0;
  }

  virtual void execute()
  {
    // No need to model stun
  }
};

struct seal_of_justice_judgement_t : public paladin_melee_attack_t
{
  seal_of_justice_judgement_t( paladin_t* p ) :
    paladin_melee_attack_t( "judgement_of_justice", p, spell_data_t::nil(), SCHOOL_HOLY )
  {
    background = true;
    may_parry  = false;
    may_dodge  = false;
    may_block  = false;

    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0.0;

    direct_power_mod             = 1.0;
    base_spell_power_multiplier  = 0.25;
    base_attack_power_multiplier = 0.16;

    weapon            = &( p -> main_hand_weapon );
    weapon_multiplier = 0.0;

    cooldown -> duration = timespan_t::from_seconds( 8 );
  }
};

// Seal of Righteousness ====================================================

struct seal_of_righteousness_proc_t : public paladin_melee_attack_t
{
  seal_of_righteousness_proc_t( paladin_t* p ) :
    paladin_melee_attack_t( "seal_of_righteousness", p, spell_data_t::nil(), SCHOOL_HOLY )
  {
    background  = true;
    may_crit    = true;
    proc        = true;
    trigger_gcd = timespan_t::zero();

    direct_power_mod             = 1.0;
    base_attack_power_multiplier = 0.011;
    base_spell_power_multiplier  = 2 * base_attack_power_multiplier;

    weapon            = &( p -> main_hand_weapon );
    weapon_multiplier = 0.0;
  }
};

struct seal_of_righteousness_judgement_t : public paladin_melee_attack_t
{
  seal_of_righteousness_judgement_t( paladin_t* p ) :
    paladin_melee_attack_t( "judgement_of_righteousness", p, spell_data_t::nil(), SCHOOL_HOLY )
  {
    background = true;
    may_parry  = false;
    may_dodge  = false;
    may_block  = false;
    may_crit   = false;

    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0.0;

    base_dd_min = base_dd_max    = 1.0;
    direct_power_mod             = 1.0;
    base_spell_power_multiplier  = 0.32;
    base_attack_power_multiplier = 0.20;

    weapon            = &( p -> main_hand_weapon );
    weapon_multiplier = 0.0;

    cooldown -> duration = timespan_t::from_seconds( 8 );
  }
};

// Seal of Truth ============================================================

struct seal_of_truth_dot_t : public paladin_melee_attack_t
{
  seal_of_truth_dot_t( paladin_t* p )
    : paladin_melee_attack_t( "censure", p, p -> find_spell( 31803 ), SCHOOL_NONE, false )
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
    dot_behavior     = DOT_REFRESH;

    base_spell_power_multiplier  = tick_power_mod;
    base_attack_power_multiplier = data().extra_coeff();
    tick_power_mod               = 1.0;
  }

  virtual void player_buff()
  {
    paladin_melee_attack_t::player_buff();
    paladin_targetdata_t* td = targetdata() -> cast_paladin();
    player_multiplier *= td -> debuffs_censure -> stack();
  }

  virtual void impact( player_t* t, result_type_e impact_result, double travel_dmg )
  {
    if ( result_is_hit( impact_result ) )
    {
      paladin_targetdata_t* td = targetdata( t ) -> cast_paladin();
      td -> debuffs_censure -> trigger();
      player_buff(); // update with new stack of the debuff
    }
    paladin_melee_attack_t::impact( t, impact_result, travel_dmg );
  }

  virtual void last_tick( dot_t* d )
  {
    paladin_targetdata_t* td = targetdata() -> cast_paladin();
    paladin_melee_attack_t::last_tick( d );
    td -> debuffs_censure -> expire();
  }
};

struct seal_of_truth_proc_t : public paladin_melee_attack_t
{
  seal_of_truth_proc_t( paladin_t* p )
    : paladin_melee_attack_t( "seal_of_truth", p, p -> find_spell( 42463 ) )
  {
    background  = true;
    proc        = true;
    may_miss    = false;
    may_dodge   = false;
    may_parry   = false;
  }
  virtual void player_buff()
  {
    paladin_targetdata_t* td = targetdata() -> cast_paladin();
    paladin_melee_attack_t::player_buff();
    player_multiplier *= td -> debuffs_censure -> stack() * 0.2;
  }
};

struct seal_of_truth_judgement_t : public paladin_melee_attack_t
{
  seal_of_truth_judgement_t( paladin_t* p )
    : paladin_melee_attack_t( "judgement_of_truth", p, p -> find_spell( 31804 ) )
  {
    background   = true;
    may_parry    = false;
    may_dodge    = false;
    may_block    = false;
    trigger_seal = 1;

    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0.0;

    base_dd_min = base_dd_max    = 1.0;
    base_spell_power_multiplier  = direct_power_mod;
    base_attack_power_multiplier = data().extra_coeff();
    direct_power_mod             = 1.0;
  }

  virtual void player_buff()
  {
    paladin_targetdata_t* td = targetdata() -> cast_paladin();
    paladin_melee_attack_t::player_buff();
    player_multiplier *= 1.0 + td -> debuffs_censure -> stack() * 0.20;
  }
};

// Seals of Command proc ====================================================

struct seals_of_command_proc_t : public paladin_melee_attack_t
{
  seals_of_command_proc_t( paladin_t* p )
    : paladin_melee_attack_t( "seals_of_command", p, p -> find_spell( 20424 ) )
  {
    background  = true;
    proc        = true;
  }
};

// Judgement ================================================================

struct judgement_t : public paladin_melee_attack_t
{
  action_t* seal_of_justice;
  action_t* seal_of_insight;
  action_t* seal_of_righteousness;
  action_t* seal_of_truth;

  judgement_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "judgement", p, p -> find_spell( "Judgment" ) ),
      seal_of_justice( 0 ), seal_of_insight( 0 ),
      seal_of_righteousness( 0 ), seal_of_truth( 0 )
  {
    parse_options( NULL, options_str );

    use_spell_haste = p -> passives.sanctity_of_battle -> ok();
    seal_of_justice       = new seal_of_justice_judgement_t      ( p );
    seal_of_insight       = new seal_of_insight_judgement_t      ( p );
    seal_of_righteousness = new seal_of_righteousness_judgement_t( p );
    seal_of_truth         = new seal_of_truth_judgement_t        ( p );

    if ( p -> set_bonus.pvp_4pc_melee() )
      cooldown -> duration -= timespan_t::from_seconds( 1.0 );
  }

  action_t* active_seal() const
  {
    switch ( p() -> active_seal )
    {
    case SEAL_OF_JUSTICE:
      return seal_of_justice;
    case SEAL_OF_INSIGHT:
      return seal_of_insight;
    case SEAL_OF_RIGHTEOUSNESS:
      return seal_of_righteousness;
    case SEAL_OF_TRUTH:
      return seal_of_truth;
    default:
      return 0;
      break;
    }
    return 0;
  }

  virtual void execute()
  {
    action_t* seal = active_seal();

    if ( ! seal )
      return;

    seal -> base_costs[ current_resource() ]   = base_costs[ current_resource() ];
    seal -> cooldown    = cooldown;
    seal -> trigger_gcd = trigger_gcd;
    seal -> execute();

    if ( seal -> result_is_hit() )
    {
      p() -> buffs.judgements_of_the_pure -> trigger();
      p() -> buffs.sacred_duty-> trigger();
    }

    p() -> procs.judgments_of_the_bold -> occur();
    p() -> resource_gain( RESOURCE_HOLY_POWER, 1, p() -> gains.judgements_of_the_bold );

    // Mists of Pandaria: Retribution Paladin Judgments trigger Physical Vulnerability
    if ( p() -> primary_tree() == PALADIN_RETRIBUTION && ! sim -> overrides.physical_vulnerability )
      target -> debuffs.physical_vulnerability -> trigger();

    p() -> buffs.judgements_of_the_wise -> trigger();

    p() -> last_foreground_action = seal; // Necessary for DPET calculations.

  }

  virtual bool ready()
  {
    action_t* seal = active_seal();
    if ( ! seal ) return false;
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
    if ( result_is_hit() )
    {
      p() -> buffs.sacred_duty -> expire();
    }
  }

  virtual void player_buff()
  {
    paladin_melee_attack_t::player_buff();

    static const double holypower_pm[] = { 0, 1.0, 3.0, 6.0 };
#ifndef NDEBUG
    assert( static_cast<unsigned>( p() -> holy_power_stacks() ) < sizeof_array( holypower_pm ) );
#endif
    player_multiplier = holypower_pm[ p() -> holy_power_stacks() ];

    if ( p() -> buffs.sacred_duty -> up() )
    {
      player_crit += 1.0;
    }
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
    : paladin_melee_attack_t( "templars_verdict", p, p -> find_class_spell( "Templar's Verdict" ) )
  {
    parse_options( NULL, options_str );

    trigger_seal      = true;
  }

  virtual void execute()
  {
    static const double holypower_wp[] = { 0, 0.30, 0.90, 2.35 };
#ifndef NDEBUG
    assert( static_cast<unsigned>( p() -> holy_power_stacks() ) < sizeof_array( holypower_wp ) );
#endif
    weapon_multiplier = holypower_wp[ p() -> holy_power_stacks() ];
    paladin_melee_attack_t::execute();
    if ( result_is_hit() )
    {
      trigger_hand_of_light( this );
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

    harmful = false;
  }

  virtual void execute()
  {
    paladin_spell_t::execute();

    p() -> buffs.avenging_wrath -> trigger( 1, data().effect1().percent() );
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

  virtual void impact( player_t* t, result_type_e impact_result, double travel_dmg )
  {
    if ( t -> debuffs.flying -> check() )
    {
      if ( sim -> debug ) log_t::output( sim, "Ground effect %s can not hit flying target %s", name(), t -> name_str.c_str() );
    }
    else
    {
      paladin_spell_t::impact( t, impact_result, travel_dmg );
    }
  }

  virtual void tick( dot_t* d )
  {
    if ( sim -> debug ) log_t::output( sim, "%s ticks (%d of %d)", name(), d -> current_tick, d -> num_ticks );
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

// Exorcism =================================================================

struct exorcism_t : public paladin_spell_t
{
  int undead_demon;

  exorcism_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "exorcism", p, p -> find_class_spell( "Exorcism" ) ), undead_demon( 0 )
  {
    option_t options[] =
    {
      { "undead_demon", OPT_BOOL, &undead_demon },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_attack_power_multiplier = 0.344;
    base_spell_power_multiplier  = 0.344;
    direct_power_mod             = 1.0;
    dot_behavior                 = DOT_REFRESH;
    hasted_ticks                 = false;
    may_crit                     = true;
    tick_may_crit                = true;
    tick_power_mod               = 0.2/3; // glyph of exorcism is 20% of damage over three ticks

    cooldown = p -> cooldowns.exorcism;
    cooldown -> duration = data().cooldown();
  }

  virtual double total_power() const
  {
    return ( std::max )( total_spell_power(), total_attack_power() );
  }

  virtual void execute()
  {
    paladin_spell_t::execute();

    paladin_targetdata_t* td = targetdata() -> cast_paladin();

    // FIXME: Should this be wrapped in a result_is_hit() ?
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
      if ( td -> debuffs_censure -> stack() >= 1 ) p() -> active_seal_of_truth_proc -> execute();
      break;
    default:
      ;
    }
  }

  virtual bool ready()
  {
    if ( undead_demon )
    {
      int target_race = target -> race;
      if ( target_race != RACE_UNDEAD &&
           target_race != RACE_DEMON  )
        return false;
    }

    return paladin_spell_t::ready();
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

    if ( p() -> primary_tree() == PALADIN_RETRIBUTION )
      p() -> guardian_of_ancient_kings -> summon( p() -> spells.guardian_of_ancient_kings_ret -> duration() );
    else if ( p() -> primary_tree() == PALADIN_PROTECTION )
      p() -> buffs.gotak_prot -> trigger();
  }
};

// Holy Shield ==============================================================

struct holy_shield_t : public paladin_spell_t
{
  holy_shield_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "holy_shield", p, p -> find_spell( 20925 ) )
  {
    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    paladin_spell_t::execute();

    p() -> buffs.holy_shield -> trigger();
  }
};

// Holy Shock ===============================================================

// TODO: fix the fugly hack
struct holy_shock_t : public paladin_spell_t
{
  holy_shock_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "holy_shock", p, p -> find_class_spell( "Holy Shock" ) )
  {
    parse_options( NULL, options_str );

    // hack! spell 20473 has the cooldown/cost/etc stuff, but the actual spell cast
    // to do damage is 25912
    parse_effect_data( ( *player -> dbc.effect( 25912 ) ) );
  }

  virtual void execute()
  {
    paladin_spell_t::execute();
    if ( result_is_hit() )
    {
      p() -> resource_gain( RESOURCE_HOLY_POWER, 1, p() -> gains.hp_holy_shock );
    }
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

// Inquisition ==============================================================

struct inquisition_t : public paladin_spell_t
{
  timespan_t base_duration;
  inquisition_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "inquisition", p, p -> find_class_spell( "Inquisition" ) ), base_duration( timespan_t::zero() )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    p() -> buffs.inquisition -> buff_duration = base_duration * p() -> holy_power_stacks();
    p() -> buffs.inquisition -> trigger( 1, data().effect1().base_value() );

    paladin_spell_t::execute();
  }
};

// ==========================================================================
// Paladin Heals
// ==========================================================================

// paladin_heal_t::execute ==================================================

void paladin_heal_t::execute()
{
  heal_t::execute();

  if ( target != p() -> beacon_target )
    trigger_beacon_of_light( this );

  trigger_illuminated_healing( this );
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

  virtual timespan_t execute_time() const
  {
    timespan_t t = paladin_heal_t::execute_time();

    if ( p() -> buffs.infusion_of_light -> up() )
      t += p() -> buffs.infusion_of_light -> data().effect1().time_value();

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

  virtual timespan_t execute_time() const
  {
    timespan_t t = paladin_heal_t::execute_time();

    if ( p() -> buffs.infusion_of_light -> up() )
      t += p() -> buffs.infusion_of_light -> data().effect1().time_value();

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

  virtual timespan_t execute_time() const
  {
    timespan_t t = paladin_heal_t::execute_time();

    if ( p() -> buffs.infusion_of_light -> up() )
      t += p() -> buffs.infusion_of_light -> data().effect1().time_value();

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
    aoe = data().effect2().base_value();

    hot = new holy_radiance_hot_t( p, data().effect1().trigger_spell_id() );
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

  virtual timespan_t execute_time() const
  {
    timespan_t t = paladin_heal_t::execute_time();

    if ( p() -> buffs.infusion_of_light -> up() )
      t += p() -> buffs.infusion_of_light -> data().effect1().time_value();

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

    p() -> resource_gain( RESOURCE_HOLY_POWER,
                        p() -> dbc.spell( 25914 ) -> effect2().base_value(),
                        p() -> gains.hp_holy_shock );

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

  virtual void player_buff()
  {
    paladin_heal_t::player_buff();

    player_multiplier *= p() -> holy_power_stacks();
  }
};

// Word of Glory Spell ======================================================

struct word_of_glory_t : public paladin_heal_t
{
  word_of_glory_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "word_of_glory", p, p -> find_class_spell( "Word of Glory" ) )
  {
    parse_options( NULL, options_str );

    base_attack_power_multiplier = 1.0;
    base_spell_power_multiplier  = 0.0;

    base_multiplier *= 1.0 + p -> passives.meditation -> effect2().percent();

    // Hot is built into the spell, but only becomes active with the glyph
    base_td = 0;
    base_tick_time = timespan_t::zero();
    num_ticks = 0;
  }

  virtual void player_buff()
  {
    paladin_heal_t::player_buff();

    player_multiplier *= p() -> holy_power_stacks();    
  }
};

} // ANONYMOUS NAMESPACE ====================================================

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
  if ( name == "consecration"              ) return new consecration_t             ( this, options_str );
  if ( name == "crusader_strike"           ) return new crusader_strike_t          ( this, options_str );
  if ( name == "divine_plea"               ) return new divine_plea_t              ( this, options_str );
  if ( name == "divine_protection"         ) return new divine_protection_t        ( this, options_str );
  if ( name == "divine_shield"             ) return new divine_shield_t            ( this, options_str );
  if ( name == "divine_storm"              ) return new divine_storm_t             ( this, options_str );
  if ( name == "exorcism"                  ) return new exorcism_t                 ( this, options_str );
  if ( name == "hammer_of_justice"         ) return new hammer_of_justice_t        ( this, options_str );
  if ( name == "hammer_of_wrath"           ) return new hammer_of_wrath_t          ( this, options_str );
  if ( name == "hammer_of_the_righteous"   ) return new hammer_of_the_righteous_t  ( this, options_str );
  if ( name == "holy_radiance"             ) return new holy_radiance_t            ( this, options_str );
  if ( name == "holy_shield"               ) return new holy_shield_t              ( this, options_str );
  if ( name == "holy_shock"                ) return new holy_shock_t               ( this, options_str );
  if ( name == "holy_shock_heal"           ) return new holy_shock_heal_t          ( this, options_str );
  if ( name == "holy_wrath"                ) return new holy_wrath_t               ( this, options_str );
  if ( name == "guardian_of_ancient_kings" ) return new guardian_of_ancient_kings_t( this, options_str );
  if ( name == "inquisition"               ) return new inquisition_t              ( this, options_str );
  if ( name == "judgement"                 ) return new judgement_t                ( this, options_str );
  if ( name == "light_of_dawn"             ) return new light_of_dawn_t            ( this, options_str );
  if ( name == "rebuke"                    ) return new rebuke_t                   ( this, options_str );
  if ( name == "shield_of_the_righteous"   ) return new shield_of_the_righteous_t  ( this, options_str );
  if ( name == "templars_verdict"          ) return new templars_verdict_t         ( this, options_str );

  if ( name == "seal_of_justice"           ) return new paladin_seal_t( this, "seal_of_justice",       SEAL_OF_JUSTICE,       options_str );
  if ( name == "seal_of_insight"           ) return new paladin_seal_t( this, "seal_of_insight",       SEAL_OF_INSIGHT,       options_str );
  if ( name == "seal_of_righteousness"     ) return new paladin_seal_t( this, "seal_of_righteousness", SEAL_OF_RIGHTEOUSNESS, options_str );
  if ( name == "seal_of_truth"             ) return new paladin_seal_t( this, "seal_of_truth",         SEAL_OF_TRUTH,         options_str );

  if ( name == "word_of_glory"             ) return new word_of_glory_t            ( this, options_str );
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

  initial_parry_rating_per_strength = 0.27;
}

// paladin_t::init_base =====================================================

void paladin_t::init_base()
{
  player_t::init_base();

  stats_initial.attack_power_per_strength = 2.0;
  stats_initial.spell_power_per_intellect = 1.0;

  base_spell_power  = 0;
  stats_base.attack_power = level * 3;

  resources.base[ RESOURCE_HOLY_POWER ] = 3 + passives.boundless_conviction -> effectN( 1 ).base_value();

  // FIXME! Level-specific!
  stats_base.miss    = 0.060;
  stats_base.parry   = 0.044; // 85
  stats_base.block   = 0.030; // 85

  diminished_kfactor    = 0.009560;
  diminished_dodge_capi = 0.01523660;
  diminished_parry_capi = 0.01523660;

  mana_per_intellect = 15;

  switch ( primary_tree() )
  {
  case PALADIN_HOLY:
    stats_base.attack_hit += 0; // TODO spirit -> hit talents.enlightened_judgements
    stats_base.spell_hit  += 0; // TODO spirit -> hit talents.enlightened_judgements
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
}

// paladin_t::init_gains ====================================================

void paladin_t::init_gains()
{
  player_t::init_gains();

  gains.divine_plea            = get_gain( "divine_plea"            );
  gains.judgements_of_the_wise = get_gain( "judgements_of_the_wise" );
  gains.judgements_of_the_bold = get_gain( "judgements_of_the_bold" );
  gains.sanctuary              = get_gain( "sanctuary"              );
  gains.seal_of_command_glyph  = get_gain( "seal_of_command_glyph"  );
  gains.seal_of_insight        = get_gain( "seal_of_insight"        );

  // Holy Power
  gains.hp_blessed_life             = get_gain( "holy_power_blessed_life" );
  gains.hp_crusader_strike          = get_gain( "holy_power_crusader_strike" );
  gains.hp_divine_plea              = get_gain( "holy_power_divine_plea" );
  gains.hp_divine_storm             = get_gain( "holy_power_divine_storm" );
  gains.hp_grand_crusader           = get_gain( "holy_power_grand_crusader" );
  gains.hp_hammer_of_the_righteous  = get_gain( "holy_power_hammer_of_the_righteous" );
  gains.hp_holy_shock               = get_gain( "holy_power_holy_shock" );
  gains.hp_pursuit_of_justice       = get_gain( "holy_power_pursuit_of_justice" );
  gains.hp_tower_of_radiance        = get_gain( "holy_power_tower_of_radiance" );
  gains.hp_judgement                = get_gain( "holy_power_judgement" );
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

  specialization_e tree = primary_tree();

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

int paladin_t::decode_set( const item_t& item ) const
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

  if ( strstr( s, "gladiators_ornamented_"  ) ) return SET_PVP_HEAL;
  if ( strstr( s, "gladiators_scaled_"      ) ) return SET_PVP_MELEE;

  return SET_NONE;
}

// paladin_t::init_buffs ====================================================

void paladin_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )

  buffs.ancient_power          = buff_creator_t( this, "ancient_power", find_spell( 86700 ) );
  buffs.avenging_wrath         = buff_creator_t( this, "avenging_wrath", find_class_spell( "Avenging Wrath" ) ).cd( timespan_t::zero() ); // Let the ability handle the CD
  buffs.divine_plea            = buff_creator_t( this, "divine_plea", find_spell( 54428 ) ).max_stack( 1 ).cd( timespan_t::zero() ); // Let the ability handle the CD
  buffs.divine_protection      = buff_creator_t( this, "divine_protection", find_spell( 498 ) ).max_stack( 1 ).cd( timespan_t::zero() ); // Let the ability handle the CD
  buffs.divine_shield          = buff_creator_t( this, "divine_shield", find_spell( 642 ) ).max_stack( 1 ).cd( timespan_t::zero() ); // Let the ability handle the CD
  buffs.gotak_prot             = buff_creator_t( this, "guardian_of_the_ancient_kings", find_spell( 86659 ) );
  buffs.holy_shield            = buff_creator_t( this, "holy_shield", find_spell( 20925 ) );
  buffs.inquisition            = buff_creator_t( this, "inquisition", find_spell( 84963 ) );
  buffs.judgements_of_the_wise = buff_creator_t( this, "judgements_of_the_wise", find_specialization_spell( "Judgments of the Wise" ) );
  buffs.zealotry               = buff_creator_t( this, "zealotry", passives.crusaders_zeal )
    .default_value( find_spell( 107397 ) -> effectN( 1 ).percent() )
    .max_stack( 3 )
    .duration( find_spell( 107397 ) -> duration() );
}

// paladin_t::init_actions ==================================================

void paladin_t::init_actions()
{
  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( !quiet )
      sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  active_hand_of_light_proc          = new hand_of_light_proc_t         ( this );
//  active_seals_of_command_proc       = new seals_of_command_proc_t      ( this );
  active_seal_of_justice_proc        = new seal_of_justice_proc_t       ( this );
  active_seal_of_insight_proc        = new seal_of_insight_proc_t       ( this );
  active_seal_of_righteousness_proc  = new seal_of_righteousness_proc_t ( this );
  active_seal_of_truth_proc          = new seal_of_truth_proc_t         ( this );
  active_seal_of_truth_dot           = new seal_of_truth_dot_t          ( this );
//  ancient_fury_explosion             = new ancient_fury_t               ( this );

  if ( action_list_str.empty() )
  {
    switch ( primary_tree() )
    {
    case PALADIN_RETRIBUTION:
    {
#if 0
      if ( level > 80 )
      {
        action_list_str += "/flask,type=titanic_strength/food,type=beer_basted_crocolisk";
      }
      else
      {
        action_list_str += "/flask,type=endless_rage/food,type=dragonfin_filet";
      }
#endif
      action_list_str += "/seal_of_truth";
      action_list_str += "/snapshot_stats";
      // TODO: action_list_str += "/rebuke";
#if 0
      if ( level > 80 )
      {
        action_list_str += "/golemblood_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<=40";
      }
      else
      {
        action_list_str += "/speed_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<=60";
      }

      // This should<tm> get Censure up before the auto attack lands
      action_list_str += "/auto_attack/judgement,if=buff.judgements_of_the_pure.down";
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
      action_list_str += "/crusader_strike,if=holy_power<3";  // CS before TV if <3 power, even with DP up
      action_list_str += "/judgement";
      if ( level >= 81 )
        action_list_str += "/inquisition,if=(buff.inquisition.down|buff.inquisition.remains<=2)&(holy_power>=3|buff.divine_purpose.react)";
      action_list_str += "/templars_verdict,if=buff.divine_purpose.react";
      action_list_str += "/templars_verdict,if=holy_power=3";
      action_list_str += "/exorcism,if=buff.the_art_of_war.react";
      action_list_str += "/hammer_of_wrath";
      action_list_str += "/judgement,if=set_bonus.tier13_2pc_melee&holy_power<3";
      action_list_str += "/wait,sec=0.1,if=cooldown.crusader_strike.remains<0.2&cooldown.crusader_strike.remains>0";
      action_list_str += "/holy_wrath";
      action_list_str += "/consecration,not_flying=1,if=mana>16000";  // Consecration is expensive, only use if we have plenty of mana
      action_list_str += "/divine_plea";
#else
      action_list_str += "/crusader_strike";
#endif
    }
    break;
    case PALADIN_PROTECTION:
    {
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
      action_list_str += "/holy_shield,use_off_gcd=1";
      action_list_str += "/shield_of_the_righteous,if=holy_power=3&(buff.sacred_duty.up|buff.inquisition.up)";
      action_list_str += "/judgement,if=holy_power=3";
      action_list_str += "/inquisition,if=holy_power=3&(buff.inquisition.down|buff.inquisition.remains<5)";
      action_list_str += "/divine_plea,if=holy_power<2";
      action_list_str += "/avengers_shield,if=buff.grand_crusader.up&holy_power<3";
      action_list_str += "/judgement,if=buff.judgements_of_the_pure.down";
      action_list_str += "/crusader_strike,if=holy_power<3";
      action_list_str += "/hammer_of_wrath";
      action_list_str += "/avengers_shield,if=cooldown.crusader_strike.remains>=0.2";
      action_list_str += "/judgement";
      action_list_str += "/consecration,not_flying=1";
      action_list_str += "/holy_wrath";
      action_list_str += "/divine_plea,if=holy_power<1";
    }
    break;
    case PALADIN_HOLY:
    {
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
      action_list_str += "/judgement,if=buff.judgements_of_the_pure.remains<10";
      action_list_str += "/word_of_glory,if=holy_power>2";
      action_list_str += "/holy_shock_heal";
      action_list_str += "/divine_light,if=mana_pct>75";
      action_list_str += "/divine_plea,if=mana_pct<75";
      action_list_str += "/holy_light";
    }
    break;
    default:
      if ( ! quiet ) abort();
      break;
    }
    action_list_default = 1;
  }

  player_t::init_actions();
}

void paladin_t::init_spells()
{
  player_t::init_spells();

  // Spells
  spells.guardian_of_ancient_kings_ret = find_class_spell( "Guardian Of Ancient Kings" );

  // Masteries
  passives.divine_bulwark         = find_mastery_spell( "Divine Bulwark" );
  passives.hand_of_light          = find_mastery_spell( "Hand of Light" );
  passives.illuminated_healing    = find_mastery_spell( "Illuminated Healing" );
  // Passives

  // Shared Passives
  passives.boundless_conviction   = find_spell( 115675 ); // find_specialization_spell( "Boundless Conviction" ); FIX-ME: (not in our spell lists for some reason)
  passives.plate_specialization   = find_specialization_spell( "Plate Specialization" );
  passives.sanctity_of_battle     = find_spell( 25956 ); // FIX-ME: find_specialization_spell( "Sanctity of Battle" )   (not in spell lists yet)

  // Holy Passives

  // Prot Passives
  passives.judgements_of_the_wise = find_specialization_spell( "Judgements of the Wise" );
  passives.vengeance              = find_specialization_spell( "Vengeance" );
  if ( passives.vengeance -> ok() )
    vengeance_enabled = true;
  
  // Ret Passives
  passives.crusaders_zeal         = find_specialization_spell( "Crusader's Zeal" );
  passives.judgements_of_the_bold = find_specialization_spell( "Judgements of the Bold" );
  passives.sword_of_light         = find_specialization_spell( "Sword of Light" );
  passives.sword_of_light_value   = find_spell( passives.sword_of_light -> ok() ? 20113 : 0 );
  passives.the_art_of_war         = find_specialization_spell( "The Art of War" );

  // Gear passives
  passives.tier13_4pc_melee_value = find_spell( 105819 );

  // Glyphs
/*
  glyphs.ascetic_crusader         = find_glyph( "Glyph of the Ascetic Crusader" );
  glyphs.beacon_of_light          = find_glyph( "Glyph of Beacon of Light" );
  glyphs.consecration             = find_glyph( "Glyph of Consecration" );
  glyphs.crusader_strike          = find_glyph( "Glyph of Crusader Strike" );
  glyphs.divine_favor             = find_glyph( "Glyph of Divine Favor" );
  glyphs.divine_plea              = find_glyph( "Glyph of Divine Plea" );
  glyphs.divine_protection        = find_glyph( "Glyph of Divine Protection" );
  glyphs.divinity                 = find_glyph( "Glyph of Divinity" );
  glyphs.exorcism                 = find_glyph( "Glyph of Exorcism" );
  glyphs.focused_shield           = find_glyph( "Glyph of Focused Shield" );
  glyphs.hammer_of_the_righteous  = find_glyph( "Glyph of Hammer of the Righteous" );
  glyphs.hammer_of_wrath          = find_glyph( "Glyph of Hammer of Wrath" );
  glyphs.holy_shock               = find_glyph( "Glyph of Holy Shock" );
  glyphs.judgement                = find_glyph( "Glyph of Judgement" );
  glyphs.lay_on_hands             = find_glyph( "Glyph of Lay on Hands" );
  glyphs.light_of_dawn            = find_glyph( "Glyph of Light of Dawn" );
  glyphs.long_word                = find_glyph( "Glyph of the Long Word" );
  glyphs.rebuke                   = find_glyph( "Glyph of Rebuke" );
  glyphs.seal_of_insight          = find_glyph( "Glyph of Seal of Insight" );
  glyphs.seal_of_truth            = find_glyph( "Glyph of Seal of Truth" );
  glyphs.shield_of_the_righteous  = find_glyph( "Glyph of Shield of the Righteous" );
  glyphs.templars_verdict         = find_glyph( "Glyph of Templar's Verdict" );
  glyphs.word_of_glory            = find_glyph( "Glyph of Word of Glory" );
*/

  // Tier Bonuses
  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //  C2P    C4P     M2P     M4P     T2P     T4P     H2P     H4P
    {     0,     0, 105765, 105820, 105800, 105744, 105743, 105798 }, // Tier13
    {     0,     0,      0,      0,      0,      0,      0,      0 },
  };

  sets = new set_bonus_array_t( this, set_bonuses );
}

// paladin_t::init_values ===================================================

void paladin_t::init_values()
{
  player_t::init_values();

  if ( set_bonus.pvp_2pc_heal() )
    attribute_initial[ ATTR_INTELLECT ] += 70;

  if ( set_bonus.pvp_4pc_heal() )
    attribute_initial[ ATTR_INTELLECT ] += 90;

  if ( set_bonus.pvp_2pc_melee() )
    attribute_initial[ ATTR_STRENGTH ] += 70;

  if ( set_bonus.pvp_4pc_melee() )
    attribute_initial[ ATTR_STRENGTH ] += 90;
}

void paladin_t::init_items()
{
  player_t::init_items();

  items.size();
  for ( size_t i = 0; i < items.size(); ++i )
  {
    const item_t& item = items[ i ];
    if ( item.slot == SLOT_HANDS && ret_pvp_gloves == -1 )  // i.e. hasn't been overriden by option
    {
      ret_pvp_gloves = strstr( item.name(), "gladiators_scaled_gauntlets" ) && item.ilevel > 140;
    }
  }
}
// paladin_t::primary_role ==================================================

role_type_e paladin_t::primary_role() const
{
  if ( player_t::primary_role() == ROLE_DPS || primary_tree() == PALADIN_RETRIBUTION )
    return ROLE_HYBRID;

  if ( player_t::primary_role() == ROLE_TANK || primary_tree() == PALADIN_PROTECTION  )
    return ROLE_TANK;

  if ( player_t::primary_role() == ROLE_HEAL || primary_tree() == PALADIN_HOLY )
    return ROLE_HEAL;

  return ROLE_HYBRID;
}

// paladin_t::composite_attribute_multiplier ================================

double paladin_t::composite_attribute_multiplier( attribute_type_e attr ) const
{
  double m = player_t::composite_attribute_multiplier( attr );
  if ( attr == ATTR_STRENGTH && buffs.ancient_power -> check() )
  {
    m *= 1.0 + 0.01 * buffs.ancient_power -> stack();
  }
  return m;
}

double paladin_t::composite_attack_speed() const
{
  double m = player_t::composite_attack_speed();

  if ( buffs.zealotry -> up() )
  {
    m /= 1.0 + buffs.zealotry -> value();
  }

  return m;
}

// paladin_t::composite_player_multiplier ===================================

double paladin_t::composite_player_multiplier( school_type_e school, const action_t* a ) const
{
  double m = player_t::composite_player_multiplier( school, a );

  // These affect all damage done by the paladin
  m *= 1.0 + buffs.avenging_wrath -> value();

  m *= 1.0 + ( ( buffs.zealotry -> up() ) ? set_bonus.tier13_4pc_melee() * passives.tier13_4pc_melee_value -> effectN( 1 ).percent() : 0.0 );

  if ( school == SCHOOL_HOLY )
  {
    m *= 1.0 + buffs.inquisition -> value();
  }

  if ( a && a -> type == ACTION_ATTACK && ! a -> class_flag1 && ( passives.sword_of_light -> ok() ) && ( main_hand_weapon.group() == WEAPON_2H ) )
  {
    m *= 1.0 + passives.sword_of_light_value -> effectN( 1 ).percent();
  }
  return m;
}

// paladin_t::composite_spell_power =========================================

double paladin_t::composite_spell_power( school_type_e school ) const
{
  double sp = player_t::composite_spell_power( school );
  switch ( primary_tree() )
  {
  case PALADIN_PROTECTION:
    break;
  case PALADIN_RETRIBUTION:
    sp += passives.sword_of_light -> effectN( 1 ).percent() * strength();
    break;
  default:
    break;
  }
  return sp;
}

// paladin_t::composite_tank_block ==========================================

double paladin_t::composite_tank_block() const
{
  double b = player_t::composite_tank_block();
  b += get_divine_bulwark();
  return b;
}

// paladin_t::composite_tank_block_reduction ================================

double paladin_t::composite_tank_block_reduction() const
{
  double b = player_t::composite_tank_block_reduction();
  b += buffs.holy_shield -> up() * buffs.holy_shield -> data().effect1().percent();
  return b;
}

// paladin_t::composite_tank_crit ===========================================

double paladin_t::composite_tank_crit( const school_type_e school ) const
{
  double c = player_t::composite_tank_crit( school );

  return c;
}

// paladin_t::matching_gear_multiplier ======================================

double paladin_t::matching_gear_multiplier( const attribute_type_e attr ) const
{
  double mult = 0.01 * passives.plate_specialization -> effectN( 1 ).base_value();
  switch ( primary_tree() )
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


  if ( buffs.divine_plea -> up() )
  {
    double tick_pct = ( buffs.divine_plea -> data().effectN( 1 ).base_value() ) * 0.01;
    double tick_amount = resources.max[ RESOURCE_MANA ] * tick_pct;
    double amount = periodicity.total_seconds() * tick_amount / 3;
    resource_gain( RESOURCE_MANA, amount, gains.divine_plea );
  }
  if ( buffs.judgements_of_the_wise -> up() )
  {
    double tot_amount = resources.base[ RESOURCE_MANA ] * buffs.judgements_of_the_wise -> data().effect1().percent();
    double amount = periodicity.total_seconds() * tot_amount / buffs.judgements_of_the_wise -> buff_duration.total_seconds();
    resource_gain( RESOURCE_MANA, amount, gains.judgements_of_the_wise );
  }
}

// paladin_t::assess_damage =================================================

double paladin_t::assess_damage( double        amount,
                                 school_type_e school,
                                 dmg_type_e    dtype,
                                 result_type_e result,
                                 action_t*     action )
{
  if ( buffs.divine_shield -> up() )
  {
    amount = 0;

    // Return out, as you don't get to benefit from anything else
    return player_t::assess_damage( amount, school, dtype, result, action );
  }

  if ( buffs.gotak_prot -> up() )
    amount *= 1.0 + dbc.spell( 86657 ) -> effect2().percent(); // Value of the buff is stored in another spell

  if ( buffs.divine_protection -> up() )
  {
    if ( school == SCHOOL_PHYSICAL )
    {
      amount *= 1.0 + buffs.divine_protection -> data().effect2().percent();
    }
    else
    {
      amount *= 1.0 + buffs.divine_protection -> data().effect3().percent();
    }
  }

  if ( result == RESULT_PARRY )
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

  return player_t::assess_damage( amount, school, dtype, result, action );
}

// paladin_t::assess_heal ===================================================

player_t::heal_info_t paladin_t::assess_heal( double        amount,
                                              school_type_e school,
                                              dmg_type_e    dtype,
                                              result_type_e result,
                                              action_t*     action )
{
  return player_t::assess_heal( amount, school, dtype, result, action );
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

int paladin_t::holy_power_stacks() const
{
  return ( int ) resources.current[ RESOURCE_HOLY_POWER ];
}

// paladin_t::get_divine_bulwark ============================================

double paladin_t::get_divine_bulwark() const
{
  if ( primary_tree() != PALADIN_PROTECTION ) return 0.0;

  // block rating, 2.25% per point of mastery
  return composite_mastery() * ( passives.divine_bulwark -> effectN( 1 ).coeff() / 100.0 );
}

// paladin_t::get_hand_of_light =============================================

double paladin_t::get_hand_of_light() const
{
  if ( primary_tree() != PALADIN_RETRIBUTION ) return 0.0;

  // chance to proc buff, 1% per point of mastery
  return composite_mastery() * ( passives.hand_of_light -> effectN( 1 ).coeff() / 100.0 );
}

#endif // SC_PALADIN

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

player_t* player_t::create_paladin( sim_t* sim, const std::string& name, race_type_e r )
{
  SC_CREATE_PALADIN( sim, name, r );
}

// player_t::paladin_init ===================================================

void player_t::paladin_init( sim_t* sim )
{
  for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[i];
    p -> buffs.beacon_of_light          = buff_creator_t( p, "beacon_of_light", p -> find_spell( 53563 ) );
    p -> buffs.illuminated_healing      = buff_creator_t( p, "illuminated_healing", p -> find_spell( 86273 ) );
    p -> debuffs.forbearance            = buff_creator_t( p, "forbearance", p -> find_spell( 25771 ) );
  }
}

// player_t::paladin_combat_begin ===========================================

void player_t::paladin_combat_begin( sim_t* )
{
}
