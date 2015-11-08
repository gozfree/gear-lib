LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libgevent

LIBRARIES_DIR	:= $(LOCAL_PATH)/../

LIBGZF_INC := $(LIBRARIES_DIR)/libgzf/
LIBLOG_INC := $(LIBRARIES_DIR)/liblog/

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
                    $(LIBGZF_INC) \
                    $(LIBLOG_INC)

# Add your application source files here...
LOCAL_SRC_FILES := libgevent.c epoll.c poll.c select.c

LOCAL_LDLIBS += -llog

include $(BUILD_SHARED_LIBRARY)
