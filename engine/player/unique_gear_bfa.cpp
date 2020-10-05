// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

#include "pet_spawner.hpp"
#include "darkmoon_deck.hpp"
#include "util/static_map.hpp"

using namespace unique_gear;

namespace
{
namespace bfa
{  // YaN - Yet another namespace - to resolve conflicts with global namespaces.

template <typename BASE = proc_spell_t>
struct base_bfa_proc_t : public BASE
{
  base_bfa_proc_t( const special_effect_t& effect, ::util::string_view name, unsigned spell_id )
    : BASE( name, effect.player, effect.player->find_spell( spell_id ), effect.item )
  {
  }

  base_bfa_proc_t( const special_effect_t& effect, ::util::string_view name, const spell_data_t* s )
    : BASE( name, effect.player, s, effect.item )
  {
  }
};

template <typename BASE = proc_spell_t>
struct base_bfa_aoe_proc_t : public base_bfa_proc_t<BASE>
{
  bool aoe_damage_increase;

  base_bfa_aoe_proc_t( const special_effect_t& effect, ::util::string_view name, unsigned spell_id,
                       bool aoe_damage_increase_ = false )
    : base_bfa_proc_t<BASE>( effect, name, spell_id ), aoe_damage_increase( aoe_damage_increase_ )
  {
    this->aoe              = -1;
    this->split_aoe_damage = true;
  }

  base_bfa_aoe_proc_t( const special_effect_t& effect, ::util::string_view name, const spell_data_t* s,
                       bool aoe_damage_increase_ = false )
    : base_bfa_proc_t<BASE>( effect, name, s ), aoe_damage_increase( aoe_damage_increase_ )
  {
    this->aoe              = -1;
    this->split_aoe_damage = true;
  }

  double action_multiplier() const override
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
using proc_t     = base_bfa_proc_t<proc_spell_t>;

/**
 * Forward declarations so we can reorganize the file a bit more sanely.
 */

namespace consumables
{
void galley_banquet( special_effect_t& );
void bountiful_captains_feast( special_effect_t& );
void famine_evaluator_and_snack_table( special_effect_t& );
void boralus_blood_sausage( special_effect_t& );
void potion_of_rising_death( special_effect_t& );
void potion_of_bursting_blood( special_effect_t& );
void potion_of_unbridled_fury( special_effect_t& );
void potion_of_focused_resolve( special_effect_t& );
void potion_of_empowered_proximity( special_effect_t& );
}  // namespace consumables

namespace enchants
{
void galeforce_striking( special_effect_t& );
void torrent_of_elements( special_effect_t& );
custom_cb_t weapon_navigation( unsigned );

/* 8.2 */
void machinists_brilliance( special_effect_t& );
void force_multiplier( special_effect_t& );
}  // namespace enchants

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
void briny_barnacle( special_effect_t& );
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
// 8.1.5 - Crucible of Storms Trinkets and Special Items
void harbingers_inscrutable_will( special_effect_t& );
void leggings_of_the_aberrant_tidesage( special_effect_t& );
void fathuuls_floodguards( special_effect_t& );
void grips_of_forsaken_sanity( special_effect_t& );
void stormglide_steps( special_effect_t& );
void idol_of_indiscriminate_consumption( special_effect_t& );
void lurkers_insidious_gift( special_effect_t& );
void abyssal_speakers_gauntlets( special_effect_t& );
void trident_of_deep_ocean( special_effect_t& );
void legplates_of_unbound_anguish( special_effect_t& );
// 8.2.0 - Rise of Azshara Trinkets and Special Items
void damage_to_aberrations( special_effect_t& );
void exploding_pufferfish( special_effect_t& );
void fathom_hunter( special_effect_t& );
void nazjatar_proc_check( special_effect_t& );
void storm_of_the_eternal_arcane_damage( special_effect_t& );
void storm_of_the_eternal_stats( special_effect_t& );
void highborne_compendium_of_sundering( special_effect_t& );
void highborne_compendium_of_storms( special_effect_t& );
void neural_synapse_enhancer( special_effect_t& );
void shiver_venom_relic_onuse( special_effect_t& );
void shiver_venom_relic_equip( special_effect_t& );
void leviathans_lure( special_effect_t& );
void aquipotent_nautilus( special_effect_t& );
void zaquls_portal_key( special_effect_t& );
void vision_of_demise( special_effect_t& );
void azsharas_font_of_power( special_effect_t& );
void arcane_tempest( special_effect_t& );
void anuazshara_staff_of_the_eternal( special_effect_t& );
void shiver_venom_crossbow( special_effect_t& );
void shiver_venom_lance( special_effect_t& );
void ashvanes_razor_coral( special_effect_t& );
void dribbling_inkpod( special_effect_t& );
void reclaimed_shock_coil( special_effect_t& );
void dreams_end( special_effect_t& );
void divers_folly( special_effect_t& );
void remote_guidance_device( special_effect_t& );
void gladiators_maledict( special_effect_t& );
void getiikku_cut_of_death( special_effect_t& );
void bilestained_crawg_tusks( special_effect_t& );
// 8.2.0 - Rise of Azshara Punchcards
void yellow_punchcard( special_effect_t& );
void subroutine_overclock( special_effect_t& );
void subroutine_recalibration( special_effect_t& );
void subroutine_optimization( special_effect_t& );
void harmonic_dematerializer( special_effect_t& );
void cyclotronic_blast( special_effect_t& );
// 8.2.0 - Mechagon combo rings
void logic_loop_of_division( special_effect_t& );
void logic_loop_of_recursion( special_effect_t& );
void logic_loop_of_maintenance( special_effect_t& );
void overclocking_bit_band( special_effect_t& );
void shorting_bit_band( special_effect_t& );
// 8.2.0 - Mechagon trinkets and special items
void hyperthread_wristwraps( special_effect_t& );
void anodized_deflectors( special_effect_t& );
// 8.3.0 - Visions of N'Zoth Trinkets and Special Items
void voidtwisted_titanshard( special_effect_t& );
void vitacharged_titanshard( special_effect_t& );
void manifesto_of_madness( special_effect_t& );
void whispering_eldritch_bow( special_effect_t& );
void psyche_shredder( special_effect_t& );
void torment_in_a_jar( special_effect_t& );
void draconic_empowerment( special_effect_t& );
void writhing_segment_of_drestagath( special_effect_t& );
}  // namespace items

namespace util
{
// feasts initialization helper
void init_feast( special_effect_t& effect, std::initializer_list<std::pair<stat_e, int>> stat_map )
{
  effect.stat = effect.player->convert_hybrid_stat( STAT_STR_AGI_INT );
  // TODO: Is this actually spec specific?
  if ( effect.player->role == ROLE_TANK && !effect.player->sim->feast_as_dps )
    effect.stat = STAT_STAMINA;

  for ( auto&& stat : stat_map )
  {
    if ( stat.first == effect.stat )
    {
      effect.trigger_spell_id = stat.second;
      break;
    }
  }
  effect.stat_amount = effect.player->find_spell( effect.trigger_spell_id )->effectN( 1 ).average( effect.player );
}

std::string tokenized_name( const spell_data_t* spell )
{
  return ::util::tokenize_fn( spell->name_cstr() );
}

buff_stack_change_callback_t callback_buff_activator( dbc_proc_callback_t* callback )
{
  return [callback]( buff_t*, int old, int new_ ) {
    if ( old == 0 )
      callback->activate();
    else if ( new_ == 0 )
      callback->deactivate();
  };
}

bool is_adjustable_class_spell( action_t* a )
{
  return a->data().class_mask() != 0 && !a->background && a->cooldown_duration() > 0_ms && a->data().race_mask() == 0;
}

}  // namespace util

namespace set_bonus
{
// 8.0 Dungeon
void waycrest_legacy( special_effect_t& );
// 8.1.0 Raid
void gift_of_the_loa( special_effect_t& );
void keepsakes_of_the_resolute_commandant( special_effect_t& );
// 8.3.0 Raid
void titanic_empowerment( special_effect_t& );
}  // namespace set_bonus

// Galley Banquet ===========================================================

void consumables::galley_banquet( special_effect_t& effect )
{
  util::init_feast(
      effect, {{STAT_STRENGTH, 259452}, {STAT_AGILITY, 259448}, {STAT_INTELLECT, 259449}, {STAT_STAMINA, 259453}} );
}

// Bountiful Captain's Feast ================================================

void consumables::bountiful_captains_feast( special_effect_t& effect )
{
  util::init_feast(
      effect, {{STAT_STRENGTH, 259456}, {STAT_AGILITY, 259454}, {STAT_INTELLECT, 259455}, {STAT_STAMINA, 259457}} );
}

// Famine Evaluator and Snack Table ================================================

void consumables::famine_evaluator_and_snack_table( special_effect_t& effect )
{
  util::init_feast(
      effect, {{STAT_STRENGTH, 297118}, {STAT_AGILITY, 297116}, {STAT_INTELLECT, 297117}, {STAT_STAMINA, 297119}} );
}

// Boralus Blood Sausage ================================================

void consumables::boralus_blood_sausage( special_effect_t& effect )
{
  util::init_feast( effect, {{STAT_STRENGTH, 290469}, {STAT_AGILITY, 290467}, {STAT_INTELLECT, 290468}} );
}

// Potion of Rising Death ===================================================

void consumables::potion_of_rising_death( special_effect_t& effect )
{
  effect.disable_action();

  // Make a bog standard damage proc for the buff
  auto secondary            = new special_effect_t( effect.player );
  secondary->type           = SPECIAL_EFFECT_EQUIP;
  secondary->spell_id       = effect.spell_id;
  secondary->cooldown_      = timespan_t::zero();
  secondary->execute_action = create_proc_action<proc_spell_t>( "potion_of_rising_death", effect );
  effect.player->special_effects.push_back( secondary );

  auto proc = new dbc_proc_callback_t( effect.player, *secondary );
  proc->deactivate();
  proc->initialize();

  effect.custom_buff = make_buff( effect.player, effect.name(), effect.driver() )
                           ->set_stack_change_callback( [proc]( buff_t*, int, int new_ ) {
                             if ( new_ == 1 )
                               proc->activate();
                             else
                               proc->deactivate();
                           } )
                           ->set_cooldown( timespan_t::zero() )  // Handled by the action
                           ->set_chance( 1.0 );                  // Override chance so the buff actually triggers
}

// Potion of Bursting Blood =================================================

void consumables::potion_of_bursting_blood( special_effect_t& effect )
{
  effect.disable_action();

  // Make a bog standard damage proc for the buff
  auto secondary       = new special_effect_t( effect.player );
  secondary->type      = SPECIAL_EFFECT_EQUIP;
  secondary->spell_id  = effect.spell_id;
  secondary->cooldown_ = timespan_t::zero();
  secondary->execute_action =
      create_proc_action<aoe_proc_t>( "potion_bursting_blood", effect, "potion_of_bursting_blood", effect.trigger() );
  effect.player->special_effects.push_back( secondary );

  auto proc = new dbc_proc_callback_t( effect.player, *secondary );
  proc->deactivate();
  proc->initialize();

  effect.custom_buff = make_buff( effect.player, effect.name(), effect.driver() )
                           ->set_stack_change_callback( [proc]( buff_t*, int, int new_ ) {
                             if ( new_ == 1 )
                               proc->activate();
                             else
                               proc->deactivate();
                           } )
                           ->set_cooldown( timespan_t::zero() )  // Handled by the action
                           ->set_chance( 1.0 );                  // Override chance so the buff actually triggers
}

// Potion of Unbridled Fury =================================================

void consumables::potion_of_unbridled_fury( special_effect_t& effect )
{
  effect.disable_action();

  // Make a bog standard damage proc for the buff
  auto secondary            = new special_effect_t( effect.player );
  secondary->type           = SPECIAL_EFFECT_EQUIP;
  secondary->spell_id       = effect.spell_id;
  secondary->cooldown_      = timespan_t::zero();
  secondary->execute_action = create_proc_action<proc_spell_t>( "potion_of_unbridled_fury", effect );
  effect.player->special_effects.push_back( secondary );

  auto proc = new dbc_proc_callback_t( effect.player, *secondary );
  proc->deactivate();
  proc->initialize();

  effect.custom_buff = make_buff( effect.player, effect.name(), effect.driver() )
                           ->set_stack_change_callback( [proc]( buff_t*, int, int new_ ) {
                             if ( new_ == 1 )
                               proc->activate();
                             else
                               proc->deactivate();
                           } )
                           ->set_cooldown( timespan_t::zero() )  // Handled by the action
                           ->set_chance( 1.0 );                  // Override chance so the buff actually triggers
}

// Potion of Focused Resolve ================================================

struct potion_of_focused_resolve_t : public item_targetdata_initializer_t
{
  potion_of_focused_resolve_t() : item_targetdata_initializer_t( 0, {} )
  {
  }

  const special_effect_t* find_effect( player_t* player ) const override
  {
    auto it = range::find_if( player->special_effects,
                              []( const special_effect_t* effect ) { return effect->spell_id == 298317; } );

    if ( it != player->special_effects.end() )
    {
      return *it;
    }

    return nullptr;
  }

  // Create the choking brine debuff that is checked on enemy demise
  void operator()( actor_target_data_t* td ) const override
  {
    const special_effect_t* effect = find_effect( td->source );
    if ( !effect )
    {
      td->debuff.focused_resolve = make_buff( *td, "focused_resolve" );
      return;
    }
    assert( !td->debuff.focused_resolve );

    td->debuff.focused_resolve = make_buff( *td, "focused_resolve", td->source->find_spell( 298614 ) )
                                     ->set_default_value( td->source->find_spell( 298614 )->effectN( 1 ).percent() );

    td->debuff.focused_resolve->reset();
  }
};

void consumables::potion_of_focused_resolve( special_effect_t& effect )
{
  struct pofr_callback_t : public dbc_proc_callback_t
  {
    player_t* current_target;

    pofr_callback_t( const special_effect_t& effect )
      : dbc_proc_callback_t( effect.player, effect ), current_target( nullptr )
    {
    }

    void execute( action_t* /* a */, action_state_t* state ) override
    {
      if ( current_target != state->target )
      {
        set_target( state->target );
      }
    }

    void set_target( player_t* t )
    {
      if ( current_target )
      {
        auto td = listener->get_target_data( current_target );
        td->debuff.focused_resolve->expire();
      }

      auto td = listener->get_target_data( t );
      td->debuff.focused_resolve->trigger();

      current_target = t;
    }

    void add_stack()
    {
      if ( current_target )
      {
        auto td = listener->get_target_data( current_target );
        td->debuff.focused_resolve->trigger();
      }
    }

    void deactivate() override
    {
      dbc_proc_callback_t::deactivate();

      if ( current_target )
      {
        auto td = listener->get_target_data( current_target );
        td->debuff.focused_resolve->expire();
      }
    }

    void activate() override
    {
      dbc_proc_callback_t::activate();

      current_target = nullptr;
    }
  };

  auto buff = buff_t::find( effect.player, "potion_of_focused_resolve" );
  if ( !buff )
  {
    buff = make_buff( effect.player, "potion_of_focused_resolve", effect.driver() );
  }

  effect.custom_buff = buff;

  auto proc_obj      = new special_effect_t( effect.player );
  proc_obj->type     = SPECIAL_EFFECT_EQUIP;
  proc_obj->spell_id = effect.spell_id;
  // Make buff jump around per target hit on aoe situations
  proc_obj->proc_flags2_ = PF2_HIT | PF2_CRIT | PF2_GLANCE;
  effect.player->special_effects.push_back( proc_obj );

  auto cb = new pofr_callback_t( *proc_obj );
  cb->initialize();
  cb->deactivate();

  buff->set_stack_change_callback( [cb]( buff_t* /* b */, int /* old */, int new_ ) {
    if ( new_ )
    {
      cb->activate();
    }
    else
    {
      cb->deactivate();
    }
  } );

  buff->set_tick_callback( [cb]( buff_t*, int, timespan_t ) { cb->add_stack(); } );
}

// Potion of Empowered Proximity ============================================

void consumables::potion_of_empowered_proximity( special_effect_t& effect )
{
  auto buff = static_cast<stat_buff_t*>( buff_t::find( effect.player, "potion_of_empowered_proximity" ) );
  if ( buff )
  {
    return;
  }

  stat_e primary_stat = effect.player->convert_hybrid_stat( STAT_STR_AGI_INT );
  double stat_value   = std::ceil( effect.driver()->effectN( 5 ).average( effect.player ) );
  double max_value    = as<double>( effect.driver()->effectN( 6 ).base_value() );
  buff                = make_buff<stat_buff_t>( effect.player, "potion_of_empowered_proximity", effect.driver() );
  buff->add_stat( primary_stat, stat_value );
  buff->set_stack_change_callback( [buff, stat_value, max_value]( buff_t* /* b */, int /* old */, int new_ ) {
    // Stack change callback is called (in stat_buff_t::bump) before stat_buff_t grants stacks, so
    // this is an ok way to grant everything in one go based on the current active enemy state
    if ( new_ )
    {
      auto new_value = clamp( buff->sim->target_non_sleeping_list.size() * stat_value, stat_value, max_value );
      buff->stats.front().amount = new_value;
    }
    else
    {
      buff->stats.front().amount = stat_value;
    }
  } );

  // Note, you cannot make callbacks to the global vectors (target_non_sleeping_list, target_list,
  // player_non_sleeping_list, etc ...) since those will be reset after init (due to single actor
  // simulation mode vs multi actor mode)
  effect.activation_cb = [buff, stat_value, primary_stat, max_value]() {
    buff->sim->target_non_sleeping_list.register_callback( [buff, stat_value, primary_stat, max_value]( player_t* ) {
      if ( !buff->up() )
      {
        return;
      }

      auto new_value = clamp( buff->sim->target_non_sleeping_list.size() * stat_value, stat_value, max_value );
      auto diff      = new_value - buff->stats.front().current_value;
      if ( diff > 0 )
      {
        buff->source->stat_gain( primary_stat, diff, buff->stat_gain );
      }
      else if ( diff < 0 )
      {
        buff->source->stat_loss( primary_stat, fabs( diff ), buff->stat_gain );
      }

      buff->stats.front().current_value += diff;
    } );
  };
}

// Gale-Force Striking ======================================================

void enchants::galeforce_striking( special_effect_t& effect )
{
  buff_t* buff = effect.player->buffs.galeforce_striking;
  if ( !buff )
  {
    auto spell = effect.trigger();
    buff       = make_buff( effect.player, util::tokenized_name( spell ), spell )
               ->add_invalidate( CACHE_ATTACK_SPEED )
               ->set_default_value( spell->effectN( 1 ).percent() )
               ->set_activated( false );
    effect.player->buffs.galeforce_striking = buff;
  }

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.player, effect );
}

// Torrent of Elements ======================================================

void enchants::torrent_of_elements( special_effect_t& effect )
{
  buff_t* buff = effect.player->buffs.torrent_of_elements;
  if ( !buff )
  {
    auto spell = effect.trigger();
    buff       = make_buff<buff_t>( effect.player, util::tokenized_name( spell ), spell )
               ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
               ->set_default_value( spell->effectN( 1 ).percent() )
               ->set_activated( false );
    effect.player->buffs.torrent_of_elements = buff;
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
    {
    }

    void execute( action_t*, action_state_t* ) override
    {
      // From logs it seems like the stacking buff can't trigger while the 'final' one is up. Logs
      // also look like the RPPM effect is allowed to proc, but the stacking buff is not triggered
      // during the final buff.
      if ( final_buff->check() )
      {
        return;
      }

      if ( proc_buff && proc_buff->trigger() && proc_buff->check() == proc_buff->max_stack() )
      {
        final_buff->trigger();
        proc_buff->expire();
      }
    }
  };

  // Both the stacking and the final buffs are unique
  return [buff_id]( special_effect_t& effect ) {
    auto spell_buff              = effect.trigger();
    const std::string spell_name = util::tokenized_name( spell_buff );
    buff_t* buff                 = buff_t::find( effect.player, spell_name );
    if ( !buff )
    {
      buff = make_buff<stat_buff_t>( effect.player, spell_name, spell_buff );
    }

    effect.custom_buff = buff;

    auto final_spell_data              = effect.player->find_spell( buff_id );
    const std::string final_spell_name = util::tokenized_name( final_spell_data ) + "_final";
    buff_t* final_buff                 = buff_t::find( effect.player, final_spell_name );
    if ( !final_buff )
    {
      final_buff = make_buff<stat_buff_t>( effect.player, final_spell_name, final_spell_data );
    }

    new navigation_proc_callback_t( effect.player, effect, final_buff );
  };
}

// Machinist's Brilliance ===================================================

namespace
{
// Simple functor to trigger the highest buff for Machinist's Brilliance and Force Multiplier
// enchants
struct bfa_82_enchant_functor_t
{
  buff_t *haste, *crit, *mastery;

  bfa_82_enchant_functor_t( buff_t* h, buff_t* c, buff_t* m ) : haste( h ), crit( c ), mastery( m )
  {
  }

  void operator()( buff_t* b, int /* old */, int new_ )
  {
    crit->expire();
    haste->expire();
    mastery->expire();

    static constexpr stat_e ratings[] = { STAT_CRIT_RATING, STAT_MASTERY_RATING, STAT_HASTE_RATING };
    buff_t* selected_buff = nullptr;
    switch ( ::util::highest_stat( b->source, ratings ) )
    {
      case STAT_CRIT_RATING:
        selected_buff = crit;
        break;
      case STAT_MASTERY_RATING:
        selected_buff = mastery;
        break;
      case STAT_HASTE_RATING:
        selected_buff = haste;
        break;
      default:
        break;
    }

    // Couldn't pick anything, don't trigger
    if ( !selected_buff )
    {
      return;
    }

    if ( new_ == 1 )
    {
      selected_buff->trigger();
    }
    else
    {
      crit->expire();
      haste->expire();
      mastery->expire();
    }
  }
};

}  // namespace

void enchants::machinists_brilliance( special_effect_t& effect )
{
  auto crit = buff_t::find( effect.player, "machinists_brilliance_crit" );
  if ( !crit )
  {
    crit = make_buff<stat_buff_t>( effect.player, "machinists_brilliance_crit", effect.player->find_spell( 298431 ) );
  }
  auto haste = buff_t::find( effect.player, "machinists_brilliance_haste" );
  if ( !haste )
  {
    haste = make_buff<stat_buff_t>( effect.player, "machinists_brilliance_haste", effect.player->find_spell( 300761 ) );
  }
  auto mastery = buff_t::find( effect.player, "machinists_brilliance_mastery" );
  if ( !mastery )
  {
    mastery =
        make_buff<stat_buff_t>( effect.player, "machinists_brilliance_mastery", effect.player->find_spell( 300762 ) );
  }

  auto buff = buff_t::find( effect.player, "machinists_brilliance" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "machinists_brilliance", effect.player->find_spell( 300693 ) );
    buff->set_stack_change_callback( bfa_82_enchant_functor_t( haste, crit, mastery ) );
  }

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.player, effect );
}

// Force Multiplier ==========================================================

void enchants::force_multiplier( special_effect_t& effect )
{
  auto crit = buff_t::find( effect.player, "force_multiplier_crit" );
  if ( !crit )
  {
    crit = make_buff<stat_buff_t>( effect.player, "force_multiplier_crit", effect.player->find_spell( 300801 ) );
  }
  auto haste = buff_t::find( effect.player, "force_multiplier_haste" );
  if ( !haste )
  {
    haste = make_buff<stat_buff_t>( effect.player, "force_multiplier_haste", effect.player->find_spell( 300802 ) );
  }
  auto mastery = buff_t::find( effect.player, "force_multiplier_mastery" );
  if ( !mastery )
  {
    mastery = make_buff<stat_buff_t>( effect.player, "force_multiplier_mastery", effect.player->find_spell( 300809 ) );
  }

  stat_buff_t* buff = debug_cast<stat_buff_t*>( buff_t::find( effect.player, "force_multiplier" ) );
  if ( !buff )
  {
    if ( effect.player->convert_hybrid_stat( STAT_STR_AGI ) == STAT_STRENGTH )
    {
      buff = make_buff<stat_buff_t>( effect.player, "force_multiplier", effect.player->find_spell( 300691 ) );
    }
    else
    {
      buff = make_buff<stat_buff_t>( effect.player, "force_multiplier", effect.player->find_spell( 300893 ) );
    }

    buff->set_stack_change_callback( bfa_82_enchant_functor_t( haste, crit, mastery ) );
  }

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.player, effect );
}

// Kaja-fied Banana =========================================================

void items::kajafied_banana( special_effect_t& effect )
{
  effect.execute_action = create_proc_action<aoe_proc_t>( "kajafied_banana", effect, "kajafied_banana", 274575 );

  new dbc_proc_callback_t( effect.player, effect );
}

// Incessantly Ticking Clock ================================================

void items::incessantly_ticking_clock( special_effect_t& effect )
{
  stat_buff_t* tick =
      create_buff<stat_buff_t>( effect.player, "tick", effect.player->find_spell( 274430 ), effect.item );
  stat_buff_t* tock =
      create_buff<stat_buff_t>( effect.player, "tock", effect.player->find_spell( 274431 ), effect.item );

  struct clock_cb_t : public dbc_proc_callback_t
  {
    std::vector<buff_t*> buffs;
    size_t state;

    clock_cb_t( const special_effect_t& effect, const std::vector<buff_t*>& b )
      : dbc_proc_callback_t( effect.player, effect ), buffs( b ), state( 0u )
    {
    }

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

  new clock_cb_t( effect, {tick, tock} );
}

// Snowpelt Mangler =========================================================

void items::snowpelt_mangler( special_effect_t& effect )
{
  effect.execute_action =
      create_proc_action<aoe_proc_t>( "sharpened_claws", effect, "sharpened_claws", effect.trigger() );

  new dbc_proc_callback_t( effect.player, effect );
}

// Vial of Storms ===========================================================

void items::vial_of_storms( special_effect_t& effect )
{
  effect.execute_action =
      create_proc_action<aoe_proc_t>( "bottled_lightning", effect, "bottled_lightning", effect.trigger() );
}

// Landoi's Scrutiny ========================================================

// TODO: Targeting mechanism
void items::landois_scrutiny( special_effect_t& effect )
{
  struct ls_cb_t : public dbc_proc_callback_t
  {
    buff_t* haste;
    player_t* current_target;

    ls_cb_t( const special_effect_t& effect, buff_t* h ) : dbc_proc_callback_t( effect.player, effect ), haste( h )
    {
    }

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

  effect.custom_buff = create_buff<stat_buff_t>( effect.player, "landois_scrutiny", effect.trigger() );

  new ls_cb_t( effect, haste_buff );
}

// Bygone Bee Almanac =======================================================

enum resource_category : unsigned
{
  RC_BUFF = 0u,
  RC_COOLDOWN
};

// Add different resource systems here
// Todo : values were changed, need confirmation on :
// rage, runic power, shield of the righteous charges
static constexpr auto __resource_map = ::util::make_static_map<int, std::array<double, 2>>( {
  { RESOURCE_HOLY_POWER,  {{ 0.800, 0.800 }} },
  { RESOURCE_RUNIC_POWER, {{ 0.075, 0.054 }} },
  { RESOURCE_RAGE,        {{ 0.025, 0.050 }} },
} );

struct bba_cb_t : public dbc_proc_callback_t
{
  bba_cb_t( const special_effect_t& effect ) : dbc_proc_callback_t( effect.player, effect )
  {
  }

  template <resource_category c>
  timespan_t value( action_state_t* state ) const
  {
    int resource = static_cast<int>( state->action->current_resource() );
    double cost  = state->action->last_resource_cost;

    // Breath of Sindragosa needs to be special cased, as the tick action doesn't cost any resource
    if ( state->action->data().id() == 155166u )
    {
      cost     = 15.0;
      resource = RESOURCE_RUNIC_POWER;

      // There seems to be a bug / special handling of BoS on server side
      // It only triggers bygone bee ~60% of the time
      // This will need more data
      if ( state->action->player->bugs && rng().roll( 0.4 ) )
      {
        return timespan_t::zero();
      }
    }

    // Protection Paladins are special :
    // - Consuming SotR charges with Seraphim doesn't trigger the callback
    // - Even though SotR can be used without a target, the effect only happens if the spell hits at
    //   least one target and has a 3.0s value for both buff extension and the on-use's cdr
    if ( state->action->data().id() == 53600u && state->action->result_is_hit( state->result ) )
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

  void trigger( action_t* a, action_state_t* state ) override
  {
    if ( state->action->last_resource_cost == 0 )
    {
      // Custom call here for BoS ticks and SotR because they don't consume resources
      // The callback should probably rather check for ticks of breath_of_sindragosa, but this will do for now
      if ( state->action->data().id() == 155166u || state->action->data().id() == 53600u )
      {
        dbc_proc_callback_t::trigger( a, state );
      }

      return;
    }

    dbc_proc_callback_t::trigger( a, state );
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
    {
    }

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

    bba_inactive_cb_t( const special_effect_t& effect, const special_effect_t& base )
      : bba_cb_t( effect ), cd( listener->get_cooldown( base.cooldown_name() ) )
    {
    }

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

  auto bba_active_effect          = new special_effect_t( effect.player );
  bba_active_effect->name_str     = "bba_active_driver_callback";
  bba_active_effect->type         = SPECIAL_EFFECT_EQUIP;
  bba_active_effect->source       = SPECIAL_EFFECT_SOURCE_ITEM;
  bba_active_effect->proc_chance_ = 1.0;
  bba_active_effect->proc_flags_  = PF_ALL_DAMAGE | PF_PERIODIC;
  effect.player->special_effects.push_back( bba_active_effect );

  auto bba_active = new bba_active_cb_t( *bba_active_effect );

  auto bba_inactive_effect          = new special_effect_t( effect.player );
  bba_inactive_effect->name_str     = "bba_inactive_driver_callback";
  bba_inactive_effect->type         = SPECIAL_EFFECT_EQUIP;
  bba_inactive_effect->source       = SPECIAL_EFFECT_SOURCE_ITEM;
  bba_inactive_effect->proc_chance_ = 1.0;
  bba_inactive_effect->proc_flags_  = PF_ALL_DAMAGE | PF_PERIODIC;
  effect.player->special_effects.push_back( bba_inactive_effect );

  auto bba_inactive  = new bba_inactive_cb_t( *bba_inactive_effect, effect );
  effect.custom_buff = create_buff<stat_buff_t>( effect.player, "process_improvement", effect.trigger(), effect.item )
                           ->set_stack_change_callback( [bba_active, bba_inactive]( buff_t*, int, int new_ ) {
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
  effect.player->buffs.leyshock_crit =
      create_buff<stat_buff_t>( effect.player, "precision_module", effect.player->find_spell( 281791 ), effect.item );
  effect.player->buffs.leyshock_haste = create_buff<stat_buff_t>( effect.player, "iteration_capacitor",
                                                                  effect.player->find_spell( 281792 ), effect.item );
  effect.player->buffs.leyshock_mastery =
      create_buff<stat_buff_t>( effect.player, "efficiency_widget", effect.player->find_spell( 281794 ), effect.item );
  effect.player->buffs.leyshock_versa =
      create_buff<stat_buff_t>( effect.player, "adaptive_circuit", effect.player->find_spell( 281795 ), effect.item );

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
  main_amount      = item_database::apply_combat_rating_multiplier( *effect.item, main_amount );
  main_amount /= ( 1 + effect.player->sim->bfa_opts.jes_howler_allies );

  // The player gains additional versatility for every ally buffed
  auto ally_amount =
      effect.player->sim->bfa_opts.jes_howler_allies * effect.driver()->effectN( 2 ).average( effect.item );
  ally_amount = item_database::apply_combat_rating_multiplier( *effect.item, ally_amount );

  effect.custom_buff = create_buff<stat_buff_t>( effect.player, "motivating_howl", effect.driver(), effect.item )
                           ->add_stat( STAT_VERSATILITY_RATING, main_amount )
                           ->add_stat( STAT_VERSATILITY_RATING, ally_amount );
}

// Razdunk's Big Red Button =================================================

void items::razdunks_big_red_button( special_effect_t& effect )
{
  effect.execute_action = create_proc_action<aoe_proc_t>( "razdunks_big_red_button", effect, "razdunks_big_red_button",
                                                          effect.trigger(), true );
}

// Merektha's Fang ==========================================================

void items::merekthas_fang( special_effect_t& effect )
{
  struct noxious_venom_dot_t : public proc_t
  {
    noxious_venom_dot_t( const special_effect_t& effect ) : proc_t( effect, "noxious_venom", 267410 )
    {
      tick_may_crit = hasted_ticks = true;
      dot_max_stack                = data().max_stacks();
    }

    timespan_t calculate_dot_refresh_duration( const dot_t* dot, timespan_t triggered_duration ) const override
    {
      // No pandemic, refreshes to base duration on every channel tick
      return triggered_duration * dot->state->haste;
    }

    double last_tick_factor( const dot_t*, timespan_t, timespan_t ) const override
    {
      return 1.0;
    }
  };

  struct noxious_venom_gland_aoe_driver_t : public spell_t
  {
    action_t* dot;

    noxious_venom_gland_aoe_driver_t( const special_effect_t& effect )
      : spell_t( "noxious_venom_gland_aoe_driver", effect.player ),
        dot( create_proc_action<noxious_venom_dot_t>( "noxious_venom", effect ) )
    {
      aoe        = -1;
      quiet      = true;
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

    noxious_venom_gland_t( const special_effect_t& effect )
      : proc_t( effect, "noxious_venom_gland", effect.driver() ),
        driver( new noxious_venom_gland_aoe_driver_t( effect ) )
    {
      channeled = hasted_ticks = true;
      tick_may_crit = tick_zero = tick_on_application = false;
    }

    void execute() override
    {
      proc_t::execute();
      event_t::cancel( player->readying );
      player->reset_auto_attacks( composite_dot_duration( execute_state ) );
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
  deadeye_spyglass_constructor_t( unsigned iid, ::util::span<const slot_e> s )
    : item_targetdata_initializer_t( iid, s )
  {}

  void operator()( actor_target_data_t* td ) const override
  {
    const special_effect_t* effect = find_effect( td->source );
    if ( !effect )
    {
      td->debuff.dead_ahead = make_buff( *td, "dead_ahead_debuff" );
      return;
    }

    assert( !td->debuff.dead_ahead );

    special_effect_t* effect2 = new special_effect_t( effect->item );
    effect2->type             = SPECIAL_EFFECT_EQUIP;
    effect2->source           = SPECIAL_EFFECT_SOURCE_ITEM;
    effect2->spell_id         = 268758;
    effect->player->special_effects.push_back( effect2 );

    auto callback = new dbc_proc_callback_t( effect->item, *effect2 );
    callback->initialize();
    callback->deactivate();

    td->debuff.dead_ahead = make_buff( *td, "dead_ahead_debuff", effect->trigger() )
                                ->set_activated( false )
                                ->set_stack_change_callback( util::callback_buff_activator( callback ) );
    td->debuff.dead_ahead->reset();
  }
};

void items::deadeye_spyglass( special_effect_t& effect )
{
  struct deadeye_spyglass_cb_t : public dbc_proc_callback_t
  {
    deadeye_spyglass_cb_t( const special_effect_t& effect ) : dbc_proc_callback_t( effect.item, effect )
    {
    }

    void execute( action_t*, action_state_t* s ) override
    {
      auto td = listener->get_target_data( s->target );
      assert( td );
      assert( td->debuff.dead_ahead );
      td->debuff.dead_ahead->trigger();
    }
  };

  if ( effect.spell_id == 268771 )
    new deadeye_spyglass_cb_t( effect );

  if ( effect.spell_id == 268758 )
    effect.create_buff();  // precreate the buff
}

// Tiny Electromental in a Jar ==============================================

void items::tiny_electromental_in_a_jar( special_effect_t& effect )
{
  struct unleash_lightning_t : public proc_spell_t
  {
    unleash_lightning_t( const special_effect_t& effect )
      : proc_spell_t( "unleash_lightning", effect.player, effect.player->find_spell( 267205 ), effect.item )
    {
      aoe              = data().effectN( 1 ).chain_target();
      chain_multiplier = data().effectN( 1 ).chain_multiplier();
    }
  };

  effect.custom_buff = buff_t::find( effect.player, "phenomenal_power" );
  if ( !effect.custom_buff )
    effect.custom_buff = make_buff( effect.player, "phenomenal_power", effect.player->find_spell( 267179 ) );

  effect.execute_action = create_proc_action<unleash_lightning_t>( "unleash_lightning", effect );

  new dbc_proc_callback_t( effect.item, effect );
}

// My'das Talisman ==========================================================

void items::mydas_talisman( special_effect_t& effect )
{
  struct touch_of_gold_cb_t : public dbc_proc_callback_t
  {
    touch_of_gold_cb_t( const special_effect_t& effect ) : dbc_proc_callback_t( effect.item, effect )
    {
    }

    void execute( action_t*, action_state_t* s ) override
    {
      assert( proc_action );
      assert( proc_buff );

      proc_action->set_target( s->target );
      proc_action->execute();
      proc_buff->decrement();
    }
  };

  auto effect2              = new special_effect_t( effect.item );
  effect2->name_str         = "touch_of_gold";
  effect2->source           = effect.source;
  effect2->proc_flags_      = effect.proc_flags();
  effect2->proc_chance_     = effect.proc_chance();
  effect2->cooldown_        = timespan_t::zero();
  effect2->trigger_spell_id = effect.trigger()->id();
  effect.player->special_effects.push_back( effect2 );

  auto callback = new touch_of_gold_cb_t( *effect2 );
  callback->deactivate();

  effect.custom_buff = create_buff<buff_t>( effect.player, "touch_of_gold", effect.driver() )
                           ->set_stack_change_callback( util::callback_buff_activator( callback ) )
                           ->set_reverse( true );
  effect2->custom_buff = effect.custom_buff;

  // the trinket 'on use' essentially triggers itself
  effect.trigger_spell_id = effect.driver()->id();
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
  auto haste_low = create_buff<stat_buff_t>( effect.player, "loaded_die_haste_low", effect.player->find_spell( 267327 ),
                                             effect.item );
  auto haste_high = create_buff<stat_buff_t>( effect.player, "loaded_die_haste_high",
                                              effect.player->find_spell( 267329 ), effect.item );
  auto crit_low   = create_buff<stat_buff_t>( effect.player, "loaded_die_critical_strike_low",
                                            effect.player->find_spell( 267330 ), effect.item );
  auto crit_high  = create_buff<stat_buff_t>( effect.player, "loaded_die_critical_strike_high",
                                             effect.player->find_spell( 267331 ), effect.item );

  struct harlans_cb_t : public dbc_proc_callback_t
  {
    std::vector<std::vector<buff_t*>> buffs;

    harlans_cb_t( const special_effect_t& effect, const std::vector<std::vector<buff_t*>>& b )
      : dbc_proc_callback_t( effect.item, effect ), buffs( b )
    {
    }

    void execute( action_t*, action_state_t* ) override
    {
      range::for_each( buffs, [this]( const std::vector<buff_t*>& buffs ) {
        // Coinflip for now
        unsigned idx = rng().roll( 0.5 ) ? 1 : 0;
        buffs[ idx ]->trigger();
      } );
    }
  };

  new harlans_cb_t( effect, {{mastery_low, mastery_high}, {haste_low, haste_high}, {crit_low, crit_high}} );
}

// Kul Tiran Cannonball Runner ==============================================

void items::kul_tiran_cannonball_runner( special_effect_t& effect )
{
  struct cannonball_cb_t : public dbc_proc_callback_t
  {
    cannonball_cb_t( const special_effect_t& effect ) : dbc_proc_callback_t( effect.item, effect )
    {
    }

    void execute( action_t*, action_state_t* state ) override
    {
      make_event<ground_aoe_event_t>( *listener->sim, listener,
                                      ground_aoe_params_t()
                                          .target( state->target )
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
  effect.execute_action =
      create_proc_action<aoe_proc_t>( "webweavers_soul_gem", effect, "webweavers_soul_gem", 270827, true );
  effect.execute_action->travel_speed = effect.trigger()->missile_speed();

  new dbc_proc_callback_t( effect.player, effect );
}

// Vigilant's Bloodshaper ==============================================

void items::vigilants_bloodshaper( special_effect_t& effect )
{
  effect.execute_action =
      create_proc_action<aoe_proc_t>( "volatile_blood_explosion", effect, "volatile_blood_explosion", 278057, true );
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
    {
    }

    void execute( action_t*, action_state_t* ) override
    {
      if ( proc_buff && proc_buff->trigger() && proc_buff->check() == proc_buff->max_stack() )
      {
        final_buff->trigger();
        proc_buff->expire();
      }
    }
  };

  effect.proc_flags2_ = PF2_ALL_HIT;

  // Doesn't seem to be linked in the effect.
  auto buff_data = effect.player->find_spell( 278155 );
  buff_t* buff   = buff_t::find( effect.player, "lingering_power_of_xalzaix" );
  if ( !buff )
  {
    buff = make_buff( effect.player, "lingering_power_of_xalzaix", buff_data, effect.item );
  }

  effect.custom_buff = buff;

  auto final_buff_data = effect.player->find_spell( 278156 );
  buff_t* final_buff   = buff_t::find( effect.player, "uncontained_power" );
  if ( !final_buff )
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

    bloody_bile_t( player_t* p, const special_effect_t& effect )
      : proc_spell_t( "bloody_bile", p, p->find_spell( 279664 ), effect.item ), n_casts( 0 )
    {
      background  = false;
      base_dd_min = effect.driver()->effectN( 1 ).min( effect.item );
      base_dd_max = effect.driver()->effectN( 1 ).max( effect.item );
    }

    void execute() override
    {
      proc_spell_t::execute();

      // Casts 6 times and then despawns
      if ( ++n_casts == 6 )
      {
        make_event( *sim, timespan_t::zero(), [this]() { player->cast_pet()->dismiss(); } );
      }
    }
  };

  struct tendril_pet_t : public pet_t
  {
    bloody_bile_t* bloody_bile;
    const special_effect_t* effect;

    tendril_pet_t( const special_effect_t& effect )
      : pet_t( effect.player->sim, effect.player, "vanquished_tendril_of_ghuun", true, true ),
        bloody_bile( nullptr ),
        effect( &effect )
    {
    }

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

    action_t* create_action( ::util::string_view name, const std::string& opts ) override
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

    tendril_cb_t( const special_effect_t& effect )
      : dbc_proc_callback_t( effect.player, effect ),
        spawner( "vanquished_tendril_of_ghuun", effect.player,
                 [&effect]( player_t* ) { return new tendril_pet_t( effect ); } )
    {
      spawner.set_default_duration( effect.player->find_spell( 278163 )->duration() );
    }

    void execute( action_t*, action_state_t* ) override
    {
      spawner.spawn();
    }
  };

  new tendril_cb_t( effect );
}

// Vial of Animated Blood ==========================================================

void items::vial_of_animated_blood( special_effect_t& effect )
{
  effect.reverse                 = true;
  effect.tick                    = effect.driver()->effectN( 2 ).period();
  effect.reverse_stack_reduction = 1;
}

// Briny Barnacle ===========================================================

struct briny_barnacle_constructor_t : public item_targetdata_initializer_t
{
  briny_barnacle_constructor_t( unsigned iid, ::util::span<const slot_e> s )
    : item_targetdata_initializer_t( iid, s )
  {}

  // Create the choking brine debuff that is checked on enemy demise
  void operator()( actor_target_data_t* td ) const override
  {
    const special_effect_t* effect = find_effect( td->source );
    if ( !effect )
    {
      td->debuff.choking_brine = make_buff( *td, "choking_brine_debuff" );
      return;
    }
    assert( !td->debuff.choking_brine );

    td->debuff.choking_brine = make_buff( *td, "choking_brine_debuff", effect->trigger() )->set_activated( false );
    td->debuff.choking_brine->reset();
  }
};

void items::briny_barnacle( special_effect_t& effect )
{
  struct choking_brine_damage_t : public proc_t
  {
    choking_brine_damage_t( const special_effect_t& effect )
      : proc_t( effect, "choking_brine", effect.driver()->effectN( 1 ).trigger() )
    {
    }

    void execute() override
    {
      proc_t::execute();

      auto td = player->get_target_data( execute_state->target );
      assert( td );
      assert( td->debuff.choking_brine );
      td->debuff.choking_brine->trigger();
    }
  };

  struct choking_brine_spreader_t : public proc_t
  {
    action_t* damage;

    choking_brine_spreader_t( const special_effect_t& effect, action_t* damage_action )
      : proc_t( effect, "choking_brine_explosion", effect.driver() ), damage( damage_action )
    {
      aoe = -1;
    }

    void impact( action_state_t* s ) override
    {
      proc_t::impact( s );
      damage->set_target( s->target );
      damage->execute();
    }
  };

  action_t* choking_brine_damage = create_proc_action<choking_brine_damage_t>( "choking_brine", effect );
  action_t* explosion            = new choking_brine_spreader_t( effect, choking_brine_damage );

  auto p = effect.player;
  // Add a callback on demise to each enemy in the simulation
  range::for_each( p->sim->actor_list, [p, explosion]( player_t* target ) {
    // Don't do anything on players
    if ( !target->is_enemy() )
    {
      return;
    }

    target->register_on_demise_callback( p, [p, explosion]( player_t* target ) {
      // Don't do anything if the sim is ending
      if ( target->sim->event_mgr.canceled )
      {
        return;
      }

      auto td = p->get_target_data( target );

      if ( td->debuff.choking_brine->up() )
      {
        target->sim->print_log( "Enemy {} dies while afflicted by Choking Brine, applying debuff on all neaby enemies",
                                target->name_str );
        explosion->schedule_execute();
      }
    } );
  } );

  effect.execute_action = choking_brine_damage;

  new dbc_proc_callback_t( effect.item, effect );
}

// Rotcrusted Voodoo Doll ===================================================

void items::rotcrusted_voodoo_doll( special_effect_t& effect )
{
  struct rotcrusted_voodoo_doll_dot_t : public proc_spell_t
  {
    action_t* final_damage;

    rotcrusted_voodoo_doll_dot_t( const special_effect_t& effect )
      : proc_spell_t( "rotcrusted_voodoo_doll", effect.player, effect.trigger(), effect.item ),
        final_damage( new proc_spell_t( "rotcrusted_voodoo_doll_final", effect.player,
                                        effect.player->find_spell( 271468 ), effect.item ) )
    {
      tick_zero    = true;
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

  effect.execute_action = create_proc_action<rotcrusted_voodoo_doll_dot_t>( "rotcrusted_voodoo_doll", effect );
}

// Hadal's Nautilus =========================================================

void items::hadals_nautilus( special_effect_t& effect )
{
  struct nautilus_cb_t : public dbc_proc_callback_t
  {
    const spell_data_t* driver;

    nautilus_cb_t( const special_effect_t& effect )
      : dbc_proc_callback_t( effect.item, effect ), driver( effect.player->find_spell( 270910 ) )
    {
    }

    void execute( action_t*, action_state_t* state ) override
    {
      make_event<ground_aoe_event_t>( *listener->sim, listener,
                                      ground_aoe_params_t()
                                          .target( state->target )
                                          .pulse_time( driver->duration() )
                                          .n_pulses( 1 )
                                          .action( proc_action ) );
    }
  };

  effect.execute_action = create_proc_action<aoe_proc_t>( "waterspout", effect, "waterspout", 270925, true );

  new nautilus_cb_t( effect );
}

// Lingering Sporepods ======================================================

void items::lingering_sporepods( special_effect_t& effect )
{
  effect.custom_buff = buff_t::find( effect.player, "lingering_spore_pods" );
  if ( !effect.custom_buff )
  {
    // TODO 8.1: Now deals increased damage with more targets.
    auto damage =
        create_proc_action<aoe_proc_t>( "lingering_spore_pods_damage", effect, "lingering_spore_pods_damage", 268068 );

    auto heal = create_proc_action<base_bfa_proc_t<proc_heal_t>>( "lingering_spore_pods_heal", effect,
                                                                  "lingering_spore_pods_heal", 278708 );

    effect.custom_buff = make_buff( effect.player, "lingering_spore_pods", effect.trigger() )
                             ->set_stack_change_callback( [damage, heal]( buff_t* b, int, int new_ ) {
                               if ( new_ == 0 )
                               {
                                 damage->set_target( b->player->target );
                                 damage->execute();
                                 heal->set_target( b->player );
                                 heal->execute();
                               }
                             } );
  }

  new dbc_proc_callback_t( effect.player, effect );
}

// Lady Waycrest's Music Box ================================================
struct waycrest_legacy_damage_t : public proc_t
{
  waycrest_legacy_damage_t( const special_effect_t& effect ) : proc_t( effect, "waycrest_legacy_damage", 271671 )
  {
    aoe                = 0;
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
  waycrest_legacy_heal_t( const special_effect_t& effect )
    : base_bfa_proc_t<proc_heal_t>( effect, "waycrest_legacy_heal", 271682 )
  {
    aoe                = 0;
    base_dd_multiplier = effect.player->find_spell( 277522 )->effectN( 2 ).base_value() / 100.0;
  }
  void execute() override
  {
    size_t target_index = static_cast<size_t>( rng().range( 0, as<double>( sim->player_no_pet_list.data().size() ) ) );
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
    cacaphonous_chord_t( const special_effect_t& effect ) : proc_t( effect, "cacaphonous_chord", 271671 )
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
    harmonious_chord_t( const special_effect_t& effect )
      : base_bfa_proc_t<proc_heal_t>( effect, "harmonious_chord", 271682 )
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
      size_t target_index =
          static_cast<size_t>( rng().range( 0, as<double>( sim->player_no_pet_list.data().size() ) ) );
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
  effect.reverse                 = true;
  effect.tick                    = effect.driver()->effectN( 2 ).period();
  effect.reverse_stack_reduction = 5;  // Nowhere to be seen in the spell data
}

// Frenetic Corpuscle =======================================================

void items::frenetic_corpuscle( special_effect_t& effect )
{
  struct frenetic_corpuscle_cb_t : public dbc_proc_callback_t
  {
    action_t* damage;

    frenetic_corpuscle_cb_t( const special_effect_t& effect )
      : dbc_proc_callback_t( effect.item, effect ),
        damage( create_proc_action<proc_t>( "frenetic_blow", effect, "frentic_blow", 278148 ) )
    {
    }

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

  effect.custom_buff =
      create_buff<buff_t>( effect.player, "frothing_rage", effect.player->find_spell( 278143 ), effect.item );

  new frenetic_corpuscle_cb_t( effect );
}

// Sandscoured Idol =======================================================

void items::sandscoured_idol( special_effect_t& effect )
{
  effect.reverse                 = true;
  effect.tick                    = effect.driver()->effectN( 2 ).period();
  effect.reverse_stack_reduction = 4;
}

// Darkmoon Faire decks

void items::darkmoon_deck_squalls( special_effect_t& effect )
{
  struct squall_damage_t : public proc_spell_t
  {
    const spell_data_t* duration;

    squall_damage_t( const special_effect_t& effect, unsigned card_id )
      : proc_spell_t( "suffocating_squall", effect.player, effect.trigger(), effect.item ),
        duration( effect.player->find_spell( card_id ) )
    {
      tick_may_crit = true;
    }

    timespan_t composite_dot_duration( const action_state_t* ) const override
    {
      return duration->effectN( 1 ).time_value();
    }
  };

  new bfa_darkmoon_deck_cb_t<squall_damage_t>( effect,
                                               {276124, 276125, 276126, 276127, 276128, 276129, 276130, 276131} );
}

void items::darkmoon_deck_fathoms( special_effect_t& effect )
{
  struct fathoms_damage_t : public proc_attack_t
  {
    fathoms_damage_t( const special_effect_t& effect, unsigned card_id )
      : proc_attack_t( "fathom_fall", effect.player, effect.player->find_spell( 276199 ), effect.item )
    {
      base_dd_min = base_dd_max = effect.player->find_spell( card_id )->effectN( 1 ).average( effect.item );
    }
  };

  new bfa_darkmoon_deck_cb_t<fathoms_damage_t>( effect,
                                                {276187, 276188, 276189, 276190, 276191, 276192, 276193, 276194} );
}

// Endless Tincture of Fractional Power =====================================

void items::endless_tincture_of_fractional_power( special_effect_t& effect )
{
  struct endless_tincture_of_fractional_power_t : public proc_spell_t
  {
    std::vector<stat_buff_t*> buffs;

    endless_tincture_of_fractional_power_t( const special_effect_t& effect )
      : proc_spell_t( "endless_tincture_of_fractional_power", effect.player, effect.driver(), effect.item )
    {
      // TOCHECK: Buffs don't appear to scale from ilevel right now and only scale to 110
      // Blizzard either has some script magic or the spells need fixing, needs testing
      // As per Navv, added some checking of the ilevel flag in case they fix this via data
      constexpr unsigned buff_ids[] = { 265442, 265443, 265444, 265446 };
      for ( unsigned buff_id : buff_ids )
      {
        const spell_data_t* buff_spell = effect.player->find_spell( buff_id );
        if ( buff_spell->flags( spell_attribute::SX_SCALE_ILEVEL ) )
          buffs.push_back(
              make_buff<stat_buff_t>( effect.player, util::tokenized_name( buff_spell ), buff_spell, effect.item ) );
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
          const auto it           = range::find_if( buffs, [flask_stat]( const stat_buff_t* buff ) {
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
  syringe_of_bloodborne_infirmity_constructor_t( unsigned iid, ::util::span<const slot_e> s )
    : item_targetdata_initializer_t( iid, s )
  {}

  void operator()( actor_target_data_t* td ) const override
  {
    const special_effect_t* effect = find_effect( td->source );
    if ( !effect )
    {
      td->debuff.wasting_infection = make_buff( *td, "wasting_infection_debuff" );
      return;
    }
    assert( !td->debuff.wasting_infection );

    special_effect_t* effect2 = new special_effect_t( effect->item );
    effect2->type             = SPECIAL_EFFECT_EQUIP;
    effect2->source           = SPECIAL_EFFECT_SOURCE_ITEM;
    effect2->spell_id         = 278109;
    effect->player->special_effects.push_back( effect2 );

    auto callback = new dbc_proc_callback_t( effect->item, *effect2 );
    callback->initialize();
    callback->deactivate();

    td->debuff.wasting_infection = make_buff( *td, "wasting_infection_debuff", effect->trigger() )
                                       ->set_activated( false )
                                       ->set_stack_change_callback( util::callback_buff_activator( callback ) );
    td->debuff.wasting_infection->reset();
  }
};

void items::syringe_of_bloodborne_infirmity( special_effect_t& effect )
{
  struct syringe_of_bloodborne_infirmity_cb_t : public dbc_proc_callback_t
  {
    action_t* damage;
    syringe_of_bloodborne_infirmity_cb_t( const special_effect_t& effect )
      : dbc_proc_callback_t( effect.item, effect ),
        damage( create_proc_action<proc_spell_t>( "wasting_infection", effect ) )
    {
    }

    void execute( action_t* /* a */, action_state_t* state ) override
    {
      damage->set_target( state->target );
      damage->execute();

      auto td = listener->get_target_data( state->target );
      assert( td );
      assert( td->debuff.wasting_infection );
      td->debuff.wasting_infection->trigger();
    }
  };

  // dot proc on attack
  if ( effect.spell_id == 278112 )
    new syringe_of_bloodborne_infirmity_cb_t( effect );

  // crit buff on attacking debuffed target
  if ( effect.spell_id == 278109 )
    effect.create_buff();  // precreate the buff
}

// Disc of Systematic Regression ============================================

void items::disc_of_systematic_regression( special_effect_t& effect )
{
  effect.execute_action = create_proc_action<aoe_proc_t>( "voided_sectors", effect, "voided_sectors", 278153, true );

  new dbc_proc_callback_t( effect.player, effect );
}

// Berserker's Juju ============================================

void items::berserkers_juju( special_effect_t& effect )
{
  struct berserkers_juju_t : public proc_spell_t
  {
    stat_buff_t* buff;
    const spell_data_t* spell;
    double amount;

    berserkers_juju_t( const special_effect_t& effect )
      : proc_spell_t( "berserkers_juju", effect.player, effect.driver(), effect.item )
    {
      const spell_data_t* spell = effect.item->player->find_spell( 274472 );
      std::string buff_name     = util::tokenized_name( spell );

      buff = make_buff<stat_buff_t>( effect.player, buff_name, spell );
    }

    void init() override
    {
      spell = player->find_spell( 274472 );
      // force to use -7 scaling type on spelldata value
      // Should probably add a check here for if spelldata changes to -7 to avoid double dipping
      amount = item_database::apply_combat_rating_multiplier( *item, spell->effectN( 1 ).average( item ) );
      proc_spell_t::init();
    }

    void execute() override
    {
      // Testing indicates buff amount is +-15% of spelldata (scaled with -7 type) amount
      double health_percent = ( std::max( player->resources.current[ RESOURCE_HEALTH ] * 1.0, 0.0 ) ) /
                              player->resources.max[ RESOURCE_HEALTH ] * 1.0;

      // Then apply +-15 to that scaled values
      double buff_amount = amount * ( 0.85 + ( 0.3 * ( 1 - health_percent ) ) );

      buff->stats[ 0 ].amount = buff_amount;

      if ( player->sim->debug )
        player->sim->out_debug.printf(
            "%s bersker's juju procs haste=%.0f of spelldata_raw_amount=%.0f spelldata_scaled_amount=%.0f",
            player->name(), buff_amount, spell->effectN( 1 ).average( item ), amount );

      buff->trigger();
    }
  };

  effect.execute_action = create_proc_action<berserkers_juju_t>( "berserkers_juju", effect );
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
                    ->set_stack_change_callback( [mastery_buff]( buff_t* b, int, int cur ) {
                      if ( cur == b->max_stack() )
                        mastery_buff->trigger();
                      else
                        mastery_buff->expire();
                    } );
  }

  timespan_t period = effect.driver()->effectN( 1 ).period();
  p->register_combat_begin( [crit_buff, period]( player_t* ) {
    make_repeating_event( crit_buff->sim, period, [crit_buff] {
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
    surging_burst_t( const special_effect_t& effect ) : aoe_proc_t( effect, "surging_burst", 288086, true )
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

    surging_waters_t( const special_effect_t& effect ) : proc_t( effect, "surging_waters", effect.driver() )
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
  effect.execute_action = create_proc_action<aoe_proc_t>( "yulons_fury", effect, "yulons_fury", 288282, true );
  effect.execute_action->travel_speed = effect.driver()->missile_speed();
}

// Variable Intensity Gigavolt Oscillating Reactor =======================

struct vigor_engaged_t : public special_effect_t
{
  // Phases of the buff
  enum oscillation : unsigned
  {
    ASCENDING = 0u,  // Ascending towards max stack
    MAX_STACK,       // Coasting at max stack
    DESCENDING,      // Descending towards zero stack
    INACTIVE,        // Hibernating at zero stack
    MAX_STATES
  };

  oscillation current_oscillation = oscillation::ASCENDING;
  unsigned ticks_at_oscillation   = 0u;
  std::array<unsigned, oscillation::MAX_STATES> max_ticks;
  std::array<oscillation, oscillation::MAX_STATES> transition_map = {{MAX_STACK, DESCENDING, INACTIVE, ASCENDING}};

  event_t* tick_event     = nullptr;
  stat_buff_t* vigor_buff = nullptr;
  buff_t* cooldown_buff   = nullptr;

  // Oscillation overload stuff
  const spell_data_t* overload_spell = nullptr;
  cooldown_t* overload_cd            = nullptr;
  buff_t* overload_buff              = nullptr;

  static std::string oscillation_str( oscillation s )
  {
    switch ( s )
    {
      case oscillation::ASCENDING:
        return "ascending";
      case oscillation::DESCENDING:
        return "descending";
      case oscillation::MAX_STACK:
        return "max_stack";
      case oscillation::INACTIVE:
        return "inactive";
      default:
        return "unknown";
    }
  }

  vigor_engaged_t( const special_effect_t& effect ) : special_effect_t( effect )
  {
    // Initialize max oscillations from spell data, buff-adjusting phases are -1 ticks, since the
    // transition event itself adjusts the buff stack count by one (up or down)
    max_ticks[ ASCENDING ] = max_ticks[ DESCENDING ] = as<int>( driver()->effectN( 2 ).base_value() - 1 );
    max_ticks[ MAX_STACK ] = max_ticks[ INACTIVE ] = as<int>( driver()->effectN( 3 ).base_value() );

    vigor_buff =
        ::create_buff<stat_buff_t>( effect.player, "vigor_engaged", effect.player->find_spell( 287916 ), effect.item );
    vigor_buff->add_stat( STAT_CRIT_RATING, driver()->effectN( 1 ).average( effect.item ) );

    cooldown_buff =
        ::create_buff<buff_t>( effect.player, "vigor_cooldown", effect.player->find_spell( 287967 ), effect.item );

    // Overload
    overload_spell = effect.player->find_spell( 287917 );
    overload_cd    = effect.player->get_cooldown( "oscillating_overload_" + ::util::to_string( overload_spell->id() ) );

    effect.player->register_on_arise_callback( effect.player, [this]() {
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

  void extend_oscillation( timespan_t by_seconds )
  {
    if ( player->sim->debug )
    {
      player->sim->out_debug.print( "{} {} oscillation overload, remains={}, new_remains={}", player->name(), name(),
                                    tick_event->remains(), tick_event->remains() + by_seconds );
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
      player->sim->out_debug.print( "{} {} transition from {} to {}", player->name(), name(),
                                    oscillation_str( current_oscillation ), oscillation_str( new_oscillation ) );
    }

    current_oscillation  = new_oscillation;
    ticks_at_oscillation = 0u;
  }

  void do_oscillation_transition()
  {
    if ( player->sim->debug )
    {
      player->sim->out_debug.print( "{} {} oscillation at {}, ticks={}, max_ticks={}", player->name(), name(),
                                    oscillation_str( current_oscillation ), ticks_at_oscillation,
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

    tick_event = make_event( player->sim, driver()->effectN( 1 ).period(), [this]() { oscillate(); } );

    if ( at_max_stack )
    {
      auto_overload( transition_map[ current_oscillation ] );
      transition_to( transition_map[ current_oscillation ] );
    }
  }

  // Randomize oscillation state to any of the four states, and any time inside the phase
  void randomize_oscillation()
  {
    oscillation phase = static_cast<oscillation>(
        static_cast<unsigned>( player->rng().range( oscillation::ASCENDING, oscillation::MAX_STATES ) ) );
    double used_time = player->rng().range( 0, max_ticks[ phase ] * driver()->effectN( 1 ).period().total_seconds() );
    int ticks        = static_cast<int>( used_time / driver()->effectN( 1 ).period().total_seconds() );
    double elapsed_tick_time = used_time - ticks * driver()->effectN( 1 ).period().total_seconds();
    double time_to_next_tick = driver()->effectN( 1 ).period().total_seconds() - elapsed_tick_time;

    if ( player->sim->debug )
    {
      player->sim->out_debug.print(
          "{} {} randomized oscillation, oscillation={}, used_time={}, "
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
        cooldown_buff->trigger(
            1, buff_t::DEFAULT_VALUE(), -1.0,
            max_ticks[ phase ] * driver()->effectN( 1 ).period() - timespan_t::from_seconds( used_time ) );
        break;
      default:
        break;
    }

    current_oscillation  = phase;
    ticks_at_oscillation = as<unsigned>( ticks );
    tick_event = make_event( player->sim, timespan_t::from_seconds( time_to_next_tick ), [this]() { oscillate(); } );
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
  auto it = range::find_if( e.player->special_effects,
                            [&e]( const special_effect_t* effect ) { return e.spell_id == effect->spell_id; } );

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

    oscillating_overload_t( player_t* p, ::util::string_view name, const spell_data_t* spell, const item_t* item )
      : buff_t( p, name, spell, item ), driver( nullptr )
    {
    }

    void execute( int stacks, double value, timespan_t duration ) override
    {
      buff_t::execute( stacks, value, duration );

      // Find the special effect for the on-equip on first execute of the on-use effect. Otherwise
      // we run into init issues.
      if ( !driver )
      {
        auto it = range::find_if( player->special_effects,
                                  []( special_effect_t* effect ) { return effect->spell_id == 287915; } );

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

    oscillating_overload_action_t( const special_effect_t& effect )
      : action_t( ACTION_OTHER, "oscillating_overload_driver", effect.player, effect.driver() ),
        buff( create_buff<oscillating_overload_t>( effect.player, "oscillating_overload", effect.driver(),
                                                   effect.item ) ),
        cooldown_buff(
            create_buff<buff_t>( effect.player, "vigor_cooldown", effect.player->find_spell( 287967 ), effect.item ) )
    {
      background = quiet = true;
      callbacks          = false;
    }

    result_e calculate_result( action_state_t* /* state */ ) const override
    {
      return RESULT_HIT;
    }

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
  everchill_anchor_constructor_t( unsigned iid, ::util::span<const slot_e> s )
    : item_targetdata_initializer_t( iid, s )
  {}

  // Create the everchill debuff to handle trinket icd
  void operator()( actor_target_data_t* td ) const override
  {
    const special_effect_t* effect = find_effect( td->source );
    if ( !effect )
    {
      td->debuff.everchill = make_buff( *td, "everchill_debuff" );
      return;
    }
    assert( !td->debuff.everchill );

    td->debuff.everchill = make_buff( *td, "everchill_debuff", effect->trigger() )->set_activated( false );
    td->debuff.everchill->reset();
  }
};

void items::everchill_anchor( special_effect_t& effect )
{
  struct everchill_t : public proc_spell_t
  {
    everchill_t( const special_effect_t& effect )
      : proc_spell_t( "everchill", effect.player, effect.player->find_spell( 289526 ), effect.item )
    {
    }
  };

  // An aoe ability will apply the dot on every target hit
  effect.proc_flags2_ = PF2_ALL_HIT;
  // Internal cd is handled on the debuff
  effect.cooldown_      = timespan_t::zero();
  effect.execute_action = create_proc_action<everchill_t>( "everchill", effect );

  new dbc_proc_callback_t( effect.player, effect );
}

// Ramping Amplitude Gigavolt Engine ======================================

void items::ramping_amplitude_gigavolt_engine( special_effect_t& effect )
{
  struct ramping_amplitude_event_t : event_t
  {
    buff_t* rage_buff;
    player_t* p;
    timespan_t prev_interval;

    ramping_amplitude_event_t( player_t* player, buff_t* buff, timespan_t interval )
      : event_t( *player, interval ), rage_buff( buff ), p( player ), prev_interval( interval )
    {
    }

    void execute() override
    {
      if ( rage_buff->check() && rage_buff->check() < rage_buff->max_stack() )
      {
        rage_buff->increment();
        make_event<ramping_amplitude_event_t>( sim(), p, rage_buff, prev_interval * 0.8 );
      }
    }
  };

  struct ramping_amplitude_gigavolt_engine_active_t : proc_spell_t
  {
    buff_t* rage_buff;

    ramping_amplitude_gigavolt_engine_active_t( special_effect_t& effect )
      : proc_spell_t( "ramping_amplitude_gigavolt_engine_active", effect.player, effect.driver(), effect.item )
    {
      rage_buff = make_buff<stat_buff_t>( effect.player, "r_a_g_e", effect.player->find_spell( 288156 ), effect.item );
      rage_buff->set_refresh_behavior( buff_refresh_behavior::DISABLED );
      rage_buff->cooldown = this->cooldown;
    }

    void execute() override
    {
      rage_buff->trigger();
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
    primal_rage_channel( const special_effect_t& effect )
      : proc_spell_t( "primal_rage", effect.player, effect.player->find_spell( 288267 ), effect.item )
    {
      channeled = hasted_ticks = true;
      interrupt_auto_attack = tick_zero = tick_on_application = false;

      tick_action = create_proc_action<aoe_proc_t>( "primal_rage_damage", effect, "primal_rage_damage",
                                                    effect.player->find_spell( 288269 ), true );
    }

    // Even though ticks are hasted, the duration is a fixed 4s
    timespan_t composite_dot_duration( const action_state_t* ) const override
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
      event_t::cancel( player->readying );
    }

    void last_tick( dot_t* d ) override
    {
      // Last_tick() will zero player_t::channeling if this action is being channeled, so check it
      // before calling the parent.
      auto was_channeling = player->channeling == this;

      proc_spell_t::last_tick( d );

      // Since Draught of Souls must be modeled as a channel (player cannot be allowed to perform
      // any actions for 3 seconds), we need to manually restart the player-ready event immediately
      // after the channel ends. This is because the channel is flagged as a background action,
      // which by default prohibits player-ready generation.
      if ( was_channeling && player->readying == nullptr )
      {
        // Due to the client not allowing the ability queue here, we have to wait
        // the amount of lag + how often the key is spammed until the next ability is used.
        // Modeling this as 2 * lag for now. Might increase to 3 * lag after looking at logs of people using the
        // trinket.
        timespan_t time = ( player->world_lag_override ? player->world_lag : sim->world_lag ) * 2.0;
        player->schedule_ready( time );
      }
    }
  };

  effect.execute_action = new primal_rage_channel( effect );
}

// Harbinger's Inscrutable Will ===========================================

void items::harbingers_inscrutable_will( special_effect_t& effect )
{
  struct oblivion_spear_t : public proc_t
  {
    timespan_t silence_duration;

    oblivion_spear_t( const special_effect_t& effect )
      : proc_t( effect, "oblivion_spear", 295393 ), silence_duration( effect.player->find_spell( 295395 )->duration() )
    {
      travel_speed = effect.player->find_spell( 295392 )->missile_speed();
    }

    void impact( action_state_t* s ) override
    {
      proc_t::impact( s );

      make_event( player->sim, time_to_travel, [this] {
        double roll    = rng().real();
        double silence = sim->bfa_opts.harbingers_inscrutable_will_silence_chance;
        double move    = sim->bfa_opts.harbingers_inscrutable_will_move_chance;

        // The actor didn't avoid the projectile and got silenced.
        if ( roll < silence )
        {
          // Copied from the stun raid event.
          player->buffs.stunned->increment();
          player->in_combat = true;
          player->stun();

          make_event( player->sim, silence_duration, [this] {
            player->buffs.stunned->decrement();
            if ( !player->buffs.stunned->check() && !player->channeling && !player->executing && !player->readying )
            {
              player->schedule_ready();
            }
          } );
        }
        // The actor avoided the projectile by moving.
        else if ( roll < silence + move )
        {
          player->moving();
        }
      } );
    }
  };

  effect.execute_action = create_proc_action<oblivion_spear_t>( "oblivion_spear", effect );
  // Stuns can now come from another source. Add an extra max stack to prevent trinket procs
  // from cancelling stun raid events and vice versa.
  // TODO: Is there a better way to solve this?
  effect.player->buffs.stunned->set_max_stack( 1 + effect.player->buffs.stunned->max_stack() );

  new dbc_proc_callback_t( effect.player, effect );
}

// Leggings of the Aberrant Tidesage ======================================

void items::leggings_of_the_aberrant_tidesage( special_effect_t& effect )
{
  // actual damage spell
  struct nimbus_bolt_t : public proc_t
  {
    nimbus_bolt_t( const special_effect_t& effect ) : proc_t( effect, "nimbus_bolt", 295811 )
    {
      // damage data comes from the driver
      base_dd_min = effect.driver()->effectN( 1 ).min( effect.item );
      base_dd_max = effect.driver()->effectN( 1 ).max( effect.item );
    }
  };

  struct storm_nimbus_proc_callback_t : public dbc_proc_callback_t
  {
    action_t* damage;

    storm_nimbus_proc_callback_t( const special_effect_t& effect )
      : dbc_proc_callback_t( effect.item, effect ),
        damage( create_proc_action<nimbus_bolt_t>( "storm_nimbus", effect ) )
    {
    }

    void execute( action_t*, action_state_t* state ) override
    {
      if ( rng().roll( damage->player->sim->bfa_opts.aberrant_tidesage_damage_chance ) )
      {
        // damage proc
        damage->set_target( state->target );
        damage->execute();
      }
      else
      {
        // heal - not implemented
      }
    }
  };

  new storm_nimbus_proc_callback_t( effect );
}

// Fa'thuul's Floodguards ==============================================

// Not tested against PTR - results _look_ maybe reasonable
// Somebody more experienced should take it for a spin

void items::fathuuls_floodguards( special_effect_t& effect )
{
  // damage spell is same as driver unlike aberrant leggings above
  struct drowning_tide_damage_t : public proc_t
  {
    drowning_tide_damage_t( const special_effect_t& effect ) : proc_t( effect, "drowning_tide", 295257 )
    {
      // damage data comes from the driver
      base_dd_min = effect.driver()->effectN( 1 ).min( effect.item );
      base_dd_max = effect.driver()->effectN( 1 ).max( effect.item );
    }
  };

  struct drowning_tide_proc_callback_t : public dbc_proc_callback_t
  {
    action_t* damage;

    drowning_tide_proc_callback_t( const special_effect_t& effect )
      : dbc_proc_callback_t( effect.item, effect ),
        damage( create_proc_action<drowning_tide_damage_t>( "drowning_tide", effect ) )
    {
    }

    void execute( action_t*, action_state_t* state ) override
    {
      if ( rng().roll( damage->player->sim->bfa_opts.fathuuls_floodguards_damage_chance ) )
      {
        // damage proc
        damage->set_target( state->target );
        damage->execute();
      }
      else
      {
        // heal - not implemented
      }
    }
  };

  new drowning_tide_proc_callback_t( effect );
}

// Grips of Forsaken Sanity ==============================================

// Not tested against PTR - results _look_ maybe reasonable
// Somebody more experienced should take it for a spin

void items::grips_of_forsaken_sanity( special_effect_t& effect )
{
  struct spiteful_binding_proc_callback_t : public dbc_proc_callback_t
  {
    spiteful_binding_proc_callback_t( const special_effect_t& effect ) : dbc_proc_callback_t( effect.item, effect )
    {
    }

    void execute( action_t* action, action_state_t* state ) override
    {
      if ( rng().roll( action->player->sim->bfa_opts.grips_of_forsaken_sanity_damage_chance ) )
      {
        dbc_proc_callback_t::execute( action, state );
      }
    }
  };

  new spiteful_binding_proc_callback_t( effect );
}

// Stormglide Steps ==============================================

void items::stormglide_steps( special_effect_t& effect )
{
  player_t* p = effect.player;

  buff_t* untouchable_buff = effect.create_buff();

  timespan_t period = timespan_t::from_seconds( 1.0 );
  p->register_combat_begin( [untouchable_buff, period]( player_t* ) {
    make_repeating_event( untouchable_buff->sim, period, [untouchable_buff] {
      if ( untouchable_buff->rng().roll( untouchable_buff->sim->bfa_opts.stormglide_steps_take_damage_chance ) )
        untouchable_buff->expire();
    } );
  } );

  new dbc_proc_callback_t( effect.player, effect );
}

// Idol of Indiscriminate Consumption =====================================

void items::idol_of_indiscriminate_consumption( special_effect_t& effect )
{
  // Healing isn't implemented
  struct indiscriminate_consumption_t : public proc_t
  {
    indiscriminate_consumption_t( const special_effect_t& effect )
      : proc_t( effect, "indiscriminate_consumption", 295962 )
    {
      aoe = 7;
    }
  };

  effect.execute_action = create_proc_action<indiscriminate_consumption_t>( "indiscriminate_consumption", effect );
}

// Lurker's Insidious Gift ================================================

void items::lurkers_insidious_gift( special_effect_t& effect )
{
  // Damage to the player isn't implemented

  buff_t* insidious_gift_buff =
      make_buff<stat_buff_t>( effect.player, "insidious_gift", effect.player->find_spell( 295408 ), effect.item );

  timespan_t duration_override = effect.player->sim->bfa_opts.lurkers_insidious_gift_duration;

  // If the overriden duration is out of bounds, yell at the user
  if ( duration_override > insidious_gift_buff->buff_duration() )
  {
    effect.player->sim->error(
        "{} Lurker's Insidious duration set higher than the buff's maximum duration, setting to {} seconds",
        effect.player->name(), insidious_gift_buff->buff_duration().total_seconds() );
  }
  // If the override is valid and different from 0, replace the buff's duration
  else if ( duration_override > 0_ms )
  {
    insidious_gift_buff->set_duration( duration_override );
  }

  effect.custom_buff = insidious_gift_buff;
}

// Abyssal Speaker's Gauntlets ==============================================

// Primarily implements an override to control how long the shield proc is expected to last

void items::abyssal_speakers_gauntlets( special_effect_t& effect )
{
  struct ephemeral_vigor_absorb_buff_t : absorb_buff_t
  {
    stat_buff_t* ephemeral_vigor_stat_buff;

    ephemeral_vigor_absorb_buff_t( const special_effect_t& effect )
      : absorb_buff_t( effect.player, "ephemeral_vigor_absorb", effect.driver()->effectN( 1 ).trigger(), effect.item )
    {
      ephemeral_vigor_stat_buff = make_buff<stat_buff_t>( effect.player, "ephemeral_vigor",
                                                          effect.driver()->effectN( 1 ).trigger(), effect.item );

      // Set the absorb value
      default_value = effect.driver()->effectN( 1 ).trigger()->effectN( 1 ).average( effect.item );

      timespan_t duration_override = effect.player->sim->bfa_opts.abyssal_speakers_gauntlets_shield_duration;

      // If the overriden duration is out of bounds, yell at the user
      if ( duration_override > buff_duration() )
      {
        effect.player->sim->error(
            "{} Abyssal Speaker's Gauntlets duration set higher than the buff's maximum duration, setting to {} "
            "seconds",
            effect.player->name(), buff_duration().total_seconds() );
      }
      // If the override is valid and different from 0, replace the buff's duration
      else if ( duration_override > 0_ms )
      {
        set_duration( duration_override );
      }
      // Manually set absorb school to chaos because autogeneration doesn't seem to catch it
      absorb_school = SCHOOL_CHAOS;
    }

    bool trigger( int stacks, double value, double chance, timespan_t duration ) override
    {
      bool success = absorb_buff_t::trigger( stacks, value, chance, duration );

      ephemeral_vigor_stat_buff->trigger();

      return success;
    }

    void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
    {
      absorb_buff_t::expire_override( expiration_stacks, remaining_duration );

      ephemeral_vigor_stat_buff->expire();
    }
  };

  effect.custom_buff = new ephemeral_vigor_absorb_buff_t( effect );

  new dbc_proc_callback_t( effect.player, effect );
}

// Trident of the Deep Ocean ==============================================

void items::trident_of_deep_ocean( special_effect_t& effect )
{
  struct custody_of_the_deep_absorb_buff_t : absorb_buff_t
  {
    stat_buff_t* custody_of_the_deep_stat_buff;

    custody_of_the_deep_absorb_buff_t( special_effect_t& effect )
      : absorb_buff_t( effect.player, "custody_of_the_deep_absorb", effect.driver()->effectN( 1 ).trigger(),
                       effect.item )
    {
      custody_of_the_deep_stat_buff = make_buff<stat_buff_t>( effect.player, "custody_of_the_deep",
                                                              effect.player->find_spell( 292653 ), effect.item );

      // Set the absorb value
      default_value = effect.driver()->effectN( 1 ).trigger()->effectN( 1 ).average( effect.item );

      timespan_t duration_override = effect.player->sim->bfa_opts.trident_of_deep_ocean_duration;

      // If the overriden duration is out of bounds, yell at the user
      if ( duration_override > buff_duration() )
      {
        effect.player->sim->error(
            "{} Trident of deep ocan duration set higher than the buff's maximum duration, setting to {} seconds",
            effect.player->name(), buff_duration().total_seconds() );
      }
      // If the override is valid and different from 0, replace the buff's duration
      else if ( duration_override > 0_ms )
      {
        set_duration( duration_override );
      }

      // TODO 2019-04-03(melekus): have the effect only absorb up to 25% of the incoming damage
      // I don't think such an option currently exists in simc, and it's pretty minor considering the low size of the
      // absorb in this case Also TODO: have the absorb scale with versatility

      // Another TODO: Have an option to let DPS players proc the effect if they want.
      // Need testing to see if the effect can reliably proc for non-tanks in the first place.
    }

    void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
    {
      absorb_buff_t::expire_override( expiration_stacks, remaining_duration );

      custody_of_the_deep_stat_buff->trigger();
    }
  };

  effect.custom_buff = new custody_of_the_deep_absorb_buff_t( effect );

  new dbc_proc_callback_t( effect.player, effect );
}

// Legplates of Unbound Anguish ===========================================

void items::legplates_of_unbound_anguish( special_effect_t& effect )
{
  struct unbound_anguish_cb_t : public dbc_proc_callback_t
  {
    unbound_anguish_cb_t( const special_effect_t& effect ) : dbc_proc_callback_t( effect.player, effect )
    {
    }

    void trigger( action_t* a, action_state_t* s ) override
    {
      if ( rng().roll( a->sim->bfa_opts.legplates_of_unbound_anguish_chance ) )
      {
        dbc_proc_callback_t::trigger( a, s );
      }
    }
  };

  // TODO: wait for spelldata regen and change ID to 295428

  action_t* unbound_anguish_damage = create_proc_action<proc_t>( "unbound_anguish", effect, "unbound_anguish", 295427 );

  unbound_anguish_damage->base_dd_min = unbound_anguish_damage->base_dd_max =
      unbound_anguish_damage->data().effectN( 1 ).average( effect.item );

  // effect.execute_action = create_proc_action<proc_t>( "unbound_anguish", effect, "unbound_anguish", 295428 );
  effect.execute_action = unbound_anguish_damage;

  new unbound_anguish_cb_t( effect );
}

// Benthic Armor Aberrations Damage% ======================================

void items::damage_to_aberrations( special_effect_t& effect )
{
  if ( !effect.player->sim->bfa_opts.nazjatar )
    return;

  auto buff = buff_t::find( effect.player, "damage_to_aberrations" );
  // TODO: Multiple items stack or not?
  if ( buff )
  {
    buff->default_value += effect.driver()->effectN( 1 ).percent();
  }
  else
  {
    buff = make_buff( effect.player, "damage_to_aberrations", effect.driver() );
    buff->set_default_value( effect.driver()->effectN( 1 ).percent() );
    buff->set_quiet( true );

    effect.player->buffs.damage_to_aberrations = buff;
    effect.player->register_combat_begin( buff );
  }

  effect.type = SPECIAL_EFFECT_NONE;
}

// Benthic Armor Exploding Pufferfish =====================================

void items::exploding_pufferfish( special_effect_t& effect )
{
  if ( !effect.player->sim->bfa_opts.nazjatar )
    return;

  struct pufferfish_cb_t : public dbc_proc_callback_t
  {
    const spell_data_t* summon_spell;

    pufferfish_cb_t( const special_effect_t& effect )
      : dbc_proc_callback_t( effect.player, effect ), summon_spell( effect.player->find_spell( 305314 ) )
    {
    }

    void execute( action_t*, action_state_t* state ) override
    {
      make_event<ground_aoe_event_t>( *listener->sim, listener,
                                      ground_aoe_params_t()
                                          .target( state->target )
                                          .pulse_time( summon_spell->duration() )
                                          .action( proc_action )
                                          .n_pulses( 1u ) );
    }
  };

  effect.execute_action =
      create_proc_action<aoe_proc_t>( "exploding_pufferfish", effect, "exploding_pufferfish", 305315, true );
  effect.execute_action->aoe         = -1;
  effect.execute_action->base_dd_min = effect.execute_action->base_dd_max =
      effect.driver()->effectN( 1 ).average( effect.item );

  new pufferfish_cb_t( effect );
}

// Benthic Armor Fathom Hunter ============================================

void items::fathom_hunter( special_effect_t& effect )
{
  if ( !effect.player->sim->bfa_opts.nazjatar )
    return;

  auto buff = buff_t::find( effect.player, "fathom_hunter" );
  if ( !buff )
  {
    buff = make_buff( effect.player, "fathom_hunter", effect.driver() );
    buff->set_default_value( effect.driver()->effectN( 1 ).percent() );
    buff->set_quiet( true );

    effect.player->buffs.fathom_hunter = buff;
    effect.player->register_combat_begin( buff );
  }

  effect.type = SPECIAL_EFFECT_NONE;
}

// Nazjatar/Eternal Palace check ==========================================

void items::nazjatar_proc_check( special_effect_t& effect )
{
  if ( !effect.player->sim->bfa_opts.nazjatar )
    return;

  if ( effect.spell_id == 304715 )  // Tidal Droplet is not an AoE
  {
    effect.execute_action      = create_proc_action<proc_spell_t>( "tidal_droplet", effect );
    effect.execute_action->aoe = 0;
  }

  effect.proc_flags_ = effect.driver()->proc_flags() & ~PF_ALL_HEAL;
  new dbc_proc_callback_t( effect.player, effect );
}

// Storm of the Eternal ===================================================
void items::storm_of_the_eternal_arcane_damage( special_effect_t& effect )
{
  struct sote_arcane_damage_t : public proc_t
  {
    sote_arcane_damage_t( const special_effect_t& e )
      : proc_t( e, "storm_of_the_eternal", e.player->find_spell( 303725 ) )
    {
      aoe         = 0;
      base_dd_min = base_dd_max = e.driver()->effectN( 1 ).average( e.item );
    }

    void execute() override
    {
      size_t index = static_cast<size_t>( rng().range( 0, as<double>( target_list().size() ) ) );
      set_target( target_list()[ index ] );

      proc_t::execute();
    }
  };

  auto action = create_proc_action<sote_arcane_damage_t>( "storm_of_the_eternal", effect );

  timespan_t storm_period = effect.player->find_spell( 303722 )->duration();    // every 2 minutes
  timespan_t storm_dur    = effect.player->find_spell( 303723 )->duration();    // over 10 seconds
  unsigned storm_hits     = effect.player->find_spell( 303724 )->max_stacks();  // 3 hits

  effect.player->register_combat_begin( [action, storm_period, storm_dur, storm_hits]( player_t* ) {
    make_repeating_event( action->sim, storm_period, [action, storm_dur, storm_hits] {
      for ( unsigned i = 0u; i < storm_hits; i++ )
      {
        auto delay = timespan_t::from_seconds( action->sim->rng().range( 0.0, storm_dur.total_seconds() ) );
        make_event( action->sim, delay, [action] { action->schedule_execute(); } );
      }
    } );
  } );
}

void items::storm_of_the_eternal_stats( special_effect_t& effect )
{
  stat_e stat;
  switch ( effect.spell_id )
  {
    case 303734:
      stat = STAT_HASTE_RATING;
      break;
    case 303735:
      stat = STAT_CRIT_RATING;
      break;
    default:
      return;
  }

  stat_buff_t* stat_buff = debug_cast<stat_buff_t*>( buff_t::find( effect.player, "storm_of_the_eternal" ) );
  if ( !stat_buff )
    stat_buff = make_buff<stat_buff_t>( effect.player, "storm_of_the_eternal", effect.player->find_spell( 303723 ) );

  for ( const auto& s : stat_buff->stats )
  {
    if ( s.stat == stat )
      return;
  }

  stat_buff->add_stat( stat, effect.driver()->effectN( 1 ).average( effect.item ) *
                                 effect.player->sim->bfa_opts.storm_of_the_eternal_ratio );

  timespan_t period = effect.player->find_spell( 303722 )->duration();
  effect.player->register_combat_begin( [stat_buff, period]( player_t* ) {
    make_repeating_event( stat_buff->sim, period, [stat_buff] { stat_buff->trigger(); } );
  } );
}

// Highborne Compendium of Storms =========================================

void items::highborne_compendium_of_storms( special_effect_t& effect )
{
  struct hcos_cb_t : public dbc_proc_callback_t
  {
    hcos_cb_t( const special_effect_t& effect ) : dbc_proc_callback_t( effect.player, effect )
    {
    }

    void execute( action_t* /* a */, action_state_t* state ) override
    {
      proc_action->set_target( target( state ) );
      proc_action->execute();
      proc_buff->trigger();
    }
  };

  auto buff = buff_t::find( effect.player, "highborne_compendium_of_storms" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "highborne_compendium_of_storms",
                                   effect.driver()->effectN( 2 ).trigger(), effect.item );
  }

  effect.custom_buff = buff;

  new hcos_cb_t( effect );
}

// Highborne Compendium of Sundering ======================================

void items::highborne_compendium_of_sundering( special_effect_t& effect )
{
  struct volcanic_eruption_t : public unique_gear::proc_spell_t
  {
    volcanic_eruption_t( const special_effect_t& effect )
      : proc_spell_t( "volcanic_eruption", effect.player, effect.player->find_spell( 300907 ), effect.item )
    {
      aoe         = -1;
      base_dd_min = base_dd_max = effect.driver()->effectN( 2 ).average( effect.item );
    }
  };

  struct volcanic_pressure_t : public unique_gear::proc_spell_t
  {
    action_t* eruption;

    volcanic_pressure_t( const special_effect_t& effect )
      : proc_spell_t( "volcanic_pressure", effect.player, effect.player->find_spell( 300832 ), effect.item ),
        eruption( create_proc_action<volcanic_eruption_t>( "volcanic_eruption", effect ) )
    {
      base_td = effect.driver()->effectN( 1 ).average( effect.item );

      add_child( eruption );
    }

    // TODO: How does the Volcanic Eruption work in game?
    void impact( action_state_t* s ) override
    {
      // Trigger explosion first
      auto dot = get_dot( s->target );
      if ( dot->current_stack() == dot->max_stack - 1 )
      {
        eruption->set_target( s->target );
        eruption->execute();
        dot->cancel();
      }
      else
      {
        proc_spell_t::impact( s );
      }
    }
  };

  effect.execute_action = create_proc_action<volcanic_pressure_t>( "volcanic_pressure", effect );

  new dbc_proc_callback_t( effect.player, effect );
}

// Neural Synapse Enhancer

void items::neural_synapse_enhancer( special_effect_t& effect )
{
  effect.disable_action();
}

/**Shiver Venom Relic
 * Equip: Shiver Venom (id=301576)
 * - Applies a stacking dot (effect#1->trigger id=301624), 0tick, hasted, can crit
 * On-use: Venomous Shiver (id=301834)
 * - Consume dots to do damage (id=305290), damage value in Equip driver->effect#1
 */
void items::shiver_venom_relic_onuse( special_effect_t& effect )
{
  struct venomous_shivers_t : public proc_t
  {
    venomous_shivers_t( const special_effect_t& e ) : proc_t( e, "venomous_shivers", e.player->find_spell( 305290 ) )
    {
      aoe         = -1;
      base_dd_max = base_dd_min = e.player->find_spell( 301576 )->effectN( 1 ).average( e.item );
    }

    bool ready() override
    {
      if ( !target || !proc_t::ready() )
        return false;

      dot_t* sv_dot;

      for ( auto t : sim->target_non_sleeping_list )
      {
        if ( ( sv_dot = t->find_dot( "shiver_venom", player ) ) && sv_dot->is_ticking() )
          return true;
      }
      return false;
    }

    double action_multiplier() const override
    {
      dot_t* sv_dot = target->find_dot( "shiver_venom", player );
      if ( !sv_dot )
        sim->print_debug( "venomous_shivers->action_multiplier() cannot find shiver_venom dot on {}", target->name() );

      return proc_t::action_multiplier() * ( sv_dot ? sv_dot->current_stack() : 0.0 );
    }

    void execute() override
    {
      proc_t::execute();

      dot_t* sv_dot = target->find_dot( "shiver_venom", player );
      if ( !sv_dot )
        sim->print_debug( "venomous_shivers->execute() cannot find shiver_venom dot on {}", target->name() );
      else
        sv_dot->cancel();
    }
  };

  effect.execute_action = create_proc_action<venomous_shivers_t>( "venomous_shivers", effect );
}

void items::shiver_venom_relic_equip( special_effect_t& effect )
{
  struct shiver_venom_t : proc_t
  {
    action_t* onuse;

    shiver_venom_t( const special_effect_t& e )
      : proc_t( e, "shiver_venom", e.trigger() ), onuse( e.player->find_action( "venomous_shivers" ) )
    {
      if ( onuse )
        add_child( onuse );
    }

    timespan_t calculate_dot_refresh_duration( const dot_t*, timespan_t t ) const override
    {
      return t;  // dot doesn't pandemic
    }
  };

  effect.execute_action = create_proc_action<shiver_venom_t>( "shiver_venom", effect );

  new dbc_proc_callback_t( effect.player, effect );
}

/**Leviathan's Lure
 * driver id=302773, unknown initial rppm (20 in data may be the MAX rppm)
 * damage id=302763
 * tooltip refers to driver->effect#2 for damage value, but effect doesn't exist and instead effect#1 is displayed
 *
 * Luminous Algae secondary proc driver id=302776
 * debuff is driver->trigger id=302775
 * presumably increases main driver rppm by debuff->effect#1
 */
struct luminous_algae_constructor_t : public item_targetdata_initializer_t
{
  luminous_algae_constructor_t( unsigned iid, ::util::span<const slot_e> s )
    : item_targetdata_initializer_t( iid, s )
  {}

  const special_effect_t* find_effect( player_t* player ) const override
  {
    auto it = range::find_if( player->special_effects,
                              []( const special_effect_t* effect ) { return effect->spell_id == 302776; } );

    if ( it != player->special_effects.end() )
      return *it;

    return nullptr;
  }

  void operator()( actor_target_data_t* td ) const override
  {
    const special_effect_t* effect = find_effect( td->source );
    if ( !effect )
    {
      td->debuff.luminous_algae = make_buff( *td, "luminous_algae_debuff" );
      return;
    }
    assert( !td->debuff.luminous_algae );

    td->debuff.luminous_algae = make_buff( *td, "luminous_algae_debuff", effect->trigger() );
    td->debuff.luminous_algae->set_max_stack( 1 );  // Data says 999, but logs show only 1 stack
    td->debuff.luminous_algae->set_default_value( effect->trigger()->effectN( 1 ).percent() );
    td->debuff.luminous_algae->set_activated( false );
    td->debuff.luminous_algae->reset();
  }
};

void items::leviathans_lure( special_effect_t& effect )
{
  struct leviathan_chomp_cb_t : public dbc_proc_callback_t
  {
    leviathan_chomp_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e )
    {
    }

    void trigger( action_t* a, action_state_t* s ) override
    {
      auto td = listener->get_target_data( s->target );
      assert( td );
      assert( td->debuff.luminous_algae );

      // adjust the rppm before triggering
      rppm->set_modifier( 1.0 + td->debuff.luminous_algae->check_value() );
      dbc_proc_callback_t::trigger( a, s );
    }

    void execute( action_t* a, action_state_t* s ) override
    {
      auto td = listener->get_target_data( s->target );
      assert( td );
      assert( td->debuff.luminous_algae );
      td->debuff.luminous_algae->expire();  // debuff removed on execute, not impact

      dbc_proc_callback_t::execute( a, s );
    }
  };

  struct luminous_algae_cb_t : public dbc_proc_callback_t
  {
    luminous_algae_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e )
    {
    }

    void execute( action_t*, action_state_t* s ) override
    {
      auto td = listener->get_target_data( s->target );
      assert( td );
      assert( td->debuff.luminous_algae );
      td->debuff.luminous_algae->trigger();
    }
  };

  // secondary (luminous algae) proc effect
  auto second      = new special_effect_t( effect.player );
  second->type     = effect.type;
  second->source   = effect.source;
  second->spell_id = 302776;

  effect.player->special_effects.push_back( second );
  new luminous_algae_cb_t( *second );

  // primary (leviathan chomp) proc
  auto action           = create_proc_action<proc_t>( "leviathan_chomp", effect, "leviathan_chomp", 302763 );
  action->base_dd_min   = effect.driver()->effectN( 1 ).average( effect.item );
  action->base_dd_max   = effect.driver()->effectN( 1 ).average( effect.item );
  effect.execute_action = action;

  effect.ppm_ = -( effect.player->sim->bfa_opts.leviathans_lure_base_rppm );
  new leviathan_chomp_cb_t( effect );
}

/**Aquipotent Nautilus
 * driver id=306146
 * driver->trigger id=302550 does not contain typical projectile or damage related info.
 * has 12s duration of unknown function. possibly time limit to 'catch' returning wave?
 * frontal wave applies dot id=302580, tick damage value in id=302579->effect#1
 * id=302579->effect#2 contains cd reduction (in seconds) for catching the returning wave.
 *
 * TODO: In-game testing suggests the dot follows old tick_zero behavior, i.e. a target in between the main target can
 * be hit by the outgoing wave and then hit again by the returning wave, and both the outgoing and returnig wave will
 * deal tick zero damage. Add a way to model this behavior if it becomes relevant.
 */
void items::aquipotent_nautilus( special_effect_t& effect )
{
  struct surging_flood_dot_t : public proc_t
  {
    cooldown_t* cd;
    cooldown_t* cdgrp;
    timespan_t reduction;

    surging_flood_dot_t( const special_effect_t& e )
      : proc_t( e, "surging_flood", e.player->find_spell( 302580 ) ),
        cd( e.player->get_cooldown( e.cooldown_name() ) ),
        cdgrp( e.player->get_cooldown( e.cooldown_group_name() ) ),
        reduction( timespan_t::from_seconds( e.player->find_spell( 302579 )->effectN( 2 ).base_value() ) )
    {
      aoe     = -1;
      base_td = e.player->find_spell( 302579 )->effectN( 1 ).average( e.item );

      // In-game testing suggests 10yd/s speed
      travel_speed = 10.0;
    }

    void impact( action_state_t* s ) override
    {
      proc_t::impact( s );

      if ( s->chain_target == 0 )
      {
        make_event( player->sim, time_to_travel, [this] {
          if ( rng().real() < sim->bfa_opts.aquipotent_nautilus_catch_chance )
          {
            sim->print_debug( "surging_flood return wave caught, adjusting cooldown from {} to {}", cd->remains(),
                              cd->remains() + reduction );
            cd->adjust( reduction );
            cdgrp->adjust( reduction );
          }
        } );
      }
    }
  };

  effect.execute_action = create_proc_action<surging_flood_dot_t>( "surging_flood", effect );
}

/**Za'qul's Portal Key
 * driver id=302696, 1.5rppm to create void tear (driver->trigger id=303104)
 * void tear id=303104, lasts 30s, move within 4yd to create portal
 * portal id=302702, lasts 10s, while open gives int buff
 * int buff id=302960, stat amount held in driver->effect#1
 */
void items::zaquls_portal_key( special_effect_t& effect )
{
  // How long void tear is up for. Seems unncessary atm so commented out.
  // timespan_t tear_duration = effect.driver()->effectN( 1 ).trigger()->duration();

  struct void_negotiation_cb_t : public dbc_proc_callback_t
  {
    buff_t* buff;

    void_negotiation_cb_t( const special_effect_t& e, buff_t* b ) : dbc_proc_callback_t( e.player, e ), buff( b )
    {
    }

    void execute( action_t*, action_state_t* ) override
    {
      // Approximate player behavior of waiting for the next gcd (or current cast to finish), then spending a gcd to
      // move and open the portal
      timespan_t delay =
          listener->gcd_ready - listener->sim->current_time() +
          ( listener->last_foreground_action ? listener->last_foreground_action->time_to_execute : 0_ms ) +
          listener->base_gcd * listener->gcd_current_haste_value;

      listener->sim->print_debug( "za'qul's portal key portal spawned, void_negotiation scheduled in {} at {}", delay,
                                  listener->sim->current_time() + delay );
      make_event( listener->sim, delay, [this] {
        // Portals spawn close enough that distance moving seems unnecessary, and an instance of moving() will suffice
        if ( rng().real() < listener->sim->bfa_opts.zaquls_portal_key_move_chance )
          listener->moving();

        buff->trigger();
      } );
    }
  };

  auto buff = buff_t::find( effect.player, "void_negotiation" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "void_negotiation", effect.player->find_spell( 302960 ) )
               ->add_stat( STAT_INTELLECT, effect.driver()->effectN( 1 ).average( effect.item ) );
    // TODO: determine difficulty in staying inside the portal for the buff. If it's an issue, may need to create an
    // option to set the buff uptime. For now assuming this is a non-issue and you can maintain the buff for the full
    // 10s.
    buff->set_duration( effect.player->find_spell( 302702 )->duration() );
    buff->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );
  }

  new void_negotiation_cb_t( effect, buff );
}

// Vision of Demise =======================================================

void items::vision_of_demise( special_effect_t& effect )
{
  struct vision_of_demise_t : public proc_spell_t
  {
    int divisor;
    stat_buff_t* buff;
    double bonus;

    vision_of_demise_t( const special_effect_t& effect, stat_buff_t* b )
      : proc_spell_t( "vision_of_demise", effect.player, effect.driver() ),
        divisor( as<int>( effect.driver()->effectN( 1 ).base_value() ) ),
        buff( b ),
        bonus( effect.player->find_spell( 303455 )->effectN( 2 ).average( effect.item ) )
    {
    }

    void execute() override
    {
      proc_spell_t::execute();

      double pct          = 100 - target->health_percentage();
      unsigned multiplier = static_cast<unsigned>( pct / divisor );

      buff->stats.back().amount = multiplier * bonus;

      buff->trigger();
    }
  };

  stat_buff_t* buff = debug_cast<stat_buff_t*>( buff_t::find( effect.player, "vision_of_demise" ) );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "vision_of_demise", effect.player->find_spell( 303431 ) );
    // Add the bonus haste as another haste rating buff, set to 0. Action will adjust
    buff->add_stat( STAT_HASTE_RATING, effect.player->find_spell( 303455 )->effectN( 1 ).average( effect.item ) );
    buff->add_stat( STAT_HASTE_RATING, 0 );
  }

  effect.execute_action = create_proc_action<vision_of_demise_t>( "vision_of_demise", effect, buff );
}

/**Azshara's Font of Power
 * driver id=296971, 4s duration channel, periodic triggers latent arcana (driver->trigger id=296962) every 1s
 * latent arcana id=296962, 6s fully extending duration stat buff.
 * unknown id=305190, maybe used to hold # of channel tics?
 *
 * TODO: Implement getting 6s of buff upon immediately cancelling channel if such a use case arises
 */
void items::azsharas_font_of_power( special_effect_t& effect )
{
  struct latent_arcana_channel_t : public proc_t
  {
    buff_t* buff;
    action_t* use_action;  // if this exists, then we're prechanneling via the APL

    latent_arcana_channel_t( const special_effect_t& e, buff_t* b )
      : proc_t( e, "latent_arcana", e.driver() ), buff( b ), use_action( nullptr )
    {
      effect    = &e;
      channeled = true;
      harmful = hasted_ticks = false;

      for ( auto a : player->action_list )
      {
        if ( a->action_list && a->action_list->name_str == "precombat" && a->name_str == "use_item_" + item->name_str )
        {
          a->harmful = harmful;  // pass down harmful to allow action_t::init() precombat check bypass
          use_action = a;
          use_action->base_execute_time = 4_s;
          break;
        }
      }
    }

    void precombat_buff()
    {
      timespan_t time = sim->bfa_opts.font_of_power_precombat_channel;

      if ( time == 0_ms )  // No global override, check for an override from an APL variable
      {
        for ( auto v : player->variables )
        {
          if ( v->name_ == "font_of_power_precombat_channel" )
          {
            time = timespan_t::from_seconds( v->value() );
            break;
          }
        }
      }

      // if ( time == 0_ms )  // No options override, first apply any spec-based hardcoded timings
      //{
      //  switch ( player->specialization() )
      //  {
      //    // case DRUID_BALANCE: time = 7.5_s; break;
      //    default: break;
      //  }
      //}

      // shared cd (other trinkets & on-use items)
      auto cdgrp = player->get_cooldown( effect->cooldown_group_name() );

      if ( time == 0_ms )  // No hardcoded override, so dynamically calculate timing via the precombat APL
      {
        time     = 4_s;  // base 4s channel for full effect
        auto apl = player->precombat_action_list;

        auto it = range::find( apl, use_action );
        if ( it == apl.end() )
        {
          sim->print_debug( "WARNING: Precombat /use_item for Font of Power exists but not found in precombat APL!" );
          return;
        }

        if ( cdgrp )
          cdgrp->start( 1_ms );  // tap the shared group cd so we can get accurate action_ready() checks

        // add cast time or gcd for any following precombat action
        std::for_each( it + 1, apl.end(), [&time, this]( action_t* a ) {
          if ( a->action_ready() )
          {
            timespan_t delta =
                std::max( std::max( a->base_execute_time, a->trigger_gcd ) * a->composite_haste(), a->min_gcd );
            sim->print_debug( "PRECOMBAT: Azshara's Font of Power prechannel timing pushed by {} for {}", delta,
                              a->name() );
            time += delta;

            return a->harmful;  // stop processing after first valid harmful spell
          }
          return false;
        } );
      }

      // how long you channel for (rounded down to seconds)
      auto channel = std::min( 4_s, timespan_t::from_seconds( static_cast<int>( time.total_seconds() ) ) );
      // total duration of the buff from channeling
      auto total = buff->buff_duration() * ( channel.total_seconds() + 1 );
      // actual duration of the buff you'll get in combat
      auto actual = total + channel - time;
      // cooldown on effect/trinket at start of combat
      auto cd_dur = cooldown->duration - time;
      // shared cooldown at start of combat
      auto cdgrp_dur = std::max( 0_ms, effect->cooldown_group_duration() - time );

      sim->print_debug(
          "PRECOMBAT: Azshara's Font of Power started {}s before combat via {}, channeled for {}s, {}s in-combat buff",
          time, use_action ? "APL" : "BFA_OPT", channel, actual );

      buff->trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, actual );

      if ( use_action )  // from the apl, so cooldowns will be started by use_item_t. adjust. we are still in precombat.
      {
        make_event( *sim, [this, time, cdgrp] {  // make an event so we adjust after cooldowns are started
          cooldown->adjust( -time );

          if ( use_action )
            use_action->cooldown->adjust( -time );

          cdgrp->adjust( -time );
        } );
      }
      else  // via bfa. option override, start cooldowns. we are in-combat.
      {
        cooldown->start( cd_dur );

        if ( use_action )
          use_action->cooldown->start( cd_dur );

        if ( cdgrp_dur > 0_ms )
          cdgrp->start( cdgrp_dur );
      }
    }

    timespan_t tick_time( const action_state_t* ) const override
    {
      return base_tick_time;
    }

    void trigger_dot( action_state_t* s ) override
    {
      if ( player->in_combat )  // only trigger channel 'dot' in combat
      {
        proc_t::trigger_dot( s );
      }
    }

    void execute() override
    {
      proc_t::execute();

      if ( player->in_combat )  // only channel in-combat
      {
        event_t::cancel( player->readying );
        player->reset_auto_attacks( data().duration() );
      }
      else  // if precombat...
      {
        if ( use_action )  // ...and use_item exists in the precombat apl
        {
          precombat_buff();
        }
      }
    }

    void tick( dot_t* d ) override
    {
      proc_t::tick( d );

      buff->trigger( 1, buff_t::DEFAULT_VALUE(), 1.0,
                     ( d->current_tick == 1 ? buff->buff_duration() : base_tick_time ) + buff->buff_duration() );
    }

    void last_tick( dot_t* d ) override
    {
      bool was_channeling = player->channeling == this;
      proc_t::last_tick( d );

      if ( was_channeling && !player->readying )
      {
        player->schedule_ready( rng().gauss( sim->channel_lag, sim->channel_lag_stddev ) );
      }
    }
  };

  auto buff = static_cast<stat_buff_t*>( buff_t::find( effect.player, "latent_arcana" ) );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "latent_arcana", effect.trigger() );
    buff->add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ),
                    effect.trigger()->effectN( 1 ).average( effect.item ) );
    buff->set_refresh_behavior( buff_refresh_behavior::EXTEND );
  }

  auto action = new latent_arcana_channel_t( effect, buff );

  effect.execute_action = action;
  effect.disable_buff();

  // pre-combat channeling hack via bfa. options
  if ( effect.player->sim->bfa_opts.font_of_power_precombat_channel > 0_ms )  // option is set
  {
    effect.player->register_combat_begin( [action]( player_t* ) {
      if ( !action->use_action )  // no use_item in precombat apl, so we apply the buff on combat start
        action->precombat_buff();
    } );
  }
}

// Arcane Tempest =========================================================

void items::arcane_tempest( special_effect_t& effect )
{
  struct arcane_tempest_t : public proc_spell_t
  {
    buff_t* buff;

    arcane_tempest_t( const special_effect_t& effect, buff_t* b )
      : proc_spell_t( "arcane_tempest", effect.player, effect.player->find_spell( 302774 ), effect.item ), buff( b )
    {
    }

    void execute() override
    {
      proc_spell_t::execute();

      if ( num_targets_hit == 1 )
      {
        buff->trigger();
      }
    }
  };

  struct arcane_tempest_buff_t : public buff_t
  {
    arcane_tempest_buff_t( const special_effect_t& e ) : buff_t( e.player, "arcane_tempest", e.trigger(), e.item )
    {
      set_refresh_behavior( buff_refresh_behavior::DISABLED );
      set_tick_on_application( true );
      set_tick_time_callback( []( const buff_t* b, unsigned /* current_tick */ ) {
        timespan_t amplitude = b->data().effectN( 1 ).period();

        // TODO: What's the speedup multiplier?
        return amplitude * ( 1.0 / ( 1.0 + ( b->current_stack - 1 ) * 0.5 ) );
      } );
    }

    // Custom stacking logic, ticks are not going to increase stacks
    bool freeze_stacks() override
    {
      return true;
    }
  };

  auto buff = buff_t::find( effect.player, "arcane_tempest" );
  if ( !buff )
  {
    buff = make_buff<arcane_tempest_buff_t>( effect );
  }

  auto action = create_proc_action<arcane_tempest_t>( "arcane_tempest", effect, buff );

  buff->set_tick_callback( [action]( buff_t* buff, int /* current_tick */, timespan_t /* tick_time */ ) {
    action->set_target( buff->source->target );
    action->execute();
  } );

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.player, effect );
}

/**Anu-Azshara, Staff of the Eternal
 * id=302986: driver, effect#1=damage, effect#2=hp/mana %threshold to trigger damage
 * id=303017: driver->trigger#1, counter aura
 * id=302988: unknown, possibly driver for the damage triggered when below %threshold
 * id=302995: damage aoe projectile, damage = driver->trigger#1->stack * driver->effect#1
 *
 * TODO: Implement method to trigger the damage via APL
 */
void items::anuazshara_staff_of_the_eternal( special_effect_t& effect )
{
  struct prodigys_potency_unleash_t : public proc_t
  {
    buff_t* buff;
    buff_t* lockout;

    prodigys_potency_unleash_t( const special_effect_t& e, buff_t* b, buff_t* lock )
      : proc_t( e, "prodigys_potency", e.player->find_spell( 302995 ) ), buff( b ), lockout( lock )
    {
      base_dd_min = base_dd_max = e.driver()->effectN( 1 ).average( e.item );
    }

    double action_multiplier() const override
    {
      return proc_t::action_multiplier() * buff->stack();
    }

    void execute() override
    {
      lockout->trigger();
      proc_t::execute();
      sim->print_debug( "anu-azshara potency unleashed at {} stacks!", buff->stack() );
      buff->expire();
    }
  };

  struct prodigys_potency_cb_t : dbc_proc_callback_t
  {
    buff_t* lockout;

    prodigys_potency_cb_t( const special_effect_t& e, buff_t* b ) : dbc_proc_callback_t( e.player, e ), lockout( b )
    {
    }

    void trigger( action_t* a, action_state_t* s ) override
    {
      if ( lockout->check() )
        return;

      dbc_proc_callback_t::trigger( a, s );
    }
  };

  auto lockout = buff_t::find( effect.player, "arcane_exhaustion" );
  if ( !lockout )
    lockout = make_buff( effect.player, "arcane_exhaustion", effect.player->find_spell( 304482 ) );

  auto buff = buff_t::find( effect.player, "prodigys_potency" );
  if ( !buff )
  {
    buff = make_buff( effect.player, "prodigys_potency", effect.trigger() );
    buff->set_max_stack( 255 );
  }

  effect.custom_buff = buff;
  new prodigys_potency_cb_t( effect, lockout );

  auto action = create_proc_action<prodigys_potency_unleash_t>( "prodigys_potency", effect, buff, lockout );
  auto time   = effect.player->sim->bfa_opts.anuazshara_unleash_time;

  if ( time > 0_ms )
  {
    effect.player->register_combat_begin( [effect, action, time]( player_t* ) {
      make_event( *effect.player->sim, time, [action]() { action->execute(); } );
    } );
  }
}

// Shiver Venom Crossbow ==================================================

void items::shiver_venom_crossbow( special_effect_t& effect )
{
  struct shivering_bolt_t : public proc_spell_t
  {
    shivering_bolt_t( const special_effect_t& effect )
      : proc_spell_t( "shivering_bolt", effect.player, effect.player->find_spell( 303559 ), effect.item )
    {
      base_dd_min = base_dd_max = effect.driver()->effectN( 2 ).average( effect.item );
    }
  };

  struct venomous_bolt_t : public proc_spell_t
  {
    action_t* frost;

    venomous_bolt_t( const special_effect_t& effect )
      : proc_spell_t( "venomous_bolt", effect.player, effect.player->find_spell( 303558 ), effect.item ),
        frost( create_proc_action<shivering_bolt_t>( "shivering_bolt", effect ) )
    {
      base_dd_min = base_dd_max = effect.driver()->effectN( 1 ).average( effect.item );
      add_child( frost );
    }

    void execute() override
    {
      proc_spell_t::execute();

      if ( sim->bfa_opts.shiver_venom )
      {
        frost->set_target( execute_state->target );
        frost->execute();
      }
    }
  };

  effect.execute_action = create_proc_action<venomous_bolt_t>( "venomous_bolt", effect );

  new dbc_proc_callback_t( effect.player, effect );
}

// Shiver Venom Lance ==================================================

void items::shiver_venom_lance( special_effect_t& effect )
{
  struct venomous_lance_t : public proc_spell_t
  {
    venomous_lance_t( const special_effect_t& effect )
      : proc_spell_t( "venomous_lance", effect.player, effect.player->find_spell( 303562 ), effect.item )
    {
      base_dd_min = base_dd_max = effect.driver()->effectN( 2 ).average( effect.item );
    }
  };

  struct shivering_lance_t : public proc_spell_t
  {
    action_t* nature;

    shivering_lance_t( const special_effect_t& effect )
      : proc_spell_t( "shivering_lance", effect.player, effect.player->find_spell( 303560 ), effect.item ),
        nature( create_proc_action<venomous_lance_t>( "venomous_lance", effect ) )
    {
      base_dd_min = base_dd_max = effect.driver()->effectN( 1 ).average( effect.item );
      add_child( nature );
    }

    void execute() override
    {
      proc_spell_t::execute();

      if ( sim->bfa_opts.shiver_venom )
      {
        nature->set_target( execute_state->target );
        nature->execute();
      }
    }
  };

  effect.execute_action = create_proc_action<shivering_lance_t>( "shivering_lance", effect );

  new dbc_proc_callback_t( effect.player, effect );
}

/**Ashvane's Razor Coral
 * On-use deals damage and gives you ability proc damage & apply debuff. Use again to gain crit per debuff.
 * id=303564 driver for on-use, deals damage + gives you damage & debuff proc on rppm
 * id=303565 damage & debuff proc rppm driver
 * id=303568 stacking debuff
 * id=303570 crit buff
 * id=303572 damage spell
 * id=303573 crit buff value in effect#1
 * id=304877 damage spell value in effect #1
 *
 * TODO: Determine refresh / stack behavior of the crit buff
 */
struct razor_coral_constructor_t : public item_targetdata_initializer_t
{
  razor_coral_constructor_t( unsigned iid, ::util::span<const slot_e> s )
    : item_targetdata_initializer_t( iid, s )
  {}

  void operator()( actor_target_data_t* td ) const override
  {
    const special_effect_t* effect = find_effect( td->source );
    if ( !effect )
    {
      td->debuff.razor_coral = make_buff( *td, "razor_coral_debuff" );
      return;
    }
    assert( !td->debuff.razor_coral );

    td->debuff.razor_coral =
        make_buff( *td, "razor_coral_debuff", td->source->find_spell( 303568 ) )->set_activated( false );
    td->debuff.razor_coral->set_stack_change_callback( [td]( buff_t*, int old_, int new_ ) {
      if ( !new_ )  // buff on expiration, including demise
        td->source->buffs.razor_coral->trigger( old_ );
    } );
    td->debuff.razor_coral->reset();
  }
};

void items::ashvanes_razor_coral( special_effect_t& effect )
{
  struct razor_coral_t : public proc_t
  {
    razor_coral_t( const special_effect_t& e ) : proc_t( e, "razor_coral", e.player->find_spell( 303572 ) )
    {
      base_dd_min = base_dd_max = e.player->find_spell( 304877 )->effectN( 1 ).average( e.item );
    }
  };

  struct ashvanes_razor_coral_t : public proc_t
  {
    action_t* action;
    buff_t* debuff;  // there can be only one target with the debuff at a time

    ashvanes_razor_coral_t( const special_effect_t& e, action_t* a )
      : proc_t( e, "ashvanes_razor_coral", e.driver() ), action( a ), debuff( nullptr )
    {
    }

    void reset_debuff()
    {
      debuff = nullptr;
    }

    void execute() override
    {
      proc_t::execute();  // no damage, but sets up execute_state

      if ( !debuff )  // first use
      {
        action->set_target( execute_state->target );
        action->execute();  // do the damage here

        auto td = player->get_target_data( execute_state->target );
        debuff  = td->debuff.razor_coral;
        debuff->trigger();  // apply to new target
      }
      else  // second use
      {
        debuff->expire();  // debuff's stack_change_callback will trigger the crit buff
        reset_debuff();
      }
    }
  };

  struct razor_coral_cb_t : public dbc_proc_callback_t
  {
    ashvanes_razor_coral_t* action;

    razor_coral_cb_t( const special_effect_t& e, ashvanes_razor_coral_t* a )
      : dbc_proc_callback_t( e.player, e ), action( a )
    {
    }

    void trigger( action_t* a, action_state_t* s ) override
    {
      if ( action->debuff && s && action->debuff->player == s->target && action->debuff->check() )
        dbc_proc_callback_t::trigger( a, s );
    }

    void execute( action_t* a, action_state_t* s ) override
    {
      dbc_proc_callback_t::execute( a, s );

      if ( action->debuff )
        action->debuff->trigger();
    }
  };

  // razor coral damage action used by both on-use and rppm proc
  auto razor = create_proc_action<razor_coral_t>( "razor_coral", effect );

  // the primary on-use
  auto action           = new ashvanes_razor_coral_t( effect, razor );
  effect.execute_action = action;

  // secondary rppm effect for when debuff is applied
  auto second            = new special_effect_t( effect.player );
  second->type           = SPECIAL_EFFECT_EQUIP;
  second->source         = effect.source;
  second->spell_id       = 303565;
  second->execute_action = razor;

  effect.player->special_effects.push_back( second );
  auto proc = new razor_coral_cb_t( *second, action );
  proc->initialize();

  // crit buff from 2nd on-use activation
  if ( !effect.player->buffs.razor_coral )
  {
    effect.player->buffs.razor_coral =
        make_buff<stat_buff_t>( effect.player, "razor_coral", effect.player->find_spell( 303570 ) )
            ->add_stat( STAT_CRIT_RATING, effect.player->find_spell( 303573 )->effectN( 1 ).average( effect.item ) )
            ->set_refresh_behavior( buff_refresh_behavior::DURATION )  // TODO: determine this behavior
            ->set_stack_change_callback( [action]( buff_t*, int, int new_ ) {
              if ( new_ )
                action->reset_debuff();  // buff also gets applied on demise, so reset the pointer in case this happens
            } );
  }
}

/**Dribbling Inkpod
 * Apply debuff to enemy over 30% hp, when enemy falls below deal damage per debuff
 * id=296963 driver, effect#3 is hp threshold, effect#1 is damage value
 * id=296964 proc aura which procs the damage effect when dropping below 30%, possibly to trigger damage
 * id=302491 damage spell
 * id=302565 stacking debuff
 * id=302597 unknown
 */
struct conductive_ink_constructor_t : public item_targetdata_initializer_t
{
  conductive_ink_constructor_t( unsigned iid, ::util::span<const slot_e> s )
    : item_targetdata_initializer_t( iid, s )
  {}

  void operator()( actor_target_data_t* td ) const override
  {
    const special_effect_t* effect = find_effect( td->source );
    if ( !effect )
    {
      td->debuff.conductive_ink = make_buff( *td, "conductive_ink_debuff" );
      return;
    }
    assert( !td->debuff.conductive_ink );

    td->debuff.conductive_ink = make_buff( *td, "conductive_ink_debuff", td->source->find_spell( 302565 ) )
                                    ->set_activated( false )
                                    ->set_max_stack( 255 );
    td->debuff.conductive_ink->reset();
  }
};

void items::dribbling_inkpod( special_effect_t& effect )
{
  struct conductive_ink_cb_t : public dbc_proc_callback_t
  {
    double hp_pct;

    conductive_ink_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ), hp_pct( e.driver()->effectN( 3 ).base_value() )
    {
    }

    void trigger( action_t* a, action_state_t* s ) override
    {
      // Conductive Ink shouldn't be able to proc on the caster (happens when a positive effect like a heal hits the
      // player)
      if ( s->target->health_percentage() > hp_pct && s->target != a->player )
      {
        dbc_proc_callback_t::trigger( a, s );
      }
    }

    void execute( action_t*, action_state_t* s ) override
    {
      if ( s->target->health_percentage() > hp_pct )
      {
        auto td = listener->get_target_data( s->target );
        assert( td );
        assert( td->debuff.conductive_ink );
        td->debuff.conductive_ink->trigger();
      }
    }
  };

  struct conductive_ink_boom_cb_t : public dbc_proc_callback_t
  {
    double hp_pct;

    conductive_ink_boom_cb_t( const special_effect_t& e, const special_effect_t& primary )
      : dbc_proc_callback_t( e.player, e ), hp_pct( primary.driver()->effectN( 3 ).base_value() )
    {
    }

    void trigger( action_t* a, action_state_t* s ) override
    {
      auto td = listener->get_target_data( s->target );
      assert( td );
      assert( td->debuff.conductive_ink );

      if ( td->debuff.conductive_ink->check() && s->target->health_percentage() <= hp_pct )
      {
        dbc_proc_callback_t::trigger( a, s );
      }
    }

    void execute( action_t* a, action_state_t* s ) override
    {
      auto td = listener->get_target_data( s->target );

      // Simultaneous attacks that hit at once can all count as the damage to burst the debuff, triggering the callback
      // multiple times. Ensure the event-scheduled callback execute checks for the debuff so we don't get multiple hits
      if ( td->debuff.conductive_ink->check() )
      {
        dbc_proc_callback_t::execute( a, s );
      }
    }
  };

  struct conductive_ink_t : public proc_t
  {
    conductive_ink_t( const special_effect_t& e, const special_effect_t& primary )
      : proc_t( e, "conductive_ink", e.driver() )
    {
      base_dd_min = base_dd_max = primary.driver()->effectN( 1 ).average( primary.item );
    }

    double action_multiplier() const override
    {
      auto td = player->get_target_data( target );
      assert( td );
      assert( td->debuff.conductive_ink );

      return proc_t::action_multiplier() * td->debuff.conductive_ink->stack();
    }

    void impact( action_state_t* s ) override
    {
      auto td = player->get_target_data( s->target );
      td->debuff.conductive_ink->expire();

      proc_t::impact( s );
    }
  };

  new conductive_ink_cb_t( effect );

  auto second            = new special_effect_t( effect.player );
  second->type           = effect.type;
  second->source         = effect.source;
  second->spell_id       = 302491;
  second->proc_flags_    = PF_ALL_DAMAGE | PF_PERIODIC;
  second->proc_flags2_   = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;
  second->proc_chance_   = 1.0;
  second->execute_action = create_proc_action<conductive_ink_t>( "conductive_ink", *second, effect );
  effect.player->special_effects.push_back( second );

  new conductive_ink_boom_cb_t( *second, effect );
}

// Reclaimed Shock Coil ===================================================

void items::reclaimed_shock_coil( special_effect_t& effect )
{
  effect.proc_flags2_ = PF2_CRIT;

  new dbc_proc_callback_t( effect.player, effect );
}

// Dream's End ============================================================
// TODO: Figure out details of how the attack speed buff stacks with other haste.

void items::dreams_end( special_effect_t& effect )
{
  struct delirious_frenzy_cb_t : public dbc_proc_callback_t
  {
    player_t* target;

    delirious_frenzy_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e )
    {
    }

    void execute( action_t*, action_state_t* s ) override
    {
      if ( !target || target != s->target )  // new target
      {
        target = s->target;
        listener->buffs.delirious_frenzy->expire();  // cancel buff, restart below
      }

      listener->buffs.delirious_frenzy->trigger();
    }
  };

  if ( !effect.player->buffs.delirious_frenzy )
  {
    effect.player->buffs.delirious_frenzy = make_buff( effect.player, "delirious_frenzy", effect.trigger() )
                                                ->set_default_value( effect.trigger()->effectN( 1 ).percent() )
                                                ->add_invalidate( CACHE_ATTACK_SPEED );
  }

  new delirious_frenzy_cb_t( effect );
}

/**Diver's Folly
 * Auto attack has a chance to proc a buff. When buff is up, store damage. When buff ends, the next auto attack will
 * discharge the stored damage.
 * id=303353 driver, e#1 is % of damage stored by buff, e#2 is targets hit by discharge
 * id=303580 buff spell (driver->trigger), stores damage
 * id=303583 discharge damage spell
 * id=303621 driver for discharge
 * TODO: Can you proc the main buff again from the AA that procs the discharge?
 */

void items::divers_folly( special_effect_t& effect )
{
  // Primary driver callback. Procs buff on AA.
  struct divers_folly_cb_t : public dbc_proc_callback_t
  {
    divers_folly_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e )
    {
    }

    void trigger( action_t* a, action_state_t* s ) override
    {
      // Doesn't proc when buff is up, and doesn't seem to trigger rppm neither
      if ( listener->buffs.bioelectric_charge->check() )
        return;

      dbc_proc_callback_t::trigger( a, s );
    }

    void execute( action_t*, action_state_t* ) override
    {
      listener->buffs.bioelectric_charge->trigger();
    }
  };

  // Buff to store damage. Activates secondary driver on expiration.
  struct bioelectric_charge_buff_t : public buff_t
  {
    dbc_proc_callback_t* proc2;

    bioelectric_charge_buff_t( const special_effect_t& e, dbc_proc_callback_t* cb )
      : buff_t( e.player, "bioelectric_charge", e.trigger(), e.item ), proc2( cb )
    {
    }

    void expire_override( int s, timespan_t d ) override
    {
      proc2->proc_action->base_dd_min = value();
      proc2->proc_action->base_dd_max = value();
      proc2->activate();
      buff_t::expire_override( s, d );
    }
  };

  // Secondary driver callback. Executes discharge action on next AA.
  struct bioelectric_charge_cb_t : public dbc_proc_callback_t
  {
    timespan_t duration;

    bioelectric_charge_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ), duration( e.driver()->duration() )
    {
    }

    void trigger( action_t* a, action_state_t* s ) override
    {
      listener->sim->print_debug( "Bioelectric Charge (Diver's Folly) discharged for {} damage!",
                                  proc_action->base_dd_min );

      dbc_proc_callback_t::trigger( a, s );
      deactivate();  // trigger should always be a success, so deactivate immediately on trigger
    }
  };

  // effect for secondary proc that activates after buff expires, triggers on next melee hit to discharge
  auto second      = new special_effect_t( effect.player );
  second->type     = effect.type;
  second->source   = effect.source;
  second->spell_id = 303621;

  // discharge action, executed by secondary proc
  auto action2 = create_proc_action<proc_t>( "bioelectric_charge", effect, "bioelectric_charge", 303583 );
  action2->aoe = static_cast<int>( effect.driver()->effectN( 2 ).base_value() );

  second->execute_action = action2;
  effect.player->special_effects.push_back( second );

  // secondary proc callback
  auto proc2 = new bioelectric_charge_cb_t( *second );
  proc2->deactivate();
  proc2->initialize();

  if ( !effect.player->buffs.bioelectric_charge )
  {
    double pct = effect.driver()->effectN( 1 ).percent();
    auto buff  = make_buff<bioelectric_charge_buff_t>( effect, proc2 );

    effect.player->buffs.bioelectric_charge = buff;
    effect.player->assessor_out_damage.add(
        assessor::TARGET_DAMAGE + 1, [buff, pct]( result_amount_type, action_state_t* s ) {
          if ( !buff->check() )
            return assessor::CONTINUE;

          double old_  = buff->current_value;
          double this_ = s->result_amount * pct;
          buff->sim->print_debug( "Bioelectric Charge (Diver's Folly) storing {}({}% of {}) damage from {}: {} -> {}",
                                  this_, pct, s->result_amount, s->action->name(), old_, old_ + this_ );
          buff->current_value += this_;
          return assessor::CONTINUE;
        } );
  }

  new divers_folly_cb_t( effect );
}

/**Remote Guidance Device
 * id=302307, on-use driver
 * id=302308, primary target damage in e#1, aoe damage in e#2
 * id=302311, primary target damage
 * id=302312, aoe damage
 * TODO: Does the aoe also hit the primary target? What is the travel time of the mechacycle?
 */

void items::remote_guidance_device( special_effect_t& effect )
{
  struct remote_guidance_device_t : public proc_t
  {
    action_t* area;

    remote_guidance_device_t( const special_effect_t& e, action_t* a )
      : proc_t( e, "remote_guidance_device", e.player->find_spell( 302311 ) ), area( a )
    {
      aoe          = 0;
      travel_speed = 10.0;  // TODO: find actual travel speed (either from data or testing) and update
      base_dd_min = base_dd_max = e.player->find_spell( 302308 )->effectN( 1 ).average( e.item );
      add_child( area );
    }

    void impact( action_state_t* s ) override
    {
      proc_t::impact( s );
      area->set_target( s->target );
      area->execute();
    }
  };

  struct remote_guidance_device_aoe_t : public proc_t
  {
    remote_guidance_device_aoe_t( const special_effect_t& e )
      : proc_t( e, "remote_guidance_device_aoe", e.player->find_spell( 302312 ) )
    {
      aoe         = -1;
      base_dd_min = base_dd_max = e.player->find_spell( 302308 )->effectN( 2 ).average( e.item );
    }
  };

  auto area             = create_proc_action<remote_guidance_device_aoe_t>( "remote_guidance_device_aoe", effect );
  effect.execute_action = create_proc_action<remote_guidance_device_t>( "remote_guidance_device", effect, area );
}

// Gladiator's Maledict ===================================================

void items::gladiators_maledict( special_effect_t& effect )
{
  effect.execute_action          = create_proc_action<proc_spell_t>( "gladiators_maledict", effect );
  effect.execute_action->base_td = effect.player->find_spell( 305251 )->effectN( 1 ).average( effect.item );
}

// Geti'ikku, Cut of Death ================================================

void items::getiikku_cut_of_death( special_effect_t& effect )
{
  // Note, no create_proc_action here, since there is the possibility of dual-wielding them and
  // special_effect_t does not have enough support for defining "shared spells" on initialization
  effect.execute_action =
      new proc_spell_t( "cut_of_death", effect.player, effect.player->find_spell( 281711 ), effect.item );

  new dbc_proc_callback_t( effect.player, effect );
}

// Bile-Stained Crawg Tusks ===============================================

void items::bilestained_crawg_tusks( special_effect_t& effect )
{
  struct vile_bile_t : public proc_spell_t
  {
    vile_bile_t( const special_effect_t& effect )
      : proc_spell_t( "vile_bile", effect.player, effect.player->find_spell( 281721 ), effect.item )
    {
    }

    // Refresh procs do not override the potency of the dot (if wielding two different
    // ilevel weapons)
    void trigger_dot( action_state_t* state ) override
    {
      auto dot        = get_dot( state->target );
      auto dot_action = dot->is_ticking() ? dot->current_action : nullptr;

      proc_spell_t::trigger_dot( state );

      if ( dot_action )
      {
        dot->current_action = dot_action;
      }
    }
  };

  // Note, no create_proc_action here, since there is the possibility of dual-wielding them and
  // special_effect_t does not have enough support for defining "shared spells" on initialization
  effect.execute_action = new vile_bile_t( effect );

  new dbc_proc_callback_t( effect.player, effect );
}

// Punchcard stuff ========================================================

item_t init_punchcard( const special_effect_t& effect )
{
  if ( !effect.enchant_data )
  {
    return {};
  }

  const auto& item_data = effect.player->dbc->item( effect.enchant_data->id_gem );
  if ( item_data.id == 0 )
  {
    return {};
  }

  // Figure out the gem slot
  auto it = range::find( effect.item->parsed.gem_id, effect.enchant_data->id_gem );
  if ( it == effect.item->parsed.gem_id.end() )
  {
    return {};
  }

  item_t punchcard( effect.player, "" );
  punchcard.parsed.data.init( item_data, *effect.player->dbc );
  punchcard.name_str    = ::util::tokenize_fn( item_data.name );

  // Punchcards use the item level of he trinket itself, apparently.
  punchcard.parsed.data.level = effect.item->item_level();

  effect.player->sim->print_debug( "{} initializing punchcard: {}", effect.player->name(), punchcard );

  return punchcard;
}

void items::yellow_punchcard( special_effect_t& effect )
{
  std::vector<std::tuple<stat_e, double>> stats;

  // We don't need to do any further initialization so don't perform phase 2 at all
  effect.type = SPECIAL_EFFECT_NONE;

  auto punchcard = init_punchcard( effect );
  if ( punchcard.parsed.data.id == 0 )
  {
    return;
  }

  auto budget = item_database::item_budget( effect.player, punchcard.item_level() );

  // Collect stats
  for ( size_t i = 1u; i <= effect.driver()->effect_count(); ++i )
  {
    if ( effect.driver()->effectN( i ).subtype() != A_MOD_RATING )
    {
      continue;
    }

    auto effect_stats = ::util::translate_all_rating_mod( effect.driver()->effectN( i ).misc_value1() );
    double value      = effect.driver()->effectN( i ).m_coefficient() * budget;
    value             = item_database::apply_combat_rating_multiplier(
        effect.player, combat_rating_multiplier_type::CR_MULTIPLIER_TRINKET, punchcard.item_level(), value );
    range::for_each( effect_stats, [value, &stats]( stat_e stat ) { stats.emplace_back( stat, value ); } );
  }

  // .. and apply them as passive stats to the actor
  range::for_each( stats, [&effect, &punchcard]( const std::tuple<stat_e, double>& stats ) {
    stat_e stat  = std::get<0>( stats );
    double value = std::get<1>( stats );
    if ( effect.player->sim->debug )
    {
      effect.player->sim->out_debug.print( "{} {}: punchcard={} ({}), stat={}, value={}", effect.player->name(),
                                           effect.item->name(), punchcard.name(), punchcard.item_level(),
                                           ::util::stat_type_string( stat ), value );
    }
    effect.player->passive.add_stat( stat, value );
  } );
}

void items::subroutine_overclock( special_effect_t& effect )
{
  auto punchcard = init_punchcard( effect );
  if ( punchcard.parsed.data.id == 0 )
  {
    return;
  }

  stat_buff_t* buff = debug_cast<stat_buff_t*>( buff_t::find( effect.player, "subroutine_overclock" ) );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "subroutine_overclock", effect.player->find_spell( 293142 ) );
    buff->add_stat( STAT_HASTE_RATING,
                    item_database::apply_combat_rating_multiplier(
                        effect.player, combat_rating_multiplier_type::CR_MULTIPLIER_TRINKET, punchcard.item_level(),
                        effect.player->find_spell( 293142 )->effectN( 1 ).average( &( punchcard ) ) ) );
  }

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.player, effect );
}

void items::subroutine_recalibration( special_effect_t& effect )
{
  struct recalibration_cb_t : public dbc_proc_callback_t
  {
    unsigned casts, req_casts;
    buff_t *buff, *debuff;

    recalibration_cb_t( const special_effect_t& effect, buff_t* b, buff_t* d )
      : dbc_proc_callback_t( effect.player, effect ),
        casts( effect.player->sim->bfa_opts.subroutine_recalibration_precombat_stacks ),
        // Reduce required casts if there's assumed extra casts per buff cycle.
        req_casts( as<unsigned>( effect.driver()->effectN( 2 ).base_value() ) -
                   effect.player->sim->bfa_opts.subroutine_recalibration_dummy_casts ),
        buff( b ),
        debuff( d )
    {
    }

    void trigger( action_t* a, action_state_t* s ) override
    {
      if ( a->background )
      {
        return;
      }

      // The cast counter does not increase if either of the associated buffs is active
      if ( buff->check() || debuff->check() )
      {
        return;
      }

      if ( listener->sim->debug )
      {
        listener->sim->out_debug.print( "{} procs subroutine_recalibration on {}, casts={}, max={}", listener->name(),
                                        a->name(), casts, req_casts );
      }

      if ( ++casts >= req_casts )
      {
        dbc_proc_callback_t::trigger( a, s );
        casts = 0;
      }
    }

    void reset() override
    {
      dbc_proc_callback_t::reset();
      casts = effect.player->sim->bfa_opts.subroutine_recalibration_precombat_stacks;
    }

    void activate() override
    {
      dbc_proc_callback_t::activate();
      casts = effect.player->sim->bfa_opts.subroutine_recalibration_precombat_stacks;
    }
  };

  auto punchcard = init_punchcard( effect );
  if ( punchcard.parsed.data.id == 0 )
  {
    return;
  }

  stat_buff_t* primary_buff = debug_cast<stat_buff_t*>( buff_t::find( effect.player, "subroutine_recalibration" ) );
  stat_buff_t* recalibration_buff = debug_cast<stat_buff_t*>( buff_t::find( effect.player, "recalibrating" ) );

  if ( !recalibration_buff )
  {
    recalibration_buff =
        make_buff<stat_buff_t>( effect.player, "recalibrating", effect.player->find_spell( 299065 ), effect.item );
    recalibration_buff->add_stat(
        STAT_HASTE_RATING,
        item_database::apply_combat_rating_multiplier(
            effect.player, combat_rating_multiplier_type::CR_MULTIPLIER_TRINKET, punchcard.item_level(),
            effect.player->find_spell( 299065 )->effectN( 1 ).average( &( punchcard ) ) ) );
  }

  if ( !primary_buff )
  {
    primary_buff = make_buff<stat_buff_t>( effect.player, "subroutine_recalibration",
                                           effect.player->find_spell( 299064 ), effect.item );
    primary_buff->add_stat(
        STAT_HASTE_RATING,
        item_database::apply_combat_rating_multiplier(
            effect.player, combat_rating_multiplier_type::CR_MULTIPLIER_TRINKET, punchcard.item_level(),
            effect.player->find_spell( 299064 )->effectN( 1 ).average( &( punchcard ) ) ) );
    primary_buff->set_stack_change_callback( [recalibration_buff]( buff_t*, int /* old */, int new_ ) {
      if ( new_ == 0 )
      {
        recalibration_buff->trigger();
      }
    } );
  }

  effect.proc_flags_  = PF_ALL_DAMAGE;
  effect.proc_flags2_ = PF2_CAST | PF2_CAST_DAMAGE | PF2_CAST_HEAL;
  effect.custom_buff  = primary_buff;

  new recalibration_cb_t( effect, primary_buff, recalibration_buff );
}

void items::subroutine_optimization( special_effect_t& effect )
{
  struct subroutine_optimization_buff_t : public buff_t
  {
    std::array<double, STAT_MAX> stats;
    stat_e major_stat;
    stat_e minor_stat;
    item_t punchcard;
    double raw_bonus;

    subroutine_optimization_buff_t( const special_effect_t& effect )
      : buff_t( effect.player, "subroutine_optimization", effect.trigger() ),
        major_stat( STAT_NONE ),
        minor_stat( STAT_NONE ),
        punchcard( init_punchcard( effect ) ),
        raw_bonus( item_database::apply_combat_rating_multiplier(
            effect.player, combat_rating_multiplier_type::CR_MULTIPLIER_TRINKET, punchcard.item_level(),
            effect.driver()->effectN( 2 ).average( &( punchcard ) ) ) )
    {
      // Find the two stats provided by the punchcard.
      auto stat_spell = yellow_punchcard( effect );
      std::vector<std::pair<stat_e, double>> punchcard_stats;
      for ( size_t i = 1u; i <= stat_spell->effect_count(); ++i )
      {
        if ( stat_spell->effectN( i ).subtype() != A_MOD_RATING )
        {
          continue;
        }

        auto stats   = ::util::translate_all_rating_mod( stat_spell->effectN( i ).misc_value1() );
        double value = stat_spell->effectN( i ).m_coefficient();

        punchcard_stats.emplace_back( stats.front(), value );
      }

      // Find out which stat has the bigger coefficient.
      range::sort( punchcard_stats, []( const auto& a, const auto& b ) { return a.second > b.second; } );
      if ( punchcard_stats.size() == 2 )
      {
        major_stat = punchcard_stats[ 0 ].first;
        minor_stat = punchcard_stats[ 1 ].first;
      }
      else
      {
        assert( false );
      }
    }

    const spell_data_t* yellow_punchcard( const special_effect_t& effect ) const
    {
      const gem_property_data_t* data = nullptr;

      auto it = range::find_if( effect.item->parsed.gem_id, [this, &data]( unsigned gem_id ) {
        const auto& item_data = source->dbc->item( gem_id );
        if ( item_data.id == 0 )
        {
          return false;
        }

        const auto& gem_props = source->dbc->gem_property( item_data.gem_properties );
        if ( gem_props.id == 0 )
        {
          return false;
        }

        if ( gem_props.color & SOCKET_COLOR_YELLOW_PUNCHCARD )
        {
          data = &( gem_props );
          return true;
        }
        return false;
      } );

      if ( it == effect.item->parsed.gem_id.end() )
      {
        return spell_data_t::not_found();
      }

      // Find the item enchantment associated with the gem
      const auto& enchantment_data = source->dbc->item_enchantment( data->enchant_id );

      for ( size_t i = 0u; i < range::size( enchantment_data.ench_type ); ++i )
      {
        if ( enchantment_data.ench_type[ i ] == ITEM_ENCHANTMENT_EQUIP_SPELL )
        {
          return source->find_spell( enchantment_data.ench_prop[ i ] );
        }
      }

      return spell_data_t::not_found();
    }

    void adjust_stat( stat_e stat, double current, double new_ )
    {
      double diff = new_ - current;
      if ( diff < 0 )
      {
        source->stat_loss( stat, std::fabs( diff ), nullptr, nullptr, true );
      }
      else if ( diff > 0 )
      {
        source->stat_gain( stat, diff, nullptr, nullptr, true );
      }

      if ( diff != 0 )
      {
        stats[ stat ] += diff;
      }
    }

    void restore_stat( stat_e stat, double delta )
    {
      if ( delta < 0 )
      {
        source->stat_gain( stat, std::fabs( delta ), nullptr, nullptr, true );
      }
      else if ( delta > 0 )
      {
        source->stat_loss( stat, delta, nullptr, nullptr, true );
      }
    }

    double stat_multiplier( stat_e s ) const
    {
      // Stat split is defined somewhere server side.
      if ( s == major_stat )
        return 0.6;
      else if ( s == minor_stat )
        return 0.4;
      else
        return 0.0;
    }

    void adjust_stats( bool up )
    {
      if ( !up )
      {
        range::fill( stats, 0.0 );
      }

      // TODO: scale factor behavior?
      double haste       = source->current.stats.haste_rating;
      double mastery     = source->current.stats.mastery_rating;
      double crit        = source->current.stats.crit_rating;
      double versatility = source->current.stats.versatility_rating;

      double total = haste + mastery + crit + versatility;
      if ( !up )
        total += raw_bonus;

      double new_haste       = total * stat_multiplier( STAT_HASTE_RATING );
      double new_mastery     = total * stat_multiplier( STAT_MASTERY_RATING );
      double new_crit        = total * stat_multiplier( STAT_CRIT_RATING );
      double new_versatility = total * stat_multiplier( STAT_VERSATILITY_RATING );

      if ( sim->debug )
      {
        sim->out_debug.print(
            "{} subroutine_optimization total={}, haste={}->{}, crit={}->{}, "
            "mastery={}->{}, versatility={}->{}",
            source->name(), total, haste, new_haste, crit, new_crit, mastery, new_mastery, versatility,
            new_versatility );
      }

      adjust_stat( STAT_HASTE_RATING, haste, new_haste );
      adjust_stat( STAT_CRIT_RATING, crit, new_crit );
      adjust_stat( STAT_MASTERY_RATING, mastery, new_mastery );
      adjust_stat( STAT_VERSATILITY_RATING, versatility, new_versatility );
    }

    void bump( int stacks, double value ) override
    {
      auto is_up = current_stack != 0;

      buff_t::bump( stacks, value );

      adjust_stats( is_up );
    }

    void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
    {
      buff_t::expire_override( expiration_stacks, remaining_duration );

      restore_stat( STAT_HASTE_RATING, stats[ STAT_HASTE_RATING ] );
      restore_stat( STAT_CRIT_RATING, stats[ STAT_CRIT_RATING ] );
      restore_stat( STAT_MASTERY_RATING, stats[ STAT_MASTERY_RATING ] );
      restore_stat( STAT_VERSATILITY_RATING, stats[ STAT_VERSATILITY_RATING ] );
    }
  };

  auto buff = buff_t::find( effect.player, "subroutine_optimization" );
  if ( !buff )
  {
    buff = make_buff<subroutine_optimization_buff_t>( effect );
  }

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.player, effect );
}

void items::harmonic_dematerializer( special_effect_t& effect )
{
  struct harmonic_dematerializer_t : public proc_spell_t
  {
    buff_t* buff;
    std::string target_name;

    harmonic_dematerializer_t( const special_effect_t& effect, buff_t* b ) : proc_spell_t( effect ), buff( b )
    {
      auto punchcard = init_punchcard( effect );
      base_dd_min = base_dd_max = effect.driver()->effectN( 1 ).average( &( punchcard ) );
      cooldown->duration        = timespan_t::zero();
    }

    double action_multiplier() const override
    {
      double m = proc_spell_t::action_multiplier();

      m *= 1.0 + buff->stack_value();

      return m;
    }

    void execute() override
    {
      if ( !::util::str_compare_ci( target_name, target->name() ) )
      {
        buff->expire();
      }

      proc_spell_t::execute();

      buff->trigger();
      target_name = target->name();
    }
  };

  auto buff = buff_t::find( effect.player, "harmonic_dematerializer" );
  if ( !buff )
  {
    buff = make_buff( effect.player, "harmonic_dematerializer", effect.driver() );
    buff->set_cooldown( timespan_t::zero() );
    buff->set_default_value( effect.driver()->effectN( 2 ).percent() );
  }

  effect.execute_action = create_proc_action<harmonic_dematerializer_t>( "harmonic_dematerializer", effect, buff );
}

// Cyclotronic Blast ======================================================

void items::cyclotronic_blast( special_effect_t& effect )
{
  struct cyclotronic_blast_t : public proc_t
  {
    cyclotronic_blast_t( const special_effect_t& e ) : proc_t( e, "cyclotronic_blast", e.driver() )
    {
      channeled = true;
    }

    bool usable_moving() const override
    {
      return true;
    }

    void execute() override
    {
      proc_t::execute();
      event_t::cancel( player->readying );
      player->reset_auto_attacks( composite_dot_duration( execute_state ) );
    }

    void last_tick( dot_t* d ) override
    {
      bool was_channeling = player->channeling == this;
      proc_t::last_tick( d );

      if ( was_channeling && !player->readying )
      {
        player->schedule_ready( rng().gauss( sim->channel_lag, sim->channel_lag_stddev ) );
      }
    }
  };

  effect.execute_action = create_proc_action<cyclotronic_blast_t>( "cyclotronic_blast", effect );

  action_t* action = effect.player->find_action( "use_item_" + effect.item->name_str );
  if ( action )
    action->base_execute_time = effect.execute_action->base_execute_time;
}

// Mechagon Logic Loop - Bit Band combo rings
const unsigned logic_loop_drivers[] = {299909, 300124, 300125};

const special_effect_t* find_logic_loop_effect( player_t* player )
{
  for ( unsigned id : logic_loop_drivers )
  {
    auto it = unique_gear::find_special_effect( player, id, SPECIAL_EFFECT_EQUIP );
    if ( it )
    {
      player->sim->print_debug( "Logic Loop found! Pairing to {}...", it->item->full_name() );
      return it;
    }
  }
  player->sim->print_debug( "404 NOT FOUND. Powering down..." );
  return nullptr;
}

struct logic_loop_callback_t : public dbc_proc_callback_t
{
  logic_loop_callback_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e )
  {
    cooldown           = e.player->get_cooldown( "logic_loop_recharging" );
    cooldown->duration = e.player->find_spell( 306474 )->duration();
  }

  void initialize() override
  {
    dbc_proc_callback_t::initialize();
    if ( !proc_buff && !proc_action )
    {
      deactivate();
      listener->sim->print_debug( "Logic Loop pairing failure. Deactivating..." );
    }
  }
};

// Logic Loop of Division ( proc when you attack from behind )

void items::logic_loop_of_division( special_effect_t& effect )
{
  struct loop_of_division_cb_t : public logic_loop_callback_t
  {
    loop_of_division_cb_t( const special_effect_t& e ) : logic_loop_callback_t( e )
    {
    }

    void trigger( action_t* a, action_state_t* state ) override
    {
      if ( listener->position() == POSITION_BACK || listener->position() == POSITION_RANGED_BACK )
      {
        logic_loop_callback_t::trigger( a, state );
      }
    }
  };

  // TODO: Does this trigger from periodic damage?
  effect.proc_flags_  = PF_ALL_DAMAGE;
  effect.proc_flags2_ = PF2_ALL_HIT;
  effect.proc_chance_ = 1.0;

  new loop_of_division_cb_t( effect );
}

// Logic Loop of Recursion ( proc when you use 3 different abilities on a target )

void items::logic_loop_of_recursion( special_effect_t& effect )
{
  struct llor_tracker_t
  {
    player_t* target;
    std::vector<int> action_ids;
  };

  struct loop_of_recursion_cb_t : public logic_loop_callback_t
  {
    std::vector<llor_tracker_t> list;
    unsigned max;

    loop_of_recursion_cb_t( const special_effect_t& e )
      : logic_loop_callback_t( e ), max( static_cast<unsigned>( e.driver()->effectN( 1 ).base_value() ) )
    {
    }

    void trigger( action_t* a, action_state_t* s ) override
    {
      int this_id = a->internal_id;
      auto it     = range::find( list, a->target, &llor_tracker_t::target );
      if ( it == list.end() )  // new target
      {
        llor_tracker_t tmp;
        tmp.target = a->target;
        tmp.action_ids.push_back( this_id );
        list.push_back( std::move( tmp ) );  // create new entry for target
        return;
      }

      auto it2 = range::find( it->action_ids, this_id );
      if ( it2 == it->action_ids.end() )  // new action
      {
        if ( it->action_ids.size() < max - 1 )  // not full
        {
          it->action_ids.push_back( this_id );  // create new entry for action
        }
        else  // full, execute
        {
          logic_loop_callback_t::trigger( a, s );
          for ( auto& tracker : list )
            tracker.action_ids.clear();
        }
      }
    }
  };

  effect.proc_flags_  = PF_ALL_DAMAGE;
  effect.proc_flags2_ = PF2_CAST_DAMAGE | PF2_CAST_HEAL | PF2_CAST;
  effect.proc_chance_ = 1.0;

  new loop_of_recursion_cb_t( effect );
}

// Logic Loop of Maintenance ( proc on cast while below 50% hp )

void items::logic_loop_of_maintenance( special_effect_t& effect )
{
  struct loop_of_maintenance_cb_t : public logic_loop_callback_t
  {
    loop_of_maintenance_cb_t( const special_effect_t& e ) : logic_loop_callback_t( e )
    {
    }

    void trigger( action_t* a, action_state_t* s ) override
    {
      if ( listener->health_percentage() < 50 )
      {
        logic_loop_callback_t::trigger( a, s );
      }
    }
  };

  effect.proc_flags_  = PF_ALL_DAMAGE;
  effect.proc_flags2_ = PF2_CAST_DAMAGE | PF2_CAST_HEAL | PF2_CAST;
  effect.proc_chance_ = 1.0;

  new loop_of_maintenance_cb_t( effect );
}

// Overclocking Bit Band ( haste proc )

void items::overclocking_bit_band( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, "overclocking_bit_band" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "overclocking_bit_band", effect.player->find_spell( 301886 ) )
               ->add_stat( STAT_HASTE_RATING, effect.driver()->effectN( 1 ).average( effect.item ) );
  }

  effect.player->sim->print_debug( "{} looking for Logic Loop to pair with...", effect.item->full_name() );
  auto loop_driver = find_logic_loop_effect( effect.player );
  if ( loop_driver )
  {
    const_cast<special_effect_t*>( loop_driver )->custom_buff = buff;
    effect.player->sim->print_debug( "Success! {} paired with {}.", effect.item->full_name(),
                                     loop_driver->item->full_name() );
  }

  effect.type = SPECIAL_EFFECT_NONE;
}

// Shorting Bit Band ( deal damage to random enemy within 8 yd )

void items::shorting_bit_band( special_effect_t& effect )
{
  struct shorting_bit_band_t : public proc_t
  {
    shorting_bit_band_t( const special_effect_t& e ) : proc_t( e, "shorting_bit_band", e.player->find_spell( 301887 ) )
    {
      aoe         = 0;
      base_dd_min = base_dd_max = e.driver()->effectN( 1 ).average( e.item );
      range                     = radius;
    }

    void execute() override
    {
      auto numTargets = targets_in_range_list( target_list() ).size();
      if ( numTargets !=
           0 )  // We only do anything if target in range; we just eat the proc and do nothing if no targets <=8y
      {
        size_t index = rng().range( numTargets );
        set_target( targets_in_range_list( target_list() )[ index ] );

        proc_t::execute();
      }
    }
  };

  auto action = create_proc_action<shorting_bit_band_t>( "shorting_bit_band", effect );

  effect.player->sim->print_debug( "{} looking for Logic Loop to pair with...", effect.item->full_name() );
  auto loop_driver = find_logic_loop_effect( effect.player );
  if ( loop_driver )
  {
    const_cast<special_effect_t*>( loop_driver )->execute_action = action;
    effect.player->sim->print_debug( "Success! {} paired with {}.", effect.item->full_name(),
                                     loop_driver->item->full_name() );
  }

  effect.type = SPECIAL_EFFECT_NONE;
}

// Hyperthread Wristwraps

void items::hyperthread_wristwraps( special_effect_t& effect )
{
  auto spell_tracker          = new special_effect_t( effect.player );
  spell_tracker->name_str     = "hyperthread_wristwraps_spell_tracker";
  spell_tracker->type         = SPECIAL_EFFECT_EQUIP;
  spell_tracker->source       = SPECIAL_EFFECT_SOURCE_ITEM;
  spell_tracker->proc_chance_ = 1.0;
  spell_tracker->proc_flags_  = PF_ALL_DAMAGE | PF_ALL_HEAL;
  spell_tracker->proc_flags2_ = PF2_CAST | PF2_CAST_DAMAGE | PF2_CAST_HEAL;
  effect.player->special_effects.push_back( spell_tracker );

  static constexpr auto spell_blacklist = ::util::make_static_set<unsigned>( {
      // Racials
      26297,   // Berserking
      28730,   // Arcane Torrent
      33702,   // Blood Fury
      58984,   // Shadowmeld
      65116,   // Stoneform
      68992,   // Darkflight
      69041,   // Rocket Barrage
      232633,  // Arcane Torrent
      255647,  // Light's Judgment
      256893,  // Light's Judgment
      260364,  // Arcane Pulse
      265221,  // Fireblood
      274738,  // Ancestral Call
      287712,  // Haymaker
      312411,  // Bag of Tricks
      // Major Essences
      295373,  // Concentrated Flame
      298357,  // Memory of Lucid Dreams
      297108,  // Blood of The Enemy
      295258,  // Focused Azerite Beam
      295840,  // Guardian of Azeroth
      295337,  // Purifying Blast
      302731,  // Ripple in Space
      298452,  // The Unbound Force
      295186,  // Worldvein Resonance
  } );

  struct spell_tracker_cb_t : public dbc_proc_callback_t
  {
    size_t max_size;
    std::vector<action_t*> last_used;

    spell_tracker_cb_t( const special_effect_t& effect, size_t size )
      : dbc_proc_callback_t( effect.player, effect ), max_size( size )
    {
    }

    void execute( action_t* a, action_state_t* ) override
    {
      if ( spell_blacklist.find( a->id ) == spell_blacklist.end() )
      {
        listener->sim->print_debug( "Adding {} to Hyperthread Wristwraps tracked spells.", a->name_str );
        last_used.push_back( a );
        while ( last_used.size() > max_size )
          last_used.erase( last_used.begin() );
      }
    }

    void reset() override
    {
      dbc_proc_callback_t::reset();
      last_used.clear();
    }
  };

  struct hyperthread_reduction_t : public proc_spell_t
  {
    timespan_t reduction;
    spell_tracker_cb_t* tracker;

    hyperthread_reduction_t( const special_effect_t& effect, spell_tracker_cb_t* cb )
      : proc_spell_t( effect ), reduction( 1000 * effect.driver()->effectN( 2 ).time_value() ), tracker( cb )
    {
      callbacks = false;
    }

    void execute() override
    {
      proc_spell_t::execute();

      for ( auto a : tracker->last_used )
      {
        sim->print_debug( "Reducing cooldown of action {} by {} s.", a->name_str, reduction.total_seconds() );
        a->cooldown->adjust( -reduction );
      }
    }
  };

  auto cb = new spell_tracker_cb_t( *spell_tracker, as<size_t>( effect.driver()->effectN( 1 ).base_value() ) );
  effect.execute_action = create_proc_action<hyperthread_reduction_t>( "hyperthread_wristwraps", effect, cb );
}

// Anodized Deflectors

void items::anodized_deflectors( special_effect_t& effect )
{
  effect.disable_action();

  buff_t* anodized_deflectors_buff = buff_t::find( effect.player, "anodized_deflection" );

  if ( !anodized_deflectors_buff )
  {
    auto damage = create_proc_action<aoe_proc_t>( "anodized_deflection", effect, "anodized_deflection", 301554 );
    // Manually set the damage because it's stored in the driver spell
    // The damaging spell has a value with a another -much higher- coefficient that doesn't seem to be used in-game
    damage -> base_dd_min = damage -> base_dd_max = effect.driver() -> effectN( 3 ).average( effect.item );

    anodized_deflectors_buff = make_buff<stat_buff_t>( effect.player, "anodized_deflection", effect.driver() )
      -> add_stat( STAT_PARRY_RATING, effect.driver() -> effectN( 1 ).average( effect.item ) )
      -> add_stat( STAT_AVOIDANCE_RATING, effect.driver() -> effectN( 2 ).average( effect.item ) )
      -> set_stack_change_callback( [ damage ]( buff_t*, int, int new_ ) {
          if ( new_ == 0 )
          {
            damage -> set_target( damage -> player -> target );
            damage -> execute();
          } } );
  }

  effect.custom_buff = anodized_deflectors_buff;
}

// Shared Callback for all Titan trinkets
struct titanic_empowerment_cb_t : public dbc_proc_callback_t
{
  std::vector<buff_t*> proc_buffs;

  titanic_empowerment_cb_t( const special_effect_t& effect, std::vector<buff_t*> proc_buffs )
    : dbc_proc_callback_t( effect.player, effect ), proc_buffs( proc_buffs )
  {
  }

  void execute( action_t* /* a */, action_state_t* /* state */ ) override
  {
    for ( auto b : proc_buffs )
    {
      b->trigger();
    }
  }
};

// Void-Twisted Titanshard
// Implement as stat buff instead of absorb. If damage taken events are used again this would need to be changed.
void items::voidtwisted_titanshard( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, "void_shroud" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "void_shroud", effect.player->find_spell( 315774 ), effect.item );

    timespan_t duration_override =
        buff->buff_duration() * effect.player->sim->bfa_opts.voidtwisted_titanshard_percent_duration;

    buff->set_duration( duration_override );
  }

  auto titanic_cb = effect.player->callbacks.get_first_of<titanic_empowerment_cb_t>();

  if ( !titanic_cb )
  {
    titanic_cb = new titanic_empowerment_cb_t( effect, {buff} );
  }
  else
    titanic_cb->proc_buffs.push_back( buff );
}

/**Vita-Charged Titanshard
 * id=315586 driver
 * id=315787 personal buff
 * id=316522 party buff
 * party haste is not implemented
 */
void items::vitacharged_titanshard( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, "vita_charged" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "vita_charged", effect.player->find_spell( 315787 ), effect.item )
               ->set_cooldown( 0_ms )
               ->set_chance( 1.0 );
  }

  auto titanic_cb = effect.player->callbacks.get_first_of<titanic_empowerment_cb_t>();
  if ( !titanic_cb )
  {
    titanic_cb = new titanic_empowerment_cb_t( effect, {buff} );
  }
  else
    titanic_cb->proc_buffs.push_back( buff );
}

// Manifesto of Madness

void items::manifesto_of_madness( special_effect_t& effect )
{
  auto first_buff  = buff_t::find( effect.player, "manifesto_of_madness_chapter_one" );
  auto second_buff = buff_t::find( effect.player, "manifesto_of_madness_chapter_two" );

  if ( !second_buff )
  {
    second_buff = make_buff<stat_buff_t>( effect.player, "manifesto_of_madness_chapter_two",
                                          effect.player->find_spell( 314040 ), effect.item )
                      ->add_stat( STAT_VERSATILITY_RATING,

                                  effect.player->find_spell( 314040 )->effectN( 3 ).average( effect.item ) *
                                      effect.player->sim->bfa_opts.manifesto_allies_end );
  }
  if ( !first_buff )
  {
    first_buff =
        make_buff<stat_buff_t>( effect.player, "manifesto_of_madness_chapter_one", effect.driver(), effect.item )
            ->add_stat( STAT_CRIT_RATING, effect.driver()->effectN( 5 ).average( effect.item ) *
                                              ( 1.0 - effect.driver()->effectN( 3 ).percent() *
                                                          effect.player->sim->bfa_opts.manifesto_allies_start ) )
            ->set_stack_change_callback( [second_buff]( buff_t*, int, int new_ ) {
              if ( new_ == 0 )
                second_buff->trigger();
            } );
  }
  effect.custom_buff = first_buff;
}

// Whispering Eldritch Bow
void items::whispering_eldritch_bow( special_effect_t& effect )
{
  struct whispered_truths_callback_t : public dbc_proc_callback_t
  {
    std::vector<cooldown_t*> cooldowns;
    timespan_t amount;

    whispered_truths_callback_t( const special_effect_t& effect )
      : dbc_proc_callback_t( effect.item, effect ),
        amount( timespan_t::from_millis( -effect.driver()->effectN( 1 ).base_value() ) )
    {
      for ( action_t* a : effect.player->action_list )
      {
        if ( util::is_adjustable_class_spell( a ) &&
             range::find( cooldowns, a->cooldown ) == cooldowns.end() )
        {
          cooldowns.push_back( a->cooldown );
        }
      }
    }

    void execute( action_t*, action_state_t* ) override
    {
      if ( !rng().roll( effect.player->sim->bfa_opts.whispered_truths_offensive_chance ) )
        return;

      const auto it = range::partition( cooldowns, &cooldown_t::down );
      auto down_cooldowns = ::util::make_span( cooldowns ).subspan( 0, it - cooldowns.begin() );
      if ( !down_cooldowns.empty() )
      {
        cooldown_t* chosen = down_cooldowns[ rng().range( down_cooldowns.size() ) ];
        chosen->adjust( amount );

        if ( effect.player->sim->debug )
        {
          effect.player->sim->out_debug.print( "{} of {} adjusted cooldown for {}, remains={}", effect.item->name(),
                                               effect.player->name(), chosen->name(), chosen->remains() );
        }
      }
    }
  };

  if ( effect.player->type == HUNTER )
    new whispered_truths_callback_t( effect );
}

/**Psyche Shredder
 * id=313640 driver and damage from hitting the debuffed target
 * id=313663 debuff and initial damage
 * id=313627 contains proc flags and ICD for debuff damage
 * id=316019 debuff damage comes from this in combat logs
             but the damage effect here is not used
 * TODO: There are some strange delays of longer than the 0.5s ICD that sometimes occur
 */
struct shredded_psyche_cb_t : public dbc_proc_callback_t
{
  action_t* damage;
  player_t* target;

  shredded_psyche_cb_t( const special_effect_t& effect, action_t* d, player_t* t )
    : dbc_proc_callback_t( effect.player, effect ), damage( d ), target( t )
  {
  }

  void trigger( action_t* a, action_state_t* state ) override
  {
    if ( state->target != target )
    {
      return;
    }

    auto td        = a->player->get_target_data( target );
    buff_t* debuff = td->debuff.psyche_shredder;
    if ( !debuff->check() )
      return;

    dbc_proc_callback_t::trigger( a, state );
  }

  void execute( action_t* /* a */, action_state_t* trigger_state ) override
  {
    damage->target = trigger_state->target;
    damage->execute();
  }
};

struct shredded_psyche_t : public proc_t
{
  shredded_psyche_t( const special_effect_t& e ) : proc_t( e, "shredded_psyche", 316019 )
  {
    base_dd_min = e.player->find_spell( 313640 )->effectN( 2 ).min( e.item );
    base_dd_max = e.player->find_spell( 313640 )->effectN( 2 ).max( e.item );
  }
};

struct psyche_shredder_constructor_t : public item_targetdata_initializer_t
{
  psyche_shredder_constructor_t( unsigned iid, ::util::span<const slot_e> s )
    : item_targetdata_initializer_t( iid, s )
  {}

  void operator()( actor_target_data_t* td ) const override
  {
    const special_effect_t* effect = find_effect( td->source );
    if ( !effect )
    {
      td->debuff.psyche_shredder = make_buff( *td, "psyche_shredder_debuff" );
      return;
    }
    assert( !td->debuff.psyche_shredder );

    td->debuff.psyche_shredder = make_buff( *td, "psyche_shredder_debuff", td->source->find_spell( 313663 ) );
    td->debuff.psyche_shredder->reset();

    auto damage_spell = td->source->find_action( "shredded_psyche" );

    auto cb_driver      = new special_effect_t( td->source );
    cb_driver->name_str = "shredded_psyche_driver";
    cb_driver->spell_id = 313627;
    td->source->special_effects.push_back( cb_driver );

    auto callback = new shredded_psyche_cb_t( *cb_driver, damage_spell, td->target );
    callback->initialize();
    callback->deactivate();
    td->debuff.psyche_shredder->set_stack_change_callback( util::callback_buff_activator( callback ) );
  }
};

void items::psyche_shredder( special_effect_t& effect )
{
  struct psyche_shredder_t : public proc_t
  {
    psyche_shredder_t( const special_effect_t& e ) : proc_t( e, "psyche_shredder", 313663 )
    {
    }

    void impact( action_state_t* state ) override
    {
      proc_t::impact( state );
      auto td = player->get_target_data( state->target );
      td->debuff.psyche_shredder->trigger();
    }
  };

  // applies the debuff and deals the initial damage
  effect.execute_action = create_proc_action<psyche_shredder_t>( "psyche_shredder", effect );

  // Create the action for the debuff damage here, before any debuff initialization takes place.
  create_proc_action<shredded_psyche_t>( "shredded_psyche", effect );

  new dbc_proc_callback_t( effect.player, effect );
}

// id=313087 driver
//  - effect 2: stacking dmg
//  - effect 4: damage amp per stack
// id=313088 stacking player buff
// id=313089 aoe proc at 12 stacks
void items::torment_in_a_jar( special_effect_t& effect )
{
  struct unleashed_agony_t : public proc_t
  {
    double dmg_mod;
    buff_t* buff;

    unleashed_agony_t( const special_effect_t& effect, double dmg_mod, buff_t* buff )
      : proc_t( effect, "unleashed_agony", 313088 ), dmg_mod( dmg_mod ), buff( buff )
    {
      base_dd_min = base_dd_max = effect.driver()->effectN( 2 ).average( effect.item );
    }

    double base_da_min( const action_state_t* ) const override
    {
      return base_dd_min * ( 1 + dmg_mod * buff->stack() );
    }

    double base_da_max( const action_state_t* s ) const override
    {
      return base_da_min( s );
    }
  };

  struct unleashed_agony_cb_t : public dbc_proc_callback_t
  {
    action_t* stacking_damage;

    unleashed_agony_cb_t( const special_effect_t& effect ) : dbc_proc_callback_t( effect.player, effect )
    {
      stacking_damage = create_proc_action<unleashed_agony_t>(
          "unleashed_agony", effect, effect.driver()->effectN( 4 ).percent(), effect.custom_buff );
      stacking_damage->add_child( effect.execute_action );
    }

    void execute( action_t* a, action_state_t* s ) override
    {
      stacking_damage->set_target( s->target );
      stacking_damage->execute();

      dbc_proc_callback_t::execute( a, s );
    }
  };

  auto buff = buff_t::find( effect.player, "unleashed_agony" );
  if ( !buff )
  {
    buff = make_buff( effect.player, "unleashed_agony", effect.player->find_spell( 313088 ) );
  }
  effect.custom_buff = buff;

  effect.execute_action = create_proc_action<proc_t>( "explosion_of_agony", effect, "explosion_of_agony", 313089 );

  new unleashed_agony_cb_t( effect );
}

void items::writhing_segment_of_drestagath( special_effect_t& effect )
{
  effect.execute_action =
      create_proc_action<aoe_proc_t>( "spine_eruption", effect, "spine_eruption", effect.driver(), true );
}

/**Draconic Empowerment
 * id=317860 driver
 * id=317859 buff
 */
void items::draconic_empowerment( special_effect_t& effect )
{
  effect.custom_buff = buff_t::find( effect.player, "draconic_empowerment" );
  if ( !effect.custom_buff )
    effect.custom_buff =
        make_buff<stat_buff_t>( effect.player, "draconic_empowerment", effect.player->find_spell( 317859 ) )
            ->add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ),
                        effect.player->find_spell( 317859 )->effectN( 1 ).average( effect.player ) );

  effect.proc_flags_ = PF_ALL_DAMAGE | PF_ALL_HEAL | PF_PERIODIC;  // Proc flags are missing in spell data.

  new dbc_proc_callback_t( effect.player, effect );
}

// Waycrest's Legacy Set Bonus ============================================

void set_bonus::waycrest_legacy( special_effect_t& effect )
{
  auto effect_heal = unique_gear::find_special_effect( effect.player, 271631, SPECIAL_EFFECT_EQUIP );
  if ( effect_heal != nullptr )
  {
    effect_heal->execute_action->add_child( new waycrest_legacy_heal_t( effect ) );
  }

  auto effect_damage = unique_gear::find_special_effect( effect.player, 271683, SPECIAL_EFFECT_EQUIP );
  if ( effect_damage != nullptr )
  {
    effect_damage->execute_action->add_child( new waycrest_legacy_damage_t( effect ) );
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

void set_bonus::titanic_empowerment( special_effect_t& effect )
{
  if ( effect.player->sim->bfa_opts.nyalotha )
  {
    auto buff = static_cast<stat_buff_t*>( buff_t::find( effect.player, "titanic_empowerment" ) );
    if ( !buff )
    {
      auto vita_shard = effect.player->find_item_by_id( 174500 );
      auto void_shard = effect.player->find_item_by_id( 174528 );
      if ( !vita_shard || !void_shard )
      {
        // Don't attempt to guess the correct stat amount when the set bonus is enabled manually.
        effect.player->sim->print_debug( "Titanic Empowerment requires both trinkets to be equipped, disabling." );
        return;
      }

      int average_ilvl = ( vita_shard->item_level() + void_shard->item_level() ) / 2;
      buff = make_buff<stat_buff_t>( effect.player, "titanic_empowerment", effect.player->find_spell( 315858 ) );
      const auto& budget = effect.player->dbc->random_property( average_ilvl );
      double value       = budget.p_epic[ 0 ] * buff->data().effectN( 1 ).m_coefficient();
      buff->add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ), value );
    }

    auto titanic_cb = effect.player->callbacks.get_first_of<titanic_empowerment_cb_t>();
    if ( !titanic_cb )
    {
      titanic_cb = new titanic_empowerment_cb_t( effect, {buff} );
    }
    else
      titanic_cb->proc_buffs.push_back( buff );
  }
}

// Keepsakes of the Resolute Commandant Set Bonus =========================

void set_bonus::keepsakes_of_the_resolute_commandant( special_effect_t& effect )
{
  new dbc_proc_callback_t( effect.player, effect );
}

void corruption_effect( special_effect_t& effect )
{
  effect.player->sim->print_debug( "{}: disabling corruption {} from item {}",
      effect.player->name(), effect.driver()->name_cstr(), effect.item->name() );

  effect.disable_action();
  effect.disable_buff();
}

}  // namespace bfa
}  // namespace

void unique_gear::register_special_effects_bfa()
{
  using namespace bfa;

  // Consumables
  register_special_effect( 259409, consumables::galley_banquet );
  register_special_effect( 259410, consumables::bountiful_captains_feast );
  register_special_effect( 297048, consumables::famine_evaluator_and_snack_table );
  register_special_effect( 290464, consumables::boralus_blood_sausage );
  register_special_effect( 269853, consumables::potion_of_rising_death );
  register_special_effect( 251316, consumables::potion_of_bursting_blood );
  register_special_effect( 300714, consumables::potion_of_unbridled_fury );
  register_special_effect( 298317, consumables::potion_of_focused_resolve );
  register_special_effect( 298225, consumables::potion_of_empowered_proximity );

  // Enchants
  register_special_effect( 255151, enchants::galeforce_striking );
  register_special_effect( 255150, enchants::torrent_of_elements );
  register_special_effect( 268855, enchants::weapon_navigation( 268856 ) );  // Versatile Navigation
  register_special_effect( 268888, enchants::weapon_navigation( 268893 ) );  // Quick Navigation
  register_special_effect( 268900, enchants::weapon_navigation( 268898 ) );  // Masterful Navigation
  register_special_effect( 268906, enchants::weapon_navigation( 268904 ) );  // Deadly Navigation
  register_special_effect( 268912, enchants::weapon_navigation( 268910 ) );  // Stalwart Navigation
  register_special_effect( 264876, "264878Trigger" );                        // Crow's Nest Scope
  register_special_effect( 264958, "264957Trigger" );                        // Monelite Scope of Alacrity
  register_special_effect( 265090, "265092Trigger" );                        // Incendiary Ammunition
  register_special_effect( 265094, "265096Trigger" );                        // Frost-Laced Ammunition
  register_special_effect( 300718, enchants::machinists_brilliance );
  register_special_effect( 300795, enchants::force_multiplier );

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
  register_special_effect( 270809, items::vessel_of_skittering_shadows );
  register_special_effect( 278053, items::vigilants_bloodshaper );
  register_special_effect( 270921, items::hadals_nautilus );
  register_special_effect( 268035, items::lingering_sporepods );
  register_special_effect( 271117, "4Tick" );
  register_special_effect( 271631, items::lady_waycrests_music_box );
  register_special_effect( 271683, items::lady_waycrests_music_box_heal );
  register_special_effect( 268999, items::balefire_branch );
  register_special_effect( 268314,
                           "268311Trigger" );  // Galecaller's Boon, assumes the player always stands in the area
  register_special_effect( 278140, items::frenetic_corpuscle );
  register_special_effect( 278383, "Reverse" );  // Azurethos' Singed Plumage
  register_special_effect( 278154, items::twitching_tentacle_of_xalzaix );
  register_special_effect( 278161, items::vanquished_tendril_of_ghuun );
  register_special_effect( 268828, items::vial_of_animated_blood );
  register_special_effect( 268191, items::briny_barnacle );
  register_special_effect( 278267, items::sandscoured_idol );
  register_special_effect( 278109, items::syringe_of_bloodborne_infirmity );
  register_special_effect( 278112, items::syringe_of_bloodborne_infirmity );
  register_special_effect( 278152, items::disc_of_systematic_regression );
  register_special_effect( 274472, items::berserkers_juju );
  register_special_effect( 285495, "285496Trigger" );  // Moonstone of Zin-Azshari
  register_special_effect( 289522, items::incandescent_sliver );
  register_special_effect( 289885, items::tidestorm_codex );
  register_special_effect( 289521, items::invocation_of_yulon );
  register_special_effect( 288328, "288330Trigger" );  // Kimbul's Razor Claw
  register_special_effect( 287915, items::variable_intensity_gigavolt_oscillating_reactor );
  register_special_effect( 287917, items::variable_intensity_gigavolt_oscillating_reactor_onuse );
  register_special_effect( 289525, items::everchill_anchor );
  register_special_effect( 288173, items::ramping_amplitude_gigavolt_engine );
  register_special_effect( 289520, items::grongs_primal_rage );
  register_special_effect( 295391, items::harbingers_inscrutable_will );
  register_special_effect( 295812, items::leggings_of_the_aberrant_tidesage );
  register_special_effect( 295254, items::fathuuls_floodguards );
  register_special_effect( 295175, items::grips_of_forsaken_sanity );
  register_special_effect( 295277, items::stormglide_steps );
  register_special_effect( 295962, items::idol_of_indiscriminate_consumption );
  register_special_effect( 295501, items::lurkers_insidious_gift );
  register_special_effect( 295430, items::abyssal_speakers_gauntlets );
  register_special_effect( 292650, items::trident_of_deep_ocean );
  register_special_effect( 295427, items::legplates_of_unbound_anguish );
  // Nazjatar & Eternal Palace only effects
  register_special_effect( 302382, items::damage_to_aberrations );
  register_special_effect( 303133, items::exploding_pufferfish );
  register_special_effect( 304637, items::fathom_hunter );
  register_special_effect( 304697, items::nazjatar_proc_check );  // Conch Strike
  register_special_effect( 304640, items::nazjatar_proc_check );  // Frost Blast
  register_special_effect( 304711, items::nazjatar_proc_check );  // Sharp Fins
  register_special_effect( 304715, items::nazjatar_proc_check );  // Tidal Droplet
  register_special_effect( 303733, items::storm_of_the_eternal_arcane_damage );
  register_special_effect( 303734, items::storm_of_the_eternal_stats );
  register_special_effect( 303735, items::storm_of_the_eternal_stats );
  // 8.2 Special Effects
  register_special_effect( 300830, items::highborne_compendium_of_sundering );
  register_special_effect( 300913, items::highborne_compendium_of_storms );
  register_special_effect( 300612, items::neural_synapse_enhancer );
  register_special_effect( 301834, items::shiver_venom_relic_onuse );  // on-use
  register_special_effect( 301576, items::shiver_venom_relic_equip );  // equip proc
  register_special_effect( 302773, items::leviathans_lure );
  register_special_effect( 306146, items::aquipotent_nautilus );
  register_special_effect( 302696, items::zaquls_portal_key );
  register_special_effect( 303277, items::vision_of_demise );
  register_special_effect( 296971, items::azsharas_font_of_power );
  register_special_effect( 304471, items::arcane_tempest );
  register_special_effect( 302986, items::anuazshara_staff_of_the_eternal );
  register_special_effect( 303358, items::shiver_venom_crossbow );
  register_special_effect( 303361, items::shiver_venom_lance );
  register_special_effect( 303564, items::ashvanes_razor_coral );
  register_special_effect( 296963, items::dribbling_inkpod );
  register_special_effect( 300142, items::hyperthread_wristwraps );
  register_special_effect( 300140, items::anodized_deflectors );
  register_special_effect( 301753, items::reclaimed_shock_coil );
  register_special_effect( 303356, items::dreams_end );
  register_special_effect( 303353, items::divers_folly );
  register_special_effect( 302307, items::remote_guidance_device );
  register_special_effect( 305252, items::gladiators_maledict );
  register_special_effect( 281712, items::getiikku_cut_of_death );
  register_special_effect( 281720, items::bilestained_crawg_tusks );
  // 8.2 Mechagon combo rings
  register_special_effect( 300124, items::logic_loop_of_division );
  register_special_effect( 300125, items::logic_loop_of_recursion );
  register_special_effect( 299909, items::logic_loop_of_maintenance );
  register_special_effect( 300126, items::overclocking_bit_band );
  register_special_effect( 300127, items::shorting_bit_band );
  // Passive two-stat punchcards
  register_special_effect( 306402, items::yellow_punchcard );
  register_special_effect( 306403, items::yellow_punchcard );
  register_special_effect( 303590, items::yellow_punchcard );
  register_special_effect( 306406, items::yellow_punchcard );
  register_special_effect( 303595, items::yellow_punchcard );
  register_special_effect( 306407, items::yellow_punchcard );
  register_special_effect( 306405, items::yellow_punchcard );
  register_special_effect( 306404, items::yellow_punchcard );
  register_special_effect( 303592, items::yellow_punchcard );
  register_special_effect( 306410, items::yellow_punchcard );
  register_special_effect( 303596, items::yellow_punchcard );
  register_special_effect( 306409, items::yellow_punchcard );
  register_special_effect( 293136, items::subroutine_overclock );
  register_special_effect( 299062, items::subroutine_recalibration );
  register_special_effect( 299464, items::subroutine_optimization );
  register_special_effect( 293512, items::harmonic_dematerializer );
  register_special_effect( 293491, items::cyclotronic_blast );

  // Misc
  register_special_effect( 276123, items::darkmoon_deck_squalls );
  register_special_effect( 276176, items::darkmoon_deck_fathoms );
  register_special_effect( 265440, items::endless_tincture_of_fractional_power );

  /* 8.0 Dungeon Set Bonuses*/
  register_special_effect( 277522, set_bonus::waycrest_legacy );

  /* 8.1.0 Raid Set Bonuses */
  register_special_effect( 290263, set_bonus::gift_of_the_loa );
  register_special_effect( 290362, set_bonus::keepsakes_of_the_resolute_commandant );

  /* 8.3.0 Corruptions */
  register_special_effect( 315529, corruption_effect );
  register_special_effect( 315530, corruption_effect );
  register_special_effect( 315531, corruption_effect );
  register_special_effect( 315544, corruption_effect );
  register_special_effect( 315545, corruption_effect );
  register_special_effect( 315546, corruption_effect );
  register_special_effect( 315549, corruption_effect );
  register_special_effect( 315552, corruption_effect );
  register_special_effect( 315553, corruption_effect );
  register_special_effect( 315554, corruption_effect );
  register_special_effect( 315557, corruption_effect );
  register_special_effect( 315558, corruption_effect );
  register_special_effect( 318303, corruption_effect );
  register_special_effect( 318484, corruption_effect );
  register_special_effect( 318276, corruption_effect );
  register_special_effect( 318477, corruption_effect );
  register_special_effect( 318478, corruption_effect );
  register_special_effect( 318266, corruption_effect );
  register_special_effect( 318492, corruption_effect );
  register_special_effect( 318496, corruption_effect );
  register_special_effect( 318269, corruption_effect );
  register_special_effect( 318494, corruption_effect );
  register_special_effect( 318498, corruption_effect );
  register_special_effect( 318268, corruption_effect );
  register_special_effect( 318493, corruption_effect );
  register_special_effect( 318497, corruption_effect );
  register_special_effect( 318270, corruption_effect );
  register_special_effect( 318495, corruption_effect );
  register_special_effect( 318499, corruption_effect );
  register_special_effect( 315277, corruption_effect );
  register_special_effect( 315281, corruption_effect );
  register_special_effect( 315282, corruption_effect );
  register_special_effect( 318239, corruption_effect );
  register_special_effect( 315574, corruption_effect );
  register_special_effect( 318274, corruption_effect );
  register_special_effect( 318487, corruption_effect );
  register_special_effect( 318488, corruption_effect );
  register_special_effect( 318280, corruption_effect );
  register_special_effect( 318485, corruption_effect );
  register_special_effect( 318486, corruption_effect );
  register_special_effect( 318294, corruption_effect );
  register_special_effect( 318272, corruption_effect );
  register_special_effect( 318286, corruption_effect );
  register_special_effect( 318479, corruption_effect );
  register_special_effect( 318480, corruption_effect );
  register_special_effect( 318293, corruption_effect );
  register_special_effect( 318481, corruption_effect );
  register_special_effect( 318482, corruption_effect );
  register_special_effect( 318483, corruption_effect );
  register_special_effect( 317290, corruption_effect );
  register_special_effect( 318299, corruption_effect );
  register_special_effect( 316651, corruption_effect );

  // 8.3 Special Effects
  register_special_effect( 315736, items::voidtwisted_titanshard );
  register_special_effect( 315586, items::vitacharged_titanshard );
  register_special_effect( 313948, items::manifesto_of_madness );
  register_special_effect( 316780, items::whispering_eldritch_bow );
  register_special_effect( 313640, items::psyche_shredder );
  register_special_effect( 313087, items::torment_in_a_jar );
  register_special_effect( 317860, items::draconic_empowerment );
  register_special_effect( 313113, items::writhing_segment_of_drestagath );

  // 8.3 Set Bonus(es)
  register_special_effect( 315793, set_bonus::titanic_empowerment );
}

void unique_gear::register_target_data_initializers_bfa( sim_t* sim )
{
  using namespace bfa;
  static constexpr std::array<slot_e, 2> items {{ SLOT_TRINKET_1, SLOT_TRINKET_2 }};

  sim->register_target_data_initializer( deadeye_spyglass_constructor_t( 159623, items ) );
  sim->register_target_data_initializer( syringe_of_bloodborne_infirmity_constructor_t( 160655, items ) );
  sim->register_target_data_initializer( everchill_anchor_constructor_t( 165570, items ) );
  sim->register_target_data_initializer( briny_barnacle_constructor_t( 159619, items ) );
  sim->register_target_data_initializer( potion_of_focused_resolve_t() );
  sim->register_target_data_initializer( razor_coral_constructor_t( 169311, items ) );
  sim->register_target_data_initializer( conductive_ink_constructor_t( 169319, items ) );
  sim->register_target_data_initializer( luminous_algae_constructor_t( 169304, items ) );
  sim->register_target_data_initializer( psyche_shredder_constructor_t( 174060, items ) );
}

void unique_gear::register_hotfixes_bfa()
{
}

namespace expansion
{
namespace bfa
{
static std::unordered_map<unsigned, stat_e> __ls_cb_map{{

}};

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

}  // namespace bfa
}  // namespace expansion
