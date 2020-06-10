// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include <string>

struct scoped_callback_t;

struct special_effect_db_item_t
{
  unsigned spell_id;
  std::string encoded_options;
  scoped_callback_t* cb_obj;
  bool fallback;

  special_effect_db_item_t() : spell_id( 0 ), encoded_options(), cb_obj( nullptr ), fallback( false )
  {
  }
};
