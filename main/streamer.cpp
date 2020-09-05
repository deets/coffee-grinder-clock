#include "streamer.hh"
#include "wifi.hh"

#include <esp_log.h>
#include <mdns.h>

#include <vector>

namespace {

size_t MAX_BUFFER_SIZE = 1000 * 5; // 5 seconds of data, 20Kb

std::vector<float> s_buffer;

const auto TAG = "das";

esp_err_t get_handler(httpd_req_t *req)
{
  ESP_LOGI(TAG, "http data: %i", s_buffer.size());
  httpd_resp_send(req, reinterpret_cast<const char*>(s_buffer.data()), s_buffer.size() * sizeof(decltype(s_buffer)::value_type));
  s_buffer.clear();
  return ESP_OK;
}

/* URI handler structure for GET /uri */
httpd_uri_t uri_get = {
    .uri      = "/",
    .method   = HTTP_GET,
    .handler  = get_handler,
    .user_ctx = NULL
};

/* Function for starting the webserver */
httpd_handle_t start_webserver(void)
{
    /* Generate default configuration */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* Empty handle to esp_http_server */
    httpd_handle_t server = NULL;

    /* Start the httpd server */
    if (httpd_start(&server, &config) == ESP_OK) {
        /* Register URI handlers */
        httpd_register_uri_handler(server, &uri_get);
    }
    /* If server failed to start, handle will be NULL */
    return server;
}

void setup_mdns()
{
  ESP_ERROR_CHECK(mdns_init());
  mdns_hostname_set("coffee-grinder-clock");
  mdns_instance_name_set("deets coffee grinder clock");
  mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
}

} // end ns anonymous

DataStreamer::DataStreamer(const std::string&, int)
{
  _task_handle = xTaskCreateStatic(
    s_run,       // Function that implements the task.
    "DAS",          // Text name for the task.
    STREAMER_TASK_STACK_SIZE,      // Stack size in bytes, not words.
    this,
    tskIDLE_PRIORITY + 1,// Priority at which the task is created.
    _task_stack,          // Array to use as the task's stack.
    &_task_buffer // Variable to hold the task's data structure.
    );
}


void DataStreamer::s_run(void* arg)
{
  static_cast<DataStreamer*>(arg)->run();
}


void DataStreamer::run()
{
  _state = IDLE;
  while(true)
  {
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "running, wifi state: %i", wifi_connected());
    switch(_state)
    {
    case IDLE:
      if(wifi_connected())
      {
        setup_mdns();
        ESP_LOGI(TAG, "WIFI_ENABLED, mDNS set up");
        _state = WIFI_ENABLED;
        _server = start_webserver();
      }
      break;
    case WIFI_ENABLED:
      break;
    case LISTENING:
      break;
    }
  }
}

void DataStreamer::feed(float value)
{
  if(s_buffer.size() < MAX_BUFFER_SIZE)
  {
    s_buffer.push_back(value);
  }
}
