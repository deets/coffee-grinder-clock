// Copyright: 2019, Diez B. Roggisch, Berlin, all rights reserved
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  const uint8_t *font;
  int size;
  int y_adjust;
} font_info_t;


extern font_info_t NORMAL;
extern font_info_t SMALL;

#ifdef __cplusplus
}
#endif
