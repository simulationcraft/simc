// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

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

namespace { // ANONYMOUS NAMESPACE

// ==========================================================================
// Warrior
// ==========================================================================

struct warrior_t;

#define SC_WARRIOR 1

#if SC_WARRIOR == 1

enum warrior_stance { STANCE_BATTLE=1, STANCE_BERSERKER, STANCE_DEFENSE=4 };

struct warrior_td_t : public actor_pair_t
{
  dot_t* dots_deep_wounds;
  dot_t* dots_rend;

  buff_t* debuffs_colossus_smash;

  warrior_td_t( player_t* target, warrior_t* p );
};

struct warrior_t : public player_t
{
  int instant_flurry_haste;
  int initial_rage;

  // Active
  action_t* active_deep_wounds;
  action_t* active_retaliation;
  action_t* active_opportunity_strike;
  warrior_stance  active_stance;

  // Buffs
  buff_t* buffs_bastion_of_defense;
  buff_t* buffs_battle_stance;
  buff_t* buffs_battle_trance;
  buff_t* buffs_berserker_rage;
  buff_t* buffs_berserker_stance;
  buff_t* buffs_bloodsurge;
  buff_t* buffs_bloodthirst;
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
  buff_t* buffs_tier13_2pc_tank;

  // Cooldowns
  cooldown_t* cooldowns_colossus_smash;
  cooldown_t* cooldowns_shield_slam;
  cooldown_t* cooldowns_strikes_of_opportunity;

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
    const spell_data_t* battle;
    const spell_data_t* berserker_rage;
    const spell_data_t* bladestorm;
    const spell_data_t* bloodthirst;
    const spell_data_t* bloody_healing;
    const spell_data_t* cleaving;
    const spell_data_t* colossus_smash;
    const spell_data_t* command;
    const spell_data_t* devastate;
    const spell_data_t* furious_sundering;
    const spell_data_t* heroic_throw;
    const spell_data_t* mortal_strike;
    const spell_data_t* overpower;
    const spell_data_t* raging_blow;
    const spell_data_t* rapid_charge;
    const spell_data_t* resonating_power;
    const spell_data_t* revenge;
    const spell_data_t* shield_slam;
    const spell_data_t* shockwave;
    const spell_data_t* slam;
    const spell_data_t* sweeping_strikes;
    const spell_data_t* victory_rush;
  } glyphs;

  // Mastery
  struct mastery_t
  {
    const spell_data_t* critical_block;
    const spell_data_t* strikes_of_opportunity;
    const spell_data_t* unshackled_fury;
  } mastery;

  // Procs
  proc_t* procs_munched_deep_wounds;
  proc_t* procs_rolled_deep_wounds;
  proc_t* procs_parry_haste;
  proc_t* procs_strikes_of_opportunity;
  proc_t* procs_sudden_death;
  proc_t* procs_tier13_4pc_melee;

  // Random Number Generation
  rng_t* rng_blood_frenzy;
  rng_t* rng_executioner_talent;
  rng_t* rng_impending_victory;
  rng_t* rng_strikes_of_opportunity;
  rng_t* rng_sudden_death;
  rng_t* rng_wrecking_crew;

  // Spec Passives
  struct spec_t
  {
    const spell_data_t* anger_management;
    const spell_data_t* dual_wield_specialization;
    const spell_data_t* precision;
    const spell_data_t* sentinel;
    const spell_data_t* two_handed_weapon_specialization;
  } spec;

  // Talents
  struct talents_t
  {
    // Arms
    const spell_data_t* bladestorm;
    const spell_data_t* blitz;
    const spell_data_t* blood_frenzy;
    const spell_data_t* deadly_calm;
    const spell_data_t* deep_wounds;
    const spell_data_t* drums_of_war;
    const spell_data_t* impale;
    const spell_data_t* improved_slam;
    const spell_data_t* juggernaut;
    const spell_data_t* lambs_to_the_slaughter;
    const spell_data_t* sudden_death;
    const spell_data_t* sweeping_strikes;
    const spell_data_t* tactical_mastery;
    const spell_data_t* taste_for_blood;
    const spell_data_t* war_academy;
    const spell_data_t* wrecking_crew;

    // Fury
    const spell_data_t* battle_trance;
    const spell_data_t* bloodsurge;
    const spell_data_t* booming_voice;
    const spell_data_t* cruelty;
    const spell_data_t* death_wish;
    const spell_data_t* enrage;
    const spell_data_t* executioner;
    const spell_data_t* flurry;
    const spell_data_t* gag_order;
    const spell_data_t* intensify_rage;
    const spell_data_t* meat_cleaver;
    const spell_data_t* raging_blow;
    const spell_data_t* rampage;
    const spell_data_t* rude_interruption;
    const spell_data_t* single_minded_fury;
    const spell_data_t* skirmisher;
    const spell_data_t* titans_grip;

    // Prot
    const spell_data_t* incite;
    const spell_data_t* toughness;
    const spell_data_t* blood_and_thunder;
    const spell_data_t* shield_specialization;
    const spell_data_t* shield_mastery;
    const spell_data_t* hold_the_line;
    const spell_data_t* last_stand;
    const spell_data_t* concussion_blow;
    const spell_data_t* bastion_of_defense;
    const spell_data_t* warbringer;
    const spell_data_t* improved_revenge;
    const spell_data_t* devastate;
    const spell_data_t* impending_victory;
    const spell_data_t* thunderstruck;
    const spell_data_t* vigilance;
    const spell_data_t* heavy_repercussions;
    const spell_data_t* safeguard;
    const spell_data_t* sword_and_board;
    const spell_data_t* shockwave;
  } talents;

  // Up-Times
  benefit_t* uptimes_rage_cap;


  target_specific_t<warrior_td_t> target_data;

  warrior_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, WARRIOR, name, r ),
    glyphs( glyphs_t() ),
    mastery( mastery_t() ),
    spec( spec_t() ),
    talents( talents_t() )
  {

    target_data.init( "target_data", this );
    // Active
    active_deep_wounds        = 0;
    active_opportunity_strike = 0;
    active_stance             = STANCE_BATTLE;

    // Cooldowns
    cooldowns_colossus_smash         = get_cooldown( "colossus_smash"         );
    cooldowns_shield_slam            = get_cooldown( "shield_slam"            );
    cooldowns_strikes_of_opportunity = get_cooldown( "strikes_of_opportunity" );

    instant_flurry_haste = 1;
    initial_rage = 0;

    initial.distance = 3;

    create_options();
  }

  // Character Definition

  virtual warrior_td_t* get_target_data( player_t* target )
  {
    warrior_td_t*& td = target_data[ target ];
    if ( ! td ) td = new warrior_td_t( target, this );
    return td;
  }

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
  virtual double    composite_attack_hit();
  virtual double    composite_attack_crit( weapon_t* );
  virtual double    composite_mastery();
  virtual double    composite_attack_haste();
  virtual double    composite_player_multiplier( school_e school, action_t* a = NULL );
  virtual double    matching_gear_multiplier( attribute_e attr );
  virtual double    composite_tank_block();
  virtual double    composite_tank_crit_block();
  virtual double    composite_tank_crit( school_e school );
  virtual void      reset();
  virtual void      regen( timespan_t periodicity );
  virtual void      create_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual int       decode_set( item_t& );
  virtual resource_e primary_resource() { return RESOURCE_RAGE; }
  virtual role_e primary_role();
  virtual double    assess_damage( double amount, school_e, dmg_e, result_e, action_t* a );
  virtual void      copy_from( player_t* source );

  // Temporary
  virtual std::string set_default_talents()
  {
    switch ( specialization() )
    {
    case SPEC_NONE: break;
    default: break;
    }

    return player_t::set_default_talents();
  }

  virtual std::string set_default_glyphs()
  {
    switch ( specialization() )
    {
    case SPEC_NONE: break;
    default: break;
    }

    return player_t::set_default_glyphs();
  }
};

warrior_td_t::warrior_td_t( player_t* target, warrior_t* p  ) :
    actor_pair_t( target, p )
{
  debuffs_colossus_smash = buff_creator_t( *this, "colossus_smash" ).duration( timespan_t::from_seconds( 6.0 ) );
}

// ==========================================================================
// Warrior Attack
// ==========================================================================

struct warrior_attack_t : public melee_attack_t
{
  int stancemask;

  warrior_attack_t( const std::string& n, warrior_t* p,
                    const spell_data_t* s = spell_data_t::nil() ) :
    melee_attack_t( n, p, s ),
    stancemask( STANCE_BATTLE|STANCE_BERSERKER|STANCE_DEFENSE )
  {
    may_crit   = true;
    may_glance = false;
  }

  warrior_attack_t( const std::string& n, uint32_t id, warrior_t* p ) :
    melee_attack_t( n, p, p -> find_spell( id ) ),
    stancemask( STANCE_BATTLE|STANCE_BERSERKER|STANCE_DEFENSE )
  {
    may_crit   = true;
    may_glance = false;
  }

  warrior_t* cast() const
  { return debug_cast<warrior_t*>( player ); }

  warrior_td_t* cast_td( player_t* t = 0 ) const
  { return cast() -> get_target_data( t ? t : target ); }

  virtual double armor();
  virtual void   consume_resource();
  virtual double cost();
  virtual void   execute();
  virtual double calculate_weapon_damage( double /* attack_power */ );
  virtual void   player_buff();
  virtual bool   ready();
  virtual void   assess_damage( player_t* t, double, dmg_e, result_e, action_state_t*);
};


// ==========================================================================
// Static Functions
// ==========================================================================

// trigger_bloodsurge =======================================================

static void trigger_bloodsurge( warrior_attack_t* a )
{
  warrior_t* p = a -> cast();

  if ( ! p -> talents.bloodsurge -> ok() )
    return;

  p -> buffs_bloodsurge -> trigger();
}

// Deep Wounds ==============================================================

struct deep_wounds_t : public ignite_like_action_t< warrior_attack_t, warrior_t >
{
  deep_wounds_t( warrior_t* p ) :
    base_t( "deep_wounds", p, p -> find_spell( 115767 ) )
  { }
};

// Warrior Deep Wounds template specialization
template <class WARRIOR_ACTION>
void trigger_deep_wounds( WARRIOR_ACTION* , player_t* )
{
// Todo: Completely redo Deep Wounds, as it is no longer a Ignite-Like mechanic in MoP
}
// trigger_deep_wounds ======================================================
/*
static void trigger_deep_wounds( warrior_attack_t* a )
{
  warrior_t* p = a -> cast();
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

  double deep_wounds_dmg = ( p -> active_deep_wounds -> calculate_weapon_damage( a -> total_attack_power() ) *
                             p -> active_deep_wounds -> weapon_multiplier *
                             p -> active_deep_wounds -> player_multiplier );

  dot_t* dot = p -> active_deep_wounds -> get_dot();

  if ( dot -> ticking )
  {
    deep_wounds_dmg += p -> active_deep_wounds -> base_td * dot -> ticks();
  }

  if ( timespan_t::from_seconds( 6.0 ) + sim -> aura_delay < dot -> remains() )
  {
    if ( sim -> log ) sim -> output( "Player %s munches Deep_Wounds due to Max Deep Wounds Duration.", p -> name() );
    p -> procs_munched_deep_wounds -> occur();
    return;
  }

  if ( p -> active_deep_wounds -> travel_event )
  {
    // There is an SPELL_AURA_APPLIED already in the queue, which will get munched.
    if ( sim -> log ) sim -> output( "Player %s munches previous Deep Wounds due to Aura Delay.", p -> name() );
    p -> procs_munched_deep_wounds -> occur();
  }

  p -> active_deep_wounds -> direct_dmg = deep_wounds_dmg;
  p -> active_deep_wounds -> result = RESULT_HIT;
  p -> active_deep_wounds -> schedule_travel( a -> target );

  dot -> prev_tick_amount = deep_wounds_dmg;

  if ( p -> active_deep_wounds -> travel_event && dot -> ticking )
  {
    if ( dot -> tick_event -> occurs() < p -> active_deep_wounds -> travel_event -> occurs() )
    {
      // Deep_Wounds will tick before SPELL_AURA_APPLIED occurs, which means that the current Deep_Wounds will
      // both tick -and- get rolled into the next Deep_Wounds.
      if ( sim -> log ) sim -> output( "Player %s rolls Deep_Wounds.", p -> name() );
      p -> procs_rolled_deep_wounds -> occur();
    }
  }
}
*/
// trigger_rage_gain ========================================================

static void trigger_rage_gain( warrior_attack_t* a )
{
  // Since 4.0.1 rage gain is 6.5 * weaponspeed and half that for off-hand

  if ( a -> proc )
    return;

  warrior_t* p = a -> cast();
  weapon_t*  w = a -> weapon;

  double rage_gain = 6.5 * w -> swing_time.total_seconds();

  if ( w -> slot == SLOT_OFF_HAND )
    rage_gain /= 2.0;

  rage_gain *= 1.0 + p -> spec.anger_management -> effectN( 2 ).percent();

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
        trigger_gcd = timespan_t::zero();
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
    warrior_attack_t( "opportunity_strike", 76858, p )
  {
    background = true;
  }
};

static void trigger_strikes_of_opportunity( warrior_attack_t* a )
{
  if ( a -> proc )
    return;

  warrior_t* p = a -> cast();

  if ( ! p -> mastery.strikes_of_opportunity -> ok() )
    return;

  if ( p -> cooldowns_strikes_of_opportunity -> remains() > timespan_t::zero() )
    return;

  double chance = p -> composite_mastery() * p -> mastery.strikes_of_opportunity -> effectN( 2 ).percent() / 100.0;

  if ( ! p -> rng_strikes_of_opportunity -> roll( chance ) )
    return;

  p -> cooldowns_strikes_of_opportunity -> start( timespan_t::from_seconds( 0.5 ) );

  assert( p -> active_opportunity_strike );

  if ( p -> sim -> debug )
    p -> sim -> output( "Opportunity Strike procced from %s", a -> name() );


  p -> procs_strikes_of_opportunity -> occur();
  p -> active_opportunity_strike -> execute();
}

// trigger_sudden_death =====================================================

static void trigger_sudden_death( warrior_attack_t* a )
{
  warrior_t* p = a -> cast();

  if ( a -> proc )
    return;

  if ( p -> rng_sudden_death -> roll ( p -> talents.sudden_death -> proc_chance() ) )
  {
    p -> cooldowns_colossus_smash -> reset();
    p -> procs_sudden_death       -> occur();
  }
}

// trigger_sword_and_board ==================================================

static void trigger_sword_and_board( warrior_attack_t* a, result_e result )
{
  warrior_t* p = a -> cast();

  if ( a -> result_is_hit( result ) )
  {
    if ( p -> buffs_sword_and_board -> trigger() )
    {
      p -> cooldowns_shield_slam -> reset();
    }
  }
}

// trigger_enrage ===========================================================

static void trigger_enrage( warrior_attack_t* a )
{
  warrior_t* p = a -> cast();

  if ( ! p -> talents.enrage -> ok() )
    return;

  // Can't proc while DW is active as of 403
  if ( p -> buffs_death_wish -> check() )
    return;

  double enrage_value = p -> buffs_enrage -> data().effectN( 1 ).percent();

  if ( p -> mastery.unshackled_fury -> ok() )
  {
    enrage_value *= 1.0 + p -> composite_mastery() * p -> mastery.unshackled_fury -> effectN( 3 ).percent() / 100.0;
  }

  p -> buffs_enrage -> trigger( 1, enrage_value );
}

// trigger_flurry ===========================================================

static void trigger_flurry( warrior_attack_t* a, int stacks )
{
  warrior_t* p = a -> cast();

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
  // FIXME
  //double mult = 1 + util::talent_rank( p -> talents.flurry -> rank(), 3, 0.08, 0.16, 0.25 );
  double mult = 1;

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
      timespan_t delta;
      if ( mhe -> reschedule_time != timespan_t::zero() )
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
      timespan_t delta;
      if ( ohe -> reschedule_time != timespan_t::zero() )
        delta = ( ohe -> reschedule_time - sim -> current_time ) * mult;
      else
        delta = ( ohe -> time - sim -> current_time ) * mult;
      p -> off_hand_attack -> reschedule_execute( delta );
    }
  }
}

// ==========================================================================
// Warrior Attacks
// ==========================================================================

double warrior_attack_t::armor()
{
  warrior_td_t* td = cast_td();

  double a = attack_t::armor();

  a *= 1.0 - td -> debuffs_colossus_smash -> value();

  return a;
}

// warrior_attack_t::assess_damage ==========================================

void warrior_attack_t::assess_damage( player_t* t, const double amount, const dmg_e dmg_type, const result_e impact_result, action_state_t* s )
{
  attack_t::assess_damage( t, amount, dmg_type, impact_result, s );

  /* warrior_t* p = cast();

  if ( t -> is_enemy() )
  {
    target_t* q =  t -> cast_target();

    if ( p -> buffs_sweeping_strikes -> up() && q -> adds_nearby )
    {
      attack_t::additional_damage( q, amount, dmg_e, impact_result );
    }
  }*/
}

// warrior_attack_t::cost ===================================================

double warrior_attack_t::cost()
{
  double c = attack_t::cost();

  warrior_t* p = cast();

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
  warrior_t* p = cast();

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
  warrior_t* p = cast();

  // Battle Trance only is effective+consumed if action cost was >5
  if ( base_costs[ current_resource() ] > 5 && p -> buffs_battle_trance -> up() )
    p -> buffs_battle_trance -> expire();

  if ( proc ) return;

  if ( result_is_hit() )
  {
    trigger_sudden_death( this );

    trigger_strikes_of_opportunity( this );

    trigger_enrage( this );

    if ( result == RESULT_CRIT )
      trigger_deep_wounds( this, target );
  }
  else if ( result == RESULT_DODGE  )
  {
    p -> buffs_overpower -> trigger();
  }
}

// warrior_attack_t::calculate_weapon_damage ================================

double warrior_attack_t::calculate_weapon_damage( double attack_power )
{
  double dmg = attack_t::calculate_weapon_damage( attack_power );

  // Catch the case where weapon == 0 so we don't crash/retest below.
  if ( dmg == 0 )
    return 0;

  warrior_t* p = cast();

  if ( weapon -> slot == SLOT_OFF_HAND )
    dmg *= 1.0 + p -> spec.dual_wield_specialization -> effectN( 2 ).percent();

  return dmg;
}

// warrior_attack_t::player_buff ============================================

void warrior_attack_t::player_buff()
{
  attack_t::player_buff();
  warrior_t* p = cast();

  // FIXME: much of this can be moved to base for efficiency, but we need
  // to be careful to get the add_percent_mod effect ordering in the
  // abilities right.

  // --- Specializations --

  if ( weapon && weapon -> group() == WEAPON_2H )
    player_multiplier *= 1.0 + p -> spec.two_handed_weapon_specialization -> effectN( 1 ).percent();

  if ( p -> dual_wield() && ( school == SCHOOL_PHYSICAL || school == SCHOOL_BLEED ) )
    player_multiplier *= 1.0 + p -> spec.dual_wield_specialization -> effectN( 3 ).percent();

  // --- Enrages ---

  if ( school == SCHOOL_PHYSICAL || school == SCHOOL_BLEED )
    player_multiplier *= 1.0 + p -> buffs_wrecking_crew -> value();

  if ( school == SCHOOL_PHYSICAL || school == SCHOOL_BLEED )
    player_multiplier *= 1.0 + p -> buffs_enrage -> value();

  // FIXME
  //if ( ( school == SCHOOL_PHYSICAL || school == SCHOOL_BLEED ) && p -> buffs_bastion_of_defense -> up() )
  //  player_multiplier *= 1.0 + p -> talents.bastion_of_defense -> rank() * 0.05;

  // --- Passive Talents ---

  if ( school == SCHOOL_PHYSICAL || school == SCHOOL_BLEED )
    player_multiplier *= 1.0 + p -> buffs_death_wish -> value();

  if ( p -> talents.single_minded_fury -> ok() && p -> dual_wield() )
  {
    if ( p -> main_hand_attack -> weapon -> group() == WEAPON_1H &&
         p ->  off_hand_attack -> weapon -> group() == WEAPON_1H )
    {
      player_multiplier *= 1.0 + p -> talents.single_minded_fury -> effectN( 1 ).percent();
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
    sim -> output( "warrior_attack_t::player_buff: %s hit=%.2f expertise=%.2f crit=%.2f",
                   name(), player_hit, player_expertise, player_crit );
}

// warrior_attack_t::ready() ================================================

bool warrior_attack_t::ready()
{
  if ( ! attack_t::ready() )
    return false;

  warrior_t* p = cast();

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
    warrior_attack_t( name, p, spell_data_t::nil() ),
    sync_weapons( sw )
  {
    may_glance      = true;
    background      = true;
    repeating       = true;
    trigger_gcd     = timespan_t::zero();

    if ( p -> dual_wield() ) base_hit -= 0.19;
  }

  virtual double swing_haste()
  {
    double h = warrior_attack_t::swing_haste();

    warrior_t* p = cast();

    if ( p -> buffs_flurry -> up() )
      h *= 1.0 / ( 1.0 + p -> buffs_flurry -> data().effectN( 1 ).percent() );

    if ( p -> buffs_executioner_talent -> check() )
      h *= 1.0 / ( 1.0 + p -> buffs_executioner_talent -> stack() *
                   p -> buffs_executioner_talent -> data().effectN( 1 ).percent() );

    return h;
  }

  virtual timespan_t execute_time()
  {
    timespan_t t = warrior_attack_t::execute_time();

    if ( player -> in_combat )
      return t;

    if ( weapon -> slot == SLOT_MAIN_HAND || sync_weapons )
      return timespan_t::from_seconds( 0.02 );

    // Before combat begins, unless we are under sync_weapons the OH is
    // delayed by half its swing time.

    return timespan_t::from_seconds( 0.02 ) + t / 2;
  }

  virtual void execute()
  {
    warrior_t* p = cast();

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
        p -> resource_gain( RESOURCE_RAGE, p -> talents.blood_frenzy -> effectN( 3 ).base_value(), p -> gains_blood_frenzy );
      }
    }
  }

  virtual void player_buff()
  {
    warrior_attack_t::player_buff();
    warrior_t* p = cast();

    if ( p -> specialization() == WARRIOR_FURY )
      player_multiplier *= 1.0 + p -> spec.precision -> effectN( 3 ).percent();
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

    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    warrior_t* p = cast();

    p -> main_hand_attack -> schedule_execute();

    if ( p -> off_hand_attack )
      p -> off_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    warrior_t* p = cast();

    if ( p -> is_moving() )
      return false;

    return( p -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Bladestorm ===============================================================

struct bladestorm_tick_t : public warrior_attack_t
{
  bladestorm_tick_t( warrior_t* p, const char* name ) :
    warrior_attack_t( name, 50622, p )
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
    warrior_attack_t( "bladestorm", p, p -> find_class_spell( "Bladestorm" ) ),
    bladestorm_mh( 0 ), bladestorm_oh( 0 )
  {
    // FIXME
    //check_talent( p -> talents.bladestorm -> rank() );

    parse_options( NULL, options_str );

    aoe       = -1;
    harmful   = false;
    channeled = true;
    tick_zero = true;


    cooldown -> duration += p -> glyphs.bladestorm -> effectN( 1 ).time_value();

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

    warrior_t* p = cast();

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
  virtual double haste() { return 1.0; }
  virtual double swing_haste() { return 1.0; }
};

// Bloodthirst Heal ==============================================================

struct bloodthirst_heal_t : public heal_t
{
  bloodthirst_heal_t( warrior_t* p ) :
    heal_t( "bloodthirst_heal", p, p -> find_class_spell( "Bloodthirst" ) )
  {
    // Add Field Dressing Talent, warrior heal etc.

    // Implemented as an actual heal because of spell callbacks ( for Hurricane, etc. )
    background= true;
    init();
  }

  virtual resource_e current_resource() { return RESOURCE_NONE; }
};

struct bloodthirst_buff_callback_t : public action_callback_t
{
  buff_t* buff;
  bloodthirst_heal_t* bloodthirst_heal;

  bloodthirst_buff_callback_t( warrior_t* p, buff_t* b ) :
    action_callback_t( p ), buff( b ),
    bloodthirst_heal( 0 )
  {
    bloodthirst_heal = new bloodthirst_heal_t( p );
  }

  virtual void trigger( action_t* a , void* /* call_data */ )
  {
    if ( buff -> check() && a -> weapon && a -> direct_dmg > 0 )
    {
      bloodthirst_heal -> base_dd_min = bloodthirst_heal -> base_dd_max = bloodthirst_heal -> data().effectN( 2 ).base_value() / 100000.0 * a -> player -> resources.max[ RESOURCE_HEALTH ];
      bloodthirst_heal -> execute();
      buff -> decrement();
    }
  }
};

// Bloodthirst ==============================================================

struct bloodthirst_t : public warrior_attack_t
{
  bloodthirst_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "bloodthirst", p, p -> find_class_spell( "Bloodthirst" ) )
  {
    check_spec( WARRIOR_FURY );

    parse_options( NULL, options_str );

    // Include the weapon so we benefit from racials
    weapon             = &( player -> main_hand_weapon );
    weapon_multiplier  = 0;
    // Bloodthirst can trigger procs from either weapon
    proc_ignores_slot  = true;

    direct_power_mod   = data().effectN( 1 ).average( p ) / 100.0;
    base_dd_min        = 0.0;
    base_dd_max        = 0.0;
    base_multiplier   *= 1.0 + p -> glyphs.bloodthirst -> effectN( 1 ).percent();
    base_crit         += p -> talents.cruelty -> effectN( 1 ).percent();
  }

  virtual void execute()
  {
    warrior_attack_t::execute();

    if ( result_is_hit() )
    {
      warrior_t* p = cast();

      p -> buffs_bloodthirst -> trigger( 3 );

      p -> buffs_battle_trance -> trigger();

      trigger_bloodsurge( this );

      if ( p -> set_bonus.tier13_4pc_melee() && sim -> roll( p -> sets -> set( SET_T13_4PC_MELEE ) -> effectN( 1 ).percent() ) )
      {
        warrior_td_t* td = cast_td();
        td -> debuffs_colossus_smash -> trigger();
        p -> procs_tier13_4pc_melee -> occur();
      }
    }
  }
};

// Charge ===================================================================

struct charge_t : public warrior_attack_t
{
  int use_in_combat;

  charge_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "charge", p, p -> find_class_spell( "Charge" ) ),
    use_in_combat( 0 ) // For now it's not usable in combat by default because we can't modell the distance/movement.
  {
    option_t options[] =
    {
      { "use_in_combat", OPT_BOOL, &use_in_combat },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    cooldown -> duration += p -> talents.juggernaut -> effectN( 3 ).time_value();
    cooldown -> duration += p -> glyphs.rapid_charge -> effectN( 1 ).time_value();

    stancemask = STANCE_BATTLE;

    // FIXME:
    // if ( p -> talents.juggernaut -> rank() || p -> talents.warbringer -> rank() )
    //  stancemask  = STANCE_BERSERKER | STANCE_BATTLE | STANCE_DEFENSE;
  }

  virtual void execute()
  {
    warrior_attack_t::execute();
    warrior_t* p = cast();

    p -> buffs_juggernaut -> trigger();

    if ( p -> position == POSITION_RANGED_FRONT )
      p -> position = POSITION_FRONT;
    else if ( ( p -> position == POSITION_RANGED_BACK ) || ( p -> position == POSITION_MAX ) )
      p -> position = POSITION_BACK;

    p -> resource_gain( RESOURCE_RAGE,
                        data().effectN( 2 ).resource( RESOURCE_RAGE ) + p -> talents.blitz -> effectN( 2 ).resource( RESOURCE_RAGE ),
                        p -> gains_charge );
  }

  virtual bool ready()
  {
    warrior_t* p = cast();

    if ( p -> in_combat )
    {
      // FIXME:
      /* if ( ! ( p -> talents.juggernaut -> rank() || p -> talents.warbringer -> rank() ) )
        return false;

      else */if ( ! use_in_combat )
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
    warrior_attack_t( "cleave", p, p -> find_class_spell( "Cleave" ) )
  {
    parse_options( NULL, options_str );

    direct_power_mod = 0.45;
    base_dd_min      = 6;
    base_dd_max      = 6;

    // Include the weapon so we benefit from racials
    weapon = &( player -> main_hand_weapon );
    weapon_multiplier = 0;

    base_multiplier *= 1.0 + p -> talents.thunderstruck -> effectN( 1 ).percent();

    aoe = 1 +  p -> glyphs.cleaving -> effectN( 1 ).base_value();
  }

  virtual void execute()
  {
    warrior_attack_t::execute();

    warrior_t* p = cast();

    if ( result_is_hit() )
      p -> buffs_meat_cleaver -> trigger();
  }

  virtual void player_buff()
  {
    warrior_attack_t::player_buff();

    warrior_t* p = cast();

    player_multiplier *= 1.0 + p -> talents.meat_cleaver -> effectN( 1 ).percent() * p -> buffs_meat_cleaver -> stack();
  }

  virtual void update_ready()
  {
    warrior_t* p = cast();

    cooldown -> duration = data().cooldown();

    if ( p -> buffs_inner_rage -> up() )
      cooldown -> duration *= 1.0 + p -> buffs_inner_rage -> data().effectN( 1 ).percent();

    warrior_attack_t::update_ready();
  }
};

// Colossus Smash ===========================================================

struct colossus_smash_t : public warrior_attack_t
{
  double armor_pen_value;

  colossus_smash_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "colossus_smash",  p, p -> find_class_spell( "Colossus Smash" ) )
    , armor_pen_value( 0.0 )
  {
    parse_options( NULL, options_str );

    stancemask  = STANCE_BERSERKER | STANCE_BATTLE;

    // FIXME:
    // armor_pen_value = base_value( E_APPLY_AURA, A_345 ) / 100.0;
  }

  virtual void execute()
  {
    warrior_attack_t::execute();

    if ( result_is_hit() )
    {
      warrior_td_t* td = cast_td();
      td -> debuffs_colossus_smash -> trigger( 1, armor_pen_value );

      if ( ! sim -> overrides.physical_vulnerability )
        target -> debuffs.physical_vulnerability -> trigger();
    }
  }
};

// Concussion Blow ==========================================================

struct concussion_blow_t : public warrior_attack_t
{
  concussion_blow_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "concussion_blow", p, p -> find_class_spell( "Concussion Blow" ) )
  {
    // FIXME:
    // check_talent( p -> talents.concussion_blow -> rank() );

    parse_options( NULL, options_str );

    direct_power_mod  = data().effectN( 3 ).percent();
  }
};

// Devastate ================================================================

struct devastate_t : public warrior_attack_t
{
  devastate_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "devastate", p, p -> find_class_spell( "Devastate" ) )
  {
    // FIXME:
    // check_talent( p -> talents.devastate -> rank() );

    parse_options( NULL, options_str );

    base_dd_min = base_dd_max = 0;

    base_multiplier *= 1.0 + p -> talents.war_academy -> effectN( 1 ).percent();

    base_crit += p -> talents.sword_and_board -> effectN( 2 ).percent();
    base_crit += p -> glyphs.devastate -> effectN( 1 ).percent();
  }

  virtual void execute()
  {
    warrior_attack_t::execute();

    //warrior_t* p = cast();

    trigger_sword_and_board( this, result );

    // FIXME:
    /*if ( p -> talents.impending_victory -> rank() && target -> health_percentage() <= 20 )
    {
      if ( p -> rng_impending_victory -> roll( p -> talents.impending_victory -> proc_chance() ) )
        p -> buffs_victory_rush -> trigger();
    }*/
  }

  virtual void impact( player_t* t, result_e impact_result, double travel_dmg )
  {
    warrior_attack_t::impact( t, impact_result, travel_dmg );

    if ( ! sim -> overrides.weakened_blows )
      t -> debuffs.weakened_blows -> trigger();
  }
};

// Execute ==================================================================

struct execute_t : public warrior_attack_t
{
  execute_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "execute", p, p -> find_class_spell( "Execute" ) )
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
    warrior_t* p = cast();

    // Consumes base_cost + 20
    resource_consumed = std::min( p -> resources.current[ current_resource() ], 20.0 + cost() );

    if ( sim -> debug )
      sim -> output( "%s consumes %.1f %s for %s", p -> name(),
                     resource_consumed, util::resource_type_string( current_resource() ), name() );

    player -> resource_loss( current_resource(), resource_consumed );

    stats -> consume_resource( current_resource(), resource_consumed );

    if ( p -> talents.sudden_death -> ok() )
    {
      double current_rage = p -> resources.current[ current_resource() ];
      double sudden_death_rage = p -> talents.sudden_death -> effectN( 1 ).base_value();

      if ( current_rage < sudden_death_rage )
      {
        p -> resource_gain( current_resource(), sudden_death_rage - current_rage, p -> gains_sudden_death ); // FIXME: Do we keep up to 10 or always at least 10?
      }
    }
  }

  virtual void execute()
  {
    warrior_attack_t::execute();
    warrior_t* p = cast();

    if ( result_is_hit() && p -> rng_executioner_talent -> roll( p -> talents.executioner -> proc_chance() ) )
      p -> buffs_executioner_talent -> trigger();
  }

  virtual void player_buff()
  {
    warrior_attack_t::player_buff();
    warrior_t* p = cast();

    // player_buff happens before consume_resource
    // so we can safely check here how much excess rage we will spend
    double base_consumed = cost();
    double max_consumed = std::min( p -> resources.current[ RESOURCE_RAGE ], 20.0 + base_consumed );

    // Damage scales directly with AP per rage since 4.0.1.
    // Can't be derived by parse_data() for now.
    direct_power_mod = 0.0437 * max_consumed;

    player_multiplier *= 1.0 + ( p -> buffs_lambs_to_the_slaughter -> data().ok() ?
                                 ( p -> buffs_lambs_to_the_slaughter -> stack()
                                   * p -> buffs_lambs_to_the_slaughter -> data().effectN( 1 ).percent() ) : 0 );
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
    warrior_attack_t( "heroic_strike", p, p -> find_class_spell( "Heroic Strike" ) )
  {
    parse_options( NULL, options_str );

    // Include the weapon so we benefit from racials
    weapon = &( player -> main_hand_weapon );
    weapon_multiplier = 0;

    base_crit        += p -> talents.incite -> effectN( 1 ).percent();
    base_dd_min       = base_dd_max = 8;
    direct_power_mod  = 0.6; // Hardcoded into tooltip, 02/11/2011
    harmful           = true;
  }

  virtual double cost()
  {
    double c = warrior_attack_t::cost();
    warrior_t* p = cast();

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
    warrior_t* p = cast();

    // Needs testing
    if ( p -> set_bonus.tier13_2pc_melee() )
      p -> buffs_inner_rage -> up();
  }

  virtual void execute()
  {
    warrior_attack_t::execute();
    warrior_t* p = cast();

    if ( p -> buffs_incite -> check() )
      p -> buffs_incite -> expire();

    else if ( result == RESULT_CRIT )
      p -> buffs_incite -> trigger();

  }

  virtual void player_buff()
  {
    warrior_attack_t::player_buff();
    warrior_t* p = cast();

    if ( p -> buffs_incite -> up() )
      player_crit += 1.0;
  }

  virtual void update_ready()
  {
    warrior_t* p = cast();

    cooldown -> duration = data().cooldown();

    if ( p -> buffs_inner_rage -> up() )
      cooldown -> duration *= 1.0 + p -> buffs_inner_rage -> data().effectN( 1 ).percent();

    warrior_attack_t::update_ready();
  }
};

// Heroic Leap ==============================================================

struct heroic_leap_t : public warrior_attack_t
{
  heroic_leap_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "heroic_leap", p, p -> find_class_spell( "Heroic Leap" ) )
  {
    parse_options( NULL, options_str );

    aoe = -1;
    may_dodge = may_parry = false;
    harmful = true; // This should be defaulted to true, but it's not

    // Damage is stored in a trigger spell
    const spell_data_t* dmg_spell = p -> dbc.spell( data().effectN( 3 ).trigger_spell_id() );
    base_dd_min = p -> dbc.effect_min( dmg_spell -> effectN( 1 ).id(), p -> level );
    base_dd_max = p -> dbc.effect_max( dmg_spell -> effectN( 1 ).id(), p -> level );
    direct_power_mod = dmg_spell -> extra_coeff();

    cooldown -> duration += p -> talents.skirmisher -> effectN( 2 ).time_value();

    // Heroic Leap can trigger procs from either weapon
    proc_ignores_slot = true;
  }
};

// Mortal Strike ============================================================

struct mortal_strike_t : public warrior_attack_t
{
  double additive_multipliers;

  mortal_strike_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "mortal_strike", p, p -> find_class_spell( "Mortal Strike" ) ),
    additive_multipliers( 0 )
  {
    check_spec( WARRIOR_ARMS );

    parse_options( NULL, options_str );

    additive_multipliers = p -> glyphs.mortal_strike -> effectN( 1 ).percent()
                           + p -> talents.war_academy -> effectN( 1 ).percent();
    crit_bonus_multiplier *= 1.0 + p -> talents.impale -> effectN( 1 ).percent();
    base_crit             += p -> talents.cruelty -> effectN( 1 ).percent();
  }

  virtual void execute()
  {
    warrior_attack_t::execute();

    warrior_t* p = cast();

    if ( result_is_hit() )
    {
      warrior_td_t* td = cast_td();
      p -> buffs_lambs_to_the_slaughter -> trigger();
      p -> buffs_battle_trance -> trigger();
      p -> buffs_juggernaut -> expire();

      // FIXME:
      /* if ( result == RESULT_CRIT && p -> rng_wrecking_crew -> roll( p -> talents.wrecking_crew -> proc_chance() ) )
      {
        double value = p -> talents.wrecking_crew -> rank() * 0.05;
        p -> buffs_wrecking_crew -> trigger( 1, value );
      }

      if ( p -> talents.lambs_to_the_slaughter -> rank() && td -> dots_rend -> ticking )
        td -> dots_rend -> refresh_duration();
      */

      if ( p -> set_bonus.tier13_4pc_melee() && sim -> roll( p -> sets -> set( SET_T13_4PC_MELEE ) -> proc_chance() ) )
      {
        td -> debuffs_colossus_smash -> trigger();
        p -> procs_tier13_4pc_melee -> occur();
      }
    }
  }

  virtual void impact( player_t* t, result_e impact_result, double travel_dmg )
  {
    warrior_attack_t::impact( t, impact_result, travel_dmg );

    if ( sim -> overrides.mortal_wounds && result_is_hit( impact_result ) )
      t -> debuffs.mortal_wounds -> trigger();
  }

  virtual void player_buff()
  {
    warrior_attack_t::player_buff();

    warrior_t* p = cast();

    if ( p -> buffs_juggernaut -> up() )
      player_crit += p -> buffs_juggernaut -> data().effectN( 1 ).percent();

    player_multiplier *= 1.0 + ( p -> buffs_lambs_to_the_slaughter -> data().ok() ?
                                 ( p -> buffs_lambs_to_the_slaughter -> stack()
                                   * p -> buffs_lambs_to_the_slaughter -> data().effectN( 1 ).percent() ) : 0 )
                         + additive_multipliers;
  }
};

// Overpower ================================================================

struct overpower_t : public warrior_attack_t
{
  overpower_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "overpower", p, p -> find_class_spell( "Overpower" ) )
  {
    parse_options( NULL, options_str );

    may_dodge  = false;
    may_parry  = false;
    may_block  = false; // The Overpower cannot be blocked, dodged or parried.

    base_crit += p -> talents.taste_for_blood -> effectN( 2 ).percent();
    crit_bonus_multiplier *= 1.0 + p -> talents.impale -> effectN( 1 ).percent();
    base_multiplier       *= 1.0 + p -> glyphs.overpower -> effectN( 1 ).percent();

    stancemask = STANCE_BATTLE;
  }

  virtual void execute()
  {
    warrior_t* p = cast();

    // Track some information on what got us the overpower
    // Talents or lack of expertise
    p -> buffs_overpower -> up();
    p -> buffs_taste_for_blood -> up();
    warrior_attack_t::execute();
    p -> buffs_overpower -> expire();
    p -> buffs_taste_for_blood -> expire();
  }

  virtual void player_buff()
  {
    warrior_attack_t::player_buff();
    warrior_t* p = cast();

    player_multiplier *= 1.0 + ( p -> buffs_lambs_to_the_slaughter -> data().ok() ?
                                 ( p -> buffs_lambs_to_the_slaughter -> stack()
                                   * p -> buffs_lambs_to_the_slaughter -> data().effectN( 1 ).percent() ) : 0 );
  }

  virtual bool ready()
  {
    warrior_t* p = cast();

    if ( ! ( p -> buffs_overpower -> check() || p -> buffs_taste_for_blood -> check() ) )
      return false;

    return warrior_attack_t::ready();
  }
};

// Pummel ===================================================================

struct pummel_t : public warrior_attack_t
{
  pummel_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "pummel", p, p -> find_class_spell( "Pummel" ) )
  {
    parse_options( NULL, options_str );

    base_costs[ current_resource() ] *= 1.0 + p -> talents.drums_of_war -> effectN( 1 ).percent();

    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
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
    base_multiplier *= 1.0 + p -> talents.war_academy -> effectN( 1 ).percent();
    base_crit += p -> glyphs.raging_blow -> effectN( 1 ).percent();
  }

  virtual void player_buff()
  {
    warrior_attack_t::player_buff();
    warrior_t* p = cast();

    player_multiplier *= 1.0 + p -> composite_mastery() *
                         p -> mastery.unshackled_fury -> effectN( 3 ).base_value() / 10000.0;
  }
};

struct raging_blow_t : public warrior_attack_t
{
  raging_blow_attack_t* mh_attack;
  raging_blow_attack_t* oh_attack;

  raging_blow_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "raging_blow", p, p -> find_class_spell( "Raging Blow" ) ),
    mh_attack( 0 ), oh_attack( 0 )
  {
    // FIXME:
    // check_talent( p -> talents.raging_blow -> rank() );

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

    if ( result_is_hit() )
    {
      mh_attack -> execute();
      if ( oh_attack )
      {
        oh_attack -> execute();
      }
    }
  }

  virtual bool ready()
  {
    warrior_t* p = cast();

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

    weapon                 = &( p -> main_hand_weapon );
    normalize_weapon_speed = false;
    base_td_init = base_td;

    base_multiplier       *= 1.0 + p -> talents.thunderstruck -> effectN( 1 ).percent();

  }

  virtual double calculate_direct_damage( result_e, int, double, double, double, player_t* )
  {
    // Rend doesn't actually hit with the weapon, but ticks on application
    return 0.0;
  }

  virtual void execute()
  {
    base_td = base_td_init;
    if ( weapon )
      base_td += ( sim -> range( weapon -> min_dmg, weapon -> max_dmg ) + weapon -> swing_time.total_seconds() * weapon_power_mod * total_attack_power() ) * 0.25;

    warrior_attack_t::execute();
  }

  virtual void tick( dot_t* d )
  {
    warrior_attack_t::tick( d );
    warrior_t* p = cast();

    p -> buffs_taste_for_blood -> trigger();
  }
};

struct rend_t : public warrior_attack_t
{
  rend_dot_t* rend_dot;

  rend_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "rend", p, p -> find_class_spell( "Rend" ) ),
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
    warrior_attack_t( "revenge", p, p -> find_class_spell( "Revenge" ) ),
    absorb_stats( 0 )
  {
    parse_options( NULL, options_str );

    base_multiplier  *= 1.0 + p -> talents.improved_revenge -> effectN( 2 ).percent()
                        + p -> glyphs.revenge -> effectN( 1 ).percent();

    direct_power_mod = data().extra_coeff();
    stancemask = STANCE_DEFENSE;

    // FIXME:
    /* if ( p -> talents.improved_revenge -> rank() )
    {
      aoe = 1;
      base_add_multiplier = p -> talents.improved_revenge -> rank() * 0.50;
    }*/

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
    warrior_t* p = cast();

    p -> buffs_revenge -> expire();

    trigger_sword_and_board( this, result );
  }

  virtual void impact( player_t* t, result_e impact_result, double travel_dmg )
  {
    warrior_attack_t::impact( t, impact_result, travel_dmg );
    warrior_t* p = cast();

    // Needs testing
    if ( absorb_stats )
    {
      if ( result_is_hit( impact_result ) )
      {
        double amount = 0.20 * travel_dmg;
        p -> buffs_tier13_2pc_tank -> trigger( 1, amount );
        absorb_stats -> add_result( amount, amount, ABSORB, impact_result );
        absorb_stats -> add_execute( timespan_t::zero() );
      }
    }
  }

  virtual bool ready()
  {
    warrior_t* p = cast();

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
    warrior_attack_t( "shattering_throw", p, p -> find_class_spell( "Shattering Throw" ) )
  {
    parse_options( NULL, options_str );

    direct_power_mod = data().extra_coeff();
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
    warrior_attack_t( "shield_bash", p, p -> find_class_spell( "Shield Bash" ) )
  {
    parse_options( NULL, options_str );

    base_costs[ current_resource() ] *= 1.0 + p -> talents.drums_of_war -> effectN( 1 ).percent();

    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;

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
    warrior_attack_t( "shield_slam", p, p -> find_class_spell( "Shield Slam" ) )
  {
    check_spec( WARRIOR_PROTECTION );

    parse_options( NULL, options_str );

    base_crit        += p -> talents.cruelty -> effectN( 1 ).percent();
    base_multiplier  *= 1.0  + p -> glyphs.shield_slam -> effectN( 1 ).percent();

    stats -> add_child( player -> get_stats( "shield_slam_combust" ) );
  }

  virtual void player_buff()
  {
    warrior_attack_t::player_buff();

    warrior_t* p = cast();

    if ( p -> buffs_shield_block -> up() )
      player_multiplier *= 1.0 + p -> talents.heavy_repercussions -> effectN( 1 ).percent();
  }

  virtual void execute()
  {
    warrior_attack_t::execute();
    warrior_t* p = cast();

    p -> buffs_sword_and_board -> expire();

    if ( result_is_hit() )
      p -> buffs_battle_trance -> trigger();
  }

  virtual double cost()
  {
    warrior_t* p = cast();

    if ( p -> buffs_sword_and_board -> check() )
      return 0;

    return warrior_attack_t::cost();
  }
};

// Shockwave ================================================================

struct shockwave_t : public warrior_attack_t
{
  shockwave_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "shockwave", p, p -> find_class_spell( "Shockwave" ) )
  {
    // FIXME:
    // check_talent( p -> talents.shockwave -> rank() );

    parse_options( NULL, options_str );

    direct_power_mod  = data().effectN( 3 ).percent();
    may_dodge         = false;
    may_parry         = false;
    may_block         = false;
    cooldown -> duration += p -> glyphs.shockwave -> effectN( 1 ).time_value();
  }

  virtual void execute()
  {
    warrior_attack_t::execute();
    warrior_t* p = cast();

    p -> buffs_thunderstruck -> expire();
  }

  virtual void player_buff()
  {
    warrior_attack_t::player_buff();
    warrior_t* p = cast();

    player_multiplier *= 1.0 + p -> buffs_thunderstruck -> stack() *
                         p -> talents.thunderstruck -> effectN( 2 ).percent();
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
    may_miss = may_dodge = may_parry = false;
    background = true;

    base_crit             += p -> glyphs.slam -> effectN( 1 ).percent();
    crit_bonus_multiplier *= 1.0 + p -> talents.impale -> effectN( 1 ).percent();

    additive_multipliers = p -> talents.improved_slam -> effectN( 2 ).percent()
                           + p -> talents.war_academy -> effectN( 1 ).percent();
  }

  virtual void player_buff()
  {
    warrior_attack_t::player_buff();
    warrior_t* p = cast();

    if ( p -> buffs_juggernaut -> up() )
      player_crit += p -> buffs_juggernaut -> data().effectN( 1 ).percent();

    if ( p -> buffs_bloodsurge -> up() )
      player_multiplier *= 1.0 + p -> talents.bloodsurge -> effectN( 1 ).percent();

    player_multiplier *= 1.0 + ( p -> buffs_lambs_to_the_slaughter -> data().ok() ?
                                 ( p -> buffs_lambs_to_the_slaughter -> stack()
                                   * p -> buffs_lambs_to_the_slaughter -> data().effectN( 1 ).percent() ) : 0 )
                         + additive_multipliers;
  }
};

struct slam_t : public warrior_attack_t
{
  attack_t* mh_attack;
  attack_t* oh_attack;

  slam_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "slam", p, p -> find_class_spell( "Slam" ) ),
    mh_attack( 0 ), oh_attack( 0 )
  {
    parse_options( NULL, options_str );

    base_execute_time += p -> talents.improved_slam -> effectN( 1 ).time_value();
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

  virtual double cost()
  {
    warrior_t* p = cast();

    if ( p -> buffs_bloodsurge -> check() )
      return 0;

    return warrior_attack_t::cost();
  }

  virtual timespan_t execute_time()
  {
    warrior_t* p = cast();

    if ( p -> buffs_bloodsurge -> check() )
      return timespan_t::zero();

    return warrior_attack_t::execute_time();
  }

  virtual void execute()
  {
    attack_t::execute();
    warrior_t* p = cast();

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

    base_costs[ current_resource() ] *= 1.0 + p -> glyphs.furious_sundering -> effectN( 1 ).percent();
    background = ( sim -> overrides.weakened_armor != 0 );
    // TODO: Glyph of Sunder armor applies affect to nearby target
  }

  virtual void impact( player_t* t, result_e impact_result, double travel_dmg )
  {
    warrior_attack_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
    {
      if ( ! sim -> overrides.weakened_armor )
        t -> debuffs.weakened_armor -> trigger();
    }
  }
};

// Thunder Clap =============================================================

struct thunder_clap_t : public warrior_attack_t
{
  thunder_clap_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "thunder_clap", p, p -> find_class_spell( "Thunder Clap" ) )
  {
    parse_options( NULL, options_str );

    aoe               = -1;
    may_dodge         = false;
    may_parry         = false;
    may_block         = false;
    direct_power_mod  = data().extra_coeff();
    base_multiplier  *= 1.0 + p -> talents.thunderstruck -> effectN( 1 ).percent();
    base_crit        += p -> talents.incite -> effectN( 1 ).percent();
    base_costs[ current_resource() ]        += p -> glyphs.resonating_power -> effectN( 1 ).resource( RESOURCE_RAGE );

    // TC can trigger procs from either weapon, even though it doesn't need a weapon
    proc_ignores_slot = true;
  }

  virtual void execute()
  {
    warrior_attack_t::execute();
    warrior_t* p = cast();
    //warrior_td_t* td = cast_td();

    p -> buffs_thunderstruck -> trigger();

    // FIXME:
    // if ( p -> talents.blood_and_thunder -> rank() && td -> dots_rend && td -> dots_rend -> ticking )
    //  td -> dots_rend -> refresh_duration();

    if ( ! sim -> overrides.weakened_blows )
      target -> debuffs.weakened_blows -> trigger();
  }
};

// Whirlwind ================================================================

struct whirlwind_t : public warrior_attack_t
{
  whirlwind_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "whirlwind", p, p -> find_class_spell( "Whirlwind" ) )
  {
    parse_options( NULL, options_str );

    aoe               = -1;
    stancemask        = STANCE_BERSERKER;
  }

  virtual void consume_resource() { }

  virtual void execute()
  {
    warrior_t* p = cast();

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
    warrior_t* p = cast();

    if ( p -> buffs_meat_cleaver -> up() )
    {
      // Caution: additive
      // FIXME:
      // player_multiplier *= 1.0 + p -> talents.meat_cleaver -> rank() * 0.05 * p -> buffs_meat_cleaver -> stack();
    }
  }
};

// Victory Rush =============================================================

struct victory_rush_t : public warrior_attack_t
{
  victory_rush_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "victory_rush", p, p -> find_class_spell( "Victory Rush" ) )
  {
    parse_options( NULL, options_str );

    base_multiplier *= 1.0 + p -> talents.war_academy -> effectN( 1 ).percent();
  }

  virtual bool ready()
  {
    warrior_t* p = cast();

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
  warrior_spell_t( const std::string& n, warrior_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    spell_t( n, p, s ),
    stancemask( STANCE_BATTLE|STANCE_BERSERKER|STANCE_DEFENSE )
  {
    _init_warrior_spell_t();
  }

  warrior_spell_t( const std::string& n, uint32_t id, warrior_t* p ) :
    spell_t( n, p, p -> find_spell( id ) ),
    stancemask( STANCE_BATTLE|STANCE_BERSERKER|STANCE_DEFENSE )
  {
    _init_warrior_spell_t();
  }


  warrior_t* cast() const
  { return debug_cast<warrior_t*>( player ); }

  warrior_td_t* cast_td( player_t* t = 0 ) const
  { return cast() -> get_target_data( t ? t : target ); }

  void _init_warrior_spell_t()
  {
  }

  virtual timespan_t gcd()
  {
    // Unaffected by haste
    return trigger_gcd;
  }

  virtual bool ready()
  {
    warrior_t* p = cast();

    // Attack vailable in current stance?
    if ( ( stancemask & p -> active_stance ) == 0 )
      return false;

    return spell_t::ready();
  }
};

// Battle Shout =============================================================

struct battle_shout_t : public warrior_spell_t
{
  double rage_gain;

  battle_shout_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "battle_shout", p, p -> find_class_spell( "Battle Shout" ) )
  {
    parse_options( NULL, options_str );

    harmful   = false;

    // FIXME:
    // rage_gain = p -> dbc.spell( effectN( 3 ).trigger_spell_id() ) -> effectN( 1 ).resource( RESOURCE_RAGE ) + p -> talents.booming_voice -> effectN( 2 ).resource( RESOURCE_RAGE );
    cooldown = player -> get_cooldown( "shout" );
    // FIXME:
    // cooldown -> duration = timespan_t::from_seconds( 10 ) + spell_id_t::cooldown() + p -> talents.booming_voice -> effectN( 1 ).time_value();
  }

  virtual void execute()
  {
    warrior_spell_t::execute();

    warrior_t* p = cast();

    if ( ! sim -> overrides.attack_power_multiplier )
      sim -> auras.attack_power_multiplier -> trigger( 1, -1.0, -1.0, data().duration() );

    p -> resource_gain( RESOURCE_RAGE, rage_gain , p -> gains_battle_shout );
  }
};

// Commanding Shout =============================================================

struct commanding_shout_t : public warrior_spell_t
{
  double rage_gain;

  commanding_shout_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "commanding_shout", p, p -> find_class_spell( "Commanding Shout" ) )
  {
    parse_options( NULL, options_str );

    harmful   = false;

    rage_gain = 20 + p -> talents.booming_voice -> effectN( 2 ).resource( RESOURCE_RAGE );

    cooldown = player -> get_cooldown( "shout" );
    cooldown -> duration = data().cooldown() + p -> talents.booming_voice -> effectN( 1 ).time_value();
  }

  virtual void execute()
  {
    warrior_spell_t::execute();

    warrior_t* p = cast();

    if ( ! sim -> overrides.stamina )
      sim -> auras.stamina -> trigger( 1, -1.0, -1.0, data().duration() );

    p -> resource_gain( RESOURCE_RAGE, rage_gain , p -> gains_commanding_shout );
  }
};

// Berserker Rage ===========================================================

struct berserker_rage_t : public warrior_spell_t
{
  berserker_rage_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "berserker_rage", p, p -> find_class_spell( "Berserker Rage" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;

    if ( p -> talents.intensify_rage -> ok() )
      cooldown -> duration *= ( 1.0 + p -> talents.intensify_rage -> effectN( 1 ).percent() );
  }

  virtual void execute()
  {
    warrior_spell_t::execute();

    warrior_t* p = cast();

    if ( p -> glyphs.berserker_rage -> ok() )
    {
      double ragegain = 5.0;
      ragegain *= 1.0 + p -> composite_mastery() * p -> mastery.unshackled_fury -> effectN( 3 ).percent() / 100.0;
      p -> resource_gain( RESOURCE_RAGE, ragegain, p -> gains_berserker_rage );
    }

    p -> buffs_berserker_rage -> trigger();
  }
};

// Deadly Calm ==============================================================

struct deadly_calm_t : public warrior_spell_t
{
  deadly_calm_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "deadly_calm", p, p -> find_spell( 85730 ) )
  {
    // FIXME:
    // check_talent( p -> talents.deadly_calm -> rank() );

    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
    warrior_t* p = cast();

    p -> buffs_deadly_calm -> trigger();
  }

  virtual bool ready()
  {
    warrior_t* p = cast();

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
    warrior_spell_t( "death_wish", p, p -> find_class_spell( "Death Wish" ) ),
    enrage_bonus( 0 )
  {
    // FIXME:
    // check_talent( p -> talents.death_wish -> rank() );

    parse_options( NULL, options_str );

    harmful = false;

    if ( p -> talents.intensify_rage -> ok() )
      cooldown -> duration *= ( 1.0 + p -> talents.intensify_rage -> effectN( 1 ).percent() );
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
    warrior_t* p = cast();

    enrage_bonus = p -> talents.death_wish -> effectN( 1 ).percent();
    enrage_bonus *= 1.0 + p -> composite_mastery() * p -> mastery.unshackled_fury -> effectN( 3 ).percent() / 100.0;

    p -> buffs_death_wish -> trigger( 1, enrage_bonus );
    p -> buffs_enrage -> expire();
  }
};

// Inner Rage ===============================================================

struct inner_rage_t : public warrior_spell_t
{
  inner_rage_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "inner_rage", p, p -> find_class_spell( "Inner Rage" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
    warrior_t* p = cast();

    p -> buffs_inner_rage -> trigger();
  }
};

// Recklessness =============================================================

struct recklessness_t : public warrior_spell_t
{
  recklessness_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "recklessness", p, p -> find_class_spell( "Recklessness" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;

    cooldown -> duration *= 1.0 + p -> talents.intensify_rage -> effectN( 1 ).percent();
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
    warrior_t* p = cast();

    p -> buffs_recklessness -> trigger( 3 );
  }

  virtual bool ready()
  {
    warrior_t* p = cast();

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
    warrior_t* p = cast();

    p -> buffs_retaliation -> trigger( 20 );
  }
};

// Shield Block =============================================================

struct shield_block_buff_t : public buff_t
{
  shield_block_buff_t( warrior_t* p ) :
    buff_t( buff_creator_t( p, "shield_block", p -> find_spell( 2565 )  ) )
  { }
};

struct shield_block_t : public warrior_spell_t
{
  shield_block_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "shield_block", p, p -> find_class_spell( "Shield Block" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;

    if ( p -> talents.shield_mastery -> ok() )
      cooldown -> duration += p -> talents.shield_mastery -> effectN( 1 ).time_value();
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
    warrior_t* p = cast();

    p -> buffs_shield_block -> trigger();
  }
};

// Stance ===================================================================

struct stance_t : public warrior_spell_t
{
  warrior_stance switch_to_stance;
  std::string stance_str;

  stance_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "stance", p ),
    switch_to_stance( STANCE_BATTLE ), stance_str( "" )
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

    harmful = false;
    trigger_gcd = timespan_t::zero();
    cooldown -> duration = timespan_t::from_seconds( 1.0 );
  }

  virtual resource_e current_resource() { return RESOURCE_RAGE; }

  virtual void execute()
  {
    warrior_t* p = cast();

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

  virtual double cost()
  {
    warrior_t* p = cast();

    double c = p -> resources.current [ current_resource() ];

    c -= 25.0; // Stance Mastery

    if ( p -> talents.tactical_mastery -> ok() )
      c -= p -> talents.tactical_mastery -> effectN( 1 ).base_value();

    if ( c < 0 ) c = 0;

    return c;
  }

  virtual bool ready()
  {
    warrior_t* p = cast();

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
    // FIXME:
    // check_talent( p -> talents.sweeping_strikes -> rank() );

    parse_options( NULL, options_str );

    harmful = false;

    base_costs[ current_resource() ] *= 1.0 + p -> glyphs.sweeping_strikes -> effectN( 1 ).percent();

    stancemask = STANCE_BERSERKER | STANCE_BATTLE;
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
    warrior_t* p = cast();

    p -> buffs_sweeping_strikes -> trigger();
  }
};

// Last Stand ===============================================================

struct last_stand_t : public warrior_spell_t
{
  last_stand_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "last_stand", 12975, p )
  {
    // FIXME:
    // check_talent( p -> talents.last_stand -> rank() );

    harmful = false;

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
    warrior_t* p = cast();

    p -> buffs_last_stand -> trigger();
  }
};

struct buff_last_stand_t : public buff_t
{
  int health_gain;

  buff_last_stand_t( warrior_t* p, const uint32_t id, const std::string& n ) :
    buff_t( buff_creator_t( p, n, p -> find_spell( id ) ) ), health_gain( 0 )
  { }

  virtual bool trigger( int stacks, double value, double chance, timespan_t duration )
  {
    health_gain = ( int ) floor( player -> resources.max[ RESOURCE_HEALTH ] * 0.3 );
    player -> stat_gain( STAT_MAX_HEALTH, health_gain );

    return buff_t::trigger( stacks, value, chance, duration );
  }

  virtual void expire()
  {
    player -> stat_loss( STAT_MAX_HEALTH, health_gain );

    buff_t::expire();
  }
};

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
  if ( name == "devastate"          ) return new devastate_t          ( this, options_str );
  if ( name == "execute"            ) return new execute_t            ( this, options_str );
  if ( name == "heroic_leap"        ) return new heroic_leap_t        ( this, options_str );
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

// warrior_t::init_spells ===================================================

void warrior_t::init_spells()
{
  player_t::init_spells();

  // Mastery
  mastery.critical_block         = find_mastery_spell( WARRIOR_PROTECTION );
  mastery.strikes_of_opportunity = find_mastery_spell( WARRIOR_ARMS );
  mastery.unshackled_fury        = find_mastery_spell( WARRIOR_FURY );

  // Spec Passives
  spec.anger_management                 = find_specialization_spell( "Anger Management" );
  spec.dual_wield_specialization        = find_specialization_spell( "dual_wield_specialization" );
  spec.precision                        = find_specialization_spell( "precision" );
  spec.sentinel                         = find_specialization_spell( "sentinel" );
  spec.two_handed_weapon_specialization = find_specialization_spell( "two_handed_weapon_specialization" );

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


  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //  C2P    C4P     M2P     M4P     T2P     T4P    H2P    H4P
    {     0,     0, 105797, 105907, 105908, 105911,     0,     0 }, // Tier13
    {     0,     0,      0,      0,      0,      0,     0,     0 },
  };

  sets = new set_bonus_array_t( this, set_bonuses );
}

// warrior_t::init_defense ==================================================

void warrior_t::init_defense()
{
  player_t::init_defense();

  initial.parry_rating_per_strength = 0.27;
}

// warrior_t::init_base =====================================================

void warrior_t::init_base()
{
  player_t::init_base();

  resources.base[  RESOURCE_RAGE  ] = 100;

  initial.attack_power_per_strength = 2.0;
  initial.attack_power_per_agility  = 0.0;

  base.attack_power = level * 2 + 60;

  // FIXME! Level-specific!
  base.miss    = 0.05;
  base.parry   = 0.05;
  base.block   = 0.05;

  if ( specialization() == WARRIOR_PROTECTION )
    vengeance.enabled = true;

  if ( talents.toughness -> ok() )
    initial.armor_multiplier *= 1.0 + talents.toughness -> effectN( 1 ).percent();

  diminished_kfactor    = 0.009560;
  diminished_dodge_capi = 0.01523660;
  diminished_parry_capi = 0.01523660;

  if ( spec.sentinel -> ok() )
  {
    initial.attribute_multiplier[ ATTR_STAMINA ] *= 1.0  + spec.sentinel -> effectN( 1 ).percent();
    base.block += spec.sentinel -> effectN( 3 ).percent();
  }

  base_gcd = timespan_t::from_seconds( 1.5 );
}

// warrior_t::init_scaling ==================================================

void warrior_t::init_scaling()
{
  player_t::init_scaling();

  if ( talents.single_minded_fury -> ok() || talents.titans_grip -> ok() )
  {
    scales_with[ STAT_WEAPON_OFFHAND_DPS    ] = true;
    scales_with[ STAT_WEAPON_OFFHAND_SPEED  ] = sim -> weapon_speed_scale_factors != 0;
    scales_with[ STAT_HIT_RATING2           ] = true;
  }

  if ( primary_role() == ROLE_TANK )
  {
    scales_with[ STAT_PARRY_RATING ] = true;
    scales_with[ STAT_BLOCK_RATING ] = true;
  }
}

// warrior_t::init_buffs ====================================================

void warrior_t::init_buffs()
{
  //timespan_t duration = timespan_t::from_seconds( 12.0 );

  player_t::init_buffs();

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
#if 0
  buffs_bastion_of_defense        = new buff_t( this, "bastion_of_defense",        1, timespan_t::from_seconds( 12.0 ),   timespan_t::zero(), talents.bastion_of_defense -> proc_chance() );
  buffs_battle_stance             = new buff_t( this, 21156, "battle_stance" );
  buffs_battle_trance             = new buff_t( this, "battle_trance",             1, timespan_t::from_seconds( 15.0 ),   timespan_t::zero(), talents.battle_trance -> proc_chance() );
  buffs_berserker_rage            = new buff_t( this, "berserker_rage",            1, timespan_t::from_seconds( 10.0 ) );
  buffs_berserker_stance          = new buff_t( this, 7381, "berserker_stance" );
  buffs_bloodsurge                = new buff_t( this, "bloodsurge",                1, timespan_t::from_seconds( 10.0 ),   timespan_t::zero(), talents.bloodsurge -> proc_chance() );
  buffs_bloodthirst               = new buff_t( this, 23885, "bloodthirst" );
  buffs_deadly_calm               = new buff_t( this, "deadly_calm",               1, timespan_t::from_seconds( 10.0 ) );
  buffs_death_wish                = new buff_t( this, "death_wish",                1, timespan_t::from_seconds( 30.0 ) );
  buffs_defensive_stance          = new buff_t( this, 7376, "defensive_stance" );
  buffs_enrage                    = new buff_t( this, talents.enrage -> effect_trigger_spell( 1 ), "enrage",  talents.enrage -> proc_chance() );
  buffs_executioner_talent        = new buff_t( this, talents.executioner -> effect_trigger_spell( 1 ), "executioner_talent", talents.executioner -> proc_chance() );
  buffs_flurry                    = new buff_t( this, talents.flurry -> effect_trigger_spell( 1 ), "flurry", talents.flurry -> proc_chance() );
  buffs_hold_the_line             = new buff_t( this, "hold_the_line",             1, timespan_t::from_seconds( 5.0 * talents.hold_the_line -> rank() ) );
  buffs_incite                    = new buff_t( this, "incite",                    1, timespan_t::from_seconds( 10.0 ), timespan_t::zero(), talents.incite -> proc_chance() );
  buffs_inner_rage                = new buff_t( this, 1134, "inner_rage" );
  buffs_overpower                 = new buff_t( this, "overpower",                 1, timespan_t::from_seconds( 6.0 ), timespan_t::from_seconds( 1.5 ) );
  buffs_juggernaut                = new buff_t( this, 65156, "juggernaut", talents.juggernaut -> proc_chance() ); //added by hellord
  buffs_lambs_to_the_slaughter    = new buff_t( this, talents.lambs_to_the_slaughter -> effectN( 1 ).trigger_spell_id(), "lambs_to_the_slaughter", talents.lambs_to_the_slaughter -> proc_chance() );
  buffs_last_stand                = new buff_last_stand_t( this, 12976, "last_stand" );
  buffs_meat_cleaver              = new buff_t( this, "meat_cleaver",              3, timespan_t::from_seconds( 10.0 ), timespan_t::zero(), talents.meat_cleaver -> proc_chance() );
  buffs_recklessness              = new buff_t( this, "recklessness",              3, timespan_t::from_seconds( 12.0 ) );
  buffs_retaliation               = new buff_t( this, 20230, "retaliation" );
  buffs_revenge                   = new buff_t( this, "revenge",                   1, timespan_t::from_seconds( 5.0 ) );
  buffs_rude_interruption         = new buff_t( this, "rude_interruption",         1, timespan_t::from_seconds( 15.0 * talents.rude_interruption ->rank() ) );
  buffs_sweeping_strikes          = new buff_t( this, "sweeping_strikes",          1, timespan_t::from_seconds( 10.0 ) );
  buffs_sword_and_board           = new buff_t( this, "sword_and_board",           1, timespan_t::from_seconds( 5.0 ), timespan_t::zero(), talents.sword_and_board -> proc_chance() );
  buffs_taste_for_blood           = new buff_t( this, "taste_for_blood",           1, timespan_t::from_seconds( 9.0 ), timespan_t::from_seconds( 5.0 ), talents.taste_for_blood -> proc_chance() );
  buffs_thunderstruck             = new buff_t( this, "thunderstruck",             3, timespan_t::from_seconds( 20.0 ), timespan_t::zero(), talents.thunderstruck -> proc_chance() );
  buffs_victory_rush              = new buff_t( this, "victory_rush",              1, timespan_t::from_seconds( 20.0 ) );
  buffs_wrecking_crew             = new buff_t( this, "wrecking_crew",             1, timespan_t::from_seconds( 12.0 ), timespan_t::zero() );
  buffs_tier13_2pc_tank           = new buff_t( this, "tier13_2pc_tank", 1, timespan_t::from_seconds( 15.0 ) /* assume some short duration */, timespan_t::from_seconds( set_bonus.tier13_2pc_tank() ) );
  absorb_buffs.push_back( buffs_tier13_2pc_tank );

  buffs_shield_block              = new shield_block_buff_t( this );
  switch ( talents.booming_voice -> rank() )
  {
  case 1 : duration = timespan_t::from_seconds( 9.0 ); break;
  case 2 : duration = timespan_t::from_seconds( 6.0 ); break;
  default: duration = timespan_t::from_seconds( 12.0 ); break;
  }
#endif
}

// warrior_t::init_values ====================================================

void warrior_t::init_values()
{
  player_t::init_values();

  if ( set_bonus.pvp_2pc_melee() )
    initial.attribute[ ATTR_STRENGTH ]   += 70;

  if ( set_bonus.pvp_4pc_melee() )
    initial.attribute[ ATTR_STRENGTH ]   += 90;
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
    clear_action_priority_lists();

    switch ( specialization() )
    {
    case WARRIOR_FURY:
    case WARRIOR_ARMS:
      // Flask
      if ( level > 85 )
        action_list_str += "/flask,type=winters_bite,precombat=1";
      else if ( level >= 80 )
        action_list_str += "/flask,type=titanic_strength,precombat=1";

      // Food
      if ( level >= 80 )
        action_list_str += "/food,type=great_pandaren_banquet,precombat=1";
      else if ( level >= 70 )
        action_list_str += "/food,type=beer_basted_crocolisk,precombat=1";

      break;

    case WARRIOR_PROTECTION:
      // Flask
      if ( level >= 80 )
        action_list_str += "/flask,type=earth,precombat=1";
      else if ( level >= 75 )
        action_list_str += "/flask,type=steelskin,precombat=1";

      // Food
      if ( level >= 80 )
        action_list_str += "/food,type=great_pandaren_banquet,precombat=1";
      else if ( level >= 70 )
        action_list_str += "/food,type=beer_basted_crocolisk,precombat=1";

    break; default: break;
    }

    action_list_str += "/snapshot_stats,precombat=1";

    // Potion
    if ( specialization() == WARRIOR_ARMS )
    {
      if ( level > 85 )
        action_list_str += "/mogu_power_potion,precombat=1/mogu_power_potion,if=buff.recklessness.up|target.time_to_die<26";
      else if ( level >= 80 )
        action_list_str += "/golemblood_potion,precombat=1/golemblood_potion,if=buff.recklessness.up|target.time_to_die<26";
    }
    else if ( specialization() == WARRIOR_FURY )
    {
      if ( level > 85 )
        action_list_str += "/mogu_power_potion,precombat=1/mogu_power_potion,if=buff.bloodlust.react";
      else if ( level >= 80 )
        action_list_str += "/golemblood_potion,precombat=1/golemblood_potion,if=buff.bloodlust.react";
    }
    else
    {
      if ( level > 85 )
        action_list_str += "/mountains_potion,precombat=1/mountains_potion,if=health_pct<35&buff.mountains_potion.down";
      else if ( level >= 80 )
        action_list_str += "/earthen_potion,precombat=1/earthen_potion,if=health_pct<35&buff.earthen_potion.down";
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

    // Heroic Leap, for everyone but tanks
    if ( primary_role() != ROLE_TANK )
      action_list_str += "/heroic_leap,use_off_gcd=1,if=buff.colossus_smash.up";

    // Arms
    if ( specialization() == WARRIOR_ARMS )
    {
      if ( glyphs.berserker_rage -> ok() ) action_list_str += "/berserker_rage,if=buff.deadly_calm.down&cooldown.deadly_calm.remains>1.5&rage<=95,use_off_gcd=1";
      if ( talents.deadly_calm -> ok() ) action_list_str += "/deadly_calm,use_off_gcd=1";
      action_list_str += "/inner_rage,if=buff.deadly_calm.down&cooldown.deadly_calm.remains>15,use_off_gcd=1";
      action_list_str += "/recklessness,if=target.health_pct>90|target.health_pct<=20,use_off_gcd=1";
      action_list_str += "/stance,choose=berserker,if=buff.taste_for_blood.down&dot.rend.remains>0&rage<=75,use_off_gcd=1";
      action_list_str += "/stance,choose=battle,use_off_gcd=1,if=!dot.rend.ticking";
      action_list_str += "/stance,choose=battle,use_off_gcd=1,if=(buff.taste_for_blood.up|buff.overpower.up)&rage<=75&cooldown.mortal_strike.remains>=1.5,use_off_gcd=1";
      if ( talents.sweeping_strikes -> ok() ) action_list_str += "/sweeping_strikes,if=target.adds>0,use_off_gcd=1";
      action_list_str += "/cleave,if=target.adds>0,use_off_gcd=1";
      action_list_str += "/rend,if=!ticking";
      // Don't want to bladestorm during SS as it's only 1 extra hit per WW not per target
      action_list_str += "/bladestorm,if=target.adds>0&!buff.deadly_calm.up&!buff.sweeping_strikes.up";
      action_list_str += "/mortal_strike,if=target.health_pct>20";
      if ( level >= 81 ) action_list_str += "/colossus_smash,if=buff.colossus_smash.down";
      if ( talents.executioner -> ok() )
        action_list_str += "/execute,if=buff.executioner_talent.remains<1.5";
      action_list_str += "/mortal_strike,if=target.health_pct<=20&(dot.rend.remains<3|buff.wrecking_crew.down|rage<=25|rage>=35)";
      action_list_str += "/execute,if=rage>90";
      action_list_str += "/overpower,if=buff.taste_for_blood.up|buff.overpower.up";
      action_list_str += "/execute";
      action_list_str += "/colossus_smash,if=buff.colossus_smash.remains<=1.5";
      action_list_str += "/slam,if=(rage>=35|buff.battle_trance.up|buff.deadly_calm.up)";
      if ( talents.deadly_calm -> ok() )
        action_list_str += "/heroic_strike,use_off_gcd=1,if=buff.deadly_calm.up";
      action_list_str += "/heroic_strike,use_off_gcd=1,if=rage>85";
      action_list_str += "/heroic_strike,use_off_gcd=1,if=buff.inner_rage.up&target.health_pct>20&(rage>=60|(set_bonus.tier13_2pc_melee&rage>=50))";
      action_list_str += "/heroic_strike,use_off_gcd=1,if=buff.inner_rage.up&target.health_pct<=20&((rage>=60|(set_bonus.tier13_2pc_melee&rage>=50))|buff.battle_trance.up)";
      action_list_str += "/battle_shout,if=rage<60|!aura.attack_power_multiplier.up";
    }

    // Fury
    else if ( specialization() == WARRIOR_FURY )
    {
      action_list_str += "/stance,choose=berserker";
      if ( talents.titans_grip -> ok() )
      {
        action_list_str += "/recklessness,use_off_gcd=1";
        if ( talents.death_wish -> ok() ) action_list_str += "/death_wish,use_off_gcd=1";
        action_list_str += "/cleave,if=target.adds>0,use_off_gcd=1";
        action_list_str += "/whirlwind,if=target.adds>0";
        if ( set_bonus.tier13_2pc_melee() )
          action_list_str += "/inner_rage,use_off_gcd=1,if=target.adds=0&((rage>=75&target.health_pct>=20)|((buff.incite.up|buff.colossus_smash.up)&((rage>=40&target.health_pct>=20)|(rage>=65&target.health_pct<20))))";
        action_list_str += "/heroic_strike,use_off_gcd=1,if=(rage>=85|(set_bonus.tier13_2pc_melee&buff.inner_rage.up&rage>=75))&target.health_pct>=20";
        action_list_str += "/heroic_strike,use_off_gcd=1,if=buff.battle_trance.up";
        action_list_str += "/heroic_strike,use_off_gcd=1,if=(buff.incite.up|buff.colossus_smash.up)&(((rage>=50|(rage>=40&set_bonus.tier13_2pc_melee&buff.inner_rage.up))&target.health_pct>=20)|((rage>=75|(rage>=65&set_bonus.tier13_2pc_melee&buff.inner_rage.up))&target.health_pct<20))";
        action_list_str += "/execute,if=buff.executioner_talent.remains<1.5";
        if ( level >= 81 ) action_list_str += "/colossus_smash";
        action_list_str += "/execute,if=buff.executioner_talent.stack<5";
        action_list_str += "/bloodthirst";
        if ( talents.raging_blow -> ok() && talents.titans_grip -> ok() )
        {
          action_list_str += "/berserker_rage,if=!(buff.death_wish.up|buff.enrage.up|buff.unholy_frenzy.up)&rage>15&cooldown.raging_blow.remains<1,use_off_gcd=1";
          action_list_str += "/raging_blow";
        }
        action_list_str += "/slam,if=buff.bloodsurge.react";
        action_list_str += "/execute,if=rage>=50";
        if ( talents.raging_blow -> ok() && ! talents.titans_grip -> ok() )
        {
          action_list_str += "/berserker_rage,if=!(buff.death_wish.up|buff.enrage.up|buff.unholy_frenzy.up)&rage>15&cooldown.raging_blow.remains<1,use_off_gcd=1";
          action_list_str += "/raging_blow";
        }
        action_list_str += "/battle_shout,if=rage<70|!aura.attack_power_multiplier.up";
        if ( ! talents.raging_blow -> ok() && glyphs.berserker_rage -> ok() ) action_list_str += "/berserker_rage";
      }
      else
      {
        action_list_str += "/berserker_rage,use_off_gcd=1,if=rage<95";
        if ( talents.death_wish -> ok() )
          action_list_str += "/death_wish,use_off_gcd=1,if=target.time_to_die>174|target.health_pct<20|target.time_to_die<31";
        action_list_str += "/recklessness,use_off_gcd=1,if=buff.death_wish.up|target.time_to_die<13";
        action_list_str += "/cleave,if=target.adds>0,use_off_gcd=1";
        action_list_str += "/whirlwind,if=target.adds>0";
        action_list_str += "/heroic_strike,use_off_gcd=1,if=set_bonus.tier13_2pc_melee&buff.inner_rage.up&rage>=60&target.health_pct>=20";
        action_list_str += "/heroic_strike,use_off_gcd=1,if=buff.battle_trance.up";
        action_list_str += "/heroic_strike,use_off_gcd=1,if=buff.colossus_smash.up&rage>50";
        if ( talents.executioner -> ok() )
        {
          action_list_str += "/execute,if=buff.executioner_talent.remains<1.5";
          action_list_str += "/execute,if=buff.executioner_talent.stack<5&rage>=30&cooldown.bloodthirst.remains>0.2";
        }
        action_list_str += "/bloodthirst";
        action_list_str += "/colossus_smash,if=buff.colossus_smash.down";
        action_list_str += "/inner_rage,use_off_gcd=1,if=buff.colossus_smash.up|cooldown.colossus_smash.remains<9";
        if ( talents.bloodsurge -> ok() )
          action_list_str += "/slam,if=buff.bloodsurge.react";
        action_list_str += "/execute,if=rage>=50&cooldown.bloodthirst.remains>0.2";
        action_list_str += "/heroic_strike,use_off_gcd=1,if=(buff.incite.up|buff.colossus_smash.up)&((rage>=50|(rage>=40&set_bonus.tier13_2pc_melee&buff.inner_rage.up))&target.health_pct>=20)";
        action_list_str += "/heroic_strike,use_off_gcd=1,if=(buff.incite.up|buff.colossus_smash.up)&((rage>=75|(rage>=65&set_bonus.tier13_2pc_melee&buff.inner_rage.up))&target.health_pct<20)";
        if ( talents.raging_blow -> ok() )
          action_list_str += "/raging_blow,if=rage>60&cooldown.inner_rage.remains>2&buff.inner_rage.down&cooldown.bloodthirst.remains>0.2";
        action_list_str += "/heroic_strike,use_off_gcd=1,if=rage>=85";
        if ( talents.raging_blow -> ok() )
        {
          action_list_str += "/raging_blow,if=rage>90&buff.inner_rage.up&cooldown.bloodthirst.remains>0.2";
          action_list_str += "/raging_blow,if=buff.colossus_smash.up&rage>50";
        }
        action_list_str += "/battle_shout,if=(rage<70&cooldown.bloodthirst.remains>0.2)|!aura.attack_power_multiplier.up";
      }
    }

    // Protection
    else if ( specialization() == WARRIOR_PROTECTION )
    {
      action_list_str += "/stance,choose=defensive";
      if ( talents.last_stand -> ok() ) action_list_str += "/last_stand,if=health<30000";
      action_list_str += "/heroic_strike,if=rage>=50";
      action_list_str += "/inner_rage,if=rage>=85,use_off_gcd=1";
      action_list_str += "/berserker_rage,use_off_gcd=1";
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
  resources.current[ RESOURCE_RAGE ] = std::min( initial_rage, 100 );

  if ( active_stance == STANCE_BATTLE && ! buffs_battle_stance -> check() )
    buffs_battle_stance -> trigger();
}

// warrior_t::register_callbacks ==============================================

void warrior_t::register_callbacks()
{
  player_t::register_callbacks();

  callbacks.register_direct_damage_callback( SCHOOL_ATTACK_MASK, new bloodthirst_buff_callback_t( this, buffs_bloodthirst ) );
}

// warrior_t::reset =========================================================

void warrior_t::reset()
{
  player_t::reset();

  active_stance = STANCE_BATTLE;
}

// warrior_t::composite_attack_hit ==========================================

double warrior_t::composite_attack_hit()
{
  double ah = player_t::composite_attack_hit();

  ah += spec.precision -> effectN( 1 ).percent();

  return ah;
}

// warrior_t::composite_attack_crit =========================================

double warrior_t::composite_attack_crit( weapon_t* w )
{
  double c = player_t::composite_attack_crit( w );

  c += talents.rampage -> effectN( 2 ).percent();

  return c;
}

// warrior_t::composite_mastery =============================================

double warrior_t::composite_mastery()
{
  double m = player_t::composite_mastery();

  m += spec.precision -> effectN( 2 ).base_value();

  return m;
}

// warrior_t::composite_attack_haste ========================================

double warrior_t::composite_attack_haste()
{
  double h = player_t::composite_attack_haste();

  // Unholy Frenzy is effected by Unshackled Fury
  if ( mastery.unshackled_fury -> ok() && buffs.unholy_frenzy -> up() )
  {
    // Assume it's multiplicative.
    h /= 1.0 / ( 1.0 + 0.2 );

    h *= 1.0 / ( 1.0 + ( 0.2 * ( 1.0 + composite_mastery() * mastery.unshackled_fury -> effectN( 3 ).percent() / 100.0 ) ) );
  }

  return h;
}

// warrior_t::composite_player_multiplier ===================================

double warrior_t::composite_player_multiplier( school_e school, action_t* a )
{
  double m = player_t::composite_player_multiplier( school, a );

  // Stances affect all damage done
  if ( active_stance == STANCE_BATTLE && buffs_battle_stance -> up() )
  {
    m *= 1.0 + buffs_battle_stance -> data().effectN( 1 ).percent();
  }
  else if ( active_stance == STANCE_BERSERKER && buffs_berserker_stance -> up() )
  {
    m *= 1.0 + buffs_berserker_stance -> data().effectN( 1 ).percent();
  }

  return m;
}

// warrior_t::matching_gear_multiplier ======================================

double warrior_t::matching_gear_multiplier( attribute_e attr )
{
  if ( ( attr == ATTR_STRENGTH ) && ( specialization() == WARRIOR_ARMS || specialization() == WARRIOR_FURY ) )
    return 0.05;

  if ( ( attr == ATTR_STAMINA ) && ( specialization() == WARRIOR_PROTECTION ) )
    return 0.05;

  return 0.0;
}

// warrior_t::composite_tank_block ==========================================

double warrior_t::composite_tank_block()
{
  double b = player_t::composite_tank_block();

  b += composite_mastery() * mastery.critical_block -> effectN( 3 ).percent() / 100.0;

  if ( buffs_shield_block -> up() )
    b = 1.0;

  return b;
}

// warrior_t::composite_tank_crit_block =====================================

double warrior_t::composite_tank_crit_block()
{
  double b = player_t::composite_tank_crit_block();

  b += composite_mastery() * mastery.critical_block->effect1().coeff() / 100.0;

  if ( buffs_hold_the_line -> up() )
    b += 0.10;

  return b;
}

// warrior_t::composite_tank_crit ===========================================

double warrior_t::composite_tank_crit( const school_e school )
{
  double c = player_t::composite_tank_crit( school );

  // FIXME:
  // if ( school == SCHOOL_PHYSICAL && talents.bastion_of_defense -> rank() )
  //  c += talents.bastion_of_defense -> effectN( 1 ).percent();

  return c;
}

// warrior_t::regen =========================================================

void warrior_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

  if ( spec.anger_management -> ok() )
    resource_gain( RESOURCE_RAGE, ( periodicity.total_seconds() / 3.0 ), gains_anger_management );

  uptimes_rage_cap -> update( resources.current[ RESOURCE_RAGE ] ==
                              resources.max    [ RESOURCE_RAGE] );
}

// warrior_t::primary_role() ================================================

role_e warrior_t::primary_role()
{
  if ( player_t::primary_role() == ROLE_TANK )
    return ROLE_TANK;

  if ( player_t::primary_role() == ROLE_DPS || player_t::primary_role() == ROLE_ATTACK )
    return ROLE_ATTACK;

  if ( specialization() == WARRIOR_PROTECTION )
    return ROLE_TANK;

  return ROLE_ATTACK;
}

// warrior_t::assess_damage =================================================

double warrior_t::assess_damage( double        amount,
                                 school_e school,
                                 dmg_e    dtype,
                                 result_e result,
                                 action_t*     action )
{
  if ( result == RESULT_HIT    ||
       result == RESULT_CRIT   ||
       result == RESULT_GLANCE ||
       result == RESULT_BLOCK  )
  {
    double rage_gain = amount * 18.92 / resources.max[ RESOURCE_HEALTH ];
    if ( buffs_berserker_rage -> up() )
      rage_gain *= 2.0;

    resource_gain( RESOURCE_RAGE, rage_gain, gains_incoming_damage );
  }


  if ( result == RESULT_BLOCK )
  {
    if ( talents.shield_specialization -> ok() )
    {
      // FIXME:
      // resource_gain( RESOURCE_RAGE, 5.0 * talents.shield_specialization -> rank(), gains_shield_specialization );
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
      timespan_t swing_time = main_hand_attack -> time_to_execute;
      timespan_t max_reschedule = ( main_hand_attack -> execute_event -> occurs() - 0.20 * swing_time ) - sim -> current_time;

      if ( max_reschedule > timespan_t::zero() )
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

  return player_t::assess_damage( amount, school, dtype, result, action );
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

  warrior_t* p = dynamic_cast<warrior_t*>( source );
  assert( p );

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

#endif // SC_WARRIOR

// WARRIOR MODULE INTERFACE ================================================

struct warrior_module_t : public module_t
{
  warrior_module_t() : module_t( WARRIOR ) {}

  virtual player_t* create_player( sim_t* /*sim*/, const std::string& /*name*/, race_e /*r = RACE_NONE*/ )
  {
    return NULL; // new warrior_t( sim, name, r );
  }
  virtual bool valid() { return false; }
  virtual void init( sim_t* sim )
  {
    for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
    {
      player_t* p = sim -> actor_list[ i ];
      p -> debuffs.shattering_throw      = buff_creator_t( p, "shattering_throw", p -> find_spell( 64382 ) );
    }
  }
  virtual void combat_begin( sim_t* ) {}
  virtual void combat_end  ( sim_t* ) {}
};

} // ANONYMOUS NAMESPACE

module_t* module_t::warrior()
{
  static module_t* m = 0;
  if ( ! m ) m = new warrior_module_t();
  return m;
}
