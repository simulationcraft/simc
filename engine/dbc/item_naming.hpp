// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef ITEM_NAMING_HPP
#define ITEM_NAMING_HPP

#include "util/span.hpp"

#include "client_data.hpp"

struct item_name_description_t
{
  unsigned id;
  const char* description;

  static const item_name_description_t& find( unsigned id, bool ptr )
  { return dbc::find<item_name_description_t>( id, ptr ); }

  static const item_name_description_t& nil()
  { return dbc::nil<item_name_description_t>(); }

  static util::span<const item_name_description_t> data( bool ptr );
};

#endif /* ITEM_NAMING_HPP */
