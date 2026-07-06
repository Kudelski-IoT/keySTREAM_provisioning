# Gateway Code Review – Changes Applied in `gateway_improved/`

This directory is a duplicate of `code/gateway/` with code-quality, safety,
and correctness fixes applied so the two trees can be diffed side-by-side.
Only files listed below were modified or added; everything else is
byte-identical to the source tree.

## Architecture (unchanged)
```
application/main_<os>_async_kta.c
    -> ktaIntegration/ktaFieldMgntHook (public C API)
        -> ktaIntegration/platform/<os>/kta_async_client (threaded async wrapper)
            -> backends/backend_interface (UART/BLE/USB/Zigbee dispatcher)
                -> backends/<X>/backend_<X> -> <X>_sal -> SAL/<platform>
            -> keyStreamIntegration/COMMSTACK/http (KTA HTTPS client)
```

## Files modified

### 1. `backends/zigbee/zigbee_sal.h`  (CRITICAL – build break)
- Replaced the bogus `#include <stdsize_t>` (which is not a real C
  header) with `#include <stddef.h>`. Without this fix every translation
  unit pulling `zigbee_sal.h` failed to compile.

### 2. `ktaIntegration/ktaFieldMgntHook.c`  (CRITICAL – data race)
- The async response buffer (`g_response_buffer`, `g_response_len`,
  `g_last_status`, `g_waiting_for_response`) is touched from both the
  main thread (`wait_for_response`) and the I/O callback thread
  (`on_response_callback`). The original code used only `volatile`,
  which is **not** sufficient to prevent torn reads, missed updates, or
  reads of a half-written buffer.
- Added a portable mutex wrapper (`response_lock_init/take/give`) that
  maps to `CRITICAL_SECTION` (Windows), `pthread_mutex_t` (Linux/macOS),
  `SemaphoreHandle_t` (FreeRTOS), and a no-op on bare metal.
- All four pieces of shared state are now read/written under the lock
  in `on_response_callback`, `wait_for_response`, and
  `send_request_and_wait`. The wait loop also samples the predicate
  under the lock.

### 3. `backends/backend_message.h`  (HIGH – contract clarification)
- Added a `@warning` on `backend_message_deserialize` documenting that
  the resulting `BackendMessage::fields[*].value` aliases into the
  caller's input buffer. Callers must keep that buffer alive for the
  message's lifetime, or copy the fields out. Without this warning
  several call-sites silently take dangling pointers.

### 4. `application/main_baremetal_async_kta.c`  (MEDIUM – broken delay)
- Replaced the unconditional, hard-coded busy-wait `delay_ms` (which
  also carried a `TODO`) with a calibration-friendly version: it picks
  up `HAL_GetTick()` automatically when the STM32 HAL is present, and
  otherwise runs a `volatile`/`asm("nop")` busy-wait keyed off a
  user-overridable `UART_CPU_CYCLES_PER_MS` define. This makes the
  status-monitoring loop produce something close to a real second
  instead of running unbounded at 100 % CPU on most targets.

## Files added

### 5. `backends/uart/sal/linux/uart_sal.c`  (HIGH – missing impl)
- The Linux UART SAL shipped only `uart_config.h`; every Linux build
  using the UART transport failed to link with undefined references to
  `uart_sal_*`.
- Added a self-contained POSIX/`termios` implementation
  (`open`/`tcsetattr`/`select`/`read`/`write`/`tcflush`) honoring the
  existing `UART_DEVICE_PATH`, `UART_VMIN`, `UART_VTIME`,
  `UART_READ_TIMEOUT_MS`, etc. from `uart_config.h`. Maps standard baud
  rates and falls back to 115200.

## Issues identified but NOT changed in this pass

Documented in the review report and left to follow-up commits:

- **HTTP resource leak / status validation** in
  `keyStreamIntegration/COMMSTACK/http/http.c` – `httpInit` cleans up
  on `salComConnect` failure but not on every other error path; HTTP
  status parsing accepts arbitrary `strtol` results without range
  checks.
- **Stack overflow risk** in `http.c` (`logPrepare` allocates 251 bytes
  on the stack on every call).
- **Hardcoded device credentials** (`C_KTA_APP_DEVICE_SERIAL_NUM`,
  `C_KTA_APP_CONTEXT_PROFILE_UID`, `C_KTA_APP__DEVICE_PUBLIC_UID`) –
  should be loaded from non-volatile storage in production.
- **Inconsistent HTTP API typing** (`uint8_t*` vs `char*` for textual
  hosts/URIs) – pure ergonomics, deferred.
- **Backend `SAFE_STRNCPY` duplication** across all four backend wrappers.
- **`backend_message_deserialize`** ideally should refactor to
  offset-based fields or copied values; the contract has been documented
  in this pass, full refactor is a separate change.

## How to compare

From a shell:

```pwsh
# overall change summary
git diff --no-index --stat code/gateway code/gateway_improved

# file-by-file diff
git diff --no-index code/gateway code/gateway_improved
```
