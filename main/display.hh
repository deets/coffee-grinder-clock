// Copyright: 2019, Diez B. Roggisch, Berlin, all rights reserved
// -*- mode: c++-mode -*-
#pragma once

#include "driver/spi_master.h"

#include <vector>
#include <array>

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
  void draw_pixel(int x, int y, uint8_t color);
  void circle(int x0, int y0, int rad, bool filled=false);
  void blit(const sprite_t&, int x, int y);
  void hline(int x, int x2, int y);
  void vline(int x, int y1, int y2);
  void flame();

private:

  spi_device_handle_t _spi;
  std::vector<uint8_t> _buffer;
  std::array<uint16_t, 256> _palette;
  std::vector<uint16_t> _line;
};
