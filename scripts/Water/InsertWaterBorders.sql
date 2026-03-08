--Work out which polygons border each other 
INSERT INTO tbl_WaterBorders 
  ( fromPkID, toPkID, fromSeaName, toSeaName  )
    SELECT 
    p1.pk_uid AS fromPkID, 
    p2.pk_uid AS toPkID,
    p1.name,
    p2.name   
  FROM spt_IHOSeas AS p1 
  JOIN spt_IHOSeas  AS p2 
  WHERE 
    p1.pk_uid != p2.pk_uid 
    AND  p1.ROWID IN (
        SELECT ROWID FROM SpatialIndex WHERE
            f_table_name = 'spt_IHOSeas' 
            AND f_geometry_column = 'ihopolygon' 
            AND search_frame = p2.IHOPolygon   
        )
 AND ST_Intersects( p1.ihopolygon, p2.ihopolygon )         
  AND NOT ST_Contains( p1.ihopolygon, p2.ihopolygon )
  AND NOT ST_Contains( p2.ihopolygon, p1.ihopolygon );