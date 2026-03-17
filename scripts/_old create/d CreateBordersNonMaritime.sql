--Table to store which polygons border each other 
CREATE TABLE tbl_BordersNonMaritime  (
  fromRelationName TEXT,
  toRelationName TEXT,
  fromRelationID INTEGER,
  toRelationID INTEGER,
  maritime INTEGER, --always zero 
    UNIQUE(fromRelationID, toRelationID) 
);


--Create an index  for from relation ID to speed up retrieval of data 
CREATE INDEX idx_tbl_BordersNonMaritime_fromRelationID ON tbl_BordersNonMaritime( fromRelationID);
