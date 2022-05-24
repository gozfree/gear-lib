cmake_minimum_required(VERSION 3.5)


# Get SDK path
if(NOT SDK_PATH)
    get_filename_component(SDK_PATH ../../ ABSOLUTE)
    if(EXISTS $ENV{MY_SDK_PATH})
        set(SDK_PATH $ENV{MY_SDK_PATH})
    endif()
endif()

# Check SDK Path
if(NOT EXISTS ${SDK_PATH})
    message(FATAL_ERROR "SDK path Error, Please set SDK_PATH or MY_SDK_PATH variable")
endif()

# Get Toolchain path
if(NOT CONFIG_TOOLCHAIN_PATH)
    if(EXISTS $ENV{MY_TOOLCHAIN_PATH})
        set(CONFIG_TOOLCHAIN_PATH $ENV{MY_TOOLCHAIN_PATH})
    endif()
endif()

## Add preprocessor definitions for whole project
# add_definitions(-DAAAAA=1)

# Call compile
include(${SDK_PATH}/tools/cmake/compile.cmake)


# Project Name, default the same as project directory name
get_filename_component(parent_dir ${CMAKE_PARENT_LIST_FILE} DIRECTORY)
get_filename_component(project_dir_name ${parent_dir} NAME)

set(PROJECT_NAME ${project_dir_name}) # change this var if don't want the same as directory's

message("-- PROJECT_NAME:${PROJECT_NAME}")
project(${PROJECT_NAME})



