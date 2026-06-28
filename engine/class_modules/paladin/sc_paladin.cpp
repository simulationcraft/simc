// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
/*
  TODO: reimplement Holy if anyone ever becomes interested in maintaining it
*/
#include "sc_paladin.hpp"

#include "simulationcraft.hpp"
#include "action/dbc_proc_callback.hpp"
#include "action/parse_effects.hpp"
#include "item/special_effect.hpp"
#include <algorithm>

// ==========================================================================
// Paladin
// ==========================================================================
namespace paladin
{
paladin_t::paladin_t( sim_t* sim, util::string_view name, race_e r )
  : player_t( sim, PALADIN, name, r ),
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
    next_armament( SACRED_WEAPON ),
    random_weapon_target( nullptr ),
    random_bulwark_target( nullptr ),
    divine_inspiration_next( -1 ),
    reflection_of_radiance_proc_chance( .05 ) // ToDo Fluttershy: Find out real proc chance - Please Blizz bring back the log event for Grand Crusader
{
  active_consecration = nullptr;
  active_boj_cons = nullptr;
  all_active_consecrations.clear();
  active_aura         = nullptr;

  cooldowns.blessing_of_protection       = get_cooldown( "blessing_of_protection" );
  cooldowns.blessing_of_spellwarding     = get_cooldown( "blessing_of_spellwarding" );
  cooldowns.lay_on_hands                 = get_cooldown( "lay_on_hands" );

  cooldowns.holy_shock    = get_cooldown( "holy_shock" );
  cooldowns.light_of_dawn = get_cooldown( "light_of_dawn" );

  cooldowns.avengers_shield                   = get_cooldown( "avengers_shield" );
  cooldowns.consecration                      = get_cooldown( "consecration" );
  cooldowns.guardian_of_ancient_kings         = get_cooldown( "guardian_of_ancient_kings" );

  cooldowns.blade_of_justice = get_cooldown( "blade_of_justice" );
  cooldowns.hammer_of_wrath  = get_cooldown( "hammer_of_wrath" );
  cooldowns.wake_of_ashes    = get_cooldown( "wake_of_ashes" );
  cooldowns.divine_toll      = get_cooldown( "divine_toll" );

  cooldowns.holy_armaments           = get_cooldown( "holy_armaments" );

  cooldowns.ret_aura_icd = get_cooldown( "ret_aura_icd" );
  cooldowns.ret_aura_icd->duration = timespan_t::from_seconds( 30 );

  cooldowns.consecrated_blade_icd = get_cooldown( "consecrated_blade_icd" );
  cooldowns.consecrated_blade_icd->duration = timespan_t::from_seconds( 10 );

  cooldowns.hammerfall_icd           = get_cooldown( "hammerfall_icd" );
  cooldowns.hammerfall_icd->duration = find_spell( 432463 )->internal_cooldown();

  cooldowns.aurora_icd = get_cooldown( "aurora_icd" );
  cooldowns.aurora_icd->duration = find_spell( 439760 )->internal_cooldown();

  cooldowns.second_sunrise_icd = get_cooldown( "second_sunrise_icd" );
  cooldowns.second_sunrise_icd->duration = find_spell( 431474 )->internal_cooldown();

  cooldowns.walk_into_light_icd = get_cooldown( "walk_into_light_icd" );
  cooldowns.walk_into_light_icd->duration = find_spell( 1263782 )->internal_cooldown();

  cooldowns.art_of_war = get_cooldown( "art_of_war" );
  cooldowns.art_of_war->duration = find_spell( 406064 )->internal_cooldown();

  cooldowns.righteous_cause_icd = get_cooldown( "righteous_cause_icd" );
  cooldowns.righteous_cause_icd->duration = find_spell( 402912 )->internal_cooldown();

  beacon_target         = nullptr;
  resource_regeneration = regen_type::DYNAMIC;
  fake_lesser_weapon_set.clear();
}

const paladin_td_t* paladin_t::find_target_data( const player_t* target ) const
{
  return target_data[ target ];
}

paladin_td_t* paladin_t::get_target_data( player_t* target ) const
{
  paladin_td_t*& td = target_data[ target ];
  if ( !td )
  {
    td = new paladin_td_t( target, const_cast<paladin_t*>( this ) );
  }
  return td;
}

template <typename T>
static std::function<int( actor_target_data_t* )> d_fn( T d, bool stack = true )
{
  if constexpr ( std::is_invocable_v<T, paladin_td_t::buffs_t> )
  {
    if ( stack )
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast<paladin_td_t*>( t )->debuff )->check();
      };
    else
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast<paladin_td_t*>( t )->debuff )->check() > 0;
      };
  }
  else if constexpr ( std::is_invocable_v<T, paladin_td_t::dots_t> )
  {
    if ( stack )
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast<paladin_td_t*>( t )->dot )->current_stack();
      };
    else
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast<paladin_td_t*>( t )->dot )->is_ticking();
      };
  }
  else
  {
    static_assert( static_false<T>, "Not a valid member of paladin_td_t" );
    return nullptr;
  }
}

// ==========================================================================
// Paladin Buffs, Part One
// ==========================================================================
// Paladin buffs are split up into two sections. This one is for ones that
// need to be defined before action_t definitions, because those action_t
// definitions call methods of these buffs. Generic buffs that can be defined
// anywhere are also put here. There's a second buffs section near the end
// containing ones that require action_t definitions to function properly.
namespace buffs
{

struct shield_of_vengeance_buff_t : public absorb_buff_t
{
  double max_absorb;
  shield_of_vengeance_buff_t( player_t* p )
    : absorb_buff_t( p, "shield_of_vengeance",
                     p->find_spell( 184662 ) ), max_absorb(0.0)
  {
    cooldown->duration = 0_ms;
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    bool res = absorb_buff_t::trigger( stacks, value, chance, duration );
    if ( res )
      max_absorb = value;
    return res;
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    absorb_buff_t::expire_override( expiration_stacks, remaining_duration );

    auto* p = static_cast<paladin_t*>( player );
    // TODO(mserrano): This is a horrible hack
    p->active.shield_of_vengeance_damage->base_dd_max = p->active.shield_of_vengeance_damage->base_dd_min =
        p->options.fake_sov ? max_absorb : current_value;
    p->active.shield_of_vengeance_damage->execute();
    max_absorb = 0.0;
  }
};

struct sacrosanct_crusade_t :public absorb_buff_t
{
  sacrosanct_crusade_t(player_t* p) : absorb_buff_t(p, "sacrosanct_crusade", p->find_spell(461867))
  {
    set_absorb_source( p->get_stats( "sacrosanct_crusade_absorb" ) );
  }
  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    auto* p        = static_cast<paladin_t*>( player );
    int max_hp_effect = p->specialization() == PALADIN_RETRIBUTION ? 4 : 1;
    double shield = p->talents.templar.sacrosanct_crusade->effectN( max_hp_effect ).percent() *
                    p->resources.max[ RESOURCE_HEALTH ] * ( 1.0 + p->composite_heal_versatility() );

    double current_shield = p->buffs.templar.sacrosanct_crusade->value();

    if (value > 0) // Hammer of Light overhealing
    {
      current_shield += value;
    }
    else
    {
      current_shield += shield;
    }

    bool ret_value = absorb_buff_t::trigger( stacks, current_shield, chance, duration );
    return ret_value;
  }
};
}  // namespace buffs

// end namespace buffs
// ==========================================================================
// End Paladin Buffs, Part One
// ==========================================================================

// Blessing of Protection =====================================================

struct blessing_of_protection_t : public paladin_spell_t
{
  blessing_of_protection_t( paladin_t* p, util::string_view options_str )
    : paladin_spell_t( "blessing_of_protection", p, p->find_talent_spell( talent_tree::CLASS, "Blessing of Protection" ) )
  {
    parse_options( options_str );
    harmful = false;
    may_miss = false;
    cooldown = p->cooldowns.blessing_of_protection; // Used to set it on cooldown via Blessing of Spellwarding
  }

  void execute() override
  {
    paladin_spell_t::execute();

    // TODO: Check if target is self, because it's castable on anyone
    p()->buffs.blessing_of_protection->trigger();

    if ( p()->talents.blessing_of_spellwarding->ok() )
      p()->cooldowns.blessing_of_spellwarding->start();
    // apply forbearance, track locally for forbearant faithful & force recharge recalculation
    p()->trigger_forbearance( execute_state->target );
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( candidate_target->debuffs.forbearance->check() )
      return false;

    if ( candidate_target->is_enemy() )
      return false;

    return paladin_spell_t::target_ready( candidate_target );
  }
};

// Avenging Wrath ===========================================================
// Most of this can be found in buffs::avenging_wrath_buff_t, this spell just triggers the buff

struct avenging_wrath_state_t : public action_state_t
{
  using action_state_t::action_state_t;

  proc_types2 cast_proc_type2() const override
  {
    // This spell can trigger on-cast procs even if it is backgrounded
    return PROC2_CAST_GENERIC;
  }
};

avenging_wrath_t::avenging_wrath_t( paladin_t* p )
  : paladin_spell_t( "avenging_wrath", p, p->find_spell( 454351 ) )
{
  background = true;
  harmful = false;
}

avenging_wrath_t::avenging_wrath_t( paladin_t* p, util::string_view options_str )
  : paladin_spell_t( "avenging_wrath", p, p->find_spell( 31884 ) )
{
  parse_options( options_str );
  if ( !p->talents.avenging_wrath->ok() )
    background = true;
  if ( p->talents.avenging_crusader->ok() )
    background = true;
  if ( p->talents.sentinel->ok() )
    background = true;
  if ( p->talents.radiant_glory->ok() )
    background = true;

  harmful = false;
}

action_state_t* avenging_wrath_t::new_state()
{
  return new avenging_wrath_state_t( this, target );
}

void avenging_wrath_t::execute()
{
  paladin_spell_t::execute();

  p()->buffs.avenging_wrath->trigger();
  if ( p()->talents.lightsmith.blessing_of_the_forge->ok() )
    p()->buffs.lightsmith.blessing_of_the_forge->execute();

  if ( p()->talents.empyrean_legacy->ok() )
    p()->buffs.empyrean_legacy->trigger();

  if ( p()->talents.hammer_of_wrath->ok() )
    p()->buffs.hammer_of_wrath->trigger();

  if ( p()->talents.herald_of_the_sun.walk_into_light->ok() &&
       p()->rng().roll( p()->talents.herald_of_the_sun.walk_into_light->effectN( 1 ).percent() ) )
  {
    p()->resource_gain( RESOURCE_HOLY_POWER,
                        as<int>( p()->talents.herald_of_the_sun.walk_into_light->effectN( 2 ).base_value() ),
                        p()->gains.hp_walk_into_light );
    p()->buffs.herald_of_the_sun.blessing_of_anshe->trigger();
  }

  if ( p()->talents.herald_of_the_sun.born_in_sunlight->ok() )
    p()->buffs.herald_of_the_sun.born_in_sunlight->trigger();
}

// Consecration =============================================================

golden_path_t::golden_path_t( paladin_t* p ) : paladin_heal_t( "golden_path", p, p->find_spell( 339119 ) )
{
  background = true;
}
  consecration_tick_t::consecration_tick_t( util::string_view name, paladin_t* p )
    : paladin_spell_t( name, p, p->find_spell( 81297 ) ),
      heal_tick( new golden_path_t( p ) )
  {
    aoe         = -1;
    dual        = true;
    direct_tick = true;
    background  = true;
    may_crit    = true;
    ground_aoe  = true;
  }

  double consecration_tick_t::action_multiplier() const
  {
    double m = paladin_spell_t::action_multiplier();
    if ( p()->talents.vision_of_sanctity->ok() )
    {
      if ( paladin_spell_t::num_targets() == 1 )
        m *= 1.0 + p()->talents.vision_of_sanctity->effectN( 1 ).percent();
    }
    return m;
  }

  double consecration_tick_t::composite_target_multiplier( player_t* target ) const
  {
    double m = paladin_spell_t::composite_target_multiplier( target );

    paladin_td_t* td = p()->get_target_data( target );

    if ( p()->talents.burn_to_ash->ok() && td->dot.truths_wake->is_ticking() )
    {
      m *= 1.0 + p()->talents.burn_to_ash->effectN( 2 ).percent();
      if ( p()->bugs )
        m *= 1.0 + p()->talents.burn_to_ash->effectN( 2 ).percent();
    }

    return m;
  }

struct divine_guidance_damage_t : public paladin_spell_t
{
  divine_guidance_damage_t( util::string_view n, paladin_t* p ) : paladin_spell_t( n, p, p->find_spell( 433808 ) )
  {
    proc = may_crit         = true;
    may_miss                = false;
    attack_power_mod.direct = p->talents.lightsmith.divine_guidance->effectN( 1 ).base_value() / 10.0;
    aoe                     = -1;
    split_aoe_damage        = true;
  }

  double action_multiplier() const override
  {
    double m = paladin_spell_t::action_multiplier();
    m *= p()->buffs.lightsmith.divine_guidance->stack();
    return m;
  }
};

struct divine_guidance_heal_t : public paladin_heal_t
{
  divine_guidance_heal_t( util::string_view n, paladin_t* p ) : paladin_heal_t( n, p, p->find_spell( 433807 ) )
  {
    proc = may_crit         = true;
    may_miss                = false;
    attack_power_mod.direct = p->talents.lightsmith.divine_guidance->effectN( 1 ).base_value() / 10.0;
    aoe                     = 1;
  }

  double action_multiplier() const override
  {
    double m = paladin_heal_t::action_multiplier();
    m *= p()->buffs.lightsmith.divine_guidance->stack();
    return m;
  }
};

struct consecration_t : public paladin_spell_t
{
  consecration_tick_t* damage_tick;
  ground_aoe_params_t cons_params;
  consecration_source source_type;

  divine_guidance_damage_t* dg_damage;
  divine_guidance_heal_t* dg_heal;

  double precombat_time;

  consecration_t( paladin_t* p, util::string_view options_str )
    : paladin_spell_t( "consecration", p, p->find_spell( 26573 ) ),
      damage_tick( new consecration_tick_t( "consecration_tick", p ) ),
      source_type( HARDCAST ),
      dg_damage( nullptr ),
      dg_heal( nullptr ),
      precombat_time( 2.0 )
  {
    add_option( opt_float( "precombat_time", precombat_time ) );
    parse_options( options_str );

    dot_duration = 0_ms;  // the periodic event is handled by ground_aoe_event_t
    may_miss = harmful = false;
    aoe                = -1;

    // technically this doesn't work for characters under level 11?
    if ( p->specialization() == PALADIN_RETRIBUTION )
      background = true;

    add_child( damage_tick );
    if ( p->talents.lightsmith.divine_guidance->ok() )
    {
      dg_damage = new divine_guidance_damage_t( "divine_guidance", p );
      dg_heal   = new divine_guidance_heal_t( "divine_guidance_heal", p );
      add_child( dg_damage );
      // Maybe later: Heal?
    }
  }

  consecration_t( paladin_t* p, util::string_view source_name, consecration_source source )
    : paladin_spell_t( "consecration" + std::string( source_name ), p, p->find_spell( 26573 ) ),
      damage_tick( new consecration_tick_t( "consecration_tick_" + std::string( source_name ), p ) ),
      source_type( source ),
      dg_damage( nullptr ),
      dg_heal( nullptr ),
      precombat_time( 0.0 )
  {
    dot_duration = 0_ms;  // the periodic event is handled by ground_aoe_event_t
    may_miss = harmful = false;
    background = true;
    cooldown->duration = 0_ms;
    aoe                = -1;

    add_child( damage_tick );
  }

  void init_finished() override
  {
    paladin_spell_t::init_finished();

    if ( action_list && action_list->name_str == "precombat" )
    {
      double MIN_TIME = player->base_gcd.total_seconds();  // the player's base unhasted gcd: 1.5s
      double MAX_TIME = cooldown->duration.total_seconds() - 1;

      // Ensure that we're using a positive value
      if ( precombat_time < 0 )
        precombat_time = -precombat_time;

      if ( precombat_time > MAX_TIME )
      {
        precombat_time = MAX_TIME;
        sim->error(
            "{} tried to use consecration in precombat more than {} seconds before combat begins, setting to {}",
            player->name(), MAX_TIME, MAX_TIME );
      }
      else if ( precombat_time < MIN_TIME )
      {
        precombat_time = MIN_TIME;
        sim->error(
            "{} tried to use consecration in precombat less than {} before combat begins, setting to {} (has to be >= "
            "base gcd)",
            player->name(), MIN_TIME, MIN_TIME );
      }
    }
    else
      precombat_time = 0;

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
                          case ground_aoe_params_t::EVENT_STOPPED:
                            break;
                          case ground_aoe_params_t::EVENT_CREATED:
                            if ( source_type == HARDCAST )
                            {
                              p()->active_consecration = event;
                            }
                            else if ( source_type == BLADE_OF_JUSTICE )
                            {
                              p()->active_boj_cons = event;
                            }
                            p()->all_active_consecrations.insert( event );
                            break;
                          case ground_aoe_params_t::EVENT_DESTRUCTED:
                            if ( source_type == HARDCAST && p()->active_consecration != nullptr )
                            {
                              p()->active_consecration = nullptr;
                            }
                            else if ( source_type == BLADE_OF_JUSTICE )
                            {
                              p()->active_boj_cons = nullptr;
                            }
                            p()->all_active_consecrations.erase( event );
                            break;
                          default:
                            break;
                        }
                      } );
  }

  void execute() override
  {
    // If this is an active Cons, cancel the current consecration if it exists
    if ( source_type == HARDCAST && p()->active_consecration != nullptr )
    {
      p()->all_active_consecrations.erase( p()->active_consecration );
      event_t::cancel( p()->active_consecration );
    }
    // or if it's a boj-triggered Cons, cancel the previous BoJ-triggered cons
    else if ( source_type == BLADE_OF_JUSTICE && p()->active_boj_cons != nullptr )
    {
      p()->all_active_consecrations.erase( p()->active_boj_cons );
      event_t::cancel( p()->active_boj_cons );
    }

    /*
      Divine Guidance seems to function as follows:
      Try to heal as many injured people (and pets!) as possible inside your Consecration, up to 5 (max_dg_heal_targets)
      If you cannot heal 5 (max_dg_heal_targets) targets, deal the rest in damage to other targets inside the Consecration
      Damage and Healing is divided by target count, up to 5 (max_dg_heal_targets), damage is then further divided by amount of mobs hit
    */
    // Divine Guidance seems to prioritise Healing, so count healing targets first
    std::vector<player_t*> healingAllies;
    int totalTargets = 0;
    if ( dg_damage && dg_heal && p()->buffs.lightsmith.divine_guidance->up() )
    {
      for (auto friendly : sim->player_non_sleeping_list)
      {
        if ( friendly == p() )  // Always heal ourselves to avoid oversim
          healingAllies.push_back( friendly );
        else if ( friendly->health_percentage() < 100 ) // Allies are only healed when they're not full HP
          healingAllies.push_back( friendly );
        if ( healingAllies.size() == as<size_t>( p()->options.max_dg_heal_targets ) )
          break;
      }
      // If we hit less than 5 (max_dg_heal_targets) healing targets, we can fill the rest with damage targets
      int healingAlliesSize = as<int>( healingAllies.size() );

      if ( healingAlliesSize > p()->options.min_dg_heal_targets )
        healingAlliesSize = p()->options.min_dg_heal_targets;

      totalTargets          = healingAlliesSize;

      if ( healingAlliesSize < 5 )
      {
        totalTargets = as<int>( sim->target_non_sleeping_list.size() ) + healingAlliesSize;
      }

      if ( healingAlliesSize > 0 )
      {
        dg_heal->base_dd_multiplier = 1.0 / totalTargets;
        // Healing events come before Consecration cast
        for ( auto friendly : healingAllies )
        {
          dg_heal->set_target( friendly );
          dg_heal->execute();
        }
      }
      dg_damage->base_dd_multiplier =
          ( as<double>( totalTargets - healingAlliesSize ) / totalTargets );
    }

    paladin_spell_t::execute();

    // Damage events come after Consecration cast
    if ( dg_damage && p()->buffs.lightsmith.divine_guidance->up() )
    {
      // Only create damage events when we're dealing damage, so not to proc stuff accidentally
      if ( dg_damage->base_dd_multiplier > 0 )
      {
        dg_damage->set_target( target );
        dg_damage->execute();
      }
      p()->buffs.lightsmith.divine_guidance->expire();
    }

    // Some parameters must be updated on each cast
    cons_params.target( execute_state->target ).start_time( sim->current_time() );

    if ( sim->distance_targeting_enabled )
      cons_params.x( p()->x_position ).y( p()->y_position );

    if ( !player->in_combat && precombat_time > 0 )
    {
      // Adjust cooldown if consecration is used in precombat
      p()->cooldowns.consecration->adjust( timespan_t::from_seconds( -precombat_time ), false );

      // Create an event that starts consecration's aoe one second after combat starts
      make_event( *sim, 1_s, [ this ]() {
        make_event<ground_aoe_event_t>( *sim, p(), cons_params, true /* Immediate pulse */ );
      } );

      sim->print_debug(
          "{} used Consecration in precombat with precombat time = {}, adjusting duration and remaining cooldown.",
          player->name(), precombat_time );
    }

    else
    {
      make_event<ground_aoe_event_t>( *sim, p(), cons_params, true /* Immediate pulse */ );
      damage_tick->execute();
    }
  }
  void impact(action_state_t* s) override
  {
    paladin_spell_t::impact( s );
    p()->get_target_data( s->target )->debuff.consecration->execute();
  }
};

// Divine Shield ==============================================================

struct divine_shield_t : public paladin_spell_t
{
  divine_shield_t( paladin_t* p, util::string_view options_str )
    : paladin_spell_t( "divine_shield", p, p->find_class_spell( "Divine Shield" ) )
  {
    parse_options( options_str );

    harmful = false;
  }

  void execute() override
  {
    paladin_spell_t::execute();

    // Technically this should also drop you from the mob's threat table,
    // but we'll assume the MT isn't using it for now
    p()->buffs.divine_shield->trigger();

    // in this sim, the only debuffs we care about are enemy DoTs.
    // Check for them and remove them when cast
    for ( size_t i = 0, size = p()->dot_list.size(); i < size; i++ )
    {
      dot_t* d = p()->dot_list[ i ];

      if ( d->source != p() && d->source->is_enemy() && d->is_ticking() )
      {
        d->cancel();
      }
    }

    // trigger forbearance
    p()->trigger_forbearance( player );
  }

  bool ready() override
  {
    if ( p()->debuffs.forbearance->check() || !( p()->talents.lights_revocation->ok() ) )
      return false;

    return paladin_spell_t::ready();
  }
};

// Divine Steed (Protection, Retribution) =====================================

struct divine_steed_t : public paladin_spell_t
{
  divine_steed_t( paladin_t* p, util::string_view options_str )
    : paladin_spell_t( "divine_steed", p, p->find_class_spell( "Divine Steed" ) )
  {
    parse_options( options_str );
    background  = false;
  }

  void execute() override
  {
    paladin_spell_t::execute();

    p()->buffs.divine_steed->trigger();
  }
};

// Flash of Light Spell =====================================================

struct flash_of_light_t : public paladin_heal_t
{
  flash_of_light_t( paladin_t* p, util::string_view options_str )
    : paladin_heal_t( "flash_of_light", p, p->find_class_spell( "Flash of Light" ) )
  {
    parse_options( options_str );
  }
};

// Blessing of Sacrifice ========================================================

struct blessing_of_sacrifice_redirect_t : public paladin_spell_t
{
  blessing_of_sacrifice_redirect_t( paladin_t* p )
    : paladin_spell_t( "blessing_of_sacrifice_redirect", p, p->find_class_spell( "Blessing of Sacrifice" ) )
  {
    background      = true;
    trigger_gcd     = 0_ms;
    may_crit        = false;
    may_miss        = false;
    base_multiplier = data().effectN( 1 ).percent();
    target          = p;
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
  blessing_of_sacrifice_t( paladin_t* p, util::string_view options_str )
    : paladin_spell_t( "blessing_of_sacrifice", p, p->find_class_spell( "Blessing of Sacrifice" ) )
  {
    parse_options( options_str );

    harmful  = false;
    may_miss = false;

    // Create redirect action conditionalized on the existence of HoS.
    if ( !p->active.blessing_of_sacrifice_redirect )
      p->active.blessing_of_sacrifice_redirect = new blessing_of_sacrifice_redirect_t( p );
  }

  void execute() override;
};

// Lay on Hands Spell =======================================================

struct lay_on_hands_t : public paladin_heal_t
{
  lay_on_hands_t( paladin_t* p, util::string_view options_str )
    : paladin_heal_t( "lay_on_hands", p, p->find_talent_spell (talent_tree::CLASS, "Lay on Hands" ) )
  {
    parse_options( options_str );

    may_crit    = false;
    use_off_gcd = true;
    trigger_gcd = 0_ms;
    cooldown    = p->cooldowns.lay_on_hands; // Link needed for Tirion's Devotion
  }

  void execute() override
  {
    base_dd_min = base_dd_max = p()->resources.max[ RESOURCE_HEALTH ];

    paladin_heal_t::execute();

    // apply forbearance, track locally for forbearant faithful & force recharge recalculation
    p()->trigger_forbearance( execute_state->target );
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( candidate_target->debuffs.forbearance->check() )
      return false;

    return paladin_heal_t::target_ready( candidate_target );
  }
};

// Blinding Light (Holy/Prot/Retribution) =====================================

struct blinding_light_t : public paladin_spell_t
{
  blinding_light_t( paladin_t* p, util::string_view options_str )
    : paladin_spell_t( "blinding_light", p, p->talents.blinding_light )
  {
    parse_options( options_str );

    aoe = -1;

    // TODO: Apply the cc?
  }
};

// Auras ===============================================

struct paladin_aura_base_t : public paladin_spell_t
{
  buff_t* aura_buff;
  paladin_aura_base_t( util::string_view n, paladin_t* p, const spell_data_t* s ) : paladin_spell_t( n, p, s )
  {
    harmful   = false;
    aura_buff = nullptr;
  }

  void init_finished() override
  {
    paladin_spell_t::init_finished();
    assert( aura_buff != nullptr && "Paladin auras must have aura_buff set to their appropriate buff" );
  }

  void execute() override
  {
    paladin_spell_t::execute();
    // If this aura is up, cancel it. Otherwise replace the current aura.
    if ( p()->active_aura != nullptr )
    {
      p()->active_aura->expire();
      if ( p()->active_aura == aura_buff )
        p()->active_aura = nullptr;
      else
        p()->active_aura = aura_buff;
    }
    else
      p()->active_aura = aura_buff;
    if ( p()->active_aura != nullptr )
      p()->active_aura->trigger();
  }
};

struct devotion_aura_t : public paladin_aura_base_t
{
  devotion_aura_t( paladin_t* p, util::string_view options_str )
    : paladin_aura_base_t( "devotion_aura", p, p->find_spell( 465 ) )
  {
    parse_options( options_str );

    if ( !p->talents.auras_of_the_resolute->ok() )
      background = true;

    aura_buff = p->buffs.devotion_aura;
  }
};

// SoV

struct shield_of_vengeance_proc_t : public paladin_spell_t
{
  shield_of_vengeance_proc_t( paladin_t* p ) : paladin_spell_t( "shield_of_vengeance_proc", p, p->find_spell( 184689 ) )
  {
    may_miss = may_dodge = may_parry = may_glance = false;
    background                                    = true;
    split_aoe_damage                              = true;
  }

  void init() override
  {
    paladin_spell_t::init();
    snapshot_flags = 0;
  }

  proc_types proc_type() const override
  {
    return PROC1_MELEE_ABILITY;
  }
};

struct shield_of_vengeance_t : public paladin_absorb_t
{
  double shield_modifier;
  shield_of_vengeance_t( paladin_t* p, util::string_view options_str )
    : paladin_absorb_t( "shield_of_vengeance", p, p->talents.shield_of_vengeance ), shield_modifier(1.0)
  {
    parse_options( options_str );

    harmful = false;
  }

  void init() override
  {
    paladin_absorb_t::init();
    snapshot_flags |= ( STATE_CRIT | STATE_VERSATILITY );
  }

  void execute() override
  {
    double shield_amount = p()->resources.max[ RESOURCE_HEALTH ] * data().effectN( 2 ).percent();

    shield_amount *= shield_modifier;
    shield_amount *= 1.0 + p()->composite_heal_versatility();

    paladin_absorb_t::execute();
    p()->buffs.shield_of_vengeance->trigger( 1, shield_amount, -1, timespan_t::min() );
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

struct crusading_strike_t : public paladin_melee_attack_t
{
  crusading_strike_t( paladin_t* p )
    : paladin_melee_attack_t( "crusading_strike", p, p -> find_spell( 408385 ) )
  {
    background = true;
    trigger_gcd = 0_ms;

    if ( p->talents.blessed_champion->ok() )
    {
      base_aoe_multiplier *= 1.0 - p->talents.blessed_champion->effectN( 3 ).percent();
    }

    triggers_higher_calling = true;
  }

  void execute() override
  {
    paladin_melee_attack_t::execute();
    if ( result_is_hit( execute_state->result ) )
    {
        p()->resource_gain(
          RESOURCE_HOLY_POWER,
          as<int>( p()->spells.crusading_strikes_data->effectN( 1 ).base_value() ),
          p()->gains.hp_crusading_strikes
        );
      p()->trigger_aura_applied_callbacks( p()->proc_data_entries.crusading_strikes_energize, p() );
    }

    if ( result_is_hit( execute_state->result ) && p()->talents.empyrean_power->ok() )
    {
      if ( rng().roll( p()->talents.empyrean_power->effectN( 2 ).percent() ) )
      {
        p()->procs.empyrean_power->occur();
        p()->buffs.empyrean_power->trigger();
      }
    }
  }

  proc_types proc_type() const override {
    return PROC1_MELEE;
  }
};

struct melee_t : public paladin_melee_attack_t
{
  bool first;
  crusading_strike_t* crusading_strike;

  melee_t( paladin_t* p )
    : paladin_melee_attack_t( "melee", p, spell_data_t::nil() ), first( true ), crusading_strike( nullptr )
  {
    school            = SCHOOL_PHYSICAL;
    special           = false;
    background        = true;
    allow_class_ability_procs = true;
    not_a_proc        = true;
    repeating         = true;
    trigger_gcd       = 0_ms;
    base_execute_time = p->main_hand_weapon.swing_time;
    weapon_multiplier = 1.0;

    if ( p->talents.crusading_strikes->ok() )
    {
      crusading_strike = new crusading_strike_t( p );
      add_child( crusading_strike );
      execute_action = crusading_strike;
      weapon_multiplier = 0.0;

      if ( p->talents.blessed_champion->ok() )
      {
        base_aoe_multiplier *= 1.0 - p->talents.blessed_champion->effectN( 3 ).percent();
      }
      // Let Crusading Strikes handle the procs
      proc_data.suppress_caster_procs = true; 
      proc_data.suppress_target_procs = true;
    }

    affected_by.avenging_wrath = affected_by.crusade = affected_by.sentinel = true;

    if ( p->talents.heart_of_the_crusader->ok() )
    {
      base_multiplier *= 1.0 + p->talents.heart_of_the_crusader->effectN( 1 ).percent();

      // This seems likely to be a bug; the tooltip does not match the spell data
      base_crit += p->talents.heart_of_the_crusader->effectN( 2 ).percent();
    }
  }

  timespan_t execute_time() const override
  {
    if ( !player->in_combat )
      return 10_ms;
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

    if ( result_is_hit( execute_state->result ) )
    {
      if ( p()->specialization() == PALADIN_RETRIBUTION )
      {
        if ( p()->talents.art_of_war->ok() && p()->cooldowns.art_of_war->up() )
        {
          // Check for BoW procs
          double aow_proc_chance = p()->talents.art_of_war->effectN( 1 ).percent();
          if ( execute_state->result == RESULT_CRIT )
          {
            aow_proc_chance *= 1.0 + p()->talents.art_of_war->effectN( 2 ).percent();
          }
          if ( rng().roll( aow_proc_chance ) )
          {
            if ( p()->buffs.art_of_war->up() )
              p()->procs.art_of_war_wasted->occur();
            p()->buffs.art_of_war->trigger();
            p()->cooldowns.art_of_war->start();
            p()->procs.art_of_war->occur();
            p()->cooldowns.blade_of_justice->reset( true );
          }
        }
      }
    }
  }
};

// Auto Attack ==============================================================

struct auto_melee_attack_t : public paladin_melee_attack_t
{
  auto_melee_attack_t( paladin_t* p, util::string_view options_str )
    : paladin_melee_attack_t( "auto_attack", p, spell_data_t::nil() )
  {
    school = SCHOOL_PHYSICAL;
    assert( p->main_hand_weapon.type != WEAPON_NONE );
    p->main_hand_attack = new melee_t( p );

    // does not incur a GCD
    trigger_gcd = 0_ms;

    parse_options( options_str );
  }

  void execute() override
  {
    if ( p()->main_hand_attack->execute_event == nullptr )
    {
      p()->main_hand_attack->schedule_execute();
    }
  }

  bool ready() override
  {
    if ( p()->is_moving() )
      return false;

    return ( p()->main_hand_attack->execute_event == nullptr );  // not swinging
  }
};

// Crusader Strike ==========================================================

struct crusader_strike_t : public paladin_melee_attack_t
{
  bool has_crusader_2;
  crusader_strike_t( paladin_t* p, util::string_view options_str )
    : paladin_melee_attack_t( "crusader_strike", p, p->find_class_spell( "Crusader Strike" ) ),
      has_crusader_2( p->find_specialization_spell( 342348 )->ok() )
  {
    parse_options( options_str );

    if ( p->talents.blessed_champion->ok() )
    {
      base_aoe_multiplier *= 1.0 - p->talents.blessed_champion->effectN( 3 ).percent();
    }

    if ( p->talents.crusading_strikes->ok() || p->talents.templar_strikes->ok() )
    {
      background = true;
    }

    triggers_higher_calling = true;
  }
  void impact( action_state_t* s ) override
  {
    paladin_melee_attack_t::impact( s );

    if ( p()->talents.crusaders_might->ok() )
    {
      timespan_t cm_cdr = p()->talents.crusaders_might->effectN( 1 ).time_value();
      p()->cooldowns.holy_shock->adjust( cm_cdr );
    }
  }

  void execute() override
  {
    paladin_melee_attack_t::execute();

    if ( p()->specialization() == PALADIN_RETRIBUTION )
    {
      p()->resource_gain( RESOURCE_HOLY_POWER, p()->spec.retribution_paladin->effectN( 14 ).base_value(),
                          p()->gains.hp_cs );
    }

    if ( result_is_hit( execute_state->result ) && p()->talents.empyrean_power->ok() )
    {
      if ( rng().roll( p()->talents.empyrean_power->effectN( 1 ).percent() ) )
      {
        p()->procs.empyrean_power->occur();
        p()->buffs.empyrean_power->trigger();
      }
    }

    p()->trigger_grand_crusader();
    p()->buffs.lightsmith.blessed_assurance->expire();
  }

  double cost() const override
  {
    if ( has_crusader_2 )
      return 0;

    return paladin_melee_attack_t::cost();
  }

  double action_multiplier() const override
  {
    double am = paladin_melee_attack_t::action_multiplier();
    am *= 1.0 + p()->buffs.lightsmith.blessed_assurance->stack_value();
    return am;
  }
};

struct light_of_the_titans_t : public paladin_heal_t
{
  struct light_of_the_titans_hot_t : public paladin_heal_t
  {
    light_of_the_titans_hot_t( paladin_t* p )
      : paladin_heal_t( "Light of the Titans (HoT)", p, p->talents.light_of_the_titans ->effectN( 1 ).trigger() )
    {
      tick_zero  = false;
      background = true;
    }
    void tick(dot_t* d) override
    {
      paladin_heal_t::tick( d );
    }
    double composite_ta_multiplier(const action_state_t* s) const override
    {
      double m = paladin_heal_t::composite_ta_multiplier( s );
      return m;
    }
  };

  light_of_the_titans_hot_t* periodic;

  light_of_the_titans_t( paladin_t* p, util::string_view options_str )
    : paladin_heal_t( "Light of the Titans", p, p->talents.light_of_the_titans ),
      periodic( new light_of_the_titans_hot_t( p ) )
  {
    parse_options( options_str );
    tick_zero = false;
    impact_action = periodic;
  }

  void impact( action_state_t* s ) override
  {
    paladin_t* p = debug_cast<paladin_t*>( player );
    for ( size_t i = 0, size = p->dot_list.size(); i < size; i++ )
    {
      dot_t* d = p->dot_list[ i ];
      // If a hostile DoT is ticking on us, Light of the Titans heals for 50% more
      if ( d->source != p && d->source->is_enemy() && d->is_ticking() )
      {
        // ToDo: This actually does nothing, tried different variables, none seem to work.
        // So... Kinda NYI-ish for now? At least something ticks...
        periodic->base_multiplier = 1.0 + p->talents.light_of_the_titans->effectN( 2 ).percent();
        break;
      }
    }
    paladin_heal_t::impact( s );
  }
};
struct sacrosanct_crusade_heal_t : public paladin_heal_t
{
  sacrosanct_crusade_heal_t( paladin_t* p ) : paladin_heal_t( "sacrosanct_crusade_heal", p, p->find_spell( 461885 ) )
  {
    background    = true;
    may_crit      = false;
    harmful       = false;
    target        = p;
    base_pct_heal = 0;  // We need to overwrite this later, since it scales with targets hit and the Paladin's spec
  }

  void impact( action_state_t* s ) override
  {
    auto health_before = p()->resources.current[ RESOURCE_HEALTH ];
    paladin_heal_t::impact( s );
    if ( p()->resources.current[ RESOURCE_HEALTH ] >= p()->resources.max[ RESOURCE_HEALTH ] )
    {
      double absorb = s->result_total + health_before - p()->resources.max[ RESOURCE_HEALTH ];
      p()->buffs.templar.sacrosanct_crusade->trigger( -1, absorb, -1, timespan_t::min() );
    }
  }
};

// Word of Glory ===================================================

struct word_of_glory_t : public holy_power_consumer_t<paladin_heal_t>
{
  struct sacred_word_t : public paladin_heal_t
  {
    sacred_word_t( paladin_t* p ) : paladin_heal_t( "sacred_word", p, p->spells.lightsmith.sacred_word )
    {
      background = true;
    }
  };

  sacred_word_t* sacred_word;
  light_of_the_titans_t* light_of_the_titans;
  word_of_glory_t( paladin_t* p, util::string_view options_str )
    : holy_power_consumer_t( "word_of_glory", p, p->find_class_spell( "Word of Glory" ) ),
      sacred_word( nullptr ),
      light_of_the_titans( new light_of_the_titans_t( p, "" ) )
  {
    parse_options( options_str );
    target = p;
    is_wog = true;
    if ( p->talents.lightsmith.blessing_of_the_forge->ok() )
    {
      sacred_word = new sacred_word_t( p );
      add_child( sacred_word );
    }
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = holy_power_consumer_t::composite_target_multiplier( t );

    if ( p()->spec.word_of_glory_2->ok() )
    {
      // Heals for a base amount, increased by up to +300% based on the target's missing health
      // Linear increase, each missing health % increases the healing by 3%
      double missing_health_percent = std::min( 1.0 - t->resources.pct( RESOURCE_HEALTH ), 1.0 );

      m *= 1.0 + missing_health_percent * p()->spec.word_of_glory_2->effectN( 1 ).percent();

      sim->print_debug( "Player {} missing {:.2f}% health, healing increased by {:.2f}%", t->name(),
                        missing_health_percent * 100,
                        missing_health_percent * p()->spec.word_of_glory_2->effectN( 1 ).percent() * 100 );
    }
    return m;
  }

  void impact( action_state_t* s ) override
  {
    holy_power_consumer_t::impact( s );

    if ( p() ->talents.light_of_the_titans->ok())
    {
      light_of_the_titans->execute();
    }
  }

  void execute() override
  {
    if ( p()->specialization() == PALADIN_PROTECTION && p()->talents.lightsmith.valiance->ok() &&
         p()->buffs.shining_light_free->up() )
    {
      timespan_t reduction = timespan_t::from_millis( p()->talents.lightsmith.valiance->effectN( 1 ).base_value() );
      p()->cooldowns.holy_armaments->adjust( -reduction );
    }

    holy_power_consumer_t::execute();

    if ( p()->specialization() == PALADIN_HOLY && p()->talents.awakening->ok() )
    {
      if ( rng().roll( p()->talents.awakening->effectN( 1 ).percent() ) )
      {
        buff_t* main_buff           = p()->buffs.avenging_wrath;
        timespan_t trigger_duration = timespan_t::from_seconds( p()->talents.awakening->effectN( 2 ).base_value() );
        if ( main_buff->check() )
        {
          p()->buffs.avenging_wrath->extend_duration( trigger_duration );
        }
        else
        {
          main_buff->trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, trigger_duration );
        }
      }
    }
    if (p()->buffs.lightsmith.blessing_of_the_forge->up())
    {
      sacred_word->execute_on_target( execute_state->target );
    }
  }

  double action_multiplier() const override
  {
    double am = holy_power_consumer_t::action_multiplier();
    if ( p()->buffs.shining_light_free->up() && p()->buffs.divine_purpose->up() )
      // Shining Light does not benefit from divine purpose
      am /= 1.0 + p()->spells.divine_purpose_buff->effectN( 2 ).percent();
    return am;
  }
};

// Hammer of Justice ========================================================

struct hammer_of_justice_t : public paladin_melee_attack_t
{
  hammer_of_justice_t( paladin_t* p, util::string_view options_str )
    : paladin_melee_attack_t( "hammer_of_justice", p, p->find_class_spell( "Hammer of Justice" ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    // TODO implement stun?
  }
};


hammer_and_anvil_t::hammer_and_anvil_t( paladin_t* p, util::string_view n )
: paladin_spell_t( n, p, p->find_spell( 433717 ) )
{
  background = proc = may_crit = true;
  may_miss                     = false;
  aoe                          = -1;
  reduced_aoe_targets          = 5;
}

bool trigger_hammer_and_anvil( paladin_t* p, player_t* target, hammer_and_anvil_t* haa,
                               hammer_and_anvil_source = HAA_JUDGMENT )
{
  if ( p->talents.lightsmith.hammer_and_anvil->ok() )
  {
    haa->set_target( target );
    haa->execute();
    return true;
  }
  return false;
}



unrelenting_edict_t::unrelenting_edict_t( paladin_t* p, util::string_view name ) : paladin_spell_t( std::string(name) + "_ue", p, p->spells.unrelenting_edict )
{

}

void unrelenting_edict_t::do_execute(action_state_t* s)
{
  if ( p()->sets->has_set_bonus( PALADIN_PROTECTION, MID2, B4 ) )
  {
    auto b      = p()->sets->set( PALADIN_PROTECTION, MID2, B4 );
    double perc = b->effectN( 1 ).percent();
    if ( s->result == result_e::RESULT_CRIT )
      perc *= 1 + b->effectN( 2 ).percent();
    base_dd_min = base_dd_max = s->result_amount * perc;

    execute();
  }
}


// Base Judgment spell ======================================================

judgment_base_t::judgment_base_t( paladin_t* p, util::string_view name, const spell_data_t* s )
  : paladin_melee_attack_t( name, p, s ),
    hammer_and_anvil( nullptr ),
    judge_holy_power( as<int>( p->find_spell( 220637 )->effectN( 1 ).base_value() ) ),
    sw_holy_power( as<int>( p->talents.sanctified_wrath->effectN( 3 ).base_value() ) )
{
  triggers_higher_calling     = true;
  triggers_highlords_judgment = p->specialization() == PALADIN_RETRIBUTION && s->id() != 24275;
  if ( p->talents.lightsmith.hammer_and_anvil->ok() )
  {
    hammer_and_anvil = new hammer_and_anvil_t( p, "hammer_and_anvil_" + name_str );
    add_child( hammer_and_anvil );
  }
}

judgment_base_t::judgment_base_t( paladin_t* p, util::string_view name, util::string_view options_str, const spell_data_t* s )
  : paladin_melee_attack_t( name, p, s ),
    hammer_and_anvil( nullptr ),
    judge_holy_power( as<int>( p->find_spell( 220637 )->effectN( 1 ).base_value() ) ),
    sw_holy_power( as<int>( p->talents.sanctified_wrath->effectN( 3 ).base_value() ) )
{
  parse_options(options_str);
  triggers_higher_calling     = true;
  triggers_highlords_judgment = p->specialization() == PALADIN_RETRIBUTION;
  if ( p->talents.lightsmith.hammer_and_anvil->ok() )
  {
    hammer_and_anvil = new hammer_and_anvil_t( p, "hammer_and_anvil_" + name_str );
    add_child( hammer_and_anvil );
  }
}

void judgment_base_t::execute()
{
  paladin_melee_attack_t::execute();

  if ( p()->talents.zealots_paragon->ok() )
  {
    auto extension = timespan_t::from_millis( p()->talents.zealots_paragon->effectN( 1 ).base_value() );

    if ( p()->buffs.avenging_wrath->up() )
    {
      p()->buffs.avenging_wrath->extend_duration( extension );
    }

    if ( p()->buffs.sentinel->up() )
    {
      p()->buffs.sentinel->extend_duration( extension );
    }
  }

  if ( p()->buffs.herald_of_the_sun.blessing_of_anshe->up() )
    p()->buffs.herald_of_the_sun.blessing_of_anshe->expire();
}
void judgment_base_t::impact(action_state_t* s)
{
  paladin_melee_attack_t::impact( s );
  if ( result_is_hit( s->result ) && p()->talents.greater_judgment->ok() )
  {
    p()->trigger_greater_judgment( td( s->target ) );
  }
  if ( triggers_highlords_judgment )
  {
    double mastery_chance = p()->cache.mastery() * p()->mastery.highlords_judgment->effectN( 4 ).mastery_value();
    if ( p()->talents.boundless_judgment->ok() )
      mastery_chance *= 1.0 + p()->talents.boundless_judgment->effectN( 1 ).percent();
    if ( p()->talents.highlords_wrath->ok() )
      mastery_chance *= 1.0 + p()->talents.highlords_wrath->effectN( 3 ).percent() /
                                  p()->talents.highlords_wrath->effectN( 2 ).base_value();

    if ( rng().roll( mastery_chance ) )
    {
      p()->active.highlords_judgment->set_target( s->target );
      p()->active.highlords_judgment->execute();
    }
  }
}

judgment_t::judgment_t( paladin_t* p, util::string_view name, const spell_data_t* s ) : judgment_base_t( p, name, "", s )
{
  // no weapon multiplier
  weapon_multiplier = 0.0;
  may_block = may_parry = may_dodge = false;
  // force effect 1 to be used for direct ratios
  parse_effect_data( data().effectN( 1 ) );
  if ( p->sets->has_set_bonus( PALADIN_PROTECTION, MID2, B4 ) )
  {
    ue = new unrelenting_edict_t( p, "judgment" );
    add_child( ue );
  }
}

judgment_t::judgment_t( paladin_t* p, util::string_view name, util::string_view options_str, const spell_data_t* s )
  : judgment_base_t( p, name, options_str, s )
{
  // no weapon multiplier
  weapon_multiplier = 0.0;
  may_block = may_parry = may_dodge = false;
  // force effect 1 to be used for direct ratios
  parse_effect_data( data().effectN( 1 ) );
  if ( p->sets->has_set_bonus( PALADIN_PROTECTION, MID2, B4 ) )
  {
    ue = new unrelenting_edict_t( p, "judgment" );
    add_child( ue );
  }

  if ( p->cooldowns.judgment == nullptr )
    p->cooldowns.judgment = cooldown;
  else
    cooldown = p->cooldowns.judgment;
}

proc_types judgment_t::proc_type() const
{
  return PROC1_MELEE_ABILITY;
}

void judgment_t::execute()
{
  judgment_base_t::execute();

  if ( result_is_hit( execute_state->result ) )
  {
    int hopo = 0;
    if ( p()->spec.judgment_3->ok() )
      hopo += judge_holy_power;
    if ( p()->talents.sanctified_wrath->ok() && p()->wings_up() )
      hopo += sw_holy_power;
    if ( hopo > 0 )
      p()->resource_gain( RESOURCE_HOLY_POWER, hopo, p()->gains.judgment );
  }

  if ( p()->talents.templar.sanctification->ok() )
  {
    p()->buffs.templar.sanctification->trigger();
  }
  // Proc chance for Glory of the Vanguard not in Spell Data, seems to be 20% according to Fatpala
  if ( p()->talents.glory_of_the_vanguard_1->ok() && p()->rng().roll( 0.2 ) )
  {
    p()->buffs.vanguard->trigger();
  }
  if ( crit_any_target )
  {
    trigger_hammer_and_anvil( p(), execute_state->target, hammer_and_anvil, HAA_JUDGMENT );
  }
}

void judgment_t::impact(action_state_t* s)
{
  judgment_base_t::impact( s );
  if ( p()->sets->has_set_bonus( PALADIN_PROTECTION, MID2, B4 ) )
    ue->do_execute( s );
}

bool judgment_t::action_ready()
{
  return judgment_base_t::action_ready() && !p()->buffs.hammer_of_wrath->up();
}

// Judgment - Retribution =================================================================

struct judgment_ret_t : public judgment_t
{
  int holy_power_generation;

  judgment_ret_t( paladin_t* p, util::string_view name, const spell_data_t* s )
    : judgment_t( p, name, s ),
      holy_power_generation( as<int>( p->find_spell( 220637 )->effectN( 1 ).base_value() ) )
  {
    if ( p->talents.blessed_champion->ok() )
    {
      base_aoe_multiplier *= 1.0 - p->talents.blessed_champion->effectN( 3 ).percent();
    }
  }

  judgment_ret_t( paladin_t* p, util::string_view name, util::string_view options_str, const spell_data_t* s )
    : judgment_t( p, name, options_str, s ),
      holy_power_generation( as<int>( p->find_spell( 220637 )->effectN( 1 ).base_value() ) )
  {
    if ( p->talents.blessed_champion->ok() )
    {
      base_aoe_multiplier *= 1.0 - p->talents.blessed_champion->effectN( 3 ).percent();
    }
  }

  void execute() override
  {
    judgment_t::execute();

    if ( !background && p()->specialization() == PALADIN_RETRIBUTION && p()->buffs.divine_resonance->up() )
    {
      p()->active.divine_resonance_ret->execute_on_target( execute_state->target );
      p()->buffs.divine_resonance->decrement();
    }
  }
};

struct divine_toll_judgment_ret_t :judgment_ret_t
{
  divine_toll_judgment_ret_t( paladin_t* p ) : judgment_ret_t( p, "judgment_divine_toll", p->spells.judgment_ret_dt )
  {
    background = true;
    aoe        = 1;  // Divine Toll's Judgments don't cleave further
    base_multiplier *= 1.0 + p->talents.divine_toll->effectN( 6 ).percent();
    cooldown->duration = 0_ms;
  }
};

struct divine_resonance_judgment_t :judgment_ret_t
{
  divine_resonance_judgment_t(paladin_t* p) : judgment_ret_t(p, "judgment_divine_resonance", p->spells.judgment_ret)
  {
    background = true;
    base_multiplier *= p->buffs.divine_resonance->data().effectN( 2 ).percent();
    cooldown->duration = 0_ms;
  }
};

struct divine_exaction_judgment_t : public judgment_ret_t
{
  divine_exaction_judgment_t( paladin_t* p ) : judgment_ret_t( p, "judgment_divine_exaction", p->spells.judgment_ret_dt )
  {
    background = true;
    aoe        = 1;  // DE's Hammer of Wrath's don't cleave further
    // 22.02.26 Fluttershy - We are thinking Divine Toll Judgment/HoW are 150% increased effectiveness from base damage, instead of 50% increased effectiveness from buffed damage (which should be 300%, instead of 250%)
    if (!p->bugs)
      base_multiplier *= 1.0 + p->talents.divine_toll->effectN( 6 ).percent();
    double de_mult = p->talents.templar.divine_exaction->effectN( 2 ).percent();
    if ( p->bugs )
      de_mult += 1.0;
    base_multiplier *= de_mult;
    cooldown->duration = 0_ms;
  }
};

struct divine_toll_hammer_of_wrath_ret_t : hammer_of_wrath_t
{
  divine_toll_hammer_of_wrath_ret_t( paladin_t* p )
    : hammer_of_wrath_t( p, "hammer_of_wrath_divine_toll", p->spells.hammer_of_wrath_ret_dt)
  {
    background = true;
    aoe        = 1;  // Divine Toll's Hammer of Wraths don't cleave further
    base_multiplier *= 1.0 + p->talents.divine_toll->effectN( 6 ).percent();
    triggers_second_sunrise   = false;
    triggers_divine_resonance = false;
    cooldown->duration        = 0_ms;
  }
};

struct divine_resonance_hammer_of_wrath_t :hammer_of_wrath_t
{
  divine_resonance_hammer_of_wrath_t(paladin_t* p)
    : hammer_of_wrath_t( p, "hammer_of_wrath_divine_resonance", p->spells.hammer_of_wrath_ret )
  {
    background = true;
    base_multiplier *= p->buffs.divine_resonance->data().effectN( 2 ).percent();
    triggers_second_sunrise = false;
    triggers_divine_resonance = false;
    cooldown->duration        = 0_ms;
  }
};

struct divine_exaction_hammer_of_wrath_t :public hammer_of_wrath_t
{
  divine_exaction_hammer_of_wrath_t(paladin_t* p)
    : hammer_of_wrath_t(p, "hammer_of_wrath_divine_exaction", p->spells.hammer_of_wrath_ret_dt)
  {
    background = true;
    aoe        = 1; // DE's Hammer of Wrath's don't cleave further
    // 22.02.26 Fluttershy - We are thinking Divine Toll Judgment/HoW are 150% increased effectiveness from base damage, instead of 50% increased effectiveness from buffed damage (which should be 300%, instead of 250%)
    if ( !p->bugs )
      base_multiplier *= 1.0 + p->talents.divine_toll->effectN( 6 ).percent();
    double de_mult = p->talents.templar.divine_exaction->effectN( 2 ).percent();
    if ( p->bugs )
      de_mult += 1.0;
    base_multiplier *= de_mult;
    triggers_second_sunrise   = false;
    triggers_divine_resonance = false;
    cooldown->duration = 0_ms;
  }
};

hammer_of_wrath_t::hammer_of_wrath_t(paladin_t* p, util::string_view n, const spell_data_t* s)
  : judgment_base_t(p, n, s)
{
  background = true;
  triggers_divine_resonance = true;
  triggers_second_sunrise   = false;
  cooldown->duration        = 0_ms;
}

hammer_of_wrath_t::hammer_of_wrath_t( paladin_t* p, util::string_view name, util::string_view options_str,
                                      const spell_data_t* s )
  : judgment_base_t( p, name, options_str, s ), echo( nullptr )
{
  if ( p->talents.adjudication->ok() )
  {
    add_child( p->active.background_blessed_hammer );
  }
  triggers_higher_calling   = true;
  triggers_second_sunrise   = !background;
  triggers_divine_resonance = !background;
  may_block = may_parry = may_dodge = false;
  // force effect 1 to be used for direct ratios
  parse_effect_data( data().effectN( 1 ) );

  if ( p->talents.blessed_champion->ok() )
  {
    aoe = as<int>( 1 + p->talents.blessed_champion->effectN( 4 ).base_value() );
    base_aoe_multiplier *= 1.0 - p->talents.blessed_champion->effectN( 3 ).percent();
  }

  if ( p->talents.herald_of_the_sun.second_sunrise->ok() )
  {
    echo = new hammer_of_wrath_t( p, "hammer_of_wrath_second_sunrise", p->spells.hammer_of_wrath_ret );
    echo->base_multiplier         = base_multiplier;
    echo->aoe                     = aoe;
    echo->base_aoe_multiplier     = base_aoe_multiplier;
    echo->crit_bonus_multiplier   = crit_bonus_multiplier;
    echo->triggers_higher_calling = true;
    echo->base_multiplier *= p->talents.herald_of_the_sun.second_sunrise->effectN( 2 ).percent();
  }
  if ( p->specialization() == PALADIN_PROTECTION )
  {
    if ( p->cooldowns.judgment == nullptr )
      p->cooldowns.judgment = cooldown;
    else
      cooldown = p->cooldowns.judgment;
  }
  else
  {
    p->cooldowns.hammer_of_wrath = cooldown;
  }
  if ( !p->bugs && p->sets->has_set_bonus( PALADIN_PROTECTION, MID2, B4 ) )
  {
    ue = new unrelenting_edict_t( p, "judgment" );
    add_child( ue );
  }
}

void hammer_of_wrath_t::execute()
{
  judgment_base_t::execute();

  // Hammer of Wrath generates an additional Holy Power for Prot with Sanctified Wrath
  if ( result_is_hit( execute_state->result ) && p()->talents.sanctified_wrath->ok() && p()->wings_up() )
    p()->resource_gain( RESOURCE_HOLY_POWER, 1, p()->gains.judgment );

  if ( triggers_divine_resonance && p()->specialization() == PALADIN_RETRIBUTION && p()->buffs.divine_resonance->up() )
  {
    p()->active.divine_resonance_ret_how->execute_on_target( execute_state->target );
    p()->buffs.divine_resonance->decrement();
  }
  if (p()->talents.herald_of_the_sun.walk_into_light->ok() && p()->wings_up() && p()->cooldowns.walk_into_light_icd->up())
  {
    p()->active.blade_of_justice->execute_on_target( execute_state->target );
    p()->cooldowns.walk_into_light_icd->start();
  }
  if ( p()->talents.templar.sanctification->ok() )
  {
    p()->buffs.templar.sanctification->trigger();
  }
  if ( crit_any_target )
  {
    trigger_hammer_and_anvil( p(), execute_state->target, hammer_and_anvil, HAA_JUDGMENT );
  }
}

void hammer_of_wrath_t::impact( action_state_t* s )
{
  judgment_base_t::impact( s );

  if ( !result_is_hit( s->result ) )
    return;

  if ( s->result == RESULT_CRIT && p()->talents.herald_of_the_sun.sun_sear->ok() &&
        p()->specialization() == PALADIN_RETRIBUTION )
  {
    p()->active.sun_sear->target = s->target;
    p()->active.sun_sear->execute();
  }

  if ( triggers_second_sunrise && echo != nullptr && s->chain_target == 0 && p()->cooldowns.second_sunrise_icd->up() )
  {
    if ( rng().roll( p()->talents.herald_of_the_sun.second_sunrise->effectN( 1 ).percent() ) )
    {
      p()->cooldowns.second_sunrise_icd->start();
      // TODO(mserrano): verify this delay
      echo->target = s->target;
      echo->start_action_execute_event( 200_ms );
    }
  }
  if ( !p()->bugs && p()->sets->has_set_bonus( PALADIN_PROTECTION, MID2, B4 ) )
    ue->do_execute( s );
}

double hammer_of_wrath_t::composite_target_multiplier( player_t* target ) const
{
  double ctm = judgment_base_t::composite_target_multiplier( target );

  // Damage is fully effective at 20% HP, according to Vael
  double min = 20.0;
  ctm *= 1.0 + p()->talents.vengeful_wrath->effectN( 1 ).percent() *
                   ( 1.0 - ( std::max( target->health_percentage() - min, 0.0 ) / ( 100.0 - min ) ) );
  return ctm;
}

bool hammer_of_wrath_t::action_ready()
{
  return judgment_base_t::action_ready() && p()->buffs.hammer_of_wrath->up();
}

void paladin_t::trigger_greater_judgment( paladin_td_t* targetdata )
{
  if ( !targetdata->target->in_combat )
    return;

  auto stack = spells.judgment_debuff->initial_stacks();

  if ( stack )
    targetdata->debuff.judgment->execute( stack );
}

struct divine_toll_t : public paladin_spell_t
{
  divine_toll_judgment_ret_t* judgment;
  divine_exaction_judgment_t* judgment_de;
  divine_toll_hammer_of_wrath_ret_t* how;
  divine_exaction_hammer_of_wrath_t* how_de;
  hammer_and_anvil_t* haa;
  divine_toll_t( paladin_t* p, util::string_view options_str )
    : paladin_spell_t( "divine_toll", p, p->talents.divine_toll ),
      judgment( nullptr ),
      judgment_de( nullptr ),
      how( nullptr ),
      how_de( nullptr )
  {
    parse_options( options_str );

    aoe = as<int>( data().effectN( 1 ).base_value() );

    add_child( p->active.divine_toll );
    if ( p->specialization() == PALADIN_PROTECTION )
    {
      add_child( p->active.divine_resonance );
    }
    else if ( p->specialization() == PALADIN_RETRIBUTION )
    {
      judgment = new divine_toll_judgment_ret_t( p );
      add_child( judgment );
      how = new divine_toll_hammer_of_wrath_ret_t( p );
      add_child( how );
      add_child( p->active.divine_resonance_ret );
      add_child( p->active.divine_resonance_ret_how );
    }
    if ( p->talents.templar.divine_exaction->ok() )
    {
      if ( p->specialization() == PALADIN_PROTECTION )
        add_child( p->active.divine_exaction_prot );
      else if ( p->specialization() == PALADIN_RETRIBUTION )
      {
        judgment_de = new divine_exaction_judgment_t( p );
        how_de      = new divine_exaction_hammer_of_wrath_t( p );
        add_child( judgment_de );
        add_child( how_de );
      }
    }
    if ( p->talents.templar.divine_hammer->ok() )
    {
      add_child( p->active.divine_hammer_tick );
    }
    if (p->talents.lightsmith.resounding_strike->ok())
    {
      haa = new hammer_and_anvil_t( p, "hammer_and_anvil_divine_toll" );
      add_child( haa );
    }
  }

  void impact( action_state_t* s ) override
  {
    paladin_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      if (p()->specialization() == PALADIN_RETRIBUTION )
      {
        if ( p()->buffs.hammer_of_wrath->up() )
          how->execute_on_target( s->target );
        else
          judgment->execute_on_target( s->target );
      }
      else
      {
        p()->active.divine_toll->execute_on_target( s->target );
      }
    }
  }

  bool target_ready(player_t* candidate_target) override
  {
    if ( p()->specialization() == PALADIN_PROTECTION && p()->buffs.templar.hammer_of_light_ready->up() )
      return false;

    return paladin_spell_t::target_ready( candidate_target );
  }

  void execute() override
  {
    paladin_spell_t::execute();
    if ( p()->talents.divine_resonance->ok() )
    {
      p()->buffs.divine_resonance->trigger();
    }
    if (p()->talents.templar.divine_hammer->ok())
    {
      p()->buffs.templar.divine_hammer->trigger();
    }
    if (p()->specialization() == PALADIN_PROTECTION && p()->templar())
    {
      p()->buffs.templar.hammer_of_light_ready->trigger();
    }
    if ( p()->talents.templar.divine_exaction->ok() )
    {
      action_t* a = nullptr;
      if ( p()->specialization() == PALADIN_RETRIBUTION )
      {
        // ToDo Fluttershy: Find out state after 300ms?
        if ( p()->buffs.hammer_of_wrath->up() )
          a = how_de;
        else
          a = judgment_de;
      }
      else if ( p()->specialization() == PALADIN_PROTECTION )
      {
        a = p()->active.divine_exaction_prot;
      }
      for ( int i = 0; i < p()->talents.templar.divine_exaction->effectN( 1 ).base_value(); i++ )
      {
        make_event<delayed_execute_event_t>( *sim, p(), a, execute_state->target, 300_ms * ( i + 1 ) );
      }
    }
    if ( p()->talents.lightsmith.resounding_strike->ok() )
    {
      trigger_hammer_and_anvil( p(), execute_state->target, haa, HAA_DIVINE_TOLL );
    }
  }
};

// Rebuke ===================================================================

struct rebuke_t : public paladin_melee_attack_t
{
  rebuke_t( paladin_t* p, util::string_view options_str )
    : paladin_melee_attack_t( "rebuke", p, p->find_class_spell( "Rebuke" ) )
  {
    parse_options( options_str );

    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
    ignore_false_positive                                                = true;

    // no weapon multiplier
    weapon_multiplier = 0.0;
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( !candidate_target->debuffs.casting || !candidate_target->debuffs.casting->check() )
      return false;

    return paladin_melee_attack_t::target_ready( candidate_target );
  }
};

// Reckoning ==================================================================

struct hand_of_reckoning_t : public paladin_melee_attack_t
{
  hand_of_reckoning_t( paladin_t* p, util::string_view options_str )
    : paladin_melee_attack_t( "hand_of_reckoning", p, p->find_class_spell( "Hand of Reckoning" ) )
  {
    parse_options( options_str );
    use_off_gcd = true;
  }

  void impact( action_state_t* s ) override
  {
    if ( s->target->is_enemy() )
      target->taunt( player );

    paladin_melee_attack_t::impact( s );
  }
};

struct weapon_enchant_t : public paladin_spell_t
{
  buff_t* enchant;

  weapon_enchant_t( util::string_view name, paladin_t* p, const spell_data_t* spell,
                  util::string_view options_str )
    : paladin_spell_t( name, p, spell ), enchant ( nullptr )
  {
    harmful = callbacks = false;
    target              = p;

    parse_options( options_str );
  }

  bool ready() override
  {
    if ( p()->items[ SLOT_MAIN_HAND ].active() && p()->items[ SLOT_MAIN_HAND ].selected_temporary_enchant() > 0 )
    {
      return false;
    }

    return paladin_spell_t::ready();
  }
};

struct rite_of_sanctification_t : public weapon_enchant_t
{
  rite_of_sanctification_t( paladin_t* p, util::string_view options_str )
    : weapon_enchant_t( "rite_of_sanctification", p, p->talents.lightsmith.rite_of_sanctification, options_str )
  {
    enchant = p->buffs.lightsmith.rite_of_sanctification;
  }
  void execute() override
  {
    weapon_enchant_t::execute();
    p()->buffs.lightsmith.rite_of_sanctification->execute();
  }

  void init_finished() override
  {
    weapon_enchant_t::init_finished();

    if ( !p()->talents.lightsmith.rite_of_sanctification->ok() )
      return;

    if ( p()->items[ SLOT_MAIN_HAND ].active() && p()->items[ SLOT_MAIN_HAND ].selected_temporary_enchant() > 0 )
    {
      sim->error( "Player {} has a temporary enchant {} on slot {}, disabling {}", p()->name(),
                  p()->items[ SLOT_MAIN_HAND ].selected_temporary_enchant(), util::slot_type_string( SLOT_MAIN_HAND ),
                  name() );
    }
  }
};

struct rite_of_adjuration_t : public weapon_enchant_t
{
  rite_of_adjuration_t( paladin_t* p, util::string_view options_str )
    : weapon_enchant_t( "rite_of_adjuration", p, p->talents.lightsmith.rite_of_adjuration, options_str )
  {
    enchant = p->buffs.lightsmith.rite_of_adjuration;
  }

  void execute() override
  {
    weapon_enchant_t::execute();
    p()->buffs.lightsmith.rite_of_adjuration->execute();
    p()->adjust_health_percent();
  }

  void init_finished() override
  {
    weapon_enchant_t::init_finished();

    if ( !p()->talents.lightsmith.rite_of_adjuration->ok() )
      return;

    if ( p()->items[ SLOT_MAIN_HAND ].active() && p()->items[ SLOT_MAIN_HAND ].selected_temporary_enchant() > 0 )
    {
      sim->error( "Player {} has a temporary enchant {} on slot {}, disabling {}", p()->name(),
                  p()->items[ SLOT_MAIN_HAND ].selected_temporary_enchant(), util::slot_type_string( SLOT_MAIN_HAND ),
                  name() );
    }
  }
};

// Hammer of Light // Light's Guidance =====================================================

struct hammer_of_light_data_t
{
  double divine_purpose_mult;
};

struct hammer_of_light_t : public holy_power_consumer_t<paladin_melee_attack_t>
{
  using state_t = paladin_action_state_t<hammer_of_light_data_t>;
  struct hammer_of_light_cleave_t : public holy_power_consumer_t<paladin_melee_attack_t>
  {
    hammer_of_light_cleave_t( paladin_t* p, util::string_view options_str )
      : holy_power_consumer_t( "hammer_of_light_damage", p, p->spells.templar.hammer_of_light )
    {
      parse_options( options_str );
      background = true;

      is_hammer_of_light_cleave         = true;
      aoe                        = as<int>( p->spells.templar.hammer_of_light_driver->effectN( 2 ).base_value() );
      doesnt_consume_dp          = true;   // The driver consumes DP
      affected_by.divine_purpose = false;  // We handle this manually
      base_execute_time          = timespan_t::from_millis( 600 ); // Still has a 600ms execute time, for whatever reasons. Not in spell data anymore.
      dual                       = true;
      target_filter_callback     = secondary_targets_only();
    }

    action_state_t* new_state() override
    {
      return new state_t( this, target );
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      auto da = paladin_melee_attack_t::composite_da_multiplier( s );
      auto s_ = static_cast<const state_t*>( s );
      da *= 1.0 + s_->divine_purpose_mult;
      return da;
    }

    void execute() override
    {
      if ( p()->specialization() == PALADIN_RETRIBUTION && p()->talents.templar.undisputed_ruling->ok() &&
           p()->talents.greater_judgment->ok() )
      {
        auto tl = target_list();
        for ( size_t i = 0; i < std::min( as<size_t>( n_targets() ), tl.size() ); i++ )
        {
          p()->trigger_greater_judgment( td( tl[ i ] ) );
        }
      }
      snapshot_state( pre_execute_state, amount_type( pre_execute_state ) );
      holy_power_consumer_t::execute();
      if ( p()->talents.templar.shake_the_heavens->ok() )
      {
        if ( p()->buffs.templar.shake_the_heavens->up() )
        {
          // While Shake the Heavens is up, 8 seconds are added to the duration, up to 10.4 seconds (Pandemic limit). If
          // the current duration is above the Pandemic Limit, it's duration does not change.
          if ( p()->buffs.templar.shake_the_heavens->remains() <
               p()->buffs.templar.shake_the_heavens->base_buff_duration * 1.3 )
            p()->buffs.templar.shake_the_heavens->refresh();
        }
        else
          p()->buffs.templar.shake_the_heavens->execute();
      }
    }
  };

  hammer_of_light_cleave_t* cleave_hammer;
  double prot_cost;
  double ret_cost;
  hammer_of_light_t( paladin_t* p, util::string_view options_str )
    : holy_power_consumer_t( "hammer_of_light", p, p->spells.templar.hammer_of_light_driver ), cleave_hammer()
  {
    parse_options( options_str );
    is_hammer_of_light_main = true;
    is_hammer_of_light_cleave        = true;
    cleave_hammer             = new hammer_of_light_cleave_t( p, options_str );
    background                = !p->templar();
    // This is not set by definition, since cost changes by spec
    resource_current = RESOURCE_HOLY_POWER;
    ret_cost         = data().powerN( 1 ).cost();
    prot_cost        = data().powerN( 2 ).cost();
    cleave_hammer->stats = stats;
    add_child( cleave_hammer );
    if ( p->specialization() == PALADIN_PROTECTION )
      add_child( p->active.hammer_of_light_cons );

    doesnt_consume_dp = false;
    hol_cost          = p->specialization() == PALADIN_RETRIBUTION ? ret_cost : prot_cost;
  }

  action_state_t* new_state() override
  {
    return new state_t( this, target );
  }

  double cost() const override
  {
    // double c = holy_power_consumer_t::cost();
    // It costs 5 for Ret, 3 for Prot
    double c = p()->specialization() == PALADIN_RETRIBUTION ? ret_cost : prot_cost;

    if ( p()->buffs.templar.hammer_of_light_free->up() )
      c *= 1.0 + p()->buffs.templar.hammer_of_light_free->value();
    if ( affected_by.divine_purpose_cost && p()->buffs.divine_purpose->check() )
      c = 0.0;

    return c;
  }

   bool target_ready( player_t* candidate_target ) override
   {
     if ( !p()->buffs.templar.hammer_of_light_ready->up() )
    {
      return false;
    }
    return paladin_melee_attack_t::target_ready( candidate_target );
   }

   void execute() override
   {
     if ( p()->specialization() == PALADIN_RETRIBUTION && p()->talents.templar.undisputed_ruling->ok() &&
          p()->talents.greater_judgment->ok() )
     {
       auto tl = target_list();
       if ( tl.size() )
        p()->trigger_greater_judgment( td( tl[ 0 ] ) );
     }
     holy_power_consumer_t<paladin_melee_attack_t>::execute();
     auto state    = static_cast<state_t*>( cleave_hammer->get_state() );
     state->target = execute_state->target;
     state->divine_purpose_mult =
         p()->buffs.divine_purpose->up() ? p()->spells.divine_purpose_buff->effectN( 2 ).percent() : 0.0;
     cleave_hammer->schedule_execute( state );

     if ( p()->buffs.templar.hammer_of_light_free->up() )
     {
       p()->buffs.templar.hammer_of_light_free->expire();
     }
     p()->buffs.templar.hammer_of_light_ready->decrement();
     p()->trigger_lights_deliverance();
     if ( p()->talents.templar.zealous_vindication->ok() )
     {
       p()->trigger_empyrean_hammer( target, 2, 0_ms, false );
     }
     p()->trigger_empyrean_hammer(
         target, as<int>( p()->talents.templar.lights_guidance->effectN( 2 ).base_value() ),
         timespan_t::from_millis( p()->talents.templar.lights_guidance->effectN( 4 ).base_value() ), true );

     if ( p()->talents.templar.sacrosanct_crusade->ok() )
     {
       int heal_percent_effect               = p()->specialization() == PALADIN_RETRIBUTION ? 5 : 2;
       int additional_heal_per_target_effect = p()->specialization() == PALADIN_RETRIBUTION ? 6 : 3;

       double heal_percent = p()->talents.templar.sacrosanct_crusade->effectN( heal_percent_effect ).percent();
       double additional_heal_per_target =
           p()->talents.templar.sacrosanct_crusade->effectN( additional_heal_per_target_effect ).percent();

       double modifier = heal_percent + std::min( as<int>( p()->sim->target_non_sleeping_list.size() ), 5 ) *
                                            additional_heal_per_target;
       double health                                    = p()->resources.max[ RESOURCE_HEALTH ] * modifier;
       p()->active.sacrosanct_crusade_heal->base_dd_min = p()->active.sacrosanct_crusade_heal->base_dd_max = health;
       p()->active.sacrosanct_crusade_heal->execute();
     }
     if ( p()->specialization() == PALADIN_PROTECTION )
     {
       // Cons has a 400ms delay, for whatever reasons
       make_event<delayed_execute_event_t>( *sim, p(), p()->active.hammer_of_light_cons, execute_state->target,
                                            400_ms );
       p()->buffs.shield_of_the_righteous->execute();
     }
     if ( p()->buffs.empyrean_legacy->up() )
     {
       p()->active.empyrean_legacy->schedule_execute();
       p()->buffs.empyrean_legacy->expire();
     }
   }
   void impact( action_state_t* s ) override
   {
     holy_power_consumer_t<paladin_melee_attack_t>::impact( s );

     if ( p()->talents.templar.undisputed_ruling->ok() )
       p()->buffs.templar.undisputed_ruling->execute();
   }
};

// Empyrean Hammer
struct empyrean_hammer_wd_t : public paladin_spell_t
{
  empyrean_hammer_wd_t( paladin_t* p )
    : paladin_spell_t( "empyrean_hammer_wd", p, p->spells.templar.empyrean_hammer_wd )
  {
    background             = true;
    may_crit               = false;
    aoe                    = -1;
    target_filter_callback = secondary_targets_only();

    // ToDo (Fluttershy)
    // This spell currently deals full damage to all targets, even above 20.
    // SimC automatically reduces AoE damage above 20 targets, so may need custom execute, if this behaviour stays
    reduced_aoe_targets = -1;
  }

  void impact(action_state_t* s) override
  {
    paladin_spell_t::impact( s );
    if ( s->result == RESULT_HIT )
      p()->get_target_data( s->target )->debuff.empyrean_hammer->execute();
  }
};

struct empyrean_hammer_t : public paladin_spell_t
{
  empyrean_hammer_wd_t* wd;
  double wrathful_descent_multiplier;
  empyrean_hammer_t( paladin_t* p )
    : paladin_spell_t( "empyrean_hammer", p, p->spells.templar.empyrean_hammer ),
      wd( nullptr ),
      wrathful_descent_multiplier(1.0)
  {
    background = proc = may_crit = true;
    may_miss                     = false;
    if ( p->talents.templar.wrathful_descent->ok())
    {
      wd = new empyrean_hammer_wd_t( p );
      add_child( wd );
      wrathful_descent_multiplier = p->talents.templar.wrathful_descent->effectN( 2 ).percent();
    }
  }

  void execute() override
  {
    paladin_spell_t::execute();
    if ( p()->talents.templar.lights_deliverance->ok() )
    {
      p()->buffs.templar.lights_deliverance->trigger();
    }
  }

  double composite_target_multiplier( player_t *t ) const override
  {
    double ctm = paladin_spell_t::composite_target_multiplier( t );

    paladin_td_t* td = this->td( t );
    if ( p()->talents.burn_to_ash->ok() && td->dot.truths_wake->is_ticking() )
      ctm *= 1.0 + p()->talents.burn_to_ash->effectN( 2 ).percent();

    return ctm;
  }

  void impact(action_state_t* s) override
  {
    paladin_spell_t::impact( s );
    if ( p()->talents.templar.wrathful_descent->ok() && s->result == RESULT_CRIT && !wd->target_list().empty() )
    {
      wd->base_dd_min = wd->base_dd_max = wrathful_descent_multiplier * s->result_total;
      wd->execute_on_target( target );
      p()->get_target_data( s->target )->debuff.empyrean_hammer->execute();
    }

    if ( ( s->result == RESULT_CRIT && p()->talents.templar.lights_judicator->ok() &&
           p()->rng().roll( p()->talents.templar.lights_judicator->proc_chance() ) ) )
    {
      p()->buffs.templar.lights_deliverance->trigger();
      p()->procs.templar_lights_judicator->occur();
    }
  }
};

struct divine_hammer_tick_t : public paladin_melee_attack_t
{
  divine_hammer_tick_t( paladin_t* p ) : paladin_melee_attack_t( "divine_hammer_tick", p, p->find_spell( 198137 ) )
  {
    aoe                 = -1;
    reduced_aoe_targets = 8;  // does not appear to have a spelldata equivalent
    direct_tick         = true;
    background          = true;
    may_crit            = true;
    clears_judgment     = false;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double ctm = paladin_melee_attack_t::composite_target_multiplier( target );

    paladin_td_t* td = this->td( target );
    if ( p()->talents.burn_to_ash->ok() && td->dot.truths_wake->is_ticking() )
    {
      ctm *= 1.0 + p()->talents.burn_to_ash->effectN( 2 ).percent();
    }

    return ctm;
  }
};
void paladin_t::trigger_empyrean_hammer( player_t* target, int number_to_trigger, timespan_t delay,
                                         bool random_after_first )
{
  player_t* next_target = target;
  timespan_t totalDelay      = delay;
  timespan_t additionalDelay = timespan_t::from_millis( talents.templar.lights_guidance->effectN( 3 ).base_value() );
  for ( int i = 0; i < number_to_trigger; i++ )
  {
    if ( ( i > 0 && random_after_first ) || target == nullptr )
      next_target = *rng().range( sim->target_non_sleeping_list.begin(), sim->target_non_sleeping_list.end() );
    make_event<delayed_execute_event_t>( *sim, this, active.empyrean_hammer, next_target, totalDelay );
    totalDelay += additionalDelay;
  }
}

void paladin_t::trigger_lights_deliverance()
{
  if ( !talents.templar.lights_deliverance->ok() || !buffs.templar.lights_deliverance->at_max_stacks() )
    return;

  // Light's Deliverance does not trigger while DT/Wake cooldown is ready to be used
  if ( ( specialization() == PALADIN_PROTECTION && cooldowns.divine_toll->up() ) ||
       ( specialization() == PALADIN_RETRIBUTION && cooldowns.wake_of_ashes->up() ) )
    return;

  if ( buffs.templar.hammer_of_light_ready->at_max_stacks() )
  return;

  buffs.templar.hammer_of_light_free->execute();
  buffs.templar.hammer_of_light_ready->trigger( 1 );
  buffs.templar.lights_deliverance->expire();
}

// Holy Armaments
// Sacred Weapon Driver
struct sacred_weapon_proc_damage_t : public paladin_spell_t
{
  sacred_weapon_proc_damage_t( paladin_t* p ) : paladin_spell_t( "sacred_weapon_proc_damage", p, p->find_spell( 432616 ) )
  {
    background          = true;
    callbacks           = false;
    aoe                 = -1;
    reduced_aoe_targets = 5;
  }

  void execute() override
  {
    paladin_spell_t::execute();
    double chance = p()->reflection_of_radiance_proc_chance;
    if ( p()->options.fake_solidarity )
      chance = 1.0 - ( std::pow( 1.0 - chance, p()->buffs.lightsmith.fake_solidarity->stack() + 1 ) );
    if ( p()->talents.lightsmith.reflection_of_radiance->ok() && p()->rng().roll( chance ) )
    {
      p()->trigger_grand_crusader( GC_ROR );
      p()->procs.grand_crusader_ror_sw->occur();
    }
  }

  double composite_aoe_multiplier(const action_state_t* state) const override
  {
    double m = paladin_spell_t::composite_aoe_multiplier( state );
    // If Sacred Weapon hits only 1 target, it's damage is increased by 50%
    if ( state->n_targets == 1 )
      m *= 1.5;
    return m;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = paladin_spell_t::composite_da_multiplier( s );
    // If we're faking Solidarity, we double the amount
    if ( p()->talents.lightsmith.solidarity->ok() && p()->options.fake_solidarity )
      m *= 1.0 + p()->buffs.lightsmith.fake_solidarity->stack();
    return m;
  }
};

struct lesser_weapon_proc_damage_t :public paladin_spell_t
{
  lesser_weapon_proc_damage_t(paladin_t* p) : paladin_spell_t("lesser_weapon_proc_damage", p, p->find_spell(1239282))
  {
    background = true;
    callbacks=false;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = paladin_spell_t::composite_da_multiplier( s );
    if (p()->options.fake_solidarity)
    {
      m *= 1.0 + p()->fake_lesser_weapon_set.size() - 1.0;
    }
    return m;
  }
};

struct sacred_weapon_proc_heal_t : public paladin_heal_t
{
  sacred_weapon_proc_heal_t( paladin_t* p ) : paladin_heal_t( "sacred_weapon_proc_heal", p, p->find_spell( 441590 ) )
  {
    background          = true;
    callbacks           = false;
    aoe                 = 5;
    harmful             = false;
  }

  double composite_aoe_multiplier( const action_state_t* state ) const override
  {
    double m = paladin_heal_t::composite_aoe_multiplier( state );
    // If Sacred Weapon heal hits only 1 target, it's healing is increased by 100%
    if ( state->n_targets == 1 )
      m *= 2.0;
    return m;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = paladin_heal_t::composite_da_multiplier( s );
    // If we're faking Solidarity, we double the amount
    if ( p()->talents.lightsmith.solidarity->ok() && p()->options.fake_solidarity )
      m *= 1.0 + p()->buffs.lightsmith.fake_solidarity->stack();
    return m;
  }
};

struct lesser_weapon_proc_heal_t :public paladin_heal_t
{
  lesser_weapon_proc_heal_t( paladin_t* p ) : paladin_heal_t( "lesser_weapon_proc_heal", p, p->find_spell( 1239276 ) )
  {
    background = true;
    callbacks  = false;
    harmful    = false;
  }
};

struct sacred_weapon_cb_t : public dbc_proc_callback_t
{
  paladin_t* p;
  sacred_weapon_cb_t( player_t* player, paladin_t* paladin, const special_effect_t& effect )
    : dbc_proc_callback_t( player, effect )
  {
    p = paladin;
  }

  void execute( const spell_data_t*, player_t* t, action_state_t* ) override
  {
    if ( t->is_enemy() )
    {
      p->active.sacred_weapon_proc_damage->execute_on_target( t );
    }
    else
    {
      p->active.sacred_weapon_proc_heal->execute_on_target( t );
    }
  }
};

struct lesser_weapon_cb_t : public dbc_proc_callback_t
{
  paladin_t* p;
  player_t* player;
  int index;
  lesser_weapon_cb_t( player_t* pl, paladin_t* paladin, const special_effect_t& effect, int idx = 0 )
    : dbc_proc_callback_t( pl, effect )
  {
    p = paladin;
    player = pl;
    index  = idx;
  }
  void execute( const spell_data_t*, player_t* t, action_state_t* ) override
  {
    if (t->is_enemy())
    {
      p->active.lesser_weapon_proc_damage->execute_on_target( t );
    }
    else
    {
      p->active.lesser_weapon_proc_heal->execute_on_target( t );
    }
    if (p == player)
    {
      if (p->options.fake_solidarity)
      {
        for (auto it = p->fake_lesser_weapon_set.begin(); it != p->fake_lesser_weapon_set.end(); )
        {
          *it = *it - 1;
          if ( *it <= 0 )
          {
            it = p->fake_lesser_weapon_set.erase( it );
            if ( p->fake_lesser_weapon_set.size() <= 0 )
              p->buffs.lightsmith.lesser_weapon->expire();
          }
          else
          {
            it++;
          }
        }
      }
      else
      {
        p->buffs.lightsmith.lesser_weapon->decrement();
      }
    }
    else
    {
      p->get_target_data( player )->buffs.lesser_weapon->decrement();
    }
  }
};

// Sacred Weapon Buff
struct sacred_weapon_t : public paladin_spell_t
{
  sacred_weapon_t( paladin_t* p ) : paladin_spell_t( "sacred_weapon", p )
  {
    harmful = false;
  }

  void execute() override
  {
    paladin_spell_t::execute();
    if ( p() != target && !target->is_enemy() )
      p()->get_target_data( target )->buffs.sacred_weapon->execute();
    else
      p()->buffs.lightsmith.sacred_weapon->execute();
    sim->print_debug( "{} executes Holy Armament Sacred Weapon on {}", p()->name(), target->name() );
  }
};

struct holy_bulwark_t : public paladin_spell_t
{
  holy_bulwark_t( paladin_t* p ) : paladin_spell_t( "holy_bulwark", p )
  {
    harmful = false;
  }
  void execute() override
  {
    paladin_spell_t::execute();
    if ( p() != target && !target->is_enemy() )
      p()->get_target_data( target )->buffs.holy_bulwark->trigger();
    else
      p()->buffs.lightsmith.holy_bulwark->trigger();
    sim->print_debug( "{} executes Holy Armament Holy Bulwark on {}", p()->name(), target->name() );
  }
};

// Holy Armaments

struct holy_armaments_t : public paladin_spell_t
{
  holy_armaments_t( paladin_t* p, util::string_view options_str )
    : paladin_spell_t( "holy_armaments", p, p->find_spell( 432459 ) )
  {
    parse_options( options_str );
    harmful            = false;
    name_str_reporting = "Holy Armaments";
    background = !p->lightsmith();
  }

  timespan_t execute_time() const override
  {
    return p()->active.armament[ p()->next_armament ]->execute_time();
  }
  void execute() override
  {
    paladin_spell_t::execute();
    p()->cast_holy_armaments( execute_state->target->is_enemy() ? p() : execute_state->target, p()->next_armament, LS_HARDCAST );
  }
};

void paladin_t::cast_holy_armaments( player_t* target, armament usedArmament, armament_source src )
{
  auto nextArmament = active.armament[ usedArmament ];
  bool changeArmament = src == LS_HARDCAST;
  bool random         = src == LS_DIVINE_INSPIRATION;

  // Random is not truly random. Starting weapon is semi-random-ish (It's always the opposite from the last and does not reset on combat start)
  // So we just rng the first one
  if (random)
  {
    if ( divine_inspiration_next == -1 )
      divine_inspiration_next = rng().range( 2 );
    usedArmament = (armament)divine_inspiration_next;
    nextArmament = active.armament[ usedArmament ];
  }

  nextArmament->execute_on_target( target );
  sim->print_debug( "Player {} cast Holy Armaments on {}", name(), target->name() );

  if ( talents.lightsmith.solidarity->ok() && !options.fake_solidarity )
  {
    if ( target != this )
    {
      nextArmament->execute_on_target( this );
      sim->print_debug( "Player {} cast Holy Armaments on self via Solidarity", name() );
    }
    else if ( sim->player_non_sleeping_list.size() > 1 )
    {
      // We try to do this twice. In case every target is invalid, we just take the first target we find.
      for ( int i = 0; i < 2; i++ )
      {
        player_t* first_dps    = nullptr;
        player_t* first_healer = nullptr;
        player_t* first_tank   = nullptr;
        for ( auto& _p : sim->player_no_pet_list )
        {
          if ( _p->is_sleeping() || _p == this )
            continue;

          // Random targetting prefers targets without a buff. Only try to find a valid target on the first iteration.
          if ( i == 0 )
          {
            if ( ( usedArmament == SACRED_WEAPON && get_target_data( _p )->buffs.sacred_weapon->up() ) ||
                 ( usedArmament == HOLY_BULWARK && get_target_data( _p )->buffs.holy_bulwark->up() ) )
              continue;
          }

          switch ( _p->role )
          {
            case ROLE_HEAL:
              if ( first_healer == nullptr )
                first_healer = _p;
              break;
            case ROLE_TANK:
              if ( first_tank == nullptr )
                first_tank = _p;
              break;
            default:
              if ( first_dps == nullptr )
                first_dps = _p;
              break;
          }
        }
        if ( first_dps != nullptr )
          random_weapon_target = first_dps;
        else if ( first_healer != nullptr )
          random_weapon_target = first_healer;
        else
          random_weapon_target = first_tank;

        if ( first_tank != nullptr )
          random_bulwark_target = first_tank;
        else if ( first_healer != nullptr )
          random_bulwark_target = first_healer;
        else
          random_bulwark_target = first_dps;
      }
      if ( random_weapon_target != nullptr )
      {
        if ( usedArmament == SACRED_WEAPON )
        {
          nextArmament->execute_on_target( random_weapon_target );
          sim->print_debug( "Player {} cast Holy Armaments (Sacred Weapon) on {} via Solidarity", name(),
                            random_weapon_target->name() );
        }
        else
        {
          nextArmament->execute_on_target( random_bulwark_target );
          sim->print_debug( "Player {} cast Holy Armaments (Holy Bulwark) on {} via Solidarity", name(),
                            random_bulwark_target->name() );
        }
      }
    }
  }

  if (options.fake_solidarity)
  {
    if ( usedArmament == SACRED_WEAPON )
      buffs.lightsmith.fake_solidarity->trigger();
    else
      buffs.lightsmith.fake_solidarity_bulwark->trigger();
  }
  if ( talents.lightsmith.masterwork->ok() && src != LS_DIVINE_INSPIRATION )
  {
    int amount = as<int>( talents.lightsmith.masterwork->effectN( 1 ).base_value() );
    if ( usedArmament == HOLY_BULWARK )
      buffs.lightsmith.masterwork_bulwark->trigger( amount );
    else
      buffs.lightsmith.masterwork_weapon->trigger( amount );
  }


  if ( changeArmament )
    next_armament = armament( ( next_armament + 1 ) % NUM_ARMAMENT );
  if ( random )
    divine_inspiration_next = (divine_inspiration_next + 1) % NUM_ARMAMENT;
}

void paladin_t::cast_lesser_armament(int amount, lesser_armament usedArmament)
{
  // Masterwork always prefers to go on other targets, because

  if ( amount > 0 && !options.fake_solidarity)
  {
    for ( int j = 0; j < 2; j++ )
    {
      for ( int i = 0; i < 3; i++ )
      {
        if ( amount == 0 )
          break;
        for ( auto& _p : sim->player_non_sleeping_list )
        {
          if ( amount == 0 )
            break;
          if ( ( i == 0 && _p->role == ROLE_ATTACK ) || ( i == 1 && _p->role == ROLE_HEAL ) ||
               ( i == 2 && _p->role == ROLE_TANK ) )
          {
            if ( _p != this )
            {
              if ( usedArmament == LESSER_WEAPON && ( j == 1 || !get_target_data( _p )->buffs.lesser_weapon->up() ) )
                get_target_data( _p )->buffs.lesser_weapon->trigger( 5 );
              else if ( usedArmament == LESSER_BULWARK &&
                        ( j == 1 || !get_target_data( _p )->buffs.lesser_bulwark->up() ) )
                get_target_data( _p )->buffs.lesser_bulwark->trigger();
            }
            else
            {
              if ( usedArmament == LESSER_WEAPON && ( j == 1 || !buffs.lightsmith.lesser_weapon->up() ) )
                buffs.lightsmith.lesser_weapon->trigger( 5 );
              else if ( usedArmament == LESSER_BULWARK && ( j == 1 || !buffs.lightsmith.lesser_bulwark->up() ) )
                buffs.lightsmith.lesser_bulwark->trigger();
            }
            amount--;
          }
        }
      }
    }
  }
  if ( amount > 0 && options.fake_solidarity)
  {
    if ( usedArmament == LESSER_BULWARK )
      buffs.lightsmith.lesser_bulwark->trigger();
    else
    {
      if ( !buffs.lightsmith.lesser_weapon->up() )
        buffs.lightsmith.lesser_weapon->trigger();

      for ( int i = 0; i < amount; i++ )
      {
        fake_lesser_weapon_set.push_back( 5 );
      }
    }
  }
}

dbc_proc_callback_t* paladin_t::create_sacred_weapon_callback( paladin_t* source, player_t* target )
{
  auto sacred_weapon_effect      = new special_effect_t( target );
  sacred_weapon_effect->name_str = "sacred_weapon_cb_" + source->name_str + "_" + target->name_str;
  sacred_weapon_effect->spell_id = 432502;
  sacred_weapon_effect->type     = SPECIAL_EFFECT_EQUIP;

  target->special_effects.push_back( sacred_weapon_effect );

  return new sacred_weapon_cb_t( target, source, *sacred_weapon_effect );
}

dbc_proc_callback_t* paladin_t::create_lesser_weapon_callback(paladin_t* source, player_t* target)
{
  auto lesser_weapon_effect = new special_effect_t( target );
  lesser_weapon_effect->name_str =
      fmt::format( "lesser_weapon_cb_{}_{}", source->name_str, target->name_str );
  lesser_weapon_effect->spell_id = 1239091;
  lesser_weapon_effect->type     = SPECIAL_EFFECT_EQUIP;


  target->special_effects.push_back( lesser_weapon_effect );
  return new lesser_weapon_cb_t( target, source, *lesser_weapon_effect, index );
}

void paladin_t::trigger_laying_down_arms()
{
  if ( !talents.lightsmith.laying_down_arms->ok() )
    return;

  if ( specialization() == PALADIN_PROTECTION )
  {
    buffs.shining_light_free->trigger();
  }
  else if (specialization() == PALADIN_HOLY)
  {
    buffs.infusion_of_light->trigger();
  }
  cooldowns.lay_on_hands->adjust( -talents.lightsmith.laying_down_arms->effectN( 1 ).time_value() );
};

struct eye_for_an_eye_t : public paladin_spell_t
{
  eye_for_an_eye_t( paladin_t* p ) : paladin_spell_t( "eye_for_an_eye", p, p->find_spell( 469311 ) )
  {
    background = true;
  }
};

// Shield of the Righteous ==================================================

shield_of_the_righteous_buff_t::shield_of_the_righteous_buff_t( paladin_t* p )
  : buff_t( p, "shield_of_the_righteous", p->spells.sotr_buff )
{
  add_invalidate( CACHE_BONUS_ARMOR );
  set_default_value_from_effect( 3 );
  this->set_refresh_duration_callback( []( const buff_t* b, timespan_t d ) {
    auto dur = b->remains() + d;
    if ( dur > b->base_buff_duration * 3 )
      dur = b->base_buff_duration * 3;
    return dur;
  } );
  set_refresh_behavior( buff_refresh_behavior::CUSTOM );
  cooldown->duration = 0_ms;  // handled by the ability
}

struct shield_of_the_righteous_t : public holy_power_consumer_t<paladin_melee_attack_t>
{
  struct forges_reckoning_t : public paladin_spell_t
  {
    forges_reckoning_t( paladin_t* p ) : paladin_spell_t( "forges_reckoning", p, p->spells.lightsmith.forges_reckoning )
    {
      background = proc = may_crit = true;
      may_miss                     = false;
    }
  };
  struct blaze_of_glory_t : public paladin_spell_t
  {
    blaze_of_glory_t( paladin_t* p ) : paladin_spell_t( "blaze_of_glory", p, p->spells.blaze_of_glory )
    {
      background             = true;
      aoe                    = as<int>( p->talents.glory_of_the_vanguard_3->effectN( 1 ).base_value() );
      target_filter_callback = secondary_targets_only();
    }
  };

  forges_reckoning_t* forges_reckoning;
  blaze_of_glory_t* blaze_of_glory;
  shield_of_the_righteous_t( paladin_t* p, util::string_view options_str )
    : holy_power_consumer_t( "shield_of_the_righteous", p, p->spec.shield_of_the_righteous ), forges_reckoning( nullptr )
  {
    parse_options( options_str );

    if ( !p->has_shield_equipped() )
    {
      sim->errorf( "%s: %s only usable with shield equipped in offhand\n", p->name(), name() );
      background = true;
    }

    aoe         = -1;
    use_off_gcd = is_sotr = true;

    // no weapon multiplier
    weapon_multiplier = 0.0;

    if ( p->talents.lightsmith.blessing_of_the_forge->ok() )
    {
      forges_reckoning = new forges_reckoning_t( p );
      add_child( forges_reckoning );
    }
    if (p->talents.glory_of_the_vanguard_3->ok())
    {
      blaze_of_glory = new blaze_of_glory_t( p );
      add_child( blaze_of_glory );
    }
  }

  void execute() override
  {
    bool hasDpUp = p()->buffs.divine_purpose->up();

    holy_power_consumer_t::execute();

    // Buff granted regardless of combat roll result
    // Duration and armor bonus recalculation handled in the buff
    p()->buffs.shield_of_the_righteous->trigger();

    if ( !background )
    {
      if ( p()->buffs.shining_light_stacks->at_max_stacks() )
      {
        p()->buffs.shining_light_stacks->expire();
        p()->buffs.shining_light_free->trigger();
      }
      else
        p()->buffs.shining_light_stacks->trigger();
    }

    if ( p()->buffs.lightsmith.blessing_of_the_forge->up() )
    {
      forges_reckoning->execute_on_target( target );
    }
    if (p()->buffs.valor->up())
    {
      blaze_of_glory->execute_on_target( target );
      p()->buffs.valor->expire();
    }
    if (p()->sets->has_set_bonus(PALADIN_PROTECTION, MID1, B4))
    {
      p()->buffs.light_blessed_shield->trigger();
    }

    // You will lose 2 Holy Power when using SotR with IotD talented when DP is up
    if (p()->bugs && p()->talents.instrument_of_the_divine->ok() && hasDpUp)
    {
      p()->resources.current[ RESOURCE_HOLY_POWER ] =
          std::max( p()->resources.current[ RESOURCE_HOLY_POWER ] - 2.0, 0.0 );
    }
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = paladin_melee_attack_t::composite_da_multiplier( state );
    if ( p()->buffs.valor->up() && state->chain_target == 0 )
    {
      m *= 1.0 + p()->buffs.valor->data().effectN( 1 ).percent();
    }
    return m;
  }
  double action_multiplier() const override
  {
    double am = paladin_melee_attack_t::action_multiplier();

    if (p()->talents.instrument_of_the_divine->ok() && cost() > 3.0)
    {
      double overflow = std::min( p()->talents.instrument_of_the_divine->effectN( 2 ).base_value(),
                                  cost() - 3.0 );
      am *= 1.0 + p()->talents.instrument_of_the_divine->effectN( 1 ).percent() * overflow;
    }
    return am;
  }
};

// TODO: friendly dawnlights
struct dawnlight_aoe_t : public paladin_spell_t
{
  dawnlight_aoe_t( paladin_t* p ) : paladin_spell_t( "dawnlight_aoe", p, p->find_spell( 431399 ) )
  {
    may_dodge = may_parry = may_miss = false;
    background                       = true;
    aoe                              = -1;
    reduced_aoe_targets              = p->spells.herald_of_the_sun.dawnlight_aoe_metadata->effectN( 1 ).base_value();
    target_filter_callback           = secondary_targets_only();
  }
};

struct dawnlight_t : public paladin_spell_t
{
  struct suns_avatar_dmg_t : public paladin_spell_t
  {
    suns_avatar_dmg_t( paladin_t* p ) : paladin_spell_t( "suns_avatar", p, p->find_spell( 431911 ) )
    {
      background          = true;
      aoe                 = -1;
      reduced_aoe_targets = p->talents.herald_of_the_sun.suns_avatar->effectN( 6 ).base_value();
    }
  };
  dawnlight_aoe_t* aoe_action;
  suns_avatar_dmg_t* suns_avatar;

  dawnlight_t( paladin_t* p )
    : paladin_spell_t( "dawnlight", p, p->find_spell( 431380 ) ),
      aoe_action( new dawnlight_aoe_t( p ) ),
      suns_avatar( new suns_avatar_dmg_t( p ) )
  {
    background                     = true;
    affected_by.highlords_judgment = true;
    tick_may_crit                  = true;
    dot_behavior                   = dot_behavior_e::DOT_EXTEND;  // per bolas test Aug 21 2024
  }

  void execute() override
  {
    paladin_spell_t::execute();

    if ( p()->talents.herald_of_the_sun.solar_grace->ok() )
      p()->buffs.herald_of_the_sun.solar_grace->trigger();

    if ( p()->buffs.herald_of_the_sun.morning_star->up() )
      p()->buffs.herald_of_the_sun.morning_star->expire();
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    double cpm = paladin_spell_t::composite_persistent_multiplier( s );

    if ( p()->buffs.herald_of_the_sun.morning_star->up() )
      cpm *= 1.0 + p()->buffs.herald_of_the_sun.morning_star->stack_value();

    return cpm;
  }

  void tick( dot_t* d ) override
  {
    paladin_spell_t::tick( d );

    aoe_action->base_dd_min = aoe_action->base_dd_max = d->state->result_amount * p()->spells.herald_of_the_sun.dawnlight_aoe_metadata->effectN( 1 ).percent();
    aoe_action->execute_on_target( d->target );
    if ( p()->talents.herald_of_the_sun.suns_avatar->ok() )
      suns_avatar->execute_on_target( d->target );
  }

  void last_tick( dot_t* d ) override
  {
    paladin_spell_t::last_tick( d );

    if ( p()->talents.herald_of_the_sun.lingering_radiance->ok() )
    {
      paladin_td_t* target_data = td( d->target );
      target_data->debuff.judgment->trigger();
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

namespace buffs
{
struct blessing_of_sacrifice_t : public absorb_buff_t
{
  paladin_t* source;  // Assumption: Only one paladin can cast HoS per target
  double absorb_pct;

  blessing_of_sacrifice_t( player_t* p )
    : absorb_buff_t( p, "blessing_of_sacrifice", p->find_spell( 6940 ) ),
      source( nullptr ),
      absorb_pct( data().effectN( 1 ).percent() )
  {}

  // Trigger function for the paladin applying HoS on the target
  bool trigger_hos( paladin_t& paladin )
  {
    if ( source )
      return false;

    source = &paladin;

    return absorb_buff_t::trigger( 1, paladin.resources.max[ RESOURCE_HEALTH ] );
  }

  double consume( double amount, action_state_t* s ) override
  {
    return absorb_buff_t::consume( amount * absorb_pct, s );
  }

  void absorb_used( double amount, player_t* ) override
  {
    source->active.blessing_of_sacrifice_redirect->trigger( amount );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    absorb_buff_t::expire_override( expiration_stacks, remaining_duration );

    source = nullptr;
  }

  void reset() override
  {
    absorb_buff_t::reset();

    source = nullptr;
  }
};

struct blessing_of_autumn_t : public buff_t
{
  bool affected_actions_initialized;
  std::vector<action_t*> affected_actions;

  blessing_of_autumn_t( player_t* p )
    : buff_t( p, "blessing_of_autumn", p->find_spell( 328622 ) ), affected_actions_initialized( false )
  {
    set_cooldown( 0_ms );
    set_default_value_from_effect( 1 );

    set_stack_change_callback( [ this ]( buff_t* /* b */, int /* old */, int new_ ) {
      if ( !affected_actions_initialized )
      {
        int label = data().effectN( 1 ).misc_value1();
        affected_actions.clear();
        for ( auto a : player->action_list )
        {
          if ( a->data().affected_by_label( label ) )
          {
            if ( range::find( affected_actions, a ) == affected_actions.end() )
            {
              affected_actions.push_back( a );
            }
          }
        }

        affected_actions_initialized = true;
      }

      update_cooldowns( new_ );
    } );
  }

  void update_cooldowns( int new_ )
  {
    assert( affected_actions_initialized );

    double recharge_rate_multiplier = 1.0 / ( 1 + default_value );
    for ( auto a : affected_actions )
    {
      if ( new_ > 0 )
      {
        a->dynamic_recharge_rate_multiplier *= recharge_rate_multiplier;
      }
      else
      {
        a->dynamic_recharge_rate_multiplier /= recharge_rate_multiplier;
      }
      if ( a->cooldown->action == a )
        a->cooldown->adjust_recharge_multiplier();
      if ( a->internal_cooldown->action == a )
        a->internal_cooldown->adjust_recharge_multiplier();
    }
  }

  void expire( timespan_t delay ) override
  {
    buff_t::expire( delay );
  }
};

}  // end namespace buffs
// ==========================================================================
// End Paladin Buffs, Part Deux
// ==========================================================================

// Blessing of Sacrifice execute function

void blessing_of_sacrifice_t::execute()
{
  paladin_spell_t::execute();

  // TODO: convert to dynamic buff creation
  auto* b = debug_cast<buffs::blessing_of_sacrifice_t*>( target->buffs.blessing_of_sacrifice );

  // TODO: confirm HoS applies before absorbs
  if ( !range::contains( target->absorb_priority, b->data().id() ) )
    target->absorb_priority.insert( target->absorb_priority.begin(), b->data().id() );

  b->trigger_hos( *p() );
}

// ==========================================================================
// Paladin Character Definition
// ==========================================================================

paladin_td_t::paladin_td_t( player_t* target, paladin_t* paladin ) : actor_target_data_t( target, paladin )
{
  debuff.blessed_hammer = make_buff( *this, "blessed_hammer", paladin->find_spell( 204301 ) );
  debuff.execution_sentence = make_buff<buffs::execution_sentence_debuff_t>( this );
  debuff.execution_sentence_gather = make_buff( *this, "execution_sentence_gather", paladin->find_spell( 1260251 ) );

  debuff.judgment              = make_buff( *this, "judgment", paladin->spells.judgment_debuff )
                                 ->set_default_value_from_effect( 1 )
                                 ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );

  debuff.sanctify          = make_buff( *this, "sanctify", paladin->spells.sanctify );
  debuff.crusaders_resolve     = make_buff( *this, "crusaders_resolve", paladin->find_spell( 383843 ) );
  debuff.empyrean_hammer = make_buff( *this, "empyrean_hammer", paladin->find_spell( 431625 ) );
  debuff.consecration          = make_buff( *this, "consecration", paladin->spells.consecration );
  debuff.seal_of_reprisal      = make_buff( *this, "seal_of_reprisal", paladin->spells.seal_of_reprisal );

  buffs.holy_bulwark = make_buff<buffs::holy_bulwark_buff_t>( this )
    ->set_cooldown( 0_s );
  buffs.sacred_weapon = make_buff( *this, "sacred_weapon_" + paladin->name_str + "_" + target->name_str, paladin->find_spell( 432502 ) );
  

  if ( !target->is_enemy() && target != paladin )
  {
    auto cb = paladin->create_sacred_weapon_callback( paladin, target );
    cb->activate_with_buff( buffs.sacred_weapon, true );
  }

  if (paladin->talents.lightsmith.masterwork->ok())
  {
    buffs.lesser_bulwark = make_buff<buffs::lesser_bulwark_buff_t>( this );
    buffs.lesser_weapon = make_buff( *this, "lesser_weapon_" + paladin->name_str + "_" + target->name_str,
                                      paladin->find_spell( 1239091 ) );
    if ( !target->is_enemy() && target != paladin )
    {
      auto cb = paladin->create_lesser_weapon_callback( paladin, target );
      cb->activate_with_buff( buffs.lesser_weapon, true );
    }
  }

  dot.expurgation = target->get_dot( "expurgation", paladin );
  dot.truths_wake = target->get_dot( "truths_wake", paladin );
  dot.dawnlight = target->get_dot( "dawnlight", paladin );
}

bool paladin_td_t::standing_in_consecration()
{
  paladin_t *p = static_cast<paladin_t*>(source);
  if ( !p->sim->distance_targeting_enabled ) {
    return !p->all_active_consecrations.empty();
  }

  // new
  for ( ground_aoe_event_t* active_cons : p->all_active_consecrations )
  {
    double distance = target->get_position_distance( active_cons -> params -> x(), active_cons -> params -> y() );

    // exit with true if we're in range of any one Cons center
    if ( distance <= p->find_spell( 81297 )->effectN( 1 ).radius() )
      return true;
  }

  return false;
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
  }
  // Ret
  else if ( specialization() == PALADIN_RETRIBUTION )
  {
    paladin_t::create_ret_actions();
    active.divine_toll     = new divine_toll_judgment_ret_t( this );
    active.divine_toll_how = new divine_toll_hammer_of_wrath_ret_t( this );
    active.divine_resonance_ret = new divine_resonance_judgment_t( this );
    active.divine_resonance_ret_how = new divine_resonance_hammer_of_wrath_t( this );
  }

  if ( talents.avenging_wrath->ok() )
  {
    active.background_avenging_wrath = new avenging_wrath_t( this );
  }

  // Hero Talents
  //Lightsmith
  if ( lightsmith() )
  {
    auto cb = create_sacred_weapon_callback(this, this);
    cb->activate_with_buff( buffs.lightsmith.sacred_weapon, true );
    active.sacred_weapon_proc_damage = new sacred_weapon_proc_damage_t( this );
    active.sacred_weapon_proc_heal   = new sacred_weapon_proc_heal_t( this );
    if ( talents.lightsmith.masterwork->ok() )
    {
      active.lesser_weapon_proc_damage = new lesser_weapon_proc_damage_t( this );
      active.lesser_weapon_proc_heal   = new lesser_weapon_proc_heal_t( this );
      auto cblw                        = create_lesser_weapon_callback( this, this );
      cblw->activate_with_buff( buffs.lightsmith.lesser_weapon );
    }
  }
  //Templar
  if (templar())
  {
    active.empyrean_hammer = new empyrean_hammer_t(this);
  }
  if (talents.templar.sacrosanct_crusade->ok())
  {
    active.sacrosanct_crusade_heal = new sacrosanct_crusade_heal_t( this );
  }
  if ( talents.templar.divine_hammer->ok() )
  {
    active.divine_hammer_tick = new divine_hammer_tick_t( this );
  }

  if ( herald_of_the_sun() )
  {
    active.dawnlight = new dawnlight_t( this );
  }

  active.shield_of_vengeance_damage = new shield_of_vengeance_proc_t( this );

  active.armament[ HOLY_BULWARK ]  = new holy_bulwark_t( this );
  active.armament[ SACRED_WEAPON ] = new sacred_weapon_t( this );

  if ( talents.eye_for_an_eye->ok() )
  {
    active.eye_for_an_eye = new eye_for_an_eye_t( this );
  }

  active.background_cons = new consecration_t( this, "blade_of_justice", BLADE_OF_JUSTICE );
  active.hammer_of_light_cons = new consecration_t( this, "hol", HAMMER_OF_LIGHT );

  player_t::create_actions();
}

// paladin_t::create_action =================================================

action_t* paladin_t::create_action( util::string_view name, util::string_view options_str )
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

  if ( name == "auto_attack" )
    return new auto_melee_attack_t( this, options_str );
  if ( name == "avenging_wrath" )
    return new avenging_wrath_t( this, options_str );
  if ( name == "blessing_of_protection" )
    return new blessing_of_protection_t( this, options_str );
  if ( name == "blinding_light" )
    return new blinding_light_t( this, options_str );
  if ( name == "consecration" )
    return new consecration_t( this, options_str );
  if ( name == "crusader_strike" )
    return new crusader_strike_t( this, options_str );
  if ( name == "divine_steed" )
    return new divine_steed_t( this, options_str );
  if ( name == "divine_shield" )
    return new divine_shield_t( this, options_str );
  if ( name == "blessing_of_sacrifice" )
    return new blessing_of_sacrifice_t( this, options_str );
  if ( name == "hammer_of_justice" )
    return new hammer_of_justice_t( this, options_str );
  if ( name == "rebuke" )
    return new rebuke_t( this, options_str );
  if ( name == "hand_of_reckoning" )
    return new hand_of_reckoning_t( this, options_str );
  if ( name == "flash_of_light" )
    return new flash_of_light_t( this, options_str );
  if ( name == "lay_on_hands" )
    return new lay_on_hands_t( this, options_str );
  if ( name == "hammer_of_wrath" )
  {
    if ( specialization() == PALADIN_PROTECTION )
      return new hammer_of_wrath_t( this, "hammer_of_wrath", options_str, find_spell( 1241413 ) );
    else if ( specialization() == PALADIN_RETRIBUTION )
      return new hammer_of_wrath_t( this, "hammer_of_wrath", options_str, find_spell( 24275 ) );
  }
  if ( name == "judgment" )
  {
    if ( specialization() == PALADIN_PROTECTION )
      return new judgment_t( this, "judgment", options_str, find_spell( 275779 ) );
    else if ( specialization() == PALADIN_RETRIBUTION )
      return new judgment_ret_t( this, "judgment", options_str, find_spell( 20271 ) );
  }
  if ( name == "devotion_aura" )
    return new devotion_aura_t( this, options_str );
  if ( name == "divine_toll" )
    return new divine_toll_t( this, options_str );
  if ( name == "shield_of_the_righteous" )
    return new shield_of_the_righteous_t( this, options_str );
  if ( name == "word_of_glory" )
    return new word_of_glory_t( this, options_str );
  if ( name == "holy_armaments" )
    return new holy_armaments_t( this, options_str );
  if ( name == "hammer_of_light" )
    return new hammer_of_light_t( this, options_str );
  if ( name == "rite_of_adjuration" )
    return new rite_of_adjuration_t( this, options_str );
  if ( name == "rite_of_sanctification" )
    return new rite_of_sanctification_t( this, options_str );
  if ( name == "shield_of_vengeance" )
    return new shield_of_vengeance_t( this, options_str );

  return player_t::create_action( name, options_str );
}

void paladin_t::trigger_forbearance( player_t* target )
{
  auto buff = debug_cast<buffs::forbearance_t*>( target->debuffs.forbearance );

  buff->paladin = this;

  timespan_t dur = buff->base_buff_duration;

  buff->trigger( dur );
}

int paladin_t::get_local_enemies( double distance ) const
{
  int num_nearby = 0;
  for ( auto* p : sim->target_non_sleeping_list )
  {
     if ( p->is_enemy() && get_player_distance( *p ) <= distance + p->combat_reach )
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

  base.attack_power_per_agility  = 0.0;
  base.attack_power_per_strength = 1.0;
  base.spell_power_per_intellect = 1.0;

  // Ignore mana for non-holy
  if ( specialization() != PALADIN_HOLY )
  {
    resources.active_resource[ RESOURCE_MANA ] = false;
  }

  player_t::init_base_stats();
}

// paladin_t::reset =========================================================

void paladin_t::reset()
{
  player_t::reset();

  active_consecration = nullptr;
  active_boj_cons = nullptr;
  all_active_consecrations.clear();
  active_aura         = nullptr;

  next_armament = SACRED_WEAPON;
  random_weapon_target = nullptr;
  random_bulwark_target = nullptr;
  divine_inspiration_next = -1;
  fake_lesser_weapon_set.clear();
}

// paladin_t::init_gains ====================================================

void paladin_t::init_gains()
{
  player_t::init_gains();

  // Mana
  gains.mana_beacon_of_light = get_gain( "beacon_of_light" );

  // Health
  gains.holy_shield        = get_gain( "holy_shield_absorb" );
  gains.bulwark_of_order   = get_gain( "bulwark_of_order_absorb" );
  gains.sacrosanct_crusade = get_gain( "sacrosanct_crusade_absorb" );
  gains.holy_bulwark       = get_gain( "holy_bulwark_absorb" );
  gains.lesser_bulwark     = get_gain( "lesser_bulwark_absorb" );

  // Holy Power
  gains.hp_templars_verdict_refund = get_gain( "templars_verdict_refund" );
  gains.judgment                   = get_gain( "judgment" );
  gains.hp_cs                      = get_gain( "crusader_strike" );
  gains.hp_divine_toll             = get_gain( "divine_toll" );
  gains.hp_crusading_strikes       = get_gain( "crusading_strikes" );
  gains.hp_glory_of_the_vanguard_2 = get_gain( "glory_of_the_vanguard" );
  gains.hp_walk_into_light            = get_gain( "walk_into_light" );

  gains.hp_judge_jury_and_executioner_refund = get_gain( "judge_jury_and_executioner_refund" );
}

// paladin_t::init_procs ====================================================

void paladin_t::init_procs()
{
  player_t::init_procs();

  procs.art_of_war        = get_proc( "Art of War" );
  procs.art_of_war_wasted = get_proc( "Art of War wasted" );
  procs.righteous_cause   = get_proc( "Righteous Cause" );
  procs.divine_purpose    = get_proc( "Divine Purpose" );
  procs.empyrean_power    = get_proc( "Empyrean Power" );

  procs.as_grand_crusader         = get_proc( "Avenger's Shield: Grand Crusader" );
  procs.as_grand_crusader_wasted  = get_proc( "Avenger's Shield: Grand Crusader wasted" );
  procs.divine_inspiration = get_proc( "Divine Inspiration" );

  procs.templar_lights_judicator = get_proc( "Templar Light's Judicator LD additional stacks" );

  procs.grand_crusader_ror_sw = get_proc( "Grand Crusader: Reflection of Radiance Sacred Weapon" );
  procs.grand_crusader_ror_hb = get_proc( "Grand Crusader: Reflection of Radiance Holy Bulwark" );
}

// paladin_t::init_scaling ==================================================

void paladin_t::init_scaling()
{
  player_t::init_scaling();

  switch ( specialization() )
  {
    case PALADIN_HOLY:
      scaling->enable( STAT_INTELLECT );
      scaling->enable( STAT_SPELL_POWER );
      break;
    case PALADIN_PROTECTION:
      scaling->enable( STAT_BONUS_ARMOR );
      break;
    default:
      break;
  }

  scaling->disable( STAT_AGILITY );
}

// paladin_t::init_assessors ================================================

void paladin_t::init_assessors()
{
  player_t::init_assessors();

  if ( talents.execution_sentence->ok() )
  {
    auto assessor_fn = [this]( result_amount_type /* rt */, action_state_t *s )
    {
      if ( this->buffs.execution_sentence->up() )
      {
        auto td = this->get_target_data( s->target );

        if ( td->debuff.execution_sentence_gather->check() && !dbc::is_school( s->action->school, SCHOOL_PHYSICAL ) )
        {
          this->accumulate_es_damage( s, 1.0 );
        }
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

  buffs.divine_steed =
      make_buff( this, "divine_steed", find_spell( "Divine Steed" ) )
          ->set_duration( 3_s )
          ->set_chance( 1.0 )
          ->set_cooldown( 0_ms )  // handled by the ability
          ->set_default_value(
              1.0 );  // TODO: change this to spellid 221883 & see if that automatically captures details

  // General
  buffs.avenging_wrath = make_buff( this, "avenging_wrath", spells.avenging_wrath )
                             ->add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER )
                             ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                             ->add_invalidate( CACHE_CRIT_CHANCE )
                             ->add_invalidate( CACHE_MASTERY )
                             ->set_expire_callback( [ this ]( buff_t*, double, timespan_t ) {
                               buffs.hammer_of_wrath->expire();
                               buffs.herald_of_the_sun.born_in_sunlight->expire();
                             } );

  if ( talents.crusade->ok() )
  {
    buffs.avenging_wrath->set_refresh_behavior( buff_refresh_behavior::DISABLED );
    buffs.avenging_wrath->set_initial_stack( 1 );
    buffs.avenging_wrath->set_max_stack( 10 );
    buffs.avenging_wrath->set_cooldown( 0_ms ); // Handled by the ability
    buffs.avenging_wrath->add_invalidate( CACHE_HASTE );
  }

  buffs.divine_purpose = make_buff( this, "divine_purpose", spells.divine_purpose_buff );
  buffs.divine_shield  = make_buff( this, "divine_shield", find_class_spell( "Divine Shield" ) )
                            ->set_cooldown( 0_ms );  // Let the ability handle the CD
  buffs.blessing_of_protection   = make_buff( this, "blessing_of_protection", find_spell( 1022 ) );
  buffs.blessing_of_spellwarding = make_buff( this, "blessing_of_spellwarding", find_spell( 204018 ) );
  buffs.strength_in_adversity    = make_buff( this, "strength_in_adversity", find_spell( 393071 ) )
                                    ->add_invalidate( CACHE_PARRY )
                                    ->set_default_value_from_effect( 1 )
                                    ->set_max_stack( 5 );  // Buff has no stacks, but can have up to 5 different values.

  buffs.shield_of_vengeance = new buffs::shield_of_vengeance_buff_t( this );
  buffs.devotion_aura = make_buff( this, "devotion_aura", find_spell( 465 ) )
                            ->set_default_value( find_spell( 465 )->effectN( 1 ).percent() );

  buffs.faiths_armor     = make_buff( this, "faiths_armor", find_spell( 379017 ) )
                           ->set_default_value_from_effect( 1 )
                           ->add_invalidate( CACHE_BONUS_ARMOR );

  if ( specialization() == PALADIN_PROTECTION )
    buffs.divine_resonance =
        make_buff( this, "divine_resonance", find_spell( 355455 ) )
            ->set_tick_callback( [ this ]( buff_t* /* b */, int /* stacks */, timespan_t /* tick_time */ ) {
              this->active.divine_resonance->set_target( this->target );
              this->active.divine_resonance->schedule_execute();
            } );
  else
  {
    buffs.divine_resonance = make_buff( this, "divine_resonance", find_spell( 1266308 ) )
                              ->set_initial_stack_to_max_stack();
  }

  buffs.hammer_of_wrath = make_buff( this, "hammer_of_wrath", find_spell( 1277026 ) );

  buffs.lightsmith.holy_bulwark =
      make_buff<buffs::holy_bulwark_buff_t>( this )
          ->set_cooldown( 0_s )
          ->set_refresh_duration_callback( [ this ]( const buff_t* b, timespan_t d ) {
            if ( b->remains().total_millis() > 0 )
              trigger_laying_down_arms();
            if ( is_ptr() )
            {
              if ( bugs )
                return d * 2;
              else
                return std::min( b->remains() + d, d * 2 );
            }
            else
            {
              timespan_t residual = std::min( d * 0.3, b->remains() );
              return residual + d;
            }
          } )
          ->set_expire_callback( [ this ]( buff_t*, double, timespan_t ) { trigger_laying_down_arms(); } );
  buffs.lightsmith.sacred_weapon =
      make_buff( this, "sacred_weapon", find_spell( 432502 ) )
          ->set_refresh_duration_callback( [ this ]( const buff_t* b, timespan_t d ) {
            if ( b->remains().total_millis() > 0 )
              trigger_laying_down_arms();
            if ( is_ptr() )
            {
              if ( bugs )
                return d * 2;
              else
                return std::min( b->remains() + d, d * 2 );
            }
            else
            {
              timespan_t residual = std::min( d * 0.3, b->remains() );
              return residual + d;
            }
          } )
          ->set_expire_callback( [ this ]( buff_t*, double, timespan_t ) { trigger_laying_down_arms(); } );
  buffs.lightsmith.masterwork_weapon = make_buff( this, "masterwork_weapon", find_spell( 1271436 ) );
  buffs.lightsmith.masterwork_bulwark = make_buff( this, "masterwork_bulwark", find_spell( 1271383 ) );
  // Not going to implement this "correctly", too much overhead for too little informational gain
  buffs.lightsmith.lesser_bulwark = make_buff<buffs::lesser_bulwark_buff_t>( this );
  buffs.lightsmith.lesser_weapon = make_buff( this, "lesser_weapon", find_spell( 1239091 ) );
  buffs.lightsmith.blessed_assurance =
      make_buff( this, "blessed_assurance", find_spell( 433019 ) )->set_default_value_from_effect( 1 );
  buffs.lightsmith.divine_guidance = make_buff( this, "divine_guidance", find_spell( 433106 ) )->set_max_stack( 5 );
  buffs.lightsmith.rite_of_sanctification = make_buff( this, "rite_of_sanctification", find_spell( 433550 ) )
                                                ->add_invalidate( CACHE_STRENGTH )
                                                ->add_invalidate( CACHE_ARMOR );
  buffs.lightsmith.rite_of_adjuration =
      make_buff( this, "rite_of_adjuration", find_spell( 433584 ) )->add_invalidate( CACHE_STAMINA );
  buffs.lightsmith.blessing_of_the_forge = make_buff( this, "blessing_of_the_forge", find_spell( 434132 ) )
                                               ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
                                                 if ( new_ )
                                                   cast_holy_armaments( this, armament::SACRED_WEAPON, LS_WINGS );
                                               } );
  buffs.lightsmith.fake_solidarity = make_buff( this, "fake_solidarity" )
                                         ->set_duration( buffs.lightsmith.sacred_weapon->base_buff_duration )
                                         ->set_chance( 1 )
                                         ->set_max_stack( 10 )
                                         ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );

  buffs.lightsmith.fake_solidarity_bulwark = make_buff( this, "fake_solidarity_bulwark" )
                                                 ->set_duration( buffs.lightsmith.holy_bulwark->base_buff_duration )
                                                 ->set_chance( 1 )
                                                 ->set_max_stack( 10 )
                                                 ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );

  buffs.templar.hammer_of_light_ready =
      make_buff( this, "hammer_of_light_ready", find_spell( 427441 ) )
          ->set_initial_stack_to_max_stack()
          ->set_expire_callback( [ this ]( buff_t*, double, timespan_t ) { trigger_lights_deliverance();
        });

  buffs.templar.hammer_of_light_free =
      make_buff( this, "hammer_of_light_free", find_spell( 433732 ) )->set_default_value_from_effect(1);

  buffs.templar.undisputed_ruling = make_buff( this, "undisputed_ruling", find_spell( 432629 ) )
                                        ->set_default_value_from_effect( 1 )
                                        ->add_invalidate( CACHE_HASTE );
  // Trigger first effect 2s after buff initially gets applied, then every 2 seconds after, unsure if it has a partial tick after it expires with extensions
  buffs.templar.shake_the_heavens = make_buff( this, "shake_the_heavens", find_spell( 431536 ) )
                                ->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
                                  this->trigger_empyrean_hammer( nullptr, 1, 0_ms );
                                        } )
                                        ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC )
                                        ->set_tick_behavior( buff_tick_behavior::REFRESH )
                                        ->set_partial_tick( true );
  buffs.templar.sanctification = make_buff( this, "sanctification", find_spell( 433671 ) )
                                     ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
                                     ->set_default_value_from_effect( 1 );

  buffs.templar.lights_deliverance    = make_buff( this, "lights_deliverance", find_spell( 433674 ) )
                                 ->set_stack_change_callback( [ this ]( buff_t* b, int, int ) {
                                   if ( b->at_max_stacks() )
                                   {
                                     trigger_lights_deliverance();
                                           }
                                 } );

  buffs.templar.sacrosanct_crusade = new buffs::sacrosanct_crusade_t( this );
  buffs.templar.divine_hammer      = make_buff( this, "divine_hammer", find_spell(198034) )
                                    ->set_tick_on_application( true )
                                    ->set_partial_tick( true )
                                    ->set_max_stack( 1 )
                                    ->set_default_value( 1.0 )
                                    ->set_period( timespan_t::from_millis( 2000 ) )
                                    ->set_freeze_stacks( true )
                                    ->set_tick_callback( [ this ]( buff_t*, int, const timespan_t& ) {
                                      active.divine_hammer_tick->schedule_execute();
                                    } );

  buffs.herald_of_the_sun.morning_star_driver = make_buff( this, "morning_star_driver", find_spell( 431568 ) )
    ->set_period( timespan_t::from_seconds( 5.0 ) ) // TODO(mserrano) grab from spell data
    ->set_quiet( true )
    ->set_tick_callback([this](buff_t*, int, const timespan_t&) { buffs.herald_of_the_sun.morning_star->trigger(); })
    ->set_tick_time_behavior( buff_tick_time_behavior::UNHASTED );
  buffs.herald_of_the_sun.morning_star = make_buff( this, "morning_star", find_spell( 431539 ) )
    ->set_default_value_from_effect( 1 );
  auto blessing_of_anshe_id = specialization() == PALADIN_RETRIBUTION ? 445206 : 445204;
  buffs.herald_of_the_sun.blessing_of_anshe = make_buff( this, "blessing_of_anshe", find_spell( blessing_of_anshe_id ) );
  buffs.herald_of_the_sun.solar_grace = make_buff( this, "solar_grace", find_spell( 439841 ) )
    -> add_invalidate( CACHE_HASTE )
    -> set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );
  buffs.herald_of_the_sun.dawnlight = make_buff( this, "dawnlight", find_spell( 431522 ) );
  buffs.vanguard = make_buff( this, "vanguard", find_spell( 1268810 ) );
  buffs.valor    = make_buff( this, "valor", find_spell( 1269179 ) );
  buffs.light_blessed_shield =
      make_buff( this, "light_blessed_shield", find_spell( 1272298 ) )->set_default_value_from_effect( 1 );
  buffs.herald_of_the_sun.born_in_sunlight =
      make_buff( this, "born_in_sunlight", find_spell( 1264050 ) )->set_default_value_from_effect( 1 );
}

// paladin_t::default_potion ================================================

std::string paladin_t::default_potion() const
{
  std::string retribution_pot = ( true_level > 80 ) ? "lights_potential_2" : "disabled";

  std::string protection_pot = ( true_level > 80 ) ? "lights_potential_2" : "disabled";

  std::string holy_dps_pot = ( true_level > 50 ) ? "spectral_intellect" : "disabled";

  std::string holy_pot = ( true_level > 50 ) ? "spectral_intellect" : "disabled";

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
  std::string retribution_food = ( true_level > 80 ) ? "royal_roast" : "disabled";

  std::string protection_food = ( true_level > 80 ) ? "blooming_feast" : "disabled";

  std::string holy_dps_food = ( true_level > 50 ) ? "feast_of_gluttonous_hedonism" : "disabled";

  std::string holy_food = ( true_level > 50 ) ? "feast_of_gluttonous_hedonism" : "disabled";

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
  std::string retribution_flask = ( true_level > 80 ) ? "flask_of_the_magisters_2" : "disabled";

  std::string protection_flask = ( true_level > 80 ) ? "flask_of_the_shattered_sun_2" : "disabled";

  std::string holy_dps_flask = ( true_level > 50 ) ? "spectral_flask_of_power" : "disabled";

  std::string holy_flask = ( true_level > 50 ) ? "spectral_flask_of_power" : "disabled";

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
  return ( true_level > 80 ) ? "void_touched" : "disabled";
}

// paladin_t::default_temporary_enchant ================================

std::string paladin_t::default_temporary_enchant() const
{
  switch ( specialization() )
  {
    case PALADIN_PROTECTION:
      return true_level < 81 ? "disabled" : "main_hand:thalassian_phoenix_oil_2,if=!(talent.rite_of_adjuration.enabled|talent.rite_of_sanctification.enabled)";
    case PALADIN_RETRIBUTION:
      return true_level < 81 ? "disabled" : "main_hand:thalassian_phoenix_oil_2";

    default:
      return "main_hand:howling_rune_3";
  }
}

// paladin_t::apply_action_effects ==========================================

void paladin_t::apply_action_effects( action_t* a ) {
  auto action = dynamic_cast<parse_action_base_t*>( a );
  assert( action );

  // Shared
  auto aw_effect_mask = effect_mask_t( true );
  if ( !talents.radiant_glory->ok() )
    aw_effect_mask.disable( 13 );
  if ( !talents.crusade->ok() )
    aw_effect_mask.disable( 11 );

  action->parse_effects( buffs.avenging_wrath, aw_effect_mask, IGNORE_STACKS );
  // TODO: add in Divine Purpose - logic here is going to be complex

  // Hero talents
  action->parse_effects( buffs.herald_of_the_sun.blessing_of_anshe, CONSUME_BUFF );
  action->parse_effects( buffs.templar.sanctification );
  action->parse_effects( buffs.herald_of_the_sun.born_in_sunlight );

  // Ret
  action->parse_effects( buffs.empyrean_power, CONSUME_BUFF );
  action->parse_effects( buffs.art_of_war, CONSUME_BUFF );
  action->parse_effects( buffs.righteous_cause, CONSUME_BUFF );

  // Prot
  action->parse_effects( buffs.sentinel );
  action->parse_effects( buffs.shining_light_free ); // Buff removal handling in holy_power_consumer_t
  action->parse_effects( buffs.bulwark_of_righteous_fury, CONSUME_BUFF );
  action->parse_effects( buffs.light_blessed_shield, CONSUME_BUFF );
}

void paladin_t::apply_target_action_effects(action_t* a)
{
  auto action = dynamic_cast<parse_action_base_t*>( a );
  assert( action );

  action->parse_target_effects( d_fn( &paladin_td_t::buffs_t::judgment, false ), spells.judgment_debuff );
  action->parse_target_effects( d_fn( &paladin_td_t::buffs_t::sanctify ), spells.sanctify );
  action->parse_target_effects( d_fn( &paladin_td_t::buffs_t::consecration ), spells.consecration );
  action->parse_target_effects( d_fn( &paladin_td_t::buffs_t::seal_of_reprisal ), spells.seal_of_reprisal );

  action->parse_target_effects( d_fn( &paladin_td_t::dots_t::expurgation ), spells.expurgation );
}

// paladin_t::init_actions ==================================================

void paladin_t::init_action_list()
{
  // sanity check - Prot/Ret can't do anything w/o main hand weapon equipped
  if ( main_hand_weapon.type == WEAPON_NONE &&
       ( specialization() == PALADIN_RETRIBUTION || specialization() == PALADIN_PROTECTION ) )
  {
    if ( !quiet )
      sim->errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
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
        generate_action_prio_list_ret();  // RET
        break;
      case PALADIN_PROTECTION:
        generate_action_prio_list_prot();  // PROT
        break;
      case PALADIN_HOLY:
        if ( primary_role() == ROLE_HEAL )
          generate_action_prio_list_holy();  // HOLY
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
    get_action_priority_list( "default" )->action_list_str = action_list_str;
    // clear action_list_str to avoid an assert error in player_t::init_actions()
    action_list_str.clear();
  }

  player_t::init_action_list();
}

void paladin_t::init_blizzard_action_list() {
  action_priority_list_t* default_ = get_action_priority_list( "default" );
  switch (specialization())
  {
    case PALADIN_RETRIBUTION:
    case PALADIN_PROTECTION:
      default_->add_action( "auto_attack" );
      break;
    default:
      assert( false );
      break;
  }
  player_t::init_blizzard_action_list();
}

// paladin_t::parse_assisted_combat_rule ==================================================

parsed_assisted_combat_rule_t paladin_t::parse_assisted_combat_rule( const assisted_combat_rule_data_t& rule,
                                                                     const assisted_combat_step_data_t& step ) const
{
  if ( rule.condition_type == AC_AURA_MISSING_PLAYER && rule.condition_value_1 == 188370 )
    return { "!consecration.up", true };

  if ( rule.condition_type == AC_AURA_ON_PLAYER && rule.condition_value_1 == 427441 )
    return { "(buff.hammer_of_light_ready.up|buff.hammer_of_light_free.up)", true };

  if ( rule.condition_type == AC_AURA_MISSING_PLAYER && rule.condition_value_1 == 427441 )
    return { "!(buff.hammer_of_light_ready.up|buff.hammer_of_light_free.up)", true };

  if ( step.spell_id == 53600 )
  {
    parsed_assisted_combat_rule_t derived_combat_rule = player_t::parse_assisted_combat_rule( rule, step );
    if ( !derived_combat_rule.expr.empty() )
    {
      derived_combat_rule.expr.append( "&!buff.hammer_of_light_ready.up" );
      derived_combat_rule.comment +=
          "Do not use SotR if Hammer of Light is ready, the One Button Rotation doesn't, either.";
    }
    return derived_combat_rule;
  }

  return player_t::parse_assisted_combat_rule( rule, step );
}

// paladin_t::action_names_from_spell_id ==================================================

std::vector<std::string> paladin_t::action_names_from_spell_id( unsigned int spell_id ) const
{
  if ( spell_id == 255937 )
    return { "wake_of_ashes", "hammer_of_light" };

  if ( spell_id == 387174 )
    return { "eye_of_tyr", "hammer_of_light" };

  if ( spell_id == 35395 )
    return { "crusader_strike", "hammer_of_the_righteous", "blessed_hammer", "templar_slash", "templar_strike" };

  return player_t::action_names_from_spell_id( spell_id );
}

// paladin_t::validate_fight_style ==========================================
bool paladin_t::validate_fight_style( fight_style_e style ) const
{
  if ( specialization() == PALADIN_PROTECTION )
  {
    switch ( style )
    {
      case FIGHT_STYLE_DUNGEON_ROUTE:
      case FIGHT_STYLE_DUNGEON_SLICE:
        return false;
      default:
        return true;
    }
  }
  return true;
}

// paladin_t::validate_actor ================================================
bool paladin_t::validate_actor()
{
  if ( specialization() == PALADIN_HOLY )
  {
    if ( !quiet )
      sim->error( "Holy Paladin for {} is not currently supported.", *this );
    return false;
  }

  return true;
}

void paladin_t::init_special_effects()
{
  player_t::init_special_effects();

  if ( talents.herald_of_the_sun.blessing_of_anshe->ok() )
  {
    struct blessing_of_anshe_cb_t : public dbc_proc_callback_t
    {
      paladin_t* p;

      blessing_of_anshe_cb_t( paladin_t* player, const special_effect_t& effect )
        : dbc_proc_callback_t( player, effect ), p( player )
      {
      }

      void execute( const spell_data_t*, player_t*, action_state_t* ) override
      {
        p->buffs.herald_of_the_sun.blessing_of_anshe->trigger();
      }
    };

    auto const blessing_of_anshe_driver = new special_effect_t( this );
    blessing_of_anshe_driver->name_str = "blessing_of_anshe_driver";
    blessing_of_anshe_driver->spell_id = 445200;
    special_effects.push_back( blessing_of_anshe_driver );

    auto cb = new blessing_of_anshe_cb_t( this, *blessing_of_anshe_driver );
    cb->initialize();
  }

  if ( talents.lightsmith.divine_inspiration->ok() )
  {
    struct divine_inspiration_cb_t : public dbc_proc_callback_t
    {
      paladin_t* p;

      divine_inspiration_cb_t( paladin_t* player, const special_effect_t& effect )
        : dbc_proc_callback_t( player, effect ), p( player )
      {
      }

      void execute( const spell_data_t*, player_t*, action_state_t* ) override
      {
        p->cast_holy_armaments( p, paladin::armament::SACRED_WEAPON, LS_DIVINE_INSPIRATION );
        p->procs.divine_inspiration->occur();
      }
    };

    auto const divine_inspiration_driver = new special_effect_t( this );
    divine_inspiration_driver->name_str  = "divine_inspiration_driver";
    divine_inspiration_driver->ppm_        = -.55;
    divine_inspiration_driver->rppm_scale_ = RPPM_HASTE;
    divine_inspiration_driver->type        = SPECIAL_EFFECT_EQUIP;
    divine_inspiration_driver->proc_flags_ =
        PF_MELEE_ABILITY | PF_RANGED | PF_RANGED_ABILITY | PF_NONE_HARMFUL | PF_MAGIC_SPELL | PF_ALL_HEAL;
    special_effects.push_back( divine_inspiration_driver );

    auto cb = new divine_inspiration_cb_t( this, *divine_inspiration_driver );
    cb->initialize();
  }
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
    sim->errorf( "%s is using an unsupported spec.", name() );
}

void paladin_t::init_spells()
{
  // Light Within (Retribution Apex Talent 2) modifies Avenging Wrath's effect 12, possibly via server side script
  if ( auto apex2 = find_talent_spell( talent_tree::SPECIALIZATION, 1261111 ); apex2.ok())
  {
    register_passive_effect_override( find_talent_spell( talent_tree::SPECIALIZATION, "Avenging Wrath" )->effectN( 12 ),
                                      apex2->effectN( 1 ).base_value() );
    register_passive_effect_override( find_spell( 454351 )->effectN( 12 ), apex2->effectN( 1 ).base_value() );
  }

  player_t::init_spells();

  init_spells_retribution();
  init_spells_protection();
  init_spells_holy();

  // Shared talents
  talents.lay_on_hands                    = find_talent_spell( talent_tree::CLASS, "Lay on Hands" );
  talents.auras_of_the_resolute           = find_talent_spell( talent_tree::CLASS, "Auras of the Resolute" );
  talents.hammer_of_wrath                 = find_talent_spell( talent_tree::CLASS, "Hammer of Wrath" );

  talents.cleanse_toxins                  = find_talent_spell( talent_tree::CLASS, "Cleanse Toxins" );
  talents.empyreal_ward                   = find_talent_spell( talent_tree::CLASS, "Empyreal Ward" );
  talents.fist_of_justice                 = find_talent_spell( talent_tree::CLASS, "Fist of Justice" );
  talents.blinding_light                  = find_talent_spell( talent_tree::CLASS, "Blinding Light" );
  talents.turn_evil                       = find_talent_spell( talent_tree::CLASS, "Turn Evil" );

  talents.a_just_reward                   = find_talent_spell( talent_tree::CLASS, "A Just Reward" );
  talents.afterimage                      = find_talent_spell( talent_tree::CLASS, "Afterimage" );
  talents.healing_hands                   = find_talent_spell( talent_tree::CLASS, "Healing Hands" );
  talents.guided_prayer                   = find_talent_spell( talent_tree::CLASS, "Guided Prayer" );
  talents.divine_steed                    = find_talent_spell( talent_tree::CLASS, "Divine Steed" );
  talents.lights_countenance              = find_talent_spell( talent_tree::CLASS, "Light's Countenance" );
  talents.greater_judgment                = find_talent_spell( talent_tree::CLASS, "Greater Judgment" );
  talents.wrench_evil                     = find_talent_spell( talent_tree::CLASS, "Wrench Evil" );
  talents.stand_against_evil              = find_talent_spell( talent_tree::CLASS, "Stand Against Evil" );

  talents.holy_reprieve                   = find_talent_spell( talent_tree::CLASS, "Holy Reprieve" );
  talents.shield_of_vengeance             = find_talent_spell( talent_tree::CLASS, "Shield of Vengeance" );
  talents.cavalier                        = find_talent_spell( talent_tree::CLASS, "Cavalier" );
  talents.divine_spurs                    = find_talent_spell( talent_tree::CLASS, "Divine Spurs" );
  talents.steed_of_liberty                = find_talent_spell( talent_tree::CLASS, "Steed of Liberty" );
  talents.blessing_of_freedom             = find_talent_spell( talent_tree::CLASS, "Blessing of Freedom" );
  talents.rebuke                          = find_talent_spell( talent_tree::CLASS, "Rebuke" );

  talents.obduracy                        = find_talent_spell( talent_tree::CLASS, "Obduracy" );
  talents.divine_toll                     = find_talent_spell( talent_tree::CLASS, "Divine Toll" );
  talents.unbound_freedom                 = find_talent_spell( talent_tree::CLASS, "Unbound Freedom" );
  talents.sanctified_plates               = find_talent_spell( talent_tree::CLASS, "Sanctified Plates" );
  talents.punishment                      = find_talent_spell( talent_tree::CLASS, "Punishment" );

  talents.divine_reach                    = find_talent_spell( talent_tree::CLASS, "Divine Reach" );
  talents.brought_to_light                = find_talent_spell( talent_tree::CLASS, "Brought to Light" );
  talents.blessing_of_sacrifice           = find_talent_spell( talent_tree::CLASS, "Blessing of Sacrifice" );
  talents.divine_resonance                = find_talent_spell( talent_tree::CLASS, "Divine Resonance" );
  talents.quickened_invocation            = find_talent_spell( talent_tree::CLASS, "Quickened Invocation" );
  talents.blessing_of_protection          = find_talent_spell( talent_tree::CLASS, "Blessing of Protection" );
  talents.fear_no_evil                    = find_talent_spell( talent_tree::CLASS, "Fear No Evil" );
  talents.consecrated_ground              = find_talent_spell( talent_tree::CLASS, "Consecrated Ground" );

  talents.holy_aegis                      = find_talent_spell( talent_tree::CLASS, "Holy Aegis" );
  talents.sacrifice_of_the_just           = find_talent_spell( talent_tree::CLASS, "Sacrifice of the Just" );
  talents.recompense                      = find_talent_spell( talent_tree::CLASS, "Recompense" );
  talents.sacred_strength                 = find_talent_spell( talent_tree::CLASS, "Sacred Strength" );
  talents.divine_purpose                  = find_talent_spell( talent_tree::CLASS, "Divine Purpose" );
  talents.improved_blessing_of_protection = find_talent_spell( talent_tree::CLASS, "Improved Blessing of Protection" );
  talents.unbreakable_spirit              = find_talent_spell( talent_tree::CLASS, "Unbreakable Spirit" );

  talents.lightforged_blessing            = find_talent_spell( talent_tree::CLASS, "Lightforged Blessing" );
  talents.lead_the_charge                 = find_talent_spell( talent_tree::CLASS, "Lead the Charge" );
  talents.worthy_sacrifice                = find_talent_spell( talent_tree::CLASS, "Worthy Sacrifice" );
  talents.righteous_protection            = find_talent_spell( talent_tree::CLASS, "Righteous Protection" );
  talents.holy_ritual                     = find_talent_spell( talent_tree::CLASS, "Holy Ritual");
  talents.blessed_calling                 = find_talent_spell( talent_tree::CLASS, "Blessed Calling" );
  talents.inspired_guard                  = find_talent_spell( talent_tree::CLASS, "Inspired Guard" );

  talents.faiths_armor                    = find_talent_spell( talent_tree::CLASS, "Faith's Armor" );
  talents.stoicism                        = find_talent_spell( talent_tree::CLASS, "Stoicism" );
  talents.seal_of_might                   = find_talent_spell( talent_tree::CLASS, "Seal of Might" );
  talents.vengeful_wrath                  = find_talent_spell( talent_tree::CLASS, "Vengeful Wrath" );
  talents.eye_for_an_eye                  = find_talent_spell( talent_tree::CLASS, "Eye for an Eye" );
  talents.golden_path                     = find_talent_spell( talent_tree::CLASS, "Golden Path" );
  talents.selfless_healer                 = find_talent_spell( talent_tree::CLASS, "Selfless Healer" );

  talents.blessing_of_dawn                = find_talent_spell( talent_tree::CLASS, "Blessing of Dawn" );
  talents.lightbearer                     = find_talent_spell( talent_tree::CLASS, "Lightbearer" );
  talents.blessing_of_dusk                = find_talent_spell( talent_tree::CLASS, "Blessing of Dusk" );

  // This is now in the Spec Tree for every Paladin
  talents.avenging_wrath = find_talent_spell( talent_tree::SPECIALIZATION, "Avenging Wrath" );

  // Hero Talents
  talents.lightsmith.holy_armaments         = find_talent_spell( talent_tree::HERO, "Holy Armaments" );

  talents.lightsmith.rite_of_sanctification = find_talent_spell( talent_tree::HERO, "Rite of Sanctification" );
  talents.lightsmith.rite_of_adjuration     = find_talent_spell( talent_tree::HERO, "Rite of Adjuration" );
  talents.lightsmith.solidarity             = find_talent_spell( talent_tree::HERO, "Solidarity" );
  talents.lightsmith.divine_guidance        = find_talent_spell( talent_tree::HERO, "Divine Guidance" );
  talents.lightsmith.blessed_assurance      = find_talent_spell( talent_tree::HERO, "Blessed Assurance" );
  talents.lightsmith.masterwork             = find_talent_spell( talent_tree::HERO, "Masterwork" );

  talents.lightsmith.laying_down_arms       = find_talent_spell( talent_tree::HERO, "Laying Down Arms" );
  talents.lightsmith.divine_inspiration     = find_talent_spell( talent_tree::HERO, "Divine Inspiration" );
  talents.lightsmith.forewarning            = find_talent_spell( talent_tree::HERO, "Forewarning" );
  talents.lightsmith.authoritative_rebuke   = find_talent_spell( talent_tree::HERO, "Authoritative Rebuke" );
  talents.lightsmith.tempered_in_battle     = find_talent_spell( talent_tree::HERO, "Tempered in Battle" );
  talents.lightsmith.hammer_and_anvil       = find_talent_spell( talent_tree::HERO, "Hammer and Anvil" );

  talents.lightsmith.shared_resolve         = find_talent_spell( talent_tree::HERO, "Shared Resolve" );
  talents.lightsmith.valiance               = find_talent_spell( talent_tree::HERO, "Valiance" );
  talents.lightsmith.reflection_of_radiance = find_talent_spell( talent_tree::HERO, "Reflection of Radiance" );
  talents.lightsmith.resounding_strike      = find_talent_spell( talent_tree::HERO, "Resounding Strike" );

  talents.lightsmith.blessing_of_the_forge  = find_talent_spell( talent_tree::HERO, "Blessing of the Forge" );

  talents.templar.lights_guidance         = find_talent_spell( talent_tree::HERO, "Light's Guidance" );

  talents.templar.zealous_vindication     = find_talent_spell( talent_tree::HERO, "Zealous Vindication" );
  talents.templar.shake_the_heavens       = find_talent_spell( talent_tree::HERO, "Shake the Heavens" );
  talents.templar.wrathful_descent        = find_talent_spell( talent_tree::HERO, "Wrathful Descent" );
  talents.templar.divine_hammer           = find_talent_spell( talent_tree::HERO, "Divine Hammer" );

  talents.templar.sacrosanct_crusade      = find_talent_spell( talent_tree::HERO, "Sacrosanct Crusade" );
  talents.templar.higher_calling          = find_talent_spell( talent_tree::HERO, "Higher Calling" );
  talents.templar.bonds_of_fellowship     = find_talent_spell( talent_tree::HERO, "Bonds of Fellowship" );
  talents.templar.unrelenting_charger     = find_talent_spell( talent_tree::HERO, "Unrelenting Charger" );
  talents.templar.lights_judicator        = find_talent_spell( talent_tree::HERO, "Light's Judicator" );

  talents.templar.endless_wrath           = find_talent_spell( talent_tree::HERO, "Endless Wrath" );
  talents.templar.sanctification          = find_talent_spell( talent_tree::HERO, "Sanctification" );
  talents.templar.hammerfall              = find_talent_spell( talent_tree::HERO, "Hammerfall" );
  talents.templar.undisputed_ruling       = find_talent_spell( talent_tree::HERO, "Undisputed Ruling" );
  talents.templar.divine_exaction         = find_talent_spell( talent_tree::HERO, "Divine Exaction" );
  talents.templar.seal_of_the_templar     = find_talent_spell( talent_tree::HERO, "Seal of the Templar" );

  talents.templar.lights_deliverance      = find_talent_spell( talent_tree::HERO, "Light's Deliverance" );

  talents.herald_of_the_sun.dawnlight     = find_talent_spell( talent_tree::HERO, "Dawnlight" );

  talents.herald_of_the_sun.morning_star       = find_talent_spell( talent_tree::HERO, "Morning Star");
  talents.herald_of_the_sun.gleaming_rays      = find_talent_spell( talent_tree::HERO, "Gleaming Rays" );
  talents.herald_of_the_sun.luminosity         = find_talent_spell( talent_tree::HERO, "Luminosity" );
  talents.herald_of_the_sun.endless_gleam      = find_talent_spell( talent_tree::HERO, "Endless Gleam" );

  talents.herald_of_the_sun.blessing_of_anshe  = find_talent_spell( talent_tree::HERO, "Blessing of An'she" );
  talents.herald_of_the_sun.lingering_radiance = find_talent_spell( talent_tree::HERO, "Lingering Radiance" );
  talents.herald_of_the_sun.sun_sear           = find_talent_spell( talent_tree::HERO, "Sun Sear" );
  talents.herald_of_the_sun.solar_grace        = find_talent_spell( talent_tree::HERO, "Solar Grace" );

  talents.herald_of_the_sun.aurora             = find_talent_spell( talent_tree::HERO, "Aurora" );
  talents.herald_of_the_sun.walk_into_light    = find_talent_spell( talent_tree::HERO, "Walk Into Light" );
  talents.herald_of_the_sun.second_sunrise     = find_talent_spell( talent_tree::HERO, "Second Sunrise" );
  talents.herald_of_the_sun.born_in_sunlight   = find_talent_spell( talent_tree::HERO, "Born in Sunlight" );

  talents.herald_of_the_sun.suns_avatar        = find_talent_spell( talent_tree::HERO, "Sun's Avatar" );

  // Shared Passives and spells
  passives.plate_specialization = find_specialization_spell( "Plate Specialization" );
  passives.paladin              = find_spell( 137026 );
  if ( talents.radiant_glory->ok() )
    spells.avenging_wrath = find_spell( 454351 );
  else
    spells.avenging_wrath = find_spell( 31884 );

  spec.word_of_glory_2          = find_rank_spell( "Word of Glory", "Rank 2" );
  spells.divine_purpose_buff    = find_spell( specialization() == PALADIN_RETRIBUTION ? 408458 : 223819 );
  spells.sanctify               = find_spell( 382538 );
  spells.consecration           = find_spell( 204242 );
  spells.seal_of_reprisal       = find_spell( 1302139 );

  // Hero Talent Spells
  spells.lightsmith.holy_bulwark        = find_spell( 432496 );
  spells.lightsmith.holy_bulwark_absorb = find_spell( 432607 );
  spells.lightsmith.forges_reckoning    = find_spell( 447258 );  // Child spell of blessing of the forge, triggered by casting shield of the righteous
  spells.lightsmith.sacred_word         = find_spell( 447246 ); // Child spell of blessing of the forge, triggered by casting Word of Glory
  spells.lightsmith.lesser_bulwark      = find_spell( 1239002 );
  spells.lightsmith.lesser_weapon       = find_spell( 1239091 );
  spells.templar.hammer_of_light_driver = find_spell( 427453 );
  spells.templar.hammer_of_light        = find_spell( 429826 );
  spells.templar.empyrean_hammer        = find_spell( 431398 );
  spells.templar.empyrean_hammer_wd     = find_spell( 431625 );

  spells.herald_of_the_sun.dawnlight_aoe_metadata = find_spell( 431581 );

  // Tier stuff
  spells.unrelenting_edict = find_spell( 1300662 );


  // Add Judgment AoE. Damage still handled manually. Hammer of Wrath also handled manually, since that AoE is 1, instead of 0
  register_passive_affect_list( talents.blessed_champion,
                                affect_list_t( 1 ).add_spell( 20271, 275773 ) );

  parse_all_class_passives();
  parse_all_passive_talents();
  parse_all_passive_sets();
  parse_raid_buffs();
  init_proc_data_entries();
}

void paladin_t::init_proc_data_entries()
{
  proc_data_entries.crusading_strikes_energize = spells.crusading_strikes_data;
}

// paladin_t::primary_role ==================================================

role_e paladin_t::primary_role() const
{
  if ( player_t::primary_role() != ROLE_NONE )
    return player_t::primary_role();

  if ( specialization() == PALADIN_RETRIBUTION )
    return ROLE_ATTACK;

  if ( specialization() == PALADIN_PROTECTION )
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
    switch ( s )
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

// paladin_t::composite_player_multiplier ===================================

double paladin_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  return m;
}

// paladin_t::composite_attribute_multiplier ================================

double paladin_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double m = player_t::composite_attribute_multiplier( attr );

  // Protection gets increased stamina
  if ( attr == ATTR_STAMINA )
  {
    // This literally never gets triggered. Apparently, invalidating the Stamina cache doesn't recalculate Stamina?
    if ( buffs.redoubt->up() )
      m *= 1.0 + buffs.redoubt->stack_value();

    if ( buffs.sentinel->up() )
      m *= 1.0 + buffs.sentinel->get_health_bonus();

    if ( buffs.lightsmith.rite_of_adjuration->up() )
      m *= 1.0 + buffs.lightsmith.rite_of_adjuration->data().effectN(1).percent();
  }

  if ( attr == ATTR_STRENGTH )
  {
    if ( buffs.redoubt->up() )
      // Applies to base str, gear str and buffs. So everything basically.
      m *= 1.0 + buffs.redoubt->stack_value();
    if ( buffs.lightsmith.rite_of_sanctification->up() )
      m *= 1.0 + buffs.lightsmith.rite_of_sanctification->data().effectN( 1 ).percent();
  }

  return m;
}

double paladin_t::composite_damage_versatility() const
{
  double cdv = player_t::composite_damage_versatility();

  return cdv;
}

double paladin_t::composite_heal_versatility() const
{
  double chv = player_t::composite_heal_versatility();

  return chv;
}

double paladin_t::composite_mitigation_versatility() const
{
  double cmv = player_t::composite_mitigation_versatility();

  return cmv;
}

double paladin_t::composite_mastery() const
{
  double m = player_t::composite_mastery();

  return m;
}

double paladin_t::composite_spell_crit_chance() const
{
  double h = player_t::composite_spell_crit_chance();

  if ( buffs.avenging_wrath->up() )
    h += buffs.avenging_wrath->data().effectN( 3 ).percent();

  if ( buffs.sentinel->up() )
    h += buffs.sentinel->get_crit_bonus();

  return h;
}

double paladin_t::composite_melee_crit_chance() const
{
  double h = player_t::composite_melee_crit_chance();

  if ( buffs.avenging_wrath->up() )
    h += buffs.avenging_wrath->data().effectN( 3 ).percent();

  if ( buffs.sentinel->up() )
    h += buffs.sentinel->get_crit_bonus();

  return h;
}

double paladin_t::composite_base_armor_multiplier() const
{
  double a = player_t::composite_base_armor_multiplier();
  if ( specialization() != PALADIN_PROTECTION )
    return a;

  if ( talents.sanctified_plates->ok() )
    a *= 1.0 + talents.sanctified_plates->effectN( 3 ).percent();

  if ( talents.faiths_armor->ok() && buffs.faiths_armor->up() )
    a *= 1.0 + buffs.faiths_armor->default_value;

  if ( buffs.lightsmith.rite_of_sanctification->up() )
    a *= 1.0 + buffs.lightsmith.rite_of_sanctification->data().effectN( 2 ).percent();

  return a;
}

double paladin_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double cptm      = player_t::composite_player_target_multiplier( target, school );

  if ( dbc::is_school( school, SCHOOL_HOLY ) )
  {
    if ( talents.holy_flames->ok() )
    {
      paladin_td_t* td = get_target_data( target );
      if ( td->dot.expurgation->is_ticking() )
      {
        cptm *= 1.0 + active.expurgation->data().effectN( 2 ).percent();
      }
    }
  }

  return cptm;
}

// paladin_t::composite_melee_haste =========================================

double paladin_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  if ( buffs.herald_of_the_sun.solar_grace->up() )
    h /= 1.0 + buffs.herald_of_the_sun.solar_grace->stack_value();

  if ( buffs.rush_of_light->up() )
    h /= 1.0 + talents.rush_of_light->effectN( 1 ).percent();

  if ( buffs.avenging_wrath->up() && talents.crusade->ok() )
    h /= 1.0 + (buffs.avenging_wrath->stack() * talents.crusade->effectN( 1 ).percent());

  if ( buffs.templar.undisputed_ruling->up() )
    h /= 1.0 + buffs.templar.undisputed_ruling->value();

  return h;
}

// paladin_t::composite_melee_auto_attack_speed =============================

double paladin_t::composite_melee_auto_attack_speed() const
{
  double s = player_t::composite_melee_auto_attack_speed();

  return s;
}

// paladin_t::composite_spell_haste ==========================================

double paladin_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( buffs.herald_of_the_sun.solar_grace->up() )
    h /= 1.0 + buffs.herald_of_the_sun.solar_grace->stack_value();

  if ( buffs.rush_of_light->up() )
    h /= 1.0 + talents.rush_of_light->effectN( 1 ).percent();

  if ( buffs.avenging_wrath->up() && talents.crusade->ok() )
    h /= 1.0 + (buffs.avenging_wrath->stack() * talents.crusade->effectN( 1 ).percent());

  if ( buffs.templar.undisputed_ruling->up() )
    h /= 1.0 + buffs.templar.undisputed_ruling->value();

  return h;
}

// paladin_t::composite_bonus_armor =========================================

double paladin_t::composite_bonus_armor() const
{
  double ba = player_t::composite_bonus_armor();

  if ( buffs.shield_of_the_righteous->check() )
  {
    double bonus = spells.sotr_buff->effectN( 1 ).percent() * cache.strength();

    ba += bonus;
  }
  return ba;
}

// paladin_t::composite_attack_power_multiplier =============================

double paladin_t::composite_attack_power_multiplier() const
{
  double ap = player_t::composite_attack_power_multiplier();

  // Mastery bonus is multiplicative with other effects
  if ( specialization() == PALADIN_PROTECTION )
  {
      //Note for future; If something changes with mastery, make sure you verify this to still be accurate
      ap *= 1.0 + cache.mastery() * mastery.divine_bulwark->effectN( 2 ).mastery_value();
  }

  return ap;
}

// paladin_t::composite_block ==========================================

double paladin_t::composite_block() const
{
  // this handles base block and and all block subject to diminishing returns
  double block_subject_to_dr = cache.mastery() * mastery.divine_bulwark->effectN( 1 ).mastery_value();
  double b                   = player_t::composite_block_dr( block_subject_to_dr );

  return b;
}

// paladin_t::composite_mitigation_multiplier =================================

double paladin_t::composite_mitigation_multiplier( const action_state_t* s, school_e school, bool direct ) const
{
  double m = player_t::composite_mitigation_multiplier( s, school, direct );

  // Passive sources
  m *= 1.0 + passives.sanctuary->effectN( 1 ).percent();
  m *= 1.0 + passives.aegis_of_light->effectN( 3 ).percent();

  // Damage Reduction Cooldowns
  if ( buffs.sentinel->up() )
  {
    m *= 1.0 + buffs.sentinel->get_damage_reduction_mod();
  }

  if ( buffs.guardian_of_ancient_kings->up() )
  {
    m *= 1.0 + buffs.guardian_of_ancient_kings->check_value();
  }

  if ( buffs.ardent_defender->up() )
  {
    m *= 1.0 + buffs.ardent_defender->check_value();
  }

  if ( buffs.divine_protection->up() )
  {
    m *= 1.0 + buffs.divine_protection->check_value();
  }

  if ( talents.blessing_of_dusk->ok() )
  {
    // ToDo Fluttershy: Fix or remove
    m *= 1.0 - talents.blessing_of_dusk->effectN( 1 ).percent();
  }

  if ( buffs.devotion_aura->up() )
  {
    double devoRed = buffs.devotion_aura->value();

    if ( talents.lightsmith.shared_resolve->ok() )
    {
      if ( buffs.lightsmith.sacred_weapon->up() )
        devoRed *= 1 + buffs.lightsmith.sacred_weapon->data().effectN( 1 ).percent();

      if ( buffs.lightsmith.holy_bulwark->up() )
        devoRed *= 1 + buffs.lightsmith.holy_bulwark->data().effectN( 1 ).percent();
    }

    m *= 1.0 + devoRed;
  }

  if ( buffs.shield_of_the_righteous->up() && spells.sotr_buff->effectN( 3 ).has_common_school( school ) )
  {
    m *= 1.0 + buffs.shield_of_the_righteous->check_value();
  }

  if ( specialization() == PALADIN_PROTECTION && standing_in_consecration() )
  {
    m *= 1.0 + spells.standing_in_consecration_buff->effectN( 3 ).percent();
  }

  return m;
}

// paladin_t::composite_mitigation_from_player_multiplier =====================

double paladin_t::composite_mitigation_from_player_multiplier( player_t* source, const action_state_t* s,
                                                               school_e school, bool direct ) const
{
  double m = player_t::composite_mitigation_from_player_multiplier( source, s, school, direct );

  if ( auto td = find_target_data( source ) )
  {
    if ( td->debuff.empyrean_hammer->up() )
    {
      m *= 1.0 + td->debuff.empyrean_hammer->data().effectN( 3 ).percent();
    }
  }

  return m;
}

// paladin_t::composite_crit_avoidance ========================================

double paladin_t::composite_crit_avoidance() const
{
  double c = player_t::composite_crit_avoidance();

  return c;
}

// paladin_t::composite_parry_rating ==========================================

double paladin_t::composite_parry_rating() const
{
  double p = player_t::composite_parry_rating();

  return p;
}

double paladin_t::composite_parry() const
{
  double p = player_t::composite_parry();

  if ( buffs.strength_in_adversity->up())
  {
    p += buffs.strength_in_adversity->value()*buffs.strength_in_adversity->stack();
  }

  return p;
}

// paladin_t::non_stacking_movement_modifier ================================

double paladin_t::non_stacking_movement_modifier() const
{
  double ms = player_t::non_stacking_movement_modifier();

  if ( buffs.divine_steed->up() )
  {
    // TODO: replace with commented version once we have spell data
    ms = std::max( buffs.divine_steed->value(), ms );
    // ms = std::max( buffs.divine_steed -> data().effectN( 1 ).percent(), ms );
  }

  return ms;
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

  if ( c == CACHE_MASTERY && mastery.divine_bulwark->ok() )
  {
    player_t::invalidate_cache( CACHE_BLOCK );
    player_t::invalidate_cache( CACHE_ATTACK_POWER );
    player_t::invalidate_cache( CACHE_SPELL_POWER );
  }

  if ( c == CACHE_STRENGTH && spec.shield_of_the_righteous->ok() )
  {
    player_t::invalidate_cache( CACHE_BONUS_ARMOR );
  }
}

// paladin_t::matching_gear_multiplier ======================================

double paladin_t::matching_gear_multiplier( attribute_e attr ) const
{
  return player_t::matching_gear_multiplier( attr );
}

// paladin_t::resource_gain =================================================

double paladin_t::resource_gain( resource_e resource_type, double amount, gain_t* source, action_t* action )
{
  double result = player_t::resource_gain( resource_type, amount, source, action );

  return result;
}

// paladin_t::resouce_loss ==================================================

double paladin_t::resource_loss( resource_e resource_type, double amount, gain_t* source, action_t* action )
{
  double result     = player_t::resource_loss( resource_type, amount, source, action );
  return result;
}

// paladin_t::assess_damage =================================================

void paladin_t::assess_damage( school_e school, result_amount_type dtype, action_state_t* s )
{
  if ( buffs.divine_shield->up() )
  {
    s->result_amount = 0;

    // Return out, as you don't get to benefit from anything else
    player_t::assess_damage( school, dtype, s );
    return;
  }

  if (buffs.blessing_of_protection->up() && school == SCHOOL_PHYSICAL)
  {
    s->result_amount = 0;

    // Return out, as you don't get to benefit from anything else
    player_t::assess_damage( school, dtype, s );
    return;
  }

  if ( buffs.blessing_of_spellwarding->up() && school != SCHOOL_PHYSICAL )
  {
    s->result_amount = 0;

    // Return out, as you don't get to benefit from anything else
    player_t::assess_damage( school, dtype, s );
    return;
  }

  if ( s->action->harmful && talents.eye_for_an_eye->ok() && s->action->player != this)
  {
    if (buffs.ardent_defender->up() || buffs.divine_protection->up() || buffs.divine_shield->up())
    {
      active.eye_for_an_eye->set_target( s->action->player );
      active.eye_for_an_eye->schedule_execute();
    }
  }

  // Trigger Grand Crusader on an avoidance event (TODO: test if it triggers on misses)
  if ( s->result == RESULT_DODGE || s->result == RESULT_PARRY || s->result == RESULT_MISS )
  {
    trigger_grand_crusader();
  }

  player_t::assess_damage( school, dtype, s );
}

// paladin_t::create_options ================================================

void paladin_t::create_options()
{
  // TODO: figure out a better solution for this.
  add_option( opt_bool( "paladin_fake_sov", options.fake_sov ) );
  add_option( opt_int( "min_dg_heal_targets", options.min_dg_heal_targets, 0, 5 ) );
  add_option( opt_int( "max_dg_heal_targets", options.max_dg_heal_targets, 0, 5 ) );
  add_option( opt_bool( "fake_solidarity", options.fake_solidarity ) );
  add_option( opt_float( "blessed_hammer_strikes", options.blessed_hammer_strikes, 1, 3 ) );
  add_option( opt_float( "ror_bulwark_additional_proc_chance", options.ror_bulwark_additional_proc_chance, 0, 1 ) );
  add_option( opt_string( "starting_armament", options.starting_armament ) );

  player_t::create_options();
}

// paladin_t::copy_from =====================================================

void paladin_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  options = debug_cast<paladin_t*>( source )->options;
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

  if ( options.starting_armament == "sacred_weapon" )
    next_armament = SACRED_WEAPON;
  // If option is set to gibberish, just roll
  else if ( options.starting_armament == "holy_bulwark" || sim->rng().roll( .5 ) )
    next_armament = HOLY_BULWARK;
  else
    next_armament = SACRED_WEAPON;

  if ( talents.herald_of_the_sun.morning_star->ok() )
  {
    buffs.herald_of_the_sun.morning_star_driver->trigger();
  }
}

bool paladin_t::standing_in_consecration() const
{
  if ( ! sim -> distance_targeting_enabled )
  {
    return ! all_active_consecrations.empty();
  }

  for ( ground_aoe_event_t* active_cons : all_active_consecrations )
  {
    double distance = get_position_distance( active_cons -> params -> x(), active_cons -> params -> y() );

    // exit with true if we're in range of any one Cons center
    if ( distance <= find_spell( 81297 )->effectN( 1 ).radius() )
      return true;
  }

  // if we're not in range of any of them
  return false;
}

// paladin_t::get_how_availability ==========================================

bool paladin_t::get_how_availability( ) const
{
  return wings_up() && talents.hammer_of_wrath->ok();
}

bool paladin_t::wings_up() const
{
  return buffs.avenging_wrath->up() || buffs.sentinel->up();
}

bool paladin_t::templar() const
{
  return has_hero_tree( hero_tree_e::HERO_TEMPLAR );
}
bool paladin_t::lightsmith() const
{
  return has_hero_tree( hero_tree_e::HERO_LIGHTSMITH );
}
bool paladin_t::herald_of_the_sun() const
{
  return has_hero_tree( hero_tree_e::HERO_HERALD_OF_THE_SUN );
}

// player_t::create_expression ==============================================

std::unique_ptr<expr_t> paladin_t::create_consecration_expression( util::string_view expr_str )
{
  auto expr = util::string_split<util::string_view>( expr_str, "." );
  if ( expr.size() != 2 )
  {
    return nullptr;
  }

  if ( !util::str_compare_ci( expr[ 0U ], "consecration" ) )
  {
    return nullptr;
  }

  if ( util::str_compare_ci( expr[ 1U ], "ticking" ) || util::str_compare_ci( expr[ 1U ], "up" ) )
  {
    return make_fn_expr( "consecration_ticking", [ this ]() { return active_consecration != nullptr; } );
  }
  else if ( util::str_compare_ci( expr[ 1U ], "remains" ) )
  {
    return make_fn_expr( "consecration_remains", [ this ]() {
      return active_consecration == nullptr ? 0 : active_consecration->remaining_time().total_seconds();
    } );
  }
  else if ( util::str_compare_ci( expr[ 1U ], "count" ) )
  {
    return make_fn_expr( "consecration_count", [ this ]() { return all_active_consecrations.size(); } );
  }

  return nullptr;
}

std::unique_ptr<expr_t> paladin_t::create_aw_expression( util::string_view name_str )
{
  auto expr = util::string_split<util::string_view>( name_str, "." );
  if ( expr.size() < 2 )
  {
    return nullptr;
  }

  if ( !util::str_compare_ci( expr[ 1 ], "avenging_wrath" ) )
  {
    return nullptr;
  }

  // Convert [talent/buff/cooldown].avenging_wrath to sentinel if taken
  if ( expr.size() >= 2 && util::str_compare_ci( expr[ 1 ], "avenging_wrath" ) &&
       ( util::str_compare_ci( expr[ 0 ], "buff" ) || util::str_compare_ci( expr[ 0 ], "talent" ) ||
         util::str_compare_ci( expr[ 0 ], "cooldown" ) ) )
  {
    if ( talents.sentinel->ok() )
      expr[ 1 ] = "sentinel";
    else
      expr[ 1 ] = "avenging_wrath";
  }
  return player_t::create_expression( util::string_join( expr, "." ) );
}

// ToDo Fluttershy: Do we need this?
std::unique_ptr<expr_t> paladin_t::create_vw_expression( util::string_view name_str )
{
  auto expr = util::string_split<util::string_view>( name_str, "." );
  if ( expr.size() < 2 )
  {
    return nullptr;
  }

  if ( !util::str_compare_ci( expr[ 1 ], "vengeful_wrath" ) )
  {
    return nullptr;
  }

  if ( expr.size() >= 2 && util::str_compare_ci( expr[ 1 ], "vengeful_wrath" ) && util::str_compare_ci( expr[ 0 ], "talent" ) )
  {
    return make_fn_expr( "talent.vengeful_wrath", [ this ]() { return this->talents.vengeful_wrath->ok() ; } );
  }

  return player_t::create_expression( util::string_join( expr, "." ) );
}

std::unique_ptr<expr_t> paladin_t::create_expression( util::string_view name_str )
{
  struct paladin_expr_t : public expr_t
  {
    paladin_t& paladin;
    paladin_expr_t( util::string_view n, paladin_t& p ) : expr_t( n ), paladin( p )
    {
    }
  };

  auto splits = util::string_split<util::string_view>( name_str, "." );

  struct time_to_hpg_expr_t : public paladin_expr_t
  {
    cooldown_t* cs_cd;
    cooldown_t* boj_cd;
    cooldown_t* j_cd;
    cooldown_t* how_cd;
    cooldown_t* wake_cd;
    cooldown_t* hs_cd;
    cooldown_t* at_cd;
    cooldown_t* hotr_cd;
    cooldown_t* bh_cd;
    cooldown_t* dt_cd;

    time_to_hpg_expr_t( util::string_view n, paladin_t& p )
      : paladin_expr_t( n, p ),
        cs_cd( p.get_cooldown( "crusader_strike" ) ),
        boj_cd( p.get_cooldown( "blade_of_justice" ) ),
        j_cd( p.get_cooldown( "judgment" ) ),
        how_cd( p.get_cooldown( "hammer_of_wrath" ) ),
        wake_cd( p.get_cooldown( "wake_of_ashes" ) ),
        hs_cd( p.get_cooldown( "holy_shock" ) ),
        at_cd( p.get_cooldown( "arcane_torrent" ) ),
        hotr_cd( p.get_cooldown( "hammer_of_the_righteous" ) ),
        bh_cd( p.get_cooldown( "blessed_hammer" ) ),
        dt_cd( p.get_cooldown( "divine_toll" ) )
    {
    }

    // todo: account for divine resonance, crusading strikes, divine auxiliary
    double evaluate() override
    {
      timespan_t gcd_ready = paladin.gcd_ready - paladin.sim->current_time();
      gcd_ready            = std::max( gcd_ready, 0_ms );

      timespan_t shortest_hpg_time = cs_cd->remains();

      // Protection base
      if ( paladin.specialization() == PALADIN_PROTECTION )
      {
        if ( paladin.talents.hammer_of_the_righteous->ok() )
          shortest_hpg_time = hotr_cd->remains();
        if ( paladin.talents.blessed_hammer->ok() )
          shortest_hpg_time = bh_cd->remains();
      }

      // Blood Elf
      if ( paladin.race == RACE_BLOOD_ELF && paladin.specialization() != PALADIN_PROTECTION )
      {
        if ( at_cd->remains() < shortest_hpg_time )
          shortest_hpg_time = at_cd->remains();
      }

      // Retribution
      if ( paladin.specialization() == PALADIN_RETRIBUTION )
      {
        if ( boj_cd->remains() < shortest_hpg_time )
          shortest_hpg_time = boj_cd->remains();

        if ( wake_cd->remains() < shortest_hpg_time )
          shortest_hpg_time = wake_cd->remains();
      }

      // Shared
      if ( paladin.get_how_availability( ) && how_cd->remains() < shortest_hpg_time )
        shortest_hpg_time = how_cd->remains();

      if ( paladin.talents.divine_toll->ok() && dt_cd->remains() < shortest_hpg_time )
        shortest_hpg_time = dt_cd->remains();

      if ( j_cd->remains() < shortest_hpg_time )
        shortest_hpg_time = j_cd->remains();

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

  struct next_armament_expr_t : public paladin_expr_t
  {
    next_armament_expr_t(util::string_view n, paladin_t& p) : paladin_expr_t(n, p)
    {

    }
    double evaluate() override
    {
      if ( paladin.lightsmith() )
        return paladin.next_armament;
      else
        return -1.0;
    }
  };

  if (splits[0] == "next_armament")
  {
    return std::make_unique<next_armament_expr_t>( name_str, *this );
  }
  if (splits[0] == "holy_bulwark")
  {
    return make_fn_expr( "holy_bulwark", []() { return armament::HOLY_BULWARK; } );
  }
  if ( splits[ 0 ] == "sacred_weapon" )
  {
    return make_fn_expr( "sacred_weapon", []() { return armament::SACRED_WEAPON; } );
  }
  if ( splits[ 0 ] == "divine_hammer_icd_remains" )
  {
    return make_fn_expr( "divine_hammer_icd_remains", []() { return timespan_t::zero(); } );
  }

  struct judgment_holy_power_expr_t : public paladin_expr_t
  {
    judgment_holy_power_expr_t( util::string_view n, paladin_t& p ) : paladin_expr_t( n, p )
    {
    }
    double evaluate() override
    {
      double gain = 1.0;
      if (paladin.talents.sanctified_wrath->ok())
      {
        if ( paladin.buffs.avenging_wrath->up() || paladin.buffs.sentinel->up() )
          gain++;
      }
      return gain;
    }
  };

  if (splits[0] == "judgment_holy_power")
  {
    return std::make_unique<judgment_holy_power_expr_t>( name_str, *this );
  }

  auto cons_expr = create_consecration_expression( name_str );
  auto aw_expr   = create_aw_expression( name_str );
  auto vw_expr   = create_vw_expression( name_str );
  if ( cons_expr )
  {
    return cons_expr;
  }
  if ( aw_expr )
  {
    return aw_expr;
  }
  if ( vw_expr )
  {
    return vw_expr;
  }

  struct time_until_next_csaa_expr_t : public paladin_expr_t
  {
    time_until_next_csaa_expr_t( util::string_view n, paladin_t& p ) : paladin_expr_t( n, p )
    {
    }

    double evaluate() override
    {
      if ( !paladin.talents.crusading_strikes->ok() )
      {
        return std::numeric_limits<double>::infinity();
      }

      if ( paladin.main_hand_attack && paladin.main_hand_attack->execute_event )
      {
        return paladin.main_hand_attack->execute_event->remains().total_seconds() +
               paladin.main_hand_attack->execute_time().total_seconds();
      }

      return std::numeric_limits<double>::infinity();
    }
  };

  if ( specialization() == PALADIN_RETRIBUTION && util::str_compare_ci( splits[ 0 ], "time_to_next_csaa_hopo" ) )
  {
    return std::make_unique<time_until_next_csaa_expr_t>( name_str, *this );
  }

  if ( specialization() == PALADIN_PROTECTION && ( splits.size() >= 2 && util::str_compare_ci( splits[ 1 ], "avenging_wrath" ) ) )
  {
    splits[ 1 ] = talents.sentinel->ok() ? "sentinel" : "avenging_wrath";
    return paladin_t::create_expression( util::string_join( splits, "." ) );
  }

  return player_t::create_expression( name_str );
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class paladin_report_t : public player_report_extension_t
{
public:
  paladin_report_t( paladin_t& player ) : p( player )
  {
  }
  void html_customsection( report::sc_html_stream& /* os */ ) override
  {
  }

private:
  [[maybe_unused]] paladin_t& p;
};

// PALADIN MODULE INTERFACE =================================================

struct paladin_module_t : public module_t
{
  paladin_module_t() : module_t( PALADIN )
  {
  }

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p              = new paladin_t( sim, name, r );
    p->report_extension = std::unique_ptr<player_report_extension_t>( new paladin_report_t( *p ) );
    return p;
  }

  bool valid() const override
  {
    return true;
  }

  void register_actor_initializers( sim_t* sim ) const override
  {
    sim->register_actor_initializer( INIT_ACTOR_CREATE_BUFFS + offset(), [ sim ]( player_t* p ) {
      if ( !p->is_player() )
        return;

      // Only create if a paladin is in the sim
      if ( !range::count_if( sim->player_no_pet_list, []( player_t* p ) { return p->type == PALADIN; } ) )
        return;

      p->buffs.blessing_of_sacrifice = new buffs::blessing_of_sacrifice_t( p );
      p->debuffs.forbearance         = new buffs::forbearance_t( p, "forbearance" );
    }, "create_buffs_paladin" );
  }

  void register_hotfixes() const override
  {
  }
};
}  // end namespace paladin

const module_t* module_t::paladin()
{
  static paladin::paladin_module_t m;
  return &m;
}
