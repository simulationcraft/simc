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

QStringList automation::splitPreservingComments( QString qstr )
{
  QStringList outputList;

  // split the entry on newlines to separate it all out
  QStringList mainList = qstr.split( "\n", QString::SkipEmptyParts );

  // for each entry, do further processing
  for ( int j = 0; j < mainList.size(); j++ )
  {
    // first though, check to see if this line is a comment
    QRegExp comment = QRegExp( "\\s*\\#" );
    // if so, preserve it and remove leading/trailing spaces
    if ( comment.indexIn( mainList[ j ] ) == 0 )
      outputList.append( mainList[ j ].simplified() );
    // otherwise, try to split on spaces and add each element individually
    // (in practice unnecessary, but helps make the generated profiles more readable)    
    else
    {
      QStringList subList = mainList[ j ].split( QRegExp( "\\s" ), QString::SkipEmptyParts );
      for ( int l = 0; l < subList.size(); l++ )
        outputList.append( subList[ l ] );
    }
  }
  return outputList;
}

// this structure is needed for finding abbreviations in the tables
struct comp
{
  comp( QString const& s ) : _s( s ) {}
  bool operator () ( const std::pair< QString, QString>& p ) const
  {
    return ( p.first.toLower() == _s.toLower() );
  }
  QString _s;
};

QString automation::automation_main( int sim_type,
                                  QString player_class,
                                  QString player_spec,
                                  QString player_race,
                                  QString player_level,
                                  QString player_talents,
                                  QString player_glyphs,
                                  QString player_gear,
                                  QString player_rotationHeader,
                                  QString player_rotationFooter,
                                  QString advanced_text,
                                  QString sidebar_text,
                                  QString footer_text
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
  advanced_list = advanced_text.split( QRegExp("\\n\\s*\\n"), QString::SkipEmptyParts );

  // If that did split it, we'll just feed advanced_list to the appropriate sim.
  // If not, then we try to split on \n instead
  if ( advanced_list.size() == 1 )
    advanced_list = advanced_text.split( "\n", QString::SkipEmptyParts );
  
  // simulation type check
  switch ( sim_type )
  {
    case 1: // talent simulation
      profile += auto_talent_sim( player_class, base_profile_info, advanced_list, player_glyphs, player_gear, player_rotationHeader );
      break;
    case 2: // glyph simulation
      profile += auto_glyph_sim( player_class, base_profile_info, player_talents, advanced_list, player_gear, player_rotationHeader );
      break;
    case 3: // gear simulation
      profile += auto_gear_sim( player_class, base_profile_info, player_talents, player_glyphs, advanced_list, player_rotationHeader );
      break;
    case 4: // rotation simulation
      profile += auto_rotation_sim( player_class, player_spec, base_profile_info, player_talents, player_glyphs, player_gear, player_rotationHeader, player_rotationFooter, advanced_list, sidebar_text );
      break;
    default: // default profile creation
      profile += base_profile_info;
      profile += "talents=" + player_talents + "\n";
      profile += "glyphs=" + player_glyphs + "\n";
      profile += player_gear + "\n";
      profile += player_rotationHeader + "\n";

  }
  profile += footer_text;

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
  QStringList names;
  
  // make profile for each entry in talentList
  for ( int i = 0; i < talentList.size(); i++ )
  {
    // first, check to see if the user has specified additional options within the list entry.
    // If so, we want to split them off and append them to the end.
    QStringList splitEntry = splitPreservingComments( talentList[ i ] );
    
    // handle isolated comments
    if ( splitEntry.size() == 1 && splitEntry[ 0 ].startsWith( "#" ) )
      continue;
    
    profile += tokenize( player_class ) + "=T_";
    if ( ! names.contains( splitEntry[ 0 ] ) )
    {
      profile += splitEntry[ 0 ];
      names.append( splitEntry[ 0 ] );
    }
    else
      profile += QString::number( i );
    profile += "\n";
    profile += base_profile_info;

    // skip comments
    int k = 0;
    while ( k < splitEntry.size() - 1 && splitEntry[ k ].startsWith( "#" ) )
    {
      profile += splitEntry[ k ] + "\n";
      k++;
    }
    // the first non-comment entry should be our talents
    if ( splitEntry[ k ].startsWith( "talents=" ) )
      profile += splitEntry[ k ] + "\n";
    else 
      profile += "talents=" + splitEntry[ k ] + "\n";

    if ( player_glyphs.startsWith( "glyphs=" ) )
      profile += player_glyphs + "\n";
    else
      profile += "glyphs=" + player_glyphs + "\n";

    if ( player_gear.size() > 0 )
      profile += player_gear + "\n";
    if ( player_rotation.size() > 0 )
      profile += player_rotation + "\n";

    // add the remaining options at the end
    if ( splitEntry.size() > k + 1 )
      for ( int j = k + 1; j < splitEntry.size(); j++ )
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
    QStringList splitEntry = splitPreservingComments( glyphList[ i ] );
    
    // handle isolated comments
    if ( splitEntry.size() == 1 && splitEntry[ 0 ].startsWith( "#" ) )
      continue;

    profile += tokenize( player_class ) + "=G_" + QString::number( i ) + "\n";
    profile += base_profile_info;

    if ( player_talents.startsWith( "talents=" ) )
      profile += player_talents + "\n";
    else
      profile += "talents=" + player_talents + "\n";
    
    // skip comments
    int k = 0;
    while ( k < splitEntry.size() - 1 && splitEntry[ k ].startsWith( "#" ) )
    {
      profile += splitEntry[ k ] + "\n";
      k++;
    }
    // the first non-comment entry should be our glyphs
    if ( splitEntry[ k ].startsWith( "glyphs=" ) )
      profile += splitEntry[ k ] + "\n";
    else
      profile += "glyphs=" + splitEntry[ k ] + "\n";

    if ( player_gear.size() > 0 )
      profile += player_gear + "\n";
    if ( player_rotation.size() > 0 )
      profile += player_rotation + "\n";
    
    // add the remaining options at the end
    if ( splitEntry.size() > k + 1 )
      for ( int j = k + 1; j < splitEntry.size(); j++ )
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
    // split the entry on newlines and spaces, preserving comments
    QStringList itemList = splitPreservingComments( gearList[ i ] );

    // handle isolated comments
    if ( itemList.size() == 1 && itemList[ 0 ].startsWith( "#" ) )
      continue;

    profile += tokenize( player_class ) + "=G_" + QString::number( i ) + "\n";
    profile += base_profile_info;

    if ( player_talents.startsWith( "talents=" ) )
      profile += player_talents + "\n";
    else
      profile += "talents=" + player_talents + "\n";
    
    if ( player_glyphs.startsWith( "glyphs=" ) )
      profile += player_glyphs + "\n";
    else
      profile += "glyphs=" + player_glyphs + "\n";

    if ( player_rotation.size() > 0 )
      profile += player_rotation + "\n";

    // add each line of the reult to the profile
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
                                       QString player_rotationHeader,
                                       QString player_rotationFooter,
                                       QStringList rotation_list,
                                       QString sidebar_text
                                     )
{
  QString profile;

  for ( int i = 0; i < rotation_list.size(); i++ )
  {
    
    // Since action lists can be specified as shorthand with options or as full lists or a mix, we need to support both.
    // To do that, let's first split the provided configuration as usual:
    QStringList actionList = splitPreservingComments( rotation_list[ i ] );
    QString name; // placeholder for naming the actor based on shorthand

    // handle comments in single-newline-separated rotation lists
    if ( actionList.size() == 1 && actionList[ 0 ].startsWith( "#" ) )
      continue;

    QString action_block; // string for action block

    // build action block first - for better naming
    if ( player_rotationHeader.size() > 0 )
      action_block += "#acton header\n" + player_rotationHeader + "\n#rotation\n";

    // cycle through the actionList handling each entry one at a time
    for ( int j = 0; j < actionList.size(); j++ )
    {
      QString entry = actionList[ j ];

      // comments, longhand entries, and other unrecognizeable text just gets appended line by line.
      // we only need to perform conversions on shorthand entries, so we look for those specifically

      // check for a shorthand by looking for a ">" in something that isn't a comment, name, or a longhand line
      QStringList shorthandList = entry.split( ">", QString::SkipEmptyParts );
      if ( ! entry.startsWith( "#" ) && ! entry.startsWith( "name=") && ! entry.contains( "actions+=") && ! entry.contains( "actions=" ) && shorthandList.size() > 1 )
      {
        // send shorthandList off to a method for conversion based on player class and spec
        QStringList convertedAPL = convert_shorthand( shorthandList, sidebar_text );
        
        // use the shorthand to create a name for this actor, if we haven't already
        if ( name.length() == 0 )
          name = entry;

        // add each line of the result to profile
        for ( int q = 0; q < convertedAPL.size(); q++ )
          action_block += convertedAPL[ q ] + "\n";

        continue;
      }
      // otherwise it's some other option or text, we just want to output that like usual
      else
        action_block += entry + "\n";
    }

    // add action footer, if it exists
    if ( player_rotationFooter.size() > 0 )
      action_block += "#action footer\n" + player_rotationFooter + "\n";

    // build profile from components
    profile += tokenize( player_class ) + "=";
    if ( name.length() > 0 )
      profile += name;
    else
      profile+= QString::number( i );
    profile += "\n";

    profile += base_profile_info;

    if ( player_talents.startsWith( "talents=" ) )
      profile += player_talents + "\n";
    else
      profile += "talents=" + player_talents + "\n";
    
    if ( player_glyphs.startsWith( "glyphs=" ) )
      profile += player_glyphs + "\n";
    else
      profile += "glyphs=" + player_glyphs + "\n";
    
    if ( player_gear.size() > 0 )
      profile += player_gear + "\n";

    profile += action_block;

    // leave some space
    profile += "\n\n";
  }

  return profile;
}

QStringList automation::splitOption( QString s )
{
  QStringList optionsBreakdown;
  int numSeparators = s.count( "(" ) + s.count( ")" ) + s.count( "&" ) + s.count( "|" ) + s.count( "!" )
                    + s.count( "+" ) + s.count( "-" ) + s.count( ">" ) + s.count( "<" ) + s.count( "=" )
                    + s.count( "/" ) + s.count( "*" );
  QRegExp delimiter( "[&|\\(\\)\\!\\+\\-\\>\\<\\=\\*/]" );

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

// this method splits only on the first instance of a delimiter, returning a 2-element QString
QStringList automation::splitOnFirst( QString str, const char* delimiter )
{
  QStringList parts = str.split( delimiter, QString::SkipEmptyParts );
  QStringList rest;
  if ( parts.size() > 1 )
  {
    rest = parts;
    rest.removeFirst();    
  }
  QStringList returnStringList;
  returnStringList.append( parts[ 0 ] );
  returnStringList.append( rest.join( delimiter ) );
  
  assert( returnStringList.size() <= 2 );

  return returnStringList;
}

QStringList automation::convert_shorthand( QStringList shorthandList, QString sidebar_text )
{
  // STEP 1:  Take the sidebar list and convert it to an abilities table and an options table

  QStringList sidebarSplit = sidebar_text.split( QRegularExpression( ":::.+:::" ), QString::SkipEmptyParts );

  assert( sidebarSplit.size() == 3 );

  QStringList abilityList = sidebarSplit[ 0 ].split( "\n", QString::SkipEmptyParts );
  QStringList optionsList = sidebarSplit[ 1 ].split( "\n", QString::SkipEmptyParts );
  QStringList operatorsList = sidebarSplit[ 2 ].split( "\n", QString::SkipEmptyParts );

  typedef std::vector<std::pair<QString, QString> > shorthandTable;
  shorthandTable abilityTable;
  shorthandTable optionTable;
  shorthandTable operatorTable;

  // Construct the table for ability conversions
  for ( int i = 0; i < abilityList.size(); i++ )
  {
    QStringList parts = splitOnFirst( abilityList[ i ], "=" );
    if ( parts.size() > 1 )
      abilityTable.push_back( std::make_pair( parts[ 0 ], parts[ 1 ] ) );
  }

  // Construct the table for option conversions
  for ( int i = 0; i < optionsList.size(); i++ )
  {
    QStringList parts = splitOnFirst( optionsList[ i ], "=" );
    if ( parts.size() > 1 )
      optionTable.push_back( std::make_pair( parts[ 0 ], parts[ 1 ] ) );
  }

  // Construct the table for operator conversions
  for ( int i = 0; i < operatorsList.size(); i++ )
  {
    QStringList parts = splitOnFirst( operatorsList[ i ], "=" );
    if ( parts.size() > 1 )
      operatorTable.push_back( std::make_pair( parts[ 0 ], parts[ 1 ] ) );
  }

  // STEP 2: now we take the shorthand and figure out what to do with each element

  QStringList actionPriorityList; // Final APL

  // cycle through the list of shorthands, converting as we go and appending to actionPriorityList
  for ( int i = 0; i < shorthandList.size(); i++ )
  {
    QString ability;
    QString buff;
    QString options;
    QString waitString;

    // skip comments - this should probably never happen though!
    if ( shorthandList[ i ].startsWith( "#" ) )
    {
      actionPriorityList.append( shorthandList[ i ] );
      continue;
    }

    // first, split into ability abbreviation and options set
    QStringList splits = splitOnFirst( shorthandList[ i ], "+" );

    // use abilityTable to replace the abbreviation with the full ability name
    shorthandTable::iterator m = std::find_if( abilityTable.begin(), abilityTable.end(), comp( splits[ 0 ] ) );
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
          if ( optionsBreakdown[ j ].contains( QRegularExpression( "[&|\\(\\)\\!\\+\\-\\>\\<\\=\\*/]+" ) ) )
            continue;

          // otherwise, use regular expressions to determine the syntax and split into components
          QRegularExpressionMatch match;
          QString operand;
          QString operation;
          QString numeric;

          // Checking for operand syntax: Operand.Operator[#] (e.g. DP.BA or DP.BR3)
          QRegularExpression rxb( "(\\D+)(\\.)(\\D+)(\\d*\\.?\\d*)" );
          match = rxb.match( optionsBreakdown[ j ] );
          if ( match.hasMatch() )
          {
            // captures[ 0 ] is the whole string, captures[ 1 ] is the first match, captures[ 2 ] is the second match, etc.
            QStringList captures = match.capturedTexts();

            // split into components
            operand =   captures[ 1 ];
            operation = captures[ 3 ];
            numeric =   captures[ 4 ];
          }
          // if that didn't work, check for simple option syntax: Option[#] (e.g. HP or HP3)
          else
          {
            QRegularExpression rxw( "(\\D+)(\\d*\\.?\\d*)" );
            match = rxw.match( optionsBreakdown[ j ] );
            if ( match.hasMatch() )
            {
              // captures[ 0 ] is the whole string, captures[ 1 ] is the first match, captures[ 2 ] is the second match, etc.
              QStringList captures = match.capturedTexts();

              // split into components
              operation = captures[ 1 ];
              numeric =   captures[ 2 ];
            }
            // if we don't have a match by this point, give up
            else
              continue;
          }

          // now go to work on the components we have

          // first, construct the option from the operation and the numeric
          QString option;
          if ( numeric.length() == 0 )
            option = operation;
          else
            option = operation + "#";

          // pick the table we're using
          shorthandTable* table;
          if ( operand.length() > 0 )
            table = &operatorTable;
          else
            table = &optionTable;

          //now look for that operation in the table
          shorthandTable::iterator n = std::find_if( (*table).begin(), (*table).end(), comp( option ) );
          if ( n != (*table).end() )
            optionsBreakdown[ j ] = n -> second;

          // replace the # sign with the numeric value if necessary
          if ( numeric.length() > 0 )
            optionsBreakdown[ j ].replace( "#", numeric );

          // if we have an operand, look it up in the table and perform the replacement
          if ( operand.length() > 0 )
          {
            shorthandTable::iterator o = std::find_if( abilityTable.begin(), abilityTable.end(), comp( operand ) );
            QString operand_string = operand;
            if ( o != abilityTable.end() )
               operand_string = o -> second;
            if ( optionsBreakdown[ j ].contains( "$operand" ) )
              optionsBreakdown[ j ].replace( "$operand", operand_string );
          }

          // we also need to do some replacements on $ability entries
          if ( optionsBreakdown[ j ].contains( "$ability" ) )
            optionsBreakdown[ j ].replace( "$ability", ability );

          // special handling of the +W option
          if ( optionsBreakdown[ j ].contains( "wait" ) )
          {
            waitString = optionsBreakdown[ j ];
            optionsBreakdown.removeAt( j );
          }

        } // end for loop

        // at this point, we should be able to merge the optionsBreakdown list into a string
        options = optionsBreakdown.join( "" );
      }

      // combine the ability and options into a single string
      QString entry = "actions+=/" + ability;
      if ( options.length() > 0 )
        entry += ",if=" + options;

      // add the entry to actionPriorityList
      actionPriorityList.append( entry );
      if ( waitString.length() > 0 )
        actionPriorityList.append( "actions+=" + waitString );
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

//QComboBox* createInputMode( int count, ... )
//{
//  QComboBox* input_mode = new QComboBox();
//  va_list vap;
//  va_start( vap, count );
//  for ( int i = 0; i < count; i++ )
//    input_mode -> addItem( va_arg( vap, char* ) );
//  va_end( vap );
//  return input_mode;
//}

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

QString defaultOptions = "AC#=action.$ability.charges>=#\nAE#=active_enemies>=#\nDR=target.dot.$ability.remains\nDR#=target.dot.$ability.remains>=#\nE=target.health.pct<=20\nE#=target.health.pct<=#\nNT=!ticking\nNF=target.debuff.flying.down\nW#=/wait,sec=cooldown.$ability.remains,if=cooldown.$ability.remains<=#\n\n";
QString defaultOperators = "BU=buff.$operand.up\nBA=buff.$operand.react\nBR=buff.$operand.remains\nBR#=buff.$operand.remains>#\nBC#=buff.$operand.charges>=#\nCD=cooldown.$operand.remains\nCD#=cooldown.$operand.remains>#\nDR=target.dot.$operand.remains\nDR#=target.dot.$operand.remains>=#\nDT=dot.$operand.ticking\nGCD=cooldown.$operand.remains<=gcd\nGCD#=cooldown.$operand.remains<=gcd*#\nT=talent.$operand.enabled\nG=glyph.$operand.enabled\n";

// constant for sidebar text (this will eventually get really long)
QString sidebarText[ 11 ][ 4 ] = {
  { // DEATHKNIGHT Shorthand Declaration
    ":::Abilities, Buffs, Glyphs, and Talents:::\nAMS=Anti Magic Shell\nAotD=Army of the Dead\nBP=Blood Plague\nCoI=Chains of Ice\nDSim=Dark Simulacrum\nDnD=Death and Decay\nDC=Death Coil\nDG=Death Grip\nDS=Death Strike\nERW=Empower Rune Weapon\nFF=Frost Fever\nHoW=Horn of Winter\nIBF=Icebound Fortitude\nIT=Icy Touch\nMF=Mind Freeze\nOB=Outbreak\nPS=Plague Strike\nSR=Soul Reaper\nPest=Pestilence\nAMZ=Anti Magic Zone\nBT=Blood Tap\nBoS=Breath of Sindragosa\nDP=Death Pact\nDSi=Death Siphon\nGG=Gorefiend's Grasp\nNP=Necrotic Plague\nPL=Plague Leech\nRC=Runic Corruption\nRE=Runic Empowerment\nUB=Unholy Blight\nRW=Remorseless Winter\nDesG=Desecrated Ground\nPATH=Path of Frost\nDef=Defile\n\n"
    "BPres=Blood Presence\nBS=Bone Shield\nDRW=Dancing Rune Weapon\nSoB=Scent of Blood\nVamp=Vampiric Blood\nWotN=Will of the Necropolis\n"
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions +
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",

    ":::Abilities, Buffs, Glyphs, and Talents:::\nAMS=Anti Magic Shell\nAotD=Army of the Dead\nBP=Blood Plague\nCoI=Chains of Ice\nDSim=Dark Simulacrum\nDnD=Death and Decay\nDC=Death Coil\nDG=Death Grip\nDS=Death Strike\nERW=Empower Rune Weapon\nFF=Frost Fever\nHoW=Horn of Winter\nIBF=Icebound Fortitude\nIT=Icy Touch\nMF=Mind Freeze\nOB=Outbreak\nPS=Plague Strike\nSR=Soul Reaper\nPest=Pestilence\nAMZ=Anti Magic Zone\nBT=Blood Tap\nBoS=Breath of Sindragosa\nDP=Death Pact\nDSi=Death Siphon\nGG=Gorefiend's Grasp\nNP=Necrotic Plague\nPL=Plague Leech\nRC=Runic Corruption\nRE=Runic Empowerment\nUB=Unholy Blight\nRW=Remorseless Winter\nDesG=Desecrated Ground\nPATH=Path of Frost\nDef=Defile\n\n"
    "FPres=Frost Presence\nFS=Frost Strike\nHB=Howling Blast\nKM=Killing Machine\nOblit=Obliterate\nPoF=Pillar of Frost\n"
    "Additional ability, buff, glyph, and talent shorthands can be added here"
   "\n\n:::Options:::\n" + defaultOptions + 
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",

    ":::Abilities, Buffs, Glyphs, and Talents:::\nAMS=Anti Magic Shell\nAotD=Army of the Dead\nBP=Blood Plague\nCoI=Chains of Ice\nDSim=Dark Simulacrum\nDnD=Death and Decay\nDC=Death Coil\nDG=Death Grip\nDS=Death Strike\nERW=Empower Rune Weapon\nFF=Frost Fever\nHoW=Horn of Winter\nIBF=Icebound Fortitude\nIT=Icy Touch\nMF=Mind Freeze\nOB=Outbreak\nPS=Plague Strike\nSR=Soul Reaper\nPest=Pestilence\nAMZ=Anti Magic Zone\nBT=Blood Tap\nBoS=Breath of Sindragosa\nDP=Death Pact\nDSi=Death Siphon\nGG=Gorefiend's Grasp\nNP=Necrotic Plague\nPL=Plague Leech\nRC=Runic Corruption\nRE=Runic Empowerment\nUB=Unholy Blight\nRW=Remorseless Winter\nDesG=Desecrated Ground\nPATH=Path of Frost\nDef=Defile\n\n"
    "UPres=Unholy Presence\nDT=Dark Transformation\nFeS=Festering Strike\nSS=Scourge Strike\nSD=Sudden Doom\nGarg=Summon Gargoyle\n"
    "Additional ability, buff, glyph, and talent shorthands can be added here"
   "\n\n:::Options:::\n" + defaultOptions + 
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",
   
    "N/A"
  },

  { // DRUID Shorthand Declaration
    ":::Abilities, Buffs, Glyphs, and Talents:::\n" 
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + 
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",
    
    ":::Abilities, Buffs, Glyphs, and Talents:::\n" 
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + 
    
    ":::Abilities, Buffs, Glyphs, and Talents:::\n" 
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + 
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",
    
    ":::Abilities, Buffs, Glyphs, and Talents:::\n" 
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + 
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",
  },

  { // HUNTER Shorthand Declaration
    ":::Abilities, Buffs, Glyphs, and Talents:::\n" 
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + 
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",
    
    ":::Abilities, Buffs, Glyphs, and Talents:::\n" 
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + 
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",
    
    ":::Abilities, Buffs, Glyphs, and Talents:::\n" 
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + 
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",
   
    "N/A"
  },
  
  { // MAGE Shorthand Declaration
    ":::Abilities, Buffs, Glyphs, and Talents:::\n" 
    "AB=arcane_blast\nAE=arcane_explosion\nAM=arcane_missiles\nABar=arcane_barrage\nCoC=cone_of_cold\nEvo=evocation\nFN = frost_nova\nPoM=presence_of_mind\nCS=counterspell\n\n"
    "BS=blazing_speed\nIF=ice_floes\nNT=nether_tempest\nSN=supernova\nAO=arcane_orb\nRoP=rune_of_power\nIF=incanters_flow\nMI=mirror_image\nPC=prismatic_crystal\nUM=unstable_magic\nOP=overpowered\n"
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + 
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",
    
    ":::Abilities, Buffs, Glyphs, and Talents:::\n" 
    "FB=fireball\nFFB=frostfire_bolt\nSC=scorch\nIB=inferno_blast\nPB=pyroblast\nComb=combustion\nFN=frost_nova\nFS=flamestrike\nDB=dragons_breath\nCS=counterspell\n\n"
    "BS=blazing_speed\nIF=ice_floes\nLB=living_bomb\nBW=blast_wave\nMet=meteor\nHU=heating_up\nHS=pyroblast\nRoP=rune_of_power\nIF=incanters_flow\nMI=mirror_image\nPC=prismatic_crystal\nUM=unstable_magic\nKind=kindling\n"
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + 
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",
    
    ":::Abilities, Buffs, Glyphs, and Talents:::\n" 
    "FB=frostbolt\nIL=ice_lance\nFFB=frostfire_bolt\nCoC=cone_of_cold\nWJ=water_jet\nFrz=freeze\nFN=frost_nova\nFO=frozen_orb\nIV=icy_veins\nBLY=blizzard\nBLZ=blizzard\nCS=counterspell\n\n"
    "BS=blazing_speed\nIF=ice_floes\nFBomb=frost_bomb\nIN=ice_nova\nCmS=comet_storm\nBF=brain_freeze\nFoF=fingers_of_frost\nRoP=rune_of_power\nIF=incanters_flow\nMI=mirror_image\nPC=prismatic_crystal\nUM=unstable_magic\nTV=thermal_void\n"
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + 
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",
   
    "N/A"
  },
  
  { // MONK Shorthand Declaration
    ":::Abilities, Buffs, Glyphs, and Talents:::\n" 
    "KS=keg_smash\nEB=elusive_brew\nEH=expel_harm\nPB=purifying_brew\nFB=fortifying_brew\nBoK=blackout_kick\nTP=tiger_palm\nTPow=buff.tiger_power\nBoF=breath_of_fire\nSCK=spinning_crane_kick\n\n"
    "CW=chi_wave\nZS=zen_sphere\nCBur=chi_burst\nPS=power_strikes\nAsc=ascension\nCB=chi_brew\nDH=dampen_harm\nRJW=rushing_jade_wind\nXuen=invoke_xuen\nCE=chi_explosion\nSer=serenity\n"
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + 
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",
    
    ":::Abilities, Buffs, Glyphs, and Talents:::\n" 
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + 
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",
    
    ":::Abilities, Buffs, Glyphs, and Talents:::\n" 
    "FoF=fists_of_fury\nBoK=blackout_kick\nCBBoK=buff.combo_breaker_bok.react\nTP=tiger_palm\nTPow=buff.tiger_power\nCBTP=buff.combo_breaker_tp.react\nRSK=rising_sun_kick\nTeB=tigereye_brew\nSCK=spinning_crane_kick\n\n"
    "CW=chi_wave\nZS=zen_sphere\nCBur=chi_burst\nPS=power_strikes\nAsc=ascension\nCB=chi_brew\nRJW=rushing_jade_wind\nXuen=invoke_xuen\nHS=hurricane_strike\nCE=chi_explosion\nCBCE=buff.combo_breaker_ce.react\nSer=serenity\n"
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + 
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",
   
    "N/A"
  },

  { // PALADIN Shorthand Declaration
    ":::Abilities, Buffs, Glyphs, and Talents:::\n" 
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + 
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",
    
    ":::Abilities, Buffs, Glyphs, and Talents:::\n"
    "AA=auto_attack\nAS=avengers_shield\nCons=consecration\nCS=crusader_strike\nEF=eternal_flame\nES=execution_sentence\nHotR=hammer_of_the_righteous\nHoW=hammer_of_wrath\nHPr=holy_prism\nHW=holy_wrath\nJ=judgment\nLH=lights_hammer\nSS=sacred_shield\nSoI=seal_of_insight\nSoR=seal_of_righteousness\nSoT=seal_of_truth\nSP=seraphim\nSotR=shield_of_the_righteous\nWoG=word_of_glory\n\n"
    "DJ=double_jeopardy\nDP=divine_purpose\nFW=holy_wrath,if=glyph.final_wrath.enabled&target.health.pct<=20\nGC=grand_crusader\nHA=holy_avenger\nSP=seraphim\nSotR=shield_of_the_righteous\nSW=sanctified_wrath\n\n"
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + "HP=holy_power\nHP#=holy_power>=#\nFW=glyph.final_wrath.enabled&target.health.pct<=20\nEverything below this line is redundant with the buff syntax method, just here for ease of use\nDP=buff.divine_purpose.react\nSW=talent.sanctified_wrath.enabled\nSP=buff.seraphim.react\n\nGC=buff.grand_crusader.react\nGC#=buff.grand_crusader.remains<#\n"
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",

    ":::Abilities, Buffs, Glyphs, and Talents:::\n"
    "AA=auto_attack\nCS=crusader_strike\nJ=judgment\nEXO=exorcism\nHotR=hammer_of_the_righteous\nHoW=hammer_of_wrath\nTV=templars_verdict\nFV=final_verdict\nDS=divine_storm\nES=execution_sentence\nHPr=holy_prism\nLH=lights_hammer\nSP=seraphim\nSoT=seal_of_truth\nSoR=seal_of_righteousness\nSoI=seal_of_insight\nAW=avenging_wrath\nHA=holy_avenger\n\n"
    "AW=avenging_wrath\nDP=divine_purpose\nHA=holy_avenger\nDC=divine_crusader\nSP=seraphim\nSW=sanctified_wrath\nDJ=double_jeopardy\n\n"
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + "HP#=holy_power>=#\nHPL#=holy_power<=#\n"
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",
    
    "N/A"
  },

  { // PRIEST Shorthand Declaration
    ":::Abilities, Buffs, Glyphs, and Talents:::\n" 
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + 
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",
    
    ":::Abilities, Buffs, Glyphs, and Talents:::\n" 
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + 
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",
    
    ":::Abilities, Buffs, Glyphs, and Talents:::\n" 
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + 
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",
   
    "N/A"
  },

  { // ROGUE Shorthand Declaration
    ":::Abilities, Buffs, Glyphs, and Talents:::\n" 
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + 
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",
    
    ":::Abilities, Buffs, Glyphs, and Talents:::\n" 
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + 
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",
    
    ":::Abilities, Buffs, Glyphs, and Talents:::\n" 
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + 
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",
   
    "N/A"
  },

  { // SHAMAN Shorthand Declaration
    ":::Abilities, Buffs, Glyphs, and Talents:::\n" 
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + 
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",
    
    ":::Abilities, Buffs, Glyphs, and Talents:::\n" 
    "AA=auto_attack\nEM=elemental_mastery\nSET=storm_elemental_totem\nFET=fire_elemental_totem\nASC=ascendance\nFS=feral_spirit\nLM=liquid_magma\nAS=ancestral_swiftness\nST=searing_totem\nUE=unleash_elements\nEB=elemental_blast\nLB=lightning_bolt\nWS=windstrike\nSS=stormstrike\nLL=lava_lash\nFlS=flame_shock\nFrS=frost_shock\n\n"
    "EM=elemental_mastery\nAS=ancestral_swiftness\nUF=unleashed_fury\nPE=primal_elementalist\nEB=elemental_blast\nEF=elemental_fusion\nSET=storm_elemental_totem\nLM=liquid_magma\n"
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + 
    "MW#=buff.maelstrom_weapon.react>=#\nNFT=!totem.fire.active\nNASC=!buff.ascendance.up\nCFE#=cooldown.fire_elemental_totem.remains>=#\nFTR#=pet.searing_totem.remains>=#|pet.fire_elemental_totem.remains>=#\nUFl=buff.unleash_flame.up\nFlSR#=dot.flame_shock.remains<=#\nASU=buff.ancestral_swiftness.up\n"    
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",
    
    ":::Abilities, Buffs, Glyphs, and Talents:::\n" 
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + 
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",
   
    "N/A"
  },

  { // WARLOCK Shorthand Declaration
    ":::Abilities, Buffs, Glyphs, and Talents:::\n" 
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + 
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",
    
    ":::Abilities, Buffs, Glyphs, and Talents:::\n" 
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + 
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",
    
    ":::Abilities, Buffs, Glyphs, and Talents:::\n" 
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + 
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",
   
    "N/A"
  },

  { // WARRIOR Shorthand Declaration
    ":::Abilities, Buffs, Glyphs, and Talents:::\n" 
    ""
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + 
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",
    
    ":::Abilities, Buffs, Glyphs, and Talents:::\n" 
    "AA=auto_attack\nBT=bloodthirst\nCS=colossus_smash\nWS=wild_strike\nRB=raging_blow\nDBTS=die_by_the_sword\nCharge=charge\n\n"
    "avatar=avatar\nBB=bloodbath\nSB=storm_bolt\nsbreak=siegebreaker\nDR=dragon_roar\nBS=bladestorm\nRavager=ravager\nUQT=talent.unquenchable_thirst.enabled\nFS=talent.furious_strikes.enabled\nSD=buff.sudden_death\nbloodsurge=buff.bloodsurge.react\nmc=buff.meat_cleaver.stack\n"
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + 
    "CS = debuff.colossus_smash.up\nenrage = buff.enrage.up\nenrage<# = buff.enrage.remains<#\nenrage># = buff.enrage.remains>#\nRB# = buff.raging_blow.stack=#\n"
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",
    
    ":::Abilities, Buffs, Glyphs, and Talents:::\n" 
    "AA=auto_attack\nSS=shield_slam\nR=revenge\nD=devastate\nHS=heroic_strike\nTC=thunder_clap\nDR=dragon_roar\nSB=storm_bolt\nBS=bladestorm\nBB=bloodbath\nSD=sudden_death\nHL=heroic_leap\nExecute=execute\nRavager=ravager\nShockwave=shockwave\nAvatar=avatar\nCharge=charge\nSBk=shield_block\nSBr=shield_barrier\nSW=shield_wall\nLS=last_stand\nDS=demoralizing_shout\nER=enraged_regeneration\nIV=impending_victory\nRP=resonating_power\nUR=unending_rage\nDAP=potion,name=draenic_armor\n\n"
    "Additional ability, buff, glyph, and talent shorthands can be added here"
    "\n\n:::Options:::\n" + defaultOptions + 
    "mCD=(buff.shield_block.up|buff.shield_wall.up|buff.last_stand.up|debuff.demoralizing_shout.up|buff.ravager.up|buff.draenic_armor_potion.up|buff.enraged_regeneration.up)\nsCD=(buff.shield_wall.up|buff.last_stand.up|debuff.demoralizing_shout.up)|(buff.shield_barrier.value>health.max*0.25)\nsCD#=(buff.shield_wall.up|buff.last_stand.up|debuff.demoralizing_shout.up)|(buff.shield_barrier.value>health.max*#*0.01)\nUR=(buff.ultimatum.up|(talent.unyielding_strikes.enabled&buff.unyielding_strikes.max_stack))\nUR#=(buff.ultimatum.up|(talent.unyielding_strikes.enabled&buff.unyielding_strikes.stack>=#))\nrage#=rage>=#\nSBr#=(buff.shield_barrier.value>health.max*#*0.01)\n\n"
    "Additional option shorthands can be added here"
    "\n\n:::Operators:::\n" + defaultOperators +
    "Additional operator shorthands can be added here\n\n",
   
    "N/A"
  }
};

// constant for the varying labels of the advanced text box
QString advancedText[ 6 ] = {
  "Unused", "Talent Configurations", "Glyph Configurations", "Gear Configurations", "Rotation Configurations", "Advanced Configuration Mode"
};
// constant for the varying labels of the helpbar text box
QString helpbarText[ 6 ] = {
  "To automate generation of a comparison, choose a comparison type from the drop-down box to the left.",
  "List the talent configurations you want to test in the box below as seven-digit integer strings, i.e. 1231231.\nEach configuration should be its own new line.\nFor more advanced syntax options, see the wiki entry at https://code.google.com/p/simulationcraft/wiki/Automation#Talent_Comparisons.",
  "List the glyph configurations you want to test (i.e. alabaster_shield/focused_shield/word_of_glory) in the box below.\nEach configuration should be its own new line.\nFor more advanced syntax options, see the wiki entry at https://code.google.com/p/simulationcraft/wiki/Automation#Glyph_Comparisons.",
  "List the different gear configurations you want to test in the box below.\nEach configuration should be separated by a blank line.\nFor more advanced syntax options, see the wiki entry at https://code.google.com/p/simulationcraft/wiki/Automation#Gear_Comparisons.",
  "List the rotations you want to test in the box below. Rotations can be specified as usual for simc (with different configurations separated by a blank line) or as shorthands (each on a new line).\nThe sidebar lists the shorthand conventions for your class and spec. You can add your own using the syntax <Shorthand>=<Longhand> (see examples).\nFor more advanced syntax options, see the wiki entry at https://code.google.com/p/simulationcraft/wiki/Automation#Rotation_Comparisons.",
  "To specify everything according to your needs. All configurations can be mixed up for more advanced multi-automated simulations but need to match the standard SimCraft input format"
};

QString advTalentToolTip = "Talent configurations can be specified as 7-digit numbers (e.g. 1231231).\n"
                           "Each configuration should be on its own new line.\n"
                           "Additional options can be added afterward, separated by a space, as in the example below:\n\n"
                           "1231231 name=Alic\n1231232 name=Bob\n1231233 name=Carl glyphs+=/focused_shield";

QString advGlyphToolTip = "Glyph configurations can be specified exactly the same way they are in SimC normally.\n"
                           "Each configuration should be on its own new line.\n"
                           "Additional options can be added afterward, separated by a space, as in the example below:\n\n"
                           "focused_shield/alabaster_shield name=Alic\nfocused_shield/final_wrath name=Bob\nfinal_wrath/alabaster_shield name=Carl talents=1231233";

QString advGearToolTip = "Gear sets can be specified just like they are in SimC normally.\nSeparate each gear set by a blank line (two carriage returns).\n\n"
                         "Example:\nhead=fake_helm,id=1111\nneck=fake_neck,id=2222\n\nhead=another_fake_helm,id=3333\nneck=another_fake_neck,id=4444";

QString advRotToolTip = "Rotations can be specified in two ways:\n(1): by shorthands separated by single carriage returns, like this:\n\n"
                        "CS>J>AS\nCS>AS>J\nJ>AS>CS\n\n"
                        "(2): as usual simc action lists, separated by two carriage returns, like this:\n\n"
                        "actions=/crusader_strike\nactions+=/judgment\nactions+=/avengers_shield\n\nactions=/crusader_strike\nactions+=/avengers_shield\nactions+=/judgment\n\nactions=/judgment\nactions+=/avengers_shield\nactions+=/crusader_strike\n\n"
                        "Note that the two examples above are equivalent.\n"
                        "For shorthand inputs, options can be specified using usual simc syntax, with \"+\" standing for \",if=\", like this:\n\n"
                        "CS+GC&(DP|!FW)\n\n"
                        "See the wiki entry for more details. https://code.google.com/p/simulationcraft/wiki/Automation#Rotation_Comparisons";

QString defaultRotationHeaderToolTip = "Default rotation, specified the same way you would in a regular simc profile.\nIf left blank, the default APL for the chosen spec will be used automatically.";
QString defaultRotationFooterToolTip = "This box is only used for Rotation comparisons.";

// method to set the sidebar text based on class slection
void SC_ImportTab::setSidebarClassText()
{
  textbox.sidebar -> setText( sidebarText[ choice.player_class -> currentIndex() ][ choice.player_spec -> currentIndex() ] );
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
  else if ( textbox.rotationFooter -> isEnabled() )
    advRotation = textbox.advanced -> document() -> toPlainText();

  // set the label of the Advanced tab appropriately
  label.advanced -> setText( advancedText[ comp ] );

  // set the text of the help bar appropriately
  textbox.helpbar -> setText( helpbarText[ comp ] );
  
  // enable everything but the sidebar & rotation Footer (default state) - adjust based on selection below
  textbox.advanced -> setDisabled( false );
  textbox.gear     -> setDisabled( false );
  textbox.glyphs   -> setDisabled( false );
  textbox.rotationHeader -> setDisabled( false );
  textbox.talents  -> setDisabled( false );
  textbox.sidebar  -> setDisabled( true  );
  textbox.rotationFooter -> setDisabled( true );

  // clear certain tooltips
  textbox.rotationHeader -> setToolTip( defaultRotationHeaderToolTip );
  label.rotationHeader -> setToolTip( defaultRotationHeaderToolTip );
  textbox.rotationFooter -> setToolTip( defaultRotationFooterToolTip );
  label.rotationFooter -> setToolTip( defaultRotationFooterToolTip );

  // Set rotation header/footer texts
  label.rotationHeader -> setText( "Default Rotation" );
  label.rotationFooter -> setText( "Unused" );

  switch ( comp )
  {
    case 0: 
      textbox.advanced -> setDisabled( true );
      textbox.advanced -> setToolTip( "Choose a comparison type to enable this text box." );
      label.advanced -> setToolTip( "Choose a comparison type to enable this text box." );
      break;
    case 1:
      textbox.advanced -> setText( advTalent );
      textbox.talents -> setDisabled( true );
      textbox.advanced -> setToolTip( advTalentToolTip );
      label.advanced   -> setToolTip( advTalentToolTip );
      break;
    case 2:
      textbox.glyphs -> setDisabled( true );
      textbox.advanced -> setText( advGlyph );
      textbox.advanced -> setToolTip( advGlyphToolTip );
      label.advanced   -> setToolTip( advGlyphToolTip );
      break;
    case 3:
      textbox.gear -> setDisabled( true );
      textbox.advanced -> setText( advGear );
      textbox.advanced -> setToolTip( advGearToolTip );
      label.advanced   -> setToolTip( advGearToolTip );
      break;
    case 4:
      //textbox.rotationHeader -> setDisabled( true );
      label.rotationHeader -> setText( "Actions Header" );
      label.rotationHeader -> setToolTip( "Use this box to specify precombat actions and any actions you want to apply to all configurations (ex: auto_attack).\nThe text in this box will be placed BEFORE each entry in the Rotation Configurations text box." );
      textbox.rotationHeader -> setToolTip( "Use this box to specify precombat actions and any actions you want to apply to all configurations (ex: auto_attack)\nThe text in this box will be placed BEFORE each entry in the Rotation Configurations text box.." );
      label.rotationFooter -> setText( "Actions Footer" );
      label.rotationFooter -> setToolTip( "Use this box to specify any actions you want to apply to all configurations.\nThe text in this box will be placed AFTER each entry in the Rotation Configurations text box." );
      textbox.rotationFooter -> setToolTip( "Use this box to specify any actions you want to apply to all configurations.\nThe text in this box will be placed AFTER each entry in the Rotation Configurations text box." );
      textbox.advanced -> setText( advRotation );
      textbox.sidebar -> setDisabled( false );
      textbox.rotationFooter -> setDisabled( false );
      textbox.advanced -> setToolTip( advRotToolTip );
      label.advanced   -> setToolTip( advRotToolTip );
      break;
  }
}

void SC_MainWindow::startAutomationImport( int tab )
{
  QString profile;

  profile = automation::automation_main( importTab -> choice.comp_type -> currentIndex(),
                                      importTab -> choice.player_class -> currentText(),
                                      importTab -> choice.player_spec -> currentText(),
                                      importTab -> choice.player_race -> currentText(),
                                      importTab -> choice.player_level -> currentText(),
                                      importTab -> textbox.talents -> text(),
                                      importTab -> textbox.glyphs -> text(),
                                      importTab -> textbox.gear -> document() -> toPlainText(),
                                      importTab -> textbox.rotationHeader -> document() -> toPlainText(),
                                      importTab -> textbox.rotationFooter -> document() -> toPlainText(),
                                      importTab -> textbox.advanced -> document() -> toPlainText(),
                                      importTab -> textbox.sidebar -> document() -> toPlainText(),
                                      importTab -> textbox.footer -> document() -> toPlainText()
                                    );

  simulateTab -> add_Text( profile,  tr( "Automation Import" ) );
  
  mainTab -> setCurrentTab( TAB_SIMULATE );
}

void SC_ImportTab::createTooltips()
{
  choice.comp_type -> setToolTip( "Choose the comparison type." );

  QString sidebarTooltip = "This box specifies the abbreviations used in rotation shorthands. It is divided into three sections.\n\n"
                           "The :::Abilities, Buffs, Glyphs, and Talents::: section defines shorthands for abilities, buffs, glyphs, and talents.\n"
                           "Example, \"HoW=hammer_of_wrath\", \"DP=divine_purpose\", \"NT=nether_tempest\", etc.\n\n"
                           "The :::Options::: section defines conditionals you want to apply to that action. For example, \"E=target.health.pct<20\" is an option you can use to specify execute range.\n"
                           "Thus, the shorthand \"HoW+E\" would convert to \"hammer_of_wrath,if=target.health.pc<20\".\n"
                           "For options, the pound sign \"#\" can be used to support a numeric argument and \"$ability\" will automatically be replaced with the name of the ability to which the option is being applied.\n"
                           "Example: The definition \"AD#=active_dot.$ability=#\" causes \"NT+AD0\" to translate to \"nether_tempest,if=active_dot.nether_tempest=0\".\n"
                           "The \"W#\" option is special since it adds an extra APL entry; W# should be the only option or should be separated from other options like so: (DP&!FW)W3\n\n"
                           "The :::Operators::: section defines function that act on another abbreviation from the first section, using a period as an operator.\n"
                           "The pound sign works as in the operators section, and \"$operand\" will be replaced with the abbreviation being operated upon.\n"
                           "Example: since \"BU=buff.$operand.up\", \"HoW+DP.BU\" would translate to \"hammer_of_wrath,if=buff.divine_purpose.up\"\n"
                           "You can string these together with logical operations, e.g. \"HoW+DP.BU&HP3\" to create \"hammer_of_wrath,if=buff.divine_purpose.up&holy_power>=3\".\n\n"
                           "You may define custom abbreviations by adding your own entries to the appropriate section.\n"
                           "See the wiki entry for more details. https://code.google.com/p/simulationcraft/wiki/Automation#Rotation_Comparisons";
  textbox.sidebar -> setToolTip( sidebarTooltip );
  label.sidebar   -> setToolTip( sidebarTooltip );

  QString footerTooltip = "The text in this box is added after all of the generated profiles, only once. It can be used to set global options or define custom bosses.";
  textbox.footer -> setToolTip( footerTooltip );
  label.footer -> setToolTip( footerTooltip );

  textbox.talents -> setToolTip( "Default talents, specified either as \"talents=1231231\" or \"1231231\"" );
  textbox.glyphs -> setToolTip( "Default glyphs, specified either as \"glyphs=glyph1/glyph2/glyph3\" or \"glyph1/glyph2/glyph3\"" );
  textbox.gear -> setToolTip( "Default gear, specified the same way you would in a regular simc profile." );
  textbox.rotationHeader -> setToolTip( "Default rotation, specified the same way you would in a regular simc profile.\nIf left blank, the default APL for the chosen spec will be used automatically." );
  textbox.rotationFooter-> setToolTip( "This box is only used for Rotation comparisons." );
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

  
  // Elements (3,0) - (8,0) are the default gear and rotation labels and text boxes
  
  // Create a label and an edit box for gear
  label.gear = new QLabel( tr( "Default Gear" ) );
  textbox.gear = new SC_TextEdit;
  // assign the label and edit box to cells
  gridLayout -> addWidget( label.gear,   3, 0, 0 );
  gridLayout -> addWidget( textbox.gear, 4, 0, 0 );
  
  // and again for rotation Header
  label.rotationHeader = new QLabel( tr( "Default Rotation" ) );
  textbox.rotationHeader = new SC_TextEdit;
  gridLayout -> addWidget( label.rotationHeader,   5, 0, 0 );
  gridLayout -> addWidget( textbox.rotationHeader, 6, 0, 0 );

  // and again for rotation Footer
  label.rotationFooter = new QLabel( tr( "Unused" ) );
  textbox.rotationFooter = new SC_TextEdit;
  gridLayout -> addWidget( label.rotationFooter,   7, 0, 0 );
  gridLayout -> addWidget( textbox.rotationFooter, 8, 0, 0 );

  // Elements (2,1) - (6,1) are the Advanced text box, which is actually
  // N different text boxes that get cycled depending on sim type choice

  label.advanced = new QLabel( tr( "Advanced Text Box" ) );
  textbox.advanced = new SC_TextEdit;
  gridLayout -> addWidget( label.advanced,   1, 1, 0 );
  gridLayout -> addWidget( textbox.advanced, 2, 1, 5, 1, 0 );

  // Element (8,1) is the footer box

  label.footer = new QLabel( tr( "Footer" ) );
  textbox.footer = new SC_TextEdit;
  gridLayout -> addWidget( label.footer,   7, 1, 0 );
  gridLayout -> addWidget( textbox.footer, 8, 1, 0 );

  // Eleements (2,2) - (8,2) are the Rotation Conversions text box 

  label.sidebar = new QLabel( tr( "Rotation Abbreviations" ) );
  textbox.sidebar = new SC_TextEdit;
  textbox.sidebar -> setText( " Stuff Goes Here" );
  gridLayout -> addWidget( label.sidebar,   1, 2, 0 );
  gridLayout -> addWidget( textbox.sidebar, 2, 2, 7, 1, 0 );
  
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

  // do some initialization
  compTypeChanged( choice.comp_type -> currentIndex() );

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
  settings.setValue( "talentbox", textbox.talents -> text() );
  settings.setValue( "glyphbox", textbox.glyphs -> text() );
  settings.setValue( "gearbox", textbox.gear -> document() -> toPlainText() );
  settings.setValue( "rotationFooterBox", textbox.rotationFooter -> document() -> toPlainText() );
  settings.setValue( "rotationHeaderBox", textbox.rotationHeader -> document() -> toPlainText() );
  settings.setValue( "advancedbox", textbox.advanced -> document() -> toPlainText() );
  settings.setValue( "advTalent", advTalent );
  settings.setValue( "advGlyph", advGlyph );
  settings.setValue( "advGear", advGear );
  settings.setValue( "advRotation", advRotation );
  settings.setValue( "sidebox", textbox.sidebar -> document() -> toPlainText() );
  settings.setValue( "footer", textbox.footer -> document() -> toPlainText() );
  
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
  load_setting( settings, "gearbox", textbox.gear, "fake_helm,stats=100sta\nneck=malachite_pendant,id=25438\n" );
  load_setting( settings, "rotationFooterBox", textbox.rotationFooter );
  load_setting( settings, "rotationHeaderBox", textbox.rotationHeader );
  load_setting( settings, "advancedbox", textbox.advanced );
  load_setting( settings, "advTalent", &advTalent, "0000000\n1111111\n2222222" );
  load_setting( settings, "advGlyph", &advGlyph, "alabaster_shield/focused_shield\nfinal_wrath/word_of_glory" );
  load_setting( settings, "advGear", &advGear, "head=fake_helm,stats=100sta\nneck=malachite_pendant,id=25438\n\nhead=different_helm,stats=100str\nneck=thick_bronze_necklace,id=21933\n" );
  load_setting( settings, "advRotation", &advRotation, "CS>J>AS\nCS>J>AS+GC\nCS>AS+GC&(DP|!FW)>J+SW>J" );
  load_setting( settings, "sidebox", textbox.sidebar );
  load_setting( settings, "footer", textbox.footer );

  settings.endGroup();
}

// random test shorthand (ret paladin)
// AA>AW>HA+HPL2>ES>LH>DS+T2&(HP5|DP|(HAB&HP3))>DS+DC&HP5>FV+HP5>FV+DP&DP4>HoW+W0.2>DS+DC>FV+AWB>HotR+T4>CS+W0.2>J+W0.2>DS+DC>FV+DP>EXO+W0.2>DS+T2>FV