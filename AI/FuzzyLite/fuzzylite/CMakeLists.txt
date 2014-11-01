cmake_minimum_required(VERSION 2.8)

project(fuzzylite CXX)

set(CMAKE_VERBOSE_MAKEFILE false)

set(FL_VERSION 5.0)
add_definitions(-DFL_VERSION="${FL_VERSION}")

set(FL_DATE "1408")
add_definitions(-DFL_DATE="${FL_DATE}")

add_definitions(-DFL_BUILD_PATH="${CMAKE_SOURCE_DIR}") #used to determine FL__FILE__

option(FL_USE_FLOAT "Use fl::scalar as float" OFF)
option(FL_BACKTRACE "Provide backtrace information in case of errors" OFF)
option(FL_CPP11 "Builds utilizing C++11, i.e., passing -std=c++11" ON)

if(FL_USE_FLOAT)
    add_definitions(-DFL_USE_FLOAT)
endif(FL_USE_FLOAT)

if(NOT FL_BACKTRACE)
    add_definitions(-DFL_BACKTRACE_OFF)
endif()

if(FL_CPP11)
    add_definitions(-DFL_CPP11)
    if(UNIX)
        add_definitions(-std=c++11)
    endif(UNIX)
endif(FL_CPP11)


if(WIN32)
    message("Windows")
    add_definitions(-DFL_WINDOWS)
endif()

if (UNIX)
    message("Unix")
    add_definitions(-DFL_UNIX)
endif()

if (APPLE)
    message("Apple")
    add_definitions(-DFL_APPLE)
endif()


set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)

if(UNIX)
    set(CMAKE_CXX_FLAGS "-pedantic -Werror -Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

    if(NOT APPLE)
        set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--no-undefined") #To avoid undefined methods in library
    endif()
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-non-literal-null-conversion")
endif()

set(FL_LIBS)

if(WIN32)
    set(CMAKE_CXX_FLAGS "/WX /W4 /EHsc")
    #Wx: Treat warnings as errors. W4: All warnings
    #http://msdn.microsoft.com/en-us/library/thxezb7y.aspx
    #EHsc: call destructors on __try __catch, and to ignore C4530: C++ exception handler used. Note, unwind semantics are not enabled
    #/wd4251 disable warning 4251 #http://www.unknownroad.com/rtfm/VisualStudio/warningC4251.html
    #To avoid issues from disabling warning 4251, MSVCRT.lib is statically linked to resolve external references
    #http://msdn.microsoft.com/en-us/library/2kzt1wy3.aspx
    #set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
    #set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
    #By default, MD is selected.

    if (FL_BACKTRACE)
        set(FL_LIBS dbghelp)
    endif()
endif()

include_directories(.)
include_directories(..)
cmake_policy(SET CMP0015 OLD)
link_directories(.)

file(STRINGS FL_HEADERS fl-headers)
file(STRINGS FL_SOURCES fl-sources)

string(REGEX REPLACE "\n" " " ${fl-headers} ${fl-headers})
string(REGEX REPLACE "\n" " " ${fl-sources} ${fl-sources})

set(CMAKE_DEBUG_POSTFIX d)
INCLUDE(CheckLibraryExists)

#check if some platform-specific libraries are needed for linking
if(NOT WIN32)
	CHECK_LIBRARY_EXISTS(rt shm_open "" HAVE_RT_LIB)
	if(HAVE_RT_LIB)
		set(SYSTEM_LIBS ${SYSTEM_LIBS} rt)
	endif()
endif()

add_library(fl-shared SHARED ${fl-headers} ${fl-sources})
set_target_properties(fl-shared PROPERTIES OUTPUT_NAME fuzzylite)
set_target_properties(fl-shared PROPERTIES DEBUG_POSTFIX d)
set_target_properties(fl-shared PROPERTIES COMPILE_DEFINITIONS "FL_EXPORT_LIBRARY")
if (UNIX)
    set_target_properties(fl-shared PROPERTIES COMPILE_FLAGS "-fPIC")
endif()
target_link_libraries(fl-shared ${FL_LIBS} ${SYSTEM_LIBS})

add_library(fl-static STATIC ${fl-headers} ${fl-sources})
set_target_properties(fl-static PROPERTIES OUTPUT_NAME fuzzylite-static)
set_target_properties(fl-static PROPERTIES DEBUG_POSTFIX d)
if (UNIX)
    set_target_properties(fl-static PROPERTIES COMPILE_FLAGS "-fPIC")
endif()
target_link_libraries(fl-static ${FL_LIBS} ${SYSTEM_LIBS})

install(TARGETS fl-shared fl-static
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )

install(DIRECTORY fl/ DESTINATION include/fl)
