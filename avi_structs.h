#pragma once

#include <cstdint>

namespace Avi {

#pragma pack(push, 1)  
  struct MainAVIHeader
  {
    uint32_t dwMicroSecPerFrame = 0; // frame display rate (or 0)
    uint32_t dwMaxBytesPerSec = 0; // max. transfer rate
    uint32_t dwPaddingGranularity = 0; // pad to multiples of this size;
    uint32_t dwFlags = 0; // the ever-present flags
    uint32_t dwTotalFrames = 0; // # frames in file
    uint32_t dwInitialFrames = 0;
    uint32_t dwStreams = 0;
    uint32_t dwSuggestedBufferSize = 4096;
    uint32_t dwWidth = 0;
    uint32_t dwHeight = 0;
    uint32_t dwReserved[4] = {0, 0, 0, 0};
  };

  struct AVIStreamHeader {
    char fccType[4] = {'s', 't', 'r', 'h'};
    char fccHandler[4] = {0, 0, 0, 0};
    uint32_t dwFlags = 0;
    uint16_t wPriority = 0;
    uint16_t wLanguage = 0;
    uint32_t dwInitialFrames = 0;
    uint32_t dwScale = 0;
    uint32_t dwRate = 0; /* dwRate / dwScale == samples/second */
    uint32_t dwStart = 0;
    uint32_t dwLength = 0; /* In units above... */
    uint32_t dwSuggestedBufferSize = 0;
    uint32_t dwQuality = 0;
    uint32_t dwSampleSize = 0;
    struct {
         short int left = 0;
         short int top = 0;
         short int right = 0;
         short int bottom = 0;
     } rcFrame;
  };

  struct BITMAPINFOHEADER {
    uint32_t  biSize = 0; 
    int32_t   biWidth = 0; 
    int32_t   biHeight = 0; 
    uint16_t   biPlanes = 0; 
    uint16_t   biBitCount = 0; 
    uint32_t  biCompression = 0; 
    uint32_t  biSizeImage = 0; 
    int32_t   biXPelsPerMeter = 0; 
    int32_t   biYPelsPerMeter = 0; 
    uint32_t  biClrUsed = 0; 
    uint32_t  biClrImportant = 0; 
  };

 struct WAVEFORMATEX {
    uint16_t  wFormatTag = 0;
    uint16_t  nChannels = 0;
    uint32_t nSamplesPerSec = 0;
    uint32_t nAvgBytesPerSec = 0;
    uint16_t  nBlockAlign = 0;
    uint16_t  wBitsPerSample = 0;
    uint16_t  cbSize = 0;
  };

  struct CHUNK_HEADER {
    char dwFourCC[4] = {0, 0, 0, 0};
    uint32_t dwSize = 0;
    //uint8_t data[dwSize] // contains headers or video/audio data
  };

 struct LIST_HEADER {
    char dwList[4];
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
#pragma pack(pop)  
  
}
