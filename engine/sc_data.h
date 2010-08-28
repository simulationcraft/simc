#ifndef SC_DATA_H
#define SC_DATA_H

#include <string>
#include <iostream>
#include "data_definitions.hh"

template <typename T>
class sc_array_t
{
public:
  sc_array_t();
  sc_array_t( const uint32_t i, const uint32_t j = 0, const uint32_t k = 0 );
  ~sc_array_t();
  sc_array_t( const T& copy );

  bool create( const uint32_t i, const uint32_t j = 0, const uint32_t k = 0 );
  bool create_copy( const T* p, const uint32_t i, const uint32_t j = 0, const uint32_t k = 0 );
  bool copy_array( const sc_array_t& copy );

  uint32_t depth;
  uint32_t rows;
  uint32_t cols;

  T& ref( const uint32_t i, const uint32_t j = 0, const uint32_t k = 0 );
  T* ptr( const uint32_t i, const uint32_t j = 0, const uint32_t k = 0 ) SC_CONST;

  void fill( T val );

private:
  T m_def;
  T *m_data;
  uint32_t m_dim;
  uint32_t m_size;
};

template <typename T> sc_array_t<T>::sc_array_t()
{
  m_data = NULL;
  depth = 0;
  rows = 0;
  cols = 0;
  m_size = 0;
  m_dim = 0;
}

template <typename T> sc_array_t<T>::sc_array_t( const uint32_t i, const uint32_t j, const uint32_t k )
{
  m_data = NULL;
  depth = 0;
  rows = 0;
  cols = 0;
  m_size = 0;
  m_dim = 0;

  create( i, j, k );
}

template <typename T> sc_array_t<T>::~sc_array_t()
{
  if ( m_data )
    delete [] m_data;
}

template <typename T> sc_array_t<T>::sc_array_t( const T& copy )
{
  m_data = NULL;

  copy_array( copy );
}

template <typename T> bool sc_array_t<T>::copy_array( const sc_array_t& copy )
{
  if ( m_data )
  {
    delete [] m_data;
    m_data = NULL;
  }
  
  if ( create( copy.cols, copy.rows, copy.depth ) )
  {    
    for ( uint32_t i = 0; i < m_size; i++ )
    {
      m_data[ i ] = copy.m_data[ i ];
    }
    return true;
  }  
  return false;
}

template <typename T> bool sc_array_t<T>::create( const uint32_t i, const uint32_t j, const uint32_t k )
{
  if ( m_data )
  {
    delete [] m_data;
    m_data = NULL;
    depth = 0;
    rows = 0;
    cols = 0;
    m_size = 0;
    m_dim = 0;
  }
  if ( i )
  {
    depth = k;
    rows = j;
    cols = i;

    if ( j )
    {
      if ( k )
      {
        m_size = k * j * i;
        m_dim = 3;
      }
      else
      {
        m_size = j * i;
        m_dim = 2;
      }
    }
    else
    {
      m_size = i;
      m_dim = 1;
    }
    m_data = new T[ m_size ];

    return true;
  }

  return false;
}

template <typename T> bool sc_array_t<T>::create_copy( const T* p, const uint32_t i, const uint32_t j, const uint32_t k )
{
  assert( p );

  if ( !create( i, j, k ) )
    return false;

  for ( uint32_t i = 0; i < m_size; i++, p++ )
  {
    m_data[ i ] = *p;
  }

  return true;
}

template <typename T> inline T& sc_array_t<T>::ref( const uint32_t i, const uint32_t j, const uint32_t k )
{
  assert( m_data && ( i < cols ) && m_dim && ( ( j < rows ) || ( m_dim < 2 ) ) && ( ( k < depth ) || ( m_dim < 3 ) ) );

  if ( m_dim == 1 )
    return m_data[ i ];
  else if ( m_dim == 2 )
    return m_data[ j * cols + i ];
  else
    return m_data[ ( k * rows + j ) * cols + i ];
}

template <typename T> inline T* sc_array_t<T>::ptr( const uint32_t i, const uint32_t j, const uint32_t k ) SC_CONST
{
  assert( m_data && ( i < cols ) && m_dim && ( ( j < rows ) || ( m_dim < 2 ) ) && ( ( k < depth ) || ( m_dim < 3 ) ) );

  if ( m_dim == 1 )
    return m_data + i;
  else if ( m_dim == 2 )
    return m_data + j * cols + i;
  else
    return m_data + ( k * rows + j ) * cols + i;
}

template <typename T> inline void sc_array_t<T>::fill( const T val )
{
  assert( m_data );

  for ( uint32_t i = 0; i < m_size; i++ )
  {
    m_data[ i ] = val;
  }
}

class sc_data_t
{
public:
  sc_data_t( sc_data_t* p = NULL );
  sc_data_t( const sc_data_t& copy );

  void set_parent( sc_data_t* p );
  void reset( );

  sc_array_t<spell_data_t>        m_spells;
  sc_array_t<spelleffect_data_t>  m_effects;
  sc_array_t<talent_data_t>       m_talents;
  sc_array_t<double>              m_melee_crit_base;
  sc_array_t<double>              m_spell_crit_base;
  sc_array_t<double>              m_spell_scaling;
  sc_array_t<double>              m_melee_crit_scale;
  sc_array_t<double>              m_spell_crit_scale;
  sc_array_t<double>              m_regen_spi;
  sc_array_t<double>              m_octregen;
  sc_array_t<double>              m_combat_ratings;
  sc_array_t<double>              m_class_combat_rating_scalar;
  sc_array_t<double>              m_dodge_base;
  sc_array_t<double>              m_dodge_scale;
  sc_array_t<double>              m_base_mp5;
  sc_array_t<stat_data_t>         m_class_stats;
  sc_array_t<stat_data_t>         m_race_stats;

  spell_data_t**                  m_spells_index;
  spelleffect_data_t**            m_effects_index;
  talent_data_t**                 m_talents_index;
  uint32_t                        m_spells_index_size;
  uint32_t                        m_effects_index_size;
  uint32_t                        m_talents_index_size;

  sc_array_t<uint32_t>            m_talent_trees;
  sc_array_t<uint32_t>            m_pet_talent_trees;

  void                            create_index();
private:
  void                            m_copy( const sc_data_t& copy );
  const sc_data_t*                m_parent;

  void                            create_spell_index();
  void                            create_effects_index();
  void                            create_talents_index();

  
  void                            sort_talents( uint32_t* p, const uint32_t num );
  int                             talent_compare( const void *vid1, const void *vid2 );
};

#endif
