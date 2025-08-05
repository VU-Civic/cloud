pub mod db;
pub mod mqtt;
pub mod s3;
pub mod secrets;
pub mod sns;
pub mod sqs;

use crate::{AlertData, params};
use db::DynamoDbClient;
use std::collections::HashMap;
use std::sync::Arc;
use tokio::sync::Mutex;
use tracing::error;

pub async fn update_device_details_from_alert(db: &Arc<Mutex<DynamoDbClient>>, alert: &AlertData) {
  // Create a database update item for the alert
  let update_item = HashMap::from([
    (
      String::from("device_id"),
      aws_sdk_dynamodb::types::AttributeValue::S(alert.device_id.clone()),
    ),
    (
      String::from("cell_signal_power"),
      aws_sdk_dynamodb::types::AttributeValue::N(alert.cell_signal_power.to_string()),
    ),
    (
      String::from("cell_signal_quality"),
      aws_sdk_dynamodb::types::AttributeValue::N(alert.cell_signal_quality.to_string()),
    ),
    (
      String::from("orientation"),
      aws_sdk_dynamodb::types::AttributeValue::M(HashMap::from([
        (
          String::from("qw"),
          aws_sdk_dynamodb::types::AttributeValue::N(alert.sensor_quaternions[0].to_string()),
        ),
        (
          String::from("qx"),
          aws_sdk_dynamodb::types::AttributeValue::N(alert.sensor_quaternions[1].to_string()),
        ),
        (
          String::from("qy"),
          aws_sdk_dynamodb::types::AttributeValue::N(alert.sensor_quaternions[2].to_string()),
        ),
        (
          String::from("qz"),
          aws_sdk_dynamodb::types::AttributeValue::N(alert.sensor_quaternions[3].to_string()),
        ),
      ])),
    ),
    (
      String::from("location"),
      aws_sdk_dynamodb::types::AttributeValue::M(HashMap::from([
        (
          String::from("lat"),
          aws_sdk_dynamodb::types::AttributeValue::N(alert.sensor_llh[0].to_string()),
        ),
        (
          String::from("lon"),
          aws_sdk_dynamodb::types::AttributeValue::N(alert.sensor_llh[1].to_string()),
        ),
        (
          String::from("height"),
          aws_sdk_dynamodb::types::AttributeValue::N(alert.sensor_llh[2].to_string()),
        ),
      ])),
    ),
    (
      String::from("last_updated"),
      aws_sdk_dynamodb::types::AttributeValue::N(alert.timestamp.to_string()),
    ),
  ]);

  // Update the relevant database listing
  if let Err(err) = db
    .lock()
    .await
    .put_item(params::DYNAMODB_DEVICES_TABLE.as_str(), update_item)
    .await
  {
    error!("{err}");
  }
}
