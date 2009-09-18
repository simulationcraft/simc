#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>
#include <simcraft.h>

class QWebView;

class MainWindow : public QWidget
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

    void createCmdLine();
    void createWelcomeTab();
    void createGlobalsTab();
    void createImportTab();
    void createSimulateTab();
    void createOverridesTab();
    void createLogTab();
    void createResultsTab();

protected:
    virtual void closeEvent( QCloseEvent* e );

private slots:
    void updateProgress();
    void mainButtonClicked( bool checked=false );
    void mainTabChanged( int index );
    void importTabChanged( int index );

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
};

#endif // MAINWINDOW_H
