// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
/*
  TODO: BfA
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
}

 // end namespace buffs
// ==========================================================================
// End Paladin Buffs, Part One
// ==========================================================================

// Blessing of Protection =====================================================

struct blessing_of_protection_t : public paladin_spell_t
{
  blessing_of_protection_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "blessing_of_protection", p, p -> find_class_spell( "Blessing of Protection" ) )
  {
    parse_options( options_str );
  }

  virtual void execute() override
  {
    paladin_spell_t::execute();

    // apply forbearance, track locally for forbearant faithful & force recharge recalculation
    p() -> trigger_forbearance( execute_state -> target );
  }

  virtual bool ready() override
  {
    if ( target -> debuffs.forbearance -> check() )
      return false;

    return paladin_spell_t::ready();
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
    p() -> trigger_forbearance( player );
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

    harmful = false;
    may_miss = false;

    // Create redirect action conditionalized on the existence of HoS.
    if ( ! p -> active.blessing_of_sacrifice_redirect )
      p -> active.blessing_of_sacrifice_redirect = new blessing_of_sacrifice_redirect_t( p );
  }

  virtual void execute() override;

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

  virtual void execute() override
  {
    base_dd_min = base_dd_max = p() -> resources.max[ RESOURCE_HEALTH ];

    paladin_heal_t::execute();

    // apply forbearance, track locally for forbearant faithful & force recharge recalculation
    p() -> trigger_forbearance( execute_state -> target );
  }

  virtual bool ready() override
  {
    if ( target -> debuffs.forbearance -> check() )
      return false;

    return paladin_heal_t::ready();
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
    paladin_melee_attack_t( "melee", p, spell_data_t::nil() ),
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
    : paladin_melee_attack_t( "auto_attack", p, spell_data_t::nil() )
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
    : holy_power_generator_t( "crusader_strike", p, p -> find_class_spell( "Crusader Strike" ) )
  {
    parse_options( options_str );

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
    : paladin_melee_attack_t( "hammer_of_justice_damage", p, p -> find_spell( 211561 ) )
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

// Judgment =================================================================

struct judgment_aoe_t : public paladin_melee_attack_t
{
  judgment_aoe_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "judgment_aoe", p, p -> find_spell( 228288 ) )
  {
    parse_options( options_str );

    may_glance = may_block = may_parry = may_dodge = false;
    weapon_multiplier = 0;
    background = true;

    if ( p -> specialization() == PALADIN_RETRIBUTION )
    {
      aoe = 1 + p -> spec.judgment_2 -> effectN( 1 ).base_value();

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
    : paladin_melee_attack_t( "judgment", p, p -> find_spell( 20271 ) )
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
  buffs.blessed_hammer_debuff = buff_creator_t( *this, "blessed_hammer", paladin -> find_spell( 204301 ) );
}

// paladin_t::create_action =================================================

action_t* paladin_t::create_action( const std::string& name, const std::string& options_str )
{
  action_t* ret_action = create_action_retribution( name, options_str );
  if ( ret_action )
    return ret_action;

  action_t* prot_action = create_action_protection( name, options_str );
  if ( prot_action )
    return prot_action;

  action_t* holy_action = create_action_holy( name, options_str );
  if ( holy_action )
    return holy_action;

  if ( name == "auto_attack"               ) return new auto_melee_attack_t        ( this, options_str );
  if ( name == "avenging_wrath"            ) return new avenging_wrath_t           ( this, options_str );
  if ( name == "blessing_of_protection"    ) return new blessing_of_protection_t   ( this, options_str );
  if ( name == "blinding_light"            ) return new blinding_light_t           ( this, options_str );
  if ( name == "consecration"              ) return new consecration_t             ( this, options_str );
  if ( name == "crusader_strike"           ) return new crusader_strike_t          ( this, options_str );
  if ( name == "divine_steed"              ) return new divine_steed_t             ( this, options_str );
  if ( name == "divine_shield"             ) return new divine_shield_t            ( this, options_str );
  if ( name == "blessing_of_sacrifice"     ) return new blessing_of_sacrifice_t    ( this, options_str );
  if ( name == "hammer_of_justice"         ) return new hammer_of_justice_t        ( this, options_str );
  if ( name == "judgment"                  ) return new judgment_t                 ( this, options_str );
  if ( name == "rebuke"                    ) return new rebuke_t                   ( this, options_str );
  if ( name == "reckoning"                 ) return new reckoning_t                ( this, options_str );
  if ( name == "flash_of_light"            ) return new flash_of_light_t           ( this, options_str );
  if ( name == "lay_on_hands"              ) return new lay_on_hands_t             ( this, options_str );

  return player_t::create_action( name, options_str );
}

void paladin_t::trigger_forbearance( player_t* target )
{
  auto buff = debug_cast<buffs::forbearance_t*>( target -> debuffs.forbearance );

  buff -> paladin = this;
  buff -> trigger();
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
  gains.extra_regen                 = get_gain( "guarded_by_the_light" );
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
  create_buffs_protection();
  create_buffs_holy();

  buffs.divine_steed                   = buff_creator_t( this, "divine_steed", find_spell( "Divine Steed" ) )
                                          .duration( timespan_t::from_seconds( 3.0 ) ).chance( 1.0 ).default_value( 1.0 ); // TODO: change this to spellid 221883 & see if that automatically captures details

  // General
  buffs.avenging_wrath         = new buffs::avenging_wrath_buff_t( this );
  buffs.sephuz                 = new buffs::sephuzs_secret_buff_t( this );
  buffs.divine_shield          = buff_creator_t( this, "divine_shield", find_class_spell( "Divine Shield" ) )
                                 .cd( timespan_t::zero() ) // Let the ability handle the CD
                                 .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.divine_purpose                 = buff_creator_t( this, "divine_purpose", specialization() == PALADIN_HOLY ? find_spell( 197646 ) : find_spell( 223819 ) );
}

// paladin_t::default_potion ================================================

std::string paladin_t::default_potion() const
{
  std::string retribution_pot = (true_level > 100) ? "old_war" :
                                (true_level >= 90) ? "draenic_strength" :
                                (true_level >= 85) ? "mogu_power" :
                                (true_level >= 80) ? "golemblood" :
                                "disabled";

  bool dps = (primary_role() == ROLE_ATTACK) || (talents.seraphim -> ok());
  std::string protection_pot = (true_level > 100) ? ( dps ? "prolonged_power" : "unbending_potion" ) :
                               (true_level >= 90) ? "draenic_strength" :
                               (true_level >= 85) ? "mogu_power" :
                               (true_level >= 80) ? "mogu_power" :
                               "disabled";

  std::string holy_dps_pot = (true_level > 100) ? "old_war" :
                             "disabled";

  std::string holy_pot = "disabled";

  switch ( specialization() )
  {
    case PALADIN_RETRIBUTION:
      return retribution_pot;
    case PALADIN_PROTECTION:
      return protection_pot;
    case PALADIN_HOLY:
      return primary_role() == ROLE_ATTACK ? holy_dps_pot : holy_pot;
    default:
      return "disabled";
  }
}

std::string paladin_t::default_food() const
{
  std::string retribution_food = (true_level > 100) ? "azshari_salad" :
                                 (true_level >= 90) ? "sleeper_sushi" :
                                 (true_level >= 85) ? "black_pepper_ribs_and_shrimp" :
                                 (true_level >= 80) ? "beer_basted_crocolisk" :
                                 "disabled";

  bool dps = (primary_role() == ROLE_ATTACK) || (talents.seraphim -> ok());
  std::string protection_food = (true_level > 100) ? ( dps ? "lavish_suramar_feast" : "seedbattered_fish_plate" ) :
                                (true_level >= 90) ? ( dps ? "pickled_eel" : "whiptail_fillet" ) :
                                (true_level >= 85) ? "chun_tian_spring_rolls" :
                                (true_level >= 80) ? "seafood_magnifique_feast" :
                                "disabled";

  std::string holy_dps_food = (true_level > 100) ? "the_hungry_magister" :
                              (true_level >= 90) ? "pickled_eel" :
                              (true_level >= 85) ? "mogu_fish_stew" :
                              (true_level >= 80) ? "seafood_magnifique_feast" :
                              "disabled";

  std::string holy_food = (true_level > 100) ? "lavish_suramar_feast" :
                          (true_level >= 90) ? "pickled_eel" :
                          (true_level >= 85) ? "mogu_fish_stew" :
                          (true_level >= 80) ? "seafood_magnifique_feast" :
                          "disabled";

  switch ( specialization() )
  {
    case PALADIN_RETRIBUTION:
      return retribution_food;
    case PALADIN_PROTECTION:
      return protection_food;
    case PALADIN_HOLY:
      return primary_role() == ROLE_ATTACK ? holy_dps_food : holy_food;
    default:
      return "disabled";
  }
}

std::string paladin_t::default_flask() const
{
  std::string retribution_flask = (true_level > 100) ? "flask_of_the_countless_armies" :
                                  (true_level >= 90) ? "greater_draenic_strength_flask" :
                                  (true_level >= 85) ? "winters_bite" :
                                  (true_level >= 80) ? "titanic_strength" :
                                  "disabled";

  bool dps = (primary_role() == ROLE_ATTACK) || (talents.seraphim -> ok());
  std::string protection_flask = (true_level > 100) ? (dps ? "flask_of_the_countless_armies" : "flask_of_ten_thousand_scars") :
                                 (true_level >= 90) ? (dps ? "greater_draenic_strength_flask" : "greater_draenic_stamina_flask") :
                                 (true_level >= 85) ? "earth" :
                                 (true_level >= 80) ? "steelskin" :
                                 "disabled";

  std::string holy_dps_flask = (true_level > 100) ? "flask_of_the_whispered_pact" :
                               (true_level >= 90) ? "greater_draenic_intellect_flask" :
                               (true_level >= 85) ? "warm_sun" :
                               (true_level >= 80) ? "draconic_mind" :
                               "disabled";

  std::string holy_flask = (true_level > 100) ? "flask_of_the_whispered_pact" :
                           (true_level >= 90) ? "greater_draenic_intellect_flask" :
                           (true_level >= 85) ? "warm_sun" :
                           (true_level >= 80) ? "draconic_mind" :
                           "disabled";

  switch ( specialization() )
  {
    case PALADIN_RETRIBUTION:
      return retribution_flask;
    case PALADIN_PROTECTION:
      return protection_flask;
    case PALADIN_HOLY:
      return primary_role() == ROLE_ATTACK ? holy_dps_flask : holy_flask;
    default:
      return "disabled";
  }
}

std::string paladin_t::default_rune() const
{
  return (true_level >= 110) ? "defiled" :
         (true_level >= 100) ? "hyper" :
         "disabled";
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

  init_spells_retribution();
  init_spells_protection();
  init_spells_holy();

  // Shared Passives
  passives.boundless_conviction   = find_spell( 115675 ); // find_spell fails here
  passives.plate_specialization   = find_specialization_spell( "Plate Specialization" );
  passives.paladin                = find_spell( 137026 );  // find_spell fails here
  passives.retribution_paladin    = find_spell( 137027 );
  passives.protection_paladin     = find_spell( 137028 );
  passives.holy_paladin           = find_spell( 137029 );

  // Ret Passives
  passives.judgment             = find_spell( 231663 );

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
  if ( specialization() == PALADIN_RETRIBUTION || passives.guarded_by_the_light -> ok() )
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

// paladin_t::invalidate_cache ==============================================

void paladin_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  if ( ( specialization() == PALADIN_RETRIBUTION || passives.guarded_by_the_light -> ok() || passives.divine_bulwark -> ok() )
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
