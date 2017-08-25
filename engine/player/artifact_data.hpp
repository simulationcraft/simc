// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef ARTIFACT_DATA_HPP
#define ARTIFACT_DATA_HPP

#include <unordered_map>
#include <string>

#include "sc_enums.hpp"
#include "report/sc_report.hpp"
#include "dbc/dbc.hpp"

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
// Artificial Stamina trait threshold for diminishing returns
static const unsigned ARTIFICIAL_STAMINA_CUTOFF_TRAIT = 52U;
// Max trait rank
static const uint8_t MAX_TRAIT_RANK = std::numeric_limits<uint8_t>::max();

struct point_data_t
{
  uint8_t purchased;
  uint8_t bonus;
  int16_t overridden; // Indicate overridden status

  point_data_t() : purchased( 0 ), bonus( 0 ), overridden( -1 )
  { }

  point_data_t( uint8_t p, uint8_t b ) : purchased( p ), bonus( b ), overridden( -1 )
  { }

  point_data_t( uint8_t p, uint8_t b, int16_t o ) : purchased( p ), bonus( b ), overridden( o )
  { }
};

using artifact_point_map_t = std::unordered_map<unsigned, point_data_t>;
using artifact_data_ptr_t = std::unique_ptr<player_artifact_data_t>;

class player_artifact_data_t
{
  // Policy on using artifact_override option information on not for various methods. Defaults to
  // allowed on all methods.
  enum override_type { ALLOW_OVERRIDE = 0, DISALLOW_OVERRIDE };

  player_t*             m_player;
  // Artifact point data storage, <power_id, <purchased_rank, relic_rank>>
  artifact_point_map_t  m_points;
  // Indicator whether artifact option has relic identifiers with it
  bool                  m_has_relic_opts;
  // User input for artifact= option
  std::string           m_artifact_str;
  // Cached values for purchased points and points
  unsigned              m_total_points;
  unsigned              m_purchased_points;
  // Primary artifact slot (i.e., main hand, off hand, or invalid [no artifact])
  slot_e                m_slot;
  // Artificial Stamina and Damage auras
  const spell_data_t*   m_artificial_stamina;
  const spell_data_t*   m_artificial_damage;

public:
  player_artifact_data_t( player_t* player );

  // Reset object to empty state
  void     reset();
  // Initialize artifact base state
  void     initialize();
  // Is artifact enabled at all?
  bool     enabled() const;

  sim_t* sim() const;
  player_t* player() const;

  // Has user-input artifact= option relic identifiers (i.e., non-zero relicNid above)
  bool     has_relic_options() const
  { return m_has_relic_opts; }

  slot_e   slot() const
  {
    if ( ! enabled() )
    {
      return SLOT_INVALID;
    }

    return m_slot;
  }

  // Note, first purchased talent does not count towards total points
  unsigned points() const
  {
    if ( ! enabled() )
    {
      return 0;
    }

    return m_purchased_points > 0 ? m_total_points - 1 : m_total_points;
  }

  // Note, first purchased talent does not count towards total points
  unsigned purchased_points() const
  {
    if ( ! enabled() )
    {
      return 0;
    }

    return m_purchased_points > 0 ? m_purchased_points - 1 : m_purchased_points;
  }

  // The damage multiplier for the Artificial Damage trait
  double   artificial_damage_multiplier() const
  {
    if ( ! enabled() )
    {
      return 0.0;
    }

    return m_artificial_damage -> effectN( ARTIFICIAL_DAMAGE_EFFECT_INDEX ).percent()
           * .01
           * ( purchased_points() + BASE_TRAIT_INCREASE );
  }

  // The stamina multiplier for the Artificial Stamina trait
  double  artificial_stamina_multiplier() const
  {
    if ( ! enabled() )
    {
      return 0.0;
    }

    auto full_effect = m_artificial_stamina -> effectN( ARTIFICIAL_STAMINA_EFFECT_INDEX ).percent()
                       * 0.01;

    // After 52nd point, Artificial Stamina is 5 times less effective.
    auto reduced_effect = full_effect * .2;

    auto full_points = std::min( ARTIFICIAL_STAMINA_CUTOFF_TRAIT, purchased_points() );
    auto reduced_points = purchased_points() - full_points;

    return full_effect * ( full_points + BASE_TRAIT_INCREASE ) + reduced_effect * reduced_points;
  }

  // Returns the purchased + bonus rank of a trait, or 0 if trait not purchased and/or bonused
  unsigned power_rank( unsigned power_id, override_type t = ALLOW_OVERRIDE ) const
  {
    if ( ! enabled() )
    {
      return 0;
    }

    auto it = m_points.find( power_id );

    return it != m_points.end()
           ? t == ALLOW_OVERRIDE && it -> second.overridden >= 0
             ? it -> second.overridden
             : ( it -> second.purchased + it -> second.bonus )
           : 0u;
  }

  unsigned purchased_power_rank( unsigned power_id, override_type t = ALLOW_OVERRIDE ) const
  {
    if ( ! enabled() )
    {
      return 0;
    }

    auto it = m_points.find( power_id );

    return it != m_points.end()
           ? t == ALLOW_OVERRIDE && it -> second.overridden >= 0
             ? it -> second.overridden
             : it -> second.purchased
           : 0u;
  }

  // Set the user-input artifact= option string
  void     set_artifact_str( const std::string& value );

  // Add purchased artifact power
  bool     add_power( unsigned power_id, unsigned rank );
  void     add_relic( unsigned item_id, unsigned power_id, unsigned rank );
  // Override an artifact power, used for artifact_override= option
  void     override_power( const std::string& name_str, unsigned rank );

  // (Re)move purchased rank from a power, required for wowhead artifact strings to be correct
  void     move_purchased_rank( unsigned power_id, unsigned rank );

  // Valid power identifier for the player
  bool     valid_power( unsigned power_id ) const;

  // Helper(s) to spit out artifact power information for the actor's artifact
  std::vector<const artifact_power_data_t*> powers() const;
  const artifact_power_data_t* power( unsigned power_id ) const;

  // Encode artifact information to a textual format
  std::string encode() const;

  // Generate (or output existing) artifact= option string
  std::string option_string() const;

  // Generate JSON report output for the artifact data
  js::JsonOutput&& generate_report( js::JsonOutput&& root ) const;
  // Generate HTML report table output for the artifact data
  report::sc_html_stream& generate_report( report::sc_html_stream& root ) const;

  // Creation helpers
  static artifact_data_ptr_t create( player_t* );
  static artifact_data_ptr_t create( const artifact_data_ptr_t& );
};

} // Namespace artifact ends

#endif // ARTIFACT_DATA_HPP
