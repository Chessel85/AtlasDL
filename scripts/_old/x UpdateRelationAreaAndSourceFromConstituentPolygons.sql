--Set the area and source for this area in tbl_Relation
UPDATE tbl_Relations AS r 
  SET 
  area = 
  ( SELECT SUM( p.area ) 
            FROM spt_Polygons p 
            WHERE p.elementID = r.relationID )
 WHERE r.area IS NULL;

