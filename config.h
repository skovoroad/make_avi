#pragma once

#include <string>

struct Config {
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

