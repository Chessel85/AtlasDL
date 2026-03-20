--Populate contains table with points contained by polygons 
INSERT INTO tbl_contains  ( polygonId, geometryCategory, featureClass, containedId, name, longitude, latitude  )
  SELECT 
    a.rowid, 
    1, --indicates a point
    b.featureClass,
    b.pointId,
    b.name,
    b.longitude,
    b.latitude 
  FROM spt_polygons AS a
    JOIN spt_points AS b 
    JOIN tbl_layers AS l ON l.layerId = a.layerId  
  WHERE 
    ( ( a.layerId = 3 OR a.layerId = 4 ) AND b.layerId = 6 )
  AND l.display = 1
  AND a.ROWID IN (                             
      SELECT rowid FROM SpatialIndex 
      WHERE f_table_name = 'spt_polygons' 
      AND search_frame = b.pt 
  )
  AND ST_Contains(a.polygon, b.pt );