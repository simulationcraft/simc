// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

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
      if ( name_str == parameters[ i ].name_str )
        return &( parameters[ i ] );
    return 0;
  }
};

namespace { // UNNAMED NAMESPACE =========================================

struct xml_cache_entry_t
{
  xml_node_t*  root;
  cache::era_t era;
};

typedef std::unordered_map<std::string, xml_cache_entry_t> xml_cache_t;

static xml_cache_t xml_cache;
static mutex_t xml_mutex;

} // UNNAMED NAMESPACE

// Forward Declarations =====================================================

static int create_children( sim_t* sim, xml_node_t* root, const std::string& input, std::string::size_type& index );

// simplify_xml =============================================================

static void simplify_xml( std::string& buffer )
{
  util::replace_all( buffer, "&lt;", '<' );
  util::replace_all( buffer, "&gt;", '>' );
  util::replace_all( buffer, "&amp;", '&' );
}

// is_white_space ===========================================================

static bool is_white_space( char c )
{
  return( c == ' '  ||
          c == '\t' ||
          c == '\n' ||
          c == '\r' ||
          // FIX-ME: Need proper UTF-8 support
          c < 0 );
}

// is_name_char =============================================================

static bool is_name_char( char c )
{
  if ( isalpha( c ) ) return true;
  if ( isdigit( c ) ) return true;
  return( c == '_' || c == '-' || c ==':' );
}

// parse_name ===============================================================

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

// create_parameter =========================================================

static void create_parameter( xml_node_t*             node,
                              const std::string&      input,
                              std::string::size_type& index )
{
  // required format:  name="value"

  std::string name_str;
  if ( ! parse_name( name_str, input, index ) )
    return;

  assert( input[ index ] == '=' );
  index++;
  char quote = input[ index ];
  assert( quote == '"' || quote == '\'' );
  index++;

  std::string::size_type start = index;
  while ( input[ index ] != quote )
  {
    assert( input[ index ] );
    index++;
  }
  std::string value_str = input.substr( start, index-start );
  index++;

  node -> parameters.push_back( xml_parm_t( name_str, value_str ) );
}

// create_node ==============================================================

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
    create_parameter( node, input, ++index );
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
    if ( sim )
    {
      int start = std::min( 0, ( ( int ) index - 32 ) );
      sim -> errorf( "Unexpected character '%c' at index=%d node=%s context=%s\n",
                     c, ( int ) index, node -> name(), input.substr( start, index-start ).c_str() );
      fprintf( sim -> output_file, "%s\n", input.c_str() );
      sim -> cancel();
    }
    return 0;
  }

  return node;
}

// create_children ==========================================================

static int create_children( sim_t*                  sim,
                            xml_node_t*             root,
                            const std::string&      input,
                            std::string::size_type& index )
{
  while ( ! sim || ! sim -> canceled )
  {
    while ( is_white_space( input[ index ] ) ) index++;

    if ( input[ index ] == '<' )
    {
      index++;

      if ( input[ index ] == '/' )
      {
        std::string name_str;
        parse_name( name_str, input, ++index );
        //assert( name_str == root -> name_str );
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
            if ( sim )
            {
              sim -> errorf( "Unexpected EOF at index %d (%s)\n", ( int ) index, root -> name() );
              sim -> errorf( "%s\n", input.c_str() );
              sim -> cancel();
            }
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
        xml_node_t* n = create_node( sim, input, index );
        if ( ! n ) return 0;
        root -> children.push_back( n );
      }
    }
    else if ( input[ index ] == '\0' )
    {
      break;
    }
    else
    {
      std::string::size_type start = index;
      while ( input[ index ] )
      {
        if ( input[ index ] == '<' )
        {
          if ( isalpha( input[ index+ 1 ] ) ) break;
          if ( input[ index+1 ] == '/' ) break;
          if ( input[ index+1 ] == '?' ) break;
          if ( input[ index+1 ] == '!' ) break;
        }
        index++;
      }
      root -> parameters.push_back( xml_parm_t( ".", input.substr( start, index-start ) ) );
    }
  }

  return ( int ) root -> children.size();
}

// search_tree ==============================================================

static xml_node_t* search_tree( xml_node_t*        root,
                                const std::string& name_str )
{
  if ( ! root ) return 0;

  if ( name_str.empty() || name_str == root -> name_str )
    return root;

  int num_children = ( int ) root -> children.size();
  for ( int i=0; i < num_children; i++ )
  {
    xml_node_t* node = search_tree( root -> children[ i ], name_str );
    if ( node ) return node;
  }

  return 0;
}

// search_tree ==============================================================

static xml_node_t* search_tree( xml_node_t*        root,
                                const std::string& name_str,
                                const std::string& parm_name,
                                const std::string& parm_value )
{
  if ( ! root ) return 0;

  if ( name_str.empty() || name_str == root -> name_str )
  {
    xml_parm_t* parm = root -> get_parm( parm_name );
    if ( parm && parm -> value_str == parm_value ) return root;
  }

  int num_children = ( int ) root -> children.size();
  for ( int i=0; i < num_children; i++ )
  {
    xml_node_t* node = search_tree( root -> children[ i ], name_str, parm_name, parm_value );
    if ( node ) return node;
  }

  return 0;
}

// split_path ===============================================================

static xml_node_t* split_path( xml_node_t*        node,
                               std::string&       key,
                               const std::string& path )
{
  if ( path.find( '/' ) == path.npos )
  {
    key = path;
  }
  else
  {
    std::vector<std::string> splits;
    int num_splits = util::string_split( splits, path, "/" );

    for ( int i=0; i < num_splits-1; i++ )
    {
      node = search_tree( node, splits[ i ] );
      if ( ! node ) return 0;
    }

    key = splits[ num_splits-1 ];
  }

  return node;
}

#ifndef UNIT_TEST

// xml::get_name ============================================================

const char* xml::get_name( xml_node_t* node )
{
  return node -> name();
}

// xml::get =================================================================

xml_node_t* xml::get( sim_t*             sim,
                      const std::string& url,
                      cache::behavior_e  caching,
                      const std::string& confirmation )
{
  auto_lock_t lock( xml_mutex );

  xml_cache_t::iterator p = xml_cache.find( url );
  if ( p != xml_cache.end() && ( caching != cache::CURRENT || p -> second.era >= cache::era() ) )
    return p -> second.root;

  std::string result;
  if ( ! http::get( result, url, caching, confirmation ) )
    return 0;

  if ( xml_node_t* node = xml::create( sim, result ) )
  {
    xml_cache_entry_t& c = xml_cache[ url ];
    c.root = node;
    c.era = cache::era();
    return node;
  }

  return 0;
}

#endif

// xml::create ==============================================================

xml_node_t* xml::create( sim_t* sim,
                         const std::string& input )
{
  xml_node_t* root = new xml_node_t( "root" );

  std::string buffer = input;
  std::string::size_type index=0;

  simplify_xml( buffer );
  create_children( sim, root, buffer, index );

  return root;
}

// xml::create ==============================================================

xml_node_t* xml::create( sim_t* sim, FILE* input )
{
  if ( ! input ) return 0;
  std::string buffer;
  char c;
  while ( ( c = fgetc( input ) ) != EOF ) buffer += c;
  return create( sim, buffer );
}

// xml::get_child ===========================================================

xml_node_t* xml::get_child( xml_node_t*        root,
                            const std::string& name_str )
{
  if ( ! root ) return 0;
  int num_children = ( int ) root -> children.size();
  for ( int i=0; i < num_children; i++ )
  {
    xml_node_t* node = root -> children[ i ];
    if ( name_str == node -> name_str ) return node;
  }

  return 0;
}

// xml::get_children ========================================================

int xml::get_children( std::vector<xml_node_t*>& nodes,
                       xml_node_t*               root,
                       const std::string&        name_str )
{
  if ( ! root ) return 0;
  int num_children = ( int ) root -> children.size();
  for ( int i=0; i < num_children; i++ )
  {
    xml_node_t* node = root -> children[ i ];
    if ( name_str.empty() || name_str == node -> name_str )
    {
      nodes.push_back( node );
    }
  }

  return ( int ) nodes.size();
}

// xml::get_node ============================================================

xml_node_t* xml::get_node( xml_node_t*        root,
                           const std::string& path )
{
  if ( ! root ) return 0;

  if ( path.empty() || path == root -> name_str )
    return root;

  std::string name_str;
  xml_node_t* node = split_path( root, name_str, path );

  if ( node ) node = search_tree( node, name_str );

  return node;
}

// xml::get_node ============================================================

xml_node_t* xml::get_node( xml_node_t*        root,
                           const std::string& path,
                           const std::string& parm_name,
                           const std::string& parm_value )
{
  if ( ! root ) return 0;

  std::string name_str;
  xml_node_t* node = split_path( root, name_str, path );

  if ( node ) node = search_tree( node, name_str, parm_name, parm_value );

  return node;
}

// xml::get_nodes ===========================================================

int xml::get_nodes( std::vector<xml_node_t*>& nodes,
                    xml_node_t*               root,
                    const std::string&        path )
{
  if ( ! root ) return 0;

  if ( path.empty() || path == root -> name_str )
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

// xml::get_nodes ===========================================================

int xml::get_nodes( std::vector<xml_node_t*>& nodes,
                    xml_node_t*               root,
                    const std::string&        path,
                    const std::string& parm_name,
                    const std::string& parm_value )
{
  if ( ! root ) return 0;

  if ( path.empty() || path == root -> name_str )
  {
    xml_parm_t* parm = root -> get_parm( parm_name );
    if ( parm && parm -> value_str == parm_value )
    {
      nodes.push_back( root );
    }
  }
  else
  {
    std::string name_str;
    xml_node_t* node = split_path( root, name_str, path );

    int num_children = ( int ) node -> children.size();
    for ( int i=0; i < num_children; i++ )
    {
      get_nodes( nodes, node -> children[ i ], name_str, parm_name, parm_value );
    }
  }

  return ( int ) nodes.size();
}

// xml::get_value ===========================================================

bool xml::get_value( std::string&       value,
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

// xml::get_value ===========================================================

bool xml::get_value( int&               value,
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

// xml::get_value ===========================================================

bool xml::get_value( double&            value,
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

// xml::print ===============================================================

void xml::print( xml_node_t* root,
                 FILE*       file,
                 int         spacing )
{
  if ( ! root ) return;

  if ( ! file ) file = stdout;

  util::fprintf( file, "%*s%s", spacing, "", root -> name() );

  int num_parms = ( int ) root -> parameters.size();
  for ( int i=0; i < num_parms; i++ )
  {
    xml_parm_t& parm = root -> parameters[ i ];
    util::fprintf( file, " %s=\"%s\"", parm.name(), parm.value_str.c_str() );
  }
  util::fprintf( file, "\n" );

  int num_children = ( int ) root -> children.size();
  for ( int i=0; i < num_children; i++ )
  {
    print( root -> children[ i ], file, spacing+2 );
  }
}

