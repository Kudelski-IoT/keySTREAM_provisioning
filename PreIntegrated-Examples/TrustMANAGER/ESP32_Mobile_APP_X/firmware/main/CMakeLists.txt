# Unified CMakeLists.txt — main app + KTA library + CryptoAuthLib

set(COMP_DIR "${CMAKE_CURRENT_LIST_DIR}/../components")
set(KTA_PROV "${CMAKE_CURRENT_LIST_DIR}/kta_provisioning")
set(CAL_DIR "${COMP_DIR}/cryptoauthlib")
set(CAL_LIB "${CAL_DIR}/lib")
set(MCU_DIR "${KTA_PROV}/mcu")

# ===========================================================================
# Parse mcu_config.h — single source of truth for OS / BACKEND / PLATFORM
# ===========================================================================
set(MCU_CONFIG_H "${MCU_DIR}/mcu_config.h")

file(STRINGS "${MCU_CONFIG_H}" _OS_LINE       REGEX "^[ \t]*#define[ \t]+MCU_OS[ \t]+")
file(STRINGS "${MCU_CONFIG_H}" _BACKEND_LINE  REGEX "^[ \t]*#define[ \t]+MCU_BACKEND[ \t]+")
file(STRINGS "${MCU_CONFIG_H}" _PLATFORM_LINE REGEX "^[ \t]*#define[ \t]+MCU_PLATFORM[ \t]+")

string(REGEX REPLACE ".*MCU_OS[ \t]+([A-Za-z0-9_]+).*"       "\\1" MCU_OS       "${_OS_LINE}")
string(REGEX REPLACE ".*MCU_BACKEND[ \t]+([A-Za-z0-9_]+).*"  "\\1" MCU_BACKEND  "${_BACKEND_LINE}")
string(REGEX REPLACE ".*MCU_PLATFORM[ \t]+([A-Za-z0-9_]+).*" "\\1" MCU_PLATFORM "${_PLATFORM_LINE}")

message(STATUS "MCU config — OS=${MCU_OS}  BACKEND=${MCU_BACKEND}  PLATFORM=${MCU_PLATFORM}")

# Derived paths (all relative to main/ for idf_component_register)
set(MCU_REL          "kta_provisioning/mcu")
set(MCU_BACKEND_PATH "${MCU_REL}/backends/${MCU_BACKEND}")
set(MCU_SAL_PATH     "${MCU_BACKEND_PATH}/sal/${MCU_PLATFORM}")
set(MCU_OS_PATH      "${MCU_REL}/examples/platform/${MCU_OS}")

# --- CryptoAuthLib sources ---
file(GLOB CAL_SRC_TOP "${CAL_LIB}/*.c")
file(GLOB CAL_SRC_CALIB "${CAL_LIB}/calib/*.c")
file(GLOB CAL_SRC_CRYPTO "${CAL_LIB}/crypto/*.c")
file(GLOB CAL_SRC_CRYPTO_HASHES "${CAL_LIB}/crypto/hashes/*.c")
file(GLOB CAL_SRC_ATCACERT "${CAL_LIB}/atcacert/*.c")
file(GLOB CAL_SRC_HOST "${CAL_LIB}/host/*.c")
file(GLOB CAL_SRC_CRYPTO_SW "${CAL_LIB}/crypto/atca_crypto_sw*.c")

set(CAL_SRCS
    ${CAL_SRC_TOP}
    ${CAL_SRC_CALIB}
    ${CAL_SRC_CRYPTO}
    ${CAL_SRC_CRYPTO_HASHES}
    ${CAL_SRC_CRYPTO_SW}
    ${CAL_SRC_ATCACERT}
    ${CAL_SRC_HOST}
    "${CAL_LIB}/hal/atca_hal.c"
    "${CAL_LIB}/hal/hal_i2c_esp_idf.c"
    "${CAL_LIB}/hal/hal_delay_esp_idf.c"
)

list(FILTER CAL_SRCS EXCLUDE REGEX ".*/hal/hal_.*harmony\\.c$")
list(FILTER CAL_SRCS EXCLUDE REGEX ".*/hal/hal_harmony_init\\.c$")
list(FILTER CAL_SRCS EXCLUDE REGEX ".*/hal/hal_gpio_harmony\\.c$")
list(FILTER CAL_SRCS EXCLUDE REGEX ".*/hal/hal_cortex_m_delay\\.c$")
list(FILTER CAL_SRCS EXCLUDE REGEX ".*/hal/ATECC608_0\\.c$")

# --- KTA library sources ---
file(GLOB KTA_SALAPI_SRCS "${KTA_PROV}/SOURCE/salapi/*.c")

set(KTA_SRCS
    "${KTA_PROV}/COMMON/logmodule/KTALog.c"
    "cust_def_device.c"
    "cust_def_signer.c"
    ${KTA_SALAPI_SRCS}
    "${KTA_PROV}/SOURCE/kta/ktamgr/ktamgr.c"
    "${KTA_PROV}/SOURCE/kta/common/crypto/crypto.c"
    "${KTA_PROV}/SOURCE/kta/common/general/general.c"
    "${KTA_PROV}/SOURCE/kta/common/icpp_parser/icpp_parser.c"
    "${KTA_PROV}/SOURCE/kta/common/version/version.c"
    "${KTA_PROV}/SOURCE/kta/modules/acthandler/acthandler.c"
    "${KTA_PROV}/SOURCE/kta/modules/cmdhandler/cmdhandler.c"
    "${KTA_PROV}/SOURCE/kta/modules/config/config.c"
    "${KTA_PROV}/SOURCE/kta/modules/fotaservice/fotaagent.c"
    "${KTA_PROV}/SOURCE/kta/modules/fotaservice/fotaplatform.c"
    "${KTA_PROV}/SOURCE/kta/modules/fotaservice/fotaprocess.c"
    "${KTA_PROV}/SOURCE/kta/modules/reghandler/reghandler.c"
)

# --- Main app sources (MCU paths resolved from mcu_config.h) ---
set(MAIN_SRCS
    "main.c"
    "${MCU_REL}/bridgeKta/bridge_kta.c"
    "${MCU_REL}/backends/backend_interface.c"
    # "${MCU_BACKEND_PATH}/backend_${MCU_BACKEND}.c"
    # "${MCU_SAL_PATH}/${MCU_BACKEND}_sal.c"
    "${MCU_REL}/examples/common/bridge_integration.c"
    "${MCU_OS_PATH}/kta_mcu_integration_${MCU_OS}.c"
)

# --- Register single component ---
idf_component_register(
    SRCS ${MAIN_SRCS} ${KTA_SRCS} ${CAL_SRCS}
    INCLUDE_DIRS
        "."
        "${CMAKE_CURRENT_LIST_DIR}/../../"
        "${KTA_PROV}/SOURCE/include"
        "${KTA_PROV}/SOURCE/salapi/include"
        "${KTA_PROV}/COMMON/logmodule/include"
        "${KTA_PROV}/SOURCE/kta/common/crypto/include"
        "${KTA_PROV}/SOURCE/kta/common/general/include"
        "${KTA_PROV}/SOURCE/kta/common/icpp_parser/include"
        "${KTA_PROV}/SOURCE/kta/common/version/include"
        "${KTA_PROV}/SOURCE/kta/modules/acthandler/include"
        "${KTA_PROV}/SOURCE/kta/modules/cmdhandler/include"
        "${KTA_PROV}/SOURCE/kta/modules/config/include"
        "${KTA_PROV}/SOURCE/kta/modules/fotaservice/include"
        "${KTA_PROV}/SOURCE/kta/modules/reghandler/include"
        "${MCU_REL}/bridgeKta"
        "${MCU_REL}/backends"
        "${MCU_BACKEND_PATH}"
        "${MCU_SAL_PATH}"
        "${MCU_REL}/examples/common"
        "${MCU_REL}/examples/platform"
        "${MCU_OS_PATH}"
        "${CAL_LIB}"
        "${CAL_LIB}/atcacert"
        "${CAL_LIB}/calib"
        "${CAL_LIB}/crypto"
        "${CAL_LIB}/crypto/hashes"
        "${CAL_LIB}/hal"
        "${CAL_LIB}/host"
    REQUIRES
        bt
        nvs_flash
        driver
        mbedtls
        esp_http_client
        esp_https_ota
        app_update
        esp_driver_i2c
        freertos
        esp_timer
)

# CryptoAuthLib and KTA compile definitions / options
target_compile_options(${COMPONENT_LIB} PRIVATE "-include" "esp_log.h")
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    LIBRARY_BUILD_EN
    ATCA_HAL_I2C
    ATCA_ATECC608_SUPPORT
    ATCA_NO_HEAP
    MAX_PACKET_SIZE=1073U
    ATCAC_SHA256_EN=1
    ATCAC_SHA256_HMAC_EN=1
    ATCA_CRYPTO_SHA2_HMAC_EN=1
    KTA_ENABLED=1
    CONFIG_CRYPTOAUTHLIB_I2C_PORT=0
    CONFIG_CRYPTOAUTHLIB_I2C_SDA_GPIO=21
    CONFIG_CRYPTOAUTHLIB_I2C_SCL_GPIO=22
    CONFIG_CRYPTOAUTHLIB_I2C_FREQ_HZ=50000
    CONFIG_CRYPTOAUTHLIB_I2C_ADDR7=0x60
    CONFIG_CRYPTOAUTHLIB_I2C_ADDR8=0x70
    CONFIG_CRYPTOAUTHLIB_I2C_ENABLE_BUS_SCAN=1
)

# MCU bridge sources are compiled directly via idf_component_register above.
# No separate Makefile build step is needed.
