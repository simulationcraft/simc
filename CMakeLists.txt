CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(simulationcraft CXX)

FIND_PACKAGE(Qt4 COMPONENTS QtCore QtGui QtWebKit QtMain QtNetwork)

SET(EXECUTABLE_OUTPUT_PATH bin)
SET(LIBRARY_OUTPUT_PATH lib)

############################
# MAIN SIMULATION ENGINE -->

SET(SIMC_ENGINE_INCLUDE_DIRS
  "engine"
)

SET(SIMC_ENGINE_SOURCE
  "engine/sc_action.cpp"
  "engine/sc_armory.cpp"
  "engine/sc_attack.cpp"
  "engine/sc_battle_net.cpp"
  "engine/sc_bcp_api.cpp"
  "engine/sc_buff.cpp"
  "engine/sc_chardev.cpp"
  "engine/sc_chart.cpp"
  "engine/sc_const_data.cpp"
  "engine/sc_consumable.cpp"
  "engine/sc_data.cpp"
  "engine/sc_death_knight.cpp"
  "engine/sc_dot.cpp"
  "engine/sc_druid.cpp"
  "engine/sc_enchant.cpp"
  "engine/sc_event.cpp"
  "engine/sc_expressions.cpp"
  "engine/sc_gear_stats.cpp"
  "engine/sc_heal.cpp"
  "engine/sc_http.cpp"
  "engine/sc_hunter.cpp"
  "engine/sc_item.cpp"
  "engine/sc_item_data.cpp"
  "engine/sc_js.cpp"
  "engine/sc_log.cpp"
  "engine/sc_mage.cpp"
  "engine/sc_mmo_champion.cpp"
  "engine/sc_option.cpp"
  "engine/sc_paladin.cpp"
  "engine/sc_pet.cpp"
  "engine/sc_player.cpp"
  "engine/sc_plot.cpp"
  "engine/sc_priest.cpp"
  "engine/sc_raid_event.cpp"
  "engine/sc_rating.cpp"
  "engine/sc_reforge_plot.cpp"
  "engine/sc_report.cpp"
  "engine/sc_report_html_player.cpp"
  "engine/sc_report_html_sim.cpp"
  "engine/sc_report_text.cpp"
  "engine/sc_report_xml.cpp"
  "engine/sc_rng.cpp"
  "engine/sc_rogue.cpp"
  "engine/sc_sample_data.cpp"
  "engine/sc_scaling.cpp"
  "engine/sc_sequence.cpp"
  "engine/sc_set_bonus.cpp"
  "engine/sc_shaman.cpp"
  "engine/sc_sim.cpp"
  "engine/sc_spell.cpp"
  "engine/sc_spell_data.cpp"
  "engine/sc_spell_info.cpp"
  "engine/sc_stats.cpp"
  "engine/sc_talent.cpp"
  "engine/sc_target.cpp"
  "engine/sc_thread.cpp"
  "engine/sc_unique_gear.cpp"
  "engine/sc_util.cpp"
  "engine/sc_warlock.cpp"
  "engine/sc_warrior.cpp"
  "engine/sc_weapon.cpp"
  "engine/sc_wowhead.cpp"
  "engine/sc_xml.cpp"
  "engine/sc_rawr.cpp"
  "engine/sc_weapon.cpp"
)

ADD_LIBRARY(simc_engine STATIC ${SIMC_ENGINE_SOURCE})

# <-- MAIN SIMULATION ENGINE
############################

###########################
# CONSOLE CONFIGURATION -->

SET(SIMC_CONSOLE_SOURCE
  "engine/sc_main.cpp"
)
  
ADD_EXECUTABLE(simc-con ${SIMC_CONSOLE_SOURCE})

INCLUDE_DIRECTORIES(${SIMC_ENGINE_INCLUDE_DIRS})

TARGET_LINK_LIBRARIES(simc-con simc_engine)
IF(WIN32)
  TARGET_LINK_LIBRARIES(simc-con wininet)
ENDIF(WIN32)

# <-- CONSOLE CONFIGURATION
###########################

#######################
# QT4 CONFIGURATION -->

IF(QT4_FOUND)

  SET(SIMC_QT_SOURCE
    "qt/main.cpp"
    "qt/sc_window.cpp"
  )
  
  SET(SIMC_QT_HEADERS
    "qt/simulationcraftqt.h"
  )

  INCLUDE(${QT_USE_FILE})
  
  QT4_WRAP_CPP(SIMC_QT_HEADERS_MOC ${SIMC_QT_HEADERS})
  
  ADD_DEFINITIONS(${QT_DEFINITIONS})
  
  IF(WIN32)
    ADD_EXECUTABLE(simc-qt WIN32 ${SIMC_QT_SOURCE} ${SIMC_QT_HEADERS_MOC})
  ELSE(WIN32)
	ADD_EXECUTABLE(simc-qt ${SIMC_QT_SOURCE} ${SIMC_QT_HEADERS_MOC})
  ENDIF(WIN32)

  INCLUDE_DIRECTORIES(${SIMC_ENGINE_INCLUDE_DIRS})

  TARGET_LINK_LIBRARIES(simc-qt simc_engine)
  IF(WIN32)
    TARGET_LINK_LIBRARIES(simc-qt wininet)
  ENDIF(WIN32)
  TARGET_LINK_LIBRARIES(simc-qt ${QT_LIBRARIES})
ENDIF()
# <-- QT4 CONFIGURATION
#######################