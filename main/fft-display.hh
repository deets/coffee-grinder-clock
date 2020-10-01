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
    char db_buffer[50];
    sprintf(db_buffer, "dB: %i", int(db));
    display.font_render(
      SMALL,
      db_buffer,
      x + W - display.font_text_width(SMALL, db_buffer),
      y + SMALL.size + 2
      );

  }
  std::array<float, W> _bars;
};
