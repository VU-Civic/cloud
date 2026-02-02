#ifndef __AWS_SECRETS_HEADER_H__
#define __AWS_SECRETS_HEADER_H__

#include <rapidjson/document.h>
#include "aws.h"

class AwsSecrets final
{
public:

  // Constructor/destructor
  AwsSecrets(void);
  AwsSecrets(const AwsSecrets&) = delete;
  AwsSecrets& operator=(const AwsSecrets&) = delete;
  ~AwsSecrets(void);

  // AWS Secrets Manager functionality
  std::string getParameter(const char* __restrict parameterID);
  rapidjson::Document getSecret(const char* __restrict secretID);
  std::string extractSecretValue(const rapidjson::Document& secretJson, const char* __restrict valueKey) const;

private:

  // Private member variables
  std::shared_ptr<Aws::SSM::SSMClient> ssmClient;
  std::shared_ptr<Aws::SecretsManager::SecretsManagerClient> secretsClient;
};
#endif  // #ifndef __AWS_SECRETS_HEADER_H__
