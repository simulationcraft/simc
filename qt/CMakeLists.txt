# SimulationCraft GUI

find_package(Qt5 COMPONENTS Core Gui WebKit WebKitWidgets)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Add all .cpp files recursively
# Also add .hpp files for correct auto-moc
file(GLOB_RECURSE gui_source *.cpp *.hpp)

add_executable(SimulationCraft ${gui_source})
target_compile_definitions(SimulationCraft PRIVATE SC_USE_WEBKIT)
target_link_libraries(SimulationCraft engine Qt5::Core Qt5::Gui Qt5::WebKit Qt5::WebKitWidgets)
target_include_directories(SimulationCraft PUBLIC ../engine/)
