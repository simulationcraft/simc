// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#pragma once
#include "config.hpp"

#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>

class QDir;
class QTreeWidget;

class SC_SampleProfilesTab : public QGroupBox
{
  Q_OBJECT
public:
  SC_SampleProfilesTab( QWidget* parent = nullptr );
  QTreeWidget* tree;

private:
  void fillTree( QDir );
};
