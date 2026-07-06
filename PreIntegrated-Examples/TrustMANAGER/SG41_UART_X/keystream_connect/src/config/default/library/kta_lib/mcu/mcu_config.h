/**
 * mcu_config.h — MCU Build Configuration
 *
 * Edit the three defines below to select your target combination,
 * then run: make  (inside kta_provisioning/mcu/)
 *
 * OS options       : freertos | bare_metal | linux
 * BACKEND options  : ble | uart | usb | zigbee
 * PLATFORM options : esp32 | nordic | stm32 | sg41 | microchip
 */

#define MCU_OS        /* freertos | bare_metal | linux    */
#define MCU_BACKEND   /* ble | uart | usb | zigbee        */
#define MCU_PLATFORM  /* esp32 | nordic | stm32 | sg41 | microchip */
