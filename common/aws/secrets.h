#ifndef __AWS_SECRETS_HEADER_H__
#define __AWS_SECRETS_HEADER_H__

#include <rapidjson/document.h>
#include "aws.h"

class AwsSecrets final
{
public:

  // Constructor/destructor
  AwsSecrets(void);
  ~AwsSecrets(void);

  // AWS Secrets Manager functionality
  std::string getParameter(const char* parameterID);
  rapidjson::Document getSecret(const char* secretID);
  std::string extractSecretValue(const rapidjson::Document& secretJson, const char* valueKey);

private:

  // Private member variables
  std::shared_ptr<Aws::SSM::SSMClient> ssmClient;
  std::shared_ptr<Aws::SecretsManager::SecretsManagerClient> secretsClient;
};
#endif  // #ifndef __AWS_SECRETS_HEADER_H__
