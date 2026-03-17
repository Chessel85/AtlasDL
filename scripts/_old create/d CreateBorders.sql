--Table to store which polygons border each other 
CREATE TABLE tbl_Borders  (
  fromRelationName TEXT,
  toRelationName TEXT,
  fromRelationID INTEGER,
  toRelationID INTEGER,
  maritime INTEGER,
    UNIQUE(fromRelationID, toRelationID) 
);


--Create an index  for from relation ID to speed up retrieval of data 
CREATE INDEX idx_tbl_Borders_fromRelationID ON tbl_Borders( fromRelationID);
