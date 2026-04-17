#include <aws/secretsmanager/model/GetSecretValueRequest.h>
#include <aws/ssm/model/GetParameterRequest.h>
#include "secrets.h"

AwsSecrets::AwsSecrets(void) : ssmClient(AWS::getSSMClient()), secretsClient(AWS::getSecretsManagerClient()) {}
AwsSecrets::~AwsSecrets(void) {}

std::string AwsSecrets::getParameter(const char* __restrict parameterID)
{
  // Ensure that the SSM client was properly initialized
  std::string parameterValue;
  if (!ssmClient.get() || !parameterID || parameterID[0] == '\0') return parameterValue;

  // Retrieve the specified parameter from AWS SSM
  Aws::SSM::Model::GetParameterRequest getParameterRequest;
  getParameterRequest.SetName(parameterID);
  const auto getParameterOutcome(ssmClient->GetParameter(getParameterRequest));
  if (getParameterOutcome.IsSuccess())
    parameterValue = getParameterOutcome.GetResult().GetParameter().GetValue();
  else
    logger.log(Logger::ERROR, "AWS SSM: GetParameter failed for key '%s' with error: %s\n", parameterID, getParameterOutcome.GetError().GetMessage().c_str());
  return parameterValue;
}

JsonObject AwsSecrets::getSecret(const char* __restrict secretID)
{
  // Ensure that the Secrets Manager client was properly initialized
  JsonObject secretJson;
  if (!secretsClient.get() || !secretID || secretID[0] == '\0') return secretJson;

  // Retrieve the specified secret from AWS Secrets Manager
  Aws::SecretsManager::Model::GetSecretValueRequest getSecretValueRequest;
  getSecretValueRequest.SetSecretId(secretID);
  const auto getSecretValueOutcome(secretsClient->GetSecretValue(getSecretValueRequest));
  if (getSecretValueOutcome.IsSuccess())
    secretJson = JsonParser::parseJsonString(getSecretValueOutcome.GetResult().GetSecretString().c_str());
  else
    logger.log(Logger::ERROR, "AWS SecretManager: GetSecret failed for key '%s' with error: %s\n", secretID, getSecretValueOutcome.GetError().GetMessage().c_str());

  // Ensure that the secret JSON was properly parsed
  if (secretJson.empty()) logger.log(Logger::ERROR, "AWS SecretManager: Failed to parse secret for key '%s'\n", secretID);

  // Return the parsed secret JSON
  return secretJson;
}

std::string AwsSecrets::extractSecretValue(const JsonObject& secretJson, const char* __restrict valueKey) const
{
  // Extract the specified value from the secret JSON
  std::string secretValue;
  if (secretJson.contains(valueKey) && !secretJson.at(valueKey).asString().empty())
    secretValue = secretJson.at(valueKey).asString();
  else
    logger.log(Logger::ERROR, "AWS SecretManager: Secret JSON does not contain string value for key '%s'\n", valueKey);
  return secretValue;
}
