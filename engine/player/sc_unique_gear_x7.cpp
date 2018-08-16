// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

#include "unique_gear.hpp"

using namespace unique_gear;

namespace {
namespace bfa { // YaN - Yet another namespace - to resolve conflicts with global namespaces.

template <typename BASE = proc_spell_t>
struct base_bfa_proc_t : public BASE
{
  base_bfa_proc_t( const special_effect_t& effect, const std::string& name, unsigned spell_id ) :
    BASE( name, effect.player, effect.player->find_spell( spell_id ),
        effect.item )
  { }

  base_bfa_proc_t( const special_effect_t& effect, const std::string& name, const spell_data_t* s ) :
    BASE( name, effect.player, s, effect.item )
  { }
};

template <typename BASE = proc_spell_t>
struct base_bfa_aoe_proc_t : public base_bfa_proc_t<BASE>
{
  base_bfa_aoe_proc_t( const special_effect_t& effect, const std::string& name, unsigned spell_id ) :
    base_bfa_proc_t<BASE>( effect, name, spell_id )
  {
    this->aoe = -1;
    this->split_aoe_damage = true;
  }

  base_bfa_aoe_proc_t( const special_effect_t& effect, const std::string& name, const spell_data_t* s ) :
    base_bfa_proc_t<BASE>( effect, name, s )
  {
    this->aoe = -1;
    this->split_aoe_damage = true;
  }
};

using aoe_proc_t = base_bfa_aoe_proc_t<proc_spell_t>;
using proc_t = base_bfa_proc_t<proc_spell_t>;

/**
 * Forward declarations so we can reorganize the file a bit more sanely.
 */

namespace consumables
{
  void galley_banquet( special_effect_t& );
  void bountiful_captains_feast( special_effect_t& );
  void potion_of_rising_death( special_effect_t& );
  void potion_of_bursting_blood( special_effect_t& );
}

namespace enchants
{
  void galeforce_striking( special_effect_t& );
  void torrent_of_elements( special_effect_t& );
  custom_cb_t weapon_navigation( unsigned );
}

namespace items
{
  // 8.0 misc
  void darkmoon_deck_squalls( special_effect_t& );
  void darkmoon_deck_fathoms( special_effect_t& );
  void endless_tincture_of_fractional_power( special_effect_t& );
  // 8.0.1 - World Trinkets
  void kajafied_banana( special_effect_t& );
  void incessantly_ticking_clock( special_effect_t& );
  void snowpelt_mangler( special_effect_t& );
  void vial_of_storms( special_effect_t& );
  void landois_scrutiny( special_effect_t& );
  void leyshocks_grand_compilation( special_effect_t& );
  // 8.0.1 - Dungeon Trinkets
  void deadeye_spyglass( special_effect_t& );
  void tiny_electromental_in_a_jar( special_effect_t& );
  void mydas_talisman( special_effect_t& );
  void harlans_loaded_dice( special_effect_t& );
  void kul_tiran_cannonball_runner( special_effect_t& );
  void rotcrusted_voodoo_doll( special_effect_t& );
  void vessel_of_skittering_shadows( special_effect_t& );
  void hadals_nautilus( special_effect_t& );
  void jes_howler( special_effect_t& );
  void razdunks_big_red_button( special_effect_t& );
  void merekthas_fang( special_effect_t& );
  void lingering_sporepods( special_effect_t& );
  void lady_waycrests_music_box( special_effect_t& );
  void balefire_branch( special_effect_t& );
  // 8.0.1 - Uldir Trinkets
  void frenetic_corpuscle( special_effect_t& );
  void vigilants_bloodshaper( special_effect_t& );
}

namespace util
{
// feasts initialization helper
void init_feast( special_effect_t& effect, arv::array_view<std::pair<stat_e, int>> stat_map )
{
  effect.stat = effect.player -> convert_hybrid_stat( STAT_STR_AGI_INT );
  // TODO: Is this actually spec specific?
  if ( effect.player -> role == ROLE_TANK && !effect.player -> sim -> feast_as_dps )
    effect.stat = STAT_STAMINA;

  for ( auto&& stat : stat_map )
  {
    if ( stat.first == effect.stat )
    {
      effect.trigger_spell_id = stat.second;
      break;
    }
  }
  effect.stat_amount = effect.player -> find_spell( effect.trigger_spell_id ) -> effectN( 1 ).average( effect.player );
}

std::string tokenized_name( const spell_data_t* spell )
{
  return ::util::tokenize_fn( spell -> name_cstr() );
}

buff_stack_change_callback_t callback_buff_activator( dbc_proc_callback_t* callback )
{
  return [ callback ]( buff_t*, int old, int new_ )
  {
    if ( old == 0 )
      callback -> activate();
    else if ( new_ == 0 )
      callback -> deactivate();
  };
}

} // namespace util

// Galley Banquet ===========================================================

void consumables::galley_banquet( special_effect_t& effect )
{
  util::init_feast( effect,
    { { STAT_STRENGTH,  259452 },
      { STAT_AGILITY,   259448 },
      { STAT_INTELLECT, 259449 },
      { STAT_STAMINA,   259453 } } );
}

// Bountiful Captain's Feast ================================================

void consumables::bountiful_captains_feast( special_effect_t& effect )
{
  util::init_feast( effect,
    { { STAT_STRENGTH,  259456 },
      { STAT_AGILITY,   259454 },
      { STAT_INTELLECT, 259455 },
      { STAT_STAMINA,   259457 } } );
}

// Potion of Rising Death ===================================================

void consumables::potion_of_rising_death( special_effect_t& effect )
{
  effect.disable_action();

  // Make a bog standard damage proc for the buff
  auto secondary = new special_effect_t( effect.player );
  secondary->type = SPECIAL_EFFECT_EQUIP;
  secondary->spell_id = effect.spell_id;
  secondary->cooldown_ = timespan_t::zero();
  secondary->execute_action = create_proc_action<proc_spell_t>( "potion_of_rising_death",
      effect );
  effect.player->special_effects.push_back( secondary );

  auto proc = new dbc_proc_callback_t( effect.player, *secondary );
  proc->deactivate();
  proc->initialize();

  effect.custom_buff = buff_creator_t( effect.player, effect.name(), effect.driver() )
    .stack_change_callback( [ proc ]( buff_t*, int, int new_ ) {
      if ( new_ == 1 ) proc->activate();
      else             proc->deactivate();
    } )
    .cd( timespan_t::zero() ) // Handled by the action
    .chance( 1.0 ); // Override chance so the buff actually triggers
}

// Potion of Bursting Blood =================================================

void consumables::potion_of_bursting_blood( special_effect_t& effect )
{
  effect.disable_action();

  // Make a bog standard damage proc for the buff
  auto secondary = new special_effect_t( effect.player );
  secondary->type = SPECIAL_EFFECT_EQUIP;
  secondary->spell_id = effect.spell_id;
  secondary->cooldown_ = timespan_t::zero();
  secondary->execute_action = create_proc_action<aoe_proc_t>( "potion_bursting_blood",
      effect, "potion_of_bursting_blood", effect.trigger() );
  effect.player->special_effects.push_back( secondary );

  auto proc = new dbc_proc_callback_t( effect.player, *secondary );
  proc->deactivate();
  proc->initialize();

  effect.custom_buff = buff_creator_t( effect.player, effect.name(), effect.driver() )
    .stack_change_callback( [ proc ]( buff_t*, int, int new_ ) {
      if ( new_ == 1 ) proc->activate();
      else             proc->deactivate();
    } )
    .cd( timespan_t::zero() ) // Handled by the action
    .chance( 1.0 ); // Override chance so the buff actually triggers
}

// Gale-Force Striking ======================================================

void enchants::galeforce_striking( special_effect_t& effect )
{
  buff_t* buff = effect.player -> buffs.galeforce_striking;
  if ( !buff )
  {
    auto spell = effect.trigger();
    buff =
      make_buff( effect.player, util::tokenized_name( spell ), spell )
        -> add_invalidate( CACHE_ATTACK_SPEED )
        -> set_default_value( spell -> effectN( 1 ).percent() )
        -> set_activated( false );
    effect.player -> buffs.galeforce_striking = buff;
  }

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.player, effect );
}

// Torrent of Elements ======================================================

void enchants::torrent_of_elements( special_effect_t& effect )
{
  buff_t* buff = effect.player -> buffs.torrent_of_elements;
  if ( !buff )
  {
    auto spell = effect.trigger();
    buff =
      make_buff<buff_t>( effect.player, util::tokenized_name( spell ), spell )
        -> add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
        -> set_default_value( spell -> effectN( 1 ).percent() )
        -> set_activated( false );
    effect.player -> buffs.torrent_of_elements = buff;
  }

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.player, effect );
}

// 'XXX' Navigation (weapon enchant) ========================================

custom_cb_t enchants::weapon_navigation( unsigned buff_id )
{
  struct navigation_proc_callback_t : public dbc_proc_callback_t
  {
    buff_t* final_buff;

    navigation_proc_callback_t( player_t* p, special_effect_t& e, buff_t* b )
      : dbc_proc_callback_t( p, e ), final_buff( b )
    { }

    void execute( action_t*, action_state_t* ) override
    {
      // From logs it seems like the stacking buff can't trigger while the 'final' one is up. Logs
      // also look like the RPPM effect is allowed to proc, but the stacking buff is not triggered
      // during the final buff.
      if ( final_buff -> check() )
      {
        return;
      }

      if ( proc_buff && proc_buff -> trigger() &&
           proc_buff -> check() == proc_buff -> max_stack() )
      {
        final_buff -> trigger();
        proc_buff -> expire();
      }
    }
  };

  // Both the stacking and the final buffs are unique
  return [ buff_id ] ( special_effect_t& effect ) {
    auto spell_buff = effect.trigger();
    const std::string spell_name = util::tokenized_name( spell_buff );
    buff_t* buff = buff_t::find( effect.player, spell_name );
    if ( ! buff )
    {
      buff = make_buff<stat_buff_t>( effect.player, spell_name, spell_buff );
    }

    effect.custom_buff = buff;

    auto final_spell_data = effect.player -> find_spell( buff_id );
    const std::string final_spell_name = util::tokenized_name( final_spell_data ) + "_final";
    buff_t* final_buff = buff_t::find( effect.player, final_spell_name );
    if ( ! final_buff )
    {
      final_buff = make_buff<stat_buff_t>( effect.player, final_spell_name, final_spell_data );
    }

    new navigation_proc_callback_t( effect.player, effect, final_buff );
  };
}

// Kaja-fied Banana =========================================================

void items::kajafied_banana( special_effect_t& effect )
{
  effect.execute_action = create_proc_action<aoe_proc_t>( "kajafied_banana", effect,
      "kajafied_banana", 274575 );

  new dbc_proc_callback_t( effect.player, effect );
}

// Incessantly Ticking Clock ================================================

void items::incessantly_ticking_clock( special_effect_t& effect )
{
  stat_buff_t* tick = create_buff<stat_buff_t>( effect.player, "tick",
      effect.player->find_spell( 274430 ), effect.item );
  stat_buff_t* tock = create_buff<stat_buff_t>( effect.player, "tock",
      effect.player->find_spell( 274431 ), effect.item );

  struct clock_cb_t : public dbc_proc_callback_t
  {
    std::vector<buff_t*> buffs;
    size_t state;

    clock_cb_t( const special_effect_t& effect, const std::vector<buff_t*>& b ) :
      dbc_proc_callback_t( effect.player, effect ), buffs( b ), state( 0u )
    { }

    void execute( action_t*, action_state_t* ) override
    {
      buffs[ state ]->trigger();
      state ^= 1u;
    }

    void reset() override
    {
      dbc_proc_callback_t::reset();
      state = 0u;
    }
  };

  new clock_cb_t( effect, { tick, tock } );
}

// Snowpelt Mangler =========================================================

void items::snowpelt_mangler( special_effect_t& effect )
{
  effect.execute_action = create_proc_action<aoe_proc_t>( "sharpened_claws", effect,
      "sharpened_claws", effect.trigger() );

  new dbc_proc_callback_t( effect.player, effect );
}

// Leyshock's Grand Compilation =============================================

void items::leyshocks_grand_compilation( special_effect_t& effect )
{
  auto crit = create_buff<stat_buff_t>( effect.player, "precision_module", effect.player->find_spell( 281791 ), effect.item );
  auto haste = create_buff<stat_buff_t>( effect.player, "iteration_capacitor", effect.player->find_spell( 281792 ), effect.item );
  auto versatility = create_buff<stat_buff_t>( effect.player, "adaptive_circuit", effect.player->find_spell( 281793 ), effect.item );
  auto mastery = create_buff<stat_buff_t>( effect.player, "efficiency_widget", effect.player->find_spell( 281794 ), effect.item );

  struct leyshocks_cb_t : public dbc_proc_callback_t
  {
    std::vector<buff_t*> buffs;

    leyshocks_cb_t( const special_effect_t& effect, const std::vector<buff_t*>& b ) :
      dbc_proc_callback_t( effect.item, effect), buffs( b )
    { }

    void execute( action_t*, action_state_t* ) override
    {
      // which stat procs is entirely random
      unsigned idx = rng().roll(4);
      buffs[ idx ]->trigger();
    }
  };

  new leyshocks_cb_t( effect, {
    crit,
    haste,
    mastery,
    versatility
  } );
}

// Vial of Storms ===========================================================

void items::vial_of_storms( special_effect_t& effect )
{
  effect.execute_action = create_proc_action<aoe_proc_t>( "bottled_lightning", effect,
      "bottled_lightning", effect.trigger() );
}

// Landoi's Scrutiny ========================================================

// TODO: Targeting mechanism
void items::landois_scrutiny( special_effect_t& effect )
{
  struct ls_cb_t : public dbc_proc_callback_t
  {
    buff_t* haste;
    player_t* current_target;

    ls_cb_t( const special_effect_t& effect, buff_t* h ) :
      dbc_proc_callback_t( effect.player, effect ), haste( h )
    { }

    void reset() override
    {
      dbc_proc_callback_t::reset();

      current_target = listener->default_target;
    }

    void execute( action_t*, action_state_t* state ) override
    {
      if ( state->target != current_target )
      {
        proc_buff->expire();
        current_target = state->target;
      }

      proc_buff->trigger();

      if ( proc_buff->check() == proc_buff->max_stack() )
      {
        haste->trigger();
        proc_buff->expire();
      }
    }

  };

  auto haste_buff = create_buff<stat_buff_t>( effect.player, "landois_scrutiny_haste",
      effect.player->find_spell( 281546 ), effect.item );

  effect.custom_buff = create_buff<stat_buff_t>( effect.player, "landois_scrutiny",
      effect.trigger() );

  new ls_cb_t( effect, haste_buff );
}


// Jes' Howler ==============================================================

void items::jes_howler( special_effect_t& effect )
{
  auto main_amount = effect.driver()->effectN( 1 ).average( effect.item );
  main_amount = item_database::apply_combat_rating_multiplier( *effect.item, main_amount );

  auto ally_amount = effect.player->sim->bfa_opts.jes_howler_allies *
    effect.driver()->effectN( 2 ).average( effect.item );
  ally_amount = item_database::apply_combat_rating_multiplier( *effect.item, ally_amount );

  effect.custom_buff = create_buff<stat_buff_t>( effect.player, "motivating_howl", effect.driver(),
      effect.item )
    ->add_stat( STAT_VERSATILITY_RATING, main_amount )
    ->add_stat( STAT_VERSATILITY_RATING, ally_amount );
}

// Razdunk's Big Red Button =================================================

void items::razdunks_big_red_button( special_effect_t& effect )
{
  effect.execute_action = create_proc_action<aoe_proc_t>( "razdunks_big_red_button", effect,
      "razdunks_big_red_button", effect.trigger() );
}

// Merektha's Fang ==========================================================

void items::merekthas_fang( special_effect_t& effect )
{
  struct noxious_venom_dot_t : public proc_t
  {
    noxious_venom_dot_t( const special_effect_t& effect ) :
      proc_t( effect, "noxious_venom", 267410 )
    {
      dot_max_stack = data().max_stacks();
    }

    timespan_t calculate_dot_refresh_duration( const dot_t* dot, timespan_t triggered_duration ) const override
    { return dot->tick_event->remains() + triggered_duration; }

    double last_tick_factor( const dot_t*, const timespan_t&, const timespan_t& ) const override
    { return 1.0; }
  };

  struct noxious_venom_gland_aoe_driver_t : public spell_t
  {
    action_t* dot;

    noxious_venom_gland_aoe_driver_t( const special_effect_t& effect ) :
      spell_t( "noxious_venom_gland_aoe_driver", effect.player ),
      dot( create_proc_action<noxious_venom_dot_t>( "noxious_venom", effect ) )
    {
      aoe = -1;
      quiet = true;
      background = callbacks = false;
    }

    void impact( action_state_t* state ) override
    {
      dot->set_target( state->target );
      dot->execute();
    }
  };

  struct noxious_venom_gland_t : public proc_t
  {
    action_t* driver;

    noxious_venom_gland_t( const special_effect_t& effect ) :
      proc_t( effect, "noxious_venom_gland", effect.driver() ),
      driver( new noxious_venom_gland_aoe_driver_t( effect ) )
    {
      channeled = tick_zero = true;
      tick_may_crit = false;
    }

    void execute() override
    {
      proc_t::execute();

      event_t::cancel( player->readying );

      // Interrupts auto-attacks so push them forward by the channeling time.
      // TODO: Does this actually pause the cooldown or just let it run & insta attack potentially
      // when channel ends?
      if ( player->main_hand_attack && player->main_hand_attack->execute_event )
      {
        player->main_hand_attack->execute_event->reschedule(
            player->main_hand_attack->execute_event->remains() +
            composite_dot_duration( execute_state ) );
      }

      if ( player->off_hand_attack && player->off_hand_attack->execute_event )
      {
        player->off_hand_attack->execute_event->reschedule(
            player->off_hand_attack->execute_event->remains() +
            composite_dot_duration( execute_state ) );
      }
    }

    void tick( dot_t* d ) override
    {
      proc_t::tick( d );

      driver->set_target( d->target );
      driver->execute();
    }

    void last_tick( dot_t* d ) override
    {
      proc_t::last_tick( d );

      if ( !player->readying )
      {
        player->schedule_ready();
      }
    }
  };

  effect.execute_action = create_proc_action<noxious_venom_gland_t>( "noxious_venom_gland", effect );
}

// Dead-Eye Spyglass ========================================================

struct deadeye_spyglass_constructor_t : public item_targetdata_initializer_t
{
  deadeye_spyglass_constructor_t( unsigned iid, const std::vector< slot_e >& s ) :
    item_targetdata_initializer_t( iid, s )
  {}

  void operator()( actor_target_data_t* td ) const override
  {
    const special_effect_t* effect = find_effect( td -> source );
    if ( !effect )
    {
      td -> debuff.dead_ahead = make_buff( *td, "dead_ahead" );
      return;
    }

    assert( !td -> debuff.dead_ahead );

    special_effect_t* effect2 = new special_effect_t( effect -> item );
    effect2 -> type = SPECIAL_EFFECT_EQUIP;
    effect2 -> source = SPECIAL_EFFECT_SOURCE_ITEM;
    effect2 -> spell_id = 268758;
    effect -> player -> special_effects.push_back( effect2 );

    auto callback = new dbc_proc_callback_t( effect -> item, *effect2 );
    callback -> initialize();
    callback -> deactivate();

    td -> debuff.dead_ahead =
      make_buff( *td, "dead_ahead_debuff", effect -> trigger() )
        -> set_activated( false )
        -> set_stack_change_callback( util::callback_buff_activator( callback ) );
    td -> debuff.dead_ahead -> reset();
  }
};

void items::deadeye_spyglass( special_effect_t& effect )
{
  struct deadeye_spyglass_cb_t : public dbc_proc_callback_t
  {
    deadeye_spyglass_cb_t( const special_effect_t& effect ):
      dbc_proc_callback_t( effect.item, effect )
    {}

    void execute( action_t*, action_state_t* s ) override
    {
      auto td = listener -> get_target_data( s -> target );
      assert( td );
      assert( td -> debuff.dead_ahead );
      td -> debuff.dead_ahead -> trigger();
    }
  };

  if ( effect.spell_id == 268771 )
    new deadeye_spyglass_cb_t( effect );

  if ( effect.spell_id == 268758 )
    effect.create_buff(); // precreate the buff
}

// Tiny Electromental in a Jar ==============================================

void items::tiny_electromental_in_a_jar( special_effect_t& effect )
{
  struct unleash_lightning_t : public proc_spell_t
  {
    unleash_lightning_t( const special_effect_t& effect ):
      proc_spell_t( "unleash_lightning", effect.player, effect.player -> find_spell( 267205 ), effect.item )
    {
      aoe = data().effectN( 1 ).chain_target();
      chain_multiplier = data().effectN( 1 ).chain_multiplier();
    }
  };

  effect.custom_buff = buff_t::find( effect.player, "phenomenal_power" );
  if ( ! effect.custom_buff )
    effect.custom_buff = make_buff( effect.player, "phenomenal_power", effect.player -> find_spell( 267179 ) );

  effect.execute_action = create_proc_action<unleash_lightning_t>( "unleash_lightning", effect );

  new dbc_proc_callback_t( effect.item, effect );
}

// My'das Talisman ==========================================================

void items::mydas_talisman( special_effect_t& effect )
{
  struct touch_of_gold_cb_t : public dbc_proc_callback_t
  {
    touch_of_gold_cb_t( const special_effect_t& effect ) :
      dbc_proc_callback_t( effect.item, effect )
    {}

    void execute( action_t*, action_state_t* s ) override
    {
      assert( proc_action );
      assert( proc_buff );

      proc_action -> set_target( s -> target );
      proc_action -> execute();
      proc_buff -> decrement();
    }
  };

  auto effect2 = new special_effect_t( effect.item );
  effect2 -> name_str = "touch_of_gold";
  effect2 -> source = effect.source;
  effect2 -> proc_flags_ = effect.proc_flags();
  effect2 -> proc_chance_ = effect.proc_chance();
  effect2 -> cooldown_ = timespan_t::zero();
  effect2 -> trigger_spell_id = effect.trigger() -> id();
  effect.player -> special_effects.push_back( effect2 );

  auto callback = new touch_of_gold_cb_t( *effect2 );
  callback -> deactivate();

  effect.custom_buff = create_buff<buff_t>( effect.player, "touch_of_gold", effect.driver() )
    -> set_stack_change_callback( util::callback_buff_activator( callback ) )
    -> set_reverse( true );
  effect2 -> custom_buff = effect.custom_buff;

  // the trinket 'on use' essentially triggers itself
  effect.trigger_spell_id = effect.driver() -> id();
}

// Harlan's Loaded Dice =====================================================

// Has 2 buffs per mastery, haste, crit, one low value, one high value. Brief-ish in game testing
// shows close-ish to 50/50 chance on the high/low roll, will need a much longer test to determine
// the real probability distribution.
void items::harlans_loaded_dice( special_effect_t& effect )
{
  auto mastery_low = create_buff<stat_buff_t>( effect.player, "loaded_die_mastery_low",
      effect.player->find_spell( 267325 ), effect.item );

  auto mastery_high = create_buff<stat_buff_t>( effect.player, "loaded_die_mastery_high",
      effect.player->find_spell( 267326 ), effect.item );
  auto haste_low = create_buff<stat_buff_t>( effect.player, "loaded_die_haste_low",
      effect.player->find_spell( 267327 ), effect.item );
  auto haste_high = create_buff<stat_buff_t>( effect.player, "loaded_die_haste_high",
      effect.player->find_spell( 267329 ), effect.item );
  auto crit_low = create_buff<stat_buff_t>( effect.player, "loaded_die_critical_strike_low",
      effect.player->find_spell( 267330 ), effect.item );
  auto crit_high = create_buff<stat_buff_t>( effect.player, "loaded_die_critical_strike_high",
      effect.player->find_spell( 267331 ), effect.item );

  struct harlans_cb_t : public dbc_proc_callback_t
  {
    std::vector<std::vector<buff_t*>> buffs;

    harlans_cb_t( const special_effect_t& effect, const std::vector<std::vector<buff_t*>>& b ) :
      dbc_proc_callback_t( effect.item, effect ), buffs( b )
    { }

    void execute( action_t*, action_state_t* ) override
    {
      range::for_each( buffs, [ this ]( const std::vector<buff_t*>& buffs ) {
        // Coinflip for now
        unsigned idx = rng().roll( 0.5 ) ? 1 : 0;
        buffs[ idx ]->trigger();
      } );
    }
  };

  new harlans_cb_t( effect, {
    { mastery_low, mastery_high },
    { haste_low,   haste_high },
    { crit_low,    crit_high }
  } );
}

// Kul Tiran Cannonball Runner ==============================================

void items::kul_tiran_cannonball_runner( special_effect_t& effect )
{
  struct cannonball_cb_t : public dbc_proc_callback_t
  {
    cannonball_cb_t( const special_effect_t& effect ) :
      dbc_proc_callback_t( effect.item, effect )
    { }

    void execute( action_t*, action_state_t* state ) override
    {
      make_event<ground_aoe_event_t>( *listener->sim, listener, ground_aoe_params_t()
        .target( state -> target )
        .pulse_time( effect.trigger()->effectN( 1 ).trigger()->effectN( 2 ).period() )
        .duration( effect.trigger()->effectN( 1 ).trigger()->duration() )
        .action( proc_action ) );
    }
  };

  effect.execute_action = create_proc_action<aoe_proc_t>( "kul_tiran_cannonball_runner", effect,
      "kul_tiran_cannonball_runner", 271197 );

  new cannonball_cb_t( effect );
}

// Vessel of Skittering Shadows ==============================================

void items::vessel_of_skittering_shadows( special_effect_t& effect )
{
  effect.execute_action = create_proc_action<aoe_proc_t>( "webweavers_soul_gem", effect,
      "webweavers_soul_gem", 270827 );
  effect.execute_action->travel_speed = effect.trigger()->missile_speed();

  new dbc_proc_callback_t( effect.player, effect );
}

// Vigilant's Bloodshaper ==============================================

void items::vigilants_bloodshaper( special_effect_t& effect )
{
  effect.execute_action = create_proc_action<aoe_proc_t>( "volatile_blood_explosion", effect,
      "volatile_blood_explosion", 278057 );
  effect.execute_action->travel_speed = effect.trigger()->missile_speed();

  new dbc_proc_callback_t( effect.player, effect );
}

// Rotcrusted Voodoo Doll ===================================================

void items::rotcrusted_voodoo_doll( special_effect_t& effect )
{
  struct rotcrusted_voodoo_doll_dot_t : public proc_spell_t
  {
    action_t* final_damage;

    rotcrusted_voodoo_doll_dot_t( const special_effect_t& effect ) :
      proc_spell_t( "rotcrusted_voodoo_doll", effect.player, effect.trigger(), effect.item ),
      final_damage( new proc_spell_t( "rotcrusted_voodoo_doll_final", effect.player,
            effect.player->find_spell( 271468 ), effect.item ) )
    {
      tick_zero = true;

      add_child( final_damage );
    }

    timespan_t composite_dot_duration( const action_state_t* state ) const override
    { return ( dot_duration / base_tick_time ) * tick_time( state ); }

    void init() override
    {
      proc_spell_t::init();

      // Don't resnapshot haste on ticks so we get a nice fixed duration always
      update_flags &= ~STATE_HASTE;
    }

    void last_tick( dot_t* d ) override
    {
      proc_spell_t::last_tick( d );

      if ( !d->target->is_sleeping() )
      {
        final_damage->set_target( d->target );
        final_damage->execute();
      }
    }
  };

  effect.execute_action = create_proc_action<rotcrusted_voodoo_doll_dot_t>(
      "rotcrusted_voodoo_doll", effect );
}

// Hadal's Nautilus =========================================================

void items::hadals_nautilus( special_effect_t& effect )
{
  struct nautilus_cb_t : public dbc_proc_callback_t
  {
    const spell_data_t* driver;

    nautilus_cb_t( const special_effect_t& effect ) :
      dbc_proc_callback_t( effect.item, effect ), driver( effect.player->find_spell( 270910 ) )
    { }

    void execute( action_t*, action_state_t* state ) override
    {
      make_event<ground_aoe_event_t>( *listener->sim, listener, ground_aoe_params_t()
        .target( state -> target )
        .pulse_time( driver->duration() )
        .n_pulses( 1 )
        .action( proc_action ) );
    }
  };

  effect.execute_action = create_proc_action<aoe_proc_t>( "waterspout", effect, "waterspout",
      270925 );

  new nautilus_cb_t( effect );
}

// Lingering Sporepods ======================================================

void items::lingering_sporepods( special_effect_t& effect )
{
  effect.custom_buff = buff_t::find( effect.player, "lingering_spore_pods" );
  if ( !effect.custom_buff )
  {
    auto damage = create_proc_action<aoe_proc_t>( "lingering_spore_pods_damage", effect,
        "lingering_spore_pods_damage", 268068 );

    auto heal = create_proc_action<base_bfa_proc_t<proc_heal_t>>( "lingering_spore_pods_heal", effect,
        "lingering_spore_pods_heal", 278708 );

    effect.custom_buff = make_buff( effect.player, "lingering_spore_pods", effect.trigger() )
      -> set_stack_change_callback( [ damage, heal ]( buff_t* b, int, int new_ ) {
          if ( new_ == 0 )
          {
            damage -> set_target( b -> player -> target );
            damage -> execute();
            heal -> set_target( b -> player );
            heal -> execute();
          }
        } );
  }

  new dbc_proc_callback_t( effect.player, effect );
}

// Lady Waycrest's Music Box ================================================

void items::lady_waycrests_music_box( special_effect_t& effect )
{
  struct cacaphonous_chord_t : public proc_t
  {
    cacaphonous_chord_t( const special_effect_t& effect ) :
      proc_t( effect, "cacaphonous_chord", 271671 )
    {
      aoe = 0;
    }

    // Pick a random active target from the range
    void execute() override
    {
      size_t target_index = static_cast<size_t>( rng().range( 0, as<double>( target_list().size() ) ) );
      set_target( target_list()[ target_index ] );

      proc_t::execute();
    }
  };

  effect.execute_action = create_proc_action<cacaphonous_chord_t>( "cacaphonous_chord", effect );

  new dbc_proc_callback_t( effect.player, effect );
}

// Balefire Branch ==========================================================

void items::balefire_branch( special_effect_t& effect )
{
  effect.reverse = true;
  effect.tick = effect.driver()->effectN( 2 ).period();
  effect.reverse_stack_reduction = 5; // Nowhere to be seen in the spell data
}

// Frenetic Corpuscle =======================================================

void items::frenetic_corpuscle( special_effect_t& effect )
{
  struct frenetic_corpuscle_cb_t : public dbc_proc_callback_t
  {
    action_t* damage;

    frenetic_corpuscle_cb_t( const special_effect_t& effect ) :
      dbc_proc_callback_t( effect.item, effect ),
      damage( create_proc_action<proc_t>( "frenetic_blow", effect, "frentic_blow", 278148 ) )
    {}

    void execute( action_t* /* a */, action_state_t* state ) override
    {
      proc_buff->trigger();
      if ( proc_buff->check() == proc_buff->max_stack() )
      {
        // TODO: Tooltip says "on next attack", which likely uses Frenetic Frenzy buff (278144) to
        // proc trigger the damage Just immediately trigger the damage here, can revisit later and
        // implement a new special_effect callback if needed
        damage->set_target( state->target );
        damage->execute();
        proc_buff->expire();
      }
    }
  };

  effect.custom_buff = create_buff<buff_t>( effect.player, "frothing_rage",
      effect.player->find_spell( 278143 ), effect.item );

  new frenetic_corpuscle_cb_t( effect );
}

// Darkmoon Faire decks

void items::darkmoon_deck_squalls( special_effect_t& effect )
{
  struct squall_damage_t : public proc_spell_t
  {
    const spell_data_t* duration;

    squall_damage_t( const special_effect_t& effect, unsigned card_id ) :
      proc_spell_t( "suffocating_squall", effect.player, effect.trigger(), effect.item ),
      duration( effect.player->find_spell( card_id ) )
    { }

    timespan_t composite_dot_duration( const action_state_t* ) const override
    { return duration->effectN( 1 ).time_value(); }
  };

  new bfa_darkmoon_deck_cb_t<squall_damage_t>( effect,
      { 276124, 276125, 276126, 276127, 276128, 276129, 276130, 276131 } );
}

void items::darkmoon_deck_fathoms( special_effect_t& effect )
{
  struct fathoms_damage_t : public proc_attack_t
  {
    fathoms_damage_t( const special_effect_t& effect, unsigned card_id ) :
      proc_attack_t( "fathom_fall", effect.player, effect.player->find_spell( 276199 ),
          effect.item )
    {
      base_dd_min = base_dd_max = effect.player->find_spell( card_id )->effectN( 1 ).average(
          effect.item );
    }
  };

  new bfa_darkmoon_deck_cb_t<fathoms_damage_t>( effect,
      { 276187, 276188, 276189, 276190, 276191, 276192, 276193, 276194 } );
}

// Endless Tincture of Fractional Power =====================================

void items::endless_tincture_of_fractional_power( special_effect_t& effect )
{
  struct endless_tincture_of_fractional_power_t : public proc_spell_t
  {
    std::vector<stat_buff_t*> buffs;

    endless_tincture_of_fractional_power_t( const special_effect_t& effect ) :
      proc_spell_t( "endless_tincture_of_fractional_power", effect.player, effect.driver(), effect.item )
    {
      // TOCHECK: Buffs don't appear to scale from ilevel right now and only scale to 110
      // Blizzard either has some script magic or the spells need fixing, needs testing
      // As per Navv, added some checking of the ilevel flag in case they fix this via data
      std::vector<unsigned> buff_ids = { 265442, 265443, 265444, 265446 };
      for ( unsigned buff_id : buff_ids )
      {
        const spell_data_t* buff_spell = effect.player->find_spell( buff_id );
        if( buff_spell->flags( spell_attribute::SX_SCALE_ILEVEL ) )
          buffs.push_back( make_buff<stat_buff_t>( effect.player, util::tokenized_name( buff_spell ), buff_spell, effect.item ) );
        else
          buffs.push_back( make_buff<stat_buff_t>( effect.player, util::tokenized_name( buff_spell ), buff_spell ) );
      }
    }

    void execute() override
    {
      if ( player->consumables.flask )
      {
        const stat_buff_t* flask_buff = dynamic_cast<stat_buff_t*>( player->consumables.flask );
        if ( flask_buff && flask_buff->stats.size() > 0 )
        {
          // Check if the flask buff matches one of the trinket's stat buffs
          const stat_e flask_stat = flask_buff->stats.front().stat;
          const auto it = range::find_if( buffs, [ flask_stat ]( const stat_buff_t* buff ) {
            return buff->stats.size() > 0 && buff->stats.front().stat == flask_stat;
          } );

          if ( it != buffs.end() )
            ( *it )->trigger();
        }
      }
    }
  };

  effect.execute_action = 
    create_proc_action<endless_tincture_of_fractional_power_t>( "endless_tincture_of_fractional_power", effect );
}

} // namespace bfa
} // anon namespace

void unique_gear::register_special_effects_bfa()
{
  using namespace bfa;

  // Consumables
  register_special_effect( 259409, consumables::galley_banquet );
  register_special_effect( 259410, consumables::bountiful_captains_feast );
  register_special_effect( 269853, consumables::potion_of_rising_death );
  register_special_effect( 251316, consumables::potion_of_bursting_blood );

  // Enchants
  register_special_effect( 255151, enchants::galeforce_striking );
  register_special_effect( 255150, enchants::torrent_of_elements );
  register_special_effect( 268855, enchants::weapon_navigation( 268856 ) ); // Versatile Navigation
  register_special_effect( 268888, enchants::weapon_navigation( 268893 ) ); // Quick Navigation
  register_special_effect( 268900, enchants::weapon_navigation( 268898 ) ); // Masterful Navigation
  register_special_effect( 268906, enchants::weapon_navigation( 268904 ) ); // Deadly Navigation
  register_special_effect( 268912, enchants::weapon_navigation( 268910 ) ); // Stalwart Navigation
  register_special_effect( 264876, "264878Trigger" ); // Crow's Nest Scope
  register_special_effect( 264958, "264957Trigger" ); // Monelite Scope of Alacrity
  register_special_effect( 265090, "265092Trigger" ); // Incendiary Ammunition
  register_special_effect( 265094, "265096Trigger" ); // Frost-Laced Ammunition

  // Trinkets
  register_special_effect( 274484, items::kajafied_banana );
  register_special_effect( 274429, items::incessantly_ticking_clock );
  register_special_effect( 268517, items::snowpelt_mangler );
  register_special_effect( 268544, items::vial_of_storms );
  register_special_effect( 281544, items::landois_scrutiny );
  register_special_effect( 281547, items::leyshocks_grand_compilation);
  register_special_effect( 266047, items::jes_howler );
  register_special_effect( 271374, items::razdunks_big_red_button );
  register_special_effect( 267402, items::merekthas_fang );
  register_special_effect( 268758, items::deadeye_spyglass );
  register_special_effect( 268771, items::deadeye_spyglass );
  register_special_effect( 267177, items::tiny_electromental_in_a_jar );
  register_special_effect( 265954, items::mydas_talisman );
  register_special_effect( 274835, items::harlans_loaded_dice );
  register_special_effect( 271190, items::kul_tiran_cannonball_runner );
  register_special_effect( 271462, items::rotcrusted_voodoo_doll );
  register_special_effect( 270809, items::vessel_of_skittering_shadows);
  register_special_effect( 278053, items::vigilants_bloodshaper);
  register_special_effect( 270921, items::hadals_nautilus );
  register_special_effect( 268035, items::lingering_sporepods );
  register_special_effect( 271117, "4Tick" );
  register_special_effect( 271631, items::lady_waycrests_music_box );
  register_special_effect( 268999, items::balefire_branch );
  register_special_effect( 268314, "268311Trigger" ); // Galecaller's Boon, assumes the player always stands in the area
  register_special_effect( 278140, items::frenetic_corpuscle );
  register_special_effect( 278383, "Reverse" ); // Azurethos' Singed Plumage

  // Misc
  register_special_effect( 276123, items::darkmoon_deck_squalls );
  register_special_effect( 276176, items::darkmoon_deck_fathoms );
  register_special_effect( 265440, items::endless_tincture_of_fractional_power );
}

void unique_gear::register_target_data_initializers_bfa( sim_t* sim )
{
  using namespace bfa;
  const std::vector<slot_e> items = { SLOT_TRINKET_1, SLOT_TRINKET_2 };

  sim -> register_target_data_initializer( deadeye_spyglass_constructor_t( 159623, items ) );
}
