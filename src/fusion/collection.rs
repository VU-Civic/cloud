use crate::{AlertData, params};
use tokio::sync::broadcast::Receiver;

async fn run_data_collection(mut receiver: Receiver<AlertData>, data: &mut Vec<AlertData>, master_alert: AlertData) {
  data.push(master_alert.clone());
  loop {
    if let Ok(message) = receiver.recv().await {
      // TODO: Verify that the message contains data feasibly related to the master_alert
      println!("Received message {}", message);
      data.push(message);
    }
  }
}

pub async fn collect_data(receiver: Receiver<AlertData>, master_alert: AlertData) -> Vec<AlertData> {
  // Run the data collection task for a limited time
  let mut data: Vec<AlertData> = Vec::new();
  let _ = tokio::time::timeout(
    std::time::Duration::from_secs(params::FUSION_DATA_COLLECTION_SECONDS),
    run_data_collection(receiver, &mut data, master_alert),
  )
  .await
  .unwrap();
  data
}
