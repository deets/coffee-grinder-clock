// Copyright: 2020, Diez B. Roggisch, Berlin, all rights reserved
#include "ringbuffer.hh"

#include <esp_dsp.h>

#include <math.h>
#include <array>

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
    auto ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
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

  size_t size() const
  {
    return N;
  }

  template<typename T>
  size_t copy_fft(T& container)
  {
    container.resize(_fft.size());
    assert(container.size() == _fft.size());
    // the data seems to be organized in
    // N real and N imaginary components. I interleave them
    // to the standard (re + im) format
    auto re_it = _fft.begin();
    auto im_it = re_it + N;
    auto dst_it = container.begin();
    for(auto i = 0; i < N; ++i)
    {
      *dst_it++ = *re_it++;
      *dst_it++ = *im_it++;
    }
    return _fft.size();
  }

private:
  std::array<float, N> _window;
  std::array<float, N * 2> _complex_input;
  std::array<float, N * 2> _fft;

  rb_t _buffer;
  typename rb_t::Reader _rb_reader;
};
