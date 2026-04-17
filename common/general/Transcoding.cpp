#include <openssl/bio.h>
#include <openssl/evp.h>
#include "Transcoding.h"

uint32_t Transcoding::hexEncode(uint8_t* __restrict output, const uint8_t* __restrict input, uint32_t inputNumBytes)
{
  // Format the input data bytes as hexadecimal output
  auto* output32 = (uint32_t*)output;
  for (uint32_t i = inputNumBytes / sizeof(uint16_t); i > 0; --i)
  {
    uint32_t x = (input[1] << 16U) | input[0];
    x = (((x & 0x00f000f0) >> 4) | ((x & 0x000f000f) << 8)) + 0x06060606;
    const uint32_t m = ((x & 0x10101010) >> 4) + 0x7f7f7f7f;
    *(output32++) = x + ((m & 0x2a2a2a2a) | (~m & 0x31313131));
    input += 2;
  }
  return inputNumBytes * 2;
}

uint32_t Transcoding::base64Encode(uint8_t* __restrict output, const uint8_t* __restrict input, uint32_t inputNumBytes)
{
  // The standard Base64 alphabet table
  static const char encodingTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  // Iterate through all input characters in groups of 3 bytes
  uint32_t inIdx = 0, outIdx = 0;
  const uint32_t threeBytePackets = inputNumBytes / 3;
  for (uint32_t i = 0; i < threeBytePackets; ++i, inIdx += 3, outIdx += 4)
  {
    const uint32_t a = input[inIdx], b = input[inIdx + 1], c = input[inIdx + 2];
    const uint32_t triple = (a << 16) + (b << 8) + c;
    output[outIdx] = encodingTable[(triple >> 18) & 0x3F];
    output[outIdx + 1] = encodingTable[(triple >> 12) & 0x3F];
    output[outIdx + 2] = encodingTable[(triple >> 6) & 0x3F];
    output[outIdx + 3] = encodingTable[triple & 0x3F];
  }

  // Pad and encode any remaining bytes
  const uint32_t remaining = inputNumBytes - inIdx;
  if (remaining == 1)
  {
    const uint32_t triple = (uint32_t)input[inIdx] << 16;
    output[outIdx] = encodingTable[(triple >> 18) & 0x3F];
    output[outIdx + 1] = encodingTable[(triple >> 12) & 0x3F];
    output[outIdx + 2] = '=';
    output[outIdx + 3] = '=';
    outIdx += 4;
  }
  else if (remaining == 2)
  {
    const uint32_t triple = ((uint32_t)input[inIdx] << 16) | ((uint32_t)input[inIdx + 1] << 8);
    output[outIdx] = encodingTable[(triple >> 18) & 0x3F];
    output[outIdx + 1] = encodingTable[(triple >> 12) & 0x3F];
    output[outIdx + 2] = encodingTable[(triple >> 6) & 0x3F];
    output[outIdx + 3] = '=';
    outIdx += 4;
  }
  return outIdx;
}

uint32_t Transcoding::base85Encode(uint8_t* __restrict output, const uint8_t* __restrict input, uint32_t inputNumBytes)
{
  // Iterate through all input bytes in groups of 4
  uint32_t inIdx = 0, outIdx = 0;
  const uint32_t fourBytePackets = inputNumBytes >> 2;
  for (uint32_t i = 0; i < fourBytePackets; ++i, inIdx += 4, outIdx += 5)
  {
    uint32_t val = ((uint32_t)input[inIdx] << 24) | ((uint32_t)input[inIdx + 1] << 16) | ((uint32_t)input[inIdx + 2] << 8) | ((uint32_t)input[inIdx + 3]);
    uint32_t q = (uint32_t)(((uint64_t)val * 3233857729ULL) >> 38);
    output[outIdx + 4] = '!' + (val - (q * 85));
    val = q;
    q = (uint32_t)(((uint64_t)val * 3233857729ULL) >> 38);
    output[outIdx + 3] = '!' + (val - (q * 85));
    val = q;
    q = (uint32_t)(((uint64_t)val * 3233857729ULL) >> 38);
    output[outIdx + 2] = '!' + (val - (q * 85));
    val = q;
    q = (uint32_t)(((uint64_t)val * 3233857729ULL) >> 38);
    output[outIdx + 1] = '!' + (val - (q * 85));
    output[outIdx] = '!' + q;
  }

  // Pad and encode any remaining bytes
  const uint32_t remaining = inputNumBytes - inIdx;
  if (remaining > 0)
  {
    // Pad with zeros to form a full 32-bit value
    uint32_t val = 0;
    for (uint32_t j = 0; j < remaining; ++j) val |= (uint32_t)input[inIdx + j] << (24 - (j * 8));
    char encoded[5];
    for (int j = 4; j >= 0; --j)
    {
      uint32_t q = (uint32_t)(((uint64_t)val * 3233857729ULL) >> 38);
      encoded[j] = '!' + (val - (q * 85));
      val = q;
    }
    for (uint32_t j = 0; j < remaining + 1; ++j) output[outIdx++] = encoded[j];
  }
  return outIdx;
}

uint32_t Transcoding::yEncEncode(uint8_t* __restrict output, const uint8_t* __restrict input, uint32_t inputNumBytes)
{
  uint32_t encodedLength = 0;
  for (uint32_t i = 0; i < inputNumBytes; ++i, ++encodedLength)
  {
    output[encodedLength] = input[i] + 42;
    if ((output[encodedLength] == '\0') || (output[encodedLength] == '\n') || (output[encodedLength] == '\r') || (output[encodedLength] == '='))
    {
      output[encodedLength + 1] = output[encodedLength] + 64;
      output[encodedLength++] = '=';
    }
  }
  return encodedLength;
}

std::vector<uint8_t> Transcoding::hexDecode(std::vector<uint8_t>&& encodedData)
{
  // Decode hexadecimal-encoded data
  std::vector<uint8_t> decodedData;
  decodedData.reserve(encodedData.size() / 2);
  for (size_t i = 0; i < encodedData.size(); i += 2)
  {
    const uint8_t highNibble = (encodedData[i] <= '9') ? (encodedData[i] - '0') : ((encodedData[i] | 0x20) - 'a' + 10);
    const uint8_t lowNibble = (encodedData[i + 1] <= '9') ? (encodedData[i + 1] - '0') : ((encodedData[i + 1] | 0x20) - 'a' + 10);
    decodedData.push_back((highNibble << 4) | lowNibble);
  }
  return decodedData;
}

std::vector<uint8_t> Transcoding::base64Decode(std::vector<uint8_t>&& encodedData)
{
  static const uint8_t decodingTable[256] = {0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0,
                                             0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  62, 0,  0,  0,  63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0,  0,  0, 0,
                                             0, 0, 0, 0, 1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0, 0,
                                             0, 0, 0, 0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};

  // Count trailing '=' padding to determine final byte count
  uint32_t padding = 0, inIdx = 0, outIdx = 0;
  const uint32_t fourCharPackets = encodedData.size() >> 2;
  if (encodedData.size() >= 1 && encodedData[encodedData.size() - 1] == '=') ++padding;
  if (encodedData.size() >= 2 && encodedData[encodedData.size() - 2] == '=') ++padding;

  // Decode all full groups except the last (which may have padding)
  std::vector<uint8_t> decodedData;
  decodedData.reserve((fourCharPackets * 3) - padding);
  const uint32_t fullPackets = (padding > 0) ? fourCharPackets - 1 : fourCharPackets;
  for (uint32_t i = 0; i < fullPackets; ++i, inIdx += 4, outIdx += 3)
  {
    const uint8_t d0 = decodingTable[encodedData[inIdx]];
    const uint8_t d1 = decodingTable[encodedData[inIdx + 1]];
    const uint8_t d2 = decodingTable[encodedData[inIdx + 2]];
    const uint8_t d3 = decodingTable[encodedData[inIdx + 3]];
    const uint32_t triple = ((uint32_t)d0 << 18) | ((uint32_t)d1 << 12) | ((uint32_t)d2 << 6) | ((uint32_t)d3);
    decodedData.push_back((uint8_t)(triple >> 16));
    decodedData.push_back((uint8_t)(triple >> 8));
    decodedData.push_back((uint8_t)(triple));
  }

  // Decode the final group if it contains padding
  if (padding > 0)
  {
    const uint8_t d0 = decodingTable[encodedData[inIdx]];
    const uint8_t d1 = decodingTable[encodedData[inIdx + 1]];
    const uint32_t triple = ((uint32_t)d0 << 18) | ((uint32_t)d1 << 12);
    if (padding == 2)
      decodedData.push_back((uint8_t)(triple >> 16));
    else
    {
      // 3 encoded chars -> 2 bytes
      const uint8_t d2 = decodingTable[encodedData[inIdx + 2]];
      const uint32_t full = triple | ((uint32_t)d2 << 6);
      decodedData.push_back((uint8_t)(full >> 16));
      decodedData.push_back((uint8_t)(full >> 8));
    }
  }
  return decodedData;
}

std::vector<uint8_t> Transcoding::base85Decode(std::vector<uint8_t>&& encodedData)
{
  // Decode Base85-encoded data
  std::vector<uint8_t> decodedData;
  const size_t fiveCharPackets = encodedData.size() / 5;
  const size_t remaining = encodedData.size() - (fiveCharPackets * 5);
  decodedData.reserve((fiveCharPackets * 4) + ((remaining > 1) ? (remaining - 1) : 0));
  for (size_t i = 0, inIdx = 0; i < fiveCharPackets; ++i, inIdx += 5)
  {
    // Decode 5 ASCII characters into their 0–84 digit values
    const uint32_t d0 = encodedData[inIdx] - '!';
    const uint32_t d1 = encodedData[inIdx + 1] - '!';
    const uint32_t d2 = encodedData[inIdx + 2] - '!';
    const uint32_t d3 = encodedData[inIdx + 3] - '!';
    const uint32_t d4 = encodedData[inIdx + 4] - '!';

    // Reconstruct the 32-bit value using Horner's method
    const uint32_t val = ((((d0 * 85 + d1) * 85 + d2) * 85) + d3) * 85 + d4;

    // Extract 4 bytes in big-endian order
    decodedData.push_back((uint8_t)(val >> 24));
    decodedData.push_back((uint8_t)(val >> 16));
    decodedData.push_back((uint8_t)(val >> 8));
    decodedData.push_back((uint8_t)(val));
  }

  // Handle remaining characters
  if (remaining > 1)
  {
    uint32_t digits[5] = {84, 84, 84, 84, 84};
    for (size_t j = 0; j < remaining; ++j) digits[j] = encodedData[(fiveCharPackets * 5) + j] - '!';
    const uint32_t val = ((((digits[0] * 85 + digits[1]) * 85 + digits[2]) * 85) + digits[3]) * 85 + digits[4];
    const size_t output_bytes = remaining - 1;
    if (output_bytes >= 1) decodedData.push_back((uint8_t)(val >> 24));
    if (output_bytes >= 2) decodedData.push_back((uint8_t)(val >> 16));
    if (output_bytes >= 3) decodedData.push_back((uint8_t)(val >> 8));
  }
  return decodedData;
}

std::vector<uint8_t> Transcoding::yEncDecode(std::vector<uint8_t>&& encodedData)
{
  // Decode yEnc-encoded data
  std::vector<uint8_t> decodedData;
  decodedData.reserve(encodedData.size());
  for (size_t i = 0; i < encodedData.size(); ++i)
  {
    if ((encodedData[i] == '=') && ((i + 1) < encodedData.size()))
      decodedData.push_back(encodedData[++i] - 64 - 42);
    else
      decodedData.push_back(encodedData[i] - 42);
  }
  return decodedData;
}
