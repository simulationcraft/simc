// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SC_IMPORTWINDOW_HPP
#define SC_IMPORTWINDOW_HPP

// Workaround for "combaseapi.h(229): error C2187: syntax error: 'identifier' was unexpected here" when using
// /permissive- Qt includes the 8.1 SDK which is unforunately not /permissive- compliant
#if defined( _WIN32 ) && !defined( _WIN32_WINNT_WINTHRESHOLD )
struct IUnknown;
#endif

#include <QDebug>
#include <QtWidgets/QShortcut>
#include <QtWidgets/QtWidgets>

class SC_MainWindow;

#include "sc_importWidget.hpp"

class BattleNetImportWindow : public QWidget
{
  Q_OBJECT

  // Components
  SC_MainWindow* m_mainWindow;
  QShortcut* m_shortcut;
  BattleNetImportWidget* m_importWidget;
  bool m_embedded;

public:
  BattleNetImportWindow( SC_MainWindow* parent, bool embedded );

  QSize sizeHint() const override
  {
    return m_importWidget->sizeHint();
  }

  BattleNetImportWidget* widget() const
  {
    return m_importWidget;
  }
public slots:
  void toggle();
};

#endif /* SC_IMPORTWINDOW_HPP */
