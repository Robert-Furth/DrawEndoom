cmake_minimum_required(VERSION 3.22.0)
project(DrawEndoom VERSION 0.1.0)
set_property(GLOBAL PROPERTY CMAKE_C_STANDARD 11)
include("cmake/helpers.cmake")

if(WIN32)
  set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};deps/")
endif()

find_package(SDL2 REQUIRED)

#include(CTest) 
#enable_testing()

embed_resource("resources/font.bmp" "src/resources/font.h")

add_executable(DrawEndoom WIN32
  src/main.c
  src/endoom.c
  src/rendering.c
)
set_property(TARGET DrawEndoom PROPERTY VS_DPI_AWARE "PerMonitor")
target_link_libraries(DrawEndoom PRIVATE ${SDL2_LIBRARIES})
if(WIN32)
  copy_dlls(TARGET DrawEndoom DLLS "${SDL2_BINDIR}/SDL2.dll")
endif()

add_executable(ToConsole src/main_console.c)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
