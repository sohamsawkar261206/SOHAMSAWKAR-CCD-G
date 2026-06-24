#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <string>
#include <vector>
#include <iostream>

// ── MySQL C API ─────────────────────────────────────────────────────────────
// The MySQL C API (mysql.h) is a C library that integrates seamlessly with C++.
// It ships with MySQL Server and is compatible with MinGW g++ on Windows,
// unlike the separate MySQL Connector/C++ package (which requires MSVC).
// All code in this project remains C++17. Only the DB library calls are C API.
// ─────────────────────────────────────────────────────────────────────────────
#include <mysql.h>

#include "Models.h"

// ============================================================================
// DatabaseManager.h — Data Access Layer (Tier 3)
//
// This is the ONLY class in the entire application that contains SQL strings.
// String inputs are escaped via mysql_real_escape_string() to prevent SQL
// injection. The Presentation and Business Logic layers call these methods only.
// ============================================================================

class DatabaseManager {
private:
    MYSQL* conn;                        // MySQL C API connection handle

    // Connection parameters (order matches constructor initializer list)
    std::string host;
    std::string dbUser;
    std::string dbPassword;
    std::string dbName;
    unsigned int port;

    // Helper: escape a string for safe SQL inclusion (prevents SQL injection)
    std::string escapeString(const std::string& input);

public:
    // ── Lifecycle ────────────────────────────────────────────────────────────
    DatabaseManager(const std::string& host,
                    const std::string& user,
                    const std::string& password,
                    const std::string& database,
                    unsigned int port = 3306);
    ~DatabaseManager();

    bool connect();                 // Establish MySQL connection
    void disconnect();              // Gracefully close connection
    bool isConnected() const;       // Connection health check

    // ── USER CRUD ────────────────────────────────────────────────────────────
    bool                insertUser(const User& user);
    User                getUserById(int userId);
    User                getUserByUsername(const std::string& username);
    std::vector<User>   getAllUsers();
    bool                deleteUser(int userId);

    // Authentication: returns User with userId > 0 on success, 0 on failure
    User authenticateUser(const std::string& username, const std::string& password);

    // ── DRIVER_DETAILS CRUD ──────────────────────────────────────────────────
    bool                        insertDriverDetails(const DriverDetails& details);
    DriverDetails               getDriverDetailsById(int driverId);
    std::vector<DriverDetails>  getAllDriverDetails();
    std::vector<DriverDetails>  getAvailableDrivers();
    bool                        updateDriverStatus(int driverId, const std::string& status);
    bool                        updateDriverLocation(int driverId, int locId);
    bool                        deleteDriverDetails(int driverId);

    // ── LOCATION CRUD ────────────────────────────────────────────────────────
    bool                    insertLocation(const Location& loc);
    Location                getLocationById(int locId);
    std::vector<Location>   getAllLocations();
    bool                    deleteLocation(int locId);
    int                     getLocationCount();

    // ── DELIVERY CRUD ────────────────────────────────────────────────────────
    bool                    insertDelivery(const Delivery& delivery);
    Delivery                getDeliveryById(int deliveryId);
    std::vector<Delivery>   getAllDeliveries();
    std::vector<Delivery>   getDeliveriesByDriver(int driverId);
    std::vector<Delivery>   getPendingDeliveriesByDriver(int driverId);
    bool                    updateDeliveryStatus(int deliveryId, const std::string& status);
    bool                    deleteDelivery(int deliveryId);

    // Returns the count of 'Pending' deliveries currently assigned to a driver.
    // Used to enforce the capacity constraint before new assignments.
    int                     getPendingDeliveryCount(int driverId);

    // ── ROADMAP CRUD ─────────────────────────────────────────────────────────
    bool                    insertRoadmap(const Roadmap& road);
    std::vector<Roadmap>    getAllRoadmaps();
    bool                    deleteRoadmap(int routeId);

    // ── Graph Builder (for Algorithm Layer) ──────────────────────────────────
    // Builds an adjacency matrix from the ROADMAP table.
    // Matrix is (N+1) x (N+1) where N = location count (1-indexed).
    // Missing edges are set to 1e9 (INF). Diagonal is 0.
    // Roads are treated as BIDIRECTIONAL.
    std::vector<std::vector<double>> getAdjacencyMatrix();
};

#endif // DATABASEMANAGER_H
