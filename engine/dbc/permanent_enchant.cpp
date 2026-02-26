#include "config.hpp"

#include "permanent_enchant.hpp"

#include "generated/permanent_enchant.inc"
#if SC_USE_PTR == 1
#include "generated/permanent_enchant_ptr.inc"
#endif

util::span<const permanent_enchant_entry_t> permanent_enchant_entry_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __permanent_enchant_data, __ptr_permanent_enchant_data, ptr );
}

static bool _check_name( const permanent_enchant_entry_t& entry, const std::string_view& name )
{
  if ( entry.tokenized_name == name )
  {
    return true;
  }

  // check again with midnight enchant verbose names 'enchant_<slot>__<name>'
  if ( strstr( entry.tokenized_name, "enchant_" ) == entry.tokenized_name )
  {
    if ( auto c = strstr( entry.tokenized_name, "__" ) )
    {
      if ( name == c + 2 )
      {
        return true;
      }
    }
  }

  return false;
}

const permanent_enchant_entry_t& permanent_enchant_entry_t::find( std::string_view name, unsigned rank, int item_class,
                                                                  int inv_type, int item_subclass, bool ptr )
{
  auto __data = data( ptr );
  auto __lower = std::lower_bound( __data.begin(), __data.end(), name,
    [ rank ]( const permanent_enchant_entry_t& left, const std::string_view& right ) {
      if ( _check_name( left, right ) )
      {
        return left.rank < rank;
      }
      else
      {
        return left.tokenized_name < right;
      }
    } );

  if ( __lower == __data.end() || !_check_name( *__lower, name ) || __lower->rank != rank )
  {
    return nil();
  }

  auto __upper = std::upper_bound( __data.begin(), __data.end(), name,
    [ rank ]( const std::string_view& left, const permanent_enchant_entry_t& right ) {
      if ( _check_name( right, left ) )
      {
        return rank < right.rank;
      }
      else
      {
        return left < right.tokenized_name;
      }
    } );

  auto span = util::span<const permanent_enchant_entry_t>( __lower, __upper );

  auto it = range::find_if( span,
    [ item_class, inv_type, item_subclass ]( const permanent_enchant_entry_t& entry ) {
      if ( item_class != entry.item_class )
      {
        return false;
      }

      if ( inv_type > 0 && entry.mask_inventory_type > 0 && !( entry.mask_inventory_type & ( 1 << inv_type ) ) )
      {
        return false;
      }

      if ( item_subclass > 0 && entry.mask_item_subclass > 0 && !( entry.mask_item_subclass & ( 1 << item_subclass ) ) )
      {
        return false;
      }

      return true;
    } );

  if ( it != span.end() )
  {
    return *it;
  }

  return nil();
}

const permanent_enchant_entry_t& permanent_enchant_entry_t::find( unsigned enchant_id, bool ptr )
{
  auto __data = data( ptr );
  auto it = range::find_if( __data, [ enchant_id ]( const permanent_enchant_entry_t& entry ) {
    return entry.enchant_id == enchant_id;
  } );

  if ( it != __data.end() )
  {
    return *it;
  }

  return nil();
}
