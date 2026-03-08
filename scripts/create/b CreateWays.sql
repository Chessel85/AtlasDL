--Geometry table for ways 
CREATE TABLE spt_ways  (
  wayId INTEGER PRIMARY KEY,
  firstNodeId INTEGER,
  lastNodeId INTEGER
);

--index the firstNodeId needed for extracting strings of coastline 
CREATE INDEX idx_spt_ways_firstNodeId ON spt_ways(firstNodeId);

--Geometry column for holding the raw way   and different simplified verisonversions 
SELECT AddGeometryColumn ( 'spt_ways', 'way1', 4326, 'LINESTRING', 'XY');

SELECT AddGeometryColumn ( 'spt_ways', 'way2', 4326, 'LINESTRING', 'XY');

SELECT AddGeometryColumn ( 'spt_ways', 'way3', 4326, 'LINESTRING', 'XY');

SELECT AddGeometryColumn ( 'spt_ways', 'way4', 4326, 'LINESTRING', 'XY');


--spatial index on the way geometries
SELECT CreateSpatialIndex('spt_ways', 'way1');
SELECT CreateSpatialIndex('spt_ways', 'way2');
SELECT CreateSpatialIndex('spt_ways', 'way3');
SELECT CreateSpatialIndex('spt_ways', 'way4');
