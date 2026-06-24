// ============================================================================
// routeOptimizer.js — Business Logic Layer (mirrors C++ RouteOptimizer)
//
// Algorithm Stack (identical to C++ implementation):
//   1. Floyd-Warshall  (DP)        — All-Pairs Shortest Path
//   2. TSP Branch & Bound          — Optimal delivery sequencing
//   3. Dijkstra        (Greedy)    — Turn-by-turn path navigation
// ============================================================================

const INF = 1e9;

class RouteOptimizer {
  constructor() {
    this.adjMatrix = [];
    this.distMatrix = [];
    this.nextMatrix = [];
    this.locationCount = 0;
  }

  // ── Initialization ─────────────────────────────────────────────────────────
  initialize(adjacencyMatrix, locCount) {
    this.adjMatrix = adjacencyMatrix;
    this.locationCount = locCount;
    this.distMatrix = [];
    this.nextMatrix = [];
  }

  // ── Algorithm 1: Floyd-Warshall (Dynamic Programming) ──────────────────────
  // Pre-computes all-pairs shortest paths. Time: O(V³)  Space: O(V²)
  floydWarshall() {
    const n = this.locationCount;

    // Deep copy the adjacency matrix into distMatrix
    this.distMatrix = this.adjMatrix.map(row => [...row]);

    // Initialize nextMatrix for path reconstruction
    this.nextMatrix = Array.from({ length: n + 1 }, () =>
      Array(n + 1).fill(-1)
    );

    // Set nextMatrix[i][j] = j for all direct edges
    for (let i = 1; i <= n; i++) {
      for (let j = 1; j <= n; j++) {
        if (i !== j && this.distMatrix[i][j] < INF) {
          this.nextMatrix[i][j] = j;
        }
      }
    }

    // Floyd-Warshall triple loop
    for (let k = 1; k <= n; k++) {
      for (let i = 1; i <= n; i++) {
        for (let j = 1; j <= n; j++) {
          if (this.distMatrix[i][k] + this.distMatrix[k][j] < this.distMatrix[i][j]) {
            this.distMatrix[i][j] = this.distMatrix[i][k] + this.distMatrix[k][j];
            this.nextMatrix[i][j] = this.nextMatrix[i][k];
          }
        }
      }
    }
  }

  // Query pre-computed shortest distance (O(1) lookup)
  getShortestDistance(from, to) {
    if (from < 1 || from > this.locationCount || to < 1 || to > this.locationCount) {
      return INF;
    }
    return this.distMatrix[from][to];
  }

  // Reconstruct the shortest path between two nodes using nextMatrix
  reconstructPath(from, to) {
    if (this.nextMatrix[from][to] === -1) return [];

    const path = [from];
    let current = from;
    while (current !== to) {
      current = this.nextMatrix[current][to];
      if (current === -1) return [];
      path.push(current);
    }
    return path;
  }

  // ── Algorithm 2: TSP via Branch & Bound ────────────────────────────────────
  // Finds optimal order to visit all delivery nodes
  // Time: O(n! / pruning) — exponential worst case, fast for n ≤ 15
  tspBranchAndBound(startNode, deliveryNodes) {
    if (deliveryNodes.length === 0) {
      return { sequence: [startNode], distance: 0 };
    }

    if (deliveryNodes.length === 1) {
      const dist = this.getShortestDistance(startNode, deliveryNodes[0]);
      return { sequence: [startNode, deliveryNodes[0]], distance: dist };
    }

    // Build local nodes list: 0 = start, 1..n = delivery nodes
    const allNodes = [startNode, ...deliveryNodes];
    const totalNodes = allNodes.length;

    // Build cost matrix using Floyd-Warshall distances
    const costMatrix = Array.from({ length: totalNodes }, () =>
      Array(totalNodes).fill(INF)
    );

    for (let i = 0; i < totalNodes; i++) {
      for (let j = 0; j < totalNodes; j++) {
        if (i !== j) {
          costMatrix[i][j] = this.getShortestDistance(allNodes[i], allNodes[j]);
        } else {
          costMatrix[i][j] = 0;
        }
      }
    }

    // Branch & Bound with priority queue (best-first search)
    let bestCost = INF;
    let bestPath = [];

    // Calculate lower bound for a node
    const calcLowerBound = (node) => {
      let bound = node.cost;
      for (let i = 0; i < totalNodes; i++) {
        if (!node.visited[i]) {
          let minEdge = INF;
          for (let j = 0; j < totalNodes; j++) {
            if (i !== j && (j === 0 || !node.visited[j] || j === node.path[node.path.length - 1])) {
              minEdge = Math.min(minEdge, costMatrix[i][j]);
            }
          }
          if (minEdge < INF) bound += minEdge;
        }
      }
      return bound;
    };

    // Priority queue (min-heap by bound)
    const pq = [];
    const pushPQ = (item) => {
      pq.push(item);
      pq.sort((a, b) => a.bound - b.bound);
    };

    // Root node: start at index 0
    const rootVisited = Array(totalNodes).fill(false);
    rootVisited[0] = true;
    const rootNode = {
      path: [0],
      visited: rootVisited,
      cost: 0,
      bound: 0,
      level: 1,
    };
    rootNode.bound = calcLowerBound(rootNode);
    pushPQ(rootNode);

    while (pq.length > 0) {
      const current = pq.shift();

      if (current.bound >= bestCost) continue;

      if (current.level === totalNodes) {
        if (current.cost < bestCost) {
          bestCost = current.cost;
          bestPath = [...current.path];
        }
        continue;
      }

      // Expand children
      for (let i = 1; i < totalNodes; i++) {
        if (!current.visited[i]) {
          const lastNode = current.path[current.path.length - 1];
          const edgeCost = costMatrix[lastNode][i];

          if (edgeCost >= INF) continue;

          const newCost = current.cost + edgeCost;
          if (newCost >= bestCost) continue;

          const newVisited = [...current.visited];
          newVisited[i] = true;

          const child = {
            path: [...current.path, i],
            visited: newVisited,
            cost: newCost,
            bound: 0,
            level: current.level + 1,
          };
          child.bound = calcLowerBound(child);

          if (child.bound < bestCost) {
            pushPQ(child);
          }
        }
      }
    }

    // Convert local indices back to location IDs
    const sequence = bestPath.map((idx) => allNodes[idx]);
    return { sequence, distance: bestCost };
  }

  // ── Algorithm 3: Dijkstra's Algorithm (Greedy) ─────────────────────────────
  // Single-source shortest path on the ORIGINAL graph
  // Time: O((V+E) log V)
  dijkstra(source, destination) {
    const n = this.locationCount;
    const dist = Array(n + 1).fill(INF);
    const prev = Array(n + 1).fill(-1);
    const visited = Array(n + 1).fill(false);

    dist[source] = 0;

    // Simple priority queue with array
    const pq = [{ node: source, dist: 0 }];

    while (pq.length > 0) {
      pq.sort((a, b) => a.dist - b.dist);
      const { node: u } = pq.shift();

      if (visited[u]) continue;
      visited[u] = true;

      if (u === destination) break;

      for (let v = 1; v <= n; v++) {
        if (!visited[v] && this.adjMatrix[u][v] < INF) {
          const newDist = dist[u] + this.adjMatrix[u][v];
          if (newDist < dist[v]) {
            dist[v] = newDist;
            prev[v] = u;
            pq.push({ node: v, dist: newDist });
          }
        }
      }
    }

    // Reconstruct path
    const path = [];
    if (dist[destination] < INF) {
      let current = destination;
      while (current !== -1) {
        path.unshift(current);
        current = prev[current];
      }
    }

    return { path, distance: dist[destination] };
  }

  // ── Master Pipeline ────────────────────────────────────────────────────────
  // Chains: Fetch deliveries → TSP → Dijkstra → RouteResult
  async optimizeRoute(driverId, db) {
    // Get driver's current location
    const [driverRows] = await db.query(
      'SELECT * FROM DRIVER_DETAILS WHERE driver_id = ?',
      [driverId]
    );
    if (driverRows.length === 0) {
      return { success: false, message: 'Driver not found.' };
    }
    const startLocId = driverRows[0].current_loc_id;

    // Get pending deliveries
    const [deliveryRows] = await db.query(
      "SELECT * FROM DELIVERY WHERE driver_id = ? AND status = 'Pending'",
      [driverId]
    );
    if (deliveryRows.length === 0) {
      return { success: false, message: 'No pending deliveries to optimize.' };
    }

    const deliveryNodes = deliveryRows.map((d) => d.loc_id);

    // Run TSP Branch & Bound
    const tspResult = this.tspBranchAndBound(startLocId, deliveryNodes);

    if (tspResult.sequence.length === 0) {
      return { success: false, message: 'No valid route found.' };
    }

    // Build legs using Dijkstra on original graph
    const legs = [];
    for (let i = 0; i < tspResult.sequence.length - 1; i++) {
      const from = tspResult.sequence[i];
      const to = tspResult.sequence[i + 1];
      const dijResult = this.dijkstra(from, to);

      legs.push({
        fromLocId: from,
        toLocId: to,
        path: dijResult.path,
        distance: dijResult.distance,
      });
    }

    return {
      success: true,
      message: 'Route optimized successfully.',
      tspSequence: tspResult.sequence,
      totalDistance: tspResult.distance,
      legs,
    };
  }
}

module.exports = RouteOptimizer;
