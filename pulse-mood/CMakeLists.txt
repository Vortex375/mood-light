set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

cmake_minimum_required(VERSION 2.8.11)

project(pulse-mood)

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
pkg_check_modules(GLIB glib-2.0)
pkg_check_modules(PULSE_GLIB libpulse-mainloop-glib)
pkg_check_modules(FFTW fftw3)

set(pahelper_SRCS
    src/fft.cpp
    src/analysis.cpp
    src/pulseaudio.cpp
    src/reader.cpp
    src/debugview.cpp
    src/serial.cpp
    src/main.cpp
)

include_directories(${Qt5Core_INCLUDE_DIRS} ${Qt5SerialPort_INCLUDE_DIRS} ${Qt5Gui_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS} ${FFTW_INCLUDE_DIRS})

add_executable(pulse-mood ${pahelper_SRCS})

target_link_libraries(pulse-mood Qt5::Core Qt5::SerialPort Qt5::Gui Qt5::Widgets ${PULSEAUDIO_LIBRARY} ${PULSE_GLIB_LIBRARIES} ${FFTW_LIBRARIES})

install(TARGETS pulse-mood DESTINATION bin)
