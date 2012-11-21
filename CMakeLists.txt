CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(simulationcraft)

SET(CPACK_PACKAGE_VERSION "422.5")
SET(CPACK_PACKAGE_VERSION_MAJOR "422")
SET(CPACK_PACKAGE_VERSION_MINOR "5")
SET(CPACK_PACKAGE_VERSION_PATCH "0")

FIND_PACKAGE(Qt4 COMPONENTS QtCore QtGui QtWebKit QtMain QtNetwork)
FIND_PACKAGE(Threads REQUIRED)
FIND_PACKAGE(OpenSSL)

IF(APPLE)
    FIND_LIBRARY(COCOA_LIBRARY Cocoa)
    MARK_AS_ADVANCED(COCOA_LIBRARY)
    SET(COCOA_LIBS ${COCOA_LIBRARY})
ENDIF(APPLE)

SET(EXECUTABLE_OUTPUT_PATH bin)
SET(LIBRARY_OUTPUT_PATH bin)

############################
# MAIN SIMULATION ENGINE -->

SET(SIMC_ENGINE_INCLUDE_DIRS
  "engine"
) 

SET(SIMC_ENGINE_SOURCE
  "engine/sc_action.cpp"
  "engine/sc_action_state.cpp"
  "engine/sc_attack.cpp"
  "engine/interfaces/sc_bcp_api.cpp"
  "engine/sc_buff.cpp"
  "engine/interfaces/sc_chardev.cpp"
  "engine/report/sc_chart.cpp"
  "engine/dbc/sc_const_data.cpp"
  "engine/sc_consumable.cpp"
  "engine/sc_cooldown.cpp"
  "engine/dbc/sc_data.cpp"
  "engine/sc_dot.cpp"
  "engine/sc_enchant.cpp"
  "engine/sc_event.cpp"
  "engine/sc_expressions.cpp"
  "engine/sc_gear_stats.cpp"
  "engine/sc_heal.cpp"
  "engine/interfaces/sc_http.cpp"
  "engine/sc_item.cpp"
  "engine/dbc/sc_item_data.cpp"
  "engine/interfaces/sc_js.cpp"
  "engine/interfaces/sc_mmo_champion.cpp"
  "engine/sc_option.cpp"
  "engine/sc_pet.cpp"
  "engine/sc_player.cpp"
  "engine/sc_plot.cpp"
  "engine/sc_raid_event.cpp"
  "engine/sc_rating.cpp"
  "engine/sc_reforge_plot.cpp"
  "engine/report/sc_report.cpp"
  "engine/report/sc_report_html_player.cpp"
  "engine/report/sc_report_html_sim.cpp"
  "engine/report/sc_report_text.cpp"
  "engine/report/sc_report_xml.cpp"
  "engine/sc_rng.cpp"
  "engine/sc_scaling.cpp"
  "engine/sc_sequence.cpp"
  "engine/sc_set_bonus.cpp"
  "engine/sc_sim.cpp"
  "engine/sc_spell_base.cpp"
  "engine/sc_harmful_spell.cpp"
  "engine/sc_absorb.cpp"
  "engine/dbc/sc_spell_data.cpp"
  "engine/dbc/sc_spell_info.cpp"
  "engine/sc_stats.cpp"
  "engine/sc_thread.cpp"
  "engine/sc_unique_gear.cpp"
  "engine/sc_util.cpp"
  "engine/sc_weapon.cpp"
  "engine/interfaces/sc_wowhead.cpp"
  "engine/interfaces/sc_xml.cpp"
  "engine/interfaces/sc_rawr.cpp"
  "engine/sc_weapon.cpp"
  "engine/class_modules/sc_death_knight.cpp"
  "engine/class_modules/sc_druid.cpp"
  "engine/class_modules/sc_enemy.cpp"
  "engine/class_modules/sc_hunter.cpp"
  "engine/class_modules/sc_monk.cpp"
  "engine/class_modules/sc_mage.cpp"
  "engine/class_modules/sc_paladin.cpp"
  "engine/class_modules/sc_priest.cpp"
  "engine/class_modules/sc_rogue.cpp"
  "engine/class_modules/sc_shaman.cpp"
  "engine/class_modules/sc_warlock.cpp"
  "engine/class_modules/sc_warrior.cpp"
)

SET(SIMC_ENGINE_HEADERS
  "engine/simulationcraft.hpp"
  "engine/sc_generic.hpp"
  "engine/sc_timespan.hpp"
  "engine/sc_rng.hpp"
  "engine/dbc/specialization.hpp"
  "engine/dbc/dbc.hpp"
  "engine/utf8.h"
  "engine/utf8/checked.h"
  "engine/utf8/core.h"
  "engine/utf8/unchecked.h"
  "engine/report/sc_report.hpp"
  "engine/sc_sample_data.hpp"
)

IF(OPENSSL_FOUND)
  ADD_DEFINITIONS(-DUSE_OPENSSL)
  INCLUDE_DIRECTORIES(${OPENSSL_INCLUDE_DIR})
ENDIF(OPENSSL_FOUND)

ADD_LIBRARY(simc_engine STATIC ${SIMC_ENGINE_SOURCE} ${SIMC_ENGINE_HEADERS})

# <-- MAIN SIMULATION ENGINE
############################

###########################
# CONSOLE CONFIGURATION -->

SET(SIMC_CONSOLE_SOURCE
  "engine/sc_main.cpp"
)

SET(SIMC_CONSOLE_HEADERS
  ${SIMC_ENGINE_HEADERS}
)

# OpenSSL configuration ->
IF(OPENSSL_FOUND)
  ADD_DEFINITIONS(-DUSE_OPENSSL)
  INCLUDE_DIRECTORIES(${OPENSSL_INCLUDE_DIR})
ENDIF(OPENSSL_FOUND)
# <- OpenSSL configuration

ADD_EXECUTABLE(simc-con ${SIMC_CONSOLE_SOURCE} ${SIMC_CONSOLE_HEADERS})

INCLUDE_DIRECTORIES(${SIMC_ENGINE_INCLUDE_DIRS})

TARGET_LINK_LIBRARIES(simc-con simc_engine)
IF(WIN32)
  TARGET_LINK_LIBRARIES(simc-con wininet)
ENDIF(WIN32)
TARGET_LINK_LIBRARIES(simc-con ${CMAKE_THREAD_LIBS_INIT})
IF(OPENSSL_FOUND)
  TARGET_LINK_LIBRARIES(simc-con ${OPENSSL_LIBRARIES})
ENDIF(OPENSSL_FOUND)

INSTALL(TARGETS simc-con DESTINATION "." CONFIGURATIONS Release COMPONENT Console)
SET(CPACK_COMPONENT_CONSOLE_DISPLAY_NAME "Console Application")
SET(CPACK_COMPONENT_CONSOLE_DESCRIPTION "Console application usable from the command line.")
SET(CPACK_COMPONENT_CONSOLE_GROUP "Runtime")
SET(CPACK_COMPONENT_GROUP_RUNTIME_DESCRIPTION "All the tools needed to execute profiles.")
SET(CPACK_COMPONENT_CONSOLE_REQUIRED 1)

# <-- CONSOLE CONFIGURATION
###########################

#######################
# QT4 CONFIGURATION -->

IF(QT4_FOUND)

  SET(SIMC_QT_SOURCE
    "qt/main.cpp"
    "qt/sc_window.cpp"
    "qt/sc_import.cpp"
  )
  
  SET(SIMC_QT_HEADERS_MOC
    "qt/simulationcraftqt.hpp"
  )

  SET(SIMC_QT_HEADERS
    ${SIMC_ENGINE_HEADERS}
    ${SIMC_QT_HEADERS_MOC}
  )

  IF(APPLE)
    SET(SIMC_QT_ICON ${CMAKE_CURRENT_SOURCE_DIR}/qt/icon/Simcraft2.icns)
    ADD_DEFINITIONS(-DSIMC_NO_AUTOUPDATE)
  ELSE(APPLE)
    SET(SIMC_QT_ICON ${CMAKE_CURRENT_SOURCE_DIR}/qt/icon/Simcraft2.ico)
  ENDIF(APPLE)

  # OpenSSL configuration ->
  IF(OPENSSL_FOUND)
    ADD_DEFINITIONS(-DUSE_OPENSSL)
    INCLUDE_DIRECTORIES(${OPENSSL_INCLUDE_DIR})
  ENDIF(OPENSSL_FOUND)
  # <- OpenSSL configuration

  INCLUDE(${QT_USE_FILE})
  
  QT4_WRAP_CPP(SIMC_QT_MOC ${SIMC_QT_HEADERS_MOC})
  
  ADD_DEFINITIONS(${QT_DEFINITIONS})
  
  IF(WIN32)
    ADD_EXECUTABLE(simc-qt WIN32 ${SIMC_QT_SOURCE} ${SIMC_QT_MOC} ${SIMC_QT_HEADERS} ${SIMC_QT_ICON})
  ELSE(WIN32)
	ADD_EXECUTABLE(simc-qt ${SIMC_QT_SOURCE} ${SIMC_QT_MOC} ${SIMC_QT_HEADERS} ${SIMC_QT_ICON})
  ENDIF(WIN32)

  INCLUDE_DIRECTORIES(${SIMC_ENGINE_INCLUDE_DIRS})

  TARGET_LINK_LIBRARIES(simc-qt simc_engine)
  IF(WIN32)
    TARGET_LINK_LIBRARIES(simc-qt wininet)
  ENDIF(WIN32)
  TARGET_LINK_LIBRARIES(simc-qt ${QT_LIBRARIES})
  TARGET_LINK_LIBRARIES(simc-qt ${CMAKE_THREAD_LIBS_INIT})
  IF(OPENSSL_FOUND)
    TARGET_LINK_LIBRARIES(simc-qt ${OPENSSL_LIBRARIES})
  ENDIF(OPENSSL_FOUND)
  IF(APPLE)
    TARGET_LINK_LIBRARIES(simc-qt ${COCOA_LIBS})
  ENDIF(APPLE)

  IF(APPLE)
    SET(MACOSX_BUNDLE_ICON_FILE qt/icon/Simcraft2.icns)
    SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/qt/icon/Simcraft2.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  ENDIF(APPLE)

  SET(PLUGIN_DEST_DIR ".")
  SET(QTCONF_DEST_DIR ".")
  SET(APPS "\${CMAKE_INSTALL_PREFIX}/simc-qt")
  IF(APPLE)
    SET(plugin_dest_dir SimulationCraft.app/Contents/MacOS)
    SET(qtconf_dest_dir SimulationCraft.app/Contents/Resources)
    SET(APPS "\${CMAKE_INSTALL_PREFIX}/SimulationCraft.app")
  ENDIF(APPLE)
  IF(WIN32)
    SET(APPS "\${CMAKE_INSTALL_PREFIX}/simc-qt.exe")
  ENDIF(WIN32)


  INSTALL(TARGETS simc-qt DESTINATION "." CONFIGURATIONS Release COMPONENT GUI)
  SET(CPACK_COMPONENT_GUI_DISPLAY_NAME "User Interface Application")
  SET(CPACK_COMPONENT_GUI_DESCRIPTION "User interface to access SimulationCraft features.")
  SET(CPACK_COMPONENT_GUI_GROUP "Runtime")

ENDIF()
# <-- QT4 CONFIGURATION
#######################

INSTALL(DIRECTORY profiles profiles_heal
  DESTINATION "." CONFIGURATIONS Release
  COMPONENT Profiles
  OPTIONAL
  PATTERN ".svn" EXCLUDE
)
SET(CPACK_COMPONENT_PROFILES_DISPLAY_NAME "Profiles")
SET(CPACK_COMPONENT_PROFILES_DESCRIPTION "DPS and HPS pre-generated profiles.")
SET(CPACK_COMPONENT_PROFILES_GROUP "Data")
SET(CPACK_COMPONENT_GROUP_DATA_DESCRIPTION "Data to generate DPS and HPS charts.")

#########################################################################################
######################################### CPACK #########################################
#########################################################################################

IF(WIN32 AND NOT UNIX)
  SET(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
ENDIF(WIN32 AND NOT UNIX)
INCLUDE(InstallRequiredSystemLibraries)

SET(CPACK_NSIS_DISPLAY_NAME "SimulationCraft")
SET(CPACK_PACKAGE_VENDOR "SimulationCraft group")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/READ_ME_FIRST.txt")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Simulationcraft - World of Warcraft DPS Simulator")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "SimulationCraft-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}")
IF(WIN32 AND NOT UNIX)
  SET(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/qt/icon\\\\Simcraft2.ico")
  SET(CPACK_NSIS_MUI_UNICON "${CMAKE_CURRENT_SOURCE_DIR}/qt/icon\\\\Simcraft2.ico")
  SET(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/qt/icon\\\\Simcraft2.ico")
  SET(CPACK_NSIS_INSTALLED_ICON_NAME "simc-qt.exe")
  SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\code.google.com/p/simulationcraft")
  SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\code.google.com/p/simulationcraft/w/list")
  SET(CPACK_NSIS_CONTACT "natehieter@gmail.com")
  SET(CPACK_NSIS_MODIFY_PATH ON)
  SET(CPACK_NSIS_MUI_FINISHPAGE_RUN "simc-qt.exe")
  STRING(REPLACE "/" "\\\\" CURRENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
  SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
    !include \\\"${CURRENT_SOURCE_DIR}\\\\qt\\\\fileassoc.nsh\\\"

    !insertmacro APP_ASSOCIATE \\\"simc\\\" \\\"simulationcraft.simcfile\\\" \\\"$INSTDIR\\\\simc-qt.exe,0\\\" \\\\
          \\\"Open with SimulationCraft\\\" \\\"open\\\" \\\"$INSTDIR\\\\myapp.exe \\\$\\\\\\\"%1\\\$\\\\\\\"\\\"    
  ")
  
  SET(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
	!insertmacro APP_UNASSOCIATE \\\"simc\\\" \\\"simulationcraft.simcfile\\\"
  ")

ENDIF(WIN32 AND NOT UNIX)

INCLUDE(CPack)
