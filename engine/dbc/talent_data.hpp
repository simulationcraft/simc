// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SC_TALENT_DATA_HPP
#define SC_TALENT_DATA_HPP

#include "sc_enums.hpp"
#include "specialization.hpp"
#include "util/span.hpp"
#include "util/string_view.hpp"

struct spell_data_t;

struct talent_data_t
{
  const char * _name;        // Talent name
  unsigned     _id;          // Talent id
  unsigned     _flags;       // Unused for now, 0x00 for all
  unsigned     _m_class;     // Class mask
  unsigned     _spec;        // Specialization
  unsigned     _col;         // Talent column
  unsigned     _row;         // Talent row
  unsigned     _spell_id;    // Talent spell
  unsigned     _replace_id;  // Talent replaces the following spell id

  // Pointers for runtime linking
  const spell_data_t* spell1;

  // Direct member access functions
  unsigned id() const
  { return _id; }

  const char* name_cstr() const
  { return _name; }

  unsigned col() const
  { return _col; }

  unsigned row() const
  { return _row; }

  unsigned spell_id() const
  { return _spell_id; }

  unsigned replace_id() const
  { return _replace_id; }

  unsigned mask_class() const
  { return _m_class; }

  unsigned spec() const
  { return _spec; }

  specialization_e specialization() const
  { return static_cast<specialization_e>( _spec ); }

  // composite access functions

  const spell_data_t* spell() const
  { assert(spell1); return spell1; }

  bool is_class( player_e c ) const;

  // static functions
  static const talent_data_t& nil();
  static const talent_data_t* find( unsigned, bool ptr = false );
  static const talent_data_t* find( unsigned, util::string_view confirmation, bool ptr = false );
  static const talent_data_t* find( util::string_view name, specialization_e spec, bool ptr = false );
  static const talent_data_t* find_tokenized( util::string_view name, specialization_e spec, bool ptr = false );
  static const talent_data_t* find( player_e c, unsigned int row, unsigned int col, specialization_e spec, bool ptr = false );
  static util::span<const talent_data_t> data( bool ptr = false );
  static void link( bool ptr = false );
private:
  static util::span<talent_data_t> _data( bool ptr );
};

struct talent_data_nil_t : public talent_data_t
{
  talent_data_nil_t();
  static const talent_data_nil_t singleton;
};

inline const talent_data_t& talent_data_t::nil()
{ return talent_data_nil_t::singleton; }

#endif // SC_TALENT_DATA_HPP
