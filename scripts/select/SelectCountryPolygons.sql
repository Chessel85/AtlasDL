--get all the polygon ids belonging to a given relation 
SELECT polygonId 
  FROM tbl_polygonOwners 
  WHERE  elementType = 3 AND elementID = ?;
