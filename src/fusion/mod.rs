mod algorithm;
mod collection;

use crate::AlertData;
use tokio::sync::broadcast::Receiver;

pub async fn begin_fusion(receiver: Receiver<AlertData>, master_alert: AlertData) {
  // Start the data collection process and wait for it to complete
  let _data = collection::collect_data(receiver, master_alert).await;

  // TODO: Use the collected data to carry out the fusion process

  // TODO: Send the fusion result somewhere, e.g., back to the MQTT broker or SQS or to a database
}
