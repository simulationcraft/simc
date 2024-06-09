#include "sc_stagger.hpp"

std::string_view level_data_t::name() const
{
  if ( spell_data != spell_data_t::nil() )
    return spell_data->name_cstr();
  if ( min_threshold > 0 )
    return "maximum";
  return "none";
}

std::string_view stagger_data_t::name() const
{
  return self_damage->name_cstr();
}

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
