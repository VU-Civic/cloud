use reqwest;

#[allow(clippy::cast_possible_truncation)]
pub async fn get_temperature(api_id: &str, lat: f64, lon: f64) -> Result<f32, String> {
  let url = format!("https://api.tomorrow.io/v4/weather/realtime?location={lat},{lon}&units=metric&apikey={api_id}");
  let client = reqwest::Client::builder()
    .user_agent("(civicalert.net, support@civicalert.net)")
    .build()
    .map_err(|e| e.to_string())?;
  let result: serde_json::Value = client
    .get(&url)
    .send()
    .await
    .map_err(|e| e.to_string())?
    .json()
    .await
    .map_err(|e| e.to_string())?;
  let temperature = result["data"]["values"]["temperature"]
    .as_f64()
    .ok_or("Failed to get temperature from Tomorrow.io")?;
  Ok(temperature as f32)
}

#[cfg(test)]
mod tests {
  use crate::{params, weather::tomorrowio};

  #[tokio::test(flavor = "multi_thread")]
  async fn test_tomorrowio() {
    // Fetch the current temperature for a known location in Nashville, TN
    let result = tomorrowio::get_temperature(params::WEATHER_TOMORROWIO_API_ID.as_str(), 36.174400, -86.762800).await;
    assert!(result.is_ok(), "Failed to fetch temperature: {:?}", result.err());
    let temperature = result.unwrap();
    assert!(
      temperature > -50.0 && temperature < 50.0,
      "Temperature out of expected range: {}",
      temperature
    );
    println!("Current Tomorrow.io temperature in Nashville, TN: {:.2}°C", temperature);
  }
}
