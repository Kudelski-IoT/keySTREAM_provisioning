/**
 * @file bridge_integration.c
 * @brief KTA Bridge Integration - MCU Implementation
 *
 * Reads incoming TLV messages from the UART backend, dispatches them to
 * bridge_kta_handle_transport(), and sends the serialized response back.
 *
 * Wire format (same as gateway backend_message.c):
 *   Header: [MSG_TYPE:1][CMD_TAG:1][FIELD_COUNT:1][RESERVED:1]
 *   Fields: [TAG:2 big-endian][LEN:2 big-endian][VALUE:LEN]
 */

#include "bridge_integration.h"
#include "../../backends/backend_interface.h"
#include "../../bridgeKta/bridge_kta.h"

#include <string.h>
#include <stdbool.h>

/* ============================================================================
 * Internal Constants
 * ============================================================================ */

#define WIRE_HEADER_SIZE  4U
#define WIRE_FIELD_HDR_SZ 4U
/* Buffers must hold the largest possible TLV packet:
 * 4-byte header + 4-byte field header + up to C_K__ICPP_MSG_MAX_SIZE payload.
 * keySTREAM server responses have been observed at 581 bytes payload
 * (589 bytes on the wire). Use 1024 to give headroom. */
#define RX_BUFFER_SIZE    1024U
#define TX_BUFFER_SIZE    1024U

/* ============================================================================
 * Internal State
 * ============================================================================ */

static bool    g_bridge_initialized = false;
static uint8_t g_rx_buffer[RX_BUFFER_SIZE];
static size_t  g_rx_len = 0;

/* Per-field scratch buffer: must hold the full field payload (up to 581 bytes
 * for keySTREAM server messages). 640 bytes gives comfortable headroom. */
#define FIELD_SCRATCH_SIZE 640U
static uint8_t g_field_scratch[TRANSPORT_MAX_FIELDS][FIELD_SCRATCH_SIZE];

/* Output buffer for serialized response */
static uint8_t g_tx_buffer[TX_BUFFER_SIZE];

/* ============================================================================
 * Internal: Parse wire bytes → TransportMessage
 *
 * Returns:
 *   > 0  : number of bytes consumed (complete message parsed)
 *     0  : not enough bytes yet (incomplete)
 *    -1  : parse error (malformed message, discard buffer)
 * ============================================================================ */
static int wire_to_transport_msg(const uint8_t *buf, size_t len, TransportMessage *msg)
{
    if (len < WIRE_HEADER_SIZE) {
        return 0; /* need more bytes */
    }

    uint8_t cmd_tag     = buf[1];
    uint8_t field_count = buf[2];

    if (field_count > TRANSPORT_MAX_FIELDS) {
        return -1; /* reject malformed */
    }

    /* Scan ahead to verify / compute total message size */
    size_t offset = WIRE_HEADER_SIZE;
    for (uint8_t i = 0U; i < field_count; i++) {
        if (offset + WIRE_FIELD_HDR_SZ > len) {
            return 0; /* wait for more bytes */
        }
        uint16_t flen = (uint16_t)(((uint16_t)buf[offset + 2U] << 8) | buf[offset + 3U]);
        offset += WIRE_FIELD_HDR_SZ + (size_t)flen;
        if (offset > len) {
            return 0; /* wait for more bytes */
        }
    }

    /* Have a complete message — populate TransportMessage */
    memset(msg, 0, sizeof(TransportMessage));
    msg->command_tag = cmd_tag;

    const uint8_t *p = buf + WIRE_HEADER_SIZE;
    for (uint8_t i = 0U; i < field_count; i++) {
        uint16_t tag  = (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
        uint16_t flen = (uint16_t)(((uint16_t)p[2] << 8) | p[3]);
        p += WIRE_FIELD_HDR_SZ;

        size_t copy_len = (flen < FIELD_SCRATCH_SIZE) ? flen : FIELD_SCRATCH_SIZE;
        memcpy(g_field_scratch[i], p, copy_len);
        p += flen;

        msg->fields[i].tag    = tag;
        msg->fields[i].length = copy_len;
        msg->fields[i].value  = g_field_scratch[i];
        msg->field_count++;
    }

    return (int)offset;
}

/* ============================================================================
 * Internal: Serialize TransportMessage → wire bytes
 *
 * Returns number of bytes written, or -1 if output buffer too small.
 * ============================================================================ */
static int transport_msg_to_wire(const TransportMessage *msg, uint8_t *buf, size_t buf_size)
{
    /* Calculate total required size */
    size_t needed = WIRE_HEADER_SIZE;
    for (uint8_t i = 0U; i < msg->field_count; i++) {
        needed += WIRE_FIELD_HDR_SZ + msg->fields[i].length;
    }
    if (needed > buf_size) {
        return -1;
    }

    uint8_t *p = buf;

    /* Header: MSG_TYPE=0x02 (RESPONSE), CMD_TAG, FIELD_COUNT, RESERVED */
    *p++ = 0x02U;
    *p++ = msg->command_tag;
    *p++ = msg->field_count;
    *p++ = 0x00U;

    for (uint8_t i = 0U; i < msg->field_count; i++) {
        uint16_t tag  = msg->fields[i].tag;
        uint16_t flen = (uint16_t)msg->fields[i].length;

        *p++ = (uint8_t)((tag  >> 8) & 0xFFU);
        *p++ = (uint8_t)( tag        & 0xFFU);
        *p++ = (uint8_t)((flen >> 8) & 0xFFU);
        *p++ = (uint8_t)( flen       & 0xFFU);

        if ((flen > 0U) && (msg->fields[i].value != NULL)) {
            memcpy(p, msg->fields[i].value, flen);
            p += flen;
        }
    }

    return (int)(p - buf);
}

/* ============================================================================
 * Public API
 * ============================================================================ */

int bridge_integration_init(BackendType transport_type)
{
    BackendStatus status = backend_init(transport_type);
    if (status != BACKEND_OK) {
        return -1;
    }

    /* Open backend — pass a config struct with the correct type so the
     * backend_interface type-check passes; SAL reads pin/baud from
     * uart_config.h at compile time. */
    BackendConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.type = transport_type;

    status = backend_open(&cfg);
    if (status != BACKEND_OK) {
        return -1;
    }

    /* Short non-blocking poll timeout for the bare-metal loop */
    backend_set_timeout(10U);

    g_rx_len = 0;
    g_bridge_initialized = true;
    return 0;
}

int bridge_integration_process(void)
{
    if (!g_bridge_initialized) {
        return -1;
    }

    /* Read available bytes into accumulation buffer */
    uint8_t chunk[64];
    size_t  received = 0;
    BackendStatus status = backend_receive(chunk, sizeof(chunk), &received);

    if ((status == BACKEND_OK) && (received > 0U)) {
        if ((g_rx_len + received) > RX_BUFFER_SIZE) {
            g_rx_len = 0U; /* overflow: discard stale data */
        }
        if ((g_rx_len + received) <= RX_BUFFER_SIZE) {
            memcpy(g_rx_buffer + g_rx_len, chunk, received);
            g_rx_len += received;
        }
    } else if ((status != BACKEND_TIMEOUT) && (status != BACKEND_OK)) {
        return -1; /* real transport error */
    }

    if (g_rx_len < WIRE_HEADER_SIZE) {
        return 0; /* not enough bytes yet */
    }

    /* Try to parse a complete message */
    TransportMessage request;
    memset(&request, 0, sizeof(request));

    int consumed = wire_to_transport_msg(g_rx_buffer, g_rx_len, &request);
    if (consumed == 0) {
        return 0; /* incomplete — wait for more bytes */
    }
    if (consumed < 0) {
        g_rx_len = 0U; /* parse error: discard */
        return -1;
    }

    /* Dispatch to bridge */
    TransportMessage response;
    memset(&response, 0, sizeof(response));

    TransportStatus ts = bridge_kta_handle_transport(&request, &response);

    if ((ts == TRANSPORT_OK) || (ts == TRANSPORT_SUCCESS)) {
        int tx_len = transport_msg_to_wire(&response, g_tx_buffer, sizeof(g_tx_buffer));
        if (tx_len > 0) {
            backend_send(g_tx_buffer, (size_t)tx_len);
        }
    }

    /* Consume processed bytes; keep any trailing partial message */
    if ((size_t)consumed < g_rx_len) {
        g_rx_len -= (size_t)consumed;
        memmove(g_rx_buffer, g_rx_buffer + consumed, g_rx_len);
    } else {
        g_rx_len = 0U;
    }

    return 1; /* one command processed */
}

bool bridge_integration_has_pending_fragment(void)
{
    return g_rx_len > 0U;
}

int bridge_integration_deinit(void)
{
    if (!g_bridge_initialized) {
        return 0;
    }

    backend_deinit();
    g_bridge_initialized = false;
    g_rx_len = 0U;
    return 0;
}
