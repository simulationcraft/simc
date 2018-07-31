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
  effect.stat = effect.player -> convert_hybrid_stat( STAT_STR_AGI_INT );
  switch ( effect.stat ) {
    case STAT_STRENGTH:
      effect.trigger_spell_id = 259456;
      break;
    case STAT_AGILITY:
      effect.trigger_spell_id = 259454;
      break;
    case STAT_INTELLECT:
      effect.trigger_spell_id = 259455;
      break;
    default:
      break;
  }

  if (effect.player -> role == ROLE_TANK && !effect.player->sim->bfa_opts.bountiful_feast_as_dps )
  {
    effect.stat = STAT_STAMINA;
    effect.trigger_spell_id = 259457;
  }

  effect.stat_amount = effect.player -> find_spell( effect.trigger_spell_id ) -> effectN( 1 ).average( effect.player );
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
  register_special_effect( 278140, trinkets::frenetic_corpuscle );
}
