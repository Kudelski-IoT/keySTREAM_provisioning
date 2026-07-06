/**
 * @file kta_bridge.c
 * @brief Generic KTA integration bridge (Transport -> KTA)
 *
 * This bridge exposes all k_kta.h public APIs through the transport layer.
 * Works with any transport backend: UART, USB, BLE, Zigbee, etc.
 * Uses descriptor table architecture for minimal code footprint.
 */

#include "bridge_kta.h"
#include <string.h>

/* KTA headers (from miniclientdev-main) */
#include "k_kta.h"
#include "k_sal_storage.h"

/* ATECC608 — used to read device serial on-chip */
#include "cryptoauthlib.h"
/* KTA config — needed for C_K__ICPP_MSG_MAX_SIZE */
#include "cryptoConfig.h"

static const char *BRIDGE_TAG = "BRIDGE_KTA";

/* Idempotency flags — match reference ensure_kta_initialized_and_seeded() pattern */
static bool g_bridge_initialized_once = false;
static bool g_bridge_started_once = false;
static bool g_bridge_device_info_once = false;
static uint8_t g_bridge_last_conn_req = 0;

/* Shared buffers for large data */
static uint8_t g_kta_msg_buffer[C_BRIDGE_KTA_MESSAGE_BUFFER_SIZE];
static uint8_t g_obj_data_buffer[C_BRIDGE_OBJECT_DATA_BUFFER_SIZE];
static uint8_t g_signed_hash_buffer[C_BRIDGE_SIGNED_HASH_BUFFER_SIZE];

/* Helper: Extract field value from message */
static const uint8_t *bridge_kta_get_field(const TransportMessage *msg, uint16_t tag, size_t *len)
{
    if (!msg || !len)
        return NULL;
    for (uint8_t i = 0; i < msg->field_count; i++)
    {
        if (msg->fields[i].tag == tag)
        {
            *len = msg->fields[i].length;
            return msg->fields[i].value;
        }
    }
    *len = 0;
    return NULL;
}

/* Helper: Add status field to response */
static TransportStatus bridge_kta_add_status(TransportMessage *resp, TKStatus status)
{
    uint8_t val = (uint8_t)status;
    return transport_message_add_field(resp, BRIDGE_FIELD_STATUS, &val, sizeof(val));
}

/* Command handler function pointer type */
typedef TransportStatus (*BridgeCmdHandler)(const TransportMessage *request, TransportMessage *response);

/* Handler implementations */
static TransportStatus bridge_kta_handle_initialize(const TransportMessage *request, TransportMessage *response)
{
    if (g_bridge_initialized_once)
    {
        KTA_LOG_I(BRIDGE_TAG, "INITIALIZE: already initialized; returning OK");
        return bridge_kta_add_status(response, E_K_STATUS_OK);
    }

    KTA_LOG_I(BRIDGE_TAG, "INITIALIZE: calling ktaInitialize()");
    TKStatus status = ktaInitialize();
    KTA_LOG_I(BRIDGE_TAG, "INITIALIZE: ktaInitialize() returned status=%d", (int)status);

    /* ktaInitialize() is a one-shot; treat ERROR/STATE on subsequent calls as
       "already initialized" — same pattern as reference ensure_kta_initialized_and_seeded() */
    if (status == E_K_STATUS_OK || status == E_K_STATUS_ERROR || status == E_K_STATUS_STATE)
    {
        g_bridge_initialized_once = true;
        status = E_K_STATUS_OK;
    }
    return bridge_kta_add_status(response, status);
}

static TransportStatus bridge_kta_handle_startup(const TransportMessage *request, TransportMessage *response)
{
    size_t len;
    const uint8_t *seed = bridge_kta_get_field(request, BRIDGE_FIELD_L1_SEG_SEED, &len);
    if (!seed || len != C_K__L1_SEGMENTATION_SEED_SIZE)
        return bridge_kta_add_status(response, E_K_STATUS_PARAMETER);

    const uint8_t *profile = bridge_kta_get_field(request, BRIDGE_FIELD_CONTEXT_PROFILE_UID, &len);
    if (!profile || len == 0 || len > C_K__CONTEXT_PROFILE_UID_MAX_SIZE)
        return bridge_kta_add_status(response, E_K_STATUS_PARAMETER);
    size_t profile_len = len;

    const uint8_t *serial = bridge_kta_get_field(request, BRIDGE_FIELD_CONTEXT_SERIAL_NUM, &len);
    if (!serial || len == 0 || len > C_K__CONTEXT_SERIAL_NUMBER_MAX_SIZE)
        return bridge_kta_add_status(response, E_K_STATUS_PARAMETER);
    size_t serial_len = len;

    const uint8_t *version = bridge_kta_get_field(request, BRIDGE_FIELD_CONTEXT_VERSION, &len);
    if (!version || len == 0 || len > C_K__CONTEXT_VERSION_MAX_SIZE)
        return bridge_kta_add_status(response, E_K_STATUS_PARAMETER);
    size_t version_len = len;

    if (g_bridge_started_once)
    {
        KTA_LOG_I(BRIDGE_TAG, "STARTUP: already started; returning OK");
        return bridge_kta_add_status(response, E_K_STATUS_OK);
    }

    KTA_LOG_I(BRIDGE_TAG, "STARTUP: calling ktaStartup(seed=%p, prof=%p/%u, ser=%p/%u, ver=%p/%u)",
             seed, profile, (unsigned)profile_len, serial, (unsigned)serial_len, version, (unsigned)version_len);
    TKStatus status = ktaStartup(seed, profile, profile_len, serial, serial_len, version, version_len);
    KTA_LOG_I(BRIDGE_TAG, "STARTUP: ktaStartup() returned status=%d", (int)status);
    if (status == E_K_STATUS_OK)
    {
        g_bridge_started_once = true;
    }
    return bridge_kta_add_status(response, status);
}

static TransportStatus bridge_kta_handle_set_device_info(const TransportMessage *request, TransportMessage *response)
{
    size_t len;
    const uint8_t *profile = bridge_kta_get_field(request, BRIDGE_FIELD_DEVICE_PROFILE_UID, &len);
    if (!profile || len == 0 || len > C_K__DEVICE_PROFILE_PUBLIC_UID_MAX_SIZE)
        return bridge_kta_add_status(response, E_K_STATUS_PARAMETER);
    size_t profile_len = len;

    /* Read device serial from ATECC608 on-chip (9 bytes) */
    uint8_t chip_serial[9];
    ATCA_STATUS atca_rc = atcab_read_serial_number(chip_serial);
    const uint8_t *serial;
    size_t serial_len;

    if (atca_rc == ATCA_SUCCESS)
    {
        serial = chip_serial;
        serial_len = sizeof(chip_serial);
        KTA_LOG_I(BRIDGE_TAG, "ATECC608 serial: %02x%02x%02x%02x%02x%02x%02x%02x%02x",
                 chip_serial[0], chip_serial[1], chip_serial[2], chip_serial[3],
                 chip_serial[4], chip_serial[5], chip_serial[6], chip_serial[7],
                 chip_serial[8]);
    }
    else
    {
        /* Fallback: use serial from CLI field if ATECC not available */
        serial = bridge_kta_get_field(request, BRIDGE_FIELD_DEVICE_SERIAL_NUM, &len);
        if (!serial || len == 0 || len > C_K__DEVICE_SERIAL_NUM_MAX_SIZE)
            return bridge_kta_add_status(response, E_K_STATUS_PARAMETER);
        serial_len = len;
        KTA_LOG_W(BRIDGE_TAG, "ATECC608 read failed (%d); using CLI-provided serial", atca_rc);
    }

    if (g_bridge_device_info_once)
    {
        KTA_LOG_I(BRIDGE_TAG, "SET_DEVICE_INFO: already set; returning OK connReq=%u",
                 (unsigned)g_bridge_last_conn_req);
        TransportStatus us = bridge_kta_add_status(response, E_K_STATUS_OK);
        if (us == TRANSPORT_SUCCESS)
        {
            us = transport_message_add_field(response, BRIDGE_FIELD_CONN_REQUEST,
                                             &g_bridge_last_conn_req, sizeof(g_bridge_last_conn_req));
        }
        return us;
    }

    uint8_t conn_req = 0;
    KTA_LOG_I(BRIDGE_TAG, "SET_DEVICE_INFO: calling ktaSetDeviceInformation(prof=%p/%u, ser=%p/%u)",
             profile, (unsigned)profile_len, serial, (unsigned)serial_len);
    TKStatus status = ktaSetDeviceInformation(profile, profile_len, serial, serial_len, &conn_req);
    KTA_LOG_I(BRIDGE_TAG, "SET_DEVICE_INFO: ktaSetDeviceInformation() returned status=%d connReq=%u",
             (int)status, (unsigned)conn_req);
    if (status == E_K_STATUS_OK)
    {
        g_bridge_device_info_once = true;
        g_bridge_last_conn_req = conn_req;
    }

    TransportStatus us = bridge_kta_add_status(response, status);
    if (us == TRANSPORT_SUCCESS)
    {
        us = transport_message_add_field(response, BRIDGE_FIELD_CONN_REQUEST, &conn_req, sizeof(conn_req));
    }
    return us;
}

static TransportStatus bridge_kta_handle_exchange_message(const TransportMessage *request, TransportMessage *response)
{
    size_t len;
    const uint8_t *ks_msg = bridge_kta_get_field(request, BRIDGE_FIELD_KS_MSG_TO_PROCESS, &len);

    /* KTA requires a non-NULL pointer even for empty (poll) exchanges */
    static uint8_t s_empty_buf[1] = {0};
    if (!ks_msg)
    {
        ks_msg = s_empty_buf;
        len = 0;
    }

    /* KTA internal general.c rejects *xpKta2ksMsgLen > C_K__ICPP_MSG_MAX_SIZE.
       Must cap to exactly C_K__ICPP_MSG_MAX_SIZE, not our larger buffer size. */
    size_t kta_msg_len = C_K__ICPP_MSG_MAX_SIZE;

    KTA_LOG_I(BRIDGE_TAG, "EXCHANGE: ks2kta_len=%u kta_cap=%u", (unsigned)len, (unsigned)kta_msg_len);
    if (len > 0 && len <= 32)
    {
        KTA_LOG_BUF_HEX(BRIDGE_TAG, ks_msg, len);
    }
    else if (len > 32)
    {
        KTA_LOG_I(BRIDGE_TAG, "EXCHANGE: ks2kta first 32 bytes:");
        KTA_LOG_BUF_HEX(BRIDGE_TAG, ks_msg, 32);
    }

    TKStatus status = ktaExchangeMessage(ks_msg, len, g_kta_msg_buffer, &kta_msg_len);

    KTA_LOG_I(BRIDGE_TAG, "EXCHANGE: status=%d kta2ks_len=%u",
             (int)status, (unsigned)kta_msg_len);
    if (status == E_K_STATUS_OK && kta_msg_len > 0)
    {
        KTA_LOG_I(BRIDGE_TAG, "EXCHANGE: kta2ks first %u bytes:",
                 (unsigned)(kta_msg_len < 32 ? kta_msg_len : 32));
        KTA_LOG_BUF_HEX(BRIDGE_TAG, g_kta_msg_buffer, kta_msg_len < 32 ? kta_msg_len : 32);
    }

    /* If KTA call failed, reset kta_msg_len so we don't add stale data */
    if (status != E_K_STATUS_OK)
    {
        kta_msg_len = 0;
    }

    /* Do NOT reset idempotency flags here. The gateway's ktaKeyStreamStatus
     * call will query gCommandStatus (REFURBISH / RENEW / NO_OPERATION) and
     * the gateway decides what to do next.  Resetting g_bridge_* flags here
     * would make the bridge consider itself uninitialised before that status
     * query, causing ktaKeyStreamStatus to fail with -1. */
    if (status == E_K_STATUS_OK && kta_msg_len == 0)
    {
        KTA_LOG_I(BRIDGE_TAG, "EXCHANGE: No-Op complete - device provisioned/operational");
    }

    TransportStatus us = bridge_kta_add_status(response, status);
    if (us != TRANSPORT_SUCCESS)
        return us;

    if (kta_msg_len > 0)
    {
        us = transport_message_add_field(response, BRIDGE_FIELD_KTA_MSG_TO_SEND, g_kta_msg_buffer, kta_msg_len);
        if (us != TRANSPORT_SUCCESS)
            return us;
    }

    uint16_t msg_len_val = (uint16_t)kta_msg_len;
    return transport_message_add_field(response, BRIDGE_FIELD_MSG_LEN, (uint8_t *)&msg_len_val, sizeof(msg_len_val));
}

static TransportStatus bridge_kta_handle_keystream_status(const TransportMessage *request, TransportMessage *response)
{
    /* Init to NO_OPERATION so even if ktaKeyStreamStatus somehow fails the
     * cast to uint8_t is 0x00, not 0xFF (-1 casted). */
    TKktaKeyStreamStatus ks_status = E_K_KTA_KS_STATUS_NO_OPERATION;
    TKStatus status = ktaKeyStreamStatus(&ks_status);

    /* When keySTREAM pushes a REFURBISH through the normal ExchangeMessage
     * flow, the KTA library wipes its own state back to SEALED/INITIAL, but
     * the bridge's idempotency flags stay latched from the first onboarding.
     * That makes the gateway's subsequent Initialize/Startup/SetDeviceInfo
     * calls short-circuit to a cached "OK", so the device can never form a
     * fresh activation request.  Clear the flags here so the next init
     * sequence genuinely re-runs ktaInitialize/ktaStartup/ktaSetDeviceInfo
     * and re-onboards the device. */
    if (ks_status == E_K_KTA_KS_STATUS_REFURBISH)
    {
        KTA_LOG_W(BRIDGE_TAG, "KEYSTREAM_STATUS: REFURBISH — clearing bridge init flags for re-onboarding");
        g_bridge_initialized_once = false;
        g_bridge_started_once = false;
        g_bridge_device_info_once = false;
        g_bridge_last_conn_req = 0;
    }

    TransportStatus us = bridge_kta_add_status(response, status);
    if (us == TRANSPORT_SUCCESS)
    {
        uint8_t ks_val = (uint8_t)(int8_t)ks_status;
        us = transport_message_add_field(response, BRIDGE_FIELD_KS_CMD_STATUS, &ks_val, sizeof(ks_val));
    }
    return us;
}

static TransportStatus bridge_kta_handle_get_object_with_assoc(const TransportMessage *request, TransportMessage *response)
{
    size_t len;
    const uint8_t *obj_id_ptr = bridge_kta_get_field(request, BRIDGE_FIELD_OBJECT_ID, &len);
    if (!obj_id_ptr || len != sizeof(uint32_t))
        return bridge_kta_add_status(response, E_K_STATUS_PARAMETER);

    uint32_t obj_id = *(uint32_t *)obj_id_ptr;
    uint32_t assoc_key_id = 0, assoc_obj_id = 0;
    size_t obj_len = sizeof(g_obj_data_buffer);

    TKStatus status = ktaGetObjectWithAssociation(obj_id, &assoc_key_id, &assoc_obj_id,
                                                  g_obj_data_buffer, &obj_len);

    TransportStatus us = bridge_kta_add_status(response, status);
    if (us != TRANSPORT_SUCCESS)
        return us;

    if (status == E_K_STATUS_OK)
    {
        us = transport_message_add_field(response, BRIDGE_FIELD_ASSOCIATED_KEY_ID,
                                         (uint8_t *)&assoc_key_id, sizeof(assoc_key_id));
        if (us != TRANSPORT_SUCCESS)
            return us;

        us = transport_message_add_field(response, BRIDGE_FIELD_ASSOCIATED_OBJ_ID,
                                         (uint8_t *)&assoc_obj_id, sizeof(assoc_obj_id));
        if (us != TRANSPORT_SUCCESS)
            return us;

        us = transport_message_add_field(response, BRIDGE_FIELD_OBJECT_DATA, g_obj_data_buffer, obj_len);
    }
    return us;
}

static TransportStatus bridge_kta_handle_get_object(const TransportMessage *request, TransportMessage *response)
{
    size_t len;
    const uint8_t *obj_id_ptr = bridge_kta_get_field(request, BRIDGE_FIELD_OBJECT_ID, &len);
    if (!obj_id_ptr || len != sizeof(uint32_t))
        return bridge_kta_add_status(response, E_K_STATUS_PARAMETER);

    uint32_t obj_id = *(uint32_t *)obj_id_ptr;
    TKktaDataObject obj = {
        .data = g_obj_data_buffer,
        .dataLen = sizeof(g_obj_data_buffer),
    };

    TKStatus status = ktaGetObject(obj_id, &obj);

    TransportStatus us = bridge_kta_add_status(response, status);
    if (us != TRANSPORT_SUCCESS)
        return us;

    if (status == E_K_STATUS_OK && obj.dataLen > 0)
    {
        us = transport_message_add_field(response, BRIDGE_FIELD_OBJECT_DATA, obj.data, obj.dataLen);
    }
    return us;
}

static TransportStatus bridge_kta_handle_sign_hash(const TransportMessage *request, TransportMessage *response)
{
    size_t len;
    const uint8_t *key_id_ptr = bridge_kta_get_field(request, BRIDGE_FIELD_KEY_ID, &len);
    if (!key_id_ptr || len != sizeof(uint32_t))
        return bridge_kta_add_status(response, E_K_STATUS_PARAMETER);

    uint32_t key_id = *(uint32_t *)key_id_ptr;
    const uint8_t *hash = bridge_kta_get_field(request, BRIDGE_FIELD_HASH_DATA, &len);
    if (!hash || len == 0)
        return bridge_kta_add_status(response, E_K_STATUS_PARAMETER);

    size_t signed_len = 0;
    TKStatus status = ktaSignHash(key_id, (uint8_t *)hash, len, g_signed_hash_buffer,
                                  sizeof(g_signed_hash_buffer), &signed_len);

    TransportStatus us = bridge_kta_add_status(response, status);
    if (us != TRANSPORT_SUCCESS)
        return us;

    if (status == E_K_STATUS_OK && signed_len > 0)
    {
        us = transport_message_add_field(response, BRIDGE_FIELD_SIGNED_HASH, g_signed_hash_buffer, signed_len);
    }
    return us;
}

/* -------------------------------------------------------------------------- */
/* REFURBISH HANDLER                                                         */
/* -------------------------------------------------------------------------- */

/* Lifecycle NVM storage ID - use value from k_sal.h (already included via k_sal_storage.h) */
#undef C_K_KTA__LIFE_CYCLE_STATE_STORAGE_ID
#define C_K_KTA__LIFE_CYCLE_STATE_STORAGE_ID (0x4000u)
#define C_KTA_CONFIG__LIFE_CYCLE_EACH_STATE_SIZE 4u

static TransportStatus bridge_kta_handle_refurbish(const TransportMessage *request, TransportMessage *response)
{
    (void)request;
    KTA_LOG_W(BRIDGE_TAG, "REFURBISH command received — resetting device state");

    /* Reset idempotency flags so next connect does full init sequence */
    g_bridge_initialized_once = false;
    g_bridge_started_once = false;
    g_bridge_device_info_once = false;
    g_bridge_last_conn_req = 0;

    /* Reset lifecycle to INIT in NVM */
    const uint8_t init_state[C_KTA_CONFIG__LIFE_CYCLE_EACH_STATE_SIZE] = {0x00, 0x00, 0x00, 0x00};
    TKStatus status = salStorageSetValue(C_K_KTA__LIFE_CYCLE_STATE_STORAGE_ID,
                                         init_state,
                                         C_KTA_CONFIG__LIFE_CYCLE_EACH_STATE_SIZE);
    if (status != E_K_STATUS_OK)
    {
        KTA_LOG_E(BRIDGE_TAG, "Failed to reset lifecycle NVM: %d", (int)status);
    }

    return bridge_kta_add_status(response, status);
}

/* Command descriptor table */
typedef struct
{
    uint8_t cmd_tag;
    BridgeCmdHandler handler;
} BridgeCmdDescriptor;

static const BridgeCmdDescriptor g_cmd_table[] = {
    {BRIDGE_CMD_INITIALIZE, bridge_kta_handle_initialize},
    {BRIDGE_CMD_STARTUP, bridge_kta_handle_startup},
    {BRIDGE_CMD_SET_DEVICE_INFO, bridge_kta_handle_set_device_info},
    {BRIDGE_CMD_EXCHANGE_MESSAGE, bridge_kta_handle_exchange_message},
    {BRIDGE_CMD_KEYSTREAM_STATUS, bridge_kta_handle_keystream_status},
    {BRIDGE_CMD_GET_OBJECT_WITH_ASSOC, bridge_kta_handle_get_object_with_assoc},
    {BRIDGE_CMD_GET_OBJECT, bridge_kta_handle_get_object},
    {BRIDGE_CMD_SIGN_HASH, bridge_kta_handle_sign_hash},
    {BRIDGE_CMD_REFURBISH, bridge_kta_handle_refurbish},
};

#define CMD_TABLE_SIZE (sizeof(g_cmd_table) / sizeof(g_cmd_table[0]))

TransportStatus bridge_kta_handle_transport(const TransportMessage *request, TransportMessage *response)
{
    if (!request || !response)
        return TRANSPORT_ERROR_INVALID_PARAM;
    if (request->command_tag == 0)
        return TRANSPORT_ERROR_INVALID_MESSAGE;

    /* Initialize response */
    TransportStatus ustatus = transport_message_init(response, TRANSPORT_MSG_TYPE_RESPONSE);
    if (ustatus != TRANSPORT_SUCCESS)
        return ustatus;

    /* Set response command tag */
    uint8_t cmd_tag = request->command_tag;
    ustatus = transport_message_set_command(response, cmd_tag);
    if (ustatus != TRANSPORT_SUCCESS)
        return ustatus;

    /* Look up handler in descriptor table */
    for (uint8_t i = 0; i < CMD_TABLE_SIZE; i++)
    {
        if (g_cmd_table[i].cmd_tag == cmd_tag)
        {
            return g_cmd_table[i].handler(request, response);
        }
    }

    /* Command not found */
    return TRANSPORT_ERROR_INVALID_MESSAGE;
}
