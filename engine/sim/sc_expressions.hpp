// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include <string>
#include <vector>
#include <functional>
#include <memory>

#include "util/timespan.hpp"
#include "util/span.hpp"
#include "util/string_view.hpp"

struct action_t;
struct sim_t;
struct player_t;
struct expr_t;

// Expressions ==============================================================

namespace expression
{
enum token_e
{
  TOK_UNKNOWN = 0,
  TOK_PLUS,
  TOK_MINUS,
  TOK_MULT,
  TOK_DIV,
  TOK_MOD,
  TOK_ADD,
  TOK_SUB,
  TOK_AND,
  TOK_OR,
  TOK_XOR,
  TOK_NOT,
  TOK_EQ,
  TOK_NOTEQ,
  TOK_LT,
  TOK_LTEQ,
  TOK_GT,
  TOK_GTEQ,
  TOK_LPAR,
  TOK_RPAR,
  TOK_IN,
  TOK_NOTIN,
  TOK_NUM,
  TOK_STR,
  TOK_ABS,
  TOK_SPELL_LIST,
  TOK_FLOOR,
  TOK_CEIL,
  TOK_MAX,
  TOK_MIN,
  TOK_EOF, // "end of stream"
};

struct expr_token_t
{
  token_e type;
  std::string label;
};

bool is_unary( token_e );
bool is_binary( token_e );
std::vector<expr_token_t> parse_tokens( action_t* action, util::string_view expr_str );
void print_tokens( util::span<const expr_token_t> tokens, sim_t* sim );
bool convert_to_rpn( std::vector<expr_token_t>& tokens );
std::unique_ptr<expr_t> build_player_expression_tree(
    player_t& player, std::vector<expression::expr_token_t>& tokens,
    bool optimize );
}

/// Action expression
struct expr_t
{
protected:
  expr_t( util::string_view name, expression::token_e op = expression::TOK_UNKNOWN )
    : op_( op )
#if !defined( NDEBUG )
      ,
      id_( get_global_id() ),
      name_(name)
#endif
  {
    (void)name;
  }

public:
  virtual ~expr_t()
  {
  }

  const char* name() const
  {
#if !defined( NDEBUG )
    return name_.c_str();
#else
    return "anonymous expression";
#endif
  }
  int id() const
  {
#if !defined( NDEBUG )
    return id_;
#else
    return -1;
#endif
  }

  double eval()
  {
    return evaluate();
  }
  bool success()
  {
    return eval() != 0;
  }
  bool always_true()
  {
    double v;
    return is_constant( &v ) && v != 0.0;
  }
  bool always_false()
  {
    double v;
    return is_constant( &v ) && v == 0.0;
  }

  static double coerce( timespan_t t )
  {
    return t.total_seconds();
  }
  template <typename T>
  static double coerce( T t )
  {
    return static_cast<double>( t );
  }

  static std::unique_ptr<expr_t> parse( action_t*, util::string_view expr_str,
                        bool optimize = false );
  template<class T>
  static std::unique_ptr<expr_t> create_constant( util::string_view name, T value );

  static void optimize_expression(std::unique_ptr<expr_t>& expression, int spacing = 0)
  {
    if (!expression)
    {
      return;
    }
    if (auto optimized = expression->build_optimized_expression(spacing))
    {
      expression.swap(optimized);
    }
  }

  virtual double evaluate() = 0;

  virtual bool is_constant( double* /*return_value*/ )
  {
    return false;
  }

  expression::token_e op_;

private:
  /* Attempts to create a optimized version of the expression.
  Should return null if no improved version can be built.
  */
  virtual std::unique_ptr<expr_t> build_optimized_expression( int /* spacing */ )
  {
    return {};
  }
#if !defined( NDEBUG )
  int id_;
  std::string name_;

  int get_global_id();
#endif
};

class const_expr_t : public expr_t
{
  double value;

public:
  const_expr_t( util::string_view name, double value_ )
    : expr_t( name, expression::TOK_NUM ), value( value_ )
  {
  }

  double evaluate() override  // override
  {
    return value;
  }

  bool is_constant( double* v ) override  // override
  {
    *v = value;
    return true;
  }
};

// Reference Expression - ref_expr_t
// Class Template to create a expression with a reference ( ref ) of arbitrary
// type T, and evaluate that reference
template <typename T>
struct ref_expr_t : public expr_t
{
public:
  ref_expr_t( util::string_view name, const T& t_ ) : expr_t( name ), t( t_ )
  {
  }

private:
  const T& t;
  virtual double evaluate() override
  {
    return coerce( t );
  }
};

// Template to return a reference expression
template <typename T>
inline std::unique_ptr<expr_t> make_ref_expr( util::string_view name, T& t )
{
  return std::make_unique<ref_expr_t<T>>( name, const_cast<const T&>( t ) );
}

// Function Expression - fn_expr_t
// Class Template to create a function ( fn ) expression with arbitrary functor
// f, which gets evaluated
template <typename F>
struct fn_expr_t : public expr_t
{
public:
  template <typename U = F>
  fn_expr_t( util::string_view name, U&& f_ ) : expr_t( name ), f( std::forward<U>( f_ ) )
  { }

private:
  F f;

  virtual double evaluate() override
  {
    return coerce( f() );
  }
};

struct target_wrapper_expr_t : public expr_t
{
  action_t& action;
  std::vector<std::unique_ptr<expr_t>> proxy_expr;
  std::string suffix_expr_str;

  // Inlined
  target_wrapper_expr_t( action_t& a, util::string_view name_str,
                         util::string_view expr_str );
  virtual player_t* target() const;
  double evaluate() override;
};

// Template to return a function expression
template <typename F>
inline std::unique_ptr<expr_t> make_fn_expr( util::string_view name, F&& f )
{
  return std::make_unique<fn_expr_t<std::decay_t<F>>>( name, std::forward<F>( f ) );
}

// Make member function expression - make_mem_fn_expr
// Template to return function expression that calls a member ( mem ) function (
// fn ) f on reference t.
template <typename F, typename T>
inline std::unique_ptr<expr_t> make_mem_fn_expr( util::string_view name, T& t, F f )
{
  return make_fn_expr( name, std::bind( std::mem_fn( f ), &t ) );
}

template<class T>
inline std::unique_ptr<expr_t> expr_t::create_constant( util::string_view name, T value )
{
  return std::make_unique<const_expr_t>( name, coerce(value) );
}

