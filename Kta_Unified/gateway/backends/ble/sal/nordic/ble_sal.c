/*******************************************************************************
*************************keySTREAM Trusted Agent ("KTA")************************

* (c) 2023-2026 Nagravision Sàrl

* Subject to your compliance with these terms, you may use the Nagravision Sàrl
* Software and any derivatives exclusively with Nagravision's products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may accompany
* Nagravision Software.

* Redistribution of this Nagravision Software in source or binary form is allowed
* and must include the above terms of use and the following disclaimer with the
* distribution and accompanying materials.

* THIS SOFTWARE IS SUPPLIED BY NAGRAVISION "AS IS". NO WARRANTIES, WHETHER EXPRESS,
* IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF
* NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE. IN NO
* EVENT WILL NAGRAVISION BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL
* OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO
* THE SOFTWARE, HOWEVER CAUSED, EVEN IF NAGRAVISION HAS BEEN ADVISED OF THE
* POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW,
* NAGRAVISION 'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY RELATED TO THIS
* SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY
* TO NAGRAVISION FOR THIS SOFTWARE.
********************************************************************************//**
 * @file ble_sal_nordic.c
 * @brief BLE SAL Implementation for Nordic nRF52/nRF53 (Gateway)
 * 
 * Nordic BLE implementation using SoftDevice stack.
 */

#include "../../ble_sal.h"
#include <string.h>

#ifdef NRF52_SERIES

#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "ble_gap.h"
#include "ble_gattc.h"
#include "nrf_log.h"

/* ============================================================================
 * BLE State
 * ============================================================================ */

static bool g_initialized = false;
static bool g_connected = false;
static uint16_t g_conn_handle = BLE_CONN_HANDLE_INVALID;
static uint16_t g_mtu = 23;

/* ============================================================================
 * BLE SAL Functions
 * ============================================================================ */

BleSalStatus ble_sal_init(void)
{
    if (g_initialized) {
        return BLE_SAL_OK;
    }
    
    ret_code_t err_code;
    
    /* Initialize SoftDevice */
    err_code = nrf_sdh_enable_request();
    if (err_code != NRF_SUCCESS) {
        NRF_LOG_ERROR("SoftDevice enable failed: %d", err_code);
        return BLE_SAL_ERROR;
    }
    
    /* Configure BLE stack */
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(1, &ram_start);
    if (err_code != NRF_SUCCESS) {
        NRF_LOG_ERROR("BLE configuration failed: %d", err_code);
        return BLE_SAL_ERROR;
    }
    
    err_code = nrf_sdh_ble_enable(&ram_start);
    if (err_code != NRF_SUCCESS) {
        NRF_LOG_ERROR("BLE enable failed: %d", err_code);
        return BLE_SAL_ERROR;
    }
    
    g_initialized = true;
    g_connected = false;
    
    NRF_LOG_INFO("BLE initialized successfully");
    return BLE_SAL_OK;
}

BleSalStatus ble_sal_deinit(void)
{
    if (!g_initialized) {
        return BLE_SAL_OK;
    }
    
    if (g_connected) {
        ble_sal_disconnect();
    }
    
    nrf_sdh_disable_request();
    g_initialized = false;
    
    NRF_LOG_INFO("BLE deinitialized");
    return BLE_SAL_OK;
}

BleSalStatus ble_sal_connect(const char *device_address)
{
    if (!g_initialized || device_address == NULL) {
        return BLE_SAL_INVALID_PARAM;
    }
    
    if (g_connected) {
        NRF_LOG_WARNING("Already connected");
        return BLE_SAL_OK;
    }
    
    /* Parse BLE address */
    ble_gap_addr_t peer_addr;
    peer_addr.addr_type = BLE_GAP_ADDR_TYPE_PUBLIC;
    
    if (sscanf(device_address, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
               &peer_addr.addr[5], &peer_addr.addr[4], &peer_addr.addr[3],
               &peer_addr.addr[2], &peer_addr.addr[1], &peer_addr.addr[0]) != 6) {
        NRF_LOG_ERROR("Invalid BLE address format");
        return BLE_SAL_INVALID_PARAM;
    }
    
    /* Configure connection parameters */
    ble_gap_conn_params_t conn_params;
    memset(&conn_params, 0, sizeof(conn_params));
    conn_params.min_conn_interval = MSEC_TO_UNITS(7.5, UNIT_1_25_MS);
    conn_params.max_conn_interval = MSEC_TO_UNITS(30, UNIT_1_25_MS);
    conn_params.slave_latency = 0;
    conn_params.conn_sup_timeout = MSEC_TO_UNITS(4000, UNIT_10_MS);
    
    /* Initiate connection */
    ret_code_t err_code = sd_ble_gap_connect(&peer_addr, NULL, &conn_params, 1);
    if (err_code != NRF_SUCCESS) {
        NRF_LOG_ERROR("Connection failed: %d", err_code);
        return BLE_SAL_ERROR;
    }
    
    NRF_LOG_INFO("Connecting to BLE device: %s", device_address);
    
    return BLE_SAL_OK;
}

BleSalStatus ble_sal_disconnect(void)
{
    if (!g_initialized) {
        return BLE_SAL_ERROR;
    }
    
    if (!g_connected || g_conn_handle == BLE_CONN_HANDLE_INVALID) {
        return BLE_SAL_OK;
    }
    
    ret_code_t err_code = sd_ble_gap_disconnect(g_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    if (err_code != NRF_SUCCESS) {
        NRF_LOG_ERROR("Disconnect failed: %d", err_code);
        return BLE_SAL_ERROR;
    }
    
    g_connected = false;
    g_conn_handle = BLE_CONN_HANDLE_INVALID;
    
    NRF_LOG_INFO("Disconnected from BLE device");
    return BLE_SAL_OK;
}

BleSalStatus ble_sal_write(const uint8_t *data, size_t length)
{
    if (!g_initialized || !g_connected || data == NULL || length == 0) {
        return BLE_SAL_INVALID_PARAM;
    }
    
    /* TODO: Implement GATT write characteristic */
    NRF_LOG_INFO("BLE write: %zu bytes", length);
    
    return BLE_SAL_ERROR; /* Stub - needs characteristic handle */
}

BleSalStatus ble_sal_read(uint8_t *buffer, size_t buffer_size, size_t *bytes_read, uint32_t timeout_ms)
{
    if (!g_initialized || !g_connected || buffer == NULL || bytes_read == NULL) {
        return BLE_SAL_INVALID_PARAM;
    }
    
    /* TODO: Implement GATT read with timeout */
    (void)buffer_size;
    (void)timeout_ms;
    
    *bytes_read = 0;
    return BLE_SAL_ERROR; /* Stub */
}

BleSalStatus ble_sal_is_connected(bool *connected)
{
    if (connected == NULL) {
        return BLE_SAL_INVALID_PARAM;
    }
    
    *connected = g_connected;
    return BLE_SAL_OK;
}

BleSalStatus ble_sal_get_mtu(uint16_t *mtu)
{
    if (mtu == NULL) {
        return BLE_SAL_INVALID_PARAM;
    }
    
    *mtu = g_mtu;
    return BLE_SAL_OK;
}

BleSalStatus ble_sal_scan_devices(BleSalScanCallback callback, uint32_t scan_duration_ms)
{
    if (!g_initialized || callback == NULL) {
        return BLE_SAL_INVALID_PARAM;
    }
    
    /* Configure scan parameters */
    ble_gap_scan_params_t scan_params;
    memset(&scan_params, 0, sizeof(scan_params));
    scan_params.active = 1;
    scan_params.interval = MSEC_TO_UNITS(100, UNIT_0_625_MS);
    scan_params.window = MSEC_TO_UNITS(50, UNIT_0_625_MS);
    scan_params.timeout = scan_duration_ms / 10; /* Convert to 10ms units */
    
    ret_code_t err_code = sd_ble_gap_scan_start(&scan_params, NULL);
    if (err_code != NRF_SUCCESS) {
        NRF_LOG_ERROR("Scan start failed: %d", err_code);
        return BLE_SAL_ERROR;
    }
    
    NRF_LOG_INFO("BLE scan started for %u ms", scan_duration_ms);
    return BLE_SAL_OK;
}

#endif /* NRF52_SERIES */
