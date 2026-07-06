#include "atca_hal.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_rom_sys.h"

void atca_delay_us(uint32_t us)
{
  // Busy-wait delay; used for short protocol timing.
  esp_rom_delay_us(us);
}

void atca_delay_ms(uint32_t ms)
{
  if (ms == 0)
  {
    return;
  }

  const TickType_t ticks = pdMS_TO_TICKS(ms);
  if (ticks == 0)
  {
    // If ms < tick period, fall back to a short busy wait.
    esp_rom_delay_us(ms * 1000u);
    return;
  }

  vTaskDelay(ticks);
}

// CryptoAuthLib core (atca_iface.c, calib_*.c) expects these HAL delay symbols.
// Provide thin wrappers around the ESP-IDF ported delay helpers above.
void hal_delay_us(uint32_t us)
{
  atca_delay_us(us);
}

void hal_delay_ms(uint32_t ms)
{
  atca_delay_ms(ms);
}
