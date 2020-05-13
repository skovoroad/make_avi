#include <cassert>
#include <fstream>
#include <sstream>
#include <list>
#include <set>
#include <string>

#include "build_avi.h"
#include "build_avi_exception.hpp"
#include "avi_structs.h"

namespace BuildAvi {

  struct VideoMediaType {
    uint32_t width = 0;
    uint32_t height = 0;

    uint32_t frameRateDen = 0;
    uint32_t frameRateNum = 0;

    void notify(const std::string& key, const std::string& value ) {
      if(key == "width") {
        width = std::stoi(value);
      }
      if(key == "height") {
        height = std::stoi(value);
      }
      if(key == "framerate") {
        size_t pos   = value.find('/');
        if(pos == std::string::npos)
          throw AviException("invalid mediatype");
        frameRateNum = std::stoi(value.substr(0, pos));
        frameRateDen = std::stoi(value.substr(pos + 1 ));
      }
    }
    void notify(const std::string& value ) {
    }
  };

  template<typename MediaType>
  void parseMediaType(const std::string& str, MediaType &mt) {
    try {
      std::istringstream iss(str);
      std::string token;
      while (std::getline(iss, token, ',')) {
        size_t pos   = token.find('=');
        if(pos == std::string::npos) 
          mt.notify(token);
        else 
          mt.notify(token.substr(0, pos), token.substr(pos + 1));
      } // while
    } // try
    catch(const std::exception & ex) {
      ex;
      throw AviException("invalid mediatype");
    }
  }


  struct AviStructureConfig {
    uint32_t dwSuggestedBufferSize = 4096;
  };

  static const AviStructureConfig aviStructureConfig;

  // just structure to change dwSize fields 
  struct SizeFields {
    std::set<uint32_t *> sizeFields_;
    void add (uint32_t *pSizeField) { sizeFields_.insert(pSizeField); }
    void remove (uint32_t *pSizeField) { sizeFields_.erase(pSizeField); }
    void increase(size_t val) {
      for(auto pField : sizeFields_)
        *pField += static_cast<uint32_t>(val);
    }
  };

  class AviBuilderImpl : public AviBuilder {
  public:
    AviBuilderImpl (const Config& c);
    ~AviBuilderImpl();

    void addAudio(size_t channelIndex, const void *, size_t );
    void addVideo(const void *, size_t );
    void close();
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
    pos_t audioInfoHeaderPosition_= 0;

    Avi::LIST_HEADER odmlList = { {'L','I','S','T'}, 4,{'o','d','m','l'} };
    pos_t odmlListPosition_ = 0;

    Avi::ODMLExtendedAVIHeader odmlHeader_;
    pos_t odmlHeaderPosition_ = 0;

    // data
    Avi::LIST_HEADER moviHeader_ = { {'L','I','S','T'}, 4 ,{'m','o','v','i'}};
    pos_t moviHeaderPosition_ = 0;

    std::vector<uint8_t> videoCache_;
    std::vector<uint8_t> audioCache_;

    void writePhonyHeaders();
    void writeHeaders();
    void writeBlock(const Avi::CHUNK_HEADER&, const void* , bool saveIndex);
    void writeBlockSplitted(const Avi::CHUNK_HEADER&, const void* );
    void writePhony(size_t nbytes);

    std::vector<uint8_t> indexes_;

    SizeFields sizeFields_;
    VideoMediaType videoMediaType_;
  };

  AviBuilderImpl::~AviBuilderImpl () {
  }

  AviBuilderImpl::AviBuilderImpl (const Config& c) 
    : config_(c) {
    ofstr.exceptions ( std::ifstream::failbit | std::ifstream::badbit );
    ofstr.open(config_.filename.c_str(), std::ios::binary);

    parseMediaType(config_.video.mediatype, videoMediaType_);
    // mainHeader_.dwMicroSecPerFrame // calculate it at finish
    mainHeader_.dwMaxBytesPerSec = 1024*1024*15; // TODO: calculate
    mainHeader_.dwPaddingGranularity = 0; 
    mainHeader_.dwFlags = AVIF_HASINDEX | AVIF_ISINTERLEAVED; 
    mainHeader_.dwTotalFrames = 0; // will be calculated later
    mainHeader_.dwInitialFrames = 0;  
    mainHeader_.dwStreams = 2; // / TODO: get it from config
    mainHeader_.dwSuggestedBufferSize = 0;
    mainHeader_.dwWidth = videoMediaType_.width;
    mainHeader_.dwHeight = videoMediaType_.height;
//  mainHeader_.dwReserved[4]; // ignore

    std::copy(FCC_TYPE_VIDEO, FCC_TYPE_VIDEO+4,  &streamHeaderVideo_.fccType[0]);
    std::copy(FCC_HANDLER_H264, FCC_HANDLER_H264+4, &streamHeaderVideo_.fccHandler[0]);
    streamHeaderVideo_.dwFlags = 0;
    streamHeaderVideo_.wPriority = 0;
    streamHeaderVideo_.wLanguage = 0;
    streamHeaderVideo_.dwInitialFrames = 0;
    // streamHeaderVideo_.dwScale ; // calculate it at finish, estimating duration by audio size
    // streamHeaderVideo_.dwRate; // calculate it at finish, estimating duration by audio size
    streamHeaderVideo_.dwStart = 0;
    streamHeaderVideo_.dwLength = 0; // will be calculated later
    streamHeaderVideo_.dwSuggestedBufferSize = aviStructureConfig.dwSuggestedBufferSize;
    streamHeaderVideo_.dwQuality = 0;   
    streamHeaderVideo_.dwSampleSize = 0;  
    streamHeaderVideo_.rcFrame.left = 0;  
    streamHeaderVideo_.rcFrame.top = 0;   
    streamHeaderVideo_.rcFrame.right = videoMediaType_.width;
    streamHeaderVideo_.rcFrame.bottom = videoMediaType_.height;

    videoInfoHeader_.biSize = sizeof(videoInfoHeader_); 
    videoInfoHeader_.biWidth = videoMediaType_.width; 
    videoInfoHeader_.biHeight = videoMediaType_.height; 
    videoInfoHeader_.biPlanes = 1; 
    videoInfoHeader_.biBitCount = 24; 
    std::copy(BICOMPRESSION_H264, BICOMPRESSION_H264+4, reinterpret_cast<char *>(&videoInfoHeader_.biCompression));
    videoInfoHeader_.biSizeImage = videoMediaType_.width * videoMediaType_.height; 
    videoInfoHeader_.biXPelsPerMeter = 0; 
    videoInfoHeader_.biYPelsPerMeter = 0; 
    videoInfoHeader_.biClrUsed = 0; 
    videoInfoHeader_.biClrImportant = 0; 


    std::copy(FCC_TYPE_AUDIO, FCC_TYPE_AUDIO+4,  &streamHeaderAudio_.fccType[0]);
    std::copy(&FCC_HANDLER_PCM[0], &FCC_HANDLER_PCM[0]+4, &streamHeaderAudio_.fccHandler[0]);    
    streamHeaderAudio_.dwFlags = 0;
    streamHeaderAudio_.wPriority = 0;
    streamHeaderAudio_.wLanguage = 0;
    streamHeaderAudio_.dwInitialFrames = 0;
    streamHeaderAudio_.dwScale = 1;
    streamHeaderAudio_.dwRate = 8000; // TODO: get it from mediatype
    streamHeaderAudio_.dwStart = 0;
    streamHeaderAudio_.dwLength = 0; // will be calculated later
    streamHeaderAudio_.dwSuggestedBufferSize = aviStructureConfig.dwSuggestedBufferSize;
    streamHeaderAudio_.dwQuality = 0;    
    streamHeaderAudio_.dwSampleSize = 2; 
    streamHeaderAudio_.rcFrame.left = 0; 
    streamHeaderAudio_.rcFrame.top = 0;  
    streamHeaderAudio_.rcFrame.right = 0;
    streamHeaderAudio_.rcFrame.bottom = 0;

    audioInfoHeader_.wFormatTag = 1;
    audioInfoHeader_.nChannels = 1;
    audioInfoHeader_.nSamplesPerSec = 8000; // TODO: get it from mediatype
    audioInfoHeader_.nAvgBytesPerSec = 16000; // TODO: get it from mediatype
    audioInfoHeader_.nBlockAlign = 2;
    audioInfoHeader_.wBitsPerSample = 16;
    audioInfoHeader_.cbSize = 0;
  }

  void AviBuilderImpl::addAudio(size_t channelIndex, const void *data, size_t nbytes) {
    if(channelIndex > config_.audio.size() - 1)
      throw AviException("invalid audio channel index");

    switch(status_) {
      case ST_READY:  
        writePhonyHeaders();
        status_ = ST_MOVI;
        addAudio(channelIndex, data, nbytes);
	break;
      case ST_MOVI: {
        audioCache_.insert(
          audioCache_.end(), 
          static_cast<const uint8_t*>(data), 
          static_cast<const uint8_t*>(data)+nbytes
        ); 
        if(audioCache_.size() < aviStructureConfig.dwSuggestedBufferSize)
          break;
        Avi::CHUNK_HEADER chunk = {{'0','1','w','b'}, static_cast<uint32_t>(audioCache_.size()) }; // TODO why 00? dc or db?
        writeBlockSplitted(chunk, audioCache_.data());
        size_t chunksCount = audioCache_.size() / aviStructureConfig.dwSuggestedBufferSize;
        streamHeaderAudio_.dwLength += static_cast<uint32_t>(
          chunksCount * aviStructureConfig.dwSuggestedBufferSize / streamHeaderAudio_.dwSampleSize); // we know it`s integer
        assert(chunksCount * aviStructureConfig.dwSuggestedBufferSize <= audioCache_.size());
        audioCache_.erase(audioCache_.begin(), audioCache_.begin() + chunksCount * aviStructureConfig.dwSuggestedBufferSize);
        break;
      }
      case ST_FINISHED:
        throw AviException("avi file already closed");
      default:
        assert(false);
    }
  } 

  void AviBuilderImpl::addVideo(const void *data, size_t nbytes) {
    switch(status_) {
      case ST_READY:
        writePhonyHeaders();
        status_ = ST_MOVI;
        addVideo(data, nbytes);
	break;
      case ST_MOVI: { 
        mainHeader_.dwTotalFrames ++;
        streamHeaderVideo_.dwLength ++; 
        Avi::CHUNK_HEADER chunk = {{'0','0','d','b'}, static_cast<uint32_t>(nbytes) }; // TODO why 00? dc or db?
        writeBlock(chunk, data, true);
	break;
      }
      case ST_FINISHED: 
        throw AviException("avi file already closed");
      default:
        assert(false);
    }
  } 

  void AviBuilderImpl::close() {
    sizeFields_.remove(&moviHeader_.dwSize);

    Avi::CHUNK_HEADER ch = {{'i','d','x','1'}, static_cast<uint32_t>(indexes_.size()) };
    writeBlock(ch, indexes_.data(), false);
    
    writeHeaders();
    ofstr.close();
    status_ = ST_FINISHED;
  } 

  void AviBuilderImpl::writePhonyHeaders() {
    // actually we rewrite headers later, when all params are known
    writePhony(sizeof(riffList));
    sizeFields_.add(&riffList.dwSize);

      headerListPosition_ = pos;
      writePhony(sizeof(headerList));
      sizeFields_.add(&headerList.dwSize);

        mainHeaderPosition_ = pos;
        mainHeaderPosition_ += sizeof(Avi::CHUNK_HEADER);
        writeBlock( {{'a','v','i','h'}, sizeof(mainHeader_) }, &mainHeader_, false);

        streamVideoListPosition_ = pos;
        writePhony(sizeof(streamVideoList));
        sizeFields_.add(&streamVideoList.dwSize);

        streamHeaderVideoPosition_ = pos;
        streamHeaderVideoPosition_ += sizeof(Avi::CHUNK_HEADER);
        writeBlock( {{'s','t','r','h'}, sizeof(streamHeaderVideo_) }, &streamHeaderVideo_, false);

        videoInfoHeaderPosition_ = pos;
        videoInfoHeaderPosition_ += sizeof(Avi::CHUNK_HEADER);
        writeBlock( {{'s','t','r','f'}, sizeof(videoInfoHeader_) }, &videoInfoHeader_, false);
        sizeFields_.remove(&streamVideoList.dwSize);

        streamAudioListPosition_ = pos;
        writePhony(sizeof(streamAudioList));
        sizeFields_.add(&streamAudioList.dwSize);

        streamHeaderAudioPosition_ = pos;
        streamHeaderAudioPosition_ += sizeof(Avi::CHUNK_HEADER);
        writeBlock( {{'s','t','r','h'}, sizeof(streamHeaderAudio_) }, &streamHeaderAudio_, false);

        audioInfoHeaderPosition_ = pos;
        audioInfoHeaderPosition_ += sizeof(Avi::CHUNK_HEADER);
        writeBlock( {{'s','t','r','f'}, sizeof(audioInfoHeader_) }, &audioInfoHeader_, false);
        sizeFields_.remove(&streamAudioList.dwSize);

        odmlListPosition_ = pos;
        writePhony(sizeof(odmlList));
        sizeFields_.add(&odmlList.dwSize);

        odmlHeaderPosition_ = pos;
        odmlHeaderPosition_ += sizeof(Avi::CHUNK_HEADER);
        writeBlock( {{'o','d','m','h'}, sizeof(odmlHeader_) }, &odmlHeader_, false);
        sizeFields_.remove(&odmlList.dwSize);

      sizeFields_.remove(&headerList.dwSize);

      moviHeaderPosition_ = pos;    
      writePhony(sizeof(moviHeader_));
      sizeFields_.add(&moviHeader_.dwSize);
  }

  void AviBuilderImpl::writeHeaders() {
    if(streamHeaderAudio_.dwLength) { // calculate from audio
      double duration = static_cast<double>(streamHeaderAudio_.dwLength) / static_cast<double>(streamHeaderAudio_.dwRate);
      double framerate = duration / static_cast<double>(streamHeaderVideo_.dwLength);
      streamHeaderVideo_.dwRate = streamHeaderVideo_.dwLength;
      streamHeaderVideo_.dwScale = static_cast<uint32_t>(duration); 
      mainHeader_.dwMicroSecPerFrame = static_cast<uint32_t>(10e5 * streamHeaderVideo_.dwScale/ streamHeaderVideo_.dwRate); 
    }
    else { // get from mediatype
      if(!videoMediaType_.frameRateNum)
        throw AviException("Cannot get framerate from mediatype neither from audio lenght (no audio?)"); 

      mainHeader_.dwMicroSecPerFrame = static_cast<uint32_t>(videoMediaType_.frameRateNum ?
        10e5 * videoMediaType_.frameRateDen/ videoMediaType_.frameRateNum : 0); 
      streamHeaderVideo_.dwScale = videoMediaType_.frameRateDen;
      streamHeaderVideo_.dwRate = videoMediaType_.frameRateNum; 
    }

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

    ofstr.seekp(streamAudioListPosition_ );
    ofstr.write(reinterpret_cast<const std::ofstream::char_type*>(&streamAudioList), sizeof(streamAudioList));

    ofstr.seekp(streamHeaderAudioPosition_ );
    ofstr.write(reinterpret_cast<const std::ofstream::char_type*>(&streamHeaderAudio_), sizeof(streamHeaderAudio_));

    ofstr.seekp(audioInfoHeaderPosition_ );
    ofstr.write(reinterpret_cast<const std::ofstream::char_type*>(&audioInfoHeader_), sizeof(audioInfoHeader_));

    ofstr.seekp(odmlListPosition_ );
    ofstr.write(reinterpret_cast<const std::ofstream::char_type*>(&odmlList), sizeof(odmlList));

    ofstr.seekp(odmlHeaderPosition_ );
    ofstr.write(reinterpret_cast<const std::ofstream::char_type*>(&odmlHeader_), sizeof(&odmlHeader_));

    ofstr.seekp(moviHeaderPosition_ );
    ofstr.write(reinterpret_cast<const std::ofstream::char_type*>(&moviHeader_), sizeof(moviHeader_));
  }

  void AviBuilderImpl::writeBlockSplitted(const Avi::CHUNK_HEADER& c, const void* data){
    Avi::CHUNK_HEADER ch = c;
    size_t remain = ch.dwSize;
    const std::ofstream::char_type* position = reinterpret_cast<const std::ofstream::char_type*>(data);

    while(remain >= aviStructureConfig.dwSuggestedBufferSize) {
      ch.dwSize = aviStructureConfig.dwSuggestedBufferSize;
      writeBlock(ch, position, true);

      remain -= ch.dwSize;
      position += ch.dwSize;
    }
  }

  void AviBuilderImpl::writeBlock(const Avi::CHUNK_HEADER& ch, const void* data, bool saveIndex){
    ofstr.write(reinterpret_cast<const std::ofstream::char_type*>(&ch), sizeof(ch));
    ofstr.write(reinterpret_cast<const std::ofstream::char_type*>(data), ch.dwSize);
    if(ch.dwSize % 2) {
      ofstr << (char)0;
    }

    if(saveIndex) {
      Avi::AVIINDEXENTRY index;
      index.ckid = *reinterpret_cast<const uint32_t *>(&ch.dwFourCC[0]);
      index.dwFlags  = 0;
      //index.dwFlags = AVIIF_KEYFRAME;
      index.dwChunkOffset = static_cast<uint32_t>(pos);
      index.dwChunkLength = ch.dwSize;
      indexes_.insert(indexes_.end(), 
        reinterpret_cast<const uint8_t *>(&index), 
        reinterpret_cast<const uint8_t *>(&index) + sizeof(index));
    }

    sizeFields_.increase(ch.dwSize + sizeof(ch));
    pos += ch.dwSize + sizeof(ch);
    if(ch.dwSize % 2) {
      sizeFields_.increase(1);
      pos += 1;
    }
  }

  void AviBuilderImpl::writePhony(size_t nbytes) {
    ofstr << std::string(nbytes, 0);
    sizeFields_.increase(nbytes);
    pos += nbytes;
  }

  AviBuilder::Ptr createAviBuilder(const Config& c) {
    return AviBuilder::Ptr(new AviBuilderImpl(c));
  }
}
