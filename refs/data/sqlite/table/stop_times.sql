-- Stop Times table for transit schedule data
-- Optimized for high-performance CSV import (no indexes for bulk loading)

CREATE TABLE stop_times (
    trip_id TEXT,
    arrival_time TEXT,
    departure_time TEXT,
    stop_id TEXT,
    stop_sequence INTEGER,
    stop_headsign TEXT,
    pickup_type INTEGER DEFAULT 0,
    drop_off_type INTEGER DEFAULT 0,
    shape_dist_traveled REAL,
    PRIMARY KEY (trip_id, stop_sequence)
) WITHOUT ROWID;

-- GO --