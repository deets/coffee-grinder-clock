#include "display.hh"
#include "buzzer.hh"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/spi_master.h>
#include <nvs_flash.h>
#include <array>

extern "C" void app_main();

namespace {

const int MAINLOOP_WAIT = 16; // 60fps
const int WIFI_WAIT = 500;



} // end ns anon

void app_main()
{
  nvs_flash_init();

  Display display;
  Buzzer buzzer(static_cast<gpio_num_t>(14));

  std::array<char, 200> distance_text;
  for( ;; )
  {

    vTaskDelay(pdMS_TO_TICKS(MAINLOOP_WAIT));
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
