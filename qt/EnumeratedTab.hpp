// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include <QtWidgets/QTabWidget>

class QKeyEvent;

class SC_enumeratedTabBase : public QTabWidget
{
private:
  QList<QPair<Qt::Key, QList<Qt::KeyboardModifier> > > ignoreKeys;
public:
  SC_enumeratedTabBase( QWidget* parent );

  void addIgnoreKeyPressEvent( Qt::Key k, const QList<Qt::KeyboardModifier>& s );
  bool removeIgnoreKeyPressEvent( Qt::Key k, const QList<Qt::KeyboardModifier>& s );
  void removeAllIgnoreKeyPressEvent();
protected:
  void keyPressEvent( QKeyEvent* e ) override;
};

template <typename E>
class SC_enumeratedTab : public SC_enumeratedTabBase
{
public:
  SC_enumeratedTab( QWidget* parent = nullptr ) : SC_enumeratedTabBase( parent )
  {
  }

  E currentTab()
  {
    return static_cast<E>( currentIndex() );
  }

  void setCurrentTab( E t )
  {
    return setCurrentIndex( static_cast<int>( t ) );
  }
};
