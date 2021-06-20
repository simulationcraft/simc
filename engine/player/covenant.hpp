// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef COVENANT_HPP
#define COVENANT_HPP

#include "action/dbc_proc_callback.hpp"
#include "report/reports.hpp"
#include "util/format.hpp"
#include "util/span.hpp"
#include "util/string_view.hpp"
#include "util/timespan.hpp"

#include "rapidjson/document.h"

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
  /// Time type conversion for azerite_power_t::time_value
  enum time_type
  {
    MS = 0,
    S,
  };

  conduit_data_t();
  conduit_data_t( const player_t* player, const conduit_rank_entry_t& conduit );

  bool ok() const;
  unsigned rank() const;

  double value() const;
  double percent() const;
  timespan_t time_value( time_type tt = MS ) const;

  operator const spell_data_t*() const
  { return m_spell; }

  const spell_data_t* operator->() const
  { return m_spell; }
};

namespace util
{
const char* covenant_type_string( covenant_e id, bool full = false );
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

  /// A list of currently enabled renown abilities for the actor
  std::vector<unsigned>        m_renown;

  /// Soulbind character string
  std::string                  m_soulbind_id;

  /// Soulbinds option string (user input)
  std::vector<std::string>     m_soulbind_str;

  /// Covenant option string (user input)
  std::string                  m_covenant_str;

  /// Covenant renown level (user input)
  unsigned                     m_renown_level;

public:
  covenant_state_t() = delete;
  covenant_state_t( const player_t* player );

  /// Covenant-related functionality enabled
  bool enabled() const
  { return m_covenant != covenant_e::DISABLED && m_covenant != covenant_e::INVALID; }

  /// Current covenant type
  covenant_e type() const
  { return m_covenant; }

  void set_type( covenant_e covenant )
  { m_covenant = covenant; }

  void add_conduit( unsigned conduit_id, unsigned rank )
  { m_conduits.emplace_back( conduit_id, rank ); }

  void add_soulbind( unsigned spell_id )
  { m_soulbinds.push_back( spell_id ); }

  void set_soulbind_id( const std::string& str )
  { m_soulbind_id = str; }

  /// Current covenant id
  unsigned id() const
  { return static_cast<unsigned>( m_covenant ); }

  unsigned renown() const
  { return m_renown_level; }

  /// Parse player-scope "covenant" option
  bool parse_covenant( sim_t* sim, util::string_view name, util::string_view value );

  /// Parse player-scope "soulbind" option
  bool parse_soulbind( sim_t* sim, util::string_view name, util::string_view value );
  bool parse_soulbind_clear( sim_t* sim, util::string_view name, util::string_view value );

  /// Parse player-scope "renown" option
  bool parse_renown( sim_t* sim, util::string_view name, util::string_view value );

  /// Sets renown level and looks up renown abilities based on the level.
  void set_renown_level( unsigned renown_level );

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
  std::unique_ptr<expr_t> create_expression( util::span<const util::string_view> expr_str ) const;

  /// HTML report covenant-related information generator
  report::sc_html_stream& generate_report( report::sc_html_stream& root ) const;

  /// Conduit checking helper function for default profile generation
  void check_conduits( util::string_view tier_name, unsigned max_conduit_rank ) const;

  /// Option string for the soulbinds on this actor
  std::string soulbind_option_str() const;

  /// Option string for covenant of this actor
  std::string covenant_option_str() const;

  /// Clone state from another actor
  void copy_state( const std::unique_ptr<covenant_state_t>& other );

  /// Register player-scope options
  void register_options( player_t* player );

  /// Returns the spell_id of the covenant class ability or the generic ability as determined via the
  /// __covenant_ability_data dbc table. Will need to be converted into a vector if multiple class abilities are added
  /// in later.
  unsigned get_covenant_ability_spell_id( bool generic = false ) const;

  /// List of soulbind spells
  const std::vector<unsigned>& soulbind_spells() const { return m_soulbinds; }

  /// List of renown spells
  const std::vector<unsigned>& renown_spells() const { return m_renown; }

  /// Callback for handling soulbinds that proc when covenant class ability is used
  dbc_proc_callback_t* cast_callback = nullptr;
};

std::unique_ptr<covenant_state_t> create_player_state( const player_t* player );

struct covenant_cb_base_t
{
  bool trigger_on_class;  // proc off class ability
  bool trigger_on_base;   // proc off base ability

  covenant_cb_base_t( bool on_class = true, bool on_base = false );
  virtual ~covenant_cb_base_t() {}
  virtual void trigger( action_t*, action_state_t* ) = 0;
};

struct covenant_ability_cast_cb_t : public dbc_proc_callback_t
{
  std::vector<unsigned> class_abilities;
  unsigned base_ability;
  auto_dispose< std::vector<covenant_cb_base_t*> > cb_list;

  covenant_ability_cast_cb_t( player_t* p, const special_effect_t& e );

  void initialize() override;
  void trigger( action_t* a, action_state_t* s ) override;
};

covenant_ability_cast_cb_t* get_covenant_callback( player_t* p );

action_t* create_action( player_t* player, util::string_view name, const std::string& options );

bool parse_blizzard_covenant_information( player_t* player, const rapidjson::Value& covenant_data );

} // Namespace covenant ends

namespace report_decorators
{
std::string decorated_conduit_name( const sim_t& sim, const conduit_data_t& conduit );
}

inline void format_to( covenant_e covenant, fmt::format_context::iterator out )
{
  fmt::format_to( out, "{}", util::covenant_type_string( covenant ) );
}

#endif /* COVENANT_HPP */
