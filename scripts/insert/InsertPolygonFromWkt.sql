--Insert polygon based on wkt 
INSERT INTO spt_polygons (
  polygon4
)
VALUES (
  ST_ForcePolygonCCW(PolyFromText(?))
);