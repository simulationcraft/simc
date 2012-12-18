// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "simulationcraftqt.hpp"
#ifdef SC_PAPERDOLL
#include "simcpaperdoll.hpp"
#endif
#include <QtWebKit/QtWebKit>
#ifdef Q_WS_MAC
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace { // UNNAMED NAMESPACE

const char* SIMC_HISTORY_FILE = "simc_history.dat";
const char* SIMC_LOG_FILE = "simc_log.txt";

// ==========================================================================
// Utilities
// ==========================================================================

struct OptionEntry
{
  const char* label;
  const char* option;
  const char* tooltip;
};

const OptionEntry buffOptions[] =
{
  { "Toggle All Buffs",             "",                                 "Toggle all buffs on/off"                         },
  { "Attack Power Multiplier",      "override.attack_power_multiplier", "+10% Attack Power Multiplier"                    },
  { "Attack Speed",                 "override.attack_haste",            "+5% Attack Speed"                                },
  { "Spell Power Multiplier",       "override.spell_power_multiplier",  "+10% Spell Power Multiplier"                     },
  { "Spell Haste",                  "override.spell_haste",             "+5% Spell Haste"                                 },

  { "Critical Strike",              "override.critical_strike",         "+5% Melee/Ranged/Spell Critical Strike Chance"   },
  { "Mastery",                      "override.mastery",                 "+5 Mastery"                                      },

  { "Stamina",                      "override.stamina",                 "+10% Stamina"                                    },
  { "Strength, Agility, Intellect", "override.str_agi_int",             "+5% Strength, Agility, Intellect"                },

  { "Bloodlust",                    "override.bloodlust",               "Ancient Hysteria\nBloodlust\nHeroism\nTime Warp" },
  { NULL, NULL, NULL }
};

const OptionEntry itemSourceOptions[] =
{
  { "Local Item Database", "local",   "Use Simulationcraft item database" },
  { "Blizzard API",        "bcpapi",  "Remote Blizzard Community Platform API source" },
  { "Wowhead.com",         "wowhead", "Remote Wowhead.com item data source" },
  { "Wowhead.com (PTR)",   "ptrhead", "Remote Wowhead.com PTR item data source" },
};

const OptionEntry debuffOptions[] =
{
  { "Toggle All Debuffs",     "",                                "Toggle all debuffs on/off"      },

  { "Bleeding",               "override.bleeding",               "Rip\nRupture\nPiercing Shots"   },

  { "Physical Vulnerability", "override.physical_vulnerability", "Physical Vulnerability (+4%)"   },
  { "Ranged Vulnerability",   "override.ranged_vulnerability",   "Ranged Vulnerability (+5%)"     },
  { "Magic Vulnerability",    "override.magic_vulnerability",    "Magic Vulnerability (+5%)"      },

  { "Weakened Armor",         "override.weakened_armor",         "Weakened Armor (-4% per stack)" },
  { "Weakened Blows",         "override.weakened_blows",         "Weakened Blows (-10%)"          },

  { NULL, NULL, NULL }
};

const OptionEntry scalingOptions[] =
{
  { "Analyze All Stats",                "",         "Scale factors are necessary for gear ranking.\nThey only require an additional simulation for each RELEVANT stat." },
  {
    "Use Positive Deltas Only",         "",         "Normally Hit/Expertise use negative scale factors to show DPS lost by reducing that stat.\n"
    "This option forces a positive scale delta, which is useful for classes with soft caps."
  },
  { "Analyze Strength",                 "str",      "Calculate scale factors for Strength"                 },
  { "Analyze Agility",                  "agi",      "Calculate scale factors for Agility"                  },
  { "Analyze Stamina",                  "sta",      "Calculate scale factors for Stamina"                  },
  { "Analyze Intellect",                "int",      "Calculate scale factors for Intellect"                },
  { "Analyze Spirit",                   "spi",      "Calculate scale factors for Spirit"                   },
  { "Analyze Spell Power",              "sp",       "Calculate scale factors for Spell Power"              },
  { "Analyze Attack Power",             "ap",       "Calculate scale factors for Attack Power"             },
  { "Analyze Expertise Rating",         "exp",      "Calculate scale factors for Expertise Rating"         },
  { "Analyze Hit Rating",               "hit",      "Calculate scale factors for Hit Rating"               },
  { "Analyze Crit Rating",              "crit",     "Calculate scale factors for Crit Rating"              },
  { "Analyze Haste Rating",             "haste",    "Calculate scale factors for Haste Rating"             },
  { "Analyze Mastery Rating",           "mastery",  "Calculate scale factors for Mastery Rating"           },
  { "Analyze Weapon DPS",               "wdps",     "Calculate scale factors for Weapon DPS"               },
  { "Analyze Weapon Speed",             "wspeed",   "Calculate scale factors for Weapon Speed"             },
  { "Analyze Off-hand Weapon DPS",      "wohdps",   "Calculate scale factors for Off-hand Weapon DPS"      },
  { "Analyze Off-hand Weapon Speed",    "wohspeed", "Calculate scale factors for Off-hand Weapon Speed"    },
  { "Analyze Armor",                    "armor",    "Calculate scale factors for Armor"                    },
  { "Analyze Latency",                  "",         "Calculate scale factors for Latency"                  },
  { NULL, NULL, NULL }
};

const OptionEntry plotOptions[] =
{
  { "Plot DPS per Strength",                 "str",     "Generate DPS curve for Strength"                 },
  { "Plot DPS per Agility",                  "agi",     "Generate DPS curve for Agility"                  },
  { "Plot DPS per Stamina",                  "sta",     "Generate DPS curve for Stamina"                  },
  { "Plot DPS per Intellect",                "int",     "Generate DPS curve for Intellect"                },
  { "Plot DPS per Spirit",                   "spi",     "Generate DPS curve for Spirit"                   },
  { "Plot DPS per Spell Power",              "sp",      "Generate DPS curve for Spell Power"              },
  { "Plot DPS per Attack Power",             "ap",      "Generate DPS curve for Attack Power"             },
  { "Plot DPS per Expertise Rating",         "exp",     "Generate DPS curve for Expertise Rating"         },
  { "Plot DPS per Hit Rating",               "hit",     "Generate DPS curve for Hit Rating"               },
  { "Plot DPS per Crit Rating",              "crit",    "Generate DPS curve for Crit Rating"              },
  { "Plot DPS per Haste Rating",             "haste",   "Generate DPS curve for Haste Rating"             },
  { "Plot DPS per Mastery Rating",           "mastery", "Generate DPS curve for Mastery Rating"           },
  { "Plot DPS per Weapon DPS",               "wdps",    "Generate DPS curve for Weapon DPS"               },
  { NULL, NULL, NULL }
};

const OptionEntry reforgePlotOptions[] =
{
  { "Plot Reforge Options for Spirit",            "spi",     "Generate reforge plot data for Spirit"           },
  { "Plot Reforge Options for Expertise Rating",  "exp",     "Generate reforge plot data for Expertise Rating" },
  { "Plot Reforge Options for Hit Rating",        "hit",     "Generate reforge plot data for Hit Rating"       },
  { "Plot Reforge Options for Crit Rating",       "crit",    "Generate reforge plot data for Crit Rating"      },
  { "Plot Reforge Options for Haste Rating",      "haste",   "Generate reforge plot data for Haste Rating"     },
  { "Plot Reforge Options for Mastery Rating",    "mastery", "Generate reforge plot data for Mastery Rating"   },

  { "Plot Reforge Options for Strength",    "str", "Generate reforge plot data for Intellect"   },
  { "Plot Reforge Options for Agility",    "agi", "Generate reforge plot data for Agility"   },
  { "Plot Reforge Options for Stamina",    "sta", "Generate reforge plot data for Stamina"   },
  { "Plot Reforge Options for Intellect",    "int", "Generate reforge plot data for Intellect"   },
  { NULL, NULL, NULL }
};

const QString defaultSimulateText( "# Profile will be downloaded into here.\n"
                                   "# Use the Back/Forward buttons to cycle through the script history.\n"
                                   "# Use the Up/Down arrow keys to cycle through the command-line history.\n"
                                   "#\n"
                                   "# Clicking Simulate will create a simc_gui.simc profile for review.\n" );

QComboBox* createChoice( int count, ... )
{
  QComboBox* choice = new QComboBox();
  va_list vap;
  va_start( vap, count );
  for ( int i = 0; i < count; i++ )
    choice -> addItem( va_arg( vap, char* ) );
  va_end( vap );
  return choice;
}

} // UNNAMED NAMESPACE

// ==========================================================================
// SC_MainWindow
// ==========================================================================

// Decode all options/setting from a string ( loaded from the history ).
// Decode / Encode order needs to be equal!

void SC_MainWindow::decodeOptions( QString encoding )
{
  int i = 0;
  QStringList tokens = encoding.split( ' ' );

  if ( i < tokens.count() )
    choice.version -> setCurrentIndex( tokens[ i++ ].toInt() );
  if ( i < tokens.count() )
    choice.iterations -> setCurrentIndex( tokens[ i++ ].toInt() );
  if ( i < tokens.count() )
    choice.fight_length -> setCurrentIndex( tokens[ i++ ].toInt() );
  if ( i < tokens.count() )
    choice.fight_variance -> setCurrentIndex( tokens[ i++ ].toInt() );
  if ( i < tokens.count() )
    choice.fight_style -> setCurrentIndex( tokens[ i++ ].toInt() );
  if ( i < tokens.count() )
    choice.target_race -> setCurrentIndex( tokens[ i++ ].toInt() );
  if ( i < tokens.count() )
    choice.num_target -> setCurrentIndex( tokens[ i++ ].toInt() );
  if ( i < tokens.count() )
    choice.player_skill -> setCurrentIndex( tokens[ i++ ].toInt() );
  if ( i < tokens.count() )
    choice.threads -> setCurrentIndex( tokens[ i++ ].toInt() );
  if ( i < tokens.count() )
    choice.armory_region -> setCurrentIndex( tokens[ i++ ].toInt() );
  if ( i < tokens.count() )
    choice.armory_spec -> setCurrentIndex( tokens[ i++ ].toInt() );
  if ( i < tokens.count() )
    choice.default_role -> setCurrentIndex( tokens[ i++ ].toInt() );
  if ( i < tokens.count() )
    choice.world_lag -> setCurrentIndex( tokens[ i++ ].toInt() );
  if ( i < tokens.count() )
    choice.target_level -> setCurrentIndex( tokens[ i++ ].toInt() );
  if ( i < tokens.count() )
    choice.aura_delay -> setCurrentIndex( tokens[ i++ ].toInt() );
  if ( i < tokens.count() )
    choice.report_pets -> setCurrentIndex( tokens[ i++ ].toInt() );
  if ( i < tokens.count() )
    choice.print_style -> setCurrentIndex( tokens[ i++ ].toInt() );
  if ( i < tokens.count() )
    choice.statistics_level -> setCurrentIndex( tokens[ i++ ].toInt() );
  if ( i < tokens.count() )
    choice.deterministic_rng -> setCurrentIndex( tokens[ i++ ].toInt() );
  if ( i < tokens.count() )
    choice.center_scale_delta -> setCurrentIndex( tokens[ i++ ].toInt() );
  if ( i < tokens.count() )
    choice.scale_over -> setCurrentIndex( tokens[ i++ ].toInt() );
  if ( i < tokens.count() )
    choice.challenge_mode -> setCurrentIndex( tokens[ i++ ].toInt() );

  QList<QAbstractButton*>        buff_buttons =        buffsButtonGroup -> buttons();
  QList<QAbstractButton*>      debuff_buttons =      debuffsButtonGroup -> buttons();
  QList<QAbstractButton*>     scaling_buttons =      scalingButtonGroup -> buttons();
  QList<QAbstractButton*>        plot_buttons =        plotsButtonGroup -> buttons();
  QList<QAbstractButton*> reforgeplot_buttons = reforgeplotsButtonGroup -> buttons();

  for ( ; i < tokens.count(); i++ )
  {
    QStringList opt_tokens = tokens[ i ].split( ':' );

    const OptionEntry* options=0;
    QList<QAbstractButton*>* buttons=0;

    if (      ! opt_tokens[ 0 ].compare( "buff"           ) ) { options = buffOptions;        buttons = &buff_buttons;        }
    else if ( ! opt_tokens[ 0 ].compare( "debuff"         ) ) { options = debuffOptions;      buttons = &debuff_buttons;      }
    else if ( ! opt_tokens[ 0 ].compare( "scaling"        ) ) { options = scalingOptions;     buttons = &scaling_buttons;     }
    else if ( ! opt_tokens[ 0 ].compare( "plots"          ) ) { options = plotOptions;        buttons = &plot_buttons;        }
    else if ( ! opt_tokens[ 0 ].compare( "reforge_plots"  ) ) { options = reforgePlotOptions; buttons = &reforgeplot_buttons; }
    else if ( ! opt_tokens[ 0 ].compare( "item_db_source" ) )
    {
      QStringList item_db_list = opt_tokens[ 1 ].split( '/' );

      for ( int opt = item_db_list.size(); --opt >= 0; )
      {
        for ( int source = 0; source < itemDbOrder -> count(); source++ )
        {
          if ( ! item_db_list[ opt ].compare( itemDbOrder -> item( source ) -> data( Qt::UserRole ).toString() ) )
          {
            itemDbOrder -> insertItem( 0, itemDbOrder -> takeItem( source ) );
            break;
          }
        }
      }
    }
    if ( ! options ) continue;

    QStringList opt_value = opt_tokens[ 1 ].split( '=' );
    for ( int opt=0; options[ opt ].label; opt++ )
    {
      if ( ! opt_value[ 0 ].compare( options[ opt ].option ) )
      {
        buttons -> at( opt )->setChecked( 1 == opt_value[ 1 ].toInt() );
        break;
      }
    }
  }
}

// Encode all options/setting into a string ( to be able to save it to the history )
// Decode / Encode order needs to be equal!

QString SC_MainWindow::encodeOptions()
{
  QString encoded;
  QTextStream ss( &encoded );

  ss << choice.version -> currentIndex();
  ss << ' ' << choice.iterations -> currentIndex();
  ss << ' ' << choice.fight_length -> currentIndex();
  ss << ' ' << choice.fight_variance -> currentIndex();
  ss << ' ' << choice.fight_style -> currentIndex();
  ss << ' ' << choice.target_race -> currentIndex();
  ss << ' ' << choice.num_target -> currentIndex();
  ss << ' ' << choice.player_skill -> currentIndex();
  ss << ' ' << choice.threads -> currentIndex();
  ss << ' ' << choice.armory_region -> currentIndex();
  ss << ' ' << choice.armory_spec -> currentIndex();
  ss << ' ' << choice.default_role -> currentIndex();
  ss << ' ' << choice.world_lag -> currentIndex();
  ss << ' ' << choice.target_level -> currentIndex();
  ss << ' ' << choice.aura_delay -> currentIndex();
  ss << ' ' << choice.report_pets -> currentIndex();
  ss << ' ' << choice.print_style -> currentIndex();
  ss << ' ' << choice.statistics_level -> currentIndex();
  ss << ' ' << choice.deterministic_rng -> currentIndex();
  ss << ' ' << choice.center_scale_delta -> currentIndex();
  ss << ' ' << choice.scale_over -> currentIndex();
  ss << ' ' << choice.challenge_mode -> currentIndex();

  QList<QAbstractButton*> buttons = buffsButtonGroup -> buttons();
  for ( int i = 1; buffOptions[ i ].label; i++ )
  {
    ss << " buff:" << buffOptions[ i ].option << '='
       << ( buttons.at( i ) -> isChecked() ? '1' : '0' );
  }

  buttons = debuffsButtonGroup -> buttons();
  for ( int i = 1; debuffOptions[ i ].label; i++ )
  {
    ss << " debuff:" << debuffOptions[ i ].option << '='
       << ( buttons.at( i ) -> isChecked() ? '1' : '0' );
  }

  buttons = scalingButtonGroup -> buttons();
  for ( int i = 2; scalingOptions[ i ].label; i++ )
  {
    ss << " scaling:" << scalingOptions[ i ].option << '='
       << ( buttons.at( i ) -> isChecked() ? '1' : '0' );
  }

  buttons = plotsButtonGroup -> buttons();
  for ( int i = 0; plotOptions[ i ].label; i++ )
  {
    ss << " plots:" << plotOptions[ i ].option << '='
       << ( buttons.at( i ) -> isChecked() ? '1' : '0' );
  }

  buttons = reforgeplotsButtonGroup -> buttons();
  for ( int i = 0; reforgePlotOptions[ i ].label; i++ )
  {
    ss << " reforge_plots:" << reforgePlotOptions[ i ].option << '='
       << ( buttons.at( i ) -> isChecked() ? '1' : '0' );
  }

  if ( itemDbOrder -> count() > 0 )
  {
    ss << " item_db_source:";
    for ( int i = 0; i < itemDbOrder -> count(); i++ )
    {
      QListWidgetItem *it = itemDbOrder -> item( i );
      ss << it -> data( Qt::UserRole ).toString();
      if ( i < itemDbOrder -> count() - 1 )
        ss << '/';
    }
  }

  return encoded;
}

void SC_MainWindow::updateSimProgress()
{
  if ( sim )
  {
    simProgress = static_cast<int>( 100.0 * sim -> progress( simPhase ) );
  }
  else
  {
    simPhase = "%p%";
    simProgress = 100;
  }
  if ( mainTab -> currentTab() != TAB_IMPORT &&
       mainTab -> currentTab() != TAB_RESULTS )
  {
    progressBar -> setFormat( QString::fromUtf8( simPhase.c_str() ) );
    progressBar -> setValue( simProgress );
  }
}

void SC_MainWindow::loadHistory()
{
  http::cache_load();

  QFile file( SIMC_HISTORY_FILE );
  if ( file.open( QIODevice::ReadOnly ) )
  {
    QDataStream in( &file );

    QString historyVersion;
    in >> historyVersion;
    if ( historyVersion != QString( SC_VERSION ) ) return;
    in >> historyWidth;
    in >> historyHeight;
    in >> historyMaximized;
    in >> simulateCmdLineHistory;
    in >> logCmdLineHistory;
    in >> resultsCmdLineHistory;
    in >> optionsHistory;
    in >> simulateTextHistory;
    in >> overridesTextHistory;
    QStringList importHistory;
    in >> importHistory;
    file.close();

    for ( int i = 0, count = importHistory.count(); i < count; i++ )
      historyList -> addItem( new QListWidgetItem( importHistory.at( i ) ) );

    decodeOptions( optionsHistory.backwards() );

    QString s = overridesTextHistory.backwards();
    if ( ! s.isEmpty() )
      overridesText -> setPlainText( s );
  }
}

void SC_MainWindow::saveHistory()
{
  charDevCookies -> save();
  http::cache_save();

  QFile file( SIMC_HISTORY_FILE );
  if ( file.open( QIODevice::WriteOnly ) )
  {
    optionsHistory.add( encodeOptions() );

    QStringList importHistory;
    int count = historyList -> count();
    for ( int i=0; i < count; i++ )
      importHistory.append( historyList -> item( i ) -> text() );

    QDataStream out( &file );
    out << QString( SC_VERSION );
    out << ( qint32 ) width();
    out << ( qint32 ) height();
    out << ( qint32 ) ( ( windowState() & Qt::WindowMaximized ) ? 1 : 0 );
    out << simulateCmdLineHistory;
    out << logCmdLineHistory;
    out << resultsCmdLineHistory;
    out << optionsHistory;
    out << simulateTextHistory;
    out << overridesTextHistory;
    out << importHistory;
    file.close();
  }
}

// ==========================================================================
// Widget Creation
// ==========================================================================

SC_MainWindow::SC_MainWindow( QWidget *parent )
  : QWidget( parent ),
    historyWidth( 0 ), historyHeight( 0 ), historyMaximized( 1 ),
    visibleWebView( 0 ), sim( 0 ), simPhase( "%p%" ), simProgress( 100 ), simResults( 0 ),
#ifndef Q_WS_MAC
    logFileText( "log.txt" ),
    resultsFileText( "results.html" )
#else
    logFileText( QDir::currentPath() + QDir::separator() + "log.txt" ),
    resultsFileText( QDir::currentPath() + QDir::separator() + "results.html" )
#endif
{
  mainTab = new SC_MainTabWidget( this );
  createWelcomeTab();
  createOptionsTab();
  createImportTab();
  createSimulateTab();
  createOverridesTab();
  createHelpTab();
  createLogTab();
  createResultsTab();
  createSiteTab();
  createCmdLine();
  createToolTips();
#ifdef SC_PAPERDOLL
  createPaperdoll();
#endif

  connect( mainTab, SIGNAL( currentChanged( int ) ), this, SLOT( mainTabChanged( int ) ) );

  QVBoxLayout* vLayout = new QVBoxLayout();
  vLayout -> addWidget( mainTab );
  vLayout -> addWidget( cmdLineGroupBox );
  setLayout( vLayout );

  timer = new QTimer( this );
  connect( timer, SIGNAL( timeout() ), this, SLOT( updateSimProgress() ) );

  importThread = new ImportThread( this );
  connect(   importThread, SIGNAL( finished() ), this, SLOT(  importFinished() ) );

  simulateThread = new SimulateThread( this );
  connect( simulateThread, SIGNAL( finished() ), this, SLOT( simulateFinished() ) );

#ifdef SC_PAPERDOLL
  paperdollThread = new PaperdollThread( this );
  connect( paperdollThread, SIGNAL( finished() ), this, SLOT( paperdollFinished() ) );

  QObject::connect( paperdollProfile, SIGNAL( profileChanged() ), this,    SLOT( start_paperdoll_sim() ) );
#endif

  setAcceptDrops( true );

  loadHistory();
}

void SC_MainWindow::createCmdLine()
{
  QHBoxLayout* cmdLineLayout = new QHBoxLayout();
  cmdLineLayout -> addWidget( backButton = new QPushButton( "<" ) );
  cmdLineLayout -> addWidget( forwardButton = new QPushButton( ">" ) );
  cmdLineLayout -> addWidget( cmdLine = new SC_CommandLine( this ) );
  cmdLineLayout -> addWidget( progressBar = new QProgressBar() );
  cmdLineLayout -> addWidget( mainButton = new QPushButton( "Simulate!" ) );
  backButton -> setMaximumWidth( 30 );
  forwardButton -> setMaximumWidth( 30 );
  progressBar -> setMaximum( 100 );
  progressBar -> setMaximumWidth( 200 );
  progressBar -> setMinimumWidth( 150 );
  QFont progfont( progressBar -> font() );
  progfont.setPointSize( 11 );
  progressBar -> setFont( progfont );
  connect( backButton,    SIGNAL( clicked( bool ) ),   this, SLOT(    backButtonClicked() ) );
  connect( forwardButton, SIGNAL( clicked( bool ) ),   this, SLOT( forwardButtonClicked() ) );
  connect( mainButton,    SIGNAL( clicked( bool ) ),   this, SLOT(    mainButtonClicked() ) );
  connect( cmdLine,       SIGNAL( returnPressed() ),            this, SLOT( cmdLineReturnPressed() ) );
  connect( cmdLine,       SIGNAL( textEdited( const QString& ) ), this, SLOT( cmdLineTextEdited( const QString& ) ) );
  cmdLineGroupBox = new QGroupBox();
  cmdLineGroupBox -> setLayout( cmdLineLayout );
}

void SC_MainWindow::createWelcomeTab()
{
  QString welcomeFile = QDir::currentPath() + "/Welcome.html";

#ifdef Q_WS_MAC
  CFURLRef fileRef    = CFBundleCopyResourceURL( CFBundleGetMainBundle(), CFSTR( "Welcome" ), CFSTR( "html" ), 0 );
  if ( fileRef )
  {
    CFStringRef macPath = CFURLCopyFileSystemPath( fileRef, kCFURLPOSIXPathStyle );
    welcomeFile         = CFStringGetCStringPtr( macPath, CFStringGetSystemEncoding() );

    CFRelease( fileRef );
    CFRelease( macPath );
  }
#endif

  QWebView* welcomeBanner = new QWebView();
  welcomeBanner -> setUrl( "file:///" + welcomeFile );
  mainTab -> addTab( welcomeBanner, tr( "Welcome" ) );
}

void SC_MainWindow::createOptionsTab()
{
  optionsTab = new QTabWidget();
  mainTab -> addTab( optionsTab, tr( "Options" ) );

  createGlobalsTab();
  createBuffsDebuffsTab();
  createScalingTab();
  createPlotsTab();
  createReforgePlotsTab();

  QAbstractButton* allBuffs   =   buffsButtonGroup -> buttons().at( 0 );
  QAbstractButton* allDebuffs = debuffsButtonGroup -> buttons().at( 0 );
  QAbstractButton* allScaling = scalingButtonGroup -> buttons().at( 0 );

  connect( choice.armory_region, SIGNAL( currentIndexChanged( const QString& ) ), this, SLOT( armoryRegionChanged( const QString& ) ) );

  connect( allBuffs,   SIGNAL( toggled( bool ) ), this, SLOT( allBuffsChanged( bool ) )   );
  connect( allDebuffs, SIGNAL( toggled( bool ) ), this, SLOT( allDebuffsChanged( bool ) ) );
  connect( allScaling, SIGNAL( toggled( bool ) ), this, SLOT( allScalingChanged( bool ) ) );
}

void SC_MainWindow::createGlobalsTab()
{

  // Create left side global options
  QFormLayout* globalsLayout_left = new QFormLayout();
  globalsLayout_left -> setFieldGrowthPolicy( QFormLayout::FieldsStayAtSizeHint );
#if SC_BETA
  globalsLayout_left -> addRow(       tr(  "Version" ),        choice.version = createChoice( 3, "Live", "Beta", "Both" ) );
#else
  globalsLayout_left -> addRow(        tr( "Version" ),        choice.version = createChoice( 3, "Live", "PTR", "Both" ) );
#endif
  globalsLayout_left -> addRow( tr(    "Iterations" ),     choice.iterations = createChoice( 5, "100", "1000", "10000", "25000", "50000" ) );
  globalsLayout_left -> addRow( tr(     "World Lag" ),      choice.world_lag = createChoice( 3, "Low", "Medium", "High" ) );
  globalsLayout_left -> addRow( tr(  "Length (sec)" ),   choice.fight_length = createChoice( 10, "100", "150", "200", "250", "300", "350", "400", "450", "500", "600" ) );
  globalsLayout_left -> addRow( tr(   "Vary Length" ), choice.fight_variance = createChoice( 3, "0%", "10%", "20%" ) );
  globalsLayout_left -> addRow( tr(   "Fight Style" ),    choice.fight_style = createChoice( 6, "Patchwerk", "HelterSkelter", "Ultraxion", "LightMovement", "HeavyMovement", "RaidDummy" ) );
  globalsLayout_left -> addRow( tr(  "Target Level" ),   choice.target_level = createChoice( 4, "Raid Boss", "5-man heroic", "5-man normal", "Max Player Level" ) );
  globalsLayout_left -> addRow( tr(   "Target Race" ),    choice.target_race = createChoice( 7, "humanoid", "beast", "demon", "dragonkin", "elemental", "giant", "undead" ) );
  globalsLayout_left -> addRow( tr(   "Num Enemies" ),     choice.num_target = createChoice( 8, "1", "2", "3", "4", "5", "6", "7", "8" ) );
  globalsLayout_left -> addRow( tr( "Challenge Mode" ),   choice.challenge_mode = createChoice( 2, "Disabled","Enabled" ) );
  globalsLayout_left -> addRow( tr(  "Player Skill" ),   choice.player_skill = createChoice( 4, "Elite", "Good", "Average", "Ouch! Fire is hot!" ) );
  globalsLayout_left -> addRow( tr(       "Threads" ),        choice.threads = createChoice( 4, "1", "2", "4", "8" ) );
  globalsLayout_left -> addRow( tr( "Armory Region" ),  choice.armory_region = createChoice( 5, "us", "eu", "tw", "cn", "kr" ) );
  globalsLayout_left -> addRow( tr(   "Armory Spec" ),    choice.armory_spec = createChoice( 2, "active", "inactive" ) );
  globalsLayout_left -> addRow( tr(  "Default Role" ),   choice.default_role = createChoice( 4, "auto", "dps", "heal", "tank" ) );
  choice.iterations -> setCurrentIndex( 1 );
  choice.fight_length -> setCurrentIndex( 7 );
  choice.fight_variance -> setCurrentIndex( 2 );

  QGroupBox* globalsGroupBox_left = new QGroupBox( tr( "Basic Options" ) );
  globalsGroupBox_left -> setLayout( globalsLayout_left );


  // Create right side of global options
  QFormLayout* globalsLayout_right = new QFormLayout();
  globalsLayout_right -> setFieldGrowthPolicy( QFormLayout::FieldsStayAtSizeHint );
  globalsLayout_right -> addRow( tr( "Aura Delay" ),               choice.aura_delay = createChoice( 3, "400ms", "500ms", "600ms" ) );
  globalsLayout_right -> addRow( tr( "Generate Debug" ),                choice.debug = createChoice( 3, "None", "Log Only", "Gory Details" ) );
  globalsLayout_right -> addRow( tr( "Report Pets Separately" ),  choice.report_pets = createChoice( 2, "Yes", "No" ) );
  globalsLayout_right -> addRow( tr( "Report Print Style" ),      choice.print_style = createChoice( 3, "MoP", "White", "Classic" ) );
  globalsLayout_right -> addRow( tr( "Statistics Level" ),   choice.statistics_level = createChoice( 4, "0", "1", "2", "3" ) );
  globalsLayout_right -> addRow( tr( "Deterministic RNG" ), choice.deterministic_rng = createChoice( 2, "Yes", "No" ) );
  choice.aura_delay -> setCurrentIndex( 1 );
  choice.report_pets -> setCurrentIndex( 1 );
  choice.statistics_level -> setCurrentIndex( 1 );
  choice.deterministic_rng -> setCurrentIndex( 1 );

  createItemDataSourceSelector( globalsLayout_right );

  QGroupBox* globalsGroupBox_right = new QGroupBox( tr( "Advanced Options" ) );
  globalsGroupBox_right -> setLayout( globalsLayout_right );

  QHBoxLayout* globalsLayout = new QHBoxLayout();
  globalsLayout -> addWidget( globalsGroupBox_left, 2 );
  globalsLayout -> addWidget( globalsGroupBox_right, 1 );

  QGroupBox* globalsGroupBox = new QGroupBox();
  globalsGroupBox -> setLayout( globalsLayout );

  optionsTab -> addTab( globalsGroupBox, tr( "Globals" ) );

}

void SC_MainWindow::createBuffsDebuffsTab()
{
  // Buffs
  QVBoxLayout* buffsLayout = new QVBoxLayout(); // Buff Layout
  buffsButtonGroup = new QButtonGroup();
  buffsButtonGroup -> setExclusive( false );
  for ( int i = 0; buffOptions[ i ].label; ++i )
  {
    QCheckBox* checkBox = new QCheckBox( buffOptions[ i ].label );

    if ( i > 0 ) checkBox -> setChecked( true );
    checkBox -> setToolTip( buffOptions[ i ].tooltip );
    buffsButtonGroup -> addButton( checkBox );
    buffsLayout -> addWidget( checkBox );
  }
  buffsLayout -> addStretch( 1 );

  QGroupBox* buffsGroupBox = new QGroupBox( tr( "Buffs" ) ); // Buff Widget
  buffsGroupBox -> setLayout( buffsLayout );

  // Debuffs
  QVBoxLayout* debuffsLayout = new QVBoxLayout(); // Debuff Layout
  debuffsButtonGroup = new QButtonGroup();
  debuffsButtonGroup -> setExclusive( false );
  for ( int i = 0; debuffOptions[ i ].label; ++i )
  {
    QCheckBox* checkBox = new QCheckBox( debuffOptions[ i ].label );

    if ( i > 0 ) checkBox -> setChecked( true );
    checkBox -> setToolTip( debuffOptions[ i ].tooltip );
    debuffsButtonGroup -> addButton( checkBox );
    debuffsLayout -> addWidget( checkBox );
  }
  debuffsLayout -> addStretch( 1 );

  QGroupBox* debuffsGroupBox = new QGroupBox( tr( "Debuffs" ) ); // Debuff Widget
  debuffsGroupBox -> setLayout( debuffsLayout );

  // Combined Buff/Debuff Layout & Widget
  QHBoxLayout* buff_debuffLayout = new QHBoxLayout();
  buff_debuffLayout -> addWidget( buffsGroupBox, 1 );
  buff_debuffLayout -> addWidget( debuffsGroupBox, 1 );

  QGroupBox* buff_debuffGroupBox = new QGroupBox();
  buff_debuffGroupBox -> setLayout( buff_debuffLayout );

  // Add Widget as Buffs/Debuffs tab
  optionsTab -> addTab( buff_debuffGroupBox, tr( "Buffs / Debuffs" ) );
}

void SC_MainWindow::createScalingTab()
{
  QVBoxLayout* scalingLayout = new QVBoxLayout();
  scalingButtonGroup = new QButtonGroup();
  scalingButtonGroup -> setExclusive( false );
  for ( int i = 0; scalingOptions[ i ].label; i++ )
  {
    QCheckBox* checkBox = new QCheckBox( scalingOptions[ i ].label );

    checkBox -> setToolTip( scalingOptions[ i ].tooltip );
    scalingButtonGroup -> addButton( checkBox );
    scalingLayout -> addWidget( checkBox );
  }
  //scalingLayout->addStretch( 1 );
  QGroupBox* scalingGroupBox = new QGroupBox();
  scalingGroupBox -> setLayout( scalingLayout );

  QFormLayout* scalingLayout2 = new QFormLayout();
  scalingLayout2 -> setFieldGrowthPolicy( QFormLayout::FieldsStayAtSizeHint );
  scalingLayout2 -> addRow( tr( "Center Scale Delta" ),  choice.center_scale_delta = createChoice( 2, "Yes", "No" ) );
  scalingLayout2 -> addRow( tr( "Scale Over" ),  choice.scale_over = createChoice( 7, "default", "dps", "hps", "dtps", "htps", "raid_dps", "raid_hps" ) );

  choice.center_scale_delta -> setCurrentIndex( 1 );

  scalingLayout -> addLayout( scalingLayout2 );

  optionsTab -> addTab( scalingGroupBox, tr ( "Scaling" ) );
}

void SC_MainWindow::createPlotsTab()
{
  QFormLayout* plotsLayout = new QFormLayout();
  plotsLayout -> setFieldGrowthPolicy( QFormLayout::FieldsStayAtSizeHint );

  // Create Combo Boxes
  choice.plots_points = createChoice( 4, "20", "30", "40", "50" );
  plotsLayout -> addRow( tr( "Number of Plot Points" ), choice.plots_points );

  choice.plots_step = createChoice( 6, "25", "50", "100", "200", "250", "500" );
  choice.plots_step -> setCurrentIndex( 2 );
  plotsLayout -> addRow( tr( "Plot Step Amount" ), choice.plots_step );

  plotsButtonGroup = new QButtonGroup();
  plotsButtonGroup -> setExclusive( false );
  for ( int i = 0; plotOptions[ i ].label; i++ )
  {
    QCheckBox* checkBox = new QCheckBox( plotOptions[ i ].label );
    checkBox -> setToolTip( plotOptions[ i ].tooltip );
    plotsButtonGroup -> addButton( checkBox );
    plotsLayout -> addWidget( checkBox );
  }
  QGroupBox* plotsGroupBox = new QGroupBox();
  plotsGroupBox -> setLayout( plotsLayout );

  optionsTab -> addTab( plotsGroupBox, "Plots" );
}

void SC_MainWindow::createReforgePlotsTab()
{
  QFormLayout* reforgePlotsLayout = new QFormLayout();
  reforgePlotsLayout -> setFieldGrowthPolicy( QFormLayout::FieldsStayAtSizeHint );

  // Create Combo Boxes
  choice.reforgeplot_amount = createChoice( 10, "100", "200", "250", "500", "750", "1000", "1500", "2000", "3000", "5000" );
  choice.reforgeplot_amount -> setCurrentIndex( 1 );
  reforgePlotsLayout -> addRow( tr( "Reforge Amount" ), choice.reforgeplot_amount );

  choice.reforgeplot_step = createChoice( 6, "25", "50", "100", "200", "250", "500" );
  choice.reforgeplot_step -> setCurrentIndex( 1 );
  reforgePlotsLayout -> addRow( tr( "Step Amount" ), choice.reforgeplot_step );

  QLabel* messageText = new QLabel( tr( "A maximum of three stats may be ran at once.\n" ) );
  reforgePlotsLayout -> addRow( messageText );

  messageText = new QLabel( "Secondary Stats" );
  reforgePlotsLayout -> addRow( messageText );

  reforgeplotsButtonGroup = new SC_ReforgeButtonGroup( this );
  reforgeplotsButtonGroup -> setExclusive( false );
  for ( int i = 0; i < 6 && reforgePlotOptions[ i ].label; i++ )
  {
    QCheckBox* checkBox = new QCheckBox( reforgePlotOptions[ i ].label );
    checkBox -> setToolTip( reforgePlotOptions[ i ].tooltip );
    reforgeplotsButtonGroup -> addButton( checkBox );
    reforgePlotsLayout -> addWidget( checkBox );
    QObject::connect( checkBox, SIGNAL( stateChanged( int ) ),
                      reforgeplotsButtonGroup, SLOT( setSelected( int ) ) );
  }

  messageText = new QLabel( "\n" + tr( "Primary Stats" ) );
  reforgePlotsLayout -> addRow( messageText );

  for ( int i = 6; reforgePlotOptions[ i ].label; i++ )
  {
    QCheckBox* checkBox = new QCheckBox( reforgePlotOptions[ i ].label );
    checkBox -> setToolTip( reforgePlotOptions[ i ].tooltip );
    reforgeplotsButtonGroup -> addButton( checkBox );
    reforgePlotsLayout -> addWidget( checkBox );
    QObject::connect( checkBox, SIGNAL( stateChanged( int ) ),
                      reforgeplotsButtonGroup, SLOT( setSelected( int ) ) );
  }

  QGroupBox* reforgeplotsGroupBox = new QGroupBox();
  reforgeplotsGroupBox -> setLayout( reforgePlotsLayout );

  optionsTab -> addTab( reforgeplotsGroupBox, tr( "Reforge Plots" ) );
}

void SC_MainWindow::createImportTab()
{
  importTab = new SC_ImportTabWidget( this );
  mainTab -> addTab( importTab, tr( "Import" ) );

  battleNetView = new SC_WebView( this );
  battleNetView -> setUrl( QUrl( "http://us.battle.net/wow/en" ) );
  importTab -> addTab( battleNetView, tr( "Battle.Net" ) );

  charDevCookies = new PersistentCookieJar( "chardev.cookies" );
  charDevCookies -> load();
  charDevView = new SC_WebView( this );
  charDevView -> page() -> networkAccessManager() -> setCookieJar( charDevCookies );
  charDevView -> setUrl( QUrl( "http://chardev.org/?planner" ) );
  importTab -> addTab( charDevView, tr( "CharDev" ) );

  createRawrTab();
  createBestInSlotTab();

  historyList = new QListWidget();
  historyList -> setSortingEnabled( true );
  importTab -> addTab( historyList, tr( "History" ) );

  connect( rawrButton,  SIGNAL( clicked( bool ) ),                       this, SLOT( rawrButtonClicked() ) );
  connect( historyList, SIGNAL( itemDoubleClicked( QListWidgetItem* ) ), this, SLOT( historyDoubleClicked( QListWidgetItem* ) ) );
  connect( importTab,   SIGNAL( currentChanged( int ) ),                 this, SLOT( importTabChanged( int ) ) );

  // Commenting out until it is more fleshed out.
  // createCustomTab();
}

void SC_MainWindow::createRawrTab()
{
  QVBoxLayout* rawrLayout = new QVBoxLayout();
  QLabel* rawrLabel = new QLabel( QString( " http://rawr.codeplex.com\n\n" ) +
                                  tr(  "Rawr is an exceptional theorycrafting tool that excels at gear optimization."
                                       " The key architectural difference between Rawr and SimulationCraft is one of"
                                       " formulation vs simulation.\nThere are strengths and weaknesses to each"
                                       " approach.  Since they come from different directions, one can be confident"
                                       " in the result when they arrive at the same destination.\n\n"
                                       " To aid comparison, SimulationCraft can import the character xml file written by Rawr.\n\n"
                                       " Alternatively, paste xml from the Rawr in-game addon into the space below." ) );
  rawrLabel -> setWordWrap( true );
  rawrLayout -> addWidget( rawrLabel );
  rawrLayout -> addWidget( rawrButton = new QPushButton( tr( "Load Rawr XML" ) ) );
  rawrLayout -> addWidget( rawrText = new SC_PlainTextEdit( this ), 1 );
  QGroupBox* rawrGroupBox = new QGroupBox();
  rawrGroupBox -> setLayout( rawrLayout );
  importTab -> addTab( rawrGroupBox, "Rawr" );
}

void SC_MainWindow::createBestInSlotTab()
{
  // Create BiS Tree ( table with profiles )
  QStringList headerLabels( tr( "Player Class" ) ); headerLabels += QString( tr( "Location" ) );

  QTreeWidget* bisTree = new QTreeWidget();
  bisTree -> setColumnCount( 1 );
  bisTree -> setHeaderLabels( headerLabels );

  const int TIER_MAX=2;
#if SC_BETA == 1
  const char* tierNames[] = { "" }; // For the beta include ALL profiles
#else
  const char* tierNames[] = { "T13", "T14" };
#endif
  QTreeWidgetItem* playerItems[ PLAYER_MAX ];
  range::fill( playerItems, 0 );
  QTreeWidgetItem* rootItems[ PLAYER_MAX ][ TIER_MAX ];
  for ( player_e i = DEATH_KNIGHT; i <= WARRIOR; i++ )
  {
    range::fill( rootItems[ i ], 0 );
  }

// Scan all subfolders in /profiles/ and create a list
#ifndef Q_WS_MAC
  QDir tdir = QString( "profiles" );
#else
  CFURLRef fileRef    = CFBundleCopyResourceURL( CFBundleGetMainBundle(), CFSTR( "profiles" ), 0, 0 );
  QDir tdir;
  if ( fileRef )
  {
    CFStringRef macPath = CFURLCopyFileSystemPath( fileRef, kCFURLPOSIXPathStyle );
    tdir            = QString( CFStringGetCStringPtr( macPath, CFStringGetSystemEncoding() ) );

    CFRelease( fileRef );
    CFRelease( macPath );
  }
#endif
  tdir.setFilter( QDir::Dirs );


  QStringList tprofileList = tdir.entryList();
  int tnumProfiles = tprofileList.count();
  // Main loop through all subfolders of ./profiles/
  for ( int i=0; i < tnumProfiles; i++ )
  {
#ifndef Q_WS_MAC
    QDir dir = QString( "profiles/" + tprofileList[ i ] );
#else
    CFURLRef fileRef = CFBundleCopyResourceURL( CFBundleGetMainBundle(),
                       CFStringCreateWithCString( NULL,
                           tprofileList[ i ].toAscii().constData(),
                           kCFStringEncodingUTF8 ),
                       0,
                       CFSTR( "profiles" ) );
    QDir dir;
    if ( fileRef )
    {
      CFStringRef macPath = CFURLCopyFileSystemPath( fileRef, kCFURLPOSIXPathStyle );
      dir            = QString( CFStringGetCStringPtr( macPath, CFStringGetSystemEncoding() ) );

      CFRelease( fileRef );
      CFRelease( macPath );
    }
#endif
    dir.setSorting( QDir::Name );
    dir.setFilter( QDir::Files );
    dir.setNameFilters( QStringList( "*.simc" ) );

    QStringList profileList = dir.entryList();
    int numProfiles = profileList.count();
    for ( int i = 0; i < numProfiles; i++ )
    {
      QString profile = dir.absolutePath() + "/";
      profile = QDir::toNativeSeparators( profile );
      profile += profileList[ i ];

      player_e player = PLAYER_MAX;

      // Hack! For now...  Need to decide sim-wide just how the heck we want to refer to DKs.
      if ( profile.contains( "Death_Knight" ) )
        player = DEATH_KNIGHT;
      else
      {
        for ( player_e j = PLAYER_NONE; j < PLAYER_MAX; j++ )
        {
          if ( profile.contains( util::player_type_string( j ), Qt::CaseInsensitive ) )
          {
            player = j;
            break;
          }
        }
      }

      // exclude generate profiles
      if ( profile.contains( "generate" ) )
        continue;

      int tier = TIER_MAX;
      for ( int j = 0; j < TIER_MAX && tier == TIER_MAX; j++ )
        if ( profile.contains( tierNames[ j ] ) )
          tier = j;

      if ( player != PLAYER_MAX && tier != TIER_MAX )
      {
        if ( !rootItems[ player ][ tier ] )
        {
          if ( !playerItems[ player ] )
          {
            QTreeWidgetItem* top = new QTreeWidgetItem( QStringList( util::inverse_tokenize( util::player_type_string( player ) ).c_str() ) );
            playerItems[ player ] = top;
          }

          if ( !rootItems[ player ][ tier ] )
          {
            QTreeWidgetItem* tieritem = new QTreeWidgetItem( QStringList( tierNames[ tier ] ) );
            playerItems[ player ] -> addChild( rootItems[ player ][ tier ] =  tieritem );
          }
        }

        QTreeWidgetItem* item = new QTreeWidgetItem( QStringList() << profileList[ i ] << profile );
        rootItems[ player ][ tier ] -> addChild( item );
      }
    }
  }

  // Register all the added profiles ( done here so they show up alphabetically )
  for ( player_e i = DEATH_KNIGHT; i <= WARRIOR; i++ )
  {
    if ( playerItems[ i ] )
    {
      bisTree -> addTopLevelItem( playerItems[ i ] );
      for ( int j = 0; j < TIER_MAX; j++ )
      {
        if ( rootItems[ i ][ j ] )
        {
          rootItems[ i ][ j ] -> setExpanded( true ); // Expand the subclass Tier bullets by default
          rootItems[ i ][ j ] -> sortChildren( 0, Qt::AscendingOrder );
        }
      }
    }
  }

  bisTree -> setColumnWidth( 0, 300 );

  connect( bisTree, SIGNAL( itemDoubleClicked( QTreeWidgetItem*,int ) ), this, SLOT( bisDoubleClicked( QTreeWidgetItem*,int ) ) );

  // Create BiS Introduction

  QFormLayout* bisIntroductionFormLayout = new QFormLayout();
  QLabel* bisText = new QLabel( tr( "These sample profiles are attempts at creating the best possible gear, talent, glyph and action priority list setups to achieve the highest possible average damage per second.\n"
                                    "The profiles are created with a lot of help from the theorycrafting community.\n"
                                    "They are only as good as the thorough testing done on them, and the feedback and critic we receive from the community, including yourself.\n"
                                    "If you have ideas for improvements, try to simulate them. If they result in increased dps, please open a ticket on our Issue tracker.\n"
                                    "The more people help improve BiS profiles, the better will they reach their goal of representing the highest possible dps." ) );
  bisIntroductionFormLayout -> addRow( bisText );

  QWidget* bisIntroduction = new QWidget();
  bisIntroduction -> setLayout( bisIntroductionFormLayout );

  // Create BiS Tab ( Introduction + BiS Tree )

  QVBoxLayout* bisTabLayout = new QVBoxLayout();
  bisTabLayout -> addWidget( bisIntroduction, 1 );
  bisTabLayout -> addWidget( bisTree, 9 );

  QGroupBox* bisTab = new QGroupBox();
  bisTab -> setLayout( bisTabLayout );
  importTab -> addTab( bisTab, tr( "Sample Profiles" ) );

}

void SC_MainWindow::createCustomTab()
{
  //In Dev - Character Retrieval Boxes & Buttons
  //In Dev - Load & Save Profile Buttons
  //In Dev - Profiler Slots, Talent & Glyph Layout
  QHBoxLayout* customLayout = new QHBoxLayout();
  QGroupBox* customGroupBox = new QGroupBox();
  customGroupBox -> setLayout( customLayout );
  importTab -> addTab( customGroupBox, "Custom Profile" );
  customLayout -> addWidget( createCustomCharData = new QGroupBox( tr( "Character Data" ) ), 1 );
  createCustomCharData -> setObjectName( QString::fromUtf8( "createCustomCharData" ) );
  customLayout -> addWidget( createCustomProfileDock = new QTabWidget(), 1 );
  createCustomProfileDock -> setObjectName( QString::fromUtf8( "createCustomProfileDock" ) );
  createCustomProfileDock -> setAcceptDrops( true );
  customGearTab = new QWidget();
  customGearTab -> setObjectName( QString::fromUtf8( "customGearTab" ) );
  createCustomProfileDock -> addTab( customGearTab, QString() );
  customTalentsTab = new QWidget();
  customTalentsTab -> setObjectName( QString::fromUtf8( "customTalentsTab" ) );
  createCustomProfileDock -> addTab( customTalentsTab, QString() );
  customGlyphsTab = new QWidget();
  customGlyphsTab -> setObjectName( QString::fromUtf8( "customGlyphsTab" ) );
  createCustomProfileDock -> addTab( customGlyphsTab, QString() );
  createCustomProfileDock -> setTabText( createCustomProfileDock -> indexOf( customGearTab ), tr( "Gear", "createCustomTab" ) );
  createCustomProfileDock -> setTabToolTip( createCustomProfileDock -> indexOf( customGearTab ), tr( "Customise Gear Setup", "createCustomTab" ) );
  createCustomProfileDock -> setTabText( createCustomProfileDock -> indexOf( customTalentsTab ), tr( "Talents", "createCustomTab" ) );
  createCustomProfileDock -> setTabToolTip( createCustomProfileDock -> indexOf( customTalentsTab ), tr( "Customise Talents", "createCustomTab" ) );
  createCustomProfileDock -> setTabText( createCustomProfileDock -> indexOf( customGlyphsTab ), tr( "Glyphs", "createCustomTab" ) );
  createCustomProfileDock -> setTabToolTip( createCustomProfileDock -> indexOf( customGlyphsTab ), tr( "Customise Glyphs", "createCustomTab" ) );
}

void SC_MainWindow::createSimulateTab()
{
  simulateText = new SC_PlainTextEdit( this );
  simulateText -> setPlainText( defaultSimulateText );
  mainTab -> addTab( simulateText, tr( "Simulate" ) );
}

void SC_MainWindow::createOverridesTab()
{
  overridesText = new SC_PlainTextEdit( this );
  overridesText -> setPlainText( "# User-specified persistent global and player parms will set here.\n" );
  mainTab -> addTab( overridesText, tr( "Overrides" ) );
}

void SC_MainWindow::createLogTab()
{
  logText = new SC_PlainTextEdit(  this, false );
  //logText -> document() -> setDefaultFont( QFont( "fixed" ) );
  logText -> setReadOnly( true );
  logText -> setPlainText( "Look here for error messages and simple text-only reporting.\n" );
  mainTab -> addTab( logText, tr( "Log" ) );
}

void SC_MainWindow::createHelpTab()
{
  helpView = new SC_WebView( this );
  helpView -> setUrl( QUrl( "http://code.google.com/p/simulationcraft/wiki/StartersGuide" ) );
  mainTab -> addTab( helpView, tr( "Help" ) );
}

void SC_MainWindow::createResultsTab()
{
  QString s = "<div align=center><h1>Understanding SimulationCraft Output!</h1>If you are seeing this text, then Legend.html was unable to load.</div>";
  QString legendFile = "Legend.html";
#ifdef Q_WS_MAC
  CFURLRef fileRef    = CFBundleCopyResourceURL( CFBundleGetMainBundle(), CFSTR( "Legend" ), CFSTR( "html" ), 0 );
  if ( fileRef )
  {
    CFStringRef macPath = CFURLCopyFileSystemPath( fileRef, kCFURLPOSIXPathStyle );
    legendFile          = CFStringGetCStringPtr( macPath, CFStringGetSystemEncoding() );

    CFRelease( fileRef );
    CFRelease( macPath );
  }
#endif

  QFile file( legendFile );
  if ( file.open( QIODevice::ReadOnly ) )
  {
    s = QString::fromUtf8( file.readAll() );
    file.close();
  }

  QTextBrowser* legendBanner = new QTextBrowser();
  legendBanner -> setHtml( s );
  legendBanner -> moveCursor( QTextCursor::Start );

  resultsTab = new QTabWidget();
  resultsTab -> setTabsClosable( true );
  resultsTab -> addTab( legendBanner, tr( "Legend" ) );
  connect( resultsTab, SIGNAL( currentChanged( int ) ),    this, SLOT( resultsTabChanged( int ) )      );
  connect( resultsTab, SIGNAL( tabCloseRequested( int ) ), this, SLOT( resultsTabCloseRequest( int ) ) );
  mainTab -> addTab( resultsTab, tr( "Results" ) );
}

void SC_MainWindow::createSiteTab()
{
  siteView = new SC_WebView( this );
  siteView -> setUrl( QUrl( "http://code.google.com/p/simulationcraft/" ) );
  mainTab -> addTab( siteView, tr( "Site" ) );
}

void SC_MainWindow::createToolTips()
{
  choice.version -> setToolTip( tr( "Live: Use mechanics on Live servers. ( WoW Build %1 )" ).arg( sim -> dbc.build_level( false ) ) +"\n" +
#if SC_BETA
                                tr( "Beta:  Use mechanics on Beta servers. ( WoW Build %1 )" ).arg( sim -> dbc.build_level( true ) ) +"\n" +
                                tr( "Both: Create Evil Twin with Beta mechanics" ) );
#else
                                tr( "PTR:  Use mechanics on PTR servers. ( WoW Build %1 )" ).arg( sim -> dbc.build_level( true ) ) +"\n" +
                                tr( "Both: Create Evil Twin with PTR mechanics" ) );
#endif

  choice.iterations -> setToolTip( tr( "%1:   Fast and Rough" ).arg( 100 ) +"\n" +
                                   tr( "%1:  Sufficient for DPS Analysis" ).arg( 1000 ) +"\n" +
                                   tr( "%1: Recommended for Scale Factor Generation" ).arg( 10000 ) +"\n" +
                                   tr( "%1: Use if %2 isn't enough for Scale Factors" ).arg( 25000 ).arg( 10000 ) +"\n" +
                                   tr( "%1: If you're patient" ).arg( 100 ) );

  choice.fight_length -> setToolTip( tr( "For custom fight lengths use max_time=seconds." ) );

  choice.fight_variance -> setToolTip( tr( "Varying the fight length over a given spectrum improves\n"
                                           "the analysis of trinkets and abilities with long cooldowns." ) );

  choice.fight_style -> setToolTip( tr( "Patchwerk: Tank-n-Spank" ) +"\n" +
                                    tr( "HelterSkelter:\n"
                                        "    Movement, Stuns, Interrupts,\n"
                                        "    Target-Switching (every 2min)" ) +"\n" +
                                    tr( "Ultraxion:\n"
                                        "    Periodic Stuns, Raid Damage" ) +"\n" +
                                    tr( "LightMovement:\n"
                                        "    %1s Movement, %2s CD,\n"
                                        "    %3% into the fight until %4% before the end" ).arg( 7 ).arg( 85 ).arg( 10 ).arg( 20 ) +"\n" +
                                    tr( "HeavyMovement:\n"
                                        "    %1s Movement, %2s CD,\n"
                                        "    beginning %3s into the fight" ).arg( 4 ).arg( 10 ).arg( 10 ) );

  choice.target_race -> setToolTip( tr( "Race of the target and any adds." ) );

  choice.challenge_mode -> setToolTip( tr( "Enables/Disables the challenge mode setting, downscaling items to level 463.\n"
                                           "Stats won't be exact, but very close.") );

  choice.num_target -> setToolTip( tr( "Number of enemies." ) );

  choice.target_level -> setToolTip( tr( "Level of the target and any adds." ) );

  choice.player_skill -> setToolTip( tr( "Elite:       No mistakes.  No cheating either." ) +"\n" +
                                     tr( "Fire-is-Hot: Frequent DoT-clipping and skipping high-priority abilities." ) );

  choice.threads -> setToolTip( tr( "Match the number of CPUs for optimal performance.\n"
                                    "Most modern desktops have at least two CPU cores." ) );

  choice.armory_region -> setToolTip( tr( "United States, Europe, Taiwan, China, Korea" ) );

  choice.armory_spec -> setToolTip( tr( "Controls which Talent/Glyph specification is used when importing profiles from the Armory." ) );

  choice.default_role -> setToolTip( tr( "Specify the character role during import to ensure correct action priority list." ) );

  choice.report_pets -> setToolTip( tr( "Specify if pets get reported separately in detail." ) );

  choice.print_style -> setToolTip( tr( "Specify html report print style." ) );


  choice.statistics_level -> setToolTip( tr( "Determines how much detailed statistical information besides count & mean will be collected during simulation.\n"
                                             " Higher Statistics Level require more memory." ) +"\n" +
                                         tr( " Level %1: Only Simulation Length data is collected." ).arg( 0 ) +"\n" +
                                         tr( " Level %1: DPS/HPS data is collected. *default*" ).arg( 1 ) +"\n" +
                                         tr( " Level %1: Player Fight Length, Death Time, DPS(e), HPS(e), DTPS, HTPS, DMG, HEAL data is collected." ).arg( 2 ) +"\n" +
                                         tr( " Level %1: Ability Amount and  portion APS is collected." ).arg( 3 ) );

  choice.debug -> setToolTip( tr( "When a log is generated, only one iteration is used.\n"
                                  "Gory details are very gory.  No documentation will be forthcoming.\n"
                                  "Due to the forced single iteration, no scale factor calculation." ) );


  choice.deterministic_rng -> setToolTip( tr( "Deterministic Random Number Generator creates all random numbers with a given, constant seed.\n"
                                              "This allows to better observe marginal changes which aren't influenced by rng, \n"
                                              " or check for other influences without having to reduce statistic noise" ) );

  choice.world_lag -> setToolTip( tr( "World Lag is the equivalent of the 'world lag' shown in the WoW Client.\n"
                                      "It is currently used to extend the cooldown duration of user executable abilities "
                                      " that have a cooldown.\n"
                                      "Each setting adds an amount of 'lag' with a default standard deviation of 10%:" ) +"\n" +
                                  tr( "    'Low'   : %1ms" ).arg( 100 ) +"\n" +
                                  tr( "    'Medium': %1ms" ).arg( 300 ) +"\n" +
                                  tr( "    'High'  : %1ms" ).arg( 500 ) );

  choice.aura_delay -> setToolTip( tr( "Aura Lag represents the server latency which occurs when buffs are applied.\n"
                                       "This value is given by Blizzard server reaction time and not influenced by your latency.\n"
                                       "Each setting adds an amount of 'lag' with a default standard deviation of 10%:\n" ) );

  backButton -> setToolTip( tr( "Backwards" ) );
  forwardButton -> setToolTip( tr( "Forwards" ) );

  choice.plots_points -> setToolTip( tr( "The number of points that will appear on the graph" ) );
  choice.plots_step -> setToolTip( tr( "The delta between two points of the graph.\n"
                                       "The deltas on the horizontal axis will be within the [-points * steps / 2 ; +points * steps / 2] interval" ) );

  choice.reforgeplot_amount -> setToolTip( tr( "The maximum amount to reforge per stat." ) );
  choice.reforgeplot_step -> setToolTip( tr( "The stat difference between two points.\n"
                                             "It's NOT the number of steps: a lower value will generate more points!" ) );
}

#ifdef SC_PAPERDOLL
void SC_MainWindow::createPaperdoll()
{
  QWidget* paperdollTab = new QWidget( this );
  QHBoxLayout* paperdollMainLayout = new QHBoxLayout();
  paperdollMainLayout -> setAlignment( Qt::AlignLeft | Qt::AlignTop );
  paperdollTab -> setLayout( paperdollMainLayout );

  paperdollProfile = new PaperdollProfile();
  paperdoll = new Paperdoll( this, paperdollProfile, paperdollTab );
  ItemSelectionWidget* items = new ItemSelectionWidget( paperdollProfile, paperdollTab );

  paperdollMainLayout -> addWidget( items );
  paperdollMainLayout -> addWidget( paperdoll );


  mainTab -> addTab( paperdollTab, "Paperdoll" );
}
#endif

void SC_MainWindow::createItemDataSourceSelector( QFormLayout* layout )
{
  itemDbOrder = new QListWidget( this );
  itemDbOrder -> setDragDropMode( QAbstractItemView::InternalMove );
  itemDbOrder -> setResizeMode( QListView::Fixed );
  itemDbOrder -> setSelectionRectVisible( false );
  itemDbOrder -> setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );
  itemDbOrder -> setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

  for ( unsigned i = 0; i < sizeof_array( itemSourceOptions ); ++i )
  {
    QListWidgetItem* item = new QListWidgetItem( itemSourceOptions[ i ].label );
    item -> setData( Qt::UserRole, QVariant( itemSourceOptions[ i ].option ) );
    item -> setToolTip( itemSourceOptions[ i ].tooltip );
    itemDbOrder -> addItem( item );
  }

  itemDbOrder -> setFixedHeight( ( itemDbOrder -> model() -> rowCount() + 1 ) * itemDbOrder -> sizeHintForRow( 0 ) );

  layout->addRow( "Item Source Order", itemDbOrder );
}

void SC_MainWindow::updateVisibleWebView( SC_WebView* wv )
{
  assert( wv );
  visibleWebView = wv;
  progressBar -> setFormat( "%p%" );
  progressBar -> setValue( visibleWebView -> progress );
  cmdLine -> setText( visibleWebView -> url().toString() );
}

// ==========================================================================
// Sim Initialization
// ==========================================================================

sim_t* SC_MainWindow::initSim()
{
  if ( ! sim )
  {
    sim = new sim_t();
    sim -> output_file = fopen( SIMC_LOG_FILE, "w" );
    sim -> report_progress = 0;
#if SC_USE_PTR
    sim -> parse_option( "ptr", ( ( choice.version -> currentIndex() == 1 ) ? "1" : "0" ) );
#endif
    sim -> parse_option( "debug", ( ( choice.debug -> currentIndex() == 2 ) ? "1" : "0" ) );
  }
  return sim;
}

void SC_MainWindow::deleteSim()
{
  if ( sim )
  {
    fclose( sim -> output_file );
    delete sim;
    sim = 0;

    QString contents;
    QFile logFile( SIMC_LOG_FILE );
    if ( logFile.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
      contents = QString::fromUtf8( logFile.readAll() );
      logFile.close();
    }

    if ( ! simulateThread -> success )
      logText -> setformat_error();

    logText -> appendPlainText( contents );
    logText -> moveCursor( QTextCursor::End );
    logText -> resetformat();
  }
}

void SC_MainWindow::startImport( int tab, const QString& url )
{
  if ( sim )
  {
    sim  ->  cancel();
    return;
  }
  simProgress = 0;
  mainButton -> setText( "Cancel!" );
  importThread -> start( initSim(), tab, url, get_db_order() );
  simulateText -> setPlainText( defaultSimulateText );
  mainTab -> setCurrentTab( TAB_SIMULATE );
  timer -> start( 500 );
}

void SC_MainWindow::importFinished()
{
  timer -> stop();
  simPhase = "%p%";
  simProgress = 100;
  progressBar -> setFormat( simPhase.c_str() );
  progressBar -> setValue( simProgress );
  if ( importThread -> player )
  {
    simulateText -> setPlainText( importThread -> profile );
    simulateTextHistory.add( importThread -> profile );

    QString label = QString::fromUtf8( importThread -> player -> name_str.c_str() );
    while ( label.size() < 20 ) label += ' ';
    label += QString::fromUtf8( importThread -> player -> origin_str.c_str() );

    bool found = false;
    for ( int i = 0; i < historyList -> count() && ! found; i++ )
      if ( historyList -> item( i ) -> text() == label )
        found = true;

    if ( ! found )
    {
      QListWidgetItem* item = new QListWidgetItem( label );
      //item -> setFont( QFont( "fixed" ) );

      historyList -> addItem( item );
      historyList -> sortItems();
    }
  }
  else
  {
    simulateText -> setformat_error(); // Print error message in big letters

    QFile logFile( SIMC_LOG_FILE );
    if ( logFile.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
      simulateText -> setPlainText( QString::fromUtf8( logFile.readAll() ) );
      logFile.close();
    }
    simulateText -> appendPlainText( "# Unable to generate profile from: " + importThread -> url );

    simulateText -> resetformat(); // Reset font
  }

  mainButton -> setText( "Simulate!" );
  mainTab -> setCurrentTab( TAB_SIMULATE );

  deleteSim();
}

void SC_MainWindow::startSim()
{
  if ( sim )
  {
    sim -> cancel();
    return;
  }
  optionsHistory.add( encodeOptions() );
  optionsHistory.current_index = 0;
  if ( simulateText -> toPlainText() != defaultSimulateText )
  {
    simulateTextHistory.add( simulateText -> toPlainText() );
  }
  overridesTextHistory.add( overridesText -> toPlainText() );
  simulateCmdLineHistory.add( cmdLine -> text() );
  simProgress = 0;
  mainButton -> setText( "Cancel!" );
  simulateThread -> start( initSim(), mergeOptions() );
  // simulateText -> setPlainText( defaultSimulateText() );
  cmdLineText = "";
  cmdLine -> setText( cmdLineText );
  timer -> start( 100 );
}

#ifdef SC_PAPERDOLL
void SC_MainWindow::start_paperdoll_sim()
{
  if ( sim )
  {
    sim -> cancel();
    paperdollThread -> wait( 100 );
    deleteSim();
  }


  sim = initSim();

  PaperdollProfile* profile = paperdollProfile;
  const module_t* module = module_t::get( profile -> currentClass() );
  player_t* player = module ? module -> create_player( sim, "Paperdoll Player", profile -> currentRace() ) : NULL;

  if ( player )
  {
    player -> role = util::parse_role_type( choice.default_role -> currentText().toUtf8().constData() );

    player -> professions_str += std::string( util::profession_type_string( profile -> currentProfession( 0 ) ) ) + "/" + util::profession_type_string( profile -> currentProfession( 1 ) ) ;

    for ( slot_e i = SLOT_MIN; i < SLOT_MAX; i++ )
    {
      const item_data_t* profile_item = profile -> slotItem( i );
      if ( profile_item )
      {
        player -> items.push_back( item_t( player, std::string() ) );

        item_database::load_item_from_data( player -> items.back(), profile_item, 0 ); // Hook up upgrade level from paperdoll once that's implemented
      }
    }

    paperdoll -> setCurrentDPS( 0, 0 );

    paperdollThread -> start( sim, player, get_globalSettings() );

    simProgress = 0;
  }
}
#endif

QString SC_MainWindow::get_db_order() const
{
  QString options;

  assert( itemDbOrder -> count() > 0 );

  for ( int i = 0; i < itemDbOrder -> count(); i++ )
  {
    QListWidgetItem *it = itemDbOrder -> item( i );
    options += it -> data( Qt::UserRole ).toString();
    if ( i < itemDbOrder -> count() - 1 )
      options += '/';
  }

  return options;
}

QString SC_MainWindow::get_globalSettings()
{
  QString options = "";

#if SC_USE_PTR
  options += "ptr=";
  options += ( ( choice.version->currentIndex() == 1 ) ? "1" : "0" );
  options += "\n";
#endif
  options += "item_db_source=" + get_db_order() + '\n';
  options += "iterations=" + choice.iterations->currentText() + "\n";
  if ( choice.iterations->currentText() == "10000" )
  {
    options += "dps_plot_iterations=1000\n";
  }

  const char *world_lag[] = { "0.1", "0.3", "0.5" };
  options += "default_world_lag=";
  options += world_lag[ choice.world_lag->currentIndex() ];
  options += "\n";


  const char *auradelay[] = { "0.4", "0.5", "0.6" };
  options += "default_aura_delay=";
  options += auradelay[ choice.aura_delay->currentIndex() ];
  options += "\n";

  options += "max_time=" + choice.fight_length->currentText() + "\n";

  options += "vary_combat_length=";
  const char *variance[] = { "0.0", "0.1", "0.2" };
  options += variance[ choice.fight_variance->currentIndex() ];
  options += "\n";

  options += "fight_style=" + choice.fight_style->currentText() + "\n";

  if ( choice.challenge_mode -> currentIndex() > 0 )
    options += "challenge_mode=1\n";

  static const char* const targetlevel[] = { "3", "2", "1", "0" };
  options += "target_level+=";
  options += targetlevel[ choice.target_level -> currentIndex() ];
  options += "\n";

  options += "target_race=" + choice.target_race->currentText() + "\n";

  options += "default_skill=";
  const char *skill[] = { "1.0", "0.9", "0.75", "0.50" };
  options += skill[ choice.player_skill->currentIndex() ];
  options += "\n";

  options += "optimal_raid=0\n";
  QList<QAbstractButton*> buttons = buffsButtonGroup->buttons();
  for ( int i=1; buffOptions[ i ].label; i++ )
  {
    options += buffOptions[ i ].option;
    options += "=";
    options += buttons.at( i )->isChecked() ? "1" : "0";
    options += "\n";
  }
  buttons = debuffsButtonGroup->buttons();
  for ( int i=1; debuffOptions[ i ].label; i++ )
  {
    options += debuffOptions[ i ].option;
    options += "=";
    options += buttons.at( i )->isChecked() ? "1" : "0";
    options += "\n";
  }

  if ( choice.deterministic_rng->currentIndex() == 0 )
  {
    options += "deterministic_rng=1\n";
  }

  return options;
}

QString SC_MainWindow::mergeOptions()
{
  QString options = "";

  options += get_globalSettings();
  options += "threads=" + choice.threads->currentText() + "\n";

  QList<QAbstractButton*> buttons = scalingButtonGroup->buttons();
  for ( int i=2; scalingOptions[ i ].label; i++ )
  {
    if ( buttons.at( i )->isChecked() )
    {
      options += "calculate_scale_factors=1\n";
      break;
    }
  }

  if ( buttons.at( 1 )->isChecked() ) options += "positive_scale_delta=1\n";
  if ( buttons.at( buttons.size() - 1 )->isChecked() ) options += "scale_lag=1\n";
  if ( buttons.at( 15 )->isChecked() || buttons.at( 17 )->isChecked() ) options += "weapon_speed_scale_factors=1\n";

  options += "scale_only=none";
  for ( int i=2; scalingOptions[ i ].label; i++ )
  {
    if ( buttons.at( i )->isChecked() )
    {
      options += ",";
      options += scalingOptions[ i ].option;
    }
  }
  options += "\n";

  if ( choice.center_scale_delta->currentIndex() == 0 )
  {
    options += "center_scale_delta=1\n";
  }

  if ( choice.scale_over -> currentIndex() != 0 )
  {
    options += "scale_over=";
    options +=  choice.scale_over -> currentText();
    options += "\n";
  }

  options += "dps_plot_stat=none";
  buttons = plotsButtonGroup->buttons();
  for ( int i=0; plotOptions[ i ].label; i++ )
  {
    if ( buttons.at( i )->isChecked() )
    {
      options += ",";
      options += plotOptions[ i ].option;
    }
  }
  options += "\n";

  options += "dps_plot_points=" + choice.plots_points -> currentText() + "\n";
  options += "dps_plot_step=" + choice.plots_step -> currentText() + "\n";

  options += "reforge_plot_stat=none";
  buttons = reforgeplotsButtonGroup->buttons();
  for ( int i=0; reforgePlotOptions[ i ].label; i++ )
  {
    if ( buttons.at( i )->isChecked() )
    {
      options += ",";
      options += reforgePlotOptions[ i ].option;
    }
  }
  options += "\n";

  options += "reforge_plot_amount=" + choice.reforgeplot_amount -> currentText() + "\n";
  options += "reforge_plot_step=" + choice.reforgeplot_step -> currentText() + "\n";
  options += "reforge_plot_output_file=reforge_plot.csv\n"; // This should be set in the gui if possible
  if ( choice.statistics_level->currentIndex() >= 0 )
  {
    options += "statistics_level=" + choice.statistics_level->currentText() + "\n";
  }

  if ( choice.report_pets->currentIndex() != 1 )
  {
    options += "report_pets_separately=1\n";
  }
  if ( choice.print_style -> currentIndex() != 0 )
  {
    options += "print_styles=";
    options += util::to_string( choice.print_style -> currentIndex() ).c_str();
    options += "\n";
  }
  options += "\n";

  options += simulateText->toPlainText();
  options += "\n";

  if ( choice.num_target -> currentIndex() >= 1 )
    for ( unsigned int i = 1; i <= static_cast<unsigned int>( choice.num_target -> currentIndex() + 1 ); ++i )
    {
      options += "enemy=enemy";
      options += QString::number( i );
      options += "\n";
    }

  options += overridesText->toPlainText();
  options += "\n";

  options += cmdLine->text();
  options += "\n";

#if SC_USE_PTR
  if ( choice.version->currentIndex() == 2 )
  {
    options += "ptr=1\n";
    options += "copy=EvilTwinPTR\n";
    options += "ptr=0\n";
  }
#endif

  if ( choice.debug->currentIndex() != 0 )
  {
    options += "log=1\n";
    options += "scale_only=none\n";
    options += "dps_plot_stat=none\n";
  }
  return options;
}

void SC_MainWindow::simulateFinished()
{
  timer -> stop();
  simPhase = "%p%";
  simProgress = 100;
  progressBar -> setFormat( simPhase.c_str() );
  progressBar -> setValue( simProgress );
  QFile file( sim -> html_file_str.c_str() );
  deleteSim();
  if ( ! simulateThread -> success )
  {
    logText -> setformat_error();
    logText -> appendPlainText( "Simulation failed!\n" );
    logText -> moveCursor( QTextCursor::End );
    logText -> resetformat();
    mainTab -> setCurrentTab( TAB_LOG );
  }
  else if ( file.open( QIODevice::ReadOnly | QIODevice::Text ) )
  {
    QString resultsName = QString( "Results %1" ).arg( ++simResults );
    SC_WebView* resultsView = new SC_WebView( this, resultsTab, QString::fromUtf8( file.readAll() ) );
    resultsView -> loadHtml();
    resultsTab -> addTab( resultsView, resultsName );
    resultsTab -> setCurrentWidget( resultsView );
    resultsView->setFocus();
    mainTab->setCurrentTab( choice.debug->currentIndex() ? TAB_LOG : TAB_RESULTS );
  }
  else
  {
    logText -> setformat_error();
    logText -> appendPlainText( "Unable to open html report!\n" );
    logText -> resetformat();
    mainTab -> setCurrentTab( TAB_LOG );
  }
}

#ifdef SC_PAPERDOLL
void SC_MainWindow::paperdollFinished()
{
  timer -> stop();
  simPhase = "%p%";
  simProgress = 100;
  progressBar -> setFormat( simPhase.c_str() );
  progressBar -> setValue( simProgress );

  assert(  sim );

  simProgress = 100;

  timer -> start( 100 );
}
#endif

// ==========================================================================
// Save Results
// ==========================================================================

void SC_MainWindow::saveLog()
{
  logCmdLineHistory.add( cmdLine->text() );

  QFile file( cmdLine->text() );

  if ( file.open( QIODevice::WriteOnly | QIODevice::Text ) )
  {
    file.write( logText -> toPlainText().toUtf8() );
    file.close();
  }

  logText->appendPlainText( QString( "Log saved to: %1\n" ).arg( cmdLine->text() ) );
}

void SC_MainWindow::saveResults()
{
  int index = resultsTab -> currentIndex();
  if ( index <= 0 ) return;

  if ( visibleWebView -> url().toString() != "about:blank" ) return;

  resultsCmdLineHistory.add( cmdLine -> text() );

  QFile file( cmdLine -> text() );

  if ( file.open( QIODevice::WriteOnly | QIODevice::Text ) )
  {
    file.write( visibleWebView -> html_str.toUtf8() );
    file.close();
  }

  logText -> appendPlainText( QString( "Results saved to: %1\n" ).arg( cmdLine->text() ) );
}

// ==========================================================================
// Window Events
// ==========================================================================

void SC_MainWindow::closeEvent( QCloseEvent* e )
{
  saveHistory();
  battleNetView -> stop();
  QCoreApplication::quit();
  e -> accept();
}

void SC_MainWindow::cmdLineTextEdited( const QString& s )
{
  switch ( mainTab -> currentTab() )
  {
  case TAB_WELCOME:   cmdLineText = s; break;
  case TAB_OPTIONS:   cmdLineText = s; break;
  case TAB_SIMULATE:  cmdLineText = s; break;
  case TAB_OVERRIDES: cmdLineText = s; break;
  case TAB_HELP:      cmdLineText = s; break;
  case TAB_SITE:      cmdLineText = s; break;
  case TAB_LOG:       logFileText = s; break;
  case TAB_RESULTS:   resultsFileText = s; break;
  case TAB_IMPORT:    break;
  }
}

void SC_MainWindow::cmdLineReturnPressed()
{
  if ( mainTab -> currentTab() == TAB_IMPORT )
  {
    if ( cmdLine->text().count( "battle.net" ) ||
         cmdLine->text().count( "battlenet.com" ) )
    {
      battleNetView -> setUrl( QUrl::fromUserInput( cmdLine -> text() ) );
      importTab -> setCurrentTab( TAB_BATTLE_NET );
    }
    else if ( cmdLine->text().count( "chardev.org" ) )
    {
      charDevView -> setUrl( QUrl::fromUserInput( cmdLine -> text() ) );
      importTab -> setCurrentTab( TAB_CHAR_DEV );
    }
    else
    {
      if ( ! sim ) mainButtonClicked( true );
    }
  }
  else
  {
    if ( ! sim ) mainButtonClicked( true );
  }
}

void SC_MainWindow::mainButtonClicked( bool /* checked */ )
{
  switch ( mainTab -> currentTab() )
  {
  case TAB_WELCOME:   startSim(); break;
  case TAB_OPTIONS:   startSim(); break;
  case TAB_SIMULATE:  startSim(); break;
  case TAB_OVERRIDES: startSim(); break;
  case TAB_HELP:      startSim(); break;
  case TAB_SITE:      startSim(); break;
  case TAB_IMPORT:
    switch ( importTab -> currentTab() )
    {
    case TAB_BATTLE_NET: startImport( TAB_BATTLE_NET, cmdLine->text() ); break;
    case TAB_CHAR_DEV:   startImport( TAB_CHAR_DEV,   cmdLine->text() ); break;
    case TAB_RAWR:       startImport( TAB_RAWR,       "Rawr XML"      ); break;
    default: break;
    }
    break;
  case TAB_LOG: saveLog(); break;
  case TAB_RESULTS: saveResults(); break;
  }
}

void SC_MainWindow::backButtonClicked( bool /* checked */ )
{
  if ( visibleWebView )
  {
    if ( mainTab -> currentTab() == TAB_RESULTS && ! visibleWebView->history()->canGoBack() )
    {
//        visibleWebView->setHtml( resultsHtml[ resultsTab->indexOf( visibleWebView ) ] );
      visibleWebView -> loadHtml();

      QWebHistory* h = visibleWebView->history();
      visibleWebView->history()->clear(); // This is not appearing to work.
      h->setMaximumItemCount( 0 );
      h->setMaximumItemCount( 100 );
    }
    else
    {
      visibleWebView->back();
    }
    visibleWebView->setFocus();
  }
  else
  {
    switch ( mainTab -> currentTab() )
    {
    case TAB_OPTIONS:   decodeOptions( optionsHistory.backwards() ); break;
    case TAB_SIMULATE:   simulateText->setPlainText(  simulateTextHistory.backwards() );  simulateText->setFocus(); break;
    case TAB_OVERRIDES: overridesText->setPlainText( overridesTextHistory.backwards() ); overridesText->setFocus(); break;
    default:            break;
    }
  }
}

void SC_MainWindow::forwardButtonClicked( bool /* checked */ )
{
  if ( visibleWebView )
  {
    visibleWebView->forward();
    visibleWebView->setFocus();
  }
  else
  {
    switch ( mainTab->currentIndex() )
    {
    case TAB_WELCOME:   break;
    case TAB_OPTIONS:   decodeOptions( optionsHistory.forwards() ); break;
    case TAB_IMPORT:    break;
    case TAB_SIMULATE:   simulateText->setPlainText(  simulateTextHistory.forwards() );  simulateText->setFocus(); break;
    case TAB_OVERRIDES: overridesText->setPlainText( overridesTextHistory.forwards() ); overridesText->setFocus(); break;
    case TAB_LOG:       break;
    case TAB_RESULTS:   break;
    }
  }
}

void SC_MainWindow::rawrButtonClicked( bool /* checked */ )
{
  QFileDialog dialog( this );
  dialog.setFileMode( QFileDialog::ExistingFile );
  dialog.setNameFilter( "Rawr Profiles (*.xml)" );
  dialog.restoreState( rawrDialogState );
  if ( dialog.exec() )
  {
    rawrDialogState = dialog.saveState();
    QStringList fileList = dialog.selectedFiles();
    if ( ! fileList.empty() )
    {
      QFile rawrFile( fileList.at( 0 ) );
      if ( rawrFile.open( QIODevice::ReadOnly | QIODevice::Text ) )
      {
        QTextStream in( &rawrFile );
        in.setCodec( "UTF-8" );
        in.setAutoDetectUnicode( true );
        rawrText->setPlainText( in.readAll() );
        rawrText->moveCursor( QTextCursor::Start );
      }
    }
  }
}

void SC_MainWindow::mainTabChanged( int index )
{
  visibleWebView = 0;
  switch ( index )
  {
  case TAB_WELCOME:   cmdLine->setText( cmdLineText ); mainButton->setText( sim ? "Cancel!" : "Simulate!" ); break;
  case TAB_OPTIONS:   cmdLine->setText( cmdLineText ); mainButton->setText( sim ? "Cancel!" : "Simulate!" ); break;
  case TAB_SIMULATE:  cmdLine->setText( cmdLineText ); mainButton->setText( sim ? "Cancel!" : "Simulate!" ); break;
  case TAB_OVERRIDES: cmdLine->setText( cmdLineText ); mainButton->setText( sim ? "Cancel!" : "Simulate!" ); break;
  case TAB_HELP:      cmdLine->setText( cmdLineText ); mainButton->setText( sim ? "Cancel!" : "Simulate!" ); break;
  case TAB_LOG:       cmdLine->setText( logFileText ); mainButton->setText( "Save!" ); break;
  case TAB_IMPORT:
    mainButton->setText( sim ? "Cancel!" : "Import!" );
    importTabChanged( importTab->currentIndex() );
    break;
  case TAB_RESULTS:
    cmdLine -> setText( resultsFileText ); mainButton -> setText( "Save!" );
    resultsTabChanged( resultsTab -> currentIndex() );
    break;
  case TAB_SITE:
    cmdLine->setText( cmdLineText );
    mainButton->setText( sim ? "Cancel!" : "Simulate!" );
    updateVisibleWebView( siteView );
    break;
#ifdef SC_PAPERDOLL
  case TAB_PAPERDOLL:
    break;
#endif
  default: assert( 0 );
  }
  if ( visibleWebView )
  {
    progressBar->setFormat( "%p%" );
  }
  else
  {
    progressBar->setFormat( simPhase.c_str() );
    progressBar->setValue( simProgress );
  }
}

void SC_MainWindow::importTabChanged( int index )
{
  if ( index == TAB_RAWR    ||
       index == TAB_BIS     ||
       index == TAB_CUSTOM  ||
       index == TAB_HISTORY )
  {
    visibleWebView = 0;
    progressBar->setFormat( simPhase.c_str() );
    progressBar->setValue( simProgress );
    cmdLine->setText( "" );
  }
  else
  {
    updateVisibleWebView( debug_cast<SC_WebView*>( importTab -> widget( index ) ) );
  }
}

void SC_MainWindow::resultsTabChanged( int index )
{
  if ( index <= 0 )
  {
    cmdLine -> setText( "" );
  }
  else
  {
    updateVisibleWebView( debug_cast<SC_WebView*>( resultsTab -> widget( index ) ) );
    cmdLine -> setText( resultsFileText );
  }
}

void SC_MainWindow::resultsTabCloseRequest( int index )
{
  if ( index <= 0 )
  {
    // Ignore attempts to close Legend
  }
  else
  {
    resultsTab -> removeTab( index );
  }
}

void SC_MainWindow::historyDoubleClicked( QListWidgetItem* item )
{
  QString text = item -> text();
  QString url = text.section( ' ', 1, 1, QString::SectionSkipEmpty );

  if ( url.count( "battle.net" ) || url.count( "battlenet.com" ) )
  {
    battleNetView -> setUrl( url );
    importTab -> setCurrentIndex( TAB_BATTLE_NET );
  }
  else if ( url.count( "chardev.org" ) )
  {
    charDevView -> setUrl( url );
    importTab -> setCurrentIndex( TAB_CHAR_DEV );
  }
  else
  {
    //importTab->setCurrentIndex( TAB_RAWR );
  }
}

void SC_MainWindow::bisDoubleClicked( QTreeWidgetItem* item, int /* col */ )
{
  QString profile = item -> text( 1 );
  QString s = "Unable to import profile " + profile;

  QFile file( profile );
  if ( file.open( QIODevice::ReadOnly | QIODevice::Text ) )
  {
    s = QString::fromUtf8( file.readAll() );
    file.close();
  }

  simulateText->setPlainText( s );
  simulateTextHistory.add( s );
  mainTab->setCurrentTab( TAB_SIMULATE );
  simulateText->setFocus();
}

void SC_MainWindow::allBuffsChanged( bool checked )
{
  QList<QAbstractButton*> buttons = buffsButtonGroup -> buttons();
  int count = buttons.count();
  for ( int i = 1; i < count; i++ )
  {
    buttons.at( i ) -> setChecked( checked );
  }
}

void SC_MainWindow::allDebuffsChanged( bool checked )
{
  QList<QAbstractButton*> buttons = debuffsButtonGroup->buttons();
  int count = buttons.count();
  for ( int i = 1; i < count; i++ )
  {
    buttons.at( i ) -> setChecked( checked );
  }
}

void SC_MainWindow::allScalingChanged( bool checked )
{
  QList<QAbstractButton*> buttons = scalingButtonGroup->buttons();
  int count = buttons.count();
  for ( int i=2; i < count - 1; i++ )
  {
    buttons.at( i ) -> setChecked( checked );
  }
}

void SC_MainWindow::armoryRegionChanged( const QString& region )
{
  battleNetView -> stop();
  battleNetView -> setUrl( "http://" + region + ".battle.net/wow/en" );
}

// ==========================================================================
// SimulateThread
// ==========================================================================

void SimulateThread::run()
{
  QByteArray utf8_profile = options.toUtf8();
  QFile file( "simc_gui.simc" );
  if ( file.open( QIODevice::WriteOnly | QIODevice::Text ) )
  {
    file.write( utf8_profile );
    file.close();
  }

  sim -> html_file_str = "simc_report.html";
  sim -> xml_file_str  = "simc_report.xml";

  sim_control_t description;

  success = description.options.parse_text( utf8_profile.constData() );

  if ( success )
  {
    success = sim -> setup( &description );
  }
  if ( success )
  {
    success = sim -> execute();
  }
  if ( success )
  {
    sim -> scaling -> analyze();
    sim -> plot -> analyze();
    sim -> reforge_plot -> analyze();
    report::print_suite( sim );
  }
}

// ============================================================================
// SC_CommandLine
// ============================================================================

void SC_CommandLine::keyPressEvent( QKeyEvent* e )
{
  int k = e->key();
  if ( k != Qt::Key_Up && k != Qt::Key_Down )
  {
    QLineEdit::keyPressEvent( e );
    return;
  }
  switch ( mainWindow -> mainTab -> currentTab() )
  {
  case TAB_WELCOME:
  case TAB_OPTIONS:
  case TAB_SIMULATE:
  case TAB_HELP:
  case TAB_SITE:
  case TAB_OVERRIDES:
    mainWindow->cmdLineText = mainWindow->simulateCmdLineHistory.next( k );
    setText( mainWindow->cmdLineText );
    break;
  case TAB_IMPORT:
    break;
  case TAB_LOG:
    mainWindow->logFileText = mainWindow->logCmdLineHistory.next( k );
    setText( mainWindow->logFileText );
    break;
  case TAB_RESULTS:
    mainWindow -> resultsFileText = mainWindow-> resultsCmdLineHistory.next( k );
    setText( mainWindow -> resultsFileText );
    break;
  }
}

// ==========================================================================
// SC_ReforgeButtonGroup
// ==========================================================================

SC_ReforgeButtonGroup::SC_ReforgeButtonGroup( QObject* parent ) :
  QButtonGroup( parent ), selected( 0 )
{}

void SC_ReforgeButtonGroup::setSelected( int state )
{
  if ( state )
    selected++;
  else
    selected--;

  // Three selected, disallow selection of any more
  if ( selected >= 3 )
  {
    QList< QAbstractButton* > b = buttons();
    for ( QList< QAbstractButton* >::iterator i = b.begin(); i != b.end(); i++ )
      if ( ! ( *i ) -> isChecked() )
        ( *i ) -> setEnabled( false );
  }
  // Less than three selected, allow selection of all/any
  else
  {
    QList< QAbstractButton* > b = buttons();
    for ( QList< QAbstractButton* >::iterator i = b.begin(); i != b.end(); i++ )
      ( *i ) -> setEnabled( true );
  }
}

// ==========================================================================
// PersistentCookieJar
// ==========================================================================

void PersistentCookieJar::save()
{
  QFile file( fileName );
  if ( file.open( QIODevice::WriteOnly ) )
  {
    QDataStream out( &file );
    QList<QNetworkCookie> cookies = allCookies();
    qint32 count = ( qint32 ) cookies.count();
    out << count;
    for ( int i=0; i < count; i++ )
    {
      const QNetworkCookie& c = cookies.at( i );
      out << c.name();
      out << c.value();
    }
    file.close();
  }
}

void PersistentCookieJar::load()
{
  QFile file( fileName );
  if ( file.open( QIODevice::ReadOnly ) )
  {
    QDataStream in( &file );
    QList<QNetworkCookie> cookies;
    qint32 count;
    in >> count;
    for ( int i=0; i < count; i++ )
    {
      QByteArray name, value;
      in >> name;
      in >> value;
      cookies.append( QNetworkCookie( name, value ) );
    }
    setAllCookies( cookies );
    file.close();
  }
}

#ifdef SC_PAPERDOLL

void PaperdollThread::run()
{
  cache::advance_era();

  sim -> iterations = 100;

  QStringList stringList = options.split( '\n', QString::SkipEmptyParts );

  std::vector<std::string> args;
  for ( int i=0; i < stringList.count(); ++i )
    args.push_back( stringList[ i ].toUtf8().constData() );
  sim_control_t description;

  success = description.options.parse_args( args );

  if ( success )
  {
    success = sim -> setup( &description );
  }
  if ( success )
  {
    for ( unsigned int i = 0; i < 10; ++i )
    {
      sim -> current_iteration = 0;
      sim -> execute();
      mainWindow -> paperdoll -> setCurrentDPS( player-> dps.mean, ( player-> dps.mean_std_dev * sim -> confidence_estimator ) );
    }
  }
}
#endif
