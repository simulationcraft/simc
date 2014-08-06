// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
/*
  This file contains all of the methods required for automated comparison sims
*/
#include "simulationcraft.hpp"


  // stuff goes here!

  std::string automation::does_something( const std::string& player_class,
                                          const std::string& player_spec,
                                          const std::string& player_race,
                                          const std::string& player_level,
                                          const std::string& player_talents,
                                          const std::string& player_glyphs,
                                          const std::string& player_gear,
                                          const std::string& player_rotation 
                                        ) 
  {
    // Dummy code - this just sets up a profile using the strings we pass. 
    // This is where we'll actually split off into different automation schemes.
    // For some reason, I was only able to pass "const std::string&" from SC_MainWindow::startAutomationImport (in sc_window.cpp), 
    // hence the silly temp crap. 
    
    std::string profile;
    std::string temp;
    temp = player_class;
    util::tokenize( temp );
    profile += temp + "=Name\n";

    temp = player_spec;
    util::tokenize( temp );
    profile += "specialization=" + temp + "\n";
      
    temp = player_race;
    util::tokenize( temp );
    profile += "race=" + temp + "\n";

    profile += "level=" + player_level + "\n";
    profile += "talents=" + player_talents + "\n";
    profile += "glyphs=" + player_glyphs + "\n";
    
    // gear
    // rotation

    return profile;
  
  }
