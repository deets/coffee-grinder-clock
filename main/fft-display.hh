// Copyright: 2020, Diez B. Roggisch, Berlin, all rights reserved
#pragma once
#include "display.hh"
#include <esp_log.h>

#include <cmath>
#include <optional>

float lerp(float a, float b, float f) { return a + f * (b - a); }

const float NO_SCALE = 12345.678;

struct LogTransform
{
  static float transform(const float& value)
  {
    return log10f(value) * 20;
  }
};

struct NoTransform
{
  static float transform(const float& value)
  {
    return value;
  }
};

template<int W, typename Transform=NoTransform>
class FFTDisplay
{

public:

  const int width = W;
  template<typename T>
  void update(T begin, T end)
  {
    _bars.fill(0);
    const auto fft_width = end - begin;
    if(fft_width >= W)
    {
      // very simple downsampling algorithm
      // based on bresenham
      int accu = fft_width;
      int divider = 0;
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
    else
    {
      for(size_t i=0; i < W; ++i)
      {
        const auto position = lerp(0.0, fft_width - 1, float(i) / (W - 1));
        const auto lower = int(floor(position));
        const auto upper = int(ceil(position));
        if(lower == upper)
        {
          _bars[i] = *(begin + lower);
        }
        else
        {
          _bars[i] = lerp(*(begin + lower), *(begin + upper), position - lower) ;
        }
      }
    }
  }

  void render(Display& display, int x, int y)
  {
    const auto min = Transform::transform(*std::min_element(_bars.begin(), _bars.end()));
    const auto max = Transform::transform(*std::max_element(_bars.begin(), _bars.end()));
    const auto diff = max - min;
    const auto scale = filtered_scale(diff);
    std::transform(
      _bars.begin(),
      _bars.end(),
      _color.begin(),
      [&](const float v) {
        return uint8_t(2.0 + std::min(v * scale, 253.0f));
      });
    int xs = 0;
    for(const auto& color : _color)
    {
      display.draw_pixel(x + xs++, y, color);
    }
  }

  float filtered_scale(float diff)
  {
    auto new_scale = 253.0 / diff;
    if(_filtered_scale != NO_SCALE) // && new_scale > _filtered_scale)
    {
      new_scale = _filtered_scale + (new_scale - _filtered_scale) * _scale_gain;
    }
    _filtered_scale = new_scale;
    return _filtered_scale;
  }

private:
  float _filtered_scale = NO_SCALE;
  float _scale_gain = 0.01;
  std::array<float, W> _bars;
  std::array<uint8_t, W> _color;
};
