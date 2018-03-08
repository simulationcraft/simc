// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include <string>
#include <vector>
#include <functional>

#include "sc_timespan.hpp"

struct action_t;
struct sim_t;
struct player_t;

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
  TOK_MIN
};

struct expr_token_t
{
  token_e type;
  std::string label;
};

int precedence( token_e );
bool is_unary( token_e );
bool is_binary( token_e );
token_e next_token( action_t* action, const std::string& expr_str,
                    int& current_index, std::string& token_str,
                    token_e prev_token );
std::vector<expr_token_t> parse_tokens( action_t* action,
                                        const std::string& expr_str );
void print_tokens( std::vector<expr_token_t>& tokens, sim_t* sim );
void convert_to_unary( std::vector<expr_token_t>& tokens );
bool convert_to_rpn( std::vector<expr_token_t>& tokens );
}

/// Action expression
struct expr_t
{
protected:
  expr_t( const std::string& name, expression::token_e op = expression::TOK_UNKNOWN )
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

  virtual const char* name() const
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

  static expr_t* parse( action_t*, const std::string& expr_str,
                        bool optimize = false );
  template<class T>
  static expr_t* create_constant( const std::string& name, T value );

  virtual expr_t* optimize( int /* spacing */ = 0 )
  { /* spacing = 0; */
    return this;
  }
  virtual double evaluate() = 0;

  virtual bool is_constant( double* /*return_value*/ )
  {
    return false;
  }

  expression::token_e op_;

private:
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
  const_expr_t( const std::string& name, double value_ )
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
  ref_expr_t( const std::string& name, const T& t_ ) : expr_t( name ), t( t_ )
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
inline expr_t* make_ref_expr( const std::string& name, T& t )
{
  return new ref_expr_t<T>( name, const_cast<const T&>( t ) );
}

// Function Expression - fn_expr_t
// Class Template to create a function ( fn ) expression with arbitrary functor
// f, which gets evaluated
template <typename F>
struct fn_expr_t : public expr_t
{
public:
  fn_expr_t( const std::string& name, F&& f_ ) : expr_t( name ), f( f_ )
  {
  }

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
  std::vector<expr_t*> proxy_expr;
  std::string suffix_expr_str;

  // Inlined
  target_wrapper_expr_t( action_t& a, const std::string& name_str,
                         const std::string& expr_str );
  virtual player_t* target() const;
  double evaluate() override;

  ~target_wrapper_expr_t()
  {
    range::dispose( proxy_expr );
  }
};

// Template to return a function expression
template <typename F>
inline expr_t* make_fn_expr( const std::string& name, F&& f )
{
  return new fn_expr_t<F>( name, std::forward<F>( f ) );
}

// Make member function expression - make_mem_fn_expr
// Template to return function expression that calls a member ( mem ) function (
// fn ) f on reference t.
template <typename F, typename T>
inline expr_t* make_mem_fn_expr( const std::string& name, T& t, F f )
{
  return make_fn_expr( name, std::bind( std::mem_fn( f ), &t ) );
}

template<class T>
inline expr_t* expr_t::create_constant( const std::string& name, T value )
{
  return new const_expr_t( name, coerce(value) );
}

