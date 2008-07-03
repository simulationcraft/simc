// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <simcraft.h>

// ==========================================================================
// Rating
// ==========================================================================

namespace { // ANONYMOUS NAMESPACE ==========================================

// rating_60 =================================================================

static rating_t& rating_60()
{
  static rating_t r;

  if( r.haste == 0 )
  {
    r.haste            = 1000;
    r.spell_hit        = 800;
    r.spell_crit       = 1400;
    r.attack_hit       = 1000;
    r.attack_crit      = 1400;
    r.attack_expertise = 1000;
  }

  return r;
}

// rating_70 =================================================================

static rating_t& rating_70()
{
  static rating_t r;

  if( r.haste == 0 )
  {
    r.haste            = 1576;
    r.spell_hit        = 1262;
    r.spell_crit       = 2208;
    r.attack_hit       = 1576;
    r.attack_crit      = 2208;
    r.attack_expertise = 1576;
  }

  return r;
}

// rating_80 =================================================================

static rating_t& rating_80()
{
  static rating_t r;

  if( r.haste == 0 )
  {
    rating_t& r_70 = rating_70();

    r.haste            = r_70.haste            * 2.0; 
    r.spell_hit        = r_70.spell_hit        * 2.0; 
    r.spell_crit       = r_70.spell_crit       * 2.0; 
    r.attack_hit       = r_70.attack_hit       * 2.0; 
    r.attack_crit      = r_70.attack_crit      * 2.0; 
    r.attack_expertise = r_70.attack_expertise * 2.0; 
  }

  return r;
}

} // ANONYMOUS NAMESPACE =====================================================

// rating_t::init ============================================================

void rating_t::init( int level )
{
  if( level <= 60 )
  {
    *this = rating_60();
  }
  else if( level == 70 )
  {
    *this = rating_70();
  }
  else if( level >= 80 )
  {
    *this = rating_80();
  }
  else if( level < 70 )
  {
    // Assume linear progression for now.

    rating_t& r_min = rating_60();
    rating_t& r_max = rating_70();

    double adjust = ( level - 60 ) / 10.0;

    haste            = r_min.haste            + adjust * ( r_max.haste            - r_min.haste            );
    spell_hit        = r_min.spell_hit        + adjust * ( r_max.spell_hit        - r_min.spell_hit        );
    spell_crit       = r_min.spell_crit       + adjust * ( r_max.spell_crit       - r_min.spell_crit       );
    attack_hit       = r_min.attack_hit       + adjust * ( r_max.attack_hit       - r_min.attack_hit       );
    attack_crit      = r_min.attack_crit      + adjust * ( r_max.attack_crit      - r_min.attack_crit      );
    attack_expertise = r_min.attack_expertise + adjust * ( r_max.attack_expertise - r_min.attack_expertise );
  }
  else if( level > 70 )
  {
    // Assume linear progression for now.

    rating_t& r_min = rating_70();
    rating_t& r_max = rating_80();

    double adjust = ( level - 70 ) / 10.0;

    haste            = r_min.haste            + adjust * ( r_max.haste            - r_min.haste            );
    spell_hit        = r_min.spell_hit        + adjust * ( r_max.spell_hit        - r_min.spell_hit        );
    spell_crit       = r_min.spell_crit       + adjust * ( r_max.spell_crit       - r_min.spell_crit       );
    attack_hit       = r_min.attack_hit       + adjust * ( r_max.attack_hit       - r_min.attack_hit       );
    attack_crit      = r_min.attack_crit      + adjust * ( r_max.attack_crit      - r_min.attack_crit      );
    attack_expertise = r_min.attack_expertise + adjust * ( r_max.attack_expertise - r_min.attack_expertise );
  }
  else assert(0);
}
