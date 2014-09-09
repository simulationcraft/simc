// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "simulationcraftqt.hpp"
#include "SC_SpellQueryTab.hpp"
#include "util/sc_mainwindowcommandline.hpp"

namespace { // unnamed namespace

struct OptionEntry
{
  const char* label;
  const char* option;
  const char* tooltip;
};

const int reforgePlotOption_cut = 7; // separate between secondary and primary stats

QComboBox* createChoiceFromRange( int lowerInclusive, int upperInclusive ) {
  QComboBox* choice = new QComboBox();
  for ( int i = lowerInclusive; i <= upperInclusive; i++ ) {
    QString choiceText = QString::number(i);
    choice -> addItem( choiceText );
  }
  return choice;
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

} // end unnamed namespace

SC_SpellQueryTab::SC_SpellQueryTab( SC_MainWindow* parent ) :
  QWidget( parent ), mainWindow( parent )
{
  // define a grid layout for placement of objects
  QGridLayout* gridLayout = new QGridLayout;

  // Add a label
  label.input = new QLabel( tr( "Input Options" ) );
  gridLayout -> addWidget( label.input, 0, 0, 0 );

  // Element (1,0) is a GroupBox containing the inputs
  QGroupBox* inputGroupBox = new QGroupBox();
  gridLayout -> addWidget( inputGroupBox, 1, 0, 0 );

  // Layout of the groupbox
  QFormLayout* inputGroupBoxLayout = new QFormLayout();
  inputGroupBoxLayout -> setFieldGrowthPolicy( QFormLayout::ExpandingFieldsGrow );


  // Add a combo box
  choice.spell = createChoice( 5, "spell", "talent_spell", "spec_spell", "mastery_spell", "glyph_spell" );
  inputGroupBoxLayout -> addRow( tr( "spell qualifier" ), choice.spell );

  // add anotehr combo box
  choice.filter = createChoice( 3, "name", "id", "class");
  inputGroupBoxLayout -> addRow( tr( "filter" ), choice.filter );

  // add a line edit for text input
  textbox.arg = new QLineEdit;
  inputGroupBoxLayout -> addRow( tr( "argument" ), textbox.arg );

  // apply the layout to the group box
  inputGroupBox -> setLayout( inputGroupBoxLayout );

  // Column 1 is the text output box
  label.output = new QLabel( tr( "Output" ) );
  textbox.result = new SC_TextEdit;
  gridLayout -> addWidget( label.output, 0, 1, 0 );
  gridLayout -> addWidget( textbox.result, 1, 1, 0 );
  
  // this adjusts the relative width of each column
  gridLayout -> setColumnStretch( 0, 1 );
  gridLayout -> setColumnStretch( 1, 3 );

  setLayout( gridLayout );
}

/* Decode all options/setting from a string ( loaded from the history ).
 * Decode / Encode order needs to be equal!
 *
 * If no default_value is specified, index 0 is used as default.
 */

void SC_SpellQueryTab::load_setting( QSettings& s, const QString& name, QComboBox* choice, const QString& default_value = QString() )
{
  const QString& v = s.value( name ).toString();

  int index = choice -> findText( v );
  if ( index != -1 )
    choice -> setCurrentIndex( index );
  else if ( !default_value.isEmpty() )
  {
    int default_index = choice -> findText( default_value );
    if ( default_index != -1 )
      choice -> setCurrentIndex( default_index );
  }
  else
  {
    choice -> setCurrentIndex( 0 );
  }
}

void SC_SpellQueryTab::run_spell_query()
{
  // everything we do to run a spell query goes in here!
  textbox.result -> setText( "Hello World." );
}

void SC_SpellQueryTab::decodeOptions()
{
  QSettings settings;
  settings.beginGroup( "spell_query" );
  load_setting( settings, "version", choice.spell );

  settings.endGroup();
}


// Encode all options/setting into a string ( to be able to save it to the history )
// Decode / Encode order needs to be equal!

void SC_SpellQueryTab::encodeOptions()
{
  QSettings settings;
  settings.beginGroup( "spell_query" );
  settings.setValue( "version", choice.spell -> currentText() );
  
  settings.endGroup(); // end group "options"
}

void SC_SpellQueryTab::createToolTips()
{
  choice.spell -> setToolTip( tr( "Test" ) );
}

