#include <cassert>
#include <fstream>
#include <list>
#include <set>

#include "build_avi.h"
#include "avi_structs.h"

namespace BuildAvi {

  struct AviStructureConfig {
    size_t dwSuggestedBufferSize = 4096;
  };

  static const AviStructureConfig aviStructureConfig;

  // just structure to change dwSize fields 
  struct SizeFields {
    std::set<uint32_t *> sizeFields_;
    void add (uint32_t *pSizeField) { sizeFields_.insert(pSizeField); }
    void remove (uint32_t *pSizeField) { sizeFields_.erase(pSizeField); }
    void increase(size_t val) {
      for(auto pField : sizeFields_)
        *pField += val;
    }
  };

  class AviBuilderImpl : public AviBuilder {
  public:
    AviBuilderImpl (const Config& c);
    ~AviBuilderImpl();

    AviBuildError::Ptr init();

    AviBuildError::Ptr addAudio(size_t channelIndex, const void *, size_t );
    AviBuildError::Ptr addVideo(const void *, size_t );
    AviBuildError::Ptr close();
  private:
    Config config_;
    std::ofstream ofstr;

    enum Status {
      ST_READY,
      ST_MOVI,
      ST_FINISHED, 
    } status_ = ST_READY;

    using pos_t = std::ofstream::pos_type;
    pos_t pos = 0;

    Avi::LIST_HEADER riffList = { {'R','I','F','F'}, 4 ,{'A','V','I',' '}};
    const pos_t riffListPosition_ = 0;

    Avi::LIST_HEADER headerList = { {'L','I','S','T'}, 4 ,{'h','d','r','l'}};
    pos_t headerListPosition_ = 0;

    Avi::MainAVIHeader mainHeader_;
    pos_t mainHeaderPosition_ = 0;

    //video sgtream header
    Avi::LIST_HEADER streamVideoList = { {'L','I','S','T'}, 4 ,{'s','t','r','l'}};
    pos_t streamVideoListPosition_ = 0;

    Avi::AVIStreamHeader streamHeaderVideo_; // strh
    pos_t streamHeaderVideoPosition_ = 0;

    Avi::BITMAPINFOHEADER videoInfoHeader_;
    pos_t videoInfoHeaderPosition_ = 0;

    // audio stream header
    Avi::LIST_HEADER streamAudioList = { {'L','I','S','T'}, 4 ,{'s','t','r','l'}};
    pos_t streamAudioListPosition_ = 0;

    Avi::AVIStreamHeader streamHeaderAudio_;
    pos_t streamHeaderAudioPosition_ = 0;

    Avi::WAVEFORMATEX audioInfoHeader_;
    pos_t audioInfoHeaderPosition_;

    // data
    Avi::LIST_HEADER moviHeader_ = { {'L','I','S','T'}, 4 ,{'m','o','v','i'}};
    pos_t moviHeaderPosition_ = 0;

    // std::vector<uint8_t> audioCache_;
    // std::list<size_t> audioCachePositions_;
    std::vector<uint8_t> videoCache_;

    AviBuildError::Ptr writePhonyHeaders();
    AviBuildError::Ptr writeHeaders();
    AviBuildError::Ptr writeBlock(const Avi::CHUNK_HEADER&, const void* );
    AviBuildError::Ptr writeBlockSplitted(const Avi::CHUNK_HEADER&, const void* );
    AviBuildError::Ptr writePhony(size_t nbytes);

    SizeFields sizeFields_;
  };

  AviBuilderImpl::AviBuilderImpl (const Config& c)
    : config_(c) {
  }

  AviBuilderImpl::~AviBuilderImpl () {
  }

  AviBuildError::Ptr AviBuilderImpl::init() {
    ofstr.open(config_.filename.c_str(), std::ios::binary);
    if(!ofstr)
      return AviBuildError::Ptr( new AviBuildError {
        AviBuildError::CANNOT_OPEN_FILE, 
        std::string("cannot open file to write: ") + config_.filename
      });;    

    mainHeader_.dwMicroSecPerFrame = config_.video.frameRateNum ?
      10e5 * config_.video.frameRateDen/ config_.video.frameRateNum : 0; // frame display rate (or 0)
    mainHeader_.dwMaxBytesPerSec = 1024*1024*15; // TODO: calculate
    mainHeader_.dwPaddingGranularity = 0; // TODO: wtf is this? 
    mainHeader_.dwFlags = 272; // TODO ??????? //AVIF_WASCAPTUREFILE; // TODO: maybe AVIF_ISINTERLEAVED?
    mainHeader_.dwTotalFrames = 0; // will be calculated later
    mainHeader_.dwInitialFrames = 0;  // docs: "Ignore that"
    mainHeader_.dwStreams = 1; // TODO: tmp debug
    mainHeader_.dwSuggestedBufferSize = 0;// aviStructureConfig.dwSuggestedBufferSize;
    mainHeader_.dwWidth = config_.video.width;
    mainHeader_.dwHeight = config_.video.height;
//  mainHeader_.dwReserved[4];

    std::copy(FCC_TYPE_VIDEO, FCC_TYPE_VIDEO+4,  &streamHeaderVideo_.fccType[0]);
    std::copy(FCC_HANDLER_H264, FCC_HANDLER_H264+4, &streamHeaderVideo_.fccHandler[0]);
    streamHeaderVideo_.dwFlags = 0;
    streamHeaderVideo_.wPriority = 0;
    streamHeaderVideo_.wLanguage = 0;
    streamHeaderVideo_.dwInitialFrames = 0;
    streamHeaderVideo_.dwScale = config_.video.frameRateDen;
    streamHeaderVideo_.dwRate = config_.video.frameRateNum; /* dwRate / dwScale == samples/second */
    streamHeaderVideo_.dwStart = 0;
    streamHeaderVideo_.dwLength = 0; // will be calculated later
    streamHeaderVideo_.dwSuggestedBufferSize = aviStructureConfig.dwSuggestedBufferSize;
    streamHeaderVideo_.dwQuality = 0;    // default
    streamHeaderVideo_.dwSampleSize = 0;  // can vary, so set to zero (docs)
    streamHeaderVideo_.rcFrame.left = 0;  // TODO is it so?
    streamHeaderVideo_.rcFrame.top = 0;   // TODO is it so?
    streamHeaderVideo_.rcFrame.right = config_.video.width;    // TODO is it so?
    streamHeaderVideo_.rcFrame.bottom = config_.video.height;  // TODO is it so?

    videoInfoHeader_.biSize = sizeof(videoInfoHeader_); 
    videoInfoHeader_.biWidth = config_.video.width; 
    videoInfoHeader_.biHeight = config_.video.height; 
    videoInfoHeader_.biPlanes = 1; 
    videoInfoHeader_.biBitCount = 24; // TODO: is it so? 
    std::copy(BICOMPRESSION_H264, BICOMPRESSION_H264+4, reinterpret_cast<char *>(&videoInfoHeader_.biCompression));
    videoInfoHeader_.biSizeImage = 307200; // TODO: calculate it! 
    videoInfoHeader_.biXPelsPerMeter = 0; 
    videoInfoHeader_.biYPelsPerMeter = 0; 
    videoInfoHeader_.biClrUsed = 0; 
    videoInfoHeader_.biClrImportant = 0; 

    // Avi::AVIStreamHeader streamHeaderAudio_;
    // Avi::WAVEFORMATEX audioInfoHeader_;
    return AviBuildError::Ptr();
  }

  AviBuildError::Ptr AviBuilderImpl::addAudio(size_t channelIndex, const void *data, size_t nbytes) {
    return AviBuildError::Ptr();

    if(channelIndex > config_.audio.size() - 1)
      return AviBuildError::Ptr( new AviBuildError {
        AviBuildError::UNKNONW_AUDIO_CHANNEL, 
        std::string("no audio channel with channel index ") + std::to_string(channelIndex)
      });

    switch(status_) {
      case ST_READY:  {
        auto err = writePhonyHeaders();
        if (err)
          return err;
        status_ = ST_MOVI;
        return addAudio(channelIndex, data, nbytes);
      }
      case ST_MOVI: {
        Avi::CHUNK_HEADER chunk = {{'0','0','w','b'}, static_cast<uint32_t>(nbytes) }; // TODO why 00? dc or db?
        //streamHeaderAudio_.dwLength++;
        return writeBlockSplitted(chunk, data);          
        // audioCache_.insert(
        //   audioCache_.end(), 
        //   static_cast<const uint8_t*>(data), 
        //   static_cast<const uint8_t*>(data)+nbytes
        // );
        // size_t prevPosition = audioCachePositions_.empty() ? 0 : *audioCachePositions_.rbegin();
        // audioCachePositions_.push_back(prevPosition+nbytes);
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
      case ST_READY: {
        auto err = writePhonyHeaders();
        if(err)
          return err;
        status_ = ST_MOVI;
        return addVideo(data, nbytes);
      }
      case ST_MOVI: {        
        videoCache_.insert(
          videoCache_.end(), 
          static_cast<const uint8_t*>(data), 
          static_cast<const uint8_t*>(data)+nbytes
        ); 
        if(videoCache_.size() < aviStructureConfig.dwSuggestedBufferSize)
          return AviBuildError::Ptr();

        Avi::CHUNK_HEADER chunk = {{'0','0','d','b'}, static_cast<uint32_t>(videoCache_.size()) }; // TODO why 00? dc or db?
        auto err = writeBlockSplitted(chunk, videoCache_.data());

        size_t chunksCount = videoCache_.size() / aviStructureConfig.dwSuggestedBufferSize;
        mainHeader_.dwTotalFrames += chunksCount;
        streamHeaderVideo_.dwLength += chunksCount; 
        assert(chunksCount * aviStructureConfig.dwSuggestedBufferSize <= videoCache_.size());
        videoCache_.erase(videoCache_.begin(), videoCache_.begin() + chunksCount * aviStructureConfig.dwSuggestedBufferSize);
        return err;
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
    // if(audioCachePositions_.empty())
    //   return;
    // auto cachePosIt = audioCachePositions_.begin();
    // auto cachePosItNext = audioCachePositions_.begin();
    // cachePosItNext++;
    // for(; cachePosItNext != audioCachePositions_.end(); cachePosIt++, cachePosItNext++) {
    //   size_t curPos = *cachePosItNext;
    //   size_t nextPos = *cachePosItNext;
    // }

    writeHeaders();
    ofstr.close();
    status_ = ST_FINISHED;
    return AviBuildError::Ptr();
  } 

  AviBuildError::Ptr AviBuilderImpl::writePhonyHeaders() {
    // actually we rewrite headers later, when all params are known
    AviBuildError::Ptr  err;

    if(err = writePhony(sizeof(riffList)))
      return err;
    sizeFields_.add(&riffList.dwSize);

      headerListPosition_ = pos;
      if(err = writePhony(sizeof(headerList)))
        return err;
      sizeFields_.add(&headerList.dwSize);

        mainHeaderPosition_ = pos;
        mainHeaderPosition_ += sizeof(Avi::CHUNK_HEADER);
        if(err = writeBlock( {{'a','v','i','h'}, sizeof(mainHeader_) }, &mainHeader_))
          return err;

        streamVideoListPosition_ = pos;
        if(err = writePhony(sizeof(streamVideoList)))
          return err;
        sizeFields_.add(&streamVideoList.dwSize);

        streamHeaderVideoPosition_ = pos;
        streamHeaderVideoPosition_ += sizeof(Avi::CHUNK_HEADER);
        if(err = writeBlock( {{'s','t','r','h'}, sizeof(streamHeaderVideo_) }, &streamHeaderVideo_))
          return err;

        videoInfoHeaderPosition_ = pos;
        videoInfoHeaderPosition_ += sizeof(Avi::CHUNK_HEADER);
        if(err = writeBlock( {{'s','t','r','f'}, sizeof(videoInfoHeader_) }, &videoInfoHeader_))
          return err;
        sizeFields_.remove(&streamVideoList.dwSize);

        // streamAudioListPosition_ = pos;
        // if(err = writePhony(sizeof(streamAudioList)))
        //   return err;
        // sizeFields_.add(&streamAudioList.dwSize);

        // streamHeaderAudioPosition_ = pos;
        // streamHeaderAudioPosition_ += sizeof(Avi::CHUNK_HEADER);
        // if(err = writeBlock( {{'s','t','r','h'}, sizeof(streamHeaderAudio_) }, &streamHeaderAudio_))
        //   return err;

        // audioInfoHeaderPosition_ = pos;
        // audioInfoHeaderPosition_ += sizeof(Avi::CHUNK_HEADER);
        // if(err = writeBlock( {{'s','t','r','f'}, sizeof(audioInfoHeader_) }, &audioInfoHeader_))
        //   return err;
        // sizeFields_.remove(&streamAudioList.dwSize);

      sizeFields_.remove(&headerList.dwSize);

      moviHeaderPosition_ = pos;    
      if(err = writePhony(sizeof(moviHeader_)))
        return err;
      sizeFields_.add(&moviHeader_.dwSize);


    return AviBuildError::Ptr();
  }

  AviBuildError::Ptr AviBuilderImpl::writeHeaders() {
    sizeFields_.remove(&moviHeader_.dwSize);
    sizeFields_.remove(&riffList.dwSize);

    ofstr.seekp(riffListPosition_ );
    ofstr.write(reinterpret_cast<const std::ofstream::char_type*>(&riffList), sizeof(riffList));

    ofstr.seekp(headerListPosition_ );
    ofstr.write(reinterpret_cast<const std::ofstream::char_type*>(&headerList), sizeof(headerList));

    ofstr.seekp(mainHeaderPosition_ );
    ofstr.write(reinterpret_cast<const std::ofstream::char_type*>(&mainHeader_), sizeof(mainHeader_));

    ofstr.seekp(streamVideoListPosition_ );
    ofstr.write(reinterpret_cast<const std::ofstream::char_type*>(&streamVideoList), sizeof(streamVideoList));

    ofstr.seekp(streamHeaderVideoPosition_ );
    ofstr.write(reinterpret_cast<const std::ofstream::char_type*>(&streamHeaderVideo_), sizeof(streamHeaderVideo_));

    ofstr.seekp(videoInfoHeaderPosition_ );
    ofstr.write(reinterpret_cast<const std::ofstream::char_type*>(&videoInfoHeader_), sizeof(videoInfoHeader_));

    // ofstr.seekp(streamAudioListPosition_ );
    // ofstr.write(reinterpret_cast<const std::ofstream::char_type*>(&streamAudioList), sizeof(streamAudioList));

    // ofstr.seekp(streamHeaderAudioPosition_ );
    // ofstr.write(reinterpret_cast<const std::ofstream::char_type*>(&streamHeaderAudio_), sizeof(streamHeaderAudio_));

    // ofstr.seekp(audioInfoHeaderPosition_ );
    // ofstr.write(reinterpret_cast<const std::ofstream::char_type*>(&audioInfoHeader_), sizeof(audioInfoHeader_));

    ofstr.seekp(moviHeaderPosition_ );
    ofstr.write(reinterpret_cast<const std::ofstream::char_type*>(&moviHeader_), sizeof(moviHeader_));

    return AviBuildError::Ptr();
  }

  AviBuildError::Ptr AviBuilderImpl::writeBlockSplitted(const Avi::CHUNK_HEADER& c, const void* data){
    Avi::CHUNK_HEADER ch = c;
    size_t remain = ch.dwSize;
    const std::ofstream::char_type* position = reinterpret_cast<const std::ofstream::char_type*>(data);

    // TODO: while remaim > dwSuggestedBufferSize! remain - to cache
    while(remain > aviStructureConfig.dwSuggestedBufferSize) {
      ch.dwSize = aviStructureConfig.dwSuggestedBufferSize;
      auto err = writeBlock(ch, position);
      if(err)
        return err;

      remain -= ch.dwSize;
      position += ch.dwSize;
    }
    return AviBuildError::Ptr();
  }

  AviBuildError::Ptr AviBuilderImpl::writeBlock(const Avi::CHUNK_HEADER& ch, const void* data){
    ofstr.write(reinterpret_cast<const std::ofstream::char_type*>(&ch), sizeof(ch));
    ofstr.write(reinterpret_cast<const std::ofstream::char_type*>(data), ch.dwSize);
    if(ofstr.fail())
      return AviBuildError::Ptr( new AviBuildError {
        AviBuildError::CANNOT_WRITE_FILE, 
        std::string("cannot write to file") 
      });

    sizeFields_.increase(ch.dwSize + sizeof(ch));
    pos += ch.dwSize + sizeof(ch);
    return AviBuildError::Ptr();
  }


  AviBuildError::Ptr AviBuilderImpl::writePhony(size_t nbytes) {
    ofstr << std::string(nbytes, 0);
    sizeFields_.increase(nbytes);
    pos += nbytes;
    return AviBuildError::Ptr();
  }

  std::tuple<AviBuilder::Ptr, AviBuildError::Ptr> createAviBuilder(const Config& c) {
    using AviBuilderImplPtr = std::shared_ptr<AviBuilderImpl>;
    

    AviBuilderImplPtr retval(new AviBuilderImpl(c));
    auto err = retval->init();
    return err ? std::make_tuple(AviBuilder::Ptr(), err) : std::make_tuple(retval, AviBuildError::Ptr());
  }
}