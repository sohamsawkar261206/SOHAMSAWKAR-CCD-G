// ============================================================================
// api.js — Centralized API client for RouteMaster backend
// ============================================================================

const API_BASE = '/api';

async function request(endpoint, options = {}) {
  const url = `${API_BASE}${endpoint}`;
  const config = {
    headers: { 'Content-Type': 'application/json' },
    ...options,
  };

  const response = await fetch(url, config);
  const data = await response.json();

  if (!response.ok) {
    throw new Error(data.error || `Request failed: ${response.status}`);
  }

  return data;
}

// ── Auth ──────────────────────────────────────────────────────────────────────
export const login = (username, password) =>
  request('/auth/login', {
    method: 'POST',
    body: JSON.stringify({ username, password }),
  });

// ── Stats ────────────────────────────────────────────────────────────────────
export const getStats = () => request('/stats');

// ── Users ────────────────────────────────────────────────────────────────────
export const getUsers = () => request('/users');
export const createUser = (data) =>
  request('/users', { method: 'POST', body: JSON.stringify(data) });
export const deleteUser = (id) =>
  request(`/users/${id}`, { method: 'DELETE' });

// ── Locations ────────────────────────────────────────────────────────────────
export const getLocations = () => request('/locations');
export const createLocation = (data) =>
  request('/locations', { method: 'POST', body: JSON.stringify(data) });
export const deleteLocation = (id) =>
  request(`/locations/${id}`, { method: 'DELETE' });

// ── Drivers ──────────────────────────────────────────────────────────────────
export const getDrivers = () => request('/drivers');
export const getAvailableDrivers = () => request('/drivers/available');
export const getDriverById = (id) => request(`/drivers/${id}`);
export const updateDriverStatus = (id, status) =>
  request(`/drivers/${id}/status`, {
    method: 'PUT',
    body: JSON.stringify({ status }),
  });

// ── Deliveries ───────────────────────────────────────────────────────────────
export const getDeliveries = () => request('/deliveries');
export const createDelivery = (driverId, locId) =>
  request('/deliveries', {
    method: 'POST',
    body: JSON.stringify({ driverId, locId }),
  });
export const getDeliveriesByDriver = (id) =>
  request(`/deliveries/driver/${id}`);
export const getPendingDeliveriesByDriver = (id) =>
  request(`/deliveries/driver/${id}/pending`);
export const updateDeliveryStatus = (id, status) =>
  request(`/deliveries/${id}/status`, {
    method: 'PUT',
    body: JSON.stringify({ status }),
  });
export const deleteDelivery = (id) =>
  request(`/deliveries/${id}`, { method: 'DELETE' });

// ── Roadmap ──────────────────────────────────────────────────────────────────
export const getRoadmap = () => request('/roadmap');
export const createRoadmap = (sourceId, destId, distance) =>
  request('/roadmap', {
    method: 'POST',
    body: JSON.stringify({ sourceId, destId, distance }),
  });
export const deleteRoadmap = (id) =>
  request(`/roadmap/${id}`, { method: 'DELETE' });
export const reloadGraph = () =>
  request('/roadmap/reload', { method: 'POST' });

// ── Route Optimization ──────────────────────────────────────────────────────
export const optimizeRoute = (driverId) =>
  request(`/route/optimize/${driverId}`);
