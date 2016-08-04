#include "sc_importWindow.hpp"

#include "simulationcraftqt.hpp"

BattleNetImportWindow::BattleNetImportWindow( SC_MainWindow* parent ) :
    QWidget( parent, Qt::Tool ),
    m_mainWindow( parent ),
    m_shortcut( new QShortcut( QKeySequence( "Ctrl+I" ), parent ) ),
    m_importWidget( new BattleNetImportWidget( this ) )
{
    setWindowTitle( tr( "Import a character" ) );

    m_shortcut -> setContext( Qt::ApplicationShortcut );
    connect( m_shortcut, SIGNAL( activated() ), this, SLOT( toggle() ) );
    connect( m_importWidget, SIGNAL( importTriggered( const QString&, const QString&, const QString&, const QString&) ),
             m_mainWindow, SLOT( startNewImport( const QString&, const QString&, const QString&, const QString& ) ) );
}

void BattleNetImportWindow::toggle()
{
    if ( isVisible() )
    {
        QWidget::hide();
    }
    else
    {
        QWidget::raise();
        QWidget::show();

        auto basePos = m_mainWindow -> geometry();
        move( basePos.center().x() - geometry().width() * .5,
              basePos.center().y() );
    }
}
