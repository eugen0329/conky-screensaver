cmake_minimum_required(VERSION 2.8.8)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules" ${CMAKE_MODULE_PATH})

project(conky_screensaver)

if(NOT WIN32)
    string(ASCII 27 Esc)
    set(BOLD_RED "${Esc}[1;31m")
    set(BOLD_BLUE "${Esc}[1;34m")
    set(RESET_COLOR "${Esc}[m")
endif()

######################
# Check requirements
######################

set(REQ_ERR_COUNTER 0)

if(NOT UNIX)
    message(FATAL_ERROR "${BOLD_RED}Only Unix or Unix-like OS required!!!${RESET_COLOR}")
endif()

find_package(PkgConfig REQUIRED)
if(NOT PKG_CONFIG_FOUND)
    message(FATAL_ERROR "${BOLD_BLUE}Pkg-confing is not found${RESET_COLOR}")
endif()


pkg_check_modules(LIBNOTIFY libnotify)
if(NOT LIBNOTIFY_FOUND)
    message(STATUS "Libnotify is not found")
    MATH(EXPR REQ_ERR_COUNTER ${ERR_COUNTER}+1)
endif()


pkg_check_modules(LIBCONFIG libconfig)
if(NOT LIBCONFIG_FOUND)
    message(STATUS "Libconfig is not found")
    MATH(EXPR REQ_ERR_COUNTER ${ERR_COUNTER}+1)
endif()

pkg_check_modules(GTK REQUIRED "gtk+-2.0")
if(NOT GTK_FOUND)
    message(STATUS "Gtk+ 2.0 is not found")
    MATH(EXPR REQ_ERR_COUNTER ${ERR_COUNTER}+1)
endif()


if(REQ_ERR_COUNTER EQUAL 0)
    message(STATUS "${BOLD_BLUE}Requirement successfully found. No error occured.${RESET_COLOR}")
elseif()
    message(SEND_ERROR "${BOLD_RED} Errors: ${REQ_ERR_COUNTER}${RESET_COLOR}" )
    return()
endif()

######################
#       General
######################

set(TARGET_NAME "bin")
set(TARGET ${TARGET_NAME})
set(SRC_DIR "src/")

#set (CMAKE_C_FLAGS "-std=c99 -Wall -pedantic-errors -lXss -lX11")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -lXss -lX11")
set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")


file(GLOB_RECURSE SRC_NAMES ${SRC_DIR}*.c)
add_executable(${TARGET} ${SRC_NAMES})

######################
# Include Settings
######################

include_directories(
    ${PROJECT_BINARY_DIR}
    ${SRC_DIR}
    ${GTK_INCLUDE_DIRS}
    ${LIBCONFIG_INCLUDE_DIRS}
)

target_link_libraries(
    ${TARGET}
    ${LIBNOTIFY_LIBRARIES}
    ${LIBCONFIG_LIBRARIES}
    ${GTK_LIBRARIES}
)


##################
# Install target
##################

install(TARGETS ${TARGET} DESTINATION ${TARGET_NAME})

