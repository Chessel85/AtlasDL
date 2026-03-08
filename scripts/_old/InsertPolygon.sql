INSERT INTO spt_Polygons  ( 
  elementType,
  elementID, 
  polygon
) 
VALUES ( ?, ?, ST_PolyFromText(?, 4326 ) );