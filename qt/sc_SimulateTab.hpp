// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#pragma once

#include "util/sc_recentlyclosed.hpp"

class SC_TextEdit;

const QString defaultSimulateText(
    "# Profile will be downloaded into a new tab.\n"
    "#\n"
    "# Clicking Simulate will create a simc_gui.simc profile for review.\n" );

const QString defaultSimulateTabTitle( "Simulate" );

class SC_SimulateTab : public SC_RecentlyClosedTab
{
  Q_OBJECT
  QWidget* addTabWidget;
  int lastSimulateTabIndexOffset;

public:
  SC_SimulateTab( QWidget* parent = 0, SC_RecentlyClosedTabItemModel* modelToUse = 0 );
  bool is_Default_New_Tab( int index );
  bool contains_Only_Default_Profiles();
  int add_Text( const QString& text, const QString& tab_name );
  SC_TextEdit* current_Text();
  void set_Text( const QString& text );
  void append_Text( const QString& text );
  void close_All_Tabs();
  int insertAt() override;
  bool wrapToEnd();
  bool wrapToBeginning();
  void insertNewTabAt( int index = -1, const QString& text = defaultSimulateText,
                       const QString& title = defaultSimulateTabTitle );

protected:
  void keyPressEvent( QKeyEvent* e ) override;
  void keyReleaseEvent( QKeyEvent* e ) override;
  void showEvent( QShowEvent* e ) override;

public slots:
  void TabCloseRequest( int index );
  void addNewTab( int index );
  void mouseDragSwitchTab( int tab );
  void enforceAddTabWidgetLocationInvariant();
};
