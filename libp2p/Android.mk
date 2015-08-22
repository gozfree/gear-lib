LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libp2p

LIBRARIES_DIR	:= $(LOCAL_PATH)/../

LIBGZF_INC := $(LIBRARIES_DIR)/libgzf/
LIBSTUN_INC := $(LIBRARIES_DIR)/libstun/
LIBSKT_INC := $(LIBRARIES_DIR)/libskt/
LIBRPC_INC := $(LIBRARIES_DIR)/librpc/
LIBPTCP_INC := $(LIBRARIES_DIR)/libptcp/
LIBGEVENT_INC := $(LIBRARIES_DIR)/libgevent/

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
		    $(LIBGZF_INC) \
		    $(LIBSKT_INC) \
		    $(LIBRPC_INC) \
		    $(LIBPTCP_INC) \
		    $(LIBGEVENT_INC) \
		    $(LIBSTUN_INC)

# Add your application source files here...
LOCAL_SRC_FILES := libp2p.c

LOCAL_SHARED_LIBRARIES :=  \
			   libglog \
			   libskt \
			   libstun \
			   librpc \
			   libptcp


include $(BUILD_SHARED_LIBRARY)
