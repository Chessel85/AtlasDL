--Table to store polygons and points contained in polygons  
--along with some meta data used in lists 
CREATE TABLE tbl_contains (
    polygonId INTEGER,
    geometryCategory INTEGER,
    featureClass TEXT,
    containedId INTEGER,
  name TEXT,
    longitude DOUBLE,
    latitude DOUBLE
);
