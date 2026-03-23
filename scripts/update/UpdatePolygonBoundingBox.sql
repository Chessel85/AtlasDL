--Update all rows  with simple approach 
UPDATE spt_polygons
  SET 
    minX = MbrMinX( polygon ),
    minY = MbrMinY( polygon ),
    maxX = MbrMaxX( polygon ),
    maxY = MbrMaxY( polygon );

--Second sweep coping with polygons spanning the antimeridian antimeridian 
-- Second sweep: Correcting countries that span the Antimeridian
UPDATE spt_polygons
  SET 
    minX = MbrMinX(ST_Shift_Longitude(polygon)),
    maxX = MbrMaxX(ST_Shift_Longitude(polygon)) 
  WHERE ABS(maxX - minX) > 300;