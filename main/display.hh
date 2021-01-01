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

class Sprite {
public:
  Sprite(size_t width, size_t height, uint8_t* image=nullptr)
    : _width(width)
    , _height(height)
  {
    if(image)
    {
      _borrowed = true;
      _image = image;
    }
    else
    {
      _borrowed = false;
      _image = new uint8_t[_width * _height];
    }
  }

  ~Sprite()
  {
    if(!_borrowed)
    {
      delete [] _image;
    }
  }

  size_t width() const noexcept
  {
    return _width;
  }

  size_t height() const noexcept
  {
    return _height;
  }

  uint8_t* data()
  {
    return _image;
  }

  template<class T>
  void blit(T& other, size_t x, size_t y)
  {
    assert(other.width() >= width() + x);
    assert(other.height() >= height() + y);
    assert(x >= 0 && x < other.width());
    assert(y >= 0 && y < other.height());
    size_t modulo = other.width() - width();
    auto dest = other.data();
    dest += x + other.width() * y;
    auto source = _image;
    copy(source, dest, modulo, width(), height());
  }
protected:
  inline void copy(const uint8_t *source, uint8_t *dest, size_t modulo, size_t width, size_t height)
  {
    for(size_t sy = 0; sy < height; ++sy)
    {
      for(size_t sx = 0; sx < width; ++sx)
      {
        *dest++ = *source++;
      }
      dest += modulo;
    }
  }

private:
  size_t _width, _height;
  uint8_t* _image;
  bool _borrowed;
};

class BufferedSprite: public Sprite
{
public:
  BufferedSprite(size_t width, size_t height, uint8_t* image=nullptr)
    : Sprite(width, height, image)
    , _buffered(false)
  {
    _buffer.resize(height * width);
  }

  template<class T>
  void restore(T& other)
  {
    if(_buffered)
    {
      assert(other.width() >= width() + _x);
      assert(other.height() >= height() + _y);
      assert(_x >= 0 && _x < other.width());
      assert(_y >= 0 && _y < other.height());
      size_t modulo = other.width() - width();
      auto dest = other.data();
      auto source = _buffer.data();
      dest += other.width() * _y + _x;
      copy(source, dest, modulo, width(), height());
    }
    _buffered = false;
  }

  template<class T>
  void blit(T& other, size_t x, size_t y)
  {
    assert(other.width() >= width() + x);
    assert(other.height() >= height() + y);
    assert(x >= 0 && x < other.width());
    assert(y >= 0 && y < other.height());
    size_t modulo = other.width() - width();
    auto dest = _buffer.data();
    auto source = other.data();
    source += other.width() * y + x;

    for(size_t sy = 0; sy < height(); ++sy)
    {
      for(size_t sx = 0; sx < width(); ++sx)
      {
        *dest++ = *source++;
      }
      source += modulo;
    }

    Sprite::blit(other, x, y);
    _buffered = true;
    _x = x;
    _y = y;
  }

private:
  std::vector<uint8_t> _buffer;
  bool _buffered;
  size_t _x, _y;
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
  void hline(int x, int x2, int y);
  void vline(int x, int y1, int y2, uint8_t color);
  void vscroll();
  Sprite sprite()
  {
    return Sprite(width(), height(), _buffer.data());
  }


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
