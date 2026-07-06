CFG ?= GCC_CFG

ifeq ($(CFG),INFENION_CFG)
CROSS_COMPILE = /cygdrive/c/Infineon/Tools/ModusToolbox/tools_2.3/gcc/bin/arm-none-eabi-
endif

ifeq ($(CFG),SILAB_CFG)
CROSS_COMPILE = /cygdrive/c/SiLabTool/gcc-arm-none-eabi-10-2020-q4-major/bin/arm-none-eabi-endif
endif

ifeq ($(CFG),MICROCHIP_CFG)
CROSS_COMPILE = /cygdrive/c/Program\ Files/Microchip/xc32/v4.60/bin/xc32-
endif

# TODO:Add for STM

ifeq ($(CFG),MUELLER_CFG)
CROSS_COMPILE = /cygdrive/c/ncs/toolchains/v2.1.0/opt/zephyr-sdk/arm-zephyr-eabi/bin/arm-zephyr-eabi-
endif


CC:= $(CROSS_COMPILE)gcc

ifeq ($(CFG),GCC_CFG)
CFLAGS := -Wall -fstack-usage
else
CFLAGS := -Wall
endif
#CFLAGS := -g
CFLAGS += -D $(CFG)

ifeq ($(CFG),INFENION_CFG)
CFLAGS += -c -mcpu=cortex-m4 --specs=nano.specs -Og -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -mthumb -ffunction-sections -fdata-sections -ffat-lto-objects
endif

ifeq ($(CFG),SILAB_CFG)
CFLAGS += -c -mcpu=cortex-m33 --specs=nano.specs -o -mthumb -std=c99 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -fmessage-length=0 -ffunction-sections -fdata-sections
endif

ifeq ($(CFG),MICROCHIP_CFG)
CFLAGS += -O0
endif

ifeq ($(CFG),MUELLER_CFG)
CFLAGS += -O0
endif

AR:= $(CROSS_COMPILE)ar

LIB :=libcommstack.a

INC += -I mbedCoap/source/include \
       -I mbedCoap/mbed-coap \
       -I salapi/include \
       -I mbedCoap/nanostack/mbed-client-libservice \
       -I mbedCoap \
	   -I ../include \
	   -I ./include \
	   -I ./ \
       -I ../../../COMMON/salapi/include 

ifeq ($(CFG),INFENION_CFG)
INC += -I ../../../SOURCE/salapi/ifx/mtb_shared/lwip/STABLE-2_1_2_RELEASE/src/include/compat/posix \
       -I ../../../SOURCE/salapi/ifx/mtb_shared/lwip/STABLE-2_1_2_RELEASE/src/include \
       -I ../../../SOURCE/salapi/ifx/mtb_shared/wifi-mw-core/release-v3.1.0/configs \
       -I ../../../SOURCE/salapi/ifx/mtb_shared/wifi-host-driver/release-v1.94.0/WiFi_Host_Driver/inc \
       -I ../../../SOURCE/salapi/ifx/mtb_shared/wifi-host-driver/release-v1.94.0/External/hal \
       -I ../../../SOURCE/salapi/ifx/mtb_shared/wifi-mw-core/release-v3.1.0/lwip-whd-port/COMPONENT_FREERTOS
# Added for infineon
endif

SRCS := \
	comm_interface.c \
	randLIB.c \
	comm_interface_util.c \
	comm_if.c

SOURCES := $(wildcard $(SRCS))
OBJECTS := $(patsubst %.c,%.o,$(SOURCES))

SUBDIR:= mbedCoap
SUBDIR1:= salapi
DPLIBS:= mbedCoap/libmcoap.a
DPLIBS1:= salapi/libsal.a

ifeq ($(DEBUG),1)
# Enabling debugs
	CFLAGS += -D ENABLE_COMM_COAP_PACKET_DEBUG_PRINTS -D ENABLE_COMM_DEBUG_PRINTS
endif

ifeq ($(COVERAGE),1)
# Enabling code coverage
	CFLAGS += -fprofile-arcs -ftest-coverage
endif

CFLAGS +=$(INC)

.PHONY: all

all: $(LIB)

$(LIB): $(OBJECTS)
	$(MAKE) -C $(SUBDIR) DEBUG=$(DEBUG) CFG=$(CFG) 
	$(MAKE) -C $(SUBDIR1) DEBUG=$(DEBUG) CFG=$(CFG) PC=$(PC)
	$(AR) -x $(DPLIBS)
	$(AR) -x $(DPLIBS1)
	$(AR) -rc $@ *.o

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	$(MAKE) -C $(SUBDIR) CFG=$(CFG) clean
	$(MAKE) -C $(SUBDIR1) CFG=$(CFG) clean
	rm -f $(OBJECTS) *.o $(LIB)



