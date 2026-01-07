#include <aws/core/auth/AWSCredentialsProvider.h>
#include "aws.h"

int AWS::referenceCount = 0;
std::mutex AWS::initializationMutex;
Aws::SDKOptions AWS::options;

void AWS::initialize(void)
{
  // Only allow one complete initialization
  std::lock_guard<std::mutex> lock(initializationMutex);
  if (referenceCount++ > 0) return;
  Aws::InitAPI(options);
}

void AWS::uninitialize(void)
{
  // Only allow one complete cleanup
  std::lock_guard<std::mutex> lock(initializationMutex);
  if (--referenceCount > 0) return;
  if (referenceCount < 0)
  {
    // Disallow unmatched uninitializations
    referenceCount = 0;
    return;
  }
  Aws::ShutdownAPI(options);
}

std::shared_ptr<Aws::S3::S3Client> AWS::getS3Client(void)
{
  // Ensure that the AWS library has been properly initialized
  if (referenceCount <= 0) return Aws::MakeShared<Aws::S3::S3Client>(nullptr);
  return Aws::MakeShared<Aws::S3::S3Client>("S3Client");
}

std::shared_ptr<Aws::SSM::SSMClient> AWS::getSSMClient(void)
{
  // Ensure that the AWS library has been properly initialized
  if (referenceCount <= 0) return Aws::MakeShared<Aws::SSM::SSMClient>(nullptr);
  return Aws::MakeShared<Aws::SSM::SSMClient>("SSMClient");
}

std::shared_ptr<Aws::SecretsManager::SecretsManagerClient> AWS::getSecretsManagerClient(void)
{
  // Ensure that the AWS library has been properly initialized
  if (referenceCount <= 0) return Aws::MakeShared<Aws::SecretsManager::SecretsManagerClient>(nullptr);
  return Aws::MakeShared<Aws::SecretsManager::SecretsManagerClient>("SecretsManagerClient");
}

std::shared_ptr<Aws::SQS::SQSClient> AWS::getSQSClient(void)
{
  // Ensure that the AWS library has been properly initialized
  if (referenceCount <= 0) return Aws::MakeShared<Aws::SQS::SQSClient>(nullptr);

  // Create and return the SQS client
  Aws::Client::ClientConfiguration config;
  config.requestTimeoutMs = 20000;
  return Aws::MakeShared<Aws::SQS::SQSClient>("SQSClient", config);
}
