#include "display.hh"
#include "i2c.hh"
#include "mpu6050.hh"
#include "madgwick.hh"
#include "fft.hh"
#include "ringbuffer.hh"
#include "io-buttons.hh"

#ifdef CONFIG_COFFEE_CLOCK_STREAM_DATA
#include "wifi.hh"
#include "streamer.hh"
#endif

#include "fft-display.hh"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <driver/spi_master.h>
#include <nvs_flash.h>
#include <math.h>
#include <array>
#include <vector>
#include <sstream>

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

  void reset()
  {
    _gyro_accu = 0.0;
  }

  void add(float v)
  {
    _gyro_accu += v;
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
  #else
  Display display;
  auto test_sprite = BufferedSprite(display.width() - 4, 28, nullptr, 0xff);
  #endif

  using FFT = FFT<256, 16>;
  auto rb = new RingBuffer<float, 2000>();

  auto fft = new FFT();



  auto fft_display = new FFTDisplay<135>;

  I2CHost i2c(I2C_NUM_0, SDA, SCL);

  MPU6050 mpu(
    MPU6050_ADDRESS_AD0_LOW,
    i2c,
    MPU6050::GYRO_2000_FS,
    MPU6050::ACC_2_FS
    );

  mpu.setup_fifo(MPU6050::fifo_e(
                   MPU6050::FIFO_EN_XG
                   #ifdef CONFIG_COFFEE_CLOCK_FILTER_IMU
                   | MPU6050::FIFO_EN_ACCEL
                   | MPU6050::FIFO_EN_YG
                   | MPU6050_EN_ZG
                   #endif
                   )
    );
  mpu.calibrate_fifo_based();

  const auto mpu_samplerate = mpu.samplerate();
  const auto elapsed_seconds = 1.0 / mpu_samplerate;
  #ifdef CONFIG_COFFEE_CLOCK_FILTER_IMU
  MadgwickAHRS mpu_filter(mpu_samplerate);
  #endif
  GyroAxisDisplay z_axis("Z", 64, 32, 12, .7);

  EventGroupHandle_t button_events = xEventGroupCreate();
  assert(button_events);
  iobuttons_setup(button_events);

  auto display_reader = rb->reader();

  auto timestamp = esp_timer_get_time();
  size_t max_datagram_count = 0;
  bool running = true;
  while(running)
  {
    const auto datagram_count = mpu.consume_fifo(
      #ifdef CONFIG_COFFEE_CLOCK_FILTER_IMU
      [rb, &mpu_filter](const MPU6050::gyro_data_t& entry)
      {
        mpu_filter.update_imu(
          entry.gyro[0],
          entry.gyro[1],
          entry.gyro[2],
          entry.acc[0],
          entry.acc[1],
          entry.acc[2]);
        float x, y, z;
        mpu_filter.compute_angles(x, y, z);
        rb->append(x);
      }
      #else
      [rb](const MPU6050::gyro_data_t& entry)
      {
        rb->append(entry.gyro[0]);
      }
      #endif
      );
    max_datagram_count = std::max(datagram_count, max_datagram_count);
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
          #else
          fft_display->update(
            // We filter out the lowest frequency bins
            // because they contain DC and the drift.
            // The value is just experience, I need to dig
            // down deeper to understand that.
            fft->fft().begin() + 12,
            // This would go to n / 2, as we throw away
            // the negative frequencies. But this use-case
            // doesen't warrant those higher frequencies.
            fft->fft().begin() + fft->n / 2
            );
          #endif
        }
      }
      );
    const auto buttons = xEventGroupGetBits(button_events);
    if(buttons & LEFT_PIN_ISR_FLAG)
    {
      ESP_LOGI("main", "left button pressed");
      z_axis.reset();
    }
    if(buttons & RIGHT_PIN_ISR_FLAG)
    {
      ESP_LOGI("main", "right button pressed");
    }
    if(buttons & DOWN_PIN_ISR_FLAG)
    {
      ESP_LOGI("main", "down button pressed");
      z_axis.add(-0.5);
    }
    if(buttons & UP_PIN_ISR_FLAG)
    {
      ESP_LOGI("main", "up button pressed");
      z_axis.add(0.5);
    }

    xEventGroupClearBits(
      button_events,
      LEFT_PIN_ISR_FLAG | RIGHT_PIN_ISR_FLAG
      | DOWN_PIN_ISR_FLAG | UP_PIN_ISR_FLAG);
    #ifndef CONFIG_COFFEE_CLOCK_STREAM_DATA
    if(display.ready())
    {
      const auto now = esp_timer_get_time();
      const float fps = 1.0 / (float(now - timestamp) / 1000000.0);
      timestamp = now;
      ESP_LOGI("main", "fps: %f, rad: %f, max datagram count: %i", fps, z_axis.rad(), max_datagram_count);
      auto ds = display.sprite();
      test_sprite.restore(ds);

      // append a new line with the curent FFT
      // readings.
      display.vscroll();
      fft_display->render(display, 0, display.height() - 1);

      const auto rad = z_axis.rad();
      std::stringstream ss;
      ss << rad;
      test_sprite.fill(0x00);
      display.render_text(test_sprite, ss.str().c_str(), 8, 28 - 2, 1, 0);

      test_sprite.blit(ds, 2, 2);
      display.update();
    }
    #endif // not(CONFIG_COFFEE_CLOCK_STREAM_DATA)
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
