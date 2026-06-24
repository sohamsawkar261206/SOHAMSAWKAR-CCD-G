import { useState, useEffect, useCallback } from 'react';
import * as api from '../api';

// ── Sidebar Component ────────────────────────────────────────────────────────
function Sidebar({ user, activeSection, setActiveSection, onLogout }) {
  const sections = [
    { key: 'overview', icon: '📊', label: 'Overview' },
    { key: 'locations', icon: '📍', label: 'Locations' },
    { key: 'roadmap', icon: '🛣️', label: 'Routes' },
    { key: 'drivers', icon: '🚛', label: 'Drivers' },
    { key: 'deliveries', icon: '📦', label: 'Deliveries' },
    { key: 'users', icon: '👥', label: 'Users' },
  ];

  return (
    <aside className="sidebar">
      <div className="sidebar-header">
        <h2>🚛 RouteMaster</h2>
        <span className="role-badge admin">Admin</span>
      </div>
      <nav className="sidebar-nav">
        {sections.map((s) => (
          <button
            key={s.key}
            className={`nav-item ${activeSection === s.key ? 'active' : ''}`}
            onClick={() => setActiveSection(s.key)}
            id={`nav-${s.key}`}
          >
            <span className="nav-icon">{s.icon}</span>
            {s.label}
          </button>
        ))}
      </nav>
      <div className="sidebar-footer">
        <div className="user-info">
          <div className="user-avatar">
            {user.username.charAt(0).toUpperCase()}
          </div>
          <div>
            <div className="user-name">{user.username}</div>
            <div className="user-role">Administrator</div>
          </div>
        </div>
        <button className="logout-btn" onClick={onLogout} id="admin-logout-btn">
          ⏻ Logout
        </button>
      </div>
    </aside>
  );
}

// ── Modal Component ──────────────────────────────────────────────────────────
function Modal({ title, children, onClose }) {
  return (
    <div className="modal-overlay" onClick={onClose}>
      <div className="modal-content" onClick={(e) => e.stopPropagation()}>
        <div className="modal-header">
          <h3>{title}</h3>
          <button className="modal-close" onClick={onClose}>✕</button>
        </div>
        {children}
      </div>
    </div>
  );
}

// ── Status Badge ─────────────────────────────────────────────────────────────
function StatusBadge({ status }) {
  const cls = status.toLowerCase().replace('_', '-');
  return <span className={`status-badge ${cls}`}>● {status}</span>;
}

// ── Elapsed Time Helper ──────────────────────────────────────────────────────
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
  const diffHrs = (new Date() - new Date(assignedTime)) / 3600000;
  return diffHrs >= 4;
}

// ═══════════════════════════════════════════════════════════════════════════
// AdminDashboard — Main Component
// ═══════════════════════════════════════════════════════════════════════════
export default function AdminDashboard({ user, onLogout }) {
  const [activeSection, setActiveSection] = useState('overview');

  return (
    <div className="dashboard-layout">
      <Sidebar
        user={user}
        activeSection={activeSection}
        setActiveSection={setActiveSection}
        onLogout={onLogout}
      />
      <main className="main-content">
        <div className="page-enter" key={activeSection}>
          {activeSection === 'overview' && <OverviewSection />}
          {activeSection === 'locations' && <LocationsSection />}
          {activeSection === 'roadmap' && <RoadmapSection />}
          {activeSection === 'drivers' && <DriversSection />}
          {activeSection === 'deliveries' && <DeliveriesSection />}
          {activeSection === 'users' && <UsersSection />}
        </div>
      </main>
    </div>
  );
}

// ── Overview Section ─────────────────────────────────────────────────────────
function OverviewSection() {
  const [stats, setStats] = useState(null);

  useEffect(() => {
    api.getStats().then(setStats).catch(console.error);
  }, []);

  if (!stats) return <div className="loading">Loading dashboard...</div>;

  return (
    <>
      <div className="page-header">
        <h1>Dashboard Overview</h1>
        <p>Real-time system status and statistics</p>
      </div>
      <div className="stats-grid">
        <div className="stat-card cyan">
          <div className="stat-icon">📍</div>
          <div className="stat-value">{stats.locations}</div>
          <div className="stat-label">Locations</div>
        </div>
        <div className="stat-card purple">
          <div className="stat-icon">🚛</div>
          <div className="stat-value">{stats.drivers}</div>
          <div className="stat-label">Drivers</div>
        </div>
        <div className="stat-card orange">
          <div className="stat-icon">📦</div>
          <div className="stat-value">{stats.pendingDeliveries}</div>
          <div className="stat-label">Pending Deliveries</div>
        </div>
        <div className="stat-card cyan">
          <div className="stat-icon">✅</div>
          <div className="stat-value">{stats.totalDeliveries}</div>
          <div className="stat-label">Total Deliveries</div>
        </div>
        <div className="stat-card purple">
          <div className="stat-icon">🛣️</div>
          <div className="stat-value">{stats.routes}</div>
          <div className="stat-label">Road Segments</div>
        </div>
        <div className="stat-card orange">
          <div className="stat-icon">👥</div>
          <div className="stat-value">{stats.users}</div>
          <div className="stat-label">Users</div>
        </div>
      </div>
    </>
  );
}

// ── Locations Section ────────────────────────────────────────────────────────
function LocationsSection() {
  const [locations, setLocations] = useState([]);
  const [loading, setLoading] = useState(true);
  const [showModal, setShowModal] = useState(false);
  const [form, setForm] = useState({ name: '', latitude: '', longitude: '' });

  const loadLocations = useCallback(() => {
    setLoading(true);
    api.getLocations().then(setLocations).catch(console.error).finally(() => setLoading(false));
  }, []);

  useEffect(() => { loadLocations(); }, [loadLocations]);

  const handleAdd = async (e) => {
    e.preventDefault();
    try {
      await api.createLocation({
        name: form.name,
        latitude: parseFloat(form.latitude),
        longitude: parseFloat(form.longitude),
      });
      setShowModal(false);
      setForm({ name: '', latitude: '', longitude: '' });
      loadLocations();
    } catch (err) {
      alert(err.message);
    }
  };

  const handleDelete = async (id) => {
    if (!confirm('Delete this location? This may affect routes and deliveries.')) return;
    try {
      await api.deleteLocation(id);
      loadLocations();
    } catch (err) {
      alert(err.message);
    }
  };

  return (
    <>
      <div className="page-header">
        <h1>📍 Location Management</h1>
        <p>Manage delivery nodes in the road network</p>
      </div>

      <div className="table-card">
        <div className="table-header">
          <h3>All Locations ({locations.length})</h3>
          <button className="btn btn-primary btn-sm" onClick={() => setShowModal(true)} id="add-location-btn">
            + Add Location
          </button>
        </div>
        {loading ? (
          <div className="loading">Loading locations...</div>
        ) : locations.length === 0 ? (
          <div className="empty-state">
            <div className="empty-icon">📍</div>
            <p>No locations found</p>
          </div>
        ) : (
          <table className="data-table">
            <thead>
              <tr>
                <th>ID</th>
                <th>Name</th>
                <th>Latitude</th>
                <th>Longitude</th>
                <th>Actions</th>
              </tr>
            </thead>
            <tbody>
              {locations.map((loc) => (
                <tr key={loc.loc_id}>
                  <td className="mono">{loc.loc_id}</td>
                  <td>{loc.name}</td>
                  <td className="mono">{parseFloat(loc.latitude).toFixed(7)}</td>
                  <td className="mono">{parseFloat(loc.longitude).toFixed(7)}</td>
                  <td>
                    <button
                      className="btn btn-danger btn-sm"
                      onClick={() => handleDelete(loc.loc_id)}
                    >
                      Delete
                    </button>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        )}
      </div>

      {showModal && (
        <Modal title="Add Location" onClose={() => setShowModal(false)}>
          <form onSubmit={handleAdd}>
            <div className="modal-body">
              <div className="form-group">
                <label>Location Name</label>
                <input className="form-input" value={form.name} onChange={(e) => setForm({ ...form, name: e.target.value })} required />
              </div>
              <div className="form-group">
                <label>Latitude</label>
                <input className="form-input" type="number" step="any" value={form.latitude} onChange={(e) => setForm({ ...form, latitude: e.target.value })} required />
              </div>
              <div className="form-group">
                <label>Longitude</label>
                <input className="form-input" type="number" step="any" value={form.longitude} onChange={(e) => setForm({ ...form, longitude: e.target.value })} required />
              </div>
            </div>
            <div className="modal-footer">
              <button type="button" className="btn btn-secondary" onClick={() => setShowModal(false)}>Cancel</button>
              <button type="submit" className="btn btn-primary btn-sm">Add Location</button>
            </div>
          </form>
        </Modal>
      )}
    </>
  );
}

// ── Roadmap Section ──────────────────────────────────────────────────────────
function RoadmapSection() {
  const [roads, setRoads] = useState([]);
  const [locations, setLocations] = useState([]);
  const [loading, setLoading] = useState(true);
  const [showModal, setShowModal] = useState(false);
  const [form, setForm] = useState({ sourceId: '', destId: '', distance: '' });
  const [reloadMsg, setReloadMsg] = useState('');

  const loadData = useCallback(() => {
    setLoading(true);
    Promise.all([api.getRoadmap(), api.getLocations()])
      .then(([r, l]) => { setRoads(r); setLocations(l); })
      .catch(console.error)
      .finally(() => setLoading(false));
  }, []);

  useEffect(() => { loadData(); }, [loadData]);

  const handleAdd = async (e) => {
    e.preventDefault();
    try {
      await api.createRoadmap(parseInt(form.sourceId), parseInt(form.destId), parseFloat(form.distance));
      setShowModal(false);
      setForm({ sourceId: '', destId: '', distance: '' });
      loadData();
    } catch (err) { alert(err.message); }
  };

  const handleDelete = async (id) => {
    if (!confirm('Delete this route?')) return;
    try { await api.deleteRoadmap(id); loadData(); }
    catch (err) { alert(err.message); }
  };

  const handleReload = async () => {
    try {
      const result = await api.reloadGraph();
      setReloadMsg(result.message);
      setTimeout(() => setReloadMsg(''), 3000);
    } catch (err) { alert(err.message); }
  };

  return (
    <>
      <div className="page-header">
        <h1>🛣️ Route Management</h1>
        <p>Manage road segments in the network graph (bidirectional)</p>
      </div>

      {reloadMsg && (
        <div style={{ background: 'rgba(16, 185, 129, 0.1)', border: '1px solid rgba(16, 185, 129, 0.3)', color: '#6ee7b7', padding: '10px 16px', borderRadius: 8, marginBottom: 16, fontSize: '0.85rem' }}>
          ✓ {reloadMsg}
        </div>
      )}

      <div className="table-card">
        <div className="table-header">
          <h3>Road Segments ({roads.length})</h3>
          <div className="table-actions">
            <button className="btn btn-secondary btn-sm" onClick={handleReload} id="reload-graph-btn">
              ⟳ Reload Graph
            </button>
            <button className="btn btn-primary btn-sm" onClick={() => setShowModal(true)} id="add-route-btn">
              + Add Route
            </button>
          </div>
        </div>
        {loading ? (
          <div className="loading">Loading routes...</div>
        ) : roads.length === 0 ? (
          <div className="empty-state"><div className="empty-icon">🛣️</div><p>No routes found</p></div>
        ) : (
          <table className="data-table">
            <thead>
              <tr>
                <th>Route ID</th>
                <th>Source</th>
                <th>Destination</th>
                <th>Distance (km)</th>
                <th>Actions</th>
              </tr>
            </thead>
            <tbody>
              {roads.map((road) => (
                <tr key={road.route_id}>
                  <td className="mono">{road.route_id}</td>
                  <td>{road.source_name} <span style={{ color: 'var(--text-muted)', fontSize: '0.8rem' }}>(#{road.source_id})</span></td>
                  <td>{road.dest_name} <span style={{ color: 'var(--text-muted)', fontSize: '0.8rem' }}>(#{road.dest_id})</span></td>
                  <td className="mono">{parseFloat(road.distance).toFixed(2)} km</td>
                  <td><button className="btn btn-danger btn-sm" onClick={() => handleDelete(road.route_id)}>Delete</button></td>
                </tr>
              ))}
            </tbody>
          </table>
        )}
      </div>

      {showModal && (
        <Modal title="Add Route" onClose={() => setShowModal(false)}>
          <form onSubmit={handleAdd}>
            <div className="modal-body">
              <div className="form-group">
                <label>Source Location</label>
                <select className="form-select" value={form.sourceId} onChange={(e) => setForm({ ...form, sourceId: e.target.value })} required>
                  <option value="">Select source...</option>
                  {locations.map((l) => <option key={l.loc_id} value={l.loc_id}>{l.name} (#{l.loc_id})</option>)}
                </select>
              </div>
              <div className="form-group">
                <label>Destination Location</label>
                <select className="form-select" value={form.destId} onChange={(e) => setForm({ ...form, destId: e.target.value })} required>
                  <option value="">Select destination...</option>
                  {locations.map((l) => <option key={l.loc_id} value={l.loc_id}>{l.name} (#{l.loc_id})</option>)}
                </select>
              </div>
              <div className="form-group">
                <label>Distance (km)</label>
                <input className="form-input" type="number" step="any" value={form.distance} onChange={(e) => setForm({ ...form, distance: e.target.value })} required />
              </div>
            </div>
            <div className="modal-footer">
              <button type="button" className="btn btn-secondary" onClick={() => setShowModal(false)}>Cancel</button>
              <button type="submit" className="btn btn-primary btn-sm">Add Route</button>
            </div>
          </form>
        </Modal>
      )}
    </>
  );
}

// ── Drivers Section ──────────────────────────────────────────────────────────
function DriversSection() {
  const [drivers, setDrivers] = useState([]);
  const [loading, setLoading] = useState(true);
  const [filter, setFilter] = useState('all');

  const loadDrivers = useCallback(() => {
    setLoading(true);
    const fetchFn = filter === 'available' ? api.getAvailableDrivers : api.getDrivers;
    fetchFn().then(setDrivers).catch(console.error).finally(() => setLoading(false));
  }, [filter]);

  useEffect(() => { loadDrivers(); }, [loadDrivers]);

  return (
    <>
      <div className="page-header">
        <h1>🚛 Driver Management</h1>
        <p>View and manage registered drivers</p>
      </div>

      <div className="section-tabs">
        <button className={`section-tab ${filter === 'all' ? 'active' : ''}`} onClick={() => setFilter('all')}>All Drivers</button>
        <button className={`section-tab ${filter === 'available' ? 'active' : ''}`} onClick={() => setFilter('available')}>Available Only</button>
      </div>

      <div className="table-card">
        <div className="table-header">
          <h3>Drivers ({drivers.length})</h3>
        </div>
        {loading ? (
          <div className="loading">Loading drivers...</div>
        ) : drivers.length === 0 ? (
          <div className="empty-state"><div className="empty-icon">🚛</div><p>No drivers found</p></div>
        ) : (
          <table className="data-table">
            <thead>
              <tr>
                <th>Driver ID</th>
                <th>Username</th>
                <th>License</th>
                <th>Capacity</th>
                <th>Status</th>
                <th>Location</th>
              </tr>
            </thead>
            <tbody>
              {drivers.map((d) => (
                <tr key={d.driver_id}>
                  <td className="mono">{d.driver_id}</td>
                  <td>{d.username}</td>
                  <td className="mono">{d.license_no}</td>
                  <td className="mono">{d.capacity}</td>
                  <td><StatusBadge status={d.status} /></td>
                  <td>{d.location_name || `Location #${d.current_loc_id}`}</td>
                </tr>
              ))}
            </tbody>
          </table>
        )}
      </div>
    </>
  );
}

// ── Deliveries Section ───────────────────────────────────────────────────────
function DeliveriesSection() {
  const [deliveries, setDeliveries] = useState([]);
  const [drivers, setDrivers] = useState([]);
  const [locations, setLocations] = useState([]);
  const [loading, setLoading] = useState(true);
  const [showModal, setShowModal] = useState(false);
  const [form, setForm] = useState({ driverId: '', locId: '' });

  const loadData = useCallback(() => {
    setLoading(true);
    Promise.all([api.getDeliveries(), api.getDrivers(), api.getLocations()])
      .then(([d, dr, l]) => { setDeliveries(d); setDrivers(dr); setLocations(l); })
      .catch(console.error)
      .finally(() => setLoading(false));
  }, []);

  useEffect(() => { loadData(); }, [loadData]);

  const handleAssign = async (e) => {
    e.preventDefault();
    try {
      await api.createDelivery(parseInt(form.driverId), parseInt(form.locId));
      setShowModal(false);
      setForm({ driverId: '', locId: '' });
      loadData();
    } catch (err) { alert(err.message); }
  };

  const handleDelete = async (id) => {
    if (!confirm('Delete this delivery?')) return;
    try { await api.deleteDelivery(id); loadData(); }
    catch (err) { alert(err.message); }
  };

  const overdueCount = deliveries.filter((d) => d.status === 'Pending' && isOverdue(d.assigned_time)).length;

  return (
    <>
      <div className="page-header">
        <h1>📦 Delivery Management</h1>
        <p>Assign and track package deliveries</p>
      </div>

      {overdueCount > 0 && (
        <div style={{ background: 'rgba(239, 68, 68, 0.1)', border: '1px solid rgba(239, 68, 68, 0.3)', color: '#fca5a5', padding: '10px 16px', borderRadius: 8, marginBottom: 16, fontSize: '0.85rem' }}>
          ⚠ ALERT: {overdueCount} pending delivery(ies) are OVERDUE!
        </div>
      )}

      <div className="table-card">
        <div className="table-header">
          <h3>All Deliveries ({deliveries.length})</h3>
          <button className="btn btn-primary btn-sm" onClick={() => setShowModal(true)} id="assign-delivery-btn">
            + Assign Delivery
          </button>
        </div>
        {loading ? (
          <div className="loading">Loading deliveries...</div>
        ) : deliveries.length === 0 ? (
          <div className="empty-state"><div className="empty-icon">📦</div><p>No deliveries found</p></div>
        ) : (
          <table className="data-table">
            <thead>
              <tr>
                <th>ID</th>
                <th>Driver</th>
                <th>Destination</th>
                <th>Status</th>
                <th>Assigned</th>
                <th>Elapsed</th>
                <th>Actions</th>
              </tr>
            </thead>
            <tbody>
              {deliveries.map((d) => {
                const overdue = d.status === 'Pending' && isOverdue(d.assigned_time);
                return (
                  <tr key={d.delivery_id}>
                    <td className="mono">{d.delivery_id}</td>
                    <td>{d.driver_name} <span style={{ color: 'var(--text-muted)', fontSize: '0.8rem' }}>(#{d.driver_id})</span></td>
                    <td>{d.location_name} <span style={{ color: 'var(--text-muted)', fontSize: '0.8rem' }}>(#{d.loc_id})</span></td>
                    <td><StatusBadge status={d.status} /></td>
                    <td style={{ fontSize: '0.8rem', color: 'var(--text-secondary)' }}>{d.assigned_time ? new Date(d.assigned_time).toLocaleString() : 'N/A'}</td>
                    <td>
                      <span className={overdue ? 'status-badge overdue' : ''}>
                        {overdue && '⚠ '}{getElapsed(d.assigned_time)}
                      </span>
                    </td>
                    <td><button className="btn btn-danger btn-sm" onClick={() => handleDelete(d.delivery_id)}>Delete</button></td>
                  </tr>
                );
              })}
            </tbody>
          </table>
        )}
      </div>

      {showModal && (
        <Modal title="Assign Delivery" onClose={() => setShowModal(false)}>
          <form onSubmit={handleAssign}>
            <div className="modal-body">
              <div className="form-group">
                <label>Driver</label>
                <select className="form-select" value={form.driverId} onChange={(e) => setForm({ ...form, driverId: e.target.value })} required>
                  <option value="">Select driver...</option>
                  {drivers.map((d) => <option key={d.driver_id} value={d.driver_id}>{d.username} (#{d.driver_id}) — {d.status}</option>)}
                </select>
              </div>
              <div className="form-group">
                <label>Destination Location</label>
                <select className="form-select" value={form.locId} onChange={(e) => setForm({ ...form, locId: e.target.value })} required>
                  <option value="">Select location...</option>
                  {locations.map((l) => <option key={l.loc_id} value={l.loc_id}>{l.name} (#{l.loc_id})</option>)}
                </select>
              </div>
            </div>
            <div className="modal-footer">
              <button type="button" className="btn btn-secondary" onClick={() => setShowModal(false)}>Cancel</button>
              <button type="submit" className="btn btn-primary btn-sm">Assign Delivery</button>
            </div>
          </form>
        </Modal>
      )}
    </>
  );
}

// ── Users Section ────────────────────────────────────────────────────────────
function UsersSection() {
  const [users, setUsers] = useState([]);
  const [loading, setLoading] = useState(true);
  const [showModal, setShowModal] = useState(false);
  const [form, setForm] = useState({ username: '', password: '', role: 'Admin', licenseNo: '', capacity: 5 });

  const loadUsers = useCallback(() => {
    setLoading(true);
    api.getUsers().then(setUsers).catch(console.error).finally(() => setLoading(false));
  }, []);

  useEffect(() => { loadUsers(); }, [loadUsers]);

  const handleRegister = async (e) => {
    e.preventDefault();
    try {
      await api.createUser(form);
      setShowModal(false);
      setForm({ username: '', password: '', role: 'Admin', licenseNo: '', capacity: 5 });
      loadUsers();
    } catch (err) { alert(err.message); }
  };

  const handleDelete = async (id) => {
    if (!confirm('Delete this user? If this is a driver, all completed delivery records will also be removed.')) return;
    try { await api.deleteUser(id); loadUsers(); }
    catch (err) { alert(err.message); }
  };

  return (
    <>
      <div className="page-header">
        <h1>👥 User Management</h1>
        <p>View registered users and add new accounts</p>
      </div>

      <div className="table-card">
        <div className="table-header">
          <h3>All Users ({users.length})</h3>
          <button className="btn btn-primary btn-sm" onClick={() => setShowModal(true)} id="register-user-btn">
            + Register User
          </button>
        </div>
        {loading ? (
          <div className="loading">Loading users...</div>
        ) : (
          <table className="data-table">
            <thead>
              <tr>
                <th>User ID</th>
                <th>Username</th>
                <th>Role</th>
                <th>Actions</th>
              </tr>
            </thead>
            <tbody>
              {users.map((u) => (
                <tr key={u.user_id}>
                  <td className="mono">{u.user_id}</td>
                  <td>{u.username}</td>
                  <td><span className={`role-badge ${u.role.toLowerCase()}`}>{u.role}</span></td>
                  <td><button className="btn btn-danger btn-sm" onClick={() => handleDelete(u.user_id)}>Delete</button></td>
                </tr>
              ))}
            </tbody>
          </table>
        )}
      </div>

      {showModal && (
        <Modal title="Register New User" onClose={() => setShowModal(false)}>
          <form onSubmit={handleRegister}>
            <div className="modal-body">
              <div className="form-group">
                <label>Username</label>
                <input className="form-input" value={form.username} onChange={(e) => setForm({ ...form, username: e.target.value })} required />
              </div>
              <div className="form-group">
                <label>Password</label>
                <input className="form-input" type="password" value={form.password} onChange={(e) => setForm({ ...form, password: e.target.value })} required />
              </div>
              <div className="form-group">
                <label>Role</label>
                <select className="form-select" value={form.role} onChange={(e) => setForm({ ...form, role: e.target.value })}>
                  <option value="Admin">Admin</option>
                  <option value="Driver">Driver</option>
                </select>
              </div>
              {form.role === 'Driver' && (
                <>
                  <div className="form-group">
                    <label>License Number</label>
                    <input className="form-input" value={form.licenseNo} onChange={(e) => setForm({ ...form, licenseNo: e.target.value })} required />
                  </div>
                  <div className="form-group">
                    <label>Carrying Capacity</label>
                    <input className="form-input" type="number" value={form.capacity} onChange={(e) => setForm({ ...form, capacity: parseInt(e.target.value) })} required />
                  </div>
                </>
              )}
            </div>
            <div className="modal-footer">
              <button type="button" className="btn btn-secondary" onClick={() => setShowModal(false)}>Cancel</button>
              <button type="submit" className="btn btn-primary btn-sm">Register</button>
            </div>
          </form>
        </Modal>
      )}
    </>
  );
}
