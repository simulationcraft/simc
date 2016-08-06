// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SC_IMPORTWINDOW_HPP
#define SC_IMPORTWINDOW_HPP

#include <QDebug>
#include <QtWidgets/QShortcut>
#include <QtWidgets/QtWidgets>

class SC_MainWindow;

#include "sc_importWidget.hpp"

class BattleNetImportWindow : public QWidget
{
    Q_OBJECT

    // Components
    SC_MainWindow*         m_mainWindow;
    QShortcut*             m_shortcut;
    BattleNetImportWidget* m_importWidget;

public:
    BattleNetImportWindow( SC_MainWindow* parent );

    QSize sizeHint() const override
    { return m_importWidget -> sizeHint(); }

    BattleNetImportWidget* widget() const
    { return m_importWidget; }
public slots:
    void toggle();
};

#endif /* SC_IMPORTWINDOW_HPP */
