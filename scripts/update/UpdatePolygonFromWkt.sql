--Update a polygon geometry given its wkt 
UPDATE spt_polygons
  SET ??? = CastToMultiPolygon( ST_ForcePolygonCCW(
    ST_Buffer(
        MPolyFromText( :wkt, 4326 ), 
        0.0                           )
    )
  )
  WHERE polygonId = :polygonId;