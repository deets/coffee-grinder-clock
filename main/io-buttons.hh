// Copyright: 2019, Diez B. Roggisch, Berlin, all rights reserved
#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <unordered_map>

#define LEFT_PIN_ISR_FLAG (1 << 0)
#define RIGHT_PIN_ISR_FLAG (1 << 1)
#define DOWN_PIN_ISR_FLAG (1 << 2)
#define UP_PIN_ISR_FLAG (1 << 3)

void iobuttons_setup(EventGroupHandle_t button_events);
