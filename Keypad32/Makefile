#====================================================================================
# User editable area
#====================================================================================

ESP_ROOT	= $(HOME)/src/github/esp32/arduino-esp32
ESP_LIBS	= $(ESP_ROOT)/libraries
BUILD_ROOT	= build
LIBCPP		= -lstdc++
CHIP		= esp32
HWTYPE		= nodemcu-32s

# ESP_ROOT=/home/danny/.arduino15/packages/esp8266/hardware/esp8266/2.3.0
# ESP_LIBS=$(ESP_ROOT)/libraries
# BUILD_ROOT = build
# LIBCPP=-lstdc++
# HWTYPE=	d1_mini

# SKETCH		= $(HOME)/src/sketchbook/esp8266/Alarm/Controller32/Controller.ino
# EXTRA_SRC	= Alarm.cpp Config.cpp Peers.cpp ThingSpeakLogger.cpp \
# 		  Siren.cpp Sensors.cpp Rfid.cpp

SKETCH		= $(HOME)/src/sketchbook/esp8266/Alarm/Keypad32/Keypad.ino
EXTRA_SRC	= Alarm.cpp Config.cpp Peers.cpp ThingSpeakLogger.cpp \
		  Oled.cpp Clock.cpp Siren.cpp Rfid.cpp \
		  BackLight.cpp Sensors.cpp Weather.cpp \
		  lzw.c libnsgif.c LoadGif.cpp mostlycloudy.c

UPLOAD_AVAHI_NAME = ESP32_Prototype.local

ALARMLIBS = $(HOME)/src/sketchbook/esp8266/Alarm/libraries
MYLIBS = $(HOME)/src/sketchbook/esp8266/libraries

LIBS =	$(ESP_LIBS)/WiFi \
	$(ESP_LIBS)/SPI \
	$(ESP_LIBS)/Wire \
	$(ESP_LIBS)/FS \
	$(ESP_LIBS)/SPIFFS \
	$(ESP_LIBS)/ArduinoOTA \
	$(ESP_LIBS)/Update \
	$(ESP_LIBS)/ESPmDNS \
	$(MYLIBS)/Timezone \
	${MYLIBS}/pubsubclient \
	${MYLIBS}/Timezone \
	${ALARMLIBS}/TFT_eSPI \
	${ALARMLIBS}/rcswitch \
	${ALARMLIBS}/arduinojson \
	${ALARMLIBS}/rfid \
	${ALARMLIBS}/thingspeak

#	${ALARMLIBS}/tzstuff \
#=== Project specific definitions: sketch and list of needed libraries

# Board definitions
FLASH_SIZE ?= 4MB
FLASH_MODE ?= dio
FLASH_FREQ ?= 80m
# FLASH_LAYOUT ?= eagle.flash.4m.ld
FLASH_LAYOUT ?= esp32_out.ld

# Upload parameters
UPLOAD_SPEED ?= 921600
UPLOAD_PORT ?= /dev/ttyUSB0
UPLOAD_VERB ?= -vv
UPLOAD_AVAHI_NAME ?= esp8266
UPLOAD_AVAHI_IP = $(shell avahi-resolve-host-name $(UPLOAD_AVAHI_NAME) | awk '{print $$2;}')
UPLOAD_AVAHI_PORT ?= 3232
UPLOAD_OTA_TOOL = python $(TOOLS_ROOT)/espota.py

#====================================================================================
# The area below should normally not need to be edited
#====================================================================================

MKESPARD_VERSION = 1.0.0

START_TIME := $(shell perl -e "print time();")
# Main output definitions
MAIN_NAME = $(basename $(notdir $(SKETCH)))
MAIN_EXE = $(BUILD_ROOT)/$(MAIN_NAME).bin
MAIN_ELF = $(OBJ_DIR)/$(MAIN_NAME).elf
SRC_GIT_VERSION = $(call git_description,$(dir $(SKETCH)))

# esp32 arduino directories
ESP_GIT_VERSION = $(call git_description,$(ESP_ROOT))
ESP_LIBS = $(ESP_ROOT)/libraries
TOOLS_ROOT = $(ESP_ROOT)/tools
TOOLS_BIN = $(TOOLS_ROOT)/xtensa-esp32-elf/bin
SDK_ROOT = $(ESP_ROOT)/tools/sdk
LWIP_SDK_ROOT = $(ESP_ROOT)/tools/sdk/lwip
ESPTOOLS_BIN = $(ESP_ROOT)/tools/sdk/bin

# Directory for intermedite build files
OBJ_DIR = $(BUILD_ROOT)/obj
OBJ_EXT = .o
DEP_EXT = .d

# Compiler definitions
CC = xtensa-esp32-elf-gcc
CPP = xtensa-esp32-elf-g++
LD =  $(CC)
AR = xtensa-esp32-elf-ar
ESP_TOOL = /home/danny/src/arduino/arduino-1.8.5/hardware/espressif/esp32/tools/esptool.py
GEN_PART = /home/danny/src/arduino/arduino-1.8.5/hardware/espressif/esp32/tools/gen_esp32part.py

# DEBUG=-g
# DEBUG=-Os -g
DEBUG	= -Os -g3

INCLUDE_DIRS += $(SDK_ROOT)/include ${LWIP_SDK_ROOT}/include $(CORE_DIR) $(ESP_ROOT)/variants/${HWTYPE} $(OBJ_DIR) ${EXTRA_INCLUDE} $(SDK_INC)

C_DEFINES	= -D__ets__ -DICACHE_FLASH -U__STRICT_ANSI__ \
		  -DARDUINO=10805 -DARDUINO_ARCH_ESP32  -DESP32 \
		  -DCORE_DEBUG_LEVEL=0 \
		  -DF_CPU=240000000L \
		  -DARDUINO_MH_ET_LIVE_ESP32MINIKIT \
		  -MMD

C_INCLUDES = -I.. $(foreach dir,$(INCLUDE_DIRS) $(USER_DIRS),-I$(dir))
C_FLAGS ?= -c ${DEBUG} -std=gnu99 \
	   -fno-exceptions -Wpointer-arith -ffunction-sections -fdata-sections \
	   -fstrict-volatile-bitfields -mlongcalls -nostdlib -w -Wno-error=unused-function \
	   -Wno-error=unused-but-set-variable -Wno-error=unused-variable \
	   -Wno-error=deprecated-declarations -Wno-unused-parameter -Wno-sign-compare \
	   -fno-rtti  -w
CPP_FLAGS ?= -c ${DEBUG} -std=gnu++11 -fno-exceptions -Os -g3 -Wpointer-arith \
	     -ffunction-sections -fdata-sections -fstrict-volatile-bitfields -mlongcalls \
	     -nostdlib -w -Wno-error=unused-function -Wno-error=unused-but-set-variable \
	     -Wno-error=unused-variable -Wno-error=deprecated-declarations \
	     -Wno-unused-parameter -Wno-sign-compare -fno-rtti -w -x c++
S_FLAGS ?= -c ${DEBUG} -x assembler-with-cpp -MMD -mlongcalls
LD_FLAGS ?= -w ${DEBUG} -nostdlib -Wl,--no-check-sections -u call_user_start \
	    -Wl,-static -L$(SDK_ROOT)/lib -L$(SDK_ROOT)/ld \
	    -T$(FLASH_LAYOUT) -T esp32.common.ld -T esp32.rom.ld -T esp32.peripherals.ld \
	    -T esp32.rom.spiram_incompatible_fns.ld -u ld_include_panic_highint_hdl \
	    -u call_user_start_cpu0 \
	    -Wl,--gc-sections -Wl,-static -Wl,--undefined=uxTopUsedPriority \
	    -u __cxa_guard_dummy 

LD_STD_LIBS ?= -lgcc -lcxx -lstdc++ -lapp_trace -lapp_update -lbootloader_support \
	       -lbt -lbtdm_app -lc -lc_nano -lcoap -lcoexist -lconsole -lcore -ldriver \
	       -lesp32 -lesp_adc_cal -lespnow -lethernet -lexpat -lfatfs -lfreertos -lhal \
	       -lheap -ljsmn -ljson -llog -llwip -lm -lmbedtls -lmdns -lmicro-ecc -lnet80211 \
	       -lnewlib -lnghttp -lnvs_flash -lopenssl -lphy -lpp -lpthread -lrtc \
	       -lsdmmc -lsmartconfig -lsoc -lspi_flash -lspiffs -ltcpip_adapter \
	       -lulp -lvfs -lwear_levelling -lwpa -lwpa2 -lwpa_supplicant \
	       -lwps -lxtensa-debug-module ${LIBCPP}

# Core source files
CORE_DIR = $(ESP_ROOT)/cores/$(CHIP)
CORE_SRC = $(shell find $(CORE_DIR) -name "*.S" -o -name "*.c" -o -name "*.cpp")
CORE_OBJ = $(patsubst %,$(OBJ_DIR)/%$(OBJ_EXT),$(notdir $(CORE_SRC)))
CORE_LIB = $(OBJ_DIR)/core.ar

SDK_DIR = ${ESP_ROOT}/tools/sdk/include
SDK_INC = $(sort $(dir $(shell find $(SDK_DIR) -name "*.h"))) /home/danny/src/github/esp32/arduino-esp32/tools/sdk/include/soc /home/danny/src/github/esp32/arduino-esp32/tools/sdk/include/freertos /home/danny/src/github/esp32/arduino-esp32/tools/sdk/include/driver

# User defined compilation units
USER_SRC = $(SKETCH) ${EXTRA_SRC} $(shell find $(LIBS) -name examples -prune -false -o -name "*.cpp" -o -name "*.c")
USER_SRC2 = $(SKETCH) ${EXTRA_SRC} $(shell find $(LIBS) -name examples -prune -false -o -name "*.cpp" -o -name "*.c" -o -name "*.h")
# Object file suffix seems to be significant for the linker...
USER_OBJ = $(subst .ino,.cpp,$(patsubst %,$(OBJ_DIR)/%$(OBJ_EXT),$(notdir $(USER_SRC))))
USER_DIRS = $(sort $(dir $(USER_SRC2)))

VPATH += $(shell find $(CORE_DIR) -type d) $(USER_DIRS)

# Automatically generated build information data
# Makes the build date and git descriptions at the actual build
# event available as string constants in the program
BUILD_INFO_H = $(OBJ_DIR)/buildinfo.h
BUILD_INFO_CPP = $(OBJ_DIR)/buildinfo.cpp
BUILD_INFO_OBJ = $(BUILD_INFO_CPP)$(OBJ_EXT)

$(BUILD_INFO_H): | $(OBJ_DIR)
	echo "typedef struct { const char *date, *time, *src_version, *env_version;} _tBuildInfo; extern _tBuildInfo _BuildInfo;" >$@

# Utility functions
git_description = $(shell git -C  $(1) describe --tags --always --dirty 2>/dev/null)
time_string = $(shell perl -e 'use POSIX qw(strftime); print strftime($(1), localtime());')
MEM_USAGE = \
  'while (<>) { \
      $$r += $$1 if /^\.(?:data|rodata|bss)\s+(\d+)/;\
		  $$f += $$1 if /^\.(?:irom0\.text|text|data|rodata)\s+(\d+)/;\
	 }\
	 print "\nMemory usage\n";\
	 print sprintf("  %-6s %6d bytes\n" x 2 ."\n", "Ram:", $$r, "Flash:", $$f);'

# Build rules
$(OBJ_DIR)/%.cpp$(OBJ_EXT): %.cpp $(BUILD_INFO_H)
	echo C++-Compile $(<F)
	$(CPP) $(C_DEFINES) $(C_INCLUDES) $(CPP_FLAGS) $< -o $@

$(OBJ_DIR)/%.cpp$(OBJ_EXT): %.ino $(BUILD_INFO_H)
	echo C++-Compile $(<F)
	$(CPP) -x c++ -include $(CORE_DIR)/Arduino.h $(C_DEFINES) $(C_INCLUDES) $(CPP_FLAGS) $< -o $@

$(OBJ_DIR)/%.c$(OBJ_EXT): %.c
	echo Compile $(<F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@

$(OBJ_DIR)/%.S$(OBJ_EXT): %.S
	echo Compile $(<F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(S_FLAGS) $< -o $@

$(CORE_LIB): $(CORE_OBJ)
	echo  Creating core archive
	rm -f $@
	$(AR) cru $@  $^

BUILD_DATE = $(call time_string,"%Y-%m-%d")
BUILD_TIME = $(call time_string,"%H:%M:%S")

$(MAIN_EXE): $(CORE_LIB) $(USER_OBJ)
	echo Linking $(MAIN_EXE)
	echo "  Versions: $(SRC_GIT_VERSION), $(ESP_GIT_VERSION)"
	echo 	'#include <buildinfo.h>' >$(BUILD_INFO_CPP)
	echo '_tBuildInfo _BuildInfo = {"$(BUILD_DATE)","$(BUILD_TIME)","$(SRC_GIT_VERSION)","$(ESP_GIT_VERSION)"};' >>$(BUILD_INFO_CPP)
	$(CPP) $(C_DEFINES) $(C_INCLUDES) $(CPP_FLAGS) $(BUILD_INFO_CPP) -o $(BUILD_INFO_OBJ)
	$(LD) $(LD_FLAGS) -Wl,--start-group $^ $(BUILD_INFO_OBJ) $(LD_STD_LIBS) -Wl,--end-group -L$(OBJ_DIR) -o $(MAIN_ELF)
	python $(GEN_PART) -q $(TOOLS_ROOT)/partitions/default.csv $(SKETCH).partitions.bin
	python $(ESP_TOOL) --chip esp32 elf2image --flash_mode $(FLASH_MODE) --flash_freq $(FLASH_FREQ) --flash_size $(FLASH_SIZE) -o $@ $(MAIN_ELF)
	xtensa-esp32-elf-size -A $(MAIN_ELF) | perl -e $(MEM_USAGE)
	perl -e 'print "Build complete. Elapsed time: ", time()-$(START_TIME),  " seconds\n\n"'

upload: all
	python $(ESP_TOOL) --chip esp32 --port $(UPLOAD_PORT) --baud $(UPLOAD_SPEED) \
	--before default_reset --after hard_reset write_flash -z --flash_mode dio \
	--flash_freq 80m --flash_size detect \
	0xe000 ${TOOLS_ROOT}/partitions/boot_app0.bin \
	0x1000 ${ESPTOOLS_BIN}/bootloader_dio_80m.bin \
	0x10000 $(MAIN_EXE) \
	0x8000 $(SKETCH).partitions.bin

run:	upload
	miniterm.py ${UPLOAD_PORT} 115200

ota: all showotaip
	$(UPLOAD_OTA_TOOL) -i $(UPLOAD_AVAHI_IP) -p $(UPLOAD_AVAHI_PORT) -f $(MAIN_EXE)

showotaip:
	echo "IP Address for OTA is $(UPLOAD_AVAHI_IP) (symbol $(UPLOAD_AVAHI_NAME))"

clean:
	echo Removing all intermediate build files...
	rm -rf $(BUILD_ROOT)
	rm -f *~ *.bak *-

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# all::
# 	echo "USER_SRC = " $(USER_SRC)
# 	echo "USER_DIRS = " $(USER_DIRS)

# .PHONY: all
all:: $(OBJ_DIR) $(BUILD_INFO_H) $(MAIN_EXE)

# Include all available dependencies
-include $(wildcard $(OBJ_DIR)/*$(DEP_EXT))

.DEFAULT_GOAL = all

ifndef VERBOSE
# Set silent mode as default
MAKEFLAGS += --silent
endif
