// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// Unary Operators ===========================================================

struct unary_minus
{
  inline static double evaluate(double input) { return -input; }
};

struct unary_plus
{
  inline static double evaluate(double input) { return input; }
};

struct unary_not
{
  inline static double evaluate(double input) { return input ? 0 : 1; }
};

template <typename Op>
struct expr_unary_t : public action_expr_t
{
  action_expr_t* input;
  expr_unary_t( action_t* a, const std::string& n, action_expr_t* i ) : action_expr_t(a, n, TOK_NUM), input(i) {}
  virtual int evaluate()
  {
    input -> evaluate();
    result_num = Op::evaluate(input -> result_num);
    return TOK_NUM;
  }
};


// Binary Operators ==========================================================

struct binary_add
{
  inline static double evaluate(double left, double right) { return left + right; }
};

struct binary_sub
{
  inline static double evaluate(double left, double right) { return left - right; }
};

struct binary_mult
{
  inline static double evaluate(double left, double right) { return left * right; }
};

struct binary_div
{
  inline static double evaluate(double left, double right) { return left / right; }
};

struct binary_eq
{
  inline static double evaluate(double left, double right) { return left == right ? 1 : 0; }
};

struct binary_noteq
{
  inline static double evaluate(double left, double right) { return left != right ? 1 : 0; }
};

struct binary_lt
{
  inline static double evaluate(double left, double right) { return left < right ? 1 : 0; }
};

struct binary_lteq
{
  inline static double evaluate(double left, double right) { return left <= right ? 1 : 0; }
};

struct binary_gt
{
  inline static double evaluate(double left, double right) { return left > right ? 1 : 0; }
};

struct binary_gteq
{
  inline static double evaluate(double left, double right) { return left >= right ? 1 : 0; }
};

struct binary_and
{
  inline static double evaluate(double left, double right) { return (left != 0 && right != 0) ? 1 : 0; }
};

struct binary_or
{
  inline static double evaluate(double left, double right) { return (left != 0 || right != 0 )? 1 : 0; }
};

template <typename Op>
struct expr_num_binary_t : public action_expr_t
{
  action_expr_t* left;
  action_expr_t* right;
  expr_num_binary_t( action_t* a, const std::string& n, action_expr_t* l, action_expr_t* r ) : action_expr_t(a,n,TOK_NUM), left(l), right(r) {}
  virtual int evaluate()
  {
    left -> evaluate();
    right -> evaluate();
    result_num = Op::evaluate(left -> result_num, right -> result_num);
    return TOK_NUM;
  }
};

// precedence ================================================================

int expression_t::precedence( int expr_token_type )
{
  switch( expr_token_type )
  {
  case TOK_NOT:
  case TOK_PLUS:
  case TOK_MINUS:
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
  }
  assert(0);

  return 0;
}

// is_unary ==================================================================

int expression_t::is_unary( int expr_token_type )
{
  switch( expr_token_type )
  {
  case TOK_NOT:
  case TOK_PLUS:
  case TOK_MINUS:
    return true;
  }
  return false;
}

// is_binary =================================================================

int expression_t::is_binary( int expr_token_type )
{
  switch( expr_token_type )
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
  }
  return false;
}

// next_token ================================================================

int expression_t::next_token( action_t* action, const std::string& expr_str, int& current_index, std::string& token_str )
{
  char c = expr_str[ current_index++ ];

  if ( c == '\0' ) return TOK_UNKNOWN;

  token_str = c;

  if ( c == '+' ) return TOK_ADD;
  if ( c == '-' && ! isdigit( expr_str[ current_index ] ) ) return TOK_SUB;
  if ( c == '*' ) return TOK_MULT;
  if ( c == '/' ) return TOK_DIV;
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
    while( isalpha( c ) || isdigit( c ) || c == '_' || c == '.' )
    {
      token_str += c;
      c = expr_str[ ++current_index ];
    }
    return TOK_STR;
  }

  if( isdigit( c ) || c == '-' )
  {
    c = expr_str[ current_index ];
    while( isdigit( c ) || c == '.' )
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

// parse_tokens ==============================================================

void expression_t::parse_tokens( action_t* action,
			  std::vector<expr_token_t>& tokens, 
			  const std::string& expr_str )
{
  expr_token_t token;
  int current_index=0;
  
  while( ( token.type = next_token( action, expr_str, current_index, token.label ) ) != TOK_UNKNOWN )
  {
    tokens.push_back( token );
  }
}

// print_tokens ==============================================================

void expression_t::print_tokens( std::vector<expr_token_t>& tokens )
{
  printf( "tokens:\n" );
  int num_tokens = tokens.size();
  for ( int i=0; i < num_tokens; i++ )
  {
    expr_token_t& t = tokens[ i ];
    printf( "%2d  '%s'\n", t.type, t.label.c_str() );
  }
}

// convert_to_unary ==========================================================

void expression_t::convert_to_unary( action_t* action, std::vector<expr_token_t>& tokens )
{
  int num_tokens = tokens.size();
  for ( int i=0; i < num_tokens; i++ )
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

// convert_to_rpn ============================================================

bool expression_t::convert_to_rpn( action_t* action, std::vector<expr_token_t>& tokens )
{
  std::vector<expr_token_t> rpn, stack;

  int num_tokens = tokens.size();
  for ( int i=0; i < num_tokens; i++ )
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
	if ( precedence( t.type ) >= precedence( s.type ) ) break;
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

// build_expression_tree =====================================================

static action_expr_t* build_expression_tree( action_t* action,
                                             const std::vector<expr_token_t>& tokens )
{
  std::vector<action_expr_t*> stack;
  action_expr_t* res = 0;

  int num_tokens = tokens.size();
  for( int i=0; i < num_tokens; i++ )
  {
    const expr_token_t& t= tokens.at(i);
    
    if ( t.type == TOK_NUM )
    {
      stack.push_back( new action_expr_t( action, t.label, atof( t.label.c_str() ) ) );
    }
    else if ( t.type == TOK_STR ) 
    {
      action_expr_t* e = action -> create_expression( t.label );
      if ( ! e ) 
      {
        action -> sim -> errorf( "Player %s action %s : Unable to decode expression function '%s'\n", 
        action -> player -> name(), action -> name(), t.label.c_str() );
        goto exit_label;
      }
      stack.push_back( e );
    }
    else if ( expression_t::is_unary( t.type ) ) 
    {
      if ( stack.size() < 1 ) 
        goto exit_label;
      action_expr_t* input = stack.back(); stack.pop_back();
      if ( ! input ) 
        goto exit_label;
      action_expr_t* e = 0;
      switch (t.type)
      {
      case TOK_PLUS:  e = input; break;
      case TOK_MINUS: e = new expr_unary_t<unary_minus>( action, t.label, input ); break;
      case TOK_NOT:   e = new expr_unary_t<unary_not>  ( action, t.label, input ); break;
      }
      stack.push_back(e);
    }
    else if ( expression_t::is_binary( t.type ) )
    {
      if ( stack.size() < 2 )
        goto exit_label;
      action_expr_t* right = stack.back(); stack.pop_back();
      action_expr_t* left  = stack.back(); stack.pop_back();
      if ( ! left || ! right ) 
        goto exit_label;
      action_expr_t* e = 0;
      switch (t.type)
      {
      case TOK_ADD:   e = new expr_num_binary_t<binary_add>  ( action, t.label, left, right ); break;
      case TOK_SUB:   e = new expr_num_binary_t<binary_sub>  ( action, t.label, left, right ); break;
      case TOK_MULT:  e = new expr_num_binary_t<binary_mult> ( action, t.label, left, right ); break;
      case TOK_DIV:   e = new expr_num_binary_t<binary_div>  ( action, t.label, left, right ); break;

      case TOK_EQ:    e = new expr_num_binary_t<binary_eq   >( action, t.label, left, right ); break;
      case TOK_NOTEQ: e = new expr_num_binary_t<binary_noteq>( action, t.label, left, right ); break;
      case TOK_LT:    e = new expr_num_binary_t<binary_lt   >( action, t.label, left, right ); break;
      case TOK_LTEQ:  e = new expr_num_binary_t<binary_lteq >( action, t.label, left, right ); break;
      case TOK_GT:    e = new expr_num_binary_t<binary_gt   >( action, t.label, left, right ); break;
      case TOK_GTEQ:  e = new expr_num_binary_t<binary_gteq >( action, t.label, left, right ); break;

      case TOK_AND:   e = new expr_num_binary_t<binary_and>  ( action, t.label, left, right ); break;
      case TOK_OR:    e = new expr_num_binary_t<binary_or >  ( action, t.label, left, right ); break;

      default:
        abort();
      }
      stack.push_back(e);
    }
  }

  if ( stack.size() != 1 ) 
  {
    goto exit_label;
  }

  res = stack.back();
  stack.pop_back();
exit_label:
  while ( stack.size() )
  {
    action_expr_t* s = stack.back();
    stack.pop_back();
    delete s;
  }
  return res;
}

// action_expr_t::parse ======================================================

action_expr_t* action_expr_t::parse( action_t* action,
				     const std::string& expr_str )
{
  if ( expr_str.empty() ) return 0;

  std::vector<expr_token_t> tokens;

  expression_t::parse_tokens( action, tokens, expr_str );

  if ( action -> sim -> debug ) expression_t::print_tokens( tokens );

  expression_t::convert_to_unary( action, tokens );

  if ( action -> sim -> debug ) expression_t::print_tokens( tokens );

  if( ! expression_t::convert_to_rpn( action, tokens ) ) 
  {
    action -> sim -> errorf( "%s-%s: Unable to convert %s into RPN\n", action -> player -> name(), action -> name(), expr_str.c_str() );
    action -> sim -> cancel();
    return 0;
  }

  if ( action -> sim -> debug ) expression_t::print_tokens( tokens );

  action_expr_t* e = build_expression_tree( action, tokens );

  if ( ! e ) 
  {
    action -> sim -> errorf( "%s-%s: Unable to build expression tree from %s\n", action -> player -> name(), action -> name(), expr_str.c_str() );
    action -> sim -> cancel();
    return 0;
  }

  return e;
}

#ifdef UNIT_TEST

int main( int argc, char** argv )
{
  for( int i=1; i < argc; i++ )
  {
    std::vector<expr_token_t> tokens;

    parse_tokens( NULL, tokens, argv[ i ] );
    convert_to_unary( NULL, tokens );
    print_tokens( tokens );

    if( convert_to_rpn( NULL, tokens ) )
    {
      printf( "rpn:\n" );
      print_tokens( tokens );

      action_expr_t* expr = build_expression_tree( 0, tokens );

      if( expr ) 
      {
	printf( "evaluate:\n" );
	std::string buffer;
	expr -> evaluate();
	if ( expr -> result_type == TOK_NUM )
	{
	  printf( "%f\n", expr -> result_num );
	}
	else if ( expr -> result_type == TOK_STR )
	{
	  printf( "%s\n", expr -> result_str.c_str() );
	}
	else
	{
	  printf( "unknown\n" );
	}
      }
    }
  }

  return 0;
}

#endif
