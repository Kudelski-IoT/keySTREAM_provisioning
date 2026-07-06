# KTA Bridge Integration Examples

This directory contains examples showing how to integrate the KTA bridge across different platforms using a clean platform abstraction layer.

## 📂 Directory Structure

```
examples/
├── kta_bridge_example.c          # Common bridge integration code
├── platform/
│   ├── platform_layer.h          # Platform abstraction interface
│   ├── bare_metal/
│   │   ├── main_bare_metal.c     # (Legacy standalone example)
│   │   └── platform_bare_metal.c # Platform implementation
│   ├── freertos/
│   │   ├── main_freertos.c       # (Legacy standalone example)
│   │   └── platform_freertos.c   # Platform implementation
│   └── linux/
│       ├── main_linux.c          # (Legacy standalone example)
│       └── platform_linux.c      # Platform implementation
└── README.md                     # This file
```

## 🏗️ Architecture

The example code uses a two-layer architecture:

### Layer 1: Common Bridge Code (`kta_bridge_example.c`)

Platform-independent functions that work everywhere:

```c
int bridge_init(TransportType type);     // Initialize transport and bridge
int bridge_process(void);                 // Process one command (non-blocking)
int bridge_deinit(void);                  // Cleanup and shutdown
int main(void);                           // Common entry point
```

### Layer 2: Platform Layer (`platform/*.c`)

Each platform implements the interface defined in `platform/platform_layer.h`:

```c
int  platform_init(void);                 // Initialize platform resources
int  platform_start_bridge(void);         // Start bridge (task/thread/loop)
void platform_delay_ms(uint32_t ms);      // Platform-appropriate delay
void platform_log(const char *fmt, ...);  // Platform-specific logging
bool platform_is_running(void);           // Check if system should continue
void platform_deinit(void);               // Cleanup platform resources
```

The common code calls the platform layer through this interface, keeping platform-specific details isolated.

## 🎯 Supported Platforms

### 1. Bare Metal

**Hardware:** Direct hardware access, no OS  
**Processing:** Polling from main loop  
**File:** `platform/bare_metal/platform_bare_metal.c`

**Build:**
```bash
gcc -DPLATFORM_BARE_METAL -I../../common -I../../bridgeKta -I../backends \
    kta_bridge_example.c \
    platform/bare_metal/platform_bare_metal.c \
    ../../common/transport_interface.c \
    ../../bridgeKta/bridge_kta.c \
    ../../backends/uart/transport_uart_mcu.c \
    ../../backends/uart/sal/esp32/uart_sal_esp32.c \
    -o bridge_bare_metal
```

**Integration Pattern:**
```c
/* Main loop continuously polls bridge */
while (1) {
    bridge_process();        /* Non-blocking */
    your_app_code();         /* Your work */
    platform_delay_ms(10);   /* Prevent busy-wait */
}
```

---

### 2. FreeRTOS

**Hardware:** ESP32, STM32, etc. with FreeRTOS  
**Processing:** Dedicated task  
**File:** `platform/freertos/platform_freertos.c`

**Build:**
```bash
# Add to your FreeRTOS/ESP-IDF project:
# SRCS: kta_bridge_example.c, platform/freertos/platform_freertos.c
# CFLAGS: -DPLATFORM_FREERTOS
# Include paths: ../../common, ../../bridgeKta, ../backends
```

**Integration Pattern:**
```c
/* ESP32 app_main() creates task */
void app_main(void) {
    platform_init();
    bridge_init(TRANSPORT_TYPE_UART);
    platform_start_bridge();  /* Creates FreeRTOS task */
    /* Task calls bridge_process() with vTaskDelay() */
}
```

---

### 3. Linux

**Hardware:** PC, Raspberry Pi, embedded Linux  
**Processing:** Separate pthread  
**File:** `platform/linux/platform_linux.c`

**Build:**
```bash
gcc -pthread -DPLATFORM_LINUX -I../../common -I../../bridgeKta -I../backends \
    kta_bridge_example.c \
    platform/linux/platform_linux.c \
    ../../common/transport_interface.c \
    ../../bridgeKta/bridge_kta.c \
    ../../backends/uart/transport_uart_mcu.c \
    ../../backends/uart/sal/esp32/uart_sal_esp32.c \
    -o bridge_linux
```

**Integration Pattern:**
```c
/* Main creates pthread */
int main(void) {
    platform_init();              /* Setup signals */
    bridge_init(TRANSPORT_TYPE_UART);
    platform_start_bridge();      /* Creates pthread */
    /* Thread calls bridge_process() with usleep() */
    while (platform_is_running()) {
        your_app_code();
    }
}
```

## ✨ Key Features

### ✅ Non-Blocking

`bridge_process()` returns immediately:
- Returns `1` - Command processed
- Returns `0` - No data available
- Returns `<0` - Error occurred

Never blocks waiting for data.

### ✅ Platform Abstraction

Same bridge code works everywhere:
- Common logic in `kta_bridge_example.c`
- Platform-specific code in `platform/*.c`
- Clean separation of concerns

### ✅ Easy Integration

Minimal effort to integrate:
1. Pick your platform implementation
2. Build with common code
3. Run - no complex setup

### ✅ Transport Agnostic

Works with any transport via HAL:
- UART, BLE, USB, Zigbee
- Select at compile time
- Same code for all transports

## 🔧 How to Add a New Platform

1. **Create platform implementation:**
   ```bash
   cp platform/bare_metal/platform_bare_metal.c \
      platform/myplatform/platform_myplatform.c
   ```

2. **Implement platform layer functions:**
   
   Edit `platform/myplatform/platform_myplatform.c`:
   ```c
   int platform_init(void) {
       /* Initialize your platform */
       return 0;
   }
   
   int platform_start_bridge(void) {
       /* Start bridge processing:
        * - Bare metal: Nothing (main loop calls bridge_process)
        * - RTOS: Create task that calls bridge_process()
        * - OS: Create thread that calls bridge_process()
        */
       return 0;
   }
   
   void platform_delay_ms(uint32_t ms) {
       /* Use your platform's delay function */
   }
   
   void platform_log(const char *format, ...) {
       /* Use your platform's logging */
       va_list args;
       va_start(args, format);
       vprintf(format, args);
       va_end(args);
   }
   
   bool platform_is_running(void) {
       /* Return true while system should run */
       return true;
   }
   
   void platform_deinit(void) {
       /* Cleanup your platform resources */
   }
   ```

3. **Build:**
   ```bash
   your-compiler -DPLATFORM_MYPLATFORM \
       kta_bridge_example.c \
       platform/myplatform/platform_myplatform.c \
       [other sources...]
   ```

## ⚙️ Customization

### Change Transport Type

Edit `kta_bridge_example.c`:
```c
#define BRIDGE_TRANSPORT_TYPE  TRANSPORT_TYPE_BLE  /* Change to BLE, USB, Zigbee */
```

### Adjust Poll Interval

In your platform implementation, change the delay:
```c
/* Bare metal */
platform_delay_ms(5);  /* Faster polling: 5ms instead of 10ms */

/* FreeRTOS */
vTaskDelay(pdMS_TO_TICKS(5));  /* 5ms instead of 10ms */

/* Linux */
usleep(5000);  /* 5ms in microseconds */
```

### Add Custom Initialization

In `platform_init()`:
```c
int platform_init(void) {
    gpio_init();
    uart_init();
    spi_init();
    /* Your custom hardware setup */
    return 0;
}
```

## 📦 File Dependencies

To build any example, you need:

**Core Bridge:**
- `../../common/transport_interface.c` - Protocol layer (TLV)
- `../../common/transport_hal_mcu.c` - HAL dispatcher
- `../../bridgeKta/bridge_kta.c` - Bridge implementation

**Transport Backend** (choose one or more):
- `../../backends/uart/transport_uart_mcu.c` - UART wrapper
- `../../backends/ble/transport_ble_mcu.c` - BLE wrapper
- `../../backends/usb/transport_usb_mcu.c` - USB wrapper
- `../../backends/zigbee/transport_zigbee_mcu.c` - Zigbee wrapper

**SAL Implementation** (platform-specific):
- `../../backends/uart/sal/esp32/uart_sal_esp32.c` - ESP32 UART
- `../../backends/ble/sal/esp32/ble_sal_esp32.c` - ESP32 BLE
- `../../backends/uart/sal/sg41/uart_sal_sg41.c` - Microchip SG41 UART
- `../../backends/ble/sal/nordic/ble_sal_nordic.c` - Nordic nRF52 BLE
- Or your own SAL implementation

## 🐛 Troubleshooting

**Problem:** Bridge doesn't respond  
**Solution:** Check transport initialization, verify SAL implementation, check connections

**Problem:** Build errors with FreeRTOS  
**Solution:** Verify FreeRTOS headers in include path, check FreeRTOSConfig.h

**Problem:** Linux build fails with pthread errors  
**Solution:** Add `-pthread` to BOTH compiler and linker flags

**Problem:** High CPU usage  
**Solution:** Increase delay in platform implementation (reduce polling frequency)

**Problem:** Commands processed slowly  
**Solution:** Decrease delay in platform implementation (increase polling frequency)

## 📊 Performance

Typical performance on ESP32 @ 240MHz with UART:

| Metric | Value |
|--------|-------|
| Poll rate | 100 Hz (10ms interval) |
| CPU idle | < 1% |
| CPU active | 5-10% |
| Command latency | < 20ms |
| RAM usage | < 8KB |

## 📚 Additional Resources

- [Bridge API](../../bridgeKta/bridge_kta.h) - Bridge interface and descriptors
- [Transport HAL](../../common/transport_hal.h) - Transport selection and config
- [SAL Implementations](../backends/) - Platform-specific drivers
- [Protocol Layer](../../common/transport_interface.h) - TLV protocol

## 📝 Notes

- **Legacy Examples:** The `main_*.c` files in platform subdirectories are legacy standalone examples. New projects should use the platform layer approach.
- **Thread Safety:** The bridge is not thread-safe. Call `bridge_process()` from only one task/thread.
- **Polling Rate:** Balance responsiveness vs CPU usage by adjusting the delay.
- **Transport Selection:** Can be changed at compile time or runtime via HAL configuration.

---

For questions, issues, or contributions, refer to the main project documentation.
