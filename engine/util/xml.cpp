// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "rapidxml/rapidxml_print.hpp"

// XML Reader ==================================================================

namespace { // UNNAMED NAMESPACE =========================================

struct xml_cache_entry_t
{
  std::shared_ptr<xml_node_t> root;
  cache::era_t era;
  xml_cache_entry_t() : root(), era( cache::era_t::IN_THE_BEGINNING ) { }
};

struct new_xml_cache_entry_t
{
  std::shared_ptr<sc_xml_t> root;
  cache::era_t era;
  new_xml_cache_entry_t() : root(), era( cache::era_t::IN_THE_BEGINNING ) { }
};

typedef std::unordered_map<std::string, xml_cache_entry_t> xml_cache_t;
typedef std::unordered_map<std::string, new_xml_cache_entry_t> new_xml_cache_t;

xml_cache_t xml_cache;
new_xml_cache_t new_xml_cache;
mutex_t xml_mutex;


// simplify_xml =============================================================

void simplify_xml( std::string& buffer )
{
  util::replace_all( buffer, "&lt;", "<" );
  util::replace_all( buffer, "&gt;", ">" );
  util::replace_all( buffer, "&amp;", "&" );
}

// is_white_space ===========================================================

bool is_white_space( char c )
{
  return( c == ' '  ||
      c == '\t' ||
      c == '\n' ||
      c == '\r' ||
      // FIX-ME: Need proper UTF-8 support
      c < 0 );
}

// is_name_char =============================================================

bool is_name_char( char c )
{
  if ( isalpha( c ) ) return true;
  if ( isdigit( c ) ) return true;
  return( c == '_' || c == '-' || c == ':' );
}

// parse_name ===============================================================

bool parse_name( std::string&            name_str,
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

} // UNNAMED NAMESPACE

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
  std::string value_str = input.substr( start, index - start );
  index++;
  simplify_xml( value_str );

  parameters.push_back( xml_parm_t( name_str, value_str ) );
}

// xml_node_t::create_node ==================================================

std::shared_ptr<xml_node_t> xml_node_t::create_node( sim_t*                  sim,
                                                     const std::string&      input,
                                                     std::string::size_type& index )
{
  char c = input[ index ];
  if ( c == '?' ) index++;

  std::string name_str;
  parse_name( name_str, input, index );
  assert( ! name_str.empty() );

  std::shared_ptr<xml_node_t> node( new xml_node_t( name_str ) );
  if ( ! node ) return node;

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
      throw std::invalid_argument(fmt::format("Unexpected character '{}' at index={} node={} context={} from input '{}'.",
                     c, index, node -> name(), input.substr( start, index - start ), input ));
    }
    return std::shared_ptr<xml_node_t>();
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
              throw std::runtime_error(fmt::format("Unexpected EOF at index {} ({}).", index, name() ));
            }
            return 0;
          }
          parameters.push_back( xml_parm_t( "cdata", input.substr( index, finish - index ) ) );
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
        std::shared_ptr<xml_node_t> n = create_node( sim, input, index );
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
          if ( isalpha( input[ index + 1 ] ) ) break;
          if ( input[ index + 1 ] == '/' ) break;
          if ( input[ index + 1 ] == '?' ) break;
          if ( input[ index + 1 ] == '!' ) break;
        }
        index++;
      }
      parameters.push_back( xml_parm_t( ".", input.substr( start, index - start ) ) );
    }
  }

  return ( int ) children.size();
}

// xml_node_t::search_tree ==================================================

xml_node_t* xml_node_t::search_tree( const std::string& node_name )
{
  if ( node_name.empty() || node_name == name_str )
    return this;

  for ( size_t i = 0; i < children.size(); ++i )
  {
    if ( children[ i ] )
    {
      xml_node_t* node = children[ i ] -> search_tree( node_name );
      if ( node ) return node;
    }
  }

  return nullptr;
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

  for ( size_t i = 0; i < children.size(); ++i )
  {
    if ( children[ i ] )
    {
      xml_node_t* node = children[ i ] -> search_tree( node_name, parm_name, parm_value );
      if ( node ) return node;
    }
  }

  return nullptr;
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
      if ( ! node ) return nullptr;
    }

    key = splits[ splits.size() - 1 ];
  }

  return node;
}

#ifndef UNIT_TEST

// xml_node_t::get ==========================================================

std::shared_ptr<xml_node_t> xml_node_t::get( sim_t*             sim,
                                             const std::string& url,
                                             cache::behavior_e  caching,
                                             const std::string& confirmation )
{
  auto_lock_t lock( xml_mutex );

  auto p = xml_cache.find( url );
  if ( p != xml_cache.end() && ( caching != cache::CURRENT || p -> second.era >= cache::era() ) )
    return p -> second.root;

  std::string result;
  if ( http::get( result, url, caching, confirmation ) != 200 )
    return std::shared_ptr<xml_node_t>();

  if ( std::shared_ptr<xml_node_t> node = xml_node_t::create( sim, result ) )
  {
    xml_cache_entry_t& c = xml_cache[ url ];
    c.root = node;
    c.era = cache::era();
    return node;
  }

  return std::shared_ptr<xml_node_t>();
}

#endif

// xml_node_t::create =======================================================

std::shared_ptr<xml_node_t> xml_node_t::create( sim_t* sim,
                                                const std::string& input )
{
  std::shared_ptr<xml_node_t> root = std::shared_ptr<xml_node_t>( new xml_node_t( "root" ) );

  std::string buffer = input;
  std::string::size_type index = 0;

  if ( root )
    root -> create_children( sim, buffer, index );

  return root;
}

// xml_node_t::get_child ====================================================

xml_node_t* xml_node_t::get_child( const std::string& name_str )
{
  for ( size_t i = 0; i < children.size(); ++i )
  {
    std::shared_ptr<xml_node_t>& node = children[ i ];
    if ( node && ( name_str == node -> name_str ) ) return node.get();
  }

  return nullptr;
}

// xml_node_t::get_children =================================================

std::vector<xml_node_t*> xml_node_t::get_children( const std::string& name_str )
{
  std::vector<xml_node_t*> nodes;
  for ( size_t i = 0; i < children.size(); ++i )
  {
    std::shared_ptr<xml_node_t>& node = children[ i ];
    if ( node && ( name_str.empty() || name_str == node -> name_str ) )
    {
      nodes.push_back( node.get() );
    }
  }

  return nodes;
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

std::vector<xml_node_t*> xml_node_t::get_nodes( const std::string& path )
{
  std::vector<xml_node_t*> nodes;
  if ( path.empty() || path == name_str )
  {
    nodes.push_back( this );
  }
  else
  {
    std::string name_str;
    xml_node_t* node = split_path( name_str, path );
    if ( ! node ) return nodes;

    for ( size_t i = 0; i < children.size(); ++i )
    {
      if ( node -> children[ i ] )
      {
        std::vector<xml_node_t*> n = node -> children[ i ] -> get_nodes( name_str );
        nodes.insert( nodes.end(), n.begin(), n.end() );
      }
    }
  }

  return nodes;
}

// xml_node_t::get_nodes ====================================================

std::vector<xml_node_t*> xml_node_t::get_nodes( const std::string&        path,
                                                const std::string&        parm_name,
                                                const std::string&        parm_value )
{
  std::vector<xml_node_t*> nodes;
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
    if ( ! node ) return nodes;

    for ( size_t i = 0; i < children.size(); ++i )
    {
      if ( node -> children[ i ] )
      {
        std::vector<xml_node_t*> n = node -> children[ i ] -> get_nodes( name_str, parm_name, parm_value );
        nodes.insert( nodes.end(), n.begin(), n.end() );
      }
    }
  }

  return nodes;
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

  value = std::stoi( parm -> value_str.c_str() );

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

  value = std::stod( parm -> value_str.c_str() );

  return true;
}

// xml_node_t::print ========================================================

void xml_node_t::print( FILE*       file,
                        int         spacing )
{
  assert( file );

  util::fprintf( file, "%*s%s", spacing, "", name() );

  for ( size_t i = 0; i < parameters.size(); i++ )
  {
    xml_parm_t& parm = parameters[ i ];
    util::fprintf( file, " %s=\"%s\"", parm.name(), parm.value_str.c_str() );
  }
  util::fprintf( file, "\n" );

  for ( size_t i = 0; i < children.size(); ++i )
  {
    if ( children[ i ] )
    {
      children[ i ] -> print( file, spacing + 2 );
    }
  }
}

// xml_node_t::print_xml ====================================================

void xml_node_t::print_xml( FILE*       file,
                            int         spacing )
{
  assert( file );

  util::fprintf( file, "%*s<%s", spacing, "", name() );

  std::string content;
  for ( size_t i = 0; i < parameters.size(); ++i )
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

  if ( children.empty() )
  {
    if ( content.empty() )
      util::fprintf( file, " />\n" );
    else
      util::fprintf( file, ">%s</%s>\n", content.c_str(), name() );
  }
  else
  {
    util::fprintf( file, ">%s\n", content.c_str() );
    for ( size_t i = 0; i < children.size(); ++i )
    {
      if ( children[ i ] )
      {
        children[ i ] -> print_xml( file, spacing + 2 );
      }
    }
    util::fprintf( file, "%*s</%s>\n", spacing, "", name() );
  }
}

// xml_node_t::get_parm =====================================================

xml_parm_t* xml_node_t::get_parm( const std::string& parm_name )
{
  for ( size_t i = 0; i < parameters.size(); ++i )
  {
    if ( parm_name == parameters[ i ].name_str )
    {
      return &( parameters[ i ] );
    }
  }
  return nullptr;
}

// xml_node_t::add_child ====================================================

xml_node_t* xml_node_t::add_child( const std::string& name )
{
  std::shared_ptr<xml_node_t> node( new xml_node_t( name ) );
  if ( node )
    children.push_back( node );
  return node.get();
}

// XML Writer ================================================================

xml_writer_t::xml_writer_t( const std::string & filename ) :
            file( filename, "w" ),
            tabulation( "  " ), current_state( NONE )
{
}

bool xml_writer_t::ready() const
{
  return file != nullptr;
}

void xml_writer_t::init_document( const std::string & stylesheet_file )
{
  assert( current_state == NONE );

  printf( "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" );
  if ( !stylesheet_file.empty() )
  {
    printf( "<?xml-stylesheet type=\"text/xml\" href=\"%s\"?>", stylesheet_file.c_str() );
  }

  current_state = TEXT;
}

void xml_writer_t::begin_tag( const std::string & tag )
{
  assert( current_state != NONE );

  if ( current_state != TEXT )
  {
    printf( ">" );
  }

  printf( "\n%s<%s", indentation.c_str(), tag.c_str() );

  current_tags.push( tag );
  indentation += tabulation;

  current_state = TAG;
}

void xml_writer_t::end_tag( const std::string & tag )
{
  assert( current_state != NONE );
  assert( ! current_tags.empty() );
  assert( indentation.size() == tabulation.size() * current_tags.size() );

  assert( tag == current_tags.top() );
  current_tags.pop();

  indentation.resize( indentation.size() - tabulation.size() );

  if ( current_state == TAG )
  {
    printf( "/>" );
  }
  else if ( current_state == TEXT )
  {
    printf( "\n%s</%s>", indentation.c_str(), tag.c_str() );
  }

  current_state = TEXT;
}

void xml_writer_t::print_attribute( const std::string & name, const std::string & value )
{ print_attribute_unescaped( name, sanitize( value ) ); }

void xml_writer_t::print_attribute_unescaped( const std::string & name, const std::string & value )
{
  assert( current_state != NONE );

  if ( current_state == TAG )
  {
    printf( " %s=\"%s\"", name.c_str(), value.c_str() );
  }
}

void xml_writer_t::print_tag( const std::string & name, const std::string & inner_value )
{
  assert( current_state != NONE );

  if ( current_state != TEXT )
  {
    printf( ">" );
  }

  printf( "\n%s<%s>%s</%s>", indentation.c_str(), name.c_str(), sanitize( inner_value ).c_str(), name.c_str() );

  current_state = TEXT;
}

void xml_writer_t::print_text( const std::string & input )
{
  assert( current_state != NONE );

  if ( current_state != TEXT )
  {
    printf( ">" );
  }

  printf( "\n%s", sanitize( input ).c_str() );

  current_state = TEXT;
}

std::string xml_writer_t::sanitize( std::string v )
{
  static const replacement replacements[] =
  {
      { "&", "&amp;" },
      { "\"", "&quot;" },
      { "<", "&lt;" },
      { ">", "&gt;" },
  };

  for ( unsigned int i = 0; i < sizeof_array( replacements ); ++i )
    util::replace_all( v, replacements[ i ].from, replacements[ i ].to );

  return v;
}

void sc_xml_t::print_xml( FILE* f, int )
{
  assert( f );
  assert( root );

  std::stringstream s;

  s << *root;

  util::fprintf( f, "%s", s.str().c_str() );
}

void sc_xml_t::print( FILE* file, int spacing )
{
  assert( file );
  assert( root );

  util::fprintf( file, "%*s%s", spacing, "", name().c_str() );

  for ( xml_attribute<>* attr = root -> first_attribute(); attr; attr = attr -> next_attribute() )
  {
    util::fprintf( file, " %s=\"%s\"", attr -> name(), attr -> value() );
  }

  if ( root -> type() == node_element && root -> value_size() > 0 )
  {
    util::fprintf( file, " .=\"%s\"", root -> value() );
  }
  util::fprintf( file, "\n" );

  for ( xml_node<>* n = root -> first_node(); n; n = n -> next_sibling() )
  {
    sc_xml_t node( n );
    node.print( file, spacing + 2 );
  }
}

bool sc_xml_t::get_value( double& value, const std::string& path )
{
  bool ret = true;

  std::string key;

  sc_xml_t node = split_path( key, path );
  if ( ! node.valid() )
  {
    return ret;
  }

  // "." in the path denotes the data inside an XML element. Old XML parser
  // used to make a "." attribute for each tag, RapidXML will contain the DATA
  // of the XML tag in the value() of the object, however the object must be an
  // element.
  if ( key == "." )
  {
    assert( node.root -> type() == node_element );
    value = std::stod( node.root -> value() );
    ret = true;
  }
  else if ( util::str_compare_ci( key, "cdata" ) )
  {
    for ( xml_node<>* n = root -> first_node(); n; n = n -> next_sibling() )
    {
      if ( n -> type() == node_cdata )
      {
        value = std::stod( n -> value() );
        ret = true;
        break;
      }
    }
  }
  else
  {
    xml_attribute<>* attr = node.root -> first_attribute( key.c_str() );
    if ( ! attr )
    {
      return ret;
    }
    value = std::stod( attr -> value() );
    ret = true;
  }

  return ret;
}

bool sc_xml_t::get_value( int& value, const std::string& path )
{
  bool ret = false;
  std::string key;

  sc_xml_t node = split_path( key, path );
  if ( ! node.valid() )
  {
    return ret;
  }

  // "." in the path denotes the data inside an XML element. Old XML parser
  // used to make a "." attribute for each tag, RapidXML will contain the DATA
  // of the XML tag in the value() of the object, however the object must be an
  // element.
  if ( key == "." )
  {
    assert( node.root -> type() == node_element );
    value = std::stoi( node.root -> value() );
    ret = true;
  }
  else if ( util::str_compare_ci( key, "cdata" ) )
  {
    for ( xml_node<>* n = root -> first_node(); n; n = n -> next_sibling() )
    {
      if ( n -> type() == node_cdata )
      {
        value = std::stoi( n -> value() );
        ret = true;
        break;
      }
    }
  }
  else
  {
    xml_attribute<>* attr = node.root -> first_attribute( key.c_str() );
    if ( ! attr )
    {
      return ret;
    }
    value = std::stoi( attr -> value() );
    ret = true;
  }

  return ret;
}

bool sc_xml_t::get_value( std::string& value,
                          const std::string& path )
{
  bool ret = false;
  std::string key;

  sc_xml_t node = split_path( key, path );
  if ( ! node.valid() )
  {
    return ret;
  }

  // "." in the path denotes the data inside an XML element. Old XML parser
  // used to make a "." attribute for each tag, RapidXML will contain the DATA
  // of the XML tag in the value() of the object, however the object must be an
  // element.
  if ( key == "." )
  {
    assert( node.root -> type() == node_element );
    value = node.root -> value();
    ret = true;
  }
  else if ( util::str_compare_ci( key, "cdata" ) )
  {
    for ( xml_node<>* n = root -> first_node(); n; n = n -> next_sibling() )
    {
      if ( n -> type() == node_cdata )
      {
        value = n -> value();
        ret = true;
        break;
      }
    }
  }
  else
  {
    xml_attribute<>* attr = node.root -> first_attribute( key.c_str() );
    if ( ! attr )
    {
      return ret;
    }
    value = attr -> value();
    ret = true;
  }

  return ret;
}

std::vector<sc_xml_t> sc_xml_t::get_nodes( const std::string& path,
                                           const std::string& parm_name,
                                           const std::string& parm_value )
{
  std::vector<sc_xml_t> nodes;

  if ( ! root )
  {
    return nodes;
  }

  if ( path.empty() || util::str_compare_ci( path, name() ) )
  {
    for ( xml_attribute<>* attr = root -> first_attribute(); attr; attr = attr -> next_attribute() )
    {
      if ( util::str_compare_ci( parm_name, attr -> name() ) &&
           util::str_compare_ci( parm_value, attr -> value() ) )
      {
        nodes.push_back( *this );
      }
    }
  }
  else
  {
    std::string name_str;
    sc_xml_t node = split_path( name_str, path );
    if ( ! node.valid() )
    {
      return nodes;
    }

    for ( xml_node<>* n = root -> first_node(); n; n = n -> next_sibling() )
    {
      sc_xml_t node( n );
      std::vector<sc_xml_t> new_nodes = node.get_nodes( name_str, parm_name, parm_value );
      nodes.insert( nodes.end(), new_nodes.begin(), new_nodes.end() );
    }
  }

  return nodes;
}

std::vector<sc_xml_t> sc_xml_t::get_nodes( const std::string& path )
{
  std::vector<sc_xml_t> nodes;

  if ( ! root )
  {
    return nodes;
  }

  if ( path.empty() || util::str_compare_ci( path, name() ) )
  {
    nodes.push_back( *this );
  }
  else
  {
    std::string name_str;
    sc_xml_t node = split_path( name_str, path );
    if ( ! node.valid() )
    {
      return nodes;
    }

    for ( xml_node<>* n = root -> first_node(); n; n = n -> next_sibling() )
    {
      sc_xml_t node( n );
      std::vector<sc_xml_t> new_nodes = node.get_nodes( name_str );
      nodes.insert( nodes.end(), new_nodes.begin(), new_nodes.end() );
    }
  }

  return nodes;
}

std::vector<sc_xml_t> sc_xml_t::get_children( const std::string& name )
{
  std::vector<sc_xml_t> nodes;

  if ( ! root )
  {
    return nodes;
  }

  for ( xml_node<>* n = root -> first_node(); n; n = n -> next_sibling() )
  {
    if ( ! name.empty() || util::str_compare_ci( name, n -> name() ) )
    {
      nodes.push_back( sc_xml_t( n ) );
    }
  }

  return nodes;
}

sc_xml_t sc_xml_t::get_child( const std::string& name ) const
{
  if ( ! root )
  {
    return sc_xml_t();
  }

  for ( xml_node<>* n = root -> first_node(); n; n = n -> next_sibling() )
  {
    if ( util::str_compare_ci( name, n -> name() ) )
    {
      return sc_xml_t( n );
    }
  }

  return sc_xml_t();
}

sc_xml_t sc_xml_t::get_node( const std::string& path,
                             const std::string& parm_name,
                             const std::string& parm_value ) const
{
  std::string name_str;
  sc_xml_t node = split_path( name_str, path );

  if ( node.valid() )
  {
    node = node.search_tree( name_str, parm_name, parm_value );
  }

  return node;
}

sc_xml_t sc_xml_t::get_node( const std::string& path ) const
{
  if ( path.empty() || util::str_compare_ci( path, name() ) )
  {
    return *this;
  }

  std::string name_str;

  sc_xml_t node = split_path( name_str, path );

  if ( node.valid() )
  {
    node = node.search_tree( name_str );
  }

  return node;
}

sc_xml_t sc_xml_t::search_tree( const std::string& node_name,
                                const std::string& parm_name,
                                const std::string& parm_value ) const
{
  if ( ! root )
  {
    return sc_xml_t();
  }

  if ( node_name.empty() || util::str_compare_ci( node_name, name() ) )
  {
    for ( xml_attribute<>* attr = root -> first_attribute(); attr; attr = attr -> next_attribute() )
    {
      if ( util::str_compare_ci( parm_name, attr -> name() ) &&
           util::str_compare_ci( parm_value, attr -> value() ) )
      {
        return *this;
      }
    }
  }

  for ( xml_node<>* n = root -> first_node(); n; n = n -> next_sibling() )
  {
    sc_xml_t xml_node( n );
    sc_xml_t target_node = xml_node.search_tree( node_name, parm_name, parm_value );

    if ( target_node.valid() )
    {
      return target_node;
    }
  }

  return sc_xml_t();
}

sc_xml_t sc_xml_t::search_tree( const std::string& node_name ) const
{
  if ( node_name.empty() || util::str_compare_ci( node_name, name() ) )
  {
    return *this;
  }

  if ( ! root )
  {
    return sc_xml_t();
  }

  for ( xml_node<>* n = root -> first_node(); n; n = n -> next_sibling() )
  {
    sc_xml_t xml_node( n );
    sc_xml_t target_node = xml_node.search_tree( node_name );

    if ( target_node.valid() )
    {
      return target_node;
    }
  }

  return sc_xml_t();
}

sc_xml_t sc_xml_t::split_path( std::string& key, const std::string& path ) const
{
  sc_xml_t node = *this;

  if ( path.find( '/' ) == path.npos )
  {
    key = path;
  }
  else
  {
    std::vector<std::string> splits = util::string_split( path, "/" );
    for ( size_t i = 0; i < splits.size() - 1; i++ )
    {
      node = node.search_tree( splits[ i ] );
      if ( ! node.valid() )
      {
        return sc_xml_t();
      }
    }

    key = splits[ splits.size() - 1 ];
  }

  return node;
}

sc_xml_t sc_xml_t::create( sim_t* sim,
                           const std::string& input,
                           const std::string& cache_key )
{
  auto_lock_t lock( xml_mutex );

  auto p = new_xml_cache.find( cache_key );
  if ( p != new_xml_cache.end() )
  {
    return sc_xml_t( *p -> second.root );
  }

  auto  document = new xml_document<>();
  auto  tmp_buf = new char[ input.size() + 1 ];
  memset( tmp_buf, 0, input.size() + 1 );
  memcpy( tmp_buf, input.c_str(), input.size() );

  try
  {
    document -> parse< parse_trim_whitespace >( tmp_buf );
  }
  catch( parse_error& e )
  {
    sim -> errorf( "Unable to parse XML input: %s", e.what() );
    delete document;
    delete[] tmp_buf;
    return sc_xml_t();
  }

  new_xml_cache_entry_t& c = new_xml_cache[ cache_key ];
  c.root = std::shared_ptr<sc_xml_t>( new sc_xml_t( document, tmp_buf ) );
  c.era = cache::era();

  return *c.root;
}

sc_xml_t sc_xml_t::get( sim_t* sim,
                        const std::string& url,
                        cache::behavior_e caching,
                        const std::string& confirmation )
{
  {
    auto_lock_t lock( xml_mutex );

    auto p = new_xml_cache.find( url );
    if ( p != new_xml_cache.end() && ( caching != cache::CURRENT || p -> second.era >= cache::era() ) )
    {
      return sc_xml_t( *p -> second.root );
    }
  }

  std::string result;
  if ( http::get( result, url, caching, confirmation ) != 200 )
    return sc_xml_t();

  return sc_xml_t::create( sim, result, url );
}

