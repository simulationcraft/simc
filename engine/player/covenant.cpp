#include "covenant.hpp"

#include "util/util.hpp"
#include "util/io.hpp"
#include "report/decorators.hpp"

#include "sim/sc_expressions.hpp"

#include "player/sc_player.hpp"
#include "player/unique_gear_helper.hpp"
#include "buff/sc_buff.hpp"

#include "sim/sc_option.hpp"

#include "dbc/dbc.hpp"
#include "dbc/spell_data.hpp"
#include "dbc/covenant_data.hpp"

conduit_data_t::conduit_data_t() :
  /* m_player( nullptr ), */ m_conduit( &conduit_rank_entry_t::nil() ), m_spell( spell_data_t::not_found() )
{ }

conduit_data_t::conduit_data_t( const player_t* player, const conduit_rank_entry_t& entry ) :
  /* m_player( player ), */ m_conduit( &entry ), m_spell( dbc::find_spell( player, entry.spell_id ) )
{ }

bool conduit_data_t::ok() const
{
  return m_conduit->conduit_id != 0;
}

unsigned conduit_data_t::rank() const
{
  // Ranks in the client data begin from 0, but better to deal with them as 1+
  return m_conduit->rank + 1;
}

double conduit_data_t::value() const
{
  return m_conduit->value;
}

double conduit_data_t::percent() const
{
  return m_conduit->value / 100.0;
}

timespan_t conduit_data_t::time_value( time_type tt ) const
{
  if ( tt == time_type::MS )
  {
    return timespan_t::from_millis( m_conduit->value );
  }
  else
  {
    return timespan_t::from_seconds( m_conduit->value );
  }
}

namespace util
{
const char* covenant_type_string( covenant_e id, bool full )
{
  switch ( id )
  {
    case covenant_e::KYRIAN:    return full ? "Kyrian" : "kyrian";
    case covenant_e::VENTHYR:   return full ? "Venthyr" : "venthyr";
    case covenant_e::NIGHT_FAE: return full ? "Night Fae" : "night_fae";
    case covenant_e::NECROLORD: return full ? "Necrolord" : "necrolord";
    case covenant_e::DISABLED:  return "disabled";
    default:                    return "invalid";
  }
}

covenant_e parse_covenant_string( util::string_view covenant_str )
{
  if ( util::str_compare_ci( covenant_str, "kyrian" ) )
  {
    return covenant_e::KYRIAN;
  }
  else if ( util::str_compare_ci( covenant_str, "venthyr" ) )
  {
    return covenant_e::VENTHYR;
  }
  else if ( util::str_compare_ci( covenant_str, "night_fae" ) )
  {
    return covenant_e::NIGHT_FAE;
  }
  else if ( util::str_compare_ci( covenant_str, "necrolord" ) )
  {
    return covenant_e::NECROLORD;
  }
  else if ( util::str_compare_ci( covenant_str, "none" ) ||
            util::str_compare_ci( covenant_str, "disabled" ) ||
            util::str_compare_ci( covenant_str, "0" ) )
  {
    return covenant_e::DISABLED;
  }

  return covenant_e::INVALID;
}
} // Namespace util ends

namespace covenant
{
std::unique_ptr<covenant_state_t> create_player_state( const player_t* player )
{
  return std::make_unique<covenant_state_t>( player );
}

covenant_state_t::covenant_state_t( const player_t* player ) :
  m_covenant( covenant_e::INVALID ), m_player( player )
{ }

bool covenant_state_t::parse_covenant( sim_t*             sim,
                                       util::string_view /* name */,
                                       util::string_view value )
{
  covenant_e covenant = util::parse_covenant_string( value );
  unsigned covenant_id = util::to_unsigned_ignore_error( value, static_cast<unsigned>( covenant_e::INVALID ) );

  if ( covenant == covenant_e::INVALID &&
       covenant_id > static_cast<unsigned>( covenant_e::COVENANT_ID_MAX ) )
  {
    sim->error( "{} unknown covenant '{}', must be one of "
                "kyrian, venthyr, night_fae, necrolord, or none, or a number between 0-4.",
        m_player->name(), value );
    return false;
  }

  if ( covenant != covenant_e::INVALID )
  {
    m_covenant = covenant;
  }
  else
  {
    m_covenant = static_cast<covenant_e>( covenant_id );
  }

  return true;
}

// Parse soulbind= option into conduit_state_t and soulbind spell ids
// Format:
// soulbind_token = conduit_id:conduit_rank | soulbind_ability_id
// soulbind = [soulbind_tree_id,]soulbind_token/soulbind_token/soulbind_token/...
// Where:
// soulbind_tree_id = numeric or tokenized soulbind tree identifier; unused by
//                    Simulationcraft, and ignored on parse.
// conduit_id = conduit id number or tokenized version of the conduit name
// conduit_rank = the rank number, starting from 1
// soulbind_ability_id = soulbind ability spell id or tokenized veresion of the soulbind
// ability name
bool covenant_state_t::parse_soulbind( sim_t*             sim,
                                       util::string_view /* name */,
                                       util::string_view value )
{
  m_conduits.clear();
  auto value_str = value;

  // Ignore anything before a comma character in the soulbind option to allow the
  // specification of the soulbind tree (as a numeric parameter, or the name of the
  // follower in tokenized form). Currently simc has no use for this information, but
  // third party tools will find it easier to determine the soulbind tree to display/use
  // from the identifier, instead of reverse-mapping it from the soulbind spell
  // identifiers.
  auto comma_pos = value_str.find(',');
  if ( comma_pos != std::string::npos )
  {
    value_str = value_str.substr( comma_pos + 1 );
  }

  for ( const util::string_view entry : util::string_split<util::string_view>( value_str, "|/" ) )
  {
    // Conduit handling
    if ( entry.find( ':' ) != util::string_view::npos )
    {
      auto _conduit_split = util::string_split<util::string_view>( entry, ":" );
      if ( _conduit_split.size() != 2 )
      {
        sim->error( "{} unknown conduit format {}, must be conduit_id:conduit_rank",
          m_player->name(), entry );
        return false;
      }

      const conduit_entry_t* conduit_entry = nullptr;
      unsigned conduit_id = util::to_unsigned_ignore_error( _conduit_split[ 0 ], 0 );
      unsigned conduit_rank = util::to_unsigned( _conduit_split[ 1 ] );
      if ( conduit_rank == 0 )
      {
        sim->error( "{} invalid conduit rank '{}', must be 1+",
            m_player->name(), _conduit_split[ 1 ] );
        return false;
      }

      // Find by number (conduit ID)
      if ( conduit_id > 0 )
      {
        conduit_entry = &conduit_entry_t::find( conduit_id, m_player->dbc->ptr );
      }
      // Try to find by tokenized string
      else
      {
        conduit_entry = &conduit_entry_t::find( _conduit_split[ 0 ], m_player->dbc->ptr,
            true );
      }

      if ( conduit_entry->id == 0 )
      {
        sim->error( "{} unknown conduit id {}", m_player->name(), _conduit_split[ 0 ] );
        return false;
      }

      const auto& conduit_rank_entry = conduit_rank_entry_t::find( conduit_entry->id,
          conduit_rank - 1u, m_player->dbc->ptr );
      if ( conduit_rank_entry.conduit_id == 0 )
      {
        sim->error( "{} unknown conduit rank {} for {} (id={})",
            m_player->name(), _conduit_split[ 1 ], conduit_entry->name, conduit_entry->id );
        return false;
      }

      // In game ranks are zero-indexed
      m_conduits.emplace_back( conduit_entry->id, conduit_rank - 1u );
    }
    // Soulbind handling
    else
    {
      const soulbind_ability_entry_t* soulbind_entry = nullptr;
      auto soulbind_spell_id = util::to_unsigned_ignore_error( entry, 0 );
      if ( soulbind_spell_id > 0 )
      {
        soulbind_entry = &soulbind_ability_entry_t::find( soulbind_spell_id,
            m_player->dbc->ptr );
      }
      else
      {
        soulbind_entry = &soulbind_ability_entry_t::find( entry, m_player->dbc->ptr, true );
      }

      if ( soulbind_entry->spell_id == 0 )
      {
        sim->error( "{} unknown soulbind spell id {}", m_player->name(), entry );
        return false;
      }

      m_soulbinds.push_back( soulbind_entry->spell_id );
    }
  }

  m_soulbind_str = std::string( value );

  return true;
}

const spell_data_t* covenant_state_t::get_covenant_ability( util::string_view name ) const
{
  if ( !enabled() )
  {
    return spell_data_t::not_found();
  }

  const auto& entry = covenant_ability_entry_t::find( name, m_player->dbc->ptr );
  if ( entry.spell_id == 0 )
  {
    return spell_data_t::nil();
  }

  if ( entry.covenant_id != id() )
  {
    return spell_data_t::not_found();
  }

  if ( entry.class_id && entry.class_id != as<unsigned>( util::class_id( m_player->type ) ) )
  {
    return spell_data_t::not_found();
  }

  const auto spell = dbc::find_spell( m_player, entry.spell_id );
  if ( as<int>( spell->level() ) > m_player->true_level )
  {
    return spell_data_t::not_found();
  }

  return spell;
}

const spell_data_t* covenant_state_t::get_soulbind_ability( util::string_view name,
                                                            bool              tokenized ) const
{
  if ( !enabled() )
  {
    return spell_data_t::not_found();
  }

  std::string search_str = tokenized ? util::tokenize_fn( name ) : std::string( name );

  const auto& soulbind = soulbind_ability_entry_t::find( search_str,
      m_player->dbc->ptr, tokenized );
  if ( soulbind.spell_id == 0 )
  {
    return spell_data_t::nil();
  }

  auto it = range::find( m_soulbinds, soulbind.spell_id );
  if ( it == m_soulbinds.end() )
  {
    return spell_data_t::not_found();
  }

  return dbc::find_spell( m_player, soulbind.spell_id );
}

conduit_data_t covenant_state_t::get_conduit_ability( util::string_view name,
                                                      bool              tokenized ) const
{
  if ( !enabled() )
  {
    return {};
  }

  const auto& conduit_entry = conduit_entry_t::find( name, m_player->dbc->ptr, tokenized );
  if ( conduit_entry.id == 0 )
  {
    return {};
  }

  auto it = range::find_if( m_conduits, [&conduit_entry]( const conduit_state_t& entry ) {
    return std::get<0>( entry ) == conduit_entry.id;
  } );

  if ( it == m_conduits.end() )
  {
    return {};
  }

  const auto& conduit_rank = conduit_rank_entry_t::find(
      std::get<0>( *it ), std::get<1>( *it ), m_player->dbc->ptr );
  if ( conduit_rank.conduit_id == 0 )
  {
    return {};
  }

  return { m_player, conduit_rank };
}

std::string covenant_state_t::soulbind_option_str() const
{
  if ( !m_soulbind_str.empty() )
  {
    return fmt::format( "soulbind={}", m_soulbind_str );
  }

  if ( m_soulbinds.empty() && m_conduits.empty() )
  {
    return {};
  }

  std::vector<std::string> b;

  range::for_each( m_conduits, [&b]( const conduit_state_t& token ) {
    // Note, conduit ranks in client data are zero-indexed
    b.emplace_back( fmt::format( "{}:{}", std::get<0>( token ), std::get<1>( token ) + 1u ) );
  } );

  range::for_each( m_soulbinds, [&b]( unsigned spell_id ) {
    b.emplace_back( fmt::format( "{}", spell_id ) );
  } );

  return fmt::format( "soulbind={}", util::string_join( b, "/" ) );
}

std::string covenant_state_t::covenant_option_str() const
{
  if ( m_covenant == covenant_e::INVALID )
  {
    return {};
  }

  return fmt::format( "covenant={}", m_covenant );
}

std::unique_ptr<expr_t> covenant_state_t::create_expression(
    util::span<const util::string_view> expr_str ) const
{
  if ( expr_str.size() == 1 )
  {
    return nullptr;
  }

  if ( util::str_compare_ci( expr_str[ 0 ], "covenant" ) )
  {
    auto covenant = util::parse_covenant_string( expr_str[ 1 ] );
    if ( covenant == covenant_e::INVALID )
    {
      throw std::invalid_argument(
          fmt::format( "Invalid covenant string '{}'.", expr_str[ 1 ] ) );
    }
    return expr_t::create_constant( "covenant", as<double>( covenant == m_covenant ) );
  }
  else if ( util::str_compare_ci( expr_str[ 0 ], "conduit" ) )
  {
    const auto conduit_ability = get_conduit_ability( expr_str[ 1 ], true );
    if ( !conduit_ability.ok() )
    {
      return expr_t::create_constant( "conduit_nok", 0.0 );
    }

    if ( util::str_compare_ci( expr_str[ 2 ], "enabled" ) )
    {
      return expr_t::create_constant( "conduit_enabled", as<double>( conduit_ability.ok() ) );
    }
    else if ( util::str_compare_ci( expr_str[ 2 ], "rank" ) )
    {
      return expr_t::create_constant( "conduit_rank", as<double>( conduit_ability.rank() ) );
    }
  }
  else if ( util::str_compare_ci( expr_str[ 0 ], "soulbind" ) )
  {
    auto soulbind_spell = get_soulbind_ability( expr_str[ 1 ], true );
    if ( soulbind_spell == spell_data_t::nil() )
    {
      throw std::invalid_argument(
          fmt::format( "Invalid soulbind ability '{}'", expr_str[ 1 ] ) );
    }

    if ( util::str_compare_ci( expr_str[ 2 ], "enabled" ) )
    {
      return expr_t::create_constant( "soulbind_enabled", as<double>( soulbind_spell->ok() ) );
    }
  }

  return nullptr;
}

void covenant_state_t::copy_state( const std::unique_ptr<covenant_state_t>& other )
{
  m_covenant = other->m_covenant;
  m_conduits = other->m_conduits;
  m_soulbinds = other->m_soulbinds;

  m_soulbind_str = other->m_soulbind_str;
  m_covenant_str = other->m_covenant_str;
}

void covenant_state_t::register_options( player_t* player )
{
  player->add_option( opt_func( "soulbind", std::bind( &covenant_state_t::parse_soulbind,
          this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 ) ) );
  player->add_option( opt_func( "covenant", std::bind( &covenant_state_t::parse_covenant,
          this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 ) ) );
}

const spell_data_t* covenant_state_t::get_covenant_ability_spell( bool generic ) const
{
  if ( !enabled() )
    return 0u;

  for ( const auto& e : covenant_ability_entry_t::data( m_player->dbc->ptr ) )
  {
    if ( e.covenant_id != static_cast<unsigned>( m_covenant ) )
      continue;

    if ( e.class_id != util::class_id( m_player->type ) && !e.ability_type )
      continue;

    if ( e.ability_type != static_cast<unsigned>( generic ) )
      continue;

    auto s_data = m_player->find_spell( e.spell_id );

    if ( !s_data->ok() )
      continue;

    return s_data;
  }

  return 0u;
}

report::sc_html_stream& covenant_state_t::generate_report( report::sc_html_stream& root ) const
{
  if ( !enabled() )
    return root;

  root.format( "<tr class=\"left\"><th>{}</th><td><ul class=\"float\">\n", util::covenant_type_string( type(), true ) );

  root.format( "<li>{}</li>\n", report_decorators::decorated_spell_name( m_player->sim, *get_covenant_ability_spell() ) );

  for ( const auto& e : conduit_entry_t::data( m_player->dbc->ptr ) )
  {
    for ( const auto& cd : m_conduits )
    {
      if ( std::get<0>( cd ) == e.id )
      {
        auto cd_spell = m_player->find_spell( e.spell_id );
        root.format( "<li>{} ({})</li>\n",
                     report_decorators::decorated_spell_name( m_player->sim, *cd_spell ),
                     std::get<1>( cd ) + 1 );
      }
    }
  }

  root << "</ul></td></tr>\n";

  if ( m_soulbinds.size() )
  {
    root << "<tr class=\"left\"><th></th><td><ul class=\"float\">\n";

    for ( const auto& sb : m_soulbinds )
    {
      auto sb_spell = m_player->find_spell( sb );
      root.format( "<li>{}</li>\n", report_decorators::decorated_spell_name( m_player->sim, *sb_spell ) );
    }

    root << "</ul></td></tr>\n";
  }

  return root;
}

void initialize_soulbinds( player_t* player )
{
  if ( !player->covenant )
    return;

  for ( auto soulbind_spell : player->covenant->soulbind_spells() )
  {
    auto spell = player->find_spell( soulbind_spell );
    if ( !spell->ok() )
      continue;

    special_effect_t effect { player };
    effect.type = SPECIAL_EFFECT_EQUIP;
    effect.source = SPECIAL_EFFECT_SOURCE_SOULBIND;

    unique_gear::initialize_special_effect( effect, soulbind_spell );

    // Ensure the soulbind has a custom special effect to protect against errant auto-inference
    if ( !effect.is_custom() )
      continue;

    player->special_effects.push_back( new special_effect_t( effect ) );
  }
}

void register_soulbinds()
{
  unique_gear::register_special_effect( 322721, soulbinds::grove_invigoration );
}

struct covenant_ability_cast_cb_t : public dbc_proc_callback_t
{
  const spell_data_t* covenant_ability;
  std::vector<buff_t*> buff_list;

  covenant_ability_cast_cb_t( player_t* p, const special_effect_t& e )
    : dbc_proc_callback_t( p, e ), covenant_ability( p->covenant->get_covenant_ability_spell() ), buff_list()
  {
    p->covenant->cast_callback = this;
  }

  void initialize() override
  {
    listener->sim->print_debug( "Initializing covenant ability cast handler..." );
    listener->callbacks.register_callback( PF_ALL_DAMAGE, PF2_CAST, this );
  }

  void trigger( action_t* a, void* call_data ) override
  {
    if ( &a->data() != covenant_ability )
      return;

    // Current all soulbinds that proc off covenant ability are self-buffs, so simple trigger is sufficient.
    // Functionality will need to be expanded if full actions are needed in the future.
    for ( const auto& b : buff_list )
      b->trigger();
  }
};

void add_covenant_callback_buff( buff_t* buff )
{
  const auto& cov = buff->player->covenant;

  if ( !buff->player->covenant->enabled() )
    return;

  if ( !buff->player->covenant->cast_callback )
  {
    auto eff = new special_effect_t( buff->player );
    eff->name_str = "covenant_cast_callback";
    eff->proc_flags_ = PF_ALL_DAMAGE;
    eff->proc_flags2_ = PF2_CAST;
    buff->player->covenant->cast_callback = new covenant_ability_cast_cb_t( buff->player, *eff );
  }

  auto cb = debug_cast<covenant_ability_cast_cb_t*>( buff->player->covenant->cast_callback );

  if ( range::contains( cb->buff_list, buff ) )
    return;

  cb->buff_list.push_back( buff );
}

namespace soulbinds
{
struct redirected_anima_buff_t : public buff_t
{
  redirected_anima_buff_t( player_t* p ) : buff_t( p, "redirected_anima", p->find_spell( 342814 ) )
  {
    // Mastery is stored in 'points' so use base_value() instead of percent()
    set_default_value( data().effectN( 2 ).base_value() );
    add_invalidate( CACHE_MASTERY );
  }

  bool trigger( int s, double v, double c, timespan_t d ) override
  {
    int anima_stacks = player->buffs.redirected_anima_stacks->check();

    if ( !anima_stacks )
      return false;

    player->buffs.redirected_anima_stacks->expire();

    return buff_t::trigger( anima_stacks, v, c, d );
  }
};

void grove_invigoration( special_effect_t& effect )
{
  auto soulbind = effect.player->find_soulbind_spell( effect.driver()->name_cstr() );
  if ( !soulbind->ok() )
    return;

  effect.custom_buff = effect.player->buffs.redirected_anima_stacks;
  new dbc_proc_callback_t( effect.player, effect );

  if ( !effect.player->buffs.redirected_anima )
    effect.player->buffs.redirected_anima = make_buff<redirected_anima_buff_t>( effect.player );

  add_covenant_callback_buff( effect.player->buffs.redirected_anima );
}
}  // namespace soulbinds
}  // namespace covenant
