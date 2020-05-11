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
      // uint32_t width = 0;
      // uint32_t height = 0;

      // size_t frameRateNum = 0; // units ...
      // size_t frameRateDen = 1; // ...per secs
    };

    struct AudioChannel {
      AudioCodec codecAudeo = AC_PCM;
    };

    std::string filename;
    VideoChannel video;
    std::vector<AudioChannel> audio;
  };

  struct AviBuildError {
    enum Code {
      UNKNOWN,
      CANNOT_OPEN_FILE,
      CANNOT_WRITE_FILE,
      ALREADY_FINISHED,
      UNKNONW_AUDIO_CHANNEL,
      BAD_MEDIA_TYPE
    };
    using Ptr = std::shared_ptr<AviBuildError>;

    Code code;
    std::string text;
  };
 
  class AviBuilder {
  public:
    using Ptr = std::shared_ptr<AviBuilder>;

    virtual ~AviBuilder() {};

    virtual AviBuildError::Ptr addAudio(
      size_t channelIndex, 
      const void *data, 
      size_t nbytes
      ) = 0; 

    virtual AviBuildError::Ptr addVideo(
      const void *data, 
      size_t nbytes
      ) = 0; 
    virtual AviBuildError::Ptr close() = 0; 
  };

  std::tuple<AviBuilder::Ptr,AviBuildError::Ptr> createAviBuilder(const Config&);
} // namespace