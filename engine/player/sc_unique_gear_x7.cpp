// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

using namespace unique_gear;

namespace {
namespace bfa { // YaN - Yet another namespace - to resolve conflicts with global namespaces.
/**
 * Forward declarations so we can reorganize the file a bit more sanely.
 */

namespace consumables
{
  void galley_banquet( special_effect_t& );
  void bountiful_captains_feast( special_effect_t& );
}

namespace enchants
{
  void galeforce_striking( special_effect_t& );
  void torrent_of_elements( special_effect_t& );
  custom_cb_t weapon_navigation( unsigned );
}

namespace trinkets
{
  // 8.0.1 - Dungeon Trinkets
  void deadeye_spyglass( special_effect_t& );
  void tiny_electromental_in_a_jar( special_effect_t& );
  void mydas_talisman( special_effect_t& );
  // 8.0.1 - Uldir Trinkets
  void frenetic_corpuscle( special_effect_t& );
}

namespace util
{
// feasts initialization helper
void init_feast( special_effect_t& effect, arv::array_view<std::pair<stat_e, int>> stat_map )
{
  effect.stat = effect.player -> convert_hybrid_stat( STAT_STR_AGI_INT );
  // TODO: Is this actually spec specific?
  if ( effect.player -> role == ROLE_TANK )
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

void trinkets::deadeye_spyglass( special_effect_t& effect )
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

void trinkets::tiny_electromental_in_a_jar( special_effect_t& effect )
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

void trinkets::mydas_talisman( special_effect_t& effect )
{
  struct touch_of_gold_cb_t : public dbc_proc_callback_t
  {
    buff_t* buff = nullptr;

    touch_of_gold_cb_t( const special_effect_t& effect ) :
      dbc_proc_callback_t( effect.item, effect )
    {}

    void execute( action_t*, action_state_t* s ) override
    {
      assert( proc_action );

      proc_action -> set_target( s -> target );
      proc_action -> execute();

      // in-game the buff has 5 stacks initially and each damage proc consumes a stack
      // as there is no generic way to do this in simc we simply "reverse" it
      if ( buff -> check() == buff -> max_stack() )
        buff -> expire();
      else
        buff -> bump( 1 );
    }
  };

  auto effect2 = new special_effect_t( effect.item );
  effect2 -> name_str = "touch_of_gold";
  effect2 -> source = effect.source;
  effect2 -> proc_flags_ = effect.proc_flags();
  effect2 -> proc_chance_ = effect.proc_chance();
  effect2 -> cooldown_ = timespan_t::zero();
  effect2 -> trigger_spell_id = 265953;
  effect.player -> special_effects.push_back( effect2 );

  auto callback = new touch_of_gold_cb_t( *effect2 );
  callback -> deactivate();

  effect.custom_buff = buff_t::find( effect.player, "touch_of_gold" );
  if ( ! effect.custom_buff )
  {
    effect.custom_buff = make_buff( effect.player, "touch_of_gold", effect.driver() )
      -> set_stack_change_callback( util::callback_buff_activator( callback ) )
      -> set_refresh_behavior( buff_refresh_behavior::DISABLED );
  }
  // reset triggered spell; we don't want to trigger a spell on use
  effect.trigger_spell_id = 0;

  callback -> buff = effect.custom_buff;
}

// Frenetic Corpuscle =======================================================

void trinkets::frenetic_corpuscle( special_effect_t& effect )
{
  struct frenetic_corpuscle_cb_t : public dbc_proc_callback_t
  {
    struct frenetic_corpuscle_damage_t : public proc_spell_t
    {
      frenetic_corpuscle_damage_t( const special_effect_t& effect ) :
        proc_spell_t( "frenetic_blow", effect.player, effect.player -> find_spell( 278148 ), effect.item )
      {}
    };

    action_t* damage;

    frenetic_corpuscle_cb_t( const special_effect_t& effect ) :
      dbc_proc_callback_t( effect.item, effect ),
      damage( create_proc_action<frenetic_corpuscle_damage_t>( "frenetic_blow", effect ) )
    {}

    void execute( action_t* /* a */, action_state_t* state ) override
    {
      proc_buff->trigger();
      if ( proc_buff->check() == proc_buff->max_stack() )
      {
        // TODO: Tooltip says "on next attack", which likely uses Frenetic Frenzy buff (278144) to proc trigger the damage
        // Just immediately trigger the damage here, can revisit later and implement a new special_effect callback if needed
        damage->set_target( state->target );
        damage->execute();
        proc_buff->expire();
      }
    }
  };

  buff_t* buff = buff_t::find( effect.player, "frothing_rage" );
  if ( !buff )
  {
    buff = make_buff( effect.player, "frothing_rage", effect.player->find_spell( 278143 ), effect.item );
  }

  effect.custom_buff = buff;
  new frenetic_corpuscle_cb_t( effect );
}


} // namespace bfa
} // anon namespace

void unique_gear::register_special_effects_bfa()
{
  using namespace bfa;

  // Consumables
  register_special_effect( 259409, consumables::galley_banquet );
  register_special_effect( 259410, consumables::bountiful_captains_feast );

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
  register_special_effect( 268758, trinkets::deadeye_spyglass );
  register_special_effect( 268771, trinkets::deadeye_spyglass );
  register_special_effect( 267177, trinkets::tiny_electromental_in_a_jar );
  register_special_effect( 265954, trinkets::mydas_talisman );
  register_special_effect( 278140, trinkets::frenetic_corpuscle );
}

void unique_gear::register_target_data_initializers_bfa( sim_t* sim )
{
  using namespace bfa;
  const std::vector<slot_e> trinkets = { SLOT_TRINKET_1, SLOT_TRINKET_2 };

  sim -> register_target_data_initializer( deadeye_spyglass_constructor_t( 159623, trinkets ) );
}
