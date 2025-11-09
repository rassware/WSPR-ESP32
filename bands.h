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
  {"160m",  183800000ULL},
  {"80m",   357000000ULL},
  {"40m",   704000000ULL},
  {"30m",  1014010000ULL},
  {"20m",  1409700000ULL},
  {"17m",  1810600000ULL},
  {"15m",  2109600000ULL},
  {"12m",  2492600000ULL},
  {"10m",  2812600000ULL}
};

BandKey getBandKeyFromString(const String& bandName) {
  if (bandName.equalsIgnoreCase("160m")) return BAND_160M;
  if (bandName.equalsIgnoreCase("80m"))  return BAND_80M;
  if (bandName.equalsIgnoreCase("40m"))  return BAND_40M;
  if (bandName.equalsIgnoreCase("30m"))  return BAND_30M;
  if (bandName.equalsIgnoreCase("20m"))  return BAND_20M;
  if (bandName.equalsIgnoreCase("17m"))  return BAND_17M;
  if (bandName.equalsIgnoreCase("15m"))  return BAND_15M;
  if (bandName.equalsIgnoreCase("12m"))  return BAND_12M;
  if (bandName.equalsIgnoreCase("10m"))  return BAND_10M;
  return BAND_UNKNOWN;
}

#endif