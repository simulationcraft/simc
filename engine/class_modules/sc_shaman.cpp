// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

#include "class_modules/apl/shaman.hpp"

namespace {

struct spell_rank_t
{
  unsigned spell_id;
};
struct flametongue_rank_t
{
  unsigned  spell_id;   // Flametongue Weapon
  unsigned  driver_id;  // Flametongue Weapon Proc
  unsigned  attack_id;  // Flametongue Attack
};
struct rockbiter_rank_t
{
  unsigned  spell_id;    // Rockbiter Weapon
  unsigned  passive_id;  // Rockbiter Weapon (Passive)
};
struct totem_rank_t
{
  unsigned  spell_id;  // the totem
  unsigned  aura_id;   // the effect the totem provides
};


template <typename RANKS>
auto max_rank( const player_t* player, const RANKS& ranks )
{
  typename RANKS::value_type best {};
  unsigned best_level = 0;

  for ( const auto& rank : ranks )
  {
    const spell_data_t* spell = player->find_spell( rank.spell_id );

    if ( spell->ok() && spell->level() >= best_level )
    {
      best = rank;
      best_level = spell->level();
    }
  }

  return best;
}

constexpr std::array flametongue_weapon_ranks {
  //                  spell  driver   attack
  flametongue_rank_t{  8024,   8026,  29469 },
  flametongue_rank_t{  8027,   8028,  29470 },
  flametongue_rank_t{  8030,   8029,  10444 },
  flametongue_rank_t{ 16339,  10445,  10444 },
  flametongue_rank_t{ 16341,  16343,  10444 },
  flametongue_rank_t{ 16342,  16344,  10444 },
};
constexpr std::array earth_shock_ranks {
  //            spell
  spell_rank_t{  8042 },
  spell_rank_t{  8044 },
  spell_rank_t{  8045 },
  spell_rank_t{  8046 },
  spell_rank_t{ 10412 },
  spell_rank_t{ 10413 },
  spell_rank_t{ 10414 },
};
constexpr std::array flame_shock_ranks {
  //            spell
  spell_rank_t{  8050 },
  spell_rank_t{  8052 },
  spell_rank_t{  8053 },
  spell_rank_t{ 10447 },
  spell_rank_t{ 10448 },
  spell_rank_t{ 29228 },
};
constexpr std::array rockbiter_weapon_ranks {
  //                spell  passive
  rockbiter_rank_t{  8017,  10400 },
  rockbiter_rank_t{  8018,  15567 },
  rockbiter_rank_t{  8019,  15568 },
  rockbiter_rank_t{ 10399,  15569 },
  rockbiter_rank_t{ 16314,  16311 },
  rockbiter_rank_t{ 16315,  16312 },
  rockbiter_rank_t{ 16316,  16313 },
};
constexpr std::array strength_of_earth_totem_ranks {
  //            spell   aura
  totem_rank_t{  8075,   8076 },
  totem_rank_t{  8160,   8162 },
  totem_rank_t{  8161,   8163 },
  totem_rank_t{ 10442,  10441 },
  totem_rank_t{ 25361,  25362 },
};

struct shaman_t final : public player_t
{
  shaman_t( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) :
    player_t( sim, SHAMAN, name, r ) {}

  resource_e primary_resource() const override { return RESOURCE_MANA; }

  void init_base_stats() override;
  void init_spells() override;
  void create_buffs() override;
  void create_actions() override;

  std::string default_potion() const override   { return shaman_apl::potion( this ); }
  std::string default_flask() const override    { return shaman_apl::flask( this ); }
  std::string default_food() const override     { return shaman_apl::food( this ); }

  double composite_melee_attack_power() const override;
  void invalidate_cache( cache_e c ) override;

  action_t* create_action( util::string_view name, util::string_view options_str ) override;
  void init_action_list() override;

  struct buffs_t
  {
    buff_t* flametongue_weapon = nullptr;
    buff_t* rockbiter_weapon = nullptr;

    buff_t* strength_of_earth = nullptr;
  } buff;

  struct talents_t
  {
    // Enhancement
    // Row 1
    player_talent_t thundering_strikes;

    // Row 2
    player_talent_t mental_dexterity;
  } talent;

  action_t* flametongue_attack = nullptr;

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

  double thundering_strikes = talent.thundering_strikes->effectN( 1 ).percent();

  // original classic crit rates according to
  // * https://github.com/wowsims/classic/blob/master/sim/core/base_stats.go#L84
  // TODO: Verify in-game
  base.attack_crit_chance = 0.017 + thundering_strikes;
  base.spell_crit_chance = 0.023 + thundering_strikes;
}

void shaman_t::init_spells()
{
  player_t::init_spells();

  parse_all_class_passives();

  talent.thundering_strikes = find_talent_spell( talent_tree::CLASS, "Thundering Strikes" );
  talent.mental_dexterity = find_talent_spell( talent_tree::CLASS, "Mental Dexterity" );
}

double shaman_t::composite_melee_attack_power() const
{
  double ap = player_t::composite_melee_attack_power();

  ap += std::floor( talent.mental_dexterity->effectN( 1 ).percent() * cache.intellect() );

  return ap;
}

void shaman_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  if ( c == CACHE_INTELLECT && talent.mental_dexterity.ok() )
    invalidate_cache( CACHE_ATTACK_POWER );
}

// ==========================================================================
// Shaman Shock Spells
// ==========================================================================

// TODO: Shared shock abstraction?

// Earth Shock

struct earth_shock_t : public spell_t
{
  earth_shock_t( shaman_t* player, util::string_view options_str ) :
    spell_t( "earth_shock", player, player->find_spell( max_rank( player, earth_shock_ranks ).spell_id ) )
  {
    parse_options( options_str );

    cooldown           = player->get_cooldown( "shock" );
    cooldown->duration = data().cooldown();
  }
};

// Flame Shock

struct flame_shock_t : public spell_t
{
  flame_shock_t( shaman_t* player, util::string_view options_str ) :
    spell_t( "flame_shock", player, player->find_spell( max_rank( player, flame_shock_ranks ).spell_id ) )
  {
    parse_options( options_str );

    cooldown           = player->get_cooldown( "shock" );
    cooldown->duration = data().cooldown();
  }
};

// ==========================================================================
// Shaman Weapon Imbues
// ==========================================================================

struct flametongue_attack_t : public spell_t
{
  flametongue_attack_t( shaman_t* player ) :
    spell_t( "flametongue_attack", player, player->find_spell( max_rank( player, flametongue_weapon_ranks ).attack_id ) )
  {
    background = true;

    auto rank = max_rank( player, flametongue_weapon_ranks );

    double proc_value = player->find_spell( rank.driver_id )->effectN( 1 ).average( player, player->level() );
    double weapon_speed = player->main_hand_weapon.swing_time.total_seconds();

    base_dd_min = base_dd_max = proc_value * weapon_speed / 100.0;
  }
};

struct flametongue_weapon_t : public spell_t
{
  flametongue_weapon_t( shaman_t* player, util::string_view options_str ) :
    spell_t( "flametongue_weapon", player, player->find_spell( max_rank( player, flametongue_weapon_ranks ).spell_id ) )
  {
    parse_options( options_str );
    harmful = false;
    target = player;
  }

  void execute() override
  {
    spell_t::execute();

    auto p = static_cast<shaman_t*>( player );

    p->buff.rockbiter_weapon->expire();
    p->buff.flametongue_weapon->trigger();
  }

  bool ready() override
  {
    if ( static_cast<shaman_t*>( player )->buff.flametongue_weapon->check() )
      return false;

    return spell_t::ready();
  }
};

struct rockbiter_weapon_t : public spell_t
{
  rockbiter_weapon_t( shaman_t* player, util::string_view options_str ) :
    spell_t( "rockbiter_weapon", player, player->find_spell( max_rank( player, rockbiter_weapon_ranks ).spell_id ) )
  {
    parse_options( options_str );
    harmful = false;
    target = player;
  }

  void execute() override
  {
    spell_t::execute();

    auto p = static_cast<shaman_t*>( player );

    p->buff.flametongue_weapon->expire();
    p->buff.rockbiter_weapon->trigger();
  }

  bool ready() override
  {
    if ( static_cast<shaman_t*>( player )->buff.rockbiter_weapon->check() )
      return false;

    return spell_t::ready();
  }
};

// ==========================================================================
// Shaman Totems
// ==========================================================================

// TODO: Totem element exclusivity
// TODO: Make totems a unit for use with range?
// TODO: Party buff

struct strength_of_earth_totem_t : public spell_t
{
  strength_of_earth_totem_t( shaman_t* player, util::string_view options_str ) :
    spell_t( "strength_of_earth_totem", player, player->find_spell( max_rank( player, strength_of_earth_totem_ranks ).spell_id ) )
  {
    parse_options( options_str );
    harmful = false;
    target = player;
  }

  void execute() override
  {
    spell_t::execute();

    static_cast<shaman_t*>( player )->buff.strength_of_earth->trigger();
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
    may_dodge         = true;
    may_parry         = true;
    special           = false;
  }

  void impact( action_state_t* state ) override
  {
    melee_attack_t::impact( state );

    auto p = static_cast<shaman_t*>( player );

    if ( result_is_hit( state->result ) && p->buff.flametongue_weapon->check() )
      p->flametongue_attack->execute_on_target( state->target );
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
  if ( name == "flame_shock" )
    return new flame_shock_t( this, options_str );

  if ( name == "flametongue_weapon" )
    return new flametongue_weapon_t( this, options_str );
  if ( name == "rockbiter_weapon" )
    return new rockbiter_weapon_t( this, options_str );

  if ( name == "strength_of_earth_totem" )
    return new strength_of_earth_totem_t( this, options_str );

  return player_t::create_action( name, options_str );
}

void shaman_t::create_buffs()
{
  player_t::create_buffs();

  buff.flametongue_weapon = make_buff( this, "flametongue_weapon", find_spell( max_rank( this, flametongue_weapon_ranks ).spell_id ) );
  buff.rockbiter_weapon = make_buff<stat_buff_t>(
      this, "rockbiter_weapon", find_spell( max_rank( this, rockbiter_weapon_ranks ).passive_id ) );

  buff.strength_of_earth = make_buff<stat_buff_t>(
      this, "strength_of_earth", find_spell( max_rank( this, strength_of_earth_totem_ranks ).aura_id ) )
    ->set_duration( find_spell( max_rank( this, strength_of_earth_totem_ranks ).spell_id )->duration() );
}

void shaman_t::create_actions()
{
  flametongue_attack = new flametongue_attack_t( this );

  player_t::create_actions();
}

void shaman_t::init_action_list()
{
  if ( action_list_str.empty() )
  {
    get_action_priority_list( "precombat" )->add_action( "flametongue_weapon" );
    get_action_priority_list( "precombat" )->add_action( "strength_of_earth_totem" );

    get_action_priority_list( "default" )->add_action( "auto_attack" );
    get_action_priority_list( "default" )->add_action( "strength_of_earth_totem,if=!buff.strength_of_earth.up" );
    get_action_priority_list( "default" )->add_action( "flame_shock,if=!ticking|remains<1" );
    get_action_priority_list( "default" )->add_action( "earth_shock,if=dot.flame_shock.ticking|!action.flame_shock.enabled" );
  }
}

}  // namespace

const module_t* module_t::shaman()
{
  static ::shaman_module_t m;
  return &m;
}
