#include "buzzer.hh"

Buzzer::Buzzer(gpio_num_t pin)
  : _pin(pin)
{
  gpio_config_t io_conf = {
    .pin_bit_mask = 1ULL << _pin,
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en=GPIO_PULLUP_DISABLE,
    .pull_down_en=GPIO_PULLDOWN_DISABLE,
    .intr_type=GPIO_INTR_DISABLE
  };
  gpio_config(&io_conf);

  esp_timer_create_args_t timer_args = {
    .callback=&Buzzer::s_timer_task,
    .arg=this,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "buzzer_timer"
  };

  auto err = esp_timer_create(
    &timer_args,
    &_buzzer_timer_handle
    );
  assert(err == ESP_OK);
}


void Buzzer::buzz(int ms, int repetitions)
{
  esp_timer_stop(_buzzer_timer_handle);
  gpio_set_level(_pin, 1);
  _buzzer_repetitions = repetitions * 2 - 1;
  esp_timer_start_periodic(_buzzer_timer_handle, ms * 1000);
}


void Buzzer::buzz(buzz_t b)
{
  switch(b)
  {
  case LAP:
    buzz(100, 1);
    break;
  case RACE_OVER:
    buzz(200, 3);
    break;
  }
}


void Buzzer::timer_task()
{
  if(!_buzzer_repetitions)
  {
    gpio_set_level(_pin, 0);
    esp_timer_stop(_buzzer_timer_handle);
  }
  else
  {
    gpio_set_level(_pin, (_buzzer_repetitions % 2) == 0);
    --_buzzer_repetitions;
  }
}


void Buzzer::s_timer_task(void* that)
{
  ((Buzzer*)that)->timer_task();
}
