--Delete tags given a particular elementType and a bunch of element ID 
DELETE FROM tbl_Tags WHERE
 elementType = ? AND
  elementID IN ( ? );
