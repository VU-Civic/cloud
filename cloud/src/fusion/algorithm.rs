use civicalert_cloud_common::AlertData;
use tracing::info;

pub fn localize_event(alerts: &[AlertData]) -> () {
  info!("Localizing event with {} alerts", alerts.len());
}
