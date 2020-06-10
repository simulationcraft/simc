// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "dbc.hpp"
#include "specialization_spell.hpp"
#include "active_spells.hpp"
#include "sim/sc_expressions.hpp"
#include "azerite.hpp"
#include "spell_query/spell_data_expr.hpp"
#include "sim/sc_sim.hpp"

namespace { // anonymous namespace ==========================================

enum sdata_field_type_t
{
  SD_TYPE_INT = 0,
  SD_TYPE_UNSIGNED,
  SD_TYPE_UINT64,
  SD_TYPE_DOUBLE,
  SD_TYPE_STR
};

struct sdata_field_t
{
  sdata_field_type_t type;
  util::string_view  name;
  size_t             offset;
};

#define O_TD(f) offsetof( talent_data_t, f )
const sdata_field_t _talent_data_fields[] =
{
  { SD_TYPE_STR,      "name",  O_TD( _name )       },
  { SD_TYPE_UNSIGNED, "id",    O_TD( _id )         },
  { SD_TYPE_UNSIGNED, "flags", O_TD( _flags )      },
  { SD_TYPE_UNSIGNED, "col",   O_TD( _col )        },
  { SD_TYPE_UNSIGNED, "row",   O_TD( _row )        },
};

#define O_SED(f) offsetof( spelleffect_data_t, f )
const sdata_field_t _effect_data_fields[] =
{
  { SD_TYPE_UNSIGNED, "id",             O_SED( _id )              },
  { SD_TYPE_UNSIGNED, "hotfix",         O_SED( _hotfix )          },
  { SD_TYPE_UNSIGNED, "spell_id",       O_SED( _spell_id )        },
  { SD_TYPE_UNSIGNED, "index",          O_SED( _index )           },
  { SD_TYPE_INT,      "type",           O_SED( _type )            },
  { SD_TYPE_INT,      "sub_type" ,      O_SED( _subtype )         },
  { SD_TYPE_DOUBLE,   "m_coefficient",  O_SED( _m_coeff )         },
  { SD_TYPE_DOUBLE,   "m_delta",        O_SED( _m_delta )         },
  { SD_TYPE_DOUBLE,   "m_bonus" ,       O_SED( _m_unk )           },
  { SD_TYPE_DOUBLE,   "coefficient",    O_SED( _sp_coeff )        },
  { SD_TYPE_DOUBLE,   "ap_coefficient", O_SED( _ap_coeff )        },
  { SD_TYPE_DOUBLE,   "amplitude",      O_SED( _amplitude )       },
  { SD_TYPE_DOUBLE,   "radius",         O_SED( _radius )          },
  { SD_TYPE_DOUBLE,   "max_radius",     O_SED( _radius_max )      },
  { SD_TYPE_INT,      "base_value",     O_SED( _base_value )      },
  { SD_TYPE_INT,      "misc_value",     O_SED( _misc_value )      },
  { SD_TYPE_INT,      "misc_value2",    O_SED( _misc_value_2 )    },
  { SD_TYPE_INT,      "trigger_spell",  O_SED( _trigger_spell )   },
  { SD_TYPE_DOUBLE,   "m_chain",        O_SED( _m_chain )         },
  { SD_TYPE_DOUBLE,   "p_combo_points", O_SED( _pp_combo_points ) },
  { SD_TYPE_DOUBLE,   "p_level",        O_SED( _real_ppl )        },
  { SD_TYPE_UNSIGNED, "mechanic",       O_SED( _mechanic )        },
  { SD_TYPE_UNSIGNED, "chain_target",   O_SED( _chain_target )    },
  { SD_TYPE_UNSIGNED, "target_1",       O_SED( _targeting_1 )     },
  { SD_TYPE_UNSIGNED, "target_2",       O_SED( _targeting_2 )     },
  { SD_TYPE_DOUBLE,   "m_value",        O_SED( _m_value )         },
  { SD_TYPE_DOUBLE,   "pvp_coefficient",O_SED( _pvp_coeff )       }
};

#define O_SD(f) offsetof( spell_data_t, f )
const sdata_field_t _spell_data_fields[] =
{
  { SD_TYPE_STR,      "name",              O_SD( _name )                   },
  { SD_TYPE_UNSIGNED, "id",                O_SD( _id )                     },
  { SD_TYPE_UINT64,   "hotfix",            O_SD( _hotfix )                 },
  { SD_TYPE_DOUBLE,   "speed",             O_SD( _prj_speed )              },
  { SD_TYPE_INT,      "scaling",           O_SD( _scaling_type )           },
  { SD_TYPE_UNSIGNED, "max_scaling_level", O_SD( _max_scaling_level )      },
  { SD_TYPE_UNSIGNED, "level",             O_SD( _spell_level )            },
  { SD_TYPE_UNSIGNED, "max_level",         O_SD( _max_level )              },
  { SD_TYPE_DOUBLE,   "min_range",         O_SD( _min_range )              },
  { SD_TYPE_DOUBLE,   "max_range",         O_SD( _max_range )              },
  { SD_TYPE_UNSIGNED, "cooldown",          O_SD( _cooldown )               },
  { SD_TYPE_UNSIGNED, "gcd",               O_SD( _gcd )                    },
  { SD_TYPE_UNSIGNED, "category_cooldown", O_SD( _category_cooldown )      },
  { SD_TYPE_UNSIGNED, "charges",           O_SD( _charges )                },
  { SD_TYPE_UNSIGNED, "charge_cooldown",   O_SD( _charge_cooldown )        },
  { SD_TYPE_UNSIGNED, "category",          O_SD( _category )               },
  { SD_TYPE_DOUBLE,   "duration",          O_SD( _duration )               },
  { SD_TYPE_UNSIGNED, "max_stack",         O_SD( _max_stack )              },
  { SD_TYPE_UNSIGNED, "proc_chance",       O_SD( _proc_chance )            },
  { SD_TYPE_INT,      "initial_stack",     O_SD( _proc_charges )           },
  { SD_TYPE_UNSIGNED, "icd",               O_SD( _internal_cooldown )      },
  { SD_TYPE_DOUBLE,   "rppm",              O_SD( _rppm )                   },
  { SD_TYPE_UNSIGNED, "equip_class",       O_SD( _equipped_class )         },
  { SD_TYPE_UNSIGNED, "equip_imask",       O_SD( _equipped_invtype_mask )  },
  { SD_TYPE_UNSIGNED, "equip_scmask",      O_SD( _equipped_subclass_mask ) },
  { SD_TYPE_INT,      "cast_min",          O_SD( _cast_min )               },
  { SD_TYPE_INT,      "cast_max",          O_SD( _cast_max )               },
  { SD_TYPE_INT,      "cast_div",          O_SD( _cast_div )               },
  { SD_TYPE_DOUBLE,   "m_scaling",         O_SD( _c_scaling )              },
  { SD_TYPE_UNSIGNED, "scaling_level",     O_SD( _c_scaling_level )        },
  { SD_TYPE_UNSIGNED, "replace_spellid",   O_SD( _replace_spell_id )       },
  { SD_TYPE_UNSIGNED, "family",            O_SD( _class_flags_family )     }, // Family
  { SD_TYPE_UNSIGNED, "stance_mask",       O_SD( _stance_mask )            },
  { SD_TYPE_UNSIGNED, "mechanic",          O_SD( _mechanic )               },
  { SD_TYPE_UNSIGNED, "power_id",          O_SD( _power_id )               }, // Azereite power id
  { SD_TYPE_UNSIGNED, "essence_id",        O_SD( _essence_id )             }, // Azereite essence id
  { SD_TYPE_STR,      "desc",              O_SD( _desc )                   },
  { SD_TYPE_STR,      "tooltip",           O_SD( _tooltip )                },
  { SD_TYPE_STR,      "desc_vars",         O_SD( _desc_vars )              },
  { SD_TYPE_STR,      "rank",              O_SD( _rank_str )               },
  { SD_TYPE_UNSIGNED, "req_max_level",     O_SD( _req_max_level )          },
  { SD_TYPE_UNSIGNED, "dmg_class",         O_SD( _dmg_class )              }
};

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
    unsigned spell_id;

    // Based on the data type, see what list of spell ids we should handle, and populate the
    // result_spell_list accordingly
    switch ( data_type )
    {
      case DATA_SPELL:
      {
        for ( const spell_data_t* spell = spell_data_t::list( dbc.ptr ); spell -> id(); spell++ )
          result_spell_list.push_back( spell -> id() );
        break;
      }
      case DATA_TALENT:
      {
        for ( const talent_data_t* talent = talent_data_t::list( dbc.ptr ); talent -> id(); talent++ )
          result_spell_list.push_back( talent -> id() );
        break;
      }
      case DATA_EFFECT:
      {
        for ( const spelleffect_data_t* effect = spelleffect_data_t::list( dbc.ptr ); effect -> id(); effect++ )
          result_spell_list.push_back( effect -> id() );
        break;
      }
      case DATA_TALENT_SPELL:
      {
        for ( const talent_data_t* talent = talent_data_t::list( dbc.ptr ); talent -> id(); talent++ )
        {
          if ( ! talent -> spell_id() )
            continue;
          result_spell_list.push_back( talent -> spell_id() );
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
        for ( unsigned race = 0; race < dbc.race_ability_tree_size(); race++ )
        {
          for ( unsigned cls = 0; cls < dbc.specialization_max_class() - 1; cls++ )
          {
            for ( unsigned n = 0; n < dbc.race_ability_size(); n++ )
            {
              if ( ! ( spell_id = dbc.race_ability( race, cls, n ) ) )
                continue;

              result_spell_list.push_back( spell_id );
            }
          }
        }
        break;
      }
      case DATA_MASTERY_SPELL:
      {
        for ( unsigned cls = 0; cls < dbc.specialization_max_class(); cls++ )
        {
          for ( unsigned tree = 0; tree < dbc.specialization_max_per_class(); tree++ )
          {
            for ( unsigned n = 0; n < dbc.mastery_ability_size(); n++ )
            {
              if ( ! ( spell_id = dbc.mastery_ability( cls, tree, n ) ) )
                continue;

              result_spell_list.push_back( spell_id );
            }
          }
        }
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
  spell_data_expr_t* left;
  spell_data_expr_t* right;

  sd_expr_binary_t( dbc_t& dbc, util::string_view n, int o, spell_data_expr_t* l, spell_data_expr_t* r ) :
    spell_list_expr_t( dbc, n ), operation( o ), left( l ), right( r ) { }

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
  int                offset;
  sdata_field_type_t field_type;

  spell_data_filter_expr_t(dbc_t& dbc, expr_data_e type, util::string_view f_name, bool eq = false ) :
    spell_list_expr_t( dbc, f_name, type, eq ), offset( 0 ), field_type( SD_TYPE_INT )
  {
    for ( const auto& field : data_fields_by_type( type, effect_query ) )
    {
      if ( util::str_compare_ci( f_name, field.name ) )
      {
        offset = static_cast<int>( field.offset );
        field_type = field.type;
        break;
      }
    }
  }

  virtual bool compare( const char* data, const spell_data_expr_t& other, expression::token_e t ) const
  {
    switch ( field_type )
    {
      case SD_TYPE_INT:
      {
        int int_v  = *reinterpret_cast< const int* >( data + offset );
        int oint_v = static_cast<int>( other.result_num );
        switch ( t )
        {
          case expression::TOK_LT:     return int_v <  oint_v;
          case expression::TOK_LTEQ:   return int_v <= oint_v;
          case expression::TOK_GT:     return int_v >  oint_v;
          case expression::TOK_GTEQ:   return int_v >= oint_v;
          case expression::TOK_EQ:     return int_v == oint_v;
          case expression::TOK_NOTEQ:  return int_v != oint_v;
          default:         return false;
        }
        break;
      }
      case SD_TYPE_UNSIGNED:
      {
        unsigned unsigned_v  = *reinterpret_cast< const unsigned* >( data + offset );
        unsigned ounsigned_v = static_cast<unsigned>( other.result_num );
        switch ( t )
        {
          case expression::TOK_LT:     return unsigned_v <  ounsigned_v;
          case expression::TOK_LTEQ:   return unsigned_v <= ounsigned_v;
          case expression::TOK_GT:     return unsigned_v >  ounsigned_v;
          case expression::TOK_GTEQ:   return unsigned_v >= ounsigned_v;
          case expression::TOK_EQ:     return unsigned_v == ounsigned_v;
          case expression::TOK_NOTEQ:  return unsigned_v != ounsigned_v;
          default:         return false;
        }
        break;
      }
      case SD_TYPE_UINT64:
      {
        uint64_t unsigned_v  = *reinterpret_cast< const uint64_t* >( data + offset );
        uint64_t ounsigned_v = static_cast<uint64_t>( other.result_num );
        switch ( t )
        {
          case expression::TOK_LT:     return unsigned_v <  ounsigned_v;
          case expression::TOK_LTEQ:   return unsigned_v <= ounsigned_v;
          case expression::TOK_GT:     return unsigned_v >  ounsigned_v;
          case expression::TOK_GTEQ:   return unsigned_v >= ounsigned_v;
          case expression::TOK_EQ:     return unsigned_v == ounsigned_v;
          case expression::TOK_NOTEQ:  return unsigned_v != ounsigned_v;
          default:         return false;
        }
        break;
      }
      case SD_TYPE_DOUBLE:
      {
        double double_v = *reinterpret_cast<const double*>( data + offset );
        switch ( t )
        {
          case expression::TOK_LT:     return double_v <  other.result_num;
          case expression::TOK_LTEQ:   return double_v <= other.result_num;
          case expression::TOK_GT:     return double_v >  other.result_num;
          case expression::TOK_GTEQ:   return double_v >= other.result_num;
          case expression::TOK_EQ:     return double_v == other.result_num;
          case expression::TOK_NOTEQ:  return double_v != other.result_num;
          default:         return false;
        }
        break;
      }
      case SD_TYPE_STR:
      {
        const char* c_str = *reinterpret_cast<const char * const*>( data + offset );
        std::string string_v = c_str ? c_str : "";
        util::tokenize( string_v );
        util::string_view ostring_v = other.result_str;

        switch ( t )
        {
          case expression::TOK_EQ:    return util::str_compare_ci( string_v, ostring_v );
          case expression::TOK_NOTEQ: return ! util::str_compare_ci( string_v, ostring_v );
          case expression::TOK_IN:    return util::str_in_str_ci( string_v, ostring_v );
          case expression::TOK_NOTIN: return ! util::str_in_str_ci( string_v, ostring_v );
          default:        return false;
        }
        break;
      }
      default:
        break;
    }
    return false;
  }

  void build_list( std::vector<uint32_t>& res, const spell_data_expr_t& other, expression::token_e t ) const
  {
    for ( const auto& result_spell : result_spell_list )
    {
      // Don't bother comparing if this spell id is already in the result set.
      if ( range::find( res, result_spell ) != res.end() )
        continue;

      if ( effect_query )
      {
        const spell_data_t& spell = *dbc.spell( result_spell );

        // Compare against every spell effect
        for ( size_t j = 0; j < spell.effect_count(); j++ )
        {
          const spelleffect_data_t& effect = spell.effectN( j + 1 );

          if ( effect.id() > 0 && dbc.effect( effect.id() ) &&
               compare( reinterpret_cast<const char*>( &effect ), other, t ) )
          {
            res.push_back( result_spell );
            break;
          }
        }
      }
      else
      {
        const char* p_data;
        if ( data_type == DATA_TALENT )
          p_data = reinterpret_cast<const char*>( dbc.talent( result_spell ) );
        else if ( data_type == DATA_EFFECT )
          p_data = reinterpret_cast<const char*>( dbc.effect( result_spell ) );
        else
          p_data = reinterpret_cast<const char*>( dbc.spell( result_spell ) );
        if ( p_data && compare( p_data, other, t ) )
          res.push_back( result_spell );
      }
    }
  }

  std::vector<uint32_t> operator==( const spell_data_expr_t& other ) override
  {
    std::vector<uint32_t> res;

    if ( other.result_tok != expression::TOK_NUM && other.result_tok != expression::TOK_STR )
    {
      throw std::invalid_argument(fmt::format("Unsupported expression operator == for left='{}' ({}), right='{}' ({})",
        name_str,
        result_tok,
        other.name_str,
        other.result_tok));
    }
    else
      build_list( res, other, expression::TOK_EQ );

    return res;
  }

  std::vector<uint32_t> operator!=( const spell_data_expr_t& other ) override
  {
    std::vector<uint32_t> res;

    if ( other.result_tok != expression::TOK_NUM && other.result_tok != expression::TOK_STR )
    {
      throw std::invalid_argument(fmt::format("Unsupported expression operator != for left='{}' ({}), right='{}' ({})",
        name_str,
        result_tok,
        other.name_str,
        other.result_tok));
    }
    else
      build_list( res, other, expression::TOK_NOTEQ );

    return res;
  }

  std::vector<uint32_t> operator<( const spell_data_expr_t& other ) override
  {
    std::vector<uint32_t> res;

    if ( other.result_tok != expression::TOK_NUM ||
         ( field_type != SD_TYPE_INT && field_type != SD_TYPE_UNSIGNED && field_type != SD_TYPE_UINT64 && field_type != SD_TYPE_DOUBLE )  )
    {
      throw std::invalid_argument(fmt::format("Unsupported expression operator < for left='{}' ({}), right='{}' ({})",
        name_str,
        result_tok,
        other.name_str,
        other.result_tok));
    }
    else
      build_list( res, other, expression::TOK_LT );

    return res;
  }

  std::vector<uint32_t> operator<=( const spell_data_expr_t& other ) override
  {
    std::vector<uint32_t> res;

    if ( other.result_tok != expression::TOK_NUM ||
         ( field_type != SD_TYPE_INT && field_type != SD_TYPE_UNSIGNED && field_type != SD_TYPE_UINT64 && field_type != SD_TYPE_DOUBLE )  )
    {
      throw std::invalid_argument(fmt::format("Unsupported expression operator <= for left='{}' ({}), right='{}' ({})",
        name_str,
        result_tok,
        other.name_str,
        other.result_tok));
    }
    else
      build_list( res, other, expression::TOK_LTEQ );

    return res;
  }

  std::vector<uint32_t> operator>( const spell_data_expr_t& other ) override
  {
    std::vector<uint32_t> res;

    if ( other.result_tok != expression::TOK_NUM ||
         ( field_type != SD_TYPE_INT && field_type != SD_TYPE_UNSIGNED && field_type != SD_TYPE_UINT64 && field_type != SD_TYPE_DOUBLE )  )
    {
      throw std::invalid_argument(fmt::format("Unsupported expression operator > for left='{}' ({}), right='{}' ({})",
        name_str,
        result_tok,
        other.name_str,
        other.result_tok));
    }
    else
      build_list( res, other, expression::TOK_GT );

    return res;
  }

  std::vector<uint32_t> operator>=( const spell_data_expr_t& other ) override
  {
    std::vector<uint32_t> res;

    if ( other.result_tok != expression::TOK_NUM ||
         ( field_type != SD_TYPE_INT && field_type != SD_TYPE_UNSIGNED && field_type != SD_TYPE_UINT64 && field_type != SD_TYPE_DOUBLE )  )
    {
      throw std::invalid_argument(fmt::format("Unsupported expression operator >= for left='{}' ({}), right='{}' ({})",
        name_str,
        result_tok,
        other.name_str,
        other.result_tok));
    }
    else
      build_list( res, other, expression::TOK_GTEQ );

    return res;
  }

  std::vector<uint32_t> in( const spell_data_expr_t& other ) override
  {
    std::vector<uint32_t> res;

    if ( other.result_tok != expression::TOK_STR || field_type != SD_TYPE_STR )
    {
      throw std::invalid_argument(fmt::format("Unsupported expression operator ~ for left='{}' ({}), right='{}' ({})",
        name_str,
        result_tok,
        other.name_str,
        other.result_tok));
    }
    else
      build_list( res, other, expression::TOK_IN );

    return res;
  }

  std::vector<uint32_t> not_in( const spell_data_expr_t& other ) override
  {
    std::vector<uint32_t> res;

    if ( other.result_tok != expression::TOK_STR || field_type != SD_TYPE_STR )
    {
      throw std::invalid_argument(fmt::format("Unsupported expression operator !~ for left='{}' ({}), right='{}' ({})",
        name_str,
        result_tok,
        other.name_str,
        other.result_tok));
    }
    else
      build_list( res, other, expression::TOK_NOTIN );

    return res;
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

spell_data_expr_t* build_expression_tree( dbc_t& dbc,
                                          const std::vector<expression::expr_token_t>& tokens )
{
  auto_dispose< std::vector<spell_data_expr_t*> > stack;

  for ( auto& t : tokens )
  {
    if ( t.type == expression::TOK_NUM )
      stack.push_back( new spell_data_expr_t( dbc, t.label, std::stod( t.label ) ) );
    else if ( t.type == expression::TOK_STR )
    {
      spell_data_expr_t* e = spell_data_expr_t::create_spell_expression( dbc, t.label );

      if ( ! e )
      {
        throw std::invalid_argument(fmt::format("Unable to decode expression function '{}'.", t.label));
      }
      stack.push_back( e );
    }
    else if ( expression::is_binary( t.type ) )
    {
      if ( stack.size() < 2 )
        return nullptr;
      spell_data_expr_t* right = stack.back(); stack.pop_back();
      spell_data_expr_t* left  = stack.back(); stack.pop_back();
      if ( ! left || ! right )
        return nullptr;
      stack.push_back( new sd_expr_binary_t( dbc, t.label, t.type, left, right ) );
    }
  }

  if ( stack.size() != 1 )
    return nullptr;

  spell_data_expr_t* res = stack.back();
  stack.pop_back();
  return res;
}

} // anonymous namespace ====================================================

spell_data_expr_t* spell_data_expr_t::create_spell_expression( dbc_t& dbc, util::string_view name_str )
{
  std::vector<std::string> splits = util::string_split( name_str, "." );

  if ( splits.empty() || splits.size() > 3 )
    return nullptr;

  expr_data_e data_type = parse_data_type( splits[ 0 ] );

  if ( splits.size() == 1 )
  {
    // No split, access raw list or create a normal expression
    if ( data_type == static_cast<expr_data_e>( -1 ) )
      return new spell_data_expr_t( dbc, name_str, name_str );
    else
      return new spell_list_expr_t( dbc, splits[ 0 ], data_type );
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
    return new spell_class_expr_t( dbc, data_type );
  else if ( util::str_compare_ci( splits[ 1 ], "race" ) )
    return new spell_race_expr_t( dbc, data_type );
  else if ( util::str_compare_ci( splits[ 1 ], "attribute" ) )
    return new spell_attribute_expr_t( dbc, data_type );
  else if ( data_type != DATA_TALENT && util::str_compare_ci( splits[ 1 ], "flag" ) )
    return new spell_flag_expr_t( dbc, data_type );
  else if ( data_type != DATA_TALENT && util::str_compare_ci( splits[ 1 ], "school" ) )
    return new spell_school_expr_t( dbc, data_type );

  for ( const auto& field : data_fields_by_type( data_type, effect_query ) )
  {
    if ( ! field.name.empty() && util::str_compare_ci( splits[ 1 ], field.name ) )
    {
      return new spell_data_filter_expr_t( dbc, data_type, field.name, effect_query );
    }
  }

  return nullptr;
}

spell_data_expr_t* spell_data_expr_t::parse( sim_t* sim, const std::string& expr_str )
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
    spell_data_expr_t* e = build_expression_tree(*sim->dbc, tokens);
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
