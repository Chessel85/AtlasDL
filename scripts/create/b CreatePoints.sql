--Table to store point geometries 
CREATE TABLE spt_points (
    pointId INTEGER PRIMARY KEY AUTOINCREMENT, 
    nodeId  INTEGER,  -- Stores the original OSM Node ID
    displayMin INTEGER DEFAULT 1,   --display from step 1 upwards by default 
    displayMax INTEGER DEFAULT 20 --display up to step size 20 by default  
);

-- Add a point geometry column 
SELECT AddGeometryColumn( 'spt_points', 'point', 4326, 'POINT', 'XY');

-- Create the spatial index
SELECT CreateSpatialIndex('spt_points', 'point');