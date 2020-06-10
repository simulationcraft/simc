// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "util/scoped_callback.hpp"

#include <functional>

struct special_effect_t;

using custom_cb_t = std::function<void( special_effect_t& )>;

struct wrapper_callback_t : public scoped_callback_t
{
  custom_cb_t cb;

  wrapper_callback_t( custom_cb_t cb_ ) : scoped_callback_t(), cb( std::move(cb_) )
  {
  }

  bool valid( const special_effect_t& ) const override
  {
    return true;
  }

  void initialize( special_effect_t& effect ) override
  {
    cb( effect );
  }
};