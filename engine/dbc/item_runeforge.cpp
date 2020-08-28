#include <array>

#include "config.hpp"

#include "util/util.hpp"

#include "item_runeforge.hpp"

#include "generated/item_runeforge.inc"
#if SC_USE_PTR == 1
#include "generated/item_runeforge_ptr.inc"
#endif

util::span<const runeforge_legendary_entry_t> runeforge_legendary_entry_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __runeforge_legendary_data, __ptr_runeforge_legendary_data, ptr );
}

util::span<const runeforge_legendary_entry_t> runeforge_legendary_entry_t::find(
    util::string_view name, bool ptr, bool tokenized )
{
  std::string name_str = tokenized ? util::tokenize_fn( name ) : std::string( name );

  auto __data = data( ptr );
  auto __begin = std::find_if( __data.begin(), __data.end(),
    [&name_str, &tokenized]( const runeforge_legendary_entry_t& e ) {
      if ( tokenized )
      {
        std::string rf_name = util::tokenize_fn( e.name );
        return util::str_compare_ci( rf_name, name_str );
      }
      else
      {
        return util::str_compare_ci( e.name, name_str );
      }
  } );

  if ( __begin == __data.end() )
  {
    return {};
  }

  auto __end = std::find_if_not( __begin + 1, __data.end(),
    [&name_str, tokenized]( const runeforge_legendary_entry_t& e ) {
      if ( tokenized )
      {
        std::string rf_name = util::tokenize_fn( e.name );
        return util::str_compare_ci( rf_name, name_str );
      }
      else
      {
        return util::str_compare_ci( e.name, name_str );
      }
  } );

  return { __begin, __end };
}

util::span<const runeforge_legendary_entry_t> runeforge_legendary_entry_t::find_by_spellid(
  unsigned spell_id, bool ptr )
{
  auto __data = data( ptr );
  auto __begin = std::find_if( __data.begin(), __data.end(),
                  [ spell_id ]( const runeforge_legendary_entry_t& e ) {
                    return spell_id == e.spell_id;
                  } );

  if ( __begin == __data.end() )
  {
    return {};
  }

  auto __end = std::find_if_not( __begin + 1, __data.end(),
                [ spell_id ]( const runeforge_legendary_entry_t& e ) {
                  return spell_id == e.spell_id;
                } );

  return { __begin, __end };
}
