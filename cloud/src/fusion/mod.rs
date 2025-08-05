mod algorithm;
mod collection;

use civicalert_cloud_common::{AlertData, aws::db::DynamoDbClient, params};
use std::sync::Arc;
use tokio::{
  sync::{Mutex, broadcast::Receiver},
  task,
};
use tracing::warn;

pub async fn begin_fusion(
  receiver: Receiver<AlertData>,
  _db_client: Arc<Mutex<DynamoDbClient>>,
  master_alert: AlertData,
) {
  // Start the data collection process and wait for it to complete
  let mut data = collection::collect_data(receiver, master_alert).await;

  // Use the collected data to carry out the fusion algorithm
  if data.len() >= params::FUSION_ALGORITHM_MIN_NUM_EVENTS {
    if let Ok(_result) = task::spawn_blocking(move || {
      algorithm::localize_event(data.as_mut_slice());
    })
    .await
    {
      // TODO: Send the fusion result somewhere, e.g., back to the MQTT broker or SQS or to a database
    }
  } else {
    warn!("Too few events for localization, skipping..."); // TODO: Only print this if no constituent events were used in another fusion
  }
}
