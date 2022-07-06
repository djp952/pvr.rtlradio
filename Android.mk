#-----------------------------------------------------------------------------
# Copyright (c) 2020-2022 Michael G. Brehm
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

# libfaad-hdc
#
include $(CLEAR_VARS)
LOCAL_MODULE := libfaad-hdc-prebuilt
LOCAL_SRC_FILES := depends/faad2-hdc/$(TARGET_ABI)/lib/libfaad_hdc.a
include $(PREBUILT_STATIC_LIBRARY)

# libfftw
#
include $(CLEAR_VARS)
LOCAL_MODULE := libfftw-prebuilt
LOCAL_SRC_FILES := depends/fftw/$(TARGET_ABI)/lib/libfftw3f.a
include $(PREBUILT_STATIC_LIBRARY)

# libmpg123
#
include $(CLEAR_VARS)
LOCAL_MODULE := libmpg123-prebuilt
LOCAL_SRC_FILES := depends/mpg123/$(TARGET_ABI)/lib/libmpg123.a
include $(PREBUILT_STATIC_LIBRARY)

# libusb
#
include $(CLEAR_VARS)
LOCAL_MODULE := libusb-prebuilt
LOCAL_SRC_FILES := depends/libusb/$(TARGET_ABI)/lib/libusb-1.0.a
include $(PREBUILT_STATIC_LIBRARY)

# libusb-jni
#
include $(CLEAR_VARS)
LOCAL_MODULE := libusb-jni-prebuilt
LOCAL_SRC_FILES := depends/libusb-jni/$(TARGET_ABI)/lib/libusb-1.0.a
include $(PREBUILT_STATIC_LIBRARY)

# librtlradio
#
include $(CLEAR_VARS)
LOCAL_MODULE := rtlradio

LOCAL_C_INCLUDES += \
	depends/glm \
	depends/xbmc/xbmc \
	depends/xbmc/xbmc/linux \
	depends/xbmc/xbmc/addons/kodi-dev-kit/include \
	depends/xbmc/xbmc/cores/VideoPlayer/Interface/Addon \
	depends/faad2-hdc/$(TARGET_ABI)/include \
	depends/fftw/$(TARGET_ABI)/include \
	depends/mpg123/$(TARGET_ABI)/include \
	depends/libusb-jni/$(TARGET_ABI)/include \
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
	-DSQLITE_TEMP_STORE=3 \
	-DDABLIN_AAC_FAAD2

LOCAL_CPP_FEATURES := \
	exceptions \
	rtti

LOCAL_CPPFLAGS += \
	-std=c++14 \
	-Wno-unknown-pragmas

LOCAL_STATIC_LIBRARIES += \
	libfaad-hdc-prebuilt \
	libfftw-prebuilt \
	libmpg123-prebuilt \
	libusb-jni-prebuilt

LOCAL_LDLIBS += \
	-llog \
	-lm \
	-lGLESv3

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
	src/compat/bionic/complex.cpp \
	src/dabdsp/channels.cpp \
	src/dabdsp/charsets.cpp \
	src/dabdsp/dab_decoder.cpp \
	src/dabdsp/dab-audio.cpp \
	src/dabdsp/dab-constants.cpp \
	src/dabdsp/dabplus_decoder.cpp \
	src/dabdsp/decode_rs_char.c \
	src/dabdsp/decoder_adapter.cpp \
	src/dabdsp/eep-protection.cpp \
	src/dabdsp/encode_rs_char.c \
	src/dabdsp/fft.cpp \
	src/dabdsp/fib-processor.cpp \
	src/dabdsp/fic-handler.cpp \
	src/dabdsp/freq-interleaver.cpp \
	src/dabdsp/init_rs_char.c \
	src/dabdsp/mot_manager.cpp \
	src/dabdsp/msc-handler.cpp \
	src/dabdsp/ofdm-decoder.cpp \
	src/dabdsp/ofdm-processor.cpp \
	src/dabdsp/pad_decoder.cpp \
	src/dabdsp/phasereference.cpp \
	src/dabdsp/phasetable.cpp \
	src/dabdsp/profiling.cpp \
	src/dabdsp/protTables.cpp \
	src/dabdsp/radio-receiver.cpp \
	src/dabdsp/tii-decoder.cpp \
	src/dabdsp/tools.cpp \
	src/dabdsp/uep-protection.cpp \
	src/dabdsp/viterbi.cpp \
	src/dabdsp/Xtan2.cpp \
	src/fmdsp/demodulator.cpp \
	src/fmdsp/downconvert.cpp \
	src/fmdsp/fastfir.cpp \
	src/fmdsp/fft.cpp \
	src/fmdsp/fir.cpp \
	src/fmdsp/fmdemod.cpp \
	src/fmdsp/fractresampler.cpp \
	src/fmdsp/iir.cpp \
	src/fmdsp/wfmdemod.cpp \
	src/hddsp/acquire.c \
	src/hddsp/conv_dec.c \
	src/hddsp/decode.c \
	src/hddsp/firdecim_q15.c \
	src/hddsp/frame.c \
	src/hddsp/input.c \
	src/hddsp/nrsc5.c \
	src/hddsp/output.c \
	src/hddsp/pids.c \
	src/hddsp/rs_decode.c \
	src/hddsp/rs_init.c \
	src/hddsp/strndup.c \
	src/hddsp/sync.c \
	src/hddsp/unicode.c \
	src/addon.cpp \
	src/channeladd.cpp \
	src/channelsettings.cpp \
	src/database.cpp \
	src/dabmuxscanner.cpp \
	src/dabstream.cpp \
	src/filedevice.cpp \
	src/fmstream.cpp \
	src/hdmuxscanner.cpp \
	src/hdstream.cpp \
	src/id3v1tag.cpp \
	src/id3v2tag.cpp \
	src/libusb_exception.cpp \
	src/rdsdecoder.cpp \
	src/signalmeter.cpp \
	src/sqlite_exception.cpp \
	src/uecp.cpp \
	src/usbdevice.cpp \
	src/tcpdevice.cpp \
	src/wxstream.cpp

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

# rtl_biast
#
include $(CLEAR_VARS)
LOCAL_MODULE := rtl_biast

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
	depends/rtl-sdr/src/rtl_biast.c
	
include $(BUILD_EXECUTABLE)

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
	-DNDEBUG

LOCAL_LDLIBS += \
	-llog

LOCAL_SRC_FILES := \
	depends/sqlite/sqlite3.c \
	depends/sqlite/shell.c
	
include $(BUILD_EXECUTABLE)
