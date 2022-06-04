
# set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER> <FLAGS> <CMAKE_C_LINK_FLAGS> <OBJECTS> -o <TARGET>.elf <LINK_LIBRARIES>")
# set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> <FLAGS> <CMAKE_CXX_LINK_FLAGS> <OBJECTS> -o <TARGET>.elf <LINK_LIBRARIES>")

# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#     COMMAND ${CMAKE_OBJCOPY} --output-format=binary ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin
#     DEPENDS ${PROJECT_NAME}
#     COMMENT "-- Generating binary file ...")

# variable #{g_dynamic_libs} have dependency dynamic libs and compiled dynamic libs(register component and assigned DYNAMIC or SHARED flag)

# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#     COMMAND mkdir -p ${PROJECT_DIST_DIR}
#     COMMAND cp ${g_dynamic_libs} ${PROJECT_DIST_DIR}
#     COMMAND cp ${PROJECT_BINARY_DIR}/${PROJECT_NAME} ${PROJECT_DIST_DIR}
#     DEPENDS ${PROJECT_NAME}
#     COMMENT "-- copy binary files to dist dir ...")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND mkdir -p ${PROJECT_DIST_DIR}
    COMMAND cp ${PROJECT_BINARY_DIR}/${PROJECT_NAME} ${PROJECT_DIST_DIR}
    # COMMAND cp ${start_app_sh} ${PROJECT_DIST_DIR}/start_app.sh
    # COMMAND chmod +x ${PROJECT_DIST_DIR}/start_app.sh && echo "$curr_dir/${PROJECT_NAME} $@" >> ${PROJECT_DIST_DIR}/start_app.sh VERBATIM
    DEPENDS ${PROJECT_NAME}
    COMMENT "-- copy binary files to dist dir ..."
    )

if(g_dynamic_libs)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND mkdir -p ${PROJECT_DIST_DIR}/lib
        COMMAND cp ${g_dynamic_libs} ${PROJECT_DIST_DIR}/lib/
        DEPENDS ${PROJECT_NAME}
        COMMENT "-- copy dynamic files to dist dir ..."
        )
endif()