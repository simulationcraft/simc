// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // ANONYMOUS ====================================================

class const_expr_t : public expr_t
{
  double value;

public:
  const_expr_t( const std::string& name, double value_ ) :
    expr_t( name ), value( value_ ) {}

  double evaluate() // override
  { return value; }
};

// Unary Operators ==========================================================

template <double ( *F )( double )>
class expr_unary_t : public expr_t
{
  expr_t* input;

public:
  expr_unary_t( const std::string& n, expr_t* i ) :
    expr_t( n ), input( i )
  { assert( input ); }

  ~expr_unary_t() { delete input; }

  double evaluate() // override
  { return F( input -> eval() ); }
};

namespace unary {
inline double plus ( double val ) { return val; }
inline double minus( double val ) { return -val; }
inline double lnot ( double val ) { return val == 0; }
inline double abs  ( double val ) { return std::fabs( val ); }
inline double floor( double val ) { return std::floor( val ); }
inline double ceil ( double val ) { return std::ceil( val ); }
}

expr_t* select_unary( const std::string& name, token_e op, expr_t* input )
{
  switch ( op )
  {
    case TOK_PLUS:  return new expr_unary_t<unary::plus> ( name, input );
    case TOK_MINUS: return new expr_unary_t<unary::minus>( name, input );
    case TOK_NOT:   return new expr_unary_t<unary::lnot> ( name, input );
    case TOK_ABS:   return new expr_unary_t<unary::abs>  ( name, input );
    case TOK_FLOOR: return new expr_unary_t<unary::floor>( name, input );
    case TOK_CEIL:  return new expr_unary_t<unary::ceil> ( name, input );

    default: assert( false ); return 0; // throw?
  }
}

// Binary Operators =========================================================

class binary_base_t : public expr_t
{
protected:
  expr_t* left;
  expr_t* right;

public:
  binary_base_t( const std::string& n, expr_t* l, expr_t* r ) :
    expr_t( n ), left( l ), right( r )
  {
    assert( left );
    assert( right );
  }

  ~binary_base_t() { delete left; delete right; }
};

class logical_and_t : public binary_base_t
{
public:
  logical_and_t( const std::string& n, expr_t* l, expr_t* r ) :
    binary_base_t( n, l, r )
  {}

  double evaluate() // override
  { return left -> eval() && right -> eval(); }
};

class logical_or_t : public binary_base_t
{
public:
  logical_or_t( const std::string& n, expr_t* l, expr_t* r ) :
    binary_base_t( n, l, r )
  {}

  double evaluate() // override
  { return left -> eval() || right -> eval(); }
};

class logical_xor_t : public binary_base_t
{
public:
  logical_xor_t( const std::string& n, expr_t* l, expr_t* r ) :
    binary_base_t( n, l, r )
  {}

  double evaluate() // override
  { return bool( left -> eval() ) != bool( right -> eval() ); }
};

template <template<typename> class F>
class expr_binary_t : public binary_base_t
{
public:
  expr_binary_t( const std::string& n, expr_t* l, expr_t* r ) :
    binary_base_t( n, l, r )
  {}

  double evaluate() // override
  { return F<double>()( left -> eval(), right -> eval() ); }
};

expr_t* select_binary( const std::string& name, token_e op, expr_t* left, expr_t* right )
{
  switch ( op )
  {
    case TOK_AND:   return new logical_and_t                     ( name, left, right );
    case TOK_OR:    return new logical_or_t                      ( name, left, right );
    case TOK_XOR:   return new logical_xor_t                     ( name, left, right );

    case TOK_ADD:   return new expr_binary_t<std::plus>          ( name, left, right );
    case TOK_SUB:   return new expr_binary_t<std::minus>         ( name, left, right );
    case TOK_MULT:  return new expr_binary_t<std::multiplies>    ( name, left, right );
    case TOK_DIV:   return new expr_binary_t<std::divides>       ( name, left, right );

    case TOK_EQ:    return new expr_binary_t<std::equal_to>      ( name, left, right );
    case TOK_NOTEQ: return new expr_binary_t<std::not_equal_to>  ( name, left, right );
    case TOK_LT:    return new expr_binary_t<std::less>          ( name, left, right );
    case TOK_LTEQ:  return new expr_binary_t<std::less_equal>    ( name, left, right );
    case TOK_GT:    return new expr_binary_t<std::greater>       ( name, left, right );
    case TOK_GTEQ:  return new expr_binary_t<std::greater_equal> ( name, left, right );

    default: assert( false ); return 0; // throw?
  }
}

} // UNNAMED NAMESPACE ====================================================

// precedence ===============================================================

int expression_t::precedence( token_e expr_token_type )
{
  switch ( expr_token_type )
  {
    case TOK_FLOOR:
    case TOK_CEIL:
      return 6;

    case TOK_NOT:
    case TOK_PLUS:
    case TOK_MINUS:
    case TOK_ABS:
      return 5;

    case TOK_MULT:
    case TOK_DIV:
      return 4;

    case TOK_ADD:
    case TOK_SUB:
      return 3;

    case TOK_EQ:
    case TOK_NOTEQ:
    case TOK_LT:
    case TOK_LTEQ:
    case TOK_GT:
    case TOK_GTEQ:
    case TOK_IN:
    case TOK_NOTIN:
      return 2;

    case TOK_AND:
    case TOK_OR:
    case TOK_XOR:
      return 1;

    default:
      assert( false );
      return 0;
  }
}

// is_unary =================================================================

bool expression_t::is_unary( token_e expr_token_type )
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

bool expression_t::is_binary( token_e expr_token_type )
{
  switch ( expr_token_type )
  {
    case TOK_MULT:
    case TOK_DIV:
    case TOK_ADD:
    case TOK_SUB:
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

// next_token ===============================================================

token_e expression_t::next_token( action_t* action, const std::string& expr_str,
                                  int& current_index, std::string& token_str, token_e prev_token )
{
  unsigned char c = expr_str[ current_index++ ];

  if ( c == '\0' ) return TOK_UNKNOWN;
  if ( ( prev_token == TOK_FLOOR || prev_token == TOK_CEIL ) && c != '(' )
    return TOK_UNKNOWN;

  token_str = c;

  if ( c == '@' ) return TOK_ABS;
  if ( c == '+' ) return TOK_ADD;
  if ( c == '-' && ( prev_token == TOK_STR || prev_token == TOK_NUM ) ) return TOK_SUB;
  if ( c == '-' && prev_token != TOK_STR && prev_token != TOK_NUM && ! isdigit( static_cast<unsigned char>( expr_str[ current_index ] ) ) ) return TOK_SUB;
  if ( c == '*' ) return TOK_MULT;
  if ( c == '%' ) return TOK_DIV;
  if ( c == '&' )
  {
    if ( expr_str[ current_index ] == '&' ) current_index++;
    return TOK_AND;
  }
  if ( c == '|' )
  {
    if ( expr_str[ current_index ] == '|' ) current_index++;
    return TOK_OR;
  }
  if ( c == '^' )
  {
    if ( expr_str[ current_index ] == '^' ) current_index++;
    return TOK_XOR;
  }
  if ( c == '~' )
  {
    if ( expr_str[ current_index ] == '~' ) current_index++;
    return TOK_IN;
  }
  if ( c == '!' )
  {
    if ( expr_str[ current_index ] == '=' ) { current_index++; token_str += "="; return TOK_NOTEQ; }
    if ( expr_str[ current_index ] == '~' ) { current_index++; token_str += "~"; return TOK_NOTIN; }
    return TOK_NOT;
  }
  if ( c == '=' )
  {
    if ( expr_str[ current_index ] == '=' ) current_index++;
    return TOK_EQ;
  }
  if ( c == '<' )
  {
    if ( expr_str[ current_index ] == '=' ) { current_index++; token_str += "="; return TOK_LTEQ; }
    return TOK_LT;
  }
  if ( c == '>' )
  {
    if ( expr_str[ current_index ] == '=' ) { current_index++; token_str += "="; return TOK_GTEQ; }
    return TOK_GT;
  }
  if ( c == '(' ) return TOK_LPAR;
  if ( c == ')' ) return TOK_RPAR;

  if ( isalpha( c ) )
  {
    c = expr_str[ current_index ];
    while ( isalpha( c ) || isdigit( c ) || c == '_' || c == '.' )
    {
      token_str += c;
      c = expr_str[ ++current_index ];
    }

    if ( util::str_compare_ci( token_str, "floor" ) )
      return TOK_FLOOR;
    else if ( util::str_compare_ci( token_str, "ceil" ) )
      return TOK_CEIL;
    else
      return TOK_STR;
  }

  if ( isdigit( c ) || c == '-' )
  {
    c = expr_str[ current_index ];
    while ( isdigit( c ) || c == '.' )
    {
      token_str += c;
      c = expr_str[ ++current_index ];
    }

    return TOK_NUM;
  }

  if ( action )
  {
    action -> sim -> errorf( "%s-%s: Unexpected token (%c) in %s\n", action -> player -> name(), action -> name(), c, expr_str.c_str() );
  }
  else
  {
    printf( "Unexpected token (%c) in %s\n", c, expr_str.c_str() );
  }

  return TOK_UNKNOWN;
}

// parse_tokens =============================================================

std::vector<expr_token_t>
expression_t::parse_tokens( action_t* action,
                            const std::string& expr_str )
{
  std::vector<expr_token_t> tokens;

  expr_token_t token;
  int current_index = 0;
  token_e t = TOK_UNKNOWN;

  while ( ( token.type = next_token( action, expr_str, current_index, token.label, t ) ) != TOK_UNKNOWN )
  {
    t = token.type;
    tokens.push_back( token );
  }

  return tokens;
}

// print_tokens =============================================================

void expression_t::print_tokens( std::vector<expr_token_t>& tokens, sim_t* sim )
{
  std::string str;
  if ( tokens.size() > 0 )
    str += "tokens: ";

  char token_str_buf[512];
  for ( size_t i = 0; i < tokens.size(); i++ )
  {
    expr_token_t& t = tokens[ i ];

    snprintf( token_str_buf, sizeof( token_str_buf ), "%2d '%s'", t.type, t.label.c_str() );
    str += token_str_buf;

    if ( i < tokens.size() - 1 )
      str += " | ";
  }

  sim -> out_debug << str;
}

// convert_to_unary =========================================================

void expression_t::convert_to_unary( std::vector<expr_token_t>& tokens )
{
  size_t num_tokens = tokens.size();
  for ( size_t i = 0; i < num_tokens; i++ )
  {
    expr_token_t& t = tokens[ i ];

    bool left_side = false;

    if ( i > 0 )
      if ( tokens[ i - 1 ].type == TOK_NUM ||
           tokens[ i - 1 ].type == TOK_STR ||
           tokens[ i - 1 ].type == TOK_RPAR )
        left_side = true;

    if ( ! left_side && ( t.type == TOK_ADD || t.type == TOK_SUB ) )
    {
      // Must be unary.
      t.type = ( t.type == TOK_ADD ) ? TOK_PLUS : TOK_MINUS;
    }
  }
}

// convert_to_rpn ===========================================================

bool expression_t::convert_to_rpn( std::vector<expr_token_t>& tokens )
{
  std::vector<expr_token_t> rpn, stack;

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
        if ( stack.empty() ) { printf( "rpar stack empty\n" ); return false; }
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
    else // operator
    {
      while ( true )
      {
        if ( stack.empty() ) break;
        expr_token_t& s = stack.back();
        if ( s.type == TOK_LPAR ) break;
        if ( precedence( t.type ) > precedence( s.type ) ) break;
        rpn.push_back( s );
        stack.pop_back();
      }
      stack.push_back( t );
    }
  }

  while ( ! stack.empty() )
  {
    expr_token_t& s = stack.back();
    if ( s.type == TOK_LPAR ) { printf( "stack lpar\n" ); return false; }
    rpn.push_back( s );
    stack.pop_back();
  }

  tokens.swap( rpn );

  return true;
}

// build_expression_tree ====================================================

static expr_t* build_expression_tree( action_t* action,
                                      std::vector<expr_token_t>& tokens )
{
  auto_dispose< std::vector<expr_t*> > stack;

  size_t num_tokens = tokens.size();
  for ( size_t i = 0; i < num_tokens; i++ )
  {
    expr_token_t& t = tokens[ i ];

    if ( t.type == TOK_NUM )
    {
      stack.push_back( new const_expr_t( t.label, atof( t.label.c_str() ) ) );
    }
    else if ( t.type == TOK_STR )
    {
      expr_t* e = action -> create_expression( t.label );
      if ( ! e )
      {
        action -> sim -> errorf( "Player %s action %s : Unable to decode expression function '%s'\n",
                                 action -> player -> name(), action -> name(), t.label.c_str() );
        return 0;
      }
      stack.push_back( e );
    }
    else if ( expression_t::is_unary( t.type ) )
    {
      if ( stack.size() < 1 )
        return 0;
      expr_t* input = stack.back(); stack.pop_back();
      assert( input );
      stack.push_back( select_unary( t.label, t.type, input ) );
    }
    else if ( expression_t::is_binary( t.type ) )
    {
      if ( stack.size() < 2 )
        return 0;
      expr_t* right = stack.back(); stack.pop_back();
      assert( right );
      expr_t* left  = stack.back(); stack.pop_back();
      assert( left );
      stack.push_back( select_binary( t.label, t.type, left, right ) );
    }
  }

  if ( stack.size() != 1 )
    return 0;

  expr_t* res = stack.back();
  stack.pop_back();
  return res;
}

// action_expr_t::parse =====================================================

expr_t* expr_t::parse( action_t* action,
                       const std::string& expr_str )
{
  if ( expr_str.empty() ) return 0;

  std::vector<expr_token_t> tokens = expression_t::parse_tokens( action, expr_str );

  if ( action -> sim -> debug ) expression_t::print_tokens( tokens, action -> sim );

  expression_t::convert_to_unary( tokens );

  if ( action -> sim -> debug ) expression_t::print_tokens( tokens, action -> sim );

  if ( ! expression_t::convert_to_rpn( tokens ) )
  {
    action -> sim -> errorf( "%s-%s: Unable to convert %s into RPN\n", action -> player -> name(), action -> name(), expr_str.c_str() );
    action -> sim -> cancel();
    return 0;
  }

  if ( action -> sim -> debug ) expression_t::print_tokens( tokens, action -> sim );

  if ( expr_t* e = build_expression_tree( action, tokens ) )
    return e;

  action -> sim -> errorf( "%s-%s: Unable to build expression tree from %s\n", action -> player -> name(), action -> name(), expr_str.c_str() );
  action -> sim -> cancel();
  return 0;
}

#ifdef UNIT_TEST

uint32_t dbc::get_school_mask( school_e ) { return 0; }

namespace {
expr_t* parse_expression( const char* arg )
{
  std::vector<expr_token_t> tokens = expression_t::parse_tokens( 0, arg );
  expression_t::convert_to_unary( tokens );
  expression_t::print_tokens( tokens, 0 );

  if ( expression_t::convert_to_rpn( tokens ) )
  {
    puts( "rpn:" );
    expression_t::print_tokens( tokens, 0 );

    return build_expression_tree( 0, tokens );
  }

  return 0;
}

void time_test( expr_t* expr, uint64_t n )
{
  double value = 0;
  const int64_t start = util::milliseconds();
  for ( uint64_t i = 0; i < n ; ++i )
    value = expr -> eval();
  const int64_t stop = util::milliseconds();
  printf( "evaluate: %f in %.4f seconds\n", value, ( stop - start ) / 1000.0 );
}
}

void sim_t::cancel() {}

int sim_t::errorf( const char *format, ... )
{
  va_list ap;
  va_start( ap, format );
  int result = vfprintf( stderr, format, ap );
  va_end( ap );
  return result;
}

void sim_t::output( sim_t*, const char *format, ... )
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
        printf( "%f\n", expr -> eval() );
      }
      else
        time_test( expr, n_evals );
    }
  }

  return 0;
}

#endif
