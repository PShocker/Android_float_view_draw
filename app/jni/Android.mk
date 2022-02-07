LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := jni_draw

LOCAL_SRC_FILES := jni_draw.cpp

LOCAL_CPP_INCLUDES += $(LOCAL_PATH)


include $(BUILD_EXECUTABLE)




