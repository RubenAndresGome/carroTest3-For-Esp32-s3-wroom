const points = [];
const limit = 300;

export function renderTelemetry(data) {
  if (!data) return;
  const write = (id, value) => { document.getElementById(id).textContent = String(value); };
  write("robot-state", data.state);
  write("pose-x", Number(data.x_mm).toFixed(1));
  write("pose-y", Number(data.y_mm).toFixed(1));
  write("pose-yaw", Number(data.yaw_deg).toFixed(1));
  write("pulse-fl", data.pulses.fl);
  write("pulse-fr", data.pulses.fr);
  write("pulse-bl", data.pulses.bl);
  write("pulse-br", data.pulses.br);
  write("pwm-label", `PWM ${data.pwm.left} / ${data.pwm.right}`);
  const speed = data.wheel_speed_cm_s || { left: 0, right: 0 };
  write("wheel-speed", `Velocidad filtrada: ${Number(speed.left).toFixed(1)} / ${Number(speed.right).toFixed(1)} cm/s`);
  const mpu = data.mpu || {};
  const calibration = mpu.calibrated === true ? "calibrado" : "sin calibrar";
  write(
    "filter-status",
    `MPU ${calibration} · offset GZ ${Number(mpu.gyro_z_offset_rad_s || 0).toFixed(5)} rad/s · θ error ${Number(mpu.theta_error_deg || 0).toFixed(2)}°`
  );
  points.push([Number(data.x_mm), Number(data.y_mm)]);
  if (points.length > limit) points.shift();
  drawPath();
}

function drawPath() {
  const canvas = document.getElementById("path-chart");
  const ratio = window.devicePixelRatio || 1;
  const width = canvas.clientWidth;
  const height = canvas.clientHeight;
  if (canvas.width !== width * ratio || canvas.height !== height * ratio) {
    canvas.width = width * ratio; canvas.height = height * ratio;
  }
  const context = canvas.getContext("2d");
  context.setTransform(ratio, 0, 0, ratio, 0, 0);
  context.clearRect(0, 0, width, height);
  context.strokeStyle = "#273548"; context.lineWidth = 1;
  for (let x = 20; x < width; x += 40) { context.beginPath(); context.moveTo(x, 0); context.lineTo(x, height); context.stroke(); }
  for (let y = 20; y < height; y += 40) { context.beginPath(); context.moveTo(0, y); context.lineTo(width, y); context.stroke(); }
  if (points.length < 2) return;
  const xs = points.map((p) => p[0]), ys = points.map((p) => p[1]);
  const minX = Math.min(...xs), maxX = Math.max(...xs), minY = Math.min(...ys), maxY = Math.max(...ys);
  const scale = Math.min((width - 24) / Math.max(maxX - minX, 1), (height - 24) / Math.max(maxY - minY, 1));
  context.strokeStyle = "#38d6e7"; context.lineWidth = 2; context.beginPath();
  points.forEach(([x, y], index) => {
    const px = 12 + (x - minX) * scale, py = height - 12 - (y - minY) * scale;
    if (index === 0) context.moveTo(px, py); else context.lineTo(px, py);
  });
  context.stroke();
}
