# SimulationCraft GUI

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Gui REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS WebEngineCore REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS WebEngineWidgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS LinguistTools QUIET)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Copy runtime dependencies to build directory
FILE(COPY Welcome.png DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
FILE(COPY Welcome.html DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
FILE(COPY Error.html DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
FILE(COPY locale/sc_de.qm DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/locale")
FILE(COPY locale/sc_cn.qm DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/locale")
FILE(COPY locale/sc_it.qm DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/locale")
FILE(COPY locale/sc_ko.qm DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/locale")


# gui executeable
include(../source_files/cmake_gui.txt)
if (WIN32)
  list(APPEND source_files ../simcqt.rc)
endif()
add_executable(SimulationCraft WIN32 ${source_files})

sc_common_compiler_options(SimulationCraft)

target_include_directories(SimulationCraft PUBLIC ../engine/)
target_link_libraries(SimulationCraft 
  engine
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Gui
  Qt${QT_VERSION_MAJOR}::Widgets
  Qt${QT_VERSION_MAJOR}::WebEngineCore
  Qt${QT_VERSION_MAJOR}::WebEngineWidgets
)

#translations
if (Qt${QT_VERSION_MAJOR}LinguistTools_FOUND AND ${QT_VERSION_MAJOR} EQUAL 6)
  qt_add_lupdate(SimulationCraft TS_FILES 
    locale/sc_de.ts locale/sc_cn.ts locale/sc_it.ts locale/sc_ko.ts)
  set_source_files_properties(locale/sc_de.ts locale/sc_cn.ts locale/sc_it.ts locale/sc_ko.ts
    PROPERTIES OUTPUT_LOCATION "locale")
  qt_add_lrelease(SimulationCraft TS_FILES 
    locale/sc_de.ts locale/sc_cn.ts locale/sc_it.ts locale/sc_ko.ts
    )
endif()

install(TARGETS SimulationCraft DESTINATION ${SIMC_INSTALL_BIN})
install(FILES 
  Error.html
  Welcome.html
  Welcome.png
DESTINATION ${SIMC_INSTALL_SHARED})
install(FILES 
  locale/sc_de.qm
  locale/sc_cn.qm
  locale/sc_it.qm
  locale/sc_ko.qm
DESTINATION ${SIMC_INSTALL_SHARED}/locale)

if(WIN32)
  include(${PROJECT_SOURCE_DIR}/cmake/windeployqt.cmake)
  windeployqt(SimulationCraft)
endif()