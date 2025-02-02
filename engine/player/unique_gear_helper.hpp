// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "action/action_state.hpp"
#include "action/attack.hpp"
#include "action/heal.hpp"
#include "action/action.hpp"
#include "action/spell.hpp"
#include "buff/buff.hpp"
#include "dbc/dbc.hpp"
#include "item/special_effect.hpp"
#include "player/action_priority_list.hpp"
#include "player/player.hpp"
#include "player/pet.hpp"
#include "player/special_effect_db_item.hpp"
#include "player/unique_gear.hpp"
#include "player/wrapper_callback.hpp"
#include "util/scoped_callback.hpp"
#include "sim/sim.hpp"
#include "stats.hpp"

#include <type_traits>
#include <utility>

namespace unique_gear
{
// Old-style special effect registering functions
void register_special_effect( unsigned spell_id, const char* encoded_str );
void register_special_effect( unsigned spell_id, std::function<void( special_effect_t& )> init_callback,
                              bool fallback = false );
// register multiple IDs to the same callback
void register_special_effect( std::initializer_list<unsigned> spell_ids,
                              std::function<void( special_effect_t& )> init_callback, bool fallback = false );

// New-style special effect registering function
template <typename T, typename = std::enable_if_t<std::is_base_of<scoped_callback_t, T>::value>>
void register_special_effect( unsigned spell_id, const T& cb, bool fallback = false )
{
  special_effect_db_item_t dbitem;
  dbitem.spell_id = spell_id;
  dbitem.cb_obj   = new T( cb );
  dbitem.fallback = fallback;

  add_effect( dbitem );
}

// Utility helpers
bool create_fallback_buffs( const special_effect_t& effect, const std::vector<util::string_view>& names );
void init_feast( special_effect_t& effect, std::initializer_list<std::pair<stat_e, int>> stat_map);
void DISABLED_EFFECT( special_effect_t& effect );

// A scoped special effect callback that validates against a player class or specialization.
struct class_scoped_callback_t : public scoped_callback_t
{
  std::vector<player_e> class_;
  std::vector<specialization_e> spec_;

  class_scoped_callback_t( player_e c ) : scoped_callback_t( PRIORITY_CLASS ), class_( { c } )
  {
  }

  class_scoped_callback_t( std::vector<player_e> c ) : scoped_callback_t( PRIORITY_CLASS ), class_( std::move( c ) )
  {
  }

  class_scoped_callback_t( specialization_e s ) : scoped_callback_t( PRIORITY_SPEC ), spec_( { s } )
  {
  }

  class_scoped_callback_t( std::vector<specialization_e> s )
    : scoped_callback_t( PRIORITY_SPEC ), spec_( std::move( s ) )
  {
  }

  bool valid( const special_effect_t& effect ) const override;
};

// A templated, class-scoped special effect initializer that automatically generates a single buff
// on the actor.
//
// First template argument is the actor class (e.g., shaman_t), required parameter
// Second template argument is the buff type (e.g., buff_t, stat_buff_t, ...)
template <typename T, typename T_BUFF = buff_t>
struct class_buff_cb_t : public class_scoped_callback_t
{
private:
  // Note, the dummy buff is assigned to from multiple threads, but since it is not meant to be
  // accessed, it does not really matter what buff ends up there.
  T_BUFF* __dummy;

public:
  using super = class_buff_cb_t<T, T_BUFF>;

  // The buff name. Optional if create_buff and create_fallback are both overridden. If fallback
  // behavior is required and the method is not overridden, must be provided.
  const std::string buff_name;

  class_buff_cb_t( specialization_e spec, util::string_view name = {} )
    : class_scoped_callback_t( spec ), __dummy( nullptr ), buff_name( name )
  {
  }

  class_buff_cb_t( player_e class_, util::string_view name = {} )
    : class_scoped_callback_t( class_ ), __dummy( nullptr ), buff_name( name )
  {
  }

  T* actor( const special_effect_t& e ) const
  {
    return debug_cast<T*>( e.player );
  }

  // Generic initializer for the class-scoped special effect buff creator. Override to customize
  // buff creation if a simple binary fallback yes/no switch is not enough.
  void initialize( special_effect_t& effect ) override
  {
    if ( !is_fallback( effect ) )
    {
      create_buff( effect );
    }
    else
    {
      create_fallback( effect );
    }
  }

  // Determine (from the given special_effect_t) whether to create the real buff, or the fallback
  // buff. All fallback special effects will have their source set to
  // SPECIAL_EFFECT_SOURCE_FALLBACK.
  virtual bool is_fallback( const special_effect_t& e ) const
  {
    return e.source == SPECIAL_EFFECT_SOURCE_FALLBACK;
  }

  // Create a correct type buff creator. Derived classes can override this method to fully
  // customize the buff creation.
  virtual T_BUFF* creator( const special_effect_t& e ) const
  {
    return new T_BUFF( e.player, buff_name );
  }

  // An accessor method to return the assignment pointer for the buff (in the actor). Primary use
  // is to automatically assign fallback buffs to correct member variables. If the special effect
  // requires a fallback buff, and create_fallback is not fully overridden, must be implemented.
  virtual T_BUFF*& buff_ptr( const special_effect_t& )
  {
    return __dummy;
  }

  // Create the real buff, default implementation calls creator() to create a correct buff
  // creator, that will then instantiate the buff to the assigned member variable pointer returned
  // by buff_ptr.
  virtual void create_buff( const special_effect_t& e )
  {
    buff_ptr( e ) = creator( e );
  }

  // Create a generic fallback buff. If the special effect requires a fallback buff, the developer
  // must also provide the following attributes, if the method is not overridden.
  // 1) A non-empty buff_name in the constructor
  // 2) Overridden buff_ptr method that returns a reference to a pointer where the buff should be
  //    assigned to.
  virtual void create_fallback( const special_effect_t& e )
  {
    assert( !buff_name.empty() );
    // Assert here, but note that release builds will not crash, rather the buff is assigned
    // to a dummy pointer inside this initializer object
    assert( &buff_ptr( e ) != &__dummy );

    // Proc chance is hardcoded to zero, essentially disabling the buff described by creator()
    // call.
    buff_ptr( e ) = creator( e );
    buff_ptr( e )->set_chance( 0 );
    if ( e.player->sim->debug )
    {
      e.player->sim->out_debug.printf( "Player %s created fallback buff for %s", e.player->name(), buff_name.c_str() );
    }
  }
};

// Manipulate actor somehow (using an overridden manipulate method)
template <typename T_ACTOR>
struct scoped_actor_callback_t : public class_scoped_callback_t
{
  using super = scoped_actor_callback_t<T_ACTOR>;

  scoped_actor_callback_t( player_e c ) : class_scoped_callback_t( c )
  {
  }

  scoped_actor_callback_t( specialization_e s ) : class_scoped_callback_t( s )
  {
  }

  void initialize( special_effect_t& e ) override
  {
    manipulate( debug_cast<T_ACTOR*>( e.player ), e );
  }

  // Overridable method to manipulate the actor. Must be implemented.
  virtual void manipulate( T_ACTOR* actor, const special_effect_t& e ) = 0;
};

// Manipulate the action somehow (using an overridden manipulate method)
template <typename T_ACTION = action_t>
struct scoped_action_callback_t : public class_scoped_callback_t
{
  using super = scoped_action_callback_t<T_ACTION>;

  const std::string name;
  const int spell_id;

  scoped_action_callback_t( player_e c, util::string_view n ) : class_scoped_callback_t( c ), name( n ), spell_id( -1 )
  {
  }

  scoped_action_callback_t( specialization_e s, util::string_view n )
    : class_scoped_callback_t( s ), name( n ), spell_id( -1 )
  {
  }

  scoped_action_callback_t( player_e c, unsigned sid ) : class_scoped_callback_t( c ), spell_id( sid )
  {
  }

  scoped_action_callback_t( specialization_e s, unsigned sid ) : class_scoped_callback_t( s ), spell_id( sid )
  {
  }

  // Initialize the callback by manipulating the action(s)
  void initialize( special_effect_t& e ) override
  {
    // "Snapshot" size's value so we don't attempt to manipulate any actions created during the loop.
    size_t size = e.player->action_list.size();

    for ( size_t i = 0; i < size; i++ )
    {
      action_t* a = e.player->action_list[ i ];

      if ( ( !name.empty() && util::str_compare_ci( name, a->name_str ) ) ||
           ( spell_id > 0 && spell_id == as<int>( a->id ) ) )
      {
        if ( a->sim->debug )
        {
          e.player->sim->out_debug.printf( "Player %s manipulating action %s", e.player->name(), a->name() );
        }
        manipulate( debug_cast<T_ACTION*>( a ), e );
      }
    }
  }

  // Overridable method to manipulate the action. Must be implemented.
  virtual void manipulate( T_ACTION* action, const special_effect_t& e ) = 0;
};

// Manipulate the buff somehow (using an overridden manipulate method)
template <typename T_BUFF = buff_t>
struct scoped_buff_callback_t : public class_scoped_callback_t
{
  using super = scoped_buff_callback_t<T_BUFF>;

  const std::string name;
  const int spell_id;

  scoped_buff_callback_t( player_e c, util::string_view n ) : class_scoped_callback_t( c ), name( n ), spell_id( -1 )
  {
  }

  scoped_buff_callback_t( specialization_e s, util::string_view n )
    : class_scoped_callback_t( s ), name( n ), spell_id( -1 )
  {
  }

  scoped_buff_callback_t( player_e c, unsigned sid ) : class_scoped_callback_t( c ), spell_id( sid )
  {
  }

  scoped_buff_callback_t( specialization_e s, unsigned sid ) : class_scoped_callback_t( s ), spell_id( sid )
  {
  }

  // Initialize the callback by manipulating the action(s)
  void initialize( special_effect_t& e ) override
  {
    // "Snapshot" size's value so we don't attempt to manipulate any buffs created during the loop.
    size_t size = e.player->buff_list.size();

    for ( size_t i = 0; i < size; i++ )
    {
      buff_t* b = e.player->buff_list[ i ];

      if ( ( !name.empty() && util::str_compare_ci( name, b->name_str ) ) ||
           ( spell_id > 0 && spell_id == as<int>( b->data().id() ) ) )
      {
        if ( b->sim->debug )
        {
          e.player->sim->out_debug.printf( "Player %s manipulating buff %s", e.player->name(), b->name() );
        }
        manipulate( debug_cast<T_BUFF*>( b ), e );
      }
    }
  }

  // Overridable method to manipulate the action. Must be implemented.
  virtual void manipulate( T_BUFF* buff, const special_effect_t& e ) = 0;
};

// Base template for various "proc actions".
template <typename T_ACTION>
struct proc_action_t : public T_ACTION
{
  using super         = T_ACTION;
  using base_action_t = proc_action_t<T_ACTION>;
  const special_effect_t* effect;

  void __initialize()
  {
    this->background   = true;

    // Reparse this as action_t is a bit conservative
    this->may_crit      = !this->data().flags( spell_attribute::SX_CANNOT_CRIT );

    if ( this->radius > 0 )
    {
      if ( this->data().max_targets() )
        this->aoe = this->data().max_targets();
      else
        this->aoe = -1;
    }

    // Reparse effect data for any item-dependent variables.
    for ( const auto& eff : this->data().effects() )
      this->parse_effect_data( eff );
  }

  proc_action_t( const special_effect_t& e ) : super( e.name(), e.player, e.trigger() ), effect( &e )
  {
    this->item     = e.item;
    this->cooldown = e.player->get_cooldown( e.cooldown_name() );

    __initialize();
  }

  void init() override
  {
    if ( effect )
    {
      override_data( *effect );
    }
    super::init();
  }

  proc_action_t( util::string_view token, player_t* p, const spell_data_t* s, const item_t* i = nullptr )
    : super( token, p, s ), effect( nullptr )
  {
    this->item = i;

    __initialize();
  }

  virtual void override_data( const special_effect_t& e )
  {
    bool is_periodic = e.duration_ > timespan_t::zero() && e.tick > timespan_t::zero();

    if ( ( e.override_result_es_mask & RESULT_CRIT_MASK ) )
    {
      this->may_crit = this->tick_may_crit = e.result_es_mask & RESULT_CRIT_MASK;
    }

    if ( ( e.override_result_es_mask & RESULT_MISS_MASK ) )
    {
      this->may_miss = e.result_es_mask & RESULT_MISS_MASK;
    }

    if ( e.aoe != 0 )
    {
      this->aoe = e.aoe;
    }

    if ( e.school != SCHOOL_NONE )
    {
      this->school = e.school;
    }

    if ( is_periodic )
    {
      this->base_tick_time = e.tick;
      this->dot_duration   = e.duration_;
    }

    if ( e.discharge_amount != 0 )
    {
      if ( is_periodic )
      {
        this->base_td = e.discharge_amount;
      }
      else
      {
        this->base_dd_min = this->base_dd_max = e.discharge_amount;
      }
    }

    if ( e.discharge_scaling != 0 )
    {
      if ( is_periodic )
      {
        if ( e.discharge_scaling < 0 )
        {
          this->attack_power_mod.tick = -1.0 * e.discharge_scaling;
        }
        else
        {
          this->spell_power_mod.tick = e.discharge_scaling;
        }
      }
      else
      {
        if ( e.discharge_scaling < 0 )
        {
          this->attack_power_mod.direct = -1.0 * e.discharge_scaling;
        }
        else
        {
          this->spell_power_mod.direct = e.discharge_scaling;
        }
      }
    }
  }
};

// Base proc spells used by the generic special effect initialization
struct proc_spell_t : public proc_action_t<spell_t>
{
  using super = proc_action_t<spell_t>;

  proc_spell_t( const special_effect_t& e ) : super( e )
  {
  }

  proc_spell_t( util::string_view token, player_t* p, const spell_data_t* s, const item_t* i = nullptr )
    : super( token, p, s, i )
  {
  }
};

struct proc_heal_t : public proc_action_t<heal_t>
{
  using super = proc_action_t<heal_t>;

  proc_heal_t( util::string_view token, player_t* p, const spell_data_t* s, const item_t* i = nullptr )
    : super( token, p, s, i )
  {
  }

  proc_heal_t( const special_effect_t& e ) : super( e )
  {
  }
};

struct proc_attack_t : public proc_action_t<attack_t>
{
  using super = proc_action_t<attack_t>;

  proc_attack_t( const special_effect_t& e ) : super( e )
  {
  }

  proc_attack_t( util::string_view token, player_t* p, const spell_data_t* s, const item_t* i = nullptr )
    : super( token, p, s, i )
  {
  }

  void override_data( const special_effect_t& e ) override;
};

struct proc_resource_t : public proc_action_t<spell_t>
{
  using super = proc_action_t<spell_t>;

  gain_t* gain;
  double gain_da, gain_ta;
  resource_e gain_resource;

  // Note, not called by proc_action_t
  void __initialize();

  proc_resource_t( const special_effect_t& e ) : super( e ), gain_da( 0 ), gain_ta( 0 ), gain_resource( RESOURCE_NONE )
  {
    __initialize();
  }

  proc_resource_t( util::string_view token, player_t* p, const spell_data_t* s, const item_t* item_ = nullptr )
    : super( token, p, s, item_ ), gain_da( 0 ), gain_ta( 0 ), gain_resource( RESOURCE_NONE )
  {
    __initialize();
  }

  result_e calculate_result( action_state_t* /* state */ ) const override
  {
    return RESULT_HIT;
  }

  void init() override
  {
    super::init();

    snapshot_flags = update_flags = 0;
  }

  void execute() override
  {
    super::execute();

    player->resource_gain( gain_resource, gain_da, gain );
  }

  void tick( dot_t* d ) override
  {
    super::tick( d );

    player->resource_gain( gain_resource, gain_ta, gain );
  }
};

template <typename BASE = proc_spell_t>
struct base_generic_proc_t : public BASE
{
  base_generic_proc_t( const special_effect_t& effect, ::util::string_view name, unsigned spell_id )
    : BASE( name, effect.player, effect.player->find_spell( spell_id ), effect.item )
  { }

  base_generic_proc_t( const special_effect_t& effect, ::util::string_view name, const spell_data_t* s )
    : BASE( name, effect.player, s, effect.item )
  { }
  
  base_generic_proc_t( player_t* p, ::util::string_view name, const spell_data_t* s, const item_t* i = nullptr )
    : BASE( name, p, s, i )
  { }
};

template <typename BASE = proc_spell_t>
struct base_generic_aoe_proc_t : public base_generic_proc_t<BASE>
{
  bool aoe_damage_increase;
  unsigned max_scaling_targets;

  base_generic_aoe_proc_t( const special_effect_t& effect, ::util::string_view name, unsigned spell_id,
                       bool aoe_damage_increase_ = false )
    : base_generic_proc_t<BASE>( effect, name, spell_id ), aoe_damage_increase( aoe_damage_increase_ ),
    max_scaling_targets( 5 )
  {
    this->aoe              = -1;
    this->split_aoe_damage = true;
  }

  base_generic_aoe_proc_t( const special_effect_t& effect, ::util::string_view name, const spell_data_t* s,
                       bool aoe_damage_increase_ = false )
    : base_generic_proc_t<BASE>( effect, name, s ), aoe_damage_increase( aoe_damage_increase_ ),
    max_scaling_targets( 5 )
  {
    this->aoe              = -1;
    this->split_aoe_damage = true;
  }

  base_generic_aoe_proc_t( player_t* p, ::util::string_view name, const spell_data_t* s, const item_t* i = nullptr,
                           bool aoe_damage_increase_ = false )
    : base_generic_proc_t<BASE>( p, name, s, i ), aoe_damage_increase( aoe_damage_increase_ ), max_scaling_targets( 5 )
  {
    this->aoe              = -1;
    this->split_aoe_damage = true;
  }

  double composite_aoe_multiplier( const action_state_t* state ) const override
  {
    double am = base_generic_proc_t<BASE>::composite_aoe_multiplier( state );

    // Blizzard" "generic" 15% damage increase per target hit, started appearing on items
    // from BfA onwards.
    if ( aoe_damage_increase )
    {
      // For some reason, using std::min here barfs Visual Studio 2017, so use clamp
      // instead which seems to work.
      am *= 1.0 + 0.15 * clamp( state->n_targets - 1u, 0u, max_scaling_targets );
    }

    return am;
  }
};

using generic_aoe_proc_t = base_generic_aoe_proc_t<proc_spell_t>;
using generic_proc_t     = base_generic_proc_t<proc_spell_t>;
using generic_heal_t     = base_generic_proc_t<proc_heal_t>;

template <typename CLASS, typename... ARGS>
action_t* create_proc_action( util::string_view name, const special_effect_t& effect, ARGS&&... args )
{
  auto player = effect.player;
  auto a      = player->find_action( name );

  if ( a == nullptr )
  {
    a = player->create_proc_action( name, effect );
  }

  if ( a == nullptr )
  {
    if constexpr ( std::is_constructible_v<CLASS, const special_effect_t&, ARGS...> )
    {
      a = new CLASS( effect, std::forward<ARGS>( args )... );
    }
    else if constexpr ( std::is_constructible_v<CLASS, const special_effect_t&, std::string_view, ARGS...> )
    {
      a = new CLASS( effect, name, std::forward<ARGS>( args )... );
    }
    else
    {
      static_assert( static_false<CLASS>, "Invalid constructor arguments for create_proc_action" );
    }
  }

  return a;
}

template <typename BUFF, typename... ARGS>
BUFF* create_buff( player_t* p, util::string_view name, ARGS&&... args )
{
  auto b = buff_t::find( p, name );
  if ( b != nullptr )
  {
    return debug_cast<BUFF*>( b );
  }

  return make_buff<BUFF>( p, name, std::forward<ARGS>( args )... );
}

template <typename BUFF, typename... ARGS>
BUFF* create_buff( player_t* p, const spell_data_t* s, ARGS&&... args )
{
  return create_buff<BUFF>( p, util::tokenize_fn( s->name_cstr() ), s, std::forward<ARGS>( args )... );
}

struct unique_gear_pet_t : public pet_t
{
protected:
  using base_t = unique_gear_pet_t;

public:
  bool use_auto_attack;
  const special_effect_t& effect;
  action_t* parent_action;

  unique_gear_pet_t( std::string_view name, const special_effect_t& e, const spell_data_t* summon_spell = nullptr )
    : pet_t( e.player->sim, e.player, name, true, true ), effect( e ), parent_action( nullptr )
  {
    if ( summon_spell )
      npc_id = summon_spell->effectN( 1 ).misc_value1();
    use_auto_attack = false;
  }

  resource_e primary_resource() const override
  { return RESOURCE_NONE; }

  virtual attack_t* create_auto_attack()
  { return nullptr; }

  double composite_owner_pet_damage_multiplier( const action_state_t* s ) const override
  {
    if ( owner->is_ptr() )
      return 1.0;

    return owner->composite_player_pet_damage_multiplier( s, type == PLAYER_GUARDIAN );
  }

  double composite_owner_pet_target_damage_multiplier( player_t* t ) const override
  {
    if( owner->is_ptr() )
      return 1.0;

    return owner->composite_player_target_pet_damage_multiplier( t, type == PLAYER_GUARDIAN );
  }

  struct auto_attack_t final : public melee_attack_t
  {
    auto_attack_t( unique_gear_pet_t* p ) : melee_attack_t( "main_hand", p )
    {
      assert( p->main_hand_weapon.type != WEAPON_NONE );
      p->main_hand_attack                    = p->create_auto_attack();
      p->main_hand_attack->weapon            = &( p->main_hand_weapon );
      p->main_hand_attack->base_execute_time = p->main_hand_weapon.swing_time;

      ignore_false_positive = true;
      trigger_gcd           = 0_ms;
      school                = SCHOOL_PHYSICAL;
    }

    void execute() override
    {
      player->main_hand_attack->schedule_execute();
    }

    bool ready() override
    {
      if ( player->is_moving() )
        return false;

      return ( player->main_hand_attack->execute_event == nullptr );
    }
  };

  void create_buffs() override
  {
    pet_t::create_buffs();

    buffs.movement->set_quiet( true );
  }

  void arise() override
  {
    pet_t::arise();

    if ( parent_action )
      parent_action->stats->add_execute( 0_ms, owner );

    if ( use_auto_attack && owner->base.distance > 8 )
    {
      trigger_movement( owner->base.distance, movement_direction_type::TOWARDS );
      auto dur = time_to_move();
      make_event( *sim, dur, [ this, dur ] { update_movement( dur ); } );
    }
  }

  action_t* create_action( std::string_view name, std::string_view options_str ) override
  {
    if ( name == "auto_attack" )
      return new auto_attack_t( this );

    return pet_t::create_action( name, options_str );
  }

  void init_action_list() override
  {
    action_priority_list_t* def = get_action_priority_list( "default" );
    if ( use_auto_attack )
      def->add_action( "auto_attack" );

    pet_t::init_action_list();
  }
};
} // unique_gear
