# SimulationCraft GUI

# qt5 detection
# brew does not link qt5 into PATH. Therefore we need to import it
if(APPLE)
list(APPEND CMAKE_PREFIX_PATH /usr/local/opt/qt5)
endif()
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Gui REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS WebEngineCore REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS WebEngineWidgets REQUIRED)
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
target_link_libraries(SimulationCraft engine Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::WebEngineCore Qt${QT_VERSION_MAJOR}::WebEngineWidgets)
