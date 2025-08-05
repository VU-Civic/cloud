mod algorithm;
mod collection;

use civicalert_cloud_common::{EventInfo, aws::db::DynamoDbClient};
use std::sync::Arc;
use tokio::sync::{Mutex, broadcast::Receiver};
use tracing::{info, warn};

pub async fn begin_fusion(
  receiver: Receiver<EventInfo>,
  _db_client: Arc<Mutex<DynamoDbClient>>,
  master_alert: EventInfo,
) {
  // Start the data collection process and wait for it to complete
  let mut data = collection::collect_data(receiver, master_alert).await;

  // Use the collected data to carry out the fusion algorithm
  match algorithm::localize_event(data.as_mut_slice(), None).await {
    Ok((event_time, lat, lon, height)) => {
      info!("Localization successful: LLH pos = <{lat:.8}, {lon:.8}, {height:.8}>, time = {event_time:.3}");
      // TODO: Send the fusion result somewhere, e.g., back to the MQTT broker or SQS or to a database
    }
    Err(message) => warn!("{message}"),
  }
}
