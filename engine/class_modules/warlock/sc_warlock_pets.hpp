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

namespace pets
{
struct warlock_pet_t : public pet_t
{
  action_t* special_action;
  action_t* special_action_two;
  melee_attack_t* melee_attack;
  stats_t* summon_stats;
  spell_t *ascendance;

  struct buffs_t
  {
    propagate_const<buff_t*> embers;

    propagate_const<buff_t*> the_expendables;
    propagate_const<buff_t*> rage_of_guldan;
    propagate_const<buff_t*> demonic_strength;
    propagate_const<buff_t*> demonic_consumption;
    propagate_const<buff_t*> grimoire_of_service;
  } buffs;

  struct active_t
  {
    spell_t*        bile_spit;
  } active;

  bool is_demonbolt_enabled = true;
  bool is_lord_of_flames = false;
  bool t21_4pc_reset = false;
  bool is_warlock_pet = true;
  int bites_executed = 0;
  int dreadbite_executes = 0;

  warlock_pet_t( warlock_t* owner, const std::string& pet_name, pet_e pt, bool guardian = false );
  void init_base_stats() override;
  void init_action_list() override;
  void create_buffs() override;
  void schedule_ready( timespan_t delta_time = timespan_t::zero(), bool waiting = false ) override;
  double composite_player_multiplier( school_e school ) const override;
  double composite_melee_crit_chance() const override;
  double composite_spell_crit_chance() const override;
  double composite_melee_haste() const override;
  double composite_spell_haste() const override;
  double composite_melee_speed() const override;
  double composite_spell_speed() const override;

  void create_buffs_pets();
  void create_buffs_demonology();
  void init_spells_pets();

  void create_buffs_destruction();

  resource_e primary_resource() const override { return RESOURCE_ENERGY; }

  warlock_t* o();
  const warlock_t* o() const;

  void trigger_sephuzs_secret( const action_state_t* state, spell_mechanic mechanic );

  struct travel_t : public action_t
  {
    travel_t( player_t* player ) : action_t( ACTION_OTHER, "travel", player )
    {
      trigger_gcd = timespan_t::zero();
    }
    void execute() override { player->current.distance = 1; }
    timespan_t execute_time() const override { return timespan_t::from_seconds( player->current.distance / 33.0 ); }
    bool ready() override { return ( player->current.distance > 1 ); }
    bool usable_moving() const override { return true; }
  };

  action_t* create_action( const std::string& name,
    const std::string& options_str ) override
  {
    if ( name == "travel" ) return new travel_t( this );

    return pet_t::create_action( name, options_str );
  }
};

// Template for common warlock pet action code. See priest_action_t.
template <class ACTION_BASE>
struct warlock_pet_action_t : public ACTION_BASE
{
private:
  typedef ACTION_BASE ab; // action base, eg. spell_t
public:
  typedef warlock_pet_action_t base_t;

  warlock_pet_action_t( const std::string& n, warlock_pet_t* p,
    const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, p, s )
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
  {
    return static_cast<warlock_pet_t*>( ab::player );
  }
  const warlock_pet_t* p() const
  {
    return static_cast<warlock_pet_t*>( ab::player );
  }

  virtual void execute()
  {
    ab::execute();

    // Some aoe pet abilities can actually reduce to 0 targets, so bail out early if we hit that
    // situation
    if ( ab::n_targets() != 0 && ab::target_list().size() == 0 )
    {
      return;
    }
  }

  warlock_td_t* td( player_t* t )
  {
    return p()->o()->get_target_data( t );
  }

  const warlock_td_t* td( player_t* t ) const
  {
    return p()->o()->get_target_data( t );
  }

  warlock_td_t* find_td( player_t* t )
  {
    return p()->o()->find_target_data( t );
  }

  const warlock_td_t* find_td( player_t* t ) const
  {
    return p()->o()->find_target_data( t );
  }
};

struct warlock_pet_melee_t : public warlock_pet_action_t<melee_attack_t>
{
  struct off_hand_swing : public warlock_pet_action_t<melee_attack_t>
  {
    off_hand_swing( warlock_pet_t* p, double wm, const char* name = "melee_oh" ) :
      warlock_pet_action_t<melee_attack_t>( name, p, spell_data_t::nil() )
    {
      school = SCHOOL_PHYSICAL;
      weapon = &(p->off_hand_weapon);
      weapon_multiplier = wm;
      base_execute_time = weapon->swing_time;
      may_crit = background = true;
      base_multiplier = 0.5;
    }
  };

  off_hand_swing* oh;

  warlock_pet_melee_t(warlock_pet_t* p, double wm = 1.0, const char* name = "melee") :
    warlock_pet_action_t<melee_attack_t>(name, p, spell_data_t::nil()), oh(nullptr)
  {
    school = SCHOOL_PHYSICAL;
    weapon = &(p->main_hand_weapon);
    weapon_multiplier = wm;
    base_execute_time = weapon->swing_time;
    may_crit = background = repeating = true;

    if (p->dual_wield())
      oh = new off_hand_swing(p, weapon_multiplier);
  }

  double action_multiplier() const override {
    double m = warlock_pet_action_t::action_multiplier();

    return m;
  }

  void execute() override
  {
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
    weapon = &( player->main_hand_weapon );
    special = true;
  }

public:
  warlock_pet_melee_attack_t( warlock_pet_t* p, const std::string& n ) :
    base_t( n, p, p -> find_pet_spell( n ) )
  {
    _init_warlock_pet_melee_attack_t();
  }

  warlock_pet_melee_attack_t( const std::string& token, warlock_pet_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    base_t( token, p, s )
  {
    _init_warlock_pet_melee_attack_t();
  }
};

struct warlock_pet_spell_t : public warlock_pet_action_t<spell_t>
{
public:
  warlock_pet_spell_t( warlock_pet_t* p, const std::string& n ) :
    base_t( n, p, p -> find_pet_spell( n ) )
  { }

  warlock_pet_spell_t( const std::string& token, warlock_pet_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    base_t( token, p, s )
  { }
};

namespace base
{
struct felhunter_pet_t : public warlock_pet_t
{
  felhunter_pet_t( warlock_t* owner, const std::string& name );
  void init_base_stats() override;
  action_t* create_action( const std::string& name, const std::string& options_str ) override;
};

struct imp_pet_t : public warlock_pet_t
{
  double firebolt_cost;

  imp_pet_t( warlock_t* owner, const std::string& name );
  action_t* create_action( const std::string& name, const std::string& options_str ) override;
  timespan_t available() const override;
};

struct succubus_pet_t : public warlock_pet_t
{
  succubus_pet_t( warlock_t* owner, const std::string& name );
  void init_base_stats() override;
  action_t* create_action( const std::string& name, const std::string& options_str ) override;
};

struct voidwalker_pet_t : warlock_pet_t
{
  voidwalker_pet_t( warlock_t* owner, const std::string& name );
  void init_base_stats() override;
  action_t* create_action( const std::string& name, const std::string& options_str ) override;
};

} // Namespace base ends

namespace demonology
{
struct felguard_pet_t : public warlock_pet_t
{
  action_t* soul_strike;
  action_t* ds_felstorm;
  cooldown_t* felstorm;

  felguard_pet_t(warlock_t* owner, const std::string& name);
  void init_base_stats() override;
  action_t* create_action(const std::string& name, const std::string& options_str) override;
  timespan_t available() const override;

  void queue_ds_felstorm();
};

struct wild_imp_pet_t : public warlock_pet_t
{
  action_t* firebolt;
  bool power_siphon;

  wild_imp_pet_t(warlock_t* owner);
  void init_base_stats() override;
  action_t* create_action(const std::string& name, const std::string& options_str) override;
  void arise() override;
  void demise() override;
};

struct dreadstalker_t : public warlock_pet_t
{
  dreadstalker_t(warlock_t* owner);
  void init_base_stats() override;
  void arise() override;
  void demise() override;
  action_t* create_action(const std::string& name, const std::string& options_str) override;
};

struct vilefiend_t : public warlock_pet_t
{
  action_t* bile_spit;

  vilefiend_t(warlock_t* owner);
  void init_base_stats() override;
  action_t* create_action(const std::string& name, const std::string& options_str) override;
};

struct demonic_tyrant_t : public warlock_pet_t
{
  demonic_tyrant_t(warlock_t* owner, const std::string& name = "demonic_tyrant");
  void init_base_stats() override;
  void demise() override;
  action_t* create_action(const std::string& name, const std::string& options_str) override;
};

namespace random_demons
{
struct shivarra_t : public warlock_pet_t
{
  action_t* multi_slash;
  shivarra_t(warlock_t* owner, const std::string& name = "shivarra");
  void init_base_stats() override;
  void arise() override;
  action_t* create_action(const std::string& name, const std::string& options_str) override;
};

struct darkhound_t : public warlock_pet_t
{
  action_t* fel_bite;
  darkhound_t(warlock_t* owner, const std::string& name = "darkhound");
  void init_base_stats() override;
  void arise() override;
  action_t* create_action(const std::string& name, const std::string& options_str) override;
};

struct bilescourge_t : public warlock_pet_t
{
  bilescourge_t(warlock_t* owner, const std::string& name = "bilescourge");
  void init_base_stats() override;
  action_t* create_action(const std::string& name, const std::string& options_str) override;
};

struct urzul_t : public warlock_pet_t
{
  action_t* many_faced_bite;
  urzul_t(warlock_t* owner, const std::string& name = "urzul");
  void init_base_stats() override;
  void arise() override;
  action_t* create_action(const std::string& name, const std::string& options_str) override;
};

struct void_terror_t : public warlock_pet_t
{
  action_t* double_breath;
  void_terror_t(warlock_t* owner, const std::string& name = "void_terror");
  void init_base_stats() override;
  void arise() override;
  action_t* create_action(const std::string& name, const std::string& options_str) override;
};

struct wrathguard_t : public warlock_pet_t
{
  action_t* overhead_assault;
  wrathguard_t(warlock_t* owner, const std::string& name = "wrathguard");
  void init_base_stats() override;
  void arise() override;
  action_t* create_action(const std::string& name, const std::string& options_str) override;
};

struct vicious_hellhound_t : public warlock_pet_t
{
  action_t* demon_fang;
  vicious_hellhound_t(warlock_t* owner, const std::string& name = "vicious_hellhound");
  void init_base_stats() override;
  void arise() override;
  action_t* create_action(const std::string& name, const std::string& options_str) override;
};

struct illidari_satyr_t : public warlock_pet_t
{
  action_t* shadow_slash;
  illidari_satyr_t(warlock_t* owner, const std::string& name = "illidari_satyr");
  void init_base_stats() override;
  void arise() override;
  action_t* create_action(const std::string& name, const std::string& options_str) override;
};

struct eyes_of_guldan_t : public warlock_pet_t
{
  eyes_of_guldan_t(warlock_t* owner, const std::string& name = "eyes_of_guldan");
  void init_base_stats() override;
  void arise() override;
  void demise() override;
  action_t* create_action(const std::string& name, const std::string& options_str) override;
};

struct prince_malchezaar_t : public warlock_pet_t
{
  prince_malchezaar_t(warlock_t* owner, const std::string& name = "prince_malchezaar");
  void arise() override;
  void demise() override;
  void init_base_stats() override;
};
} // Namespace random_pets ends
} // Namespace demonology ends

namespace destruction
{
struct infernal_t : public warlock_pet_t
{
  buff_t* immolation;

  infernal_t(warlock_t* owner, const std::string& name = "infernal");
  virtual void init_base_stats() override;
  virtual void create_buffs() override;
  virtual void arise() override;
  virtual void demise() override;
};
} // Namespace destruction ends

namespace affliction
{
struct darkglare_t : public warlock_pet_t
{
  darkglare_t(warlock_t* owner, const std::string& name = "darkglare");
  virtual double composite_player_multiplier(school_e school) const override;
  virtual action_t* create_action(const std::string& name, const std::string& options_str) override;
};
} // Namespace affliction ends
} // Namespace pets ends
} // Namespace warlock ends

#endif /* SC_WARLOCK_PETS_HPP */
