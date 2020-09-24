// Copyright: 2020, Diez B. Roggisch, Berlin, all rights reserved
#include "ringbuffer.hh"

#include <esp_dsp.h>

#include <math.h>
#include <array>
#include <algorithm>


template<int N, int OVERLAP>
class FFT
{
  struct complex
  {
    float re;
    float im;
  };

  using rb_t = RingBuffer<complex, OVERLAP * 2>;

public:
  FFT()
    : _rb_reader(_buffer.reader())
  {
    auto ret = dsps_fft2r_init_fc32(NULL, N);
    assert(ret == ESP_OK);
    dsps_wind_hann_f32(_window.data(), N);
    _complex_input.fill(0.0);
  }

  bool feed(float re, float im)
  {
    _buffer.append({re, im});
    return _rb_reader.count() >= OVERLAP;
  }

  void update_input()
  {
    std::copy(
      _complex_input.begin() + OVERLAP * 2,
      _complex_input.end(),
      _complex_input.begin()
      );
    const auto offset = N * 2 - OVERLAP * 2;
    for(size_t i=0; i < OVERLAP; ++i)
    {
      auto input = _rb_reader.read();
      _complex_input[offset + i * 2] = input.re;
      _complex_input[offset + i * 2 + 1] = input.im;
    }
  }

  void compute()
  {
    update_input();
    apply_window();
    auto p = _fft.data();
    dsps_fft2r_fc32(p, N);
    // Bit reverse
    dsps_bit_rev_fc32(p, N);
    // Convert one complex vector to two complex vectors
    dsps_cplx2reC_fc32(p, N);
  }

  void apply_window()
  {
    for(auto i=0; i < N; ++i)
    {
      _fft[i * 2] = _complex_input[i * 2] * _window[i];
      _fft[i * 2 + 1] = _complex_input[i * 2 + 1] * _window[i];
    }
  }

  void postprocess()
  {
    // This processes the data so we get a usable
    // real-valued spectrum

    // generate spectrum
    std::transform(
      _fft.begin(),
      _fft.end(),
      _fft.begin(),
      [](const float v) { return v / N; }
      );
  }

  size_t size() const
  {
    return N;
  }

  template<typename T>
  size_t copy_fft(T& container)
  {
    container.resize(_fft.size());
    std::copy(_fft.begin(), _fft.end(), container.begin());
    return _fft.size();
  }

private:
  std::array<float, N> _window;
  std::array<float, N * 2> _complex_input;
  std::array<float, N * 2> _fft;

  rb_t _buffer;
  typename rb_t::Reader _rb_reader;
};
