--Calculate area from polygon 
--Want answers in square kilometres 
--Convert to SRID 6933 global cylindrical projection to get answer in square kilometres 
UPDATE spt_polygons
SET area = ST_Area( ST_Transform( polygon1, 6933 ) ) / 1000000.0
  WHERE area IS NULL;
