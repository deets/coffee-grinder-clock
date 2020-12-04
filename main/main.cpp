#include "display.hh"
#include "i2c.hh"
#include "mpu6050.hh"
#include "fft.hh"
#include "ringbuffer.hh"

#ifdef CONFIG_COFFEE_CLOCK_STREAM_DATA
#include "wifi.hh"
#include "streamer.hh"
#endif

#include "fft-display.hh"

#include <esp_heap_caps.h>
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

#define USE_WIFI 0

namespace {

const int MAINLOOP_WAIT = 16; // 60fps
const int WIFI_WAIT = 500;
const auto SDA = gpio_num_t(19);
const auto SCL = gpio_num_t(20);

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
      // display.font_render(
      //   SMALL,
      //   _name,
      //   _x - 1,
      //   SMALL.size + _y - 1
      // );
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
  #ifdef CONFIG_COFFEE_CLOCK_STREAM_DATA
  setup_wifi();
  auto streamer = new DataStreamer("");
  #endif

  using FFT = FFT<256, 16>;
  auto rb = new RingBuffer<float, 2000>();

  auto fft = new FFT();
  Display display;

  auto fft_display = new FFTDisplay<54, 60, -200>;

  I2CHost i2c(I2C_NUM_0, SDA, SCL);

  MPU6050 mpu(
    MPU6050_ADDRESS_AD0_LOW,
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

  auto free = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  auto largest_free = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  ESP_LOGI("main", "8 bit free: %i, 8 bit largest_free: %i", free, largest_free);
  free = heap_caps_get_free_size(MALLOC_CAP_32BIT);
  largest_free = heap_caps_get_largest_free_block(MALLOC_CAP_32BIT);
  ESP_LOGI("main", "32 bit free: %i, 32 bit largest_free: %i", free, largest_free);

  auto timestamp = esp_timer_get_time();
  bool running = true;
  while(running)
  {
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
        #ifdef CONFIG_COFFEE_CLOCK_STREAM_DATA
        streamer->feed(rad);
        #endif
        if(fft->feed(rad, rad))
        {
          fft->compute();
          fft->postprocess();
          #ifdef CONFIG_COFFEE_CLOCK_STREAM_DATA
          streamer->deliver_fft(fft);
          #endif
          fft_display->update(
            // We filter out the lowest frequency bins
            // because they contain DC and the drift.
            // The 10 is just experience, I need to dig
            // down deeper to understand that.
            fft->fft().begin() + 10,
            // The concrete use-case does not warrant
            // frequencies higher
            fft->fft().begin() + fft->n / 4
            );
        }
      }
      );
    const auto now = esp_timer_get_time();
    const float fps = 1.0 / (float(now - timestamp) / 1000000.0);
    timestamp = now;
    ESP_LOGI("main", "fps: %f", fps);
    //                   sprintf(time_buffer, "%f", elapsed);
    // display.font_render(
    //   SMALL,
    //   time_buffer,
    //   0,
    //   SMALL.size + 2
    //   );
    display.clear();
    if(display.ready())
    {
      fft_display->render(display, 0, 64 - fft_display->height);
      display.update();
    }
    else
    {
      ESP_LOGI("main", "display not ready");
    }
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
