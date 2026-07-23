const token = document.querySelector('meta[name="app-token"]').content;

async function request(path, options = {}) {
  const response = await fetch(path, {
    ...options,
    headers: { "Content-Type": "application/json", "X-App-Token": token, ...(options.headers || {}) }
  });
  const data = await response.json();
  if (!response.ok) throw new Error(data.error || `HTTP ${response.status}`);
  return data;
}

export const api = {
  status: () => request("/api/v1/status", { method: "GET" }),
  configure: (robotHost) => request("/api/v1/config/robot", { method: "PUT", body: JSON.stringify({ robot_host: robotHost }) }),
  command: (name, payload = {}) => request("/api/v1/commands", { method: "POST", body: JSON.stringify({ name, payload }) }),
  estop: () => request("/api/v1/estop", { method: "POST", body: "{}" }),
  session: (action) => request(`/api/v1/sessions/${action}`, { method: "POST", body: "{}" }),
  connection: (action) => request(`/api/v1/connection/${action}`, { method: "POST", body: "{}" }),
  sessions: () => request("/api/v1/sessions", { method: "GET" })
};
