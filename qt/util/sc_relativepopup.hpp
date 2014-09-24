// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include <QtWidgets/QtWidgets>
#include <QtGui/QtGui>

// ============================================================================
// SC_RelativePopupWidget
// ============================================================================

class SC_RelativePopup : public QWidget
{
Q_OBJECT
Q_PROPERTY( Qt::Corner parentCornerToAnchor READ parentCornerToAnchor WRITE setParentCornerToAnchor )
Q_PROPERTY( Qt::Corner widgetCornerToAnchor READ widgetCornerToAnchor WRITE setWidgetCornerToAnchor )
Q_PROPERTY( int timeTillHide READ timeTillHide WRITE setTimeTillHide )
Q_PROPERTY( int timeTillFastHide READ timeTillFastHide WRITE setTimeTillFastHide )
Q_PROPERTY( int timeFastHide READ timeFastHide WRITE setTimeFastHide )
public:
  SC_RelativePopup(QWidget* parent,
      Qt::Corner parentCornerToAnchor = Qt::BottomRightCorner,
      Qt::Corner widgetCornerToAnchor = Qt::TopRightCorner);
  // Q_PROPERTIES
  Qt::Corner parentCornerToAnchor() const
  {
    return parentCornerToAnchor_;
  }
  void setParentCornerToAnchor(Qt::Corner corner)
  {
    parentCornerToAnchor_ = corner;
  }
  Qt::Corner widgetCornerToAnchor() const
  {
    return parentCornerToAnchor_;
  }
  void setWidgetCornerToAnchor(Qt::Corner corner)
  {
    parentCornerToAnchor_ = corner;
  }
  int timeTillHide() const
  {
    return timeTillHide_;
  }
  void setTimeTillHide(int msec)
  {
    timeTillHide_ = msec;
  }
  int timeTillFastHide() const
  {
    return timeTillFastHide_;
  }
  void setTimeTillFastHide(int msec)
  {
    timeTillHide_ = msec;
  }
  int timeFastHide() const
  {
    return timeFastHide_;
  }
  void setTimeFastHide(int msec)
  {
    timeFastHide_ = msec;
  }
private:
  Qt::Corner parentCornerToAnchor_;
  Qt::Corner widgetCornerToAnchor_;
  // Initial time to hide
  int timeTillHide_;
  QTimer timerTillHide_;
  // After being visible/having focus for this time,
  // the mouse leaving should hide quickly
  int timeTillFastHide_;
  QTimer timerTillFastHide_;
  int timeFastHide_;
  int timeToWait__;

  int xbuffer;
  int ybuffer;
  bool hideChildren;
  void disableChildrenHiding()
  {
    hideChildren = false;
  }
  void enableChildrenHiding()
  {
    hideChildren = true;
  }
  bool isWidgetUnderCursorAChild()
  {
    QWidget* widget = qApp->widgetAt(QCursor::pos());
    QObject* parentObject = parent();
    QWidget* parentWidget = qobject_cast< QWidget* >( parentObject );

    while( widget != nullptr )
    {
      if ( widget == this )
      {
        return true;
      }
      else if ( parentWidget != nullptr ) // could be one if, but gcc complains about nullptr cast
      {
        if ( widget == parentWidget )
        {
          return true;
        }
      }
      widget = widget -> parentWidget();
    }

    return false;
  }
  void calculateGeometry()
  {
    // Convert to global points
    QObject* parent__ = parent();
    if ( parent__ == NULL )
      return;
    QWidget* parent_ = static_cast< QWidget* >( parent__ );
    QRect parentRect = parent_ -> geometry();
    parentRect.moveTopLeft( parent_ -> mapToGlobal(QPoint(0,0)) );

    QRect widgetRect( parentRect );
    QLayout* layout_ = layout();
    if ( layout_ != NULL )
    {
      QApplication::instance() -> sendPostedEvents();
      layout_ -> activate();
      layout_ -> update();
    }
    QRect normalGeometry_ = normalGeometry();
    // Use normal geometry if there is any
    if ( normalGeometry_.width() != 0 && normalGeometry_.height() != 0 )
    {
      widgetRect.setWidth( normalGeometry_.width() );
      widgetRect.setHeight( normalGeometry_.height() );
    }
    if ( layout_ != nullptr )
    {
      QSize sizeHint = layout_ -> sizeHint();
      if ( widgetRect.height() < sizeHint.height() )
        widgetRect.setHeight( sizeHint.height() );
      if ( widgetRect.width() < sizeHint.width() )
        widgetRect.setWidth( sizeHint.width() );
      widgetRect.setSize( sizeHint );
    }

    QPoint bindTo;

    switch(parentCornerToAnchor_)
    {
      default:
      case Qt::TopLeftCorner:
      bindTo = parentRect.topLeft(); break;
      case Qt::TopRightCorner:
      bindTo = parentRect.topRight(); break;
      case Qt::BottomLeftCorner:
      bindTo = parentRect.bottomLeft(); break;
      case Qt::BottomRightCorner:
      bindTo = parentRect.bottomRight(); break;
    }

    switch(widgetCornerToAnchor_)
    {
      default:
      case Qt::TopLeftCorner:
      widgetRect.moveTopLeft( bindTo ); break;
      case Qt::TopRightCorner:
      widgetRect.moveTopRight( bindTo ); break;
      case Qt::BottomLeftCorner:
      widgetRect.moveBottomLeft( bindTo ); break;
      case Qt::BottomRightCorner:
      widgetRect.moveBottomRight( bindTo ); break;
    }

    QDesktopWidget desktopWidget;
    // If user only has one screen, ensure the popup doesn't go off screen
    // If multiple screens, this could be annoying as the popup can be viewed on a 2nd screen
    if ( desktopWidget.screenCount() == 1)
      widgetRect = desktopWidget.screenGeometry( parent_ ).intersected( widgetRect );
    setGeometry( widgetRect );
  }
public slots:
  void hideRequest()
  {
    // Set time to hide to default length
    timeToWait__ = timeTillHide_;
    if ( isVisible() )
    {
      if ( ! isWidgetUnderCursorAChild() )
      {
        hide();
        if ( hideChildren )
        {
          // Hide children that are Popups
          // If we hide all children, floating widgets will be empty when they are shown again
          QRegExp matchAll(".*");
          QList< QWidget* > children = findChildren< QWidget* >( matchAll );

          for ( QList< QWidget*>::iterator it = children.begin();
              it != children.end(); ++it )
          {
            if ( ( *it ) -> windowType() == Qt::Popup )
            ( *it ) -> hide();
          }
          QList< SC_RelativePopup* > popupChildren = findChildren< SC_RelativePopup* >( matchAll );
          for ( QList< SC_RelativePopup* >::iterator it = popupChildren.begin();
              it != popupChildren.end(); ++it )
          {
            ( *it ) -> disableChildrenHiding();
          }
        }
      }
      else
      {
        timerTillHide_.start( timeToWait__ );
      }
    }
  }
  void fastHideRequest()
  {
    // set time to hide to the fast version
    timeToWait__ = timeFastHide_;
  }
protected:
  virtual void showEvent( QShowEvent* /* event */)
  {
    // Start waiting for the mouse
    if ( ! underMouse() )
    {
      if ( timerTillHide_.isActive() )
      timerTillHide_.stop();
      if ( timerTillFastHide_.isActive() )
      timerTillFastHide_.stop();
      timerTillHide_.start( timeTillHide_ );
    }
    calculateGeometry();
  }
  virtual void enterEvent( QEvent* /* event */)
  {
    if ( underMouse() )
    {
      // Have mouse so stop waiting to hide
      timerTillHide_.stop();
      // If we have the mouse for so long,
      // make the next hide quicker
      // most likely the user is done with the widget
      if ( timeToWait__ != timeFastHide_ )
      timerTillFastHide_.start( timeTillFastHide_ );
    }
  }
  virtual void leaveEvent( QEvent* /* event */)
  {
    timerTillHide_.start( timeToWait__ );
  }
  virtual void resizeEvent( QResizeEvent* event )
  {
    if ( event -> oldSize() != size() )
    {
      calculateGeometry();
    }
  }
};
