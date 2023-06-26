// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
/*
  TODO: Update Holy for BfA
*/
#include "sc_paladin.hpp"

#include "simulationcraft.hpp"

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
    holy_power_generators_used( 0 ),
    melee_swing_count( 0 )
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

  cooldowns.avengers_shield           = get_cooldown( "avengers_shield" );
  cooldowns.consecration              = get_cooldown( "consecration" );
  cooldowns.inner_light_icd           = get_cooldown( "inner_light_icd" );
  cooldowns.inner_light_icd->duration = find_spell( 386556 )->internal_cooldown();
  cooldowns.righteous_protector_icd   = get_cooldown( "righteous_protector_icd" );
  cooldowns.righteous_protector_icd->duration = find_spell( 204074 )->internal_cooldown();
  cooldowns.judgment                  = get_cooldown( "judgment" );
  cooldowns.shield_of_the_righteous   = get_cooldown( "shield_of_the_righteous" );
  cooldowns.guardian_of_ancient_kings = get_cooldown( "guardian_of_ancient_kings" );
  cooldowns.ardent_defender           = get_cooldown( "ardent_defender" );

  cooldowns.blade_of_justice = get_cooldown( "blade_of_justice" );
  cooldowns.final_reckoning  = get_cooldown( "final_reckoning" );
  cooldowns.hammer_of_wrath  = get_cooldown( "hammer_of_wrath" );
  cooldowns.wake_of_ashes    = get_cooldown( "wake_of_ashes" );

  cooldowns.blessing_of_the_seasons = get_cooldown( "blessing_of_the_seasons" );

  cooldowns.ret_aura_icd = get_cooldown( "ret_aura_icd" );
  cooldowns.ret_aura_icd->duration = timespan_t::from_seconds( 30 );

  cooldowns.consecrated_blade_icd = get_cooldown( "consecrated_blade_icd" );
  cooldowns.consecrated_blade_icd->duration = timespan_t::from_seconds( 10 );

  cooldowns.searing_light_icd = get_cooldown( "searing_light_icd" );
  cooldowns.searing_light_icd->duration = timespan_t::from_seconds( 15 );

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
  harmful = false;

  // link needed for Righteous Protector / SotR cooldown reduction
  cooldown = p->cooldowns.avenging_wrath;
}

void avenging_wrath_t::execute()
{
  paladin_spell_t::execute();

  p()->buffs.avenging_wrath->trigger();
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
    : paladin_spell_t( name, p, p->find_spell( 81297 ) ), heal_tick( new golden_path_t( p ) )
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

struct consecration_t : public paladin_spell_t
{
  consecration_tick_t* damage_tick;
  ground_aoe_params_t cons_params;
  consecration_source source_type;

  double precombat_time;

  consecration_t( paladin_t* p, util::string_view options_str )
    : paladin_spell_t( "consecration", p, p->find_spell( 26573 ) ),
      damage_tick( new consecration_tick_t( "consecration_tick", p ) ),
      source_type( HARDCAST ),
      precombat_time( 2.0 )
  {
    add_option( opt_float( "precombat_time", precombat_time ) );
    parse_options( options_str );

    dot_duration = 0_ms;  // the periodic event is handled by ground_aoe_event_t
    may_miss = harmful = false;
    if ( p->specialization() == PALADIN_PROTECTION && p->spec.consecration_3->ok() )
      cooldown->duration *= 1.0 + p->spec.consecration_3->effectN( 1 ).percent();

    if ( p->talents.divine_hammer->ok() || p->talents.consecrated_blade->ok() )
      background = true;

    add_child( damage_tick );
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
                          case ground_aoe_params_t::EVENT_CREATED:
                            if ( source_type == HARDCAST ) {
                              p()->active_consecration = event;
                            } else if ( source_type == BLADE_OF_JUSTICE ) {
                              p()->active_boj_cons = event;
                            } else if ( source_type == SEARING_LIGHT ) {
                              p()->active_searing_light_cons = event;
                            }
                            p()->all_active_consecrations.insert(event);
                            break;
                          case ground_aoe_params_t::EVENT_DESTRUCTED:
                            if ( source_type == HARDCAST ) {
                              p()->active_consecration = nullptr;
                            } else if ( source_type == BLADE_OF_JUSTICE ) {
                              p()->active_boj_cons = nullptr;
                            } else if ( source_type == SEARING_LIGHT ) {
                              p()->active_searing_light_cons = nullptr;
                            }
                            p()->all_active_consecrations.erase(event);
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
      event_t::cancel( p()->active_consecration );
    // or if it's a boj-triggered Cons, cancel the previous BoJ-triggered cons
    else if ( source_type == BLADE_OF_JUSTICE && p()->active_boj_cons != nullptr )
      event_t::cancel( p()->active_boj_cons );
    // or if it's a searing light-triggered Cons, cancel the previous searing light-triggered cons
    else if ( source_type == SEARING_LIGHT && p()->active_searing_light_cons != nullptr )
      event_t::cancel( p()->active_searing_light_cons );

    paladin_spell_t::execute();

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
    : paladin_spell_t( "blinding_light", p, p->find_talent_spell( "Blinding Light" ) )
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

struct touch_of_light_dmg_t : public paladin_spell_t
{
  touch_of_light_dmg_t( paladin_t* p ) : paladin_spell_t( "touch_of_light_dmg", p, p -> find_spell( 385354 ) )
  {
    background = true;
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
      affected_by.hand_of_light = true;
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
  }

  void execute() override
  {
    paladin_melee_attack_t::execute();
    p()->melee_swing_count++;
    if ( p()->melee_swing_count % as<int>( p()->talents.crusading_strikes->effectN( 3 ).base_value() ) == 0 )
    {
      p()->resource_gain(
        RESOURCE_HOLY_POWER,
        as<int>( p()->talents.crusading_strikes->effectN( 4 ).base_value() ),
        p()->gains.hp_crusading_strikes
      );
    }

    if ( p()->talents.empyrean_power->ok() )
    {
      if ( rng().roll( p()->talents.empyrean_power->effectN( 2 ).percent() ) )
      {
        p()->procs.empyrean_power->occur();
        p()->buffs.empyrean_power->trigger();
      }
    }
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
      impact_action = crusading_strike;
      weapon_multiplier = 0.0;
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

  void execute() override
  {
    if ( first )
      first = false;

    paladin_melee_attack_t::execute();

    if ( result_is_hit( execute_state->result ) )
    {
      if ( p()->specialization() == PALADIN_RETRIBUTION )
      {
        if ( p()->talents.art_of_war->ok() )
        {
          // Check for BoW procs
          double aow_proc_chance = p()->talents.art_of_war->effectN( 1 ).percent();
          if ( rng().roll( aow_proc_chance ) )
          {
            p()->procs.art_of_war->occur();
            p()->cooldowns.blade_of_justice->reset( true );
          }
        }
      }

      if ( p()->talents.seal_of_the_crusader->ok() )
      {
        seal_of_the_crusader->target = execute_state->target;
        seal_of_the_crusader->schedule_execute();
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
      affected_by.hand_of_light = true;
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

    p()->trigger_grand_crusader();

    if ( p()->sets->has_set_bonus( PALADIN_PROTECTION, T29, B4 ) )
    {
      p()->t29_4p_prot();
    }

    if ( p()->talents.empyrean_power->ok() )
    {
      if ( rng().roll( p()->talents.empyrean_power->effectN( 1 ).percent() ) )
      {
        p()->procs.empyrean_power->occur();
        p()->buffs.empyrean_power->trigger();
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

// Word of Glory ===================================================

struct word_of_glory_t : public holy_power_consumer_t<paladin_heal_t>
{
  light_of_the_titans_t* light_of_the_titans;
  word_of_glory_t( paladin_t* p, util::string_view options_str )
    : holy_power_consumer_t( "word_of_glory", p, p->find_class_spell( "Word of Glory" ) ),
      light_of_the_titans( new light_of_the_titans_t( p, "" ) )
  {
    parse_options( options_str );
    target = p;
    is_wog = true;
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

  double cost() const override
  {
    double c = holy_power_consumer_t::cost();

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
        if ( p()->talents.highlords_judgment->ok() )
        {
        num_stacks += as<int>( p()->talents.highlords_judgment->effectN( 1 ).base_value() );
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
}

double judgment_t::action_multiplier() const
{
  double am = paladin_melee_attack_t::action_multiplier();

  if ( p()->talents.justification->ok() )
    am *= 1.0 + p()->talents.justification->effectN( 1 ).percent();

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
    paladin_spell_t::execute();

    if ( p()->talents.divine_resonance->ok() )
    {
      p()->buffs.divine_resonance->trigger();
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

// Hammer of Wrath

struct hammer_of_wrath_t : public paladin_melee_attack_t
{
  hammer_of_wrath_t( paladin_t* p, util::string_view options_str )
    : paladin_melee_attack_t( "hammer_of_wrath", p, p->find_talent_spell( talent_tree::CLASS, "Hammer of Wrath" ) )
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

    if ( p->talents.vengeful_wrath->ok() )
    {
      base_crit = p->talents.vengeful_wrath->effectN( 1 ).percent();
    }

    if ( p->talents.adjudication->ok() )
    {
      add_child( p->active.background_blessed_hammer );
    }

    if ( p->sets->has_set_bonus( PALADIN_RETRIBUTION, T30, B2 ) )
    {
      crit_bonus_multiplier *= 1.0 + p->sets->set( PALADIN_RETRIBUTION, T30, B2 )->effectN( 2 ).percent();
      base_multiplier *= 1.0 + p->sets->set( PALADIN_RETRIBUTION, T30, B2 )->effectN( 1 ).percent();
    }

    if ( p->sets->has_set_bonus( PALADIN_RETRIBUTION, T30, B4 ) )
    {
      aoe = as<int>( p->sets->set( PALADIN_RETRIBUTION, T30, B4 )->effectN( 2 ).base_value() );
      base_aoe_multiplier *= p->sets->set( PALADIN_RETRIBUTION, T30, B4 )->effectN( 4 ).percent();
    }
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( !p()->get_how_availability( candidate_target ) )
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
  }

  void impact( action_state_t* s ) override
  {
    paladin_melee_attack_t::impact( s );

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
      if ( s->target->health_percentage() <= p()->talents.vanguards_momentum->effectN( 2 ).base_value() )
      {
        // technically this is in spell 403081 for some reason
        p()->resource_gain( RESOURCE_HOLY_POWER, 1, p()->gains.hp_vm );
      }
    }

    if ( p()->talents.adjudication->ok() )
    {
      if ( s->result == RESULT_CRIT && s->chain_target == 0 )
      {
        p()->active.background_blessed_hammer->schedule_execute();
      }
    }

    if ( p()->sets->has_set_bonus( PALADIN_RETRIBUTION, T30, B2 ) )
    {
      td( s->target )->debuff.judgment->trigger();
    }
  }

  double action_multiplier() const override
  {
    double am = paladin_melee_attack_t::action_multiplier();

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

  debuff.judgment              = make_buff( *this, "judgment", paladin->spells.judgment_debuff );
  if ( paladin->talents.highlords_judgment->ok() )
  {
    debuff.judgment =
        debuff.judgment->set_max_stack( as<int>( 1 + paladin->talents.highlords_judgment->effectN( 1 ).base_value() ) )
            ->modify_duration(
                timespan_t::from_millis( paladin->talents.highlords_judgment->effectN( 3 ).base_value() ) );
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
  debuff.crusaders_resolve     = make_buff( *this, "crusaders_resolve", paladin->find_spell( 383843 ) )
                                 ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
                                 ->set_max_stack( 3 );
  debuff.heartfire = make_buff( *this, "heartfire", paladin-> find_spell( 408461 ) );
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
  if ( name == "word_of_glory" )
    return new word_of_glory_t( this, options_str );

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
  holy_power_generators_used = 0;
  melee_swing_count = 0;
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
  gains.moment_of_glory  = get_gain( "moment_of_glory_absorb" );


  // Holy Power
  gains.hp_templars_verdict_refund = get_gain( "templars_verdict_refund" );
  gains.judgment                   = get_gain( "judgment" );
  gains.hp_cs                      = get_gain( "crusader_strike" );
  gains.hp_divine_toll             = get_gain( "divine_toll" );
  gains.hp_vm                      = get_gain( "vanguards_momentum" );
  gains.hp_crusading_strikes       = get_gain( "crusading_strikes" );
  gains.hp_divine_auxiliary        = get_gain( "divine_auxiliary" );
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
  //.avenging_wrath_might = new buffs::avenging_wrath_buff_t( this );
  buffs.divine_purpose = make_buff( this, "divine_purpose", spells.divine_purpose_buff );
  buffs.divine_shield  = make_buff( this, "divine_shield", find_class_spell( "Divine Shield" ) )
                            ->set_cooldown( 0_ms );  // Let the ability handle the CD
  buffs.blessing_of_protection = make_buff( this, "blessing_of_protection", find_spell( 1022 ) );
  buffs.blessing_of_spellwarding = make_buff( this, "blessing_of_spellwarding", find_spell( 204018 ) );
  buffs.strength_in_adversity    = make_buff( this, "strength_in_adversity", find_spell( 393071 ) )
                                    ->add_invalidate( CACHE_PARRY )
                                    ->set_default_value_from_effect(1)
                                    ->set_max_stack(5); // Buff has no stacks, but can have up to 5 different values.

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
  buffs.faiths_armor = make_buff( this, "faiths_armor", find_spell( 379017 ) )
                           ->set_default_value_from_effect(1)
                           ->add_invalidate( CACHE_BONUS_ARMOR );

  if ( talents.seal_of_order->ok() )
  {
    buffs.blessing_of_dusk->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
        double recharge_mult = 1.0 / ( 1.0 + talents.seal_of_order->effectN( 1 ).percent() );
      for ( size_t i = 3; i < 9; i++ )
      {
        // Effects 6 (Blessed Hammer) and 7 (Crusader Strike) are already in Effect 3
        // Effect 4: Hammer of the Righteous, Effect 5: Judgment, Effect 8: Hammer of Wrath
        if ( i == 6 )
          i = 8;
        spelleffect_data_t label = find_spell( 385126 )->effectN( i );
        for ( auto a : action_list )
        {
          if ( a->cooldown->duration != 0_ms && (a->data().affected_by( label ) || a->data().affected_by_category(label) ))
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
  buffs.divine_resonance = make_buff( this, "divine_resonance", find_spell( 355455 ) )
          ->set_tick_callback( [ this ]( buff_t* /* b */, int /* stacks */, timespan_t /* tick_time */ ) {
              this->active.divine_resonance->set_target( this->target );
              this->active.divine_resonance->schedule_execute();
          } );
}

// paladin_t::default_potion ================================================

std::string paladin_t::default_potion() const
{
  std::string retribution_pot = ( true_level > 60 ) ? "elemental_potion_of_ultimate_power_3" : "disabled";

  std::string protection_pot = ( true_level > 60 ) ? "elemental_potion_of_ultimate_power_3" : "disabled";

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
  std::string retribution_food = ( true_level > 50 ) ? "fated_fortune_cookie" : "disabled";

  std::string protection_food = ( true_level > 50 ) ? "fated_fortune_cookie" : "disabled";

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
  std::string retribution_flask = ( true_level > 60 ) ? "phial_of_tepid_versatility_3" : "disabled";

  std::string protection_flask = ( true_level > 60 ) ? "phial_of_tepid_versatility_3" : "disabled";

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
  return ( true_level > 50 ) ? "draconic_augment_rune" : "disabled";
}

// paladin_t::default_temporary_enchant ================================

std::string paladin_t::default_temporary_enchant() const
{
  switch ( specialization() )
  {
    case PALADIN_PROTECTION:
      return "main_hand:howling_rune_3";
    case PALADIN_RETRIBUTION:
      return "main_hand:howling_rune_3";

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
  talents.avenging_wrath_might           = find_talent_spell( talent_tree::SPECIALIZATION, "Avenging Wrath: Might" );
  talents.relentless_inquisitor          = find_talent_spell( talent_tree::SPECIALIZATION, "Relentless Inquisitor" );

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

  // Shared Passives and spells
  passives.plate_specialization = find_specialization_spell( "Plate Specialization" );
  passives.paladin              = find_spell( 137026 );
  spells.avenging_wrath         = find_spell( 31884 );
  spells.judgment_2             = find_rank_spell( "Judgment", "Rank 2" );         // 327977
  spells.hammer_of_wrath_2      = find_rank_spell( "Hammer of Wrath", "Rank 2" );  // 326730
  spec.word_of_glory_2          = find_rank_spell( "Word of Glory", "Rank 2" );
  spells.divine_purpose_buff    = find_spell( specialization() == PALADIN_RETRIBUTION ? 408458 : 223819 );

  // Dragonflight Tier Sets
  tier_sets.ally_of_the_light_2pc = sets->set( PALADIN_PROTECTION, T29, B2 );
  tier_sets.ally_of_the_light_4pc = sets->set( PALADIN_PROTECTION, T29, B4 );
  tier_sets.heartfire_sentinels_authority_2pc = sets->set( PALADIN_PROTECTION, T30, B2 );
  tier_sets.heartfire_sentinels_authority_4pc = sets->set( PALADIN_PROTECTION, T30, B4 );
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
  }

  if ( attr == ATTR_STRENGTH )
  {
    if ( talents.seal_of_might->ok() )
      m *= 1.0 + talents.seal_of_might->effectN( 1 ).percent();
    if ( buffs.redoubt->up() )
      // Applies to base str, gear str and buffs. So everything basically.
      m *= 1.0 + buffs.redoubt->stack_value();
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

  return m;
}

double paladin_t::composite_mastery_value() const
{
  double m = player_t::composite_mastery_value();

  if ( talents.seal_of_might->ok() )
  {
    m += talents.seal_of_might->effectN( 2 ).percent();
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

  return a;
}

double paladin_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double cptm      = player_t::composite_player_target_multiplier( target, school );
  return cptm;
}

// paladin_t::composite_melee_haste =========================================

double paladin_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  if ( buffs.crusade->up() )
    h /= 1.0 + buffs.crusade->get_haste_bonus();

  if ( buffs.relentless_inquisitor->up() )
    h /= 1.0 + buffs.relentless_inquisitor->stack_value();

  if ( talents.seal_of_alacrity->ok() )
    h /= 1.0 + talents.seal_of_alacrity->effectN( 1 ).percent();

  if ( buffs.rush_of_light->up() )
    h /= 1.0 + talents.rush_of_light->effectN( 1 ).percent();

  return h;
}

// paladin_t::composite_melee_speed =========================================

double paladin_t::composite_melee_speed() const
{
  double s = player_t::composite_melee_speed();

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

  if ( buffs.relentless_inquisitor->up() )
    h /= 1.0 + buffs.relentless_inquisitor->stack_value();

  if ( talents.seal_of_alacrity->ok() )
    h /= 1.0 + talents.seal_of_alacrity->effectN( 1 ).percent();

  if ( buffs.rush_of_light->up() )
    h /= 1.0 + talents.rush_of_light->effectN( 1 ).percent();

  return h;
}

// paladin_t::composite_bonus_armor =========================================

double paladin_t::composite_bonus_armor() const
{
  double ba = player_t::composite_bonus_armor();

  if ( buffs.shield_of_the_righteous->check() )
  {
    double bonus = buffs.shield_of_the_righteous->value() * cache.strength();
    ba += bonus;
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
      sp = spec.protection_paladin->effectN( 7 ).percent();
      sp *= composite_melee_attack_power_by_type( attack_power_type::WEAPON_MAINHAND ) *
           composite_attack_power_multiplier();
      break;
    case PALADIN_RETRIBUTION:
      sp = spec.retribution_paladin->effectN( 10 ).percent() *
           composite_melee_attack_power_by_type( attack_power_type::WEAPON_MAINHAND ) *
           composite_attack_power_multiplier();
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
    return composite_spell_power( SCHOOL_MAX ) * spec.holy_paladin->effectN( 9 ).percent();
  }

  return player_t::composite_melee_attack_power();
}

double paladin_t::composite_melee_attack_power_by_type( attack_power_type ap_type ) const
{
  // Holy paladin AP scales purely off of Spell power and nothing else not even Weapon
  if ( specialization() == PALADIN_HOLY )
  {
    return player_t::composite_melee_attack_power_by_type( attack_power_type::NO_WEAPON );
  }

  return player_t::composite_melee_attack_power_by_type( ap_type );
}

// paladin_t::composite_attack_power_multiplier =============================

double paladin_t::composite_attack_power_multiplier() const
{
  double ap = player_t::composite_attack_power_multiplier();

  // Mastery bonus is multiplicative with other effects
  if ( specialization() == PALADIN_PROTECTION )
  {
      //Note for future; If something changes with mastery, make sure you verify this to still be accurate
    auto m = cache.mastery();

    if ( talents.seal_of_might->ok() && ( buffs.avenging_wrath->up() || buffs.crusade->up() || buffs.sentinel->up() ) )
      m += talents.seal_of_might->effectN( 2 ).base_value();

    ap *= 1.0 + m * mastery.divine_bulwark->effectN( 2 ).mastery_value();
  }

  // Holy paladin AP scales purely off of Spell power and nothing else not even Weapon
  // This means literally nothing, not sharpning/weight stones, battle shout, weapon, etc etc
  // It is purely off SP and nothing effects it.
  if ( specialization() == PALADIN_HOLY )
  {
    return 1.0;
  }

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

// paladin_t::temporary_movement_modifier =====================================

double paladin_t::temporary_movement_modifier() const
{
  double temporary = player_t::temporary_movement_modifier();

  // shamelessly stolen from warrior_t - see that module for how to add more buffs

  // These are ordered in the highest speed movement increase to the lowest, there's no reason to check the rest as they
  // will just be overridden. Also gives correct benefit numbers.
  if ( buffs.divine_steed->up() )
  {
    // TODO: replace with commented version once we have spell data
    temporary = std::max( buffs.divine_steed->value(), temporary );
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

  if ( talents.inquisitors_ire->ok() )
  {
    buffs.inquisitors_ire_driver->trigger();
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
  bool buffs_ok = talents.avenging_wrath->ok() && (buffs.avenging_wrath->up() || buffs.crusade->up() || buffs.sentinel->up() );
  buffs_ok      = buffs_ok || buffs.final_verdict->up();
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

  if ( splits[ 0 ] == "next_season" )
  {
    return std::make_unique<next_season_expr_t>( name_str, *this );
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
  action.apply_affecting_aura( spec.holy_paladin );
  action.apply_affecting_aura( spec.protection_paladin );
  action.apply_affecting_aura( passives.paladin );
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
  void html_customsection( report::sc_html_stream& os ) override
  {
  }

private:
  paladin_t& p;
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

  void create_actions( player_t* p ) const override
  {
    if ( p->is_enemy() || p->type == HEALING_ENEMY || p->is_pet() )
      return;

    // 9.0 Paladin Night Fae

    // Only create these if the player sets the option to get the buff.
    if ( !p->external_buffs.blessing_of_summer.empty() )
    {
      action_t* summer_proc           = new blessing_of_summer_proc_t( p );
      const spell_data_t* summer_data = p->find_spell( 328620 );

      // This effect can proc on almost any damage, including many actions in simc that have callbacks = false.
      // Using an assessor here will cause this to have the chance to proc on damage from any action.
      // TODO: Ensure there is no incorrect looping that can happen with other similar effects.
      p->assessor_out_damage.add(
          assessor::CALLBACKS, [ p, summer_proc, summer_data ]( result_amount_type, action_state_t* s ) {
            if ( !(p->buffs.blessing_of_summer->up()) )
              return assessor::CONTINUE;

            double proc_chance = summer_data->proc_chance();

            if ( s->action != summer_proc && s->result_total > 0.0 && p->buffs.blessing_of_summer->up() &&
                 summer_data->proc_flags() & ( UINT64_C( 1 ) << s->proc_type() ) && p->rng().roll( proc_chance ) )
            {
              double da = s->result_amount * summer_data->effectN( 1 ).percent();
              make_event( p->sim, [ t = s->target, summer_proc, da ] {
                summer_proc->set_target( t );
                summer_proc->base_dd_min = summer_proc->base_dd_max = da;
                summer_proc->execute();
              } );
            }

            return assessor::CONTINUE;
          } );
    }

    if ( !p->external_buffs.blessing_of_winter.empty() )
    {
      action_t* winter_proc;
      if ( p->type == PALADIN )
        // Some Paladin auras affect this spell and are handled in paladin_spell_t.
        winter_proc = new blessing_of_winter_proc_t<paladin_spell_t, paladin_t>( debug_cast<paladin_t*>( p ) );
      else
        winter_proc = new blessing_of_winter_proc_t<spell_t, player_t>( p );

      auto winter_effect            = new special_effect_t( p );
      winter_effect->name_str       = "blessing_of_winter_cb";
      winter_effect->type           = SPECIAL_EFFECT_EQUIP;
      winter_effect->spell_id       = 328281;
      winter_effect->cooldown_      = p->find_spell( 328281 )->internal_cooldown();
      winter_effect->execute_action = winter_proc;
      p->special_effects.push_back( winter_effect );

      auto winter_cb = new dbc_proc_callback_t( p, *winter_effect );
      winter_cb->deactivate();
      winter_cb->initialize();

      p->buffs.blessing_of_winter->set_stack_change_callback( [ winter_cb ]( buff_t*, int, int new_ ) {
        if ( new_ )
          winter_cb->activate();
        else
          winter_cb->deactivate();
      } );
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
