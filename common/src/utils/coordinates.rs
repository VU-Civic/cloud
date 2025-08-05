const A: f64 = 6_378_137.0; // WGS-84 semi-major axis
const F: f64 = 1.0 / 298.257_223_563; // WGS-84 flattening
const E2: f64 = F * (2.0 - F); // Square of eccentricity

const A1: f64 = A * E2;
const A2: f64 = A1 * A1;
const A3: f64 = 0.5 * A * E2 * E2;
const A4: f64 = (5.0 / 2.0) * A2;
const A5: f64 = A1 + A3;
const A6: f64 = 1.0 - E2;

#[must_use]
pub fn llh_to_ecef(lat: f64, lon: f64, height: f64) -> (f64, f64, f64) {
  let lat_rad = lat.to_radians();
  let lon_rad = lon.to_radians();
  let sin_lat_rad = lat_rad.sin();
  let n = A / (1.0 - E2 * sin_lat_rad.powi(2)).sqrt();
  let common_param = (n + height) * lat_rad.cos();

  let x = common_param * lon_rad.cos();
  let y = common_param * lon_rad.sin();
  let z = ((1.0 - E2) * n + height) * sin_lat_rad;
  (x, y, z)
}

#[must_use]
pub fn ecef_to_enu(lat_ref: f64, lon_ref: f64, height_ref: f64, x: f64, y: f64, z: f64) -> (f64, f64, f64) {
  let lat_rad = lat_ref.to_radians();
  let lon_rad = lon_ref.to_radians();
  let sin_lat_rad = lat_rad.sin();
  let cos_lat_rad = lat_rad.cos();
  let sin_lon_rad = lon_rad.sin();
  let cos_lon_rad = lon_rad.cos();
  let n = A / (1.0 - E2 * sin_lat_rad.powi(2)).sqrt();
  let common_param = (n + height_ref) * cos_lat_rad;
  let x_ref = common_param * cos_lon_rad;
  let y_ref = common_param * sin_lon_rad;
  let z_ref = ((1.0 - E2) * n + height_ref) * sin_lat_rad;
  let dx = x - x_ref;
  let dy = y - y_ref;
  let dz = z - z_ref;
  let east = -sin_lon_rad * dx + cos_lon_rad * dy;
  let north = -sin_lat_rad * cos_lon_rad * dx - sin_lat_rad * sin_lon_rad * dy + cos_lat_rad * dz;
  let up = cos_lat_rad * cos_lon_rad * dx + cos_lat_rad * sin_lon_rad * dy + sin_lat_rad * dz;
  (east, north, up)
}

#[must_use]
pub fn enu_to_ecef(lat_ref: f64, lon_ref: f64, height_ref: f64, east: f64, north: f64, up: f64) -> (f64, f64, f64) {
  let lat_rad = lat_ref.to_radians();
  let lon_rad = lon_ref.to_radians();
  let sin_lat_rad = lat_rad.sin();
  let cos_lat_rad = lat_rad.cos();
  let sin_lon_rad = lon_rad.sin();
  let cos_lon_rad = lon_rad.cos();
  let n = A / (1.0 - E2 * sin_lat_rad.powi(2)).sqrt();
  let common_param = (n + height_ref) * cos_lat_rad;
  let x_ref = common_param * cos_lon_rad;
  let y_ref = common_param * sin_lon_rad;
  let z_ref = ((1.0 - E2) * n + height_ref) * sin_lat_rad;
  let dx = -sin_lon_rad * east - sin_lat_rad * cos_lon_rad * north + cos_lat_rad * cos_lon_rad * up;
  let dy = cos_lon_rad * east - sin_lat_rad * sin_lon_rad * north + cos_lat_rad * sin_lon_rad * up;
  let dz = cos_lat_rad * north + sin_lat_rad * up;
  (x_ref + dx, y_ref + dy, z_ref + dz)
}

#[must_use]
#[allow(clippy::many_single_char_names)]
pub fn ecef_to_llh(ecef_x: f64, ecef_y: f64, ecef_z: f64) -> (f64, f64, f64) {
  let z_pos = ecef_z.abs();
  let w2 = ecef_x.powi(2) + ecef_y.powi(2);
  let z2 = z_pos.powi(2);
  let w = w2.sqrt();
  let r2 = w2 + z2;
  let r = r2.sqrt();
  let mut s = z_pos / r;
  let mut c = w / r;
  let mut u = A2 / r;
  let mut v = A3 - (A4 / r);
  let mut s2 = s.powi(2);
  let c2 = c.powi(2);

  // Compute latitude differently depending on its nearness to the Earth's poles
  let lon_rad = ecef_y.atan2(ecef_x);
  let mut lat_rad = if c2 > 0.3 {
    s *= 1.0 + c2 * (A1 + u + s2 * v) / r;
    s2 = s.powi(2);
    c = (1.0 - s2).sqrt();
    s.asin()
  } else {
    c *= 1.0 - s2 * (A5 - u - c2 * v) / r;
    s2 = 1.0 - c.powi(2);
    s = s2.sqrt();
    c.acos()
  };

  // Compute height
  let g = 1.0 - E2 * s2;
  let r1 = A / g.sqrt();
  let rf = A6 * r1;
  u = w - r1 * c;
  v = z_pos - rf * s;
  let f = c * u + s * v;
  let m = c * v - s * u;
  let p = m / (rf / g + f);
  lat_rad += p;
  let height = f + 0.5 * m * p;

  // Convert radians to degrees
  if ecef_z < 0.0 {
    lat_rad = -lat_rad;
  }
  (lat_rad.to_degrees(), lon_rad.to_degrees(), height)
}

#[must_use]
pub fn distance_from_llh3d(lat1: f64, lon1: f64, height1: f64, lat2: f64, lon2: f64, height2: f64) -> f64 {
  let (x1, y1, z1) = llh_to_ecef(lat1, lon1, height1);
  let (x2, y2, z2) = llh_to_ecef(lat2, lon2, height2);
  ((x2 - x1).powi(2) + (y2 - y1).powi(2) + (z2 - z1).powi(2)).sqrt()
}

#[must_use]
pub fn distance_from_llh2d(lat1: f64, lon1: f64, lat2: f64, lon2: f64) -> f64 {
  let (x1, y1, z1) = llh_to_ecef(lat1, lon1, 0.0);
  let (x2, y2, z2) = llh_to_ecef(lat2, lon2, 0.0);
  ((x2 - x1).powi(2) + (y2 - y1).powi(2) + (z2 - z1).powi(2)).sqrt()
}

#[must_use]
pub fn distance_from_ecef(x1: f64, y1: f64, z1: f64, x2: f64, y2: f64, z2: f64) -> f64 {
  ((x2 - x1).powi(2) + (y2 - y1).powi(2) + (z2 - z1).powi(2)).sqrt()
}

#[must_use]
pub fn estimate_distance2d(lat1: f64, lon1: f64, lat2: f64, lon2: f64) -> f64 {
  let x = lat2.to_radians() - lat1.to_radians();
  let y = (lon2.to_radians() - lon1.to_radians()) * lat1.to_radians().cos();
  (x.powi(2) + y.powi(2)).sqrt() * A
}

#[cfg(test)]
mod tests {
  use crate::utils::coordinates;

  #[tokio::test(flavor = "multi_thread")]
  async fn test_coords_llh_to_ecef() {
    let (x, y, z) = coordinates::llh_to_ecef(36.174465, -86.767960, 173.0);
    assert!(x.is_finite() && y.is_finite() && z.is_finite());
    assert!((x - 290623.1176287577).abs() < 1e-6);
    assert!((y + 5146536.654098851).abs() < 1e-6);
    assert!((z - 3743937.9931584625).abs() < 1e-6);
  }

  #[tokio::test(flavor = "multi_thread")]
  async fn test_coords_ecef_to_enu() {
    let (east, north, up) = coordinates::ecef_to_enu(
      36.174465,
      -86.767960,
      173.0,
      291438.1704373956,
      -5146433.161489106,
      3744016.3697645636,
    );
    assert!(east.is_finite() && north.is_finite() && up.is_finite());
    assert!((east - 819.5912750373138).abs() < 1e-6);
    assert!((north - 97.13306626462759).abs() < 1e-6);
    assert!((up - -0.05333789603885464).abs() < 1e-6);
  }

  #[tokio::test(flavor = "multi_thread")]
  async fn test_coords_enu_to_ecef() {
    let (x, y, z) = coordinates::enu_to_ecef(
      36.174465,
      -86.767960,
      173.0,
      819.5912750373138,
      97.13306626462759,
      -0.05333789603885464,
    );
    assert!(x.is_finite() && y.is_finite() && z.is_finite());
    assert!((x - 291438.1704373956).abs() < 1e-6);
    assert!((y + 5146433.161489106).abs() < 1e-6);
    assert!((z - 3744016.3697645636).abs() < 1e-6);
  }

  #[tokio::test(flavor = "multi_thread")]
  async fn test_coords_ecef_to_llh() {
    let (lat, lon, height) = coordinates::ecef_to_llh(290623.1176287577, -5146536.654098851, 3743937.9931584625);
    assert!(lat.is_finite() && lon.is_finite() && height.is_finite());
    assert!((lat - 36.174465).abs() < 1e-6);
    assert!((lon + 86.767960).abs() < 1e-6);
    assert!((height - 173.0).abs() < 1e-6);
  }

  #[tokio::test(flavor = "multi_thread")]
  async fn test_coords_distance_from_llh3d() {
    let distance = coordinates::distance_from_llh3d(36.174465, -86.767960, 173.0, 36.175340, -86.758850, 183.0);
    assert!(distance.is_finite());
    assert!((distance - 825.35).abs() < 0.1);
  }

  #[tokio::test(flavor = "multi_thread")]
  async fn test_coords_distance_from_llh2d() {
    let distance = coordinates::distance_from_llh2d(36.174465, -86.767960, 36.175340, -86.758850);
    assert!(distance.is_finite());
    assert!((distance - 825.3).abs() < 0.1);
  }

  #[tokio::test(flavor = "multi_thread")]
  async fn test_coords_distance_from_ecef() {
    let distance = coordinates::distance_from_ecef(
      290623.1176287577,
      -5146536.654098851,
      3743937.9931584625,
      291438.1704373956,
      -5146433.161489106,
      3744016.3697645636,
    );
    assert!(distance.is_finite());
    assert!((distance - 825.3).abs() < 0.1);
  }

  #[tokio::test(flavor = "multi_thread")]
  async fn test_coords_estimate_distance2d() {
    let distance = coordinates::estimate_distance2d(36.174465, -86.767960, 36.175340, -86.758850);
    assert!(distance.is_finite());
    assert!((distance - 825.0).abs() < 1.0);
  }
}
