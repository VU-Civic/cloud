use civicalert_cloud_common::{EventInfo, params, utils::coordinates, weather};
use tracing::{debug, info};

pub fn localize_using_hough_transform(
  events: &[&mut EventInfo],
  speed_of_sound: f32,
) -> Result<(f64, f64, f64, f64), String> {
  //if event.angle_of_arrival.iter().all(|&angle| !angle.is_nan())
  Ok((0.0, 0.0, 0.0, 0.0))
}

#[allow(clippy::cast_precision_loss)]
pub async fn localize_event(
  events: &mut [EventInfo],
  manual_speed_of_sound: Option<f32>,
) -> Result<(f64, f64, f64, f64), String> {
  // Only utilize events that have not already been used in a previous localization
  let mut events: Vec<&mut EventInfo> = events.iter_mut().filter(|event| !event.previously_used).collect();
  if events.len() >= params::FUSION_ALGORITHM_MIN_NUM_EVENTS {
    info!("Localizing event with {} alerts", events.len());

    // Calculate the centroid of the sensor positions and the timestamps of arrival relative to the earliest event
    let reference_time_of_arrival = events
      .iter()
      .map(|alert| alert.timestamp)
      .fold(f64::MAX, f64::min);
    let lat_centroid = events.iter().map(|event| event.sensor_llh[0]).sum::<f64>() / events.len() as f64;
    let lon_centroid = events.iter().map(|event| event.sensor_llh[1]).sum::<f64>() / events.len() as f64;
    let height_centroid = events.iter().map(|event| event.sensor_llh[2]).sum::<f64>() / events.len() as f64;
    for event in &mut events {
      event.relative_timestamp = event.timestamp - reference_time_of_arrival;
    }
    debug!(
      "Centroid of sensor positions (LLH): <{:.8}, {:.8}, {:.8}>\n{}",
      lat_centroid,
      lon_centroid,
      height_centroid,
      "Sensor Positions (LLH) / Sensor Positions (ENU) / Angles of Arrival (ENU) / Relative Times of Arrival (MS):"
    );

    // Transform the sensor positions for each event into an ENU coordinate frame
    for event in &mut events {
      (event.sensor_enu[0], event.sensor_enu[1], event.sensor_enu[2]) = coordinates::ecef_to_enu(
        lat_centroid,
        lon_centroid,
        height_centroid,
        event.sensor_ecef[0],
        event.sensor_ecef[1],
        event.sensor_ecef[2],
      );
      debug!(
        "<{:.8}, {:.8}, {:.8}> / <{:.6}, {:.6}, {:.6}> / [{:.6}, {:.6}, {:.6}] / {:.3}",
        event.sensor_llh[0],
        event.sensor_llh[1],
        event.sensor_llh[2],
        event.sensor_enu[0],
        event.sensor_enu[1],
        event.sensor_enu[2],
        event.angle_of_arrival[0],
        event.angle_of_arrival[1],
        event.angle_of_arrival[2],
        1000.0 * event.relative_timestamp
      );
    }

    // Calculate the speed of sound using the current temperature at the event location
    let speed_of_sound = if let Some(manual_speed_of_sound) = manual_speed_of_sound {
      manual_speed_of_sound
    } else if let Ok(temperature) = weather::get_temperature(lat_centroid, lon_centroid).await {
      331.4 + 0.6 * temperature
    } else {
      343.3
    };
    debug!("Speed of sound at event location = {speed_of_sound} m/s");

    // Perform a Hough transform-based localization algorithm and transform the ENU result into a geodetic coordinate frame
    if let Ok((event_time_seconds, east_result, north_result, up_result)) =
      localize_using_hough_transform(events.as_slice(), speed_of_sound)
    {
      let event_time = reference_time_of_arrival + event_time_seconds;
      let (x_result, y_result, z_result) = coordinates::enu_to_ecef(
        lat_centroid,
        lon_centroid,
        height_centroid,
        east_result,
        north_result,
        up_result,
      );
      let (lat_result, lon_result, height_result) =
        coordinates::ecef_to_llh(x_result, y_result, z_result);
      Ok((event_time, lat_result, lon_result, height_result))
    } else {
      Err(String::from("Unable to determine the location of the event"))
    }
  } else {
    Err(String::from("Too few events for localization, skipping..."))
  }
}
