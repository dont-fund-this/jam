-- AIS Vessel Tracks table for maritime tracking data
-- Optimized for high-performance analytics and spatial queries

CREATE TABLE ais_tracks (
    mmsi INTEGER NOT NULL,                    -- Maritime Mobile Service Identity
    base_datetime TEXT NOT NULL,             -- UTC timestamp in ISO format
    lat REAL NOT NULL,                       -- Latitude (decimal degrees)
    lon REAL NOT NULL,                       -- Longitude (decimal degrees)
    sog REAL,                                -- Speed Over Ground (knots)
    cog REAL,                                -- Course Over Ground (degrees)
    heading REAL,                            -- Vessel heading (degrees)
    vessel_name TEXT,                        -- Ship name
    imo TEXT,                                -- International Maritime Organization number
    call_sign TEXT,                          -- Radio call sign
    vessel_type INTEGER,                     -- Ship type code
    status INTEGER,                          -- Navigation status
    length INTEGER,                          -- Vessel length (meters)
    width INTEGER,                           -- Vessel width (meters)
    draft REAL,                              -- Vessel draft (meters)
    cargo INTEGER,                           -- Cargo type code
    transceiver_class TEXT,                  -- AIS equipment class (A/B)
    PRIMARY KEY (mmsi, base_datetime)
) WITHOUT ROWID;

-- Performance indexes for common queries (COMMENTED OUT FOR MAXIMUM INSERT PERFORMANCE)
-- CREATE INDEX IF NOT EXISTS idx_ais_mmsi ON ais_tracks(mmsi);
-- CREATE INDEX IF NOT EXISTS idx_ais_datetime ON ais_tracks(base_datetime);
-- CREATE INDEX IF NOT EXISTS idx_ais_location ON ais_tracks(lat, lon);
-- CREATE INDEX IF NOT EXISTS idx_ais_vessel_type ON ais_tracks(vessel_type);
-- CREATE INDEX IF NOT EXISTS idx_ais_vessel_name ON ais_tracks(vessel_name);

-- GO --