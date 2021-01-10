// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "EnumeratedTab.hpp"

#include <QKeyEvent>

SC_enumeratedTabBase::SC_enumeratedTabBase( QWidget* parent ) : QTabWidget( parent )
{
}

void SC_enumeratedTabBase::addIgnoreKeyPressEvent( Qt::Key k, const QList<Qt::KeyboardModifier>& s )
{
  QPair<Qt::Key, QList<Qt::KeyboardModifier> > p( k, s );
  if ( !ignoreKeys.contains( p ) )
    ignoreKeys.push_back( p );
}

bool SC_enumeratedTabBase::removeIgnoreKeyPressEvent( Qt::Key k, const QList<Qt::KeyboardModifier>& s )
{
  QPair<Qt::Key, QList<Qt::KeyboardModifier> > p( k, s );
  return ignoreKeys.removeAll( p );
}

void SC_enumeratedTabBase::removeAllIgnoreKeyPressEvent()
{
  QList<QPair<Qt::Key, QList<Qt::KeyboardModifier> > > emptyList;
  ignoreKeys = emptyList;
}

void SC_enumeratedTabBase::keyPressEvent( QKeyEvent* e )
{
  int k                   = e->key();
  Qt::KeyboardModifiers m = e->modifiers();

  QList<QPair<Qt::Key, QList<Qt::KeyboardModifier> > >::iterator i = ignoreKeys.begin();
  for ( ; i != ignoreKeys.end(); ++i )
  {
    if ( ( *i ).first == k )
    {
      bool passModifiers                      = true;
      QList<Qt::KeyboardModifier>::iterator j = ( *i ).second.begin();

      for ( ; j != ( *i ).second.end(); ++j )
      {
        if ( !m.testFlag( ( *j ) ) )
        {
          passModifiers = false;
          break;
        }
      }

      if ( passModifiers )
      {
        // key combination matches, send key to base classe's base
        QWidget::keyPressEvent( e );
        return;
      }
    }
  }
  // no key match
  QTabWidget::keyPressEvent( e );
}
