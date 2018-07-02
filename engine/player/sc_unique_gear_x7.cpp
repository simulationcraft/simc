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
  custom_cb_t weapon_navigation( unsigned );
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
      make_buff<haste_buff_t>( effect.player, util::tokenized_name( spell ), spell )
        -> add_invalidate( CACHE_ATTACK_SPEED )
        -> set_default_value( spell -> effectN( 1 ).percent() )
        -> set_activated( false );
    effect.player -> buffs.galeforce_striking = buff;
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
    {}

    void trigger( action_t* a, void* call_data ) override
    {
      // from logs it seems like the stacking buff
      // can't trigger while the 'final' one is up
      // XXX: check how it interacts with rppm (maybe it should go into execute)
      if ( ! final_buff -> check() )
        dbc_proc_callback_t::trigger( a, call_data );
    }

    void execute( action_t*, action_state_t* ) override
    {
      if ( proc_buff && proc_buff -> trigger() &&
           proc_buff -> check() == proc_buff -> max_stack() )
      {
        final_buff -> trigger();
        proc_buff -> expire();
      }
    }
  };

  return [ buff_id ] ( special_effect_t& effect ) {
    // get (or create) the 'final' buff
    // the stacking proc buff will be created automagically
    auto spell_data = effect.player -> find_spell( buff_id );
    const std::string spell_name = util::tokenized_name( spell_data ) + "_final";
    buff_t* buff = buff_t::find( effect.player, spell_name );
    if ( !buff )
      buff = make_buff<stat_buff_t>( effect.player, spell_name, spell_data );

    new navigation_proc_callback_t( effect.player, effect, buff );
  };
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
  register_special_effect( 268855, enchants::weapon_navigation( 268856 ) ); // Versatile Navigation
  register_special_effect( 268888, enchants::weapon_navigation( 268893 ) ); // Quick Navigation
  register_special_effect( 268900, enchants::weapon_navigation( 268898 ) ); // Masterful Navigation
  register_special_effect( 268906, enchants::weapon_navigation( 268904 ) ); // Deadly Navigation
  register_special_effect( 268912, enchants::weapon_navigation( 268910 ) ); // Stalwart Navigation
}
