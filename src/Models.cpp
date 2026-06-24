// ============================================================================
// Models.cpp — Domain Model Implementations
//
// Implements constructors and display() for all 5 domain classes.
// Uses <iomanip> for clean, tabular console output.
// ============================================================================

#include "Models.h"

using namespace std;

// ═══════════════════════════════════════════════════════════════════════════
// User
// ═══════════════════════════════════════════════════════════════════════════

User::User()
    : userId(0), username(""), password(""), role("") {}

User::User(int id, const string& uname, const string& pwd, const string& r)
    : userId(id), username(uname), password(pwd), role(r) {}

void User::display() const {
    cout << "| " << left << setw(8)  << userId
         << "| " << left << setw(15) << username
         << "| " << left << setw(10) << role
         << "|" << endl;
}

// ═══════════════════════════════════════════════════════════════════════════
// DriverDetails
// ═══════════════════════════════════════════════════════════════════════════

DriverDetails::DriverDetails()
    : driverId(0), licenseNo(""), capacity(0), status(""), currentLocId(1) {}

DriverDetails::DriverDetails(int dId, const string& license, int cap,
                             const string& stat, int locId)
    : driverId(dId), licenseNo(license), capacity(cap),
      status(stat), currentLocId(locId) {}

void DriverDetails::display() const {
    cout << "| " << left << setw(10) << driverId
         << "| " << left << setw(20) << licenseNo
         << "| " << left << setw(10) << capacity
         << "| " << left << setw(12) << status
         << "| " << left << setw(10) << currentLocId
         << "|" << endl;
}

// ═══════════════════════════════════════════════════════════════════════════
// Location
// ═══════════════════════════════════════════════════════════════════════════

Location::Location()
    : locId(0), name(""), latitude(0.0), longitude(0.0) {}

Location::Location(int id, const string& n, double lat, double lon)
    : locId(id), name(n), latitude(lat), longitude(lon) {}

void Location::display() const {
    cout << "| " << left  << setw(8)  << locId
         << "| " << left  << setw(22) << name
         << "| " << right << setw(12) << fixed << setprecision(7) << latitude
         << "| " << right << setw(12) << fixed << setprecision(7) << longitude
         << " |" << endl;
}

// ═══════════════════════════════════════════════════════════════════════════
// Delivery
// ═══════════════════════════════════════════════════════════════════════════

Delivery::Delivery()
    : deliveryId(0), driverId(0), locId(0), status(""), assignedTime("") {}

Delivery::Delivery(int delId, int drId, int lId, const string& stat,
                   const string& aTime)
    : deliveryId(delId), driverId(drId), locId(lId),
      status(stat), assignedTime(aTime) {}

void Delivery::display() const {
    cout << "| " << left << setw(12) << deliveryId
         << "| " << left << setw(10) << driverId
         << "| " << left << setw(8)  << locId
         << "| " << left << setw(12) << status
         << "| " << left << setw(22) << assignedTime
         << "|" << endl;
}

// ═══════════════════════════════════════════════════════════════════════════
// Roadmap
// ═══════════════════════════════════════════════════════════════════════════

Roadmap::Roadmap()
    : routeId(0), sourceId(0), destId(0), distance(0.0) {}

Roadmap::Roadmap(int rId, int srcId, int dstId, double dist)
    : routeId(rId), sourceId(srcId), destId(dstId), distance(dist) {}

void Roadmap::display() const {
    cout << "| " << left  << setw(10) << routeId
         << "| " << left  << setw(10) << sourceId
         << "| " << left  << setw(10) << destId
         << "| " << right << setw(10) << fixed << setprecision(2) << distance
         << " |" << endl;
}
