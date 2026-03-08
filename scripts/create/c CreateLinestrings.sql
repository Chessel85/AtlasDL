--Geometry table for linestrings 
CREATE TABLE spt_linestrings (
    lineId INTEGER PRIMARY KEY AUTOINCREMENT,
    length REAL DEFAULT NULL,
    displayMin INTEGER DEFAULT 1,  -- display step size 1 and above 
    displayMax INTEGER DEFAULT 20 --display step size 20 and below 
);

-- Add 4 levels of detail
SELECT AddGeometryColumn('spt_linestrings', 'line1', 4326, 'MULTILINESTRING', 'XY');
SELECT AddGeometryColumn('spt_linestrings', 'line2', 4326, 'MULTILINESTRING', 'XY');
SELECT AddGeometryColumn('spt_linestrings', 'line3', 4326, 'MULTILINESTRING', 'XY');
SELECT AddGeometryColumn('spt_linestrings', 'line4', 4326, 'MULTILINESTRING', 'XY');

-- Spatial Indexes
SELECT CreateSpatialIndex('spt_linestrings', 'line1');
SELECT CreateSpatialIndex('spt_linestrings', 'line2');
SELECT CreateSpatialIndex('spt_linestrings', 'line3');
SELECT CreateSpatialIndex('spt_linestrings', 'line4');