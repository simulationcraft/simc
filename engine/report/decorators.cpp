// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "decorators.hpp"

#include "action/action.hpp"
#include "buff/buff.hpp"
#include "dbc/dbc.hpp"
#include "dbc/item_set_bonus.hpp"
#include "item/item.hpp"
#include "player/player.hpp"
#include "player/pet.hpp"
#include "report/color.hpp"
#include "util/util.hpp"
#include "sim/sim.hpp"

#include <vector>

namespace
{
color::rgb item_quality_color( const item_t& item )
{
  switch ( item.parsed.data.quality )
  {
    case 1: return color::COMMON;
    case 2: return color::UNCOMMON;
    case 3: return color::RARE;
    case 4: return color::EPIC;
    case 5: return color::LEGENDARY;
    case 6: return color::HEIRLOOM;
    case 7: return color::HEIRLOOM;
    default: return color::POOR;
  }
}

struct decorator_data_t
{
  virtual ~decorator_data_t() = default;

  virtual bool can_decorate() const = 0;

  virtual void base_url( fmt::memory_buffer& buf ) const = 0;

  // Full blown name, displayed as the link name in the HTML report
  virtual std::string url_name() const = 0;

  // Tokenized name of the spell
  virtual std::string token() const = 0;

  // An optional prefix applied to url_name (included in the link)
  virtual std::string url_name_prefix() const
  {
    return {};
  }

  // shown when data can not be decorated
  virtual std::string undecorated_fallback() const
  {
    return fmt::format( "<a href=\"#\">{}</a>", token() );
  }

  std::vector<std::string> params;
};

// a template to "force" devirtualization
template <typename T>
std::string decorate( const T& data )
{
  static_assert( std::is_base_of<decorator_data_t, T>::value );

  if ( !data.can_decorate() )
  {
    return data.undecorated_fallback();
  }

  const std::string url_name = data.url_name();

  std::string prefix;
  std::string suffix;
  {
    std::string tokenized_name = util::tokenize_fn( url_name );
    std::string obj_token = data.token();

    std::string::size_type affix_offset = obj_token.find( tokenized_name );

    // Add an affix to the name, if the name does not match the
    // spell name. Affix is either the prefix- or suffix portion of the
    // non matching parts of the stats name.
    if ( affix_offset != std::string::npos && tokenized_name != obj_token )
    {
      if ( affix_offset == 0 )
        suffix = obj_token.substr( tokenized_name.size() );
      else if ( affix_offset > 0 )
        prefix = obj_token.substr( 0, affix_offset );
    }
    else if ( affix_offset == std::string::npos )
    {
      suffix = obj_token;
    }
  }

  fmt::memory_buffer buf;

  if ( !prefix.empty() )
  {
    fmt::format_to( std::back_inserter( buf ), "({})&#160;", prefix );
  }

  // Generate base url
  data.base_url( buf );

  // Append url params, if any
  if ( !data.params.empty() )
    fmt::format_to( std::back_inserter( buf ), "?{}", fmt::join( data.params, "&" ) );

  // Close the tag and insert the "name"
  fmt::format_to( std::back_inserter( buf ), "\">{}{}</a>", data.url_name_prefix(), url_name );

  // Add suffix if present
  if ( !suffix.empty() )
  {
    fmt::format_to( std::back_inserter( buf ), "&#160;({})", suffix );
  }

  return to_string( buf );
}

template <typename T>
class spell_decorator_t : public decorator_data_t
{
protected:
  const T* m_obj;

public:
  spell_decorator_t( const T* obj ) : m_obj( obj )
  {
    if ( m_obj->item )
    {
      params.push_back( fmt::format( "ilvl={}", m_obj->item->item_level() ) );
    }
  }

  void base_url( fmt::memory_buffer& buf ) const override
  {
    fmt::format_to( std::back_inserter( buf ), "<a href=\"https://{}.wowhead.com/spell={}",
                    report_decorators::decoration_domain( *this->m_obj->sim ), this->m_obj->data_reporting().id() );
  }

  bool can_decorate() const override
  {
    return this->m_obj->sim->decorated_tooltips && this->m_obj->data_reporting().id() > 0;
  }

  std::string url_name() const override
  {
    return util::encode_html( m_obj->data_reporting().id() ? m_obj->data_reporting().name_cstr()
                                                           : m_obj->name_reporting() );
  }

  std::string token() const override
  {
    return util::encode_html( m_obj->name_reporting() );
  }
};

// Generic spell data decorator, supports player and item driven spell data
class spell_data_decorator_t : public decorator_data_t
{
  const sim_t* m_sim;
  const spell_data_t* m_spell;
  const item_t* m_item;

public:
  spell_data_decorator_t( const sim_t* obj, const spell_data_t* spell )
    : m_sim( obj ), m_spell( spell ), m_item( nullptr )
  {}

  spell_data_decorator_t( const sim_t* obj, const spell_data_t* spell, const item_t* item )
    : m_sim( obj ), m_spell( spell ), m_item( item )
  {
    if ( m_item )
    {
      params.push_back( fmt::format( "ilvl={}", m_item->item_level() ) );
    }
  }

  void base_url( fmt::memory_buffer& buf ) const override
  {
    fmt::format_to( std::back_inserter( buf ), "<a href=\"https://{}.wowhead.com/spell={}",
                    report_decorators::decoration_domain( *m_sim ), m_spell->id() );
  }

  bool can_decorate() const override
  {
    return m_sim->decorated_tooltips && m_spell->id() > 0;
  }
  std::string url_name() const override
  {
    return util::encode_html( m_spell->name_cstr() );
  }
  std::string token() const override
  {
    return util::encode_html( util::tokenize_fn( m_spell->name_cstr() ) );
  }
};

class item_decorator_t : public decorator_data_t
{
  const item_t* m_item;

public:
  item_decorator_t( const item_t* obj ) : m_item( obj )
  {
    if ( m_item->parsed.enchant_id > 0 )
    {
      params.push_back( fmt::format( "ench={}", m_item->parsed.enchant_id ) );
    }

    std::vector<unsigned> gem_ids;
    for ( size_t i = 0, end = m_item->parsed.gem_id.size(); i < end; ++i )
    {
      if ( m_item->parsed.gem_id[ i ] == 0 )
        continue;

      gem_ids.push_back( m_item->parsed.gem_id[ i ] );
      // Include relic bonus ids
      range::copy( m_item->parsed.gem_bonus_id[ i ], std::back_inserter( gem_ids ) );
    }

    if ( !gem_ids.empty() )
    {
      params.push_back( fmt::format( "gems={}", fmt::join( gem_ids, ":" ) ) );
    }

    if ( !m_item->parsed.bonus_id.empty() )
    {
      params.push_back( fmt::format( "bonus={}", fmt::join( m_item->parsed.bonus_id, ":" ) ) );
    }

    if ( !m_item->parsed.azerite_ids.empty() )
    {
      params.push_back( fmt::format( "azerite-powers={}:{}", util::class_id( m_item->player->type ),
                                     fmt::join( m_item->parsed.azerite_ids, ":" ) ) );
    }

    if ( !m_item->parsed.crafted_stat_mod.empty() )
    {
      params.push_back( fmt::format( "crafted-stats={}", fmt::join( m_item->parsed.crafted_stat_mod, ":" ) ) );
    }

    params.push_back( fmt::format( "ilvl={}", m_item->item_level() ) );
  }

  void base_url( fmt::memory_buffer& buf ) const override
  {
    fmt::format_to( std::back_inserter( buf ), "<a style=\"color:{};\" href=\"https://{}.wowhead.com/item={}",
                    item_quality_color( *m_item ), report_decorators::decoration_domain( *m_item->sim ),
                    m_item->parsed.data.id );
  }

  bool can_decorate() const override
  {
    return m_item->sim->decorated_tooltips && m_item->parsed.data.id > 0;
  }
  std::string url_name() const override
  {
    return util::encode_html( m_item->full_name() );
  }
  std::string token() const override
  {
    return util::encode_html( m_item->name() );
  }
};

class buff_decorator_t : public spell_decorator_t<buff_t>
{
public:
  buff_decorator_t( const buff_t* obj ) : spell_decorator_t<buff_t>( obj )
  {}

  // Buffs have pet names included in them
  std::string url_name_prefix() const override
  {
    if ( m_obj->source && m_obj->source->is_pet() )
    {
      return util::encode_html( m_obj->source->name_str ) + ":&#160;";
    }

    return {};
  }
};

class action_decorator_t : public spell_decorator_t<action_t>
{
public:
  action_decorator_t( const action_t* obj ) : spell_decorator_t<action_t>( obj ) {}
};

// Generic spell data decorator, supports player and item driven spell data
class npc_decorator_t : public decorator_data_t
{
  const sim_t* m_sim;
  const std::string m_name;
  const int m_npc_id;

public:
  npc_decorator_t( const sim_t* obj, util::string_view name, int npc_id )
    : m_sim( obj ), m_name( name ), m_npc_id( npc_id )
  {}

  npc_decorator_t( const pet_t& pet ) : npc_decorator_t( pet.sim, pet.name_str, pet.npc_id )
  {}

  void base_url( fmt::memory_buffer& buf ) const override
  {
    fmt::format_to( std::back_inserter( buf ), "<a href=\"https://{}.wowhead.com/npc={}",
                    report_decorators::decoration_domain( *m_sim ), m_npc_id );
  }

  std::string undecorated_fallback() const override
  {
    return token();
  }

  bool can_decorate() const override
  {
    return m_sim->decorated_tooltips && m_npc_id > 0;
  }

  std::string url_name() const override
  {
    return util::encode_html( m_name );
  }

  std::string token() const override
  {
    return util::encode_html( util::tokenize_fn( m_name ) );
  }
};

class item_set_decorator_t : public decorator_data_t
{
  const sim_t* m_sim;
  const item_set_bonus_t* m_bonus;

public:
  item_set_decorator_t( const sim_t* sim, const item_set_bonus_t* bonus ) : m_sim( sim ), m_bonus( bonus )
  {

  }

  bool can_decorate() const override
  {
    return m_bonus->enum_id < set_bonus_type_e::SET_BONUS_MAX;
  }

  void base_url( fmt::memory_buffer& buf ) const override
  {
    fmt::format_to( std::back_inserter( buf ), "<a href=\"https://{}.wowhead.com/item-set={}",
                    report_decorators::decoration_domain( *m_sim ), m_bonus->set_id );
  }

  std::string url_name() const override
  {
    return util::encode_html( m_bonus->set_name );
  }

  std::string token() const override
  {
    auto tok = util::encode_html( m_bonus->set_opt_name );
    util::tolower( tok );

    if ( tok.find( "tier" ) != std::string::npos )
      util::replace_all( tok, "tier", "Tier " );

    return tok;
  }
};

}  // unnamed namespace

namespace report_decorators
{
std::string decoration_domain( const sim_t& sim )
{
#if SC_BETA == 0
  if ( maybe_ptr( sim.dbc->ptr ) )
  {
    return "ptr-2";
  }
  else
  {
    return "www";
  }
#else
  return "beta";
  (void)sim;
#endif
}

std::string decorated_spell_name( const sim_t& sim, const spell_data_t& spell, util::string_view additional_parameters )
{
  if ( sim.decorated_tooltips == 0 )
  {
    return fmt::format( "<a href=\"#\">{}</a>", util::encode_html( spell.name_cstr() ) );
  }

  return fmt::format( "<a href=\"https://{}.wowhead.com/spell={}{}{}\">{}</a>", decoration_domain( sim ), spell.id(),
                      additional_parameters.empty() ? "" : "?", additional_parameters,
                      util::encode_html( spell.name_cstr() ) );
}

std::string decorated_item_name( const item_t* item )
{
  if ( item->sim->decorated_tooltips == 0 || item->parsed.data.id == 0 )
  {
    return fmt::format( "<a style=\"color:{};\" href=\"#\">{}</a>", item_quality_color( *item ),
                        util::encode_html( item->full_name() ) );
  }

  std::vector<std::string> params;
  if ( item->parsed.enchant_id > 0 )
  {
    params.push_back( fmt::format( "enchantment={}", item->parsed.enchant_id ) );
  }

  std::vector<int> gem_ids;
  range::copy_if( item->parsed.gem_id, std::back_inserter( gem_ids ), []( int gem_id ) {
    return gem_id != 0;
  } );
  if ( !gem_ids.empty() )
  {
    params.push_back( fmt::format( "gems={}", fmt::join( gem_ids, "," ) ) );
  }

  if ( !item->parsed.bonus_id.empty() )
  {
    params.push_back( fmt::format( "bonusIDs={}", fmt::join( item->parsed.bonus_id, "," ) ) );
  }

  fmt::memory_buffer buf;
  fmt::format_to( std::back_inserter( buf ), "<a style=\"color:{};\" href=\"https://{}.wowhead.com/item={}",
                  item_quality_color( *item ), decoration_domain( *item->sim ), item->parsed.data.id );
  if ( !params.empty() )
  {
    fmt::format_to( std::back_inserter( buf ), "?{}", fmt::join( params, "&" ) );
  }
  fmt::format_to( std::back_inserter( buf ), "\">{}</a>", util::encode_html( item->full_name() ) );

  return to_string( buf );
}

std::string decorate_html_string( util::string_view value, const color::rgb& color )
{
  return fmt::format( "<span style=\"color:{}\">{}</span>", color, value );
}

std::string decorated_buff( const buff_t& buff )
{
  return decorate( buff_decorator_t( &buff ) );
}

std::string decorated_action( const action_t& a )
{
  return decorate( action_decorator_t( &a ) );
}

std::string decorated_spell_data( const sim_t& sim, const spell_data_t* spell )
{
  return decorate( spell_data_decorator_t( &sim, spell ) );
}

std::string decorated_spell_data_item( const sim_t& sim, const spell_data_t* spell, const item_t& item )
{
  return decorate( spell_data_decorator_t( &sim, spell, &item ) );
}

std::string decorated_item( const item_t& item )
{
  return decorate( item_decorator_t( &item ) );
}

std::string decorated_npc( const pet_t& pet )
{
  return decorate( npc_decorator_t( pet ) );
}

std::string decorated_set( const sim_t& sim, const item_set_bonus_t& bonus )
{
  return decorate( item_set_decorator_t( &sim, &bonus ) );
}

}  // namespace report_decorators
