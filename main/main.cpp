#include "display.hh"
#include "i2c.hh"
#include "mpu6050.hh"
#include "fft.hh"
#include "ringbuffer.hh"
#include "wifi.hh"
#include "streamer.hh"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/spi_master.h>
#include <nvs_flash.h>
#include <math.h>
#include <array>
#include <vector>

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

void main_task(void*)
{
  setup_wifi();
  using FFT = FFT<1024>;
  auto rb = new RingBuffer<float, 2000>();

  auto fft = new FFT();

  Display display;
  I2CHost i2c(I2C_NUM_0, SDA, SCL);
  auto streamer = new DataStreamer("");

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

  const auto start = esp_timer_get_time();
  bool running = true;
  while(running)
  {
    vTaskDelay(pdMS_TO_TICKS(MAINLOOP_WAIT));
    mpu.consume_fifo(
      [rb](const MPU6050::gyro_data_t& entry)
      {
        rb->append(entry.gyro[2]);
      }
      );
    display_reader.consume(
      [&](const float& v)
      {
        z_axis.update(v, elapsed_seconds);
        const auto rad = z_axis.rad();
        streamer->feed(rad);
        if(fft->feed(cos(rad), 0.0))
        {
        }
      }
      );
    display.clear();
    display.set_color(1);
    z_axis.display(display);

    const float elapsed = float(esp_timer_get_time() - start) / 1000000.0;
    char time_buffer[50];
    sprintf(time_buffer, "%f", elapsed);
    display.font_render(
      SMALL,
      time_buffer,
      0,
      SMALL.size + 2
      );
    display.update();
    //ESP_LOGI("main", "fifo overflown: %i", mpu.fifo_overflown());
  }
}

} // end ns anon

void app_main()
{
  //Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  // it seems if I don't bind this to core 0, the i2c
  // subsystem fails randomly.
  xTaskCreatePinnedToCore(main_task, "main", 8192, NULL, uxTaskPriorityGet(NULL), NULL, 0);
}
