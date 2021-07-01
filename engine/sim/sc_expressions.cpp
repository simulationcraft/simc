// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "sc_expressions.hpp"
#include "action/sc_action.hpp"
#include "player/sc_player.hpp"
#include "sim/sc_sim.hpp"
#include <atomic>

namespace expression
{

namespace
{  // ANONYMOUS ====================================================

constexpr bool EXPRESSION_DEBUG = false;
constexpr int EXPRESSION_CONSIDER_MARKING_THRESHOLD = 2; // number of false/true events to have been collected before offering the 'consider marking constant' info.

struct lexer_t
{
  struct token_t {
    token_e type = TOK_UNKNOWN;
    util::string_view str;
  };

  util::string_view input;
  size_t current_len = 0;

  explicit lexer_t( util::string_view s )
    : input( s )
  {}

  char peek() const
  {
    return current_len < input.size() ? input[ current_len ] : '\0';
  }

  bool match( char c )
  {
    if ( starts_with( input.substr( current_len ), c ) ) {
      current_len++;
      return true;
    }
    return false;
  }

  token_t yield_token( token_e type )
  {
    token_t token;
    token.type = type;
    token.str = input.substr( 0, current_len );
    input.remove_prefix( current_len );
    return token;
  }

  token_t next()
  {
    if ( input.empty() )
      return yield_token( TOK_EOF );

    const char ch = input.front();
    current_len = 1;

    // numbers
    if ( is_digit( ch ) ) {
      while ( is_digit( peek() ) )
        current_len++;
      if ( match( '.' ) ) {
        while ( is_digit( peek() ) )
          current_len++;
      }
      return yield_token( TOK_NUM );
    }

    // "strings" (identifiers, really)
    if ( is_alpha( ch ) ) {
      for ( char c = peek(); is_alpha( c ) || is_digit( c ) || c == '_' || c == '.'; c = peek() )
        current_len++;
      return yield_token( TOK_STR );
    }

    switch ( ch )
    {
      case '(': return yield_token( TOK_LPAR );
      case ')': return yield_token( TOK_RPAR );
      case '+': return yield_token( TOK_PLUS );
      case '-': return yield_token( TOK_MINUS );
      case '*': return yield_token( TOK_MULT );
      case '%':
      {
        // as '/' is taken by option separators we use '%' as division and '%%' as modulus
        if ( match( '%' ) )
          return yield_token( TOK_MOD );
        return yield_token( TOK_DIV );
      }
      case '@': return yield_token( TOK_ABS );
      case '&': match( '&' ); return yield_token( TOK_AND );
      case '|': match( '|' ); return yield_token( TOK_OR );
      case '^': match( '^' ); return yield_token( TOK_XOR );
      case '~': match( '~' ); return yield_token( TOK_IN );
      case '=': match( '=' ); return yield_token( TOK_EQ );
      case '!':
      {
        if ( match( '=' ) )
          return yield_token( TOK_NOTEQ );
        if ( match( '~' ) )
          return yield_token( TOK_NOTIN );
        return yield_token( TOK_NOT );
      }
      case '<':
      {
        if ( match( '=' ) )
          return yield_token( TOK_LTEQ );
        if ( match( '?' ) )
          return yield_token( TOK_MAX );
        return yield_token( TOK_LT );
      }
      case '>':
      {
        if ( match( '=' ) )
          return yield_token( TOK_GTEQ );
        if ( match( '?' ) )
          return yield_token( TOK_MIN );
        return yield_token( TOK_GT );
      }

      default: break;
    }
    return yield_token( TOK_UNKNOWN );
  }

  static constexpr bool is_digit( char c ) {
    return c >= '0' && c <= '9';
  }
  static constexpr bool is_alpha( char c ) {
    return ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' );
  }
  static constexpr char lowercase( char c ) {
    return ( c >= 'A' && c <= 'Z' ) ? ( (char)(int(c) | 0x20) ) : c;
  }
};

// Unary Operators ==========================================================

template <class F>
class expr_unary_t : public expr_t
{
  std::unique_ptr<expr_t> input;

public:
  expr_unary_t( util::string_view n, token_e o, std::unique_ptr<expr_t> i )
    : expr_t( n, o ), input( std::move(i) )
  {
    assert(input);
  }

  double evaluate() override  // override
  {
    return F()( input->eval() );
  }
};

namespace unary
{
struct abs
{
  double operator()( const double& val ) const
  {
    return std::fabs( val );
  }
};
struct floor
{
  double operator()( const double& val ) const
  {
    return std::floor( val );
  }
};
struct ceil
{
  double operator()( const double& val ) const
  {
    return std::ceil( val );
  }
};
}

namespace binary
{
  template<class T = void>
  struct max
  {
    constexpr T operator()(const T& _Left, const T& _Right) const
    {
      return (std::max(_Left, _Right));
    }
  };

  template<class T = void>
  struct min
  {
    constexpr T operator()(const T& _Left, const T& _Right) const
    {
      return (std::min(_Left, _Right));
    }
  };

  template<class T = void>
  struct modulus
  {
    constexpr T operator()(const T& _Left, const T& _Right) const
    {
      return (std::fmod(_Left, _Right));
    }
  };
}

std::unique_ptr<expr_t> select_unary( util::string_view name, token_e op, std::unique_ptr<expr_t> input )
{
  switch ( op )
  {
    case TOK_PLUS:
      return input;  // No need to modify input
    case TOK_MINUS:
      return std::make_unique<expr_unary_t<std::negate<double>>>( name, op, std::move(input) );
    case TOK_NOT:
      return std::make_unique<expr_unary_t<std::logical_not<double>>>( name, op, std::move(input) );
    case TOK_ABS:
      return std::make_unique<expr_unary_t<unary::abs>>( name, op, std::move(input) );
    case TOK_FLOOR:
      return std::make_unique<expr_unary_t<unary::floor>>( name, op, std::move(input) );
    case TOK_CEIL:
      return std::make_unique<expr_unary_t<unary::ceil>>( name, op, std::move(input) );

    default:
      assert( false );
      return {};  // throw?
  }
}

// Binary Operators =========================================================

class binary_base_t : public expr_t
{
public:
  std::unique_ptr<expr_t> left;
  std::unique_ptr<expr_t> right;

  binary_base_t( util::string_view n, token_e o, std::unique_ptr<expr_t> l, std::unique_ptr<expr_t> r )
    : expr_t( n, o ), left( std::move(l) ), right( std::move(r) )
  {
    assert(left);
    assert(right);
  }
};

class logical_and_t : public binary_base_t
{
public:
  logical_and_t( util::string_view n, std::unique_ptr<expr_t> l, std::unique_ptr<expr_t> r )
    : binary_base_t( n, TOK_AND, std::move(l), std::move(r) )
  {
  }

  double evaluate() override  // override
  {
    return left->eval() && right->eval();
  }
};

class logical_or_t : public binary_base_t
{
public:
  logical_or_t( util::string_view n, std::unique_ptr<expr_t> l, std::unique_ptr<expr_t> r )
    : binary_base_t( n, TOK_OR, std::move(l), std::move(r) )
  {
  }

  double evaluate() override  // override
  {
    return left->eval() || right->eval();
  }
};

class logical_xor_t : public binary_base_t
{
public:
  logical_xor_t( util::string_view n, std::unique_ptr<expr_t> l, std::unique_ptr<expr_t> r )
    : binary_base_t( n, TOK_XOR, std::move(l), std::move(r) )
  {
  }

  double evaluate() override  // override
  {
    return bool( left->eval() != 0 ) != bool( right->eval() != 0 );
  }
};

template <template <typename> class F, typename T = double>
class expr_binary_t : public binary_base_t
{
public:
  expr_binary_t( util::string_view n, token_e o, std::unique_ptr<expr_t> l, std::unique_ptr<expr_t> r )
    : binary_base_t( n, o, std::move(l), std::move(r) )
  {
  }

  double evaluate() override  // override
  {
    return static_cast<double>( F<T>()( static_cast<T>( left->eval() ), static_cast<T>( right->eval() ) ) );
  }
};

std::unique_ptr<expr_t> select_binary( util::string_view name, token_e op, std::unique_ptr<expr_t> left,
                       std::unique_ptr<expr_t> right )
{
  switch ( op )
  {
    case TOK_AND:
      return std::make_unique<logical_and_t>( name, std::move(left), std::move(right) );
    case TOK_OR:
      return std::make_unique<logical_or_t>( name, std::move(left), std::move(right) );
    case TOK_XOR:
      return std::make_unique<logical_xor_t>( name, std::move(left), std::move(right) );

    case TOK_ADD:
      return std::make_unique<expr_binary_t<std::plus>>( name, op, std::move(left), std::move(right) );
    case TOK_SUB:
      return std::make_unique<expr_binary_t<std::minus>>( name, op, std::move(left), std::move(right) );
    case TOK_MULT:
      return std::make_unique<expr_binary_t<std::multiplies>>( name, op, std::move(left), std::move(right) );
    case TOK_DIV:
      return std::make_unique<expr_binary_t<std::divides>>( name, op, std::move(left), std::move(right) );
    case TOK_MOD:
      return std::make_unique<expr_binary_t<binary::modulus>>( name, op, std::move(left), std::move(right) );

    case TOK_MAX:
      return std::make_unique<expr_binary_t<binary::max>>(name, op, std::move(left), std::move(right));
    case TOK_MIN:
      return std::make_unique<expr_binary_t<binary::min>>(name, op, std::move(left), std::move(right));

    case TOK_EQ:
      return std::make_unique<expr_binary_t<std::equal_to>>( name, op, std::move(left), std::move(right) );
    case TOK_NOTEQ:
      return std::make_unique<expr_binary_t<std::not_equal_to>>( name, op, std::move(left), std::move(right) );
    case TOK_LT:
      return std::make_unique<expr_binary_t<std::less>>( name, op, std::move(left), std::move(right) );
    case TOK_LTEQ:
      return std::make_unique<expr_binary_t<std::less_equal>>( name, op, std::move(left), std::move(right) );
    case TOK_GT:
      return std::make_unique<expr_binary_t<std::greater>>( name, op, std::move(left), std::move(right) );
    case TOK_GTEQ:
      return std::make_unique<expr_binary_t<std::greater_equal>>( name, op, std::move(left), std::move(right) );

    default:
      assert( false );
      return {};  // throw?
  }
}

std::unique_ptr<expr_t> select_unary( bool analyze, util::string_view name, token_e op, std::unique_ptr<expr_t> input );
std::unique_ptr<expr_t> select_binary( bool analyze, util::string_view name, token_e op, std::unique_ptr<expr_t> left,
                                       std::unique_ptr<expr_t> right );

// Analyzing Unary Operators ================================================

template <class F>
class expr_analyze_unary_t : public expr_t
{
  std::unique_ptr<expr_t> input;

public:
  expr_analyze_unary_t( util::string_view n, token_e o, std::unique_ptr<expr_t> i )
    : expr_t( n, o ), input( std::move(i) )
  {
    assert( input );
  }

  double evaluate() override  // override
  {
    return F()( input->eval() );
  }

  std::unique_ptr<expr_t> build_optimized_expression( bool analyze_further, int spacing ) override
  {
    if (EXPRESSION_DEBUG)
    {
      printf("%*d %s ( %s )\n", spacing, id(), name(), input->name());
    }

    expr_t::optimize_expression(input, analyze_further, spacing + 2);

    double input_value;
    if ( input->is_constant( &input_value ) )
    {
      double result = F()( input_value );
      if (EXPRESSION_DEBUG)
      {
        printf("Reduced %*d %s (%s) unary expression to %f\n", spacing, id(),
          name(), input->name(), result);
      }
      auto new_name = fmt::format("const_unary('{}')", input->name());
      return std::make_unique<const_expr_t>( new_name, result );
    }

    return select_unary( analyze_further, name(), op_, std::move(input));
  }
  
  bool is_analyze_expression() override
  {
    return true;
  }
};

std::unique_ptr<expr_t> select_analyze_unary( util::string_view name, token_e op,
                              std::unique_ptr<expr_t> input )
{
  switch ( op )
  {
    case TOK_PLUS:
      return input;
    case TOK_MINUS:
      return std::make_unique<expr_analyze_unary_t<std::negate<double>>>( name, op, std::move(input) );
    case TOK_NOT:
      return std::make_unique<expr_analyze_unary_t<std::logical_not<double>>>( name, op,
                                                                 std::move(input) );
    case TOK_ABS:
      return std::make_unique<expr_analyze_unary_t<unary::abs>>( name, op, std::move(input) );
    case TOK_FLOOR:
      return std::make_unique<expr_analyze_unary_t<unary::floor>>( name, op, std::move(input) );
    case TOK_CEIL:
      return std::make_unique<expr_analyze_unary_t<unary::ceil>>( name, op, std::move(input) );

    default:
      assert( false );
      return {};  // throw?
  }
}

std::unique_ptr<expr_t> select_unary( bool analyze, util::string_view name, token_e op, std::unique_ptr<expr_t> input )
{
  if (analyze)
  {
    return select_analyze_unary(name, std::move( op ), std::move( input ));
  }
  return select_unary(name, std::move( op ), std::move( input ));
}

// Analyzing Binary Operators ===============================================

class analyze_binary_base_t : public binary_base_t
{
protected:
  token_e op;
  double result;
  double left_result, right_result;
  uint64_t left_true, right_true;
  uint64_t left_false, right_false;

public:
  analyze_binary_base_t( util::string_view n, token_e o, std::unique_ptr<expr_t> l, std::unique_ptr<expr_t> r )
    : binary_base_t( n, o, std::move(l), std::move(r) ),
      op(),
      result( 0 ),
      left_result( 0 ),
      right_result( 0 ),
      left_true( 0 ),
      right_true( 0 ),
      left_false( 0 ),
      right_false( 0 )
  {
    assert( left );
    assert( right );
  }

  void analyze_boolean()
  {
    if ( left_result != 0 )
      left_true++;
    else
      left_false++;

    if ( right_result != 0 )
      right_true++;
    else
      right_false++;
  }

  bool is_analyze_expression() override
  {
    return true;
  }
};

template <template <typename> class F, typename T = double>
struct left_reduced_t : public expr_t
{
  double left;
  std::unique_ptr<expr_t> right;
  left_reduced_t( util::string_view n, token_e o, double l, std::unique_ptr<expr_t> r )
    : expr_t( n, o ), left( l ), right( std::move(r) )
  {
  }
  
  std::unique_ptr<expr_t> build_optimized_expression( bool analyze_further, int spacing ) override
  {
    expr_t::optimize_expression( right, analyze_further, spacing + 2 );
    double right_value;
    bool right_constant = right->is_constant( &right_value );
    if ( right_constant )
    {
      auto result = static_cast<double>( F<T>()( static_cast<T>( left ), static_cast<T>( right_value ) ) );
      if ( EXPRESSION_DEBUG )
      {
        printf( "Reduced %*d %s binary expression to %f\n", spacing, id(), name(), result );
      }
      return std::make_unique<const_expr_t>( fmt::format( "const_binary({})", name() ), result );
    }
    return {};
  }

  double evaluate() override
  {
    return static_cast<double>( F<T>()( static_cast<T>( left ), static_cast<T>( right->eval() ) ) );
  }
};

template <template <typename> class F, typename T = double>
struct right_reduced_t : public expr_t
{
  std::unique_ptr<expr_t> left;
  double right;
  right_reduced_t( util::string_view n, token_e o, std::unique_ptr<expr_t> l, double r )
    : expr_t( n, o ), left( std::move(l) ), right( r )
  {
  }

  std::unique_ptr<expr_t> build_optimized_expression( bool analyze_further, int spacing ) override
  {
    expr_t::optimize_expression( left, analyze_further, spacing + 2 );
    double left_value;
    bool left_constant = left->is_constant( &left_value );
    if ( left_constant )
    {
      auto result = static_cast<double>( F<T>()( static_cast<T>( left_value ), static_cast<T>( right ) ) );
      if ( EXPRESSION_DEBUG )
      {
        printf( "Reduced %*d %s binary expression to %f\n", spacing, id(), name(), result );
      }
      return std::make_unique<const_expr_t>( fmt::format( "const_binary({})", name() ), result );
    }
    return {};
  }

  double evaluate() override
  {
    return static_cast<double>( F<T>()( static_cast<T>( left->eval() ), static_cast<T>( right ) ) );
  }
};
class analyze_logical_and_t : public analyze_binary_base_t
{
public:
  analyze_logical_and_t( util::string_view n, std::unique_ptr<expr_t> l, std::unique_ptr<expr_t> r )
    : analyze_binary_base_t( n, TOK_AND, std::move(l), std::move(r) )
  {
  }

  double evaluate() override  // override
  {
    left_result  = left->eval();
    right_result = right->eval();
    result       = ( left_result && right_result ) ? 1.0 : 0.0;
    analyze_boolean();
    return result;
  }

  std::unique_ptr<expr_t> build_optimized_expression( bool analyze_further, int spacing ) override
  {
    if ( EXPRESSION_DEBUG )
    {
      fmt::print( "{:>{}} and {} {} {} {} ( {} {} )\n", id(), spacing,
                    left_true, left_false, right_true, right_false,
                    left->name(), right->name() );
    }
    expr_t::optimize_expression(left, analyze_further, spacing + 2);
    expr_t::optimize_expression(right, analyze_further, spacing + 2);
    bool left_always_true   = left->always_true();
    bool left_always_false  = left->always_false();
    bool right_always_true  = right->always_true();
    bool right_always_false = right->always_false();
    if ( EXPRESSION_DEBUG )
    {
      if ( left->op_ == TOK_UNKNOWN )
      {
        if ( left_false + left_true >= EXPRESSION_CONSIDER_MARKING_THRESHOLD && ( ( !left_always_true && left_false == 0 ) ||
             ( !left_always_false && left_true == 0 ) ) )
        {
          printf( "consider marking expression %d %s as constant\n", left->id(),
                  left->name() );
        }
      }
      if ( right->op_ == TOK_UNKNOWN )
      {
        if ( left_false + left_true >= EXPRESSION_CONSIDER_MARKING_THRESHOLD && ( ( !right_always_true && right_false == 0 ) ||
             ( !right_always_false && right_true == 0 ) ) )
        {
          printf( "consider marking expression %d %s as constant\n",
                  right->id(), right->name() );
        }
      }
    }
    if ( left_always_false || right_always_false )
    {
      if ( EXPRESSION_DEBUG )
      {
        printf( "Reduced %*d %s (%s, %s) and expression to false\n", spacing,
                id(), name(), left->name(), right->name() );
      }
      auto new_name = fmt::format("const_and_afalse('{}','{}')", left->name(), right->name());
      return std::make_unique<const_expr_t>( new_name, 0.0 );
    }
    if ( left_always_true && right_always_true )
    {
      if ( EXPRESSION_DEBUG )
      {
        printf( "Reduced %*d %s (%s, %s) and expression to true\n", spacing,
                id(), name(), left->name(), right->name() );
      }
      auto new_name = fmt::format( "const_and_atrue('{}','{}')", left->name(), right->name());
      return std::make_unique<const_expr_t>( new_name, 1.0 );
    }
    if ( left_always_true )
    {
      if (EXPRESSION_DEBUG)
      {
        printf("Reduced %*d %s (%s) and expression to right\n", spacing, id(),
          name(), left->name());
      }
      return std::make_unique<left_reduced_t<std::logical_and>>(
          fmt::format( "{}_left_reduced('{}')", name(), right->name() ),
          token_e::TOK_UNKNOWN, 1, std::move(right) );
    }
    if ( right_always_true )
    {
      if (EXPRESSION_DEBUG)
      {
        printf("Reduced %*d %s (%s) and expression to left\n", spacing, id(),
          name(), right->name());
      }
      return std::make_unique<right_reduced_t<std::logical_and>>(
          fmt::format( "{}_right_reduced('{}')", name(), left->name() ),
          token_e::TOK_UNKNOWN, std::move(left), 1 );
    }

    // We need to separate constant propagation and flattening for proper term sorting.
    if ( left_false < right_false )
    {
      auto& right_ = right->op_ == TOK_AND
                           ? debug_cast<binary_base_t*>( right.get() )->left
                           : right;
      std::swap( left, right_ );
    }
    else if ( left->op_ == TOK_AND )
    {
      std::swap( left, right );
      std::swap( left, debug_cast<binary_base_t*>( right.get() )->left );
    }
    return select_binary( analyze_further, name(), TOK_AND, std::move(left), std::move(right) );
  }
};

class analyze_logical_or_t : public analyze_binary_base_t
{
public:
  analyze_logical_or_t( util::string_view n, std::unique_ptr<expr_t> l, std::unique_ptr<expr_t> r )
    : analyze_binary_base_t( n, TOK_OR, std::move(l), std::move(r) )
  {
  }

  double evaluate() override  // override
  {
    left_result  = left->eval();
    right_result = right->eval();
    result       = ( left_result || right_result ) ? 1.0 : 0.0;
    analyze_boolean();
    return result;
  }

  std::unique_ptr<expr_t> build_optimized_expression( bool analyze_further, int spacing ) override
  {
    if ( EXPRESSION_DEBUG )
    {
      fmt::print( "{:>{}} and {} {} {} {} ( {} {} )\n", id(), spacing,
                    left_true, left_false, right_true, right_false,
                    left->name(), right->name() );
    }
    expr_t::optimize_expression(left, analyze_further, spacing + 2);
    expr_t::optimize_expression(right, analyze_further, spacing + 2);
    bool left_always_true   = left->always_true();
    bool left_always_false  = left->always_false();
    bool right_always_true  = right->always_true();
    bool right_always_false = right->always_false();
    if ( EXPRESSION_DEBUG )
    {
      if ( left->op_ == TOK_UNKNOWN )
      {
        if ( left_false + left_true >= EXPRESSION_CONSIDER_MARKING_THRESHOLD && ( ( !left_always_true && left_false == 0 ) ||
             ( !left_always_false && left_true == 0 ) ) )
        {
          printf( "consider marking expression %d %s as constant\n", left->id(),
                  left->name() );
        }
      }
      if ( right->op_ == TOK_UNKNOWN )
      {
        if ( left_false + left_true >= EXPRESSION_CONSIDER_MARKING_THRESHOLD && ( ( !right_always_true && right_false == 0 ) ||
             ( !right_always_false && right_true == 0 ) ) )
        {
          printf( "consider marking expression %d %s as constant\n",
                  right->id(), right->name() );
        }
      }
    }
    if ( left_always_true || right_always_true )
    {
      if (EXPRESSION_DEBUG)
      {
        printf("%*d %s or expression reduced to true\n", spacing, id(),
          name());
      }
      return std::make_unique<const_expr_t>( fmt::format("const_or_atrue('{}','{}')", left->name(), right->name()), 1.0 );
    }
    if ( left_always_false && right_always_false )
    {
      if (EXPRESSION_DEBUG)
      {
        printf("%*d %s or expression reduced to false\n", spacing, id(),
          name());
      }
      return std::make_unique<const_expr_t>( fmt::format("const_or_afalse('{}','{}')", left->name(), right->name()), 0.0 );
    }
    if ( left_always_false )
    {
      if (EXPRESSION_DEBUG)
      {
        printf("%*d %s or expression reduced to right\n", spacing, id(),
          name());
      }
      return std::make_unique<left_reduced_t<std::logical_or>>(
          fmt::format( "{}_left_reduced('{}')", name(), left->name() ),
          token_e::TOK_UNKNOWN, 0, std::move(right) );
    }
    if ( right_always_false )
    {
      if (EXPRESSION_DEBUG)
      {
        printf("%*d %s or expression reduced to left\n", spacing, id(),
          name());
      }
      return std::make_unique<right_reduced_t<std::logical_or>>(
          fmt::format( "{}_right_reduced('{}')", name(), left->name() ),
          token_e::TOK_UNKNOWN, std::move(left), 0 );
    }
    // We need to separate constant propagation and flattening for proper term
    // sorting.
    if ( left_true < right_true )
    {
      std::swap( left, right->op_ == TOK_OR
                           ? debug_cast<binary_base_t*>( right.get() )->left
                           : right );
    }
    else if ( left->op_ == TOK_OR )
    {
      std::swap( left, right );
      std::swap( left, debug_cast<binary_base_t*>( right.get() )->left );
    }
    
    return select_binary( analyze_further, name(), TOK_OR, std::move(left), std::move(right) );
  }
};

class analyze_logical_xor_t : public analyze_binary_base_t
{
public:
  analyze_logical_xor_t( util::string_view n, std::unique_ptr<expr_t> l, std::unique_ptr<expr_t> r )
    : analyze_binary_base_t( n, TOK_XOR, std::move(l), std::move(r) )
  {
  }

  double evaluate() override  // override
  {
    left_result  = left->eval();
    right_result = right->eval();
    result =
        ( bool( left_result != 0 ) != bool( right_result != 0 ) ) ? 1.0 : 0.0;
    analyze_boolean();
    return result;
  }

  std::unique_ptr<expr_t> build_optimized_expression( bool analyze_further, int spacing ) override
  {
    if (EXPRESSION_DEBUG)
    {
      printf("%*d xor ( %s %s )\n", spacing, id(), left->name(),
        right->name());
    }
    expr_t::optimize_expression(left, analyze_further, spacing + 2);
    expr_t::optimize_expression(right, analyze_further, spacing + 2);
    bool left_always_true   = left->always_true();
    bool left_always_false  = left->always_false();
    bool right_always_true  = right->always_true();
    bool right_always_false = right->always_false();
    if ( ( left_always_true && right_always_false ) ||
         ( left_always_false && right_always_true ) )
    {
      if (EXPRESSION_DEBUG)
      {
        printf("%*d %s xor expression reduced to true\n", spacing, id(), name());
      }
      return std::make_unique<const_expr_t>( fmt::format("const_xor_atrue('{}','{}')", left->name(), right->name()), 1.0 );
    }
    if ( ( left_always_true && right_always_true ) ||
         ( left_always_false && right_always_false ) )
    {
      if (EXPRESSION_DEBUG)
      {
        printf("%*d %s xor expression reduced to false\n", spacing, id(),
          name());
      }
      return std::make_unique<const_expr_t>( fmt::format("const_xor_afalse('{}','{}')", left->name(), right->name()), 0.0 );
    }
    if ( left_always_false )
    {
      if ( EXPRESSION_DEBUG )
        printf( "%*d %s xor expression reduced to right\n", spacing, id(), name() );
      return std::move(right);
    }
    if ( right_always_false )
    {
      if ( EXPRESSION_DEBUG )
        printf( "%*d %s xor expression reduced to left\n", spacing, id(),
                name() );
      return std::move(left);
    }
    if ( left_always_true )
    {
      if ( EXPRESSION_DEBUG )
        printf( "%*d %s xor expression reduced to !right\n", spacing, id(),
                name() );
      return select_unary( analyze_further, fmt::format("const_xor_left_atrue('{}','{}')", left->name(), right->name()), TOK_NOT, std::move(right) );
    }
    if ( right_always_true )
    {
      if ( EXPRESSION_DEBUG )
        printf( "%*d %s xor expression reduced to !left\n", spacing, id(),
                name() );
      return select_unary( analyze_further, fmt::format("const_xor_right_atrue('{}','{}')", left->name(), right->name()), TOK_NOT, std::move(left) );
    }
    return select_binary( analyze_further, name(), TOK_XOR, std::move(left), std::move(right) );
  }
};

template <template <typename> class F, typename T = double>
class expr_analyze_binary_t : public analyze_binary_base_t
{
public:
  expr_analyze_binary_t( util::string_view n, token_e o, std::unique_ptr<expr_t> l, std::unique_ptr<expr_t> r )
    : analyze_binary_base_t( n, o, std::move(l), std::move(r) )
  {
  }

  double evaluate() override  // override
  {
    result = static_cast<double>( F<T>()( static_cast<T>( left->eval() ), static_cast<T>( right->eval() ) ) );
    return result;
  }

  std::unique_ptr<expr_t> build_optimized_expression( bool analyze_further, int spacing ) override
  {
    if (EXPRESSION_DEBUG)
    {
      printf("%*d %s ( %s %s )\n", spacing, id(), name(), left->name(),
        right->name());
    }
    expr_t::optimize_expression(left, analyze_further, spacing + 2);
    expr_t::optimize_expression(right, analyze_further, spacing + 2);
    double left_value;
    double right_value;
    bool left_constant  = left->is_constant( &left_value );
    bool right_constant = right->is_constant( &right_value );
    if ( left_constant && right_constant )
    {
      result = static_cast<double>( F<T>()( static_cast<T>( left_value ), static_cast<T>( right_value ) ) );
      if (EXPRESSION_DEBUG)
      {
        printf("Reduced %*d %s (%s, %s) binary expression to %f\n", spacing,
          id(), name(), left->name(), right->name(), result);
      }
      return std::make_unique<const_expr_t>(fmt::format("const_binary('{}','{}')", left->name(), right->name()), result );
    }
    if ( left_constant )
    {
      if (EXPRESSION_DEBUG)
      {
        printf("Reduced %*d %s (%s) binary expression left\n", spacing, id(),
          name(), left->name());
      }
      
      return std::make_unique<left_reduced_t<F, T>>(
          fmt::format( "{}_left_reduced('{}')", name(), left->name() ),
          op_, left_value, std::move(right) );
    }
    if ( right_constant )
    {
      if ( EXPRESSION_DEBUG )
        printf( "Reduced %*d %s (%s) binary expression right\n", spacing, id(),
                name(), right->name() );
      
      return std::make_unique<right_reduced_t<F, T>>(
          fmt::format( "{}_right_reduced('{}')", name(), left->name() ),
          op_, std::move(left), right_value );
    }
    return select_binary( analyze_further, name(), op_, std::move(left), std::move(right) );
  }
};

std::unique_ptr<expr_t> select_analyze_binary( util::string_view name, token_e op,
                               std::unique_ptr<expr_t> left, std::unique_ptr<expr_t> right )
{
  switch ( op )
  {
    case TOK_AND:
      return std::make_unique<analyze_logical_and_t>( name, std::move(left), std::move(right) );
    case TOK_OR:
      return std::make_unique<analyze_logical_or_t>( name, std::move(left), std::move(right) );
    case TOK_XOR:
      return std::make_unique<analyze_logical_xor_t>( name, std::move(left), std::move(right) );

    case TOK_ADD:
      return std::make_unique<expr_analyze_binary_t<std::plus>>( name, op, std::move(left), std::move(right) );
    case TOK_SUB:
      return std::make_unique<expr_analyze_binary_t<std::minus>>( name, op, std::move(left), std::move(right) );
    case TOK_MULT:
      return std::make_unique<expr_analyze_binary_t<std::multiplies>>( name, op, std::move(left), std::move(right) );
    case TOK_DIV:
      return std::make_unique<expr_analyze_binary_t<std::divides>>( name, op, std::move(left), std::move(right) );
    case TOK_MOD:
      return std::make_unique<expr_analyze_binary_t<binary::modulus>>( name, op, std::move(left), std::move(right) );

    case TOK_MAX:
      return std::make_unique<expr_analyze_binary_t<binary::max>>( name, op, std::move(left), std::move(right) );
    case TOK_MIN:
      return std::make_unique<expr_analyze_binary_t<binary::min>>( name, op, std::move(left), std::move(right) );

    case TOK_EQ:
      return std::make_unique<expr_analyze_binary_t<std::equal_to>>( name, op, std::move(left), std::move(right) );
    case TOK_NOTEQ:
      return std::make_unique<expr_analyze_binary_t<std::not_equal_to>>( name, op, std::move(left), std::move(right) );
    case TOK_LT:
      return std::make_unique<expr_analyze_binary_t<std::less>>( name, op, std::move(left), std::move(right) );
    case TOK_LTEQ:
      return std::make_unique<expr_analyze_binary_t<std::less_equal>>( name, op, std::move(left), std::move(right) );
    case TOK_GT:
      return std::make_unique<expr_analyze_binary_t<std::greater>>( name, op, std::move(left), std::move(right) );
    case TOK_GTEQ:
      return std::make_unique<expr_analyze_binary_t<std::greater_equal>>( name, op, std::move(left), std::move(right) );

    default:
      assert( false );
      return {};  // throw?
  }
}

std::unique_ptr<expr_t> select_binary( bool analyze, util::string_view name, token_e op, std::unique_ptr<expr_t> left,
                                       std::unique_ptr<expr_t> right )
{
  if ( analyze )
  {
    return select_analyze_binary( name, op, std::move( left ), std::move( right ) );
  }
  return select_binary( name, op, std::move( left ), std::move( right ) );
}

int precedence( token_e expr_token_type )
{
  switch ( expr_token_type )
  {
    case TOK_FLOOR:
    case TOK_CEIL:
      return 9;

    case TOK_NOT:
    case TOK_PLUS:
    case TOK_MINUS:
    case TOK_ABS:
      return 8;

    case TOK_MULT:
    case TOK_DIV:
    case TOK_MOD:
      return 7;

    case TOK_ADD:
    case TOK_SUB:
      return 6;

    case TOK_MAX:
    case TOK_MIN:
      return 5;

    case TOK_EQ:
    case TOK_NOTEQ:
    case TOK_LT:
    case TOK_LTEQ:
    case TOK_GT:
    case TOK_GTEQ:
    case TOK_IN:
    case TOK_NOTIN:
      return 4;

    case TOK_AND:
      return 3;

    case TOK_XOR:
      return 2;

    case TOK_OR:
      return 1;

    default:
      assert( false );
      return 0;
  }
}

}  // UNNAMED NAMESPACE ====================================================

// is_unary =================================================================

bool is_unary( token_e expr_token_type )
{
  switch ( expr_token_type )
  {
    case TOK_NOT:
    case TOK_PLUS:
    case TOK_MINUS:
    case TOK_ABS:
    case TOK_FLOOR:
    case TOK_CEIL:
      return true;
    default:
      return false;
  }
}

// is_binary ================================================================

bool is_binary( token_e expr_token_type )
{
  switch ( expr_token_type )
  {
    case TOK_MULT:
    case TOK_DIV:
    case TOK_MOD:
    case TOK_ADD:
    case TOK_SUB:
    case TOK_MAX:
    case TOK_MIN:
    case TOK_EQ:
    case TOK_NOTEQ:
    case TOK_LT:
    case TOK_LTEQ:
    case TOK_GT:
    case TOK_GTEQ:
    case TOK_AND:
    case TOK_XOR:
    case TOK_OR:
    case TOK_IN:
    case TOK_NOTIN:
      return true;
    default:
      return false;
  }
}

// parse_tokens =============================================================

std::vector<expr_token_t> parse_tokens( action_t* action, util::string_view expr_str )
{
  std::vector<expr_token_t> tokens;
  lexer_t lexer( expr_str );

  token_e prev_token = TOK_UNKNOWN;
  lexer_t::token_t token = lexer.next();
  for (; token.type != TOK_EOF; token = lexer.next() )
  {
    if ( token.type == TOK_UNKNOWN )
    {
      if ( action )
      {
        action->sim->error( "{}-{}: Unexpected token ({}) in {}\n",
                            action->player->name(), action->name(), token.str, expr_str );
      }
      else
      {
        fmt::print( "Unexpected token ({}) in {}\n", token.str, expr_str );
      }
      break;
    }

    // lexer never produces binary SUB/ADD. convert unary MINUS/PLUS to them
    // when they can "bind" to the previous token as a binary op
    const bool prev_is_left_side = prev_token == TOK_STR || prev_token == TOK_NUM || prev_token == TOK_RPAR;
    if ( token.type == TOK_MINUS && prev_is_left_side )
      token.type = TOK_SUB;
    if ( token.type == TOK_PLUS && prev_is_left_side )
      token.type = TOK_ADD;

    // lexer never produces FLOOR/CEIL "pseudo-functions", match them here
    if ( token.type == TOK_LPAR && prev_token == TOK_STR )
    {
      if ( util::str_compare_ci( tokens.back().label, "floor" ) )
        tokens.back().type = TOK_FLOOR;
      else if ( util::str_compare_ci( tokens.back().label, "ceil" ) )
        tokens.back().type = TOK_CEIL;
    }

    // optimization: fold minus followed by a number together into a single number token
    if ( token.type == TOK_NUM && prev_token == TOK_MINUS )
    {
      tokens.back().type = TOK_NUM;
      tokens.back().label.append( token.str.data(), token.str.size() );
      prev_token = TOK_NUM;
      continue;
    }

    tokens.push_back( { token.type, std::string( token.str ) } );
    prev_token = token.type;
  }

  return tokens;
}

// print_tokens =============================================================

void print_tokens( util::span<const expr_token_t> tokens, sim_t* sim )
{
  if ( tokens.empty() )
    return;

  std::vector<std::string> strings;
  for ( const expr_token_t& token : tokens )
    strings.push_back( fmt::format("{:2d} '{}'", token.type, token.label ) );

  sim->out_debug.print( "{}", fmt::join( strings, " | " ) );
}

// convert_to_rpn ===========================================================

bool convert_to_rpn( std::vector<expr_token_t>& tokens )
{
  std::vector<expr_token_t> rpn;
  std::vector<expr_token_t> stack;

  size_t num_tokens = tokens.size();
  for ( size_t i = 0; i < num_tokens; i++ )
  {
    expr_token_t& t = tokens[ i ];

    if ( t.type == TOK_NUM || t.type == TOK_STR )
    {
      rpn.push_back( t );
    }
    else if ( t.type == TOK_LPAR )
    {
      stack.push_back( t );
    }
    else if ( t.type == TOK_RPAR )
    {
      while ( true )
      {
        if ( stack.empty() )
        {
          return false;
        }
        expr_token_t& s = stack.back();
        if ( s.type == TOK_LPAR )
        {
          stack.pop_back();
          break;
        }
        else
        {
          rpn.push_back( s );
          stack.pop_back();
        }
      }
    }
    else  // operator
    {
      while ( true )
      {
        if ( stack.empty() )
          break;
        expr_token_t& s = stack.back();
        if ( s.type == TOK_LPAR )
          break;
        if ( precedence( t.type ) > precedence( s.type ) )
          break;
        rpn.push_back( s );
        stack.pop_back();
      }
      stack.push_back( t );
    }
  }

  while ( !stack.empty() )
  {
    expr_token_t& s = stack.back();
    if ( s.type == TOK_LPAR )
    {
      return false;
    }
    rpn.push_back( s );
    stack.pop_back();
  }

  tokens.swap( rpn );

  return true;
}

std::unique_ptr<expr_t> build_player_expression_tree(
    player_t& player, std::vector<expression::expr_token_t>& tokens,
    bool optimize )
{
  std::vector<std::unique_ptr<expr_t>> stack;

  for ( auto& t : tokens )
  {
    if ( t.type == expression::TOK_NUM )
    {
      stack.emplace_back( std::make_unique<const_expr_t>( t.label, std::stod( t.label ) ) );
    }
    else if ( t.type == expression::TOK_STR )
    {
      auto e = player.create_expression( t.label );
      if ( !e )
      {
        std::throw_with_nested(std::runtime_error("No expression found."));
      }
      stack.push_back( std::move(e) );
    }
    else if ( expression::is_unary( t.type ) )
    {
      if ( stack.empty() )
        return nullptr;
      auto input = std::move(stack.back());
      stack.pop_back();
      assert( input );
      auto expr = expression::select_unary( optimize, t.label, t.type, std::move(input) );
      stack.push_back( std::move(expr) );
    }
    else if ( expression::is_binary( t.type ) )
    {
      if ( stack.size() < 2 )
        return nullptr;
      auto right = std::move(stack.back());
      stack.pop_back();
      assert( right );
      auto left = std::move(stack.back());
      stack.pop_back();
      assert( left );
      auto expr = expression::select_binary( optimize, t.label, t.type, std::move(left), std::move(right) );
      stack.push_back( std::move(expr) );
    }
  }

  if ( stack.size() != 1 )
    return nullptr;

  auto res = std::move(stack.back());
  stack.pop_back();
  return res;
}

}  // expression

#if !defined( NDEBUG )
int expr_t::get_global_id()
{
  static std::atomic<int> unique_id;
  return ++unique_id;
}
#endif

// build_expression_tree ====================================================

static std::unique_ptr<expr_t> build_expression_tree(
    action_t* action, util::span<expression::expr_token_t> tokens,
    bool optimize )
{
  std::vector<std::unique_ptr<expr_t>> stack;

  for ( auto& t : tokens )
  {
    if ( t.type == expression::TOK_NUM )
    {
      stack.push_back( std::make_unique<const_expr_t>( t.label, std::stod( t.label ) ) );
    }
    else if ( t.type == expression::TOK_STR )
    {
      auto e = action->create_expression( t.label );
      if ( !e )
      {
        throw std::invalid_argument("No expression found.");
      }
      stack.push_back( std::move(e) );
    }
    else if ( expression::is_unary( t.type ) )
    {
      if ( stack.empty() )
        return {};
      auto input = std::move(stack.back());
      stack.pop_back();
      assert( input );
      auto expr = expression::select_unary( optimize, t.label, t.type, std::move(input) );
      stack.push_back( std::move(expr) );
    }
    else if ( expression::is_binary( t.type ) )
    {
      if ( stack.size() < 2 )
        return nullptr;
      auto right = std::move(stack.back());
      stack.pop_back();
      assert( right );
      auto left = std::move(stack.back());
      stack.pop_back();
      assert( left );
      auto expr = expression::select_binary( optimize, t.label, t.type, std::move(left), std::move(right) );
      stack.push_back( std::move(expr) );
    }
  }

  if ( stack.size() != 1 )
    return nullptr;

  auto res = std::move(stack.back());
  stack.pop_back();
  return res;
}

void expr_t::optimize_expression( std::unique_ptr<expr_t>& expression, bool analyze_further, int spacing )
{
  if ( !expression )
  {
    return;
  }
  if ( auto optimized = expression->build_optimized_expression( analyze_further, spacing ) )
  {
    expression.swap( optimized );
  }
}

void expr_t::optimize_expression( std::unique_ptr<expr_t>& expression, sim_t& sim )
{
  auto iterations = sim.current_iteration;
  if ( iterations < 0 )
  {
    return;
  }
  if ( sim.optimize_expressions - 1 - iterations < 0 )
  {
    return;
  }
  bool analyze_further = sim.optimize_expressions - 1 - iterations  > 0;
  
  for(int i = 0; i < sim.optimize_expressions_rounds; ++i)
  {
    optimize_expression( expression, analyze_further );
  }
}

// action_expr_t::create_constant ===========================================

// action_expr_t::parse =====================================================

std::unique_ptr<expr_t> expr_t::parse( action_t* action, util::string_view expr_str,
                       bool optimize )
{
  try
  {
    if ( expr_str.empty() )
      return nullptr;

    std::vector<expression::expr_token_t> tokens =
        expression::parse_tokens( action, expr_str );

    if ( action->sim->debug )
      expression::print_tokens( tokens, action->sim );

    if ( action->sim->debug )
      expression::print_tokens( tokens, action->sim );

    if ( !expression::convert_to_rpn( tokens ) )
    {
      throw std::invalid_argument( fmt::format( "Unable to convert '{}' into RPN.", expr_str ) );
    }

    if ( action->sim->debug )
      expression::print_tokens( tokens, action->sim );

    if ( auto e = build_expression_tree( action, tokens, optimize ) )
      return e;

    throw std::invalid_argument("Unable to build expression tree.");
  }
  catch (const std::exception& )
  {
    std::throw_with_nested(std::runtime_error(fmt::format("Cannot parse expression from '{}'",
        expr_str)));
  }
}

target_wrapper_expr_t::target_wrapper_expr_t(action_t& a, util::string_view name_str, util::string_view expr_str) :
  expr_t(name_str), action(a), suffix_expr_str(expr_str)
{
  std::generate_n(std::back_inserter(proxy_expr), action.sim->actor_list.size(), [] { return std::unique_ptr<expr_t>(); });
}

double target_wrapper_expr_t::evaluate()
{
  assert(target());

  size_t actor_index = target()->actor_index;

  if (proxy_expr[actor_index] == nullptr)
  {
    proxy_expr[actor_index] = target()->create_expression(suffix_expr_str);
  }

  return proxy_expr[actor_index]->eval();
}

player_t* target_wrapper_expr_t::target() const
{
  return action.target;
}

#ifdef UNIT_TEST

uint32_t dbc::get_school_mask( school_e )
{
  return 0;
}

namespace
{
expr_t* parse_expression( const char* arg )
{
  std::vector<expr_token_t> tokens = expression_t::parse_tokens( 0, arg );
  expression_t::print_tokens( tokens, 0 );

  if ( expression_t::convert_to_rpn( tokens ) )
  {
    puts( "rpn:" );
    expression_t::print_tokens( tokens, 0 );

    return build_expression_tree( 0, tokens, false );
  }

  return 0;
}

void time_test( expr_t* expr, uint64_t n )
{
  double value        = 0;
  const int64_t start = util::milliseconds();
  for ( uint64_t i = 0; i < n; ++i )
    value            = expr->eval();
  const int64_t stop = util::milliseconds();
  printf( "evaluate: %f in %.4f seconds\n", value, ( stop - start ) / 1000.0 );
}
}

void sim_t::cancel()
{
}

int sim_t::errorf( const char* format, ... )
{
  va_list ap;
  va_start( ap, format );
  int result = vfprintf( stderr, format, ap );
  va_end( ap );
  return result;
}

void sim_t::output( sim_t*, const char* format, ... )
{
  va_list ap;
  va_start( ap, format );
  vfprintf( stdout, format, ap );
  va_end( ap );
}

int main( int argc, char** argv )
{
  uint64_t n_evals = 1;

  for ( int i = 1; i < argc; i++ )
  {
    if ( util::str_compare_ci( argv[ i ], "-n" ) )
    {
      ++i;
      assert( i < argc );
      std::istringstream is( argv[ i ] );
      is >> n_evals;
      assert( n_evals > 0 );
      continue;
    }

    expr_t* expr = parse_expression( argv[ i ] );
    if ( expr )
    {
      if ( n_evals == 1 )
      {
        puts( "evaluate:" );
        printf( "%f\n", expr->eval() );
      }
      else
        time_test( expr, n_evals );
    }
  }

  return 0;
}

#endif
