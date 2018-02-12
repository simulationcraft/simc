// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
/*
  TODO (Holy):
    - everything, pretty much :(

 TODO (Shockadin - e.h. Holy w/ role=attack):
    - Default APL
    + Mana
    + Holy Shock
    + Shock cd
    + Shock artifact trait
    + HA (haste mod)
    + CM
    + DP - need to review retribution model and update for Holy
    + The Topless Tower (leg)
    + Soul of Highlord (leg)
    + Belt (leg)
    + Sephus
    + T20_2 bonus
    + Light of Dawn (needed as filler)
    + Judgement
    + Damage for spells (fixed composite_melee_attack_power for Holy Spec)
        + Crusader strike
        + Judgement
        + Consecration

  TODO (ret):
    - Eye for an Eye
    - verify Divine Purpose interactions with The Fires of Justice and Seal of Light
    - bugfixes & cleanup
    - BoK/BoW
    - Check mana/mana regen for ret, sword of light has been significantly changed to no longer have the mana regen stuff, or the bonus to healing, reduction in mana costs, etc.
  TODO (prot):
    - Uther's Guard (leg)
    - Tyr's Hand of Faith (leg)
    - Ilterendi, Crown Jewel of Silvermoon (leg - Holy?)
    - Aegis of Light: Convert from self-buff to totem/pet with area buff? (low priority)

    - Everything below this line is super-low priority and can probably be ignored ======
    - Final Stand?? (like Hand of Reckoning, but AOE)
    - Blessing of Spellweaving??
*/
#include "simulationcraft.hpp"
#include "sc_paladin.hpp"

// ==========================================================================
// Paladin
// ==========================================================================
namespace paladin {

paladin_t::paladin_t( sim_t* sim, const std::string& name, race_e r ) :
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
  fake_sov( true )
{
  whisper_of_the_nathrezim            = nullptr;
  liadrins_fury_unleashed             = nullptr;
  chain_of_thrayn                     = nullptr;
  ashes_to_dust                       = nullptr;
  scarlet_inquisitors_expurgation     = nullptr;
  justice_gaze                        = nullptr;
  ferren_marcuss_strength             = nullptr;
  pillars_of_inmost_light             = nullptr;
  saruans_resolve                     = nullptr;
  gift_of_the_golden_valkyr           = nullptr;
  heathcliffs_immortality             = nullptr;
  sephuz                              = nullptr;
  topless_tower                       = nullptr;
  active_beacon_of_light              = nullptr;
  active_enlightened_judgments        = nullptr;
  active_shield_of_vengeance_proc     = nullptr;
  active_holy_shield_proc             = nullptr;
  active_tyrs_enforcer_proc           = nullptr;
  active_painful_truths_proc          = nullptr;
  active_judgment_of_light_proc       = nullptr;
  active_sotr                         = nullptr;
  active_protector_of_the_innocent    = nullptr;

  cooldowns.avengers_shield           = get_cooldown( "avengers_shield" );
  cooldowns.judgment                  = get_cooldown("judgment");
  cooldowns.shield_of_the_righteous   = get_cooldown( "shield_of_the_righteous" );
  cooldowns.guardian_of_ancient_kings = get_cooldown("guardian_of_ancient_kings");
  cooldowns.avenging_wrath            = get_cooldown( "avenging_wrath" );
  cooldowns.light_of_the_protector    = get_cooldown( "light_of_the_protector" );
  cooldowns.hand_of_the_protector     = get_cooldown( "hand_of_the_protector" );
  cooldowns.hammer_of_justice         = get_cooldown( "hammer_of_justice" );
  cooldowns.blade_of_justice          = get_cooldown( "blade_of_justice" );
  cooldowns.blade_of_wrath            = get_cooldown( "blade_of_wrath" );
  cooldowns.divine_hammer             = get_cooldown( "divine_hammer" );
  cooldowns.eye_of_tyr                = get_cooldown( "eye_of_tyr");
  cooldowns.holy_shock                = get_cooldown( "holy_shock");
  cooldowns.light_of_dawn             = get_cooldown( "light_of_dawn");

  talent_points.register_validity_fn([this](const spell_data_t* spell)
  {
    // Soul of the Highlord
    if (find_item(151644))
    {
      switch (specialization())
      {
      case PALADIN_RETRIBUTION:
        return spell->id() == 223817; // Divine Purpose
      case PALADIN_HOLY:
        return spell->id() == 197646; // Divine Purpose
      case PALADIN_PROTECTION:
        return spell->id() == 152261; // Holy Shield
      default:
        return false;
      }
    }
    return false;
  } );

  beacon_target = nullptr;
  regen_type = REGEN_DYNAMIC;
}

paladin_td_t* paladin_t::get_target_data( player_t* target ) const
{
  paladin_td_t*& td = target_data[ target ];
  if ( ! td )
  {
    td = new paladin_td_t( target, const_cast<paladin_t*>(this) );
  }
  return td;
}

// ==========================================================================
// Paladin Buffs, Part One
// ==========================================================================
// Paladin buffs are split up into two sections. This one is for ones that
// need to be defined before action_t definitions, because those action_t
// definitions call methods of these buffs. Generic buffs that can be defined
// anywhere are also put here. There's a second buffs section near the end
// containing ones that require action_t definitions to function properly.
namespace buffs {
  liadrins_fury_unleashed_t::liadrins_fury_unleashed_t( player_t* p ) :
      buff_t( buff_creator_t( p, "liadrins_fury_unleashed", p -> find_spell( 208410 ) )
        .tick_zero( true )
        .tick_callback( [ this, p ]( buff_t*, int, const timespan_t& ) {
          paladin_t* paladin = debug_cast<paladin_t*>( p );
          paladin -> resource_gain( RESOURCE_HOLY_POWER, data().effectN( 1 ).base_value(), paladin -> gains.hp_liadrins_fury_unleashed );
        } ) )
  {
    // nothing to do here
  }

  avenging_wrath_buff_t::avenging_wrath_buff_t( player_t* p ) :
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

  void avenging_wrath_buff_t::expire_override( int expiration_stacks, timespan_t remaining_duration )
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    paladin_t* p = static_cast<paladin_t*>( player );
    p -> buffs.liadrins_fury_unleashed -> expire(); // Force Liadrin's Fury to fade
  }

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
}

 // end namespace buffs
// ==========================================================================
// End Paladin Buffs, Part One
// ==========================================================================

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

  void init() override
  {
    paladin_spell_t::init();

    if (p()->ferren_marcuss_strength){
      aoe += (p()->spells.ferren_marcuss_strength->effectN(1).base_value());
      base_multiplier *= 1.0 + p()->spells.ferren_marcuss_strength->effectN(2).percent();
    }

  }

  void impact( action_state_t* s ) override
  {
    paladin_spell_t::impact( s );

    // Bulwark of Order absorb shield. Amount is additive per hit.
    if ( p() -> artifact.bulwark_of_order.rank() )
      p() -> buffs.bulwark_of_order -> trigger( 1, p() -> buffs.bulwark_of_order -> value() + s -> result_amount * p() -> artifact.bulwark_of_order.percent() );

  if (p()->gift_of_the_golden_valkyr){
    timespan_t reduction = timespan_t::from_seconds(-1.0 * p()->spells.gift_of_the_golden_valkyr->effectN(1).base_value());
    p()->cooldowns.guardian_of_ancient_kings ->adjust(reduction);
  }

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
    p() -> trigger_forbearance( execute_state -> target );
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
    p() -> trigger_forbearance( execute_state -> target );
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
      cooldown -> charges += p -> sets -> set( PALADIN_RETRIBUTION, T18, B2 ) -> effectN( 1 ).base_value();
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

// Holy Avenger
    struct holy_avenger_t : public paladin_heal_t
    {
        holy_avenger_t( paladin_t* p, const std::string& options_str )
        : paladin_heal_t( "holy_avenger", p, p -> talents.holy_avenger )
        {
            parse_options( options_str );

            if ( ! ( p -> talents.holy_avenger -> ok() ) )
                background = true;

        }

        void execute() override
        {
            paladin_heal_t::execute();

            p() -> buffs.holy_avenger -> trigger();
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

  double action_multiplier() const override
  {
    double am = paladin_spell_t::action_multiplier();

    am *= 1.0 + p()->artifact.hammer_time.percent(1);

    return am;
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
      base_multiplier *= 1.0 + p -> passives.retribution_paladin -> effectN( 9 ).percent();
    }

    if (p->specialization() == PALADIN_PROTECTION)
    {
      base_multiplier *= 1.0 + p->passives.protection_paladin->effectN(4).percent();
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

    paladin_ground_aoe_t* alt_aoe = nullptr;
    // create a new ground aoe event
    if ( p() -> specialization() == PALADIN_RETRIBUTION ) {
      alt_aoe =
        make_event<paladin_ground_aoe_t>( *sim, p(), ground_aoe_params_t()
        .target( execute_state -> target )
        // spawn at feet of player
        .x( execute_state -> action -> player -> x_position )
        .y( execute_state -> action -> player -> y_position )
        .n_pulses( 13 )
        .start_time( sim -> current_time()  )
        .action( damage_tick )
        .hasted( ground_aoe_params_t::SPELL_HASTE ), true );
    } else {
      alt_aoe =
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
    }

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

    p() -> trigger_forbearance( player, false );
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

  cooldown = p->cooldowns.guardian_of_ancient_kings;
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
    crit_bonus_multiplier *= 1.0 + (2 * p -> artifact.shock_treatment.percent());

  }

  virtual double composite_crit_chance() const override
  {
    double cc = paladin_spell_t::composite_crit_chance();

    // effect 1 doubles crit chance
    cc *= crit_chance_multiplier;

    return cc;
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


//    virtual void execute() override
//    {
//        paladin_heal_t::execute();
//
//        if ( execute_state -> result == RESULT_CRIT )
//            p() -> buffs.infusion_of_light -> trigger();
//
//    }
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
    crit_bonus_multiplier *= 1.0 + (2 * p -> artifact.shock_treatment.percent());
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

struct holy_shock_t : public paladin_spell_t
{
  holy_shock_damage_t* damage;
  holy_shock_heal_t* heal;
  timespan_t cd_duration;
  double cooldown_mult;
  bool dmg;

  holy_shock_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "holy_shock", p, p -> find_specialization_spell( 20473 ) ),
    cooldown_mult( 1.0 ), dmg( false )
  {
    add_option( opt_bool( "damage", dmg ) );
    check_spec( PALADIN_HOLY );
    parse_options( options_str );

    cooldown = p -> cooldowns.holy_shock;
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
//      cooldown -> duration = timespan_t::from_seconds( 9.0 );

    paladin_spell_t::execute();

      if ( p() -> buffs.divine_purpose -> check() )
      {
          p() -> buffs.divine_purpose -> expire();
      }

      if ( p() -> talents.divine_purpose -> ok() )
      {
          bool success = p() -> buffs.divine_purpose -> trigger( 1,
                                                                p() -> buffs.divine_purpose -> default_value,
                                                                p() -> spells.divine_purpose_holy -> proc_chance() );
          if ( success ) {
              p() -> procs.divine_purpose -> occur();
              p() -> cooldowns.holy_shock -> reset (true);
          }
      }

  }

  double cooldown_multiplier() override
  {
    double cdm = paladin_spell_t::cooldown_multiplier();

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
    p() -> trigger_forbearance( execute_state -> target );
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

  void init() override
  {
    paladin_heal_t::init();

    if (p()->saruans_resolve){
      cooldown->charges = 2;
    }

  }

  double recharge_multiplier() const override{
    double cdr = paladin_heal_t::recharge_multiplier();
    if (p()->saruans_resolve){
      cdr *= (1 + p()->spells.saruans_resolve->effectN(3).percent());
    }
    return cdr;
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
    internal_cooldown -> duration = timespan_t::from_seconds( .75 );

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

  void init() override
  {
    paladin_heal_t::init();

    if (p()->saruans_resolve){
      cooldown->charges = 2;
    }

  }

  double recharge_multiplier() const override{
    double cdr = paladin_heal_t::recharge_multiplier();
    if (p()->saruans_resolve){
      cdr *= (1 + p()->spells.saruans_resolve->effectN(3).percent());
    }
    return cdr;
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
    : paladin_spell_t( "eye_of_tyr", p, p -> find_spell( 209202 ) )
  {
    parse_options( options_str );

    aoe = -1;
    may_crit = true;

  }

  void init() override
  {
    paladin_spell_t::init();
    if ( p() -> pillars_of_inmost_light ) {
      base_multiplier *= 1.0 + p() -> spells.pillars_of_inmost_light -> effectN( 1 ).percent();
    }
  }

  bool ready() override
  {
    if (!player->artifact->enabled())
    {
      return false;
    }

    if (p()->artifact.eye_of_tyr.rank() == 0)
    {
      return false;
    }

    return paladin_spell_t::ready();
  }

  virtual void execute() override
  {
    paladin_spell_t::execute();

    if ( p() -> pillars_of_inmost_light )
    {
      p() -> cooldowns.eye_of_tyr -> ready += (p() -> cooldowns.eye_of_tyr -> duration * (p() -> spells.pillars_of_inmost_light -> effectN( 2 ).percent()));
    }
  }


  virtual void impact( action_state_t* s ) override
  {
    paladin_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
      td( s -> target ) -> buffs.eye_of_tyr_debuff -> trigger();
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

      cooldown = p -> cooldowns.light_of_dawn;
  }

    virtual void execute() override
    {
        if (p()->topless_tower) {
            if (p()->shuffled_rngs.topless_tower->trigger())
            {
                p()->procs.topless_tower->occur();

                timespan_t proc_duration = timespan_t::from_seconds(p()->topless_tower->effectN(2).base_value());
                if (p()->buffs.avenging_wrath->check())
                    p()->buffs.avenging_wrath->extend_duration(p(), proc_duration);
                else
                    p()->buffs.avenging_wrath->trigger(1, p()->buffs.avenging_wrath->default_value, -1.0, proc_duration);
            }
        }

        if ( p() -> sets -> has_set_bonus( PALADIN_HOLY, T20, B2 ) )
        {
            p()->cooldowns.light_of_dawn->adjust( timespan_t::from_seconds(-2.0));
        }

    }
};

// ==========================================================================
// End Spells, Heals, and Absorbs
// ==========================================================================

// ==========================================================================
// Paladin Attacks
// ==========================================================================

// paladin-specific melee_attack_t class for inheritance

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

    if ( p() -> main_hand_attack -> target -> is_sleeping() && potential_target == nullptr )
    {
      return false;
    }

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

    if ( p -> specialization() == PALADIN_RETRIBUTION )
    {
      base_multiplier *= 1.0 + p -> passives.retribution_paladin -> effectN( 8 ).percent();
    }

      if (p->specialization() == PALADIN_HOLY) {
          base_multiplier *= 1.0 + p->passives.holy_paladin->effectN(5).percent();
      }

    background = ( p -> talents.zeal -> ok() );
  }

  void impact( action_state_t* s ) override
  {
    holy_power_generator_t::impact( s );

      if ( p() -> talents.crusaders_might -> ok() ) {
          p() -> cooldowns.holy_shock -> adjust( timespan_t::from_seconds( -1.5) );
          p() -> cooldowns.light_of_dawn -> adjust( timespan_t::from_seconds( -1.5));
      }

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

    double composite_target_multiplier( player_t* t ) const override
    {
        double m = paladin_melee_attack_t::composite_target_multiplier( t );

        if (p() -> specialization() == PALADIN_HOLY) {
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

  void init() override {
    paladin_spell_t::init();
    snapshot_flags = 0;
  }

  proc_types proc_type() const override
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

      if ( p -> sets -> has_set_bonus( PALADIN_RETRIBUTION, T21, B2 ) )
        base_multiplier *= 1.0 + p -> sets -> set( PALADIN_RETRIBUTION, T21, B2 ) -> effectN( 1 ).percent();

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
    am *= 1.0 + p() -> get_divine_judgment( true );
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
      if ( p -> sets -> has_set_bonus( PALADIN_RETRIBUTION, T21, B2 ) )
        base_multiplier *= 1.0 + p -> sets -> set( PALADIN_RETRIBUTION, T21, B2 ) -> effectN( 1 ).percent();
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
    if ( p() -> sets -> has_set_bonus( PALADIN_RETRIBUTION, T20, B2 ) )
      p() -> buffs.sacred_judgment -> trigger();
    if ( p() -> sets -> has_set_bonus( PALADIN_RETRIBUTION, T21, B4 ) )
      p() -> buffs.ret_t21_4p -> trigger();
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
        if ( p() -> sets -> has_set_bonus( PALADIN_PROTECTION, T20, B2 ) &&
          rng().roll( p() -> sets -> set( PALADIN_PROTECTION, T20, B2 ) -> proc_chance() ) )
        {
          p() -> cooldowns.avengers_shield -> reset( true );
        }

        p() -> cooldowns.shield_of_the_righteous -> adjust( s -> result == RESULT_CRIT ? 2.0 * sotr_cdr : sotr_cdr );
      }
    }

    paladin_melee_attack_t::impact( s );
  }

  double action_multiplier() const override
  {
    double am = paladin_melee_attack_t::action_multiplier();
    // todo: refer to actual spelldata instead of magic constant
    am *= 1.0 + p() -> get_divine_judgment( true );
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
    if ( ! target -> debuffs.casting || ! target -> debuffs.casting -> check() )
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
  buffs.forbearant_faithful = new buffs::forbearance_t( this, "forbearant_faithful" );
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
  if ( name == "holy_avenger"              ) return new holy_avenger_t             ( this, options_str );
  if ( name == "bastion_of_light"          ) return new bastion_of_light_t         ( this, options_str );
  if ( name == "blessed_hammer"            ) return new blessed_hammer_t           ( this, options_str );
  if ( name == "blessing_of_protection"    ) return new blessing_of_protection_t   ( this, options_str );
  if ( name == "blessing_of_spellwarding"  ) return new blessing_of_spellwarding_t ( this, options_str );
  if ( name == "blinding_light"            ) return new blinding_light_t           ( this, options_str );
  if ( name == "beacon_of_light"           ) return new beacon_of_light_t          ( this, options_str );
  if ( name == "consecration"              ) return new consecration_t             ( this, options_str );
  if ( name == "crusader_strike"           ) return new crusader_strike_t          ( this, options_str );
  if ( name == "denounce"                  ) return new denounce_t                 ( this, options_str );
  if ( name == "divine_protection"         ) return new divine_protection_t        ( this, options_str );
  if ( name == "divine_steed"              ) return new divine_steed_t             ( this, options_str );
  if ( name == "divine_shield"             ) return new divine_shield_t            ( this, options_str );
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
  if ( name == "holy_prism"                ) return new holy_prism_t               ( this, options_str );
  if ( name == "eye_of_tyr"                ) return new eye_of_tyr_t               ( this, options_str );
  if ( name == "seraphim"                  ) return new seraphim_t                 ( this, options_str );
  if ( name == "holy_light"                ) return new holy_light_t               ( this, options_str );
  if ( name == "flash_of_light"            ) return new flash_of_light_t           ( this, options_str );
  if ( name == "lay_on_hands"              ) return new lay_on_hands_t             ( this, options_str );

  action_t* ret_action = create_action_retribution( name, options_str );
  if ( ret_action )
    return ret_action;

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

void paladin_t::trigger_forbearance( player_t* target, bool update_recharge_multipliers )
{
  auto buff = debug_cast<buffs::forbearance_t*>( target -> debuffs.forbearance );

  buff -> paladin = this;
  buff -> trigger();

  get_target_data( target ) -> buffs.forbearant_faithful -> trigger();

  if ( update_recharge_multipliers )
  {
    update_forbearance_recharge_multipliers();
  }
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
    base.mana_regen_per_second = resources.base[ RESOURCE_MANA ] * 0.015;

  // Holy Insight increases max mana for Holy
//  resources.base_multiplier[ RESOURCE_MANA ] = 1.0 + passives.holy_insight -> effectN( 1 ).percent();
}

// paladin_t::reset =========================================================

void paladin_t::reset()
{
  player_t::reset();

  active_consecrations.clear();
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
    procs.topless_tower = get_proc( "topless_tower");
}

// paladin_t::init_scaling ==================================================

void paladin_t::init_scaling()
{
  player_t::init_scaling();

  specialization_e tree = specialization();

  // Only Holy cares about INT/SPI/SP.
  if ( tree == PALADIN_HOLY )
  {
    scaling -> enable( STAT_INTELLECT );
    scaling -> enable( STAT_SPELL_POWER );
  }

  if ( tree == PALADIN_PROTECTION )
  {
    scaling -> enable( STAT_BONUS_ARMOR );
  }

  scaling -> disable( STAT_AGILITY );
}

// paladin_t::init_buffs ====================================================

void paladin_t::create_buffs()
{
  player_t::create_buffs();

  create_buffs_retribution();

  buffs.divine_steed                   = buff_creator_t( this, "divine_steed", find_spell( "Divine Steed" ) )
                                          .duration( timespan_t::from_seconds( 3.0 ) ).chance( 1.0 ).default_value( 1.0 ); // TODO: change this to spellid 221883 & see if that automatically captures details

  // Talents
  buffs.holy_shield_absorb     = absorb_buff_creator_t( this, "holy_shield", find_spell( 157122 ) )
                                 .school( SCHOOL_MAGIC )
                                 .source( get_stats( "holy_shield_absorb" ) )
                                 .gain( get_gain( "holy_shield_absorb" ) );

  // General
  buffs.avenging_wrath         = new buffs::avenging_wrath_buff_t( this );
  buffs.holy_avenger           = new buffs::holy_avenger_buff_t( this);
  buffs.sephuz                 = new buffs::sephuzs_secret_buff_t( this );
  buffs.divine_protection      = new buffs::divine_protection_t( this );
  buffs.divine_shield          = buff_creator_t( this, "divine_shield", find_class_spell( "Divine Shield" ) )
                                 .cd( timespan_t::zero() ) // Let the ability handle the CD
                                 .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.divine_purpose                 = buff_creator_t( this, "divine_purpose", specialization() == PALADIN_HOLY ? find_spell( 197646 ) : find_spell( 223819 ) );

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
      precombat->add_action("flask,type=flask_of_ten_thousand_scars,if=!talent.seraphim.enabled");
      precombat->add_action("flask,type=flask_of_the_countless_armies,if=(role.attack|talent.seraphim.enabled)");
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
      precombat->add_action("food,type=seedbattered_fish_plate,if=!talent.seraphim.enabled");
      precombat->add_action("food,type=lavish_suramar_feast,if=(role.attack|talent.seraphim.enabled)");
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
    if (true_level > 100) {
      precombat->add_action("potion,name=unbending_potion,if=!talent.seraphim.enabled");
      precombat->add_action("potion,name=old_war,if=(role.attack|talent.seraphim.enabled)&active_enemies<3");
      precombat->add_action("potion,name=prolonged_power,if=(role.attack|talent.seraphim.enabled)&active_enemies>=3");
    }
    else if ( true_level > 90 ){
      potion_type = "draenic_strength";
      precombat->add_action("potion,name=" + potion_type);
    }
    else if (true_level >= 80) {
      potion_type = "mogu_power";
      precombat->add_action("potion,name=" + potion_type);
    }
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
  //prot->add_talent(this, "Seraphim", "if=talent.seraphim.enabled&action.shield_of_the_righteous.charges>=2");
  prot->add_action(this, "Shield of the Righteous", "if=!talent.seraphim.enabled&(action.shield_of_the_righteous.charges>2)&!(debuff.eye_of_tyr.up&buff.aegis_of_light.up&buff.ardent_defender.up&buff.guardian_of_ancient_kings.up&buff.divine_shield.up&buff.potion.up)");
  //prot->add_action(this, "Shield of the Righteous", "if=(talent.bastion_of_light.enabled&talent.seraphim.enabled&buff.seraphim.up&cooldown.bastion_of_light.up)&!(debuff.eye_of_tyr.up&buff.aegis_of_light.up&buff.ardent_defender.up&buff.guardian_of_ancient_kings.up&buff.divine_shield.up&buff.potion.up)");
  //prot->add_action(this, "Shield of the Righteous", "if=(talent.bastion_of_light.enabled&!talent.seraphim.enabled&cooldown.bastion_of_light.up)&!(debuff.eye_of_tyr.up&buff.aegis_of_light.up&buff.ardent_defender.up&buff.guardian_of_ancient_kings.up&buff.divine_shield.up&buff.potion.up)");
  prot->add_talent(this, "Bastion of Light", "if=!talent.seraphim.enabled&talent.bastion_of_light.enabled&action.shield_of_the_righteous.charges<1");
  prot->add_action(this, "Light of the Protector", "if=(health.pct<40)");
  prot->add_talent(this, "Hand of the Protector",  "if=(health.pct<40)");
  prot->add_action(this, "Light of the Protector", "if=("+threshold_lotp_rp+")&health.pct<55&talent.righteous_protector.enabled");
  prot->add_action(this, "Light of the Protector", "if=("+threshold_lotp+")&health.pct<55");
  prot->add_talent(this, "Hand of the Protector",  "if=("+threshold_hotp_rp+")&health.pct<65&talent.righteous_protector.enabled");
  prot->add_talent(this, "Hand of the Protector",  "if=("+threshold_hotp+")&health.pct<55");
  prot->add_action(this, "Divine Steed", "if=!talent.seraphim.enabled&talent.knight_templar.enabled&" + threshold + "&!(debuff.eye_of_tyr.up|buff.aegis_of_light.up|buff.ardent_defender.up|buff.guardian_of_ancient_kings.up|buff.divine_shield.up|buff.potion.up)");
  prot->add_action(this, "Eye of Tyr", "if=!talent.seraphim.enabled&" + threshold + "&!(debuff.eye_of_tyr.up|buff.aegis_of_light.up|buff.ardent_defender.up|buff.guardian_of_ancient_kings.up|buff.divine_shield.up|buff.potion.up)");
  prot->add_talent(this, "Aegis of Light", "if=!talent.seraphim.enabled&" + threshold + "&!(debuff.eye_of_tyr.up|buff.aegis_of_light.up|buff.ardent_defender.up|buff.guardian_of_ancient_kings.up|buff.divine_shield.up|buff.potion.up)");
  prot->add_action(this, "Guardian of Ancient Kings", "if=!talent.seraphim.enabled&" + threshold + "&!(debuff.eye_of_tyr.up|buff.aegis_of_light.up|buff.ardent_defender.up|buff.guardian_of_ancient_kings.up|buff.divine_shield.up|buff.potion.up)");
  prot->add_action(this, "Divine Shield", "if=!talent.seraphim.enabled&talent.final_stand.enabled&" + threshold + "&!(debuff.eye_of_tyr.up|buff.aegis_of_light.up|buff.ardent_defender.up|buff.guardian_of_ancient_kings.up|buff.divine_shield.up|buff.potion.up)");
  prot->add_action(this, "Ardent Defender", "if=!talent.seraphim.enabled&" + threshold + "&!(debuff.eye_of_tyr.up|buff.aegis_of_light.up|buff.ardent_defender.up|buff.guardian_of_ancient_kings.up|buff.divine_shield.up|buff.potion.up)");
  prot->add_action(this, "Lay on Hands", "if=!talent.seraphim.enabled&health.pct<15");

  //potion
  if (sim->allow_potions)
  {
    if (level() > 100)
    {
      prot->add_action("potion,name=old_war,if=buff.avenging_wrath.up&talent.seraphim.enabled&active_enemies<3");
      prot->add_action("potion,name=prolonged_power,if=buff.avenging_wrath.up&talent.seraphim.enabled&active_enemies>=3");
      prot->add_action("potion,name=unbending_potion,if=!talent.seraphim.enabled");
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
  prot->add_action("stoneform,if=!talent.seraphim.enabled&" + threshold + "&!(debuff.eye_of_tyr.up|buff.aegis_of_light.up|buff.ardent_defender.up|buff.guardian_of_ancient_kings.up|buff.divine_shield.up|buff.potion.up)");

  //dps-single-target
  prot->add_action(this, "Avenging Wrath", "if=!talent.seraphim.enabled");
  //prot->add_action(this, "Avenging Wrath", "if=talent.seraphim.enabled&buff.seraphim.up");
  //prot->add_action( "call_action_list,name=prot_aoe,if=spell_targets.avenger_shield>3" );
  prot->add_action(this, "Judgment", "if=!talent.seraphim.enabled");
  prot->add_action(this, "Avenger's Shield","if=!talent.seraphim.enabled&talent.crusaders_judgment.enabled&buff.grand_crusader.up");
  prot->add_talent(this, "Blessed Hammer", "if=!talent.seraphim.enabled");
  prot->add_action(this, "Avenger's Shield", "if=!talent.seraphim.enabled");
  prot->add_action(this, "Consecration", "if=!talent.seraphim.enabled");
  prot->add_action(this, "Hammer of the Righteous", "if=!talent.seraphim.enabled");

  //max dps build

  prot->add_talent(this,"Seraphim","if=talent.seraphim.enabled&action.shield_of_the_righteous.charges>=2");
  prot->add_action(this, "Avenging Wrath", "if=talent.seraphim.enabled&(buff.seraphim.up|cooldown.seraphim.remains<4)");
  prot->add_action(this, "Ardent Defender", "if=talent.seraphim.enabled&buff.seraphim.up");
  prot->add_action(this, "Shield of the Righteous", "if=talent.seraphim.enabled&(cooldown.consecration.remains>=0.1&(action.shield_of_the_righteous.charges>2.5&cooldown.seraphim.remains>3)|(buff.seraphim.up))");
  prot->add_action(this, "Eye of Tyr", "if=talent.seraphim.enabled&equipped.151812&buff.seraphim.up");
  prot->add_action(this, "Avenger's Shield", "if=talent.seraphim.enabled");
  prot->add_action(this, "Judgment", "if=talent.seraphim.enabled&(active_enemies<2|set_bonus.tier20_2pc)");
  prot->add_action(this, "Consecration", "if=talent.seraphim.enabled&(buff.seraphim.remains>6|buff.seraphim.down)");
  prot->add_action(this, "Judgment", "if=talent.seraphim.enabled");
  prot->add_action(this, "Consecration", "if=talent.seraphim.enabled");
  prot->add_action(this, "Eye Of Tyr", "if=talent.seraphim.enabled&!equipped.151812");
  prot->add_talent(this, "Blessed Hammer", "if=talent.seraphim.enabled");
  prot->add_action(this, "Hammer of the Righteous", "if=talent.seraphim.enabled");
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
      if (true_level > 100) {
          flask_action += "flask_of_the_whispered_pact";
      }
    else if ( true_level > 90 )
      flask_action += "greater_draenic_intellect_flask";
    else
      flask_action += ( true_level > 85 ) ? "warm_sun" : "draconic_mind";

    precombat -> add_action( flask_action );
  }

  if ( sim -> allow_food && level() >= 80 )
  {
    std::string food_action = "food,type=";
      if (true_level > 100) {
          food_action += "the_hungry_magister";
      }
      else if ( level() > 90 )
      food_action += "pickled_eel";
    else
      food_action += ( level() > 85 ) ? "mogu_fish_stew" : "seafood_magnifique_feast";
    precombat -> add_action( food_action );
  }

    if ( true_level > 100 )
        precombat -> add_action( "augmentation,type=defiled" );


  // Snapshot stats
  precombat -> add_action( "snapshot_stats",  "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat -> add_action( "potion,name=old_war" );

  // action priority list
    action_priority_list_t* def = get_action_priority_list( "default" );
    action_priority_list_t* cds = get_action_priority_list( "cooldowns" );
    action_priority_list_t* priority = get_action_priority_list( "priority" );

  def -> add_action( "auto_attack" );
    def -> add_action( "call_action_list,name=cooldowns");
    def -> add_action( "call_action_list,name=priority");

    cds -> add_action("avenging_wrath");
    if ( sim -> allow_potions )
    {
        cds -> add_action("potion,name=old_war,if=(buff.avenging_wrath.up)");
    }
    cds -> add_action("blood_fury,if=(buff.avenging_wrath.up)");
    cds -> add_action("berserking,if=(buff.avenging_wrath.up)");
    cds -> add_action("holy_avenger,if=(buff.avenging_wrath.up)");
    cds -> add_action("use_items,if=(buff.avenging_wrath.up)");

    priority -> add_action("judgment");
    priority -> add_action("holy_shock,damage=1");
    priority -> add_action("crusader_strike");
    priority -> add_action("holy_prism,target=self,if=active_enemies>=2");
    priority -> add_action("holy_prism");
    priority -> add_action("consecration");
    priority -> add_action("light_of_dawn");
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

// paladin_t::init_actions ==================================================

void paladin_t::init_action_list()
{
#ifdef NDEBUG // Only restrict on release builds.
  // Holy isn't fully supported atm
//  if ( specialization() == PALADIN_HOLY )
//  {
//    if ( ! quiet )
//      sim -> errorf( "Paladin holy healing for player %s is not currently supported.", name() );
//
//    quiet = true;
//    return;
//  }
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
  init_rng_retribution();
}

void paladin_t::init_assessors()
{
  player_t::init_assessors();
  init_assessors_retribution();
}

void paladin_t::init()
{
  player_t::init();

  if ( specialization() == PALADIN_HOLY && primary_role() != ROLE_ATTACK )
    sim -> errorf( "%s is using an unsupported spec.", name() );
}

void paladin_t::init_spells()
{
  player_t::init_spells();

  paladin_t::init_spells_retribution();

  // Talents
  talents.bestow_faith               = find_talent_spell( "Bestow Faith" );
  talents.lights_hammer              = find_talent_spell( "Light's Hammer" );
  talents.crusaders_might            = find_talent_spell( "Crusader's Might" );
  talents.cavalier                   = find_talent_spell( "Cavalier" );
  talents.unbreakable_spirit         = find_talent_spell( "Unbreakable Spirit" );
  talents.rule_of_law                = find_talent_spell( "Rule of Law" );
  talents.devotion_aura              = find_talent_spell( "Devotion Aura" );
  talents.aura_of_sacrifice          = find_talent_spell( "Aura of Sacrifice" );
  talents.aura_of_mercy              = find_talent_spell( "Aura of Mercy" );
    //    talents.divine_purpose           = find_talent_spell( "Sanctified Wrath" ); // TODO: Fix
  talents.holy_avenger               = find_talent_spell( "Holy Avenger" );
  talents.holy_prism                 = find_talent_spell( "Holy Prism" );
  talents.fervent_martyr             = find_talent_spell( "Fervent Martyr" );
  talents.sanctified_wrath           = find_talent_spell( "Sanctified Wrath" );
  talents.judgment_of_light          = find_talent_spell( "Judgment of Light" );
  talents.beacon_of_faith            = find_talent_spell( "Beacon of Faith" );
  talents.beacon_of_the_lightbringer = find_talent_spell( "Beacon of the Lightbringer" );
  talents.beacon_of_virtue           = find_talent_spell( "Beacon of Virtue" );

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
  artifact.holy_aegis              = find_artifact_spell("Holy Aegis");
  artifact.bulwark_of_the_silver_hand = find_artifact_spell("Bulwark of the Silver Hand");

  artifact.light_of_the_silver_hand = find_artifact_spell( "Light of the Silver Hand");
  artifact.virtues_of_the_light = find_artifact_spell("Virtues of the Light");
  artifact.shock_treatment = find_artifact_spell("Shock Treatment");

  // Spells
  spells.holy_light                    = find_specialization_spell( "Holy Light" );
  spells.divine_purpose_holy           = find_spell( 197646 );
  spells.pillars_of_inmost_light       = find_spell( 248102 );
  spells.ferren_marcuss_strength       = find_spell( 207614 );
  spells.saruans_resolve               = find_spell( 234653 );
  spells.gift_of_the_golden_valkyr     = find_spell( 207628 );
  spells.heathcliffs_immortality       = find_spell( 207599 );
  spells.consecration_bonus            = find_spell( 188370 );

  // Masteries
  passives.divine_bulwark         = find_mastery_spell( PALADIN_PROTECTION );
  passives.lightbringer           = find_mastery_spell( PALADIN_HOLY );

  // Specializations
  switch ( specialization() )
  {
    case PALADIN_HOLY:
      spec.judgment_2 = find_specialization_spell( 231644 );
      break;
    case PALADIN_PROTECTION:
      spec.judgment_2 = find_specialization_spell( 231657 );
      break;
    default:
      break;
  }

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
    return ROLE_ATTACK;

  return ROLE_HYBRID;
}

// paladin_t::primary_stat ==================================================

stat_e paladin_t::primary_stat() const
{
  switch ( specialization() )
  {
    case PALADIN_PROTECTION:  return STAT_STAMINA;
    case PALADIN_HOLY:        return STAT_INTELLECT;
    case PALADIN_RETRIBUTION: return STAT_STRENGTH;
    default:                  return STAT_STRENGTH;
  }
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
      // TODO(mserrano): fix this to grab from spelldata
      m *= 1.04;
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

  if (buffs.holy_avenger -> check())
    h *= buffs.holy_avenger -> value();

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

  if ( specialization() == PALADIN_HOLY )
    m += artifact.virtues_of_the_light.percent( 1 );

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

  // TODO: HA
  if (buffs.holy_avenger -> check())
    h *= buffs.holy_avenger -> value();

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
    double aw_multiplier = 1.0 + buffs.crusade -> get_damage_mod();
    if ( chain_of_thrayn )
    {
      aw_multiplier += spells.chain_of_thrayn -> effectN( 4 ).percent();
    }
    m *= aw_multiplier;
  }

  // Last Defender
  if ( talents.last_defender -> ok() )
  {
    // Last defender gives the same amount of damage increase as it gives mitigation.
    // Mitigation is 0.97^n, or (1-0.03)^n, where the 0.03 is in the spell data.
    // The damage buff is then 1+(1-0.97^n), or 2-(1-0.03)^n.
    m *= 2.0 - std::pow( 1.0 - talents.last_defender -> effectN( 2 ).percent(), buffs.last_defender -> current_stack );
  }

  // artifacts
  m *= 1.0 + artifact.ashbringers_light.percent();
  m *= 1.0 + artifact.ferocity_of_the_silver_hand.percent();
  m *= 1.0 + artifact.bulwark_of_the_silver_hand.percent();
  m *= 1.0 + artifact.light_of_the_silver_hand.percent();

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
    if ( specialization() == PALADIN_HOLY ) //thx for Mistweaver maintainer
        return composite_spell_power( SCHOOL_MAX );

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

double paladin_t::composite_parry() const
{
  double p_r = player_t::composite_parry();

  p_r += artifact.holy_aegis.percent(1);

  return p_r;
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
    s -> result_amount *= std::pow( 1.0 - talents.last_defender -> effectN( 2 ).percent(), buffs.last_defender -> current_stack );
  }

  // heathcliffs
  if (standing_in_consecration() && heathcliffs_immortality)
  {
    s->result_amount *= 1.0 - spells.heathcliffs_immortality->effectN(1).percent();
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
  add_option( opt_bool( "paladin_fake_sov", fake_sov ) );
  player_t::create_options();
}

// paladin_t::copy_from =====================================================

void paladin_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  paladin_t* p = debug_cast<paladin_t*>( source );

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

  if ( scarlet_inquisitors_expurgation )
  {
    buffs.scarlet_inquisitors_expurgation -> trigger( 30 );
    buffs.scarlet_inquisitors_expurgation_driver -> trigger();
  }
}


// Last defender ===========================================================

struct last_defender_callback_t
{
  paladin_t* p;
  double last_defender_distance;
  last_defender_callback_t( paladin_t* p ) : p( p ), last_defender_distance( 0 )
  {
    last_defender_distance = p -> talents.last_defender -> effectN( 1 ).base_value();
  }

  void operator()( player_t* )
  {
    int num_enemies = p -> get_local_enemies( last_defender_distance );

    p -> buffs.last_defender -> expire();
    p -> buffs.last_defender -> trigger( num_enemies );
  }
};

// paladin_t::create_actions ================================================

void paladin_t::activate()
{
  player_t::activate();

  if ( talents.last_defender -> ok() )
  {
    sim -> target_non_sleeping_list.register_callback( last_defender_callback_t( this ) );
  }
}


// paladin_t::get_divine_judgment =============================================

double paladin_t::get_divine_judgment(bool is_judgment) const
{
  if ( specialization() != PALADIN_RETRIBUTION ) return 0.0;

  if ( ! passives.divine_judgment -> ok() ) return 0.0;

  double handoflight;
  handoflight = cache.mastery_value(); // HoL modifier is in effect #1
  if ( is_judgment ) {
    handoflight *= passives.divine_judgment -> effectN( 3 ).sp_coeff();
    handoflight /= passives.divine_judgment -> effectN( 1 ).sp_coeff();
  }

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
  do_trinket_init( s, PALADIN_PROTECTION, s -> justice_gaze, effect);
}

static void chain_of_thrayn( special_effect_t& effect )
{
  paladin_t* s = debug_cast<paladin_t*>( effect.player );
  do_trinket_init( s, PALADIN_RETRIBUTION, s -> chain_of_thrayn, effect );
  do_trinket_init( s, PALADIN_PROTECTION, s -> chain_of_thrayn, effect);
}

static void ferren_marcuss_strength(special_effect_t& effect)
{
  paladin_t* s = debug_cast<paladin_t*>(effect.player);
  do_trinket_init(s, PALADIN_PROTECTION, s->ferren_marcuss_strength, effect);
}

static void pillars_of_inmost_light(special_effect_t& effect)
{
  paladin_t* s = debug_cast<paladin_t*>(effect.player);
  do_trinket_init(s, PALADIN_PROTECTION, s->pillars_of_inmost_light, effect);
}

static void saruans_resolve(special_effect_t& effect)
{
  paladin_t* s = debug_cast<paladin_t*>(effect.player);
  do_trinket_init(s, PALADIN_PROTECTION, s->saruans_resolve, effect);
}

static void gift_of_the_golden_valkyr(special_effect_t& effect)
{
  paladin_t* s = debug_cast<paladin_t*>(effect.player);
  do_trinket_init(s, PALADIN_PROTECTION, s->gift_of_the_golden_valkyr, effect);
}

static void heathcliffs_immortality(special_effect_t& effect)
{
  paladin_t* s = debug_cast<paladin_t*>(effect.player);
  do_trinket_init(s, PALADIN_PROTECTION, s->heathcliffs_immortality, effect);
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

struct topless_tower_t : public unique_gear::scoped_actor_callback_t<paladin_t>
{
    topless_tower_t() : super(PALADIN_HOLY)
    {
    }

    void manipulate(paladin_t* p, const special_effect_t& e) override
    {
        p->topless_tower = e.driver();

        const int total_entries = 20; // 6/23/2017 -- Reddit AMA Comment
        const int success_entries = (int)util::round(p->topless_tower->effectN(1).percent() * total_entries);
        p->shuffled_rngs.topless_tower = p->get_shuffled_rng("topless_tower", success_entries, total_entries);
    }
};

static void scarlet_inquisitors_expurgation( special_effect_t& effect )
{
  paladin_t* s = debug_cast<paladin_t*>( effect.player );
  do_trinket_init( s, PALADIN_RETRIBUTION, s -> scarlet_inquisitors_expurgation, effect );
}

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
    unique_gear::register_special_effect( 207633, whisper_of_the_nathrezim );
    unique_gear::register_special_effect( 208408, liadrins_fury_unleashed );
    unique_gear::register_special_effect( 206338, chain_of_thrayn );
    unique_gear::register_special_effect( 207614, ferren_marcuss_strength );
    unique_gear::register_special_effect( 234653, saruans_resolve );
    unique_gear::register_special_effect( 207628, gift_of_the_golden_valkyr );
    unique_gear::register_special_effect( 207599, heathcliffs_immortality );
    unique_gear::register_special_effect( 236106, ashes_to_dust );
    unique_gear::register_special_effect( 211557, justice_gaze );
    unique_gear::register_special_effect( 248102, pillars_of_inmost_light );
    unique_gear::register_special_effect( 208051, sephuzs_secret_enabler_t() );
    unique_gear::register_special_effect( 248103, scarlet_inquisitors_expurgation );
    unique_gear::register_special_effect( 248033, topless_tower_t() );
  }

  virtual void init( player_t* p ) const override
  {
    p -> buffs.beacon_of_light          = buff_creator_t( p, "beacon_of_light", p -> find_spell( 53563 ) );
    p -> buffs.blessing_of_sacrifice    = new buffs::blessing_of_sacrifice_t( p );
    p -> debuffs.forbearance            = new buffs::forbearance_t( p, "forbearance" );
  }

  virtual void register_hotfixes() const override
  {
  }

  virtual void combat_begin( sim_t* ) const override {}

  virtual void combat_end  ( sim_t* ) const override {}
};

} // end namespace paladin

const module_t* module_t::paladin()
{
  static paladin::paladin_module_t m;
  return &m;
}
