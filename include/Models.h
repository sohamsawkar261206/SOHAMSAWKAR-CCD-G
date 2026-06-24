#ifndef MODELS_H
#define MODELS_H

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>

// ============================================================================
// Models.h — Domain Model Layer
//
// Each class maps 1:1 to a MySQL table. These are pure data containers
// with constructors and display utilities. No business logic or SQL here.
// ============================================================================

// ─────────────────────────────────────────────────────────────────────────────
// USER table → User class
// Fields: user_id (PK), username, password, role ('Admin'/'Driver')
// ─────────────────────────────────────────────────────────────────────────────
class User {
public:
    int         userId;
    std::string username;
    std::string password;
    std::string role;           // "Admin" or "Driver"

    // Default constructor — initializes to safe empty state
    User();

    // Parameterized constructor — maps directly from a DB row
    User(int id, const std::string& uname, const std::string& pwd, const std::string& r);

    // Pretty-print this user to stdout
    void display() const;
};

// ─────────────────────────────────────────────────────────────────────────────
// DRIVER_DETAILS table → DriverDetails class
// Fields: driver_id (PK/FK→USER), license_no, capacity, status, current_loc_id
// ─────────────────────────────────────────────────────────────────────────────
class DriverDetails {
public:
    int         driverId;       // FK → USER.user_id (1:1 relationship)
    std::string licenseNo;
    int         capacity;       // Max packages the driver can carry
    std::string status;         // "Available", "On_Route", "Off_Duty"
    int         currentLocId;   // FK → LOCATION.loc_id (DEFAULT 1)

    DriverDetails();
    DriverDetails(int dId, const std::string& license, int cap,
                  const std::string& stat, int locId);

    void display() const;
};

// ─────────────────────────────────────────────────────────────────────────────
// LOCATION table → Location class
// Fields: loc_id (PK), name, latitude, longitude
// Represents a node in the road graph.
// ─────────────────────────────────────────────────────────────────────────────
class Location {
public:
    int         locId;
    std::string name;
    double      latitude;
    double      longitude;

    Location();
    Location(int id, const std::string& n, double lat, double lon);

    void display() const;
};

// ─────────────────────────────────────────────────────────────────────────────
// DELIVERY table → Delivery class
// Fields: delivery_id (PK), driver_id (FK), loc_id (FK), status, assigned_time
// ─────────────────────────────────────────────────────────────────────────────
class Delivery {
public:
    int         deliveryId;
    int         driverId;       // FK → DRIVER_DETAILS.driver_id
    int         locId;          // FK → LOCATION.loc_id (destination)
    std::string status;         // "Pending" or "Completed"
    std::string assignedTime;   // TIMESTAMP stored as string

    Delivery();
    Delivery(int delId, int drId, int lId, const std::string& stat,
             const std::string& aTime);

    void display() const;
};

// ─────────────────────────────────────────────────────────────────────────────
// ROADMAP table → Roadmap class
// Fields: route_id (PK), source_id (FK), dest_id (FK), distance
// Represents a weighted edge in the road graph.
// ─────────────────────────────────────────────────────────────────────────────
class Roadmap {
public:
    int    routeId;
    int    sourceId;        // FK → LOCATION.loc_id (edge start)
    int    destId;          // FK → LOCATION.loc_id (edge end)
    double distance;        // Weight in kilometers

    Roadmap();
    Roadmap(int rId, int srcId, int dstId, double dist);

    void display() const;
};

// ─────────────────────────────────────────────────────────────────────────────
// RouteResult — Returned by RouteOptimizer::optimizeRoute()
// Carries the full algorithm pipeline output for the Presentation layer.
// ─────────────────────────────────────────────────────────────────────────────
struct RouteLeg {
    int fromLocId;
    int toLocId;
    std::vector<int> path;      // Full Dijkstra path (intermediate nodes)
    double distance;            // Total distance for this leg
};

struct RouteResult {
    bool success;
    std::string message;
    std::vector<int> tspSequence;   // Optimal sequence of loc_ids
    double totalDistance;
    std::vector<RouteLeg> legs;     // Turn-by-turn legs from Dijkstra
};

#endif // MODELS_H
