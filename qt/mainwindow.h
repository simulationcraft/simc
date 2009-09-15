#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QWidget>
#include <simcraft.h>

class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTabWidget;
class QWebView;

class MainWindow : public QWidget
{
    Q_OBJECT
    QTabWidget* tab_widget;
    QWebView* armory_view;
    QWebView* wowhead_view;
    QWebView* chardev_view;
    QWebView* warcrafter_view;
    QPlainTextEdit* script_text;
    QPlainTextEdit* overrides_text;
    QPlainTextEdit* log_text;
    QLineEdit* cmd_line;
    QPushButton* main_button;

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
};

#endif // MAINWINDOW_H
