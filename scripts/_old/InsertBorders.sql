--Work out which polygons border each other 
INSERT INTO tbl_Borders 
( fromPolygonID, toPolygonID )
 SELECT p1.id AS fromID, p2.id AS toID  
  FROM spt_Polygons  AS p1 
  JOIN spt_Polygons AS p2 
  WHERE 
    p1.elementID  != p2.elementID  AND 
p1.ROWID IN (
             SELECT ROWID FROM SpatialIndex WHERE(( f_table_name = 'spt_Polygons' )
             AND ( f_geometry_column = 'polygon' )
             AND ( search_frame = p2.polygon  ))
         )
 AND ST_Intersects( p1.polygon, p2.polygon )         
  AND NOT ST_Contains( p1.polygon, p2.polygon )
  AND NOT ST_Contains( p2.polygon, p1.polygon );



--Populate the from name and id columns 
UPDATE tbl_Borders
SET ( fromRelationName, fromRelationID )  = (
  SELECT 
    COALESCE(t2.value, t1.value),
    p.elementID 
  FROM tbl_Tags AS t1
  JOIN spt_Polygons AS p ON p.elementID = t1.elementID
  LEFT JOIN tbl_Tags AS t2 ON ( p.elementID = t2.elementID AND t2.key = 'name:en' )
  WHERE t1.key = 'name'
  AND fromPolygonID = p.id
);


--Populate the to name  and id columns 
UPDATE tbl_Borders
SET ( toRelationName, toRelationID ) = (
  SELECT COALESCE(t2.value, t1.value),
  p.elementID 
  FROM tbl_Tags AS t1
  JOIN spt_Polygons AS p ON p.elementID = t1.elementID
  LEFT JOIN tbl_Tags AS t2 ON ( p.elementID = t2.elementID AND t2.key = 'name:en' )
  WHERE t1.key = 'name'
  AND toPolygonID = p.id
);