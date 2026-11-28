// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "action/parse_effects.hpp"
#include "action/residual_action.hpp"
#include "player/ground_aoe.hpp"

#include "class_modules/apl/mage.hpp"

namespace mage
{
// ==========================================================================
// Spell ranks
// ==========================================================================

// tick_id is the separate damage spell used by ground effects and channels
// whose periodic effect only triggers another spell.
struct spell_rank_t
{
  unsigned spell_id;
  unsigned tick_id = 0;
};

template <typename RANKS>
auto max_rank( const player_t* player, const RANKS& ranks )
{
  typename RANKS::value_type best{};
  unsigned best_level = 0;

  for ( const auto& rank : ranks )
  {
    const spell_data_t* spell = player->find_spell( rank.spell_id );

    if ( spell->ok() && spell->level() >= best_level )
    {
      best       = rank;
      best_level = spell->level();
    }
  }

  return best;
}

constexpr std::array fireball_ranks{
  spell_rank_t{ 133 },   spell_rank_t{ 143 },   spell_rank_t{ 145 },   spell_rank_t{ 3140 },
  spell_rank_t{ 8400 },  spell_rank_t{ 8401 },  spell_rank_t{ 8402 },  spell_rank_t{ 10148 },
  spell_rank_t{ 10149 }, spell_rank_t{ 10150 }, spell_rank_t{ 10151 }, spell_rank_t{ 25306 },
};
constexpr std::array frostbolt_ranks{
  spell_rank_t{ 116 },   spell_rank_t{ 205 },   spell_rank_t{ 837 },   spell_rank_t{ 7322 },
  spell_rank_t{ 8406 },  spell_rank_t{ 8407 },  spell_rank_t{ 8408 },  spell_rank_t{ 10179 },
  spell_rank_t{ 10180 }, spell_rank_t{ 10181 }, spell_rank_t{ 25304 },
};
constexpr std::array fire_blast_ranks{
  spell_rank_t{ 2136 }, spell_rank_t{ 2137 },  spell_rank_t{ 2138 },  spell_rank_t{ 8412 },
  spell_rank_t{ 8413 }, spell_rank_t{ 10197 }, spell_rank_t{ 10199 },
};
constexpr std::array scorch_ranks{
  spell_rank_t{ 2948 },  spell_rank_t{ 8444 },  spell_rank_t{ 8445 }, spell_rank_t{ 8446 },
  spell_rank_t{ 10205 }, spell_rank_t{ 10206 }, spell_rank_t{ 10207 },
};
// Arcane Missiles: channel spell + the missile it triggers each second
constexpr std::array arcane_missiles_ranks{
  spell_rank_t{ 5143, 7268 },   spell_rank_t{ 5144, 7269 },   spell_rank_t{ 5145, 7270 },
  spell_rank_t{ 8416, 8419 },   spell_rank_t{ 8417, 8418 },   spell_rank_t{ 10211, 10273 },
  spell_rank_t{ 10212, 10274 }, spell_rank_t{ 25345, 25346 },
};
constexpr std::array arcane_explosion_ranks{
  spell_rank_t{ 1449 },  spell_rank_t{ 8437 },  spell_rank_t{ 8438 },
  spell_rank_t{ 8439 },  spell_rank_t{ 10201 }, spell_rank_t{ 10202 },
};
// Flamestrike: cast spell + the area trigger damage spell
constexpr std::array flamestrike_ranks{
  spell_rank_t{ 2120, 1279983 },  spell_rank_t{ 2121, 1279985 },  spell_rank_t{ 8422, 1279987 },
  spell_rank_t{ 8423, 1279988 },  spell_rank_t{ 10215, 1279989 }, spell_rank_t{ 10216, 1279990 },
};
// Blizzard: channel spell + the area trigger damage spell
constexpr std::array blizzard_ranks{
  spell_rank_t{ 10, 1279976 },    spell_rank_t{ 6141, 1279977 },  spell_rank_t{ 8427, 1279978 },
  spell_rank_t{ 10185, 1279979 }, spell_rank_t{ 10186, 1279980 }, spell_rank_t{ 10187, 1279949 },
};
constexpr std::array cone_of_cold_ranks{
  spell_rank_t{ 120 },   spell_rank_t{ 8492 },  spell_rank_t{ 10159 },
  spell_rank_t{ 10160 }, spell_rank_t{ 10161 },
};
constexpr std::array frost_nova_ranks{
  spell_rank_t{ 122 }, spell_rank_t{ 865 }, spell_rank_t{ 6131 }, spell_rank_t{ 10230 },
};
constexpr std::array frost_armor_ranks{
  spell_rank_t{ 168 }, spell_rank_t{ 7300 }, spell_rank_t{ 7301 },
};
constexpr std::array ice_armor_ranks{
  spell_rank_t{ 7302 }, spell_rank_t{ 7320 }, spell_rank_t{ 10219 }, spell_rank_t{ 10220 },
};
constexpr std::array mage_armor_ranks{
  spell_rank_t{ 6117 }, spell_rank_t{ 22782 }, spell_rank_t{ 22783 },
};
constexpr std::array replenish_mana_ranks{
  spell_rank_t{ 5405 }, spell_rank_t{ 10052 }, spell_rank_t{ 10057 }, spell_rank_t{ 10058 },
};

// Talent-granted spells with ranks
constexpr std::array pyroblast_ranks{
  spell_rank_t{ 11366 }, spell_rank_t{ 12505 }, spell_rank_t{ 12522 }, spell_rank_t{ 12523 },
  spell_rank_t{ 12524 }, spell_rank_t{ 12525 }, spell_rank_t{ 12526 }, spell_rank_t{ 18809 },
};
constexpr std::array blast_wave_ranks{
  spell_rank_t{ 11113 }, spell_rank_t{ 13018 }, spell_rank_t{ 13019 },
  spell_rank_t{ 13020 }, spell_rank_t{ 13021 },
};
constexpr std::array ice_barrier_ranks{
  spell_rank_t{ 11426 }, spell_rank_t{ 13031 }, spell_rank_t{ 13032 }, spell_rank_t{ 13033 },
};
constexpr std::array ice_lance_ranks{
  spell_rank_t{ 1312002 }, spell_rank_t{ 400640 },  spell_rank_t{ 1240044 },
  spell_rank_t{ 1240045 }, spell_rank_t{ 1240046 }, spell_rank_t{ 1240047 },
};
constexpr std::array arcane_blast_ranks{
  spell_rank_t{ 400574 },  spell_rank_t{ 1239696 }, spell_rank_t{ 1239697 },
  spell_rank_t{ 1239699 }, spell_rank_t{ 1239700 },
};
constexpr std::array arcane_wrath_ranks{
  spell_rank_t{ 1326079 }, spell_rank_t{ 1326080 }, spell_rank_t{ 1326081 }, spell_rank_t{ 1326082 },
};

// ==========================================================================
// Mage
// ==========================================================================

struct mage_t;

struct mage_td_t final : public actor_target_data_t
{
  struct debuffs_t
  {
    buff_t* winters_chill      = nullptr;
    buff_t* fire_vulnerability = nullptr;
    buff_t* frozen             = nullptr;
  } debuffs;

  struct dots_t
  {
    dot_t* ignite    = nullptr;
    dot_t* fireball  = nullptr;
    dot_t* pyroblast = nullptr;
  } dots;

  mage_td_t( player_t* target, mage_t* mage );
};

struct mage_t final : public player_t
{
  struct talents_t
  {
    // Arcane
    player_talent_t wand_specialization;
    player_talent_t arcane_focus;
    player_talent_t improved_channeling;
    player_talent_t missile_barrage;
    player_talent_t arcane_subtlety;
    player_talent_t magic_absorption;
    player_talent_t arcane_concentration;
    player_talent_t arcane_resilience;
    player_talent_t arcane_geometry;
    player_talent_t arcane_impact;
    player_talent_t arcane_blast;
    player_talent_t arcane_shielding;
    player_talent_t improved_counterspell;
    player_talent_t arcane_meditation;
    player_talent_t arcane_wrath;
    player_talent_t presence_of_mind;
    player_talent_t arcane_mind;
    player_talent_t arcane_instability;
    player_talent_t arcane_power;

    // Fire
    player_talent_t improved_fire_blast;
    player_talent_t incineration;
    player_talent_t improved_fireball;
    player_talent_t ignite;
    player_talent_t flame_throwing;
    player_talent_t impact;
    player_talent_t burning_soul;
    player_talent_t improved_flamestrike;
    player_talent_t pyroblast;
    player_talent_t improved_scorch;
    player_talent_t improved_fire_ward;
    player_talent_t heating_up;
    player_talent_t master_of_elements;
    player_talent_t critical_mass;
    player_talent_t blast_wave;
    player_talent_t fire_power;
    player_talent_t combustion;

    // Frost
    player_talent_t frost_warding;
    player_talent_t improved_frostbolt;
    player_talent_t elemental_precision;
    player_talent_t ice_shards;
    player_talent_t frostbite;
    player_talent_t improved_frost_nova;
    player_talent_t permafrost;
    player_talent_t piercing_ice;
    player_talent_t frost_channeling;
    player_talent_t ice_lance;
    player_talent_t improved_blizzard;
    player_talent_t arctic_reach;
    player_talent_t ice_block;
    player_talent_t shatter;
    player_talent_t improved_cone_of_cold;
    player_talent_t cold_snap;
    player_talent_t fingers_of_frost;
    player_talent_t winters_chill;
    player_talent_t ice_barrier;
  } talents;

  struct buffs_t
  {
    buff_t* mage_armor       = nullptr;
    buff_t* frost_armor      = nullptr;
    buff_t* ice_armor        = nullptr;
    buff_t* evocation        = nullptr;
    buff_t* arcane_power     = nullptr;
    buff_t* presence_of_mind = nullptr;
    buff_t* combustion       = nullptr;  // fire crit chance, one stack per fire hit
    buff_t* combustion_crits = nullptr;  // fire crits left before Combustion ends
    buff_t* clearcasting     = nullptr;
    buff_t* heating_up       = nullptr;
    buff_t* missile_barrage  = nullptr;
    buff_t* arcane_blast     = nullptr;
    buff_t* fingers_of_frost = nullptr;
  } buffs;

  struct procs_t
  {
    proc_t* ignite_munch = nullptr;
  } procs;

  action_t* ignite = nullptr;

  struct cooldowns_t
  {
    cooldown_t* arcane_concentration = nullptr;  // internal cooldown of the Clearcasting proc
  } cooldowns;

  struct options_t
  {
    // raid bosses cannot be frozen; enable for targets Frost Nova and Frostbite can root
    bool freezable_target = false;
  } options;

  struct gains_t
  {
    gain_t* evocation          = nullptr;
    gain_t* master_of_elements = nullptr;
    gain_t* mana_gem           = nullptr;
  } gains;

  mutable target_specific_t<mage_td_t> target_data;

  mage_t( sim_t* sim, std::string_view name, race_e r = RACE_NONE ) :
    player_t( sim, MAGE, name, r ),
    talents(),
    buffs(),
    procs(),
    gains(),
    cooldowns(),
    target_data()
  {
    cooldowns.arcane_concentration = get_cooldown( "arcane_concentration" );
  }

  resource_e primary_resource() const override { return RESOURCE_MANA; }
  role_e primary_role() const override { return ROLE_SPELL; }
  stat_e convert_hybrid_stat( stat_e s ) const override { return s; }

  std::string default_potion() const override { return mage_apl::potion( this ); }
  std::string default_flask() const override { return mage_apl::flask( this ); }
  std::string default_food() const override { return mage_apl::food( this ); }

  mage_td_t* get_target_data( player_t* target ) const override
  {
    mage_td_t*& td = target_data[ target ];
    if ( !td )
      td = new mage_td_t( target, const_cast<mage_t*>( this ) );
    return td;
  }

  action_t* create_action( std::string_view name, std::string_view options_str ) override;
  void create_actions() override;
  void create_options() override;
  void init_action_list() override;
  void init_base_stats() override;
  void init_spells() override;
  void init_gains() override;
  void init_procs() override;
  void create_buffs() override;
  void reset() override;
  double resource_regen_per_second( resource_e ) const override;
  double composite_spell_power( school_e ) const override;
  double mana_regen_while_casting() const;
  // talent tree (arcane, fire, frost) holding the most points
  enum class mage_tree_e { ARCANE, FIRE, FROST };
  mage_tree_e primary_tree() const;
};

mage_td_t::mage_td_t( player_t* target, mage_t* mage ) : actor_target_data_t( target, mage ), debuffs(), dots()
{
  dots.ignite    = target->get_dot( "ignite", mage );
  dots.fireball  = target->get_dot( "fireball", mage );
  dots.pyroblast = target->get_dot( "pyroblast", mage );

  // fire damage taken from the mage, per stack
  debuffs.fire_vulnerability =
    make_buff( *this, "fire_vulnerability", mage->find_spell( 22959 ) )->set_default_value_from_effect( 1 );

  // empowers the next Cone of Cold, per stack
  debuffs.winters_chill = make_buff( *this, "winters_chill", mage->find_spell( 12579 ) );

  // rooted by Frost Nova or Frostbite; duration given on trigger
  debuffs.frozen = make_buff( *this, "frozen" )->set_duration( 0_ms );
}

// ==========================================================================
// Mage actions
// ==========================================================================

template <class Base>
struct mage_action_t : public parse_action_effects_t<Base>
{
private:
  using ab = parse_action_effects_t<Base>;

public:
  using base_t = mage_action_t<Base>;

  template <typename... Args>
  mage_action_t( Args&&... args ) : ab( std::forward<Args>( args )... )
  { }

  mage_t* p()
  { return debug_cast<mage_t*>( this->player ); }

  const mage_t* p() const
  { return debug_cast<const mage_t*>( this->player ); }

  mage_td_t* td( player_t* t ) const
  { return p()->get_target_data( t ); }
};

struct mage_spell_t : public mage_action_t<spell_t>
{
  // set by parse_effects() when the buff modifies this spell
  bool affected_by_clearcasting     = false;
  bool affected_by_presence_of_mind = false;

  // direct damage that can trigger talent procs; false for ground effect
  // pulses and dots fed by other spells
  bool triggers_procs = true;

  // non-periodic crits stack Heating Up (Fireball, Fire Blast, Scorch)
  bool triggers_heating_up = false;

  // hits apply a Chill effect (Frostbite, Fingers of Frost)
  bool chills = false;

  // this cast spends a Fingers of Frost charge: the target counts as frozen
  bool fingers_of_frost_cast = false;

  // share of the Missile Barrage talent chance this cast rolls (1 = full)
  double missile_barrage_share = 0.0;

  mage_spell_t( std::string_view n, mage_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  {
    if ( !data().ok() )
      background = true;
  }

  // Buff and debuff effects are parsed after the constructor of the final
  // class ran, so overrides of e.g. base_execute_time are already applied.
  void init() override
  {
    base_t::init();

    parse_effects( p()->buffs.arcane_power );
    parse_effects( p()->buffs.combustion );
    parse_effects( p()->buffs.heating_up );
    parse_effects( p()->buffs.arcane_blast );
    affected_by_clearcasting     = parse_effects( p()->buffs.clearcasting );
    affected_by_presence_of_mind = base_execute_time > 0_ms && parse_effects( p()->buffs.presence_of_mind );
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = base_t::composite_target_multiplier( t );

    if ( school == SCHOOL_FIRE )
      m *= 1.0 + td( t )->debuffs.fire_vulnerability->check_stack_value();

    return m;
  }

  bool frozen( player_t* t ) const
  {
    return fingers_of_frost_cast || td( t )->debuffs.frozen->check();
  }

  double composite_target_crit_chance( player_t* t ) const override
  {
    double c = base_t::composite_target_crit_chance( t );

    if ( p()->talents.shatter.ok() && frozen( t ) )
      c += p()->talents.shatter->effectN( 1 ).percent();

    return c;
  }

  // Frostbite can freeze, Fingers of Frost can empower the next casts
  void trigger_chill( action_state_t* s )
  {
    if ( p()->talents.frostbite.ok() && p()->options.freezable_target &&
         rng().roll( p()->talents.frostbite->effectN( 1 ).percent() ) )
      td( s->target )->debuffs.frozen->trigger( p()->find_spell( 12494 )->duration() );

    if ( p()->talents.fingers_of_frost.ok() && rng().roll( p()->talents.fingers_of_frost->effectN( 2 ).percent() ) )
      p()->buffs.fingers_of_frost->trigger( as<int>( p()->talents.fingers_of_frost->effectN( 1 ).base_value() ) );
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( chills && result_is_hit( s->result ) )
      trigger_chill( s );

    if ( !triggers_procs || !result_is_hit( s->result ) )
      return;

    if ( school == SCHOOL_FIRE )
      trigger_combustion( s );

    trigger_arcane_concentration();

    if ( s->result == RESULT_CRIT )
    {
      if ( school == SCHOOL_FIRE )
        trigger_ignite( s );

      if ( triggers_heating_up && p()->talents.heating_up.ok() )
        p()->buffs.heating_up->trigger();

      // refunds once per cast, not per target hit
      if ( s->chain_target <= 1 && ( school == SCHOOL_FIRE || school == SCHOOL_FROST ) )
        trigger_master_of_elements();
    }
  }

  // Each fire hit adds crit chance; the buff ends after its crits are used.
  void trigger_combustion( const action_state_t* s )
  {
    if ( !p()->buffs.combustion_crits->check() )
      return;

    if ( s->result == RESULT_CRIT )
    {
      p()->buffs.combustion_crits->decrement();
      if ( !p()->buffs.combustion_crits->check() )
      {
        p()->buffs.combustion->expire();
        return;
      }
    }

    p()->buffs.combustion->trigger();
  }

  // Clearcasting after a damage spell hits, with an internal cooldown
  void trigger_arcane_concentration()
  {
    if ( !p()->talents.arcane_concentration.ok() || p()->cooldowns.arcane_concentration->down() )
      return;

    if ( rng().roll( p()->talents.arcane_concentration->effectN( 1 ).percent() ) )
    {
      p()->buffs.clearcasting->trigger();
      p()->cooldowns.arcane_concentration->start( p()->talents.arcane_concentration->internal_cooldown() );
    }
  }

  void trigger_winters_chill( action_state_t* s )
  {
    if ( p()->talents.winters_chill.ok() && rng().roll( p()->talents.winters_chill->effectN( 2 ).percent() ) )
      td( s->target )->debuffs.winters_chill->trigger();
  }

  void trigger_ignite( const action_state_t* s )
  {
    if ( !p()->talents.ignite.ok() )
      return;

    residual_action::trigger( p()->ignite, s->target, s->result_amount * p()->talents.ignite->effectN( 1 ).percent() );
  }

  void trigger_master_of_elements()
  {
    if ( !p()->talents.master_of_elements.ok() )
      return;

    double refund = base_costs[ RESOURCE_MANA ] * p()->talents.master_of_elements->effectN( 1 ).percent();
    if ( refund > 0 )
      p()->resource_gain( RESOURCE_MANA, refund, p()->gains.master_of_elements, this );
  }

  void execute() override
  {
    // the buffs modify this cast, so read them before execute() and consume after
    bool clearcasting     = affected_by_clearcasting && p()->buffs.clearcasting->check();
    bool presence_of_mind = affected_by_presence_of_mind && p()->buffs.presence_of_mind->check();

    // any damaging cast spends a Fingers of Frost charge
    fingers_of_frost_cast = harmful && !background && p()->buffs.fingers_of_frost->check();

    base_t::execute();

    if ( fingers_of_frost_cast )
      p()->buffs.fingers_of_frost->decrement();

    if ( missile_barrage_share > 0 && p()->talents.missile_barrage.ok() &&
         rng().roll( missile_barrage_share * p()->talents.missile_barrage->effectN( 1 ).percent() ) )
      p()->buffs.missile_barrage->trigger();

    if ( clearcasting )
      p()->buffs.clearcasting->decrement();
    if ( presence_of_mind )
      p()->buffs.presence_of_mind->decrement();
  }
};

// Ignite: rolling fire dot holding a share of fire crit damage
struct ignite_t final : public residual_action::residual_periodic_action_t<mage_spell_t>
{
  ignite_t( mage_t* p ) : residual_periodic_action_t( "ignite", p, p->find_spell( 412538 ) )
  {
    triggers_procs = false;
  }
};

// Direct damage spells ======================================================

template <size_t N>
const spell_data_t* rank_spell( mage_t* p, const std::array<spell_rank_t, N>& ranks )
{
  return p->find_spell( max_rank( p, ranks ).spell_id );
}

// talent-granted spells only exist with the talent
template <size_t N>
const spell_data_t* talent_rank_spell( mage_t* p, const player_talent_t& talent,
                                       const std::array<spell_rank_t, N>& ranks )
{
  return talent.ok() ? rank_spell( p, ranks ) : spell_data_t::not_found();
}

struct fireball_t final : public mage_spell_t
{
  fireball_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "fireball", p, rank_spell( p, fireball_ranks ) )
  {
    parse_options( options_str );
    triggers_heating_up   = true;
    missile_barrage_share = 0.5;
  }
};

struct frostbolt_t final : public mage_spell_t
{
  frostbolt_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "frostbolt", p, rank_spell( p, frostbolt_ranks ) )
  {
    parse_options( options_str );
    chills                = true;
    missile_barrage_share = 0.5;
  }

  void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
      trigger_winters_chill( s );
  }
};

struct fire_blast_t final : public mage_spell_t
{
  fire_blast_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "fire_blast", p, rank_spell( p, fire_blast_ranks ) )
  {
    parse_options( options_str );
    triggers_heating_up = true;
  }

  void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    // Fire Blast adds a Heating Up stack whenever it deals damage
    if ( result_is_hit( s->result ) && p()->talents.heating_up.ok() )
      p()->buffs.heating_up->trigger();
  }
};

struct scorch_t final : public mage_spell_t
{
  scorch_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "scorch", p, rank_spell( p, scorch_ranks ) )
  {
    parse_options( options_str );
    triggers_heating_up = true;
  }

  void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    // Improved Scorch: chance to stack Fire Vulnerability
    if ( result_is_hit( s->result ) && p()->talents.improved_scorch.ok() &&
         rng().roll( p()->talents.improved_scorch->effectN( 1 ).percent() ) )
      td( s->target )->debuffs.fire_vulnerability->trigger();
  }
};

struct pyroblast_t final : public mage_spell_t
{
  pyroblast_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "pyroblast", p, talent_rank_spell( p, p->talents.pyroblast, pyroblast_ranks ) )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    mage_spell_t::execute();
    p()->buffs.heating_up->expire();
  }
};

struct ice_lance_t final : public mage_spell_t
{
  ice_lance_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "ice_lance", p, talent_rank_spell( p, p->talents.ice_lance, ice_lance_ranks ) )
  {
    parse_options( options_str );
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = mage_spell_t::composite_target_multiplier( t );

    if ( frozen( t ) )
      m *= 1.0 + data().effectN( 2 ).percent();

    return m;
  }

  void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    // only Ice Lance empowered by Fingers of Frost applies Winter's Chill
    if ( result_is_hit( s->result ) && fingers_of_frost_cast )
      trigger_winters_chill( s );
  }
};

struct arcane_blast_t final : public mage_spell_t
{
  arcane_blast_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "arcane_blast", p, talent_rank_spell( p, p->talents.arcane_blast, arcane_blast_ranks ) )
  {
    parse_options( options_str );
    missile_barrage_share = 1.0;
  }

  // each cast adds a stack: spell damage from intellect, higher Arcane Blast cost
  void execute() override
  {
    mage_spell_t::execute();
    p()->buffs.arcane_blast->trigger();
  }
};

struct arcane_wrath_t final : public mage_spell_t
{
  arcane_wrath_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "arcane_wrath", p, talent_rank_spell( p, p->talents.arcane_wrath, arcane_wrath_ranks ) )
  {
    parse_options( options_str );
    missile_barrage_share = 0.5;
  }

  // extra spell damage from maximum mana
  double composite_total_spell_power() const override
  {
    return mage_spell_t::composite_total_spell_power() +
           p()->resources.max[ RESOURCE_MANA ] * data().effectN( 2 ).percent();
  }
};

// AoE spells ================================================================

struct arcane_explosion_t final : public mage_spell_t
{
  arcane_explosion_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "arcane_explosion", p, rank_spell( p, arcane_explosion_ranks ) )
  {
    parse_options( options_str );
    aoe = -1;
  }
};

struct cone_of_cold_t final : public mage_spell_t
{
  cone_of_cold_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "cone_of_cold", p, rank_spell( p, cone_of_cold_ranks ) )
  {
    parse_options( options_str );
    aoe    = -1;
    chills = true;
  }

  // Winter's Chill: more damage and spell power coefficient per stack, then consumed
  double composite_target_multiplier( player_t* t ) const override
  {
    double m = mage_spell_t::composite_target_multiplier( t );
    auto wc  = td( t )->debuffs.winters_chill;
    m *= 1.0 + wc->check() * wc->data().effectN( 1 ).percent();
    return m;
  }

  double spell_direct_power_coefficient( const action_state_t* s ) const override
  {
    double c = mage_spell_t::spell_direct_power_coefficient( s );
    auto wc  = td( s->target )->debuffs.winters_chill;
    c += wc->check() * wc->data().effectN( 2 ).percent();
    return c;
  }

  void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );
    td( s->target )->debuffs.winters_chill->expire();
  }
};

struct frost_nova_t final : public mage_spell_t
{
  frost_nova_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "frost_nova", p, rank_spell( p, frost_nova_ranks ) )
  {
    parse_options( options_str );
    aoe = -1;
  }

  void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) && p()->options.freezable_target )
      td( s->target )->debuffs.frozen->trigger( data().duration() );
  }
};

struct blast_wave_t final : public mage_spell_t
{
  blast_wave_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "blast_wave", p, talent_rank_spell( p, p->talents.blast_wave, blast_wave_ranks ) )
  {
    parse_options( options_str );
    aoe = -1;
  }
};

// damage spell fired by a ground effect or a channel
struct mage_tick_spell_t final : public mage_spell_t
{
  mage_tick_spell_t( std::string_view n, mage_t* p, unsigned spell_id ) :
    mage_spell_t( n, p, p->find_spell( spell_id ) )
  {
    background = dual = ground_aoe = true;
    aoe                         = -1;
    triggers_procs              = false;
  }
};

struct flamestrike_t final : public mage_spell_t
{
  action_t* burn = nullptr;

  flamestrike_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "flamestrike", p, rank_spell( p, flamestrike_ranks ) )
  {
    parse_options( options_str );
    aoe = -1;

    if ( data().ok() )
    {
      burn = new mage_tick_spell_t( "flamestrike_burn", p, max_rank( p, flamestrike_ranks ).tick_id );
      add_child( burn );
    }
  }

  void execute() override
  {
    mage_spell_t::execute();

    make_event<ground_aoe_event_t>( *sim, p(),
                                    ground_aoe_params_t()
                                      .target( execute_state->target )
                                      .duration( data().duration() )
                                      .pulse_time( data().effectN( 3 ).period() )
                                      .action( burn ) );
  }
};

// Blizzard channels; the area trigger it creates deals the damage each second
struct blizzard_t final : public mage_spell_t
{
  blizzard_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "blizzard", p, rank_spell( p, blizzard_ranks ) )
  {
    parse_options( options_str );
    channeled = true;
    may_miss = may_crit = false;
    hasted_ticks        = false;
    tick_zero           = false;

    if ( data().ok() )
    {
      dot_duration   = data().duration();
      base_tick_time = data().effectN( 3 ).period();
      auto tick      = new mage_tick_spell_t( "blizzard_tick", p, max_rank( p, blizzard_ranks ).tick_id );
      tick->chills   = p->talents.improved_blizzard.ok();
      tick_action    = tick;
    }
  }
};

struct arcane_missiles_t final : public mage_spell_t
{
  struct arcane_missile_t final : public mage_spell_t
  {
    arcane_missile_t( mage_t* p, unsigned spell_id ) :
      mage_spell_t( "arcane_missiles_tick", p, p->find_spell( spell_id ) )
    {
      background = dual = true;
    }
  };

  arcane_missiles_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "arcane_missiles", p, rank_spell( p, arcane_missiles_ranks ) )
  {
    parse_options( options_str );
    channeled = true;
    may_miss = may_crit = false;
    hasted_ticks        = false;
    tick_zero           = false;

    if ( data().ok() )
      tick_action = new arcane_missile_t( p, max_rank( p, arcane_missiles_ranks ).tick_id );
  }

  void init() override
  {
    mage_spell_t::init();
    parse_effects( p()->buffs.missile_barrage );
  }

  // Missile Barrage shortens the channel and the time between missiles; it
  // stays up until the channel ends so every tick keeps the faster rate
  void last_tick( dot_t* d ) override
  {
    mage_spell_t::last_tick( d );
    p()->buffs.missile_barrage->expire();
  }
};

// Utility ===================================================================

struct armor_t final : public mage_spell_t
{
  buff_t* buff;

  template <size_t N>
  armor_t( std::string_view n, mage_t* p, std::string_view options_str, const std::array<spell_rank_t, N>& ranks,
           buff_t* b ) :
    mage_spell_t( n, p, rank_spell( p, ranks ) ), buff( b )
  {
    parse_options( options_str );
    harmful = false;
    target  = p;
  }

  void execute() override
  {
    mage_spell_t::execute();

    // only one armor can be active
    for ( buff_t* b : { p()->buffs.mage_armor, p()->buffs.ice_armor, p()->buffs.frost_armor } )
      if ( b != buff )
        b->expire();

    buff->trigger();
  }

  bool ready() override
  {
    return !buff->check() && mage_spell_t::ready();
  }
};

struct evocation_t final : public mage_spell_t
{
  evocation_t( mage_t* p, std::string_view options_str ) : mage_spell_t( "evocation", p, p->find_spell( 12051 ) )
  {
    parse_options( options_str );
    harmful = false;
    target  = p;
    channeled = true;
    hasted_ticks = tick_zero = false;
    may_miss = may_crit = false;
  }

  void execute() override
  {
    mage_spell_t::execute();
    p()->buffs.evocation->trigger();
  }

  void last_tick( dot_t* d ) override
  {
    mage_spell_t::last_tick( d );
    p()->buffs.evocation->expire();
  }
};

// Conjured mana gems; one of each gem the mage can conjure, sharing a cooldown
struct mana_gem_t final : public mage_spell_t
{
  std::vector<const spell_data_t*> gems;
  size_t used = 0;

  mana_gem_t( mage_t* p, std::string_view options_str ) : mage_spell_t( "mana_gem", p )
  {
    parse_options( options_str );
    harmful = may_miss = may_crit = false;
    target              = p;
    background          = false;
    trigger_gcd         = 0_ms;
    use_off_gcd         = true;

    for ( auto rank : replenish_mana_ranks )
      if ( auto s = p->find_spell( rank.spell_id ); s->ok() )
        gems.push_back( s );

    // highest gem first
    std::reverse( gems.begin(), gems.end() );

    if ( gems.empty() )
      background = true;
    else
      cooldown->duration = gems.front()->cooldown();
  }

  void execute() override
  {
    mage_spell_t::execute();

    const auto& e = gems[ used++ ]->effectN( 1 );
    double amount = rng().range( e.min( p() ), e.max( p() ) );
    p()->resource_gain( RESOURCE_MANA, amount, p()->gains.mana_gem, this );
  }

  bool ready() override
  {
    return used < gems.size() && mage_spell_t::ready();
  }

  void reset() override
  {
    mage_spell_t::reset();
    used = 0;
  }
};

// cooldown that only triggers a buff
struct buff_spell_t final : public mage_spell_t
{
  buff_t* buff;

  buff_spell_t( std::string_view n, mage_t* p, std::string_view options_str, const player_talent_t& talent,
                buff_t* b ) :
    mage_spell_t( n, p, talent.ok() ? talent.spell() : spell_data_t::not_found() ), buff( b )
  {
    parse_options( options_str );
    harmful = may_miss = may_crit = false;
    target              = p;
    trigger_gcd         = 0_ms;
    use_off_gcd         = true;
  }

  void execute() override
  {
    mage_spell_t::execute();
    buff->trigger();
  }
};

// Combustion lasts until its fire crits are used up
struct combustion_t final : public mage_spell_t
{
  combustion_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "combustion", p, p->talents.combustion.ok() ? p->talents.combustion.spell() : spell_data_t::not_found() )
  {
    parse_options( options_str );
    harmful = may_miss = may_crit = false;
    target              = p;
    trigger_gcd         = 0_ms;
    use_off_gcd         = true;
    triggers_procs      = false;
  }

  void execute() override
  {
    mage_spell_t::execute();
    p()->buffs.combustion->expire();
    p()->buffs.combustion_crits->trigger( p()->buffs.combustion_crits->max_stack() );
  }
};

struct cold_snap_t final : public mage_spell_t
{
  cold_snap_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "cold_snap", p, p->talents.cold_snap.ok() ? p->talents.cold_snap.spell() : spell_data_t::not_found() )
  {
    parse_options( options_str );
    harmful = may_miss = may_crit = false;
    target              = p;
    trigger_gcd         = 0_ms;
    use_off_gcd         = true;
  }

  void execute() override
  {
    mage_spell_t::execute();

    // finishes the cooldown of every frost spell
    for ( auto a : p()->action_list )
      if ( a != this && a->school == SCHOOL_FROST && a->cooldown->down() )
        a->cooldown->reset( false );
  }
};

// ==========================================================================
// Mage character definition
// ==========================================================================

action_t* mage_t::create_action( std::string_view name, std::string_view options_str )
{
  if ( name == "fireball" )         return new fireball_t( this, options_str );
  if ( name == "frostbolt" )        return new frostbolt_t( this, options_str );
  if ( name == "fire_blast" )       return new fire_blast_t( this, options_str );
  if ( name == "scorch" )           return new scorch_t( this, options_str );
  if ( name == "pyroblast" )        return new pyroblast_t( this, options_str );
  if ( name == "ice_lance" )        return new ice_lance_t( this, options_str );
  if ( name == "arcane_blast" )     return new arcane_blast_t( this, options_str );
  if ( name == "arcane_wrath" )     return new arcane_wrath_t( this, options_str );
  if ( name == "arcane_missiles" )  return new arcane_missiles_t( this, options_str );
  if ( name == "arcane_explosion" ) return new arcane_explosion_t( this, options_str );
  if ( name == "cone_of_cold" )     return new cone_of_cold_t( this, options_str );
  if ( name == "frost_nova" )       return new frost_nova_t( this, options_str );
  if ( name == "blast_wave" )       return new blast_wave_t( this, options_str );
  if ( name == "flamestrike" )      return new flamestrike_t( this, options_str );
  if ( name == "blizzard" )         return new blizzard_t( this, options_str );
  if ( name == "evocation" )        return new evocation_t( this, options_str );
  if ( name == "mana_gem" )         return new mana_gem_t( this, options_str );
  if ( name == "cold_snap" )        return new cold_snap_t( this, options_str );

  if ( name == "mage_armor" )
    return new armor_t( name, this, options_str, mage_armor_ranks, buffs.mage_armor );
  if ( name == "ice_armor" )
    return new armor_t( name, this, options_str, ice_armor_ranks, buffs.ice_armor );
  if ( name == "frost_armor" )
    return new armor_t( name, this, options_str, frost_armor_ranks, buffs.frost_armor );

  if ( name == "arcane_power" )
    return new buff_spell_t( name, this, options_str, talents.arcane_power, buffs.arcane_power );
  if ( name == "presence_of_mind" )
    return new buff_spell_t( name, this, options_str, talents.presence_of_mind, buffs.presence_of_mind );
  if ( name == "combustion" )       return new combustion_t( this, options_str );

  return player_t::create_action( name, options_str );
}

void mage_t::create_options()
{
  player_t::create_options();

  add_option( opt_bool( "freezable_target", options.freezable_target ) );
}

void mage_t::create_actions()
{
  ignite = new ignite_t( this );

  player_t::create_actions();
}

void mage_t::init_spells()
{
  player_t::init_spells();

  auto t = [ this ]( std::string_view n ) { return find_talent_spell( talent_tree::CLASS, n ); };

  talents.wand_specialization   = t( "Wand Specialization" );
  talents.arcane_focus          = t( "Arcane Focus" );
  talents.improved_channeling   = t( "Improved Channeling" );
  talents.missile_barrage       = t( "Missile Barrage" );
  talents.arcane_subtlety       = t( "Arcane Subtlety" );
  talents.magic_absorption      = t( "Magic Absorption" );
  talents.arcane_concentration  = t( "Arcane Concentration" );
  talents.arcane_resilience     = t( "Arcane Resilience" );
  talents.arcane_geometry       = t( "Arcane Geometry" );
  talents.arcane_impact         = t( "Arcane Impact" );
  talents.arcane_blast          = t( "Arcane Blast" );
  talents.arcane_shielding      = t( "Arcane Shielding" );
  talents.improved_counterspell = t( "Improved Counterspell" );
  talents.arcane_meditation     = t( "Arcane Meditation" );
  talents.arcane_wrath          = t( "Arcane Wrath" );
  talents.presence_of_mind      = t( "Presence of Mind" );
  talents.arcane_mind           = t( "Arcane Mind" );
  talents.arcane_instability    = t( "Arcane Instability" );
  talents.arcane_power          = t( "Arcane Power" );

  talents.improved_fire_blast  = t( "Improved Fire Blast" );
  talents.incineration         = t( "Incineration" );
  talents.improved_fireball    = t( "Improved Fireball" );
  talents.ignite               = t( "Ignite" );
  talents.flame_throwing       = t( "Flame Throwing" );
  talents.impact               = t( "Impact" );
  talents.burning_soul         = t( "Burning Soul" );
  talents.improved_flamestrike = t( "Improved Flamestrike" );
  talents.pyroblast            = t( "Pyroblast" );
  talents.improved_scorch      = t( "Improved Scorch" );
  talents.improved_fire_ward   = t( "Improved Fire Ward" );
  talents.heating_up           = t( "Heating Up" );
  talents.master_of_elements   = t( "Master of Elements" );
  talents.critical_mass        = t( "Critical Mass" );
  talents.blast_wave           = t( "Blast Wave" );
  talents.fire_power           = t( "Fire Power" );
  talents.combustion           = t( "Combustion" );

  talents.frost_warding         = t( "Frost Warding" );
  talents.improved_frostbolt    = t( "Improved Frostbolt" );
  talents.elemental_precision   = t( "Elemental Precision" );
  talents.ice_shards            = t( "Ice Shards" );
  talents.frostbite             = t( "Frostbite" );
  talents.improved_frost_nova   = t( "Improved Frost Nova" );
  talents.permafrost            = t( "Permafrost" );
  talents.piercing_ice          = t( "Piercing Ice" );
  talents.frost_channeling      = t( "Frost Channeling" );
  talents.ice_lance             = t( "Ice Lance" );
  talents.improved_blizzard     = t( "Improved Blizzard" );
  talents.arctic_reach          = t( "Arctic Reach" );
  talents.ice_block             = t( "Ice Block" );
  talents.shatter               = t( "Shatter" );
  talents.improved_cone_of_cold = t( "Improved Cone of Cold" );
  talents.cold_snap             = t( "Cold Snap" );
  talents.fingers_of_frost      = t( "Fingers of Frost" );
  talents.winters_chill         = t( "Winter's Chill" );
  talents.ice_barrier           = t( "Ice Barrier" );

  parse_all_class_passives();
  parse_all_passive_talents();
}

void mage_t::create_buffs()
{
  player_t::create_buffs();

  buffs.mage_armor  = make_buff( this, "mage_armor", find_spell( max_rank( this, mage_armor_ranks ).spell_id ) );
  buffs.ice_armor   = make_buff( this, "ice_armor", find_spell( max_rank( this, ice_armor_ranks ).spell_id ) );
  buffs.frost_armor = make_buff( this, "frost_armor", find_spell( max_rank( this, frost_armor_ranks ).spell_id ) );

  buffs.evocation = make_buff( this, "evocation", find_spell( 12051 ) )->set_cooldown( 0_ms );

  buffs.arcane_power = make_buff( this, "arcane_power", talents.arcane_power.spell() )->set_cooldown( 0_ms );
  buffs.presence_of_mind =
    make_buff( this, "presence_of_mind", talents.presence_of_mind.spell() )->set_cooldown( 0_ms );
  buffs.combustion = make_buff( this, "combustion", find_spell( 28682 ) )->set_cooldown( 0_ms );
  buffs.combustion_crits = make_buff( this, "combustion_crits", talents.combustion.spell() )
                             ->set_cooldown( 0_ms )
                             ->set_duration( 0_ms )
                             ->set_max_stack( std::max( 1u, talents.combustion->max_stacks() ) );

  buffs.clearcasting     = make_buff( this, "clearcasting", find_spell( 12536 ) );
  buffs.heating_up       = make_buff( this, "heating_up", find_spell( 400625 ) );
  buffs.missile_barrage  = make_buff( this, "missile_barrage", find_spell( 400589 ) );
  buffs.arcane_blast     = make_buff( this, "arcane_blast", find_spell( 400573 ) )
                         ->set_default_value_from_effect( 1 )
                         ->add_invalidate( CACHE_SPELL_POWER );
  buffs.fingers_of_frost = make_buff( this, "fingers_of_frost", find_spell( 400669 ) )
                             ->set_max_stack( 2 );
}

void mage_t::init_base_stats()
{
  player_t::init_base_stats();
}

void mage_t::init_gains()
{
  player_t::init_gains();

  gains.evocation          = get_gain( "Evocation" );
  gains.master_of_elements = get_gain( "Master of Elements" );
  gains.mana_gem           = get_gain( "Mana Gem" );
}

void mage_t::init_procs()
{
  player_t::init_procs();

  procs.ignite_munch = get_proc( "Ignite Munch" );
}

void mage_t::reset()
{
  player_t::reset();
}

// Fraction of spirit-based regeneration that continues while casting.
double mage_t::mana_regen_while_casting() const
{
  double pct = 0.0;

  if ( buffs.mage_armor->check() )
    pct += buffs.mage_armor->data().effectN( 2 ).percent();
  if ( buffs.evocation->check() )
    pct += buffs.evocation->data().effectN( 2 ).percent();
  if ( talents.arcane_meditation.ok() )
    pct += talents.arcane_meditation->effectN( 1 ).percent();

  return std::min( pct, 1.0 );
}

double mage_t::composite_spell_power( school_e school ) const
{
  double sp = player_t::composite_spell_power( school );

  // Arcane Blast: spell damage equal to a share of intellect per stack
  sp += buffs.arcane_blast->check_stack_value() * cache.intellect();

  return sp;
}

double mage_t::resource_regen_per_second( resource_e r ) const
{
  if ( r != RESOURCE_MANA )
    return player_t::resource_regen_per_second( r );

  double reg    = resources.base_regen_per_second[ r ];
  double spirit = mana_regen_from_spirit();

  reg += recent_cast() ? spirit * mana_regen_while_casting() : spirit;

  if ( buffs.evocation->check() )
    reg *= 1.0 + buffs.evocation->data().effectN( 1 ).percent();

  return reg;
}

mage_t::mage_tree_e mage_t::primary_tree() const
{
  // the class tree places arcane in columns 1-4, fire in 5-8 and frost in 9-12
  std::array<unsigned, 3> points{};

  for ( auto [ tree, entry_id, rank ] : player_traits )
  {
    if ( !rank || tree != talent_tree::CLASS )
      continue;

    if ( auto trait = trait_data_t::find( entry_id, is_ptr() ); trait && trait->id_trait_node_entry )
      points[ trait->col <= 4 ? 0 : trait->col <= 8 ? 1 : 2 ] += rank;
  }

  if ( points[ 1 ] >= points[ 0 ] && points[ 1 ] >= points[ 2 ] )
    return mage_tree_e::FIRE;
  if ( points[ 2 ] >= points[ 0 ] )
    return mage_tree_e::FROST;
  return mage_tree_e::ARCANE;
}

void mage_t::init_action_list()
{
  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }

  clear_action_priority_lists();

  switch ( primary_tree() )
  {
    case mage_tree_e::ARCANE: mage_apl::arcane( this ); break;
    case mage_tree_e::FIRE:   mage_apl::fire( this ); break;
    case mage_tree_e::FROST:  mage_apl::frost( this ); break;
  }

  use_default_action_list = true;
  player_t::init_action_list();
}

// ==========================================================================
// Mage module
// ==========================================================================

struct mage_module_t final : public module_t
{
  mage_module_t() : module_t( MAGE ) {}

  player_t* create_player( sim_t* sim, std::string_view name, race_e r = RACE_NONE ) const override
  {
    return new mage_t( sim, name, r );
  }

  bool valid() const override { return true; }

  void register_hotfixes() const override {}

  void register_actor_initializers( sim_t* ) const override {}
};

}  // namespace mage

const module_t* module_t::mage()
{
  static mage::mage_module_t m;
  return &m;
}
