#include "streamer.hh"
#include "wifi.hh"

#include <esp_log.h>
#include <mdns.h>

namespace {

const auto TAG = "das";

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
  if(_raw_buffer.size() < MAX_BUFFER_SIZE)
  {
    _raw_buffer.push_back(value);
  }
}

esp_err_t DataStreamer::get_raw_handler(httpd_req_t *req)
{
  httpd_resp_send(req, reinterpret_cast<const char*>(_raw_buffer.data()), _raw_buffer.size() * sizeof(decltype(_raw_buffer)::value_type));
  _raw_buffer.clear();
  return ESP_OK;
}

/* Function for starting the webserver */
httpd_handle_t DataStreamer::start_webserver(void)
{
    /* Generate default configuration */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* Empty handle to esp_http_server */
    httpd_handle_t server = NULL;

    /* Start the httpd server */
    if (httpd_start(&server, &config) == ESP_OK) {
      _raw_get_callback = [this](httpd_req_t* req)
                          {
                            return get_raw_handler(req);
                          };

      httpd_uri_t uri_get = {
        .uri      = "/",
        .method   = HTTP_GET,
        .handler  = s_http_request_forwarder,
        .user_ctx = &_raw_get_callback
      };
      httpd_register_uri_handler(server, &uri_get);
    }
    /* If server failed to start, handle will be NULL */
    return server;
}

esp_err_t DataStreamer::s_http_request_forwarder(httpd_req_t *req)
{
  return (*static_cast<http_callback_t*>(req->user_ctx))(req);
}
