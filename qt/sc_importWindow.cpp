#include "sc_importWindow.hpp"

#include "sc_OptionsTab.hpp"
#include "simulationcraftqt.hpp"

BattleNetImportWindow::BattleNetImportWindow( SC_MainWindow* parent, bool embedded )
  : QWidget( parent, Qt::Tool ),
    m_mainWindow( parent ),
    m_shortcut( embedded ? nullptr : new QShortcut( QKeySequence( "Ctrl+I" ), parent ) ),
    m_importWidget( new BattleNetImportWidget( this ) ),
    m_embedded( embedded )
{
  if ( !m_embedded )
  {
    setWindowTitle( tr( "Import a character" ) );

    m_shortcut->setContext( Qt::ApplicationShortcut );
    connect( m_shortcut, SIGNAL( activated() ), this, SLOT( toggle() ) );
    connect( m_mainWindow->optionsTab, SIGNAL( armory_region_changed( const QString& ) ), m_importWidget,
             SLOT( armoryRegionChangedIn( const QString& ) ) );
  }

  connect( m_importWidget,
           SIGNAL( importTriggeredOut( const QString&, const QString&, const QString&, const QString& ) ), m_mainWindow,
           SLOT( startNewImport( const QString&, const QString&, const QString&, const QString& ) ) );
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
    activateWindow();

    auto basePos = m_mainWindow->geometry();
    move( basePos.center().x() - geometry().width() * .5, basePos.center().y() );
  }
}
