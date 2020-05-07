#include <cassert>
#include <cstdint>
#include <list>
#include "build_avi.h"

namespace Avi {
  
  struct MainAVIHeader
  {
    uint32_t dwMicroSecPerFrame; // frame display rate (or 0)
    uint32_t dwMaxBytesPerSec; // max. transfer rate
    uint32_t dwPaddingGranularity; // pad to multiples of this size;
    uint32_t dwFlags; // the ever-present flags
    uint32_t dwTotalFrames; // # frames in file
    uint32_t dwInitialFrames;
    uint32_t dwStreams;
    uint32_t dwSuggestedBufferSize;
    uint32_t dwWidth;
    uint32_t dwHeight;
    uint32_t dwReserved[4];
  };

  struct AVIStreamHeader {
    char fccType[4];
    char fccHandler[4];
    uint32_t dwFlags;
    uint16_t wPriority;
    uint16_t wLanguage;
    uint32_t dwInitialFrames;
    uint32_t dwScale;
    uint32_t dwRate; /* dwRate / dwScale == samples/second */
    uint32_t dwStart;
    uint32_t dwLength; /* In units above... */
    uint32_t dwSuggestedBufferSize;
    uint32_t dwQuality;
    uint32_t dwSampleSize;
    struct {
         short int left;
         short int top;
         short int right;
         short int bottom;
     } rcFrame;
  };

  struct BITMAPINFOHEADER {
    uint32_t  biSize; 
    int32_t   biWidth; 
    int32_t   biHeight; 
    uint16_t   biPlanes; 
    uint16_t   biBitCount; 
    uint32_t  biCompression; 
    uint32_t  biSizeImage; 
    int32_t   biXPelsPerMeter; 
    int32_t   biYPelsPerMeter; 
    uint32_t  biClrUsed; 
    uint32_t  biClrImportant; 
  };

 struct WAVEFORMATEX {
    uint16_t  wFormatTag;
    uint16_t  nChannels;
    uint32_t nSamplesPerSec;
    uint32_t nAvgBytesPerSec;
    uint16_t  nBlockAlign;
    uint16_t  wBitsPerSample;
    uint16_t  cbSize;
  };

  struct CHUNK_HEADER {
    char dwFourCC[4];
    uint32_t dwSize;
    //uint8_t data[dwSize] // contains headers or video/audio data
  };

 struct LIST_HEADER {
    uint32_t dwList;
    uint32_t dwSize;
    char dwFourCC[4];
    //std::vector<char> data;
  //  uint8_t data[dwSize-4] // contains Lists and Chunks
  };  

  struct AVIINDEXENTRY{
    uint32_t ckid;
    uint32_t dwFlags;
    uint32_t dwChunkOffset;
    uint32_t dwChunkLength;
  };
}

namespace BuildAvi {

  class AviBuilderImpl : public AviBuilder {
  public:
    AviBuilderImpl (const Config& c, AviFileReceiver *h);
    ~AviBuilderImpl();

    AviBuildError::Ptr addAudio(size_t channelIndex, const void *, size_t );
    AviBuildError::Ptr addVideo(const void *, size_t );
    AviBuildError::Ptr close();
  private:
    Config config_;
    AviFileReceiver * handler_;
    enum Status {
      ST_READY,
      ST_MOVI,
      ST_FINISHED, 
    } status_;

    using pos_t = size_t;
    pos_t pos = 0;
    Avi::MainAVIHeader mainHeader_;
    pos_t mainHeaderPosition_ = 0;

    std::vector<uint8_t> audioCache_;
    std::list<size_t> audioCachePositions_;

    AviBuildError::Ptr writeHeaders();
    AviBuildError::Ptr writeBlock(const Avi::CHUNK_HEADER&, const void* );
  };

  AviBuilderImpl::AviBuilderImpl (const Config& c, AviFileReceiver *h)
    : config_(c), handler_(h) {
  }

  AviBuilderImpl::~AviBuilderImpl () {
  }

  AviBuildError::Ptr AviBuilderImpl::addAudio(size_t channelIndex, const void *data, size_t nbytes) {
    if(channelIndex > config_.audio.size() - 1)
      return AviBuildError::Ptr( new AviBuildError {
        AviBuildError::UNKNONW_AUDIO_CHANNEL, 
        std::string("no audio channel with channel index ") + std::to_string(channelIndex)
      });

    switch(status_) {
      case ST_READY: 
        writeHeaders();
        status_ = ST_MOVI;
        return addAudio(channelIndex, data, nbytes);
      case ST_MOVI: {
        Avi::CHUNK_HEADER chunk;
        // here prepare chunk;
        return writeBlock(chunk, data);        
      }
      case ST_FINISHED: 
        return AviBuildError::Ptr( new AviBuildError {
          AviBuildError::ALREADY_FINISHED, 
          std::string("file already closed, check da application!")
        });
      default:
        assert(false);
    }
    return AviBuildError::Ptr();
  } 

  AviBuildError::Ptr AviBuilderImpl::addVideo(const void *data, size_t nbytes) {
    switch(status_) {
      case ST_READY: 
        writeHeaders();
        status_ = ST_MOVI;
        return addVideo(data, nbytes);
      case ST_MOVI: {
        audioCache_.insert(
          audioCache_.end(), 
          static_cast<const uint8_t*>(data), 
          static_cast<const uint8_t*>(data)+nbytes
        );
        size_t prevPosition = audioCachePositions_.empty() ? 0 : *audioCachePositions_.rbegin();
        audioCachePositions_.push_back(prevPosition+nbytes);
      }
      case ST_FINISHED: 
        return AviBuildError::Ptr( new AviBuildError {
          AviBuildError::ALREADY_FINISHED, 
          std::string("file already closed, check da application!") 
        });
      default:
        assert(false);
    }
    return AviBuildError::Ptr();
  } 

  AviBuildError::Ptr AviBuilderImpl::close() {
    return AviBuildError::Ptr();
  } 

  AviBuildError::Ptr AviBuilderImpl::writeHeaders() {
    //mainHeader_;
    mainHeaderPosition_ = pos;    
    return AviBuildError::Ptr();

  }

  AviBuildError::Ptr AviBuilderImpl::writeBlock(const Avi::CHUNK_HEADER&, const void* ){
    return AviBuildError::Ptr();
  }


  AviBuilder * createAviBuilder(const Config& c, AviFileReceiver* h) {
    return new AviBuilderImpl(c, h);
  }
}