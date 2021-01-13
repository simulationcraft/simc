// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "snapshot_stats.hpp"

#include "action/attack.hpp"
#include "action/spell.hpp"
#include "player/sc_player.hpp"
#include "player/pet.hpp"
#include "player/player_scaling.hpp"


snapshot_stats_t::snapshot_stats_t(player_t* player, util::string_view options_str) :
  action_t(ACTION_OTHER, "snapshot_stats", player),
  completed(false),
  proxy_spell(nullptr),
  proxy_attack(nullptr),
  role(player->primary_role())
{
  parse_options(options_str);
  trigger_gcd = timespan_t::zero();
  harmful = false;

  if (role == ROLE_SPELL || role == ROLE_HYBRID || role == ROLE_HEAL)
  {
    proxy_spell = new spell_t("snapshot_spell", player);
    proxy_spell->background = true;
    proxy_spell->callbacks = false;
  }

  if (role == ROLE_ATTACK || role == ROLE_HYBRID || role == ROLE_TANK)
  {
    proxy_attack = new melee_attack_t("snapshot_attack", player);
    proxy_attack->background = true;
    proxy_attack->callbacks = false;
  }
}

void snapshot_stats_t::init_finished()
{
  player_t* p = player;
  for (size_t i = 0; sim->report_pets_separately && i < p->pet_list.size(); ++i)
  {
    pet_t* pet = p->pet_list[i];
    action_t* pet_snapshot = pet->find_action("snapshot_stats");
    if (!pet_snapshot)
    {
      pet_snapshot = pet->create_action("snapshot_stats", "");
      pet_snapshot->init();
    }
  }

  action_t::init_finished();
}

void snapshot_stats_t::execute()
{
  player_t* p = player;

  if (completed)
    return;

  completed = true;

  if (p->nth_iteration() > 0)
    return;

  if (sim->log)
    sim->out_log.printf("%s performs %s", p->name(), name());

  player_collected_data_t::buffed_stats_t& buffed_stats = p->collected_data.buffed_stats_snapshot;

  for (attribute_e i = ATTRIBUTE_NONE; i < ATTRIBUTE_MAX; ++i)
    buffed_stats.attribute[i] = floor(p->get_attribute(i));

  buffed_stats.resource = p->resources.max;

  buffed_stats.spell_haste = p->cache.spell_haste();
  buffed_stats.spell_speed = p->cache.spell_speed();
  buffed_stats.attack_haste = p->cache.attack_haste();
  buffed_stats.attack_speed = p->cache.attack_speed();
  buffed_stats.mastery_value = p->cache.mastery_value();
  buffed_stats.bonus_armor = p->composite_bonus_armor();
  buffed_stats.damage_versatility = p->cache.damage_versatility();
  buffed_stats.heal_versatility = p->cache.heal_versatility();
  buffed_stats.mitigation_versatility = p->cache.mitigation_versatility();
  buffed_stats.run_speed = p->cache.run_speed();
  buffed_stats.avoidance = p->cache.avoidance();
  buffed_stats.corruption = p->cache.corruption();
  buffed_stats.corruption_resistance = p->cache.corruption_resistance();
  buffed_stats.leech = p->cache.leech();

  buffed_stats.spell_power =
    util::round(p->cache.spell_power(SCHOOL_MAX) * p->composite_spell_power_multiplier());
  buffed_stats.spell_hit = p->cache.spell_hit();
  buffed_stats.spell_crit_chance = p->cache.spell_crit_chance();
  buffed_stats.manareg_per_second = p->resource_regen_per_second(RESOURCE_MANA);

  buffed_stats.attack_power = p->cache.attack_power() * p->composite_attack_power_multiplier();
  buffed_stats.attack_hit = p->cache.attack_hit();
  buffed_stats.mh_attack_expertise = p->composite_melee_expertise(&(p->main_hand_weapon));
  buffed_stats.oh_attack_expertise = p->composite_melee_expertise(&(p->off_hand_weapon));
  buffed_stats.attack_crit_chance = p->cache.attack_crit_chance();

  buffed_stats.armor = p->composite_armor();
  buffed_stats.miss = p->composite_miss();
  buffed_stats.dodge = p->cache.dodge();
  buffed_stats.parry = p->cache.parry();
  buffed_stats.block = p->cache.block();
  buffed_stats.crit = p->cache.crit_avoidance();

  double spell_hit_extra  = 0;
  double attack_hit_extra = 0;
  double expertise_extra  = 0;

  // The code below is not properly handling the case where the player has
  // so much Hit Rating or Expertise Rating that the extra amount of stat
  // they have is higher than the delta.

  // In this case, the following line in sc_scaling.cpp
  //     if ( divisor < 0.0 ) divisor += ref_p -> over_cap[ i ];
  // would give divisor a positive value (whereas it is expected to always
  // remain negative).
  // Also, if a player has an extra amount of Hit Rating or Expertise Rating
  // that is equal to the ``delta'', the following line in sc_scaling.cpp
  //     double score = ( delta_score - ref_score ) / divisor;
  // will cause a division by 0 error.
  // We would need to increase the delta before starting the scaling sim to remove this error

  if (role == ROLE_SPELL || role == ROLE_HYBRID || role == ROLE_HEAL)
  {
    double chance = proxy_spell->miss_chance(proxy_spell->composite_hit(), sim->target);
    if (chance < 0)
      spell_hit_extra = -chance * p->current.rating.spell_hit;
  }

  if (role == ROLE_ATTACK || role == ROLE_HYBRID || role == ROLE_TANK)
  {
    double chance = proxy_attack->miss_chance(proxy_attack->composite_hit(), sim->target);
    if (p->dual_wield())
      chance += 0.19;
    if (chance < 0)
      attack_hit_extra = -chance * p->current.rating.attack_hit;
    if (p->position() != POSITION_FRONT)
    {
      chance = proxy_attack->dodge_chance(p->cache.attack_expertise(), sim->target);
      if (chance < 0)
        expertise_extra = -chance * p->current.rating.expertise;
    }
    else if (p->position() == POSITION_FRONT)
    {
      chance = proxy_attack->parry_chance(p->cache.attack_expertise(), sim->target);
      if (chance < 0)
        expertise_extra = -chance * p->current.rating.expertise;
    }
  }

  if (p->scaling)
  {
    p->scaling->over_cap[STAT_HIT_RATING] = std::max(spell_hit_extra, attack_hit_extra);
    p->scaling->over_cap[STAT_EXPERTISE_RATING] = expertise_extra;
  }

  for (size_t i = 0; i < p->pet_list.size(); ++i)
  {
    pet_t* pet = p->pet_list[i];
    action_t* pet_snapshot = pet->find_action("snapshot_stats");
    if (pet_snapshot)
    {
      pet_snapshot->execute();
    }
  }
}

void snapshot_stats_t::reset()
{
  action_t::reset();

  completed = false;
}

bool snapshot_stats_t::ready()
{
  if (completed || player->nth_iteration() > 0)
    return false;
  return action_t::ready();
}
