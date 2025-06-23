pub mod aws;
pub mod params;

use bytes::Bytes;

#[repr(packed, C)]
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

// TODO: Implement all of this
#[derive(Clone)]
pub struct EvidenceClip {
  pub client_id: String,
  pub clip_idx: u32,
  pub clip_data: Bytes,
}

#[must_use]
pub fn bytes_to_alert_data(message: &[u8]) -> Option<AlertData> {
  if message.len() == std::mem::size_of::<AlertDataPacked>() {
    let packed: &AlertDataPacked = unsafe { &*message.as_ptr().cast() };
    if packed.timestamp > 1_750_453_333.0
      && packed.timestamp < 1_908_237_733.0
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

#[must_use]
pub fn bytes_to_evidence_clip_data(message: Bytes) -> Option<EvidenceClip> {
  if message.len() > 4 {
    Some(EvidenceClip {
      client_id: String::from("Unknown"), // TODO: Implement this
      clip_idx: unsafe { *message.as_ptr().cast() },
      clip_data: message.slice(4..),
    })
  } else {
    None
  }
}
