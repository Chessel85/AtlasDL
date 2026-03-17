--Table to list ways and how they can be constructed to form multipolygons 
--The geometries for a way are stored separately 
CREATE TABLE tbl_wayLists (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  polygonId INTEGER, -- foreign key from polygon spatial table 
  innerPolygonId INTEGER, -- 0 means not an inner polygon 
  sequenceNumber INTEGER, --starts at 1 
  direction INTEGER, --Forward = 1, backward = -1 and unknown is 0 
  wayId INTEGER,
  FOREIGN KEY ( polygonId )
  REFERENCES spt_polygons ( polygonId )
  ON DELETE CASCADE
);
