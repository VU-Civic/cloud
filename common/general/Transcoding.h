#ifndef __TRANSCODING_HEADER_H__
#define __TRANSCODING_HEADER_H__

#include <cstdint>
#include <vector>

class Transcoding final
{
public:

  // Encoding functions
  static uint32_t hexEncode(uint8_t* __restrict output, const uint8_t* __restrict input, uint32_t inputNumBytes);
  static uint32_t base64Encode(uint8_t* __restrict output, const uint8_t* __restrict input, uint32_t inputNumBytes);
  static uint32_t yEncEncode(uint8_t* __restrict output, const uint8_t* __restrict input, uint32_t inputNumBytes);

  // Decoding functions
  static std::vector<uint8_t> hexDecode(std::vector<uint8_t>&& encodedData);
  static std::vector<uint8_t> base64Decode(std::vector<uint8_t>&& encodedData);
  static std::vector<uint8_t> yEncDecode(std::vector<uint8_t>&& encodedData);
};

#endif  // #ifndef __TRANSCODING_HEADER_H__
