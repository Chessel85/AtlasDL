--Insert the owner of a polygon id which could be a relation or a single way 
INSERT INTO tbl_polygonOwners ( 
  polygonId, 
  elementType,
    elementID
) 
VALUES ( ?, ?, ? );