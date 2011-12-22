// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
//
// TODO:
//  Sooner:
//   * Recheck the fixmes.
//   * Measure and implement the formula for rage from damage taken
//     (n.b., based on percentage of HP for unmitigated hit size).
//   * Watch Raging Blow and see if Blizzard fix the bug where it's
//     not refunding 80% of the rage cost if it misses.
//   * Consider testing the rest of the abilities for that too.
//   * Sanity check init_buffs() wrt durations and chances.
//  Later:
//   * Verify that Colossus Smash itself doesn't ignore armor.
//   * Get Heroic Strike to trigger properly "off gcd" using priority.
//   * Move the bleeds out of being warrior_attack_t to stop them
//     triggering effects or having special cases in the class.
//   * Prot? O_O
//
// NOTES:
//  Damage increase types per spell:
//
//   * battle_stance                    = 21156 = mod_damage_done% (0x7f)
//   * berserker_stance                 =  7381 = mod_damage_done% (0x7f)
//   * blood_frenzy                     = 30070 = mod_damage_taken% (0x1)
//                                      = 46857 = ???
//   * cruelty                          = 12582 = add_flat_mod_spell_crit_chance (7)
//   * death_wish                       = 12292 = mod_damage_done% (0x1)
//   * dual_wield_specialization        = 23588 = mod_damage_done% (0x7f)
//   * enrage (bastion_of_defense)      = 57516 = mod_damage_done% (0x1)
//   * enrage (fury)                    = 14202 = mod_damage_done% (0x1)
//   * enrage (wrecking_crew)           = 57519 = mod_damage_done% (0x1)
//   * heavy_repercussions              = 86896 = add_flat_mod_spell_effect2 (12)
//   * hold_the_line                    = 84620 = mod_crit% (127)
//   * improved_revenge                 = 12799 = add_percent_mod_generic
//   * incite                           = 50687 = add_flat_mod_spell_crit_chance (7)
//                                      = 86627 = add_flat_mod_spell_crit_chance (7)
//   * inner_rage                       =  1134 = mod_damage_done% (0x7f)
//   * juggernaut                       = 64976 = add_flat_mod_spell_crit_chance (7)
//   * lambs_to_the_slaughter           = 84586 = add_percent_mod_generic
//   * meat_cleaver                     = 85739 = add_percent_mod_generic
//   * rampage                          = 29801 = mod_crit% (7)
//   * recklessness                     =  1719 = add_flat_mod (7)
//   * rude_interruption                = 86663 = mod_damage_done% (0x7f)
//   * singleminded_fury                = 81099 = mod_damage_done% (0x7f)
//   * sword_and_board                  = 46953 = add_flat_mod_spell_crit_chance (7)
//   * thunderstruck                    = 87096 = add_percent_mod_generic
//   * war_acacdemy                     = 84572 = add_percent_mod_generic
//   * warrior_tier11_dps_2pc_bonus     = 90293 = add_percent_mod_generic
//   * warrior_tier11_dps_4pc_bonus     = 90294 = mod_melee_attack_power%
//   * twohanded_weapon_specialization  = 12712 = mod_damage_done% (0x1)
//   * unshackled_fury                  = 76856 = add_percent_mod_spell_effect1/2
//
//   * glyph_of_bloodthirst             = 58367 = add_percent_mod
//   * glyph_of_devastate               = 58388 = add_flat_mod_spell_crit_chance (7)
//   * glyph_of_mortal_strike           = 58368 = add_percent_mod_generic
//   * glyph_of_overpower               = 58386 = add_percent_mod_generic
//   * glyph_of_raging_blow             = 58370 = add_flat_mod_spell_crit_chance (7)
//   * glyph_of_revenge                 = 58364 = add_percent_mod_generic
//   * glyph_of_shield_slam             = 58375 = add_percent_mod_generic
//   * glyph_of_slam                    = 58385 = add_flat_mod_spell_crit_chance (7)
//
// ==========================================================================


// ==========================================================================
// Warrior
// ==========================================================================

enum warrior_stance { STANCE_BATTLE=1, STANCE_BERSERKER, STANCE_DEFENSE=4 };

struct warrior_t : public player_t
{
  int instant_flurry_haste;
  int initial_rage;

  // Active
  action_t* active_deep_wounds;
  action_t* active_retaliation;
  action_t* active_opportunity_strike;
  int       active_stance;
  action_t* active_tier12_2pc_tank;
  action_t* fiery_attack;

  // Buffs
  buff_t* buffs_bastion_of_defense;
  buff_t* buffs_battle_stance;
  buff_t* buffs_battle_trance;
  buff_t* buffs_berserker_rage;
  buff_t* buffs_berserker_stance;
  buff_t* buffs_bloodsurge;
  buff_t* buffs_bloodthirst;
  buff_t* buffs_colossus_smash;
  buff_t* buffs_deadly_calm;
  buff_t* buffs_death_wish;
  buff_t* buffs_defensive_stance;
  buff_t* buffs_enrage;
  buff_t* buffs_executioner_talent;
  buff_t* buffs_flurry;
  buff_t* buffs_hold_the_line;
  buff_t* buffs_incite;
  buff_t* buffs_inner_rage;
  buff_t* buffs_juggernaut;
  buff_t* buffs_lambs_to_the_slaughter;
  buff_t* buffs_last_stand;
  buff_t* buffs_meat_cleaver;
  buff_t* buffs_overpower;
  buff_t* buffs_recklessness;
  buff_t* buffs_retaliation;
  buff_t* buffs_revenge;
  buff_t* buffs_rude_interruption;
  buff_t* buffs_shield_block;
  buff_t* buffs_sweeping_strikes;
  buff_t* buffs_sword_and_board;
  buff_t* buffs_taste_for_blood;
  buff_t* buffs_thunderstruck;
  buff_t* buffs_victory_rush;
  buff_t* buffs_wrecking_crew;
  buff_t* buffs_tier11_4pc_melee;
  buff_t* buffs_tier12_2pc_melee;
  buff_t* buffs_tier12_4pc_tank;
  buff_t* buffs_tier13_2pc_tank;

  // Cooldowns
  cooldown_t* cooldowns_colossus_smash;
  cooldown_t* cooldowns_shield_slam;
  cooldown_t* cooldowns_strikes_of_opportunity;

  // Dots
  dot_t* dots_deep_wounds;
  dot_t* dots_rend;

  // Gains
  gain_t* gains_anger_management;
  gain_t* gains_avoided_attacks;
  gain_t* gains_battle_shout;
  gain_t* gains_commanding_shout;
  gain_t* gains_berserker_rage;
  gain_t* gains_blood_frenzy;
  gain_t* gains_incoming_damage;
  gain_t* gains_charge;
  gain_t* gains_melee_main_hand;
  gain_t* gains_melee_off_hand;
  gain_t* gains_shield_specialization;
  gain_t* gains_sudden_death;

  // Glyphs
  struct glyphs_t
  {
    glyph_t* battle;
    glyph_t* berserker_rage;
    glyph_t* bladestorm;
    glyph_t* bloodthirst;
    glyph_t* bloody_healing;
    glyph_t* cleaving;
    glyph_t* colossus_smash;
    glyph_t* command;
    glyph_t* devastate;
    glyph_t* furious_sundering;
    glyph_t* heroic_throw;
    glyph_t* mortal_strike;
    glyph_t* overpower;
    glyph_t* raging_blow;
    glyph_t* rapid_charge;
    glyph_t* resonating_power;
    glyph_t* revenge;
    glyph_t* shield_slam;
    glyph_t* shockwave;
    glyph_t* slam;
    glyph_t* sweeping_strikes;
    glyph_t* victory_rush;
    glyphs_t() { memset( ( void* ) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  // Mastery
  struct war_mastery_t
  {
    mastery_t* critical_block;
    mastery_t* strikes_of_opportunity;
    mastery_t* unshackled_fury;

    war_mastery_t() { memset( ( void* ) this, 0x0, sizeof( war_mastery_t ) ); }
  };
  war_mastery_t mastery;

  // Procs
  proc_t* procs_munched_deep_wounds;
  proc_t* procs_rolled_deep_wounds;
  proc_t* procs_parry_haste;
  proc_t* procs_strikes_of_opportunity;
  proc_t* procs_sudden_death;
  proc_t* procs_fiery_attack;
  proc_t* procs_munched_tier12_2pc_tank;
  proc_t* procs_rolled_tier12_2pc_tank;
  proc_t* procs_tier13_4pc_melee;

  // Random Number Generation
  rng_t* rng_blood_frenzy;
  rng_t* rng_executioner_talent;
  rng_t* rng_impending_victory;
  rng_t* rng_strikes_of_opportunity;
  rng_t* rng_sudden_death;
  rng_t* rng_wrecking_crew;
  rng_t* rng_fiery_attack;

  // Spec Passives
  struct spec_t
  {
    passive_spell_t* anger_management;
    passive_spell_t* dual_wield_specialization;
    passive_spell_t* precision;
    passive_spell_t* sentinel;
    passive_spell_t* two_handed_weapon_specialization;

    spec_t() { memset( ( void* ) this, 0x0, sizeof( spec_t ) ); }
  };
  spec_t spec;

  // Talents
  struct talents_t
  {
    // Arms
    talent_t* bladestorm;
    talent_t* blitz;
    talent_t* blood_frenzy;
    talent_t* deadly_calm;
    talent_t* deep_wounds;
    talent_t* drums_of_war;
    talent_t* impale;
    talent_t* improved_slam;
    talent_t* juggernaut;
    talent_t* lambs_to_the_slaughter;
    talent_t* sudden_death;
    talent_t* sweeping_strikes;
    talent_t* tactical_mastery;
    talent_t* taste_for_blood;
    talent_t* war_academy;
    talent_t* wrecking_crew;

    // Fury
    talent_t* battle_trance;
    talent_t* bloodsurge;
    talent_t* booming_voice;
    talent_t* cruelty;
    talent_t* death_wish;
    talent_t* enrage;
    talent_t* executioner;
    talent_t* flurry;
    talent_t* gag_order;
    talent_t* intensify_rage;
    talent_t* meat_cleaver;
    talent_t* raging_blow;
    talent_t* rampage;
    talent_t* rude_interruption;
    talent_t* single_minded_fury;
    talent_t* titans_grip;

    // Prot
    talent_t* incite;
    talent_t* toughness;
    talent_t* blood_and_thunder;
    talent_t* shield_specialization;
    talent_t* shield_mastery;
    talent_t* hold_the_line;
    talent_t* last_stand;
    talent_t* concussion_blow;
    talent_t* bastion_of_defense;
    talent_t* warbringer;
    talent_t* improved_revenge;
    talent_t* devastate;
    talent_t* impending_victory;
    talent_t* thunderstruck;
    talent_t* vigilance;
    talent_t* heavy_repercussions;
    talent_t* safeguard;
    talent_t* sword_and_board;
    talent_t* shockwave;

    talents_t() { memset( ( void* ) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  // Up-Times
  benefit_t* uptimes_rage_cap;

  warrior_t( sim_t* sim, const std::string& name, race_type r = RACE_NONE ) :
    player_t( sim, WARRIOR, name, r )
  {
    if ( race == RACE_NONE ) race = RACE_NIGHT_ELF;

    tree_type[ WARRIOR_ARMS       ] = TREE_ARMS;
    tree_type[ WARRIOR_FURY       ] = TREE_FURY;
    tree_type[ WARRIOR_PROTECTION ] = TREE_PROTECTION;

    // Active
    active_deep_wounds        = 0;
    active_opportunity_strike = 0;
    active_tier12_2pc_tank    = 0;
    active_stance             = STANCE_BATTLE;
    fiery_attack = NULL;

    // Cooldowns
    cooldowns_colossus_smash         = get_cooldown( "colossus_smash"         );
    cooldowns_shield_slam            = get_cooldown( "shield_slam"            );
    cooldowns_strikes_of_opportunity = get_cooldown( "strikes_of_opportunity" );

    // Dots
    dots_deep_wounds = get_dot( "deep_wounds" );
    dots_rend        = get_dot( "rend"        );

    instant_flurry_haste = 1;
    initial_rage = 0;

    distance = 3;
    default_distance = 3;

    create_talents();
    create_glyphs();
    create_options();
  }

  // Character Definition
  virtual void      init_talents();
  virtual void      init_spells();
  virtual void      init_defense();
  virtual void      init_base();
  virtual void      init_scaling();
  virtual void      init_buffs();
  virtual void      init_values();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_benefits();
  virtual void      init_rng();
  virtual void      init_actions();
  virtual void      register_callbacks();
  virtual void      combat_begin();
  virtual double    composite_attack_power_multiplier() const;
  virtual double    composite_attack_hit() const;
  virtual double    composite_attack_crit() const;
  virtual double    composite_mastery() const;
  virtual double    composite_attack_haste() const;
  virtual double    composite_player_multiplier( const school_type school, action_t* a = NULL ) const;
  virtual double    matching_gear_multiplier( const attribute_type attr ) const;
  virtual double    composite_tank_block() const;
  virtual double    composite_tank_crit_block() const;
  virtual double    composite_tank_crit( const school_type school ) const;
  virtual double    composite_tank_parry() const;
  virtual void      reset();
  virtual void      regen( double periodicity );
  virtual void      create_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() const { return RESOURCE_RAGE; }
  virtual int       primary_role() const;
  virtual double    assess_damage( double amount, const school_type school, int    dmg_type, int result, action_t* a );
  virtual void      copy_from( player_t* source );
};

namespace   // ANONYMOUS NAMESPACE ==========================================
{

// ==========================================================================
// Warrior Attack
// ==========================================================================

struct warrior_attack_t : public attack_t
{
  int stancemask;

  warrior_attack_t( const char* n, warrior_t* p, const school_type s=SCHOOL_PHYSICAL, int t=TREE_NONE, bool special=true  ) :
    attack_t( n, p, RESOURCE_RAGE, s, t, special ),
    stancemask( STANCE_BATTLE|STANCE_BERSERKER|STANCE_DEFENSE )
  {
    _init_warrior_attack_t();
  }

  warrior_attack_t( const char* n, uint32_t id, warrior_t* p, int t=TREE_NONE, bool special = true ) :
    attack_t( n, id, p, t, special ),
    stancemask( STANCE_BATTLE|STANCE_BERSERKER|STANCE_DEFENSE )
  {
    _init_warrior_attack_t();
  }

  /* Class spell data based construction, spell name in s_name */
  warrior_attack_t( const char* n, const char* s_name, warrior_t* p ) :
    attack_t( n, s_name, p, TREE_NONE, true ),
    stancemask( STANCE_BATTLE|STANCE_BERSERKER|STANCE_DEFENSE )
  {
    _init_warrior_attack_t();
  }

  void _init_warrior_attack_t()
  {
    may_crit   = true;
    may_glance = false;
    // normalize_weapon_speed is set correctly by parse_data now
  }

  virtual double armor() const;
  virtual void   consume_resource();
  virtual double cost() const;
  virtual void   execute();
  virtual double calculate_weapon_damage();
  virtual void   player_buff();
  virtual bool   ready();
  virtual void   assess_damage( player_t* t, double amount, int dmg_type, int impact_result );
};


// ==========================================================================
// Static Functions
// ==========================================================================

// trigger_blood_frenzy =====================================================

static void trigger_blood_frenzy( action_t* a )
{
  warrior_t* p = a -> player -> cast_warrior();

  if ( ! p -> talents.blood_frenzy -> ok() )
    return;

  player_t* t = a -> target;

  // Don't alter the duration if it is set to 0 (override/optimal_raid)

  if ( t -> debuffs.blood_frenzy_bleed -> buff_duration > 0 )
  {
    t -> debuffs.blood_frenzy_bleed -> buff_duration = a -> num_ticks * a -> base_tick_time;
  }
  if ( t -> debuffs.blood_frenzy_physical -> buff_duration > 0 )
  {
    t -> debuffs.blood_frenzy_physical -> buff_duration = a -> num_ticks * a -> base_tick_time;
  }

  double rank = p -> talents.blood_frenzy -> rank();

  if ( rank * 15 >= t -> debuffs.blood_frenzy_bleed -> current_value )
  {
    t -> debuffs.blood_frenzy_bleed -> trigger( 1, rank * 15 );
    t -> debuffs.blood_frenzy_bleed -> source = p;
  }
  if ( rank * 2 >= t -> debuffs.blood_frenzy_physical -> current_value )
  {
    t -> debuffs.blood_frenzy_physical -> trigger( 1, rank * 2 );
    t -> debuffs.blood_frenzy_physical -> source = p;
  }
}

// trigger_bloodsurge =======================================================

static void trigger_bloodsurge( action_t* a )
{
  warrior_t* p = a -> player -> cast_warrior();

  if ( ! p -> talents.bloodsurge -> ok() )
    return;

  p -> buffs_bloodsurge -> trigger();
}

// Deep Wounds ==============================================================

struct deep_wounds_t : public warrior_attack_t
{
  deep_wounds_t( warrior_t* p ) :
    warrior_attack_t( "deep_wounds", 12721, p )
  {
    background = true;
    weapon_multiplier = p -> talents.deep_wounds -> rank() * 0.16; // hardcoded into tooltip, 02/11/2011
    may_crit = false;
    tick_may_crit = false;
    hasted_ticks  = false;
    tick_power_mod = 0;
    dot_behavior  = DOT_REFRESH;
  }
  virtual double total_td_multiplier() const { return target_multiplier; }
  virtual double travel_time() { return sim -> gauss( sim -> aura_delay, 0.25 * sim -> aura_delay ); }
  virtual void impact( player_t* t, int impact_result, double deep_wounds_dmg )
  {
    warrior_attack_t::impact( t, impact_result, 0 );
    if ( result_is_hit( impact_result ) )
    {
      base_td = deep_wounds_dmg / dot -> num_ticks;
      trigger_blood_frenzy( this );
    }
  }
};

// trigger_deep_wounds ======================================================

static void trigger_deep_wounds( action_t* a )
{
  warrior_t* p = a -> player -> cast_warrior();
  sim_t*   sim = a -> sim;

  if ( ! p -> talents.deep_wounds -> ok() )
    return;

  assert ( p -> active_deep_wounds );

  if ( a -> weapon )
    p -> active_deep_wounds -> weapon = a -> weapon;
  else
    p -> active_deep_wounds -> weapon = &( p -> main_hand_weapon );

  // Normally, we cache the base damage and then combine them with the multipliers at the point of damage.
  // However, in this case we need to maintain remaining damage on refresh and the player-buff multipliers may change.
  // So we neeed to push the player-buff multipliers into the damage bank and then make sure we only use
  // the target-debuff multipliers at tick time.

  p -> active_deep_wounds -> player_buff();

  double deep_wounds_dmg = ( p -> active_deep_wounds -> calculate_weapon_damage() *
                             p -> active_deep_wounds -> weapon_multiplier *
                             p -> active_deep_wounds -> player_multiplier );

  dot_t* dot = p -> active_deep_wounds -> dot;

  if ( dot -> ticking )
  {
    deep_wounds_dmg += p -> active_deep_wounds -> base_td * dot -> ticks();
  }

  if ( ( 6.0 + sim -> aura_delay ) < dot -> remains() )
  {
    if ( sim -> log ) log_t::output( sim, "Player %s munches Deep_Wounds due to Max Deep Wounds Duration.", p -> name() );
    p -> procs_munched_deep_wounds -> occur();
    return;
  }

  if ( p -> active_deep_wounds -> travel_event )
  {
    // There is an SPELL_AURA_APPLIED already in the queue, which will get munched.
    if ( sim -> log ) log_t::output( sim, "Player %s munches previous Deep Wounds due to Aura Delay.", p -> name() );
    p -> procs_munched_deep_wounds -> occur();
  }

  p -> active_deep_wounds -> direct_dmg = deep_wounds_dmg;
  p -> active_deep_wounds -> result = RESULT_HIT;
  p -> active_deep_wounds -> schedule_travel( a -> target );

  if ( p -> active_deep_wounds -> travel_event && dot -> ticking )
  {
    if ( dot -> tick_event -> occurs() < p -> active_deep_wounds -> travel_event -> occurs() )
    {
      // Deep_Wounds will tick before SPELL_AURA_APPLIED occurs, which means that the current Deep_Wounds will
      // both tick -and- get rolled into the next Deep_Wounds.
      if ( sim -> log ) log_t::output( sim, "Player %s rolls Deep_Wounds.", p -> name() );
      p -> procs_rolled_deep_wounds -> occur();
    }
  }
}

// trigger_rage_gain ========================================================

static void trigger_rage_gain( attack_t* a )
{
  // Since 4.0.1 rage gain is 6.5 * weaponspeed and half that for off-hand

  if ( a -> proc )
    return;

  warrior_t* p = a -> player -> cast_warrior();
  weapon_t*  w = a -> weapon;

  double rage_gain = 6.5 * w -> swing_time;

  if ( w -> slot == SLOT_OFF_HAND )
    rage_gain /= 2.0;

  rage_gain *= 1.0 + p -> spec.anger_management -> effect2().percent();

  p -> resource_gain( RESOURCE_RAGE,
                      rage_gain,
                      w -> slot == SLOT_OFF_HAND ? p -> gains_melee_off_hand : p -> gains_melee_main_hand );
}

// trigger_retaliation ======================================================

static void trigger_retaliation( warrior_t* p, int school, int result )
{
  if ( ! p -> buffs_retaliation -> up() )
    return;

  if ( school != SCHOOL_PHYSICAL )
    return;

  if ( ! ( result == RESULT_HIT || result == RESULT_CRIT || result == RESULT_BLOCK ) )
    return;

  if ( ! p -> active_retaliation )
  {
    struct retaliation_strike_t : public warrior_attack_t
    {
      retaliation_strike_t( warrior_t* p ) :
        warrior_attack_t( "retaliation_strike", p )
      {
        background = true;
        proc = true;
        trigger_gcd = 0;
        weapon = &( p -> main_hand_weapon );
        weapon_multiplier = 1.0;

        init();
      }
    };

    p -> active_retaliation = new retaliation_strike_t( p );
  }

  p -> active_retaliation -> execute();
  p -> buffs_retaliation -> decrement();
}

// trigger_strikes_of_opportunity ===========================================

struct opportunity_strike_t : public warrior_attack_t
{
  opportunity_strike_t( warrior_t* p ) :
    warrior_attack_t( "opportunity_strike", 76858, p, TREE_ARMS )
  {
    background = true;
  }
};

static void trigger_strikes_of_opportunity( attack_t* a )
{
  if ( a -> proc )
    return;

  warrior_t* p = a -> player -> cast_warrior();

  if ( ! p -> mastery.strikes_of_opportunity -> ok() )
    return;

  if ( p -> cooldowns_strikes_of_opportunity -> remains() > 0 )
    return;

  double chance = p -> composite_mastery() * p -> mastery.strikes_of_opportunity -> effect2().percent() / 100.0;

  if ( ! p -> rng_strikes_of_opportunity -> roll( chance ) )
    return;

  p -> cooldowns_strikes_of_opportunity -> start( 0.5 );

  assert( p -> active_opportunity_strike );

  if ( p -> sim -> debug )
    log_t::output( p -> sim, "Opportunity Strike procced from %s", a -> name() );


  p -> procs_strikes_of_opportunity -> occur();
  p -> active_opportunity_strike -> execute();
}

// trigger_sudden_death =====================================================

static void trigger_sudden_death( action_t* a )
{
  warrior_t* p = a -> player -> cast_warrior();

  if ( a -> proc )
    return;

  if ( p -> rng_sudden_death -> roll ( p -> talents.sudden_death -> proc_chance() ) )
  {
    p -> cooldowns_colossus_smash -> reset();
    p -> procs_sudden_death       -> occur();
  }
}

// trigger_sword_and_board ==================================================

static void trigger_sword_and_board( attack_t* a, int result )
{
  warrior_t* p = a -> player -> cast_warrior();

  if ( a -> result_is_hit( result ) )
  {
    if ( p -> buffs_sword_and_board -> trigger() )
    {
      p -> cooldowns_shield_slam -> reset();
    }
  }
}

// trigger_tier12_2pc_tank ==================================================

static void trigger_tier12_2pc_tank( attack_t* s, double dmg )
{
  if ( s -> school != SCHOOL_PHYSICAL ) return;

  warrior_t* p = s -> player -> cast_warrior();
  sim_t* sim = s -> sim;

  if ( ! p -> set_bonus.tier12_2pc_tank() ) return;

  struct combust_t : public warrior_attack_t
  {
    combust_t( warrior_t* player ) :
      warrior_attack_t( "shield_slam_combust", 99240, player )
    {
      background    = true;
      proc          = true;
      may_resist    = true;
      tick_may_crit = false;
      hasted_ticks  = false;
      dot_behavior  = DOT_REFRESH;
      init();
    }

    virtual void impact( player_t* t, int impact_result, double total_dot_dmg )
    {
      warrior_attack_t::impact( t, impact_result, 0 );

      base_td = total_dot_dmg / dot -> num_ticks;
    }

    virtual double travel_time()
    {
      return sim -> gauss( sim -> aura_delay, 0.25 * sim -> aura_delay );
    }

    virtual void target_debuff( player_t* /* t */, int /* dmg_type */ )
    {
      target_multiplier            = 1.0;
      target_hit                   = 0;
      target_crit                  = 0;
      target_attack_power          = 0;
      target_spell_power           = 0;
      target_penetration           = 0;
      target_dd_adder              = 0;
      if ( sim -> debug )
        log_t::output( sim, "action_t::target_debuff: %s multiplier=%.2f hit=%.2f crit=%.2f attack_power=%.2f spell_power=%.2f penetration=%.0f",
                       name(), target_multiplier, target_hit, target_crit, target_attack_power, target_spell_power, target_penetration );
    }

    virtual double total_td_multiplier() const { return 1.0; }
  };

  double total_dot_dmg = dmg * p -> dbc.spell( 99239 ) -> effect1().percent();

  if ( ! p -> active_tier12_2pc_tank ) p -> active_tier12_2pc_tank = new combust_t( p );

  dot_t* dot = p -> active_tier12_2pc_tank -> dot;

  if ( dot -> ticking )
  {
    total_dot_dmg += p -> active_tier12_2pc_tank -> base_td * dot -> ticks();
  }

  if ( ( p -> dbc.spell( 99240 ) -> duration() + sim -> aura_delay ) < dot -> remains() )
  {
    if ( sim -> log ) log_t::output( sim, "Player %s munches Combust due to Combust duration.", p -> name() );
    p -> procs_munched_tier12_2pc_tank -> occur();
    return;
  }

  if ( p -> active_tier12_2pc_tank -> travel_event )
  {
    // There is an SPELL_AURA_APPLIED already in the queue, which will get munched.
    if ( sim -> log ) log_t::output( sim, "Player %s munches previous Combust due to Aura Delay.", p -> name() );
    p -> procs_munched_tier12_2pc_tank -> occur();
  }

  p -> active_tier12_2pc_tank -> direct_dmg = total_dot_dmg;
  p -> active_tier12_2pc_tank -> result = RESULT_HIT;
  p -> active_tier12_2pc_tank -> schedule_travel( s -> target );

  if ( p -> active_tier12_2pc_tank -> travel_event && dot -> ticking )
  {
    if ( dot -> tick_event -> occurs() < p -> active_tier12_2pc_tank -> travel_event -> occurs() )
    {
      // Combust will tick before SPELL_AURA_APPLIED occurs, which means that the current Combust will
      // both tick -and- get rolled into the next Combust
      if ( sim -> log ) log_t::output( sim, "Player %s rolls Combust.", p -> name() );
      p -> procs_rolled_tier12_2pc_tank -> occur();
    }
  }
}

// trigger_enrage ===========================================================

static void trigger_enrage( attack_t* a )
{
  warrior_t* p = a -> player -> cast_warrior();

  if ( ! p -> talents.enrage -> ok() )
    return;

  // Can't proc while DW is active as of 403
  if ( p -> buffs_death_wish -> check() )
    return;

  double enrage_value = p -> buffs_enrage -> effect1().percent();

  if ( p -> mastery.unshackled_fury -> ok() )
  {
    enrage_value *= 1.0 + p -> composite_mastery() * p -> mastery.unshackled_fury -> effect3().percent() / 100.0;
  }

  p -> buffs_enrage -> trigger( 1, enrage_value );
}

// trigger_flurry ===========================================================

static void trigger_flurry( attack_t* a, int stacks )
{
  warrior_t* p = a -> player -> cast_warrior();

  bool up_before = p -> buffs_flurry -> check() != 0;

  if ( stacks >= 0 )
    p -> buffs_flurry -> trigger( stacks );
  else
    p -> buffs_flurry -> decrement();

  if ( ! p -> instant_flurry_haste )
    return;

  // Flurry needs to haste the in-progress attacks, and flurry dropping
  // needs to de-haste them.

  bool up_after = p -> buffs_flurry -> check() != 0;

  if ( up_before == up_after )
    return;

  sim_t *sim = p -> sim;

  // Default mult is the up -> down case
  double mult = 1 + util_t::talent_rank( p -> talents.flurry -> rank(), 3, 0.08, 0.16, 0.25 );

  // down -> up case
  if ( ! up_before && up_after )
    mult = 1 / mult;

  // This mess would be a lot easier if we could give a time instead of
  // a delta to reschedule_execute().
  if ( p -> main_hand_attack )
  {
    event_t* mhe = p -> main_hand_attack -> execute_event;
    if ( mhe )
    {
      double delta;
      if ( mhe -> reschedule_time )
        delta = ( mhe -> reschedule_time - sim -> current_time ) * mult;
      else
        delta = ( mhe -> time - sim -> current_time ) * mult;
      p -> main_hand_attack -> reschedule_execute( delta );
    }
  }
  if ( p -> off_hand_attack )
  {
    event_t* ohe = p -> off_hand_attack -> execute_event;
    if ( ohe )
    {
      double delta;
      if ( ohe -> reschedule_time )
        delta = ( ohe -> reschedule_time - sim -> current_time ) * mult;
      else
        delta = ( ohe -> time - sim -> current_time ) * mult;
      p -> off_hand_attack -> reschedule_execute( delta );
    }
  }
}

// trigger_tier12_4pc_melee =================================================

static void trigger_tier12_4pc_melee( attack_t* a )
{
  warrior_t* p = a -> player -> cast_warrior();

  if ( ! p -> set_bonus.tier12_4pc_melee() ) return;

  if ( ! a -> weapon ) return;
  if ( a -> weapon -> slot != SLOT_MAIN_HAND ) return;
  if ( a -> proc ) return;

  assert( p -> fiery_attack );

  if ( p -> rng_fiery_attack -> roll( p -> sets -> set( SET_T12_4PC_MELEE ) -> proc_chance() ) )
  {
    p -> procs_fiery_attack -> occur();
    p -> fiery_attack -> execute();
  }
}

// ==========================================================================
// Warrior Attacks
// ==========================================================================

double warrior_attack_t::armor() const
{
  warrior_t* p = player -> cast_warrior();

  double a = attack_t::armor();

  a *= 1.0 - p -> buffs_colossus_smash -> value();

  return a;
}

// warrior_attack_t::assess_damage ==========================================

void warrior_attack_t::assess_damage( player_t* t, double amount, int dmg_type, int impact_result )
{
  attack_t::assess_damage( t, amount, dmg_type, impact_result );

  /* warrior_t* p = player -> cast_warrior();

  if ( t -> is_enemy() )
  {
    target_t* q =  t -> cast_target();

    if ( p -> buffs_sweeping_strikes -> up() && q -> adds_nearby )
    {
      attack_t::additional_damage( q, amount, dmg_type, impact_result );
    }
  }*/
}

// warrior_attack_t::cost ===================================================

double warrior_attack_t::cost() const
{
  double c = attack_t::cost();

  warrior_t* p = player -> cast_warrior();

  if ( p -> buffs_deadly_calm -> check() )
    c  = 0;

  if ( p -> buffs_battle_trance -> check() && c > 5 )
    c  = 0;

  return c;
}

// warrior_attack_t::consume_resource =======================================

void warrior_attack_t::consume_resource()
{
  attack_t::consume_resource();
  warrior_t* p = player -> cast_warrior();

  p -> buffs_deadly_calm -> up();

  if ( proc )
    return;

  if ( result == RESULT_CRIT )
  {
    // Triggered here so it's applied between melee hits and next schedule.
    trigger_flurry( this, 3 );
  }

  // Warrior attacks (non-AoE) which are are avoided by the target consume only 20%
  if ( resource_consumed > 0 && ! aoe && result_is_miss() )
  {
    double rage_restored = resource_consumed * 0.80;
    p -> resource_gain( RESOURCE_RAGE, rage_restored, p -> gains_avoided_attacks );
  }
}

// warrior_attack_t::execute ================================================

void warrior_attack_t::execute()
{
  attack_t::execute();
  warrior_t* p = player -> cast_warrior();

  // Battle Trance only is effective+consumed if action cost was >5
  if ( base_cost > 5 && p -> buffs_battle_trance -> up() )
    p -> buffs_battle_trance -> expire();

  if ( proc ) return;

  if ( result_is_hit() )
  {
    trigger_sudden_death( this );

    trigger_strikes_of_opportunity( this );

    trigger_enrage( this );

    if ( result == RESULT_CRIT )
      trigger_deep_wounds( this );
  }
  else if ( result == RESULT_DODGE  )
  {
    p -> buffs_overpower -> trigger();
  }
}

// warrior_attack_t::calculate_weapon_damage ================================

double warrior_attack_t::calculate_weapon_damage()
{
  double dmg = attack_t::calculate_weapon_damage();

  // Catch the case where weapon == 0 so we don't crash/retest below.
  if ( dmg == 0 )
    return 0;

  warrior_t* p = player -> cast_warrior();

  if ( weapon -> slot == SLOT_OFF_HAND )
    dmg *= 1.0 + p -> spec.dual_wield_specialization -> effect_base_value( 2 ) / 100.0;

  return dmg;
}

// warrior_attack_t::player_buff ============================================

void warrior_attack_t::player_buff()
{
  attack_t::player_buff();
  warrior_t* p = player -> cast_warrior();

  // FIXME: much of this can be moved to base for efficiency, but we need
  // to be careful to get the add_percent_mod effect ordering in the
  // abilities right.

  // --- Specializations --

  if ( weapon && weapon -> group() == WEAPON_2H )
    player_multiplier *= 1.0 + p -> spec.two_handed_weapon_specialization -> effect_base_value( 1 ) / 100.0;

  if ( p -> dual_wield() && ( school == SCHOOL_PHYSICAL || school == SCHOOL_BLEED ) )
    player_multiplier *= 1.0 + p -> spec.dual_wield_specialization -> effect_base_value( 3 ) / 100.0;

  // --- Enrages ---

  if ( school == SCHOOL_PHYSICAL || school == SCHOOL_BLEED )
    player_multiplier *= 1.0 + p -> buffs_wrecking_crew -> value();

  if ( school == SCHOOL_PHYSICAL || school == SCHOOL_BLEED )
    player_multiplier *= 1.0 + p -> buffs_enrage -> value();

  if ( ( school == SCHOOL_PHYSICAL || school == SCHOOL_BLEED ) && p -> buffs_bastion_of_defense -> up() )
    player_multiplier *= 1.0 + p -> talents.bastion_of_defense -> rank() * 0.05;

  // --- Passive Talents ---

  if ( school == SCHOOL_PHYSICAL || school == SCHOOL_BLEED )
    player_multiplier *= 1.0 + p -> buffs_death_wish -> value();

  if ( p -> talents.single_minded_fury -> ok() && p -> dual_wield() )
  {
    if ( p -> main_hand_attack -> weapon -> group() == WEAPON_1H &&
         p ->  off_hand_attack -> weapon -> group() == WEAPON_1H )
    {
      player_multiplier *= 1.0 + p -> talents.single_minded_fury -> effect1().percent();
    }
  }

  // --- Buffs / Procs ---

  if ( p -> buffs_rude_interruption -> up() )
    player_multiplier *= 1.05;

  if ( special && p -> buffs_recklessness -> up() )
    player_crit += 0.5;

  if ( p -> buffs_hold_the_line -> up() )
    player_crit += 0.10;

  if ( sim -> debug )
    log_t::output( sim, "warrior_attack_t::player_buff: %s hit=%.2f expertise=%.2f crit=%.2f",
                   name(), player_hit, player_expertise, player_crit );
}

// warrior_attack_t::ready() ================================================

bool warrior_attack_t::ready()
{
  if ( ! attack_t::ready() )
    return false;

  warrior_t* p = player -> cast_warrior();

  // Attack available in current stance?
  if ( ( stancemask & p -> active_stance ) == 0 )
    return false;

  return true;
}

// Melee Attack =============================================================

struct melee_t : public warrior_attack_t
{
  int sync_weapons;

  melee_t( const char* name, warrior_t* p, int sw ) :
    warrior_attack_t( name, p, SCHOOL_PHYSICAL, TREE_NONE, false ),
    sync_weapons( sw )
  {
    may_glance      = true;
    background      = true;
    repeating       = true;
    trigger_gcd     = 0;

    if ( p -> dual_wield() ) base_hit -= 0.19;
  }

  virtual double swing_haste() const
  {
    double h = warrior_attack_t::swing_haste();

    warrior_t* p = player -> cast_warrior();

    if ( p -> buffs_flurry -> up() )
      h *= 1.0 / ( 1.0 + p -> buffs_flurry -> effect1().percent() );

    if ( p -> buffs_executioner_talent -> check() )
      h *= 1.0 / ( 1.0 + p -> buffs_executioner_talent -> stack() *
                   p -> buffs_executioner_talent -> effect1().percent() );

    return h;
  }

  virtual double execute_time() const
  {
    double t = warrior_attack_t::execute_time();

    if ( player -> in_combat )
      return t;

    if ( weapon -> slot == SLOT_MAIN_HAND || sync_weapons )
      return 0.02;

    // Before combat begins, unless we are under sync_weapons the OH is
    // delayed by half its swing time.

    return 0.02 + t / 2;
  }

  virtual void execute()
  {
    warrior_t* p = player -> cast_warrior();

    // Be careful changing where this is done.  Flurry that procs from melee
    // must be applied before the (repeating) event schedule, and the decrement
    // here must be done before it.
    trigger_flurry( this, -1 );

    warrior_attack_t::execute();

    if ( result != RESULT_MISS ) // Any attack that hits or is dodged/blocked/parried generates rage
      trigger_rage_gain( this );

    if ( result_is_hit() )
    {
      if ( ! proc &&  p -> rng_blood_frenzy -> roll( p -> talents.blood_frenzy -> proc_chance() ) )
      {
        p -> resource_gain( RESOURCE_RAGE, p -> talents.blood_frenzy -> effect3().base_value(), p -> gains_blood_frenzy );
      }
    }
  }

  virtual void player_buff()
  {
    warrior_attack_t::player_buff();
    warrior_t* p = player -> cast_warrior();

    if ( p -> primary_tree() == TREE_FURY )
      player_multiplier *= 1.0 + p -> spec.precision -> effect3().percent();
  }
};

// Auto Attack ==============================================================

struct auto_attack_t : public warrior_attack_t
{
  int sync_weapons;

  auto_attack_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "auto_attack", p ), sync_weapons( 0 )
  {
    option_t options[] =
    {
      { "sync_weapons", OPT_BOOL, &sync_weapons },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    assert( p -> main_hand_weapon.type != WEAPON_NONE );

    p -> main_hand_attack = new melee_t( "melee_main_hand", p, sync_weapons );
    p -> main_hand_attack -> weapon = &( p -> main_hand_weapon );
    p -> main_hand_attack -> base_execute_time = p -> main_hand_weapon.swing_time;

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      p -> off_hand_attack = new melee_t( "melee_off_hand", p, sync_weapons );
      p -> off_hand_attack -> weapon = &( p -> off_hand_weapon );
      p -> off_hand_attack -> base_execute_time = p -> off_hand_weapon.swing_time;
    }

    trigger_gcd = 0;
  }

  virtual void execute()
  {
    warrior_t* p = player -> cast_warrior();

    p -> main_hand_attack -> schedule_execute();

    if ( p -> off_hand_attack )
      p -> off_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    warrior_t* p = player -> cast_warrior();

    if ( p -> is_moving() )
      return false;

    return( p -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Bladestorm ===============================================================

struct bladestorm_tick_t : public warrior_attack_t
{
  bladestorm_tick_t( warrior_t* p, const char* name ) :
    warrior_attack_t( name, 50622, p, TREE_ARMS, false )
  {
    background  = true;
    direct_tick = true;
    aoe         = -1;
  }
};

struct bladestorm_t : public warrior_attack_t
{
  attack_t* bladestorm_mh;
  attack_t* bladestorm_oh;

  bladestorm_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "bladestorm", "Bladestorm", p ),
    bladestorm_mh( 0 ), bladestorm_oh( 0 )
  {
    check_talent( p -> talents.bladestorm -> rank() );

    parse_options( NULL, options_str );

    aoe       = -1;
    harmful   = false;
    channeled = true;
    tick_zero = true;

    cooldown -> duration += p -> glyphs.bladestorm -> effect1().seconds();

    bladestorm_mh = new bladestorm_tick_t( p, "bladestorm_mh" );
    bladestorm_mh -> weapon = &( player -> main_hand_weapon );
    add_child( bladestorm_mh );

    if ( player -> off_hand_weapon.type != WEAPON_NONE )
    {
      bladestorm_oh = new bladestorm_tick_t( p, "bladestorm_oh" );
      bladestorm_oh -> weapon = &( player -> off_hand_weapon );
      add_child( bladestorm_oh );
    }
  }

  virtual void execute()
  {
    warrior_attack_t::execute();

    warrior_t* p = player -> cast_warrior();

    if ( p -> main_hand_attack )
      p -> main_hand_attack -> cancel();

    if ( p ->  off_hand_attack )
      p -> off_hand_attack -> cancel();
  }

  virtual void tick( dot_t* d )
  {
    warrior_attack_t::tick( d );

    bladestorm_mh -> execute();

    if ( bladestorm_mh -> result_is_hit() && bladestorm_oh )
    {
      bladestorm_oh -> execute();
    }
  }

  // Bladestorm is not modified by haste effects
  virtual double haste() const { return 1.0; }
  virtual double swing_haste() const { return 1.0; }
};

// Bloodthirst Heal ==============================================================

struct bloodthirst_heal_t : public heal_t
{
  bloodthirst_heal_t( warrior_t* p ) :
    heal_t( "bloodthirst_heal", p, "Bloodthirst" )
  {
    // Add Field Dressing Talent, warrior heal etc.

    // Implemented as an actual heal because of spell callbacks ( for Hurricane, etc. )
    resource = RESOURCE_NONE;
    background= true;
    init();
  }
};

struct bloodthirst_buff_callback_t : public action_callback_t
{
  buff_t* buff;
  bloodthirst_heal_t* bloodthirst_heal;

  bloodthirst_buff_callback_t( warrior_t* p, buff_t* b ) :
    action_callback_t( p -> sim, p ), buff( b ),
    bloodthirst_heal( 0 )
  {
    bloodthirst_heal = new bloodthirst_heal_t( p );
  }

  virtual void trigger( action_t* a , void* /* call_data */ )
  {
    if ( buff -> check() && a -> weapon && a -> direct_dmg > 0 )
    {
      bloodthirst_heal -> base_dd_min = bloodthirst_heal -> base_dd_max = bloodthirst_heal -> effect2().base_value() / 100000.0 * a -> player -> resource_max[ RESOURCE_HEALTH ];
      bloodthirst_heal -> execute();
      buff -> decrement();
    }
  }
};


// Bloodthirst ==============================================================

struct bloodthirst_t : public warrior_attack_t
{
  bloodthirst_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "bloodthirst", "Bloodthirst", p )
  {
    check_spec( TREE_FURY );

    parse_options( NULL, options_str );

    // Include the weapon so we benefit from racials
    weapon             = &( player -> main_hand_weapon );
    weapon_multiplier  = 0;

    direct_power_mod   = effect_average( 1 ) / 100.0;
    base_dd_min        = 0.0;
    base_dd_max        = 0.0;
    base_multiplier   *= 1.0 + p -> glyphs.bloodthirst -> effect1().percent()
                             + p -> set_bonus.tier11_2pc_melee() * 0.05;
    base_crit         += p -> talents.cruelty -> effect1().percent();
  }

  virtual void execute()
  {
    warrior_attack_t::execute();

    if ( result_is_hit() )
    {
      warrior_t* p = player -> cast_warrior();

      p -> buffs_bloodthirst -> trigger( 3 );

      p -> buffs_battle_trance -> trigger();

      trigger_bloodsurge( this );
    }
  }
};

// Charge ===================================================================

struct charge_t : public warrior_attack_t
{
  int use_in_combat;

  charge_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "charge",  "Charge", p ),
    use_in_combat( 0 ) // For now it's not usable in combat by default because we can't modell the distance/movement.
  {
    option_t options[] =
    {
      { "use_in_combat", OPT_BOOL, &use_in_combat },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    cooldown -> duration += p -> talents.juggernaut -> effect3().seconds();
    cooldown -> duration += p -> glyphs.rapid_charge -> effect1().seconds();

    stancemask = STANCE_BATTLE;

    if ( p -> talents.juggernaut -> rank() || p -> talents.warbringer -> rank() )
      stancemask  = STANCE_BERSERKER | STANCE_BATTLE | STANCE_DEFENSE;
  }

  virtual void execute()
  {
    warrior_attack_t::execute();
    warrior_t* p = player -> cast_warrior();

    p -> buffs_juggernaut -> trigger();

    if ( p -> position == POSITION_RANGED_FRONT )
      p -> position = POSITION_FRONT;
    else if ( ( p -> position == POSITION_RANGED_BACK ) || ( p -> position == POSITION_MAX ) )
      p -> position = POSITION_BACK;

    p -> resource_gain( RESOURCE_RAGE,
                        effect2().resource( RESOURCE_RAGE ) + p -> talents.blitz -> effect2().resource( RESOURCE_RAGE ),
                        p -> gains_charge );
  }

  virtual bool ready()
  {
    warrior_t* p = player -> cast_warrior();

    if ( p -> in_combat )
    {
      if ( ! ( p -> talents.juggernaut -> rank() || p -> talents.warbringer -> rank() ) )
        return false;

      else if ( ! use_in_combat )
        return false;

      if ( ( p -> position == POSITION_BACK ) || ( p -> position == POSITION_FRONT ) )
      {
        return false;
      }
    }

    return warrior_attack_t::ready();
  }
};

// Cleave ===================================================================

struct cleave_t : public warrior_attack_t
{
  cleave_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "cleave", "Cleave", p )
  {
    parse_options( NULL, options_str );

    direct_power_mod = 0.45;
    base_dd_min      = 6;
    base_dd_max      = 6;

    base_multiplier *= 1.0 + p -> talents.thunderstruck -> effect1().percent();

    aoe = 1 +  p -> glyphs.cleaving -> effect1().base_value();
  }

  virtual void execute()
  {
    warrior_attack_t::execute();

    warrior_t* p = player -> cast_warrior();

    if ( result_is_hit() )
      p -> buffs_meat_cleaver -> trigger();
  }

  virtual void player_buff()
  {
    warrior_attack_t::player_buff();

    warrior_t* p = player -> cast_warrior();

    player_multiplier *= 1.0 + p -> talents.meat_cleaver -> effect1().percent() * p -> buffs_meat_cleaver -> stack();
  }

  virtual void update_ready()
  {
    warrior_t* p = player -> cast_warrior();

    cooldown -> duration = spell_id_t::cooldown();

    if ( p -> buffs_inner_rage -> up() )
      cooldown -> duration *= 1.0 + p -> buffs_inner_rage -> effect1().percent();

    warrior_attack_t::update_ready();
  }
};

// Colossus Smash ===========================================================

struct colossus_smash_t : public warrior_attack_t
{
  double armor_pen_value;

  colossus_smash_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "colossus_smash",  "Colossus Smash", p ), armor_pen_value( 0.0 )
  {
    parse_options( NULL, options_str );

    stancemask  = STANCE_BERSERKER | STANCE_BATTLE;

    armor_pen_value = base_value( E_APPLY_AURA, A_345 ) / 100.0;
  }

  virtual void execute()
  {
    warrior_attack_t::execute();

    warrior_t* p = player -> cast_warrior();

    if ( result_is_hit() )
      p -> buffs_colossus_smash -> trigger( 1, armor_pen_value );
  }
};

// Concussion Blow ==========================================================

struct concussion_blow_t : public warrior_attack_t
{
  concussion_blow_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "concussion_blow", "Concussion Blow", p )
  {
    check_talent( p -> talents.concussion_blow -> rank() );

    parse_options( NULL, options_str );

    direct_power_mod  = effect3().percent();
  }
};

// Demoralizing Shout =======================================================

struct demoralizing_shout_t : public warrior_attack_t
{
  demoralizing_shout_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "demoralizing_shout", "Demoralizing Shout", p )
  {
    parse_options( NULL, options_str );

    may_dodge  = false;
    may_parry  = false;
    may_block  = false;
    may_glance = false;
  }

  virtual void impact( player_t* t, int impact_result, double dmg )
  {
    warrior_attack_t::impact( t, impact_result, dmg );

    if ( result_is_hit( impact_result ) )
    {
      t -> debuffs.demoralizing_shout -> trigger();
    }
  }
};

// Devastate ================================================================

struct devastate_t : public warrior_attack_t
{
  devastate_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "devastate", "Devastate", p )
  {
    check_talent( p -> talents.devastate -> rank() );

    parse_options( NULL, options_str );

    base_dd_min = base_dd_max = 0;

    base_multiplier *= 1.0 + p -> talents.war_academy -> effect1().percent();

    base_crit += p -> talents.sword_and_board -> effect2().percent();
    base_crit += p -> glyphs.devastate -> effect1().percent();
  }

  virtual void execute()
  {
    warrior_attack_t::execute();

    warrior_t* p = player -> cast_warrior();

    trigger_sword_and_board( this, result );

    if ( p -> talents.impending_victory -> rank() && target -> health_percentage() <= 20 )
    {
      if ( p -> rng_impending_victory -> roll( p -> talents.impending_victory -> proc_chance() ) )
        p -> buffs_victory_rush -> trigger();
    }
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    warrior_attack_t::target_debuff( t, dmg_type );

    target_dd_adder = t -> debuffs.sunder_armor -> stack() * effect_average( 2 );
  }

  virtual void impact( player_t* t, int impact_result, double dmg )
  {
    warrior_attack_t::impact( t, impact_result, dmg );

    t -> debuffs.sunder_armor -> trigger();
  }
};

// Execute ==================================================================

struct execute_t : public warrior_attack_t
{
  execute_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "execute", "Execute", p )
  {
    parse_options( NULL, options_str );

    // Include the weapon so we benefit from racials
    weapon             = &( player -> main_hand_weapon );
    weapon_multiplier  = 0;
    base_dd_min        = 10;
    base_dd_max        = 10;

    // Rage scaling is handled in player_buff()

    stancemask = STANCE_BATTLE | STANCE_BERSERKER;
  }

  virtual void consume_resource()
  {
    warrior_t* p = player -> cast_warrior();

    // Consumes base_cost + 20
    resource_consumed = std::min( p -> resource_current[ RESOURCE_RAGE ], 20.0 + cost() );

    if ( sim -> debug )
      log_t::output( sim, "%s consumes %.1f %s for %s", p -> name(),
                     resource_consumed, util_t::resource_type_string( resource ), name() );

    player -> resource_loss( resource, resource_consumed );

    stats -> consume_resource( resource_consumed );

    if ( p -> talents.sudden_death -> ok() )
    {
      double current_rage = p -> resource_current[ RESOURCE_RAGE ];
      double sudden_death_rage = p -> talents.sudden_death -> effect1().base_value();

      if ( current_rage < sudden_death_rage )
      {
        p -> resource_gain( RESOURCE_RAGE, sudden_death_rage - current_rage, p -> gains_sudden_death ); // FIXME: Do we keep up to 10 or always at least 10?
      }
    }
  }

  virtual void execute()
  {
    warrior_attack_t::execute();
    warrior_t* p = player -> cast_warrior();

    if ( result_is_hit() && p -> rng_executioner_talent -> roll( p -> talents.executioner -> proc_chance() ) )
      p -> buffs_executioner_talent -> trigger();
  }

  virtual void player_buff()
  {
    warrior_attack_t::player_buff();
    warrior_t* p = player -> cast_warrior();

    // player_buff happens before consume_resource
    // so we can safely check here how much excess rage we will spend
    double base_consumed = cost();
    double max_consumed = std::min( p -> resource_current[ RESOURCE_RAGE ], 20.0 + base_consumed );

    // Damage scales directly with AP per rage since 4.0.1.
    // Can't be derived by parse_data() for now.
    direct_power_mod = 0.0437 * max_consumed;

    player_multiplier *= 1.0 + ( p -> buffs_lambs_to_the_slaughter -> ok() ?
                              ( p -> buffs_lambs_to_the_slaughter -> stack()
                             * p -> buffs_lambs_to_the_slaughter -> effect1().percent() ) : 0 );
  }

  virtual bool ready()
  {
    if ( target -> health_percentage() > 20 )
      return false;

    return warrior_attack_t::ready();
  }
};

// Heroic Strike ============================================================

struct heroic_strike_t : public warrior_attack_t
{
  heroic_strike_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "heroic_strike", "Heroic Strike", p )
  {
    parse_options( NULL, options_str );

    // Include the weapon so we benefit from racials
    weapon = &( player -> main_hand_weapon );
    weapon_multiplier = 0;

    base_crit        += p -> talents.incite -> effect1().percent();
    base_dd_min       = base_dd_max = 8;
    direct_power_mod  = 0.6; // Hardcoded into tooltip, 02/11/2011
    harmful           = true;
  }

  virtual double cost() const
  {
    double c = warrior_attack_t::cost();
    warrior_t* p = player -> cast_warrior();

    // Needs testing
    if ( p -> set_bonus.tier13_2pc_melee() && p -> buffs_inner_rage -> check() )
    {
      c -= 10.0;
    }

    return c;
  }

  virtual void consume_resource()
  {
    warrior_attack_t::consume_resource();
    warrior_t* p = player -> cast_warrior();

    // Needs testing
    if ( p -> set_bonus.tier13_2pc_melee() )
      p -> buffs_inner_rage -> up();
  }

  virtual void execute()
  {
    warrior_attack_t::execute();
    warrior_t* p = player -> cast_warrior();

    if ( p -> buffs_incite -> check() )
      p -> buffs_incite -> expire();

    else if ( result == RESULT_CRIT )
      p -> buffs_incite -> trigger();

  }

  virtual void player_buff()
  {
    warrior_attack_t::player_buff();
    warrior_t* p = player -> cast_warrior();

    if ( p -> buffs_incite -> up() )
      player_crit += 1.0;
  }

  virtual void update_ready()
  {
    warrior_t* p = player -> cast_warrior();

    cooldown -> duration = spell_id_t::cooldown();

    if ( p -> buffs_inner_rage -> up() )
      cooldown -> duration *= 1.0 + p -> buffs_inner_rage -> effect1().percent();

    warrior_attack_t::update_ready();
  }
};

// Mortal Strike ============================================================

struct mortal_strike_t : public warrior_attack_t
{
  double additive_multipliers;

  mortal_strike_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "mortal_strike", "Mortal Strike", p ),
    additive_multipliers( 0 )
  {
    check_spec( TREE_ARMS );

    parse_options( NULL, options_str );

    additive_multipliers = p -> glyphs.mortal_strike -> effect1().percent()
                           + p -> set_bonus.tier11_2pc_melee() * 0.05
                           + p -> talents.war_academy -> effect1().percent();
    crit_bonus_multiplier *= 1.0 + p -> talents.impale -> effect1().percent();
    base_crit             += p -> talents.cruelty -> effect1().percent();
  }

  virtual void execute()
  {
    warrior_attack_t::execute();

    warrior_t* p = player -> cast_warrior();

    if ( result_is_hit() )
    {
      p -> buffs_lambs_to_the_slaughter -> trigger();
      p -> buffs_battle_trance -> trigger();
      p -> buffs_juggernaut -> expire();

      if ( result == RESULT_CRIT && p -> rng_wrecking_crew -> roll( p -> talents.wrecking_crew -> proc_chance() ) )
      {
        double value = p -> talents.wrecking_crew -> rank() * 0.05;
        p -> buffs_wrecking_crew -> trigger( 1, value );
      }

      if ( p -> talents.lambs_to_the_slaughter -> rank() && p -> dots_rend -> ticking )
        p -> dots_rend -> refresh_duration();

      trigger_tier12_4pc_melee( this );

      if ( p -> set_bonus.tier13_4pc_melee() && sim -> roll( 0.13 ) )
      {
        p -> buffs_colossus_smash -> trigger();
        p -> procs_tier13_4pc_melee -> occur();
      }
    }
  }

  virtual void player_buff()
  {
    warrior_attack_t::player_buff();

    warrior_t* p = player -> cast_warrior();

    if ( p -> buffs_juggernaut -> up() )
      player_crit += p -> buffs_juggernaut -> effect1().percent();

    player_multiplier *= 1.0 + ( p -> buffs_lambs_to_the_slaughter -> ok() ?
                              ( p -> buffs_lambs_to_the_slaughter -> stack()
                             * p -> buffs_lambs_to_the_slaughter -> effect1().percent() ) : 0 )
                             + additive_multipliers;
  }
};

// Fiery Attack Attack ======================================================

struct fiery_attack_t : public warrior_attack_t
{
  fiery_attack_t( warrior_t* player ) :
    warrior_attack_t( "fiery_attack", 99237, player )
  {
    background = true;
    repeating = false;
    proc = true;
    normalize_weapon_speed=true;
  }
};

// Overpower ================================================================

struct overpower_t : public warrior_attack_t
{
  overpower_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "overpower", "Overpower", p )
  {
    parse_options( NULL, options_str );

    may_dodge  = false;
    may_parry  = false;
    may_block  = false; // The Overpower cannot be blocked, dodged or parried.

    base_crit += p -> talents.taste_for_blood -> effect2().percent();
    crit_bonus_multiplier *= 1.0 + p -> talents.impale -> effect1().percent();
    base_multiplier       *= 1.0 + p -> glyphs.overpower -> effect1().percent();

    stancemask = STANCE_BATTLE;
  }

  virtual void execute()
  {
    warrior_t* p = player -> cast_warrior();

    // Track some information on what got us the overpower
    // Talents or lack of expertise
    p -> buffs_overpower -> up();
    p -> buffs_taste_for_blood -> up();
    warrior_attack_t::execute();
    p -> buffs_overpower -> expire();
    p -> buffs_taste_for_blood -> expire();

    // FIXME: The wording indicates it triggers even if you miss.
    p -> buffs_tier11_4pc_melee -> trigger();
  }

  virtual void player_buff()
  {
    warrior_attack_t::player_buff();
    warrior_t* p = player -> cast_warrior();

    player_multiplier *= 1.0 + ( p -> buffs_lambs_to_the_slaughter -> ok() ?
                              ( p -> buffs_lambs_to_the_slaughter -> stack()
                              * p -> buffs_lambs_to_the_slaughter -> effect1().percent() ) : 0 );
  }

  virtual bool ready()
  {
    warrior_t* p = player -> cast_warrior();

    if ( ! ( p -> buffs_overpower -> check() || p -> buffs_taste_for_blood -> check() ) )
      return false;

    return warrior_attack_t::ready();
  }
};

// Pummel ===================================================================

struct pummel_t : public warrior_attack_t
{
  pummel_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "pummel", "Pummel", p )
  {
    parse_options( NULL, options_str );

    base_cost *= 1.0 + p -> talents.drums_of_war -> effect1().percent();

    may_miss = may_resist = may_glance = may_block = may_dodge = may_parry = may_crit = false;
  }

  virtual bool ready()
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return warrior_attack_t::ready();
  }
};

// Raging Blow ==============================================================

struct raging_blow_attack_t : public warrior_attack_t
{
  raging_blow_attack_t( warrior_t* p, const char* name ) :
    warrior_attack_t( name, 96103, p )
  {
    may_miss = may_dodge = may_parry = false;
    background = true;
    base_multiplier *= 1.0 + p -> talents.war_academy -> effect1().percent();
    base_crit += p -> glyphs.raging_blow -> effect1().percent();
  }

  virtual void player_buff()
  {
    warrior_attack_t::player_buff();
    warrior_t* p = player -> cast_warrior();

    player_multiplier *= 1.0 + p -> composite_mastery() *
                               p -> mastery.unshackled_fury -> effect3().base_value() / 10000.0;
  }
};

struct raging_blow_t : public warrior_attack_t
{
  raging_blow_attack_t* mh_attack;
  raging_blow_attack_t* oh_attack;

  raging_blow_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "raging_blow", "Raging Blow", p ),
    mh_attack( 0 ), oh_attack( 0 )
  {
    check_talent( p -> talents.raging_blow -> rank() );

    // Parent attack is only to determine miss/dodge/parry
    base_dd_min = base_dd_max = 0;
    weapon_multiplier = direct_power_mod = 0;
    may_crit = false;
    weapon = &( p -> main_hand_weapon ); // Include the weapon for racial expertise

    parse_options( NULL, options_str );

    stancemask = STANCE_BERSERKER;

    mh_attack = new raging_blow_attack_t( p, "raging_blow_mh" );
    mh_attack -> weapon = &( p -> main_hand_weapon );
    add_child( mh_attack );

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      oh_attack = new raging_blow_attack_t( p, "raging_blow_oh" );
      oh_attack -> weapon = &( p -> off_hand_weapon );
      add_child( oh_attack );
    }
  }

  virtual void execute()
  {
    attack_t::execute();
    warrior_t* p = player -> cast_warrior();

    if ( result_is_hit() )
    {
      mh_attack -> execute();
      if ( oh_attack )
      {
        oh_attack -> execute();
      }

      // PTR - is this triggered per attack or once
      if ( p -> set_bonus.tier13_4pc_melee() && sim -> roll( 0.13 ) )
      {
        p -> buffs_colossus_smash -> trigger();
        p -> procs_tier13_4pc_melee -> occur();
      }
    }
    p -> buffs_tier11_4pc_melee -> trigger();

    trigger_tier12_4pc_melee( this );
  }

  virtual bool ready()
  {
    warrior_t* p = player -> cast_warrior();

    if ( ! ( p -> buffs_berserker_rage -> check() ||
             p -> buffs_death_wish     -> check() ||
             p -> buffs_enrage         -> check() ||
             p -> buffs.unholy_frenzy  -> check() ) )
      return false;

    return warrior_attack_t::ready();
  }
};

// Rend =====================================================================

struct rend_dot_t : public warrior_attack_t
{
  rend_dot_t( warrior_t* p ) :
    warrior_attack_t( "rend_dot", 94009, p )
  {
    background = true;
    tick_may_crit          = true;
    may_crit               = false;
    tick_zero              = true;

    dot = p ->  get_dot( "rend" );

    weapon                 = &( p -> main_hand_weapon );
    normalize_weapon_speed = false;
    base_td_init = base_td;

    base_multiplier       *= 1.0 + p -> talents.thunderstruck -> effect1().percent();

  }

  virtual double calculate_direct_damage()
  {
    // Rend doesn't actually hit with the weapon, but ticks on application
    return 0.0;
  }

  virtual void execute()
  {
    base_td = base_td_init;
    if ( weapon )
      base_td += ( sim -> range( weapon -> min_dmg, weapon -> max_dmg ) + weapon -> swing_time * weapon_power_mod * total_attack_power() ) * 0.25;

    warrior_attack_t::execute();

    if ( result_is_hit() )
      trigger_blood_frenzy( this );
  }

  virtual void tick( dot_t* d )
  {
    warrior_attack_t::tick( d );
    warrior_t* p = player -> cast_warrior();

    p -> buffs_taste_for_blood -> trigger();
  }
};

struct rend_t : public warrior_attack_t
{
  rend_dot_t* rend_dot;

  rend_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "rend", "Rend", p ),
    rend_dot( 0 )
  {
    parse_options( NULL, options_str );

    harmful = false;

    stancemask             = STANCE_BATTLE | STANCE_DEFENSE;

    rend_dot = new rend_dot_t( p );
    add_child( rend_dot );
  }

  virtual void execute()
  {
    warrior_attack_t::execute();

    if ( rend_dot )
      rend_dot -> execute();
  }
};

// Revenge ==================================================================

struct revenge_t : public warrior_attack_t
{
  stats_t* absorb_stats;

  revenge_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "revenge", "Revenge", p ), absorb_stats( 0 )
  {
    parse_options( NULL, options_str );

    base_multiplier  *= 1.0 + p -> talents.improved_revenge -> effect2().percent()
                            + p -> glyphs.revenge -> effect1().percent();
    direct_power_mod = extra_coeff();
    stancemask = STANCE_DEFENSE;

    if ( p -> talents.improved_revenge -> rank() )
    {
      aoe = 1;
      base_add_multiplier = p -> talents.improved_revenge -> rank() * 0.50;
    }

    // Needs testing
    if ( p -> set_bonus.tier13_2pc_tank() )
    {
      absorb_stats = p -> get_stats( name_str + "_tier13_2pc" );
      absorb_stats -> type = STATS_ABSORB;
    }
  }

  virtual void execute()
  {
    warrior_attack_t::execute();
    warrior_t* p = player -> cast_warrior();

    p -> buffs_revenge -> expire();

    trigger_sword_and_board( this, result );
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    warrior_attack_t::impact( t, impact_result, travel_dmg );
    warrior_t* p = player -> cast_warrior();

    // Needs testing
    if ( absorb_stats )
    {
      if ( result_is_hit( impact_result ) )
      {
        double amount = 0.20 * travel_dmg;
        p -> buffs_tier13_2pc_tank -> trigger(1, amount);
        absorb_stats -> add_result( amount, amount, STATS_ABSORB, impact_result );
        absorb_stats -> add_execute( 0 );
      }
    }
  }

  virtual bool ready()
  {
    warrior_t* p = player -> cast_warrior();

    if ( ! p -> buffs_revenge -> check() )
      return false;

    return warrior_attack_t::ready();
  }
};

// Shattering Throw =========================================================

// TO-DO: Only a shell at the moment. Needs testing for damage etc.
struct shattering_throw_t : public warrior_attack_t
{
  shattering_throw_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "shattering_throw", "Shattering Throw", p )
  {
    parse_options( NULL, options_str );

    direct_power_mod = extra_coeff();
    stancemask = STANCE_BATTLE;
  }

  virtual void execute()
  {
    warrior_attack_t::execute();

    if ( result_is_hit() )
      target -> debuffs.shattering_throw -> trigger();
  }

  virtual bool ready()
  {
    if ( target -> debuffs.shattering_throw -> check() )
      return false;

    return warrior_attack_t::ready();
  }
};

// Shield Bash ==============================================================

struct shield_bash_t : public warrior_attack_t
{
  shield_bash_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "shield_bash", "Shield Bash", p )
  {
    parse_options( NULL, options_str );

    base_cost *= 1.0 + p -> talents.drums_of_war -> effect1().percent();

    may_miss = may_resist = may_glance = may_block = may_dodge = may_parry = may_crit = false;

    stancemask = STANCE_DEFENSE | STANCE_BATTLE;
  }

  virtual void execute()
  {
    warrior_attack_t::execute();

    // Implement lock school

    if ( target -> debuffs.casting -> check() )
      target -> interrupt();
  }

  virtual bool ready()
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return warrior_attack_t::ready();
  }
};

// Shield Slam ==============================================================

struct shield_slam_t : public warrior_attack_t
{
  shield_slam_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "shield_slam", "Shield Slam", p )
  {
    check_spec( TREE_PROTECTION );

    parse_options( NULL, options_str );

    base_crit        += p -> talents.cruelty -> effect1().percent();
    base_multiplier  *= 1.0  + p -> glyphs.shield_slam -> effect1().percent();

    stats -> add_child( player -> get_stats( "shield_slam_combust" ) );
  }

  virtual void player_buff()
  {
    warrior_attack_t::player_buff();

    warrior_t* p = player -> cast_warrior();

    if ( p -> buffs_shield_block -> up() )
      player_multiplier *= 1.0 + p -> talents.heavy_repercussions -> effect1().percent();
  }

  virtual void execute()
  {
    warrior_attack_t::execute();
    warrior_t* p = player -> cast_warrior();

    p -> buffs_sword_and_board -> expire();

    if ( result_is_hit() )
      p -> buffs_battle_trance -> trigger();
  }

  virtual double cost() const
  {
    warrior_t* p = player -> cast_warrior();

    if ( p -> buffs_sword_and_board -> check() )
      return 0;

    return warrior_attack_t::cost();
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    warrior_attack_t::impact( t, impact_result, travel_dmg );
    trigger_tier12_2pc_tank( this, direct_dmg );
  }
};

// Shockwave ================================================================

struct shockwave_t : public warrior_attack_t
{
  shockwave_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "shockwave", "Shockwave", p )
  {
    check_talent( p -> talents.shockwave -> rank() );

    parse_options( NULL, options_str );

    direct_power_mod  = effect3().percent();
    may_dodge         = false;
    may_parry         = false;
    may_block         = false;
    cooldown -> duration += p -> glyphs.shockwave -> effect1().seconds();
  }

  virtual void execute()
  {
    warrior_attack_t::execute();
    warrior_t* p = player -> cast_warrior();

    p -> buffs_thunderstruck -> expire();
  }

  virtual void player_buff()
  {
    warrior_attack_t::player_buff();
    warrior_t* p = player -> cast_warrior();

    player_multiplier *= 1.0 + p -> buffs_thunderstruck -> stack() *
                               p -> talents.thunderstruck -> effect2().percent();
  }
};

// Slam =====================================================================

struct slam_attack_t : public warrior_attack_t
{
  double additive_multipliers;

  slam_attack_t( warrior_t* p, const char* name ) :
    warrior_attack_t( name, 50782, p ),
    additive_multipliers( 0 )
  {
    base_cost = 0;
    may_miss = may_dodge = may_parry = false;
    background = true;

    base_crit             += p -> glyphs.slam -> effect1().percent();
    crit_bonus_multiplier *= 1.0 + p -> talents.impale -> effect1().percent();
    base_execute_time     += p -> talents.improved_slam -> effect1().seconds();

    additive_multipliers = p -> talents.improved_slam -> effect2().percent()
                           + p -> talents.war_academy -> effect1().percent();
  }

  virtual void player_buff()
  {
    warrior_attack_t::player_buff();
    warrior_t* p = player -> cast_warrior();

    if ( p -> buffs_juggernaut -> up() )
      player_crit += p -> buffs_juggernaut -> effect1().percent();

    if ( p -> buffs_bloodsurge -> up() )
      player_multiplier *= 1.0 + p -> talents.bloodsurge -> effect1().percent();

    player_multiplier *= 1.0 + ( p -> buffs_lambs_to_the_slaughter -> ok() ?
                              ( p -> buffs_lambs_to_the_slaughter -> stack()
                             * p -> buffs_lambs_to_the_slaughter -> effect1().percent() ) : 0 )
                             + additive_multipliers;
  }
};

struct slam_t : public warrior_attack_t
{
  attack_t* mh_attack;
  attack_t* oh_attack;

  slam_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "slam", "Slam", p ),
    mh_attack( 0 ), oh_attack( 0 )
  {
    parse_options( NULL, options_str );

    may_crit = false;

    // Ensure we include racial expertise
    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;

    mh_attack = new slam_attack_t( p, "slam_mh" );
    mh_attack -> weapon = &( p -> main_hand_weapon );
    add_child( mh_attack );

    if ( p -> talents.single_minded_fury -> ok() && p -> off_hand_weapon.type != WEAPON_NONE )
    {
      oh_attack = new slam_attack_t( p, "slam_oh" );
      oh_attack -> weapon = &( p -> off_hand_weapon );
      add_child( oh_attack );
    }
  }

  virtual double haste() const { return 1.0; }
  virtual double swing_haste() const { return 1.0; }

  virtual double cost() const
  {
    warrior_t* p = player -> cast_warrior();

    if ( p -> buffs_bloodsurge -> check() )
      return 0;

    return warrior_attack_t::cost();
  }

  virtual double execute_time() const
  {
    warrior_t* p = player -> cast_warrior();

    if ( p -> buffs_bloodsurge -> check() )
      return 0.0;

    return warrior_attack_t::execute_time();
  }

  virtual void execute()
  {
    attack_t::execute();
    warrior_t* p = player -> cast_warrior();

    if ( result_is_hit() )
    {
      mh_attack -> execute();

      p -> buffs_juggernaut -> expire();

      if ( oh_attack )
      {
        oh_attack -> execute();

        if ( p -> bugs ) // http://elitistjerks.com/f81/t106912-fury_dps_4_0_cataclysm/p19/#post1875264
          p -> buffs_battle_trance -> expire();
      }
    }

    p -> buffs_bloodsurge -> decrement();
  }
};

// Sunder Armor =============================================================

struct sunder_armor_t : public warrior_attack_t
{
  sunder_armor_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "sunder_armor", 7386, p )
  {
    parse_options( NULL, options_str );

    base_cost *= 1.0 + p -> glyphs.furious_sundering -> effect1().percent();

    // TODO: Glyph of Sunder armor applies affect to nearby target
  }

  virtual void impact( player_t* t, int impact_result, double dmg )
  {
    warrior_attack_t::impact( t, impact_result, dmg );

    if ( result_is_hit( impact_result ) )
    {
      t -> debuffs.sunder_armor -> trigger();
    }
  }
};

// Thunder Clap =============================================================

struct thunder_clap_t : public warrior_attack_t
{
  thunder_clap_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "thunder_clap", "Thunder Clap", p )
  {
    parse_options( NULL, options_str );

    aoe               = -1;
    may_dodge         = false;
    may_parry         = false;
    may_block         = false;
    direct_power_mod  = extra_coeff();
    base_multiplier  *= 1.0 + p -> talents.thunderstruck -> effect1().percent();
    base_crit        += p -> talents.incite -> effect1().percent();
    base_cost        += p -> glyphs.resonating_power -> effect1().resource( RESOURCE_RAGE );
  }

  virtual void execute()
  {
    warrior_attack_t::execute();
    warrior_t* p = player -> cast_warrior();

    p -> buffs_thunderstruck -> trigger();

    if ( p -> talents.blood_and_thunder -> rank() && p -> dots_rend && p -> dots_rend -> ticking )
      p -> dots_rend -> refresh_duration();
  }
};

// Whirlwind ================================================================

struct whirlwind_t : public warrior_attack_t
{
  whirlwind_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "whirlwind", "Whirlwind", p )
  {
    parse_options( NULL, options_str );

    aoe               = -1;
    stancemask        = STANCE_BERSERKER;
  }

  virtual void consume_resource() { }

  virtual void execute()
  {
    warrior_t* p = player -> cast_warrior();

    // MH hit
    weapon = &( p -> main_hand_weapon );
    warrior_attack_t::execute();

    if ( result_is_hit() )
      p -> buffs_meat_cleaver -> trigger();

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      weapon = &( p -> off_hand_weapon );
      warrior_attack_t::execute();
      if ( result_is_hit() )
        p -> buffs_meat_cleaver -> trigger();
    }

    warrior_attack_t::consume_resource();
  }

  virtual void player_buff()
  {
    warrior_attack_t::player_buff();
    warrior_t* p = player -> cast_warrior();

    if ( p -> buffs_meat_cleaver -> up() )
    {
      // Caution: additive
      player_multiplier *= 1.0 + p -> talents.meat_cleaver -> rank() * 0.05 * p -> buffs_meat_cleaver -> stack();
    }
  }
};

// Victory Rush =============================================================

struct victory_rush_t : public warrior_attack_t
{
  victory_rush_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "victory_rush", "Victory Rush", p )
  {
    parse_options( NULL, options_str );

    base_multiplier *= 1.0 + p -> talents.war_academy -> effect1().percent();
  }

  virtual bool ready()
  {
    warrior_t* p = player -> cast_warrior();

    if ( ! p -> buffs_victory_rush -> check() )
      return false;

    return warrior_attack_t::ready();
  }
};

// ==========================================================================
// Warrior Spells
// ==========================================================================

struct warrior_spell_t : public spell_t
{
  int stancemask;
  warrior_spell_t( const char* n, warrior_t* p, const school_type s=SCHOOL_PHYSICAL, int t=TREE_NONE ) :
    spell_t( n, p, RESOURCE_RAGE, s, t ),
    stancemask( STANCE_BATTLE|STANCE_BERSERKER|STANCE_DEFENSE )
  {
    _init_warrior_spell_t();
  }

  warrior_spell_t( const char* n, uint32_t id, warrior_t* p, int t=TREE_NONE ) :
    spell_t( n, id, p, t ),
    stancemask( STANCE_BATTLE|STANCE_BERSERKER|STANCE_DEFENSE )
  {
    _init_warrior_spell_t();
  }

  warrior_spell_t( const char* n, const char* s_name, warrior_t* p, int t=TREE_NONE ) :
    spell_t( n, s_name, p, t ),
    stancemask( STANCE_BATTLE|STANCE_BERSERKER|STANCE_DEFENSE )
  {
    _init_warrior_spell_t();
  }

  void _init_warrior_spell_t()
  {
  }

  virtual double gcd() const;
  virtual bool   ready();
};

// warrior_spell_t::gcd =====================================================

double warrior_spell_t::gcd() const
{
  // Unaffected by haste
  return trigger_gcd;
}

// warrior_spell_t::ready() =================================================

bool warrior_spell_t::ready()
{
  warrior_t* p = player -> cast_warrior();

  // Attack vailable in current stance?
  if ( ( stancemask & p -> active_stance ) == 0 )
    return false;

  return spell_t::ready();
}

// Battle Shout =============================================================

struct battle_shout_t : public warrior_spell_t
{
  double rage_gain;
  double burning_rage_value;

  battle_shout_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "battle_shout", "Battle Shout", p ), burning_rage_value( 0.0 )
  {
    parse_options( NULL, options_str );

    harmful   = false;

    rage_gain = p -> dbc.spell( effect3().trigger_spell_id() ) -> effect1().resource( RESOURCE_RAGE ) + p -> talents.booming_voice -> effect2().resource( RESOURCE_RAGE );
    cooldown = player -> get_cooldown( "shout" );
    cooldown -> duration = 10 + spell_id_t::cooldown() + p -> talents.booming_voice -> effect1().seconds();

    if ( p -> set_bonus.tier12_2pc_melee() )
    {
      uint32_t id = p -> sets -> set( SET_T12_2PC_MELEE ) -> effect_trigger_spell( 1 );
      spell_data_t* s = spell_data_t::find( id, "Burning Rage", p -> dbc.ptr );
      burning_rage_value = s -> effect1().percent();
    }
  }

  virtual void execute()
  {
    warrior_spell_t::execute();

    warrior_t* p = player -> cast_warrior();

    if ( ! sim -> overrides.battle_shout )
    {
      for ( player_t* q = sim -> player_list; q; q = q -> next )
      {
        q -> buffs.battle_shout -> buff_duration = duration() + p -> glyphs.battle -> effect1().seconds();
        q -> buffs.battle_shout -> trigger( 1, effect_average( 1 ) );
      }

    }

    p -> resource_gain( RESOURCE_RAGE, rage_gain , p -> gains_battle_shout );

    p -> buffs_tier12_2pc_melee -> trigger( 1, burning_rage_value );
  }
};

// Commanding Shout =============================================================

struct commanding_shout_t : public warrior_spell_t
{
  double rage_gain;
  double burning_rage_value;

  commanding_shout_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "commanding_shout", "Commanding Shout", p ), burning_rage_value( 0.0 )
  {
    parse_options( NULL, options_str );

    harmful   = false;

    rage_gain = 20 + p -> talents.booming_voice -> effect2().resource( RESOURCE_RAGE );

    cooldown = player -> get_cooldown( "shout" );
    cooldown -> duration = spell_id_t::cooldown() + p -> talents.booming_voice -> effect1().seconds();

    if ( p -> set_bonus.tier12_2pc_melee() )
    {
      uint32_t id = p -> sets -> set( SET_T12_2PC_MELEE ) -> effect_trigger_spell( 1 );
      spell_data_t* s = spell_data_t::find( id, "Burning Rage", p -> dbc.ptr );
      burning_rage_value = s -> effect1().percent();
    }
  }

  virtual void execute()
  {
    warrior_spell_t::execute();

    warrior_t* p = player -> cast_warrior();
    for ( player_t* q = sim -> player_list; q; q = q -> next )
    {
      q -> buffs.commanding_shout -> buff_duration = 120 /* include commanding glyph */;
      q -> buffs.commanding_shout -> trigger( 1, effect_average( 1 ) );
    }

    p -> resource_gain( RESOURCE_RAGE, rage_gain , p -> gains_commanding_shout );

    p -> buffs_tier12_2pc_melee -> trigger( 1, burning_rage_value );
  }
};

// Berserker Rage ===========================================================

struct berserker_rage_t : public warrior_spell_t
{
  berserker_rage_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "berserker_rage", "Berserker Rage", p )
  {
    parse_options( NULL, options_str );

    harmful = false;

    if ( p -> talents.intensify_rage -> ok() )
      cooldown -> duration *= ( 1.0 + p -> talents.intensify_rage -> effect1().percent() );
  }

  virtual void execute()
  {
    warrior_spell_t::execute();

    warrior_t* p = player -> cast_warrior();

    if ( p -> glyphs.berserker_rage -> ok() )
    {
      double ragegain = 5.0;
      ragegain *= 1.0 + p -> composite_mastery() * p -> mastery.unshackled_fury -> effect3().percent() / 100.0;
      p -> resource_gain( RESOURCE_RAGE, ragegain, p -> gains_berserker_rage );
    }

    p -> buffs_berserker_rage -> trigger();
  }
};

// Deadly Calm ==============================================================

struct deadly_calm_t : public warrior_spell_t
{
  deadly_calm_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "deadly_calm", 85730, p )
  {
    check_talent( p -> talents.deadly_calm -> rank() );

    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
    warrior_t* p = player -> cast_warrior();

    p -> buffs_deadly_calm -> trigger();
  }

  virtual bool ready()
  {
    warrior_t* p = player -> cast_warrior();

    if ( p -> buffs_inner_rage -> check() )
      return false;

    if ( p -> buffs_recklessness -> check() )
      return false;

    return warrior_spell_t::ready();
  }
};

// Death Wish ===============================================================

struct death_wish_t : public warrior_spell_t
{
  double enrage_bonus;

  death_wish_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "death_wish", "Death Wish", p ),
    enrage_bonus( 0 )
  {
    check_talent( p -> talents.death_wish -> rank() );

    parse_options( NULL, options_str );

    harmful = false;

    if ( p -> talents.intensify_rage -> ok() )
      cooldown -> duration *= ( 1.0 + p -> talents.intensify_rage -> effect1().percent() );
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
    warrior_t* p = player -> cast_warrior();

    enrage_bonus = p -> talents.death_wish -> effect1().percent();
    enrage_bonus *= 1.0 + p -> composite_mastery() * p -> mastery.unshackled_fury -> effect3().percent() / 100.0;

    p -> buffs_death_wish -> trigger( 1, enrage_bonus );
    p -> buffs_enrage -> expire();
  }
};

// Inner Rage ===============================================================

struct inner_rage_t : public warrior_spell_t
{
  inner_rage_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "inner_rage", "Inner Rage", p )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
    warrior_t* p = player -> cast_warrior();

    p -> buffs_inner_rage -> trigger();
  }
};

// Recklessness =============================================================

struct recklessness_t : public warrior_spell_t
{
  recklessness_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "recklessness", "Recklessness", p )
  {
    parse_options( NULL, options_str );

    harmful = false;

    cooldown -> duration *= 1.0 + p -> talents.intensify_rage -> effect1().percent();
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
    warrior_t* p = player -> cast_warrior();

    p -> buffs_recklessness -> trigger( 3 );
  }

  virtual bool ready()
  {
    warrior_t* p = player -> cast_warrior();

    if ( p -> buffs_deadly_calm -> check() )
      return false;

    return warrior_spell_t::ready();
  }
};

// Retaliation ==============================================================

struct retaliation_t : public warrior_spell_t
{
  retaliation_t( warrior_t* p,  const std::string& options_str ) : 
    warrior_spell_t( "retaliation", 20230, p )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
     warrior_t* p = player -> cast_warrior();

     p -> buffs_retaliation -> trigger( 20 );
  }
};

// Shield Block =============================================================

struct shield_block_buff_t : public buff_t
{
  shield_block_buff_t( warrior_t* p ) :
    buff_t( p, 2565, "shield_block" )
  { }

  virtual void expire()
  {
    buff_t::expire();
    warrior_t* p = player -> cast_warrior();

    p -> buffs_tier12_4pc_tank -> trigger( 1, p -> buffs_tier12_4pc_tank -> effect1().percent() );
  }
};
struct shield_block_t : public warrior_spell_t
{
  shield_block_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "shield_block", "Shield Block", p )
  {
    parse_options( NULL, options_str );

    harmful = false;

    if ( p -> talents.shield_mastery -> ok() )
      cooldown -> duration += p -> talents.shield_mastery -> effect1().seconds();
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
    warrior_t* p = player -> cast_warrior();

    p -> buffs_shield_block -> trigger();
  }
};

// Stance ===================================================================

struct stance_t : public warrior_spell_t
{
  int switch_to_stance;
  std::string stance_str;

  stance_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "stance", p ),
    switch_to_stance( 0 ), stance_str( "" )
  {
    option_t options[] =
    {
      { "choose",  OPT_STRING, &stance_str     },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    if ( ! stance_str.empty() )
    {
      if ( stance_str == "battle" )
        switch_to_stance = STANCE_BATTLE;
      else if ( stance_str == "berserker" || stance_str == "zerker" )
        switch_to_stance = STANCE_BERSERKER;
      else if ( stance_str == "def" || stance_str == "defensive" )
        switch_to_stance = STANCE_DEFENSE;
    }
    else
    {
      // Default to Battle Stance
      switch_to_stance = STANCE_BATTLE;
    }

    harmful = false;
    base_cost   = 0;
    trigger_gcd = 0;
    cooldown -> duration = 1.0;
    resource    = RESOURCE_RAGE;
  }

  virtual void execute()
  {
    warrior_t* p = player -> cast_warrior();

    switch ( p -> active_stance )
    {
    case STANCE_BATTLE:     p -> buffs_battle_stance    -> expire(); break;
    case STANCE_BERSERKER:  p -> buffs_berserker_stance -> expire(); break;
    case STANCE_DEFENSE:    p -> buffs_defensive_stance -> expire(); break;
    }
    p -> active_stance = switch_to_stance;

    switch ( p -> active_stance )
    {
    case STANCE_BATTLE:     p -> buffs_battle_stance    -> trigger(); break;
    case STANCE_BERSERKER:  p -> buffs_berserker_stance -> trigger(); break;
    case STANCE_DEFENSE:    p -> buffs_defensive_stance -> trigger(); break;
    }

    consume_resource();

    update_ready();
  }

  virtual double cost() const
  {
    warrior_t* p = player -> cast_warrior();

    double c = p -> resource_current [ RESOURCE_RAGE ];

    c -= 25.0; // Stance Mastery

    if ( p -> talents.tactical_mastery -> ok() )
      c -= p -> talents.tactical_mastery -> effect1().base_value();

    if ( c < 0 ) c = 0;

    return c;
  }

  virtual bool ready()
  {
    warrior_t* p = player -> cast_warrior();

    if ( p -> active_stance == switch_to_stance )
      return false;

    return warrior_spell_t::ready();
  }
};

// Sweeping Strikes =========================================================

struct sweeping_strikes_t : public warrior_spell_t
{
  sweeping_strikes_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "sweeping_strikes", 12328, p )
  {
    check_talent( p -> talents.sweeping_strikes -> rank() );

    parse_options( NULL, options_str );

    harmful = false;

    base_cost *= 1.0 + p -> glyphs.sweeping_strikes -> effect1().percent();

    stancemask = STANCE_BERSERKER | STANCE_BATTLE;
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
    warrior_t* p = player -> cast_warrior();

    p -> buffs_sweeping_strikes -> trigger();
  }
};

// Last Stand ===============================================================

struct last_stand_t : public warrior_spell_t
{
  last_stand_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "last_stand", 12975, p )
  {
    check_talent( p -> talents.last_stand -> rank() );

    harmful = false;

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
    warrior_t* p = player -> cast_warrior();

    p -> buffs_last_stand -> trigger();
  }
};

struct buff_last_stand_t : public buff_t
{
  int health_gain;

  buff_last_stand_t( warrior_t* p, const uint32_t id, const std::string& n ) :
    buff_t( p, id, n ), health_gain( 0 )
  { }

  virtual bool trigger( int stacks, double value, double chance )
  {
    health_gain = ( int ) floor( player -> resource_max[ RESOURCE_HEALTH ] * 0.3 );
    player -> stat_gain( STAT_MAX_HEALTH, health_gain );

    return buff_t::trigger( stacks, value, chance );
  }

  virtual void expire()
  {
    player -> stat_loss( STAT_MAX_HEALTH, health_gain );

    buff_t::expire();
  }
};

} // ANONYMOUS NAMESPACE ====================================================

// ==========================================================================
// Warrior Character Definition
// ==========================================================================

// warrior_t::create_action  ================================================

action_t* warrior_t::create_action( const std::string& name,
                                    const std::string& options_str )
{
  if ( name == "auto_attack"        ) return new auto_attack_t        ( this, options_str );
  if ( name == "battle_shout"       ) return new battle_shout_t       ( this, options_str );
  if ( name == "berserker_rage"     ) return new berserker_rage_t     ( this, options_str );
  if ( name == "bladestorm"         ) return new bladestorm_t         ( this, options_str );
  if ( name == "bloodthirst"        ) return new bloodthirst_t        ( this, options_str );
  if ( name == "charge"             ) return new charge_t             ( this, options_str );
  if ( name == "cleave"             ) return new cleave_t             ( this, options_str );
  if ( name == "colossus_smash"     ) return new colossus_smash_t     ( this, options_str );
  if ( name == "concussion_blow"    ) return new concussion_blow_t    ( this, options_str );
  if ( name == "deadly_calm"        ) return new deadly_calm_t        ( this, options_str );
  if ( name == "death_wish"         ) return new death_wish_t         ( this, options_str );
  if ( name == "demoralizing_shout" ) return new demoralizing_shout_t ( this, options_str );
  if ( name == "devastate"          ) return new devastate_t          ( this, options_str );
  if ( name == "execute"            ) return new execute_t            ( this, options_str );
  if ( name == "heroic_strike"      ) return new heroic_strike_t      ( this, options_str );
  if ( name == "inner_rage"         ) return new inner_rage_t         ( this, options_str );
  if ( name == "last_stand"         ) return new last_stand_t         ( this, options_str );
  if ( name == "mortal_strike"      ) return new mortal_strike_t      ( this, options_str );
  if ( name == "overpower"          ) return new overpower_t          ( this, options_str );
  if ( name == "pummel"             ) return new pummel_t             ( this, options_str );
  if ( name == "raging_blow"        ) return new raging_blow_t        ( this, options_str );
  if ( name == "recklessness"       ) return new recklessness_t       ( this, options_str );
  if ( name == "rend"               ) return new rend_t               ( this, options_str );
  if ( name == "retaliation"        ) return new retaliation_t        ( this, options_str );
  if ( name == "revenge"            ) return new revenge_t            ( this, options_str );
  if ( name == "shattering_throw"   ) return new shattering_throw_t   ( this, options_str );
  if ( name == "shield_bash"        ) return new shield_bash_t        ( this, options_str );
  if ( name == "shield_block"       ) return new shield_block_t       ( this, options_str );
  if ( name == "shield_slam"        ) return new shield_slam_t        ( this, options_str );
  if ( name == "shockwave"          ) return new shockwave_t          ( this, options_str );
  if ( name == "slam"               ) return new slam_t               ( this, options_str );
  if ( name == "stance"             ) return new stance_t             ( this, options_str );
  if ( name == "sunder_armor"       ) return new sunder_armor_t       ( this, options_str );
  if ( name == "sweeping_strikes"   ) return new sweeping_strikes_t   ( this, options_str );
  if ( name == "thunder_clap"       ) return new thunder_clap_t       ( this, options_str );
  if ( name == "victory_rush"       ) return new victory_rush_t       ( this, options_str );
  if ( name == "whirlwind"          ) return new whirlwind_t          ( this, options_str );

  return player_t::create_action( name, options_str );
}

// warrior_t::init_talents ==================================================

void warrior_t::init_talents()
{
  // Talents

  // Arms
  talents.bladestorm              = find_talent( "Bladestorm" );
  talents.blitz                   = find_talent( "Blitz" );
  talents.blood_frenzy            = find_talent( "Blood Frenzy" );
  talents.deadly_calm             = find_talent( "Deadly Calm" );
  talents.deep_wounds             = find_talent( "Deep Wounds" );
  talents.drums_of_war            = find_talent( "Drums of War" );
  talents.impale                  = find_talent( "Impale" );
  talents.improved_slam           = find_talent( "Improved Slam" );
  talents.juggernaut              = find_talent( "Juggernaut" );
  talents.lambs_to_the_slaughter  = find_talent( "Lambs to the Slaughter" );
  talents.sudden_death            = find_talent( "Sudden Death" );
  talents.sweeping_strikes        = find_talent( "Sweeping Strikes" );
  talents.tactical_mastery        = find_talent( "Tactical Mastery" );
  talents.taste_for_blood         = find_talent( "Taste for Blood" );
  talents.war_academy             = find_talent( "War Academy" );
  talents.wrecking_crew           = find_talent( "Wrecking Crew" );

  // Fury
  talents.battle_trance           = find_talent( "Battle Trance" );
  talents.bloodsurge              = find_talent( "Bloodsurge" );
  talents.booming_voice           = find_talent( "Booming Voice" );
  talents.cruelty                 = find_talent( "Cruelty" );
  talents.death_wish              = find_talent( "Death Wish" );
  talents.enrage                  = find_talent( "Enrage" );
  talents.executioner             = find_talent( "Executioner" );
  talents.flurry                  = find_talent( "Flurry" );
  talents.gag_order               = find_talent( "Gag Order" );
  talents.intensify_rage          = find_talent( "Intensify Rage" );
  talents.meat_cleaver            = find_talent( "Meat Cleaver" );
  talents.raging_blow             = find_talent( "Raging Blow" );
  talents.rampage                 = find_talent( "Rampage" );
  talents.rude_interruption       = find_talent( "Rude Interruption" );
  talents.single_minded_fury      = find_talent( "Single-Minded Fury" );
  talents.titans_grip             = find_talent( "Titan's Grip" );

  // Prot
  talents.incite                  = find_talent( "Incite" );
  talents.toughness               = find_talent( "Toughness" );
  talents.blood_and_thunder       = find_talent( "Blood and Thunder" );
  talents.shield_specialization   = find_talent( "Shield Specialization" );
  talents.shield_mastery          = find_talent( "Shield Mastery" );
  talents.hold_the_line           = find_talent( "Hold the Line" );
  talents.last_stand              = find_talent( "Last Stand" );
  talents.concussion_blow         = find_talent( "Concussion Blow" );
  talents.bastion_of_defense      = find_talent( "Bastion of Defense" );
  talents.warbringer              = find_talent( "Warbringer" );
  talents.improved_revenge        = find_talent( "Improved Revenge" );
  talents.devastate               = find_talent( "Devastate" );
  talents.impending_victory       = find_talent( "Impending Victory" );
  talents.thunderstruck           = find_talent( "Thunderstruck" );
  talents.vigilance               = find_talent( "Vigilance" );
  talents.heavy_repercussions     = find_talent( "Heavy Repercussions" );
  talents.safeguard               = find_talent( "Safeguard" );
  talents.sword_and_board         = find_talent( "Sword and Board" );
  talents.shockwave               = find_talent( "Shockwave" );

  player_t::init_talents();
}

// warrior_t::init_spells ===================================================

void warrior_t::init_spells()
{
  player_t::init_spells();

  // Mastery
  mastery.critical_block         = new mastery_t( this, "critical_block",         76857, TREE_PROTECTION );
  mastery.strikes_of_opportunity = new mastery_t( this, "strikes_of_opportunity", 76838, TREE_ARMS );
  mastery.unshackled_fury        = new mastery_t( this, "unshackled_fury",        76856, TREE_FURY );

  // Spec Passives
  spec.anger_management                 = new passive_spell_t( this, "anger_management",                 12296 );
  spec.dual_wield_specialization        = new passive_spell_t( this, "dual_wield_specialization",        23588 );
  spec.precision                        = new passive_spell_t( this, "precision",                        29592 );
  spec.sentinel                         = new passive_spell_t( this, "sentinel",                         29144 );
  spec.two_handed_weapon_specialization = new passive_spell_t( this, "two_handed_weapon_specialization", 12712 );

  glyphs.battle              = find_glyph( "Glyph of Battle" );
  glyphs.berserker_rage      = find_glyph( "Glyph of Berserker Rage" );
  glyphs.bladestorm          = find_glyph( "Glyph of Bladestorm" );
  glyphs.bloodthirst         = find_glyph( "Glyph of Bloodthirst" );
  glyphs.bloody_healing      = find_glyph( "Glyph of Bloody Healing" );
  glyphs.cleaving            = find_glyph( "Glyph of Cleaving" );
  glyphs.colossus_smash      = find_glyph( "Glyph of Colossus Smash" );
  glyphs.command             = find_glyph( "Glyph of Command" );
  glyphs.devastate           = find_glyph( "Glyph of Devastate" );
  glyphs.furious_sundering   = find_glyph( "Glyph of Furious Sundering" );
  glyphs.heroic_throw        = find_glyph( "Glyph of Heroic Throw" );
  glyphs.mortal_strike       = find_glyph( "Glyph of Mortal Strike" );
  glyphs.overpower           = find_glyph( "Glyph of Overpower" );
  glyphs.raging_blow         = find_glyph( "Glyph of Raging Blow" );
  glyphs.rapid_charge        = find_glyph( "Glyph of Rapid Charge" );
  glyphs.resonating_power    = find_glyph( "Glyph of Resonating Power" );
  glyphs.revenge             = find_glyph( "Glyph of Revenge" );
  glyphs.shield_slam         = find_glyph( "Glyph of Shield Slam" );
  glyphs.shockwave           = find_glyph( "Glyph of Shockwave" );
  glyphs.slam                = find_glyph( "Glyph of Slam" );
  glyphs.sweeping_strikes    = find_glyph( "Glyph of Sweeping Strikes" );
  glyphs.victory_rush        = find_glyph( "Glyph of Victory Rush" );

  // Active spells
  if ( talents.deep_wounds -> ok() )
    active_deep_wounds = new deep_wounds_t( this );

  if ( mastery.strikes_of_opportunity -> ok() )
    active_opportunity_strike = new opportunity_strike_t( this );

  fiery_attack = new fiery_attack_t( this );


  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //  C2P    C4P     M2P     M4P     T2P     T4P    H2P    H4P
    {     0,     0,  90293,  90295,  90296,  90297,     0,     0 }, // Tier11
    {     0,     0,  99234,  99238,  99239,  99242,     0,     0 }, // Tier12
    {     0,     0, 105797, 105907, 105908, 105911,     0,     0 }, // Tier13
    {     0,     0,      0,      0,      0,      0,     0,     0 },
  };

  sets = new set_bonus_array_t( this, set_bonuses );
}

// warrior_t::init_defense ==================================================

void warrior_t::init_defense()
{
  player_t::init_defense();

  initial_parry_rating_per_strength = 0.27;
}

// warrior_t::init_base =====================================================

void warrior_t::init_base()
{
  player_t::init_base();

  resource_base[  RESOURCE_RAGE  ] = 100;

  initial_attack_power_per_strength = 2.0;
  initial_attack_power_per_agility  = 0.0;

  base_attack_power = level * 2 + 60;

  // FIXME! Level-specific!
  base_miss    = 0.05;
  base_parry   = 0.05;
  base_block   = 0.05;

  if ( primary_tree() == TREE_PROTECTION )
    vengeance_enabled = true;

  if ( talents.toughness -> ok() )
    initial_armor_multiplier *= 1.0 + talents.toughness -> effect1().percent();

  diminished_kfactor    = 0.009560;
  diminished_dodge_capi = 0.01523660;
  diminished_parry_capi = 0.01523660;

  if ( spec.sentinel -> ok() )
  {
    attribute_multiplier_initial[ ATTR_STAMINA ] *= 1.0  + spec.sentinel -> effect_base_value( 1 ) / 100.0;
    base_block += spec.sentinel -> effect_base_value( 3 ) / 100.0;
  }

  base_gcd = 1.5;
}

// warrior_t::init_scaling ==================================================

void warrior_t::init_scaling()
{
  player_t::init_scaling();

  if ( talents.single_minded_fury -> ok() || talents.titans_grip -> ok() )
  {
    scales_with[ STAT_WEAPON_OFFHAND_DPS    ] = 1;
    scales_with[ STAT_WEAPON_OFFHAND_SPEED  ] = sim -> weapon_speed_scale_factors;
    scales_with[ STAT_HIT_RATING2           ] = 1;
  }

  if ( primary_role() == ROLE_TANK )
  {
    scales_with[ STAT_PARRY_RATING ] = 1;
    scales_with[ STAT_BLOCK_RATING ] = 1;
  }
}

// warrior_t::init_buffs ====================================================

void warrior_t::init_buffs()
{
  double duration = 12.0;

  player_t::init_buffs();

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, rng_type=RNG_CYCLIC, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, rng_type=RNG_CYCLIC, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, rng_type=RNG_CYCLIC, activated=true )

  buffs_bastion_of_defense        = new buff_t( this, "bastion_of_defense",        1, 12.0,   0, talents.bastion_of_defense -> proc_chance() );
  buffs_battle_stance             = new buff_t( this, 21156, "battle_stance" );
  buffs_battle_trance             = new buff_t( this, "battle_trance",             1, 15.0,   0, talents.battle_trance -> proc_chance() );
  buffs_berserker_rage            = new buff_t( this, "berserker_rage",            1, 10.0 );
  buffs_berserker_stance          = new buff_t( this, 7381, "berserker_stance" );
  buffs_bloodsurge                = new buff_t( this, "bloodsurge",                1, 10.0,   0, talents.bloodsurge -> proc_chance() );
  buffs_bloodthirst               = new buff_t( this, 23885, "bloodthirst" );
  buffs_colossus_smash            = new buff_t( this, "colossus_smash",            1,  6.0 );
  buffs_deadly_calm               = new buff_t( this, "deadly_calm",               1, 10.0 );
  buffs_death_wish                = new buff_t( this, "death_wish",                1, 30.0 );
  buffs_defensive_stance          = new buff_t( this, 7376, "defensive_stance" );
  buffs_enrage                    = new buff_t( this, talents.enrage -> effect_trigger_spell( 1 ), "enrage",  talents.enrage -> proc_chance() );
  buffs_executioner_talent        = new buff_t( this, talents.executioner -> effect_trigger_spell( 1 ), "executioner_talent", talents.executioner -> proc_chance() );
  buffs_flurry                    = new buff_t( this, talents.flurry -> effect_trigger_spell( 1 ), "flurry", talents.flurry -> proc_chance() );
  buffs_hold_the_line             = new buff_t( this, "hold_the_line",             1,  5.0 * talents.hold_the_line -> rank() );
  buffs_incite                    = new buff_t( this, "incite",                    1, 10.0, 0.0, talents.incite -> proc_chance() );
  buffs_inner_rage                = new buff_t( this, 1134, "inner_rage" );
  buffs_overpower                 = new buff_t( this, "overpower",                 1,  6.0, 1.5 );
  buffs_juggernaut                = new buff_t( this, 65156, "juggernaut", talents.juggernaut -> proc_chance() ); //added by hellord
  buffs_lambs_to_the_slaughter    = new buff_t( this, talents.lambs_to_the_slaughter -> effect1().trigger_spell_id(), "lambs_to_the_slaughter", talents.lambs_to_the_slaughter -> proc_chance() );
  buffs_last_stand                = new buff_last_stand_t( this, 12976, "last_stand" );
  buffs_meat_cleaver              = new buff_t( this, "meat_cleaver",              3, 10.0,  0, talents.meat_cleaver -> proc_chance() );
  buffs_recklessness              = new buff_t( this, "recklessness",              3, 12.0 );
  buffs_retaliation               = new buff_t( this, 20230, "retaliation" );
  buffs_revenge                   = new buff_t( this, "revenge",                   1,  5.0 );
  buffs_rude_interruption         = new buff_t( this, "rude_interruption",         1, 15.0 * talents.rude_interruption ->rank() );
  buffs_sweeping_strikes          = new buff_t( this, "sweeping_strikes",          1, 10.0 );
  buffs_sword_and_board           = new buff_t( this, "sword_and_board",           1,  5.0,   0, talents.sword_and_board -> proc_chance() );
  buffs_taste_for_blood           = new buff_t( this, "taste_for_blood",           1,  9.0, 5.0, talents.taste_for_blood -> proc_chance() );
  buffs_thunderstruck             = new buff_t( this, "thunderstruck",             3, 20.0,   0, talents.thunderstruck -> proc_chance() );
  buffs_victory_rush              = new buff_t( this, "victory_rush",              1, 20.0 );
  buffs_wrecking_crew             = new buff_t( this, "wrecking_crew",             1, 12.0,   0 );
  buffs_tier11_4pc_melee          = new buff_t( this, "tier11_4pc_melee",          3, 30.0,   0, set_bonus.tier11_4pc_melee() );
  buffs_tier12_4pc_tank           = new buff_t( this, 99243, "flame_wall", set_bonus.tier12_4pc_tank() );
  buffs_tier13_2pc_tank           = new buff_t( this, "tier13_2pc_tank", 1, 15.0 /* assume some short duration */, set_bonus.tier13_2pc_tank() );
  absorb_buffs.push_back( buffs_tier13_2pc_tank );

  buffs_shield_block              = new shield_block_buff_t( this );
  switch ( talents.booming_voice -> rank() )
  {
  case 1 : duration = 9.0; break;
  case 2 : duration = 6.0; break;
  default: duration = 12.0; break;
  }
  buffs_tier12_2pc_melee          = new buff_t( this, "tier12_2pc_melee",          1, duration,   0, set_bonus.tier12_2pc_melee() );
}

// warrior_t::init_values ====================================================

void warrior_t::init_values()
{
  player_t::init_values();

  if ( set_bonus.pvp_2pc_melee() )
    attribute_initial[ ATTR_STRENGTH ]   += 70;

  if ( set_bonus.pvp_4pc_melee() )
    attribute_initial[ ATTR_STRENGTH ]   += 90;
}

// warrior_t::init_gains ====================================================

void warrior_t::init_gains()
{
  player_t::init_gains();

  gains_anger_management       = get_gain( "anger_management"      );
  gains_avoided_attacks        = get_gain( "avoided_attacks"       );
  gains_battle_shout           = get_gain( "battle_shout"          );
  gains_commanding_shout       = get_gain( "commanding_shout"      );
  gains_berserker_rage         = get_gain( "berserker_rage"        );
  gains_blood_frenzy           = get_gain( "blood_frenzy"          );
  gains_incoming_damage        = get_gain( "incoming_damage"       );
  gains_charge                 = get_gain( "charge"                );
  gains_melee_main_hand        = get_gain( "melee_main_hand"       );
  gains_melee_off_hand         = get_gain( "melee_off_hand"        );
  gains_shield_specialization  = get_gain( "shield_specialization" );
  gains_sudden_death           = get_gain( "sudden_death"          );
}

// warrior_t::init_procs ====================================================

void warrior_t::init_procs()
{
  player_t::init_procs();

  procs_munched_deep_wounds     = get_proc( "munched_deep_wounds"     );
  procs_rolled_deep_wounds      = get_proc( "rolled_deep_wounds"      );
  procs_parry_haste             = get_proc( "parry_haste"             );
  procs_strikes_of_opportunity  = get_proc( "strikes_of_opportunity"  );
  procs_sudden_death            = get_proc( "sudden_death"            );
  procs_fiery_attack            = get_proc( "fiery_attack"            );
  procs_munched_tier12_2pc_tank = get_proc( "munched_tier12_2pc_tank" );
  procs_rolled_tier12_2pc_tank  = get_proc( "rolled_tier12_2pc_tank"  );
  procs_tier13_4pc_melee        = get_proc( "tier13_4pc_melee"        );
}

// warrior_t::init_uptimes ==================================================

void warrior_t::init_benefits()
{
  player_t::init_benefits();

  uptimes_rage_cap    = get_benefit( "rage_cap" );
}

// warrior_t::init_rng ======================================================

void warrior_t::init_rng()
{
  player_t::init_rng();

  rng_blood_frenzy              = get_rng( "blood_frenzy"              );
  rng_executioner_talent        = get_rng( "executioner_talent"        );
  rng_impending_victory         = get_rng( "impending_victory"         );
  rng_strikes_of_opportunity    = get_rng( "strikes_of_opportunity"    );
  rng_sudden_death              = get_rng( "sudden_death"              );
  rng_wrecking_crew             = get_rng( "wrecking_crew"             );
  rng_fiery_attack              = get_rng( "fiery_attack"              );
}

// warrior_t::init_actions ==================================================

void warrior_t::init_actions()
{
  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( !quiet )
      sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  if ( action_list_str.empty() )
  {
    switch ( primary_tree() )
    {

    case TREE_FURY:
    case TREE_ARMS:
      // Flask
      if ( level >= 80 )
        action_list_str += "/flask,type=titanic_strength";
      else if ( level >= 75 )
        action_list_str += "/flask,type=endless_rage";

      // Food
      if ( level >= 80 )
        action_list_str += "/food,type=beer_basted_crocolisk";
      else if ( level >= 70 )
        action_list_str += "/food,type=dragonfin_filet";

      break;

    case TREE_PROTECTION:
      // Flask
      if ( level >= 80 )
        action_list_str += "/flask,type=steelskin";
      else if ( level >= 75 )
        action_list_str += "/flask,type=stoneblood";

      // Food
      if ( level >= 80 )
        action_list_str += "/food,type=beer_basted_crocolisk";
      else if ( level >= 70 )
        action_list_str += "/food,type=dragonfin_filet";

    break; default: break;
    }

    action_list_str += "/snapshot_stats";

    // Potion
    if ( primary_tree() == TREE_ARMS )
    {
      if ( level >= 80 )
        action_list_str += "/golemblood_potion,if=!in_combat|buff.recklessness.up|target.time_to_die<26";
      else if ( level >= 70 )
        action_list_str += "/speed_potion,if=!in_combat|buff.bloodlust.react";
    }
    else if ( primary_tree() == TREE_FURY )
    {
      if ( level >= 80 )
        action_list_str += "/golemblood_potion,if=!in_combat|buff.bloodlust.react";
      else if ( level >= 70 )
        action_list_str += "/speed_potion,if=!in_combat|buff.bloodlust.react";
    }
    else
    {
      if ( level >= 80 )
        action_list_str += "/earthen_potion,if=!in_combat|health_pct<35&buff.earthen_potion.down";
      else if ( level >= 70 )
        action_list_str += "/indestructible_potion,if=!in_combat|health_pct<35&buff.indestructible_potion.down";
    }

    action_list_str += "/auto_attack";

    // Usable Item
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

    // Arms
    if ( primary_tree() == TREE_ARMS )
    {
      if ( glyphs.berserker_rage -> ok() ) action_list_str += "/berserker_rage,if=rage<=95,use_off_gcd=1";
      if ( talents.deadly_calm -> ok() ) action_list_str += "/deadly_calm,use_off_gcd=1";
      action_list_str += "/inner_rage,if=buff.deadly_calm.down&cooldown.deadly_calm.remains>15,use_off_gcd=1";
      action_list_str += "/recklessness,if=target.health_pct>90|target.health_pct<=20,use_off_gcd=1";
      action_list_str += "/stance,choose=berserker,if=buff.taste_for_blood.down&dot.rend.remains>0&rage<=75,use_off_gcd=1";
      action_list_str += "/stance,choose=battle,if=dot.rend.remains=0,use_off_gcd=1";
      action_list_str += "/rend,if=!ticking";
      if ( talents.sweeping_strikes -> ok() ) action_list_str += "/sweeping_strikes,if=target.adds>0,use_off_gcd=1";
      // Don't want to bladestorm during SS as it's only 1 extra hit per WW not per target
      action_list_str += "/bladestorm,if=target.adds>0&!buff.deadly_calm.up&!buff.sweeping_strikes.up";
      action_list_str += "/cleave,if=target.adds>0,use_off_gcd=1";
      action_list_str += "/mortal_strike,if=target.health_pct>20";
      if ( level >= 81 ) action_list_str += "/colossus_smash,if=buff.colossus_smash.down";
      action_list_str += "/mortal_strike,if=target.health_pct<=20&(buff.colossus_smash.down|dot.rend.remains<3|buff.wrecking_crew.down|rage<30)";
      action_list_str += "/stance,choose=battle,if=target.health_pct>20&(buff.taste_for_blood.up|buff.overpower.up)&rage<=75&cooldown.mortal_strike.remains>=1.5,use_off_gcd=1";
      action_list_str += "/overpower,if=buff.taste_for_blood.up|buff.overpower.up";
      // Seperated to see usage
      action_list_str += "/heroic_strike,if=buff.deadly_calm.up,use_off_gcd=1";
      if ( set_bonus.tier13_2pc_melee() )
      {
        action_list_str += "/heroic_strike,if=target.health_pct>20&rage>85,use_off_gcd=1";
        action_list_str += "/heroic_strike,if=rage>75&buff.inner_rage.up,use_off_gcd=1";
        action_list_str += "/heroic_strike,if=buff.incite.up&(target.health_pct>20|(target.health_pct<=20&buff.battle_trance.up)),use_off_gcd=1";
        action_list_str += "/heroic_strike,if=buff.inner_rage.up&target.health_pct>20&(rage>40|buff.battle_trance.up),use_off_gcd=1";
        action_list_str += "/heroic_strike,if=buff.inner_rage.up&target.health_pct<=20&(rage>=50|buff.battle_trance.up),use_off_gcd=1";
      }
      else
      {
        action_list_str += "/heroic_strike,if=target.health_pct>20&rage>95,use_off_gcd=1";
        action_list_str += "/heroic_strike,if=rage>85&buff.inner_rage.up,use_off_gcd=1";
        action_list_str += "/heroic_strike,if=buff.incite.up&(target.health_pct>20|(target.health_pct<=20&buff.battle_trance.up)),use_off_gcd=1";
        action_list_str += "/heroic_strike,if=buff.inner_rage.up&target.health_pct>20&(rage>50|buff.battle_trance.up),use_off_gcd=1";
        action_list_str += "/heroic_strike,if=buff.inner_rage.up&target.health_pct<=20&(rage>=60|buff.battle_trance.up),use_off_gcd=1";
      }
      action_list_str += "/execute";
      action_list_str += "/colossus_smash,if=buff.colossus_smash.remains<=1.5&buff.inner_rage.down";
      action_list_str += "/slam,if=rage>=35|buff.battle_trance.up&buff.inner_rage.down";
      action_list_str += "/battle_shout,if=rage<60";
    }

    // Fury
    else if ( primary_tree() == TREE_FURY )
    {
      action_list_str += "/stance,choose=berserker";
      action_list_str += "/recklessness,use_off_gcd=1";
      if ( talents.death_wish -> ok() ) action_list_str += "/death_wish,use_off_gcd=1";
      action_list_str += "/cleave,if=target.adds>0,use_off_gcd=1";
      action_list_str += "/whirlwind,if=target.adds>0";
      if ( set_bonus.tier13_2pc_melee() )
        action_list_str += "/inner_rage,use_off_gcd=1,if=target.adds=0&((rage>=75&target.health_pct>=20)|((buff.incite.up|buff.colossus_smash.up)&((rage>=40&target.health_pct>=20)|(rage>=65&target.health_pct<20))))";
      action_list_str += "/heroic_strike,use_off_gcd=1,if=(((rage>=85|(set_bonus.tier13_2pc_melee&buff.inner_rage.up&rage>=75))&target.health_pct>=20)|buff.battle_trance.up|((buff.incite.up|buff.colossus_smash.up)&(((rage>=50|(rage>=40&set_bonus.tier13_2pc_melee&buff.inner_rage.up))&target.health_pct>=20)|((rage>=75|(rage>=65&set_bonus.tier13_2pc_melee&buff.inner_rage.up))&target.health_pct<20))))";
      action_list_str += "/execute,if=buff.executioner_talent.remains<1.5";
      if ( level >= 81 ) action_list_str += "/colossus_smash";
      action_list_str += "/execute,if=buff.executioner_talent.stack<5";
      action_list_str += "/bloodthirst";
      if ( talents.raging_blow -> ok() && talents.titans_grip -> ok() )
      {
        action_list_str += "/berserker_rage,if=!(buff.death_wish.up|buff.enrage.up|buff.unholy_frenzy.up)&rage>15&cooldown.raging_blow.remains<1";
        action_list_str += "/raging_blow";
      }
      action_list_str += "/slam,if=buff.bloodsurge.react";
      action_list_str += "/execute,if=rage>=50";
      if ( talents.raging_blow -> ok() && ! talents.titans_grip -> ok() )
      {
        action_list_str += "/berserker_rage,if=!(buff.death_wish.up|buff.enrage.up|buff.unholy_frenzy.up)&rage>15&cooldown.raging_blow.remains<1";
        action_list_str += "/raging_blow";
      }
      action_list_str += "/battle_shout,if=rage<70";
      if ( ! talents.raging_blow -> ok() && glyphs.berserker_rage -> ok() ) action_list_str += "/berserker_rage";
    }

    // Protection
    else if ( primary_tree() == TREE_PROTECTION )
    {
      action_list_str += "/stance,choose=defensive";
      if ( talents.last_stand -> ok() ) action_list_str += "/last_stand,if=health<30000";
      action_list_str += "/heroic_strike,if=rage>=50";
      action_list_str += "/inner_rage,if=rage>=85";
      action_list_str += "/berserker_rage";

      action_list_str += "/shield_block,sync=shield_slam";
      action_list_str += "/shield_slam";
      action_list_str += "/thunder_clap,if=dot.rend.remains<=3";
      action_list_str += "/rend,if=!ticking";
      if ( talents.devastate -> ok() ) action_list_str += "/devastate,if=cooldown.shield_slam.remains>1.5";
      if ( talents.shockwave -> ok() ) action_list_str += "/shockwave";
      action_list_str += "/concussion_blow";
      action_list_str += "/revenge";
      if ( talents.devastate -> ok() ) action_list_str += "/devastate";
      action_list_str += "/battle_shout";
    }

    // Default
    else
    {
      action_list_str += "/stance,choose=berserker/auto_attack";
    }

    action_list_default = 1;
  }

  player_t::init_actions();
}

// warrior_t::combat_begin ==================================================

void warrior_t::combat_begin()
{
  player_t::combat_begin();

  // We (usually) start combat with zero rage.
  resource_current[ RESOURCE_RAGE ] = std::min( initial_rage, 100 );

  if ( active_stance == STANCE_BATTLE && ! buffs_battle_stance -> check() )
    buffs_battle_stance -> trigger();

  if ( talents.rampage -> ok() )
    sim -> auras.rampage -> trigger();
}

// warrior_t::register_callbacks ==============================================

void warrior_t::register_callbacks()
{
  player_t::register_callbacks();

  register_direct_damage_callback( SCHOOL_ATTACK_MASK, new bloodthirst_buff_callback_t( this, buffs_bloodthirst ) );
}

// warrior_t::reset =========================================================

void warrior_t::reset()
{
  player_t::reset();

  active_stance = STANCE_BATTLE;
}

// warrior_t::composite_attack_power_multiplier =============================

double warrior_t::composite_attack_power_multiplier() const
{
  double mult = player_t::composite_attack_power_multiplier();

  mult *= 1.0 + 0.01 * buffs_tier11_4pc_melee -> stack();

  return mult;
}

// warrior_t::composite_attack_hit ==========================================

double warrior_t::composite_attack_hit() const
{
  double ah = player_t::composite_attack_hit();

  ah += spec.precision -> effect_base_value( 1 ) / 100.0;

  return ah;
}

// warrior_t::composite_attack_crit =========================================

double warrior_t::composite_attack_crit() const
{
  double c = player_t::composite_attack_crit();

  c += talents.rampage -> effect2().percent();

  return c;
}

// warrior_t::composite_mastery =============================================

double warrior_t::composite_mastery() const
{
  double m = player_t::composite_mastery();

  m += spec.precision -> effect_base_value( 2 );

  return m;
}

// warrior_t::composite_attack_haste ========================================

double warrior_t::composite_attack_haste() const
{
  double h = player_t::composite_attack_haste();

  // Unholy Frenzy is effected by Unshackled Fury
  if ( mastery.unshackled_fury -> ok() && buffs.unholy_frenzy -> up() )
  {
    // Assume it's multiplicative.
    h /= 1.0 / ( 1.0 + 0.2 );

    h *= 1.0 / ( 1.0 + ( 0.2 * ( 1.0 + composite_mastery() * mastery.unshackled_fury -> effect_base_value( 3 ) / 10000.0 ) ) );
  }

  return h;
}

// warrior_t::composite_player_multiplier ===================================

double warrior_t::composite_player_multiplier( const school_type school, action_t* a ) const
{
  double m = player_t::composite_player_multiplier( school, a );

  // Stances affect all damage done
  if ( active_stance == STANCE_BATTLE && buffs_battle_stance -> up() )
  {
    m *= 1.0 + buffs_battle_stance -> effect1().percent();
  }
  else if ( active_stance == STANCE_BERSERKER && buffs_berserker_stance -> up() )
  {
    m *= 1.0 + buffs_berserker_stance -> effect1().percent();
  }

  if ( ( school == SCHOOL_PHYSICAL || school == SCHOOL_BLEED ) && buffs_tier12_2pc_melee -> check() )
  {
    m *= 1.0 + buffs_tier12_2pc_melee -> value();
  }

  return m;
}

// warrior_t::matching_gear_multiplier ======================================

double warrior_t::matching_gear_multiplier( const attribute_type attr ) const
{
  if ( ( attr == ATTR_STRENGTH ) && ( primary_tree() == TREE_ARMS || primary_tree() == TREE_FURY ) )
    return 0.05;

  if ( ( attr == ATTR_STAMINA ) && ( primary_tree() == TREE_PROTECTION ) )
    return 0.05;

  return 0.0;
}

// warrior_t::composite_tank_block ==========================================

double warrior_t::composite_tank_block() const
{
  double b = player_t::composite_tank_block();

  b += composite_mastery() * mastery.critical_block -> effect_base_value( 3 ) / 10000.0;

  if ( buffs_shield_block -> up() )
    b = 1.0;

  return b;
}

// warrior_t::composite_tank_crit_block =====================================

double warrior_t::composite_tank_crit_block() const
{
  double b = player_t::composite_tank_crit_block();

  b += composite_mastery() * mastery.critical_block -> effect_coeff( 1 ) / 100.0;

  if ( buffs_hold_the_line -> up() )
    b += 0.10;

  return b;
}

// warrior_t::composite_tank_crit ===========================================

double warrior_t::composite_tank_crit( const school_type school ) const
{
  double c = player_t::composite_tank_crit( school );

  if ( school == SCHOOL_PHYSICAL && talents.bastion_of_defense -> rank() )
    c += talents.bastion_of_defense -> effect1().percent();

  return c;
}

// warrior_t::composite_tank_parry ===========================================

double warrior_t::composite_tank_parry() const
{
  double p = player_t::composite_tank_parry();

  p += buffs_tier12_4pc_tank -> value();

  return p;
}

// warrior_t::regen =========================================================

void warrior_t::regen( double periodicity )
{
  player_t::regen( periodicity );

  if ( spec.anger_management -> ok() )
    resource_gain( RESOURCE_RAGE, ( periodicity / 3.0 ), gains_anger_management );

  uptimes_rage_cap -> update( resource_current[ RESOURCE_RAGE ] ==
                                     resource_max    [ RESOURCE_RAGE] );
}

// warrior_t::primary_role() ================================================

int warrior_t::primary_role() const
{
  if ( player_t::primary_role() == ROLE_TANK )
    return ROLE_TANK;

  if ( player_t::primary_role() == ROLE_DPS || player_t::primary_role() == ROLE_ATTACK )
    return ROLE_ATTACK;

  if ( primary_tree() == TREE_PROTECTION )
    return ROLE_TANK;

  return ROLE_ATTACK;
}

// warrior_t::assess_damage =================================================

double warrior_t::assess_damage( double            amount,
                                 const school_type school,
                                 int               dmg_type,
                                 int               result,
                                 action_t*         action )
{
  if ( result == RESULT_HIT    ||
       result == RESULT_CRIT   ||
       result == RESULT_GLANCE ||
       result == RESULT_BLOCK  )
  {
    double rage_gain = amount * 18.92 / resource_max[ RESOURCE_HEALTH ];
    resource_gain( RESOURCE_RAGE, rage_gain, gains_incoming_damage );
  }


  if ( result == RESULT_BLOCK )
  {
    if ( talents.shield_specialization -> ok() )
    {
      resource_gain( RESOURCE_RAGE, 5.0 * talents.shield_specialization -> rank(), gains_shield_specialization );
    }
  }


  if ( result == RESULT_BLOCK ||
       result == RESULT_DODGE ||
       result == RESULT_PARRY )
  {
    buffs_bastion_of_defense -> trigger();
    buffs_revenge -> trigger();
  }


  if ( result == RESULT_PARRY )
  {
    buffs_hold_the_line -> trigger();

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

  trigger_retaliation( this, school, result );

  // Defensive Stance
  if ( active_stance == STANCE_DEFENSE && buffs_defensive_stance -> up() )
    amount *= 0.90;

  return player_t::assess_damage( amount, school, dmg_type, result, action );
}

// warrior_t::create_options ================================================

void warrior_t::create_options()
{
  player_t::create_options();

  option_t warrior_options[] =
  {
    { "initial_rage",            OPT_INT,  &initial_rage            },
    { "instant_flurry_haste",    OPT_BOOL, &instant_flurry_haste    },
    { NULL, OPT_UNKNOWN, NULL }
  };

  option_t::copy( options, warrior_options );
}

// warrior_t::copy_from =====================================================

void warrior_t::copy_from( player_t* source )
{
  player_t::copy_from( source );
  warrior_t* p = source -> cast_warrior();

  initial_rage            = p -> initial_rage;
  instant_flurry_haste    = p -> instant_flurry_haste;
}

// warrior_t::decode_set ====================================================

int warrior_t::decode_set( item_t& item )
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

  if ( strstr( s, "earthen" ) )
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

    if ( is_melee ) return SET_T11_MELEE;
    if ( is_tank  ) return SET_T11_TANK;
  }

  if ( strstr( s, "_of_the_molten_giant" ) )
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

    if ( is_melee ) return SET_T12_MELEE;
    if ( is_tank  ) return SET_T12_TANK;
  }

  if ( strstr( s, "colossal_dragonplate" ) )
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

    if ( is_melee ) return SET_T13_MELEE;
    if ( is_tank  ) return SET_T13_TANK;
  }

  if ( strstr( s, "_gladiators_plate_"   ) ) return SET_PVP_MELEE;

  return SET_NONE;
}

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

// player_t::create_warrior =================================================

player_t* player_t::create_warrior( sim_t* sim, const std::string& name, race_type r )
{
  return new warrior_t( sim, name, r );
}

// warrior_init =============================================================

void player_t::warrior_init( sim_t* sim )
{
  sim -> auras.rampage      = new aura_t( sim, "rampage",      1, 0.0 );

  for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[i];
    p -> debuffs.blood_frenzy_bleed    = new debuff_t( p, "blood_frenzy_bleed",    1, 60.0 );
    p -> debuffs.blood_frenzy_physical = new debuff_t( p, "blood_frenzy_physical", 1, 60.0 );
    p -> debuffs.demoralizing_shout    = new debuff_t( p, "demoralizing_shout",    1, 30.0 );
    p -> debuffs.shattering_throw      = new debuff_t( p, 64382, "shattering_throw" );
    p -> debuffs.sunder_armor          = new debuff_t( p, 58567, "sunder_armor" );
    p -> debuffs.thunder_clap          = new debuff_t( p, "thunder_clap",          1, 30.0 );
    p -> buffs.commanding_shout        = new   buff_t( p, 469, "commanding_shout" );
    p -> buffs.battle_shout            = new   buff_t( p, 6673, "battle_shout" );
  }
}

// player_t::warrior_combat_begin ===========================================

void player_t::warrior_combat_begin( sim_t* sim )
{
  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if ( sim -> overrides.battle_shout )
      p -> buffs.battle_shout -> override( 1, p -> buffs.battle_shout -> effect_average( 1 ) );
  }
  if ( sim -> overrides.rampage      ) sim -> auras.rampage      -> override();

  for ( player_t* t = sim -> target_list; t; t = t -> next )
  {
    if ( sim -> overrides.blood_frenzy_bleed    ) t -> debuffs.blood_frenzy_bleed    -> override( 1, 30 );
    if ( sim -> overrides.blood_frenzy_physical ) t -> debuffs.blood_frenzy_physical -> override( 1,  4 );
    if ( sim -> overrides.demoralizing_shout    ) t -> debuffs.demoralizing_shout    -> override();
    if ( sim -> overrides.sunder_armor          ) t -> debuffs.sunder_armor          -> override( 3 );
    if ( sim -> overrides.thunder_clap          ) t -> debuffs.thunder_clap          -> override();
  }
}
