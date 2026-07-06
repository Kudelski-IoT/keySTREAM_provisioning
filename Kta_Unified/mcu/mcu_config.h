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
 * mcu_config.h — MCU Build Configuration
 *
 * Edit the three defines below to select your target combination,
 * then run: make  (inside kta_provisioning/mcu/)
 *
 * OS options       : freertos | bare_metal | linux
 * BACKEND options  : ble | uart | usb | zigbee
 * PLATFORM options : esp32 | nordic | stm32 | sg41 | microchip
 */

#ifndef MCU_CONFIG_H
#define MCU_CONFIG_H

/* --------------------------------------------------------------------------
 * Build selection — uncomment one value per macro
 * -------------------------------------------------------------------------- */

/** MCU operating system / scheduler */
#ifndef MCU_OS
#define MCU_OS        freertos       /* freertos | bare_metal | linux */
#endif

/** Closed-network transport backend */
#ifndef MCU_BACKEND
#define MCU_BACKEND   uart           /* ble | uart | usb | zigbee    */
#endif

/** Hardware platform */
#ifndef MCU_PLATFORM
#define MCU_PLATFORM  esp32          /* esp32 | nordic | sg41        */
#endif

#endif /* MCU_CONFIG_H */
