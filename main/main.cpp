#include "display.hh"
#include "buzzer.hh"
#include "i2c.hh"
#include "mpu6050.hh"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/spi_master.h>
#include <nvs_flash.h>
#include <array>

extern "C" void app_main();

#define MPU6050_ADDRESS_AD0_LOW 0x68
#define MPU6050_ADDRESS_AD0_HI 0x69
#define MPU6050_RA_WHO_AM_I 0x75

namespace {

const int MAINLOOP_WAIT = 16; // 60fps
const int WIFI_WAIT = 500;
const auto SDA = gpio_num_t(18);
const auto SCL = gpio_num_t(21);


} // end ns anon

void app_main()
{
  nvs_flash_init();

  Display display;
  Buzzer buzzer(static_cast<gpio_num_t>(14));
  I2CHost i2c(I2C_NUM_0, SDA, SCL);
  MPU6050 mpu(
    MPU6050_ADDRESS_AD0_HI,
    i2c,
    MPU6050::GYRO_250_FS,
    MPU6050::ACC_2_FS
    );

  for( ;; )
  {

    vTaskDelay(pdMS_TO_TICKS(MAINLOOP_WAIT));

    const auto gyro_data = mpu.read_raw();
    ESP_LOGI("mpu", "gyros: %f %f %f", gyro_data.gyro[0], gyro_data.gyro[1], gyro_data.gyro[2]);
    display.clear();
    display.font_render(
      SMALL,
      "hallo",
      0,
      SMALL.size + 2
      );

    display.update();
  }
}
