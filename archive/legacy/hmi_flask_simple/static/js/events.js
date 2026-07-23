export function connectEvents(onEvent, onError) {
  const source = new EventSource("/api/v1/events");
  source.addEventListener("snapshot", (event) => onEvent("snapshot", JSON.parse(event.data)));
  source.onmessage = (event) => {
    const message = JSON.parse(event.data);
    onEvent(message.type, message.payload);
  };
  source.onerror = () => onError?.();
  return () => source.close();
}
