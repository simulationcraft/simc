// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraftqt.h"
#include <QtWebKit>

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
      { "Toggle All Buffs",      "",                                "Toggle all buffs on/off"                                                    },
      { "Focus Magic",           "override.focus_magic",            "Focus Magic"                                                                },
      { "Heroic Presence",       "override.heroic_presence",        "Draenei Heroic Presence Racial"                                             },
      { "Agility and Strength",  "override.strength_of_earth",      "Horn of Winter\nStrength of Earth Totem"                                    },
      { "Attack Power",          "override.blessing_of_might",      "Battle Shout\nBlessing of Might"                                            },
      { "Attack Power (%)",      "override.trueshot_aura",          "Abomination's Might\nTrueshot Aura\nUnleashed Rage"                         },
      { "Bloodlust",             "override.bloodlust",              "Bloodlust\nHeroism"                                                         },
      { "All Damage",            "override.sanctified_retribution", "Arcane Empowerment\nFerocious Inspiration\nSanctified Retribution"          },
      { "All Haste",             "override.swift_retribution",      "Improved Moonkin Form\nSwift Retribution"                                   },
      { "Intellect",             "override.arcane_brilliance",      "Arcane Intellect"                                                           },
      { "Mana Regen",            "override.blessing_of_wisdom",     "Blessing of Wisdom\nMana Spring Totem"                                      },
      { "Melee Critical Strike", "override.leader_of_the_pack",     "Leader of the Pack\nRampage"                                                },
      { "Melee Haste",           "override.windfury_totem",         "Improved Icy Talons\nWindfury Totem"                                        },
      { "Replenishment",         "override.replenishment",          "Hunting Party\nImproved Soul Leech\nJudgements of the Wise\nVampiric Touch" },
      { "Spell Critical Strike", "override.moonkin_aura",           "Elemental Oath\nMoonkin Aura"                                               },
      { "Spell Haste",           "override.wrath_of_air",           "Wrath of Air Totem"                                                         },
      { "Spell Power",           "override.totem_of_wrath",         "Demonic Pact\nFlametongue Totem\nTotem of Wrath"                            },
      { "Spirit",                "override.divine_spirit",          "Divine Spirit\nFel Intelligence"                                            },
      { "Stamina",               "override.fortitude",              "Power Word: Fortitude"                                                      },
      { "Stat Add",              "override.mark_of_the_wild",       "Mark of the Wild"                                                           },
      { "Stat Multiplier",       "override.blessing_of_kings",      "Blessing of Kings"                                                          },
      { NULL, NULL, NULL }
    };
  return options;
}

static OptionEntry* getDebuffOptions()
{
  static OptionEntry options[] =
    {
      { "Toggle All Debuffs",     "",                               "Toggle all debuffs on/off"                                        },
      { "Armor (Major)",          "override.sunder_armor",          "Acid Spit\nExpose Armor\nSunder Armor"                            },
      { "Armor (Minor)",          "override.faerie_fire",           "Curse of Weakness\nFaerie Fire"                                   },
      { "Boss Attack Speed Slow", "override.thunder_clap",          "Icy Touch\nInfected Wounds\nJudgements of the Just\nThunder Clap" },
      { "Boss Hit Reduction",     "override.insect_swarm",          "Insect Swarm\nScorpid Sting"                                      },
      { "Bleed Damage",           "override.mangle",                "Mangle\nTrauma"                                                   },
      { "Bleeding",               "override.bleeding",              "Rip\nRupture\nPiercing Shots"                                     },
      { "Critical Strike",        "override.master_poisoner",       "Heart of the Crusader\nMaster Poisoner\nTotem of Wrath"           },
      { "Disease Damage",         "override.crypt_fever",           "Crypt Fever"                                                      },
      { "Mana Restore",           "override.judgement_of_wisdom",   "Judgement of Wisdom"                                              },
      { "Physical Damage",        "override.blood_frenzy",          "Blood Frenzy\nSavage Combat"                                      },
      { "Poisoned",               "override.poisoned",              "Deadly Poison\nSerpent Sting"                                     },
      { "Ranged Attack Power",    "override.hunters_mark",          "Hunter's Mark"                                                    },
      { "Spell Critical Strike",  "override.improved_scorch",       "Improved Scorch\nImproved Shadow Bolt\nWinters's Chill"           },
      { "Spell Damage",           "override.earth_and_moon",        "Curse of the Elements\nEarth and Moon\nEbon Plaguebriger"         },
      { "Spell Hit",              "override.misery",                "Improved Faerie Fire\nMisery"                                     },
      { NULL, NULL, NULL }
    };
  return options;
}

static OptionEntry* getScalingOptions()
{
  static OptionEntry options[] =
    {
      { "Analyze All Stats",                "",         "Scale factors are necessary for gear ranking.\nThey only require an additional simulation for each RELEVANT stat." },
      { "Use Positive Deltas Only",         "",         "Normally Hit/Expertise use negative scale factors to show DPS lost by reducing that stat.\n"
                                                        "This option forces a positive scale delta, which is useful for classes with soft caps." },
      { "Analyze Strength",                 "str",      "Calculate scale factors for Strength"                 },
      { "Analyze Agility",                  "agi",      "Calculate scale factors for Agility"                  },
      { "Analyze Stamina",                  "sta",      "Calculate scale factors for Stamina"                  },
      { "Analyze Intellect",                "int",      "Calculate scale factors for Intellect"                },
      { "Analyze Spirit",                   "spi",      "Calculate scale factors for Spirit"                   },
      { "Analyze Spell Power",              "sp",       "Calculate scale factors for Spell Power"              },
      { "Analyze Attack Power",             "ap",       "Calculate scale factors for Attack Power"             },
      { "Analyze Expertise Rating",         "exp",      "Calculate scale factors for Expertise Rating"         },
      { "Analyze Armor Penetration Rating", "arpen",    "Calculate scale factors for Armor Penetration Rating" },
      { "Analyze Hit Rating",               "hit",      "Calculate scale factors for Hit Rating"               },
      { "Analyze Crit Rating",              "crit",     "Calculate scale factors for Crit Rating"              },
      { "Analyze Haste Rating",             "haste",    "Calculate scale factors for Haste Rating"             },
      { "Analyze Weapon DPS",               "wdps",     "Calculate scale factors for Weapon DPS"               },
	  { "Analyze Weapon Speed",             "wspeed",   "Calculate scale factors for Weapon Speed"             },
      { "Analyze Off-hand Weapon DPS",      "wohdps",   "Calculate scale factors for Off-hand Weapon DPS"      },
	  { "Analyze Off-hand Weapon Speed",    "wohspeed", "Calculate scale factors for Off-hand Weapon Speed"    },
      { NULL, NULL, NULL }
    };
  return options;
}

static OptionEntry* getPlotOptions()
{
  static OptionEntry options[] =
    {
      { "Plot DPS per Strength",                 "str",   "Generate DPS curve for Strength"                 },
      { "Plot DPS per Agility",                  "agi",   "Generate DPS curve for Agility"                  },
      { "Plot DPS per Stamina",                  "sta",   "Generate DPS curve for Stamina"                  },
      { "Plot DPS per Intellect",                "int",   "Generate DPS curve for Intellect"                },
      { "Plot DPS per Spirit",                   "spi",   "Generate DPS curve for Spirit"                   },
      { "Plot DPS per Spell Power",              "sp",    "Generate DPS curve for Spell Power"              },
      { "Plot DPS per Attack Power",             "ap",    "Generate DPS curve for Attack Power"             },
      { "Plot DPS per Expertise Rating",         "exp",   "Generate DPS curve for Expertise Rating"         },
      { "Plot DPS per Armor Penetration Rating", "arpen", "Generate DPS curve for Armor Penetration Rating" },
      { "Plot DPS per Hit Rating",               "hit",   "Generate DPS curve for Hit Rating"               },
      { "Plot DPS per Crit Rating",              "crit",  "Generate DPS curve for Crit Rating"              },
      { "Plot DPS per Haste Rating",             "haste", "Generate DPS curve for Haste Rating"             },
      { "Plot DPS per Weapon DPS",               "wdps",  "Generate DPS curve for Weapon DPS"               },
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
		  "# Clicking Simulate will create a simc_gui.simc profile for review.\n");
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

void SimulationCraftWindow::decodeOptions( QString encoding )
{
  QStringList tokens = encoding.split( ' ' );
  if( tokens.count() >= 13 )
  {
           patchChoice->setCurrentIndex( tokens[  0 ].toInt() );
         latencyChoice->setCurrentIndex( tokens[  1 ].toInt() );
      iterationsChoice->setCurrentIndex( tokens[  2 ].toInt() );
     fightLengthChoice->setCurrentIndex( tokens[  3 ].toInt() );
   fightVarianceChoice->setCurrentIndex( tokens[  4 ].toInt() );
            addsChoice->setCurrentIndex( tokens[  5 ].toInt() );
      fightStyleChoice->setCurrentIndex( tokens[  6 ].toInt() );
      targetRaceChoice->setCurrentIndex( tokens[  7 ].toInt() );
     playerSkillChoice->setCurrentIndex( tokens[  8 ].toInt() );
         threadsChoice->setCurrentIndex( tokens[  9 ].toInt() );
       smoothRNGChoice->setCurrentIndex( tokens[ 10 ].toInt() );
    armoryRegionChoice->setCurrentIndex( tokens[ 11 ].toInt() );
      armorySpecChoice->setCurrentIndex( tokens[ 12 ].toInt() );
  }

  QList<QAbstractButton*>    buff_buttons =   buffsButtonGroup->buttons();
  QList<QAbstractButton*>  debuff_buttons = debuffsButtonGroup->buttons();
  QList<QAbstractButton*> scaling_buttons = scalingButtonGroup->buttons();
  QList<QAbstractButton*>    plot_buttons =   plotsButtonGroup->buttons();

  OptionEntry*   buffs = getBuffOptions();
  OptionEntry* debuffs = getDebuffOptions();
  OptionEntry* scaling = getScalingOptions();
  OptionEntry*   plots = getPlotOptions();

  for(int i = 13; i < tokens.count(); i++)
  {
     QStringList opt_tokens = tokens[ i ].split(':');

     OptionEntry* options=0;
     QList<QAbstractButton*>* buttons=0;

     if(      ! opt_tokens[ 0 ].compare( "buff"    ) ) { options = buffs;   buttons = &buff_buttons;    }
     else if( ! opt_tokens[ 0 ].compare( "debuff"  ) ) { options = debuffs; buttons = &debuff_buttons;  }
     else if( ! opt_tokens[ 0 ].compare( "scaling" ) ) { options = scaling; buttons = &scaling_buttons; }
     else if( ! opt_tokens[ 0 ].compare( "plots"   ) ) { options = plots;   buttons = &plot_buttons;    }

     if ( ! options ) continue;

     QStringList opt_value = opt_tokens[ 1 ].split('=');
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
    .arg(         patchChoice->currentIndex() )
    .arg(       latencyChoice->currentIndex() )
    .arg(    iterationsChoice->currentIndex() )
    .arg(   fightLengthChoice->currentIndex() )
    .arg( fightVarianceChoice->currentIndex() )
    .arg(          addsChoice->currentIndex() )
    .arg(    fightStyleChoice->currentIndex() )
    .arg(    targetRaceChoice->currentIndex() )
    .arg(   playerSkillChoice->currentIndex() )
    .arg(       threadsChoice->currentIndex() )
    .arg(     smoothRNGChoice->currentIndex() )
    .arg(  armoryRegionChoice->currentIndex() )
    .arg(    armorySpecChoice->currentIndex() )
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
  for( int i=1; scaling[ i ].label; i++ )
  {
    encoded += " scaling:";
    encoded += scaling[ i ].option;
    encoded += "=";
    encoded += buttons.at( i )->isChecked() ? "1" : "0";
  }
  
  buttons = plotsButtonGroup->buttons();
  OptionEntry* plots = getPlotOptions();
  for( int i=1; plots[ i ].label; i++ )
  {
    encoded += " plots:";
    encoded += plots[ i ].option;
    encoded += "=";
    encoded += buttons.at( i )->isChecked() ? "1" : "0";
  }
  
  return encoded;
}

void SimulationCraftWindow::updateSimProgress()
{
  if( sim )
  {
    simProgress = (int) ( 100.0 * sim->progress( simPhase ) );
  }
  else
  {
    simPhase = "%p%";
    simProgress = 100;
  }
  if( mainTab->currentIndex() != TAB_IMPORT &&
      mainTab->currentIndex() != TAB_RESULTS )
  {
    progressBar->setFormat( simPhase.c_str() );
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
      //item->setFont( QFont( "fixed" ) );
      historyList->addItem( item );
    }

    decodeOptions( optionsHistory.backwards() );

    QString s = overridesTextHistory.backwards();
    if( ! s.isEmpty() ) overridesText->setPlainText( s );
  }
}

void SimulationCraftWindow::saveHistory()
{
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

SimulationCraftWindow::SimulationCraftWindow(QWidget *parent)
  : QWidget(parent), visibleWebView(0), sim(0), simPhase( "%p%" ), simProgress(100), simResults(0)
{
  cmdLineText = "";
  logFileText = "log.txt";
  resultsFileText = "results.html";

  mainTab = new QTabWidget();
  createWelcomeTab();
  createOptionsTab();
  createImportTab();
  createSimulateTab();
  createOverridesTab();
  createExamplesTab();
  createLogTab();
  createResultsTab();
  createCmdLine();
  createToolTips();

  connect( mainTab, SIGNAL(currentChanged(int)), this, SLOT(mainTabChanged(int)) );
  
  QVBoxLayout* vLayout = new QVBoxLayout();
  vLayout->addWidget( mainTab );
  vLayout->addWidget( cmdLineGroupBox );
  setLayout( vLayout );
  
  timer = new QTimer( this );
  connect( timer, SIGNAL(timeout()), this, SLOT(updateSimProgress()) );

  importThread = new ImportThread( this );
  simulateThread = new SimulateThread( this );

  connect(   importThread, SIGNAL(finished()), this, SLOT(  importFinished()) );
  connect( simulateThread, SIGNAL(finished()), this, SLOT(simulateFinished()) );

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
  progressBar->setMaximumWidth( 150 );
  connect( backButton,    SIGNAL(clicked(bool)),   this, SLOT(    backButtonClicked()) );
  connect( forwardButton, SIGNAL(clicked(bool)),   this, SLOT( forwardButtonClicked()) );
  connect( mainButton,    SIGNAL(clicked(bool)),   this, SLOT(    mainButtonClicked()) );
  connect( cmdLine,       SIGNAL(returnPressed()),            this, SLOT( cmdLineReturnPressed()) );
  connect( cmdLine,       SIGNAL(textEdited(const QString&)), this, SLOT( cmdLineTextEdited(const QString&)) );
  cmdLineGroupBox = new QGroupBox();
  cmdLineGroupBox->setLayout( cmdLineLayout );
}

void SimulationCraftWindow::createWelcomeTab()
{
  QString s = "<div align=center><h1>Welcome to SimulationCraft!</h1>If you are seeing this text, then Welcome.html was unable to load.</div>";

  QFile file( "Welcome.html" );
  if( !file.exists() )
  {
    file.setFileName( "simcqt.app/Contents/Resources/Welcome.html" );
  }
  if( file.open( QIODevice::ReadOnly ) )
  {
    s = file.readAll();
    file.close();
  }

  QTextBrowser* welcomeBanner = new QTextBrowser();
  welcomeBanner->setHtml( s );
  welcomeBanner->moveCursor( QTextCursor::Start );
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

  QAbstractButton* allBuffs   =   buffsButtonGroup->buttons().at( 0 );
  QAbstractButton* allDebuffs = debuffsButtonGroup->buttons().at( 0 );
  QAbstractButton* allScaling = scalingButtonGroup->buttons().at( 0 );

  connect( armoryRegionChoice, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(armoryRegionChanged(const QString&)) );
  
  connect( allBuffs,   SIGNAL(toggled(bool)), this, SLOT(allBuffsChanged(bool))   );  
  connect( allDebuffs, SIGNAL(toggled(bool)), this, SLOT(allDebuffsChanged(bool)) );  
  connect( allScaling, SIGNAL(toggled(bool)), this, SLOT(allScalingChanged(bool)) );  
}

void SimulationCraftWindow::createGlobalsTab()
{
  QFormLayout* globalsLayout = new QFormLayout();
  globalsLayout->setFieldGrowthPolicy( QFormLayout::FieldsStayAtSizeHint );
  globalsLayout->addRow(         "Patch",         patchChoice = createChoice( 1, "3.3.3" ) );
  globalsLayout->addRow(       "Latency",       latencyChoice = createChoice( 2, "Low", "High" ) );
  globalsLayout->addRow(    "Iterations",    iterationsChoice = createChoice( 3, "100", "1000", "10000" ) );
  globalsLayout->addRow(  "Length (sec)",   fightLengthChoice = createChoice( 3, "100", "300", "500" ) );
  globalsLayout->addRow(   "Vary Length", fightVarianceChoice = createChoice( 3, "0%", "10%", "20%" ) );
  globalsLayout->addRow(          "Adds",          addsChoice = createChoice( 5, "0", "1", "2", "3", "9" ) );
  globalsLayout->addRow(   "Fight Style",    fightStyleChoice = createChoice( 2, "Patchwerk", "Helter Skelter" ) );
  globalsLayout->addRow(   "Target Race",    targetRaceChoice = createChoice( 7, "humanoid", "beast", "demon", "dragonkin", "elemental", "giant", "undead" ) );
  globalsLayout->addRow(  "Player Skill",   playerSkillChoice = createChoice( 4, "Elite", "Good", "Average", "Ouch! Fire is hot!" ) );
  globalsLayout->addRow(       "Threads",       threadsChoice = createChoice( 4, "1", "2", "4", "8" ) );
  globalsLayout->addRow(    "Smooth RNG",     smoothRNGChoice = createChoice( 2, "No", "Yes" ) );
  globalsLayout->addRow( "Armory Region",  armoryRegionChoice = createChoice( 4, "us", "eu", "tw", "cn" ) );
  globalsLayout->addRow(   "Armory Spec",    armorySpecChoice = createChoice( 2, "active", "inactive" ) );
  iterationsChoice->setCurrentIndex( 1 );
  fightLengthChoice->setCurrentIndex( 1 );
  QGroupBox* globalsGroupBox = new QGroupBox();
  globalsGroupBox->setLayout( globalsLayout );

  optionsTab->addTab( globalsGroupBox, "Globals" );
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
  buffsLayout->addStretch(1);
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
  debuffsLayout->addStretch(1);
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
  scalingLayout->addStretch(1);
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
  plotsLayout->addStretch(1);
  QGroupBox* plotsGroupBox = new QGroupBox();
  plotsGroupBox->setLayout( plotsLayout );

  optionsTab->addTab( plotsGroupBox, "Plots" );
}

void SimulationCraftWindow::createImportTab()
{
  importTab = new QTabWidget();
  mainTab->addTab( importTab, "Import" );

  armoryView = new SimulationCraftWebView( this );
  armoryView->setUrl( QUrl( "http://us.wowarmory.com" ) );
  importTab->addTab( armoryView, "Armory" );

  wowheadView = new SimulationCraftWebView( this );
  wowheadView->setUrl( QUrl( "http://www.wowhead.com/profiles" ) );
  importTab->addTab( wowheadView, "Wowhead" );
  
  createRawrTab();
  createBestInSlotTab();

  historyList = new QListWidget();
  historyList->setSortingEnabled( true );
  importTab->addTab( historyList, "History" );

  connect( rawrButton,  SIGNAL(clicked(bool)),                       this, SLOT(rawrButtonClicked()) );
  connect( rawrList,    SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(   rawrDoubleClicked(QListWidgetItem*)) );
  connect( historyList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(historyDoubleClicked(QListWidgetItem*)) );
  connect( importTab,   SIGNAL(currentChanged(int)),                 this, SLOT(importTabChanged(int)) );
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
				  " To aid comparison, SimulationCraft can import the character xml file written by Rawr." );
  rawrLabel->setWordWrap( true );
  rawrLayout->addWidget( rawrLabel );
  rawrLayout->addWidget( rawrButton = new QPushButton( "Change Directory" ) );
  rawrLayout->addWidget( rawrDir = new QLabel( "" ) );
  rawrLayout->addWidget( rawrList = new QListWidget(), 1 );
  QGroupBox* rawrGroupBox = new QGroupBox();
  rawrGroupBox->setLayout( rawrLayout );
  importTab->addTab( rawrGroupBox, "Rawr" );
}

void SimulationCraftWindow::createBestInSlotTab()
{
  QStringList headerLabels( "Player Class" ); headerLabels += QString( "URL" );

  bisTree = new QTreeWidget();
  bisTree->setColumnCount( 2 );
  bisTree->setHeaderLabels( headerLabels );
  importTab->addTab( bisTree, "BiS" );

  const int T9=0, T10=1, TMAX=2;
  const char* TNAMES[] = { "T9", "T10" };
  QTreeWidgetItem* rootItems[ PLAYER_MAX ][ TMAX ];
  for( int i=DEATH_KNIGHT; i <= WARRIOR; i++ )
  {
    QTreeWidgetItem* top = new QTreeWidgetItem( QStringList( util_t::player_type_string( i ) ) );
    //top->setFont( 0, QFont( "fixed" ) );
    bisTree->addTopLevelItem( top );
    for( int j=0; j < TMAX; j++ )
    {
      top->addChild( rootItems[ i ][ j ] = new QTreeWidgetItem( QStringList( TNAMES[ j ] ) ) );
      //rootItems[ i ][ j ]->setFont( 0, QFont( "fixed" ) );
    }
  }

  struct bis_entry_t { int type; int tier; const char* name; int site; const char* id; };

  bis_entry_t bisList[] = 
    {
      { DEATH_KNIGHT,  T9, "Death_Knight_T9_51_00_20",      TAB_WOWHEAD, "18101892" },
      { DEATH_KNIGHT,  T9, "Death_Knight_T9_00_53_18",      TAB_WOWHEAD, "18133139" },
      { DEATH_KNIGHT,  T9, "Death_Knight_T9_03_51_17",      TAB_WOWHEAD, "18133075" },
      { DEATH_KNIGHT,  T9, "Death_Knight_T9_00_17_54",      TAB_WOWHEAD, "20992958" },
      { DEATH_KNIGHT, T10, "Death_Knight_T10_51_00_20",     TAB_WOWHEAD, "21203256" },
      { DEATH_KNIGHT, T10, "Death_Knight_T10_51_00_20_GoD", TAB_WOWHEAD, "21203314" },
      { DEATH_KNIGHT, T10, "Death_Knight_T10_00_53_18",     TAB_WOWHEAD, "21203260" },
      { DEATH_KNIGHT, T10, "Death_Knight_T10_14_00_57",     TAB_WOWHEAD, "21203277" },
      { DEATH_KNIGHT, T10, "Death_Knight_T10_00_17_54",     TAB_WOWHEAD, "21203280" },

      { DRUID,      T9, "Druid_T9_00_55_16",                TAB_WOWHEAD, "14717965" },
      { DRUID,      T9, "Druid_T9_00_55_16_M",              TAB_WOWHEAD, "14726081" },
      { DRUID,      T9, "Druid_T9_58_00_13",                TAB_WOWHEAD, "14187592" },
      { DRUID,     T10, "Druid_T10_00_55_16",               TAB_WOWHEAD, "21284121" },
      { DRUID,     T10, "Druid_T10_00_55_16_M",             TAB_WOWHEAD, "21284118" },
      { DRUID,     T10, "Druid_T10_58_00_13",               TAB_WOWHEAD, "21284122" },

      { HUNTER,     T9, "Hunter_T9_02_18_51",               TAB_WOWHEAD, "14649679" },
      { HUNTER,     T9, "Hunter_T9_01_18_52_HP",            TAB_WOWHEAD, "21156704" },
      { HUNTER,     T9, "Hunter_T9_07_57_07",               TAB_WOWHEAD, "14189960" },
      { HUNTER,     T9, "Hunter_T9_07_57_07_HM",            TAB_WOWHEAD, "14651857" },
      { HUNTER,     T9, "Hunter_T9_53_14_04",               TAB_WOWHEAD, "14663160" },
      { HUNTER,    T10, "Hunter_T10_53_14_04",              TAB_WOWHEAD, "21284132" },
      { HUNTER,    T10, "Hunter_T10_07_57_07",              TAB_WOWHEAD, "21284133" },
      { HUNTER,    T10, "Hunter_T10_07_57_07_HM",           TAB_WOWHEAD, "21284135" },
      { HUNTER,    T10, "Hunter_T10_06_14_51",              TAB_WOWHEAD, "21284137" },
      { HUNTER,    T10, "Hunter_T10_03_15_53_HP",           TAB_WOWHEAD, "21284138" },

      { MAGE,       T9, "Mage_T9_00_53_18",                 TAB_WOWHEAD, "14306487" },
      { MAGE,       T9, "Mage_T9_18_00_53",                 TAB_WOWHEAD, "14306485" },
      { MAGE,       T9, "Mage_T9_20_51_00",                 TAB_WOWHEAD, "14306481" },
      { MAGE,       T9, "Mage_T9_57_03_11",                 TAB_WOWHEAD, "14306468" },
      { MAGE,      T10, "Mage_T10_00_53_18",                TAB_WOWHEAD, "21284296" },
      { MAGE,      T10, "Mage_T10_18_00_53",                TAB_WOWHEAD, "21284292" },
      { MAGE,      T10, "Mage_T10_20_51_00",                TAB_WOWHEAD, "21284285" },
      { MAGE,      T10, "Mage_T10_57_03_11",                TAB_WOWHEAD, "21284284" },

      { PALADIN,    T9, "Paladin_T9_05_11_55",              TAB_WOWHEAD, "16421920" },
      { PALADIN,   T10, "Paladin_T10_05_11_55",             TAB_WOWHEAD, "16266770" },
      { PALADIN,   T10, "Paladin_T10_09_05_57_H",           TAB_WOWHEAD, "20977380" },
      { PALADIN,   T10, "Paladin_T10_09_05_57_N",           TAB_WOWHEAD, "20966865" },
      { PALADIN,   T10, "Paladin_T10_09_05_57_HA",          TAB_WOWHEAD, "20980773" },
      { PALADIN,   T10, "Paladin_T10_09_05_57_HAA",         TAB_WOWHEAD, "20983668" },

      { PRIEST,     T9, "Priest_T9_13_00_58_232",           TAB_WOWHEAD, "13043846" },
      { PRIEST,     T9, "Priest_T9_13_00_58_245",           TAB_WOWHEAD, "13043847" },
      { PRIEST,     T9, "Priest_T9_13_00_58_258",           TAB_WOWHEAD, "13043848" },
      { PRIEST,     T9, "Priest_T9_13_00_58_258",           TAB_WOWHEAD, "13043848" },
      { PRIEST,    T10, "Priest_T10_13_00_58_264",          TAB_WOWHEAD, "18052647" },
      { PRIEST,    T10, "Priest_T10_13_00_58_277",          TAB_WOWHEAD, "20704178" },

      { ROGUE,      T9, "Rogue_T9_08_20_43",                TAB_WOWHEAD, "14562456" },
      { ROGUE,      T9, "Rogue_T9_20_51_00",                TAB_WOWHEAD, "20631652" },
      { ROGUE,      T9, "Rogue_T9_51_18_02",                TAB_WOWHEAD, "18612609" },
      { ROGUE,      T9, "Rogue_T9_51_18_02_R",              TAB_WOWHEAD, "21162157" },
      { ROGUE,     T10, "Rogue_T10_20_51_00",               TAB_WOWHEAD, "21162773" },
      { ROGUE,     T10, "Rogue_T10_51_18_02",               TAB_WOWHEAD, "21162321" },
      { ROGUE,     T10, "Rogue_T10_51_18_02_R",             TAB_WOWHEAD, "21162335" },

      { SHAMAN,     T9, "Shaman_T9_16_55_00_258",           TAB_WOWHEAD, "14635667" },
      { SHAMAN,     T9, "Shaman_T9_20_51_00_258",           TAB_WOWHEAD, "14635698" },
      { SHAMAN,     T9, "Shaman_T9_57_14_00_245",           TAB_WOWHEAD, "14636463" },
      { SHAMAN,     T9, "Shaman_T9_57_14_00_258",           TAB_WOWHEAD, "14636542" },
      { SHAMAN,     T9, "Shaman_T9_57_14_00_258_ToW",       TAB_WOWHEAD, "14731192" },
      { SHAMAN,    T10, "Shaman_T10_19_52_00",              TAB_WOWHEAD, "20837779" },
      { SHAMAN,    T10, "Shaman_T10_57_14_00",              TAB_WOWHEAD, "20837754" },
      { SHAMAN,    T10, "Shaman_T10_57_14_00_ToW",          TAB_WOWHEAD, "20837752" },

      { WARLOCK,    T9, "Warlock_T9_00_13_58",              TAB_WOWHEAD, "13808359" },
      { WARLOCK,    T9, "Warlock_T9_00_18_53",              TAB_WOWHEAD, "18859553" },
      { WARLOCK,    T9, "Warlock_T9_55_00_16",              TAB_WOWHEAD, "15214160" },
      { WARLOCK,    T9, "Warlock_T9_56_00_15",              TAB_WOWHEAD, "20850306" },
      { WARLOCK,    T9, "Warlock_T9_00_56_15",              TAB_WOWHEAD, "18859247" },
      { WARLOCK,   T10, "Warlock_T10_00_13_58",             TAB_WOWHEAD, "20850049" },
      { WARLOCK,   T10, "Warlock_T10_00_18_53",             TAB_WOWHEAD, "20983226" },
      { WARLOCK,   T10, "Warlock_T10_55_00_16",             TAB_WOWHEAD, "20850045" },
      { WARLOCK,   T10, "Warlock_T10_56_00_15",             TAB_WOWHEAD, "20983156" },
      { WARLOCK,   T10, "Warlock_T10_00_56_15",             TAB_WOWHEAD, "20983242" },

      { WARRIOR,    T9, "Warrior_T9_19_52_00",              TAB_WOWHEAD, "13110966" },
      { WARRIOR,    T9, "Warrior_T9_54_17_00_A",            TAB_WOWHEAD, "13111680" },
      { WARRIOR,    T9, "Warrior_T9_54_17_00_M",            TAB_WOWHEAD, "13112110" },
      { WARRIOR,    T9, "Warrior_T9_54_17_00_S",            TAB_WOWHEAD, "13112036" },
      { WARRIOR,   T10, "Warrior_T10_19_52_00_277",         TAB_WOWHEAD, "21169162" },
      { WARRIOR,   T10, "Warrior_T10_55_08_08_277",         TAB_WOWHEAD, "21169165" },
      { WARRIOR,   T10, "Warrior_T10_19_52_00_264",         TAB_WOWHEAD, "21169160" },

      { PLAYER_NONE, TMAX, NULL, -1, NULL }
    };

  for( int i=0; bisList[ i ].type != PLAYER_NONE; i++ )
  {
    bis_entry_t& b = bisList[ i ];
    QString url = "";
    switch( b.site )
    {
    case TAB_WOWHEAD: url += "http://www.wowhead.com/profile="; break;
    default: assert(0);
    }
    url += b.id;
    QStringList labels = QStringList( b.name ); labels += url;
    QTreeWidgetItem* item = new QTreeWidgetItem( labels );
    //item->setFont( 0, QFont( "fixed" ) );
    //item->setFont( 1, QFont( "fixed" ) );
    rootItems[ b.type ][ b.tier ]->addChild( item );
  }

  bisTree->setColumnWidth( 0, 300 );

  connect( bisTree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(bisDoubleClicked(QTreeWidgetItem*,int)) );
}

void SimulationCraftWindow::createSimulateTab()
{
  simulateText = new SimulationCraftTextEdit();
  simulateText->setLineWrapMode( QPlainTextEdit::NoWrap );
  //simulateText->document()->setDefaultFont( QFont( "fixed" ) );
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
  logText->setReadOnly(true);
  logText->setPlainText( "Look here for error messages and simple text-only reporting.\n" );
  mainTab->addTab( logText, "Log" );
}

void SimulationCraftWindow::createExamplesTab()
{
  QString s = "# If you are seeing this text, then Examples.simc was unable to load.";

  QFile file( "Examples.simc" );
  if( !file.exists() )
  {
    file.setFileName( "simcqt.app/Contents/Resources/Examples.simc" );
  }
  if( file.open( QIODevice::ReadOnly ) )
  {
    s = file.readAll();
    file.close();
  }

  QTextBrowser* examplesText = new QTextBrowser();
  examplesText->setPlainText( s );
  mainTab->addTab( examplesText, "Examples" );
}
 
void SimulationCraftWindow::createResultsTab()
{
  QString s = "<div align=center><h1>Understanding SimulationCraft Output!</h1>If you are seeing this text, then Legend.html was unable to load.</div>";

  QFile file( "Legend.html" );
  if( !file.exists() )
  {
    file.setFileName( "simcqt.app/Contents/Resources/Legend.html" );
  }
  if( file.open( QIODevice::ReadOnly ) )
  {
    s = file.readAll();
    file.close();
  }

  QTextBrowser* legendBanner = new QTextBrowser();
  legendBanner->setHtml( s );
  legendBanner->moveCursor( QTextCursor::Start );

  resultsTab = new QTabWidget();
  resultsTab->addTab( legendBanner, "Legend" );
  connect( resultsTab, SIGNAL(currentChanged(int)), this, SLOT(resultsTabChanged(int)) );
  mainTab->addTab( resultsTab, "Results" );
}

void SimulationCraftWindow::createToolTips()
{
  patchChoice->setToolTip( "3.3.3: Live\n"
			   "unknown: PTR" );

  latencyChoice->setToolTip( "Low:  queue=0.075  gcd=0.150  channel=0.250\n"
			     "High: queue=0.150  gcd=0.300  channel=0.500" );

  iterationsChoice->setToolTip( "100:   Fast and Rough\n"
				"1000:  Sufficient for DPS Analysis\n"
				"10000: Recommended for Scale Factor Generation" );

  fightLengthChoice->setToolTip( "For custom fight lengths use max_time=seconds." );

  fightVarianceChoice->setToolTip( "Varying the fight length over a given spectrum improves\n"
				   "the analysis of trinkets and abilities with long cooldowns." );

  addsChoice->setToolTip( "Number of additional targets nearby boss for entire fight.\n"
			  "See Examples tab for how to use raid_events to summon temporary adds.\n"
			  "Support for multi-DoT has not yet been implemented.\n"
			  "Many AoE abilities have not yet been implemented." );

  fightStyleChoice->setToolTip( "Patchwerk: Tank-n-Spank\n"
				"Helter Skelter:\n"
				"    Movement, Stuns, Interrupts,\n"
				"    Target-Switching (every 2min)\n"
				"    Three Adds (for 20sec every 1min)\n"
				"    Distraction (10% -skill every other 45sec" );

  targetRaceChoice->setToolTip( "Race of the target and any adds." );

  playerSkillChoice->setToolTip( "Elite:       No mistakes.  No cheating either.\n"
				 "Fire-is-Hot: Frequent DoT-clipping and skipping high-priority abilities." );

  threadsChoice->setToolTip( "Match the number of CPUs for optimal performance.\n"
			     "Most modern desktops have two at least two CPU cores." );

  smoothRNGChoice->setToolTip( "Introduce some determinism into the RNG packages, improving convergence by 10x.\n"
			       "This enables the use of fewer iterations, but the scale factors of non-linear stats may suffer." );

  armoryRegionChoice->setToolTip( "United States, Europe, Taiwan, China" );

  armorySpecChoice->setToolTip( "Controls which Talent/Glyph specification is used when importing profiles from the Armory." );

  backButton->setToolTip( "Backwards" );
  forwardButton->setToolTip( "Forwards" );
}

// ==========================================================================
// Sim Initialization
// ==========================================================================

sim_t* SimulationCraftWindow::initSim()
{
  if( ! sim ) 
  {
    sim = new sim_t();
    sim -> output_file = fopen( "simc_log.txt", "w" );
    sim -> report_progress = 0;
    sim -> parse_option( "patch", patchChoice->currentText().toStdString() );
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
    logFile.close();
  }
}

// ==========================================================================
// Import 
// ==========================================================================

void ImportThread::importArmory()
{
  QString region, server, character;
  QStringList tokens = url.split( QRegExp( "[?&=:/.]" ), QString::SkipEmptyParts );
  int count = tokens.count();
  for( int i=0; i < count-1; i++ ) 
  {
    QString& t = tokens[ i ];
    if( t == "http" )
    {
      region = tokens[ i+1 ];
    }
    else if( t == "r" )
    {
      server = tokens[ i+1 ];
    }
    else if( t == "n" || t == "cn" )
    {
      character = tokens[ i+1 ];
    }
  }
  if( region.isEmpty() || server.isEmpty() || character.isEmpty() )
  {
    fprintf( sim->output_file, "Unable to determine Server and Character information!\n" );
  }
  else
  {
    std::string talents = mainWindow->armorySpecChoice  ->currentText().toStdString();
    player = armory_t::download_player( sim, region.toStdString(), server.toStdString(), character.toStdString(), talents );
  }
}

void ImportThread::importWowhead()
{
  QString id;
  QStringList tokens = url.split( QRegExp( "[/?&=]" ), QString::SkipEmptyParts );
  int count = tokens.count();
  for( int i=0; i < count-1; i++ ) 
  {
    if( tokens[ i ] == "profile" )
    {
      id = tokens[ i+1 ];
      break;
    }
  }
  if( id.isEmpty() )
  {
    fprintf( sim->output_file, "Import from Wowhead requires use of Profiler!\n" );
  }
  else
  {
    tokens = id.split( '.', QString::SkipEmptyParts );
    count = tokens.count();
    if( count == 1 )
    {
      player = wowhead_t::download_player( sim, id.toStdString() );
    }
    else if( count == 3 )
    {
      player = wowhead_t::download_player( sim, tokens[ 0 ].toStdString(), tokens[ 1 ].toStdString(), tokens[ 2 ].toStdString() ); 
    }
  }
}

void ImportThread::importRawr()
{
  player = rawr_t::load_player( sim, url.toStdString() );
}

void ImportThread::run()
{
  switch( tab )
  {
  case TAB_ARMORY:     importArmory();     break;
  case TAB_WOWHEAD:    importWowhead();    break;
  case TAB_RAWR:       importRawr();       break;
  default: assert(0);
  }

  if( player )
  {
    sim->init();
    std::string buffer="";
    player->create_profile( buffer );
    profile = buffer.c_str();
  }
}

void SimulationCraftWindow::startImport( int tab, const QString& url )
{
  if( sim )
  {
    sim -> cancel();
    return;
  }
  if( tab == TAB_RAWR ) 
  {
    rawrCmdLineHistory.add( url );
    rawrFileText = "";
    cmdLine->setText( rawrFileText );
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

    QString label = importThread->player->name_str.c_str();
    while( label.size() < 20 ) label += " ";
    label += importThread->player->origin_str.c_str();

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
  }
  else
  {
    simulateText->setPlainText( QString( "# Unable to generate profile from: " ) + importThread->url );
  }
  deleteSim();
  mainButton->setText( "Simulate!" );
  mainTab->setCurrentIndex( TAB_SIMULATE );
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
  for( int i=1; i < argc; i++ ) lines.append( stringList[ i-1 ].toAscii() );
  for( int i=0; i < argc; i++ ) argv[ i ] = lines[ i ].data();

  if( sim -> parse_options( argc, argv ) )
  {
    success = sim -> execute();

    if ( success )
    {
      sim -> scaling -> analyze();
      sim -> plot -> analyze();
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
  simulateText->setPlainText( defaultSimulateText() );
  cmdLineText = "";
  cmdLine->setText( cmdLineText );
  timer->start( 500 );
}

QString SimulationCraftWindow::mergeOptions()
{
  QString options = "";
  options += "patch=" + patchChoice->currentText() + "\n";
  if( latencyChoice->currentText() == "Low" )
  {
    options += "queue_lag=0.075  gcd_lag=0.150  channel_lag=0.250\n";
  }
  else
  {
    options += "queue_lag=0.150  gcd_lag=0.300  channel_lag=0.500\n";
  }
  options += "iterations=" + iterationsChoice->currentText() + "\n";
  if( iterationsChoice->currentText() == "10000" )
  {
    options += "dps_plot_iterations=1000\n";
  }
  options += "max_time=" + fightLengthChoice->currentText() + "\n";
  options += "vary_combat_length=";
  const char *variance[] = { "0.0", "0.1", "0.2" };
  options += variance[ fightVarianceChoice->currentIndex() ];
  options += "\n";
  options += "target_adds=" + addsChoice->currentText() + "\n";
  if( fightStyleChoice->currentText() == "Helter Skelter" )
  {
    options += "raid_events=casting,cooldown=30,duration=3,first=15\n";
    options += "raid_events+=/movement,cooldown=30,duration=5\n";
    options += "raid_events+=/stun,cooldown=60,duration=2\n";
    options += "raid_events+=/invulnerable,cooldown=120,duration=3\n";
    options += "raid_events+=/adds,count=3,cooldown=60,duration=20\n";
    options += "raid_events+=/distraction,skill=0.2,cooldown=90,duration=45\n";
  }
  options += "target_race=" + targetRaceChoice->currentText() + "\n";
  options += "default_skill=";
  const char *skill[] = { "1.0", "0.9", "0.75", "0.50" };
  options += skill[ playerSkillChoice->currentIndex() ];
  options += "\n";
  options += "threads=" + threadsChoice->currentText() + "\n";
  options += smoothRNGChoice->currentIndex() ? "smooth_rng=1\n" : "smooth_rng=0\n";
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
  options += "calculate_scale_factors=1\n";
  if( buttons.at( 1 )->isChecked() ) options += "positive_scale_delta=1\n";
  if( buttons.at( 15 )->isChecked() || buttons.at( 17 )->isChecked() ) options += "weapon_speed_scale_factors=1\n";
  options += "scale_only=none";
  OptionEntry* scaling = getScalingOptions();
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
  options += simulateText->toPlainText();
  options += "\n";
  options += overridesText->toPlainText();
  options += "\n";
  options += cmdLine->text();
  options += "\n";
  return options;
}

void SimulationCraftWindow::simulateFinished()
{
  timer->stop();
  simPhase = "%p%";
  simProgress = 100;
  progressBar->setFormat( simPhase.c_str() );
  progressBar->setValue( simProgress );
  QFile file( sim->html_file_str.c_str() );
  deleteSim();
  if( ! simulateThread->success )
  {
    logText->appendPlainText( "Simulation failed!\n" );
    logText->moveCursor( QTextCursor::End );
    mainTab->setCurrentIndex( TAB_LOG );
  }
  else if( file.open( QIODevice::ReadOnly ) )
  {
    QString resultsName = QString( "Results %1" ).arg( ++simResults );
    SimulationCraftWebView* resultsView = new SimulationCraftWebView( this );
    resultsHtml.append( file.readAll() );
    resultsView->setHtml( resultsHtml.last() );
    resultsTab->addTab( resultsView, resultsName );
    resultsTab->setCurrentWidget( resultsView );
    mainTab->setCurrentIndex( TAB_RESULTS );
    resultsView->setFocus();
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
  armoryView->stop();
  wowheadView->stop();
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
  case TAB_EXAMPLES:  cmdLineText = s; break;
  case TAB_LOG:       logFileText = s; break;
  case TAB_RESULTS:   resultsFileText = s; break;
  case TAB_IMPORT:    if( importTab->currentIndex() == TAB_RAWR ) rawrFileText = s; break;
  }
}

void SimulationCraftWindow::cmdLineReturnPressed()
{
  if( mainTab->currentIndex() == TAB_IMPORT )
  {
    if( cmdLine->text().count( "wowarmory" ) )
    {
      armoryView->setUrl( QUrl( cmdLine->text() ) ); 
      importTab->setCurrentIndex( TAB_ARMORY );
    }
    else if( cmdLine->text().count( "wowhead" ) )
    {
      wowheadView->setUrl( QUrl( cmdLine->text() ) );
      importTab->setCurrentIndex( TAB_WOWHEAD );
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

void SimulationCraftWindow::mainButtonClicked( bool checked )
{
  checked=true;
  switch( mainTab->currentIndex() )
  {
  case TAB_WELCOME:   startSim(); break;
  case TAB_OPTIONS:   startSim(); break;
  case TAB_SIMULATE:  startSim(); break;
  case TAB_OVERRIDES: startSim(); break;
  case TAB_EXAMPLES:  startSim(); break;
  case TAB_IMPORT:
    switch( importTab->currentIndex() )
    {
    case TAB_ARMORY:     startImport( TAB_ARMORY,     cmdLine->text() ); break;
    case TAB_WOWHEAD:    startImport( TAB_WOWHEAD,    cmdLine->text() ); break;
    case TAB_RAWR:       startImport( TAB_RAWR,       cmdLine->text() ); break;
    }
    break;
  case TAB_LOG: saveLog(); break;
  case TAB_RESULTS: saveResults(); break;
  }
}

void SimulationCraftWindow::backButtonClicked( bool checked )
{
  checked=true;
  if( visibleWebView ) 
  {
    if( mainTab->currentIndex() == TAB_RESULTS && ! visibleWebView->history()->canGoBack() )
    {
      visibleWebView->setHtml( resultsHtml[ resultsTab->indexOf( visibleWebView ) - 1 ] );

      QWebHistory* h = visibleWebView->history();
      visibleWebView->history()->clear(); // This is not appearing to work.
      h->setMaximumItemCount(0);
      h->setMaximumItemCount(100);
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
    case TAB_EXAMPLES:  break;
    }
  }
}

void SimulationCraftWindow::forwardButtonClicked( bool checked )
{
  checked=true;
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
    case TAB_EXAMPLES:  break;
    }
  }
}

void SimulationCraftWindow::rawrButtonClicked( bool checked )
{
  checked=true;
  QFileDialog dialog( this );
  dialog.setFileMode( QFileDialog::Directory );
  dialog.setNameFilter( "Rawr Profiles (*.xml)" );
  dialog.restoreState( rawrDialogState );
  if( dialog.exec() )
  {
    rawrDialogState = dialog.saveState();
    QDir dir = dialog.directory();
    dir.setSorting( QDir::Name );
    dir.setFilter( QDir::Files );
    dir.setNameFilters( QStringList( "*.xml" ) );
    QString pathName = dir.absolutePath() + "/";
    rawrDir->setText( QDir::toNativeSeparators( pathName ) );
    rawrList->clear();
    rawrList->addItems( dir.entryList() );
  }
}

void SimulationCraftWindow::mainTabChanged( int index )
{
  visibleWebView = 0;
  switch( index )
  {
  case TAB_WELCOME:   cmdLine->setText(     cmdLineText ); mainButton->setText( sim ? "Cancel!" : "Simulate!" ); break;
  case TAB_OPTIONS:   cmdLine->setText(     cmdLineText ); mainButton->setText( sim ? "Cancel!" : "Simulate!" ); break;
  case TAB_SIMULATE:  cmdLine->setText(     cmdLineText ); mainButton->setText( sim ? "Cancel!" : "Simulate!" ); break;
  case TAB_OVERRIDES: cmdLine->setText(     cmdLineText ); mainButton->setText( sim ? "Cancel!" : "Simulate!" ); break;
  case TAB_EXAMPLES:  cmdLine->setText(     cmdLineText ); mainButton->setText( sim ? "Cancel!" : "Simulate!" ); break;
  case TAB_LOG:       cmdLine->setText(     logFileText ); mainButton->setText( "Save!" ); break;
  case TAB_IMPORT:    
    mainButton->setText( sim ? "Cancel!" : "Import!" ); 
    importTabChanged( importTab->currentIndex() ); 
    break;
  case TAB_RESULTS:   
    mainButton->setText( "Save!" ); 
    resultsTabChanged( resultsTab->currentIndex() ); 
    break;
  default: assert(0);
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
      index == TAB_HISTORY )
  {
    visibleWebView = 0;
    progressBar->setFormat( simPhase.c_str() );
    progressBar->setValue( simProgress );
    if( index == TAB_RAWR )
    {
      cmdLine->setText( rawrFileText );
    }
    else
    {
      cmdLine->setText( "" );
    }
  }
  else
  {
    visibleWebView = (SimulationCraftWebView*) importTab->widget( index );
    progressBar->setFormat( "%p%" );
    progressBar->setValue( visibleWebView->progress );
    cmdLine->setText( visibleWebView->url().toString() );
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
    visibleWebView = (SimulationCraftWebView*) resultsTab->widget( index );
    progressBar->setValue( visibleWebView->progress );
    QString s = visibleWebView->url().toString();
    if( s == "about:blank" ) s = resultsFileText;
    cmdLine->setText( s );
  }
}

void SimulationCraftWindow::rawrDoubleClicked( QListWidgetItem* item )
{
  rawrFileText = rawrDir->text();
  rawrFileText += item->text();
  cmdLine->setText( rawrFileText );
}

void SimulationCraftWindow::historyDoubleClicked( QListWidgetItem* item )
{
  QString text = item->text();
  QString url = text.section( ' ', 1, 1, QString::SectionSkipEmpty );

  if( url.count( ".wowarmory." ) )
  {
    armoryView->setUrl( QUrl::fromEncoded( url.toAscii() ) );
    importTab->setCurrentIndex( TAB_ARMORY );
  }
  else if( url.count( ".wowhead." ) )
  {
    wowheadView->setUrl( QUrl::fromEncoded( url.toAscii() ) );
    importTab->setCurrentIndex( TAB_WOWHEAD );
  }
  else
  {
    importTab->setCurrentIndex( TAB_RAWR );
  }
}

void SimulationCraftWindow::bisDoubleClicked( QTreeWidgetItem* item, int col )
{
  col=0;

  if( item->columnCount() == 1 ) return;

  QString url = item->text( 1 );

  if( url.count( ".wowarmory." ) )
  {
    armoryView->setUrl( url );
    importTab->setCurrentIndex( TAB_ARMORY );
  }
  else if( url.count( ".wowhead." ) )
  {
    wowheadView->setUrl( url );
    importTab->setCurrentIndex( TAB_WOWHEAD );
  }
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

  QString s = "http://" + region + ".wowarmory.com";
  armoryView->setUrl( QUrl( s ) );
}
