#ifndef ROUTEOPTIMIZER_H
#define ROUTEOPTIMIZER_H

#include <vector>
#include <string>
#include <utility>
#include <queue>
#include <algorithm>
#include <cmath>
#include <climits>
#include <iostream>
#include <iomanip>
#include <functional>

#include "Models.h"
#include "DatabaseManager.h"

// ============================================================================
// RouteOptimizer.h — Business Logic Layer (Tier 2)
//
// This class owns ALL algorithm execution. It receives data from the
// DatabaseManager (Tier 3) and returns structured results to the
// Presentation layer (Tier 1). No SQL or cin/cout menus belong here.
//
// Algorithm Stack:
//   1. Floyd-Warshall  (DP)            — All-Pairs Shortest Path
//   2. TSP Branch & Bound              — Optimal delivery sequencing
//   3. Dijkstra        (Greedy)        — Turn-by-turn path navigation
// ============================================================================

class RouteOptimizer {
private:
    // ── Graph Data ───────────────────────────────────────────────────────────
    static constexpr double INF = 1e9;

    std::vector<std::vector<double>> adjMatrix;   // Original from ROADMAP
    std::vector<std::vector<double>> distMatrix;  // Floyd-Warshall output
    std::vector<std::vector<int>>    nextMatrix;  // Path reconstruction
    int locationCount;

    // ── TSP Branch & Bound Internal ──────────────────────────────────────────
    // A node in the B&B search tree
    struct TSPNode {
        std::vector<int>  path;       // Nodes visited so far (local indices)
        std::vector<bool> visited;    // Visited flags
        double cost;                  // Actual cost accumulated
        double bound;                 // Lower bound on total cost
        int    level;                 // Depth in search tree

        // Priority queue orders by lowest bound (best-first search)
        bool operator>(const TSPNode& other) const {
            return bound > other.bound;
        }
    };

    // Calculate a lower bound for a partial TSP solution
    double calculateLowerBound(const TSPNode& node,
                               const std::vector<std::vector<double>>& costMatrix,
                               int totalNodes) const;

public:
    // ── Initialization ───────────────────────────────────────────────────────
    // Must be called after DatabaseManager provides the adjacency matrix
    void initialize(const std::vector<std::vector<double>>& adjacencyMatrix,
                    int locCount);

    // ── Algorithm 1: Floyd-Warshall (Dynamic Programming) ────────────────────
    // Pre-computes all-pairs shortest paths. Run ONCE at boot.
    // Time: O(V³)   Space: O(V²)
    void floydWarshall();

    // Query the pre-computed shortest distance (O(1) lookup)
    double getShortestDistance(int from, int to) const;

    // Reconstruct the shortest path between two nodes using nextMatrix
    std::vector<int> reconstructPath(int from, int to) const;

    // ── Algorithm 2: TSP via Branch & Bound ──────────────────────────────────
    // Finds the optimal order to visit all delivery nodes.
    // Input:  startNode (driver's current loc_id), deliveryNodes (dest loc_ids)
    // Output: {optimal sequence of loc_ids, total distance}
    // Time: O(n! / pruning)  — exponential worst case, fast in practice for n≤15
    std::pair<std::vector<int>, double>
    tspBranchAndBound(int startNode, const std::vector<int>& deliveryNodes);

    // ── Algorithm 3: Dijkstra's Algorithm (Greedy) ───────────────────────────
    // Finds single-source shortest path on the ORIGINAL graph.
    // Used for turn-by-turn navigation between consecutive TSP stops.
    // Time: O((V+E) log V)
    std::pair<std::vector<int>, double>
    dijkstra(int source, int destination);

    // ── Master Pipeline ──────────────────────────────────────────────────────
    // Chains: Fetch deliveries → TSP → Dijkstra → RouteResult
    // Called by the DriverDashboard when driver requests route optimization.
    RouteResult optimizeRoute(int driverId, DatabaseManager& db);
};

#endif // ROUTEOPTIMIZER_H
