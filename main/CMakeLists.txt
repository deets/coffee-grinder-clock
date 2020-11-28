set(srcs
  display.cpp
  display.hh
  fft-display.hh
  fft.hh
  font.c
  font.h
  main.cpp
  streamer.cpp
  streamer.hh
  )

idf_component_register(
  SRCS ${srcs}
  INCLUDE_DIRS "."
  )
