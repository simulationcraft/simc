// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

struct special_effect_t;

// Special Effect ===========================================================

// A special effect callback that will be scoped by the pure virtual valid() method of the class.
// If the implemented valid() returns false, the special effect initializer callbacks will not be
// invoked.
//
// Scoped callbacks are prioritized statically.
//
// Special effects will be selected and initialize automatically for the actor from the highest
// priority class.  Currently PRIORITY_CLASS is used by class-scoped special effects callback
// objects that use the class scope (i.e., player_e) validator, and PRIORITY_SPEC is used by
// specialization-scoped special effect callback objects. The separation of the two allows, for
// example, the creation of a class-scoped special effect, and then an additional
// specialization-scoped special effect for a single specialization for the class.
//
// The PRIORITY_DEFAULT value is used for old-style callbacks (i.e., static functions or
// string-based initialization defined in sc_unique_gear.cpp).
struct scoped_callback_t
{
  enum priority
  {
    PRIORITY_DEFAULT = 0U, // Old-style callbacks, and any kind of "last resort" special effect
    PRIORITY_CLASS,        // Class-filtered callbacks
    PRIORITY_SPEC          // Specialization-filtered callbacks
  };

  priority priority;

  scoped_callback_t() : priority( PRIORITY_DEFAULT )
  { }

  scoped_callback_t( enum priority p ) : priority( p )
  { }

  virtual ~scoped_callback_t()
  { }

  // Validate special effect against conditions. Return value of false indicates that the
  // initializer should not be invoked.
  virtual bool valid( const special_effect_t& ) const = 0;

  // Initialize the special effect.
  virtual void initialize( special_effect_t& ) = 0;
};