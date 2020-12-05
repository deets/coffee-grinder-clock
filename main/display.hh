// Copyright: 2019, Diez B. Roggisch, Berlin, all rights reserved
// -*- mode: c++-mode -*-
#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "driver/spi_master.h"
#include <freertos/event_groups.h>

#include <vector>
#include <array>
#include <atomic>

struct sprite_t
{
  unsigned short* data;
  int width;
  int height;
  int hotspot_x;
  int hotspot_y;
};

class Display {
public:
  Display();

  bool ready();

  int height() const;
  int width() const;

  void clear();
  void update();
  void set_color(uint8_t color);
  void draw_pixel(int x, int y, uint8_t color);
  void circle(int x0, int y0, int rad, bool filled=false);
  void blit(const sprite_t&, int x, int y);
  void hline(int x, int x2, int y);
  void vline(int x, int y1, int y2, uint8_t color);
  void vscroll();

private:
  friend void lcd_spi_pre_transfer_callback(spi_transaction_t *t);
  friend void lcd_spi_post_transfer_callback(spi_transaction_t *t);

  void lcd_cmd(spi_device_handle_t spi, const uint8_t cmd);
  void lcd_data(spi_device_handle_t spi, const uint8_t *data, int len);
  void lcd_write_u8(spi_device_handle_t spi, const uint8_t data);
  void lcd_init(spi_device_handle_t spi);
  void lcd_write_word(spi_device_handle_t spi, const uint16_t data);
  void setAddress(spi_device_handle_t spi, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
  void setRotation(spi_device_handle_t spi, uint8_t m);

  void dc(spi_transaction_t&, int);

  static void s_update_task(void*);
  void update_task();
  void update_work();

  spi_device_handle_t _spi;
  std::vector<uint8_t> _buffer;
  std::array<uint16_t, 256> _palette;
  std::vector<uint16_t> _line;
  EventGroupHandle_t _update_events;

  TaskHandle_t _update_task_handle;
  spi_transaction_t _spi_transaction;
  int _dc;
  std::atomic<bool> _spi_transaction_ongoing;
};
