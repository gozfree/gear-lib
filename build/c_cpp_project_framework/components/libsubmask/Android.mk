LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libsubmask

ifeq ($(MODE), release)
LOCAL_CFLAGS += -O2
endif

LIBRARIES_DIR	:= $(LOCAL_PATH)/../

LOCAL_C_INCLUDES := $(LOCAL_PATH)

# Add your application source files here...
LOCAL_SRC_FILES := libsubmask.c

include $(BUILD_SHARED_LIBRARY)
