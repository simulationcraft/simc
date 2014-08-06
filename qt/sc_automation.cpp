// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
/*
  This file contains all of the methods required for automated comparison sims
*/
#include "simulationcraft.hpp"
#include "simulationcraftqt.hpp"


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
