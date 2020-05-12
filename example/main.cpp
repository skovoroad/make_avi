#include <cassert>
#include <iostream>
#include <fstream>
#include <memory>

#include "config.h"
#include "build_avi.h"

struct TestData {
  enum Type { AUDIO, VIDEO };
  struct PacketInfo {
    double ts = 0.0;
    size_t size = 0;
    Type type = AUDIO;
    const char *buffer = 0;
  };
  std::vector<PacketInfo> tss; // timestamp - size
  std::vector<char> data;

  bool read(Type t, const char *data_filename, const char * ts_filename) {
    std::ifstream f(data_filename, std::ios::binary | std::ios::in);
    if(!f)
      return false;

    std::copy(  std::istreambuf_iterator<char>(f), 
                std::istreambuf_iterator<char>(),
                std::back_inserter(data));

    std::ifstream fts(ts_filename, std::ios::binary);

    size_t pos = 0;    
    while(fts) {
      PacketInfo p;
      fts >> p.ts >> p.size;
      p.buffer = data.data() + pos;
      p.type = t;
      tss.push_back(p);
      pos += p.size;
    }
    return true;
  }
};

class FileDumper {
public:
  FileDumper(const char *fname) 
    : filename(fname)
  {}

  ~FileDumper() {
    if(ofstr)
      ofstr.close();
  }

  bool onAvi (const void *data, size_t nbytes)  {
    if(!ofstr) {
      ofstr.open(filename.c_str(), std::ios::binary);
      if(!ofstr)
        return false;
    }
    ofstr.write(static_cast<const char*>(data), nbytes);
  }

  std::string filename;
  std::ofstream ofstr;
};

int main(int argc, char** argv) {
  Config appConfig;
  if(!parseConfig(argc, argv, appConfig))
    return -1;

  TestData video, audio;
  if( !video.read(TestData::VIDEO, appConfig.videoData.c_str(), appConfig.videoTimestamps.c_str())) {
    return -2;
  }
  if( !audio.read(TestData::AUDIO, appConfig.audioData.c_str(), appConfig.audioTimestamps.c_str())) {
    return -3;
  }

  BuildAvi::Config config;
  config.filename = appConfig.fileOut.c_str();
  config.video.mediatype = appConfig.mediatype.c_str();
  config.video.codecVideo = BuildAvi::VC_H264;
  config.audio.push_back( { BuildAvi::AC_PCM } );

  auto [aviBuilder, error] = BuildAvi::createAviBuilder(config);
  assert(aviBuilder || error);
  if(error) {
    std::cerr << "cannot init avi builder: " << error->text;
    return -4;
  }

  auto audio_it = audio.tss.begin();
  auto video_it = video.tss.begin();
  while (! (audio_it == audio.tss.end() && video_it == video.tss.end()) ) {
    TestData::PacketInfo next;

    if(audio_it == audio.tss.end()) {
      next = *video_it;
      video_it++;
    }
    else if(video_it == video.tss.end()) {
      next = *audio_it;
      audio_it++;
    }
    else if(audio_it->ts < video_it->ts) {
      next = *audio_it;
      audio_it++;      
    }
    else {
      next = *video_it;
      video_it++;            
    }

    if(next.type == TestData::AUDIO) {
      aviBuilder->addAudio(0, next.buffer, next.size);
    }
    else {
      aviBuilder->addVideo(next.buffer, next.size);
    }
  } // while
  aviBuilder->close();
  return 0;
}
