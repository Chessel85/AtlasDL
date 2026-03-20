--Populate borders table with intersecting polygons from the same layer 
INSERT INTO tbl_borders (polygon1Id, polygon2Id, name1, name2)
  SELECT 
    a.rowid, 
    b.rowid, 
    a.name, 
    b.name
  FROM spt_polygons AS a
  JOIN spt_polygons AS b ON a.layerId = b.layerId -- Only compare within the same layer
  WHERE a.ROWID != b.ROWID                       -- Don't compare a polygon to itself
  AND ( a.layerId=4 AND b.layerId = 4 )
  AND b.ROWID IN (                             
      SELECT rowid FROM SpatialIndex 
      WHERE f_table_name = 'spt_polygons' 
      AND search_frame = a.polygon
  )
  AND ST_Intersects(a.polygon, b.polygon);