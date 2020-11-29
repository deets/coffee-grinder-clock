// Copyright: 2019, Diez B. Roggisch, Berlin, all rights reserved
// -*- mode: c++-mode -*-
#pragma once
#include "font.h"
#include "driver/spi_master.h"

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

  int height() const;
  int width() const;

  void clear();
  void update();
  void set_color(uint8_t color);
  void draw_pixel(int x, int y, uint16_t color);
  void circle(int x0, int y0, int rad, bool filled=false);
  void blit(const sprite_t&, int x, int y);
  void hline(int x, int x2, int y);
  void vline(int x, int y1, int y2);

  void font_render(const font_info_t& font, const char*, int x, int y);
  int font_text_width(const font_info_t& font, const char*);

private:

  spi_device_handle_t _spi;
};
