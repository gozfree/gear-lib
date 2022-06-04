#
# @file from https://github.com/Neutree/c_cpp_project_framework
# @author neucrack
# @license Apache 2.0
#


# Convert to cmake path(for Windows)
file(TO_CMAKE_PATH "${SDK_PATH}" SDK_PATH)

get_filename_component(parent_dir ${CMAKE_PARENT_LIST_FILE} DIRECTORY)
get_filename_component(current_dir ${CMAKE_CURRENT_LIST_FILE} DIRECTORY)
get_filename_component(parent_dir_name ${parent_dir} NAME)

#  global variables
set(g_dynamic_libs "" CACHE INTERNAL "g_dynamic_libs")
set(g_link_search_path "" CACHE INTERNAL "g_link_search_path")

# Set project dir, so just projec can include this cmake file!!!
set(PROJECT_SOURCE_DIR ${parent_dir})
set(PROJECT_PATH       ${PROJECT_SOURCE_DIR})
set(PROJECT_BINARY_DIR "${parent_dir}/build")
set(PROJECT_DIST_DIR   "${parent_dir}/dist")
message(STATUS "SDK_PATH:${SDK_PATH}")
message(STATUS "PROJECT_PATH:${PROJECT_SOURCE_DIR}")

include(${SDK_PATH}/tools/cmake/tools.cmake)

function(register_component)
    get_filename_component(component_dir ${CMAKE_CURRENT_LIST_FILE} DIRECTORY)
    get_filename_component(component_name ${component_dir} NAME)
    message(STATUS "[register component: ${component_name} ], path:${component_dir}")

    # Get params: DYNAMIC/SHARED
    foreach(name ${ARGN})
        string(TOUPPER ${name} name)
        if(${name} STREQUAL "DYNAMIC" OR ${name} STREQUAL "SHARED")
            set(to_dynamic_lib true)
        endif()
    endforeach()
    if(to_dynamic_lib)
        message("-- component ${component_name} will compiled to dynamic lib")
        # Add dynamic file path to g_dynamic_libs variable
        set(dynamic_libs ${g_dynamic_libs})
        list(APPEND dynamic_libs "${PROJECT_BINARY_DIR}/${component_name}/lib${component_name}${DL_EXT}")
        set(g_dynamic_libs ${dynamic_libs}  CACHE INTERNAL "g_dynamic_libs")
    else()
        message("-- component ${component_name} will compiled to static lib")
    endif()

    # Add src to lib
    if(ADD_SRCS)
        if(to_dynamic_lib)
            add_library(${component_name} SHARED ${ADD_SRCS})
        else()
            add_library(${component_name} STATIC ${ADD_SRCS})
        endif()
        set(include_type PUBLIC)
    else()
        if(to_dynamic_lib)
            add_library(${component_name} SHARED)
            set(include_type PUBLIC)
        else()
            add_library(${component_name} INTERFACE)
            set(include_type INTERFACE)
        endif()
    endif()

    # Add include
    foreach(include_dir ${ADD_INCLUDE})
        get_filename_component(abs_dir ${include_dir} ABSOLUTE BASE_DIR ${component_dir})
        if(NOT IS_DIRECTORY ${abs_dir})
            message(FATAL_ERROR "${CMAKE_CURRENT_LIST_FILE}: ${include_dir} not found!")
        endif()
        target_include_directories(${component_name} ${include_type} ${abs_dir})
    endforeach()

    # Add private include
    foreach(include_dir ${ADD_PRIVATE_INCLUDE})
        if(${include_type} STREQUAL INTERFACE)
            message(FATAL_ERROR "${CMAKE_CURRENT_LIST_FILE}: ADD_PRIVATE_INCLUDE set but no source file！")
        endif()
        get_filename_component(abs_dir ${include_dir} ABSOLUTE BASE_DIR ${component_dir})
        if(NOT IS_DIRECTORY ${abs_dir})
            message(FATAL_ERROR "${CMAKE_CURRENT_LIST_FILE}: ${include_dir} not found!")
        endif()
        target_include_directories(${component_name} PRIVATE ${abs_dir})
    endforeach()

    # Add blobal config include
    if(${include_type} STREQUAL INTERFACE)
        target_include_directories(${component_name} INTERFACE ${global_config_dir})
    else()
        target_include_directories(${component_name} PUBLIC ${global_config_dir})
    endif()

    # Add definitions public
    foreach(difinition ${ADD_DEFINITIONS})
        target_compile_options(${component_name} PUBLIC ${difinition})
    endforeach()

    # Add definitions private
    foreach(difinition ${ADD_DEFINITIONS_PRIVATE})
        target_compile_options(${component_name} PRIVATE ${difinition})
    endforeach()

    # Add lib search path
    if(ADD_LINK_SEARCH_PATH)
        foreach(path ${ADD_LINK_SEARCH_PATH})
            if(NOT EXISTS "${path}")
                prepend(lib_full "${component_dir}/" ${path})
                if(NOT EXISTS "${lib_full}")
                    message(FATAL_ERROR "Can not find ${path} or ${lib_full}")
                endif()
                set(path ${lib_full})
            endif()
            get_filename_component(abs_dir ${path} ABSOLUTE)
            if(EXISTS "${abs_dir}")
                set(link_search_path ${g_link_search_path})
                list(APPEND link_search_path "${abs_dir}")
                # target_link_directories(${component_name} PUBLIC ${link_search_path}) # this will fail add -L -Wl,-rpath flag for some .so
                list(REMOVE_DUPLICATES link_search_path)
                set(g_link_search_path ${link_search_path}  CACHE INTERNAL "g_link_search_path")
            endif()
        endforeach()
    endif()

    # Add static lib
    if(ADD_STATIC_LIB)
        foreach(lib ${ADD_STATIC_LIB})
            if(NOT EXISTS "${lib}")
                prepend(lib_full "${component_dir}/" ${lib})
                if(NOT EXISTS "${lib_full}")
                    message(FATAL_ERROR "Can not find ${lib} or ${lib_full}")
                endif()
                set(lib ${lib_full})
            endif()
            target_link_libraries(${component_name} ${include_type} ${lib})
        endforeach()
    endif()
    # Add dynamic lib
    if(ADD_DYNAMIC_LIB)
        set(dynamic_libs ${g_dynamic_libs})
        foreach(lib ${ADD_DYNAMIC_LIB})
            if(NOT EXISTS "${lib}")
                prepend(lib_full "${component_dir}/" ${lib})
                if(NOT EXISTS "${lib_full}")
                    message(FATAL_ERROR "Can not find ${lib} or ${lib_full}")
                endif()
                set(lib ${lib_full})
            endif()
            get_filename_component(lib ${lib} ABSOLUTE)
            list(APPEND dynamic_libs ${lib})
            get_filename_component(lib_dir ${lib} DIRECTORY)
            get_filename_component(lib_name ${lib} NAME)
            target_link_libraries(${component_name} ${include_type} -L${lib_dir} ${lib_name})
        endforeach()
        list(REMOVE_DUPLICATES dynamic_libs)
        set(g_dynamic_libs ${dynamic_libs}  CACHE INTERNAL "g_dynamic_libs")
    endif()

    # Add requirements
    target_link_libraries(${component_name} ${include_type} ${ADD_REQUIREMENTS})
endfunction()

function(is_path_component ret param_path)
    set(res 1)
    get_filename_component(abs_dir ${param_path} ABSOLUTE)

    if(NOT IS_DIRECTORY "${abs_dir}")
        set(res 0)
    endif()

    get_filename_component(base_dir ${abs_dir} NAME)
    string(SUBSTRING "${base_dir}" 0 1 first_char)

    if(NOT first_char STREQUAL ".")
        if(NOT EXISTS "${abs_dir}/CMakeLists.txt")
            set(res 0)
        endif()
    else()
        set(res 0)
    endif()

    set(${ret} ${res} PARENT_SCOPE)
endfunction()

function(get_python python version info_str)
    set(res 1)
    execute_process(COMMAND python3 --version RESULT_VARIABLE cmd_res OUTPUT_VARIABLE cmd_out)
    if(${cmd_res} EQUAL 0)
        set(${python} python3 PARENT_SCOPE)
        set(${version} 3 PARENT_SCOPE)
        set(${info_str} ${cmd_out} PARENT_SCOPE)
    else()
        execute_process(COMMAND python --version RESULT_VARIABLE cmd_res OUTPUT_VARIABLE cmd_out)
        if(${cmd_res} EQUAL 0)
            set(${python} python PARENT_SCOPE)
            set(${version} 2 PARENT_SCOPE)
            set(${info_str} ${cmd_out} PARENT_SCOPE)
        endif()
    endif()
endfunction(get_python python)


macro(project name)
    
    get_filename_component(current_dir ${CMAKE_CURRENT_LIST_FILE} DIRECTORY)
    set(PROJECT_SOURCE_DIR ${current_dir})
    set(PROJECT_BINARY_DIR "${current_dir}/build")

    # Find components in SDK's components folder, register components
    file(GLOB component_dirs ${SDK_PATH}/components/*)
    foreach(component_dir ${component_dirs})
        is_path_component(is_component ${component_dir})
        if(is_component)
            message(STATUS "Find component: ${component_dir}")
            get_filename_component(base_dir ${component_dir} NAME)
            list(APPEND components_dirs ${component_dir})
            if(EXISTS ${component_dir}/Kconfig)
                message(STATUS "Find component Kconfig of ${base_dir}")
                list(APPEND components_kconfig_files ${component_dir}/Kconfig)
            endif()
            if(EXISTS ${component_dir}/config_defaults.mk)
                message(STATUS "Find component defaults config of ${base_dir}")
                list(APPEND kconfig_defaults_files_args --defaults "${component_dir}/config_defaults.mk")
            endif()
        endif()
    endforeach()

    # Find components in project folder
    file(GLOB project_component_dirs ${PROJECT_SOURCE_DIR}/*)
    foreach(component_dir ${project_component_dirs})
        is_path_component(is_component ${component_dir})
        if(is_component)
            message(STATUS "find component: ${component_dir}")
            get_filename_component(base_dir ${component_dir} NAME)
            list(APPEND components_dirs ${component_dir})
            if(${base_dir} STREQUAL "main")
                set(main_component 1)
            endif()
            if(EXISTS ${component_dir}/Kconfig)
                message(STATUS "Find component Kconfig of ${base_dir}")
                list(APPEND components_kconfig_files ${component_dir}/Kconfig)
            endif()
            if(EXISTS ${component_dir}/config_defaults.mk)
                message(STATUS "Find component defaults config of ${base_dir}")
                list(APPEND kconfig_defaults_files_args --defaults "${component_dir}/config_defaults.mk")
            endif()
        endif()
    endforeach()
    if(NOT main_component)
        message(FATAL_ERROR "=================\nCan not find main component(folder) in project folder!!\n=================")
    endif()

    # Find default config file
    if(DEFAULT_CONFIG_FILE)
        if(EXISTS ${PROJECT_SOURCE_DIR}/.config.mk)
            message(STATUS "Find project defaults config(.config.mk)")
            list(APPEND kconfig_defaults_files_args --defaults "${PROJECT_SOURCE_DIR}/.config.mk")
        endif()
        message(STATUS "Project defaults config file:${DEFAULT_CONFIG_FILE}")
        list(APPEND kconfig_defaults_files_args --defaults "${DEFAULT_CONFIG_FILE}")
    else()
        if(EXISTS ${PROJECT_SOURCE_DIR}/config_defaults.mk)
            message(STATUS "Find project defaults config(config_defaults.mk)")
            list(APPEND kconfig_defaults_files_args --defaults "${PROJECT_SOURCE_DIR}/config_defaults.mk")
        endif()
        if(EXISTS ${PROJECT_SOURCE_DIR}/.config.mk)
            message(STATUS "Find project defaults config(.config.mk)")
            list(APPEND kconfig_defaults_files_args --defaults "${PROJECT_SOURCE_DIR}/.config.mk")
        endif()
    endif()

    # Generate config file from Kconfig
    get_python(python python_version python_info_str)
    if(NOT python)
        message(FATAL_ERROR "python not found, please install python firstly(python3 recommend)!")
    endif()
    message(STATUS "python command: ${python}, version: ${python_info_str}")
    string(REPLACE ";" " " components_kconfig_files "${kconfig_defaults_files_args}")
    string(REPLACE ";" " " components_kconfig_files "${components_kconfig_files}")
    set(generate_config_cmd ${python}  ${SDK_PATH}/tools/kconfig/genconfig.py
                            --kconfig "${SDK_PATH}/Kconfig"
                            ${kconfig_defaults_files_args}
                            --menuconfig False
                            --env "SDK_PATH=${SDK_PATH}"
                            --env "PROJECT_PATH=${PROJECT_SOURCE_DIR}"
                            --output makefile ${PROJECT_BINARY_DIR}/config/global_config.mk
                            --output cmake  ${PROJECT_BINARY_DIR}/config/global_config.cmake
                            --output header ${PROJECT_BINARY_DIR}/config/global_config.h
                            )
    set(generate_config_cmd2 ${python}  ${SDK_PATH}/tools/kconfig/genconfig.py
                            --kconfig "${SDK_PATH}/Kconfig"
                            ${kconfig_defaults_files_args}
                            --menuconfig True
                            --env "SDK_PATH=${SDK_PATH}"
                            --env "PROJECT_PATH=${PROJECT_SOURCE_DIR}"
                            --output makefile ${PROJECT_BINARY_DIR}/config/global_config.mk
                            --output cmake  ${PROJECT_BINARY_DIR}/config/global_config.cmake
                            --output header ${PROJECT_BINARY_DIR}/config/global_config.h
                            )
    execute_process(COMMAND ${generate_config_cmd} RESULT_VARIABLE cmd_res)
    if(NOT cmd_res EQUAL 0)
        message(FATAL_ERROR "Check Kconfig content")
    endif()

    # Include confiurations
    set(global_config_dir "${PROJECT_BINARY_DIR}/config")
    include(${global_config_dir}/global_config.cmake)
    if(WIN32)
        set(EXT ".exe")
        set(DL_EXT ".dll")
    else()
        set(EXT "")
        set(DL_EXT ".so")
    endif()

    # Config toolchain
    if(CONFIG_TOOLCHAIN_PATH OR CONFIG_TOOLCHAIN_PREFIX)
        if(CONFIG_TOOLCHAIN_PATH)
            if(WIN32)
                file(TO_CMAKE_PATH ${CONFIG_TOOLCHAIN_PATH} CONFIG_TOOLCHAIN_PATH)
            endif()
            if(NOT IS_DIRECTORY ${CONFIG_TOOLCHAIN_PATH})
                message(FATAL_ERROR "TOOLCHAIN_PATH set error:${CONFIG_TOOLCHAIN_PATH}")
            endif()
            set(path_split /)
        endif()
        set(TOOLCHAIN_PATH ${CONFIG_TOOLCHAIN_PATH})
        message(STATUS "TOOLCHAIN_PATH:${CONFIG_TOOLCHAIN_PATH}")
        set(CMAKE_C_COMPILER "${CONFIG_TOOLCHAIN_PATH}${path_split}${CONFIG_TOOLCHAIN_PREFIX}gcc${EXT}")
        set(CMAKE_CXX_COMPILER "${CONFIG_TOOLCHAIN_PATH}${path_split}${CONFIG_TOOLCHAIN_PREFIX}g++${EXT}")
        set(CMAKE_ASM_COMPILER "${CONFIG_TOOLCHAIN_PATH}${path_split}${CONFIG_TOOLCHAIN_PREFIX}gcc${EXT}")
        set(CMAKE_LINKER "${CONFIG_TOOLCHAIN_PATH}${path_split}${CONFIG_TOOLCHAIN_PREFIX}ld${EXT}")
    else()
        set(CMAKE_C_COMPILER "gcc${EXT}")
        set(CMAKE_CXX_COMPILER "g++${EXT}")
        set(CMAKE_ASM_COMPILER "gcc${EXT}")
        set(CMAKE_LINKER  "ld${EXT}")
    endif()

    set(CMAKE_C_COMPILER_WORKS 1)
    set(CMAKE_CXX_COMPILER_WORKS 1)

    
    # set(CMAKE_SYSTEM_NAME Generic) # set this flag may leads to dymamic(/shared) lib compile fail

    # Declare project # This function will cler flags!
    _project(${name} ASM C CXX)
    
    include(${SDK_PATH}/tools/cmake/compile_flags.cmake)

    # Add dependence: update configfile, append time and git info for global config header file
    # we didn't generate build info for cmake and makefile for if we do, it will always rebuild cmake
    # everytime we execute make
    set(gen_build_info_config_cmd ${python}  ${SDK_PATH}/tools/kconfig/update_build_info.py
                                  --configfile header ${PROJECT_BINARY_DIR}/config/global_build_info_time.h ${PROJECT_BINARY_DIR}/config/global_build_info_version.h
                                  )
    add_custom_target(update_build_info COMMAND ${gen_build_info_config_cmd})

    # Sort component according to priority.conf config file
    set(component_priority_conf_file "${PROJECT_PATH}/compile/priority.conf")
    set(sort_components ${python}  ${SDK_PATH}/tools/cmake/sort_components.py
                                   ${component_priority_conf_file} ${components_dirs}
                        )
    execute_process(COMMAND ${sort_components} OUTPUT_VARIABLE component_dirs_sorted RESULT_VARIABLE cmd_res)
    if(cmd_res EQUAL 2)
        message(STATUS "No components priority config file")
        set(component_dirs_sorted ${components_dirs})
    elseif(cmd_res EQUAL 0)
        message(STATUS "Config components priority success")
    else()
        message(STATUS "Components priority config fail ${component_dirs_sorted}, check config file:${component_priority_conf_file}")
    endif()

    # Call CMakeLists.txt
    foreach(component_dir ${component_dirs_sorted})
        get_filename_component(base_dir ${component_dir} NAME)
        add_subdirectory(${component_dir} ${base_dir})
        if(TARGET ${base_dir})
            add_dependencies(${base_dir} update_build_info) # add build info dependence
        else()
            message(STATUS "component ${base_dir} not enabled")
        endif()
    endforeach()

    # Add lib search path to link flags
    foreach(abs_dir ${g_link_search_path})
        set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -L${abs_dir} -Wl,-rpath,${abs_dir}")
        set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -L${abs_dir} -Wl,-rpath,${abs_dir}")
    endforeach()

    # Add menuconfig target for makefile
    add_custom_target(menuconfig COMMAND ${generate_config_cmd2})

    # Create dummy source file exe_src.c to satisfy cmake's `add_executable` interface!
    set(exe_src ${CMAKE_BINARY_DIR}/exe_src.c)
    add_executable(${name} "${exe_src}")
    add_custom_command(OUTPUT ${exe_src} COMMAND ${CMAKE_COMMAND} -E touch ${exe_src} VERBATIM)
    add_custom_target(gen_exe_src DEPENDS "${exe_src}")
    add_dependencies(${name} gen_exe_src)

    # Add main component(lib)
    target_link_libraries(${name} main)

    # Add binary
    include("${SDK_PATH}/tools/cmake/gen_binary.cmake")

endmacro()



