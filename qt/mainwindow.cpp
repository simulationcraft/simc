#include "mainwindow.h"
#include <QtGui>
#include <QtWebKit>

static QComboBox* create_choice( int count, ... )
{
    QComboBox* choice = new QComboBox();
    va_list vap;
    va_start( vap, count );
    for ( int i=0; i < count; i++ )
    {
      const char* s = ( char* ) va_arg( vap, char* );
      choice->addItem( s );
    }
    va_end( vap );
    return choice;
}

void MainWindow::updateProgress()
{
}

void MainWindow::mainTabChanged( int index )
{
    switch( index )
    {
        case 0: main_button->setText( "Go!"       ); break;
        case 1: main_button->setText( "Save!"     ); break;
        case 2: main_button->setText( "Import!"   ); break;
        case 3: main_button->setText( "Simulate!" ); break;
        case 4: main_button->setText( "Simulate!" ); break;
        case 5: main_button->setText( "Simulate!" ); break;
        case 6: main_button->setText( "Save!"     ); break;
    }
}

void MainWindow::importTabChanged( int index )
{
}

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout* cmd_line_layout = new QHBoxLayout();
    cmd_line_layout->addWidget( cmd_line = new QLineEdit() );
    cmd_line_layout->addWidget( progress_bar = new QProgressBar() );
    cmd_line_layout->addWidget( main_button = new QPushButton( "Go!" ) );
    progress_bar->setMaximum( 100 );
    progress_bar->setMaximumWidth( 100 );
    QGroupBox* cmd_line_groupbox = new QGroupBox();
    cmd_line_groupbox->setLayout( cmd_line_layout );

    main_tab = new QTabWidget();
    connect( main_tab, SIGNAL(currentChanged(int)), this, SLOT(mainTabChanged(int)) );

    QLabel* welcome_label = new QLabel( "\n  Welcome to SimulationCraft!\n\n  Help text will be here.\n" );
    welcome_label->setAlignment( Qt::AlignLeft|Qt::AlignTop );
    main_tab->addTab( welcome_label, "Welcome" );

    QFormLayout* globals_layout = new QFormLayout();
    globals_layout->addRow( "Region", region_choice = create_choice( 2, "US", "EU" ) );
    globals_layout->addRow( "Patch", patch_choice = create_choice( 2, "3.2.0", "3.2.2" ) );
    globals_layout->addRow( "Latency", latency_choice = create_choice( 2, "Low", "High" ) );
    globals_layout->addRow( "Iterations", iterations_choice = create_choice( 3, "100", "1000", "10000" ) );
    globals_layout->addRow( "Length (sec)", fight_length_choice = create_choice( 3, "100", "300", "500" ) );
    globals_layout->addRow( "Fight Style", fight_style_choice = create_choice( 2, "Patchwerk", "Helter Skelter" ) );
    globals_layout->addRow( "Scale Factors", scale_factors_choice = create_choice( 2, "No", "Yes" ) );
    globals_layout->addRow( "Threads", threads_choice = create_choice( 4, "1", "2", "4", "8" ) );
    iterations_choice->setCurrentIndex( 1 );
    fight_length_choice->setCurrentIndex( 1 );
    threads_choice->setCurrentIndex( 1 );
    QGroupBox* globals_groupbox = new QGroupBox( "Global Options" );
    globals_groupbox->setLayout( globals_layout );
    main_tab->addTab( globals_groupbox, "Globals" );

    import_tab = new QTabWidget();
    connect( import_tab, SIGNAL(currentChanged(int)), this, SLOT(importTabChanged(int)) );
    main_tab->addTab( import_tab, "Import" );

    armory_view = new QWebView();
    armory_view->setUrl( QUrl( "http://www.wowarmory.com" ) );
    import_tab->addTab( armory_view, "Armory" );

    wowhead_view = new QWebView();
    wowhead_view->setUrl( QUrl( "http://www.wowhead.com/?profiles" ) );
    import_tab->addTab( wowhead_view, "Wowhead" );

    chardev_view = new QWebView();
    chardev_view->setUrl( QUrl( "http://www.chardev.org" ) );
    import_tab->addTab( chardev_view, "CharDev" );

    warcrafter_view = new QWebView();
    warcrafter_view->setUrl( QUrl( "http://www.warcrafter.net" ) );
    import_tab->addTab( warcrafter_view, "Warcrafter" );

    simulate_text = new QPlainTextEdit();
    simulate_text->document()->setPlainText( "# Profiles will be downloaded into here.  Right-Click menu will Open/Save scripts." );
    main_tab->addTab( simulate_text, "Simulate" );

    overrides_text = new QPlainTextEdit();
    overrides_text->document()->setPlainText( "# Examples of all available global and player parms will shown here." );
    main_tab->addTab( overrides_text, "Overrides" );

    log_text = new QPlainTextEdit();
    log_text->setReadOnly(true);
    log_text->document()->setPlainText( "Standard-Out will end up here....." );
    main_tab->addTab( log_text, "Log" );

    results_tab = new QTabWidget();
    results_tab->setTabsClosable( true );
    main_tab->addTab( results_tab, "Results" );

    QVBoxLayout* v_layout = new QVBoxLayout();
    v_layout->addWidget( main_tab );
    v_layout->addWidget( cmd_line_groupbox );
    setLayout( v_layout );

    timer = new QTimer();
    connect( timer, SIGNAL(timeout()), this, SLOT(updateProgress()) );
}

MainWindow::~MainWindow()
{
    exit(0);
}
