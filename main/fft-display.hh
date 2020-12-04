// Copyright: 2020, Diez B. Roggisch, Berlin, all rights reserved
#pragma once
#include "display.hh"
#include <esp_log.h>

template<int W>
class FFTDisplay
{

public:

  const int width = W;
  template<typename T>
  void update(T begin, T end)
  {
    const auto fft_width = end - begin;
    assert(fft_width >= W);
    // very simple downsampling algorithm
    // based on bresenham
    int accu = fft_width;
    int divider = 0;
    _bars.fill(0);
    int i = 0;
    for(; begin != end; ++begin)
    {
      const auto value = *begin;
      _bars[i] += value;
      ++divider;
      accu -= W;
      if(accu <= 0)
      {
        _bars[i] /= divider;
        divider = 0;
        accu += fft_width;
        ++i;
      }
    }
  }

  void render(Display& display, int x, int y)
  {
    const auto min = *std::min_element(_bars.begin(), _bars.end());
    const auto max = *std::max_element(_bars.begin(), _bars.end());
    const auto height = max - min;
    ESP_LOGI("fftd", "remapping case: %f", height);
    const auto scale = 253.0 / height;
    std::transform(
      _bars.begin(),
      _bars.end(),
      _color.begin(),
      [&](const float v) {
        return uint8_t(2.0 + v * scale);
      });
    int xs = 0;
    for(const auto& color : _color)
    {
      display.draw_pixel(x + xs++, y, color);
    }
  }

private:

  std::array<float, W> _bars;
  std::array<uint8_t, W> _color;
};
