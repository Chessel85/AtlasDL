--Populate contains table with polygons and points it contains 
INSERT INTO tbl_contains  ( polygonId, geometryCategory, featureClass, containedId, name, latitude, longitude  )
  SELECT 
    a.rowid, 
    3,
    b.featureClass,
    b.polygonId, 
    b.name,
    b.labelX,
    b.labelY
  FROM spt_polygons AS a
    JOIN spt_polygons AS b 
    JOIN tbl_layers AS l ON l.layerId = a.layerId  
  WHERE a.ROWID != b.ROWID                       -- Do not compare a polygon to itself
    AND ( a.layerId = 3 AND b.layerId = 4 )
  AND l.display = 1
  AND b.ROWID IN (                             
      SELECT rowid FROM SpatialIndex 
      WHERE f_table_name = 'spt_polygons' 
      AND search_frame = a.polygon
  )
  AND ST_Contains(a.polygon, b.polygon);