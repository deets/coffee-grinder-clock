set(srcs
  colormap.cpp
  colormap.hh
  display.cpp
  display.hh
  fft-display.hh
  fft.hh
  io-buttons.hh
  io-buttons.cpp
  main.cpp
  )

if(CONFIG_COFFEE_CLOCK_STREAM_DATA)
  list(
    APPEND srcs
    streamer.cpp
    streamer.hh
    )
endif()

idf_component_register(
  SRCS ${srcs}
  INCLUDE_DIRS "."
  )
