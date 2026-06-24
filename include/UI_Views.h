#ifndef UI_VIEWS_H
#define UI_VIEWS_H

#include <string>
#include <iostream>
#include <iomanip>
#include <limits>

#include "Models.h"
#include "DatabaseManager.h"
#include "RouteOptimizer.h"

// ============================================================================
// UI_Views.h — Presentation Layer (Tier 1)
//
// CLI dashboard classes. These handle ONLY std::cin/std::cout.
// All business logic is delegated to RouteOptimizer (Tier 2).
// All database access is delegated to DatabaseManager (Tier 3).
//
// Class Hierarchy:
//   LoginView       → Entry point: authentication flow
//   AdminDashboard  → Full CRUD management menus
//   DriverDashboard → Delivery viewing, route optimization, completion
// ============================================================================

// ─────────────────────────────────────────────────────────────────────────────
// LoginView — Authentication entry point
// Loops until a valid login or user chooses to exit.
// Routes to AdminDashboard or DriverDashboard based on role.
// ─────────────────────────────────────────────────────────────────────────────
class LoginView {
private:
    DatabaseManager& db;
    RouteOptimizer&  optimizer;

public:
    LoginView(DatabaseManager& db, RouteOptimizer& opt);

    // Main loop: prompts for credentials, authenticates, routes to dashboard
    void start();
};

// ─────────────────────────────────────────────────────────────────────────────
// AdminDashboard — Full system management
// Menu options for managing Locations, Routes, Drivers, Deliveries, Users
// ─────────────────────────────────────────────────────────────────────────────
class AdminDashboard {
private:
    DatabaseManager& db;
    RouteOptimizer&  optimizer;
    User             currentUser;

    // Sub-menus (each loops until user returns)
    void manageLocations();
    void manageRoutes();
    void manageDrivers();
    void manageDeliveries();
    void viewAllUsers();
    void registerNewUser();
    void deleteUser();

    // Utility: print a horizontal separator line
    void printSeparator(int width = 60) const;

public:
    AdminDashboard(DatabaseManager& db, RouteOptimizer& opt, const User& user);

    // Main menu loop — returns when admin logs out
    void show();
};

// ─────────────────────────────────────────────────────────────────────────────
// DriverDashboard — Driver-facing operations
// View deliveries, optimize route, mark deliveries as complete
// ─────────────────────────────────────────────────────────────────────────────
class DriverDashboard {
private:
    DatabaseManager& db;
    RouteOptimizer&  optimizer;
    User             currentUser;

    void viewPendingDeliveries();
    void optimizeAndStartRoute();
    void markDeliveryComplete();
    void viewMyProfile();

    void printSeparator(int width = 60) const;

public:
    DriverDashboard(DatabaseManager& db, RouteOptimizer& opt, const User& user);

    // Main menu loop — returns when driver logs out
    void show();
};

#endif // UI_VIEWS_H
