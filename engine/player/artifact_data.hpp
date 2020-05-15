// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef ARTIFACT_DATA_HPP
#define ARTIFACT_DATA_HPP

#include <unordered_map>
#include <string>
#include <limits>
#include <memory>
#include <vector>

#include "sc_enums.hpp"
#include "report/reports.hpp"

struct artifact_power_data_t;
struct spell_data_t;

namespace js
{
struct JsonOutput;
} // Namespace js ends

namespace artifact
{
class player_artifact_data_t;

// Tuple index to the trait ranks purchased in artifact_power_data_t
static const size_t POINT_PURCHASED = 0U;
// Tuple index to the trait ranks from relics in artifact_power_data_t
static const size_t POINT_BONUS = 1U;
// Spell effect index for the value of the Artificial Damage trait
static const size_t ARTIFICIAL_DAMAGE_EFFECT_INDEX = 2U;
// Spell effect index for the value of the Artificial Stamina trait
static const size_t ARTIFICIAL_STAMINA_EFFECT_INDEX = 2U;
// Base point increase for Artificial Damage & Stamina traits
static const unsigned BASE_TRAIT_INCREASE = 6U;
// Artificial Damage trait threshold for diminishing returns
static const unsigned ARTIFICIAL_DAMAGE_CUTOFF_TRAIT = 75U;
// Artificial Stamina trait threshold for diminishing returns
static const unsigned ARTIFICIAL_STAMINA_CUTOFF_TRAIT = 52U;
// Max trait rank
static const uint8_t MAX_TRAIT_RANK = std::numeric_limits<uint8_t>::max();
// Ilevel crucible power id
static const unsigned CRUCIBLE_ILEVEL_POWER_ID = 1739U;
// Default override value
static const int16_t NO_OVERRIDE = -1;

struct point_data_t
{
  uint8_t purchased;
  uint8_t bonus;
  uint8_t crucible;
  int16_t overridden; // Indicate overridden status

  point_data_t() : purchased( 0 ), bonus( 0 ), crucible( 0 ), overridden( NO_OVERRIDE )
  { }

  point_data_t( uint8_t p, uint8_t b ) : purchased( p ), bonus( b ), crucible( 0 ),
    overridden( NO_OVERRIDE )
  { }

  point_data_t( uint8_t p, uint8_t b, int16_t o ) : purchased( p ), bonus( b ),
    crucible( 0 ), overridden( o )
  { }
};

struct relic_data_t
{
  unsigned power_id;
  uint8_t  rank;
};

using artifact_point_map_t = std::unordered_map<unsigned, point_data_t>;
using artifact_data_ptr_t = std::unique_ptr<player_artifact_data_t>;

class player_artifact_data_t
{
  // Policy on using artifact_override option information on not for various methods. Defaults to
  // allowed on all methods.
  enum override_type { ALLOW_OVERRIDE = 0, DISALLOW_OVERRIDE };
  // Policy how to manipulate cruciple powers. Crucible option supports two different input formats,
  // one of which directly sets the crucible rank, and another that individually adds trait ranks
  // based on relic information.
  enum power_op { OP_SET = 0, OP_ADD };

  player_t*             m_player;
  // Artifact point data storage, <power_id, <purchased_rank, relic_rank>>
  artifact_point_map_t  m_points;
  // Indicator whether artifact option has relic identifiers with it
  bool                  m_has_relic_opts;
  // User input for artifact= option
  std::string           m_artifact_str;
  // User input for artifact_crucible= option
  std::string           m_crucible_str;
  // Cached values for purchased points and points
  unsigned              m_total_points;
  unsigned              m_purchased_points;
  unsigned              m_crucible_points;
  // Primary artifact slot (i.e., main hand, off hand, or invalid [no artifact])
  slot_e                m_slot;
  // Artificial Stamina and Damage auras
  const spell_data_t*   m_artificial_stamina;
  const spell_data_t*   m_artificial_damage;
  // Relic information
  std::vector<relic_data_t> m_relic_data;

  // Standard format parsing functions for artifact data from 'artifact', and 'crucible' user
  // options
  bool parse();
  bool parse_crucible1();
  bool parse_crucible2();
  bool parse_crucible();

  void reset_artifact();
  void reset_crucible();
public:
  player_artifact_data_t( player_t* player );

  // Initialize artifact state, including parsing of the 'artifact' and 'crucible' options
  bool     initialize();
  // Is artifact enabled at all?
  bool     enabled() const;

  sim_t* sim() const;
  player_t* player() const;

  // Return point data or an empty point information if not found
  const point_data_t& point_data( unsigned power_id ) const;

  // Crucible ilevel increase. Needs to be done separately as item initialization occurs early on,
  // and the ilevel increase must be present at that point to correctly compute stats.
  int ilevel_increase() const;

  // Has user-input artifact= option relic identifiers (i.e., non-zero relicNid above)
  bool     has_relic_options() const
  { return m_has_relic_opts; }

  // Primary artifact slot, or SLOT_INVALID if artifacts disabled / no artifact equipped
  slot_e   slot() const;

  // Note, first purchased talent does not count towards total points
  unsigned points() const;

  // Note, first purchased talent does not count towards total points
  unsigned purchased_points() const;

  // Note, first purchased talent does not count towards total points
  unsigned crucible_points() const;

  // The damage multiplier for the Artificial Damage trait
  double   artificial_damage_multiplier() const;

  // The stamina multiplier for the Artificial Stamina trait
  double  artificial_stamina_multiplier() const;

  // Returns the purchased + bonus rank of a trait, or 0 if trait not purchased and/or bonused
  unsigned power_rank(unsigned power_id, override_type t = ALLOW_OVERRIDE) const;

  unsigned bonus_rank(unsigned power_id, override_type t = ALLOW_OVERRIDE) const;

  unsigned crucible_rank(unsigned power_id, override_type t = ALLOW_OVERRIDE) const;

  unsigned purchased_power_rank(unsigned power_id, override_type t = ALLOW_OVERRIDE) const;

  // Set the user-input artifact= option string
  void     set_artifact_str( const std::string& value );
  // Set the user-input artifact_crucible= option string
  void     set_crucible_str( const std::string&  value );
  // Set the primary artifact slot (called by item init)
  void     set_artifact_slot( slot_e slot );

  // Add purchased artifact power
  bool     add_power( unsigned power_id, unsigned rank );
  void     add_relic( size_t index, unsigned item_id, unsigned power_id, unsigned rank );
  void     remove_relic( size_t index );
  bool     add_crucible_power( unsigned power_id, unsigned rank, power_op = OP_SET );
  // Override an artifact power, used for artifact_override= option
  void     override_power( const std::string& name_str, unsigned rank );

  // (Re)move purchased rank from a power, required for wowhead artifact strings to be correct
  void     move_purchased_rank( unsigned index, unsigned power_id, unsigned rank );

  // Valid power identifier for the player
  bool     valid_power( unsigned power_id ) const;

  // Valid Netherlight Crucible power for the player
  bool     valid_crucible_power( unsigned power_id ) const;

  // Helper(s) to spit out artifact power information for the actor's artifact
  std::vector<const artifact_power_data_t*> powers() const;
  const artifact_power_data_t* power( unsigned power_id ) const;

  // Encode artifact information to a textual format
  std::string encode() const;
  // Encode crucible artifact information to textual format
  std::string encode_crucible() const;

  // Generate (or output existing) artifact= option string
  std::string artifact_option_string() const;

  // Generate (or output existing) artifact_crucible= option string
  std::string crucible_option_string() const;

  // Generate JSON report output for the artifact data
  js::JsonOutput&& generate_report( js::JsonOutput&& root ) const;
  // Generate HTML report table output for the artifact data
  report::sc_html_stream& generate_report( report::sc_html_stream& root ) const;

  // Creation helpers
  static artifact_data_ptr_t create( player_t* );
  static artifact_data_ptr_t create( player_t*, const artifact_data_ptr_t& );
};

} // Namespace artifact ends

#endif // ARTIFACT_DATA_HPP
