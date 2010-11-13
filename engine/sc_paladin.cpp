// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Paladin
// ==========================================================================

namespace {

enum seal_type_t { 
  SEAL_NONE=0,
  SEAL_OF_JUSTICE,
  SEAL_OF_INSIGHT,
  SEAL_OF_RIGHTEOUSNESS,
  SEAL_OF_TRUTH,
  SEAL_MAX
};

}

struct paladin_t : public player_t
{
  // Active
  int       active_seal;
  action_t* active_seals_of_command_proc;
  action_t* active_seal_of_justice_proc;
  action_t* active_seal_of_insight_proc;
  action_t* active_seal_of_righteousness_proc;
  action_t* active_seal_of_truth_proc;
  action_t* active_seal_of_truth_dot;
  action_t* ancient_fury_explosion;

  // Buffs
  buff_t* buffs_avenging_wrath;
  buff_t* buffs_divine_favor;
  buff_t* buffs_divine_plea;
  buff_t* buffs_holy_shield;
  buff_t* buffs_judgements_of_the_pure;
  buff_t* buffs_reckoning;
  buff_t* buffs_censure;
  buff_t* buffs_the_art_of_war;

  buff_t* buffs_holy_power;
  buff_t* buffs_inquisition;
  buff_t* buffs_zealotry;
  buff_t* buffs_judgements_of_the_wise;
  buff_t* buffs_judgements_of_the_bold;
  buff_t* buffs_hand_of_light;
  buff_t* buffs_ancient_power;

  // Gains
  gain_t* gains_divine_plea;
  gain_t* gains_judgements_of_the_wise;
  gain_t* gains_judgements_of_the_bold;
  gain_t* gains_seal_of_command_glyph;
  gain_t* gains_seal_of_insight;

  // Procs
  proc_t* procs_parry_haste;

  pet_t* guardian_of_ancient_kings;

  struct passives_t
  {
    passive_spell_t* touched_by_the_light;
    passive_spell_t* vengeance;
    mastery_t* divine_bulwark;
    passive_spell_t* sheath_of_light;
    passive_spell_t* two_handed_weapon_spec;
    mastery_t* hand_of_light;
    passive_spell_t* judgements_of_the_bold; // passive stuff is hidden here because spells
    passive_spell_t* judgements_of_the_wise; // can only have three effects
    passive_spell_t* plate_specialization;
    passives_t() { memset( ( void* ) this, 0x0, sizeof( passives_t ) ); }
  };
  passives_t passives;

  struct spells_t {
    active_spell_t* avengers_shield;
    active_spell_t* avenging_wrath;
    active_spell_t* consecration;
    active_spell_t* consecration_tick;
    active_spell_t* crusader_strike;
    active_spell_t* divine_favor;
    active_spell_t* divine_plea;
    active_spell_t* divine_storm;
    active_spell_t* exorcism;
    active_spell_t* guardian_of_ancient_kings;
    active_spell_t* guardian_of_ancient_kings_ret;
    active_spell_t* ancient_fury;
    active_spell_t* hammer_of_justice;
    active_spell_t* hammer_of_the_righteous;
    active_spell_t* hammer_of_wrath;
    active_spell_t* holy_shock;
    active_spell_t* holy_wrath;
    active_spell_t* inquisition;
    active_spell_t* judgement;
    active_spell_t* shield_of_the_righteous;
    active_spell_t* templars_verdict;
    active_spell_t* zealotry;
    active_spell_t* judgements_of_the_bold; // the actual self-buff
    active_spell_t* judgements_of_the_wise; // the actual self-buff
    spells_t() { memset( ( void* ) this, 0x0, sizeof( spells_t ) ); }
  } spells;

  struct talents_t
  {
    // holy
    talent_t* arbiter_of_the_light;
    int protector_of_the_innocent;
    talent_t* judgements_of_the_pure;
    int clarity_of_purpose;
    int last_word;
    talent_t* blazing_light;
    int denounce;
    talent_t* divine_favor;
    int infusion_of_light;
    int daybreak;
    int enlightened_judgements;
    int beacon_of_light;
    int speed_of_light;
    int sacred_cleansing;
    int conviction;
    int aura_mastery;
    int paragon_of_virtue;
    int tower_of_radiance;
    int blessed_life;
    int light_of_dawn;
    // prot
    int divinity;
    talent_t* seals_of_the_pure;
    int eternal_glory;
    int judgements_of_the_just;
    int toughness;
    talent_t* improved_hammer_of_justice;
    talent_t* hallowed_ground;
    int sanctuary;
    talent_t* hammer_of_the_righteous;
    talent_t* wrath_of_the_lightbringer;
    int reckoning;
    talent_t* shield_of_the_righteous;
    int grand_crusader;
    int divine_guardian;
    int vindication;
    int holy_shield;
    int guarded_by_the_light;
    int sacred_duty;
    int shield_of_the_templar;
    int ardent_defender;
    // ret
    int eye_for_an_eye;
    talent_t* crusade;
    int improved_judgement;
    int guardians_favor;
    talent_t* rule_of_law;
    int pursuit_of_justice;
    talent_t* communion; // damage aura NYI
    talent_t* the_art_of_war;
    int long_arm_of_the_law;
    talent_t* divine_storm;
    int rebuke;
    talent_t* sanctity_of_battle;
    talent_t* seals_of_command;
    talent_t* sanctified_wrath;
    int selfless_healer;
    int repentance;
    talent_t* divine_purpose;
    talent_t* inquiry_of_faith;
    int acts_of_sacrifice;
    talent_t* zealotry;

    talents_t() { memset( ( void* ) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  struct glyphs_t
  {
    // prime
    int crusader_strike;
    int exorcism;
    int holy_shock;
    int judgement;
    int hammer_of_the_righteous;
    int seal_of_truth;
    int shield_of_the_righteous;
    int templars_verdict;
    // major
    int ascetic_crusader;
    int consecration;
    int focused_shield;
    int hammer_of_wrath;
    // minor
    glyphs_t() { memset( ( void* ) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  paladin_t( sim_t* sim, const std::string& name, race_type r = RACE_NONE ) : player_t( sim, PALADIN, name, r )
  {
    tree_type[ PALADIN_HOLY        ] = TREE_HOLY;
    tree_type[ PALADIN_PROTECTION  ] = TREE_PROTECTION;
    tree_type[ PALADIN_RETRIBUTION ] = TREE_RETRIBUTION;

    active_seal = SEAL_NONE;

    active_seals_of_command_proc      = 0;
    active_seal_of_justice_proc       = 0;
    active_seal_of_insight_proc       = 0;
    active_seal_of_righteousness_proc = 0;
    active_seal_of_truth_proc         = 0;
    active_seal_of_truth_dot          = 0;
  }

  virtual void      init_race();
  virtual void      init_base();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_glyphs();
  virtual void      init_rng();
  virtual void      init_scaling();
  virtual void      init_buffs();
  virtual void      init_talents();
  virtual void      init_spells();
  virtual void      init_actions();
  virtual void      reset();
  virtual void      interrupt();
  virtual double    composite_attribute_multiplier( int attr ) SC_CONST;
  virtual double    composite_spell_power( const school_type school ) SC_CONST;
  virtual double    composite_tank_block() SC_CONST;
  virtual double    matching_gear_multiplier( const attribute_type attr ) SC_CONST;
  virtual std::vector<talent_translation_t>& get_talent_list();
  virtual std::vector<option_t>& get_options();
  virtual action_t* create_action( const std::string& name, const std::string& options_str );
  virtual int       decode_set( item_t& item );
  virtual int                primary_resource() SC_CONST { return RESOURCE_MANA; }
  virtual int                primary_role() SC_CONST     { return ROLE_HYBRID; }
  virtual talent_tab_name    primary_tab() SC_CONST;
  virtual void      regen( double periodicity );
  virtual int       target_swing();
  virtual cooldown_t* get_cooldown( const std::string& name );
  virtual pet_t*    create_pet    ( const std::string& name );
  virtual void      create_pets   ();

  bool              has_holy_power(int power_needed=1) SC_CONST;
  int               holy_power_stacks() SC_CONST;
  double            get_divine_bulwark() SC_CONST;
  double            get_hand_of_light() SC_CONST;
};

namespace { // ANONYMOUS NAMESPACE ==========================================

// trigger_judgements_of_the_wise ==========================================

static void trigger_judgements_of_the_wise( action_t* a )
{
  paladin_t* p = a -> player -> cast_paladin();

  if ( p -> primary_tree() == TREE_PROTECTION )
  {
    p -> buffs_judgements_of_the_wise -> trigger();
  }
}

static void trigger_judgements_of_the_bold( action_t* a )
{
  paladin_t* p = a -> player -> cast_paladin();

  if ( p -> primary_tree() == TREE_RETRIBUTION )
  {
    p -> buffs_judgements_of_the_bold -> trigger();
  }
}

static void trigger_hand_of_light( action_t* a )
{
  paladin_t* p = a -> player -> cast_paladin();

  if ( a -> sim -> roll( p -> get_hand_of_light() ) )
  {
    p -> buffs_hand_of_light -> trigger();
  }
}

// Retribution guardian of ancient kings pet
// TODO: melee attack
struct guardian_of_ancient_kings_ret_t : public pet_t
{
  attack_t* melee;

  struct melee_t : public attack_t
  {
    paladin_t* owner;

    melee_t(player_t *p)
      : attack_t("melee", p, RESOURCE_NONE, SCHOOL_PHYSICAL)
    {
      weapon = &(p->main_hand_weapon);
      base_execute_time = weapon -> swing_time;
      weapon_multiplier = 1.0;
      background = true;
      repeating  = true;
      trigger_gcd = 0;
      base_cost   = 0;

      owner = p->cast_pet()->owner->cast_paladin();
    }

    virtual void execute()
    {
      attack_t::execute();
      if (result_is_hit())
      {
        owner->buffs_ancient_power->trigger();
      }
    }
  };

  guardian_of_ancient_kings_ret_t(sim_t *sim, paladin_t *p)
    : pet_t( sim, p, "guardian_of_ancient_kings", true )
  {
    main_hand_weapon.type = WEAPON_BEAST;
    main_hand_weapon.swing_time = 2.0;
    main_hand_weapon.min_dmg = 5500; // TODO
    main_hand_weapon.max_dmg = 7000; // TODO
  }

  virtual void init_base()
  {
    pet_t::init_base();
    melee = new melee_t(this);
  }

  virtual void dismiss()
  {
    pet_t::dismiss();
    owner->cast_paladin()->ancient_fury_explosion->execute();
  }

  virtual void schedule_ready( double delta_time=0, bool waiting=false )
  {
    pet_t::schedule_ready( delta_time, waiting );
    if ( ! melee -> execute_event ) melee -> execute();
  }
};

// =========================================================================
// Paladin Attacks
// =========================================================================

struct paladin_attack_t : public attack_t
{
  bool trigger_seal;
  bool uses_holy_power;
  bool spell_haste; // Some attacks (CS w/ sanctity of battle, censure) use spell haste. sigh.
  double holy_power_chance;

  paladin_attack_t( const char* n, paladin_t* p, const school_type s=SCHOOL_PHYSICAL, int t=TREE_NONE, bool special=true, bool use2hspec=true ) :
      attack_t( n, p, RESOURCE_MANA, s, t, special ),
      trigger_seal( false ), uses_holy_power(false), spell_haste(false), holy_power_chance(0.0)
  {
    may_crit = true;
    if ( use2hspec && p -> primary_tree() == TREE_RETRIBUTION && p -> main_hand_weapon.group() == WEAPON_2H )
    {
      base_multiplier *= 1.0 + 0.01 * p->passives.two_handed_weapon_spec->effect_base_value(1);
    }

    base_multiplier *= 1.0 + 0.01 * p->talents.communion->effect_base_value(3);

    if (p->set_bonus.tier10_2pc_melee())
      base_multiplier *= 1.05;
  }

  virtual double haste() SC_CONST
  {
    paladin_t* p = player -> cast_paladin();
    double h = spell_haste ? p->composite_spell_haste() : attack_t::haste();
    if ( p -> buffs_judgements_of_the_pure -> check() )
    {
      h *= 1.0 / ( 1.0 + p -> talents.judgements_of_the_pure->rank() * 0.03 );
    }
    return h;
  }

  virtual void execute()
  {
    attack_t::execute();
    if ( result_is_hit() )
    {
      consume_and_gain_holy_power();

      if ( trigger_seal )
      {
        paladin_t* p = player -> cast_paladin();
        switch ( p -> active_seal )
        {
        case SEAL_OF_JUSTICE:       p -> active_seal_of_justice_proc       -> execute(); break;
        case SEAL_OF_INSIGHT:       p -> active_seal_of_insight_proc       -> execute(); break;
        case SEAL_OF_RIGHTEOUSNESS: p -> active_seal_of_righteousness_proc -> execute(); break;
        case SEAL_OF_TRUTH:         if ( p -> buffs_censure -> stack() >= 1 ) p -> active_seal_of_truth_proc -> execute(); break;
        default:;
        }

        if ( p -> talents.seals_of_command->rank() )
        {
          p -> active_seals_of_command_proc -> execute();
        }

        // It seems like it's the seal-triggering attacks that stack up ancient power
        if (!p->guardian_of_ancient_kings->sleeping)
        {
          p->buffs_ancient_power->trigger();
        }
      }
    }
  }
  
  virtual void player_buff()
  {
    paladin_t* p = player -> cast_paladin();
    attack_t::player_buff();
    if ( p -> active_seal == SEAL_OF_TRUTH && p -> glyphs.seal_of_truth )
    {
      player_expertise += 0.10;
    }
    if ( p -> buffs_avenging_wrath -> up() ) 
    {
      player_multiplier *= 1.0 + 0.01 * p->player_data.effect_base_value(p->spells.avenging_wrath->effect_id(1));
    }
    if ( school == SCHOOL_HOLY )
    {
      if ( p -> buffs_inquisition -> up() )
      {
        player_multiplier *= 1.0 + 0.01 * p->player_data.effect_base_value(p->spells.inquisition->effect_id(1));
      }
    }
  }

  virtual void consume_and_gain_holy_power()
  {
    paladin_t* p = player -> cast_paladin();
    if ( uses_holy_power )
    {
      if ( p -> buffs_hand_of_light -> up() )
        p -> buffs_hand_of_light -> expire();
      else
        p -> buffs_holy_power -> expire();
    }
    p -> buffs_holy_power -> trigger( 1, -1, holy_power_chance );
  }
};

// Melee Attack ============================================================

struct melee_t : public paladin_attack_t
{
  melee_t( paladin_t* p ) :
    paladin_attack_t( "melee", p, SCHOOL_PHYSICAL, TREE_NONE, false )
  {
    trigger_seal      = true;
    background        = true;
    repeating         = true;
    trigger_gcd       = 0;
    base_cost         = 0;
    weapon            = &( p -> main_hand_weapon );
    base_execute_time = p -> main_hand_weapon.swing_time;
  }

  virtual double execute_time() SC_CONST
  {
    if ( ! player -> in_combat ) return 0.01;
    return paladin_attack_t::execute_time();
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    paladin_attack_t::execute();
    if ( result_is_hit() )
    {
      if ( p -> get_cooldown( "exorcism" ) -> remains() <= 0 )
        p -> buffs_the_art_of_war -> trigger();

      if ( p -> active_seal == SEAL_OF_TRUTH )
      {
        p -> active_seal_of_truth_dot -> execute();
      }

      trigger_hand_of_light( this );
    }
    if ( p -> buffs_reckoning -> up() )
    {
      p -> buffs_reckoning -> decrement();
      proc = true;
      paladin_attack_t::execute();
      proc = false;
    }
  }
};

// Auto Attack =============================================================

struct auto_attack_t : public paladin_attack_t
{
  auto_attack_t( paladin_t* p, const std::string& options_str ) :
      paladin_attack_t( "auto_attack", p )
  {
    assert( p -> main_hand_weapon.type != WEAPON_NONE );
    p -> main_hand_attack = new melee_t( p );

    trigger_gcd = 0;
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    p -> main_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    paladin_t* p = player -> cast_paladin();
    if ( p -> buffs.moving -> check() ) return false;
    return( p -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Avengers Shield =========================================================

// TODO
struct avengers_shield_t : public paladin_attack_t
{
  avengers_shield_t( paladin_t* p, const std::string& options_str ) :
      paladin_attack_t( "avengers_shield", p, SCHOOL_HOLY, TREE_PROTECTION, true, false )
  {
    id = p->spells.avengers_shield->spell_id();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 5, 1100, 1344, 0, 0.26 },
      { 75, 4,  913, 1115, 0, 0.26 },
      { 70, 3,  796,  972, 0, 0.26 },
      { 60, 2,  601,  733, 0, 0.26 },
    };
    init_rank( ranks );

    trigger_seal = false;

    may_parry = false;
    may_dodge = false;
    may_block = false;

    parse_data(p->player_data);

    direct_power_mod = 1.0;
    base_spell_power_multiplier  = 0.15;
    base_attack_power_multiplier = 0.15;

    if ( p -> glyphs.focused_shield ) base_multiplier *= 1.30;
  }
};

// Crusader Strike =========================================================

struct crusader_strike_t : public paladin_attack_t
{
  double base_cooldown;
  crusader_strike_t( paladin_t* p, const std::string& options_str ) :
      paladin_attack_t( "crusader_strike", p )
  {
    id = p->spells.crusader_strike->spell_id();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_seal = true;
    spell_haste = true;

    parse_data(p->player_data);
    if (p->primary_tree() == TREE_PROTECTION)
    {
      cooldown->duration += 0.001 * p->passives.judgements_of_the_wise->effect_base_value(2);
    }
    base_cooldown = cooldown->duration;

    base_crit       +=       0.01 * p->talents.rule_of_law->effect_base_value(1);
    assert(p->talents.crusade->effect_type(2) == E_APPLY_AURA);
    assert(p->talents.crusade->effect_subtype(2) == A_ADD_PCT_MODIFIER);
    base_multiplier *= 1.0 + 0.01 * p->talents.crusade->effect_base_value(2)
                           + 0.01 * p->talents.wrath_of_the_lightbringer->effect_base_value(1); // TODO how do they stack?

    if (p->glyphs.ascetic_crusader) base_cost *= 0.70;
    if (p->glyphs.crusader_strike)  base_crit += 0.05;
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    paladin_attack_t::execute();
    p -> buffs_holy_power -> trigger( p -> buffs_zealotry -> up() ? 3 : 1 );
  }

  virtual void update_ready()
  {
    paladin_t* p = player->cast_paladin();
    if ( p->talents.sanctity_of_battle->rank() > 0 )
    {
      cooldown->duration = base_cooldown * haste();
      if ( sim -> log ) log_t::output( sim, "%s %s cooldown is %.2f", p -> name(), name(),  cooldown->duration );
    }

    paladin_attack_t::update_ready();
  }
};

// Divine Storm ============================================================

struct divine_storm_t : public paladin_attack_t
{
  double base_cooldown;
  divine_storm_t( paladin_t* p, const std::string& options_str ) :
      paladin_attack_t( "divine_storm", p )
  {
    check_talent( p -> talents.divine_storm->rank() );

    id = p->spells.divine_storm->spell_id();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_seal = false;
    spell_haste = true;
    aoe = true;
    holy_power_chance = p->talents.divine_purpose->proc_chance();

    parse_data(p->player_data);

    base_cooldown = cooldown->duration;
  }

  virtual void update_ready()
  {
    paladin_t* p = player->cast_paladin();
    if ( p->talents.sanctity_of_battle->rank() > 0 )
    {
      cooldown->duration = base_cooldown * haste();
    }

    paladin_attack_t::update_ready();
  }
};

// Hammer of Justice =======================================================

struct hammer_of_justice_t : public paladin_attack_t
{
  hammer_of_justice_t( paladin_t* p, const std::string& options_str ) :
      paladin_attack_t( "hammer_of_justice", p, SCHOOL_HOLY )
  {
    id = p->spells.hammer_of_justice->spell_id();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    parse_data(p->player_data);

    cooldown -> duration += 0.001 * p->talents.improved_hammer_of_justice->effect_base_value(1);
  }

  virtual bool ready()
  {
    if ( ! sim -> target -> debuffs.casting -> check() ) return false;
    return paladin_attack_t::ready();
  }
};

// Hammer of the Righteous =================================================

// TODO: holy aoe
struct hammer_of_the_righteous_t : public paladin_attack_t
{
  hammer_of_the_righteous_t( paladin_t* p, const std::string& options_str ) :
      paladin_attack_t( "hammer_of_the_righteous", p )
  {
    check_talent( p -> talents.hammer_of_the_righteous->rank() );

    id = p->spells.hammer_of_the_righteous->spell_id();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0.30;

    parse_data(p->player_data);

    base_multiplier *= 1.0 + 0.01 * p->talents.crusade->effect_base_value(2);

    if ( p -> glyphs.hammer_of_the_righteous ) base_multiplier *= 1.10;
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    paladin_attack_t::execute();
    if ( result_is_hit() )
    {
      // TODO: Is this still happening?
      if ( p -> active_seal == SEAL_OF_TRUTH )
      {
        p -> active_seal_of_truth_dot -> execute();
      }
    }
  }
};

// Hammer of Wrath =========================================================

struct hammer_of_wrath_t : public paladin_attack_t
{
  double base_cooldown;

  hammer_of_wrath_t( paladin_t* p, const std::string& options_str ) :
      paladin_attack_t( "hammer_of_wrath", p, SCHOOL_HOLY, TREE_RETRIBUTION, true, false )
  {
    id = p->spells.hammer_of_wrath->spell_id();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    parse_data(p->player_data);
    
    weapon            = &( p -> main_hand_weapon );
    weapon_multiplier = 0.0;

    may_parry = false;
    may_dodge = false;
    may_block = false;

    holy_power_chance = p->talents.divine_purpose->proc_chance();

    base_cooldown = cooldown->duration;

    // base_crit += 0.01 * p->talents.wrath_of_the_lightbringer->effect_base_value(2)
    //            + 0.01 * p->talents.sanctified_wrath->effect_base_value(2);
    parse_effect_data(p->player_data, p->talents.wrath_of_the_lightbringer->spell_id(), 2);
    parse_effect_data(p->player_data, p->talents.sanctified_wrath->spell_id(), 2);

    // The coefficient in the data file seems to be the SP modifier, the AP modifier is ~39% (determined experimentally)
    base_spell_power_multiplier = direct_power_mod;
    direct_power_mod = 1.0;
    base_attack_power_multiplier = 0.39;

    if ( p -> glyphs.hammer_of_wrath ) base_cost = 0;
  }

  virtual bool ready()
  {
    paladin_t* p = player -> cast_paladin();
    if ( sim -> target -> health_percentage() > 20 && !( p -> talents.sanctified_wrath->rank() && p -> buffs_avenging_wrath -> up() ) )
      return false;

    return paladin_attack_t::ready();
  }
};

// Shield of Righteousness =================================================

// TODO
struct shield_of_the_righteous_t : public paladin_attack_t
{
  shield_of_the_righteous_t( paladin_t* p, const std::string& options_str ) :
      paladin_attack_t( "shield_of_the_righteous", p, SCHOOL_HOLY )
  {
    check_talent( p->talents.shield_of_the_righteous->rank() );

    id = p->spells.shield_of_the_righteous->spell_id();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    parse_data(p->player_data);

    base_dd_min = base_dd_max = 1; // ugh

    may_parry = false;
    may_dodge = false;
    may_block = false;

    uses_holy_power = true;

    if ( p -> glyphs.shield_of_the_righteous ) base_multiplier *= 1.10;
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    direct_power_mod = util_t::talent_rank( p -> holy_power_stacks(), 3, 0.20, 0.60, 1.20 );
    paladin_attack_t::execute();
    if ( p -> talents.holy_shield )
      p -> buffs_holy_shield -> trigger();
  }

  virtual bool ready()
  {
    paladin_t* p = player -> cast_paladin();
    if ( p -> main_hand_weapon.group() == WEAPON_2H ) return false;
    if ( ! p -> has_holy_power() ) return false;
    return paladin_attack_t::ready();
  }
};

// Templar's Verdict ========================================================

struct templars_verdict_t : public paladin_attack_t
{
  templars_verdict_t( paladin_t* p, const std::string& options_str ) :
      paladin_attack_t( "templars_verdict", p )
  {
    assert( p -> primary_tree() == TREE_RETRIBUTION );

    id = p->spells.templars_verdict->spell_id();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    parse_data(p->player_data);

    trigger_seal = true;
    uses_holy_power = true;
    holy_power_chance = p->talents.divine_purpose->proc_chance();

    base_crit       +=     0.01 * p->talents.arbiter_of_the_light->effect_base_value(1);
    base_multiplier *= 1 + 0.01 * p->talents.crusade->effect_base_value(2)
                         + (p->glyphs.templars_verdict      ? 0.15 : 0.0)
                         + (p->set_bonus.tier11_2pc_melee() ? 0.10 : 0.0);
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    weapon_multiplier = util_t::talent_rank( p -> holy_power_stacks(), 3, 0.30, 0.90, 2.35 );
    paladin_attack_t::execute();
  }

  virtual bool ready()
  {
    paladin_t* p = player -> cast_paladin();
    if ( ! p -> has_holy_power() ) return false;
    return paladin_attack_t::ready();
  }
};

// Seals of Command proc ====================================================

struct seals_of_command_proc_t : public paladin_attack_t
{
  seals_of_command_proc_t( paladin_t* p ) :
    paladin_attack_t( "seals_of_command", p, SCHOOL_HOLY )
  {
    background  = true;
    proc        = true;
    trigger_gcd = 0;

    weapon            = &( p -> main_hand_weapon );
    weapon_multiplier = 0.01 * p->talents.seals_of_command->effect_base_value(1);
  }
};

// Ancient Fury =============================================================

struct ancient_fury_t : public paladin_attack_t
{
  ancient_fury_t( paladin_t* p ) :
    paladin_attack_t( "ancient_fury", p, SCHOOL_HOLY, TREE_RETRIBUTION, true, false )
  {
    // TODO meteor stuff
    id = p->spells.ancient_fury->spell_id();
    parse_data(p->player_data);
    background = true;
  }

  virtual void execute()
  {
    paladin_attack_t::execute();
    player->cast_paladin()->buffs_ancient_power->expire();
  }

  virtual void player_buff()
  {
    paladin_attack_t::player_buff();
    player_multiplier *= player->cast_paladin()->buffs_ancient_power->stack();
  }
};

// Paladin Seals ============================================================

struct paladin_seal_t : public paladin_attack_t
{
  int seal_type;

  paladin_seal_t( paladin_t* p, const char* n, int st, const std::string& options_str ) :
    paladin_attack_t( n, p ), seal_type( st )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    harmful    = false;
    base_cost  = p -> resource_base[ RESOURCE_MANA ] * 0.14;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    paladin_t* p = player -> cast_paladin();
    p -> active_seal = seal_type;
  }

  virtual bool ready()
  {
    paladin_t* p = player -> cast_paladin();
    if ( p -> active_seal == seal_type ) return false;
    return paladin_attack_t::ready();
  }

  virtual double cost() SC_CONST 
  { 
    return player -> in_combat ? paladin_attack_t::cost() : 0;
  }
};

// Seal of Justice ==========================================================

struct seal_of_justice_proc_t : public paladin_attack_t
{
  seal_of_justice_proc_t( paladin_t* p ) :
    paladin_attack_t( "seal_of_justice", p, SCHOOL_HOLY )
  {
    background  = true;
    proc        = true;
    trigger_gcd = 0;

    weapon            = &( p -> main_hand_weapon );
    weapon_multiplier = 0.0;
  }
  virtual void execute() 
  {
    // No need to model stun
  }
};

struct seal_of_justice_judgement_t : public paladin_attack_t
{
  seal_of_justice_judgement_t( paladin_t* p ) :
    paladin_attack_t( "judgement_of_justice", p, SCHOOL_HOLY )
  {
    background = true;

    may_parry = false;
    may_dodge = false;
    may_block = false;

    base_cost  = p -> resource_base[ RESOURCE_MANA ] * 0.05;

    base_crit       +=       0.01 * p->talents.arbiter_of_the_light->effect_base_value(1);
    base_multiplier *= 1.0 + 0.01 * p->talents.wrath_of_the_lightbringer->effect_base_value(1)
                           + 0.10 * p->set_bonus.tier10_4pc_melee();

    base_dd_min = base_dd_max = 1;
    direct_power_mod = 1.0;
    base_spell_power_multiplier = 0.25;
    base_attack_power_multiplier = 0.16;

    weapon            = &( p -> main_hand_weapon );
    weapon_multiplier = 0.0;
	  
    cooldown -> duration = 8;

    if ( p -> glyphs.judgement ) base_multiplier *= 1.10;
  }
};

// Seal of Insight ==========================================================

struct seal_of_insight_proc_t : public paladin_attack_t
{
  seal_of_insight_proc_t( paladin_t* p ) :
    paladin_attack_t( "seal_of_insight", p, SCHOOL_HOLY )
  {
    background  = true;
    proc        = true;
    trigger_gcd = 0;

    base_multiplier *= 1.0 + 0.10 * p -> set_bonus.tier10_4pc_melee();

    base_spell_power_multiplier = 0.15;
    base_attack_power_multiplier = 0.15;

    weapon            = &( p -> main_hand_weapon );
    weapon_multiplier = 0.0;
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    p -> resource_gain( RESOURCE_HEALTH, total_power() );
    p -> resource_gain( RESOURCE_MANA, p -> resource_max[ RESOURCE_MANA ] * 0.04, p -> gains_seal_of_insight );
  }
};

struct seal_of_insight_judgement_t : public paladin_attack_t
{
  seal_of_insight_judgement_t( paladin_t* p ) :
    paladin_attack_t( "judgement_of_insight", p, SCHOOL_HOLY )
  {
    background = true;

    may_parry = false;
    may_dodge = false;
    may_block = false;

    base_cost  = 0.05 * p -> resource_base[ RESOURCE_MANA ];

    base_crit       +=       0.01 * p->talents.arbiter_of_the_light->effect_base_value(1);
    base_multiplier *= 1.0 + 0.10 * p->set_bonus.tier10_4pc_melee()
                           + 0.01 * p->talents.wrath_of_the_lightbringer->effect_base_value(1);

    base_dd_min = base_dd_max = 1;
    direct_power_mod = 1.0;
    base_spell_power_multiplier = 0.25;
    base_attack_power_multiplier = base_spell_power_multiplier / 1.57;

    weapon            = &( p -> main_hand_weapon );
    weapon_multiplier = 0.0;
	  
    cooldown -> duration = 8;

    if ( p -> glyphs.judgement ) base_multiplier *= 1.10;
  }
};

// Seal of Righteousness ====================================================

// TODO: seals of command on multi targets
struct seal_of_righteousness_proc_t : public paladin_attack_t
{
  seal_of_righteousness_proc_t( paladin_t* p ) :
    paladin_attack_t( "seal_of_righteousness", p, SCHOOL_HOLY )
  {
    background  = true;
    proc        = true;
    trigger_gcd = 0;

    base_multiplier *= p -> main_hand_weapon.swing_time; // Note that tooltip changes with haste, but actual damage doesn't
    base_multiplier *= 1.0 + ( 0.01 * p->talents.seals_of_the_pure->effect_base_value(1) + 
                               0.10 * p->set_bonus.tier10_4pc_melee() );

    direct_power_mod = 1.0;
    base_attack_power_multiplier = 0.011;
    base_spell_power_multiplier = 2 * base_attack_power_multiplier;

    weapon            = &( p -> main_hand_weapon );
    weapon_multiplier = 0.0;
  }
};

struct seal_of_righteousness_judgement_t : public paladin_attack_t
{
  seal_of_righteousness_judgement_t( paladin_t* p ) :
    paladin_attack_t( "judgement_of_righteousness", p, SCHOOL_HOLY )
  {
    background = true;

    may_parry = false;
    may_dodge = false;
    may_block = false;
    may_crit  = false;

    base_cost  = 0.05 * p -> resource_base[ RESOURCE_MANA ];

    base_crit       +=       0.01 * p->talents.arbiter_of_the_light->effect_base_value(1);
    base_multiplier *= 1.0 + 0.10 * p->set_bonus.tier10_4pc_melee()
                           + 0.01 * p->talents.wrath_of_the_lightbringer->effect_base_value(1);

    base_dd_min = base_dd_max = 1;
    direct_power_mod = 1.0;
    base_spell_power_multiplier = 0.32;
    base_attack_power_multiplier = base_spell_power_multiplier / 1.57;

    weapon            = &( p -> main_hand_weapon );
    weapon_multiplier = 0.0;

    cooldown -> duration = 8;

    if ( p -> glyphs.judgement ) base_multiplier *= 1.10;
  }
};

// Seal of Truth ============================================================

struct seal_of_truth_dot_t : public paladin_attack_t
{
  seal_of_truth_dot_t( paladin_t* p ) :
    paladin_attack_t( "censure", p, SCHOOL_HOLY, TREE_RETRIBUTION, true, false/*TODO: check 2hspec*/ )
  {
    background  = true;
    proc        = true;
    trigger_gcd = 0;
    base_cost   = 0;

    weapon            = &( p -> main_hand_weapon );
    weapon_multiplier = 0.0;

    base_td = 1.0;
    num_ticks = 5;
    base_tick_time = 3;
    scale_with_haste = true;
    spell_haste = true;
    tick_may_crit = true;
    
    tick_power_mod = 1.0;
    base_spell_power_multiplier  = 0.05*0.2;   // Determined experimentally by Redcape
    base_attack_power_multiplier = 0.0966*0.2; // Determined experimentally by Redcape

    // For some reason, SotP is multiplicative with 4T10 for the procs but additive for the DoT
    base_multiplier *= 1.0 + ( 0.01 * p->talents.seals_of_the_pure->effect_base_value(1) +
                               0.01 * p->talents.inquiry_of_faith->effect_base_value(1) +
                               0.10 * p->set_bonus.tier10_4pc_melee() );
  }

  virtual void player_buff()
  {
    paladin_t* p = player -> cast_paladin();
    paladin_attack_t::player_buff();
    player_multiplier *= p -> buffs_censure -> stack();
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    player_buff();
    target_debuff( DMG_DIRECT );
    calculate_result();
    if ( result_is_hit() )
    {
      p -> buffs_censure -> trigger();
      player_buff(); // update with new stack of the debuff

      if ( ticking )
      {
        // Even though an attack has been executed, it counts as a refresh
        refresh_duration();
      }
      else
      {
        // First application, take a snapshot of the haste and schedule the tick
        snapshot_haste = haste();
        number_ticks = hasted_num_ticks();
        schedule_tick();
      }
    }
  }

  virtual void last_tick()
  {
    paladin_t* p = player -> cast_paladin();
    paladin_attack_t::last_tick();
    p -> buffs_censure -> expire();
  }
};

struct seal_of_truth_proc_t : public paladin_attack_t
{
  seal_of_truth_proc_t( paladin_t* p ) :
    paladin_attack_t( "seal_of_truth", p, SCHOOL_HOLY )
  {
    id = 42463;
    background  = true;
    proc        = true;
    may_miss    = false;
    may_dodge   = false;
    may_parry   = false;
    trigger_gcd = 0;

    parse_data(p->player_data);

    // For some reason, SotP is multiplicative with 4T10 for the procs but additive for the DoT
    base_multiplier *= ( 1.0 + 0.01 * p -> talents.seals_of_the_pure->effect_base_value(1) )
                    *  ( 1.0 + 0.10 * p -> set_bonus.tier10_4pc_melee() );
  }
  virtual void player_buff()
  {
    paladin_t* p = player -> cast_paladin();
    paladin_attack_t::player_buff();
    player_multiplier *= p -> buffs_censure -> stack() * 0.2;
  }
  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    paladin_attack_t::execute();
    if ( result_is_hit() )
    {
      // The seal of truth proc attack triggers seals of command at the moment, unclear if this is intended
      if ( p -> talents.seals_of_command->rank() )
      {
        p -> active_seals_of_command_proc -> execute();
      }
    }
  }
};

struct seal_of_truth_judgement_t : public paladin_attack_t
{
  seal_of_truth_judgement_t( paladin_t* p ) :
    paladin_attack_t( "judgement_of_truth", p, SCHOOL_HOLY )
  {
    background = true;

    may_parry = false;
    may_dodge = false;
    may_block = false;

    base_cost  = p -> resource_base[ RESOURCE_MANA ] * 0.05;

    base_crit       +=       0.01 * p->talents.arbiter_of_the_light->effect_base_value(1);
    base_multiplier *= 1.0 + 0.10 * p->set_bonus.tier10_4pc_melee()
                           + 0.01 * p->talents.wrath_of_the_lightbringer->effect_base_value(1);

    weapon            = &( p -> main_hand_weapon );
    weapon_multiplier = 0.0;
	  
    direct_power_mod = 1.0;
    base_spell_power_multiplier = 0.223;
    base_attack_power_multiplier = base_spell_power_multiplier / 1.57;

    cooldown -> duration = 8;

    if ( p -> glyphs.judgement ) base_multiplier *= 1.10;
  }

  virtual void player_buff()
  {
    paladin_t* p = player -> cast_paladin();
    paladin_attack_t::player_buff();
    player_multiplier *= 1.0 + p -> buffs_censure -> stack() * 0.10;
  }
};

// Judgement ================================================================

struct judgement_t : public paladin_attack_t
{
  action_t* seal_of_justice;
  action_t* seal_of_insight;
  action_t* seal_of_righteousness;
  action_t* seal_of_truth;

  judgement_t( paladin_t* p, const std::string& options_str ) :
      paladin_attack_t( "judgement", p, SCHOOL_HOLY )
  {
    id = p->spells.judgement->spell_id();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    holy_power_chance = p->talents.divine_purpose->proc_chance();

    seal_of_justice       = new seal_of_justice_judgement_t      ( p );
    seal_of_insight       = new seal_of_insight_judgement_t      ( p );
    seal_of_righteousness = new seal_of_righteousness_judgement_t( p );
    seal_of_truth         = new seal_of_truth_judgement_t        ( p );
  }

  action_t* active_seal() SC_CONST
  {
    paladin_t* p = player -> cast_paladin();
    switch ( p -> active_seal )
    {
    case SEAL_OF_JUSTICE:       return seal_of_justice;
    case SEAL_OF_INSIGHT:       return seal_of_insight;
    case SEAL_OF_RIGHTEOUSNESS: return seal_of_righteousness;
    case SEAL_OF_TRUTH:         return seal_of_truth;
    }
    return 0;
  }

  virtual double cost() SC_CONST 
  { 
    action_t* seal = active_seal(); 
    if ( ! seal ) return 0.0;
    return seal -> cost(); 
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    target_t* t = sim -> target;
    action_t* seal = active_seal();

    if ( ! seal )
      return;

    seal -> execute();

    if ( seal -> result_is_hit() )
    {
      consume_and_gain_holy_power();

      if ( p -> talents.judgements_of_the_pure->rank() ) 
      {
        p -> buffs_judgements_of_the_pure -> trigger();
      }
      if ( p -> talents.judgements_of_the_just ) 
      {
        t -> debuffs.judgements_of_the_just -> trigger();
      }
    }
    trigger_judgements_of_the_wise( seal );
    trigger_judgements_of_the_bold( seal );
    if ( p -> talents.communion->rank() ) p -> trigger_replenishment();
    
    p -> last_foreground_action = seal; // Necessary for DPET calculations.
  }

  virtual bool ready()
  {
    action_t* seal = active_seal();
    if( ! seal ) return false;
    if( ! seal -> ready() ) return false;
    return paladin_attack_t::ready();
  }
};

// =========================================================================
// Paladin Spells
// =========================================================================

struct paladin_spell_t : public spell_t
{
  bool uses_holy_power;
  double holy_power_chance;

  paladin_spell_t( const char* n, paladin_t* p, const school_type s=SCHOOL_HOLY, int t=TREE_NONE ) :
      spell_t( n, p, RESOURCE_MANA, s, t ), uses_holy_power(false), holy_power_chance(0.0)
  {
    base_multiplier *= 1.0 + 0.01 * p->talents.communion->effect_base_value(3);

    if (p->set_bonus.tier10_2pc_melee())
      base_multiplier *= 1.05;
  }

  virtual double haste() SC_CONST
  {
    paladin_t* p = player -> cast_paladin();
    double h = spell_t::haste();
    if ( p -> buffs_judgements_of_the_pure -> up() )
    {
      h *= 1.0 / ( 1.0 + p -> talents.judgements_of_the_pure->rank() * 0.03 );
    }
    return h;
  }

  virtual void player_buff()
  {
    paladin_t* p = player -> cast_paladin();

    spell_t::player_buff();

    if ( p -> buffs_avenging_wrath -> up() ) 
    {
      player_multiplier *= 1.0 + 0.01 * p->player_data.effect_base_value(p->spells.avenging_wrath->effect_id(1));
    }
    if ( school == SCHOOL_HOLY )
    {
      if ( p -> buffs_inquisition -> up() )
      {
        player_multiplier *= 1.30;
      }
    }
  }

  virtual void execute()
  {
    spell_t::execute();
    if ( result_is_hit() )
    {
      consume_and_gain_holy_power();
    }
  }

  virtual void consume_and_gain_holy_power()
  {
    paladin_t* p = player -> cast_paladin();
    if ( uses_holy_power )
    {
      if ( p -> buffs_hand_of_light -> up() )
        p -> buffs_hand_of_light -> expire();
      else
        p -> buffs_holy_power -> expire();
    }
    p -> buffs_holy_power -> trigger( 1, -1, holy_power_chance );
  }
};

// Avenging Wrath ==========================================================

struct avenging_wrath_t : public paladin_spell_t
{
  avenging_wrath_t( paladin_t* p, const std::string& options_str ) :
      paladin_spell_t( "avenging_wrath", p )
  {
    id = p->spells.avenging_wrath->spell_id();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    parse_data(p->player_data);
    harmful = false;
    parse_effect_data(p->player_data, p->talents.sanctified_wrath->spell_id(), 1);
    //cooldown -> duration += 0.001 * p -> talents.sanctified_wrath->effect_base_value(1);
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    consume_resource();
    update_ready();
    p -> buffs_avenging_wrath -> trigger();
  }
};

// Consecration ============================================================

struct consecration_tick_t : public paladin_spell_t
{
  consecration_tick_t( paladin_t* p ) :
      paladin_spell_t( "consecration", p )
  {
    id = p->spells.consecration_tick->spell_id();
    aoe        = true;
    dual       = true;
    background = true;
    may_crit   = true;
    may_miss   = true;

    parse_data(p->player_data);

    base_spell_power_multiplier = base_attack_power_multiplier = 1.0;

    base_multiplier *= 1.0 + 0.01 * p->talents.hallowed_ground->effect_base_value(1);
  }

  virtual void execute()
  {
    paladin_spell_t::execute();
    if ( result_is_hit() )
    {
      tick_dmg = direct_dmg;     
    }
    update_stats( DMG_OVER_TIME );
  }
};

struct consecration_t : public paladin_spell_t
{
  action_t* consecration_tick;

  consecration_t( paladin_t* p, const std::string& options_str ) :
      paladin_spell_t( "consecration", p )
  {
    id = p->spells.consecration->spell_id();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    parse_data(p->player_data);

    may_miss       = false;
    num_ticks      = 10;
    base_tick_time = 1;
    
    //base_cost *= 1.0 + 0.01 * p->talents.hallowed_ground->effect_base_value(2);
    parse_effect_data(p->player_data, p->talents.hallowed_ground->spell_id(), 2);

    if ( p -> glyphs.consecration )
    {
      num_ticks += 2;
      cooldown -> duration *= 1.2;
    }

    consecration_tick = new consecration_tick_t( p );
  }

  // Consecration ticks are modeled as "direct" damage, requiring a dual-spell setup.

  virtual void tick()
  {
    if ( sim -> debug ) log_t::output( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );
    consecration_tick -> execute();
    update_time( DMG_OVER_TIME );
  }
};

// Divine Favor ============================================================

struct divine_favor_t : public paladin_spell_t
{
  divine_favor_t( paladin_t* p, const std::string& options_str ) :
      paladin_spell_t( "divine_favor", p )
  {
    check_talent( p -> talents.divine_favor->rank() );

    id = p->spells.divine_favor->spell_id();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    harmful = false;

    parse_data(p->player_data);
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    update_ready();
    p -> buffs_divine_favor -> trigger();
  }
};


// Divine Plea =============================================================

// TODO: shield of the templar generating hopow
struct divine_plea_t : public paladin_spell_t
{
  divine_plea_t( paladin_t* p, const std::string& options_str ) :
      paladin_spell_t( "divine_plea", p )
  {
    id = p->spells.divine_plea->spell_id();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    harmful = false;
    parse_data(p->player_data);
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    update_ready();
    p -> buffs_divine_plea -> trigger();
  }

  virtual bool ready()
  {
    paladin_t* p = player -> cast_paladin();
    if ( p -> buffs_divine_plea -> check() ) return false;
    return paladin_spell_t::ready();
  }
};

// Exorcism ================================================================

struct exorcism_t : public paladin_spell_t
{
  int art_of_war;
  int undead_demon;
  double saved_multiplier;
  double blazing_light_multiplier;

  exorcism_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "exorcism", p ), art_of_war(0), undead_demon(0)
  {
    id        = p -> spells.exorcism->spell_id();
    option_t options[] =
    {
      { "art_of_war",   OPT_BOOL, &art_of_war   },
      { "undead_demon", OPT_BOOL, &undead_demon },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    parse_data(p->player_data);

    weapon            = &p->main_hand_weapon;
    weapon_multiplier = 0.0;

    blazing_light_multiplier = 1.0 + 0.01 * p->talents.blazing_light->effect_base_value(1);

    holy_power_chance = p->talents.divine_purpose->proc_chance();
	  
    may_crit = true;
    tick_may_crit = true;
    dot_behavior = DOT_REFRESH;
    scale_with_haste = false;

    direct_power_mod = 1.0;
    tick_power_mod = 0.2/3; // glyph of exorcism is 20% of damage over three ticks ... FIXME: or just base damage?
    base_spell_power_multiplier = 0.344;
    base_attack_power_multiplier = 0.344;
    if ( ! p -> glyphs.exorcism )
      num_ticks = 0;
  }
  
  virtual double cost() SC_CONST
  {
    paladin_t* p = player->cast_paladin();
    if ( p->buffs_the_art_of_war->up() ) return 0.0;
    return paladin_spell_t::cost();
  }

  virtual double execute_time() SC_CONST
  {
    paladin_t* p = player->cast_paladin();
    if ( p -> buffs_the_art_of_war->up() ) return 0.0;
    return paladin_spell_t::execute_time();
  }

  virtual void player_buff()
  {
    paladin_spell_t::player_buff();
    paladin_t* p = player->cast_paladin();
    saved_multiplier = player_multiplier;
    // blazing light should really be a base_multiplier, but since the DoT doesn't benefit from it
    // and art of war and blazing light stack additively, we're doing it here instead. Hooray.
    if ( p->buffs_the_art_of_war->up() )
    {
      player_multiplier *= (blazing_light_multiplier + 1);
    }
    else
    {
      player_multiplier *= blazing_light_multiplier;
    }
    int target_race = sim -> target -> race;
    if ( target_race == RACE_UNDEAD ||
         target_race == RACE_DEMON  )
    {
      player_crit += 1.0;
    }
  }

  virtual double total_power() SC_CONST
  {
    return (std::max)(total_spell_power(), total_attack_power());
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    paladin_spell_t::execute();
    p -> buffs_the_art_of_war -> expire();
  }

  virtual bool ready()
  {
    paladin_t* p = player -> cast_paladin();

    if ( art_of_war )
      if ( ! p -> buffs_the_art_of_war -> check() )
        return false;

    if ( undead_demon )
    {
      int target_race = sim -> target -> race;
      if ( target_race != RACE_UNDEAD &&
           target_race != RACE_DEMON  )
        return false;
    }

    return paladin_spell_t::ready();
  }

  virtual void tick()
  {
    // Since the glyph DoT doesn't benefit from the AoW and blazing light multipliers,
    // we save it in player_buff() and restore it here. Rather hackish.
    player_multiplier = saved_multiplier;
    paladin_spell_t::tick();
  }
};

// Holy Shock ==============================================================

// TODO: fix the fugly hack
struct holy_shock_t : public paladin_spell_t
{
  holy_shock_t( paladin_t* p, const std::string& options_str ) :
      paladin_spell_t( "holy_shock", p )
  {
    id = p->spells.holy_shock->spell_id();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    parse_data(p->player_data);
    // hack! spell 20473 has the cooldown/cost/etc stuff, but the actual spell cast
    // to do damage is 25912
    parse_effect_data(p->player_data, 25912, 1);

    base_multiplier *= 1.0 + 0.01 * p->talents.blazing_light->effect_base_value(1)
                           + 0.01 * p->talents.crusade->effect_base_value(2); // TODO how do they stack?
    base_crit       +=       0.01 * p->talents.rule_of_law->effect_base_value(1);

    if ( p -> glyphs.holy_shock ) base_crit += 0.05;
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    paladin_spell_t::execute();
    if (result_is_hit())
    {
      p -> buffs_holy_power -> trigger();
    }
  }
};

// Holy Wrath ==============================================================

struct holy_wrath_t : public paladin_spell_t
{
  holy_wrath_t( paladin_t* p, const std::string& options_str ) :
      paladin_spell_t( "holy_wrath", p )
  {
    id = p->spells.holy_wrath->spell_id();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    parse_data(p->player_data);

    // aoe = true; FIXME disabled until we have meteor support
    may_crit = true;

    direct_power_mod = 0.61;

    holy_power_chance = p->talents.divine_purpose->proc_chance();

    base_crit += 0.01 * p->talents.wrath_of_the_lightbringer->effect_base_value(2);
  }
};

struct inquisition_t : public paladin_spell_t
{
  double base_duration;
  inquisition_t( paladin_t* p, const std::string& options_str ) :
      paladin_spell_t( "inquisition", p )
  {
    assert(p->spells.inquisition->ok());
    id = p->spells.inquisition->spell_id();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    uses_holy_power = true;
    harmful = false;
    holy_power_chance = p->talents.divine_purpose->proc_chance();
    parse_data(p->player_data);
    base_duration = p->player_data.spell_duration(id) * (1.0 + 0.01 * p->talents.inquiry_of_faith->effect_base_value(2));
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    update_ready();
    p -> buffs_inquisition -> buff_duration = base_duration * p -> holy_power_stacks();
    if (p->set_bonus.tier11_4pc_melee())
      p -> buffs_inquisition -> buff_duration += base_duration;
    p -> buffs_inquisition -> trigger();
    if ( p -> talents.holy_shield )
      p -> buffs_holy_shield -> trigger();
    consume_and_gain_holy_power();
  }

  virtual bool ready()
  {
    paladin_t* p = player -> cast_paladin();
    if( ! p -> has_holy_power() )
      return false;
    return paladin_spell_t::ready();
  }
};

struct zealotry_t : public paladin_spell_t
{
  zealotry_t( paladin_t* p, const std::string& options_str ) :
      paladin_spell_t( "zealotry", p )
  {
    check_talent( p -> talents.zealotry->rank() );

    id = p->spells.zealotry->spell_id();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    uses_holy_power = false;
    harmful = false;
    parse_data(p->player_data);
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    update_ready();
    p -> buffs_zealotry -> trigger();
  }

  virtual bool ready()
  {
    paladin_t* p = player -> cast_paladin();
    if( ! p -> has_holy_power(3) )
      return false;
    return paladin_spell_t::ready();
  }
};

struct guardian_of_ancient_kings_t : public paladin_spell_t
{
  guardian_of_ancient_kings_t( paladin_t* p, const std::string& options_str ) :
      paladin_spell_t( "guardian_of_ancient_kings", p )
  {
    assert(p->primary_tree() == TREE_RETRIBUTION);
    id = p->spells.guardian_of_ancient_kings->spell_id();
    option_t options[] = 
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    parse_data(p->player_data);
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    update_ready();
    p->summon_pet("guardian_of_ancient_kings", p->spells.guardian_of_ancient_kings_ret->duration());
  }
};

} // ANONYMOUS NAMESPACE ===================================================

// ==========================================================================
// Paladin Character Definition
// ==========================================================================

// paladin_t::create_action ==================================================

action_t* paladin_t::create_action( const std::string& name, const std::string& options_str )
{
  if ( name == "auto_attack"             ) return new auto_attack_t            ( this, options_str );
  if ( name == "avengers_shield"         ) return new avengers_shield_t        ( this, options_str );
  if ( name == "avenging_wrath"          ) return new avenging_wrath_t         ( this, options_str );
  if ( name == "consecration"            ) return new consecration_t           ( this, options_str );
  if ( name == "crusader_strike"         ) return new crusader_strike_t        ( this, options_str );
  if ( name == "divine_favor"            ) return new divine_favor_t           ( this, options_str );
  if ( name == "divine_plea"             ) return new divine_plea_t            ( this, options_str );
  if ( name == "divine_storm"            ) return new divine_storm_t           ( this, options_str );
  if ( name == "exorcism"                ) return new exorcism_t               ( this, options_str );
  if ( name == "hammer_of_justice"       ) return new hammer_of_justice_t      ( this, options_str );
  if ( name == "hammer_of_wrath"         ) return new hammer_of_wrath_t        ( this, options_str );
  if ( name == "hammer_of_the_righteous" ) return new hammer_of_the_righteous_t( this, options_str );
  if ( name == "holy_shock"              ) return new holy_shock_t             ( this, options_str );
  if ( name == "holy_wrath"              ) return new holy_wrath_t             ( this, options_str );
  if ( name == "judgement"               ) return new judgement_t              ( this, options_str );
  if ( name == "shield_of_the_righteous" ) return new shield_of_the_righteous_t( this, options_str );

  if ( name == "seal_of_justice"         ) return new paladin_seal_t( this, "seal_of_justice",       SEAL_OF_JUSTICE,       options_str );
  if ( name == "seal_of_insight"         ) return new paladin_seal_t( this, "seal_of_insight",       SEAL_OF_INSIGHT,       options_str );
  if ( name == "seal_of_righteousness"   ) return new paladin_seal_t( this, "seal_of_righteousness", SEAL_OF_RIGHTEOUSNESS, options_str );
  if ( name == "seal_of_truth"           ) return new paladin_seal_t( this, "seal_of_truth",         SEAL_OF_TRUTH,         options_str );

  if ( name == "inquisition"             ) return new inquisition_t( this, options_str );
  if ( name == "zealotry"                ) return new zealotry_t( this, options_str );
  if ( name == "templars_verdict"        ) return new templars_verdict_t( this, options_str );

  if ( name == "guardian_of_ancient_kings" ) return new guardian_of_ancient_kings_t( this, options_str );

//if ( name == "aura_mastery"            ) return new aura_mastery_t           ( this, options_str );
//if ( name == "blessings"               ) return new blessings_t              ( this, options_str );
//if ( name == "concentration_aura"      ) return new concentration_aura_t     ( this, options_str );
//if ( name == "devotion_aura"           ) return new devotion_aura_t          ( this, options_str );
//if ( name == "retribution_aura"        ) return new retribution_aura_t       ( this, options_str );

  return player_t::create_action( name, options_str );
}

// paladin_t::init_race ======================================================

void paladin_t::init_race()
{
  race = util_t::parse_race_type( race_str );
  switch ( race )
  {
  case RACE_HUMAN:
  case RACE_DWARF:
  case RACE_DRAENEI:
  case RACE_BLOOD_ELF:
  case RACE_TAUREN:
    break;
  default:
    race = RACE_DWARF;
    race_str = util_t::race_type_string( race );
  }

  player_t::init_race();
}

// paladin_t::init_base =====================================================

void paladin_t::init_base()
{
  player_t::init_base();

  initial_attack_power_per_strength = 2.0;
  initial_attack_power_per_agility  = 0.0;

  initial_spell_power_per_intellect = 1;
  
  base_spell_power  = 0;
  base_attack_power = ( level * 3 ) - 20;

  // FIXME! Level-specific!
  base_miss    = 0.05;
  base_parry   = 0.05;
  base_block   = 0.05;
  initial_armor_multiplier *= 1.0 + 0.1 * talents.toughness / 3.0;

  diminished_kfactor    = 0.009560;
  diminished_dodge_capi = 0.01523660;
  diminished_parry_capi = 0.01523660;

  health_per_stamina = 10;
  mana_per_intellect = 15;

  switch ( primary_tree() )
  {
  case TREE_HOLY:
    base_attack_hit += 0; // TODO spirit -> hit talents.enlightened_judgements
    base_spell_hit  += 0; // TODO spirit -> hit talents.enlightened_judgements
    break;

  case TREE_PROTECTION:
    tank = 1;
    attribute_multiplier_initial[ ATTR_STAMINA   ] *= 1.0 + 0.01 * passives.touched_by_the_light->effect_base_value(3);
    base_spell_hit += 0.01 * passives.judgements_of_the_wise->effect_base_value(3);
    break;

  case TREE_RETRIBUTION:
    base_spell_hit += 0.01 * passives.sheath_of_light->effect_base_value(3);
    break;
  default: break;
  }

  ancient_fury_explosion = new ancient_fury_t(this);
}

// paladin_t::reset =========================================================

void paladin_t::reset()
{
  player_t::reset();

  active_seal = SEAL_NONE;
}

// paladin_t::interrupt =====================================================

void paladin_t::interrupt()
{
  player_t::interrupt();

  if ( main_hand_attack ) main_hand_attack -> cancel();
}

// paladin_t::init_gains ====================================================

void paladin_t::init_gains()
{
  player_t::init_gains();

  gains_divine_plea            = get_gain( "divine_plea"            );
  gains_judgements_of_the_wise = get_gain( "judgements_of_the_wise" );
  gains_judgements_of_the_bold = get_gain( "judgements_of_the_bold" );
  gains_seal_of_command_glyph  = get_gain( "seal_of_command_glyph"  );
  gains_seal_of_insight        = get_gain( "seal_of_insight"        );
}

// paladin_t::init_procs ====================================================

void paladin_t::init_procs()
{
  player_t::init_procs();

  procs_parry_haste = get_proc( "parry_haste" );
}

// paladin_t::init_rng ======================================================

void paladin_t::init_rng()
{
  player_t::init_rng();
}

// paladin_t::init_glyphs ===================================================

void paladin_t::init_glyphs()
{
  memset( ( void* ) &glyphs, 0x0, sizeof( glyphs_t ) );

  std::vector<std::string> glyph_names;
  int num_glyphs = util_t::string_split( glyph_names, glyphs_str, ",/" );

  for ( int i=0; i < num_glyphs; i++ )
  {
    const std::string& n = glyph_names[ i ];

    // prime glyphs
    if      ( n == "crusader_strike"         ) glyphs.crusader_strike = 1;
    else if ( n == "divine_favor"            ) ;
    else if ( n == "exorcism"                ) glyphs.exorcism = 1;
    else if ( n == "holy_shock"              ) glyphs.holy_shock = 1;
    else if ( n == "judgement"               ) glyphs.judgement = 1;
    else if ( n == "hammer_of_the_righteous" ) glyphs.hammer_of_the_righteous = 1;
    else if ( n == "seal_of_insight"         ) ;
    else if ( n == "seal_of_truth"           ) glyphs.seal_of_truth = 1;
    else if ( n == "shield_of_the_righteous" ) glyphs.shield_of_the_righteous = 1;
    else if ( n == "templars_verdict"        ) glyphs.templars_verdict = 1;
    else if ( n == "word_of_glory"           ) ;
    // major glyphs
    else if ( n == "ascetic_crusader"        ) glyphs.ascetic_crusader = 1;
    else if ( n == "beacon_of_light"         ) ;
    else if ( n == "cleansing"               ) ;
    else if ( n == "consecration"            ) glyphs.consecration = 1;
    else if ( n == "dazing_shield"           ) ;
    else if ( n == "divine_plea"             ) ;
    else if ( n == "divine_protection"       ) ;
    else if ( n == "divinity"                ) ;
    else if ( n == "focused_shield"          ) glyphs.focused_shield = 1;
    else if ( n == "hammer_of_justice"       ) ;
    else if ( n == "hammer_of_wrath"         ) glyphs.hammer_of_wrath = 1;
    else if ( n == "hand_of_salvation"       ) ;
    else if ( n == "holy_wrath"              ) ;
    else if ( n == "light_of_dawn"           ) ;
    else if ( n == "long_word"               ) ;
    else if ( n == "rebuke"                  ) ;
    else if ( n == "turn_evil"               ) ;
    // minor glyphs
    else if ( n == "blessing_of_kings"       ) ;
    else if ( n == "blessing_of_might"       ) ;
    else if ( n == "insight"                 ) ;
    else if ( n == "justice"                 ) ;
    else if ( n == "lay_on_hands"            ) ;
    else if ( n == "righteousness"           ) ;
    else if ( n == "truth"                   ) ;
    else if ( ! sim -> parent ) 
    {
      sim -> errorf( "Player %s has unrecognized glyph %s\n", name(), n.c_str() );
    }
  }
}

// paladin_t::init_scaling ==================================================

void paladin_t::init_scaling()
{
  player_t::init_scaling();

  talent_tree_type tree = primary_tree();

  // Technically prot and ret scale with int and sp too, but it's so minor it's not worth the sim time.
  scales_with[ STAT_INTELLECT   ] = tree == TREE_HOLY;
  scales_with[ STAT_SPIRIT      ] = tree == TREE_HOLY;
  scales_with[ STAT_SPELL_POWER ] = tree == TREE_HOLY;
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

  bool is_melee = ( strstr( s, "helm"           ) || 
                    strstr( s, "shoulderplates" ) ||
                    strstr( s, "battleplate"    ) ||
                    strstr( s, "chestpiece"     ) ||
                    strstr( s, "legplates"      ) ||
                    strstr( s, "gauntlets"      ) );

  bool is_tank = ( strstr( s, "faceguard"      ) || 
                   strstr( s, "shoulderguards" ) ||
                   strstr( s, "breastplate"    ) ||
                   strstr( s, "legguards"      ) ||
                   strstr( s, "handguards"     ) );
  
  if ( strstr( s, "redemption" ) ) 
  {
    if ( is_melee ) return SET_T7_MELEE;
    if ( is_tank  ) return SET_T7_TANK;
  }
  if ( strstr( s, "aegis" ) )
  {
    if ( is_melee ) return SET_T8_MELEE;
    if ( is_tank  ) return SET_T8_TANK;
  }
  if ( strstr( s, "turalyons" ) ||
       strstr( s, "liadrins"  ) ) 
  {
    if ( is_melee ) return SET_T9_MELEE;
    if ( is_tank  ) return SET_T9_TANK;
  }
  if ( strstr( s, "lightsworn" ) )
  {
    if ( is_melee  ) return SET_T10_MELEE;
    if ( is_tank   ) return SET_T10_TANK;
  }
  if ( strstr( s, "reinforced_sapphirium" ) )
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

    if ( is_melee  ) return SET_T11_MELEE;
    if ( is_tank   ) return SET_T11_TANK;
  }

  return SET_NONE;
}

// paladin_t::init_buffs ====================================================

void paladin_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( sim, player, name, max_stack, duration, cooldown, proc_chance, quiet )

  buffs_avenging_wrath         = new buff_t( this, "avenging_wrath",         1, spells.avenging_wrath->duration() );
  buffs_divine_favor           = new buff_t( this, "divine_favor",           1, spells.divine_favor->duration() );
  buffs_divine_plea            = new buff_t( this, "divine_plea",            1, spells.divine_plea->duration() );
  buffs_holy_shield            = new buff_t( this, "holy_shield",            1, 20.0 );
  buffs_judgements_of_the_pure = new buff_t( this, "judgements_of_the_pure", 1, 60.0 );
  buffs_reckoning              = new buff_t( this, "reckoning",              4,  8.0, 0, talents.reckoning * 0.10 );
  buffs_censure                = new buff_t( this, "censure",                5       );
  buffs_the_art_of_war         = new buff_t( this, "the_art_of_war",         1,
                                             player_data.spell_duration( talents.the_art_of_war->effect_trigger_spell(3) ),
                                             0,
                                             talents.the_art_of_war->proc_chance() );

  // stat_buff_t( sim, player, name, stat, amount, max_stack, duration, cooldown, proc_chance, quiet )

  buffs_holy_power             = new buff_t( this, "holy_power", 3 );
  buffs_inquisition            = new buff_t( this, "inquisition", 1 );
  buffs_zealotry               = new buff_t( this, "zealotry", 1, spells.zealotry->duration() );
  buffs_judgements_of_the_wise = new buff_t( this, "judgements_of_the_wise", 1, spells.judgements_of_the_wise->duration() );
  buffs_judgements_of_the_bold = new buff_t( this, "judgements_of_the_bold", 1, spells.judgements_of_the_bold->duration() );
  buffs_hand_of_light          = new buff_t( this, "hand_of_light", 1, player_data.spell_duration( passives.hand_of_light->effect_trigger_spell(1) ) );
  buffs_ancient_power          = new buff_t( this, "ancient_power", -1 );
}

// paladin_t::init_actions ==================================================

void paladin_t::init_actions()
{
  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  active_seals_of_command_proc      = new seals_of_command_proc_t     ( this );
  active_seal_of_justice_proc       = new seal_of_justice_proc_t      ( this );
  active_seal_of_insight_proc       = new seal_of_insight_proc_t      ( this );
  active_seal_of_righteousness_proc = new seal_of_righteousness_proc_t( this );
  active_seal_of_truth_proc         = new seal_of_truth_proc_t        ( this );
  active_seal_of_truth_dot          = new seal_of_truth_dot_t         ( this );

  if ( action_list_str.empty() && primary_tree() == TREE_RETRIBUTION )
  {
    // TODO: new food
    action_list_str = "flask,type=titanic_strength/food,type=dragonfin_filet/speed_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<=60";
    action_list_str += "/seal_of_truth";
    action_list_str += "/snapshot_stats";
    // TODO: action_list_str += "/rebuke";

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
    if ( race == RACE_BLOOD_ELF ) action_list_str += "/arcane_torrent";
    action_list_str += "/guardian_of_ancient_kings";
    action_list_str += "avenging_wrath,if=buff.zealotry.down";
    action_list_str += "zealotry,if=buff.avenging_wrath.down";
    action_list_str += "/inquisition,if=(buff.inquisition.down|buff.inquisition.remains<1)&(buff.holy_power.react==3|buff.hand_of_light.react)";
    action_list_str += "/exorcism,recast=1,if=buff.the_art_of_war.react";
    action_list_str += "/hammer_of_wrath";
    // CS before TV if <3 power, even with HoL up
    action_list_str += "/templars_verdict,if=buff.holy_power.react==3";
    action_list_str += "/crusader_strike,if=buff.hand_of_light.react&(buff.hand_of_light.remains>2)&(buff.holy_power.react<3)";
    action_list_str += "/templars_verdict,if=buff.hand_of_light.react";
    action_list_str += "/crusader_strike";
    // fillers (todo: consecration)
    action_list_str += "/judgement";
    action_list_str += "/holy_wrath";
    // Don't delay CS for consecration or divine plea
    action_list_str += "/wait,sec=0.1,if=(cooldown.crusader_strike.remains>0)&(cooldown.crusader_strike.remains<1)";
    action_list_str += "/consecration";
    action_list_str += "/divine_plea";

    action_list_default = 1;
  }

  player_t::init_actions();
}

void paladin_t::init_talents()
{
  // Holy
  talents.arbiter_of_the_light   = new talent_t( this, "arbiter_of_the_light", "Arbiter of the Light" );
  talents.judgements_of_the_pure = new talent_t( this, "judgements_of_the_pure", "Judgements of the Pure" );
  talents.blazing_light          = new talent_t( this, "blazing_light", "Blazing Light" );
  talents.divine_favor           = new talent_t( this, "divine_favor", "Divine Favor" );
  // Prot
  talents.seals_of_the_pure         = new talent_t( this, "seals_of_the_pure", "Seals of the Pure" );
  talents.improved_hammer_of_justice= new talent_t( this, "improved_hammer_of_justice", "Improved Hammer of Justice" );
  talents.hallowed_ground           = new talent_t( this, "hallowed_ground", "Hallowed Ground" );
  talents.hammer_of_the_righteous   = new talent_t( this, "hammer_of_the_righteous", "Hammer of the Righteous" );
  talents.shield_of_the_righteous   = new talent_t( this, "shield_of_the_righteous", "Shield of the Righteous" );
  talents.wrath_of_the_lightbringer = new talent_t( this, "wrath_of_the_lightbringer", "Wrath of the Lightbringer" );
  // Ret
  talents.crusade            = new talent_t( this, "crusade", "Crusade" );
  talents.rule_of_law        = new talent_t( this, "rule_of_law", "Rule of Law" );
  talents.communion          = new talent_t( this, "communion", "Communion" );
  talents.the_art_of_war     = new talent_t( this, "the_art_of_war", "The Art of War" );
  talents.divine_storm       = new talent_t( this, "divine_storm", "Divine Storm" );
  talents.sanctity_of_battle = new talent_t( this, "sanctity_of_battle", "Sanctity of Battle" );
  talents.seals_of_command   = new talent_t( this, "seals_of_command", "Seals of Command" );
  talents.divine_purpose     = new talent_t( this, "divine_purpose", "Divine Purpose" );
  talents.sanctified_wrath   = new talent_t( this, "sanctified_wrath", "Sanctified Wrath" );
  talents.inquiry_of_faith   = new talent_t( this, "inquiry_of_faith", "Inquiry of Faith" );
  talents.zealotry           = new talent_t( this, "zealotry", "Zealotry" );

  player_t::init_talents();
}

void paladin_t::init_spells()
{
  player_t::init_spells();

  spells.avengers_shield           = new active_spell_t( this, "avengers_shield", "Avenger's Shield" );
  spells.avenging_wrath            = new active_spell_t( this, "avenging_wrath", "Avenging Wrath" );
  spells.consecration              = new active_spell_t( this, "consecration", "Consecration" );
  spells.consecration_tick         = new active_spell_t( this, "consecration_tick", 81297 );
  spells.crusader_strike           = new active_spell_t( this, "crusader_strike", "Crusader Strike" );
  spells.divine_favor              = new active_spell_t( this, "divine_favor", "Divine Favor", talents.divine_favor );
  spells.divine_plea               = new active_spell_t( this, "divine_plea", "Divine Plea" );
  spells.divine_storm              = new active_spell_t( this, "divine_storm", "Divine Storm", talents.divine_storm );
  spells.exorcism                  = new active_spell_t( this, "exorcism", "Exorcism" );
  spells.guardian_of_ancient_kings = new active_spell_t( this, "guardian_of_ancient_kings", 86150 );
  spells.guardian_of_ancient_kings_ret = new active_spell_t( this, "guardian_of_ancient_kings", 86698 );
  spells.ancient_fury              = new active_spell_t( this, "ancient_fury", 86704 );
  spells.hammer_of_justice         = new active_spell_t( this, "hammer_of_justice", "Hammer of Justice" );
  spells.hammer_of_the_righteous   = new active_spell_t( this, "hammer_of_the_righteous", "Hammer of the Righteous", talents.hammer_of_the_righteous );
  spells.hammer_of_wrath           = new active_spell_t( this, "hammer_of_wrath", "Hammer of Wrath" );
  spells.holy_shock                = new active_spell_t( this, "holy_shock", "Holy Shock" );
  spells.holy_wrath                = new active_spell_t( this, "holy_wrath", "Holy Wrath" );
  spells.inquisition               = new active_spell_t( this, "inquisition", "Inquisition" );
  spells.judgement                 = new active_spell_t( this, "judgement", "Judgement" );
  spells.shield_of_the_righteous   = new active_spell_t( this, "shield_of_the_righteous", "Shield of the Righteous", talents.shield_of_the_righteous );
  spells.templars_verdict          = new active_spell_t( this, "templars_verdict", "Templar's Verdict" );
  spells.zealotry                  = new active_spell_t( this, "zealotry", "Zealotry", talents.zealotry );
  spells.judgements_of_the_wise    = new active_spell_t( this, "judgements_of_the_wise", 31930 );
  spells.judgements_of_the_bold    = new active_spell_t( this, "judgements_of_the_bold", 89906 );

  passives.touched_by_the_light   = new passive_spell_t( this, "touched_by_the_light", "Touched by the Light" );
  passives.vengeance              = new passive_spell_t( this, "vengeance", "Vengeance" );
  passives.divine_bulwark         = new mastery_t( this, "divine_bulwark", "Divine Bulwark", TREE_PROTECTION );
  passives.sheath_of_light        = new passive_spell_t( this, "sheath_of_light", "Sheath of Light" );
  passives.two_handed_weapon_spec = new passive_spell_t( this, "two_handed_weapon_specialization", "Two-Handed Weapon Specialization" );
  passives.hand_of_light          = new mastery_t( this, "hand_of_light", "Hand of Light", TREE_RETRIBUTION );
  passives.plate_specialization   = new passive_spell_t( this, "plate_specialization", 86525 );
  passives.judgements_of_the_bold = new passive_spell_t( this, "judgements_of_the_bold", "Judgements of the Bold" );
  passives.judgements_of_the_wise = new passive_spell_t( this, "judgements_of_the_wise", "Judgements of the Wise" );
}

// paladin_t::primary_tab ====================================================

talent_tab_name paladin_t::primary_tab() SC_CONST
{
  int num_ranks[3] = { 0, 0, 0 };
  for (size_t i = 0; i < talent_list2.size(); ++i)
  {
    unsigned page = talent_list2.at(i)->t_data->tab_page;
    assert(page < 3);
    num_ranks[page] += talent_list2.at(i)->rank();
  }

  int min_ranks = level > 69 ? 11 : 1;
  if (num_ranks[PALADIN_HOLY] >= min_ranks)
    return PALADIN_HOLY;
  else if (num_ranks[PALADIN_PROTECTION] >= min_ranks)
    return PALADIN_PROTECTION;
  else if (num_ranks[PALADIN_RETRIBUTION] >= min_ranks)
    return PALADIN_RETRIBUTION;
  else
    return TALENT_TAB_NONE;
}

// paladin_t::composite_attribute_multiplier =================================

double paladin_t::composite_attribute_multiplier( int attr ) SC_CONST
{
  double m = player_t::composite_attribute_multiplier(attr);
  if ( attr == ATTR_STRENGTH && buffs_ancient_power->check() )
  {
    m *= 1.0 + 0.01 * buffs_ancient_power->stack();
  }
  return m;
}

// paladin_t::composite_spell_power ==========================================

double paladin_t::composite_spell_power( const school_type school ) SC_CONST
{
  double sp = player_t::composite_spell_power( school );
  switch ( primary_tree() )
  {
  case TREE_PROTECTION:
    sp += strength() * 0.01 * passives.touched_by_the_light->effect_base_value(1);
    break;
  case TREE_RETRIBUTION:
    sp += composite_attack_power_multiplier() * composite_attack_power() * 0.01 * passives.sheath_of_light->effect_base_value(1);
    break;
  default: break;
  }
  return sp;
}

// paladin_t::composite_tank_block ===========================================

double paladin_t::composite_tank_block() SC_CONST
{
  double b = player_t::composite_tank_block();
  b += get_divine_bulwark();
  return b;
}

// paladin_t::matching_gear_multiplier =====================================

double paladin_t::matching_gear_multiplier( const attribute_type attr ) SC_CONST
{
  double mult = 0.01 * passives.plate_specialization->effect_base_value(1);
  switch ( primary_tree() )
  {
  case TREE_PROTECTION:
    if ( attr == ATTR_STAMINA )
      return mult;
    break;
  case TREE_RETRIBUTION:
    if ( attr == ATTR_STRENGTH )
      return mult;
    break;
  case TREE_HOLY:
    if ( attr == ATTR_INTELLECT )
      return mult;
    break;
  default: 
    break;
  }
  return 0.0;
}

// paladin_t::regen  ========================================================

void paladin_t::regen( double periodicity )
{
  player_t::regen( periodicity );

  if ( buffs_divine_plea -> up() )
  {
    double amount = periodicity * resource_max[ RESOURCE_MANA ] * 0.10 / buffs_divine_plea -> buff_duration;
    resource_gain( RESOURCE_MANA, amount, gains_divine_plea );
  }
  if ( buffs_judgements_of_the_wise -> up() )
  {
    double mps = resource_base[ RESOURCE_MANA ] * spells.judgements_of_the_wise->effect_base_value(1) * 0.01;
    double amount = periodicity * mps / buffs_judgements_of_the_wise -> buff_duration;
    resource_gain( RESOURCE_MANA, amount, gains_judgements_of_the_wise );
  }
  if ( buffs_judgements_of_the_bold -> up() )
  {
    double mps = resource_base[ RESOURCE_MANA ] * spells.judgements_of_the_bold->effect_base_value(1) * 0.01;
    double amount = periodicity * mps / buffs_judgements_of_the_bold -> buff_duration;
    resource_gain( RESOURCE_MANA, amount, gains_judgements_of_the_bold );
  }
}

// paladin_t::target_swing ==================================================

int paladin_t::target_swing()
{
  int result = player_t::target_swing();

  if ( sim -> log ) log_t::output( sim, "%s swing result: %s", sim -> target -> name(), util_t::result_type_string( result ) );

  if ( result == RESULT_BLOCK )
  {
    buffs_reckoning -> trigger();
  }
  if ( result == RESULT_PARRY )
  {
    if ( main_hand_attack && main_hand_attack -> execute_event )
    {
      double swing_time = main_hand_attack -> time_to_execute;
      double max_reschedule = ( main_hand_attack -> execute_event -> occurs() - 0.20 * swing_time ) - sim -> current_time;

      if ( max_reschedule > 0 )
      {
	main_hand_attack -> reschedule_execute( std::min( ( 0.40 * swing_time ), max_reschedule ) );
	procs_parry_haste -> occur();
      }
    }
  }
  return result;
}

// paladin_t::get_talents_trees ==============================================

std::vector<talent_translation_t>& paladin_t::get_talent_list()
{
  talent_list.clear();
  return talent_list;
}

// paladin_t::get_options ====================================================

std::vector<option_t>& paladin_t::get_options()
{
  if ( options.empty() )
  {
    player_t::get_options();

    option_t paladin_options[] =
    {
      // @option_doc loc=player/paladin/talents title="Talents"
      { "ardent_defender",             OPT_INT,         &( talents.ardent_defender             ) },
      { "aura_mastery",                OPT_INT,         &( talents.aura_mastery                ) },
      { "blessed_life",                OPT_INT,         &( talents.blessed_life                ) },
      { "enlightened_judgements",      OPT_INT,         &( talents.enlightened_judgements      ) },
      { "eternal_glory",               OPT_INT,         &( talents.eternal_glory               ) },
      { "eye_for_an_eye",              OPT_INT,         &( talents.eye_for_an_eye              ) },
      { "guarded_by_the_light",        OPT_INT,         &( talents.guarded_by_the_light        ) },
      { "holy_shield",                 OPT_INT,         &( talents.holy_shield                 ) },
      { "improved_judgement",          OPT_INT,         &( talents.improved_judgement          ) },
      { "judgements_of_the_just",      OPT_INT,         &( talents.judgements_of_the_just      ) },
      { "long_arm_of_the_law",         OPT_INT,         &( talents.long_arm_of_the_law         ) },
      { "rebuke",                      OPT_INT,         &( talents.rebuke                      ) },
      { "reckoning",                   OPT_INT,         &( talents.reckoning                   ) },
      { "sacred_duty",                 OPT_INT,         &( talents.sacred_duty                 ) },
      { "shield_of_the_templar",       OPT_INT,         &( talents.shield_of_the_templar       ) },
      { "toughness",                   OPT_INT,         &( talents.toughness                   ) },
      { "vindication",                 OPT_INT,         &( talents.vindication                 ) },
      // @option_doc loc=player/paladin/misc title="Misc"
      { "tier10_2pc_procs_from_strikes", OPT_DEPRECATED, NULL },
      { NULL, OPT_UNKNOWN, NULL }
    };

    option_t::copy( options, paladin_options );
  }
  return options;
}

// paladin_t::get_cooldown ===================================================

cooldown_t* paladin_t::get_cooldown( const std::string& name )
{
  if ( name == "hammer_of_the_righteous" ) return player_t::get_cooldown( "crusader_strike" );
  if ( name == "divine_storm"            ) return player_t::get_cooldown( "crusader_strike" );

  return player_t::get_cooldown( name );
}

// paladin_t::create_pet =====================================================

pet_t* paladin_t::create_pet( const std::string& name )
{
  pet_t* p = find_pet(name);
  if (p) return p;
  if (name == "guardian_of_ancient_kings_ret")
  {
    return new guardian_of_ancient_kings_ret_t(sim, this);
  }
  return 0;
}

// paladin_t::create_pets ====================================================

// FIXME: Not possible to check spec at this point, but in the future when all
// three versions of the guardian are implemented, it would be fugly to have to
// give them different names just for the lookup
void paladin_t::create_pets()
{
  guardian_of_ancient_kings = create_pet("guardian_of_ancient_kings_ret");
}

// paladin_t::has_holy_power =================================================

bool paladin_t::has_holy_power(int power_needed) SC_CONST
{
  return buffs_hand_of_light -> check() || buffs_holy_power -> check() >= power_needed;
}

// paladin_t::holy_power_stacks ==============================================

int paladin_t::holy_power_stacks() SC_CONST
{
  if ( buffs_hand_of_light -> up() )
    return 3;
  else if ( buffs_holy_power -> up() )
    return buffs_holy_power -> check();
  else
    return 0;
}

// paladin_t::get_divine_bulwark =============================================

double paladin_t::get_divine_bulwark() SC_CONST
{
  if ( primary_tree() != TREE_PROTECTION ) return 0.0;

  // block rating, 2.25% per point of mastery
  return composite_mastery() * 0.0225;
}

// paladin_t::get_hand_of_light ==============================================

double paladin_t::get_hand_of_light() SC_CONST
{
  if ( primary_tree() != TREE_RETRIBUTION ) return 0.0;

  // chance to proc buff, 1% per point of mastery
  return composite_mastery() * 0.01;
}

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

player_t* player_t::create_paladin( sim_t* sim, const std::string& name, race_type r )
{
  return new paladin_t( sim, name, r );
}

// player_t::paladin_init ====================================================

void player_t::paladin_init( sim_t* sim )
{
  sim -> auras.devotion_aura          = new aura_t( sim, "devotion_aura",          1 );
  sim -> auras.sanctified_retribution = new buff_t( sim, "sanctified_retribution", 1 );

  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    p -> buffs.blessing_of_kings  = new buff_t( p, "blessing_of_kings",  !p -> is_pet() );
    p -> buffs.blessing_of_might  = new buff_t( p, "blessing_of_might",  !p -> is_pet() );
    p -> buffs.blessing_of_wisdom = new buff_t( p, "blessing_of_wisdom", !p -> is_pet() );
  }

  for ( target_t* t = sim -> target_list; t; t = t -> next )
  {
  t -> debuffs.judgements_of_the_just = new debuff_t( sim, "judgements_of_the_just", 1, 20.0 );
  }
}

// player_t::paladin_combat_begin ============================================

void player_t::paladin_combat_begin( sim_t* sim )
{
  if( sim -> overrides.devotion_aura          ) sim -> auras.devotion_aura          -> override( 1, 1205 * 1.5 );
  if( sim -> overrides.sanctified_retribution ) sim -> auras.sanctified_retribution -> override();

  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if ( p -> ooc_buffs() )
    {
      if ( sim -> overrides.blessing_of_kings  ) p -> buffs.blessing_of_kings  -> override();
      if ( sim -> overrides.blessing_of_might  ) p -> buffs.blessing_of_might  -> override( 1, 688 );
      if ( sim -> overrides.blessing_of_wisdom ) p -> buffs.blessing_of_wisdom -> override( 1, 91 * 1.2 );
    }
  }

  for ( target_t* t = sim -> target_list; t; t = t -> next )
  {
  if ( sim -> overrides.judgements_of_the_just ) t -> debuffs.judgements_of_the_just -> override();
  }
}
