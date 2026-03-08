--Table to store geometries  relating to parent elements 
--Need two denormalised columns to speed up rendering 
CREATE TABLE spt_polygons (
  polygonId INTEGER PRIMARY KEY AUTOINCREMENT,
  area REAL DEFAULT NULL,
  displayMin INTEGER DEFAULT 1,  --display from step 1 and above 
  displayMax INTEGER DEFAULT 20, -- display from step 20 and less 
    layerId INTEGER,
    colourIndex INTEGER
);

--Geometry columns for holding polygons at different levels of simplification 
SELECT AddGeometryColumn
  ( 'spt_polygons', 'polygon1', 4326, 'MULTIPOLYGON', 'XY');
SELECT AddGeometryColumn
  ( 'spt_polygons', 'polygon2', 4326, 'MULTIPOLYGON', 'XY');
SELECT AddGeometryColumn
  ( 'spt_polygons', 'polygon3', 4326, 'MULTIPOLYGON', 'XY');
SELECT AddGeometryColumn
  ( 'spt_polygons', 'polygon4', 4326, 'MULTIPOLYGON', 'XY');


--create an index for each polygon column 
SELECT CreateSpatialIndex('spt_polygons', 'polygon1');
SELECT CreateSpatialIndex('spt_polygons', 'polygon2');
SELECT CreateSpatialIndex('spt_polygons', 'polygon3');
SELECT CreateSpatialIndex('spt_polygons', 'polygon4');
