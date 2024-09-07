// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
/*
  TODO: reimplement Holy if anyone ever becomes interested in maintaining it
*/
#include "sc_paladin.hpp"

#include "simulationcraft.hpp"
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
    next_season( SUMMER ),
    next_armament( SACRED_WEAPON ),
    radiant_glory_accumulator( 0.0 ),
    lights_deliverance_triggered_during_ready( false ),
    holy_power_generators_used( 0 ),
    melee_swing_count( 0 ),
    random_weapon_target( nullptr ),
    random_bulwark_target( nullptr ),
    divine_inspiration_next( -1 )
{
  active_consecration = nullptr;
  active_boj_cons = nullptr;
  active_searing_light_cons = nullptr;
  all_active_consecrations.clear();
  active_hallow_damaging       = nullptr;
  active_hallow_healing     = nullptr;
  active_aura         = nullptr;

  cooldowns.avenging_wrath               = get_cooldown( "avenging_wrath" );
  cooldowns.sentinel                     = get_cooldown( "sentinel" );
  cooldowns.hammer_of_justice            = get_cooldown( "hammer_of_justice" );
  cooldowns.judgment_of_light_icd        = get_cooldown( "judgment_of_light_icd" );
  cooldowns.blessing_of_protection       = get_cooldown( "blessing_of_protection" );
  cooldowns.blessing_of_spellwarding     = get_cooldown( "blessing_of_spellwarding" );
  cooldowns.divine_shield                = get_cooldown( "divine_shield" );
  cooldowns.lay_on_hands                 = get_cooldown( "lay_on_hands" );

  cooldowns.holy_shock    = get_cooldown( "holy_shock" );
  cooldowns.light_of_dawn = get_cooldown( "light_of_dawn" );

  cooldowns.avengers_shield                   = get_cooldown( "avengers_shield" );
  cooldowns.consecration                      = get_cooldown( "consecration" );
  cooldowns.inner_light_icd                   = get_cooldown( "inner_light_icd" );
  cooldowns.inner_light_icd->duration         = find_spell( 386556 )->internal_cooldown();
  cooldowns.righteous_protector_icd           = get_cooldown( "righteous_protector_icd" );
  cooldowns.righteous_protector_icd->duration = find_spell( 204074 )->internal_cooldown();
  cooldowns.judgment                          = get_cooldown( "judgment" );
  cooldowns.shield_of_the_righteous           = get_cooldown( "shield_of_the_righteous" );
  cooldowns.guardian_of_ancient_kings         = get_cooldown( "guardian_of_ancient_kings" );
  cooldowns.ardent_defender                   = get_cooldown( "ardent_defender" );
  cooldowns.eye_of_tyr                        = get_cooldown( "eye_of_tyr" );

  cooldowns.blade_of_justice = get_cooldown( "blade_of_justice" );
  cooldowns.final_reckoning  = get_cooldown( "final_reckoning" );
  cooldowns.hammer_of_wrath  = get_cooldown( "hammer_of_wrath" );
  cooldowns.wake_of_ashes    = get_cooldown( "wake_of_ashes" );

  cooldowns.blessing_of_the_seasons = get_cooldown( "blessing_of_the_seasons" );
  cooldowns.holy_armaments           = get_cooldown( "holy_armaments" );

  cooldowns.ret_aura_icd = get_cooldown( "ret_aura_icd" );
  cooldowns.ret_aura_icd->duration = timespan_t::from_seconds( 30 );

  cooldowns.consecrated_blade_icd = get_cooldown( "consecrated_blade_icd" );
  cooldowns.consecrated_blade_icd->duration = timespan_t::from_seconds( 10 );

  cooldowns.searing_light_icd = get_cooldown( "searing_light_icd" );
  cooldowns.searing_light_icd->duration = timespan_t::from_seconds( 15 );

  cooldowns.endless_wrath_icd           = get_cooldown( "endless_wrath_icd" );
  cooldowns.endless_wrath_icd->duration = find_spell( 432615 )->internal_cooldown();

  cooldowns.hammerfall_icd           = get_cooldown( "hammerfall_icd" );
  cooldowns.hammerfall_icd->duration = find_spell( 432463 )->internal_cooldown();

  cooldowns.aurora_icd = get_cooldown( "aurora_icd" );
  cooldowns.aurora_icd->duration = find_spell( 439760 )->internal_cooldown();

  cooldowns.second_sunrise_icd = get_cooldown( "second_sunrise_icd" );
  cooldowns.second_sunrise_icd->duration = find_spell( 431474 )->internal_cooldown();

  cooldowns.art_of_war = get_cooldown( "art_of_war" );
  cooldowns.art_of_war->duration = find_spell( 406064 )->internal_cooldown();

  cooldowns.radiant_glory_icd = get_cooldown( "radiant_glory_icd" );
  cooldowns.radiant_glory_icd->duration = timespan_t::from_millis( 500 );

  cooldowns.righteous_cause_icd = get_cooldown( "righteous_cause_icd" );
  cooldowns.righteous_cause_icd->duration = find_spell( 402912 )->internal_cooldown();

  beacon_target         = nullptr;
  resource_regeneration = regen_type::DYNAMIC;
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
avenging_wrath_buff_t::avenging_wrath_buff_t( paladin_t* p )
  : buff_t( p, "avenging_wrath", p->spells.avenging_wrath ),
    damage_modifier( 0.0 ),
    healing_modifier( 0.0 ),
    crit_bonus( 0.0 )
{
  // Map modifiers appropriately based on spec
  healing_modifier = p->talents.avenging_wrath->effectN( 2 ).percent();
  crit_bonus       = 0;
  damage_modifier  = p->talents.avenging_wrath->effectN( 2 ).percent();

  if ( p->talents.sanctified_wrath->ok() )
  {
    // the tooltip doesn't say this, but the spelldata does
    base_buff_duration *= 1.0 + p->talents.sanctified_wrath->effectN( 1 ).percent();
  }

  if ( p->talents.avenging_wrath_might->ok() )
    crit_bonus = p->talents.avenging_wrath_might->effectN( 1 ).percent();

  if ( p->talents.divine_wrath->ok() )
  {
    base_buff_duration += p->talents.divine_wrath->effectN( 1 ).time_value();
  }

  // let the ability handle the cooldown
  cooldown->duration = 0_ms;

  // invalidate Healing
  add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );
  add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  add_invalidate( CACHE_CRIT_CHANCE );
  add_invalidate( CACHE_MASTERY );
}

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

    // Uther's Counsel reduces cooldown
    if ( p->talents.uthers_counsel->ok() )
    {
      cooldown->duration *= 1.0 + p->talents.uthers_counsel->effectN( 2 ).percent();
    }
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

avenging_wrath_t::avenging_wrath_t( paladin_t* p, util::string_view options_str )
  : paladin_spell_t( "avenging_wrath", p, p->find_spell( 31884 ) )
{
  parse_options( options_str );
  if ( !p->talents.avenging_wrath->ok() )
    background = true;
  if ( p->talents.crusade->ok() )
    background = true;
  if ( p->talents.avenging_crusader->ok() )
    background = true;
  if ( p->talents.sentinel->ok() )
    background = true;
  if ( p->talents.radiant_glory->ok() )
    background = true;

  harmful = false;

  // link needed for Righteous Protector / SotR cooldown reduction
  cooldown = p->cooldowns.avenging_wrath;
}

void avenging_wrath_t::execute()
{
  paladin_spell_t::execute();

  p()->buffs.avenging_wrath->trigger();
  if ( p()->talents.lightsmith.blessing_of_the_forge->ok() )
    p()->buffs.lightsmith.blessing_of_the_forge->execute();
  p()->tww1_4p_prot();
  if ( p()->talents.herald_of_the_sun.suns_avatar->ok() )
  {
    p()->apply_avatar_dawnlights();
  }
}

// Holy Avenger
struct holy_avenger_t : public paladin_spell_t
{
  holy_avenger_t( paladin_t* p, util::string_view options_str )
    : paladin_spell_t( "holy_avenger", p, p->talents.holy_avenger )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    paladin_spell_t::execute();

    p()->buffs.holy_avenger->trigger();
  }
};

// Consecration =============================================================

struct golden_path_t : public paladin_heal_t
{
  golden_path_t( paladin_t* p ) : paladin_heal_t( "golden_path", p, p->find_spell( 339119 ) )
  {
    background = true;
  }
};

struct consecration_tick_t : public paladin_spell_t
{
  golden_path_t* heal_tick;

  consecration_tick_t( util::string_view name, paladin_t* p )
    : paladin_spell_t( name, p, p->find_spell( 81297 ) ),
      heal_tick( new golden_path_t( p ) )
  {
    aoe         = -1;
    dual        = true;
    direct_tick = true;
    background  = true;
    may_crit    = true;
    ground_aoe  = true;
    searing_light_disabled = true;
  }

  double action_multiplier() const override
  {
    double m = paladin_spell_t::action_multiplier();
    if ( p()->talents.consecration_in_flame->ok() )
    {
      m *= 1.0 + p()->talents.consecration_in_flame->effectN( 2 ).percent();
    }
    if (p()->buffs.sanctification_empower->up())
    {
      m *= 1.0 + p()->buffs.sanctification_empower->data().effectN( 1 ).percent();
    }
    return m;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = paladin_spell_t::composite_target_multiplier( target );

    paladin_td_t* td = p()->get_target_data( target );
    if ( td->debuff.sanctify->up() )
      m *= 1.0 + td->debuff.sanctify->data().effectN( 1 ).percent();

    return m;
  }
};

struct divine_guidance_damage_t : public paladin_spell_t
{
  divine_guidance_damage_t( util::string_view n, paladin_t* p ) : paladin_spell_t( n, p, p->find_spell( 433808 ) )
  {
    proc = may_crit         = true;
    may_miss                = false;
    attack_power_mod.direct = 1;
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
    attack_power_mod.direct = 1;
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
    if ( p->specialization() == PALADIN_PROTECTION && p->spec.consecration_3->ok() )
      cooldown->duration *= 1.0 + p->spec.consecration_3->effectN( 1 ).percent();

    // technically this doesn't work for characters under level 11?
    if ( p->specialization() == PALADIN_RETRIBUTION )
      background = true;

    add_child( damage_tick );
    if ( p->talents.lightsmith.divine_guidance->ok() )
    {
      dg_damage = new divine_guidance_damage_t( "_divine_guidance", p );
      dg_heal   = new divine_guidance_heal_t( "_divine_guidance_heal", p );
      add_child( dg_damage );
      // Maybe later: Heal?
    }
  }

  consecration_t( paladin_t* p, util::string_view source_name, consecration_source source )
    : paladin_spell_t( std::string(source_name) + "_consecration", p, p->find_spell( 26573 ) ),
      damage_tick( new consecration_tick_t( std::string(source_name) + "_consecration_tick", p ) ),
      source_type( source )
  {
    dot_duration = 0_ms;  // the periodic event is handled by ground_aoe_event_t
    may_miss = harmful = false;
    background = true;

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

    if ( p()->talents.consecration_in_flame->ok() )
      cons_duration += timespan_t::from_millis( p()->talents.consecration_in_flame->effectN( 1 ).base_value() );

    cons_params = ground_aoe_params_t()
                      .duration( cons_duration )
                      .hasted( ground_aoe_params_t::SPELL_HASTE )
                      .action( damage_tick )
                      .state_callback( [ this ]( ground_aoe_params_t::state_type type, ground_aoe_event_t* event ) {
                        switch ( type )
                        {
                          case ground_aoe_params_t::EVENT_STOPPED:
                            if ( p()->buffs.sanctification_empower->up() )
                            {
                              p()->buffs.sanctification_empower->expire();
                            }
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
                            else if ( source_type == SEARING_LIGHT )
                            {
                              p()->active_searing_light_cons = event;
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
                            else if ( source_type == SEARING_LIGHT )
                            {
                              p()->active_searing_light_cons = nullptr;
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
      if ( p()->buffs.sanctification_empower->up() )
      {
        p()->buffs.sanctification_empower->expire();
      }
      p()->all_active_consecrations.erase( p()->active_consecration );
      event_t::cancel( p()->active_consecration );
    }
    // or if it's a boj-triggered Cons, cancel the previous BoJ-triggered cons
    else if ( source_type == BLADE_OF_JUSTICE && p()->active_boj_cons != nullptr )
    {
      p()->all_active_consecrations.erase( p()->active_boj_cons );
      event_t::cancel( p()->active_boj_cons );
    }
    // or if it's a searing light-triggered Cons, cancel the previous searing light-triggered cons
    else if ( source_type == SEARING_LIGHT && p()->active_searing_light_cons != nullptr )
    {
      p()->all_active_consecrations.erase( p()->active_searing_light_cons );
      event_t::cancel( p()->active_searing_light_cons );
    }

    if (p()->buffs.sanctification->at_max_stacks())
    {
      p()->buffs.sanctification->expire();
      p()->buffs.sanctification_empower->execute();
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
    if ( p()->buffs.lightsmith.divine_guidance->up() )
    {
      for (auto friendly : sim->player_non_sleeping_list)
      {
        if ( friendly == p() )  // Always heal ourselves to avoid oversim
          healingAllies.push_back( friendly );
        else if ( friendly->health_percentage() < 100 ) // Allies are only healed when they're not full HP
          healingAllies.push_back( friendly );
        if ( healingAllies.size() == p()->options.max_dg_heal_targets )
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
    if ( p()->buffs.lightsmith.divine_guidance->up() )
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
      make_event<ground_aoe_event_t>( *sim, p(), cons_params, true /* Immediate pulse */ );
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
    cooldown = p->cooldowns.divine_shield; // Needed for cooldown reduction via Resolute Defender

    // unbreakable spirit reduces cooldown
    if ( p->talents.unbreakable_spirit->ok() )
      cooldown->duration = data().cooldown() * ( 1 + p->talents.unbreakable_spirit->effectN( 1 ).percent() );

    // Uther's Counsel also reduces cooldown
    if ( p->talents.uthers_counsel->ok() )
    {
      cooldown->duration *= 1.0 + p->talents.uthers_counsel->effectN( 2 ).percent();
    }

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
    if ( player->debuffs.forbearance->check() )
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

// Judgment of Light proc =====================================================

struct judgment_of_light_proc_t : public paladin_heal_t
{
  judgment_of_light_proc_t( paladin_t* p )
    : paladin_heal_t( "judgment_of_light", p, p->find_spell( 183811 ) )  // proc data stored in 183811
  {
    background = proc = may_crit = true;
    may_miss                     = false;

    // NOTE: this is implemented in SimC as a self-heal only. It does NOT proc for other players attacking the boss.
    // This is mostly done because it's much simpler to code, and for the most part Prot doesn't care about raid healing
    // efficiency. If Holy wants this to work like the in-game implementation, they'll have to go through the pain of
    // moving things to player_t
  }
};

// Lay on Hands Spell =======================================================

struct lay_on_hands_t : public paladin_heal_t
{
  lay_on_hands_t( paladin_t* p, util::string_view options_str )
    : paladin_heal_t( "lay_on_hands", p, p->find_talent_spell (talent_tree::CLASS, "Lay on Hands" ) )
  {
    parse_options( options_str );
    // unbreakable spirit reduces cooldown
    if ( p->talents.unbreakable_spirit->ok() )
    {
      cooldown->duration *= 1.0 + p->talents.unbreakable_spirit->effectN( 1 ).percent();
    }

    // Uther's Counsel also reduces cooldown
    if ( p->talents.uthers_counsel->ok())
    {
      cooldown->duration *= 1.0 + p->talents.uthers_counsel->effectN( 2 ).percent();
    }

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

struct retribution_aura_t : public paladin_aura_base_t
{
  retribution_aura_t( paladin_t* p, util::string_view options_str )
    : paladin_aura_base_t( "retribution_aura", p, p->find_spell( 183435 ) )
  {
    parse_options( options_str );

    if ( !p->talents.auras_of_swift_vengeance->ok() )
      background = true;

    aura_buff = p->buffs.retribution_aura;
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

    // unbreakable spirit reduces cooldown
    if ( p->talents.unbreakable_spirit->ok() )
      cooldown->duration = data().cooldown() * ( 1 + p->talents.unbreakable_spirit->effectN( 1 ).percent() );
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

struct touch_of_light_dmg_t : public paladin_spell_t
{
  touch_of_light_dmg_t( paladin_t* p ) : paladin_spell_t( "touch_of_light_dmg", p, p -> find_spell( 385354 ) )
  {
    background = true;
    skip_es_accum = true; // per bolas test Aug 17 2024
  }
};

struct touch_of_light_heal_t : public paladin_heal_t
{
  touch_of_light_heal_t( paladin_t* p ) : paladin_heal_t( "touch_of_light_heal", p, p -> find_spell( 385352 ) )
  {
    background = true;
  }
};

struct touch_of_light_cb_t : public dbc_proc_callback_t
{
  paladin_t* p;
  touch_of_light_dmg_t* dmg;
  touch_of_light_heal_t* heal;

  touch_of_light_cb_t( paladin_t* player, const special_effect_t& effect )
    : dbc_proc_callback_t( player, effect ), p( player )
  {
    dmg = new touch_of_light_dmg_t( player );
    heal = new touch_of_light_heal_t( player );
  }

  void execute( action_t*, action_state_t* s ) override
  {
    if ( s->target->is_enemy() )
    {
      dmg->set_target( s->target );
      dmg->schedule_execute();
    }
    else
    {
      heal->set_target( s->target );
      heal->schedule_execute();
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

struct seal_of_the_crusader_t : public paladin_spell_t
{
  seal_of_the_crusader_t( paladin_t* p )
    : paladin_spell_t( "seal_of_the_crusader", p, p->talents.seal_of_the_crusader->effectN( 1 ).trigger() )
  {
    background = true;
  }

  double action_multiplier() const override
  {
    double am = paladin_spell_t::action_multiplier();
    am *= 1.0 + p()->talents.seal_of_the_crusader->effectN( 2 ).percent();
    return am;
  }
};

struct crusading_strike_t : public paladin_melee_attack_t
{
  crusading_strike_t( paladin_t* p )
    : paladin_melee_attack_t( "crusading_strike", p, p -> find_spell( 408385 ) )
  {
    background = true;
    trigger_gcd = 0_ms;

    if ( p->talents.blades_of_light->ok() )
    {
      affected_by.highlords_judgment = true;
    }

    if ( p->talents.blessed_champion->ok() )
    {
      aoe = as<int>( 1 + p->talents.blessed_champion->effectN( 4 ).base_value() );
      base_aoe_multiplier *= 1.0 - p->talents.blessed_champion->effectN( 3 ).percent();
    }

    if ( p->talents.heart_of_the_crusader->ok() )
    {
      crit_bonus_multiplier *= 1.0 + p->talents.heart_of_the_crusader->effectN( 4 ).percent();
      base_multiplier *= 1.0 + p->talents.heart_of_the_crusader->effectN( 3 ).percent();
    }
    triggers_higher_calling = true;
  }

  void execute() override
  {
    paladin_melee_attack_t::execute();
    if ( result_is_hit( execute_state->result ) )
    {
      p()->melee_swing_count++;
      if ( p()->melee_swing_count % as<int>( p()->talents.crusading_strikes->effectN( 3 ).base_value() ) == 0 )
      {
        p()->resource_gain(
          RESOURCE_HOLY_POWER,
          as<int>( p()->talents.crusading_strikes->effectN( 4 ).base_value() ),
          p()->gains.hp_crusading_strikes
        );
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    paladin_melee_attack_t::impact( s );

    if ( result_is_hit( s->result ) && p()->talents.empyrean_power->ok() )
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
  seal_of_the_crusader_t* seal_of_the_crusader;
  crusading_strike_t* crusading_strike;

  melee_t( paladin_t* p )
    : paladin_melee_attack_t( "melee", p, spell_data_t::nil() ), first( true ), seal_of_the_crusader( nullptr ), crusading_strike( nullptr )
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
        aoe = as<int>( 1 + p->talents.blessed_champion->effectN( 4 ).base_value() );
        base_aoe_multiplier *= 1.0 - p->talents.blessed_champion->effectN( 3 ).percent();
      }
    }

    affected_by.avenging_wrath = affected_by.crusade = true;

    if ( p->talents.seal_of_the_crusader->ok() )
    {
      seal_of_the_crusader = new seal_of_the_crusader_t( p );
      add_child( seal_of_the_crusader );
    }

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

  void impact( action_state_t* s ) override
  {
    paladin_melee_attack_t::impact( s );

    if ( p()->talents.seal_of_the_crusader->ok() )
    {
      seal_of_the_crusader->execute_on_target( s->target );
    }
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

    if ( p->talents.blades_of_light->ok() )
    {
      affected_by.highlords_judgment = true;
    }

    if ( p->talents.swift_justice->ok() )
    {
      cooldown->duration += timespan_t::from_millis( p->talents.swift_justice->effectN( 2 ).base_value() );
    }

    if ( p->spec.improved_crusader_strike )
    {
      cooldown->charges += as<int>( p->spec.improved_crusader_strike->effectN( 1 ).base_value() );
    }

    if ( p->talents.blessed_champion->ok() )
    {
      aoe = as<int>( 1 + p->talents.blessed_champion->effectN( 4 ).base_value() );
      base_aoe_multiplier *= 1.0 - p->talents.blessed_champion->effectN( 3 ).percent();
    }

    if ( p->talents.heart_of_the_crusader->ok() )
    {
      crit_bonus_multiplier *= 1.0 + p->talents.heart_of_the_crusader->effectN( 4 ).percent();
      base_multiplier *= 1.0 + p->talents.heart_of_the_crusader->effectN( 3 ).percent();
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

   if ( result_is_hit( s->result ) && p()->talents.empyrean_power->ok() )
    {
      if ( rng().roll( p()->talents.empyrean_power->effectN( 1 ).percent() ) )
      {
        p()->procs.empyrean_power->occur();
        p()->buffs.empyrean_power->trigger();
      }
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

    if ( p()->specialization() == PALADIN_PROTECTION )
    {
      if ( p()->talents.faith_in_the_light->ok() )
      {
        p()->buffs.faith_in_the_light->trigger();
      }
    }

    if ( p()->specialization() == PALADIN_HOLY && p()->talents.awakening->ok() )
    {
      if ( rng().roll( p()->talents.awakening->effectN( 1 ).percent() ) )
      {
        buff_t* main_buff           = p()->buffs.avenging_wrath;
        timespan_t trigger_duration = timespan_t::from_seconds( p()->talents.awakening->effectN( 2 ).base_value() );
        if ( main_buff->check() )
        {
          p()->buffs.avenging_wrath->extend_duration( p(), trigger_duration );
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
    if ( p()->talents.strength_of_conviction->ok() && p()->standing_in_consecration() )
    {
      am *= 1.0 + p()->talents.strength_of_conviction->effectN( 1 ).percent();
    }
    return am;
  }

  double cost_pct_multiplier() const override
  {
    double c = holy_power_consumer_t::cost_pct_multiplier();

    if ( p()->buffs.shining_light_free->check() )
      c *= 1.0 + p()->buffs.shining_light_free->data().effectN( 1 ).percent();
    if ( p()->buffs.royal_decree->check() )
      c *= 1.0 + p()->buffs.royal_decree->data().effectN( 1 ).percent();

    return c;
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

// Holy Shield damage proc ==================================================

struct holy_shield_damage_t : public paladin_spell_t
{
  holy_shield_damage_t( paladin_t* p )
    : paladin_spell_t( "holy_shield", p, p->talents.holy_shield->effectN( 2 ).trigger() )
  {
    background = proc = may_crit = true;
    may_miss                     = false;
  }
};


// Inner light damage proc ==================================================
//TODO Check new spell id
struct inner_light_damage_t : public paladin_spell_t
{
  inner_light_damage_t( paladin_t* p )
      : paladin_spell_t( "inner_light", p, p->find_spell ( 386553 )  )
  {
    background  = proc = may_crit = true;
    may_miss                      = false;
    cooldown                      = p->cooldowns.inner_light_icd;
  }
};

// Base Judgment spell ======================================================

judgment_t::judgment_t( paladin_t* p, util::string_view name ) :
  paladin_melee_attack_t( name, p, p->find_class_spell( "Judgment" ) )
{
  // no weapon multiplier
  weapon_multiplier = 0.0;
  may_block = may_parry = may_dodge = false;

  // force effect 1 to be used for direct ratios
  parse_effect_data( data().effectN( 1 ) );

  // rank 2 multiplier
  if ( p->spells.judgment_2->ok() )
  {
    base_multiplier *= 1.0 + p->spells.judgment_2->effectN( 1 ).percent();
  }

  if ( p->talents.zealots_paragon->ok() )
  {
    base_multiplier *= 1.0 + p->talents.zealots_paragon->effectN( 3 ).percent();
  }

  if ( p->talents.seal_of_alacrity->ok() )
  {
    cooldown->duration += timespan_t::from_millis( p->talents.seal_of_alacrity->effectN( 2 ).base_value() );
  }

  if ( p->talents.swift_justice->ok() )
  {
    cooldown->duration += timespan_t::from_millis( p->talents.swift_justice->effectN( 2 ).base_value() );
  }

  if ( p->talents.judgment_of_justice->ok() )
  {
    base_multiplier *= 1.0 + p->talents.judgment_of_justice->effectN( 2 ).percent();
  }

  if ( p->talents.improved_judgment->ok() )
  {
    cooldown->charges += as<int>( p->talents.improved_judgment->effectN( 1 ).base_value() );
  }
}

proc_types judgment_t::proc_type() const
{
  return PROC1_MELEE_ABILITY;
}

void judgment_t::impact( action_state_t* s )
{
  paladin_melee_attack_t::impact( s );

  if ( result_is_hit( s->result ) )
  {
    if ( p()->talents.greater_judgment->ok() )
    {
      int num_stacks = 1;
      if ( p()->talents.highlords_wrath->ok() )
      {
        num_stacks += as<int>( p()->talents.highlords_wrath->effectN( 1 ).base_value() );
      }
      td( s->target )->debuff.judgment->trigger( num_stacks );
    }

    int amount = 5;
    if ( p()->talents.judgment_of_light->ok() )
      td( s->target )->debuff.judgment_of_light->trigger( amount );
  }
}

void judgment_t::execute()
{
  paladin_melee_attack_t::execute();

  if ( p()->talents.zealots_paragon->ok() )
  {
    auto extension = timespan_t::from_millis( p()->talents.zealots_paragon->effectN( 1 ).base_value() );

    if ( p()->buffs.avenging_wrath->up() )
    {
      p()->buffs.avenging_wrath->extend_duration( p(), extension );
    }

    if ( p()->buffs.crusade->up() )
    {
      p()->buffs.crusade->extend_duration( p(), extension );
    }

    if ( p()->buffs.sentinel->up() )
    {
      p()->buffs.sentinel->extend_duration( p(), extension );
      // 2022-11-14 If Sentinel is still at max stacks, Zealot's Paragon increases decay length, too.
      if ( p()->bugs && p()->buffs.sentinel->at_max_stacks() && p()->buffs.sentinel_decay->up())
      {
        p()->buffs.sentinel_decay->extend_duration( p(), extension );
      }
    }
  }

  // Decrement only if active Judgment, Divine Toll handling on judgment_ret_t
  if ( !background )
    p()->buffs.templar.for_whom_the_bell_tolls->decrement();

  if ( p()->talents.templar.sanctification->ok() )
  {
    p()->buffs.templar.sanctification->trigger();
  }
}

double judgment_t::action_multiplier() const
{
  double am = paladin_melee_attack_t::action_multiplier();

  if ( p()->talents.justification->ok() )
    am *= 1.0 + p()->talents.justification->effectN( 1 ).percent();

  // Increase only if active Judgment, Divine Toll handling on judgment_ret_t
  if ( p()->buffs.templar.for_whom_the_bell_tolls->up() && !background )
    am *= 1.0 + p()->buffs.templar.for_whom_the_bell_tolls->current_value;

  return am;
}

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


// Covenants =======

struct righteous_might_t : public heal_t
{
  righteous_might_t( paladin_t* p ) : heal_t( "righteous_might", p, p->find_spell( 340193 ) )
  {
    background = true;
    callbacks = may_crit = may_miss = false;
    // target = p;
  }
};

struct divine_toll_t : public paladin_spell_t
{
  bool t31HasProcced;
  divine_toll_t( paladin_t* p, util::string_view options_str )
    : paladin_spell_t( "divine_toll", p, p->talents.divine_toll )
  {
    parse_options( options_str );

    aoe = as<int>( data().effectN( 1 ).base_value() );

    add_child( p->active.divine_toll );
    add_child( p->active.divine_resonance );

    if ( p->talents.quickened_invocation->ok() ) // They share spell id, so it's the same in the end
      cooldown->duration += timespan_t::from_millis( p->talents.quickened_invocation->effectN( 1 ).base_value() );
  }

  void impact( action_state_t* s ) override
  {
    paladin_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      p()->active.divine_toll->set_target( s->target );
      p()->active.divine_toll->schedule_execute();
    }
  }

  void execute() override
  {
    t31HasProcced = false;

    paladin_spell_t::execute();
    if ( p()->talents.divine_resonance->ok() )
    {
      p()->buffs.divine_resonance->trigger();
    }
    if ( p()->talents.templar.for_whom_the_bell_tolls->ok() )
    {
      auto data = p()->buffs.templar.for_whom_the_bell_tolls->data();
      // Damage for Judgment increases by 100%, reduced by 20% for each target more than 1, maximum 5 (Since Divine Toll caps at 5)
      auto increase = data.effectN( 1 ).base_value() - ( data.effectN( 4 ).base_value() * ( execute_state->n_targets - 1 ) );
      p()->buffs.templar.for_whom_the_bell_tolls->execute(-1, increase / 100.0, timespan_t::min());
    }
  }
};

// Blessing of Seasons

struct blessing_of_summer_proc_t : public spell_t
{
  blessing_of_summer_proc_t( player_t* p ) : spell_t( "blessing_of_summer_proc", p, p->find_spell( 328123 ) )
  {
    may_dodge = may_parry = may_block = may_crit = callbacks = false;
    background                                               = true;
  }

  void init() override
  {
    spell_t::init();
    snapshot_flags &= STATE_NO_MULTIPLIER;
  }
};

struct blessing_of_summer_t : public paladin_spell_t
{
  timespan_t buff_duration;

  blessing_of_summer_t( paladin_t* p )
    : paladin_spell_t( "blessing_of_summer", p, p->find_spell( 328620 ) ),
      buff_duration( data().duration() )
  {
    harmful = false;

    cooldown = p->cooldowns.blessing_of_the_seasons;
  }

  void execute() override
  {
    paladin_spell_t::execute();

    player->buffs.blessing_of_summer->trigger( buff_duration );
  }
};

struct blessing_of_autumn_t : public paladin_spell_t
{
  blessing_of_autumn_t( paladin_t* p ) : paladin_spell_t( "blessing_of_autumn", p, p->find_spell( 328622 ) )
  {
    harmful = false;

    cooldown = p->cooldowns.blessing_of_the_seasons;
  }

  void execute() override
  {
    paladin_spell_t::execute();

    player->buffs.blessing_of_autumn->trigger();
  }
};

struct blessing_of_spring_t : public paladin_spell_t
{
  blessing_of_spring_t( paladin_t* p ) : paladin_spell_t( "blessing_of_spring", p, p->find_spell( 328282 ) )
  {
    harmful = false;

    cooldown = p->cooldowns.blessing_of_the_seasons;
  }

  void execute() override
  {
    paladin_spell_t::execute();

    player->buffs.blessing_of_spring->trigger();
  }
};

template <typename Base, typename Player>
struct blessing_of_winter_proc_t : public Base
{
private:
  using ab = Base;  // action base, eg. spell_t
public:
  blessing_of_winter_proc_t( Player* p ) : ab( "blessing_of_winter_proc", p, p->find_spell( 328506 ) )
  {
    ab::callbacks              = false;
    ab::background             = true;
    ab::spell_power_mod.direct = ab::attack_power_mod.direct = ab::data().effectN( 2 ).percent();
  }

  // Uses max(AP, SP)
  double attack_direct_power_coefficient( const action_state_t* s ) const override
  {
    if ( s->attack_power < s->spell_power )
      return 0;

    return ab::attack_direct_power_coefficient( s );
  }

  double spell_direct_power_coefficient( const action_state_t* s ) const override
  {
    if ( s->spell_power <= s->attack_power )
      return 0;

    return ab::spell_direct_power_coefficient( s );
  }

  double action_multiplier() const override
  {
    double am = ab::action_multiplier();

    return am;
  }
};

struct blessing_of_winter_t : public paladin_spell_t
{
  blessing_of_winter_t( paladin_t* p ) : paladin_spell_t( "blessing_of_winter", p, p->find_spell( 328281 ) )
  {
    harmful = false;

    cooldown = p->cooldowns.blessing_of_the_seasons;
  }

  void execute() override
  {
    paladin_spell_t::execute();

    player->buffs.blessing_of_winter->trigger();
  }
};

struct blessing_of_the_seasons_t : public paladin_spell_t
{
  blessing_of_the_seasons_t( paladin_t* p, util::string_view options_str )
    : paladin_spell_t( "blessing_of_the_seasons", p, spell_data_t::nil() )
  {
    parse_options( options_str );

    // we're keeping this around in case it's relevant to holy someday
    background = true;

    harmful = false;

    hasted_gcd  = true;

    cooldown           = p->cooldowns.blessing_of_the_seasons;
  }

  timespan_t execute_time() const override
  {
    return p()->active.seasons[ p()->next_season ]->execute_time();
  }

  void execute() override
  {
    paladin_spell_t::execute();
    p()->active.seasons[ p()->next_season ]->execute();
    p()->next_season = season( ( p()->next_season + 1 ) % NUM_SEASONS );
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

struct hammer_of_light_damage_t : public holy_power_consumer_t<paladin_melee_attack_t>
{
  hammer_of_light_damage_t( paladin_t* p, util::string_view options_str )
    : holy_power_consumer_t( "hammer_of_light_damage", p, p->spells.templar.hammer_of_light )
  {
    parse_options( options_str );
    background = true;

    auto hol                = p->spells.templar.hammer_of_light;
    is_hammer_of_light      = true;
    attack_power_mod.direct = hol->effectN( 1 ).ap_coeff();
    aoe                     = 5;
    base_aoe_multiplier     = hol->effectN( 2 ).ap_coeff() / hol->effectN( 1 ).ap_coeff();
  }
  void execute() override
  {
    holy_power_consumer_t::execute();
    p()->trigger_empyrean_hammer(
        target, as<int>( p()->talents.templar.lights_guidance->effectN( 2 ).base_value() ),
        timespan_t::from_millis( p()->talents.templar.lights_guidance->effectN( 4 ).base_value() ), true );
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
    // Since Hammer of Light's damage component is a background action, we need to call reset AA ourselves
    p()->reset_auto_attacks( 0_ms, player->procs.reset_aa_cast );
  }
  void impact( action_state_t* s ) override
  {
    holy_power_consumer_t::impact( s );
    if ( p()->talents.templar.undisputed_ruling->ok() )
    {
      if ( p()->talents.greater_judgment->ok() )
      {
        td( s->target )->debuff.judgment->trigger();
      }
      if ( s->chain_target < 2 )
      {
        p()->buffs.templar.undisputed_ruling->execute();
      }
    }
  }
};

struct hammer_of_light_t : public holy_power_consumer_t<paladin_melee_attack_t>
{
  hammer_of_light_damage_t* direct_hammer;
  double prot_cost;
  double ret_cost;
  hammer_of_light_t( paladin_t* p, util::string_view options_str )
    : holy_power_consumer_t( "hammer_of_light", p, p->spells.templar.hammer_of_light_driver ), direct_hammer()
  {
    parse_options( options_str );
    is_hammer_of_light_driver = true;
    is_hammer_of_light        = true;
    direct_hammer             = new hammer_of_light_damage_t( p, options_str );
    background                = !p->talents.templar.lights_guidance->ok();
    hasted_gcd                = true;
    // This is not set by definition, since cost changes by spec
    resource_current = RESOURCE_HOLY_POWER;
    ret_cost         = data().powerN( 1 ).cost();
    prot_cost        = data().powerN( 2 ).cost();
    add_child( direct_hammer );

    doesnt_consume_dp = !( p->specialization() == PALADIN_PROTECTION && p->bugs );
    hol_cost          = cost();
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
    if ( !(p()->buffs.templar.hammer_of_light_ready->up() || p()->buffs.templar.hammer_of_light_free->up()) )
    {
      return false;
    }
    return paladin_melee_attack_t::target_ready( candidate_target );
   }

   void execute() override
   {
     holy_power_consumer_t<paladin_melee_attack_t>::execute();
     direct_hammer->target = execute_state->target;
     direct_hammer->start_action_execute_event(
         timespan_t::from_millis( p()->spells.templar.hammer_of_light_driver->effectN( 1 ).misc_value1() ) );

    if ( p()->buffs.templar.hammer_of_light_ready->up() )
    {
      p()->buffs.templar.hammer_of_light_ready->expire();
      if (p()->buffs.templar.lights_deliverance->at_max_stacks())
      {
        p()->trigger_lights_deliverance(true);
      }
    }
    else if (p()->buffs.templar.hammer_of_light_free->up())
    {
      p()->buffs.templar.hammer_of_light_free->expire();
    }
    if (p()->talents.templar.zealous_vindication->ok())
    {
      p()->trigger_empyrean_hammer( target, 2, 0_ms, false );
    }
    if ( p()->talents.templar.sacrosanct_crusade->ok() )
    {
      int heal_percent_effect = p()->specialization() == PALADIN_RETRIBUTION ? 5 : 2;
      int additional_heal_per_target_effect = p()->specialization() == PALADIN_RETRIBUTION ? 6 : 3;

      double heal_percent = p()->talents.templar.sacrosanct_crusade->effectN( heal_percent_effect ).percent();
      double additional_heal_per_target =
          p()->talents.templar.sacrosanct_crusade->effectN( additional_heal_per_target_effect ).percent();

      double modifier = heal_percent + std::min(as<int>(p()->sim->target_non_sleeping_list.size()), 5) * additional_heal_per_target;
      double health   = p()->resources.max[ RESOURCE_HEALTH ] * modifier;
      p()->active.sacrosanct_crusade_heal->base_dd_min = p()->active.sacrosanct_crusade_heal->base_dd_max = health;
      p()->active.sacrosanct_crusade_heal->execute();
    }
   }
};


// Empyrean Hammer
struct empyrean_hammer_wd_t : public paladin_spell_t
{
  empyrean_hammer_wd_t( paladin_t* p )
    : paladin_spell_t( "empyrean_hammer_wrathful_descent", p, p->spells.templar.empyrean_hammer_wd )
  {
    background          = true;
    may_crit            = false;
    aoe                 = -1;
    // ToDo (Fluttershy)
    // This spell currently deals full damage to all targets, even above 20.
    // SimC automatically reduces AoE damage above 20 targets, so may need custom execute, if this behaviour stays
    reduced_aoe_targets = -1;
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    paladin_spell_t::available_targets( tl );

    // Does not hit the main target
    auto it = range::find( tl, target );
    if ( it != tl.end() )
    {
      tl.erase( it );
    }

    return tl.size();
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
  empyrean_hammer_t( paladin_t* p ) : paladin_spell_t( "empyrean_hammer", p, p->spells.templar.empyrean_hammer ), wd(nullptr)
  {
    background = proc = may_crit = true;
    may_miss                     = false;
    if ( p->talents.templar.wrathful_descent->ok())
    {
      wd = new empyrean_hammer_wd_t( p );
      add_child( wd );
    }
  }

  void execute() override
  {
    paladin_spell_t::execute();
    if ( p()->talents.templar.lights_deliverance->ok() )
    {
      p()->buffs.templar.lights_deliverance->trigger();
    }
    if ( p()->talents.templar.endless_wrath->ok() )
    {
      if ( p()->cooldowns.endless_wrath_icd->up() )
      {
        if ( p()->buffs.templar.endless_wrath->trigger() )
          p()->cooldowns.hammer_of_wrath->reset( true );
        // ICD starts regardless of whether HoW reset or not
        p()->cooldowns.endless_wrath_icd->start();
      }
      else if ( sim->debug )
        sim->print_debug( "{} tries to trigger Endless Wrath (cooldown)", p()->name_str );
    }
  }

  double action_multiplier() const override
  {
    double am = paladin_spell_t::action_multiplier();
    if ( p()->buffs.templar.sanctification->up() )
    {
      am *= 1.0 + p()->buffs.templar.sanctification->stack_value();
    }
    return am;
  }

  void impact(action_state_t* s) override
  {
    paladin_spell_t::impact( s );
    if ( p()->talents.templar.wrathful_descent->ok() && s->result == RESULT_CRIT && !wd->target_list().empty() )
    {
      wd->base_dd_min = wd->base_dd_max =
          p()->talents.templar.wrathful_descent->effectN( 2 ).percent() * s->result_total;
      wd->execute_on_target( target );
      p()->get_target_data( s->target )->debuff.empyrean_hammer->execute();
    }
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
    {
      int result  = as<int>( std::floor( rng().real() * sim->target_non_sleeping_list.size() ) );
      next_target   = sim->target_non_sleeping_list[ result ];
    }
    make_event<delayed_execute_event_t>( *sim, this, active.empyrean_hammer, next_target, totalDelay );
    totalDelay += additionalDelay;
  }
}

void paladin_t::trigger_lights_deliverance(bool triggered_by_hol)
{
  if ( !talents.templar.lights_deliverance->ok() || !buffs.templar.lights_deliverance->at_max_stacks() )
    return;

  // Light's Deliverance does not trigger while EoT/Wake cooldown is ready to be used
  if ( ( specialization() == PALADIN_PROTECTION && cooldowns.eye_of_tyr->up() ) ||
       ( specialization() == PALADIN_RETRIBUTION && cooldowns.wake_of_ashes->up() ) )
    return;

  if ( !bugs && buffs.templar.hammer_of_light_ready->up() )
    return;
  // 2024-06-25 When we reach 60 stacks of Light's Deliverance while EoT/Wake is already used, but Hammer of Light isn't used yet, the hammer will cost 5 Holy Power
  else if ( bugs && !triggered_by_hol && buffs.templar.hammer_of_light_ready->up() )
  {
    lights_deliverance_triggered_during_ready = true;
    return;
  }
  auto cost_reduction = buffs.templar.hammer_of_light_free->default_value;
  if ( lights_deliverance_triggered_during_ready )
  {
    cost_reduction = 0.0;
    lights_deliverance_triggered_during_ready = false;
  }

  buffs.templar.hammer_of_light_free->execute(-1, cost_reduction, timespan_t::min());
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
      m *= 2.0;
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
      m *= 2.0;
    return m;
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

  void execute( action_t*, action_state_t* s ) override
  {
    if ( s->target->is_enemy() )
    {
      p->active.sacred_weapon_proc_damage->execute_on_target( s->target );
    }
    else
    {
      p->active.sacred_weapon_proc_heal->execute_on_target( s->target );
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
    if ( p() != target )
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
    if ( p() != target )
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
    hasted_gcd         = true;
    name_str_reporting = "Holy Armaments";
    if ( p->talents.lightsmith.forewarning->ok() )
      apply_affecting_aura( p->talents.lightsmith.forewarning );
    background = !p->talents.lightsmith.holy_armaments->ok();
  }

  timespan_t execute_time() const override
  {
    return p()->active.armament[ p()->next_armament ]->execute_time();
  }
  void execute() override
  {
    paladin_spell_t::execute();
    p()->cast_holy_armaments( execute_state->target->is_enemy() ? p() : execute_state->target, p()->next_armament, true,
                              false );
  }
};

void paladin_t::cast_holy_armaments( player_t* target, armament usedArmament, bool changeArmament, bool random )
{
  auto nextArmament = active.armament[ usedArmament ];

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
    else if ( sim->player_no_pet_list.size() > 1 )
    {
      // We try to do this twice for the new option. In case every target is invalid per options.sacred_weapon_prefer_new_targets, we ignore the option and just take the first target we find.
      for ( int i = 0; i < 2; i++ )
      {
        // We do not know who to cast Weapon/Bulwark randomly on. Determine it
        if ( random_weapon_target == nullptr || ( options.sacred_weapon_prefer_new_targets && i == 0 ) )
        {
          player_t* first_dps    = nullptr;
          player_t* first_healer = nullptr;
          player_t* first_tank   = nullptr;
          for ( auto& _p : sim->player_no_pet_list )
          {
            if ( _p->is_sleeping() || _p == this )
              continue;

            // We prefer our random targets to have no buff, this can be done ingame via hugging them over any other
            // target, since it prefers to target closeby targets
            if ( options.sacred_weapon_prefer_new_targets && i == 0 )
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
      }
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
  if ( changeArmament )
    next_armament = armament( ( next_armament + 1 ) % NUM_ARMAMENT );
  if ( random )
    divine_inspiration_next = (divine_inspiration_next + 1) % NUM_ARMAMENT;
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

// Hammer of Wrath

struct hammer_of_wrath_t : public paladin_melee_attack_t
{
private:
  hammer_of_wrath_t* echo;

  hammer_of_wrath_t( paladin_t* p )
    : paladin_melee_attack_t( "hammer_of_wrath_echo", p, p->find_talent_spell( talent_tree::CLASS, "Hammer of Wrath" ) ),
      echo( nullptr )
  {
    background = true;
  }

public:
  hammer_of_wrath_t( paladin_t* p, util::string_view options_str )
    : paladin_melee_attack_t( "hammer_of_wrath", p, p->find_talent_spell( talent_tree::CLASS, "Hammer of Wrath" ) ),
      echo( nullptr )
  {
    parse_options( options_str );

    if ( p->talents.vanguards_momentum->ok() )
    {
      cooldown->charges += as<int>( p->talents.vanguards_momentum->effectN( 1 ).base_value() );

      if ( p->bugs )
      {
        // this is not documented in the tooltip but the spelldata sure shows it
        base_multiplier *= 1.0 + p->talents.vanguards_momentum->effectN( 3 ).percent();
      }
    }

    if ( p->talents.zealots_paragon->ok() )
    {
      base_multiplier *= 1.0 + p->talents.zealots_paragon->effectN( 2 ).percent();
    }

    if ( p->talents.adjudication->ok() )
    {
      add_child( p->active.background_blessed_hammer );
    }
    triggers_higher_calling = true;

    if ( p->talents.herald_of_the_sun.second_sunrise->ok() )
    {
      echo = new hammer_of_wrath_t( p );
      echo->base_multiplier = base_multiplier;
      echo->aoe = aoe;
      echo->base_aoe_multiplier = base_aoe_multiplier;
      echo->crit_bonus_multiplier = crit_bonus_multiplier;
      echo->triggers_higher_calling = true;
      echo->base_multiplier *= p->talents.herald_of_the_sun.second_sunrise->effectN( 2 ).percent();
    }
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( !background && !p()->get_how_availability( candidate_target ) )
    {
      return false;
    }

    return paladin_melee_attack_t::target_ready( candidate_target );
  }

  void execute() override
  {
    paladin_melee_attack_t::execute();

    if ( p()->buffs.final_verdict->up() )
    {
      p()->buffs.final_verdict->expire();
    }
    p()->buffs.templar.endless_wrath->expire();

    if ( p()->buffs.herald_of_the_sun.blessing_of_anshe->up() )
    {
      p()->buffs.herald_of_the_sun.blessing_of_anshe->expire();
    }
  }

  double action_multiplier() const override
  {
    double am = paladin_melee_attack_t::action_multiplier();

    if ( p()->buffs.herald_of_the_sun.blessing_of_anshe->up() )
    {
      am *= 1.0 + p()->buffs.herald_of_the_sun.blessing_of_anshe->data().effectN( 1 ).percent();
    }

    return am;
  }

  void impact( action_state_t* s ) override
  {
    paladin_melee_attack_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    if ( p()->talents.zealots_paragon->ok() )
    {
      auto extension = timespan_t::from_millis( p()->talents.zealots_paragon->effectN( 1 ).base_value() );

      if ( p()->buffs.avenging_wrath->up() )
      {
        p()->buffs.avenging_wrath->extend_duration( p(), extension );
      }

      if ( p()->buffs.crusade->up() )
      {
        p()->buffs.crusade->extend_duration( p(), extension );
      }

      if ( p() ->buffs.sentinel->up())
      {
        p()->buffs.sentinel->extend_duration( p(), extension );
        // 2022-11-14 If Sentinel is still at max stacks, Zealot's Paragon increases decay length, too.
        if ( p()->bugs && p()->buffs.sentinel->at_max_stacks() && p()->buffs.sentinel_decay->up() )
        {
          p()->buffs.sentinel_decay->extend_duration( p(), extension );
        }
      }
    }

    if ( p()->talents.vanguards_momentum->ok() )
    {
      if ( s->target->health_percentage() <= p()->talents.vanguards_momentum->effectN( 2 ).base_value() && s->chain_target == 0 )
      {
        // technically this is in spell 403081 for some reason
        p()->resource_gain( RESOURCE_HOLY_POWER, 1, p()->gains.hp_vm );
      }
    }

    if ( p()->talents.adjudication->ok() )
    {
      // TODO: verify this is right
      double mastery_chance = p()->cache.mastery() * p()->mastery.highlords_judgment->effectN( 4 ).mastery_value();
      if ( p()->talents.boundless_judgment->ok() )
      {
        // according to bolas this also increases HoW proc chance
        mastery_chance *= 1.0 + p()->talents.boundless_judgment->effectN( 3 ).percent();
      }
      if ( p()->talents.highlords_wrath->ok() )
      {
        mastery_chance *= 1.0 + p()->talents.highlords_wrath->effectN( 3 ).percent() / p()->talents.highlords_wrath->effectN( 2 ).base_value();
      }

      if ( rng().roll( mastery_chance ) )
      {
        p()->active.highlords_judgment->set_target( target );
        p()->active.highlords_judgment->execute();
      }
    }

    if ( s->result == RESULT_CRIT && p()->talents.herald_of_the_sun.sun_sear->ok() && p()->specialization() == PALADIN_RETRIBUTION )
    {
      p()->active.sun_sear->target = s->target;
      p()->active.sun_sear->execute();
    }

    if ( echo != nullptr &&
        s->chain_target == 0 &&
        p()->cooldowns.second_sunrise_icd->up() )
    {
      if ( rng().roll( p()->talents.herald_of_the_sun.second_sunrise->effectN( 1 ).percent() ) )
      {
        p()->cooldowns.second_sunrise_icd->start();
        // TODO(mserrano): verify this delay
        echo->target = s->target;
        echo->start_action_execute_event( 200_ms );
      }
    }
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double ctm = paladin_melee_attack_t::composite_target_multiplier( target );

    if ( p()->talents.vengeful_wrath->ok() )
    {
      if ( target->health_percentage() <= p()->talents.vengeful_wrath->effectN( 2 ).base_value() )
      {
        ctm *= 1.0 + p()->talents.vengeful_wrath->effectN( 1 ).percent();
      }
    }

    return ctm;
  }
};

// Shield of the Righteous ==================================================

shield_of_the_righteous_buff_t::shield_of_the_righteous_buff_t( paladin_t* p )
  : buff_t( p, "shield_of_the_righteous", p->spells.sotr_buff )
{
  add_invalidate( CACHE_BONUS_ARMOR );
  set_default_value_from_effect( 3 );
  set_refresh_behavior( buff_refresh_behavior::EXTEND );
  cooldown->duration = 0_ms;  // handled by the ability
  if ( p->sets->has_set_bonus( PALADIN_PROTECTION, TWW1, B2 ) )
  {
    apply_affecting_aura( p->sets->set( PALADIN_PROTECTION, TWW1, B2 ) );
  }
}

void shield_of_the_righteous_buff_t::expire_override( int expiration_stacks, timespan_t remaining_duration )
{
  buff_t::expire_override( expiration_stacks, remaining_duration );

  auto* p = debug_cast<paladin_t*>( player );

  if ( p->talents.inner_light->ok() )
  {
    p->buffs.inner_light->trigger();
  }
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

  forges_reckoning_t* forges_reckoning;
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
    if ( p->sets->has_set_bonus( PALADIN_PROTECTION, TWW1, B2 ) )
    {
      apply_affecting_aura( p->sets->set( PALADIN_PROTECTION, TWW1, B2 ) );
    }

    if ( p->talents.lightsmith.blessing_of_the_forge->ok() )
    {
      forges_reckoning = new forges_reckoning_t( p );
      add_child( forges_reckoning );
    }
  }

  shield_of_the_righteous_t( paladin_t* p )
    : holy_power_consumer_t( "shield_of_the_righteous_vanquishers_hammer", p, p->spec.shield_of_the_righteous )
  {
    // This is the "free" SotR from vanq hammer. Identifiable by being background.
    background        = true;
    aoe               = -1;
    trigger_gcd       = 0_ms;
    weapon_multiplier = 0.0;
  }

  void execute() override
  {
    holy_power_consumer_t::execute();

    // Buff granted regardless of combat roll result
    // Duration and armor bonus recalculation handled in the buff
    p()->buffs.shield_of_the_righteous->trigger();

    if ( p()->talents.redoubt->ok() )
    {
      p()->buffs.redoubt->trigger();
    }

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

    p()->buffs.bulwark_of_righteous_fury->expire();

    if ( p()->buffs.lightsmith.blessing_of_the_forge->up() )
    {
      forges_reckoning->execute_on_target( target );
    }
  }

  double action_multiplier() const override
  {
    double am = holy_power_consumer_t::action_multiplier();
    // Range increase on bulwark of righteous fury not implemented.
    if ( p()->talents.bulwark_of_righteous_fury->ok() )
    {
      am *= 1.0 + p()->buffs.bulwark_of_righteous_fury->stack_value();
    }
    if ( p()->talents.strength_of_conviction->ok() && p()->standing_in_consecration() )
    {
      am *= 1.0 + p()->talents.strength_of_conviction->effectN( 1 ).percent();
    }
    return am;
  }
};

struct incandescence_t : public paladin_spell_t
{
  incandescence_t( paladin_t* p ) : paladin_spell_t( "incandescence", p, p->find_spell( 385816 ) )
  {
    background = true;
    aoe = 5;
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
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    paladin_spell_t::available_targets( tl );

    for ( size_t i = 0; i < tl.size(); i++ )
    {
      if ( tl[ i ] == target )
      {
        tl.erase( tl.begin() + i );
        break;
      }
    }
    return tl.size();
  }
};

struct dawnlight_t : public paladin_spell_t
{
  dawnlight_aoe_t* aoe_action;

  dawnlight_t( paladin_t* p ) :
    paladin_spell_t( "dawnlight", p, p->find_spell( 431380 ) ),
    aoe_action( new dawnlight_aoe_t( p ) )

  {
    background = true;
    affected_by.highlords_judgment = true;
    tick_may_crit = true;
    dot_behavior = dot_behavior_e::DOT_EXTEND; // per bolas test Aug 21 2024
  }

  void execute() override
  {
    paladin_spell_t::execute();

    if ( p()->talents.herald_of_the_sun.solar_grace->ok() )
      p()->buffs.herald_of_the_sun.solar_grace->trigger();

    if ( p()->buffs.herald_of_the_sun.morning_star->up() )
      p()->buffs.herald_of_the_sun.morning_star->expire();

    if ( p()->talents.herald_of_the_sun.gleaming_rays->ok() )
      p()->buffs.herald_of_the_sun.gleaming_rays->trigger();

    if ( p()->talents.herald_of_the_sun.suns_avatar->ok() )
    {
      if ( ( !p()->buffs.herald_of_the_sun.suns_avatar->up() ) && ( p()->buffs.crusade->up() || p()->buffs.avenging_wrath->up() ) )
      {
        p()->buffs.herald_of_the_sun.suns_avatar->trigger();
      }
    }
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    double cpm = paladin_spell_t::composite_persistent_multiplier( s );

    if ( p()->buffs.herald_of_the_sun.morning_star->up() )
      cpm *= 1.0 + p()->buffs.herald_of_the_sun.morning_star->stack_value();

    return cpm;
  }

  double dot_duration_pct_multiplier( const action_state_t* s ) const override
  {
    double mul = paladin_spell_t::dot_duration_pct_multiplier( s );

    if ( p()->talents.herald_of_the_sun.suns_avatar->ok() )
    {
      if ( p()->buffs.avenging_wrath->up() || p()->buffs.crusade->up() )
      {
        mul *= 1.0 + p()->talents.herald_of_the_sun.suns_avatar->effectN( 5 ).percent();
      }
    }

    return mul;
  }

  void tick( dot_t* d ) override
  {
    paladin_spell_t::tick( d );

    aoe_action->base_dd_min = aoe_action->base_dd_max = d->state->result_amount * p()->spells.herald_of_the_sun.dawnlight_aoe_metadata->effectN( 1 ).percent();
    aoe_action->execute_on_target( d->target );
  }

  void last_tick( dot_t* d ) override
  {
    paladin_spell_t::last_tick( d );

    unsigned num_dawnlights = p()->get_active_dots( d );
    if ( num_dawnlights == 0 )
    {
      if ( p()->talents.herald_of_the_sun.suns_avatar->ok() )
      {
        p()->buffs.herald_of_the_sun.suns_avatar->expire();
      }

      if ( p()->talents.herald_of_the_sun.gleaming_rays->ok() )
      {
        p()->buffs.herald_of_the_sun.gleaming_rays->expire();
      }
    }

    if ( p()->talents.herald_of_the_sun.lingering_radiance->ok() )
    {
      paladin_td_t* target_data = td( d->target );
      target_data->debuff.judgment->trigger();
    }
  }
};

void paladin_t::apply_avatar_dawnlights()
{
  if ( !talents.herald_of_the_sun.suns_avatar->ok() )
    return;

  unsigned num_dawnlights = (unsigned) as<int>( talents.herald_of_the_sun.suns_avatar->effectN( 3 ).base_value() );

  // per bolas Aug 21 2024. Can't seem to find this in spelldata
  if ( !talents.crusade->ok() )
    num_dawnlights = 2;
  if ( talents.radiant_glory->ok() )
    num_dawnlights = 1;

  std::vector<player_t*> tl_candidates;
  std::vector<player_t*> tl_yes_dawnlight;
  for ( auto* t : sim->target_non_sleeping_list )
  {
    if ( t->is_enemy() )
    {
      auto* dawnlight = active.dawnlight->get_dot( t );
      if ( dawnlight->is_ticking() )
        tl_yes_dawnlight.push_back( t );
      else
        tl_candidates.push_back( t );
    }
  }

  if ( tl_candidates.size() < num_dawnlights )
  {
    auto needed = num_dawnlights - tl_candidates.size();

    std::stable_sort( tl_yes_dawnlight.begin(), tl_yes_dawnlight.end(), [this]( player_t* a, player_t* b ){
      auto* dla = active.dawnlight->get_dot( a );
      auto* dlb = active.dawnlight->get_dot( b );
      return dla->remains() < dlb->remains();
    } );

    auto have = tl_yes_dawnlight.size();
    auto num_extra = have < needed ? have : needed;

    for ( auto i = 0u; i < num_extra; i++ )
      tl_candidates.push_back( tl_yes_dawnlight[ i ] );
  }

  auto have_total = tl_candidates.size();
  if ( num_dawnlights < have_total )
    have_total = num_dawnlights;

  for ( auto i = 0u; i < have_total; i++ )
  {
    active.dawnlight->execute_on_target( tl_candidates[ i ] );
  }

  if ( bugs && have_total > 0 && talents.radiant_glory->ok() )
  {
    active.dawnlight->execute_on_target( tl_candidates[ 0 ] );
  }
}

struct suns_avatar_dmg_t : public paladin_spell_t
{
  suns_avatar_dmg_t( paladin_t* p ) : paladin_spell_t( "suns_avatar", p, p->find_spell( 431911 ) )
  {
    background = true;
    aoe = -1;
    reduced_aoe_targets = p->talents.herald_of_the_sun.suns_avatar->effectN( 6 ).base_value();
  }
};

void paladin_t::tww1_4p_prot()
{
  if ( !sets->has_set_bonus( PALADIN_PROTECTION, TWW1, B4 ) )
    return;

  auto stack_value = buffs.rising_wrath->stack_value() / 1000.0;
  buffs.rising_wrath->expire();
  buffs.heightened_wrath->execute( -1, stack_value, timespan_t::min() );
}

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
struct blessing_of_sacrifice_t : public buff_t
{
  paladin_t* source;  // Assumption: Only one paladin can cast HoS per target
  double source_health_pool;

  blessing_of_sacrifice_t( player_t* p )
    : buff_t( p, "blessing_of_sacrifice", p->find_spell( 6940 ) ), source( nullptr ), source_health_pool( 0.0 )
  {
  }

  // Trigger function for the paladin applying HoS on the target
  bool trigger_hos( paladin_t& source )
  {
    if ( this->source )
      return false;

    this->source       = &source;
    source_health_pool = source.resources.max[ RESOURCE_HEALTH ];

    return buff_t::trigger( 1 );
  }

  // Misuse functions as the redirect callback for damage onto the source
  bool trigger( int, double value, double, timespan_t ) override
  {
    assert( source );

    value = std::min( source_health_pool, value );
    source->active.blessing_of_sacrifice_redirect->trigger( value );
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

    source             = nullptr;
    source_health_pool = 0.0;
  }

  void reset() override
  {
    buff_t::reset();

    source             = nullptr;
    source_health_pool = 0.0;
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

  auto* b = debug_cast<buffs::blessing_of_sacrifice_t*>( target->buffs.blessing_of_sacrifice );

  b->trigger_hos( *p() );
}

// ==========================================================================
// Paladin Character Definition
// ==========================================================================

paladin_td_t::paladin_td_t( player_t* target, paladin_t* paladin ) : actor_target_data_t( target, paladin )
{
  debuff.blessed_hammer        = make_buff( *this, "blessed_hammer", paladin->find_spell( 204301 ) );
  debuff.execution_sentence    = make_buff<buffs::execution_sentence_debuff_t>( this );

  debuff.judgment              = make_buff( *this, "judgment", paladin->spells.judgment_debuff )
                                 ->set_default_value_from_effect( 1 )
                                 ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );

  if (paladin->specialization() == PALADIN_PROTECTION)
  {
    debuff.judgment->apply_affecting_aura( paladin->spec.protection_paladin );
  }

  debuff.judgment_of_light     = make_buff( *this, "judgment_of_light", paladin->find_spell( 196941 ) );

  debuff.final_reckoning       = make_buff( *this, "final_reckoning", paladin->talents.final_reckoning )
                                ->set_cooldown( 0_ms );  // handled by ability
  if ( paladin->talents.executioners_will->ok() )
  {
    debuff.final_reckoning = debuff.final_reckoning->modify_duration( timespan_t::from_millis( paladin->talents.executioners_will->effectN( 1 ).base_value() ) );
  }

  debuff.sanctify              = make_buff( *this, "sanctify", paladin->find_spell( 382538 ) );
  debuff.eye_of_tyr            = make_buff( *this, "eye_of_tyr", paladin->find_spell( 387174 ) )
                                ->set_cooldown( 0_ms );
  debuff.crusaders_resolve     = make_buff( *this, "crusaders_resolve", paladin->find_spell( 383843 ) );
  debuff.heartfire = make_buff( *this, "heartfire", paladin-> find_spell( 408461 ) );
  debuff.empyrean_hammer = make_buff( *this, "empyrean_hammer", paladin->find_spell( 431625 ) );
  debuff.vanguard_of_justice = make_buff( *this, "vanguard_of_justice", paladin->find_spell( 453451 ) );

  buffs.holy_bulwark = make_buff<buffs::holy_bulwark_buff_t>( this )
    ->set_cooldown( 0_s );
  buffs.sacred_weapon = make_buff( *this, "sacred_weapon_" + paladin->name_str + "_" + target->name_str, paladin->find_spell( 432502 ) );

  if ( !target->is_enemy() && target != paladin )
  {
    auto cb = paladin->create_sacred_weapon_callback( paladin, target );
    cb->activate_with_buff( buffs.sacred_weapon, true );
  }

  dots.expurgation = target->get_dot( "expurgation", paladin );
  dots.truths_wake = target->get_dot( "truths_wake", paladin );
  dots.dawnlight = target->get_dot( "dawnlight", paladin );
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

    // Prot only but handled in the core paladin module
    if ( talents.holy_shield->ok() )
    {
      active.holy_shield_damage = new holy_shield_damage_t( this );
    }
    if ( talents.inner_light -> ok() )
    {
      active.inner_light_damage = new inner_light_damage_t( this );
    }
  }
  // Ret
  else if ( specialization() == PALADIN_RETRIBUTION )
  {
    paladin_t::create_ret_actions();
  }
  // Hero Talents
  //Lightsmith
  if ( talents.lightsmith.holy_armaments->ok() )
  {
    auto cb = create_sacred_weapon_callback(this, this);
    cb->activate_with_buff( buffs.lightsmith.sacred_weapon, true );
    active.sacred_weapon_proc_damage = new sacred_weapon_proc_damage_t( this );
    active.sacred_weapon_proc_heal   = new sacred_weapon_proc_heal_t( this );
  }
  //Templar
  if (talents.templar.lights_guidance->ok())
  {
    active.empyrean_hammer = new empyrean_hammer_t(this);
  }
  if (talents.templar.sacrosanct_crusade->ok())
  {
    active.sacrosanct_crusade_heal = new sacrosanct_crusade_heal_t( this );
  }

  if ( talents.herald_of_the_sun.dawnlight->ok() )
  {
    active.dawnlight = new dawnlight_t( this );
  }
  if ( talents.herald_of_the_sun.suns_avatar->ok() )
  {
    active.suns_avatar_dmg = new suns_avatar_dmg_t( this );
  }

  active.shield_of_vengeance_damage = new shield_of_vengeance_proc_t( this );

  if ( talents.judgment_of_light->ok() )
  {
    active.judgment_of_light = new judgment_of_light_proc_t( this );
    cooldowns.judgment_of_light_icd->duration =
        timespan_t::from_seconds( talents.judgment_of_light->effectN( 1 ).base_value() );
  }

  active.seasons[ SUMMER ] = new blessing_of_summer_t( this );
  active.seasons[ AUTUMN ] = new blessing_of_autumn_t( this );
  active.seasons[ WINTER ] = new blessing_of_winter_t( this );
  active.seasons[ SPRING ] = new blessing_of_spring_t( this );

  active.armament[ HOLY_BULWARK ]  = new holy_bulwark_t( this );
  active.armament[ SACRED_WEAPON ] = new sacred_weapon_t( this );

  if ( talents.incandescence->ok() )
  {
    active.incandescence = new incandescence_t( this );
  }
  else
  {
    active.incandescence = nullptr;
  }

  active.background_cons = new consecration_t( this, "blade_of_justice", BLADE_OF_JUSTICE );
  active.searing_light_cons = new consecration_t( this, "searing_light", SEARING_LIGHT );

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
  if ( name == "holy_avenger" )
    return new holy_avenger_t( this, options_str );
  if ( name == "hammer_of_wrath" )
    return new hammer_of_wrath_t( this, options_str );
  if ( name == "devotion_aura" )
    return new devotion_aura_t( this, options_str );
  if ( name == "retribution_aura" )
    return new retribution_aura_t( this, options_str );
  if ( name == "divine_toll" )
    return new divine_toll_t( this, options_str );
  if ( name == "blessing_of_the_seasons" )
    return new blessing_of_the_seasons_t( this, options_str );
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
  buff->trigger();
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

  player_t::init_base_stats();

  base.attack_power_per_agility  = 0.0;
  base.attack_power_per_strength = 1.0;
  base.spell_power_per_intellect = 1.0;

  // Boundless Conviction raises max holy power to 5
  resources.base[ RESOURCE_HOLY_POWER ] = 3 + passives.boundless_conviction->effectN( 1 ).base_value();

  // Ignore mana for non-holy
  if ( specialization() != PALADIN_HOLY )
  {
    resources.base[ RESOURCE_MANA ]                  = 0;
    resources.base_regen_per_second[ RESOURCE_MANA ] = 0;
  }

  // Avoidance diminishing Returns constants/conversions now handled in player_t::init_base_stats().
  // Base miss, dodge, parry, and block are set in player_t::init_base_stats().
  // Just need to add class- or spec-based modifiers here.
  // add Sanctuary dodge
  base.dodge += passives.sanctuary->effectN( 3 ).percent();
  // add Sanctuary expertise
  base.expertise += passives.sanctuary->effectN( 4 ).percent();

  // Paladins gets +7% block from their class aura
  base.block += passives.paladin->effectN( 7 ).percent();
}

// paladin_t::reset =========================================================

void paladin_t::reset()
{
  player_t::reset();

  active_consecration = nullptr;
  active_boj_cons = nullptr;
  active_searing_light_cons = nullptr;
  all_active_consecrations.clear();
  active_hallow_damaging       = nullptr;
  active_hallow_healing     = nullptr;
  active_aura         = nullptr;

  next_season = SUMMER;
  next_armament = SACRED_WEAPON;
  radiant_glory_accumulator = 0.0;
  holy_power_generators_used = 0;
  melee_swing_count = 0;
  lights_deliverance_triggered_during_ready = false;
  random_weapon_target = nullptr;
  random_bulwark_target = nullptr;
  divine_inspiration_next = -1;
}

// paladin_t::init_gains ====================================================

void paladin_t::init_gains()
{
  player_t::init_gains();

  // Mana
  gains.mana_beacon_of_light = get_gain( "beacon_of_light" );

  // Health
  gains.holy_shield   = get_gain( "holy_shield_absorb" );
  gains.bulwark_of_order = get_gain( "bulwark_of_order_absorb" );
  gains.sacrosanct_crusade = get_gain( "sacrosanct_crusade_absorb" );
  gains.moment_of_glory  = get_gain( "moment_of_glory_absorb" );


  // Holy Power
  gains.hp_templars_verdict_refund = get_gain( "templars_verdict_refund" );
  gains.judgment                   = get_gain( "judgment" );
  gains.hp_cs                      = get_gain( "crusader_strike" );
  gains.hp_divine_toll             = get_gain( "divine_toll" );
  gains.hp_vm                      = get_gain( "vanguards_momentum" );
  gains.hp_crusading_strikes       = get_gain( "crusading_strikes" );
  gains.hp_divine_auxiliary        = get_gain( "divine_auxiliary" );
  gains.eye_of_tyr                 = get_gain( "eye_of_tyr" );
}

// paladin_t::init_procs ====================================================

void paladin_t::init_procs()
{
  player_t::init_procs();

  procs.art_of_war        = get_proc( "Art of War" );
  procs.righteous_cause   = get_proc( "Righteous Cause" );
  procs.divine_purpose    = get_proc( "Divine Purpose" );
  procs.final_reckoning   = get_proc( "Final Reckoning" );
  procs.empyrean_power    = get_proc( "Empyrean Power" );

  procs.as_grand_crusader         = get_proc( "Avenger's Shield: Grand Crusader" );
  procs.as_grand_crusader_wasted  = get_proc( "Avenger's Shield: Grand Crusader wasted" );
  procs.as_engraved_sigil         = get_proc( "Avenger's Shield: Engraved Sigil" );
  procs.as_engraved_sigil_wasted  = get_proc( "Avenger's Shield: Engraved Sigil wasted" );
  procs.as_moment_of_glory        = get_proc( "Avenger's Shield: Moment of Glory" );
  procs.as_moment_of_glory_wasted = get_proc( "Avenger's Shield: Moment of Glory wasted" );

  procs.divine_inspiration = get_proc( "Divine Inspiration" );
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
  buffs.avenging_wrath = new buffs::avenging_wrath_buff_t( this );
  buffs.avenging_wrath->set_expire_callback( [ this ]( buff_t*, double, timespan_t ) {
    buffs.heightened_wrath->expire();
    buffs.herald_of_the_sun.suns_avatar->expire();
  } );
  //.avenging_wrath_might = new buffs::avenging_wrath_buff_t( this );
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
  buffs.holy_avenger =
      make_buff( this, "holy_avenger", talents.holy_avenger )->set_cooldown( 0_ms );  // handled by the ability
  buffs.devotion_aura = make_buff( this, "devotion_aura", find_spell( 465 ) )
                            ->set_default_value( find_spell( 465 )->effectN( 1 ).percent() );

  buffs.blessing_of_dawn =
      make_buff( this, "blessing_of_dawn", find_spell( 385127 ) )->set_default_value_from_effect( 1 );
  if ( specialization() == PALADIN_RETRIBUTION )
  {
    buffs.blessing_of_dawn->apply_affecting_aura( spec.retribution_paladin );
  }
  buffs.blessing_of_dusk = make_buff( this, "blessing_of_dusk", find_spell( 385126 ) );
  buffs.faiths_armor     = make_buff( this, "faiths_armor", find_spell( 379017 ) )
                           ->set_default_value_from_effect( 1 )
                           ->add_invalidate( CACHE_BONUS_ARMOR );

  if ( talents.seal_of_order->ok() )
  {
    buffs.blessing_of_dusk->set_stack_change_callback( [ this ]( buff_t* b, int, int new_ ) {
      for ( auto a : action_list )
      {
        if ( a->cooldown->duration == 0_ms )
          continue;

        // per bolas (Aug 19 2024) Wake is unaffected in spite of being in the spelldata
        if ( a->data().id() == 255937 )
         continue;

        bool already_done = false;
        for ( size_t i = 3; i < 14; i++ )
        {
          // spell effect 13, for Eye of Tyr, only applies if we have undisputed ruling
          if ( i == 13 && !talents.templar.undisputed_ruling->ok() )
            continue;

          spelleffect_data_t label = b->data().effectN( i );
          if ( label.subtype() != A_MOD_RECHARGE_TIME_PCT_CATEGORY )
            continue;
          if ( a->data().affected_by( label ) || a->data().affected_by_category( label ) )
          {
            if ( new_ == 1 )
              a->dynamic_recharge_multiplier *= 1.0 + label.percent();
            else
              a->dynamic_recharge_multiplier /= 1.0 + label.percent();

            if ( a->cooldown->action == a )
              a->cooldown->adjust_recharge_multiplier();
            if ( a->internal_cooldown->action == a )
              a->internal_cooldown->adjust_recharge_multiplier();

            already_done = true;

            // empirically no spell seems to be affected by multiple of these
            break;
          }
        }

        // For some reason some spells are in both lists. Empirically it seems like the
        // "displayed" CDR takes precedence
        if ( already_done )
          continue;

        for ( size_t i = 3; i < 14; i++ )
        {
          // Spell effect 9, for ES/FR, only applies if we have divine auxiliary talented
          if ( i == 9 && !talents.divine_auxiliary->ok() )
            continue;

          spelleffect_data_t label = b->data().effectN( i );
          if ( label.subtype() == A_MOD_RECHARGE_TIME_PCT_CATEGORY )
            continue;

          double recharge_mult = 1.0 / ( 1.0 + label.percent() );
          if ( a->data().affected_by( label ) || a->data().affected_by_category( label ) )
          {
            if ( new_ == 1 )
              a->dynamic_recharge_rate_multiplier *= recharge_mult;
            else
              a->dynamic_recharge_rate_multiplier /= recharge_mult;

            if ( a->cooldown->action == a )
              a->cooldown->adjust_recharge_multiplier();
            if ( a->internal_cooldown->action == a )
              a->internal_cooldown->adjust_recharge_multiplier();
          }
        }
      }
    } );
  }

  buffs.relentless_inquisitor = make_buff( this, "relentless_inquisitor", find_spell( 383389 ) )
                                    ->set_default_value( find_spell( 383389 )->effectN( 1 ).percent() )
                                    ->add_invalidate( CACHE_HASTE );

  if ( talents.relentless_inquisitor->ok() )
  {
    buffs.relentless_inquisitor->set_max_stack( as<int>( talents.relentless_inquisitor->effectN( 2 ).base_value() +
                                                         talents.relentless_inquisitor->effectN( 3 ).base_value() ) );
  }

  buffs.final_verdict = make_buff( this, "final_verdict", find_spell( 337228 ) );
  buffs.divine_resonance =
      make_buff( this, "divine_resonance", find_spell( 355455 ) )
          ->set_tick_callback( [ this ]( buff_t* /* b */, int /* stacks */, timespan_t /* tick_time */ ) {
            this->active.divine_resonance->set_target( this->target );
            this->active.divine_resonance->schedule_execute();
          } );

  buffs.lightsmith.holy_bulwark = make_buff<buffs::holy_bulwark_buff_t>( this )
                                      ->set_cooldown( 0_s )
                                      ->set_refresh_duration_callback( [ this ]( const buff_t* b, timespan_t d ) {
                                        if ( b->remains().total_millis() > 0 )
                                          trigger_laying_down_arms();
                                        timespan_t residual = std::min( d * 0.3, b->remains() );
                                        return residual + d;
                                      } )
                                      ->set_expire_callback( [ this ]( buff_t*, double, timespan_t ) {
                                        trigger_laying_down_arms();
                                      } );
  buffs.lightsmith.sacred_weapon = make_buff( this, "sacred_weapon", find_spell( 432502 ) )
                                       ->set_refresh_duration_callback( [ this ]( const buff_t* b, timespan_t d ) {
                                         if ( b->remains().total_millis() > 0 )
                                           trigger_laying_down_arms();
                                         timespan_t residual = std::min( d * 0.3, b->remains() );
                                         return residual + d;
                                       } )
                                       ->set_expire_callback( [ this ]( buff_t*, double, timespan_t ) {
                                         trigger_laying_down_arms();
                                       } );
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
                                                   cast_holy_armaments( this, armament::SACRED_WEAPON, false, false );
                                               } );


  buffs.templar.hammer_of_light_ready =
      make_buff( this, "hammer_of_light_ready", find_spell( 427453 ) )
          ->set_duration( 12_s )
          ->set_expire_callback( [ this ]( buff_t*, double, timespan_t ) { trigger_lights_deliverance();
        });
  buffs.templar.hammer_of_light_free =
      make_buff( this, "hammer_of_light_free", find_spell( 433732 ) )->set_duration( 12_s )->set_default_value_from_effect(1);

  buffs.templar.for_whom_the_bell_tolls = make_buff( this, "for_whom_the_bell_tolls", find_spell( 433618 ) );
  buffs.templar.for_whom_the_bell_tolls->set_initial_stack( buffs.templar.for_whom_the_bell_tolls->max_stack() );

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
  buffs.templar.endless_wrath = make_buff( this, "endless_wrath", find_spell( 452244 ) )
                                    ->set_chance( talents.templar.endless_wrath->effectN( 1 ).percent() );
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

  buffs.herald_of_the_sun.morning_star_driver = make_buff( this, "morning_star_driver", find_spell( 431568 ) )
    ->set_period( timespan_t::from_seconds( 5.0 ) ) // TODO(mserrano) grab from spell data
    ->set_quiet( true )
    ->set_tick_callback([this](buff_t*, int, const timespan_t&) { buffs.herald_of_the_sun.morning_star->trigger(); })
    ->set_tick_time_behavior( buff_tick_time_behavior::UNHASTED );
  buffs.herald_of_the_sun.morning_star = make_buff( this, "morning_star", find_spell( 431539 ) )
    ->set_default_value_from_effect( 1 );
  buffs.herald_of_the_sun.gleaming_rays = make_buff( this, "gleaming_rays", spells.herald_of_the_sun.gleaming_rays )
    ->set_duration( timespan_t::zero() ); // infinite duration
  auto blessing_of_anshe_id = specialization() == PALADIN_RETRIBUTION ? 445206 : 445204;
  buffs.herald_of_the_sun.blessing_of_anshe = make_buff( this, "blessing_of_anshe", find_spell( blessing_of_anshe_id ) );
  buffs.herald_of_the_sun.solar_grace = make_buff( this, "solar_grace", find_spell( 439841 ) )
    -> add_invalidate( CACHE_HASTE )
    -> set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );
  buffs.herald_of_the_sun.dawnlight = make_buff( this, "dawnlight", find_spell( 431522 ) )
    -> set_max_stack( 2 );
  buffs.herald_of_the_sun.suns_avatar = make_buff( this, "suns_avatar", find_spell( 431907 ) )
    ->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
        active.suns_avatar_dmg->execute_on_target( target );
      });

  buffs.rising_wrath = make_buff( this, "rising_wrath", find_spell( 456700 ) )
    ->set_default_value_from_effect(1);
  buffs.heightened_wrath = make_buff( this, "heightened_wrath", find_spell( 456759 ) );
}

// paladin_t::default_potion ================================================

std::string paladin_t::default_potion() const
{
  std::string retribution_pot = ( true_level > 70 ) ? "tempered_potion_3" : "disabled";

  std::string protection_pot = ( true_level > 70 ) ? "tempered_potion_3" : "disabled";

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
  std::string retribution_food = ( true_level > 70 ) ? "the_sushi_special" : "disabled";

  std::string protection_food = ( true_level > 70 ) ? "feast_of_the_divine_day" : "disabled";

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
  std::string retribution_flask = ( true_level > 70 ) ? "flask_of_alchemical_chaos_3" : "disabled";

  std::string protection_flask = ( true_level > 70 ) ? "flask_of_alchemical_chaos_3" : "disabled";

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
  return ( true_level > 70 ) ? "crystallized" : "disabled";
}

// paladin_t::default_temporary_enchant ================================

std::string paladin_t::default_temporary_enchant() const
{
  switch ( specialization() )
  {
    case PALADIN_PROTECTION:
      return "main_hand:algari_mana_oil_3,if=!(talent.rite_of_adjuration.enabled|talent.rite_of_sanctification.enabled)";
    case PALADIN_RETRIBUTION:
      return "main_hand:ironclaw_whetstone_3";

    default:
      return "main_hand:howling_rune_3";
  }
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

void paladin_t::init_special_effects()
{
  player_t::init_special_effects();

  if ( talents.touch_of_light->ok() )
  {
    auto const touch_of_light_driver = new special_effect_t( this );
    touch_of_light_driver->name_str = "touch_of_light_driver";
    touch_of_light_driver->spell_id = 385349;
    special_effects.push_back( touch_of_light_driver );

    auto cb = new paladin::touch_of_light_cb_t( this, *touch_of_light_driver );
    cb->initialize();
  }

  if ( talents.herald_of_the_sun.blessing_of_anshe->ok() )
  {
    struct blessing_of_anshe_cb_t : public dbc_proc_callback_t
    {
      paladin_t* p;

      blessing_of_anshe_cb_t( paladin_t* player, const special_effect_t& effect )
        : dbc_proc_callback_t( player, effect ), p( player )
      {
      }

      void execute( action_t*, action_state_t* ) override
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

      void execute( action_t*, action_state_t* ) override
      {
        p->cast_holy_armaments( p, paladin::armament::SACRED_WEAPON, false, true );
        p->procs.divine_inspiration->occur();
      }
    };

    auto const divine_inspiration_driver = new special_effect_t( this );
    divine_inspiration_driver->name_str  = "divine_inspiration_driver";
    // Since proc chance is hidden, this is just a guess. Average proc rate seems to match, though
    divine_inspiration_driver->ppm_        = -1.2;
    divine_inspiration_driver->rppm_scale_ = RPPM_HASTE;
    divine_inspiration_driver->type        = SPECIAL_EFFECT_EQUIP;
    divine_inspiration_driver->proc_flags_ =
        PF_MELEE_ABILITY | PF_RANGED | PF_RANGED_ABILITY | PF_NONE_SPELL | PF_MAGIC_SPELL | PF_ALL_HEAL;
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
  player_t::init_spells();

  init_spells_retribution();
  init_spells_protection();
  init_spells_holy();

  // Shared talents
  talents.lay_on_hands                    = find_talent_spell( talent_tree::CLASS, "Lay on Hands" );
  talents.blessing_of_freedom             = find_talent_spell( talent_tree::CLASS, "Blessing of Freedom" );
  talents.hammer_of_wrath                 = find_talent_spell( talent_tree::CLASS, "Hammer of Wrath" );
  talents.auras_of_the_resolute           = find_talent_spell( talent_tree::CLASS, "Auras of the Resolute" );
  talents.auras_of_swift_vengeance        = find_talent_spell( talent_tree::CLASS, "Auras of Swift Vengeance" );
  talents.blinding_light                  = find_talent_spell( talent_tree::CLASS, "Blinding Light" );
  talents.repentance                      = find_talent_spell( talent_tree::CLASS, "Repentance" );
  talents.divine_steed                    = find_talent_spell( talent_tree::CLASS, 190784 );
  talents.fist_of_justice                 = find_talent_spell( talent_tree::CLASS, "Fist of Justice" );
  talents.holy_aegis                      = find_talent_spell( talent_tree::CLASS, "Holy Aegis" );
  talents.cavalier                        = find_talent_spell( talent_tree::CLASS, "Cavalier" );
  talents.seasoned_warhorse               = find_talent_spell( talent_tree::CLASS, "Seasoned Warhorse" );
  talents.seal_of_alacrity                = find_talent_spell( talent_tree::CLASS, "Seal of Alacrity" );
  talents.golden_path                     = find_talent_spell( talent_tree::CLASS, "Golden Path" );
  talents.judgment_of_light               = find_talent_spell( talent_tree::CLASS, "Judgment of Light" );
  //Avenging Wrath spell
  talents.avenging_wrath                  = find_talent_spell( talent_tree::CLASS, "Avenging Wrath" );
  talents.turn_evil                       = find_talent_spell( talent_tree::CLASS, "Turn Evil" );
  talents.rebuke                          = find_talent_spell( talent_tree::CLASS, "Rebuke" );
  talents.seal_of_mercy                   = find_talent_spell( talent_tree::CLASS, "Seal of Mercy" );
  talents.cleanse_toxins                  = find_talent_spell( talent_tree::CLASS, "Cleanse Toxins" );
  talents.blessing_of_sacrifice           = find_talent_spell( talent_tree::CLASS, "Blessing of Sacrifice" );
  //Judgment causes the target to take 25% more damage from your next holy power spending ability
  talents.greater_judgment                = find_talent_spell( talent_tree::CLASS, "Greater Judgment" );
  talents.afterimage                      = find_talent_spell( talent_tree::CLASS, "Afterimage" );
  talents.recompense                      = find_talent_spell( talent_tree::CLASS, "Recompense" );
  talents.sacrifice_of_the_just           = find_talent_spell( talent_tree::CLASS, "Sacrifice of the Just" );
  talents.blessing_of_protection          = find_talent_spell( talent_tree::CLASS, "Blessing of Protection" );
  talents.holy_avenger                    = find_talent_spell( talent_tree::CLASS, "Holy Avenger" );
  talents.divine_purpose                  = find_talent_spell( talent_tree::CLASS, "Divine Purpose" );
  talents.obduracy                        = find_talent_spell( talent_tree::CLASS, "Obduracy" );
  talents.touch_of_light                  = find_talent_spell( talent_tree::CLASS, "Touch of Light" );
  talents.incandescence                   = find_talent_spell( talent_tree::CLASS, "Incandescence" );
  talents.of_dusk_and_dawn                = find_talent_spell( talent_tree::CLASS, "Of Dusk and Dawn" );
  talents.unbreakable_spirit              = find_talent_spell( talent_tree::CLASS, "Unbreakable Spirit" );
  talents.seal_of_might                   = find_talent_spell( talent_tree::CLASS, "Seal of Might" );
  talents.improved_blessing_of_protection = find_talent_spell( talent_tree::CLASS, "Improved Blessing of Protection" );
  talents.seal_of_the_crusader            = find_talent_spell( talent_tree::CLASS, "Seal of the Crusader" );
  talents.seal_of_order                   = find_talent_spell( talent_tree::CLASS, "Seal of Order" );
  talents.zealots_paragon                 = find_talent_spell( talent_tree::CLASS, "Zealot's Paragon" );

  // spec talents shared among specs
  talents.avenging_wrath_might  = find_talent_spell( talent_tree::SPECIALIZATION, "Avenging Wrath: Might" );
  talents.relentless_inquisitor = find_talent_spell( talent_tree::SPECIALIZATION, "Relentless Inquisitor" );

  talents.divine_toll      = find_talent_spell( talent_tree::CLASS, "Divine Toll" );
  talents.divine_resonance = find_talent_spell( talent_tree::CLASS, "Divine Resonance" );
  talents.quickened_invocation = find_talent_spell( talent_tree::CLASS, "Quickened Invocation" );
  talents.faiths_armor         = find_talent_spell( talent_tree::CLASS, "Faith's Armor" );
  talents.strength_of_conviction = find_talent_spell( talent_tree::CLASS, "Strength of Conviction" );

  talents.justification = find_talent_spell( talent_tree::CLASS, "Justification" );
  talents.punishment = find_talent_spell( talent_tree::CLASS, "Punishment" );
  talents.sanctified_plates = find_talent_spell( talent_tree::CLASS, "Sanctified Plates" );
  talents.lightforged_blessing = find_talent_spell( talent_tree::CLASS, "Lightforged Blessing" );
  talents.crusaders_reprieve = find_talent_spell( talent_tree::CLASS, "Crusader's Reprieve" );
  talents.fading_light = find_talent_spell( talent_tree::CLASS, "Fading Light" );

  // Hero Talents
  talents.lightsmith.holy_armaments         = find_talent_spell( talent_tree::HERO, "Holy Armaments" );

  talents.lightsmith.rite_of_sanctification = find_talent_spell( talent_tree::HERO, "Rite of Sanctification" );
  talents.lightsmith.rite_of_adjuration     = find_talent_spell( talent_tree::HERO, "Rite of Adjuration" );
  talents.lightsmith.solidarity             = find_talent_spell( talent_tree::HERO, "Solidarity" );
  talents.lightsmith.divine_guidance        = find_talent_spell( talent_tree::HERO, "Divine Guidance" );
  talents.lightsmith.blessed_assurance      = find_talent_spell( talent_tree::HERO, "Blessed Assurance" );

  talents.lightsmith.laying_down_arms       = find_talent_spell( talent_tree::HERO, "Laying Down Arms" );
  talents.lightsmith.divine_inspiration     = find_talent_spell( talent_tree::HERO, "Divine Inspiration" );
  talents.lightsmith.forewarning            = find_talent_spell( talent_tree::HERO, "Forewarning" );
  talents.lightsmith.fear_no_evil           = find_talent_spell( talent_tree::HERO, "Fear No Evil" );
  talents.lightsmith.excoriation            = find_talent_spell( talent_tree::HERO, "Excoriation" );

  talents.lightsmith.shared_resolve         = find_talent_spell( talent_tree::HERO, "Shared Resolve" );
  talents.lightsmith.valiance               = find_talent_spell( talent_tree::HERO, "Valiance" );
  talents.lightsmith.hammer_and_anvil       = find_talent_spell( talent_tree::HERO, "Hammer and Anvil" );

  talents.lightsmith.blessing_of_the_forge  = find_talent_spell( talent_tree::HERO, "Blessing of the Forge" );

  talents.templar.lights_guidance         = find_talent_spell( talent_tree::HERO, "Light's Guidance" );

  talents.templar.zealous_vindication     = find_talent_spell( talent_tree::HERO, "Zealous Vindication" );
  talents.templar.for_whom_the_bell_tolls = find_talent_spell( talent_tree::HERO, "For Whom the Bell Tolls" );
  talents.templar.shake_the_heavens       = find_talent_spell( talent_tree::HERO, "Shake the Heavens" );
  talents.templar.wrathful_descent        = find_talent_spell( talent_tree::HERO, "Wrathful Descent" );

  talents.templar.sacrosanct_crusade      = find_talent_spell( talent_tree::HERO, "Sacrosanct Crusade" );
  talents.templar.higher_calling          = find_talent_spell( talent_tree::HERO, "Higher Calling" );
  talents.templar.bonds_of_fellowship     = find_talent_spell( talent_tree::HERO, "Bonds of Fellowship" );
  talents.templar.unrelenting_charger     = find_talent_spell( talent_tree::HERO, "Unrelenting Charger" );

  talents.templar.endless_wrath           = find_talent_spell( talent_tree::HERO, "Endless Wrath" );
  talents.templar.sanctification          = find_talent_spell( talent_tree::HERO, "Sanctification" );
  talents.templar.hammerfall              = find_talent_spell( talent_tree::HERO, "Hammerfall" );
  talents.templar.undisputed_ruling       = find_talent_spell( talent_tree::HERO, "Undisputed Ruling" );

  talents.templar.lights_deliverance      = find_talent_spell( talent_tree::HERO, "Light's Deliverance" );

  talents.herald_of_the_sun.dawnlight     = find_talent_spell( talent_tree::HERO, "Dawnlight" );

  talents.herald_of_the_sun.morning_star       = find_talent_spell( talent_tree::HERO, "Morning Star");
  talents.herald_of_the_sun.gleaming_rays      = find_talent_spell( talent_tree::HERO, "Gleaming Rays" );
  talents.herald_of_the_sun.luminosity         = find_talent_spell( talent_tree::HERO, "Luminosity" );

  talents.herald_of_the_sun.blessing_of_anshe  = find_talent_spell( talent_tree::HERO, "Blessing of An'she" );
  talents.herald_of_the_sun.lingering_radiance = find_talent_spell( talent_tree::HERO, "Lingering Radiance" );
  talents.herald_of_the_sun.sun_sear           = find_talent_spell( talent_tree::HERO, "Sun Sear" );

  talents.herald_of_the_sun.aurora             = find_talent_spell( talent_tree::HERO, "Aurora" );
  talents.herald_of_the_sun.solar_grace        = find_talent_spell( talent_tree::HERO, "Solar Grace" );
  talents.herald_of_the_sun.second_sunrise     = find_talent_spell( talent_tree::HERO, "Second Sunrise" );

  talents.herald_of_the_sun.suns_avatar        = find_talent_spell( talent_tree::HERO, "Sun's Avatar" );

  // Shared Passives and spells
  passives.plate_specialization = find_specialization_spell( "Plate Specialization" );
  passives.paladin              = find_spell( 137026 );
  spells.avenging_wrath         = find_spell( 31884 );
  spells.judgment_2             = find_rank_spell( "Judgment", "Rank 2" );         // 327977
  spells.hammer_of_wrath_2      = find_rank_spell( "Hammer of Wrath", "Rank 2" );  // 326730
  spec.word_of_glory_2          = find_rank_spell( "Word of Glory", "Rank 2" );
  spells.divine_purpose_buff    = find_spell( specialization() == PALADIN_RETRIBUTION ? 408458 : 223819 );

  // Hero Talent Spells
  spells.lightsmith.holy_bulwark        = find_spell( 432496 );
  spells.lightsmith.holy_bulwark_absorb = find_spell( 432607 );
  spells.lightsmith.forges_reckoning    = find_spell( 447258 );  // Child spell of blessing of the forge, triggered by casting shield of the righteous
  spells.lightsmith.sacred_word         = find_spell( 447246 ); // Child spell of blessing of the forge, triggered by casting Word of Glory
  spells.templar.hammer_of_light_driver = find_spell( 427453 );
  spells.templar.hammer_of_light        = find_spell( 429826 );
  spells.templar.empyrean_hammer        = find_spell( 431398 );
  spells.templar.empyrean_hammer_wd     = find_spell( 431625 );

  spells.herald_of_the_sun.gleaming_rays = find_spell( 431481 );
  spells.herald_of_the_sun.dawnlight_aoe_metadata = find_spell( 431581 );
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

  // This also seems likely to be a bug: the spelldata says Fire, but the tooltip says Radiant
  if ( dbc::is_school( school, SCHOOL_FIRE ) )
  {
    if ( talents.burning_crusade->ok() )
    {
      m *= 1.0 + talents.burning_crusade->effectN( 2 ).percent();
    }
  }

  return m;
}

// paladin_t::composite_attribute_multiplier ================================

double paladin_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double m = player_t::composite_attribute_multiplier( attr );

  // Protection gets increased stamina
  if ( attr == ATTR_STAMINA )
  {
    if ( passives.aegis_of_light -> ok() )
      m *= 1.0 + passives.aegis_of_light -> effectN( 1 ).percent();

    // 2023-02-24 Sanctified Plates currently doesn't give the stamina bonus, despite an apparent bugfix (it worked before)
    if ( bugs && talents.sanctified_plates->ok() )
      m *= 1.0 + talents.sanctified_plates->effectN( 1 ).percent();

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
    if ( talents.seal_of_might->ok() )
      m *= 1.0 + talents.seal_of_might->effectN( 1 ).percent();
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

  if ( buffs.ally_of_the_light->up() )
    cdv += buffs.ally_of_the_light->data().effectN( 1 ).percent();

  return cdv;
}

double paladin_t::composite_heal_versatility() const
{
  double chv = player_t::composite_heal_versatility();

  if ( buffs.ally_of_the_light->up() )
    chv += buffs.ally_of_the_light->data().effectN( 1 ).percent();

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

  if ( talents.seal_of_might->ok() )
  {
    m += talents.seal_of_might->effectN( 2 ).base_value();
  }

  return m;
}

double paladin_t::composite_spell_crit_chance() const
{
  double h = player_t::composite_spell_crit_chance();

  if ( talents.holy_aegis->ok() )
    h += talents.holy_aegis->effectN( 1 ).percent();

  if ( buffs.avenging_wrath -> up() )
    h += buffs.avenging_wrath -> get_crit_bonus();

  return h;
}

double paladin_t::composite_melee_crit_chance() const
{
  double h = player_t::composite_melee_crit_chance();

  if ( talents.holy_aegis->ok() )
    h += talents.holy_aegis->effectN( 1 ).percent();

  if ( buffs.avenging_wrath -> up() )
    h += buffs.avenging_wrath -> get_crit_bonus();

  return h;
}

double paladin_t::composite_base_armor_multiplier() const
{
  double a = player_t::composite_base_armor_multiplier();
  if ( specialization() != PALADIN_PROTECTION )
    return a;

  if ( passives.aegis_of_light -> ok() )
    a *= 1.0 + passives.aegis_of_light -> effectN( 2 ).percent();

  if ( talents.holy_aegis -> ok() )
    a *= 1.0 + talents.holy_aegis -> effectN( 1 ).percent();

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
      if ( td->dots.expurgation->is_ticking() )
      {
        cptm *= 1.0 + talents.holy_flames->effectN( 2 ).percent();
      }
    }
  }

  return cptm;
}

// paladin_t::composite_melee_haste =========================================

double paladin_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  if ( buffs.crusade->up() )
    h /= 1.0 + buffs.crusade->get_haste_bonus();

  if ( buffs.herald_of_the_sun.solar_grace->up() )
    h /= 1.0 + buffs.herald_of_the_sun.solar_grace->stack_value();

  if ( buffs.relentless_inquisitor->up() )
    h /= 1.0 + buffs.relentless_inquisitor->stack_value();

  if ( talents.seal_of_alacrity->ok() )
    h /= 1.0 + talents.seal_of_alacrity->effectN( 1 ).percent();

  if ( buffs.rush_of_light->up() )
    h /= 1.0 + talents.rush_of_light->effectN( 1 ).percent();

  if ( buffs.templar.undisputed_ruling->up() )
    h /= 1.0 + buffs.templar.undisputed_ruling->value();

  return h;
}

// paladin_t::composite_melee_auto_attack_speed =============================

double paladin_t::composite_melee_auto_attack_speed() const
{
  double s = player_t::composite_melee_auto_attack_speed();

  if ( talents.zealots_fervor->ok() )
    s /= 1.0 + talents.zealots_fervor->effectN( 1 ).percent();

  return s;
}

// paladin_t::composite_spell_haste ==========================================

double paladin_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( buffs.crusade->up() )
    h /= 1.0 + buffs.crusade->get_haste_bonus();

  if ( buffs.herald_of_the_sun.solar_grace->up() )
    h /= 1.0 + buffs.herald_of_the_sun.solar_grace->stack_value();

  if ( buffs.relentless_inquisitor->up() )
    h /= 1.0 + buffs.relentless_inquisitor->stack_value();

  if ( talents.seal_of_alacrity->ok() )
    h /= 1.0 + talents.seal_of_alacrity->effectN( 1 ).percent();

  if ( buffs.rush_of_light->up() )
    h /= 1.0 + talents.rush_of_light->effectN( 1 ).percent();

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

  b += talents.holy_shield->effectN( 1 ).percent();
  b += buffs.faith_in_the_light->value();
  b += buffs.barricade_of_faith->value();
  b += buffs.inner_light->value();
  return b;
}

// paladin_t::composite_crit_avoidance ========================================

double paladin_t::composite_crit_avoidance() const
{
  double c = player_t::composite_crit_avoidance();
  c += spec.protection_paladin->effectN( 8 ).percent();
  return c;
}

// paladin_t::composite_parry_rating ==========================================

double paladin_t::composite_parry_rating() const
{
  double p = player_t::composite_parry_rating();

  // add Riposte
  if ( passives.riposte->ok() )
    p += composite_melee_crit_rating();

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

  if ( c == CACHE_ATTACK_CRIT_CHANCE && passives.riposte->ok() )
    player_t::invalidate_cache( CACHE_PARRY );

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
  double mult = passives.plate_specialization->effectN( 1 ).percent();

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
    if ( buffs.holy_avenger->up() )
    {
      amount *= 1.0 + buffs.holy_avenger->data().effectN( 1 ).percent();
    }
  }

  double result = player_t::resource_gain( resource_type, amount, source, action );

  if ( resource_type == RESOURCE_HOLY_POWER && amount > 0 && ( talents.of_dusk_and_dawn->ok() ) )
  {
    // There's probably a better way to do this, some spells don't trigger Dawn
    // Also Judgment only gives Dawn when it impacts, but eh...
    if ( !( source->name_str == "arcane_torrent" || source->name_str == "divine_toll" ) )
    {
      holy_power_generators_used++;
      int hpGensNeeded = as<int>( talents.of_dusk_and_dawn->effectN( 1 ).base_value() );
      if ( holy_power_generators_used >= hpGensNeeded )
      {
        holy_power_generators_used -= hpGensNeeded;
        buffs.blessing_of_dawn->trigger();
      }
    }
  }

  if ( resource_type == RESOURCE_HOLY_POWER && !( source->name_str == "arcane_torrent" ) )
  {
    if ( talents.judge_jury_and_executioner->ok() )
    {
      if ( rppm.judge_jury_and_executioner->trigger() )
      {
        buffs.judge_jury_and_executioner->trigger();
      }
    }
  }
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

  // On a block event, trigger Holy Shield
  if ( s->block_result == BLOCK_RESULT_BLOCKED )
  {
    trigger_holy_shield( s );
  }

  // 2022-11-04 Inner Light Talent deals no damage, so reflect that here
  if (!bugs && buffs.inner_light->up() && !s->action->special && cooldowns.inner_light_icd->up() )
  {
    active.inner_light_damage->set_target( s->action->player );
    active.inner_light_damage->schedule_execute();
    cooldowns.inner_light_icd->start();
  }

  // Trigger Grand Crusader on an avoidance event (TODO: test if it triggers on misses)
  if ( s->result == RESULT_DODGE || s->result == RESULT_PARRY || s->result == RESULT_MISS )
  {
    trigger_grand_crusader();
  }

  // Holy Shield's magic block
  // 2022-11-10 Holy Shield can now only block direct magical damage, standing in Consecration can reduce damage over time, but doesn't proc damage
  if ( school != SCHOOL_PHYSICAL && s->action->harmful &&
       ( ( s->result_type == result_amount_type::DMG_DIRECT && talents.holy_shield->ok() ) ||
         ( s->result_type == result_amount_type::DMG_OVER_TIME && standing_in_consecration() ) ) )
  {
    // Block code mimics attack_t::block_chance()
    // cache.block() contains our block chance
    double block = cache.block();
    // Not sure if this talent works for Mastery Block
    if ( talents.improved_holy_shield->ok() && s->result_type != result_amount_type::DMG_OVER_TIME )
      block += talents.improved_holy_shield->effectN( 1 ).percent();
    // add or subtract 1.5% per level difference
    block += ( level() - s->action->player->level() ) * 0.015;

    auto absorbName = s->result_type != result_amount_type::DMG_OVER_TIME ? "Holy Shield" : "Divine Bulwark";

    if ( block > 0 )
    {
      // Roll for "block"
      if ( rng().roll( block ) )
      {
        // Can't find a block method so lets just copy+paste from sc_player.cpp
        double block_value = composite_block_reduction( s );
        double block_amount =
            s->result_amount *
            clamp( block_value / ( block_value + s->action->player->current.armor_coeff ), 0.0, 0.85 );
        sim->print_debug( "{} {} absorbs {}", name(), absorbName, block_amount );

        // update the relevant counters
        iteration_absorb_taken += block_amount;
        s->self_absorb_amount += block_amount;
        s->result_amount -= block_amount;
        s->result_absorbed = s->result_amount;

        // hack to register this on the abilities table
        if ( s->result_type != result_amount_type::DMG_OVER_TIME )
        {
          buffs.holy_shield_absorb->trigger( 1, block_amount );
          buffs.holy_shield_absorb->consume( block_amount );
        }
        else
        {
          buffs.divine_bulwark_absorb->trigger( 1, block_amount );
          buffs.divine_bulwark_absorb->consume( block_amount );
        }

        // Trigger the damage event
        if ( s->result_type != result_amount_type::DMG_OVER_TIME )
          trigger_holy_shield( s );
      }
      else
      {
        sim->print_debug( "{} {} fails to activate", name(), absorbName );
      }
    }

    sim->print_debug( "Damage to {} after {} mitigation is {}", name(), absorbName, s->result_amount );
  }

  player_t::assess_damage( school, dtype, s );
}

// paladin_t::create_options ================================================

void paladin_t::create_options()
{
  // TODO: figure out a better solution for this.
  add_option( opt_bool( "paladin_fake_sov", options.fake_sov ) );
  add_option( opt_float( "proc_chance_ret_aura_sera", options.proc_chance_ret_aura_sera, 0.0, 1.0 ) );
  add_option( opt_int( "min_dg_heal_targets", options.min_dg_heal_targets, 0, 5 ) );
  add_option( opt_int( "max_dg_heal_targets", options.max_dg_heal_targets, 0, 5 ) );
  add_option( opt_bool( "sacred_weapon_prefer_new_targets", options.sacred_weapon_prefer_new_targets ) );
  add_option( opt_bool( "fake_solidarity", options.fake_solidarity ) );

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

  // evidently it resets to summer on combat start
  next_season = SUMMER;
  next_armament = SACRED_WEAPON;

  if ( talents.inquisitors_ire->ok() )
  {
    buffs.inquisitors_ire_driver->trigger();
  }

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

bool paladin_t::get_how_availability( player_t* t ) const
{
  // Regardless what buff is up, both Hammer of Wrath Talent and Avenging Wrath Talent have to be talented for Hammer of Wrath to be usable on the target. (You can talent into Crusade/Sentinel without Avenging Wrath)
  // Maybe ToDo: Do the same for Avenging Wrath: Might
  // Moved Hammer of Wrath Check to return value
  bool buffs_ok = talents.avenging_wrath->ok() && ( buffs.avenging_wrath->up() || buffs.crusade->up() || buffs.sentinel->up() );
  buffs_ok = buffs_ok || buffs.final_verdict->up() || buffs.templar.endless_wrath->up() || buffs.herald_of_the_sun.blessing_of_anshe->up();
  // Health threshold has to be hardcoded :peepocri:
  return ( buffs_ok || t->health_percentage() <= 20 ) && talents.hammer_of_wrath->ok();
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
    return make_fn_expr( "consecration_ticking", [ this ]() { return all_active_consecrations.empty() ? 0 : 1; } );
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

    time_to_hpg_expr_t( util::string_view n, paladin_t& p )
      : paladin_expr_t( n, p ),
        cs_cd( p.get_cooldown( "crusader_strike" ) ),
        boj_cd( p.get_cooldown( "blade_of_justice" ) ),
        j_cd( p.get_cooldown( "judgment" ) ),
        how_cd( p.get_cooldown( "hammer_of_wrath" ) ),
        wake_cd( p.get_cooldown( "wake_of_ashes" ) ),
        hs_cd( p.get_cooldown( "holy_shock" ) ),
        at_cd( p.get_cooldown( "arcane_torrent" ) )
    {
    }

    // todo: account for divine resonance, crusading strikes, divine auxiliary
    double evaluate() override
    {
      if ( paladin.specialization() == PALADIN_PROTECTION )
      {
        paladin.sim->errorf( "\"time_to_hpg\" not supported for Protection" );
        return 0;
      }
      timespan_t gcd_ready = paladin.gcd_ready - paladin.sim->current_time();
      gcd_ready            = std::max( gcd_ready, 0_ms );

      timespan_t shortest_hpg_time = cs_cd->remains();

      // Blood Elf
      if ( paladin.race == RACE_BLOOD_ELF )
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

        if ( j_cd->remains() < shortest_hpg_time )
          shortest_hpg_time = j_cd->remains();
      }

      // Holy
      if ( paladin.specialization() == PALADIN_HOLY )
      {
        if ( hs_cd->remains() < shortest_hpg_time )
          shortest_hpg_time = hs_cd->remains();
      }

      // TODO: Protection

      // Shared
      // TODO: check every target rather than just the paladin's main target
      if ( paladin.get_how_availability( paladin.target ) && how_cd->remains() < shortest_hpg_time )
        shortest_hpg_time = how_cd->remains();

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

  struct next_season_expr_t : public paladin_expr_t
  {
    next_season_expr_t( util::string_view n, paladin_t& p ) : paladin_expr_t( n, p )
    {
    }

    double evaluate() override
    {
      return paladin.next_season;
    }
  };

  struct next_armament_expr_t : public paladin_expr_t
  {
    next_armament_expr_t(util::string_view n, paladin_t& p) : paladin_expr_t(n, p)
    {

    }
    double evaluate() override
    {
      if ( paladin.talents.lightsmith.holy_armaments->ok() )
        return paladin.next_armament;
      else
        return -1.0;
    }
  };

  if ( splits[ 0 ] == "next_season" )
  {
    return std::make_unique<next_season_expr_t>( name_str, *this );
  }

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
      if (paladin.buffs.bastion_of_light->up())
      {
        gain += paladin.talents.bastion_of_light->effectN( 1 ).base_value();
      }
      return gain;
    }
  };

  if (splits[0] == "judgment_holy_power")
  {
    return std::make_unique<judgment_holy_power_expr_t>( name_str, *this );
  }

  struct hpg_to_2dawn_expr_t : public paladin_expr_t
  {
    hpg_to_2dawn_expr_t( util::string_view n, paladin_t& p ) : paladin_expr_t( n, p )
    {
    }
    double evaluate() override
    {
      if ( paladin.talents.of_dusk_and_dawn->ok() )
      {
        return 6.0 - paladin.holy_power_generators_used - ( paladin.buffs.blessing_of_dawn->stack() * 3 );
      }
      else
        return -1.0;
    }
  };

  if (splits[0] == "hpg_to_2dawn")
  {
    return std::make_unique<hpg_to_2dawn_expr_t>( name_str, *this );
  }

  auto cons_expr = create_consecration_expression( name_str );
  auto aw_expr   = create_aw_expression( name_str );
  if ( cons_expr )
  {
    return cons_expr;
  }
  if ( aw_expr )
  {
    return aw_expr;
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

      if ( paladin.melee_swing_count % 2 == 0 )
      {
        if ( paladin.main_hand_attack && paladin.main_hand_attack->execute_event )
        {
          return paladin.main_hand_attack->execute_event->remains().total_seconds() + paladin.main_hand_attack->execute_time().total_seconds();
        }

        return std::numeric_limits<double>::infinity();
      }
      else
      {
        if ( paladin.main_hand_attack && paladin.main_hand_attack->execute_event )
          return paladin.main_hand_attack->execute_event->remains().total_seconds();
        return std::numeric_limits<double>::infinity();
      }
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

void paladin_t::apply_affecting_auras( action_t& action )
{
  player_t::apply_affecting_auras( action );

  action.apply_affecting_aura( spec.retribution_paladin );
  if ( specialization() == PALADIN_RETRIBUTION )
    action.apply_affecting_aura( spec.retribution_paladin_2 );
  action.apply_affecting_aura( spec.holy_paladin );
  action.apply_affecting_aura( spec.protection_paladin );
  action.apply_affecting_aura( passives.paladin );

  if ( talents.herald_of_the_sun.luminosity )
    action.apply_affecting_aura( talents.herald_of_the_sun.luminosity );

  if ( sets->has_set_bonus( PALADIN_RETRIBUTION, TWW1, B2 ) )
    action.apply_affecting_aura( sets->set( PALADIN_RETRIBUTION, TWW1, B2 ) );
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

  void static_init() const override
  {
  }

  void init( player_t* p ) const override
  {
    p->buffs.beacon_of_light       = make_buff( p, "beacon_of_light", p->find_spell( 53563 ) );
    p->buffs.blessing_of_sacrifice = new buffs::blessing_of_sacrifice_t( p );
    p->debuffs.forbearance         = new buffs::forbearance_t( p, "forbearance" );

    p->buffs.blessing_of_summer =
        make_buff( p, "blessing_of_summer", p->find_spell( 328620 ) )->set_chance( 1.0 )->set_cooldown( 0_ms );
    p->buffs.blessing_of_autumn = make_buff<buffs::blessing_of_autumn_t>( p );
    p->buffs.blessing_of_winter = make_buff( p, "blessing_of_winter", p->find_spell( 328281 ) )->set_cooldown( 0_ms );
    p->buffs.blessing_of_spring = make_buff( p, "blessing_of_spring", p->find_spell( 328282 ) )
                                      ->set_cooldown( 0_ms )
                                      ->add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );
  }

  void create_actions(player_t* p) const override
  {
    if (p->is_enemy() || p->type == HEALING_ENEMY || p->is_pet())
      return;

    // 9.0 Paladin Night Fae

    // Only create these if the player sets the option to get the buff.
    if ( !p->external_buffs.blessing_of_summer.empty() )
    {
      action_t* summer_proc = new blessing_of_summer_proc_t(p);
      const spell_data_t* summer_data = p->find_spell(328620);

      // This effect can proc on almost any damage, including many actions in simc that have callbacks = false.
      // Using an assessor here will cause this to have the chance to proc on damage from any action.
      p->assessor_out_damage.add(
        assessor::CALLBACKS, [p, summer_proc, summer_data](result_amount_type, action_state_t* s) {
          if (!(p->buffs.blessing_of_summer->up()))
            return assessor::CONTINUE;

          double proc_chance = summer_data->proc_chance();

          if (s->action != summer_proc && s->result_total > 0.0 && p->buffs.blessing_of_summer->up() &&
            summer_data->proc_flags() & (UINT64_C(1) << s->proc_type()) && p->rng().roll(proc_chance))
          {
            double da = s->result_amount * summer_data->effectN(1).percent();
            make_event(p->sim, [t = s->target, summer_proc, da] {
              summer_proc->set_target(t);
              summer_proc->base_dd_min = summer_proc->base_dd_max = da;
              summer_proc->execute();
              });
          }

          return assessor::CONTINUE;
        });
    }

    if (!p->external_buffs.blessing_of_winter.empty())
    {
      action_t* winter_proc;
      if (p->type == PALADIN)
        // Some Paladin auras affect this spell and are handled in paladin_spell_t.
        winter_proc = new blessing_of_winter_proc_t<paladin_spell_t, paladin_t>(debug_cast<paladin_t*>(p));
      else
        winter_proc = new blessing_of_winter_proc_t<spell_t, player_t>(p);

      auto winter_effect = new special_effect_t(p);
      winter_effect->name_str = "blessing_of_winter_cb";
      winter_effect->type = SPECIAL_EFFECT_EQUIP;
      winter_effect->spell_id = 328281;
      winter_effect->cooldown_ = p->find_spell(328281)->internal_cooldown();
      winter_effect->execute_action = winter_proc;
      p->special_effects.push_back(winter_effect);

      auto winter_cb = new dbc_proc_callback_t(p, *winter_effect);
      winter_cb->deactivate();
      winter_cb->initialize();

      p->buffs.blessing_of_winter->set_stack_change_callback([winter_cb](buff_t*, int, int new_) {
        if (new_)
          winter_cb->activate();
        else
          winter_cb->deactivate();
        });
    }
  }

  void register_hotfixes() const override
  {
  }

  void combat_begin( sim_t* ) const override
  {
  }

  void combat_end( sim_t* ) const override
  {
  }
};

}  // end namespace paladin

const module_t* module_t::paladin()
{
  static paladin::paladin_module_t m;
  return &m;
}
