// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
/*
  This file contains all of the methods required for automated comparison sims
*/
#include "simulationcraft.hpp"
#include "simulationcraftqt.hpp"

///////////////////////////////////////////////////////////////////////////////
////
////                        Logic Stuff
////
///////////////////////////////////////////////////////////////////////////////


  // stuff goes here!
QString automation::tokenize( QString qstr )
{
  std::string temp = qstr.toLocal8Bit().constData();
  util::tokenize( temp );
  
  return QString::fromStdString( temp );
}

QString automation::do_something( QString player_class,
                                  QString player_spec,
                                  QString player_race,
                                  QString player_level,
                                  QString player_talents,
                                  QString player_glyphs,
                                  QString player_gear,
                                  QString player_rotation
                                ) 
{
  // Dummy code - this just sets up a profile using the strings we pass. 
  // This is where we'll actually split off into different automation schemes.
  // For some reason, I was only able to pass "const std::string&" from SC_MainWindow::startAutomationImport (in sc_window.cpp), 
  // hence the silly temp crap. 

  QString profile;
  
  profile += tokenize( player_class ) + "=Name\n";
    
  profile += "specialization=" + tokenize( player_spec )+ "\n";

  profile += "race=" + tokenize( player_race ) + "\n";

  profile += "level=" + player_level + "\n";
  profile += "talents=" + player_talents + "\n";
  profile += "glyphs=" + player_glyphs + "\n";

  // gear
  profile += player_gear + "\n";

  // rotation
  profile += player_rotation;

  return profile;

}

///////////////////////////////////////////////////////////////////////////////
////
////                        GUI Stuff
////
///////////////////////////////////////////////////////////////////////////////


void SC_MainWindow::startAutomationImport( int tab )
{
  QString profile;

  profile = automation::do_something( importTab -> choice.player_class -> currentText(),
                                      importTab -> choice.player_spec -> currentText(),
                                      importTab -> choice.player_race -> currentText(),
                                      importTab -> choice.player_level -> currentText(),
                                      importTab -> textbox.talents -> document() -> toPlainText(),
                                      importTab -> textbox.glyphs -> document() -> toPlainText(),
                                      importTab -> textbox.gear -> document() -> toPlainText(),
                                      importTab -> textbox.rotation -> document() -> toPlainText()
                                    );

  simulateTab -> add_Text( profile,  tr( "Testing" ) );
  
  mainTab -> setCurrentTab( TAB_SIMULATE );
}


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

void SC_ImportTab::createAutomationTab()
{ 
  // layout building based on 
  // http://qt-project.org/doc/qt-4.8/layouts-basiclayouts.html

  QScrollArea* automationTabScrollArea = new QScrollArea;

  QVBoxLayout* mainLayout = new QVBoxLayout;
  
  // create a box for the "Form" section of the layout
  QGroupBox* formGroupBox = new QGroupBox(tr("Form layout"));
  mainLayout -> addWidget( formGroupBox );

  // define the formGroupBox's layout
  QFormLayout* formLayout = new QFormLayout();

  formLayout -> setFieldGrowthPolicy( QFormLayout::FieldsStayAtSizeHint );

  // Create Combo Boxes
  choice.player_class = createChoice( 11 , "Death Knight", "Druid", "Hunter", "Mage", "Monk", "Paladin", "Priest", "Rogue", "Shaman", "Warlock", "Warrior" );
  formLayout -> addRow( tr( "Class" ), choice.player_class );

  choice.player_spec = createChoice( 3, "1", "2", "3" );
  formLayout -> addRow( tr( "Spec (PH)" ), choice.player_spec );

  choice.player_race = createChoice( 13, "Blood Elf", "Draenei", "Dwarf", "Gnome", "Goblin", "Human", "Night Elf", "Orc", "Pandaren", "Tauren", "Troll", "Undead", "Worgen");
  formLayout -> addRow( tr( "Race" ), choice.player_race );

  choice.player_level = createChoice( 2, "100", "90" );
  formLayout -> addRow( tr( "Level" ), choice.player_level );

  QLabel* messageText = new QLabel( tr( "Sample Text\n" ) );
  formLayout -> addRow( messageText );

  // set the formGroupBox's layout now that we've defined it
  formGroupBox -> setLayout( formLayout );

  // Define a grid box for the text boxes
  QGroupBox* gridGroupBox = new QGroupBox(tr("Grid Layout"));
  QGridLayout* gridLayout = new QGridLayout;

  // Create a label and an edit box for talents
  QLabel* talentsLabel = new QLabel( tr( "Default Talents" ) );
  textbox.talents = new SC_TextEdit;
  textbox.talents -> resize( 200, 250 );
  textbox.talents -> setPlainText( "0000000" );
  
  // assign the label and edit box to cells
  gridLayout -> addWidget( talentsLabel, 0, 0, 0 );
  gridLayout -> addWidget( textbox.talents, 1, 0, 0 );

  // and again for glyphs
  QLabel* glyphsLabel = new QLabel( tr( "Default Glyphs" ) );
  textbox.glyphs = new SC_TextEdit;
  textbox.glyphs -> resize( 200, 250 );
  textbox.glyphs -> setPlainText( "focused_shield/alabaster_shield" );
  
  // assign the label and edit box to cells
  gridLayout -> addWidget( glyphsLabel,    2, 0, 0 );
  gridLayout -> addWidget( textbox.glyphs, 3, 0, 0 );
  
  // and again for gear
  QLabel* gearLabel = new QLabel( tr( "Default Gear" ) );
  textbox.gear = new SC_TextEdit;
  textbox.gear -> resize( 200, 250 );
  textbox.gear -> setPlainText( "head=\nneck=\n " );
  
  // assign the label and edit box to cells
  gridLayout -> addWidget( gearLabel,    0, 1, 0 );
  gridLayout -> addWidget( textbox.gear, 1, 1, 0 );
  
  // and again for rotation
  QLabel* rotationLabel = new QLabel( tr( "Default Rotation" ) );
  textbox.rotation = new SC_TextEdit;
  textbox.rotation -> resize( 200, 250 );
  textbox.rotation -> setPlainText( "actions=/auto_attack\n" );
  
  // assign the label and edit box to cells
  gridLayout -> addWidget( rotationLabel,    2, 1, 0 );
  gridLayout -> addWidget( textbox.rotation, 3, 1, 0 );

  // Add a box on the side for rotation conversion text (TBD)
  QLabel* sidebarLabel = new QLabel( tr( "Rotation Abbreviations" ) );
  textbox.sidebar = new SC_TextEdit;
  textbox.sidebar -> setText( " Stuff Goes Here" );
  textbox.sidebar -> setDisabled( true );
  gridLayout -> addWidget( sidebarLabel,    0, 2, 0 );
  gridLayout -> addWidget( textbox.sidebar, 1, 2, 3, 1, 0 );
  
  // this adjusts the relative width of each column
  gridLayout -> setColumnStretch( 0, 1 );
  gridLayout -> setColumnStretch( 1, 2 );
  gridLayout -> setColumnStretch( 2, 1 );

  // set the layout of the grid box
  gridGroupBox -> setLayout( gridLayout );
  
  // add the grid box to the main layout
  mainLayout -> addWidget( gridGroupBox );

  // set the tab's layout to mainLayout
  automationTabScrollArea -> setLayout( mainLayout );

  // Finally, add the tab
  addTab( automationTabScrollArea, tr( "Automation" ) );

  //connect( optionsTab, SIGNAL( armory_region_changed( const QString& ) ), this, SLOT( armoryRegionChanged( const QString& ) ) );
}