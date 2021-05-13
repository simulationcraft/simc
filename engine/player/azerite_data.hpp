// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef AZERITE_DATA_HPP
#define AZERITE_DATA_HPP

#include <vector>
#include <unordered_map>
#include <functional>

#include "util/util.hpp"
#include "rapidjson/document.h"
#include "report/reports.hpp"

#include "util/timespan.hpp"

struct azerite_power_entry_t;
struct azerite_essence_entry_t;
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
  /// Return the raw budget values represented by the items for this azerite power spell effect.
  std::vector<double> budget( size_t index ) const;
  /// Return the raw budget values represented by the items used for this power, using the given
  /// spell effect as context
  std::vector<double> budget( const spelleffect_data_t* effect ) const;
  std::vector<double> budget( const spelleffect_data_t& effect ) const;
  /// List of item levels associated with this azerite power
  std::vector<unsigned> ilevels() const;
  /// Number of items worn with this azerite power
  unsigned n_items() const;
  /// Azerite power client data information
  const azerite_power_entry_t* data() const;
};

enum class essence_type : unsigned
{
  INVALID,      /// Unknown essence
  MAJOR,        /// Major Azerite Essence
  MINOR,        /// Minor Azerite Essence
  PASSIVE       /// "Passive" Azerite Essence (travel nodes between minor/major ones)
};

enum class essence_spell : unsigned
{
  BASE = 0u,
  UPGRADE
};

class azerite_essence_t
{
  /// Actor reference
  const player_t*                          m_player;
  /// Azerite essence entry
  const azerite_essence_entry_t*           m_essence;
  /// Associated major essence power spells
  std::vector<const spell_data_t*>         m_base_major, m_base_minor;
  /// Associated minor essence power spells
  std::vector<const spell_data_t*>         m_upgrade_major, m_upgrade_minor;
  /// Actor's power rank for the essence type
  unsigned                                 m_rank;
  /// Actor's used essence type
  essence_type                             m_type;

public:
  azerite_essence_t();
  /// Invalid constructor, contains actor pointer
  azerite_essence_t( const player_t* player );
  /// Major/Minor constructor
  azerite_essence_t( const player_t* player, essence_type t, unsigned rank,
      const azerite_essence_entry_t* essence );
  /// Passives constructor
  azerite_essence_t( const player_t* player, const spell_data_t* passive );

  /// Is Azerite Essence enabled?
  bool enabled() const;

  const player_t* player() const
  { return m_player; }

  const char* name() const;

  unsigned rank() const
  { return m_rank; }

  bool is_minor() const
  { return m_type == essence_type::MINOR; }

  bool is_major() const
  { return m_type == essence_type::MAJOR; }

  bool is_passive() const
  { return m_type == essence_type::PASSIVE; }

  // Accessor to return neck item
  const item_t* item() const;

  /// Fetch a specific rank minor/major/passive spell pointer of the essence base or upgrade spell
  const spell_data_t* spell( unsigned rank, essence_spell s, essence_type t = essence_type::INVALID ) const;
  /// Fetch a specific rank minor/major/passive spell pointer of the essence base spell
  const spell_data_t* spell( unsigned rank, essence_type t = essence_type::INVALID ) const;
  /// Fetch a specific rank minor/major/passive spell reference of the essence base or upgrade spell
  const spell_data_t& spell_ref( unsigned rank, essence_spell s, essence_type t = essence_type::INVALID ) const;
  /// Fetch a specific rank minor/major/passive spell reference of the essence base spell
  const spell_data_t& spell_ref( unsigned rank, essence_type t = essence_type::INVALID ) const;
  /// Fetch base or upgrade spell pointers of all essence ranks
  std::vector<const spell_data_t*> spells( essence_spell s = essence_spell::BASE, essence_type t = essence_type::INVALID ) const;
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
  /// Map of the actor's azerite power spell ids, and their associated items (items that select the power that uses the spell)
  std::unordered_map<unsigned, std::vector<const item_t*>> m_spell_items;
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
  azerite_power_t get_power( util::string_view name, bool tokenized = false );

  /// Check the enable status of an azerite power
  bool is_enabled( unsigned id ) const;
  /// Check the enable status of an azerite power
  bool is_enabled( util::string_view name, bool tokenized = false ) const;
  /// Rank of the azrerite power (how many items have the selected power)
  size_t rank( unsigned id ) const;
  /// Rank of the azrerite power (how many items have the selected power)
  size_t rank( util::string_view name, bool tokenized = false ) const;

  /// Parse and sanitize azerite_override option
  bool parse_override( sim_t*, util::string_view /*name*/, util::string_view /*value*/ );
  /// Output overrides as an azerite_override options string
  std::string overrides_str() const;
  /// Clone overrides from another actor
  void copy_overrides( const std::unique_ptr<azerite_state_t>& other );

  /// Create azerite-related expressions
  std::unique_ptr<expr_t> create_expression( util::span<const util::string_view> expr_str ) const;

  /// Enabled azerite spells
  std::vector<unsigned> enabled_spells() const;

  // Generate HTML report table output for the azerite data
  report::sc_html_stream& generate_report( report::sc_html_stream& root ) const;
};

class azerite_essence_state_t
{
  class slot_state_t
  {
    essence_type m_type;
    unsigned     m_id;
    unsigned     m_rank;

    public:
      slot_state_t() : m_type( essence_type::INVALID ), m_id( 0u ), m_rank( 0u )
      { }

      slot_state_t( essence_type t, unsigned id, unsigned rank ) :
        m_type( t ), m_id( id ), m_rank( rank )
      { }

      unsigned rank() const
      { return m_rank; }

      bool enabled() const
      { return rank() != 0; }

      unsigned id() const
      { return m_id; }

      essence_type type() const
      { return m_type; }

      std::string str() const
      {
        return util::to_string( m_id ) + ":" +
               util::to_string( m_rank ) + ":" +
               ( type() == essence_type::MAJOR ? '1' : '0' );
      }
  };

  const player_t*           m_player;
  std::vector<slot_state_t> m_state;
  std::string               m_option_str;

public:
  azerite_essence_state_t( const player_t* player );

  azerite_essence_t get_essence( unsigned id ) const;
  azerite_essence_t get_essence( util::string_view name, bool tokenized = false ) const;

  /// Add an azerite essence
  bool add_essence( essence_type type, unsigned id, unsigned rank );

  /// Clone state from another actor
  void copy_state( const std::unique_ptr<azerite_essence_state_t>& other );

  bool parse_azerite_essence( sim_t*, util::string_view /* name */, util::string_view /* value */ );

  /// Create essence-related expressions
  std::unique_ptr<expr_t> create_expression( util::span<const util::string_view> expr_str ) const;

  std::vector<unsigned> enabled_essences() const;

  std::string option_str() const;

  void update_traversal_nodes();

  // Generate HTML report table output for the azerite data
  report::sc_html_stream& generate_report( report::sc_html_stream& root ) const;
};

/// Creates an azerite state object for the actor
std::unique_ptr<azerite_state_t> create_state( player_t* );
/// Creates an azerite essence state object for the actor
std::unique_ptr<azerite_essence_state_t> create_essence_state( player_t* );
/// Initialize azerite powers through the generic special effect subsystem
void initialize_azerite_powers( player_t* actor );
/// Initialize azerite essences  through the generic special effect subsystem
void initialize_azerite_essences( player_t* actor );
/// Register generic azerite and azerite essence powers to the special effect system
void register_azerite_powers();
/// Register generic azerite and azerite essence powers target data initializers
void register_azerite_target_data_initializers( sim_t* );
/// Create major Azerite Essence actions
action_t* create_action( player_t* p, util::string_view name, const std::string& options );

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
void undulating_tides( special_effect_t& effect );
void loyal_to_the_end( special_effect_t& effect );
void arcane_heart( special_effect_t& effect );
void clockwork_heart( special_effect_t& effect );
void personcomputer_interface( special_effect_t& effect );
void heart_of_darkness( special_effect_t& effect );
void impassive_visage( special_effect_t& effect );
void gemhide(special_effect_t& effect);
void crystalline_carapace(special_effect_t& effect);
} // Namespace special_effects ends

namespace azerite_essences
{
void stamina_milestone( special_effect_t& effect );
void the_crucible_of_flame( special_effect_t& effect );
void essence_of_the_focusing_iris( special_effect_t& effect );
void blood_of_the_enemy( special_effect_t& effect );
void condensed_life_force( special_effect_t& effect );
void conflict_and_strife( special_effect_t& effect );
void purification_protocol( special_effect_t& effect );
void ripple_in_space( special_effect_t& effect );
void the_unbound_force( special_effect_t& effect );
void worldvein_resonance( special_effect_t& effect );
void strive_for_perfection( special_effect_t& effect );
void vision_of_perfection( special_effect_t& effect );
void anima_of_life_and_death( special_effect_t& effect );
void nullification_dynamo( special_effect_t& effect );
void aegis_of_the_deep( special_effect_t& effect );
void sphere_of_suppression( special_effect_t& effect );
void breath_of_the_dying( special_effect_t& effect );
void spark_of_inspiration( special_effect_t& effect );
void formless_void( special_effect_t& effect );
void strength_of_the_warden( special_effect_t& effect );
void unwavering_ward( special_effect_t& effect );
void spirit_of_preservation( special_effect_t& effect );
void touch_of_the_everlasting( special_effect_t& effect );
} // Namepsace azerite_essences ends

// Vision of Perfection CDR helper
double vision_of_perfection_cdr( const azerite_essence_t& essence );

} // Namespace azerite ends

#endif /* AZERITE_DATA_HPP */

