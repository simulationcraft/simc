// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#pragma once
#include "config.hpp"

#include <QtWidgets/QTabWidget>

class SC_MainWindow;
class SC_ReforgeButtonGroup;
class QComboBox;
class QButtonGroup;
class QLineEdit;
class QListWidget;
class QFormLayout;

// ============================================================================
// SC_OptionsTabWidget
// ============================================================================

class SC_OptionsTab : public QTabWidget
{
  Q_OBJECT
public:
  SC_OptionsTab( SC_MainWindow* parent );

  void decodeOptions();
  void encodeOptions();

  QString get_db_order() const;
  QString get_globalSettings();
  QString getReportlDestination() const;
  QString mergeOptions();
  QString get_active_spec();
  QString get_player_role();
  QString get_api_key();
  QString auto_save_location;

  void createToolTips();
  QListWidget* itemDbOrder;
  struct choices_t
  {
    // options
    QComboBox* version;
    QComboBox* world_lag;
    QComboBox* target_error;
    QComboBox* iterations;
    QComboBox* fight_length;
    QComboBox* fight_variance;
    QComboBox* fight_style;
    QComboBox* target_level;
    QComboBox* target_race;
    QComboBox* num_target;
    QComboBox* player_skill;
    QComboBox* threads;
    QComboBox* process_priority;
    QComboBox* auto_save;
    QComboBox* pvp_crit;
    QComboBox* armory_region;
    QComboBox* armory_spec;
    QComboBox* default_role;
    QComboBox* gui_localization;
    QComboBox* update_check;
    QComboBox* boss_type;
    QComboBox* tank_dummy;
    QComboBox* tmi_boss;
    QComboBox* tmi_window;
    QComboBox* show_etmi;
    QComboBox* debug;
    QComboBox* report_pets;
    QComboBox* statistics_level;
    QComboBox* deterministic_rng;
    QComboBox* center_scale_delta;
    QComboBox* challenge_mode;
    // scaling
    QComboBox* scale_over;
    QComboBox* plots_points;
    QComboBox* plots_step;
    QComboBox* plots_target_error;
    QComboBox* plots_iterations;
    QComboBox* reforgeplot_amount;
    QComboBox* reforgeplot_step;
  } choice;

  QButtonGroup* buffsButtonGroup;
  QButtonGroup* debuffsButtonGroup;
  QButtonGroup* scalingButtonGroup;
  QButtonGroup* plotsButtonGroup;
  SC_ReforgeButtonGroup* reforgeplotsButtonGroup;
  QLineEdit* api_client_id, * api_client_secret;
public slots:
  void _resetallSettings();
  void _savefilelocation();
  void _armoryRegionChanged( const QString& );
protected:
  SC_MainWindow* mainWindow;
  void createGlobalsTab();
  void createBuffsDebuffsTab();
  void createScalingTab();
  void createPlotsTab();
  void createReforgePlotsTab();
  void createItemDataSourceSelector( QFormLayout* );
  QComboBox* addValidatorToComboBox( int lowerBound, int upperBound, QComboBox* );
  void toggleInterdependentOptions();

private slots:
  void allBuffsChanged( bool checked );
  void allDebuffsChanged( bool checked );
  void allScalingChanged( bool checked );
  void _optionsChanged();
signals:
  void armory_region_changed( const QString& );
  void optionsChanged(); // FIXME: hookup to everything
};
