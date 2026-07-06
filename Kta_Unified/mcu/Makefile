################################################################################
# KTA Bridge MCU Makefile
# 
# This Makefile builds the KTA bridge for various platforms, OS types, and 
# transport backends.
#
# Usage:
#   make OS=bare_metal BACKEND=uart PLATFORM=esp32
#   make OS=freertos BACKEND=ble PLATFORM=nordic
#   make OS=linux BACKEND=uart PLATFORM=sg41
#   make clean
#
# Configuration is read from platform-specific config headers:
#   backends/<backend>/sal/<platform>/<backend>_config.h
################################################################################

# ============================================================================
# Build Configuration (User-settable)
# ============================================================================

# Operating System Type
# Options: bare_metal, freertos, linux
OS ?= freertos

# Transport Backend
# Options: uart, ble, usb, zigbee
BACKEND ?= uart

# Hardware Platform
# Options: esp32, sg41, nordic, stm32, microchip
PLATFORM ?= esp32

# Build Type
# Options: debug, release
BUILD_TYPE ?= debug

# Output directory
BUILD_DIR = build/$(OS)_$(BACKEND)_$(PLATFORM)

# ============================================================================
# Toolchain Configuration
# ============================================================================

# Cross-compiler prefix (override for embedded targets)
CROSS_COMPILE ?=

CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
SIZE = $(CROSS_COMPILE)size

# ============================================================================
# Compiler Flags
# ============================================================================

# Common flags
CFLAGS = -Wall -Wextra -std=c11
CFLAGS += -I.
CFLAGS += -Ibackends
CFLAGS += -Iexamples/common
CFLAGS += -Iexamples/platform
CFLAGS += -IbridgeKta

# Build type specific flags
ifeq ($(BUILD_TYPE),debug)
    CFLAGS += -g -O0 -DDEBUG
else
    CFLAGS += -O2 -DNDEBUG
endif

# OS-specific flags
ifeq ($(OS),bare_metal)
    CFLAGS += -DOS_BARE_METAL
    EXAMPLE_DIR = examples/platform/bare_metal
    INTEGRATION_SUFFIX = baremetal
else ifeq ($(OS),freertos)
    CFLAGS += -DOS_FREERTOS -DESP_PLATFORM
    CFLAGS += -I$(ESP_IDF_PATH)/components/freertos/include
    CFLAGS += -I$(ESP_IDF_PATH)/components/freertos/FreeRTOS-Kernel/include
    EXAMPLE_DIR = examples/platform/freertos
    INTEGRATION_SUFFIX = freertos
else ifeq ($(OS),linux)
    CFLAGS += -DOS_LINUX -pthread
    LDFLAGS += -pthread
    EXAMPLE_DIR = examples/platform/linux
    INTEGRATION_SUFFIX = linux
endif

# Backend-specific flags
CFLAGS += -DBACKEND_$(shell echo $(BACKEND) | tr '[:lower:]' '[:upper:]')

# Platform-specific flags
CFLAGS += -DPLATFORM_$(shell echo $(PLATFORM) | tr '[:lower:]' '[:upper:]')

# Include path for selected backend's config
BACKEND_SAL_PATH = backends/$(BACKEND)/sal/$(PLATFORM)
CFLAGS += -I$(BACKEND_SAL_PATH)

# Include path for selected backend
CFLAGS += -Ibackends/$(BACKEND)

# Include path for the keySTREAM Trusted Agent (KTA) library headers.
# bridgeKta/bridge_kta.c includes "k_kta.h", which ships with the KTA library
# (kta_lib SOURCE/include) and NOT with this package. Point KTA_LIB_INC at that
# folder, e.g.:
#   make OS=freertos BACKEND=uart PLATFORM=esp32 \
#        KTA_LIB_INC=/path/to/kta_lib/SOURCE/include
KTA_LIB_INC ?=
ifneq ($(strip $(KTA_LIB_INC)),)
    CFLAGS += -I$(KTA_LIB_INC)
endif

# ============================================================================
# Source Files
# ============================================================================

# Bridge integration layer
BRIDGE_INTEGRATION_SRCS = examples/common/bridge_integration.c

# Bridge KTA layer
BRIDGE_KTA_SRCS = bridgeKta/bridge_kta.c

# Backend Interface layer
BACKEND_INTERFACE_SRCS = backends/backend_interface.c

# Backend layer (selected based on BACKEND variable)
BACKEND_SRCS = backends/$(BACKEND)/backend_$(BACKEND).c

# SAL layer (selected based on BACKEND and PLATFORM)
SAL_SRCS = $(BACKEND_SAL_PATH)/$(BACKEND)_sal.c

# Example application + OS integration entry (selected based on OS)
EXAMPLE_SRCS = $(EXAMPLE_DIR)/main_$(OS).c \
               $(EXAMPLE_DIR)/kta_mcu_integration_$(INTEGRATION_SUFFIX).c

# All sources
SRCS = $(BRIDGE_INTEGRATION_SRCS) \
       $(BRIDGE_KTA_SRCS) \
       $(BACKEND_INTERFACE_SRCS) \
       $(BACKEND_SRCS) \
       $(SAL_SRCS) \
       $(EXAMPLE_SRCS)

# Object files
OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(SRCS)))

# Output binary
TARGET = $(BUILD_DIR)/kta_bridge_$(OS)_$(BACKEND)_$(PLATFORM)

# ============================================================================
# Build Rules
# ============================================================================

.PHONY: all clean info help

all: info $(TARGET)

# Link target
$(TARGET): $(OBJS)
	@echo "Linking: $@"
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) -o $@ $^
	$(SIZE) $@
	@echo "Build complete: $@"

# Compile sources
$(BUILD_DIR)/%.o: examples/common/%.c
	@echo "Compiling: $<"
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: bridgeKta/%.c
	@echo "Compiling: $<"
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: backends/%.c
	@echo "Compiling: $<"
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: backends/$(BACKEND)/%.c
	@echo "Compiling: $<"
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(BACKEND_SAL_PATH)/%.c
	@echo "Compiling: $<"
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(EXAMPLE_DIR)/%.c
	@echo "Compiling: $<"
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	@echo "Cleaning build directory..."
	rm -rf build
	@echo "Clean complete"

# Build info
info:
	@echo "════════════════════════════════════════════════════════════"
	@echo "  KTA Bridge Build Configuration"
	@echo "════════════════════════════════════════════════════════════"
	@echo "  OS:           $(OS)"
	@echo "  Backend:      $(BACKEND)"
	@echo "  Platform:     $(PLATFORM)"
	@echo "  Build Type:   $(BUILD_TYPE)"
	@echo "  Config Path:  $(BACKEND_SAL_PATH)"
	@echo "  Output:       $(TARGET)"
	@echo "════════════════════════════════════════════════════════════"
	@echo ""

# Help
help:
	@echo "KTA Bridge Makefile"
	@echo ""
	@echo "Usage:"
	@echo "  make [OPTIONS]"
	@echo ""
	@echo "Options:"
	@echo "  OS=<os>             Operating system (bare_metal, freertos, linux)"
	@echo "  BACKEND=<backend>   Transport backend (uart, ble, usb, zigbee)"
	@echo "  PLATFORM=<platform> Hardware platform (esp32, sg41, nordic, etc.)"
	@echo "  BUILD_TYPE=<type>   Build type (debug, release)"
	@echo ""
	@echo "Examples:"
	@echo "  make OS=freertos BACKEND=uart PLATFORM=esp32"
	@echo "  make OS=linux BACKEND=ble PLATFORM=nordic BUILD_TYPE=release"
	@echo "  make OS=bare_metal BACKEND=usb PLATFORM=stm32"
	@echo "  make clean"
	@echo ""
	@echo "Configuration:"
	@echo "  Edit platform-specific config files:"
	@echo "    backends/<backend>/sal/<platform>/<backend>_config.h"
	@echo ""

# ============================================================================
# Predefined Build Targets (Common Configurations)
# ============================================================================

.PHONY: esp32-freertos-uart esp32-freertos-ble linux-uart nordic-ble

esp32-freertos-uart:
	$(MAKE) OS=freertos BACKEND=uart PLATFORM=esp32

esp32-freertos-ble:
	$(MAKE) OS=freertos BACKEND=ble PLATFORM=esp32

linux-uart:
	$(MAKE) OS=linux BACKEND=uart PLATFORM=esp32

nordic-ble:
	$(MAKE) OS=freertos BACKEND=ble PLATFORM=nordic

sg41-usb:
	$(MAKE) OS=bare_metal BACKEND=usb PLATFORM=sg41

# ============================================================================
# Dependencies (auto-generated)
# ============================================================================

-include $(OBJS:.o=.d)

# Generate dependencies
$(BUILD_DIR)/%.d: %.c
	@mkdir -p $(BUILD_DIR)
	@$(CC) $(CFLAGS) -MM -MT $(BUILD_DIR)/$*.o $< > $@
