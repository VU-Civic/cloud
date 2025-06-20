use aws_config::{BehaviorVersion, SdkConfig};
use log::error;
use std::sync::LazyLock;
use tokio::runtime::Handle;

// Lazy AWS SDK configuration and SSM client initialization
pub static AWS_SDK_CONFIG: LazyLock<SdkConfig> =
  LazyLock::new(|| Handle::current().block_on(async { aws_config::load_defaults(BehaviorVersion::latest()).await }));
pub static AWS_SSM: LazyLock<aws_sdk_ssm::Client> =
  LazyLock::new(|| Handle::current().block_on(async { aws_sdk_ssm::Client::new(&AWS_SDK_CONFIG) }));

// Helper function to lazily load a parameter from the AWS Systems Manager (SSM)
fn load_parameter(parameter_name: &str) -> String {
  match Handle::current().block_on(async move {
    AWS_SSM
      .get_parameter()
      .name(parameter_name)
      .with_decryption(true)
      .send()
      .await
      .map_err(|e| format!("Failed to get parameter '{parameter_name}' with error: {e}"))?
      .parameter
      .and_then(|p| p.value)
      .ok_or_else(|| format!("Parameter '{parameter_name}' not found"))
  }) {
    Ok(value) => value,
    Err(error) => {
      error!("AWS SSM: {error}");
      error
    }
  }
}

// AWS-specific constants retrieved as secrets or system parameters
pub static MQTT_DEVICE_INFO_TOPIC: LazyLock<String> = LazyLock::new(|| load_parameter("MqttDeviceInfoTopic"));
pub static MQTT_ALERTS_TOPIC: LazyLock<String> = LazyLock::new(|| load_parameter("MqttAlertsTopic"));
pub static MQTT_AUDIO_EVIDENCE_TOPIC: LazyLock<String> = LazyLock::new(|| load_parameter("MqttAudioEvidenceTopic"));
pub static MQTT_ENDPOINT: LazyLock<String> = LazyLock::new(|| load_parameter("MqttEndpoint"));
pub static MQTT_PORT: LazyLock<String> = LazyLock::new(|| load_parameter("MqttPort"));
pub static MQTT_CREDENTIALS_KEY: LazyLock<String> = LazyLock::new(|| load_parameter("MqttCredentialsKey"));
pub static MQTT_CA_KEY: LazyLock<String> = LazyLock::new(|| load_parameter("MqttCaKey"));
pub static MQTT_CLIENT_CERT_KEY: LazyLock<String> = LazyLock::new(|| load_parameter("MqttClientCertKey"));
pub static MQTT_CLIENT_KEY_KEY: LazyLock<String> = LazyLock::new(|| load_parameter("MqttClientKeyKey"));
pub static DYNAMODB_USERS_TABLE: LazyLock<String> = LazyLock::new(|| load_parameter("DynamoDbUsersTable"));
pub static DYNAMODB_ALERTS_TABLE: LazyLock<String> = LazyLock::new(|| load_parameter("DynamoDbAlertsTable"));
pub static DYNAMODB_DEVICES_TABLE: LazyLock<String> = LazyLock::new(|| load_parameter("DynamoDbDevicesTable"));
pub static DYNAMODB_ERRORS_TABLE: LazyLock<String> = LazyLock::new(|| load_parameter("DynamoDbErrorsTable"));
pub static S3_EVIDENCE_BUCKET: LazyLock<String> = LazyLock::new(|| load_parameter("S3EvidenceBucket"));

// MQTT client settings
pub const MQTT_CLIENT_ID: &str = "civicalert-cloud-client";
pub const MQTT_CLEAN_SESSION: bool = true;
pub const MQTT_KEEP_ALIVE: u64 = 230;

// Process-specific constants
pub const PROCESS_LOG_FILE_PATH: &str = "/var/log/civicalert-cloud.log";
pub const ALIVE_WATCHDOG_INTERVAL_SECONDS: u64 = 4 * 60;

// Fusion algorithm parameters
pub const FUSION_DATA_COLLECTION_SECONDS: u64 = 3;
pub const FUSION_ALGORITHM_MAX_DISTANCE: f64 = 100.0; // Maximum distance in meters for fusion
pub const FUSION_ALGORITHM_MAX_TIME_DIFF: u64 = 60; // Maximum time difference in seconds for fusion
pub const FUSION_ALGORITHM_MINIMUM_FUSED_EVENTS: usize = 3; // Minimum number of events to consider for fusion
