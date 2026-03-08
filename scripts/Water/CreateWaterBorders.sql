--Table to store which polygons border each other 
CREATE TABLE tbl_WaterBorders  (
    fromPkID INTEGER,
    toPkID TEXT,
  fromSeaName TEXT,
  toSeaName TEXT
);

--Create an index  for fromPkID to speed up retrieval of data 
CREATE INDEX idx_tbl_WaterBorders_fromPkID ON tbl_WaterBorders( fromPkID);


