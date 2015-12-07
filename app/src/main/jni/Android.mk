LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libevent
LOCAL_SRC_FILES := ../prebuilt/libevent/.libs/libevent.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../prebuilt/libevent/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libevent_pthreads
LOCAL_SRC_FILES := ../prebuilt/libevent/.libs/libevent_pthreads.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../prebuilt/libevent/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libfluid_base
LOCAL_SRC_FILES := ../prebuilt/libfluid_base/.libs/libfluid_base.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../prebuilt/libfluid_base
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libfluid_msg
LOCAL_SRC_FILES := ../prebuilt/libfluid_msg/.libs/libfluid_msg.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../prebuilt/libfluid_msg
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := ofcontroller
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := AndroidController.cc
LOCAL_STATIC_LIBRARIES := libfluid_base libfluid_msg libevent libevent_pthreads
LOCAL_LDLIBS := -llog
LOCAL_CFLAGS += -std=c++11
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := arpscan
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := ArpScan.cc
LOCAL_STATIC_LIBRARIES := libevent_pthreads
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE    := ofswitch
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := AndroidSwitch.cc \
                   switch/datapath.cc \
                   switch/flow.cc switch/port.cc \
                   switch/client/OFClient.cc \
                   switch/client/base/BaseOFClient.cc
LOCAL_C_INCLUDES := $(NDK_ROOT)/external/libpcap
LOCAL_STATIC_LIBRARIES := libfluid_base libfluid_msg libevent libevent_pthreads libpcap
LOCAL_LDLIBS := -ldl -llog
LOCAL_CFLAGS += -std=c++11

include $(BUILD_EXECUTABLE)
include $(NDK_ROOT)/external/libpcap/Android.mk