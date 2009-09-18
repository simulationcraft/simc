// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SIMCRAFTQT_H
#define SIMCRAFTQT_H

#include <QtGui/QtGui>
#include <QtNetwork/QtNetwork>
#include <simcraft.h>

class QWebView;

class SimcraftWindow : public QWidget
{
    Q_OBJECT
    QTabWidget* mainTab;
    QComboBox* regionChoice;
    QComboBox* patchChoice;
    QComboBox* latencyChoice;
    QComboBox* iterationsChoice;
    QComboBox* fightLengthChoice;
    QComboBox* fightStyleChoice;
    QComboBox* scaleFactorsChoice;
    QComboBox* threadsChoice;
    QTabWidget* importTab;
    QWebView* armoryView;
    QWebView* wowheadView;
    QWebView* chardevView;
    QWebView* warcrafterView;
    QLineEdit* rawrFile;
    QPlainTextEdit* simulateText;
    QPlainTextEdit* overridesText;
    QPlainTextEdit* logText;
    QTabWidget* resultsTab;
    QLineEdit* cmdLine;
    QGroupBox* cmdLineGroupBox;
    QProgressBar* progressBar;
    QPushButton* mainButton;
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

    bool armoryImport    ( QString& profile );
    bool wowheadImport   ( QString& profile );
    bool chardevImport   ( QString& profile );
    bool warcrafterImport( QString& profile );
    bool rawrImport      ( QString& profile );

protected:
    virtual void closeEvent( QCloseEvent* e );

private slots:
    void updateProgress();
    void mainButtonClicked( bool checked=false );
    void mainTabChanged( int index );
    void importTabChanged( int index );

public:
    SimcraftWindow(QWidget *parent = 0);
};

#endif // SIMCRAFTQT_H
