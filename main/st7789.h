// Copyright: 2020, Diez B. Roggisch, Berlin, all rights reserved
#pragma once

#define SPIFIFOSIZE                                 64
#define SWAPBYTES(i)                                ((i>>8) | (i<<8))

#define ST7789_SLPIN                                0x10
#define ST7789_SLPOUT                               0x11
#define ST7789_NORON                                0x13
#define ST7789_MADCTL                               0x36      // Memory data access control
#define TFT_MAD_RGB                                 0x08
#define ST7789_COLMOD                               0x3A
#define ST7789_PORCTRL                              0xB2      // Porch control
#define ST7789_GCTRL                                0xB7      // Gate control
#define ST7789_VCOMS                                0xBB      // VCOMS setting
#define ST7789_LCMCTRL                              0xC0      // LCM control
#define ST7789_VDVVRHEN                             0xC2      // VDV and VRH command enable
#define ST7789_VRHS                                 0xC3      // VRH set
#define ST7789_VDVSET                               0xC4      // VDV setting
#define ST7789_FRCTR2                               0xC6      // FR Control 2
#define ST7789_PWCTRL1                              0xD0      // Power control 1
#define ST7789_PVGAMCTRL                            0xE0      // Positive voltage gamma control
#define ST7789_NVGAMCTRL                            0xE1      // Negative voltage gamma control
#define ST7789_INVON                                0x21
#define ST7789_CASET                                0x2A
#define ST7789_RASET                                0x2B
#define ST7789_RAMWR                                0x2C
#define ST7789_DISPOFF                              0x28
#define ST7789_DISPON                               0x29
#define TFT_MAD_COLOR_ORDER                         TFT_MAD_RGB
#define TFT_MAD_MY                                  0x80
#define TFT_MAD_MX                                  0x40
#define TFT_MAD_MV                                  0x20
#define TFT_MAD_ML                                  0x10

#define TFT_BLACK                                   0x0000      /*   0,   0,   0 */
#define TFT_NAVY                                    0x000F      /*   0,   0, 128 */
#define TFT_DARKGREEN                               0x03E0      /*   0, 128,   0 */
#define TFT_DARKCYAN                                0x03EF      /*   0, 128, 128 */
#define TFT_MAROON                                  0x7800      /* 128,   0,   0 */
#define TFT_PURPLE                                  0x780F      /* 128,   0, 128 */
#define TFT_OLIVE                                   0x7BE0      /* 128, 128,   0 */
#define TFT_LIGHTGREY                               0xD69A      /* 211, 211, 211 */
#define TFT_DARKGREY                                0x7BEF      /* 128, 128, 128 */
#define TFT_BLUE                                    0x001F      /*   0,   0, 255 */
#define TFT_GREEN                                   0x07E0      /*   0, 255,   0 */
#define TFT_CYAN                                    0x07FF      /*   0, 255, 255 */
#define TFT_RED                                     0xF800      /* 255,   0,   0 */
#define TFT_MAGENTA                                 0xF81F      /* 255,   0, 255 */
#define TFT_YELLOW                                  0xFFE0      /* 255, 255,   0 */
#define TFT_WHITE                                   0xFFFF      /* 255, 255, 255 */
#define TFT_ORANGE                                  0xFDA0      /* 255, 180,   0 */
#define TFT_GREENYELLOW                             0xB7E0      /* 180, 255,   0 */
#define TFT_PINK                                    0xFE19      /* 255, 192, 203 */ //Lighter pink, was 0xFC9F
#define TFT_BROWN                                   0x9A60      /* 150,  75,   0 */
#define TFT_GOLD                                    0xFEA0      /* 255, 215,   0 */
#define TFT_SILVER                                  0xC618      /* 192, 192, 192 */
#define TFT_SKYBLUE                                 0x867D      /* 135, 206, 235 */
#define TFT_VIOLET                                  0x915C      /* 180,  46, 226 */

#define LCD_HOST                                    SPI2_HOST
#define DMA_CHAN                                    LCD_HOST

#define PIN_NUM_MISO                                4
#define PIN_NUM_MOSI                                35
#define PIN_NUM_CLK                                 36
#define PIN_NUM_CS                                  34

#define PIN_NUM_DC                                  37
#define PIN_NUM_RST                                 38
#define PIN_NUM_BCKL                                33
