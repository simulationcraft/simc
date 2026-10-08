#include "simulationcraft.hpp"
#include "action/parse_effects.hpp"

#include "class_modules/apl/warlock.hpp"

namespace warlock
{
struct spell_rank_t
{
  unsigned spell_id;
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

constexpr std::array shadow_bolt_ranks {
  spell_rank_t{ 686 },
  spell_rank_t{ 695 },
  spell_rank_t{ 705 },
  spell_rank_t{ 1088 },
  spell_rank_t{ 1106 },
  spell_rank_t{ 7641 },
  spell_rank_t{ 11659 },
  spell_rank_t{ 11660 },
  spell_rank_t{ 11661 },
  spell_rank_t{ 25307 },
};
constexpr std::array immolate_ranks {
  spell_rank_t{ 348 },
  spell_rank_t{ 707 },
  spell_rank_t{ 1094 },
  spell_rank_t{ 2941 },
  spell_rank_t{ 11665 },
  spell_rank_t{ 11667 },
  spell_rank_t{ 11668 },
  spell_rank_t{ 25309 },
};
constexpr std::array corruption_ranks {
  spell_rank_t{ 172 },
  spell_rank_t{ 6222 },
  spell_rank_t{ 6223 },
  spell_rank_t{ 7648 },
  spell_rank_t{ 11671 },
  spell_rank_t{ 11672 },
  spell_rank_t{ 25311 },
};

constexpr std::array firebolt_ranks {
  spell_rank_t{ 3110 },   // 1
  spell_rank_t{ 7799 },   // 8
  spell_rank_t{ 7800 },   // 18
  spell_rank_t{ 7801 },   // 28
  spell_rank_t{ 7802 },   // 38
  spell_rank_t{ 11762 },  // 48
  spell_rank_t{ 11763 },  // 58
};

struct warlock_t final : public player_t
{
  warlock_t( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) :
    player_t( sim, WARLOCK, name, r ) {}

  resource_e primary_resource() const override { return RESOURCE_MANA; }

  std::string default_potion() const override   { return warlock_apl::potion( this ); }
  std::string default_flask() const override    { return warlock_apl::flask( this ); }
  std::string default_food() const override     { return warlock_apl::food( this ); }

  action_t* create_action( util::string_view name, util::string_view options_str ) override;
  void init_action_list() override;
  void init_base_stats() override;

  void create_pets() override;

  struct pets_t
  {
    pet_t* imp = nullptr;
  } pets;
};

struct warlock_module_t : public module_t
{
  warlock_module_t() : module_t( WARLOCK )
  { }

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p = new warlock_t( sim, name, r );
    return p;
  }

  bool valid() const override
  { return true; }

  void register_hotfixes() const override {}

  void register_actor_initializers( sim_t* ) const override {}
};

// Shadow Bolt

struct shadow_bolt_t : public spell_t
{
  shadow_bolt_t( warlock_t* player, util::string_view options_str ) :
    spell_t( "shadow_bolt", player, player->find_spell( max_rank( player, shadow_bolt_ranks ).spell_id ) )
  {
    parse_options( options_str );
  }
};

// Immolate

struct immolate_t : public spell_t
{
  immolate_t( warlock_t* player, util::string_view options_str ) :
    spell_t( "immolate", player, player->find_spell( max_rank( player, immolate_ranks ).spell_id ) )
  {
    parse_options( options_str );
  }
};

// Corruption

struct corruption_t : public spell_t
{
  corruption_t( warlock_t* player, util::string_view options_str ) :
    spell_t( "corruption", player, player->find_spell( max_rank( player, corruption_ranks ).spell_id ) )
  {
    parse_options( options_str );
  }
};

struct summon_imp_t final : public spell_t
{
  summon_imp_t( std::string_view name, warlock_t* player, std::string_view options_str ) :
    spell_t( name, player, player->find_spell( 688 ) )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    spell_t::execute();
    static_cast<warlock_t*>(player)->pets.imp->summon();
  }
};

///////////////////////////////////////////
//
// Demons
//
///////////////////////////////////////////

namespace pets {
  struct warlock_pet_t : public pet_t
  {
    warlock_pet_t( sim_t* sim, warlock_t* owner, std::string_view pet_name, bool guardian = false, bool dynamic = false ) :
      pet_t( sim, owner, pet_name, guardian, dynamic )
    {
    }

    const warlock_t* o() const
    { return static_cast<warlock_t*>( owner ); }

    warlock_t* o()
    { return static_cast<warlock_t*>( owner ); }
  };

  namespace imp {
    struct imp_pet_t : warlock_pet_t
    {
      imp_pet_t( sim_t* sim, warlock_t* owner ) :
        warlock_pet_t( sim, owner, "imp" )
      {
        // from level 6 imp character sheet
        owner_coeff.armor = 0.35;
        owner_coeff.ap_from_sp = 0.17;

        // TODO: Figure out the real answer, this is a placeholder
        owner_coeff.health = 1.5;

        intellect_per_owner = 0;
      }

      resource_e primary_resource() const override { return RESOURCE_MANA; }

      // copies owner's crit which matches imp char sheet
      double pet_crit() const override
      { return current_pet_stats.composite_spell_crit; }

      double resource_regen_per_second( resource_e r ) const override
      {
        if ( r != RESOURCE_MANA )
        {
          return pet_t::resource_regen_per_second( r );
        }

        // TODO - this is at level 7, probably need a table for this
        return recent_cast() ? 1.65 : 2.73;
      }

      void init_base_stats() override
      {
        warlock_pet_t::init_base_stats();

        // TODO: figure this out - probably another table of int/mana at every level
        // fit to obsevations level 3-8 but starts to break down at level 9
        double mana = 29 + (9.5 * level());
        resources.base[ RESOURCE_MANA ] = std::floor(mana);
      }

      void init_action_list() override
      {
        action_list_str = "firebolt";
        pet_t::init_action_list();
      }

      action_t* create_action( std::string_view, std::string_view ) override;
    };

    // TODO: There's some funky delay on beta. Some casts are back-to-back, some have a 0.5-1.0s gap even when
    // the imp has mana
    struct firebolt_t : spell_t
    {
      firebolt_t( std::string_view n, imp_pet_t* p, std::string_view options_str  ) :
        spell_t( n, p, p->find_spell( max_rank( p, firebolt_ranks ).spell_id ) )
      {
        parse_options( options_str );

        // beta casts have been wonky with gaps in between casts
        //ability_lag.mean   = 500_ms;
        //ability_lag.stddev = 100_ms;
      }
    };

    action_t* imp_pet_t::create_action( std::string_view name, std::string_view options_str )
    {
      if ( name == "firebolt" ) return new firebolt_t( name, this, options_str );

      return warlock_pet_t::create_action( name, options_str );
    }
  }
}

action_t* warlock_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "shadow_bolt" )
    return new shadow_bolt_t( this, options_str );
  if ( name == "immolate" )
    return new immolate_t( this, options_str );
  if ( name == "corruption" )
    return new corruption_t( this, options_str );

  if ( name == "summon_imp" )
    return new summon_imp_t( name, this, options_str );

  return player_t::create_action( name, options_str );
}

void warlock_t::create_pets()
{
  player_t::create_pets();

  pets.imp = new pets::imp::imp_pet_t( sim, this );
}

void warlock_t::init_base_stats()
{
  player_t::init_base_stats();

  base.attack_crit_chance = 0.02;
  base.spell_crit_chance = 0.01701;
}

void warlock_t::init_action_list()
{
  if ( action_list_str.empty() )
  {
    get_action_priority_list( "precombat" )->add_action( "summon_imp" );

    get_action_priority_list( "default" )->add_action( "immolate,if=!ticking|remains<2" );
    get_action_priority_list( "default" )->add_action( "corruption,if=!ticking|remains<2" );
    get_action_priority_list( "default" )->add_action( "shadow_bolt" );
  }
}

}  // namespace warlock

const module_t* module_t::warlock()
{
  static warlock::warlock_module_t m;
  return &m;
}
