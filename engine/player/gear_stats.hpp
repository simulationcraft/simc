// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "util/generic.hpp"
#include "util/format.hpp"
#include "sc_enums.hpp"

#include <array>
#include <functional>


struct gear_stats_t
{
  double default_value;

  std::array<double, ATTRIBUTE_MAX> attribute;
  std::array<double, RESOURCE_MAX> resource;
  double spell_power;
  double attack_power;
  double expertise_rating;
  double expertise_rating2;
  double hit_rating;
  double hit_rating2;
  double crit_rating;
  double haste_rating;
  double weapon_dps;
  double weapon_speed;
  double weapon_offhand_dps;
  double weapon_offhand_speed;
  double armor;
  double bonus_armor;
  double dodge_rating;
  double parry_rating;
  double block_rating;
  double mastery_rating;
  double resilience_rating;
  double pvp_power;
  double versatility_rating;
  double leech_rating;
  double speed_rating;
  double avoidance_rating;
  double corruption_rating;
  double corruption_resistance_rating;

  gear_stats_t() :
    default_value( 0.0 ), attribute(), resource(),
    spell_power( 0.0 ), attack_power( 0.0 ), expertise_rating( 0.0 ), expertise_rating2( 0.0 ),
    hit_rating( 0.0 ), hit_rating2( 0.0 ), crit_rating( 0.0 ), haste_rating( 0.0 ), weapon_dps( 0.0 ), weapon_speed( 0.0 ),
    weapon_offhand_dps( 0.0 ), weapon_offhand_speed( 0.0 ), armor( 0.0 ), bonus_armor( 0.0 ), dodge_rating( 0.0 ),
    parry_rating( 0.0 ), block_rating( 0.0 ), mastery_rating( 0.0 ), resilience_rating( 0.0 ), pvp_power( 0.0 ),
    versatility_rating( 0.0 ), leech_rating( 0.0 ), speed_rating( 0.0 ),
    avoidance_rating( 0.0 ), corruption_rating( 0.0 ), corruption_resistance_rating( 0.0 )
  { }

  void initialize( double initializer )
  {
    default_value = initializer;

    range::fill( attribute, initializer );
    range::fill( resource, initializer );

    spell_power = initializer;
    attack_power = initializer;
    expertise_rating = initializer;
    expertise_rating2 = initializer;
    hit_rating = initializer;
    hit_rating2 = initializer;
    crit_rating = initializer;
    haste_rating = initializer;
    weapon_dps = initializer;
    weapon_speed = initializer;
    weapon_offhand_dps = initializer;
    weapon_offhand_speed = initializer;
    armor = initializer;
    bonus_armor = initializer;
    dodge_rating = initializer;
    parry_rating = initializer;
    block_rating = initializer;
    mastery_rating = initializer;
    resilience_rating = initializer;
    pvp_power = initializer;
    versatility_rating = initializer;
    leech_rating = initializer;
    speed_rating = initializer;
    avoidance_rating = initializer;
    corruption_rating = initializer;
    corruption_resistance_rating = initializer;
  }

  friend gear_stats_t operator+( const gear_stats_t& left, const gear_stats_t& right )
  {
    gear_stats_t a = gear_stats_t( left );
    a += right;
    return a;
  }

  gear_stats_t& operator+=( const gear_stats_t& right )
  {
    spell_power += right.spell_power;
    attack_power += right.attack_power;
    expertise_rating += right.expertise_rating;
    expertise_rating2 += right.expertise_rating2;
    hit_rating += right.hit_rating;
    hit_rating2 += right.hit_rating2;
    crit_rating += right.crit_rating;
    haste_rating += right.haste_rating;
    weapon_dps += right.weapon_dps;
    weapon_speed += right.weapon_speed;
    weapon_offhand_dps += right.weapon_offhand_dps;
    weapon_offhand_speed += right.weapon_offhand_speed;
    armor += right.armor;
    bonus_armor += right.bonus_armor;
    dodge_rating += right.dodge_rating;
    parry_rating += right.parry_rating;
    block_rating += right.block_rating;
    mastery_rating += right.mastery_rating;
    resilience_rating += right.resilience_rating;
    pvp_power += right.pvp_power;
    versatility_rating += right.versatility_rating;
    leech_rating += right.leech_rating;
    speed_rating += right.speed_rating;
    avoidance_rating += right.avoidance_rating;
    corruption_rating += right.corruption_rating;
    corruption_resistance_rating += right.corruption_resistance_rating;
    range::transform ( attribute, right.attribute, attribute.begin(), std::plus<double>() );
    range::transform ( resource, right.resource, resource.begin(), std::plus<double>() );
    return *this;
  }

  void   add_stat( stat_e stat, double value );
  void   set_stat( stat_e stat, double value );
  double get_stat( stat_e stat ) const;
  static double stat_mod( stat_e stat );

  friend fmt::format_context::iterator format_to( const gear_stats_t&, fmt::format_context& );
};