#include "mainwindow.h"
#include <QtGui>
#include <QtWebKit>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    cmd_line = new QLineEdit();
    main_button = new QPushButton( "Go!" );
    QHBoxLayout* h_layout = new QHBoxLayout();
    h_layout->addWidget( cmd_line );
    h_layout->addWidget( main_button );
    QGroupBox* h_groupbox = new QGroupBox();
    h_groupbox->setLayout( h_layout );

    tab_widget = new QTabWidget();
    QLabel* welcome_label = new QLabel( "\n  Welcome to SimulationCraft!\n\n  Help text will be here.\n" );
    welcome_label->setAlignment( Qt::AlignLeft|Qt::AlignTop );
    tab_widget->addTab( welcome_label, "Welcome" );

    armory_view = new QWebView();
    armory_view->setUrl( QUrl( "http://www.wowarmory.com" ) );
    tab_widget->addTab( armory_view, "Armory" );

    wowhead_view = new QWebView();
    wowhead_view->setUrl( QUrl( "http://www.wowhead.com/?profiles" ) );
    tab_widget->addTab( wowhead_view, "Wowhead" );

    chardev_view = new QWebView();
    chardev_view->setUrl( QUrl( "http://www.chardev.org" ) );
    tab_widget->addTab( chardev_view, "CharDev" );

    warcrafter_view = new QWebView();
    warcrafter_view->setUrl( QUrl( "http://www.warcrafter.net" ) );
    tab_widget->addTab( warcrafter_view, "Warcrafter" );

    script_text = new QPlainTextEdit();
    script_text->document()->setPlainText( "# Profiles will be downloaded into here.  Right-Click menu will Open/Save scripts." );
    tab_widget->addTab( script_text, "Script" );

    overrides_text = new QPlainTextEdit();
    overrides_text->document()->setPlainText( "# Examples of all available global and player parms will shown here." );
    tab_widget->addTab( overrides_text, "Overrides" );

    log_text = new QPlainTextEdit();
    log_text->setReadOnly(true);
    log_text->document()->setPlainText( "Standard-Out will end up here....." );
    tab_widget->addTab( log_text, "Log" );

    QVBoxLayout* v_layout = new QVBoxLayout();
    v_layout->addWidget( tab_widget );
    v_layout->addWidget( h_groupbox );
    setLayout( v_layout );
}

MainWindow::~MainWindow()
{

}
