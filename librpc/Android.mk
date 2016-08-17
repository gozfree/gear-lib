LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := librpc

LIBRARIES_DIR	:= $(LOCAL_PATH)/../

LIBSKT_INC := $(LIBRARIES_DIR)/libskt/
LIBGEVENT_INC := $(LIBRARIES_DIR)/libgevent/
LIBGLOG_INC := $(LIBRARIES_DIR)/liblog/

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
                    $(LIBSKT_INC) \
                    $(LIBGEVENT_INC) \
                    $(LIBGLOG_INC)

# Add your application source files here...
LOCAL_SRC_FILES := librpc.c

LOCAL_SHARED_LIBRARIES :=  \
			   libgevent \
			   libskt

LOCAL_LDLIBS += -llog

include $(BUILD_SHARED_LIBRARY)
