--Used to associate island polygon ids against a country relation id 
INSERT OR IGNORE INTO tbl_polygonOwners 
  (
    polygonId,
    elementType,
    elementId
  )
  SELECT DISTINCT
    ip.polygonId, --island polygon id 
    ?, --parent element type
    ? --:parent element id
  FROM
    spt_polygons AS twp  -- The Territorial Water polygon geometry (passed in via parameter)
    JOIN spt_polygons AS ip -- All Island polygon geometries
    JOIN tbl_layerPolygons AS ilp ON ip.polygonId = ilp.polygonId
  WHERE
    twp.polygonId = ?   --the current tw polygon id 
    AND ilp.layerId = ? --island layer id  
    AND ip.ROWID IN (
        SELECT ROWID
        FROM SpatialIndex
        WHERE
            f_table_name = 'spt_polygons'
            AND f_geometry_column = 'polygon1'
            AND search_frame = twp.polygon1 
    )
    AND ST_Contains( twp.polygon1, ip.polygon1);
