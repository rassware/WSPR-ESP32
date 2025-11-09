#ifndef BANDS_H
#define BANDS_H

enum BandKey {
  BAND_160M,
  BAND_80M,
  BAND_40M,
  BAND_30M,
  BAND_20M,
  BAND_17M,
  BAND_15M,
  BAND_12M,
  BAND_10M,
  BAND_UNKNOWN
};

struct BandInfo {
  const char* band;
  unsigned long frequency;
};

BandInfo bands[] = {
  {"160m", 183800000ULL},
  {"80m",  357000000ULL},
  {"40m",  704000000ULL},
  {"30m", 1014010000ULL},
  {"20m", 1409700000ULL},
  {"17m", 1810600000ULL},
  {"15m", 2109600000ULL},
  {"12m", 2492600000ULL},
  {"10m", 2812600000ULL}
};

#endif