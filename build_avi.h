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
      VideoCodec codecVideo = VC_H264;
      size_t frameRate[2] =  {0, 1}; //
    };

    struct AudioChannel {
      AudioCodec codecAudeo = AC_PCM;
    };

    VideoChannel video;
    std::vector<AudioChannel> audio;
  };

  // this implemented by client to receive result
  class AviFileReceiver {
    virtual bool onAvi (const void *, size_t ) = 0;
  };

  struct AviBuildError {
    enum Code {
      UNKNOWN,
      ALREADY_FINISHED,
      UNKNONW_AUDIO_CHANNEL
    };
    using Ptr = std::shared_ptr<AviBuildError>;

    Code code;
    std::string text;
  };
 
  class AviBuilder {
  public:
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

  AviBuilder * createAviBuilder(const Config&, AviFileReceiver*);
} // namespace