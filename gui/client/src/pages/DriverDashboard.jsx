import { useState, useEffect, useCallback } from 'react';
import * as api from '../api';

// ── Helpers ──────────────────────────────────────────────────────────────────
function getElapsed(assignedTime) {
  if (!assignedTime) return 'N/A';
  const assigned = new Date(assignedTime);
  const now = new Date();
  const diffMs = now - assigned;
  if (diffMs < 60000) return 'Just now';
  const totalMin = Math.floor(diffMs / 60000);
  const hours = Math.floor(totalMin / 60);
  const minutes = totalMin % 60;
  const days = Math.floor(hours / 24);
  if (days > 0) return `${days}d ${hours % 24}h ago`;
  if (hours > 0) return `${hours}h ${minutes}m ago`;
  return `${minutes}m ago`;
}

function isOverdue(assignedTime) {
  if (!assignedTime) return false;
  return (new Date() - new Date(assignedTime)) / 3600000 >= 4;
}

function StatusBadge({ status }) {
  const cls = status.toLowerCase().replace('_', '-');
  return <span className={`status-badge ${cls}`}>● {status}</span>;
}

// ═══════════════════════════════════════════════════════════════════════════
// DriverDashboard
// ═══════════════════════════════════════════════════════════════════════════
export default function DriverDashboard({ user, onLogout }) {
  const [activeSection, setActiveSection] = useState('deliveries');

  const sections = [
    { key: 'deliveries', icon: '📦', label: 'My Deliveries' },
    { key: 'optimize', icon: '🗺️', label: 'Optimize Route' },
    { key: 'complete', icon: '✅', label: 'Complete Delivery' },
    { key: 'profile', icon: '👤', label: 'My Profile' },
  ];

  return (
    <div className="dashboard-layout">
      <aside className="sidebar">
        <div className="sidebar-header">
          <h2>🚛 RouteMaster</h2>
          <span className="role-badge driver">Driver</span>
        </div>
        <nav className="sidebar-nav">
          {sections.map((s) => (
            <button
              key={s.key}
              className={`nav-item ${activeSection === s.key ? 'active' : ''}`}
              onClick={() => setActiveSection(s.key)}
              id={`driver-nav-${s.key}`}
            >
              <span className="nav-icon">{s.icon}</span>
              {s.label}
            </button>
          ))}
        </nav>
        <div className="sidebar-footer">
          <div className="user-info">
            <div className="user-avatar">{user.username.charAt(0).toUpperCase()}</div>
            <div>
              <div className="user-name">{user.username}</div>
              <div className="user-role">Driver</div>
            </div>
          </div>
          <button className="logout-btn" onClick={onLogout} id="driver-logout-btn">⏻ Logout</button>
        </div>
      </aside>

      <main className="main-content">
        <div className="page-enter" key={activeSection}>
          {activeSection === 'deliveries' && <PendingDeliveries userId={user.userId} />}
          {activeSection === 'optimize' && <OptimizeRoute userId={user.userId} />}
          {activeSection === 'complete' && <CompleteDelivery userId={user.userId} />}
          {activeSection === 'profile' && <DriverProfile userId={user.userId} />}
        </div>
      </main>
    </div>
  );
}

// ── Pending Deliveries ───────────────────────────────────────────────────────
function PendingDeliveries({ userId }) {
  const [deliveries, setDeliveries] = useState([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    api.getPendingDeliveriesByDriver(userId)
      .then(setDeliveries)
      .catch(console.error)
      .finally(() => setLoading(false));
  }, [userId]);

  const overdueCount = deliveries.filter((d) => isOverdue(d.assigned_time)).length;

  return (
    <>
      <div className="page-header">
        <h1>📦 My Pending Deliveries</h1>
        <p>Packages assigned to you for delivery</p>
      </div>

      {overdueCount > 0 && (
        <div style={{ background: 'rgba(239, 68, 68, 0.1)', border: '1px solid rgba(239, 68, 68, 0.3)', color: '#fca5a5', padding: '10px 16px', borderRadius: 8, marginBottom: 16, fontSize: '0.85rem' }}>
          ⚠ WARNING: {overdueCount} delivery(ies) OVERDUE (pending &gt; 4 hours)!
        </div>
      )}

      <div className="table-card">
        <div className="table-header">
          <h3>Pending ({deliveries.length})</h3>
        </div>
        {loading ? (
          <div className="loading">Loading deliveries...</div>
        ) : deliveries.length === 0 ? (
          <div className="empty-state">
            <div className="empty-icon">✅</div>
            <p>No pending deliveries assigned to you.</p>
          </div>
        ) : (
          <table className="data-table">
            <thead>
              <tr>
                <th>Delivery ID</th>
                <th>Destination</th>
                <th>Status</th>
                <th>Assigned</th>
                <th>Elapsed</th>
              </tr>
            </thead>
            <tbody>
              {deliveries.map((d) => {
                const overdue = isOverdue(d.assigned_time);
                return (
                  <tr key={d.delivery_id}>
                    <td className="mono">{d.delivery_id}</td>
                    <td>{d.location_name} <span style={{ color: 'var(--text-muted)', fontSize: '0.8rem' }}>(#{d.loc_id})</span></td>
                    <td><StatusBadge status={d.status} /></td>
                    <td style={{ fontSize: '0.8rem', color: 'var(--text-secondary)' }}>{d.assigned_time ? new Date(d.assigned_time).toLocaleString() : 'N/A'}</td>
                    <td>
                      <span className={overdue ? 'status-badge overdue' : ''}>
                        {overdue && '⚠ '}{getElapsed(d.assigned_time)}
                      </span>
                    </td>
                  </tr>
                );
              })}
            </tbody>
          </table>
        )}
      </div>
    </>
  );
}

// ── Route Optimization ───────────────────────────────────────────────────────
function OptimizeRoute({ userId }) {
  const [result, setResult] = useState(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState('');

  const handleOptimize = async () => {
    setLoading(true);
    setError('');
    setResult(null);
    try {
      const data = await api.optimizeRoute(userId);
      setResult(data);
      // Set driver status to On_Route
      await api.updateDriverStatus(userId, 'On_Route');
    } catch (err) {
      setError(err.message);
    } finally {
      setLoading(false);
    }
  };

  return (
    <>
      <div className="page-header">
        <h1>🗺️ Route Optimization Engine</h1>
        <p>Floyd-Warshall → TSP Branch &amp; Bound → Dijkstra pipeline</p>
      </div>

      <div className="route-panel">
        <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', marginBottom: 16 }}>
          <h3>⚡ Algorithm Pipeline</h3>
          <button
            className="btn btn-primary btn-sm"
            onClick={handleOptimize}
            disabled={loading}
            id="optimize-route-btn"
          >
            {loading ? '⏳ Computing...' : '▶ Optimize My Route'}
          </button>
        </div>

        {loading && (
          <div style={{ padding: 20 }}>
            <div style={{ color: 'var(--text-accent)', marginBottom: 8, fontSize: '0.85rem' }}>⟳ [1/3] Floyd-Warshall distances: pre-computed ✓</div>
            <div style={{ color: 'var(--text-accent)', marginBottom: 8, fontSize: '0.85rem' }}>⟳ [2/3] TSP Branch &amp; Bound: computing optimal sequence...</div>
            <div style={{ color: 'var(--text-muted)', fontSize: '0.85rem' }}>⏳ [3/3] Dijkstra navigation: waiting...</div>
          </div>
        )}

        {error && (
          <div style={{ background: 'rgba(239, 68, 68, 0.1)', border: '1px solid rgba(239, 68, 68, 0.3)', color: '#fca5a5', padding: '10px 16px', borderRadius: 8, fontSize: '0.85rem' }}>
            ✕ {error}
          </div>
        )}

        {result && result.success && (
          <div style={{ animation: 'pageEnter 0.4s ease' }}>
            {/* Algorithm Status */}
            <div style={{ padding: '12px 16px', background: 'rgba(16, 185, 129, 0.05)', borderRadius: 8, marginBottom: 16 }}>
              <div style={{ color: '#6ee7b7', fontSize: '0.85rem', marginBottom: 4 }}>✓ [1/3] Floyd-Warshall distances: pre-computed [DONE]</div>
              <div style={{ color: '#6ee7b7', fontSize: '0.85rem', marginBottom: 4 }}>✓ [2/3] TSP Branch &amp; Bound: optimal sequence found [DONE]</div>
              <div style={{ color: '#6ee7b7', fontSize: '0.85rem' }}>✓ [3/3] Dijkstra navigation: paths computed [DONE]</div>
            </div>

            {/* Optimal Sequence */}
            <h3 style={{ marginBottom: 12 }}>📍 Optimal Delivery Sequence</h3>
            <div className="route-sequence">
              {result.tspSequence.map((loc, i) => (
                <span key={i} style={{ display: 'contents' }}>
                  <span className="route-node">{loc.name}</span>
                  {i < result.tspSequence.length - 1 && <span className="route-arrow">→</span>}
                </span>
              ))}
            </div>

            {/* Turn-by-Turn Navigation */}
            <h3 style={{ marginTop: 20, marginBottom: 12 }}>🧭 Turn-by-Turn Navigation (Dijkstra)</h3>
            {result.legs.map((leg, i) => (
              <div className="route-leg" key={i}>
                <h4>
                  LEG {i + 1}: {leg.fromName} → {leg.toName}
                </h4>
                <div className="leg-distance">{leg.distance.toFixed(2)} km</div>
                <div className="leg-path">
                  Path: {leg.pathNames.join(' → ')}
                </div>
              </div>
            ))}

            {/* Total */}
            <div className="route-total">
              <span className="total-label">🏁 Total Optimized Distance</span>
              <span className="total-value">{result.totalDistance.toFixed(2)} km</span>
            </div>

            <div style={{ textAlign: 'center', marginTop: 16, color: 'var(--text-secondary)', fontSize: '0.85rem' }}>
              ✓ Driver status updated to 'On_Route'. Drive safe!
            </div>
          </div>
        )}

        {!loading && !result && !error && (
          <div className="empty-state">
            <div className="empty-icon">🗺️</div>
            <p>Click "Optimize My Route" to compute the optimal delivery path</p>
            <p style={{ marginTop: 4, fontSize: '0.8rem', color: 'var(--text-muted)' }}>
              Uses Floyd-Warshall (DP) + TSP B&amp;B + Dijkstra (Greedy)
            </p>
          </div>
        )}
      </div>
    </>
  );
}

// ── Complete Delivery ────────────────────────────────────────────────────────
function CompleteDelivery({ userId }) {
  const [deliveries, setDeliveries] = useState([]);
  const [loading, setLoading] = useState(true);
  const [completing, setCompleting] = useState(null);
  const [message, setMessage] = useState('');

  const loadDeliveries = useCallback(() => {
    setLoading(true);
    api.getPendingDeliveriesByDriver(userId)
      .then(setDeliveries)
      .catch(console.error)
      .finally(() => setLoading(false));
  }, [userId]);

  useEffect(() => { loadDeliveries(); }, [loadDeliveries]);

  const handleComplete = async (deliveryId) => {
    setCompleting(deliveryId);
    try {
      await api.updateDeliveryStatus(deliveryId, 'Completed');
      setMessage(`✓ Delivery #${deliveryId} marked as Completed!`);
      setTimeout(() => setMessage(''), 4000);
      loadDeliveries();
    } catch (err) {
      alert(err.message);
    } finally {
      setCompleting(null);
    }
  };

  return (
    <>
      <div className="page-header">
        <h1>✅ Complete Delivery</h1>
        <p>Mark your pending deliveries as completed</p>
      </div>

      {message && (
        <div style={{ background: 'rgba(16, 185, 129, 0.1)', border: '1px solid rgba(16, 185, 129, 0.3)', color: '#6ee7b7', padding: '10px 16px', borderRadius: 8, marginBottom: 16, fontSize: '0.85rem' }}>
          {message}
        </div>
      )}

      <div className="table-card">
        <div className="table-header">
          <h3>Pending Deliveries ({deliveries.length})</h3>
        </div>
        {loading ? (
          <div className="loading">Loading...</div>
        ) : deliveries.length === 0 ? (
          <div className="empty-state">
            <div className="empty-icon">🎉</div>
            <p>All deliveries completed! Great work!</p>
          </div>
        ) : (
          <table className="data-table">
            <thead>
              <tr>
                <th>Delivery ID</th>
                <th>Destination</th>
                <th>Elapsed</th>
                <th>Action</th>
              </tr>
            </thead>
            <tbody>
              {deliveries.map((d) => (
                <tr key={d.delivery_id}>
                  <td className="mono">{d.delivery_id}</td>
                  <td>{d.location_name} <span style={{ color: 'var(--text-muted)', fontSize: '0.8rem' }}>(#{d.loc_id})</span></td>
                  <td>{getElapsed(d.assigned_time)}</td>
                  <td>
                    <button
                      className="btn btn-primary btn-sm"
                      onClick={() => handleComplete(d.delivery_id)}
                      disabled={completing === d.delivery_id}
                    >
                      {completing === d.delivery_id ? '...' : '✓ Complete'}
                    </button>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        )}
      </div>
    </>
  );
}

// ── Driver Profile ───────────────────────────────────────────────────────────
function DriverProfile({ userId }) {
  const [driver, setDriver] = useState(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    api.getDriverById(userId)
      .then(setDriver)
      .catch(console.error)
      .finally(() => setLoading(false));
  }, [userId]);

  if (loading) return <div className="loading">Loading profile...</div>;
  if (!driver) return <div className="empty-state"><p>Driver profile not found.</p></div>;

  return (
    <>
      <div className="page-header">
        <h1>👤 My Profile</h1>
        <p>Your driver details and current status</p>
      </div>

      <div className="profile-card">
        <div className="profile-header">
          <div className="profile-avatar">{driver.username.charAt(0).toUpperCase()}</div>
          <div className="profile-info">
            <h3>{driver.username}</h3>
            <p>Driver ID: #{driver.driver_id}</p>
          </div>
        </div>
        <div className="profile-fields">
          <div className="profile-field">
            <span className="field-label">License No</span>
            <span className="field-value">{driver.license_no}</span>
          </div>
          <div className="profile-field">
            <span className="field-label">Capacity</span>
            <span className="field-value">{driver.capacity} packages</span>
          </div>
          <div className="profile-field">
            <span className="field-label">Status</span>
            <StatusBadge status={driver.status} />
          </div>
          <div className="profile-field">
            <span className="field-label">Current Location</span>
            <span className="field-value">{driver.location_name || `#${driver.current_loc_id}`}</span>
          </div>
        </div>
      </div>
    </>
  );
}
