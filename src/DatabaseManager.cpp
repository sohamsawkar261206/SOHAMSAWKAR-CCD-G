// ============================================================================
// DatabaseManager.cpp — Data Access Layer Implementation (MySQL C API)
//
// Uses mysql_real_escape_string() for SQL injection safety on all user inputs.
// All SQL in the entire application is isolated in this single file.
// Errors are logged to cerr; callers check return values.
// ============================================================================

#include "DatabaseManager.h"
#include <cstring>
#include <cstdlib>
#include <sstream>

using namespace std;

// Constant used for unreachable edges in the adjacency matrix
static const double INF = 1e9;


// ═══════════════════════════════════════════════════════════════════════════
// Helper: Escape string for SQL injection prevention
// ═══════════════════════════════════════════════════════════════════════════

string DatabaseManager::escapeString(const string& input) {
    if (!conn || input.empty()) return input;
    // Output buffer must be 2x input + 1 (worst case: every char is escaped)
    vector<char> buffer(input.length() * 2 + 1);
    mysql_real_escape_string(conn, buffer.data(), input.c_str(),
                              static_cast<unsigned long>(input.length()));
    return string(buffer.data());
}


// ═══════════════════════════════════════════════════════════════════════════
// Lifecycle
// ═══════════════════════════════════════════════════════════════════════════

DatabaseManager::DatabaseManager(const string& host, const string& user,
                                 const string& password, const string& database,
                                 unsigned int port)
    : conn(nullptr), host(host), dbUser(user), dbPassword(password),
      dbName(database), port(port) {}

DatabaseManager::~DatabaseManager() {
    disconnect();
}

bool DatabaseManager::connect() {
    // Initialize the MySQL C API connection handle
    conn = mysql_init(nullptr);
    if (!conn) {
        cerr << "[DB ERROR] mysql_init() failed - out of memory." << endl;
        return false;
    }

    // Connect to the MySQL server
    if (!mysql_real_connect(conn, host.c_str(), dbUser.c_str(),
                            dbPassword.c_str(), dbName.c_str(),
                            port, nullptr, 0)) {
        cerr << "[DB ERROR] Connection failed!" << endl;
        cerr << "    Code:    " << mysql_errno(conn) << endl;
        cerr << "    Message: " << mysql_error(conn) << endl;
        mysql_close(conn);
        conn = nullptr;
        return false;
    }

    cout << "[DB] Connected to " << dbName << " at " << host << endl;
    return true;
}

void DatabaseManager::disconnect() {
    if (conn) {
        mysql_close(conn);
        conn = nullptr;
        cout << "[DB] Disconnected." << endl;
    }
}

bool DatabaseManager::isConnected() const {
    return conn != nullptr;
}


// ═══════════════════════════════════════════════════════════════════════════
// USER CRUD
// ═══════════════════════════════════════════════════════════════════════════

bool DatabaseManager::insertUser(const User& user) {
    string query = "INSERT INTO USER (username, password, role) VALUES ('" +
        escapeString(user.username) + "', '" +
        escapeString(user.password) + "', '" +
        escapeString(user.role) + "')";

    if (mysql_query(conn, query.c_str())) {
        cerr << "[DB ERROR] insertUser: " << mysql_error(conn) << endl;
        return false;
    }
    return true;
}

User DatabaseManager::getUserById(int userId) {
    User user;
    string query = "SELECT * FROM USER WHERE user_id = " + to_string(userId);

    if (mysql_query(conn, query.c_str())) {
        cerr << "[DB ERROR] getUserById: " << mysql_error(conn) << endl;
        return user;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result) {
        MYSQL_ROW row = mysql_fetch_row(result);
        if (row) {
            user.userId   = atoi(row[0]);
            user.username = row[1] ? row[1] : "";
            user.password = row[2] ? row[2] : "";
            user.role     = row[3] ? row[3] : "";
        }
        mysql_free_result(result);
    }
    return user;
}

User DatabaseManager::getUserByUsername(const string& username) {
    User user;
    string query = "SELECT * FROM USER WHERE username = '" +
        escapeString(username) + "'";

    if (mysql_query(conn, query.c_str())) {
        cerr << "[DB ERROR] getUserByUsername: " << mysql_error(conn) << endl;
        return user;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result) {
        MYSQL_ROW row = mysql_fetch_row(result);
        if (row) {
            user.userId   = atoi(row[0]);
            user.username = row[1] ? row[1] : "";
            user.password = row[2] ? row[2] : "";
            user.role     = row[3] ? row[3] : "";
        }
        mysql_free_result(result);
    }
    return user;
}

vector<User> DatabaseManager::getAllUsers() {
    vector<User> users;

    if (mysql_query(conn, "SELECT * FROM USER ORDER BY user_id")) {
        cerr << "[DB ERROR] getAllUsers: " << mysql_error(conn) << endl;
        return users;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result))) {
            users.emplace_back(
                atoi(row[0]),
                row[1] ? row[1] : "",
                row[2] ? row[2] : "",
                row[3] ? row[3] : ""
            );
        }
        mysql_free_result(result);
    }
    return users;
}

bool DatabaseManager::deleteUser(int userId) {
    string query = "DELETE FROM USER WHERE user_id = " + to_string(userId);

    if (mysql_query(conn, query.c_str())) {
        cerr << "[DB ERROR] deleteUser: " << mysql_error(conn) << endl;
        return false;
    }
    return mysql_affected_rows(conn) > 0;
}

User DatabaseManager::authenticateUser(const string& username, const string& password) {
    User user;
    string query = "SELECT * FROM USER WHERE username = '" +
        escapeString(username) + "' AND password = '" +
        escapeString(password) + "'";

    if (mysql_query(conn, query.c_str())) {
        cerr << "[DB ERROR] authenticateUser: " << mysql_error(conn) << endl;
        return user;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result) {
        MYSQL_ROW row = mysql_fetch_row(result);
        if (row) {
            user.userId   = atoi(row[0]);
            user.username = row[1] ? row[1] : "";
            user.password = row[2] ? row[2] : "";
            user.role     = row[3] ? row[3] : "";
        }
        mysql_free_result(result);
    }
    return user;   // userId == 0 means authentication failed
}


// ═══════════════════════════════════════════════════════════════════════════
// DRIVER_DETAILS CRUD
// ═══════════════════════════════════════════════════════════════════════════

bool DatabaseManager::insertDriverDetails(const DriverDetails& details) {
    string query = "INSERT INTO DRIVER_DETAILS (driver_id, license_no, capacity, "
                   "status, current_loc_id) VALUES (" +
        to_string(details.driverId) + ", '" +
        escapeString(details.licenseNo) + "', " +
        to_string(details.capacity) + ", '" +
        escapeString(details.status) + "', " +
        to_string(details.currentLocId) + ")";

    if (mysql_query(conn, query.c_str())) {
        cerr << "[DB ERROR] insertDriverDetails: " << mysql_error(conn) << endl;
        return false;
    }
    return true;
}

DriverDetails DatabaseManager::getDriverDetailsById(int driverId) {
    DriverDetails details;
    string query = "SELECT * FROM DRIVER_DETAILS WHERE driver_id = " +
        to_string(driverId);

    if (mysql_query(conn, query.c_str())) {
        cerr << "[DB ERROR] getDriverDetailsById: " << mysql_error(conn) << endl;
        return details;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result) {
        MYSQL_ROW row = mysql_fetch_row(result);
        if (row) {
            details.driverId     = atoi(row[0]);
            details.licenseNo    = row[1] ? row[1] : "";
            details.capacity     = atoi(row[2]);
            details.status       = row[3] ? row[3] : "";
            details.currentLocId = atoi(row[4]);
        }
        mysql_free_result(result);
    }
    return details;
}

vector<DriverDetails> DatabaseManager::getAllDriverDetails() {
    vector<DriverDetails> drivers;

    if (mysql_query(conn, "SELECT * FROM DRIVER_DETAILS ORDER BY driver_id")) {
        cerr << "[DB ERROR] getAllDriverDetails: " << mysql_error(conn) << endl;
        return drivers;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result))) {
            drivers.emplace_back(
                atoi(row[0]),
                row[1] ? row[1] : "",
                atoi(row[2]),
                row[3] ? row[3] : "",
                atoi(row[4])
            );
        }
        mysql_free_result(result);
    }
    return drivers;
}

vector<DriverDetails> DatabaseManager::getAvailableDrivers() {
    vector<DriverDetails> drivers;

    if (mysql_query(conn, "SELECT * FROM DRIVER_DETAILS WHERE status = 'Available'")) {
        cerr << "[DB ERROR] getAvailableDrivers: " << mysql_error(conn) << endl;
        return drivers;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result))) {
            drivers.emplace_back(
                atoi(row[0]),
                row[1] ? row[1] : "",
                atoi(row[2]),
                row[3] ? row[3] : "",
                atoi(row[4])
            );
        }
        mysql_free_result(result);
    }
    return drivers;
}

bool DatabaseManager::updateDriverStatus(int driverId, const string& status) {
    string query = "UPDATE DRIVER_DETAILS SET status = '" +
        escapeString(status) + "' WHERE driver_id = " + to_string(driverId);

    if (mysql_query(conn, query.c_str())) {
        cerr << "[DB ERROR] updateDriverStatus: " << mysql_error(conn) << endl;
        return false;
    }
    return mysql_affected_rows(conn) > 0;
}

bool DatabaseManager::updateDriverLocation(int driverId, int locId) {
    string query = "UPDATE DRIVER_DETAILS SET current_loc_id = " +
        to_string(locId) + " WHERE driver_id = " + to_string(driverId);

    if (mysql_query(conn, query.c_str())) {
        cerr << "[DB ERROR] updateDriverLocation: " << mysql_error(conn) << endl;
        return false;
    }
    return mysql_affected_rows(conn) > 0;
}

bool DatabaseManager::deleteDriverDetails(int driverId) {
    string query = "DELETE FROM DRIVER_DETAILS WHERE driver_id = " +
        to_string(driverId);

    if (mysql_query(conn, query.c_str())) {
        cerr << "[DB ERROR] deleteDriverDetails: " << mysql_error(conn) << endl;
        return false;
    }
    return mysql_affected_rows(conn) > 0;
}


// ═══════════════════════════════════════════════════════════════════════════
// LOCATION CRUD
// ═══════════════════════════════════════════════════════════════════════════

bool DatabaseManager::insertLocation(const Location& loc) {
    // Use ostringstream for floating-point precision in SQL
    ostringstream oss;
    oss << "INSERT INTO LOCATION (name, latitude, longitude) VALUES ('"
        << escapeString(loc.name) << "', "
        << fixed << loc.latitude << ", "
        << fixed << loc.longitude << ")";

    if (mysql_query(conn, oss.str().c_str())) {
        cerr << "[DB ERROR] insertLocation: " << mysql_error(conn) << endl;
        return false;
    }
    return true;
}

Location DatabaseManager::getLocationById(int locId) {
    Location loc;
    string query = "SELECT * FROM LOCATION WHERE loc_id = " + to_string(locId);

    if (mysql_query(conn, query.c_str())) {
        cerr << "[DB ERROR] getLocationById: " << mysql_error(conn) << endl;
        return loc;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result) {
        MYSQL_ROW row = mysql_fetch_row(result);
        if (row) {
            loc.locId     = atoi(row[0]);
            loc.name      = row[1] ? row[1] : "";
            loc.latitude  = atof(row[2]);
            loc.longitude = atof(row[3]);
        }
        mysql_free_result(result);
    }
    return loc;
}

vector<Location> DatabaseManager::getAllLocations() {
    vector<Location> locations;

    if (mysql_query(conn, "SELECT * FROM LOCATION ORDER BY loc_id")) {
        cerr << "[DB ERROR] getAllLocations: " << mysql_error(conn) << endl;
        return locations;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result))) {
            locations.emplace_back(
                atoi(row[0]),
                row[1] ? row[1] : "",
                atof(row[2]),
                atof(row[3])
            );
        }
        mysql_free_result(result);
    }
    return locations;
}

bool DatabaseManager::deleteLocation(int locId) {
    string query = "DELETE FROM LOCATION WHERE loc_id = " + to_string(locId);

    if (mysql_query(conn, query.c_str())) {
        cerr << "[DB ERROR] deleteLocation: " << mysql_error(conn) << endl;
        return false;
    }
    return mysql_affected_rows(conn) > 0;
}

int DatabaseManager::getLocationCount() {
    if (mysql_query(conn, "SELECT COUNT(*) AS cnt FROM LOCATION")) {
        cerr << "[DB ERROR] getLocationCount: " << mysql_error(conn) << endl;
        return 0;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    int count = 0;
    if (result) {
        MYSQL_ROW row = mysql_fetch_row(result);
        if (row && row[0]) {
            count = atoi(row[0]);
        }
        mysql_free_result(result);
    }
    return count;
}


// ═══════════════════════════════════════════════════════════════════════════
// DELIVERY CRUD
// ═══════════════════════════════════════════════════════════════════════════

bool DatabaseManager::insertDelivery(const Delivery& delivery) {
    string query = "INSERT INTO DELIVERY (driver_id, loc_id, status) VALUES (" +
        to_string(delivery.driverId) + ", " +
        to_string(delivery.locId) + ", '" +
        escapeString(delivery.status) + "')";

    if (mysql_query(conn, query.c_str())) {
        cerr << "[DB ERROR] insertDelivery: " << mysql_error(conn) << endl;
        return false;
    }
    return true;
}

Delivery DatabaseManager::getDeliveryById(int deliveryId) {
    Delivery del;
    string query = "SELECT * FROM DELIVERY WHERE delivery_id = " +
        to_string(deliveryId);

    if (mysql_query(conn, query.c_str())) {
        cerr << "[DB ERROR] getDeliveryById: " << mysql_error(conn) << endl;
        return del;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result) {
        MYSQL_ROW row = mysql_fetch_row(result);
        if (row) {
            del.deliveryId   = atoi(row[0]);
            del.driverId     = atoi(row[1]);
            del.locId        = atoi(row[2]);
            del.status       = row[3] ? row[3] : "";
            del.assignedTime = row[4] ? row[4] : "";
        }
        mysql_free_result(result);
    }
    return del;
}

vector<Delivery> DatabaseManager::getAllDeliveries() {
    vector<Delivery> deliveries;

    if (mysql_query(conn, "SELECT * FROM DELIVERY ORDER BY delivery_id")) {
        cerr << "[DB ERROR] getAllDeliveries: " << mysql_error(conn) << endl;
        return deliveries;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result))) {
            deliveries.emplace_back(
                atoi(row[0]),
                atoi(row[1]),
                atoi(row[2]),
                row[3] ? row[3] : "",
                row[4] ? row[4] : ""
            );
        }
        mysql_free_result(result);
    }
    return deliveries;
}

vector<Delivery> DatabaseManager::getDeliveriesByDriver(int driverId) {
    vector<Delivery> deliveries;
    string query = "SELECT * FROM DELIVERY WHERE driver_id = " +
        to_string(driverId) + " ORDER BY delivery_id";

    if (mysql_query(conn, query.c_str())) {
        cerr << "[DB ERROR] getDeliveriesByDriver: " << mysql_error(conn) << endl;
        return deliveries;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result))) {
            deliveries.emplace_back(
                atoi(row[0]),
                atoi(row[1]),
                atoi(row[2]),
                row[3] ? row[3] : "",
                row[4] ? row[4] : ""
            );
        }
        mysql_free_result(result);
    }
    return deliveries;
}

vector<Delivery> DatabaseManager::getPendingDeliveriesByDriver(int driverId) {
    vector<Delivery> deliveries;
    string query = "SELECT * FROM DELIVERY WHERE driver_id = " +
        to_string(driverId) + " AND status = 'Pending' ORDER BY delivery_id";

    if (mysql_query(conn, query.c_str())) {
        cerr << "[DB ERROR] getPendingDeliveriesByDriver: " << mysql_error(conn) << endl;
        return deliveries;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result))) {
            deliveries.emplace_back(
                atoi(row[0]),
                atoi(row[1]),
                atoi(row[2]),
                row[3] ? row[3] : "",
                row[4] ? row[4] : ""
            );
        }
        mysql_free_result(result);
    }
    return deliveries;
}

bool DatabaseManager::updateDeliveryStatus(int deliveryId, const string& status) {
    string query = "UPDATE DELIVERY SET status = '" +
        escapeString(status) + "' WHERE delivery_id = " + to_string(deliveryId);

    if (mysql_query(conn, query.c_str())) {
        cerr << "[DB ERROR] updateDeliveryStatus: " << mysql_error(conn) << endl;
        return false;
    }
    return mysql_affected_rows(conn) > 0;
}

bool DatabaseManager::deleteDelivery(int deliveryId) {
    string query = "DELETE FROM DELIVERY WHERE delivery_id = " +
        to_string(deliveryId);

    if (mysql_query(conn, query.c_str())) {
        cerr << "[DB ERROR] deleteDelivery: " << mysql_error(conn) << endl;
        return false;
    }
    return mysql_affected_rows(conn) > 0;
}

int DatabaseManager::getPendingDeliveryCount(int driverId) {
    string query = "SELECT COUNT(*) AS cnt FROM DELIVERY WHERE driver_id = " +
        to_string(driverId) + " AND status = 'Pending'";

    if (mysql_query(conn, query.c_str())) {
        cerr << "[DB ERROR] getPendingDeliveryCount: " << mysql_error(conn) << endl;
        return 0;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    int count = 0;
    if (result) {
        MYSQL_ROW row = mysql_fetch_row(result);
        if (row && row[0]) {
            count = atoi(row[0]);
        }
        mysql_free_result(result);
    }
    return count;
}


// ═══════════════════════════════════════════════════════════════════════════
// ROADMAP CRUD
// ═══════════════════════════════════════════════════════════════════════════

bool DatabaseManager::insertRoadmap(const Roadmap& road) {
    ostringstream oss;
    oss << "INSERT INTO ROADMAP (source_id, dest_id, distance) VALUES ("
        << road.sourceId << ", "
        << road.destId << ", "
        << fixed << road.distance << ")";

    if (mysql_query(conn, oss.str().c_str())) {
        cerr << "[DB ERROR] insertRoadmap: " << mysql_error(conn) << endl;
        return false;
    }
    return true;
}

vector<Roadmap> DatabaseManager::getAllRoadmaps() {
    vector<Roadmap> roads;

    if (mysql_query(conn, "SELECT * FROM ROADMAP ORDER BY route_id")) {
        cerr << "[DB ERROR] getAllRoadmaps: " << mysql_error(conn) << endl;
        return roads;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result))) {
            roads.emplace_back(
                atoi(row[0]),
                atoi(row[1]),
                atoi(row[2]),
                atof(row[3])
            );
        }
        mysql_free_result(result);
    }
    return roads;
}

bool DatabaseManager::deleteRoadmap(int routeId) {
    string query = "DELETE FROM ROADMAP WHERE route_id = " + to_string(routeId);

    if (mysql_query(conn, query.c_str())) {
        cerr << "[DB ERROR] deleteRoadmap: " << mysql_error(conn) << endl;
        return false;
    }
    return mysql_affected_rows(conn) > 0;
}


// ═══════════════════════════════════════════════════════════════════════════
// Graph Builder — Bridge between DBMS and DAA layers
// ═══════════════════════════════════════════════════════════════════════════

vector<vector<double>> DatabaseManager::getAdjacencyMatrix() {
    int n = getLocationCount();

    // Create (N+1) x (N+1) matrix, 1-indexed. Index 0 is unused.
    // Initialize all cells to INF (unreachable).
    vector<vector<double>> matrix(n + 1, vector<double>(n + 1, INF));

    // Distance from any node to itself is 0
    for (int i = 0; i <= n; i++) {
        matrix[i][i] = 0.0;
    }

    // Load all road segments from the database
    vector<Roadmap> roads = getAllRoadmaps();

    // Populate the matrix — treating each edge as BIDIRECTIONAL
    for (const auto& road : roads) {
        if (road.sourceId <= n && road.destId <= n) {
            matrix[road.sourceId][road.destId] = road.distance;
            matrix[road.destId][road.sourceId] = road.distance;  // Bidirectional
        }
    }

    cout << "[DB] Adjacency matrix built: " << n << " nodes, "
         << roads.size() << " edges (bidirectional)." << endl;

    return matrix;
}
