cmake_minimum_required(VERSION 3.5)
project(host_update)

MACRO(INCLUDE_SUBDIR targetdir)
FILE(GLOB files ${targetdir}/*)
FOREACH(file IN LISTS files)
    IF(IS_DIRECTORY ${file})
        include_directories(${file})
    ENDIF()
ENDFOREACH(file)
ENDMACRO()

# set ouput folder name
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)

# set project version info
#   < Version format >
#   ------------
#   vx.x.x
#   YYYY.MM.DD
#   ------------
#   Note:
#       1. 'V' and 'v' are both accept for version
#       2. '/' and '.' are both accept for date
set(RELEASE_VERSION v1.1.0)
set(RELEASE_DATE 2022.08.16)

# define macro to include config.h
add_definitions(-DHAVE_CONFIG_H)
add_definitions(-D_GNU_SOURCE)
add_definitions(-DCONFIG_PROJ_VERSION="${RELEASE_VERSION}")
add_definitions(-DCONFIG_PROJ_DATE="${RELEASE_DATE}")

# list neccessary code in freeipmi 
FILE(GLOB_RECURSE freeipmi_sources freeipmi-1.6.9/libfreeipmi/*.c)
FILE(GLOB freeipmi_common freeipmi-1.6.9/common/debugutil/*.c freeipmi-1.6.9/common/miscutil/*.c)
LIST(APPEND freeipmi_sources ${freeipmi_common})

# add target of src and common code(.c)
set(source_path src)
set(common_path common)

FILE(GLOB app_sources
    ${source_path}/*.c
    ${source_path}/fwupdate/*.c
)
FILE(GLOB common_sources
    ${common_path}/*.c
    ${common_path}/freeipmi/*.c
    ${common_path}/plock/*.c
)

# set output file name and add target above
add_executable( host_update
    ${freeipmi_sources}
    ${app_sources}
    ${common_sources}
)

# include directories of header(.h)
INCLUDE_SUBDIR(freeipmi-1.6.9)
INCLUDE_SUBDIR(freeipmi-1.6.9/common)
INCLUDE_SUBDIR(freeipmi-1.6.9/libfreeipmi)
include_directories(freeipmi-1.6.9 include)
include_directories(
    ${source_path}/include
    ${source_path}/fwupdate/include
    ${common_path}/include
    ${common_path}/freeipmi/include
    ${common_path}/plock/include
)

# link c math library
target_link_libraries(host_update m)
