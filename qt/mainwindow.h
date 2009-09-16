#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QWidget>
#include <simcraft.h>

class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QTabWidget;
class QTimer;
class QWebView;

class MainWindow : public QWidget
{
    Q_OBJECT
    QTabWidget* main_tab;
    QComboBox* region_choice;
    QComboBox* patch_choice;
    QComboBox* latency_choice;
    QComboBox* iterations_choice;
    QComboBox* fight_length_choice;
    QComboBox* fight_style_choice;
    QComboBox* scale_factors_choice;
    QComboBox* threads_choice;
    QTabWidget* import_tab;
    QWebView* armory_view;
    QWebView* wowhead_view;
    QWebView* chardev_view;
    QWebView* warcrafter_view;
    QLineEdit* rawr_file;
    QPlainTextEdit* simulate_text;
    QPlainTextEdit* overrides_text;
    QPlainTextEdit* log_text;
    QTabWidget* results_tab;
    QLineEdit* cmd_line;
    QProgressBar* progress_bar;
    QPushButton* main_button;
    QTimer* timer;

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
