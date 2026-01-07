#ifndef __AWS_HEADER_H__
#define __AWS_HEADER_H__

#include <memory>
#include <mutex>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/secretsmanager/SecretsManagerClient.h>
#include <aws/sqs/SQSClient.h>
#include <aws/ssm/SSMClient.h>
#include "Logger.h"

// Make global logger accessible to all AWS modules
extern Logger logger;

class AWS final
{
public:

  // Initialization/cleanup
  static void initialize(void);
  static void uninitialize(void);

  // AWS factory functions
  static std::shared_ptr<Aws::S3::S3Client> getS3Client(void);
  static std::shared_ptr<Aws::SSM::SSMClient> getSSMClient(void);
  static std::shared_ptr<Aws::SecretsManager::SecretsManagerClient> getSecretsManagerClient(void);
  static std::shared_ptr<Aws::SQS::SQSClient> getSQSClient(void);

private:

  // Member variables
  static int referenceCount;
  static std::mutex initializationMutex;
  static Aws::SDKOptions options;
};

#endif  // #ifndef __AWS_HEADER_H__
