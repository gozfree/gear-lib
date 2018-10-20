LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libmp4parser

LIBRARIES_DIR	:= $(LOCAL_PATH)/../

LOCAL_C_INCLUDES := $(LOCAL_PATH)

# Add your application source files here...
LOCAL_SRC_FILES := libmp4parser.c patch.c mp4parser_inner.c

LOCAL_CFLAGS := -Werror -std=c99

include $(BUILD_SHARED_LIBRARY)
