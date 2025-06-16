pub const AWS_REGION: &str = "us-east-1";
pub const AWS_IAM_ROLE: &str = "arn:aws:iam::127214175560:role/CivicAlertAccessRole";

pub const MQTT_DEVICE_INFO_TOPIC: &str = "devices";
pub const MQTT_ALERTS_TOPIC: &str = "alerts";
pub const MQTT_AUDIO_EVIDENCE_TOPIC: &str = "evidence";

pub const AWS_MQTT_CLIENT_ID: &str = "civicalert-fusion-client";
pub const AWS_MQTT_ENDPOINT: &str = "a2a5h7878cau5i-ats.iot.us-east-2.amazonaws.com";
pub const AWS_MQTT_PORT: u16 = 8883;
pub const AWS_MQTT_CLEAN_SESSION: bool = true;
pub const AWS_MQTT_KEEP_ALIVE: u64 = 230;
pub const AWS_MQTT_CA_PATH: &str = "/opt/civicalert/AmazonRootCA3.pem";
pub const AWS_MQTT_CLIENT_CERT_PATH: &str = "/opt/civicalert/civicalert-device.pem.crt";
pub const AWS_MQTT_CLIENT_KEY_PATH: &str = "/opt/civicalert/civicalert-device-private.pem.key";

pub const AWS_DYNAMODB_USERS_TABLE: &str = "CivicAlertUsers";
pub const AWS_DYNAMODB_ALERTS_TABLE: &str = "CivicAlerts";
pub const AWS_DYNAMODB_DEVICES_TABLE: &str = "CivicAlertDevices";
pub const AWS_DYNAMODB_ERRORS_TABLE: &str = "CivicAlertErrors";

pub const AWS_S3_EVIDENCE_BUCKET: &str = "civicalert-evidence-clips";
