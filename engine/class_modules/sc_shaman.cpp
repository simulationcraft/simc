// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

#include "class_modules/apl/shaman.hpp"

namespace {

struct shaman_t final : public player_t
{
  shaman_t( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) :
    player_t( sim, SHAMAN, name, r ) {}

  resource_e primary_resource() const override { return RESOURCE_MANA; }

  void init_base_stats() override;

  std::string default_potion() const override   { return shaman_apl::potion( this ); }
  std::string default_flask() const override    { return shaman_apl::flask( this ); }
  std::string default_food() const override     { return shaman_apl::food( this ); }

  action_t* create_action( util::string_view name, util::string_view options_str ) override;
  void init_action_list() override;
};

// SHAMAN MODULE INTERFACE ==================================================

struct shaman_module_t : public module_t
{
  shaman_module_t() : module_t( SHAMAN ) {}

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p = new shaman_t( sim, name, r );
    return p;
  }

  bool valid() const override
  {
    return true;
  }

  void register_actor_initializers( sim_t* sim ) const override
  {
    sim->register_actor_initializer( INIT_ACTOR_CREATE_BUFFS + offset(), []( player_t* p ) {
      p->buffs.bloodlust = make_buff( p, "bloodlust", p->find_spell( 2825 ) )
          ->set_cooldown( 0_ms )
          ->set_max_stack( 1 )
          ->set_default_value_from_effect_type( A_HASTE_ALL )
          ->add_invalidate( CACHE_HASTE );

      p->buffs.exhaustion = make_buff( p, "exhaustion", p->find_spell( 57723 ) )
          ->set_max_stack( 1 )
          ->set_quiet( true );
    }, "create_buffs_shaman" );
  }

  void register_hotfixes() const override {}
};

void shaman_t::init_base_stats()
{
  base.attack_power_per_strength = 2.0;

  player_t::init_base_stats();

  base.stats.attack_power = 2.0 * level() - 20.0;

  // original classic crit rates according to
  // * https://github.com/wowsims/classic/blob/master/sim/core/base_stats.go#L84
  // * https://github.com/ElliotWood/Forever/blob/master/sim/core/base_stats.go#L233
  base.attack_crit_chance = 0.017;
  base.spell_crit_chance = 0.023;
}

// ==========================================================================
// Shaman Shock Spells
// ==========================================================================

// Earth Shock

struct earth_shock_t : public spell_t
{
  earth_shock_t( shaman_t* player, util::string_view options_str ) :
    spell_t( "earth_shock", player, player->find_spell( 8045 ) )
  {
    parse_options( options_str );
  }
};

// ==========================================================================
// Shaman Attack
// ==========================================================================

// shaman_attack_t::impact ============================================

// Melee Attack =============================================================

struct shaman_melee_t : public melee_attack_t
{
  shaman_melee_t( util::string_view name, player_t* player, weapon_t* w )
    : melee_attack_t( name, player, spell_data_t::nil() )
  {
    weapon            = w;
    weapon_multiplier = 1.0;
    base_execute_time = w->swing_time;
    school            = SCHOOL_PHYSICAL;
    trigger_gcd       = 0_ms;

    background        = true;
    repeating         = true;
    may_glance        = true;
    may_crit          = true;
    special           = false;
  }
};


struct shaman_auto_attack_t : public melee_attack_t
{
  shaman_auto_attack_t( shaman_t* p, util::string_view options_str )
    : melee_attack_t( "auto_attack", p, spell_data_t::nil() )
  {
    parse_options( options_str );
    trigger_gcd           = 0_ms;
    ignore_false_positive = true;

    assert( p->main_hand_weapon.type != WEAPON_NONE );

    p->main_hand_attack = new shaman_melee_t( "melee", p, &p->main_hand_weapon );
  }

  void execute() override
  {
    player->main_hand_attack->schedule_execute();
  }

  bool ready() override
  {
    if ( player->is_moving() )
    {
      return false;
    }
    return player->main_hand_attack->execute_event == nullptr;
  }
};

action_t* shaman_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "auto_attack" )
    return new shaman_auto_attack_t( this, options_str );
  if ( name == "earth_shock" )
    return new earth_shock_t( this, options_str );

  return player_t::create_action( name, options_str );
}

void shaman_t::init_action_list()
{
  if ( action_list_str.empty() )
  {
    get_action_priority_list( "default" )->add_action( "auto_attack" );
    get_action_priority_list( "default" )->add_action( "earth_shock" );
  }
}

}  // namespace

const module_t* module_t::shaman()
{
  static ::shaman_module_t m;
  return &m;
}
