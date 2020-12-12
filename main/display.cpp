// Copyright: 2019, Diez B. Roggisch, Berlin, all rights reserved
#include "display.hh"
#include "st7789.h"
#include "colormap.hh"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <esp_log.h>

#include <array>
#include <algorithm>
#include <cstring>
#include <assert.h>

namespace {

uint16_t colstart = 52;
uint16_t rowstart = 40;
uint16_t _init_height = 240;
uint16_t _init_width = 135;
uint16_t _width = 135;
uint16_t _height = 240;

#define TRANSMIT_BUFFER 1

} // end namespace

//This function is called (in irq context!) just before a transmission starts. It will
//set the D/C line to the value indicated in the user field.
void lcd_spi_pre_transfer_callback(spi_transaction_t *t)
{
  int dc = static_cast<Display*>(t->user)->_dc;
  gpio_set_level(gpio_num_t(PIN_NUM_DC), dc);
}

/* Send a command to the LCD. Uses spi_device_polling_transmit, which waits
 * until the transfer is complete.
 *
 * Since command transactions are usually small, they are handled in polling
 * mode for higher speed. The overhead of interrupt transactions is more than
 * just waiting for the transaction to complete.
 */
void Display::lcd_cmd(spi_device_handle_t spi, const uint8_t cmd)
{
  esp_err_t ret;
  spi_transaction_t t;
  memset(&t, 0, sizeof(t));                   //Zero out the transaction
  t.length = 8;                               //Command is 8 bits
  t.tx_buffer = &cmd;                         //The data is the cmd itself
  dc(t, 0);                         //D/C needs to be set to 0
  ret = spi_device_polling_transmit(spi, &t); //Transmit!
  assert(ret == ESP_OK);                      //Should have had no issues.
}

/* Send data to the LCD. Uses spi_device_polling_transmit, which waits until the
 * transfer is complete.
 *
 * Since data transactions are usually small, they are handled in polling
 * mode for higher speed. The overhead of interrupt transactions is more than
 * just waiting for the transaction to complete.
 */
void Display::lcd_data(spi_device_handle_t spi, const uint8_t *data, int len)
{
    esp_err_t ret;
    spi_transaction_t t;
    if (len == 0) return;                       //no need to send anything
    std::memset(&t, 0, sizeof(t));                   //Zero out the transaction
    t.length = len * 8;                         //Len is in bytes, transaction length is in bits.
    t.tx_buffer = data;                         //Data
    dc(t, 1);                         //D/C needs to be set to 1
    ret = spi_device_polling_transmit(spi, &t); //Transmit!
    assert(ret == ESP_OK);                      //Should have had no issues.
}

void Display::lcd_write_u8(spi_device_handle_t spi, const uint8_t data)
{
  lcd_data(spi, &data, 1);
}

//Initialize the display
void Display::lcd_init(spi_device_handle_t spi)
{
    //Initialize non-SPI GPIOs
  gpio_set_direction(gpio_num_t(PIN_NUM_DC), GPIO_MODE_OUTPUT);
  gpio_set_direction(gpio_num_t(PIN_NUM_RST), GPIO_MODE_OUTPUT);
  gpio_set_direction(gpio_num_t(PIN_NUM_BCKL), GPIO_MODE_OUTPUT);

  //Reset the display
  gpio_set_level(gpio_num_t(PIN_NUM_RST), 0);
  vTaskDelay(100 / portTICK_RATE_MS);
  gpio_set_level(gpio_num_t(PIN_NUM_RST), 1);
  vTaskDelay(100 / portTICK_RATE_MS);


  lcd_cmd(spi, ST7789_SLPOUT);                // Sleep out
  vTaskDelay(120 / portTICK_RATE_MS);

  lcd_cmd(spi, ST7789_NORON);                 // Normal display mode on

  //------------------------------display and color format setting--------------------------------//
  lcd_cmd(spi, ST7789_MADCTL);
  lcd_write_u8(spi, TFT_MAD_RGB);

  // JLX240 display datasheet
  lcd_cmd(spi, 0xB6);
  lcd_write_u8(spi, 0x0A);
  lcd_write_u8(spi, 0x82);

  lcd_cmd(spi, ST7789_COLMOD);
  lcd_write_u8(spi, 0x55);
  vTaskDelay(10 / portTICK_RATE_MS);

  //--------------------------------ST7789V Frame rate setting----------------------------------//
  lcd_cmd(spi, ST7789_PORCTRL);
  lcd_write_u8(spi, 0x0c);
  lcd_write_u8(spi, 0x0c);
  lcd_write_u8(spi, 0x00);
  lcd_write_u8(spi, 0x33);
  lcd_write_u8(spi, 0x33);

  lcd_cmd(spi, ST7789_GCTRL);                 // Voltages: VGH / VGL
  lcd_write_u8(spi, 0x35);

  //---------------------------------ST7789V Power
  //setting--------------------------------------//
  lcd_cmd(spi,ST7789_VCOMS);
  lcd_write_u8(spi,0x28); // JLX240 display datasheet

  lcd_cmd(spi,ST7789_LCMCTRL);
  lcd_write_u8(spi,0x0C);

  lcd_cmd(spi,ST7789_VDVVRHEN);
  lcd_write_u8(spi,0x01);
  lcd_write_u8(spi,0xFF);

  lcd_cmd(spi,ST7789_VRHS); // voltage VRHS
  lcd_write_u8(spi,0x10);

  lcd_cmd(spi,ST7789_VDVSET);
  lcd_write_u8(spi,0x20);

  lcd_cmd(spi,ST7789_FRCTR2);
  lcd_write_u8(spi,0x0f);

  lcd_cmd(spi,ST7789_PWCTRL1);
  lcd_write_u8(spi,0xa4);
  lcd_write_u8(spi,0xa1);

  //--------------------------------ST7789V gamma
  //setting---------------------------------------//
  lcd_cmd(spi,ST7789_PVGAMCTRL);
  lcd_write_u8(spi,0xd0);
  lcd_write_u8(spi,0x00);
  lcd_write_u8(spi,0x02);
  lcd_write_u8(spi,0x07);
  lcd_write_u8(spi,0x0a);
  lcd_write_u8(spi,0x28);
  lcd_write_u8(spi,0x32);
  lcd_write_u8(spi,0x44);
  lcd_write_u8(spi,0x42);
  lcd_write_u8(spi,0x06);
  lcd_write_u8(spi,0x0e);
  lcd_write_u8(spi,0x12);
  lcd_write_u8(spi,0x14);
  lcd_write_u8(spi,0x17);

  lcd_cmd(spi,ST7789_NVGAMCTRL);
  lcd_write_u8(spi,0xd0);
  lcd_write_u8(spi,0x00);
  lcd_write_u8(spi,0x02);
  lcd_write_u8(spi,0x07);
  lcd_write_u8(spi,0x0a);
  lcd_write_u8(spi,0x28);
  lcd_write_u8(spi,0x31);
  lcd_write_u8(spi,0x54);
  lcd_write_u8(spi,0x47);
  lcd_write_u8(spi,0x0e);
  lcd_write_u8(spi,0x1c);
  lcd_write_u8(spi,0x17);
  lcd_write_u8(spi,0x1b);
  lcd_write_u8(spi,0x1e);

  lcd_cmd(spi,ST7789_INVON);

  lcd_cmd(spi,ST7789_DISPON); // Display on
  vTaskDelay(120 / portTICK_RATE_MS);

  /// Enable backlight
  gpio_set_level(gpio_num_t(PIN_NUM_BCKL), 1);
}

void Display::lcd_write_word(spi_device_handle_t spi, const uint16_t data)
{
    uint8_t val;
    val = data >> 8 ;
    lcd_data(spi, &val, 1);
    val = data;
    lcd_data(spi, &val, 1);
}

void Display::setAddress(spi_device_handle_t spi, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
  lcd_cmd(spi, ST7789_CASET);
  lcd_write_word(spi, x1 + colstart);
  lcd_write_word(spi, x2 + colstart);
  lcd_cmd(spi, ST7789_RASET);
  lcd_write_word(spi, y1 + rowstart);
  lcd_write_word(spi, y2 + rowstart);
  lcd_cmd(spi, ST7789_RAMWR);
}

void Display::setRotation(spi_device_handle_t spi, uint8_t m)
{
    uint8_t rotation = m % 4;
    lcd_cmd(spi, ST7789_MADCTL);
    switch (rotation) {
    case 0:
        colstart = 52;
        rowstart = 40;
        _width  = _init_width;
        _height = _init_height;
        lcd_write_u8(spi, TFT_MAD_COLOR_ORDER);
        break;

    case 1:
        colstart = 40;
        rowstart = 53;
        _width  = _init_height;
        _height = _init_width;
        lcd_write_u8(spi, TFT_MAD_MX | TFT_MAD_MV | TFT_MAD_COLOR_ORDER);
        break;
    case 2:
        colstart = 53;
        rowstart = 40;
        _width  = _init_width;
        _height = _init_height;
        lcd_write_u8(spi, TFT_MAD_MX | TFT_MAD_MY | TFT_MAD_COLOR_ORDER);
        break;
    case 3:
        colstart = 40;
        rowstart = 52;
        _width  = _init_height;
        _height = _init_width;
        lcd_write_u8(spi, TFT_MAD_MV | TFT_MAD_MY | TFT_MAD_COLOR_ORDER);
        break;
    }
}


Display::Display()
{
  esp_err_t ret;
  spi_bus_config_t buscfg;
  memset(&buscfg, 0, sizeof(spi_bus_config_t));

  buscfg.miso_io_num = PIN_NUM_MISO;

  buscfg.mosi_io_num = PIN_NUM_MOSI;
  buscfg.sclk_io_num = PIN_NUM_CLK;
  buscfg.quadwp_io_num = -1;
  buscfg.quadhd_io_num = -1;
  buscfg.max_transfer_sz = SPIFIFOSIZE * 240 * 2 + 8;

  spi_device_interface_config_t devcfg = {
    .command_bits = 0,
    .address_bits = 0,
    .dummy_bits = 0,
    .mode = 0,                              //SPI mode 0
    .duty_cycle_pos = 0,
    .cs_ena_pretrans = 0,
    .cs_ena_posttrans = 0,
    .clock_speed_hz = 40 * 1000 * 1000,
    .input_delay_ns = 0,
    .spics_io_num = PIN_NUM_CS,             //CS pin
    .flags = 0,
    .queue_size = 7,                        //We want to be able to queue 7 transactions at a time
    .pre_cb = lcd_spi_pre_transfer_callback, //Specify pre-transfer callback to handle D/C line
    .post_cb = nullptr,
  };
  //Initialize the SPI bus
  ret = spi_bus_initialize(LCD_HOST, &buscfg, DMA_CHAN);
  ESP_ERROR_CHECK(ret);
  //Attach the LCD to the SPI bus
  ret = spi_bus_add_device(LCD_HOST, &devcfg, &_spi);
  ESP_ERROR_CHECK(ret);
  //Initialize the LCD
  lcd_init(_spi);
  setRotation(_spi, 0);

  _buffer.resize(width() * height());
  fill_palette(_palette);
  // always tie 0 to black and 1 to white
  _palette[0] = 0x0;
  _palette[1] = 0xffff;
  _line.resize(width());
  _update_events = xEventGroupCreate();
  assert(_update_events);

  _update_task_handle = nullptr;
  xTaskCreate(
    Display::s_update_task,
    "dup", 8192, this, tskIDLE_PRIORITY + 1, &_update_task_handle);
  assert(_update_task_handle);
  _spi_transaction_ongoing = false;
}

void Display::s_update_task(void* display)
{
  static_cast<Display*>(display)->update_task();
}

int Display::height() const { return _height; }

int Display::width() const { return _width; }

void Display::clear()
{
  std::memset(_buffer.data(), 0, _buffer.size());
}

void Display::update() {
    _spi_transaction_ongoing = true;
    xEventGroupSetBits(_update_events, TRANSMIT_BUFFER);
}

void Display::update_task()
{
  while(true)
  {
    xEventGroupWaitBits(
      _update_events,   /* The event group being tested. */
      TRANSMIT_BUFFER, /* The bits within the event group to wait for. */
      pdTRUE,        /* BIT_0 & BIT_4 should be cleared before returning. */
      pdFALSE,       /* Don't wait for both bits, either bit will do. */
      portMAX_DELAY);/* Wait a maximum of 100ms for either bit to be set. */
    update_work();
    _spi_transaction_ongoing = false;
  }
}

void Display::update_work()
{
  setAddress(_spi, 0, 0, width() - 1, height() - 1);
  size_t offset = 0;

  for(size_t y=0; y < height(); ++y)
  {
    for(size_t x=0; x < width(); ++x)
    {
      _line[x] = SWAPBYTES(_palette[_buffer[offset + x]]);
    }
    offset += width();
    std::memset(&_spi_transaction, 0, sizeof(_spi_transaction));  //Zero out the transaction
    _spi_transaction.tx_buffer = _line.data();  //Data
    dc(_spi_transaction, 1);
    _spi_transaction.length = sizeof(decltype(_line)::value_type) * _line.size() * 8;   //Len is in byte
    spi_device_transmit(_spi, &_spi_transaction);
  }
}

bool Display::ready()
{
  return !_spi_transaction_ongoing.load();
}

void Display::dc(spi_transaction_t& t, int dc)
{
  t.user = this;
  _dc = dc;
}

void Display::draw_pixel(int x, int y, uint8_t color)
{
  _buffer[x + y * width()] = color;
}

void Display::blit(const sprite_t& sprite, int x, int y)
{
}

void Display::set_color(uint8_t color)
{
}

void Display::hline(int x, int x2, int y)
{
}

void Display::vline(int x, int y, int y2, uint8_t color)
{
  if(y > y2) { int h = y; y = y2; y2 = h; }
  auto length = y2 - y + 1;
  for(int yd=0; yd < length; ++yd)
  {
    draw_pixel(x, y + yd, color);
  }
}

void Display::circle(int x0, int y0, int rad, bool filled)
{
}


void Display::vscroll()
{
  std::copy(_buffer.begin() + width(), _buffer.end(), _buffer.begin());
}
