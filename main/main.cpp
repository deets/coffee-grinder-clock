#include "display.hh"
#include "i2c.hh"
#include "mpu6050.hh"
#include "drv2605.hh"
#include "ringbuffer.hh"

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
    const auto r = static_cast<float>(_radius) + _offset;
    const auto cx = int(cos(rad()) * r) + _x;
    const auto cy = int(sin(rad()) * r) + _y;
    display.circle(_x, _y, _radius);
    display.circle(cx, cy, 2, true);
  }

  float rad() const
  {
    return _gyro_accu / 180.0 * M_PI;
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
  auto rb = new RingBuffer<float, 2000>();

  Display display;
  I2CHost i2c(I2C_NUM_0, SDA, SCL);
  DRV2605 hf(i2c);

  MPU6050 mpu(
    MPU6050_ADDRESS_AD0_HI,
    i2c,
    MPU6050::GYRO_2000_FS,
    MPU6050::ACC_2_FS
    );

  mpu.setup_fifo(MPU6050::fifo_e(MPU6050::FIFO_EN_ZG));
  mpu.calibrate_fifo_based();
  const auto mpu_samplerate = mpu.samplerate();
  const auto elapsed_seconds = 1.0 / mpu_samplerate;
  GyroAxisDisplay z_axis("Z", 64, 32, 12, .7);

  auto display_reader = rb->reader();

  hf.use_erm();
  hf.select_library(1);

  esp_timer_create_args_t ta = {
    .callback=[](void* arg)
              {
                static int counter=0;
                ++counter;
                if(counter % 5 < 4)
                {
                  auto& hf = *static_cast<DRV2605*>(arg);
                  hf.set_waveform(0, 16);  // play effect
                  hf.set_waveform(1, 0);       // end waveform
                  hf.go();
                }
              },
    .arg=&hf,
    .dispatch_method=ESP_TIMER_TASK,
    .name="lra"
  };
  esp_timer_handle_t th;
  esp_timer_create(&ta, &th);
  esp_timer_start_periodic(th, 1000000);
  for( ;; )
  {

    vTaskDelay(pdMS_TO_TICKS(MAINLOOP_WAIT));
    mpu.consume_fifo(
      [rb](const MPU6050::gyro_data_t& entry)
      {
        rb->append(entry.gyro[2]);
      }
      );
    display_reader.consume(
      [&z_axis, elapsed_seconds](const float& v)
      {
        z_axis.update(v, elapsed_seconds);
      }
      );
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
