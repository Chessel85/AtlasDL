--Table to store land polygons of countries with no terretorial waters  (NTW)
CREATE TABLE spt_NTWPolygons (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
    borderPolygonID INTEGER,
  elementType INTEGER,
  elementID INTEGER,
  area REAL
);

--Column for polygon geometry 
SELECT AddGeometryColumn
  ( 'spt_NTWPolygons', 'polygon', 4326, 'GEOMETRY', 'XY');


--create an index for polygons 
SELECT CreateSpatialIndex('spt_NTWPolygons', 'polygon');
