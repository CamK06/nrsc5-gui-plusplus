cmake_minimum_required(VERSION 3.5)
project(nrsc5-gui-plusplus)

add_subdirectory(thirdparty/spdlog)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

file(GLOB SOURCES "src/*.cpp")

find_package(QT NAMES Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt5 COMPONENTS Widgets REQUIRED)

if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
  execute_process(
    COMMAND git log -1 --format=%h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
else()
  set(GIT_COMMIT "unknown")
endif()
add_definitions("-DGIT_COMMIT=\"${GIT_COMMIT}\"")

add_executable(nrsc5-gui ${SOURCES})
target_link_libraries(nrsc5-gui Qt5::Widgets nrsc5 spdlog)