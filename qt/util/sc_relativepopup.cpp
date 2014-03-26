// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "sc_relativepopup.hpp"

// ============================================================================
// SC_RelativePopupWidget
// ============================================================================

SC_RelativePopup::SC_RelativePopup(QWidget* parent,
                                   Qt::Corner parentCornerToAnchor,
                                   Qt::Corner widgetCornerToAnchor) :
  QWidget(parent),
  parentCornerToAnchor_(parentCornerToAnchor),
  widgetCornerToAnchor_( widgetCornerToAnchor),
  timeTillHide_(1000),
  timeTillFastHide_(800),
  timeFastHide_(200),
  hideChildren(true)
{
  setMouseTracking(true);
  setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);

  timerTillHide_.setSingleShot(true);
  timerTillFastHide_.setSingleShot(true);

  timeToWait__ = timeTillHide_;

  connect(&timerTillHide_, SIGNAL( timeout() ), this, SLOT( hideRequest() ));
  connect(&timerTillFastHide_, SIGNAL( timeout() ), this, SLOT( fastHideRequest() ));
}
