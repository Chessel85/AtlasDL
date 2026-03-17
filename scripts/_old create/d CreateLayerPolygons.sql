--Table to store the polygons in a layer as a polygon could go in more than one layer 
--Polygons are grouped into relations in the polygon owner table 
CREATE TABLE tbl_layerPolygons (
  layerId INTEGER,
  polygonId INTEGER,
  FOREIGN KEY( polygonId ) 
    REFERENCES spt_polygons( polygonId ) 
    ON DELETE CASCADE,
  UNIQUE ( layerId, polygonId )
);


--Index for polygonId to speed up visible polygons query
CREATE INDEX idx_tbl_layerPolygons_polygonId ON tbl_layerPolygons(polygonId);
