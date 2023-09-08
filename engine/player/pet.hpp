// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "player.hpp"


// Pet ======================================================================

struct pet_t : public player_t
{
  using base_t = player_t;

  std::string full_name_str;
  player_t* const owner;
  double stamina_per_owner;
  double intellect_per_owner;
  bool summoned;
  bool dynamic;
  bool affects_wod_legendary_ring;
  pet_e pet_type;
  event_t* expiration;
  timespan_t duration;
  int npc_id; // WoW NPC id, can be used to display icons/links for the pet

  struct owner_coefficients_t
  {
    double armor = 1.0;
    double health = 1.0;
    double ap_from_ap = 0.0;
    double ap_from_sp = 0.0;
    double sp_from_ap = 0.0;
    double sp_from_sp = 0.0;
  } owner_coeff;

  struct current_pet_stats_t
  {
    // current pet stats stored in these doubles, and updated on the heartbeat event
    // testing as of 6-6-2023 shows that Mastery, and Versatility update instantly, and are ommited due to that
    double attack_power_from_ap = 0.0;
    double attack_power_from_sp = 0.0;
    double spell_power_from_ap = 0.0;
    double spell_power_from_sp = 0.0;
    double composite_melee_haste = 0.0;
    double composite_melee_speed = 0.0;
    double composite_spell_haste = 0.0;
    double composite_spell_speed = 0.0;
    double composite_melee_crit = 0.0;
    double composite_spell_crit = 0.0;
  } current_pet_stats;
  bool use_delayed_pet_stat_updates;


public:
  pet_t( sim_t* sim, player_t* owner, util::string_view name, bool guardian = false, bool dynamic = false );
  pet_t( sim_t* sim, player_t* owner, util::string_view name, pet_e pet_type, bool guardian = false,
         bool dynamic = false );

  void create_options() override;
  void create_buffs() override;
  void init() override;
  void init_base_stats() override;
  void init_target() override;
  void init_finished() override;
  void reset() override;
  void assess_damage( school_e, result_amount_type, action_state_t* s ) override;
  void trigger_callbacks( proc_types, proc_types2, action_t*, action_state_t* ) override;

  virtual void summon( timespan_t duration = timespan_t::zero() );
  virtual void dismiss( bool expired = false );
  // Adjust pet remaining duration. New duration of <= 0 dismisses pet. No-op on
  // persistent pets.
  virtual void adjust_duration( timespan_t adjustment );

  void update_stats();

  const char* name() const override { return full_name_str.c_str(); }
  const player_t* get_owner_or_self() const override
  { return owner; }

  const spell_data_t* find_pet_spell( util::string_view name );

  double composite_attribute( attribute_e attr ) const override;
  double composite_player_multiplier( school_e ) const override;
  double composite_player_target_multiplier( player_t*, school_e ) const override;

  // new pet scaling by Ghostcrawler, see http://us.battle.net/wow/en/forum/topic/5889309137?page=49#977
  // http://us.battle.net/wow/en/forum/topic/5889309137?page=58#1143

  double hit_exp() const;

  double composite_movement_speed() const override
  { return owner -> composite_movement_speed(); }

  double composite_melee_expertise( const weapon_t* ) const override
  { return hit_exp(); }
  double composite_melee_hit() const override
  { return hit_exp(); }
  double composite_spell_hit() const override
  { return hit_exp() * 2.0; }

  virtual double pet_crit() const;

  double composite_melee_crit_chance() const override
  { return pet_crit(); }
  double composite_spell_crit_chance() const override
  { return pet_crit(); }

  double composite_melee_speed() const override;

  double composite_melee_haste() const override;

  void adjust_auto_attack( gcd_haste_type type ) override;

  double composite_spell_haste() const override;

  double composite_spell_speed() const override;

  double composite_bonus_armor() const override
  { return owner -> cache.bonus_armor(); }

  double composite_damage_versatility() const override
  { return owner -> cache.damage_versatility(); }

  double composite_heal_versatility() const override
  { return owner -> cache.heal_versatility(); }

  double composite_mitigation_versatility() const override
  { return owner -> cache.mitigation_versatility(); }

  double composite_melee_attack_power() const override;

  double composite_spell_power( school_e school ) const override;

  double composite_player_critical_damage_multiplier( const action_state_t* s ) const override;

  // Assuming diminishing returns are transfered to the pet as well
  double composite_dodge() const override
  { return owner -> cache.dodge(); }

  double composite_parry() const override
  { return owner -> cache.parry(); }

  // Influenced by coefficients [ 0, 1 ]
  double composite_armor() const override
  { return owner -> cache.armor() * owner_coeff.armor; }

  void init_resources( bool force ) override;
  bool requires_data_collection() const override;

  timespan_t composite_active_time() const override;

  void acquire_target( retarget_source /* event */, player_t* /* context */ = nullptr ) override;
  
  void demise() override;
  
  friend void sc_format_to( const pet_t&, fmt::format_context::iterator );
};