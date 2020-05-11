#pragma once

#include <string>

struct Config {
  size_t width = 0;
  size_t height = 0;
  std::string audioData;
  std::string audioTimestamps;
  std::string videoData;
  std::string videoTimestamps;
  std::string fileOut;
};

bool parseConfig(
  int argc,
  char **argv,
  Config &config
  );

