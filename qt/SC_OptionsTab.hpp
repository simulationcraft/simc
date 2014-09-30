#ifndef SC_OPTIONSTAB_HPP
#define SC_OPTIONSTAB_HPP

#include <QtWidgets/QtWidgets>

class SC_MainWindow;
class SC_ReforgeButtonGroup;

// ============================================================================
// SC_OptionsTabWidget
// ============================================================================

class SC_OptionsTab : public QTabWidget
{
  Q_OBJECT
public:
  SC_OptionsTab( SC_MainWindow* parent );

  void    decodeOptions();
  void    encodeOptions();
  QString get_db_order() const;
  QString get_globalSettings();
  QString mergeOptions();
  QString get_active_spec();
  QString get_player_role();

  void createToolTips();
  QListWidget* itemDbOrder;
  struct choices_t
  {
    // options
    QComboBox* version;
    QComboBox* world_lag;
    QComboBox* iterations;
    QComboBox* fight_length;
    QComboBox* fight_variance;
    QComboBox* fight_style;
    QComboBox* target_level;
    QComboBox* target_race;
    QComboBox* num_target;
    QComboBox* player_skill;
    QComboBox* threads;
    QComboBox* thread_priority;
    QComboBox* armory_region;
    QComboBox* armory_spec;
    QComboBox* default_role;
    QComboBox* boss_type;
    QComboBox* tank_dummy;
    QComboBox* tmi_boss;
    QComboBox* tmi_window;
    QComboBox* show_etmi;
    QComboBox* debug;
    QComboBox* report_pets;
    QComboBox* print_style;
    QComboBox* statistics_level;
    QComboBox* deterministic_rng;
    QComboBox* center_scale_delta;
    QComboBox* challenge_mode;
    // scaling
    QComboBox* scale_over;
    QComboBox* plots_points;
    QComboBox* plots_step;
    QComboBox* reforgeplot_amount;
    QComboBox* reforgeplot_step;
  } choice;

  QButtonGroup* buffsButtonGroup;
  QButtonGroup* debuffsButtonGroup;
  QButtonGroup* scalingButtonGroup;
  QButtonGroup* plotsButtonGroup;
  SC_ReforgeButtonGroup* reforgeplotsButtonGroup;
public slots:
  void _resetallSettings();
protected:
  SC_MainWindow* mainWindow;
  void createGlobalsTab();
  void createBuffsDebuffsTab();
  void createScalingTab();
  void createPlotsTab();
  void createReforgePlotsTab();
  void createItemDataSourceSelector( QFormLayout* );
  QComboBox* addValidatorToComboBox( int lowerBound, int upperBound, QComboBox* );

private slots:
  void allBuffsChanged( bool checked );
  void allDebuffsChanged( bool checked );
  void allScalingChanged( bool checked );
  void _optionsChanged();
signals:
  void armory_region_changed( const QString& );
  void optionsChanged(); // FIXME: hookup to everything

};

#endif // SC_OPTIONSTAB_HPP
