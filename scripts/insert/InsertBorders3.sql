--Populate borders table with intersecting polygons from oceans and countries 
INSERT INTO tbl_borders (polygon1Id, polygon2Id, name1, name2)
  SELECT 
    a.rowid, 
    b.rowid, 
    a.name, 
    b.name
  FROM spt_polygons AS a
  JOIN spt_polygons AS b 
  WHERE a.ROWID != b.ROWID                       -- Don't compare a polygon to itself
  AND ( ( a.layerId = 1 OR a.layerId = 3 ) 
        AND ( b.layerId = 1  OR b.layerId  = 3 ))
  AND b.ROWID IN (                             
      SELECT rowid FROM SpatialIndex 
      WHERE f_table_name = 'spt_polygons' 
      AND search_frame = a.polygon
  )
  AND ST_Intersects(a.polygon, b.polygon)
  AND NOT ST_CONTAINS( a.polygon, b.polygon );