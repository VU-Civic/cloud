use reqwest;

async fn get_grid_details(
  client: &reqwest::Client,
  lat: f64,
  lon: f64,
) -> Result<(String, String, String), reqwest::Error> {
  let url = format!("https://api.weather.gov/points/{lat:.4},{lon:.4}");
  let result: serde_json::Value = client.get(&url).send().await?.json().await?;
  let properties = &result["properties"];
  Ok((
    properties["gridId"].as_str().unwrap().to_string(),
    properties["gridX"].as_number().unwrap().to_string(),
    properties["gridY"].as_number().unwrap().to_string(),
  ))
}

#[allow(clippy::cast_possible_truncation)]
async fn get_forecast(
  client: &reqwest::Client,
  grid_id: &str,
  grid_x: &str,
  grid_y: &str,
) -> Result<f32, reqwest::Error> {
  let url = format!("https://api.weather.gov/gridpoints/{grid_id}/{grid_x},{grid_y}/forecast");
  let result: serde_json::Value = client.get(&url).send().await?.json().await?;
  let properties = &result["properties"]["periods"][0];
  if properties["temperatureUnit"].as_str().unwrap_or("F").starts_with('F') {
    let fahrenheit = properties["temperature"].as_f64().unwrap();
    Ok(((fahrenheit - 32.0) * 5.0 / 9.0) as f32)
  } else {
    Ok(properties["temperature"].as_f64().unwrap() as f32)
  }
}

pub async fn get_temperature(lat: f64, lon: f64) -> Result<f32, String> {
  let client = reqwest::Client::builder()
    .user_agent("(civicalert.net, support@civicalert.net)")
    .build()
    .map_err(|e| e.to_string())?;
  let (grid_id, grid_x, grid_y) = get_grid_details(&client, lat, lon).await.map_err(|e| e.to_string())?;
  get_forecast(&client, &grid_id, &grid_x, &grid_y)
    .await
    .map_err(|e| e.to_string())
}

#[cfg(test)]
mod tests {
  use crate::weather::noaa;

  #[tokio::test(flavor = "multi_thread")]
  async fn test_noaa() {
    // Fetch the current temperature for a known location in Nashville, TN
    let result = noaa::get_temperature(36.174400, -86.762800).await;
    assert!(result.is_ok(), "Failed to fetch temperature: {:?}", result.err());
    let temperature = result.unwrap();
    assert!(
      temperature > -50.0 && temperature < 50.0,
      "Temperature out of expected range: {}",
      temperature
    );
    println!("Current NOAA temperature in Nashville, TN: {:.2}°C", temperature);
  }
}
