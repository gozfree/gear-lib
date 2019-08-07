LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libp2p

ifeq ($(MODE), release)
LOCAL_CFLAGS += -O2
endif

LIBRARIES_DIR	:= $(LOCAL_PATH)/../

LIBSKT_INC := $(LIBRARIES_DIR)/libskt/
LIBRPC_INC := $(LIBRARIES_DIR)/librpc/
LIBGEVENT_INC := $(LIBRARIES_DIR)/libgevent/

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
		    $(LIBSKT_INC) \
		    $(LIBRPC_INC) \
		    $(LIBGEVENT_INC)

# Add your application source files here...
LOCAL_SRC_FILES := libp2p.c

LOCAL_SHARED_LIBRARIES :=  \
			   libskt \
			   librpc


include $(BUILD_SHARED_LIBRARY)
