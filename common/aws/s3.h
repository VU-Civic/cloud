#ifndef __AWS_S3_HEADER_H__
#define __AWS_S3_HEADER_H__

#include "aws.h"

class AwsS3 final
{
public:

  // Constructor/destructor
  AwsS3(const char* s3BucketName);
  ~AwsS3(void);

  // AWS S3 management functionality
  bool isConnected(void);
  bool deleteBucket(void);
  bool storeFile(const char* s3FileName, const char* inputFilePath);
  bool storeFile(const char* s3FileName, const uint8_t* inputData, size_t inputDataLength);
  bool deleteFile(const char* s3FileName);
  bool deleteFiles(const std::vector<std::string>& s3FileNames);
  bool downloadFile(const char* s3FileName, const char* outputFilePath);
  bool downloadFile(const char* s3FileName, uint8_t* outputData, size_t outputDataLength);
  size_t getFileSize(const char* s3FileName);
  std::vector<std::string> listFiles(void);
  std::vector<std::pair<std::string, size_t>> listFilesAndSizes(void);
  std::vector<std::pair<std::string, std::chrono::system_clock::time_point>> listFilesAndModificationTimes(void);

private:

  // Private member variables
  std::shared_ptr<Aws::S3::S3Client> s3Client;
  std::string bucketName;
};
#endif  // #ifndef __AWS_S3_HEADER_H__
