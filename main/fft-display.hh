// Copyright: 2020, Diez B. Roggisch, Berlin, all rights reserved
#pragma once
#include "display.hh"
#include <esp_log.h>

template<int W, int H>
class FFTDisplay
{

public:

  template<typename T>
  void update(T begin, T end)
  {
    assert(end - begin > W);
    for(auto i=0; i < W; ++i)
    {
      const auto value = *(begin + i);
      //ESP_LOGI("fftd", "v: %f", value);
      _bars[i] = value;
    }
  }

  void render(Display& display, int x, int y)
  {
    const auto min = *std::min_element(_bars.begin(), _bars.end());
    const auto max = *std::max_element(_bars.begin(), _bars.end());
    const auto height = max - min;

    ESP_LOGI("fftd", "remapping case: %f", height);
    const auto scale = double(H) / height;
    std::transform(
      _bars.begin(),
      _bars.end(),
      _bars.begin(),
      [&](const float v) { return v * scale + min; });
    int xs = 0;
    const auto bottom = y + H - 1;
    for(const auto value : _bars)
    {
      display.vline(x + xs++, bottom - value, bottom);
    }
  }

private:

  std::array<float, W> _bars;
};
