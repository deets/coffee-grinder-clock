// Copyright: 2020, Diez B. Roggisch, Berlin, all rights reserved
#pragma once
#include "display.hh"
#include <esp_log.h>

template<int W, int H, int DB>
class FFTDisplay
{

public:
  const int width = W;
  const int height = H;
  const int dB = DB;

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
    const auto max_limit = powf(10, float(DB) / 20);
    const auto min = *std::min_element(_bars.begin(), _bars.end());
    const auto max = *std::max_element(_bars.begin(), _bars.end());
    const auto db = log10f(max) * 20;
    // we don't drop below the max_limit for displaying
    const auto height = std::max(max, max_limit) - min;

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

    render_db(display, db, x, y);
  }

private:
  void render_db(Display& display, float db, int x, int y)
  {
    // char db_buffer[50];
    // sprintf(db_buffer, "dB: %i", int(db));
    // display.font_render(
    //   SMALL,
    //   db_buffer,
    //   x + W - display.font_text_width(SMALL, db_buffer),
    //   y + SMALL.size + 2
    //   );

  }
  std::array<float, W> _bars;
};
