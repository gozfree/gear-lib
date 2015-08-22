LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libstun

LIBRARIES_DIR	:= $(LOCAL_PATH)/../

LIBGZF_INC := $(LIBRARIES_DIR)/libgzf/

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
                    $(LIBGZF_INC)

# Add your application source files here...
LOCAL_SRC_FILES := libstun.c

include $(BUILD_SHARED_LIBRARY)
