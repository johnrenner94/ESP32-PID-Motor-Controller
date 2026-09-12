#include "i2cmanager.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_err.h"
#include "input.h"

static const char *TAG = "i2c_mgr";

// writes a request to seesaw, packs regHigh containing seesaw module and regLow containing register
esp_err_t i2cWriteRegister(uint8_t regHigh, uint8_t regLow,
                                  const uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SEESAW_ADDR << 1) | I2C_MASTER_WRITE, true);    // shift 7 bit address and add write bit to lsb
    i2c_master_write_byte(cmd, regHigh, true);      // regHigh is module
    i2c_master_write_byte(cmd, regLow, true);       // regLow is register within module
    if (data && len) {
        i2c_master_write(cmd, data, len, true);
    }
    
    i2c_master_stop(cmd);

        esp_err_t err = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(500));
        i2c_cmd_link_delete(cmd);
    return err;
}

static esp_err_t i2cReadRegister(uint8_t regHigh, uint8_t regLow,
                                 uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SEESAW_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, regHigh, true);
    i2c_master_write_byte(cmd, regLow, true);
    i2c_master_stop(cmd);

    esp_err_t err = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(500));
    i2c_cmd_link_delete(cmd);
    if (err != ESP_OK) {
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(2));

    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SEESAW_ADDR << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    err = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(500));
    i2c_cmd_link_delete(cmd);
    return err;
}

uint32_t readU32BE(uint8_t regHigh, uint8_t regLow, esp_err_t *out_err)
{
    uint8_t buf[4] = {0};
    esp_err_t err = i2cReadRegister(regHigh, regLow, buf, sizeof(buf));
    if (out_err) {
        *out_err = err;
    }
    if (err != ESP_OK) {
        return 0;
    }
    return ((uint32_t)buf[0] << 24) |
           ((uint32_t)buf[1] << 16) |
           ((uint32_t)buf[2] << 8)  |
           ((uint32_t)buf[3]);
}

esp_err_t writeU32BE(uint8_t regHigh, uint8_t regLow, uint32_t value)
{
    uint8_t buf[4] = {
        (uint8_t)((value >> 24) & 0xFF),
        (uint8_t)((value >> 16) & 0xFF),
        (uint8_t)((value >> 8) & 0xFF),
        (uint8_t)(value & 0xFF),
    };
    return i2cWriteRegister(regHigh, regLow, buf, sizeof(buf));
}

void i2cInit(void)
{
    ESP_LOGI(TAG, "Initializing i2c...");

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    ESP_ERROR_CHECK(i2c_param_config(I2C_PORT, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0));

    i2cScan();

    ESP_LOGI(TAG, "I2C initialized: SDA=%d, SCL=%d, freq=%d",
             I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, I2C_MASTER_FREQ_HZ);
}

// scans for I2c devices by attempting to write to each possible address and checking for ACK response
void i2cScan(void)
{
    ESP_LOGI(TAG, "Scanning I2C bus...");
    int devicesFound = 0;


    for (uint8_t address = 1; address < 127; address++) {

        // build a command link to write to the address
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);

        // trigger command link, use err to check if device at address responded with ACK
        esp_err_t err = i2c_master_cmd_begin(
            I2C_PORT,
            cmd,
            pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS)
        );

        i2c_cmd_link_delete(cmd);   // delete link after use

        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Found device at 0x%02X", address);
            devicesFound++;
        }
    }

    if (devicesFound == 0) {
        ESP_LOGW(TAG, "No I2C devices found");
    } else {
        ESP_LOGI(TAG, "Scan complete, found %d device(s)", devicesFound);
    }
}