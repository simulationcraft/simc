// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef AZERITE_DATA_HPP
#define AZERITE_DATA_HPP

#include <vector>
#include <unordered_map>
#include <functional>

#include "util/rapidjson/document.h"

#include "sc_timespan.hpp"

#include "dbc/azerite.hpp"

struct spell_data_t;
struct item_t;
struct player_t;
struct sim_t;
struct action_t;
struct expr_t;
struct special_effect_t;
struct spelleffect_data_t;

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
  /// Actor the power belongs to
  const player_t*             m_player;
  /// Associated spell data
  const spell_data_t*         m_spell;
  /// Ilevels of the items that enable this azerite power
  std::vector<unsigned>       m_ilevels;
  /// Cached scaled (total) value
  mutable std::vector<double> m_value;
  /// Azerite power data entry (client data)
  const azerite_power_entry_t* m_data;

  /// Helper to check if the combat rating penalty needs to be applied to the azerite spell effect
  bool check_combat_rating_penalty( size_t index = 1 ) const;
public:
  /// Time type conversion for azerite_power_t::time_value
  enum time_type
  {
    MS = 0,
    S,
  };

  using azerite_value_fn_t = std::function<double(const azerite_power_t&)>;

  azerite_power_t();
  azerite_power_t( const player_t* p, const azerite_power_entry_t* data, const std::vector<const item_t*>& items );
  azerite_power_t( const player_t* p, const azerite_power_entry_t* data, const std::vector<unsigned>& ilevels );

  /// Implicit conversion to spell_data_t* object for easy use in accessors that accept spell data
  /// pointers
  operator const spell_data_t*() const;

  /// State of the azerite power
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
  /// Return the raw budget values represented by the items used for this power, using the given
  /// spell as context
  std::vector<double> budget( const spell_data_t* spell ) const;
  /// List of item levels associated with this azerite power
  const std::vector<unsigned> ilevels() const;
  /// Number of items worn with this azerite power
  unsigned n_items() const;
  /// Azerite power client data information
  const azerite_power_entry_t* data() const;
};

namespace azerite
{
/**
 * A state class that holds the composite of all azerite-related state an actor has. For now,
 * includes associations between azerite powers and items, and azerite overrides.
 */
class azerite_state_t
{
  /// Player associated with the azerite power
  player_t*                          m_player;
  /// Map of the actor's azerite power ids, and their associated items (items that select the power)
  std::unordered_map<unsigned, std::vector<const item_t*>> m_items;
  /// Azerite power overrides, (power, list of override ilevels)
  std::unordered_map<unsigned, std::vector<unsigned>> m_overrides;

public:
  azerite_state_t( player_t* p );

  /// Initializes the azerite state from the actor's items. Must be run after player_t::init_items()
  ///  is called (successfully), but before player_t::init_spells() is invoked by the actor
  ///  initialization process.
  void initialize();

  /// Get an azerite_power_t object for a given power identifier
  azerite_power_t get_power( unsigned id );
  /// Get an azerite_power_t object for a given power name, potentially tokenized
  azerite_power_t get_power( const std::string& name, bool tokenized = false );

  /// Check the enable status of an azerite power
  bool is_enabled( unsigned id ) const;
  /// Check the enable status of an azerite power
  bool is_enabled( const std::string& name, bool tokenized = false ) const;
  /// Rank of the azrerite power (how many items have the selected power)
  size_t rank( unsigned id ) const;
  /// Rank of the azrerite power (how many items have the selected power)
  size_t rank( const std::string& name, bool tokenized = false ) const;

  /// Parse and sanitize azerite_override option
  bool parse_override( sim_t*, const std::string& /*name*/, const std::string& /*value*/ );
  /// Output overrides as an azerite_override options string
  std::string overrides_str() const;
  /// Clone overrides from another actor
  void copy_overrides( const std::unique_ptr<azerite_state_t>& other );

  /// Create azerite-related expressions
  expr_t* create_expression( const std::vector<std::string>& expr_str ) const;

  /// Enabled azerite spells
  std::vector<unsigned> enabled_spells() const;
};

/// Creates an azerite state object for the actor
std::unique_ptr<azerite_state_t> create_state( player_t* );
/// Initialize azerite powers through the generic special effect subsystem
void initialize_azerite_powers( player_t* actor );
/// Register generic azerite powers to the special effect system
void register_azerite_powers();
/// Register generic azerite powers target data initializers
void register_azerite_target_data_initializers( sim_t* );

/// Compute the <min, avg, max> value of the spell effect given, based on the azerite power
std::tuple<int, int, int> compute_value( const azerite_power_t& power,
    const spelleffect_data_t& effect );

/// Parse azerite item information from the Blizzard Community API JSON output
void parse_blizzard_azerite_information( item_t& item, const rapidjson::Value& data );

namespace special_effects
{
void resounding_protection( special_effect_t& effect );
void elemental_whirl( special_effect_t& effect );
void blood_siphon( special_effect_t& effect );
void lifespeed( special_effect_t& effect );
void on_my_way( special_effect_t& effect );
void champion_of_azeroth( special_effect_t& effect );
void unstable_flames( special_effect_t& );
void thunderous_blast( special_effect_t& );
void filthy_transfusion( special_effect_t& );
void retaliatory_fury( special_effect_t& );
void blood_rite( special_effect_t& );
void glory_in_battle( special_effect_t& );
void sylvanas_resolve( special_effect_t& );
void tidal_surge( special_effect_t& );
void heed_my_call( special_effect_t& );
void azerite_globules( special_effect_t& );
void overwhelming_power( special_effect_t& );
void earthlink( special_effect_t& );
void wandering_soul( special_effect_t& );
void swirling_sands( special_effect_t& );
void tradewinds( special_effect_t& );
void unstable_catalyst( special_effect_t& );
void stand_as_one( special_effect_t& );
void archive_of_the_titans( special_effect_t& );
void laser_matrix( special_effect_t& );
void incite_the_pack( special_effect_t& );
void dagger_in_the_back( special_effect_t& );
void rezans_fury( special_effect_t& );
void secrets_of_the_deep( special_effect_t& );
void combined_might( special_effect_t& );
void relational_normalization_gizmo( special_effect_t& );
void barrage_of_many_bombs( special_effect_t& );
void meticulous_scheming( special_effect_t& );
void synaptic_spark_capacitor( special_effect_t& );
void ricocheting_inflatable_pyrosaw( special_effect_t& );
void gutripper( special_effect_t& );
void battlefield_focus_precision( special_effect_t& );
void endless_hunger( special_effect_t& effect );
void apothecarys_concoctions( special_effect_t& effect );
void shadow_of_elune( special_effect_t& effect );
void treacherous_covenant( special_effect_t& effect );
void seductive_power( special_effect_t& effect );
void bonded_souls( special_effect_t& effect );
void fight_or_flight( special_effect_t& effect );
} // Namespace special_effects ends

} // Namespace azerite ends

#endif /* AZERITE_DATA_HPP */

