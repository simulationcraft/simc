// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SC_WARLOCK_PETS_HPP
#define SC_WARLOCK_PETS_HPP

#include "simulationcraft.hpp"

namespace warlock
{
// Forward declarations
struct warlock_t;
struct warlock_td_t;
struct warlock_pet_t;

struct warlock_pet_td_t : public actor_target_data_t
{
  propagate_const<buff_t*> debuff_whiplash;

  warlock_pet_t& pet;
  warlock_pet_td_t( player_t*, warlock_pet_t& );
};

struct warlock_pet_t : public pet_t
{
  action_t* special_action; // Used for pet interrupts (Axe Toss, Spell Lock)
  melee_attack_t* melee_attack;
  stats_t* summon_stats;

  struct buffs_t
  {
    propagate_const<buff_t*> embers;  // Infernal Shard Generation
    propagate_const<buff_t*> demonic_strength; // Talent that buffs Felguard
    propagate_const<buff_t*> grimoire_of_service; // Buff used by Grimoire: Felguard talent
    propagate_const<buff_t*> annihilan_training; // Permanent aura when talented, 10% increased damage to all abilities
    propagate_const<buff_t*> dread_calling;
    propagate_const<buff_t*> imp_gang_boss; // Aura applied to some Wild Imps for increased damage (and size)
    propagate_const<buff_t*> antoran_armaments; // Permanent aura when talented, 20% increased damage to all abilities plus Soul Strike cleave
    propagate_const<buff_t*> the_expendables;
    propagate_const<buff_t*> reign_of_tyranny;
    propagate_const<buff_t*> fiendish_wrath; // Guillotine talent buff, causes AoE melee attacks and prevents Felstorm
    propagate_const<buff_t*> demonic_inspiration; // Haste buff triggered by filling a Soul Shard
    propagate_const<buff_t*> wrathful_minion; // Damage buff triggered by filling a Soul Shard
    propagate_const<buff_t*> demonic_power;
    propagate_const<buff_t*> empowered_legion_strike; // TWW1 Demonology 4pc buff
    propagate_const<buff_t*> demonic_hunger; // TWW2 Demonology 2pc buff
    propagate_const<buff_t*> spliced_4pc; // TWW2 Demonology 4pc dummy buff
  } buffs;

  bool is_main_pet = false;
  bool melee_on_summon = true; // Set this to false for a pet to prevent t=0 melees. You MUST schedule a new auto attack manually elsewhere in the implementation if this is disabled

  warlock_pet_t( warlock_t*, util::string_view, pet_e, bool = false );
  void init_base_stats() override;
  void init_action_list() override;
  void create_buffs() override;
  void schedule_ready( timespan_t = 0_ms, bool = false ) override;
  double composite_player_multiplier( school_e ) const override;
  double composite_spell_haste() const override;
  double composite_spell_cast_speed() const override;
  double composite_melee_auto_attack_speed() const override;
  double composite_player_critical_damage_multiplier( const action_state_t* ) const override;
  void arise() override;
  void demise() override;

  target_specific_t<warlock_pet_td_t> target_data;

  const warlock_pet_td_t* find_target_data( const player_t* target ) const override
  { return target_data[ target ]; }

  warlock_pet_td_t* get_target_data( player_t* target ) const override
  {
    warlock_pet_td_t*& td = target_data[ target ];
    if ( !td )
    {
      td = new warlock_pet_td_t( target, const_cast<warlock_pet_t&>( *this ) );
    }
    return td;
  }

  resource_e primary_resource() const override
  { return RESOURCE_ENERGY; }

  warlock_t* o();
  const warlock_t* o() const;

  // Pet action to simulate travel time. Places actor at distance 1.0.
  // "Executes" for a length of time it would take to travel from current distance to 1.0 at 33 yds/sec
  struct travel_t : public action_t
  {
    double speed;
    double melee_pos;

    travel_t( warlock_pet_t* player ) : travel_t( player, "travel" )
    { }

    travel_t( warlock_pet_t* player, util::string_view action_name ) : action_t( ACTION_OTHER, action_name, player )
    {
      trigger_gcd = 0_ms;
      speed = 33.0;
      melee_pos = 1.0;
    }

    void execute() override
    { player->current.distance = melee_pos; }

    timespan_t execute_time() const override
    { return timespan_t::from_seconds( ( player->current.distance - melee_pos ) / speed ); }

    bool ready() override
    {
      //For now, we assume the pet does not ever need to be anywhere except the main raid target
      return ( player->current.distance > melee_pos );
    }

    bool usable_moving() const override
    { return true; }
  };

  action_t* create_action( util::string_view name, util::string_view options_str ) override
  {
    if ( name == "travel" )
      return new travel_t( this );

    return pet_t::create_action( name, options_str );
  }
};

namespace pets
{
/**
 * A simple warlock pet that has a potential melee attack, and a single on-cooldown special ability
 * that it uses on cooldown.
 *
 * The "availability" of the pet is checked against the cooldown of the special_ability member
 * variable. Regeneration of the pet is automatically disabled. The pets are presumed to be of the
 * "guardian" type (i.e. abilities not triggerable by the player).
 */
struct warlock_simple_pet_t : public warlock_pet_t
{
  warlock_simple_pet_t( warlock_t*, util::string_view, pet_e );
  timespan_t available() const override;

protected:
  action_t* special_ability;
};

// Template for common warlock pet action code.
template <class ACTION_BASE>
struct warlock_pet_action_t : public ACTION_BASE
{
private:
  typedef ACTION_BASE ab;  // action base, eg. spell_t
public:
  typedef warlock_pet_action_t base_t;

  warlock_pet_action_t( util::string_view n, warlock_pet_t* p, const spell_data_t* s = spell_data_t::nil() )
    : ab( n, p, s )
  {
    ab::may_crit = true;

    // If pets are not reported separately, create single stats_t objects for the various pet
    // abilities.
    if ( !ab::sim->report_pets_separately )
    {
      auto first_pet = p->owner->find_pet( p->name_str );
      if ( first_pet && first_pet != p )
      {
        auto it = range::find( p->stats_list, ab::stats );
        if ( it != p->stats_list.end() )
        {
          p->stats_list.erase( it );
          delete ab::stats;
          ab::stats = first_pet->get_stats( ab::name_str, this );
        }
      }
    }
  }

  warlock_pet_t* p()
  { return static_cast<warlock_pet_t*>( ab::player ); }

  const warlock_pet_t* p() const
  { return static_cast<warlock_pet_t*>( ab::player ); }

  void execute() override
  {
    ab::execute();

    // Some aoe pet abilities can actually reduce to 0 targets, so bail out early if we hit that situation
    if ( ab::n_targets() != 0 && ab::target_list().size() == 0 )
      return;
  }

  warlock_td_t* owner_td( player_t* t )
  { return p()->o()->get_target_data( t ); }

  const warlock_td_t* owner_td( player_t* t ) const
  { return p()->o()->get_target_data( t ); }

  warlock_pet_td_t* pet_td( player_t* t )
  { return p()->get_target_data( t ); }

  const warlock_pet_td_t* pet_td( player_t* t ) const
  { return p()->get_target_data( t ); }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = ab::composite_target_multiplier( target );

    if ( p()->o()->talents.shadowtouched.ok() && dbc::has_common_school( ab::get_school(), SCHOOL_SHADOW ) && owner_td( target )->debuffs_wicked_maw->check() )
      m *= 1.0 + p()->o()->talents.shadowtouched->effectN( 1 ).percent();

    return m;
  }
};

// TODO: Switch to a general autoattack template if one is added
struct warlock_pet_melee_t : public warlock_pet_action_t<melee_attack_t>
{
  bool first; // Needed for t=0 autoattack execution
  warlock_pet_action_t<melee_attack_t>* oh;

  warlock_pet_melee_t( warlock_pet_t* p, double wm = 1.0, const char* name = "melee" )
    : warlock_pet_action_t<melee_attack_t>( name, p, spell_data_t::nil() ), oh( nullptr )
  {
    school            = SCHOOL_PHYSICAL;
    weapon            = &( p->main_hand_weapon );
    weapon_multiplier = wm;
    base_execute_time = weapon->swing_time;
    may_crit = background = repeating = true;

    if ( p->dual_wield() )
    {
      oh = new warlock_pet_action_t<melee_attack_t>( "melee_oh", p, spell_data_t::nil() );
      oh->school = SCHOOL_PHYSICAL;
      oh->weapon = &( p->off_hand_weapon );
      oh->weapon_multiplier = wm;
      oh->base_execute_time = weapon->swing_time;
      oh->may_crit = oh->background = true;
      oh->base_multiplier = 0.5;
    }
  }

  void reset() override
  {
    warlock_pet_action_t::reset();

    first = true;
  }

  timespan_t execute_time() const override
  { return first ? 0_ms : warlock_pet_action_t::execute_time(); }

  void execute() override
  {
    if ( first )
      first = false;

    if ( !player->executing && !player->channeling )
    {
      melee_attack_t::execute();
      if ( oh )
      {
        oh->time_to_execute = time_to_execute;
        oh->execute();
      }
    }
    else
    {
      schedule_execute();
    }
  }
};

struct warlock_pet_melee_attack_t : public warlock_pet_action_t<melee_attack_t>
{
private:
  void _init_warlock_pet_melee_attack_t()
  {
    weapon  = &( player->main_hand_weapon );
    special = true;
  }

public:
  warlock_pet_melee_attack_t( warlock_pet_t* p, util::string_view n ) : base_t( n, p, p->find_pet_spell( n ) )
  { _init_warlock_pet_melee_attack_t(); }

  warlock_pet_melee_attack_t( util::string_view token, warlock_pet_t* p, const spell_data_t* s = spell_data_t::nil() )
    : base_t( token, p, s )
  { _init_warlock_pet_melee_attack_t(); }
};

struct warlock_pet_spell_t : public warlock_pet_action_t<spell_t>
{
public:
  warlock_pet_spell_t( warlock_pet_t* p, util::string_view n ) : base_t( n, p, p->find_pet_spell( n ) )
  { }

  warlock_pet_spell_t( util::string_view token, warlock_pet_t* p, const spell_data_t* s = spell_data_t::nil() )
    : base_t( token, p, s )
  { }
};

namespace base
{
struct felhunter_pet_t : public warlock_pet_t
{
  felhunter_pet_t( warlock_t*, util::string_view );
  void init_base_stats() override;
  action_t* create_action( util::string_view, util::string_view ) override;
};

struct imp_pet_t : public warlock_pet_t
{
  double firebolt_cost;

  imp_pet_t( warlock_t*, util::string_view );
  action_t* create_action( util::string_view, util::string_view ) override;
  timespan_t available() const override;
};

struct sayaad_pet_t : public warlock_pet_t
{
  sayaad_pet_t( warlock_t*, util::string_view );
  void init_base_stats() override;
  action_t* create_action( util::string_view, util::string_view ) override;
  double composite_player_target_multiplier( player_t*, school_e ) const override;
};

struct voidwalker_pet_t : public warlock_pet_t
{
  voidwalker_pet_t( warlock_t*, util::string_view );
  void init_base_stats() override;
  action_t* create_action( util::string_view, util::string_view ) override;
};

}  // namespace base

namespace demonology
{
struct felguard_pet_t : public warlock_pet_t
{
  action_t* soul_strike;
  action_t* felguard_guillotine;
  action_t* hatred_proc;
  cooldown_t* felstorm_cd;
  cooldown_t* dstr_cd;
  int demonic_strength_executes;

  // Energy thresholds to wake felguard up for something to do, minimum is the felstorm energy cost,
  // and maximum is a predetermined empirical value from in game
  double min_energy_threshold;
  double max_energy_threshold;

  felguard_pet_t( warlock_t*, util::string_view );
  void init_base_stats() override;
  action_t* create_action( util::string_view, util::string_view ) override;
  timespan_t available() const override;
  void arise() override;
  double composite_player_multiplier( school_e ) const override;
  double composite_melee_auto_attack_speed() const override;
  double composite_melee_crit_chance() const override;
  double composite_spell_crit_chance() const override;

  void queue_ds_felstorm();
};

struct grimoire_felguard_pet_t : public warlock_pet_t
{
  const spell_data_t* felstorm_spell;
  cooldown_t* felstorm_cd;

  // Energy thresholds to wake felguard up for something to do, minimum is the felstorm energy cost,
  // and maximum is a predetermined empirical value from in game
  double min_energy_threshold;
  double max_energy_threshold;

  grimoire_felguard_pet_t( warlock_t* );
  void init_base_stats() override;
  action_t* create_action( util::string_view, util::string_view ) override;
  timespan_t available() const override;
  void arise() override;
  void demise() override;
  double composite_player_multiplier( school_e ) const override;
};

struct wild_imp_pet_t : public warlock_pet_t
{
  action_t* firebolt;
  bool power_siphon;
  bool imploded;

  wild_imp_pet_t( warlock_t* );
  void init_base_stats() override;
  void create_actions() override;
  void schedule_ready( timespan_t, bool ) override;
  void arise() override;
  void demise() override;
  void finish_moving() override;
  double composite_player_multiplier( school_e ) const override;

private:
  void reschedule_firebolt();
};

struct dreadstalker_t : public warlock_pet_t
{
  int dreadbite_executes;
  timespan_t server_action_delay;

  dreadstalker_t( warlock_t* );
  dreadstalker_t( warlock_t*, util::string_view, pet_e );
  void init_base_stats() override;
  void arise() override;
  void demise() override;
  timespan_t available() const override;
  action_t* create_action( util::string_view, util::string_view ) override;
  double composite_player_multiplier( school_e ) const override;
  double composite_melee_crit_chance() const override;
  double composite_spell_crit_chance() const override;
  void queue_dreadbite();
};

struct vilefiend_t : public warlock_simple_pet_t
{
  int bile_spit_executes;
  buff_t* infernal_presence;
  buff_t* mark_of_shatug; // Dummy buff to track if this is a Gloomhound
  buff_t* mark_of_fharg; // Dummy buff to track if this is a Charhound

  vilefiend_t( warlock_t* );
  void init_base_stats() override;
  void create_buffs() override;
  void arise() override;
  action_t* create_action( util::string_view, util::string_view ) override;
  double composite_player_multiplier( school_e ) const override;
};

struct demonic_tyrant_t : public warlock_pet_t
{
  demonic_tyrant_t( warlock_t*, util::string_view = "demonic_tyrant" );
  action_t* create_action( util::string_view, util::string_view ) override;
  double composite_player_multiplier( school_e ) const override;
};

struct doomguard_t : public warlock_simple_pet_t
{
  int doom_bolt_executes;

  doomguard_t( warlock_t* );
  void init_base_stats() override;
  action_t* create_action( util::string_view, util::string_view ) override;
  void arise() override;
};

struct greater_dreadstalker_t : public dreadstalker_t
{
  greater_dreadstalker_t( warlock_t* );
  void arise() override;
  void demise() override;
  double composite_player_multiplier( school_e ) const override;
  double composite_melee_crit_chance() const override;
  double composite_spell_crit_chance() const override;
};
}  // namespace demonology

namespace destruction
{
struct infernal_t : public warlock_pet_t
{
  buff_t* immolation;
  enum infernal_type_e { MAIN, RAIN, FRAG };
  infernal_type_e type;

  infernal_t( warlock_t*, util::string_view = "infernal" );
  void init_base_stats() override;
  void create_buffs() override;
  void arise() override;
  void demise() override;
  double composite_player_multiplier( school_e ) const override;
};

struct shadowy_tear_t : public warlock_pet_t
{
  int barrages;
  action_t* cinder;

  shadowy_tear_t( warlock_t*, util::string_view = "Shadowy Tear" );
  void arise() override;
  action_t* create_action( util::string_view, util::string_view ) override;
};

struct unstable_tear_t : public warlock_pet_t
{
  int barrages;
  action_t* cinder;

  unstable_tear_t( warlock_t*, util::string_view = "Unstable Tear" );
  void arise() override;
  action_t* create_action( util::string_view, util::string_view ) override;
};

struct chaos_tear_t : public warlock_pet_t
{
  int bolts;
  action_t* cinder;

  chaos_tear_t( warlock_t*, util::string_view = "Chaos Tear" );
  void arise() override;
  action_t* create_action( util::string_view, util::string_view ) override;
};

struct overfiend_t : public warlock_pet_t
{
  overfiend_t( warlock_t*, util::string_view = "Overfiend" );
  action_t* create_action( util::string_view, util::string_view ) override;
};
}  // namespace destruction

namespace affliction
{
struct darkglare_t : public warlock_pet_t
{
  darkglare_t( warlock_t*, util::string_view = "darkglare" );
  action_t* create_action( util::string_view , util::string_view ) override;
};
}  // namespace affliction

namespace diabolist
{
  struct overlord_t : public warlock_pet_t
  {
    int cleaves;

    overlord_t( warlock_t*, util::string_view = "overlord" );
    void arise() override;
    action_t* create_action( util::string_view, util::string_view ) override;
  };

  struct mother_of_chaos_t : public warlock_pet_t
  {
    int salvos;

    mother_of_chaos_t( warlock_t*, util::string_view = "mother_of_chaos" );
    void arise() override;
    action_t* create_action( util::string_view, util::string_view ) override;
  };

  struct pit_lord_t : public warlock_pet_t
  {
    int felseekers;

    pit_lord_t( warlock_t*, util::string_view = "pit_lord" );
    void arise() override;
    void init_base_stats() override;
    action_t* create_action( util::string_view, util::string_view ) override;
  };

  struct infernal_fragment_t : public destruction::infernal_t
  {
    infernal_fragment_t( warlock_t*, util::string_view = "infernal_fragment" );
  };

  struct diabolic_imp_t : public warlock_pet_t
  {
    int bolts;

    diabolic_imp_t( warlock_t*, util::string_view = "diabolic_imp" );
    void arise() override;
    action_t* create_action( util::string_view, util::string_view ) override;
  };
}  // namespace diabolist
}  // namespace pets
}  // namespace warlock

#endif /* SC_WARLOCK_PETS_HPP */
