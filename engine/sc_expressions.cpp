// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

struct token_t
{
  int type;
  std::string label;
};

// Unary Operators ===========================================================

struct expr_unary_t : public action_expr_t
{
  int operation;
  action_expr_t* input;
  expr_unary_t( action_t* a, const std::string& n, int o, action_expr_t* i ) : action_expr_t(a,n), operation(o), input(i) {}
  virtual int evaluate()
  {
    result_type = TOK_UNKNOWN;
    int input_result = input -> evaluate();
    if ( input_result == TOK_NUM )
    {
      result_type = TOK_NUM;
      switch( operation )
      {
      case TOK_PLUS:  result_num =   input -> result_num; break;
      case TOK_MINUS: result_num = - input -> result_num; break;
      case TOK_NOT:   result_num = ( input -> result_num != 0 ) ? 0 : 1; break;
      }
    }
    else
    {
      log_t::output( action -> sim, "%s-%s: Unexpected input type (%s) for unary operator '%s'\n", 
		     action -> player -> name(), action -> name(), input -> name(), name() );
    }
    return result_type;
  }
};

// Binary Operators ==========================================================

struct expr_binary_t : public action_expr_t
{
  int operation;
  action_expr_t* left;
  action_expr_t* right;
  expr_binary_t( action_t* a, const std::string& n, int o, action_expr_t* l, action_expr_t* r ) : action_expr_t(a,n), operation(o), left(l), right(r) {}
  virtual int evaluate()
  {
    result_type = TOK_UNKNOWN;
    int  left_result =  left -> evaluate();
    int right_result = right -> evaluate();
    if ( left_result != right_result )
    {
      log_t::output( action -> sim, "%s-%s: Inconsistent input types (%s and %s) for binary operator '%s'\n", 
		     action -> player -> name(), action -> name(), left -> name(), right -> name(), name() );
    }
    else if ( left_result == TOK_NUM )
    {
      result_type = TOK_NUM;
      switch( operation )
      {
      case TOK_ADD:  result_num = left -> result_num + right -> result_num; break;
      case TOK_SUB:  result_num = left -> result_num - right -> result_num; break;
      case TOK_MULT: result_num = left -> result_num * right -> result_num; break;
      case TOK_DIV:  result_num = left -> result_num / right -> result_num; break;

      case TOK_EQ:    result_num = ( left -> result_num == right -> result_num ) ? 1 : 0; break;
      case TOK_NOTEQ: result_num = ( left -> result_num != right -> result_num ) ? 1 : 0; break;
      case TOK_LT:    result_num = ( left -> result_num <  right -> result_num ) ? 1 : 0; break;
      case TOK_LTEQ:  result_num = ( left -> result_num <= right -> result_num ) ? 1 : 0; break;
      case TOK_GT:    result_num = ( left -> result_num >  right -> result_num ) ? 1 : 0; break;
      case TOK_GTEQ:  result_num = ( left -> result_num >= right -> result_num ) ? 1 : 0; break;

      case TOK_AND: result_num = ( ( left -> result_num != 0 ) && ( right -> result_num != 0 ) ) ? 1 : 0; break;
      case TOK_OR:  result_num = ( ( left -> result_num != 0 ) || ( right -> result_num != 0 ) ) ? 1 : 0; break;

      default: assert(0);
      }
    }
    else if ( left_result == TOK_STR )
    {
      result_type = TOK_NUM;
      switch( operation )
      {
      case TOK_EQ:    result_num = ( left -> result_str == right -> result_str ) ? 1 : 0; break;
      case TOK_NOTEQ: result_num = ( left -> result_str != right -> result_str ) ? 1 : 0; break;
      case TOK_LT:    result_num = ( left -> result_str <  right -> result_str ) ? 1 : 0; break;
      case TOK_LTEQ:  result_num = ( left -> result_str <= right -> result_str ) ? 1 : 0; break;
      case TOK_GT:    result_num = ( left -> result_str >  right -> result_str ) ? 1 : 0; break;
      case TOK_GTEQ:  result_num = ( left -> result_str >= right -> result_str ) ? 1 : 0; break;

      default: assert(0);
      }
    }
    return result_type;
  }
};

// precedence ================================================================

static int precedence( int token_type )
{
  switch( token_type )
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
    return 2;

  case TOK_AND:
  case TOK_OR:
    return 1;
  }
  assert(0);

  return 0;
}

// is_unary ==================================================================

static int is_unary( int token_type )
{
  switch( token_type )
  {
  case TOK_NOT:
  case TOK_PLUS:
  case TOK_MINUS:
    return true;
  }
  return false;
}

// is_binary =================================================================

static int is_binary( int token_type )
{
  switch( token_type )
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
    return true;
  }
  return false;
}

// next_token ================================================================

static int next_token( action_t* action, const std::string& expr_str, int& current_index, std::string& token_str )
{
  char c = expr_str[ current_index++ ];

  if ( c == '\0' ) return TOK_UNKNOWN;

  token_str = c;

  if ( c == '+' ) return TOK_ADD;
  if ( c == '-' ) return TOK_SUB;
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
  if ( c == '!' ) 
  {
    if ( expr_str[ current_index ] == '=' ) { current_index++; return TOK_NOTEQ; }
    return TOK_NOT;
  }
  if ( c == '=' ) 
  {
    if ( expr_str[ current_index ] == '=' ) current_index++;
    return TOK_EQ;
  }
  if ( c == '<' )
  {
    if ( expr_str[ current_index ] == '=' ) { current_index++; return TOK_LTEQ; }
    return TOK_LT;
  }
  if ( c == '>' )
  {
    if ( expr_str[ current_index ] == '=' ) { current_index++; return TOK_GTEQ; }
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

  if( isdigit( c ) )
  {
    c = expr_str[ current_index ];
    while( isdigit( c ) )
    {
      token_str += c;
      c = expr_str[ ++current_index ];
    }
    return TOK_NUM;
  }

  log_t::output( action -> sim, "%s-%s: Unexpected token (%c) in %s\n", action -> player -> name(), action -> name(), c, expr_str.c_str() );

  return TOK_UNKNOWN;
}

// parse_tokens ==============================================================

static void parse_tokens( action_t* action,
			  std::vector<token_t>& tokens, 
			  const std::string& expr_str )
{
  token_t token;
  int current_index=0;
  
  while( ( token.type = next_token( action, expr_str, current_index, token.label ) ) != TOK_UNKNOWN )
  {
    tokens.push_back( token );
  }
}

// print_tokens ==============================================================

static void print_tokens( std::vector<token_t>& tokens )
{
  printf( "tokens:\n" );
  int num_tokens = tokens.size();
  for ( int i=0; i < num_tokens; i++ )
  {
    token_t& t = tokens[ i ];
    printf( "%2d  '%s'\n", t.type, t.label.c_str() );
  }
}

// convert_to_unary ==========================================================

static void convert_to_unary( action_t* action, std::vector<token_t>& tokens )
{
  int num_tokens = tokens.size();
  for ( int i=0; i < num_tokens; i++ )
  {
    token_t& t = tokens[ i ];

    bool left_side = false;

    if ( i > 0 )
      if ( tokens[ i-1 ].type == TOK_NUM ||
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

static bool convert_to_rpn( action_t* action, std::vector<token_t>& tokens )
{
  std::vector<token_t> rpn, stack;

  int num_tokens = tokens.size();
  for ( int i=0; i < num_tokens; i++ )
  {
    token_t& t = tokens[ i ];

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
	if ( stack.empty() ) return false;
	token_t& s = stack.back();
	if ( s.type == TOK_LPAR ) break;
	rpn.push_back( s );
	stack.pop_back();
      }
    }
    else // operator
    {
      while ( true )
      {
	if ( stack.empty() ) break;
	token_t& s = stack.back();
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
    token_t& s = stack.back();
    if ( s.type == TOK_LPAR ) return false;
    rpn.push_back( s );
    stack.pop_back();
  }

  tokens.swap( rpn );

  return true;
}

// build_expression_tree =====================================================

static action_expr_t* build_expression_tree( action_t* action,
					     std::vector<token_t>& tokens )
{
  std::vector<action_expr_t*> stack;

  int num_tokens = tokens.size();
  for( int i=0; i < num_tokens; i++ )
  {
    token_t& t= tokens[ i ];
    
    if ( t.type == TOK_NUM ) 
    {
      stack.push_back( new action_expr_t( action, t.label, atof( t.label.c_str() ) ) );
    }
    else if ( t.type == TOK_STR ) 
    {
      action_expr_t* e = action -> create_expression( t.label );
      if ( ! e ) 
      {
	log_t::output( action -> sim, 
		       "Player %s action %s : Unable to decode expression function '%s'\n", 
		       action -> player -> name(), action -> name(), t.label.c_str() );
	action -> sim -> canceled = 1;
	e = new action_expr_t( action, t.label, t.label );
      }
      stack.push_back( e );
    }
    else if ( is_unary( t.type ) ) 
    {
      if ( stack.size() < 1 ) return 0;
      action_expr_t* input = stack.back(); stack.pop_back();
      stack.push_back( new expr_unary_t( action, t.label, t.type, input ) );
    }
    else if ( is_binary( t.type ) )
    {
      if ( stack.size() < 2 ) return 0;
      action_expr_t* right = stack.back(); stack.pop_back();
      action_expr_t* left  = stack.back(); stack.pop_back();
      stack.push_back( new expr_binary_t( action, t.label, t.type, left, right ) );
    }
  }

  if ( stack.size() != 1 ) return 0;

  return stack.back();
}

// action_expr_t::parse ======================================================

action_expr_t* action_expr_t::parse( action_t* action,
				     const std::string& expr_str )
{
  if ( expr_str.empty() ) return 0;

  std::vector<token_t> tokens;

  parse_tokens( action, tokens, expr_str );

  convert_to_unary( action, tokens );

  if ( action -> sim -> debug ) print_tokens( tokens );

  if( ! convert_to_rpn( action, tokens ) ) 
  {
    log_t::output( action -> sim, "%s-%s: Unable to convert %s into RPN\n", action -> player -> name(), action -> name(), expr_str.c_str() );
    return 0;
  }

  action_expr_t* e = build_expression_tree( action, tokens );

  if ( ! e ) log_t::output( action -> sim, "%s-%s: Unable to build expression tree from %s\n", action -> player -> name(), action -> name(), expr_str.c_str() );

  return e;
}

#ifdef UNIT_TEST

int main( int argc, char** argv )
{
  for( int i=1; i < argc; i++ )
  {
    std::vector<token_t> tokens;

    parse_tokens( tokens, argv[ i ] );
    convert_to_unary( tokens );
    print_tokens( tokens );

    if( convert_to_rpn( tokens ) )
    {
      printf( "rpn:\n" );
      num_tokens = tokens.size();
      for ( int i=0; i < num_tokens; i++ )
      {
	token_t& t = tokens[ i ];
	printf( "%2d  '%s'\n", t.type, t.label.c_str() );
      }

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
