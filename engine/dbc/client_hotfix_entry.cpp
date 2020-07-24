#include "client_hotfix_entry.hpp"

#include "util/generic.hpp"

util::span<const hotfix::client_hotfix_entry_t> hotfix::find_hotfixes(
  util::span<const hotfix::client_hotfix_entry_t> data, unsigned id )
{
  auto r = range::equal_range( data, id, {}, &client_hotfix_entry_t::id );
  if ( r.first == data.end() )
    return {};
  return { r.first, r.second };
}
