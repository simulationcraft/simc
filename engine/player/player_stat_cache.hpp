// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "sc_enums.hpp"
#include <array>


struct action_state_t;
struct player_t;

/* The Cache system increases simulation performance by moving the calculation point
 * from call-time to modification-time of a stat. Because a stat is called much more
 * often than it is changed, this reduces costly and unnecessary repetition of floating-point
 * operations.
 *
 * When a stat is accessed, its 'valid'-state is checked:
 *   If its true, the cached value is accessed.
 *   If it is false, the stat value is recalculated and written to the cache.
 *
 * To indicate when a stat gets modified, it needs to be 'invalidated'. Every time a stat
 * is invalidated, its 'valid'-state gets set to false.
 */

/* - To invalidate a stat, use player_t::invalidate_cache( cache_e )
 * - using player_t::stat_gain/loss automatically invalidates the corresponding cache
 * - Same goes for stat_buff_t, which works through player_t::stat_gain/loss
 * - Buffs with effects in a composite_ function need invalidates added to their buff_creator
 *
 * To create invalidation chains ( eg. Priest: Spirit invalidates Hit ) override the
 * virtual player_t::invalidate_cache( cache_e ) function.
 *
 * Attention: player_t::invalidate_cache( cache_e ) is recursive and may call itself again.
 */
struct player_stat_cache_t
{
  const player_t* player;
  mutable std::array<bool, CACHE_MAX> valid;
  mutable std::array<bool, SCHOOL_MAX + 1> spell_power_valid, player_mult_valid, player_heal_mult_valid;
  // 'valid'-states
private:
  // cached values
  mutable double _strength, _agility, _stamina, _intellect, _spirit;
  mutable std::array<double, SCHOOL_MAX + 1> _spell_power;
  mutable double  _attack_power, _total_melee_attack_power;
  mutable double _attack_expertise;
  mutable double _attack_hit, _spell_hit;
  mutable double _attack_crit_chance, _spell_crit_chance;
  mutable double _attack_haste, _spell_haste;
  mutable double _attack_speed, _spell_speed;
  mutable double _dodge, _parry, _block, _crit_block, _armor, _bonus_armor;
  mutable double _mastery, _mastery_value, _crit_avoidance, _miss;
  mutable std::array<double, SCHOOL_MAX + 1> _player_mult;
  mutable std::array<double, SCHOOL_MAX + 1> _player_heal_mult;
  mutable double _damage_versatility, _heal_versatility, _mitigation_versatility;
  mutable double _leech, _run_speed, _avoidance;
  mutable double _rppm_haste_coeff, _rppm_crit_coeff;
  mutable double _corruption, _corruption_resistance;
public:
  bool active; // runtime active-flag
  void invalidate_all();
  void invalidate( cache_e );
  double get_attribute( attribute_e ) const;
  player_stat_cache_t( const player_t* p ) : player( p ), active( false ) { invalidate_all(); }
#if defined(SC_USE_STAT_CACHE)
  // Cache stat functions
  double strength() const;
  double agility() const;
  double stamina() const;
  double intellect() const;
  double spirit() const;
  double spell_power( school_e ) const;
  double attack_power() const;
  double total_melee_attack_power() const;
  double attack_expertise() const;
  double attack_hit() const;
  double attack_crit_chance() const;
  double attack_haste() const;
  double attack_speed() const;
  double spell_hit() const;
  double spell_crit_chance() const;
  double spell_haste() const;
  double spell_speed() const;
  double dodge() const;
  double parry() const;
  double block() const;
  double crit_block() const;
  double crit_avoidance() const;
  double miss() const;
  double armor() const;
  double mastery() const;
  double mastery_value() const;
  double bonus_armor() const;
  double player_multiplier( school_e ) const;
  double player_heal_multiplier( const action_state_t* ) const;
  double damage_versatility() const;
  double heal_versatility() const;
  double mitigation_versatility() const;
  double leech() const;
  double run_speed() const;
  double avoidance() const;
  double corruption() const;
  double corruption_resistance() const;
  double rppm_haste_coeff() const;
  double rppm_crit_coeff() const;
#else
  // Passthrough cache stat functions for inactive cache
  double strength() const  { return _player -> strength();  }
  double agility() const   { return _player -> agility();   }
  double stamina() const   { return _player -> stamina();   }
  double intellect() const { return _player -> intellect(); }
  double spirit() const    { return _player -> spirit();    }
  double spell_power( school_e s ) const  { return _player -> composite_spell_power( s ); }
  double attack_power() const             { return _player -> composite_melee_attack_power();   }
  double total_melee_attack_power() const { return _player -> composite_melee_attack_power_by_type( _player -> default_ap_type() ); }
  double attack_expertise() const { return _player -> composite_melee_expertise(); }
  double attack_hit() const       { return _player -> composite_melee_hit();       }
  double attack_crit_chance() const      { return _player -> composite_melee_crit_chance();      }
  double attack_haste() const     { return _player -> composite_melee_haste();     }
  double attack_speed() const     { return _player -> composite_melee_speed();     }
  double spell_hit() const        { return _player -> composite_spell_hit();       }
  double spell_crit_chance() const       { return _player -> composite_spell_crit_chance();      }
  double spell_haste() const      { return _player -> composite_spell_haste();     }
  double spell_speed() const      { return _player -> composite_spell_speed();     }
  double dodge() const            { return _player -> composite_dodge();      }
  double parry() const            { return _player -> composite_parry();      }
  double block() const            { return _player -> composite_block();      }
  double crit_block() const       { return _player -> composite_crit_block(); }
  double crit_avoidance() const   { return _player -> composite_crit_avoidance();       }
  double miss() const             { return _player -> composite_miss();       }
  double armor() const            { return _player -> composite_armor();           }
  double mastery() const          { return _player -> composite_mastery();   }
  double mastery_value() const    { return _player -> composite_mastery_value();   }
  double damage_versatility() const { return _player -> composite_damage_versatility(); }
  double heal_versatility() const { return _player -> composite_heal_versatility(); }
  double mitigation_versatility() const { return _player -> composite_mitigation_versatility(); }
  double leech() const { return _player -> composite_leech(); }
  double run_speed() const { return _player -> composite_run_speed(); }
  double avoidance() const { return _player -> composite_avoidance(); }
  double corruption() const { return _player -> composite_corruption(); }
  double corruption_resistance() const { return _player -> composite_corruption_resistance(); }
#endif
};
