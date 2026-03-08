--Table for storing relations that have been downloaded and have at least one polygon in the polygons table 
CREATE TABLE tbl_countries (
    relationId INTEGER  PRIMARY KEY,
    name TEXT,
  areaCalculated REAL,
  colourIndex INTEGER,
  minX REAL,
  minY REAL,
  maxX REAL,
  maxY REAL,
  labelX REAL,
  labelY REAL,
  midX REAL,
  midY REAL
);

--Create an index 
CREATE INDEX idx_tbl_countries_relationId ON tbl_countries (relationId);