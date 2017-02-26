set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

cmake_minimum_required(VERSION 2.8.11)

project(light-control)

include(FindPkgConfig)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

find_package(PulseAudio REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5SerialPort REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)

set(pahelper_SRCS
    src/debugview.cpp
    src/serial.cpp
    src/main.cpp
)

include_directories(${Qt5Core_INCLUDE_DIRS} ${Qt5SerialPort_INCLUDE_DIRS} ${Qt5Gui_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS})

add_executable(light-control ${pahelper_SRCS})

target_link_libraries(light-control Qt5::Core Qt5::SerialPort Qt5::Gui Qt5::Widgets)

install(TARGETS light-control DESTINATION bin)
