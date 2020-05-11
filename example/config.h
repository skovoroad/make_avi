#pragma once

#include <string>

struct Config {
  uint32_t width = 0;
  uint32_t height = 0;
  std::string audioData;
  std::string audioTimestamps;
  std::string videoData;
  std::string videoTimestamps;
  std::string fileOut;
  std::string mediatype;
};

bool parseConfig(
  int argc,
  char **argv,
  Config &config
  );

