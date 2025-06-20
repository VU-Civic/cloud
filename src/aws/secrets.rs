use aws_config::SdkConfig;
use aws_sdk_secretsmanager::Client;
use serde_json::Value;

pub struct SecretManagerClient {
  client: Client,
}

impl SecretManagerClient {
  #[must_use]
  pub fn new(config: &SdkConfig) -> Self {
    SecretManagerClient {
      client: aws_sdk_secretsmanager::Client::new(config),
    }
  }

  pub async fn get_secret(&self, secret_name: &str) -> Result<Value, String> {
    let response = self
      .client
      .get_secret_value()
      .secret_id(secret_name)
      .send()
      .await
      .map_err(|e| format!("AWS SecretManager: GetSecret failed for key '{secret_name}' with error: {e}"))?;
    match response.secret_string() {
      Some(secret_string) => serde_json::from_str(secret_string)
        .map_err(|_| format!("AWS SecretManager: Failed to parse secret for key '{secret_name}'")),
      None => Err(format!(
        "AWS SecretManager: Secret string for key '{secret_name}' is empty or missing"
      )),
    }
  }

  pub fn extract_secret_value(&self, secret: &Value, key: &str) -> Result<String, String> {
    secret
      .get(key)
      .ok_or_else(|| format!("AWS SecretManager: Key '{key}' not found in secret"))?
      .as_str()
      .map(String::from)
      .ok_or_else(|| format!("AWS SecretManager: Secret for key '{key}' is not of the expected type"))
  }
}

#[cfg(test)]
mod tests {
  use crate::{aws, params};

  #[tokio::test(flavor = "multi_thread")]
  async fn test_secrets() {
    let secret_manager = aws::secrets::SecretManagerClient::new(&params::AWS_SDK_CONFIG);
    let secret = secret_manager
      .get_secret(params::MQTT_CREDENTIALS_KEY.as_str())
      .await
      .expect("Failed to get secret");
    assert!(
      secret.get(params::MQTT_CA_KEY.as_str()).is_some(),
      "CA certificate not found in secret"
    );
    assert!(
      secret.get(params::MQTT_CLIENT_CERT_KEY.as_str()).is_some(),
      "Client certificate not found in secret"
    );
    assert!(
      secret.get(params::MQTT_CLIENT_KEY_KEY.as_str()).is_some(),
      "Client key not found in secret"
    );
    assert!(
      secret_manager
        .extract_secret_value(&secret, params::MQTT_CA_KEY.as_str())
        .is_ok(),
      "Failed to extract CA certificate"
    );
    assert!(
      secret_manager
        .extract_secret_value(&secret, params::MQTT_CLIENT_CERT_KEY.as_str())
        .is_ok(),
      "Failed to extract client certificate"
    );
    assert!(
      secret_manager
        .extract_secret_value(&secret, params::MQTT_CLIENT_KEY_KEY.as_str())
        .is_ok(),
      "Failed to extract client key"
    );
  }
}
