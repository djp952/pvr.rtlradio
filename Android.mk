#-----------------------------------------------------------------------------
# Copyright (c) 2016-2020 Michael G. Brehm
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#-----------------------------------------------------------------------------

LOCAL_PATH := $(call my-dir)

# libusb
#
include $(CLEAR_VARS)
LOCAL_MODULE := libusb-prebuilt
LOCAL_SRC_FILES := depends/libusb/$(TARGET_ABI)/lib/libusb-1.0.a
include $(PREBUILT_STATIC_LIBRARY)

# librtlradio
#
include $(CLEAR_VARS)
LOCAL_MODULE := rtlradio

LOCAL_C_INCLUDES += \
	depends/xbmc/xbmc \
	depends/xbmc/xbmc/linux \
	depends/xbmc/xbmc/addons/kodi-addon-dev-kit/include/kodi \
	depends/xbmc/xbmc/cores/VideoPlayer/Interface/Addon \
	depends/libusb/$(TARGET_ABI)/include \
	depends/rapidjson/include \
	depends/rtl-sdr/include \
	depends/sqlite \
	tmp/version
	
LOCAL_CFLAGS += \
	-Wall \
	-Wno-unused-function \
	-Wno-unused-variable \
	-Wno-unused-const-variable \
	-DNDEBUG \
	-DSQLITE_THREADSAFE=2 \
	-DSQLITE_ENABLE_JSON1=1 \
	-DSQLITE_TEMP_STORE=3
	
LOCAL_CPP_FEATURES := \
	exceptions \
	rtti
	
LOCAL_CPPFLAGS += \
	-std=c++14 \
	-Wno-unknown-pragmas
	
LOCAL_STATIC_LIBRARIES += \
	libusb-prebuilt

LOCAL_LDLIBS += \
	-llog

LOCAL_LDFLAGS += \
	-Wl,--version-script=exportlist/exportlist.android

LOCAL_SRC_FILES := \
    depends/rtl-sdr/src/librtlsdr.c \
    depends/rtl-sdr/src/tuner_e4k.c \
    depends/rtl-sdr/src/tuner_fc0012.c \
    depends/rtl-sdr/src/tuner_fc0013.c \
    depends/rtl-sdr/src/tuner_fc2580.c \
    depends/rtl-sdr/src/tuner_r82xx.c \
    depends/sqlite/sqlite3.c \
    src/fmdsp/agc.cpp \
    src/fmdsp/datamodifier.cpp \
    src/fmdsp/demodulator.cpp \
    src/fmdsp/downconvert.cpp \
    src/fmdsp/fastfir.cpp \
    src/fmdsp/fft.cpp \
    src/fmdsp/fir.cpp \
    src/fmdsp/fractresampler.cpp \
    src/fmdsp/iir.cpp \
    src/fmdsp/smeter.cpp \
    src/fmdsp/wfmdemod.cpp \
    src/database.cpp \
    src/dialogchanneladd.cpp \
    src/dialogchannelscan.cpp \
    src/dialogchannelsettings.cpp \
    src/fmstream.cpp \
    src/libusb_exception.cpp \
    src/pvr.cpp \
    src/rdsdecoder.cpp \
    src/scanner.cpp \
    src/sqlite_exception.cpp \
    src/uecp.cpp \
    src/usbdevice.cpp \
    src/tcpdevice.cpp

include $(BUILD_SHARED_LIBRARY)

# rtl_adsb (do not build - incompatible with Android NDK)
#
# include $(CLEAR_VARS)
# LOCAL_MODULE := rtl_adsb
#
# LOCAL_C_INCLUDES += \
#	depends/libusb/$(TARGET_ABI)/include \
#	depends/rtl-sdr/include
#	
# LOCAL_CFLAGS += \
#	-DNDEBUG
#
# LOCAL_STATIC_LIBRARIES += \
#	libusb-prebuilt
#
# LOCAL_LDLIBS += \
#	-llog
#
# LOCAL_SRC_FILES := \
#    depends/rtl-sdr/src/librtlsdr.c \
#    depends/rtl-sdr/src/tuner_e4k.c \
#    depends/rtl-sdr/src/tuner_fc0012.c \
#    depends/rtl-sdr/src/tuner_fc0013.c \
#    depends/rtl-sdr/src/tuner_fc2580.c \
#    depends/rtl-sdr/src/tuner_r82xx.c \
#    depends/rtl-sdr/src/convenience/convenience.c \
#    depends/rtl-sdr/src/getopt/getopt.c \
#    depends/rtl-sdr/src/rtl_adsb.c
#	
# include $(BUILD_EXECUTABLE)

# rtl_eeprom
#
include $(CLEAR_VARS)
LOCAL_MODULE := rtl_eeprom

LOCAL_CFLAGS += \
	-DNDEBUG

LOCAL_C_INCLUDES += \
	depends/libusb/$(TARGET_ABI)/include \
	depends/rtl-sdr/include
	
LOCAL_STATIC_LIBRARIES += \
	libusb-prebuilt

LOCAL_LDLIBS += \
	-llog

LOCAL_SRC_FILES := \
    depends/rtl-sdr/src/librtlsdr.c \
    depends/rtl-sdr/src/tuner_e4k.c \
    depends/rtl-sdr/src/tuner_fc0012.c \
    depends/rtl-sdr/src/tuner_fc0013.c \
    depends/rtl-sdr/src/tuner_fc2580.c \
    depends/rtl-sdr/src/tuner_r82xx.c \
    depends/rtl-sdr/src/convenience/convenience.c \
    depends/rtl-sdr/src/getopt/getopt.c \
    depends/rtl-sdr/src/rtl_eeprom.c
	
include $(BUILD_EXECUTABLE)

# rtl_fm
#
include $(CLEAR_VARS)
LOCAL_MODULE := rtl_fm

LOCAL_CFLAGS += \
	-DNDEBUG

LOCAL_C_INCLUDES += \
	depends/libusb/$(TARGET_ABI)/include \
	depends/rtl-sdr/include
	
LOCAL_STATIC_LIBRARIES += \
	libusb-prebuilt

LOCAL_LDLIBS += \
	-llog

LOCAL_SRC_FILES := \
    depends/rtl-sdr/src/librtlsdr.c \
    depends/rtl-sdr/src/tuner_e4k.c \
    depends/rtl-sdr/src/tuner_fc0012.c \
    depends/rtl-sdr/src/tuner_fc0013.c \
    depends/rtl-sdr/src/tuner_fc2580.c \
    depends/rtl-sdr/src/tuner_r82xx.c \
    depends/rtl-sdr/src/convenience/convenience.c \
    depends/rtl-sdr/src/getopt/getopt.c \
    depends/rtl-sdr/src/rtl_fm.c
	
include $(BUILD_EXECUTABLE)

# rtl_power
#
include $(CLEAR_VARS)
LOCAL_MODULE := rtl_power

LOCAL_C_INCLUDES += \
	depends/libusb/$(TARGET_ABI)/include \
	depends/rtl-sdr/include
	
LOCAL_CFLAGS += \
	-DNDEBUG

LOCAL_STATIC_LIBRARIES += \
	libusb-prebuilt

LOCAL_LDLIBS += \
	-llog

LOCAL_SRC_FILES := \
    depends/rtl-sdr/src/librtlsdr.c \
    depends/rtl-sdr/src/tuner_e4k.c \
    depends/rtl-sdr/src/tuner_fc0012.c \
    depends/rtl-sdr/src/tuner_fc0013.c \
    depends/rtl-sdr/src/tuner_fc2580.c \
    depends/rtl-sdr/src/tuner_r82xx.c \
    depends/rtl-sdr/src/convenience/convenience.c \
    depends/rtl-sdr/src/getopt/getopt.c \
    depends/rtl-sdr/src/rtl_power.c
	
include $(BUILD_EXECUTABLE)

# rtl_sdr
#
include $(CLEAR_VARS)
LOCAL_MODULE := rtl_sdr

LOCAL_C_INCLUDES += \
	depends/libusb/$(TARGET_ABI)/include \
	depends/rtl-sdr/include
	
LOCAL_CFLAGS += \
	-DNDEBUG

LOCAL_STATIC_LIBRARIES += \
	libusb-prebuilt

LOCAL_LDLIBS += \
	-llog

LOCAL_SRC_FILES := \
    depends/rtl-sdr/src/librtlsdr.c \
    depends/rtl-sdr/src/tuner_e4k.c \
    depends/rtl-sdr/src/tuner_fc0012.c \
    depends/rtl-sdr/src/tuner_fc0013.c \
    depends/rtl-sdr/src/tuner_fc2580.c \
    depends/rtl-sdr/src/tuner_r82xx.c \
    depends/rtl-sdr/src/convenience/convenience.c \
    depends/rtl-sdr/src/getopt/getopt.c \
    depends/rtl-sdr/src/rtl_sdr.c
	
include $(BUILD_EXECUTABLE)

# rtl_tcp
#
include $(CLEAR_VARS)
LOCAL_MODULE := rtl_tcp

LOCAL_C_INCLUDES += \
	depends/libusb/$(TARGET_ABI)/include \
	depends/rtl-sdr/include
	
LOCAL_CFLAGS += \
	-DNDEBUG

LOCAL_STATIC_LIBRARIES += \
	libusb-prebuilt

LOCAL_LDLIBS += \
	-llog

LOCAL_SRC_FILES := \
    depends/rtl-sdr/src/librtlsdr.c \
    depends/rtl-sdr/src/tuner_e4k.c \
    depends/rtl-sdr/src/tuner_fc0012.c \
    depends/rtl-sdr/src/tuner_fc0013.c \
    depends/rtl-sdr/src/tuner_fc2580.c \
    depends/rtl-sdr/src/tuner_r82xx.c \
    depends/rtl-sdr/src/convenience/convenience.c \
    depends/rtl-sdr/src/getopt/getopt.c \
    depends/rtl-sdr/src/rtl_tcp.c
	
include $(BUILD_EXECUTABLE)

# rtl_test
#
include $(CLEAR_VARS)
LOCAL_MODULE := rtl_test

LOCAL_C_INCLUDES += \
	depends/libusb/$(TARGET_ABI)/include \
	depends/rtl-sdr/include
	
LOCAL_CFLAGS += \
	-DNDEBUG

LOCAL_STATIC_LIBRARIES += \
	libusb-prebuilt

LOCAL_LDLIBS += \
	-llog

LOCAL_SRC_FILES := \
    depends/rtl-sdr/src/librtlsdr.c \
    depends/rtl-sdr/src/tuner_e4k.c \
    depends/rtl-sdr/src/tuner_fc0012.c \
    depends/rtl-sdr/src/tuner_fc0013.c \
    depends/rtl-sdr/src/tuner_fc2580.c \
    depends/rtl-sdr/src/tuner_r82xx.c \
    depends/rtl-sdr/src/convenience/convenience.c \
    depends/rtl-sdr/src/getopt/getopt.c \
    depends/rtl-sdr/src/rtl_test.c
	
include $(BUILD_EXECUTABLE)

# sqlite3
#
include $(CLEAR_VARS)
LOCAL_MODULE := sqlite3

LOCAL_C_INCLUDES += \
	depends/sqlite
	
LOCAL_CFLAGS += \
	-DNDEBUG \
	-DSQLITE_ENABLE_JSON1=1

LOCAL_LDLIBS += \
	-llog

LOCAL_SRC_FILES := \
	depends/sqlite/sqlite3.c \
	depends/sqlite/shell.c
	
include $(BUILD_EXECUTABLE)
