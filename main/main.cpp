#include "display.hh"
#include "i2c.hh"
#include "mpu6050.hh"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/spi_master.h>
#include <nvs_flash.h>
#include <math.h>
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

class GyroAxisDisplay
{
public:
  GyroAxisDisplay(const char* name, int x, int y, int radius, float offset)
    : _name(name)
    , _x(x)
    , _y(y)
    , _radius(radius)
    , _offset(offset)
  {
  }

  void update(float gyro, float elapsed_seconds)
  {
    _gyro_accu += gyro * elapsed_seconds;
  }

  void display(Display& display)
  {
    if(_name)
    {
      display.font_render(
        SMALL,
        _name,
        _x - 1,
        SMALL.size + _y - 1
      );
    }
    const auto rad = _gyro_accu / 180.0 * M_PI;
    const auto r = static_cast<float>(_radius) + _offset;
    const auto cx = int(cos(rad) * r) + _x;
    const auto cy = int(sin(rad) * r) + _y;
    display.circle(_x, _y, _radius);
    display.circle(cx, cy, 2, true);
  }

private:
  const char* _name;
  int _x, _y, _radius;
  float _offset;
  float _gyro_accu = 0.0;
};

} // end ns anon

void app_main()
{
  nvs_flash_init();

  Display display;
  I2CHost i2c(I2C_NUM_0, SDA, SCL);
  MPU6050 mpu(
    MPU6050_ADDRESS_AD0_HI,
    i2c,
    MPU6050::GYRO_250_FS,
    MPU6050::ACC_2_FS
    );

  mpu.calibrate(2000);
  auto now = esp_timer_get_time();
  GyroAxisDisplay z_axis("Z", 64, 32, 12, .7);

  for( ;; )
  {

    vTaskDelay(pdMS_TO_TICKS(MAINLOOP_WAIT));
    const auto h = esp_timer_get_time();
    const auto elapsed = h - now;
    now = h;
    const auto gyro_data = mpu.read();
    const auto elapsed_seconds = static_cast<float>(elapsed) / 1000000.0;
    const auto z_gyro = gyro_data.gyro[2];
    z_axis.update(z_gyro, elapsed_seconds);

    display.clear();
    display.set_color(1);
    z_axis.display(display);
    display.font_render(
      SMALL,
      "hallo",
      0,
      SMALL.size + 2
      );


    display.update();
  }
}
