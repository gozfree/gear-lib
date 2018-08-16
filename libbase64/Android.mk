LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libbase64

LIBRARIES_DIR	:= $(LOCAL_PATH)/../

LOCAL_C_INCLUDES := $(LOCAL_PATH)

# Add your application source files here...
LOCAL_SRC_FILES := libbase64.c

include $(BUILD_SHARED_LIBRARY)
