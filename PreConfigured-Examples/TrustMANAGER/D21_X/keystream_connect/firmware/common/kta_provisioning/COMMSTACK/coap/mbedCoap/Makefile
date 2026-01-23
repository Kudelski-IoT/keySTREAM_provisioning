CFG ?= GCC_CFG

ifeq ($(CFG),INFENION_CFG)
CROSS_COMPILE = /cygdrive/c/Infineon/Tools/ModusToolbox/tools_2.3/gcc/bin/arm-none-eabi-
endif

ifeq ($(CFG),SILAB_CFG)
CROSS_COMPILE = /cygdrive/c/SiLabTool/gcc-arm-none-eabi-10-2020-q4-major/bin/arm-none-eabi-
endif

ifeq ($(CFG),MICROCHIP_CFG)
CROSS_COMPILE = /cygdrive/c/Program\ Files/Microchip/xc32/v4.60/bin/xc32-
endif

ifeq ($(CFG),MUELLER_CFG)
CROSS_COMPILE = /cygdrive/c/ncs/toolchains/v2.1.0/opt/zephyr-sdk/arm-zephyr-eabi/bin/arm-zephyr-eabi-
endif

CC:= $(CROSS_COMPILE)gcc
CFLAGS := -Wall

ifeq ($(CFG),INFENION_CFG)
CFLAGS += -c -mcpu=cortex-m4 --specs=nano.specs -Og -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -mthumb -ffunction-sections -fdata-sections -ffat-lto-objects
endif

ifeq ($(CFG),SILAB_CFG)
CFLAGS += -c -mcpu=cortex-m33 --specs=nano.specs -o -mthumb -std=c99 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -fmessage-length=0 -ffunction-sections -fdata-sections
endif

ifeq ($(CFG),GCC_CFG)
CFLAGS += -g -fPIC
endif

CFLAGS += -D SN_COAP_BLOCKWISE_ENABLED -D SN_COAP_RESENDING_QUEUE_SIZE_MSGS=5 -D SN_COAP_DUPLICATION_MAX_MSGS_COUNT=5

AR:= $(CROSS_COMPILE)ar

LIB :=libmcoap.a
INC := -I source/include -I mbed-coap -I ../ -I . -I nanostack/mbed-client-libservice

SRCS := \
	source/sn_coap_parser.c \
	source/sn_coap_builder.c \
	source/sn_coap_protocol.c \
	source/sn_coap_header_check.c \
	nanostack/source/libList/ns_list.c

SOURCES := $(wildcard $(SRCS))
OBJECTS := $(patsubst %.c,%.o,$(SOURCES))

CFLAGS +=$(INC)

.PHONY: all

all: $(LIB)

$(LIB): $(OBJECTS)
	$(AR) -r $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -Os -c $^ -o $@

clean:
	rm -f $(OBJECTS) $(LIB)


