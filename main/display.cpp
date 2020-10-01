// Copyright: 2019, Diez B. Roggisch, Berlin, all rights reserved
#include "display.hh"

// DISPLAY
#define DISPLAY_CLK  23
#define DISPLAY_MOSI 22
#define DISPLAY_RST  19
#define DISPLAY_DC   05
#define DISPLAY_CS   16

#define DISPLAY_SPEED (2 * 1000*1000)


Display::Display()
{
  _u8g2_esp32_hal.clk   = (gpio_num_t)DISPLAY_CLK;
  _u8g2_esp32_hal.mosi  = (gpio_num_t)DISPLAY_MOSI;
  _u8g2_esp32_hal.cs    = (gpio_num_t)DISPLAY_CS;
  _u8g2_esp32_hal.dc    = (gpio_num_t)DISPLAY_DC;
  _u8g2_esp32_hal.reset = (gpio_num_t)DISPLAY_RST;

  u8g2_esp32_hal_init(_u8g2_esp32_hal);

   u8g2_Setup_sh1106_128x64_noname_f(
    &_u8g2,
    U8G2_R0,
    u8g2_esp32_spi_byte_cb,
    u8g2_esp32_gpio_and_delay_cb);  // init u8g2 structure

  u8g2_InitDisplay(&_u8g2); // send init sequence to the display, display is in sleep mode after this,
  u8g2_SetPowerSave(&_u8g2, 0); // wake up display
}

void Display::clear()
{
  u8g2_ClearBuffer(&_u8g2);
  set_color(0);
}

void Display::update()
{
  u8g2_SendBuffer(&_u8g2);
}

void Display::draw_pixel(int x, int y)
{
  u8g2_DrawPixel(&_u8g2, x, y);
}

void Display::blit(const sprite_t& sprite, int x, int y)
{
  u8g2_SetDrawColor(&_u8g2, 0);
  u8g2_DrawXBM(
    &_u8g2,
    x - sprite.hotspot_x, y - sprite.hotspot_y,
    sprite.width, sprite.height,
    reinterpret_cast<const uint8_t*>(sprite.data)
    );
}

void Display::set_color(uint8_t color)
{
  u8g2_SetDrawColor(&_u8g2, color);
}

void Display::hline(int x, int x2, int y)
{
  u8g2_DrawHLine(
    &_u8g2,
    x, y, abs(x2 - x) + 1
    );
}


void Display::vline(int x, int y, int y2)
{
  u8g2_DrawVLine(
    &_u8g2,
    x, y, abs(y2 - y) + 1
    );
}

void Display::circle(int x0, int y0, int rad, bool filled, uint8_t opt)
{
  if(filled)
  {
    u8g2_DrawDisc(&_u8g2, x0, y0, rad, opt);
  }
  else
  {
    u8g2_DrawCircle(&_u8g2, x0, y0, rad, opt);
  }
}

void Display::font_render(const font_info_t& font, const char* text, int x, int y)
{
  u8g2_SetFontMode(&_u8g2, 0); // draw solid (background is rendered)
  u8g2_SetDrawColor(&_u8g2, 1); // draw with white
  u8g2_SetFont(&_u8g2, font.font);
  u8g2_DrawStr(&_u8g2, x, y + font.y_adjust, text);
}


int Display::font_text_width(const font_info_t& font, const char* text)
{
  u8g2_SetFont(&_u8g2, font.font);
  return u8g2_GetStrWidth(&_u8g2, text);
}


u8g2_t* Display::handle()
{
  return &_u8g2;
}
