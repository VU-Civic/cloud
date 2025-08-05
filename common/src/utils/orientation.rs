pub fn orientation_to_quaternions(sensor_orientation: [i32; 3]) -> (f64, f64, f64, f64) {
  let qx = f64::from(sensor_orientation[1]) / 1073741824.0;
  let qy = f64::from(sensor_orientation[2]) / 1073741824.0;
  let qz = -f64::from(sensor_orientation[0]) / 1073741824.0;
  let qw = (1.0 - (qx.powi(2) + qy.powi(2) + qz.powi(2))).sqrt();
  (qw, qx, qy, qz)
}

pub fn adjust_angle_of_arrival(angle_of_arrival: [f32; 3], _qw: f64, _qx: f64, _qy: f64, _qz: f64) -> [f64; 3] {
  // TODO: Implement this adjustment algorithm
  // For now, we will just return the angle_of_arrival as f64

  // Convert sensor orientation from quaternions to rotation matrix
  //let rotation_matrix = quaternion_to_rotation_matrix(sensor_orientation);

  // Apply the rotation to the angle of arrival
  //let adjusted_aoa = apply_rotation_to_vector(rotation_matrix, angle_of_arrival);

  // Return the adjusted angle of arrival as f64
  /*[
    qw * f64::from(angle_of_arrival[0]) + qy * f64::from(angle_of_arrival[2]) - qz * f64::from(angle_of_arrival[1]),
    qw * f64::from(angle_of_arrival[1]) + qz * f64::from(angle_of_arrival[0]) - qx * f64::from(angle_of_arrival[2]),
    qw * f64::from(angle_of_arrival[2]) + qx * f64::from(angle_of_arrival[1]) - qy * f64::from(angle_of_arrival[0]),
  ]*/
  [
    f64::from(angle_of_arrival[0]),
    f64::from(angle_of_arrival[1]),
    f64::from(angle_of_arrival[2]),
  ]
}
