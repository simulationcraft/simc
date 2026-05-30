// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "unique_gear_midnight.hpp"

#include "action/absorb.hpp"
#include "action/action.hpp"
#include "action/dot.hpp"
#include "buff/buff.hpp"
#include "dbc/data_enums.hh"
#include "dbc/item_database.hpp"
#include "dbc/spell_data.hpp"
#include "item/item.hpp"
#include "player/actor_target_data.hpp"
#include "player/consumable.hpp"
#include "player/darkmoon_deck.hpp"
#include "player/ground_aoe.hpp"
#include "player/pet_spawner.hpp"
#include "set_bonus.hpp"
#include "sim/cooldown.hpp"
#include "sim/proc_rng.hpp"
#include "sim/sim.hpp"
#include "unique_gear.hpp"
#include "unique_gear_helper.hpp"

namespace unique_gear::midnight
{
std::vector<unsigned> __mid_special_effect_ids;
wowv_t version_min = { 12 };
wowv_t version_max = { UINT8_MAX };

void reset_version_check()
{ version_min = { 12 }; version_max = { UINT8_MAX }; }

void set_min_version( wowv_t build )
{ version_min = build; }

void set_max_version( wowv_t build )
{ version_max = build; }

// from item_naming.inc
enum gem_color_e : unsigned
{
  GEM_PERIDOT = 14262,
  GEM_GARNET = 14276,
  GEM_LAPIS = 14277,
  GEM_AMETHYST = 14278,
  GEM_DIAMOND = 14279
};

static constexpr unsigned __gem_colors[] = { GEM_PERIDOT, GEM_GARNET, GEM_LAPIS, GEM_AMETHYST };
static constexpr util::span<const unsigned> gem_colors = util::make_span( __gem_colors );

// can be called via unqualified lookup
void register_special_effect( unsigned spell_id, custom_cb_t init_callback, bool fallback = false, bool passive = false )
{
  unique_gear::register_special_effect( spell_id, init_callback, fallback, passive, version_min, version_max );
  __mid_special_effect_ids.push_back( spell_id );
}

void register_special_effect( std::initializer_list<unsigned> spell_ids, custom_cb_t init_callback,
                              bool fallback = false, bool passive = false )
{
  for ( auto id : spell_ids )
    register_special_effect( id, init_callback, fallback, passive );
}

namespace consumables
{
// Food
static constexpr unsigned food_coeff_spell_id = 1219179;
using selector_fn = std::function<stat_e( const player_t*, util::span<const stat_e> )>;

struct selector_food_buff_t : public consumable_buff_t<stat_buff_t>
{

  double amount;
  bool highest;

  selector_food_buff_t( const special_effect_t& e, bool b )
    : consumable_buff_t( e.player, e.name(), e.driver() ), highest( b )
  {
    amount = e.stat_amount;
  }

  void start( int s, double v, timespan_t d ) override
  {
    auto stat = highest ? util::highest_stat( player, secondary_ratings )
                        : util::lowest_stat( player, secondary_ratings );

    if( !manual_stats_added )
      add_stat( stat, amount );

    consumable_buff_t::start( s, v, d );
  }
};

custom_cb_t selector_food( unsigned id, bool highest, bool major = true )
{
  return [ = ]( special_effect_t& effect ) {
    effect.spell_id = id;

    auto coeff = effect.player->find_spell( food_coeff_spell_id );

    effect.stat_amount = coeff->effectN( 4 ).average( effect );
    if ( !major )
      effect.stat_amount *= coeff->effectN( 1 ).base_value() * 0.1;

    effect.custom_buff = new selector_food_buff_t( effect, highest );
  };
}

custom_cb_t primary_food( unsigned id, stat_e stat, size_t primary_idx = 3, bool major = true )
{
  return [ = ]( special_effect_t& effect ) {
    effect.spell_id = id;

    auto coeff = effect.player->find_spell( food_coeff_spell_id );

    auto buff = create_buff<consumable_buff_t<stat_buff_t>>( effect.player, effect.driver() );

    if ( primary_idx )
    {
      auto _amt = coeff->effectN( primary_idx ).average( effect );
      if ( !major )
        _amt *= coeff->effectN( 1 ).base_value() * 0.1;

      buff->add_stat( effect.player->convert_hybrid_stat( stat ), _amt );
    }

    if ( primary_idx == 3 )
    {
      auto _amt = coeff->effectN( 8 ).average( effect );
      if ( !major )
        _amt *= coeff->effectN( 1 ).base_value() * 0.1;

      buff->add_stat( STAT_STAMINA, _amt );
    }

    effect.custom_buff = buff;
  };
}

custom_cb_t secondary_food( unsigned id, stat_e stat1, stat_e stat2 = STAT_NONE )
{
  return [ = ]( special_effect_t& effect ) {
    effect.spell_id = id;

    auto coeff = effect.player->find_spell( food_coeff_spell_id );

    auto buff = create_buff<consumable_buff_t<stat_buff_t>>( effect.player, effect.driver() );

    if ( stat2 == STAT_NONE )
    {
      auto _amt = coeff->effectN( 4 ).average( effect );
      buff->add_stat( stat1, _amt );
    }
    else
    {
      auto _amt = coeff->effectN( 5 ).average( effect );
      buff->add_stat( stat1, _amt );
      buff->add_stat( stat2, _amt );
    }

    effect.custom_buff = buff;
  };
}
// Potions
// Draught of Rampant Abandon
// 1236998 driver & buff
// 1237154 AoE trigger (NYI)
void draught_of_rampant_abandon( special_effect_t& effect )
{
  effect.custom_buff = create_buff<stat_buff_t>( effect.player, effect.driver(), effect.item )
    // TODO: RPPM is for the AoE trigger (NYI), disabling for now
    ->set_rppm( rppm_scale_e::RPPM_DISABLE );
}

// Potion of Recklessness
// 1236994 - driver & buff
void potion_of_recklessness( special_effect_t& effect )
{
  // The potion grants a positive buff to your highest secondary stat and
  // a negative buff to your lowest secondary stat.
  struct recklessness_t : public generic_proc_t
  {
    std::unordered_map<stat_e, buff_t*> pos_map, neg_map;

    recklessness_t( const special_effect_t& e ) : generic_proc_t( e, e.driver()->name_cstr(), e.driver() )
    {
      quiet = true;

      // create and populate bonus/penalty maps. penalties will get '_penalty' suffix and ' Penalty' for reporting
      create_all_stat_buffs( e, &data(), 0.0,
        []( std::string n, const spelleffect_data_t& e ) {
          return e.m_coefficient() < 0.0 ? fmt::format( "{}_penalty", n ) : n;
        },
        [ this ]( stat_e s, buff_t* b ) {
          if ( static_cast<stat_buff_t*>( b )->stats.front().amount < 0.0 )
            neg_map[ s ] = b->set_name_reporting( b->name_str_reporting + " Penalty" );
          else
            pos_map[ s ] = b;
        } );
    }

    void execute() override
    {
      generic_proc_t::execute();

      pos_map[ util::highest_stat( player, secondary_ratings ) ]->trigger();
      neg_map[ util::lowest_stat( player, secondary_ratings ) ]->trigger();
    }
  };

  effect.disable_buff();
  effect.execute_action = create_proc_action<recklessness_t>( "potion_of_recklessness", effect );
}

// Potion of Zealotry
// 1238443 Driver & buff
// 1237886 Damage Taken Debuff
// 1237158 Damage
// TODO: Does the debuff trigger before, or after the damage? Order will affect damage output
void potion_of_zealotry( special_effect_t& effect )
{
  struct burst_of_zealotry_t : public generic_proc_t
  {
    const special_effect_t& effect;
    player_t* last_target;

    burst_of_zealotry_t( const special_effect_t& e )
      : generic_proc_t( e, "burst_of_zealotry", 1237158 ), effect( e ), last_target( nullptr )
    {
      base_dd_min = base_dd_max = e.driver()->effectN( 3 ).average( e );
      target_debuff             = e.driver()->effectN( 1 ).trigger();
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = generic_proc_t::composite_da_multiplier( s );

      if ( auto debuff = find_debuff( s->target ) )
        m *= 1.0 + debuff->check() * effect.driver()->effectN( 4 ).percent();

      return m;
    }

    void impact( action_state_t* s ) override
    {
      generic_proc_t::impact( s );
      if ( s->target != last_target )
      {
        last_target = s->target;

        for ( auto& t : target_list() )
          if ( auto debuff = find_debuff( t ) )
            debuff->expire();
      }

      get_debuff( s->target )->trigger();
    }
  };

  auto buff = create_buff<buff_t>( effect.player, "potion_of_zealotry", effect.driver() )->set_rppm( RPPM_DISABLE );

  auto burst_of_zealotry            = new special_effect_t( effect.player );
  burst_of_zealotry->name_str       = "burst_of_zealotry_proc";
  burst_of_zealotry->spell_id       = effect.driver()->id();
  burst_of_zealotry->cooldown_      = 0_ms; // Cooldown handled by the main special effect
  burst_of_zealotry->execute_action = create_proc_action<burst_of_zealotry_t>( "burst_of_zealotry", effect );
  effect.player->special_effects.push_back( burst_of_zealotry );

  auto zealotry_cb = new dbc_proc_callback_t( effect.player, *burst_of_zealotry );
  zealotry_cb->activate_with_buff( buff, true );

  effect.custom_buff = buff;
}

// 1262056 r1 driver
// 1262108 r1 dot
// 1262111 r2 driver
// 1262109 r2 dot
void laced_zoomshots( special_effect_t& effect )
{
  effect.player->sim->error( UNVERIFIED_IMPLEMENTATION,
    "Laced Zoomshots: Implemented using spell data which indicates it procs from all damage, not just auto-attacks. "
    "This has not be verified in-game." );

  effect.execute_action = create_proc_action<generic_proc_t>( "laced_zoomshots", effect, effect.trigger() );

  new dbc_proc_callback_t( effect.player, effect );
}

// 1237009 r1 driver
// 1237010 r1 damage
// 1237012 r2 driver
// 1237013 r2 damage
void smugglers_enchanted_edge( special_effect_t& effect )
{
  effect.player->sim->error( UNVERIFIED_IMPLEMENTATION,
    "Smuggler's Enchanted Edge: Implemented assuming that coating both weapons will results in two indepedent procs. "
    "This has not be verified in-game." );

  auto dam = create_proc_action<generic_proc_t>( "smugglers_enchanted_edge", effect, effect.trigger() );
  dam->base_dd_min = dam->base_dd_max = effect.driver()->effectN( 1 ).average( effect );

  effect.execute_action = dam;

  new dbc_proc_callback_t( effect.player, effect );
}

// 1262120 r1 driver
// 1262140 r1 missile
// 1262142 r1 aoe
// 1262141 r2 driver
// 1262147 r2 missile
// 1262146 r2 aoe
void weighted_boomshots( special_effect_t& effect )
{
  auto missile = create_proc_action<generic_proc_t>( "weighted_boomshots_missile", effect, effect.trigger() );

  // assumed to not split so use generic_proc_t and just set aoe = -1;
  auto aoe = create_proc_action<generic_proc_t>( "weighted_boomshots", effect, missile->data().effectN( 1 ).trigger() );
  aoe->aoe = -1;
  missile->stats = aoe->stats;  // just report the damage
  missile->impact_action = aoe;

  effect.execute_action = missile;

  new dbc_proc_callback_t( effect.player, effect );
}
}  // namespace consumables

namespace enchants
{
// Powerful Eversong Diamond
// 1258209 driver
void powerful_eversong_diamond( special_effect_t& effect )
{
  auto pct = effect.driver()->effectN( 1 ).base_value() * unique_gem_list( effect.player, gem_colors ).size();

  effect.player->register_passive_item_effect_override( effect.driver()->effectN( 2 ), pct );
  effect.player->register_passive_item_effect_override( effect.driver()->effectN( 3 ), pct );
  effect.player->parse_passive_item_effect( effect.driver() );
}

// Strength of Halazzi
// 1236733 Rank 1 Driver
// 1236734 Rank 2 Driver
// 1241721 RPPM
// 1241784 Damage
void strength_of_halazzi( special_effect_t& effect )
{
  effect.player->sim->error( IMPLEMENTATION_NOTES,
    "Strength of the Halazzi: Currently not penalized by tank role multiplier in-game, and implemented to match." );

  auto proc_data = effect.trigger()->effectN( 1 ).trigger();
  auto proc_value = effect.driver()->effectN( 1 ).average( effect );

  assert( proc_data->effectN( 1 ).subtype() == A_PERIODIC_DAMAGE );

  auto damage = create_proc_action<generic_proc_t>( "halazzis_claws", effect, proc_data );
  damage->base_td += proc_value;

  // skip setup if callback has been created by already having another copy of the enchant
  if ( find_special_effect( effect.player, effect.trigger()->id() ) )
    return;

  // No Role Mult currently
  // damage->base_ta_multiplier *= role_mult( effect );

  effect.execute_action = damage;
  effect.spell_id = effect.trigger()->id();

  new dbc_proc_callback_t( effect.player, effect );
}

// 1236739 r1 driver
// 1236740 r2 driver
// 1241728 rppm
// 1242127 dot
// 1277263 unknonw (vfx?)
// 1242129 aoe
void flames_of_the_sindorei( special_effect_t& effect )
{
  struct phoenix_fire_t : public generic_proc_t
  {
    action_t* aoe;

    phoenix_fire_t( const special_effect_t& e, action_t* aoe )
      : generic_proc_t( e, "phoenix_fire", e.trigger()->effectN( 1 ).trigger() ), aoe( aoe )
    {
      assert( data().effectN( 1 ).subtype() == A_PERIODIC_DAMAGE );
    }

    void last_tick( dot_t* d ) override
    {
      generic_proc_t::last_tick( d );

      aoe->execute_on_target( d->target );
    }
  };

  auto dot_value = effect.driver()->effectN( 1 ).average( effect );
  auto aoe_value = effect.driver()->effectN( 2 ).average( effect );

  auto aoe = create_proc_action<generic_aoe_proc_t>( "phoenix_fire_aoe", effect, 1242129 );
  aoe->name_str_reporting = "AoE";
  aoe->base_dd_min += aoe_value;
  aoe->base_dd_max += aoe_value;

  auto dot = create_proc_action<phoenix_fire_t>( "phoenix_fire", effect, aoe );
  dot->base_td += dot_value;

  // skip setup if callback has been created by already having another copy of the enchant
  if ( find_special_effect( effect.player, effect.trigger()->id() ) )
    return;

  dot->add_child( aoe );

  effect.execute_action = dot;
  effect.spell_id = effect.trigger()->id();

  new dbc_proc_callback_t( effect.player, effect );
}

void stat_weapon_enchant( special_effect_t& effect )
{
  auto proc_value = effect.driver()->effectN( 1 ).average( effect );
  auto proc_data = effect.trigger()->effectN( 1 ).trigger();
  auto proc_subtype = proc_data->effectN( 1 ).subtype();

  assert( proc_value && ( proc_subtype == A_MOD_RATING || proc_subtype == A_MOD_STAT ) );

  auto buff = create_buff<stat_buff_t>( effect.player, proc_data )
    ->add_stat_from_effect_type( proc_subtype, proc_value );

  // skip setup if callback has been created by already having another copy of the enchant
  if ( find_special_effect( effect.player, effect.trigger()->id() ) )
    return;

  effect.custom_buff = buff;
  effect.spell_id = effect.trigger()->id();

  new dbc_proc_callback_t( effect.player, effect );
}

// Eyes of the Eagle
// 1236700 Driver rank 1
// 1236701 Driver rank 2
void eyes_of_the_eagle( special_effect_t& effect )
{
  effect.player->parse_passive_item_effect( effect.driver() );
}

// 1262295 r1 driver
// 1262294 r1 buff
// 1262298 r2 driver
// 1262299 r2 buff
void smugglers_lynxeye( special_effect_t& effect )
{
  effect.custom_buff = create_buff<stat_buff_t>( effect.player, effect.trigger(), effect.item );

  new dbc_proc_callback_t( effect.player, effect );
}

// 1262337 r1 driver
// 1262336 r1 buff
// 1262339 r2 driver
// 1262338 r2 buff
void farstriders_hawkeye( special_effect_t& effect )
{
  effect.custom_buff = create_buff<stat_buff_t>( effect.player, effect.trigger(), effect.item );

  new dbc_proc_callback_t( effect.player, effect );
}
}  // namespace enchants

namespace embellishments
{
// 1283697 Driver
// 1229511 rppm
// 1229746 buff
void arcanoweave_lining( special_effect_t& effect )
{
  // EffectN 2 percent goes to the player
  // EffectN 3 percent goes to the ally
  auto buff_amount = effect.driver()->effectN( 1 ).average( effect ) * effect.driver()->effectN( 2 ).percent();

  auto buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 1229746 ) )
                  ->add_stat_from_effect_type( A_MOD_STAT, buff_amount );

  // skip setup if callback has been created by already having another copy of the embellishment
  if ( find_special_effect( effect.player, effect.trigger()->id() ) )
    return;

  struct arcanoweave_lining_cb_t : public dbc_proc_callback_t
  {
    stat_buff_t* buff;
    double ally_conversion_multiplier;

    arcanoweave_lining_cb_t( const special_effect_t& e, stat_buff_t* personal_buff )
      : dbc_proc_callback_t( e.player, e ),
        buff( personal_buff ),
        ally_conversion_multiplier( e.driver()->effectN( 3 ).percent() / e.driver()->effectN( 2 ).percent() )
    {}

    buff_t* create_debuff( player_t* t ) override
    {
      // don't cache in ctor so we can grab double value for double embellishment
      auto ally_stat_amount = buff->stats.front().amount * ally_conversion_multiplier;

      return make_buff<stat_buff_t>( actor_pair_t( t, listener ), "arcanoweave_insight_ally", &buff->data() )
        ->set_stat_from_effect_type( A_MOD_STAT, ally_stat_amount )
        ->set_name_reporting( "Ally" );
    }

    void execute( const spell_data_t*, player_t*, action_state_t* ) override
    {
      buff->trigger();

      if ( effect.player->sim->player_non_sleeping_list.size() > 1 && !effect.player->sim->single_actor_batch )
      {
        auto allies = effect.player->sim->player_non_sleeping_list.data();  // make a copy
        range::erase_remove( allies, [ p = effect.player ]( player_t* t ) { return t->is_pet() || t == p; } );

        if ( !allies.empty() )
          get_debuff( rng().range( allies ) )->trigger();
      }
    }
  };

  effect.spell_id = effect.trigger()->id();

  new arcanoweave_lining_cb_t( effect, buff );
}

// 1241711 driver
// 1230364 rppm driver
// 1230366 buff
void sunfire_silk_lining( special_effect_t& effect )
{
  auto stat_amount = effect.driver()->effectN( 1 ).average( effect );

  auto buff = create_buff<stat_buff_t>( effect.player, effect.trigger()->effectN( 1 ).trigger() )
    ->add_stat_from_effect_type( A_MOD_STAT, stat_amount );

  // skip setup if callback has been created by already having another copy of the embellishment
  if ( find_special_effect( effect.player, effect.trigger()->id() ) )
    return;

  effect.custom_buff = buff;
  effect.spell_id    = effect.trigger()->id();

  new dbc_proc_callback_t( effect.player, effect );
}

// 1244238 driver
// 1259213 rppm driver
// 1259216 damage missile
// 1259218 damage
// 1259229 buff missile
// 1259230 buff
void devouring_banding( special_effect_t& effect )
{
  auto proc_damage = effect.driver()->effectN( 1 ).average( effect );

  struct seized_power_t : public generic_proc_t
  {
    std::unordered_map<stat_e, buff_t*> buffs;

    seized_power_t( const special_effect_t& e, std::unordered_map<stat_e, buff_t*> map )
      : generic_proc_t( e, "seized_power", 1259229 ), buffs( std::move( map ) )
    {
      quiet = true;
    }

    void impact( action_state_t* ) override
    {
      auto stat = util::highest_stat( player, secondary_ratings );
      for ( auto [ s, b ] : buffs )
      {
        if ( s == stat )
          b->trigger();
        else
          b->expire();
      }
    }
  };

  // 1259213 damage driver
  // 1259216 missile
  // 1259218 damage
  struct devouring_bolt_t : public generic_proc_t
  {
    devouring_bolt_t( const special_effect_t& e ) : generic_proc_t( e, "devouring_bolt_missile", 1259216 )
    {
      dual = true;

      impact_action = create_proc_action<generic_proc_t>( "devouring_bolt", e, data().effectN( 1 ).trigger() );
      stats         = impact_action->stats;  // report the damage only
    }
  };

  std::unordered_map<stat_e, buff_t*> buffs;

  create_all_stat_buffs( effect, effect.player->find_spell( 1259230 ), 0, [ &buffs ]( stat_e s, buff_t* b ) {
    buffs[ s ] = b;
  } );

  auto damage = create_proc_action<devouring_bolt_t>( "devouring_bolt_missile", effect );
  damage->base_dd_min += proc_damage;
  damage->base_dd_max += proc_damage;

  // skip setup if callback has been created by already having another copy of the enchant
  if ( find_special_effect( effect.player, effect.trigger()->id() ) )
    return;

  auto buff_action = create_proc_action<seized_power_t>( "seized_power", effect, buffs );

  damage->base_multiplier *= role_mult( effect );

  effect.spell_id = effect.trigger()->id();

  // TODO: Can this proc off of self damage?
  effect.player->callbacks.register_callback_trigger_function(
    effect.spell_id, dbc_proc_callback_t::trigger_fn_type::CONDITION, []( auto, const auto&, player_t* t, auto, auto ) {
      return t->is_enemy();
    } );

  effect.player->callbacks.register_callback_execute_function(
    effect.spell_id, [ damage, buff_action ]( auto, auto, player_t* t, auto ) {
      assert( t->is_enemy() );
      damage->execute_on_target( t );
      buff_action->execute_on_target( t );
    } );

  new dbc_proc_callback_t( effect.player, effect );
}

// 1259060 equip driver
// 1244243 rppm driver
// 1259061 buff
void blessed_pango_charm( special_effect_t& effect )
{
  auto stat_amount = effect.driver()->effectN( 1 ).average( effect );
  auto buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 1259061 ) )
    ->add_stat_from_effect_type( A_MOD_RATING, stat_amount );

  // skip setup if callback has been created by already having another copy of the enchant
  if ( find_special_effect( effect.player, effect.trigger()->id() ) )
    return;

  effect.spell_id    = effect.trigger()->id();
  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.player, effect );
}

// 1244276 driver
// 1259124 rppm driver
// 1259127 heal coeff?
// 1259128 damage
// 1259130 heal
void primal_spore_binding( special_effect_t& effect )
{
  effect.player->sim->error( UNVERIFIED_IMPLEMENTATION,
    "Primal Spore Binding: What determines if you get a damage proc vs healing proc is unknown." );

  auto damage_amount = effect.driver()->effectN( 1 ).average( effect );
  auto heal_amount   = effect.driver()->effectN( 2 ).average( effect );

  auto damage = create_proc_action<generic_aoe_proc_t>( "primal_spore_explosion", effect, 1259128 );
  damage->base_dd_min += damage_amount;
  damage->base_dd_max += damage_amount;

  auto heal =
    create_proc_action<base_generic_aoe_proc_t<proc_heal_t>>( "primal_spore_explosion_heal", effect, 1259130 );
  heal->base_dd_min += heal_amount;
  heal->base_dd_max += heal_amount;
  heal->name_str_reporting = "Heal";

  // skip setup if callback has been created by already having another copy of the enchant
  if ( find_special_effect( effect.player, effect.trigger()->id() ) )
    return;

  damage->base_multiplier *= role_mult( effect );
  heal->base_multiplier *= role_mult( effect );

  effect.spell_id = effect.trigger()->id();

  effect.player->callbacks.register_callback_execute_function(
    effect.spell_id, [ damage, heal ]( auto, auto, player_t* t, auto ) {
      if ( t->is_enemy() )
        damage->execute_on_target( t );
      else
        heal->execute_on_target( t );
    } );

  new dbc_proc_callback_t( effect.player, effect );
}

// 1251906 driver
// 1252383 rppm driver
// 1252389 dot
void prismatic_focusing_iris( special_effect_t& effect )
{
  auto dot_damage = effect.driver()->effectN( 1 ).average( effect );

  auto dot =
    create_proc_action<generic_proc_t>( "prismatic_focusing_iris", effect, effect.trigger()->effectN( 1 ).trigger() );
  dot->base_td += dot_damage;

  // skip setup if callback has been created by already having another copy of the enchant
  if ( find_special_effect( effect.player, effect.trigger()->id() ) )
    return;

  auto pct_per_gem  = effect.driver()->effectN( 3 ).percent();
  dot->base_td_multiplier *= 1.0 + ( pct_per_gem * unique_gem_list( effect.player, gem_colors ).size() );

  // Feb 23 2026 - tank mod is not being applied in game
  // dot->base_td_multiplier *= role_mult( effect );
  dot->base_td_multiplier *= bandolier_mul( effect.player );

  effect.spell_id       = effect.trigger()->id();
  effect.execute_action = dot;

  new dbc_proc_callback_t( effect.player, effect );
}

// Thalassian Phoenix Torque
// 1251815 Driver
// 1251907 Damage
// 1251908 Heal
void thalassian_phoenix_torque( special_effect_t& effect )
{
  auto pct_per_gem = effect.driver()->effectN( 2 ).percent();

  auto damage         = create_proc_action<generic_proc_t>( "phoenix_flames", effect, 1251907 );
  damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 1 ).average( effect );
  damage->base_multiplier *= 1.0 + ( pct_per_gem * unique_gem_list( effect.player, gem_colors ).size() );
  damage->base_multiplier *= role_mult( effect );
  damage->base_multiplier *= bandolier_mul( effect.player );

  auto heal = create_proc_action<generic_heal_t>( "phoenix_flames_heal", effect, 1251908 );
  heal->name_str_reporting = "Heal";
  heal->base_dd_min = heal->base_dd_max = effect.driver()->effectN( 2 ).average( effect );
  heal->base_multiplier *= 1.0 + ( pct_per_gem * unique_gem_list( effect.player, gem_colors ).size() );
  heal->base_multiplier *= role_mult( effect );
  heal->base_multiplier *= bandolier_mul( effect.player );

  effect.player->callbacks.register_callback_execute_function(
      effect.spell_id, [ damage, heal ]( auto, auto, player_t* t, auto ) {
        if ( t->is_enemy() )
          damage->execute_on_target( t );
        else
          heal->execute_on_target( t );
      } );

  new dbc_proc_callback_t( effect.player, effect );
}

// Signet of Azerothian Blessings
// 1251902 Driver
// 1252202 Buff
void signet_of_azerothian_blessings( special_effect_t& effect )
{
  auto base_value = effect.driver()->effectN( 1 ).average( effect );
  std::unordered_map<stat_e, double> values    = { { STAT_HASTE_RATING, 1.0 },
                                                   { STAT_CRIT_RATING, 1.0 },
                                                   { STAT_MASTERY_RATING, 1.0 },
                                                   { STAT_VERSATILITY_RATING, 1.0 } };

  std::unordered_map<stat_e, gem_color_e> gems = { { STAT_HASTE_RATING, GEM_PERIDOT },
                                                   { STAT_CRIT_RATING, GEM_GARNET },
                                                   { STAT_MASTERY_RATING, GEM_AMETHYST },
                                                   { STAT_VERSATILITY_RATING, GEM_LAPIS } };

  auto buff = create_buff<stat_buff_t>( effect.player, effect.driver()->effectN( 1 ).trigger() );

  for ( auto stat : secondary_ratings )
  {
    int count = 0;
    for( auto gem : equipped_gem_list( effect.player, gem_colors ) )
      if ( gem == gems.at( stat ) )
        count++;

    values.at( stat ) =
        base_value * ( 1.0 + count * effect.driver()->effectN( 2 ).percent() ) * bandolier_mul( effect.player );

    buff->add_stat( stat, values.at( stat ) );
  }

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.player, effect );
}

// Loa Worshiper's Band
// 1251904 Driver
// 1252524 Capybara Buff - Always Available
// 1257183 Nalorakk Buff - Garnet
// 1252832 Nalorakk Periodic Trigger - Garnet
// 1252814 Halazzi Damage - Amethyst
// 1252817 Janalai Damage - Lapis
// 1252818 Akilzon Buff - Peridot
// TODO: Does bandolier do anything special for this?
void loa_worshipers_band( special_effect_t& effect )
{
  enum loa_e : unsigned
  {
    LOA_CAPYBARA,
    LOA_NALORAKK,
    LOA_HALAZZI,
    LOA_JANALAI,
    LOA_AKILZON,
    LOA_MAX
  };

  struct loa_worshipers_band_cb_t final : public dbc_proc_callback_t
  {
    buff_t* capybara;
    buff_t* nalorakk;
    action_t* halazzi;
    action_t* janalai;
    buff_t* akilzon;

    std::vector<loa_e> loas;

    loa_worshipers_band_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ),
        capybara( nullptr ),
        nalorakk( nullptr ),
        halazzi( nullptr ),
        janalai( nullptr ),
        akilzon( nullptr )
    {
      // Capybara is always available, even with no gems socketed.
      loas.push_back( LOA_CAPYBARA );
      double capy_value = effect.driver()->effectN( 2 ).average( effect );
      capy_value *= bandolier_mul( effect.player );

      capybara = create_buff<stat_buff_t>( e.player, e.player->find_spell( 1252524 ) )
        ->add_stat_from_effect_type( A_MOD_STAT, capy_value );

      if ( range::contains( unique_gem_list( e.player, gem_colors ), GEM_GARNET ) )
      {
        loas.push_back( LOA_NALORAKK );
        auto periodic_spell = e.player->find_spell( 1252832 );
        auto nalorakk_spell = periodic_spell->effectN( 1 ).trigger();
        double nalo_value = nalorakk_spell->effectN( 1 ).average( e );
        nalo_value *= bandolier_mul( effect.player );

        auto nalorakk_stat = create_buff<stat_buff_t>( e.player, "nalorakks_call_to_war_crit", nalorakk_spell )
          ->set_stat_from_effect_type( A_MOD_RATING, nalo_value )
          ->set_name_reporting( "Crit" );

        nalorakk = create_buff<buff_t>( e.player, periodic_spell )
          ->set_tick_callback( [ nalorakk_stat ]( buff_t*, int, timespan_t ) {
            nalorakk_stat->trigger();
          } );
      }

      if ( range::contains( unique_gem_list( e.player, gem_colors ), GEM_AMETHYST ) )
      {
        loas.push_back( LOA_HALAZZI );
        double halazzi_value = effect.driver()->effectN( 3 ).average( effect );
        halazzi_value *= bandolier_mul( effect.player );

        halazzi = create_proc_action<generic_proc_t>( "claws_of_halazzi", e, 1252814 );
        halazzi->base_dd_min = halazzi->base_dd_max = halazzi_value;
        // halazzi->base_multiplier *= role_mult( e ); - Role Mult currently not applied to Loa Worshiper's Band
      }

      if ( range::contains( unique_gem_list( e.player, gem_colors ), GEM_LAPIS ) )
      {
        loas.push_back( LOA_JANALAI );
        double janalai_value = effect.driver()->effectN( 4 ).average( effect );
        janalai_value *= bandolier_mul( effect.player );

        janalai = create_proc_action<generic_proc_t>( "janalais_flames", e, 1252817 );
        janalai->base_dd_min = janalai->base_dd_max = janalai_value;
        janalai->aoe = -1;
        // janalai->base_multiplier *= role_mult( e ); - Role Mult currently not applied to Loa Worshiper's Band
      }

      if ( range::contains( unique_gem_list( e.player, gem_colors ), GEM_PERIDOT ) )
      {
        loas.push_back( LOA_AKILZON );
        const spell_data_t* akilzon_spell = e.player->find_spell( 1252818 );
        double akilzon_value = akilzon_spell->effectN( 1 ).average( e );
        akilzon_value *= bandolier_mul( effect.player );

        // Akilzon buff has the values in the buff itself.
        akilzon = create_buff<stat_buff_t>( e.player, akilzon_spell )
          ->add_stat_from_effect( 1, akilzon_value );
      }
    }

    void execute( const spell_data_t*, player_t* t, action_state_t* ) override
    {
      auto loa = rng().range( loas );
      switch (loa)
      {
        case LOA_CAPYBARA:
          capybara->trigger();
          break;
        case LOA_NALORAKK:
          nalorakk->trigger();
          break;
        case LOA_HALAZZI:
          halazzi->execute_on_target( t );
          break;
        case LOA_JANALAI:
          janalai->execute_on_target( t );
          break;
        case LOA_AKILZON:
          akilzon->trigger();
          break;
        default:
          break;
      }
    }
  };

  new loa_worshipers_band_cb_t( effect );
}

// 1261968 driver
// 1262512 rppm
// 1262515 damage
// 1262513 heal
void b0p_curator_of_booms( special_effect_t& effect )
{
  effect.player->sim->error( UNVERIFIED_IMPLEMENTATION,
    "B0P Curator of Booms: Implemented assuming procs on enemies will only fire damaging bombs, "
    "and procs on allies will only fire healing bombs. Each proc will randomly do 20%-100% of maximum amount. "
    "This has not be verified in-game." );

  auto damage_eff = effect.driver()->effectN( 2 );
  auto heal_eff = effect.driver()->effectN( 3 );

  auto max_bombs = as<int>( effect.driver()->effectN( 1 ).base_value() );

  auto damage = create_proc_action<generic_proc_t>( "big_boom", effect, damage_eff.trigger() );
  damage->base_dd_min += damage_eff.average( effect );
  damage->base_dd_max += damage_eff.average( effect );

  auto heal = create_proc_action<generic_heal_t>( "big_boom_heal", effect, heal_eff.trigger() );
  heal->name_str_reporting = "Heal";
  heal->base_dd_min += heal_eff.average( effect );
  heal->base_dd_max += heal_eff.average( effect );

  // skip setup if callback has been created by already having another copy of the embellishment
  if ( find_special_effect( effect.player, effect.trigger()->id() ) )
    return;

  effect.spell_id = effect.trigger()->id();

  effect.player->callbacks.register_callback_execute_function(
    effect.spell_id, [ damage, heal, max_bombs ]( const dbc_proc_callback_t* cb, auto, player_t* t, auto ) {
      auto _bombs = cb->rng().range( max_bombs ) + 1;
      auto _mul = as<double>( _bombs ) / max_bombs;

      cb->listener->sim->print_debug( "{} launching {} BIG BOMBS", *cb->listener, _bombs );

      if ( t->is_enemy() )
      {
        auto orig_mul = damage->base_multiplier;
        damage->base_multiplier *= _mul;
        damage->execute_on_target( t );
        damage->base_multiplier = orig_mul;
      }
      else
      {
        auto orig_mul = heal->base_multiplier;
        heal->base_multiplier *= _mul;
        heal->execute_on_target( t );
        heal->base_multiplier = orig_mul;
      }
    } );

  new dbc_proc_callback_t( effect.player, effect );
}

// 1246309 on-use
// 1279407 damage
void b1p_scorcher_of_souls( special_effect_t& effect )
{
  struct b1p_scorcher_of_souls_t : public generic_proc_t
  {
    b1p_scorcher_of_souls_t( const special_effect_t& e ) : generic_proc_t( e, "b1p_scorcher_of_souls", e.driver() )
    {
      // no split or increase # target so use generic_proc_t with aoe = -1
      tick_action = create_proc_action<generic_proc_t>( "b1p_scorcher_of_souls_tick", e, e.trigger() );
      tick_action->aoe = -1;
    }

    void execute() override
    {
      // this might blow up, in which case we'll need to figure out another workaround
      start_gcd();

      generic_proc_t::execute();
    }
  };

  effect.execute_action = create_proc_action<b1p_scorcher_of_souls_t>( "b1p_scorcher_of_souls", effect );
}
}  // namespace embellishments

namespace darkmoon
{
// Blood
// 1245053 Embellishment Driver
// 1245001 Trinket Driver
// 1245012 RPPM
// 1245025 Stat Buff
// TODO: What happens with both the trinket, and embellishment active?
void blood( special_effect_t& effect )
{
  // skip setup if callback has been created by already having trinket or embellishment
  bool do_setup = find_special_effect( effect.player, effect.trigger()->id() ) == nullptr;

  std::unordered_map<stat_e, buff_t*> buffs;

  create_all_stat_buffs( effect, effect.player->find_spell( 1245025 ), effect.driver()->effectN( 1 ).average( effect ),
    [ &buffs ]( stat_e s, buff_t* b ) { buffs[ s ] = b; }, do_setup );

  struct blood_cb_t : public dbc_proc_callback_t
  {
    std::unordered_map<stat_e, buff_t*> buffs;

    blood_cb_t( const special_effect_t& e, std::unordered_map<stat_e, buff_t*> map )
      : dbc_proc_callback_t( e.player, e ), buffs( std::move( map ) )
    {}

    void execute( const spell_data_t*, player_t*, action_state_t* ) override
    {
      auto stat = util::lowest_stat( listener, secondary_ratings );
      for ( auto [ s, b ] : buffs )
      {
        if ( s == stat )
          b->trigger();
        else
          b->expire();
      }
    }
  };

  if ( do_setup )
  {
    effect.spell_id = effect.trigger()->id();

    new blood_cb_t( effect, buffs );
  }
}

// Rot
// 1245055 Embellishment Driver
// 1245051 Trinket Driver
// 1244332 RPPM
// 1247411 Asyncronous DoT
// TODO: What happens with both the trinket, and embellishment active?
void rot( special_effect_t& effect )
{
  // skip setup if callback has been created by already having trinket or embellishment
  bool do_setup = find_special_effect( effect.player, effect.trigger()->id() ) == nullptr;

  struct root_rot_t : public generic_proc_t
  {
    root_rot_t( const special_effect_t& e ) : generic_proc_t( e, "root_rot", e.trigger()->effectN( 1 ).trigger() )
    {
      dot_max_stack = 1;  // Override Max Stacks to 1, this behavior is handled by the asyncronous debuff
    }

    double composite_ta_multiplier( const action_state_t* s ) const override
    {
      double m = generic_proc_t::composite_ta_multiplier( s );

      if ( auto debuff = find_debuff( s->target ) )
        m *= debuff->check();
      else
        m = 0.0;

      return m;
    }

    buff_t* create_debuff( player_t* t ) override
    {
      return make_buff<buff_t>( actor_pair_t( t, player ), "root_rot_debuff", &data() )
        ->set_activated( true )
        ->set_duration( data().duration() + 1_ms );  // Extra 1ms to avoid expiration before next tick
    }

    void execute() override
    {
      generic_proc_t::execute();
      get_debuff( execute_state->target )->trigger();
    }
  };

  auto dot = create_proc_action<root_rot_t>( "root_rot", effect );
  dot->base_td += effect.driver()->effectN( 1 ).average( effect );

  if ( do_setup )
  {
    effect.spell_id = effect.trigger()->id();
    effect.execute_action = dot;

    new dbc_proc_callback_t( effect.player, effect );
  }
}

// Void
// 1245052 Embellishment Driver
// 1244254 Trinket Driver
// 1244253 RPPM
// 1244617 Asyncronous Buff
void void_( special_effect_t& effect )
{
  // skip setup if callback has been created by already having trinket or embellishment
  bool do_setup = find_special_effect( effect.player, effect.trigger()->id() ) == nullptr;

  auto buff = create_buff<stat_buff_t>( effect.player, effect.trigger()->effectN( 1 ).trigger() )
    ->add_stat_from_effect_type( A_MOD_RATING, effect.driver()->effectN( 1 ).average( effect ) );

  if ( do_setup )
  {
    effect.custom_buff = buff;
    effect.spell_id = effect.trigger()->id();

    new dbc_proc_callback_t( effect.player, effect );
  }
}

// Hunt
// 1245054 Embellishment Driver
// 1245050 Trinket Driver
// 1252457 RPPM
// 1252486 Haste Buff - Elemental, Aberration, Demon
// 1252487 Crit Buff - Beast, Mechanical
// 1252488 Mastery Buff - Humanoid, Dragonkin
// 1252489 Versatility Buff - Undead, Giant, Not Specified
// TODO: What happens with both the trinket, and embellishment active?
void hunt( special_effect_t& effect )
{
  // skip setup if callback has been created by already having trinket or embellishment
  bool do_setup = find_special_effect( effect.player, effect.trigger()->id() ) == nullptr;

  std::unordered_map<stat_e, buff_t*> buffs;

  for ( auto id : { 1252486, 1252487, 1252488, 1252489 } )
  {
    auto spell = effect.player->find_spell( id );
    auto buff = create_buff<stat_buff_t>( effect.player, spell )
      ->add_stat_from_effect_type( A_MOD_RATING, effect.driver()->effectN( 1 ).average( effect ) );

    buffs[ buff->stats.front().stat ] = buff;
  }

  // L'ura emulated as Undead, as we dont classify using CreatureType.db2 data. Not Specified triggers the vers buff
  // like Undead and Giant.
  static constexpr std::array<race_e, 9> raid_races = {
    RACE_ABERRATION, RACE_ABERRATION, RACE_HUMANOID,  RACE_DRAGONKIN, RACE_HUMANOID,
    RACE_HUMANOID,   RACE_ABERRATION, RACE_ELEMENTAL, RACE_NOT_SPECIFIED
  };

  static constexpr std::array<race_e, 10> valid_races = {
    RACE_ABERRATION, RACE_BEAST,    RACE_DEMON,      RACE_DRAGONKIN, RACE_ELEMENTAL,
    RACE_GIANT,      RACE_HUMANOID, RACE_MECHANICAL, RACE_UNDEAD,    RACE_NOT_SPECIFIED
  };

  struct hunt_cb_t : public dbc_proc_callback_t
  {
    enum mode_e
    {
      MODE_SPECIFIED,
      MODE_ACTUAL,
      MODE_RANDOM,
      MODE_RAID_RANDOM
    };

    std::unordered_map<stat_e, buff_t*> buffs;
    race_e race;
    mode_e mode;

    hunt_cb_t( const special_effect_t& e, std::unordered_map<stat_e, buff_t*> map )
      : dbc_proc_callback_t( e.player, e ), buffs( map ), race( RACE_NONE ), mode( MODE_RAID_RANDOM )
    {
      if ( util::str_compare_ci( e.player->midnight_opts.darkmoon_hunt_race, "none" ) ||
           ( listener->sim->fight_style == fight_style_e::FIGHT_STYLE_DUNGEON_ROUTE &&
             e.player->midnight_opts.darkmoon_hunt_race.is_default() ) )
      {
        mode = MODE_ACTUAL;
      }
      else if ( util::str_compare_ci( e.player->midnight_opts.darkmoon_hunt_race, "random" ) )
      {
        mode = MODE_RANDOM;
      }
      else if ( util::str_compare_ci( e.player->midnight_opts.darkmoon_hunt_race, "raid_random" ) )
      {
        mode = MODE_RAID_RANDOM;
      }
      else
      {
        mode = MODE_SPECIFIED;
        race = util::parse_race_type( e.player->midnight_opts.darkmoon_hunt_race );

        if ( !range::contains( valid_races, race ) )
        {
          std::vector<std::string> valid_strings;
          for ( auto r : valid_races )
          {
            std::string val = util::race_type_string( r );
            valid_strings.emplace_back( val );
          }

          e.player->sim->error( error_level_e::SEVERE,
                                "midnight.darkmoon_hunt_race has invalid race type '{}'. Valid race types are {}. "
                                "Defaulting to targets actual race.",
                                listener->midnight_opts.darkmoon_hunt_race, fmt::join( valid_strings, ", " ) );

          mode = MODE_ACTUAL;
        }
      }
    }

    void pick_random_race()
    {
      race = rng().range( valid_races );
    }

    void pick_random_raid_race()
    {
      race = rng().range( raid_races );
    }

    void reset() override
    {
      dbc_proc_callback_t::reset();
      // Pick a new random race each iteration
      if ( mode == MODE_RANDOM )
        pick_random_race();
      else if ( mode == MODE_RAID_RANDOM )
        pick_random_raid_race();
    }

    void trigger_race_buff( race_e r )
    {
      switch ( r )
      {
        case RACE_HUMANOID:
        case RACE_DRAGONKIN:
          buffs[ STAT_MASTERY_RATING ]->trigger();
          break;
        case RACE_ABERRATION:
        case RACE_ELEMENTAL:
        case RACE_DEMON:
          buffs[ STAT_HASTE_RATING ]->trigger();
          break;
        case RACE_BEAST:
        case RACE_MECHANICAL:
          buffs[ STAT_CRIT_RATING ]->trigger();
          break;
        case RACE_GIANT:
        case RACE_UNDEAD:
        case RACE_NOT_SPECIFIED:
          buffs[ STAT_VERSATILITY_RATING ]->trigger();
          break;
        default:
          break;
      }
    }

    void execute( const spell_data_t*, player_t* t, action_state_t* ) override
    {
      if ( mode == MODE_ACTUAL )
        trigger_race_buff( t->race );
      else
        trigger_race_buff( race );
    }
  };

  if ( do_setup )
  {
    effect.spell_id = effect.trigger()->id();

    new hunt_cb_t( effect, std::move( buffs ) );
  }
}
}  // namespace darkmoon

namespace trinkets
{
// Heart of the Wind
// 1250599 Driver
// 1263318 Buff
void heart_of_the_wind( special_effect_t& effect )
{
  auto buff = create_buff<stat_buff_t>( effect.player, effect.trigger() )
                  ->set_stat_from_effect_type( A_MOD_RATING, effect.driver()->effectN( 1 ).average( effect ) );

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.player, effect );
}

// Kroluk's Warbanner
// 1250563 driver
// 1255816 damage
void kroluks_warbanner( special_effect_t& effect )
{
  auto damage = create_proc_action<generic_proc_t>( "kroluks_warbanner", effect, 1255816 );
  damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 1 ).average( effect );
  damage->base_multiplier *= role_mult( effect );

  effect.execute_action = damage;

  new dbc_proc_callback_t( effect.player, effect );
}

// Vessel of Souls
// 1250602 Driver
// 1265513 Area Trigger
// 1265566 Buff
void vessel_of_souls( special_effect_t& effect )
{
  auto buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 1265566 ) )
                ->set_stat_from_effect_type( A_MOD_STAT, effect.driver()->effectN( 1 ).average( effect ) );

  // create the fake tracker buff to track orb spawns
  auto orb = create_buff<buff_t>( effect.player, "a_restless_soul_orb", effect.trigger() )
    ->set_quiet( true )
    ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
    ->set_max_stack( 15 )  // sufficiently high??
    ->set_chance( 1.0 - effect.player->midnight_opts.vessel_of_tortured_souls_miss_chance );

  effect.custom_buff = orb;

  new dbc_proc_callback_t( effect.player, effect );

  // create a fake callback to proc on next ability that can be cast while moving
  auto pickup = new special_effect_t( effect.player );
  pickup->name_str = "a_restless_soul_pickup";
  pickup->spell_id = orb->data().id();
  pickup->proc_flags_ = PF_CAST_SUCCESSFUL;
  pickup->proc_chance_ = 1.0;
  pickup->set_can_only_proc_from_class_abilities( true );
  pickup->set_can_proc_from_procs( false );
  effect.player->special_effects.push_back( pickup );

  struct a_restless_soul_pickup_cb_t : public dbc_proc_callback_t
  {
    buff_t* buff;
    buff_t* orb;

    a_restless_soul_pickup_cb_t( const special_effect_t& e, buff_t* buff, buff_t* orb )
      : dbc_proc_callback_t( e.player, e ), buff( buff ), orb( orb )
    {}

    void trigger( const proc_data_t& data, player_t* t, action_state_t* s, proc_trigger_type_e type ) override
    {
      // trigger only if the action is usable while moving
      // we can't just use usable_moving() since it returns false for melee abilities
      if ( ( s->action->trigger_gcd > 0_ms && s->action->execute_time() == 0_ms ) ||
           ( s->action->channeled && s->action->usable_moving() ) )
      {
        dbc_proc_callback_t::trigger( data, t, s, type );
      }
    }

    void execute( const spell_data_t*, player_t*, action_state_t* s ) override
    {
      if ( auto move_delay = s->action->gcd() - 10_ms; orb->remains_gt( move_delay ) )
      {
        make_event( *s->action->sim, move_delay, [ this ] {
          buff->trigger();
          orb->decrement();
        } );
      }
    }
  };

  auto cb = new a_restless_soul_pickup_cb_t( *pickup, buff, orb );
  cb->activate_with_buff( orb );
}

// Mark of Light
// 1250582 Driver
// 1258226 Damage
void mark_of_light( special_effect_t& effect )
{
  auto damage = create_proc_action<generic_aoe_proc_t>( "mark_of_light", effect, 1258226 );
  damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 1 ).average( effect );
  damage->base_multiplier *= role_mult( effect );

  effect.execute_action = damage;

  new dbc_proc_callback_t( effect.player, effect );
}

// Solarflare Prism
// 1254640 Driver
// 1255504 Buff
void solarflare_prism( special_effect_t& effect )
{
  struct solarflare_prism_buff_t : public stat_buff_t
  {
    double hp_inc;
    double max_val;

    solarflare_prism_buff_t( std::string_view n, const special_effect_t& e )
      : stat_buff_t( e.player, n, e.player->find_spell( 1255504 ) ),
        hp_inc( e.driver()->effectN( 2 ).average( e ) ),
        max_val( e.driver()->effectN( 4 ).average( e ) )
    {
      add_stat_from_effect_type( A_MOD_RATING, e.driver()->effectN( 1 ).average( e ) );
    }

    double buff_stat_stack_amount( const buff_stat_t& stat, int ) const override
    {
      return std::min( stat.amount + hp_inc * check_value(), max_val );
    }
  };

  struct solarflare_prism_cb_t final : public dbc_proc_callback_t
  {
    buff_t* buff;

    solarflare_prism_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e )
    {
      buff = make_buff<solarflare_prism_buff_t>( "solarflare_prism", e );
    }

    void execute( const spell_data_t*, player_t* t, action_state_t* ) override
    {
      buff->trigger( -1, 100.0 - t->health_percentage() );
    }
  };

  new solarflare_prism_cb_t( effect );
}

// Sapling of the Dawnroot
// 1250604 Driver
// 1263077 Pet Summon
// 1263087 Pet Aura (Overrides Auto Attack)
// 1263101 Auto Attack
// 1263121 Demise Explosion
void sapling_of_the_dawnroot( special_effect_t& effect )
{
  struct lightbloom_lashing_t final : public attack_t
  {
    lightbloom_lashing_t( pet_t* p, const special_effect_t& e, std::string_view name, action_t* a = nullptr )
      : attack_t( name, p, e.player->find_spell( 1263101 ) )
    {
      auto proxy = a;
      auto it    = range::find( proxy->child_action, data().id(), &action_t::id );
      if ( it != proxy->child_action.end() )
        stats = ( *it )->stats;
      else
        proxy->add_child( this );

      background = repeating = not_a_proc = may_crit = true;
      special                                        = false;
      trigger_gcd                                    = 0_ms;
      base_dd_min = base_dd_max = e.driver()->effectN( 1 ).average( e );
      base_multiplier *= role_mult( e );
    }
  };

  struct sappy_demise_t final : public spell_t
  {
    sappy_demise_t( const special_effect_t& e, std::string_view n, pet_t* p, const spell_data_t* s, action_t* a )
      : spell_t( n, p, s )
    {
      auto proxy = a;
      auto it    = range::find( proxy->child_action, data().id(), &action_t::id );
      if ( it != proxy->child_action.end() )
        stats = ( *it )->stats;
      else
        proxy->add_child( this );

      background = split_aoe_damage = true;
      aoe                           = -1;
      base_dd_min = base_dd_max = e.driver()->effectN( 2 ).average( e );
      base_multiplier *= role_mult( e );
    }
  };

  struct uprooted_lasher_pet_t final : public unique_gear_pet_t
  {
    const special_effect_t& effect;

    uprooted_lasher_pet_t( const special_effect_t& e, action_t* parent = nullptr )
      : unique_gear_pet_t( "uprooted_lasher", e, &parent->data() ), effect( e )
    {
      parent_action = parent;
      main_hand_weapon.type = WEAPON_BEAST;
      main_hand_weapon.swing_time = 2_s;
      use_auto_attack = true;
    }

    void demise() override
    {
      // Dont explode if the sim has ended.
      if( !sim->event_mgr.canceled )
        sappy_demise->execute();
      unique_gear_pet_t::demise();
    }

    attack_t* create_auto_attack() override
    {
      return new lightbloom_lashing_t( this, effect, "lightbloom_lashing", parent_action );
    }

    void create_actions() override
    {
      unique_gear_pet_t::create_actions();
      sappy_demise = new sappy_demise_t( effect, "sappy_demise", this, find_spell( 1263121 ), parent_action );
    }

  private:
    action_t* sappy_demise;
  };

  struct sapling_of_the_dawnroot_t final : public generic_proc_t
  {
    spawner::pet_spawner_t<uprooted_lasher_pet_t> lasher;

    sapling_of_the_dawnroot_t( const special_effect_t& e )
      : generic_proc_t( e, "sapling_of_the_dawnroot", e.driver() ), lasher( "uprooted_lasher", e.player )
    {
      auto lasher_summon_spell = e.player->find_spell( 1263077 );
      lasher.set_creation_callback( [ &e, this ]( player_t* ) { return new uprooted_lasher_pet_t( e, this ); } );
      lasher.set_default_duration( lasher_summon_spell->duration() );
    }

    void execute() override
    {
      generic_proc_t::execute();
      lasher.spawn();
    }
  };

  effect.execute_action = create_proc_action<sapling_of_the_dawnroot_t>( "sapling_of_the_dawnroot", effect );

  new dbc_proc_callback_t( effect.player, effect );
}

// Seed of the Devouring Wild
// 1250580 Driver
// 1259351 Values
// 1259352 Buff
void seed_of_the_devouring_wild( special_effect_t& effect )
{
  auto equip = find_special_effect( effect.player, 1259351 );
  assert( equip && "Seed of the Devouring Wild missing equip effect" );

  auto buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 1259352 ) )
                ->set_stat_from_effect_type( A_MOD_RATING, equip->driver()->effectN( 2 ).average( effect ) );

  auto damage         = create_proc_action<generic_aoe_proc_t>( "seed_of_the_devouring_wild", effect, effect.driver() );
  damage->base_dd_min = damage->base_dd_max = equip->driver()->effectN( 1 ).average( effect );
  damage->base_multiplier *= role_mult( effect );

  effect.execute_action = damage;
  effect.custom_buff    = buff;
}

// Idol of the War Loa
// 1250567 Driver
// 1258222 Ally Speed Buff
// 1258223 Buff
// Speed Buff NYI, TODO: implement if it matters for sims.
void idol_of_the_war_loa( special_effect_t& effect )
{
  auto buff = create_buff<stat_buff_t>( effect.player, effect.trigger() )
                ->set_stat_from_effect_type( A_MOD_STAT, effect.driver()->effectN( 1 ).average( effect ) );

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.player, effect );
}

// Gaze of the Alnseer
// 1256896 Driver
// 1266686 Alnsight Buff
// 1266687 Primary Buff
// 1266701 vfx
void gaze_of_the_alnseer( special_effect_t& effect )
{
  auto alnsight_spell = effect.trigger();
  auto buff = create_buff<buff_t>( effect.player, alnsight_spell );

  auto stat = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 1266687 ) )
                  ->set_stat_from_effect_type( A_MOD_STAT, effect.driver()->effectN( 1 ).average( effect ) );

  auto alnsight          = new special_effect_t( effect.player );
  alnsight->item         = effect.item;
  alnsight->spell_id     = alnsight_spell->id();
  alnsight->custom_buff  = stat;
  alnsight->proc_flags2_ = PF2_ALL_HIT;
  effect.player->special_effects.push_back( alnsight );

  struct alnsight_cb_t : public dbc_proc_callback_t
  {
    cooldown_t* orig_cd = nullptr;
    bool refreshed = false;

    alnsight_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e )
    {}

    void initialize() override
    {
      dbc_proc_callback_t::initialize();

      orig_cd = cooldown;
    }

    void execute( const spell_data_t*, player_t*, action_state_t* ) override
    {
      // when bugged, the next trigger does not obey the icd
      if ( refreshed )
      {
        refreshed = false;
        cooldown = nullptr;
      }
      else if ( !cooldown )
      {
        cooldown = orig_cd;
      }

      proc_buff->trigger();
    }

    void reset() override
    {
      cooldown = orig_cd;
      refreshed = false;
    }
  };

  auto alnsight_cb = new alnsight_cb_t( *alnsight );
  alnsight_cb->activate_with_buff( buff, true );

  effect.name_str = "gaze_of_the_alnseer";  // trigger has it's own cb so explicitly name the driver
  effect.custom_buff = buff;

  struct gaze_of_the_alnseer_cb_t : public dbc_proc_callback_t
  {
    alnsight_cb_t* proc_cb;

    gaze_of_the_alnseer_cb_t( const special_effect_t& e, alnsight_cb_t* cb )
      : dbc_proc_callback_t( e.player, e ), proc_cb( cb )
    {}

    void execute( const spell_data_t*, player_t*, action_state_t* ) override
    {
      // if alnsight is refreshed while it's icd is down, bug it out
      // TODO: what happens if it's refreshed again while it's bugged?
      if ( proc_buff->check() && proc_cb->cooldown && proc_cb->cooldown->down() )
      {
        assert( proc_cb->active );
        proc_cb->refreshed = true;
      }

      proc_buff->trigger();
    }
  };

  new gaze_of_the_alnseer_cb_t( effect, alnsight_cb );
}


// Resonant Bellowstone
// 1250564 Driver
// 1254180 Buff
// 1254331 Stacking Buff
void resonant_bellowstone( special_effect_t& effect )
{
  auto last_roar_spell = effect.trigger();
  auto echoing_roar = last_roar_spell->effectN( 2 ).trigger();

  auto buff = create_buff<stat_buff_t>( effect.player, last_roar_spell )
                ->set_stat_from_effect_type( A_MOD_RATING, effect.driver()->effectN( 1 ).average( effect ) );
  auto stacking_buff = create_buff<stat_buff_t>( effect.player, echoing_roar )
                         ->set_stat_from_effect_type( A_MOD_RATING, effect.driver()->effectN( 2 ).average( effect ) );

  auto last_roar          = new special_effect_t( effect.player );
  last_roar->name_str     = "last_roar_proc";
  last_roar->item         = effect.item;
  last_roar->spell_id     = last_roar_spell->id();
  last_roar->custom_buff  = stacking_buff;
  last_roar->proc_flags2_ = PF2_CRIT;
  effect.player->special_effects.push_back( last_roar );

  auto last_roar_cb = new dbc_proc_callback_t( effect.player, *last_roar );
  last_roar_cb->activate_with_buff( buff, true );

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.player, effect );
}

// Undreamt God's Oozing Vestige
// 1256790 Driver
// 1269597 AoE damage
// 1269591 DoT
void undreamt_gods_oozing_vestige( special_effect_t& effect )
{
  struct undreamt_gods_oozing_vestige_cb_t final : dbc_proc_callback_t
  {
    action_t* dot;
    action_t* aoe;
    undreamt_gods_oozing_vestige_cb_t( special_effect_t& e, action_t* a )
      : dbc_proc_callback_t( e.player, e ), dot( nullptr ), aoe( a )
    {
      dot = create_proc_action<generic_proc_t>( "volatile_phlegm", e, e.trigger() );
      // Data has dot duration as infinite. Setting it to an extremely high value to get periodic behavior
      dot->dot_duration = 900_s;
      dot->base_td = e.driver()->effectN( 1 ).average( e );
      dot->base_td_multiplier *= role_mult( e );
      dot->add_child( aoe );
    }

    void execute( const spell_data_t*, player_t* t, action_state_t* ) override
    {
      dot->execute_on_target( t );
      dot_t* d = dot->get_dot( t );
      if ( d && d->is_ticking() && d->at_max_stacks() )
      {
        d->cancel();
        aoe->execute();
      }
    }
  };

  auto aoe = create_proc_action<generic_aoe_proc_t>( "phlegmpocalypse", effect, 1269597 );
  aoe->base_dd_min = aoe->base_dd_max = effect.driver()->effectN( 2 ).average( effect );
  aoe->base_multiplier *= role_mult( effect );

  effect.player->register_on_kill_callback( [ aoe, effect ]( player_t* t ) {
    dot_t* d = t->find_dot( "volatile_phlegm", effect.player );
    if ( d && d->is_ticking() && !effect.player->sim->event_mgr.canceled )
      aoe->execute();
  } );

  new undreamt_gods_oozing_vestige_cb_t( effect, aoe );
}

// Light Company Guidon
// 1251817 equip
// 1259633 on use + Haste Buff
// 1262496 Speed Buff
// TODO: Speed buff if it matters for sims.
void light_company_guidon( special_effect_t& effect )
{
  auto equip = find_special_effect( effect.player, 1251817 );
  assert( equip && "Light Company Guidon missing equip effect" );

  effect.custom_buff = create_buff<stat_buff_t>( effect.player, effect.driver() )
    ->set_stat_from_effect_type( A_MOD_RATING, equip->driver()->effectN( 1 ).average( effect ) )
    ->set_cooldown( 0_ms );
}

// Heart of Ancient Hunger
// 1251822 Driver
// 1262753 Buff
void heart_of_ancient_hunger( special_effect_t& effect )
{
  effect.custom_buff =
      create_buff<stat_buff_t>( effect.player, effect.driver()->effectN( 1 ).trigger() )
          ->set_stat_from_effect_type( A_MOD_RATING,
                                       effect.driver()->effectN( 1 ).average( effect ) /
                                           effect.driver()->effectN( 1 ).trigger()->duration().total_seconds() )
          ->set_reverse( true )
          ->set_max_stack( as<int>( effect.driver()->effectN( 1 ).trigger()->duration().total_seconds() ) );

  new dbc_proc_callback_t( effect.player, effect );
}

// Umbral & Radiant Plume
// 1265809 Umbral Driver
// 1260592 Radiant Driver
// 1265808 Umbral Buff
// 1260615 Radiant Buff
void plume_of_beloren( special_effect_t& effect )
{
  auto buff_spell = effect.driver()->effectN( 1 ).trigger();
  auto buff       = create_buff<stat_buff_t>( effect.player, buff_spell )
                  ->set_stat_from_effect_type(
                      A_MOD_RATING, effect.driver()->effectN( 1 ).average( effect ) / buff_spell->max_stacks() )
                  ->set_reverse( true )
                  ->set_expire_callback( [ effect ]( buff_t* b, int, timespan_t ) {
                    make_event( *effect.player->sim, 0_ms, [ b ] { b->trigger(); } );
                  } );

  effect.player->register_on_arise_callback( effect.player, [ buff ] { buff->trigger(); } );

  effect.player->register_on_combat_state_callback( [ buff ]( player_t*, bool c ) {
    if ( c )
      buff->set_reverse( true );
    else
      buff->set_reverse( false );
  } );
}

// Sealed Chaos Urn
// 1253115 Buff & Driver
// 1259442 Debuff
// 1259443 Fear
void sealed_chaos_urn( special_effect_t& effect )
{
  auto fear = create_buff<buff_t>( effect.player, "sealed_chaos_urn_fear", effect.player->find_spell( 1259443 ) )
                  ->set_name_reporting( "Fear" )
                  ->set_stack_change_callback( [ effect ]( buff_t*, int, int new_ ) {
                    // Emulate the fear with a stun on the player for the duration of the fear.
                    if ( new_ )
                    {
                      effect.player->buffs.stunned->trigger();
                      effect.player->stun();
                    }
                    else
                    {
                      effect.player->buffs.stunned->expire();
                      if( !effect.player->readying )
                        effect.player->schedule_ready();
                    }
                  } );

  auto debuff = create_buff<buff_t>( effect.player, "sealed_chaos_urn_debuff", effect.driver()->effectN( 2 ).trigger() )
                    ->set_name_reporting( "Debuff" )
                    ->set_expire_callback( [ &effect, fear ]( buff_t*, int, timespan_t d ) {
                      if ( d == 0_ms )
                      {
                        if ( effect.player->midnight_opts.sealed_chaos_urn_dispell )
                        {
                          timespan_t dur = effect.player->rng().gauss_ab(
                              effect.player->midnight_opts.sealed_chaos_urn_dispell_time, 1_s, 500_ms, 4.5_s );
                          fear->trigger( dur );
                        }
                        else
                          fear->trigger();
                      }
                    } );

  effect.player->register_on_kill_callback( [ debuff ]( player_t* ) {
    if ( debuff->check() )
      debuff->expire();
  } );

  effect.custom_buff = create_buff<stat_buff_t>( effect.player, "sealed_chaos_urn_stats", effect.driver(), effect.item )
                         ->set_name_reporting( "Stats" )
                         ->set_stack_change_callback( [ debuff ]( buff_t*, int, int new_ ) {
                           if ( new_ )
                             debuff->trigger();
                         } );
}

// Lost Idol of the Hash'ey
// 1253111 Driver
// 1266182 Primary
// 1266184 Secondary (Highest)
// 1266197 Secondary (Lowest)
void lost_idol_of_the_hashey( special_effect_t& effect )
{
  struct lost_idol_of_the_hashey_cb_t final : public dbc_proc_callback_t
  {
    enum idol_type_e
    {
      NONE,
      PRIMARY,
      HIGHEST,
      LOWEST
    };

    buff_t* primary;
    std::unordered_map<stat_e, buff_t*> highest;
    std::unordered_map<stat_e, buff_t*> lowest;
    std::vector<idol_type_e> idol_types;
    idol_type_e last_type = NONE;

    lost_idol_of_the_hashey_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e ), highest(), lowest()
    {
      primary = create_buff<stat_buff_t>( e.player, e.player->find_spell( 1266182 ) )
        ->set_stat_from_effect_type( A_MOD_STAT, e.driver()->effectN( 2 ).average( e ) );
      deactivate_with_buff( primary );

      create_all_stat_buffs( e, e.player->find_spell( 1266184 ), e.driver()->effectN( 1 ).average( e ),
        [ this ]( stat_e s, buff_t* b ) {
          highest[ s ] = b;
          deactivate_with_buff( b );
        } );

      create_all_stat_buffs( e, e.player->find_spell( 1266197 ), e.driver()->effectN( 1 ).average( e ),
        [ this ]( stat_e s, buff_t* b ) {
          lowest[ s ] = b;
          deactivate_with_buff( b );
        } );
    }

    void reset() override
    {
      dbc_proc_callback_t::reset();

      idol_types = { PRIMARY, HIGHEST, LOWEST };
    }

    void execute( const spell_data_t*, player_t*, action_state_t* ) override
    {
      rng().shuffle( idol_types.begin(), idol_types.end() );

      auto type = idol_types.back();

      idol_types.pop_back();  // remove the current type from pool

      if ( last_type != NONE )
        idol_types.push_back( last_type );  // add the previous type back into the pool

      last_type = type;

      switch ( type )
      {
        case PRIMARY: primary->trigger(); break;
        case HIGHEST: highest[ util::highest_stat( listener, secondary_ratings ) ]->trigger(); break;
        case LOWEST:  lowest[ util::lowest_stat( listener, secondary_ratings ) ]->trigger(); break;
        default:                   break;
      }
    }
  };

  new lost_idol_of_the_hashey_cb_t( effect );
}

// Withered Saptor's Paw
// 1253110 Driver
// 1255226 Buff
void withered_saptors_paw( special_effect_t& effect )
{
  size_t stat_idx;

  switch ( effect.player->type )
  {
    case DEATH_KNIGHT:
    case PALADIN:
    case WARRIOR:      stat_idx = 3; break;  // strength
    default:           stat_idx = 1; break;  // agility
  }

  effect.custom_buff = create_buff<stat_buff_t>( effect.player, effect.trigger() )
    ->set_stat_from_effect( stat_idx, effect.driver()->effectN( 1 ).average( effect ) );

  new dbc_proc_callback_t( effect.player, effect );
}

// Shadow of the Empyrean Requiem
// 1259518 Driver
// 1264325 Main target damage
// 1268775 Second target damage
// 1264337 Haste Buff
void shadow_of_the_empyrean_requiem( special_effect_t& effect )
{
  struct shadow_of_the_empyrean_requiem_damage_t : public generic_proc_t
  {
    buff_t* haste_buff;
    double hp_threshold;
    action_t* cleave;

    shadow_of_the_empyrean_requiem_damage_t( const special_effect_t& e, std::string_view n, unsigned id, buff_t* buff,
                                             action_t* cleave = nullptr )
      : generic_proc_t( e, n, id ),
        haste_buff( buff ),
        hp_threshold( e.driver()->effectN( 3 ).base_value() ),
        cleave( cleave )
    {
      base_dd_min = base_dd_max = e.driver()->effectN( 1 ).average( e );
      base_multiplier *= role_mult( e );
      if ( cleave )
        add_child( cleave );
    }

    void execute() override
    {
      generic_proc_t::execute();
      if ( cleave && sim->target_non_sleeping_list.size() > 1 )
      {
        for ( auto& target : sim->target_non_sleeping_list )
        {
          if ( target != execute_state->target )
          {
            cleave->execute_on_target( target );
            return;
          }
        }
      }
    }

    void impact( action_state_t* s ) override
    {
      generic_proc_t::impact( s );

      if ( s->target->health_percentage() < hp_threshold )
        haste_buff->trigger();
    }
  };

  auto buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 1264337 ) )
                  ->set_stat_from_effect_type( A_MOD_RATING, effect.driver()->effectN( 2 ).average( effect ) );

  auto second_target_damage = create_proc_action<shadow_of_the_empyrean_requiem_damage_t>(
      "shadow_of_the_empyrean_requiem_secondary", effect, 1268775, buff );
  second_target_damage->name_str_reporting = "Cleave";

  auto main_target_damage = create_proc_action<shadow_of_the_empyrean_requiem_damage_t>(
      "shadow_of_the_empyrean_requiem", effect, 1264325, buff, second_target_damage );

  effect.execute_action = main_target_damage;

  new dbc_proc_callback_t( effect.player, effect );
}

// Void Execution Mandate
// 1250557 Driver & Haste Buff
// 1263355 Debuff
// 1263357 Stacking Crit Buff
void void_execution_mandate( special_effect_t& effect )
{
  struct marked_for_execution_t : public generic_proc_t
  {
    buff_t* buff;
    marked_for_execution_t( const special_effect_t& e, std::string_view n, buff_t* b )
      : generic_proc_t( e, n, e.driver() ), buff( b )
    {
      target_debuff = e.trigger();
    }

    void impact( action_state_t* s ) override
    {
      generic_proc_t::impact( s );
      get_debuff( s->target )->trigger();
      buff->trigger();
    }
  };

  auto crit_buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 1263357 ) )
                     ->set_stat_from_effect_type( A_MOD_RATING, effect.driver()->effectN( 2 ).average( effect ) )
                     ->set_refresh_behavior( buff_refresh_behavior::DISABLED );

  auto haste_buff = create_buff<stat_buff_t>( effect.player, effect.driver() )
                      ->set_stat_from_effect_type( A_MOD_RATING, effect.driver()->effectN( 3 ).average( effect ) )
                      ->set_expire_callback( [ crit_buff ]( buff_t*, int, timespan_t ) {
                        crit_buff->expire();
                      } );

  auto debuff = create_proc_action<marked_for_execution_t>( "marked_for_execution", effect, haste_buff );

  effect.execute_action = debuff;

  auto impending          = new special_effect_t( effect.player );
  impending->name_str     = "impending_execution_proc";
  impending->item         = effect.item;
  impending->spell_id     = effect.driver()->id();
  impending->cooldown_    = 0_ms; // Cooldown is for on use effect, not the equip effect.
  impending->proc_flags2_ = PF2_ALL_HIT;
  effect.player->special_effects.push_back( impending );

  new dbc_proc_callback_t( effect.player, *impending );

  effect.player->callbacks.register_callback_execute_function(
    effect.driver()->id(), [ debuff, crit_buff ]( auto, auto, player_t* t, auto ) {
      if ( debuff->get_debuff( t )->check() )
        crit_buff->trigger();
    } );
}

// Emberwing Feather
// 1250508 Driver & Haste Buff
// 1255853 Crit Debuff
// 1255856 Mastery Debuff
// 1255857 Vers Debuff
// TODO: What is the low chance? Not in data, needs testing.
void emberwing_feather( special_effect_t& effect )
{
  effect.player->sim->error( UNVERIFIED_IMPLEMENTATION,
    "Emberwing Feather: 'Low chance' for stat penalty is unknown. 10% is currently implemented as a placeholder value." );

  // Assume "low" chance means flat 10% chance for now until we have more testing data
  static constexpr double EMBERWING_BURN_CHANCE = 0.1;

  struct emberwing_heatwave_t : public generic_proc_t
  {
    buff_t* buff;
    std::array<buff_t*, 3> debuffs;

    emberwing_heatwave_t( const special_effect_t& e, std::string_view n )
      : generic_proc_t( e, n, e.driver() ), buff( nullptr )
    {
      cooldown->duration = 0_ms;  // Handled by the special effect

      buff = create_buff<stat_buff_t>( e.player, "emberwing_heatwave", e.driver() )
                 ->set_stat_from_effect_type( A_MOD_RATING, e.driver()->effectN( 1 ).average( e ) )
                 ->set_cooldown( 0_ms );

      debuffs[ 0 ] = create_buff<stat_buff_t>( e.player, "emberwing_burn_crit", e.player->find_spell( 1255853 ) )
                       ->set_stat_from_effect_type( A_MOD_RATING, -e.driver()->effectN( 2 ).average( e ) )
                       ->set_name_reporting( "Crit" );

      debuffs[ 1 ] = create_buff<stat_buff_t>( e.player, "emberwing_burn_mast", e.player->find_spell( 1255856 ) )
                       ->set_stat_from_effect_type( A_MOD_RATING, -e.driver()->effectN( 2 ).average( e ) )
                       ->set_name_reporting( "Mastery" );

      debuffs[ 2 ] = create_buff<stat_buff_t>( e.player, "emberwing_burn_vers", e.player->find_spell( 1255857 ) )
                       ->set_stat_from_effect_type( A_MOD_RATING, -e.driver()->effectN( 2 ).average( e ) )
                       ->set_name_reporting( "Vers" );
    }

    void execute() override
    {
      generic_proc_t::execute();
      buff->trigger();

      if ( rng().roll( EMBERWING_BURN_CHANCE ) )
        rng().range( debuffs )->trigger();
    }
  };

  effect.execute_action = create_proc_action<emberwing_heatwave_t>( "emberwing_heatwave", effect );
}

// Ranger-Captain's Iridescent Insignia
// 1260265 Equip/Value Driver
// 1260266 Damage / On use Driver
void ranger_captains_iridescent_insignia( special_effect_t& effect )
{
  auto equip = find_special_effect( effect.player, 1260265 );
  assert( equip && "Ranger-Captain's Iridescent Insignia missing equip driver" );

  struct silverstrike_trick_shot_t : public generic_proc_t
  {
    silverstrike_trick_shot_t( const special_effect_t& e, std::string_view n ) : generic_proc_t( e, n, e.driver() )
    {}

    result_e calculate_result( action_state_t* ) const override
    {
      return RESULT_CRIT;
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = generic_proc_t::composite_da_multiplier( s );

      // Assume this uses the highest crit chance
      if ( player->cache.spell_crit_chance() > player->cache.attack_crit_chance() )
        m *= 1.0 + player->cache.spell_crit_chance();
      else
        m *= 1.0 + player->cache.attack_crit_chance();

      return m;
    }
  };

  // set up the on-use
  auto damage = create_proc_action<silverstrike_trick_shot_t>( "silverstrike_trick_shot", effect );
  damage->base_dd_min = damage->base_dd_max = equip->driver()->effectN( 1 ).average( effect );
  damage->base_multiplier *= role_mult( effect );

  effect.execute_action = damage;

  // set up the equip
  equip->proc_flags2_ = PF2_CRIT;

  // set up the cooldown reduction, note both action and item cooldowns must be adjusted
  auto cdr = -timespan_t::from_seconds( equip->driver()->effectN( 2 ).base_value() );
  auto action_cd = damage->cooldown;
  auto item_cd = effect.player->get_cooldown( effect.cooldown_name() );

  effect.player->callbacks.register_callback_execute_function(
    equip->spell_id, [ cdr, item_cd, action_cd ]( auto, auto, auto, auto ) {
        action_cd->adjust( cdr );
        item_cd->adjust( cdr );
    } );

  new dbc_proc_callback_t( effect.player, *equip );
}

// Eye of the Drowning Void
// 1250601 Driver
// 1255476 Damage
// TODO: Does this have the increased damage per target hit?
void eye_of_the_drowning_void( special_effect_t& effect )
{
  auto damage = create_proc_action<generic_aoe_proc_t>( "eye_of_the_drowning_void", effect, effect.trigger() );
  damage->base_multiplier *= role_mult( effect );
  damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 1 ).average( effect );

  effect.execute_action = damage;
  new dbc_proc_callback_t( effect.player, effect );
}

// Latch's Crooked Hook
// 1254193 Use Driver
// 1255379 Equip
// 1255298 Ground Impact Damage
// 1254328 Main damage
// TODO: Does the aoe effect increase per target hit?
void latchs_crooked_hook( special_effect_t& effect )
{
  struct latchs_crooked_hook_t : public generic_proc_t
  {
    action_t* main_damage;
    action_t* impact_damage;

    latchs_crooked_hook_t( const special_effect_t& e, std::string_view n ) : generic_proc_t( e, n, e.driver() )
    {
      auto equip = find_special_effect( e.player, 1255379 );
      assert( equip && "Latch's Cooked Hook missing equip effect" );

      target_debuff      = e.driver();
      cooldown->duration = 0_ms;  // Handled by the special effect
      aoe                = -1;

      main_damage = create_proc_action<generic_proc_t>( "latchs_crooked_hook", e, 1254328 );
      main_damage->base_dd_min = main_damage->base_dd_max = equip->driver()->effectN( 2 ).average( e );
      main_damage->base_multiplier *= role_mult( e );

      impact_damage =
        create_proc_action<generic_aoe_proc_t>( "latchs_crooked_hook_impact", e, 1255298 );
      impact_damage->base_dd_min = impact_damage->base_dd_max = equip->driver()->effectN( 1 ).average( e );
      impact_damage->base_multiplier *= role_mult( e );

      main_damage->add_child( impact_damage );
    }

    buff_t* create_debuff( player_t* t ) override
    {
      auto debuff = generic_proc_t::create_debuff( t );
      debuff->set_expire_callback( [ &, t ]( buff_t*, int, timespan_t d ) {
        if ( d == 0_ms )
          impact_damage->execute_on_target( t );
      } );

      return debuff;
    }

    void execute() override
    {
      generic_proc_t::execute();
      assert( execute_state && "Latch's Crooked Hook unable to get execute state" );
      get_debuff( execute_state->target )->trigger();
    }

    void impact( action_state_t* s ) override
    {
      generic_proc_t::impact( s );
      main_damage->execute_on_target( s->target );
    }
  };

  auto missile = create_proc_action<latchs_crooked_hook_t>( "latchs_crooked_hook_missile", effect );

  effect.execute_action = missile;
}

// Lightspire Core
// 1250527 Driver & Passive Mastery Buff
// 1263768 Light mastery buff
// 1263762 Area Trigger
// TODO: Emulate not standing in the light
void lightspire_core( special_effect_t& effect )
{
  auto buff = create_buff<stat_buff_t>( effect.player, effect.driver(), effect.item )
                  ->set_rppm( RPPM_DISABLE )
                  ->set_chance( 1.01 );

  effect.player->register_on_arise_callback( effect.player, [ buff ] { buff->trigger(); } );

  auto light_buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 1263768 ) )
                        ->set_stat_from_effect_type( A_MOD_RATING, effect.driver()->effectN( 3 ).average( effect ) )
                        ->set_duration( effect.player->find_spell( 1263762 )->duration() );

  effect.custom_buff = light_buff;

  new dbc_proc_callback_t( effect.player, effect );
}

// Magister's Alchemist Stone
// 1280591 Driver
// 299788 Strength Buff
// 299789 Agility Buff
// 299790 Int Buff
void magisters_alchemist_stone( special_effect_t& effect )
{
  auto stat = effect.player->convert_hybrid_stat( STAT_STR_AGI_INT );
  const spell_data_t* buff_spell;
  switch ( stat )
  {
    case STAT_STRENGTH:
      buff_spell = effect.player->find_spell( 299788 );
      break;
    case STAT_AGILITY:
      buff_spell = effect.player->find_spell( 299789 );
      break;
    default:
      buff_spell = effect.player->find_spell( 299790 );
      break;
  }

  effect.custom_buff = create_buff<stat_buff_t>( effect.player, buff_spell )
    ->add_stat_from_effect_type( A_MOD_STAT, effect.driver()->effectN( 1 ).average( effect ) );

  new dbc_proc_callback_t( effect.player, effect );
}

// Vaelgor's Final Stare
// 1259293 Driver
// 1260459 Nullsight
void vaelgors_final_stare( special_effect_t& e )
{
  unsigned equip_id = 1259293;
  auto equip        = find_special_effect( e.player, equip_id );
  assert( equip && "Vaelgor's final stare missing equip effect" );

  auto buff_spell    = e.driver();
  int stacks         = static_cast<int>( buff_spell->duration() / buff_spell->effectN( 3 ).period() );
  double buff_val    = equip->driver()->effectN( 1 ).average( e );
  double buff_stacks = buff_val / stacks;

  e.custom_buff = create_buff<stat_buff_t>( e.player, buff_spell )
                      ->set_stat_from_effect_type( A_MOD_RATING, buff_stacks )
                      ->set_max_stack( stacks )
                      ->set_reverse( true );
}

// Ever-collapsing Void Fissure
// 1253114 Driver and Buff
void evercollapsing_void_fissure( special_effect_t& e )
{
  struct evercollapsing_void_fissure_buff_t : public stat_buff_t
  {
    evercollapsing_void_fissure_buff_t( player_t* p, std::string_view name, const spell_data_t* s, special_effect_t& e )
      : stat_buff_t( p, name, s )
    {
      set_max_stack( static_cast<int>( s->duration() / s->effectN( 4 ).period() ) );
      set_stat_from_effect_type( A_MOD_RATING, s->effectN( 5 ).average( e ) );
    }

    double buff_stat_stack_amount( const buff_stat_t& stat, int s ) const override
    {
      return stat_buff_t::buff_stat_stack_amount( stat, std::max( 0, s - 1 ) );
    }
  };

  e.custom_buff = create_buff<evercollapsing_void_fissure_buff_t>( e.player, e.driver(), e );
}


// Locus-Walker's Ribbon
// 1259314 Driver
// 1259317 stat buff
// 1268058 stack buff
void locuswalkers_ribbon( special_effect_t& effect )
{
  struct riftwalkers_temptation_t : public stat_buff_t
  {
    buff_t* stack_buff;

    riftwalkers_temptation_t( player_t* p, std::string_view n, const spell_data_t* s, buff_t* stack_buff )
      : stat_buff_t( p, n, s ), stack_buff( stack_buff )
    {}

    double buff_stat_stack_amount( const buff_stat_t& stat, int s ) const override
    {
      return stat_buff_t::buff_stat_stack_amount( stat, s ) * ( 1.0 + stack_buff->check_stack_value() );
    }
  };

  struct locuswalkers_ribbon_t final : public dbc_proc_callback_t
  {
    stat_buff_t* stat_buff;
    buff_t* stack_buff;

    locuswalkers_ribbon_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e )
    {
      stack_buff = create_buff<buff_t>( e.player, e.trigger()->effectN( 2 ).trigger() )
                       ->set_freeze_stacks( true )
                       ->set_default_value( e.driver()->effectN( 2 ).percent() )
                       ->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
                         if ( !stack_buff->player->in_combat && stack_buff->check() )
                           stack_buff->decrement();
                       } );

      stat_buff = create_buff<riftwalkers_temptation_t>( e.player, e.trigger(), stack_buff )
                      ->set_stat_from_effect_type( A_MOD_STAT, e.driver()->effectN( 1 ).average( e ) );
      stat_buff->set_refresh_behavior( buff_refresh_behavior::PANDEMIC );
    }

    void execute( const spell_data_t*, player_t*, action_state_t* ) override
    {
      stat_buff->trigger();
      stack_buff->trigger();
    }
  };

  new locuswalkers_ribbon_t( effect );
}

// Wraps of Cosmic Madness
// 1259153 Driver
// 1263614 Missile
// 1263393 Damage Spell (Cosmic Barrage)
// 1259103 Equip Driver
// 1263475 vfx?
void wraps_of_cosmic_madness( special_effect_t& effect )
{
  struct cosmic_madness_channel_t : public proc_spell_t
  {
    cosmic_madness_channel_t( const special_effect_t& e ) :
      proc_spell_t( "wraps_of_cosmic_madness", e.player, e.driver() )
    {
      unsigned equip_id = 1259103;
      auto equip        = find_special_effect( e.player, equip_id );
      assert( equip && "Wraps of Cosmic Madness missing equip effect" );

      channeled = true;

      auto missile_spell = e.player->find_spell( 1263614 );
      auto damage_spell = missile_spell->effectN( 1 ).trigger();
      auto missile_damage = equip->driver()->effectN( 1 ).average( e );

      auto cosmic_barrage = create_proc_action<generic_proc_t>( "cosmic_barrage_missile", e, missile_spell );
      auto damage = create_proc_action<generic_aoe_proc_t>( "cosmic_barrage_damage", e, damage_spell );
      damage->base_dd_min = damage->base_dd_max = missile_damage;
      damage->dual = true;

      tick_action = cosmic_barrage;
      cosmic_barrage->impact_action = damage;
      damage->stats = stats;
    }

    void execute() override
    {
      proc_spell_t::execute();

      // cancel the player-ready event triggered by use_item_t
      event_t::cancel( player->readying );

      // prevent auto attacks while channeling
      player->reset_auto_attacks( composite_dot_duration( execute_state ) );
    }

    void last_tick( dot_t* d ) override 
    {
      // cache first since last_tick() will null out player->channeling
      bool was_channeling = player->channeling == this;

      proc_spell_t::last_tick( d );

      // restart the player since the player-ready from use_item_t was canceled
      if ( was_channeling && !player->readying )
        player->schedule_ready( rng().gauss( sim->channel_lag ) );
    }
  };

  effect.execute_action = create_proc_action<cosmic_madness_channel_t>( "wraps_of_cosmic_madness", effect );
}

// 1253113 driver
// 1266394 dot
// 1266403 crit buff
// 1266407 remaining dot pop
void voidreapers_libram( special_effect_t& effect )
{
  effect.player->sim->error( UNVERIFIED_IMPLEMENTATION,
    "Void-Reaper's Libram: Sacred Text dot is assumed to be able to proc while Sacred Duty buff is up." );

  struct voidreapers_libram_cb_t : public dbc_proc_callback_t
  {
    action_t* dot_action;
    action_t* pop_action;
    buff_t* crit_buff;

    voidreapers_libram_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e )
    {
      dot_action = create_proc_action<generic_proc_t>( "sacred_text", e, 1266394 );
      dot_action->base_td = e.driver()->effectN( 1 ).average( e );

      pop_action = create_proc_action<generic_proc_t>( "text_ignite", e, 1266407 );
      dot_action->add_child( pop_action );

      crit_buff = create_buff<stat_buff_t>( e.player, e.player->find_spell( 1266403 ) )
        ->set_stat_from_effect_type( A_MOD_RATING, e.driver()->effectN( 2 ).average( e ) );
    }

    void execute( const spell_data_t*, player_t* t, action_state_t* ) override
    {
      if ( auto dot = dot_action->find_dot( t ); dot && dot->is_ticking() )
      {
        assert( dot->current_action == dot_action );

        pop_action->execute_on_target( t, dot->tick_damage_over_remaining_time() );
        crit_buff->trigger();
      }
      else
      {
        dot_action->execute_on_target( t );
      }
    }
  };

  new voidreapers_libram_cb_t( effect );
}

// 1250589 driver
// 1264146 aoe
// 1264156 heal
void crawling_plague( special_effect_t& effect )
{
  auto aoe = create_proc_action<generic_aoe_proc_t>( "crawling_plague", effect, 1264146 );
  aoe->base_dd_min = aoe->base_dd_max = effect.driver()->effectN( 1 ).average( effect );

  auto heal = create_proc_action<generic_heal_t>( "crawling_plague_heal", effect, 1264156 );
  heal->base_dd_min = heal->base_dd_max = effect.driver()->effectN( 2 ).average( effect );
  heal->name_str_reporting = "Heal";

  effect.player->callbacks.register_callback_execute_function(
    effect.spell_id, [ aoe, heal ]( auto, auto, auto, auto ) {
      aoe->execute();
      heal->execute();
    } );

  new dbc_proc_callback_t( effect.player, effect );
}

// 1250541 driver
// 1264613 dot
void echo_of_the_evercurse( special_effect_t& effect )
{
  auto dot = create_proc_action<generic_proc_t>( "echo_of_the_evercurse", effect, 1264613 );
  auto orig_td = effect.driver()->effectN( 1 ).average( effect );
  dot->base_td = orig_td;

  effect.execute_action = dot;

  new dbc_proc_callback_t( effect.player, effect );

  auto _mul = effect.driver()->effectN( 2 ).percent();

  effect.player->register_on_kill_callback( [ dot, orig_td, _mul ]( player_t* t ) {
    if ( auto d = dot->find_dot( t ); d && d->is_ticking() )
    {
      auto dam = d->tick_damage_over_remaining_time() * _mul;
      auto ticks = d->ticks_left_fractional();
      auto new_td = dam / ticks;

      if ( const auto& tl = dot->target_list(); !tl.empty() )
      {
        auto new_target = dot->rng().range( tl );
        dot->base_td = new_td;
        dot->execute_on_target( new_target );
        dot->base_td = orig_td;
      }
    }
  } );
}

// 1250546 driver
// 1264374 aoe
// 1264404 buff
void mindpiercers_sigil( special_effect_t& effect )
{
  auto aoe = create_proc_action<generic_aoe_proc_t>( "voidtorn_eruption", effect, 1264374 );
  aoe->base_dd_min = aoe->base_dd_max = effect.driver()->effectN( 1 ).average( effect );

  auto buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 1264404 ) )
    ->set_stat_from_effect_type( A_MOD_STAT, effect.driver()->effectN( 2 ).average( effect ) );

  effect.player->callbacks.register_callback_execute_function(
    effect.spell_id, [ aoe, buff ]( auto, auto, player_t* t, auto ) {
      aoe->execute_on_target( t );
      buff->trigger();
    } );

  new dbc_proc_callback_t( effect.player, effect );
}

// 1258283 on-use
// 1258275 equip
// 1263721 damage driver
// 1263725 damage
// 1263727 shield NYI
void litany_of_lightblind_wrath( special_effect_t& effect )
{
  auto equip = find_special_effect( effect.player, 1258275 );
  assert( equip && "Litany of Lightblind Wrath missing equip effect" );

  struct beacon_of_lightblind_wrath_buff_t : public buff_t
  {
    player_t* target = nullptr;

    beacon_of_lightblind_wrath_buff_t( player_t* p, std::string_view n, const spell_data_t* s ) : buff_t( p, n, s )
    {
      set_initial_stack_to_max_stack();
    }

    bool trigger( int s, double v, double c, timespan_t d ) override
    {
      auto ret = buff_t::trigger( s, v, c, d );
      if ( ret )
        target = player->target;

      return ret;
    }

    void reset() override
    {
      buff_t::reset();
      target = nullptr;
    }
  };

  // create on-use stacks
  auto beacon = create_buff<beacon_of_lightblind_wrath_buff_t>( effect.player, effect.driver() );
  effect.custom_buff = beacon;

  // create damage
  auto damage = create_proc_action<generic_proc_t>( "beacon_of_lightblind_wrath", effect, 1263725 );
  damage->base_dd_min = damage->base_dd_max = equip->driver()->effectN( 1 ).average( effect );

  // set up driver to proc damage & consume stacks
  auto driver = new special_effect_t( effect.player );
  driver->name_str = "litany_of_lightblind_wrath_driver";
  driver->spell_id = effect.trigger()->id();
  effect.player->special_effects.push_back( driver );

  effect.player->callbacks.register_callback_execute_function(
    driver->spell_id, [ beacon, damage ]( auto, auto, auto, auto ) {
      assert( beacon->target && "Beacon of Lightblind Wrath has no target." );

      damage->execute_on_target( beacon->target );
      beacon->decrement();
    } );

  auto cb = new dbc_proc_callback_t( effect.player, *driver );
  cb->activate_with_buff( beacon );
}

// 71563 on-use
// 71564 buff + driver
void deadly_precision( special_effect_t& effect )
{
  // create buff
  auto buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 71564 ), effect.item )
    ->set_initial_stack_to_max_stack();

  effect.custom_buff = buff;

  // setup the driver to consume the buff
  auto driver = new special_effect_t( effect.player );
  driver->name_str = "deadly_precision_driver";
  driver->spell_id = buff->data().id();
  driver->proc_flags2_ = PF2_CRIT;
  effect.player->special_effects.push_back( driver );

  effect.player->callbacks.register_callback_execute_function( driver->spell_id, [ buff ]( auto, auto, auto, auto ) {
    buff->decrement();
  } );

  auto cb = new dbc_proc_callback_t( effect.player, *driver );
  cb->activate_with_buff( buff );
}

// 1258535 driver
// 1258534 buff
void volatile_void_suffuser( special_effect_t& effect )
{
  // create buff
  auto buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 1258534 ) );
  buff->set_stat_from_effect_type( A_MOD_STAT, effect.driver()->effectN( 1 ).average( effect ) );
    buff->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );
    
  effect.player->sim->error( UNVERIFIED_IMPLEMENTATION,
    "Volatile Void Suffuser: Implementation may incorrectly over trigger on some DPS specs. Flags are Yellow Melee, Heal, Helpful Periodic." );

  effect.custom_buff = buff;
}

// On use Driver 1272693
// Other Driver 1272690
// Damage Spell 1272720
// Leech Buff 1272698
void astalors_anguish_agitator( special_effect_t& e )
{
  auto other_driver = e.player->find_spell( 1272690 );

  auto proj_spell = e.player->find_spell( 1272720 );
  auto buff_spell = e.player->find_spell( 1272698 );

  auto projectile = create_proc_action<generic_proc_t>( util::tokenize_fn( proj_spell->name_cstr() ), e,
                                                                proj_spell->name_cstr(), proj_spell );
  projectile->base_dd_min = projectile->base_dd_max = other_driver->effectN( 1 ).average( e );

  auto leech_buff = create_buff<stat_buff_t>( e.player, util::tokenize_fn( buff_spell->name_cstr() ), buff_spell );
  leech_buff->set_stat_from_effect_type( A_MOD_RATING, other_driver->effectN( 2 ).average( e ) );

  e.execute_action = projectile;
  e.custom_buff    = leech_buff;
}

// 1253120 Driver
// 1266300 Ally Buff
// 1266299 Player Buff
void glorious_crusaders_keepsake( special_effect_t& e )
{
  struct glorious_crusaders_keepsake_cb_t : public dbc_proc_callback_t
  {
    target_specific_t<std::unordered_map<stat_e, buff_t*>> buffs;
    std::unordered_map<size_t, bool> valid_party_targets;


    glorious_crusaders_keepsake_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ), buffs{ true }, valid_party_targets()
    {
      create_buffs( effect.player );
    }

    // Adapted create all buffs.
    void create_all_buffs( player_t* target, const special_effect_t& effect_, const spell_data_t* buff_data,
                           double amount, std::function<void( stat_e, buff_t* )> add_fn )
    {
      auto buff_name = util::tokenize_fn( buff_data->name_cstr() );

      for ( const auto& eff : buff_data->effects() )
      {
        if ( eff.type() != E_APPLY_AURA || eff.subtype() != A_MOD_RATING )
          continue;

        auto stats = util::translate_all_rating_mod( eff.misc_value1() );

        std::vector<std::string_view> stat_strs;
        range::transform( stats, std::back_inserter( stat_strs ), &util::stat_type_abbrev );

        auto name = fmt::format( "{}_{}", buff_name, util::string_join( stat_strs, "_" ) );

        if ( auto buff = buff_t::find( target, name ) )
        {
          add_fn( stats.front(), buff );
          continue;
        }

        auto buff = make_buff<stat_buff_t>( actor_pair_t{ target, target }, name, buff_data )
                        ->add_stat( stats.front(), amount ? amount : eff.average( effect_ ) )
                        ->set_name_reporting( util::string_join( stat_strs ) );

        if ( target != effect_.player )
          buff->set_refresh_behavior( buff_refresh_behavior::DISABLED );

        if ( add_fn )
          add_fn( stats.front(), buff );
      }
    }

    void create_buffs( player_t* target_player )
    {
      if ( buffs[ target_player ] )
        return;

      auto buff_spell =
          effect.player == target_player ? effect.player->find_spell( 1266299 ) : effect.player->find_spell( 1266300 );

      auto buff_size = effect.driver()->effectN( effect.player == target_player ? 1 : 2 ).average( effect );

      buffs[ target_player ] = new std::unordered_map<stat_e, buff_t*>();

      create_all_buffs( target_player, effect, buff_spell, buff_size,
                        [ & ]( stat_e s, buff_t* b ) { ( *buffs[ target_player ] )[ s ] = b; } );
    }

    bool valid_player( player_t* target_player )
    {
      if ( target_player == effect.player )
        return false;

      if ( valid_party_targets[ target_player->actor_index ] )
        return valid_party_targets[ target_player->actor_index ];

      valid_party_targets[ target_player->actor_index ] = find_special_effect( target_player, effect.trigger()->id() );
      return valid_party_targets[ target_player->actor_index ];
    }

    buff_t* highest_buff( player_t* target_player )
    {
      create_buffs( target_player );

      return buffs[ target_player ]->at( util::highest_stat( target_player, secondary_ratings ) );
    }

    buff_t* lowest_buff( player_t* target_player )
    {
      create_buffs( target_player );

      return buffs[ target_player ]->at( util::lowest_stat( target_player, secondary_ratings ) );
    }

    buff_t* get_buff( player_t* target_player )
    {
      return target_player == effect.player ? highest_buff( target_player ) : lowest_buff( target_player );
    }

    void execute( const spell_data_t*, player_t*, action_state_t* ) override
    {
      get_buff( effect.player )->trigger();

      if ( !effect.player->sim->single_actor_batch && effect.player->sim->player_non_sleeping_list.size() > 1 )
      {
        int buffs_applied = 0;
        for ( auto player : effect.player->sim->player_non_sleeping_list )
        {
          if ( player->is_sleeping() || player->is_pet() )
            continue;

          if ( valid_player( player ) )
          {
            get_buff( player )->trigger();
            if ( ++buffs_applied >= 4 )
              break;
          }
        }
      }
    }
  };

  new glorious_crusaders_keepsake_cb_t( e );
}

// 1254752 Driver
// 1254577 Buff - Create the buff {target, target} -> it's a shared buff.
// 1254534 Projectile
void refueling_orb( special_effect_t& e )
{
  struct refueling_orb_cb_t : public dbc_proc_callback_t
  {
    double refueling_orb_heal_chance;
    int chain_targets;
    double velocity;
    timespan_t min_travel;
    const spell_data_t* buff_data;
    double stat_amount;

    refueling_orb_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ),
        refueling_orb_heal_chance( effect.player->midnight_opts.refueling_orb_heal_chance ),
        chain_targets( effect.player->find_spell( 1254534 )->effectN( 1 ).chain_target() ),
        velocity( effect.player->find_spell( 1254534 )->missile_speed() ),
        min_travel( timespan_t::from_seconds( effect.player->find_spell( 1254534 )->missile_min_duration() ) ),
        buff_data( effect.player->find_spell( 1254577 ) ),
        stat_amount( effect.driver()->effectN( 2 ).average( effect ) )
    {}

    buff_t* create_debuff( player_t* t ) override
    {
      return make_buff<stat_buff_t>( actor_pair_t( t, listener ), "refueling_orb", buff_data )
        ->set_stat_from_effect_type( A_MOD_RATING, stat_amount );
    }

    void execute( const spell_data_t*, player_t*, action_state_t* ) override
    {
      if ( effect.player->sim->player_non_sleeping_list.size() == 1 )
      {
        get_debuff( effect.player )->trigger();
      }
      else
      {
        auto allies = effect.player->sim->player_non_sleeping_list.data(); // make a copy

        if ( auto it = std::find( allies.begin(), allies.end(), effect.player ); it != allies.end() )
          std::iter_swap( it, std::prev ( allies.end() ) );

        timespan_t total_travel_time = 0_s;
        player_t* previous_target    = effect.player;

        for ( int i = 0; i < chain_targets; i++ )
        {
          auto target_iterator = rng().range( allies.begin(), std::prev( allies.end() ) );
          player_t* target     = *target_iterator;
          std::iter_swap( target_iterator, std::prev( allies.end() ) );

          total_travel_time += std::max(
              min_travel, timespan_t::from_seconds( previous_target->get_player_distance( *target ) / velocity ) );

          if ( !rng().roll( refueling_orb_heal_chance ) )
          {
            make_event( effect.player->sim, total_travel_time, [ this, target ] { get_debuff( target )->trigger(); } );
          }

          previous_target = target;
        }
      }
    }
  };

  new refueling_orb_cb_t( e );
}

// Driver 1253112
// Damage 1266366
// Missile 1 1266370
// Missile 2 1266371
// Missile 3 1266372
void sylvan_wakrapuku( special_effect_t& effect )
{
  auto damage         = create_proc_action<generic_aoe_proc_t>( "DiveBomb", effect, 1266366 );
  damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 1 ).average( effect );
  damage->base_multiplier *= role_mult( effect );

  struct sylvan_wakrapuku_cb_t : public dbc_proc_callback_t
  {
    action_t* divebomb;
    sylvan_wakrapuku_cb_t( const special_effect_t& e, action_t* divebomb )
      : dbc_proc_callback_t( e.player, e ), divebomb( divebomb )
    {
    }

    void execute( const spell_data_t*, player_t* t, action_state_t* ) override
    {
      for ( auto travel_time : { 0.5, 1.0, 1.5 } )
      {
        divebomb->min_travel_time = travel_time;
        divebomb->execute_on_target( t );
      }
    }
  };

  new sylvan_wakrapuku_cb_t( effect, damage );
}

// 1272091 driver
// 1277482 buff
// 1255685 protocol of violence (higher rppm?)
// 1255687 protocol of sustenance (longer duration?)
// 1255688 protocol of predation (higher buff value?)
void crucible_of_erratic_energies( special_effect_t& effect )
{
  double stat_value = effect.driver()->effectN( 1 ).average( effect );
  // Predation increases the crit rating provided by 20%. 
  if ( effect.player->midnight_opts.crucible_of_erratic_energies_predation )
    stat_value *= 1.0 + effect.driver()->effectN( 4 ).percent();

  // Without Voilence selected, this behaves as if it were 2rppm, not 4rppm.
  if ( !effect.player->midnight_opts.crucible_of_erratic_energies_violence )
    effect.rppm_modifier_ = 1.0 / effect.driver()->effectN( 3 ).base_value();

  auto buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 1277482 ) )
    ->set_stat_from_effect_type( A_MOD_RATING, stat_value );

  // Protocol of Sustenance doubles the buff duration.
  if ( effect.player->midnight_opts.crucible_of_erratic_energies_sustenance )
    buff->set_duration( buff->data().duration() * effect.driver()->effectN( 5 ).base_value() );

  effect.custom_buff = buff;

  effect.proc_flags2_ = PF2_LANDED;

  new dbc_proc_callback_t( effect.player, effect );
}

// 1255278 driver
// 1255296 missile trigger
// 1255357 debuff
void tangle_of_vibrant_vines( special_effect_t& effect )
{
  auto missile = create_proc_action<generic_proc_t>( "tangle_of_vibrant_vines_missile", effect, effect.trigger() );
  auto debuff = create_proc_action<generic_proc_t>( "tangle_of_vibrant_vines", effect, 1255357 );

  debuff->base_td = effect.driver()->effectN( 2 ).average( effect );
  missile->impact_action = debuff;
  effect.execute_action = missile;

  new dbc_proc_callback_t( effect.player, effect );
}

// 1260633 driver
// 1260627 damage
// 1263141 absorb
void gloomspattered_dreadscale( special_effect_t& effect )
{
  struct fractional_absorb_t : public absorb_buff_t
  {
    double absorb_fraction;

    fractional_absorb_t( player_t* player, std::string_view name, const spell_data_t* spell,
                         const item_t* item = nullptr )
      : absorb_buff_t( player, name, spell, item ), absorb_fraction( 1.0 )
    {
    }

    double consume( double amount, action_state_t* state = nullptr ) override
    {
      return absorb_buff_t::consume( amount * absorb_fraction, state );
    }

    absorb_buff_t* set_absorb_fraction( double fraction )
    {
      absorb_fraction = fraction;
      return this;
    }
  };

  struct gloomspattered_dreadscale_t : public generic_aoe_proc_t
  {
    buff_t* absorb;
    double shield_amount;

    gloomspattered_dreadscale_t( const special_effect_t& effect )
      : generic_aoe_proc_t( effect, "gloomspattered_dreadscale", effect.driver(), true ), shield_amount( 0 )
    {
      auto equip = find_special_effect( effect.player, 1260627 );
      assert( equip && "Gloom-Spattered Dreadscale missing equip effect" );

      base_dd_min = base_dd_max = equip->driver()->effectN( 1 ).average( effect );

      auto absorb_spell = effect.player->find_spell( 1263141 );
      absorb = create_buff<fractional_absorb_t>( effect.player, name(), absorb_spell )
                   ->set_absorb_fraction( absorb_spell->effectN( 2 ).percent() )
                   ->set_absorb_source( effect.player->get_stats( "gloomspattered_dreadscale_absorb", this ) );
    }

    void execute() override
    {
      generic_aoe_proc_t::execute();

      absorb->trigger( -1, shield_amount );
      shield_amount = 0;
    }

    void impact( action_state_t* state ) override
    {
      generic_aoe_proc_t::impact( state );

      shield_amount += state->result_amount;
    }
  };

  effect.execute_action = create_proc_action<gloomspattered_dreadscale_t>( "gloomspattered_dreadscale", effect );
}

// 1284696 driver
// 1284698 buff
void sporelords_mycelium( special_effect_t& effect )
{
  std::unordered_map<stat_e, buff_t*> buffs;
  auto value = effect.driver()->effectN( 1 ).average( effect );

  create_all_stat_buffs( effect, effect.trigger(), value, [ &buffs, value ]( stat_e s, buff_t* b ) {
    // don't need a separate leech buff
    if ( s == STAT_LEECH_RATING )
      return;

    // add leech stat
    debug_cast<stat_buff_t*>( b )->add_stat_from_effect( 5, value );

    buffs[ s ] = b;
  } );

  new dbc_proc_callback_t( effect.player, effect );

  effect.player->callbacks.register_callback_execute_function(
    effect.spell_id, [ buffs, p = effect.player ]( auto, auto, auto, auto ) {
      auto stat = p->rng().range( secondary_ratings );
      for ( auto [ s, b ] : buffs )
      {
        if ( s == stat )
          b->trigger();
        else
          b->expire();
      }
    } );
}
}  // namespace trinkets

namespace weapons
{
// 1253357 abyss driver
// 1265822 abyss damage
// 1253359 radiant driver
// 1266012 radiant damage
// 1265823 void tear
// 1253358 set
void torments_duality( special_effect_t& effect )
{
  struct torments_duality_cb_t : public dbc_proc_callback_t
  {
    torments_duality_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e )
    {
      // hardcoded since abyss sabre won't always be equipped
      target_debuff = e.player->find_spell( 1265823 );
    }

    buff_t* create_debuff( player_t* t ) override
    {
      if ( auto debuff = buff_t::find( t, "void_tear", listener ) )
        return debuff;

      return make_buff( actor_pair_t( t, listener ), "void_tear", target_debuff );
    }
  };

  auto proxy = effect.player->find_action( "torments_duality" );
  if ( !proxy )
  {
    proxy = new action_t( action_e::ACTION_OTHER, "torments_duality", effect.player );
    proxy->name_str_reporting = "Torment's Duality";
  }

  if ( effect.spell_id == 1253357 )  // abyss sabre
  {
    auto damage = create_proc_action<generic_proc_t>( "abyss_sabre", effect, effect.trigger() );
    damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 1 ).average( effect );
    proxy->add_child( damage );

    // two stacks at a time with set
    auto stacks = find_special_effect( effect.player, 1253358 ) ? 2 : 1;

    effect.player->callbacks.register_callback_execute_function(
      effect.spell_id, [ damage, stacks ]( dbc_proc_callback_t* cb, auto, player_t* t, auto ) {
        damage->execute_on_target( t );
        cb->get_debuff( t )->trigger( stacks );
      } );
  }
  else if ( effect.driver()->id() == 1253359 )  // radiant foil
  {
    effect.player->sim->error( UNVERIFIED_VALUE,
      "Torment's Duality: Damage increase to Radiant Foil per purified Void Tear is assumed to be added to the base "
      "damage before multipliers." );

    auto damage = create_proc_action<generic_proc_t>( "radiant_foil", effect, effect.trigger() );
    damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 1 ).average( effect );
    proxy->add_child( damage );

    // assume additional damage per stack is added to base damage
    auto void_add = effect.driver()->effectN( 2 ).average( effect );

    effect.player->callbacks.register_callback_execute_function(
      effect.spell_id, [ damage, void_add ]( dbc_proc_callback_t* cb, auto, player_t* t, auto ) {
        auto _debuff = cb->get_debuff( t );

        damage->base_dd_adder = void_add * _debuff->check();
        damage->execute_on_target( t );
        damage->base_dd_adder = 0.0;

        _debuff->expire();
      } );
  }

  new torments_duality_cb_t( effect );
}

// Lightless Lament
// 1266257 driver (RPPM equip proc)
// 1266591 missile (intermediate, triggered by driver)
// 1266592 AoE damage (Cosmic, 8yd radius, triggered by missile)
void lightless_lament( special_effect_t& effect )
{
  auto missile = create_proc_action<generic_proc_t>( "heavens_glaive_missile", effect, effect.trigger() );
  auto damage = create_proc_action<generic_aoe_proc_t>( "heavens_glaive", effect,
      missile->data().effectN( 1 ).trigger() );
  damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 1 ).average( effect );
  damage->base_multiplier *= role_mult( effect );

  missile->add_child( damage );
  missile->impact_action = damage;

  effect.execute_action = missile;

  new dbc_proc_callback_t( effect.player, effect );
}

// 1250529 driver
// 1250528 damage
void murder_row_fishhook( special_effect_t& effect )
{
  auto dot = create_proc_action<generic_proc_t>( "murder_row_fishhook", effect, effect.trigger() );
  dot->base_dd_min = dot->base_dd_max = effect.driver()->effectN( 1 ).average( effect );
  dot->base_multiplier *= role_mult( effect );

  effect.execute_action = dot;

  new dbc_proc_callback_t( effect.player, effect );
}
}  // namespace weapons

namespace armors
{
// Eternal Voidsong Chain
// 1271211 Driver
// 1271226 DoT
void eternal_voidsong_chain( special_effect_t& effect )
{
  auto dot     = create_proc_action<generic_proc_t>( "voidstalker_sting", effect, 1271226 );
  dot->base_td = effect.driver()->effectN( 1 ).average( effect );
  dot->base_td_multiplier *= role_mult( effect );

  effect.execute_action = dot;

  effect.player->callbacks.register_callback_trigger_function(
      effect.spell_id, dbc_proc_callback_t::trigger_fn_type::CONDITION,
      []( auto, const auto&, auto, action_state_t* s, auto ) {
        return dbc::has_common_school( s->action->get_school(), SCHOOL_SHADOW );
      } );

  new dbc_proc_callback_t( effect.player, effect );
}

// 1243883 driver
// 1258590 spread counter debuff
// 1258604 dot
// 1258845 direct damage (unknown?)
void necrotic_hexweave( special_effect_t& effect )
{
  effect.player->sim->error( UNVERIFIED_IMPLEMENTATION,
    "Necrotic Hexweave: Implementation assumes hex can only be spread to non-hexed targets, "
    "and each spread applies a full duration hex that cannot spread further." );

  struct necrotic_hexweave_cb_t : public dbc_proc_callback_t
  {
    action_t* dot;

    necrotic_hexweave_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e )
    {
      dot = create_proc_action<generic_proc_t>( "necrotic_hex", e, 1258604 );
      dot->base_td = e.driver()->effectN( 1 ).average( e );
      dot->base_multiplier *= role_mult( e );

      target_debuff = e.trigger();
    }

    buff_t* create_debuff( player_t* t ) override
    {
      return dbc_proc_callback_t::create_debuff( t )
        ->set_initial_stack_to_max_stack()
        ->set_tick_callback( [ this ]( buff_t* b, auto, auto ) {
          if ( b->check() )  // last stack must remain
          {
            auto tl = dot->target_list();  // make a copy
            rng().shuffle( tl.begin(), tl.end() );
            for ( auto t : tl )
            {
              auto target_dot = dot->find_dot( t );
              if ( !target_dot || !target_dot->is_ticking() )
              {
                b->decrement();
                dot->execute_on_target( t );
                break;
              }
            }
          }
        } );
    }

    void execute( const spell_data_t*, player_t* t, action_state_t* ) override
    {
      dot->execute_on_target( t );
      get_debuff( t )->trigger();
    }
  };

  new necrotic_hexweave_cb_t( effect );
}

// 1243876 driver
// 1258545 delay
// 1258556 damage
void rangergenerals_call( special_effect_t& effect )
{
  auto damage =
    create_proc_action<generic_proc_t>( "surprise_attack", effect, effect.trigger()->effectN( 1 ).trigger() );
  damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 1 ).average( effect );
  damage->base_multiplier *= role_mult( effect );

  auto delay = timespan_t::from_millis( effect.trigger()->effectN( 1 ).misc_value1() );

  effect.player->callbacks.register_callback_execute_function(
    effect.spell_id, [ damage, delay ]( const dbc_proc_callback_t* cb, auto, player_t* t, auto ) {
      make_event( *cb->listener->sim, delay, [ damage, t = t ] {
        damage->execute_on_target( t );
      } );
    } );

  new dbc_proc_callback_t( effect.player, effect );
}

// 1243903 driver
// 1246714 orb duration
// 1246739 buffs
void azerothian_power( special_effect_t& effect )
{
  // create all the stat buffs
  std::unordered_map<stat_e, buff_t*> buffs;

  create_all_stat_buffs( effect, effect.player->find_spell( 1246739 ), 0.0, [ &buffs ]( stat_e s, buff_t* b ) {
    buffs[ s ] = b;
  } );

  // create the fake tracker buff to track orb spawns
  auto orb = create_buff<buff_t>( effect.player, "azerothian_power_orb", effect.trigger() )
    ->set_quiet( true )
    ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
    ->set_max_stack( 10 );  // sufficiently high

  effect.custom_buff = orb;

  new dbc_proc_callback_t( effect.player, effect );

  // create a fake callback to proc on next ability that can be cast while moving
  auto pickup = new special_effect_t( effect.player );
  pickup->name_str = "azerothian_power_pickup";
  pickup->spell_id = orb->data().id();
  pickup->proc_flags_ = PF_CAST_SUCCESSFUL;
  pickup->proc_chance_ = 1.0;
  pickup->set_can_only_proc_from_class_abilities( true );
  pickup->set_can_proc_from_procs( false );
  effect.player->special_effects.push_back( pickup );

  struct azerothian_power_pickup_cb_t : public dbc_proc_callback_t
  {
    std::unordered_map<stat_e, buff_t*> buffs;
    buff_t* orb;

    azerothian_power_pickup_cb_t( const special_effect_t& e, std::unordered_map<stat_e, buff_t*> map, buff_t* b )
      : dbc_proc_callback_t( e.player, e ), buffs( std::move( map ) ), orb( b )
    {}

    void trigger( const proc_data_t& data, player_t* t, action_state_t* s, proc_trigger_type_e type ) override
    {
      // trigger only if the action is usable while moving
      // we can't just use usable_moving() since it returns false for melee abilities
      if ( ( s->action->trigger_gcd > 0_ms && s->action->execute_time() == 0_ms ) ||
           ( s->action->channeled && s->action->usable_moving() ) )
      {
        dbc_proc_callback_t::trigger( data, t, s, type );
      }
    }

    void execute( const spell_data_t*, player_t*, action_state_t* s ) override
    {
      if ( auto move_delay = s->action->gcd() - 10_ms; orb->remains_gt( move_delay ) )
      {
        make_event( *listener->sim, move_delay, [ this ] {
          buffs.at( util::highest_stat( orb->player, secondary_ratings ) )->trigger();
          orb->decrement();
        } );
      }
    }
  };

  auto cb = new azerothian_power_pickup_cb_t( *pickup, buffs, orb );
  cb->activate_with_buff( orb );
};

// 1241529 driver
// 1241530 buff
void arcanoweave_cord( special_effect_t& effect )
{
  effect.custom_buff = create_buff<stat_buff_t>( effect.player, effect.trigger() )
    ->set_stat_from_effect_type( A_MOD_RATING, effect.driver()->effectN( 1 ).average( effect ) );

  new dbc_proc_callback_t( effect.player, effect );
};

// 1241503 driver
// 1241522 rppm
// 1241502 damage
void sunfire_sash( special_effect_t& effect )
{
  auto damage =
    create_proc_action<generic_proc_t>( "radiant_conflagration", effect, effect.trigger()->effectN( 1 ).trigger() );
  damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 1 ).average( effect );

  effect.execute_action = damage;
  effect.spell_id = effect.trigger()->id();

  new dbc_proc_callback_t( effect.player, effect );
}

// 1285138 driver
// 1286533 missile
// 1286135 dot
// 1286316 remaining damage
void sporecallers_blooming_loop( special_effect_t& effect )
{
  effect.player->sim->error( UNVERIFIED_IMPLEMENTATION,
    "Sporecaller's Blooming Loop: Implementation assumes damage for remaining dot triggers when the missile hits." );
  effect.player->sim->error( UNVERIFIED_VALUE,
    "Sporecaller's Blooming Loop: Calculation for damage from remaining dot has not been verified." );

  struct rotbloom_missile_t : public generic_proc_t
  {
    action_t* dot;
    action_t* damage;

    rotbloom_missile_t( const special_effect_t& effect ) : generic_proc_t( effect, "rotbloom_missile", 1286533 )
    {
      // set up the dot
      dot = create_proc_action<generic_proc_t>( "rotbloom", effect, 1286135 );
      auto dot_ticks = dot->dot_duration / dot->base_tick_time;
      dot->base_td = effect.driver()->effectN( 1 ).average( effect ) / dot_ticks;
      dot->base_multiplier *= role_mult( effect );

      // set up the remaining damage
      damage = create_proc_action<generic_proc_t>( "rotbloom_damage", effect, 1286316 );
      damage->base_multiplier *= effect.driver()->effectN( 2 ).percent();

      // set up reporting
      dot->dual = damage->dual = true;
      stats = dot->stats;
      damage->stats = dot->stats;
    }

    void impact( action_state_t* state ) override
    {
      generic_proc_t::impact( state );

      if ( auto target_dot = dot->find_dot( state->target ); target_dot && target_dot->is_ticking() )
        damage->execute_on_target( state->target, target_dot->tick_damage_over_remaining_time() );
      else
        dot->execute_on_target( state->target );
    }
  };

  effect.execute_action = create_proc_action<rotbloom_missile_t>( "rotbloom_missile", effect );

  effect.player->callbacks.register_callback_trigger_function(
    effect.spell_id, dbc_proc_callback_t::trigger_fn_type::CONDITION,
    []( auto, const auto&, auto, action_state_t* s, auto ) {
      return dbc::has_common_school( s->action->get_school(), SCHOOL_NATURE );
    } );

  new dbc_proc_callback_t( effect.player, effect );
}

// 1285139 driver
// 1285161 shield
// 1285163 aoe
void rotmires_sporeheart( special_effect_t& effect )
{
  effect.player->sim->error( UNVERIFIED_IMPLEMENTATION,
    "Rotmire's Sporeheart: Damage is assumed to split and increase by 15% per target hit. "
    "Shield is assumed to apply immediately and will only burst when depleted by incoming damage. "
    "What types of triggers can proc the shield has not been verified." );

  struct protective_toadstool_buff_t : public absorb_buff_t
  {
    action_t* damage;

    protective_toadstool_buff_t( const special_effect_t& e )
      : absorb_buff_t( e.player, "protective_toadstools", e.player->find_spell( 1285161 ) )
    {
      damage = create_proc_action<generic_aoe_proc_t>( "bursting_toadstools", e, 1285163 );
      damage->base_dd_min = damage->base_dd_max = e.driver()->effectN( 3 ).average( e );
      damage->base_multiplier *= role_mult( e );

      set_default_value( e.driver()->effectN( 2 ).average( e ) );
    }

    void absorb_used( double, player_t* t ) override
    {
      if ( current_value <= 0 && t )
        damage->execute_on_target( t );
    }
  };

  effect.custom_buff = make_buff<protective_toadstool_buff_t>( effect );

  new dbc_proc_callback_t( effect.player, effect );
}
}  // namespace armors

namespace sets
{
// 1244005 driver
// 1259297 coeff
// 1259504 shiv missile
// 1259264 shiv direct damage
// 1259503 crystal missile
// 1259273 crystal aoe damage
// 1259508 tonic missile
// 1259276 tonic heal
namespace
{
template <typename T, typename U>
action_t* create_mrm_action( std::string n, const special_effect_t& e, unsigned id, double a )
{
  auto missile = create_proc_action<U>( n + "_missile", e, id );
  auto impact = create_proc_action<T>( n, e, missile->data().effectN( 1 ).trigger() );
  impact->base_dd_min = impact->base_dd_max = a;
  missile->dual = true;
  missile->stats = impact->stats;  // report the damage only
  missile->impact_action = impact;

  return missile;
}
}  // namespace

void murder_row_materials( special_effect_t& effect )
{
  auto equip = find_special_effects( effect.player, 1259297 );
  assert( !equip.empty() && "Murder Row Materials missing equip effect" );

  double shiv_amount = 0;
  double crystal_amount = 0;
  double tonic_amount = 0;

  range::for_each( equip, [ & ]( auto e ) {
    shiv_amount += e->driver()->effectN( 1 ).average( *e );
    crystal_amount += e->driver()->effectN( 2 ).average( *e );
    tonic_amount += e->driver()->effectN( 3 ).average( *e );
  } );

  auto shiv = create_mrm_action<generic_proc_t, generic_proc_t>(
    "murder_row_shiv", effect, 1259504, shiv_amount );

  auto crystal = create_mrm_action<generic_aoe_proc_t, generic_proc_t>(
    "slightlystabilized_arcanocrystal", effect, 1259503, crystal_amount );
  auto crystal_impact = static_cast<generic_aoe_proc_t*>( crystal->impact_action );
  crystal_impact->split_aoe_damage = crystal_impact->aoe_damage_increase = false;

  auto tonic = create_mrm_action<generic_heal_t, generic_heal_t>(
    "emergency_healing_tonic", effect, 1259508, tonic_amount );

  effect.proc_flags2_ = PF2_CRIT;
  effect.player->callbacks.register_callback_execute_function(
    effect.spell_id, [ shiv, crystal, tonic ]( auto, auto, player_t* t, auto ) {
      if ( !t->is_enemy() )
      {
        tonic->execute_on_target( t );
      }
      else
      {
        if ( crystal->target_list().size() > 1 )
          crystal->execute_on_target( t );
        else
          shiv->execute_on_target( t );
      }
    } );

  new dbc_proc_callback_t( effect.player, effect );
}

// 1244021 driver
// 1258885 vers
// 1258886 mast
// 1258887 haste
// 1258890 crit
void root_wardens_regalia( special_effect_t& effect )
{
  // driver does not have SX_SCALE_ILEVEL so average() scales from player level
  auto stat_amount = effect.driver()->effectN( 1 ).average( effect );

  std::vector<stat_buff_t*> buffs;

  for ( auto id : { 1258885, 1258886, 1258887, 1258890 } )
  {
    auto _b = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( id ) )
      ->add_stat_from_effect_type( A_MOD_RATING, stat_amount );
    buffs.push_back( _b );
  }

  effect.player->callbacks.register_callback_execute_function(
    effect.spell_id, [ buffs ]( const dbc_proc_callback_t* cb, auto, auto, auto ) {
      cb->rng().range( buffs )->trigger();
    } );

  new dbc_proc_callback_t( effect.player, effect );
};

// Voidlight Bindings
// 1281574 Set Driver
// 1281581 Equip
// 1281580 Area Trigger
// 1281579 Damage
void voidlight_bindings( special_effect_t& effect )
{
  auto equip = find_special_effects( effect.player, 1281581 );
  assert( !equip.empty() && "Voidlight Bindings missing equip effect" );

  auto damage = create_proc_action<generic_aoe_proc_t>( "twilight_barrage", effect, 1281579 );

  range::for_each( equip, [ damage ]( auto e ) {
    damage->base_dd_min += e->driver()->effectN( 1 ).average( *e );
    damage->base_dd_max += e->driver()->effectN( 1 ).average( *e );
  } );

  // No Role multiplier currently
  //damage->base_multiplier *= role_mult( effect );

  effect.execute_action = damage;
  new dbc_proc_callback_t( effect.player, effect );
}

// Umbra-Weaver's Portent
// 253819 Driver
// 1290152 Equip Driver
// 253826 Mastery Buff
void umbral_shift( special_effect_t& effect)
{
  auto equip = find_special_effects( effect.player, 1290152 );
  assert( !equip.empty() && "Umbra-Weaver's Portent missing equip effect" );

  auto buff = effect.driver()->effectN( 1 ).trigger();
  auto stat_buff = create_buff<stat_buff_t>( effect.player, buff );

  const bool has_3pc = effect.player->sets->has_set_bonus(
    effect.player->specialization(), MID_UWP, B3 );

  // 3pc is required for the buff to grant mastery
  if ( has_3pc )
  {
    range::for_each( equip, [ stat_buff ]( auto effect ) {
        auto stat_coeff = effect->driver()->effectN( 2 ).average( *effect );
        stat_buff->add_stat_from_effect_type( A_MOD_RATING, stat_coeff );
      } );
  }

  effect.custom_buff = stat_buff;
  new dbc_proc_callback_t( effect.player, effect );
}

// 1241262 driver
// 1241227 coeff (unused?)
// 1241289 buff
void arcanoweave_trappings( special_effect_t& effect )
{
  effect.player->sim->error(
    UNVERIFIED_VALUE,
    "Arcanoweave Trappings: How the buff value scales with item level is unknown. "
    "Currently implemented to scale off player level and values have not been verified in-game." );

  struct arcanoweave_trappings_t : public stat_buff_t
  {
    rng::truncated_gauss_t interval;

    arcanoweave_trappings_t( player_t* p, std::string_view n, const spell_data_t* s )
      : stat_buff_t( p, n, s ),
        interval( p->midnight_opts.arcanoweave_trappings_update_interval,
                  p->midnight_opts.arcanoweave_trappings_update_interval_stddev )
    {}
  };

  auto buff = create_buff<arcanoweave_trappings_t>( effect.player, effect.trigger() );

  effect.player->register_precombat_begin( [ buff ]( player_t* p ) {
    buff->trigger();

    make_repeating_event( *p->sim,
      [ p, buff ] { return p->rng().gauss( buff->interval ); },
      [ p, buff ] {
        if ( p->rng().roll( p->midnight_opts.arcanoweave_trappings_uptime ) )
          buff->trigger();
        else
          buff->expire();
      } );
  } );
}

// 1270977 driver
// 1270985 buff
void sunfire_silk_trappings( special_effect_t& effect )
{
  effect.player->sim->error(
    UNVERIFIED_VALUE,
    "Sunfire Silk Trappings: How the buff value scales with item level is unknown. "
    "Currently implemented to scale off player level and values have not been verified in-game." );

  struct sunfire_silk_trappings_t : public stat_buff_t
  {
    rng::truncated_gauss_t interval;

    sunfire_silk_trappings_t( player_t* p, std::string_view n, const spell_data_t* s )
      : stat_buff_t( p, n, s ),
        interval( p->midnight_opts.sunfire_silk_trappings_update_interval,
                  p->midnight_opts.sunfire_silk_trappings_update_interval_stddev )
    {}
  };

  auto buff = create_buff<sunfire_silk_trappings_t>( effect.player, effect.trigger() );

  effect.player->register_precombat_begin( [ buff ]( player_t* p ) {
    buff->trigger();

    make_repeating_event( *p->sim,
      [ p, buff ] { return p->rng().gauss( buff->interval ); },
      [ p, buff ] {
        if ( p->rng().roll( p->midnight_opts.sunfire_silk_trappings_uptime ) )
          buff->trigger();
        else
          buff->expire();
      } );
  } );
}
}  // namespace sets

namespace omnium
{
// Item level used for omnium coeff scaling spell 1288183
static constexpr unsigned OMNIUM_ITEM_LEVEL = 289;

struct rune_of_echoes_debuff_t : public buff_t
{
  double accumulator;
  action_t* echo;

  rune_of_echoes_debuff_t( actor_pair_t pair, std::string_view n, const spell_data_t* s,
                           action_t* d )
    : buff_t( pair, n, s ), accumulator( 0 ), echo( d )
  {
  }

  void reset() override
  {
    buff_t::reset();
    accumulator = 0;
  }

  void start( int s, double v, timespan_t d ) override
  {
    buff_t::start( s, v, d );
    accumulator = 0;
  }

  void expire_override( int s, timespan_t d ) override
  {
    buff_t::expire_override( s, d );
    if ( accumulator > 0 )
    {
      echo->execute_on_target( player, accumulator );
      accumulator = 0;
    }
  }
};

template <typename BASE>
struct lingering_rune_t : public BASE
{
  bool has_echoes;
  double echo_coeff;
  action_t* echo;

  lingering_rune_t( const special_effect_t& e, std::string_view n, const spell_data_t* s, bool ech, double coef, action_t* d )
    : BASE( e, n, s ), has_echoes( ech ), echo_coeff( coef ), echo( d )
  {
  }

  buff_t* create_debuff( player_t* t ) override
  {
    auto debuff = buff_t::find( t, "rune_of_echoes_debuff" );

    if ( !debuff )
      debuff = make_buff<rune_of_echoes_debuff_t>( actor_pair_t( t, BASE::player ), "rune_of_echoes_debuff",
                                                   BASE::player->find_spell( 1289063 ), echo );

    return debuff;
  }

  void tick( dot_t* d ) override
  {
    BASE::tick( d );
    if ( !has_echoes )
      return;

    rune_of_echoes_debuff_t* debuff = debug_cast<rune_of_echoes_debuff_t*>( BASE::get_debuff( d->target ) );
    if ( debuff->check() )
      debuff->accumulator += d->state->result_amount * echo_coeff;
  }
};

template <typename BASE>
struct omnium_core_rune_t : public BASE
{
  stat_buff_t* buff = nullptr;
  const spell_data_t* coeff;
  double dmg_coeff;
  double stat_coeff;
  bool has_echoes;
  double echo_coeff;
  action_t* echo;

  omnium_core_rune_t( const special_effect_t& e, std::string_view n, unsigned id )
    : BASE( e, n, id ),
      coeff( e.driver()->effectN( 2 ).trigger() ),
      dmg_coeff( std::floor( coeff->effectN( 1 ).average_no_item( BASE::player, OMNIUM_ITEM_LEVEL ) ) * 0.01 ),
      stat_coeff( std::floor( coeff->effectN( 3 ).average_no_item( BASE::player, OMNIUM_ITEM_LEVEL ) ) * 0.01 ),
      has_echoes( false ),
      echo_coeff( 0 ),
      echo( nullptr )
  {
    constexpr bool heal = std::is_base_of_v<heal_t, BASE>;

    if ( heal )
      BASE::name_str_reporting = "Heal";

    BASE::base_dd_min = BASE::base_dd_max = dmg_coeff * BASE::data().effectN( 2 ).base_value();

    has_echoes = find_special_effect( e.player, 1279616 ) != nullptr;
    if ( has_echoes )
    {
      echo              = create_proc_action<BASE>( fmt::format( "{}_echo", n ), e, heal ? 1303071 : 1303048 );
      echo->base_dd_min = echo->base_dd_max = 1.0;  // actual value is determined by accumulator
      echo->name_str_reporting               = "rune_of_echoes";
      echo_coeff                            = e.player->find_spell( 1279616 )->effectN( 1 ).percent();
      if( !echo->stats->parent )
        BASE::add_child( echo );
    }

    // Rune of Lingering: 1287555 driver, 1287663 dot, 1287665 hot
    if ( find_special_effect( e.player, 1287555 ) )
    {
      auto name = fmt::format( "{}_lingering", n );
      auto dot  = create_proc_action<lingering_rune_t<BASE>>(
          name, e, name, heal ? e.player->find_spell( 1287665 ) : e.player->find_spell( 1287663 ), has_echoes,
          echo_coeff, echo );

      dot->base_td = dmg_coeff * dot->data().effectN( 2 ).base_value() / dot->dot_duration.value().total_seconds();
      dot->name_str_reporting = "rune_of_lingering";
      if ( !dot->stats->parent )
        BASE::add_child( dot );

      BASE::impact_action = dot;

      // Rune of Residual Energy: 1279615 driver
      if ( auto residual = find_special_effect( e.player, 1279615 ) )
        dot->base_multiplier *= 1.0 + residual->driver()->effectN( 1 ).percent();
    }

    apply_stat_rune( 1279609, 1287772 );  // Rune of Critical Power
    apply_stat_rune( 1279610, 1287774 );  // Rune of Burning Haste
    apply_stat_rune( 1279612, 1287771 );  // Rune of Masterful Cunning
    apply_stat_rune( 1279613, 1287770 );  // Rune of the Versatile Warrior

    // Rune of Overload: 1279614 driver
    if ( auto overload = find_special_effect( e.player, 1279614 ) )
      BASE::base_multiplier *= 1.0 + overload->driver()->effectN( 1 ).percent();
  }

  void apply_stat_rune( unsigned driver_id, unsigned buff_id )
  {
    if ( !find_special_effect( BASE::player, driver_id ) )
      return;

    buff = create_buff<stat_buff_t>( BASE::player, BASE::player->find_spell( buff_id ) );
    buff->set_stat_from_effect_type( A_MOD_RATING, stat_coeff * buff->data().effectN( 2 ).base_value() );
  }

  void execute() override
  {
    BASE::execute();

    if ( buff )
      buff->trigger();
  }

  buff_t* create_debuff( player_t* t ) override
  {
    auto debuff = buff_t::find( t, "rune_of_echoes_debuff" );

    if ( !debuff )
      debuff = make_buff<rune_of_echoes_debuff_t>( actor_pair_t( t, BASE::player ), "rune_of_echoes_debuff",
                                                   BASE::player->find_spell( 1289063 ), echo );

    return debuff;
  }

  void impact( action_state_t* s ) override
  {
    BASE::impact( s );
    if ( !has_echoes )
      return;

    rune_of_echoes_debuff_t* debuff = debug_cast<rune_of_echoes_debuff_t*>( BASE::get_debuff( s->target ) );
    // TODO: is the accumulator value increased on the initial hit?
    if ( !debuff->check() )
      debuff->trigger();
    else
      debuff->accumulator += s->result_amount * echo_coeff;
  }
};

// 1279599 driver
// 1286970 damage
// 1263002 heal
void rune_of_unleashed_fire( special_effect_t& effect )
{
  effect.player->sim->error( UNVERIFIED_IMPLEMENTATION,
    "Rune of Unleashed Fire: Procs are assumed to target the same unit that triggered them. "
    "Procs triggered by damage are assumed to proc damage. "
    "Procs triggered by healing/aura are assumed to proc heal." );

  auto damage =
    create_proc_action<omnium_core_rune_t<generic_proc_t>>( "rune_of_unleashed_fire", effect, 1286970 );

  auto heal =
    create_proc_action<omnium_core_rune_t<generic_heal_t>>( "rune_of_unleashed_fire_heal", effect, 1263002 );

  effect.player->callbacks.register_callback_execute_function(
    effect.spell_id, [ damage, heal ]( auto, auto, player_t* t, auto ) {
      if ( t->is_enemy() )
        damage->execute_on_target( t );
      else
        heal->execute_on_target( t );
    } );

  new dbc_proc_callback_t( effect.player, effect );
}

// 1279596 driver
// 1286690 orb1?
// 1286716 damage
// 1286721 heal
// 1287255 orb2?
// 1287256 orb3?
// 1287257 orb4?
// 1287258 orb5?
// 1287425 orb counter/driver
void rune_of_voidtouched_orbs( special_effect_t& effect )
{
  effect.player->sim->error( UNVERIFIED_IMPLEMENTATION,
    "Rune of Voidtouched Orbs: Orbs are assumed to proc on hit and require a damage/healing amount. "
    "Procs are assumed to target the same unit that triggered them. "
    "All procs are assumed to fire individually at the same time when triggered. "
    "Orbs are assumed to stack while out of combat." );

  auto damage =
    create_proc_action<omnium_core_rune_t<generic_proc_t>>( "rune_of_voidtouched_orbs", effect, 1286716 );

  auto heal =
    create_proc_action<omnium_core_rune_t<generic_heal_t>>( "rune_of_voidtouched_orbs_heal", effect, 1286721 );

  // create orb buff & periodic trigger
  auto orb_buff = create_buff( effect.player, effect.trigger() );
  auto period = effect.driver()->effectN( 1 ).period();

  effect.player->register_precombat_begin( [ orb_buff, period ]( player_t* p ) {
    orb_buff->trigger( orb_buff->max_stack() );
    make_event( *p->sim, p->rng().range( 1_ms, period ), [ orb_buff, period ] {
      orb_buff->trigger();
      make_repeating_event( *orb_buff->sim, period, [ orb_buff ] { orb_buff->trigger(); } );
    } );
  } );

  // create damage/heal callback
  auto orb = new special_effect_t( effect.player );
  orb->name_str = "rune_of_voidtouched_orbs";
  orb->spell_id = effect.trigger()->id();
  orb->proc_flags2_ = PF2_ALL_HIT;  // TODO: confirm
  effect.player->special_effects.push_back( orb );

  effect.player->callbacks.register_callback_trigger_function(
    orb->spell_id, dbc_proc_callback_t::trigger_fn_type::CONDITION,
    []( auto, const auto&, auto, action_state_t* s, auto ) {
      return s && s->result_amount > 0.0;  // TODO: confirm only trigger on hits that do damage/healing
    } );

  effect.player->callbacks.register_callback_execute_function(
    orb->spell_id, [ orb_buff, damage, heal ]( auto, auto, player_t* t, auto ) {
      auto stacks = orb_buff->check();
      orb_buff->expire();

      while ( stacks-- )
      {
        if ( t->is_enemy() )
          damage->execute_on_target( t );
        else
          heal->execute_on_target( t );
      }
    } );

  auto orb_cb = new dbc_proc_callback_t( effect.player, *orb );
  orb_cb->activate_with_buff( orb_buff );
}
}  // namespace omnium

void register_special_effects()
{
  // NOTE: use unique_gear:: namespace for static consumables so we don't activate them with enable_all_item_effects
  // Food
  unique_gear::register_special_effect( 1232257, consumables::selector_food( 1219185, true ) );  // bloom skewers / [PH] Vegetarian Recipe
  unique_gear::register_special_effect( 1232916, consumables::selector_food( 1219185, true ) );  // braised blood hunter
  unique_gear::register_special_effect( 1232915, consumables::selector_food( 1219185, true ) );  // crimson calamari
  unique_gear::register_special_effect( 1219187, consumables::selector_food( 1219185, true ) );  // felberry figs
  unique_gear::register_special_effect( 1232914, consumables::selector_food( 1219185, true ) );  // tasty smoked tetra
  unique_gear::register_special_effect( 1232489, consumables::selector_food( 1219185, true ) );  // twilight angler's medley
  unique_gear::register_special_effect( 1259656, consumables::selector_food( 1232324, true ) );  // blooming feast
  unique_gear::register_special_effect( 1232919, consumables::selector_food( 1233408, true ) );  // flora frenzy / champion's bento
  unique_gear::register_special_effect( 1259657, consumables::selector_food( 1232325, true ) );  // quel'dorei medley
  unique_gear::register_special_effect( 1259658, consumables::primary_food( 1232582, STAT_STR_AGI_INT, 2 ) ); // rootland celebration
  unique_gear::register_special_effect( 1259659, consumables::primary_food( 1232585, STAT_STR_AGI_INT, 2 ) ); // silvermoon parade
  unique_gear::register_special_effect( 1232917, consumables::primary_food( 1232584, STAT_STR_AGI_INT, 2 ) );  // [impossibly] royal roast
  unique_gear::register_special_effect( 1232902, consumables::secondary_food( 1219183, STAT_CRIT_RATING ) ); // arcano cutlets
  unique_gear::register_special_effect( 1232903, consumables::secondary_food( 1232087, STAT_HASTE_RATING ) ); // fel-kissed filet
  unique_gear::register_special_effect( 1232905, consumables::secondary_food( 1232089, STAT_MASTERY_RATING ) ); // warped wise wings
  unique_gear::register_special_effect( 1232906, consumables::secondary_food( 1232091, STAT_VERSATILITY_RATING ) ); // void-kissed fish rolls
  unique_gear::register_special_effect( 1232253, consumables::secondary_food( 1232321, STAT_CRIT_RATING, STAT_VERSATILITY_RATING ) ); // spiced biscuits
  unique_gear::register_special_effect( 1232910, consumables::secondary_food( 1232492, STAT_VERSATILITY_RATING, STAT_SPEED_RATING ) ); // buttered root crab
  unique_gear::register_special_effect( 1232481, consumables::secondary_food( 1233400, STAT_MASTERY_RATING, STAT_HASTE_RATING ) ); // bloodthistle-wrapped cutlets
  unique_gear::register_special_effect( 1232485, consumables::secondary_food( 1232318, STAT_MASTERY_RATING, STAT_CRIT_RATING ) ); // eversong pudding
  unique_gear::register_special_effect( 1232246, consumables::secondary_food( 1232316, STAT_MASTERY_RATING, STAT_HASTE_RATING ) ); // farstrider rations
  unique_gear::register_special_effect( 1232252, consumables::secondary_food( 1233403, STAT_MASTERY_RATING, STAT_VERSATILITY_RATING ) ); // silvermoon standard
  unique_gear::register_special_effect( 1232908, consumables::secondary_food( 1232491, STAT_MASTERY_RATING, STAT_SPEED_RATING ) ); // null and void plate
  unique_gear::register_special_effect( 1232483, consumables::secondary_food( 1233401, STAT_MASTERY_RATING, STAT_SPEED_RATING ) ); // hearthflame supper
  unique_gear::register_special_effect( 1232251, consumables::secondary_food( 1233404, STAT_MASTERY_RATING, STAT_CRIT_RATING ) ); // forager's medley
  unique_gear::register_special_effect( 1232487, consumables::secondary_food( 1233402, STAT_CRIT_RATING, STAT_VERSATILITY_RATING ) ); // wise tails
  unique_gear::register_special_effect( 1232486, consumables::secondary_food( 1232318, STAT_CRIT_RATING, STAT_VERSATILITY_RATING ) ); // fried bloomtail
  unique_gear::register_special_effect( 1232909, consumables::secondary_food( 1232493, STAT_HASTE_RATING, STAT_SPEED_RATING ) ); // glitter skewers
  unique_gear::register_special_effect( 1232250, consumables::secondary_food( 1233405, STAT_VERSATILITY_RATING, STAT_HASTE_RATING ) ); // quick sandwich
  unique_gear::register_special_effect( 1232907, consumables::secondary_food( 1232490, STAT_CRIT_RATING, STAT_SPEED_RATING ) ); // sun-seared lumifin
  unique_gear::register_special_effect( 1232249, consumables::secondary_food( 1233401, STAT_CRIT_RATING, STAT_HASTE_RATING ) ); // portable snack
  unique_gear::register_special_effect( 1232484, consumables::secondary_food( 1233405, STAT_VERSATILITY_RATING, STAT_HASTE_RATING ) ); // sunwell delight
  // Flasks
  // Potions
  register_special_effect( 1236998, consumables::draught_of_rampant_abandon );
  register_special_effect( 1236994, consumables::potion_of_recklessness );
  register_special_effect( 1238443, consumables::potion_of_zealotry );
  // Oils
  register_special_effect( { 1262056, 1262111 }, consumables::laced_zoomshots );
  register_special_effect( { 1237009, 1237012 }, consumables::smugglers_enchanted_edge );
  register_special_effect( { 1262120, 1262141 }, consumables::weighted_boomshots );
  // Enchants & gems
  register_special_effect( 1258209, enchants::powerful_eversong_diamond, false, true );
  register_special_effect( { 1236733, 1236734 }, enchants::strength_of_halazzi );
  register_special_effect( { 1236739, 1236740 }, enchants::flames_of_the_sindorei );
  register_special_effect( { 1236741, 1236742,    // Acuity of the Ren'dorei (Primary)
                             1236727, 1236728,    // Berserker's Rage (Haste)
                             1236712, 1236721,    // Arcane Mastery (Mastery)
                             1236724, 1236725,    // Janalai's Precision (Crit)
                             1236729, 1236730 },  // Worldsoul Tenacity (Vers)
                           enchants::stat_weapon_enchant );
  register_special_effect( { 1236700, 1236701 }, enchants::eyes_of_the_eagle, false, true );
  register_special_effect( { 1262295, 1262298 }, enchants::smugglers_lynxeye );
  register_special_effect( { 1262337, 1262339 }, enchants::farstriders_hawkeye );
  // Embellishments & Tinkers
  register_special_effect( 1283697, embellishments::arcanoweave_lining );
  register_special_effect( 1241711, embellishments::sunfire_silk_lining );
  register_special_effect( 1244238, embellishments::devouring_banding );
  register_special_effect( 1259060, embellishments::blessed_pango_charm );
  register_special_effect( 1244276, embellishments::primal_spore_binding );
  register_special_effect( 1251906, embellishments::prismatic_focusing_iris );
  register_special_effect( 1251905, DISABLED_EFFECT );  // stabilizing gemstone bandolier
  register_special_effect( 1251815, embellishments::thalassian_phoenix_torque );
  register_special_effect( 1251902, embellishments::signet_of_azerothian_blessings );
  register_special_effect( 1251904, embellishments::loa_worshipers_band );
  register_special_effect( 1261968, embellishments::b0p_curator_of_booms );
  register_special_effect( 1246309, embellishments::b1p_scorcher_of_souls );
  // Darkmoon Trinkets & Embellishments
  register_special_effect( { 1245001, 1245053 }, darkmoon::blood );
  register_special_effect( { 1245055, 1245051 }, darkmoon::rot );
  register_special_effect( { 1245052, 1244254 }, darkmoon::void_ );
  register_special_effect( { 1245050, 1245054 }, darkmoon::hunt );
  // Trinkets
  register_special_effect( 1250599, trinkets::heart_of_the_wind );
  register_special_effect( 1250563, trinkets::kroluks_warbanner );
  register_special_effect( 1250602, trinkets::vessel_of_souls );
  register_special_effect( 1250582, trinkets::mark_of_light );
  register_special_effect( 1254640, trinkets::solarflare_prism );
  register_special_effect( 1250604, trinkets::sapling_of_the_dawnroot );
  register_special_effect( 1250580, trinkets::seed_of_the_devouring_wild );
  register_special_effect( 1259351, DISABLED_EFFECT );  // Seed of the Devouring Wild equip driver
  register_special_effect( 1250567, trinkets::idol_of_the_war_loa );
  register_special_effect( 1256896, trinkets::gaze_of_the_alnseer );
  register_special_effect( 1250564, trinkets::resonant_bellowstone );
  register_special_effect( 1256790, trinkets::undreamt_gods_oozing_vestige );
  register_special_effect( 1259633, trinkets::light_company_guidon );
  register_special_effect( 1251817, DISABLED_EFFECT );  // Light Company Guidon equip driver
  register_special_effect( 1251822, trinkets::heart_of_ancient_hunger );
  register_special_effect( { 1260592, 1265809 }, trinkets::plume_of_beloren ); // Radiant and Umbral Plume
  register_special_effect( { 1265806, 1265805 }, DISABLED_EFFECT ); // Radiant and Umbral Plume on use
  register_special_effect( 1253115, trinkets::sealed_chaos_urn );
  register_special_effect( 1253111, trinkets::lost_idol_of_the_hashey );
  register_special_effect( 1253110, trinkets::withered_saptors_paw );
  register_special_effect( 1259518, trinkets::shadow_of_the_empyrean_requiem );
  register_special_effect( 1250557, trinkets::void_execution_mandate );
  register_special_effect( 1250508, trinkets::emberwing_feather );
  register_special_effect( 1260266, trinkets::ranger_captains_iridescent_insignia );
  register_special_effect( 1260265, DISABLED_EFFECT ); // Ranger-Captain's Iridescent Insignia equip driver
  register_special_effect( 1250601, trinkets::eye_of_the_drowning_void );
  register_special_effect( 1254193, trinkets::latchs_crooked_hook );
  register_special_effect( 1255379, DISABLED_EFFECT );  // latch's crooked hook equip driver
  register_special_effect( 1250527, trinkets::lightspire_core );
  register_special_effect( 1280591, trinkets::magisters_alchemist_stone );
  register_special_effect( 1260459, trinkets::vaelgors_final_stare );
  register_special_effect( 1259293, DISABLED_EFFECT ); // Vaelgor's Final Stare equip driver
  register_special_effect( 1259314, trinkets::locuswalkers_ribbon);
  register_special_effect( 1259153, trinkets::wraps_of_cosmic_madness);
  register_special_effect( 1259103, DISABLED_EFFECT); // Wraps of the Cosmic Madness equip driver
  register_special_effect( 1253113, trinkets::voidreapers_libram );
  register_special_effect( 1258275, DISABLED_EFFECT );  // litany of lightblind wrath
  register_special_effect( 1250589, trinkets::crawling_plague );  // tumor of the swarm
  register_special_effect( 1250541, trinkets::echo_of_the_evercurse );  // soulcatcher's charm
  register_special_effect( 1250546, trinkets::mindpiercers_sigil );  // mindpiercer's sigil
  register_special_effect( 1258283, trinkets::litany_of_lightblind_wrath );  // litany of lightblind wrath on-use
  register_special_effect( 1258275, DISABLED_EFFECT );  // litany of lightblind wrath equip driver
  register_special_effect( 71563, trinkets::deadly_precision );  // nevermelting ice crystal on-use
  register_special_effect( 1272091, trinkets::crucible_of_erratic_energies );
  register_special_effect( 1253114, trinkets::evercollapsing_void_fissure );
  register_special_effect( 1255278, trinkets::tangle_of_vibrant_vines );
  register_special_effect( 1254752, trinkets::refueling_orb );
  register_special_effect( 1258535, trinkets::volatile_void_suffuser );
  register_special_effect( 1272693, trinkets::astalors_anguish_agitator );
  register_special_effect( 1272690, DISABLED_EFFECT ); // Astalors Anguish Agitator Passive Driver
  register_special_effect( 1247311, DISABLED_EFFECT ); // Drum of Renewed Bonds on use
  register_special_effect( 1253120, trinkets::glorious_crusaders_keepsake ); 
  register_special_effect( 1253112, trinkets::sylvan_wakrapuku );
  register_special_effect( 1260633, trinkets::gloomspattered_dreadscale );
  register_special_effect( 1260627, DISABLED_EFFECT );  // Gloom-Spattered Dreadscale Passive Driver
  set_min_version( wowv_t( 12, 0, 7 ) );
  register_special_effect( 1284696, trinkets::sporelords_mycelium );
  reset_version_check();
  // Weapons
  register_special_effect( { 1253357, 1253359 }, weapons::torments_duality );  // umbral sabre & radiant foil
  register_special_effect( 1266257, weapons::lightless_lament );
  register_special_effect( 1250529, weapons::murder_row_fishhook );
  // Armor
  register_special_effect( 1271211, armors::eternal_voidsong_chain );
  register_special_effect( 1243883, armors::necrotic_hexweave );
  register_special_effect( 1243876, armors::rangergenerals_call );
  register_special_effect( 1243903, armors::azerothian_power );
  register_special_effect( 1241529, armors::arcanoweave_cord );
  register_special_effect( 1241503, armors::sunfire_sash );
  set_min_version( wowv_t( 12, 0, 7 ) );
  register_special_effect( 1285138, armors::sporecallers_blooming_loop );
  register_special_effect( 1285139, armors::rotmires_sporeheart );
  reset_version_check();
  // Sets
  register_special_effect( 1281574, sets::voidlight_bindings );
  register_special_effect( 1281581, DISABLED_EFFECT );  // voidlight bindings equip effect
  register_special_effect( 1244005, sets::murder_row_materials );
  register_special_effect( 1259297, DISABLED_EFFECT );  // murder row materials equip effect
  register_special_effect( 1244021, sets::root_wardens_regalia );
  register_special_effect( 1241262, sets::arcanoweave_trappings );
  register_special_effect( 1270977, sets::sunfire_silk_trappings );
  register_special_effect( 1253358, DISABLED_EFFECT );  // torments duality
  register_special_effect( 253819, sets::umbral_shift );
  register_special_effect( 1290152, DISABLED_EFFECT ); // umbral shift equip effect
  // Omnium Folio
  set_min_version( wowv_t( 12, 0, 7 ) );
  register_special_effect( 1279599, omnium::rune_of_unleashed_fire );
  register_special_effect( 1279596, omnium::rune_of_voidtouched_orbs );
  register_special_effect( 1287555, DISABLED_EFFECT );  // rune of lingering
  register_special_effect( 1279609, DISABLED_EFFECT );  // rune of critical power
  register_special_effect( 1279610, DISABLED_EFFECT );  // rune of burning haste
  register_special_effect( 1279612, DISABLED_EFFECT );  // rune of masterful cunning
  register_special_effect( 1279613, DISABLED_EFFECT );  // rune of the versatile warrior
  register_special_effect( 1279614, DISABLED_EFFECT );  // rune of overload
  register_special_effect( 1279615, DISABLED_EFFECT );  // rune of residual energy
  register_special_effect( 1279616, DISABLED_EFFECT );  // rune of echoes
  reset_version_check();
}

void register_target_data_initializers( sim_t& )
{}

void register_actor_initializers( sim_t& )
{}

void register_hotfixes()
{}

action_t* create_action( player_t* /*p*/ , std::string_view /*name*/, std::string_view /*opt*/ )
{
  return nullptr;
}

double bandolier_mul( player_t* p )
{
  if ( unique_gear::find_special_effect( p, 1251905 ) )
    return 2.0;  // hardcoded
  else
    return 1.0;
}
}  // namespace unique_gear::midnight
