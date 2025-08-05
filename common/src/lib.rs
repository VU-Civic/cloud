pub mod aws;
pub mod params;
pub mod utils;
pub mod weather;

use bytes::Bytes;
use std::time::{SystemTime, UNIX_EPOCH};

#[repr(packed, C)]
struct AlertHeaderPacked {
  num_events: u8,
  cell_signal_power: u8,
  cell_signal_quality: u8,
  sensor_temperature_alert: u8,
  sensor_orientation: [i32; 3],
  sensor_location: [f32; 3],
  device_id: [u8; params::CELL_IMEI_LENGTH + 1],
}

#[repr(packed, C)]
struct EventInfoPacked {
  id: u32,
  class: u32,
  timestamp: f64,
  confidence: f32,
  angle_of_arrival: [f32; 3],
}

#[derive(Clone, Copy)]
pub struct EventInfo {
  pub id: u32,
  pub class: u32,
  pub timestamp: f64,
  pub confidence: f32,
  pub angle_of_arrival: [f64; 3],
  pub sensor_llh: [f64; 3],
  pub sensor_ecef: [f64; 3],
  pub previously_used: bool,
  pub sensor_enu: [f64; 3],
  pub relative_timestamp: f64,
}

pub struct AlertData {
  pub device_id: String,
  pub timestamp: f64,
  pub cell_signal_power: u8,
  pub cell_signal_quality: u8,
  pub sensor_temperature_alert: u8,
  pub sensor_quaternions: [f64; 4],
  pub sensor_llh: [f64; 3],
  pub events: Vec<EventInfo>,
}

// TODO: Implement everything about evidence clips
#[derive(Clone)]
pub struct EvidenceClip {
  pub client_id: String,
  pub clip_idx: u32,
  pub clip_data: Bytes,
}

impl std::fmt::Display for EventInfo {
  fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
    write!(
      f,
      "{{ id: {}, timestamp: {}, class: {}, confidence: {}, lat: {}, lon: {}, height: {}, angle_of_arrival: [{}, {}, {}] }}",
      self.id,
      self.timestamp,
      self.class,
      self.confidence,
      self.sensor_llh[0],
      self.sensor_llh[1],
      self.sensor_llh[2],
      self.angle_of_arrival[0],
      self.angle_of_arrival[1],
      self.angle_of_arrival[2]
    )
  }
}

#[must_use]
pub fn bytes_to_alert_data(message: &[u8]) -> Option<AlertData> {
  // Ensure the message is large enough to contain a valid header
  if message.len() >= std::mem::size_of::<AlertHeaderPacked>() {
    let header: &AlertHeaderPacked = unsafe { &*message.as_ptr().cast() };

    // Validate the header fields
    if header.sensor_location[0] >= -90.0
      && header.sensor_location[0] <= 90.0
      && header.sensor_location[1] >= -180.0
      && header.sensor_location[1] <= 180.0
      && header.sensor_location[2] >= -500.0
    {
      // Retrieve the device ID and current timestamp
      let device_id = String::from_utf8_lossy(header.device_id.as_slice());
      let current_timestamp = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap_or(tokio::time::Duration::from_secs(0))
        .as_secs_f64();

      // Convert the sensor location into ECEF coordinates
      let (lat, lon, height) = (
        f64::from(header.sensor_location[0]),
        f64::from(header.sensor_location[1]),
        f64::from(header.sensor_location[2]),
      );
      let (ecef_x, ecef_y, ecef_z) = utils::coordinates::llh_to_ecef(lat, lon, height);

      // Convert the sensor orientation into quaternions
      let (qw, qx, qy, qz) = utils::orientation::orientation_to_quaternions(header.sensor_orientation);

      // Iterate through all events in the message
      let mut events = Vec::with_capacity(header.num_events as usize);
      for i in 0..header.num_events.into() {
        if message.len() >= std::mem::size_of::<AlertHeaderPacked>() + std::mem::size_of::<EventInfoPacked>() * (i + 1)
        {
          let packed: &EventInfoPacked = unsafe {
            &*message
              .as_ptr()
              .add(std::mem::size_of::<AlertHeaderPacked>() + std::mem::size_of::<EventInfoPacked>() * i)
              .cast()
          };

          // Validate the event data and convert it into an EventInfo structure
          if packed.timestamp > current_timestamp - params::DATA_MAX_TIMESTAMP_IN_PAST_SECONDS
            && packed.timestamp < current_timestamp + 10.0
          {
            events.push(EventInfo {
              id: packed.id,
              class: packed.class,
              timestamp: packed.timestamp,
              confidence: packed.confidence,
              angle_of_arrival: utils::orientation::adjust_angle_of_arrival(packed.angle_of_arrival, qw, qx, qy, qz),
              sensor_llh: [lat, lon, height],
              sensor_ecef: [ecef_x, ecef_y, ecef_z],
              previously_used: false,
              sensor_enu: [0.0, 0.0, 0.0],
              relative_timestamp: 0.0,
            });
          }
        }
      }

      // Construct and return an AlertData structure
      Some(AlertData {
        device_id: device_id.to_string(),
        timestamp: current_timestamp,
        cell_signal_power: header.cell_signal_power,
        cell_signal_quality: header.cell_signal_quality,
        sensor_temperature_alert: header.sensor_temperature_alert,
        sensor_quaternions: [qw, qx, qy, qz],
        sensor_llh: [lat, lon, height],
        events,
      })
    } else {
      None
    }
  } else {
    None
  }
}

#[must_use]
pub fn bytes_to_evidence_clip_data(message: &Bytes) -> Option<EvidenceClip> {
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
