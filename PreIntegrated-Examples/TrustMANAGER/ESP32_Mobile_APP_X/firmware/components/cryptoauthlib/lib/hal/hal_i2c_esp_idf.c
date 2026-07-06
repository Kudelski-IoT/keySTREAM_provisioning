#include "atca_hal.h"

#include <string.h>

#include "esp_err.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c_master.h"

#ifndef CONFIG_CRYPTOAUTHLIB_I2C_PORT
#define CONFIG_CRYPTOAUTHLIB_I2C_PORT 0
#endif
#ifndef CONFIG_CRYPTOAUTHLIB_I2C_SDA_GPIO
#define CONFIG_CRYPTOAUTHLIB_I2C_SDA_GPIO 21
#endif
#ifndef CONFIG_CRYPTOAUTHLIB_I2C_SCL_GPIO
#define CONFIG_CRYPTOAUTHLIB_I2C_SCL_GPIO 22
#endif
#ifndef CONFIG_CRYPTOAUTHLIB_I2C_FREQ_HZ
#define CONFIG_CRYPTOAUTHLIB_I2C_FREQ_HZ 100000
#endif

#ifndef CONFIG_CRYPTOAUTHLIB_I2C_ADDR7
#define CONFIG_CRYPTOAUTHLIB_I2C_ADDR7 0x60
#endif

#ifndef CONFIG_CRYPTOAUTHLIB_I2C_ADDR8
#define CONFIG_CRYPTOAUTHLIB_I2C_ADDR8 0x00
#endif

#ifndef CONFIG_CRYPTOAUTHLIB_I2C_ENABLE_BUS_SCAN
#define CONFIG_CRYPTOAUTHLIB_I2C_ENABLE_BUS_SCAN 0
#endif

typedef struct
{
  i2c_master_bus_handle_t bus;
  i2c_master_dev_handle_t dev_normal;
  i2c_master_dev_handle_t dev_wake;
  uint8_t address_normal_7bit;
} atca_esp_idf_i2c_ctx_t;

static const char* kTag = "cryptoauthlib_i2c";

static esp_err_t i2c_tx_with_retry(i2c_master_bus_handle_t bus,
                                  i2c_master_dev_handle_t dev,
                                  const uint8_t* data,
                                  size_t len,
                                  int timeout_ms)
{
  esp_err_t err = ESP_FAIL;
#define CRYPTOAUTHLIB_I2C_RETRY_ATTEMPTS 3
  static const uint32_t kDelayMs[CRYPTOAUTHLIB_I2C_RETRY_ATTEMPTS] = {2u, 5u, 12u};

  for (int attempt = 0; attempt < (int)CRYPTOAUTHLIB_I2C_RETRY_ATTEMPTS; ++attempt)
  {
    err = i2c_master_transmit(dev, data, len, timeout_ms);
    if (err == ESP_OK)
    {
      return ESP_OK;
    }

    if (bus != NULL)
    {
      (void)i2c_master_bus_reset(bus);
    }
    vTaskDelay(pdMS_TO_TICKS(kDelayMs[attempt]));
  }
  return err;
#undef CRYPTOAUTHLIB_I2C_RETRY_ATTEMPTS
}

static esp_err_t i2c_rx_with_retry(i2c_master_bus_handle_t bus,
                                  i2c_master_dev_handle_t dev,
                                  uint8_t* data,
                                  size_t len,
                                  int timeout_ms)
{
  esp_err_t err = ESP_FAIL;
#define CRYPTOAUTHLIB_I2C_RETRY_ATTEMPTS 3
  static const uint32_t kDelayMs[CRYPTOAUTHLIB_I2C_RETRY_ATTEMPTS] = {2u, 5u, 12u};

  for (int attempt = 0; attempt < (int)CRYPTOAUTHLIB_I2C_RETRY_ATTEMPTS; ++attempt)
  {
    err = i2c_master_receive(dev, data, len, timeout_ms);
    if (err == ESP_OK)
    {
      return ESP_OK;
    }

    if (bus != NULL)
    {
      (void)i2c_master_bus_reset(bus);
    }
    vTaskDelay(pdMS_TO_TICKS(kDelayMs[attempt]));
  }
  return err;
#undef CRYPTOAUTHLIB_I2C_RETRY_ATTEMPTS
}

ATCA_STATUS hal_i2c_discover_buses(int i2c_buses[], int max_buses)
{
  if (i2c_buses == NULL || max_buses <= 0)
  {
    return ATCA_BAD_PARAM;
  }
  i2c_buses[0] = CONFIG_CRYPTOAUTHLIB_I2C_PORT;
  return ATCA_SUCCESS;
}

ATCA_STATUS hal_i2c_discover_devices(int bus_num, ATCAIfaceCfg cfg[], int* found)
{
  (void)bus_num;
  (void)cfg;
  if (found != NULL)
  {
    *found = 0;
  }
  return ATCA_UNIMPLEMENTED;
}

static ATCA_STATUS i2c_ctx_init(atca_esp_idf_i2c_ctx_t* ctx, const ATCAIfaceCfg* cfg)
{
  if (ctx == NULL || cfg == NULL)
  {
    return ATCA_BAD_PARAM;
  }

  const uint8_t cfg_address8 = (uint8_t)ATCA_IFACECFG_I2C_ADDRESS(cfg);
  uint8_t effective_addr7 = (uint8_t)CONFIG_CRYPTOAUTHLIB_I2C_ADDR7;
  if ((uint8_t)CONFIG_CRYPTOAUTHLIB_I2C_ADDR8 != 0u)
  {
    effective_addr7 = (uint8_t)(((uint8_t)CONFIG_CRYPTOAUTHLIB_I2C_ADDR8) >> 1);
  }
  const uint8_t effective_addr8 = (uint8_t)(effective_addr7 << 1);
  ctx->address_normal_7bit = effective_addr7;
  
  ESP_LOGI(kTag, "CryptoAuth I2C cfg: addr8=0x%02X (cfg addr8=0x%02X) port=%d sda=%d scl=%d freq=%d",
           (unsigned)effective_addr8,
           (unsigned)cfg_address8,
           (int)CONFIG_CRYPTOAUTHLIB_I2C_PORT,
           (int)CONFIG_CRYPTOAUTHLIB_I2C_SDA_GPIO,
           (int)CONFIG_CRYPTOAUTHLIB_I2C_SCL_GPIO,
           (int)CONFIG_CRYPTOAUTHLIB_I2C_FREQ_HZ);

  i2c_master_bus_config_t bus_cfg = {
      .i2c_port = CONFIG_CRYPTOAUTHLIB_I2C_PORT,
      .sda_io_num = CONFIG_CRYPTOAUTHLIB_I2C_SDA_GPIO,
      .scl_io_num = CONFIG_CRYPTOAUTHLIB_I2C_SCL_GPIO,
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .glitch_ignore_cnt = 7,
      .intr_priority = 0,
      // Use synchronous transactions. The async queue mode is experimental and
      // can produce warnings/crashes depending on usage.
      .trans_queue_depth = 0,
      .flags = {
          .enable_internal_pullup = 1,
      },
  };

  esp_err_t err = i2c_new_master_bus(&bus_cfg, &ctx->bus);
  if (err != ESP_OK)
  {
    ESP_LOGE(kTag, "i2c_new_master_bus failed: %d (%s)", (int)err, esp_err_to_name(err));

    // Some ESP-IDF versions treat trans_queue_depth=0 as invalid.
    // Retry with a small queue depth to keep the HAL compatible across IDF revisions.
    if (err == ESP_ERR_INVALID_ARG && bus_cfg.trans_queue_depth == 0)
    {
      bus_cfg.trans_queue_depth = 8;
      ESP_LOGW(kTag, "Retrying i2c_new_master_bus with trans_queue_depth=%d", (int)bus_cfg.trans_queue_depth);
      err = i2c_new_master_bus(&bus_cfg, &ctx->bus);
      if (err != ESP_OK)
      {
        ESP_LOGE(kTag, "i2c_new_master_bus retry failed: %d (%s)", (int)err, esp_err_to_name(err));
        return ATCA_COMM_FAIL;
      }
    }
    else
    {
      return ATCA_COMM_FAIL;
    }
  }

    i2c_device_config_t dev_cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = ctx->address_normal_7bit,
      .scl_speed_hz = CONFIG_CRYPTOAUTHLIB_I2C_FREQ_HZ,
      .scl_wait_us = 0,
      .flags = {
        .disable_ack_check = 0,
      },
    };

  err = i2c_master_bus_add_device(ctx->bus, &dev_cfg, &ctx->dev_normal);
  if (err != ESP_OK)
  {
    ESP_LOGE(kTag, "i2c_master_bus_add_device failed: %d", (int)err);
    (void)i2c_del_master_bus(ctx->bus);
    ctx->bus = NULL;
    return ATCA_COMM_FAIL;
  }

  // Device handle for I2C general call (address 0) used by CryptoAuthLib wake sequence.
  i2c_device_config_t wake_dev_cfg = dev_cfg;
  wake_dev_cfg.device_address = 0;
  // For the general-call wake token, an ACK is not required. Disabling ACK check
  // avoids noisy "hardware NACK" errors from the I2C driver.
  wake_dev_cfg.flags.disable_ack_check = 1;
  err = i2c_master_bus_add_device(ctx->bus, &wake_dev_cfg, &ctx->dev_wake);
  if (err != ESP_OK)
  {
    ESP_LOGE(kTag, "i2c_master_bus_add_device(wake) failed: %d", (int)err);
    (void)i2c_master_bus_rm_device(ctx->dev_normal);
    ctx->dev_normal = NULL;
    (void)i2c_del_master_bus(ctx->bus);
    ctx->bus = NULL;
    return ATCA_COMM_FAIL;
  }

  return ATCA_SUCCESS;
}

static bool looks_like_atecc_wake_rsp(const uint8_t* rsp, size_t len)
{
  if (rsp == NULL || len < 4u)
  {
    return false;
  }
  // Typical ATECC wake response: 0x04 0x11 0x33 0x43
  return (rsp[0] == 0x04u) && (rsp[1] == 0x11u) && (rsp[2] == 0x33u) && (rsp[3] == 0x43u);
}

static void i2c_try_wake_best_effort(atca_esp_idf_i2c_ctx_t* ctx)
{
  if (ctx == NULL || ctx->dev_wake == NULL || ctx->dev_normal == NULL)
  {
    return;
  }

  static const uint8_t wake_token = 0x00u;
  (void)i2c_master_transmit(ctx->dev_wake, &wake_token, 1u, 1000);
  vTaskDelay(pdMS_TO_TICKS(10));

  uint8_t wake_rsp[4] = {0};
  for (int attempt = 0; attempt < 3; ++attempt)
  {
    if (i2c_master_receive(ctx->dev_normal, wake_rsp, sizeof(wake_rsp), 1000) == ESP_OK)
    {
      break;
    }
    vTaskDelay(pdMS_TO_TICKS(2));
  }
}

static void i2c_bus_scan(i2c_master_bus_handle_t bus, i2c_master_dev_handle_t dev_wake)
{
  ESP_LOGI(kTag, "--- I2C bus scan (probing addresses 0x03..0x77) ---");

  // Expected behavior during scan is lots of NACKs (most addresses have no device,
  // and many devices won't respond to a raw read). Suppress the ESP-IDF I2C driver's
  // ERROR logs temporarily so monitor output remains readable.
  const esp_log_level_t prev_i2c_level = esp_log_level_get("i2c.master");
  const esp_log_level_t prev_gpio_level = esp_log_level_get("gpio");
  esp_log_level_set("i2c.master", ESP_LOG_NONE);
  esp_log_level_set("gpio", ESP_LOG_WARN);

  // Many ATECC608 setups do not ACK normal traffic while asleep.
  // Wake all CryptoAuth devices using the I2C general-call token first,
  // then probe by reading the 4-byte wake response on each address.
  if (dev_wake != NULL)
  {
    static const uint8_t wake_token = 0x00u;
    (void)i2c_master_transmit(dev_wake, &wake_token, 1u, 50);
    vTaskDelay(pdMS_TO_TICKS(3));
  }

  int found = 0;
  int found_atecc = 0;
  for (uint8_t addr = 0x03u; addr <= 0x77u; ++addr)
  {
    i2c_device_config_t probe_cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = addr,
      .scl_speed_hz = CONFIG_CRYPTOAUTHLIB_I2C_FREQ_HZ,
      .scl_wait_us = 0,
      .flags = { .disable_ack_check = 0 },
    };
    i2c_master_dev_handle_t probe_dev = NULL;
    if (i2c_master_bus_add_device(bus, &probe_cfg, &probe_dev) != ESP_OK)
    {
      continue;
    }

    uint8_t rsp[4] = {0};
    esp_err_t err = i2c_master_receive(probe_dev, rsp, sizeof(rsp), 50);
    (void)i2c_master_bus_rm_device(probe_dev);
    if (err == ESP_OK)
    {
      ESP_LOGI(kTag, "  I2C device found at addr8 0x%02X", (unsigned)(addr << 1));
      ++found;

      if (looks_like_atecc_wake_rsp(rsp, sizeof(rsp)))
      {
        ESP_LOGW(kTag, "  ATECC wake response matched at addr7 0x%02X (addr8 0x%02X)", (unsigned)addr, (unsigned)(addr << 1));
        ++found_atecc;

        // We found what we care about; stop scanning to avoid needless bus traffic.
        break;
      }
    }
  }
  if (found == 0)
  {
    ESP_LOGW(kTag, "  NO devices found on I2C bus! Check wiring/pullups/power.");
  }
  else
  {
    ESP_LOGI(kTag, "  %d device(s) found on I2C bus.", found);
  }

  if (found_atecc == 0)
  {
    ESP_LOGW(kTag, "  No ATECC wake responses detected. If your ATECC is behind an I2C mux or uses a non-default address, configure CONFIG_CRYPTOAUTHLIB_I2C_ADDR7/ADDR8 accordingly.");
  }

  // Restore I2C driver logging for normal operation.
  esp_log_level_set("i2c.master", prev_i2c_level);
  esp_log_level_set("gpio", prev_gpio_level);
  ESP_LOGI(kTag, "--- I2C bus scan complete ---");
}

ATCA_STATUS hal_i2c_init(ATCAIface iface, ATCAIfaceCfg* cfg)
{
  (void)iface;
  if (cfg == NULL)
  {
    return ATCA_BAD_PARAM;
  }

  // Allocate and attach a bus/device context to this interface configuration.
  atca_esp_idf_i2c_ctx_t* ctx = (atca_esp_idf_i2c_ctx_t*)calloc(1u, sizeof(*ctx));
  if (ctx == NULL)
  {
    return ATCA_ALLOC_FAILURE;
  }

  ATCA_STATUS st = i2c_ctx_init(ctx, cfg);
  if (st != ATCA_SUCCESS)
  {
    free(ctx);
    return st;
  }

  // Optional one-time I2C bus scan (can be very noisy because every NACK is logged
  // by the ESP-IDF I2C driver). Enable only when debugging wiring/address issues.
  if (CONFIG_CRYPTOAUTHLIB_I2C_ENABLE_BUS_SCAN)
  {
    static bool s_scan_done = false;
    if (!s_scan_done)
    {
      s_scan_done = true;
      i2c_bus_scan(ctx->bus, ctx->dev_wake);
    }
  }

  cfg->cfg_data = ctx;
  return ATCA_SUCCESS;
}

ATCA_STATUS hal_i2c_post_init(ATCAIface iface)
{
  (void)iface;
  return ATCA_SUCCESS;
}

ATCA_STATUS hal_i2c_send(ATCAIface iface, uint8_t word_address, uint8_t* txdata, int txlength)
{
  ATCAIfaceCfg* cfg = atgetifacecfg(iface);
  if (cfg == NULL)
  {
    return ATCA_BAD_PARAM;
  }

  atca_esp_idf_i2c_ctx_t* ctx = (atca_esp_idf_i2c_ctx_t*)cfg->cfg_data;
  if (ctx == NULL || ctx->dev_normal == NULL || ctx->dev_wake == NULL)
  {
    return ATCA_BAD_PARAM;
  }

  if (txlength < 0)
  {
    return ATCA_BAD_PARAM;
  }

  const size_t total = (size_t)txlength + 1u;
  // Avoid allocating MAX_PACKET_SIZE (up to ~1KB) on the stack, which can
  // overflow small task stacks during deep call chains.
  uint8_t* buf = (uint8_t*)malloc(total);
  if (buf == NULL)
  {
    return ATCA_ALLOC_FAILURE;
  }

  buf[0] = word_address;
  if (txdata != NULL && txlength > 0)
  {
    memcpy(&buf[1], txdata, (size_t)txlength);
  }

  // CryptoAuthLib's wake implementation temporarily sets cfg's I2C address to 0
  // to issue a general-call wake token. Support that by selecting the wake handle.
  const uint8_t current_addr8 = (uint8_t)ATCA_IFACECFG_I2C_ADDRESS(cfg);
  i2c_master_dev_handle_t dev = (current_addr8 == 0u) ? ctx->dev_wake : ctx->dev_normal;

  esp_err_t err = (current_addr8 == 0u)
                      ? i2c_master_transmit(dev, buf, total, 1000)
                      : i2c_tx_with_retry(ctx->bus, dev, buf, total, 1000);

  if (err != ESP_OK && current_addr8 != 0u)
  {
    // Best-effort recovery: if the device was asleep or the bus glitched, wake
    // the device and retry once.
    ESP_LOGW(kTag, "i2c transmit failed (err=%d); attempting wake+retry", (int)err);
    i2c_try_wake_best_effort(ctx);
    err = i2c_tx_with_retry(ctx->bus, dev, buf, total, 1000);
  }

  free(buf);
  if (err != ESP_OK)
  {
    // CryptoAuthLib uses an I2C general-call (address 0) wake token.
    // ATECC wake does not require an ACK, so a NACK here is expected on many buses.
    // Treat wake-token transmit errors as non-fatal so the subsequent wake response
    // read can complete.
    if (current_addr8 == 0u)
    {
      ESP_LOGD(kTag, "wake token transmit returned err=%d (ignored)", (int)err);
      return ATCA_SUCCESS;
    }

    ESP_LOGD(kTag, "i2c_master_transmit failed: %d", (int)err);
    return ATCA_COMM_FAIL;
  }

  return ATCA_SUCCESS;
}

ATCA_STATUS hal_i2c_receive(ATCAIface iface, uint8_t word_address, uint8_t* rxdata, uint16_t* rxlength)
{
  (void)word_address;

  ATCAIfaceCfg* cfg = atgetifacecfg(iface);
  if (cfg == NULL || rxdata == NULL || rxlength == NULL)
  {
    return ATCA_BAD_PARAM;
  }

  atca_esp_idf_i2c_ctx_t* ctx = (atca_esp_idf_i2c_ctx_t*)cfg->cfg_data;
  if (ctx == NULL || ctx->dev_normal == NULL)
  {
    return ATCA_BAD_PARAM;
  }

  esp_err_t err = i2c_rx_with_retry(ctx->bus, ctx->dev_normal, rxdata, *rxlength, 1000);
  if (err != ESP_OK)
  {
    ESP_LOGW(kTag, "i2c receive failed (err=%d); attempting wake+retry", (int)err);
    i2c_try_wake_best_effort(ctx);
    err = i2c_rx_with_retry(ctx->bus, ctx->dev_normal, rxdata, *rxlength, 1000);
  }
  if (err != ESP_OK)
  {
    ESP_LOGD(kTag, "i2c_master_receive failed: %d", (int)err);
    return ATCA_COMM_FAIL;
  }

  return ATCA_SUCCESS;
}

ATCA_STATUS hal_i2c_control(ATCAIface iface, uint8_t option, void* param, size_t paramlen)
{
  (void)param;
  (void)paramlen;

  ATCAIfaceCfg* cfg = atgetifacecfg(iface);
  if (cfg == NULL)
  {
    return ATCA_BAD_PARAM;
  }

  atca_esp_idf_i2c_ctx_t* ctx = (atca_esp_idf_i2c_ctx_t*)cfg->cfg_data;
  if (ctx == NULL || ctx->dev_normal == NULL || ctx->dev_wake == NULL)
  {
    return ATCA_BAD_PARAM;
  }

  switch (option)
  {
    case ATCA_HAL_CHANGE_BAUD:
      // ESP-IDF i2c_master driver doesn't support changing device speed on the fly
      // for an existing handle. Use menuconfig to set CONFIG_CRYPTOAUTHLIB_I2C_FREQ_HZ.
      return ATCA_SUCCESS;

    case ATCA_HAL_CONTROL_WAKE:
    {
      // Wake sequence for ATECC devices over I2C:
      // - General-call write of 0x00 (no ACK required)
      // - tWHI delay (~2-3ms)
      // - Read 4-byte wake response from device address
      static const uint8_t wake_token = 0x00u;
      (void)i2c_master_transmit(ctx->dev_wake, &wake_token, 1u, 1000);

      // Give the device extra time to wake on long wires / weak pullups.
      vTaskDelay(pdMS_TO_TICKS(10));

      uint8_t wake_rsp[4] = {0};
      esp_err_t err = ESP_FAIL;
      for (int attempt = 0; attempt < 3; ++attempt)
      {
        err = i2c_master_receive(ctx->dev_normal, wake_rsp, sizeof(wake_rsp), 1000);
        if (err == ESP_OK)
        {
          break;
        }
        vTaskDelay(pdMS_TO_TICKS(2));
      }

      if (err != ESP_OK)
      {
        ESP_LOGD(kTag, "wake response read failed: %d", (int)err);
        return ATCA_WAKE_FAILED;
      }

      // Typical wake response is 0x04 0x11 0x33 0x43, but some configurations/variants
      // can differ. Log and continue as long as we got a response.
      ESP_LOGD(kTag, "wake response: %02X %02X %02X %02X",
               wake_rsp[0], wake_rsp[1], wake_rsp[2], wake_rsp[3]);
      return ATCA_SUCCESS;
    }

    case ATCA_HAL_CONTROL_IDLE:
    {
      // Idle token is 0x02 written to device.
      static const uint8_t idle_token = 0x02u;
      const esp_err_t err = i2c_master_transmit(ctx->dev_normal, &idle_token, 1u, 1000);
      return (err == ESP_OK) ? ATCA_SUCCESS : ATCA_COMM_FAIL;
    }

    case ATCA_HAL_CONTROL_SLEEP:
    {
      // Sleep token is 0x01 written to device.
      static const uint8_t sleep_token = 0x01u;
      const esp_err_t err = i2c_master_transmit(ctx->dev_normal, &sleep_token, 1u, 1000);
      return (err == ESP_OK) ? ATCA_SUCCESS : ATCA_COMM_FAIL;
    }

    default:
      return ATCA_SUCCESS;
  }
}

ATCA_STATUS hal_i2c_release(void* hal_data)
{
  atca_esp_idf_i2c_ctx_t* ctx = (atca_esp_idf_i2c_ctx_t*)hal_data;
  if (ctx == NULL)
  {
    return ATCA_SUCCESS;
  }

  if (ctx->bus != NULL && ctx->dev_normal != NULL)
  {
    (void)i2c_master_bus_rm_device(ctx->dev_normal);
    ctx->dev_normal = NULL;
  }

  if (ctx->bus != NULL && ctx->dev_wake != NULL)
  {
    (void)i2c_master_bus_rm_device(ctx->dev_wake);
    ctx->dev_wake = NULL;
  }

  if (ctx->bus != NULL)
  {
    (void)i2c_del_master_bus(ctx->bus);
    ctx->bus = NULL;
  }

  free(ctx);
  return ATCA_SUCCESS;
}
