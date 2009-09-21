// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SIMCRAFTQT_H
#define SIMCRAFTQT_H

#include <QtGui/QtGui>
#include <QtNetwork/QtNetwork>
#include <simcraft.h>

#define TAB_WELCOME   0
#define TAB_GLOBALS   1
#define TAB_IMPORT    2
#define TAB_SIMULATE  3
#define TAB_OVERRIDES 4
#define TAB_LOG       5
#define TAB_RESULTS   6
#define TAB_HELP      7

#define TAB_ARMORY_US  0
#define TAB_ARMORY_EU  1
#define TAB_WOWHEAD    2
#define TAB_CHARDEV    3
#define TAB_WARCRAFTER 4
#define TAB_RAWR       5

class QWebView;

class SimcraftWindow : public QWidget
{
    Q_OBJECT
    QTabWidget* mainTab;
    QComboBox* patchChoice;
    QComboBox* latencyChoice;
    QComboBox* iterationsChoice;
    QComboBox* fightLengthChoice;
    QComboBox* fightStyleChoice;
    QComboBox* scaleFactorsChoice;
    QComboBox* threadsChoice;
    QTabWidget* importTab;
    QWebView* armoryUsView;
    QWebView* armoryEuView;
    QWebView* wowheadView;
    QWebView* chardevView;
    QWebView* warcrafterView;
    QLineEdit* rawrFile;
    QListWidget* historyList;
    QPlainTextEdit* simulateText;
    QPlainTextEdit* overridesText;
    QPlainTextEdit* logText;
    QTabWidget* resultsTab;
    QPushButton* backButton;
    QPushButton* forwardButton;
    QLineEdit* cmdLine;
    QProgressBar* progressBar;
    QPushButton* mainButton;
    QGroupBox* cmdLineGroupBox;
    QTimer* timer;

    sim_t* sim;
    int numResults;
    
    void createCmdLine();
    void createWelcomeTab();
    void createGlobalsTab();
    void createImportTab();
    void createSimulateTab();
    void createOverridesTab();
    void createLogTab();
    void createResultsTab();

    void addHistoryItem( const QString& name, const QString& origin );

    bool armoryUsImport  ( QString& profile );
    bool armoryEuImport  ( QString& profile );
    bool wowheadImport   ( QString& profile );
    bool chardevImport   ( QString& profile );
    bool warcrafterImport( QString& profile );
    bool rawrImport      ( QString& profile );

protected:
    virtual void closeEvent( QCloseEvent* e );

private slots:
    void updateProgress();
    void backButtonClicked( bool checked=false );
    void forwardButtonClicked( bool checked=false );
    void mainButtonClicked( bool checked=false );
    void mainTabChanged( int index );
    void historyDoubleClicked( QListWidgetItem* item );

public:
    SimcraftWindow(QWidget *parent = 0);
};

#endif // SIMCRAFTQT_H
