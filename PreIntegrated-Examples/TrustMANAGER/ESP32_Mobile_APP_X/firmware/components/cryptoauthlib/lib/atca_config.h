/* ESP-IDF shim for Microchip CryptoAuthLib configuration.
 *
 * The upstream Harmony-generated atca_config.h includes MPLAB/Harmony
 * "definitions.h" which is not available in ESP-IDF builds.
 *
 * This header provides the minimal configuration/macros needed to compile the
 * vendored CryptoAuthLib sources under ESP-IDF.
 */
#ifndef ATCA_CONFIG_H
#define ATCA_CONFIG_H

#ifndef FEATURE_ENABLED
#define FEATURE_ENABLED  (1)
#endif

#ifndef FEATURE_DISABLED
#define FEATURE_DISABLED (0)
#endif

/* Enable I2C HAL and ATECC608 support by default for ESP builds.
 * These may also be provided via compiler definitions.
 */
#ifndef ATCA_HAL_I2C
#define ATCA_HAL_I2C
#endif

#ifndef ATCA_ATECC608_SUPPORT
#define ATCA_ATECC608_SUPPORT
#endif

#ifndef MAX_PACKET_SIZE
#define MAX_PACKET_SIZE (1073U)
#endif

#ifndef ATCA_NO_HEAP
#define ATCA_NO_HEAP
#endif

#ifndef ATCA_CHECK_PARAMS_EN
#define ATCA_CHECK_PARAMS_EN (FEATURE_ENABLED)
#endif

/* Enable software crypto implementations for SHA256/HMAC */
#ifndef ATCAC_SHA256_EN
#define ATCAC_SHA256_EN (FEATURE_ENABLED)
#endif

#ifndef ATCAC_SHA256_HMAC_EN
#define ATCAC_SHA256_HMAC_EN (FEATURE_ENABLED)
#endif

/* Delay after a wake pulse/transaction as required by some devices/buses. */
#ifndef ATCA_POST_DELAY_MSEC
#define ATCA_POST_DELAY_MSEC (25)
#endif

/* NOTE: Do NOT alias atca_delay_ms/atca_delay_us to hal_delay_ms/hal_delay_us here.
 * hal_delay_esp_idf.c already provides atca_delay_ms/atca_delay_us (the ESP-IDF
 * delay implementations) AND hal_delay_ms/hal_delay_us (thin wrappers) as distinct
 * symbols. Aliasing them with macros makes atca_hal.h skip the atca_delay_* decls
 * (via its "#ifndef atca_delay_ms" guard) and rewrites the atca_delay_* definitions
 * onto hal_delay_*, producing a duplicate definition of hal_delay_ms/hal_delay_us
 * in the same translation unit (redefinition error).
 */

#endif /* ATCA_CONFIG_H */
