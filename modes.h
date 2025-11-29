#ifndef MODES_H
#define MODES_H

enum ModeKey {
  WSPR,
  CW,
  MODE_UNKNOWN
};

const char* getModeName(ModeKey mode) {
  switch (mode) {
    case WSPR: return "WSPR";
    case CW: return "CW";
    default: return "MODE_UNKNOWN";
  }
}

ModeKey getModeKeyFromString(const String& modeName) {
  if (modeName.equalsIgnoreCase("wspr")) return WSPR;
  if (modeName.equalsIgnoreCase("cw"))  return CW;
  return MODE_UNKNOWN;
}

#endif