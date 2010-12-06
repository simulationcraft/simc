// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

struct js_node_t
{
  std::string name_str;
  std::string value;
  std::vector<js_node_t*> children;
  js_node_t() {}
  js_node_t( const std::string& n ) : name_str( n ) {}
  const char* name() { return name_str.c_str(); }
};

namespace   // ANONYMOUS NAMESPACE =========================================
{

// is_white_space ==========================================================

static bool is_white_space( char c )
{
  return( c == ' '  ||
          c == '\t' ||
          c == '\n' ||
          c == '\r' );
}

// is_token_char ===========================================================

static bool is_token_char( char c )
{
  if ( isalpha( c ) ) return true;
  if ( isdigit( c ) ) return true;
  if ( c == '-' ) return true;
  if ( c == '+' ) return true;
  if ( c == '_' ) return true;
  if ( c == '.' ) return true;
  if ( c == '$' ) return true;
  return false;
}

// parse_token =============================================================

static char parse_token( std::string&            token_str,
                         const std::string&      input,
                         std::string::size_type& index )
{
  token_str = "";

  while ( is_white_space( input[ index ] ) ) index++;

  char c = input[ index++ ];

  if ( c == '{' || c == '}' ||
       c == '[' || c == ']' ||
       c == '(' || c == ')' ||
       c == ',' || c == ':' )
  {
    return c;
  }

  if ( c == '\'' || c == '\"' )
  {
    char start = c;
    while ( true )
    {
      c = input[ index++ ];
      if ( c == '\0' ) return c;
      if ( c == start ) break;
      if ( c == '\\' )
      {
        c = input[ index++ ];
        switch ( c )
        {
        case 'n': c = '\n'; break;
        case 'r': c = '\r'; break;
        case 't': c = '\t'; break;
        case '\0': return c;
        }
      }
      token_str += c;
    }
    return 'S';
  }
  else if ( is_token_char( c ) )
  {
    token_str += c;
    while ( is_token_char( input[ index ] ) ) token_str += input[ index++ ];
    if ( token_str == "new" ) return 'N';
    return 'S';
  }

  return '\0';
}

// parse_value =============================================================

static void parse_value( sim_t*                  sim,
			 js_node_t*              node,
			 char                    token_type,
			 std::string&            token_str,
                         const std::string&      input,
                         std::string::size_type& index )
{
  if ( token_type == '{' )
  {
    while ( ! sim -> canceled )
    {
      token_type = parse_token( token_str, input, index );
      if ( token_type == '}' ) break;
      if ( token_type != 'S' )
      {
        sim -> errorf( "Unexpected token '%c' (%s) at index %d (%s)\n", token_type, token_str.c_str(), ( int ) index, node -> name() );
        sim -> cancel();
	return;
      }
      js_node_t* child = new js_node_t( token_str );
      node -> children.push_back( child );

      token_type = parse_token( token_str, input, index );
      if ( token_type != ':' )
      {
        sim -> errorf( "Unexpected token '%c' at index %d (%s)\n", token_type, ( int ) index, node -> name() );
        sim -> cancel();
	return;
      }

      token_type = parse_token( token_str, input, index );
      parse_value( sim, child, token_type, token_str, input, index );

      token_type = parse_token( token_str, input, index );
      if ( token_type == ',' ) continue;
      if ( token_type == '}' ) break;

      sim -> errorf( "Unexpected token '%c' at index %d (%s)\n", token_type, ( int ) index, node -> name() );
      sim -> cancel();
      return;
    }
  }
  else if ( token_type == '[' )
  {
    int  array_index=-1;

    while ( ! sim -> canceled )
    {
      token_type = parse_token( token_str, input, index );
      if ( token_type == ']' ) break;

      char buffer[ 64 ];
      snprintf( buffer, sizeof( buffer ), "%d", ++array_index );

      js_node_t* child = new js_node_t( buffer );
      node -> children.push_back( child );

      parse_value( sim, child, token_type, token_str, input, index );

      token_type = parse_token( token_str, input, index );
      if ( token_type == ',' ) continue;
      if ( token_type == ']' ) break;

      sim -> errorf( "Unexpected token '%c' at index %d (%s)\n", token_type, ( int ) index, node -> name() );
      sim -> cancel();
      return;
    }
  }
  else if ( token_type == 'N' )
  {
    token_type = parse_token( token_str, input, index );
    if ( token_type != 'S' )
    {
      sim -> errorf( "Unexpected token '%c' at index %d (%s)\n", token_type, ( int ) index, node -> name() );
      sim -> cancel();
      return;
    }
    node -> value = token_str;

    token_type = parse_token( token_str, input, index );
    if ( token_type != '(' )
    {
      sim -> errorf( "Unexpected token '%c' at index %d (%s)\n", token_type, ( int ) index, node -> name() );
      sim -> cancel();
      return;
    }

    std::string::size_type start = index;
    token_type = parse_token( token_str, input, index );
    if ( token_type == ')' ) return;
    index = start;

    while ( ! sim -> canceled )
    {
      js_node_t* child = new js_node_t( node -> value );
      node -> children.push_back( child );

      token_type = parse_token( token_str, input, index );
      parse_value( sim, child, token_type, token_str, input, index );

      token_type = parse_token( token_str, input, index );
      if ( token_type == ',' ) continue;
      if ( token_type == ')' ) break;

      sim -> errorf( "Unexpected token '%c' at index %d (%s)\n", token_type, ( int ) index, node -> name() );
      sim -> cancel();
      return;
    }
  }
  else if ( token_type == 'S' )
  {
    node -> value = token_str;
  }
  else
  {
    sim -> errorf( "Unexpected token '%c' at index %d (%s)\n", token_type, ( int ) index, node -> name() );
    sim -> cancel();
  }
}

// search_tree =============================================================

static js_node_t* search_tree( js_node_t*         root,
                               const std::string& name_str,
			       int                depth )
{
  if ( name_str.empty() || name_str.size() == 0 || name_str == root -> name() )
    return root;

  if ( depth == 0 ) return 0;

  int num_children = ( int ) root -> children.size();

  for ( int i=0; i < num_children; i++ )
  {
    js_node_t* node = search_tree( root -> children[ i ], name_str, 0 );
    if ( node ) return node;
  }

  for ( int i=0; i < num_children; i++ )
  {
    js_node_t* node = search_tree( root -> children[ i ], name_str, depth-1 );
    if ( node ) return node;
  }

  return 0;
}

// split_path ==============================================================

static js_node_t* split_path( js_node_t*         node,
                              const std::string& path )
{
  std::vector<std::string> splits;
  int num_splits = util_t::string_split( splits, path, "/" );

  for ( int i=0; i < num_splits; i++ )
  {
    node = search_tree( node, splits[ i ], ( i ? +1 : -1 ) );
    if ( ! node ) return 0;
  }

  return node;
}

} // ANONYMOUS NAMESPACE ===================================================

// js_t::create ===========================================================

js_node_t* js_t::create( sim_t* sim, const std::string& input )
{
  js_node_t* root = new js_node_t( "root" );

  std::string::size_type index=0;

  // Only parsing "values" not "statements" nor "expressions".

  std::string token_str;
  char token_type = parse_token( token_str, input, index );

  parse_value( sim, root, token_type, token_str, input, index );

  return root;
}

// js_t::create ===========================================================

js_node_t* js_t::create( sim_t* sim, FILE* input )
{
  if ( ! input ) return 0;
  std::string buffer;
  char c;
  while ( ( c = fgetc( input ) ) != EOF ) buffer += c;
  return create( sim, buffer );
}

// js_t::get_child ========================================================

js_node_t* js_t::get_child( js_node_t*         root,
                            const std::string& name_str )
{
  int num_children = ( int ) root -> children.size();
  for ( int i=0; i < num_children; i++ )
  {
    js_node_t* node = root -> children[ i ];
    if ( name_str == node -> name() ) return node;
  }

  return 0;
}

// js_t::get_children ======================================================

int js_t::get_children( std::vector<js_node_t*>& nodes,
                        js_node_t*               root,
                        const std::string&       name_str )
{
  int num_children = ( int ) root -> children.size();
  for ( int i=0; i < num_children; i++ )
  {
    js_node_t* node = root -> children[ i ];
    if ( name_str == node -> name() ) nodes.push_back( node );
  }

  return ( int ) nodes.size();
}

// js_t::get_node =========================================================

js_node_t* js_t::get_node( js_node_t*         root,
                           const std::string& path )
{
  if ( path.empty() || path.size() == 0 || path == root -> name() )
    return root;

  return split_path( root, path );
}

// js_t::get_nodes ========================================================

int js_t::get_nodes( std::vector<js_node_t*>& nodes,
                     js_node_t*               root,
                     const std::string&       path )
{
  if ( path.empty() || path.size() == 0 || path == root -> name() )
  {
    nodes.push_back( root );
  }
  else
  {
    js_node_t* node = root;
    std::string name_str = path;

    if ( path.find( "/" ) != std::string::npos )
    {
      std::vector<std::string> splits;
      int num_splits = util_t::string_split( splits, path, "/" );

      for ( int i=0; i < num_splits-1; i++ )
      {
        node = search_tree( node, splits[ i ], ( i ? +1 : -1 ) );
        if ( ! node ) return 0;
      }
      name_str = splits[ num_splits-1 ];
    }

    int num_children = ( int ) node -> children.size();
    for ( int i=0; i < num_children; i++ )
    {
      get_nodes( nodes, node -> children[ i ], name_str );
    }
  }

  return ( int ) nodes.size();
}

// js_t::get_value ========================================================

bool js_t::get_value( std::string&       value,
                      js_node_t*         root,
                      const std::string& path )
{
  js_node_t* node = split_path( root, path );
  if ( ! node ) return false;
  if ( node -> value.empty() ) return false;
  value = node -> value;
  return true;
}

// js_t::get_value ========================================================

bool js_t::get_value( int&               value,
                      js_node_t*         root,
                      const std::string& path )
{
  js_node_t* node = split_path( root, path );
  if ( ! node ) return false;
  if ( node -> value.empty() ) return false;
  value = atoi( node -> value.c_str() );
  return true;
}

// js_t::get_value ========================================================

bool js_t::get_value( double&            value,
                      js_node_t*         root,
                      const std::string& path )
{
  js_node_t* node = split_path( root, path );
  if ( ! node ) return false;
  if ( node -> value.empty() ) return false;
  value = atof( node -> value.c_str() );
  return true;
}

// js_t::get_value ========================================================

int js_t::get_value( std::vector<std::string>& value,
                     js_node_t*               root,
                     const std::string&       path )
{
  js_node_t* node = split_path( root, path );
  if ( ! node ) return 0;
  if ( node -> children.empty() ) return 0;
  int size = node -> children.size();
  value.resize( size );
  for( int i=0; i < size; i++ )
  {
    value[ i ] = node -> children[ i ] -> value;
  }
  return size;
}

// js_t::print ============================================================

void js_t::print( js_node_t* root,
                  FILE*      file,
                  int        spacing )
{
  if ( ! root ) return;

  if ( ! file ) file = stdout;

  util_t::fprintf( file, "%*s%s", spacing, "", root -> name() );

  if ( ! root -> value.empty() )
  {
    util_t::fprintf( file, " : '%s'", root -> value.c_str() );
  }
  util_t::fprintf( file, "\n" );

  int num_children = ( int ) root -> children.size();
  for ( int i=0; i < num_children; i++ )
  {
    print( root -> children[ i ], file, spacing+2 );
  }
}

