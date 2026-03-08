--Calculate area from polygon but only if is null
--Convert to SRID 6933 global cylindrical projection to get answer in kilometres squared 
UPDATE spt_polygons
SET area = st_area( ST_Transform( polygon , 6933) )  / 1000000
 WHERE area IS NULL;

--And update the relations table summing all constituent polygons 
UPDATE tbl_Relations AS r 
  SET 
  areaCalculated = 
  ( SELECT SUM( p.area ) 
            FROM spt_Polygons AS p 
            WHERE p.elementID = r.relationID )
 WHERE r.areaCalculated  IS NULL;

