const initial = { connection: { state: "connecting", detail: null, protocol: "—" }, telemetry: null, recording: false, session_id: null };
let state = structuredClone(initial);
const listeners = new Set();

export const store = {
  get: () => state,
  set(patch) { state = { ...state, ...patch }; listeners.forEach((listener) => listener(state)); },
  subscribe(listener) { listeners.add(listener); listener(state); return () => listeners.delete(listener); }
};
