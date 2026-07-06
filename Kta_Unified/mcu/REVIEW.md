# MCU Code Review – Changes Applied in `mcu_improved/`

This directory is a duplicate of `code/mcu/` with code-quality, safety, and
correctness fixes applied so the two trees can be diffed side-by-side.
Only files listed below were modified; everything else is byte-identical to
the source tree.

## Architecture (unchanged)
```
Application -> bridge_integration -> backend_interface -> backend_<X> -> <X>_sal -> SAL/<platform>
```
- `backends/`         transport dispatcher + per-transport wrappers (UART/BLE/USB/Zigbee)
- `bridgeKta/`        TLV ↔ KTA C API descriptor-table dispatcher
- `examples/common/`  OS-independent init / main-loop glue
- `examples/platform/<rtos>/`  per-RTOS thin entry points

## Files modified

### 1. `bridgeKta/bridge_kta.c`  (CRITICAL)
- Hardened `bridge_kta_get_field()` against `NULL` `fields[]` arrays and
  against length-without-data inconsistencies; also widened the loop
  index to `size_t` to match `field_count` semantics.
- Replaced unaligned `*(uint32_t*)ptr` reads with a new
  `bridge_kta_read_u32()` helper that uses `memcpy` — fixes undefined
  behavior on strict-alignment ARM cores.
- Added bounded length checks in `bridge_kta_handle_exchange_message`
  (`BRIDGE_MAX_KS_MESSAGE_SIZE`) and `bridge_kta_handle_sign_hash`
  (`BRIDGE_MAX_HASH_DATA_SIZE`) before calling KTA core – prevents
  buffer overflow if KTA core does not validate input size.
- Validated KTA-reported output length against destination buffer size.
- Marked the unused `request` parameter of `bridge_kta_handle_initialize`
  with `(void)request;` (silences `-Wunused-parameter`).

### 2. `examples/common/bridge_integration.c`  (CRITICAL)
- Added explicit guard `rx_len > sizeof(g_rx_buffer)` after `backend_receive`
  – defense-in-depth against a misbehaving SAL.
- On deserialization failure the bridge now emits a best-effort error
  response (status field) so the host does not hang waiting for a reply.
- Bridge handler errors no longer abort the loop with `-1`; they now log
  a warning and let the existing response be sent (handlers populate
  proper error responses themselves).

### 3. `examples/platform/freertos/main_freertos.c`
- Removed the stray `#endif` and orphan trailing comments that left the
  file syntactically malformed (the original would fail to compile).
- Cleaned up the entry: returns `kta_mcu_integration_entry()` and only
  spins as a defensive fallback.

### 4. `examples/platform/bare_metal/kta_mcu_integration_baremetal.c`
- `application_init()` is no longer an undefined external; a `weak`
  default returning 0 is provided so the example links standalone.
- The infinite `while(1)` was replaced by a flag-driven loop so
  `bridge_integration_deinit()` is actually reachable, and a transient
  failure from `bridge_integration_process()` is logged but does not
  abort the loop (transport may recover).

## Issues identified but NOT changed in this pass
The following non-critical observations are noted in the review report but
were left unchanged to keep the diff minimal and reviewable:
- Magic numbers for backend timeouts (UART 1000 ms, BLE 100 ms, …).
- `printf`-based logging vs ESP-IDF `ESP_LOG*` (no platform-agnostic
  macro yet).
- Optional protocol versioning / capability negotiation.
- Mutex protection inside the ESP32 BLE SAL (`ble_sal.c`) – the
  original file is large, partially platform-specific, and the
  recommended fix involves restructuring queue and state-machine
  ownership; tracked as Phase-1 follow-up.
