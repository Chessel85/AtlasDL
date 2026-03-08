--Table to store point geometries 

CREATE TABLE spt_Points (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
    nodeID INTEGER
);

--Add a point geometry column 
SELECT AddGeometryColumn( 
  'spt_Points', 'point', 4326, 'POINT', 'XY');

--create an index for points 
SELECT CreateSpatialIndex( 'spt_Points', 'point');


