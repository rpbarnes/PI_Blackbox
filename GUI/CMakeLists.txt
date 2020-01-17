cmake_minimum_required(VERSION 2.8.0)

PROJECT("GTK Pi GUI" C)

ADD_EXECUTABLE(PI_DAQ Rasp_GUI.c)

SET(CMAKE_C_FLAGS "-std=gnu99 -g -O2 -Wall -Wextra -rdynamic ")

find_package( PkgConfig REQUIRED)


#pkg_check_modules ( GLIB REQUIRED glib-2.0 )
#include_directories(${GLIB_INCLUDE_DIRS})
#set(LIBS ${LIBS} ${GLIB_LIBRARIES})

#pkg_check_modules ( GDK REQUIRED gdk-3.0 )
#include_directories(${GDK_INCLUDE_DIRS})
#set(LIBS ${LIBS} ${GDK_LIBRARIES})

pkg_check_modules ( GTK REQUIRED gtk+-3.0 )
include_directories(${GTK_INCLUDE_DIRS})
set(LIBS ${LIBS} ${GTK_LIBRARIES})

#pkg_check_modules ( LIBGTOP  REQUIRED libgtop-2.0 )
#include_directories(${LIBGTOP_INCLUDE_DIRS})
#set(LIBGTOPLIBS  ${LIBGTOP_LIBRARIES})

pkg_check_modules ( FT2  REQUIRED freetype2 )
include_directories(${FT2_INCLUDE_DIRS})
set(FT2LIBS  ${FT2_LIBRARIES})


target_link_libraries(PI_DAQ ${LIBS} )
