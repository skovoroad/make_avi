#pragma once 
#include<vector>
#include<memory>

namespace BuildAvi {

  enum VideoCodec {
    VC_H264,
  };

  enum AudioCodec {
    AC_PCM,
  };

  struct Config {
    struct VideoChannel {
      std::string mediatype;
      VideoCodec codecVideo = VC_H264;
    };

    struct AudioChannel {
      AudioCodec codecAudeo = AC_PCM;
    };

    std::string filename;
    VideoChannel video;
    std::vector<AudioChannel> audio;
  };

 class AviBuilder {
  public:
    using Ptr = std::shared_ptr<AviBuilder>;

    virtual ~AviBuilder() {};

    virtual void addAudio(
      size_t channelIndex, 
      const void *data, 
      size_t nbytes
      ) = 0; 

    virtual void addVideo(
      const void *data, 
      size_t nbytes
      ) = 0; 
    virtual void close() = 0; 
  };

  AviBuilder::Ptr createAviBuilder(const Config&);
} // namespace
