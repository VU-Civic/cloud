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
  // Set up OpenSSL BIO objects for Base64 encoding
  auto* bioB64 = BIO_new(BIO_f_base64());
  auto* bioMem = BIO_push(bioB64, BIO_new_mem_buf(output, -1));
  BIO_set_flags(bioMem, BIO_FLAGS_BASE64_NO_NL);

  // Encode the input data into Base64 format
  BIO_write(bioMem, input, static_cast<int>(inputNumBytes));
  BIO_flush(bioMem);
  const auto encodedLength = BIO_ctrl_pending(bioMem);

  // Clean up OpenSSL BIO objects
  BIO_free_all(bioMem);
  return static_cast<uint32_t>(encodedLength);
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
  // Set up OpenSSL BIO objects for Base64 decoding
  auto* bioB64 = BIO_new(BIO_f_base64());
  auto* bioMem = BIO_push(bioB64, BIO_new_mem_buf(encodedData.data(), static_cast<int>(encodedData.size())));
  BIO_set_flags(bioMem, BIO_FLAGS_BASE64_NO_NL);

  // Decode the Base64 data into a byte vector
  std::vector<uint8_t> decodedData(encodedData.size());
  const auto decodedLength = BIO_read(bioMem, decodedData.data(), static_cast<int>(decodedData.size()));
  decodedData.resize(decodedLength > 0 ? static_cast<size_t>(decodedLength) : 0);

  // Clean up OpenSSL BIO objects
  BIO_free_all(bioMem);
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
