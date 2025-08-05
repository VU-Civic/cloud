use reqwest;

#[allow(clippy::cast_possible_truncation)]
pub async fn get_temperature(api_id: &str, lat: f64, lon: f64) -> Result<f32, String> {
  let client = reqwest::Client::builder()
    .user_agent("(civicalert.net, support@civicalert.net)")
    .build()
    .map_err(|e| e.to_string())?;
  let url =
    format!("https://api.openweathermap.org/data/3.0/onecall?lat={lat:.6}&lon={lon:.6}&units=metric&appid={api_id}");
  let result: serde_json::Value = client
    .get(&url)
    .send()
    .await
    .map_err(|e| e.to_string())?
    .json()
    .await
    .map_err(|e| e.to_string())?;
  let temperature = result["current"]["temp"]
    .as_f64()
    .ok_or("Failed to get temperature from OpenWeatherMap")?;
  Ok(temperature as f32)
}

#[cfg(test)]
mod tests {
  use crate::{params, weather::openweathermap};

  #[tokio::test(flavor = "multi_thread")]
  async fn test_openweathermap() {
    // Fetch the current temperature for a known location in Nashville, TN
    let result =
      openweathermap::get_temperature(params::WEATHER_OPENWEATHERMAP_API_ID.as_str(), 36.174400, -86.762800).await;
    assert!(result.is_ok(), "Failed to fetch temperature: {:?}", result.err());
    let temperature = result.unwrap();
    assert!(
      temperature > -50.0 && temperature < 50.0,
      "Temperature out of expected range: {}",
      temperature
    );
    println!(
      "Current OpenWeatherMap temperature in Nashville, TN: {:.2}°C",
      temperature
    );
  }
}
