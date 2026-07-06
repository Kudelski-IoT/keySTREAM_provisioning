# KTA Bridge MCU Build System

## Overview

This build system compiles the KTA bridge for various **OS types**, **transport backends**, and **hardware platforms** using a single Makefile.

Configuration is read from platform-specific header files in:
```
backends/<backend>/sal/<platform>/<backend>_config.h
```

## Quick Start

### Build Examples

```bash
# ESP32 with FreeRTOS and UART
make OS=freertos BACKEND=uart PLATFORM=esp32

# Nordic nRF52 with FreeRTOS and BLE
make OS=freertos BACKEND=ble PLATFORM=nordic

# Linux with UART (host testing)
make OS=linux BACKEND=uart PLATFORM=linux

# Bare metal STM32 with USB
make OS=bare_metal BACKEND=usb PLATFORM=stm32

# Clean build
make clean
```

### Predefined Targets

```bash
make esp32-freertos-uart   # ESP32 + FreeRTOS + UART
make esp32-freertos-ble    # ESP32 + FreeRTOS + BLE
make linux-uart            # Linux + UART
make nordic-ble            # Nordic + FreeRTOS + BLE
make sg41-usb              # SG41 + Bare Metal + USB
```

## Build Parameters

### OS Types
- `bare_metal` - No RTOS, polling-based
- `freertos` - FreeRTOS task-based
- `linux` - Linux pthread-based

### Transport Backends
- `uart` - UART/Serial transport
- `ble` - Bluetooth Low Energy
- `usb` - USB CDC/Bulk transport
- `zigbee` - Zigbee wireless

### Hardware Platforms
- `esp32` - Espressif ESP32 (FreeRTOS/ESP-IDF)
- `sg41` - Silicon Labs EFR32xG41
- `nordic` - Nordic nRF52/nRF53
- `stm32` - STM32 (various models)
- `microchip` - Microchip SAME54
- `linux` - Linux host (for testing)

## Configuration

### Platform-Specific Configuration

Edit the config file for your platform:

```
backends/uart/sal/esp32/uart_config.h      # ESP32 UART config
backends/ble/sal/nordic/ble_config.h       # Nordic BLE config
backends/usb/sal/sg41/usb_config.h         # SG41 USB config
```

Example `uart_config.h`:
```c
#define UART_PORT_NUM          2
#define UART_BAUD_RATE         921600
#define UART_DATA_BITS         8
#define UART_STOP_BITS         1
#define UART_PARITY            0
#define UART_FLOW_CONTROL      false
#define UART_RX_BUFFER_SIZE    2048
#define UART_TX_BUFFER_SIZE    2048
```

### Main Application Configuration

Each OS type has its own main file:
```
examples/platform/bare_metal/main_bare_metal.c
examples/platform/freertos/main_freertos.c
examples/platform/linux/main_linux.c
```

Edit `BRIDGE_TRANSPORT_TYPE` in the main file:
```c
#define BRIDGE_TRANSPORT_TYPE    TRANSPORT_TYPE_UART
```

## Build Output

```
build/
  ├── freertos_uart_esp32/
  │   ├── *.o
  │   └── kta_bridge_freertos_uart_esp32
  ├── linux_ble_nordic/
  │   └── kta_bridge_linux_ble_nordic
  └── ...
```

## Architecture

```
Application (main_*.c)
    ↓
Bridge Integration (bridge_integration.c)
    ↓
Protocol Layer (transport_interface.c) - TLV serialization
    ↓
HAL Layer (transport_hal_mcu.c) - Backend selection
    ↓
Backend Layer (transport_<backend>_mcu.c) - Transport wrapper
    ↓
SAL Layer (<backend>_sal_<platform>.c) - Platform driver
    ↓
Hardware
```

## File Organization

```
mcu/
├── Makefile                    # Main build file
├── build_config.mk             # Platform-specific toolchain config
├── BUILD.md                    # This file
│
├── examples/
│   ├── common/
│   │   ├── bridge_integration.h    # OS-independent API
│   │   └── bridge_integration.c    # OS-independent impl
│   └── platform/
│       ├── bare_metal/
│       │   └── main_bare_metal.c
│       ├── freertos/
│       │   └── main_freertos.c
│       └── linux/
│           └── main_linux.c
│
├── backends/
│   ├── transport_hal_mcu.h/c       # HAL dispatcher
│   ├── uart/
│   │   ├── transport_uart_mcu.c    # UART backend
│   │   ├── uart_sal.h              # UART SAL interface
│   │   └── sal/
│   │       ├── esp32/
│   │       │   ├── uart_config.h   # ← USER EDITS THIS
│   │       │   └── uart_sal_esp32.c
│   │       └── sg41/
│   │           ├── uart_config.h   # ← USER EDITS THIS
│   │           └── uart_sal_sg41.c
│   ├── ble/
│   │   └── sal/...
│   ├── usb/
│   │   └── sal/...
│   └── zigbee/
│       └── sal/...
│
└── bridgeKta/
    ├── bridge_kta.h
    └── bridge_kta.c                # KTA command handlers
```

## Dependencies

### For ESP32 (FreeRTOS)
- ESP-IDF v4.4+ installed
- `ESP_IDF_PATH` environment variable set
- Run: `source $IDF_PATH/export.sh`

### For Nordic (FreeRTOS)
- nRF5 SDK or nRF Connect SDK
- ARM GCC toolchain: `arm-none-eabi-gcc`

### For Linux
- GCC with pthread support
- Standard build tools

### For Bare Metal (STM32/Microchip/etc.)
- ARM GCC toolchain: `arm-none-eabi-gcc`
- Platform SDK/HAL libraries

## Advanced Usage

### Custom Build Directory
```bash
make BUILD_DIR=custom_build OS=freertos BACKEND=uart PLATFORM=esp32
```

### Debug Build
```bash
make BUILD_TYPE=debug OS=linux BACKEND=uart PLATFORM=linux
```

### Release Build (Optimized)
```bash
make BUILD_TYPE=release OS=freertos BACKEND=ble PLATFORM=nordic
```

### Verbose Logging
```bash
make VERBOSE_LOG=1 OS=freertos BACKEND=uart PLATFORM=esp32
```

### Cross-Compilation
```bash
make CROSS_COMPILE=arm-none-eabi- OS=bare_metal BACKEND=usb PLATFORM=stm32
```

## Troubleshooting

### "ESP_IDF_PATH not set"
```bash
source ~/esp/esp-idf/export.sh
```

### "arm-none-eabi-gcc not found"
Install ARM GCC toolchain:
```bash
# Ubuntu/Debian
sudo apt-get install gcc-arm-none-eabi

# macOS
brew install arm-none-eabi-gcc
```

### "Cannot find config file"
Ensure the platform config exists:
```bash
ls backends/uart/sal/esp32/uart_config.h
```

### Compilation Errors
Check:
1. Platform config syntax
2. Toolchain installation
3. Include paths in `build_config.mk`
4. KTA library path

## Adding New Platforms

1. Create SAL directory:
```bash
mkdir -p backends/uart/sal/myplatform
```

2. Create config header:
```bash
cp backends/uart/sal/esp32/uart_config.h \
   backends/uart/sal/myplatform/uart_config.h
```

3. Create SAL implementation:
```bash
cp backends/uart/sal/esp32/uart_sal_esp32.c \
   backends/uart/sal/myplatform/uart_sal_myplatform.c
```

4. Update SAL implementation for your platform

5. Build:
```bash
make OS=freertos BACKEND=uart PLATFORM=myplatform
```

## Integration with IDEs

### VS Code
Add to `.vscode/tasks.json`:
```json
{
    "label": "Build ESP32 UART",
    "type": "shell",
    "command": "make",
    "args": ["OS=freertos", "BACKEND=uart", "PLATFORM=esp32"],
    "problemMatcher": ["$gcc"]
}
```

### Eclipse
Import as Makefile project, set build command:
```
make OS=freertos BACKEND=uart PLATFORM=esp32
```

### CLion
Add custom build target with command:
```
make OS=freertos BACKEND=uart PLATFORM=esp32
```

## Performance Notes

- **Bare Metal**: Smallest footprint, fastest response
- **FreeRTOS**: Balanced performance, good for real-time
- **Linux**: Easiest for testing, not for production embedded

## License

See main project LICENSE file.

## Support

For issues or questions, refer to the main project documentation.
