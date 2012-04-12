// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // ANONYMOUS ====================================================

class const_expr_t : public expr_t
{
  const double value;

  double evaluate() // override
  { return value; }

public:
  const_expr_t( const std::string& name, double value_ ) :
    expr_t( name ), value( value_ ) {}
};

// Unary Operators ==========================================================

struct expr_unary_t : public expr_t
{
  expr_t* input;
  token_type_e operation;

  expr_unary_t( const std::string& n, token_type_e o, expr_t* i ) :
    expr_t( n ), input( i ), operation( o ) { assert( input ); }
  ~expr_unary_t() { delete input; }

  virtual double evaluate()
  {
    double val = input -> eval();
    switch ( operation )
    {
    case TOK_PLUS:  return  val;
    case TOK_MINUS: return -val;
    case TOK_NOT:   return val == 0;
    case TOK_ABS:   return std::fabs( val );
    case TOK_FLOOR: return std::floor( val );
    case TOK_CEIL:  return std::ceil( val );
    default: assert( false ); return 0; // throw?
    }
  }
};

// Binary Operators =========================================================

struct expr_binary_t : public expr_t
{
  expr_t* left;
  expr_t* right;
  token_type_e operation;

  expr_binary_t( const std::string& n, token_type_e o, expr_t* l, expr_t* r ) :
    expr_t( n ), left( l ), right( r ), operation( o )
  { assert( left ); assert( right ); }
  ~expr_binary_t() { delete left; delete right; }

  virtual double evaluate()
  {
    double l = left -> eval();

    switch( operation )
    {
    case TOK_AND: return l && right -> eval();
    case TOK_OR:  return l || right -> eval();
    default: break;
    }

    double r = right -> eval();

    switch ( operation )
    {
    case TOK_ADD:   return l + r;
    case TOK_SUB:   return l - r;
    case TOK_MULT:  return l * r;
    case TOK_DIV:   return l / r;

    case TOK_EQ:    return l == r;
    case TOK_NOTEQ: return l != r;
    case TOK_LT:    return l <  r;
    case TOK_LTEQ:  return l <= r;
    case TOK_GT:    return l >  r;
    case TOK_GTEQ:  return l >= r;

    default: assert( false ); return 0; // throw?
    }
  }
};

} // ANONYMOUS NAMESPACE ====================================================

// precedence ===============================================================

int expression_t::precedence( token_type_e expr_token_type )
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
    return 1;

  default:
    assert( false );
    return 0;
  }
}

// is_unary =================================================================

bool expression_t::is_unary( token_type_e expr_token_type )
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

bool expression_t::is_binary( token_type_e expr_token_type )
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
  case TOK_OR:
  case TOK_IN:
  case TOK_NOTIN:
    return true;
  default:
    return false;
  }
}

// next_token ===============================================================

token_type_e expression_t::next_token( action_t* action, const std::string& expr_str,
                                       int& current_index, std::string& token_str, token_type_e prev_token )
{
  unsigned char c = expr_str[ current_index++ ];

  if ( c == '\0' ) return TOK_UNKNOWN;
  if ( ( prev_token == TOK_FLOOR || prev_token == TOK_CEIL ) && c != '(' )
    return TOK_UNKNOWN;

  token_str = c;

  if ( c == '@' ) return TOK_ABS;
  if ( c == '+' ) return TOK_ADD;
  if ( c == '-' && ( prev_token == TOK_STR || prev_token == TOK_NUM ) ) return TOK_SUB;
  if ( c == '-' && prev_token != TOK_STR && prev_token != TOK_NUM && ! isdigit( expr_str[ current_index ] ) ) return TOK_SUB;
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

    if ( util_t::str_compare_ci( token_str, "floor" ) )
      return TOK_FLOOR;
    else if ( util_t::str_compare_ci( token_str, "ceil" ) )
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
  int current_index=0;
  token_type_e t = TOK_UNKNOWN;

  while ( ( token.type = next_token( action, expr_str, current_index, token.label, t ) ) != TOK_UNKNOWN )
  {
    t = token.type;
    tokens.push_back( token );
  }

  return tokens;
}

// print_tokens =============================================================

void expression_t::print_tokens( const std::vector<expr_token_t>& tokens, sim_t* sim )
{
  log_t::output( sim, "tokens:\n" );
  size_t num_tokens = tokens.size();
  for ( size_t i=0; i < num_tokens; i++ )
  {
    const expr_token_t& t = tokens[ i ];
    log_t::output( sim,  "%2d  '%s'\n", t.type, t.label.c_str() );
  }
}

// convert_to_unary =========================================================

void expression_t::convert_to_unary( std::vector<expr_token_t>& tokens )
{
  size_t num_tokens = tokens.size();
  for ( size_t i=0; i < num_tokens; i++ )
  {
    expr_token_t& t = tokens[ i ];

    bool left_side = false;

    if ( i > 0 )
      if ( tokens[ i-1 ].type == TOK_NUM ||
           tokens[ i-1 ].type == TOK_STR ||
           tokens[ i-1 ].type == TOK_RPAR )
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
  for ( size_t i=0; i < num_tokens; i++ )
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
  for ( size_t i=0; i < num_tokens; i++ )
  {
    expr_token_t& t= tokens[ i ];

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
      stack.push_back( new expr_unary_t( t.label, t.type, input ) );
    }
    else if ( expression_t::is_binary( t.type ) )
    {
      if ( stack.size() < 2 )
        return 0;
      expr_t* right = stack.back(); stack.pop_back();
      assert( right );
      expr_t* left  = stack.back(); stack.pop_back();
      assert( left );
      stack.push_back( new expr_binary_t( t.label, t.type, left, right ) );
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

int main( int argc, char** argv )
{
  for ( int i=1; i < argc; i++ )
  {
    std::vector<expr_token_t> tokens = parse_tokens( 0, argv[ i ] );
    convert_to_unary( tokens );
    print_tokens( tokens );

    if ( convert_to_rpn( tokens ) )
    {
      printf( "rpn:\n" );
      print_tokens( tokens );

      expr_t* expr = build_expression_tree( 0, tokens );

      if ( expr )
      {
        printf( "evaluate:\n" );
        std::string buffer;
        printf( "%f\n", expr -> eval() );
      }
    }
  }

  return 0;
}

#endif
