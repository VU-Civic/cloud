use crate::constants;
use aws_config::{BehaviorVersion, SdkConfig};
use aws_sdk_ssm;

pub async fn create() -> Result<SdkConfig, String> {
  // Load the default AWS SDK configuration
  let sdk_config = aws_config::load_defaults(BehaviorVersion::latest()).await;

  // Load all necessary parameters from the AWS Parameter Store
  let ssm = aws_sdk_ssm::Client::new(&sdk_config);
  if let Some(parameter) = ssm
    .get_parameter()
    .name(constants::MQTT_DEVICE_INFO_TOPIC.lock().unwrap().as_str())
    .with_decryption(false)
    .send()
    .await
    .map_err(|e| {
      format!(
        "Failed to get parameter {}: {e}",
        constants::MQTT_DEVICE_INFO_TOPIC.lock().unwrap()
      )
    })?
    .parameter
  {
    if let Some(value) = parameter.value {
      constants::MQTT_DEVICE_INFO_TOPIC
        .lock()
        .unwrap()
        .replace_range(.., value.as_str());
    }
  }

  if let Some(parameter) = ssm
    .get_parameter()
    .name(constants::MQTT_ALERTS_TOPIC.lock().unwrap().as_str())
    .with_decryption(false)
    .send()
    .await
    .map_err(|e| {
      format!(
        "Failed to get parameter {}: {e}",
        constants::MQTT_ALERTS_TOPIC.lock().unwrap()
      )
    })?
    .parameter
  {
    if let Some(value) = parameter.value {
      constants::MQTT_ALERTS_TOPIC
        .lock()
        .unwrap()
        .replace_range(.., value.as_str());
    }
  }

  if let Some(parameter) = ssm
    .get_parameter()
    .name(constants::MQTT_AUDIO_EVIDENCE_TOPIC.lock().unwrap().as_str())
    .with_decryption(false)
    .send()
    .await
    .map_err(|e| {
      format!(
        "Failed to get parameter {}: {e}",
        constants::MQTT_AUDIO_EVIDENCE_TOPIC.lock().unwrap()
      )
    })?
    .parameter
  {
    if let Some(value) = parameter.value {
      constants::MQTT_AUDIO_EVIDENCE_TOPIC
        .lock()
        .unwrap()
        .replace_range(.., value.as_str());
    }
  }

  if let Some(parameter) = ssm
    .get_parameter()
    .name(constants::MQTT_ENDPOINT.lock().unwrap().as_str())
    .with_decryption(false)
    .send()
    .await
    .map_err(|e| {
      format!(
        "Failed to get parameter {}: {e}",
        constants::MQTT_ENDPOINT.lock().unwrap()
      )
    })?
    .parameter
  {
    if let Some(value) = parameter.value {
      constants::MQTT_ENDPOINT
        .lock()
        .unwrap()
        .replace_range(.., value.as_str());
    }
  }

  if let Some(parameter) = ssm
    .get_parameter()
    .name(constants::MQTT_PORT.lock().unwrap().as_str())
    .with_decryption(false)
    .send()
    .await
    .map_err(|e| format!("Failed to get parameter {}: {e}", constants::MQTT_PORT.lock().unwrap()))?
    .parameter
  {
    if let Some(value) = parameter.value {
      constants::MQTT_PORT.lock().unwrap().replace_range(.., value.as_str());
    }
  }

  if let Some(parameter) = ssm
    .get_parameter()
    .name(constants::MQTT_CREDENTIALS_KEY.lock().unwrap().as_str())
    .with_decryption(false)
    .send()
    .await
    .map_err(|e| {
      format!(
        "Failed to get parameter {}: {e}",
        constants::MQTT_CREDENTIALS_KEY.lock().unwrap()
      )
    })?
    .parameter
  {
    if let Some(value) = parameter.value {
      constants::MQTT_CREDENTIALS_KEY
        .lock()
        .unwrap()
        .replace_range(.., value.as_str());
    }
  }

  if let Some(parameter) = ssm
    .get_parameter()
    .name(constants::MQTT_CA_KEY.lock().unwrap().as_str())
    .with_decryption(false)
    .send()
    .await
    .map_err(|e| {
      format!(
        "Failed to get parameter {}: {e}",
        constants::MQTT_CA_KEY.lock().unwrap()
      )
    })?
    .parameter
  {
    if let Some(value) = parameter.value {
      constants::MQTT_CA_KEY.lock().unwrap().replace_range(.., value.as_str());
    }
  }

  if let Some(parameter) = ssm
    .get_parameter()
    .name(constants::MQTT_CLIENT_CERT_KEY.lock().unwrap().as_str())
    .with_decryption(false)
    .send()
    .await
    .map_err(|e| {
      format!(
        "Failed to get parameter {}: {e}",
        constants::MQTT_CLIENT_CERT_KEY.lock().unwrap()
      )
    })?
    .parameter
  {
    if let Some(value) = parameter.value {
      constants::MQTT_CLIENT_CERT_KEY
        .lock()
        .unwrap()
        .replace_range(.., value.as_str());
    }
  }

  if let Some(parameter) = ssm
    .get_parameter()
    .name(constants::MQTT_CLIENT_KEY_KEY.lock().unwrap().as_str())
    .with_decryption(false)
    .send()
    .await
    .map_err(|e| {
      format!(
        "Failed to get parameter {}: {e}",
        constants::MQTT_CLIENT_KEY_KEY.lock().unwrap()
      )
    })?
    .parameter
  {
    if let Some(value) = parameter.value {
      constants::MQTT_CLIENT_KEY_KEY
        .lock()
        .unwrap()
        .replace_range(.., value.as_str());
    }
  }

  if let Some(parameter) = ssm
    .get_parameter()
    .name(constants::DYNAMODB_USERS_TABLE.lock().unwrap().as_str())
    .with_decryption(false)
    .send()
    .await
    .map_err(|e| {
      format!(
        "Failed to get parameter {}: {e}",
        constants::DYNAMODB_USERS_TABLE.lock().unwrap()
      )
    })?
    .parameter
  {
    if let Some(value) = parameter.value {
      constants::DYNAMODB_USERS_TABLE
        .lock()
        .unwrap()
        .replace_range(.., value.as_str());
    }
  }

  if let Some(parameter) = ssm
    .get_parameter()
    .name(constants::DYNAMODB_ALERTS_TABLE.lock().unwrap().as_str())
    .with_decryption(false)
    .send()
    .await
    .map_err(|e| {
      format!(
        "Failed to get parameter {}: {e}",
        constants::DYNAMODB_ALERTS_TABLE.lock().unwrap()
      )
    })?
    .parameter
  {
    if let Some(value) = parameter.value {
      constants::DYNAMODB_ALERTS_TABLE
        .lock()
        .unwrap()
        .replace_range(.., value.as_str());
    }
  }

  if let Some(parameter) = ssm
    .get_parameter()
    .name(constants::DYNAMODB_DEVICES_TABLE.lock().unwrap().as_str())
    .with_decryption(false)
    .send()
    .await
    .map_err(|e| {
      format!(
        "Failed to get parameter {}: {e}",
        constants::DYNAMODB_DEVICES_TABLE.lock().unwrap()
      )
    })?
    .parameter
  {
    if let Some(value) = parameter.value {
      constants::DYNAMODB_DEVICES_TABLE
        .lock()
        .unwrap()
        .replace_range(.., value.as_str());
    }
  }

  if let Some(parameter) = ssm
    .get_parameter()
    .name(constants::DYNAMODB_ERRORS_TABLE.lock().unwrap().as_str())
    .with_decryption(false)
    .send()
    .await
    .map_err(|e| {
      format!(
        "Failed to get parameter {}: {e}",
        constants::DYNAMODB_ERRORS_TABLE.lock().unwrap()
      )
    })?
    .parameter
  {
    if let Some(value) = parameter.value {
      constants::DYNAMODB_ERRORS_TABLE
        .lock()
        .unwrap()
        .replace_range(.., value.as_str());
    }
  }

  if let Some(parameter) = ssm
    .get_parameter()
    .name(constants::S3_EVIDENCE_BUCKET.lock().unwrap().as_str())
    .with_decryption(false)
    .send()
    .await
    .map_err(|e| {
      format!(
        "Failed to get parameter {}: {e}",
        constants::S3_EVIDENCE_BUCKET.lock().unwrap()
      )
    })?
    .parameter
  {
    if let Some(value) = parameter.value {
      constants::S3_EVIDENCE_BUCKET
        .lock()
        .unwrap()
        .replace_range(.., value.as_str());
    }
  }

  // Return the loaded configuration for other clients to use
  Ok(sdk_config)
}
