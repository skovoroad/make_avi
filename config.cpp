#include <iostream>
#include <fstream>
#include <vector>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include "config.h"

bool parseConfig(
  int argc,
  char **argv,
  Config &config
  )
{
  namespace po = boost::program_options;
  try {
    po::options_description generalOptions{"General"};
    generalOptions.add_options()
      ("help,h", "Help screen")
      ("config", po::value<std::string>(), "Config file");
    
    po::options_description fileOptions{"File"};
    fileOptions.add_options()
      ("audio-data-in", po::value<std::string>(&config.audioData), "input file with audio binary data")
      ("audio-timestamps-in", po::value<std::string>(&config.audioTimestamps), "input file with audio timestamps and chunk sizes")
      ("video-data-in", po::value<std::string>(&config.videoData), "input file with video binary data")
      ("video-timestamps-in", po::value<std::string>(&config.videoTimestamps), "input file with video timestamps and chunk sizes")
      ("avi-file-out", po::value<std::string>(&config.fileOut), "output avi file")

    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, generalOptions), vm);
    po::notify(vm);
    if (vm.count("config")) {
      std::ifstream ifs { vm["config"].as<std::string>().c_str() };
      po::variables_map vmf;
 
      if (!ifs) {
        std::cerr << "cannot open config file: " << vm["config"].as<std::string>() << std::endl;
	return false;
      } 

      po::store(po::parse_config_file(ifs, fileOptions), vmf);
      po::notify(vmf);
    }
    
    if (vm.count("help")) {
      std::cout << "command line options: " << std::endl;
      std::cout << generalOptions << std::endl;
      std::cout << "config file options: " << std::endl;
      std::cout << fileOptions << std::endl;
      return false;
    }
  }
  catch (const po::error &ex)
  {
    std::cerr << ex.what() << std::endl;
  }
  return true;
}
