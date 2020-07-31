// Copyright: 2019, Diez B. Roggisch, Berlin, all rights reserved
// -*- mode: c++-mode -*-
#pragma once
#include "font.h"

#include <u8g2_esp32_hal.h>

#include <u8g2.h>

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

  int height() const { return 64 ;}
  int width() const { return 128 ;}

  void clear();
  void update();
  void set_color(uint8_t color);
  void draw_pixel(int x, int y);
  void circle(int x0, int y0, int rad, bool filled=false, uint8_t opt=U8G2_DRAW_ALL);
  void blit(const sprite_t&, int x, int y);
  void hline(int x, int x2, int y);

  void font_render(const font_info_t& font, const char*, int x, int y);
  int font_text_width(const font_info_t& font, const char*);

  u8g2_t* handle();

private:
  u8g2_esp32_hal_t _u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
  u8g2_t _u8g2;

};
