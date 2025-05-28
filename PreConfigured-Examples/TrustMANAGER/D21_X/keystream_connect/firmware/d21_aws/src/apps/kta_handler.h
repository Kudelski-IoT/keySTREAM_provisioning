/** \brief  Handler for keySTREAM Trusted Agent to integrate with
 *          keySTREAM aws connect application.
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file kta_handler.h
 ******************************************************************************/

/**
 * @brief  Handler for keySTREAM Trusted Agent to integrate with 
 *         keySTREAM aws connect application.
 */

#ifndef KTA_HANDLER_H
#define KTA_HANDLER_H

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "wdrv_winc_client_api.h"
#include "cloud_wifi_ecc_process.h"
#include "cloud_wifi_task.h"
#include "app.h"
#include "cryptoauthlib.h"
#include "ktaFieldMgntHook.h"

/* -------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                     */
/* -------------------------------------------------------------------------- */
// Overriding the functions
#define WDRV_WINC_SocketRegisterEventCallback     ktaSocketRegisterEventCallback
#define WDRV_WINC_SocketRegisterResolverCallback  ktaSocketRegisterResolverCallback
#define transfer_ecc_certs_to_winc                ktaTransferEccCertsToWinc
#define WDRV_WINC_IPUseDHCPSet                    ktaIPUseDHCPSet

/* -------------------------------------------------------------------------- */
/* VARIABLES                                                                  */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* FUNCTIONS                                                                  */
/* -------------------------------------------------------------------------- */
/**
 * @brief
 * Register an event callback for socket events.
 *
 * @param[in] handle
 *   Client handle obtained by a call to WDRV_WINC_Open.
 * @param[in] pfAppSocketCb
 *   Callback function for socket events.
 *
 * @return
 * - WDRV_WINC_STATUS_OK in case of success.
 * - Error in case of other.
 */
WDRV_WINC_STATUS ktaSocketRegisterEventCallback
(
  DRV_HANDLE handle,
  tpfAppSocketCb pfAppSocketCb
);

/**
 * @brief
 * Register an event callback for DNS resolver events.
 *
 * @param[in] handle
 *   Client handle obtained by a call to WDRV_WINC_Open.
 * @param[in] pfAppResolveCb
 *   Callback function DNS resolver events.
 *
 * @return
 * - WDRV_WINC_STATUS_OK in case of success.
 * - Error in case of other.
 */
WDRV_WINC_STATUS ktaSocketRegisterResolverCallback
(
  DRV_HANDLE handle,
  tpfAppResolveCb pfAppResolveCb
);

/**
 * @brief
 *   Enables the use of DHCP by the WINC. This removes any static IP
 *   configuration. The use of DHCP isn't applied to the WINC until a
 *   connection is formed.
 *
 * @param[in] handle
 *   Client handle obtained by a call to WDRV_WINC_Open.
 * @param[in] pfDHCPAddressEventCallback
 *   Callback function DNS  initialization.
 *
 * @return
 * - WDRV_WINC_STATUS_OK in case of success.
 * - Error in case of others.
 */
WDRV_WINC_STATUS ktaIPUseDHCPSet
(
  DRV_HANDLE handle,
  const WDRV_WINC_DHCP_ADDRESS_EVENT_HANDLER pfDHCPAddressEventCallback
);

/**
 * @brief
 *   Transfers ecc certificates to winc driver.
 *
 * @return
 * - WDRV_WINC_STATUS_OK in case of success.
 * - Error in case of others.
 */
int8_t ktaTransferEccCertsToWinc
(
  void
);

#endif // KTA_HANDLER_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
