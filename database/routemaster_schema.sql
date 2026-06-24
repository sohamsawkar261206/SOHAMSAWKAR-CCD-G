-- ============================================================================
-- RouteMaster Database Schema
-- ============================================================================
-- Run in MySQL Workbench:  File → Open SQL Script → Execute (⚡)
-- This script drops and recreates the entire database with seed data.
-- ============================================================================

DROP DATABASE IF EXISTS routemaster_db;
CREATE DATABASE routemaster_db;
USE routemaster_db;

-- ────────────────────────────────────────────────────────────────────────────
-- Table 1: USER
-- Stores all system users (Admins and Drivers).
-- The 'role' column determines dashboard access.
-- ────────────────────────────────────────────────────────────────────────────
CREATE TABLE USER (
    user_id     INT AUTO_INCREMENT PRIMARY KEY,
    username    VARCHAR(50)  NOT NULL UNIQUE,
    password    VARCHAR(255) NOT NULL,
    role        ENUM('Admin', 'Driver') NOT NULL
) ENGINE=InnoDB;

-- ────────────────────────────────────────────────────────────────────────────
-- Table 2: LOCATION
-- Represents nodes in the road graph.
-- Created before DRIVER_DETAILS because of FK dependency.
-- ────────────────────────────────────────────────────────────────────────────
CREATE TABLE LOCATION (
    loc_id      INT AUTO_INCREMENT PRIMARY KEY,
    name        VARCHAR(100) NOT NULL,
    latitude    DECIMAL(10, 7) NOT NULL,
    longitude   DECIMAL(10, 7) NOT NULL
) ENGINE=InnoDB;

-- ────────────────────────────────────────────────────────────────────────────
-- Table 3: DRIVER_DETAILS
-- Extended profile for users with role='Driver'.
-- driver_id is both PK and FK to USER (1:1 relationship).
-- current_loc_id defaults to Location 1 (the central warehouse).
-- ────────────────────────────────────────────────────────────────────────────
CREATE TABLE DRIVER_DETAILS (
    driver_id       INT PRIMARY KEY,
    license_no      VARCHAR(20) NOT NULL UNIQUE,
    capacity        INT NOT NULL DEFAULT 5,
    status          ENUM('Available', 'On_Route', 'Off_Duty') DEFAULT 'Available',
    current_loc_id  INT DEFAULT 1,

    FOREIGN KEY (driver_id)       REFERENCES USER(user_id)     ON DELETE CASCADE,
    FOREIGN KEY (current_loc_id)  REFERENCES LOCATION(loc_id)  ON UPDATE CASCADE
) ENGINE=InnoDB;

-- ────────────────────────────────────────────────────────────────────────────
-- Table 4: DELIVERY
-- Each row is a package assigned to a driver, destined for a location.
-- Status tracks lifecycle: Pending → Completed.
-- ────────────────────────────────────────────────────────────────────────────
CREATE TABLE DELIVERY (
    delivery_id     INT AUTO_INCREMENT PRIMARY KEY,
    driver_id       INT NOT NULL,
    loc_id          INT NOT NULL,
    status          ENUM('Pending', 'Completed') DEFAULT 'Pending',
    assigned_time   TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (driver_id) REFERENCES DRIVER_DETAILS(driver_id) ON DELETE CASCADE,
    FOREIGN KEY (loc_id)    REFERENCES LOCATION(loc_id)          ON UPDATE CASCADE
) ENGINE=InnoDB;

-- ────────────────────────────────────────────────────────────────────────────
-- Table 5: ROADMAP
-- Sparse graph edges. Each row = one directed road segment.
-- Treated as BIDIRECTIONAL in application code (A→B implies B→A).
-- ────────────────────────────────────────────────────────────────────────────
CREATE TABLE ROADMAP (
    route_id    INT AUTO_INCREMENT PRIMARY KEY,
    source_id   INT NOT NULL,
    dest_id     INT NOT NULL,
    distance    DECIMAL(10, 2) NOT NULL,

    FOREIGN KEY (source_id) REFERENCES LOCATION(loc_id) ON UPDATE CASCADE,
    FOREIGN KEY (dest_id)   REFERENCES LOCATION(loc_id) ON UPDATE CASCADE
) ENGINE=InnoDB;


-- ============================================================================
-- SEED DATA
-- ============================================================================

-- ── 8 Locations (Delhi NCR road network) ────────────────────────────────────
INSERT INTO LOCATION (name, latitude, longitude) VALUES
    ('Central Warehouse',  28.6139000, 77.2090000),   -- loc_id = 1  (Hub)
    ('North Hub',          28.7200000, 77.1025000),   -- loc_id = 2  (Rohini)
    ('East Market',        28.6312000, 77.2773000),   -- loc_id = 3  (Laxmi Nagar)
    ('South Plaza',        28.5244000, 77.2051000),   -- loc_id = 4  (Saket)
    ('West Point',         28.6275000, 77.0861000),   -- loc_id = 5  (Janakpuri)
    ('Airport Terminal',   28.5562000, 77.1000000),   -- loc_id = 6  (IGI Airport)
    ('University Campus',  28.5402000, 77.1663000),   -- loc_id = 7  (JNU)
    ('Tech Park',          28.5355000, 77.3910000);   -- loc_id = 8  (Noida)

-- ── 2 Users (1 Admin, 1 Driver) ─────────────────────────────────────────────
INSERT INTO USER (username, password, role) VALUES
    ('admin',  'admin123',  'Admin'),    -- user_id = 1
    ('driver', 'driver123', 'Driver');   -- user_id = 2

-- ── Driver Profile ──────────────────────────────────────────────────────────
INSERT INTO DRIVER_DETAILS (driver_id, license_no, capacity, status, current_loc_id) VALUES
    (2, 'DL-1420110012345', 5, 'Available', 1);

-- ── 15 Road Segments (bidirectional in code) ────────────────────────────────
-- These edges form a connected sparse graph over 8 nodes.
--
--        2 (North)
--       / |  \
--     12.5 18.4 25.0
--     /   |     \
--    1---8.2---3---11.5---8
--    |\ 14.7  /|          |
--    | 5    12.8          22.3
--   15.3  /   |          |
--    | 10.1  4---8.6---7
--    |/     |  \       /|
--    5---9.3-6--7.5--/ 
--         \16.9/
--
INSERT INTO ROADMAP (source_id, dest_id, distance) VALUES
    (1, 2, 12.50),   -- Central Warehouse ↔ North Hub
    (1, 3,  8.20),   -- Central Warehouse ↔ East Market
    (1, 4, 15.30),   -- Central Warehouse ↔ South Plaza
    (1, 5, 14.70),   -- Central Warehouse ↔ West Point
    (2, 3, 18.40),   -- North Hub ↔ East Market
    (2, 5, 10.10),   -- North Hub ↔ West Point
    (2, 8, 25.00),   -- North Hub ↔ Tech Park
    (3, 4, 12.80),   -- East Market ↔ South Plaza
    (3, 8, 11.50),   -- East Market ↔ Tech Park
    (4, 6, 16.90),   -- South Plaza ↔ Airport Terminal
    (4, 7,  8.60),   -- South Plaza ↔ University Campus
    (5, 6,  9.30),   -- West Point ↔ Airport Terminal
    (5, 7, 12.00),   -- West Point ↔ University Campus
    (6, 7,  7.50),   -- Airport Terminal ↔ University Campus
    (7, 8, 22.30);   -- University Campus ↔ Tech Park

-- ── 4 Pending Deliveries for Driver ─────────────────────────────────────────
INSERT INTO DELIVERY (driver_id, loc_id, status) VALUES
    (2, 3, 'Pending'),    -- Deliver to East Market
    (2, 5, 'Pending'),    -- Deliver to West Point
    (2, 7, 'Pending'),    -- Deliver to University Campus
    (2, 8, 'Pending');    -- Deliver to Tech Park

-- ============================================================================
-- Verification Queries (run manually to confirm)
-- ============================================================================
-- SELECT * FROM USER;
-- SELECT * FROM LOCATION;
-- SELECT * FROM DRIVER_DETAILS;
-- SELECT * FROM ROADMAP;
-- SELECT * FROM DELIVERY;
