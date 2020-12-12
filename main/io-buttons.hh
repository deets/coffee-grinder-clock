// Copyright: 2019, Diez B. Roggisch, Berlin, all rights reserved
#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <unordered_map>

#define LEFT_PIN_ISR_FLAG 1

void iobuttons_setup(EventGroupHandle_t button_events);
