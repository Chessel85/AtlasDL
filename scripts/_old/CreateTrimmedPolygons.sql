--Table to store trimmed polygon geometries 

CREATE TABLE spt_TrimmedPolygons (
  id INTEGER 
);

SELECT AddGeometryColumn
  ( 'spt_TrimmedPolygons', 'tpolygon', 4326, 'POLYGON', 'XY');
