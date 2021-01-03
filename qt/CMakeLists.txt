# SimulationCraft GUI

# qt5 detection
# brew does not link qt5 into PATH. Therefore we need to import it
if(APPLE)
list(APPEND CMAKE_PREFIX_PATH /usr/local/opt/qt5)
endif()
find_package(Qt5Core CONFIG REQUIRED)
find_package(Qt5Gui CONFIG REQUIRED)
find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qt5WebEngine CONFIG REQUIRED)
find_package(Qt5WebEngineWidgets CONFIG REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Resource files
FILE(COPY ../qt/Welcome.png DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
FILE(COPY ../qt/Welcome.html DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
FILE(COPY ../qt/Error.html DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

# gui executeable
include(../source_files/cmake_gui.txt)
add_executable(SimulationCraft ${source_files})
target_include_directories(SimulationCraft PUBLIC ../engine/)
target_link_libraries(SimulationCraft engine Qt5::Core Qt5::Gui Qt5::Widgets Qt5::WebEngine Qt5::WebEngineWidgets)
