#include <fstream>
#include <aws/s3/model/DeleteBucketRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/DeleteObjectsRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/ListObjectsV2Request.h>
#include <aws/s3/model/PutObjectRequest.h>
#include "s3.h"

AwsS3::AwsS3(const char* __restrict s3BucketName) : s3Client(AWS::getS3Client()), bucketName(s3BucketName) {}
AwsS3::~AwsS3(void) {}

bool AwsS3::isConnected(void)
{
  // Ensure that the AWS library was properly initialized
  if (!s3Client.get()) return false;

  // Ensure that the specified S3 bucket exists
  const auto listBucketsOutcome(s3Client->ListBuckets());
  if (listBucketsOutcome.IsSuccess())
  {
    for (const auto& bucket : listBucketsOutcome.GetResult().GetBuckets())
      if (bucket.GetName().compare(bucketName) == 0) return true;
  }
  return false;
}

bool AwsS3::deleteBucket(void)
{
  // Delete the current S3 bucket
  Aws::S3::Model::DeleteBucketRequest deleteBucketRequest;
  deleteBucketRequest.SetBucket(bucketName.c_str());
  return s3Client->DeleteBucket(deleteBucketRequest).IsSuccess();
}

bool AwsS3::storeFile(const char* __restrict s3FileName, const char* __restrict inputFilePath)
{
  // Store the specified S3 file
  Aws::S3::Model::PutObjectRequest putObjectRequest;
  putObjectRequest.SetBucket(bucketName.c_str());
  putObjectRequest.SetKey(s3FileName);
  putObjectRequest.SetBody(Aws::MakeShared<Aws::FStream>("S3FileUploader", inputFilePath, std::ios_base::in | std::ios_base::binary));
  const auto putObjectOutcome = s3Client->PutObject(putObjectRequest);
  if (!putObjectOutcome.IsSuccess())
    logger.log(Logger::ERROR, "AWS S3: PutObject failed on Bucket '%s' for file '%s' with error: %s\n", bucketName.c_str(), s3FileName,
               putObjectOutcome.GetError().GetMessage().c_str());
  return putObjectOutcome.IsSuccess();
}

bool AwsS3::storeFile(const char* __restrict s3FileName, const uint8_t* __restrict inputData, size_t inputDataLength)
{
  // Store the specified S3 file
  Aws::S3::Model::PutObjectRequest putObjectRequest;
  putObjectRequest.SetBucket(bucketName.c_str());
  putObjectRequest.SetKey(s3FileName);
  std::shared_ptr<Aws::StringStream> binaryStream(
      Aws::MakeShared<Aws::StringStream>("S3BinaryStreamUploader", std::stringstream::in | std::stringstream::out | std::stringstream::binary));
  binaryStream->write(reinterpret_cast<const char*>(inputData), static_cast<std::streamsize>(inputDataLength));
  putObjectRequest.SetBody(binaryStream);
  const auto putObjectOutcome = s3Client->PutObject(putObjectRequest);
  if (!putObjectOutcome.IsSuccess())
    logger.log(Logger::ERROR, "AWS S3: PutObject failed on Bucket '%s' for file '%s' with error: %s\n", bucketName.c_str(), s3FileName,
               putObjectOutcome.GetError().GetMessage().c_str());
  return putObjectOutcome.IsSuccess();
}

bool AwsS3::deleteFile(const char* __restrict s3FileName)
{
  // Delete the specified S3 file
  Aws::S3::Model::DeleteObjectRequest deleteObjectRequest;
  deleteObjectRequest.SetBucket(bucketName.c_str());
  deleteObjectRequest.SetKey(s3FileName);
  const auto deleteObjectOutcome = s3Client->DeleteObject(deleteObjectRequest);
  if (!deleteObjectOutcome.IsSuccess())
    logger.log(Logger::ERROR, "AWS S3: DeleteObject failed on Bucket '%s' for file '%s' with error: %s\n", bucketName.c_str(), s3FileName,
               deleteObjectOutcome.GetError().GetMessage().c_str());
  return deleteObjectOutcome.IsSuccess();
}

bool AwsS3::deleteFiles(const std::vector<std::string>& s3FileNames)
{
  // Delete the specified S3 files
  Aws::S3::Model::Delete objectsToDelete;
  for (const std::string& fileName : s3FileNames) objectsToDelete.AddObjects(Aws::S3::Model::ObjectIdentifier().WithKey(fileName.c_str()));
  Aws::S3::Model::DeleteObjectsRequest deleteObjectsRequest;
  deleteObjectsRequest.SetBucket(bucketName.c_str());
  deleteObjectsRequest.SetDelete(objectsToDelete);
  const auto deleteObjectsOutcome = s3Client->DeleteObjects(deleteObjectsRequest);
  if (!deleteObjectsOutcome.IsSuccess())
    logger.log(Logger::ERROR, "AWS S3: DeleteObjects failed on Bucket '%s' with error: %s\n", bucketName.c_str(), deleteObjectsOutcome.GetError().GetMessage().c_str());
  return deleteObjectsOutcome.IsSuccess();
}

bool AwsS3::downloadFile(const char* __restrict s3FileName, const char* __restrict outputFilePath)
{
  // Download the specified S3 file
  Aws::S3::Model::GetObjectRequest getObjectRequest;
  getObjectRequest.SetBucket(bucketName.c_str());
  getObjectRequest.SetKey(s3FileName);
  getObjectRequest.SetResponseStreamFactory(
      [&] { return Aws::New<Aws::FStream>("S3FileDownloader", outputFilePath, std::ios_base::in | std::ios_base::out | std::ios_base::binary | std::ios_base::trunc); });
  const auto getObjectOutcome = s3Client->GetObject(getObjectRequest);
  if (!getObjectOutcome.IsSuccess())
    logger.log(Logger::ERROR, "AWS S3: GetObject failed on Bucket '%s' for file '%s' with error: %s\n", bucketName.c_str(), s3FileName,
               getObjectOutcome.GetError().GetMessage().c_str());
  return getObjectOutcome.IsSuccess();
}

bool AwsS3::downloadFile(const char* __restrict s3FileName, uint8_t* __restrict outputData, size_t outputDataLength)
{
  // Download the specified S3 file
  Aws::S3::Model::GetObjectRequest getObjectRequest;
  getObjectRequest.SetBucket(bucketName.c_str());
  getObjectRequest.SetKey(s3FileName);
  getObjectRequest.SetResponseStreamFactory(
      [] { return Aws::New<Aws::StringStream>("S3FileDownloader", std::stringstream::in | std::stringstream::out | std::stringstream::binary); });
  auto getObjectOutcome = s3Client->GetObject(getObjectRequest);
  if (!getObjectOutcome.IsSuccess())
  {
    logger.log(Logger::ERROR, "AWS S3: GetObject failed on Bucket '%s' for file '%s' with error: %s\n", bucketName.c_str(), s3FileName,
               getObjectOutcome.GetError().GetMessage().c_str());
    return false;
  }
  getObjectOutcome.GetResult().GetBody().read(reinterpret_cast<char*>(outputData), static_cast<std::streamsize>(outputDataLength));
  return true;
}

size_t AwsS3::getFileSize(const char* __restrict s3FileName)
{
  // Retrieve the size of the specified S3 file
  std::vector<std::string> bucketFiles;
  Aws::S3::Model::ListObjectsV2Request listObjectsRequest;
  listObjectsRequest.SetBucket(bucketName.c_str());
  listObjectsRequest.SetPrefix(s3FileName);
  const auto listObjectsOutcome(s3Client->ListObjectsV2(listObjectsRequest));
  if (listObjectsOutcome.IsSuccess())
    for (const auto& object : listObjectsOutcome.GetResult().GetContents()) return static_cast<size_t>(object.GetSize());
  else
    logger.log(Logger::ERROR, "AWS S3: ListObjects failed on Bucket '%s' for file '%s' with error: %s\n", bucketName.c_str(), s3FileName,
               listObjectsOutcome.GetError().GetMessage().c_str());
  return 0;
}

std::vector<std::string> AwsS3::listFiles(void)
{
  // List all files in the S3 bucket
  bool hasMoreFiles = true;
  std::vector<std::string> bucketFiles;
  Aws::S3::Model::ListObjectsV2Request listObjectsRequest;
  listObjectsRequest.SetBucket(bucketName.c_str());
  auto listObjectsOutcome(s3Client->ListObjectsV2(listObjectsRequest));
  while (hasMoreFiles && listObjectsOutcome.IsSuccess())
  {
    // Add these files to the list
    for (const auto& object : listObjectsOutcome.GetResult().GetContents()) bucketFiles.emplace_back(object.GetKey().c_str());

    // See if more files exist
    if ((hasMoreFiles = listObjectsOutcome.GetResult().GetIsTruncated()))
    {
      listObjectsRequest.SetContinuationToken(listObjectsOutcome.GetResult().GetNextContinuationToken().c_str());
      listObjectsOutcome = s3Client->ListObjectsV2(listObjectsRequest);
    }
  }
  return bucketFiles;
}

std::vector<std::pair<std::string, size_t>> AwsS3::listFilesAndSizes(void)
{
  // List all files in the S3 bucket along with their sizes
  bool hasMoreFiles = true;
  std::vector<std::pair<std::string, size_t>> bucketFilesAndSizes;
  Aws::S3::Model::ListObjectsV2Request listObjectsRequest;
  listObjectsRequest.SetBucket(bucketName.c_str());
  auto listObjectsOutcome(s3Client->ListObjectsV2(listObjectsRequest));
  while (hasMoreFiles && listObjectsOutcome.IsSuccess())
  {
    // Add these files to the list
    for (const auto& object : listObjectsOutcome.GetResult().GetContents()) bucketFilesAndSizes.emplace_back(object.GetKey().c_str(), static_cast<size_t>(object.GetSize()));

    // See if more files exist
    if ((hasMoreFiles = listObjectsOutcome.GetResult().GetIsTruncated()))
    {
      listObjectsRequest.SetContinuationToken(listObjectsOutcome.GetResult().GetNextContinuationToken().c_str());
      listObjectsOutcome = s3Client->ListObjectsV2(listObjectsRequest);
    }
  }
  return bucketFilesAndSizes;
}

std::vector<std::pair<std::string, std::chrono::system_clock::time_point>> AwsS3::listFilesAndModificationTimes(void)
{
  // List all files in the S3 bucket along with their modification times
  bool hasMoreFiles = true;
  std::vector<std::pair<std::string, std::chrono::system_clock::time_point>> bucketFilesAndTimes;
  Aws::S3::Model::ListObjectsV2Request listObjectsRequest;
  listObjectsRequest.SetBucket(bucketName.c_str());
  auto listObjectsOutcome(s3Client->ListObjectsV2(listObjectsRequest));
  while (hasMoreFiles && listObjectsOutcome.IsSuccess())
  {
    // Add these files to the list
    for (const auto& object : listObjectsOutcome.GetResult().GetContents())
      bucketFilesAndTimes.emplace_back(object.GetKey().c_str(), object.GetLastModified().UnderlyingTimestamp());

    // See if more files exist
    if ((hasMoreFiles = listObjectsOutcome.GetResult().GetIsTruncated()))
    {
      listObjectsRequest.SetContinuationToken(listObjectsOutcome.GetResult().GetNextContinuationToken().c_str());
      listObjectsOutcome = s3Client->ListObjectsV2(listObjectsRequest);
    }
  }
  return bucketFilesAndTimes;
}
