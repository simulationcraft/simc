// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef AZERITE_DATA_HPP
#define AZERITE_DATA_HPP

#include <vector>
#include <unordered_map>
#include <functional>

#include "sc_timespan.hpp"

struct spell_data_t;
struct item_t;
struct player_t;
struct sim_t;

namespace azerite
{
class azerite_state_t;
}

/**
 * A class representing a single azerite power and the actors items associated with the power. An
 * item is associated with a given azerite power if the user has selected the azerite power on the
 * item
 *
 * Contains accessors to access the associated azerite power's scaled values based on the context,
 * and its backing client spell data object.
 */
class azerite_power_t
{
  /// Time type conversion for azerite_power_t::time_value
  enum time_type
  {
    MS = 0,
    S,
  };

  /// Actor the power belongs to
  const player_t*       m_player;
  /// Associated spell data
  const spell_data_t*   m_spell;
  /// Ilevels of the items that enable this azerite power
  std::vector<unsigned> m_ilevels;
  /// Cached scaled (total) value
  mutable double        m_value;

public:
  using azerite_value_fn_t = std::function<double(const azerite_power_t&)>;

  azerite_power_t();
  azerite_power_t( const player_t* p, const spell_data_t* spell, const std::vector<const item_t*>& items );
  azerite_power_t( const player_t* p, const spell_data_t* spell, const std::vector<unsigned>& ilevels );

  /// Implicit conversion to spell_data_t* object for easy use in accessors that accept spell data
  /// pointers
  operator const spell_data_t*() const;

  // State of the azerite power
  bool ok() const;
  /// State of the azerite power
  bool enabled() const;
  /// Return the associated spell data for the azerite power
  const spell_data_t* spell() const;
  /// Return the associated spell data for the azerite power
  const spell_data_t& spell_ref() const;
  /// Return the scaled base value of the spell. Caches the computed value.
  double value( size_t index = 1 ) const;
  /// Return the scaled value based off of a functor. Caches the computed value.
  double value( const azerite_value_fn_t& fn ) const;
  /// Return the scaled time value of the spell (base value represents by default milliseconds)
  timespan_t time_value( size_t index = 1, time_type tt = MS ) const;
  /// Return the scaled base value as a percent (value divided by 100)
  double percent( size_t index = 1 ) const;
  /// Return the raw budget values represented by the items for this azerite power.
  std::vector<double> budget() const;
  /// List of items associated with this azerite power
  const std::vector<unsigned> ilevels() const;
};

namespace azerite
{
/**
 * A state class that holds the composite of all azerite-related state an actor has. For now,
 * includes associations between azerite powers and items, as well as the initialization status of
 * each azerite power
 *
 * Initialization status must be tracked across a simulator object, as multiple azerite powers
 * combine into a single additive effect in game, instead of creating separate azerite effects.
 * Initialization state changes on successful invocations of get_power().
 */
class azerite_state_t
{
  /// Player associated with the azerite power
  player_t*                          m_player;
  /// Map of the actor's azerite power ids and their initialization state
  std::unordered_map<unsigned, bool> m_state;
  /// Map of the actor's azerite power ids, and their associated items (items that select the power)
  std::unordered_map<unsigned, std::vector<const item_t*>> m_items;
  /// Azerite power overrides, (power, list of override ilevels)
  std::unordered_map<unsigned, std::vector<unsigned>> m_overrides;

public:
  azerite_state_t( player_t* p );

  /// Initializes the azerite state from the actor's items. Must be run after player_t::init_items()
  //  is called (successfully)
  void initialize();

  /// Get an azerite_power_t object for a given power identifier
  azerite_power_t get_power( unsigned id );
  /// Get an azerite_power_t object for a given power name, potentially tokenized
  azerite_power_t get_power( const std::string& name, bool tokenized = false );
  /// Check initialization status of an azerite power
  bool is_initialized( unsigned id ) const;

  /// Parse and sanitize azerite_override option
  bool parse_override( sim_t*, const std::string& /*name*/, const std::string& /*value*/ );
  /// Output overrides as an azerite_override options string
  std::string overrides_str() const;
  /// Clone overrides from another actor
  void copy_overrides( const std::unique_ptr<azerite_state_t>& other );
};

/// Creates an azerite state object for the actor
std::unique_ptr<azerite_state_t> create_state( player_t* );

} // Namespace azerite ends

#endif /* AZERITE_DATA_HPP */

