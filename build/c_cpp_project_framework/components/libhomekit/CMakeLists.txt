# Config enable component3 or not in Kconfig
set(MODULE_DIR_C "../../../../gear-lib/libhomekit")
if(CONFIG_LIBHOMEKIT_ENABLED)

    ################# Add include #################
    list(APPEND ADD_INCLUDE "${MODULE_DIR_C}/include")
    list(APPEND ADD_INCLUDE "${MODULE_DIR_C}")
    # list(APPEND ADD_PRIVATE_INCLUDE "include_private")
    ###############################################

    ############## Add source files ###############
    list(APPEND ADD_SRCS
                            "${MODULE_DIR_C}/src/accessories.c"
                            "${MODULE_DIR_C}/src/base64.c"
                            "${MODULE_DIR_C}/src/crypto.c"
                            "${MODULE_DIR_C}/src/debug.c"
                            "${MODULE_DIR_C}/src/json.c"
                            "${MODULE_DIR_C}/src/pairing.c"
                            "${MODULE_DIR_C}/src/port.c"
                            "${MODULE_DIR_C}/src/query_params.c"
                            "${MODULE_DIR_C}/src/storage.c"
                            "${MODULE_DIR_C}/src/server.c"
                            "${MODULE_DIR_C}/src/tlv.c"
                            "${MODULE_DIR_C}/mdns.c"
                            "${MODULE_DIR_C}/accessory.c"
                            "${MODULE_DIR_C}/camera_session.c"
                            "${MODULE_DIR_C}/streaming.c"
                            "${MODULE_DIR_C}/crypto.c"
                            "${MODULE_DIR_C}/cJSON.c"
                            "${MODULE_DIR_C}/http-parser/http_parser.c"
    )

    # aux_source_directory(src ADD_SRCS)  # collect all source file in src dir, will set var ADD_SRCS
    # append_srcs_dir(ADD_SRCS "src")     # append source file in src dir to var ADD_SRCS
    # list(REMOVE_ITEM COMPONENT_SRCS "src/test.c")
    # set(ADD_ASM_SRCS "src/asm.S")
    # list(APPEND ADD_SRCS ${ADD_ASM_SRCS})
    # SET_PROPERTY(SOURCE ${ADD_ASM_SRCS} PROPERTY LANGUAGE C) # set .S  ASM file as C language
    # SET_SOURCE_FILES_PROPERTIES(${ADD_ASM_SRCS} PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp -D BBBBB")
    ###############################################


    ###### Add required/dependent components ######
    list(APPEND ADD_REQUIREMENTS libqueue)
    list(APPEND ADD_REQUIREMENTS libposix)
    list(APPEND ADD_REQUIREMENTS libtime)
    
    ###############################################

    ###### Add link search path for requirements/libs ######
    # list(APPEND ADD_LINK_SEARCH_PATH "${CONFIG_TOOLCHAIN_PATH}/lib")
    # list(APPEND ADD_REQUIREMENTS pthread m)  # add system libs, pthread and math lib for example here
    # list(APPEND ADD_REQUIREMENTS pthread media-io thread uvc pulse xcb xcb-shm xcb-randr xcb-xinerama)
    list(APPEND ADD_REQUIREMENTS wolfssl jpeg x264)
    ###############################################

    ############ Add static libs ##################
    # list(APPEND ADD_STATIC_LIB "lib/libtest.a")
    ###############################################

    ############ Add dynamic libs ##################
    # list(APPEND ADD_DYNAMIC_LIB "lib/arch/v831/libmaix_nn.so"
    #                             "lib/arch/v831/libmaix_cam.so"
    # )
    ###############################################

    #### Add compile option for this component ####
    #### Just for this component, won't affect other 
    #### modules, including component that depend 
    #### on this component
    # list(APPEND ADD_DEFINITIONS_PRIVATE -DAAAAA=1)

    #### Add compile option for this component
    #### and components denpend on this component
    # list(APPEND ADD_DEFINITIONS -DAAAAA222=1
    #                             -DAAAAA333=1)
    list(APPEND ADD_DEFINITIONS 
    -DESP_IDF
    -DWOLFCRYPT_HAVE_SRP
    -DWOLFSSL_SHA512
    -DWOLFSSL_BASE64_ENCODE
    -DNO_MD5
    -DNO_SHA
    -DHAVE_HKDF
    -DHAVE_CHACHA
    -DHAVE_POLY1305
    -DHAVE_ED25519
    -DHAVE_CURVE25519
    -DNO_SESSION_CACHE
    -DRSA_LOW_MEM
    -DGCM_SMALL
    -DUSE_SLOW_SHA512
    -DWOLFCRYPT_ONLY
    )
    ###############################################

    ############ Add static libs ##################
    #### Update parent's variables like CMAKE_C_LINK_FLAGS
    # set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--start-group libmaix/libtest.a -ltest2 -Wl,--end-group" PARENT_SCOPE)
    ###############################################

    # register component, DYNAMIC or SHARED flags will make component compiled to dynamic(shared) lib
    # if(CONFIG_COMPONENT3_DYNAMIC)
    #     register_component(DYNAMIC)
    # else()
    #     register_component()
    # endif()
    register_component()
endif()