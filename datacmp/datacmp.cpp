// datacmp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <cassert>
#include <string>
#include <cstdio>
#include <algorithm>

namespace
{

std::string class_name( unsigned class_mask )
{
  static const char *names[] =
  {
    "War",
    "Pal",
    "Hun",
    "Rog",
    "Pri",
    "DKn",
    "Sha",
    "Mag",
    "Wlo",
    0,
    "Dru",
  };

  std::string ret;
  for ( unsigned i = 0; i < 11; ++i )
  {
    if ( names[i] && ( class_mask & ( 1 << i ) ) )
      ret += names[i];
  }

  return ret.size() ? ret : std::string( "???" );
}

void indent( int levels, const char *str )
{
  for ( int i = 0; i < levels; ++i )
    putc( ' ', stdout );
  puts( str );
}

}

template <typename Talent, typename Spell, typename Effect>
struct base_data
{
  typedef Talent talent_type;
  typedef Spell spell_type;
  typedef Effect effect_type;

  talent_type const *t_b, *t_e;
  spell_type const *s_b, *s_e;
  effect_type const *e_b, *e_e;

  unsigned t_maxid;
  talent_type const ** t_index;

  unsigned s_maxid;
  spell_type const ** s_index;

  unsigned e_maxid;
  effect_type const ** e_index;

  template <size_t t_size, size_t s_size, size_t e_size>
  base_data( talent_type const ( &t_b_ )[t_size], spell_type const ( &s_b_ )[s_size], effect_type const ( &e_b_ )[e_size] )
    : t_b( t_b_ ), t_e( t_b_ + t_size ), s_b( s_b_ ), s_e( s_b_ + s_size ), e_b( e_b_ ), e_e( e_b_ + e_size )
  {
    t_index = build_index( t_b, t_e, t_maxid );
    s_index = build_index( s_b, s_e, s_maxid );
    e_index = build_index( e_b, e_e, e_maxid );
  }

  ~base_data()
  {
    delete[] t_index;
    delete[] s_index;
    delete[] e_index;
  }

  template <typename T>
  T const **build_index( T const *first, T const *last, unsigned &maxid )
  {
    maxid = std::max_element( first, last, id_lt<T> )->id;
    T const **index = new T const *[maxid + 1];
    memset( index, 0, sizeof( T const * ) * ( maxid + 1 ) );
    for ( T const *a = first; a != last; ++a )
    {
      index[a->id] = a;
    }
    return index;
  }

  talent_type const * const t_begin() const { return t_b; }
  talent_type const * const t_end() const { return t_e; }

  spell_type const * const s_begin() const { return s_b; }
  spell_type const * const s_end() const { return s_e; }

  effect_type const * const e_begin() const { return e_b; }
  effect_type const * const e_end() const { return e_e; }

  void print_talent( talent_type const *t )
  {
    std::string t_class = class_name( t->m_class );

    printf( "Talent '%s' (%u):\n", t->name, t->id );
    printf( "  Tab page:    %u\n", t->tab_page );
    printf( "  Class:       %s\n", t_class.c_str() );
    printf( "  Depends on:  %u\n", t->dependance );
    printf( "  Column:      %u\n", t->col );
    printf( "  Row:         %u\n", t->row );
    for ( int i = 0; i < 3 && t->rank_id[i]; ++i )
    {
      printf( "  Rank #%d id:  %u\n", i + 1, t->rank_id[i] );
    }
  }

  void print_spell( spell_type const *s )
  {
    std::string s_class = class_name( s->class_mask );

    printf( "Spell '%s' (%u):\n", s->name, s->id );
    printf( "  School:        %u\n", s->school );
    printf( "  Power type:    %d\n", s->power_type );
    printf( "  Class:         %s\n", s_class.c_str() );
    printf( "  Level:         %u\n", s->spell_level );
    printf( "  Min range:     %lf\n", s->min_range );
    printf( "  Max range:     %lf\n", s->max_range );
    printf( "  Cooldown:      %u\n", s->cooldown );
    printf( "  GCD:           %u\n", s->gcd );
    printf( "  Category:      %u\n", s->category );
    printf( "  Duration:      %lf\n", s->duration );
    printf( "  Cost:          %u\n", s->cost );
    printf( "  Rune cost:     %X\n", s->rune_cost );
    printf( "  Runic power:   %u\n", s->runic_power_gain );
    printf( "  Max stack:     %u\n", s->max_stack );
    printf( "  Proc chance:   %u\n", s->proc_chance );
    printf( "  Proc charges:  %u\n", s->proc_charges );
    printf( "  Min cast time: %d\n", s->cast_min );
    printf( "  Max cast time: %d\n", s->cast_max );
    printf( "  Cast time div: %d\n", s->cast_div );
    for ( int i = 0; i < 3 && s->effect[i]; ++i )
    {
      printf( "  Effect #%d id:  %u\n", i + 1, s->effect[i] );
    }
    if ( s->desc )
    {
      printf( "  Description:\n" );
      indent( 4, s->desc );
    }
    if ( s->tooltip )
    {
      printf( "  Tooltip:\n" );
      indent( 4, s->tooltip );
    }
  }

  void print_effect( effect_type const *e )
  {
    printf( "Effect %u:\n", e->id );
    printf( "  Spell:              '%s' (%u)\n", s_index[e->spell_id]->name, e->spell_id );
    printf( "  Index:              %u\n", e->index );
    printf( "  Type:               %d\n", e->type );
    printf( "  Subtype:            %d\n", e->subtype );
    printf( "  Avg. scaling mult:  %lf\n", e->m_avg );
    printf( "  Delta scaling mult: %lf\n", e->m_delta );
    printf( "  Coefficient:        %lf\n", e->coeff );
    printf( "  Amplitude:          %lf\n", e->amplitude );
    printf( "  Radius:             %lf\n", e->radius );
    printf( "  Radius max:         %lf\n", e->radius_max );
    printf( "  Base value:         %d\n", e->base_value );
    printf( "  Misc. value 1:      %d\n", e->misc_value );
    printf( "  Misc. value 2:      %d\n", e->misc_value_2 );
    printf( "  Trigger spell:      %d\n", e->trigger_spell );
    printf( "  Effect per combopt: %lf\n", e->pp_combo_points );
  }

  int count_ranks( talent_type const *t )
  {
    for ( int i = 0; i < 3; ++i )
    {
      if ( !t->rank_id[i] )
        return i;
    }

    return 3;
  }

private:

  template <typename T>
  static bool id_lt( T const & v1, T const & v2 ) { return v1.id < v2.id; }
};

namespace v1
{

struct data : public base_data<talent_data_t, spell_data_t, spelleffect_data_t>
{
  data() : base_data( __talent_data, __spell_data, __spelleffect_data )
  {
  }
};

}

namespace v2
{

struct data : public base_data<talent_data_t, spell_data_t, spelleffect_data_t>
{
  data() : base_data( __talent_data, __spell_data, __spelleffect_data )
  {
  }
};

}

v1::data d1;
v2::data d2;

void compare_spell( std::string t_class, const char *t_name, unsigned t_id, bool& print_header, int ix, unsigned id );

struct compare_effect_helper
{
  const std::string s_class;
  const char *s_name;
  const unsigned s_id;
  int ix;
  const unsigned id;

  compare_effect_helper( const std::string& s_class_, const char *s_name_, unsigned s_id_, int ix_, unsigned id_ )
    : s_class( s_class_ ), s_name( s_name_ ), s_id( s_id_ ), ix( ix_ ), id( id_ )
  {
  }

  template <typename T1, typename T2>
  bool cmp( T1 v1, T2 v2, const char *fmt )
  {
    if ( v1 == v2 )
      return true;

    printf( "%s: Spell '%s' (%d) effect #%d (%d) ", s_class.c_str(), s_name, s_id, ix, id );
    printf( fmt, v1, v2 );

    return false;
  }
};

void compare_spell_effect( const std::string& s_class, const char *s_name, unsigned s_id, int ix, unsigned id )
{
  v1::data::effect_type const *e1 = d1.e_index[id]; assert( e1 );
  v2::data::effect_type const *e2 = d2.e_index[id]; assert( e2 );

  compare_effect_helper h( s_class, s_name, s_id, ix, id );

  h.cmp( e1->type,         e2->type,         "changed type from %d to %d\n" );
  h.cmp( e1->subtype,      e2->subtype,      "changed subtype from %d to %d\n" );
  h.cmp( e1->m_avg,        e2->m_avg,        "changed m_avg from %lf to %lf\n" );
  h.cmp( e1->m_delta,      e2->m_delta,      "changed m_delta from %lf to %lf\n" );
  h.cmp( e1->coeff,        e2->coeff,        "changed coeff from %lf to %lf\n" );
  h.cmp( e1->amplitude,    e2->amplitude,    "changed amplitude from %lf to %lf\n" );
  h.cmp( e1->radius,       e2->radius,       "changed radius from %lf to %lf\n" );
  h.cmp( e1->radius_max,   e2->radius_max,   "changed radius_max from %lf to %lf\n" );
  h.cmp( e1->base_value,   e2->base_value,   "changed base value from %d to %d\n" );
  h.cmp( e1->misc_value,   e2->misc_value,   "changed misc value from %d to %d\n" );
  h.cmp( e1->misc_value_2, e2->misc_value_2, "changed misc value 2 from %d to %d\n" );

  if ( h.cmp( e1->trigger_spell, e2->trigger_spell, "changed trigger spell from %d to %d\n" ) && e1->trigger_spell )
  {
    if ( d1.s_index[e1->trigger_spell] && d2.s_index[e1->trigger_spell] )
    {
      //compare_spell(t_class, t_name, t_id, print_header, s_ix, e1->trigger_spell);
    }
  }
}

struct compare_spell_helper
{
  const std::string s_class;
  const char *name;
  const unsigned id;

  compare_spell_helper( const std::string& s_class_, const char *s_name_, unsigned id_ )
    : s_class( s_class_ ), name( s_name_ ), id( id_ )
  {
  }

  template <typename T1, typename T2>
  bool cmp( T1 v1, T2 v2, const char *fmt )
  {
    if ( v1 == v2 )
      return true;

    printf( "%s: Spell '%s' (%d) ", s_class.c_str(), name, id );
    printf( fmt, v1, v2 );

    return false;
  }
};

void compare_spell( v1::data::spell_type const *s1, v2::data::spell_type const *s2 )
{
  std::string s_class = class_name( s1->class_mask );
  compare_spell_helper h( s_class, s1->name, s1->id );

  h.cmp( s1->prj_speed,        s2->prj_speed,        "changed projectile speed from %lf to %lf\n" );
  h.cmp( s1->school,           s2->school,           "changed school from 0x%02x to 0x%02x\n" );
  h.cmp( s1->power_type,       s2->power_type,       "changed power type from %d to %d\n" );
  h.cmp( s1->class_mask,       s2->class_mask,       "changed class mask from 0x%02x to 0x%02x\n" );
  h.cmp( s1->race_mask,        s2->race_mask,        "changed race mask from 0x%02x to 0x%02x\n" );
  h.cmp( s1->spell_level,      s2->spell_level,      "changed spell level from %d to %d\n" );
  h.cmp( s1->min_range,        s2->min_range,        "changed min range from %lf to %lf\n" );
  h.cmp( s1->max_range,        s2->max_range,        "changed max range from %lf to %lf\n" );
  h.cmp( s1->cooldown,         s2->cooldown,         "changed cooldown from %d to %d\n" );
  h.cmp( s1->gcd,              s2->gcd,              "changed gcd from %d to %d\n" );
  h.cmp( s1->category,         s2->category,         "changed category from %d to %d\n" );
  h.cmp( s1->duration,         s2->duration,         "changed duration from %lf to %lf\n" );
  h.cmp( s1->cost,             s2->cost,             "changed cost from %d to %d\n" );
  h.cmp( s1->rune_cost,        s2->rune_cost,        "changed rune cost from 0x%02x to 0x%02x\n" );
  h.cmp( s1->runic_power_gain, s2->runic_power_gain, "changed runic power gain from %d to %d\n" );
  h.cmp( s1->max_stack,        s2->max_stack,        "changed max stack from %d to %d\n" );
  h.cmp( s1->proc_chance,      s2->proc_chance,      "changed proc chance from %d to %d\n" );
  h.cmp( s1->proc_charges,     s2->proc_charges,     "changed proc charges from %d to %d\n" );
  h.cmp( s1->cast_min,         s2->cast_min,         "changed min cast time from %d to %d\n" );
  h.cmp( s1->cast_max,         s2->cast_max,         "changed max cast time from %d to %d\n" );
  h.cmp( s1->cast_div,         s2->cast_div,         "changed cast time divisor from %d to %d\n" );

  for ( int i = 0; i < 3; ++i )
  {
    char buf[256];
    sprintf_s( buf, "changed spell effect #%d from %%d to %%d\n", i + 1 );
    if ( h.cmp( s1->effect[i], s2->effect[i], buf ) && s1->effect[i] )
    {
      compare_spell_effect( s_class, s1->name, s1->id, i + 1, s1->effect[i] );
    }
  }

  // TODO attributes
}

void compare_talent( v1::data::talent_type const *t1, v2::data::talent_type const *t2 )
{
  bool print_header = true;
  std::string t_class = class_name( t1->m_class );

  if ( std::string( t1->name ) != std::string( t2->name ) )
  {
    printf( "%s: Talent '%s' (%d) changed name to '%s'\n", t_class.c_str(), t1->name, t1->id, t2->name );
    print_header = false;
  }

  const char* name = t1->name;
  unsigned id = t1->id;

  assert( t1->flags == t2->flags );
  assert( t1->m_class == t2->m_class );
  assert( t1->m_pet == t2->m_pet );

  if ( t1->tab_page != t2->tab_page )
  {
    printf( "%s: Talent '%s' (%d) moved from page %d to page %d\n", t_class.c_str(), name, id, t1->tab_page, t2->tab_page );
    print_header = false;
  }
  else if ( t1->col != t2->col || t1->row != t2->row )
  {
    printf( "%s: Talent '%s' (%d) moved from (%d,%d) to (%d,%d)\n", t_class.c_str(), name, id, t1->col, t1->row, t2->col, t2->row );
    print_header = false;
  }

  if ( d1.count_ranks( t1 ) != d2.count_ranks( t2 ) )
  {
    printf( "%s: Talent '%s' (%d) changed #ranks from %d to %d\n", t_class.c_str(), name, id, d1.count_ranks( t1 ), d2.count_ranks( t2 ) );
    print_header = false;
  }
  else
  {
    for ( int i = 0; i < d1.count_ranks( t1 ); ++i )
    {
      if ( t1->rank_id[i] != t2->rank_id[i] )
      {
        printf( "%s: Talent '%s' (%d) spell #%d changed from %d to %d\n", t_class.c_str(), name, id, i + 1, t1->rank_id[i], t2->rank_id[i] );
        print_header = false;
      }
    }
  }
}

int main( int argc, char* argv[] )
{
  v1::talent_data_t const *t1 = v1::__talent_data;
  v2::talent_data_t const *t2 = v2::__talent_data;

  if ( argc > 2 )
  {
    if ( _stricmp( argv[1], "-t" ) == 0 )
    {
      for ( int i = 2; i < argc; ++i )
      {
        int id = atoi( argv[i] );
        if ( d2.t_index[id] ) d2.print_talent( d2.t_index[id] );
      }
      return 0;
    }

    if ( _stricmp( argv[1], "-s" ) == 0 )
    {
      for ( int i = 2; i < argc; ++i )
      {
        int id = atoi( argv[i] );
        if ( d2.s_index[id] ) d2.print_spell( d2.s_index[id] );
      }
      return 0;
    }

    if ( _stricmp( argv[1], "-e" ) == 0 )
    {
      for ( int i = 2; i < argc; ++i )
      {
        int id = atoi( argv[i] );
        if ( d2.e_index[id] ) d2.print_effect( d2.e_index[id] );
      }
      return 0;
    }
  }

  if ( argc > 1 )
  {
    FILE *fp;
    freopen_s( &fp, argv[1], "w", stdout );
  }

  while ( t1 < d1.t_end() || t2 < d2.t_end() )
  {
    while ( t1 < d1.t_end() && t2 < d2.t_end() && t1->id == t2->id )
    {
      // same talent
      compare_talent( t1, t2 );
      t1++;
      t2++;
    }

    while ( t1 < d1.t_end() && ( t2 == d2.t_end() || t1->id < t2->id ) )
    {
      // removed talents
      std::string t_class = class_name( t1->m_class );
      printf( "%s: Talent '%s' (%d) was removed\n", t_class.c_str(), t1->name, t1->id );
      t1++;
    }

    while ( t2 < d2.t_end() && ( t1 == d1.t_end() || t1->id > t2->id ) )
    {
      // added talents
      std::string t_class = class_name( t2->m_class );
      printf( "%s: Talent '%s' (%d) was added\n", t_class.c_str(), t2->name, t2->id );
      t2++;
    }
  }

  puts( "" );

  v1::spell_data_t const *s1 = v1::__spell_data;
  v2::spell_data_t const *s2 = v2::__spell_data;

  while ( s1 < d1.s_end() || s2 < d2.s_end() )
  {
    while ( s1 < d1.s_end() && s2 < d2.s_end() && s1->id == s2->id )
    {
      compare_spell( s1, s2 );
      s1++;
      s2++;
    }

    while ( s1 < d1.s_end() && ( s2 == d2.s_end() || s1->id < s2->id ) )
    {
      // removed spells
      std::string t_class = class_name( s1->class_mask );
      printf( "%s: Spell '%s' (%d) was removed\n", t_class.c_str(), s1->name, s1->id );
      s1++;
    }

    while ( s2 < d2.s_end() && ( s1 == d1.s_end() || s1->id > s2->id ) )
    {
      // added spells
      std::string t_class = class_name( s2->class_mask );
      printf( "%s: Spell '%s' (%d) was added\n", t_class.c_str(), s2->name, s2->id );
      s2++;
    }
  }

  return 0;
}

