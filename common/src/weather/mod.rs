mod noaa;
mod openweathermap;
mod tomorrowio;

use crate::params;

pub async fn get_temperature(lat: f32, lon: f32) -> Result<f32, String> {
  // Attempt to get temperature from NOAA first followed by Tomorrow.io then OpenWeatherMap
  let mut result = noaa::get_temperature(lat, lon).await;
  if !result.is_ok() {
    result = tomorrowio::get_temperature(params::WEATHER_TOMORROWIO_API_ID.as_str(), lat, lon).await;
  }
  if !result.is_ok() {
    result = openweathermap::get_temperature(params::WEATHER_OPENWEATHERMAP_API_ID.as_str(), lat, lon).await;
  }
  result.map_err(|e| format!("Failed to get temperature: {e}"))
}

#[cfg(test)]
mod tests {
  use crate::weather;

  #[tokio::test(flavor = "multi_thread")]
  async fn test_weather() {
    // Fetch the current temperature for a known location in Nashville, TN
    let result = weather::get_temperature(36.174400, -86.762800).await;
    assert!(result.is_ok(), "Failed to fetch temperature: {:?}", result.err());
    let temperature = result.unwrap();
    assert!(
      temperature > -50.0 && temperature < 50.0,
      "Temperature out of expected range: {}",
      temperature
    );
    println!("Current temperature in Nashville, TN: {:.2}°C", temperature);
  }
}
