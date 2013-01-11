// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE =========================================

struct xml_cache_entry_t
{
  xml_node_t*  root;
  cache::era_t era;
  xml_cache_entry_t() : root( 0 ), era( cache::IN_THE_BEGINNING ) { }
  ~xml_cache_entry_t() { if ( root ) delete root; }
};

typedef std::unordered_map<std::string, xml_cache_entry_t> xml_cache_t;

static xml_cache_t xml_cache;
static mutex_t xml_mutex;

} // UNNAMED NAMESPACE

// simplify_xml =============================================================

static void simplify_xml( std::string& buffer )
{
  util::replace_all( buffer, "&lt;", "<" );
  util::replace_all( buffer, "&gt;", ">" );
  util::replace_all( buffer, "&amp;", "&" );
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

// xml_node_t::create_parameter =============================================

void xml_node_t::create_parameter( const std::string&      input,
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

  parameters.push_back( xml_parm_t( name_str, value_str ) );
}

// xml_node_t::create_node ==================================================

xml_node_t* xml_node_t::create_node( sim_t*                  sim,
                                     const std::string&      input,
                                     std::string::size_type& index )
{
  char c = input[ index ];
  if ( c == '?' ) index++;

  std::string name_str;
  parse_name( name_str, input, index );
  assert( ! name_str.empty() );

  xml_node_t* node = new xml_node_t( name_str );
  if ( ! node ) return 0;

  while ( is_white_space( input[ index ] ) )
  {
    node -> create_parameter( input, ++index );
  }

  c = input[ index ];

  if ( c == '/' || c == '?' )
  {
    index += 2;
  }
  else if ( c == '>' )
  {
    node -> create_children( sim, input, ++index );
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

// xml_node_t::create_children ==============================================

int xml_node_t::create_children( sim_t*                  sim,
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
          std::string::size_type finish = input.find( "]]>", index );
          if ( finish == std::string::npos )
          {
            if ( sim )
            {
              sim -> errorf( "Unexpected EOF at index %d (%s)\n", ( int ) index, name() );
              sim -> errorf( "%s\n", input.c_str() );
              sim -> cancel();
            }
            return 0;
          }
          parameters.push_back( xml_parm_t( "cdata", input.substr( index, finish-index ) ) );
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
        children.push_back( n );
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
      parameters.push_back( xml_parm_t( ".", input.substr( start, index-start ) ) );
    }
  }

  return ( int ) children.size();
}

// xml_node_t::search_tree ==================================================

xml_node_t* xml_node_t::search_tree( const std::string& node_name )
{
  if ( node_name.empty() || node_name == name_str )
    return this;

  int num_children = ( int ) children.size();
  for ( int i=0; i < num_children; i++ )
  {
    if ( children[ i ] )
    {
      xml_node_t* node = children[ i ] -> search_tree( node_name );
      if ( node ) return node;
    }
  }

  return 0;
}

// xml_node_t::search_tree ==================================================

xml_node_t* xml_node_t::search_tree( const std::string& node_name,
                                     const std::string& parm_name,
                                     const std::string& parm_value )
{
  if ( node_name.empty() || node_name == name_str )
  {
    xml_parm_t* parm = get_parm( parm_name );
    if ( parm && parm -> value_str == parm_value ) return this;
  }

  int num_children = ( int ) children.size();
  for ( int i=0; i < num_children; i++ )
  {
    if ( children[ i ] )
    {
      xml_node_t* node = children[ i ] -> search_tree( node_name, parm_name, parm_value );
      if ( node ) return node;
    }
  }

  return 0;
}

// xml_node_t::split_path ===================================================

xml_node_t* xml_node_t::split_path( std::string&       key,
                                    const std::string& path )
{
  xml_node_t* node = this;

  if ( path.find( '/' ) == path.npos )
  {
    key = path;
  }
  else
  {
    std::vector<std::string> splits = util::string_split( path, "/" );
    for ( size_t i = 0; i < splits.size() - 1; i++ )
    {
      node = node -> search_tree( splits[ i ] );
      if ( ! node ) return 0;
    }

    key = splits[ splits.size() - 1 ];
  }

  return node;
}

#ifndef UNIT_TEST

// xml_node_t::get ==========================================================

xml_node_t* xml_node_t::get( sim_t*             sim,
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

  if ( xml_node_t* node = xml_node_t::create( sim, result ) )
  {
    xml_cache_entry_t& c = xml_cache[ url ];
    c.root = node;
    c.era = cache::era();
    return node;
  }

  return 0;
}

#endif

// xml_node_t::create =======================================================

xml_node_t* xml_node_t::create( sim_t* sim,
                                const std::string& input )
{
  xml_node_t* root = new xml_node_t( "root" );

  std::string buffer = input;
  std::string::size_type index=0;

  simplify_xml( buffer );
  if ( root )
    root -> create_children( sim, buffer, index );

  return root;
}

// xml_node_t::create =======================================================

xml_node_t* xml_node_t::create( sim_t* sim, FILE* input )
{
  if ( ! input ) return 0;
  std::string buffer;
  char c;
  while ( ( c = fgetc( input ) ) != EOF ) buffer += c;
  return create( sim, buffer );
}

// xml_node_t::get_child ====================================================

xml_node_t* xml_node_t::get_child( const std::string& name_str )
{
  int num_children = ( int ) children.size();
  for ( int i=0; i < num_children; i++ )
  {
    xml_node_t* node = children[ i ];
    if ( node && ( name_str == node -> name_str ) ) return node;
  }

  return 0;
}

// xml_node_t::get_children =================================================

int xml_node_t::get_children( std::vector<xml_node_t*>& nodes,
                              const std::string&        name_str )
{
  int num_children = ( int ) children.size();
  for ( int i=0; i < num_children; i++ )
  {
    xml_node_t* node = children[ i ];
    if ( node && ( name_str.empty() || name_str == node -> name_str ) )
    {
      nodes.push_back( node );
    }
  }

  return ( int ) nodes.size();
}

// xml_node_t::get_node =====================================================

xml_node_t* xml_node_t::get_node( const std::string& path )
{
  if ( path.empty() || path == name_str )
    return this;

  std::string name_str;
  xml_node_t* node = split_path( name_str, path );

  if ( node ) node = node -> search_tree( name_str );

  return node;
}

// xml_node_t::get_node =====================================================

xml_node_t* xml_node_t::get_node( const std::string& path,
                                  const std::string& parm_name,
                                  const std::string& parm_value )
{
  std::string name_str;
  xml_node_t* node = split_path( name_str, path );

  if ( node ) node = node -> search_tree( name_str, parm_name, parm_value );

  return node;
}

// xml_node_t::get_nodes ====================================================

int xml_node_t::get_nodes( std::vector<xml_node_t*>& nodes,
                           const std::string&        path )
{
  if ( path.empty() || path == name_str )
  {
    nodes.push_back( this );
  }
  else
  {
    std::string name_str;
    xml_node_t* node = split_path( name_str, path );
    if ( ! node ) return ( int ) nodes.size();

    int num_children = ( int ) node -> children.size();
    for ( int i=0; i < num_children; i++ )
    {
      if ( node -> children[ i ] )
      {
        node -> children[ i ] -> get_nodes( nodes, name_str );
      }
    }
  }

  return ( int ) nodes.size();
}

// xml_node_t::get_nodes ====================================================

int xml_node_t::get_nodes( std::vector<xml_node_t*>& nodes,
                           const std::string&        path,
                           const std::string&        parm_name,
                           const std::string&        parm_value )
{
  if ( path.empty() || path == name_str )
  {
    xml_parm_t* parm = get_parm( parm_name );
    if ( parm && parm -> value_str == parm_value )
    {
      nodes.push_back( this );
    }
  }
  else
  {
    std::string name_str;
    xml_node_t* node = split_path( name_str, path );
    if ( ! node ) return ( int ) nodes.size();

    int num_children = ( int ) node -> children.size();
    for ( int i=0; i < num_children; i++ )
    {
      if ( node -> children[ i ] )
      {
        node -> children[ i ] -> get_nodes( nodes, name_str, parm_name, parm_value );
      }
    }
  }

  return ( int ) nodes.size();
}

// xml_node_t::get_value ====================================================

bool xml_node_t::get_value( std::string&       value,
                            const std::string& path )
{
  std::string key;

  xml_node_t* node = split_path( key, path );
  if ( ! node ) return false;

  xml_parm_t* parm = node -> get_parm( key );
  if ( ! parm ) return false;

  value = parm -> value_str;

  return true;
}

// xml_node_t::get_value ====================================================

bool xml_node_t::get_value( int&               value,
                            const std::string& path )
{
  std::string key;

  xml_node_t* node = split_path( key, path );
  if ( ! node ) return false;

  xml_parm_t* parm = node -> get_parm( key );
  if ( ! parm ) return false;

  value = atoi( parm -> value_str.c_str() );

  return true;
}

// xml_node_t::get_value ====================================================

bool xml_node_t::get_value( double&            value,
                            const std::string& path )
{
  std::string key;

  xml_node_t* node = split_path( key, path );
  if ( ! node ) return false;

  xml_parm_t* parm = node -> get_parm( key );
  if ( ! parm ) return false;

  value = atof( parm -> value_str.c_str() );

  return true;
}

// xml_node_t::print ========================================================

void xml_node_t::print( FILE*       file,
                        int         spacing )
{
  if ( ! file ) file = stdout;

  util::fprintf( file, "%*s%s", spacing, "", name() );

  int num_parms = ( int ) parameters.size();
  for ( int i=0; i < num_parms; i++ )
  {
    xml_parm_t& parm = parameters[ i ];
    util::fprintf( file, " %s=\"%s\"", parm.name(), parm.value_str.c_str() );
  }
  util::fprintf( file, "\n" );

  int num_children = ( int ) children.size();
  for ( int i=0; i < num_children; i++ )
  {
    if ( children[ i ] )
    {
      children[ i ] -> print( file, spacing+2 );
    }
  }
}

// xml_node_t::print_xml ====================================================

void xml_node_t::print_xml( FILE*       file,
                            int         spacing )
{
  if ( ! file ) file = stdout;

  util::fprintf( file, "%*s<%s", spacing, "", name() );

  int num_parms = ( int ) parameters.size();
  std::string content;
  for ( int i=0; i < num_parms; i++ )
  {
    xml_parm_t& parm = parameters[ i ];
    std::string parm_value = parm.value_str;
    util::replace_all( parm_value, "&", "&amp;" );
    util::replace_all( parm_value, "\"", "&quot;" );
    util::replace_all( parm_value, "<", "&lt;" );
    util::replace_all( parm_value, ">", "&gt;" );
    if ( parm.name_str == "." )
      content = parm_value;
    else
      util::fprintf( file, " %s=\"%s\"", parm.name(), parm_value.c_str() );
  }

  int num_children = ( int ) children.size();
  if ( num_children == 0 )
  {
    if ( content.empty() )
      util::fprintf( file, " />\n" );
    else
      util::fprintf( file, ">%s</%s>\n", content.c_str(), name() );
  }
  else
  {
    util::fprintf( file, ">%s\n", content.c_str() );
    for ( int i=0; i < num_children; i++ )
    {
      if ( children[ i ] )
      {
        children[ i ] -> print_xml( file, spacing+2 );
      }
    }
    util::fprintf( file, "%*s</%s>\n", spacing, "", name() );
  }
}

// xml_node_t::get_parm =====================================================

xml_parm_t* xml_node_t::get_parm( const std::string& parm_name )
{
  int num_parms = ( int ) parameters.size();
  for ( int i=0; i < num_parms; i++ )
  {
    if ( parm_name == parameters[ i ].name_str )
    {
      return &( parameters[ i ] );
    }
  }
  return 0;
}

// xml_node_t::~xml_node_t ==================================================

xml_node_t::~xml_node_t()
{
  int num_children = ( int ) children.size();
  for ( int i=0; i < num_children; i++ )
  {
    if ( children[ i ] ) delete children[ i ];
  }
}

// xml_node_t::add_child ====================================================

xml_node_t* xml_node_t::add_child( const std::string& name )
{
  xml_node_t* node = new xml_node_t( name );
  if ( node ) children.push_back( node );
  return node;
}
