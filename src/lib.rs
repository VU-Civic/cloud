pub mod aws;
pub mod fusion;
pub mod params;

use bytes::Bytes;

#[repr(packed)]
struct AlertDataPacked {
  timestamp: f64,
  lat: f32,
  lon: f32,
  height: f32,
}

// TODO: Implement all of this
#[derive(Clone, Copy)]
pub struct AlertData {
  pub timestamp: f64,
  pub lat: f32,
  pub lon: f32,
  pub height: f32,
}

impl std::fmt::Display for AlertData {
  fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
    write!(f, "{{ timestamp: {} }}", self.timestamp)
  }
}

pub fn bytes_to_alert_data(bytes: &Bytes) -> Option<AlertData> {
  if bytes.len() == std::mem::size_of::<AlertDataPacked>() {
    let packed: &AlertDataPacked = unsafe { &*(bytes.as_ptr() as *const _) };
    if packed.timestamp > 1750453333.0
      && packed.timestamp < 1908237733.0
      && packed.lat >= -90.0
      && packed.lat <= 90.0
      && packed.lon >= -180.0
      && packed.lon <= 180.0
      && packed.height >= -500.0
    {
      Some(AlertData {
        timestamp: packed.timestamp,
        lat: packed.lat,
        lon: packed.lon,
        height: packed.height,
      })
    } else {
      None
    }
  } else {
    None
  }
}
