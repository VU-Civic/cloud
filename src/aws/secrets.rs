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
      .map_err(|e| format!("Failed to get secret value: {e}"))?;
    let secret_string = response.secret_string().ok_or("Secret string is empty or not found")?;
    serde_json::from_str(secret_string).map_err(|e| format!("Failed to parse secret JSON: {e}"))
  }

  pub fn extract_secret_value(&self, secret: &Value, key: &str) -> Result<String, String> {
    secret
      .get(key)
      .and_then(|v| v.as_str())
      .map(String::from)
      .ok_or_else(|| format!("Key '{key}' not found in secret"))
  }
}

#[cfg(test)]
mod tests {
  use crate::{aws, constants};

  #[tokio::test]
  async fn test_secrets() {
    let config = aws::config::create().await.expect("Failed to create AWS config");
    let secret_manager = aws::secrets::SecretManagerClient::new(&config);
    let secret = secret_manager
      .get_secret(constants::MQTT_CREDENTIALS_KEY.lock().unwrap().as_str())
      .await
      .expect("Failed to get secret");
    assert!(
      secret.get(constants::MQTT_CA_KEY.lock().unwrap().as_str()).is_some(),
      "CA certificate not found in secret"
    );
    assert!(
      secret
        .get(constants::MQTT_CLIENT_CERT_KEY.lock().unwrap().as_str())
        .is_some(),
      "Client certificate not found in secret"
    );
    assert!(
      secret
        .get(constants::MQTT_CLIENT_KEY_KEY.lock().unwrap().as_str())
        .is_some(),
      "Client key not found in secret"
    );
    assert!(
      secret_manager
        .extract_secret_value(&secret, constants::MQTT_CA_KEY.lock().unwrap().as_str())
        .is_ok(),
      "Failed to extract CA certificate"
    );
    assert!(
      secret_manager
        .extract_secret_value(&secret, constants::MQTT_CLIENT_CERT_KEY.lock().unwrap().as_str())
        .is_ok(),
      "Failed to extract client certificate"
    );
    assert!(
      secret_manager
        .extract_secret_value(&secret, constants::MQTT_CLIENT_KEY_KEY.lock().unwrap().as_str())
        .is_ok(),
      "Failed to extract client key"
    );
  }
}
