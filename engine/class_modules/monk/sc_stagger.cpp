#include "sc_stagger.hpp"

namespace stagger_impl
{
sample_data_t::sample_data_t( player_t* player, const stagger_data_t& data )
{
  absorbed  = player->get_sample_data( fmt::format( "Total damage absorbed by {}.", data.name() ) );
  taken     = player->get_sample_data( fmt::format( "Total damage taken from {}.", data.name() ) );
  mitigated = player->get_sample_data( fmt::format( "Total damage mitigated by {}.", data.name() ) );

  for ( std::string_view token : data.mitigation_tokens )
    mitigated_by_ability[ token ] =
        player->get_sample_data( fmt::format( "Total {} purified by {}.", data.name(), token ) );
}
}  // namespace stagger_impl
