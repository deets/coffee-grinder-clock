// Copyright: 2020, Diez B. Roggisch, Berlin, all rights reserved
#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_http_server.h>

#include <string>
#include <vector>
#include <functional>
#include <mutex>

#define STREAMER_TASK_STACK_SIZE 2000

const size_t MAX_BUFFER_SIZE = 1000 * 5; // 5 seconds of data, 20Kb

using http_callback_t = std::function<esp_err_t(httpd_req_t*)>;

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

  template<typename FFT>
  void deliver_fft(FFT& fft)
  {
    fft->copy_fft(_fft_incoming);
    swap(_fft_incoming, _fft_in_flight);
  }

private:
  static void s_run(void*);
  void run();
  httpd_handle_t start_webserver(void);
  static esp_err_t s_http_request_forwarder(httpd_req_t *req);

  esp_err_t get_raw_handler(httpd_req_t *req);
  esp_err_t get_fft_handler(httpd_req_t *req);

  template<typename C>
  void swap(C& left, C& right)
  {
    std::lock_guard<std::mutex> guard(_fft_mutex);
    std::swap(left, right);
  }


  TaskHandle_t _task_handle;
  StaticTask_t _task_buffer;
  StackType_t  _task_stack[STREAMER_TASK_STACK_SIZE];

  streamer_state_e _state;
  httpd_handle_t _server;

  std::vector<float> _raw_buffer;

  std::vector<float> _fft_incoming;
  std::vector<float> _fft_in_flight;
  std::vector<float> _fft_streaming;
  std::mutex _fft_mutex;

  http_callback_t _raw_get_callback;
  http_callback_t _fft_get_callback;

};
