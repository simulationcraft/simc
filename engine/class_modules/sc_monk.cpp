// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
/*
  TODO:

  Add all buffs
  - Crackling Jade Lightning
  Change expel harm to heal later on.

  GENERAL:
  - Healing Elixers - something is wrong with it's implementation

  WINDWALKER:
  - Make Sure the healing for Blackout Kick is working
  - Implement Storm, Earth, and Fire

  MISTWEAVER: Pretty much everything. I have no plans of fixing mistweaver. -alex 8/26/14
  Implement the following spells:
  - Renewing Mist
  - Revival
  - Uplift
  - Life Cocoon
  - Teachings of the Monastery
  - Focus and Harmony
  * SCK healing
  * BoK's cleave effect
  - Non-glyphed Mana Tea

  BREWMASTER:
  - Fix Tier 16 2-piece to properly lower the cooldown by 30 seconds.
  - Purifying Brew - mostly implemented
  - Level 75 talents - dampen harm added - currently stacks are consumed on all attacks, not just above 15% max hp
  - Black Ox Statue

  - Zen Meditation
  - Cache stagger_pct
  */
#include "simulationcraft.hpp"

// ==========================================================================
// Monk
// ==========================================================================

namespace { // UNNAMED NAMESPACE
// Forward declarations
namespace actions {
namespace spells {
struct stagger_self_damage_t;
}
}
struct monk_t;

enum stance_e { STURDY_OX = 0x1, FIERCE_TIGER = 0x2, SPIRITED_CRANE = 0x4, WISE_SERPENT = 0x8 };

struct monk_td_t: public actor_pair_t
{
public:
  struct dots_t
  {
    dot_t* enveloping_mist;
    dot_t* renewing_mist;
    dot_t* soothing_mist;
    dot_t* zen_sphere;
  } dots;

  struct buffs_t
  {
    debuff_t* dizzying_haze;
    debuff_t* rising_sun_kick;
  } debuff;

  monk_td_t( player_t* target, monk_t* p );
};

struct monk_t: public player_t
{
private:
  stance_e _active_stance;
public:
  typedef player_t base_t;

  struct active_actions_t
  {
    action_t* blackout_kick_dot;
    action_t* blackout_kick_heal;
    action_t* chi_explosion_dot;
    actions::spells::stagger_self_damage_t* stagger_self_damage;
  } active_actions;

  double track_chi_consumption;

  struct buffs_t
  {
    absorb_buff_t* guard;
    buff_t* bladed_armor;
    buff_t* channeling_soothing_mist;
    buff_t* chi_sphere;
    buff_t* combo_breaker_bok;
    buff_t* combo_breaker_ce;
    buff_t* combo_breaker_tp;
    buff_t* dampen_harm;
    buff_t* diffuse_magic;
    buff_t* elusive_brew_activated;
    buff_t* elusive_brew_stacks;
    buff_t* energizing_brew;
    buff_t* forceful_winds;
    buff_t* fortifying_brew;
    buff_t* mana_tea;
    buff_t* power_strikes;
    buff_t* rushing_jade_wind;
    buff_t* serenity;
    buff_t* shuffle;
    buff_t* storm_earth_and_fire;
    buff_t* gift_of_the_ox;
    buff_t* tiger_power;
    buff_t* tiger_strikes;
    buff_t* tigereye_brew;
    buff_t* tigereye_brew_use;

    buff_t* zen_meditation;
  } buff;

private:
  struct stance_data_t
  {
    const spell_data_t* fierce_tiger;
    const spell_data_t* spirited_crane;
    const spell_data_t* sturdy_ox;
    const spell_data_t* wise_serpent;
  } stance_data;
public:

  struct gains_t
  {
    gain_t* chi_brew;
    gain_t* chi_refund;
    gain_t* chi_sphere;
    gain_t* combo_breaker_ce;
    gain_t* combo_breaker_savings;
    gain_t* crackling_jade_lightning;
    gain_t* energizing_brew;
    gain_t* energy_refund;
    gain_t* expel_harm;
    gain_t* jab;
    gain_t* keg_smash;
    gain_t* gift_of_the_ox;
    gain_t* mana_tea;
    gain_t* renewing_mist;
    gain_t* serenity;
    gain_t* soothing_mist;
    gain_t* spinning_crane_kick;
    gain_t* rushing_jade_wind;
    gain_t* surging_mist;
    gain_t* tier15_2pc_melee;
    gain_t* tier16_4pc_melee;
    gain_t* tier16_4pc_tank;
    gain_t* healing_elixirs;
  } gain;

  struct procs_t
  {
    proc_t* combo_breaker_bok;
    proc_t* combo_breaker_ce;
    proc_t* combo_breaker_tp;
    proc_t* mana_tea;
    proc_t* tier15_2pc_melee;
    proc_t* tier15_4pc_melee;
    proc_t* tier17_4pc_heal;
    proc_t* tigereye_brew;
    proc_t* tigereye_brew_wasted;
  } proc;

  struct talents_t
  {
    const spell_data_t* celerity;
    const spell_data_t* tigers_lust;
    const spell_data_t* momentum;

    const spell_data_t* chi_wave;
    const spell_data_t* zen_sphere;
    const spell_data_t* chi_burst;

    const spell_data_t* power_strikes;
    const spell_data_t* ascension;
    const spell_data_t* chi_brew;

    const spell_data_t* ring_of_peace;
    const spell_data_t* charging_ox_wave;
    const spell_data_t* leg_sweep;

    const spell_data_t* healing_elixirs;
    const spell_data_t* dampen_harm;
    const spell_data_t* diffuse_magic;

    const spell_data_t* rushing_jade_wind;
    const spell_data_t* invoke_xuen;
    const spell_data_t* chi_torpedo;

    const spell_data_t* breath_of_the_serpent;
    const spell_data_t* soul_dance;
    const spell_data_t* hurricane_strike;
    const spell_data_t* chi_explosion;
    const spell_data_t* path_of_mists;
    const spell_data_t* serenity;
  } talent;

  // Specialization
  struct specs_t
  {
    // GENERAL
    const spell_data_t* critical_strikes;
    const spell_data_t* leather_specialization;
    const spell_data_t* legacy_of_the_white_tiger;
    const spell_data_t* rising_sun_kick;
    const spell_data_t* way_of_the_monk;
    const spell_data_t* zen_meditaiton;
    const spell_data_t* touch_of_death;
    const spell_data_t* spinning_crane_kick;

    // Brewmaster
    const spell_data_t* bladed_armor;
    const spell_data_t* breath_of_fire;
    const spell_data_t* brewing_elusive_brew;
    const spell_data_t* brewmaster_training;
    const spell_data_t* desperate_measures;
    const spell_data_t* dizzying_haze;
    const spell_data_t* elusive_brew;
    const spell_data_t* ferment;
    const spell_data_t* gift_of_the_ox;
    const spell_data_t* guard;
    const spell_data_t* keg_smash;
    const spell_data_t* purifying_brew;
    const spell_data_t* resolve;
    const spell_data_t* summon_black_ox_statue;

    // Mistweaver
    const spell_data_t* brewing_mana_tea;
    const spell_data_t* crane_style_techniques;
    const spell_data_t* detonate_chi;
    const spell_data_t* enveloping_mist;
    const spell_data_t* focus_and_harmony; // To-do: Implement
    const spell_data_t* internal_medicine;
    const spell_data_t* jade_mists;
    const spell_data_t* legacy_of_the_emperor;
    const spell_data_t* life_cocoon;
    const spell_data_t* mana_meditation;
    const spell_data_t* mana_tea;
    const spell_data_t* mana_tea_driver;
    const spell_data_t* renewing_mist;
    const spell_data_t* revival;
    const spell_data_t* soothing_mist;
    const spell_data_t* soothing_mists_trigger;
    const spell_data_t* summon_jade_serpent_statue;
    const spell_data_t* teachings_of_the_monastery;
    const spell_data_t* thunder_focus_tea;
    const spell_data_t* uplift;

    // Windwalker
    const spell_data_t* afterlife;
    const spell_data_t* battle_trance;
    const spell_data_t* brewing_tigereye_brew;
    const spell_data_t* combat_conditioning;
    const spell_data_t* combo_breaker;
    const spell_data_t* disable;
    const spell_data_t* energizing_brew;
    const spell_data_t* fists_of_fury;
    const spell_data_t* flying_serpent_kick;
    const spell_data_t* storm_earth_and_fire;
    const spell_data_t* tigereye_brew;
    const spell_data_t* touch_of_karma;
    const spell_data_t* windflurry;
  } spec;

  // Warlords of Draenor Perks
  struct
  {
    // GENERAL
    const spell_data_t* enhanced_roll;
    const spell_data_t* enhanced_transcendence;
    // Brewmaster
    const spell_data_t* improved_breath_of_fire;
    const spell_data_t* improved_guard;
    // Mistweaver
    const spell_data_t* improved_life_cocoon;
    const spell_data_t* improved_renewing_mist;
    // Windwalker
    const spell_data_t* empowered_chi;
    const spell_data_t* empowered_spinning_crane_kick;
  } perk;

  struct mastery_spells_t
  {
    const spell_data_t* bottled_fury;        // Windwalker
    const spell_data_t* elusive_brawler;     // Brewmaster
    const spell_data_t* gift_of_the_serpent; // Mistweaver
  } mastery;

  struct glyphs_t
  {
    // General
    const spell_data_t* fortifying_brew;
    const spell_data_t* targeted_expulsion;
    const spell_data_t* touch_of_death;
    // Brewmaster
    const spell_data_t* expel_harm;
    const spell_data_t* guard;
    // Mistweaver
    const spell_data_t* mana_tea;
    // Windwalker
    const spell_data_t* blackout_kick;
    // Minor
  } glyph;

  // Cooldowns
  struct cooldowns_t
  {
    cooldowns_t* fists_of_fury;
    cooldowns_t* rising_sun_kick;
  } cooldown;

  struct passives_t
  {
    const spell_data_t* enveloping_mist;
    const spell_data_t* surging_mist;
    const spell_data_t* tier15_2pc_melee;
    const spell_data_t* healing_elixirs;
    const spell_data_t* storm_earth_and_fire;

  } passives;

  // Options
  struct options_t
  {
    int initial_chi;
  } user_options;

private:
  target_specific_t<monk_td_t*> target_data;
public:

  monk_t( sim_t* sim, const std::string& name, race_e r ):
    player_t( sim, MONK, name, r ),
    active_actions( active_actions_t() ),
    track_chi_consumption( 0.0 ),
    buff( buffs_t() ),
    stance_data( stance_data_t() ),
    gain( gains_t() ),
    proc( procs_t() ),
    talent( talents_t() ),
    spec( specs_t() ),
    mastery( mastery_spells_t() ),
    glyph( glyphs_t() ),
    passives( passives_t() ),
    user_options( options_t() )
  {
    // actives
    _active_stance = FIERCE_TIGER;

    regen_type = REGEN_DYNAMIC;
    regen_caches[CACHE_HASTE] = true;
    regen_caches[CACHE_ATTACK_HASTE] = true;
  }

  // player_t overrides
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual double    composite_armor_multiplier() const;
  virtual double    composite_melee_speed() const;
  virtual double    composite_melee_crit() const;
  virtual double    composite_spell_crit() const;
  virtual double    energy_regen_per_second() const;
  virtual double    composite_attribute_multiplier( attribute_e attr ) const override;
  virtual double    composite_player_multiplier( school_e school ) const;
  virtual double    composite_player_heal_multiplier( const action_state_t* s ) const;
  virtual double    composite_player_absorb_multiplier( const action_state_t* s ) const;
  virtual double    composite_melee_expertise( weapon_t* weapon ) const;
  virtual double    composite_melee_attack_power() const;
  virtual double    composite_parry() const;
  virtual double    composite_dodge() const;
  virtual double    composite_crit_avoidance() const;
  virtual double    composite_rating_multiplier( rating_e rating ) const;
  virtual double    composite_multistrike() const;
  virtual pet_t*    create_pet( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual void      init_spells();
  virtual void      init_base_stats();
  virtual void      init_scaling();
  virtual void      create_buffs();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      regen( timespan_t periodicity );
  virtual void      reset();
  virtual void      interrupt();
  virtual double    matching_gear_multiplier( attribute_e attr ) const;
  virtual void      create_options();
  virtual void      copy_from( player_t* );
  virtual resource_e primary_resource() const;
  virtual role_e    primary_role() const;
  virtual stat_e    convert_hybrid_stat( stat_e s ) const;
  virtual void      pre_analyze_hook();
  virtual void      combat_begin();
  virtual void      target_mitigation( school_e, dmg_e, action_state_t* );
  virtual void      assess_damage( school_e, dmg_e, action_state_t* s );
  virtual void      assess_damage_imminent_pre_absorb( school_e, dmg_e, action_state_t* s );
  virtual void      invalidate_cache( cache_e );
  virtual void      init_action_list();
  virtual expr_t*   create_expression( action_t* a, const std::string& name_str );
  virtual monk_td_t* get_target_data( player_t* target ) const
  {
    monk_td_t*& td = target_data[target];
    if ( !td )
    {
      td = new monk_td_t( target, const_cast<monk_t*>( this ) );
    }
    return td;
  }

  // Monk specific
  double current_stagger_dmg();
  double stagger_pct();
  void apl_combat_brewmaster();
  void apl_combat_mistweaver();
  void apl_combat_windwalker();
  void apl_pre_brewmaster();
  void apl_pre_mistweaver();
  void apl_pre_windwalker();

  // Stance
  stance_e current_stance() const
  {
    return _active_stance;
  }
  bool switch_to_stance( stance_e );
  void stance_invalidates( stance_e );
  const spell_data_t& static_stance_data( stance_e ) const;
  const spell_data_t& active_stance_data( stance_e ) const;

  // Custom Monk Functions
  void  clear_stagger();
  bool  has_stagger();
};

// ==========================================================================
// Monk Pets & Statues
// ==========================================================================

namespace pets {
struct statue_t: public pet_t
{
  statue_t( sim_t* sim, monk_t* owner, const std::string& n, pet_e pt, bool guardian = false ):
    pet_t( sim, owner, n, pt, guardian )
  { }

  monk_t* o()
  {
    return static_cast<monk_t*>( owner );
  }
};

struct jade_serpent_statue_t: public statue_t
{
  typedef statue_t base_t;
  jade_serpent_statue_t( sim_t* sim, monk_t* owner, const std::string& n ):
    base_t( sim, owner, n, PET_NONE, true )
  { }
};

struct xuen_pet_t: public pet_t
{
private:
  struct melee_t: public melee_attack_t
  {
    melee_t( const std::string& n, xuen_pet_t* player ):
      melee_attack_t( n, player, spell_data_t::nil() )
    {
      background = repeating = may_crit = may_glance = true;
      school = SCHOOL_NATURE;

      // Use damage numbers from the level-scaled weapon
      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      trigger_gcd = timespan_t::zero();
      special = false;
    }

    void execute()
    {
      if ( time_to_execute > timespan_t::zero() && player -> executing )
      {
        if ( sim -> debug )
          sim -> out_debug.printf( "Executing '%s' during melee (%s).", player -> executing -> name(), util::slot_type_string( weapon -> slot ) );
        schedule_execute();
      }
      else
        attack_t::execute();
    }
  };

  struct crackling_tiger_lightning_tick_t: public spell_t
  {
    crackling_tiger_lightning_tick_t( xuen_pet_t *p ): spell_t( "crackling_tiger_lightning_tick", p, p->find_spell( 123996 ) )
    {
      aoe = 3;
      dual = direct_tick = background = may_crit = may_miss = true;
    }
  };

  struct crackling_tiger_lightning_driver_t: public spell_t
  {
    crackling_tiger_lightning_driver_t( xuen_pet_t *p, const std::string& options_str ): spell_t( "crackling_tiger_lightning_driver", p, NULL )
    {
      parse_options( NULL, options_str );

      // for future compatibility, we may want to grab Xuen and our tick spell and build this data from those (Xuen summon duration, for example)
      dot_duration = timespan_t::from_seconds( 45.0 );
      hasted_ticks = may_miss = false;
      tick_zero = dynamic_tick_action = true; // trigger tick when t == 0
      base_tick_time = timespan_t::from_seconds( 1.0 ); // trigger a tick every second
      cooldown->duration = timespan_t::from_seconds( 45.0 ); // we're done after 45 seconds

      tick_action = new crackling_tiger_lightning_tick_t( p );
    }
  };

  struct crackling_tiger_lightning_t: public melee_attack_t
  {
    crackling_tiger_lightning_t( xuen_pet_t* player, const std::string& options_str ): melee_attack_t( "crackling_tiger_lightning", player, player -> find_spell( 123996 ) )
    {
      parse_options( nullptr, options_str );

      // Looks like Xuen needs a couple fixups to work properly.  Let's do that now.
      aoe = 3;
      special = tick_may_crit = true;
      cooldown -> duration = timespan_t::from_seconds( 6.0 );
    }
  };

  struct auto_attack_t: public attack_t
  {
    auto_attack_t( xuen_pet_t* player, const std::string& options_str ):
      attack_t( "auto_attack", player, spell_data_t::nil() )
    {
      parse_options( nullptr, options_str );

      player -> main_hand_attack = new melee_t( "melee_main_hand", player );
      player -> main_hand_attack -> base_execute_time = player -> main_hand_weapon.swing_time;

      trigger_gcd = timespan_t::zero();
    }

    virtual void execute()
    {
      player -> main_hand_attack -> schedule_execute();

      if ( player -> off_hand_attack )
        player -> off_hand_attack -> schedule_execute();
    }

    virtual bool ready()
    {
      if ( player -> is_moving() ) return false;

      return ( player -> main_hand_attack -> execute_event == 0 ); // not swinging
    }
  };

public:
  xuen_pet_t( sim_t* sim, monk_t* owner ):
    pet_t( sim, owner, "xuen_the_white_tiger", true )
  {
    main_hand_weapon.type = WEAPON_BEAST;
    main_hand_weapon.min_dmg = dbc.spell_scaling( o() -> type, level );
    main_hand_weapon.max_dmg = dbc.spell_scaling( o() -> type, level );
    main_hand_weapon.damage = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.0 );
    owner_coeff.ap_from_ap = 0.33;
  }

  monk_t* o()
  {
    return static_cast<monk_t*>( owner );
  }

  virtual void init_action_list()
  {
    action_list_str = "auto_attack";
    action_list_str += "/crackling_tiger_lightning";

    pet_t::init_action_list();
  }

  action_t* create_action( const std::string& name,
                           const std::string& options_str )
  {
    if ( name == "crackling_tiger_lightning" )
      return new crackling_tiger_lightning_driver_t( this, options_str );

    if ( name == "auto_attack" )
      return new auto_attack_t( this, options_str );

    return pet_t::create_action( name, options_str );
  }
};
} // end namespace pets

namespace actions {
// ==========================================================================
// Monk Abilities
// ==========================================================================

// Template for common monk action code. See priest_action_t.
template <class Base>
struct monk_action_t: public Base
{
  int stancemask;

private:
  std::array < resource_e, WISE_SERPENT + 1 > _resource_by_stance;
  typedef Base ab; // action base, eg. spell_t
public:
  typedef monk_action_t base_t;

  monk_action_t( const std::string& n, monk_t* player,
                 const spell_data_t* s = spell_data_t::nil() ):
                 ab( n, player, s ),
                 stancemask( STURDY_OX | FIERCE_TIGER | WISE_SERPENT | SPIRITED_CRANE )
  {
    ab::may_crit = true;
    range::fill( _resource_by_stance, RESOURCE_MAX );
    if ( player -> specialization() != MONK_MISTWEAVER )
    {
      ab::trigger_gcd = timespan_t::from_seconds( 1.0 );
      ab::min_gcd = timespan_t::from_seconds( 1.0 );
    }
  }
  virtual ~monk_action_t() {}

  monk_t* p()
  {
    return debug_cast<monk_t*>( ab::player );
  }
  const monk_t* p() const
  {
    return debug_cast<monk_t*>( ab::player );
  }
  monk_td_t* td( player_t* t ) const
  {
    return p() -> get_target_data( t );
  }

  virtual bool ready()
  {
    if ( !ab::ready() )
      return false;

    // Attack available in current stance?
    if ( ( stancemask & p() -> current_stance() ) == 0 )
      return false;

    return true;
  }

  virtual void init()
  {
    ab::init();

    /* Iterate through power entries, and find if there are resources linked to one of our stances
     */
    for ( size_t i = 0; ab::data()._power && i < ab::data()._power -> size(); i++ )
    {
      const spellpower_data_t* pd = ( *ab::data()._power )[i];
      switch ( pd -> aura_id() )
      {
      case 103985:
        assert( _resource_by_stance[FIERCE_TIGER] == RESOURCE_MAX && "Two power entries per aura id." );
        _resource_by_stance[FIERCE_TIGER] = pd -> resource();
        break;
      case 115069:
        assert( _resource_by_stance[STURDY_OX] == RESOURCE_MAX && "Two power entries per aura id." );
        _resource_by_stance[STURDY_OX] = pd -> resource();
        break;
      case 115070:
        assert( _resource_by_stance[WISE_SERPENT] == RESOURCE_MAX && "Two power entries per aura id." );
        _resource_by_stance[WISE_SERPENT] = pd -> resource();
        break;
      case 154436:
        assert( _resource_by_stance[SPIRITED_CRANE] == RESOURCE_MAX && "Two power entries per aura id." );
        _resource_by_stance[SPIRITED_CRANE] = pd -> resource();
        break;
      default: break;
      }
    }
  }

  virtual resource_e current_resource() const
  {
    resource_e resource_by_stance = _resource_by_stance[p() -> current_stance()];

    if ( resource_by_stance == RESOURCE_MAX )
      return ab::current_resource();

    return resource_by_stance;
  }

  virtual double composite_multistrike_multiplier( const action_state_t* s ) const
  {
    double m = ab::composite_multistrike_multiplier( s );

    if ( p() -> buff.forceful_winds -> up() )
      m *= 1 + p() -> buff.forceful_winds -> value();

    return m;
  }

  void trigger_brew( double base_stacks )
  {
    if ( p() -> spec.brewing_tigereye_brew -> ok() )
    {
      int stacks = static_cast<int>(base_stacks);
      // Look through each ability stack and test against mastery value for extra stack
      for (int x = 0; x < base_stacks; ++x){
        stacks += ( ab::rng().roll( p() -> cache.mastery_value() ) ) ? 1 : 0;
      }
      p() -> buff.tigereye_brew -> trigger( stacks );
    }
    else if ( p() -> spec.brewing_elusive_brew -> ok() )
      p() -> buff.elusive_brew_stacks -> trigger( static_cast<int>(base_stacks) );

    else if (p()->spec.brewing_mana_tea->ok())
    {
      // Manatee
      //                      _.---.._
      //     _        _.-'         ''-.
      //   .'  '-,_.-'                 '''.
      //  (       _                     o  :
      //   '._ .-'  '-._         \  \-  ---]
      //                 '-.___.-')  )..-'
      //                          (_/lame
      int stacks = static_cast<int>(base_stacks);
      for (int x = 0; x < base_stacks; ++x) {
        stacks += ( ab::rng().roll( p() -> cache.spell_crit() ) ) ? 1 : 0;
      }
      p() -> buff.mana_tea -> trigger( stacks );
    }
  }

  virtual void consume_resource()
  {
    ab::consume_resource();

    // Handle Tigereye Brew and Mana Tea
    if ( ab::result_is_hit( ab::execute_state -> result ) )
    {
      // Track Chi Consumption
      if ( current_resource() == RESOURCE_CHI )
        p() -> track_chi_consumption += ab::resource_consumed;

      if ( p() -> spec.brewing_tigereye_brew -> ok() || p() -> spec.brewing_mana_tea -> ok() )
      {
        int chi_to_consume = p() -> spec.brewing_tigereye_brew -> ok() ? p() -> spec.brewing_tigereye_brew -> effectN( 1 ).base_value() : 4;

        if ( p() -> track_chi_consumption >= chi_to_consume )
        {
          p() -> track_chi_consumption -= chi_to_consume;
          trigger_brew( 1 );
        }
      }
    }

    // Chi Savings on Dodge & Parry & Miss
    if ( current_resource() == RESOURCE_CHI && ab::resource_consumed > 0 )
    {
      double chi_restored = ab::resource_consumed;
      if ( !ab::aoe && ab::result_is_miss( ab::execute_state -> result ) )
        p() -> resource_gain( RESOURCE_CHI, chi_restored, p() -> gain.chi_refund );
      else if ( p() -> buff.serenity -> up() )
        p() -> resource_gain( RESOURCE_CHI, chi_restored, p() -> gain.serenity );
    }

    // Energy refund, estimated at 80%
    if ( current_resource() == RESOURCE_ENERGY && ab::resource_consumed > 0 && ab::result_is_miss( ab::execute_state -> result ) )
    {
      double energy_restored = ab::resource_consumed * 0.8;

      p() -> resource_gain( RESOURCE_ENERGY, energy_restored, p() -> gain.energy_refund );
    }
  }

  virtual void execute()
  {
    /* String look ups during runtime, but it's mistweaver... soo.... commented out.
    if ( p() -> buff.channeling_soothing_mist -> check()
    && p() -> executing
    && !( p() -> executing -> name_str == "enveloping_mist" || p() -> executing -> name_str == "surging_mist" ) )
    {
    for ( size_t i = 0, actors = p() -> sim -> player_non_sleeping_list.size(); i < actors; ++i )
    {
    player_t* t = p() -> sim -> player_non_sleeping_list[i];
    if ( td( t ) -> dots.soothing_mist -> is_ticking() )
    {
    td( t ) -> dots.soothing_mist -> cancel();
    p() -> buff.channeling_soothing_mist -> expire();
    break;
    }
    }
    }*/
    ab::execute();
  }
};

struct monk_spell_t: public monk_action_t < spell_t >
{
  monk_spell_t( const std::string& n, monk_t* player,
                const spell_data_t* s = spell_data_t::nil() ):
                base_t( n, player, s )
  {
  }

  virtual double composite_target_multiplier( player_t* t ) const
  {
    double m = base_t::composite_target_multiplier( t );

    if ( td( t ) -> debuff.rising_sun_kick -> check() )
      m *= 1.0 + td( t ) -> debuff.rising_sun_kick -> data().effectN( 1 ).percent();

    return m;
  }
};

struct monk_heal_t: public monk_action_t < heal_t >
{
  monk_heal_t( const std::string& n, monk_t& p,
               const spell_data_t* s = spell_data_t::nil() ):
               base_t( n, &p, s )
  {
    harmful = false;
  }
};

namespace attacks {
struct monk_melee_attack_t: public monk_action_t < melee_attack_t >
{
  weapon_t* mh;
  weapon_t* oh;

  monk_melee_attack_t( const std::string& n, monk_t* player,
                       const spell_data_t* s = spell_data_t::nil() ):
                       base_t( n, player, s ),
                       mh( NULL ), oh( NULL )
  {
    special = true;
    may_glance = false;
  }

  virtual double target_armor( player_t* t ) const
  {
    double a = base_t::target_armor( t );

    if ( p() -> buff.tiger_power -> up() )
      a *= 1.0 - p() -> buff.tiger_power -> check() * p() -> buff.tiger_power -> data().effectN( 1 ).percent();

    return a;
  }

  // Special Monk Attack Weapon damage collection, if the pointers mh or oh are set, instead of the classical action_t::weapon
  // Damage is divided instead of multiplied by the weapon speed, AP portion is not multiplied by weapon speed.
  // Both MH and OH are directly weaved into one damage number
  virtual double calculate_weapon_damage( double ap )
  {
    double total_dmg = 0;
    // Main Hand
    if ( mh && mh -> type != WEAPON_NONE && weapon_multiplier > 0 )
    {
      assert( mh -> slot != SLOT_OFF_HAND );
      double dmg = sim -> averaged_range( mh -> min_dmg, mh -> max_dmg ) + mh -> bonus_dmg;
      dmg /= mh -> swing_time.total_seconds();
      total_dmg += dmg;

      if ( sim -> debug )
      {
        sim -> out_debug.printf( "%s main hand weapon damage portion for %s: td=%.3f wd=%.3f bd=%.3f ws=%.3f ap=%.3f",
                                 player -> name(), name(), total_dmg, dmg, mh -> bonus_dmg, mh -> swing_time.total_seconds(), ap );
      }
    }

    // Off Hand
    if ( oh && oh -> type != WEAPON_NONE && weapon_multiplier > 0 )
    {
      assert( oh -> slot == SLOT_OFF_HAND );
      double dmg = sim -> averaged_range( oh -> min_dmg, oh -> max_dmg ) + oh -> bonus_dmg;
      dmg /= oh -> swing_time.total_seconds();
      // OH penalty
      dmg *= 0.5;

      total_dmg += dmg;

      if ( sim -> debug )
      {
        sim -> out_debug.printf( "%s off-hand weapon damage portion for %s: td=%.3f wd=%.3f bd=%.3f ws=%.3f ap=%.3f",
                                 player -> name(), name(), total_dmg, dmg, oh -> bonus_dmg, oh -> swing_time.total_seconds(), ap );
      }
    }

    if ( player -> dual_wield() )
      total_dmg *= 0.898882275;

    if ( !mh && !oh )
      total_dmg += base_t::calculate_weapon_damage( ap );
    else
      total_dmg += weapon_power_mod * ap;

    return total_dmg;
  }

  virtual double composite_target_multiplier( player_t* t ) const
  {
    double m = base_t::composite_target_multiplier( t );

    if ( td( t ) -> debuff.rising_sun_kick -> check() && special )
      m *= 1.0 + td( t ) -> debuff.rising_sun_kick -> data().effectN( 1 ).percent();

    return m;
  }
};

// ==========================================================================
// Jab
// ==========================================================================

struct jab_t: public monk_melee_attack_t
{
  static const spell_data_t* jab_data( monk_t* p )
  {
    return p -> find_class_spell( "Jab" );
  }

  jab_t( monk_t* p, const std::string& options_str ):
    monk_melee_attack_t( "jab", p, jab_data( p ) )
  {
    parse_options( nullptr, options_str );
    stancemask = STURDY_OX | FIERCE_TIGER | SPIRITED_CRANE;

    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );

	  base_costs[RESOURCE_ENERGY] = (p -> specialization() == MONK_WINDWALKER ? 45 : 40);

    base_multiplier *= 1.15; // hardcoded into tooltip
  }

  double combo_breaker_chance()
  {
    return p() -> spec.combo_breaker -> effectN( 1 ).percent();
  }

  virtual void execute()
  {
    monk_melee_attack_t::execute();

    if ( result_is_miss( execute_state -> result ) )
      return;

    double cb_chance = combo_breaker_chance();
    if ( p() -> talent.chi_explosion -> ok() )
    {
      if ( p() -> buff.combo_breaker_ce -> trigger( 1, buff_t::DEFAULT_VALUE(), cb_chance ) )
        p() -> proc.combo_breaker_ce -> occur();
    }
    else
    {
      if ( p() -> buff.combo_breaker_bok -> trigger( 1, buff_t::DEFAULT_VALUE(), cb_chance ) )
        p() -> proc.combo_breaker_bok -> occur();
    }
    if ( p() -> buff.combo_breaker_tp  -> trigger( 1, buff_t::DEFAULT_VALUE(), cb_chance ) )
      p() -> proc.combo_breaker_tp -> occur();

    // Chi Gain
    double chi_gain = data().effectN( 2 ).base_value();
    chi_gain += p() -> active_stance_data( FIERCE_TIGER ).effectN( 4 ).base_value();

    if ( p() -> buff.power_strikes -> up() )
    {
      if ( p() -> resources.current[RESOURCE_CHI] + chi_gain < p() -> resources.max[RESOURCE_CHI] )
        chi_gain += p() -> buff.power_strikes -> data().effectN( 1 ).base_value();
      else
        p() -> buff.chi_sphere -> trigger();

      p() -> buff.power_strikes -> expire();
    }
    player -> resource_gain( RESOURCE_CHI, chi_gain, p() -> gain.jab, this );

    if ( rng().roll( p() -> sets.set( SET_MELEE, T15, B2 ) -> proc_chance() ) )
    {
      p() -> resource_gain( RESOURCE_ENERGY, p() -> passives.tier15_2pc_melee -> effectN( 1 ).base_value(), p() -> gain.tier15_2pc_melee );
      p() -> proc.tier15_2pc_melee -> occur();
    }
  }
};

// ==========================================================================
// Tiger Palm
// ==========================================================================

struct tiger_palm_t: public monk_melee_attack_t
{
  tiger_palm_t( monk_t* p, const std::string& options_str ):
    monk_melee_attack_t( "tiger_palm", p, p -> find_class_spell( "Tiger Palm" ) )
  {
    parse_options( nullptr, options_str );
    stancemask = STURDY_OX | FIERCE_TIGER | SPIRITED_CRANE;
    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );
    base_multiplier *= 3; // hardcoded into tooltip
    if ( p -> specialization() == MONK_MISTWEAVER )
      base_multiplier *= 2;
    base_costs[RESOURCE_CHI] *= 1.0 + p -> spec.brewmaster_training -> effectN( 2 ).percent();
  }

  double action_multiplier() const
  {
    double m = monk_melee_attack_t::action_multiplier();

    m *= 1.0 + p() -> spec.teachings_of_the_monastery -> effectN( 5 ).percent();

    if ( p() -> sets.has_set_bonus( SET_MELEE, T16, B2 ) && p() -> buff.combo_breaker_tp -> check() )
      m *= 1.0 + ( p() -> sets.set( SET_MELEE, T16, B2 ) -> effectN( 1 ).base_value() / 100 );

    return m;
  }

  virtual void impact( action_state_t* s )
  {
    monk_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
      p() -> buff.tiger_power -> trigger();
  }

  virtual double cost() const
  {
    if ( p() -> buff.combo_breaker_tp -> check() )
      return 0;

    return monk_melee_attack_t::cost();
  }

  virtual void consume_resource()
  {
    if ( p() -> buff.combo_breaker_tp -> check() && result_is_hit( execute_state -> result ) )
      p() -> track_chi_consumption += base_costs[RESOURCE_CHI];

    monk_melee_attack_t::consume_resource();

    if ( p() -> buff.combo_breaker_tp -> up() )
    {
      p() -> buff.combo_breaker_tp -> expire();
      p() -> gain.combo_breaker_savings -> add( RESOURCE_CHI, cost() );
    }
  }
};

// ==========================================================================
// Blackout Kick
// ==========================================================================

struct dot_blackout_kick_t: public residual_action::residual_periodic_action_t < monk_melee_attack_t >
{
  dot_blackout_kick_t( monk_t* p ):
    base_t( "blackout_kick_dot", p, p -> find_spell( 128531 ) )
  {
    may_miss = may_crit = false;
  }
};

struct heal_blackout_kick_t : public monk_heal_t
{
  heal_blackout_kick_t( monk_t& p ) :
    monk_heal_t( "blackout_kick_heal" , p, p.find_spell( 128591 ) )
  {
    may_miss = may_crit = false;
    target = player;
  }
};

struct blackout_kick_t: public monk_melee_attack_t
{
  void trigger_blackout_kick_dot( blackout_kick_t* s, player_t* t, double dmg )
  {
    monk_t* p = s -> p();

    residual_action::trigger(
      p -> active_actions.blackout_kick_dot,
      t,
      dmg );
  }

  void trigger_blackout_kick_heal( blackout_kick_t* s, player_t* t, double heal )
  {
    monk_t* p = s -> p();

    residual_action::trigger(
      p -> active_actions.blackout_kick_heal,
      t,
      heal);
  }

  blackout_kick_t( monk_t* p, const std::string& options_str ):
    monk_melee_attack_t( "blackout_kick", p, p -> find_class_spell( "Blackout Kick" ) )
  {
    parse_options( nullptr, options_str );
    if ( p -> talent.chi_explosion -> ok() )
      background = true;
    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );
    base_multiplier *= 5.375; // hardcoded into tooltip

    if ( p -> spec.teachings_of_the_monastery -> ok() )
      aoe = 1 + p -> spec.teachings_of_the_monastery -> effectN( 4 ).base_value();
  }

  virtual void impact( action_state_t* s )
  {
    monk_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> spec.brewmaster_training -> ok() )
      {
        if ( p() -> buff.shuffle -> check() )
          p() -> buff.shuffle -> extend_duration( p(), timespan_t::from_seconds( 6.0 ) );
        else
          p() -> buff.shuffle -> trigger();
      }
    }
  }

  virtual double action_multiplier() const
  {
    double m = monk_melee_attack_t::action_multiplier();

    // check for melee 2p and CB: TP, for the 50% dmg bonus
    if ( p() -> sets.has_set_bonus( SET_MELEE, T16, B2 ) && p() -> buff.combo_breaker_bok -> check() ) {
      // damage increased by 40% for WW 2pc upon CB
      m *= 1 + ( p() -> sets.set( SET_MELEE, T16, B2 ) -> effectN( 1 ).base_value() / 100 );
    }
    return m;
  }

  virtual void assess_damage( dmg_e type, action_state_t* s )
  {
    monk_melee_attack_t::assess_damage( type, s );

    if (p()->specialization() == MONK_WINDWALKER)
    {
      if ( p() -> current.position == POSITION_BACK || p() -> glyph.blackout_kick )
        trigger_blackout_kick_dot( this, s -> target, s -> result_amount * data().effectN( 2 ).percent() );
      //else
      //  trigger_blackout_kick_heal( this, s -> target, s -> result_amount * data().effectN( 2 ).percent() );
    }
  }

  virtual double cost() const
  {
    if ( p() -> buff.combo_breaker_bok -> check() )
      return 0.0;

    return monk_melee_attack_t::cost();
  }

  virtual void consume_resource()
  {
    monk_melee_attack_t::consume_resource();

    double savings = base_costs[RESOURCE_CHI] - cost();
    if ( result_is_hit( execute_state -> result ) )
      p() -> track_chi_consumption += savings;

    if ( p() -> buff.combo_breaker_bok -> up() )
    {
      p() -> gain.combo_breaker_savings -> add( RESOURCE_CHI, savings );
      p() -> buff.combo_breaker_bok -> expire();
    }
  }
};

// ==========================================================================
// Chi Explosion State
// ==========================================================================

struct chi_state_t final: public action_state_t
{
  int chi_used;

  chi_state_t( action_t* a, player_t* t ):
    action_state_t( a, t ),
    chi_used( 0 )
  { }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    action_state_t::debug_str( s ) << " chi_used=" << chi_used; return s;
  }

  void initialize() override
  {
    action_state_t::initialize(); chi_used = 0;
  }

  void copy_state( const action_state_t* o ) override
  {
    action_state_t::copy_state( o );
    const chi_state_t* dps_t = static_cast<const chi_state_t*>( o );
    chi_used = dps_t -> chi_used;
  }
};

// ==========================================================================
// Chi Explosion
// ==========================================================================

struct dot_chi_explosion_t: public residual_action::residual_periodic_action_t < monk_melee_attack_t >
{
  dot_chi_explosion_t( monk_t* p ):
    base_t( "chi_explosion_dot", p, p -> specialization() == MONK_WINDWALKER ? p -> find_spell( 157680 ) :
    p -> specialization() == MONK_MISTWEAVER ? p -> find_spell( 157681 ) :
    p -> find_spell( 157682 ) )
  {
    dual = true;
    may_miss = false;
  }
};

struct chi_explosion_t: public monk_melee_attack_t
{
  chi_explosion_t( monk_t* p, const std::string& options_str ):
    monk_melee_attack_t( "chi_explosion", p, p -> talent.chi_explosion )
  {
    parse_options( nullptr, options_str );
  }

  void execute()
  {
    monk_melee_attack_t::execute();

    if ( p() -> specialization() == MONK_BREWMASTER )
    {
      if ( resource_consumed >= 2 )
      {
        if ( p() -> buff.shuffle -> check() )
          // hard code the 2 second shuffle 2 seconds per chi spent until an effect comes around 04/28/2014
          p() -> buff.shuffle -> extend_duration( p(), timespan_t::from_seconds( 2 + ( 2 * resource_consumed ) ) );
        else
        {
          p() -> buff.shuffle -> trigger();
          p() -> buff.shuffle -> extend_duration( p(), timespan_t::from_seconds( 2 + ( 2 * resource_consumed ) ) );
        }
      }
      if ( ( resource_consumed >= 3 ) && ( p() -> has_stagger() ) )
      {
        p() -> clear_stagger();

        if (p()->sets.has_set_bonus(MONK_BREWMASTER, T17, B4))
          trigger_brew(p()->sets.set(MONK_BREWMASTER, T17, B4)->effectN(1).base_value());
      }
    }
    else if ( p() -> specialization() == MONK_WINDWALKER )
    {
      if ( resource_consumed >= 3 )
        trigger_brew( 1 );
      if ( resource_consumed == 4 )
        aoe = -1;
    }
  }

  double action_multiplier() const
  {
    double m = monk_melee_attack_t::action_multiplier();


    // check for melee 2p and CB: TP, for the 50% dmg bonus
    if ( p() -> sets.has_set_bonus( SET_MELEE, T16, B2 ) && p() -> buff.combo_breaker_ce -> check() ) {
      // damage increased by 40% for WW 2pc upon CB
      m *= 1 + ( p() -> sets.set( SET_MELEE, T16, B2 ) -> effectN( 1 ).base_value() / 100 );
    }

    m *= 1.0 + std::min( 4.0, p() -> resources.current[RESOURCE_CHI] );

    return m;
  }

  void assess_damage( dmg_e type, action_state_t* s )
  {
    monk_melee_attack_t::assess_damage( type, s );

    if ( ( p() -> specialization() == MONK_WINDWALKER ) && ( resource_consumed >= 2 ) )
      residual_action::trigger(
      p() -> active_actions.chi_explosion_dot,
      s -> target,
      s -> result_amount * p() -> find_spell( 157680 ) -> effectN( 2 ).percent() );
  }

  double cost() const
  {
    double c = std::min( 4.0, p() -> resources.current[RESOURCE_CHI] );

    return c;
  }

  void consume_resource()
  {
    monk_melee_attack_t::consume_resource();

    if ( p() -> buff.combo_breaker_ce -> up() )
    {
      player -> resource_gain( RESOURCE_CHI, p() -> buff.combo_breaker_ce -> data().effectN( 1 ).base_value(), p() -> gain.combo_breaker_ce, this );
      p() -> buff.combo_breaker_ce -> expire();
    }
  }
};

// ==========================================================================
// Rising Sun Kick
// ==========================================================================

struct rsk_debuff_t: public monk_melee_attack_t
{
  rsk_debuff_t( monk_t* p, const spell_data_t* s ):
    monk_melee_attack_t( "rsk_debuff", p, s )
  {
    dual = true;
    aoe = -1;
    weapon_multiplier = 0;
    stancemask = FIERCE_TIGER | SPIRITED_CRANE;
  }

  virtual void impact( action_state_t* s )
  {
    monk_melee_attack_t::impact( s );

    td( s -> target ) -> debuff.rising_sun_kick -> trigger();
  }
};

struct rising_sun_kick_t: public monk_melee_attack_t
{
  rsk_debuff_t* rsk_debuff;

  rising_sun_kick_t( monk_t* p, const std::string& options_str ):
    monk_melee_attack_t( "rising_sun_kick", p, p -> spec.rising_sun_kick ),
    rsk_debuff( new rsk_debuff_t( p, p -> find_spell( 130320 ) ) )
  {
    cooldown -> duration = timespan_t::from_seconds( 8.0 );
    parse_options( nullptr, options_str );
    stancemask = FIERCE_TIGER;
    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );
    base_multiplier *= 8; // hardcoded into tooltip
  }

  virtual void impact( action_state_t* s )
  {
    monk_melee_attack_t::impact( s );

    rsk_debuff -> execute();
  }

  virtual void consume_resource()
  {
    monk_melee_attack_t::consume_resource();

    double savings = base_costs[RESOURCE_CHI] - cost();
    if ( result_is_hit( execute_state -> result ) )
      p() -> track_chi_consumption += savings;
  }
};

// ==========================================================================
// Spinning Crane Kick
// ==========================================================================
bool chi_refund = false;

struct spinning_crane_kick_t: public monk_melee_attack_t
{
  struct spinning_crane_kick_tick_t: public monk_melee_attack_t
  {
    spinning_crane_kick_tick_t( monk_t* p, const spell_data_t* s ):
      monk_melee_attack_t( "spinning_crane_kick_tick", p, s )
    {
      dual = true;
      aoe = -1;
      mh = &( player -> main_hand_weapon );
      oh = &( player -> off_hand_weapon );
      base_multiplier *= 0.75; // hardcoded into tooltip
    }

    void execute()
    {
      monk_melee_attack_t::execute();
      if ( execute_state -> n_targets >= 3 && chi_refund )
      {
        player -> resource_gain( RESOURCE_CHI, p() -> find_spell( 129881 ) -> effectN( 1 ).base_value(),
                                 p() -> gain.spinning_crane_kick, this );
        chi_refund = false; // Only let one tick refund chi.
      }
    }
  };

  struct rushing_jade_wind_tick_t: public monk_melee_attack_t
  {
    rushing_jade_wind_tick_t( monk_t* p, const spell_data_t* s ):
      monk_melee_attack_t( "rushing_jade_wind_tick", p, s )
    {
      dual = true;
      aoe = -1;
      dot_duration = timespan_t::zero();
      mh = &( player -> main_hand_weapon );
      oh = &( player -> off_hand_weapon );
      base_multiplier *= 0.6; // hardcoded into tooltip
      school = SCHOOL_PHYSICAL;
      trigger_gcd = timespan_t::zero();
      base_costs[RESOURCE_ENERGY] = 0;
      cooldown -> duration = timespan_t::zero();
    }

    void execute()
    {
      monk_melee_attack_t::execute();
      if ( execute_state -> n_targets >= 3 && chi_refund )
      {
        player -> resource_gain( RESOURCE_CHI, p() -> find_spell( 129881 ) -> effectN( 1 ).base_value(),
                                 p() -> gain.rushing_jade_wind, this );
        chi_refund = false; // Only let one tick refund chi.
      }
    }
  };

  rushing_jade_wind_tick_t* jade;
  spinning_crane_kick_tick_t* crane;
  spinning_crane_kick_t( monk_t* p, const std::string& options_str ):
    monk_melee_attack_t( p -> talent.rushing_jade_wind -> ok() ? "rushing_jade_wind" : "spinning_crane_kick",
    p,
    p -> talent.rushing_jade_wind -> ok() ? p -> talent.rushing_jade_wind : p -> spec.spinning_crane_kick ),
    jade( 0 ), crane( 0 )
  {
    parse_options( nullptr, options_str );
    stancemask = STURDY_OX | FIERCE_TIGER | WISE_SERPENT | SPIRITED_CRANE;
    may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;
    tick_zero = true;

    if ( p -> talent.rushing_jade_wind -> ok() )
    {
      hasted_ticks = false;
      channeled = false;
      update_flags &= ~STATE_HASTE;
      school = SCHOOL_NATURE; // Application is Nature but the actual damage ticks is Physical
      jade = new rushing_jade_wind_tick_t( p, p -> talent.rushing_jade_wind );
      add_child( jade );
    }
    else
    {
      hasted_ticks = true;
      school = SCHOOL_PHYSICAL;
      channeled = true;
      base_tick_time *= 1 + p -> perk.empowered_spinning_crane_kick -> effectN( 1 ).percent();
      crane = new spinning_crane_kick_tick_t( p, p -> find_spell( 107270 ) );
      add_child( crane );
    }
  }

  timespan_t dot_duration()
  {
    timespan_t dot_length = dot_duration();

    if ( !p() -> talent.rushing_jade_wind -> ok() )
      dot_length *= 1 + p() -> perk.empowered_spinning_crane_kick -> effectN( 2 ).percent();
    else
      dot_length = dot_length * p() -> cache.attack_haste();

    return dot_length;
  }

  void tick( dot_t*d )
  {
    monk_melee_attack_t::tick( d );
    if ( crane != NULL )
      crane -> execute();
    else if ( jade != NULL )
      jade -> execute();
  }

  void update_ready( timespan_t cd_duration )
  {
    if ( p() -> talent.rushing_jade_wind -> ok() )
      cd_duration = cooldown -> duration * p() -> cache.attack_haste();

    monk_melee_attack_t::update_ready( cd_duration );
  }

  void execute()
  {
    chi_refund = true;
    monk_melee_attack_t::execute();

    if ( p() -> talent.rushing_jade_wind -> ok() )
      p() -> buff.rushing_jade_wind -> trigger( 1, 0, 1.0, cooldown -> duration * p() -> cache.attack_haste() );

    if ( rng().roll( p() -> sets.set( SET_MELEE, T15, B2 ) -> proc_chance() ) )
    {
      p() -> resource_gain( RESOURCE_ENERGY, p() -> passives.tier15_2pc_melee -> effectN( 1 ).base_value(), p() -> gain.tier15_2pc_melee );
      p() -> proc.tier15_2pc_melee -> occur();
    }
  }
};

// ==========================================================================
// Fists of Fury
// ==========================================================================

struct fists_of_fury_tick_t: public monk_melee_attack_t
{
  fists_of_fury_tick_t( monk_t* p, const std::string& name ):
    monk_melee_attack_t( name, p, p -> spec.fists_of_fury )
  {
    dual = true;
    aoe = -1;
    base_multiplier *= 5.875; // hardcoded into tooltip
    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );
    base_costs[RESOURCE_CHI] = 0;
    dot_duration = timespan_t::zero();
    trigger_gcd = timespan_t::zero();
  }
};

struct fists_of_fury_t: public monk_melee_attack_t
{
  attack_t* fists_of_fury;
  fists_of_fury_t( monk_t* p, const std::string& options_str ):
    monk_melee_attack_t( "fists_of_fury", p, p -> spec.fists_of_fury ),
    fists_of_fury( new fists_of_fury_tick_t( p, "fists_of_fury_tick" ) )
  {
    parse_options( nullptr, options_str );
    stancemask = FIERCE_TIGER;
    channeled = tick_zero = true;
    may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;
    add_child( fists_of_fury );

    // T14 WW 2PC
    cooldown -> duration = data().cooldown();
    cooldown -> duration += p -> sets.set( SET_MELEE, T14, B2 ) -> effectN( 1 ).time_value();
  }

  void tick( dot_t*d )
  {
    monk_melee_attack_t::tick( d );
    fists_of_fury -> execute();
  }

  void consume_resource()
  {
    monk_melee_attack_t::consume_resource();

    double savings = base_costs[RESOURCE_CHI] - cost();
    if ( result_is_hit( execute_state -> result ) )
    {
      p() -> track_chi_consumption += savings;
      if ( p() -> sets.has_set_bonus( MONK_WINDWALKER, T17, B2 ) )
        trigger_brew( p() -> sets.set( MONK_WINDWALKER, T17, B2 ) -> effectN( 1 ).base_value() );
    }
  }
};

// ==========================================================================
// Hurricane Strike
// ==========================================================================

struct hurricane_strike_tick_t: public monk_melee_attack_t
{
  hurricane_strike_tick_t( const std::string& name, monk_t* p, const spell_data_t* s ):
    monk_melee_attack_t( name, p, s )
  {
    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );
    base_multiplier *= 2.0; // hardcoded into tooltip
    dual = true;
  }
};

struct hurricane_strike_t: public monk_melee_attack_t
{
  hurricane_strike_tick_t* hs_tick;
  hurricane_strike_t( monk_t* p, const std::string& options_str ):
    monk_melee_attack_t( "hurricane_strike", p, p -> talent.hurricane_strike ),
    hs_tick( new hurricane_strike_tick_t( "hurricane_strike_tick", p, p -> find_spell( 158221 ) ) )
  {
    parse_options( nullptr, options_str );
    stancemask = FIERCE_TIGER;
    add_child( hs_tick );
    hasted_ticks = interrupt_auto_attack = callbacks = false;
    channeled = true;
    dot_duration = data().duration();
    base_tick_time = dot_duration / 15;
  }

  void tick( dot_t*d )
  {
    monk_melee_attack_t::tick( d );
    hs_tick -> execute();
  }

  void consume_resource()
  {
    monk_melee_attack_t::consume_resource();

    double savings = base_costs[RESOURCE_CHI] - cost();
    if ( result_is_hit( execute_state -> result ) )
      p() -> track_chi_consumption += savings;
  }
};

// ==========================================================================
// Melee
// ==========================================================================

struct melee_t: public monk_melee_attack_t
{
  int sync_weapons;
  bool first;
  melee_t( const std::string& name, monk_t* player, int sw ):
    monk_melee_attack_t( name, player, spell_data_t::nil() ),
    sync_weapons( sw ), first( true )
  {
    background = repeating = may_glance = true;
    trigger_gcd = timespan_t::zero();
    special = false;
    school = SCHOOL_PHYSICAL;

    if ( player -> dual_wield() )
    {
      base_hit -= 0.19;
      base_multiplier *= 1.0 + player -> spec.way_of_the_monk -> effectN( 1 ).percent();
    }
  }

  void reset()
  {
    monk_melee_attack_t::reset();
    first = true;
  }

  virtual timespan_t execute_time() const
  {
    timespan_t t = monk_melee_attack_t::execute_time();

    if ( first )
      return ( weapon -> slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t / 2, timespan_t::zero() ) : t / 2 ) : timespan_t::zero();
    else
      return t;
  }

  void execute()
  {
    // Prevent the monk from melee'ing while channeling soothing_mist.
    // FIXME: This is super hacky and spams up the APL sample sequence a bit.
    if ( p() -> buff.channeling_soothing_mist -> check() )
      return;

    if ( first )
      first = false;

    if ( time_to_execute > timespan_t::zero() && player -> executing )
    {
      if ( sim -> debug ) sim -> out_debug.printf( "Executing '%s' during melee (%s).", player -> executing -> name(), util::slot_type_string( weapon -> slot ) );
      schedule_execute();
    }
    else
      monk_melee_attack_t::execute();
  }

  void impact( action_state_t* s )
  {
    monk_melee_attack_t::impact( s );

    p() -> buff.tiger_strikes -> trigger();

    if ( p() -> spec.brewing_elusive_brew -> ok() && s -> result == RESULT_CRIT )
    {
      // Formula taken from http://www.wowhead.com/spell=128938  2013/04/15
      if ( weapon -> group() == WEAPON_1H || weapon -> group() == WEAPON_SMALL )
        trigger_brew( 1.5 * weapon -> swing_time.total_seconds() / 2.6 );
      else
        trigger_brew( 3.0 * weapon -> swing_time.total_seconds() / 3.6 );
    }

    if (p()->spec.brewing_elusive_brew->ok() && s->result == RESULT_MULTISTRIKE)
    {
      p() -> buff.gift_of_the_ox -> trigger();
    }
  }
};

// ==========================================================================
// Auto Attack
// ==========================================================================

struct auto_attack_t: public monk_melee_attack_t
{
  int sync_weapons;
  auto_attack_t( monk_t* player, const std::string& options_str ):
    monk_melee_attack_t( "auto_attack", player, spell_data_t::nil() ),
    sync_weapons( 0 )
  {
    option_t options[] =
    {
      opt_bool( "sync_weapons", sync_weapons ),
      opt_null()
    };
    parse_options( options, options_str );

    p() -> main_hand_attack = new melee_t( "melee_main_hand", player, sync_weapons );
    p() -> main_hand_attack -> weapon = &( player -> main_hand_weapon );
    p() -> main_hand_attack -> base_execute_time = player -> main_hand_weapon.swing_time;

    if ( player -> off_hand_weapon.type != WEAPON_NONE )
    {
      if ( !player -> dual_wield() ) return;

      p() -> off_hand_attack = new melee_t( "melee_off_hand", player, sync_weapons );
      p() -> off_hand_attack -> weapon = &( player -> off_hand_weapon );
      p() -> off_hand_attack -> base_execute_time = player -> off_hand_weapon.swing_time;
      p() -> off_hand_attack -> id = 1;
    }

    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    p() -> main_hand_attack -> schedule_execute();

    if ( player -> off_hand_attack )
      p() -> off_hand_attack -> schedule_execute();
  }

  bool ready()
  {
    if ( p() -> current.distance_to_move > 5 )
      return false;

    return( p() -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// ==========================================================================
// Keg Smash
// ==========================================================================

struct keg_smash_t: public monk_melee_attack_t
{
  const spell_data_t* chi_generation;
  bool apply_dizzying_haze;

  keg_smash_t( monk_t& p, const std::string& options_str ):
    monk_melee_attack_t( "keg_smash", &p, p.spec.keg_smash ),
    chi_generation( p.find_spell( 127796 ) ),
    apply_dizzying_haze( false )
  {
    option_t options[] =
    {
      opt_bool( "dizzying_haze", apply_dizzying_haze ),
      opt_null()
    };
    parse_options( options, options_str );

    stancemask = STURDY_OX;
    aoe = -1;
    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );

    base_multiplier *= 14.5; // hardcoded into tooltip
  }

  virtual void execute()
  {
    monk_melee_attack_t::execute();

    player -> resource_gain( RESOURCE_CHI,
                             chi_generation -> effectN( 1 ).resource( RESOURCE_CHI ),
                             p() -> gain.keg_smash, this );
  }

  virtual void impact( action_state_t* s )
  {
    monk_melee_attack_t::impact( s );

    if ( apply_dizzying_haze )
    {
      if ( result_is_hit( s -> result ) )
        td( s -> target ) -> debuff.dizzying_haze -> trigger();
    }
  }
};

// ==========================================================================
// Touch of Death
// ==========================================================================

struct touch_of_death_t : public monk_melee_attack_t
{
  touch_of_death_t(monk_t* p, const std::string& options_str) :
    monk_melee_attack_t("touch_of_death", p, p -> find_class_spell("Touch of Death"))
  {
    parse_options(nullptr, options_str);
    stancemask = STURDY_OX | FIERCE_TIGER | SPIRITED_CRANE;
    if ( p -> glyph.touch_of_death -> ok() )
    {
      cooldown -> duration += p -> glyph.touch_of_death -> effectN( 1 ).time_value();
      base_costs[RESOURCE_CHI] *= 1.0 + p -> glyph.touch_of_death -> effectN( 2 ).percent();
    }
    may_crit = may_miss = may_dodge = may_parry = may_multistrike = false;
  }

  virtual void impact(action_state_t* s)
  {
    s -> result_amount = player -> resources.max[RESOURCE_HEALTH];
    monk_melee_attack_t::impact(s);
  }

  virtual void consume_resource()
  {
    monk_melee_attack_t::consume_resource();
  }

  bool ready()
  {
    if ( target -> health_percentage() > 10)
      return false;

    return monk_melee_attack_t::ready();
  }
};

// ==========================================================================
// Expel Harm
// ==========================================================================

struct expel_harm_t: public monk_melee_attack_t
{
  double result_total;
  expel_harm_t( monk_t* p ):
    monk_melee_attack_t( "expel_harm", p, p -> find_class_spell( "Expel Harm" ) )
  {
    stancemask = STURDY_OX | FIERCE_TIGER | SPIRITED_CRANE;
    background = true;

    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );

    base_multiplier = 7.5; // hardcoded into tooltip
    base_multiplier *= p -> find_spell( 115129 ) -> effectN( 2 ).percent(); // 33% of the heal is done as damage

    if ( p -> glyph.targeted_expulsion -> ok() )
      base_multiplier *= 1.0 - p -> glyph.targeted_expulsion -> effectN( 2 ).percent();
  }

  virtual double cost() const
  {
    double c = monk_melee_attack_t::cost();
    if ( player -> health_percentage() < 35 && p() -> glyph.expel_harm -> ok() )
      c += p() -> glyph.expel_harm -> effectN( 1 ).base_value();

    return c;
  }

  double trigger_attack()
  {
    execute();
    return result_total;
  }

  void impact( action_state_t* s )
  {
    monk_melee_attack_t::impact( s );

    result_total = s -> result_total;
  }
};

// ==========================================================================
// Provoke
// ==========================================================================

struct provoke_t: public monk_melee_attack_t
{
  provoke_t( monk_t* p, const std::string& options_str ):
    monk_melee_attack_t( "provoke", p, p -> find_class_spell( "Provoke" ) )
  {
    parse_options( NULL, options_str );
    use_off_gcd = true;
  }

  void impact( action_state_t* s )
  {
    if ( s -> target -> is_enemy() )
      target -> taunt( player );

    monk_melee_attack_t::impact( s );
  }
};
} // END melee_attacks NAMESPACE

namespace spells {
// ==========================================================================
// Stance
// ==========================================================================

struct stance_t: public monk_spell_t
{
  stance_e switch_to_stance;
  std::string stance_str;

  stance_t( monk_t* p, const std::string& options_str ):
    monk_spell_t( "stance", p ),
    switch_to_stance( FIERCE_TIGER ), stance_str()
  {
    option_t options[] =
    {
      opt_string( "choose", stance_str ),
      opt_null()
    };
    parse_options( options, options_str );

    try
    {
      if ( !stance_str.empty() )
      {
        if ( stance_str == "sturdy_ox" )
        {
          if ( p -> static_stance_data( STURDY_OX ).ok() )
            switch_to_stance = STURDY_OX;
          else
            throw std::string( "Attemping to use Stance of the Sturdy Ox without necessary spec." );
        }
        else if ( stance_str == "fierce_tiger" )
          switch_to_stance = FIERCE_TIGER;
        else if ( stance_str == "spirited_crane" )
          if ( p -> static_stance_data( SPIRITED_CRANE ).ok() )
            switch_to_stance = SPIRITED_CRANE;
          else
            throw std::string( "Attemping to use Stance of the Spirited Crane without necessary spec" );
        else if ( stance_str == "wise_serpent" )
          if ( p -> static_stance_data( WISE_SERPENT ).ok() )
            switch_to_stance = WISE_SERPENT;
          else
            throw std::string( "Attemping to use Stance of the Wise Serpent without necessary spec" );
        else
          throw std::string( "invalid stance " ) + stance_str + "specified";
      }
      else
        throw std::string( "no stance specified" );
    }
    catch ( const std::string& s )
    {
      sim -> errorf( "%s: %s %s, using Stance of Fierce Tiger\n",
                     p -> name(), name(), s.c_str() );
    }

    harmful = false;
    trigger_gcd = timespan_t::zero();
    cooldown -> duration = timespan_t::from_seconds( 1.0 );
  }

  void execute()
  {
    monk_spell_t::execute();

    p() -> switch_to_stance( switch_to_stance );
  }

  bool ready()
  {
    if ( p() -> current_stance() == switch_to_stance )
      return false;

    return monk_spell_t::ready();
  }
};

// ==========================================================================
// Tigereye Brew
// ==========================================================================

struct tigereye_brew_t: public monk_spell_t
{
  tigereye_brew_t( monk_t* player, const std::string& options_str ):
    monk_spell_t( "tigereye_brew", player, player -> spec.tigereye_brew )
  {
    parse_options( nullptr, options_str );
    harmful = false;
    trigger_gcd = timespan_t::zero();
  }

  double value()
  {
    double value = p() -> buff.tigereye_brew_use -> data().effectN( 1 ).percent();
    value += p() -> sets.set( SET_MELEE, T15, B4 ) -> effectN( 1 ).base_value() / 10000; // t154pc
    return value;
  }

  virtual void execute()
  {
    monk_spell_t::execute();

    int max_stacks_consumable = p() -> spec.brewing_tigereye_brew -> effectN( 2 ).base_value();
    double teb_stacks_used = std::min( p() -> buff.tigereye_brew -> stack(), max_stacks_consumable );
    // EEIN: Seperated teb_stacks_used from use_value so it can be used to track focus of xuen.
    double use_value = value() * teb_stacks_used;

    p() -> buff.tigereye_brew_use -> trigger( 1, use_value );
    p() -> buff.tigereye_brew -> decrement( max_stacks_consumable );

    if (p()->sets.has_set_bonus(MONK_WINDWALKER, T17, B4))
    {
      double fw_use_value = teb_stacks_used * p() -> buff.forceful_winds -> s_data -> effectN( 1 ).percent();
      p() -> buff.forceful_winds -> trigger( 1, fw_use_value );
    }
  }
};

// ==========================================================================
// Energizing Brew
// ==========================================================================

struct energizing_brew_t: public monk_spell_t
{
  energizing_brew_t( monk_t* player, const std::string& options_str ):
    monk_spell_t( "energizing_brew", player, player -> spec.energizing_brew )
  {
    parse_options( nullptr, options_str );

    trigger_gcd = timespan_t::zero();
    harmful = false;
    dot_duration = trigger_gcd;
  }

  virtual void execute()
  {
    monk_spell_t::execute();

    p() -> buff.energizing_brew -> trigger();
  }
};

// ==========================================================================
// Chi Brew
// ==========================================================================

struct chi_brew_t: public monk_spell_t
{
  chi_brew_t( monk_t* player, const std::string& options_str ):
    monk_spell_t( "chi_brew", player, player -> talent.chi_brew )
  {
    parse_options( nullptr, options_str );

    harmful = false;
    cooldown -> duration = timespan_t::from_seconds( 60.0 ); // Hardcoded in Cooldown
    cooldown -> charges = 2;
    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    monk_spell_t::execute();

    // Instantly restore 2 chi
    player -> resource_gain( RESOURCE_CHI, data().effectN( 1 ).base_value(), p() -> gain.chi_brew, this );

    // and generate x stacks of Brew
    if ( p() -> spec.brewing_tigereye_brew -> ok() || p() -> spec.brewing_mana_tea -> ok() )
      trigger_brew( p() -> talent.chi_brew -> effectN( 2 ).trigger() -> effectN( 1 ).base_value() );
    else if ( p() -> spec.brewing_elusive_brew -> ok() )
      trigger_brew( p() -> talent.chi_brew -> effectN( 2 ).trigger() -> effectN( 2 ).base_value() );
  }
};

// ==========================================================================
// Zen Sphere
// ==========================================================================
// Healing tick Co-efficient = 0.156

struct zen_sphere_damage_t: public monk_spell_t
{
  zen_sphere_damage_t( monk_t* player ):
    monk_spell_t( "zen_sphere_damage", player, player -> dbc.spell( 124098 ) )
  {
    background = true;

    attack_power_mod.direct = 0.069165; // fix this for the love of jebus
    school = SCHOOL_NATURE;
  }
};

struct zen_sphere_detonate_damage_t: public monk_spell_t
{
  zen_sphere_detonate_damage_t( monk_t* player ):
    monk_spell_t( "zen_sphere_damage", player, player -> find_spell( 125033 ) )
  {
    background = true;
    aoe = -1;

    attack_power_mod.direct = 0.47155; // hardcoded into tooltip
    school = SCHOOL_NATURE;
  }
};

// ==========================================================================
// Chi Wave
// ==========================================================================

struct chi_wave_heal_tick_t: public monk_heal_t
{
  chi_wave_heal_tick_t( monk_t& p, const std::string& name ):
    monk_heal_t( name, p, p.talent.chi_wave )
  {
    background = direct_tick = true;
    attack_power_mod.direct = 0.500; // Hard code 09/09/14
    target = player;
  }
};

struct chi_wave_dmg_tick_t: public monk_spell_t
{
  chi_wave_dmg_tick_t( monk_t* player, const std::string& name ):
    monk_spell_t( name, player, player -> talent.chi_wave )
  {
    background = direct_tick = true;
    attack_power_mod.direct = 0.500; // Hard code 09/09/14
  }
};

struct chi_wave_t: public monk_spell_t
{
  heal_t* heal;
  spell_t* damage;
  bool dmg;
  chi_wave_t( monk_t* player, const std::string& options_str ):
    monk_spell_t( "chi_wave", player, player -> talent.chi_wave ),
    heal( new chi_wave_heal_tick_t( *player, "chi_wave_heal" ) ),
    damage( new chi_wave_dmg_tick_t( player, "chi_wave_damage" ) ),
    dmg( true )
  {
    parse_options( nullptr, options_str );
    hasted_ticks = harmful = false;
    dot_duration = timespan_t::from_seconds( 7.0 );
    base_tick_time = timespan_t::from_seconds( 1.0 );
    add_child( heal );
    add_child( damage );
    tick_zero = true;
  }

  void impact( action_state_t* s )
  {
    dmg = true; // Set flag so that the first tick does damage
    monk_spell_t::impact( s );
  }

  void tick( dot_t* d )
  {
    monk_spell_t::tick( d );
    // Select appropriate tick action
    if ( dmg )
      damage -> execute();
    else
      heal -> execute();

    dmg = !dmg; // Invert flag for next use
  }
};

// ==========================================================================
// Chi Burst
// ==========================================================================
// Healing Coefficient = 2.03

struct chi_burst_t: public monk_spell_t
{
  chi_burst_t( monk_t* player, const std::string& options_str ):
    monk_spell_t( "chi_burst", player, player -> talent.chi_burst )
  {
    parse_options( nullptr, options_str );
    aoe = -1;
    interrupt_auto_attack = false;
    attack_power_mod.direct = 1.345; // hardcoded into tooltip
  }
};

// ==========================================================================
// Chi Torpedo
// ==========================================================================

struct chi_torpedo_t: public monk_spell_t
{
  chi_torpedo_t( monk_t* player, const std::string& options_str ):
    monk_spell_t( "chi_torpedo", player, player -> talent.chi_torpedo -> ok() ? player -> find_spell( 117993 ) : spell_data_t::not_found() )
  {
    parse_options( nullptr, options_str );
    aoe = -1;
  }
};

// ==========================================================================
// Serenity
// ==========================================================================

struct serenity_t: public monk_spell_t
{
  serenity_t( monk_t* player, const std::string& options_str ):
    monk_spell_t( "serenity", player, player -> talent.serenity )
  {
    parse_options( nullptr, options_str );
    harmful = false;
    trigger_gcd = timespan_t::zero();
  }

  void execute()
  {
    monk_spell_t::execute();

    p() -> buff.serenity -> trigger();
  }
};

struct summon_pet_t: public monk_spell_t
{
  timespan_t summoning_duration;
  pet_t* pet;

public:
  summon_pet_t( const std::string& n, const std::string& pet_name, monk_t* p, const spell_data_t* sd = spell_data_t::nil() ):
    monk_spell_t( n, p, sd ),
    summoning_duration( timespan_t::zero() ),
    pet( p -> find_pet( pet_name ) )
  {
    harmful = false;

    if ( !pet )
      sim -> errorf( "Player %s unable to find pet %s for summons.\n", player -> name(), pet_name.c_str() );
  }

  virtual void execute()
  {
    pet -> summon( summoning_duration );

    monk_spell_t::execute();
  }
};

// ==========================================================================
// Invoke Xuen, the White Tiger
// ==========================================================================

struct xuen_spell_t: public summon_pet_t
{
  xuen_spell_t( monk_t* p, const std::string& options_str ):
    summon_pet_t( "invoke_xuen", "xuen_the_white_tiger", p, p -> talent.invoke_xuen )
  {
    parse_options( nullptr, options_str );

    trigger_gcd = timespan_t::zero();
    harmful = false;
    summoning_duration = data().duration();
  }
};

// ==========================================================================
// Storm, Earth, and Fire
// =========================================================S=================

struct sef_fire_spell_t : public summon_pet_t
{
  sef_fire_spell_t( monk_t* p, const std::string& options_str ) :
    summon_pet_t( "storm_earth_and_fire", "sef_fire_spirit", p, p -> find_spell( 138123 ) )
  {
    parse_options( nullptr, options_str );

    trigger_gcd = timespan_t::zero();
    harmful = false;
  }

  virtual void execute()
  {
    pet -> summon( timespan_t::zero() );
    summon_pet_t::execute();

    p() -> buff.storm_earth_and_fire -> trigger();
  }
};

struct sef_earth_spell_t : public summon_pet_t
{
  sef_earth_spell_t( monk_t* p, const std::string& options_str ) :
    summon_pet_t( "storm_earth_and_fire", "sef_earth_spirit", p, p -> find_spell( 138121 ) )
  {
    parse_options( nullptr, options_str );

    trigger_gcd = timespan_t::zero();
    harmful = false;
  }

  virtual void execute()
  {
    pet -> summon( timespan_t::zero() );
    summon_pet_t::execute();

    p() -> buff.storm_earth_and_fire -> trigger();
  }
};

// ==========================================================================
// Chi Sphere
// ==========================================================================

struct chi_sphere_t: public monk_spell_t
{
  chi_sphere_t( monk_t* p, const std::string& options_str ):
    monk_spell_t( "chi_sphere", p, spell_data_t::nil() )
  {
    parse_options( nullptr, options_str );
    harmful = false;
    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    monk_spell_t::execute();

    if ( p() -> buff.chi_sphere -> up() )
    {
      // Only use 1 Orb per execution
      player -> resource_gain( RESOURCE_CHI, 1, p() -> gain.chi_sphere, this );

      p() -> buff.chi_sphere -> decrement();
    }
  }
};

// ==========================================================================
// Breath of Fire
// ==========================================================================

struct breath_of_fire_t: public monk_spell_t
{
  struct periodic_t: public monk_spell_t
  {
    periodic_t( monk_t& p ):
      monk_spell_t( "breath_of_fire_dot", &p, p.find_spell( 123725 ) )
    {
      background = true;
    }
  };

  periodic_t* dot_action;
  breath_of_fire_t( monk_t& p, const std::string& options_str ):
    monk_spell_t( "breath_of_fire", &p, p.spec.breath_of_fire ),
    dot_action( new periodic_t( p ) )
  {
    parse_options( nullptr, options_str );
    aoe = -1;
    stancemask = STURDY_OX;
  }

  virtual void impact( action_state_t* s )
  {
    monk_spell_t::impact( s );

    monk_td_t& td = *this -> td( s -> target );
    // Improved Breath of Fire
    if ( ( td.debuff.dizzying_haze -> up() ) || p() -> perk.improved_breath_of_fire -> ok() )
      dot_action -> execute();
  }
};

// ==========================================================================
// Dizzying Haze
// ==========================================================================

struct dizzying_haze_t: public monk_spell_t
{
  dizzying_haze_t( monk_t& p, const std::string& options_str ):
    monk_spell_t( "dizzying_haze", &p, p.spec.dizzying_haze )
  {
    parse_options( nullptr, options_str );

    aoe = -1;
    stancemask = STURDY_OX;
    ability_lag = timespan_t::from_seconds( 0.5 ); // ground target malus
  }

  virtual void impact( action_state_t* s )
  {
    monk_spell_t::impact( s );

    td( s -> target ) -> debuff.dizzying_haze -> trigger();
  }
};

// ==========================================================================
// Fortifying Brew
// ==========================================================================

struct fortifying_brew_t: public monk_spell_t
{
  fortifying_brew_t( monk_t& p, const std::string& options_str ):
    monk_spell_t( "fortifying_brew", &p, p.find_class_spell( "Fortifying Brew" ) )
  {
    parse_options( nullptr, options_str );

    harmful = may_crit = false;
    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    monk_spell_t::execute();

    p() -> buff.fortifying_brew -> trigger();

    // Extra Health is set by current max_health, doesn't change when max_health changes.
    double health_gain = player -> resources.max[RESOURCE_HEALTH];
    health_gain *= ( p() -> glyph.fortifying_brew -> ok() ? p() -> find_spell( 124997 ) -> effectN( 2 ).percent() : p() -> find_class_spell( "Fortifying Brew" ) -> effectN( 1 ).percent() );
    player -> stat_gain( STAT_MAX_HEALTH, health_gain, nullptr, this );
  }
};

// ==========================================================================
// Elusive Brew
// ==========================================================================

struct elusive_brew_t: public monk_spell_t
{
  elusive_brew_t( monk_t& p, const std::string& options_str ):
    monk_spell_t( "elusive_brew", &p, p.spec.elusive_brew )
  {
    parse_options( nullptr, options_str );

    stancemask = STURDY_OX;
    harmful = false;
  }

  virtual void execute()
  {
    monk_spell_t::execute();

    p() -> buff.elusive_brew_activated -> trigger( 1,
                                                   buff_t::DEFAULT_VALUE(),
                                                   1.0, timespan_t::from_seconds( p() -> buff.elusive_brew_stacks -> stack() ) );
    p() -> buff.elusive_brew_stacks -> expire();
  }

  virtual bool ready()
  {
    if ( !p() -> buff.elusive_brew_stacks -> check() )
      return false;

    if ( p() -> buff.elusive_brew_activated -> check() )
      return false;

    return monk_spell_t::ready();
  }
};

// ==========================================================================
// Mana Tea
// ==========================================================================
/*
  FIX ME: Current behavior resembles the glyphed ability
  regardless of if the glyph is actually present.
  */
struct mana_tea_t: public monk_spell_t
{
  mana_tea_t( monk_t& p, const std::string& options_str ):
    monk_spell_t( "mana_tea", &p, p.spec.mana_tea )
  {
    parse_options( nullptr, options_str );

    stancemask = WISE_SERPENT;
    harmful = false;
  }

  virtual bool ready()
  {
    if ( p() -> buff.mana_tea -> stack() == 0 )
      return false;

    return monk_spell_t::ready();
  }

  virtual void execute()
  {
    monk_spell_t::execute();

    int max_stacks_consumable = 2;

    double mana_gain = player -> resources.max[RESOURCE_MANA]
      * data().effectN( 1 ).percent()
      * std::min( p() -> buff.mana_tea -> stack(), max_stacks_consumable );
    player -> resource_gain( RESOURCE_MANA, mana_gain, p() -> gain.mana_tea, this );
    p() -> buff.mana_tea -> decrement( max_stacks_consumable );
  }
};

struct stagger_self_damage_t: public residual_action::residual_periodic_action_t < monk_spell_t >
{
  stagger_self_damage_t( monk_t* p ):
    base_t( "stagger_self_damage", p, p -> find_spell( 124255 ) )
  {
    dot_duration = timespan_t::from_seconds( 10.0 );
    target = p;

    callbacks = false;
  }

  virtual void init()
  {
    base_t::init();

    // We don't want this counted towards our dps
    stats -> type = STATS_NEUTRAL;
  }

  /* Clears the dot and all damage. Used by Purifying Brew
   * Returns amount purged
   */
  double clear_all_damage()
  {
    dot_t* d = get_dot();
    double damage_remaining = 0.0;
    if ( d -> is_ticking() )
      damage_remaining += d -> ticks_left() * base_td; // Assumes base_td == damage, no modifiers or crits

    cancel();
    d -> cancel();

    return damage_remaining;
  }

  bool stagger_ticking()
  {
    dot_t* d = get_dot();
    return d -> is_ticking();
  }
};

// ==========================================================================
// Purifying Brew
// ==========================================================================

struct purifying_brew_t: public monk_spell_t
{
  purifying_brew_t( monk_t& p, const std::string& options_str ):
    monk_spell_t( "purifying_brew", &p, p.spec.purifying_brew )
  {
    parse_options( nullptr, options_str );

    stancemask = STURDY_OX;
    harmful = false;
    trigger_gcd = timespan_t::zero();
  }

  void execute()
  {
    monk_spell_t::execute();

    // Tier 16 4 pieces Brewmaster: Purifying Brew also heals you for 15% of the amount of staggered damage cleared.
    if ( p() -> sets.has_set_bonus( SET_TANK, T16, B4 ) )
    {
      double stagger_heal = p() -> current_stagger_dmg() * p() -> sets.set( SET_TANK, T16, B4 ) -> effectN( 1 ).percent();
      player -> resource_gain( RESOURCE_HEALTH, stagger_heal, p() -> gain.tier16_4pc_tank, this);
    }

    // Optional addition: Track and report amount of damage cleared
    p() -> active_actions.stagger_self_damage -> clear_all_damage();

    // Tier 17 4 pieces Brewmaster: Purifying Brew generates 1 stacks of Elusive Brew.
    if ( p() -> sets.has_set_bonus( MONK_BREWMASTER, T17, B4 ) )
      trigger_brew( p() -> sets.set( MONK_BREWMASTER, T17, B4 ) -> effectN( 1 ).base_value() );
  }

  bool ready()
  {
    // Irrealistic of in-game, but let's make sure stagger is actually present
    if ( !p() -> active_actions.stagger_self_damage -> stagger_ticking() )
      return false;

    return monk_spell_t::ready();
  }
};

// ==========================================================================
// Crackling Jade Lightning
// ==========================================================================

struct crackling_jade_lightning_t: public monk_spell_t
{
  // Crackling Jade Spirit needs to bypass all mana costs for the duration
  // of the channel, if Lucidity is up when the spell is cast. Thus,
  // we need custom state to go around the channeling cost per second.
  struct cjl_state_t: public action_state_t
  {
    bool lucidity;

    cjl_state_t( crackling_jade_lightning_t* cjl, player_t* target ):
      action_state_t( cjl, target ), lucidity( false )
    { }

    std::ostringstream& debug_str( std::ostringstream& s )
    {
      action_state_t::debug_str( s ) << " lucidity=" << lucidity; return s;
    }

    void initialize()
    {
      action_state_t::initialize(); lucidity = false;
    }

    void copy_state( const action_state_t* o )
    {
      action_state_t::copy_state( o );
      const cjl_state_t* ss = debug_cast<const cjl_state_t*>( o );
      lucidity = ss -> lucidity;
    }
  };

  const spell_data_t* proc_driver;

  crackling_jade_lightning_t( monk_t& p, const std::string& options_str ):
    monk_spell_t( "crackling_jade_lightning", &p, p.find_class_spell( "Crackling Jade Lightning" ) ),
    proc_driver( p.find_spell( 123332 ) )
  {
    parse_options( nullptr, options_str );

    stancemask = STURDY_OX | WISE_SERPENT | SPIRITED_CRANE | FIERCE_TIGER;
    channeled = tick_may_crit = true;
    hasted_ticks = false; // Channeled spells always have hasted ticks. Use hasted_ticks = false to disable the increase in the number of ticks.
    procs_courageous_primal_diamond = false;
  }

  action_state_t* new_state()
  {
    return new cjl_state_t( this, target );
  }

  void snapshot_state( action_state_t* state, dmg_e rt )
  {
    monk_spell_t::snapshot_state( state, rt );
    cjl_state_t* ss = debug_cast<cjl_state_t*>( state );
    ss -> lucidity = player -> buffs.courageous_primal_diamond_lucidity -> check() != 0;
  }

  double cost_per_second( const cjl_state_t* state )
  {
    resource_e resource = current_resource();
    if ( resource == RESOURCE_MANA && state -> lucidity )
      return 0;

    return costs_per_second[resource];
  }

  void last_tick( dot_t* dot )
  {
    monk_spell_t::last_tick( dot );

    // Reset swing timer
    if ( player -> main_hand_attack )
    {
      player -> main_hand_attack -> cancel();
      player -> main_hand_attack -> schedule_execute();
    }

    if ( player -> off_hand_attack )
    {
      player -> off_hand_attack -> cancel();
      player -> off_hand_attack -> schedule_execute();
    }
  }

  void tick( dot_t* dot )
  {
    monk_spell_t::tick( dot );

    resource_e resource = current_resource();
    double resource_per_second = cost_per_second( debug_cast<const cjl_state_t*>( dot -> state ) );

    if ( player -> resources.current[resource] >= resource_per_second )
    {
      player -> resource_loss( resource, resource_per_second, 0, this );
      stats -> consume_resource( resource, resource_per_second );
      if ( sim -> log )
        sim -> out_log.printf( "%s consumes %.0f %s for %s (%.0f)", player -> name(),
        resource_per_second, util::resource_type_string( resource ),
        name(), player -> resources.current[resource] );

      if ( rng().roll( proc_driver -> proc_chance() ) )
        p() -> resource_gain( RESOURCE_CHI, proc_driver -> effectN( 1 ).trigger() -> effectN( 1 ).base_value(), p() -> gain.crackling_jade_lightning, this );
    }
    else
      dot -> cancel();
  }
};

// ==========================================================================
// Dampen Harm
// ==========================================================================

struct dampen_harm_t : public monk_spell_t
{
  dampen_harm_t( monk_t& p, const std::string& options_str ):
    monk_spell_t( "dampen_harm", &p, p.talent.dampen_harm )
      {
        parse_options( NULL, options_str );
        trigger_gcd = timespan_t::zero();
        harmful = false;
        base_dd_min = 0;
        base_dd_max = 0;
      }

  virtual void execute()
  {
    p() -> buff.dampen_harm -> trigger( 3 );
    monk_spell_t::execute();
  }
};

// ==========================================================================
// Diffuse Magic
// ==========================================================================

struct diffuse_magic_t : public monk_spell_t
{
  diffuse_magic_t( monk_t& p, const std::string& options_str ) :
    monk_spell_t( "diffuse_magic", &p, p.talent.diffuse_magic )
  {
    parse_options(NULL, options_str);
    trigger_gcd = timespan_t::zero();
    harmful = false;
    base_dd_min = 0;
    base_dd_max = 0;
  }

  virtual void execute()
  {
    p() -> buff.diffuse_magic -> trigger();
    monk_spell_t::execute();
  }
};

} // END spells NAMESPACE

namespace heals {
// ==========================================================================
// Enveloping Mist
// ==========================================================================
/*
  TODO: Verify healing values.
  */

struct enveloping_mist_t: public monk_heal_t
{
  enveloping_mist_t( monk_t& p, const std::string& options_str ):
    monk_heal_t( "enveloping_mist", p, p.find_spell( 132120 ) )
  {
    parse_options( nullptr, options_str );

    stancemask = WISE_SERPENT;
    may_crit = may_miss = false;
    resource_current = RESOURCE_CHI;
    base_costs[RESOURCE_CHI] = 3.0;
  }

  virtual timespan_t execute_time() const
  {
    if ( p() -> buff.channeling_soothing_mist -> check() )
      return timespan_t::zero();

    return monk_heal_t::execute_time();
  }
};

// ==========================================================================
// Expel Harm (Heal)
// ==========================================================================
/*
  TODO: Verify healing values.
  */

struct expel_harm_heal_t: public monk_heal_t
{
  attacks::expel_harm_t* attack;

  expel_harm_heal_t( monk_t& p, const std::string& options_str ):
    monk_heal_t( "expel_harm_heal", p, p.find_class_spell( "Expel Harm" ) )
  {
    parse_options( nullptr, options_str );

    stancemask = STURDY_OX | FIERCE_TIGER | SPIRITED_CRANE;
    if ( !p.glyph.targeted_expulsion -> ok() )
      target = &p;
    base_multiplier = 7.5;

    attack = new attacks::expel_harm_t( &p );

    if ( p.specialization() == MONK_MISTWEAVER )
      base_costs[RESOURCE_MANA] = 0;
    else
      base_costs[RESOURCE_ENERGY] = 0;
  }

  virtual void execute()
  {
    base_dd_min = base_dd_max = attack -> trigger_attack();

    monk_heal_t::execute();

    // Chi Gain
    double chi_gain = data().effectN( 2 ).base_value();
    chi_gain += p() -> active_stance_data( FIERCE_TIGER ).effectN( 4 ).base_value();

    player -> resource_gain( RESOURCE_CHI, chi_gain, p() -> gain.expel_harm, this );

    if ( rng().roll( p() -> sets.set( SET_MELEE, T15, B2 ) -> proc_chance() ) )
    {
      p() -> resource_gain( RESOURCE_ENERGY, p() -> passives.tier15_2pc_melee -> effectN( 1 ).base_value(), p() -> gain.tier15_2pc_melee );
      p() -> proc.tier15_2pc_melee -> occur();
    }
  }
};

// ==========================================================================
// Renewing Mist
// ==========================================================================
/*
  TODO: Verify healing values.
  Add bouncing.
  */

struct renewing_mist_t: public monk_heal_t
{
  renewing_mist_t( monk_t& p, const std::string& options_str ):
    monk_heal_t( "renewing_mist", p, p.find_spell( 119611 ) )
  {
    parse_options( nullptr, options_str );
    stancemask = WISE_SERPENT;
    may_crit = may_miss = false;
    spell_power_mod.tick = p.spec.renewing_mist -> effectN( 3 ).sp_coeff();

    trigger_gcd = p.spec.renewing_mist -> gcd();
    base_execute_time = p.spec.renewing_mist -> cast_time( p.level );
    resource_current = RESOURCE_MANA;
    base_costs[RESOURCE_MANA] = p.spec.renewing_mist -> cost( POWER_MANA ) * p.resources.base[RESOURCE_MANA];
    cooldown -> duration = p.spec.renewing_mist -> cooldown();

    // Improved Renewing Mist
    dot_duration += p.perk.improved_renewing_mist -> effectN( 1 ).time_value();
  }

  virtual void execute()
  {
    monk_heal_t::execute();
    player -> resource_gain( RESOURCE_CHI, p() -> find_spell( 115151 ) -> effectN( 2 ).base_value(), p() -> gain.renewing_mist, this );
  }
};

// ==========================================================================
// Soothing Mist
// ==========================================================================
/*
  DESC: Surging Mist and Enveloping Mist need to be able to be cast while
  we're channeling Soothing Mist WITHOUT interrupting the channel. to
  achieve this, soothing_mist applies a HoT to the target that is
  removed whenever the monk executes an action that is not one of these
  whitelised spells. Auto attack is also halted while the HoT is
  active. The HoT is also removed any time the user moves, is stunned,
  interrupted, etc.

  TODO: Verify healing values.
  Change cost from per tick to on use + per second.
  Confirm the mana consumption is affected by lucidity.
  */

struct soothing_mist_t: public monk_heal_t
{
  int consecutive_failed_chi_procs;

  soothing_mist_t( monk_t& p, const std::string& options_str ):
    monk_heal_t( "soothing_mist", p, p.spec.soothing_mist )
  {
    parse_options( nullptr, options_str );

    stancemask = WISE_SERPENT;
    tick_zero = true;
    consecutive_failed_chi_procs = 0;

    // Cost is handled in action_t::tick()
    base_costs[RESOURCE_MANA] = 0.0;
    costs_per_second[RESOURCE_MANA] = 0;
  }

  virtual double action_ta_multiplier() const
  {
    double tm = monk_heal_t::action_ta_multiplier();

    player_t* t = ( execute_state ) ? execute_state -> target : target;

    if ( td( t ) -> dots.enveloping_mist -> is_ticking() )
      tm *= 1.0 + p() -> passives.enveloping_mist -> effectN( 2 ).percent();

    return tm;
  }

  virtual void tick( dot_t* d )
  {
    // Deduct mana / fizzle if the caster does not have enough mana
    double tick_cost = data().cost( POWER_MANA ) * p() -> resources.base[RESOURCE_MANA] * p() -> composite_spell_haste();
    if ( p() -> resources.current[RESOURCE_MANA] >= tick_cost )
      p() -> resource_loss( RESOURCE_MANA, tick_cost, p() -> gain.soothing_mist, this );
    else
    {
      player_t* t = ( execute_state ) ? execute_state -> target : target;

      td( t ) -> dots.enveloping_mist -> cancel();
      p() -> buff.channeling_soothing_mist -> expire();
      return;
    }

    monk_heal_t::tick( d );

    // Chi Gain
    if ( rng().roll( 0.15 + consecutive_failed_chi_procs * 0.15 ) )
    {
      p() -> resource_gain( RESOURCE_CHI, 1.0, p() -> gain.soothing_mist, this );
      consecutive_failed_chi_procs = 0;
    }
    else
      consecutive_failed_chi_procs++;
  }

  virtual void impact( action_state_t* s )
  {
    monk_heal_t::impact( s );

    p() -> buff.channeling_soothing_mist -> trigger();
  }

  virtual void last_tick( dot_t* d )
  {
    monk_heal_t::last_tick( d );

    p() -> buff.channeling_soothing_mist -> expire();
  }

  virtual bool ready()
  {
    if ( p() -> buff.channeling_soothing_mist -> check() )
      return false;

    // Mana cost check
    double tick_cost = data().cost( POWER_MANA ) * p() -> resources.base[RESOURCE_MANA];
    if ( p() -> resources.current[RESOURCE_MANA] < tick_cost )
      return false;

    return monk_heal_t::ready();
  }
};

// ==========================================================================
// Surging Mist
// ==========================================================================
/*
  TODO: Verify healing values.
  */

struct surging_mist_t: public monk_heal_t
{
  surging_mist_t( monk_t& p, const std::string& options_str ):
    monk_heal_t( "surging_mist", p, p.find_spell( 116994 ) )
  {
    parse_options( nullptr, options_str );

    stancemask = STURDY_OX | FIERCE_TIGER | WISE_SERPENT | SPIRITED_CRANE;

    if ( p.specialization() == MONK_MISTWEAVER )
    {
      resource_current = RESOURCE_MANA;
      base_costs[RESOURCE_MANA] = p.passives.surging_mist -> cost( POWER_MANA ) * p.resources.base[RESOURCE_MANA];
    }
    else
    {
      resource_current = RESOURCE_ENERGY;
      base_costs[RESOURCE_ENERGY] = p.passives.surging_mist -> cost( POWER_ENERGY );
    }

    may_miss = false;
  }

  virtual timespan_t execute_time() const
  {
    timespan_t et = monk_heal_t::execute_time();

    if ( p() -> buff.channeling_soothing_mist -> check() )
      return timespan_t::zero();

    return et;
  }

  virtual void execute()
  {
    monk_heal_t::execute();

    if ( p() -> specialization() == MONK_MISTWEAVER )
      player -> resource_gain( RESOURCE_CHI, p() -> passives.surging_mist -> effectN( 2 ).base_value(), p() -> gain.surging_mist, this );
  }

  virtual void impact(action_state_t* /*s*/)
  {
    //if (result_is_multistrike(s->result) && p() -> sets.has_set_bonus( MONK_MISTWEAVER, T17, B4 ) )

  }
};

// ==========================================================================
// Zen Sphere (Heal)
// ==========================================================================

struct zen_sphere_t: public monk_heal_t
{
  monk_spell_t* zen_sphere_damage;
  monk_spell_t* zen_sphere_detonate_damage;

  struct zen_sphere_detonate_heal_t: public monk_heal_t
  {
    zen_sphere_detonate_heal_t( monk_t& player ):
      monk_heal_t( "zen_sphere_detonate", player, player.find_spell( 124101 ) )
    {
      background = dual = true;
      aoe = -1;

      attack_power_mod.direct = 0.547; // hardcoded into tooltip
      school = SCHOOL_NATURE;
    }
  };

  zen_sphere_detonate_heal_t* zen_sphere_detonate_heal;

  zen_sphere_t( monk_t& p, const std::string& options_str ):
    monk_heal_t( "zen_sphere", p, p.talent.zen_sphere ),
    zen_sphere_damage( new spells::zen_sphere_damage_t( &p ) ),
    zen_sphere_detonate_damage( new spells::zen_sphere_detonate_damage_t( &p ) ),
    zen_sphere_detonate_heal( new zen_sphere_detonate_heal_t( p ) )
  {
    parse_options( nullptr, options_str );

    school = SCHOOL_NATURE;
    if ( player -> specialization() == MONK_MISTWEAVER )
      attack_power_mod.tick = 0.156 * 1.2; // hardcoded into tooltip
    else
      attack_power_mod.tick = 0.156;  // hardcoded into tooltip

    cooldown -> duration = timespan_t::from_seconds( 10.0 );
  }

  virtual void tick( dot_t* d )
  {
    monk_heal_t::tick( d );

    zen_sphere_damage -> execute();
  }

  virtual void last_tick( dot_t* d )
  {
    monk_heal_t::last_tick( d );

    zen_sphere_detonate_damage -> execute();
    zen_sphere_detonate_heal -> execute();
  }
};


// ==========================================================================
// Gift of the Ox
// ==========================================================================

struct gift_of_the_ox_t : public monk_heal_t
{
  gift_of_the_ox_t( monk_t& p, const std::string& options_str ) :
    monk_heal_t( "gift_of_the_ox", p, p.spec.gift_of_the_ox )
  {
    parse_options(nullptr, options_str);
    harmful = false;
    trigger_gcd = timespan_t::zero();
    attack_power_mod.direct = 0.40; // Hardcoded Sep-15-2014
  }

  virtual void execute()
  {
    if ( p() -> buff.gift_of_the_ox -> up() )
    {
      monk_heal_t::execute();

      p() -> buff.gift_of_the_ox -> decrement();
    }
  }
};

// ==========================================================================
// Healing Elixirs
// ==========================================================================

struct healing_elixirs_t : public monk_heal_t
{
  healing_elixirs_t( monk_t& p, const std::string& options_str ) :
    monk_heal_t( "healing_elixirs", p, p.talent.healing_elixirs )
  {
    parse_options(nullptr, options_str);
    harmful = false;
    trigger_gcd = timespan_t::zero();
    cooldown -> duration = p.talent.healing_elixirs -> effectN( 1 ).period();
  }

  virtual bool ready()
  {
    if ( p() -> buff.tigereye_brew_use -> check() )
      return true;

    if ( p() -> buff.elusive_brew_activated -> check() )
      return true;

    if ( p() -> buff.mana_tea -> check() )
      return true;

    //if ( p() -> health_percentage() < 35)
    //  return true;

    return false;
  }

  virtual void execute()
  {
      monk_heal_t::execute();

      double amount_healed = player -> resources.max[RESOURCE_HEALTH] * p() -> passives.healing_elixirs -> effectN( 1 ).percent();
      player -> resource_gain( RESOURCE_HEALTH, amount_healed, p() -> gain.healing_elixirs, this );
  }
};
} // end namespace heals

namespace absorbs {
struct monk_absorb_t: public monk_action_t < absorb_t >
{
  monk_absorb_t( const std::string& n, monk_t& player,
                 const spell_data_t* s = spell_data_t::nil() ):
                 base_t( n, &player, s )
  {
  }
};

// ==========================================================================
// Guard
// ==========================================================================

struct guard_t: public monk_absorb_t
{
  guard_t( monk_t& p, const std::string& options_str ):
    monk_absorb_t( "guard", p, p.spec.guard )
  {
    parse_options( nullptr, options_str );
    harmful = false;
    trigger_gcd = timespan_t::zero();
    target = &p;
    cooldown -> charges = p.perk.improved_guard -> effectN( 1 ).base_value();
    attack_power_mod.direct = 9; // hardcoded into tooltip 2014/09/09
    base_multiplier += p.sets.set( SET_TANK, T14, B4 ) -> effectN( 1 ).percent();
    if ( p.glyph.guard -> ok() )
      base_multiplier += p.glyph.guard -> effectN( 1 ).percent();
  }

  virtual void impact( action_state_t* s )
  {
    p() -> buff.guard -> trigger( 1, s -> result_amount );
    stats -> add_result( 0.0, s -> result_amount, ABSORB, s -> result, s -> block_result, s -> target );
  }
};
} // end namespace asborbs

using namespace attacks;
using namespace spells;
using namespace heals;
using namespace absorbs;
} // end namespace actions;

struct power_strikes_event_t: public event_t
{
  power_strikes_event_t( monk_t& player, timespan_t tick_time ):
    event_t( player, "power_strikes" )
  {
    // Safety clamp
    tick_time = clamp( tick_time, timespan_t::zero(), timespan_t::from_seconds( 15.0 ) );
    add_event( tick_time );
  }

  virtual void execute()
  {
    monk_t* p = debug_cast<monk_t*>( actor );

    p -> buff.power_strikes -> trigger();

    new ( sim() ) power_strikes_event_t( *p, timespan_t::from_seconds( 15.0 ) );
  }
};

// ==========================================================================
// Monk Character Definition
// ==========================================================================

monk_td_t::monk_td_t( player_t* target, monk_t* p ):
actor_pair_t( target, p ),
dots( dots_t() ),
debuff( buffs_t() )
{
  debuff.rising_sun_kick = buff_creator_t( *this, "rising_sun_kick" ).spell( p -> find_spell( 130320 ) );
  debuff.dizzying_haze = buff_creator_t( *this, "dizzying_haze" ).spell( p -> find_spell( 123727 ) );

  dots.enveloping_mist = target -> get_dot( "enveloping_mist", p );
  dots.renewing_mist = target -> get_dot( "renewing_mist", p );
  dots.soothing_mist = target -> get_dot( "soothing_mist", p );
  dots.zen_sphere = target -> get_dot( "zen_sphere", p );
}

// monk_t::create_action ====================================================

action_t* monk_t::create_action( const std::string& name,
                                 const std::string& options_str )
{
  using namespace actions;
  // Melee Attacks
  if ( name == "auto_attack"           ) return new            auto_attack_t( this, options_str );
  if ( name == "jab"                   ) return new                    jab_t( this, options_str );
  if ( name == "expel_harm"            ) return new       expel_harm_heal_t( *this, options_str );
  if ( name == "tiger_palm"            ) return new             tiger_palm_t( this, options_str );
  if ( name == "blackout_kick"         ) return new          blackout_kick_t( this, options_str );
  if ( name == "spinning_crane_kick"   ) return new    spinning_crane_kick_t( this, options_str );
  if ( name == "fists_of_fury"         ) return new          fists_of_fury_t( this, options_str );
  if ( name == "rising_sun_kick"       ) return new        rising_sun_kick_t( this, options_str );
  if ( name == "stance"                ) return new                 stance_t( this, options_str );
  if ( name == "tigereye_brew"         ) return new          tigereye_brew_t( this, options_str );
  if ( name == "energizing_brew"       ) return new        energizing_brew_t( this, options_str );
  if ( name == "provoke"               ) return new                provoke_t( this, options_str );
  if ( name == "touch_of_death"        ) return new         touch_of_death_t( this, options_str );
  // Brewmaster
  if ( name == "breath_of_fire"        ) return new         breath_of_fire_t( *this, options_str );
  if ( name == "keg_smash"             ) return new              keg_smash_t( *this, options_str );
  if ( name == "dizzying_haze"         ) return new          dizzying_haze_t( *this, options_str );
  if ( name == "guard"                 ) return new                  guard_t( *this, options_str );
  if ( name == "fortifying_brew"       ) return new        fortifying_brew_t( *this, options_str );
  if ( name == "elusive_brew"          ) return new           elusive_brew_t( *this, options_str );
  if ( name == "purifying_brew"        ) return new         purifying_brew_t( *this, options_str );
  if ( name == "gift_of_the_ox"        ) return new         gift_of_the_ox_t( *this, options_str );
  // Mistweaver
  if ( name == "enveloping_mist"       ) return new        enveloping_mist_t( *this, options_str );
  if ( name == "mana_tea"              ) return new               mana_tea_t( *this, options_str );
  if ( name == "renewing_mist"         ) return new          renewing_mist_t( *this, options_str );
  if ( name == "soothing_mist"         ) return new          soothing_mist_t( *this, options_str );
  if ( name == "surging_mist"          ) return new           surging_mist_t( *this, options_str );
  if ( name == "crackling_jade_lightning" ) return new crackling_jade_lightning_t( *this, options_str );
  // Talents
  if ( name == "chi_wave"              ) return new               chi_wave_t( this, options_str );
  if ( name == "zen_sphere"            ) return new             zen_sphere_t( *this, options_str );
  if ( name == "chi_burst"             ) return new              chi_burst_t( this, options_str );
  if ( name == "chi_sphere"            ) return new             chi_sphere_t( this, options_str ); // For Power Strikes
  if ( name == "chi_brew"              ) return new               chi_brew_t( this, options_str );
  if ( name == "healing_elixirs"       ) return new        healing_elixirs_t( *this, options_str );
  if ( name == "dampen_harm"           ) return new            dampen_harm_t( *this, options_str );
  if ( name == "diffuse_magic"         ) return new          diffuse_magic_t( *this, options_str );
  if ( name == "rushing_jade_wind"     ) return new    spinning_crane_kick_t( this, options_str );
  if ( name == "invoke_xuen"           ) return new             xuen_spell_t( this, options_str );
  if ( name == "chi_torpedo"           ) return new            chi_torpedo_t( this, options_str );
  if ( name == "hurricane_strike"      ) return new       hurricane_strike_t( this, options_str );
  if ( name == "chi_explosion"         ) return new          chi_explosion_t( this, options_str );
  if ( name == "serenity"              ) return new               serenity_t( this, options_str );
  return base_t::create_action( name, options_str );
}

// monk_t::create_pet =======================================================

pet_t* monk_t::create_pet( const std::string& name,
                           const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( name );

  if ( p ) return p;

  using namespace pets;
  if ( name == "xuen_the_white_tiger" ) return new xuen_pet_t( sim, this );

  // if ( name == "sef_fire_spirit" ) return new sef_fire_spell_t( sim, this );
  // if ( name == "sef_earth_spirit" ) return new sef_earth_spell_t( sim, this );

  return nullptr;
}

// monk_t::create_pets ======================================================

void monk_t::create_pets()
{
  base_t::create_pets();

  create_pet( "xuen_the_white_tiger" );
  //create_pet( "sef_fire_spirit" );
  //create_pet( "sef_earth_spirit" );
}

// monk_t::init_spells ======================================================

void monk_t::init_spells()
{
  base_t::init_spells();

  //TALENTS
  talent.celerity                 = find_talent_spell( "Celerity" );
  talent.tigers_lust              = find_talent_spell( "Tiger's Lust" );
  talent.momentum                 = find_talent_spell( "Momentum" );

  talent.chi_wave                 = find_talent_spell( "Chi Wave" );
  talent.zen_sphere               = find_talent_spell( "Zen Sphere" );
  talent.chi_burst                = find_talent_spell( "Chi Burst" );

  talent.power_strikes            = find_talent_spell( "Power Strikes" );
  talent.ascension                = find_talent_spell( "Ascension" );
  talent.chi_brew                 = find_talent_spell( "Chi Brew" );

  talent.ring_of_peace            = find_talent_spell( "Ring of Peace" );
  talent.charging_ox_wave         = find_talent_spell( "Charging Ox Wave" );
  talent.leg_sweep                = find_talent_spell( "Leg Sweep" );

  talent.healing_elixirs          = find_talent_spell( "Healing Elixirs" );
  talent.dampen_harm              = find_talent_spell( "Dampen Harm" );
  talent.diffuse_magic            = find_talent_spell( "Diffuse Magic" );

  talent.rushing_jade_wind        = find_talent_spell( "Rushing Jade Wind" );
  talent.invoke_xuen              = find_talent_spell( "Invoke Xuen, the White Tiger", "invoke_xuen" );
  talent.chi_torpedo              = find_talent_spell( "Chi Torpedo" );

  talent.breath_of_the_serpent    = find_talent_spell( "Breath of the Serpent" );
  talent.soul_dance               = find_talent_spell( "Soul Dance" );
  talent.hurricane_strike         = find_talent_spell( "Hurricane Strike" );
  talent.chi_explosion            = find_talent_spell( "Chi Explosion" );
  talent.path_of_mists            = find_talent_spell( "Path of Mists" );
  talent.serenity                 = find_talent_spell( "Serenity" );

  // PERKS
  perk.improved_guard                   = find_perk_spell( "Improved Guard" );
  perk.improved_life_cocoon             = find_perk_spell( "Improved Life Cocoon" );
  perk.improved_renewing_mist           = find_perk_spell( "Improved Renewing Mist" );
  perk.empowered_chi                    = find_perk_spell( "Empowered Chi" );
  perk.empowered_spinning_crane_kick    = find_perk_spell( "Empowered Spinning Crane Kick" );
  perk.enhanced_roll                    = find_perk_spell( "Enhanced Roll" );
  perk.improved_breath_of_fire          = find_perk_spell( "Improved Breath of Fire" );
  perk.enhanced_transcendence           = find_perk_spell( "Enhanced Transcendence" );

  // General Passives
  spec.way_of_the_monk            = find_spell( 108977 );
  spec.leather_specialization     = find_specialization_spell( "Leather Specialization" );
  spec.critical_strikes           = find_specialization_spell( "Critical Strikes" );
  spec.way_of_the_monk            = find_specialization_spell( "Way of the Monk" );
  spec.rising_sun_kick            = find_specialization_spell( "Rising Sun Kick" );
  spec.legacy_of_the_white_tiger  = find_specialization_spell( "Legacy of the White Tiger" );
  spec.touch_of_death             = find_specialization_spell( "Touch of Death" );
  spec.spinning_crane_kick        = find_class_spell( "Spinning Crane Kick" );

  // Windwalker Passives
  spec.brewing_tigereye_brew      = find_specialization_spell( "Brewing: Tigereye Brew" );
  spec.combo_breaker              = find_specialization_spell( "Combo Breaker" );
  spec.fists_of_fury              = find_specialization_spell( "Fists of Fury" );
  spec.flying_serpent_kick        = find_specialization_spell( "Flying Serpent Kick" );
  spec.energizing_brew            = find_specialization_spell( "Energizing Brew" );
  spec.afterlife                  = find_specialization_spell( "Afterlife" );
  spec.disable                    = find_specialization_spell( "Disable" );
  spec.tigereye_brew              = find_specialization_spell( "Tigereye Brew" );
  spec.touch_of_karma             = find_specialization_spell( "Touch of Karma" );
  spec.combat_conditioning        = find_specialization_spell( "Combat Conditioning" );
  spec.storm_earth_and_fire       = find_specialization_spell( "Storm Earth and Fire" );
  spec.battle_trance              = find_specialization_spell( "Battle Trance" );
  spec.windflurry                 = find_specialization_spell( "Windflurry" );

  // Brewmaster Passives
  spec.brewmaster_training        = find_specialization_spell( "Brewmaster Training" );
  spec.brewing_elusive_brew       = find_specialization_spell( "Brewing: Elusive Brew" );
  spec.elusive_brew               = find_specialization_spell( "Elusive Brew" );
  spec.desperate_measures         = find_specialization_spell( "Desperate Measures" );
  spec.dizzying_haze              = find_specialization_spell( "Dizzying Haze" );
  spec.breath_of_fire             = find_specialization_spell( "Breath of Fire" );
  spec.guard                      = find_specialization_spell( "Guard" );
  spec.summon_black_ox_statue     = find_specialization_spell( "Summon Black Ox Statue" );
  spec.purifying_brew             = find_specialization_spell( "Purifying Brew" );
  spec.keg_smash                  = find_specialization_spell( "Keg Smash" );
  spec.gift_of_the_ox             = find_specialization_spell( "Gift of the Ox" );
  spec.resolve                    = find_specialization_spell( "Resolve" );
  spec.bladed_armor               = find_specialization_spell( "Bladed Armor" );
  spec.ferment                    = find_specialization_spell( "Ferment" );

  // Mistweaver Passives
  spec.brewing_mana_tea             = find_specialization_spell( "Brewing: Mana Tea" );
  spec.mana_meditation              = find_specialization_spell( "Mana Meditation" );
  spec.focus_and_harmony            = find_specialization_spell( "Focus and Harmony" );
  spec.crane_style_techniques       = find_specialization_spell( "Crane Style Techniques" );
  spec.teachings_of_the_monastery   = find_specialization_spell( "Teachings of the Monastery" );
  spec.renewing_mist                = find_specialization_spell( "Renewing Mist" );
  spec.soothing_mist                = find_specialization_spell( "Soothing Mist" );
  spec.mana_tea                     = find_specialization_spell( "Mana Tea" );
  spec.revival                      = find_specialization_spell( "Revival" );
  spec.summon_jade_serpent_statue   = find_specialization_spell( "Summon Jade Serpent Statue" );
  spec.internal_medicine            = find_specialization_spell( "Internal Medicine" );
  spec.detonate_chi                 = find_specialization_spell( "Detonate Chi" );
  spec.mana_tea_driver              = find_specialization_spell( "Mana Tea Driver" );
  spec.legacy_of_the_emperor        = find_specialization_spell( "Legacy of the Emperor" );
  spec.soothing_mists_trigger       = find_specialization_spell( "Soothing Mists Trigger" );
  spec.uplift                       = find_specialization_spell( "Uplift" );
  spec.thunder_focus_tea            = find_specialization_spell( "Thunder Focus Tea" );
  spec.life_cocoon                  = find_specialization_spell( "Life Cocoon" );
  spec.enveloping_mist              = find_specialization_spell( "Enveloping Mist" );
  spec.jade_mists                   = find_specialization_spell( "Jade Mists" );

  // Stance
  stance_data.fierce_tiger        = find_class_spell( "Stance of the Fierce Tiger" );
  stance_data.sturdy_ox           = find_specialization_spell( "Stance of the Sturdy Ox" );
  stance_data.wise_serpent        = find_specialization_spell( "Stance of the Wise Serpent" );
  stance_data.spirited_crane      = find_specialization_spell( "Stance of the Spirited Crane" );

  //SPELLS
  active_actions.blackout_kick_dot = new actions::dot_blackout_kick_t( this );
  //active_actions.blackout_kick_heal = new actions::heal_blackout_kick_t( this );
  active_actions.chi_explosion_dot = new actions::dot_chi_explosion_t( this );

  if ( specialization() == MONK_BREWMASTER )
    active_actions.stagger_self_damage = new actions::stagger_self_damage_t( this );

  passives.tier15_2pc_melee       = find_spell( 138311 );
  passives.enveloping_mist        = find_class_spell( "Enveloping Mist" );
  passives.surging_mist           = find_class_spell( "Surging Mist" );
  passives.healing_elixirs        = find_spell( 134563 );
  passives.storm_earth_and_fire   = find_spell( 138228 );

  // GLYPHS
  glyph.expel_harm         = find_glyph( "Glyph of Expel Harm" );
  glyph.targeted_expulsion = find_glyph( "Glyph of Targeted Expulsion" );
  glyph.touch_of_death     = find_glyph( "Glyph of Touch of Death" );
  glyph.fortifying_brew    = find_glyph( "Glyph of Fortifying Brew"    );
  glyph.guard              = find_glyph( "Glyph of Guard" );
  glyph.mana_tea           = find_glyph( "Glyph of Mana Tea" );
  glyph.blackout_kick      = find_glyph( "Glyph of Blackout Kick" );

  //MASTERY
  mastery.bottled_fury        = find_mastery_spell( MONK_WINDWALKER );
  mastery.elusive_brawler     = find_mastery_spell( MONK_BREWMASTER );
  mastery.gift_of_the_serpent = find_mastery_spell( MONK_MISTWEAVER );
}

// monk_t::init_base ========================================================

void monk_t::init_base_stats()
{
  base_t::init_base_stats();

  base.distance = ( specialization() == MONK_MISTWEAVER ) ? 40 : 3;
  if ( specialization() != MONK_MISTWEAVER )
    base_gcd = timespan_t::from_seconds( 1.0 );

  resources.base[RESOURCE_CHI] = 4 + talent.ascension -> effectN( 1 ).base_value() + perk.empowered_chi -> effectN( 1 ).base_value();
  resources.base[RESOURCE_ENERGY] = 100;
  resources.base_multiplier[RESOURCE_MANA] *= 1.0 + talent.ascension -> effectN( 2 ).percent();

  base_chi_regen_per_second = 0;
  base_energy_regen_per_second = 10.0;

  if ( specialization() != MONK_MISTWEAVER )
    base.attack_power_per_agility = 1.0;
  if ( specialization() == MONK_MISTWEAVER )
    base.spell_power_per_intellect = 1.0;

  // Mistweaver
  if ( spec.mana_meditation -> ok() )
    base.mana_regen_from_spirit_multiplier = spec.mana_meditation -> effectN( 1 ).percent();

  // initialize resolve for Berwmaster
  if ( specialization() == MONK_BREWMASTER )
    resolve_manager.init();
}

// monk_t::init_scaling =====================================================

void monk_t::init_scaling()
{
  base_t::init_scaling();

  if ( specialization() != MONK_MISTWEAVER )
  {
    scales_with[STAT_INTELLECT] = false;
    scales_with[STAT_SPELL_POWER] = false;
    scales_with[STAT_AGILITY] = true;
    scales_with[STAT_WEAPON_DPS] = true;
  }
  scales_with[STAT_STRENGTH] = false;

  if ( off_hand_weapon.type != WEAPON_NONE )
    scales_with[STAT_WEAPON_OFFHAND_DPS] = true;
}

// monk_t::init_buffs =======================================================

void energizing_brew_energize( buff_t* buff, int, int )
{
  monk_t* monk = debug_cast<monk_t*>( buff -> player );

  monk -> resource_gain( RESOURCE_ENERGY,
                         monk -> buff.energizing_brew -> data().effectN( 1 ).base_value(),
                         monk -> gain.energizing_brew );
}

// monk_t::create_buffs =====================================================

void monk_t::create_buffs()
{
  base_t::create_buffs();

  // General
  buff.fortifying_brew = buff_creator_t( this, "fortifying_brew", find_spell( 120954 ) );
  buff.fortifying_brew -> cooldown -> duration += sets.set( SET_TANK, T16, B2 ) -> effectN( 1 ).time_value();

  buff.power_strikes = buff_creator_t( this, "power_strikes", talent.power_strikes -> effectN( 1 ).trigger() );

  // hard code the 5% for dual welding since currently no effect shows that at this point.
  buff.tiger_strikes = buff_creator_t( this, "tiger_strikes", find_spell( 120273 ) )
    .chance( main_hand_weapon.group() == WEAPON_1H ? 0.05 : 0.08 )
    .add_invalidate( CACHE_MULTISTRIKE );

  buff.tiger_power = buff_creator_t( this, "tiger_power", find_class_spell( "Tiger Palm" ) -> effectN( 2 ).trigger() );

  buff.rushing_jade_wind = buff_creator_t( this, "rushing_jade_wind", talent.rushing_jade_wind )
    .cd( timespan_t::zero() );

  buff.dampen_harm  = buff_creator_t( this, "dampen_harm", talent.dampen_harm )
    .cd( timespan_t::zero() )
    .max_stack( 3 );

  buff.diffuse_magic = buff_creator_t(this, "diffuse_magic", talent.diffuse_magic);

  buff.serenity = buff_creator_t(this, "serenity", talent.serenity);

  // Brewmaster
  buff.bladed_armor = buff_creator_t( this, "bladed_armor", spec.bladed_armor )
    .add_invalidate( CACHE_ATTACK_POWER );

  buff.elusive_brew_stacks = buff_creator_t( this, "elusive_brew_stacks", find_spell( 128939 ) );

  buff.elusive_brew_activated = buff_creator_t( this, "elusive_brew_activated", spec.elusive_brew )
    .add_invalidate( CACHE_DODGE )
    .cd( timespan_t::zero() );

  buff.guard = absorb_buff_creator_t( this, "guard", spec.guard )
    .source( get_stats( "guard" ) )
    .cd( timespan_t::zero() );

  buff.shuffle = buff_creator_t( this, "shuffle" ).spell( find_spell( 115307 ) )
    .add_invalidate( CACHE_PARRY );

  // Mistweaver
  buff.channeling_soothing_mist = buff_creator_t( this, "channeling_soothing_mist", spell_data_t::nil() );

  buff.mana_tea = buff_creator_t( this, "mana_tea", spec.mana_tea );

  // Windwalker
  buff.chi_sphere = buff_creator_t( this, "chi_sphere" ).max_stack( 5 );

  buff.gift_of_the_ox = buff_creator_t( this, "gift_of_the_ox" ).max_stack( 99 );

  buff.combo_breaker_bok = buff_creator_t( this, "combo_breaker_bok", find_class_spell( "Combo Breaker: Blackout Kick" ) );

  buff.combo_breaker_tp = buff_creator_t( this, "combo_breaker_tp", find_class_spell( "Combo Breaker: Tiger Palm" ) );

  buff.combo_breaker_ce = buff_creator_t( this, "combo_breaker_ce", find_class_spell( "Combo Breaker: Chi Explosion" ) );

  buff.energizing_brew = buff_creator_t( this, "energizing_brew", spec.energizing_brew )
    .max_stack( 1 )
    .tick_callback( energizing_brew_energize )
    .add_invalidate( CACHE_MULTISTRIKE );

  buff.energizing_brew -> buff_duration += sets.set( SET_MELEE, T14, B4 ) -> effectN( 1 ).time_value(); //verify working

  buff.tigereye_brew = buff_creator_t( this, "tigereye_brew", find_spell( 125195 ) )
    .period( timespan_t::zero() ); // Tigereye Brew does not tick, despite what the spelldata implies.
  buff.tigereye_brew_use = buff_creator_t( this, "tigereye_brew_use", spec.tigereye_brew ).add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buff.tigereye_brew_use -> buff_duration += sets.set( SET_MELEE, PVP, B4 ) -> effectN( 1 ).time_value();

  buff.storm_earth_and_fire = buff_creator_t( this, "storm_earth_and_fire", spec.storm_earth_and_fire );

  buff.forceful_winds = buff_creator_t( this, "forceful_winds", find_spell(166603) );
}

// monk_t::init_gains =======================================================

void monk_t::init_gains()
{
  base_t::init_gains();

  gain.chi_brew = get_gain( "chi_brew" );
  gain.chi_refund = get_gain( "chi_refund" );
  gain.chi_sphere = get_gain( "chi_sphere" );
  gain.combo_breaker_ce = get_gain( "combo_breaker_chi_explosion" );
  gain.combo_breaker_savings = get_gain( "combo_breaker_savings" );
  gain.crackling_jade_lightning = get_gain( "crackling_jade_lightning" );
  gain.energizing_brew = get_gain( "energizing_brew" );
  gain.energy_refund = get_gain( "energy_refund" );
  gain.expel_harm = get_gain( "expel_harm" );
  gain.jab = get_gain( "jab" );
  gain.keg_smash = get_gain( "keg_smash" );
  gain.mana_tea = get_gain( "mana_tea" );
  gain.renewing_mist = get_gain( "renewing_mist" );
  gain.serenity = get_gain( "serenity" );
  gain.soothing_mist = get_gain( "soothing_mist" );
  gain.spinning_crane_kick = get_gain( "spinning_crane_kick" );
  gain.rushing_jade_wind = get_gain( "rushing_jade_wind" );
  gain.surging_mist = get_gain( "surging_mist" );
  gain.tier15_2pc_melee = get_gain( "tier15_2pc_melee" );
  gain.tier16_4pc_melee = get_gain( "tier16_4pc_melee" );
  gain.tier16_4pc_tank = get_gain( "tier16_4pc_tank" );
  gain.gift_of_the_ox = get_gain( "gift_of_the_ox" );
}

// monk_t::init_procs =======================================================

void monk_t::init_procs()
{
  base_t::init_procs();

  proc.combo_breaker_bok = get_proc( "combo_breaker_bok" );
  proc.combo_breaker_ce = get_proc( "combo_breaker_ce" );
  proc.combo_breaker_tp = get_proc( "combo_breaker_tp" );
  proc.mana_tea = get_proc( "mana_tea" );
  proc.tier15_2pc_melee = get_proc( "tier15_2pc_melee" );
  proc.tier15_4pc_melee = get_proc( "tier15_4pc_melee" );
  proc.tier17_4pc_heal = get_proc( "tier17_2pc_heal" );
  proc.tigereye_brew = get_proc( "tigereye_brew" );
  proc.tigereye_brew_wasted = get_proc( "tigereye_brew_wasted" );
}

// monk_t::reset ============================================================

void monk_t::reset()
{
  base_t::reset();

  track_chi_consumption = 0.0;
  _active_stance = FIERCE_TIGER;
}

// monk_t::regen (brews/teas)================================================

void monk_t::regen( timespan_t periodicity )
{
  resource_e resource_type = primary_resource();

  if ( resource_type == RESOURCE_MANA )
  {
    //TODO: add mana tea here
  }

  base_t::regen( periodicity );
}

// monk_t::interrupt =========================================================

void monk_t::interrupt()
{
  // This function triggers stuns, movement, and other types of halts.

  // End any active soothing_mist channels
  if ( buff.channeling_soothing_mist -> check() )
  {
    for ( size_t i = 0, actors = sim -> player_non_sleeping_list.size(); i < actors; ++i )
    {
      player_t* t = sim -> player_non_sleeping_list[i];
      if ( get_target_data( t ) -> dots.soothing_mist -> is_ticking() )
      {
        get_target_data( t ) -> dots.soothing_mist -> cancel();
        buff.channeling_soothing_mist -> expire();
        break;
      }
    }
  }
  player_t::interrupt();
}

// monk_t::matching_gear_multiplier =========================================

double monk_t::matching_gear_multiplier( attribute_e attr ) const
{
  switch ( specialization() )
  {
  case MONK_MISTWEAVER:
    if ( attr == ATTR_INTELLECT )
      return spec.leather_specialization -> effectN( 1 ).percent();
    break;
  case MONK_WINDWALKER:
    if ( attr == ATTR_AGILITY )
      return spec.leather_specialization -> effectN( 1 ).percent();
    break;
  case MONK_BREWMASTER:
    if ( attr == ATTR_STAMINA )
      return spec.leather_specialization -> effectN( 1 ).percent();
    break;
  default:
    break;
  }

  return 0.0;
}

// monk_t::has_stagger ====================================================

bool monk_t::has_stagger()
{
  return active_actions.stagger_self_damage -> stagger_ticking();
}
void monk_t::clear_stagger()
{
  active_actions.stagger_self_damage -> clear_all_damage();
}

// monk_t::composite_attack_speed =========================================

double monk_t::composite_melee_speed() const
{
  double cas = base_t::composite_melee_speed();

  if ( !dual_wield() )
    cas *= 1.0 / ( 1.0 + spec.way_of_the_monk -> effectN( 2 ).percent() );

  return cas;
}

// monk_t::composite_melee_crit ============================================

double monk_t::composite_melee_crit() const
{
  double crit = player_t::composite_melee_crit();

  crit += spec.critical_strikes -> effectN( 1 ).percent();

  return crit;
}

// monk_t::composite_spell_crit ============================================

double monk_t::composite_spell_crit() const
{
  double crit = player_t::composite_spell_crit();

  crit += spec.critical_strikes -> effectN( 1 ).percent();

  return crit;
}

// monk_t::composite_player_multiplier =====================================

double monk_t::composite_player_multiplier( school_e school ) const
{
  double m = base_t::composite_player_multiplier( school );

  m *= 1.0 + active_stance_data( FIERCE_TIGER ).effectN( 3 ).percent();

  m *= 1.0 + buff.tigereye_brew_use -> value();

  if ( buff.storm_earth_and_fire -> up() )
  {
    if ( buff.storm_earth_and_fire -> stack() == 1 )
      m *= passives.storm_earth_and_fire -> effectN( 1 ).percent();
    else if ( buff.storm_earth_and_fire -> stack() == 2 )
      m *= passives.storm_earth_and_fire -> effectN( 2 ).percent();
  }

  return m;
}

// monk_t::composite_attribute_multiplier =====================================

double monk_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double cam = base_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_STAMINA )
   cam *= 1.0 + active_stance_data( STURDY_OX ).effectN( 5 ).percent();

  return cam;
}

// monk_t::composite_player_heal_multiplier ==================================

double monk_t::composite_player_heal_multiplier( const action_state_t* s ) const
{
  double m = base_t::composite_player_heal_multiplier( s );

  if ( current_stance() == WISE_SERPENT )
    m *= 1.0 + active_stance_data( WISE_SERPENT ).effectN( 3 ).percent();

  if ( buff.guard -> up() )
    m *= 1.0 + spec.guard -> effectN( 2 ).percent();

  // Resolve applies a blanket -60% healing for tanks
  if ( spec.resolve -> ok() )
    m *= 1.0 + spec.resolve -> effectN( 2 ).percent();

  return m;
}

// monk_t::composite_player_absorb_multiplier ==================================

double monk_t::composite_player_absorb_multiplier( const action_state_t* s ) const
{
  double m = base_t::composite_player_absorb_multiplier( s );

  // Resolve applies a blanket -60% healing & absorb for tanks
  if ( spec.resolve -> ok() )
    m *= 1.0 + spec.resolve -> effectN( 3 ).percent();

  return m;
}

// monk_t::composite_melee_expertise ========================================

double monk_t::composite_melee_expertise( weapon_t* weapon ) const
{
  double e = base_t::composite_melee_expertise( weapon );

  e += static_stance_data( STURDY_OX ).effectN( 11 ).percent();

  return e;
}

// monk_t::composite_melee_attack_power ==================================

double monk_t::composite_melee_attack_power() const
{
  if ( current_stance() == SPIRITED_CRANE )
    return composite_spell_power( SCHOOL_MAX ) * static_stance_data( SPIRITED_CRANE ).effectN( 3 ).percent();

  double ap = player_t::composite_melee_attack_power();

  ap += buff.bladed_armor -> data().effectN( 1 ).percent() * current.stats.get_stat( STAT_BONUS_ARMOR );

  return ap;
}

// monk_t::composite_parry ==============================================

double monk_t::composite_parry() const
{
  double p = base_t::composite_parry();

  if ( buff.shuffle -> check() )
    p += buff.shuffle -> data().effectN( 1 ).percent();

  return p;
}

// monk_t::composite_dodge ==============================================

double monk_t::composite_dodge() const
{
  double d = base_t::composite_dodge();

  if (buff.elusive_brew_activated -> check())
  {
    d += buff.elusive_brew_activated -> data().effectN( 1 ).percent();
    d += sets.set( SET_TANK, T14, B2 ) -> effectN( 1 ).percent();
  }

  return d;
}

// monk_t::composite_crit_avoidance =====================================

double monk_t::composite_crit_avoidance() const
{
  double c = base_t::composite_crit_avoidance();

  c += active_stance_data( STURDY_OX ).effectN( 7 ).percent();

  return c;
}

// monk_t::composite_rating_multiplier =================================

double monk_t::composite_rating_multiplier( rating_e rating ) const
{
  double m = base_t::composite_rating_multiplier( rating );

  switch ( rating )
  {
  case RATING_MELEE_CRIT:
    return m *= 1.0 + spec.ferment -> effectN( 1 ).percent();
  case RATING_SPELL_CRIT:
    return m *= 1.0 + spec.ferment -> effectN( 1 ).percent();
  case RATING_MULTISTRIKE:
    m *= 1.0 + spec.battle_trance -> effectN( 1 ).percent();
    m *= 1.0 + spec.jade_mists -> effectN( 1 ).percent();
    return m;
    break;
  default:
    break;
  }

  return m;
}

// monk_t::composite_multistrike ======================================

double monk_t::composite_multistrike() const
{
  double m = player_t::composite_multistrike();

  if ( buff.tiger_strikes -> check() )
    m += buff.tiger_strikes -> data().effectN( 1 ).percent();

  return m;
}

// monk_t::composite_armor_multiplier ===================================

double monk_t::composite_armor_multiplier() const
{
  double a = player_t::composite_armor_multiplier();

  a += active_stance_data( STURDY_OX ).effectN( 13 ).percent();

  return a;
}

// monk_t::composite_dodge ==============================================

void monk_t::invalidate_cache( cache_e c )
{
  base_t::invalidate_cache( c );

  switch ( c )
  {
  case CACHE_SPELL_POWER:
    if ( current_stance() == WISE_SERPENT || current_stance() == SPIRITED_CRANE )
      player_t::invalidate_cache( CACHE_ATTACK_POWER );
    break;
  case CACHE_BONUS_ARMOR:
    if ( spec.bladed_armor -> ok() )
      player_t::invalidate_cache( CACHE_ATTACK_POWER );
    break;
  default: break;
  }
}


// monk_t::create_options ===================================================

void monk_t::create_options()
{
  base_t::create_options();

  option_t monk_options[] =
  {
    opt_int( "initial_chi", user_options.initial_chi ),
    opt_null()
  };

  option_t::copy( options, monk_options );
}

// monk_t::copy_from =========================================================

void monk_t::copy_from( player_t* source )
{
  base_t::copy_from( source );

  monk_t* source_p = debug_cast<monk_t*>( source );

  user_options = source_p -> user_options;
}

// monk_t::primary_resource =================================================

resource_e monk_t::primary_resource() const
{
  if ( current_stance() == WISE_SERPENT )
    return RESOURCE_MANA;

  return RESOURCE_ENERGY;
}

// monk_t::primary_role =====================================================

role_e monk_t::primary_role() const
{
  if ( base_t::primary_role() == ROLE_DPS )
    return ROLE_HYBRID;

  if ( base_t::primary_role() == ROLE_TANK )
    return ROLE_TANK;

  if ( base_t::primary_role() == ROLE_HEAL )
    return ROLE_HYBRID;//To prevent spawning healing_target, as there is no support for healing.

  if ( specialization() == MONK_BREWMASTER )
    return ROLE_TANK;

  if ( specialization() == MONK_MISTWEAVER )
    return ROLE_HYBRID;//To prevent spawning healing_target, as there is no support for healing.

  if ( specialization() == MONK_WINDWALKER )
    return ROLE_DPS;

  return ROLE_HYBRID;
}

// monk_t::convert_hybrid_stat ==============================================

stat_e monk_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
  case STAT_STR_AGI_INT:
    switch ( specialization() )
    {
      case MONK_MISTWEAVER:
        return STAT_INTELLECT;
      case MONK_BREWMASTER:
      case MONK_WINDWALKER:
        return STAT_AGILITY;
      default:
        return STAT_NONE;
    }
  case STAT_AGI_INT:
    if ( specialization() == MONK_MISTWEAVER )
      return STAT_INTELLECT;
    else
      return STAT_AGILITY;
  case STAT_STR_AGI:
    return STAT_AGILITY;
  case STAT_STR_INT:
    return STAT_INTELLECT;
  case STAT_SPIRIT:
    if ( specialization() == MONK_MISTWEAVER )
      return s;
    else
      return STAT_NONE;
  case STAT_BONUS_ARMOR:
    if ( specialization() == MONK_BREWMASTER )
      return s;
    else
      return STAT_NONE;
  default: return s;
  }
}

// monk_t::pre_analyze_hook  ================================================

void monk_t::pre_analyze_hook()
{
  base_t::pre_analyze_hook();

  if ( stats_t* zen_sphere = find_stats( "zen_sphere" ) )
  {
    if ( stats_t* zen_sphere_dmg = find_stats( "zen_sphere_damage" ) )
    {
      zen_sphere_dmg -> total_execute_time = zen_sphere -> total_execute_time;
    }
  }
}

// monk_t::energy_regen_per_second ==========================================

double monk_t::energy_regen_per_second() const
{
  double r = base_t::energy_regen_per_second();

  r *= 1.0 + talent.ascension -> effectN( 3 ).percent();

  return r;
}

// monk_t::combat_begin ====================================================

void monk_t::combat_begin()
{
  base_t::combat_begin();

  resources.current[RESOURCE_CHI] = clamp( as<double>( user_options.initial_chi ), 0.0, resources.max[RESOURCE_CHI] );

  if ( _active_stance == FIERCE_TIGER && !buffs.fierce_tiger_movement_aura -> up() )
  {
    for ( size_t i = 0; i < sim -> player_non_sleeping_list.size(); ++i )
    {
      player_t* p = sim -> player_non_sleeping_list[i];
      if ( p -> is_enemy() || p -> type == PLAYER_GUARDIAN )
        break;

      p -> buffs.fierce_tiger_movement_aura -> trigger();
    }
  }

  if ( spec.resolve -> ok() )
    resolve_manager.start();

  if ( talent.power_strikes -> ok() )
  {
    // Random start of the first tick.
    timespan_t d = player_t::rng().real() * timespan_t::from_seconds( 15.0 );
    new ( *sim ) power_strikes_event_t( *this, d );
  }

  if ( spec.bladed_armor -> ok() )
    buff.bladed_armor -> trigger();
}

// monk_t::target_mitigation ====================================================

void monk_t::target_mitigation( school_e school,
                                dmg_e    dt,
                                action_state_t* s )
{
  // Stagger is not reduced by damage mitigation effects
  if ( s -> action -> id == 124255 )
    return;

  player_t::target_mitigation( school, dt, s );

  // Passive sources (Sturdy Ox)
  if ( school != SCHOOL_PHYSICAL)
    s -> result_amount *= 1.0 + active_stance_data( STURDY_OX ).effectN( 4 ).percent();

  // Damage Reduction Cooldowns

  // Dampen Harm // Currently reduces hits below 15% hp as well
  if ( buff.dampen_harm -> up() )
  {
    s -> result_amount *= 1.0 - buff.dampen_harm -> data().effectN( 2 ).percent(); // Dampen Harm reduction is stored as +50
    buff.dampen_harm -> decrement(); // A stack will only be removed if the reduction was applied.
  }

  // Diffuse Magic
  if ( buff.diffuse_magic -> up() && school != SCHOOL_PHYSICAL)
    s -> result_amount *= 1.0 + buff.diffuse_magic -> data().effectN( 1 ).percent();

  //Fortifying Brew
  if ( buff.fortifying_brew -> check() )
    s -> result_amount *= 1.0 + buff.fortifying_brew -> data().effectN( 2 ).percent();
}

// monk_t::assess_damage ====================================================

void monk_t::assess_damage( school_e school,
                            dmg_e    dtype,
                            action_state_t* s )
{
  buff.shuffle -> up();
  buff.fortifying_brew -> up();
  buff.elusive_brew_activated -> up();
  if ( s -> result_total > 0 && school == SCHOOL_PHYSICAL && !glyph.guard -> ok() )
    buff.guard -> up();
  else if ( s -> result_total > 0 && school != SCHOOL_PHYSICAL && glyph.guard -> ok() )
    buff.guard->up();

  if ( s -> result == RESULT_DODGE && sets.set( MONK_BREWMASTER, T17, B2 ) )
    resource_gain(RESOURCE_ENERGY, sets.set( MONK_BREWMASTER, T17, B2 ) -> effectN( 1 ).base_value(), gain.energy_refund);

  base_t::assess_damage( school, dtype, s );
}

// monk_t::assess_damage_imminent_pre_absorb ==============================

void monk_t::assess_damage_imminent_pre_absorb( school_e school,
                                                dmg_e    dtype,
                                                action_state_t* s )
{
  base_t::assess_damage_imminent_pre_absorb( school, dtype, s );

  if ( current_stance() != STURDY_OX )
    return;

  // Stagger damage can't be staggered!
  if ( s -> action -> id == 124255 )
    return;

  double stagger_dmg = 0;

  if ( school == SCHOOL_PHYSICAL )
    stagger_dmg += s -> result_amount > 0 ? s -> result_amount * stagger_pct() : 0.0;

  if (school != SCHOOL_PHYSICAL && talent.soul_dance)
    stagger_dmg += s -> result_amount > 0 ? s -> result_amount * ( stagger_pct() * talent.soul_dance -> effectN( 1 ).percent() ) : 0.0;

  s->result_amount -= stagger_dmg;
  // Hook up Stagger Mechanism
  if ( stagger_dmg > 0 )
    residual_action::trigger( active_actions.stagger_self_damage, this, stagger_dmg );
}

// Brewmaster Pre-Combat Action Priority List ============================

void monk_t::apl_pre_brewmaster()
{
  action_priority_list_t* pre = get_action_priority_list( "precombat" );

  // Flask
  if ( sim -> allow_flasks && level >= 80 )
  {
    if ( level >= 90 )
      pre -> add_action( "flask,type=greater_draenic_stamina_flask" );
    else if ( level >= 90 )
      pre -> add_action( "flask,type=earth" );
    else
      pre -> add_action( "flask,type=steelskin" );
  }

  if ( sim -> allow_food && level >= 80 )
  {
    if ( level >= 90 )
      pre -> add_action( "/food,type=sleeper_surprise" );
    else if ( level >= 85 )
      pre -> add_action( "/food,type=mogu_fish_stew" );
    else
      pre -> add_action( "/food,type=seafood_magnifique_feast" );
  }

  pre -> add_action( "stance,choose=sturdy_ox" );
  pre -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  if ( sim -> allow_potions && level >= 80 )
  {
    if ( level > 90 )
      pre -> add_action( "potion,name=draenic_agility" );
    else if ( level >= 85 )
      pre -> add_action( "potion,name=virmens_bite" );
    else
      pre -> add_action( "potion,name=tolvir" );
  }

  pre -> add_action( "dampen_harm" );
}

// Windwalker Pre-Combat Action Priority List ==========================

void monk_t::apl_pre_windwalker()
{
  std::string& precombat = get_action_priority_list( "precombat" ) -> action_list_str;

  if ( sim -> allow_flasks )
  {
    // Flask
    if ( level >= 90 )
      precombat += "flask,type=greater_draenic_agility_flask";
    else
      precombat += "flask,type=spring_blossoms";
  }

  if ( sim -> allow_food )
  {
    // Food
    if ( level >= 90 )
      precombat += "/food,type=rylak_crepes";
    else
      precombat += "/food,type=sea_mist_rice_noodles";
  }

  precombat += "/stance,choose=fierce_tiger";
  precombat += "/snapshot_stats";

  if ( sim -> allow_potions )
  {
    // Prepotion
    if ( level >= 90 )
      precombat += "/potion,name=draenic_agility";
    else if ( level >= 85 )
      precombat += "/potion,name=virmens_bite";
    else if ( level > 80 )
      precombat += "/potion,name=tolvir";
  }
}

// Mistweaver Pre-Combat Action Priority List ==========================

void monk_t::apl_pre_mistweaver()
{
  std::string& precombat = get_action_priority_list( "precombat" ) -> action_list_str;

  if ( sim -> allow_flasks )
  {
    if ( level >= 90 )
      precombat += "flask,type=greater_draenic_intellect_flask";
    // Flask
    if ( level >= 85 )
      precombat += "flask,type=warm_sun";
    else if ( level > 80 )
      precombat += "flask,type=draconic_mind";
  }

  if ( sim -> allow_food )
  {
    // Food
    if ( level >= 90 )
      precombat += "/food,type=rylak_crepes";
    else if ( level >= 85 )
      precombat += "/food,type=mogu_fish_stew";
    else if ( level > 80 )
      precombat += "/food,type=seafood_magnifique_feast";
  }

  precombat += "/stance,choose=wise_serpent";
  precombat += "/snapshot_stats";

  if ( sim -> allow_potions )
  {
    // Prepotion
    if ( level >= 90 )
      precombat += "/potion,name=draenic_intellect_potion";
    else if ( level >= 85 )
      precombat += "/potion,name=jade_serpent_potion";
    else if ( level > 80 )
      precombat += "/potion,name=volcanic_potion";
  }
}

// Brewmaster Combat Action Priority List =========================

void monk_t::apl_combat_brewmaster()
{
  std::vector<std::string> racial_actions = get_racial_actions();
  action_priority_list_t* def = get_action_priority_list( "default" );
  action_priority_list_t* st = get_action_priority_list( "st" );
  action_priority_list_t* aoe = get_action_priority_list( "aoe" );

  def -> add_action( "auto_attack" );

  for (size_t i = 0; i < racial_actions.size(); i++)
    def -> add_action( racial_actions[i] + ",if=energy<=40" );

  def -> add_action( this, "chi_sphere", "if=talent.power_strikes.enabled&buff.chi_sphere.react&chi<4" );
  def -> add_talent( this, "Chi Brew", "if=talent.chi_brew.enabled&chi.max-chi>=2&buff.elusive_brew_stacks.stack<=10" );
  def -> add_action( this, "Gift of the Ox", "if=buff.gift_of_the_ox.react&incoming_damage_1500ms");
  def -> add_talent( this, "Dampen Harm", "if=incoming_damage_1500ms&buff.fortifying_brew.down&buff.elusive_brew_activated.down" );
  def -> add_action( this, "Fortifying Brew", "if=incoming_damage_1500ms&buff.dampen_harm.down&buff.elusive_brew_activated.down" );
  def -> add_action( this, "Elusive Brew", "if=buff.elusive_brew_stacks.react>=9&buff.dampen_harm.down&buff.elusive_brew_activated.down" );
  def -> add_action( "invoke_xuen,if=talent.invoke_xuen.enabled&time>5" );
  def -> add_talent( this, "Serenity", "if=talent.serenity.enabled&energy<=40" );
  def -> add_action( "call_action_list,name=st,if=active_enemies<3" );
  def -> add_action( "call_action_list,name=aoe,if=active_enemies>=3" );

  st -> add_action( this, "Blackout Kick", "if=buff.shuffle.down");
  st -> add_action( this, "Purifying Brew", "if=!talent.chi_explosion.enabled&stagger.heavy" );
  st -> add_action( this, "Purifying Brew", "if=!buff.serenity.up" );
  st -> add_action( this, "Guard" );
  st -> add_action( this, "Keg Smash", "if=chi.max-chi>=2&!buff.serenity.remains" );
  st -> add_talent( this, "Chi Burst", "if=talent.chi_burst.enabled&energy.time_to_max>3" );
  st -> add_talent( this, "Chi Wave", "if=talent.chi_wave.enabled&energy.time_to_max>3" );
  st -> add_talent( this, "Zen Sphere", "cycle_targets=1,if=talent.zen_sphere.enabled&!dot.zen_sphere.ticking" );
  st -> add_talent( this, "Chi Explosion", "if=chi>=3" );
  st -> add_action( this, "Blackout Kick", "if=buff.shuffle.remains<=3&cooldown.keg_smash.remains>=gcd" );
  st -> add_action( this, "Blackout Kick", "if=buff.serenity.up" );
  st -> add_action( this, "Blackout Kick", "if=chi>=4" );
  st -> add_action( this, "Expel Harm", "if=chi.max-chi>=1&cooldown.keg_smash.remains>=gcd" );
  st -> add_action( this, "Jab", "if=chi.max-chi>=1&cooldown.keg_smash.remains>=gcd&cooldown.expel_harm.remains>=gcd" );
  st -> add_action( this, "Purifying Brew", "if=!talent.chi_explosion.enabled&stagger.moderate&buff.shuffle.remains>=6");
  st -> add_action( this, "Tiger Palm", "if=(energy+(energy.regen*(cooldown.keg_smash.remains)))>=40" );
  st -> add_action( this, "Tiger Palm", "if=cooldown.keg_smash.remains>=gcd" );

  aoe->add_action(this, "Guard");
  if (level >= 100)
    aoe -> add_action( this, "Breath of Fire", "if=chi>=3&buff.shuffle.remains>=6&dot.breath_of_fire.remains<=gcd" );
  else
    aoe -> add_action( this, "Breath of Fire", "if=chi>=3&buff.shuffle.remains>=6&dot.breath_of_fire.remains<=1&target.debuff.dizzying_haze.up" );
  aoe -> add_talent( this, "Chi Explosion", "if=chi>=4" );
  aoe -> add_talent( this, "Rushing Jade Wind", "if=chi.max-chi>=1&talent.rushing_jade_wind.enabled" );
  aoe -> add_action( this, "Purifying Brew", "if=!talent.chi_explosion.enabled&stagger.heavy" );
  aoe -> add_action( this, "Guard" );
  aoe -> add_action( this, "Keg Smash", "if=chi.max-chi>=2&!buff.serenity.remains" );
  aoe -> add_talent( this, "Chi Burst", "if=talent.chi_burst.enabled&energy.time_to_max>3" );
  aoe -> add_talent( this, "Chi Wave", "if=talent.chi_wave.enabled&energy.time_to_max>3" );
  aoe -> add_talent( this, "Zen Sphere", "cycle_targets=1,if=talent.zen_sphere.enabled&!dot.zen_sphere.ticking" );
  aoe -> add_action( this, "Blackout Kick", "if=talent.rushing_jade_wind.enabled&buff.shuffle.remains<=3&cooldown.keg_smash.remains>=gcd" );
  aoe -> add_action( this, "Blackout Kick", "if=talent.rushing_jade_wind.enabled&buff.serenity.up" );
  aoe -> add_action( this, "Blackout Kick", "if=talent.rushing_jade_wind.enabled&chi>=4" );
  aoe -> add_action( this, "Expel Harm", "if=chi.max-chi>=1&cooldown.keg_smash.remains>=gcd&(energy+(energy.regen*(cooldown.keg_smash.remains)))>=40" );
  aoe -> add_action( this, "Spinning Crane Kick", "if=chi.max-chi>=1&!talent.rushing_jade_wind.enabled" );
  aoe -> add_action( this, "Jab", "if=talent.rushing_jade_wind.enabled&chi.max-chi>=1&cooldown.keg_smash.remains>=gcd&cooldown.expel_harm.remains>=gcd" );
  aoe -> add_action( this, "Purifying Brew", "if=!talent.chi_explosion.enabled&talent.rushing_jade_wind.enabled&stagger.moderate&buff.shuffle.remains>=6");
  aoe -> add_action( this, "Tiger Palm", "if=talent.rushing_jade_wind.enabled&(energy+(energy.regen*(cooldown.keg_smash.remains)))>=40" );
  aoe -> add_action( this, "Tiger Palm", "if=talent.rushing_jade_wind.enabled&cooldown.keg_smash.remains>=gcd" );
}

// Windwalker Combat Action Priority List ===============================

void monk_t::apl_combat_windwalker()
{
  std::vector<std::string> racial_actions = get_racial_actions();
  action_priority_list_t* def = get_action_priority_list( "default" );
  action_priority_list_t* st = get_action_priority_list( "st" );
  action_priority_list_t* aoe = get_action_priority_list( "aoe" );

  def -> add_action( "auto_attack" );
  def -> add_action( "invoke_xuen,if=talent.invoke_xuen.enabled&time>5" );
  def -> add_action( "chi_sphere,if=talent.power_strikes.enabled&buff.chi_sphere.react&chi<4" );

  if ( sim -> allow_potions )
  {
    if ( level == 100 )
      def -> add_action( "potion,name=draenic_agility,if=buff.serenity.up|(!talent.serenity.enabled&trinket.proc.agility.react)" );
    else if ( level >= 85 )
      def -> add_action( "potion,name=virmens_bite,if=buff.bloodlust.react|target.time_to_die<=60" );
  }

  int num_items = (int)items.size();
  for ( int i = 0; i < num_items; i++ )
  {
    if ( items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
      def -> add_action( "use_item,name=" + items[i].name_str + ",if=buff.tigereye_brew_use.up|target.time_to_die<18" );
  }
  for ( size_t i = 0; i < racial_actions.size(); i++ )
    def -> add_action( racial_actions[i] + ",if=buff.tigereye_brew_use.up|target.time_to_die<18" );

  def -> add_talent( this, "Chi Brew", "if=chi.max-chi>=2&((charges=1&recharge_time<=10)|charges=2|target.time_to_die<charges*10)&buff.tigereye_brew.stack<=16" );
  def -> add_action( this, "Tiger Palm", "if=buff.tiger_power.remains<=3" );
  def -> add_action( this, "Tigereye Brew", "if=buff.tigereye_brew_use.down&buff.tigereye_brew.stack=20" );
  def -> add_action( this, "Tigereye Brew", "if=buff.tigereye_brew_use.down&buff.tigereye_brew.stack>=10&buff.serenity.up" );
  def -> add_action( this, "Tigereye Brew", "if=buff.tigereye_brew_use.down&buff.tigereye_brew.stack>=10&cooldown.fists_of_fury.up&chi>=3&debuff.rising_sun_kick.up&buff.tiger_power.up" );
  def -> add_action( this, "Tigereye Brew", "if=talent.hurricane_strike.enabled&buff.tigereye_brew_use.down&buff.tigereye_brew.stack>=10&cooldown.hurricane_strike.up&chi>=3&debuff.rising_sun_kick.up&buff.tiger_power.up" );
  def -> add_action( this, "Tigereye Brew", "if=buff.tigereye_brew_use.down&chi>=2&(buff.tigereye_brew.stack>=16|target.time_to_die<40)&debuff.rising_sun_kick.up&buff.tiger_power.up" );
  def -> add_action( this, "Rising Sun Kick", "if=(debuff.rising_sun_kick.down|debuff.rising_sun_kick.remains<3)" );
  def -> add_action( this, "Tiger Palm", "if=buff.tiger_power.down&debuff.rising_sun_kick.remains>1&energy.time_to_max>1" );
  def -> add_talent( this, "Serenity", "if=talent.serenity.enabled&chi>=2&buff.tiger_power.up&debuff.rising_sun_kick.up" );
  def -> add_action( "call_action_list,name=aoe,if=active_enemies>=3" );
  def -> add_action( "call_action_list,name=st,if=active_enemies<3" );

  st -> add_action( this, "Fists of Fury", "if=energy.time_to_max>cast_time&buff.tiger_power.remains>cast_time&debuff.rising_sun_kick.remains>cast_time&!buff.serenity.remains" );
  st -> add_action( this, "Touch of Death", "if=target.health.percent<10" );
  st -> add_talent( this, "Hurricane Strike", "if=talent.hurricane_strike.enabled&energy.time_to_max>cast_time&buff.tiger_power.remains>cast_time&debuff.rising_sun_kick.remains>cast_time&buff.energizing_brew.down" );
  st -> add_action( this, "Energizing Brew", "if=cooldown.fists_of_fury.remains>6&(!talent.serenity.enabled|(!buff.serenity.remains&cooldown.serenity.remains>4))&energy+energy.regen*gcd<50" );
  st -> add_action( this, "Rising Sun Kick", "if=!talent.chi_explosion.enabled");
  st -> add_talent( this, "Chi Wave", "if=energy.time_to_max>2&buff.serenity.down" );
  st -> add_talent( this, "Chi Burst", "if=talent.chi_burst.enabled&energy.time_to_max>2&buff.serenity.down" );
  st -> add_action( "zen_sphere,cycle_targets=1,if=energy.time_to_max>2&!dot.zen_sphere.ticking&buff.serenity.down" );
  st -> add_action( this, "Blackout Kick", "if=!talent.chi_explosion.enabled&(buff.combo_breaker_bok.react|buff.serenity.up)" );
  st -> add_talent( this, "Chi Explosion", "if=talent.chi_explosion.enabled&chi>=3&buff.combo_breaker_ce.react" );
  st -> add_action( this, "Tiger Palm", "if=buff.combo_breaker_tp.react&buff.combo_breaker_tp.remains<=2" );
  st -> add_action( this, "Blackout Kick", "if=!talent.chi_explosion.enabled&chi.max-chi<2" );
  st -> add_talent( this, "Chi Explosion", "if=talent.chi_explosion.enabled&chi>=3" );
  st -> add_action( this, "Jab", "if=chi.max-chi>=2" );

  aoe -> add_talent( this, "Chi Explosion", "if=chi>=4" );
  aoe -> add_talent( this, "Rushing Jade Wind" );
  aoe -> add_action( this, "Rising Sun Kick", "if=!talent.rushing_jade_wind.enabled&chi=chi.max" );
  aoe -> add_action( this, "Fists of Fury", "if=talent.rushing_jade_wind.enabled&energy.time_to_max>cast_time&buff.tiger_power.remains>cast_time&debuff.rising_sun_kick.remains>cast_time&!buff.serenity.remains" );
  aoe -> add_action( this, "Touch of Death", "if=target.health.percent<10" );
  aoe -> add_talent( this, "Hurricane Strike", "if=talent.rushing_jade_wind.enabled&talent.hurricane_strike.enabled&energy.time_to_max>cast_time&buff.tiger_power.remains>cast_time&debuff.rising_sun_kick.remains>cast_time&buff.energizing_brew.down" );
  aoe -> add_action( "zen_sphere,cycle_targets=1,if=!dot.zen_sphere.ticking" );
  aoe -> add_talent( this, "Chi Wave", "if=energy.time_to_max>2&buff.serenity.down");
  aoe -> add_talent( this, "Chi Burst", "if=talent.chi_burst.enabled&energy.time_to_max>2&buff.serenity.down" );
  aoe -> add_action( this, "Blackout Kick", "if=talent.rushing_jade_wind.enabled&!talent.chi_explosion.enabled&(buff.combo_breaker_bok.react|buff.serenity.up)" );
  aoe -> add_action( this, "Tiger Palm", "if=talent.rushing_jade_wind.enabled&buff.combo_breaker_tp.react&buff.combo_breaker_tp.remains<=2" );
  aoe -> add_action( this, "Blackout Kick", "if=talent.rushing_jade_wind.enabled&!talent.chi_explosion.enabled&chi.max-chi<2" );
  aoe -> add_action( this, "Spinning Crane Kick", "if=!talent.rushing_jade_wind.enabled" );
  aoe -> add_action( this, "Jab", "if=talent.rushing_jade_wind.enabled&chi.max-chi>=2" );
}

// Mistweaver Combat Action Priority List ==================================

void monk_t::apl_combat_mistweaver()
{
  action_priority_list_t* def = get_action_priority_list( "default" );

  std::string& aoe_list_str = get_action_priority_list( "aoe" ) -> action_list_str;
  std::string& st_list_str = get_action_priority_list( "single_target" ) -> action_list_str;

  def -> add_action( "auto_attack" );
  def -> add_talent( this, "Chi Brew", "if=chi=0" );
  def -> add_action( this, "Mana Tea", "if=buff.mana_tea.react>=2&mana.pct<=25" );

  if ( sim -> allow_potions )
  {
    if ( level >= 85 )
      def -> add_action( "/potion,name=jade_serpent_potion,if=buff.bloodlust.react|target.time_to_die<=60" );
  }

  def -> add_talent( this, "Invoke Xuen" );
  def -> add_action( "run_action_list,name=aoe,if=active_enemies>=3" );
  def -> add_action( "run_action_list,name=single_target,if=active_enemies<3" );

  st_list_str += "/crackling_jade_lightning,if=buff.bloodlust.up&buff.lucidity.up";
  st_list_str += "/tiger_palm,if=buff.lucidity.up";
  st_list_str += "/jab,if=buff.lucidity.up";
  st_list_str += "/tiger_palm,if=!buff.tiger_power.up";
  st_list_str += "/blackout_kick,if=buff.tiger_power.up&chi>1";
  st_list_str += "/tiger_palm,if=buff.tiger_power.up";
  st_list_str += "/chi_wave,if=talent.chi_wave.enabled";
  st_list_str += "/zen_sphere,cycle_targets=1,if=talent.zen_sphere.enabled&!dot.zen_sphere.ticking";
  st_list_str += "/jab";

  aoe_list_str += "/spinning_crane_kick,if=!talent.rushing_jade_wind.enabled";
  aoe_list_str += "/rushing_jade_wind,if=talent.rushing_jade_wind.enabled";
  aoe_list_str += "/zen_sphere,cycle_targets=1,if=talent.zen_sphere.enabled&!dot.zen_sphere.ticking";
  aoe_list_str += "/chi_burst,if=talent.chi_burst.enabled";
  aoe_list_str += "/tiger_palm,if=!buff.tiger_power.up";
  aoe_list_str += "/blackout_kick,if=buff.tiger_power.up&chi>1";
  aoe_list_str += "/jab,if=talent.rushing_jade_wind.enabled";
}

// monk_t::init_actions =====================================================

void monk_t::init_action_list()
{
  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }
  clear_action_priority_lists();

  // Precombat
  switch ( specialization() )
  {
  case MONK_BREWMASTER:
    apl_pre_brewmaster();
    break;
  case MONK_WINDWALKER:
    apl_pre_windwalker();
    break;
  case MONK_MISTWEAVER:
    apl_pre_mistweaver();
    break;
  default: break;
  }

  // Combat
  switch ( specialization() )
  {
  case MONK_BREWMASTER:
    apl_combat_brewmaster();
    break;
  case MONK_WINDWALKER:
    apl_combat_windwalker();
    break;
  case MONK_MISTWEAVER:
    apl_combat_mistweaver();
    break;
  default:
    add_action( "Jab" );
    break;
  }
  use_default_action_list = true;

  base_t::init_action_list();
}

// monk_t::stagger_pct ===================================================

double monk_t::stagger_pct()
{
  double stagger = 0.0;

  if ( current_stance() == STURDY_OX ) // no stagger without active stance
  {
    stagger += static_stance_data( STURDY_OX ).effectN( 8 ).percent();

    if ( buff.shuffle -> check() )
      stagger += buff.shuffle -> data().effectN( 2 ).percent();

    if ( spec.brewmaster_training -> ok() && buff.fortifying_brew -> check() )
      stagger += spec.brewmaster_training -> effectN( 1 ).percent();

    if ( mastery.elusive_brawler -> ok() )
      stagger += cache.mastery_value();
  }

  return stagger;
}

// monk_t::stagger_dmg ==================================================

double monk_t::current_stagger_dmg()
{
  double dmg = 0;
  if ( active_actions.stagger_self_damage )
  {
    dot_t* dot = active_actions.stagger_self_damage -> get_dot();

    if ( dot && dot -> state )
    {
      dmg = dot -> state -> result_amount;
      if ( dot -> state -> result == RESULT_HIT )
        dmg *= 1.0 + active_actions.stagger_self_damage -> total_crit_bonus();
    }
  }
  return dmg;
}

// monk_t::create_expression ==================================================

expr_t* monk_t::create_expression( action_t* a, const std::string& name_str )
{
  std::vector<std::string> splits = util::string_split( name_str, "." );
  if ( splits.size() == 2 && splits[0] == "stagger" )
  {
    struct stagger_threshold_expr_t: public expr_t
    {
      monk_t& player;
      double stagger_health_pct;
      stagger_threshold_expr_t( monk_t& p, double stagger_health_pct ):
        expr_t( "stagger_threshold_" + util::to_string( stagger_health_pct ) ),
        player( p ), stagger_health_pct( stagger_health_pct )
      { }

      virtual double evaluate()
      {
        return player.current_stagger_dmg() / player.resources.max[RESOURCE_HEALTH] > stagger_health_pct;
      }
    };
    struct stagger_amount_expr_t: public expr_t
    {
      monk_t& player;
      stagger_amount_expr_t( monk_t& p ):
        expr_t( "stagger_amount" ),
        player( p )
      { }

      virtual double evaluate()
      {
        return player.current_stagger_dmg();
      }
    };

    if ( splits[1] == "light" )
      return new stagger_threshold_expr_t( *this, 0.0 );
    else if ( splits[1] == "moderate" )
      return new stagger_threshold_expr_t( *this, 0.03 );
    else if ( splits[1] == "heavy" )
      return new stagger_threshold_expr_t( *this, 0.06 );
    else if ( splits[1] == "amount" )
      return new stagger_amount_expr_t( *this );
  }

  return base_t::create_expression( a, name_str );
}

// monk_t::stance_invalidates =====================================================

void monk_t::stance_invalidates( stance_e stance )
{
  switch ( stance )
  {
  case FIERCE_TIGER:
    invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    invalidate_cache( CACHE_ATTACK_SPEED );
    break;
  case STURDY_OX:
    invalidate_cache( CACHE_STAMINA );
    invalidate_cache( CACHE_CRIT_AVOIDANCE );
    break;
  case WISE_SERPENT:
    invalidate_cache( CACHE_PLAYER_HEAL_MULTIPLIER );
    invalidate_cache( CACHE_HASTE );
    break;
  case SPIRITED_CRANE:
    invalidate_cache( CACHE_PLAYER_HEAL_MULTIPLIER );
    invalidate_cache( CACHE_HASTE );
    invalidate_cache( CACHE_ATTACK_POWER );
    break;
  }
}

// monk_t::switch_to_stance =================================================

bool monk_t::switch_to_stance( stance_e to )
{
  if ( to == current_stance() )
    return false;

  // validate stance availability
  if ( !static_stance_data( to ).ok() )
    return false;

  if ( to == FIERCE_TIGER )
  {
    for ( size_t i = 0; i < sim -> player_non_sleeping_list.size(); ++i )
    {
      player_t* p = sim -> player_non_sleeping_list[i];
      if ( p -> is_enemy() || p -> type == PLAYER_GUARDIAN )
        break;

      p -> buffs.fierce_tiger_movement_aura -> trigger();
    }
  }
  else if ( _active_stance == FIERCE_TIGER )
  {
    for ( size_t i = 0; i < sim -> player_non_sleeping_list.size(); ++i )
    {
      player_t* p = sim -> player_non_sleeping_list[i];
      if ( p -> is_enemy() || p -> type == PLAYER_GUARDIAN )
        break;

      p -> buffs.fierce_tiger_movement_aura -> expire();
    }
  }

  stance_invalidates( current_stance() ); // Invalidate old stance
  _active_stance = to;
  stance_invalidates( current_stance() ); // Invalidate new stance

  recalculate_resource_max( RESOURCE_HEALTH ); // Sturdy Ox Stamina multiplier

  return true;
}

// monk_t::static_stance_data =================================================

/* Returns the stance data of the requested stance
 */
inline const spell_data_t& monk_t::static_stance_data( stance_e stance ) const
{
  switch ( stance )
  {
  case FIERCE_TIGER:
    return *stance_data.fierce_tiger;
  case STURDY_OX:
    return *stance_data.sturdy_ox;
  case SPIRITED_CRANE:
    return *stance_data.spirited_crane;
  case WISE_SERPENT:
    return *stance_data.wise_serpent;
  }

  assert( false );
  return *spell_data_t::not_found();
}

// monk_t::active_stance_data =================================================

/* Returns the stance data of the requested stance ONLY IF the stance is active
 */
const spell_data_t& monk_t::active_stance_data( stance_e stance ) const
{
  if ( stance != current_stance() )
    return *spell_data_t::not_found();

  return static_stance_data( stance );
}

// monk_t::monk_report =================================================

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class monk_report_t: public player_report_extension_t
{
public:
  monk_report_t( monk_t& player ):
    p( player )
  {
  }

  virtual void html_customsection( report::sc_html_stream& /* os*/ ) override
  {
    (void)p;
    /*// Custom Class Section
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n"
    << "\t\t\t\t\t<h3 class=\"toggle open\">Custom Section</h3>\n"
    << "\t\t\t\t\t<div class=\"toggle-content\">\n";

    os << p.name();

    os << "\t\t\t\t\t\t</div>\n" << "\t\t\t\t\t</div>\n";
    */
  }
private:
  monk_t& p;
};

// MONK MODULE INTERFACE ====================================================

struct monk_module_t: public module_t
{
  monk_module_t(): module_t( MONK ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const
  {
    monk_t* p = new monk_t( sim, name, r );
    p -> report_extension = std::shared_ptr<player_report_extension_t>( new monk_report_t( *p ) );
    return p;
  }
  virtual bool valid() const { return true; }
  virtual void init( sim_t* sim ) const
  {
    for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
    {
      player_t* p = sim -> actor_list[i];
      p -> buffs.fierce_tiger_movement_aura = buff_creator_t( p, "fierce_tiger_movement_aura", p -> find_spell( 103985 ) )
        .duration( timespan_t::from_seconds( 0 ) );
    }
  }
  virtual void combat_begin( sim_t* ) const {}
  virtual void combat_end( sim_t* ) const {}
};
} // UNNAMED NAMESPACE

const module_t* module_t::monk()
{
  static monk_module_t m;
  return &m;
}
