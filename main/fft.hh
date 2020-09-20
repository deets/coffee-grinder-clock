// Copyright: 2020, Diez B. Roggisch, Berlin, all rights reserved
#include <esp_dsp.h>

#include <math.h>
#include <array>

template<int N>
class FFT
{
public:
  FFT()
    : _position(0)
  {
    auto ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    assert(ret == ESP_OK);
    dsps_wind_hann_f32(_window.data(), N);
  }

  bool feed(float re, float im)
  {
    _complex_input[_position * 2] = re;
    _complex_input[_position * 2 + 1] = im;
    _position = (_position + 1) % N;
    return _position == 0;
  }

  void compute()
  {
    apply_window();
    auto p = _complex_input.data();
    dsps_fft2r_fc32(p, N);
    // Bit reverse
    dsps_bit_rev_fc32(p, N);
    // Convert one complex vector to two complex vectors
    dsps_cplx2reC_fc32(p, N);
  }

  void display()
  {
    auto y1_cf = &_complex_input[0];
    auto y2_cf = &_complex_input[N];
    for (int i = 0 ; i < N/2 ; i++) {
      y1_cf[i] = 10 * log10f((y1_cf[i * 2 + 0] * y1_cf[i * 2 + 0] + y1_cf[i * 2 + 1] * y1_cf[i * 2 + 1])/N);
      y2_cf[i] = 10 * log10f((y2_cf[i * 2 + 0] * y2_cf[i * 2 + 0] + y2_cf[i * 2 + 1] * y2_cf[i * 2 + 1])/N);
      // Simple way to show two power spectrums as one plot
      _sum_y[i] = fmax(y1_cf[i], y2_cf[i]);
    }

    // Show power spectrum in 64x10 window from -100 to 0 dB from 0..N/4 samples
    // ESP_LOGW("fft", "Signal x1");
    // dsps_view(y1_cf, N/2, 64, 10,  -60, 40, '|');
    // ESP_LOGW("fft", "Signal x2");
    // dsps_view(y2_cf, N/2, 64, 10,  -60, 40, '|');
    ESP_LOGW("fft", "Signals x1 and x2 on one plot");
    dsps_view(_sum_y.data(), N/2, 64, 10,  -60, 40, '|');

  }

  void apply_window()
  {
    for(auto i=0; i < N; ++i)
    {
      _complex_input[i * 2] *= _window[i];
      _complex_input[i * 2 + 1] *= _window[i];
    }
  }

  size_t size() const
  {
    return N;
  }

  template<typename T>
  size_t copy_fft(T& container)
  {
    container.resize(_complex_input.size());
    assert(container.size() == _complex_input.size());
    // the data seems to be organized in
    // N real and N imaginary components. I interleave them
    // to the standard (re + im) format
    auto re_it = _complex_input.begin();
    auto im_it = re_it + N;
    auto dst_it = container.begin();
    for(auto i = 0; i < N; ++i)
    {
      *dst_it++ = *re_it++;
      *dst_it++ = *im_it++;
    }
    return _complex_input.size();
  }

private:
  size_t _position;
  std::array<float, N> _window;
  std::array<float, N * 2> _complex_input;
  std::array<float, N> _sum_y;
};
