cmake_minimum_required(VERSION 3.21)

project(amule-remote-qt
    VERSION 0.1.3
    DESCRIPTION "Qt remote control for aMule, driving amuled over the EC protocol"
    LANGUAGES CXX)

# ADR-0002: latest C++ (C++23), no compiler extensions.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

option(AMULE_REMOTE_QT_BUILD_TESTS "Build the unit tests" ON)

# ADR-0002: minimum supported Qt is 6.5 LTS. ec-core uses Qt Core and Network
# (QTcpSocket for the EC client) but never Qt Widgets (ADR-0003); the GUI app
# uses Qt Widgets (and Svg, to render the bundled icon resources).
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Network Widgets Svg)

# ADR-0003: the EC protocol core is a GUI-agnostic library.
add_subdirectory(core)

# ADR-0002/0003: the Qt Widgets GUI application links ec-core.
add_subdirectory(app)

if(AMULE_REMOTE_QT_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 6.5 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()
