--Table to store which polygons border each other 
CREATE TABLE tbl_Borders  (
    fromPolygonID INTEGER,  --Interim column that can be deleted once table is populated 
    toPolygonID INTEGER, --Can also be deleted once table populated 
  fromRelationName TEXT,
  toRelationName TEXT,
  fromRelationID INTEGER,
  toRelationID INTEGER 
);


--Create an index  for from relation ID to speed up retrieval of data 
CREATE INDEX idx_tbl_Borders_fromRelationID ON tbl_Borders( fromRelationID);
