// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
/*
  TODO:

  Add all buffs
  - Crackling Jade Lightning
  Change expel harm to heal later on.

  WINDWALKER:
  Power Strikes timers not linked to spelldata (fix soon)

  MISTWEAVER:
  Implement the following spells:
   - Surging Mist
    * Interaction w/ Vital Mists
   - Renewing Mist
   - Revival
   - Uplift
   - Life Cocoon
   - Teachings of the Monastery
    * SCK healing
    * CJL damage increase
    * BoK's cleave effect
   - Non-glyphed Mana Tea
  Check damage values of Jab, TP, BoK, SCK, etc.

  BREWMASTER:
  - Swift Reflexes strike back
  - Purifying Brew
  - Level 75 talents
  - Black Ox Statue
  - Gift of the Ox

  - Avert Harm
  - Zen Meditation
  - Cache stagger_pct
*/
#include "simulationcraft.hpp"

// ==========================================================================
// Monk
// ==========================================================================

namespace { // UNNAMED NAMESPACE

struct monk_t;

enum stance_e { STURDY_OX = 1, FIERCE_TIGER, WISE_SERPENT = 4 };

struct monk_td_t : public actor_pair_t
{
public:
  struct buffs_t
  {
    debuff_t* rising_sun_kick;
    debuff_t* rushing_jade_wind;
    debuff_t* dizzying_haze;
    buff_t* enveloping_mist;
  } buff;

  monk_td_t( player_t*, monk_t* );
};

struct monk_t : public player_t
{
private:
  stance_e _active_stance;
public:
  typedef player_t base_t;

  struct active_actions_t
  {
    action_t* blackout_kick_dot;
    action_t* stagger_self_damage;
  } active_actions;

  double track_chi_consumption;

  struct buffs_t
  {
    buff_t* chi_sphere;
    buff_t* combo_breaker_tp;
    buff_t* combo_breaker_bok;
    buff_t* energizing_brew;
    buff_t* elusive_brew_stacks;
    buff_t* elusive_brew_activated;
    buff_t* fortifying_brew;
    absorb_buff_t* guard;
    buff_t* mana_tea;
    buff_t* muscle_memory;
    buff_t* power_guard;
    buff_t* power_strikes;
    buff_t* serpents_zeal;
    buff_t* shuffle;
    buff_t* tigereye_brew;
    buff_t* tigereye_brew_use;
    buff_t* tiger_power;
    haste_buff_t* tiger_strikes;
    buff_t* vital_mists;
    buff_t* zen_sphere;

    //  buff_t* zen_meditation;
    //  buff_t* path_of_blossoms;
  } buff;

private:
  struct stance_data_t
  {
    const spell_data_t* fierce_tiger;
    const spell_data_t* sturdy_ox;
    const spell_data_t* wise_serpent;
  } stance_data;
public:

  struct gains_t
  {
    gain_t* avoided_chi;
    gain_t* chi;
    gain_t* chi_brew;
    gain_t* combo_breaker_savings;
    gain_t* energy_refund;
    gain_t* energizing_brew;
    gain_t* mana_tea;
    gain_t* muscle_memory;
    gain_t* soothing_mist;
    gain_t* tier15_2pc;
  } gain;

  struct procs_t
  {
    proc_t* combo_breaker_bok;
    proc_t* combo_breaker_tp;
    proc_t* mana_tea;
    proc_t* tier15_2pc_melee;
    proc_t* tier15_4pc_melee;
  } proc;

  struct rngs_t
  {
    rng_t* mana_tea;
    rng_t* tier15_2pc_melee;
    rng_t* tier15_4pc_melee;
  } rng;

  struct talents_t
  {
    //  TODO: Implement
    //   const spell_data_t* celerity;
    //   const spell_data_t* tigers_lust;
    //   const spell_data_t* momentum;

    const spell_data_t* chi_wave;
    const spell_data_t* zen_sphere;
    const spell_data_t* chi_burst;

    const spell_data_t* power_strikes;
    const spell_data_t* ascension;
    const spell_data_t* chi_brew;

    //   const spell_data_t* deadly_reach;
    //   const spell_data_t* charging_ox_wave;
    //   const spell_data_t* leg_sweep;

    //   const spell_data_t* healing_elixers;
    //   const spell_data_t* dampen_harm;
    //   const spell_data_t* diffuse_magic;

    const spell_data_t* rushing_jade_wind;
    const spell_data_t* invoke_xuen;
    const spell_data_t* chi_torpedo;
  } talent;

  // Specialization
  struct specs_t
  {
    // GENERAL
    const spell_data_t* leather_specialization;
    const spell_data_t* way_of_the_monk;

    // Brewmaster
    const spell_data_t* brewing_elusive_brew;
    const spell_data_t* brewmaster_training;
    const spell_data_t* elusive_brew;
    const spell_data_t* desperate_measures;

    // Mistweaver
    const spell_data_t* brewing_mana_tea;
    const spell_data_t* mana_meditation;
    const spell_data_t* muscle_memory;
    const spell_data_t* teachings_of_the_monastery;

    // Windwalker
    const spell_data_t* brewing_tigereye_brew;
    const spell_data_t* combo_breaker;

  } spec;

  struct mastery_spells_t
  {
    const spell_data_t* bottled_fury;        // Windwalker
    const spell_data_t* elusive_brawler;     // Brewmaster
    const spell_data_t* gift_of_the_serpent; // Mistweaver
  } mastery;

  struct glyphs_t
  {
    // Prime
    const spell_data_t* fortifying_brew;

    // Major
  } glyph;

  struct cooldowns_t
  {
  } cooldowns;

  struct passives_t
  {
    const spell_data_t* tier15_2pc;
    const spell_data_t* swift_reflexes;
  } passives;

  // Options
  struct options_t
  {
    int initial_chi;
  } user_options;

private:
  target_specific_t<monk_td_t*> target_data;
public:

  monk_t( sim_t* sim, const std::string& name, race_e r ) :
    player_t( sim, MONK, name, r ),
    _active_stance( FIERCE_TIGER ),
    active_actions( active_actions_t() ),
    track_chi_consumption( 0.0 ),
    buff( buffs_t() ),
    stance_data( stance_data_t() ),
    gain( gains_t() ),
    proc( procs_t() ),
    rng( rngs_t() ),
    talent( talents_t() ),
    spec( specs_t() ),
    mastery( mastery_spells_t() ),
    glyph( glyphs_t() ),
    cooldowns( cooldowns_t() ),
    passives( passives_t() ),
    user_options( options_t() )
  {

  }

  // player_t overrides
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual double    composite_melee_speed();
  virtual double    energy_regen_per_second();
  virtual double    composite_melee_haste();
  virtual double    composite_spell_haste();
  virtual double    composite_player_multiplier( school_e school );
  virtual double    composite_player_heal_multiplier( school_e school );
  virtual double    composite_spell_hit();
  virtual double    composite_melee_hit();
  virtual double    composite_melee_expertise( weapon_t* weapon );
  virtual double    composite_melee_attack_power();
  virtual double    composite_parry();
  virtual double    composite_dodge();
  virtual double    composite_crit_avoidance();
  virtual pet_t*    create_pet   ( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual void      init_spells();
  virtual void      init_base_stats();
  virtual void      init_scaling();
  virtual void      create_buffs();
  virtual void      init_gains();
  virtual void      init_rng();
  virtual void      init_procs();
  virtual void      init_defense();
  virtual void      regen( timespan_t periodicity );
  virtual void      reset();
  virtual double    matching_gear_multiplier( attribute_e attr );
  virtual int       decode_set( item_t& );
  virtual void      create_options();
  virtual void      copy_from( player_t* );
  virtual resource_e primary_resource();
  virtual role_e    primary_role();
  virtual void      pre_analyze_hook();
  virtual void      combat_begin();
  virtual void      assess_damage( school_e, dmg_e, action_state_t* s );
  virtual void      assess_damage_imminent( school_e, dmg_e, action_state_t* s );
  virtual void      target_mitigation( school_e, dmg_e, action_state_t* );
  virtual void invalidate_cache( cache_e );
  virtual void      init_actions();
  virtual monk_td_t* get_target_data( player_t* target )
  {
    monk_td_t*& td = target_data[ target ];
    if ( ! td )
    {
      td = new monk_td_t( target, this );
    }
    return td;
  }

  // Monk specific
  void apl_pre_brewmaster();
  void apl_pre_windwalker();
  void apl_pre_mistweaver();
  void apl_combat_brewmaster();
  void apl_combat_windwalker();
  void apl_combat_mistweaver();
  double stagger_pct();

  // Stance
  stance_e current_stance() const
  { return _active_stance; }
  bool switch_to_stance( stance_e );
  void stance_invalidates( stance_e );
  const spell_data_t& static_stance_data( stance_e );
  const spell_data_t& active_stance_data( stance_e );

};

// ==========================================================================
// Monk Pets & Statues
// ==========================================================================

namespace pets {

struct statue_t : public pet_t
{
  statue_t( sim_t* sim, monk_t* owner, const std::string& n, pet_e pt, bool guardian = false ) :
    pet_t( sim, owner, n, pt, guardian )
  { }

  monk_t* o() const
  { return static_cast<monk_t*>( owner ); }
};

struct jade_serpent_statue_t : public statue_t
{
  typedef statue_t base_t;

  jade_serpent_statue_t ( sim_t* sim, monk_t* owner, const std::string& n ) :
    base_t( sim, owner, n, PET_NONE, true )
  { }
};

struct xuen_pet_t : public pet_t
{
private:
  struct melee_t : public melee_attack_t
  {
    melee_t( const std::string& n, xuen_pet_t* player ) :
      melee_attack_t( n, player, spell_data_t::nil() )
    {
      background = true;
      repeating = true;
      may_crit = true;
      may_glance = true;
      school      = SCHOOL_NATURE;

      // Use damage numbers from the level-scaled weapon
      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;

      trigger_gcd = timespan_t::zero();
      special     = false;
    }

    void execute()
    {
      if ( time_to_execute > timespan_t::zero() && player -> executing )
      {
        if ( sim -> debug )
          sim -> output( "Executing '%s' during melee (%s).", player -> executing -> name(), util::slot_type_string( weapon -> slot ) );
        schedule_execute();
      }
      else
      {
        attack_t::execute();
      }
    }
  };

  struct crackling_tiger_lightning_t : public melee_attack_t
  {
    crackling_tiger_lightning_t( xuen_pet_t* player, const std::string& options_str ) :
      melee_attack_t( "crackling_tiger_lightning", player, player -> find_spell( 123996 ) )
    {
      parse_options( nullptr, options_str );

      special = true;
      tick_may_crit  = true;
      aoe = 3;
      tick_power_mod = data().extra_coeff();
      cooldown -> duration = timespan_t::from_seconds( 6.0 );
      base_spell_power_multiplier  = 0;

      //base_multiplier = 1.323; //1.58138311; EDITED FOR ACTUAL VALUE. verify in the future.
    }
  };

  struct auto_attack_t : public attack_t
  {
    auto_attack_t( xuen_pet_t* player, const std::string& options_str ) :
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
  xuen_pet_t( sim_t* sim, monk_t* owner ) :
    pet_t( sim, owner, "xuen_the_white_tiger", true )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc.spell_scaling( o() -> type, level );
    main_hand_weapon.max_dmg    = dbc.spell_scaling( o() -> type, level );
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.0 );

    owner_coeff.ap_from_ap = 0.5;
  }

  monk_t* o() const { return static_cast<monk_t*>( owner ); }

  virtual void init_actions()
  {
    action_list_str = "auto_attack";
    action_list_str += "/crackling_tiger_lightning";

    pet_t::init_actions();
  }

  action_t* create_action( const std::string& name,
                           const std::string& options_str )
  {
    if ( name == "crackling_tiger_lightning" ) return new crackling_tiger_lightning_t( this, options_str );
    if ( name == "auto_attack" ) return new auto_attack_t( this, options_str );

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
struct monk_action_t : public Base
{
  int stancemask;

private:
  std::array < resource_e, WISE_SERPENT + 1 > _resource_by_stance;
  typedef Base ab; // action base, eg. spell_t
public:
  typedef monk_action_t base_t;

  monk_action_t( const std::string& n, monk_t* player,
                 const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s ),
    stancemask( STURDY_OX | FIERCE_TIGER | WISE_SERPENT )
  {
    ab::may_crit   = true;
    range::fill( _resource_by_stance, RESOURCE_MAX );
  }
  virtual ~monk_action_t() {}

  monk_t* p() const { return debug_cast<monk_t*>( ab::player ); }

  monk_td_t* td( player_t* t = 0 ) { return p() -> get_target_data( t ? t : ab::target ); }

  virtual bool ready()
  {
    if ( ! ab::ready() )
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
      const spellpower_data_t* pd = ( *ab::data()._power )[ i ];
      switch ( pd -> aura_id() )
      {
        case 103985:
          assert( _resource_by_stance[ FIERCE_TIGER ] == RESOURCE_MAX && "Two power entries per aura id." );
          _resource_by_stance[ FIERCE_TIGER ] = pd -> resource();
          break;
        case 115069:
          assert( _resource_by_stance[ STURDY_OX ] == RESOURCE_MAX && "Two power entries per aura id." );
          _resource_by_stance[ STURDY_OX ] = pd -> resource();
          break;
        case 115070:
          assert( _resource_by_stance[ WISE_SERPENT ] == RESOURCE_MAX && "Two power entries per aura id." );
          _resource_by_stance[ WISE_SERPENT ] = pd -> resource();
          break;
        default: break;
      }
    }
  }

  virtual resource_e current_resource()
  {
    resource_e resource_by_stance = _resource_by_stance[ p() -> current_stance() ];

    if ( resource_by_stance == RESOURCE_MAX )
      return ab::current_resource();

    return resource_by_stance;
  }

  virtual void consume_resource()
  {
    ab::consume_resource();

    // Handle Tigereye Brew and Mana Tea
    if ( ab::result_is_hit( ab::execute_state -> result ) )
    {
      // Track Chi Consumption
      if ( current_resource() == RESOURCE_CHI )
      {
        p() -> track_chi_consumption += ab::resource_consumed;
      }

      // Tigereye Brew
      if ( p() -> spec.brewing_tigereye_brew -> ok() )
      {
        int chi_to_consume = p() -> spec.brewing_tigereye_brew -> effectN( 1 ).base_value();

        if ( p() -> track_chi_consumption >= chi_to_consume )
        {
          p() -> track_chi_consumption -= chi_to_consume;

          p() -> buff.tigereye_brew -> trigger();

          if ( p() -> set_bonus.tier15_4pc_melee() &&
               p() -> rng.tier15_4pc_melee -> roll( p() -> sets -> set( SET_T15_4PC_MELEE ) -> effectN( 1 ).percent() ) )
          {
            p() -> buff.tigereye_brew -> trigger();
            p() -> proc.tier15_4pc_melee -> occur();
          }
        }
      }

      // Mana Tea
      if ( p() -> spec.brewing_mana_tea -> ok() )
      {
        int chi_to_consume = 4; // FIX ME: For some reason this value isn't in spell data...

        if ( p() -> track_chi_consumption >= chi_to_consume )
        {
          p() -> track_chi_consumption -= chi_to_consume;

          p() -> buff.mana_tea -> trigger();
          p() -> proc.mana_tea -> occur();

          if ( p() -> rng.mana_tea -> roll( p() -> composite_spell_crit() ) )
          {
            p() -> buff.mana_tea -> trigger();
          }
        }
      }
    }

    // Chi Savings on Dodge & Parry & Miss
    if ( current_resource() == RESOURCE_CHI && ab::resource_consumed > 0 && ! ab::aoe && ab::result_is_miss( ab::execute_state -> result ) )
    {
      double chi_restored = ab::resource_consumed;
      p() -> resource_gain( RESOURCE_CHI, chi_restored, p() -> gain.avoided_chi );
    }

    // Energy refund, estimated at 80%
    if ( current_resource() == RESOURCE_ENERGY && ab::resource_consumed > 0 && ab::result_is_miss( ab::execute_state -> result ) )
    {
      double energy_restored = ab::resource_consumed * 0.8;

      p() -> resource_gain( RESOURCE_ENERGY, energy_restored, p() -> gain.energy_refund );
    }
  }
};

namespace attacks {

struct monk_melee_attack_t : public monk_action_t<melee_attack_t>
{
  weapon_t* mh;
  weapon_t* oh;

  monk_melee_attack_t( const std::string& n, monk_t* player,
                       const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, player, s ),
    mh( NULL ), oh( NULL )
  {
    special = true;
    may_glance = false;
  }

  virtual double target_armor( player_t* t )
  {
    double a = base_t::target_armor( t );

    if ( p() -> buff.tiger_power -> up() )
      a *= 1.0 - p() -> buff.tiger_power -> check() * p() -> buff.tiger_power -> data().effectN( 1 ).percent();

    return a;
  }

  virtual void init()
  {
    base_t::init();

    if ( ! base_t::weapon && player -> weapon_racial( mh ) )
      base_attack_expertise += 0.01;
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
        sim -> output( "%s main hand weapon damage portion for %s: td=%.3f wd=%.3f bd=%.3f ws=%.3f ap=%.3f",
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
        sim -> output( "%s off-hand weapon damage portion for %s: td=%.3f wd=%.3f bd=%.3f ws=%.3f ap=%.3f",
                       player -> name(), name(), total_dmg, dmg, oh -> bonus_dmg, oh -> swing_time.total_seconds(), ap );
      }
    }

    if ( player -> dual_wield() )
      total_dmg *= 0.898882275;

    // All Brewmaster special abilities use a 0.4 modifier for some reason on
    // the base weapon damage, much like the Dual Wielding penalty
    if ( special && player -> specialization() == MONK_BREWMASTER )
      total_dmg *= 0.4;

    if ( ! mh && ! oh )
      total_dmg += base_t::calculate_weapon_damage( ap );
    else
      total_dmg += weapon_power_mod * ap;

    return total_dmg;
  }

  virtual double composite_target_multiplier( player_t* t )
  {
    double m = base_t::composite_target_multiplier( t );

    if ( special && td( t ) -> buff.rising_sun_kick -> up() )
    {
      m *=  1.0 + td( t ) -> buff.rising_sun_kick -> data().effectN( 1 ).percent();
    }

    return m;
  }
};

// ==========================================================================
// Jab
// ==========================================================================

struct jab_t : public monk_melee_attack_t
{
  static const spell_data_t* jab_data( monk_t* p )
  { return p -> specialization() == MONK_BREWMASTER ? p -> find_spell( 108557 ) : p -> find_class_spell( "Jab" ); }

  jab_t( monk_t* p, const std::string& options_str ) :
    monk_melee_attack_t( "jab", p, jab_data( p ) )
  {
    parse_options( nullptr, options_str );
    stancemask = STURDY_OX | FIERCE_TIGER | WISE_SERPENT;

    base_dd_min = base_dd_max = direct_power_mod = 0.0; // deactivate parsed spelleffect1

    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );

    base_multiplier = 1.5; // hardcoded into tooltip

    if ( player -> specialization() == MONK_BREWMASTER )
      weapon_power_mod = 1 / 11.0;
  }

  double combo_breaker_chance()
  {
    return p() -> spec.combo_breaker -> effectN( 1 ).percent();
  }

  virtual void impact( action_state_t* s )
  {
    monk_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> spec.muscle_memory -> ok() )
        p() -> buff.muscle_memory -> trigger();
    }
  }

  virtual void execute()
  {
    monk_melee_attack_t::execute();

    // Windwalker Mastery
    // Debuffs are independent of each other

    if ( result_is_miss( execute_state -> result ) )
      return;

    double cb_chance = combo_breaker_chance();
    p() -> buff.combo_breaker_bok -> trigger( 1, buff_t::DEFAULT_VALUE(), cb_chance );
    p() -> buff.combo_breaker_tp  -> trigger( 1, buff_t::DEFAULT_VALUE(), cb_chance );

    // Chi Gain
    double chi_gain = data().effectN( 2 ).base_value();
    chi_gain += p() -> active_stance_data( FIERCE_TIGER ).effectN( 4 ).base_value();

    if ( p() -> buff.power_strikes -> up() )
    {
      if ( p() -> resources.current[ RESOURCE_CHI ] + chi_gain < p() -> resources.max[ RESOURCE_CHI ] )
      {
        chi_gain += p() -> buff.power_strikes -> data().effectN( 1 ).base_value();
      }
      else
      {
        p() -> buff.chi_sphere -> trigger();
      }
      p() -> buff.power_strikes -> expire();
    }
    player -> resource_gain( RESOURCE_CHI, chi_gain, p() -> gain.chi );

    if ( p() -> set_bonus.tier15_2pc_melee() &&
         p() -> rng.tier15_2pc_melee -> roll( p() -> sets -> set( SET_T15_2PC_MELEE ) -> proc_chance() ) )
    {
      p() -> resource_gain( RESOURCE_ENERGY, p() -> passives.tier15_2pc -> effectN( 1 ).base_value(), p() -> gain.tier15_2pc );
      p() -> proc.tier15_2pc_melee -> occur();
    }
  }
};

// ==========================================================================
// Expel Harm
// ==========================================================================
/*
  Change to heal later.
*/

struct expel_harm_t : public monk_melee_attack_t
{
  expel_harm_t( monk_t* p, const std::string& options_str ) :
    monk_melee_attack_t( "expel_harm", p, p -> find_class_spell( "Expel Harm" ) )
  {
    parse_options( nullptr, options_str );
    stancemask = STURDY_OX | FIERCE_TIGER;

    base_dd_min = base_dd_max = direct_power_mod = 0.0; // deactivate parsed spelleffect1

    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );

    base_multiplier = 7.0; // hardcoded into tooltip

    if ( player -> specialization() == MONK_BREWMASTER )
      weapon_power_mod = 1 / 11.0;
  }

  virtual resource_e current_resource()
  {
    // Apparently energy requirement in Fierce Tiger stance is not in spell data
    if ( p() -> current_stance() == FIERCE_TIGER )
      return RESOURCE_ENERGY;

    return monk_melee_attack_t::current_resource();
  }

  virtual void execute()
  {
    monk_melee_attack_t::execute();

    // Chi Gain
    double chi_gain = data().effectN( 2 ).base_value();
    chi_gain += p() -> active_stance_data( FIERCE_TIGER ).effectN( 4 ).base_value();

    player -> resource_gain( RESOURCE_CHI, chi_gain, p() -> gain.chi );

    if ( p() -> set_bonus.tier15_2pc_melee() &&
         p() -> rng.tier15_2pc_melee -> roll( p() -> sets -> set( SET_T15_2PC_MELEE ) -> proc_chance() ) )
    {
      p() -> resource_gain( RESOURCE_ENERGY, p() -> passives.tier15_2pc -> effectN( 1 ).base_value(), p() -> gain.tier15_2pc );
      p() -> proc.tier15_2pc_melee -> occur();
    }
  }


  virtual void update_ready( timespan_t cd )
  {
    if ( p() -> spec.desperate_measures -> ok() && p() -> health_percentage() <= 35.0 )
      cd = timespan_t::zero();

    monk_melee_attack_t::update_ready( cd );
  }
};

// ==========================================================================
// Tiger Palm
// ==========================================================================

struct tiger_palm_t : public monk_melee_attack_t
{
  tiger_palm_t( monk_t* p, const std::string& options_str ) :
    monk_melee_attack_t( "tiger_palm", p, p -> find_class_spell( "Tiger Palm" ) )
  {
    parse_options( nullptr, options_str );
    stancemask = STURDY_OX | FIERCE_TIGER | WISE_SERPENT;
    base_dd_min = base_dd_max = direct_power_mod = 0.0;//  deactivate parsed spelleffect1
    mh = &( player -> main_hand_weapon ) ;
    oh = &( player -> off_hand_weapon ) ;
    base_multiplier = 3.0; // hardcoded into tooltip

    if ( p -> spec.brewmaster_training -> ok() )
      base_costs[ RESOURCE_CHI ] = 0.0;

    if ( player -> specialization() == MONK_BREWMASTER )
      weapon_power_mod = 1 / 11.0;
  }

  virtual double action_multiplier()
  {
    double m = monk_melee_attack_t::action_multiplier();

    if ( p() -> spec.teachings_of_the_monastery -> ok() )
      m *= 1.0 + p() -> spec.teachings_of_the_monastery -> effectN( 7 ).percent();
    if ( p() -> buff.muscle_memory -> check() )
      m *= 1.0 + 1.5; // FIX ME: Not reliant upon spell data! At time of writing spell only has a dummy effect.

    return m;
  }

  virtual void impact( action_state_t* s )
  {
    monk_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      p() -> buff.tiger_power -> trigger();

      if ( p() -> spec.brewmaster_training -> ok() )
        p() -> buff.power_guard -> trigger();
      if ( p() -> spec.teachings_of_the_monastery -> ok() )
        p() -> buff.vital_mists -> trigger();
    }
  }

  virtual void execute()
  {
    monk_melee_attack_t::execute();

    if ( p() -> buff.muscle_memory -> up() )
    {
      double mana_gain = player -> resources.max[ RESOURCE_MANA ] * 0.04; // FIX ME: Amount should be retrieved from spell data if possible.
      player -> resource_gain( RESOURCE_MANA, mana_gain, p() -> gain.muscle_memory );
      p() -> buff.muscle_memory -> decrement();
    }
  }

  virtual double cost()
  {
    if ( p() -> buff.combo_breaker_tp -> check() )
      return 0;

    return monk_melee_attack_t::cost();
  }

  virtual void consume_resource()
  {
    if ( p() -> buff.combo_breaker_tp -> check() && result_is_hit( execute_state -> result ) )
      p() -> track_chi_consumption += base_costs[ RESOURCE_CHI ];

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

struct dot_blackout_kick_t : public ignite::pct_based_action_t< monk_melee_attack_t >
{
  dot_blackout_kick_t( monk_t* p ) :
    base_t( "blackout_kick_dot", p, p -> find_spell( 128531 ) )
  {
    tick_may_crit = true;
    may_miss = false;
  }
};

struct blackout_kick_t : public monk_melee_attack_t
{
  void trigger_blackout_kick_dot( blackout_kick_t* s, player_t* t, double dmg )
  {
    monk_t* p = s -> p();

    ignite::trigger_pct_based(
      p -> active_actions.blackout_kick_dot,
      t,
      dmg );
  }

  blackout_kick_t( monk_t* p, const std::string& options_str ) :
    monk_melee_attack_t( "blackout_kick", p, p -> find_class_spell( "Blackout Kick" ) )
  {
    parse_options( nullptr, options_str );
    base_dd_min = base_dd_max = 0.0; direct_power_mod = 0.0; //  deactivate parsed spelleffect1
    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );
    base_multiplier = 8.0 * 0.89; // hardcoded into tooltip
    if ( player -> specialization() == MONK_BREWMASTER )
      weapon_power_mod = 1 / 11.0;

    if ( p -> spec.teachings_of_the_monastery -> ok() )
    {
      aoe = 1 + p -> spec.teachings_of_the_monastery -> effectN( 4 ).base_value();
      base_aoe_multiplier = p -> spec.teachings_of_the_monastery -> effectN( 5 ).percent();
    }
  }

  virtual void impact( action_state_t* s )
  {
    monk_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> spec.brewmaster_training -> ok() )
        p() -> buff.shuffle -> trigger();
      if ( p() -> spec.teachings_of_the_monastery -> ok() )
        p() -> buff.serpents_zeal -> trigger();
    }
  }

  virtual void execute()
  {
    monk_melee_attack_t::execute();

    if ( p() -> buff.muscle_memory -> up() )
    {
      double mana_gain = player -> resources.max[ RESOURCE_MANA ] * 0.04; // FIX ME: Amount should be retrieved from spell data if possible.
      player -> resource_gain( RESOURCE_MANA, mana_gain, p() -> gain.muscle_memory );
      p() -> buff.muscle_memory -> decrement();
    }
  }

  virtual double action_multiplier()
  {
    double m = monk_melee_attack_t::action_multiplier();

    if ( p() -> buff.muscle_memory -> check() )
      m *= 1.0 + 1.5; // FIX ME: Not reliant upon spell data! At time of writing spell only has a dummy effect.

    return m;
  }

  virtual void assess_damage( dmg_e type, action_state_t* s )
  {
    monk_melee_attack_t::assess_damage( type, s );

    if ( p() -> specialization() == MONK_WINDWALKER )
      trigger_blackout_kick_dot( this, s -> target, s -> result_amount * data().effectN( 2 ).percent( ) );
  }

  virtual double cost()
  {
    if ( p() -> buff.combo_breaker_bok -> check() )
      return 0.0;

    return monk_melee_attack_t::cost();
  }

  virtual void consume_resource()
  {
    if ( p() -> buff.combo_breaker_bok -> check() && result_is_hit( execute_state -> result ) )
      p() -> track_chi_consumption += base_costs[ RESOURCE_CHI ];

    monk_melee_attack_t::consume_resource();

    if ( p() -> buff.combo_breaker_bok -> up() )
    {
      p() -> buff.combo_breaker_bok -> expire();
      p() -> gain.combo_breaker_savings -> add( RESOURCE_CHI, cost() );
    }
  }
};

// ==========================================================================
// Rising Sun Kick
// ==========================================================================

struct rsk_debuff_t : public monk_melee_attack_t
{
  rsk_debuff_t( monk_t* p, const spell_data_t* s ) :
    monk_melee_attack_t( "rsk_debuff", p, s )
  {
    background  = true;
    dual        = true;
    aoe = -1;
    weapon_multiplier = 0;
  }

  virtual void impact ( action_state_t* s )
  {
    monk_melee_attack_t::impact( s );

    td( s -> target ) -> buff.rising_sun_kick -> trigger();
  }
};

struct rising_sun_kick_t : public monk_melee_attack_t
{
  rsk_debuff_t* rsk_debuff;

  rising_sun_kick_t( monk_t* p, const std::string& options_str ) :
    monk_melee_attack_t( "rising_sun_kick", p, p -> find_class_spell( "Rising Sun Kick" ) ),
    rsk_debuff( new rsk_debuff_t( p, p -> find_spell( 130320 ) ) )
  {
    parse_options( nullptr, options_str );
    stancemask = FIERCE_TIGER;
    base_dd_min = base_dd_max = direct_power_mod = 0.0;//  deactivate parsed spelleffect1
    mh = &( player -> main_hand_weapon ) ;
    oh = &( player -> off_hand_weapon ) ;
    base_multiplier = 14.4 * 0.89; // hardcoded into tooltip
  }

  virtual void impact ( action_state_t* s )
  {
    monk_melee_attack_t::impact( s );

    rsk_debuff -> execute();
  }
};

// ==========================================================================
// Spinning Crane Kick
// ==========================================================================
/*
  may need to modify this and fists of fury depending on how spell ticks
*/

struct spinning_crane_kick_t : public monk_melee_attack_t
{
  struct spinning_crane_kick_tick_t : public monk_melee_attack_t
  {
    spinning_crane_kick_tick_t( monk_t* p, const spell_data_t* s ) :
      monk_melee_attack_t( "spinning_crane_kick_tick", p, s )
    {
      background  = true;
      dual        = true;
      direct_tick = true;
      aoe = -1;
      base_dd_min = base_dd_max = 0.0; direct_power_mod = 0.0;//  deactivate parsed spelleffect1
      mh = &( player -> main_hand_weapon ) ;
      oh = &( player -> off_hand_weapon ) ;
      school = SCHOOL_PHYSICAL;

      if ( player -> specialization() == MONK_BREWMASTER )
        weapon_power_mod = 1 / 11.0;
    }
  };

  spinning_crane_kick_t( monk_t* p, const std::string& options_str ) :
    monk_melee_attack_t( "spinning_crane_kick", p, p -> find_class_spell( "Spinning Crane Kick" ) )
  {
    parse_options( nullptr, options_str );

    stancemask = STURDY_OX | FIERCE_TIGER;

    may_crit = false;
    tick_zero = true;
    channeled = true;
    hasted_ticks = true;
    school = SCHOOL_PHYSICAL;
    base_multiplier = 1.59 * 1.75 / 1.59; // hardcoded into tooltip

    tick_action = new spinning_crane_kick_tick_t( p, p -> find_spell( data().effectN( 1 ).trigger_spell_id() ) );
    dynamic_tick_action = true;
  }

  virtual resource_e current_resource()
  {
    // Apparently energy requirement in Fierce Tiger stance is not in spell data
    if ( p() -> current_stance() == FIERCE_TIGER )
      return RESOURCE_ENERGY;

    return monk_melee_attack_t::current_resource();
  }

  virtual double action_multiplier()
  {
    double m = monk_melee_attack_t::action_multiplier();

    if ( td( target ) -> buff.rushing_jade_wind -> up() )
      m *= 1.0 + p() -> talent.rushing_jade_wind -> effectN( 2 ).percent();

    return m;
  }

  virtual void execute()
  {
    monk_melee_attack_t::execute();

    double chi_gain = data().effectN( 4 ).base_value();
    player -> resource_gain( RESOURCE_CHI, chi_gain, p() -> gain.chi );

    if ( p() -> set_bonus.tier15_2pc_melee() &&
         p() -> rng.tier15_2pc_melee -> roll( p() -> sets -> set( SET_T15_2PC_MELEE ) -> proc_chance() ) )
    {
      p() -> resource_gain( RESOURCE_ENERGY, p() -> passives.tier15_2pc -> effectN( 1 ).base_value(), p() -> gain.tier15_2pc );
      p() -> proc.tier15_2pc_melee -> occur();
    }
  }
};

// ==========================================================================
// Fists of Fury
// ==========================================================================

struct fists_of_fury_t : public monk_melee_attack_t
{
  struct fists_of_fury_tick_t : public monk_melee_attack_t
  {
    fists_of_fury_tick_t( monk_t* p ) :
      monk_melee_attack_t( "fists_of_fury_tick", p )
    {
      background  = true;
      dual        = true;
      aoe = -1;
      direct_tick = true;
      base_dd_min = base_dd_max = direct_power_mod = 0.0;//  deactivate parsed spelleffect1
      mh = &( player -> main_hand_weapon ) ;
      oh = &( player -> off_hand_weapon ) ;

      split_aoe_damage = true;
      school = SCHOOL_PHYSICAL;
    }
  };

  fists_of_fury_t( monk_t* p, const std::string& options_str ) :
    monk_melee_attack_t( "fists_of_fury", p, p -> find_class_spell( "Fists of Fury" ) )
  {
    parse_options( nullptr, options_str );
    stancemask = FIERCE_TIGER;
    channeled = true;
    tick_zero = true;
    base_multiplier = 7.5 * 0.89; // hardcoded into tooltip
    school = SCHOOL_PHYSICAL;
    weapon_multiplier = 0;

    // T14 WW 2PC
    cooldown -> duration = data().cooldown();
    cooldown -> duration += p -> sets -> set( SET_T14_2PC_MELEE ) -> effectN( 1 ).time_value();

    tick_action = new fists_of_fury_tick_t( p );
    dynamic_tick_action = true;
  }
};

// ==========================================================================
// Tiger Strikes
// ==========================================================================

struct tiger_strikes_melee_attack_t : public monk_melee_attack_t
{
  tiger_strikes_melee_attack_t( const std::string& n, monk_t* p, weapon_t* w ) :
    monk_melee_attack_t( n, p, spell_data_t::nil()  )
  {
    weapon           = w;
    school           = SCHOOL_PHYSICAL;
    background       = true;
    special          = false;
    may_glance       = false;
    if ( player -> dual_wield() )
      base_multiplier *= 1.0 + p -> spec.way_of_the_monk -> effectN( 1 ).percent();
  }
};

// ==========================================================================
// Melee
// ==========================================================================

struct melee_t : public monk_melee_attack_t
{
  struct ts_delay_t : public event_t
  {
    melee_t* melee;

    ts_delay_t( monk_t* player, melee_t* m ) :
      event_t( player, "tiger_strikes_delay" ),
      melee( m )
    {
      sim.add_event( this, timespan_t::from_seconds( sim.gauss( 1.0, 0.2 ) ) );
    }

    void execute()
    {
      assert( melee );
      melee -> tsproc -> execute();
    }
  };


  int sync_weapons;
  tiger_strikes_melee_attack_t* tsproc;
  rng_t* elusive_brew;

  melee_t( const std::string& name, monk_t* player, int sw ) :
    monk_melee_attack_t( name, player, spell_data_t::nil() ),
    sync_weapons( sw ), tsproc( nullptr ), elusive_brew( nullptr )
  {
    background  = true;
    repeating   = true;
    trigger_gcd = timespan_t::zero();
    special     = false;
    school      = SCHOOL_PHYSICAL;
    may_glance  = true;

    if ( player -> dual_wield() )
    {
      base_hit -= 0.19;
      base_multiplier *= 1.0 + player -> spec.way_of_the_monk -> effectN( 1 ).percent();
    }
  }

  virtual timespan_t execute_time()
  {
    timespan_t t = monk_melee_attack_t::execute_time();
    if ( ! player -> in_combat )
    {
      return ( weapon -> slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t / 2, timespan_t::from_seconds( 0.2 ) ) : t / 2 ) : timespan_t::from_seconds( 0.01 );
    }
    return t;
  }

  void execute()
  {
    if ( time_to_execute > timespan_t::zero() && player -> executing )
    {
      if ( sim -> debug ) sim -> output( "Executing '%s' during melee (%s).", player -> executing -> name(), util::slot_type_string( weapon -> slot ) );
      schedule_execute();
    }
    else
    {
      if ( p() -> buff.tiger_strikes -> up() )
        new ( *sim ) ts_delay_t( p(), this );

      p() -> buff.tiger_strikes -> decrement();

      monk_melee_attack_t::execute();
    }
  }

  void init()
  {
    monk_melee_attack_t::init();

    tsproc = new tiger_strikes_melee_attack_t( "tiger_strikes_melee", p(), weapon );
    assert( tsproc );

    if ( p() -> spec.brewing_elusive_brew -> ok() )
    {
      elusive_brew = p() -> get_rng( "Elusive Brew" );
    }
  }

  virtual void impact( action_state_t* s )
  {
    monk_melee_attack_t::impact( s );

    // FIX ME: tsproc should have a significant delay after the auto attack
    // tsproc should consume the buff.
    // If you refresh TS, the buff goes to 4 stacks and a tsproc will happen
    // up to 1200ms later and consume the buff (so you'll basically lose one
    // stack)
    /*    if ( p() -> buff.tiger_strikes -> up() )
        {
          tsproc -> execute();
          p() -> buff.tiger_strikes -> decrement();
        }
        else
        {*/
    p() -> buff.tiger_strikes -> trigger( 4 );
//    }

    if ( p() -> spec.brewing_elusive_brew -> ok() && s -> result == RESULT_CRIT )
    {
      trigger_elusive_brew();
    }
  }


  /* Trigger buff.elusive_brew_stacks with the correct #stacks calculated from weapon type & speed
   */
  void trigger_elusive_brew()
  {
    // Formula taken from http://www.wowhead.com/spell=128938  2013/04/15
    double expected_stacks, low_chance;
    int low, high;

    // Calculate expected #stacks
    if ( weapon -> group() == WEAPON_1H || weapon -> group() == WEAPON_SMALL )
      expected_stacks = 1.5 * weapon -> swing_time.total_seconds() / 2.6;
    else
      expected_stacks = 3.0 * weapon -> swing_time.total_seconds() / 3.6;
    expected_stacks = clamp( expected_stacks, 1.0, 3.0 );

    // Low and High together need to achieve expected_stacks through rng
    low = as<int>( util::floor( expected_stacks ) );
    high = as<int>( util::ceil( expected_stacks ) );

    // Solve low * low_chance + high * ( 1 - low_chance ) = expected_stacks
    if ( high > low )
      low_chance = ( high - expected_stacks ) / ( high - low );
    else
      low_chance = 0.0;
    assert( low_chance >= 0.0 && low_chance <= 1.0 && "elusive brew proc chance out of bounds" );

    // Proc Buff
    if ( elusive_brew -> roll( low_chance ) )
      p() -> buff.elusive_brew_stacks -> trigger( low );
    else
      p() -> buff.elusive_brew_stacks -> trigger( high );
  }
};

// ==========================================================================
// Auto Attack
// ==========================================================================

struct auto_attack_t : public monk_melee_attack_t
{
  int sync_weapons;

  auto_attack_t( monk_t* player, const std::string& options_str ) :
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
      if ( ! player -> dual_wield() ) return;

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

  virtual bool ready()
  {
    if ( p() -> is_moving() ) return false;

    return ( p() -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// ==========================================================================
// Keg Smash
// ==========================================================================

struct keg_smash_t : public monk_melee_attack_t
{
  const spell_data_t* chi_generation;
  bool apply_dizzying_haze;

  keg_smash_t( monk_t& p, const std::string& options_str ) :
    monk_melee_attack_t( "keg_smash", &p, p.find_class_spell( "Keg Smash" ) ),
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
    mh = &( player -> main_hand_weapon ) ;
    oh = &( player -> off_hand_weapon ) ;

    base_multiplier = 8.12; // hardcoded into tooltip
    base_multiplier *= 1.5; // Unknown 1.5 modifier applied at some point in time
    weapon_power_mod = 1 / 11.0; // BM AP -> DPS conversion is with ap/11
  }

  virtual void execute()
  {
    monk_melee_attack_t::execute();

    player -> resource_gain( RESOURCE_CHI,
                             chi_generation -> effectN( 1 ).resource( RESOURCE_CHI ),
                             p() -> gain.chi,
                             this );
  }

  virtual void impact( action_state_t* s )
  {
    monk_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( apply_dizzying_haze )
        td( s -> target ) -> buff.dizzying_haze -> trigger();

      if ( ! sim -> overrides.weakened_blows )
        s -> target -> debuffs.weakened_blows -> trigger();
    }
  }
};

} // END melee_attacks NAMESPACE

namespace spells {
struct monk_spell_t : public monk_action_t<spell_t>
{
  monk_spell_t( const std::string& n, monk_t* player,
                const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, player, s )
  {
  }

  virtual double composite_target_multiplier( player_t* t )
  {
    double m = base_t::composite_target_multiplier( t );

    if ( td( t ) -> buff.rising_sun_kick -> check() )
    {
      m *= 1.0 + td( t ) -> buff.rising_sun_kick -> data().effectN( 1 ).percent();
    }

    return m;
  }
};

// ==========================================================================
// Stance
// ==========================================================================

struct stance_t : public monk_spell_t
{
  stance_e switch_to_stance;
  std::string stance_str;

  stance_t( monk_t* p, const std::string& options_str ) :
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
      if ( ! stance_str.empty() )
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

  virtual void execute()
  {
    monk_spell_t::execute();

    p() -> switch_to_stance( switch_to_stance );
  }

  virtual bool ready()
  {
    if ( p() -> current_stance() == switch_to_stance )
      return false;

    return monk_spell_t::ready();
  }
};

// ==========================================================================
// Tigereye Brew
// ==========================================================================

struct tigereye_brew_t : public monk_spell_t
{
  tigereye_brew_t( monk_t* player, const std::string& options_str ) :
    monk_spell_t( "tigereye_brew", player, player -> find_spell( 116740 ) )
  {
    parse_options( nullptr, options_str );

    harmful = false;
  }

  double value()
  {
    double v = p() -> buff.tigereye_brew_use -> data().effectN( 1 ).percent();

    if ( p() -> mastery.bottled_fury -> ok() )
      v += p() -> cache.mastery_value();

    return v;
  }

  virtual void execute()
  {
    monk_spell_t::execute();

    int max_stacks_consumable = p() -> spec.brewing_tigereye_brew -> effectN( 2 ).base_value();

    double use_value = value() * std::min( p() -> buff.tigereye_brew -> stack(), max_stacks_consumable );
    p() -> buff.tigereye_brew_use -> trigger( 1, use_value );
    p() -> buff.tigereye_brew -> decrement( max_stacks_consumable );
  }
};

// ==========================================================================
// Energizing Brew
// ==========================================================================

struct energizing_brew_t : public monk_spell_t
{
  energizing_brew_t( monk_t* player, const std::string& options_str ) :
    monk_spell_t( "energizing_brew", player, player -> find_spell( 115288 ) )
  {
    parse_options( nullptr, options_str );

    harmful   = false;
    num_ticks = 0;
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

struct chi_brew_t : public monk_spell_t
{
  chi_brew_t( monk_t* player, const std::string& options_str ) :
    monk_spell_t( "chi_brew", player, player -> talent.chi_brew )
  {
    parse_options( nullptr, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    monk_spell_t::execute();

    double chi_gain = data().effectN( 1 ).base_value();
    player -> resource_gain( RESOURCE_CHI, chi_gain, p() -> gain.chi_brew );
  }
};

// ==========================================================================
// Zen Sphere
// ==========================================================================

struct zen_sphere_damage_t : public monk_spell_t
{
  zen_sphere_damage_t( monk_t* player ) :
    monk_spell_t( "zen_sphere_damage", player, player -> dbc.spell( 124098 ) )
  {
    background  = true;
    base_attack_power_multiplier = 1.0;
    direct_power_mod = data().extra_coeff();
    dual = true;
  }
};
//NYI
struct zen_sphere_detonate_t : public monk_spell_t
{
  zen_sphere_detonate_t( monk_t* player, const std::string& options_str ) :
    monk_spell_t( "zen_sphere_detonate", player, player -> find_spell( 125033 ) )
  {
    parse_options( nullptr, options_str );
    aoe = -1;
  }

  virtual void execute()
  {
    monk_spell_t::execute();
    p() -> buff.zen_sphere -> expire();
  }
};

// ==========================================================================
// Spinning Fire Blossoms
// ==========================================================================

struct spinning_fire_blossom_t : public monk_spell_t
{
  spinning_fire_blossom_t( monk_t* player, const std::string& options_str  ) :
    monk_spell_t( "spinning_fire_blossom", player, player -> find_spell( 115073 ) )
  {
    parse_options( nullptr, options_str );

    direct_power_mod = data().extra_coeff();
    base_attack_power_multiplier = 1.0;
    base_spell_power_multiplier = 0.0;
  }

  virtual double composite_target_da_multiplier( player_t* t )
  {
    double m = monk_spell_t::composite_target_da_multiplier( t );

    if ( player -> current.distance >= 10.0 ) // replace with target-dependant range check
      m *= 1.5; // hardcoded into tooltip

    return m;
  }
};

// ==========================================================================
// Chi Wave
// ==========================================================================
/*
 * TODO: FOR REALISTIC BOUNCING, IT WILL BOUNCE ENEMY -> MONK -> ENEMY -> MONK -> ENEMY but on dummies it hits enemy then monk and stops.
 * So only 3 ticks will occur in a single target simming scenario. Alternate scenarios need to be determined.
 * TODO: Need to add decrementing buff to handle bouncing mechanic. .561 coeff
 * verify damage
*/
struct chi_wave_t : public monk_spell_t
{
  struct direct_damage_t : public monk_spell_t
  {
    direct_damage_t( monk_t* p ) :
      monk_spell_t( "chi_wave_dd", p, p -> find_spell( 132467 ) )
    {
      direct_power_mod = 0.45; // hardcoded into tooltip of 115098

      base_attack_power_multiplier = 1.0;
      base_spell_power_multiplier = 0.0;
      background = true;
    }
  };

  chi_wave_t( monk_t* player, const std::string& options_str  ) :
    monk_spell_t( "chi_wave", player, player -> talent.chi_wave )
  {
    parse_options( nullptr, options_str );
    num_ticks = 3;
    hasted_ticks   = false;
    base_tick_time = timespan_t::from_seconds( 1.5 );

    direct_power_mod = base_dd_min = base_dd_max = 0;

    special = false;

    tick_action = new direct_damage_t( player );
  }
};

// ==========================================================================
// Chi Burst
// ==========================================================================

struct chi_burst_t : public monk_spell_t
{
  chi_burst_t( monk_t* player, const std::string& options_str  ) :
    monk_spell_t( "chi_burst", player, player -> talent.chi_burst )
  {
    parse_options( nullptr, options_str );
    aoe = -1;
    special = false; // Disable pausing of auto attack while casting this spell
    base_attack_power_multiplier = 1.0;
    base_spell_power_multiplier = 0.0;
    direct_power_mod = 1.21; // hardcoded into tooltip 2013/04/10
    base_dd_min = player -> find_spell( 130651 ) -> effectN( 1 ).min( player );
    base_dd_max = player -> find_spell( 130651 ) -> effectN( 1 ).max( player );
  }
};

// ==========================================================================
// Rushing Jade Wind
// ==========================================================================

struct rushing_jade_wind_t : public monk_spell_t
{
  rushing_jade_wind_t( monk_t* player, const std::string& options_str  ) :
    monk_spell_t( "rushing_jade_wind", player, player -> talent.rushing_jade_wind )
  {
    parse_options( nullptr, options_str );
    aoe = -1;
    procs_courageous_primal_diamond = false;

    direct_power_mod = data().extra_coeff();
    base_attack_power_multiplier = 1.0;
    base_spell_power_multiplier = 0.0;
  }

  virtual void impact( action_state_t* s )
  {
    monk_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
      td( s -> target ) -> buff.rushing_jade_wind -> trigger();
  }
};

// ==========================================================================
// Chi Torpedo
// ==========================================================================

struct chi_torpedo_t : public monk_spell_t
{
  chi_torpedo_t( monk_t* player, const std::string& options_str  ) :
    monk_spell_t( "chi_torpedo", player, player -> talent.chi_torpedo -> ok() ? player -> find_spell( 117993 ) : spell_data_t::not_found() )
  {
    parse_options( nullptr, options_str );
    aoe = -1;
    direct_power_mod = data().extra_coeff();
    base_attack_power_multiplier = 1.0;
    base_spell_power_multiplier = 0.0;
  }
};

struct summon_pet_t : public monk_spell_t
{
  timespan_t summoning_duration;
  pet_t* pet;

public:
  summon_pet_t( const std::string& n, const std::string& pet_name, monk_t* p, const spell_data_t* sd = spell_data_t::nil() ) :
    monk_spell_t( n, p, sd ),
    summoning_duration ( timespan_t::zero() ),
    pet( p -> find_pet( pet_name ) )
  {
    harmful = false;

    if ( ! pet )
    {
      sim -> errorf( "Player %s unable to find pet %s for summons.\n", player -> name(), pet_name.c_str() );
    }
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

struct xuen_spell_t : public summon_pet_t
{
  xuen_spell_t( monk_t* p, const std::string& options_str ) :
    summon_pet_t( "invoke_xuen", "xuen_the_white_tiger", p, p -> talent.invoke_xuen ) //123904
  {
    parse_options( nullptr, options_str );

    harmful = false;
    summoning_duration = data().duration();
  }
};

struct chi_sphere_t : public monk_spell_t
{
  chi_sphere_t( monk_t* p, const std::string& options_str  ) :
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
      player -> resource_gain( RESOURCE_CHI, 1, p() -> gain.chi );

      p() -> buff.chi_sphere -> decrement();
    }
  }

};

// ==========================================================================
// Breath of Fire
// ==========================================================================

struct breath_of_fire_t : public monk_spell_t
{
  struct periodic_t : public monk_spell_t
  {
    periodic_t( monk_t& p ) :
      monk_spell_t( "breath_of_fire_dot", &p, p.find_spell( 123725 ) )
    {
      background = true;
      base_attack_power_multiplier = 1.0;
      base_spell_power_multiplier = 0.0;
      direct_power_mod = data().extra_coeff();
    }
  };
  periodic_t* dot_action;

  breath_of_fire_t( monk_t& p, const std::string& options_str  ) :
    monk_spell_t( "breath_of_fire", &p, p.find_class_spell( "Breath of Fire" ) ),
    dot_action( new periodic_t( p ) )
  {
    parse_options( nullptr, options_str );

    aoe = -1;
    stancemask = STURDY_OX;
    base_attack_power_multiplier = 1.0;
    base_spell_power_multiplier = 0.0;
    direct_power_mod = data().extra_coeff();
  }

  virtual void impact( action_state_t* s )
  {
    monk_spell_t::impact( s );

    monk_td_t& td = *this -> td( s -> target );
    if ( td.buff.dizzying_haze -> up() )
    {
      dot_action -> execute();
    }
  }
};

// ==========================================================================
// Dizzying Haze
// ==========================================================================

struct dizzying_haze_t : public monk_spell_t
{
  dizzying_haze_t( monk_t& p, const std::string& options_str  ) :
    monk_spell_t( "dizzying_haze", &p, p.find_class_spell( "Dizzying Haze" ) )
  {
    parse_options( nullptr, options_str );

    aoe = -1;
    stancemask = STURDY_OX;
    ability_lag = timespan_t::from_seconds( 0.5 ); // ground target malus
  }

  virtual void impact( action_state_t* s )
  {
    monk_spell_t::impact( s );

    td( s -> target ) -> buff.dizzying_haze -> trigger();
  }
};

// ==========================================================================
// Fortifying Brew
// ==========================================================================

struct fortifying_brew_t : public monk_spell_t
{
  fortifying_brew_t( monk_t& p, const std::string& options_str  ) :
    monk_spell_t( "fortifying_brew", &p, p.find_class_spell( "Fortifying Brew" ) )
  {
    parse_options( nullptr, options_str );

    harmful = false;
    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    monk_spell_t::execute();

    p() -> buff.fortifying_brew -> trigger();

    // Extra Health is set by current max_health, doesn't change when max_health changes.
    double health_gain = player -> resources.max[ RESOURCE_HEALTH ];
    health_gain *= p() -> glyph.fortifying_brew -> ok() ? 0.10 : 0.20; // hardcoded into spelldata 2013/04/10
    player -> stat_gain( STAT_MAX_HEALTH, health_gain, nullptr, this );
  }
};

// ==========================================================================
// Elusive Brew
// ==========================================================================

struct elusive_brew_t : public monk_spell_t
{
  elusive_brew_t( monk_t& p, const std::string& options_str  ) :
    monk_spell_t( "elusive_brew", &p, p.spec.elusive_brew )
  {
    parse_options( nullptr, options_str );

    stancemask = STURDY_OX;
    harmful = false;
    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    monk_spell_t::execute();

    p() -> buff.elusive_brew_activated -> trigger( 1,
                                                   p() -> buff.elusive_brew_stacks -> check() );
    p() -> buff.elusive_brew_stacks -> expire();
  }
};

// ==========================================================================
// Mana Tea
// ==========================================================================
/*
  FIX ME: Current behavior resembles the glyphed ability
          regardless of if the glyph is actually present.
*/
struct mana_tea_t : public monk_spell_t
{
  mana_tea_t( monk_t& p, const std::string& options_str  ) :
    monk_spell_t( "mana_tea", &p, p.find_class_spell( "Mana Tea" ) )
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

    double mana_gain = player -> resources.max[ RESOURCE_MANA ]
                       * data().effectN( 1 ).percent()
                       * std::min( p() -> buff.mana_tea -> stack(), max_stacks_consumable );
    player -> resource_gain( RESOURCE_MANA, mana_gain, p() -> gain.mana_tea );
    p() -> buff.mana_tea -> decrement( max_stacks_consumable );
  }
};

struct stagger_self_damage_t : public ignite::pct_based_action_t<monk_spell_t>
{
  stagger_self_damage_t( monk_t* p ) :
    base_t( "stagger_self_damage", p, p -> find_spell( 124255 ) )
  {
    num_ticks = 10;
    target = p;
  }

  virtual void init()
  {
    base_t::init();

    // We don't want this counted towards our dps
    stats -> type = STATS_NEUTRAL;
  }
};

} // END spells NAMESPACE

namespace heals {

struct monk_heal_t : public monk_action_t<heal_t>
{
  monk_heal_t( const std::string& n, monk_t& p,
               const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, &p, s )
  {
  }
};

// ==========================================================================
// Zen Sphere
// ==========================================================================
/*
  TODO:
   - Add healing Component
   - find out if direct tick or tick zero applies
*/

struct zen_sphere_t : public monk_heal_t
{
  spells::monk_spell_t* zen_sphere_damage;

  zen_sphere_t( monk_t& p, const std::string& options_str  ) :
    monk_heal_t( "zen_sphere", p, p.talent.zen_sphere ),
    zen_sphere_damage( new spells::zen_sphere_damage_t( &p ) )
  {
    parse_options( nullptr, options_str );

    tick_power_mod = data().extra_coeff();
  }

  virtual void execute()
  {
    monk_heal_t::execute();

    p() -> buff.zen_sphere -> trigger();

    zen_sphere_damage -> stats -> add_execute( time_to_execute, target );
  }

  virtual void tick( dot_t* d )
  {
    monk_heal_t::tick( d );

    zen_sphere_damage -> execute();
  }

  virtual bool ready()
  {
    if ( p() -> buff.zen_sphere -> check() )
      return false; // temporary to hold off on action

    return monk_heal_t::ready();
  }
};

// ==========================================================================
// Enveloping Mist
// ==========================================================================

struct enveloping_mist_t : public monk_heal_t
{
  enveloping_mist_t( monk_t& p, const std::string& options_str ) :
    monk_heal_t( "zen_sphere_detonate", p, p.find_class_spell( "Enveloping Mist" ) )
  {
    parse_options( nullptr, options_str );

    stancemask = WISE_SERPENT;
  }

  virtual void impact( action_state_t* s )
  {
    monk_heal_t::impact( s );

    td( s -> target ) -> buff.enveloping_mist -> trigger();
  }
};

// ==========================================================================
// Soothing Mist
// ==========================================================================

struct soothing_mist_t : public monk_heal_t
{
  rng_t* chi_gain;

  soothing_mist_t( monk_t& p, const std::string& options_str ) :
    monk_heal_t( "soothing_mist", p, p.find_specialization_spell( "Soothing Mist" ) ),
    chi_gain( p.get_rng( "soothing_mist" ) )
  {
    parse_options( nullptr, options_str );

    stancemask = WISE_SERPENT;

    channeled = true;
  }

  virtual void tick( dot_t* d )
  {
    monk_heal_t::tick( d );

    if ( chi_gain -> roll( data().proc_chance() ) )
    {
      p() -> resource_gain( RESOURCE_CHI, 1, p() -> gain.soothing_mist, this );
    }
  }
};

} // end namespace heals

namespace absorbs {
struct monk_absorb_t : public monk_action_t<absorb_t>
{
  monk_absorb_t( const std::string& n, monk_t& player,
                 const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, &player, s )
  {
  }
};

// ==========================================================================
// Guard
// ==========================================================================

struct guard_t : public monk_absorb_t
{
  guard_t( monk_t& p, const std::string& options_str  ) :
    monk_absorb_t( "guard", p, p.find_class_spell( "Guard" ) )
  {
    parse_options( nullptr, options_str );
    harmful = false;
    trigger_gcd = timespan_t::zero();
    target = &p;

    base_attack_power_multiplier = 1.0;
    base_spell_power_multiplier = 0.0;
    direct_power_mod = 1.971; // hardcoded into tooltip 2013/04/10
  }

  virtual void impact( action_state_t* s )
  {
    p() -> buff.guard -> trigger( 1, s -> result_amount );
    stats -> add_result( 0.0, s -> result_amount, ABSORB, s -> result, s -> target );
  }

  virtual void execute()
  {
    p() -> buff.power_guard -> up();
    monk_absorb_t::execute();
    p() -> buff.power_guard -> expire();
  }

  virtual double action_multiplier()
  {
    double am = monk_absorb_t::action_multiplier();

    if ( p() -> buff.power_guard -> check() )
      am *= 1.0 + p() -> buff.power_guard -> data().effectN( 1 ).percent();

    return am;
  }
};

} // end namespace asborbs

using namespace attacks;
using namespace spells;
using namespace heals;
using namespace absorbs;

} // end namespace actions;

struct power_strikes_event_t : public event_t
{
  power_strikes_event_t( monk_t* player, timespan_t tick_time ) :
    event_t( player, "power_strikes" )
  {
    // Safety clamp
    tick_time = clamp( tick_time, timespan_t::zero(), timespan_t::from_seconds( 20 ) );
    sim.add_event( this, tick_time );
  }

  virtual void execute()
  {
    monk_t* p = debug_cast<monk_t*>( player );

    p -> buff.power_strikes -> trigger();

    new ( sim ) power_strikes_event_t( p, timespan_t::from_seconds( 20.0 ) );
  }
};

// ==========================================================================
// Monk Character Definition
// ==========================================================================

monk_td_t::monk_td_t( player_t* target, monk_t* p ) :
  actor_pair_t( target, p ),
  buff( buffs_t() )
{
  buff.rising_sun_kick   = buff_creator_t( *this, "rising_sun_kick"   ).spell( p -> find_spell( 130320 ) );
  buff.enveloping_mist   = buff_creator_t( *this, "enveloping_mist"   ).spell( p -> find_class_spell( "Enveloping Mist" ) );
  buff.dizzying_haze     = buff_creator_t( *this, "dizzying_haze" ).spell( p -> find_spell( 123727 ) );
  buff.rushing_jade_wind = buff_creator_t( *this, "rushing_jade_wind", p ->  talent.rushing_jade_wind -> effectN( 2 ).trigger() );
}

// monk_t::create_action ====================================================

action_t* monk_t::create_action( const std::string& name,
                                 const std::string& options_str )
{
  using namespace actions;

  // Melee Attacks
  if ( name == "auto_attack"           ) return new            auto_attack_t( this, options_str );
  if ( name == "jab"                   ) return new                    jab_t( this, options_str );
  if ( name == "expel_harm"            ) return new             expel_harm_t( this, options_str );
  if ( name == "tiger_palm"            ) return new             tiger_palm_t( this, options_str );
  if ( name == "blackout_kick"         ) return new          blackout_kick_t( this, options_str );
  if ( name == "spinning_crane_kick"   ) return new    spinning_crane_kick_t( this, options_str );
  if ( name == "fists_of_fury"         ) return new          fists_of_fury_t( this, options_str );
  if ( name == "rising_sun_kick"       ) return new        rising_sun_kick_t( this, options_str );
  if ( name == "stance"                ) return new                 stance_t( this, options_str );
  if ( name == "tigereye_brew"         ) return new          tigereye_brew_t( this, options_str );
  if ( name == "energizing_brew"       ) return new        energizing_brew_t( this, options_str );
  if ( name == "spinning_fire_blossom" ) return new  spinning_fire_blossom_t( this, options_str );

  // Brewmaster
  if ( name == "breath_of_fire"        ) return new         breath_of_fire_t( *this, options_str );
  if ( name == "keg_smash"             ) return new              keg_smash_t( *this, options_str );
  if ( name == "dizzying_haze"         ) return new          dizzying_haze_t( *this, options_str );
  if ( name == "guard"                 ) return new                  guard_t( *this, options_str );
  if ( name == "fortifying_brew"       ) return new        fortifying_brew_t( *this, options_str );
  if ( name == "elusive_brew"          ) return new           elusive_brew_t( *this, options_str );

  // Mistweaver
  if ( name == "enveloping_mist"       ) return new        enveloping_mist_t( *this, options_str );
  if ( name == "soothing_mist"         ) return new          soothing_mist_t( *this, options_str );
  if ( name == "mana_tea"              ) return new               mana_tea_t( *this, options_str );

  // Talents
  if ( name == "chi_sphere"            ) return new             chi_sphere_t( this, options_str ); // For Power Strikes
  if ( name == "chi_brew"              ) return new               chi_brew_t( this, options_str );

  if ( name == "zen_sphere"            ) return new             zen_sphere_t( *this, options_str );
  if ( name == "zen_sphere_detonate"   ) return new    zen_sphere_detonate_t( this, options_str );
  if ( name == "chi_wave"              ) return new               chi_wave_t( this, options_str );
  if ( name == "chi_burst"             ) return new              chi_burst_t( this, options_str );

  if ( name == "rushing_jade_wind"     ) return new      rushing_jade_wind_t( this, options_str );
  if ( name == "invoke_xuen"           ) return new             xuen_spell_t( this, options_str );
  if ( name == "chi_torpedo"           ) return new            chi_torpedo_t( this, options_str );

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

  return nullptr;
}

// monk_t::create_pets ======================================================

void monk_t::create_pets()
{
  base_t::create_pets();

  create_pet( "xuen_the_white_tiger" );
}

// monk_t::init_spells ======================================================

void monk_t::init_spells()
{
  base_t::init_spells();

  //TALENTS
  talent.ascension                = find_talent_spell( "Ascension" );
  talent.zen_sphere               = find_talent_spell( "Zen Sphere" );
  talent.invoke_xuen              = find_talent_spell( "Invoke Xuen, the White Tiger", "invoke_xuen" ); //find_spell( 123904 );
  talent.chi_wave                 = find_talent_spell( "Chi Wave" );
  talent.chi_burst                = find_talent_spell( "Chi Burst" );
  talent.chi_brew                 = find_talent_spell( "Chi Brew" );
  talent.rushing_jade_wind        = find_talent_spell( "Rushing Jade Wind" );
  talent.chi_torpedo              = find_talent_spell( "Chi Torpedo" );
  talent.power_strikes            = find_talent_spell( "Power Strikes" );

  // General Passives
  spec.way_of_the_monk            = find_spell( 108977 );
  spec.leather_specialization     = find_specialization_spell( "Leather Specialization" );

  // Windwalker Passives
  spec.brewing_tigereye_brew      = find_specialization_spell( "Brewing: Tigereye Brew" );
  spec.combo_breaker              = find_specialization_spell( "Combo Breaker" );

  // Brewmaster Passives
  spec.brewmaster_training        = find_specialization_spell( "Brewmaster Training" );
  spec.brewing_elusive_brew       = find_specialization_spell( "Brewing: Elusive Brew" );
  spec.elusive_brew               = find_specialization_spell( "Elusive Brew" );
  spec.desperate_measures         = find_specialization_spell( "Desperate Measures" );

  // Mistweaver Passives
  spec.brewing_mana_tea           = find_specialization_spell( "Brewing: Mana Tea" );
  spec.mana_meditation            = find_specialization_spell( "Mana Meditation" );
  spec.muscle_memory              = find_specialization_spell( "Muscle Memory" );
  spec.teachings_of_the_monastery = find_specialization_spell( "Teachings of the Monastery" );

  // Stance
  stance_data.fierce_tiger        = find_class_spell( "Stance of the Fierce Tiger" );
  stance_data.sturdy_ox           = find_specialization_spell( "Stance of the Sturdy Ox" );
  stance_data.wise_serpent        = find_specialization_spell( "Stance of the Wise Serpent" );

  //SPELLS
  active_actions.blackout_kick_dot = new actions::dot_blackout_kick_t( this );

  if ( specialization() == MONK_BREWMASTER )
    active_actions.stagger_self_damage = new actions::stagger_self_damage_t( this );

  passives.tier15_2pc = find_spell( 138311 );
  passives.swift_reflexes = find_spell( 124334 );

  // GLYPHS
  glyph.fortifying_brew = find_glyph( "Glyph of Fortifying Brew" );

  //MASTERY
  mastery.bottled_fury        = find_mastery_spell( MONK_WINDWALKER );
  mastery.elusive_brawler     = find_mastery_spell( MONK_BREWMASTER );
  mastery.gift_of_the_serpent = find_mastery_spell( MONK_MISTWEAVER );

  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //    C2P      C4P     M2P     M4P     T2P     T4P     H2P     H4P
    {       0,       0,      0,      0,      0,      0,      0,      0 }, // Tier13
    {       0,       0, 123149, 123150, 123157, 123159, 123152, 123153 }, // Tier14
    {       0,       0, 138177, 138315, 138231, 138236, 138289, 138290 }, // Tier15
  };

  sets = new set_bonus_array_t( this, set_bonuses );


  // Holy Mastery uses effect#2 by default
  if ( specialization() == MONK_WINDWALKER )
  {
    _mastery = &find_mastery_spell( specialization() ) -> effectN( 3 );
  }
}

// monk_t::init_base ========================================================

void monk_t::init_base_stats()
{
  base_t::init_base_stats();

  base.distance = ( specialization() == MONK_MISTWEAVER ) ? 40 : 3;

  base_gcd = timespan_t::from_seconds( 1.0 );

  resources.base[  RESOURCE_CHI  ] = 4 + talent.ascension -> effectN( 1 ).base_value();
  resources.base[ RESOURCE_ENERGY ] = 100;
  resources.base_multiplier[ RESOURCE_MANA ] *= 1.0 + talent.ascension -> effectN( 2 ).percent();

  base_chi_regen_per_second = 0;
  base_energy_regen_per_second = 10.0;

  base.stats.attack_power = level * 2.0;
  base.attack_power_per_strength = 1.0;
  base.attack_power_per_agility  = 2.0;
  base.spell_power_per_intellect = 1.0;

  // Mistweaver
  if ( spec.mana_meditation -> ok() )
    base.mana_regen_from_spirit_multiplier = spec.mana_meditation -> effectN( 1 ).percent();

  // FIXME! Level-specific!
  base.miss  = 0.060;
  base.dodge = 0.1176;  //30
  base.parry = 0.080; //30

  // FIXME: Add defensive constants
  //diminished_kfactor    = 0;
  //diminished_dodge_cap = 0;
  //diminished_parry_cap = 0;
}

// monk_t::init_scaling =====================================================

void monk_t::init_scaling()
{
  base_t::init_scaling();

  if ( specialization() != MONK_MISTWEAVER )
  {
    scales_with[ STAT_INTELLECT             ] = false;
    scales_with[ STAT_SPIRIT                ] = false;
    scales_with[ STAT_SPELL_POWER           ] = false;
  }

  if ( off_hand_weapon.type != WEAPON_NONE )
  {
    scales_with[ STAT_WEAPON_OFFHAND_DPS   ] = true;
    scales_with[ STAT_WEAPON_OFFHAND_SPEED ] = sim -> weapon_speed_scale_factors != 0;
    scales_with[ STAT_HIT_RATING2          ] = true;
  }
}

// monk_t::init_buffs =======================================================

void monk_t::create_buffs()
{
  base_t::create_buffs();

  // General
  buff.fortifying_brew   = buff_creator_t( this, "fortifying_brew"     ).spell( find_spell( 120954 ) );
  buff.power_strikes     = buff_creator_t( this, "power_strikes"       ).spell( find_spell( 129914 ) );
  buff.tiger_strikes     = haste_buff_creator_t( this, "tiger_strikes" ).spell( find_spell( 120273 ) )
                           .chance( find_specialization_spell( "Tiger Strikes" ) -> proc_chance() );
  buff.tiger_power       = buff_creator_t( this, "tiger_power" )
                           .spell( find_class_spell( "Tiger Palm" ) -> effectN( 2 ).trigger() );
  buff.zen_sphere        = buff_creator_t( this, "zen_sphere" , talent.zen_sphere );

  // Brewmaster
  buff.elusive_brew_stacks    = buff_creator_t( this, "elusive_brew_stacks"    ).spell( find_spell( 128939 ) );
  buff.elusive_brew_activated = buff_creator_t( this, "elusive_brew_activated" ).spell( spec.elusive_brew )
                                .add_invalidate( CACHE_DODGE );
  buff.guard                  = absorb_buff_creator_t( this, "guard" ).spell( find_class_spell( "Guard" ) )
                                .source( get_stats( "guard" ) )
                                .cd( timespan_t::zero() );
  buff.power_guard            = buff_creator_t( this, "power_guard" )
                                .spell( spec.brewmaster_training -> effectN( 1 ).trigger() );
  buff.shuffle                = buff_creator_t( this, "shuffle" ).spell( find_spell( 115307 ) )
                                .add_invalidate( CACHE_PARRY );

  // Mistweaver
  buff.mana_tea          = buff_creator_t( this, "mana_tea"            ).spell( find_spell( 115867 ) );
  buff.muscle_memory     = buff_creator_t( this, "muscle_memory"       ).spell( find_spell( 139597 ) );
  buff.serpents_zeal     = buff_creator_t( this, "serpents_zeal"       ).spell( find_spell( 127722 ) );
  buff.vital_mists       = buff_creator_t( this, "vital_mists"         ).spell( find_spell( 118674 ) );

  // Windwalker
  buff.chi_sphere        = buff_creator_t( this, "chi_sphere"          ).max_stack( 5 );
  buff.combo_breaker_bok = buff_creator_t( this, "combo_breaker_bok"   ).spell( find_spell( 116768 ) );
  buff.combo_breaker_tp  = buff_creator_t( this, "combo_breaker_tp"    ).spell( find_spell( 118864 ) );
  buff.energizing_brew   = buff_creator_t( this, "energizing_brew" ).spell( find_class_spell( "Energizing Brew" ) );
  buff.energizing_brew -> buff_duration += sets -> set( SET_T14_4PC_MELEE ) -> effectN( 1 ).time_value(); //verify working
  buff.tigereye_brew     = buff_creator_t( this, "tigereye_brew"       ).spell( find_spell( 125195 ) );
  buff.tigereye_brew_use = buff_creator_t( this, "tigereye_brew_use"   ).spell( find_spell( 116740 ) ).add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
}

// monk_t::init_gains =======================================================

void monk_t::init_gains()
{
  base_t::init_gains();

  gain.avoided_chi           = get_gain( "chi_from_avoided_attacks" );
  gain.chi                   = get_gain( "chi"                      );
  gain.chi_brew              = get_gain( "chi_from_chi_brew"        );
  gain.combo_breaker_savings = get_gain( "combo_breaker_savings"    );
  gain.energy_refund         = get_gain( "energy_refund"            );
  gain.energizing_brew       = get_gain( "energizing_brew"          );
  gain.mana_tea              = get_gain( "mana_tea"                 );
  gain.muscle_memory         = get_gain( "muscle_memory"            );
  gain.soothing_mist         = get_gain( "Soothing Mist"            );
  gain.tier15_2pc            = get_gain( "tier15_2pc"               );
}

// monk_t::init_rng =========================================================

void monk_t::init_rng()
{
  base_t::init_rng();

  rng.mana_tea                    = get_rng( "mana_tea"   );
  rng.tier15_2pc_melee            = get_rng( "tier15_2pc" );
  rng.tier15_4pc_melee            = get_rng( "tier15_4pc" );
}

// monk_t::init_procs =======================================================

void monk_t::init_procs()
{
  base_t::init_procs();

  proc.mana_tea         = get_proc( "mana_tea"   );
  proc.tier15_2pc_melee = get_proc( "tier15_2pc" );
  proc.tier15_4pc_melee = get_proc( "tier15_4pc" );
}

// monk_t::reset ============================================================

void monk_t::reset()
{
  base_t::reset();

  track_chi_consumption = 0.0;
  _active_stance = FIERCE_TIGER;
}

void monk_t::init_defense()
{
  base_t::init_defense();

}

// monk_t::regen (brews/teas)================================================

void monk_t::regen( timespan_t periodicity )
{
  resource_e resource_type = primary_resource();

  if ( resource_type == RESOURCE_MANA )
  {
    //TODO: add mana tea here
  }
  else if ( resource_type == RESOURCE_ENERGY )
  {
    if ( buff.energizing_brew -> up() )
      resource_gain( RESOURCE_ENERGY,
                     buff.energizing_brew -> data().effectN( 1 ).base_value() * periodicity.total_seconds(),
                     gain.energizing_brew );
  }

  base_t::regen( periodicity );
}

// monk_t::matching_gear_multiplier =========================================

double monk_t::matching_gear_multiplier( attribute_e attr )
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

// monk_t::decode_set =======================================================

int monk_t::decode_set( item_t& item )
{
  if ( item.slot != SLOT_HEAD      &&
       item.slot != SLOT_SHOULDERS &&
       item.slot != SLOT_CHEST     &&
       item.slot != SLOT_HANDS     &&
       item.slot != SLOT_LEGS      )
  {
    return SET_NONE;
  }

  std::string s = item.name();

  if ( util::str_in_str_ci( s, "red_crane" ) )
  {
    if ( util::str_in_str_ci( s, "helm"      ) ||
         util::str_in_str_ci( s, "mantle"    ) ||
         util::str_in_str_ci( s, "vest"      ) ||
         util::str_in_str_ci( s, "legwraps"  ) ||
         util::str_in_str_ci( s, "handwraps" ) )
    {
      return SET_T14_HEAL;
    }

    if ( util::str_in_str_ci( s, "tunic"     ) ||
         util::str_in_str_ci( s, "headpiece" ) ||
         util::str_in_str_ci( s, "leggings"  ) ||
         util::str_in_str_ci( s, "spaulders" ) ||
         util::str_in_str_ci( s, "grips"     ) )
    {
      return SET_T14_MELEE;
    }

    if ( util::str_in_str_ci( s, "chestguard"     ) ||
         util::str_in_str_ci( s, "crown"          ) ||
         util::str_in_str_ci( s, "legguards"      ) ||
         util::str_in_str_ci( s, "shoulderguards" ) ||
         util::str_in_str_ci( s, "gauntlets"      ) )
    {
      return SET_T14_TANK;
    }
  } // end "red_crane"

  if ( util::str_in_str_ci( s, "firecharm" ) )
  {
    if ( util::str_in_str_ci( s, "helm"      ) ||
         util::str_in_str_ci( s, "mantle"    ) ||
         util::str_in_str_ci( s, "vest"      ) ||
         util::str_in_str_ci( s, "legwraps"  ) ||
         util::str_in_str_ci( s, "handwraps" ) )
    {
      return SET_T15_HEAL;
    }

    if ( util::str_in_str_ci( s, "tunic"     ) ||
         util::str_in_str_ci( s, "headpiece" ) ||
         util::str_in_str_ci( s, "leggings"  ) ||
         util::str_in_str_ci( s, "spaulders" ) ||
         util::str_in_str_ci( s, "grips"     ) )
    {
      return SET_T15_MELEE;
    }

    if ( util::str_in_str_ci( s, "chestguard"     ) ||
         util::str_in_str_ci( s, "crown"          ) ||
         util::str_in_str_ci( s, "legguards"      ) ||
         util::str_in_str_ci( s, "shoulderguards" ) ||
         util::str_in_str_ci( s, "gauntlets"      ) )
    {
      return SET_T15_TANK;
    }
  } // end "fire_charm"

  if ( util::str_in_str_ci( s, "_gladiators_copperskin_"  ) ) return SET_PVP_HEAL;
  if ( util::str_in_str_ci( s, "_gladiators_ironskin_"    ) ) return SET_PVP_MELEE;

  return SET_NONE;
}

// monk_t::composite_attack_speed

double monk_t::composite_melee_speed()
{
  double cas = base_t::composite_melee_speed();

  if ( ! dual_wield() )
    cas *= 1.0 / ( 1.0 + spec.way_of_the_monk -> effectN( 2 ).percent() );

  if ( buff.tiger_strikes -> up() )
    cas *= 1.0 / ( 1.0 + buff.tiger_strikes -> data().effectN( 1 ).percent() );

  return cas;
}

// monk_t::composite_attack_haste

double monk_t::composite_melee_haste()
{
  double h = base_t::composite_melee_haste();

  if ( current_stance() == WISE_SERPENT )
  {
    h *= 1.0 + current.stats.haste_rating / current_rating().attack_haste;
    h /= 1.0 + current.stats.haste_rating * ( 1 + static_stance_data( WISE_SERPENT ).effectN( 4 ).percent() ) / current_rating().attack_haste;
  }

  return h;
}

// monk_t::composite_spell_haste

double monk_t::composite_spell_haste()
{
  double h = base_t::composite_spell_haste();

  if ( current_stance() == WISE_SERPENT )
  {
    h *= 1.0 + current.stats.haste_rating / current_rating().spell_haste;
    h /= 1.0 + current.stats.haste_rating * ( 1 + static_stance_data( WISE_SERPENT ).effectN( 4 ).percent() ) / current_rating().spell_haste;
  }

  return h;
}

// monk_t::composite_player_multiplier

double monk_t::composite_player_multiplier( school_e school )
{
  double m = base_t::composite_player_multiplier( school );

  m *= 1.0 + active_stance_data( FIERCE_TIGER ).effectN( 3 ).percent();

  m *= 1.0 + buff.tigereye_brew_use -> value();

  return m;
}

// monk_t::composite_player_heal_multiplier

double monk_t::composite_player_heal_multiplier( school_e school )
{
  double m = base_t::composite_player_heal_multiplier( school );

  m *= 1.0 + active_stance_data( WISE_SERPENT ).effectN( 5 ).percent();

  return m;
}

// monk_t::composite_attack_hit

double monk_t::composite_melee_hit()
{
  double ah = base_t::composite_melee_hit();

  if ( current_stance() == WISE_SERPENT )
    ah += ( ( cache.spirit() - base.stats.get_stat( STAT_SPIRIT ) ) * static_stance_data( WISE_SERPENT ).effectN( 4 ).percent() ) / current_rating().attack_hit;

  return ah;
}

// monk_t::composite_spell_hit

double monk_t::composite_spell_hit()
{
  double sh = base_t::composite_spell_hit();

  if ( current_stance() == WISE_SERPENT )
    sh += ( ( cache.spirit() - base.stats.get_stat( STAT_SPIRIT ) ) * static_stance_data( WISE_SERPENT ).effectN( 4 ).percent() ) / current_rating().spell_hit;

  return sh;
}

// monk_t::composite_attack_expertise

double monk_t::composite_melee_expertise( weapon_t* weapon )
{
  double e = base_t::composite_melee_expertise( weapon );

  if ( current_stance() == WISE_SERPENT )
    e += ( ( cache.spirit() - base.stats.get_stat( STAT_SPIRIT ) ) * static_stance_data( WISE_SERPENT ).effectN( 4 ).percent() ) / current_rating().expertise;

  return e;
}

// monk_t::composite_attack_power

double monk_t::composite_melee_attack_power()
{
  if ( current_stance() == WISE_SERPENT )
    return composite_spell_power( SCHOOL_MAX ) * static_stance_data( WISE_SERPENT ).effectN( 2 ).percent();

  return base_t::composite_melee_attack_power();
}

// monk_t::composite_tank_parry

double monk_t::composite_parry()
{
  double p = base_t::composite_parry();

  if ( buff.shuffle -> check() )
  { p += buff.shuffle -> data().effectN( 1 ).percent(); }

  p += passives.swift_reflexes -> effectN( 2 ).percent();

  return p;
}

// monk_t::composite_tank_dodge

double monk_t::composite_dodge()
{
  double d = base_t::composite_dodge();

  if ( buff.elusive_brew_activated -> check() )
  { d += buff.elusive_brew_activated -> current_value * buff.elusive_brew_activated -> data().effectN( 1 ).percent(); }

  return d;
}

double monk_t::composite_crit_avoidance()
{
  double c = base_t::composite_crit_avoidance();

  c += active_stance_data( STURDY_OX ).effectN( 5 ).percent();

  return c;
}

void monk_t::invalidate_cache( cache_e c )
{
  base_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_SPELL_POWER:
      if ( current_stance() == WISE_SERPENT )
        player_t::invalidate_cache( CACHE_ATTACK_POWER );
      break;
    case CACHE_SPIRIT:
      if ( current_stance() == WISE_SERPENT )
      {
        player_t::invalidate_cache( CACHE_HIT );
        player_t::invalidate_cache( CACHE_ATTACK_EXP );
      }
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

void monk_t::copy_from( player_t* source )
{
  base_t::copy_from( source );

  monk_t* source_p = debug_cast<monk_t*>( source );

  user_options = source_p -> user_options;
}

// monk_t::primary_role =====================================================

resource_e monk_t::primary_resource()
{
  if ( current_stance() == WISE_SERPENT )
    return RESOURCE_MANA;

  return RESOURCE_ENERGY;
}

// monk_t::primary_role =====================================================

role_e monk_t::primary_role()
{
  if ( base_t::primary_role() == ROLE_DPS )
    return ROLE_HYBRID;

  if ( base_t::primary_role() == ROLE_TANK  )
    return ROLE_TANK;

  if ( base_t::primary_role() == ROLE_HEAL )
    return ROLE_HEAL;

  if ( specialization() == MONK_BREWMASTER )
    return ROLE_TANK;

  if ( specialization() == MONK_MISTWEAVER )
    return ROLE_HEAL;

  if ( specialization() == MONK_WINDWALKER )
    return ROLE_DPS;

  return ROLE_HYBRID;
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

double monk_t::energy_regen_per_second()
{
  double r = base_t::energy_regen_per_second();

  r *= 1.0 + talent.ascension -> effectN( 3 ).percent();

  r *= 1.0 + active_stance_data( STURDY_OX ).effectN( 9 ).percent();

  return r;
}

void monk_t::combat_begin()
{
  base_t::combat_begin();

  resources.current[ RESOURCE_CHI ] = clamp( as<double>( user_options.initial_chi ), 0.0, resources.max[ RESOURCE_CHI ] );

  if ( specialization() == MONK_BREWMASTER )
    vengeance_start();

  if ( talent.power_strikes -> ok() )
  {
    // Random start of the first tick.
    timespan_t d = sim -> default_rng() -> real() * timespan_t::from_seconds( 20.0 );
    new ( *sim ) power_strikes_event_t( this, d );
  }
}

void monk_t::target_mitigation( school_e school,
                                dmg_e    dt,
                                action_state_t* s )
{
  base_t::target_mitigation( school, dt, s );

  if ( buff.fortifying_brew -> check() )
  { s -> result_amount *= 1.0 + buff.fortifying_brew -> data().effectN( 2 ).percent(); }

  s -> result_amount *= 1.0 + active_stance_data( STURDY_OX ).effectN( 4 ).percent();
}

void monk_t::assess_damage( school_e school,
                            dmg_e    dtype,
                            action_state_t* s )
{
  buff.shuffle -> up();
  buff.fortifying_brew -> up();
  buff.elusive_brew_activated -> up();

  base_t::assess_damage( school, dtype, s );
}

void monk_t::assess_damage_imminent( school_e school,
                                     dmg_e    dtype,
                                     action_state_t* s )
{
  base_t::assess_damage_imminent( school, dtype, s );

  if ( current_stance() != STURDY_OX )
    return;

  double stagger_dmg = s -> result_amount > 0 ? s -> result_amount * stagger_pct() : 0.0;
  s -> result_amount -= stagger_dmg;

  // Hook up Stagger Mechanism
  if ( stagger_dmg > 0 )
    ignite::trigger_pct_based( active_actions.stagger_self_damage, this, stagger_dmg );
}

// Mistweaver Pre-Combat Action Priority List

void monk_t::apl_pre_brewmaster()
{
  action_priority_list_t* pre = get_action_priority_list( "precombat" );

  // Flask
  if ( sim -> allow_flasks && level >= 80 )
  {
    if ( level >= 85 )
      pre -> add_action( "flask,type=earth" );
    else
      pre -> add_action( "flask,type=steelskin" );
  }

  // Food
  if ( sim -> allow_food && level >= 80 )
  {
    if ( level >= 85 )
      pre -> add_action( "/food,type=great_pandaren_banquet" );
    else
      pre -> add_action( "/food,type=great_pandaren_banquet" ); // FIXME
  }

  pre -> add_action( "stance,choose=sturdy_ox" );
  pre -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  // Potion
  if ( sim -> allow_potions && level >= 80 )
  {
    if ( level >= 85 )
      pre -> add_action( "mountains_potion" );
    else
      pre -> add_action( "mountains_potion" ); // FIXME
  }
}

// Mistweaver Pre-Combat Action Priority List

void monk_t::apl_pre_windwalker()
{
  std::string& precombat = get_action_priority_list( "precombat" ) -> action_list_str;

  if ( sim -> allow_flasks )
  {
    // Flask
    precombat += "flask,type=spring_blossoms";
  }

  if ( sim -> allow_food )
  {
    // Food
    precombat += "/food,type=sea_mist_rice_noodles";
  }

  precombat += "/stance,choose=fierce_tiger";
  precombat += "/snapshot_stats";

  if ( sim -> allow_potions )
  {
    // Prepotion
    if ( level >= 85 )
      precombat += "/virmens_bite_potion";
    else if ( level > 80 )
      precombat += "/tolvir_potion";
  }
}

// Mistweaver Pre-Combat Action Priority List

void monk_t::apl_pre_mistweaver()
{
  std::string& precombat = get_action_priority_list( "precombat" ) -> action_list_str;

  if ( sim -> allow_flasks )
  {
    // Flask
    if ( level >= 85 )
      precombat += "flask,type=spring_blossoms";
    else if ( level > 80 )
      precombat += "flask,type=draconic_mind";
  }

  if ( sim -> allow_food )
  {
    // Food
    if ( level >= 85 )
      precombat += "/food,type=mogu_fish_stew";
    else if ( level > 80 )
      precombat += "/food,type=seafood_magnifique_feast";
  }

  precombat += "/stance,choose=wise_serpent";
  precombat += "/snapshot_stats";

  if ( sim -> allow_potions )
  {
    // Prepotion
    if ( level >= 85 )
      precombat += "/jade_serpent_potion";
    else if ( level > 80 )
      precombat += "/volcanic_potion";
  }
}

// Brewmaster Combat Action Priority List

void monk_t::apl_combat_brewmaster()
{
  action_priority_list_t* def       = get_action_priority_list( "default"   );

  def -> add_action( "auto_attack" );
  def -> add_action( this, "Fortifying Brew", "if=health.pct<40" );

  def -> add_action( "mountains_potion,if=health.pct<35&buff.mountains_potion.down" );
  def -> add_action( this, "Elusive Brew", "if=buff.elusive_brew_stacks.react>=6" );
  def -> add_action( this, "Keg Smash" );
  def -> add_action( this, "Breath of Fire", "if=target.debuff.dizzying_haze.up" );
  def -> add_action( this, "Guard" );
  def -> add_talent( this, "Chi Burst" );
  def -> add_action( this, "Jab", "if=energy.pct>40&chi+2<=chi.max" );
  def -> add_action( this, "Blackout Kick", "if=chi>=4" );
}

// Windwalker Combat Action Priority List

void monk_t::apl_combat_windwalker()
{
  std::string& aoe_list_str = get_action_priority_list( "aoe" ) -> action_list_str;
  std::string& st_list_str = get_action_priority_list( "single_target" ) -> action_list_str;

  action_list_str += "/auto_attack";
  action_list_str += "/chi_sphere,if=talent.power_strikes.enabled&buff.chi_sphere.react&chi<4";

  if ( sim -> allow_potions )
  {
    if ( level >= 85 )
      action_list_str += "/virmens_bite_potion,if=buff.bloodlust.react|target.time_to_die<=60";
  }

  // PROFS/RACIALS
  action_list_str += init_use_profession_actions();

  // USE ITEM (engineering etc)
  for ( size_t i = 0, end = items.size(); i < end; ++i )
  {
    if ( items[ i ].parsed.use.active() )
    {
      action_list_str += "/use_item,name=";
      action_list_str += items[ i ].name();
    }
  }

  action_list_str += init_use_racial_actions();
  action_list_str += "/chi_brew,if=talent.chi_brew.enabled&chi=0";
  action_list_str += "/tiger_palm,if=buff.tiger_power.remains<=3";
  if ( find_item( "rune_of_reorigination" ) )
  {
    action_list_str += "/tigereye_brew,line_cd=15,if=buff.rune_of_reorigination.react&(buff.rune_of_reorigination.remains<=1|(buff.tigereye_brew_use.down&cooldown.rising_sun_kick.remains=0&chi>=2&target.debuff.rising_sun_kick.remains&buff.tiger_power.remains))";
    action_list_str += "/tigereye_brew,if=!buff.tigereye_brew_use.up&(buff.tigereye_brew.react>19|target.time_to_die<20)";
  }
  else
  {
    action_list_str += "/tigereye_brew,if=buff.tigereye_brew_use.down&cooldown.rising_sun_kick.remains=0&chi>=2&target.debuff.rising_sun_kick.remains&buff.tiger_power.remains";
  }
  action_list_str += "/energizing_brew,if=energy.time_to_max>5";
  action_list_str += "/rising_sun_kick,if=!target.debuff.rising_sun_kick.remains";
  action_list_str  += "/tiger_palm,if=buff.tiger_power.down&target.debuff.rising_sun_kick.remains>1&energy.time_to_max>1";

  action_list_str += "/invoke_xuen,if=talent.invoke_xuen.enabled";
  action_list_str += "/run_action_list,name=aoe,if=active_enemies>=5";
  action_list_str += "/run_action_list,name=single_target,if=active_enemies<5";
  //aoe

  aoe_list_str += "/rushing_jade_wind,if=talent.rushing_jade_wind.enabled";
  aoe_list_str += "/rising_sun_kick,if=chi=4";
  aoe_list_str += "/spinning_crane_kick";

  //st
  st_list_str += "/rising_sun_kick";
  st_list_str += "/fists_of_fury,if=!buff.energizing_brew.up&energy.time_to_max>4&buff.tiger_power.remains>4";
  st_list_str += "/chi_wave,if=talent.chi_wave.enabled&energy.time_to_max>2";
  st_list_str += "/blackout_kick,if=buff.combo_breaker_bok.react";
  st_list_str += "/tiger_palm,if=(buff.combo_breaker_tp.react&energy.time_to_max>=2)|(buff.combo_breaker_tp.remains<=2&buff.combo_breaker_tp.react)";
  st_list_str += "/jab,if=talent.ascension.enabled&chi<=3";
  st_list_str += "/jab,if=!talent.ascension.enabled&chi<=2";
  st_list_str += "/blackout_kick,if=(energy+(energy.regen*(cooldown.rising_sun_kick.remains)))>=40";
}

// Mistweaver Combat Action Priority List

void monk_t::apl_combat_mistweaver()
{
  action_priority_list_t* def       = get_action_priority_list( "default"   );

  def -> add_action( "auto_attack" );
  def -> add_action( this, "Mana Tea", "if=buff.mana_tea.react>=2&mana.pct<=90" );
  def -> add_talent( this, "Chi Wave" );
  def -> add_talent( this, "Chi Brew", "if=buff.tiger_power.up" );
  def -> add_action( this, "Blackout Kick", "if=buff.muscle_memory.up&buff.tiger_power.up&buff.serpents_zeal.down" );
  def -> add_action( this, "Tiger Palm", "if=buff.muscle_memory.up" );
  def -> add_action( this, "Jab" );
  def -> add_action( this, "Mana Tea" );
}

// monk_t::init_actions =====================================================

void monk_t::init_actions()
{
  if ( ! action_list_str.empty() )
  {
    player_t::init_actions();
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
  action_list_default = 1;

  base_t::init_actions();
}

double monk_t::stagger_pct()
{
  double stagger = 0.0;

  if ( current_stance() == STURDY_OX ) // no stagger without active stance
  {
    stagger += static_stance_data( STURDY_OX ).effectN( 8 ).percent();

    if ( buff.shuffle -> check() )
      stagger += buff.shuffle -> data().effectN( 2 ).percent();

    if ( spec.brewmaster_training -> ok() && buff.fortifying_brew -> check() )
      stagger += spec.brewmaster_training -> effectN( 2 ).percent();

    if ( mastery.elusive_brawler -> ok() )
      stagger += cache.mastery_value();
  }

  return stagger;
}

/* Invalidate Cache affected by stance
 */
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
      invalidate_cache( CACHE_HIT );
      invalidate_cache( CACHE_EXP );
      invalidate_cache( CACHE_HASTE );
      invalidate_cache( CACHE_ATTACK_POWER );
      break;
  }
}

/* Switch to specified stance
 */
bool monk_t::switch_to_stance( stance_e to )
{
  if ( to == current_stance() )
    return false;

  // validate stance availability
  if ( ! static_stance_data( to ).ok() )
    return false;

  stance_invalidates( current_stance() ); // Invalidate old stance
  _active_stance = to;
  stance_invalidates( current_stance() ); // Invalidate new stance

  return true;
}

/* Returns the stance data of the requested stance
 */
inline const spell_data_t& monk_t::static_stance_data( stance_e stance )
{
  switch ( stance )
  {
    case FIERCE_TIGER:
      return *stance_data.fierce_tiger;
    case STURDY_OX:
      return *stance_data.sturdy_ox;
    case WISE_SERPENT:
      return *stance_data.wise_serpent;
  }

  assert( false );
  return *spell_data_t::not_found();
}

/* Returns the stance data of the requested stance ONLY IF the stance is active
 */
const spell_data_t& monk_t::active_stance_data( stance_e stance )
{
  if ( stance != current_stance() )
    return *spell_data_t::not_found();

  return static_stance_data( stance );
}

// MONK MODULE INTERFACE ====================================================

struct monk_module_t : public module_t
{
  monk_module_t() : module_t( MONK ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const
  {
    return new monk_t( sim, name, r );
  }
  virtual bool valid() const { return true; }
  virtual void init        ( sim_t* ) const {}
  virtual void combat_begin( sim_t* ) const {}
  virtual void combat_end  ( sim_t* ) const {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::monk()
{
  static monk_module_t m;
  return &m;
}
