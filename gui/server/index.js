// ============================================================================
// index.js — RouteMaster Express REST API Server
//
// Mirrors every DatabaseManager.h method as a REST endpoint.
// Connects to the same routemaster_db that the C++ TUI uses.
// ============================================================================

require('dotenv').config();
const express = require('express');
const cors = require('cors');
const mysql = require('mysql2/promise');
const RouteOptimizer = require('./routeOptimizer');

const app = express();
app.use(cors());
app.use(express.json());

// ── MySQL Connection Pool ────────────────────────────────────────────────────
const pool = mysql.createPool({
  host: process.env.DB_HOST,
  user: process.env.DB_USER,
  password: process.env.DB_PASSWORD,
  database: process.env.DB_NAME,
  port: parseInt(process.env.DB_PORT) || 3306,
  waitForConnections: true,
  connectionLimit: 10,
});

// ── Route Optimizer Instance ─────────────────────────────────────────────────
const optimizer = new RouteOptimizer();

// Build adjacency matrix from ROADMAP table and run Floyd-Warshall
async function initializeOptimizer() {
  try {
    const [locRows] = await pool.query('SELECT COUNT(*) AS cnt FROM LOCATION');
    const locCount = locRows[0].cnt;

    const [roadRows] = await pool.query('SELECT * FROM ROADMAP');

    // Build (N+1) x (N+1) matrix, 1-indexed
    const INF = 1e9;
    const matrix = Array.from({ length: locCount + 1 }, () =>
      Array(locCount + 1).fill(INF)
    );
    for (let i = 0; i <= locCount; i++) matrix[i][i] = 0;

    // Populate bidirectional edges
    for (const road of roadRows) {
      const src = road.source_id;
      const dst = road.dest_id;
      const dist = parseFloat(road.distance);
      if (src <= locCount && dst <= locCount) {
        matrix[src][dst] = dist;
        matrix[dst][src] = dist;
      }
    }

    optimizer.initialize(matrix, locCount);
    optimizer.floydWarshall();
    console.log(`[Boot] Floyd-Warshall computed for ${locCount} nodes, ${roadRows.length} edges.`);
  } catch (err) {
    console.error('[Boot] Failed to initialize optimizer:', err.message);
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// AUTH
// ═══════════════════════════════════════════════════════════════════════════

app.post('/api/auth/login', async (req, res) => {
  try {
    const { username, password } = req.body;
    const [rows] = await pool.query(
      'SELECT * FROM USER WHERE username = ? AND password = ?',
      [username, password]
    );
    if (rows.length === 0) {
      return res.status(401).json({ error: 'Invalid credentials' });
    }
    const user = rows[0];
    res.json({
      userId: user.user_id,
      username: user.username,
      role: user.role,
    });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// ═══════════════════════════════════════════════════════════════════════════
// USERS
// ═══════════════════════════════════════════════════════════════════════════

app.get('/api/users', async (_req, res) => {
  try {
    const [rows] = await pool.query('SELECT user_id, username, role FROM USER ORDER BY user_id');
    res.json(rows);
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.post('/api/users', async (req, res) => {
  try {
    const { username, password, role, licenseNo, capacity } = req.body;
    const [result] = await pool.query(
      'INSERT INTO USER (username, password, role) VALUES (?, ?, ?)',
      [username, password, role]
    );
    const userId = result.insertId;

    // If registering a driver, create driver details too
    if (role === 'Driver' && licenseNo) {
      await pool.query(
        'INSERT INTO DRIVER_DETAILS (driver_id, license_no, capacity, status, current_loc_id) VALUES (?, ?, ?, ?, ?)',
        [userId, licenseNo, capacity || 5, 'Available', 1]
      );
    }

    res.json({ success: true, userId });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.delete('/api/users/:id', async (req, res) => {
  try {
    const userId = req.params.id;

    // ── Guard: refuse deletion of the last admin ─────────────────────────
    const [userRows] = await pool.query(
      'SELECT role FROM USER WHERE user_id = ?',
      [userId]
    );
    if (userRows.length > 0 && userRows[0].role === 'Admin') {
      const [adminCount] = await pool.query(
        "SELECT COUNT(*) AS cnt FROM USER WHERE role = 'Admin'"
      );
      if (adminCount[0].cnt <= 1) {
        return res.status(400).json({
          error: 'Cannot delete the last admin. At least one admin account must remain.',
        });
      }
    }

    // ── Guard: refuse deletion if driver has pending deliveries ───────────
    const [driverRows] = await pool.query(
      'SELECT * FROM DRIVER_DETAILS WHERE driver_id = ?',
      [userId]
    );

    if (driverRows.length > 0) {
      const [pendingRows] = await pool.query(
        "SELECT COUNT(*) AS cnt FROM DELIVERY WHERE driver_id = ? AND status = 'Pending'",
        [userId]
      );
      const pendingCount = pendingRows[0].cnt;

      if (pendingCount > 0) {
        return res.status(400).json({
          error: `Cannot delete driver: ${pendingCount} pending delivery(ies) still assigned. Complete or reassign them first.`,
        });
      }
    }

    const [result] = await pool.query('DELETE FROM USER WHERE user_id = ?', [userId]);
    res.json({ success: result.affectedRows > 0 });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// ═══════════════════════════════════════════════════════════════════════════
// LOCATIONS
// ═══════════════════════════════════════════════════════════════════════════

app.get('/api/locations', async (_req, res) => {
  try {
    const [rows] = await pool.query('SELECT * FROM LOCATION ORDER BY loc_id');
    res.json(rows);
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.post('/api/locations', async (req, res) => {
  try {
    const { name, latitude, longitude } = req.body;
    const [result] = await pool.query(
      'INSERT INTO LOCATION (name, latitude, longitude) VALUES (?, ?, ?)',
      [name, latitude, longitude]
    );
    res.json({ success: true, locId: result.insertId });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.delete('/api/locations/:id', async (req, res) => {
  try {
    const [result] = await pool.query('DELETE FROM LOCATION WHERE loc_id = ?', [req.params.id]);
    res.json({ success: result.affectedRows > 0 });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// ═══════════════════════════════════════════════════════════════════════════
// DRIVERS
// ═══════════════════════════════════════════════════════════════════════════

app.get('/api/drivers', async (_req, res) => {
  try {
    const [rows] = await pool.query(
      `SELECT d.*, u.username, l.name AS location_name 
       FROM DRIVER_DETAILS d 
       JOIN USER u ON d.driver_id = u.user_id 
       LEFT JOIN LOCATION l ON d.current_loc_id = l.loc_id
       ORDER BY d.driver_id`
    );
    res.json(rows);
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.get('/api/drivers/available', async (_req, res) => {
  try {
    const [rows] = await pool.query(
      `SELECT d.*, u.username, l.name AS location_name 
       FROM DRIVER_DETAILS d 
       JOIN USER u ON d.driver_id = u.user_id 
       LEFT JOIN LOCATION l ON d.current_loc_id = l.loc_id
       WHERE d.status = 'Available'`
    );
    res.json(rows);
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.get('/api/drivers/:id', async (req, res) => {
  try {
    const [rows] = await pool.query(
      `SELECT d.*, u.username, l.name AS location_name 
       FROM DRIVER_DETAILS d 
       JOIN USER u ON d.driver_id = u.user_id 
       LEFT JOIN LOCATION l ON d.current_loc_id = l.loc_id
       WHERE d.driver_id = ?`,
      [req.params.id]
    );
    if (rows.length === 0) return res.status(404).json({ error: 'Driver not found' });
    res.json(rows[0]);
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.put('/api/drivers/:id/status', async (req, res) => {
  try {
    const { status } = req.body;
    const [result] = await pool.query(
      'UPDATE DRIVER_DETAILS SET status = ? WHERE driver_id = ?',
      [status, req.params.id]
    );
    res.json({ success: result.affectedRows > 0 });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// ═══════════════════════════════════════════════════════════════════════════
// DELIVERIES
// ═══════════════════════════════════════════════════════════════════════════

app.get('/api/deliveries', async (_req, res) => {
  try {
    const [rows] = await pool.query(
      `SELECT del.*, l.name AS location_name, u.username AS driver_name
       FROM DELIVERY del
       JOIN LOCATION l ON del.loc_id = l.loc_id
       JOIN USER u ON del.driver_id = u.user_id
       ORDER BY del.delivery_id`
    );
    res.json(rows);
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.post('/api/deliveries', async (req, res) => {
  try {
    const { driverId, locId } = req.body;

    // Capacity check (matches C++ logic)
    const [driverRows] = await pool.query(
      'SELECT * FROM DRIVER_DETAILS WHERE driver_id = ?',
      [driverId]
    );
    if (driverRows.length === 0) {
      return res.status(404).json({ error: 'Driver not found' });
    }

    const [countRows] = await pool.query(
      "SELECT COUNT(*) AS cnt FROM DELIVERY WHERE driver_id = ? AND status = 'Pending'",
      [driverId]
    );
    const pendingCount = countRows[0].cnt;
    const capacity = driverRows[0].capacity;

    if (pendingCount >= capacity) {
      return res.status(400).json({
        error: `Driver at max capacity (${pendingCount}/${capacity} slots used)`,
      });
    }

    const [result] = await pool.query(
      "INSERT INTO DELIVERY (driver_id, loc_id, status) VALUES (?, ?, 'Pending')",
      [driverId, locId]
    );
    res.json({
      success: true,
      deliveryId: result.insertId,
      slotsUsed: `${pendingCount + 1}/${capacity}`,
    });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.get('/api/deliveries/driver/:id', async (req, res) => {
  try {
    const [rows] = await pool.query(
      `SELECT del.*, l.name AS location_name 
       FROM DELIVERY del 
       JOIN LOCATION l ON del.loc_id = l.loc_id
       WHERE del.driver_id = ? 
       ORDER BY del.delivery_id`,
      [req.params.id]
    );
    res.json(rows);
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.get('/api/deliveries/driver/:id/pending', async (req, res) => {
  try {
    const [rows] = await pool.query(
      `SELECT del.*, l.name AS location_name 
       FROM DELIVERY del 
       JOIN LOCATION l ON del.loc_id = l.loc_id
       WHERE del.driver_id = ? AND del.status = 'Pending' 
       ORDER BY del.delivery_id`,
      [req.params.id]
    );
    res.json(rows);
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.put('/api/deliveries/:id/status', async (req, res) => {
  try {
    const { status } = req.body;
    const deliveryId = req.params.id;

    const [result] = await pool.query(
      'UPDATE DELIVERY SET status = ? WHERE delivery_id = ?',
      [status, deliveryId]
    );

    if (status === 'Completed') {
      // Get the delivery to find its loc_id and driver_id
      const [delRows] = await pool.query(
        'SELECT * FROM DELIVERY WHERE delivery_id = ?',
        [deliveryId]
      );
      if (delRows.length > 0) {
        const del = delRows[0];
        // Update driver location to delivery destination
        await pool.query(
          'UPDATE DRIVER_DETAILS SET current_loc_id = ? WHERE driver_id = ?',
          [del.loc_id, del.driver_id]
        );

        // Check if all deliveries done → set Available + return to warehouse
        const [remaining] = await pool.query(
          "SELECT COUNT(*) AS cnt FROM DELIVERY WHERE driver_id = ? AND status = 'Pending'",
          [del.driver_id]
        );
        if (remaining[0].cnt === 0) {
          await pool.query(
            "UPDATE DRIVER_DETAILS SET status = 'Available', current_loc_id = 1 WHERE driver_id = ?",
            [del.driver_id]
          );
        }
      }
    }

    res.json({ success: result.affectedRows > 0 });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.delete('/api/deliveries/:id', async (req, res) => {
  try {
    const [result] = await pool.query('DELETE FROM DELIVERY WHERE delivery_id = ?', [req.params.id]);
    res.json({ success: result.affectedRows > 0 });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// ═══════════════════════════════════════════════════════════════════════════
// ROADMAP
// ═══════════════════════════════════════════════════════════════════════════

app.get('/api/roadmap', async (_req, res) => {
  try {
    const [rows] = await pool.query(
      `SELECT r.*, ls.name AS source_name, ld.name AS dest_name
       FROM ROADMAP r
       JOIN LOCATION ls ON r.source_id = ls.loc_id
       JOIN LOCATION ld ON r.dest_id = ld.loc_id
       ORDER BY r.route_id`
    );
    res.json(rows);
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.post('/api/roadmap', async (req, res) => {
  try {
    const { sourceId, destId, distance } = req.body;
    const [result] = await pool.query(
      'INSERT INTO ROADMAP (source_id, dest_id, distance) VALUES (?, ?, ?)',
      [sourceId, destId, distance]
    );
    res.json({ success: true, routeId: result.insertId });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.delete('/api/roadmap/:id', async (req, res) => {
  try {
    const [result] = await pool.query('DELETE FROM ROADMAP WHERE route_id = ?', [req.params.id]);
    res.json({ success: result.affectedRows > 0 });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// Reload graph (re-initialize optimizer)
app.post('/api/roadmap/reload', async (_req, res) => {
  try {
    await initializeOptimizer();
    res.json({ success: true, message: 'Graph reloaded and Floyd-Warshall re-computed.' });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// ═══════════════════════════════════════════════════════════════════════════
// ROUTE OPTIMIZATION
// ═══════════════════════════════════════════════════════════════════════════

app.get('/api/route/optimize/:driverId', async (req, res) => {
  try {
    const result = await optimizer.optimizeRoute(parseInt(req.params.driverId), pool);

    if (!result.success) {
      return res.status(400).json(result);
    }

    // Enrich with location names
    const [locations] = await pool.query('SELECT * FROM LOCATION');
    const locMap = {};
    for (const loc of locations) {
      locMap[loc.loc_id] = loc;
    }

    const enrichedLegs = result.legs.map((leg) => ({
      ...leg,
      fromName: locMap[leg.fromLocId]?.name || `Location ${leg.fromLocId}`,
      toName: locMap[leg.toLocId]?.name || `Location ${leg.toLocId}`,
      pathNames: leg.path.map((id) => locMap[id]?.name || `Location ${id}`),
    }));

    const enrichedSequence = result.tspSequence.map((id) => ({
      locId: id,
      name: locMap[id]?.name || `Location ${id}`,
    }));

    res.json({
      ...result,
      tspSequence: enrichedSequence,
      legs: enrichedLegs,
    });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// ── Dashboard Stats ──────────────────────────────────────────────────────────
app.get('/api/stats', async (_req, res) => {
  try {
    const [locCount] = await pool.query('SELECT COUNT(*) AS cnt FROM LOCATION');
    const [driverCount] = await pool.query('SELECT COUNT(*) AS cnt FROM DRIVER_DETAILS');
    const [deliveryCount] = await pool.query('SELECT COUNT(*) AS cnt FROM DELIVERY');
    const [pendingCount] = await pool.query("SELECT COUNT(*) AS cnt FROM DELIVERY WHERE status = 'Pending'");
    const [roadCount] = await pool.query('SELECT COUNT(*) AS cnt FROM ROADMAP');
    const [userCount] = await pool.query('SELECT COUNT(*) AS cnt FROM USER');

    res.json({
      locations: locCount[0].cnt,
      drivers: driverCount[0].cnt,
      totalDeliveries: deliveryCount[0].cnt,
      pendingDeliveries: pendingCount[0].cnt,
      routes: roadCount[0].cnt,
      users: userCount[0].cnt,
    });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// ── Start Server ─────────────────────────────────────────────────────────────
const PORT = process.env.SERVER_PORT || 5000;

async function start() {
  try {
    // Test DB connection
    const conn = await pool.getConnection();
    console.log(`[DB] Connected to ${process.env.DB_NAME} at ${process.env.DB_HOST}`);
    conn.release();

    // Initialize route optimizer
    await initializeOptimizer();

    app.listen(PORT, () => {
      console.log(`\n[Server] RouteMaster API running on http://localhost:${PORT}`);
      console.log('[Server] Ready to accept requests.\n');
    });
  } catch (err) {
    console.error('[FATAL] Cannot start:', err.message);
    process.exit(1);
  }
}

start();
