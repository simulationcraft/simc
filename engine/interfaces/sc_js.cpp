// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

struct js_node_t
{
  std::string name_str;
  std::string value;
  std::vector<js_node_t*> children;
  js_node_t() {}
  js_node_t( const std::string& n ) : name_str( n ) {}
  const char* name() { return name_str.c_str(); }
};

// is_white_space ===========================================================

static bool is_white_space( char c )
{
  return( c == ' '  ||
          c == '\t' ||
          c == '\n' ||
          c == '\r' );
}

// is_token_char ============================================================

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

// parse_token ==============================================================

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
       c == ',' || c == ':' ||
       c == '=' )
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

// parse_value ==============================================================

static void parse_value( sim_t*                  sim,
                         js_node_t*              node,
                         char                    token_type,
                         std::string&            token_str,
                         const std::string&      input,
                         std::string::size_type& index )
{
  if ( token_type == 0 )
    return;

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

      // Apparently Blizzard does not know what kind of pets Hunters have in their game
      if ( token_type == ',' ) continue;

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
    int  array_index = -1;

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

    std::string::size_type start = index;
    token_type = parse_token( token_str, input, index );

    if ( token_type == '=' )
    {
      token_type = parse_token( token_str, input, index );
      js_node_t* child = new js_node_t( node -> value );
      node -> children.push_back( child );
      parse_value( sim, child, token_type, token_str, input, index );
    }
    else
      index = start;
  }
  else
  {
    sim -> errorf( "Unexpected token '%c' at index %d (%s)\n", token_type, ( int ) index, node -> name() );
    sim -> cancel();
  }
}

// split_path ===============================================================

static js_node_t* split_path( js_node_t*         node,
                              const std::string& path )
{
  std::vector<std::string> splits = util::string_split( path, "/" );

  for ( size_t i = 0; i < splits.size(); i++ )
  {
    node = js::get_child( node, splits[ i ] );
    if ( ! node ) return 0;
  }

  return node;
}

// js::create ===============================================================

js_node_t* js::create( sim_t* sim, const std::string& input )
{
  if ( input.empty() ) return 0;

  js_node_t* root = new js_node_t( "root" );

  std::string::size_type index = 0;

  std::string token_str;
  char token_type = parse_token( token_str, input, index );

  parse_value( sim, root, token_type, token_str, input, index );

  return root;
}

// js::create ===============================================================

js_node_t* js::create( sim_t* sim, FILE* input )
{
  if ( ! input ) return 0;
  std::string buffer;
  char c;
  while ( ( c = fgetc( input ) ) != EOF ) buffer += c;
  return create( sim, buffer );
}

void js::delete_node( js_node_t* root )
{
  delete root;
}

// js::get_child ============================================================

js_node_t* js::get_child( js_node_t*         root,
                          const std::string& name_str )
{
  int num_children = ( int ) root -> children.size();
  for ( int i = 0; i < num_children; i++ )
  {
    js_node_t* node = root -> children[ i ];
    if ( name_str == node -> name() ) return node;
  }

  return 0;
}

// js::get_children =========================================================

int js::get_children( std::vector<js_node_t*>& nodes,
                      js_node_t*               root )
{
  nodes = root -> children;
  return ( int ) nodes.size();
}

// js::get_node =============================================================

js_node_t* js::get_node( js_node_t*         root,
                         const std::string& path )
{
  if ( path.empty() || path == root -> name() )
    return root;

  return split_path( root, path );
}

// js::get_value ============================================================

bool js::get_value( std::string&       value,
                    js_node_t*         root,
                    const std::string& path )
{
  js_node_t* node = split_path( root, path );
  if ( ! node ) return false;
  if ( node -> value.empty() ) return false;
  value = node -> value;
  return true;
}

// js::get_value ============================================================

bool js::get_value( int&               value,
                    js_node_t*         root,
                    const std::string& path )
{
  js_node_t* node = split_path( root, path );
  if ( ! node ) return false;
  if ( node -> value.empty() ) return false;
  value = atoi( node -> value.c_str() );
  return true;
}

// js::get_value ============================================================

bool js::get_value( unsigned&          value,
                    js_node_t*         root,
                    const std::string& path )
{
  js_node_t* node = split_path( root, path );
  if ( ! node ) return false;
  if ( node -> value.empty() ) return false;
  value = strtoul( node -> value.c_str(), 0, 10 );
  return true;
}

// js::get_value ============================================================

bool js::get_value( double&            value,
                    js_node_t*         root,
                    const std::string& path )
{
  js_node_t* node = split_path( root, path );
  if ( ! node ) return false;
  if ( node -> value.empty() ) return false;
  value = atof( node -> value.c_str() );
  return true;
}

// js::get_value ============================================================

int js::get_value( std::vector<std::string>& value,
                   js_node_t*               root,
                   const std::string&       path )
{
  js_node_t* node = split_path( root, path );
  if ( ! node ) return 0;
  if ( node -> children.empty() ) return 0;
  size_t size = node -> children.size();
  value.resize( size );
  for ( size_t i = 0; i < size; i++ )
  {
    value[ i ] = node -> children[ i ] -> value;
  }
  return static_cast<int>( size );
}

// js::get_name =============================================================

const char* js::get_name( js_node_t* node )
{
  if ( node -> name_str.empty() )
    return 0;

  return node -> name();
}

// js::print ================================================================

void js::print( js_node_t* root,
                FILE*      file,
                int        spacing )
{
  if ( ! root ) return;

  if ( ! file ) file = stdout;

  util::fprintf( file, "%*s%s", spacing, "", root -> name() );

  if ( ! root -> value.empty() )
  {
    util::fprintf( file, " : '%s'", root -> value.c_str() );
  }
  util::fprintf( file, "\n" );

  int num_children = ( int ) root -> children.size();
  for ( int i = 0; i < num_children; i++ )
  {
    print( root -> children[ i ], file, spacing + 2 );
  }
}

