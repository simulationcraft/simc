// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

#include "pet_spawner.hpp"

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
  bool aoe_damage_increase;

  base_bfa_aoe_proc_t( const special_effect_t& effect, const std::string& name,
                       unsigned spell_id, bool aoe_damage_increase_ = false ) :
    base_bfa_proc_t<BASE>( effect, name, spell_id ),
    aoe_damage_increase( aoe_damage_increase_ )
  {
    this->aoe = -1;
    this->split_aoe_damage = true;
  }

  base_bfa_aoe_proc_t( const special_effect_t& effect, const std::string& name,
                       const spell_data_t* s, bool aoe_damage_increase_ = false ) :
    base_bfa_proc_t<BASE>( effect, name, s ),
    aoe_damage_increase( aoe_damage_increase_ )
  {
    this->aoe = -1;
    this->split_aoe_damage = true;
  }

  virtual double action_multiplier() const override
  {
    double am = base_bfa_proc_t<BASE>::action_multiplier();

    if ( aoe_damage_increase )
    {
      // 15% extra damage per target hit, up to 6 targets. Hidden somewhere on
      // the server side.
      // TODO: Check if the target limit is the same for all these trinkets.
      am *= 1.0 + 0.15 * ( std::min( as<int>( this->target_list().size() ), 6 ) - 1 );
    }

    return am;
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
  void boralus_blood_sausage( special_effect_t& );
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
  void bygone_bee_almanac( special_effect_t& );
  void leyshocks_grand_compilation( special_effect_t& );
  void berserkers_juju( special_effect_t& );
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
  void lady_waycrests_music_box_heal( special_effect_t& );
  void balefire_branch( special_effect_t& );
  void vial_of_animated_blood( special_effect_t& );
  // 8.0.1 - World Boss Trinkets
  void sandscoured_idol( special_effect_t& );
  // 8.0.1 - Uldir Trinkets
  void frenetic_corpuscle( special_effect_t& );
  void vigilants_bloodshaper( special_effect_t& );
  void twitching_tentacle_of_xalzaix( special_effect_t& );
  void vanquished_tendril_of_ghuun( special_effect_t& );
  void syringe_of_bloodborne_infirmity( special_effect_t& );
  void disc_of_systematic_regression( special_effect_t& );
  // 8.1.0 - Battle of Dazar'alor Trinkets
  void incandescent_sliver( special_effect_t& );
  void tidestorm_codex( special_effect_t& );
  void invocation_of_yulon( special_effect_t& );
  void variable_intensity_gigavolt_oscillating_reactor( special_effect_t& );
  void variable_intensity_gigavolt_oscillating_reactor_onuse( special_effect_t& );
  void everchill_anchor( special_effect_t& );
  void ramping_amplitude_gigavolt_engine( special_effect_t& );
  void grongs_primal_rage( special_effect_t& );
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

namespace set_bonus
{
  // 8.0 Dungeon
  void waycrest_legacy( special_effect_t& );
  // 8.1.0 Raid
  void gift_of_the_loa( special_effect_t& );
  void keepsakes_of_the_resolute_commandant( special_effect_t& );
}

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

// Boralus Blood Sausage ================================================

void consumables::boralus_blood_sausage( special_effect_t& effect )
{
  util::init_feast( effect,
    { { STAT_STRENGTH,  290469 },
      { STAT_AGILITY,   290467 },
      { STAT_INTELLECT, 290468 } } );
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


// Bygone Bee Almanac =======================================================

enum resource_category : unsigned
{
  RC_BUFF = 0u, RC_COOLDOWN
};

// Add different resource systems here
// Todo : values were changed, need confirmation on :
// rage, runic power, shield of the righteous charges
static const std::unordered_map<int, std::tuple<double, double>> __resource_map { {
  { RESOURCE_HOLY_POWER,  std::tuple<double, double> { 0.800, 0.800 } },
  { RESOURCE_RUNIC_POWER, std::tuple<double, double> { 0.075, 0.054 } },
  { RESOURCE_RAGE,        std::tuple<double, double> { 0.025, 0.050 } }
} };

struct bba_cb_t : public dbc_proc_callback_t
{
  bba_cb_t( const special_effect_t& effect ) :
    dbc_proc_callback_t( effect.player, effect )
  { }

  template <resource_category c>
  timespan_t value( action_state_t* state ) const
  {
    int resource = static_cast<int>( state->action->current_resource() );
    double cost = state->action->last_resource_cost;

    // Breath of Sindragosa needs to be special cased, as the tick action doesn't cost any resource
    if ( state -> action -> data().id() == 155166u )
    {
      cost = 15.0;
      resource = RESOURCE_RUNIC_POWER;

      // There seems to be a bug / special handling of BoS on server side
      // It only triggers bygone bee ~60% of the time
      // This will need more data
      if ( state -> action -> player -> bugs && rng().roll( 0.4 ) )
      {
        return timespan_t::zero();
      }
    }

    // Protection Paladins are special :
    // - Consuming SotR charges with Seraphim doesn't trigger the callback
    // - Even though SotR can be used without a target, the effect only happens if the spell hits at
    //   least one target and has a 3.0s value for both buff extension and the on-use's cdr
    if ( state -> action -> data().id() == 53600u &&
         state -> action -> result_is_hit( state -> result ) )
    {
      return timespan_t::from_seconds( c == RC_BUFF ? 3.0 : 3.0 );
    }

    if ( cost == 0.0 )
    {
      return timespan_t::zero();
    }

    auto resource_tuple = __resource_map.find( resource );
    if ( resource_tuple == __resource_map.end() )
    {
      return timespan_t::zero();
    }

    return timespan_t::from_seconds( cost * std::get<c>( resource_tuple->second ) );
  }

  void trigger( action_t* a, void* raw_state ) override
  {
    auto state = reinterpret_cast<action_state_t*>( raw_state );

    if ( state -> action -> last_resource_cost == 0 )
    {
      // Custom call here for BoS ticks and SotR because they don't consume resources
      // The callback should probably rather check for ticks of breath_of_sindragosa, but this will do for now
      if ( state -> action -> data().id() == 155166u ||
           state -> action -> data().id() == 53600u )
      {
        dbc_proc_callback_t::trigger( a, raw_state );
      }

      return;
    }

    dbc_proc_callback_t::trigger( a, raw_state );
  }

  void reset() override
  {
    dbc_proc_callback_t::reset();

    deactivate();
  }
};

void items::bygone_bee_almanac( special_effect_t& effect )
{
  struct bba_active_cb_t : public bba_cb_t
  {
    buff_t* bba_buff;

    bba_active_cb_t( const special_effect_t& effect ) : bba_cb_t( effect ), bba_buff( nullptr )
    { }

    void initialize() override
    {
      bba_cb_t::initialize();

      bba_buff = buff_t::find( listener, "process_improvement" );
      assert( bba_buff );
    }

    void execute( action_t*, action_state_t* state ) override
    {
      timespan_t adjust = value<RC_BUFF>( state );
      if ( adjust == timespan_t::zero() )
      {
        return;
      }

      bba_buff->extend_duration( listener, adjust );
    }
  };

  struct bba_inactive_cb_t : public bba_cb_t
  {
    cooldown_t* cd;

    bba_inactive_cb_t( const special_effect_t& effect, const special_effect_t& base ) :
      bba_cb_t( effect ), cd( listener->get_cooldown( base.cooldown_name() ) )
    { }

    void execute( action_t*, action_state_t* state ) override
    {
      timespan_t adjust = value<RC_COOLDOWN>( state );
      if ( adjust == timespan_t::zero() )
      {
        return;
      }

      cd->adjust( -adjust );
    }
  };

  auto bba_active_effect = new special_effect_t( effect.player );
  bba_active_effect->name_str = "bba_active_driver_callback";
  bba_active_effect->type = SPECIAL_EFFECT_EQUIP;
  bba_active_effect->source = SPECIAL_EFFECT_SOURCE_ITEM;
  bba_active_effect->proc_chance_ = 1.0;
  bba_active_effect->proc_flags_ = PF_ALL_DAMAGE | PF_PERIODIC;
  effect.player->special_effects.push_back( bba_active_effect );

  auto bba_active = new bba_active_cb_t( *bba_active_effect );

  auto bba_inactive_effect = new special_effect_t( effect.player );
  bba_inactive_effect->name_str = "bba_inactive_driver_callback";
  bba_inactive_effect->type = SPECIAL_EFFECT_EQUIP;
  bba_inactive_effect->source = SPECIAL_EFFECT_SOURCE_ITEM;
  bba_inactive_effect->proc_chance_ = 1.0;
  bba_inactive_effect->proc_flags_ = PF_ALL_DAMAGE | PF_PERIODIC;
  effect.player->special_effects.push_back( bba_inactive_effect );

  auto bba_inactive = new bba_inactive_cb_t( *bba_inactive_effect, effect );
  effect.custom_buff = create_buff<stat_buff_t>( effect.player, "process_improvement",
      effect.trigger(), effect.item )
    -> set_stack_change_callback( [ bba_active, bba_inactive ]( buff_t*, int, int new_ ) {
        if ( new_ == 0 )
        {
          bba_active->deactivate();
          bba_inactive->activate();
        }
        else
        {
          bba_inactive->deactivate();
          bba_active->activate();
        }
    } );

  // Note must be done after buff creation above
  bba_active->initialize();
  bba_inactive->initialize();

  bba_active->deactivate();
  bba_inactive->deactivate();
}

// Leyshock's Grand Compilation ============================================

void items::leyshocks_grand_compilation( special_effect_t& effect )
{
  // Crit
  effect.player->buffs.leyshock_crit = create_buff<stat_buff_t>( effect.player, "precision_module",
      effect.player->find_spell( 281791 ), effect.item );
  effect.player->buffs.leyshock_haste = create_buff<stat_buff_t>( effect.player, "iteration_capacitor",
      effect.player->find_spell( 281792 ), effect.item );
  effect.player->buffs.leyshock_mastery = create_buff<stat_buff_t>( effect.player, "efficiency_widget",
      effect.player->find_spell( 281794 ), effect.item );
  effect.player->buffs.leyshock_versa = create_buff<stat_buff_t>( effect.player, "adaptive_circuit",
      effect.player->find_spell( 281795 ), effect.item );

  // We don't need to make a special effect from this item at all, since it needs very special
  // handling. Just going to use the callback to initialize the buffs.
  effect.type = SPECIAL_EFFECT_NONE;
}

// Jes' Howler ==============================================================

void items::jes_howler( special_effect_t& effect )
{
  // The first versatility gain component is divided among the player and up to 4 allies in the game
  // This will not buff other actors in the sim
  // Default behavior is buffing 4 allies
  auto main_amount = effect.driver()->effectN( 1 ).average( effect.item );
  main_amount = item_database::apply_combat_rating_multiplier( *effect.item, main_amount );
  main_amount /= ( 1 + effect.player->sim->bfa_opts.jes_howler_allies );

  // The player gains additional versatility for every ally buffed
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
      "razdunks_big_red_button", effect.trigger(), true );
}

// Merektha's Fang ==========================================================

void items::merekthas_fang( special_effect_t& effect )
{
  struct noxious_venom_dot_t : public proc_t
  {
    noxious_venom_dot_t( const special_effect_t& effect ) :
      proc_t( effect, "noxious_venom", 267410 )
    {
      tick_may_crit = hasted_ticks = true;
      dot_max_stack = data().max_stacks();
    }

    timespan_t calculate_dot_refresh_duration( const dot_t* dot, timespan_t triggered_duration ) const override
    {
      // No pandemic, refreshes to base duration on every channel tick
      return triggered_duration * dot->state->haste;
    }

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
      channeled = hasted_ticks = true;
      tick_may_crit = tick_zero = false;
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
      "kul_tiran_cannonball_runner", 271197, true );

  new cannonball_cb_t( effect );
}

// Vessel of Skittering Shadows ==============================================

void items::vessel_of_skittering_shadows( special_effect_t& effect )
{
  effect.execute_action = create_proc_action<aoe_proc_t>( "webweavers_soul_gem", effect,
      "webweavers_soul_gem", 270827, true );
  effect.execute_action->travel_speed = effect.trigger()->missile_speed();

  new dbc_proc_callback_t( effect.player, effect );
}

// Vigilant's Bloodshaper ==============================================

void items::vigilants_bloodshaper( special_effect_t& effect )
{
  effect.execute_action = create_proc_action<aoe_proc_t>( "volatile_blood_explosion", effect,
    "volatile_blood_explosion", 278057, true );
  effect.execute_action->travel_speed = effect.trigger()->missile_speed();

  new dbc_proc_callback_t( effect.player, effect );
}

// Twitching Tentacle of Xalzaix

void items::twitching_tentacle_of_xalzaix( special_effect_t& effect )
{
  struct lingering_power_callback_t : public dbc_proc_callback_t
  {
    buff_t* final_buff;

    lingering_power_callback_t( player_t* p, special_effect_t& e, buff_t* b )
      : dbc_proc_callback_t( p, e ), final_buff( b )
    { }

    void execute( action_t*, action_state_t* ) override
    {
      if ( proc_buff && proc_buff -> trigger() && proc_buff -> check() == proc_buff -> max_stack() )
      {
        final_buff -> trigger();
        proc_buff -> expire();
      }
    }
  };

  effect.proc_flags2_ = PF2_ALL_HIT;

  // Doesn't seem to be linked in the effect.
  auto buff_data = effect.player -> find_spell( 278155 );
  buff_t* buff = buff_t::find( effect.player, "lingering_power_of_xalzaix" );
  if ( ! buff )
  {
    buff = make_buff( effect.player, "lingering_power_of_xalzaix", buff_data, effect.item );
  }

  effect.custom_buff = buff;

  auto final_buff_data = effect.player -> find_spell( 278156 );
  buff_t* final_buff = buff_t::find( effect.player, "uncontained_power" );
  if ( ! final_buff )
  {
    final_buff = make_buff<stat_buff_t>( effect.player, "uncontained_power", final_buff_data, effect.item );
  }

  new lingering_power_callback_t( effect.player, effect, final_buff );
}

// Vanquished Tendril of G'huun =============================================

void items::vanquished_tendril_of_ghuun( special_effect_t& effect )
{
  struct bloody_bile_t : public proc_spell_t
  {
    unsigned n_casts;

    bloody_bile_t( player_t* p, const special_effect_t& effect ) :
      proc_spell_t( "bloody_bile", p, p->find_spell( 279664 ), effect.item ),
      n_casts( 0 )
    {
      background = false;
      base_dd_min = effect.driver()->effectN( 1 ).min( effect.item );
      base_dd_max = effect.driver()->effectN( 1 ).max( effect.item );
    }

    void execute() override
    {
      proc_spell_t::execute();

      // Casts 6 times and then despawns
      if ( ++n_casts == 6 )
      {
        make_event( *sim, timespan_t::zero(), [ this ]() { player->cast_pet()->dismiss(); } );
      }
    }
  };

  struct tendril_pet_t : public pet_t
  {
    bloody_bile_t* bloody_bile;
    const special_effect_t* effect;

    tendril_pet_t( const special_effect_t& effect ) :
      pet_t( effect.player->sim, effect.player, "vanquished_tendril_of_ghuun", true, true ),
      bloody_bile( nullptr ), effect( &effect )
    { }

    // Chill until we can shoot another bloody bile
    timespan_t available() const override
    {
      auto delta = bloody_bile->cooldown->ready - sim->current_time();
      if ( delta > timespan_t::zero() )
      {
        return delta;
      }

      return pet_t::available();
    }

    void arise() override
    {
      pet_t::arise();
      bloody_bile->n_casts = 0u;
    }

    action_t* create_action( const std::string& name, const std::string& opts ) override
    {
      if ( ::util::str_compare_ci( name, "bloody_bile" ) )
      {
        bloody_bile = new bloody_bile_t( this, *effect );
        return bloody_bile;
      }
      return pet_t::create_action( name, opts );
    }

    void init_action_list() override
    {
      pet_t::init_action_list();

      get_action_priority_list( "default" )->add_action( "bloody_bile" );
    }
  };

  struct tendril_cb_t : public dbc_proc_callback_t
  {
    spawner::pet_spawner_t<tendril_pet_t> spawner;

    tendril_cb_t( const special_effect_t& effect ) :
      dbc_proc_callback_t( effect.player, effect ),
      spawner( "vanquished_tendril_of_ghuun", effect.player, [&effect]( player_t* ) {
        return new tendril_pet_t( effect ); } )
    {
      spawner.set_default_duration( effect.player->find_spell( 278163 )->duration() );
    }

    void execute( action_t*, action_state_t* ) override
    { spawner.spawn(); }
  };

  new tendril_cb_t( effect );
}

// Vial of Animated Blood ==========================================================

void items::vial_of_animated_blood( special_effect_t& effect )
{
    effect.reverse = true;
    effect.tick = effect.driver() -> effectN( 2 ).period();
    effect.reverse_stack_reduction = 1;
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
      travel_speed = effect.driver()->missile_speed();

      add_child( final_damage );
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
      270925, true );

  new nautilus_cb_t( effect );
}

// Lingering Sporepods ======================================================

void items::lingering_sporepods( special_effect_t& effect )
{
  effect.custom_buff = buff_t::find( effect.player, "lingering_spore_pods" );
  if ( !effect.custom_buff )
  {
    // TODO 8.1: Now deals increased damage with more targets.
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
struct waycrest_legacy_damage_t : public proc_t
{
  waycrest_legacy_damage_t( const special_effect_t& effect ):
    proc_t( effect, "waycrest_legacy_damage", 271671 )
  {
    aoe = 0;
    base_dd_multiplier = effect.player->find_spell( 277522 )->effectN( 2 ).base_value() / 100.0;
  }
  void execute() override
  {
    size_t target_index = static_cast<size_t>( rng().range( 0, as<double>( target_list().size() ) ) );
    set_target( target_list()[ target_index ] );

    proc_t::execute();
  }
};

struct waycrest_legacy_heal_t : public base_bfa_proc_t<proc_heal_t>
{
  waycrest_legacy_heal_t( const special_effect_t& effect ):
    base_bfa_proc_t<proc_heal_t>( effect, "waycrest_legacy_heal", 271682 )
  {
    aoe = 0;
    base_dd_multiplier = effect.player->find_spell( 277522 )->effectN( 2 ).base_value() / 100.0;
  }
  void execute() override
  {
    size_t target_index = static_cast< size_t >( rng().range( 0, as<double>( sim->player_no_pet_list.data().size() ) ) );
    set_target( sim->player_list.data()[ target_index ] );

    base_bfa_proc_t<proc_heal_t>::execute();
  }
};


void items::lady_waycrests_music_box( special_effect_t& effect )
{
  struct cacaphonous_chord_t : public proc_t
  {
    const spell_data_t* waycrests_legacy = player->find_spell( 277522 );
    action_t* waycrests_legacy_heal;
    cacaphonous_chord_t( const special_effect_t& effect ) :
      proc_t( effect, "cacaphonous_chord", 271671 )
    {
      aoe = 0;
    }

    void init() override
    {
      proc_t::init();
      waycrests_legacy_heal = player->find_action( "waycrest_legacy_heal" );
    }

    // Pick a random active target from the range
    void execute() override
    {
      size_t target_index = static_cast<size_t>( rng().range( 0, as<double>( target_list().size() ) ) );
      set_target( target_list()[ target_index ] );

      proc_t::execute();
      if ( waycrests_legacy_heal != nullptr )
      {
        if ( rng().roll( waycrests_legacy->effectN( 1 ).base_value() / 100.0 ) )
        {
          waycrests_legacy_heal->schedule_execute();
        }
      }
    }
  };

  effect.execute_action = create_proc_action<cacaphonous_chord_t>( "cacaphonous_chord", effect );

  new dbc_proc_callback_t( effect.player, effect );
}

void items::lady_waycrests_music_box_heal( special_effect_t& effect )
{
  struct harmonious_chord_t : public base_bfa_proc_t<proc_heal_t>
  {
    const spell_data_t* waycrests_legacy = player->find_spell( 277522 );
    action_t* waycrests_legacy_damage;
    harmonious_chord_t( const special_effect_t& effect ):
      base_bfa_proc_t<proc_heal_t>( effect, "harmonious_chord", 271682 )
    {
      aoe = 0;
    }

    void init() override
    {
      proc_heal_t::init();
      waycrests_legacy_damage = player->find_action( "waycrest_legacy_damage" );
    }

    void execute() override
    {
      size_t target_index = static_cast< size_t >( rng().range( 0, as<double>( sim->player_no_pet_list.data().size() ) ) );
      set_target( sim->player_list.data()[ target_index ] );

      base_bfa_proc_t<proc_heal_t>::execute();

      if ( waycrests_legacy_damage != nullptr )
      {
        if ( rng().roll( waycrests_legacy->effectN( 1 ).base_value() / 100.0 ) )
        {
          waycrests_legacy_damage->schedule_execute();
        }
      }
    }
  };
  effect.execute_action = create_proc_action<harmonious_chord_t>( "harmonious_chord", effect );

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

// Sandscoured Idol =======================================================

void items::sandscoured_idol( special_effect_t& effect )
{
  effect.reverse = true;
  effect.tick = effect.driver()->effectN( 2 ).period();
  effect.reverse_stack_reduction = 4;
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
    {
      tick_may_crit = true;
    }

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

// Syringe of Bloodborne Infirmity ==========================================

struct syringe_of_bloodborne_infirmity_constructor_t : public item_targetdata_initializer_t
{
  syringe_of_bloodborne_infirmity_constructor_t( unsigned iid, const std::vector< slot_e >& s ) :
    item_targetdata_initializer_t( iid, s )
  {}

  void operator()( actor_target_data_t* td ) const override
  {
    const special_effect_t* effect = find_effect( td -> source );
    if ( !effect )
    {
      td -> debuff.wasting_infection = make_buff( *td, "wasting_infection" );
      return;
    }
    assert( !td -> debuff.wasting_infection );

    special_effect_t* effect2 = new special_effect_t( effect -> item );
    effect2 -> type = SPECIAL_EFFECT_EQUIP;
    effect2 -> source = SPECIAL_EFFECT_SOURCE_ITEM;
    effect2 -> spell_id = 278109;
    effect -> player -> special_effects.push_back( effect2 );

    auto callback = new dbc_proc_callback_t( effect -> item, *effect2 );
    callback -> initialize();
    callback -> deactivate();

    td -> debuff.wasting_infection =
      make_buff( *td, "wasting_infection_debuff", effect -> trigger() )
      -> set_activated( false )
      -> set_stack_change_callback( util::callback_buff_activator( callback ) );
    td -> debuff.wasting_infection -> reset();
  }
};

void items::syringe_of_bloodborne_infirmity( special_effect_t& effect )
{
  struct syringe_of_bloodborne_infirmity_cb_t : public dbc_proc_callback_t
  {
    action_t* damage;
    syringe_of_bloodborne_infirmity_cb_t( const special_effect_t& effect ) :
      dbc_proc_callback_t( effect.item, effect ),
      damage( create_proc_action<proc_spell_t>( "wasting_infection", effect ) )
    {}

    void execute( action_t* /* a */, action_state_t* state ) override
    {
      damage -> set_target( state -> target );
      damage -> execute();

      auto td = listener -> get_target_data( state -> target );
      assert( td );
      assert( td -> debuff.wasting_infection );
      td -> debuff.wasting_infection -> trigger();
    }
  };

  // dot proc on attack
  if ( effect.spell_id == 278112 )
    new syringe_of_bloodborne_infirmity_cb_t( effect );

  // crit buff on attacking debuffed target
  if ( effect.spell_id == 278109 )
    effect.create_buff(); // precreate the buff
}

// Disc of Systematic Regression ============================================

void items::disc_of_systematic_regression( special_effect_t& effect )
{
  effect.execute_action = create_proc_action<aoe_proc_t>( "voided_sectors", effect,
    "voided_sectors", 278153, true );

  new dbc_proc_callback_t( effect.player, effect );
}

//Berserker's Juju ============================================

void items::berserkers_juju( special_effect_t& effect )
{
  struct berserkers_juju_t : public proc_spell_t
  {
    stat_buff_t* buff;
    const spell_data_t* spell;
    double amount;

    berserkers_juju_t( const special_effect_t& effect ):
      proc_spell_t( "berserkers_juju", effect.player, effect.driver(), effect.item )
    {
      const spell_data_t* spell = effect.item->player->find_spell( 274472 );
      std::string buff_name = util::tokenized_name( spell );

      buff = make_buff<stat_buff_t>( effect.player, buff_name, spell );
    }

    void init() override
    {
      spell = player -> find_spell( 274472 );
      //force to use -7 scaling type on spelldata value
      //Should probably add a check here for if spelldata changes to -7 to avoid double dipping
      amount = item_database::apply_combat_rating_multiplier( *item, spell->effectN( 1 ).average( item ) );
      proc_spell_t::init( );
    }

    virtual void execute( ) override
    {
      //Testing indicates buff amount is +-15% of spelldata (scaled with -7 type) amount
      double health_percent = ( std::max( player -> resources.current[ RESOURCE_HEALTH ] * 1.0, 0.0 ) ) / player -> resources.max[ RESOURCE_HEALTH ] * 1.0;

      //Then apply +-15 to that scaled values
      double buff_amount = amount * ( 0.85 + ( 0.3 * (1 - health_percent ) ) );

      buff -> stats[ 0 ].amount = buff_amount;

      if ( player->sim->debug )
        player->sim->out_debug.printf( "%s bersker's juju procs haste=%.0f of spelldata_raw_amount=%.0f spelldata_scaled_amount=%.0f",
          player->name(), buff_amount, spell->effectN( 1 ).average( item ), amount );

      buff -> trigger();
    }
  };

  effect.execute_action = create_proc_action <berserkers_juju_t>( "berserkers_juju", effect );
}

// Incandescent Sliver ===================================================

void items::incandescent_sliver( special_effect_t& effect )
{
  player_t* p = effect.player;

  buff_t* mastery_buff = buff_t::find( p, "incandescent_brilliance" );
  if ( !mastery_buff )
  {
    mastery_buff = make_buff<stat_buff_t>( p, "incandescent_brilliance", p->find_spell( 289524 ), effect.item );
  }

  buff_t* crit_buff = buff_t::find( p, "incandescent_luster" );
  if ( !crit_buff )
  {
    crit_buff = make_buff<stat_buff_t>( p, "incandescent_luster", p->find_spell( 289523 ), effect.item )
      ->set_stack_change_callback( [ mastery_buff ] ( buff_t* b, int, int cur )
        {
          if ( cur == b->max_stack() )
            mastery_buff->trigger();
          else
            mastery_buff->expire();
        } );
  }

  timespan_t period = effect.driver()->effectN( 1 ).period();
  p->register_combat_begin( [ crit_buff, period ] ( player_t* )
  {
    make_repeating_event( crit_buff->sim, period, [ crit_buff ]
    {
      if ( crit_buff->rng().roll( crit_buff->sim->bfa_opts.incandescent_sliver_chance ) )
        crit_buff->trigger();
      else
        crit_buff->decrement();
    } );
  } );
}

// Tidestorm Codex =======================================================

void items::tidestorm_codex( special_effect_t& effect )
{
  struct surging_burst_t : public aoe_proc_t
  {
    surging_burst_t( const special_effect_t& effect ) :
      aoe_proc_t( effect, "surging_burst", 288086, true )
    {
      // Damage values are in a different spell, which isn't linked from the driver.
      base_dd_min = base_dd_max = player->find_spell( 288046 )->effectN( 1 ).average( effect.item );

      // Travel speed is around 10 yd/s, doesn't seem to be in spell data.
      travel_speed = 10.0;
    }
  };

  struct surging_waters_t : public proc_t
  {
    action_t* damage;

    surging_waters_t( const special_effect_t& effect ) :
      proc_t( effect, "surging_waters", effect.driver() )
    {
      channeled = true;

      if ( player->bugs )
      {
        // For some reason, the elementals keep spawning if you interrupt the channel
        // by casting another cast time spell (but not if you interrupt by moving or
        // /stopcasting). This is really annoying to model properly in simc, so here
        // we just increase elemental spawn frequency and set channel time to match
        // current GCD.
        base_tick_time *= 0.5;
      }

      damage = create_proc_action<surging_burst_t>( "surging_burst", effect );
      add_child( damage );
    }

    timespan_t composite_dot_duration( const action_state_t* ) const override
    {
      if ( player->bugs )
        // Assume we're queueing another spell as soon as possible, i.e.
        // after the GCD has elapsed.
        return std::max( player->cache.spell_speed() * trigger_gcd, player->min_gcd );
      else
        return dot_duration;
    }

    double composite_haste() const override
    {
      // Does not benefit from haste.
      return 1.0;
    }

    void execute() override
    {
      proc_t::execute();

      // This action is a background channel. use_item_t has already scheduled player ready event,
      // so we need to remove it to prevent the player from channeling and doing something else at
      // the same time.
      event_t::cancel( player->readying );
    }

    void tick( dot_t* d ) override
    {
      proc_t::tick( d );
      // Spawn up to six elementals, see the bug note above.
      if ( d->current_tick <= as<int>( data().effectN( 3 ).base_value() ) )
      {
        damage->set_target( d->target );
        damage->execute();
      }
    }

    void last_tick( dot_t* d ) override
    {
      bool was_channeling = player->channeling == this;
      proc_t::last_tick( d );

      if ( was_channeling && !player->readying )
      {
        // Since this is a background channel, we need to manually wake
        // the player up.

        timespan_t wait = 0_ms;
        if ( !player->bugs )
        {
          // Without bugs, we assume that the full channel needs to happen to get
          // all six elementals. This means we need take into account channel lag.
          wait = rng().gauss( sim->channel_lag, sim->channel_lag_stddev );
        }

        player->schedule_ready( wait );
      }
    }
  };

  effect.execute_action = create_proc_action<surging_waters_t>( "surging_waters", effect );
}

// Invocation of Yu'lon ==================================================

void items::invocation_of_yulon( special_effect_t& effect )
{
  effect.execute_action = create_proc_action<aoe_proc_t>( "yulons_fury", effect,
    "yulons_fury", 288282, true );
  effect.execute_action->travel_speed = effect.driver()->missile_speed();
}

// Variable Intensity Gigavolt Oscillating Reactor =======================

struct vigor_engaged_t : public special_effect_t
{
  // Phases of the buff
  enum oscillation : unsigned
  {
    ASCENDING = 0u,     // Ascending towards max stack
    MAX_STACK,          // Coasting at max stack
    DESCENDING,         // Descending towards zero stack
    INACTIVE,           // Hibernating at zero stack
    MAX_STATES
  };

  oscillation current_oscillation = oscillation::ASCENDING;
  unsigned ticks_at_oscillation = 0u;
  std::array<unsigned, oscillation::MAX_STATES> max_ticks;
  std::array<oscillation, oscillation::MAX_STATES> transition_map = {
    { MAX_STACK, DESCENDING, INACTIVE, ASCENDING }
  };

  event_t* tick_event     = nullptr;
  stat_buff_t* vigor_buff = nullptr;
  buff_t* cooldown_buff   = nullptr;

  // Oscillation overload stuff
  const spell_data_t* overload_spell = nullptr;
  cooldown_t*         overload_cd    = nullptr;
  buff_t*             overload_buff  = nullptr;

  static std::string oscillation_str( oscillation s )
  {
    switch ( s )
    {
      case oscillation::ASCENDING:  return "ascending";
      case oscillation::DESCENDING: return "descending";
      case oscillation::MAX_STACK:  return "max_stack";
      case oscillation::INACTIVE:   return "inactive";
      default:                      return "unknown";
    }
  }

  vigor_engaged_t( const special_effect_t& effect ) : special_effect_t( effect )
  {
    // Initialize max oscillations from spell data, buff-adjusting phases are -1 ticks, since the
    // transition event itself adjusts the buff stack count by one (up or down)
    max_ticks[ ASCENDING ] = max_ticks[ DESCENDING ] = driver()->effectN( 2 ).base_value() - 1;
    max_ticks[ MAX_STACK ] = max_ticks[ INACTIVE ] = driver()->effectN( 3 ).base_value();

    vigor_buff = ::create_buff<stat_buff_t>( effect.player, "vigor_engaged",
        effect.player->find_spell( 287916 ), effect.item );
    vigor_buff->add_stat( STAT_CRIT_RATING, driver()->effectN( 1 ).average( effect.item ) );

    cooldown_buff = ::create_buff<buff_t>( effect.player, "vigor_cooldown",
        effect.player->find_spell( 287967 ), effect.item );

    // Overload
    overload_spell = effect.player->find_spell( 287917 );
    overload_cd = effect.player->get_cooldown( "oscillating_overload_" + ::util::to_string( overload_spell->id() ) );

    effect.player->callbacks_on_arise.push_back( [ this ]() {
      reset_oscillation();

      if ( !player->sim->bfa_opts.randomize_oscillation )
      {
        do_oscillation_transition();
      }
      // If we are randomizing the initial oscillation, don't do any oscillation transition on arise
      else
      {
        randomize_oscillation();
      }
    } );
  }

  void extend_oscillation( const timespan_t& by_seconds )
  {
    if ( player->sim->debug )
    {
      player->sim->out_debug.print( "{} {} oscillation overload, remains={}, new_remains={}",
        player->name(), name(), tick_event->remains(),
        tick_event->remains() + by_seconds );
    }

    tick_event->reschedule( tick_event->remains() + by_seconds );
  }

  // Perform automatic overload at MAX_STACK transition
  void auto_overload( oscillation new_state )
  {
    if ( !player->sim->bfa_opts.auto_oscillating_overload )
    {
      return;
    }

    if ( new_state != oscillation::MAX_STACK )
    {
      return;
    }

    if ( !overload_cd->up() )
    {
      return;
    }

    // The overload buff is going to be created by the special effect code, so find and cache
    // the pointer here to avoid init issues
    if ( !overload_buff )
    {
      overload_buff = buff_t::find( player, "oscillating_overload" );
      assert( overload_buff );
    }

    // Put on-use effects on cooldown
    overload_cd->start( overload_spell->cooldown() );
    overload_buff->trigger();
  }

  void oscillate()
  {
    ticks_at_oscillation++;
    do_oscillation_transition();
  }

  void transition_to( oscillation new_oscillation )
  {
    if ( player->sim->debug )
    {
      player->sim->out_debug.print( "{} {} transition from {} to {}",
        player->name(), name(), oscillation_str( current_oscillation ),
        oscillation_str( new_oscillation ) );
    }

    current_oscillation = new_oscillation;
    ticks_at_oscillation = 0u;
  }

  void do_oscillation_transition()
  {
    if ( player->sim->debug )
    {
      player->sim->out_debug.print( "{} {} oscillation at {}, ticks={}, max_ticks={}",
        player->name(), name(), oscillation_str( current_oscillation ), ticks_at_oscillation,
        max_ticks[ current_oscillation ] );
    }

    auto at_max_stack = ticks_at_oscillation == max_ticks[ current_oscillation ];
    switch ( current_oscillation )
    {
      case oscillation::ASCENDING:
      case oscillation::DESCENDING:
        vigor_buff->trigger();
        if ( !vigor_buff->check() )
        {
          cooldown_buff->trigger();
        }
        break;
      case oscillation::INACTIVE:
      case oscillation::MAX_STACK:
        if ( at_max_stack )
        {
          vigor_buff->reverse = !vigor_buff->reverse;
          vigor_buff->trigger();
        }
        break;
      default:
        break;
    }

    tick_event = make_event( player->sim, driver()->effectN( 1 ).period(),
      [ this ]() { oscillate(); } );

    if ( at_max_stack )
    {
      auto_overload( transition_map[ current_oscillation ] );
      transition_to( transition_map[ current_oscillation ] );
    }
  }

  // Randomize oscillation state to any of the four states, and any time inside the phase
  void randomize_oscillation()
  {
    oscillation phase = static_cast<oscillation>( static_cast<unsigned>( player->rng().range(
        oscillation::ASCENDING, oscillation::MAX_STATES ) ) );
    double used_time = player->rng().range( 0, max_ticks[ phase ] *
        driver()->effectN( 1 ).period().total_seconds() );
    int ticks = used_time / driver()->effectN( 1 ).period().total_seconds();
    double elapsed_tick_time = used_time - ticks * driver()->effectN( 1 ).period().total_seconds();
    double time_to_next_tick = driver()->effectN( 1 ).period().total_seconds() - elapsed_tick_time;

    if ( player->sim->debug )
    {
      player->sim->out_debug.print( "{} {} randomized oscillation, oscillation={}, used_time={}, "
                                    "ticks={}, next_tick_in={}, phase_left={}",
        player->name(), name(), oscillation_str( phase ), used_time, ticks, time_to_next_tick,
        max_ticks[ phase ] * driver()->effectN( 1 ).period() - timespan_t::from_seconds( used_time ) );
    }

    switch ( phase )
    {
      case oscillation::ASCENDING:
        vigor_buff->trigger( 1 + ticks );
        break;
      case oscillation::DESCENDING:
        vigor_buff->trigger( vigor_buff->max_stack() - ( 1 + ticks ) );
        vigor_buff->reverse = true;
        break;
      case oscillation::MAX_STACK:
        vigor_buff->trigger( vigor_buff->max_stack() );
        break;
      case oscillation::INACTIVE:
        vigor_buff->reverse = true;
        cooldown_buff->trigger( 1, buff_t::DEFAULT_VALUE(), -1.0,
          max_ticks[ phase ] * driver()->effectN( 1 ).period() - timespan_t::from_seconds( used_time ) );
        break;
      default:
        break;
    }

    current_oscillation = phase;
    ticks_at_oscillation = as<unsigned>( ticks );
    tick_event = make_event( player->sim, timespan_t::from_seconds( time_to_next_tick ),
      [ this ]() { oscillate(); } );
  }

  void reset_oscillation()
  {
    transition_to( oscillation::ASCENDING );
    vigor_buff->reverse = false;
  }
};

void items::variable_intensity_gigavolt_oscillating_reactor( special_effect_t& e )
{
  // Ensure the sim don't do stupid things if someone thinks it wise to try initializing 2 of these
  // trinkets on a single actor
  auto it = range::find_if( e.player->special_effects, [&e]( const special_effect_t* effect ) {
    return e.spell_id == effect->spell_id;
  } );

  if ( it == e.player->special_effects.end() )
  {
    e.player->special_effects.push_back( new vigor_engaged_t( e ) );
    e.type = SPECIAL_EFFECT_NONE;
  }
}

void items::variable_intensity_gigavolt_oscillating_reactor_onuse( special_effect_t& effect )
{
  struct oscillating_overload_t : public buff_t
  {
    vigor_engaged_t* driver;

    oscillating_overload_t( player_t* p, const std::string& name, const spell_data_t* spell, const item_t* item ) :
      buff_t( p, name, spell, item ), driver( nullptr )
    { }

    void execute( int stacks, double value, timespan_t duration ) override
    {
      buff_t::execute( stacks, value, duration );

      // Find the special effect for the on-equip on first execute of the on-use effect. Otherwise
      // we run into init issues.
      if ( !driver )
      {
        auto it = range::find_if( player->special_effects, []( special_effect_t* effect ) {
          return effect->spell_id == 287915;
        } );

        assert( it != player->special_effects.end() );
        driver = static_cast<vigor_engaged_t*>( *it );
      }

      driver->extend_oscillation( data().duration() );
    }
  };

  // Implement an additional level of indirection so we can control when the overload use-item can
  // be triggered. This is necessary for a few reasons:
  // 1) The on-use will not work if the actor is cooling down (has the Vigor Cooldown buff)
  // 2) The on-use can not be used, if bfa.auto_oscillating_overload option is used
  struct oscillating_overload_action_t : public action_t
  {
    oscillating_overload_t* buff;
    buff_t* cooldown_buff;

    oscillating_overload_action_t( const special_effect_t& effect ) :
      action_t( ACTION_OTHER, "oscillating_overload_driver", effect.player, effect.driver() ),
      buff( create_buff<oscillating_overload_t>( effect.player, "oscillating_overload",
            effect.driver(), effect.item ) ),
      cooldown_buff( create_buff<buff_t>( effect.player, "vigor_cooldown",
        effect.player->find_spell( 287967 ), effect.item ) )
    {
      background = quiet = true;
      callbacks = false;
    }

    result_e calculate_result( action_state_t* /* state */ ) const override
    { return RESULT_HIT; }

    void execute() override
    {
      action_t::execute();

      buff->trigger();
    }

    bool ready() override
    {
      if ( sim->bfa_opts.auto_oscillating_overload )
      {
        return false;
      }

      // Overload on-use is not available if the cooldown buff is up
      if ( cooldown_buff->check() )
      {
        return false;
      }

      return action_t::ready();
    }
  };

  effect.execute_action = new oscillating_overload_action_t( effect );
}

// Everchill Anchor

struct everchill_anchor_constructor_t : public item_targetdata_initializer_t
{
  everchill_anchor_constructor_t( unsigned iid, const std::vector< slot_e >& s ) :
    item_targetdata_initializer_t( iid, s )
  {}

  // Create the everchill debuff to handle trinket icd
  void operator()( actor_target_data_t* td ) const override
  {
    const special_effect_t* effect = find_effect( td -> source );
    if ( !effect )
    {
      td -> debuff.everchill = make_buff( *td, "everchill" );
      return;
    }
    assert( !td -> debuff.everchill );

    td -> debuff.everchill =
      make_buff( *td, "everchill_debuff", effect -> trigger() )
      -> set_activated( false );
    td -> debuff.everchill -> reset();
  }
};

void items::everchill_anchor( special_effect_t& effect )
{
  struct everchill_t : public proc_spell_t
  {
    everchill_t( const special_effect_t& effect ):
      proc_spell_t( "everchill", effect.player, effect.player -> find_spell( 289526 ), effect.item )
    { }
  };

  struct everchill_anchor_cb_t : public dbc_proc_callback_t
  {
    everchill_t* damage;
    everchill_anchor_cb_t( const special_effect_t& effect ) :
      dbc_proc_callback_t( effect.item, effect ),
      damage( new everchill_t( effect ) )
    {}

    void execute( action_t* /* a */, action_state_t* state ) override
    {
      actor_target_data_t* td = listener -> get_target_data( state -> target );
      assert( td );

      if ( td -> debuff.everchill -> trigger() )
      {
        // The dot doesn't tick when it's refreshed but ticks when it's applied
        // So we set tick_zero to false if it's a refresh, execute the dot, then set it back to true
        // Hacky but at least it works
        dot_t* current_dot = damage -> find_dot( state -> target );
        bool already_ticking = current_dot ? current_dot -> is_ticking() : false;
        if ( already_ticking )
        {
          damage -> tick_zero = false;
        }
        damage -> set_target( state -> target );
        damage -> execute();
        if ( already_ticking )
        {
          damage -> tick_zero = true;
        }
      }
    }
  };

  // An aoe ability will apply the dot on every target hit
  effect.proc_flags2_ = PF2_ALL_HIT;
  // Internal cd is handled on the debuff
  effect.cooldown_ = timespan_t::zero();

  new everchill_anchor_cb_t( effect );
}

// Ramping Amplitude Gigavolt Engine ======================================

void items::ramping_amplitude_gigavolt_engine( special_effect_t& effect )
{
  struct ramping_amplitude_event_t : event_t
  {
    buff_t* rage_buff;
    player_t* p;
    timespan_t prev_interval;

    ramping_amplitude_event_t( player_t* player, buff_t* buff, timespan_t interval ) :
      event_t( *player, interval ), rage_buff( buff ), p( player ), prev_interval( interval )
    { }

    void execute() override
    {
      if ( rage_buff -> check() && rage_buff -> check() < rage_buff -> max_stack() )
      {
        rage_buff -> increment();
        make_event<ramping_amplitude_event_t>( sim(), p, rage_buff, prev_interval * 0.8 );
      }
    }
  };

  struct ramping_amplitude_gigavolt_engine_active_t : proc_spell_t
  {
    buff_t* rage_buff;

    ramping_amplitude_gigavolt_engine_active_t( special_effect_t& effect ) :
      proc_spell_t( "ramping_amplitude_gigavolt_engine_active", effect.player, effect.driver(), effect.item )
    {
      rage_buff = make_buff<stat_buff_t>( effect.player, "r_a_g_e", effect.player -> find_spell( 288156 ), effect.item );
      rage_buff -> set_refresh_behavior( buff_refresh_behavior::DISABLED );
      rage_buff -> cooldown = this -> cooldown;
    }

    void execute() override
    {
      rage_buff -> trigger();
      make_event<ramping_amplitude_event_t>( *sim, player, rage_buff, timespan_t::from_seconds( 2.0 ) );
    }
  };

  effect.disable_buff();
  
  effect.execute_action = new ramping_amplitude_gigavolt_engine_active_t( effect );
}

// Grong's Primal Rage 

void items::grongs_primal_rage( special_effect_t& effect )
{
  struct primal_rage_channel : public proc_spell_t
  {
    primal_rage_channel( const special_effect_t& effect ) :
      proc_spell_t( "primal_rage", effect.player, effect.player -> find_spell( 288267 ), effect.item )
    { 
      channeled = hasted_ticks = true;
      interrupt_auto_attack = tick_zero = false;

      tick_action = create_proc_action<aoe_proc_t>( "primal_rage_damage", effect,
                                               "primal_rage_damage", effect.player -> find_spell( 288269 ), true );
    }

    // Even though ticks are hasted, the duration is a fixed 4s
    timespan_t composite_dot_duration( const action_state_t* state ) const override
    {
      return dot_duration;
    }

    // Copied from draught of souls
    void execute() override
    {
      proc_spell_t::execute();

      // Use_item_t (that executes this action) will trigger a player-ready event after execution.
      // Since this action is a "background channel", we'll need to cancel the player ready event to
      // prevent the player from picking something to do while channeling.
      event_t::cancel( player -> readying );
    }

    void last_tick( dot_t* d ) override
    {
      // Last_tick() will zero player_t::channeling if this action is being channeled, so check it
      // before calling the parent.
      auto was_channeling = player -> channeling == this;

      proc_spell_t::last_tick( d );

      // Since Draught of Souls must be modeled as a channel (player cannot be allowed to perform
      // any actions for 3 seconds), we need to manually restart the player-ready event immediately
      // after the channel ends. This is because the channel is flagged as a background action,
      // which by default prohibits player-ready generation.
      if ( was_channeling && player -> readying == nullptr )
      {
        // Due to the client not allowing the ability queue here, we have to wait
        // the amount of lag + how often the key is spammed until the next ability is used.
        // Modeling this as 2 * lag for now. Might increase to 3 * lag after looking at logs of people using the trinket.
        timespan_t time = ( player -> world_lag_override ? player -> world_lag : sim -> world_lag ) * 2.0;
        player -> schedule_ready( time );
      }
    }
  };

  effect.execute_action = new primal_rage_channel( effect );
}

// Waycrest's Legacy Set Bonus ============================================

void set_bonus::waycrest_legacy( special_effect_t& effect )
{
  auto effect_heal = unique_gear::find_special_effect( effect.player, 271631, SPECIAL_EFFECT_EQUIP );
  if ( effect_heal != nullptr )
  {
    effect_heal -> execute_action->add_child( new waycrest_legacy_heal_t( effect ) );
  }

  auto effect_damage = unique_gear::find_special_effect( effect.player, 271683, SPECIAL_EFFECT_EQUIP );
  if ( effect_damage != nullptr )
  {
    effect_damage -> execute_action->add_child( new waycrest_legacy_damage_t( effect ) );
  }
}

// Gift of the Loa Set Bonus ==============================================

void set_bonus::gift_of_the_loa( special_effect_t& effect )
{
  auto p = effect.player;

  // Set bonus is only active in Zuldazar
  if ( p->sim->bfa_opts.zuldazar )
  {
    // The values are stored in a different spell.
    double value = p->find_spell( 290264 )->effectN( 1 ).average( p );
    p->passive.add_stat( p->convert_hybrid_stat( STAT_STR_AGI_INT ), value );
  }
}

// Keepsakes of the Resolute Commandant Set Bonus =========================

void set_bonus::keepsakes_of_the_resolute_commandant( special_effect_t& effect )
{
  new dbc_proc_callback_t( effect.player, effect );
}

} // namespace bfa
} // anon namespace

void unique_gear::register_special_effects_bfa()
{
  using namespace bfa;

  // Consumables
  register_special_effect( 259409, consumables::galley_banquet );
  register_special_effect( 259410, consumables::bountiful_captains_feast );
  register_special_effect( 290464, consumables::boralus_blood_sausage );
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
  register_special_effect( 281543, items::bygone_bee_almanac );
  register_special_effect( 281547, items::leyshocks_grand_compilation );
  register_special_effect( 266047, items::jes_howler );
  register_special_effect( 278364, "Reverse" );
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
  register_special_effect( 271683, items::lady_waycrests_music_box_heal );
  register_special_effect( 268999, items::balefire_branch );
  register_special_effect( 268314, "268311Trigger" ); // Galecaller's Boon, assumes the player always stands in the area
  register_special_effect( 278140, items::frenetic_corpuscle );
  register_special_effect( 278383, "Reverse" ); // Azurethos' Singed Plumage
  register_special_effect( 278154, items::twitching_tentacle_of_xalzaix );
  register_special_effect( 278161, items::vanquished_tendril_of_ghuun );
  register_special_effect( 268828, items::vial_of_animated_blood );
  register_special_effect( 278267, items::sandscoured_idol );
  register_special_effect( 278109, items::syringe_of_bloodborne_infirmity );
  register_special_effect( 278112, items::syringe_of_bloodborne_infirmity );
  register_special_effect( 278152, items::disc_of_systematic_regression );
  register_special_effect( 274472, items::berserkers_juju );
  register_special_effect( 285495, "285496Trigger" ); // Moonstone of Zin-Azshari
  register_special_effect( 289522, items::incandescent_sliver );
  register_special_effect( 289885, items::tidestorm_codex );
  register_special_effect( 289521, items::invocation_of_yulon );
  register_special_effect( 288328, "288330Trigger" ); // Kimbul's Razor Claw
  register_special_effect( 287915, items::variable_intensity_gigavolt_oscillating_reactor );
  register_special_effect( 287917, items::variable_intensity_gigavolt_oscillating_reactor_onuse );
  register_special_effect( 289525, items::everchill_anchor );
  register_special_effect( 288173, items::ramping_amplitude_gigavolt_engine );
  register_special_effect( 289520, items::grongs_primal_rage );

  // Misc
  register_special_effect( 276123, items::darkmoon_deck_squalls );
  register_special_effect( 276176, items::darkmoon_deck_fathoms );
  register_special_effect( 265440, items::endless_tincture_of_fractional_power );

  /* 8.0 Dungeon Set Bonuses*/
  register_special_effect( 277522, set_bonus::waycrest_legacy );

  /* 8.1.0 Raid Set Bonuses */
  register_special_effect( 290263, set_bonus::gift_of_the_loa );
  register_special_effect( 290362, set_bonus::keepsakes_of_the_resolute_commandant );
}

void unique_gear::register_target_data_initializers_bfa( sim_t* sim )
{
  using namespace bfa;
  const std::vector<slot_e> items = { SLOT_TRINKET_1, SLOT_TRINKET_2 };

  sim -> register_target_data_initializer( deadeye_spyglass_constructor_t( 159623, items ) );
  sim -> register_target_data_initializer( syringe_of_bloodborne_infirmity_constructor_t( 160655, items ) );
  sim -> register_target_data_initializer( everchill_anchor_constructor_t( 165570, items ) );
}

namespace expansion
{
namespace bfa
{
static std::unordered_map<unsigned, stat_e> __ls_cb_map { {

} };

void register_leyshocks_trigger( unsigned spell_id, stat_e stat_buff )
{
  __ls_cb_map[ spell_id ] = stat_buff;
}

void trigger_leyshocks_grand_compilation( unsigned spell_id, player_t* actor )
{
  if ( !actor || !actor->buffs.leyshock_crit )
  {
    return;
  }

  auto it = __ls_cb_map.find( spell_id );
  if ( it == __ls_cb_map.end() )
  {
    return;
  }

  trigger_leyshocks_grand_compilation( it->second, actor );
}

void trigger_leyshocks_grand_compilation( stat_e stat, player_t* actor )
{
  if ( !actor || !actor->buffs.leyshock_crit )
  {
    return;
  }

  switch ( stat )
  {
    case STAT_CRIT_RATING:
      actor->buffs.leyshock_crit->trigger();
      break;
    case STAT_HASTE_RATING:
      actor->buffs.leyshock_haste->trigger();
      break;
    case STAT_MASTERY_RATING:
      actor->buffs.leyshock_mastery->trigger();
      break;
    case STAT_VERSATILITY_RATING:
      actor->buffs.leyshock_versa->trigger();
      break;
    default:
      break;
  }
}

} // Namespace bfa ends
} // Namespace expansion ends
