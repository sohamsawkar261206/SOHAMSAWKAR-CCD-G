// ============================================================================
// RouteOptimizer.cpp — Business Logic Layer Implementation
//
// Contains native C++ implementations of three graph algorithms:
//   1. Floyd-Warshall  — All-Pairs Shortest Path (DP)
//   2. TSP Branch & Bound — Optimal delivery sequencing
//   3. Dijkstra — Turn-by-turn navigation (Greedy)
//
// No external routing APIs are used. All math is written natively.
// ============================================================================

#include "RouteOptimizer.h"

using namespace std;

// ═══════════════════════════════════════════════════════════════════════════
// Initialization
// ═══════════════════════════════════════════════════════════════════════════

void RouteOptimizer::initialize(const vector<vector<double>>& adjacencyMatrix,
                                int locCount) {
    adjMatrix     = adjacencyMatrix;
    locationCount = locCount;

    // distMatrix and nextMatrix will be populated by floydWarshall()
    distMatrix.clear();
    nextMatrix.clear();

    cout << "[Optimizer] Initialized with " << locationCount << " locations." << endl;
}


// ═══════════════════════════════════════════════════════════════════════════
// ALGORITHM 1: Floyd-Warshall (Dynamic Programming)
// ═══════════════════════════════════════════════════════════════════════════
//
// Computes the shortest distance between EVERY pair of nodes.
// Also builds a 'next' matrix for path reconstruction.
//
// Recurrence: dist[i][j] = min(dist[i][j], dist[i][k] + dist[k][j])
//
// Time Complexity:  O(V³)
// Space Complexity: O(V²)
// ─────────────────────────────────────────────────────────────────────────

void RouteOptimizer::floydWarshall() {
    int n = locationCount;

    // Step 1: Copy the adjacency matrix as the initial distance matrix
    distMatrix = adjMatrix;

    // Step 2: Initialize the 'next' matrix for path reconstruction
    // next[i][j] = the next node to visit on the shortest path from i to j
    nextMatrix.assign(n + 1, vector<int>(n + 1, -1));

    for (int i = 1; i <= n; i++) {
        for (int j = 1; j <= n; j++) {
            if (distMatrix[i][j] < INF && i != j) {
                nextMatrix[i][j] = j;   // Direct edge exists → next hop is j
            }
        }
    }

    // Step 3: Core DP — try every intermediate node k
    for (int k = 1; k <= n; k++) {
        for (int i = 1; i <= n; i++) {
            for (int j = 1; j <= n; j++) {
                // If going through k gives a shorter path
                if (distMatrix[i][k] + distMatrix[k][j] < distMatrix[i][j]) {
                    distMatrix[i][j] = distMatrix[i][k] + distMatrix[k][j];
                    nextMatrix[i][j] = nextMatrix[i][k]; // Redirect path through k
                }
            }
        }
    }

    cout << "[Floyd-Warshall] All-pairs shortest paths computed for "
         << n << " nodes." << endl;
}

double RouteOptimizer::getShortestDistance(int from, int to) const {
    if (from >= 1 && from <= locationCount && to >= 1 && to <= locationCount) {
        return distMatrix[from][to];
    }
    return INF;
}

vector<int> RouteOptimizer::reconstructPath(int from, int to) const {
    vector<int> path;

    // If no path exists, return empty
    if (nextMatrix[from][to] == -1) {
        return path;
    }

    // Walk the 'next' pointers from source to destination
    path.push_back(from);
    int current = from;
    while (current != to) {
        current = nextMatrix[current][to];
        if (current == -1) {
            // Broken path — should not happen in a connected graph
            return {};
        }
        path.push_back(current);
    }

    return path;
}


// ═══════════════════════════════════════════════════════════════════════════
// ALGORITHM 2: TSP via Branch & Bound
// ═══════════════════════════════════════════════════════════════════════════
//
// Given a start node and a set of delivery nodes, finds the OPTIMAL
// sequence to visit all delivery nodes with minimum total distance.
//
// This is a Hamiltonian Path problem (no return to start needed).
//
// Algorithm:
//   1. Build a cost matrix for the relevant nodes using Floyd-Warshall
//      distances (O(1) lookups).
//   2. Use a priority queue (min-heap by lower bound) for best-first
//      search through the state-space tree.
//   3. At each node, branch by trying each unvisited delivery node next.
//   4. Prune branches whose lower bound exceeds the current best solution.
//   5. The first complete solution found via best-first search is optimal.
//
// Lower Bound Strategy:
//   For each unvisited node, we add its minimum incoming edge cost.
//   Since each node is entered via a distinct edge, this sum is a
//   valid (admissible) lower bound.
//
// Time: O(n!) worst case, but pruning dramatically reduces exploration.
// ─────────────────────────────────────────────────────────────────────────

double RouteOptimizer::calculateLowerBound(
    const TSPNode& node,
    const vector<vector<double>>& costMatrix,
    int totalNodes) const {

    double bound = node.cost;

    // For the current (last visited) node, add the cost of the cheapest
    // edge going to any unvisited node
    if (node.level < totalNodes) {
        int lastIdx = node.path.back();
        double minOutgoing = INF;
        for (int j = 0; j < totalNodes; j++) {
            if (!node.visited[j]) {
                minOutgoing = min(minOutgoing, costMatrix[lastIdx][j]);
            }
        }
        if (minOutgoing < INF) {
            bound += minOutgoing;
        }
    }

    // For each unvisited node, add the minimum cost of ANY edge
    // coming INTO that node (from any other node in the set).
    // This is valid because each node must be entered exactly once,
    // and distinct edges are used for each entry.
    int unvisitedCount = 0;
    for (int i = 0; i < totalNodes; i++) {
        if (!node.visited[i]) {
            unvisitedCount++;
            double minIncoming = INF;
            for (int j = 0; j < totalNodes; j++) {
                if (j != i) {
                    minIncoming = min(minIncoming, costMatrix[j][i]);
                }
            }
            if (minIncoming < INF) {
                bound += minIncoming;
            }
        }
    }

    // Subtract one minimum since the last unvisited node's incoming edge
    // might overlap with the outgoing edge we already counted above.
    // This adjustment prevents double-counting the bridging edge.
    if (unvisitedCount > 0) {
        // Find biggest minimum to subtract (greedy correction)
        double maxMinIncoming = 0;
        for (int i = 0; i < totalNodes; i++) {
            if (!node.visited[i]) {
                double minIncoming = INF;
                for (int j = 0; j < totalNodes; j++) {
                    if (j != i) {
                        minIncoming = min(minIncoming, costMatrix[j][i]);
                    }
                }
                maxMinIncoming = max(maxMinIncoming, minIncoming);
            }
        }
        bound -= maxMinIncoming;
    }

    return bound;
}

pair<vector<int>, double>
RouteOptimizer::tspBranchAndBound(int startNode, const vector<int>& deliveryNodes) {

    // ── Step 1: Build the node set ──────────────────────────────────────────
    // nodes[0] = startNode, nodes[1..k] = delivery destinations
    vector<int> nodes;
    nodes.push_back(startNode);
    for (int loc : deliveryNodes) {
        nodes.push_back(loc);
    }
    int N = nodes.size();   // Total nodes in the TSP sub-problem

    // ── Step 2: Build cost matrix using Floyd-Warshall distances ────────────
    // costMatrix[i][j] = shortest distance from nodes[i] to nodes[j]
    vector<vector<double>> costMatrix(N, vector<double>(N, INF));
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (i == j) {
                costMatrix[i][j] = 0;
            } else {
                costMatrix[i][j] = getShortestDistance(nodes[i], nodes[j]);
            }
        }
    }

    // ── Step 3: Initialize root of the search tree ──────────────────────────
    TSPNode root;
    root.path     = {0};                    // Start from index 0 (startNode)
    root.visited.assign(N, false);
    root.visited[0] = true;
    root.cost     = 0.0;
    root.level    = 1;
    root.bound    = calculateLowerBound(root, costMatrix, N);

    // Best-first search: priority queue ordered by lower bound (min-heap)
    priority_queue<TSPNode, vector<TSPNode>, greater<TSPNode>> pq;
    pq.push(root);

    // Track the best complete solution
    double bestCost = INF;
    vector<int> bestPath;

    // ── Step 4: Branch & Bound exploration ──────────────────────────────────
    while (!pq.empty()) {
        TSPNode current = pq.top();
        pq.pop();

        // Pruning: skip if this node's lower bound can't beat current best
        if (current.bound >= bestCost) {
            continue;
        }

        // If all nodes visited → complete solution found
        if (current.level == N) {
            if (current.cost < bestCost) {
                bestCost = current.cost;
                bestPath = current.path;
            }
            continue;
        }

        // Branch: try visiting each unvisited node next
        for (int i = 0; i < N; i++) {
            if (current.visited[i]) continue;

            double edgeCost = costMatrix[current.path.back()][i];
            if (edgeCost >= INF) continue;   // No path exists → skip

            // Create child node
            TSPNode child;
            child.path    = current.path;
            child.path.push_back(i);
            child.visited = current.visited;
            child.visited[i] = true;
            child.cost    = current.cost + edgeCost;
            child.level   = current.level + 1;
            child.bound   = calculateLowerBound(child, costMatrix, N);

            // Only enqueue if bound is promising
            if (child.bound < bestCost) {
                pq.push(child);
            }
        }
    }

    // ── Step 5: Map indices back to original loc_ids ────────────────────────
    vector<int> resultSequence;
    for (int idx : bestPath) {
        resultSequence.push_back(nodes[idx]);
    }

    return {resultSequence, bestCost};
}


// ═══════════════════════════════════════════════════════════════════════════
// ALGORITHM 3: Dijkstra's Algorithm (Greedy)
// ═══════════════════════════════════════════════════════════════════════════
//
// Finds the single-source shortest path on the ORIGINAL road graph.
// Used to produce turn-by-turn navigation between consecutive TSP stops.
//
// Uses a min-heap (priority queue) for efficient extraction.
//
// Time:  O((V + E) log V)
// Space: O(V)
// ─────────────────────────────────────────────────────────────────────────

pair<vector<int>, double>
RouteOptimizer::dijkstra(int source, int destination) {
    int n = locationCount;

    // dist[v] = shortest known distance from source to v
    vector<double> dist(n + 1, INF);

    // parent[v] = previous node on the shortest path to v
    vector<int> parent(n + 1, -1);

    // Min-heap: (distance, node)
    priority_queue<pair<double, int>,
                   vector<pair<double, int>>,
                   greater<pair<double, int>>> pq;

    // Initialize source
    dist[source] = 0.0;
    pq.push({0.0, source});

    // ── Main Dijkstra loop ──────────────────────────────────────────────────
    while (!pq.empty()) {
        double d = pq.top().first;
        int u    = pq.top().second;
        pq.pop();

        // Skip if we've already found a better path to u
        if (d > dist[u]) continue;

        // Early exit: if we've reached the destination
        if (u == destination) break;

        // Relax all neighbors of u (using original adjacency matrix)
        for (int v = 1; v <= n; v++) {
            if (adjMatrix[u][v] < INF) {    // Edge exists
                double newDist = dist[u] + adjMatrix[u][v];
                if (newDist < dist[v]) {
                    dist[v]   = newDist;
                    parent[v] = u;
                    pq.push({newDist, v});
                }
            }
        }
    }

    // ── Reconstruct the path ────────────────────────────────────────────────
    vector<int> path;
    if (dist[destination] >= INF) {
        // No path exists
        return {path, INF};
    }

    // Walk backwards from destination to source using parent pointers
    for (int v = destination; v != -1; v = parent[v]) {
        path.push_back(v);
    }
    reverse(path.begin(), path.end());

    return {path, dist[destination]};
}


// ═══════════════════════════════════════════════════════════════════════════
// MASTER PIPELINE: optimizeRoute()
// ═══════════════════════════════════════════════════════════════════════════
//
// This is the entry point called by the DriverDashboard.
// It chains: Fetch Deliveries → TSP → Dijkstra → RouteResult
//
// Flow:
//   1. Get driver's current location from DB
//   2. Fetch all pending deliveries for this driver
//   3. Run TSP B&B to get the optimal visiting sequence
//   4. For each consecutive pair in the sequence, run Dijkstra
//      to get the actual road-level turn-by-turn path
//   5. Package everything into a RouteResult struct
// ─────────────────────────────────────────────────────────────────────────

RouteResult RouteOptimizer::optimizeRoute(int driverId, DatabaseManager& db) {
    RouteResult result;
    result.success = false;
    result.totalDistance = 0.0;

    // ── Step 1: Get driver info ─────────────────────────────────────────────
    DriverDetails driver = db.getDriverDetailsById(driverId);
    if (driver.driverId == 0) {
        result.message = "Driver profile not found.";
        return result;
    }
    int startLoc = driver.currentLocId;

    // ── Step 2: Fetch pending deliveries ────────────────────────────────────
    vector<Delivery> deliveries = db.getPendingDeliveriesByDriver(driverId);
    if (deliveries.empty()) {
        result.message = "No pending deliveries to optimize.";
        return result;
    }

    // Extract unique delivery location IDs
    vector<int> deliveryLocs;
    for (const auto& del : deliveries) {
        // Avoid duplicates (multiple deliveries to same location)
        bool found = false;
        for (int loc : deliveryLocs) {
            if (loc == del.locId) { found = true; break; }
        }
        if (!found) {
            deliveryLocs.push_back(del.locId);
        }
    }

    // ── Step 3: TSP Branch & Bound ──────────────────────────────────────────
    auto tspResult = tspBranchAndBound(startLoc, deliveryLocs);
    vector<int> sequence  = tspResult.first;
    double totalDist      = tspResult.second;

    if (sequence.empty() || totalDist >= INF) {
        result.message = "Could not compute an optimal route. Some locations may be unreachable.";
        return result;
    }

    result.tspSequence   = sequence;
    result.totalDistance  = totalDist;

    // ── Step 4: Dijkstra for each leg ───────────────────────────────────────
    for (size_t i = 0; i < sequence.size() - 1; i++) {
        auto dijResult = dijkstra(sequence[i], sequence[i + 1]);
        vector<int> path = dijResult.first;
        double legDist   = dijResult.second;

        RouteLeg leg;
        leg.fromLocId = sequence[i];
        leg.toLocId   = sequence[i + 1];
        leg.path      = path;
        leg.distance  = legDist;
        result.legs.push_back(leg);
    }

    result.success = true;
    result.message = "Route optimized successfully!";
    return result;
}
