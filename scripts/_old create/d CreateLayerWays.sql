--Table to store the ways in a layer 
CREATE TABLE tbl_layerWays  (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  layerId INTEGER,
  wayId INTEGER,
  UNIQUE ( layerId, wayId )
);

--create index for looking up way id
CREATE INDEX idx_tbl_layerWays_wayId ON tbl_layerWays(wayId);