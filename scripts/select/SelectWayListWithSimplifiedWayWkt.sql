--Return all the ways that make up a given polygon
--Using simplified ways 
SELECT
    wl.innerPolygonId,
    wl.sequenceNumber,
    wl.wayId,
  CASE wl.direction 
    WHEN 1 THEN AsText( w.??? ) --will be way2, way3 or way4 
    ELSE AsText( ST_Reverse( w.??? ) ) --will be way2, way3 or way4 
  END
  FROM 
    tbl_wayLists  AS wl
  JOIN
    spt_ways AS w ON wl.wayId = w.wayId 
  WHERE
    polygonId = :polygonId 
  ORDER BY innerPolygonId, sequenceNumber;