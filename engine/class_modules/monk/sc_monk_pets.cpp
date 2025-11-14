// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "sc_monk.hpp"

#include "simulationcraft.hpp"

namespace monk
{
namespace pets
{
monk_pet_t::monk_pet_t( monk_t *owner, std::string_view name, pet_e pet_type, bool guardian, bool dynamic )
  : pet_t( owner->sim, owner, name, pet_type, guardian, dynamic )
{
}

monk_t *monk_pet_t::o()
{
  return static_cast<monk_t *>( owner );
}

const monk_t *monk_pet_t::o() const
{
  return static_cast<monk_t *>( owner );
}

void monk_pet_t::init_assessors()
{
  base_t::init_assessors();
}

template <typename BASE, typename PET_TYPE = monk_pet_t>
struct pet_action_base_t : public BASE
{
  using super_t = BASE;
  using base_t  = pet_action_base_t<BASE>;
  bool merge_report;

  pet_action_base_t( std::string_view n, PET_TYPE *p, const spell_data_t *data = spell_data_t::nil() )
    : BASE( n, p, data ), merge_report( true )
  {
    // No costs are needed either
    super_t::base_costs[ RESOURCE_ENERGY ]          = 0;
    super_t::base_costs[ RESOURCE_CHI ]             = 0;
    super_t::base_costs[ RESOURCE_MANA ]            = 0;
    super_t::base_costs_per_tick[ RESOURCE_ENERGY ] = 0;
    super_t::base_costs_per_tick[ RESOURCE_CHI ]    = 0;
    super_t::base_costs_per_tick[ RESOURCE_MANA ]   = 0;
  }

  void init() override
  {
    if ( !this->player->sim->report_pets_separately && merge_report )
    {
      auto it =
          range::find_if( o()->pet_list, [ this ]( pet_t *pet ) { return this->player->name_str == pet->name_str; } );

      if ( it != o()->pet_list.end() && this->player != *it )
      {
        this->stats = ( *it )->get_stats( this->name(), this );
      }
    }

    super_t::init();
  }

  monk_t *o()
  {
    return p()->o();
  }

  const monk_t *o() const
  {
    return p()->o();
  }

  const PET_TYPE *p() const
  {
    return debug_cast<const PET_TYPE *>( this->player );
  }

  PET_TYPE *p()
  {
    return debug_cast<PET_TYPE *>( this->player );
  }

  void impact( action_state_t *s ) override
  {
    super_t::impact( s );

    if ( s->result_type != result_amount_type::DMG_DIRECT && s->result_type != result_amount_type::DMG_OVER_TIME )
      return;

    o()->trigger_empowered_tiger_lightning( s );

    if ( super_t::result_is_miss( s->result ) || s->result_amount <= 0.0 )
      return;
  }

  void tick( dot_t *dot ) override
  {
    super_t::tick( dot );

    if ( !super_t::result_is_miss( dot->state->result ) &&
         dot->state->result_type == result_amount_type::DMG_OVER_TIME )
      o()->trigger_empowered_tiger_lightning( dot->state );
  }
};

struct pet_melee_attack_t : public pet_action_base_t<melee_attack_t>
{
  bool trigger_mystic_touch;  // Some pets can trigger Mystic Touch debuff from attacks

  pet_melee_attack_t( std::string_view n, monk_pet_t *p, const spell_data_t *data = spell_data_t::nil() )
    : base_t( n, p, data ), trigger_mystic_touch( false )
  {
  }

  // Physical tick_action abilities need amount_type() override, so the
  // tick_action multistrikes are properly physically mitigated.
  result_amount_type amount_type( const action_state_t *state, bool periodic ) const override
  {
    if ( tick_action && tick_action->school == SCHOOL_PHYSICAL )
    {
      return result_amount_type::DMG_DIRECT;
    }
    else
    {
      return base_t::amount_type( state, periodic );
    }
  }

  void impact( action_state_t *s ) override
  {
    base_t::impact( s );

    if ( trigger_mystic_touch )
      s->target->debuffs.mystic_touch->trigger();
  }
};

struct pet_melee_t : pet_melee_attack_t
{
  pet_melee_t( std::string_view name, monk_pet_t *player, weapon_t *weapon )
    : pet_melee_attack_t( name, player, spell_data_t::nil() )
  {
    background = repeating = may_crit = may_glance = true;
    school                                         = SCHOOL_PHYSICAL;
    weapon_multiplier                              = 1.0;
    this->weapon                                   = weapon;
    // Use damage numbers from the level-scaled weapon
    base_execute_time = weapon->swing_time;
    trigger_gcd       = timespan_t::zero();
    special           = false;

    // TODO: check if there should be a dual wield hit malus here.
  }

  void execute() override
  {
    if ( time_to_execute > timespan_t::zero() && player->executing )
    {
      sim->print_debug( "Executing {} during melee ({}).", *player->executing, weapon->slot );
      schedule_execute();
    }
    else
      pet_melee_attack_t::execute();
  }
};

struct pet_auto_attack_t : public melee_attack_t
{
  bool trigger_mystic_touch;  // Some pets can trigger Mystic Touch debuff from attacks

  pet_auto_attack_t( monk_pet_t *player ) : melee_attack_t( "auto_attack", player ), trigger_mystic_touch( false )
  {
    assert( player->main_hand_weapon.type != WEAPON_NONE );
    player->main_hand_attack = nullptr;
    trigger_gcd              = 0_ms;
  }

  void init() override
  {
    melee_attack_t::init();

    assert( player->main_hand_attack && "Pet auto attack created without main hand attack" );
  }

  void execute() override
  {
    player->main_hand_attack->schedule_execute();

    if ( player->off_hand_attack )
      player->off_hand_attack->schedule_execute();
  }

  bool ready() override
  {
    if ( player->is_moving() )
      return false;
    return ( player->main_hand_attack->execute_event == nullptr );
  }

  void impact( action_state_t *s ) override
  {
    melee_attack_t::impact( s );

    if ( trigger_mystic_touch )
      s->target->debuffs.mystic_touch->trigger();
  }
};

struct pet_spell_t : public pet_action_base_t<spell_t>
{
  pet_spell_t( std::string_view n, monk_pet_t *p, const spell_data_t *data = spell_data_t::nil() )
    : base_t( n, p, data )
  {
  }
};

struct pet_heal_t : public pet_action_base_t<heal_t>
{
  pet_heal_t( std::string_view n, monk_pet_t *p, const spell_data_t *data = spell_data_t::nil() ) : base_t( n, p, data )
  {
  }
};

struct pet_absorb_t : public pet_action_base_t<absorb_t>
{
  pet_absorb_t( std::string_view n, monk_pet_t *p, const spell_data_t *data = spell_data_t::nil() )
    : base_t( n, p, data )
  {
  }
};

namespace buffs
{
template <typename buff_t>
struct monk_pet_buff_t : public buff_t
{
public:
  using base_t = monk_pet_buff_t;

  monk_pet_buff_t( monk_pet_t &p, std::string_view name, const spell_data_t *s = spell_data_t::nil(),
                   const item_t *item = nullptr )
    : buff_t( &p, name, s, item )
  {
  }

  monk_pet_t &p()
  {
    return *debug_cast<monk_pet_t *>( buff_t::source );
  }

  const monk_pet_t &p() const
  {
    return *debug_cast<monk_pet_t *>( buff_t::source );
  }

  const monk_t &o()
  {
    return p().o();
  };
};
}  // namespace buffs

struct xuen_pet_t : public monk_pet_t
{
private:
  struct melee_t : public pet_melee_t
  {
    melee_t( std::string_view n, xuen_pet_t *player, weapon_t *weapon ) : pet_melee_t( n, player, weapon )
    {
    }
  };

  struct crackling_tiger_lightning_tick_t : public pet_spell_t
  {
    crackling_tiger_lightning_tick_t( xuen_pet_t *p )
      : pet_spell_t( "crackling_tiger_lightning_tick", p,
                     p->o()->talent.conduit_of_the_celestials.crackling_tiger_lightning_driver->effectN( 1 ).trigger() )
    {
      background   = true;
      merge_report = false;
    }
  };

  struct crackling_tiger_lightning_t : public pet_spell_t
  {
    crackling_tiger_lightning_t( xuen_pet_t *p, std::string_view options_str )
      : pet_spell_t( "crackling_tiger_lightning", p,
                     p->o()->talent.conduit_of_the_celestials.crackling_tiger_lightning_driver )
    {
      parse_options( options_str );
      s_data_reporting =
          p->o()->talent.conduit_of_the_celestials.crackling_tiger_lightning_driver->effectN( 1 ).trigger();

      dot_duration = p->o()->talent.conduit_of_the_celestials.invoke_xuen_the_white_tiger->duration();
      cooldown->duration =
          p->o()->talent.conduit_of_the_celestials.invoke_xuen_the_white_tiger->duration();  // we're done when Xuen
                                                                                             // despawns

      tick_action = new crackling_tiger_lightning_tick_t( p );
    }

    double last_tick_factor( const dot_t *, timespan_t, timespan_t ) const
    {
      return 0.0;
    }
  };

  struct auto_attack_t : public pet_auto_attack_t
  {
    auto_attack_t( xuen_pet_t *player, std::string_view options_str ) : pet_auto_attack_t( player )
    {
      parse_options( options_str );

      player->main_hand_attack = new melee_t( "melee_main_hand", player, &( player->main_hand_weapon ) );
      player->main_hand_attack->base_execute_time = player->main_hand_weapon.swing_time;
    }
  };

public:
  xuen_pet_t( monk_t *owner ) : monk_pet_t( owner, "xuen_the_white_tiger", PET_XUEN, false, true )
  {
    npc_id = as<int>( o()->talent.brewmaster.invoke_niuzao_the_black_ox_npc->effectN( 1 ).misc_value1() );
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.max_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.0 );
    owner_coeff.ap_from_ap      = 1.00;
  }

  void init_action_list() override
  {
    action_list_str = "auto_attack";
    action_list_str += "/crackling_tiger_lightning";

    pet_t::init_action_list();
  }

  action_t *create_action( std::string_view name, std::string_view options_str ) override
  {
    if ( name == "crackling_tiger_lightning" )
      return new crackling_tiger_lightning_t( this, options_str );

    if ( name == "auto_attack" )
      return new auto_attack_t( this, options_str );

    return pet_t::create_action( name, options_str );
  }
};

namespace niuzao
{
struct melee_t : public pet_melee_t
{
  melee_t( niuzao_pet_t *pet, weapon_t *weapon ) : pet_melee_t( "melee_main_hand", pet, weapon )
  {
  }

  void impact( action_state_t *state ) override
  {
    pet_melee_t::impact( state );

    o()->buff.aspect_of_harmony.trigger( state );
  }
};

struct stomp_t : public pet_melee_attack_t
{
  stomp_t( niuzao_pet_t *pet )
    : pet_melee_attack_t( "stomp", pet, pet->o()->talent.brewmaster.invoke_niuzao_the_black_ox_stomp )
  {
    aoe      = -1;
    may_crit = true;
  }

  double action_multiplier() const override
  {
    double am = pet_melee_attack_t::action_multiplier();
    am *= 1.0 + o()->talent.brewmaster.walk_with_the_ox->effectN( 1 ).percent();
    return am;
  }

  void impact( action_state_t *state ) override
  {
    pet_melee_attack_t::impact( state );

    o()->buff.aspect_of_harmony.trigger( state );
  }
};

struct auto_attack_t : public pet_auto_attack_t
{
  auto_attack_t( niuzao_pet_t *pet, std::string_view options_str ) : pet_auto_attack_t( pet )
  {
    parse_options( options_str );
    player->main_hand_attack                    = new melee_t( pet, &( pet->main_hand_weapon ) );
    player->main_hand_attack->base_execute_time = player->main_hand_weapon.swing_time;
  }
};

niuzao_pet_t::niuzao_pet_t( std::string_view name, monk_t *player )
  : monk_pet_t( player, name, PET_NIUZAO, false, true ), stomp( nullptr )
{
  npc_id = as<int>( o()->talent.conduit_of_the_celestials.invoke_xuen_the_white_tiger_npc->effectN( 1 ).misc_value1() );
  main_hand_weapon.type       = WEAPON_BEAST;
  main_hand_weapon.min_dmg    = dbc->spell_scaling( o()->type, level() );
  main_hand_weapon.max_dmg    = dbc->spell_scaling( o()->type, level() );
  main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
  main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
  owner_coeff.ap_from_ap      = 1;
}

void niuzao_pet_t::init_spells()
{
  monk_pet_t::init_spells();

  stomp = new stomp_t( this );
}

void niuzao_pet_t::init_action_list()
{
  action_list_str = "auto_attack";

  monk_pet_t::init_action_list();
}

action_t *niuzao_pet_t::create_action( std::string_view name, std::string_view options_str )
{
  if ( name == "auto_attack" )
    return new auto_attack_t( this, options_str );

  return monk_pet_t::create_action( name, options_str );
}

}  // namespace niuzao

struct invoke_niuzao_pet_t : public niuzao::niuzao_pet_t
{
  invoke_niuzao_pet_t( monk_t *player ) : niuzao_pet_t( "invoke_niuzao_the_black_ox", player )
  {
  }
};
}  // end namespace pets

monk_t::pets_t::pets_t( monk_t *p )
  : xuen( "xuen_the_white_tiger", p, []( monk_t *p ) { return new pets::xuen_pet_t( p ); } ),
    niuzao( "niuzao_the_black_ox", p, []( monk_t *p ) { return new pets::invoke_niuzao_pet_t( p ); } )
{
}
}  // namespace monk
