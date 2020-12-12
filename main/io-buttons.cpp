#include "io-buttons.hh"

#include <soc/gpio_struct.h>
#include <driver/gpio.h>
#include <esp_log.h>

#include <unordered_map>
#include <stdint.h>


namespace
{


const gpio_num_t GPIO_LEFT = gpio_num_t(8);

const int DEBOUNCE = (200 * 1000);

std::unordered_map<int, uint64_t> s_debounces = {
    // {GPIO_DOWN, 200 * 1000},
    // {GPIO_UP, 200 * 1000},
    // {GPIO_RIGHT, 20 * 1000},
    {GPIO_LEFT, 20 * 1000}
};

std::unordered_map<int, uint64_t> s_last;

static EventGroupHandle_t s_button_events;

void IRAM_ATTR gpio_isr_handler(void* arg)
{
  BaseType_t higher_prio_has_woken;
  int pin = (int)arg;
  int bit = 0;
  int64_t ts = esp_timer_get_time();
  if(s_last.count(pin) && s_last[pin] + s_debounces[pin] > ts)
  {
    return;
  }
  s_last[pin] = ts;

  switch(pin)
  {
  // case GPIO_RIGHT:
  //   bit = RIGHT_PIN_ISR_FLAG;
  //   break;
  case GPIO_LEFT:
    bit = LEFT_PIN_ISR_FLAG;
    break;
  // case GPIO_DOWN:
  //   bit = DOWN_PIN_ISR_FLAG;
  //   break;
  // case GPIO_UP:
  //   bit = UP_PIN_ISR_FLAG;
  //   break;
  }

  auto xHigherPriorityTaskWoken = pdFALSE;
  auto xResult = xEventGroupSetBitsFromISR(
    s_button_events,
    bit,
    &xHigherPriorityTaskWoken );

  // Was the message posted successfully?
  if( xResult == pdPASS )
  {
    // If xHigherPriorityTaskWoken is now set to pdTRUE then a context
    // switch should be requested.  The macro used is port specific and
    // will be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() -
    // refer to the documentation page for the port being used.
    portYIELD_FROM_ISR();
  }
}

} // end ns anonymous

void iobuttons_setup(EventGroupHandle_t button_events)
{
  s_button_events = button_events;

  gpio_config_t io_conf;
  //interrupt of rising edge
  io_conf.intr_type = (gpio_int_type_t)GPIO_PIN_INTR_POSEDGE;
  io_conf.pin_bit_mask = \
  // (1ULL<< GPIO_DOWN) |
  // (1ULL<< GPIO_RIGHT) |
  // (1ULL<< GPIO_UP) |
  (1ULL<< GPIO_LEFT);

  //set as input mode
  io_conf.mode = GPIO_MODE_INPUT;
  //enable pull-up mode
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  gpio_config(&io_conf);

  // install global GPIO ISR handler
  gpio_install_isr_service(0);
  // install individual interrupts
  // gpio_isr_handler_add(GPIO_DOWN, gpio_isr_handler, (void*)GPIO_DOWN);
  // gpio_isr_handler_add(GPIO_RIGHT, gpio_isr_handler, (void*)GPIO_RIGHT);
  // gpio_isr_handler_add(GPIO_UP, gpio_isr_handler, (void*)GPIO_UP);
  gpio_isr_handler_add(GPIO_LEFT, gpio_isr_handler, (void*)GPIO_LEFT);
  // fill map to avoid allocates in ISR
  int64_t ts = esp_timer_get_time();
  for(const auto& item : s_debounces)
  {
    s_last[item.first] = ts;
  }
}
