use std::sync::{LazyLock, Mutex};

// AWS-specific constants retrieved as secrets or parameter packs
pub static MQTT_DEVICE_INFO_TOPIC: LazyLock<Mutex<String>> =
  LazyLock::new(|| Mutex::new(String::from("MqttDeviceInfoTopic")));
pub static MQTT_ALERTS_TOPIC: LazyLock<Mutex<String>> = LazyLock::new(|| Mutex::new(String::from("MqttAlertsTopic")));
pub static MQTT_AUDIO_EVIDENCE_TOPIC: LazyLock<Mutex<String>> =
  LazyLock::new(|| Mutex::new(String::from("MqttAudioEvidenceTopic")));
pub static MQTT_ENDPOINT: LazyLock<Mutex<String>> = LazyLock::new(|| Mutex::new(String::from("MqttEndpoint")));
pub static MQTT_PORT: LazyLock<Mutex<String>> = LazyLock::new(|| Mutex::new(String::from("MqttPort")));
pub static MQTT_CREDENTIALS_KEY: LazyLock<Mutex<String>> =
  LazyLock::new(|| Mutex::new(String::from("MqttCredentialsKey")));
pub static MQTT_CA_KEY: LazyLock<Mutex<String>> = LazyLock::new(|| Mutex::new(String::from("MqttCaKey")));
pub static MQTT_CLIENT_CERT_KEY: LazyLock<Mutex<String>> =
  LazyLock::new(|| Mutex::new(String::from("MqttClientCertKey")));
pub static MQTT_CLIENT_KEY_KEY: LazyLock<Mutex<String>> =
  LazyLock::new(|| Mutex::new(String::from("MqttClientKeyKey")));
pub static DYNAMODB_USERS_TABLE: LazyLock<Mutex<String>> =
  LazyLock::new(|| Mutex::new(String::from("DynamoDbUsersTable")));
pub static DYNAMODB_ALERTS_TABLE: LazyLock<Mutex<String>> =
  LazyLock::new(|| Mutex::new(String::from("DynamoDbAlertsTable")));
pub static DYNAMODB_DEVICES_TABLE: LazyLock<Mutex<String>> =
  LazyLock::new(|| Mutex::new(String::from("DynamoDbDevicesTable")));
pub static DYNAMODB_ERRORS_TABLE: LazyLock<Mutex<String>> =
  LazyLock::new(|| Mutex::new(String::from("DynamoDbErrorsTable")));
pub static S3_EVIDENCE_BUCKET: LazyLock<Mutex<String>> = LazyLock::new(|| Mutex::new(String::from("S3EvidenceBucket")));

// MQTT client settings
pub const MQTT_CLIENT_ID: &str = "civicalert-cloud-client";
pub const MQTT_CLEAN_SESSION: bool = true;
pub const MQTT_KEEP_ALIVE: u64 = 230;
