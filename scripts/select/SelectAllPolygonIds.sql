--get all the polygon ids from the polygon table  with valid geometries 
SELECT polygonId  
  FROM spt_polygons  WHERE polygon1 IS NOT NULL;
