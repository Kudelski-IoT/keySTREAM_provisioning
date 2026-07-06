/*******************************************************************************
*************************keySTREAM Trusted Agent ("KTA")************************

* (c) 2023-2025 Nagravision Sàrl

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
********************************************************************************/
/** \brief  keySTREAM Trusted Agent - Hook for keySTREAM Trusted Agent
 *          initialization and field management.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file ktaFieldMgntHook.c
 ******************************************************************************/

/**
 * @brief keySTREAM Trusted Agent - Hook for field management integration.
 */

#include "ktaFieldMgntHook.h"

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "platform/include/kta_async_client.h"
#include "../../App_Config.h"
#include "../../keyStreamIntegration/COMMSTACK/http/include/comm_if.h"
#include "KTALog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Platform-specific sleep */
#ifdef _WIN32
#include <windows.h>
#define SLEEP_MS(ms) Sleep(ms)
#else
#include <unistd.h>
#define SLEEP_MS(ms) usleep((ms) * 1000)
#endif

/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */

/* ============================================================================
 * Configuration Constants (Read-Only, placed in Flash/ROM)
 * ============================================================================ */

/** @brief Device Serial Number */
static const uint8_t C_KTA_APP_DEVICE_SERIAL_NUM[] = {
    0x22, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
#define C_KTA_APP_DEVICE_SERIAL_NUM_LEN (sizeof(C_KTA_APP_DEVICE_SERIAL_NUM))

/** @brief Context profile UID */
static const uint8_t C_KTA_APP_CONTEXT_PROFILE_UID[] = {
    0x11, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a};
#define C_KTA_APP_CONTEXT_PROFILE_UID_LEN (sizeof(C_KTA_APP_CONTEXT_PROFILE_UID))

/** @brief Serial No */
static const uint8_t C_KTA_APP_CONTEXT_SERIAL_NUM[] = {
    0x11, 0x22, 0x33, 0x04, 0x05, 0x06, 0x07, 0x08};
#define C_KTA_APP_CONTEXT_SERIAL_NUM_LEN (sizeof(C_KTA_APP_CONTEXT_SERIAL_NUM))

/** @brief Context Version */
static const uint8_t C_KTA_APP_CONTEXT_VERSION[] = {
    0x22, 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x04, 0x00, 0x05};
#define C_KTA_APP_CONTEXT_VERSION_LEN (sizeof(C_KTA_APP_CONTEXT_VERSION))

/** @brief L1 Segmentation Seed — must match C_KTA_APP__L1_SEG_SEED in ktaConfig.h */
static const uint8_t C_KTA_APP__L1_SEG_SEED_DEFAULT[] = {
    0x2b, 0x2b, 0x42, 0x6e, 0x10, 0x35, 0xad, 0x6b,
    0x73, 0xf0, 0x56, 0x1d, 0xc4, 0xe0, 0x54, 0x72};

/** @brief Maximum message exchange iterations (reduced for low-end devices) */
#define C_KTA_MAX_EXCHANGES (10u)

/** @brief Maximum ICPP message size (reduced to 2KB for low-end devices) */
#define C_K__ICPP_MSG_MAX_SIZE (2048u)

/** @brief Operation timeout in milliseconds */
#define C_KTA_OPERATION_TIMEOUT_MS (30000u)

/** @brief Poll interval for response waiting (reduced CPU usage) */
#define C_KTA_POLL_INTERVAL_MS (50u)

/* All log output is routed through the standard KTALog framework (KTALog.h).
 * Activate logging by defining LOG_KTA_ENABLE in ktaConfig.h.
 * The legacy KTA_ENABLE_LOGGING compile flag is still accepted for backward
 * compatibility but routing now always goes through M_KTALOG__INF so that log
 * level, module filtering, and output sink are controlled in one place. */
#define C_KTA_APP__LOG(fmt, ...) M_KTALOG__INFO(gpModuleName, fmt, ##__VA_ARGS__)

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */

/** @brief Module name for the KTA log module (gated by LOG_KTA_ENABLE in
 *         ktaConfig.h). When logging is disabled the M_KTALOG__* macros
 *         expand to nothing and this stays unused. */
static const char *gpModuleName = "KTAMGR";

/** @brief Connection required by KTA status */
static bool gConnectionReq = false;

/** @brief KTA Initialized state */
static bool gKtaInitialized = false;

/** @brief Segmentation seed (mutable, initialized from ROM) */
static uint8_t gaSegSeed[16];

/** @brief Device profile public UID pointer (points to ROM by default) */
static const char *gpDeviceProfPubUid = C_KTA_APP__DEVICE_PUBLIC_UID;

/** @brief Async client instance */
static KtaAsyncClient g_client;

/** @brief Response tracking (optimized for low-end devices)
 *
 * THREAD SAFETY: Access to g_response_buffer / g_response_len /
 * g_last_status / g_waiting_for_response must be serialized between the
 * main thread (poller) and the async I/O callback thread. We use a small
 * portable mutex wrapper below to avoid torn reads / writes and to give
 * the main thread a consistent view of the response after the wait
 * returns. */
static volatile bool g_waiting_for_response = false;
static volatile TKStatus g_last_status = E_K_STATUS_ERROR;
static uint8_t g_response_buffer[C_K__ICPP_MSG_MAX_SIZE];
static uint16_t g_response_len = 0; /* uint16_t sufficient for 2KB max */

/** @brief Last bridge transport status code returned by the MCU.
 *  Diagnostics only: -1000 = no response yet, -2000 = transport/error string
 *  reported by the async client, otherwise the raw bridge status_code. */
static volatile int g_last_bridge_status_code = -1000;
/** @brief Copy of the last error string from the async client (diagnostics). */
static char g_last_error_msg[128] = {0};

/* ---- Portable mutex wrapper ------------------------------------------------ */
#if defined(_WIN32)
#include <windows.h>
static CRITICAL_SECTION g_response_lock;
static bool g_response_lock_init = false;
static void response_lock_init(void)
{
  if (!g_response_lock_init)
  {
    InitializeCriticalSection(&g_response_lock);
    g_response_lock_init = true;
  }
}
static void response_lock_take(void)
{
  if (g_response_lock_init)
    EnterCriticalSection(&g_response_lock);
}
static void response_lock_give(void)
{
  if (g_response_lock_init)
    LeaveCriticalSection(&g_response_lock);
}
#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#include <pthread.h>
static pthread_mutex_t g_response_lock = PTHREAD_MUTEX_INITIALIZER;
static void response_lock_init(void) { /* statically initialized */ }
static void response_lock_take(void) { (void)pthread_mutex_lock(&g_response_lock); }
static void response_lock_give(void) { (void)pthread_mutex_unlock(&g_response_lock); }
#elif defined(FREERTOS) || defined(INC_FREERTOS_H)
#include "FreeRTOS.h"
#include "semphr.h"
static SemaphoreHandle_t g_response_lock = NULL;
static void response_lock_init(void)
{
  if (NULL == g_response_lock)
    g_response_lock = xSemaphoreCreateMutex();
}
static void response_lock_take(void)
{
  if (NULL != g_response_lock)
    (void)xSemaphoreTake(g_response_lock, portMAX_DELAY);
}
static void response_lock_give(void)
{
  if (NULL != g_response_lock)
    (void)xSemaphoreGive(g_response_lock);
}
#else
/* Bare-metal / single-threaded fallback: no real lock available. */
static void response_lock_init(void) {}
static void response_lock_take(void) {}
static void response_lock_give(void) {}
#endif

/** @brief Initialization flag for one-time setup */
static bool g_module_initialized = false;

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - PROTOTYPE                                                */
/* -------------------------------------------------------------------------- */

/**
 * @brief
 *   Set startup information for KTA.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lsetStartupInfo(void);

/**
 * @brief
 *   Set device information for KTA.
 *
 * @param[in,out] xpConnectionReq
 *   [in] Pointer to buffer for connection request flag.
 *   [out] Connection request flag from server.
 *   Should not be NULL.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lsetDeviceInfo(bool *xpConnectionReq);

/**
 * @brief
 *   Poll keySTREAM server for message exchange.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lPollKeyStream(void);

/* -------------------------------------------------------------------------- */
/* CALLBACK                                                                   */
/* -------------------------------------------------------------------------- */

/**
 * @brief
 *   Response callback handler for KTA async operations.
 *
 * @param[in] xpRequest
 *   Original request structure.
 * @param[in] xpResponse
 *   Response structure from MCU (NULL if error).
 * @param[in] xpError
 *   Error message string (NULL if success).
 * @param[in] xpUserData
 *   User-provided context pointer.
 *
 * @return
 *   None.
 */
static void on_response_callback(const KtaRequest *xpRequest, const KtaResponse *xpResponse,
                                 const char *xpError, void *xpUserData)
{
  (void)xpRequest;
  (void)xpUserData;

  response_lock_take();

  if (NULL != xpError)
  {
    g_last_status = E_K_STATUS_ERROR;
    g_response_len = 0U;
    g_last_bridge_status_code = -2000;
    (void)strncpy(g_last_error_msg, xpError, sizeof(g_last_error_msg) - 1U);
    g_last_error_msg[sizeof(g_last_error_msg) - 1U] = '\0';
    g_waiting_for_response = false;
    response_lock_give();
    M_KTALOG__ERR("MCU reported transport error: %s", xpError);
    return;
  }

  if (NULL == xpResponse)
  {
    g_last_status = E_K_STATUS_ERROR;
    g_response_len = 0U;
    g_last_bridge_status_code = -2000;
    (void)strncpy(g_last_error_msg, "null response", sizeof(g_last_error_msg) - 1U);
    g_last_error_msg[sizeof(g_last_error_msg) - 1U] = '\0';
    g_waiting_for_response = false;
    response_lock_give();
    M_KTALOG__ERR("MCU returned a NULL response (no payload, no status)");
    return;
  }

  /* Always copy any returned payload, regardless of the bridge status code, so
   * the caller can still inspect data that accompanies a non-zero status
   * (e.g. the keySTREAM command status byte returned by ktaKeyStreamStatus).
   * Size is already bounded by the sender, double-check anyway. */
  if ((xpResponse->data_len > 0U) && (xpResponse->data_len <= C_K__ICPP_MSG_MAX_SIZE))
  {
    (void)memcpy(g_response_buffer, xpResponse->data, xpResponse->data_len);
    g_response_len = (uint16_t)xpResponse->data_len;
  }
  else
  {
    g_response_len = 0U;
  }

  /* status_code 0 == success at the bridge transport level. */
  g_last_bridge_status_code = (int)xpResponse->status_code;
  g_last_error_msg[0] = '\0';
  g_last_status = (0 == xpResponse->status_code) ? E_K_STATUS_OK : E_K_STATUS_ERROR;

  g_waiting_for_response = false;
  response_lock_give();

  if (0 != xpResponse->status_code)
  {
    M_KTALOG__ERR("MCU returned non-zero bridge status_code=%d (payload %u bytes)",
                  (int)xpResponse->status_code, (unsigned)xpResponse->data_len);
  }
}

/* -------------------------------------------------------------------------- */
/* HELPER FUNCTIONS                                                           */
/* -------------------------------------------------------------------------- */

/**
 * @brief
 *   Wait for response from async operation with timeout.
 *
 * @param[in] xTimeoutMs
 *   Timeout in milliseconds.
 *
 * @return
 * - E_K_STATUS_OK if response received successfully.
 * - E_K_STATUS_ERROR if timeout or error occurred.
 */
static TKStatus wait_for_response(uint32_t xTimeoutMs)
{
  uint32_t elapsed = 0U;
  bool waiting = true;

  /* Use longer poll interval to reduce CPU usage on low-end devices */
  while (waiting && (elapsed < xTimeoutMs))
  {
    SLEEP_MS(C_KTA_POLL_INTERVAL_MS);
    elapsed += C_KTA_POLL_INTERVAL_MS;

    response_lock_take();
    waiting = g_waiting_for_response;
    response_lock_give();
  }

  TKStatus result;
  response_lock_take();
  if (g_waiting_for_response)
  {
    g_waiting_for_response = false;
    result = E_K_STATUS_ERROR; /* Timeout */
  }
  else
  {
    result = g_last_status;
  }
  response_lock_give();

  return result;
}

/**
 * @brief
 *   Send request and wait for response.
 *
 * @param[in] xReqId
 *   Request ID from async operation.
 * @param[in] xpApiName
 *   API name for logging (may be NULL).
 *
 * @return
 * - E_K_STATUS_OK if request completed successfully.
 * - E_K_STATUS_ERROR if request failed or timeout.
 */
static TKStatus send_request_and_wait(uint32_t xReqId, const char *xpApiName)
{
  const char *apiName = (NULL != xpApiName) ? xpApiName : "?";

  if (0U == xReqId)
  {
    M_KTALOG__ERR("%s: request was not queued/sent (req_id=0) - serialize or "
                  "UART backend_send failed; MCU never received the command", apiName);
    return E_K_STATUS_ERROR;
  }

  response_lock_init();
  response_lock_take();
  g_waiting_for_response = true;
  g_response_len = 0U;
  g_last_bridge_status_code = -1000;
  g_last_error_msg[0] = '\0';
  response_lock_give();

  TKStatus result = wait_for_response(C_KTA_OPERATION_TIMEOUT_MS);

  if (E_K_STATUS_OK != result)
  {
    response_lock_take();
    int bridgeStatus = g_last_bridge_status_code;
    char errCopy[128];
    (void)strncpy(errCopy, g_last_error_msg, sizeof(errCopy) - 1U);
    errCopy[sizeof(errCopy) - 1U] = '\0';
    response_lock_give();

    if (-1000 == bridgeStatus)
    {
      M_KTALOG__ERR("%s: TIMEOUT after %u ms - no response from MCU. Check that "
                    "the device firmware is running and answering on the UART.",
                    apiName, (unsigned)C_KTA_OPERATION_TIMEOUT_MS);
    }
    else if (-2000 == bridgeStatus)
    {
      M_KTALOG__ERR("%s: async client error: %s", apiName, errCopy);
    }
    else
    {
      M_KTALOG__ERR("%s: MCU rejected the command (bridge status_code=%d)",
                    apiName, bridgeStatus);
    }
  }

  return result;
}

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief implement ktaKeyStreamInit
 *
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * Using goto for breaking during the error and return cases.
 */
TKStatus ktaKeyStreamInit(void)
{
  TKStatus retStatus = E_K_STATUS_ERROR;

  /* One-time module initialization */
  if (false == g_module_initialized)
  {
    /* The response lock guards g_response_buffer / g_response_len /
     * g_waiting_for_response, all of which are touched from the async
     * client's RX thread inside on_response_callback. The lock MUST exist
     * before kta_async_client_start() is called below, otherwise an early
     * inbound frame could race with the lazy init in send_request_and_wait. */
    response_lock_init();

    /* Initialize segmentation seed from ROM */
    (void)memcpy(gaSegSeed, C_KTA_APP__L1_SEG_SEED_DEFAULT, 16U);
    g_module_initialized = true;
  }

  if (false == gKtaInitialized)
  {
    /* ----------------------------------------------------------------
     * Transport layer: only (re-)open UART and start the receive thread
     * when the client is not already running.  The receive thread sets
     * g_client.is_running = false when it exits due to an error, so this
     * block also handles automatic reconnection after a UART fault.
     * ---------------------------------------------------------------- */
    if (false == g_client.is_running)
    {
      M_KTALOG__INFO("Transport: opening UART and starting RX thread");

      /* Clean up any stale context from a previous session (thread may have
       * exited itself but platform_thread_context still holds stale handles). */
      (void)kta_async_client_stop(&g_client);

      BackendStatus status = kta_async_client_init(&g_client, true);
      if (BACKEND_OK != status)
      {
        M_KTALOG__ERR("Transport: kta_async_client_init failed (BackendStatus %d) "
                      "- check UART/COM port (busy, missing, or permission denied)",
                      (int)status);
        retStatus = E_K_STATUS_ERROR;
        goto end;
      }

      (void)kta_async_client_set_callback(&g_client, on_response_callback, NULL);

      status = kta_async_client_start(&g_client);
      if (BACKEND_OK != status)
      {
        M_KTALOG__ERR("Transport: kta_async_client_start failed (BackendStatus %d) "
                      "- could not start RX thread / open port", (int)status);
        (void)kta_async_client_deinit(&g_client);
        retStatus = E_K_STATUS_ERROR;
        goto end;
      }

      /* Wait for connection with timeout (5 seconds) */
      uint32_t conn_timeout = 5000U;
      uint32_t elapsed = 0U;
      while ((false == kta_async_is_connected(&g_client)) && (elapsed < conn_timeout))
      {
        SLEEP_MS(100U);
        elapsed += 100U;
      }

      if (false == kta_async_is_connected(&g_client))
      {
        M_KTALOG__ERR("Transport: not connected after %u ms timeout "
                      "- MCU not responding on the UART (check cable, COM port, "
                      "and that the device firmware is running)", (unsigned)conn_timeout);
        (void)kta_async_client_deinit(&g_client);
        retStatus = E_K_STATUS_ERROR;
        goto end;
      }

      M_KTALOG__INFO("Transport: connected after %u ms", (unsigned)elapsed);

      /* Allow MCU to boot / initialize ATECC608 after DTR reset */
      SLEEP_MS(2000U);
    }
    else
    {
      M_KTALOG__INFO("Transport: client already running, reusing connection");
    }

    /* ----------------------------------------------------------------
     * KTA protocol layer: always re-run Initialize → Startup →
     * SetDeviceInfo on every poll cycle.  This forces the MCU to
     * regenerate a fresh ICPP message in ktaExchangeMessage, giving
     * keySTREAM the opportunity to push server-initiated commands
     * (REFURBISH, key delivery, etc.) even when the device is already
     * activated.
     *
     * NOTE: REFURBISH is intentionally NOT sent here. Refurbish is a full
     * factory wipe of the ATECC608 lifecycle/credentials and must only run
     * in response to keySTREAM returning status 0x04 (E_K_KTA_KS_STATUS_
     * REFURBISH), with the documented reset + ~5s settle before re-init.
     * Sending it before every ktaInitialize left the MCU mid-wipe and caused
     * ktaInitialize to fail (-1). */

    uint32_t req_id = kta_async_initialize(&g_client);
    retStatus = send_request_and_wait(req_id, "ktaInitialize");

    if (E_K_STATUS_OK != retStatus)
    {
      M_KTALOG__ERR("ktaInitialize failed (status %d)", retStatus);
      goto end;
    }
    M_KTALOG__INFO("ktaInitialize OK");

    /* Step 2: ktaStartup */
    retStatus = lsetStartupInfo();

    if (E_K_STATUS_OK != retStatus)
    {
      M_KTALOG__ERR("ktaStartup failed (status %d)", retStatus);
      goto end;
    }
    M_KTALOG__INFO("ktaStartup OK");

    /* Step 3: ktaSetDeviceInfo */
    retStatus = lsetDeviceInfo(&gConnectionReq);

    if (E_K_STATUS_OK != retStatus)
    {
      M_KTALOG__ERR("ktaSetDeviceInfo failed (status %d)", retStatus);
      goto end;
    }
    M_KTALOG__INFO("ktaSetDeviceInfo OK (connReq=%u)", (unsigned)gConnectionReq);

    gKtaInitialized = true;
  }

  retStatus = E_K_STATUS_OK;

end:
  return retStatus;
}

/**
 * @brief implement ktaKeyStreamFieldMgmt
 *
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * Using goto for breaking during the error and return cases.
 */
TKStatus ktaKeyStreamFieldMgmt(
    bool xIsFieldMgmtReq,
    TKktaKeyStreamStatus *xpKtaKSCmdStatus)
{
  TKStatus retStatus = E_K_STATUS_ERROR;

  if (NULL == xpKtaKSCmdStatus)
  {
    retStatus = E_K_STATUS_PARAMETER;
    goto end;
  }

  /* Run KTA init sequence every cycle:
   *  - transport (UART + thread) only when not already running
   *  - KTA protocol (Initialize/Startup/SetDeviceInfo) always, so the MCU
   *    generates a fresh ICPP message and keySTREAM can push any pending
   *    commands (REFURBISH, key delivery, etc.) */
  retStatus = ktaKeyStreamInit();
  if (E_K_STATUS_OK != retStatus)
  {
    goto end;
  }

  /* Always run the exchange loop regardless of gConnectionReq.
   * - SEALED/CON_REQ  : provisioning exchanges.
   * - ACTIVATED/PROVISIONED : field-management exchanges (key delivery,
   *   cert rotation, renew, rotate). If nothing is pending the MCU returns
   *   empty on the first call and lPollKeyStream exits immediately. */
  retStatus = lPollKeyStream();

  if (E_K_STATUS_OK != retStatus)
  {
    goto end;
  }

  /* Query the final keySTREAM command status.
   *
   * The provisioning / field-management exchange above (lPollKeyStream) has
   * already completed successfully at this point, so this query is purely
   * informational: it tells us whether keySTREAM wants a RENEW / REFURBISH or
   * nothing (NO_OPERATION).
   *
   * Honor whatever command status the MCU returned in the payload even if the
   * bridge wrapper reports a non-zero transport status, and fall back to
   * NO_OPERATION when no payload is available. This prevents a transient
   * status-query hiccup from aborting an otherwise successful cycle and
   * tearing down the connection. */
  uint32_t req_id = kta_async_keystream_status(&g_client);
  (void)send_request_and_wait(req_id, "ktaKeyStreamStatus");

  /* Extract status from response */
  if (g_response_len > 0U)
  {
    *xpKtaKSCmdStatus = (TKktaKeyStreamStatus)g_response_buffer[0];
  }
  else
  {
    *xpKtaKSCmdStatus = E_K_KTA_KS_STATUS_NO_OPERATION;
  }

  retStatus = E_K_STATUS_OK;

  /* Always reset the initialized flag so the next poll re-runs the full
   * Initialize → Startup → SetDeviceInfo → Exchange sequence. This forces
   * the MCU to generate a fresh ICPP message on every poll cycle, giving
   * keySTREAM the opportunity to push server-initiated commands (REFURBISH,
   * RENEW, key delivery) even after the device is already activated.
   * Without this, ktaExchangeMessage(NULL,0) returns empty post-RENEW and
   * keySTREAM can never deliver its queued commands. */
  gKtaInitialized = false;

end:
  return retStatus;
}

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */

/**
 * @brief implement lsetStartupInfo
 *
 * @note Uses const ROM data directly to avoid stack copies
 */
static TKStatus lsetStartupInfo(void)
{

  uint32_t req_id = kta_async_startup(&g_client,
                                      gaSegSeed,
                                      C_KTA_APP_CONTEXT_PROFILE_UID, C_KTA_APP_CONTEXT_PROFILE_UID_LEN,
                                      C_KTA_APP_CONTEXT_SERIAL_NUM, C_KTA_APP_CONTEXT_SERIAL_NUM_LEN,
                                      C_KTA_APP_CONTEXT_VERSION, C_KTA_APP_CONTEXT_VERSION_LEN);

  return send_request_and_wait(req_id, "ktaStartup");
}

/**
 * @brief implement lsetDeviceInfo
 *
 * @note Uses const ROM data directly to avoid stack copies
 */
static TKStatus lsetDeviceInfo(bool *xpConnectionReq)
{
  size_t deviceProfPubUidLen = strlen(gpDeviceProfPubUid);
  TKStatus retStatus;

  if (NULL == xpConnectionReq)
  {
    return E_K_STATUS_PARAMETER;
  }

  uint32_t req_id = kta_async_set_device_info(&g_client,
                                              (const uint8_t *)gpDeviceProfPubUid,
                                              deviceProfPubUidLen,
                                              C_KTA_APP_DEVICE_SERIAL_NUM,
                                              C_KTA_APP_DEVICE_SERIAL_NUM_LEN);

  retStatus = send_request_and_wait(req_id, "ktaSetDeviceInformation");

  if ((E_K_STATUS_OK == retStatus) && (g_response_len > 0U))
  {
    *xpConnectionReq = (0U != g_response_buffer[0]) ? true : false;
  }
  else
  {
    /* If no CONN_REQUEST field, assume connection is needed for first provision */
    *xpConnectionReq = true;
  }

  return retStatus;
}

/**
 * @brief implement lPollKeyStream
 *
 * Exchanges KTA messages with the MCU via UART and relays them
 * to/from the keySTREAM server over HTTP until no more messages remain.
 */
static TKStatus lPollKeyStream(void)
{
  TKStatus retStatus = E_K_STATUS_ERROR;
  const uint8_t *pKs2RotMsg = NULL;
  uint16_t ks2rotMsgSize = 0U;
  uint8_t exchange_count = 0U;

  /* HTTP response buffer — large enough for keySTREAM responses.
   * Kept as static so the pointer in pKs2RotMsg stays valid across
   * iterations.  Zero-init before each use to enable safe string print. */
  static uint8_t s_ks_resp_buf[C_K__ICPP_MSG_MAX_SIZE];

  /* Open ONE TCP connection for the whole provisioning session.
   * The HTTP layer propagates the server's Set-Cookie into the next
   * request cookie automatically, so we must reuse the same connection. */
  TCommIfStatus commStatus = commInit(
      C_K_COMM__SERVER_HOST,
      C_K_COMM__SERVER_PORT,
      (const uint8_t *)C_K_COMM__SERVER_URI);

  if (commStatus != E_COMM_IF_STATUS_OK)
  {
    return E_K_STATUS_ERROR;
  }

  while (exchange_count < C_KTA_MAX_EXCHANGES)
  {
    /* Step 1: ask MCU for the next KTA message to forward */
    uint32_t req_id = kta_async_exchange_message(&g_client,
                                                 pKs2RotMsg,
                                                 (size_t)ks2rotMsgSize);

    retStatus = send_request_and_wait(req_id, "ktaExchangeMessage");

    if (retStatus != E_K_STATUS_OK)
    {
      M_KTALOG__ERR("ktaExchangeMessage failed (status %d)", retStatus);
      break;
    }

    /* Empty response from MCU means the exchange is complete */
    if (g_response_len == 0U)
    {
      M_KTALOG__INFO("Exchange complete (MCU returned empty message)");
      retStatus = E_K_STATUS_OK;
      break;
    }

    /* Print hex dump of bytes being sent to keySTREAM */
    printf("Sending [%u bytes]:\n", g_response_len);
    for (uint16_t _i = 0U; _i < g_response_len; _i++)
    {
      if ((_i % 16U) == 0U)
        printf("  %04X: ", _i);
      printf("%02X ", g_response_buffer[_i]);
      if ((_i % 16U) == 15U)
        printf("\n");
    }
    if ((g_response_len % 16U) != 0U)
      printf("\n");

    /* Step 2: POST to keySTREAM — zero-init buffer so error body is
     * null-terminated and can be printed as a string on failure. */
    (void)memset(s_ks_resp_buf, 0, sizeof(s_ks_resp_buf));
    size_t ks_resp_size = sizeof(s_ks_resp_buf);
    commStatus = commMsgExchange(
        g_response_buffer,
        (size_t)g_response_len,
        s_ks_resp_buf,
        &ks_resp_size);

    if (commStatus != E_COMM_IF_STATUS_OK)
    {
      /* Print the raw server error body so we can see the exact reason */
      size_t err_len = strnlen((const char *)s_ks_resp_buf, sizeof(s_ks_resp_buf));
      if (err_len > 0U)
      {
        printf("Server error (%zu bytes):\n", err_len);
        fwrite(s_ks_resp_buf, 1U, err_len, stdout);
        printf("\n");
      }
      M_KTALOG__ERR("commMsgExchange failed (status %d)", commStatus);
      retStatus = E_K_STATUS_ERROR;
      break;
    }

    printf("Received [%zu bytes]:\n", ks_resp_size);

    for (size_t _j = 0U; _j < ks_resp_size; _j++)
    {
      if ((_j % 16U) == 0U)
        printf("  %04zX: ", _j);
      printf("%02X ", s_ks_resp_buf[_j]);
      if ((_j % 16U) == 15U)
        printf("\n");
    }
    if ((ks_resp_size % 16U) != 0U)
      printf("\n");

    /* Step 3: pass keySTREAM response back to MCU in next iteration */
    pKs2RotMsg = s_ks_resp_buf;
    ks2rotMsgSize = (uint16_t)(ks_resp_size <= 0xFFFFU ? ks_resp_size : 0xFFFFU);

    exchange_count++;
  }

  if (exchange_count >= C_KTA_MAX_EXCHANGES)
  {
    C_KTA_APP__LOG("Warning: Maximum exchanges reached\n");
  }

  (void)commTerm();
  return retStatus;
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
