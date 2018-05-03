# SimulationCraft GUI

if(LINUX AND NOT APPLE)
    find_package(Qt5 COMPONENTS Core Gui WebKit WebKitWidgets)
else()
    find_package(Qt5 COMPONENTS Core Gui WebEngine WebEngineWidgets)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Add all .cpp files recursively
# Also add .hpp files for correct auto-moc
file(GLOB_RECURSE gui_source *.cpp *.hpp)

# Resource files
FILE(COPY ../Welcome.png DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
FILE(COPY ../Welcome.html DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(SimulationCraft ${gui_source})

if(LINUX AND NOT APPLE)
    target_compile_definitions(SimulationCraft PRIVATE SC_USE_WEBKIT)
	target_link_libraries(SimulationCraft engine Qt5::Core Qt5::Gui Qt5::WebKit Qt5::WebKitWidgets)
else()
    target_compile_definitions(SimulationCraft PRIVATE SC_USE_WEBENGINE)
	target_link_libraries(SimulationCraft engine Qt5::Core Qt5::Gui Qt5::WebEngine Qt5::WebEngineWidgets)
endif()

target_include_directories(SimulationCraft PUBLIC ../engine/)
