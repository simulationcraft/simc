// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "../qt/simulationcraftqt.hpp"

#ifdef QT_VERSION_5
#include <QtWidgets/QtWidgets>
#endif
#include <QtGui/QtGui>


class SC_MainWindowCommandLine : public QWidget
{
  Q_OBJECT
  Q_PROPERTY( state_e state READ currentState WRITE setState ) // Which page of the stacked layout are we on
public:
  enum state_e
  {
    IDLE = 0,
    SIMULATING,          // Simulating only one, nothing queued
    SIMULATING_MULTIPLE, // Simulating but others are queued
    SIMULATING_PAUSED,
    SIMULATING_MULTIPLE_PAUSED,
    STATE_COUNT
  };

  enum tabs_e // contains main_tabs_e then import_tabs_e
  {
    CMDLINE_TAB_WELCOME = 0,
    CMDLINE_TAB_IMPORT,
    CMDLINE_TAB_OPTIONS,
    CMDLINE_TAB_SIMULATE,
    CMDLINE_TAB_RESULTS,
    CMDLINE_TAB_OVERRIDES,
    CMDLINE_TAB_HELP,
    CMDLINE_TAB_LOG,
    CMDLINE_TAB_SPELLQUERY,
#ifdef SC_PAPERDOLL
    CMDLINE_TAB_PAPERDOLL,
#endif
    CMDLINE_TAB_BATTLE_NET,
    CMDLINE_TAB_BIS,
    CMDLINE_TAB_HISTORY,
    CMDLINE_TAB_RECENT,
    CMDLINE_TAB_AUTOMATION,
    CMDLINE_TAB_CUSTOM,
    CMDLINE_TAB_COUNT
  };

protected:
  QString text_simulate;
  QString text_queue;
  QString text_pause;
  QString text_resume;
  QString text_queue_tooltip;
  QString text_cancel;
  QString text_cancel_all;
  QString text_cancel_all_tooltip;
  QString text_save;
  QString text_import;
  QString text_spellquery;
  QString text_prev;
  QString text_next;
  QString text_prev_tooltip;
  QString text_next_tooltip;
  QString text_custom;
  QString text_hide_widget; // call hide() on the button, else call show()

  enum widgets_e
  {
    BUTTON_MAIN = 0,
    BUTTON_QUEUE,
    BUTTON_PREV,
    BUTTON_NEXT,
    TEXTEDIT_CMDLINE,
    PROGRESSBAR_WIDGET,
    BUTTON_PAUSE,
    WIDGET_COUNT
  };

  enum progressbar_states_e // Different states for progressbars
  {
    PROGRESSBAR_IGNORE = 0,
    PROGRESSBAR_IDLE,
    PROGRESSBAR_SIMULATING,
    PROGRESSBAR_IMPORTING,
    PROGRESSBAR_BATTLE_NET,
    PROGRESSBAR_HELP,
    PROGRESSBAR_STATE_COUNT
  };

  struct _widget_state
  {
    QString* text;
    QString* tool_tip;
    progressbar_states_e progressbar_state;
  };
  _widget_state states[STATE_COUNT][CMDLINE_TAB_COUNT][WIDGET_COUNT]; // all the states

  // ProgressBar state progress/format
  struct _progressbar_state
  {
    QString text;
    QString tool_tip;
    int progress;
  };
  _progressbar_state progressBarFormat[PROGRESSBAR_STATE_COUNT];

  // CommandLine buffers
  QString commandLineBuffer_DEFAULT; // different buffers for different tabs
  QString commandLineBuffer_TAB_BATTLE_NET;
  QString commandLineBuffer_TAB_RESULTS;
  QString commandLineBuffer_TAB_HELP;
  QString commandLineBuffer_TAB_LOG;

  QWidget* widgets[STATE_COUNT][WIDGET_COUNT]; // holds all widgets in all states

  QStackedLayout* statesStackedLayout; // Contains all states
  tabs_e current_tab;
  state_e current_state;

public:
  SC_MainWindowCommandLine( QWidget* parent = nullptr );
  state_e currentState() const;
  void setSimulatingProgress( int value, QString format, QString toolTip );
  int getSimulatingProgress();
  void setImportingProgress( int value, QString format, QString toolTip );
  int getImportingProgress();
  void setBattleNetLoadProgress( int value, QString format, QString toolTip );
  int getBattleNetProgress();
  void setHelpViewProgress( int value, QString format, QString toolTip );
  int getHelpViewProgress();
  void setSiteLoadProgress( int value, QString format, QString toolTip );
  int getSiteProgress();
  QString commandLineText();
  QString commandLineText( main_tabs_e tab );
  QString commandLineText( import_tabs_e tab );
  QString commandLineText( tabs_e tab );
  void setCommandLineText( QString text );
  void setCommandLineText( main_tabs_e tab, QString text );
  void setCommandLineText( import_tabs_e tab, QString text );
  void setCommandLineText( tabs_e tab, QString text );
  void togglePaused();
  void setPaused( bool pause );
  bool isPaused();
protected:
  void init();
  void initStateInfo();
  void initWidgetsToNull();
  void initTextStrings();
  void initStatesStructToNull();
  void initDefaultStates();
  void initImportStates();
  void initStackedLayout();
  void initLogResultsStates();
  void initProgressBarStates();
  void initCommandLineBuffers();
  void initPauseStates();
  virtual QWidget* createState( state_e state );
  virtual void createState_IDLE( QWidget* parent );
  virtual void createState_SIMULATING( QWidget* parent );
  virtual void createState_SIMULATING_PAUSED( QWidget* parent );
  virtual void createState_SIMULATING_MULTIPLE( QWidget* parent );
  virtual void createState_SIMULATING_MULTIPLE_PAUSED( QWidget* parent );
  virtual void _createState_SIMULATING( state_e state, QWidget* parent );
  virtual void createCommandLine( state_e state, QWidget* parent );
  bool tryToHideWidget( QString* text, QWidget* widget );
  void emitSignal( QString* text );
  // Accessors for widgets/displayable text; hide underlying implementation of states even from init()
  void setProgressBarProgress( progressbar_states_e state, int value );
  int getProgressBarProgressForState( progressbar_states_e state );
  void updateProgressBars();
  void adjustText( state_e state, tabs_e tab, widgets_e widget, QString text );
  void setText( state_e state, tabs_e tab, widgets_e widget, QString* text, QString* tooltip = nullptr );
  void setProgressBarState( state_e state, tabs_e tab, progressbar_states_e progressbar_state );
  progressbar_states_e getProgressBarStateForState( state_e state, tabs_e tab );
  void updateWidget( state_e state, tabs_e tab, widgets_e widget );
  QString* getText( state_e state, tabs_e tab, widgets_e widget );
  QString* getToolTip( state_e state, tabs_e tab, widgets_e widget );
  QWidget* getWidget( state_e state, widgets_e widget );
  void setWidget( state_e state, widgets_e widget, QWidget* widgetPointer );
  void setProgressBarFormat( progressbar_states_e state, QString format, bool update = false );
  void setProgressBarToolTip( progressbar_states_e state, QString toolTip, bool update = false );
  void updateProgress( progressbar_states_e state, int value, QString format, QString toolTip );
  tabs_e convertTabsEnum( main_tabs_e tab );
  tabs_e convertTabsEnum( import_tabs_e tab );
  // End accessors
public slots:
  void mainButtonClicked();
  void pauseButtonClicked();
  void queueButtonClicked();
  void setTab( main_tabs_e tab );
  void setTab( import_tabs_e tab );
  void setTab( tabs_e tab );
  void setState( state_e state );
  void commandLineTextEditedSlot( const QString& text );
signals:
  void pauseClicked();
  void resumeClicked();
  void simulateClicked();
  void queueClicked();
  void importClicked();
  void saveLogClicked();
  void saveResultsClicked();
  void cancelSimulationClicked();
  void cancelImportClicked();
  void cancelAllSimulationClicked();
  void backButtonClicked();
  void forwardButtonClicked();
// SC_CommandLine signals
  void switchToLeftSubTab();
  void switchToRightSubTab();
  void currentlyViewedTabCloseRequest();
  void commandLineReturnPressed();
  void commandLineTextEdited( const QString& );
  void queryClicked();
#ifdef SC_PAPERDOLL
  void simulatePaperdollClicked();
#endif
};
