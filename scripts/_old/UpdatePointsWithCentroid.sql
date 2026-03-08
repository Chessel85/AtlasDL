--Populate the point column with polygon centroid as long as it is null which indicates it has not been populated with a label or other relation  node 
UPDATE spt_polygons
SET point = st_centroid( polygon  ) 
 WHERE point IS NULL;