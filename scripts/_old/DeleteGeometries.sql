--Delete polygons based on elementType and a bunch of element ID 
DELETE FROM spt_Polygons WHERE
 elementType = ? AND
  elementID IN ( ? );

