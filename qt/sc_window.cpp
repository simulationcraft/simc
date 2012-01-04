// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraftqt.h"
#ifdef SC_PAPERDOLL
#include "simcpaperdoll.h"
#endif
#include <QtWebKit>
#ifdef Q_WS_MAC
#include <CoreFoundation/CoreFoundation.h>
#endif

// ==========================================================================
// Utilities
// ==========================================================================

struct OptionEntry
{
  const char* label;
  const char* option;
  const char* tooltip;
};

static OptionEntry* getBuffOptions()
{
  static OptionEntry options[] =
  {
    { "Toggle All Buffs",       "",                                "Toggle all buffs on/off"                                                                         },
    { "Dark Intent",            "override.dark_intent",            "Dark Intent"                                                                                     },
    { "Focus Magic",            "override.focus_magic",            "Focus Magic"                                                                                     },
    { "Agility and Strength",   "override.strength_of_earth",      "Battle Shout\nHorn of Winter\nRoar of Courage\nStrength of Earth Totem"                          },
    { "All Damage",             "override.communion",              "Arcane Tactics\nCommunion\nFerocious Inspiration"                                                },
    { "Armor",                  "override.devotion_aura",          "Devotion Aura\nStoneskin Totem"                                                                  },
    { "Attack Power (%)",       "override.blessing_of_might",      "Abomination's Might\nBlessing of Might\nTrueshot Aura\nUnleashed Rage"                           },
    { "Bloodlust",              "override.bloodlust",              "Ancient Hysteria\nBloodlust\nHeroism\nTime Warp"                                                 },
    { "Critical Strike",        "override.leader_of_the_pack",     "Elemental Oath\nFurious Howl\nHonor Among Thieves\nLeader of the Pack\nRampage\nTerrifying Roar" },
    { "Mana",                   "override.arcane_brilliance",      "Arcane Brilliance\nFel Intelligence"                                                             },
    { "Mana Regen",             "override.mana_spring_totem",      "Blessing of Might\nFel Intelligence\nMana Spring Totem"                                          },
    { "Melee and Ranged Haste", "override.windfury_totem",         "Hunting Party\nImproved Icy Talons\nWindfury Totem"                                              },
    { "Replenishment",          "override.replenishment",          "Communion\nEnduring Winter\nReviatalize\nSoul Leech\nVampiric Touch"                             },
    { "Spell Haste",            "override.wrath_of_air",           "Moonkin Form\nShadowform\nWrath of Air Totem"                                                    },
    { "Spell Power 6%",         "override.arcane_brilliance",      "Arcane Brilliance\nFlametongue Totem"                                                            },
    { "Spell Power 10%",        "override.demonic_pact",           "Demonic Pact\nTotemic Wrath"                                                                     },
    { "Stamina",                "override.fortitude",              "Blood Pact\nCommanding Shout\nPower Word: Fortitude\nQiraji Fortitude"                           },
    { "Stat Multiplier",        "override.blessing_of_kings",      "Blessing of Kings\nEmbrace of the Shale Spider\nMark of the Wild"                                },
    { NULL, NULL, NULL }
  };
  return options;
}

static OptionEntry* getItemSourceOptions()
{
  static OptionEntry options[] =
  {
    { "Local Item Database", "local",   "Use Simulationcraft item database" },
    { "Blizzard API",        "bcpapi",  "Remote Blizzard Community Platform API source" },
    { "Wowhead.com",         "wowhead", "Remote Wowhead.com item data source" },
    { "Mmo-champion.com",    "mmoc",    "Remote Mmo-champion.com item data source" },
    { "Blizzard Armory",     "armory",  "Remote item database from Blizzard (DEPRECATED, SHOULD NOT BE USED)" },
    { "Wowhead.com (PTR)",   "ptrhead", "Remote Wowhead.com PTR item data source" },
    { NULL, NULL, NULL }
  };

  return options;
}

static OptionEntry* getDebuffOptions()
{
  static OptionEntry options[] =
  {
    { "Toggle All Debuffs",     "",                               "Toggle all debuffs on/off"                                                                                     },
    { "Armor Reduction",        "override.sunder_armor",          "Corrosive Spit\nExpose Armor\nFaerie Fire\nSunder Armor\nTear Armor"                                           },
    { "Bleed Damage",           "override.mangle",                "Blood Frenzy\nGore\nHemorrhage\nMangle\nStampede\nTendon Rip"                                                  },
    { "Bleeding",               "override.bleeding",              "Rip\nRupture\nPiercing Shots"                                                                                  },
    { "Physical Damage Done",   "override.demoralizing_roar",     "Curse of Weakness\nDemoralizing Roar\nDemoralizing Shout\nScarlet Fever\nVindication"                          },
    { "Physical Damage Taken",  "override.blood_frenzy_physical", "Acid Spit\nBlood Frenzy\nBrittle Bones\nRavage\nSavage Combat"                                                 },
    { "Poisoned",               "override.poisoned",              "Deadly Poison\nSerpent Sting"                                                                                  },
    { "Ranged Attack Power",    "override.hunters_mark",          "Hunter's Mark"                                                                                                 },
    { "Reduced Attack Speed",   "override.thunder_clap",          "Dust Cloud\nEarth Shock\nFrost Fever\nInfected Wounds\nJudgements of the Just\nTailspin\nThunder Clap\nWaylay" },
    { "Spell Critical Strike",  "override.critical_mass",         "Critical Mass\nShadow and Flame"                                                                               },
    { "Spell Damage",           "override.earth_and_moon",        "Curse of the Elements\nEarth and Moon\nEbon Plaguebriger\nFire Breath\nLightning Breath\nMaster Poisoner"      },
    { NULL, NULL, NULL }
  };
  return options;
}

static OptionEntry* getScalingOptions()
{
  static OptionEntry options[] =
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
    { NULL, NULL, NULL }
  };
  return options;
}

static OptionEntry* getPlotOptions()
{
  static OptionEntry options[] =
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
  return options;
}

static OptionEntry* getReforgePlotOptions()
{
  static OptionEntry options[] =
  {
    { "Plot Reforge Options for Spirit",            "spi",     "Generate reforge plot data for Spirit"           },
    { "Plot Reforge Options for Expertise Rating",  "exp",     "Generate reforge plot data for Expertise Rating" },
    { "Plot Reforge Options for Hit Rating",        "hit",     "Generate reforge plot data for Hit Rating"       },
    { "Plot Reforge Options for Crit Rating",       "crit",    "Generate reforge plot data for Crit Rating"      },
    { "Plot Reforge Options for Haste Rating",      "haste",   "Generate reforge plot data for Haste Rating"     },
    { "Plot Reforge Options for Mastery Rating",    "mastery", "Generate reforge plot data for Mastery Rating"   },
    { NULL, NULL, NULL }
  };
  return options;
}

static QString defaultSimulateText()
{
  return QString( "# Profile will be downloaded into here.\n"
                  "# Use the Back/Forward buttons to cycle through the script history.\n"
                  "# Use the Up/Down arrow keys to cycle through the command-line history.\n"
                  "#\n"
                  "# Clicking Simulate will create a simc_gui.simc profile for review.\n" );
}

static QComboBox* createChoice( int count, ... )
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

ReforgeButtonGroup::ReforgeButtonGroup( QObject* parent ) :
  QButtonGroup( parent ), selected( 0 )
{

}

void ReforgeButtonGroup::setSelected( int state )
{
  if ( state ) selected++; else selected--;

  // Three selected, disallow selection of any more
  if ( selected >= 3 )
  {
    QList< QAbstractButton* > b = buttons();
    for ( QList< QAbstractButton* >::iterator i = b.begin(); i != b.end(); i++ )
      if ( ! ( *i ) -> isChecked() ) ( *i ) -> setEnabled( false );
  }
  // Less than three selected, allow selection of all/any
  else
  {
    QList< QAbstractButton* > b = buttons();
    for ( QList< QAbstractButton* >::iterator i = b.begin(); i != b.end(); i++ )
      ( *i ) -> setEnabled( true );
  }
}

void SimulationCraftWindow::decodeOptions( QString encoding )
{
  QStringList tokens = encoding.split( ' ' );
  if( tokens.count() >= 14 )
  {
         versionChoice->setCurrentIndex( tokens[  0 ].toInt() );
      iterationsChoice->setCurrentIndex( tokens[  1 ].toInt() );
     fightLengthChoice->setCurrentIndex( tokens[  2 ].toInt() );
   fightVarianceChoice->setCurrentIndex( tokens[  3 ].toInt() );
      fightStyleChoice->setCurrentIndex( tokens[  4 ].toInt() );
      targetRaceChoice->setCurrentIndex( tokens[  5 ].toInt() );
     playerSkillChoice->setCurrentIndex( tokens[  6 ].toInt() );
         threadsChoice->setCurrentIndex( tokens[  7 ].toInt() );
    armoryRegionChoice->setCurrentIndex( tokens[  8 ].toInt() );
      armorySpecChoice->setCurrentIndex( tokens[  9 ].toInt() );
     defaultRoleChoice->setCurrentIndex( tokens[ 10 ].toInt() );
         latencyChoice->setCurrentIndex( tokens[ 11 ].toInt() );
     targetLevelChoice->setCurrentIndex( tokens[ 12 ].toInt() );
  }

  QList<QAbstractButton*>       buff_buttons  =        buffsButtonGroup->buttons();
  QList<QAbstractButton*>     debuff_buttons  =      debuffsButtonGroup->buttons();
  QList<QAbstractButton*>    scaling_buttons  =      scalingButtonGroup->buttons();
  QList<QAbstractButton*>        plot_buttons =        plotsButtonGroup->buttons();
  QList<QAbstractButton*> reforgeplot_buttons = reforgeplotsButtonGroup->buttons();

  OptionEntry*        buffs = getBuffOptions();
  OptionEntry*      debuffs = getDebuffOptions();
  OptionEntry*      scaling = getScalingOptions();
  OptionEntry*        plots = getPlotOptions();
  OptionEntry* reforgeplots = getReforgePlotOptions();

  for( int i = 13; i < tokens.count(); i++ )
  {
    QStringList opt_tokens = tokens[ i ].split( ':' );

    OptionEntry* options=0;
    QList<QAbstractButton*>* buttons=0;

    if(      ! opt_tokens[ 0 ].compare( "buff"           ) ) { options = buffs;           buttons = &buff_buttons;        }
    else if( ! opt_tokens[ 0 ].compare( "debuff"         ) ) { options = debuffs;         buttons = &debuff_buttons;      }
    else if( ! opt_tokens[ 0 ].compare( "scaling"        ) ) { options = scaling;         buttons = &scaling_buttons;     }
    else if( ! opt_tokens[ 0 ].compare( "plots"          ) ) { options = plots;           buttons = &plot_buttons;        }
    else if( ! opt_tokens[ 0 ].compare( "reforge_plots"  ) ) { options = reforgeplots;    buttons = &reforgeplot_buttons; }
    else if( ! opt_tokens[ 0 ].compare( "item_db_source" ) )
    {
      QStringList item_db_list = opt_tokens[ 1 ].split( '/' );
      QListWidgetItem** items = new QListWidgetItem *[item_db_list.size()];

      for ( int opt = 0; opt < item_db_list.size(); opt++ )
      {
        for ( int source = 0; itemDbOrder -> count(); source++ )
        {
          if ( ! item_db_list[ opt ].compare( itemDbOrder -> item( source ) -> data( Qt::UserRole ).toString() ) )
          {
            items[ opt ] = itemDbOrder -> takeItem( source );
            break;
          }
        }
      }

      for ( int j = 0; j < item_db_list.size(); j++ )
        itemDbOrder -> addItem( items[ j ] );

      delete [] items;
    }
    if ( ! options ) continue;

    QStringList opt_value = opt_tokens[ 1 ].split( '=' );
    for( int opt=0; options[ opt ].label; opt++ )
    {
      if( ! opt_value[ 0 ].compare( options[ opt ].option ) )
      {
        buttons -> at( opt )->setChecked( 1 == opt_value[ 1 ].toInt() );
        break;
      }
    }
  }
}

QString SimulationCraftWindow::encodeOptions()
{
  QString encoded = QString( "%1 %2 %3 %4 %5 %6 %7 %8 %9 %10 %11 %12 %13" )
    .arg(       versionChoice->currentIndex() )
    .arg(    iterationsChoice->currentIndex() )
    .arg(   fightLengthChoice->currentIndex() )
    .arg( fightVarianceChoice->currentIndex() )
    .arg(    fightStyleChoice->currentIndex() )
    .arg(    targetRaceChoice->currentIndex() )
    .arg(   playerSkillChoice->currentIndex() )
    .arg(       threadsChoice->currentIndex() )
    .arg(  armoryRegionChoice->currentIndex() )
    .arg(    armorySpecChoice->currentIndex() )
    .arg(   defaultRoleChoice->currentIndex() )
    .arg(       latencyChoice->currentIndex() )
    .arg(   targetLevelChoice->currentIndex() )
    ;

  QList<QAbstractButton*> buttons = buffsButtonGroup->buttons();
  OptionEntry* buffs = getBuffOptions();
  for( int i=1; buffs[ i ].label; i++ )
  {
    encoded += " buff:";
    encoded += buffs[ i ].option;
    encoded += "=";
    encoded += buttons.at( i )->isChecked() ? "1" : "0";
  }

  buttons = debuffsButtonGroup->buttons();
  OptionEntry* debuffs = getDebuffOptions();
  for( int i=1; debuffs[ i ].label; i++ )
  {
    encoded += " debuff:";
    encoded += debuffs[ i ].option;
    encoded += "=";
    encoded += buttons.at( i )->isChecked() ? "1" : "0";
  }

  buttons = scalingButtonGroup->buttons();
  OptionEntry* scaling = getScalingOptions();
  for( int i=2; scaling[ i ].label; i++ )
  {
    encoded += " scaling:";
    encoded += scaling[ i ].option;
    encoded += "=";
    encoded += buttons.at( i )->isChecked() ? "1" : "0";
  }

  buttons = plotsButtonGroup->buttons();
  OptionEntry* plots = getPlotOptions();
  for( int i=0; plots[ i ].label; i++ )
  {
    encoded += " plots:";
    encoded += plots[ i ].option;
    encoded += "=";
    encoded += buttons.at( i )->isChecked() ? "1" : "0";
  }

  buttons = reforgeplotsButtonGroup->buttons();
  OptionEntry* reforgeplots = getReforgePlotOptions();
  for( int i=0; reforgeplots[ i ].label; i++ )
  {
    encoded += " reforge_plots:";
    encoded += reforgeplots[ i ].option;
    encoded += "=";
    encoded += buttons.at( i )->isChecked() ? "1" : "0";
  }

  if ( itemDbOrder -> count() > 0 )
  {
    encoded += " item_db_source:";
    for ( int i = 0; i < itemDbOrder -> count(); i++ )
    {
      QListWidgetItem *it = itemDbOrder -> item( i );
      encoded += it -> data( Qt::UserRole ).toString();
      if ( i < itemDbOrder -> count() - 1 )
        encoded += "/";
    }
  }

  return encoded;
}

void SimulationCraftWindow::updateSimProgress()
{
  if( sim )
  {
    simProgress = ( int ) ( 100.0 * sim->progress( simPhase ) );
  }
  else
  {
    simPhase = "%p%";
    simProgress = 100;
  }
  if( mainTab->currentIndex() != TAB_IMPORT &&
      mainTab->currentIndex() != TAB_RESULTS )
  {
    progressBar->setFormat( QString::fromUtf8( simPhase.c_str() ) );
    progressBar->setValue( simProgress );
  }
}

void SimulationCraftWindow::loadHistory()
{
  http_t::cache_load();
  QFile file( "simc_history.dat" );
  if( file.open( QIODevice::ReadOnly ) )
  {
    QDataStream in( &file );
    QString historyVersion;
    in >> historyVersion;
    if( historyVersion != HISTORY_VERSION ) return;
    in >> historyWidth;
    in >> historyHeight;
    in >> historyMaximized;
    QStringList importHistory;
    in >> simulateCmdLineHistory;
    in >> logCmdLineHistory;
    in >> resultsCmdLineHistory;
    in >> optionsHistory;
    in >> simulateTextHistory;
    in >> overridesTextHistory;
    in >> importHistory;
    file.close();

    int count = importHistory.count();
    for( int i=0; i < count; i++ )
    {
      QListWidgetItem* item = new QListWidgetItem( importHistory.at( i ) );
      historyList->addItem( item );
    }

    decodeOptions( optionsHistory.backwards() );

    QString s = overridesTextHistory.backwards();
    if( ! s.isEmpty() ) overridesText->setPlainText( s );
  }
}

void SimulationCraftWindow::saveHistory()
{
  charDevCookies->save();
  http_t::cache_save();
  QFile file( "simc_history.dat" );
  if( file.open( QIODevice::WriteOnly ) )
  {
    optionsHistory.add( encodeOptions() );

    QStringList importHistory;
    int count = historyList->count();
    for( int i=0; i < count; i++ )
    {
      importHistory.append( historyList->item( i )->text() );
    }

    QDataStream out( &file );
    out << QString( HISTORY_VERSION );
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

SimulationCraftWindow::SimulationCraftWindow( QWidget *parent )
  : QWidget( parent ),
    historyWidth( 0 ), historyHeight( 0 ), historyMaximized( 1 ),
    visibleWebView( 0 ), sim( 0 ), simPhase( "%p%" ), simProgress( 100 ), simResults( 0 )
{
  cmdLineText = "";
#ifndef Q_WS_MAC
  logFileText = "log.txt";
  resultsFileText = "results.html";
#else
  logFileText = QDir::currentPath() + QDir::separator() + "log.txt";
  resultsFileText = QDir::currentPath() + QDir::separator() + "results.html";
#endif

  mainTab = new QTabWidget();
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
  vLayout->addWidget( mainTab );
  vLayout->addWidget( cmdLineGroupBox );
  setLayout( vLayout );

  timer = new QTimer( this );
  connect( timer, SIGNAL( timeout() ), this, SLOT( updateSimProgress() ) );

  importThread = new ImportThread( this );
  simulateThread = new SimulateThread( this );

  connect(   importThread, SIGNAL( finished() ), this, SLOT(  importFinished() ) );
  connect( simulateThread, SIGNAL( finished() ), this, SLOT( simulateFinished() ) );

  setAcceptDrops( true );

  loadHistory();
}

void SimulationCraftWindow::createCmdLine()
{
  QHBoxLayout* cmdLineLayout = new QHBoxLayout();
  cmdLineLayout->addWidget( backButton = new QPushButton( "<" ) );
  cmdLineLayout->addWidget( forwardButton = new QPushButton( ">" ) );
  cmdLineLayout->addWidget( cmdLine = new SimulationCraftCommandLine( this ) );
  cmdLineLayout->addWidget( progressBar = new QProgressBar() );
  cmdLineLayout->addWidget( mainButton = new QPushButton( "Simulate!" ) );
  backButton->setMaximumWidth( 30 );
  forwardButton->setMaximumWidth( 30 );
  progressBar->setStyle( new QPlastiqueStyle() );
  progressBar->setMaximum( 100 );
  progressBar->setMaximumWidth( 200 );
  progressBar->setMinimumWidth( 150 );
  QFont progfont( progressBar -> font() );
  progfont.setPointSize( 11 );
  progressBar->setFont( progfont );
  connect( backButton,    SIGNAL( clicked( bool ) ),   this, SLOT(    backButtonClicked() ) );
  connect( forwardButton, SIGNAL( clicked( bool ) ),   this, SLOT( forwardButtonClicked() ) );
  connect( mainButton,    SIGNAL( clicked( bool ) ),   this, SLOT(    mainButtonClicked() ) );
  connect( cmdLine,       SIGNAL( returnPressed() ),            this, SLOT( cmdLineReturnPressed() ) );
  connect( cmdLine,       SIGNAL( textEdited( const QString& ) ), this, SLOT( cmdLineTextEdited( const QString& ) ) );
  cmdLineGroupBox = new QGroupBox();
  cmdLineGroupBox->setLayout( cmdLineLayout );
}

void SimulationCraftWindow::createWelcomeTab()
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
  QString url = "file:///" + welcomeFile;

  QWebView* welcomeBanner = new QWebView();
  welcomeBanner->setUrl( url );
  mainTab->addTab( welcomeBanner, "Welcome" );
}

void SimulationCraftWindow::createOptionsTab()
{
  optionsTab = new QTabWidget();
  mainTab->addTab( optionsTab, "Options" );

  createGlobalsTab();
  createBuffsTab();
  createDebuffsTab();
  createScalingTab();
  createPlotsTab();
  createReforgePlotsTab();

  QAbstractButton* allBuffs   =   buffsButtonGroup->buttons().at( 0 );
  QAbstractButton* allDebuffs = debuffsButtonGroup->buttons().at( 0 );
  QAbstractButton* allScaling = scalingButtonGroup->buttons().at( 0 );

  connect( armoryRegionChoice, SIGNAL( currentIndexChanged( const QString& ) ), this, SLOT( armoryRegionChanged( const QString& ) ) );

  connect( allBuffs,   SIGNAL( toggled( bool ) ), this, SLOT( allBuffsChanged( bool ) )   );
  connect( allDebuffs, SIGNAL( toggled( bool ) ), this, SLOT( allDebuffsChanged( bool ) ) );
  connect( allScaling, SIGNAL( toggled( bool ) ), this, SLOT( allScalingChanged( bool ) ) );
}

void SimulationCraftWindow::createGlobalsTab()
{
  QFormLayout* globalsLayout = new QFormLayout();
  globalsLayout->setFieldGrowthPolicy( QFormLayout::FieldsStayAtSizeHint );
  globalsLayout->addRow(        "Version",       versionChoice = createChoice( 3, "Live", "PTR", "Both" ) );
  globalsLayout->addRow(     "Iterations",    iterationsChoice = createChoice( 5, "100", "1000", "10000", "25000", "50000" ) );
  globalsLayout->addRow(      "World Lag",       latencyChoice = createChoice( 3, "Low", "Medium", "High" ) );
  globalsLayout->addRow(   "Length (sec)",   fightLengthChoice = createChoice( 9, "100", "150", "200", "250", "300", "350", "400", "450", "500" ) );
  globalsLayout->addRow(    "Vary Length", fightVarianceChoice = createChoice( 3, "0%", "10%", "20%" ) );
  globalsLayout->addRow(    "Fight Style",    fightStyleChoice = createChoice( 3, "Patchwerk", "HelterSkelter", "Ultraxion" ) );
  globalsLayout->addRow(   "Target Level",   targetLevelChoice = createChoice( 3, "Raid Boss", "5-man heroic", "5-man normal" ) );
  globalsLayout->addRow(    "Target Race",    targetRaceChoice = createChoice( 7, "humanoid", "beast", "demon", "dragonkin", "elemental", "giant", "undead" ) );
  globalsLayout->addRow(   "Player Skill",   playerSkillChoice = createChoice( 4, "Elite", "Good", "Average", "Ouch! Fire is hot!" ) );
  globalsLayout->addRow(        "Threads",       threadsChoice = createChoice( 4, "1", "2", "4", "8" ) );
  globalsLayout->addRow(  "Armory Region",  armoryRegionChoice = createChoice( 5, "us", "eu", "tw", "cn", "kr" ) );
  globalsLayout->addRow(    "Armory Spec",    armorySpecChoice = createChoice( 2, "active", "inactive" ) );
  globalsLayout->addRow(   "Default Role",   defaultRoleChoice = createChoice( 4, "auto", "dps", "heal", "tank" ) );
  globalsLayout->addRow( "Generate Debug",         debugChoice = createChoice( 3, "None", "Log Only", "Gory Details" ) );
  iterationsChoice->setCurrentIndex( 1 );
  fightLengthChoice->setCurrentIndex( 7 );
  fightVarianceChoice->setCurrentIndex( 2 );
  QGroupBox* globalsGroupBox = new QGroupBox();
  globalsGroupBox->setLayout( globalsLayout );

  optionsTab->addTab( globalsGroupBox, "Globals" );

  createItemDataSourceSelector( globalsLayout );
}

void SimulationCraftWindow::createBuffsTab()
{
  QVBoxLayout* buffsLayout = new QVBoxLayout();
  buffsButtonGroup = new QButtonGroup();
  buffsButtonGroup->setExclusive( false );
  OptionEntry* buffs = getBuffOptions();
  for( int i=0; buffs[ i ].label; i++ )
  {
    QCheckBox* checkBox = new QCheckBox( buffs[ i ].label );
    if ( i>2 ) checkBox->setChecked( true );
    checkBox->setToolTip( buffs[ i ].tooltip );
    buffsButtonGroup->addButton( checkBox );
    buffsLayout->addWidget( checkBox );
  }
  buffsLayout->addStretch( 1 );
  QGroupBox* buffsGroupBox = new QGroupBox();
  buffsGroupBox->setLayout( buffsLayout );

  optionsTab->addTab( buffsGroupBox, "Buffs" );
}

void SimulationCraftWindow::createDebuffsTab()
{
  QVBoxLayout* debuffsLayout = new QVBoxLayout();
  debuffsButtonGroup = new QButtonGroup();
  debuffsButtonGroup->setExclusive( false );
  OptionEntry* debuffs = getDebuffOptions();
  for( int i=0; debuffs[ i ].label; i++ )
  {
    QCheckBox* checkBox = new QCheckBox( debuffs[ i ].label );
    if ( i>0 ) checkBox->setChecked( true );
    checkBox->setToolTip( debuffs[ i ].tooltip );
    debuffsButtonGroup->addButton( checkBox );
    debuffsLayout->addWidget( checkBox );
  }
  debuffsLayout->addStretch( 1 );
  QGroupBox* debuffsGroupBox = new QGroupBox();
  debuffsGroupBox->setLayout( debuffsLayout );

  optionsTab->addTab( debuffsGroupBox, "Debuffs" );
}

void SimulationCraftWindow::createScalingTab()
{
  QVBoxLayout* scalingLayout = new QVBoxLayout();
  scalingButtonGroup = new QButtonGroup();
  scalingButtonGroup->setExclusive( false );
  OptionEntry* scaling = getScalingOptions();
  for( int i=0; scaling[ i ].label; i++ )
  {
    QCheckBox* checkBox = new QCheckBox( scaling[ i ].label );
    checkBox->setToolTip( scaling[ i ].tooltip );
    scalingButtonGroup->addButton( checkBox );
    scalingLayout->addWidget( checkBox );
  }
  scalingLayout->addStretch( 1 );
  QGroupBox* scalingGroupBox = new QGroupBox();
  scalingGroupBox->setLayout( scalingLayout );

  optionsTab->addTab( scalingGroupBox, "Scaling" );
}

void SimulationCraftWindow::createPlotsTab()
{
  QVBoxLayout* plotsLayout = new QVBoxLayout();
  plotsButtonGroup = new QButtonGroup();
  plotsButtonGroup->setExclusive( false );
  OptionEntry* plots = getPlotOptions();
  for( int i=0; plots[ i ].label; i++ )
  {
    QCheckBox* checkBox = new QCheckBox( plots[ i ].label );
    checkBox->setToolTip( plots[ i ].tooltip );
    plotsButtonGroup->addButton( checkBox );
    plotsLayout->addWidget( checkBox );
  }
  plotsLayout->addStretch( 1 );
  QGroupBox* plotsGroupBox = new QGroupBox();
  plotsGroupBox->setLayout( plotsLayout );

  optionsTab->addTab( plotsGroupBox, "Plots" );
}

void SimulationCraftWindow::createReforgePlotsTab()
{
  QFormLayout* reforgePlotsLayout = new QFormLayout();
  reforgePlotsLayout -> setFieldGrowthPolicy( QFormLayout::FieldsStayAtSizeHint );

  // Create Combo Boxes
  plotAmountChoice = createChoice( 5, "100", "200", "300", "400", "500" );
  plotAmountChoice -> setCurrentIndex( 1 ); // Default is 200
  reforgePlotsLayout -> addRow( "Reforge Amount", plotAmountChoice );

  plotStepChoice = createChoice( 5, "10", "20", "30", "40", "50" );
  plotStepChoice -> setCurrentIndex( 1 ); // Default is 20
  reforgePlotsLayout -> addRow( "Step Amount", plotStepChoice );

  QLabel* messageText = new QLabel( "A maximum of three stats may be ran at once." );
  reforgePlotsLayout -> addRow( messageText );

  reforgeplotsButtonGroup = new ReforgeButtonGroup();
  reforgeplotsButtonGroup -> setExclusive( false );
  OptionEntry* reforgeplots = getReforgePlotOptions();
  for( int i=0; reforgeplots[ i ].label; i++ )
  {
    QCheckBox* checkBox = new QCheckBox( reforgeplots[ i ].label );
    checkBox -> setToolTip( reforgeplots[ i ].tooltip );
    reforgeplotsButtonGroup -> addButton( checkBox );
    reforgePlotsLayout -> addWidget( checkBox );
    QObject::connect( checkBox, SIGNAL( stateChanged( int ) ),
                      reforgeplotsButtonGroup, SLOT( setSelected( int ) ) );
  }

  QGroupBox* reforgeplotsGroupBox = new QGroupBox();
  reforgeplotsGroupBox -> setLayout( reforgePlotsLayout );

  optionsTab->addTab( reforgeplotsGroupBox, "Reforge Plots" );
}

void SimulationCraftWindow::createImportTab()
{
  importTab = new QTabWidget();
  mainTab->addTab( importTab, "Import" );

  battleNetView = new SimulationCraftWebView( this );
  battleNetView->setUrl( QUrl( "http://us.battle.net/wow/en" ) );
  importTab->addTab( battleNetView, "Battle.Net" );

  charDevCookies = new PersistentCookieJar( "chardev.cookies" );
  charDevCookies->load();
  charDevView = new SimulationCraftWebView( this );
  charDevView->page()->networkAccessManager()->setCookieJar( charDevCookies );
  charDevView->setUrl( QUrl( "http://chardev.org/?planner" ) );
  importTab->addTab( charDevView, "CharDev" );

  createRawrTab();
  createBestInSlotTab();

  historyList = new QListWidget();
  historyList->setSortingEnabled( true );
  importTab->addTab( historyList, "History" );

  connect( rawrButton,  SIGNAL( clicked( bool ) ),                       this, SLOT( rawrButtonClicked() ) );
  connect( historyList, SIGNAL( itemDoubleClicked( QListWidgetItem* ) ), this, SLOT( historyDoubleClicked( QListWidgetItem* ) ) );
  connect( importTab,   SIGNAL( currentChanged( int ) ),                 this, SLOT( importTabChanged( int ) ) );

  // Commenting out until it is more fleshed out.
  // createCustomTab();
}

void SimulationCraftWindow::createRawrTab()
{
  QVBoxLayout* rawrLayout = new QVBoxLayout();
  QLabel* rawrLabel = new QLabel( " http://rawr.codeplex.com\n\n"
                                  "Rawr is an exceptional theorycrafting tool that excels at gear optimization."
                                  " The key architectural difference between Rawr and SimulationCraft is one of"
                                  " formulation vs simulation.  There are strengths and weaknesses to each"
                                  " approach.  Since they come from different directions, one can be confident"
                                  " in the result when they arrive at the same destination.\n\n"
                                  " To aid comparison, SimulationCraft can import the character xml file written by Rawr.\n\n"
                                  " Alternatively, paste xml from the Rawr in-game addon into the space below." );
  rawrLabel->setWordWrap( true );
  rawrLayout->addWidget( rawrLabel );
  rawrLayout->addWidget( rawrButton = new QPushButton( "Load Rawr XML" ) );
  rawrLayout->addWidget( rawrText = new SimulationCraftTextEdit(), 1 );
  QGroupBox* rawrGroupBox = new QGroupBox();
  rawrGroupBox->setLayout( rawrLayout );
  importTab->addTab( rawrGroupBox, "Rawr" );
}

void SimulationCraftWindow::createBestInSlotTab()
{
  QStringList headerLabels( "Player Class" ); headerLabels += QString( "Profile" );

  bisTree = new QTreeWidget();
  bisTree->setColumnCount( 1 );
  bisTree->setHeaderLabels( headerLabels );
  importTab->addTab( bisTree, "BiS" );

  const int TIER_MAX=2;
  const char* tierNames[] = { "T12", "T13" };
  QTreeWidgetItem* rootItems[ PLAYER_MAX ][ TIER_MAX ];
  for( int i=DEATH_KNIGHT; i <= WARRIOR; i++ )
  {
    // Ignore the Monk for now
    if ( i == MONK ) continue;

    QTreeWidgetItem* top = new QTreeWidgetItem( QStringList( util_t::player_type_string( i ) ) );
    bisTree->addTopLevelItem( top );
    for( int j=0; j < TIER_MAX; j++ )
    {
      top->addChild( rootItems[ i ][ j ] = new QTreeWidgetItem( QStringList( tierNames[ j ] ) ) );
    }
  }
#ifndef Q_WS_MAC
  QDir dir = QString( "profiles" );
#else
  CFURLRef fileRef    = CFBundleCopyResourceURL( CFBundleGetMainBundle(), CFSTR( "profiles" ), 0, 0 );
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
  bisProfilePath = dir.absolutePath() + "/";
  bisProfilePath = QDir::toNativeSeparators( bisProfilePath );

  QStringList profileList = dir.entryList();
  int numProfiles = profileList.count();
  for( int i=0; i < numProfiles; i++ )
  {
    QString& profile = profileList[ i ];

    int player = PLAYER_MAX;
    for( int j=0; j < PLAYER_MAX && player == PLAYER_MAX; j++ )
      if( profile.contains( util_t::player_type_string( j ), Qt::CaseInsensitive ) )
        player = j;

    // Hack! For now...  Need to decide sim-wide just how the heck we want to refer to DKs.
    if ( profile.contains( "Death_Knight" ) )
      player = DEATH_KNIGHT;

    int tier = TIER_MAX;
    for( int j=0; j < TIER_MAX && tier == TIER_MAX; j++ )
      if( profile.contains( tierNames[ j ] ) )
        tier = j;

    if( player != PLAYER_MAX && tier != TIER_MAX )
    {
      QTreeWidgetItem* item = new QTreeWidgetItem( QStringList( profile ) );
      rootItems[ player ][ tier ]->addChild( item );
    }
  }

  bisTree->setColumnWidth( 0, 300 );

  connect( bisTree, SIGNAL( itemDoubleClicked( QTreeWidgetItem*,int ) ), this, SLOT( bisDoubleClicked( QTreeWidgetItem*,int ) ) );
}

void SimulationCraftWindow::createCustomTab()
{
  //In Dev - Character Retrieval Boxes & Buttons
  //In Dev - Load & Save Profile Buttons
  //In Dev - Profiler Slots, Talent & Glyph Layout
  QHBoxLayout* customLayout = new QHBoxLayout();
  QGroupBox* customGroupBox = new QGroupBox();
  customGroupBox->setLayout( customLayout );
  importTab->addTab( customGroupBox, "Custom Profile" );
  customLayout->addWidget( createCustomCharData = new QGroupBox( tr( "Character Data" ) ), 1 );
  createCustomCharData->setObjectName( QString::fromUtf8( "createCustomCharData" ) );
  customLayout->addWidget( createCustomProfileDock = new QTabWidget(), 1 );
  createCustomProfileDock->setObjectName( QString::fromUtf8( "createCustomProfileDock" ) );
  createCustomProfileDock->setAcceptDrops( true );
  customGearTab = new QWidget();
  customGearTab->setObjectName( QString::fromUtf8( "customGearTab" ) );
  createCustomProfileDock->addTab( customGearTab, QString() );
  customTalentsTab = new QWidget();
  customTalentsTab->setObjectName( QString::fromUtf8( "customTalentsTab" ) );
  createCustomProfileDock->addTab( customTalentsTab, QString() );
  customGlyphsTab = new QWidget();
  customGlyphsTab->setObjectName( QString::fromUtf8( "customGlyphsTab" ) );
  createCustomProfileDock->addTab( customGlyphsTab, QString() );
  createCustomProfileDock->setTabText( createCustomProfileDock->indexOf( customGearTab ), QApplication::translate( "createCustomTab", "Gear", 0, QApplication::UnicodeUTF8 ) );
  createCustomProfileDock->setTabToolTip( createCustomProfileDock->indexOf( customGearTab ), QApplication::translate( "createCustomTab", "Customise Gear Setup", 0, QApplication::UnicodeUTF8 ) );
  createCustomProfileDock->setTabText( createCustomProfileDock->indexOf( customTalentsTab ), QApplication::translate( "createCustomTab", "Talents", 0, QApplication::UnicodeUTF8 ) );
  createCustomProfileDock->setTabToolTip( createCustomProfileDock->indexOf( customTalentsTab ), QApplication::translate( "createCustomTab", "Customise Talents", 0, QApplication::UnicodeUTF8 ) );
  createCustomProfileDock->setTabText( createCustomProfileDock->indexOf( customGlyphsTab ), QApplication::translate( "createCustomTab", "Glyphs", 0, QApplication::UnicodeUTF8 ) );
  createCustomProfileDock->setTabToolTip( createCustomProfileDock->indexOf( customGlyphsTab ), QApplication::translate( "createCustomTab", "Customise Glyphs", 0, QApplication::UnicodeUTF8 ) );
}

void SimulationCraftWindow::createSimulateTab()
{
  simulateText = new SimulationCraftTextEdit();
  simulateText->setLineWrapMode( QPlainTextEdit::NoWrap );
  simulateText->setPlainText( defaultSimulateText() );
  mainTab->addTab( simulateText, "Simulate" );
}

void SimulationCraftWindow::createOverridesTab()
{
  overridesText = new SimulationCraftTextEdit();
  overridesText->setLineWrapMode( QPlainTextEdit::NoWrap );
  //overridesText->document()->setDefaultFont( QFont( "fixed" ) );
  overridesText->setPlainText( "# User-specified persistent global and player parms will set here.\n" );
  mainTab->addTab( overridesText, "Overrides" );
}

void SimulationCraftWindow::createLogTab()
{
  logText = new QPlainTextEdit();
  logText->setLineWrapMode( QPlainTextEdit::NoWrap );
  //logText->document()->setDefaultFont( QFont( "fixed" ) );
  logText->setReadOnly( true );
  logText->setPlainText( "Look here for error messages and simple text-only reporting.\n" );
  mainTab->addTab( logText, "Log" );
}

void SimulationCraftWindow::createHelpTab()
{
  helpView = new SimulationCraftWebView( this );
  helpView->setUrl( QUrl( "http://code.google.com/p/simulationcraft/wiki/StartersGuide" ) );
  mainTab->addTab( helpView, "Help" );
}

void SimulationCraftWindow::createResultsTab()
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
  if( file.open( QIODevice::ReadOnly ) )
  {
    s = file.readAll();
    file.close();
  }

  QTextBrowser* legendBanner = new QTextBrowser();
  legendBanner->setHtml( s );
  legendBanner->moveCursor( QTextCursor::Start );

  resultsTab = new QTabWidget();
  resultsTab->setTabsClosable( true );
  resultsTab->addTab( legendBanner, "Legend" );
  connect( resultsTab, SIGNAL( currentChanged( int ) ),    this, SLOT( resultsTabChanged( int ) )      );
  connect( resultsTab, SIGNAL( tabCloseRequested( int ) ), this, SLOT( resultsTabCloseRequest( int ) ) );
  mainTab->addTab( resultsTab, "Results" );
}

void SimulationCraftWindow::createSiteTab()
{
  siteView = new SimulationCraftWebView( this );
  siteView->setUrl( QUrl( "http://code.google.com/p/simulationcraft/" ) );
  mainTab->addTab( siteView, "Site" );
}

void SimulationCraftWindow::createToolTips()
{
  versionChoice->setToolTip( "Live: Use mechanics on Live servers\n"
                             "PTR:  Use mechanics on PTR server"
                             "Both: Create Evil Twin with PTR mechanics" );

  iterationsChoice->setToolTip( "100:   Fast and Rough\n"
                                "1000:  Sufficient for DPS Analysis\n"
                                "10000: Recommended for Scale Factor Generation\n"
                                "25000: Use if 10,000 isn't enough for Scale Factors\n"
                                "50000: If you're patient" );

  fightLengthChoice->setToolTip( "For custom fight lengths use max_time=seconds." );

  fightVarianceChoice->setToolTip( "Varying the fight length over a given spectrum improves\n"
                                   "the analysis of trinkets and abilities with long cooldowns." );

  fightStyleChoice->setToolTip( "Patchwerk: Tank-n-Spank\n"
                                "HelterSkelter:\n"
                                "    Movement, Stuns, Interrupts,\n"
                                "    Target-Switching (every 2min)\n"
				"Ultraxion:\n"
                                "    Periodic Stuns, Raid Damage\n" );

  targetRaceChoice->setToolTip( "Race of the target and any adds." );

  targetLevelChoice->setToolTip( "Level of the target and any adds." );

  playerSkillChoice->setToolTip( "Elite:       No mistakes.  No cheating either.\n"
                                 "Fire-is-Hot: Frequent DoT-clipping and skipping high-priority abilities." );

  threadsChoice->setToolTip( "Match the number of CPUs for optimal performance.\n"
                             "Most modern desktops have at least two CPU cores." );

  armoryRegionChoice->setToolTip( "United States, Europe, Taiwan, China, Korea" );

  armorySpecChoice->setToolTip( "Controls which Talent/Glyph specification is used when importing profiles from the Armory." );

  defaultRoleChoice->setToolTip( "Specify the character role during import to ensure correct action priority list." );

  debugChoice->setToolTip( "When a log is generated, only one iteration is used.\n"
                           "Gory details are very gory.  No documentation will be forthcoming.\n"
                           "Due to the forced single iteration, no scale factor calculation." );

  latencyChoice->setToolTip( "World Lag is the equivalent of the 'world lag' shown in the WoW Client.\n"
                             "It is currently used to extend the cooldown duration of user executable abilities "
                             " that have a cooldown.\n"
                             "Each setting adds an amount of 'lag' with a default standard deviation of 10%:\n"
                             "    'Low'   : 100ms\n"
                             "    'Medium': 300ms\n"
                             "    'High'  : 500ms" );

  backButton->setToolTip( "Backwards" );
  forwardButton->setToolTip( "Forwards" );

  plotAmountChoice -> setToolTip( "The maximum amount to reforge per stat." );
  plotStepChoice -> setToolTip( "The stat difference between two points.\n"
                                "It's NOT the number of steps: a lower value will generate more points!" );
}

#ifdef SC_PAPERDOLL
void SimulationCraftWindow::createPaperdoll()
{
  QWidget* paperdollTab = new QWidget( this );
  QHBoxLayout* paperdollMainLayout = new QHBoxLayout();
  paperdollMainLayout -> setAlignment( Qt::AlignLeft | Qt::AlignTop );
  paperdollTab -> setLayout( paperdollMainLayout );

  PaperdollProfile* profile = new PaperdollProfile();
  Paperdoll* paperdoll = new Paperdoll( profile, paperdollTab );
  ItemSelectionWidget* items = new ItemSelectionWidget( profile, paperdollTab );

  paperdollMainLayout -> addWidget( items );
  paperdollMainLayout -> addWidget( paperdoll );


  mainTab -> addTab( paperdollTab, "Paperdoll" );
}
#endif

void SimulationCraftWindow::createItemDataSourceSelector( QFormLayout* layout )
{
  itemDbOrder = new QListWidget( this );
  itemDbOrder -> setDragDropMode( QAbstractItemView::InternalMove );
  itemDbOrder -> setResizeMode( QListView::Fixed );
  itemDbOrder -> setSelectionRectVisible( false );
  itemDbOrder -> setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );
  itemDbOrder -> setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
  OptionEntry* item_sources = getItemSourceOptions();

  for ( int i = 0; item_sources[ i ].label; i++ )
  {
    QListWidgetItem* item = new QListWidgetItem( item_sources[ i ].label );
    item -> setData( Qt::UserRole, QVariant( item_sources[ i ].option ) );
    item -> setToolTip( item_sources[ i ].tooltip );
    itemDbOrder -> addItem( item );
  }

  itemDbOrder -> setFixedHeight( ( itemDbOrder -> model() -> rowCount() + 1 ) * itemDbOrder -> sizeHintForRow( 0 ) );

  layout->addRow( "Item Source Order", itemDbOrder );
}

void SimulationCraftWindow::updateVisibleWebView( SimulationCraftWebView* wv )
{
  visibleWebView = wv;
  progressBar->setFormat( "%p%" );
  progressBar->setValue( visibleWebView->progress );
  cmdLine->setText( visibleWebView->url().toString() );
}

// ==========================================================================
// Sim Initialization
// ==========================================================================

sim_t* SimulationCraftWindow::initSim()
{
  if( ! sim )
  {
    sim = new sim_t();
    sim -> input_is_utf8 = true; // Presume GUI input is always UTF-8
    sim -> output_file = fopen( "simc_log.txt", "w" );
    sim -> report_progress = 0;
#if SC_USE_PTR
    sim -> parse_option( "ptr", ( ( versionChoice->currentIndex() == 1 ) ? "1" : "0" ) );
#endif
    sim -> parse_option( "debug", ( (   debugChoice->currentIndex() == 2 ) ? "1" : "0" ) );
  }
  return sim;
}

void SimulationCraftWindow::deleteSim()
{
  if( sim )
  {
    fclose( sim -> output_file );
    delete sim;
    sim = 0;
    QFile logFile( "simc_log.txt" );
    logFile.open( QIODevice::ReadOnly );
    logText->appendPlainText( logFile.readAll() );
    logText->moveCursor( QTextCursor::End );
    logFile.close();
  }
}

// ==========================================================================
// Import
// ==========================================================================

void ImportThread::importBattleNet()
{
  QString region, server, character;
  QUrl qurl = url;

  {
    QStringList parts = qurl.host().split( '.' );

    if( parts.size() )
    {
      if( parts[ parts.size() - 1 ].length() == 2 )
        region = parts[ parts.size() - 1 ];
      else
      {
        for( QStringList::size_type i = 0; i < parts.size(); ++i )
        {
          if( parts[ i ].length() == 2 )
          {
            region = parts[ i ];
            break;
          }
        }
      }
    }
  }

  {
    QStringList parts = qurl.path().split( '/' );
    for( QStringList::size_type i = 0, n = parts.size(); i + 2 < n; ++i )
    {
      if( parts[ i ] == "character" )
      {
        server = parts[ i + 1 ];
        character = parts[ i + 2 ];
        break;
      }
    }
  }

  if( false )
  {
    QStringList tokens = url.split( QRegExp( "[?&=:/.]" ), QString::SkipEmptyParts );
    int count = tokens.count();
    for( int i=0; i < count-1; i++ )
    {
      QString& t = tokens[ i ];
      if( t == "http" )
      {
        region = tokens[ ++i ];
      }
      else if( t == "r" ) // old armory
      {
        server = tokens[ ++i ];
      }
      else if( t == "n" ) // old armory
      {
        character = tokens[ ++i ];
      }
      else if( t == "character" && ( i<count-2 ) ) // new battle.net
      {
        server    = tokens[ ++i ];
        character = tokens[ ++i ];
      }
    }
  }

  if( region.isEmpty() || server.isEmpty() || character.isEmpty() )
  {
    fprintf( sim->output_file, "Unable to determine Server and Character information!\n" );
  }
  else
  {
    // Windows 7 64bit somehow cannot handle straight toStdString() conversion, so
    // do it in a silly way as a workaround for now.
    std::string talents = mainWindow->armorySpecChoice->currentText().toUtf8().constData(),
                cpp_s   = server.toUtf8().constData(),
                cpp_c   = character.toUtf8().constData(),
                cpp_r   = region.toUtf8().constData();
    player = bcp_api::download_player( sim, cpp_r, cpp_s, cpp_c, talents );

    if( false )
    {
      if( cpp_r == "cn" )
        player = armory_t::download_player( sim, cpp_r, cpp_s, cpp_c, talents );
      else
        player = battle_net_t::download_player( sim, cpp_r, cpp_s, cpp_c, talents );
    }
  }
}

void ImportThread::importCharDev()
{
  QStringList tokens = url.split( QRegExp( "[?&=:/.]" ), QString::SkipEmptyParts );
  int count = tokens.count();
  if( count > 0 )
  {
    // Win7/x86_64 workaround
    std::string c = tokens[ count-1 ].toUtf8().constData();
    player = chardev_t::download_player( sim, c );
  }
}

void ImportThread::importRawr()
{
  // Win7/x86_64 workaround
  std::string xml = mainWindow->rawrText->toPlainText().toUtf8().constData();
  player = rawr_t::load_player( sim, "rawr.xml", xml );
}

void ImportThread::run()
{
  cache::advance_era();
  switch( tab )
  {
  case TAB_BATTLE_NET: importBattleNet(); break;
  case TAB_CHAR_DEV:   importCharDev();   break;
  case TAB_RAWR:       importRawr();      break;
  default: assert( 0 );
  }

  if( player )
  {
    player -> role = util_t::parse_role_type( mainWindow->defaultRoleChoice->currentText().toUtf8().constData() );

    if ( sim->init() )
    {
      std::string buffer="";
      player->create_profile( buffer );
      profile = QString::fromUtf8( buffer.c_str() );
    }
    else player = 0;
  }
}

void SimulationCraftWindow::startImport( int tab, const QString& url )
{
  if( sim )
  {
    sim -> cancel();
    return;
  }
  simProgress = 0;
  mainButton->setText( "Cancel!" );
  importThread->start( initSim(), tab, url );
  simulateText->setPlainText( defaultSimulateText() );
  mainTab->setCurrentIndex( TAB_SIMULATE );
  timer->start( 500 );
}

void SimulationCraftWindow::importFinished()
{
  timer->stop();
  simPhase = "%p%";
  simProgress = 100;
  progressBar->setFormat( simPhase.c_str() );
  progressBar->setValue( simProgress );
  if ( importThread->player )
  {
    simulateText->setPlainText( importThread->profile );
    simulateTextHistory.add( importThread->profile );

    QString label = QString::fromUtf8( importThread->player->name_str.c_str() );
    while( label.size() < 20 ) label += ' ';
    label += QString::fromUtf8( importThread->player->origin_str.c_str() );

    bool found = false;
    for( int i=0; i < historyList->count() && ! found; i++ )
      if( historyList->item( i )->text() == label )
        found = true;

    if( ! found )
    {
      QListWidgetItem* item = new QListWidgetItem( label );
      //item->setFont( QFont( "fixed" ) );

      historyList->addItem( item );
      historyList->sortItems();
    }

    mainButton->setText( "Simulate!" );
    mainTab->setCurrentIndex( TAB_SIMULATE );
  }
  else
  {
    simulateText->setPlainText( QString( "# Unable to generate profile from: " ) + importThread->url );
    mainButton->setText( "Save!" );
    mainTab->setCurrentIndex( TAB_LOG );
  }
  deleteSim();
}

// ==========================================================================
// Simulate
// ==========================================================================

void SimulateThread::run()
{
  QFile file( "simc_gui.simc" );
  if( file.open( QIODevice::WriteOnly ) )
  {
    file.write( options.toAscii() );
    file.close();
  }

  sim -> html_file_str = "simc_report.html";

  QStringList stringList = options.split( '\n', QString::SkipEmptyParts );

  int argc = stringList.count() + 1;
  char** argv = new char*[ argc ];

  QList<QByteArray> lines;
  lines.append( "simc" );
  for( int i=1; i < argc; i++ )
  {
    lines.append( stringList[ i-1 ].toUtf8().constData() );
  }
  for( int i=0; i < argc; i++ ) argv[ i ] = lines[ i ].data();

  if( sim -> parse_options( argc, argv ) )
  {
    success = sim -> execute();

    if ( success )
    {
      sim -> scaling -> analyze();
      sim -> plot -> analyze();
      sim -> reforge_plot -> analyze();
      report_t::print_suite( sim );
    }
  }
}

void SimulationCraftWindow::startSim()
{
  if( sim )
  {
    sim -> cancel();
    return;
  }
  optionsHistory.add( encodeOptions() );
  optionsHistory.current_index = 0;
  if( simulateText->toPlainText() != defaultSimulateText() )
  {
    simulateTextHistory.add(  simulateText->toPlainText() );
  }
  overridesTextHistory.add( overridesText->toPlainText() );
  simulateCmdLineHistory.add( cmdLine->text() );
  simProgress = 0;
  mainButton->setText( "Cancel!" );
  simulateThread->start( initSim(), mergeOptions() );
  // simulateText->setPlainText( defaultSimulateText() );
  cmdLineText = "";
  cmdLine->setText( cmdLineText );
  timer->start( 100 );
}

QString SimulationCraftWindow::mergeOptions()
{
  QString options = "";
#if SC_USE_PTR
  options += "ptr="; options += ( ( versionChoice->currentIndex() == 1 ) ? "1" : "0" ); options += "\n";
#endif
  if ( itemDbOrder -> count() > 0 )
  {
    options += "item_db_source=";
    for ( int i = 0; i < itemDbOrder -> count(); i++ )
    {
      QListWidgetItem *it = itemDbOrder -> item( i );
      options += it -> data( Qt::UserRole ).toString();
      if ( i < itemDbOrder -> count() - 1 )
        options += "/";
    }
    options += "\n";
  }
  options += "iterations=" + iterationsChoice->currentText() + "\n";
  if( iterationsChoice->currentText() == "10000" )
  {
    options += "dps_plot_iterations=1000\n";
  }
  const char *world_lag[] = { "0.1", "0.3", "0.5" };
  options += "default_world_lag=";
  options += world_lag[ latencyChoice->currentIndex() ];
  options += "\n";
  options += "max_time=" + fightLengthChoice->currentText() + "\n";
  options += "vary_combat_length=";
  const char *variance[] = { "0.0", "0.1", "0.2" };
  options += variance[ fightVarianceChoice->currentIndex() ];
  options += "\n";
  options += "fight_style=" + fightStyleChoice->currentText() + "\n";

  static const char* const targetlevel[] = { "88", "87", "85" };
  options += "target_level=";
  options += targetlevel[ targetLevelChoice->currentIndex() ];
  options += "\n";

  options += "target_race=" + targetRaceChoice->currentText() + "\n";
  options += "default_skill=";
  const char *skill[] = { "1.0", "0.9", "0.75", "0.50" };
  options += skill[ playerSkillChoice->currentIndex() ];
  options += "\n";
  options += "threads=" + threadsChoice->currentText() + "\n";
  options += "optimal_raid=0\n";
  QList<QAbstractButton*> buttons = buffsButtonGroup->buttons();
  OptionEntry* buffs = getBuffOptions();
  for( int i=1; buffs[ i ].label; i++ )
  {
    options += buffs[ i ].option;
    options += "=";
    options += buttons.at( i )->isChecked() ? "1" : "0";
    options += "\n";
  }
  buttons = debuffsButtonGroup->buttons();
  OptionEntry* debuffs = getDebuffOptions();
  for( int i=1; debuffs[ i ].label; i++ )
  {
    options += debuffs[ i ].option;
    options += "=";
    options += buttons.at( i )->isChecked() ? "1" : "0";
    options += "\n";
  }
  buttons = scalingButtonGroup->buttons();
  OptionEntry* scaling = getScalingOptions();
  for( int i=2; scaling[ i ].label; i++ )
  {
    if( buttons.at( i )->isChecked() )
    {
      options += "calculate_scale_factors=1\n";
      break;
    }
  }
  if( buttons.at( 1 )->isChecked() ) options += "positive_scale_delta=1\n";
  if( buttons.at( 15 )->isChecked() || buttons.at( 17 )->isChecked() ) options += "weapon_speed_scale_factors=1\n";
  options += "scale_only=none";
  for( int i=2; scaling[ i ].label; i++ )
  {
    if( buttons.at( i )->isChecked() )
    {
      options += ",";
      options += scaling[ i ].option;
    }
  }
  options += "\n";
  options += "dps_plot_stat=none";
  buttons = plotsButtonGroup->buttons();
  OptionEntry* plots = getPlotOptions();
  for( int i=0; plots[ i ].label; i++ )
  {
    if( buttons.at( i )->isChecked() )
    {
      options += ",";
      options += plots[ i ].option;
    }
  }
  options += "\n";
  options += "reforge_plot_stat=none";
  buttons = reforgeplotsButtonGroup->buttons();
  OptionEntry* reforgeplots = getReforgePlotOptions();
  for( int i=0; reforgeplots[ i ].label; i++ )
  {
    if( buttons.at( i )->isChecked() )
    {
      options += ",";
      options += reforgeplots[ i ].option;
    }
  }
  options += "\n";
  options += "reforge_plot_output_file=reforge_plot.csv"; // This should be set in the gui if possible
  options += "\n";
  options += simulateText->toPlainText();
  options += "\n";
  options += overridesText->toPlainText();
  options += "\n";
  options += cmdLine->text();
  options += "\n";
#if SC_USE_PTR
  if( versionChoice->currentIndex() == 2 )
  {
    options += "ptr=1\n";
    options += "copy=EvilTwinPTR\n";
    options += "ptr=0\n";
  }
#endif
  if( debugChoice->currentIndex() != 0 )
  {
    options += "log=1\n";
    options += "scale_only=none\n";
    options += "dps_plot_stat=none\n";
  }
  return options;
}

void SimulationCraftWindow::simulateFinished()
{
  timer->stop();
  simPhase = "%p%";
  simProgress = 100;
  progressBar->setFormat( simPhase.c_str() );
  progressBar->setValue( simProgress );
  QFile file( sim -> html_file_str.c_str() );
  deleteSim();
  if( ! simulateThread->success )
  {
    logText->appendPlainText( "Simulation failed!\n" );
    logText->moveCursor( QTextCursor::End );
    mainTab->setCurrentIndex( TAB_LOG );
  }
  else if( file.open( QIODevice::ReadOnly ) )
  {
    // Html results will _ALWAYS_ be utf-8, regardless of the input encoding
    // so read them to the WebView through QTextStream
    QTextStream s( &file );
    s.setCodec( "UTF-8" );
    QString resultsName = QString( "Results %1" ).arg( ++simResults );
    SimulationCraftWebView* resultsView = new SimulationCraftWebView( this );
    resultsHtml.append( s.readAll() );
    resultsView->setHtml( resultsHtml.last() );
    resultsTab->addTab( resultsView, resultsName );
    resultsTab->setCurrentWidget( resultsView );
    resultsView->setFocus();
    mainTab->setCurrentIndex( debugChoice->currentIndex() ? TAB_LOG : TAB_RESULTS );
  }
  else
  {
    logText->appendPlainText( "Unable to open html report!\n" );
    mainTab->setCurrentIndex( TAB_LOG );
  }
}

// ==========================================================================
// Save Results
// ==========================================================================

void SimulationCraftWindow::saveLog()
{
  logCmdLineHistory.add( cmdLine->text() );

  QFile file( cmdLine->text() );

  if( file.open( QIODevice::WriteOnly ) )
  {
    file.write( logText->toPlainText().toAscii() );
    file.close();
  }

  logText->appendPlainText( QString( "Log saved to: %1\n" ).arg( cmdLine->text() ) );
}

void SimulationCraftWindow::saveResults()
{
  int index = resultsTab->currentIndex();
  if( index <= 0 ) return;

  if( visibleWebView->url().toString() != "about:blank" ) return;

  resultsCmdLineHistory.add( cmdLine->text() );

  QFile file( cmdLine->text() );

  if( file.open( QIODevice::WriteOnly ) )
  {
    file.write( resultsHtml[ index-1 ].toAscii() );
    file.close();
  }

  logText->appendPlainText( QString( "Results saved to: %1\n" ).arg( cmdLine->text() ) );
}

// ==========================================================================
// Window Events
// ==========================================================================

void SimulationCraftWindow::closeEvent( QCloseEvent* e )
{
  saveHistory();
  battleNetView->stop();
  QCoreApplication::quit();
  e->accept();
}

void SimulationCraftWindow::cmdLineTextEdited( const QString& s )
{
  switch( mainTab->currentIndex() )
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

void SimulationCraftWindow::cmdLineReturnPressed()
{
  if( mainTab->currentIndex() == TAB_IMPORT )
  {
    if( cmdLine->text().count( "battle.net" ) ||
        cmdLine->text().count( "wowarmory.com" ) )
    {
      battleNetView->setUrl( QUrl::fromUserInput( cmdLine->text() ) );
      importTab->setCurrentIndex( TAB_BATTLE_NET );
    }
    else if( cmdLine->text().count( "chardev.org" ) )
    {
      charDevView->setUrl( QUrl::fromUserInput( cmdLine->text() ) );
      importTab->setCurrentIndex( TAB_CHAR_DEV );
    }
    else
    {
      if( ! sim ) mainButtonClicked( true );
    }
  }
  else
  {
    if( ! sim ) mainButtonClicked( true );
  }
}

void SimulationCraftWindow::mainButtonClicked( bool /* checked */ )
{
  switch( mainTab->currentIndex() )
  {
  case TAB_WELCOME:   startSim(); break;
  case TAB_OPTIONS:   startSim(); break;
  case TAB_SIMULATE:  startSim(); break;
  case TAB_OVERRIDES: startSim(); break;
  case TAB_HELP:      startSim(); break;
  case TAB_SITE:      startSim(); break;
  case TAB_IMPORT:
    switch( importTab->currentIndex() )
    {
    case TAB_BATTLE_NET: startImport( TAB_BATTLE_NET, cmdLine->text() ); break;
    case TAB_CHAR_DEV:   startImport( TAB_CHAR_DEV,   cmdLine->text() ); break;
    case TAB_RAWR:       startImport( TAB_RAWR,       "Rawr XML"      ); break;
    }
    break;
  case TAB_LOG: saveLog(); break;
  case TAB_RESULTS: saveResults(); break;
  }
}

void SimulationCraftWindow::backButtonClicked( bool /* checked */ )
{
  if( visibleWebView )
  {
    if( mainTab->currentIndex() == TAB_RESULTS && ! visibleWebView->history()->canGoBack() )
    {
      visibleWebView->setHtml( resultsHtml[ resultsTab->indexOf( visibleWebView ) - 1 ] );

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
    switch( mainTab->currentIndex() )
    {
    case TAB_WELCOME:   break;
    case TAB_OPTIONS:   decodeOptions( optionsHistory.backwards() ); break;
    case TAB_IMPORT:    break;
    case TAB_SIMULATE:   simulateText->setPlainText(  simulateTextHistory.backwards() );  simulateText->setFocus(); break;
    case TAB_OVERRIDES: overridesText->setPlainText( overridesTextHistory.backwards() ); overridesText->setFocus(); break;
    case TAB_LOG:       break;
    case TAB_RESULTS:   break;
    }
  }
}

void SimulationCraftWindow::forwardButtonClicked( bool /* checked */ )
{
  if( visibleWebView )
  {
    visibleWebView->forward();
    visibleWebView->setFocus();
  }
  else
  {
    switch( mainTab->currentIndex() )
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

void SimulationCraftWindow::rawrButtonClicked( bool /* checked */ )
{
  QFileDialog dialog( this );
  dialog.setFileMode( QFileDialog::ExistingFile );
  dialog.setNameFilter( "Rawr Profiles (*.xml)" );
  dialog.restoreState( rawrDialogState );
  if( dialog.exec() )
  {
    rawrDialogState = dialog.saveState();
    QStringList fileList = dialog.selectedFiles();
    if ( ! fileList.empty() )
    {
      QFile rawrFile( fileList.at( 0 ) );
      rawrFile.open( QIODevice::ReadOnly );
      rawrText->setPlainText( rawrFile.readAll() );
      rawrText->moveCursor( QTextCursor::Start );
    }
  }
}

void SimulationCraftWindow::mainTabChanged( int index )
{
  visibleWebView = 0;
  switch( index )
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
    mainButton->setText( "Save!" );
    resultsTabChanged( resultsTab->currentIndex() );
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
  if( visibleWebView )
  {
    progressBar->setFormat( "%p%" );
  }
  else
  {
    progressBar->setFormat( simPhase.c_str() );
    progressBar->setValue( simProgress );
  }
}

void SimulationCraftWindow::importTabChanged( int index )
{
  if( index == TAB_RAWR ||
      index == TAB_BIS  ||
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
    updateVisibleWebView( ( SimulationCraftWebView* ) importTab->widget( index ) );
  }
}

void SimulationCraftWindow::resultsTabChanged( int index )
{
  if( index <= 0 )
  {
    cmdLine->setText( "" );
  }
  else
  {
    updateVisibleWebView( ( SimulationCraftWebView* ) resultsTab->widget( index ) );
    QString s = visibleWebView->url().toString();
    if( s == "about:blank" ) s = resultsFileText;
    cmdLine->setText( s );
  }
}

void SimulationCraftWindow::resultsTabCloseRequest( int index )
{
  if( index <= 0 )
  {
    // Ignore attempts to close Legend
  }
  else
  {
    resultsTab->removeTab( index );
    resultsHtml.removeAt( index-1 );
  }
}

void SimulationCraftWindow::historyDoubleClicked( QListWidgetItem* item )
{
  QString text = item->text();
  QString url = text.section( ' ', 1, 1, QString::SectionSkipEmpty );

  if( url.count( "battle.net"    ) ||
      url.count( "wowarmory.com" ) )
  {
    battleNetView->setUrl( QUrl::fromEncoded( url.toAscii() ) );
    importTab->setCurrentIndex( TAB_BATTLE_NET );
  }
  else if( url.count( "chardev.org" ) )
  {
    charDevView->setUrl( QUrl::fromEncoded( url.toAscii() ) );
    importTab->setCurrentIndex( TAB_CHAR_DEV );
  }
  else
  {
    importTab->setCurrentIndex( TAB_RAWR );
  }
}

void SimulationCraftWindow::bisDoubleClicked( QTreeWidgetItem* item, int /* col */ )
{
  QString profile = item->text( 0 );

  QString s = "Unable to import profile "; s += profile;

  QFile file( bisProfilePath + profile );
  if( file.open( QIODevice::ReadOnly ) )
  {
    s = file.readAll();
    file.close();
  }

  simulateText->setPlainText( s );
  simulateTextHistory.add( s );
  mainTab->setCurrentIndex( TAB_SIMULATE );
  simulateText->setFocus();
}

void SimulationCraftWindow::allBuffsChanged( bool checked )
{
  QList<QAbstractButton*> buttons = buffsButtonGroup->buttons();
  int count = buttons.count();
  for( int i=1; i < count; i++ )
  {
    buttons.at( i ) -> setChecked( checked );
  }
}

void SimulationCraftWindow::allDebuffsChanged( bool checked )
{
  QList<QAbstractButton*> buttons = debuffsButtonGroup->buttons();
  int count = buttons.count();
  for( int i=1; i < count; i++ )
  {
    buttons.at( i ) -> setChecked( checked );
  }
}

void SimulationCraftWindow::allScalingChanged( bool checked )
{
  QList<QAbstractButton*> buttons = scalingButtonGroup->buttons();
  int count = buttons.count();
  for( int i=2; i < count; i++ )
  {
    buttons.at( i ) -> setChecked( checked );
  }
}

void SimulationCraftWindow::armoryRegionChanged( const QString& region )
{
  QString importUrl = "http://" + region + ".battle.net/wow/en";

  battleNetView->stop();
  battleNetView->setUrl( QUrl( importUrl ) );
}

void PersistentCookieJar::save()
{
  QFile file( fileName );
  if( file.open( QIODevice::WriteOnly ) )
  {
    QDataStream out( &file );
    QList<QNetworkCookie> cookies = allCookies();
    qint32 count = ( qint32 ) cookies.count();
    out << count;
    for( int i=0; i < count; i++ )
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
  if( file.open( QIODevice::ReadOnly ) )
  {
    QDataStream in( &file );
    QList<QNetworkCookie> cookies;
    qint32 count;
    in >> count;
    for( int i=0; i < count; i++ )
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
