// ============================================================================
// main.cpp — RouteMaster Application Entry Point
//
// Boot Sequence:
//   1. Connect to MySQL database
//   2. Load the road graph (adjacency matrix from ROADMAP table)
//   3. Pre-compute Floyd-Warshall all-pairs shortest paths
//   4. Launch the login screen (routes to Admin/Driver dashboards)
//   5. On exit, disconnect and clean up
//
// Architecture:
//   main.cpp creates the 3 tier objects and wires them together:
//     DatabaseManager (Tier 3) → RouteOptimizer (Tier 2) → UI Views (Tier 1)
// ============================================================================

#include <iostream>
#include "DatabaseManager.h"
#include "RouteOptimizer.h"
#include "UI_Views.h"

using namespace std;

int main() {
    cout << "\n";
    cout << "============================================================\n";
    cout << "  RouteMaster - Intelligent Logistics Route Optimizer\n";
    cout << "  Booting up...\n";
    cout << "============================================================\n\n";

    // ── Step 1: Initialize the Data Access Layer (Tier 3) ───────────────────
    // ⚠️  CHANGE THESE to match your MySQL Workbench credentials:
    //     Host:     "tcp://127.0.0.1:3306" (default MySQL port)
    //     User:     your MySQL username (usually "root")
    //     Password: your MySQL password
    //     Database: "routemaster_db" (created by routemaster_schema.sql)

    DatabaseManager db(
        "127.0.0.1",                 // MySQL host
        "root",                      // MySQL username
        "Piyush@321#",        // MySQL password  <-- CHANGE THIS
        "routemaster_db"             // Database name
    );

    if (!db.connect()) {
        cerr << "\n[FATAL] Cannot start without database connection.\n";
        cerr << "Please check:\n";
        cerr << "  1. MySQL Server is running\n";
        cerr << "  2. Credentials in main.cpp are correct\n";
        cerr << "  3. routemaster_schema.sql has been executed\n\n";
        return 1;
    }

    // ── Step 2: Initialize the Business Logic Layer (Tier 2) ────────────────
    RouteOptimizer optimizer;

    // Load the road graph from the ROADMAP table
    cout << "\n[Boot] Loading road network...\n";
    auto adjMatrix = db.getAdjacencyMatrix();
    int locCount   = db.getLocationCount();

    optimizer.initialize(adjMatrix, locCount);

    // ── Step 3: Pre-compute Floyd-Warshall ──────────────────────────────────
    // This gives O(1) shortest-distance lookups for TSP and navigation.
    cout << "[Boot] Running Floyd-Warshall pre-computation...\n";
    optimizer.floydWarshall();
    cout << "[Boot] System ready.\n";

    // ── Step 4: Launch the Presentation Layer (Tier 1) ──────────────────────
    LoginView login(db, optimizer);
    login.start();      // Main application loop

    // ── Step 5: Graceful shutdown ───────────────────────────────────────────
    db.disconnect();
    cout << "\n[Shutdown] RouteMaster terminated successfully.\n\n";

    return 0;
}
