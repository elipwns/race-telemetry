// Race Telemetry Dashboard
// Update WEBSOCKET_URL after terraform apply (from outputs: websocket_endpoint)

const WEBSOCKET_URL = 'wss://REPLACE_ME.execute-api.us-west-2.amazonaws.com/prod';

// --- Map setup ---
const map = L.map('map').setView([36.0, -96.0], 4);

L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
  attribution: '© OpenStreetMap contributors',
  maxZoom: 19,
}).addTo(map);

const carIcon = L.divIcon({ className: 'car-marker', iconSize: [14, 14] });
let carMarker = null;
let trackPath = L.polyline([], { color: '#58a6ff', weight: 2, opacity: 0.7 }).addTo(map);
let hasZoomed = false;

// --- State ---
let sessionStart = null;
let sessionTimerInterval = null;

// --- WebSocket ---
let ws = null;

function connect() {
  ws = new WebSocket(WEBSOCKET_URL);

  ws.onopen = () => {
    document.getElementById('connection-status').textContent = 'Connected';
    document.getElementById('connection-status').className = 'status connected';
  };

  ws.onclose = () => {
    document.getElementById('connection-status').textContent = 'Disconnected';
    document.getElementById('connection-status').className = 'status disconnected';
    // Reconnect after 3s
    setTimeout(connect, 3000);
  };

  ws.onerror = () => ws.close();

  ws.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);
      handleTelemetry(data);
    } catch (e) {
      console.warn('Failed to parse message', e);
    }
  };
}

function handleTelemetry(data) {
  const lat = parseFloat(data.lat);
  const lon = parseFloat(data.lon);
  const speed = parseFloat(data.speed_kph);
  const sats = parseInt(data.satellites);
  const session = data.session_id || '—';

  // Update speed
  document.getElementById('speed').textContent = speed.toFixed(0);

  // Update satellites
  document.getElementById('satellites').textContent = sats;

  // Update session
  document.getElementById('session-id').textContent = session;
  if (!sessionStart) {
    sessionStart = Date.now();
    startSessionTimer();
  }

  // Update map
  if (!isNaN(lat) && !isNaN(lon)) {
    const latlng = [lat, lon];
    trackPath.addLatLng(latlng);

    if (!carMarker) {
      carMarker = L.marker(latlng, { icon: carIcon }).addTo(map);
    } else {
      carMarker.setLatLng(latlng);
    }

    // Auto-zoom to car on first fix
    if (!hasZoomed && sats >= 4) {
      map.setView(latlng, 16);
      hasZoomed = true;
    }
  }
}

function startSessionTimer() {
  if (sessionTimerInterval) clearInterval(sessionTimerInterval);
  sessionTimerInterval = setInterval(() => {
    const elapsed = Math.floor((Date.now() - sessionStart) / 1000);
    const h = Math.floor(elapsed / 3600);
    const m = Math.floor((elapsed % 3600) / 60).toString().padStart(2, '0');
    const s = (elapsed % 60).toString().padStart(2, '0');
    document.getElementById('session-time').textContent =
      h > 0 ? `${h}:${m}:${s}` : `${m}:${s}`;
  }, 1000);
}

// Weather is not sent over WebSocket (only telemetry is).
// To show weather, poll the REST API — implement in Phase 2.
// For now, weather card shows last values from session context.

connect();
