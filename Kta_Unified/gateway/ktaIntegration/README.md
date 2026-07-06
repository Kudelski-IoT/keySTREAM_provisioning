# KTA Integration Layer

> **Do not edit** `ktaFieldMgntHook.c`, `ktaFieldMgntHook.h`, or any file under `platform/`.  
> Only edit configuration in `App_Config.h` (project root) and `ktaConfig.h`.

---

## What This Directory Contains

```
ktaIntegration/
├── ktaFieldMgntHook.h          ← Public API — only header customers include
├── ktaFieldMgntHook.c          ← Public API implementation (do not edit)
└── platform/
    ├── include/
    │   └── kta_async_client.h  ← Internal async wrapper (do not include directly)
    ├── windows/
    │   └── kta_async_client.c  ← Windows: CreateThread / HANDLE
    ├── linux/
    │   └── kta_async_client.c  ← Linux: pthread
    └── freertos/
        └── kta_async_client.c  ← FreeRTOS: xTaskCreate
```

---

## Architecture

```
┌──────────────────────────────────────────────────────┐
│  Customer Application  main_windows_async_kta.c      │
│    ktaKeyStreamInit()                                 │
│    ktaKeyStreamFieldMgmt(false/true, &status)         │
└───────────────────────┬──────────────────────────────┘
                        │  #include "ktaFieldMgntHook.h"
┌───────────────────────▼──────────────────────────────┐
│  ktaIntegration/  (THIS DIRECTORY — do not edit)      │
│    ktaFieldMgntHook.c                                 │
│      · ktaInitialize / ktaStartup / ktaSetDeviceInfo  │
│      · lPollKeyStream  (MCU ↔ HTTP relay loop)        │
│      · ktaKeyStreamStatus                             │
│    platform/windows/kta_async_client.c                │
│      · TLV frame builder / UART TX                    │
│      · Receive thread (ReadFile + frame reassembly)   │
└───────────────────────┬──────────────────────────────┘
                        │
┌───────────────────────▼──────────────────────────────┐
│  backends/  (transport layer — do not edit)           │
│    backend_interface.c   backend_message.c            │
│    uart/backend_uart.c                                │
│    uart/sal/windows/uart_sal.c                        │
└───────────────────────┬──────────────────────────────┘
                        │  UART (COM port)
                    ┌───▼───┐
                    │  MCU  │  PIC32CX SG41 + ATECC608
                    └───────┘
```

---

## Configuration (Edit These Files Only)

| What | File | Symbol |
|---|---|---|
| Fleet Profile UID | `App_Config.h` (project root) | `KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID` |
| L1 Segmentation Seed | `ktaConfig.h` | `C_KTA_APP__L1_SEG_SEED` |
| keySTREAM HTTP URL | `ktaConfig.h` | `C_KTA_APP__KEYSTREAM_HOST_HTTP_URL` |
| COM port (Windows) | `App_Config.h` | `UART_COM_PORT` / `UART_COM_PORT_STR` |

`ktaConfig.h` location: `keystream_connect/src/config/default/library/kta_lib/SOURCE/include/ktaConfig.h`

`ktaConfig.h` includes `App_Config.h` automatically. Do not define `C_KTA_APP__L1_SEG_SEED` or the URL macros in `App_Config.h` — they live in `ktaConfig.h`.

---

## Public API

Include only this header in your application:

```c
#include "ktaIntegration/ktaFieldMgntHook.h"
```

### `ktaKeyStreamInit()`

Call **once at startup** (or after REFURBISH resets `gKtaInitialized`).

Internally runs:  
1. Open UART / start receive thread (first call only — connection is kept open across re-inits)  
2. `ktaInitialize` (0xA0) → MCU  
3. `ktaStartup` (0xA1) → MCU with L1 seed, profile UID, serial, version  
4. `ktaSetDeviceInfo` (0xA2) → MCU; reads `CONN_REQUEST` flag  

If the device is already provisioned, `ktaStartup` returns `E_K_STATUS_ERROR` (status=6).  
`ktaKeyStreamInit` handles this gracefully — it skips to the caller without error.

Returns `E_K_STATUS_OK` on success.

### `ktaKeyStreamFieldMgmt(xIsFieldMgmtReq, &status)`

| `xIsFieldMgmtReq` | What it does |
|---|---|
| `false` | Lightweight: sends `ktaKeyStreamStatus` (0xA4) only. Fast — returns in < 1 s. Use for routine polling. |
| `true` | Full exchange: runs `lPollKeyStream` relay loop (MCU ↔ HTTP) then `ktaKeyStreamStatus`. Use on RENEW / REFURBISH. |

Status values written to `*status`:

| Value | Meaning |
|---|---|
| `E_K_KTA_KS_STATUS_NONE` (-1) | No exchange this session |
| `E_K_KTA_KS_STATUS_NO_OPERATION` (0) | Provisioned, nothing to do |
| `E_K_KTA_KS_STATUS_RENEW` (1) | Cloud requests key/cert renewal |
| `E_K_KTA_KS_STATUS_REFURBISH` (2) | Cloud requests full wipe + re-provision |

---

## Main Application Loop Pattern

```c
#include "ktaIntegration/ktaFieldMgntHook.h"

TKStatus         status;
TKktaKeyStreamStatus ks_status;

/* --- Startup --- */
status = ktaKeyStreamInit();
// handles already-provisioned device automatically

status = ktaKeyStreamFieldMgmt(false, &ks_status);
// runs provisioning exchange if CONN_REQUEST was set by MCU

/* --- Polling loop (every 10 s) --- */
while (running) {
    Sleep(10000);

    status = ktaKeyStreamFieldMgmt(false, &ks_status);  // lightweight status query

    if (ks_status == E_K_KTA_KS_STATUS_RENEW) {
        ktaKeyStreamFieldMgmt(true, &ks_status);        // full exchange
    }
    if (ks_status == E_K_KTA_KS_STATUS_REFURBISH) {
        ktaKeyStreamFieldMgmt(true, &ks_status);        // full re-provisioning
        ktaKeyStreamInit();                             // re-run startup protocol
    }
}
```

---

## Wire Protocol Summary

All UART frames use this TLV format:

```
[msg_type : 1 byte][cmd_tag : 1 byte][field_count : 1 byte]
  for each field: [tag : 2 LE][len : 2 LE][value : len bytes]
```

| Command | Tag | Direction | Fields sent |
|---|---|---|---|
| Initialize | 0xA0 | GW → MCU | none |
| Startup | 0xA1 | GW → MCU | 0x0001 seed, 0x0002 profile UID, 0x0003 serial, 0x0004 version |
| SetDeviceInfo | 0xA2 | GW → MCU | 0x0005 device profile UID, 0x0006 device serial |
| ExchangeMessage | 0xA3 | GW → MCU | 0x0007 KS message (empty on first call) |
| KeyStreamStatus | 0xA4 | GW → MCU | none |

MCU response fields:

| Tag | Name | Used for |
|---|---|---|
| 0x0101 | STATUS | `TKStatus` return code |
| 0x0008 | KTA_MSG_TO_SEND | Payload to relay to HTTP server |
| 0x0102 | KS_CMD_STATUS | `TKktaKeyStreamStatus` value |
| 0x0103 | CONN_REQUEST | 1 = provisioning exchange needed |

---

## Build (Windows)

`build_gateway.bat` in the project root compiles everything. Key flags:

```bat
-DKTA_CLIENT_BACKEND=BACKEND_TYPE_UART
-DKTA_ENABLE_LOGGING
-Ikeystream_connect\src\config\default\library\kta_lib\SOURCE\include
```

To switch transport change `-DKTA_CLIENT_BACKEND=BACKEND_TYPE_UART` to `BACKEND_TYPE_BLE`, `BACKEND_TYPE_USB`, or `BACKEND_TYPE_ZIGBEE` and swap the corresponding SAL source file.

---

## Thread Safety

- The receive thread (`receive_thread_worker`) runs continuously, appending bytes to `rx_buffer`.  
- `process_received_data` reassembles complete TLV frames before calling the callback.  
- `g_response_buffer` / `g_response_len` / `g_waiting_for_response` are protected by `g_response_lock` (Windows `CRITICAL_SECTION`).  
- `send_request_and_wait` blocks the main thread polling the lock at 50 ms intervals, up to 120 s.


## Directory Structure

```
ktaIntegration/
├── ktaFieldMgntHook.h         ← PUBLIC API (customer-facing)
├── ktaFieldMgntHook.c         ← PUBLIC API implementation
├── kta_async_client.h         ← Internal async wrapper interface
└── platform/
    ├── windows/
    │   └── kta_async_client.c ← Windows threading (CreateThread)
    ├── linux/
    │   └── kta_async_client.c ← Linux threading (pthread)
    └── freertos/
        └── kta_async_client.c ← FreeRTOS threading (xTaskCreate)
```

## Architecture

This directory implements **Layer 2** in the KTA communication stack:

```
┌─────────────────────────────────────────────────┐
│ Customer Application (main.c)                   │  ← LAYER 1
│   #include "ktaIntegration/ktaFieldMgntHook.h"  │
└─────────────────────────────────────────────────┘
                     ↓
┌─────────────────────────────────────────────────┐
│ ktaIntegration/ (THIS DIRECTORY)                │  ← LAYER 2
│   • ktaFieldMgntHook.h/.c (3 public functions)  │
│   • kta_async_client.h (12 async functions)     │
│   • Platform implementations (threading)        │
└─────────────────────────────────────────────────┘
                     ↓
┌─────────────────────────────────────────────────┐
│ backends/ (Transport Abstraction)               │  ← LAYER 3
│   • backend_interface (UART/BLE/USB/Zigbee)     │
└─────────────────────────────────────────────────┘
                     ↓
┌─────────────────────────────────────────────────┐
│ SAL (Hardware Abstraction)                      │  ← LAYER 4
│   • uart_sal/ (hal_uart_windows, etc.)          │
└─────────────────────────────────────────────────┘
```

## Public API (Customer-Facing)

### ktaFieldMgntHook.h - 3 Functions

Customers should **ONLY** include this header:

```c
#include "ktaIntegration/ktaFieldMgntHook.h"
```

**API Functions:**
1. `ktaKeyStreamInit()` - Initialize KTA (runs ktaInitialize → ktaStartup → ktaSetDeviceInfo)
2. `ktaKeyStreamFieldMgmt()` - Run provisioning workflow (ktaExchangeMessage loop → ktaKeyStreamStatus)
3. `ktaKeyStreamUpdateConfig()` - Update configuration parameters

**Example Usage:**
```c
TKStatus status;
TKktaKeyStreamStatus ks_status;

// Step 1: Initialize
status = ktaKeyStreamInit();
if (status != E_K_STATUS_OK) {
    printf("Init failed\n");
    return -1;
}

// Step 2: Run provisioning
status = ktaKeyStreamFieldMgmt(false, &ks_status);
if (status == E_K_STATUS_OK) {
    printf("Status: %d\n", ks_status);
}
```

---

*Do not edit files in this directory. All configuration is in `App_Config.h` and `ktaConfig.h`.*


**DO NOT** include this in customer applications. This header is for internal library implementation only.

Provides async/callback-based wrapper around KTA API calls:
- `kta_async_client_init()` - Initialize client
- `kta_async_client_start()` - Start async processing thread
- `kta_async_initialize()` - Send ktaInitialize request
- `kta_async_startup()` - Send ktaStartup request
- `kta_async_set_device_info()` - Send ktaSetDeviceInfo request
- `kta_async_exchange_message()` - Send ktaExchangeMessage request
- `kta_async_keystream_status()` - Send ktaKeyStreamStatus request
- `kta_async_is_connected()` - Check connection status
- `kta_async_get_pending_count()` - Get pending request count
- `kta_async_client_stop()` - Stop async processing
- `kta_async_client_deinit()` - Deinitialize client

### Platform Implementations

Each platform provides its own threading implementation:

**Windows** (`platform/windows/kta_async_client.c`):
- Uses `CreateThread()`, `HANDLE`, `InterlockedExchange()`
- Event-based thread synchronization

**Linux** (`platform/linux/kta_async_client.c`):
- Uses `pthread_create()`, `pthread_join()`, `pthread_mutex_t`
- POSIX threading model

**FreeRTOS** (`platform/freertos/kta_async_client.c`):
- Uses `xTaskCreate()`, `vTaskDelay()`, `xSemaphoreGive()`
- RTOS task-based model

## Build Integration

### Include Paths

Add to your build system:
```makefile
INCLUDES += -I$(PROJECT_ROOT)/code/gateway/ktaIntegration
INCLUDES += -I$(PROJECT_ROOT)/code/gateway/backends
INCLUDES += -I$(PROJECT_ROOT)/code/common
```

### Source Files

Link appropriate platform implementation:

**Windows:**
```makefile
SOURCES += ktaIntegration/ktaFieldMgntHook.c
SOURCES += ktaIntegration/platform/windows/kta_async_client.c
SOURCES += backends/backend_interface.c
SOURCES += backends/uart/backend_uart.c
```

**Linux:**
```makefile
SOURCES += ktaIntegration/ktaFieldMgntHook.c
SOURCES += ktaIntegration/platform/linux/kta_async_client.c
SOURCES += backends/backend_interface.c
SOURCES += backends/uart/backend_uart.c
LDFLAGS += -lpthread
```

**FreeRTOS:**
```makefile
SOURCES += ktaIntegration/ktaFieldMgntHook.c
SOURCES += ktaIntegration/platform/freertos/kta_async_client.c
SOURCES += backends/backend_interface.c
SOURCES += backends/uart/backend_uart.c
```

### Compile-Time Backend Selection

Select transport backend at compile time:
```makefile
# UART backend (default)
CFLAGS += -DKTA_CLIENT_BACKEND=BACKEND_TYPE_UART

# Or BLE backend
CFLAGS += -DKTA_CLIENT_BACKEND=BACKEND_TYPE_BLE

# Or USB backend
CFLAGS += -DKTA_CLIENT_BACKEND=BACKEND_TYPE_USB

# Or Zigbee backend
CFLAGS += -DKTA_CLIENT_BACKEND=BACKEND_TYPE_ZIGBEE
```

## Comparison with examples/

| Aspect | ktaIntegration/ (Product) | examples/ (Demos) |
|--------|--------------------------|-------------------|
| **Purpose** | Product library code | Usage demonstrations |
| **Contents** | API implementation | main.c demonstration files |
| **Customer Use** | Link into application | Reference for integration |
| **Shipping** | Ships with SDK | Ships as reference |
| **Modification** | Rarely modified | Frequently customized |

## Thread Safety

The async client implementations are thread-safe:
- Request queue protected by platform-specific synchronization
- Response callbacks invoked from receive thread
- Customer callback must be thread-safe if it accesses shared data

## Error Handling

All public API functions return `TKStatus`:
- `E_K_STATUS_OK` (0) - Success
- `E_K_STATUS_PARAMETER` (-2) - Invalid parameters
- `E_K_STATUS_ERROR` (-1) - General error

Internal functions return `BackendStatus`:
- `BACKEND_OK` - Success
- `BACKEND_ERROR` - Error
- `BACKEND_TIMEOUT` - Timeout
- `BACKEND_INVALID_PARAM` - Invalid parameter

## Logging

The async client automatically logs all requests/responses when initialized with `log_to_file = true`:

```
kta_session_20250109_143022.log
```

Format:
```
[Timestamp] REQUEST #1 - Initialize
  Serialized: [hex dump]

[Timestamp] RESPONSE #1 - Status: 0
  Data: [hex dump]
```

## Migration from examples/

**OLD Structure** (examples/ contained everything):
```c
#include "examples/common/ktaFieldMgntHook.h"
```

**NEW Structure** (product code separated):
```c
#include "ktaIntegration/ktaFieldMgntHook.h"
```

**Why This Change?**
- Clear separation between product library vs demonstration code
- Mirrors industry-standard SDK structure
- Examples directory now contains only usage demonstrations
- Easier to ship as pre-built library

## Dependencies

```
ktaIntegration/
    ├── Depends on: backends/
    │   └── backend_interface.h
    │       └── backend_uart.c / backend_ble.c / etc.
    │           └── uart_sal/ / ble_sal/ / etc.
    │
    └── Depends on: common/
        └── transport_interface.h
            └── Transport TLV serialization
```

## Related Documentation

- [../examples/CUSTOMER_INTEGRATION.md](../examples/CUSTOMER_INTEGRATION.md) - Customer integration guide
- [../backends/README.md](../backends/README.md) - Backend abstraction layer
- [API_INTERFACE_VERIFICATION.md](../../API_INTERFACE_VERIFICATION.md) - Complete API verification

## Summary

**Customer View:**
```c
#include "ktaIntegration/ktaFieldMgntHook.h"

int main(void) {
    ktaKeyStreamInit();
    ktaKeyStreamFieldMgmt(false, &status);
    // Done!
}
```

**What's Hidden:**
- Threading (Windows/Linux/FreeRTOS)
- Async request/response handling
- Backend selection (UART/BLE/USB/Zigbee)
- TLV serialization
- Error recovery
- Logging

**Result:** Simple 3-function API that hides all complexity.
