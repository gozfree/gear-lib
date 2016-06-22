LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# because liblog is used by android default, so change to libglog
LOCAL_MODULE := liblock

LIBRARIES_DIR	:= $(LOCAL_PATH)/../

LIBGZF_INC := $(LIBRARIES_DIR)/libgzf/

LOCAL_C_INCLUDES := $(LOCAL_PATH)                     $(LIBGZF_INC)

# Add your application source files here...
LOCAL_SRC_FILES := liblock.c

include $(BUILD_SHARED_LIBRARY)
