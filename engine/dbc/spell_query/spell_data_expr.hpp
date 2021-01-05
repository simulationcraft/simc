// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "sim/sc_expressions.hpp"
#include "util/string_view.hpp"

#include <memory>

class dbc_t;

// Spell query expression types
enum expr_data_e : int
{
  DATA_SPELL = 0,
  DATA_TALENT,
  DATA_EFFECT,
  DATA_TALENT_SPELL,
  DATA_CLASS_SPELL,
  DATA_RACIAL_SPELL,
  DATA_MASTERY_SPELL,
  DATA_SPECIALIZATION_SPELL,
  DATA_AZERITE_SPELL,
  DATA_COVENANT_SPELL,
  DATA_SOULBIND_SPELL,
  DATA_CONDUIT_SPELL,
  DATA_RUNEFORGE_SPELL
};

struct spell_data_expr_t
{
  std::string name_str;
  dbc_t& dbc;
  expr_data_e data_type;
  bool effect_query;

  expression::token_e result_tok;
  double result_num;
  std::vector<uint32_t> result_spell_list;
  std::string result_str;

  spell_data_expr_t( dbc_t& dbc, util::string_view n,
                     expr_data_e dt = DATA_SPELL, bool eq = false,
                     expression::token_e t = expression::TOK_UNKNOWN )
    : name_str( n ),
      dbc( dbc ),
      data_type( dt ),
      effect_query( eq ),
      result_tok( t ),
      result_num( 0 ),
      result_spell_list(),
      result_str( "" )
  {
  }
  spell_data_expr_t(dbc_t& dbc, util::string_view n, double constant_value )
    : name_str( n ),
      dbc(dbc),
      data_type( DATA_SPELL ),
      effect_query( false ),
      result_tok( expression::TOK_NUM ),
      result_num( constant_value ),
      result_spell_list(),
      result_str( "" )
  {
  }
  spell_data_expr_t(dbc_t& dbc, util::string_view n,
                     util::string_view constant_value )
    : name_str( n ),
      dbc(dbc),
      data_type( DATA_SPELL ),
      effect_query( false ),
      result_tok( expression::TOK_STR ),
      result_num( 0.0 ),
      result_spell_list(),
      result_str( constant_value )
  {
  }
  spell_data_expr_t(dbc_t& dbc, util::string_view n,
                     const std::vector<uint32_t>& constant_value )
    : name_str( n ),
      dbc(dbc),
      data_type( DATA_SPELL ),
      effect_query( false ),
      result_tok( expression::TOK_SPELL_LIST ),
      result_num( 0.0 ),
      result_spell_list( constant_value ),
      result_str( "" )
  {
  }
  virtual ~spell_data_expr_t() = default;

  virtual int evaluate()
  {
    return result_tok;
  }
  const char* name()
  {
    return name_str.c_str();
  }

  virtual std::vector<uint32_t> operator|( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator&( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator-( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }

  virtual std::vector<uint32_t> operator<( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator>( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator<=( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator>=( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator==( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator!=( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }

  virtual std::vector<uint32_t> in( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> not_in( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }

  static std::unique_ptr<spell_data_expr_t> parse( sim_t* sim, util::string_view expr_str );
  static std::unique_ptr<spell_data_expr_t> create_spell_expression( dbc_t& dbc, util::string_view name_str );
};
