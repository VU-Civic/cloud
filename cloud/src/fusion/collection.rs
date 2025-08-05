use civicalert_cloud_common::{EventInfo, params};
use tokio::sync::broadcast::Receiver;

async fn run_data_collection(mut receiver: Receiver<EventInfo>, data: &mut Vec<EventInfo>, master_alert: EventInfo) {
  data.push(master_alert);
  loop {
    if let Ok(message) = receiver.recv().await {
      // TODO: Verify that the message contains data feasibly related to the master_alert
      data.push(message);
    }
  }
}

pub async fn collect_data(receiver: Receiver<EventInfo>, master_alert: EventInfo) -> Vec<EventInfo> {
  // Run the data collection task for a limited time
  let mut data: Vec<EventInfo> = Vec::new();
  let _ = tokio::time::timeout(
    tokio::time::Duration::from_secs(params::FUSION_DATA_COLLECTION_SECONDS),
    run_data_collection(receiver, &mut data, master_alert),
  )
  .await;
  data
}
