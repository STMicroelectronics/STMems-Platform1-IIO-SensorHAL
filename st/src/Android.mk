#
# Copyright (C) 2013-2020 STMicroelectronics
# Denis Ciocca - Motion MEMS Product Div.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#/

ifneq ($(TARGET_SIMULATOR),true)

LOCAL_PATH := $(call my-dir)
ST_HAL_ROOT_PATH := $(call my-dir)

include $(CLEAR_VARS)
include $(ST_HAL_ROOT_PATH)/../hal_config

ANDROID_VERSION_CONFIG_HAL=$(LOCAL_PATH)/../android_data_config
$(shell source $(ANDROID_VERSION_CONFIG_HAL))

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libhardware \
	libhardware_legacy \
	libutils \
	liblog \
	libdl \
	libc

LOCAL_HEADER_LIBRARIES := \
	libhardware_headers

ifeq ($(shell test $(ST_HAL_ANDROID_VERSION) -gt 4; echo $$?),0)
LOCAL_SHARED_LIBRARIES += libstagefright_foundation
LOCAL_HEADER_LIBRARIES += libstagefright_foundation_headers
endif # ST_HAL_ANDROID_VERSION

LOCAL_VENDOR_MODULE := true

LOCAL_PRELINK_MODULE := false
LOCAL_PROPRIETARY_MODULE := true

ifdef TARGET_BOARD_PLATFORM
LOCAL_MODULE := sensors.$(TARGET_BOARD_PLATFORM)
else # TARGET_BOARD_PLATFORM
LOCAL_MODULE := sensors.default
endif # TARGET_BOARD_PLATFORM

ifeq ($(ST_HAL_ANDROID_VERSION),0)
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
else # ST_HAL_ANDROID_VERSION
LOCAL_MODULE_RELATIVE_PATH := hw
endif # ST_HAL_ANDROID_VERSION

LOCAL_MODULE_OWNER := STMicroelectronics

LOCAL_C_INCLUDES := $(LOCAL_PATH)/ \
			$(LOCAL_PATH)/../

LOCAL_CFLAGS += -DLOG_TAG=\"SensorHAL\" -Wall \
		-Wunused-parameter -Wunused-value -Wunused-function

ifeq ($(DEBUG),y)
LOCAL_CFLAGS += -g -O0
LOCAL_LDFLAGS += -Wl,-Map,$(LOCAL_PATH)/../$(LOCAL_MODULE).map
endif # DEBUG


LOCAL_SRC_FILES := \
		SensorHAL.cpp \
		utils.cpp \
		CircularBuffer.cpp \
		FlushBufferStack.cpp \
		FlushRequested.cpp \
		ChangeODRTimestampStack.cpp \
		SensorBase.cpp \
		HWSensorBase.cpp \
		SWSensorBase.cpp

ifdef CONFIG_ST_HAL_ACCEL_ENABLED
LOCAL_SRC_FILES += Accelerometer.cpp
endif # CONFIG_ST_HAL_ACCEL_ENABLED

ifdef CONFIG_ST_HAL_GYRO_ENABLED
LOCAL_SRC_FILES += Gyroscope.cpp
endif # CONFIG_ST_HAL_GYRO_ENABLED

ifneq ($(or $(CONFIG_ST_HAL_GYRO_UNCALIB_AP_EMULATED),\
	   $(CONFIG_ST_HAL_GYRO_UNCALIB_AP_ENABLED)),)
LOCAL_SRC_FILES += SWGyroscopeUncalibrated.cpp
endif # CONFIG_ST_HAL_GYRO_UNCALIB_AP_EMULATED

ifneq ($(or $(CONFIG_ST_HAL_ACCEL_UNCALIB_AP_EMULATED),\
	   $(CONFIG_ST_HAL_ACCEL_UNCALIB_AP_ENABLED)),)
LOCAL_SRC_FILES += SWAccelerometerUncalibrated.cpp
endif # CONFIG_ST_HAL_ACCEL_UNCALIB_AP_EMULATED

ifdef CONFIG_ST_HAL_ADDITIONAL_INFO_ENABLED
LOCAL_SRC_FILES += SensorAdditionalInfo.cpp
endif # CONFIG_ST_HAL_ADDITIONAL_INFO_ENABLED


LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

endif # !TARGET_SIMULATOR
