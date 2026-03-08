--Identify which polygons contain each other and the layer they each come from
INSERT INTO tbl_links (layerId1, geomId1, linkType, layerId2, geomId2, intersectionDimension, intersectionSize )
  SELECT 
    lp1.layerId AS layerId1, 
    p1.polygonId AS geomId1, 
    1 AS linkType, 
    lp2.layerId AS layerId2, 
    p2.polygonId AS geomId2,
2, --always a 2d dimension 
  p2.area/p1.area  --percentage polygon2 takes up of polygon1 
  FROM spt_polygons AS p1
  JOIN tbl_layerPolygons AS lp1 ON p1.polygonId = lp1.polygonId
  JOIN spt_polygons AS p2 ON p1.polygonId != p2.polygonId
  JOIN tbl_layerPolygons AS lp2 ON p2.polygonId = lp2.polygonId
  WHERE 
  -- 1. Explicit R*Tree Index Filter
  p2.polygonId IN (
      SELECT rowid 
      FROM SpatialIndex 
      WHERE f_table_name = 'spt_polygons' 
        AND f_geometry_column = 'polygon1' -- Double-check this matches your CREATE TABLE
        AND search_frame = p1.polygon1
  )
  -- 2. Prevent impossible checks
  AND ST_Area(p1.polygon1) > ST_Area(p2.polygon1)
  
  -- 3. The Truth verified by our test
  AND ST_Contains(p1.polygon1, p2.polygon1);