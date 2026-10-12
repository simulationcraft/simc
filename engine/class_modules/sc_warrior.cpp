// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "action/parse_effects.hpp"
#include "class_modules/apl/warrior.hpp"

namespace
{
// ==========================================================================
// Warrior
// ==========================================================================

struct spell_rank_t
{
  unsigned spell_id;
};
struct bloodrage_rank_t
{
  unsigned spell_id;
  unsigned buff_id;
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

constexpr std::array heroic_strike_ranks {
  spell_rank_t{ 78 },
  spell_rank_t{ 284 },
  spell_rank_t{ 285 },
  spell_rank_t{ 1608 },
  spell_rank_t{ 11564 },
  spell_rank_t{ 11565 },
  spell_rank_t{ 11566 },
  spell_rank_t{ 11567 },
  spell_rank_t{ 25286 },
};

constexpr std::array rend_ranks {
  spell_rank_t{ 772 },
  spell_rank_t{ 6546 },
  spell_rank_t{ 6547 },
  spell_rank_t{ 6548 },
  spell_rank_t{ 11572 },
  spell_rank_t{ 11573 },
  spell_rank_t{ 11574 },
};

constexpr std::array battle_shout_ranks {
  spell_rank_t{ 5242 },
  spell_rank_t{ 6192 },
  spell_rank_t{ 6673 },
  spell_rank_t{ 11549 },
  spell_rank_t{ 11550 },
  spell_rank_t{ 11551 },
  spell_rank_t{ 25289 },
};

constexpr std::array bloodrage_ranks {
  bloodrage_rank_t{ 2687, 29131 },
};

constexpr std::array charge_ranks {
  spell_rank_t{ 100 },
  spell_rank_t{ 6178 },
  spell_rank_t{ 11578 },
};

constexpr std::array thunder_clap_ranks {
  spell_rank_t{ 6343 },
  spell_rank_t{ 8198 },
  spell_rank_t{ 8204 },
  spell_rank_t{ 8205 },
  spell_rank_t{ 11580 },
  spell_rank_t{ 11581 },
};

struct warrior_t : public player_t
{
  warrior_t( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) :
    player_t( sim, WARRIOR, name, r ) {}

  resource_e primary_resource() const override { return RESOURCE_RAGE; }

  void init_base_stats() override;
  void init_gains() override;
  void create_actions() override;
  void create_buffs() override;
  void init_spells() override;

  action_t* create_action( util::string_view name, util::string_view options_str ) override;
  void init_action_list() override;

  std::string default_potion() const override   { return warrior_apl::potion( this ); }
  std::string default_flask() const override    { return warrior_apl::flask( this ); }
  std::string default_food() const override     { return warrior_apl::food( this ); }

  double rage_per_swing( const weapon_t& w ) const;

  struct talents_t
  {
    // Arms
    // Row 1
    player_talent_t improved_heroic_strike;
    player_talent_t deflection;
    player_talent_t improved_rend;

  } talent;

  struct buffs_t
  {
    buff_t* battle_shout = nullptr;
    buff_t* bloodrage    = nullptr;

    buff_t* heroic_strike_queued = nullptr;
  } buff;

  struct gains_t
  {
    gain_t* melee = nullptr;
  } gain;

  action_t* heroic_strike = nullptr;
};

struct warrior_attack_t : public melee_attack_t
{
  warrior_attack_t( util::string_view name, warrior_t* p, const spell_data_t* s )
    : melee_attack_t( name, p, s ) {}

  void consume_resource() override
  {
    resource_e cr = current_resource();

    auto discountOnMiss = data().flags( spell_attribute::SX_DISCOUNT_ON_MISS );

    if (
        cr == RESOURCE_RAGE
        && cost() > 0
        && discountOnMiss
        && !proc
        && !result_is_hit( execute_state->result ) )
    {
      last_resource_cost = 0.2 * cost();

      player->resource_loss( cr, last_resource_cost, nullptr, this );
      stats->consume_resource( cr, last_resource_cost );

      sim->print_log("{} consumes {} {} for {} ({})",
          *player, last_resource_cost, cr, *this, player->resources.current[ cr ] );
    }
    else
    {
      melee_attack_t::consume_resource();
    }
  }
};

// TODO: Party buff version
struct battle_shout_t : public spell_t
{
  battle_shout_t( warrior_t* player, util::string_view options_str ) :
    spell_t( "battle_shout", player, player->find_spell( max_rank( player, battle_shout_ranks ).spell_id ) )
  {
    parse_options( options_str );

    harmful  = false;
    may_miss = false;

    target   = player;
  }

  void execute() override
  {
    spell_t::execute();

    // TODO: Legacy talent "Permanence" can extend the duration

    static_cast<warrior_t*>( player )->buff.battle_shout->trigger();
  }
};

struct bloodrage_t : public spell_t
{
  bloodrage_t( warrior_t* player, util::string_view options_str ) :
    spell_t( "bloodrage", player, player->find_spell( max_rank( player, bloodrage_ranks ).spell_id ) )
  {
    parse_options( options_str );

    harmful  = false;
    may_miss = false;

    target   = player;
  }

  void execute() override
  {
    spell_t::execute();

    auto p = static_cast<warrior_t*>( player );

    p->buff.bloodrage->trigger();
    p->resource_loss( RESOURCE_HEALTH, 100 * data().cost( POWER_HEALTH ), nullptr, this );
  }
};

/////////////////////////////////////////////////////////////////////////////////////
//
// Melee
//
/////////////////////////////////////////////////////////////////////////////////////

struct warrior_melee_t : public melee_attack_t
{
  warrior_melee_t( util::string_view name, warrior_t* player, weapon_t* w )
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

  void execute() override
  {
    auto p = static_cast<warrior_t*>( player );

    // try to trigger a queued heroic strike
    if ( p->buff.heroic_strike_queued->check() )
    {
      if ( p->resource_available( RESOURCE_RAGE, p->heroic_strike->cost() ) )
      {
        // TODO: figure out if we should track the target at queue time. I assume this hits the current target which
        // could be different.
        p->heroic_strike->execute_on_target( target );
        schedule_execute();
        return;
      }
      // didn't have enough rage, cancel the tracking queue buff
      p->buff.heroic_strike_queued->expire();
    }

    melee_attack_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    melee_attack_t::impact( s );

    auto p = static_cast<warrior_t*>( player );

    if ( result_is_hit( s->result ) )
    {
      double mult = s->result == RESULT_CRIT ? 2.0 : 1.0;
      p->resource_gain( RESOURCE_RAGE, mult * p->rage_per_swing( *weapon ), p->gain.melee, this );
    }
  }
};

struct warrior_auto_attack_t : public melee_attack_t
{
  warrior_auto_attack_t( warrior_t* p, util::string_view options_str )
    : melee_attack_t( "auto_attack", p, spell_data_t::nil() )
  {
    parse_options( options_str );
    trigger_gcd           = 0_ms;
    ignore_false_positive = true;

    assert( p->equipped_weapons[ SLOT_MAIN_HAND ].type != WEAPON_NONE );

    p->main_hand_attack = new warrior_melee_t( "melee", p, &p->equipped_weapons[ SLOT_MAIN_HAND ] );
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

double warrior_t::rage_per_swing( const weapon_t& w ) const
{
  // log observations on low level warrior
  double mult = w.group() == WEAPON_2H ? 4.5 : 3.45;
  return w.swing_time.total_seconds() * mult;
}

/////////////////////////////////////////////////////////////////////////////////////
//
// Abilities
//
/////////////////////////////////////////////////////////////////////////////////////

struct rend_t : public warrior_attack_t
{
  rend_t( warrior_t* player, util::string_view options_str ) :
    warrior_attack_t( "rend", player, player->find_spell( max_rank( player, rend_ranks ).spell_id ) )
  {
    parse_options( options_str );

    dot_behavior = DOT_CLIP;

    constexpr std::array<double, 4> improved_rend_pct {
      0,
      0.15,
      0.25,
      0.35,
    };

    base_td_multiplier *= 1.0 + improved_rend_pct[ player->talent.improved_rend.rank() ];

    // TODO: Verify this at higher levels
    attack_power_mod.tick = 0.016;
  }
};

// APL action to queue up heroic strike
// Checks to make sure warrior has enough rage to queue it up
// The actual HS execute in the melee swing will check rage again to make sure that rage
// wasn't consumed mid-swing
// TODO: Is there a better way to handle On Next Swing?
// TODO: look into generalizing this pattern for cleave, hunter (raptor strike), and druid (maul)
struct heroic_strike_queue_t : public action_t
{
  heroic_strike_queue_t( warrior_t* p, util::string_view options_str )
    : action_t( ACTION_OTHER, "heroic_strike_queue", p )
  {
    parse_options( options_str );

    trigger_gcd = 0_ms;
    harmful = false;
    ignore_false_positive = true;
  }

  bool ready() override
  {
    auto p = static_cast<warrior_t*>( player );

    // already queued
    if ( p->buff.heroic_strike_queued->check() )
      return false;

    // not enough rage
    if ( !p->resource_available( RESOURCE_RAGE, p->heroic_strike->cost() ) )
      return false;

    return action_t::ready();
  }

  void execute() override
  {
    static_cast<warrior_t*>( player )->buff.heroic_strike_queued->trigger();
  }
};

struct heroic_strike_t : public warrior_attack_t
{
  heroic_strike_t( warrior_t* player, util::string_view options_str ) :
    warrior_attack_t( "heroic_strike", player, player->find_spell( max_rank( player, heroic_strike_ranks ).spell_id ) )
  {
    parse_options( options_str );

    trigger_gcd = 0_ms;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    static_cast<warrior_t*>( player )->buff.heroic_strike_queued->expire();
  }
};

struct charge_t : public warrior_attack_t
{
  charge_t( warrior_t* player, util::string_view options_str ) :
    warrior_attack_t( "charge", player, player->find_spell( max_rank( player, charge_ranks ).spell_id ) )
  {
    parse_options( options_str );

    harmful = false;
    may_miss = false;
  }

  bool ready() override
  {
    return !player->in_combat && warrior_attack_t::ready();
  }
};

struct thunder_clap_t : public warrior_attack_t
{
  thunder_clap_t( warrior_t* player, util::string_view options_str ) :
    warrior_attack_t( "thunder_clap", player, player->find_spell( max_rank( player, thunder_clap_ranks ).spell_id ) )
  {
    parse_options( options_str );

    aoe = data().max_targets();

    // TODO: Verify this at higher levels
    attack_power_mod.direct = 0.05;

    // TODO: attack speed slow
  }
};

/////////////////////////////////////////////////////////////////////////////////////
//
// Stats, Actions, Oh my
//
/////////////////////////////////////////////////////////////////////////////////////

void warrior_t::init_base_stats()
{
  base.attack_power_per_strength = 2.0;

  player_t::init_base_stats();

  base.all_crit           = 0.0;
  base.attack_crit_chance = 0.0;
  base.spell_crit_chance  = 0.0;

  base.dodge_per_agility = base.attack_crit_per_agility;
  base.dodge = ( dbc->race_base( race ).agility + dbc->attribute_base( type, level() ).agility )
               * base.dodge_per_agility;

  base.parry_per_strength = 0.0;
  base.parry = 0.05;
  base.block = 0.05;

  base.stats.attack_power = 3.0 * level() - 20.0;
}

void warrior_t::init_gains()
{
  player_t::init_gains();

  gain.melee = get_gain( "melee" );
}

void warrior_t::create_actions()
{
  heroic_strike = new heroic_strike_t( this, "" );

  player_t::create_actions();
}

void warrior_t::create_buffs()
{
  buff.heroic_strike_queued = new buff_t( this, "heroic_strike_queued" );

  buff.battle_shout = make_buff<stat_buff_t>( this, "battle_shout",
                        find_spell( max_rank( this, battle_shout_ranks ).spell_id ) );
  buff.bloodrage = make_buff<stat_buff_t>( this, "bloodrage",
                        find_spell( max_rank( this, bloodrage_ranks ).buff_id ) )
                        ->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
                            resource_gain( RESOURCE_RAGE, 1.0, get_gain( "bloodrage" ) ); } );

  player_t::create_buffs();
}

void warrior_t::init_spells()
{
  player_t::init_spells();

  parse_all_class_passives();

  talent.improved_rend = find_talent_spell( talent_tree::CLASS, "Improved Rend" );
}

action_t* warrior_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "auto_attack" ) return new warrior_auto_attack_t( this, options_str );

  if ( name == "battle_shout" ) return new battle_shout_t( this, options_str );
  if ( name == "bloodrage" ) return new bloodrage_t( this, options_str );
  if ( name == "charge" ) return new charge_t( this, options_str );

  if ( name == "rend" ) return new rend_t( this, options_str );
  if ( name == "heroic_strike" ) return new heroic_strike_queue_t( this, options_str );


  if ( name == "thunder_clap" ) return new thunder_clap_t( this, options_str );

  return player_t::create_action( name, options_str );
}

void warrior_t::init_action_list()
{
  if ( action_list_str.empty() )
  {
    get_action_priority_list( "precombat" )->add_action( "bloodrage" );
    get_action_priority_list( "precombat" )->add_action( "battle_shout" );
    get_action_priority_list( "precombat" )->add_action( "charge" );

    get_action_priority_list( "default" )->add_action( "auto_attack" );

    get_action_priority_list( "default" )->add_action( "battle_shout,if=!buff.battle_shout.up" );
    get_action_priority_list( "default" )->add_action( "bloodrage,if=rage<80" );
    get_action_priority_list( "default" )->add_action( "rend,if=!ticking" );
    get_action_priority_list( "default" )->add_action( "thunder_clap,if=spell_targets>=2" );
    get_action_priority_list( "default" )->add_action( "heroic_strike,if=rage>=20" );
  }
}

struct warrior_module_t : public module_t
{
  warrior_module_t() : module_t( WARRIOR ) {}

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p = new warrior_t( sim, name, r );
    return p;
  }

  bool valid() const override
  {
    return true;
  }

  void register_hotfixes() const override {}

  void register_actor_initializers( sim_t* ) const override {}
};
}  // UNNAMED NAMESPACE

const module_t* module_t::warrior()
{
  static warrior_module_t m;
  return &m;
}
