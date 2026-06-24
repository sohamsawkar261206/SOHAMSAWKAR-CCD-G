// ============================================================================
// UI_Views.cpp — Presentation Layer Implementation
//
// All std::cin / std::cout interactions happen here.
// Business logic is delegated to RouteOptimizer (Tier 2).
// Database access is delegated to DatabaseManager (Tier 3).
// ============================================================================

#include "UI_Views.h"
#include <ctime>
#include <sstream>

using namespace std;

// ── Overdue threshold (in hours) ────────────────────────────────────────────
static const double OVERDUE_THRESHOLD_HOURS = 4.0;

// ─────────────────────────────────────────────────────────────────────────────
// Helper: Parse MySQL TIMESTAMP string → time_t
// Format: "YYYY-MM-DD HH:MM:SS"
// ─────────────────────────────────────────────────────────────────────────────
static time_t parseTimestamp(const string& timestamp) {
    struct tm tm = {};
    sscanf(timestamp.c_str(), "%d-%d-%d %d:%d:%d",
           &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
           &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
    tm.tm_year -= 1900;  // years since 1900
    tm.tm_mon  -= 1;     // months 0-11
    return mktime(&tm);
}

// ─────────────────────────────────────────────────────────────────────────────
// Helper: Get elapsed time as a human-readable string
// Returns e.g. "2h 35m ago", "45m ago", "Just now"
// ─────────────────────────────────────────────────────────────────────────────
static string getElapsedTimeStr(const string& assignedTime) {
    if (assignedTime.empty()) return "N/A";

    time_t assigned = parseTimestamp(assignedTime);
    time_t now      = time(nullptr);
    double diffSec  = difftime(now, assigned);

    if (diffSec < 0)   return "Just now";
    if (diffSec < 60)  return "Just now";

    int totalMin = static_cast<int>(diffSec / 60);
    int hours    = totalMin / 60;
    int minutes  = totalMin % 60;
    int days     = hours / 24;

    ostringstream oss;
    if (days > 0) {
        oss << days << "d " << (hours % 24) << "h ago";
    } else if (hours > 0) {
        oss << hours << "h " << minutes << "m ago";
    } else {
        oss << minutes << "m ago";
    }
    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// Helper: Check if a delivery is overdue (past threshold)
// ─────────────────────────────────────────────────────────────────────────────
static bool isOverdue(const string& assignedTime) {
    if (assignedTime.empty()) return false;

    time_t assigned = parseTimestamp(assignedTime);
    time_t now      = time(nullptr);
    double diffHrs  = difftime(now, assigned) / 3600.0;

    return diffHrs >= OVERDUE_THRESHOLD_HOURS;
}

// ─────────────────────────────────────────────────────────────────────────────
// Utility: safe integer input with validation
// ─────────────────────────────────────────────────────────────────────────────
static int getIntInput(const string& prompt) {
    int value;
    while (true) {
        cout << prompt;
        if (cin >> value) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return value;
        }
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "  [!] Invalid input. Please enter a number.\n";
    }
}

static double getDoubleInput(const string& prompt) {
    double value;
    while (true) {
        cout << prompt;
        if (cin >> value) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return value;
        }
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "  [!] Invalid input. Please enter a number.\n";
    }
}

static string getStringInput(const string& prompt) {
    string value;
    cout << prompt;
    getline(cin, value);
    return value;
}


// ═══════════════════════════════════════════════════════════════════════════
// LoginView
// ═══════════════════════════════════════════════════════════════════════════

LoginView::LoginView(DatabaseManager& db, RouteOptimizer& opt)
    : db(db), optimizer(opt) {}

void LoginView::start() {
    while (true) {
        cout << "\n";
        cout << "============================================================\n";
        cout << "     ____             _       __  __           _            \n";
        cout << "    |  _ \\ ___  _   _| |_ ___|  \\/  | __ _ ___| |_ ___ _ __ \n";
        cout << "    | |_) / _ \\| | | | __/ _ \\ |\\/| |/ _` / __| __/ _ \\ '__|\n";
        cout << "    |  _ < (_) | |_| | ||  __/ |  | | (_| \\__ \\ ||  __/ |   \n";
        cout << "    |_| \\_\\___/ \\__,_|\\__\\___|_|  |_|\\__,_|___/\\__\\___|_|   \n";
        cout << "                                                            \n";
        cout << "          Intelligent Logistics Route Optimizer             \n";
        cout << "============================================================\n";
        cout << "\n";
        cout << "  1. Login\n";
        cout << "  2. Exit\n";
        cout << "\n";

        int choice = getIntInput("  Enter choice: ");

        if (choice == 2) {
            cout << "\n  Goodbye! Thank you for using RouteMaster.\n\n";
            break;
        }

        if (choice != 1) {
            cout << "  [!] Invalid choice.\n";
            continue;
        }

        // ── Login Flow ──────────────────────────────────────────────────────
        string username = getStringInput("  Username: ");
        string password = getStringInput("  Password: ");

        User user = db.authenticateUser(username, password);

        if (user.userId == 0) {
            cout << "\n  [X] Authentication failed. Invalid credentials.\n";
            continue;
        }

        cout << "\n  [OK] Welcome, " << user.username
             << "! (Role: " << user.role << ")\n";

        // Route to the correct dashboard based on role
        if (user.role == "Admin") {
            AdminDashboard admin(db, optimizer, user);
            admin.show();
        } else if (user.role == "Driver") {
            DriverDashboard driver(db, optimizer, user);
            driver.show();
        }

        // After dashboard returns (logout), loop back to login screen
    }
}


// ═══════════════════════════════════════════════════════════════════════════
// AdminDashboard
// ═══════════════════════════════════════════════════════════════════════════

AdminDashboard::AdminDashboard(DatabaseManager& db, RouteOptimizer& opt,
                               const User& user)
    : db(db), optimizer(opt), currentUser(user) {}

void AdminDashboard::printSeparator(int width) const {
    cout << string(width, '-') << endl;
}

void AdminDashboard::show() {
    while (true) {
        cout << "\n";
        printSeparator(60);
        cout << "  ADMIN DASHBOARD - Logged in as: " << currentUser.username << "\n";
        printSeparator(60);
        cout << "  1. Manage Locations\n";
        cout << "  2. Manage Routes (Roadmap)\n";
        cout << "  3. Manage Drivers\n";
        cout << "  4. Manage Deliveries\n";
        cout << "  5. View All Users\n";
        cout << "  6. Register New User\n";
        cout << "  7. Delete User\n";
        cout << "  8. Logout\n";
        printSeparator(60);

        int choice = getIntInput("  Enter choice: ");

        switch (choice) {
            case 1: manageLocations();  break;
            case 2: manageRoutes();     break;
            case 3: manageDrivers();    break;
            case 4: manageDeliveries(); break;
            case 5: viewAllUsers();     break;
            case 6: registerNewUser();  break;
            case 7: deleteUser();       break;
            case 8:
                cout << "  Logging out...\n";
                return;
            default:
                cout << "  [!] Invalid choice.\n";
        }
    }
}

// ── Admin: Manage Locations ─────────────────────────────────────────────────

void AdminDashboard::manageLocations() {
    while (true) {
        cout << "\n  --- LOCATION MANAGEMENT ---\n";
        cout << "  1. Add Location\n";
        cout << "  2. View All Locations\n";
        cout << "  3. Delete Location\n";
        cout << "  4. Back\n";

        int choice = getIntInput("  Enter choice: ");

        if (choice == 4) return;

        switch (choice) {
            case 1: {
                // Add new location
                string name = getStringInput("  Location name: ");
                double lat   = getDoubleInput("  Latitude:  ");
                double lon   = getDoubleInput("  Longitude: ");

                Location loc(0, name, lat, lon);
                if (db.insertLocation(loc)) {
                    cout << "  [OK] Location added successfully.\n";
                } else {
                    cout << "  [X] Failed to add location.\n";
                }
                break;
            }
            case 2: {
                // View all locations
                auto locations = db.getAllLocations();
                if (locations.empty()) {
                    cout << "  No locations found.\n";
                    break;
                }
                cout << "\n";
                printSeparator(70);
                cout << "| " << left << setw(8)  << "ID"
                     << "| " << left << setw(22) << "Name"
                     << "| " << right << setw(12) << "Latitude"
                     << "| " << right << setw(12) << "Longitude"
                     << " |" << endl;
                printSeparator(70);
                for (const auto& loc : locations) {
                    loc.display();
                }
                printSeparator(70);
                cout << "  Total: " << locations.size() << " locations.\n";
                break;
            }
            case 3: {
                // Delete location
                int id = getIntInput("  Location ID to delete: ");
                if (db.deleteLocation(id)) {
                    cout << "  [OK] Location deleted.\n";
                } else {
                    cout << "  [X] Failed to delete (may have dependencies).\n";
                }
                break;
            }
            default:
                cout << "  [!] Invalid choice.\n";
        }
    }
}

// ── Admin: Manage Routes ────────────────────────────────────────────────────

void AdminDashboard::manageRoutes() {
    while (true) {
        cout << "\n  --- ROADMAP MANAGEMENT ---\n";
        cout << "  1. Add Route\n";
        cout << "  2. View All Routes\n";
        cout << "  3. Delete Route\n";
        cout << "  4. Reload Graph (re-run Floyd-Warshall)\n";
        cout << "  5. Back\n";

        int choice = getIntInput("  Enter choice: ");

        if (choice == 5) return;

        switch (choice) {
            case 1: {
                int src  = getIntInput("  Source Location ID:      ");
                int dst  = getIntInput("  Destination Location ID: ");
                double d = getDoubleInput("  Distance (km):           ");

                Roadmap road(0, src, dst, d);
                if (db.insertRoadmap(road)) {
                    cout << "  [OK] Route added. Remember to reload graph (option 4).\n";
                } else {
                    cout << "  [X] Failed to add route.\n";
                }
                break;
            }
            case 2: {
                auto roads = db.getAllRoadmaps();
                if (roads.empty()) {
                    cout << "  No routes found.\n";
                    break;
                }
                cout << "\n";
                printSeparator(55);
                cout << "| " << left  << setw(10) << "RouteID"
                     << "| " << left  << setw(10) << "Source"
                     << "| " << left  << setw(10) << "Dest"
                     << "| " << right << setw(10) << "Distance"
                     << " |" << endl;
                printSeparator(55);
                for (const auto& road : roads) {
                    road.display();
                }
                printSeparator(55);
                cout << "  Total: " << roads.size() << " routes.\n";
                break;
            }
            case 3: {
                int id = getIntInput("  Route ID to delete: ");
                if (db.deleteRoadmap(id)) {
                    cout << "  [OK] Route deleted. Remember to reload graph (option 4).\n";
                } else {
                    cout << "  [X] Failed to delete route.\n";
                }
                break;
            }
            case 4: {
                // Reload adjacency matrix and re-run Floyd-Warshall
                cout << "  Reloading graph from database...\n";
                auto adjMatrix = db.getAdjacencyMatrix();
                int locCount   = db.getLocationCount();
                optimizer.initialize(adjMatrix, locCount);
                optimizer.floydWarshall();
                cout << "  [OK] Graph reloaded and Floyd-Warshall re-computed.\n";
                break;
            }
            default:
                cout << "  [!] Invalid choice.\n";
        }
    }
}

// ── Admin: Manage Drivers ───────────────────────────────────────────────────

void AdminDashboard::manageDrivers() {
    while (true) {
        cout << "\n  --- DRIVER MANAGEMENT ---\n";
        cout << "  1. View All Drivers\n";
        cout << "  2. View Available Drivers\n";
        cout << "  3. Back\n";

        int choice = getIntInput("  Enter choice: ");

        if (choice == 3) return;

        vector<DriverDetails> drivers;

        if (choice == 1) {
            drivers = db.getAllDriverDetails();
        } else if (choice == 2) {
            drivers = db.getAvailableDrivers();
        } else {
            cout << "  [!] Invalid choice.\n";
            continue;
        }

        if (drivers.empty()) {
            cout << "  No drivers found.\n";
            continue;
        }

        cout << "\n";
        printSeparator(80);
        cout << "| " << left << setw(10) << "DriverID"
             << "| " << left << setw(20) << "License"
             << "| " << left << setw(10) << "Capacity"
             << "| " << left << setw(12) << "Status"
             << "| " << left << setw(10) << "Location"
             << "|" << endl;
        printSeparator(80);
        for (const auto& d : drivers) {
            d.display();
        }
        printSeparator(80);
        cout << "  Total: " << drivers.size() << " drivers.\n";
    }
}

// ── Admin: Manage Deliveries ────────────────────────────────────────────────

void AdminDashboard::manageDeliveries() {
    while (true) {
        cout << "\n  --- DELIVERY MANAGEMENT ---\n";
        cout << "  1. Assign New Delivery\n";
        cout << "  2. View All Deliveries\n";
        cout << "  3. View Deliveries by Driver\n";
        cout << "  4. Delete Delivery\n";
        cout << "  5. Back\n";

        int choice = getIntInput("  Enter choice: ");

        if (choice == 5) return;

        switch (choice) {
            case 1: {
                int driverId = getIntInput("  Driver ID: ");

                // ── Capacity Check ──────────────────────────────────────────
                DriverDetails driverInfo = db.getDriverDetailsById(driverId);
                if (driverInfo.driverId == 0) {
                    cout << "  [X] Driver ID " << driverId << " not found.\n";
                    break;
                }

                int pendingCount = db.getPendingDeliveryCount(driverId);
                cout << "  Driver capacity: " << pendingCount << "/"
                     << driverInfo.capacity << " slots used.\n";

                if (pendingCount >= driverInfo.capacity) {
                    cout << "  [X] Cannot assign: Driver has reached maximum "
                         << "capacity (" << driverInfo.capacity
                         << " pending deliveries).\n";
                    break;
                }
                // ─────────────────────────────────────────────────────────────

                int locId = getIntInput("  Destination Location ID: ");

                Delivery del(0, driverId, locId, "Pending", "");
                if (db.insertDelivery(del)) {
                    cout << "  [OK] Delivery assigned successfully. Driver now has "
                         << (pendingCount + 1) << "/" << driverInfo.capacity
                         << " slots used.\n";
                } else {
                    cout << "  [X] Failed to assign delivery.\n";
                }
                break;
            }
            case 2: {
                auto deliveries = db.getAllDeliveries();
                if (deliveries.empty()) {
                    cout << "  No deliveries found.\n";
                    break;
                }

                // Count overdue pending deliveries
                int overdueCount = 0;
                for (const auto& del : deliveries) {
                    if (del.status == "Pending" && isOverdue(del.assignedTime))
                        overdueCount++;
                }
                if (overdueCount > 0) {
                    cout << "\n  [!] ALERT: " << overdueCount
                         << " pending delivery(ies) are OVERDUE!\n";
                }

                cout << "\n";
                printSeparator(95);
                cout << "| " << left << setw(12) << "DeliveryID"
                     << "| " << left << setw(10) << "DriverID"
                     << "| " << left << setw(8)  << "LocID"
                     << "| " << left << setw(12) << "Status"
                     << "| " << left << setw(22) << "Assigned Time"
                     << "| " << left << setw(15) << "Elapsed"
                     << "|" << endl;
                printSeparator(95);
                for (const auto& del : deliveries) {
                    string elapsed = getElapsedTimeStr(del.assignedTime);
                    bool overdue   = (del.status == "Pending" && isOverdue(del.assignedTime));

                    cout << "| " << left << setw(12) << del.deliveryId
                         << "| " << left << setw(10) << del.driverId
                         << "| " << left << setw(8)  << del.locId
                         << "| " << left << setw(12) << del.status
                         << "| " << left << setw(22) << del.assignedTime
                         << "| " << left << setw(15)
                         << (overdue ? "[!] " + elapsed : elapsed)
                         << "|" << endl;
                }
                printSeparator(95);
                break;
            }
            case 3: {
                int driverId = getIntInput("  Driver ID: ");
                auto deliveries = db.getDeliveriesByDriver(driverId);
                if (deliveries.empty()) {
                    cout << "  No deliveries found for Driver " << driverId << ".\n";
                    break;
                }
                cout << "\n";
                printSeparator(75);
                cout << "| " << left << setw(12) << "DeliveryID"
                     << "| " << left << setw(10) << "DriverID"
                     << "| " << left << setw(8)  << "LocID"
                     << "| " << left << setw(12) << "Status"
                     << "| " << left << setw(22) << "Assigned Time"
                     << "|" << endl;
                printSeparator(75);
                for (const auto& del : deliveries) {
                    del.display();
                }
                printSeparator(75);
                break;
            }
            case 4: {
                int id = getIntInput("  Delivery ID to delete: ");
                if (db.deleteDelivery(id)) {
                    cout << "  [OK] Delivery deleted.\n";
                } else {
                    cout << "  [X] Failed to delete delivery.\n";
                }
                break;
            }
            default:
                cout << "  [!] Invalid choice.\n";
        }
    }
}

// ── Admin: View All Users ───────────────────────────────────────────────────

void AdminDashboard::viewAllUsers() {
    auto users = db.getAllUsers();
    if (users.empty()) {
        cout << "  No users found.\n";
        return;
    }
    cout << "\n";
    printSeparator(40);
    cout << "| " << left << setw(8)  << "UserID"
         << "| " << left << setw(15) << "Username"
         << "| " << left << setw(10) << "Role"
         << "|" << endl;
    printSeparator(40);
    for (const auto& u : users) {
        u.display();
    }
    printSeparator(40);
}

// ── Admin: Register New User ────────────────────────────────────────────────

void AdminDashboard::registerNewUser() {
    cout << "\n  --- REGISTER NEW USER ---\n";

    string username = getStringInput("  Username: ");
    string password = getStringInput("  Password: ");

    cout << "  Role (1 = Admin, 2 = Driver): ";
    int roleChoice = getIntInput("  ");
    string role = (roleChoice == 1) ? "Admin" : "Driver";

    User user(0, username, password, role);
    if (!db.insertUser(user)) {
        cout << "  [X] Failed to register user.\n";
        return;
    }
    cout << "  [OK] User registered as " << role << ".\n";

    // If registering a driver, also create driver details
    if (role == "Driver") {
        // Fetch the newly created user to get their ID
        User newUser = db.getUserByUsername(username);
        if (newUser.userId == 0) {
            cout << "  [X] Could not retrieve new user for driver profile.\n";
            return;
        }

        string license = getStringInput("  License Number: ");
        int capacity   = getIntInput("  Carrying Capacity: ");

        DriverDetails details(newUser.userId, license, capacity, "Available", 1);
        if (db.insertDriverDetails(details)) {
            cout << "  [OK] Driver profile created.\n";
        } else {
            cout << "  [X] Failed to create driver profile.\n";
        }
    }
}

// ── Admin: Delete User (Guarded) ────────────────────────────────────────────

void AdminDashboard::deleteUser() {
    cout << "\n  --- DELETE USER ---\n";

    // Show all users first
    viewAllUsers();

    int userId = getIntInput("  Enter User ID to delete (0 to cancel): ");
    if (userId == 0) return;

    // Look up the user
    User target = db.getUserById(userId);
    if (target.userId == 0) {
        cout << "  [X] User ID " << userId << " not found.\n";
        return;
    }

    // ── Guard: refuse deletion of the last admin ─────────────────────────
    if (target.role == "Admin") {
        auto allUsers = db.getAllUsers();
        int adminCount = 0;
        for (const auto& u : allUsers) {
            if (u.role == "Admin") adminCount++;
        }
        if (adminCount <= 1) {
            cout << "  [X] Cannot delete the last admin. At least one admin "
                 << "account must remain.\n";
            return;
        }
    }

    // ── Guard: refuse deletion if driver has pending deliveries ───────────
    if (target.role == "Driver") {
        int pendingCount = db.getPendingDeliveryCount(userId);
        if (pendingCount > 0) {
            cout << "  [X] Cannot delete driver: " << pendingCount
                 << " pending delivery(ies) still assigned.\n"
                 << "      Complete or reassign them first.\n";
            return;
        }
    }

    // Confirm
    cout << "  Are you sure you want to delete user '" << target.username
         << "' (" << target.role << ")? (1 = Yes, 0 = No): ";
    int confirm = getIntInput("  ");
    if (confirm != 1) {
        cout << "  [!] Deletion cancelled.\n";
        return;
    }

    if (db.deleteUser(userId)) {
        cout << "  [OK] User '" << target.username << "' deleted successfully.\n";
    } else {
        cout << "  [X] Failed to delete user.\n";
    }
}


// ═══════════════════════════════════════════════════════════════════════════
// DriverDashboard
// ═══════════════════════════════════════════════════════════════════════════

DriverDashboard::DriverDashboard(DatabaseManager& db, RouteOptimizer& opt,
                                 const User& user)
    : db(db), optimizer(opt), currentUser(user) {}

void DriverDashboard::printSeparator(int width) const {
    cout << string(width, '-') << endl;
}

void DriverDashboard::show() {
    while (true) {
        cout << "\n";
        printSeparator(60);
        cout << "  DRIVER DASHBOARD - Logged in as: " << currentUser.username << "\n";
        printSeparator(60);
        cout << "  1. View My Pending Deliveries\n";
        cout << "  2. Optimize & Start Route\n";
        cout << "  3. Mark Delivery as Completed\n";
        cout << "  4. View My Profile\n";
        cout << "  5. Logout\n";
        printSeparator(60);

        int choice = getIntInput("  Enter choice: ");

        switch (choice) {
            case 1: viewPendingDeliveries();  break;
            case 2: optimizeAndStartRoute();  break;
            case 3: markDeliveryComplete();   break;
            case 4: viewMyProfile();          break;
            case 5:
                cout << "  Logging out...\n";
                return;
            default:
                cout << "  [!] Invalid choice.\n";
        }
    }
}

// ── Driver: View Pending Deliveries ─────────────────────────────────────────

void DriverDashboard::viewPendingDeliveries() {
    auto deliveries = db.getPendingDeliveriesByDriver(currentUser.userId);

    if (deliveries.empty()) {
        cout << "\n  No pending deliveries assigned to you.\n";
        return;
    }

    // Count overdue deliveries for summary
    int overdueCount = 0;
    for (const auto& del : deliveries) {
        if (isOverdue(del.assignedTime)) overdueCount++;
    }

    cout << "\n  --- YOUR PENDING DELIVERIES ---\n";
    if (overdueCount > 0) {
        cout << "  [!] WARNING: " << overdueCount << " delivery(ies) OVERDUE "
             << "(pending > " << static_cast<int>(OVERDUE_THRESHOLD_HOURS)
             << " hours)!\n";
    }

    printSeparator(95);
    cout << "| " << left << setw(12) << "DeliveryID"
         << "| " << left << setw(10) << "DriverID"
         << "| " << left << setw(8)  << "LocID"
         << "| " << left << setw(12) << "Status"
         << "| " << left << setw(22) << "Assigned Time"
         << "| " << left << setw(15) << "Elapsed"
         << "|" << endl;
    printSeparator(95);
    for (const auto& del : deliveries) {
        string elapsed = getElapsedTimeStr(del.assignedTime);
        bool overdue   = isOverdue(del.assignedTime);

        cout << "| " << left << setw(12) << del.deliveryId
             << "| " << left << setw(10) << del.driverId
             << "| " << left << setw(8)  << del.locId
             << "| " << left << setw(12) << del.status
             << "| " << left << setw(22) << del.assignedTime
             << "| " << left << setw(15) << (overdue ? "[!] " + elapsed : elapsed)
             << "|" << endl;
    }
    printSeparator(95);

    // Show location names + elapsed time for each delivery
    cout << "\n  Delivery Destinations:\n";
    for (const auto& del : deliveries) {
        Location loc = db.getLocationById(del.locId);
        string elapsed = getElapsedTimeStr(del.assignedTime);
        bool overdue   = isOverdue(del.assignedTime);

        cout << "    Delivery #" << del.deliveryId
             << " -> " << loc.name
             << " (Location " << loc.locId << ")"
             << " - assigned " << elapsed;
        if (overdue) cout << " [!] OVERDUE";
        cout << "\n";
    }
}

// ── Driver: Optimize & Start Route ──────────────────────────────────────────

void DriverDashboard::optimizeAndStartRoute() {
    cout << "\n";
    printSeparator(60);
    cout << "  ROUTE OPTIMIZATION ENGINE\n";
    printSeparator(60);

    // Get driver's current location for display
    DriverDetails driver = db.getDriverDetailsById(currentUser.userId);
    Location startLoc = db.getLocationById(driver.currentLocId);

    cout << "  Starting Location: " << startLoc.name
         << " (ID: " << startLoc.locId << ")\n";
    cout << "  Running algorithm pipeline...\n\n";

    // ── Execute the master pipeline ─────────────────────────────────────────
    cout << "  [1/3] Floyd-Warshall distances: pre-computed [DONE]\n";

    RouteResult result = optimizer.optimizeRoute(currentUser.userId, db);

    if (!result.success) {
        cout << "  [X] " << result.message << "\n";
        return;
    }

    cout << "  [2/3] TSP Branch & Bound: optimal sequence found [DONE]\n";
    cout << "  [3/3] Dijkstra navigation: paths computed [DONE]\n";

    // ── Display TSP Optimal Sequence ────────────────────────────────────────
    cout << "\n";
    printSeparator(60);
    cout << "  OPTIMAL DELIVERY SEQUENCE\n";
    printSeparator(60);

    cout << "  Visit order: ";
    for (size_t i = 0; i < result.tspSequence.size(); i++) {
        Location loc = db.getLocationById(result.tspSequence[i]);
        if (i > 0) cout << " -> ";
        cout << loc.name;
    }
    cout << "\n";
    cout << "  Total optimized distance: " << fixed << setprecision(2)
         << result.totalDistance << " km\n";

    // ── Display Turn-by-Turn Navigation ─────────────────────────────────────
    cout << "\n";
    printSeparator(60);
    cout << "  TURN-BY-TURN NAVIGATION (Dijkstra)\n";
    printSeparator(60);

    for (size_t i = 0; i < result.legs.size(); i++) {
        const RouteLeg& leg = result.legs[i];
        Location from = db.getLocationById(leg.fromLocId);
        Location to   = db.getLocationById(leg.toLocId);

        cout << "\n  LEG " << (i + 1) << ": "
             << from.name << " -> " << to.name
             << " (" << fixed << setprecision(2) << leg.distance << " km)\n";

        cout << "  Path: ";
        for (size_t j = 0; j < leg.path.size(); j++) {
            Location node = db.getLocationById(leg.path[j]);
            if (j > 0) cout << " -> ";
            cout << node.name;
        }
        cout << "\n";

        // Show intermediate road segments
        if (leg.path.size() > 2) {
            cout << "  Segments:\n";
            for (size_t j = 0; j < leg.path.size() - 1; j++) {
                Location a = db.getLocationById(leg.path[j]);
                Location b = db.getLocationById(leg.path[j + 1]);
                double segDist = optimizer.getShortestDistance(leg.path[j], leg.path[j + 1]);
                cout << "    " << (j + 1) << ". " << a.name << " -> " << b.name
                     << " (" << fixed << setprecision(2) << segDist << " km)\n";
            }
        }
    }

    cout << "\n";
    printSeparator(60);
    cout << "  Route optimization complete. Drive safe!\n";
    printSeparator(60);

    // Update driver status to On_Route
    db.updateDriverStatus(currentUser.userId, "On_Route");
    cout << "  [OK] Driver status updated to 'On_Route'.\n";
}

// ── Driver: Mark Delivery Complete ──────────────────────────────────────────

void DriverDashboard::markDeliveryComplete() {
    // Show pending deliveries first
    auto deliveries = db.getPendingDeliveriesByDriver(currentUser.userId);
    if (deliveries.empty()) {
        cout << "\n  No pending deliveries to complete.\n";
        return;
    }

    cout << "\n  Your pending deliveries:\n";
    for (const auto& del : deliveries) {
        Location loc = db.getLocationById(del.locId);
        cout << "    Delivery #" << del.deliveryId
             << " -> " << loc.name << "\n";
    }

    int delId = getIntInput("  Enter Delivery ID to mark complete: ");

    if (db.updateDeliveryStatus(delId, "Completed")) {
        // Update driver's current location to the delivery destination
        // Find which delivery was completed to get its loc_id
        for (const auto& del : deliveries) {
            if (del.deliveryId == delId) {
                db.updateDriverLocation(currentUser.userId, del.locId);
                Location loc = db.getLocationById(del.locId);
                cout << "  [OK] Delivery #" << delId << " marked as Completed.\n";
                cout << "  [OK] Current location updated to: " << loc.name << "\n";
                break;
            }
        }

        // Check if all deliveries are now complete
        auto remaining = db.getPendingDeliveriesByDriver(currentUser.userId);
        if (remaining.empty()) {
            cout << "  [OK] All deliveries completed! Status updated to 'Available'.\n";
            db.updateDriverStatus(currentUser.userId, "Available");
            db.updateDriverLocation(currentUser.userId, 1); // Return to warehouse
            cout << "  [OK] Returned to Central Warehouse.\n";
        }
    } else {
        cout << "  [X] Failed to update delivery status.\n";
    }
}

// ── Driver: View Profile ────────────────────────────────────────────────────

void DriverDashboard::viewMyProfile() {
    DriverDetails details = db.getDriverDetailsById(currentUser.userId);
    Location loc = db.getLocationById(details.currentLocId);

    cout << "\n";
    printSeparator(40);
    cout << "  DRIVER PROFILE\n";
    printSeparator(40);
    cout << "  User ID:     " << currentUser.userId << "\n";
    cout << "  Username:    " << currentUser.username << "\n";
    cout << "  License No:  " << details.licenseNo << "\n";
    cout << "  Capacity:    " << details.capacity << " packages\n";
    cout << "  Status:      " << details.status << "\n";
    cout << "  Location:    " << loc.name << " (ID: " << loc.locId << ")\n";
    printSeparator(40);
}
