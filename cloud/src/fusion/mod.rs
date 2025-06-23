mod algorithm;
mod collection;

use civicalert_cloud_common::{AlertData, aws::db::DynamoDbClient};
use std::sync::{Arc, Mutex};
use tokio::{sync::broadcast::Receiver, task};

pub async fn begin_fusion(
  receiver: Receiver<AlertData>,
  _db_client: Arc<Mutex<DynamoDbClient>>,
  master_alert: AlertData,
) {
  // Start the data collection process and wait for it to complete
  let data = collection::collect_data(receiver, master_alert).await;

  // Use the collected data to carry out the fusion algorithm
  if let Ok(_result) = task::spawn_blocking(move || {
    algorithm::localize_event(data);
  })
  .await
  {
    // TODO: Send the fusion result somewhere, e.g., back to the MQTT broker or SQS or to a database
  }
}
