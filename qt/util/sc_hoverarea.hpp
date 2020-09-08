// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

#include <QEvent>
#include <QTimer>
#include <QtWidgets/QWidget>

// ============================================================================
// SC_HoverAreaWidget
// ============================================================================

class SC_HoverArea : public QWidget
{
  Q_OBJECT
  Q_PROPERTY( int timeout READ timeout WRITE setTimeout )
  int timeout_;
  QTimer timeSinceMouseEntered;

public:
  SC_HoverArea( QWidget* parent = 0, int timeout = 1000 ) : QWidget( parent ), timeout_( timeout )
  {
    setMouseTracking( true );
    connect( &timeSinceMouseEntered, SIGNAL( timeout() ), this, SLOT( TimerTimeout() ) );
    timeSinceMouseEntered.setSingleShot( true );
  }

  int timeout() const
  {
    return timeout_;
  }

  void setTimeout( int timeout )
  {
    timeout_ = timeout;
  }

protected:
  void enterEvent( QEvent* /* event */ ) override
  {
    if ( underMouse() && timeSinceMouseEntered.isActive() == false )
      timeSinceMouseEntered.start( timeout_ );
  }

  void leaveEvent( QEvent* /* event */ ) override
  {
    timeSinceMouseEntered.stop();
  }

public slots:
  virtual void TimerTimeout()
  {
    emit( mouseHoverTimeout() );
  }

signals:
  void mouseHoverTimeout();
};
