// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include <vector>
#include <mutex>
#include "util/generic.hpp"

struct sim_progress_t;

struct work_queue_t
  {
    private:
#ifndef SC_NO_THREADING
    std::recursive_mutex m;
    using G = std::lock_guard<std::recursive_mutex>;
#else
    struct no_m {
      void lock() {}
      void unlock() {}
    };
    struct nop {
      nop(no_m i) { (void)i; }
    };
    no_m m;
    using G = nop;
#endif
    public:
    std::vector<int> _total_work, _work, _projected_work;
    size_t index;

    work_queue_t() : index( 0 )
    { _total_work.resize( 1 ); _work.resize( 1 ); _projected_work.resize( 1 ); }

    void init( int w )    { G l(m); range::fill( _total_work, w ); range::fill( _projected_work, w ); }
    // Single actor batch sim init methods. Batches is the number of active actors
    void batches( size_t n ) { G l(m); _total_work.resize( n ); _work.resize( n ); _projected_work.resize( n ); }

    void flush()          { G l(m); _total_work[ index ] = _projected_work[ index ] = _work[ index ]; }
    int  size()           { G l(m); return index < _total_work.size() ? _total_work[ index ] : _total_work.back(); }
    bool more_work()      { G l(m); return index < _total_work.size() && _work[ index ] < _total_work[ index ]; }
    void lock()           { m.lock(); }
    void unlock()         { m.unlock(); }

    void project( int w )
    {
      G l(m);
      _projected_work[ index ] = w;
#ifdef NDEBUG
      if ( w > _work[ index ] )
      {
      }
#endif
    }

    // Single-actor batch pop, uses several indices of work (per active actor), each thread has it's
    // own state on what index it is simulating
    size_t pop()
    {
      G l(m);

      if ( _work[ index ] >= _total_work[ index ] )
      {
        if ( index < _work.size() - 1 )
        {
          ++index;
        }
        return index;
      }

      if ( ++_work[ index ] == _total_work[ index ] )
      {
        _projected_work[ index ] = _work[ index ];
        if ( index < _work.size() - 1 )
        {
          ++index;
        }
        return index;
      }

      return index;
    }

    sim_progress_t progress( int idx = -1 );
  };