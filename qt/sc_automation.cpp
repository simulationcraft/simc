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
////                        Constants
////
///////////////////////////////////////////////////////////////////////////////



QString classSpecText[ 11 ][ 5 ] = {
    { "Death Knight", "Blood", "Frost", "Unholy", "N/A" },
    { "Druid", "Balance", "Feral", "Guardian", "Restoration" },
    { "Hunter", "Beast Master", "Marksmanship", "Survival", "N/A" },
    { "Mage", "Arcane", "Fire", "Frost", "N/A" },
    { "Monk", "Brewmaster", "Mistweaver", "Windwalker", "N/A" },
    { "Paladin", "Holy", "Protection", "Retribution", "N/A" },
    { "Priest", "Discipline", "Holy", "Shadow", "N/A" },
    { "Rogue", "Assassination", "Combat", "Subtlety", "N/A" },
    { "Shaman", "Elemental", "Enhancement", "Restoration", "N/A" },
    { "Warlock", "Affliction", "Demonology", "Destruction", "N/A" },
    { "Warrior", "Arms", "Fury", "Protection", "N/A" }
};

///////////////////////////////////////////////////////////////////////////////
////
////                        Logic Stuff
////
///////////////////////////////////////////////////////////////////////////////


// Local method to perform tokenize on QStrings
QString automation::tokenize( QString qstr )
{
  std::string temp = qstr.toLocal8Bit().constData();
  util::tokenize( temp );
  
  return QString::fromStdString( temp );
}

QString automation::do_something( int sim_type,
                                  QString player_class,
                                  QString player_spec,
                                  QString player_race,
                                  QString player_level,
                                  QString player_talents,
                                  QString player_glyphs,
                                  QString player_gear,
                                  QString player_rotation,
                                  QString advanced_text,
                                  QString sidebar_text
                                  )
{
  QString profile;
  QString base_profile_info; // class, spec, race & level definition
  QStringList advanced_list; // list constructed from advanced_text

  // basic profile information
  base_profile_info += "specialization=" + tokenize( player_spec ) + "\n";
  base_profile_info += "race=" + tokenize( player_race ) + "\n";
  base_profile_info += "level=" + player_level + "\n";

  // Take advanced_text and try splitting by \n\n
  advanced_list = advanced_text.split( "\n\n", QString::SkipEmptyParts );

  // If that did split it, we'll just feed advanced_list to the appropriate sim.
  // If not, then we try to split on \n instead
  if ( advanced_list.size() == 1 )
    advanced_list = advanced_text.split( "\n", QString::SkipEmptyParts );

  // check for advanced input mode (This can be switched to a CheckBox easily)
  //if ( input_mode == 1 )
  //{
  //  // do advanced stuff here
  //  profile = "ADVANCED MODE INCOMING !!";
  //  return profile;
  //}

  // simulation type check
  switch ( sim_type )
  {
    case 1: // talent simulation
      return auto_talent_sim( player_class, base_profile_info, advanced_list, player_glyphs, player_gear, player_rotation );
    case 2: // glyph simulation
      return auto_glyph_sim( player_class, base_profile_info, player_talents, advanced_list, player_gear, player_rotation );
    case 3: // gear simulation
      return auto_gear_sim( player_class, base_profile_info, player_talents, player_glyphs, advanced_list, player_rotation );
    case 4: // rotation simulation
      return auto_rotation_sim( player_class, player_spec, base_profile_info, player_talents, player_glyphs, player_rotation, advanced_list, sidebar_text );
    default: // default profile creation
      profile += base_profile_info;
      profile += "talents=" + player_talents + "\n";
      profile += "glyphs=" + player_glyphs + "\n";
      profile += player_gear + "\n";
      profile += player_rotation;
      return profile;

  }

  return profile;
}

  // Method for profile creation for the specific TALENT simulation
QString automation::auto_talent_sim( QString player_class,
                                     QString base_profile_info, 
                                     QStringList talentList,
                                     QString player_glyphs,
                                     QString player_gear,
                                     QString player_rotation
                                   )
{
  QString profile;
  
  // make profile for each entry in talentList
  for ( int i = 0; i < talentList.size(); i++ )
  {
    // first, check to see if the user has specified additional options within the list entry.
    // If so, we want to split them off and append them to the end. We do this by splitting on spaces and \n
    QStringList splitEntry = talentList[ i ].split( QRegExp( "\\n|\\s" ), QString::SkipEmptyParts );
    
    profile += tokenize( player_class ) + "=T_" + splitEntry[ 0 ] + "\n";
    profile += base_profile_info;

    if ( splitEntry[ 0 ].startsWith( "talents=" ) )
      profile += splitEntry[ 0 ];
    else 
      profile += "talents=" + splitEntry[ 0 ];
    profile += "\n";

    profile += "glyphs=" + player_glyphs + "\n";

    if ( player_gear.size() > 0 )
      profile += player_gear + "\n";
    if ( player_rotation.size() > 0 )
      profile += player_rotation + "\n";

    if ( splitEntry.size() > 1 )
      for ( int j = 1; j < splitEntry.size(); j++ )
        profile += splitEntry[ j ] + "\n";

    profile += "\n";
  }

  return profile;
}

  // Method for profile creation for the specific GLYPHS simulation
QString automation::auto_glyph_sim( QString player_class,
                                    QString base_profile_info,
                                    QString player_talents,
                                    QStringList glyphList,
                                    QString player_gear,
                                    QString player_rotation
                                  )
{
  QString profile;
  
  // make profile for each entry in glyphList
   for ( int i = 0; i < glyphList.size(); i++ )
  {
    // first, check to see if the user has specified additional options within the list entry.
    // If so, we want to split them off and append them to the end. We do this by splitting on spaces and \n
    QStringList splitEntry = glyphList[ i ].split( QRegExp( "\\n|\\s" ), QString::SkipEmptyParts );

    profile += tokenize( player_class ) + "=G_" + QString::number( i ) + "\n";
    profile += base_profile_info;
    profile += "talents=" + player_talents + "\n";

    if ( splitEntry[ 0 ].startsWith( "glyphs=" ) )
      profile += splitEntry[ 0 ];
    else
      profile += "glyphs=" + splitEntry[ 0 ];
    profile += "\n";

    if ( player_gear.size() > 0 )
      profile += player_gear + "\n";
    if ( player_rotation.size() > 0 )
      profile += player_rotation + "\n";

    if ( splitEntry.size() > 1 )
      for ( int j = 1; j < splitEntry.size(); j++ )
        profile += splitEntry[ j ] + "\n";

    profile += "\n";
  }

  return profile;
}

// Method for profile creation for the specific GEAR simulation
QString automation::auto_gear_sim( QString player_class,
                                   QString base_profile_info,
                                   QString player_talents,
                                   QString player_glyphs,
                                   QStringList gearList,
                                   QString player_rotation
                                   )
{
  QString profile;
  
  // make profile for each entry in gearList, which should be a complete gear set (plus optional extra stuff)
  for ( int i = 0; i < gearList.size(); i++ )
  {
    profile += tokenize( player_class ) + "=G_" + QString::number( i ) + "\n";
    profile += base_profile_info;
    profile += "talents=" + player_talents + "\n";
    profile += "glyphs=" + player_glyphs + "\n";

    if ( player_rotation.size() > 0 )
      profile += player_rotation + "\n";

    // split the entry on newlines and spaces just to separate it all out
    // (in practice unnecessary, but helps make the generated profiles more readable)
    QStringList itemList = gearList[ i ].split( QRegExp( "\\n|\\s" ), QString::SkipEmptyParts );
    
    for ( int j = 0; j < itemList.size(); j++ )
      profile += itemList[ j ] + "\n";

    // note that we don't bother pushing extra options to the end here, since the gear set is
    // already being specified at the end of the profile.

    profile += "\n";
  }
  return profile;
}

// Method for profile creation for the specific ROTATION simulation
QString automation::auto_rotation_sim( QString player_class,
                                       QString player_spec,
                                       QString base_profile_info,
                                       QString player_talents,
                                       QString player_glyphs,
                                       QString player_gear,
                                       QStringList rotation_list,
                                       QString sidebar_text
                                     )
{
  QString profile;

  for ( int i = 0; i < rotation_list.size(); i++ )
  {
    profile += tokenize( player_class ) + "=R_" + QString::number( i ) + "\n";
    profile += base_profile_info;
    profile += "talents=" + player_talents + "\n";
    profile += "glyphs=" + player_glyphs + "\n";
    if ( player_gear.size() > 0 )
      profile += player_gear + "\n";
    
    // Since action lists can be specified as shorthand with options or as full lists, we need to support both.
    // To do that, we split on newlines and spaces just like in the other modules
    QStringList actionList = rotation_list[ i ].split( QRegExp( "\\n|\\s" ), QString::SkipEmptyParts );

    // now, we check to see if the first element is a shorthand
    QStringList shorthandList = actionList[ 0 ].split( ">", QString::SkipEmptyParts );

    // if the shorthand list has more than one element, and this wasn't because of a ">" in a conditional, 
    // we have a shorthand sequence and need to do some conversion.
    if ( shorthandList.size() > 0 && ! shorthandList[ 0 ].startsWith( "actions" ) )
    {
      profile += "#Warning: Shorthand Conversion not yet complete - waits aren't working quite yet\n";

      // send shorthandList off to a method for conversion based on player class and spec
      QStringList convertedAPL = convert_shorthand( shorthandList, sidebar_text );

      // take the returned QStringList and output it.
      for ( int i = 0; i < convertedAPL.size(); i++ )
        profile += convertedAPL[ i ] + "\n";
    }

    // Otherwise, the user has specified the action list in its full and gory detail, so we can just use that
    // Spit out the first element here, the rest gets spit out below (for either case)
    else
      profile += actionList[ 0 ] + "\n";

    // spit out the rest of the actionList
    if ( actionList.size() > 1 )
      for ( int j = 1; j < actionList.size(); j++ )
        profile += actionList[ j ] + "\n";

    profile += "\n";
  }
  return profile;
}

QStringList automation::splitOption( QString s )
{
  QStringList optionsBreakdown;
  int numSeparators = s.count( "(" ) + s.count( ")" ) + s.count( "&" ) + s.count( "|" )+s.count( "!" );
  QRegExp delimiter( "[&|\\(\\)\\!]" );

  // ideally we will split this into ~2*numSeparators parts 
  if ( numSeparators > 0 )
  {
    // section the string
    for ( int i = 0; i <= numSeparators; i++ )
      optionsBreakdown << s.section( delimiter, i, i, QString::SectionIncludeTrailingSep ).simplified();

    // check for the trailing separators and move them to their own entries
    for ( int i = 0; i < optionsBreakdown.size(); i++ )
    {
      QString temp = optionsBreakdown[ i ];
      if ( temp.size() > 1 && delimiter.indexIn( temp ) == temp.size() - 1 )
      {
        optionsBreakdown.insert( i + 1, temp.right( 1 ) );
        optionsBreakdown[ i ].remove( temp.size() -1, 1 );
      }
    }
  }
  else
    optionsBreakdown.append( s );

  // at this point, optionsBreakdown should consist of entries that are only delimiters or abbreviations, no mixing
  return optionsBreakdown;
}

QStringList automation::convert_shorthand( QStringList shorthandList, QString sidebar_text )
{
  // STEP 1:  Take the sidebar list and convert it to an abilities table and an options table

  QStringList sidebarSplit = sidebar_text.split( QRegExp( "\\n\\n" ), QString::SkipEmptyParts );
  
  assert( sidebarSplit.size() == 2 );

  QString abilityConversions = sidebarSplit[ 0 ];
  QString optionConversions = sidebarSplit[ 1 ];

  QStringList abilityList = abilityConversions.split( "\n", QString::SkipEmptyParts );
  QStringList optionsList = optionConversions.split( "\n", QString::SkipEmptyParts );

  typedef std::vector<std::pair<QString, QString>> shorthandTable;
  shorthandTable abilityTable;
  shorthandTable optionsTable;

  // Construct the table for ability conversions
  for ( int i = 0; i < abilityList.size(); i++ )
  {
    QStringList parts = abilityList[ i ].split( "=", QString::SkipEmptyParts );
    if ( parts.size() > 1 )
    {
      // some conditionals will have ='s in them, and will generate extra elements in parts. 
      // To handle that, we'll duplicate parts, remove the first element (the abbreviation),
      // and then join the remainder for the second QString of the pair.
      QStringList rest = parts;
      rest.removeFirst();

      abilityTable.push_back( std::make_pair( parts[ 0 ], rest.join( "=" ) ) );
    }
  }

  // Construct the table for ability conversions
  for ( int i = 0; i < optionsList.size(); i++ )
  {
    QStringList parts = optionsList[ i ].split( "=", QString::SkipEmptyParts );
    if ( parts.size() > 1 )
    {
      // some conditionals will have ='s in them, and will generate extra elements in parts. 
      // To handle that, we'll duplicate parts, remove the first element (the abbreviation),
      // and then join the remainder for the second QString of the pair.
      QStringList rest = parts;
      rest.removeFirst();

      optionsTable.push_back( std::make_pair( parts[ 0 ], rest.join( "=" ) ) );
    }
  }

  // STEP 2: now we take the shorthand and figure out what to do with each element

  QStringList actionPriorityList; // Final APL

  // this structure is needed for finding abbreviations in the tables
  struct comp
  {
    comp( QString const& s ) : _s( s ) {}
    bool operator () ( std::pair< QString, QString> const& p )
    {
      return ( p.first == _s );
    }
    QString _s;
  };

  // cycle through the list of shorthands, converting as we go and appending to actionPriorityList
  for ( int i = 0; i < shorthandList.size(); i++ )
  {
    QString ability;
    QString options;

    // first, split into ability abbreviation and options set
    QStringList splits = shorthandList[ i ].split( "+", QString::SkipEmptyParts );
    assert( splits.size() <= 2 );

    // use abilityTable to replace the abbreviation with the full ability name
    shorthandTable::iterator m = find_if( abilityTable.begin(), abilityTable.end(), comp( splits[ 0 ] ) );
    if ( m != abilityTable.end() )
    {
      ability = m -> second;

      // now handle options if there are any
      if ( splits.size() > 1 )
      {
        // options are specified just as in simc, but with shorthands, e.g. as A&(B|!C)
        // we need to split on [&|()!], match the resulting abbreviations in the table,
        // and use those match pairs to replace the appropriate portions of the options string.
        // This method takes the options QString and returns a QStringList of symbols [&!()!] & abbreviations.
        QStringList optionsBreakdown = splitOption( splits[ 1 ] );

        // now cycle through this QStringList and replace any recognized shorthands with their longhand forms
        for ( int j = 0; j < optionsBreakdown.size(); j++ )
        {
          // if this entry is &|()!, skip it
          if ( optionsBreakdown[ j ].contains( QRegExp( "[&|\\(\\)\\!]+" ) ) )
            continue;

          // Each option can have a text part and a numeric part (e.g. W3). We need to split that up using regular expressions
          // captures[ 0 ] is the whole string, captures[ 1 ] is the first match (e.g. "W"), captures[ 2 ] is the second match (e.g. "3")
          QRegExp rx( "(\\D+)(\\d*\\.?\\d*)" );
          int pos = rx.indexIn( optionsBreakdown[ j ] );
          QStringList captures = rx.capturedTexts();

          // if we have just a text component, it's fairly easy
          if ( captures[ 2 ].length() == 0 )
          {
            shorthandTable::iterator m = find_if( optionsTable.begin(), optionsTable.end(), comp( captures[ 1 ] ) );
            if ( m != optionsTable.end() )
              optionsBreakdown[ j ] = m -> second;
          }
          // otherwise we need to do some trickery with numbers. Search for "W#" in the table and replace, then replace # with our number (e.g. "3")
          else
          {
            shorthandTable::iterator m = find_if( optionsTable.begin(), optionsTable.end(), comp( captures[ 1 ] + "#" ) );
            if ( m != optionsTable.end() )
              optionsBreakdown[ j ] = m -> second.replace( "#", captures[ 2 ] );
          }

        } // end for loop

        // TODO: Here we need to check for wait options and handle them a little differently
        // Ex: CS+W3 should translate into two lines:
        // actions+=/crusader_strike
        // actions+=/wait,sec=cooldown.crusader_strike.remains,if=cooldown.crusader_strike.remains>0&cooldown.crusader_strike.remains<=3
        // Thus, we eventually need to do some fancy stuff for waits here before merging options

        // at this point, we should be able to merge the optionsBreakdown list into a string
        options = optionsBreakdown.join( "" );
      }

      // combine the ability and options into a single string
      QString entry = "actions";
      if ( ! actionPriorityList.empty() )
        entry += "+";
      entry += "=/" + ability;
      if ( options.length() > 0 )
        entry += ",if=" + options;

      // add the entry to actionPriorityList
      actionPriorityList.append( entry );
    }
  }

  return actionPriorityList;
}

///////////////////////////////////////////////////////////////////////////////
////
////                        GUI Stuff
////
///////////////////////////////////////////////////////////////////////////////


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

QComboBox* createInputMode( int count, ... )
{
  QComboBox* input_mode = new QComboBox();
  va_list vap;
  va_start( vap, count );
  for ( int i = 0; i < count; i++ )
    input_mode -> addItem( va_arg( vap, char* ) );
  va_end( vap );
  return input_mode;
}

// Method to set the "Spec" drop-down based on "Class" selection
void SC_ImportTab::setSpecDropDown( const int player_class )
{
  // If we have a fourth spec, remove it here
  if ( choice.player_spec -> count() > 3 )
    choice.player_spec -> removeItem( 3 );

  choice.player_spec -> setItemText( 0, classSpecText[ player_class ][ 1 ] );
  choice.player_spec -> setItemText( 1, classSpecText[ player_class ][ 2 ] );
  choice.player_spec -> setItemText( 2, classSpecText[ player_class ][ 3 ] );
  if ( player_class == 1 )
    choice.player_spec -> addItem( classSpecText[ player_class ][ 4 ] );
}

// constant for sidebar text (this will eventually get really long)
QString sidebarText[ 11 ][ 4 ] = {
  { "Blood shorthand goes here.", "Frost shorthand goes here", "Unholy shorthand goes here", "N/A" },
  { "Balance shorthand goes here.", "Feral shorthand goes here.", "Guardian shorthand goes here.", "Restoration shorthand goes here." },
  { "Beast Master shorthand goes here.", "Marksmanship shorthand goes here.", "Survival shorthand goes here." },
  { "Arcane shorthand goes here.", "Fire shorthand goes here.", "Frost shorthand goes here.", "N/A" },
  { "Brewmaster shorthand goes here.", "Mistweaver shorthand goes here.", "Windwalker shorthand goes here.", "N/A" },
  { "Holy shorthand goes here.",
  ":::Abilities:::\nCS=crusader_strike\nJ=judgment\nAS=avengers_shield\nHW=holy_wrath\nHotR=hammer_of_the_righteous\nHoW=\hammer_of_wrath\nCons=consecration\nSS=sacred_shield\nHPr=holy_prism\nES=execution_sentence\nLH=lights_hammer\nSotR=shield_of_the_righteous\nEF=eternal_flame\nWoG=word_of_glory\nSoI=seal_of_insight\nSoT=seal_of_truth\nSoR=seal_of_righteousness\nSP=seraphim\n\n:::Options:::\nW#=wait,sec=#\nGC=buff.grand_crusader.react\nGC#=buff.grand_crusader.remains<#\nDP=buff.divine_purpose.react\nDPHP#=buff.divine_purpose.react|holy_power>=#\nE=target.health.pct<=20\nFW=glyph.final_wrath.enabled&target.health.pct<=20\nHP#=holy_power>=#\nNT=!ticking\nNF=target.debuff.flying.down\nSW=talent.sanctified_wrath.enabled\nSP=buff.seraphim.react\nT#=active_enemies>=#\nR#=buff.<ability_name>.remains<#\n",
  "Retribution shorthand goes here.", "N/A" },
  { "Discipline shorthand goes here.", "Holy shorthand goes here.", "Shadow shorthand goes here.", "N/A" },
  { "Assasination shorthand goes here.", "Combat shorthand goes here.", "Subtlety shorthand goes here.", "N/A" },
  { "Elemental shorthand goes here.", "Enhancement shorthand goes here.", "Restoration shorthand goes here.", "N/A" },
  { "Affliction shorthand goes here.", "Demonology shorthand goes here.", "Destruction shorthand goes here.", "N/A" },
  { "Arms shorthand goes here.", "Fury shorthand goes here.", "Protection shorthand goes here.", "N/A" },
};

// constant for the varying labels of the advanced text box
QString advancedText[ 6 ] = {
  "Unused", "Talent Configurations", "Glyph Configurations", "Gear Configurations", "Rotation Configurations", "Advanced Configuration Mode"
};
// constant for the varying labels of the helpbar text box
QString helpbarText[ 6 ] = {
  "To automate generation of a comparison, choose a comparison type from the drop-down box to the left.",
  "List the talent configurations you want to test in the box below as seven-digit integer strings, i.e. 1231231.\nEach configuration should be its own new line.\nFor more advanced syntax options, see the wiki entry at (PUT LINK HERE).",
  "List the glyph configurations you want to test (i.e. alabaster_shield/focused_shield/word_of_glory) in the box below.\nEach configuration should be its own new line.\nFor more advanced syntax options, see the wiki entry at (PUT LINK HERE).",
  "List the different gear configurations you want to test in the box below.\nEach configuration should be separated by a blank line.\nFor more advanced syntax options, see the wiki entry at (PUT LINK HERE).",
  "List the rotations you want to test in the box below. Rotations can be specified as usual for simc (with different configurations separated by a blank line) or as shorthands (each on a new line).\nThe sidebar lists the shorthand conventions for your class and spec. If a shorthand you want isn't documented, please contact the simulationcraft team to have it added.\nFor more advanced syntax options, see the wiki entry at (PUT LINK HERE).",
  "To specify everything according to your needs. All configurations can be mixed up for more advanced multi-automated simulations but need to match the standard SimCraft input format"
};


// method to set the sidebar text based on class slection
void SC_ImportTab::setSidebarClassText()
{
  textbox.sidebar -> setText( sidebarText[ choice.player_class -> currentIndex() ][ choice.player_spec -> currentIndex() ] );
}

void SC_ImportTab::inputTypeChanged( const int mode ){
 
  // get current comparison type
  int comp = choice.comp_type -> currentIndex();

  // set the label of the Advanced tab depending on comparison type chosen
  switch ( mode )
  {
    case 0:
      // set boxes & labels to the current comparison choice
      choice.comp_type -> setDisabled ( false );
      compTypeChanged( comp );
      break;
    case 1:
      // set the label of the Advanced tab
      label.advanced -> setText( advancedText[ 5 ] );

      // set the text of the help bar for advanced mode
      textbox.helpbar -> setText( helpbarText[ 5 ] );

      textbox.advanced -> setDisabled( false );
      textbox.gear -> setDisabled( true );
      textbox.glyphs -> setDisabled( true );
      textbox.rotation -> setDisabled( true );
      textbox.talents -> setDisabled( true );

      choice.comp_type -> setDisabled ( true );
      break;
  }
}

void SC_ImportTab::compTypeChanged( const int comp )
{
  // store whatever text is in the advanced window in the appropriate QString 
  if ( ! textbox.talents -> isEnabled() )
    advTalent = textbox.advanced -> document() -> toPlainText();
  else if ( ! textbox.glyphs -> isEnabled() )
    advGlyph = textbox.advanced -> document() -> toPlainText();
  else if ( ! textbox.gear -> isEnabled() )
    advGear = textbox.advanced -> document() -> toPlainText();
  else if ( ! textbox.rotation -> isEnabled() )
    advRotation = textbox.advanced -> document() -> toPlainText();

  // set the label of the Advanced tab appropriately
  label.advanced -> setText( advancedText[ comp ] );

  // set the text of the help bar appropriately
  textbox.helpbar -> setText( helpbarText[ comp ] );
  
  // TODO: set the text of the box with some sort of default for each case

  textbox.advanced -> setDisabled( false );
  textbox.gear     -> setDisabled( false );
  textbox.glyphs   -> setDisabled( false );
  textbox.rotation -> setDisabled( false );
  textbox.talents  -> setDisabled( false );
  textbox.sidebar  -> setDisabled( true  );

  switch ( comp )
  {
    case 0: 
      textbox.advanced -> setDisabled( true );
      break;
    case 1:
      textbox.advanced -> setText( advTalent );
      textbox.talents -> setDisabled( true );
      break;
    case 2:
      textbox.glyphs -> setDisabled( true );
      textbox.advanced -> setText( advGlyph );
      break;
    case 3:
      textbox.gear -> setDisabled( true );
      textbox.advanced -> setText( advGear );
      break;
    case 4:
      textbox.rotation -> setDisabled( true );
      textbox.advanced -> setText( advRotation );
      textbox.sidebar -> setDisabled( false );
      break;
  }
}

void SC_MainWindow::startAutomationImport( int tab )
{
  QString profile;

  profile = automation::do_something( importTab -> choice.comp_type -> currentIndex(),
                                      importTab -> choice.player_class -> currentText(),
                                      importTab -> choice.player_spec -> currentText(),
                                      importTab -> choice.player_race -> currentText(),
                                      importTab -> choice.player_level -> currentText(),
                                      importTab -> textbox.talents -> text(),
                                      importTab -> textbox.glyphs -> text(),
                                      importTab -> textbox.gear -> document() -> toPlainText(),
                                      importTab -> textbox.rotation -> document() -> toPlainText(),
                                      importTab -> textbox.advanced -> document() -> toPlainText(),
                                      importTab -> textbox.sidebar -> document() -> toPlainText()
                                    );

  simulateTab -> add_Text( profile,  tr( "Testing" ) );
  
  mainTab -> setCurrentTab( TAB_SIMULATE );
}

void SC_ImportTab::createTooltips()
{
  choice.comp_type -> setToolTip( "Choose the comparison type." );
  //choice.input_mode -> setToolTip( "Select to use advanced input mode.\nAdvanced input mode allows for more complex\ninput for multi-automated simulations." );
  textbox.sidebar -> setToolTip( "This box specifies the abbreviations used in rotation shorthands. You may define custom abbreviations by adding your own entries." );
  label.sidebar -> setToolTip( "This box specifies the abbreviations used in rotation shorthands. You may define custom abbreviations by adding your own entries." );
}

void SC_ImportTab::createAutomationTab()
{ 
  // layout building based on 
  // http://qt-project.org/doc/qt-4.8/layouts-basiclayouts.html

  // This scroll area is the parent of the entire tab
  QScrollArea* automationTabScrollArea = new QScrollArea;

  // Define a grid Layout which we will eventually apply to the scroll area
  QGridLayout* gridLayout = new QGridLayout;

  // Now we start adding things to the grid layout

  // Element (0,0) is a GroupBox containing a FormLayout with one ComboBox (choice of sim type)
  // Create box and add to Layout
  QGroupBox* compGroupBox = new QGroupBox();
  gridLayout -> addWidget( compGroupBox, 0, 0, 0 );

  // Define a layout for the box
  QFormLayout* compLayout = new QFormLayout();
  compLayout -> setFieldGrowthPolicy( QFormLayout::FieldsStayAtSizeHint );
  
  // Create Combo Box and add it to the layout
  choice.comp_type = createChoice( 5 , "None", "Talents", "Glyphs", "Gear", "Rotation" );
  compLayout -> addRow( tr( "Comparison Type" ), choice.comp_type );

  //// Ceate a Checkbox for input mode
  //choice.input_mode = createInputMode( 2 , "Simple", "Advanced" );
  //compLayout->addRow( tr( "Input Mode" ), choice.input_mode );

  // Apply the layout to the compGroupBox
  compGroupBox -> setLayout( compLayout );
  

  // Elements (0,1) - (0,2) are the helpbar
  textbox.helpbar = new SC_TextEdit;
  textbox.helpbar -> setDisabled( true );
  textbox.helpbar -> setMaximumHeight( 65 );
  textbox.helpbar -> setMinimumHeight( 50 );
  textbox.helpbar -> setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
  gridLayout -> addWidget( textbox.helpbar, 0, 1, 1, 2, 0 );


  // Elements (1,0) & (2,0) are a QLabel and QGroupBox for some of the defaults  
  QLabel* defaultsLabel = new QLabel( tr( "Defaults" ) );
  gridLayout -> addWidget( defaultsLabel, 1, 0, 0 );

  // create a box for the defaults section
  QGroupBox* defaultsGroupBox = new QGroupBox();
  gridLayout -> addWidget( defaultsGroupBox, 2, 0, 0 );

  // define a FormLayout for the GroupBox
  QFormLayout* defaultsFormLayout = new QFormLayout();
  defaultsFormLayout -> setFieldGrowthPolicy( QFormLayout::ExpandingFieldsGrow );

  // Create Combo Boxes and add to the FormLayout
  choice.player_class = createChoice( 11 , "Death Knight", "Druid", "Hunter", "Mage", "Monk", "Paladin", "Priest", "Rogue", "Shaman", "Warlock", "Warrior" );
  defaultsFormLayout -> addRow( tr( "Class" ), choice.player_class );

  choice.player_spec = createChoice( 3, "1", "2", "3" );
  choice.player_spec -> setSizeAdjustPolicy( QComboBox::AdjustToContents );
  setSpecDropDown( choice.player_class -> currentIndex() );
  defaultsFormLayout -> addRow( tr( "Spec" ), choice.player_spec );

  choice.player_race = createChoice( 13, "Blood Elf", "Draenei", "Dwarf", "Gnome", "Goblin", "Human", "Night Elf", "Orc", "Pandaren", "Tauren", "Troll", "Undead", "Worgen");
  defaultsFormLayout -> addRow( tr( "Race" ), choice.player_race );

  choice.player_level = createChoice( 2, "100", "90" );
  defaultsFormLayout -> addRow( tr( "Level" ), choice.player_level );

  // Create text boxes for default talents and glyphs, and add them to the FormLayout
  label.talents = new QLabel( tr("Default Talents" ) );
  textbox.talents = new QLineEdit;
  defaultsFormLayout -> addRow( label.talents, textbox.talents );
  
  label.glyphs = new QLabel( tr("Default Glyphs" ) );
  textbox.glyphs = new QLineEdit;
  defaultsFormLayout -> addRow( label.glyphs, textbox.glyphs );

  // set the GroupBox's layout now that we've defined it
  defaultsGroupBox -> setLayout( defaultsFormLayout );

  
  // Elements (3,0) - (6,0) are the default gear and rotation labels and text boxes
  
  // Create a label and an edit box for gear
  label.gear = new QLabel( tr( "Default Gear" ) );
  textbox.gear = new SC_TextEdit;
  // assign the label and edit box to cells
  gridLayout -> addWidget( label.gear,   3, 0, 0 );
  gridLayout -> addWidget( textbox.gear, 4, 0, 0 );
  
  // and again for rotation
  label.rotation = new QLabel( tr( "Default Rotation" ) );
  textbox.rotation = new SC_TextEdit;
  gridLayout -> addWidget( label.rotation,   5, 0, 0 );
  gridLayout -> addWidget( textbox.rotation, 6, 0, 0 );


  // Elements (2,1) - (6,1) are the Advanced text box, which is actually
  // N different text boxes that get cycled depending on sim type choice

  label.advanced = new QLabel( tr( "Advanced Text Box" ) );
  textbox.advanced = new SC_TextEdit;
  gridLayout -> addWidget( label.advanced,   1, 1, 0 );
  gridLayout -> addWidget( textbox.advanced, 2, 1, 5, 1, 0 );


  // Eleements (2,2) - (6,2) are the Rotation Conversions text box 

  label.sidebar = new QLabel( tr( "Rotation Abbreviations" ) );
  textbox.sidebar = new SC_TextEdit;
  textbox.sidebar -> setText( " Stuff Goes Here" );
  gridLayout -> addWidget( label.sidebar,   1, 2, 0 );
  gridLayout -> addWidget( textbox.sidebar, 2, 2, 5, 1, 0 );
  
  // this adjusts the relative width of each column
  gridLayout -> setColumnStretch( 0, 1 );
  gridLayout -> setColumnStretch( 1, 2 );
  gridLayout -> setColumnStretch( 2, 1 );
  
  // set the tab's layout to mainLayout
  automationTabScrollArea -> setLayout( gridLayout );

  // Finally, add the tab
  addTab( automationTabScrollArea, tr( "Automation" ) );

  connect( choice.player_class, SIGNAL( currentIndexChanged( const int& ) ), this, SLOT( setSpecDropDown( const int ) ) );
  connect( choice.player_class, SIGNAL( currentIndexChanged( const int& ) ), this, SLOT( setSidebarClassText() ) );
  connect( choice.player_spec,  SIGNAL( currentIndexChanged( const int& ) ), this, SLOT( setSidebarClassText() ) );
  connect( choice.comp_type,    SIGNAL( currentIndexChanged( const int& ) ), this, SLOT( compTypeChanged( const int ) ) );
  //connect( choice.input_mode,   SIGNAL( currentIndexChanged( const int& ) ), this, SLOT( inputTypeChanged( const int ) ) );

  // do some initialization
  compTypeChanged( choice.comp_type -> currentIndex() );
  //inputTypeChanged( choice.input_mode -> currentIndex() );

  // set up all the tooltips
  createTooltips();
}


// Encode all options and text fields into a string ( to be able to save it to the history )
// Decode / Encode order needs to be equal!

void SC_ImportTab::encodeSettings()
{
  QSettings settings;
  settings.beginGroup( "automation" );
  settings.setValue( "comp_type", choice.comp_type -> currentText() );
  settings.setValue( "class", choice.player_class -> currentText() );
  settings.setValue( "spec", choice.player_spec -> currentText() );
  settings.setValue( "race", choice.player_race -> currentText() );
  settings.setValue( "level", choice.player_level -> currentText() );
  settings.setValue( "talentsbox", textbox.talents -> text() );
  settings.setValue( "glyphsbox", textbox.glyphs -> text() );
  settings.setValue( "gearbox", textbox.gear -> document() -> toPlainText() );
  settings.setValue( "rotationbox", textbox.rotation -> document() -> toPlainText() );
  settings.setValue( "advancedbox", textbox.advanced -> document() -> toPlainText() );
  settings.setValue( "advTalent", advTalent );
  settings.setValue( "advGlyph", advGlyph );
  settings.setValue( "advGear", advGear );
  settings.setValue( "advRotation", advRotation );
  settings.setValue( "sidebox", textbox.sidebar -> document() -> toPlainText() );
  
  QString encoded;

  settings.endGroup(); // end group "automation"
}

/* Decode all options/setting from a string ( loaded from the history ).
 * Decode / Encode order needs to be equal!
 *
 * If no default_value is specified, index 0 is used as default.
 */

void SC_ImportTab::load_setting( QSettings& s, const QString& name, QComboBox* choice, const QString& default_value = QString() )
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

void SC_ImportTab::load_setting( QSettings& s, const QString& name, QString* text, const QString& default_value = QString() )
{
  const QString& v = s.value( name ).toString();

  if ( ! v.isEmpty() )
    *text = v;
  else if ( ! default_value.isEmpty() )
    *text = default_value;
}

void SC_ImportTab::load_setting( QSettings& s, const QString& name, QLineEdit* textbox, const QString& default_value = QString() )
{
  const QString& v = s.value( name ).toString();

  if ( !v.isEmpty() )
    textbox -> setText( v );
  else if ( !default_value.isEmpty() )
    textbox -> setText( default_value );
}

void SC_ImportTab::load_setting( QSettings& s, const QString& name, SC_TextEdit* textbox, const QString& default_value = QString() )
{
  const QString& v = s.value( name ).toString();

  if ( !v.isEmpty() )
    textbox -> setText( v );
  else if ( !default_value.isEmpty() )
    textbox -> setText( default_value );
}

void SC_ImportTab::decodeSettings()
{
  QSettings settings;
  settings.beginGroup( "automation" );
  load_setting( settings, "comp_type", choice.comp_type );
  load_setting( settings, "class", choice.player_class );
  load_setting( settings, "spec", choice.player_spec );
  load_setting( settings, "race", choice.player_race );
  load_setting( settings, "level", choice.player_level );
  load_setting( settings, "talentbox", textbox.talents, "0000000" );
  load_setting( settings, "glyphbox", textbox.glyphs, "alabaster_shield/focused_shield/final_wrath" );
  load_setting( settings, "gearbox", textbox.gear, "head=\nneck=\n" );
  load_setting( settings, "rotationbox", textbox.rotation );
  load_setting( settings, "advancedbox", textbox.advanced, "default" );
  load_setting( settings, "advTalent", &advTalent, "0000000\n1111111\n2222222" );
  load_setting( settings, "advGlyph", &advGlyph, "alabaster_shield/focused_shield\nfinal_wrath/word_of_glory" );
  load_setting( settings, "advGear", &advGear, "head=test\nneck=test" );
  load_setting( settings, "advRotation", &advRotation, "CS>J>AS\nCS>J>AS+GC\nCS>AS+GC&(DP|!FW)>J+SW>J" );
  load_setting( settings, "sidebox", textbox.sidebar );
  
  

  settings.endGroup();
}
