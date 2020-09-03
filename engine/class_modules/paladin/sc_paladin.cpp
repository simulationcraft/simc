// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
/*
  TODO: Update Holy for BfA
*/
#include "simulationcraft.hpp"
#include "sc_paladin.hpp"

// ==========================================================================
// Paladin
// ==========================================================================
namespace paladin {

paladin_t::paladin_t( sim_t* sim, util::string_view name, race_e r ) :
  player_t( sim, PALADIN, name, r ),
  active( active_spells_t() ),
  buffs( buffs_t() ),
  gains( gains_t() ),
  spec( spec_t() ),
  cooldowns( cooldowns_t() ),
  passives( passives_t() ),
  mastery( mastery_t() ),
  procs( procs_t() ),
  spells( spells_t() ),
  talents( talents_t() ),
  options( options_t() ),
  beacon_target( nullptr ),
  lucid_dreams_accumulator( 0.0 )
{
  active_consecration = nullptr;

  cooldowns.avenging_wrath          = get_cooldown( "avenging_wrath" );
  cooldowns.hammer_of_justice       = get_cooldown( "hammer_of_justice" );
  cooldowns.judgment_of_light_icd   = get_cooldown( "judgment_of_light_icd" );

  cooldowns.holy_shock              = get_cooldown( "holy_shock");
  cooldowns.light_of_dawn           = get_cooldown( "light_of_dawn");

  cooldowns.avengers_shield         = get_cooldown( "avengers_shield" );
  cooldowns.consecration            = get_cooldown( "consecration" );
  cooldowns.hand_of_the_protector   = get_cooldown( "hand_of_the_protector" );
  cooldowns.inner_light_icd         = get_cooldown( "inner_light_icd" );
  cooldowns.judgment                = get_cooldown( "judgment" );
  cooldowns.light_of_the_protector  = get_cooldown( "light_of_the_protector" );
  cooldowns.shield_of_the_righteous = get_cooldown( "shield_of_the_righteous" );

  cooldowns.blade_of_justice        = get_cooldown( "blade_of_justice" );
  cooldowns.final_reckoning         = get_cooldown( "final_reckoning" );

  beacon_target = nullptr;
  resource_regeneration = regen_type::DYNAMIC;
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
  avenging_wrath_buff_t::avenging_wrath_buff_t( paladin_t* p ) :
      buff_t( p, "avenging_wrath", p -> spells.avenging_wrath ),
      damage_modifier( 0.0 ),
      healing_modifier( 0.0 ),
      crit_bonus( 0.0 )
  {
    // Map modifiers appropriately based on spec
    healing_modifier = data().effectN( 4 ).percent();
    crit_bonus = data().effectN( 3 ).percent();
    damage_modifier = data().effectN( 1 ).percent();

    // Lengthen duration if Sanctified Wrath is taken
    switch ( p -> specialization() )
    {
    case PALADIN_HOLY:
      base_buff_duration *= 1.0 + p -> talents.holy_sanctified_wrath -> effectN( 1 ).percent();
      break;
    case PALADIN_RETRIBUTION:
      base_buff_duration *= 1.0 + p -> talents.ret_sanctified_wrath -> effectN( 1 ).percent();
      break;
    case PALADIN_PROTECTION:
      base_buff_duration *= 1.0 + p -> talents.prot_sanctified_wrath -> effectN( 1 ).percent();
      break;
    default:
      break;
    }

    // ... or if we have Light's Decree
    if ( p -> azerite.lights_decree.ok() )
      base_buff_duration += p -> spells.lights_decree -> effectN( 2 ).time_value();

    // let the ability handle the cooldown
    cooldown -> duration = 0_ms;

    // invalidate Healing
    add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );
  }
}

 // end namespace buffs
// ==========================================================================
// End Paladin Buffs, Part One
// ==========================================================================

// Blessing of Protection =====================================================

struct blessing_of_protection_t : public paladin_spell_t
{
  blessing_of_protection_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "blessing_of_protection", p, p -> find_class_spell( "Blessing of Protection" ) )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    paladin_spell_t::execute();

    // apply forbearance, track locally for forbearant faithful & force recharge recalculation
    p() -> trigger_forbearance( execute_state -> target );
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( candidate_target -> debuffs.forbearance -> check() )
      return false;

    return paladin_spell_t::target_ready( candidate_target );
  }
};

// Avenging Wrath ===========================================================
// Most of this can be found in buffs::avenging_wrath_buff_t, this spell just triggers the buff

struct avenging_wrath_t : public paladin_spell_t
{
  avenging_wrath_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "avenging_wrath", p, p -> spells.avenging_wrath )
  {
    parse_options( options_str );

    if ( p -> talents.crusade -> ok() )
      background = true;

    harmful = false;

    // link needed for Righteous Protector / SotR cooldown reduction
    cooldown = p -> cooldowns.avenging_wrath;

    cooldown -> duration *= 1.0 + azerite::vision_of_perfection_cdr( p -> azerite_essence.vision_of_perfection );
  }

  void execute() override
  {
    paladin_spell_t::execute();

    p() -> buffs.avenging_wrath -> trigger();

    if ( p() -> azerite.avengers_might.ok() )
      p() -> buffs.avengers_might -> trigger( 1, p() -> buffs.avengers_might -> default_value, -1.0, p() -> buffs.avenging_wrath -> buff_duration() );

    p() -> buffs.avenging_wrath_autocrit -> trigger();
  }
};

// Holy Avenger
struct holy_avenger_t : public paladin_spell_t
{
  holy_avenger_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "holy_avenger", p, p -> talents.holy_avenger )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    paladin_spell_t::execute();

    p() -> buffs.holy_avenger -> trigger();
  }
};

// seraphim
struct seraphim_t : public holy_power_consumer_t
{
  seraphim_t( paladin_t* p, const std::string& options_str) :
    holy_power_consumer_t( "seraphim", p, p -> talents.seraphim )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    holy_power_consumer_t::execute();

    p() -> buffs.seraphim -> trigger();
  }
};

// Consecration =============================================================

struct consecration_tick_t : public paladin_spell_t
{
  consecration_tick_t( paladin_t* p ) :
    paladin_spell_t( "consecration_tick", p, p -> find_spell( 81297 ) )
  {
    aoe = -1;
    dual = true;
    direct_tick = true;
    background = true;
    may_crit = true;
    ground_aoe = true;
  }
};

struct consecration_t : public paladin_spell_t
{
  consecration_tick_t* damage_tick;
  ground_aoe_params_t cons_params;

  double precombat_time;

  consecration_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "consecration", p, p -> find_spell( "Consecration" ) ),
    damage_tick( new consecration_tick_t( p ) ), precombat_time( 2.0 )
  {
    add_option( opt_float( "precombat_time", precombat_time ) );
    parse_options( options_str );

    dot_duration = 0_ms; // the periodic event is handled by ground_aoe_event_t
    may_miss = harmful = false;

    add_child( damage_tick );
  }

  void init_finished() override
  {
    paladin_spell_t::init_finished();

    if ( action_list -> name_str == "precombat" )
    {
      double MIN_TIME = player -> base_gcd.total_seconds(); // the player's base unhasted gcd: 1.5s
      double MAX_TIME = cooldown -> duration.total_seconds() - 1;

      // Ensure that we're using a positive value
      if ( precombat_time < 0 )
        precombat_time = -precombat_time;

      if ( precombat_time > MAX_TIME )
      {
        precombat_time = MAX_TIME;
        sim -> error( "{} tried to use consecration in precombat more than {} seconds before combat begins, setting to {}",
                       player -> name(), MAX_TIME, MAX_TIME );
      }
      else if ( precombat_time < MIN_TIME )
      {
        precombat_time = MIN_TIME;
        sim -> error( "{} tried to use consecration in precombat less than {} before combat begins, setting to {} (has to be >= base gcd)",
                       player -> name(), MIN_TIME, MIN_TIME );
      }
    }
    else precombat_time = 0;

    timespan_t cons_duration = data().duration();

    // Add a one second penalty for the aoe's duration on top of the precombat time
    // Simulates the boss moving to the area during the first second of the fight
    if ( precombat_time > 0 )
      cons_duration -= timespan_t::from_seconds( precombat_time + 1 );

    cons_params = ground_aoe_params_t()
      .duration( cons_duration )
      .hasted( ground_aoe_params_t::SPELL_HASTE )
      .action( damage_tick )
      .state_callback( [ this ]( ground_aoe_params_t::state_type type, ground_aoe_event_t* event ) {
      switch ( type )
      {
      case ground_aoe_params_t::EVENT_CREATED:
        p() -> active_consecration = event;
        break;
      case ground_aoe_params_t::EVENT_DESTRUCTED:
        p() -> active_consecration = nullptr;
        break;
      default:
        break;
      } } );
  }

  void execute() override
  {
    // Cancel the current consecration if it exists
    if ( p() -> active_consecration != nullptr )
      event_t::cancel( p() -> active_consecration );

    paladin_spell_t::execute();

    // Some parameters must be updated on each cast
    cons_params.target( execute_state -> target )
               .start_time( sim -> current_time() );

    if ( sim -> distance_targeting_enabled )
      cons_params.x( p() -> x_position )
                 .y( p() -> y_position );

    if ( ! player -> in_combat && precombat_time > 0 )
    {
      // Adjust cooldown if consecration is used in precombat
      p() -> cooldowns.consecration -> adjust( timespan_t::from_seconds( -precombat_time ), false );

      // Create an event that starts consecration's aoe one second after combat starts
      make_event( *sim, 1_s, [ this ]( )
      {
        make_event<ground_aoe_event_t>( *sim, p(), cons_params, true /* Immediate pulse */ );
      } );

      sim -> print_debug( "{} used Consecration in precombat with precombat time = {}, adjusting duration and remaining cooldown.",
                          player -> name(), precombat_time );

    }

    else
      make_event<ground_aoe_event_t>( *sim, p(), cons_params, true /* Immediate pulse */ );
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

  void execute() override
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

  bool ready() override
  {
    if ( player -> debuffs.forbearance -> check() )
      return false;

    return paladin_spell_t::ready();
  }
};

// Divine Steed (Protection, Retribution) =====================================

struct divine_steed_t : public paladin_spell_t
{
  divine_steed_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "divine_steed", p, p -> find_class_spell( "Divine Steed" ) )
  {
    parse_options( options_str );
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
};

// Blessing of Sacrifice ========================================================

struct blessing_of_sacrifice_redirect_t : public paladin_spell_t
{
  blessing_of_sacrifice_redirect_t( paladin_t* p ) :
    paladin_spell_t( "blessing_of_sacrifice_redirect", p, p -> find_class_spell( "Blessing of Sacrifice" ) )
  {
    background = true;
    trigger_gcd = 0_ms;
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
    paladin_spell_t( "blessing_of_sacrifice", p, p -> find_class_spell( "Blessing of Sacrifice" ) )
  {
    parse_options( options_str );

    harmful = false;
    may_miss = false;

    // Create redirect action conditionalized on the existence of HoS.
    if ( ! p -> active.blessing_of_sacrifice_redirect )
      p -> active.blessing_of_sacrifice_redirect = new blessing_of_sacrifice_redirect_t( p );
  }

  void execute() override;

};

// Judgment of Light proc =====================================================

struct judgment_of_light_proc_t : public paladin_heal_t
{
  judgment_of_light_proc_t( paladin_t* p ) :
    paladin_heal_t( "judgment_of_light", p, p -> find_spell( 183811 ) ) // proc data stored in 183811
  {
    background = proc = may_crit = true;
    may_miss = false;

    // NOTE: this is implemented in SimC as a self-heal only. It does NOT proc for other players attacking the boss.
    // This is mostly done because it's much simpler to code, and for the most part Prot doesn't care about raid healing efficiency.
    // If Holy wants this to work like the in-game implementation, they'll have to go through the pain of moving things to player_t
  }
};

// Lay on Hands Spell =======================================================

struct lay_on_hands_t : public paladin_heal_t
{
  lay_on_hands_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "lay_on_hands", p, p -> find_class_spell( "Lay on Hands" ) )
  {
    parse_options( options_str );
    // unbreakable spirit reduces cooldown
    if ( p -> talents.unbreakable_spirit -> ok() )
    {
      cooldown -> duration *= 1.0 + p -> talents.unbreakable_spirit -> effectN( 1 ).percent();
    }

    may_crit = false;
    use_off_gcd = true;
    trigger_gcd = 0_ms;
  }

  void execute() override
  {
    base_dd_min = base_dd_max = p() -> resources.max[ RESOURCE_HEALTH ];

    paladin_heal_t::execute();

    // apply forbearance, track locally for forbearant faithful & force recharge recalculation
    p() -> trigger_forbearance( execute_state -> target );
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( candidate_target -> debuffs.forbearance -> check() )
      return false;

    return paladin_heal_t::target_ready( candidate_target );
  }
};

// Blinding Light (Holy/Prot/Retribution) =====================================

struct blinding_light_t : public paladin_spell_t
{
  blinding_light_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "blinding_light", p, p -> find_talent_spell( "Blinding Light" ) )
  {
    parse_options( options_str );

    aoe = -1;

    // TODO: Apply the cc?
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
    trigger_gcd           = 0_ms;
    base_execute_time     = p -> main_hand_weapon.swing_time;
    weapon_multiplier     = 1.0;

    affected_by.avenging_wrath = affected_by.crusade = true;
  }

  timespan_t execute_time() const override
  {
    if ( ! player -> in_combat ) return 10_ms;
    if ( first )
      return 0_ms;
    else
      return paladin_melee_attack_t::execute_time();
  }

  void execute() override
  {
    if ( first )
      first = false;

    paladin_melee_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p() -> specialization() == PALADIN_RETRIBUTION )
      {
        // Check for BoW procs
        double aow_proc_chance = p() -> passives.art_of_war -> effectN( 1 ).percent();
        if ( p() -> passives.art_of_war_2 -> ok() )
          aow_proc_chance += p() -> passives.art_of_war_2 -> effectN( 1 ).percent();

        if ( p() -> talents.blade_of_wrath -> ok() )
          aow_proc_chance *= 1.0 + p() -> talents.blade_of_wrath -> effectN( 1 ).percent();

        if ( rng().roll( aow_proc_chance ) )
        {
          p() -> procs.art_of_war -> occur();

          if ( p() -> talents.blade_of_wrath -> ok() )
            p() -> buffs.blade_of_wrath -> trigger();

          p() -> cooldowns.blade_of_justice -> reset( true );
        }

        if ( p() -> buffs.zeal -> up() && p() -> active.zeal )
        {
          p() -> active.zeal -> set_target( execute_state -> target );
          p() -> active.zeal -> schedule_execute();
          p() -> buffs.zeal -> decrement();
        }
      }
    }
  }
};

// Auto Attack ==============================================================

struct auto_melee_attack_t : public paladin_melee_attack_t
{
  auto_melee_attack_t( paladin_t* p, const std::string& options_str ) :
    paladin_melee_attack_t( "auto_attack", p, spell_data_t::nil() )
  {
    school = SCHOOL_PHYSICAL;
    assert( p -> main_hand_weapon.type != WEAPON_NONE );
    p -> main_hand_attack = new melee_t( p );

    // does not incur a GCD
    trigger_gcd = 0_ms;

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

    return( p() -> main_hand_attack -> execute_event == nullptr ); // not swinging
  }
};

// Crusader Strike ==========================================================

struct crusader_strike_t : public paladin_melee_attack_t
{
  bool has_crusader_2;
  crusader_strike_t( paladin_t* p, const std::string& options_str ) :
    paladin_melee_attack_t( "crusader_strike", p, p -> find_class_spell( "Crusader Strike" ) )
  {
    parse_options( options_str );

    if ( p -> talents.fires_of_justice -> ok() )
    {
      cooldown -> duration *= 1.0 + p -> talents.fires_of_justice -> effectN( 3 ).percent();
    }

    const spell_data_t* crusader_strike_2 = p -> find_specialization_spell( 342348 );
    if ( crusader_strike_2 )
    {
      has_crusader_2 = true;
    }
    else
    {
      has_crusader_2 = false;
    }

    const spell_data_t* crusader_strike_3 = p -> find_specialization_spell( 231667 );
    if ( crusader_strike_3 )
    {
      cooldown -> charges += as<int>( crusader_strike_3 -> effectN( 1 ).base_value() );
    }
  }

  void impact( action_state_t* s ) override
  {
    paladin_melee_attack_t::impact( s );

    if ( p() -> talents.crusaders_might -> ok() )
    {
      timespan_t cm_cdr = p() -> talents.crusaders_might -> effectN( 1 ).time_value();
      p() -> cooldowns.holy_shock -> adjust( cm_cdr );
      p() -> cooldowns.light_of_dawn -> adjust( cm_cdr );
    }

    // Special things that happen when CS connects
    if ( result_is_hit( s -> result ) )
    {
      // fires of justice
      if ( p() -> talents.fires_of_justice -> ok() )
      {
        if ( p() -> buffs.fires_of_justice -> trigger() )
          p() -> procs.fires_of_justice -> occur();
      }

      if ( p() -> talents.empyrean_power -> ok() )
      {
        if ( rng().roll( p() -> talents.empyrean_power -> effectN( 1 ).percent() ) )
        {
          p() -> procs.empyrean_power -> occur();
          p() -> buffs.empyrean_power -> trigger();
        }
      }

      if ( p() -> specialization() == PALADIN_RETRIBUTION )
      {
        p() -> resource_gain( RESOURCE_HOLY_POWER, p() -> spec.retribution_paladin -> effectN( 14 ).base_value(), p() -> gains.hp_cs );
      }
    }
  }

  double cost() const override
  {
    if ( has_crusader_2 )
      return 0;

    return paladin_melee_attack_t::cost();
  }
};

// Hammer of Justice ========================================================

struct hammer_of_justice_t : public paladin_melee_attack_t
{
  hammer_of_justice_t( paladin_t* p, const std::string& options_str ) :
    paladin_melee_attack_t( "hammer_of_justice", p, p -> find_class_spell( "Hammer of Justice" ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    //TODO implement stun?
  }
};

// Holy Shield damage proc ==================================================

struct holy_shield_damage_t : public paladin_spell_t
{
  holy_shield_damage_t( paladin_t* p ) :
    paladin_spell_t( "holy_shield", p, p -> talents.holy_shield -> effectN( 2 ).trigger() )
  {
    background = proc = may_crit = true;
    may_miss = false;
  }
};

// Inner light damage proc ==================================================

struct inner_light_damage_t : public paladin_spell_t
{
  inner_light_damage_t( paladin_t* p ) :
    paladin_spell_t( "inner_light", p, p -> find_spell( 275483 ) )
  {
    background = true;
    base_dd_min = base_dd_max = p -> azerite.inner_light.value( 2 );
  }
};

// Holy power consumers =====================================================

double holy_power_consumer_t::cost() const
{
  if ( background )
  {
    return 0.0;
  }

  if ( ( is_divine_storm && ( p() -> buffs.empyrean_power_azerite -> check() || p() -> buffs.empyrean_power -> check() ) ) ||
       p() -> buffs.divine_purpose -> check() )
  {
    return 0.0;
  }

  double c = paladin_melee_attack_t::cost();

  if ( p() -> buffs.fires_of_justice -> check() )
  {
    c += p() -> buffs.fires_of_justice -> data().effectN( 1 ).base_value();
  }

  return c;
}

void holy_power_consumer_t::execute()
{
  double hp_used = cost();

  paladin_melee_attack_t::execute();

  // Crusade and Relentless Inquisitor gain full stacks from free spells, but reduced stacks with FoJ
  if ( p() -> buffs.crusade -> check() )
  {
    int num_stacks = as<int>( hp_used == 0 ? base_costs[ RESOURCE_HOLY_POWER ] : hp_used );
    p() -> buffs.crusade -> trigger( num_stacks );
  }

  if ( p() -> azerite.relentless_inquisitor.ok() )
  {
    int num_stacks = as<int>( hp_used == 0 ? base_costs[ RESOURCE_HOLY_POWER ] : hp_used );

    p() -> buffs.relentless_inquisitor -> trigger( num_stacks );
  }

  // Consume Empyrean Power on Divine Storm, handled here for interaction with DP/FoJ
  // Cost reduction is still in divine_storm_t
  if ( p() -> buffs.empyrean_power_azerite -> up() && is_divine_storm )
  {
    p() -> buffs.empyrean_power_azerite -> expire();
  }
  else if ( p() -> buffs.empyrean_power -> up() && is_divine_storm )
  {
    p() -> buffs.empyrean_power -> expire();
  }
  // Divine Purpose isn't consumed on DS if EP was consumed
  else if ( p() -> buffs.divine_purpose -> up() )
  {
    p() -> buffs.divine_purpose -> expire();
  }
  // FoJ isn't consumed if EP or DP were consumed
  else if ( p() -> buffs.fires_of_justice -> up() )
  {
    p() -> buffs.fires_of_justice -> expire();
  }

  // Roll for Divine Purpose
  if ( p() -> talents.divine_purpose -> ok() &&
       rng().roll( p() -> talents.divine_purpose -> effectN( 1 ).percent() ) )
  {
    p() -> buffs.divine_purpose -> trigger();
    p() -> procs.divine_purpose -> occur();
  }

  if ( p() -> buffs.avenging_wrath -> up() || p() -> buffs.crusade -> up() )
  {
    if ( p() -> azerite.lights_decree.ok() )
    {
      lights_decree_t* ld = debug_cast<lights_decree_t*>( p() -> active.lights_decree );
      ld -> last_holy_power_cost = as<int>( base_costs[ RESOURCE_HOLY_POWER ] );
      ld -> execute();
    }

    if ( p() -> talents.ret_sanctified_wrath -> ok() )
    {
      sanctified_wrath_t* st = debug_cast<sanctified_wrath_t*>( p() -> active.sanctified_wrath );
      st -> last_holy_power_cost = as<int>( base_costs[ RESOURCE_HOLY_POWER ] );
      st -> execute();
    }
  }
}

void holy_power_consumer_t::consume_resource()
{
  paladin_melee_attack_t::consume_resource();

  if ( current_resource() == RESOURCE_HOLY_POWER)
  {
    p() -> trigger_memory_of_lucid_dreams( last_resource_cost );
  }
}

// Base Judgment spell ======================================================

void judgment_t::do_ctor_common( paladin_t* p )
{
  // no weapon multiplier
  weapon_multiplier = 0.0;
  may_block = may_parry = may_dodge = false;

  // Handle indomitable justice option
  if ( p -> azerite.indomitable_justice.enabled() )
  {
    // If using the default setting, set to 80% hp for protection, 100% hp for other specs
    if ( p -> options.indomitable_justice_pct == 0 )
    {
      indomitable_justice_pct = p -> specialization() == PALADIN_PROTECTION ? 80 : 100;
    }
    // Else, clamp the value between -1 ("real" usage) and 100
    else
    {
      indomitable_justice_pct = clamp<int>( p -> options.indomitable_justice_pct, -1, 100 );
    }
  }

  if ( p -> spells.judgment_2 -> ok() )
  {
    base_multiplier *= 1.0 + p -> spells.judgment_2 -> effectN( 1 ).percent();
  }
}

judgment_t::judgment_t( paladin_t* p, const std::string& options_str ) :
    paladin_melee_attack_t( "judgment", p, p -> find_spell( 20271 ) )
{
  parse_options( options_str );
  do_ctor_common( p );
}

judgment_t::judgment_t( paladin_t* p ) :
    paladin_melee_attack_t( "judgment", p, p -> find_spell( 20271 ) )
{
  do_ctor_common( p );
}

double judgment_t::bonus_da( const action_state_t* s ) const
{
  double da = paladin_melee_attack_t::bonus_da( s );
  if ( p() -> azerite.indomitable_justice.ok() )
  {
    double player_percent = indomitable_justice_pct < 0 ? p() -> health_percentage() : indomitable_justice_pct;
    double target_percent = s -> target -> health_percentage();

    if ( player_percent > target_percent )
    {
      double ij_bonus = p() -> azerite.indomitable_justice.value();

      ij_bonus *= ( player_percent - target_percent ) / 100.0;

      // Protection has a 50% damage reduction to IJ
      ij_bonus *= 1.0 + p() -> spec.protection_paladin -> effectN( 14 ).percent();

      da += ij_bonus;
    }
  }
  return da;
}

proc_types judgment_t::proc_type() const
{
  return PROC1_MELEE_ABILITY;
}

void judgment_t::impact( action_state_t* s )
{
  if ( result_is_hit( s -> result ) )
  {
    if ( p() -> talents.judgment_of_light -> ok() )
      td( s -> target ) -> debuff.judgment_of_light -> trigger( 25 );
  }

  paladin_melee_attack_t::impact( s );
}

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

  bool target_ready( player_t* candidate_target ) override
  {
    if ( ! candidate_target -> debuffs.casting || ! candidate_target -> debuffs.casting -> check() )
      return false;

    return paladin_melee_attack_t::target_ready( candidate_target );
  }
};

// Reckoning ==================================================================

struct hand_of_reckoning_t: public paladin_melee_attack_t
{
  hand_of_reckoning_t( paladin_t* p, const std::string& options_str ) :
    paladin_melee_attack_t( "hand_of_reckoning", p, p -> find_class_spell( "Hand of Reckoning" ) )
  {
    parse_options( options_str );
    use_off_gcd = true;
  }

  void impact( action_state_t* s ) override
  {
    if ( s -> target -> is_enemy() )
      target -> taunt( player );

    paladin_melee_attack_t::impact( s );
  }
};

// Covenants =======

struct vanquishers_hammer_t : public holy_power_consumer_t
{
  vanquishers_hammer_t( paladin_t* p, const std::string& options_str ) :
    holy_power_consumer_t( "vanquishers_hammer", p, p -> covenant.necrolord )
  {
    parse_options( options_str );

    base_multiplier *= 1.0 + p -> conduit.righteous_might.percent(); // todo: implement heal
  }

  void impact( action_state_t* s ) override
  {
    holy_power_consumer_t::impact( s );

    p() -> buffs.vanquishers_hammer -> trigger();
  }
};

// TODO: implement divine toll conduit
struct divine_toll_t : public paladin_spell_t
{
  divine_toll_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "divine_toll", p, p -> covenant.kyrian )
  {
    parse_options( options_str );

    aoe = data().effectN( 1 ).base_value();

    add_child( p -> active.divine_toll );
  }

  void impact( action_state_t* s ) override
  {
    paladin_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      p() -> active.divine_toll -> set_target( s -> target );
      p() -> active.divine_toll -> schedule_execute();
    }
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
    buff_t( p, "blessing_of_sacrifice", p -> find_spell( 6940 ) ),
    source( nullptr ),
    source_health_pool( 0.0 )
  { }

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
  bool trigger( int, double value, double, timespan_t ) override
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

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    source = nullptr;
    source_health_pool = 0.0;
  }

  void reset() override
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
  debuff.blessed_hammer = make_buff( *this, "blessed_hammer", paladin -> find_spell( 204301 ) );
  debuff.execution_sentence = make_buff<buffs::execution_sentence_debuff_t>( this );
  debuff.judgment = make_buff( *this, "judgment", paladin -> spells.judgment_debuff );
  debuff.judgment_of_light = make_buff( *this, "judgment_of_light", paladin -> find_spell( 196941 ) );
  debuff.final_reckoning = make_buff( *this, "final_reckoning", paladin -> talents.final_reckoning );
  debuff.reckoning = make_buff( *this, "reckoning", paladin -> spells.reckoning );
}

// paladin_t::create_actions ================================================

void paladin_t::create_actions()
{
  // Holy
  if ( specialization() == PALADIN_HOLY )
  {
    paladin_t::create_holy_actions();
  }
  // Prot
  else if ( specialization() == PALADIN_PROTECTION )
  {
    paladin_t::create_prot_actions();

    // Prot only but handled in the core paladin module
    if ( talents.holy_shield -> ok() )
    {
      active.holy_shield_damage = new holy_shield_damage_t( this );
    }

    if ( azerite.inner_light.enabled() )
    {
      active.inner_light_damage = new inner_light_damage_t( this );
      cooldowns.inner_light_icd -> duration = find_spell( 275481 ) -> internal_cooldown();
    }
  }
  // Ret
  else if ( specialization() == PALADIN_RETRIBUTION )
  {
    paladin_t::create_ret_actions();
  }

  if ( talents.judgment_of_light -> ok() )
  {
    active.judgment_of_light = new judgment_of_light_proc_t( this );
    cooldowns.judgment_of_light_icd -> duration = timespan_t::from_seconds( talents.judgment_of_light -> effectN( 1 ).base_value() );
  }

  player_t::create_actions();
}

// paladin_t::create_action =================================================

action_t* paladin_t::create_action( util::string_view name, const std::string& options_str )
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
  if ( name == "rebuke"                    ) return new rebuke_t                   ( this, options_str );
  if ( name == "hand_of_reckoning"         ) return new hand_of_reckoning_t        ( this, options_str );
  if ( name == "flash_of_light"            ) return new flash_of_light_t           ( this, options_str );
  if ( name == "lay_on_hands"              ) return new lay_on_hands_t             ( this, options_str );
  if ( name == "holy_avenger"              ) return new holy_avenger_t             ( this, options_str );
  if ( name == "seraphim"                  ) return new seraphim_t                 ( this, options_str );

  if ( name == "vanquishers_hammer"        ) return new vanquishers_hammer_t       ( this, options_str );
  if ( name == "divine_toll"               ) return new divine_toll_t              ( this, options_str );

  return player_t::create_action( name, options_str );
}

void paladin_t::trigger_forbearance( player_t* target )
{
  auto buff = debug_cast<buffs::forbearance_t*>( target -> debuffs.forbearance );

  buff -> paladin = this;
  buff -> trigger();
}

void paladin_t::trigger_memory_of_lucid_dreams( double cost )
{
  if ( !azerite_essence.memory_of_lucid_dreams.enabled() )
    return;

  if ( cost <= 0 )
    return;

  if ( specialization() == PALADIN_RETRIBUTION ) {
    if ( ! rng().roll( options.proc_chance_ret_memory_of_lucid_dreams ) )
      return;

    double total_gain = lucid_dreams_accumulator + cost * lucid_dreams_minor_refund_coeff;

    // mserrano note: apparently when you get a proc on a 1-holy-power spender, if it did proc,
    // you always get 1 holy power instead of alternating between 0 and 1. This is based on
    // Skeletor's PTR testing; should revisit this periodically.
    if ( cost == 1 && total_gain < 1 ) {
      total_gain = 1;
    }

    double real_gain = floor( total_gain );

    lucid_dreams_accumulator = total_gain - real_gain;

    resource_gain( RESOURCE_HOLY_POWER, real_gain, gains.hp_memory_of_lucid_dreams );
  }

  else if ( specialization() == PALADIN_PROTECTION )
  {
    if ( ! rng().roll( options.proc_chance_prot_memory_of_lucid_dreams ) )
      return;

    cooldowns.shield_of_the_righteous -> adjust( -1.0 * cooldown_t::cooldown_duration( cooldowns.shield_of_the_righteous ) * lucid_dreams_minor_refund_coeff );

    procs.prot_lucid_dreams -> occur();
  }

  if ( azerite_essence.memory_of_lucid_dreams.rank() >= 3 )
    player_t::buffs.lucid_dreams -> trigger();
}

// TODO?: holy specifics
void paladin_t::vision_of_perfection_proc()
{
  auto vision = azerite_essence.vision_of_perfection;
  double vision_multiplier = vision.spell( 1u, essence_type::MAJOR ) -> effectN( 1 ).percent() +
                             vision.spell( 2u, essence_spell::UPGRADE, essence_type::MAJOR ) -> effectN( 1 ).percent();
  if ( vision_multiplier <= 0 )
    return;

  buff_t* main_buff = buffs.avenging_wrath;
  if ( talents.crusade -> ok() )
    main_buff =  buffs.crusade;

  buff_t* autocrit_buff = talents.crusade -> ok() ? nullptr : buffs.avenging_wrath_autocrit;

  // Light's Decree's duration increase to AW doesn't affect the VoP proc
  // We use the duration from spelldata rather than buff -> buff_duration
  timespan_t trigger_duration = vision_multiplier * main_buff -> data().duration();

  if ( main_buff -> check() )
  {
    main_buff -> extend_duration( this, trigger_duration );

    if ( azerite.avengers_might.enabled() )
      buffs.avengers_might -> extend_duration( this, trigger_duration );
  }
  else
  {
    main_buff -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, trigger_duration );

    if ( autocrit_buff )
      autocrit_buff -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, trigger_duration );

    if ( azerite.avengers_might.enabled() )
      buffs.avengers_might -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, trigger_duration );
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

// paladin_t::init_base =====================================================

void paladin_t::init_base_stats()
{
  if ( base.distance < 1 )
  {
    base.distance = 5;
  }

  player_t::init_base_stats();

  base.attack_power_per_agility = 0.0;
  base.attack_power_per_strength = 1.0;
  base.spell_power_per_intellect = 1.0;

  // Ignore mana for non-holy
  if ( specialization() != PALADIN_HOLY )
  {
    resources.base[ RESOURCE_MANA ] = 0;
    resources.base_regen_per_second[ RESOURCE_MANA ] = 0;
  }

  if ( specialization() == PALADIN_RETRIBUTION )
  {
    // Boundless Conviction raises max holy power to 5
    resources.base[ RESOURCE_HOLY_POWER ] = 3 + passives.boundless_conviction -> effectN( 1 ).base_value();
  }

  if ( specialization() == PALADIN_HOLY )
  {
    resources.base_regen_per_second[ RESOURCE_MANA ] = resources.base[ RESOURCE_MANA ] * 0.015;
  }

  // Avoidance diminishing Returns constants/conversions now handled in player_t::init_base_stats().
  // Base miss, dodge, parry, and block are set in player_t::init_base_stats().
  // Just need to add class- or spec-based modifiers here.
  // add Sanctuary dodge
  base.dodge += passives.sanctuary -> effectN( 3 ).percent();
  // add Sanctuary expertise
  base.expertise += passives.sanctuary -> effectN( 4 ).percent();
}

// paladin_t::reset =========================================================

void paladin_t::reset()
{
  player_t::reset();

  active_consecration = nullptr;
}

// paladin_t::init_gains ====================================================

void paladin_t::init_gains()
{
  player_t::init_gains();

  // Mana
  gains.mana_beacon_of_light        = get_gain( "beacon_of_light" );

  // Health
  gains.holy_shield                 = get_gain( "holy_shield_absorb" );

  // Holy Power
  gains.hp_templars_verdict_refund  = get_gain( "templars_verdict_refund" );
  gains.judgment                    = get_gain( "judgment" );
  gains.hp_cs                       = get_gain( "crusader_strike" );
  gains.hp_memory_of_lucid_dreams   = get_gain( "memory_of_lucid_dreams" );
}

// paladin_t::init_procs ====================================================

void paladin_t::init_procs()
{
  player_t::init_procs();

  procs.art_of_war                = get_proc( "Art of War"       );
  procs.divine_purpose            = get_proc( "Divine Purpose"   );
  procs.fires_of_justice          = get_proc( "Fires of Justice" );
  procs.grand_crusader            = get_proc( "Grand Crusader"   );
  procs.prot_lucid_dreams         = get_proc( "Lucid Dreams SotR");
  procs.final_reckoning           = get_proc( "Final Reckoning"  );
  procs.empyrean_power            = get_proc( "Empyrean Power"   );
}

// paladin_t::init_scaling ==================================================

void paladin_t::init_scaling()
{
  player_t::init_scaling();

  switch ( specialization() )
  {
    case PALADIN_HOLY:
      scaling -> enable( STAT_INTELLECT );
      scaling -> enable( STAT_SPELL_POWER );
      break;
    case PALADIN_PROTECTION:
      scaling -> enable( STAT_BONUS_ARMOR );
      break;
    default:
      break;
  }

  scaling -> disable( STAT_AGILITY );
}

// paladin_t::init_assessors ================================================

void paladin_t::init_assessors()
{
  player_t::init_assessors();

  if ( talents.execution_sentence -> ok() )
  {
    auto assessor_fn = [ this ] ( result_amount_type /* rt */, action_state_t* s )
    {
      auto td = this -> get_target_data( s -> target );

      if ( td -> debuff.execution_sentence -> check() )
      {
        td -> debuff.execution_sentence -> accumulate_damage( s );
      }

      return assessor::CONTINUE;
    };

    assessor_out_damage.add( assessor::TARGET_DAMAGE - 1, assessor_fn );
    for ( auto pet : pet_list )
      pet -> assessor_out_damage.add( assessor::TARGET_DAMAGE - 1, assessor_fn );
  }
}

// paladin_t::init_buffs ====================================================

void paladin_t::create_buffs()
{
  player_t::create_buffs();

  create_buffs_retribution();
  create_buffs_protection();
  create_buffs_holy();

  buffs.divine_steed = make_buff( this, "divine_steed", find_spell( "Divine Steed" ) )
                     -> set_duration( 3_s )
                     -> set_chance( 1.0 )
                     -> set_cooldown( 0_ms ) // handled by the ability
                     -> set_default_value( 1.0 ); // TODO: change this to spellid 221883 & see if that automatically captures details

  // General
  buffs.avenging_wrath          = new buffs::avenging_wrath_buff_t( this );
  buffs.avenging_wrath_autocrit = make_buff( this, "avenging_wrath_autocrit", spells.avenging_wrath_autocrit );
  buffs.divine_purpose          = make_buff( this, "divine_purpose", spells.divine_purpose_buff );
  buffs.divine_shield           = make_buff( this, "divine_shield", find_class_spell( "Divine Shield" ) )
                                -> set_cooldown( 0_ms ); // Let the ability handle the CD

  buffs.avengers_might = make_buff<stat_buff_t>( this, "avengers_might", find_spell( 272903 ) )
                       -> add_stat( STAT_MASTERY_RATING, azerite.avengers_might.value() );

  buffs.seraphim = make_buff( this, "seraphim", talents.seraphim )
                 -> add_invalidate( CACHE_CRIT_CHANCE )
                 -> add_invalidate( CACHE_HASTE )
                 -> add_invalidate( CACHE_MASTERY )
                 -> add_invalidate( CACHE_VERSATILITY )
                 -> set_cooldown( 0_ms ); // let the ability handle the cooldown

  buffs.holy_avenger = make_buff( this, "holy_avenger", talents.holy_avenger )
                     -> set_cooldown( 0_ms ); // handled by the ability

  // Covenants
  buffs.vanquishers_hammer = make_buff( this, "vanquishers_hammer", covenant.necrolord )
                             -> set_cooldown( 0_ms );
}

// paladin_t::default_potion ================================================

std::string paladin_t::default_potion() const
{
  std::string retribution_pot = (true_level > 110) ? "potion_of_focused_resolve" :
                                (true_level > 100) ? "old_war" :
                                (true_level >= 90) ? "draenic_strength" :
                                (true_level >= 85) ? "mogu_power" :
                                (true_level >= 80) ? "golemblood" :
                                "disabled";

  std::string protection_pot = (true_level > 110) ? "potion_of_unbridled_fury" :
                               (true_level > 100) ? "prolonged_power" :
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
  std::string retribution_food = (true_level > 110) ? "famine_evaluator_and_snack_table" :
                                 (true_level > 100) ? "azshari_salad" :
                                 (true_level >= 90) ? "sleeper_sushi" :
                                 (true_level >= 85) ? "black_pepper_ribs_and_shrimp" :
                                 (true_level >= 80) ? "beerbasted_crocolisk" :
                                 "disabled";

  std::string protection_food = (true_level > 110) ? "mechdowels_big_mech" :
                                (true_level > 100) ? "lavish_suramar_feast" :
                                (true_level >= 90) ? "pickled_eel" :
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
  std::string retribution_flask = (true_level > 110) ? "greater_flask_of_the_undertow" :
                                  (true_level > 100) ? "flask_of_the_countless_armies" :
                                  (true_level >= 90) ? "greater_draenic_strength_flask" :
                                  (true_level >= 85) ? "winters_bite" :
                                  (true_level >= 80) ? "titanic_strength" :
                                  "disabled";

  std::string protection_flask = (true_level > 110) ? "greater_flask_of_the_undertow" :
                                 (true_level > 100) ? "flask_of_the_countless_armies" :
                                 (true_level >= 90) ? "greater_draenic_strength_flask" :
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
  return (true_level >= 120) ? "battle_scarred" :
         (true_level >= 110) ? "defiled" :
         (true_level >= 100) ? "hyper" :
         "disabled";
}

// paladin_t::init_actions ==================================================

void paladin_t::init_action_list()
{
  // 2019-04-03: The Holy module is outdated and not supported (both for dps and healing)
  if ( !sim->allow_experimental_specializations && specialization() == PALADIN_HOLY )
  {
    if ( ! quiet )
      sim -> errorf( "Paladin holy for player %s is not currently supported.", name() );

    quiet = true;
    return;
  }

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

  // Shared talents
  talents.fist_of_justice    = find_talent_spell( "Fist of Justice" );
  talents.repentance         = find_talent_spell( "Repentance" );
  talents.blinding_light     = find_talent_spell( "Blinding Light" );
  talents.unbreakable_spirit = find_talent_spell( "Unbreakable Spirit" );
  talents.cavalier           = find_talent_spell( "Cavalier" );
  talents.holy_avenger       = find_talent_spell( "Holy Avenger" );
  talents.seraphim           = find_talent_spell( "Seraphim" );
  talents.divine_purpose     = find_talent_spell( "Divine Purpose" );

  // Shared Passives and spells
  passives.plate_specialization = find_specialization_spell( "Plate Specialization" );
  passives.paladin              = find_spell( 137026 );
  spells.avenging_wrath = find_class_spell( "Avenging Wrath" );
  spells.avenging_wrath_autocrit = find_spell( 294027 );
  spells.judgment_2 = find_spell( 327977 );

  // Shared Azerite traits
  azerite.avengers_might        = find_azerite_spell( "Avenger's Might" );
  azerite.grace_of_the_justicar = find_azerite_spell( "Grace of the Justicar" );
  azerite.indomitable_justice   = find_azerite_spell( "Indomitable Justice" );

  // Essences
  azerite_essence.memory_of_lucid_dreams = find_azerite_essence( "Memory of Lucid Dreams" );
  lucid_dreams_minor_refund_coeff = azerite_essence.memory_of_lucid_dreams.spell( 1u, essence_type::MINOR ) -> effectN( 1 ).percent();
  azerite_essence.vision_of_perfection = find_azerite_essence( "Vision of Perfection" );

  // Shadowlands legendaries
  legendary.vanguards_momentum = find_runeforge_legendary( "Vanguard's Momentum" );
  legendary.badge_of_the_mad_paragon = find_runeforge_legendary( "Badge of the Mad Paragon" );
  legendary.final_verdict = find_runeforge_legendary( "Final Verdict" );
  legendary.from_dusk_till_dawn = find_runeforge_legendary( "From Dusk till Dawn" );
  legendary.the_magistrates_judgment = find_runeforge_legendary( "The Magistrate's Judgment" );

  // Covenants
  covenant.kyrian = find_covenant_spell( "Divine Toll" );
  covenant.venthyr = find_covenant_spell( "Ashen Hollow" );
  covenant.necrolord = find_covenant_spell( "Vanquisher's Hammer" );
  covenant.night_fae = find_covenant_spell( "Blessing of the Seasons" ); // TODO: fix

  // Conduits
  // TODO: non-damage conduits
  conduit.ringing_clarity = find_conduit_spell( "Ringing Clarity" );
  conduit.vengeful_shock = find_conduit_spell( "Vengeful Shock" );
  conduit.focused_light = find_conduit_spell( "Focused Light" );
  conduit.lights_reach = find_conduit_spell( "Light's Reach" );
  conduit.templars_vindication = find_conduit_spell( "Templar's Vindication" );
  conduit.the_long_summer = find_conduit_spell( "The Long Summer" ); // TODO: implement
  conduit.truths_wake = find_conduit_spell( "Truth's Wake" );
  conduit.virtuous_command = find_conduit_spell( "Virtuous Command" );
  conduit.righteous_might = find_conduit_spell( "Righteous Might" );
  conduit.hallowed_discernment = find_conduit_spell( "Hallowed Discernment" ); // TODO: implement
  conduit.punish_the_guilty = find_conduit_spell( "Punish the Guilty" );
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

// paladin_t::primary_resource() ============================================

resource_e paladin_t::primary_resource() const
{
  if ( specialization() == PALADIN_HOLY || specialization() == PALADIN_PROTECTION )
    return RESOURCE_MANA;

  if ( specialization() == PALADIN_RETRIBUTION )
    return RESOURCE_HOLY_POWER;

  return RESOURCE_NONE;
}

// paladin_t::convert_hybrid_stat ===========================================

stat_e paladin_t::convert_hybrid_stat( stat_e s ) const
{
  // Holy's primary stat is intellect
  if ( specialization() == PALADIN_HOLY )
  {
    switch( s )
    {
      case STAT_STR_AGI_INT:
      case STAT_STR_INT:
      case STAT_AGI_INT:
        return STAT_INTELLECT;
      case STAT_STR_AGI:
      case STAT_STRENGTH:
      case STAT_AGILITY:
        return STAT_NONE;
      default:
        break;
    }
  }
  // Protection and Retribution use strength
  else
  {
    switch ( s )
    {
      case STAT_STR_AGI_INT:
      case STAT_STR_INT:
      case STAT_STR_AGI:
        return STAT_STRENGTH;
      case STAT_AGI_INT:
      case STAT_INTELLECT:
      case STAT_AGILITY:
        return STAT_NONE;
      default:
        break;
    }
  }

  // Handle non-primary stats
  switch ( s )
  {
    case STAT_SPIRIT:
      if ( specialization() != PALADIN_HOLY )
        return STAT_NONE;
      break;
    case STAT_BONUS_ARMOR:
      if ( specialization() != PALADIN_PROTECTION )
        return STAT_NONE;
      break;
    default:
      break;
  }

  return s;
}

// paladin_t::composite_attribute_multiplier ================================

double paladin_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double m = player_t::composite_attribute_multiplier( attr );

  // Protection gets increased stamina
  if ( attr == ATTR_STAMINA )
  {
    m *= 1.0 + spec.protection_paladin -> effectN( 3 ).percent();
  }

  return m;
}

double paladin_t::composite_damage_versatility() const
{
  double cdv = player_t::composite_damage_versatility();

  if ( buffs.seraphim -> up() )
    cdv += buffs.seraphim -> data().effectN( 2 ).percent();

  return cdv;
}

double paladin_t::composite_heal_versatility() const
{
  double chv = player_t::composite_heal_versatility();

  if ( buffs.seraphim -> up() )
    chv += buffs.seraphim -> data().effectN( 2 ).percent();

  return chv;
}

double paladin_t::composite_mitigation_versatility() const
{
  double cmv = player_t::composite_mitigation_versatility();

  if ( buffs.seraphim -> up() )
    cmv += buffs.seraphim -> data().effectN( 2 ).percent() / 2;

  return cmv;
}

double paladin_t::composite_mastery() const
{
  double m = player_t::composite_mastery();

  if( buffs.seraphim -> up() )
    m += buffs.seraphim -> data().effectN( 4 ).percent();

  return m;
}

double paladin_t::composite_spell_crit_chance() const
{
  double h = player_t::composite_spell_crit_chance();

  if ( buffs.seraphim -> up() )
    h += buffs.seraphim -> data().effectN( 1 ).percent();

  return h;
}

double paladin_t::composite_melee_crit_chance() const
{
  double h = player_t::composite_melee_crit_chance();

  if ( buffs.seraphim -> up() )
    h += buffs.seraphim -> data().effectN( 1 ).percent();

  return h;
}

// paladin_t::composite_melee_haste =========================================

double paladin_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  if ( buffs.crusade -> up() )
    h /= 1.0 + buffs.crusade -> get_haste_bonus();

  if ( buffs.seraphim -> up() )
    h /= 1.0 + buffs.seraphim -> data().effectN( 3 ).percent();

  return h;
}

// paladin_t::composite_melee_speed =========================================

double paladin_t::composite_melee_speed() const
{
  double s = player_t::composite_melee_speed();
  if ( buffs.zeal -> check() )
    s /= 1.0 + buffs.zeal -> data().effectN( 1 ).percent();
  return s;
}

// paladin_t::composite_spell_haste ==========================================

double paladin_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( buffs.crusade -> up() )
    h /= 1.0 + buffs.crusade -> get_haste_bonus();

  if ( buffs.seraphim -> up() )
    h /= 1.0 + buffs.seraphim -> data().effectN( 3 ).percent();

  return h;
}

// paladin_t::composite_bonus_armor =========================================

double paladin_t::composite_bonus_armor() const
{
  double ba = player_t::composite_bonus_armor();

  if ( buffs.shield_of_the_righteous -> check() )
  {
    shield_of_the_righteous_buff_t* sotr_buff = debug_cast<shield_of_the_righteous_buff_t*>( buffs.shield_of_the_righteous );

    ba += sotr_buff -> value() * cache.strength() * ( 1.0 + sotr_buff -> avengers_valor_increase );
  }

  return ba;
}

// paladin_t::composite_spell_power =========================================

double paladin_t::composite_spell_power( school_e school ) const
{
  double sp = player_t::composite_spell_power( school );

  // For Protection and Retribution, SP is fixed to AP by passives
  switch ( specialization() )
  {
  case PALADIN_PROTECTION:
    sp = spec.protection_paladin -> effectN( 8 ).percent() * composite_melee_attack_power( attack_power_type::WEAPON_MAINHAND ) * composite_attack_power_multiplier();
    break;
  case PALADIN_RETRIBUTION:
    sp = spec.retribution_paladin -> effectN( 10 ).percent() * composite_melee_attack_power( attack_power_type::WEAPON_MAINHAND ) * composite_attack_power_multiplier();
    break;
  default:
    break;
  }
  return sp;
}

// paladin_t::composite_melee_attack_power ==================================

double paladin_t::composite_melee_attack_power() const
{
  if ( specialization() == PALADIN_HOLY )
  {
    return composite_spell_power( SCHOOL_MAX ) * spec.holy_paladin -> effectN( 9 ).percent();
  }

  return player_t::composite_melee_attack_power();
}

double paladin_t::composite_melee_attack_power( attack_power_type ap_type) const
{
  return player_t::composite_melee_attack_power( ap_type );
}

// paladin_t::composite_attack_power_multiplier =============================

double paladin_t::composite_attack_power_multiplier() const
{
  double ap = player_t::composite_attack_power_multiplier();

  // Mastery bonus is multiplicative with other effects
  if ( specialization() == PALADIN_PROTECTION )
    ap *= 1.0 + cache.mastery() * mastery.divine_bulwark -> effectN( 3 ).mastery_value();

  return ap;
}

// paladin_t::composite_spell_power_multiplier ==============================

double paladin_t::composite_spell_power_multiplier() const
{
  if ( specialization() == PALADIN_RETRIBUTION || specialization() == PALADIN_PROTECTION )
    return 1.0;

  return player_t::composite_spell_power_multiplier();
}

// paladin_t::composite_block ==========================================

double paladin_t::composite_block() const
{
  // this handles base block and and all block subject to diminishing returns
  double block_subject_to_dr = cache.mastery() * mastery.divine_bulwark -> effectN( 1 ).mastery_value();
  double b = player_t::composite_block_dr( block_subject_to_dr );

  b += talents.holy_shield -> effectN( 1 ).percent();

  return b;
}

// paladin_t::composite_block_reduction =======================================

double paladin_t::composite_block_reduction( action_state_t* s ) const
{
  double br = player_t::composite_block_reduction( s );

  br += buffs.inner_light -> value();

  if ( buffs.redoubt -> up() )
  {
    br *= 1.0 + talents.redoubt -> effectN( 1 ).trigger() -> effectN( 1 ).percent();
  }

  return br;
}

// paladin_t::composite_crit_avoidance ========================================

double paladin_t::composite_crit_avoidance() const
{
  double c = player_t::composite_crit_avoidance();

  c += spec.protection_paladin -> effectN( 9 ).percent();

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

  if ( ( specialization() == PALADIN_RETRIBUTION || specialization() == PALADIN_PROTECTION ) &&
       ( c == CACHE_STRENGTH || c == CACHE_ATTACK_POWER ) )
  {
    player_t::invalidate_cache( CACHE_SPELL_POWER );
  }

  if ( specialization() == PALADIN_HOLY && ( c == CACHE_INTELLECT || c == CACHE_SPELL_POWER ) )
  {
    player_t::invalidate_cache( CACHE_ATTACK_POWER );
  }

  if ( c == CACHE_ATTACK_CRIT_CHANCE && passives.riposte -> ok() )
    player_t::invalidate_cache( CACHE_PARRY );

  if ( c == CACHE_MASTERY && mastery.divine_bulwark -> ok() )
  {
    player_t::invalidate_cache( CACHE_BLOCK );
    player_t::invalidate_cache( CACHE_ATTACK_POWER );
    player_t::invalidate_cache( CACHE_SPELL_POWER );
  }

  if ( c == CACHE_STRENGTH && spec.shield_of_the_righteous -> ok() )
  {
    player_t::invalidate_cache( CACHE_BONUS_ARMOR );
  }
}

// paladin_t::matching_gear_multiplier ======================================

double paladin_t::matching_gear_multiplier( attribute_e attr ) const
{
  double mult = passives.plate_specialization -> effectN( 1 ).percent();

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

// paladin_t::resource_gain =================================================

double paladin_t::resource_gain( resource_e resource_type, double amount, gain_t* source, action_t* action )
{
  if ( resource_type == RESOURCE_HOLY_POWER )
  {
    if ( specialization() == PALADIN_RETRIBUTION )
    {
      if ( player_t::buffs.memory_of_lucid_dreams -> up() )
      {
        amount *= 1.0 + player_t::buffs.memory_of_lucid_dreams -> data().effectN( 1 ).percent();
      }

    }

    if ( buffs.holy_avenger -> up() )
    {
      amount *= 1.0 + buffs.holy_avenger -> data().effectN( 2 ).percent();
    }
  }

  return player_t::resource_gain( resource_type, amount, source, action );
}

// paladin_t::assess_damage =================================================

void paladin_t::assess_damage( school_e school,
                               result_amount_type    dtype,
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

  if ( buffs.inner_light -> up() && !s -> action -> special && cooldowns.inner_light_icd -> up() )
  {
    active.inner_light_damage -> set_target( s -> action -> player );
    active.inner_light_damage -> schedule_execute();
    cooldowns.inner_light_icd -> start();
  }

  // Trigger Grand Crusader on an avoidance event (TODO: test if it triggers on misses)
  if ( s -> result == RESULT_DODGE || s -> result == RESULT_PARRY || s -> result == RESULT_MISS )
  {
    trigger_grand_crusader();
  }

  // Holy Shield's magic block
  if ( talents.holy_shield -> ok() && school != SCHOOL_PHYSICAL && !s -> action -> may_block )
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
        // 2019-03-19: Holy Shield might not be 40% damage reduction. TODO: Investigate
        double block_amount = s -> result_amount * 0.4;

        sim -> print_debug( "{} Holy Shield absorbs {}", name(), block_amount );

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
        sim -> print_debug( "{} Holy Shield fails to activate", name() );
      }
    }

    sim -> print_debug( "Damage to {} after Holy Shield mitigation is {}", name(), s -> result_amount );
  }

  player_t::assess_damage( school, dtype, s );
}

// paladin_t::create_options ================================================

void paladin_t::create_options()
{
  // TODO: figure out a better solution for this.
  add_option( opt_bool( "paladin_fake_sov", options.fake_sov ) );
  add_option( opt_int( "indomitable_justice_pct", options.indomitable_justice_pct ) );
  add_option( opt_float( "proc_chance_ret_memory_of_lucid_dreams", options.proc_chance_ret_memory_of_lucid_dreams, 0.0, 1.0 ) );
  add_option( opt_float( "proc_chance_prot_memory_of_lucid_dreams", options.proc_chance_prot_memory_of_lucid_dreams, 0.0, 1.0 ) );

  player_t::create_options();
}

// paladin_t::copy_from =====================================================

void paladin_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  options = debug_cast<paladin_t*>( source ) -> options;
}

// paladin_t::combat_begin ==================================================

void paladin_t::combat_begin()
{
  player_t::combat_begin();

  auto hp_overflow = resources.current[ RESOURCE_HOLY_POWER ] - MAX_START_OF_COMBAT_HOLY_POWER;

  if ( hp_overflow > 0 )
  {
    resource_loss( RESOURCE_HOLY_POWER, hp_overflow );
  }

  lucid_dreams_accumulator = 0;
}

// paladin_t::get_how_availability ==========================================

bool paladin_t::get_how_availability( player_t* t ) const
{
  // Health threshold has to be hardcoded :peepocri:
  return ( buffs.avenging_wrath -> up() || buffs.crusade -> up() || t -> health_percentage() <= 20 );
}

// player_t::create_expression ==============================================

std::unique_ptr<expr_t> paladin_t::create_consecration_expression( util::string_view expr_str )
{
  auto expr = util::string_split<util::string_view>( expr_str, "." );
  if ( expr.size() != 2 )
  {
    return nullptr;
  }

  if ( ! util::str_compare_ci( expr[ 0u ], "consecration" ) )
  {
    return nullptr;
  }

  if ( util::str_compare_ci( expr[ 1u ], "ticking" ) ||
       util::str_compare_ci( expr[ 1u ], "up" ) )
  {
    return make_fn_expr( "consecration_ticking", [ this ]() { return active_consecration == nullptr ? 0 : 1; } );
  }
  else if ( util::str_compare_ci( expr[ 1u ], "remains" ) )
  {
    return make_fn_expr( "consecration_remains", [ this ]() {
      return active_consecration == nullptr ? 0 : active_consecration -> remaining_time().total_seconds();
    } );
  }

  return nullptr;
}

std::unique_ptr<expr_t> paladin_t::create_expression( util::string_view name_str )
{
  struct paladin_expr_t : public expr_t
  {
    paladin_t& paladin;
    paladin_expr_t( util::string_view n, paladin_t& p ) :
      expr_t( n ), paladin( p ) {}
  };


  auto splits = util::string_split<util::string_view>( name_str, "." );

  struct time_to_hpg_expr_t : public paladin_expr_t
  {
    cooldown_t* cs_cd;
    cooldown_t* boj_cd;
    cooldown_t* j_cd;
    cooldown_t* how_cd;
    cooldown_t* wake_cd;

    time_to_hpg_expr_t( util::string_view n, paladin_t& p ) :
      paladin_expr_t( n, p ), cs_cd( p.get_cooldown( "crusader_strike" ) ),
      boj_cd ( p.get_cooldown( "blade_of_justice" )),
      j_cd( p.get_cooldown( "judgment" ) ),
      how_cd( p.get_cooldown( "hammer_of_wrath" ) ),
      wake_cd( p.get_cooldown( "wake_of_ashes" ) )
    { }

    double evaluate() override
    {
      timespan_t gcd_ready = paladin.gcd_ready - paladin.sim -> current_time();
      gcd_ready = std::max( gcd_ready, 0_ms );

      timespan_t shortest_hpg_time = cs_cd -> remains();

      if ( boj_cd -> remains() < shortest_hpg_time )
        shortest_hpg_time = boj_cd -> remains();

      // TODO: check every target rather than just the paladin's main target
      if ( paladin.get_how_availability( paladin.target ) && how_cd -> remains() < shortest_hpg_time )
        shortest_hpg_time = how_cd -> remains();

      if ( wake_cd -> remains() < shortest_hpg_time )
        shortest_hpg_time = wake_cd -> remains();

      if ( gcd_ready > shortest_hpg_time )
        return gcd_ready.total_seconds();
      else
        return shortest_hpg_time.total_seconds();
    }
  };

  if ( splits[ 0 ] == "time_to_hpg" )
  {
    return std::make_unique<time_to_hpg_expr_t>( name_str, *this );
  }

  auto cons_expr = create_consecration_expression( name_str );
  if ( cons_expr )
  {
    return cons_expr;
  }

  return player_t::create_expression( name_str );
}

void paladin_t::merge( player_t& other )
{
  player_t::merge( other );

  const paladin_t& op = static_cast<paladin_t&>( other );

  for ( size_t i = 0; i < cooldown_waste_data_list.size(); i++ )
  {
    cooldown_waste_data_list[ i ] -> merge( *op.cooldown_waste_data_list[ i ] );
  }
}

void paladin_t::analyze( sim_t& s )
{
  player_t::analyze( s );
  for ( auto cdw : cooldown_waste_data_list )
  {
    cdw -> analyze();
  }
}

void paladin_t::datacollection_begin()
{
  player_t::datacollection_begin();
  for ( auto cdw : cooldown_waste_data_list )
  {
    cdw -> datacollection_begin();
  }
}

void paladin_t::datacollection_end()
{
  player_t::datacollection_end();
  for ( auto cdw : cooldown_waste_data_list )
  {
    cdw -> datacollection_end();
  }
}

void paladin_t::apply_affecting_auras( action_t& action )
{
  player_t::apply_affecting_auras(action);

  action.apply_affecting_aura( spec.retribution_paladin );
  action.apply_affecting_aura( spec.holy_paladin );
  action.apply_affecting_aura( spec.protection_paladin );
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class paladin_report_t : public player_report_extension_t
{
public:
  paladin_report_t( paladin_t& player ) :
    p( player )
  { }

  void cdwaste_table_header( report::sc_html_stream& os )
  {
    os << "<table class=\"sc\" style=\"float: left;margin-right: 10px;\">\n"
         << "<tr>\n"
           << "<th></th>\n"
           << "<th colspan=\"3\">Seconds per Execute</th>\n"
           << "<th colspan=\"3\">Seconds per Iteration</th>\n"
         << "</tr>\n"
         << "<tr>\n"
           << "<th>Ability</th>\n"
           << "<th>Average</th>\n"
           << "<th>Minimum</th>\n"
           << "<th>Maximum</th>\n"
           << "<th>Average</th>\n"
           << "<th>Minimum</th>\n"
           << "<th>Maximum</th>\n"
         << "</tr>\n";
  }

  void cdwaste_table_footer( report::sc_html_stream& os )
  {
    os << "</table>\n";
  }

  void cdwaste_table_contents( report::sc_html_stream& os )
  {
    size_t row = 0;
    for ( const cooldown_waste_data_t* data : p.cooldown_waste_data_list )
    {
      if ( ! data -> active() ) continue;

      std::string name = data -> cd -> name_str;
      if ( action_t* a = p.find_action( name ) )
        name = report_decorators::decorated_action(*a);
      else
        name = util::encode_html( name );

      std::string row_class;
      if ( ++row & 1 ) row_class = " class=\"odd\"";
      os.printf( "<tr%s>", row_class.c_str() );
      os << "<td class=\"left\">" << name << "</td>";
      os.printf( "<td class=\"right\">%.3f</td>", data -> normal.mean() );
      os.printf( "<td class=\"right\">%.3f</td>", data -> normal.min() );
      os.printf( "<td class=\"right\">%.3f</td>", data -> normal.max() );
      os.printf( "<td class=\"right\">%.3f</td>", data -> cumulative.mean() );
      os.printf( "<td class=\"right\">%.3f</td>", data -> cumulative.min() );
      os.printf( "<td class=\"right\">%.3f</td>", data -> cumulative.max() );
      os << "</tr>\n";
    }
  }

  void html_customsection( report::sc_html_stream& os ) override
  {
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n";
    if ( p.cooldown_waste_data_list.size() > 0 )
    {
      os << "\t\t\t\t\t<h3 class=\"toggle open\">Cooldown waste details</h3>\n"
         << "\t\t\t\t\t<div class=\"toggle-content\">\n";

      cdwaste_table_header( os );
      cdwaste_table_contents( os );
      cdwaste_table_footer( os );

      os << "\t\t\t\t\t</div>\n";

      os << "<div class=\"clear\"></div>\n";
    }

    os << "\t\t\t\t\t</div>\n";
  }
private:
  paladin_t& p;
};

// PALADIN MODULE INTERFACE =================================================

struct paladin_module_t : public module_t
{
  paladin_module_t() : module_t( PALADIN ) {}

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    auto  p = new paladin_t( sim, name, r );
    p -> report_extension = std::unique_ptr<player_report_extension_t>( new paladin_report_t( *p ) );
    return p;
  }

  bool valid() const override { return true; }

  void static_init() const override
  {
    unique_gear::register_special_effect( 286390, empyrean_power );
  }

  void init( player_t* p ) const override
  {
    p -> buffs.beacon_of_light          = make_buff( p, "beacon_of_light", p -> find_spell( 53563 ) );
    p -> buffs.blessing_of_sacrifice    = new buffs::blessing_of_sacrifice_t( p );
    p -> debuffs.forbearance            = new buffs::forbearance_t( p, "forbearance" );
  }

  void register_hotfixes() const override
  {
  }

  void combat_begin( sim_t* ) const override {}

  void combat_end  ( sim_t* ) const override {}
};

} // end namespace paladin

const module_t* module_t::paladin()
{
  static paladin::paladin_module_t m;
  return &m;
}
