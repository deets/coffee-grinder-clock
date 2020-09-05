// Copyright: 2020, Diez B. Roggisch, Berlin, all rights reserved
#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_http_server.h>

#include <string>

#define STREAMER_TASK_STACK_SIZE 2000

class DataStreamer
{
  enum streamer_state_e
  {
    IDLE,
    WIFI_ENABLED,
    LISTENING
  };

public:
  DataStreamer(const std::string& ip_destiniation, int port=55555);

  void feed(float value);
private:
  static void s_run(void*);
  void run();

  TaskHandle_t _task_handle;
  StaticTask_t _task_buffer;
  StackType_t  _task_stack[STREAMER_TASK_STACK_SIZE];

  streamer_state_e _state;
  httpd_handle_t _server;
};
