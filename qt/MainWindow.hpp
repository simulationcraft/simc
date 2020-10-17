// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include <QString>
#include <QtWidgets/QtWidgets>
#include <memory>
#include <string>

class BattleNetImportWindow;
class SC_MainTab;
class SC_ImportTab;
class SC_ResultTab;
class SC_WebView;
class SC_TextEdit;
class SC_QueueItemModel;
class SC_MainWindowCommandLine;
class SC_OptionsTab;
class SC_SpellQueryTab;
class SC_SimulateTab;
class SC_RecentlyClosedTabItemModel;
class SC_RecentlyClosedTabWidget;
class SC_ImportThread;
class SC_SimulateThread;
struct sim_t;

class SC_MainWindow : public QWidget
{
  Q_OBJECT
public:
  QWidget* customGearTab;
  QWidget* customTalentsTab;
  SC_MainTab* mainTab;
  SC_OptionsTab* optionsTab;
  SC_ImportTab* importTab;
  SC_SimulateTab* simulateTab;
  SC_ResultTab* resultsTab;
  SC_SpellQueryTab* spellQueryTab;
  QTabWidget* createCustomProfileDock;

  BattleNetImportWindow* newBattleNetView;
  SC_WebView* helpView;
  SC_WebView* visibleWebView;
  SC_TextEdit* overridesText;
  SC_TextEdit* logText;
  QPushButton* backButton;
  QPushButton* forwardButton;
  SC_MainWindowCommandLine* cmdLine;
  QLineEdit* apikey;
  QGroupBox* cmdLineGroupBox;
  QGroupBox* createCustomCharData;
  SC_RecentlyClosedTabItemModel* recentlyClosedTabModel;
  SC_RecentlyClosedTabWidget* recentlyClosedTabImport;
  QDesktopWidget desktopWidget;

  QTimer* timer;
  QTimer* soloChar;
  SC_ImportThread* importThread;
  SC_SimulateThread* simulateThread;

  QSignalMapper mainTabSignalMapper;
  QList<QShortcut*> shortcuts;

  std::shared_ptr<sim_t> sim;
  std::shared_ptr<sim_t> import_sim;
  std::string simPhase;
  std::string importSimPhase;
  int simProgress;
  int importSimProgress;
  int soloimport;
  int simResults;

  QString AppDataDir;      // output goes here
  QString ResultsDestDir;  // user documents dir, default location offered to persitently save reports
  QString TmpDir;          // application specific temporary dir

  QString cmdLineText;
  QString logFileText;
  QString resultsFileText;

  SC_QueueItemModel* simulationQueue;
  int consecutiveSimulationsRun;

  void startImport( int tab, const QString& url );
  bool importRunning();
  void startSim();
  bool simRunning();

  std::shared_ptr<sim_t> initSim();
  void deleteSim( std::shared_ptr<sim_t>& sim, SC_TextEdit* append_error_message = 0 );

  void loadHistory();
  void saveHistory();

  void createCmdLine();
  void createWelcomeTab();
  void createImportTab();
  void createOptionsTab();
  void createSimulateTab();
  void createResultsTab();
  void createOverridesTab();
  void createHelpTab();
  void createLogTab();
  void createSpellQueryTab();
  void createTabShortcuts();
  void createCustomTab();
  void updateWebView( SC_WebView* );

  void toggle_pause();

protected:
  void closeEvent( QCloseEvent* ) override;

private slots:
  void updatetimer();
  void itemWasEnqueuedTryToSim();
  void importFinished();
  void simulateFinished( std::shared_ptr<sim_t> );
  void updateSimProgress();
  void cmdLineReturnPressed();
  void cmdLineTextEdited( const QString& );
  void backButtonClicked( bool checked = false );
  void forwardButtonClicked( bool checked = false );
  void reloadButtonClicked( bool checked = false );
  void mainButtonClicked( bool checked = false );
  void pauseButtonClicked( bool checked = false );
  void cancelButtonClicked();
  void queueButtonClicked();
  void importButtonClicked();
  void queryButtonClicked();
  void mainTabChanged( int index );
  void importTabChanged( int index );
  void resultsTabChanged( int index );
  void resultsTabCloseRequest( int index );
  void bisDoubleClicked( QTreeWidgetItem* item, int col );
  void armoryRegionChanged( const QString& region );
  void simulateTabRestored( QWidget* tab, const QString& title, const QString& tooltip, const QIcon& icon );
  void switchToASubTab( int direction );
  void switchToLeftSubTab();
  void switchToRightSubTab();
  void currentlyViewedTabCloseRequest();

public slots:
  void enqueueSim();
  void saveLog();
  void saveResults();
  void stopImport();
  void stopSim();
  void stopAllSim();
  void startNewImport( const QString&, const QString&, const QString&, const QString& );

public:
  SC_MainWindow( QWidget* parent = nullptr );
};
