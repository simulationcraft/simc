// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "util/scoped_callback.hpp"

#include <functional>

struct special_effect_t;
struct wowv_t;

using custom_cb_t = std::function<void( special_effect_t& )>;

struct wrapper_callback_t : public scoped_callback_t
{
  custom_cb_t cb;
  wowv_t min_build;
  wowv_t max_build;

  wrapper_callback_t( custom_cb_t cb_, wowv_t min_, wowv_t max_ );

  bool valid( const special_effect_t& ) const override;
  void initialize( special_effect_t& effect ) override;
};
