// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "dbc.hpp"
#include "specialization_spell.hpp"
#include "active_spells.hpp"
#include "mastery_spells.hpp"
#include "racial_spells.hpp"
#include "sim/sc_expressions.hpp"
#include "azerite.hpp"
#include "spell_query/spell_data_expr.hpp"
#include "sim/sc_sim.hpp"

namespace { // anonymous namespace ==========================================

enum sdata_field_type_t : int
{
  SD_TYPE_NUM = 0,
  SD_TYPE_STR
};

struct sdata_field_t
{
  union field_data_t
  {
    double num;
    const char* str;

    constexpr explicit field_data_t( const char* s ) : str( s ) {}

    template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
    constexpr explicit field_data_t( T v ) : num( static_cast<double>( v ) ) {}
  };

  struct field_data_getter_t
  {
    using func_t = field_data_t(*)( const dbc_t&, const void* );
    sdata_field_type_t type;
    func_t             get;
  };

  util::string_view   name;
  field_data_getter_t data;
};

namespace detail {

template <typename T>
struct mem_fn_traits_impl;

template <typename T, typename U>
struct mem_fn_traits_impl<T U::*> {
  using base_type = T;
  using object_type = U;
};

template <typename T>
using mem_fn_traits_t = mem_fn_traits_impl<std::remove_cv_t<T>>;

template <typename T, typename U, T U::*Pm>
struct mem_fn_t {
  using object_type = U;

  template <typename... Args>
  decltype(auto) operator()( Args&&... args ) const {
    return range::invoke( Pm, std::forward<Args>(args)... );
  }
};

template <typename T>
constexpr sdata_field_type_t field_type_impl() {
  if ( std::is_arithmetic<T>::value )
    return SD_TYPE_NUM;
  if ( std::is_same<T, const char*>::value )
    return SD_TYPE_STR;
  return static_cast<sdata_field_type_t>( -1 );
}

template <typename T>
constexpr sdata_field_type_t field_type() {
  constexpr auto type = field_type_impl<std::decay_t<T>>();
  assert( static_cast<int>( type ) != -1 );
  return type;
}

template <typename DataType, typename Fn>
sdata_field_t::field_data_t get_data_field( const dbc_t&, const void* data ) {
  return sdata_field_t::field_data_t( Fn{}( *static_cast<const DataType*>( data ) ) );
}

template <typename Field>
constexpr sdata_field_t::field_data_getter_t data_field( Field ) {
  using data_type = typename Field::object_type;
  using result_type = decltype( Field{}( std::declval<const data_type&>() ) );
  return { field_type<result_type>(), get_data_field<data_type, Field> };
}

} // namespace detail

template <typename Impl, typename DataType>
struct func_field_t {
  static sdata_field_t::field_data_t get( const dbc_t& dbc, const void* data ) {
    return sdata_field_t::field_data_t( Impl{}( dbc, *static_cast<const DataType*>( data ) ) );
  }

  constexpr operator sdata_field_t::field_data_getter_t() const {
    using result_type = decltype( Impl{}( std::declval<const dbc_t&>(), std::declval<const DataType&>() ) );
    return { detail::field_type<result_type>(), get };
  }
};

// this and most of the supporting detail machinery could be greatly simplified with C++17 auto nttps
#define MEM_FN_T(...) \
  detail::mem_fn_t<detail::mem_fn_traits_t<decltype(__VA_ARGS__)>::base_type, \
                   detail::mem_fn_traits_t<decltype(__VA_ARGS__)>::object_type, \
                   __VA_ARGS__>

#define FIELD(...) detail::data_field( MEM_FN_T(__VA_ARGS__){} )

static constexpr std::array<sdata_field_t, 5> _talent_data_fields { {
  { "name",  FIELD( &talent_data_t::_name ) },
  { "id",    FIELD( &talent_data_t::_id ) },
  { "flags", FIELD( &talent_data_t::_flags ) },
  { "col",   FIELD( &talent_data_t::_col ) },
  { "row",   FIELD( &talent_data_t::_row ) },
} };

static constexpr std::array<sdata_field_t, 26> _effect_data_fields { {
  { "id",              FIELD( &spelleffect_data_t::_id ) },
  { "spell_id",        FIELD( &spelleffect_data_t::_spell_id ) },
  { "index",           FIELD( &spelleffect_data_t::_index ) },
  { "type",            FIELD( &spelleffect_data_t::_type ) },
  { "sub_type" ,       FIELD( &spelleffect_data_t::_subtype ) },
  { "m_coefficient",   FIELD( &spelleffect_data_t::_m_coeff ) },
  { "m_delta",         FIELD( &spelleffect_data_t::_m_delta ) },
  { "m_bonus" ,        FIELD( &spelleffect_data_t::_m_unk ) },
  { "coefficient",     FIELD( &spelleffect_data_t::_sp_coeff ) },
  { "ap_coefficient",  FIELD( &spelleffect_data_t::_ap_coeff ) },
  { "amplitude",       FIELD( &spelleffect_data_t::_amplitude ) },
  { "radius",          FIELD( &spelleffect_data_t::_radius ) },
  { "max_radius",      FIELD( &spelleffect_data_t::_radius_max ) },
  { "base_value",      FIELD( &spelleffect_data_t::_base_value ) },
  { "misc_value",      FIELD( &spelleffect_data_t::_misc_value ) },
  { "misc_value2",     FIELD( &spelleffect_data_t::_misc_value_2 ) },
  { "trigger_spell",   FIELD( &spelleffect_data_t::_trigger_spell_id ) },
  { "m_chain",         FIELD( &spelleffect_data_t::_m_chain ) },
  { "p_combo_points",  FIELD( &spelleffect_data_t::_pp_combo_points ) },
  { "p_level",         FIELD( &spelleffect_data_t::_real_ppl ) },
  { "mechanic",        FIELD( &spelleffect_data_t::_mechanic ) },
  { "chain_target",    FIELD( &spelleffect_data_t::_chain_target ) },
  { "target_1",        FIELD( &spelleffect_data_t::_targeting_1 ) },
  { "target_2",        FIELD( &spelleffect_data_t::_targeting_2 ) },
  { "m_value",         FIELD( &spelleffect_data_t::_m_value ) },
  { "pvp_coefficient", FIELD( &spelleffect_data_t::_pvp_coeff ) },
} };

struct spell_replace_spell_id_t : func_field_t<spell_replace_spell_id_t, spell_data_t> {
  unsigned operator()( const dbc_t& dbc, const spell_data_t& data ) const {
    return dbc.replace_spell_id( data.id() );
  }
};

static constexpr std::array<sdata_field_t, 37> _spell_data_fields { {
  { "name",              FIELD( &spell_data_t::_name ) },
  { "id",                FIELD( &spell_data_t::_id ) },
  { "speed",             FIELD( &spell_data_t::_prj_speed ) },
  { "scaling",           FIELD( &spell_data_t::_scaling_type ) },
  { "max_scaling_level", FIELD( &spell_data_t::_max_scaling_level ) },
  { "level",             FIELD( &spell_data_t::_spell_level ) },
  { "max_level",         FIELD( &spell_data_t::_max_level ) },
  { "min_range",         FIELD( &spell_data_t::_min_range ) },
  { "max_range",         FIELD( &spell_data_t::_max_range ) },
  { "cooldown",          FIELD( &spell_data_t::_cooldown ) },
  { "gcd",               FIELD( &spell_data_t::_gcd ) },
  { "category_cooldown", FIELD( &spell_data_t::_category_cooldown ) },
  { "charges",           FIELD( &spell_data_t::_charges ) },
  { "charge_cooldown",   FIELD( &spell_data_t::_charge_cooldown ) },
  { "category",          FIELD( &spell_data_t::_category ) },
  { "duration",          FIELD( &spell_data_t::_duration ) },
  { "max_stack",         FIELD( &spell_data_t::_max_stack ) },
  { "proc_chance",       FIELD( &spell_data_t::_proc_chance ) },
  { "initial_stack",     FIELD( &spell_data_t::_proc_charges ) },
  { "icd",               FIELD( &spell_data_t::_internal_cooldown ) },
  { "rppm",              FIELD( &spell_data_t::_rppm ) },
  { "equip_class",       FIELD( &spell_data_t::_equipped_class ) },
  { "equip_imask",       FIELD( &spell_data_t::_equipped_invtype_mask ) },
  { "equip_scmask",      FIELD( &spell_data_t::_equipped_subclass_mask ) },
  { "cast_time",         FIELD( &spell_data_t::_cast_time ) },
  { "replace_spellid",   spell_replace_spell_id_t{} },
  { "family",            FIELD( &spell_data_t::_class_flags_family ) },
  { "stance_mask",       FIELD( &spell_data_t::_stance_mask ) },
  { "mechanic",          FIELD( &spell_data_t::_mechanic ) },
  { "power_id",          FIELD( &spell_data_t::_power_id ) }, // Azereite power id
  { "essence_id",        FIELD( &spell_data_t::_essence_id ) }, // Azereite essence id
  { "desc",              FIELD( &spell_data_t::_desc ) },
  { "tooltip",           FIELD( &spell_data_t::_tooltip ) },
  { "rank",              FIELD( &spell_data_t::_rank_str ) },
  { "desc_vars",         FIELD( &spell_data_t::_desc_vars ) },
  { "req_max_level",     FIELD( &spell_data_t::_req_max_level ) },
  { "dmg_class",         FIELD( &spell_data_t::_dmg_class ) },
} };

#undef FIELD
#undef MEM_FN_T

static constexpr std::array<util::string_view, 13> _class_strings { {
  "",
  "warrior",
  "paladin",
  "hunter",
  "rogue",
  "priest",
  "deathknight",
  "shaman",
  "mage",
  "warlock",
  "monk",
  "druid",
  "demonhunter"
} };

static constexpr std::array<util::string_view, 33> _race_strings { {
  "",
  "human",
  "orc",
  "dwarf",
  "night_elf",
  "undead",
  "tauren",
  "gnome",
  "troll",
  "goblin",
  "blood_elf",
  "draenei",
  "dark_iron_dwarf",
  "vulpera",
  "maghar_orc",
  "mechagnome",
  "",
  "",
  "",
  "",
  "",
  "",
  "worgen",
  "",
  "",
  "pandaren",
  "",
  "nightborne",
  "highmountain_tauren",
  "void_elf",
  "lightforged_draenei",
  "zandalari_troll",
  "kul_tiran"
} };

struct expr_data_map_t
{
  util::string_view name;
  expr_data_e type;
};

static constexpr std::array<expr_data_map_t, 9> expr_map { {
  { "spell", DATA_SPELL },
  { "talent", DATA_TALENT },
  { "effect", DATA_EFFECT },
  { "talent_spell", DATA_TALENT_SPELL },
  { "class_spell", DATA_CLASS_SPELL },
  { "race_spell", DATA_RACIAL_SPELL },
  { "mastery", DATA_MASTERY_SPELL },
  { "spec_spell", DATA_SPECIALIZATION_SPELL },
  { "azerite", DATA_AZERITE_SPELL },
} };

expr_data_e parse_data_type( util::string_view name )
{
  for ( auto& entry : expr_map )
  {
    if ( util::str_compare_ci( entry.name, name ) )
      return entry.type;
  }

  return static_cast<expr_data_e>( -1 );
}

unsigned class_str_to_mask( util::string_view str )
{
  int cls_id = -1;

  for ( unsigned int i = 0; i < range::size(_class_strings); ++i )
  {
    if ( _class_strings[ i ].empty() )
      continue;

    if ( ! util::str_compare_ci( _class_strings[ i ], str ) )
      continue;

    cls_id = i;
    break;
  }

  return 1 << ( ( cls_id < 1 ) ? 0 : cls_id - 1 );
}

uint64_t race_str_to_mask( util::string_view str )
{
  int race_id = -1;

  for ( unsigned int i = 0; i < range::size( _race_strings ); ++i )
  {
    if ( _race_strings[ i ].empty() )
      continue;

    if ( ! util::str_compare_ci( _race_strings[ i ], str ) )
      continue;

    race_id = i;
    break;
  }

  return ( uint64_t(1) << ( ( race_id < 1 ) ? 0 : race_id - 1 ) );
}

unsigned school_str_to_mask( util::string_view str )
{
  unsigned mask = 0;

  if ( util::str_in_str_ci( str, "physical" ) || util::str_in_str_ci( str, "strike" ) )
    mask |= 0x1;

  if ( util::str_in_str_ci( str, "holy" ) || util::str_in_str_ci( str, "light" ) )
    mask |= 0x2;

  if ( util::str_in_str_ci( str, "fire" ) || util::str_in_str_ci( str, "flame" ) )
    mask |= 0x4;

  if ( util::str_in_str_ci( str, "nature" ) || util::str_in_str_ci( str, "storm" ) )
    mask |= 0x8;

  if ( util::str_in_str_ci( str, "frost" ) )
    mask |= 0x10;

  if ( util::str_in_str_ci( str, "shadow" ) )
    mask |= 0x20;

  if ( util::str_in_str_ci( str, "arcane" ) || util::str_in_str_ci( str, "spell" ) )
    mask |= 0x40;

  // Special case: Arcane + Holy
  if ( util::str_compare_ci( "divine", str ) )
    mask = 0x42;

  return mask;
}

util::span<const sdata_field_t> data_fields_by_type( expr_data_e type, bool effect_query )
{
  if ( type == DATA_TALENT )
    return { _talent_data_fields };
  else if ( effect_query || type == DATA_EFFECT )
    return { _effect_data_fields };
  return { _spell_data_fields };
}

// Generic spell list based expression, holds intersection, union for list
// For these expression types, you can only use two spell lists as parameters
struct spell_list_expr_t : public spell_data_expr_t
{
  spell_list_expr_t( dbc_t& dbc, util::string_view name, expr_data_e type = DATA_SPELL, bool eq = false ) :
    spell_data_expr_t( dbc, name, type, eq, expression::TOK_SPELL_LIST ) { }

  int evaluate() override
  {
    // Based on the data type, see what list of spell ids we should handle, and populate the
    // result_spell_list accordingly
    switch ( data_type )
    {
      case DATA_SPELL:
      {
        for ( const spell_data_t& spell : spell_data_t::data( dbc.ptr ) )
          result_spell_list.push_back( spell.id() );
        break;
      }
      case DATA_TALENT:
      {
        for ( const talent_data_t& talent : talent_data_t::data( dbc.ptr ) )
          result_spell_list.push_back( talent.id() );
        break;
      }
      case DATA_EFFECT:
      {
        for ( const spelleffect_data_t& effect : spelleffect_data_t::data( dbc.ptr ) )
          result_spell_list.push_back( effect.id() );
        break;
      }
      case DATA_TALENT_SPELL:
      {
        for ( const talent_data_t& talent : talent_data_t::data( dbc.ptr ) )
        {
          if ( ! talent.spell_id() )
            continue;
          result_spell_list.push_back( talent.spell_id() );
        }
        break;
      }
      case DATA_CLASS_SPELL:
      {
        range::for_each( active_class_spell_t::data( dbc.ptr ),
            [this]( const active_class_spell_t& e ) {
              result_spell_list.push_back( e.spell_id );
        } );

        range::for_each( active_pet_spell_t::data( dbc.ptr ),
            [this]( const active_pet_spell_t& e ) {
              result_spell_list.push_back( e.spell_id );
        } );
        break;
      }
      case DATA_RACIAL_SPELL:
      {
        range::for_each( racial_spell_entry_t::data( dbc.ptr ),
            [this]( const racial_spell_entry_t& entry ) {
              result_spell_list.push_back( entry.spell_id );
        } );
        break;
      }
      case DATA_MASTERY_SPELL:
      {
        range::for_each( mastery_spell_entry_t::data( dbc.ptr ),
            [this]( const mastery_spell_entry_t& entry ) {
              result_spell_list.push_back( entry.spell_id );
        } );
        break;
      }
      case DATA_SPECIALIZATION_SPELL:
      {
        range::for_each( specialization_spell_entry_t::data( dbc.ptr ),
            [this]( const specialization_spell_entry_t& e ) {
              result_spell_list.push_back( e.spell_id );
        } );
        break;
      }
      case DATA_AZERITE_SPELL:
      {
        for ( const auto& p : azerite_power_entry_t::data( dbc.ptr ) )
        {
          if ( range::find( result_spell_list, p.spell_id ) == result_spell_list.end() )
          {
            result_spell_list.push_back( p.spell_id );
          }
        }
        break;
      }


      default:
        return expression::TOK_UNKNOWN;
    }

    result_spell_list.resize( range::unique( range::sort( result_spell_list ) ) - result_spell_list.begin() );

    return expression::TOK_SPELL_LIST;
  }

  // Intersect two spell lists
  std::vector<uint32_t> operator&( const spell_data_expr_t& other ) override
  {
    std::vector<uint32_t> res;

    // Only or two spell lists together
    if ( other.result_tok != expression::TOK_SPELL_LIST )
    {
      throw std::invalid_argument(fmt::format("Unsupported right side operand '{}' ({}) for operator &", other.name_str, other.result_tok));
    }
    else
      range::set_intersection( result_spell_list, other.result_spell_list, std::back_inserter( res ) );

    return res;
  }

  // Merge two spell lists, uniqueing entries
  std::vector<uint32_t> operator|( const spell_data_expr_t& other ) override
  {
    std::vector<uint32_t> res;

    // Only or two spell lists together
    if ( other.result_tok != expression::TOK_SPELL_LIST )
    {
      throw std::invalid_argument(fmt::format("Unsupported right side operand '{}' ({}) for operator |", other.name_str, other.result_tok));
    }
    else
      range::set_union( result_spell_list, other.result_spell_list, std::back_inserter( res ) );

    return res;
  }

  // Subtract two spell lists, other from this
  std::vector<uint32_t>operator-( const spell_data_expr_t& other ) override
  {
    std::vector<uint32_t> res;

    // Only or two spell lists together
    if ( other.result_tok != expression::TOK_SPELL_LIST )
    {
      throw std::invalid_argument(fmt::format("Unsupported right side operand '{}' ({}) for operator -", other.name_str, other.result_tok));
    }
    else
      range::set_difference( result_spell_list, other.result_spell_list, std::back_inserter( res ) );

    return res;
  }
};

struct sd_expr_binary_t : public spell_list_expr_t
{
  int                operation;
  std::unique_ptr<spell_data_expr_t> left;
  std::unique_ptr<spell_data_expr_t> right;

  sd_expr_binary_t( dbc_t& dbc, util::string_view n, int o,
                    std::unique_ptr<spell_data_expr_t> l, std::unique_ptr<spell_data_expr_t> r ) :
    spell_list_expr_t( dbc, n ), operation( o ), left( std::move( l ) ), right( std::move( r ) ) { }

  int evaluate() override
  {
    int  left_result =  left -> evaluate();

    right -> evaluate();
    result_tok      = expression::TOK_UNKNOWN;

    if ( left_result != expression::TOK_SPELL_LIST )
    {
      throw std::invalid_argument(fmt::format("Inconsistent input types ('{}' and '{}') for binary operator '{}', left must always be a spell list.\n",
                     left -> name(), right -> name(), name() ));
    }
    else
    {
      result_tok = expression::TOK_SPELL_LIST;
      // Data type follows from left side operand
      data_type   = left -> data_type;

      switch ( operation )
      {
        case expression::TOK_EQ:    result_spell_list = *left == *right; break;
        case expression::TOK_NOTEQ: result_spell_list = *left != *right; break;
        case expression::TOK_OR:    result_spell_list = *left | *right; break;
        case expression::TOK_AND:   result_spell_list = *left & *right; break;
        case expression::TOK_SUB:   result_spell_list = *left - *right; break;
        case expression::TOK_LT:    result_spell_list = *left < *right; break;
        case expression::TOK_LTEQ:  result_spell_list = *left <= *right; break;
        case expression::TOK_GT:    result_spell_list = *left > *right; break;
        case expression::TOK_GTEQ:  result_spell_list = *left >= *right; break;
        case expression::TOK_IN:    result_spell_list = left -> in( *right ); break;
        case expression::TOK_NOTIN: result_spell_list = left -> not_in( *right ); break;
        default:
          throw std::invalid_argument(fmt::format("Unsupported spell query operator {}", operation));
          result_spell_list = std::vector<uint32_t>();
          result_tok = expression::TOK_UNKNOWN;
          break;
      }
    }

    return result_tok;
  }
};

struct spell_data_filter_expr_t : public spell_list_expr_t
{
  sdata_field_t field;

  spell_data_filter_expr_t(dbc_t& dbc, expr_data_e type, util::string_view f_name, bool eq = false ) :
    spell_list_expr_t( dbc, f_name, type, eq ), field{}
  {
    for ( const auto& field_ : data_fields_by_type( type, effect_query ) )
    {
      if ( util::str_compare_ci( f_name, field_.name ) )
      {
        field = field_;
        break;
      }
    }
  }

  bool compare( const void* data, const spell_data_expr_t& other, expression::token_e t ) const
  {
    assert( field.data.get );
    const auto field_data = field.data.get( dbc, data );
    switch ( field.data.type )
    {
      case SD_TYPE_NUM:
      {
        const double value = field_data.num;
        const double ovalue = other.result_num;
        switch ( t )
        {
          case expression::TOK_EQ:    return value == ovalue;
          case expression::TOK_NOTEQ: return value != ovalue;
          case expression::TOK_LT:    return value <  ovalue;
          case expression::TOK_LTEQ:  return value <= ovalue;
          case expression::TOK_GT:    return value >  ovalue;
          case expression::TOK_GTEQ:  return value >= ovalue;
          default: break;
        }
        break;
      }
      case SD_TYPE_STR:
      {
        std::string string_v = util::tokenize_fn( field_data.str ? field_data.str : "" );
        util::string_view ostring_v = other.result_str;
        switch ( t )
        {
          case expression::TOK_EQ:    return util::str_compare_ci( string_v, ostring_v );
          case expression::TOK_NOTEQ: return ! util::str_compare_ci( string_v, ostring_v );
          case expression::TOK_IN:    return util::str_in_str_ci( string_v, ostring_v );
          case expression::TOK_NOTIN: return ! util::str_in_str_ci( string_v, ostring_v );
          default: break;
        }
        break;
      }
      default:
        break;
    }
    return false;
  }

  std::vector<uint32_t> build_list( const spell_data_expr_t& other, expression::token_e t ) const
  {
    std::vector<uint32_t> res;

    for ( const auto& result_spell : result_spell_list )
    {
      // Don't bother comparing if this spell id is already in the result set.
      if ( range::find( res, result_spell ) != res.end() )
        continue;

      if ( effect_query )
      {
        const spell_data_t& spell = *dbc.spell( result_spell );

        // Compare against every spell effect
        for ( const spelleffect_data_t& effect : spell.effects() )
        {
          if ( effect.id() > 0 && dbc.effect( effect.id() ) &&
               compare( &effect, other, t ) )
          {
            res.push_back( result_spell );
            break;
          }
        }
      }
      else
      {
        const void* p_data;
        if ( data_type == DATA_TALENT )
          p_data = dbc.talent( result_spell );
        else if ( data_type == DATA_EFFECT )
          p_data = dbc.effect( result_spell );
        else
          p_data = dbc.spell( result_spell );
        if ( p_data && compare( p_data, other, t ) )
          res.push_back( result_spell );
      }
    }

    return res;
  }

  std::vector<uint32_t> operator==( const spell_data_expr_t& other ) override
  {
    if ( other.result_tok != expression::TOK_NUM && other.result_tok != expression::TOK_STR )
      throw_invalid_op_arg( "==", other );

    return build_list( other, expression::TOK_EQ );
  }

  std::vector<uint32_t> operator!=( const spell_data_expr_t& other ) override
  {
    if ( other.result_tok != expression::TOK_NUM && other.result_tok != expression::TOK_STR )
      throw_invalid_op_arg( "!=", other );

    return build_list( other, expression::TOK_NOTEQ );
  }

  std::vector<uint32_t> operator<( const spell_data_expr_t& other ) override
  {
    if ( other.result_tok != expression::TOK_NUM || field.data.type != SD_TYPE_NUM )
      throw_invalid_op_arg( "<", other );

    return build_list( other, expression::TOK_LT );
  }

  std::vector<uint32_t> operator<=( const spell_data_expr_t& other ) override
  {
    if ( other.result_tok != expression::TOK_NUM || field.data.type != SD_TYPE_NUM )
      throw_invalid_op_arg( "<=", other );

    return build_list( other, expression::TOK_LTEQ );
  }

  std::vector<uint32_t> operator>( const spell_data_expr_t& other ) override
  {
    if ( other.result_tok != expression::TOK_NUM || field.data.type != SD_TYPE_NUM )
      throw_invalid_op_arg( ">", other );

    return build_list( other, expression::TOK_GT );
  }

  std::vector<uint32_t> operator>=( const spell_data_expr_t& other ) override
  {
    if ( other.result_tok != expression::TOK_NUM || field.data.type != SD_TYPE_NUM )
      throw_invalid_op_arg( ">=", other );

    return build_list( other, expression::TOK_GTEQ );
  }

  std::vector<uint32_t> in( const spell_data_expr_t& other ) override
  {
    if ( other.result_tok != expression::TOK_STR || field.data.type != SD_TYPE_STR )
      throw_invalid_op_arg( "~", other );

    return build_list( other, expression::TOK_IN );
  }

  std::vector<uint32_t> not_in( const spell_data_expr_t& other ) override
  {
    if ( other.result_tok != expression::TOK_STR || field.data.type != SD_TYPE_STR )
      throw_invalid_op_arg( "!~", other );

    return build_list( other, expression::TOK_NOTIN );
  }

  /* [[noreturn]] */ void throw_invalid_op_arg( util::string_view op, const spell_data_expr_t& other ) {
    throw std::invalid_argument(
            fmt::format("Unsupported expression operator {} for left='{}' ({}), right='{}' ({})",
              op, name_str, result_tok, other.name_str, other.result_tok));
  }
};

struct spell_class_expr_t : public spell_list_expr_t
{
  spell_class_expr_t( dbc_t& dbc, expr_data_e type ) : spell_list_expr_t( dbc, "class", type ) { }

  std::vector<uint32_t> operator==( const spell_data_expr_t& other ) override
  {
    std::vector<uint32_t> res;
    uint32_t              class_mask;

    if ( other.result_tok == expression::TOK_STR )
      class_mask = class_str_to_mask( other.result_str );
    // Other types will not be allowed, e.g. you cannot do class=list
    else
      return res;

    for ( const auto& result_spell : result_spell_list )
    {
      if ( data_type == DATA_TALENT )
      {
        if ( ! dbc.talent( result_spell ) )
          continue;

        if ( dbc.talent( result_spell ) -> mask_class() & class_mask )
          res.push_back( result_spell );
      }
      else
      {
        const spell_data_t* spell = dbc.spell( result_spell );

        if ( ! spell )
          continue;

        if ( spell -> class_mask() & class_mask )
          res.push_back( result_spell );
      }
    }

    return res;
  }

  std::vector<uint32_t> operator!=( const spell_data_expr_t& other ) override
  {
    std::vector<uint32_t> res;
    uint32_t              class_mask;

    if ( other.result_tok == expression::TOK_STR )
      class_mask = class_str_to_mask( other.result_str );
    // Other types will not be allowed, e.g. you cannot do class=list
    else
      return res;

    for ( const auto& result_spell : result_spell_list )
    {
      if ( data_type == DATA_TALENT )
      {
        if ( ! dbc.talent( result_spell ) )
          continue;

        if ( ( dbc.talent( result_spell ) -> mask_class() & class_mask ) == 0 )
          res.push_back( result_spell );
      }
      else
      {
        const spell_data_t* spell = dbc.spell( result_spell );
        if ( ! spell )
          continue;

        if ( ( spell -> class_mask() & class_mask ) == 0 )
          res.push_back( result_spell );
      }
    }

    return res;
  }
};

struct spell_race_expr_t : public spell_list_expr_t
{
  spell_race_expr_t( dbc_t& dbc, expr_data_e type ) : spell_list_expr_t( dbc, "race", type ) { }

  std::vector<uint32_t> operator==( const spell_data_expr_t& other ) override
  {
    std::vector<uint32_t> res;
    uint64_t              race_mask;

    // Talents are not race specific
    if ( data_type == DATA_TALENT )
      return res;

    if ( other.result_tok == expression::TOK_STR )
      race_mask = race_str_to_mask( other.result_str );
    // Other types will not be allowed, e.g. you cannot do race=list
    else
      return res;

    for ( const auto& result_spell : result_spell_list )
    {
      const spell_data_t* spell = dbc.spell( result_spell );

      if ( ! spell )
        continue;

      if ( spell -> race_mask() & race_mask )
        res.push_back( result_spell );
    }

    return res;
  }

  std::vector<uint32_t> operator!=( const spell_data_expr_t& other ) override
  {
    std::vector<uint32_t> res;
    uint64_t              class_mask;

    // Talents are not race specific
    if ( data_type == DATA_TALENT )
      return res;

    if ( other.result_tok == expression::TOK_STR )
      class_mask = race_str_to_mask( other.result_str );
    // Other types will not be allowed, e.g. you cannot do class=list
    else
      return res;

    for ( const auto& result_spell : result_spell_list )
    {
      const spell_data_t* spell = dbc.spell( result_spell );
      if ( ! spell )
        continue;

      if ( ( spell -> class_mask() & class_mask ) == 0 )
        res.push_back( result_spell );
    }

    return res;
  }
};

struct spell_flag_expr_t : public spell_list_expr_t
{
  spell_flag_expr_t( dbc_t& dbc, expr_data_e type ) : spell_list_expr_t( dbc, "flag", type ) { }

  std::vector<uint32_t> operator==( const spell_data_expr_t& other ) override
  {
    std::vector<uint32_t> res;

    // Only for spells
    if ( data_type == DATA_EFFECT || data_type == DATA_TALENT )
      return res;

    // Numbered attributes only
    if ( other.result_tok != expression::TOK_NUM )
      return res;

    for ( const auto& result_spell : result_spell_list )
    {
      const spell_data_t* spell = dbc.spell( result_spell );

      if ( ! spell )
        continue;

      if ( spell -> class_flag( as<unsigned>( other.result_num ) ) )
        res.push_back( result_spell );
    }

    return res;
  }
};

struct spell_attribute_expr_t : public spell_list_expr_t
{
  spell_attribute_expr_t( dbc_t& dbc, expr_data_e type ) : spell_list_expr_t( dbc, "attribute", type ) { }

  std::vector<uint32_t> operator==( const spell_data_expr_t& other ) override
  {
    std::vector<uint32_t> res;

    // Only for spells
    if ( data_type == DATA_EFFECT || data_type == DATA_TALENT )
      return res;

    // Numbered attributes only
    if ( other.result_tok != expression::TOK_NUM )
      return res;

    uint32_t attridx = ( unsigned ) other.result_num / ( sizeof( unsigned ) * 8 );
    uint32_t flagidx = ( unsigned ) other.result_num % ( sizeof( unsigned ) * 8 );

    assert( attridx < NUM_SPELL_FLAGS && flagidx < 32 );

    for ( const auto& result_spell : result_spell_list )
    {
      const spell_data_t* spell = dbc.spell( result_spell );

      if ( ! spell )
        continue;

      if ( spell -> attribute( attridx ) & ( 1 << flagidx ) )
        res.push_back( result_spell );
    }

    return res;
  }
};

struct spell_school_expr_t : public spell_list_expr_t
{
  spell_school_expr_t(dbc_t& dbc, expr_data_e type ) : spell_list_expr_t( dbc, "school", type ) { }

  std::vector<uint32_t> operator==( const spell_data_expr_t& other ) override
  {
    std::vector<uint32_t> res;
    uint32_t              school_mask;

    if ( other.result_tok == expression::TOK_STR )
      school_mask = school_str_to_mask( other.result_str );
    // Other types will not be allowed, e.g. you cannot do class=list
    else
      return res;

    for ( const auto& result_spell : result_spell_list )
    {
      const spell_data_t* spell = dbc.spell( result_spell );

      if ( ! spell )
        continue;

      if ( ( spell -> school_mask() & school_mask ) == school_mask )
        res.push_back( result_spell );
    }

    return res;
  }

  std::vector<uint32_t> operator!=( const spell_data_expr_t& other ) override
  {
    std::vector<uint32_t> res;
    uint32_t              school_mask;

    if ( other.result_tok == expression::TOK_STR )
      school_mask = school_str_to_mask( other.result_str );
    // Other types will not be allowed, e.g. you cannot do school=list
    else
      return res;

    for ( const auto& result_spell : result_spell_list )
    {
      const spell_data_t* spell = dbc.spell( result_spell );

      if ( ! spell )
        continue;

      if ( ( spell -> school_mask() & school_mask ) == 0 )
        res.push_back( result_spell );
    }

    return res;
  }
};

std::unique_ptr<spell_data_expr_t> build_expression_tree(
  dbc_t& dbc, const std::vector<expression::expr_token_t>& tokens )
{
  std::vector<std::unique_ptr<spell_data_expr_t>> stack;

  for ( auto& t : tokens )
  {
    if ( t.type == expression::TOK_NUM )
    {
      stack.push_back( std::make_unique<spell_data_expr_t>( dbc, t.label, std::stod( t.label ) ) );
    }
    else if ( t.type == expression::TOK_STR )
    {
      auto e = spell_data_expr_t::create_spell_expression( dbc, t.label );

      if ( ! e )
      {
        throw std::invalid_argument(fmt::format("Unable to decode expression function '{}'.", t.label));
      }
      stack.push_back( std::move( e ) );
    }
    else if ( expression::is_binary( t.type ) )
    {
      if ( stack.size() < 2 )
        return nullptr;
      auto right = std::move( stack.back() ); stack.pop_back();
      auto left  = std::move( stack.back() ); stack.pop_back();
      if ( ! left || ! right )
        return nullptr;
      stack.push_back( std::make_unique<sd_expr_binary_t>( dbc, t.label, t.type, std::move(left), std::move(right) ) );
    }
  }

  if ( stack.size() != 1 )
    return nullptr;

  return std::move( stack.back() );
}

} // anonymous namespace ====================================================

std::unique_ptr<spell_data_expr_t> spell_data_expr_t::create_spell_expression( dbc_t& dbc, util::string_view name_str )
{
  std::vector<std::string> splits = util::string_split( name_str, "." );

  if ( splits.empty() || splits.size() > 3 )
    return nullptr;

  expr_data_e data_type = parse_data_type( splits[ 0 ] );

  if ( splits.size() == 1 )
  {
    // No split, access raw list or create a normal expression
    if ( data_type == static_cast<expr_data_e>( -1 ) )
      return std::make_unique<spell_data_expr_t>( dbc, name_str, name_str );
    else
      return std::make_unique<spell_list_expr_t>( dbc, splits[ 0 ], data_type );
  }

  if ( data_type == static_cast<expr_data_e>( -1 ) )
    return nullptr;

  // Effect handling, set flag and remove effect keyword from tokens
  bool effect_query = false;
  if ( util::str_compare_ci( splits[ 1 ], "effect" ) )
  {
    if ( data_type == DATA_EFFECT )
    {
      // "effect.effect"?!?
      return nullptr;
    }
    effect_query = true;
    splits.erase( splits.begin() + 1 );
  }
  else if ( util::str_compare_ci( splits[ 1 ], "class" ) )
    return std::make_unique<spell_class_expr_t>( dbc, data_type );
  else if ( util::str_compare_ci( splits[ 1 ], "race" ) )
    return std::make_unique<spell_race_expr_t>( dbc, data_type );
  else if ( util::str_compare_ci( splits[ 1 ], "attribute" ) )
    return std::make_unique<spell_attribute_expr_t>( dbc, data_type );
  else if ( data_type != DATA_TALENT && util::str_compare_ci( splits[ 1 ], "flag" ) )
    return std::make_unique<spell_flag_expr_t>( dbc, data_type );
  else if ( data_type != DATA_TALENT && util::str_compare_ci( splits[ 1 ], "school" ) )
    return std::make_unique<spell_school_expr_t>( dbc, data_type );

  for ( const auto& field : data_fields_by_type( data_type, effect_query ) )
  {
    if ( util::str_compare_ci( splits[ 1 ], field.name ) )
    {
      return std::make_unique<spell_data_filter_expr_t>( dbc, data_type, field.name, effect_query );
    }
  }

  return nullptr;
}

std::unique_ptr<spell_data_expr_t> spell_data_expr_t::parse( sim_t* sim, const std::string& expr_str )
{
  if ( expr_str.empty() ) return nullptr;

  std::vector<expression::expr_token_t> tokens = expression::parse_tokens( nullptr, expr_str );

  if ( sim -> debug ) expression::print_tokens( tokens, sim );

  expression::convert_to_unary( tokens );

  if ( sim -> debug ) expression::print_tokens( tokens, sim );

  if ( ! expression::convert_to_rpn( tokens ) )
  {
    throw std::invalid_argument(fmt::format("Unable to convert '{}' into RPN.", expr_str ));
  }

  if ( sim -> debug ) expression::print_tokens( tokens, sim );

  try
  {
    auto e = build_expression_tree(*sim->dbc, tokens);
    if (!e)
    {
      throw std::invalid_argument(fmt::format("Unable to build expression tree from '{}'.", expr_str));
    }

    return e;
  }
  catch (const std::exception &)
  {
    std::throw_with_nested(std::invalid_argument(fmt::format("Unable to build expression tree from '{}'.", expr_str)));
  }
}
