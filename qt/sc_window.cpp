// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "MainWindow.hpp"
#include "WebView.hpp"
#include "dbc/spell_query/spell_data_expr.hpp"
#include "interfaces/sc_http.hpp"
#include "util/git_info.hpp"
#include "sc_AddonImportTab.hpp"
#include "sc_OptionsTab.hpp"
#include "sc_SampleProfilesTab.hpp"
#include "sc_SimulateTab.hpp"
#include "sc_SimulationThread.hpp"
#include "sc_SpellQueryTab.hpp"
#include "sc_WelcomeTab.hpp"
#include "simulationcraftqt.hpp"
#include "util/sc_mainwindowcommandline.hpp"

#if defined( Q_OS_MAC )
#include <CoreFoundation/CoreFoundation.h>
#endif
#include <QDateTime>
#include <QStandardPaths>
#include <memory>

namespace
{  // UNNAMED NAMESPACE

struct HtmlOutputFunctor
{
  QString fname;

  HtmlOutputFunctor( const QString& fn ) : fname( fn )
  {
  }

  void operator()( QString htmlOutput )
  {
    QFile file( fname );
    if ( file.open( QIODevice::WriteOnly | QIODevice::Text ) )
    {
      QByteArray out_utf8 = htmlOutput.toUtf8();
      qint64 ret          = file.write( out_utf8 );
      file.close();
      assert( ret == htmlOutput.size() );
      (void)ret;
    }
  }
};
}  // UNNAMED NAMESPACE

// ==========================================================================
// SC_PATHS
// ==========================================================================

QStringList SC_PATHS::getDataPaths()
{
#if defined( Q_OS_WIN )
  return QStringList( QCoreApplication::applicationDirPath() );
#elif defined( Q_OS_MAC )
  return QStringList( QCoreApplication::applicationDirPath() + "/../Resources" );
#else
#if !defined( SC_TO_INSTALL )
  return QStringList( QCoreApplication::applicationDirPath() );
#else
  QStringList shared_paths;
  QStringList appdatalocation = QStandardPaths::standardLocations( QStandardPaths::DataLocation );
  for ( int i = 0; i < appdatalocation.size(); ++i )
  {
    QDir dir( appdatalocation[ i ] );
    if ( dir.exists() )
    {
      shared_paths.append( dir.path() );
    }
  }
  return shared_paths;
#endif
#endif
}

// ============================================================================
// SC_CommandLine
// ============================================================================

SC_CommandLine::SC_CommandLine( QWidget* parent ) : QLineEdit( parent )
{
  QShortcut* altUp    = new QShortcut( QKeySequence( Qt::ALT + Qt::Key_Up ), this );
  QShortcut* altLeft  = new QShortcut( QKeySequence( Qt::ALT + Qt::Key_Left ), this );
  QShortcut* altDown  = new QShortcut( QKeySequence( Qt::ALT + Qt::Key_Down ), this );
  QShortcut* altRight = new QShortcut( QKeySequence( Qt::ALT + Qt::Key_Right ), this );
  QShortcut* ctrlDel  = new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_Delete ), this );

  connect( altUp, SIGNAL( activated() ), this, SIGNAL( switchToLeftSubTab() ) );
  connect( altLeft, SIGNAL( activated() ), this, SIGNAL( switchToLeftSubTab() ) );
  connect( altDown, SIGNAL( activated() ), this, SIGNAL( switchToRightSubTab() ) );
  connect( altRight, SIGNAL( activated() ), this, SIGNAL( switchToRightSubTab() ) );
  connect( ctrlDel, SIGNAL( activated() ), this, SIGNAL( currentlyViewedTabCloseRequest() ) );
}

// ==========================================================================
// SC_ReforgeButtonGroup
// ==========================================================================

SC_ReforgeButtonGroup::SC_ReforgeButtonGroup( QObject* parent ) : QButtonGroup( parent ), selected( 0 )
{
  connect( this, SIGNAL( buttonToggled( int, bool ) ), this, SLOT( setSelected( int, bool ) ) );
}

void SC_ReforgeButtonGroup::setSelected( int id, bool checked )
{
  Q_UNUSED( id );
  if ( checked )
    selected++;
  else
    selected--;

  // Three selected, disallow selection of any more
  if ( selected >= 3 )
  {
    foreach ( QAbstractButton* button, buttons() )
    {
      button->setEnabled( button->isChecked() );
    }
  }
  // Less than three selected, allow selection of all/any
  else
  {
    foreach ( QAbstractButton* button, buttons() )
    {
      button->setEnabled( true );
    }
  }
}

void SC_SingleResultTab::save_result()
{
  QString destination;
  QString defaultDestination;
  QString extension;
  switch ( currentTab() )
  {
    case TAB_HTML:
      defaultDestination = "results_html.html";
      extension          = "html";
      break;
    case TAB_TEXT:
      defaultDestination = "results_text.txt";
      extension          = "txt";
      break;
    case TAB_JSON:
      defaultDestination = "results_json.json";
      extension          = "json";
      break;
    case TAB_PLOTDATA:
      defaultDestination = "results_plotdata.csv";
      extension          = "csv";
      break;
  }
  destination             = defaultDestination;
  QString savePath        = mainWindow->ResultsDestDir;
  QString commandLinePath = mainWindow->cmdLine->commandLineText( TAB_RESULTS );
  int fname_offset        = commandLinePath.lastIndexOf( QDir::separator() );

  if ( commandLinePath.size() > 0 )
  {
    if ( fname_offset == -1 )
      destination = commandLinePath;
    else
    {
      savePath    = commandLinePath.left( fname_offset + 1 );
      destination = commandLinePath.right( commandLinePath.size() - ( fname_offset + 1 ) );
    }
  }
  if ( destination.size() == 0 )
  {
    destination = defaultDestination;
  }
  if ( destination.indexOf( "." + extension ) == -1 )
    destination += "." + extension;
  QFileInfo savePathInfo( savePath );
  QFileDialog f( this );
  f.setDirectory( savePathInfo.absoluteDir() );
  f.setAcceptMode( QFileDialog::AcceptSave );
  f.setDefaultSuffix( extension );
  f.selectFile( destination );
  f.setWindowTitle( tr( "Save results" ) );

  if ( f.exec() )
  {
    switch ( currentTab() )
    {
      case TAB_HTML:
      {
        QFile file( f.selectedFiles().at( 0 ) );
        if ( file.open( QIODevice::WriteOnly | QIODevice::Text ) )
        {
          file.write( debug_cast<SC_WebView*>( currentWidget() )->out_html );
          file.close();
        }
        break;
      }
      case TAB_TEXT:
        break;
      case TAB_JSON:
        break;
      case TAB_PLOTDATA:
        break;
    }
    QMessageBox::information( this, tr( "Save Result" ), tr( "Result saved to %1" ).arg( f.selectedFiles().at( 0 ) ),
                              QMessageBox::Ok, QMessageBox::Ok );
    mainWindow->logText->appendPlainText( QString( tr( "Results saved to: %1\n" ) ).arg( f.selectedFiles().at( 0 ) ) );
  }
}

void SC_SingleResultTab::TabChanged( int /* index */ )
{
}

SC_ResultTab::SC_ResultTab( SC_MainWindow* mw ) : SC_RecentlyClosedTab( mw ), mainWindow( mw )
{
  setTabsClosable( true );
  setMovable( true );
  enableDragHoveredOverTabSignal( true );
  connect( this, SIGNAL( mouseDragHoveredOverTab( int ) ), this, SLOT( setCurrentIndex( int ) ) );
}
