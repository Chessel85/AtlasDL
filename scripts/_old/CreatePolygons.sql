--Table to store geometries  relating to parent elements 
CREATE TABLE spt_polygons (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  elementType INTEGER,
  elementID INTEGER,
  area REAL
);

--Geometry column for holding polygons 
SELECT AddGeometryColumn
  ( 'spt_Polygons', 'polygon', 4326, 'GEOMETRY', 'XY');


--create an index for polygons 
SELECT CreateSpatialIndex('spt_Polygons', 'polygon');

