// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

struct xml_parm_t
{
  std::string name_str;
  std::string value_str;
  xml_parm_t( const std::string& n, const std::string& v ) : name_str( n ), value_str( v ) {}
  const char* name() { return name_str.c_str(); }
};

struct xml_node_t
{
  std::string name_str;
  std::vector<xml_node_t*> children;
  std::vector<xml_parm_t> parameters;
  xml_node_t() {}
  xml_node_t( const std::string& n ) : name_str( n ) {}
  const char* name() { return name_str.c_str(); }
  xml_parm_t* get_parm( const std::string& name_str )
  {
    int num_parms = ( int ) parameters.size();
    for ( int i=0; i < num_parms; i++ )
      if ( name_str == parameters[ i ].name() )
        return &( parameters[ i ] );
    return 0;
  }
};

struct xml_cache_t
{
  std::string url;
  xml_node_t* node;
  xml_cache_t() : node( 0 ) {}
  xml_cache_t( const std::string& u, xml_node_t* n ) : url( u ), node( n ) {}
};

static void* xml_mutex = 0;

namespace   // ANONYMOUS NAMESPACE =========================================
{

// Forward Declarations ====================================================

  static int create_children( sim_t* sim, xml_node_t* root, const std::string& input, std::string::size_type& index );

// simplify_xml ============================================================

static void simplify_xml( std::string& buffer )
{
  for ( std::string::size_type pos = buffer.find( "&lt;", 0 ); pos != std::string::npos; pos = buffer.find( "&lt;", pos ) )
  {
    buffer.replace( pos, 4, "<" );
  }
  for ( std::string::size_type pos = buffer.find( "&gt;", 0 ); pos != std::string::npos; pos = buffer.find( "&gt;", pos ) )
  {
    buffer.replace( pos, 4, ">" );
  }
  for ( std::string::size_type pos = buffer.find( "&amp;", 0 ); pos != std::string::npos; pos = buffer.find( "&amp;", pos ) )
  {
    buffer.replace( pos, 5, "&" );
  }
}

// is_white_space ==========================================================

static bool is_white_space( char c )
{
  return( c == ' '  ||
          c == '\t' ||
          c == '\n' ||
          c == '\r' ||
          // FIX-ME: Need proper UTF-8 support
          c < 0 );
}

// is_name_char ============================================================

static bool is_name_char( char c )
{
  if ( isalpha( c ) ) return true;
  if ( isdigit( c ) ) return true;
  return( c == '_' || c == '-' || c ==':' );
}

// parse_name ==============================================================

static bool parse_name( std::string&            name_str,
                        const std::string&      input,
                        std::string::size_type& index )
{
  name_str.clear();

  char c = input[ index ];

  if ( ! is_name_char( c ) )
    return false;

  while ( is_name_char( c ) )
  {
    name_str += c;
    c = input[ ++index ];
  }

  return true;
}

// create_parameter ========================================================

static void create_parameter( sim_t*                  sim,
			      xml_node_t*             node,
                              const std::string&      input,
                              std::string::size_type& index )
{
  // required format:  name="value"

  std::string name_str;
  if ( ! parse_name( name_str, input, index ) )
    return;

  assert( input[ index ] == '=' );
  index++;
  assert( input[ index ] == '"' );
  index++;

  std::string::size_type start = index;
  while ( input[ index ] != '"' )
  {
    assert( input[ index ] );
    index++;
  }
  std::string value_str = input.substr( start, index-start );
  index++;

  node -> parameters.push_back( xml_parm_t( name_str, value_str ) );
}

// create_node =============================================================

static xml_node_t* create_node( sim_t*                  sim,
				const std::string&      input,
                                std::string::size_type& index )
{
  char c = input[ index ];
  if ( c == '?' ) index++;

  std::string name_str;
  parse_name( name_str, input, index );
  assert( ! name_str.empty() );

  xml_node_t* node = new xml_node_t( name_str );

  while ( is_white_space( input[ index ] ) )
  {
    create_parameter( sim, node, input, ++index );
  }

  c = input[ index ];

  if ( c == '/' || c == '?' )
  {
    index += 2;
  }
  else if ( c == '>' )
  {
    create_children( sim, node, input, ++index );
  }
  else
  {
    sim -> errorf( "Unexpected character '%c' at index %d (%s)\n", c, ( int ) index, node -> name() );
    sim -> errorf( "%s\n", input.c_str() );
    sim -> cancel();
    return 0;
  }

  return node;
}

// create_children =========================================================

static int create_children( sim_t*                  sim,
			    xml_node_t*             root,
                            const std::string&      input,
                            std::string::size_type& index )
{
  while ( ! sim -> canceled )
  {
    while ( is_white_space( input[ index ] ) ) index++;

    if ( input[ index ] == '<' )
    {
      index++;

      if ( input[ index ] == '/' )
      {
        std::string name_str;
        parse_name( name_str, input, ++index );
        //assert( name_str == root -> name() );
        index++;
        break;
      }
      else if ( input[ index ] == '!' )
      {
        index++;
        if ( input.substr( index, 7 ) == "[CDATA[" )
        {
          index += 7;
          std::string::size_type finish = input.find( "]]", index );
          if ( finish == std::string::npos )
          {
            sim -> errorf( "Unexpected EOF at index %d (%s)\n", ( int ) index, root -> name() );
            sim -> errorf( "%s\n", input.c_str() );
            sim -> cancel();
	    return 0;
          }
          root -> parameters.push_back( xml_parm_t( "cdata", input.substr( index, finish-index ) ) );
          index = finish + 2;
        }
        else
        {
          while ( input[ index ] && input[ index ] != '>' ) index++;
          if ( ! input[ index ] ) break;
        }
        index++;
      }
      else
      {
        root -> children.push_back( create_node( sim, input, index ) );
      }
    }
    else if ( input[ index ] == '\0' )
    {
      break;
    }
    else
    {
      std::string::size_type start = index;
      while ( input[ index ] && input[ index ] != '<' ) index++;
      root -> parameters.push_back( xml_parm_t( ".", input.substr( start, index-start ) ) );
    }
  }

  return ( int ) root -> children.size();
}

// search_tree =============================================================

static xml_node_t* search_tree( xml_node_t*        root,
                                const std::string& name_str )
{
  if ( name_str.empty() || name_str.size() == 0 || name_str == root -> name() )
    return root;

  int num_children = ( int ) root -> children.size();
  for ( int i=0; i < num_children; i++ )
  {
    xml_node_t* node = search_tree( root -> children[ i ], name_str );
    if ( node ) return node;
  }

  return 0;
}

// split_path ==============================================================

static xml_node_t* split_path( xml_node_t*        node,
                               std::string&       key,
                               const std::string& path )
{
  if ( path.find( "/" ) == path.npos )
  {
    key = path;
  }
  else
  {
    std::vector<std::string> splits;
    int num_splits = util_t::string_split( splits, path, "/" );

    for ( int i=0; i < num_splits-1; i++ )
    {
      node = search_tree( node, splits[ i ] );
      if ( ! node ) return 0;
    }

    key = splits[ num_splits-1 ];
  }

  return node;
}

} // ANONYMOUS NAMESPACE ===================================================

#ifndef UNIT_TEST

// xml_t::get_name =========================================================

const char* xml_t::get_name( xml_node_t* node )
{
  return node -> name();
}

// xml_t::download =========================================================

xml_node_t* xml_t::download( sim_t*             sim,
			     const std::string& url,
                             const std::string& confirmation,
                             int64_t            timestamp,
                             int                throttle_seconds )
{
  thread_t::mutex_lock( xml_mutex );

  static std::vector<xml_cache_t> xml_cache;
  int size = ( int ) xml_cache.size();

  xml_node_t* node = 0;

  for ( int i=0; i < size && ! node; i++ )
    if ( xml_cache[ i ].url == url )
      node = xml_cache[ i ].node;

  if ( ! node )
  {
    std::string result;

    if ( http_t::get( result, url, confirmation, timestamp, throttle_seconds ) )
    {
      node = xml_t::create( sim, result );

      if ( node ) xml_cache.push_back( xml_cache_t( url, node ) );
    }
  }

  thread_t::mutex_unlock( xml_mutex );

  return node;
}

// xml_t::download =========================================================

xml_node_t* xml_t::download_cache( sim_t*             sim,
				   const std::string& url,
                                   int64_t            timestamp )
{
  thread_t::mutex_lock( xml_mutex );

  static std::vector<xml_cache_t> xml_cache;
  int size = ( int ) xml_cache.size();

  if ( timestamp < 0 ) timestamp = 0;

  xml_node_t* node = 0;

  for ( int i=0; i < size && ! node; i++ )
    if ( xml_cache[ i ].url == url )
      node = xml_cache[ i ].node;

  if ( ! node )
  {
    std::string result;

    if ( http_t::cache_get( result, url, timestamp ) )
    {
      node = xml_t::create( sim, result );

      if ( node ) xml_cache.push_back( xml_cache_t( url, node ) );
    }
  }

  thread_t::mutex_unlock( xml_mutex );

  return node;
}

#endif

// xml_t::create ===========================================================

xml_node_t* xml_t::create( sim_t* sim, const std::string& input )
{
  xml_node_t* root = new xml_node_t( "root" );

  std::string buffer = input;
  std::string::size_type index=0;

  simplify_xml( buffer );
  create_children( sim, root, buffer, index );

  return root;
}

// xml_t::create ===========================================================

xml_node_t* xml_t::create( sim_t* sim, FILE* input )
{
  if ( ! input ) return 0;
  std::string buffer;
  char c;
  while ( ( c = fgetc( input ) ) != EOF ) buffer += c;
  return create( sim, buffer );
}

// xml_t::get_child ========================================================

xml_node_t* xml_t::get_child( xml_node_t*        root,
                              const std::string& name_str )
{
  int num_children = ( int ) root -> children.size();
  for ( int i=0; i < num_children; i++ )
  {
    xml_node_t* node = root -> children[ i ];
    if ( name_str == node -> name() ) return node;
  }

  return 0;
}

// xml_t::get_children ======================================================

int xml_t::get_children( std::vector<xml_node_t*>& nodes,
                         xml_node_t*               root,
                         const std::string&        name_str )
{
  int num_children = ( int ) root -> children.size();
  for ( int i=0; i < num_children; i++ )
  {
    xml_node_t* node = root -> children[ i ];
    if ( name_str.empty() || name_str == node -> name() )
    {
      nodes.push_back( node );
    }
  }

  return ( int ) nodes.size();
}

// xml_t::get_node =========================================================

xml_node_t* xml_t::get_node( xml_node_t*        root,
                             const std::string& path )
{
  if ( path.empty() || path.size() == 0 || path == root -> name() )
    return root;

  std::string name_str;
  xml_node_t* node = split_path( root, name_str, path );

  if ( node ) node = search_tree( node, name_str );

  return node;
}

// xml_t::get_nodes ========================================================

int xml_t::get_nodes( std::vector<xml_node_t*>& nodes,
                      xml_node_t*               root,
                      const std::string&        path )
{
  if ( path.empty() || path.size() == 0 || path == root -> name() )
  {
    nodes.push_back( root );
  }
  else
  {
    std::string name_str;
    xml_node_t* node = split_path( root, name_str, path );

    int num_children = ( int ) node -> children.size();
    for ( int i=0; i < num_children; i++ )
    {
      get_nodes( nodes, node -> children[ i ], name_str );
    }
  }

  return ( int ) nodes.size();
}

// xml_t::get_value ========================================================

bool xml_t::get_value( std::string&       value,
                       xml_node_t*        root,
                       const std::string& path )
{
  std::string key;
  xml_node_t* node = split_path( root, key, path );
  if ( ! node ) return false;

  xml_parm_t* parm = node -> get_parm( key );
  if ( ! parm ) return false;

  value = parm -> value_str;

  return true;
}

// xml_t::get_value ========================================================

bool xml_t::get_value( int&               value,
                       xml_node_t*        root,
                       const std::string& path )
{
  std::string key;
  xml_node_t* node = split_path( root, key, path );
  if ( ! node ) return false;

  xml_parm_t* parm = node -> get_parm( key );
  if ( ! parm ) return false;

  value = atoi( parm -> value_str.c_str() );

  return true;
}

// xml_t::get_value ========================================================

bool xml_t::get_value( double&            value,
                       xml_node_t*        root,
                       const std::string& path )
{
  std::string key;
  xml_node_t* node = split_path( root, key, path );
  if ( ! node ) return false;

  xml_parm_t* parm = node -> get_parm( key );
  if ( ! parm ) return false;

  value = atof( parm -> value_str.c_str() );

  return true;
}

// xml_t::print ============================================================

void xml_t::print( xml_node_t* root,
                   FILE*       file,
                   int         spacing )
{
  if ( ! root ) return;

  if ( ! file ) file = stdout;

  util_t::fprintf( file, "%*s%s", spacing, "", root -> name() );

  int num_parms = ( int ) root -> parameters.size();
  for ( int i=0; i < num_parms; i++ )
  {
    xml_parm_t& parm = root -> parameters[ i ];
    util_t::fprintf( file, " %s=\"%s\"", parm.name(), parm.value_str.c_str() );
  }
  util_t::fprintf( file, "\n" );

  int num_children = ( int ) root -> children.size();
  for ( int i=0; i < num_children; i++ )
  {
    print( root -> children[ i ], file, spacing+2 );
  }
}

