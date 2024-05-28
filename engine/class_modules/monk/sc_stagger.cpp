#include "sc_stagger.hpp"

namespace stagger_impl
{
sample_data_t::sample_data_t( player_t* player, std::string_view name, std::vector<std::string_view> mitigation_tokens )
{
  absorbed  = player->get_sample_data( fmt::format( "Total damage absorbed by {}.", name ) );
  taken     = player->get_sample_data( fmt::format( "Total damage taken from {}.", name ) );
  mitigated = player->get_sample_data( fmt::format( "Total damage mitigated by {}.", name ) );

  for ( const std::string_view& token : mitigation_tokens )
    mitigated_by_ability[ token ] = player->get_sample_data( fmt::format( "Total {} purified by {}.", name, token ) );
}
}  // namespace stagger_impl
