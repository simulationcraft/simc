// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef COVENANT_HPP
#define COVENANT_HPP

#include "util/format.hpp"
#include "util/string_view.hpp"
#include "sc_timespan.hpp"

#include "report/reports.hpp"

struct player_t;
struct sim_t;
struct expr_t;
struct spell_data_t;
struct conduit_rank_entry_t;

enum class covenant_e : unsigned
{
  DISABLED = 0u,
  KYRIAN,
  VENTHYR,
  NIGHT_FAE,
  NECROLORD,
  COVENANT_ID_MAX = NECROLORD,
  INVALID
};

class conduit_data_t
{
  /* const player_t*             m_player; */
  const conduit_rank_entry_t* m_conduit;
  const spell_data_t*         m_spell;

public:
  conduit_data_t();
  conduit_data_t( const player_t* player, const conduit_rank_entry_t& conduit );

  bool ok() const;
  unsigned rank() const;

  double value() const;
  double percent() const;
  timespan_t time_value() const;

  operator const spell_data_t*() const
  { return m_spell; }

  const spell_data_t* operator->() const
  { return m_spell; }
};

namespace util
{
const char* covenant_type_string( covenant_e id );
covenant_e parse_covenant_string( util::string_view covenant_str );
}

namespace covenant
{
class covenant_state_t
{
  /// A conduit token in the form of <conduit id, rank>
  using conduit_state_t = std::tuple<unsigned, unsigned>;

  /// Current actor covenant
  covenant_e                   m_covenant;

  /// Actor associated with this covenant state information
  const player_t*              m_player;

  /// A list of actor conduits and their ranks
  std::vector<conduit_state_t> m_conduits;

  /// A list of currently enabled soulbind abilities for the actor
  std::vector<unsigned>        m_soulbinds;

  /// Soulbinds option string (user input)
  std::string                  m_soulbind_str;

  /// Covenant option string (user input)
  std::string                  m_covenant_str;

public:
  covenant_state_t() = delete;
  covenant_state_t( const player_t* player );

  /// Covenant-related functionality enabled
  bool enabled() const
  { return m_covenant != covenant_e::DISABLED && m_covenant != covenant_e::INVALID; }

  /// Current covenant type
  covenant_e type() const
  { return m_covenant; }

  /// Current covenant id
  unsigned id() const
  { return static_cast<unsigned>( m_covenant ); }

  /// Parse player-scope "covenant" option
  bool parse_covenant( sim_t* sim, util::string_view name, const std::string& value );

  /// Parse player-scope "soulbind" option
  bool parse_soulbind( sim_t* sim, util::string_view name, const std::string& value );

  /// Retrieve covenant ability spell data. Returns spell_data_t::not_found if covenant
  /// ability is not enabled on the actor. Returns spell_data_t::nil if covenant ability
  /// cannot be found from client data.
  const spell_data_t* get_covenant_ability( util::string_view name ) const;

  /// Retrieve soulbind ability spell data. Returns spell_data_t::not_found if soulbind
  /// ability is not enabled on the actor. Returns spell_data_t::nil if soulbind ability
  /// cannot be found from client data.
  const spell_data_t* get_soulbind_ability( util::string_view name, bool tokenized = false ) const;

  /// Retrieve conduit ability data. Returns an empty conduit object (with "not found"
  /// spell data), if the conduit is not found on the actor.
  conduit_data_t get_conduit_ability( util::string_view name, bool tokenized = false ) const;

  /// Create essence-related expressions
  std::unique_ptr<expr_t> create_expression( const std::vector<std::string>& expr_str ) const;

  /// HTML report covenant-related information generator
  report::sc_html_stream& generate_report( report::sc_html_stream& root ) const;

  /// Option string for the soulbinds on this actor
  std::string soulbind_option_str() const;

  /// Option string for covenant of this actor
  std::string covenant_option_str() const;

  /// Clone state from another actor
  void copy_state( const std::unique_ptr<covenant_state_t>& other );

  /// Register player-scope options
  void register_options( player_t* player );
};

std::unique_ptr<covenant_state_t> create_player_state( const player_t* player );

} // Namespace covenant ends

inline void format_to( covenant_e covenant, fmt::format_context::iterator out )
{
  fmt::format_to( out, "{}", util::covenant_type_string( covenant ) );
}

#endif /* COVENANT_HPP */
