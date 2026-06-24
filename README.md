# RouteMaster - Intelligent Logistics Route Optimizer

A console-based logistics application that integrates **OOP**, **MySQL database management**, and a **DAA algorithm stack** (Floyd-Warshall, TSP Branch & Bound, Dijkstra) to optimize delivery routes.

## Architecture

**3-Tier Layered Architecture:**

| Tier | Layer | Files |
|------|-------|-------|
| 1 | **Presentation** (CLI Dashboards) | `UI_Views.h` / `UI_Views.cpp` |
| 2 | **Business Logic** (Algorithms) | `RouteOptimizer.h` / `RouteOptimizer.cpp` |
| 3 | **Data Access** (MySQL CRUD) | `DatabaseManager.h` / `DatabaseManager.cpp` |

## Features

- **Admin Dashboard** — Manage locations, routes, drivers, deliveries, and users
- **Driver Dashboard** — View pending deliveries, optimize routes, mark completions
- **Floyd-Warshall** — All-pairs shortest path (Dynamic Programming)
- **TSP Branch & Bound** — Optimal delivery sequencing
- **Dijkstra** — Turn-by-turn navigation (Greedy)
- **Overdue Alerts** — Flags deliveries pending > 4 hours
- **Elapsed Time Tracking** — Shows how long ago each delivery was assigned

## Tech Stack

- **Language:** C++17
- **Compiler:** MSYS2 MinGW-w64 (GCC 15.2)
- **Database:** MySQL Server 8.0 (C API)
- **Build:** Batch script (`do_build.bat`)

## Prerequisites

1. [MSYS2](https://www.msys2.org/) with MinGW-w64 GCC installed
2. [MySQL Server 8.0](https://dev.mysql.com/downloads/mysql/)
3. Run the schema: `database/routemaster_schema.sql` in MySQL Workbench

## Build & Run

```bash
# 1. Load database schema (first time only)
mysql -u root -p < database/routemaster_schema.sql

# 2. Update MySQL credentials in src/main.cpp (line 37-42)

# 3. Build
do_build.bat

# 4. Run
./build/routemaster.exe
```

## Default Login Credentials

| Role | Username | Password |
|------|----------|----------|
| Admin | `admin` | `admin123` |
| Driver | `driver` | `driver123` |

## Project Structure

```
RouteMaster/
├── database/
│   └── routemaster_schema.sql    # DB schema + seed data (8 locations, 15 roads)
├── include/
│   ├── Models.h                  # Domain model classes
│   ├── DatabaseManager.h         # Data access layer header
│   ├── RouteOptimizer.h          # Algorithm layer header
│   └── UI_Views.h                # Presentation layer header
├── src/
│   ├── Models.cpp                # Model constructors & display
│   ├── DatabaseManager.cpp       # Full MySQL CRUD (SQL injection safe)
│   ├── RouteOptimizer.cpp        # Floyd-Warshall, TSP B&B, Dijkstra
│   ├── UI_Views.cpp              # Admin & Driver CLI dashboards
│   └── main.cpp                  # Entry point (wires 3 tiers)
├── do_build.bat                  # Build script
├── Makefile                      # Alternative build config
└── README.md
```
