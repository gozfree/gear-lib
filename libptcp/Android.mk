LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libptcp

LIBRARIES_DIR	:= $(LOCAL_PATH)/../

LOCAL_C_INCLUDES := $(LOCAL_PATH)

# Add your application source files here...
LOCAL_SRC_FILES := libptcp.c

LOCAL_SHARED_LIBRARIES :=  \
                          libskt-prebuilt


include $(BUILD_SHARED_LIBRARY)
