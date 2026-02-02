#ifndef __AWS_S3_HEADER_H__
#define __AWS_S3_HEADER_H__

#include "aws.h"

class AwsS3 final
{
public:

  // Constructor/destructor
  AwsS3(const char* __restrict s3BucketName);
  ~AwsS3(void);

  // AWS S3 management functionality
  bool isConnected(void);
  bool deleteBucket(void);
  bool storeFile(const char* __restrict s3FileName, const char* __restrict inputFilePath);
  bool storeFile(const char* __restrict s3FileName, const uint8_t* __restrict inputData, size_t inputDataLength);
  bool deleteFile(const char* __restrict s3FileName);
  bool deleteFiles(const std::vector<std::string>& s3FileNames);
  bool downloadFile(const char* __restrict s3FileName, const char* __restrict outputFilePath);
  bool downloadFile(const char* __restrict s3FileName, uint8_t* __restrict outputData, size_t outputDataLength);
  size_t getFileSize(const char* __restrict s3FileName);
  std::vector<std::string> listFiles(void);
  std::vector<std::pair<std::string, size_t>> listFilesAndSizes(void);
  std::vector<std::pair<std::string, std::chrono::system_clock::time_point>> listFilesAndModificationTimes(void);

private:

  // Private member variables
  std::shared_ptr<Aws::S3::S3Client> s3Client;
  const std::string bucketName;
};
#endif  // #ifndef __AWS_S3_HEADER_H__
