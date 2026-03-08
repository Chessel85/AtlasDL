--Table for storing relations that have been downloaded and have at least one polygon in the polygons table 
CREATE TABLE tbl_Relations (
    relationID INTEGER  PRIMARY KEY,
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
CREATE INDEX idx_tbl_relations_relationID ON tbl_Relations (relationID);